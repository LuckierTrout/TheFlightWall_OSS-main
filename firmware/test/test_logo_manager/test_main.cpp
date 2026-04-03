/*
Purpose: Unity tests for LogoManager.
Tests: init creates /logos/, load existing logo, fallback on missing, fallback on bad size,
       count/usage reflect filesystem state.
Environment: esp32dev (on-device test) — requires LittleFS.
*/
#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include "core/LogoManager.h"

// Helper: remove all files in /logos/ and the directory itself
static void cleanLogosDir() {
    if (LittleFS.exists("/logos")) {
        File dir = LittleFS.open("/logos");
        if (dir && dir.isDirectory()) {
            File entry = dir.openNextFile();
            while (entry) {
                String path = String("/logos/") + entry.name();
                entry.close();
                LittleFS.remove(path);
                entry = dir.openNextFile();
            }
        }
        LittleFS.rmdir("/logos");
    }
}

// Helper: write a test logo file with given size
static void writeTestLogo(const char* icao, const uint8_t* data, size_t size) {
    String path = String("/logos/") + icao + ".bin";
    File f = LittleFS.open(path, "w");
    if (f) {
        f.write(data, size);
        f.close();
    }
}

// --- Test: init creates /logos/ directory ---
void test_init_creates_logos_dir() {
    cleanLogosDir();
    TEST_ASSERT_FALSE(LittleFS.exists("/logos"));

    bool result = LogoManager::init();
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(LittleFS.exists("/logos"));
}

// --- Test: init succeeds when /logos/ already exists ---
void test_init_existing_dir() {
    // /logos/ should exist from previous test
    bool result = LogoManager::init();
    TEST_ASSERT_TRUE(result);
}

// --- Test: load existing logo returns true and copies expected bytes ---
void test_load_existing_logo() {
    cleanLogosDir();
    LogoManager::init();

    // Create a test logo: fill with a known pattern
    uint8_t testData[LOGO_BUFFER_BYTES];
    for (size_t i = 0; i < LOGO_BUFFER_BYTES; i++) {
        testData[i] = (uint8_t)(i & 0xFF);
    }
    writeTestLogo("UAL", testData, LOGO_BUFFER_BYTES);

    uint16_t buffer[LOGO_PIXEL_COUNT];
    memset(buffer, 0, LOGO_BUFFER_BYTES);

    bool result = LogoManager::loadLogo("UAL", buffer);
    TEST_ASSERT_TRUE(result);

    // Verify buffer contents match test data
    TEST_ASSERT_EQUAL_MEMORY(testData, buffer, LOGO_BUFFER_BYTES);
}

// --- Test: load with case normalization ---
void test_load_case_insensitive() {
    // "UAL" logo should still exist from previous test
    uint16_t buffer[LOGO_PIXEL_COUNT];
    memset(buffer, 0, LOGO_BUFFER_BYTES);

    bool result = LogoManager::loadLogo("ual", buffer);
    TEST_ASSERT_TRUE(result);
}

// --- Test: missing logo returns false and writes fallback bytes ---
void test_load_missing_returns_fallback() {
    uint16_t buffer[LOGO_PIXEL_COUNT];
    memset(buffer, 0xAA, LOGO_BUFFER_BYTES);

    bool result = LogoManager::loadLogo("XYZ", buffer);
    TEST_ASSERT_FALSE(result);

    // Buffer should not be all 0xAA — fallback was written
    bool allAA = true;
    for (size_t i = 0; i < LOGO_PIXEL_COUNT; i++) {
        if (buffer[i] != 0xAAAA) {
            allAA = false;
            break;
        }
    }
    TEST_ASSERT_FALSE_MESSAGE(allAA, "Fallback sprite was not copied to buffer");
}

// --- Test: invalid size logo returns fallback path ---
void test_load_bad_size_returns_fallback() {
    cleanLogosDir();
    LogoManager::init();

    // Write a logo with wrong size (half the expected)
    uint8_t smallData[1024];
    memset(smallData, 0x55, sizeof(smallData));
    writeTestLogo("BAD", smallData, sizeof(smallData));

    uint16_t buffer[LOGO_PIXEL_COUNT];
    bool result = LogoManager::loadLogo("BAD", buffer);
    TEST_ASSERT_FALSE(result);
}

// --- Test: empty ICAO returns fallback ---
void test_load_empty_icao() {
    uint16_t buffer[LOGO_PIXEL_COUNT];
    bool result = LogoManager::loadLogo("", buffer);
    TEST_ASSERT_FALSE(result);
}

// --- Test: whitespace-only ICAO returns fallback ---
void test_load_whitespace_icao() {
    uint16_t buffer[LOGO_PIXEL_COUNT];
    bool result = LogoManager::loadLogo("  ", buffer);
    TEST_ASSERT_FALSE(result);
}

// --- Test: null buffer returns false ---
void test_load_null_buffer() {
    bool result = LogoManager::loadLogo("UAL", nullptr);
    TEST_ASSERT_FALSE(result);
}

// --- Test: count reflects filesystem state ---
void test_logo_count() {
    cleanLogosDir();
    LogoManager::init();
    TEST_ASSERT_EQUAL(0, LogoManager::getLogoCount());

    // Add two valid logos
    uint8_t data[LOGO_BUFFER_BYTES];
    memset(data, 0, sizeof(data));
    writeTestLogo("AAL", data, LOGO_BUFFER_BYTES);
    writeTestLogo("DAL", data, LOGO_BUFFER_BYTES);

    // Re-scan by re-init
    LogoManager::init();
    TEST_ASSERT_EQUAL(2, LogoManager::getLogoCount());
}

// --- Test: FS usage returns non-zero values ---
void test_fs_usage() {
    size_t used = 0, total = 0;
    LogoManager::getLittleFSUsage(used, total);
    TEST_ASSERT_GREATER_THAN(0, total);
    // used may be 0 on a fresh filesystem, but total should reflect partition size
}

void setup() {
    delay(2000);

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed — aborting tests");
        return;
    }

    UNITY_BEGIN();

    // Init tests
    RUN_TEST(test_init_creates_logos_dir);
    RUN_TEST(test_init_existing_dir);

    // Logo loading
    RUN_TEST(test_load_existing_logo);
    RUN_TEST(test_load_case_insensitive);
    RUN_TEST(test_load_missing_returns_fallback);
    RUN_TEST(test_load_bad_size_returns_fallback);
    RUN_TEST(test_load_empty_icao);
    RUN_TEST(test_load_whitespace_icao);
    RUN_TEST(test_load_null_buffer);

    // Count and usage
    RUN_TEST(test_logo_count);
    RUN_TEST(test_fs_usage);

    // Clean up
    cleanLogosDir();

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
