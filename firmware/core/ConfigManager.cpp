/*
Purpose: Centralized configuration manager with NVS persistence and compile-time fallbacks.
Responsibilities:
- Load settings from NVS on init, falling back to compile-time defaults from config headers.
- Provide category struct getters for thread-safe reads (copy semantics).
- Apply JSON patches via applyJson(), distinguishing reboot vs hot-reload keys.
- Debounce NVS writes for hot-reload keys via schedulePersist().
- Only file that includes config/*.h headers (AR13 compliance).
*/
#include "core/ConfigManager.h"
#include "core/ModeOrchestrator.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "utils/Log.h"

// AR13: Only ConfigManager.cpp includes config headers
#include "config/UserConfiguration.h"
#include "config/WiFiConfiguration.h"
#include "config/TimingConfiguration.h"
#include "config/HardwareConfiguration.h"
#include "config/APIConfiguration.h"

static const char* NVS_NAMESPACE = "flightwall";
static SemaphoreHandle_t g_configMutex = nullptr;

struct ConfigSnapshot {
    DisplayConfig display;
    LocationConfig location;
    HardwareConfig hardware;
    TimingConfig timing;
    NetworkConfig network;
    ScheduleConfig schedule;
};

class ConfigLockGuard {
public:
    ConfigLockGuard() {
        if (g_configMutex != nullptr) {
            xSemaphoreTake(g_configMutex, portMAX_DELAY);
        }
    }

    ~ConfigLockGuard() {
        if (g_configMutex != nullptr) {
            xSemaphoreGive(g_configMutex);
        }
    }
};

// Post hw-1.3 (revised 2026-04-23): canvas is fixed at 256x192 uniform;
// there is no composite/slave mode anymore, so the helper collapses to a
// compile-time constant.
static uint8_t maxHorizontalZonePadX() {
    constexpr uint16_t mw = HardwareConfiguration::MASTER_CANVAS_WIDTH;
    constexpr uint16_t mh = HardwareConfiguration::MASTER_CANVAS_HEIGHT;
    static_assert(mw > mh, "canvas must be wider than tall for horizontal padding");
    return static_cast<uint8_t>((mw - mh - 1U) / 2U);
}

static bool updateConfigValue(DisplayConfig& display,
                              LocationConfig& location,
                              HardwareConfig& hardware,
                              TimingConfig& timing,
                              NetworkConfig& network,
                              ScheduleConfig& schedule,
                              const char* key,
                              JsonVariantConst value) {
    // Display
    if (strcmp(key, "brightness") == 0)        { display.brightness = value.as<uint8_t>(); return true; }
    if (strcmp(key, "text_color_r") == 0)      { display.text_color_r = value.as<uint8_t>(); return true; }
    if (strcmp(key, "text_color_g") == 0)      { display.text_color_g = value.as<uint8_t>(); return true; }
    if (strcmp(key, "text_color_b") == 0)      { display.text_color_b = value.as<uint8_t>(); return true; }

    // Location
    if (strcmp(key, "center_lat") == 0)        { location.center_lat = value.as<double>(); return true; }
    if (strcmp(key, "center_lon") == 0)        { location.center_lon = value.as<double>(); return true; }
    if (strcmp(key, "radius_km") == 0)         { location.radius_km = value.as<double>(); return true; }

    // Hardware — legacy tile/pin keys retired in hw-1.3. The HW-1 master chain
    // is fixed at 3x2 of 64x64 (192x128); there is no user-configurable data
    // pin. Silently accept and discard these keys so older wizard/dashboard
    // POSTs keep working until hw-1.4 strips them from the UI.
    if (strcmp(key, "tiles_x") == 0 ||
        strcmp(key, "tiles_y") == 0 ||
        strcmp(key, "tile_pixels") == 0 ||
        strcmp(key, "display_pin") == 0) {
        (void)value;
        return true;  // accepted-but-ignored
    }
    if (strcmp(key, "origin_corner") == 0)     { hardware.origin_corner = value.as<uint8_t>(); return true; }
    if (strcmp(key, "scan_dir") == 0)          { hardware.scan_dir = value.as<uint8_t>(); return true; }
    if (strcmp(key, "zigzag") == 0)            { hardware.zigzag = value.as<uint8_t>(); return true; }
    if (strcmp(key, "zone_logo_pct") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (v > 99) return false;
        hardware.zone_logo_pct = v; return true;
    }
    if (strcmp(key, "zone_split_pct") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (v > 99) return false;
        hardware.zone_split_pct = v; return true;
    }
    if (strcmp(key, "zone_layout") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (v > 1) return false;
        hardware.zone_layout = v; return true;
    }
    if (strcmp(key, "zone_pad_x") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (v > maxHorizontalZonePadX()) return false;
        hardware.zone_pad_x = v; return true;
    }

    // Timing
    if (strcmp(key, "fetch_interval") == 0)    { timing.fetch_interval = value.as<uint16_t>(); return true; }
    if (strcmp(key, "display_cycle") == 0)     { timing.display_cycle = value.as<uint16_t>(); return true; }

    // Network
    if (strcmp(key, "wifi_ssid") == 0)         { network.wifi_ssid = value.as<String>(); return true; }
    if (strcmp(key, "wifi_password") == 0)     { network.wifi_password = value.as<String>(); return true; }
    if (strcmp(key, "agg_url") == 0)           { network.agg_url = value.as<String>(); return true; }
    if (strcmp(key, "agg_token") == 0)         { network.agg_token = value.as<String>(); return true; }

    // Schedule (Foundation release - night mode) — all hot-reload, NOT in REBOOT_KEYS
    if (strcmp(key, "timezone") == 0) {
        if (!value.is<const char*>()) return false;  // Must be string type
        String tz = value.as<String>();
        if (tz.length() == 0) return false;          // Empty string is not a valid TZ
        if (tz.length() > 40) return false;          // Max POSIX timezone length
        schedule.timezone = tz;
        return true;
    }
    if (strcmp(key, "sched_enabled") == 0) {
        // Validate as wider type first to catch overflow before cast
        if (!value.is<unsigned int>()) return false;
        unsigned int v = value.as<unsigned int>();
        if (v > 1) return false;  // Only 0 or 1 valid
        schedule.sched_enabled = static_cast<uint8_t>(v);
        return true;
    }
    if (strcmp(key, "sched_dim_start") == 0) {
        if (!value.is<int>()) return false;  // Must be numeric type
        int32_t v = value.as<int32_t>();
        if (v < 0 || v > 1439) return false;  // 0-1439 (24*60-1 minutes in a day)
        schedule.sched_dim_start = static_cast<uint16_t>(v);
        return true;
    }
    if (strcmp(key, "sched_dim_end") == 0) {
        if (!value.is<int>()) return false;  // Must be numeric type
        int32_t v = value.as<int32_t>();
        if (v < 0 || v > 1439) return false;  // 0-1439 (24*60-1 minutes in a day)
        schedule.sched_dim_end = static_cast<uint16_t>(v);
        return true;
    }
    if (strcmp(key, "sched_dim_brt") == 0) {
        // Accept 0-255 (full uint8_t range) but validate type first
        if (!value.is<unsigned int>()) return false;
        unsigned int v = value.as<unsigned int>();
        if (v > 255) return false;
        schedule.sched_dim_brt = static_cast<uint8_t>(v);
        return true;
    }

    return false;
}

// Reboot-required keys. Post hw-1.3 (revised 2026-04-23): the legacy
// tile/pin keys are silent-discarded and the dual-MCU slave_enabled flag
// is retired along with the dual-MCU plan, so only network keys remain
// reboot-required.
static const char* REBOOT_KEYS[] = {
    "wifi_ssid", "wifi_password",
    "agg_url", "agg_token",
};
static const size_t REBOOT_KEY_COUNT = sizeof(REBOOT_KEYS) / sizeof(REBOOT_KEYS[0]);

// Static member initialization
DisplayConfig ConfigManager::_display = {};
LocationConfig ConfigManager::_location = {};
HardwareConfig ConfigManager::_hardware = {};
TimingConfig ConfigManager::_timing = {};
NetworkConfig ConfigManager::_network = {};
ScheduleConfig ConfigManager::_schedule = {};
std::vector<std::function<void()>> ConfigManager::_callbacks;
unsigned long ConfigManager::_persistScheduledAt = 0;
bool ConfigManager::_persistPending = false;

void ConfigManager::loadDefaults() {
    _display.brightness = UserConfiguration::DISPLAY_BRIGHTNESS;
    _display.text_color_r = UserConfiguration::TEXT_COLOR_R;
    _display.text_color_g = UserConfiguration::TEXT_COLOR_G;
    _display.text_color_b = UserConfiguration::TEXT_COLOR_B;

    _location.center_lat = UserConfiguration::CENTER_LAT;
    _location.center_lon = UserConfiguration::CENTER_LON;
    _location.radius_km = UserConfiguration::RADIUS_KM;

    _hardware.origin_corner = 0;
    _hardware.scan_dir = 0;
    _hardware.zigzag = 0;
    _hardware.zone_logo_pct = 0;
    _hardware.zone_split_pct = 0;
    _hardware.zone_layout = 0;
    _hardware.zone_pad_x = 0;

    _timing.fetch_interval = TimingConfiguration::FETCH_INTERVAL_SECONDS;
    _timing.display_cycle = TimingConfiguration::DISPLAY_CYCLE_SECONDS;

    _network.wifi_ssid = WiFiConfiguration::WIFI_SSID;
    _network.wifi_password = WiFiConfiguration::WIFI_PASSWORD;
    // agg_url / agg_token have no compile-time defaults — they are always
    // provided at runtime via the setup wizard (or factory-reset blank).

    // Schedule defaults (Foundation release - night mode)
    _schedule.timezone = "UTC0";           // POSIX timezone
    _schedule.sched_enabled = 0;           // disabled by default
    _schedule.sched_dim_start = 1380;      // 23:00 = 23 * 60
    _schedule.sched_dim_end = 420;         // 07:00 = 7 * 60
    _schedule.sched_dim_brt = 10;          // dim brightness
}

void ConfigManager::init() {
    if (g_configMutex == nullptr) {
        g_configMutex = xSemaphoreCreateMutex();
        if (g_configMutex == nullptr) {
            LOG_E("ConfigManager", "Failed to create config mutex");
            return;
        }
    }

    {
        ConfigLockGuard guard;
        _persistPending = false;
        _persistScheduledAt = 0;
        _callbacks.clear();  // Prevent stale callbacks from prior init() calls (e.g., repeated test setUp)
        loadDefaults();
    }

    // Override with NVS values where they exist
    loadFromNvs();

    LOG_I("ConfigManager", "Initialized");
}

bool ConfigManager::factoryReset() {
    if (g_configMutex == nullptr) {
        init();
        if (g_configMutex == nullptr) {
            return false;
        }
    }

    ConfigLockGuard guard;
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for factory reset");
        return false;
    }

    prefs.clear();
    prefs.end();

    _persistPending = false;
    _persistScheduledAt = 0;
    loadDefaults();
    LOG_I("ConfigManager", "Factory reset complete — defaults restored");
    return true;
}

void ConfigManager::tick() {
    unsigned long persistScheduledAt = 0;
    bool persistPending = false;
    {
        ConfigLockGuard guard;
        persistPending = _persistPending;
        persistScheduledAt = _persistScheduledAt;
    }

    if (!persistPending) {
        return;
    }

    if ((long)(millis() - persistScheduledAt) >= 0) {
        persistToNvs();
    }
}

void ConfigManager::loadFromNvs() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        LOG_I("ConfigManager", "NVS namespace empty or first boot — using compile-time defaults");
        return;
    }

    ConfigSnapshot snapshot;
    {
        ConfigLockGuard guard;
        snapshot.display = _display;
        snapshot.location = _location;
        snapshot.hardware = _hardware;
        snapshot.timing = _timing;
        snapshot.network = _network;
        snapshot.schedule = _schedule;
    }

    // Display
    if (prefs.isKey("brightness"))     snapshot.display.brightness = prefs.getUChar("brightness", snapshot.display.brightness);
    if (prefs.isKey("text_color_r"))   snapshot.display.text_color_r = prefs.getUChar("text_color_r", snapshot.display.text_color_r);
    if (prefs.isKey("text_color_g"))   snapshot.display.text_color_g = prefs.getUChar("text_color_g", snapshot.display.text_color_g);
    if (prefs.isKey("text_color_b"))   snapshot.display.text_color_b = prefs.getUChar("text_color_b", snapshot.display.text_color_b);

    // Location
    if (prefs.isKey("center_lat"))     snapshot.location.center_lat = prefs.getDouble("center_lat", snapshot.location.center_lat);
    if (prefs.isKey("center_lon"))     snapshot.location.center_lon = prefs.getDouble("center_lon", snapshot.location.center_lon);
    if (prefs.isKey("radius_km"))      snapshot.location.radius_km = prefs.getDouble("radius_km", snapshot.location.radius_km);

    // Hardware — legacy tile/pin keys retired in hw-1.3. `slave_enabled`
    // was added by the initial dual-MCU plan and retired in the 2026-04-23
    // uniform-panel revision. Remove any of these from NVS on boot so the
    // schema drifts cleanly to the new shape.
    for (const char* legacyKey : {"tiles_x", "tiles_y", "tile_pixels",
                                   "display_pin", "slave_enabled"}) {
        if (prefs.isKey(legacyKey)) {
            prefs.remove(legacyKey);
            LOG_I("ConfigManager", "Removed legacy NVS key");
        }
    }
    if (prefs.isKey("origin_corner"))  snapshot.hardware.origin_corner = prefs.getUChar("origin_corner", snapshot.hardware.origin_corner);
    if (prefs.isKey("scan_dir"))       snapshot.hardware.scan_dir = prefs.getUChar("scan_dir", snapshot.hardware.scan_dir);
    if (prefs.isKey("zigzag"))         snapshot.hardware.zigzag = prefs.getUChar("zigzag", snapshot.hardware.zigzag);
    if (prefs.isKey("zone_logo_pct"))  snapshot.hardware.zone_logo_pct = prefs.getUChar("zone_logo_pct", snapshot.hardware.zone_logo_pct);
    if (prefs.isKey("zone_split_pct")) snapshot.hardware.zone_split_pct = prefs.getUChar("zone_split_pct", snapshot.hardware.zone_split_pct);
    if (prefs.isKey("zone_layout"))    snapshot.hardware.zone_layout = prefs.getUChar("zone_layout", snapshot.hardware.zone_layout);
    if (prefs.isKey("zone_pad_x")) {
        const uint8_t storedPad = prefs.getUChar("zone_pad_x", snapshot.hardware.zone_pad_x);
        if (storedPad <= maxHorizontalZonePadX()) {
            snapshot.hardware.zone_pad_x = storedPad;
        } else {
            LOG_W("ConfigManager", "NVS zone_pad_x invalid — using default");
        }
    }

    // Timing
    if (prefs.isKey("fetch_interval")) snapshot.timing.fetch_interval = prefs.getUShort("fetch_interval", snapshot.timing.fetch_interval);
    if (prefs.isKey("display_cycle"))  snapshot.timing.display_cycle = prefs.getUShort("display_cycle", snapshot.timing.display_cycle);

    // Network
    if (prefs.isKey("wifi_ssid"))      snapshot.network.wifi_ssid = prefs.getString("wifi_ssid", snapshot.network.wifi_ssid);
    if (prefs.isKey("wifi_password"))  snapshot.network.wifi_password = prefs.getString("wifi_password", snapshot.network.wifi_password);
    if (prefs.isKey("agg_url"))        snapshot.network.agg_url = prefs.getString("agg_url", snapshot.network.agg_url);
    if (prefs.isKey("agg_token"))      snapshot.network.agg_token = prefs.getString("agg_token", snapshot.network.agg_token);

    // Schedule (Foundation release - night mode)
    // Validate timezone on NVS load using the same rules as applyJson: non-empty, max 40 chars.
    // Guards against corrupted NVS data (manual writes or older firmware) reaching configTzTime().
    if (prefs.isKey("timezone")) {
        String tz = prefs.getString("timezone", snapshot.schedule.timezone);
        if (tz.length() > 0 && tz.length() <= 40) snapshot.schedule.timezone = tz;
        // else: keep the default "UTC0" — invalid NVS value is silently ignored
    }
    // Validate schedule numeric fields on NVS load using the same rules as applyJson.
    // Guards against corrupted NVS data (e.g., manual tooling, flash wear) producing
    // impossible window values that drive tickNightScheduler() every second (Story fn-2.2).
    // anyCorrupted tracks whether any value was clamped so we can self-heal NVS on first loop tick.
    bool anyCorrupted = false;
    if (prefs.isKey("sched_enabled")) {
        uint8_t v = prefs.getUChar("sched_enabled", snapshot.schedule.sched_enabled);
        if (v > 1) { LOG_W("ConfigManager", "Corrupted sched_enabled in NVS — reset to 0"); v = 0; anyCorrupted = true; }
        snapshot.schedule.sched_enabled = v;
    }
    if (prefs.isKey("sched_dim_start")) {
        uint16_t v = prefs.getUShort("sched_dim_start", snapshot.schedule.sched_dim_start);
        if (v > 1439) { LOG_W("ConfigManager", "Corrupted sched_dim_start in NVS — clamped to 1380"); v = 1380; anyCorrupted = true; }
        snapshot.schedule.sched_dim_start = v;
    }
    if (prefs.isKey("sched_dim_end")) {
        uint16_t v = prefs.getUShort("sched_dim_end", snapshot.schedule.sched_dim_end);
        if (v > 1439) { LOG_W("ConfigManager", "Corrupted sched_dim_end in NVS — clamped to 420"); v = 420; anyCorrupted = true; }
        snapshot.schedule.sched_dim_end = v;
    }
    if (prefs.isKey("sched_dim_brt")) {
        // uint8_t (0-255) is always valid per applyJson rules; no additional clamping needed
        snapshot.schedule.sched_dim_brt = prefs.getUChar("sched_dim_brt", snapshot.schedule.sched_dim_brt);
    }

    prefs.end();
    {
        ConfigLockGuard guard;
        _display = snapshot.display;
        _location = snapshot.location;
        _hardware = snapshot.hardware;
        _timing = snapshot.timing;
        _network = snapshot.network;
        _schedule = snapshot.schedule;
    }

    // Self-heal: if any schedule NVS value was clamped, schedule an immediate corrective persist
    // so the warning does not repeat on every subsequent boot (values corrected in NVS).
    if (anyCorrupted) {
        schedulePersist(0);
        LOG_W("ConfigManager", "Scheduling NVS self-heal persist for corrupted schedule fields");
    }

    LOG_I("ConfigManager", "NVS values loaded");
}

void ConfigManager::persistToNvs() {
    ConfigLockGuard guard;

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for writing");
        return;
    }

    // Display
    prefs.putUChar("brightness", _display.brightness);
    prefs.putUChar("text_color_r", _display.text_color_r);
    prefs.putUChar("text_color_g", _display.text_color_g);
    prefs.putUChar("text_color_b", _display.text_color_b);

    // Location
    prefs.putDouble("center_lat", _location.center_lat);
    prefs.putDouble("center_lon", _location.center_lon);
    prefs.putDouble("radius_km", _location.radius_km);

    // Hardware — post hw-1.3: no tile/pin/slave keys persisted.
    prefs.putUChar("origin_corner", _hardware.origin_corner);
    prefs.putUChar("scan_dir", _hardware.scan_dir);
    prefs.putUChar("zigzag", _hardware.zigzag);
    prefs.putUChar("zone_logo_pct", _hardware.zone_logo_pct);
    prefs.putUChar("zone_split_pct", _hardware.zone_split_pct);
    prefs.putUChar("zone_layout", _hardware.zone_layout);
    prefs.putUChar("zone_pad_x", _hardware.zone_pad_x);

    // Timing
    prefs.putUShort("fetch_interval", _timing.fetch_interval);
    prefs.putUShort("display_cycle", _timing.display_cycle);

    // Network
    prefs.putString("wifi_ssid", _network.wifi_ssid);
    prefs.putString("wifi_password", _network.wifi_password);
    prefs.putString("agg_url", _network.agg_url);
    prefs.putString("agg_token", _network.agg_token);

    // Schedule (Foundation release - night mode)
    prefs.putString("timezone", _schedule.timezone);
    prefs.putUChar("sched_enabled", _schedule.sched_enabled);
    prefs.putUShort("sched_dim_start", _schedule.sched_dim_start);
    prefs.putUShort("sched_dim_end", _schedule.sched_dim_end);
    prefs.putUChar("sched_dim_brt", _schedule.sched_dim_brt);

    prefs.end();
    _persistScheduledAt = 0;
    _persistPending = false;
    LOG_I("ConfigManager", "NVS persisted");
}

void ConfigManager::dumpSettingsJson(JsonObject& out) {
    ConfigSnapshot snapshot;
    {
        ConfigLockGuard guard;
        snapshot.display = _display;
        snapshot.location = _location;
        snapshot.hardware = _hardware;
        snapshot.timing = _timing;
        snapshot.network = _network;
        snapshot.schedule = _schedule;
    }

    // Display — uses same key names as updateCacheFromKey / applyJson
    out["brightness"] = snapshot.display.brightness;
    out["text_color_r"] = snapshot.display.text_color_r;
    out["text_color_g"] = snapshot.display.text_color_g;
    out["text_color_b"] = snapshot.display.text_color_b;

    // Location
    out["center_lat"] = snapshot.location.center_lat;
    out["center_lon"] = snapshot.location.center_lon;
    out["radius_km"] = snapshot.location.radius_km;

    // Hardware — post hw-1.3: canvas is fixed (256x192 uniform 4x3 of 64x64).
    // Legacy tile/pin keys are not exposed; neither is slave_enabled (retired
    // along with the dual-MCU plan when the panel mix went uniform).
    out["origin_corner"] = snapshot.hardware.origin_corner;
    out["scan_dir"] = snapshot.hardware.scan_dir;
    out["zigzag"] = snapshot.hardware.zigzag;
    out["zone_logo_pct"] = snapshot.hardware.zone_logo_pct;
    out["zone_split_pct"] = snapshot.hardware.zone_split_pct;
    out["zone_layout"] = snapshot.hardware.zone_layout;
    out["zone_pad_x"] = snapshot.hardware.zone_pad_x;

    // Timing
    out["fetch_interval"] = snapshot.timing.fetch_interval;
    out["display_cycle"] = snapshot.timing.display_cycle;

    // Network — NVS abbreviated keys for API round-trip consistency
    out["wifi_ssid"] = snapshot.network.wifi_ssid;
    out["wifi_password"] = snapshot.network.wifi_password;
    out["agg_url"] = snapshot.network.agg_url;
    out["agg_token"] = snapshot.network.agg_token;

    // Schedule
    out["timezone"] = snapshot.schedule.timezone;
    out["sched_enabled"] = snapshot.schedule.sched_enabled;
    out["sched_dim_start"] = snapshot.schedule.sched_dim_start;
    out["sched_dim_end"] = snapshot.schedule.sched_dim_end;
    out["sched_dim_brt"] = snapshot.schedule.sched_dim_brt;
}

void ConfigManager::persistAllNow() {
    persistToNvs();
    LOG_I("ConfigManager", "Full NVS flush completed");
}

DisplayConfig ConfigManager::getDisplay() {
    ConfigLockGuard guard;
    return _display;
}

LocationConfig ConfigManager::getLocation() {
    ConfigLockGuard guard;
    return _location;
}

HardwareConfig ConfigManager::getHardware() {
    ConfigLockGuard guard;
    return _hardware;
}

TimingConfig ConfigManager::getTiming() {
    ConfigLockGuard guard;
    return _timing;
}

NetworkConfig ConfigManager::getNetwork() {
    ConfigLockGuard guard;
    return _network;
}

ScheduleConfig ConfigManager::getSchedule() {
    ConfigLockGuard guard;
    return _schedule;
}

bool ConfigManager::requiresReboot(const char* key) {
    for (size_t i = 0; i < REBOOT_KEY_COUNT; i++) {
        if (strcmp(key, REBOOT_KEYS[i]) == 0) return true;
    }
    return false;
}

bool ConfigManager::updateCacheFromKey(const char* key, JsonVariant value) {
    return updateConfigValue(_display, _location, _hardware, _timing, _network, _schedule, key, value);
}

ApplyResult ConfigManager::applyJson(const JsonObject& settings) {
    ApplyResult result;
    result.reboot_required = false;

    ConfigSnapshot snapshot;
    bool hasRebootKey = false;
    bool hasHotKey = false;
    {
        ConfigLockGuard guard;
        snapshot.display = _display;
        snapshot.location = _location;
        snapshot.hardware = _hardware;
        snapshot.timing = _timing;
        snapshot.network = _network;
        snapshot.schedule = _schedule;

        for (JsonPair kv : settings) {
            const char* key = kv.key().c_str();
            JsonVariant value = kv.value();

            if (!updateConfigValue(snapshot.display, snapshot.location, snapshot.hardware, snapshot.timing, snapshot.network, snapshot.schedule, key, value)) {
                LOG_E("ConfigManager", "Rejected unknown or invalid key");
                result.applied.clear();
                return result;
            }

            result.applied.push_back(String(key));

            if (requiresReboot(key)) {
                hasRebootKey = true;
            } else {
                hasHotKey = true;
            }
        }

        _display = snapshot.display;
        _location = snapshot.location;
        _hardware = snapshot.hardware;
        _timing = snapshot.timing;
        _network = snapshot.network;
        _schedule = snapshot.schedule;
    }

    if (hasRebootKey) {
        // Reboot keys persist immediately — no debounce
        persistAllNow();
        result.reboot_required = true;
    }

    if (hasHotKey && !hasRebootKey) {
        schedulePersist(2000);
    }

    if (hasHotKey) {
        fireCallbacks();
    }

    return result;
}

void ConfigManager::schedulePersist(uint16_t delayMs) {
    ConfigLockGuard guard;
    _persistScheduledAt = millis() + delayMs;
    _persistPending = true;
}

void ConfigManager::onChange(std::function<void()> callback) {
    ConfigLockGuard guard;
    _callbacks.push_back(callback);
}

void ConfigManager::fireCallbacks() {
    std::vector<std::function<void()>> callbacks;
    {
        ConfigLockGuard guard;
        callbacks = _callbacks;
    }

    for (auto& cb : callbacks) {
        cb();
    }
}

// Display mode NVS persistence (Story ds-1.3, AC #8)
// Key "disp_mode" (9 chars) — within NVS 15-char limit
String ConfigManager::getDisplayMode() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return "classic_card";
    }
    String mode = prefs.getString("disp_mode", "classic_card");
    prefs.end();
    if (mode.length() == 0) {
        return "classic_card";
    }
    return mode;
}

bool ConfigManager::hasDisplayMode() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    bool exists = prefs.isKey("disp_mode");
    prefs.end();
    return exists;
}

void ConfigManager::setDisplayMode(const char* modeId) {
    if (modeId == nullptr || strlen(modeId) == 0) return;
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for display mode write");
        return;
    }
    prefs.putString("disp_mode", modeId);
    prefs.end();
    LOG_I("ConfigManager", "Display mode persisted");
}

// Upgrade notification NVS persistence (Story ds-3.2, AC #4)
void ConfigManager::setUpgNotif(bool enabled) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for upg_notif write");
        return;
    }
    prefs.putUChar("upg_notif", enabled ? 1 : 0);
    prefs.end();
}

bool ConfigManager::getUpgNotif() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return false;
    }
    bool val = prefs.getUChar("upg_notif", 0) == 1;
    prefs.end();
    return val;
}

// Active layout id NVS persistence (Story le-1.1, AC #5)
// Key "layout_active" (13 chars) — within NVS 15-char limit.
// Empty string is a valid "unset" indicator so callers can distinguish
// "never configured" from "explicitly empty".
String ConfigManager::getLayoutActiveId() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return String();
    }
    String val = prefs.getString("layout_active", "");
    prefs.end();
    return val;
}

bool ConfigManager::setLayoutActiveId(const char* id) {
    if (id == nullptr) return false;
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for layout_active write");
        return false;
    }
    size_t written = prefs.putString("layout_active", id);
    prefs.end();
    if (written != strlen(id)) return false;
    // Notify listeners (display task, etc.) so g_configChanged is set and
    // the LE-1.3 AC #7 re-init hook fires when the active layout changes.
    fireCallbacks();
    return true;
}

// Per-mode NVS settings (Story dl-1.1, AC #3/#6)
// Key format: m_{abbrev}_{key} — composed key must be ≤15 chars (NVS limit).
// Read path: never writes to NVS (AC #3 — no accidental key creation on first read).
// Write path: validates range for known mode/key combos, persists immediately.

int32_t ConfigManager::getModeSetting(const char* abbrev, const char* key, int32_t defaultValue) {
    if (abbrev == nullptr || key == nullptr) return defaultValue;

    // Build composed NVS key: m_{abbrev}_{key}
    char nvsKey[16];
    int len = snprintf(nvsKey, sizeof(nvsKey), "m_%s_%s", abbrev, key);
    if (len < 0 || len >= (int)sizeof(nvsKey)) {
        LOG_E("ConfigManager", "Mode setting key too long");
        return defaultValue;
    }

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return defaultValue;
    }
    if (!prefs.isKey(nvsKey)) {
        prefs.end();
        return defaultValue;
    }
    int32_t val = prefs.getInt(nvsKey, defaultValue);
    prefs.end();
    return val;
}

bool ConfigManager::setModeSetting(const char* abbrev, const char* key, int32_t value) {
    if (abbrev == nullptr || key == nullptr) return false;

    // Build composed NVS key: m_{abbrev}_{key}
    char nvsKey[16];
    int len = snprintf(nvsKey, sizeof(nvsKey), "m_%s_%s", abbrev, key);
    if (len < 0 || len >= (int)sizeof(nvsKey)) {
        LOG_E("ConfigManager", "Mode setting key too long");
        return false;
    }

    // Note: Validation of mode-specific value ranges should be performed
    // by the caller using the mode's ModeSettingsSchema (minValue/maxValue)
    // before calling setModeSetting. ConfigManager is storage-only.
    //
    // EXCEPTION: Known mode settings validated here to prevent invalid NVS writes.
    // This ensures AC compliance and prevents violation of write-then-clamp pattern.
    if (strcmp(abbrev, "depbd") == 0 && strcmp(key, "rows") == 0) {
        if (value < 1 || value > 4) {
            LOG_E("ConfigManager", "depbd/rows out of range, must be 1-4");
            return false;
        }
    }
    if (strcmp(abbrev, "clock") == 0 && strcmp(key, "format") == 0) {
        if (value < 0 || value > 1) {
            LOG_E("ConfigManager", "clock/format out of range, must be 0-1");
            return false;
        }
    }

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for mode setting write");
        return false;
    }
    prefs.putInt(nvsKey, value);
    prefs.end();
    LOG_I("ConfigManager", "Mode setting persisted");
    // Notify listeners so display task picks up the change (Story dl-5.1, AC #4)
    fireCallbacks();
    return true;
}

// Mode schedule NVS persistence (Story dl-4.1, AC #1/#2)
// Keys: sched_r{N}_start (14), sched_r{N}_end (12), sched_r{N}_mode (13),
//        sched_r{N}_ena (12), sched_r_count (13) — all within 15-char NVS limit.
// Validation: range checks done here; modeId existence validated by ModeOrchestrator
// at evaluation time (ConfigManager cannot depend on ModeRegistry — circular dep).

ModeScheduleConfig ConfigManager::getModeSchedule() {
    ModeScheduleConfig config = {};
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        return config;
    }

    config.ruleCount = prefs.getUChar("sched_r_count", 0);
    if (config.ruleCount > MAX_SCHEDULE_RULES) {
        config.ruleCount = 0;  // Corrupted — treat as empty
        LOG_W("ConfigManager", "Corrupted sched_r_count in NVS — reset to 0");
    }

    char key[16];
    for (uint8_t i = 0; i < config.ruleCount; i++) {
        snprintf(key, sizeof(key), "sched_r%d_start", i);
        uint16_t start = prefs.getUShort(key, 0);
        if (start > 1439) start = 0;
        config.rules[i].startMin = start;

        snprintf(key, sizeof(key), "sched_r%d_end", i);
        uint16_t end = prefs.getUShort(key, 0);
        if (end > 1439) end = 0;
        config.rules[i].endMin = end;

        snprintf(key, sizeof(key), "sched_r%d_mode", i);
        String mode = prefs.getString(key, "");
        strncpy(config.rules[i].modeId, mode.c_str(), MODE_ID_BUF_LEN - 1);
        config.rules[i].modeId[MODE_ID_BUF_LEN - 1] = '\0';

        snprintf(key, sizeof(key), "sched_r%d_ena", i);
        uint8_t ena = prefs.getUChar(key, 0);
        config.rules[i].enabled = (ena > 1) ? 0 : ena;
    }

    prefs.end();
    return config;
}

bool ConfigManager::setModeSchedule(const ModeScheduleConfig& config) {
    if (config.ruleCount > MAX_SCHEDULE_RULES) {
        LOG_E("ConfigManager", "ruleCount exceeds MAX_SCHEDULE_RULES");
        return false;
    }

    // Validate all rules before persisting (AC #2)
    // NOTE: modeId existence in ModeRegistry is NOT validated here to avoid
    // circular dependency. Invalid modeIds will fail at runtime when
    // ModeOrchestrator::tick() attempts to switch. See AC #2 documentation.
    for (uint8_t i = 0; i < config.ruleCount; i++) {
        const ScheduleRule& r = config.rules[i];
        if (r.startMin > 1439 || r.endMin > 1439) {
            LOG_E("ConfigManager", "Schedule rule time out of range");
            return false;
        }
        if (r.enabled > 1) {
            LOG_E("ConfigManager", "Schedule rule enabled field invalid");
            return false;
        }
        if (strlen(r.modeId) == 0) {
            LOG_E("ConfigManager", "Schedule rule modeId is empty");
            return false;
        }
    }

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for mode schedule write");
        return false;
    }

    // Persist all rules atomically (single NVS session)
    char key[16];
    for (uint8_t i = 0; i < config.ruleCount; i++) {
        const ScheduleRule& r = config.rules[i];

        snprintf(key, sizeof(key), "sched_r%d_start", i);
        prefs.putUShort(key, r.startMin);

        snprintf(key, sizeof(key), "sched_r%d_end", i);
        prefs.putUShort(key, r.endMin);

        snprintf(key, sizeof(key), "sched_r%d_mode", i);
        prefs.putString(key, r.modeId);

        snprintf(key, sizeof(key), "sched_r%d_ena", i);
        prefs.putUChar(key, r.enabled);
    }

    // Clear any old rules beyond new ruleCount
    for (uint8_t i = config.ruleCount; i < MAX_SCHEDULE_RULES; i++) {
        snprintf(key, sizeof(key), "sched_r%d_start", i);
        prefs.remove(key);
        snprintf(key, sizeof(key), "sched_r%d_end", i);
        prefs.remove(key);
        snprintf(key, sizeof(key), "sched_r%d_mode", i);
        prefs.remove(key);
        snprintf(key, sizeof(key), "sched_r%d_ena", i);
        prefs.remove(key);
    }

    prefs.putUChar("sched_r_count", config.ruleCount);
    prefs.end();

    // Invalidate ModeOrchestrator schedule cache so new rules take effect immediately
    // (Story dl-4.1, AC #2: changes must be visible to orchestrator on next tick)
    ModeOrchestrator::invalidateScheduleCache();

    LOG_I("ConfigManager", "Mode schedule persisted");
    return true;
}

// Compile-time URL accessors — these never change at runtime
const char* ConfigManager::getFlightWallCdnBaseUrl() { return APIConfiguration::FLIGHTWALL_CDN_BASE_URL; }
bool ConfigManager::getFlightWallInsecureTls() { return APIConfiguration::FLIGHTWALL_INSECURE_TLS; }
