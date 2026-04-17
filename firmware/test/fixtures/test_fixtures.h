/*
 * Test Fixtures for FlightWall Display System Tests
 *
 * Purpose: Shared test data, mock factories, and helper structures for Unity tests.
 * Environment: esp32dev (PlatformIO Unity test framework)
 *
 * Usage: Include this header in test files that need mock FlightInfo, RenderContext,
 *        or other shared test infrastructure.
 */
#pragma once

#include <Arduino.h>
#include <vector>
#include <cmath>

// Forward declarations for types that may not yet exist
// These will be replaced with actual includes as components are implemented
#ifdef TEST_WITH_FLIGHT_INFO
#include "models/FlightInfo.h"
#endif

#ifdef TEST_WITH_LAYOUT_ENGINE
#include "core/LayoutEngine.h"
#endif

namespace TestFixtures {

// ============================================================================
// Flight Data Fixtures
// ============================================================================

/**
 * Creates a minimal valid FlightInfo for testing.
 * All required fields populated with realistic defaults.
 */
inline FlightInfo createMinimalFlight() {
    FlightInfo info;
    info.ident_icao = "UAL123";
    info.operator_icao = "UAL";
    info.flight_number = "UA 123";
    info.origin_code = "SFO";
    info.destination_code = "LAX";
    info.aircraft_type = "B738";
    info.altitude_kft = 35.0;
    info.speed_mph = 520.0;
    info.track_deg = 180.0;
    info.vertical_rate_fps = 0.0;
    return info;
}

/**
 * Creates a FlightInfo with telemetry data for Live Flight Card testing.
 */
inline FlightInfo createFlightWithTelemetry(
    const char* ident,
    const char* operatorIcao,
    double altKft,
    double speedMph,
    double trackDeg,
    double vrFps
) {
    FlightInfo info;
    info.ident_icao = ident;
    info.operator_icao = operatorIcao;
    info.flight_number = ident;
    info.origin_code = "SFO";
    info.destination_code = "LAX";
    info.aircraft_type = "B738";
    info.altitude_kft = altKft;
    info.speed_mph = speedMph;
    info.track_deg = trackDeg;
    info.vertical_rate_fps = vrFps;
    return info;
}

/**
 * Creates a FlightInfo with NAN telemetry values (missing data scenario).
 */
inline FlightInfo createFlightWithMissingTelemetry() {
    FlightInfo info;
    info.ident_icao = "DAL456";
    info.operator_icao = "DAL";
    info.flight_number = "DL 456";
    info.origin_code = "JFK";
    info.destination_code = "ORD";
    info.aircraft_type = "A320";
    info.altitude_kft = NAN;
    info.speed_mph = NAN;
    info.track_deg = NAN;
    info.vertical_rate_fps = NAN;
    return info;
}

/**
 * Creates a vector of N test flights for flight cycling tests.
 */
inline std::vector<FlightInfo> createFlightVector(size_t count) {
    std::vector<FlightInfo> flights;
    flights.reserve(count);

    const char* airlines[] = {"UAL", "DAL", "AAL", "SWA", "JBU"};
    const char* origins[] = {"SFO", "LAX", "JFK", "ORD", "DFW"};
    const char* dests[] = {"LAX", "SFO", "BOS", "DEN", "SEA"};

    for (size_t i = 0; i < count; i++) {
        FlightInfo info;
        info.ident_icao = String(airlines[i % 5]) + String(100 + i);
        info.operator_icao = airlines[i % 5];
        info.flight_number = String(airlines[i % 5][0]) + String(airlines[i % 5][1]) + " " + String(100 + i);
        info.origin_code = origins[i % 5];
        info.destination_code = dests[(i + 1) % 5];
        info.aircraft_type = (i % 2 == 0) ? "B738" : "A320";
        info.altitude_kft = 30.0 + (i * 2.0);
        info.speed_mph = 450.0 + (i * 10.0);
        info.track_deg = (i * 45) % 360;
        info.vertical_rate_fps = (i % 3 == 0) ? 5.0 : ((i % 3 == 1) ? -5.0 : 0.0);
        flights.push_back(info);
    }

    return flights;
}

// ============================================================================
// Layout Engine Fixtures
// ============================================================================

/**
 * Standard 10x2 @ 16px layout (160x32 matrix, "full" mode).
 * Most common test configuration matching the default hardware.
 */
inline LayoutResult createStandardLayout() {
    return LayoutEngine::compute(10, 2, 16);
}

/**
 * Compact 10x1 @ 16px layout (160x16 matrix, "compact" mode).
 */
inline LayoutResult createCompactLayout() {
    return LayoutEngine::compute(10, 1, 16);
}

/**
 * Expanded 12x3 @ 16px layout (192x48 matrix, "expanded" mode).
 */
inline LayoutResult createExpandedLayout() {
    return LayoutEngine::compute(12, 3, 16);
}

// ============================================================================
// Timing Fixtures
// ============================================================================

/**
 * Default display cycle timing in milliseconds.
 */
constexpr uint16_t DEFAULT_DISPLAY_CYCLE_MS = 5000;

/**
 * Fast cycle timing for rapid testing.
 */
constexpr uint16_t FAST_DISPLAY_CYCLE_MS = 100;

// ============================================================================
// Mock Utilities
// ============================================================================

/**
 * Simulates the passage of time for cycling tests.
 * Uses millis() internally - call between render operations.
 */
inline void advanceTimeBy(uint32_t ms) {
    // Note: On ESP32, we can't actually advance millis() in tests.
    // Use delay() for real-time testing, or refactor to inject time source.
    delay(ms);
}

/**
 * Validates that a flight index is within bounds of a flight vector.
 */
inline bool isValidFlightIndex(size_t index, const std::vector<FlightInfo>& flights) {
    return !flights.empty() && index < flights.size();
}

// ============================================================================
// Display Mode Test Fixtures (Display System Release)
// ============================================================================

/**
 * Mock RenderContext for testing modes without actual hardware.
 *
 * Note: The actual RenderContext is defined in interfaces/DisplayMode.h.
 * This mock version allows testing mode logic without GFX dependencies.
 *
 * Architecture Reference: Decision D1 - RenderContext struct
 */
#ifdef TEST_WITH_RENDER_CONTEXT

/**
 * Creates a minimal mock RenderContext for unit testing.
 * Does NOT include actual matrix pointer — only for logic testing.
 */
struct MockRenderContext {
    LayoutResult layout;
    uint16_t textColor;
    uint8_t brightness;
    uint16_t* logoBuffer;
    uint32_t displayCycleMs;

    // Flag to track if render was called
    mutable bool renderCalled;
    mutable int renderCallCount;
};

inline MockRenderContext createMockRenderContext(
    uint8_t tilesX = 10,
    uint8_t tilesY = 2,
    uint8_t tilePixels = 16
) {
    MockRenderContext ctx;
    ctx.layout = LayoutEngine::compute(tilesX, tilesY, tilePixels);
    ctx.textColor = 0xFFFF;  // White in RGB565
    ctx.brightness = 40;
    ctx.logoBuffer = nullptr;  // Set by test if needed
    ctx.displayCycleMs = DEFAULT_DISPLAY_CYCLE_MS;
    ctx.renderCalled = false;
    ctx.renderCallCount = 0;
    return ctx;
}

#endif // TEST_WITH_RENDER_CONTEXT

// ============================================================================
// Mode Zone Descriptor Fixtures (for Mode Picker UI testing)
// ============================================================================

/**
 * Zone region test data matching architecture spec.
 * Used to verify mode metadata serialization for API.
 */
struct TestZoneRegion {
    const char* label;
    uint8_t relX, relY;
    uint8_t relW, relH;
};

/**
 * Classic Card mode expected zones (per architecture D1).
 */
inline std::vector<TestZoneRegion> getClassicCardZones() {
    return {
        {"Logo",     0,  0, 20, 100},
        {"Airline", 20,  0, 80,  33},
        {"Route",   20, 33, 80,  33},
        {"Aircraft",20, 66, 80,  34}
    };
}

/**
 * Live Flight Card mode expected zones (per architecture D1).
 */
inline std::vector<TestZoneRegion> getLiveFlightCardZones() {
    return {
        {"Logo",    0,  0, 20, 100},
        {"Airline",20,  0, 40,  50},
        {"Route",  60,  0, 40,  50},
        {"Alt",    20, 50, 20,  50},
        {"Spd",    40, 50, 20,  50},
        {"Hdg",    60, 50, 20,  50},
        {"VRate",  80, 50, 20,  50}
    };
}

// ============================================================================
// Switch State Test Fixtures (for ModeRegistry testing)
// ============================================================================

/**
 * Expected switch state sequence for a successful mode switch.
 * Per architecture D2: IDLE -> REQUESTED -> SWITCHING -> IDLE
 */
inline std::vector<uint8_t> getExpectedSwitchSequence() {
    return {
        0,  // IDLE (start)
        1,  // REQUESTED (after requestSwitch)
        2,  // SWITCHING (during tick)
        0   // IDLE (after completion)
    };
}

// ============================================================================
// Memory Requirement Test Constants
// ============================================================================

/**
 * Expected memory requirements per architecture.
 * ClassicCardMode: ~64 bytes (cycling state only)
 * LiveFlightCardMode: ~64 bytes (cycling state only)
 */
constexpr uint32_t CLASSIC_CARD_MEMORY_REQ = 64;
constexpr uint32_t LIVE_FLIGHT_MEMORY_REQ = 64;

/**
 * Heap floor requirement per NFR S4.
 */
constexpr uint32_t HEAP_FLOOR_BYTES = 30 * 1024;  // 30KB

/**
 * NVS debounce delay per architecture D2.
 */
constexpr uint32_t NVS_DEBOUNCE_MS = 2000;

} // namespace TestFixtures
