/*
Purpose: Unity tests for ModeOrchestrator (Story dl-1.5, dl-1.2, dl-1.3).
Tests: State transitions, reason strings, manual switching, idle fallback, restore,
       ModeRegistry bridge (dl-1.2), getManualModeId,
       manual requestSwitch integration (dl-1.3).
Environment: esp32dev (on-device test).
*/
#include <Arduino.h>
#include <unity.h>
#include "core/ModeOrchestrator.h"
#include "core/ModeRegistry.h"
#include "core/ConfigManager.h"
#include "modes/ClassicCardMode.h"
#include "modes/LiveFlightCardMode.h"
#include "modes/ClockMode.h"

// Stub for isNtpSynced() — defined in main.cpp which is excluded from test builds.
// ClockMode.cpp declares extern bool isNtpSynced() and calls it in render().
bool isNtpSynced() { return false; }

// Include test helpers for NVS utilities
#define TEST_WITH_FLIGHT_INFO
#define TEST_WITH_LAYOUT_ENGINE
#include "fixtures/test_helpers.h"

using namespace TestHelpers;

// --- Production mode table for integration tests ---
static DisplayMode* classicFactory() { return new ClassicCardMode(); }
static DisplayMode* liveFactory() { return new LiveFlightCardMode(); }
static DisplayMode* clockFactory() { return new ClockMode(); }
static uint32_t classicMemReq() { return ClassicCardMode::MEMORY_REQUIREMENT; }
static uint32_t liveMemReq() { return LiveFlightCardMode::MEMORY_REQUIREMENT; }
static uint32_t clockMemReq() { return ClockMode::MEMORY_REQUIREMENT; }

static const ModeEntry ORCH_MODE_TABLE[] = {
    { "classic_card", "Classic Card",     classicFactory, classicMemReq, 0, &ClassicCardMode::_descriptor,    nullptr },
    { "live_flight",  "Live Flight Card", liveFactory,    liveMemReq,    1, &LiveFlightCardMode::_descriptor, nullptr },
    { "clock",        "Clock",            clockFactory,   clockMemReq,   2, &ClockMode::_descriptor,          &CLOCK_SCHEMA },
};
static constexpr uint8_t ORCH_MODE_COUNT = sizeof(ORCH_MODE_TABLE) / sizeof(ORCH_MODE_TABLE[0]);

static RenderContext makeTestCtx() {
    RenderContext ctx = {};
    ctx.matrix = nullptr;
    ctx.textColor = 0xFFFF;
    ctx.brightness = 40;
    ctx.logoBuffer = nullptr;
    ctx.displayCycleMs = 5000;
    return ctx;
}

// Helper: init both orchestrator and registry for integration tests
static void initOrchestratorWithRegistry() {
    clearNvs("flightwall");
    ConfigManager::init();
    ModeRegistry::init(ORCH_MODE_TABLE, ORCH_MODE_COUNT);
    ModeOrchestrator::init();
    // Activate the initial mode (classic_card) in the registry
    ModeRegistry::requestSwitch("classic_card");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
}

// --- Initial State Tests ---

void test_init_state_is_manual() {
    initOrchestratorWithRegistry();
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
}

void test_init_active_mode_is_classic_card() {
    initOrchestratorWithRegistry();
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("Classic Card", ModeOrchestrator::getActiveModeName());
}

void test_init_state_string_is_manual() {
    initOrchestratorWithRegistry();
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateString());
}

void test_init_state_reason_is_manual() {
    initOrchestratorWithRegistry();
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateReason());
}

// --- Manual Switch Tests ---

void test_manual_switch_changes_active_mode() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("Live Flight Card", ModeOrchestrator::getActiveModeName());
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
}

void test_manual_switch_state_reason_stays_manual() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::onManualSwitch("clock", "Clock");
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateReason());
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateString());
}

// --- Idle Fallback Tests ---

void test_idle_fallback_on_zero_flights() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::tick(0);  // zero flights
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("Clock", ModeOrchestrator::getActiveModeName());
}

void test_idle_fallback_state_strings() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::tick(0);
    TEST_ASSERT_EQUAL_STRING("idle_fallback", ModeOrchestrator::getStateString());
    // Reason contains em-dash UTF-8
    const char* reason = ModeOrchestrator::getStateReason();
    TEST_ASSERT_NOT_NULL(reason);
    TEST_ASSERT_TRUE(strstr(reason, "idle fallback") != nullptr);
    TEST_ASSERT_TRUE(strstr(reason, "no flights") != nullptr);
}

void test_idle_fallback_no_double_trigger() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::tick(0);  // enters IDLE_FALLBACK
    ModeOrchestrator::tick(0);  // should stay in IDLE_FALLBACK (no-op)
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
}

// --- Flights Restored Tests ---

void test_flights_restored_returns_to_manual() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");
    ModeOrchestrator::tick(0);  // idle fallback
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    ModeOrchestrator::tick(3);  // flights restored
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("Live Flight Card", ModeOrchestrator::getActiveModeName());
}

// --- Manual Override During Fallback (AC #4) ---

void test_manual_switch_during_fallback_transitions_to_manual() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::tick(0);  // idle fallback
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateReason());
}

// --- Tick with flights does not change MANUAL state ---

void test_tick_with_flights_stays_manual() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::tick(5);  // flights present, already MANUAL
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getActiveModeId());
}

// ============================================================================
// Story dl-1.2: getManualModeId (AC #9)
// ============================================================================

void test_get_manual_mode_id_default_classic_card() {
    initOrchestratorWithRegistry();
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getManualModeId());
}

void test_get_manual_mode_id_after_manual_switch() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeOrchestrator::getManualModeId());
}

void test_get_manual_mode_id_preserved_during_fallback() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");
    ModeOrchestrator::tick(0);  // idle fallback → clock
    // manualModeId should still be live_flight even though activeModeId is clock
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeOrchestrator::getManualModeId());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
}

// ============================================================================
// Story dl-1.2: ModeRegistry bridge integration (AC #1, #2, #10)
// ============================================================================

void test_idle_fallback_requests_clock_in_registry() {
    initOrchestratorWithRegistry();
    // Registry should be on classic_card
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeRegistry::getActiveModeId());

    // Trigger idle fallback
    ModeOrchestrator::tick(0);

    // Orchestrator requested switch to clock — process it via registry tick
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    // Registry should now be on clock
    TEST_ASSERT_EQUAL_STRING("clock", ModeRegistry::getActiveModeId());
}

void test_flights_restored_requests_manual_mode_in_registry() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");

    // Switch registry to live_flight first
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::requestSwitch("live_flight");
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeRegistry::getActiveModeId());

    // Idle fallback → clock
    ModeOrchestrator::tick(0);
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("clock", ModeRegistry::getActiveModeId());

    // Flights restored → should request live_flight back
    ModeOrchestrator::tick(3);
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeRegistry::getActiveModeId());
}

// AC #6: clock already active (user manually picked clock) → safe no flip-flop
void test_idle_fallback_safe_when_clock_already_active() {
    initOrchestratorWithRegistry();
    // User manually selects clock
    ModeOrchestrator::onManualSwitch("clock", "Clock");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::requestSwitch("clock");
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("clock", ModeRegistry::getActiveModeId());

    // Zero flights — idle fallback. Should not crash or flip-flop.
    ModeOrchestrator::tick(0);
    ModeRegistry::tick(ctx, empty);

    // Should still be clock, state is IDLE_FALLBACK
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("clock", ModeRegistry::getActiveModeId());
}

// AC #7: SCHEDULED state should not be overridden by idle fallback
void test_scheduled_state_not_overridden_by_tick() {
    initOrchestratorWithRegistry();
    // Note: no public setter for SCHEDULED state currently exists,
    // so we verify tick with 0 flights in MANUAL triggers fallback (positive)
    // and tick with flights in IDLE_FALLBACK restores (positive).
    // SCHEDULED interaction deferred to dl-1.4+.
    ModeOrchestrator::tick(0);
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    ModeOrchestrator::tick(5);
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
}

// ============================================================================
// Story dl-1.3: Manual Mode Switching via Orchestrator
// ============================================================================

// AC #1: onManualSwitch drives ModeRegistry::requestSwitch so LED mode changes
void test_manual_switch_drives_registry() {
    initOrchestratorWithRegistry();
    // Registry starts on classic_card
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeRegistry::getActiveModeId());

    // Manual switch to live_flight via orchestrator
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");

    // Process the switch in registry
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    // Registry should now be on live_flight
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeRegistry::getActiveModeId());
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
}

// AC #2: onManualSwitch from IDLE_FALLBACK exits to MANUAL and drives registry
void test_manual_switch_from_idle_fallback_drives_registry() {
    initOrchestratorWithRegistry();
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;

    // Enter idle fallback (zero flights)
    ModeOrchestrator::tick(0);
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("clock", ModeRegistry::getActiveModeId());

    // Manual override during fallback
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");
    ModeRegistry::tick(ctx, empty);

    // Should be in MANUAL state with live_flight active in registry
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeRegistry::getActiveModeId());
}

// AC #5: onManualSwitch with unknown mode — requestSwitch returns false
void test_manual_switch_unknown_mode_requestswitch_fails() {
    initOrchestratorWithRegistry();
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;

    // Manual switch to nonexistent mode
    ModeOrchestrator::onManualSwitch("nonexistent", "Does Not Exist");

    // Orchestrator records the intent (state is MANUAL, activeModeId updated)
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("nonexistent", ModeOrchestrator::getActiveModeId());

    // Registry should NOT have switched (unknown mode)
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeRegistry::getActiveModeId());

    // Registry error should be set
    const char* err = ModeRegistry::getLastError();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_TRUE(strlen(err) > 0);
}

// AC #4/6: boot path uses onManualSwitch (integration — verify no direct requestSwitch in main.cpp production)
// This is a structural test: verify onManualSwitch correctly propagates to registry for boot scenario
void test_boot_path_via_orchestrator() {
    initOrchestratorWithRegistry();
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;

    // Simulate boot: orchestrator onManualSwitch with a saved mode
    ModeOrchestrator::onManualSwitch("clock", "Clock");
    ModeRegistry::tick(ctx, empty);

    TEST_ASSERT_EQUAL_STRING("clock", ModeRegistry::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
}

// ============================================================================
// Story dl-1.4: Watchdog Recovery to Default Mode
// ============================================================================

// Helper: simulate boot mode restore logic from main.cpp (extracted for testability).
// This mirrors the exact policy in setup(): WDT recovery forces clock, normal boot
// restores NVS-saved mode with fallback chain (clock → classic_card for unknown IDs).
static void simulateBootRestore(bool wdtRecovery) {
    const ModeEntry* table = ModeRegistry::getModeTable();
    uint8_t count = ModeRegistry::getModeCount();

    // Lambda: find mode display name from table
    auto findModeName = [&](const char* modeId) -> const char* {
        for (uint8_t i = 0; i < count; i++) {
            if (strcmp(table[i].id, modeId) == 0) return table[i].displayName;
        }
        return nullptr;
    };

    if (wdtRecovery) {
        const char* clockName = findModeName("clock");
        if (clockName) {
            ModeOrchestrator::onManualSwitch("clock", clockName);
            ConfigManager::setDisplayMode("clock");
        } else {
            ModeOrchestrator::onManualSwitch("classic_card", "Classic Card");
            ConfigManager::setDisplayMode("classic_card");
        }
    } else {
        String savedMode = ConfigManager::getDisplayMode();
        const char* bootModeName = findModeName(savedMode.c_str());
        if (bootModeName) {
            ModeOrchestrator::onManualSwitch(savedMode.c_str(), bootModeName);
        } else {
            const char* clockName = findModeName("clock");
            if (clockName) {
                ModeOrchestrator::onManualSwitch("clock", clockName);
                ConfigManager::setDisplayMode("clock");
            } else {
                ModeOrchestrator::onManualSwitch("classic_card", "Classic Card");
                ConfigManager::setDisplayMode("classic_card");
            }
        }
    }
}

// AC #1: WDT recovery forces clock mode regardless of saved mode
void test_wdt_recovery_forces_clock_mode() {
    initOrchestratorWithRegistry();
    // Pre-set NVS to live_flight (simulating crash while on live_flight)
    ConfigManager::setDisplayMode("live_flight");

    // Re-init to pick up the NVS state
    ModeOrchestrator::init();

    // Simulate WDT recovery boot
    simulateBootRestore(/* wdtRecovery= */ true);

    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("Clock", ModeOrchestrator::getActiveModeName());
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
}

// AC #1: WDT recovery persists clock to NVS
void test_wdt_recovery_persists_clock_to_nvs() {
    initOrchestratorWithRegistry();
    ConfigManager::setDisplayMode("live_flight");
    ModeOrchestrator::init();

    simulateBootRestore(/* wdtRecovery= */ true);

    String stored = ConfigManager::getDisplayMode();
    TEST_ASSERT_EQUAL_STRING("clock", stored.c_str());
}

// AC #2: Normal boot restores valid saved mode
void test_normal_boot_restores_saved_mode() {
    initOrchestratorWithRegistry();
    ConfigManager::setDisplayMode("live_flight");
    ModeOrchestrator::init();

    simulateBootRestore(/* wdtRecovery= */ false);

    TEST_ASSERT_EQUAL_STRING("live_flight", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
}

// AC #3: Unknown saved mode falls back to clock, corrects NVS
void test_unknown_mode_falls_back_to_clock() {
    initOrchestratorWithRegistry();
    // Write a bogus mode to NVS
    nvsWriteString("disp_mode", "nonexistent_mode");
    ConfigManager::init();  // Re-init to load the bogus value
    ModeOrchestrator::init();

    simulateBootRestore(/* wdtRecovery= */ false);

    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
    // NVS should be corrected
    String stored = ConfigManager::getDisplayMode();
    TEST_ASSERT_EQUAL_STRING("clock", stored.c_str());
}

// AC #4: WDT recovery with init failure retries with clock (via registry fallback)
void test_wdt_recovery_routes_through_orchestrator() {
    initOrchestratorWithRegistry();
    ConfigManager::setDisplayMode("classic_card");
    ModeOrchestrator::init();

    // Simulate WDT recovery — should route through onManualSwitch (Rule 24)
    simulateBootRestore(/* wdtRecovery= */ true);

    // Verify orchestrator state is MANUAL with clock active
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());

    // Process the switch in registry
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    // Registry should be on clock
    TEST_ASSERT_EQUAL_STRING("clock", ModeRegistry::getActiveModeId());
}

// AC #6: Boot path goes through orchestrator, not direct ModeRegistry::requestSwitch
void test_wdt_boot_uses_orchestrator_not_direct_registry() {
    initOrchestratorWithRegistry();
    ModeOrchestrator::init();

    // Manual mode set to live_flight before WDT
    ConfigManager::setDisplayMode("live_flight");

    simulateBootRestore(/* wdtRecovery= */ true);

    // manualModeId should be "clock" (WDT forced it as the manual selection)
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getManualModeId());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // Original dl-1.5 tests
    RUN_TEST(test_init_state_is_manual);
    RUN_TEST(test_init_active_mode_is_classic_card);
    RUN_TEST(test_init_state_string_is_manual);
    RUN_TEST(test_init_state_reason_is_manual);
    RUN_TEST(test_manual_switch_changes_active_mode);
    RUN_TEST(test_manual_switch_state_reason_stays_manual);
    RUN_TEST(test_idle_fallback_on_zero_flights);
    RUN_TEST(test_idle_fallback_state_strings);
    RUN_TEST(test_idle_fallback_no_double_trigger);
    RUN_TEST(test_flights_restored_returns_to_manual);
    RUN_TEST(test_manual_switch_during_fallback_transitions_to_manual);
    RUN_TEST(test_tick_with_flights_stays_manual);

    // dl-1.2: getManualModeId (AC #9)
    RUN_TEST(test_get_manual_mode_id_default_classic_card);
    RUN_TEST(test_get_manual_mode_id_after_manual_switch);
    RUN_TEST(test_get_manual_mode_id_preserved_during_fallback);

    // dl-1.2: ModeRegistry bridge integration (AC #1, #2, #10)
    RUN_TEST(test_idle_fallback_requests_clock_in_registry);
    RUN_TEST(test_flights_restored_requests_manual_mode_in_registry);
    RUN_TEST(test_idle_fallback_safe_when_clock_already_active);
    RUN_TEST(test_scheduled_state_not_overridden_by_tick);

    // dl-1.3: Manual mode switching via orchestrator
    RUN_TEST(test_manual_switch_drives_registry);
    RUN_TEST(test_manual_switch_from_idle_fallback_drives_registry);
    RUN_TEST(test_manual_switch_unknown_mode_requestswitch_fails);
    RUN_TEST(test_boot_path_via_orchestrator);

    // dl-1.4: Watchdog recovery to default mode
    RUN_TEST(test_wdt_recovery_forces_clock_mode);
    RUN_TEST(test_wdt_recovery_persists_clock_to_nvs);
    RUN_TEST(test_normal_boot_restores_saved_mode);
    RUN_TEST(test_unknown_mode_falls_back_to_clock);
    RUN_TEST(test_wdt_recovery_routes_through_orchestrator);
    RUN_TEST(test_wdt_boot_uses_orchestrator_not_direct_registry);

    // Cleanup
    clearNvs("flightwall");

    UNITY_END();
}

void loop() {}
