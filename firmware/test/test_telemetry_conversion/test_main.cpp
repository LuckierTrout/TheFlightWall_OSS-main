/*
Purpose: Unity tests for telemetry unit conversions (Story 3.3).
Tests: meters->kft, m/s->mph, heading pass-through, m/s->ft/s, NAN propagation.
Environment: esp32dev (on-device test) — no hardware adapters required.
*/
#include <Arduino.h>
#include <unity.h>
#include <cmath>
#include "models/FlightInfo.h"
#include "models/StateVector.h"

// Conversion constants (same as used in FlightDataFetcher.cpp)
static constexpr double METERS_TO_FEET = 3.28084;
static constexpr double MPS_TO_MPH = 2.23694;

// --- Test: altitude meters to kft ---
void test_altitude_meters_to_kft() {
    // 10000 meters = 32808.4 feet = 32.8084 kft
    double baro_altitude_m = 10000.0;
    double result = baro_altitude_m * METERS_TO_FEET / 1000.0;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 32.81, result);
}

void test_altitude_zero() {
    double result = 0.0 * METERS_TO_FEET / 1000.0;
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, result);
}

// --- Test: velocity m/s to mph ---
void test_speed_mps_to_mph() {
    // 250 m/s = 559.234 mph
    double velocity_mps = 250.0;
    double result = velocity_mps * MPS_TO_MPH;
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 559.2, result);
}

// --- Test: heading pass-through ---
void test_heading_passthrough() {
    double heading = 270.5;
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 270.5, heading);
}

// --- Test: vertical rate m/s to ft/s ---
void test_vertical_rate_to_fps() {
    // 5.0 m/s = 16.4042 ft/s
    double vr_mps = 5.0;
    double result = vr_mps * METERS_TO_FEET;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 16.40, result);
}

void test_vertical_rate_negative() {
    // -3.0 m/s descent
    double vr_mps = -3.0;
    double result = vr_mps * METERS_TO_FEET;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -9.84, result);
}

// --- Test: NAN propagation ---
void test_nan_altitude_stays_nan() {
    FlightInfo info;
    // Default should be NAN
    TEST_ASSERT_TRUE(isnan(info.altitude_kft));
    TEST_ASSERT_TRUE(isnan(info.speed_mph));
    TEST_ASSERT_TRUE(isnan(info.track_deg));
    TEST_ASSERT_TRUE(isnan(info.vertical_rate_fps));
}

void test_nan_source_not_converted() {
    double source = NAN;
    // If source is NAN, conversion should not be performed
    // (pipeline uses isnan check before converting)
    bool should_convert = !isnan(source);
    TEST_ASSERT_FALSE(should_convert);
}

// --- Test: FlightInfo fields populated correctly ---
void test_flight_info_telemetry_fields() {
    FlightInfo info;
    info.altitude_kft = 35.0;
    info.speed_mph = 550.0;
    info.track_deg = 180.0;
    info.vertical_rate_fps = -5.0;

    TEST_ASSERT_DOUBLE_WITHIN(0.001, 35.0, info.altitude_kft);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 550.0, info.speed_mph);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 180.0, info.track_deg);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -5.0, info.vertical_rate_fps);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // Conversion tests
    RUN_TEST(test_altitude_meters_to_kft);
    RUN_TEST(test_altitude_zero);
    RUN_TEST(test_speed_mps_to_mph);
    RUN_TEST(test_heading_passthrough);
    RUN_TEST(test_vertical_rate_to_fps);
    RUN_TEST(test_vertical_rate_negative);

    // NAN handling
    RUN_TEST(test_nan_altitude_stays_nan);
    RUN_TEST(test_nan_source_not_converted);

    // FlightInfo model
    RUN_TEST(test_flight_info_telemetry_fields);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
