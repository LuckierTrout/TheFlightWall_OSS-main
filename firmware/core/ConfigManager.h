#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

// NVS key abbreviations (15-char limit):
// os_client_id   = opensky_client_id
// os_client_sec  = opensky_client_secret
// scan_dir       = scan_direction
// sched_enabled  = schedule_enabled (13 chars)
// sched_dim_start = schedule_dim_start (15 chars - AT LIMIT)
// sched_dim_end  = schedule_dim_end (13 chars)
// sched_dim_brt  = schedule_dim_brightness (13 chars)

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
    uint8_t zone_logo_pct;   // 0 = auto (square logo), 1-99 = % of matrix width
    uint8_t zone_split_pct;  // 0 = auto (50/50), 1-99 = % of matrix height for flight zone
    uint8_t zone_layout;     // 0 = classic (logo full-height left), 1 = full-width bottom
};

struct TimingConfig {
    uint16_t fetch_interval, display_cycle;
};

struct NetworkConfig {
    String wifi_ssid, wifi_password;
    String opensky_client_id, opensky_client_secret;
    String aeroapi_key;
};

// Schedule configuration for night mode / brightness scheduling
// NVS keys: timezone (8), sched_enabled (13), sched_dim_start (15),
//           sched_dim_end (13), sched_dim_brt (13) — all within 15-char limit
struct ScheduleConfig {
    String timezone;           // POSIX timezone string, default "UTC0", max 40 chars
    uint8_t sched_enabled;     // 0=disabled, 1=enabled
    uint16_t sched_dim_start;  // minutes since midnight (0-1439), default 1380 (23:00)
    uint16_t sched_dim_end;    // minutes since midnight (0-1439), default 420 (07:00)
    uint8_t sched_dim_brt;     // brightness during dim window (0-255), default 10
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
    static ScheduleConfig getSchedule();
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
    static ScheduleConfig _schedule;

    static std::vector<std::function<void()>> _callbacks;
    static unsigned long _persistScheduledAt;
    static bool _persistPending;

    static void loadDefaults();
    static void loadFromNvs();
    static void persistToNvs();
    static void fireCallbacks();
    static bool updateCacheFromKey(const char* key, JsonVariant value);
};
