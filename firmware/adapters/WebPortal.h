#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <functional>
#include "adapters/WiFiManager.h"

class WebPortal {
public:
    using RebootCallback = std::function<void()>;

    void init(AsyncWebServer& server, WiFiManager& wifiMgr);
    void begin();
    void onReboot(RebootCallback callback);

private:
    AsyncWebServer* _server = nullptr;
    WiFiManager* _wifiMgr = nullptr;
    RebootCallback _rebootCallback = nullptr;

    void _registerRoutes();
    void _handleRoot(AsyncWebServerRequest* request);
    void _handleGetSettings(AsyncWebServerRequest* request);
    void _handlePostSettings(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void _handleGetStatus(AsyncWebServerRequest* request);
    void _handlePostReboot(AsyncWebServerRequest* request);
    void _handlePostReset(AsyncWebServerRequest* request);
    void _handleGetWifiScan(AsyncWebServerRequest* request);
    void _handleGetLayout(AsyncWebServerRequest* request);

    static void _serveGzAsset(AsyncWebServerRequest* request, const char* path, const char* contentType);
    void _sendJsonError(AsyncWebServerRequest* request, int httpCode, const char* error, const char* code);
};
