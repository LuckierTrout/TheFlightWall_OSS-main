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

static bool isSupportedDisplayPin(uint8_t pin);
static bool isValidTileConfig(uint8_t tiles_x, uint8_t tiles_y, uint8_t tile_pixels);

// Reject tile configs that would exceed ESP32 RAM (~160KB usable).
// CRGB is 3 bytes per pixel; cap at 16,384 pixels (~49KB).
static bool isValidTileConfig(uint8_t tiles_x, uint8_t tiles_y, uint8_t tile_pixels) {
    if (tiles_x == 0 || tiles_y == 0 || tile_pixels == 0) return false;
    uint32_t total = (uint32_t)tiles_x * tiles_y * tile_pixels * tile_pixels;
    return total <= 16384;
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

    // Hardware — validate tile values to prevent OOM
    if (strcmp(key, "tiles_x") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (!isValidTileConfig(v, hardware.tiles_y, hardware.tile_pixels)) return false;
        hardware.tiles_x = v; return true;
    }
    if (strcmp(key, "tiles_y") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (!isValidTileConfig(hardware.tiles_x, v, hardware.tile_pixels)) return false;
        hardware.tiles_y = v; return true;
    }
    if (strcmp(key, "tile_pixels") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (!isValidTileConfig(hardware.tiles_x, hardware.tiles_y, v)) return false;
        hardware.tile_pixels = v; return true;
    }
    if (strcmp(key, "display_pin") == 0) {
        const uint8_t pin = value.as<uint8_t>();
        if (!isSupportedDisplayPin(pin)) {
            return false;
        }
        hardware.display_pin = pin;
        return true;
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

    // Timing
    if (strcmp(key, "fetch_interval") == 0)    { timing.fetch_interval = value.as<uint16_t>(); return true; }
    if (strcmp(key, "display_cycle") == 0)     { timing.display_cycle = value.as<uint16_t>(); return true; }

    // Network
    if (strcmp(key, "wifi_ssid") == 0)         { network.wifi_ssid = value.as<String>(); return true; }
    if (strcmp(key, "wifi_password") == 0)     { network.wifi_password = value.as<String>(); return true; }
    if (strcmp(key, "os_client_id") == 0)      { network.opensky_client_id = value.as<String>(); return true; }
    if (strcmp(key, "os_client_sec") == 0)     { network.opensky_client_secret = value.as<String>(); return true; }
    if (strcmp(key, "aeroapi_key") == 0)       { network.aeroapi_key = value.as<String>(); return true; }

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

static bool isSupportedDisplayPin(uint8_t pin) {
    switch (pin) {
        case 0:
        case 2:
        case 4:
        case 5:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 21:
        case 22:
        case 23:
        case 25:
        case 26:
        case 27:
        case 32:
        case 33:
            return true;
        default:
            return false;
    }
}

// Reboot-required keys
static const char* REBOOT_KEYS[] = {
    "wifi_ssid", "wifi_password",
    "os_client_id", "os_client_sec",
    "aeroapi_key", "display_pin",
    "tiles_x", "tiles_y", "tile_pixels"
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

    _hardware.tiles_x = HardwareConfiguration::DISPLAY_TILES_X;
    _hardware.tiles_y = HardwareConfiguration::DISPLAY_TILES_Y;
    _hardware.tile_pixels = HardwareConfiguration::DISPLAY_TILE_PIXEL_W;
    _hardware.display_pin = HardwareConfiguration::DISPLAY_PIN;
    _hardware.origin_corner = 0;
    _hardware.scan_dir = 0;
    _hardware.zigzag = 0;
    _hardware.zone_logo_pct = 0;
    _hardware.zone_split_pct = 0;
    _hardware.zone_layout = 0;

    _timing.fetch_interval = TimingConfiguration::FETCH_INTERVAL_SECONDS;
    _timing.display_cycle = TimingConfiguration::DISPLAY_CYCLE_SECONDS;

    _network.wifi_ssid = WiFiConfiguration::WIFI_SSID;
    _network.wifi_password = WiFiConfiguration::WIFI_PASSWORD;
    _network.opensky_client_id = APIConfiguration::OPENSKY_CLIENT_ID;
    _network.opensky_client_secret = APIConfiguration::OPENSKY_CLIENT_SECRET;
    _network.aeroapi_key = APIConfiguration::AEROAPI_KEY;

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

    // Hardware — validate tile config to prevent OOM crash
    if (prefs.isKey("tiles_x") || prefs.isKey("tiles_y") || prefs.isKey("tile_pixels")) {
        uint8_t tx = prefs.isKey("tiles_x") ? prefs.getUChar("tiles_x", snapshot.hardware.tiles_x) : snapshot.hardware.tiles_x;
        uint8_t ty = prefs.isKey("tiles_y") ? prefs.getUChar("tiles_y", snapshot.hardware.tiles_y) : snapshot.hardware.tiles_y;
        uint8_t tp = prefs.isKey("tile_pixels") ? prefs.getUChar("tile_pixels", snapshot.hardware.tile_pixels) : snapshot.hardware.tile_pixels;
        if (isValidTileConfig(tx, ty, tp)) {
            snapshot.hardware.tiles_x = tx;
            snapshot.hardware.tiles_y = ty;
            snapshot.hardware.tile_pixels = tp;
        } else {
            LOG_E("ConfigManager", "NVS tile config invalid — using defaults");
        }
    }
    if (prefs.isKey("display_pin")) {
        const uint8_t storedPin = prefs.getUChar("display_pin", snapshot.hardware.display_pin);
        if (isSupportedDisplayPin(storedPin)) {
            snapshot.hardware.display_pin = storedPin;
        }
    }
    if (prefs.isKey("origin_corner"))  snapshot.hardware.origin_corner = prefs.getUChar("origin_corner", snapshot.hardware.origin_corner);
    if (prefs.isKey("scan_dir"))       snapshot.hardware.scan_dir = prefs.getUChar("scan_dir", snapshot.hardware.scan_dir);
    if (prefs.isKey("zigzag"))         snapshot.hardware.zigzag = prefs.getUChar("zigzag", snapshot.hardware.zigzag);
    if (prefs.isKey("zone_logo_pct"))  snapshot.hardware.zone_logo_pct = prefs.getUChar("zone_logo_pct", snapshot.hardware.zone_logo_pct);
    if (prefs.isKey("zone_split_pct")) snapshot.hardware.zone_split_pct = prefs.getUChar("zone_split_pct", snapshot.hardware.zone_split_pct);
    if (prefs.isKey("zone_layout"))    snapshot.hardware.zone_layout = prefs.getUChar("zone_layout", snapshot.hardware.zone_layout);

    // Timing
    if (prefs.isKey("fetch_interval")) snapshot.timing.fetch_interval = prefs.getUShort("fetch_interval", snapshot.timing.fetch_interval);
    if (prefs.isKey("display_cycle"))  snapshot.timing.display_cycle = prefs.getUShort("display_cycle", snapshot.timing.display_cycle);

    // Network
    if (prefs.isKey("wifi_ssid"))      snapshot.network.wifi_ssid = prefs.getString("wifi_ssid", snapshot.network.wifi_ssid);
    if (prefs.isKey("wifi_password"))  snapshot.network.wifi_password = prefs.getString("wifi_password", snapshot.network.wifi_password);
    if (prefs.isKey("os_client_id"))   snapshot.network.opensky_client_id = prefs.getString("os_client_id", snapshot.network.opensky_client_id);
    if (prefs.isKey("os_client_sec"))  snapshot.network.opensky_client_secret = prefs.getString("os_client_sec", snapshot.network.opensky_client_secret);
    if (prefs.isKey("aeroapi_key"))    snapshot.network.aeroapi_key = prefs.getString("aeroapi_key", snapshot.network.aeroapi_key);

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

    // Hardware
    prefs.putUChar("tiles_x", _hardware.tiles_x);
    prefs.putUChar("tiles_y", _hardware.tiles_y);
    prefs.putUChar("tile_pixels", _hardware.tile_pixels);
    prefs.putUChar("display_pin", _hardware.display_pin);
    prefs.putUChar("origin_corner", _hardware.origin_corner);
    prefs.putUChar("scan_dir", _hardware.scan_dir);
    prefs.putUChar("zigzag", _hardware.zigzag);
    prefs.putUChar("zone_logo_pct", _hardware.zone_logo_pct);
    prefs.putUChar("zone_split_pct", _hardware.zone_split_pct);
    prefs.putUChar("zone_layout", _hardware.zone_layout);

    // Timing
    prefs.putUShort("fetch_interval", _timing.fetch_interval);
    prefs.putUShort("display_cycle", _timing.display_cycle);

    // Network
    prefs.putString("wifi_ssid", _network.wifi_ssid);
    prefs.putString("wifi_password", _network.wifi_password);
    prefs.putString("os_client_id", _network.opensky_client_id);
    prefs.putString("os_client_sec", _network.opensky_client_secret);
    prefs.putString("aeroapi_key", _network.aeroapi_key);

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

    // Hardware
    out["tiles_x"] = snapshot.hardware.tiles_x;
    out["tiles_y"] = snapshot.hardware.tiles_y;
    out["tile_pixels"] = snapshot.hardware.tile_pixels;
    out["display_pin"] = snapshot.hardware.display_pin;
    out["origin_corner"] = snapshot.hardware.origin_corner;
    out["scan_dir"] = snapshot.hardware.scan_dir;
    out["zigzag"] = snapshot.hardware.zigzag;
    out["zone_logo_pct"] = snapshot.hardware.zone_logo_pct;
    out["zone_split_pct"] = snapshot.hardware.zone_split_pct;
    out["zone_layout"] = snapshot.hardware.zone_layout;

    // Timing
    out["fetch_interval"] = snapshot.timing.fetch_interval;
    out["display_cycle"] = snapshot.timing.display_cycle;

    // Network — NVS abbreviated keys for API round-trip consistency
    out["wifi_ssid"] = snapshot.network.wifi_ssid;
    out["wifi_password"] = snapshot.network.wifi_password;
    out["os_client_id"] = snapshot.network.opensky_client_id;
    out["os_client_sec"] = snapshot.network.opensky_client_secret;
    out["aeroapi_key"] = snapshot.network.aeroapi_key;

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

    // Validate known mode/key ranges
    if (strcmp(abbrev, "clock") == 0 && strcmp(key, "format") == 0) {
        if (value < 0 || value > 1) return false;  // Only 0 (24h) or 1 (12h)
    }
    if (strcmp(abbrev, "depbd") == 0 && strcmp(key, "rows") == 0) {
        if (value < 1 || value > 4) return false;  // Clamp 1-4 (AC #1)
    }

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for mode setting write");
        return false;
    }
    prefs.putInt(nvsKey, value);
    prefs.end();
    LOG_I("ConfigManager", "Mode setting persisted");
    return true;
}

// Compile-time URL accessors — these never change at runtime
const char* ConfigManager::getOpenSkyTokenUrl() { return APIConfiguration::OPENSKY_TOKEN_URL; }
const char* ConfigManager::getOpenSkyBaseUrl() { return APIConfiguration::OPENSKY_BASE_URL; }
const char* ConfigManager::getAeroApiBaseUrl() { return APIConfiguration::AEROAPI_BASE_URL; }
const char* ConfigManager::getFlightWallCdnBaseUrl() { return APIConfiguration::FLIGHTWALL_CDN_BASE_URL; }
bool ConfigManager::getAeroApiInsecureTls() { return APIConfiguration::AEROAPI_INSECURE_TLS; }
bool ConfigManager::getFlightWallInsecureTls() { return APIConfiguration::FLIGHTWALL_INSECURE_TLS; }
