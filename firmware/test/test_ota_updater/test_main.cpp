/*
Purpose: Unity tests for OTAUpdater version check, download, and failure handling logic.
Stories: dl-6.1 (AC #9), dl-7.1 (AC #10), dl-7.2 (AC #12).
Tests: normalizeVersion, compareVersions, parseReleaseJson, parseSha256File, compareSha256,
       getFailurePhase, isRetriable, subsystemName(OTA_PULL).
Environment: esp32dev (on-device test) — no live GitHub calls or WiFi.
*/
#include <Arduino.h>
#include <unity.h>
#include "core/OTAUpdater.h"

// ============================================================================
// normalizeVersion tests
// ============================================================================

void test_normalize_strips_leading_v() {
    char buf[32];
    TEST_ASSERT_TRUE(OTAUpdater::normalizeVersion("v1.2.3", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("1.2.3", buf);
}

void test_normalize_strips_leading_V() {
    char buf[32];
    TEST_ASSERT_TRUE(OTAUpdater::normalizeVersion("V2.0.0", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("2.0.0", buf);
}

void test_normalize_no_prefix() {
    char buf[32];
    TEST_ASSERT_TRUE(OTAUpdater::normalizeVersion("1.0.0", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("1.0.0", buf);
}

void test_normalize_trims_whitespace() {
    char buf[32];
    TEST_ASSERT_TRUE(OTAUpdater::normalizeVersion("  v3.1.0  ", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("3.1.0", buf);
}

void test_normalize_trims_tabs_and_newlines() {
    char buf[32];
    TEST_ASSERT_TRUE(OTAUpdater::normalizeVersion("\tv1.0.0\r\n", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("1.0.0", buf);
}

void test_normalize_empty_returns_false() {
    char buf[32];
    TEST_ASSERT_FALSE(OTAUpdater::normalizeVersion("", buf, sizeof(buf)));
}

void test_normalize_just_v_returns_false() {
    char buf[32];
    TEST_ASSERT_FALSE(OTAUpdater::normalizeVersion("v", buf, sizeof(buf)));
}

void test_normalize_null_returns_false() {
    char buf[32];
    TEST_ASSERT_FALSE(OTAUpdater::normalizeVersion(nullptr, buf, sizeof(buf)));
}

void test_normalize_null_buf_returns_false() {
    TEST_ASSERT_FALSE(OTAUpdater::normalizeVersion("v1.0.0", nullptr, 32));
}

void test_normalize_zero_buf_returns_false() {
    char buf[1];
    TEST_ASSERT_FALSE(OTAUpdater::normalizeVersion("v1.0.0", buf, 0));
}

// ============================================================================
// compareVersions tests
// ============================================================================

void test_compare_same_returns_false() {
    TEST_ASSERT_FALSE(OTAUpdater::compareVersions("1.0.0", "1.0.0"));
}

void test_compare_different_returns_true() {
    TEST_ASSERT_TRUE(OTAUpdater::compareVersions("1.1.0", "1.0.0"));
}

void test_compare_null_remote_returns_false() {
    TEST_ASSERT_FALSE(OTAUpdater::compareVersions(nullptr, "1.0.0"));
}

void test_compare_null_local_returns_false() {
    TEST_ASSERT_FALSE(OTAUpdater::compareVersions("1.0.0", nullptr));
}

void test_compare_prerelease_tag_differs() {
    // releases/latest only returns stable, but if tag differs it's an update
    TEST_ASSERT_TRUE(OTAUpdater::compareVersions("2.0.0-beta", "1.0.0"));
}

// ============================================================================
// parseReleaseJson tests
// ============================================================================

// Fixture: minimal valid GitHub release JSON with .bin and .sha256 assets
static const char* FIXTURE_VALID_RELEASE = R"({
  "tag_name": "v1.2.0",
  "body": "## What's New\n- Bug fixes\n- Performance improvements",
  "assets": [
    {
      "name": "firmware.bin",
      "browser_download_url": "https://github.com/owner/repo/releases/download/v1.2.0/firmware.bin"
    },
    {
      "name": "firmware.bin.sha256",
      "browser_download_url": "https://github.com/owner/repo/releases/download/v1.2.0/firmware.bin.sha256"
    }
  ]
})";

// Fixture: release with no assets
static const char* FIXTURE_NO_ASSETS = R"({
  "tag_name": "v2.0.0",
  "body": "Release notes here",
  "assets": []
})";

// Fixture: release with only .bin, no .sha256
static const char* FIXTURE_BIN_ONLY = R"({
  "tag_name": "v1.5.0",
  "body": "Binary only release",
  "assets": [
    {
      "name": "flightwall-1.5.0.bin",
      "browser_download_url": "https://example.com/flightwall-1.5.0.bin"
    }
  ]
})";

// Fixture: missing tag_name
static const char* FIXTURE_NO_TAG = R"({
  "body": "No tag",
  "assets": []
})";

void test_parse_valid_release() {
    TEST_ASSERT_TRUE(OTAUpdater::parseReleaseJson(
        FIXTURE_VALID_RELEASE, strlen(FIXTURE_VALID_RELEASE)));

    TEST_ASSERT_EQUAL_STRING("1.2.0", OTAUpdater::getRemoteVersion());
    TEST_ASSERT_TRUE(strlen(OTAUpdater::getReleaseNotes()) > 0);
    TEST_ASSERT_TRUE(strstr(OTAUpdater::getBinaryAssetUrl(), "firmware.bin") != nullptr);
    TEST_ASSERT_TRUE(strstr(OTAUpdater::getSha256AssetUrl(), "firmware.bin.sha256") != nullptr);
}

void test_parse_no_assets() {
    TEST_ASSERT_TRUE(OTAUpdater::parseReleaseJson(
        FIXTURE_NO_ASSETS, strlen(FIXTURE_NO_ASSETS)));

    TEST_ASSERT_EQUAL_STRING("2.0.0", OTAUpdater::getRemoteVersion());
    // Asset URLs should be empty
    TEST_ASSERT_EQUAL_STRING("", OTAUpdater::getBinaryAssetUrl());
    TEST_ASSERT_EQUAL_STRING("", OTAUpdater::getSha256AssetUrl());
}

void test_parse_bin_only_no_sha256() {
    TEST_ASSERT_TRUE(OTAUpdater::parseReleaseJson(
        FIXTURE_BIN_ONLY, strlen(FIXTURE_BIN_ONLY)));

    TEST_ASSERT_EQUAL_STRING("1.5.0", OTAUpdater::getRemoteVersion());
    TEST_ASSERT_TRUE(strlen(OTAUpdater::getBinaryAssetUrl()) > 0);
    TEST_ASSERT_EQUAL_STRING("", OTAUpdater::getSha256AssetUrl());
}

void test_parse_missing_tag_name_fails() {
    TEST_ASSERT_FALSE(OTAUpdater::parseReleaseJson(
        FIXTURE_NO_TAG, strlen(FIXTURE_NO_TAG)));
    // _lastError should contain "Missing tag_name"
    TEST_ASSERT_TRUE(strstr(OTAUpdater::getLastError(), "tag_name") != nullptr);
}

void test_parse_null_json_fails() {
    TEST_ASSERT_FALSE(OTAUpdater::parseReleaseJson(nullptr, 0));
}

void test_parse_empty_json_fails() {
    TEST_ASSERT_FALSE(OTAUpdater::parseReleaseJson("", 0));
}

void test_parse_invalid_json_fails() {
    const char* bad = "this is not json";
    TEST_ASSERT_FALSE(OTAUpdater::parseReleaseJson(bad, strlen(bad)));
}

void test_parse_release_notes_present() {
    OTAUpdater::parseReleaseJson(FIXTURE_VALID_RELEASE, strlen(FIXTURE_VALID_RELEASE));
    const char* notes = OTAUpdater::getReleaseNotes();
    TEST_ASSERT_TRUE(strstr(notes, "Bug fixes") != nullptr);
}

// Fixture: asset with "url" field only (no browser_download_url) - AC #4 fallback test
static const char* FIXTURE_URL_ONLY = R"({
  "tag_name": "v3.0.0",
  "body": "Uses url field",
  "assets": [
    {
      "name": "firmware.bin",
      "url": "https://api.github.com/repos/owner/repo/releases/assets/12345"
    },
    {
      "name": "firmware.bin.sha256",
      "url": "https://api.github.com/repos/owner/repo/releases/assets/12346"
    }
  ]
})";

void test_parse_url_fallback() {
    // AC #4: When browser_download_url is missing, fall back to url field
    TEST_ASSERT_TRUE(OTAUpdater::parseReleaseJson(
        FIXTURE_URL_ONLY, strlen(FIXTURE_URL_ONLY)));

    TEST_ASSERT_EQUAL_STRING("3.0.0", OTAUpdater::getRemoteVersion());
    // Both assets should be found using the url field fallback
    TEST_ASSERT_TRUE(strstr(OTAUpdater::getBinaryAssetUrl(), "assets/12345") != nullptr);
    TEST_ASSERT_TRUE(strstr(OTAUpdater::getSha256AssetUrl(), "assets/12346") != nullptr);
}

void test_parse_clears_old_values_on_new_parse() {
    // First parse a release with assets
    TEST_ASSERT_TRUE(OTAUpdater::parseReleaseJson(
        FIXTURE_VALID_RELEASE, strlen(FIXTURE_VALID_RELEASE)));
    TEST_ASSERT_TRUE(strlen(OTAUpdater::getBinaryAssetUrl()) > 0);
    TEST_ASSERT_TRUE(strlen(OTAUpdater::getSha256AssetUrl()) > 0);

    // Now parse a release with NO assets — old URLs must be cleared
    TEST_ASSERT_TRUE(OTAUpdater::parseReleaseJson(
        FIXTURE_NO_ASSETS, strlen(FIXTURE_NO_ASSETS)));
    TEST_ASSERT_EQUAL_STRING("", OTAUpdater::getBinaryAssetUrl());
    TEST_ASSERT_EQUAL_STRING("", OTAUpdater::getSha256AssetUrl());
}

// ============================================================================
// parseSha256File tests (Story dl-7.1, AC #10)
// ============================================================================

// Known SHA-256 of empty string: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
static const char* SHA256_EMPTY_HEX = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
static const uint8_t SHA256_EMPTY_BIN[32] = {
    0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
    0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
    0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
    0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
};

void test_sha256_parse_valid_hex() {
    uint8_t digest[32];
    TEST_ASSERT_TRUE(OTAUpdater::parseSha256File(SHA256_EMPTY_HEX, 64, digest, sizeof(digest)));
    TEST_ASSERT_EQUAL_MEMORY(SHA256_EMPTY_BIN, digest, 32);
}

void test_sha256_parse_with_filename_suffix() {
    // Common .sha256 format: "abcd...1234  firmware.bin\n"
    const char* shaLine = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855  firmware.bin\n";
    uint8_t digest[32];
    TEST_ASSERT_TRUE(OTAUpdater::parseSha256File(shaLine, strlen(shaLine), digest, sizeof(digest)));
    TEST_ASSERT_EQUAL_MEMORY(SHA256_EMPTY_BIN, digest, 32);
}

void test_sha256_parse_with_leading_whitespace() {
    const char* shaLine = "  \te3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\n";
    uint8_t digest[32];
    TEST_ASSERT_TRUE(OTAUpdater::parseSha256File(shaLine, strlen(shaLine), digest, sizeof(digest)));
    TEST_ASSERT_EQUAL_MEMORY(SHA256_EMPTY_BIN, digest, 32);
}

void test_sha256_parse_uppercase_hex() {
    const char* upper = "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855";
    uint8_t digest[32];
    TEST_ASSERT_TRUE(OTAUpdater::parseSha256File(upper, strlen(upper), digest, sizeof(digest)));
    TEST_ASSERT_EQUAL_MEMORY(SHA256_EMPTY_BIN, digest, 32);
}

void test_sha256_parse_too_short() {
    const char* shortHex = "e3b0c44298fc1c14";
    uint8_t digest[32];
    TEST_ASSERT_FALSE(OTAUpdater::parseSha256File(shortHex, strlen(shortHex), digest, sizeof(digest)));
}

void test_sha256_parse_invalid_char() {
    // 'g' is not valid hex
    const char* bad = "g3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    uint8_t digest[32];
    TEST_ASSERT_FALSE(OTAUpdater::parseSha256File(bad, strlen(bad), digest, sizeof(digest)));
}

void test_sha256_parse_null_body() {
    uint8_t digest[32];
    TEST_ASSERT_FALSE(OTAUpdater::parseSha256File(nullptr, 0, digest, sizeof(digest)));
}

void test_sha256_parse_null_output() {
    TEST_ASSERT_FALSE(OTAUpdater::parseSha256File(SHA256_EMPTY_HEX, 64, nullptr, 0));
}

void test_sha256_parse_small_output_buf() {
    uint8_t digest[16];  // Too small — needs 32
    TEST_ASSERT_FALSE(OTAUpdater::parseSha256File(SHA256_EMPTY_HEX, 64, digest, sizeof(digest)));
}

// ============================================================================
// compareSha256 tests (Story dl-7.1, AC #10)
// ============================================================================

void test_sha256_compare_matching() {
    uint8_t a[32], b[32];
    memcpy(a, SHA256_EMPTY_BIN, 32);
    memcpy(b, SHA256_EMPTY_BIN, 32);
    TEST_ASSERT_TRUE(OTAUpdater::compareSha256(a, b));
}

void test_sha256_compare_different() {
    uint8_t a[32], b[32];
    memcpy(a, SHA256_EMPTY_BIN, 32);
    memcpy(b, SHA256_EMPTY_BIN, 32);
    b[0] ^= 0x01;  // Flip one bit
    TEST_ASSERT_FALSE(OTAUpdater::compareSha256(a, b));
}

void test_sha256_compare_last_byte_different() {
    uint8_t a[32], b[32];
    memcpy(a, SHA256_EMPTY_BIN, 32);
    memcpy(b, SHA256_EMPTY_BIN, 32);
    b[31] ^= 0xFF;
    TEST_ASSERT_FALSE(OTAUpdater::compareSha256(a, b));
}

void test_sha256_compare_null_a() {
    uint8_t b[32];
    TEST_ASSERT_FALSE(OTAUpdater::compareSha256(nullptr, b));
}

void test_sha256_compare_null_b() {
    uint8_t a[32];
    TEST_ASSERT_FALSE(OTAUpdater::compareSha256(a, nullptr));
}

// ============================================================================
// Download guards (Story dl-7.1, AC #10)
// ============================================================================

void test_start_download_rejects_when_not_available() {
    // Reset state and init to ensure IDLE state for deterministic test
    OTAUpdater::_resetForTest();
    OTAUpdater::init("test", "repo");

    // Now state is definitely IDLE, startDownload must return false
    TEST_ASSERT_TRUE(OTAUpdater::getState() == OTAState::IDLE);
    TEST_ASSERT_FALSE(OTAUpdater::startDownload());
    // Verify lastError was set with rejection message
    TEST_ASSERT_TRUE(strlen(OTAUpdater::getLastError()) > 0);
}

void test_progress_initial_zero() {
    OTAUpdater::_resetForTest();
    TEST_ASSERT_EQUAL_UINT8(0, OTAUpdater::getProgress());
}

// ============================================================================
// OTAState lifecycle
// ============================================================================

void test_initial_state_after_init() {
    // Reset and init explicitly to ensure deterministic state
    OTAUpdater::_resetForTest();
    OTAUpdater::init("test", "repo");

    // After init, state MUST be IDLE
    TEST_ASSERT_TRUE(OTAUpdater::getState() == OTAState::IDLE);
}

void test_error_message_empty_after_init() {
    // Reset and init explicitly
    OTAUpdater::_resetForTest();
    OTAUpdater::init("test", "repo");

    const char* err = OTAUpdater::getLastError();
    TEST_ASSERT_NOT_NULL(err);
    // After successful init, lastError MUST be empty
    TEST_ASSERT_EQUAL_STRING("", err);
}

// ============================================================================
// Failure metadata tests (Story dl-7.2, AC #12)
// ============================================================================

void test_failure_phase_initial_none() {
    // Reset to ensure deterministic state
    OTAUpdater::_resetForTest();
    OTAUpdater::init("test", "repo");

    // After init, failure phase MUST be NONE
    TEST_ASSERT_TRUE(OTAUpdater::getFailurePhase() == OTAFailurePhase::NONE);
}

void test_is_retriable_false_when_no_failure() {
    // Reset to ensure NONE phase
    OTAUpdater::_resetForTest();
    OTAUpdater::init("test", "repo");

    // With NONE phase, isRetriable MUST return false
    TEST_ASSERT_TRUE(OTAUpdater::getFailurePhase() == OTAFailurePhase::NONE);
    TEST_ASSERT_FALSE(OTAUpdater::isRetriable());
}

void test_failure_phase_enum_values() {
    // Verify enum values are distinct and ordered
    TEST_ASSERT_TRUE(static_cast<uint8_t>(OTAFailurePhase::NONE) == 0);
    TEST_ASSERT_TRUE(static_cast<uint8_t>(OTAFailurePhase::DOWNLOAD) == 1);
    TEST_ASSERT_TRUE(static_cast<uint8_t>(OTAFailurePhase::VERIFY) == 2);
    TEST_ASSERT_TRUE(static_cast<uint8_t>(OTAFailurePhase::BOOT) == 3);
}

// ============================================================================
// SystemStatus OTA_PULL subsystem name (Story dl-7.2, AC #6, #12)
// ============================================================================

#include "core/SystemStatus.h"

void test_subsystem_ota_pull_exists() {
    // Verify OTA_PULL is a valid Subsystem enum value
    Subsystem otaPull = Subsystem::OTA_PULL;
    uint8_t idx = static_cast<uint8_t>(otaPull);
    // OTA_PULL should be index 8 (appended after NTP=7)
    TEST_ASSERT_EQUAL_UINT8(8, idx);
}

void test_subsystem_ota_pull_get_set() {
    // Verify we can set and get OTA_PULL status via SystemStatus
    SystemStatus::init();
    SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::OK, "Test message");
    SubsystemStatus status = SystemStatus::get(Subsystem::OTA_PULL);
    TEST_ASSERT_TRUE(status.level == StatusLevel::OK);
    TEST_ASSERT_TRUE(status.message.indexOf("Test") >= 0);
}

// ============================================================================
// getStateString tests (Story dl-7.3, AC #3)
// ============================================================================

void test_get_state_string_returns_non_null() {
    const char* s = OTAUpdater::getStateString();
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_TRUE(strlen(s) > 0);
}

void test_get_state_string_known_values() {
    // Verify the known state strings are lowercase and match the contract
    // We can only test the current state, but verify the accessor works
    const char* s = OTAUpdater::getStateString();
    // Must be one of the defined values
    bool valid = (strcmp(s, "idle") == 0 || strcmp(s, "checking") == 0 ||
                  strcmp(s, "available") == 0 || strcmp(s, "downloading") == 0 ||
                  strcmp(s, "verifying") == 0 || strcmp(s, "rebooting") == 0 ||
                  strcmp(s, "error") == 0);
    TEST_ASSERT_TRUE(valid);
}

// ============================================================================
// getFailurePhaseString tests (Story dl-7.3, AC #3)
// ============================================================================

void test_get_failure_phase_string_returns_non_null() {
    const char* s = OTAUpdater::getFailurePhaseString();
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_TRUE(strlen(s) > 0);
}

void test_get_failure_phase_string_known_values() {
    const char* s = OTAUpdater::getFailurePhaseString();
    bool valid = (strcmp(s, "none") == 0 || strcmp(s, "download") == 0 ||
                  strcmp(s, "verify") == 0 || strcmp(s, "boot") == 0);
    TEST_ASSERT_TRUE(valid);
}

void test_get_failure_phase_string_initial_none() {
    // Reset to ensure NONE phase
    OTAUpdater::_resetForTest();
    OTAUpdater::init("test", "repo");

    // After init, failure phase string MUST be "none"
    TEST_ASSERT_TRUE(OTAUpdater::getFailurePhase() == OTAFailurePhase::NONE);
    TEST_ASSERT_EQUAL_STRING("none", OTAUpdater::getFailurePhaseString());
}

// ============================================================================
// OTA pull guard tests (Story dl-7.3, AC #2)
// ============================================================================

void test_start_download_rejects_idle_state() {
    // Reset to ensure IDLE state
    OTAUpdater::_resetForTest();
    OTAUpdater::init("test", "repo");

    // Verify state is IDLE
    TEST_ASSERT_TRUE(OTAUpdater::getState() == OTAState::IDLE);

    // startDownload MUST return false when not in AVAILABLE state
    TEST_ASSERT_FALSE(OTAUpdater::startDownload());

    // lastError MUST be set with rejection message
    TEST_ASSERT_TRUE(strlen(OTAUpdater::getLastError()) > 0);
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // --- normalizeVersion (AC #5, AC #9) ---
    RUN_TEST(test_normalize_strips_leading_v);
    RUN_TEST(test_normalize_strips_leading_V);
    RUN_TEST(test_normalize_no_prefix);
    RUN_TEST(test_normalize_trims_whitespace);
    RUN_TEST(test_normalize_trims_tabs_and_newlines);
    RUN_TEST(test_normalize_empty_returns_false);
    RUN_TEST(test_normalize_just_v_returns_false);
    RUN_TEST(test_normalize_null_returns_false);
    RUN_TEST(test_normalize_null_buf_returns_false);
    RUN_TEST(test_normalize_zero_buf_returns_false);

    // --- compareVersions (AC #5, AC #9) ---
    RUN_TEST(test_compare_same_returns_false);
    RUN_TEST(test_compare_different_returns_true);
    RUN_TEST(test_compare_null_remote_returns_false);
    RUN_TEST(test_compare_null_local_returns_false);
    RUN_TEST(test_compare_prerelease_tag_differs);

    // --- parseReleaseJson (AC #4, AC #9) ---
    RUN_TEST(test_parse_valid_release);
    RUN_TEST(test_parse_no_assets);
    RUN_TEST(test_parse_bin_only_no_sha256);
    RUN_TEST(test_parse_missing_tag_name_fails);
    RUN_TEST(test_parse_null_json_fails);
    RUN_TEST(test_parse_empty_json_fails);
    RUN_TEST(test_parse_invalid_json_fails);
    RUN_TEST(test_parse_release_notes_present);
    RUN_TEST(test_parse_url_fallback);
    RUN_TEST(test_parse_clears_old_values_on_new_parse);

    // --- parseSha256File (dl-7.1, AC #10) ---
    RUN_TEST(test_sha256_parse_valid_hex);
    RUN_TEST(test_sha256_parse_with_filename_suffix);
    RUN_TEST(test_sha256_parse_with_leading_whitespace);
    RUN_TEST(test_sha256_parse_uppercase_hex);
    RUN_TEST(test_sha256_parse_too_short);
    RUN_TEST(test_sha256_parse_invalid_char);
    RUN_TEST(test_sha256_parse_null_body);
    RUN_TEST(test_sha256_parse_null_output);
    RUN_TEST(test_sha256_parse_small_output_buf);

    // --- compareSha256 (dl-7.1, AC #10) ---
    RUN_TEST(test_sha256_compare_matching);
    RUN_TEST(test_sha256_compare_different);
    RUN_TEST(test_sha256_compare_last_byte_different);
    RUN_TEST(test_sha256_compare_null_a);
    RUN_TEST(test_sha256_compare_null_b);

    // --- Download guards (dl-7.1, AC #10) ---
    RUN_TEST(test_start_download_rejects_when_not_available);
    RUN_TEST(test_progress_initial_zero);

    // --- State lifecycle ---
    RUN_TEST(test_initial_state_after_init);
    RUN_TEST(test_error_message_empty_after_init);

    // --- Failure metadata (dl-7.2, AC #12) ---
    RUN_TEST(test_failure_phase_initial_none);
    RUN_TEST(test_is_retriable_false_when_no_failure);
    RUN_TEST(test_failure_phase_enum_values);

    // --- SystemStatus OTA_PULL (dl-7.2, AC #6, #12) ---
    RUN_TEST(test_subsystem_ota_pull_exists);
    RUN_TEST(test_subsystem_ota_pull_get_set);

    // --- getStateString (dl-7.3, AC #3) ---
    RUN_TEST(test_get_state_string_returns_non_null);
    RUN_TEST(test_get_state_string_known_values);

    // --- getFailurePhaseString (dl-7.3, AC #3) ---
    RUN_TEST(test_get_failure_phase_string_returns_non_null);
    RUN_TEST(test_get_failure_phase_string_known_values);
    RUN_TEST(test_get_failure_phase_string_initial_none);

    // --- OTA pull guards (dl-7.3, AC #2) ---
    RUN_TEST(test_start_download_rejects_idle_state);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
