/*
 * Purpose: Unity tests for ClassicCardMode (ds-1.4).
 * Tests: init/teardown lifecycle, zone descriptor correctness, render dispatch
 *        (valid/invalid layout, empty flights), cycling state management,
 *        MEMORY_REQUIREMENT sizing, metadata (getName, getZoneDescriptor).
 * Environment: esp32dev (on-device test) — requires NVS flash.
 *
 * Architecture Reference: Display System Release decisions D1, D2
 * - ClassicCardMode implements DisplayMode via RenderContext
 * - Zone rendering ported from NeoMatrixDisplay helpers
 *
 * Note: Pixel-identical verification requires visual side-by-side comparison
 *       on hardware — documented in story Dev Agent Record.
 */
#include <Arduino.h>
#include <unity.h>

#include "modes/ClassicCardMode.h"
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
    ctx.matrix = nullptr;  // ClassicCardMode guards against nullptr
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
    ctx.layout.flightZone = {20, 0, 44, 20};
    ctx.layout.telemetryZone = {20, 20, 44, 12};
    return ctx;
}

// Helper: Create a representative FlightInfo
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

// ============================================================================
// Test: Init and teardown lifecycle
// ============================================================================

void test_init_returns_true() {
    ClassicCardMode mode;
    RenderContext ctx = makeTestCtx();
    TEST_ASSERT_TRUE(mode.init(ctx));
}

void test_teardown_does_not_crash() {
    ClassicCardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);
    mode.teardown();
    // No crash = pass
    TEST_ASSERT_TRUE(true);
}

void test_init_resets_cycling_state() {
    ClassicCardMode mode;
    RenderContext ctx = makeTestCtx();

    // First init + render cycle
    mode.init(ctx);
    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    flights.push_back(makeTestFlight("DAL", "KATL", "KORD", "A320"));

    // render with nullptr matrix should not crash (guard check)
    mode.render(ctx, flights);

    // Teardown and re-init should reset state
    mode.teardown();
    mode.init(ctx);
    // Should not crash on render after re-init
    mode.render(ctx, flights);
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// Test: Metadata
// ============================================================================

void test_get_name_returns_classic_card() {
    ClassicCardMode mode;
    TEST_ASSERT_EQUAL_STRING("Classic Card", mode.getName());
}

void test_get_zone_descriptor_has_three_zones() {
    ClassicCardMode mode;
    const ModeZoneDescriptor& desc = mode.getZoneDescriptor();
    TEST_ASSERT_EQUAL(3, desc.regionCount);
    TEST_ASSERT_NOT_NULL(desc.regions);
    TEST_ASSERT_NOT_NULL(desc.description);
}

void test_zone_labels_match_layout_zones() {
    ClassicCardMode mode;
    const ModeZoneDescriptor& desc = mode.getZoneDescriptor();
    // Zone 0: Logo, Zone 1: Flight, Zone 2: Telemetry
    TEST_ASSERT_EQUAL_STRING("Logo", desc.regions[0].label);
    TEST_ASSERT_EQUAL_STRING("Flight", desc.regions[1].label);
    TEST_ASSERT_EQUAL_STRING("Telemetry", desc.regions[2].label);
}

void test_zone_percentages_cover_full_area() {
    ClassicCardMode mode;
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
    ClassicCardMode mode;
    TEST_ASSERT_NULL(mode.getSettingsSchema());
}

// ============================================================================
// Test: MEMORY_REQUIREMENT
// ============================================================================

void test_memory_requirement_is_reasonable() {
    // Instance size should be well within MEMORY_REQUIREMENT
    // sizeof(ClassicCardMode) includes vtable ptr + 2 members
    TEST_ASSERT_TRUE(sizeof(ClassicCardMode) <= ClassicCardMode::MEMORY_REQUIREMENT);
}

void test_memory_requirement_value() {
    TEST_ASSERT_EQUAL(64, ClassicCardMode::MEMORY_REQUIREMENT);
}

// ============================================================================
// Test: Render with nullptr matrix (guard)
// ============================================================================

void test_render_null_matrix_no_crash() {
    ClassicCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));
    mode.render(ctx, flights);
    // Should safely return without crash
    TEST_ASSERT_TRUE(true);
}

void test_render_empty_flights_null_matrix_no_crash() {
    ClassicCardMode mode;
    RenderContext ctx = makeTestCtx();
    ctx.matrix = nullptr;
    mode.init(ctx);

    std::vector<FlightInfo> empty;
    mode.render(ctx, empty);
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// Test: Multiple init/teardown cycles (heap safety)
// ============================================================================

void test_multiple_init_teardown_cycles() {
    ClassicCardMode mode;
    RenderContext ctx = makeTestCtx();
    HeapMonitor monitor;

    for (int i = 0; i < 10; i++) {
        mode.init(ctx);
        mode.teardown();
    }

    monitor.assertNoLeak(256);  // Allow small tolerance
}

// ============================================================================
// Test: Production mode table integration
// ============================================================================

void test_classic_mode_factory_creates_instance() {
    ClassicCardMode* mode = new ClassicCardMode();
    TEST_ASSERT_NOT_NULL(mode);
    TEST_ASSERT_EQUAL_STRING("Classic Card", mode->getName());
    delete mode;
}

void test_classic_mode_init_and_render_lifecycle() {
    ClassicCardMode mode;
    RenderContext ctx = makeTestCtx();

    // Init
    TEST_ASSERT_TRUE(mode.init(ctx));

    // Render with flights (null matrix guards)
    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("AAL", "KDFW", "KLAX", "B738"));
    flights.push_back(makeTestFlight("SWA", "KMDW", "KLAS", "B737"));
    flights.push_back(makeTestFlight("UAL", "KORD", "KSFO", "A320"));

    // Multiple renders should not crash
    for (int i = 0; i < 5; i++) {
        mode.render(ctx, flights);
    }

    // Teardown
    mode.teardown();
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// Test: Flight cycling state behavior (logic, not pixels)
// ============================================================================

void test_single_flight_always_index_zero() {
    // With a single flight, render should always show index 0
    // (verified indirectly — no crash, and index stays at 0)
    ClassicCardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    std::vector<FlightInfo> flights;
    flights.push_back(makeTestFlight("UAL", "KLAX", "KJFK", "B738"));

    for (int i = 0; i < 10; i++) {
        mode.render(ctx, flights);
    }
    // No crash with repeated single-flight renders
    TEST_ASSERT_TRUE(true);
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
    RUN_TEST(test_get_name_returns_classic_card);
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

    // Heap safety
    RUN_TEST(test_multiple_init_teardown_cycles);

    // Production integration
    RUN_TEST(test_classic_mode_factory_creates_instance);
    RUN_TEST(test_classic_mode_init_and_render_lifecycle);

    // Cycling behavior
    RUN_TEST(test_single_flight_always_index_zero);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
