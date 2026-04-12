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
#include <vector>
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"
#include "core/LogoManager.h"
#include "utils/Log.h"

// Defined in main.cpp — provides thread-safe flight stats for the health page
extern FlightStatsSnapshot getFlightStatsSnapshot();

#include "core/LayoutEngine.h"

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
