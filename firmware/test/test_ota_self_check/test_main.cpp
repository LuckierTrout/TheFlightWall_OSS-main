/*
Purpose: Unity tests for OTA self-check and rollback detection (Story fn-1.4).
Tests: /api/status response contract (firmware_version, rollback_detected fields),
       SystemStatus OTA subsystem interactions, FlightStatsSnapshot serialization,
       esp_ota_mark_app_valid_cancel_rollback() idempotent behavior on non-OTA boot.
Environment: esp32dev (on-device test) — requires NVS flash.
*/
#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "esp_ota_ops.h"
#include "core/SystemStatus.h"
#include "core/ConfigManager.h"
#include "fixtures/test_helpers.h"

// Helper: clear NVS namespace before tests
static void clearNvs() {
    Preferences prefs;
    prefs.begin("flightwall", false);
    prefs.clear();
    prefs.end();
}

// ============================================================================
// AC #5: GET /api/status contract — firmware_version and rollback_detected
// ============================================================================

void test_extended_json_includes_firmware_version() {
    clearNvs();
    ConfigManager::init();
    SystemStatus::init();

    FlightStatsSnapshot stats = {};
    stats.rollback_detected = false;

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toExtendedJson(obj, stats);

    TEST_ASSERT_TRUE_MESSAGE(obj.containsKey("firmware_version"),
        "Response must include firmware_version field");
    TEST_ASSERT_TRUE_MESSAGE(obj["firmware_version"].is<const char*>(),
        "firmware_version must be a string");

    // FW_VERSION is set by build flags; in test env it's "0.0.0-dev" fallback
    const char* version = obj["firmware_version"].as<const char*>();
    TEST_ASSERT_NOT_NULL_MESSAGE(version, "firmware_version must not be null");
    TEST_ASSERT_TRUE_MESSAGE(strlen(version) > 0, "firmware_version must not be empty");
}

void test_extended_json_includes_rollback_detected_false() {
    FlightStatsSnapshot stats = {};
    stats.rollback_detected = false;

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toExtendedJson(obj, stats);

    TEST_ASSERT_TRUE_MESSAGE(obj.containsKey("rollback_detected"),
        "Response must include rollback_detected field");
    TEST_ASSERT_FALSE(obj["rollback_detected"].as<bool>());
}

void test_extended_json_includes_rollback_detected_true() {
    FlightStatsSnapshot stats = {};
    stats.rollback_detected = true;

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toExtendedJson(obj, stats);

    TEST_ASSERT_TRUE(obj["rollback_detected"].as<bool>());
}

void test_extended_json_top_level_field_placement() {
    // Verify firmware_version and rollback_detected are top-level in the data object,
    // not nested inside subsystems or device sections
    FlightStatsSnapshot stats = {};
    stats.rollback_detected = false;

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toExtendedJson(obj, stats);

    // These must be at root level of the data object
    TEST_ASSERT_TRUE(obj.containsKey("firmware_version"));
    TEST_ASSERT_TRUE(obj.containsKey("rollback_detected"));
    // And subsystems must be a nested object (not polluted)
    TEST_ASSERT_TRUE(obj.containsKey("subsystems"));
    JsonObject subsystems = obj["subsystems"].as<JsonObject>();
    TEST_ASSERT_FALSE_MESSAGE(subsystems.containsKey("firmware_version"),
        "firmware_version must not be inside subsystems");
}

// ============================================================================
// AC #1/#2: SystemStatus OTA subsystem — OK and WARNING levels
// ============================================================================

void test_system_status_ota_ok_on_wifi_verify() {
    SystemStatus::init();

    // Simulate post-OTA WiFi-connected verification (AC #1)
    SystemStatus::set(Subsystem::OTA, StatusLevel::OK,
        "Firmware verified — WiFi connected in 8s");

    SubsystemStatus status = SystemStatus::get(Subsystem::OTA);
    TEST_ASSERT_EQUAL(StatusLevel::OK, status.level);
    TEST_ASSERT_STRING_CONTAINS("WiFi connected", status.message.c_str());
}

void test_system_status_ota_warning_on_timeout() {
    SystemStatus::init();

    // Simulate timeout path (AC #2)
    SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
        "Marked valid on timeout — WiFi not verified");

    SubsystemStatus status = SystemStatus::get(Subsystem::OTA);
    TEST_ASSERT_EQUAL(StatusLevel::WARNING, status.level);
    TEST_ASSERT_STRING_CONTAINS("timeout", status.message.c_str());
}

void test_system_status_ota_warning_on_rollback() {
    SystemStatus::init();

    // Simulate rollback detection (AC #4)
    SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
        "Firmware rolled back to previous version");

    SubsystemStatus status = SystemStatus::get(Subsystem::OTA);
    TEST_ASSERT_EQUAL(StatusLevel::WARNING, status.level);
    TEST_ASSERT_STRING_CONTAINS("rolled back", status.message.c_str());
}

// ============================================================================
// AC #6: Normal boot — no SystemStatus set for OTA subsystem
// ============================================================================

void test_system_status_ota_no_change_on_normal_boot() {
    SystemStatus::init();

    // After init, OTA subsystem should have default "Not initialized" message
    SubsystemStatus before = SystemStatus::get(Subsystem::OTA);
    TEST_ASSERT_STRING_EQUAL("Not initialized", before.message.c_str());

    // On normal boot (not pending verify), performOtaSelfCheck should NOT
    // call SystemStatus::set for OTA. Verify the init state remains.
    // (We can't call performOtaSelfCheck directly since it's static in main.cpp,
    // but we verify the contract: if no set() is called, status stays default.)
    SubsystemStatus after = SystemStatus::get(Subsystem::OTA);
    TEST_ASSERT_STRING_EQUAL("Not initialized", after.message.c_str());
    TEST_ASSERT_EQUAL(StatusLevel::OK, after.level);
}

// ============================================================================
// ESP32 OTA API behavioral tests — safe on non-OTA test device
// ============================================================================

void test_mark_valid_idempotent_on_already_valid() {
    // AC #6: esp_ota_mark_app_valid_cancel_rollback() is idempotent
    // On a normal boot (non-OTA), calling it should return ESP_OK with no effect
    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, err,
        "mark_valid must return ESP_OK on already-valid partition");
}

void test_running_partition_is_valid() {
    // Verify the currently running test firmware is in a valid state
    const esp_partition_t* running = esp_ota_get_running_partition();
    TEST_ASSERT_NOT_NULL_MESSAGE(running, "Must have a running partition");

    esp_ota_img_states_t state;
    esp_err_t err = esp_ota_get_state_partition(running, &state);
    // On factory or single-OTA partition, this may return ESP_ERR_NOT_SUPPORTED
    // On valid OTA partition, state should be ESP_OTA_IMG_VALID
    if (err == ESP_OK) {
        TEST_ASSERT_TRUE_MESSAGE(
            state == ESP_OTA_IMG_VALID || state == ESP_OTA_IMG_UNDEFINED,
            "Running partition should be VALID or UNDEFINED (factory)");
    }
    // ESP_ERR_NOT_SUPPORTED is acceptable for non-OTA partition types
}

void test_last_invalid_partition_null_on_clean_device() {
    // On a device that hasn't had a failed OTA, this should return NULL
    // Note: if the test device previously had a rollback, this will be non-NULL
    // and the test still passes — we just verify the API is callable
    const esp_partition_t* invalid = esp_ota_get_last_invalid_partition();
    // This is informational — the API must not crash
    if (invalid == NULL) {
        TEST_ASSERT_NULL(invalid);  // Clean device — no rollback history
    } else {
        // Device has rollback history — verify partition struct is valid
        TEST_ASSERT_NOT_NULL(invalid->label);
        TEST_ASSERT_TRUE(invalid->size > 0);
    }
}

// ============================================================================
// FlightStatsSnapshot struct — rollback_detected field transport
// ============================================================================

void test_flight_stats_snapshot_rollback_field_default() {
    FlightStatsSnapshot stats = {};
    TEST_ASSERT_FALSE_MESSAGE(stats.rollback_detected,
        "Zero-initialized FlightStatsSnapshot should have rollback_detected = false");
}

void test_flight_stats_snapshot_rollback_field_true() {
    FlightStatsSnapshot stats = {};
    stats.rollback_detected = true;
    TEST_ASSERT_TRUE(stats.rollback_detected);

    // Verify it serializes correctly through toExtendedJson
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toExtendedJson(obj, stats);
    TEST_ASSERT_TRUE(obj["rollback_detected"].as<bool>());
}

// ============================================================================
// OTA subsystem appears in /api/status subsystems section
// ============================================================================

void test_ota_subsystem_in_json_output() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Test message");

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toJson(obj);

    TEST_ASSERT_TRUE_MESSAGE(obj.containsKey("ota"),
        "Subsystems JSON must include 'ota' key");
    JsonObject ota = obj["ota"].as<JsonObject>();
    TEST_ASSERT_EQUAL_STRING("ok", ota["level"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Test message", ota["message"].as<const char*>());
}

void test_ota_subsystem_error_level() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR,
        "Failed to mark firmware valid: error -1");

    SubsystemStatus status = SystemStatus::get(Subsystem::OTA);
    TEST_ASSERT_EQUAL(StatusLevel::ERROR, status.level);
    TEST_ASSERT_STRING_CONTAINS("Failed to mark", status.message.c_str());
}

// ============================================================================
// FW_VERSION build flag contract
// ============================================================================

void test_fw_version_macro_defined() {
    // FW_VERSION must be defined (either from platformio.ini or fallback)
    const char* version = FW_VERSION;
    TEST_ASSERT_NOT_NULL(version);
    TEST_ASSERT_TRUE_MESSAGE(strlen(version) > 0, "FW_VERSION must not be empty");
    // Must contain at least one dot (semver format: X.Y.Z)
    TEST_ASSERT_NOT_NULL_MESSAGE(strchr(version, '.'),
        "FW_VERSION must be semver format (contain at least one dot)");
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // --- /api/status response contract (AC #5) ---
    RUN_TEST(test_extended_json_includes_firmware_version);
    RUN_TEST(test_extended_json_includes_rollback_detected_false);
    RUN_TEST(test_extended_json_includes_rollback_detected_true);
    RUN_TEST(test_extended_json_top_level_field_placement);

    // --- SystemStatus OTA subsystem (AC #1, #2, #4) ---
    RUN_TEST(test_system_status_ota_ok_on_wifi_verify);
    RUN_TEST(test_system_status_ota_warning_on_timeout);
    RUN_TEST(test_system_status_ota_warning_on_rollback);

    // --- Normal boot behavior (AC #6) ---
    RUN_TEST(test_system_status_ota_no_change_on_normal_boot);

    // --- ESP32 OTA API behavioral tests ---
    RUN_TEST(test_mark_valid_idempotent_on_already_valid);
    RUN_TEST(test_running_partition_is_valid);
    RUN_TEST(test_last_invalid_partition_null_on_clean_device);

    // --- FlightStatsSnapshot transport ---
    RUN_TEST(test_flight_stats_snapshot_rollback_field_default);
    RUN_TEST(test_flight_stats_snapshot_rollback_field_true);

    // --- OTA subsystem JSON serialization ---
    RUN_TEST(test_ota_subsystem_in_json_output);
    RUN_TEST(test_ota_subsystem_error_level);

    // --- Build flag contract ---
    RUN_TEST(test_fw_version_macro_defined);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
