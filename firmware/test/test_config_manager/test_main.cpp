/*
Purpose: Unity tests for ConfigManager.
Tests: NVS defaults, write/read round-trip, applyJson hot vs reboot paths,
       debounce scheduling, requiresReboot key detection.
Environment: esp32dev (on-device test) — requires NVS flash.
*/
#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"

// Helper: clear NVS namespace before tests
static void clearNvs() {
    Preferences prefs;
    prefs.begin("flightwall", false);
    prefs.clear();
    prefs.end();
}

// --- Default Fallback Tests ---

void test_defaults_display_brightness() {
    clearNvs();
    ConfigManager::init();
    DisplayConfig d = ConfigManager::getDisplay();
    TEST_ASSERT_EQUAL_UINT8(5, d.brightness);
}

void test_defaults_display_text_color() {
    DisplayConfig d = ConfigManager::getDisplay();
    TEST_ASSERT_EQUAL_UINT8(255, d.text_color_r);
    TEST_ASSERT_EQUAL_UINT8(255, d.text_color_g);
    TEST_ASSERT_EQUAL_UINT8(255, d.text_color_b);
}

void test_defaults_location() {
    LocationConfig l = ConfigManager::getLocation();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 37.7749, l.center_lat);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -122.4194, l.center_lon);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 10.0, l.radius_km);
}

void test_defaults_hardware() {
    HardwareConfig h = ConfigManager::getHardware();
    TEST_ASSERT_EQUAL_UINT8(10, h.tiles_x);
    TEST_ASSERT_EQUAL_UINT8(2, h.tiles_y);
    TEST_ASSERT_EQUAL_UINT8(16, h.tile_pixels);
    TEST_ASSERT_EQUAL_UINT8(25, h.display_pin);
}

void test_defaults_timing() {
    TimingConfig t = ConfigManager::getTiming();
    TEST_ASSERT_EQUAL_UINT16(30, t.fetch_interval);
    TEST_ASSERT_EQUAL_UINT16(3, t.display_cycle);
}

// --- NVS Write + Read Round-trip ---

void test_nvs_write_read_roundtrip() {
    clearNvs();

    // Write a value to NVS directly
    Preferences prefs;
    prefs.begin("flightwall", false);
    prefs.putUChar("brightness", 42);
    prefs.putDouble("center_lat", 40.7128);
    prefs.putUShort("fetch_interval", 60);
    prefs.putString("wifi_ssid", "TestNet");
    prefs.end();

    // Re-init ConfigManager — should pick up NVS values
    ConfigManager::init();

    TEST_ASSERT_EQUAL_UINT8(42, ConfigManager::getDisplay().brightness);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 40.7128, ConfigManager::getLocation().center_lat);
    TEST_ASSERT_EQUAL_UINT16(60, ConfigManager::getTiming().fetch_interval);
    TEST_ASSERT_TRUE(ConfigManager::getNetwork().wifi_ssid == "TestNet");
}

// --- applyJson Hot-Reload Path ---

void test_apply_json_hot_reload() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["brightness"] = 100;
    doc["text_color_r"] = 200;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(2, result.applied.size());
    TEST_ASSERT_FALSE(result.reboot_required);
    TEST_ASSERT_EQUAL_UINT8(100, ConfigManager::getDisplay().brightness);
    TEST_ASSERT_EQUAL_UINT8(200, ConfigManager::getDisplay().text_color_r);
}

void test_apply_json_matrix_mapping_hot_reload() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["origin_corner"] = 3;
    doc["scan_dir"] = 1;
    doc["zigzag"] = 1;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(3, result.applied.size());
    TEST_ASSERT_FALSE(result.reboot_required);
    TEST_ASSERT_EQUAL_UINT8(3, ConfigManager::getHardware().origin_corner);
    TEST_ASSERT_EQUAL_UINT8(1, ConfigManager::getHardware().scan_dir);
    TEST_ASSERT_EQUAL_UINT8(1, ConfigManager::getHardware().zigzag);
}

void test_apply_json_hot_reload_persists_after_debounce() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["brightness"] = 77;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(1, result.applied.size());
    TEST_ASSERT_FALSE(result.reboot_required);

    Preferences prefs;
    prefs.begin("flightwall", true);
    TEST_ASSERT_EQUAL_UINT8(0, prefs.getUChar("brightness", 0));
    prefs.end();

    delay(2100);
    ConfigManager::tick();

    prefs.begin("flightwall", true);
    TEST_ASSERT_EQUAL_UINT8(77, prefs.getUChar("brightness", 0));
    prefs.end();
}

// --- applyJson Reboot Path ---

void test_apply_json_reboot_path() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["wifi_ssid"] = "NewNetwork";
    doc["wifi_password"] = "secret123";
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(2, result.applied.size());
    TEST_ASSERT_TRUE(result.reboot_required);
    TEST_ASSERT_TRUE(ConfigManager::getNetwork().wifi_ssid == "NewNetwork");

    // Verify NVS was persisted immediately for reboot keys
    Preferences prefs;
    prefs.begin("flightwall", true);
    String storedSsid = prefs.getString("wifi_ssid", "");
    prefs.end();
    TEST_ASSERT_TRUE(storedSsid == "NewNetwork");
}

// --- applyJson Mixed Keys ---

void test_apply_json_mixed_keys() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["brightness"] = 50;       // hot-reload
    doc["aeroapi_key"] = "abc";   // reboot
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(2, result.applied.size());
    TEST_ASSERT_TRUE(result.reboot_required);
    TEST_ASSERT_EQUAL_UINT8(50, ConfigManager::getDisplay().brightness);
    TEST_ASSERT_TRUE(ConfigManager::getNetwork().aeroapi_key == "abc");
}

void test_apply_json_rejects_unknown_keys() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["brightness"] = 88;
    doc["bogus_key"] = 123;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    // applyJson uses all-or-nothing validation - unknown key rejects entire batch
    TEST_ASSERT_EQUAL(0, result.applied.size());
    TEST_ASSERT_EQUAL_UINT8(5, ConfigManager::getDisplay().brightness);  // default unchanged
}

// --- requiresReboot Key Detection ---

void test_requires_reboot_known_keys() {
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("wifi_ssid"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("wifi_password"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("os_client_id"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("os_client_sec"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("aeroapi_key"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("display_pin"));
}

void test_requires_reboot_hardware_layout_keys() {
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("tiles_x"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("tiles_y"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("tile_pixels"));
}

void test_requires_reboot_hot_reload_keys() {
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("brightness"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("text_color_r"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("center_lat"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("fetch_interval"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("origin_corner"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("scan_dir"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("zigzag"));
}

// --- Factory Reset Tests ---

void test_factory_reset_clears_nvs_and_restores_defaults() {
    clearNvs();
    ConfigManager::init();

    // Write custom values to NVS via applyJson
    JsonDocument doc;
    doc["wifi_ssid"] = "MyNetwork";
    doc["brightness"] = 200;
    doc["center_lat"] = 51.5074;
    JsonObject settings = doc.as<JsonObject>();
    ConfigManager::applyJson(settings);

    // Verify values were applied
    TEST_ASSERT_TRUE(ConfigManager::getNetwork().wifi_ssid == "MyNetwork");
    TEST_ASSERT_EQUAL_UINT8(200, ConfigManager::getDisplay().brightness);

    // Perform factory reset
    ConfigManager::factoryReset();

    // After reset, wifi_ssid should be empty (compile-time default)
    TEST_ASSERT_EQUAL(0, ConfigManager::getNetwork().wifi_ssid.length());

    // Brightness should be back to compile-time default
    TEST_ASSERT_EQUAL_UINT8(5, ConfigManager::getDisplay().brightness);

    // Location should be back to compile-time default
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 37.7749, ConfigManager::getLocation().center_lat);

    // Verify NVS is actually empty
    Preferences prefs;
    prefs.begin("flightwall", true);
    TEST_ASSERT_FALSE(prefs.isKey("wifi_ssid"));
    TEST_ASSERT_FALSE(prefs.isKey("brightness"));
    prefs.end();
}

// --- onChange Callback ---

void test_on_change_callback_fires() {
    clearNvs();
    ConfigManager::init();

    bool callbackFired = false;
    ConfigManager::onChange([&callbackFired]() {
        callbackFired = true;
    });

    JsonDocument doc;
    doc["brightness"] = 77;
    JsonObject settings = doc.as<JsonObject>();
    ConfigManager::applyJson(settings);

    TEST_ASSERT_TRUE(callbackFired);
}

// --- Schedule Tests ---

void test_defaults_schedule() {
    clearNvs();
    ConfigManager::init();
    ScheduleConfig s = ConfigManager::getSchedule();
    TEST_ASSERT_TRUE(s.timezone == "UTC0");
    TEST_ASSERT_EQUAL_UINT8(0, s.sched_enabled);
    TEST_ASSERT_EQUAL_UINT16(1380, s.sched_dim_start);
    TEST_ASSERT_EQUAL_UINT16(420, s.sched_dim_end);
    TEST_ASSERT_EQUAL_UINT8(10, s.sched_dim_brt);
}

void test_nvs_write_read_roundtrip_schedule() {
    clearNvs();

    // Write schedule values to NVS directly
    Preferences prefs;
    prefs.begin("flightwall", false);
    prefs.putString("timezone", "PST8PDT");
    prefs.putUChar("sched_enabled", 1);
    prefs.putUShort("sched_dim_start", 1200);
    prefs.putUShort("sched_dim_end", 360);
    prefs.putUChar("sched_dim_brt", 25);
    prefs.end();

    // Re-init ConfigManager — should pick up NVS values
    ConfigManager::init();

    ScheduleConfig s = ConfigManager::getSchedule();
    TEST_ASSERT_TRUE(s.timezone == "PST8PDT");
    TEST_ASSERT_EQUAL_UINT8(1, s.sched_enabled);
    TEST_ASSERT_EQUAL_UINT16(1200, s.sched_dim_start);
    TEST_ASSERT_EQUAL_UINT16(360, s.sched_dim_end);
    TEST_ASSERT_EQUAL_UINT8(25, s.sched_dim_brt);
}

void test_apply_json_schedule_hot_reload() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["timezone"] = "PST8PDT";
    doc["sched_enabled"] = 1;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(2, result.applied.size());
    TEST_ASSERT_FALSE(result.reboot_required);
    TEST_ASSERT_TRUE(ConfigManager::getSchedule().timezone == "PST8PDT");
    TEST_ASSERT_EQUAL_UINT8(1, ConfigManager::getSchedule().sched_enabled);
}

void test_apply_json_schedule_validation() {
    clearNvs();
    ConfigManager::init();

    // Test sched_enabled > 1 is rejected
    JsonDocument doc1;
    doc1["sched_enabled"] = 2;
    ApplyResult result1 = ConfigManager::applyJson(doc1.as<JsonObject>());
    TEST_ASSERT_EQUAL(0, result1.applied.size());

    // Test sched_dim_start > 1439 is rejected
    JsonDocument doc2;
    doc2["sched_dim_start"] = 1440;
    ApplyResult result2 = ConfigManager::applyJson(doc2.as<JsonObject>());
    TEST_ASSERT_EQUAL(0, result2.applied.size());

    // Test sched_dim_end > 1439 is rejected
    JsonDocument doc3;
    doc3["sched_dim_end"] = 1440;
    ApplyResult result3 = ConfigManager::applyJson(doc3.as<JsonObject>());
    TEST_ASSERT_EQUAL(0, result3.applied.size());

    // Test timezone too long is rejected (>40 chars)
    JsonDocument doc4;
    doc4["timezone"] = "ThisIsAVeryLongTimezoneStringThatExceedsTheMaximumLengthOf40Characters";
    ApplyResult result4 = ConfigManager::applyJson(doc4.as<JsonObject>());
    TEST_ASSERT_EQUAL(0, result4.applied.size());

    // Test empty timezone string is rejected (would reach configTzTime("") otherwise)
    JsonDocument doc4b;
    doc4b["timezone"] = "";
    ApplyResult result4b = ConfigManager::applyJson(doc4b.as<JsonObject>());
    TEST_ASSERT_EQUAL(0, result4b.applied.size());

    // Test sched_dim_brt > 255 is rejected
    JsonDocument doc5;
    doc5["sched_dim_brt"] = 256;
    ApplyResult result5 = ConfigManager::applyJson(doc5.as<JsonObject>());
    TEST_ASSERT_EQUAL(0, result5.applied.size());

    // Test sched_enabled overflow (256 wraps to 0 without proper validation)
    JsonDocument doc6;
    doc6["sched_enabled"] = 256;
    ApplyResult result6 = ConfigManager::applyJson(doc6.as<JsonObject>());
    TEST_ASSERT_EQUAL(0, result6.applied.size());
}

void test_dump_settings_json_includes_schedule() {
    clearNvs();
    ConfigManager::init();

    // Set known schedule values
    JsonDocument setDoc;
    setDoc["timezone"] = "EST5EDT";
    setDoc["sched_enabled"] = 1;
    setDoc["sched_dim_start"] = 1320;  // 22:00
    setDoc["sched_dim_end"] = 360;     // 06:00
    setDoc["sched_dim_brt"] = 5;
    ConfigManager::applyJson(setDoc.as<JsonObject>());

    // Call dumpSettingsJson
    JsonDocument outDoc;
    JsonObject out = outDoc.to<JsonObject>();
    ConfigManager::dumpSettingsJson(out);

    // Verify all schedule keys present
    TEST_ASSERT_TRUE(out["timezone"].is<String>());
    TEST_ASSERT_TRUE(out["timezone"] == "EST5EDT");
    TEST_ASSERT_EQUAL_UINT8(1, out["sched_enabled"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT16(1320, out["sched_dim_start"].as<uint16_t>());
    TEST_ASSERT_EQUAL_UINT16(360, out["sched_dim_end"].as<uint16_t>());
    TEST_ASSERT_EQUAL_UINT8(5, out["sched_dim_brt"].as<uint8_t>());

    // Verify total key count (29 keys total: 4 display + 3 location + 10 hardware + 2 timing + 5 network + 5 schedule)
    size_t keyCount = 0;
    for (JsonPair kv : out) keyCount++;
    TEST_ASSERT_EQUAL_UINT32(29, keyCount);
}

// --- SystemStatus Tests ---

void test_system_status_init() {
    SystemStatus::init();
    SubsystemStatus s = SystemStatus::get(Subsystem::WIFI);
    TEST_ASSERT_EQUAL(StatusLevel::OK, s.level);
    TEST_ASSERT_TRUE(s.message == "Not initialized");
}

void test_system_status_set_get() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::WIFI, StatusLevel::OK, "Connected");
    SubsystemStatus s = SystemStatus::get(Subsystem::WIFI);
    TEST_ASSERT_EQUAL(StatusLevel::OK, s.level);
    TEST_ASSERT_TRUE(s.message == "Connected");
}

void test_system_status_error() {
    SystemStatus::set(Subsystem::OPENSKY, StatusLevel::ERROR, "401 Unauthorized");
    SubsystemStatus s = SystemStatus::get(Subsystem::OPENSKY);
    TEST_ASSERT_EQUAL(StatusLevel::ERROR, s.level);
    TEST_ASSERT_TRUE(s.message == "401 Unauthorized");
}

void test_system_status_to_json() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::WIFI, StatusLevel::OK, "Connected");
    SystemStatus::set(Subsystem::NVS, StatusLevel::WARNING, "High usage");

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toJson(obj);

    TEST_ASSERT_TRUE(obj["wifi"].is<JsonObject>());
    TEST_ASSERT_TRUE(obj["wifi"]["level"] == "ok");
    TEST_ASSERT_TRUE(obj["wifi"]["message"] == "Connected");

    TEST_ASSERT_TRUE(obj["nvs"].is<JsonObject>());
    TEST_ASSERT_TRUE(obj["nvs"]["level"] == "warning");
    TEST_ASSERT_TRUE(obj["nvs"]["message"] == "High usage");
}

void test_system_status_ota_ntp() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Ready");
    SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Synced");

    SubsystemStatus ota = SystemStatus::get(Subsystem::OTA);
    TEST_ASSERT_EQUAL(StatusLevel::OK, ota.level);
    TEST_ASSERT_TRUE(ota.message == "Ready");

    SubsystemStatus ntp = SystemStatus::get(Subsystem::NTP);
    TEST_ASSERT_EQUAL(StatusLevel::OK, ntp.level);
    TEST_ASSERT_TRUE(ntp.message == "Synced");

    // Verify OTA and NTP appear in JSON output
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toJson(obj);

    TEST_ASSERT_TRUE(obj["ota"].is<JsonObject>());
    TEST_ASSERT_TRUE(obj["ota"]["level"] == "ok");
    TEST_ASSERT_TRUE(obj["ota"]["message"] == "Ready");

    TEST_ASSERT_TRUE(obj["ntp"].is<JsonObject>());
    TEST_ASSERT_TRUE(obj["ntp"]["level"] == "ok");
    TEST_ASSERT_TRUE(obj["ntp"]["message"] == "Synced");
}

// --- NTP Story fn-2.1 Tests ---

void test_ntp_status_transitions() {
    // Verify NTP subsystem supports WARNING -> OK state transitions
    SystemStatus::init();

    // Initial state: WARNING (clock not set)
    SystemStatus::set(Subsystem::NTP, StatusLevel::WARNING, "Clock not set");
    SubsystemStatus s1 = SystemStatus::get(Subsystem::NTP);
    TEST_ASSERT_EQUAL(StatusLevel::WARNING, s1.level);
    TEST_ASSERT_TRUE(s1.message == "Clock not set");

    // After sync: OK (clock synced)
    SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Clock synced");
    SubsystemStatus s2 = SystemStatus::get(Subsystem::NTP);
    TEST_ASSERT_EQUAL(StatusLevel::OK, s2.level);
    TEST_ASSERT_TRUE(s2.message == "Clock synced");
}

void test_schedule_timezone_default() {
    // AC #1: getSchedule().timezone returns correct default "UTC0"
    clearNvs();
    ConfigManager::init();
    ScheduleConfig s = ConfigManager::getSchedule();
    TEST_ASSERT_TRUE(s.timezone == "UTC0");
}

void test_timezone_is_hot_reload_key() {
    // AC #4: timezone change does NOT require reboot
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("timezone"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("sched_enabled"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("sched_dim_start"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("sched_dim_end"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("sched_dim_brt"));
}

void test_ntp_status_in_json_output() {
    // AC #6: /api/status includes NTP subsystem in JSON.
    // Uses toExtendedJson() to match the production code path in
    // WebPortal::_handleGetStatus() — NTP data lives at data["subsystems"]["ntp"].
    SystemStatus::init();
    SystemStatus::set(Subsystem::NTP, StatusLevel::WARNING, "Clock not set");

    FlightStatsSnapshot dummy = {};
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toExtendedJson(obj, dummy);

    TEST_ASSERT_TRUE(obj["subsystems"]["ntp"].is<JsonObject>());
    TEST_ASSERT_TRUE(obj["subsystems"]["ntp"]["level"] == "warning");
    TEST_ASSERT_TRUE(obj["subsystems"]["ntp"]["message"] == "Clock not set");

    // Transition to OK
    SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Clock synced");
    JsonDocument doc2;
    JsonObject obj2 = doc2.to<JsonObject>();
    SystemStatus::toExtendedJson(obj2, dummy);

    TEST_ASSERT_TRUE(obj2["subsystems"]["ntp"]["level"] == "ok");
    TEST_ASSERT_TRUE(obj2["subsystems"]["ntp"]["message"] == "Clock synced");
}

void test_nvs_invalid_timezone_ignored() {
    // Regression guard: NVS-loaded empty timezone must not override the "UTC0" default.
    // Protects against corrupted NVS data (manual writes, older firmware) reaching
    // configTzTime("") — mirrors the empty-string guard already in applyJson().
    Preferences prefs;
    prefs.begin("flightwall", false);
    prefs.clear();
    prefs.putString("timezone", "");  // Write invalid empty timezone to NVS
    prefs.end();

    ConfigManager::init();
    ScheduleConfig s = ConfigManager::getSchedule();
    TEST_ASSERT_TRUE(s.timezone == "UTC0");  // Default must be preserved
}

// --- Night mode scheduler logic tests (Story fn-2.2) ---

// Helper: replicates the dim-window check from tickNightScheduler() in main.cpp.
// Extracted here so unit tests validate the exact same arithmetic the scheduler uses.
static bool isInDimWindow(uint16_t nowMinutes, uint16_t dimStart, uint16_t dimEnd) {
    if (dimStart <= dimEnd) {
        return (nowMinutes >= dimStart && nowMinutes < dimEnd);
    } else {
        return (nowMinutes >= dimStart || nowMinutes < dimEnd);
    }
}

void test_schedule_dim_window_same_day() {
    // dimStart=480 (08:00), dimEnd=1020 (17:00)
    TEST_ASSERT_TRUE(isInDimWindow(720, 480, 1020));   // 12:00 → IN window
    TEST_ASSERT_FALSE(isInDimWindow(1200, 480, 1020));  // 20:00 → NOT in window
    TEST_ASSERT_FALSE(isInDimWindow(300, 480, 1020));   // 05:00 → NOT in window
}

void test_schedule_dim_window_crosses_midnight() {
    // dimStart=1380 (23:00), dimEnd=420 (07:00)
    TEST_ASSERT_TRUE(isInDimWindow(120, 1380, 420));   // 02:00 → IN window (after midnight)
    TEST_ASSERT_TRUE(isInDimWindow(1400, 1380, 420));  // 23:20 → IN window (before midnight)
    TEST_ASSERT_FALSE(isInDimWindow(720, 1380, 420));  // 12:00 → NOT in window
}

void test_schedule_dim_window_edge_cases() {
    // dimStart == dimEnd (zero-length window) → never in window
    TEST_ASSERT_FALSE(isInDimWindow(600, 600, 600));

    // Exactly at dimStart → IN window (inclusive start)
    TEST_ASSERT_TRUE(isInDimWindow(480, 480, 1020));

    // Exactly at dimEnd → NOT in window (exclusive end)
    TEST_ASSERT_FALSE(isInDimWindow(1020, 480, 1020));

    // Midnight-crossing: exactly at dimStart → IN
    TEST_ASSERT_TRUE(isInDimWindow(1380, 1380, 420));

    // Midnight-crossing: exactly at dimEnd → NOT in
    TEST_ASSERT_FALSE(isInDimWindow(420, 1380, 420));

    // Full day (0 to 1440 is invalid, but 0 to 0 = zero-length)
    TEST_ASSERT_FALSE(isInDimWindow(500, 0, 0));
}

void test_schedule_defaults() {
    // Verify all ScheduleConfig default values (not just sched_dim_brt)
    clearNvs();
    ConfigManager::init();
    ScheduleConfig sched = ConfigManager::getSchedule();
    TEST_ASSERT_EQUAL_UINT8(10, sched.sched_dim_brt);
    TEST_ASSERT_EQUAL_UINT16(1380, sched.sched_dim_start);  // 23:00
    TEST_ASSERT_EQUAL_UINT16(420, sched.sched_dim_end);     // 07:00
    TEST_ASSERT_EQUAL_UINT8(0, sched.sched_enabled);        // disabled by default
}

// --- Per-mode NVS settings tests (Story dl-1.1, AC #3, #6) ---

void test_get_mode_setting_missing_key_returns_default() {
    // AC #3: getModeSetting with absent key returns default, no NVS write
    clearNvs();
    ConfigManager::init();
    int32_t val = ConfigManager::getModeSetting("clock", "format", 0);
    TEST_ASSERT_EQUAL_INT32(0, val);

    // Verify key was NOT created in NVS (read-only path)
    Preferences prefs;
    prefs.begin("flightwall", true);
    TEST_ASSERT_FALSE(prefs.isKey("m_clock_format"));
    prefs.end();
}

void test_set_mode_setting_persists_and_reads_back() {
    // AC #6: setModeSetting persists value, getModeSetting reads it back
    clearNvs();
    ConfigManager::init();
    bool ok = ConfigManager::setModeSetting("clock", "format", 1);
    TEST_ASSERT_TRUE(ok);

    int32_t val = ConfigManager::getModeSetting("clock", "format", 0);
    TEST_ASSERT_EQUAL_INT32(1, val);
}

void test_set_mode_setting_validates_clock_format_range() {
    // Only 0 or 1 is valid for clock/format
    clearNvs();
    ConfigManager::init();
    TEST_ASSERT_TRUE(ConfigManager::setModeSetting("clock", "format", 0));
    TEST_ASSERT_TRUE(ConfigManager::setModeSetting("clock", "format", 1));
    TEST_ASSERT_FALSE(ConfigManager::setModeSetting("clock", "format", 2));
    TEST_ASSERT_FALSE(ConfigManager::setModeSetting("clock", "format", -1));
}

void test_mode_setting_key_length_within_nvs_limit() {
    // m_clock_format = 13 chars, within 15-char NVS limit
    clearNvs();
    ConfigManager::init();
    bool ok = ConfigManager::setModeSetting("clock", "format", 0);
    TEST_ASSERT_TRUE(ok);

    // Verify the composed key exists in NVS
    Preferences prefs;
    prefs.begin("flightwall", true);
    TEST_ASSERT_TRUE(prefs.isKey("m_clock_format"));
    prefs.end();
}

void test_mode_setting_key_too_long_rejected() {
    // Keys that would exceed 15 chars must be rejected
    clearNvs();
    ConfigManager::init();
    // "m_toolong_settingname" would exceed 15 chars
    bool ok = ConfigManager::setModeSetting("toolong", "settingname", 0);
    TEST_ASSERT_FALSE(ok);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // ConfigManager defaults
    RUN_TEST(test_defaults_display_brightness);
    RUN_TEST(test_defaults_display_text_color);
    RUN_TEST(test_defaults_location);
    RUN_TEST(test_defaults_hardware);
    RUN_TEST(test_defaults_timing);

    // NVS round-trip
    RUN_TEST(test_nvs_write_read_roundtrip);

    // Schedule tests
    RUN_TEST(test_defaults_schedule);
    RUN_TEST(test_nvs_write_read_roundtrip_schedule);
    RUN_TEST(test_apply_json_schedule_hot_reload);
    RUN_TEST(test_apply_json_schedule_validation);
    RUN_TEST(test_dump_settings_json_includes_schedule);

    // applyJson paths
    RUN_TEST(test_apply_json_hot_reload);
    RUN_TEST(test_apply_json_matrix_mapping_hot_reload);
    RUN_TEST(test_apply_json_hot_reload_persists_after_debounce);
    RUN_TEST(test_apply_json_reboot_path);
    RUN_TEST(test_apply_json_mixed_keys);
    RUN_TEST(test_apply_json_rejects_unknown_keys);

    // Reboot key detection
    RUN_TEST(test_requires_reboot_known_keys);
    RUN_TEST(test_requires_reboot_hardware_layout_keys);
    RUN_TEST(test_requires_reboot_hot_reload_keys);

    // Factory reset
    RUN_TEST(test_factory_reset_clears_nvs_and_restores_defaults);

    // onChange callback
    RUN_TEST(test_on_change_callback_fires);

    // SystemStatus
    RUN_TEST(test_system_status_init);
    RUN_TEST(test_system_status_set_get);
    RUN_TEST(test_system_status_error);
    RUN_TEST(test_system_status_to_json);
    RUN_TEST(test_system_status_ota_ntp);

    // NTP Story fn-2.1
    RUN_TEST(test_ntp_status_transitions);
    RUN_TEST(test_schedule_timezone_default);
    RUN_TEST(test_timezone_is_hot_reload_key);
    RUN_TEST(test_ntp_status_in_json_output);
    RUN_TEST(test_nvs_invalid_timezone_ignored);

    // Night mode scheduler logic tests (Story fn-2.2)
    RUN_TEST(test_schedule_dim_window_same_day);
    RUN_TEST(test_schedule_dim_window_crosses_midnight);
    RUN_TEST(test_schedule_dim_window_edge_cases);
    RUN_TEST(test_schedule_defaults);

    // Per-mode NVS settings tests (Story dl-1.1)
    RUN_TEST(test_get_mode_setting_missing_key_returns_default);
    RUN_TEST(test_set_mode_setting_persists_and_reads_back);
    RUN_TEST(test_set_mode_setting_validates_clock_format_range);
    RUN_TEST(test_mode_setting_key_length_within_nvs_limit);
    RUN_TEST(test_mode_setting_key_too_long_rejected);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
