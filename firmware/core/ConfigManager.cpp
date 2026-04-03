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

static bool updateConfigValue(DisplayConfig& display,
                              LocationConfig& location,
                              HardwareConfig& hardware,
                              TimingConfig& timing,
                              NetworkConfig& network,
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

    // Hardware
    if (strcmp(key, "tiles_x") == 0)           { hardware.tiles_x = value.as<uint8_t>(); return true; }
    if (strcmp(key, "tiles_y") == 0)           { hardware.tiles_y = value.as<uint8_t>(); return true; }
    if (strcmp(key, "tile_pixels") == 0)       { hardware.tile_pixels = value.as<uint8_t>(); return true; }
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

    // Timing
    if (strcmp(key, "fetch_interval") == 0)    { timing.fetch_interval = value.as<uint16_t>(); return true; }
    if (strcmp(key, "display_cycle") == 0)     { timing.display_cycle = value.as<uint16_t>(); return true; }

    // Network
    if (strcmp(key, "wifi_ssid") == 0)         { network.wifi_ssid = value.as<String>(); return true; }
    if (strcmp(key, "wifi_password") == 0)     { network.wifi_password = value.as<String>(); return true; }
    if (strcmp(key, "os_client_id") == 0)      { network.opensky_client_id = value.as<String>(); return true; }
    if (strcmp(key, "os_client_sec") == 0)     { network.opensky_client_secret = value.as<String>(); return true; }
    if (strcmp(key, "aeroapi_key") == 0)       { network.aeroapi_key = value.as<String>(); return true; }

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

    _timing.fetch_interval = TimingConfiguration::FETCH_INTERVAL_SECONDS;
    _timing.display_cycle = TimingConfiguration::DISPLAY_CYCLE_SECONDS;

    _network.wifi_ssid = WiFiConfiguration::WIFI_SSID;
    _network.wifi_password = WiFiConfiguration::WIFI_PASSWORD;
    _network.opensky_client_id = APIConfiguration::OPENSKY_CLIENT_ID;
    _network.opensky_client_secret = APIConfiguration::OPENSKY_CLIENT_SECRET;
    _network.aeroapi_key = APIConfiguration::AEROAPI_KEY;
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

    // Hardware
    if (prefs.isKey("tiles_x"))        snapshot.hardware.tiles_x = prefs.getUChar("tiles_x", snapshot.hardware.tiles_x);
    if (prefs.isKey("tiles_y"))        snapshot.hardware.tiles_y = prefs.getUChar("tiles_y", snapshot.hardware.tiles_y);
    if (prefs.isKey("tile_pixels"))    snapshot.hardware.tile_pixels = prefs.getUChar("tile_pixels", snapshot.hardware.tile_pixels);
    if (prefs.isKey("display_pin")) {
        const uint8_t storedPin = prefs.getUChar("display_pin", snapshot.hardware.display_pin);
        if (isSupportedDisplayPin(storedPin)) {
            snapshot.hardware.display_pin = storedPin;
        }
    }
    if (prefs.isKey("origin_corner"))  snapshot.hardware.origin_corner = prefs.getUChar("origin_corner", snapshot.hardware.origin_corner);
    if (prefs.isKey("scan_dir"))       snapshot.hardware.scan_dir = prefs.getUChar("scan_dir", snapshot.hardware.scan_dir);
    if (prefs.isKey("zigzag"))         snapshot.hardware.zigzag = prefs.getUChar("zigzag", snapshot.hardware.zigzag);

    // Timing
    if (prefs.isKey("fetch_interval")) snapshot.timing.fetch_interval = prefs.getUShort("fetch_interval", snapshot.timing.fetch_interval);
    if (prefs.isKey("display_cycle"))  snapshot.timing.display_cycle = prefs.getUShort("display_cycle", snapshot.timing.display_cycle);

    // Network
    if (prefs.isKey("wifi_ssid"))      snapshot.network.wifi_ssid = prefs.getString("wifi_ssid", snapshot.network.wifi_ssid);
    if (prefs.isKey("wifi_password"))  snapshot.network.wifi_password = prefs.getString("wifi_password", snapshot.network.wifi_password);
    if (prefs.isKey("os_client_id"))   snapshot.network.opensky_client_id = prefs.getString("os_client_id", snapshot.network.opensky_client_id);
    if (prefs.isKey("os_client_sec"))  snapshot.network.opensky_client_secret = prefs.getString("os_client_sec", snapshot.network.opensky_client_secret);
    if (prefs.isKey("aeroapi_key"))    snapshot.network.aeroapi_key = prefs.getString("aeroapi_key", snapshot.network.aeroapi_key);

    prefs.end();
    {
        ConfigLockGuard guard;
        _display = snapshot.display;
        _location = snapshot.location;
        _hardware = snapshot.hardware;
        _timing = snapshot.timing;
        _network = snapshot.network;
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

    // Timing
    prefs.putUShort("fetch_interval", _timing.fetch_interval);
    prefs.putUShort("display_cycle", _timing.display_cycle);

    // Network
    prefs.putString("wifi_ssid", _network.wifi_ssid);
    prefs.putString("wifi_password", _network.wifi_password);
    prefs.putString("os_client_id", _network.opensky_client_id);
    prefs.putString("os_client_sec", _network.opensky_client_secret);
    prefs.putString("aeroapi_key", _network.aeroapi_key);

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

    // Timing
    out["fetch_interval"] = snapshot.timing.fetch_interval;
    out["display_cycle"] = snapshot.timing.display_cycle;

    // Network — NVS abbreviated keys for API round-trip consistency
    out["wifi_ssid"] = snapshot.network.wifi_ssid;
    out["wifi_password"] = snapshot.network.wifi_password;
    out["os_client_id"] = snapshot.network.opensky_client_id;
    out["os_client_sec"] = snapshot.network.opensky_client_secret;
    out["aeroapi_key"] = snapshot.network.aeroapi_key;
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

bool ConfigManager::requiresReboot(const char* key) {
    for (size_t i = 0; i < REBOOT_KEY_COUNT; i++) {
        if (strcmp(key, REBOOT_KEYS[i]) == 0) return true;
    }
    return false;
}

bool ConfigManager::updateCacheFromKey(const char* key, JsonVariant value) {
    return updateConfigValue(_display, _location, _hardware, _timing, _network, key, value);
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

        for (JsonPair kv : settings) {
            const char* key = kv.key().c_str();
            JsonVariant value = kv.value();

            if (!updateConfigValue(snapshot.display, snapshot.location, snapshot.hardware, snapshot.timing, snapshot.network, key, value)) {
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

// Compile-time URL accessors — these never change at runtime
const char* ConfigManager::getOpenSkyTokenUrl() { return APIConfiguration::OPENSKY_TOKEN_URL; }
const char* ConfigManager::getOpenSkyBaseUrl() { return APIConfiguration::OPENSKY_BASE_URL; }
const char* ConfigManager::getAeroApiBaseUrl() { return APIConfiguration::AEROAPI_BASE_URL; }
const char* ConfigManager::getFlightWallCdnBaseUrl() { return APIConfiguration::FLIGHTWALL_CDN_BASE_URL; }
bool ConfigManager::getAeroApiInsecureTls() { return APIConfiguration::AEROAPI_INSECURE_TLS; }
bool ConfigManager::getFlightWallInsecureTls() { return APIConfiguration::FLIGHTWALL_INSECURE_TLS; }
