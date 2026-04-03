#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

// NVS key abbreviations (15-char limit):
// os_client_id   = opensky_client_id
// os_client_sec  = opensky_client_secret
// scan_dir       = scan_direction

struct DisplayConfig {
    uint8_t brightness;
    uint8_t text_color_r, text_color_g, text_color_b;
};

struct LocationConfig {
    double center_lat, center_lon, radius_km;
};

struct HardwareConfig {
    uint8_t tiles_x, tiles_y, tile_pixels, display_pin;
    uint8_t origin_corner, scan_dir, zigzag;
};

struct TimingConfig {
    uint16_t fetch_interval, display_cycle;
};

struct NetworkConfig {
    String wifi_ssid, wifi_password;
    String opensky_client_id, opensky_client_secret;
    String aeroapi_key;
};

struct ApplyResult {
    std::vector<String> applied;
    bool reboot_required;
};

class ConfigManager {
public:
    static void init();
    static bool factoryReset();
    static void tick();
    static DisplayConfig getDisplay();
    static LocationConfig getLocation();
    static HardwareConfig getHardware();
    static TimingConfig getTiming();
    static NetworkConfig getNetwork();
    static ApplyResult applyJson(const JsonObject& settings);
    static void dumpSettingsJson(JsonObject& out);
    static void persistAllNow();
    static void schedulePersist(uint16_t delayMs = 2000);
    static void onChange(std::function<void()> callback);
    static bool requiresReboot(const char* key);

    // Compile-time URL accessors (remain in ConfigManager, not NVS)
    static const char* getOpenSkyTokenUrl();
    static const char* getOpenSkyBaseUrl();
    static const char* getAeroApiBaseUrl();
    static const char* getFlightWallCdnBaseUrl();
    static bool getAeroApiInsecureTls();
    static bool getFlightWallInsecureTls();

private:
    static DisplayConfig _display;
    static LocationConfig _location;
    static HardwareConfig _hardware;
    static TimingConfig _timing;
    static NetworkConfig _network;

    static std::vector<std::function<void()>> _callbacks;
    static unsigned long _persistScheduledAt;
    static bool _persistPending;

    static void loadDefaults();
    static void loadFromNvs();
    static void persistToNvs();
    static void fireCallbacks();
    static bool updateCacheFromKey(const char* key, JsonVariant value);
};
