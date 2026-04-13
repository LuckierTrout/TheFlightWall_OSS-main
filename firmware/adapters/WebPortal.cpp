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
  Network:   wifi_ssid, wifi_password, os_client_id, os_client_sec, aeroapi_key

GET /api/status extended JSON (Story 2.4):
  data.subsystems   — existing six subsystem objects (wifi, opensky, aeroapi, cdn, nvs, littlefs)
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
#include "core/SystemStatus.h"
#include "core/LogoManager.h"
#include "utils/Log.h"

// Defined in main.cpp — provides thread-safe flight stats for the health page
extern FlightStatsSnapshot getFlightStatsSnapshot();

// Defined in main.cpp — NTP sync status accessor (Story fn-2.1)
extern bool isNtpSynced();

#include "core/LayoutEngine.h"
#include "core/ModeOrchestrator.h"

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
                PendingRequestBody pending{request, String()};
                pending.body.reserve(total);
                g_pendingBodies.push_back(pending);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }

            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }

            for (size_t i = 0; i < len; ++i) {
                pending->body += static_cast<char>(data[i]);
            }

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

    // POST /api/display/mode (Story dl-1.5) — manual mode switch
    _server->on("/api/display/mode", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (total == 0 || data == nullptr) {
                _sendJsonError(request, 400, "Empty request body", "EMPTY_PAYLOAD");
                return;
            }
            if (index + len == total) {
                // Parse the mode_id from request body (use `len` not `total`: data points to the
                // current chunk only; passing `total` would read past the buffer on multi-chunk bodies)
                JsonDocument reqDoc;
                DeserializationError err = deserializeJson(reqDoc, data, len);
                if (err) {
                    _sendJsonError(request, 400, "Invalid JSON", "INVALID_JSON");
                    return;
                }
                const char* modeId = reqDoc["mode_id"] | (const char*)nullptr;
                if (!modeId) {
                    _sendJsonError(request, 400, "Missing mode_id field", "MISSING_FIELD");
                    return;
                }

                // Resolve mode name from ID (simple lookup)
                const char* modeName = nullptr;
                if (strcmp(modeId, "classic_card") == 0) modeName = "Classic Card";
                else if (strcmp(modeId, "live_flight") == 0) modeName = "Live Flight Card";
                else if (strcmp(modeId, "clock") == 0) modeName = "Clock";
                else {
                    _sendJsonError(request, 400, "Unknown mode_id", "UNKNOWN_MODE");
                    return;
                }

                // Switch via orchestrator (Rule 24: always go through orchestrator)
                ModeOrchestrator::onManualSwitch(modeId, modeName);

                JsonDocument doc;
                JsonObject root = doc.to<JsonObject>();
                root["ok"] = true;
                JsonObject respData = root["data"].to<JsonObject>();
                respData["switching_to"] = modeId;
                respData["orchestrator_state"] = ModeOrchestrator::getStateString();
                respData["state_reason"] = ModeOrchestrator::getStateReason();
                String output;
                serializeJson(doc, output);
                request->send(200, "application/json", output);
            }
        }
    );

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

    // NTP and schedule status (Story fn-2.1)
    data["ntp_synced"] = isNtpSynced();
    data["schedule_active"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();

    // Rollback acknowledgment state (Story fn-1.6) — NVS-backed, not via ConfigManager
    Preferences prefs;
    prefs.begin("flightwall", true);  // read-only
    data["rollback_acknowledged"] = prefs.getUChar("ota_rb_ack", 0) == 1;
    prefs.end();

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
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // Mode list (static for now; will grow with ModeRegistry in future stories)
    JsonArray modes = data["modes"].to<JsonArray>();

    const char* activeId = ModeOrchestrator::getActiveModeId();

    // Classic Card mode
    JsonObject mClassic = modes.add<JsonObject>();
    mClassic["id"] = "classic_card";
    mClassic["name"] = "Classic Card";
    mClassic["active"] = (strcmp(activeId, "classic_card") == 0);

    // Live Flight Card mode
    JsonObject mLive = modes.add<JsonObject>();
    mLive["id"] = "live_flight";
    mLive["name"] = "Live Flight Card";
    mLive["active"] = (strcmp(activeId, "live_flight") == 0);

    // Clock mode
    JsonObject mClock = modes.add<JsonObject>();
    mClock["id"] = "clock";
    mClock["name"] = "Clock";
    mClock["active"] = (strcmp(activeId, "clock") == 0);

    data["active"] = activeId;
    data["switch_state"] = "idle";
    data["orchestrator_state"] = ModeOrchestrator::getStateString();
    data["state_reason"] = ModeOrchestrator::getStateReason();

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
    hardware["tiles_x"] = hw.tiles_x;
    hardware["tiles_y"] = hw.tiles_y;
    hardware["tile_pixels"] = hw.tile_pixels;

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
