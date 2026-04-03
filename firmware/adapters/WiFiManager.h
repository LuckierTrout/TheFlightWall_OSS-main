#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include <vector>
#include <functional>

enum class WiFiState : uint8_t {
    AP_SETUP,
    CONNECTING,
    STA_CONNECTED,
    STA_RECONNECTING,
    AP_FALLBACK
};

class WiFiManager {
public:
    using StateCallback = std::function<void(WiFiState oldState, WiFiState newState)>;

    void init(bool forceApSetup = false);
    void tick();

    WiFiState getState() const;
    String getLocalIP() const;
    String getSSID() const;
    void onStateChange(StateCallback callback);

private:
    WiFiState _state = WiFiState::AP_SETUP;
    bool _stateInitialized = false;
    std::vector<StateCallback> _callbacks;

    // Atomic flags set by WiFi.onEvent() handler (runs on WiFi system task)
    std::atomic<bool> _gotIP{false};
    std::atomic<bool> _disconnected{false};

    // Reconnection retry state
    uint8_t _retryCount = 0;
    unsigned long _lastRetryMs = 0;

    static constexpr uint8_t MAX_RETRIES = 12;
    static constexpr unsigned long RETRY_INTERVAL_MS = 5000;

    void _setState(WiFiState newState);
    void _startAP(const char* ssid);
    void _startSTA(const String& ssid, const String& password);
    void _startAPFallback();
    void _onConnected();
    void _onDisconnected();
    void _updateSystemStatus();

    static void _onWiFiEvent(WiFiEvent_t event);
    static WiFiManager* _instance;
};
