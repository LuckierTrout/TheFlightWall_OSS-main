/*
Purpose: Unity tests for LayoutEngine.
Tests: Four architecture test vectors, invalid input behavior.
Environment: esp32dev (native test) — no hardware adapters required.
*/
#include <Arduino.h>
#include <unity.h>
#include "core/LayoutEngine.h"

// Helper: assert a Rect matches expected values
static void assertRect(const char* label, const Rect& r, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    char msg[64];
    snprintf(msg, sizeof(msg), "%s.x", label);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(x, r.x, msg);
    snprintf(msg, sizeof(msg), "%s.y", label);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(y, r.y, msg);
    snprintf(msg, sizeof(msg), "%s.w", label);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(w, r.w, msg);
    snprintf(msg, sizeof(msg), "%s.h", label);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(h, r.h, msg);
}

// --- Test Vector 1: 10x2 @ 16px -> 160x32, full ---
void test_vector_10x2_at_16() {
    LayoutResult r = LayoutEngine::compute(10, 2, 16);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT16(160, r.matrixWidth);
    TEST_ASSERT_EQUAL_UINT16(32, r.matrixHeight);
    TEST_ASSERT_EQUAL_STRING("full", r.mode);
    assertRect("logo",      r.logoZone,      0,  0,  32, 32);
    assertRect("flight",    r.flightZone,    32, 0,  128, 16);
    assertRect("telemetry", r.telemetryZone, 32, 16, 128, 16);
}

// --- Test Vector 2: 5x2 @ 16px -> 80x32, full ---
void test_vector_5x2_at_16() {
    LayoutResult r = LayoutEngine::compute(5, 2, 16);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT16(80, r.matrixWidth);
    TEST_ASSERT_EQUAL_UINT16(32, r.matrixHeight);
    TEST_ASSERT_EQUAL_STRING("full", r.mode);
    assertRect("logo",      r.logoZone,      0,  0,  32, 32);
    assertRect("flight",    r.flightZone,    32, 0,  48, 16);
    assertRect("telemetry", r.telemetryZone, 32, 16, 48, 16);
}

// --- Test Vector 3: 12x3 @ 16px -> 192x48, expanded ---
void test_vector_12x3_at_16() {
    LayoutResult r = LayoutEngine::compute(12, 3, 16);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT16(192, r.matrixWidth);
    TEST_ASSERT_EQUAL_UINT16(48, r.matrixHeight);
    TEST_ASSERT_EQUAL_STRING("expanded", r.mode);
    assertRect("logo",      r.logoZone,      0,  0,  48, 48);
    assertRect("flight",    r.flightZone,    48, 0,  144, 24);
    assertRect("telemetry", r.telemetryZone, 48, 24, 144, 24);
}

// --- Test Vector 4: 10x1 @ 16px -> 160x16, compact ---
void test_vector_10x1_at_16() {
    LayoutResult r = LayoutEngine::compute(10, 1, 16);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT16(160, r.matrixWidth);
    TEST_ASSERT_EQUAL_UINT16(16, r.matrixHeight);
    TEST_ASSERT_EQUAL_STRING("compact", r.mode);
    assertRect("logo",      r.logoZone,      0,  0,  16, 16);
    assertRect("flight",    r.flightZone,    16, 0,  144, 8);
    assertRect("telemetry", r.telemetryZone, 16, 8,  144, 8);
}

// --- Invalid input: zero tiles ---
void test_invalid_zero_tiles() {
    LayoutResult r1 = LayoutEngine::compute(0, 2, 16);
    TEST_ASSERT_FALSE(r1.valid);

    LayoutResult r2 = LayoutEngine::compute(10, 0, 16);
    TEST_ASSERT_FALSE(r2.valid);

    LayoutResult r3 = LayoutEngine::compute(10, 2, 0);
    TEST_ASSERT_FALSE(r3.valid);
}

// --- Invalid input: matrixWidth < matrixHeight ---
void test_invalid_width_less_than_height() {
    // 1x10@16 -> 16x160, width < height
    LayoutResult r = LayoutEngine::compute(1, 10, 16);
    TEST_ASSERT_FALSE(r.valid);
    TEST_ASSERT_EQUAL_UINT16(16, r.matrixWidth);
    TEST_ASSERT_EQUAL_UINT16(160, r.matrixHeight);
}

// --- HardwareConfig overload (post hw-1.3) ---
// Canvas dimensions are fixed by the HW-1 HUB75 build. compute(HardwareConfig)
// always returns 192x128 (master-only) or 192x160 (composite when slave_enabled)
// regardless of any tile values in the caller's struct. These tests verify the
// new dimensions and that zone knobs still flow through correctly.

void test_compute_from_hardware_config_master_only() {
    HardwareConfig hw = {};
    hw.slave_enabled = false;

    LayoutResult r = LayoutEngine::compute(hw);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT16(192, r.matrixWidth);
    TEST_ASSERT_EQUAL_UINT16(128, r.matrixHeight);
    TEST_ASSERT_EQUAL_STRING("expanded", r.mode);
    // Default layout: classic, auto zones — logo is a 128x128 square on the
    // left, flight/telemetry on the right. Precise rect coords depend on
    // FRAME_INSET_PX and LOGO_GAP_PX which are engine internals; just sanity-check
    // the geometry is non-degenerate and fits within the canvas.
    TEST_ASSERT_GREATER_THAN(0, r.logoZone.w);
    TEST_ASSERT_GREATER_THAN(0, r.flightZone.w);
    TEST_ASSERT_GREATER_THAN(0, r.telemetryZone.w);
    TEST_ASSERT_LESS_OR_EQUAL(192, r.logoZone.x + r.logoZone.w);
    TEST_ASSERT_LESS_OR_EQUAL(128, r.logoZone.y + r.logoZone.h);
}

void test_compute_from_hardware_config_composite() {
    HardwareConfig hw = {};
    hw.slave_enabled = true;

    LayoutResult r = LayoutEngine::compute(hw);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT16(192, r.matrixWidth);
    TEST_ASSERT_EQUAL_UINT16(160, r.matrixHeight);
    TEST_ASSERT_EQUAL_STRING("expanded", r.mode);
}

void test_compute_from_hardware_config_with_horizontal_padding() {
    HardwareConfig hw = {};
    hw.slave_enabled = false;
    hw.zone_pad_x = 4;

    LayoutResult r = LayoutEngine::compute(hw);
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_UINT16(192, r.matrixWidth);
    TEST_ASSERT_EQUAL_UINT16(128, r.matrixHeight);
    // With pad=4, logo x should start at 4 + FRAME_INSET_PX.
    TEST_ASSERT_GREATER_OR_EQUAL(4, r.logoZone.x);
}

void test_compute_full_width_bottom_with_horizontal_padding() {
    HardwareConfig hw = {};
    hw.slave_enabled = false;
    hw.zone_layout = 1;
    hw.zone_pad_x = 4;

    LayoutResult r = LayoutEngine::compute(hw);
    TEST_ASSERT_TRUE(r.valid);
    // zone_layout=1 means the telemetry band spans full content width below
    // the split line; sanity-check that the telemetry zone is wider than
    // either the logo or flight zones.
    TEST_ASSERT_GREATER_THAN(r.logoZone.w, r.telemetryZone.w);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // Architecture test vectors
    RUN_TEST(test_vector_10x2_at_16);
    RUN_TEST(test_vector_5x2_at_16);
    RUN_TEST(test_vector_12x3_at_16);
    RUN_TEST(test_vector_10x1_at_16);

    // Invalid input
    RUN_TEST(test_invalid_zero_tiles);
    RUN_TEST(test_invalid_width_less_than_height);

    // HardwareConfig overload
    RUN_TEST(test_compute_from_hardware_config_master_only);
    RUN_TEST(test_compute_from_hardware_config_composite);
    RUN_TEST(test_compute_from_hardware_config_with_horizontal_padding);
    RUN_TEST(test_compute_full_width_bottom_with_horizontal_padding);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
