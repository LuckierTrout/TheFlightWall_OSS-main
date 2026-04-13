/*
Purpose: Unity tests for ModeOrchestrator (Story dl-1.5).
Tests: State transitions, reason strings, manual switching, idle fallback, restore.
Environment: esp32dev (on-device test).
*/
#include <Arduino.h>
#include <unity.h>
#include "core/ModeOrchestrator.h"

// --- Initial State Tests ---

void test_init_state_is_manual() {
    ModeOrchestrator::init();
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
}

void test_init_active_mode_is_classic_card() {
    ModeOrchestrator::init();
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("Classic Card", ModeOrchestrator::getActiveModeName());
}

void test_init_state_string_is_manual() {
    ModeOrchestrator::init();
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateString());
}

void test_init_state_reason_is_manual() {
    ModeOrchestrator::init();
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateReason());
}

// --- Manual Switch Tests ---

void test_manual_switch_changes_active_mode() {
    ModeOrchestrator::init();
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("Live Flight Card", ModeOrchestrator::getActiveModeName());
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
}

void test_manual_switch_state_reason_stays_manual() {
    ModeOrchestrator::init();
    ModeOrchestrator::onManualSwitch("clock", "Clock");
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateReason());
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateString());
}

// --- Idle Fallback Tests ---

void test_idle_fallback_on_zero_flights() {
    ModeOrchestrator::init();
    ModeOrchestrator::tick(0);  // zero flights
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("Clock", ModeOrchestrator::getActiveModeName());
}

void test_idle_fallback_state_strings() {
    ModeOrchestrator::init();
    ModeOrchestrator::tick(0);
    TEST_ASSERT_EQUAL_STRING("idle_fallback", ModeOrchestrator::getStateString());
    // Reason contains em-dash UTF-8
    const char* reason = ModeOrchestrator::getStateReason();
    TEST_ASSERT_NOT_NULL(reason);
    TEST_ASSERT_TRUE(strstr(reason, "idle fallback") != nullptr);
    TEST_ASSERT_TRUE(strstr(reason, "no flights") != nullptr);
}

void test_idle_fallback_no_double_trigger() {
    ModeOrchestrator::init();
    ModeOrchestrator::tick(0);  // enters IDLE_FALLBACK
    ModeOrchestrator::tick(0);  // should stay in IDLE_FALLBACK (no-op)
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
}

// --- Flights Restored Tests ---

void test_flights_restored_returns_to_manual() {
    ModeOrchestrator::init();
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
    ModeOrchestrator::init();
    ModeOrchestrator::tick(0);  // idle fallback
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("manual", ModeOrchestrator::getStateReason());
}

// --- Tick with flights does not change MANUAL state ---

void test_tick_with_flights_stays_manual() {
    ModeOrchestrator::init();
    ModeOrchestrator::tick(5);  // flights present, already MANUAL
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getActiveModeId());
}

void setup() {
    delay(2000);
    UNITY_BEGIN();

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

    UNITY_END();
}

void loop() {}
