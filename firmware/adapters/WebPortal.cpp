/*
Purpose: Web server adapter serving gzipped HTML pages and REST API endpoints.
Responsibilities:
- Serve wizard.html.gz in AP mode, dashboard.html.gz in STA mode via GET /.
- Expose JSON API: GET/POST /api/settings, GET /api/status, GET /api/layout, POST /api/reboot, POST /api/reset, GET /api/wifi/scan.
- Use consistent JSON envelope: { "ok": bool, "data": ..., "error": "...", "code": "..." }.
Architecture: ESPAsyncWebServer (mathieucarbou fork) — non-blocking, runs on AsyncTCP task.

JSON key alignment (matches ConfigManager::updateCacheFromKey / NVS):
  Display:   brightness, text_color_r, text_color_g, text_color_b
  Location:  center_lat, center_lon, radius_km
  Hardware:  tiles_x, tiles_y, tile_pixels, display_pin, origin_corner, scan_dir, zigzag
  Timing:    fetch_interval, display_cycle
  Network:   wifi_ssid, wifi_password, agg_url, agg_token

GET /api/status extended JSON (Story 2.4):
  data.subsystems   — eight subsystem objects (wifi, aggregator, cdn, nvs, littlefs, ota, ntp, ota_pull)
  data.wifi_detail  — SSID, RSSI, IP, mode
  data.device       — uptime_ms, free_heap, fs_total, fs_used
  data.flight       — last_fetch_ms, state_vectors, enriched_flights, logos_matched
  data.quota        — fetches_since_boot, limit, fetch_interval_s, estimated_monthly_polls, over_pace
*/
#include "adapters/WebPortal.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <vector>
#include "core/ConfigManager.h"
#include "config/HardwareConfiguration.h"
#include "core/SystemStatus.h"
#include "core/LogoManager.h"
#include "core/OTAUpdater.h"
#include "models/FlightInfo.h"
#include "utils/Log.h"

// Defined in main.cpp — provides thread-safe flight stats for the health page
extern FlightStatsSnapshot getFlightStatsSnapshot();

// Defined in main.cpp — snapshot of the latest enriched flight list, used by
// the dashboard's Custom Layout preview to show live data while editing.
extern std::vector<FlightInfo> getCurrentFlights();

// Defined in main.cpp — NTP sync status accessor (Story fn-2.1)
extern bool isNtpSynced();

// Defined in main.cpp — Night mode scheduler dimming state (Story fn-2.2)
extern bool isScheduleDimming();

#include "core/LayoutEngine.h"
#include "core/LayoutStore.h"
#include "core/ModeOrchestrator.h"
#include "core/ModeRegistry.h"
#include "widgets/FlightFieldWidget.h"
#include "widgets/MetricWidget.h"

namespace {
struct PendingRequestBody {
    AsyncWebServerRequest* request;
    String body;
};

std::vector<PendingRequestBody> g_pendingBodies;
constexpr size_t MAX_SETTINGS_BODY_BYTES = 4096;

// Logo upload state — streams file data directly to LittleFS
struct LogoUploadState {
    AsyncWebServerRequest* request;
    String filename;
    String path;
    File file;
    bool valid;
    bool written;
    String error;
    String errorCode;
};

std::vector<LogoUploadState> g_logoUploads;

LogoUploadState* findLogoUpload(AsyncWebServerRequest* request) {
    for (auto& u : g_logoUploads) {
        if (u.request == request) return &u;
    }
    return nullptr;
}

void clearLogoUpload(AsyncWebServerRequest* request, bool removeFile = false) {
    for (auto it = g_logoUploads.begin(); it != g_logoUploads.end(); ++it) {
        if (it->request == request) {
            if (it->file) it->file.close();
            if (removeFile && it->path.length()) {
                LittleFS.remove(it->path);
            }
            g_logoUploads.erase(it);
            return;
        }
    }
}

// OTA upload state — streams firmware directly to flash via Update library
struct OTAUploadState {
    AsyncWebServerRequest* request;
    bool valid;           // false if any validation/write failed
    bool started;         // true after Update.begin() succeeds
    size_t bytesWritten;  // for debugging/logging only
    String error;         // human-readable error message
    String errorCode;     // machine-readable error code
};

std::vector<OTAUploadState> g_otaUploads;
static bool g_otaInProgress = false;  // Enforce single-flight OTA — Update is a singleton

OTAUploadState* findOTAUpload(AsyncWebServerRequest* request) {
    for (auto& u : g_otaUploads) {
        if (u.request == request) return &u;
    }
    return nullptr;
}

void clearOTAUpload(AsyncWebServerRequest* request) {
    for (auto it = g_otaUploads.begin(); it != g_otaUploads.end(); ++it) {
        if (it->request == request) {
            // CRITICAL: abort in-progress update on cleanup (started=false means already completed)
            if (it->started) {
                Update.abort();
                LOG_I("OTA", "Upload aborted during cleanup");
            }
            g_otaUploads.erase(it);
            g_otaInProgress = false;
            return;
        }
    }
}

PendingRequestBody* findPendingBody(AsyncWebServerRequest* request) {
    for (auto& pending : g_pendingBodies) {
        if (pending.request == request) {
            return &pending;
        }
    }
    return nullptr;
}

// Shared SwitchState → string mapping used by GET and POST mode handlers.
// Centralised here to avoid copy-paste drift between the two call sites.
const char* switchStateToString(SwitchState ss) {
    switch (ss) {
        case SwitchState::REQUESTED: return "requested";
        case SwitchState::SWITCHING: return "switching";
        case SwitchState::FAILED:    return "failed";  // BF-1 AC #5
        default:                     return "idle";
    }
}

void clearPendingBody(AsyncWebServerRequest* request) {
    for (auto it = g_pendingBodies.begin(); it != g_pendingBodies.end(); ++it) {
        if (it->request == request) {
            g_pendingBodies.erase(it);
            return;
        }
    }
}
} // namespace

void WebPortal::init(AsyncWebServer& server, WiFiManager& wifiMgr) {
    _server = &server;
    _wifiMgr = &wifiMgr;
    _registerRoutes();
    LOG_I("WebPortal", "Routes registered");
}

void WebPortal::begin() {
    if (!_server) return;
    _server->begin();
    LOG_I("WebPortal", "Server started on port 80");
}

void WebPortal::onReboot(RebootCallback callback) {
    _rebootCallback = callback;
}

void WebPortal::onCalibration(CalibrationCallback callback) {
    _calibrationCallback = callback;
}

void WebPortal::onPositioning(PositioningCallback callback) {
    _positioningCallback = callback;
}

void WebPortal::_registerRoutes() {
    // GET / — serve wizard or dashboard based on WiFi mode
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleRoot(request);
    });

    // GET /api/settings
    _server->on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetSettings(request);
    });

    // POST /api/settings — uses body handler for JSON parsing
    _server->on("/api/settings", HTTP_POST,
        // request handler (called after body is received)
        [](AsyncWebServerRequest* request) {
            // no-op: response sent in body handler
        },
        nullptr, // upload handler
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (data == nullptr || total == 0) {
                clearPendingBody(request);
                _sendJsonError(request, 400, "Empty settings object", "EMPTY_PAYLOAD");
                return;
            }

            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }

            if (index == 0) {
                clearPendingBody(request);
                // push_back first, then reserve on the stored element — avoids capacity
                // loss from the String copy constructor during push_back (synthesis ds-3.2 Pass 3).
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }

            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }

            pending->body.concat(reinterpret_cast<const char*>(data), len);

            if (index + len == total) {
                _handlePostSettings(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // GET /api/status
    _server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetStatus(request);
    });

    // GET /api/flights/current — snapshot of the latest enriched flights,
    // consumed by the Custom Layout preview for live-data rendering.
    _server->on("/api/flights/current", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetCurrentFlights(request);
    });

    // POST /api/reboot
    _server->on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostReboot(request);
    });

    // POST /api/reset — factory reset (erase NVS + restart)
    _server->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostReset(request);
    });

    // GET /api/wifi/scan
    _server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetWifiScan(request);
    });

    // GET /api/layout (Story 3.1)
    _server->on("/api/layout", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLayout(request);
    });

    // POST /api/calibration/start (Story 4.2) — gradient pattern
    _server->on("/api/calibration/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostCalibrationStart(request);
    });

    // POST /api/positioning/start — panel positioning guide (independent from calibration)
    _server->on("/api/positioning/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (_positioningCallback) {
            _positioningCallback(true);
        }
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["message"] = "Positioning mode started";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // POST /api/positioning/stop
    _server->on("/api/positioning/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (_positioningCallback) {
            _positioningCallback(false);
        }
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["message"] = "Positioning mode stopped";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // POST /api/calibration/stop (Story 4.2)
    _server->on("/api/calibration/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostCalibrationStop(request);
    });

    // GET /api/display/modes (Story dl-1.5) — mode list with orchestrator state
    _server->on("/api/display/modes", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetDisplayModes(request);
    });

    // POST /api/display/mode (Story ds-3.1) — manual mode switch
    // AC #5: accept "mode" with fallback to "mode_id" for backward compat
    // AC #6: orchestrator-only path (Rule 24)
    // AC #7: async response with switch_state — no bounded wait in async handler
    //   Strategy: return switch_state: "requested" with client re-fetch (ds-3.4 UX).
    //   Architecture D5 "synchronous" intent is not feasible in ESPAsyncWebServer body
    //   handler because tick() runs on Core 0 display task. Explicit deviation documented.
    _server->on("/api/display/mode", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (total == 0 || data == nullptr) {
                _sendJsonError(request, 400, "Empty request body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                // push_back first, then reserve on the stored element — avoids capacity
                // loss from the String copy constructor during push_back (synthesis ds-3.2 Pass 3).
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePostDisplayMode(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // GET /api/schedule (Story dl-4.2) — mode schedule rules
    _server->on("/api/schedule", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetSchedule(request);
    });

    // POST /api/schedule (Story dl-4.2) — replace mode schedule rules
    _server->on("/api/schedule", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (total == 0 || data == nullptr) {
                _sendJsonError(request, 400, "Empty request body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                // push_back first, then reserve on the stored element — avoids capacity
                // loss from the String copy constructor during push_back (synthesis ds-3.2 Pass 3).
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePostSchedule(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // GET /api/ota/check (Story dl-6.2) — check GitHub for firmware update
    _server->on("/api/ota/check", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetOtaCheck(request);
    });

    // POST /api/ota/pull (Story dl-7.3, AC #1–#2) — start pull OTA download
    _server->on("/api/ota/pull", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostOtaPull(request);
    });

    // GET /api/ota/status (Story dl-7.3, AC #1, #3) — OTA state/progress polling
    _server->on("/api/ota/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetOtaStatus(request);
    });

    // POST /api/display/notification/dismiss (Story ds-3.1, AC #10; ds-3.2, AC #4)
    // Clears upgrade_notification NVS flag so GET returns false thereafter.
    // Uses ConfigManager::setUpgNotif() to keep NVS writes centralized (AR7).
    _server->on("/api/display/notification/dismiss", HTTP_POST, [](AsyncWebServerRequest* request) {
        ConfigManager::setUpgNotif(false);

        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // GET /api/logos — list uploaded logos
    _server->on("/api/logos", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLogos(request);
    });

    // POST /api/logos — multipart file upload (streaming to LittleFS)
    _server->on("/api/logos", HTTP_POST,
        // Request handler — called after upload completes
        [this](AsyncWebServerRequest* request) {
            auto* state = findLogoUpload(request);
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();

            if (!state || !state->valid) {
                root["ok"] = false;
                root["error"] = (state && state->error.length()) ? state->error : "Upload failed";
                root["code"] = (state && state->errorCode.length()) ? state->errorCode : "UPLOAD_ERROR";
                String output;
                serializeJson(doc, output);
                clearLogoUpload(request, true);
                request->send(400, "application/json", output);
                return;
            }

            root["ok"] = true;
            JsonObject data = root["data"].to<JsonObject>();
            data["filename"] = state->filename;
            data["size"] = LOGO_BUFFER_BYTES;

            String output;
            serializeJson(doc, output);
            clearLogoUpload(request);
            LogoManager::scanLogoCount();
            request->send(200, "application/json", output);
        },
        // Upload handler — called for each chunk of file data
        [this](AsyncWebServerRequest* request, const String& filename,
               size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                // First chunk — validate and open file for writing
                clearLogoUpload(request);
                LogoUploadState state;
                state.request = request;
                state.filename = filename;
                state.path = String("/logos/") + filename;
                state.valid = true;
                state.written = false;

                // Validate filename before touching LittleFS.
                if (!LogoManager::isSafeLogoFilename(filename)) {
                    state.valid = false;
                    state.error = filename + " - invalid filename";
                    state.errorCode = "INVALID_NAME";
                    g_logoUploads.push_back(state);
                    return;
                }

                request->onDisconnect([request]() {
                    clearLogoUpload(request, true);
                });

                // Open file for streaming write
                state.file = LittleFS.open(state.path, "w");
                if (!state.file) {
                    state.valid = false;
                    state.error = "LittleFS full or write error";
                    state.errorCode = "FS_WRITE_ERROR";
                }
                g_logoUploads.push_back(state);
            }

            auto* state = findLogoUpload(request);
            if (!state || !state->valid) return;

            // Stream write chunk to file
            if (state->file && len > 0) {
                size_t written = state->file.write(data, len);
                if (written != len) {
                    state->valid = false;
                    state->error = "LittleFS full";
                    state->errorCode = "FS_FULL";
                    state->file.close();
                    LittleFS.remove(state->path);
                    return;
                }
            }

            if (final) {
                size_t totalSize = index + len;
                if (state->file) state->file.close();

                if (totalSize != LOGO_BUFFER_BYTES) {
                    state->valid = false;
                    state->error = state->filename + " - invalid size (" + String((unsigned long)totalSize) + " bytes, expected 2048)";
                    state->errorCode = "INVALID_SIZE";
                    LittleFS.remove(state->path);
                    return;
                }

                state->written = true;
            }
        }
    );

    // DELETE /api/logos/:name
    _server->on("/api/logos/*", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        _handleDeleteLogo(request);
    });

    // GET /logos/:name — serve raw logo binary for thumbnail rendering
    _server->on("/logos/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLogoFile(request);
    });

    // POST /api/ota/upload — firmware OTA upload (streaming to flash)
    _server->on("/api/ota/upload", HTTP_POST,
        // Request handler — called after upload completes
        [this](AsyncWebServerRequest* request) {
            auto* state = findOTAUpload(request);
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();

            if (!state || !state->valid) {
                root["ok"] = false;
                root["error"] = (state && state->error.length()) ? state->error : "Upload failed";
                const String errCode = (state && state->errorCode.length()) ? state->errorCode : "UPLOAD_ERROR";
                root["code"] = errCode;
                // Map error codes to semantically correct HTTP status codes
                int httpCode = 400;  // default: client error (e.g. bad firmware file)
                if (errCode == "OTA_BUSY") httpCode = 409;  // Conflict
                else if (errCode == "NO_OTA_PARTITION" || errCode == "BEGIN_FAILED" ||
                         errCode == "WRITE_FAILED"     || errCode == "VERIFY_FAILED") httpCode = 500;
                String output;
                serializeJson(doc, output);
                clearOTAUpload(request);
                request->send(httpCode, "application/json", output);
                return;
            }

            // Success — schedule reboot
            root["ok"] = true;
            root["message"] = "Rebooting...";
            String output;
            serializeJson(doc, output);
            clearOTAUpload(request);

            SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Update complete — rebooting");
            LOG_I("OTA", "Upload complete, scheduling reboot");

            request->send(200, "application/json", output);

            // Schedule reboot after 500ms to allow response to be sent
            static esp_timer_handle_t otaRebootTimer = nullptr;
            if (!otaRebootTimer) {
                esp_timer_create_args_t args = {};
                args.callback = [](void*) { ESP.restart(); };
                args.name = "ota_reboot";
                esp_timer_create(&args, &otaRebootTimer);
            }
            esp_timer_start_once(otaRebootTimer, 500000); // 500ms in microseconds
        },
        // Upload handler — called for each chunk of firmware data
        [this](AsyncWebServerRequest* request, const String& filename,
               size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                // First chunk — validate magic byte and begin update
                clearOTAUpload(request);

                // Reject concurrent OTA uploads — Update singleton is not re-entrant
                if (g_otaInProgress) {
                    OTAUploadState busy;
                    busy.request = request;
                    busy.valid = false;
                    busy.started = false;
                    busy.bytesWritten = 0;
                    busy.error = "Another OTA update is already in progress";
                    busy.errorCode = "OTA_BUSY";
                    g_otaUploads.push_back(busy);
                    LOG_I("OTA", "Rejected concurrent OTA upload");
                    return;
                }

                OTAUploadState state;
                state.request = request;
                state.valid = true;
                state.started = false;
                state.bytesWritten = 0;

                // Register disconnect handler for WiFi interruption
                request->onDisconnect([request]() {
                    auto* s = findOTAUpload(request);
                    if (s && s->started) {
                        Update.abort();
                        LOG_I("OTA", "Upload aborted due to disconnect");
                    }
                    clearOTAUpload(request);
                });

                // Validate ESP32 magic byte (0xE9)
                if (len == 0 || data[0] != 0xE9) {
                    state.valid = false;
                    state.error = "Not a valid ESP32 firmware image";
                    state.errorCode = "INVALID_FIRMWARE";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Invalid firmware image");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "Invalid firmware magic byte");
                    return;
                }

                // Get next OTA partition
                const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
                if (!partition) {
                    state.valid = false;
                    state.error = "No OTA partition available";
                    state.errorCode = "NO_OTA_PARTITION";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "No OTA partition found");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "No OTA partition found");
                    return;
                }

                // Story dl-7.1, AC #9: tear down display mode before flash write
                // so push and pull OTA share the same safe teardown path.
                if (!ModeRegistry::prepareForOTA()) {
                    // prepareForOTA failed — abort upload with clear error
                    state.valid = false;
                    state.error = "Could not prepare display for OTA";
                    state.errorCode = "PREPARE_OTA_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Display teardown failed");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "prepareForOTA() failed — aborting upload");
                    return;
                }

                // Begin update with partition size (NOT Content-Length per AR18)
                if (!Update.begin(partition->size)) {
                    state.valid = false;
                    state.error = "Could not begin OTA update";
                    state.errorCode = "BEGIN_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Could not begin OTA update");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "Update.begin() failed");
                    return;
                }

                state.started = true;
                g_otaInProgress = true;
                g_otaUploads.push_back(state);
                Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size);
                SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Upload in progress");
                LOG_I("OTA", "Update started");
            }

            auto* state = findOTAUpload(request);
            if (!state || !state->valid) return;

            // Stream write chunk to flash
            if (len > 0) {
                size_t written = Update.write(data, len);
                if (written != len) {
                    Update.abort();
                    state->valid = false;
                    state->error = "Write failed — flash may be worn or corrupted";
                    state->errorCode = "WRITE_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Write failed");
                    LOG_E("OTA", "Flash write failed");
                    return;
                }
                state->bytesWritten += len;
            }

            if (final) {
                // Finalize update
                if (!Update.end(true)) {
                    Update.abort();
                    state->valid = false;
                    state->error = "Firmware verification failed";
                    state->errorCode = "VERIFY_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Verification failed");
                    LOG_E("OTA", "Firmware verification failed");
                    return;
                }
                // Clear started so clearOTAUpload does NOT call Update.abort() on a completed update
                state->started = false;

                // Reset rollback acknowledgment so a future rollback shows the banner again (Story fn-1.6)
                Preferences otaPrefs;
                if (otaPrefs.begin("flightwall", false)) {
                    otaPrefs.putUChar("ota_rb_ack", 0);
                    otaPrefs.end();
                } else {
                    LOG_E("OTA", "Failed to open NVS to clear rollback ack — banner may stay dismissed after rollback");
                }

                LOG_I("OTA", "Update finalized successfully");
            }
        }
    );

    // POST /api/ota/ack-rollback — dismiss rollback banner (Story fn-1.6)
    _server->on("/api/ota/ack-rollback", HTTP_POST, [](AsyncWebServerRequest* request) {
        Preferences prefs;
        if (!prefs.begin("flightwall", false)) {
            request->send(500, "application/json", "{\"ok\":false,\"error\":\"NVS access failed\",\"code\":\"NVS_ERROR\"}");
            return;
        }
        size_t written = prefs.putUChar("ota_rb_ack", 1);
        prefs.end();
        if (written == 0) {
            request->send(500, "application/json", "{\"ok\":false,\"error\":\"Failed to save acknowledgment\",\"code\":\"NVS_WRITE_ERROR\"}");
            return;
        }

        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // GET /api/settings/export — download config as JSON file (Story fn-1.6)
    _server->on("/api/settings/export", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["flightwall_settings_version"] = 1;

        // Timestamp — use NTP time if available, else uptime
        time_t now;
        time(&now);
        if (now > 1000000000) {  // NTP synced (past year 2001)
            char buf[32];
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
            root["exported_at"] = String(buf);
        } else {
            root["exported_at"] = String("uptime_ms_") + String(millis());
        }

        ConfigManager::dumpSettingsJson(root);

        String output;
        serializeJson(doc, output);

        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", output);
        response->addHeader("Content-Disposition", "attachment; filename=flightwall-settings.json");
        request->send(response);
    });

    // ─── Layout CRUD (LE-1.4) ───
    // Register specific paths before wildcards. POST /api/layouts/*/activate
    // is a distinct path from POST /api/layouts (exact) and POST /api/layouts/*
    // would overlap with activate — but ESPAsyncWebServer matches the longer
    // activate path first. Register activate ahead of the generic PUT/DELETE
    // wildcards for safety.
    _server->on("/api/layouts", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLayouts(request);
    });

    _server->on("/api/layouts", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (data == nullptr || total == 0) {
                clearPendingBody(request);
                _sendJsonError(request, 400, "Empty body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > LAYOUT_STORE_MAX_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() { clearPendingBody(request); });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePostLayout(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // Register activate BEFORE generic wildcards so longer/more specific
    // path matches first on the mathieucarbou fork.
    _server->on("/api/layouts/*/activate", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostLayoutActivate(request);
    });

    _server->on("/api/layouts/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLayoutById(request);
    });

    _server->on("/api/layouts/*", HTTP_PUT,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (data == nullptr || total == 0) {
                clearPendingBody(request);
                _sendJsonError(request, 400, "Empty body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > LAYOUT_STORE_MAX_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() { clearPendingBody(request); });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePutLayout(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    _server->on("/api/layouts/*", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        _handleDeleteLayout(request);
    });

    _server->on("/api/widgets/types", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetWidgetTypes(request);
    });

    // Shared static assets (gzipped on LittleFS)
    _server->on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/style.css.gz", "text/css");
    });
    _server->on("/common.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/common.js.gz", "application/javascript");
    });
    _server->on("/wizard.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/wizard.js.gz", "application/javascript");
    });
    _server->on("/dashboard.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/dashboard.js.gz", "application/javascript");
    });
    _server->on("/health.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/health.html.gz", "text/html");
    });
    _server->on("/health.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/health.js.gz", "application/javascript");
    });
    // Layout Editor assets (LE-1.6)
    _server->on("/editor.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.html.gz", "text/html");
    });
    _server->on("/editor.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.js.gz", "application/javascript");
    });
    _server->on("/editor.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.css.gz", "text/css");
    });
    // Pixel-accurate layout preview: glyph data + renderer (served to the
    // editor page; not loaded from any other route).
    _server->on("/preview-glyphs.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/preview-glyphs.js.gz", "application/javascript");
    });
    _server->on("/preview.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/preview.js.gz", "application/javascript");
    });
}

void WebPortal::_handleRoot(AsyncWebServerRequest* request) {
    WiFiState state = _wifiMgr->getState();
    const char* file;
    if (state == WiFiState::AP_SETUP || state == WiFiState::AP_FALLBACK) {
        file = "/wizard.html.gz";
    } else {
        // STA_CONNECTED, CONNECTING, STA_RECONNECTING — serve dashboard
        file = "/dashboard.html.gz";
    }

    if (!LittleFS.exists(file)) {
        _sendJsonError(request, 404, "Asset not found", "ASSET_MISSING");
        return;
    }

    AsyncWebServerResponse* response = request->beginResponse(LittleFS, file, "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void WebPortal::_handleGetSettings(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    ConfigManager::dumpSettingsJson(data);

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostSettings(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "PARSE_ERROR");
        return;
    }

    if (!doc.is<JsonObject>()) {
        _sendJsonError(request, 400, "Expected JSON object", "INVALID_PAYLOAD");
        return;
    }

    JsonObject settings = doc.as<JsonObject>();
    if (settings.size() == 0) {
        _sendJsonError(request, 400, "Empty settings object", "EMPTY_PAYLOAD");
        return;
    }

    ApplyResult result = ConfigManager::applyJson(settings);
    if (result.applied.size() != settings.size()) {
        _sendJsonError(request, 400, "Unknown or invalid settings key", "INVALID_SETTING");
        return;
    }

    JsonDocument respDoc;
    JsonObject resp = respDoc.to<JsonObject>();
    resp["ok"] = true;
    JsonArray applied = resp["applied"].to<JsonArray>();
    for (const String& key : result.applied) {
        applied.add(key);
    }
    resp["reboot_required"] = result.reboot_required;

    // Persist the reboot-pending flag so the dashboard can rehydrate the
    // sticky bar across refreshes. Only *setting* it here — clearing happens
    // naturally when the device reboots (flag is default-initialized false).
    if (result.reboot_required) {
        _rebootPending.store(true, std::memory_order_release);
    }

    String output;
    serializeJson(respDoc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    FlightStatsSnapshot stats = getFlightStatsSnapshot();
    SystemStatus::toExtendedJson(data, stats);

    // NTP and schedule status (Story fn-2.1, fn-2.2)
    data["ntp_synced"] = isNtpSynced();
    data["schedule_active"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();
    data["schedule_dimming"] = isScheduleDimming();

    // Rollback acknowledgment state (Story fn-1.6) — NVS-backed, not via ConfigManager
    Preferences prefs;
    prefs.begin("flightwall", true);  // read-only
    data["rollback_acknowledged"] = prefs.getUChar("ota_rb_ack", 0) == 1;
    prefs.end();

    // OTA availability from last check (Story dl-6.2, AC #3)
    bool otaAvail = (OTAUpdater::getState() == OTAState::AVAILABLE);
    data["ota_available"] = otaAvail;
    data["ota_version"] = otaAvail ? (const char*)OTAUpdater::getRemoteVersion()
                                   : (const char*)nullptr;

    // Reboot-pending flag — set true when a reboot-required setting was saved
    // since boot but the device has not yet rebooted. Drives the dashboard's
    // sticky "Reboot Now" bar; cleared implicitly by the device reboot.
    data["reboot_pending"] = _rebootPending.load(std::memory_order_acquire);

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetCurrentFlights(AsyncWebServerRequest* request) {
    // Snapshot is a copy — getCurrentFlights() handles cross-task safety by
    // reading from the stable reader side of the double buffer. We serialize
    // the minimal field set the preview needs (ident, operator, route, live
    // telemetry) to keep the payload small for frequent polling.
    std::vector<FlightInfo> flights = getCurrentFlights();

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    JsonArray arr = data["flights"].to<JsonArray>();

    auto addNumber = [](JsonObject& obj, const char* key, double value) {
        if (!isnan(value)) {
            obj[key] = value;
        } else {
            obj[key] = (const char*)nullptr;
        }
    };

    for (const FlightInfo& f : flights) {
        JsonObject row = arr.add<JsonObject>();
        row["ident"] = f.ident;
        row["ident_icao"] = f.ident_icao;
        row["ident_iata"] = f.ident_iata;
        row["operator_code"] = f.operator_code;
        row["operator_icao"] = f.operator_icao;
        row["operator_iata"] = f.operator_iata;
        row["origin_icao"] = f.origin.code_icao;
        row["destination_icao"] = f.destination.code_icao;
        row["aircraft_code"] = f.aircraft_code;
        row["airline_display_name_full"] = f.airline_display_name_full;
        row["aircraft_display_name_short"] = f.aircraft_display_name_short;
        row["aircraft_display_name_full"] = f.aircraft_display_name_full;
        addNumber(row, "altitude_kft", f.altitude_kft);
        addNumber(row, "speed_mph", f.speed_mph);
        addNumber(row, "track_deg", f.track_deg);
        addNumber(row, "vertical_rate_fps", f.vertical_rate_fps);
        addNumber(row, "distance_km", f.distance_km);
        addNumber(row, "bearing_deg", f.bearing_deg);
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostReboot(AsyncWebServerRequest* request) {
    // Flush all pending NVS writes before restart
    ConfigManager::persistAllNow();

    // Notify main coordinator to show "Saving config..." on LED
    if (_rebootCallback) {
        _rebootCallback();
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Rebooting...";

    String output;
    serializeJson(doc, output);

    // Send response first, then schedule restart
    request->send(200, "application/json", output);

    // Delay restart slightly so the HTTP response can be sent
    // Use a one-shot timer to avoid blocking the async callback
    static esp_timer_handle_t rebootTimer = nullptr;
    if (!rebootTimer) {
        esp_timer_create_args_t args = {};
        args.callback = [](void*) { ESP.restart(); };
        args.name = "reboot";
        esp_timer_create(&args, &rebootTimer);
    }
    esp_timer_start_once(rebootTimer, 1000000); // 1 second in microseconds
}

void WebPortal::_handlePostReset(AsyncWebServerRequest* request) {
    // Erase NVS config and restore compile-time defaults
    if (!ConfigManager::factoryReset()) {
        _sendJsonError(request, 500, "Factory reset failed", "RESET_FAILED");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Factory reset complete. Rebooting...";

    String output;
    serializeJson(doc, output);

    // Send response first, then schedule restart (reuse reboot timer pattern)
    request->send(200, "application/json", output);

    static esp_timer_handle_t resetTimer = nullptr;
    if (!resetTimer) {
        esp_timer_create_args_t args = {};
        args.callback = [](void*) { ESP.restart(); };
        args.name = "reset";
        esp_timer_create(&args, &resetTimer);
    }
    esp_timer_start_once(resetTimer, 1000000); // 1 second in microseconds
}

void WebPortal::_handleGetWifiScan(AsyncWebServerRequest* request) {
    int16_t scanResult = WiFi.scanComplete();

    if (scanResult == WIFI_SCAN_FAILED) {
        // No scan running — kick off an async scan
        WiFi.scanNetworks(true); // async=true
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["scanning"] = true;
        root["data"].to<JsonArray>();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    if (scanResult == WIFI_SCAN_RUNNING) {
        // Scan still in progress
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["scanning"] = true;
        root["data"].to<JsonArray>();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    // Scan complete — return results
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["scanning"] = false;
    JsonArray data = root["data"].to<JsonArray>();

    for (int i = 0; i < scanResult; i++) {
        JsonObject net = data.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
    }

    // Free scan memory for next scan
    WiFi.scanDelete();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetDisplayModes(AsyncWebServerRequest* request) {
    // BF-1 AC #5: lazy stall watchdog — promote a stuck REQUESTED to FAILED before
    // we read switch state below, so the dashboard sees the failure on this poll
    // instead of timing out client-side.
    ModeRegistry::pollAndAdvanceStall();

    // Capacity: ~256 base + ~200 per mode (with zone_layout) — 3 modes ≈ 900 bytes
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // Build modes array from ModeRegistry (AC #1, #2, #4 — no heap allocation per request)
    JsonArray modes = data["modes"].to<JsonArray>();
    const ModeEntry* table = ModeRegistry::getModeTable();
    const uint8_t count = ModeRegistry::getModeCount();

    // Active mode from ModeRegistry (truth source); fall back to "classic_card" if nullptr
    const char* registryActiveId = ModeRegistry::getActiveModeId();
    const char* activeId = registryActiveId ? registryActiveId : "classic_card";

    for (uint8_t i = 0; i < count; i++) {
        const ModeEntry& entry = table[i];
        JsonObject modeObj = modes.add<JsonObject>();
        modeObj["id"] = entry.id;
        modeObj["name"] = entry.displayName;
        modeObj["active"] = (strcmp(activeId, entry.id) == 0);

        // Zone layout from static descriptor (AC #2) — walks BSS/static data only
        JsonArray zoneLayout = modeObj["zone_layout"].to<JsonArray>();
        if (entry.zoneDescriptor != nullptr) {
            if (entry.zoneDescriptor->description != nullptr) {
                modeObj["description"] = entry.zoneDescriptor->description;
            }
            for (uint8_t z = 0; z < entry.zoneDescriptor->regionCount; z++) {
                const ZoneRegion& region = entry.zoneDescriptor->regions[z];
                JsonObject zoneObj = zoneLayout.add<JsonObject>();
                zoneObj["label"] = region.label;
                zoneObj["relX"] = region.relX;
                zoneObj["relY"] = region.relY;
                zoneObj["relW"] = region.relW;
                zoneObj["relH"] = region.relH;
            }
        }

        // Per-mode settings from settingsSchema (Story dl-5.1, AC #1–#2)
        // Loops ModeEntry.settingsSchema only — no hardcoded key lists (rule 28).
        // Adding a ModeSettingDef to any schema auto-appears here.
        if (entry.settingsSchema != nullptr && entry.settingsSchema->settingCount > 0) {
            JsonArray settings = modeObj["settings"].to<JsonArray>();
            const ModeSettingsSchema* schema = entry.settingsSchema;
            for (uint8_t s = 0; s < schema->settingCount; s++) {
                const ModeSettingDef& def = schema->settings[s];
                JsonObject settingObj = settings.add<JsonObject>();
                settingObj["key"] = def.key;
                settingObj["label"] = def.label;
                settingObj["type"] = def.type;
                settingObj["default"] = def.defaultValue;
                settingObj["min"] = def.minValue;
                settingObj["max"] = def.maxValue;
                if (def.enumOptions != nullptr) {
                    settingObj["enumOptions"] = def.enumOptions;
                } else {
                    settingObj["enumOptions"] = (const char*)nullptr;
                }
                // Current persisted value via ConfigManager (uses modeAbbrev for NVS key)
                settingObj["value"] = ConfigManager::getModeSetting(
                    schema->modeAbbrev, def.key, def.defaultValue);
            }
        } else {
            modeObj["settings"] = (const char*)nullptr;  // null for modes without settings
        }
    }

    data["active"] = activeId;

    // Switch state from ModeRegistry (AC #1, replacing stub "idle")
    data["switch_state"] = switchStateToString(ModeRegistry::getSwitchState());

    // Orchestrator transparency (AC #3 — retain dl-1.5 fields)
    data["orchestrator_state"] = ModeOrchestrator::getStateString();
    data["state_reason"] = ModeOrchestrator::getStateReason();

    // Registry error if present (AC #9)
    // Use copyLastError() — thread-safe snapshot for Core 1 (synthesis ds-3.2 Pass 3).
    // getLastError() returns a raw pointer to a Core 0 mutable buffer; unsafe here.
    char regErrBuf[64];
    ModeRegistry::copyLastError(regErrBuf, sizeof(regErrBuf));
    if (regErrBuf[0] != '\0') {
        data["registry_error"] = regErrBuf;
    }

    // BF-1 AC #4: surface preemption source so the dashboard can show "Calibration
    // stopped. Switched to <mode>" toast once per click. Sticky until next requestSwitch().
    const char* preemptedSource = ModeRegistry::getLastPreemptionSource();
    if (preemptedSource != nullptr) {
        data["preempted"] = preemptedSource;
    }

    // BF-1 AC #5: stall watchdog error envelope. When pollAndAdvanceStall() above
    // promoted state to FAILED, surface the stable error code so the client can
    // render a "mode switch stalled" message instead of polling forever.
    if (ModeRegistry::getSwitchState() == SwitchState::FAILED) {
        const char* code = ModeRegistry::getLastErrorCode();
        if (code != nullptr) {
            data["switch_error_code"] = code;
            // Reuse top-level "error" / "ok" envelope: ok stays true (this is an
            // observation, not a request failure) but error gives a human string.
            root["error"] = "Mode switch stalled";
            root["code"] = code;
        }
    }

    // Upgrade notification NVS flag (AC #10; ds-3.2, AC #4)
    // Uses ConfigManager::getUpgNotif() to keep NVS reads centralized (AR7).
    data["upgrade_notification"] = ConfigManager::getUpgNotif();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

// POST /api/display/mode (Story ds-3.1)
// AC #5: accept "mode" with fallback to "mode_id" for one release.
// AC #6 / Rule 24: always route user intent through orchestrator — even when re-selecting
//   the current mode — so the orchestrator exits IDLE_FALLBACK / SCHEDULED and returns
//   to MANUAL state as the user explicitly intended.
//   ModeRegistry::tick() is idempotent when requested == active (line 367 guard), so
//   calling onManualSwitch for the same mode does not restart the running DisplayMode.
// AC #7: async strategy (documented deviation from architecture D5 — bounded wait not
//   feasible in ESPAsyncWebServer body handler; client re-fetches via ds-3.4 UX).
void WebPortal::_handlePostDisplayMode(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument reqDoc;
    DeserializationError err = deserializeJson(reqDoc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "INVALID_JSON");
        return;
    }

    // AC #5: prefer "mode", fall back to "mode_id" for one release
    const char* modeId = reqDoc["mode"] | (const char*)nullptr;
    if (!modeId) {
        modeId = reqDoc["mode_id"] | (const char*)nullptr;
    }
    if (!modeId) {
        _sendJsonError(request, 400, "Missing mode or mode_id field", "MISSING_FIELD");
        return;
    }

    // Resolve mode from ModeRegistry (AC #8 — unknown mode → 400)
    const ModeEntry* table = ModeRegistry::getModeTable();
    const uint8_t count = ModeRegistry::getModeCount();
    const ModeEntry* matchedEntry = nullptr;
    for (uint8_t i = 0; i < count; i++) {
        if (strcmp(table[i].id, modeId) == 0) {
            matchedEntry = &table[i];
            break;
        }
    }
    if (!matchedEntry) {
        _sendJsonError(request, 400, "Unknown mode_id", "UNKNOWN_MODE");
        return;
    }

    // Story dl-5.1, AC #4: handle optional "settings" object
    bool hasSettings = reqDoc["settings"].is<JsonObject>();
    bool settingsApplied = false;
    if (hasSettings) {
        JsonObject settingsObj = reqDoc["settings"].as<JsonObject>();

        // Validate all keys exist in schema before applying any (no partial apply)
        if (matchedEntry->settingsSchema == nullptr || matchedEntry->settingsSchema->settingCount == 0) {
            if (settingsObj.size() > 0) {
                _sendJsonError(request, 400, "Mode has no configurable settings", "UNKNOWN_SETTING");
                return;
            }
        } else {
            const ModeSettingsSchema* schema = matchedEntry->settingsSchema;
            // Pre-validate: check all keys exist in schema
            for (JsonPair kv : settingsObj) {
                bool found = false;
                for (uint8_t s = 0; s < schema->settingCount; s++) {
                    if (strcmp(kv.key().c_str(), schema->settings[s].key) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    _sendJsonError(request, 400, "Unknown setting key", "UNKNOWN_SETTING");
                    return;
                }
            }
            // Pre-validate values against schema min/max BEFORE writing any setting
            // to NVS — prevents partial apply where early keys succeed but a later
            // key fails, leaving NVS in an inconsistent state (no partial apply rule).
            // Also reject non-numeric values: as<int32_t>() silently returns 0 for strings,
            // which could pass range checks (e.g. format=0 is valid). is<int32_t>() guards this.
            for (JsonPair kv : settingsObj) {
                if (!kv.value().is<int32_t>()) {
                    _sendJsonError(request, 400, "Setting value must be numeric", "INVALID_SETTING_TYPE");
                    return;
                }
                int32_t val = kv.value().as<int32_t>();
                for (uint8_t s = 0; s < schema->settingCount; s++) {
                    if (strcmp(kv.key().c_str(), schema->settings[s].key) == 0) {
                        if (val < schema->settings[s].minValue || val > schema->settings[s].maxValue) {
                            _sendJsonError(request, 400, "Setting value out of range", "INVALID_SETTING_VALUE");
                            return;
                        }
                        break;
                    }
                }
            }
            // Apply all settings (all values pre-validated — no partial apply risk)
            for (JsonPair kv : settingsObj) {
                int32_t val = kv.value().as<int32_t>();
                if (!ConfigManager::setModeSetting(schema->modeAbbrev, kv.key().c_str(), val)) {
                    _sendJsonError(request, 400, "Setting validation failed", "INVALID_SETTING_VALUE");
                    return;
                }
            }
            if (settingsObj.size() > 0) settingsApplied = true;
        }
    }

    // AC #6 / Rule 24: always call onManualSwitch so the orchestrator transitions to
    // MANUAL state regardless of whether the requested mode is already active.
    ModeOrchestrator::onManualSwitch(modeId, matchedEntry->displayName);

    // Build truthful response (AC #7, #9)
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject respData = root["data"].to<JsonObject>();
    respData["switching_to"] = modeId;
    respData["active"] = ModeRegistry::getActiveModeId()
                           ? ModeRegistry::getActiveModeId() : "classic_card";
    respData["settings_applied"] = settingsApplied;
    respData["switch_state"] = switchStateToString(ModeRegistry::getSwitchState());
    respData["orchestrator_state"] = ModeOrchestrator::getStateString();
    respData["state_reason"] = ModeOrchestrator::getStateReason();

    // Registry error if present (AC #9 — HEAP_INSUFFICIENT etc.)
    // Use copyLastError() — thread-safe snapshot for Core 1 (synthesis ds-3.2 Pass 3).
    char regErrBuf2[64];
    ModeRegistry::copyLastError(regErrBuf2, sizeof(regErrBuf2));
    if (regErrBuf2[0] != '\0') {
        respData["registry_error"] = regErrBuf2;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLayout(AsyncWebServerRequest* request) {
    HardwareConfig hw = ConfigManager::getHardware();
    LayoutResult layout = LayoutEngine::compute(hw);

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    data["mode"] = layout.mode;

    JsonObject matrix = data["matrix"].to<JsonObject>();
    matrix["width"] = layout.matrixWidth;
    matrix["height"] = layout.matrixHeight;

    JsonObject logo = data["logo_zone"].to<JsonObject>();
    logo["x"] = layout.logoZone.x;
    logo["y"] = layout.logoZone.y;
    logo["w"] = layout.logoZone.w;
    logo["h"] = layout.logoZone.h;

    JsonObject flight = data["flight_zone"].to<JsonObject>();
    flight["x"] = layout.flightZone.x;
    flight["y"] = layout.flightZone.y;
    flight["w"] = layout.flightZone.w;
    flight["h"] = layout.flightZone.h;

    JsonObject telemetry = data["telemetry_zone"].to<JsonObject>();
    telemetry["x"] = layout.telemetryZone.x;
    telemetry["y"] = layout.telemetryZone.y;
    telemetry["w"] = layout.telemetryZone.w;
    telemetry["h"] = layout.telemetryZone.h;

    JsonObject hardware = data["hardware"].to<JsonObject>();
    // Post hw-1.3: canvas is fixed by the HW-1 master/composite build.
    // tile_pixels is the nominal 64 so the LE-1 editor's snap-grid still works;
    // tiles_x/tiles_y are derived from the true canvas dims.
    hardware["tiles_x"] = layout.matrixWidth / HardwareConfiguration::NOMINAL_TILE_PIXELS;
    hardware["tiles_y"] = (layout.matrixHeight + HardwareConfiguration::NOMINAL_TILE_PIXELS - 1)
                          / HardwareConfiguration::NOMINAL_TILE_PIXELS;
    hardware["tile_pixels"] = HardwareConfiguration::NOMINAL_TILE_PIXELS;
    hardware["zone_layout"] = hw.zone_layout;
    hardware["zone_pad_x"] = hw.zone_pad_x;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_serveGzAsset(AsyncWebServerRequest* request, const char* path, const char* contentType) {
    if (!LittleFS.exists(path)) {
        request->send(404, "text/plain", "Not found");
        return;
    }
    AsyncWebServerResponse* response = request->beginResponse(LittleFS, path, contentType);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void WebPortal::_handlePostCalibrationStart(AsyncWebServerRequest* request) {
    if (_calibrationCallback) {
        _calibrationCallback(true);
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Calibration mode started";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostCalibrationStop(AsyncWebServerRequest* request) {
    if (_calibrationCallback) {
        _calibrationCallback(false);
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Calibration mode stopped";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLogos(AsyncWebServerRequest* request) {
    std::vector<LogoEntry> logos;
    if (!LogoManager::listLogos(logos)) {
        _sendJsonError(request, 500, "Logo storage unavailable. Reboot the device and try again.", "STORAGE_UNAVAILABLE");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonArray data = root["data"].to<JsonArray>();

    for (const auto& logo : logos) {
        JsonObject entry = data.add<JsonObject>();
        entry["name"] = logo.name;
        entry["size"] = logo.size;
    }

    // Storage usage metadata
    size_t usedBytes = 0, totalBytes = 0;
    LogoManager::getLittleFSUsage(usedBytes, totalBytes);
    JsonObject storage = root["storage"].to<JsonObject>();
    storage["used"] = usedBytes;
    storage["total"] = totalBytes;
    storage["logo_count"] = LogoManager::getLogoCount();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleDeleteLogo(AsyncWebServerRequest* request) {
    // Extract filename from URL: /api/logos/FILENAME
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0 || lastSlash == (int)(url.length() - 1)) {
        _sendJsonError(request, 400, "Missing logo filename", "MISSING_FILENAME");
        return;
    }
    String filename = url.substring(lastSlash + 1);
    int queryStart = filename.indexOf('?');
    if (queryStart >= 0) {
        filename = filename.substring(0, queryStart);
    }

    if (!LogoManager::isSafeLogoFilename(filename)) {
        _sendJsonError(request, 400, "Invalid logo filename", "INVALID_NAME");
        return;
    }

    if (!LogoManager::hasLogo(filename)) {
        _sendJsonError(request, 404, "Logo not found", "NOT_FOUND");
        return;
    }

    if (!LogoManager::deleteLogo(filename)) {
        _sendJsonError(request, 500, "Could not delete logo. Check storage health and try again.", "FS_DELETE_ERROR");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Logo deleted";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLogoFile(AsyncWebServerRequest* request) {
    // Extract filename from URL: /logos/FILENAME
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0 || lastSlash == (int)(url.length() - 1)) {
        request->send(404, "text/plain", "Not found");
        return;
    }
    String filename = url.substring(lastSlash + 1);
    int queryStart = filename.indexOf('?');
    if (queryStart >= 0) {
        filename = filename.substring(0, queryStart);
    }

    if (!LogoManager::isSafeLogoFilename(filename)) {
        request->send(404, "text/plain", "Not found");
        return;
    }

    String path = String("/logos/") + filename;
    if (!LittleFS.exists(path)) {
        request->send(404, "text/plain", "Not found");
        return;
    }

    request->send(LittleFS, path, "application/octet-stream");
}

void WebPortal::_handleGetOtaCheck(AsyncWebServerRequest* request) {
    // AC #2: if WiFi not connected, return error rather than hitting GitHub
    if (WiFi.status() != WL_CONNECTED) {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        JsonObject data = root["data"].to<JsonObject>();
        data["available"] = false;
        data["version"] = (const char*)nullptr;
        data["current_version"] = FW_VERSION;
        data["release_notes"] = (const char*)nullptr;
        data["error"] = "WiFi not connected";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    // Perform the check — this makes an HTTPS request to GitHub (blocks ~1-10s)
    bool updateAvailable = OTAUpdater::checkForUpdate();

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    if (OTAUpdater::getState() == OTAState::AVAILABLE) {
        data["available"] = true;
        data["version"] = OTAUpdater::getRemoteVersion();
        data["current_version"] = FW_VERSION;
        data["release_notes"] = OTAUpdater::getReleaseNotes();
    } else if (OTAUpdater::getState() == OTAState::ERROR) {
        // Rate limit / network / parse failure — include error message
        data["available"] = false;
        data["version"] = (const char*)nullptr;
        data["current_version"] = FW_VERSION;
        data["release_notes"] = (const char*)nullptr;
        data["error"] = OTAUpdater::getLastError();
    } else {
        // Up to date (IDLE state after successful check)
        data["available"] = false;
        data["version"] = (const char*)nullptr;
        data["current_version"] = FW_VERSION;
        data["release_notes"] = (const char*)nullptr;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostOtaPull(AsyncWebServerRequest* request) {
    OTAState state = OTAUpdater::getState();

    // Guard: already downloading/verifying (AC #2)
    if (state == OTAState::DOWNLOADING || state == OTAState::VERIFYING) {
        _sendJsonError(request, 409, "Download already in progress", "OTA_BUSY");
        return;
    }

    // Guard: no update available (AC #2)
    if (state != OTAState::AVAILABLE) {
        _sendJsonError(request, 400, "No update available \u2014 check for updates first", "NO_UPDATE");
        return;
    }

    // Start the download (AC #2)
    bool started = OTAUpdater::startDownload();

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    if (started) {
        root["ok"] = true;
        JsonObject data = root["data"].to<JsonObject>();
        data["started"] = true;
    } else {
        root["ok"] = false;
        root["error"] = OTAUpdater::getLastError();
        root["code"] = "OTA_START_FAILED";
    }

    String output;
    serializeJson(doc, output);
    request->send(started ? 200 : 500, "application/json", output);
}

void WebPortal::_handleGetOtaStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // AC #3: stable contract — lowercase state string
    data["state"] = OTAUpdater::getStateString();

    // progress: required for downloading, null otherwise
    OTAState state = OTAUpdater::getState();
    if (state == OTAState::DOWNLOADING) {
        data["progress"] = OTAUpdater::getProgress();
    } else if (state == OTAState::VERIFYING) {
        data["progress"] = (const char*)nullptr;  // indeterminate
    } else {
        data["progress"] = (const char*)nullptr;
    }

    // error: non-null only when state == error
    if (state == OTAState::ERROR) {
        data["error"] = OTAUpdater::getLastError();
    } else {
        data["error"] = (const char*)nullptr;
    }

    // Phase from dl-7.2 structured failure
    data["failure_phase"] = OTAUpdater::getFailurePhaseString();
    data["retriable"] = OTAUpdater::isRetriable();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetSchedule(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // Build rules array from ConfigManager (AC #1)
    ModeScheduleConfig sched = ConfigManager::getModeSchedule();
    JsonArray rules = data["rules"].to<JsonArray>();
    for (uint8_t i = 0; i < sched.ruleCount; i++) {
        const ScheduleRule& r = sched.rules[i];
        JsonObject ruleObj = rules.add<JsonObject>();
        ruleObj["index"] = i;
        ruleObj["start_min"] = r.startMin;
        ruleObj["end_min"] = r.endMin;
        ruleObj["mode_id"] = r.modeId;
        ruleObj["enabled"] = r.enabled;
    }

    // Orchestrator state and active rule index (AC #1)
    data["orchestrator_state"] = ModeOrchestrator::getStateString();
    data["active_rule_index"] = ModeOrchestrator::getActiveScheduleRuleIndex();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostSchedule(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument reqDoc;
    DeserializationError err = deserializeJson(reqDoc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "INVALID_JSON");
        return;
    }

    if (!reqDoc["rules"].is<JsonArray>()) {
        _sendJsonError(request, 400, "Missing or invalid 'rules' array", "INVALID_SCHEDULE");
        return;
    }

    JsonArray rulesArr = reqDoc["rules"].as<JsonArray>();
    if (rulesArr.size() > MAX_SCHEDULE_RULES) {
        _sendJsonError(request, 400, "Too many rules (max 8)", "INVALID_SCHEDULE");
        return;
    }

    // Build ModeScheduleConfig from JSON (AC #2–#5)
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = rulesArr.size();

    // Build mode ID lookup table for validation
    const ModeEntry* modeTable = ModeRegistry::getModeTable();
    const uint8_t modeCount = ModeRegistry::getModeCount();

    for (size_t i = 0; i < rulesArr.size(); i++) {
        JsonObject ruleObj = rulesArr[i];

        // Validate required fields
        if (!ruleObj["start_min"].is<int>() || !ruleObj["end_min"].is<int>() ||
            !ruleObj["mode_id"].is<const char*>() || !ruleObj["enabled"].is<int>()) {
            _sendJsonError(request, 400, "Rule missing required fields", "INVALID_SCHEDULE");
            return;
        }

        int startMin = ruleObj["start_min"].as<int>();
        int endMin = ruleObj["end_min"].as<int>();
        const char* modeId = ruleObj["mode_id"].as<const char*>();
        int enabled = ruleObj["enabled"].as<int>();

        // Range validation (AC #3)
        if (startMin < 0 || startMin > 1439 || endMin < 0 || endMin > 1439) {
            _sendJsonError(request, 400, "start_min/end_min out of range (0-1439)", "INVALID_SCHEDULE");
            return;
        }

        if (enabled < 0 || enabled > 1) {
            _sendJsonError(request, 400, "enabled must be 0 or 1", "INVALID_SCHEDULE");
            return;
        }

        if (!modeId || strlen(modeId) == 0 || strlen(modeId) >= MODE_ID_BUF_LEN) {
            _sendJsonError(request, 400, "Invalid mode_id", "INVALID_SCHEDULE");
            return;
        }

        // Validate mode_id exists in ModeRegistry (AC #3)
        bool modeFound = false;
        for (uint8_t m = 0; m < modeCount; m++) {
            if (strcmp(modeTable[m].id, modeId) == 0) {
                modeFound = true;
                break;
            }
        }
        if (!modeFound) {
            _sendJsonError(request, 400, "Unknown mode_id", "UNKNOWN_MODE");
            return;
        }

        cfg.rules[i].startMin = (uint16_t)startMin;
        cfg.rules[i].endMin = (uint16_t)endMin;
        strncpy(cfg.rules[i].modeId, modeId, MODE_ID_BUF_LEN - 1);
        cfg.rules[i].modeId[MODE_ID_BUF_LEN - 1] = '\0';
        cfg.rules[i].enabled = (uint8_t)enabled;
    }

    // Persist via ConfigManager (AC #2) — orchestrator tick picks up on next cycle (Rule 24)
    if (!ConfigManager::setModeSchedule(cfg)) {
        _sendJsonError(request, 500, "Failed to save schedule", "NVS_ERROR");
        return;
    }

    JsonDocument respDoc;
    JsonObject resp = respDoc.to<JsonObject>();
    resp["ok"] = true;
    JsonObject respData = resp["data"].to<JsonObject>();
    respData["applied"] = true;

    String output;
    serializeJson(respDoc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_sendJsonError(AsyncWebServerRequest* request, int httpCode, const char* error, const char* code) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = false;
    root["error"] = error;
    root["code"] = code;
    String output;
    serializeJson(doc, output);
    request->send(httpCode, "application/json", output);
}

// ──────────────────────────────────────────────────────────────────────────
// Layout CRUD + activation (Story LE-1.4)
// ──────────────────────────────────────────────────────────────────────────

namespace {

// Shared helpers for layout handlers — keep path parsing consistent.

// Extract a layout id segment from a URL of the form /api/layouts/<id>
// (or /api/layouts/<id>/activate when activateSegment = true).
// Strips any query string. Returns empty String on failure.
String extractLayoutIdFromUrl(const String& url, bool activateSegment) {
    if (activateSegment) {
        const char* prefix = "/api/layouts/";
        const char* suffix = "/activate";
        size_t prefixLen = strlen(prefix);
        size_t suffixLen = strlen(suffix);
        if (!url.startsWith(prefix) || !url.endsWith(suffix)) {
            return String();
        }
        if (url.length() < prefixLen + suffixLen + 1) return String();
        return url.substring(prefixLen, url.length() - suffixLen);
    }
    // Simple "/api/layouts/<id>" path: take everything after the final '/'.
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0 || lastSlash == (int)(url.length() - 1)) return String();
    String id = url.substring(lastSlash + 1);
    int queryStart = id.indexOf('?');
    if (queryStart >= 0) id = id.substring(0, queryStart);
    return id;
}

} // namespace

void WebPortal::_handleGetLayouts(AsyncWebServerRequest* request) {
    std::vector<LayoutEntry> entries;
    if (!LayoutStore::list(entries)) {
        _sendJsonError(request, 500, "Layout storage unavailable", "STORAGE_UNAVAILABLE");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    data["active_id"] = LayoutStore::getActiveId();
    JsonArray arr = data["layouts"].to<JsonArray>();
    for (const auto& e : entries) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = e.id;
        obj["name"] = e.name;
        obj["bytes"] = e.sizeBytes;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLayoutById(AsyncWebServerRequest* request) {
    String id = extractLayoutIdFromUrl(request->url(), false);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Missing layout id", "MISSING_ID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    String body;
    if (!LayoutStore::load(id.c_str(), body)) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }
    // Parse body so it's embedded as a JSON object (not a double-encoded
    // string) under data.
    JsonDocument doc;
    JsonDocument layoutDoc;
    DeserializationError err = deserializeJson(layoutDoc, body);
    if (err) {
        _sendJsonError(request, 500, "Stored layout is corrupt", "LAYOUT_CORRUPT");
        return;
    }
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["data"] = layoutDoc.as<JsonVariant>();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

// LE-1.10 — reject widget payloads whose `content` field id is not in the
// owning widget's catalog. Single helper shared by POST and PUT. Returns true
// when the layout is ok, false after sending the 400 response itself.
//
// Strictness: we reject only when a widget of type `flight_field` or `metric`
// carries a `content` string that is non-empty AND not in the widget's
// catalog. Missing/empty `content` is allowed (renderer falls back to
// kCatalog[0]) — rejecting it here would break the "drop a fresh widget"
// flow where the editor POSTs before the user picks a field.
static bool validateLayoutFieldIds(JsonDocument& doc,
                                   AsyncWebServerRequest* request) {
    JsonVariantConst widgets = doc["widgets"];
    if (!widgets.is<JsonArrayConst>()) return true;  // schema-check handles shape
    for (JsonVariantConst wv : widgets.as<JsonArrayConst>()) {
        const char* type = wv["type"] | (const char*)nullptr;
        if (type == nullptr) continue;
        const char* content = wv["content"] | (const char*)nullptr;
        if (content == nullptr || content[0] == '\0') continue;

        bool isFlightField = (strcmp(type, "flight_field") == 0);
        bool isMetric      = (strcmp(type, "metric")       == 0);
        if (!isFlightField && !isMetric) continue;

        bool ok = isFlightField
            ? FlightFieldWidgetCatalog::isKnownFieldId(content)
            : MetricWidgetCatalog::isKnownFieldId(content);
        if (ok) continue;

        JsonDocument errDoc;
        JsonObject root = errDoc.to<JsonObject>();
        root["ok"]    = false;
        root["error"] = "Unknown field_id";
        root["code"]  = "UNKNOWN_FIELD_ID";
        String output;
        serializeJson(errDoc, output);
        request->send(400, "application/json", output);
        return false;
    }
    return true;
}

void WebPortal::_handlePostLayout(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "LAYOUT_INVALID");
        return;
    }
    if (!doc.is<JsonObject>()) {
        _sendJsonError(request, 400, "Expected JSON object", "LAYOUT_INVALID");
        return;
    }
    const char* id = doc["id"] | (const char*)nullptr;
    if (id == nullptr || *id == '\0') {
        _sendJsonError(request, 400, "Missing id in layout body", "LAYOUT_INVALID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id)) {
        _sendJsonError(request, 400, "Invalid layout id", "LAYOUT_INVALID");
        return;
    }
    if (!validateLayoutFieldIds(doc, request)) {
        return;  // helper already sent the 400 response.
    }
    String jsonStr;
    serializeJson(doc, jsonStr);

    LayoutStoreError result = LayoutStore::save(id, jsonStr.c_str());
    switch (result) {
        case LayoutStoreError::OK: {
            JsonDocument respDoc;
            JsonObject root = respDoc.to<JsonObject>();
            root["ok"] = true;
            JsonObject respData = root["data"].to<JsonObject>();
            respData["id"] = id;
            respData["bytes"] = jsonStr.length();
            String output;
            serializeJson(respDoc, output);
            request->send(200, "application/json", output);
            return;
        }
        case LayoutStoreError::TOO_LARGE:
            _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
            return;
        case LayoutStoreError::INVALID_SCHEMA:
            _sendJsonError(request, 400, "Layout schema invalid", "LAYOUT_INVALID");
            return;
        case LayoutStoreError::FS_FULL:
            _sendJsonError(request, 507, "Filesystem full", "FS_FULL");
            return;
        case LayoutStoreError::IO_ERROR:
        default:
            _sendJsonError(request, 500, "Layout IO error", "IO_ERROR");
            return;
    }
}

void WebPortal::_handlePutLayout(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    String id = extractLayoutIdFromUrl(request->url(), false);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Missing layout id", "MISSING_ID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    // Must exist already — PUT is overwrite-only. Caller should use POST to
    // create. Check via load() (fills a default on miss; use return value).
    String existing;
    if (!LayoutStore::load(id.c_str(), existing)) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "LAYOUT_INVALID");
        return;
    }
    if (!doc.is<JsonObject>()) {
        _sendJsonError(request, 400, "Expected JSON object", "LAYOUT_INVALID");
        return;
    }
    const char* docId = doc["id"] | (const char*)nullptr;
    if (docId == nullptr) {
        _sendJsonError(request, 400, "Missing id field in layout body", "LAYOUT_INVALID");
        return;
    }
    if (strcmp(docId, id.c_str()) != 0) {
        _sendJsonError(request, 400, "Body id does not match URL id", "LAYOUT_INVALID");
        return;
    }
    if (!validateLayoutFieldIds(doc, request)) {
        return;  // helper already sent the 400 response.
    }
    String jsonStr;
    serializeJson(doc, jsonStr);

    LayoutStoreError result = LayoutStore::save(id.c_str(), jsonStr.c_str());
    switch (result) {
        case LayoutStoreError::OK: {
            JsonDocument respDoc;
            JsonObject root = respDoc.to<JsonObject>();
            root["ok"] = true;
            JsonObject respData = root["data"].to<JsonObject>();
            respData["id"] = id;
            respData["bytes"] = jsonStr.length();
            String output;
            serializeJson(respDoc, output);
            request->send(200, "application/json", output);
            return;
        }
        case LayoutStoreError::TOO_LARGE:
            _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
            return;
        case LayoutStoreError::INVALID_SCHEMA:
            _sendJsonError(request, 400, "Layout schema invalid", "LAYOUT_INVALID");
            return;
        case LayoutStoreError::FS_FULL:
            _sendJsonError(request, 507, "Filesystem full", "FS_FULL");
            return;
        case LayoutStoreError::IO_ERROR:
        default:
            _sendJsonError(request, 500, "Layout IO error", "IO_ERROR");
            return;
    }
}

void WebPortal::_handleDeleteLayout(AsyncWebServerRequest* request) {
    String id = extractLayoutIdFromUrl(request->url(), false);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Missing layout id", "MISSING_ID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    String activeId = LayoutStore::getActiveId();
    if (activeId == id) {
        _sendJsonError(request, 409, "Cannot delete active layout", "LAYOUT_ACTIVE");
        return;
    }
    if (!LayoutStore::remove(id.c_str())) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostLayoutActivate(AsyncWebServerRequest* request) {
    String id = extractLayoutIdFromUrl(request->url(), true);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Invalid activate path", "INVALID_PATH");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    // Verify the layout actually exists — load() returns false on miss.
    String tmp;
    if (!LayoutStore::load(id.c_str(), tmp)) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }
    if (!LayoutStore::setActiveId(id.c_str())) {
        _sendJsonError(request, 500, "Failed to persist active layout id", "NVS_ERROR");
        return;
    }
    // Rule #24: route mode switch through orchestrator. Never call
    // ModeRegistry::requestSwitch directly from WebPortal.
    ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout");

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    data["active_id"] = id;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetWidgetTypes(AsyncWebServerRequest* request) {
    // ─────────────────────────────────────────────────────────────────────
    // SYNC-RISK NOTE (LE-1.4 review follow-up):
    // The widget type strings below ("text", "clock", "logo", "flight_field",
    // "metric") and their field schemas are hard-coded here. The same type
    // strings also live in:
    //   • core/LayoutStore.cpp  — kAllowedWidgetTypes[] (save() validation)
    //   • core/WidgetRegistry.cpp — WidgetRegistry::fromString() (render dispatch)
    // All three lists MUST stay in lock-step. If a new widget type is added
    // without updating this handler, the editor UI (LE-1.7) will have no
    // way to surface it even though LayoutStore accepts it and the
    // renderer draws it. Consolidate in LE-1.7 when the widget system
    // stabilises (e.g. promote to a single WidgetType descriptor table
    // consumed by all three call sites).
    // ─────────────────────────────────────────────────────────────────────
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonArray data = root["data"].to<JsonArray>();

    // Helper lambdas to tighten the repetitive build-up. JsonDocument is
    // scoped to this function — no static allocations (AR rule).
    auto addField = [](JsonArray& fields, const char* key, const char* kind,
                       const char* defaultStr, int defaultInt, bool useInt,
                       const char* const* options, size_t optCount) {
        JsonObject f = fields.add<JsonObject>();
        f["key"] = key;
        f["kind"] = kind;
        if (useInt) f["default"] = defaultInt;
        else        f["default"] = defaultStr;
        if (options != nullptr && optCount > 0) {
            JsonArray arr = f["options"].to<JsonArray>();
            for (size_t i = 0; i < optCount; ++i) arr.add(options[i]);
        }
    };

    // Shared option tables + sibling-label emitter for the font + size
    // dropdowns across all text-rendering widgets. Keeping this in one place
    // means adding support to a new widget only needs one call to
    // `emitFontLabels(obj)` plus the matching `addField` pair.
    static const char* const fontOpts[] = { "default", "tomthumb", "picopixel" };
    static const char* const sizeOpts[] = { "1", "2", "3" };
    // text_transform is offered on text/flight_field/metric (clock digits
    // wouldn't benefit, so its dropdown is intentionally suppressed).
    static const char* const transformOpts[] = { "none", "upper", "lower" };
    auto emitTransformLabels = [](JsonObject& obj) {
        JsonArray arr = obj["transform_options"].to<JsonArray>();
        {
            JsonObject o = arr.add<JsonObject>();
            o["id"] = "none"; o["label"] = "None";
        }
        {
            JsonObject o = arr.add<JsonObject>();
            o["id"] = "upper"; o["label"] = "UPPER";
        }
        {
            JsonObject o = arr.add<JsonObject>();
            o["id"] = "lower"; o["label"] = "lower";
        }
    };
    auto emitFontLabels = [](JsonObject& obj) {
        JsonArray fontFieldOptions = obj["font_options"].to<JsonArray>();
        {
            JsonObject o = fontFieldOptions.add<JsonObject>();
            o["id"] = "default"; o["label"] = "Default (6\u00d78)";
        }
        {
            JsonObject o = fontFieldOptions.add<JsonObject>();
            o["id"] = "tomthumb"; o["label"] = "TomThumb (3\u00d75)";
        }
        {
            JsonObject o = fontFieldOptions.add<JsonObject>();
            o["id"] = "picopixel"; o["label"] = "Picopixel (4\u00d75)";
        }
        JsonArray sizeFieldOptions = obj["size_options"].to<JsonArray>();
        {
            JsonObject o = sizeFieldOptions.add<JsonObject>();
            o["id"] = "1"; o["label"] = "1\u00d7";
        }
        {
            JsonObject o = sizeFieldOptions.add<JsonObject>();
            o["id"] = "2"; o["label"] = "2\u00d7";
        }
        {
            JsonObject o = sizeFieldOptions.add<JsonObject>();
            o["id"] = "3"; o["label"] = "3\u00d7";
        }
    };

    // ── text ─────────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "text";
        obj["label"] = "Text";
        JsonArray fields = obj["fields"].to<JsonArray>();
        addField(fields, "content", "string", "", 0, false, nullptr, 0);
        static const char* const alignOptions[] = { "left", "center", "right" };
        addField(fields, "align", "select", "left", 0, false, alignOptions, 3);
        addField(fields, "font", "select", "default", 0, false, fontOpts, 2);
        addField(fields, "text_size", "select", "1", 0, false, sizeOpts, 3);
        addField(fields, "text_transform", "select", "none", 0, false, transformOpts, 3);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 32, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
        emitFontLabels(obj);
        emitTransformLabels(obj);
    }
    // ── clock ────────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "clock";
        obj["label"] = "Clock";
        JsonArray fields = obj["fields"].to<JsonArray>();
        static const char* const clockFmtOptions[] = { "12h", "24h" };
        addField(fields, "content", "select", "24h", 0, false, clockFmtOptions, 2);
        addField(fields, "font", "select", "default", 0, false, fontOpts, 2);
        addField(fields, "text_size", "select", "1", 0, false, sizeOpts, 3);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
        emitFontLabels(obj);
    }
    // ── logo ─────────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "logo";
        obj["label"] = "Logo";
        // LE-1.5: real RGB565 bitmap pipeline — upload via POST /api/logos,
        // then set content to the ICAO/id (e.g. "UAL"). Falls back to
        // PROGMEM airplane sprite when logo_id is not found on LittleFS.
        JsonArray fields = obj["fields"].to<JsonArray>();
        addField(fields, "content", "string", "", 0, false, nullptr, 0);
        addField(fields, "color", "color", "#0000FF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 16, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 16, true, nullptr, 0);
    }
    // ── flight_field ─────────────────────────────────────────────────────
    // LE-1.8 + LE-1.10 — dropdown of field ids owned by FlightFieldWidget's
    // kCatalog (single source of truth). `field_options` carries {id, label}
    // pairs so the editor can render human-readable dropdown text; `options`
    // on the content `fields` entry continues to carry the id strings for
    // the pre-LE-1.10 select plumbing and to seed the content default.
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "flight_field";
        obj["label"] = "Flight Field";
        size_t ffCount = 0;
        const FieldDescriptor* ffCat = FlightFieldWidgetCatalog::catalog(ffCount);
        JsonArray fields = obj["fields"].to<JsonArray>();
        // Build the legacy string-array options from the catalog so the
        // existing select plumbing keeps working, then override the default
        // to kCatalog[0].id.
        std::vector<const char*> ffOptions;
        ffOptions.reserve(ffCount);
        for (size_t i = 0; i < ffCount; ++i) ffOptions.push_back(ffCat[i].id);
        addField(fields, "content", "select", ffCount > 0 ? ffCat[0].id : "", 0, false,
                 ffOptions.empty() ? nullptr : ffOptions.data(), ffOptions.size());
        addField(fields, "font", "select", "default", 0, false, fontOpts, 2);
        addField(fields, "text_size", "select", "1", 0, false, sizeOpts, 3);
        addField(fields, "text_transform", "select", "none", 0, false, transformOpts, 3);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
        // LE-1.10 — catalog with human labels, consumed by the editor's
        // property-panel field picker. A sibling key to avoid colliding with
        // the existing `fields[]` property schema.
        JsonArray fieldOptions = obj["field_options"].to<JsonArray>();
        for (size_t i = 0; i < ffCount; ++i) {
            JsonObject fo = fieldOptions.add<JsonObject>();
            fo["id"]    = ffCat[i].id;
            fo["label"] = ffCat[i].label;
        }
        emitFontLabels(obj);
        emitTransformLabels(obj);
    }
    // ── metric ───────────────────────────────────────────────────────────
    // LE-1.8 + LE-1.10 — dropdown of telemetry ids owned by MetricWidget's
    // kCatalog. Same pattern as flight_field above; catalog entries include
    // a `unit` string for the editor dropdown.
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "metric";
        obj["label"] = "Metric";
        size_t mCount = 0;
        const FieldDescriptor* mCat = MetricWidgetCatalog::catalog(mCount);
        JsonArray fields = obj["fields"].to<JsonArray>();
        std::vector<const char*> mOptions;
        mOptions.reserve(mCount);
        for (size_t i = 0; i < mCount; ++i) mOptions.push_back(mCat[i].id);
        addField(fields, "content", "select", mCount > 0 ? mCat[0].id : "", 0, false,
                 mOptions.empty() ? nullptr : mOptions.data(), mOptions.size());
        addField(fields, "font", "select", "default", 0, false, fontOpts, 2);
        addField(fields, "text_size", "select", "1", 0, false, sizeOpts, 3);
        addField(fields, "text_transform", "select", "none", 0, false, transformOpts, 3);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
        JsonArray fieldOptions = obj["field_options"].to<JsonArray>();
        for (size_t i = 0; i < mCount; ++i) {
            JsonObject fo = fieldOptions.add<JsonObject>();
            fo["id"]    = mCat[i].id;
            fo["label"] = mCat[i].label;
            if (mCat[i].unit != nullptr) fo["unit"] = mCat[i].unit;
        }
        emitFontLabels(obj);
        emitTransformLabels(obj);
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}
