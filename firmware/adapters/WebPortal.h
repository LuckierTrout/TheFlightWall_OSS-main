#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <atomic>
#include <functional>
#include "adapters/WiFiManager.h"

class WebPortal {
public:
    using RebootCallback = std::function<void()>;
    using CalibrationCallback = std::function<void(bool)>;
    using PositioningCallback = std::function<void(bool)>;

    void init(AsyncWebServer& server, WiFiManager& wifiMgr);
    void begin();
    void onReboot(RebootCallback callback);
    void onCalibration(CalibrationCallback callback);
    void onPositioning(PositioningCallback callback);

private:
    AsyncWebServer* _server = nullptr;
    WiFiManager* _wifiMgr = nullptr;
    // Set true when POST /api/settings writes a reboot-required key. Surfaced
    // via GET /api/status so the dashboard can hydrate the reboot-pending UI
    // state across browser refreshes without holding state in the client alone.
    // Reset naturally at boot (default-initialized false).
    std::atomic<bool> _rebootPending{false};
    RebootCallback _rebootCallback = nullptr;
    CalibrationCallback _calibrationCallback = nullptr;
    PositioningCallback _positioningCallback = nullptr;

    void _registerRoutes();
    void _handleRoot(AsyncWebServerRequest* request);
    void _handleGetSettings(AsyncWebServerRequest* request);
    void _handlePostSettings(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void _handleGetStatus(AsyncWebServerRequest* request);
    void _handleGetCurrentFlights(AsyncWebServerRequest* request);
    void _handlePostReboot(AsyncWebServerRequest* request);
    void _handlePostReset(AsyncWebServerRequest* request);
    void _handleGetWifiScan(AsyncWebServerRequest* request);
    void _handleGetLayout(AsyncWebServerRequest* request);
    void _handlePostCalibrationStart(AsyncWebServerRequest* request);
    void _handlePostCalibrationStop(AsyncWebServerRequest* request);
    void _handleGetLogos(AsyncWebServerRequest* request);
    void _handleDeleteLogo(AsyncWebServerRequest* request);
    void _handleGetLogoFile(AsyncWebServerRequest* request);

    void _handleGetDisplayModes(AsyncWebServerRequest* request);
    void _handlePostDisplayMode(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void _handleGetOtaCheck(AsyncWebServerRequest* request);
    void _handlePostOtaPull(AsyncWebServerRequest* request);
    void _handleGetOtaStatus(AsyncWebServerRequest* request);
    void _handleGetSchedule(AsyncWebServerRequest* request);
    void _handlePostSchedule(AsyncWebServerRequest* request, uint8_t* data, size_t len);

    // Layout CRUD + activation (LE-1.4)
    void _handleGetLayouts(AsyncWebServerRequest* request);
    void _handleGetLayoutById(AsyncWebServerRequest* request);
    void _handlePostLayout(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void _handlePutLayout(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void _handleDeleteLayout(AsyncWebServerRequest* request);
    void _handlePostLayoutActivate(AsyncWebServerRequest* request);
    void _handleGetWidgetTypes(AsyncWebServerRequest* request);

    static void _serveGzAsset(AsyncWebServerRequest* request, const char* path, const char* contentType);
    void _sendJsonError(AsyncWebServerRequest* request, int httpCode, const char* error, const char* code);
};
