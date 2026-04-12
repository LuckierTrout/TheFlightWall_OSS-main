/*
 * Purpose: Unity tests for ModeRegistry.
 * Tests: Switch lifecycle, heap guard, NVS debounce, error handling,
 *        state transitions, rapid-switch safety.
 * Environment: esp32dev (on-device test) — requires NVS flash.
 *
 * Architecture Reference: Display System Release decisions D2, D4
 * - ModeRegistry static table with cooperative switch serialization
 * - Display task integration via ModeRegistry::tick()
 *
 * Note: These tests require the ModeRegistry and DisplayMode interfaces to be
 * implemented. The skeleton is provided per Story 3.5 for test-first development.
 */
#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>

// Include test helpers and fixtures
#include "fixtures/test_helpers.h"
#include "fixtures/test_fixtures.h"

// Uncomment when ModeRegistry is implemented:
// #include "core/ModeRegistry.h"
// #include "interfaces/DisplayMode.h"

using namespace TestHelpers;

// ============================================================================
// Mock Mode Implementation (for testing)
// ============================================================================

/**
 * Mock DisplayMode that tracks lifecycle calls.
 * Used to verify ModeRegistry properly calls init/render/teardown.
 */
struct MockModeStats {
    int initCalls = 0;
    int renderCalls = 0;
    int teardownCalls = 0;
    bool initShouldFail = false;

    void reset() {
        initCalls = 0;
        renderCalls = 0;
        teardownCalls = 0;
        initShouldFail = false;
    }
};

// Global mock stats accessible by test mock implementations
static MockModeStats g_mockModeAStats;
static MockModeStats g_mockModeBStats;

/*
 * NOTE: Actual mock mode classes will be added when DisplayMode interface exists.
 * They will implement:
 * - bool init(const RenderContext& ctx)
 * - void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights)
 * - void teardown()
 * - const char* getName()
 * - const ModeZoneDescriptor& getZoneDescriptor()
 */

// ============================================================================
// Helper: Clear NVS for test isolation
// ============================================================================

static void clearModeNvs() {
    clearNvs("flightwall");
}

// ============================================================================
// Test: Registry initialization
// ============================================================================

void test_registry_init_sets_default_mode() {
    // Verify ModeRegistry::init() with a mode table sets the first mode as active
    // Per architecture: "classic_card" is the default mode at index 0
    //
    // TEST_ASSERT_NOT_NULL(ModeRegistry::getActiveMode());
    // TEST_ASSERT_EQUAL_STRING("classic_card", ModeRegistry::getActiveModeId());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

void test_registry_init_restores_from_nvs() {
    // Set display_mode in NVS, then verify init restores it
    // nvsWriteString("display_mode", "live_flight");
    // ModeRegistry::init(MODE_TABLE, MODE_COUNT);
    // TEST_ASSERT_EQUAL_STRING("live_flight", ModeRegistry::getActiveModeId());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

void test_registry_init_defaults_if_nvs_invalid() {
    // Set invalid mode ID in NVS, verify fallback to default
    // nvsWriteString("display_mode", "nonexistent_mode");
    // ModeRegistry::init(MODE_TABLE, MODE_COUNT);
    // TEST_ASSERT_EQUAL_STRING("classic_card", ModeRegistry::getActiveModeId());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

// ============================================================================
// Test: Switch lifecycle
// ============================================================================

void test_switch_calls_teardown_then_init() {
    // Verify switch sequence: teardown old -> heap check -> init new
    // g_mockModeAStats.reset();
    // g_mockModeBStats.reset();
    // ModeRegistry::requestSwitch("mock_mode_b");
    // ModeRegistry::tick(ctx, flights);  // Execute switch
    // TEST_ASSERT_EQUAL(1, g_mockModeAStats.teardownCalls);
    // TEST_ASSERT_EQUAL(1, g_mockModeBStats.initCalls);
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

void test_switch_sets_state_to_switching() {
    // Verify _switchState = SWITCHING during transition
    // ModeRegistry::requestSwitch("mock_mode_b");
    // TEST_ASSERT_EQUAL(SwitchState::REQUESTED, ModeRegistry::getSwitchState());
    // // Inside tick(), state should transition to SWITCHING then IDLE
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

void test_switch_completes_to_idle_state() {
    // After tick() processes switch, state should be IDLE
    // ModeRegistry::requestSwitch("mock_mode_b");
    // ModeRegistry::tick(ctx, flights);
    // TEST_ASSERT_EQUAL(SwitchState::IDLE, ModeRegistry::getSwitchState());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

// ============================================================================
// Test: Heap guard
// ============================================================================

void test_heap_guard_rejects_insufficient_memory() {
    // Mock a mode with MEMORY_REQUIREMENT > available heap
    // Should reject switch and restore previous mode
    // TEST_ASSERT_NOT_NULL(ModeRegistry::getActiveMode());
    // TEST_ASSERT_STRING_CONTAINS("Insufficient memory", ModeRegistry::getLastError());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

void test_heap_guard_allows_sufficient_memory() {
    // Normal case: enough heap available
    // HeapMonitor monitor;
    // ModeRegistry::requestSwitch("mock_mode_b");
    // ModeRegistry::tick(ctx, flights);
    // TEST_ASSERT_EQUAL_STRING("mock_mode_b", ModeRegistry::getActiveModeId());
    // TEST_ASSERT_TRUE(String(ModeRegistry::getLastError()).length() == 0);
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

// ============================================================================
// Test: Mode init failure handling
// ============================================================================

void test_init_failure_restores_previous_mode() {
    // If new mode's init() returns false, restore previous mode
    // g_mockModeBStats.initShouldFail = true;
    // ModeRegistry::requestSwitch("mock_mode_b");
    // ModeRegistry::tick(ctx, flights);
    // TEST_ASSERT_EQUAL_STRING("classic_card", ModeRegistry::getActiveModeId());
    // TEST_ASSERT_STRING_CONTAINS("initialization failed", ModeRegistry::getLastError());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

// ============================================================================
// Test: NVS debounce
// ============================================================================

void test_nvs_not_written_immediately_after_switch() {
    // NVS should NOT contain new mode immediately after switch
    // ModeRegistry::requestSwitch("mock_mode_b");
    // ModeRegistry::tick(ctx, flights);
    // String stored = nvsReadString("display_mode", "");
    // TEST_ASSERT_FALSE(stored == "mock_mode_b");  // Not persisted yet
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

void test_nvs_written_after_debounce_delay() {
    // After 2+ seconds and another tick(), NVS should be updated
    // ModeRegistry::requestSwitch("mock_mode_b");
    // ModeRegistry::tick(ctx, flights);
    // delay(2100);  // Wait for debounce
    // ModeRegistry::tick(ctx, flights);
    // String stored = nvsReadString("display_mode", "");
    // TEST_ASSERT_EQUAL_STRING("mock_mode_b", stored.c_str());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

void test_rapid_switches_debounce_to_single_nvs_write() {
    // Switch 3 times rapidly — should result in 1 NVS write at the end
    // HeapMonitor monitor;
    // ModeRegistry::requestSwitch("mock_mode_b");
    // ModeRegistry::tick(ctx, flights);
    // ModeRegistry::requestSwitch("classic_card");
    // ModeRegistry::tick(ctx, flights);
    // ModeRegistry::requestSwitch("mock_mode_b");
    // ModeRegistry::tick(ctx, flights);
    // delay(2100);
    // ModeRegistry::tick(ctx, flights);
    // // Verify final mode is persisted, not intermediate states
    // String stored = nvsReadString("display_mode", "");
    // TEST_ASSERT_EQUAL_STRING("mock_mode_b", stored.c_str());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

// ============================================================================
// Test: Mode enumeration (for API)
// ============================================================================

void test_get_mode_table_returns_all_modes() {
    // Verify getModeTable() returns the static table
    // const ModeEntry* table = ModeRegistry::getModeTable();
    // TEST_ASSERT_NOT_NULL(table);
    // TEST_ASSERT_EQUAL(MODE_COUNT, ModeRegistry::getModeCount());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

void test_mode_table_has_required_entries() {
    // Verify expected modes exist: classic_card, live_flight
    // const ModeEntry* table = ModeRegistry::getModeTable();
    // bool hasClassic = false;
    // bool hasLiveFlight = false;
    // for (uint8_t i = 0; i < ModeRegistry::getModeCount(); i++) {
    //     if (strcmp(table[i].id, "classic_card") == 0) hasClassic = true;
    //     if (strcmp(table[i].id, "live_flight") == 0) hasLiveFlight = true;
    // }
    // TEST_ASSERT_TRUE(hasClassic);
    // TEST_ASSERT_TRUE(hasLiveFlight);
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

// ============================================================================
// Test: Rapid-switch safety (NFR S2)
// ============================================================================

void test_switch_rejected_while_switching() {
    // If a switch is already in progress, new requests should be queued/ignored
    // Start a switch but don't tick() yet
    // ModeRegistry::requestSwitch("mock_mode_b");
    // bool accepted = ModeRegistry::requestSwitch("classic_card");
    // TEST_ASSERT_FALSE(accepted);  // Should reject while SWITCHING
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

void test_ten_rapid_switches_no_crash() {
    // NFR S2: 10 toggles in 3 seconds should not crash
    // HeapMonitor monitor;
    // for (int i = 0; i < 10; i++) {
    //     ModeRegistry::requestSwitch(i % 2 == 0 ? "mock_mode_b" : "classic_card");
    //     ModeRegistry::tick(ctx, flights);
    // }
    // TEST_ASSERT_NOT_NULL(ModeRegistry::getActiveMode());
    // monitor.assertNoLeak(1024);  // Allow 1KB tolerance for mode allocations
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

// ============================================================================
// Test: Unknown mode handling
// ============================================================================

void test_unknown_mode_returns_false() {
    // requestSwitch() with unknown mode ID should return false
    // bool result = ModeRegistry::requestSwitch("nonexistent_mode");
    // TEST_ASSERT_FALSE(result);
    // TEST_ASSERT_STRING_CONTAINS("Unknown mode", ModeRegistry::getLastError());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

// ============================================================================
// Test: prepareForOTA (Foundation+Delight integration)
// ============================================================================

void test_prepare_for_ota_tears_down_active_mode() {
    // prepareForOTA() should teardown active mode and set state to SWITCHING
    // ModeRegistry::prepareForOTA();
    // TEST_ASSERT_NULL(ModeRegistry::getActiveMode());
    // TEST_ASSERT_EQUAL(SwitchState::SWITCHING, ModeRegistry::getSwitchState());
    TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented");
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000);

    UNITY_BEGIN();

    // Registry initialization
    RUN_TEST(test_registry_init_sets_default_mode);
    RUN_TEST(test_registry_init_restores_from_nvs);
    RUN_TEST(test_registry_init_defaults_if_nvs_invalid);

    // Switch lifecycle
    RUN_TEST(test_switch_calls_teardown_then_init);
    RUN_TEST(test_switch_sets_state_to_switching);
    RUN_TEST(test_switch_completes_to_idle_state);

    // Heap guard
    RUN_TEST(test_heap_guard_rejects_insufficient_memory);
    RUN_TEST(test_heap_guard_allows_sufficient_memory);

    // Init failure handling
    RUN_TEST(test_init_failure_restores_previous_mode);

    // NVS debounce
    RUN_TEST(test_nvs_not_written_immediately_after_switch);
    RUN_TEST(test_nvs_written_after_debounce_delay);
    RUN_TEST(test_rapid_switches_debounce_to_single_nvs_write);

    // Mode enumeration
    RUN_TEST(test_get_mode_table_returns_all_modes);
    RUN_TEST(test_mode_table_has_required_entries);

    // Rapid-switch safety
    RUN_TEST(test_switch_rejected_while_switching);
    RUN_TEST(test_ten_rapid_switches_no_crash);

    // Unknown mode handling
    RUN_TEST(test_unknown_mode_returns_false);

    // OTA integration
    RUN_TEST(test_prepare_for_ota_tears_down_active_mode);

    // Cleanup
    clearModeNvs();

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
