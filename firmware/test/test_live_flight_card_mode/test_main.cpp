/*
 * Purpose: Unity tests for LiveFlightCardMode (ds-2.1).
 * Tests: init/teardown lifecycle, zone descriptor correctness, render dispatch
 *        (valid/invalid layout, empty flights), cycling state management,
 *        MEMORY_REQUIREMENT sizing, metadata (getName, getZoneDescriptor).
 * Environment: esp32dev (on-device test) — requires NVS flash.
 *
 * Architecture Reference: Display System Release decisions D1, D2
 * - LiveFlightCardMode implements DisplayMode via RenderContext
 * - Zone rendering: logo, flight info, enriched telemetry (alt, spd, hdg, vrate)
 * - ds-2.2 scope: adaptive field dropping + vertical trend glyph (classifyVerticalTrend,
 *   computeTelemetryFields tested here as public static methods; drawVerticalTrend tested
 *   indirectly via render() with null-matrix no-crash guards)
 *
 * Note on null-matrix tests: cycling state is updated before the matrix null guard,
 * so cycling-logic tests that pass ctx.matrix=nullptr exercise the state machine
 * without needing hardware. Draw-path tests require a physical matrix.
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
    // Cycling state is updated before the null guard, so this exercises
    // the state machine even without a physical matrix.
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();  // ctx.matrix = nullptr

    mode.init(ctx);
    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    flights.push_back(makeTestFlight("DAL", "KATL", "KORD", "A320"));

    // Cycling state updates before null guard — these calls exercise state logic
    mode.render(ctx, flights);
    mode.render(ctx, flights);

    // Teardown and re-init must reset cycle fields
    mode.teardown();
    mode.init(ctx);
    mode.render(ctx, flights);
    // After teardown + init the timer anchors on first render; index stays 0 (no elapsed interval)
    TEST_ASSERT_EQUAL(0, mode.getCurrentFlightIndex());
    TEST_ASSERT_TRUE(mode.getLastCycleMs() > 0);  // Timer anchored after first multi-flight render
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
// Test: Render with nullptr matrix (null guard)
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
    // Cycling state updates before null guard; render calls exercise state machine
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();  // ctx.matrix = nullptr

    TEST_ASSERT_TRUE(mode.init(ctx));

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("AAL", "KDFW", "KLAX", "B738"));
    flights.push_back(makeTestFlight("SWA", "KMDW", "KLAS", "B737"));
    flights.push_back(makeTestFlight("UAL", "KORD", "KSFO", "A320"));

    for (int i = 0; i < 5; i++) {
        mode.render(ctx, flights);
    }

    mode.teardown();
    // teardown() must reset both cycling fields to 0
    TEST_ASSERT_EQUAL(0, mode.getCurrentFlightIndex());
    TEST_ASSERT_EQUAL(0, mode.getLastCycleMs());
}

// ============================================================================
// Test: Flight cycling state behavior (logic, not pixels)
// ============================================================================

void test_single_flight_always_index_zero() {
    // With single flight, cycling logic must always set index=0 and lastCycleMs=0,
    // regardless of render call count (single-flight branch resets both fields).
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));

    for (int i = 0; i < 10; i++) {
        mode.render(ctx, flights);
    }
    TEST_ASSERT_EQUAL(0, mode.getCurrentFlightIndex());
    TEST_ASSERT_EQUAL(0, mode.getLastCycleMs());  // Single-flight branch resets timer
}

void test_cycling_zero_interval_no_rapid_strobe() {
    // displayCycleMs == 0 must NOT advance index (guard: ctx.displayCycleMs > 0).
    // Even after many render calls the index must stay at 0.
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.displayCycleMs = 0;  // zero interval
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    flights.push_back(makeTestFlight("DAL", "KATL", "KORD", "A320"));

    for (int i = 0; i < 20; i++) {
        mode.render(ctx, flights);
    }
    // Zero-interval guard must prevent any index advancement
    TEST_ASSERT_EQUAL(0, mode.getCurrentFlightIndex());
}

void test_empty_flights_then_flights_no_strobe() {
    // After empty-flights state, _lastCycleMs is reset to 0 so the timer
    // anchors fresh on first multi-flight render, preventing index strobing.
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    std::vector<FlightInfo> empty;
    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    flights.push_back(makeTestFlight("DAL", "KATL", "KORD", "A320"));

    // Empty render must reset the timer
    mode.render(ctx, empty);
    TEST_ASSERT_EQUAL(0, mode.getLastCycleMs());  // Timer reset by empty-flights path

    // First multi-flight render anchors timer; index must be 0 (no elapsed time yet)
    mode.render(ctx, flights);
    TEST_ASSERT_EQUAL(0, mode.getCurrentFlightIndex());
    TEST_ASSERT_TRUE(mode.getLastCycleMs() > 0);  // Timer now anchored at millis()

    // Second render within same tick: no cycle has elapsed, index stays 0
    mode.render(ctx, flights);
    TEST_ASSERT_EQUAL(0, mode.getCurrentFlightIndex());
}

// ============================================================================
// Test: Heap stability under mixed flight rendering (cycling state path)
// ============================================================================

void test_render_mixed_flights_heap_stable() {
    LiveFlightCardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    HeapMonitor monitor;

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    flights.push_back(makeTestFlight("DAL", "KATL", "KORD", "A320"));
    flights.push_back(makeTestFlight("AAL", "KDFW", "KLAX", "B738"));
    flights.push_back(makeNanTelemetryFlight());

    for (int i = 0; i < 100; i++) {
        mode.render(ctx, flights);
    }

    monitor.assertNoLeak(512);
}

// ============================================================================
// Test: ds-2.2 — classifyVerticalTrend (pure function, no hardware needed)
// ============================================================================

void test_classify_climbing() {
    // vrate clearly above EPS → CLIMBING
    TEST_ASSERT_EQUAL((int)VerticalTrend::CLIMBING,
                      (int)LiveFlightCardMode::classifyVerticalTrend(2.0f));
}

void test_classify_descending() {
    // vrate clearly below -EPS → DESCENDING
    TEST_ASSERT_EQUAL((int)VerticalTrend::DESCENDING,
                      (int)LiveFlightCardMode::classifyVerticalTrend(-2.0f));
}

void test_classify_level_zero() {
    // vrate == 0.0 → LEVEL (inside epsilon band)
    TEST_ASSERT_EQUAL((int)VerticalTrend::LEVEL,
                      (int)LiveFlightCardMode::classifyVerticalTrend(0.0f));
}

void test_classify_level_just_below_eps() {
    // vrate = 0.4f (< VRATE_LEVEL_EPS 0.5f) → still LEVEL
    TEST_ASSERT_EQUAL((int)VerticalTrend::LEVEL,
                      (int)LiveFlightCardMode::classifyVerticalTrend(0.4f));
}

void test_classify_unknown_nan() {
    // NAN vrate → UNKNOWN (glyph must not be drawn — AC #6)
    TEST_ASSERT_EQUAL((int)VerticalTrend::UNKNOWN,
                      (int)LiveFlightCardMode::classifyVerticalTrend(NAN));
}

// ============================================================================
// Test: ds-2.2 — computeTelemetryFields (pure function, no hardware needed)
// ============================================================================

void test_fields_two_lines_wide_all_four() {
    // 2 lines, 10 text cols: both pair threshold (>=7) and single (>=3) met → all 4 fields
    TelemetryFieldSet fs = LiveFlightCardMode::computeTelemetryFields(2, 10);
    TEST_ASSERT_TRUE(fs.alt);
    TEST_ASSERT_TRUE(fs.spd);
    TEST_ASSERT_TRUE(fs.hdg);
    TEST_ASSERT_TRUE(fs.vrate);
}

void test_fields_two_lines_narrow_singles_only() {
    // 2 lines, 5 cols: canFitPair (>=7) fails → spd and vrate dropped; alt and hdg kept
    TelemetryFieldSet fs = LiveFlightCardMode::computeTelemetryFields(2, 5);
    TEST_ASSERT_TRUE(fs.alt);
    TEST_ASSERT_FALSE(fs.spd);
    TEST_ASSERT_TRUE(fs.hdg);
    TEST_ASSERT_FALSE(fs.vrate);
}

void test_fields_two_lines_too_narrow_nothing() {
    // 2 lines, 2 cols: canFitSingle (>=3) fails → no fields at all
    TelemetryFieldSet fs = LiveFlightCardMode::computeTelemetryFields(2, 2);
    TEST_ASSERT_FALSE(fs.alt);
    TEST_ASSERT_FALSE(fs.spd);
    TEST_ASSERT_FALSE(fs.hdg);
    TEST_ASSERT_FALSE(fs.vrate);
}

void test_fields_one_line_wide_all_four() {
    // 1 line, 16 cols: all thresholds met (15=vrate, 11=hdg, 7=spd, 3=alt) → all 4 fields
    TelemetryFieldSet fs = LiveFlightCardMode::computeTelemetryFields(1, 16);
    TEST_ASSERT_TRUE(fs.alt);
    TEST_ASSERT_TRUE(fs.spd);
    TEST_ASSERT_TRUE(fs.hdg);
    TEST_ASSERT_TRUE(fs.vrate);
}

void test_fields_one_line_alt_only() {
    // 1 line, 4 cols: only alt threshold (>=3) met → alt only
    TelemetryFieldSet fs = LiveFlightCardMode::computeTelemetryFields(1, 4);
    TEST_ASSERT_TRUE(fs.alt);
    TEST_ASSERT_FALSE(fs.spd);
    TEST_ASSERT_FALSE(fs.hdg);
    TEST_ASSERT_FALSE(fs.vrate);
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
    RUN_TEST(test_cycling_zero_interval_no_rapid_strobe);
    RUN_TEST(test_empty_flights_then_flights_no_strobe);

    // Heap stability
    RUN_TEST(test_render_mixed_flights_heap_stable);

    // ds-2.2: classifyVerticalTrend
    RUN_TEST(test_classify_climbing);
    RUN_TEST(test_classify_descending);
    RUN_TEST(test_classify_level_zero);
    RUN_TEST(test_classify_level_just_below_eps);
    RUN_TEST(test_classify_unknown_nan);

    // ds-2.2: computeTelemetryFields
    RUN_TEST(test_fields_two_lines_wide_all_four);
    RUN_TEST(test_fields_two_lines_narrow_singles_only);
    RUN_TEST(test_fields_two_lines_too_narrow_nothing);
    RUN_TEST(test_fields_one_line_wide_all_four);
    RUN_TEST(test_fields_one_line_alt_only);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
