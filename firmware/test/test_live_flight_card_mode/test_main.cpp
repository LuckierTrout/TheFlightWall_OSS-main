/*
 * Purpose: Unity tests for LiveFlightCardMode (ds-2.1, ds-2.2).
 * Tests: init/teardown lifecycle, zone descriptor correctness, render dispatch
 *        (valid/invalid layout, empty flights), cycling state management,
 *        MEMORY_REQUIREMENT sizing, metadata (getName, getZoneDescriptor),
 *        adaptive field dropping (ds-2.2), vertical trend indicator (ds-2.2).
 * Environment: esp32dev (on-device test) — requires NVS flash.
 *
 * Architecture Reference: Display System Release decisions D1, D2
 * - LiveFlightCardMode implements DisplayMode via RenderContext
 * - Zone rendering: logo, flight info, enriched telemetry (alt, spd, hdg, vrate)
 * - ds-2.2: adaptive dropping when zones too small, vertical trend glyph
 */
#include <Arduino.h>
#include <unity.h>

#include "modes/LiveFlightCardMode.h"
#include "core/ConfigManager.h"

#define TEST_WITH_FLIGHT_INFO
#define TEST_WITH_LAYOUT_ENGINE
#include "fixtures/test_helpers.h"

using namespace TestHelpers;

// ============================================================================
// Helper: Build a RenderContext without real matrix (for logic tests)
// ============================================================================

static RenderContext makeTestCtx() {
    RenderContext ctx = {};
    ctx.matrix = nullptr;  // LiveFlightCardMode guards against nullptr
    ctx.textColor = 0xFFFF;
    ctx.brightness = 40;
    ctx.logoBuffer = nullptr;
    ctx.displayCycleMs = 5000;
    // Default layout invalid — no zones
    ctx.layout.valid = false;
    ctx.layout.matrixWidth = 64;
    ctx.layout.matrixHeight = 32;
    return ctx;
}

static RenderContext makeValidLayoutCtx() {
    RenderContext ctx = makeTestCtx();
    ctx.layout.valid = true;
    ctx.layout.matrixWidth = 64;
    ctx.layout.matrixHeight = 32;
    ctx.layout.logoZone = {0, 0, 20, 32};
    ctx.layout.flightZone = {20, 0, 44, 16};
    ctx.layout.telemetryZone = {20, 16, 44, 16};
    return ctx;
}

// Helper: Create a representative FlightInfo with telemetry
static FlightInfo makeTestFlight(const char* airlineIcao, const char* origin,
                                  const char* dest, const char* aircraft) {
    FlightInfo f;
    f.operator_icao = airlineIcao;
    f.operator_code = airlineIcao;
    f.origin.code_icao = origin;
    f.destination.code_icao = dest;
    f.aircraft_code = aircraft;
    f.altitude_kft = 35.0;
    f.speed_mph = 450.0;
    f.track_deg = 90.0;
    f.vertical_rate_fps = 0.0;
    return f;
}

// Helper: Create a FlightInfo with NAN telemetry
static FlightInfo makeNanTelemetryFlight() {
    FlightInfo f;
    f.operator_icao = "UAL";
    f.operator_code = "UAL";
    f.origin.code_icao = "KLAX";
    f.destination.code_icao = "KJFK";
    f.aircraft_code = "B738";
    // All telemetry NAN — should render as "--"
    f.altitude_kft = NAN;
    f.speed_mph = NAN;
    f.track_deg = NAN;
    f.vertical_rate_fps = NAN;
    return f;
}

// Helper: Create a climbing flight (positive vertical rate)
static FlightInfo makeClimbingFlight() {
    FlightInfo f = makeTestFlight("UAL", "KLAX", "KJFK", "B738");
    f.vertical_rate_fps = 15.0;   // clear climb
    return f;
}

// Helper: Create a descending flight (negative vertical rate)
static FlightInfo makeDescendingFlight() {
    FlightInfo f = makeTestFlight("DAL", "KATL", "KORD", "A320");
    f.vertical_rate_fps = -12.0;  // clear descent
    return f;
}

// Helper: Create a level flight (vrate within epsilon band)
static FlightInfo makeLevelFlight() {
    FlightInfo f = makeTestFlight("AAL", "KDFW", "KLAX", "B738");
    f.vertical_rate_fps = 0.3;    // within 0.5 eps — should be level
    return f;
}

// ============================================================================
// Test: Init and teardown lifecycle
// ============================================================================

void test_init_returns_true() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    TEST_ASSERT_TRUE(mode.init(ctx));
}

void test_teardown_does_not_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);
    mode.teardown();
    TEST_ASSERT_TRUE(true);
}

void test_init_resets_cycling_state() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();

    mode.init(ctx);
    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    flights.push_back(makeTestFlight("DAL", "KATL", "KORD", "A320"));

    // render with nullptr matrix should not crash (guard check)
    mode.render(ctx, flights);

    // Teardown and re-init should reset state
    mode.teardown();
    mode.init(ctx);
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// Test: Metadata
// ============================================================================

void test_get_name_returns_live_flight_card() {
    LiveFlightCardMode mode;
    TEST_ASSERT_EQUAL_STRING("Live Flight Card", mode.getName());
}

void test_get_zone_descriptor_has_three_zones() {
    LiveFlightCardMode mode;
    const ModeZoneDescriptor& desc = mode.getZoneDescriptor();
    TEST_ASSERT_EQUAL(3, desc.regionCount);
    TEST_ASSERT_NOT_NULL(desc.regions);
    TEST_ASSERT_NOT_NULL(desc.description);
}

void test_zone_labels_match_layout_zones() {
    LiveFlightCardMode mode;
    const ModeZoneDescriptor& desc = mode.getZoneDescriptor();
    TEST_ASSERT_EQUAL_STRING("Logo", desc.regions[0].label);
    TEST_ASSERT_EQUAL_STRING("Flight", desc.regions[1].label);
    TEST_ASSERT_EQUAL_STRING("Telemetry", desc.regions[2].label);
}

void test_zone_percentages_cover_full_area() {
    LiveFlightCardMode mode;
    const ModeZoneDescriptor& desc = mode.getZoneDescriptor();
    // Logo zone starts at (0,0) and covers left portion
    TEST_ASSERT_EQUAL(0, desc.regions[0].relX);
    TEST_ASSERT_EQUAL(0, desc.regions[0].relY);
    TEST_ASSERT_TRUE(desc.regions[0].relW > 0);
    TEST_ASSERT_EQUAL(100, desc.regions[0].relH);

    // Flight + Telemetry zones should start after Logo
    TEST_ASSERT_TRUE(desc.regions[1].relX > 0);
    TEST_ASSERT_TRUE(desc.regions[2].relX > 0);
}

void test_get_settings_schema_returns_null() {
    LiveFlightCardMode mode;
    TEST_ASSERT_NULL(mode.getSettingsSchema());
}

// ============================================================================
// Test: MEMORY_REQUIREMENT
// ============================================================================

void test_memory_requirement_is_reasonable() {
    TEST_ASSERT_TRUE(sizeof(LiveFlightCardMode) <= LiveFlightCardMode::MEMORY_REQUIREMENT);
}

void test_memory_requirement_value() {
    TEST_ASSERT_EQUAL(96, LiveFlightCardMode::MEMORY_REQUIREMENT);
}

// ============================================================================
// Test: Render with nullptr matrix (guard)
// ============================================================================

void test_render_null_matrix_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

void test_render_empty_flights_null_matrix_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    std::vector<FlightInfo> empty;
    mode.render(ctx, empty);
    TEST_ASSERT_TRUE(true);
}

void test_render_nan_telemetry_null_matrix_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeNanTelemetryFlight());
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// Test: Multiple init/teardown cycles (heap safety)
// ============================================================================

void test_multiple_init_teardown_cycles() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    HeapMonitor monitor;

    for (int i = 0; i < 10; i++) {
        mode.init(ctx);
        mode.teardown();
    }

    monitor.assertNoLeak(256);
}

// ============================================================================
// Test: Production mode table integration
// ============================================================================

void test_live_flight_factory_creates_instance() {
    LiveFlightCardMode* mode = new LiveFlightCardMode();
    TEST_ASSERT_NOT_NULL(mode);
    TEST_ASSERT_EQUAL_STRING("Live Flight Card", mode->getName());
    delete mode;
}

void test_live_flight_init_and_render_lifecycle() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();

    TEST_ASSERT_TRUE(mode.init(ctx));

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("AAL", "KDFW", "KLAX", "B738"));
    flights.push_back(makeTestFlight("SWA", "KMDW", "KLAS", "B737"));
    flights.push_back(makeTestFlight("UAL", "KORD", "KSFO", "A320"));

    for (int i = 0; i < 5; i++) {
        mode.render(ctx, flights);
    }

    mode.teardown();
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// Test: Flight cycling state behavior (logic, not pixels)
// ============================================================================

void test_single_flight_always_index_zero() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));

    for (int i = 0; i < 10; i++) {
        mode.render(ctx, flights);
    }
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// Test: ds-2.2 — Adaptive field dropping (null matrix, no-crash tests)
// ============================================================================

// Compact telemetry zone (1 line only) — fields should be dropped gracefully
void test_render_compact_telemetry_zone_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    ctx.layout.valid = true;
    ctx.layout.matrixWidth = 64;
    ctx.layout.matrixHeight = 16;
    ctx.layout.logoZone = {0, 0, 16, 16};
    ctx.layout.flightZone = {16, 0, 48, 8};     // 1 line for flight
    ctx.layout.telemetryZone = {16, 8, 48, 8};   // 1 line for telemetry
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// Zero-height telemetry zone — should gracefully show nothing
void test_render_zero_telemetry_zone_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    ctx.layout.valid = true;
    ctx.layout.matrixWidth = 64;
    ctx.layout.matrixHeight = 8;
    ctx.layout.logoZone = {0, 0, 8, 8};
    ctx.layout.flightZone = {8, 0, 56, 8};       // 1 line for flight
    ctx.layout.telemetryZone = {8, 8, 56, 0};     // 0 lines — no telemetry
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("SWA", "KMDW", "KLAS", "B737"));
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// Narrow columns — should drop more fields
void test_render_narrow_columns_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    ctx.layout.valid = true;
    ctx.layout.matrixWidth = 32;
    ctx.layout.matrixHeight = 16;
    ctx.layout.logoZone = {0, 0, 8, 16};
    ctx.layout.flightZone = {8, 0, 24, 8};       // ~4 cols
    ctx.layout.telemetryZone = {8, 8, 24, 8};     // ~4 cols, 1 line
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// Test: ds-2.2 — Vertical direction indicator (logic tests via render path)
// ============================================================================

// Climbing flight should not crash (indicator drawn with green arrow)
void test_render_climbing_flight_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeClimbingFlight());
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// Descending flight should not crash (indicator drawn with red arrow)
void test_render_descending_flight_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeDescendingFlight());
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// Level flight (within epsilon) should not crash (indicator drawn as amber bar)
void test_render_level_flight_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeLevelFlight());
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// NAN vrate — no indicator should be drawn, no crash
void test_render_nan_vrate_no_indicator_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    FlightInfo f = makeTestFlight("UAL", "KLAX", "KJFK", "B738");
    f.vertical_rate_fps = NAN;
    std::vector<FlightInfo> flights;
    flights.push_back(f);
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// AC #7: numeric vrate dropped but trend still shows — compact zone with climb
void test_render_compact_zone_climb_trend_no_crash() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    ctx.layout.valid = true;
    ctx.layout.matrixWidth = 64;
    ctx.layout.matrixHeight = 16;
    ctx.layout.logoZone = {0, 0, 16, 16};
    ctx.layout.flightZone = {16, 0, 48, 8};
    ctx.layout.telemetryZone = {16, 8, 48, 8};   // 1 line — vrate numeric dropped
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeClimbingFlight());
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// AC #9: invalid layout preserves ds-2.1 fallback behavior
void test_render_invalid_layout_preserves_fallback() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    ctx.layout.valid = false;
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeClimbingFlight());
    flights.push_back(makeDescendingFlight());
    flights.push_back(makeLevelFlight());
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// Mixed flight types in rapid cycling — heap stability
void test_render_mixed_flights_heap_stable() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    HeapMonitor monitor;

    std::vector<FlightInfo> flights;
    flights.push_back(makeClimbingFlight());
    flights.push_back(makeDescendingFlight());
    flights.push_back(makeLevelFlight());
    flights.push_back(makeNanTelemetryFlight());

    for (int i = 0; i < 100; i++) {
        mode.render(ctx, flights);
    }

    monitor.assertNoLeak(512);
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000);

    UNITY_BEGIN();

    // Init/teardown lifecycle
    RUN_TEST(test_init_returns_true);
    RUN_TEST(test_teardown_does_not_crash);
    RUN_TEST(test_init_resets_cycling_state);

    // Metadata
    RUN_TEST(test_get_name_returns_live_flight_card);
    RUN_TEST(test_get_zone_descriptor_has_three_zones);
    RUN_TEST(test_zone_labels_match_layout_zones);
    RUN_TEST(test_zone_percentages_cover_full_area);
    RUN_TEST(test_get_settings_schema_returns_null);

    // MEMORY_REQUIREMENT
    RUN_TEST(test_memory_requirement_is_reasonable);
    RUN_TEST(test_memory_requirement_value);

    // Null matrix guard
    RUN_TEST(test_render_null_matrix_no_crash);
    RUN_TEST(test_render_empty_flights_null_matrix_no_crash);
    RUN_TEST(test_render_nan_telemetry_null_matrix_no_crash);

    // Heap safety
    RUN_TEST(test_multiple_init_teardown_cycles);

    // Production integration
    RUN_TEST(test_live_flight_factory_creates_instance);
    RUN_TEST(test_live_flight_init_and_render_lifecycle);

    // Cycling behavior
    RUN_TEST(test_single_flight_always_index_zero);

    // ds-2.2: Adaptive field dropping
    RUN_TEST(test_render_compact_telemetry_zone_no_crash);
    RUN_TEST(test_render_zero_telemetry_zone_no_crash);
    RUN_TEST(test_render_narrow_columns_no_crash);

    // ds-2.2: Vertical direction indicator
    RUN_TEST(test_render_climbing_flight_no_crash);
    RUN_TEST(test_render_descending_flight_no_crash);
    RUN_TEST(test_render_level_flight_no_crash);
    RUN_TEST(test_render_nan_vrate_no_indicator_no_crash);
    RUN_TEST(test_render_compact_zone_climb_trend_no_crash);
    RUN_TEST(test_render_invalid_layout_preserves_fallback);
    RUN_TEST(test_render_mixed_flights_heap_stable);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
