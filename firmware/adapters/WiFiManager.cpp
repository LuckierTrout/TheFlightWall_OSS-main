#include "WiFiManager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "utils/Log.h"
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"

WiFiManager* WiFiManager::_instance = nullptr;

void WiFiManager::init(bool forceApSetup) {
    _instance = this;

    // Register event handler before any WiFi operations
    WiFi.onEvent(_onWiFiEvent);

    if (forceApSetup) {
        // GPIO boot-hold detected — force AP setup regardless of stored credentials
        _startAP("FlightWall-Setup");
        LOG_I("WiFi", "Forced AP setup via boot button");
        return;
    }

    NetworkConfig netCfg = ConfigManager::getNetwork();

    if (netCfg.wifi_ssid.length() == 0) {
        // No credentials — enter AP setup mode
        _startAP("FlightWall-Setup");
        LOG_I("WiFi", "No credentials, entering AP setup mode");
    } else {
        // Credentials exist — attempt STA connection
        _startSTA(netCfg.wifi_ssid, netCfg.wifi_password);
        LOG_I("WiFi", "Credentials found, connecting to STA");
    }
}

void WiFiManager::tick() {
    // Process GOT_IP flag from event handler
    if (_gotIP.exchange(false)) {
        _onConnected();
    }

    // Process DISCONNECTED flag from event handler
    if (_disconnected.exchange(false)) {
        _onDisconnected();
    }

    // Reconnection retry logic
    if (_state == WiFiState::STA_RECONNECTING) {
        if (millis() - _lastRetryMs >= RETRY_INTERVAL_MS) {
            if (_retryCount >= MAX_RETRIES) {
                _startAPFallback();
            } else {
                WiFi.reconnect();
                _retryCount++;
                _lastRetryMs = millis();
#if LOG_LEVEL >= 2
                Serial.println("[WiFi] Reconnect attempt " + String(_retryCount) + "/" + String(MAX_RETRIES));
#endif
            }
        }
    }
}

WiFiState WiFiManager::getState() const {
    return _state;
}

String WiFiManager::getLocalIP() const {
    return WiFi.localIP().toString();
}

String WiFiManager::getSSID() const {
    return WiFi.SSID();
}

void WiFiManager::onStateChange(StateCallback callback) {
    _callbacks.push_back(callback);
}

void WiFiManager::_setState(WiFiState newState) {
    WiFiState oldState = _state;
    if (_stateInitialized && oldState == newState) return;

    _state = newState;
    _stateInitialized = true;

#if LOG_LEVEL >= 2
    static const char* stateNames[] = {
        "AP_SETUP", "CONNECTING", "STA_CONNECTED", "STA_RECONNECTING", "AP_FALLBACK"
    };
    Serial.println("[WiFi] State: " + String(stateNames[(uint8_t)oldState]) + " -> " + String(stateNames[(uint8_t)newState]));
#endif

    _updateSystemStatus();

    for (auto& cb : _callbacks) {
        cb(oldState, newState);
    }
}

void WiFiManager::_startAP(const char* ssid) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    _setState(WiFiState::AP_SETUP);
}

void WiFiManager::_startSTA(const String& ssid, const String& password) {
    _setState(WiFiState::CONNECTING);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
}

void WiFiManager::_startAPFallback() {
    LOG_E("WiFi", "Reconnection timeout, entering AP fallback");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("FlightWall-Setup");
    _setState(WiFiState::AP_FALLBACK);
}

void WiFiManager::_onConnected() {
    _setState(WiFiState::STA_CONNECTED);
    _retryCount = 0;

    // mDNS registration
    if (MDNS.begin("flightwall")) {
        MDNS.addService("_http", "_tcp", 80);
        LOG_I("WiFi", "mDNS started: flightwall.local");
    } else {
        LOG_E("WiFi", "mDNS failed to start");
    }

    // NTP time sync (fire-and-forget, non-blocking)
    configTime(0, 0, "pool.ntp.org");
    LOG_I("WiFi", "NTP configured (pool.ntp.org, UTC)");

#if LOG_LEVEL >= 2
    Serial.println("[WiFi] Connected, IP: " + WiFi.localIP().toString());
#endif
}

void WiFiManager::_onDisconnected() {
    if (_state == WiFiState::STA_CONNECTED || _state == WiFiState::CONNECTING) {
        // Clean up mDNS
        if (_state == WiFiState::STA_CONNECTED) {
            MDNS.end();
            LOG_I("WiFi", "mDNS stopped");
        }

        // Enter reconnection mode
        _setState(WiFiState::STA_RECONNECTING);
        _retryCount = 0;
        _lastRetryMs = millis();
    }
    // Ignore disconnect events during CONNECTING or STA_RECONNECTING
    // (ESP32 fires spurious disconnect events during connection attempts)
}

void WiFiManager::_updateSystemStatus() {
    switch (_state) {
        case WiFiState::STA_CONNECTED: {
            int rssi = WiFi.RSSI();
            if (rssi < -75) {
                SystemStatus::set(Subsystem::WIFI, StatusLevel::WARNING,
                    "Weak signal, " + String(rssi) + " dBm");
            } else {
                SystemStatus::set(Subsystem::WIFI, StatusLevel::OK,
                    "Connected, " + String(rssi) + " dBm");
            }
            break;
        }
        case WiFiState::STA_RECONNECTING:
            SystemStatus::set(Subsystem::WIFI, StatusLevel::ERROR,
                "Disconnected, retrying...");
            break;
        case WiFiState::AP_SETUP:
            SystemStatus::set(Subsystem::WIFI, StatusLevel::OK,
                "AP mode: FlightWall-Setup");
            break;
        case WiFiState::AP_FALLBACK:
            SystemStatus::set(Subsystem::WIFI, StatusLevel::ERROR,
                "WiFi failed, AP fallback active");
            break;
        case WiFiState::CONNECTING:
            SystemStatus::set(Subsystem::WIFI, StatusLevel::WARNING,
                "Connecting...");
            break;
    }
}

void WiFiManager::_onWiFiEvent(WiFiEvent_t event) {
    if (!_instance) return;

    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            _instance->_gotIP.store(true);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            _instance->_disconnected.store(true);
            break;
        default:
            break;
    }
}
