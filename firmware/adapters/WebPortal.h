#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
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
    RebootCallback _rebootCallback = nullptr;
    CalibrationCallback _calibrationCallback = nullptr;
    PositioningCallback _positioningCallback = nullptr;

    void _registerRoutes();
    void _handleRoot(AsyncWebServerRequest* request);
    void _handleGetSettings(AsyncWebServerRequest* request);
    void _handlePostSettings(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void _handleGetStatus(AsyncWebServerRequest* request);
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

    static void _serveGzAsset(AsyncWebServerRequest* request, const char* path, const char* contentType);
    void _sendJsonError(AsyncWebServerRequest* request, int httpCode, const char* error, const char* code);
};
