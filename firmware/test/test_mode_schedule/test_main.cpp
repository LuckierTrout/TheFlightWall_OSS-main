/*
Purpose: Unity tests for mode schedule system (Story dl-4.1, AC #11).
Tests: minutesInWindow helper, ConfigManager ModeScheduleConfig NVS round-trip,
       ModeOrchestrator schedule evaluation, SCHEDULED + zero flights no fallback,
       rule priority (first enabled match wins).
Environment: esp32dev (on-device test) — requires NVS flash.
*/
#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include "utils/TimeUtils.h"
#include "core/ConfigManager.h"
#include "core/ModeOrchestrator.h"
#include "core/ModeRegistry.h"
#include "modes/ClassicCardMode.h"
#include "modes/LiveFlightCardMode.h"
#include "modes/ClockMode.h"
#include "modes/DeparturesBoardMode.h"

// Stub for isNtpSynced() — defined in main.cpp which is excluded from test builds.
bool isNtpSynced() { return false; }

#define TEST_WITH_FLIGHT_INFO
#define TEST_WITH_LAYOUT_ENGINE
#include "fixtures/test_helpers.h"

using namespace TestHelpers;

// --- Mode table for integration tests ---
static DisplayMode* classicFactory() { return new ClassicCardMode(); }
static DisplayMode* liveFactory() { return new LiveFlightCardMode(); }
static DisplayMode* clockFactory() { return new ClockMode(); }
static DisplayMode* depBdFactory() { return new DeparturesBoardMode(); }
static uint32_t classicMemReq() { return ClassicCardMode::MEMORY_REQUIREMENT; }
static uint32_t liveMemReq() { return LiveFlightCardMode::MEMORY_REQUIREMENT; }
static uint32_t clockMemReq() { return ClockMode::MEMORY_REQUIREMENT; }
static uint32_t depBdMemReq() { return DeparturesBoardMode::MEMORY_REQUIREMENT; }

static const ModeEntry SCHED_MODE_TABLE[] = {
    { "classic_card",      "Classic Card",       classicFactory,  classicMemReq,  0, &ClassicCardMode::_descriptor,         nullptr },
    { "live_flight",       "Live Flight Card",   liveFactory,     liveMemReq,     1, &LiveFlightCardMode::_descriptor,      nullptr },
    { "clock",             "Clock",              clockFactory,    clockMemReq,    2, &ClockMode::_descriptor,               &CLOCK_SCHEMA },
    { "departures_board",  "Departures Board",   depBdFactory,    depBdMemReq,    3, &DeparturesBoardMode::_descriptor,     &DEPBD_SCHEMA },
};
static constexpr uint8_t SCHED_MODE_COUNT = sizeof(SCHED_MODE_TABLE) / sizeof(SCHED_MODE_TABLE[0]);

static RenderContext makeTestCtx() {
    RenderContext ctx = {};
    ctx.matrix = nullptr;
    ctx.textColor = 0xFFFF;
    ctx.brightness = 40;
    ctx.logoBuffer = nullptr;
    ctx.displayCycleMs = 5000;
    return ctx;
}

static void initAll() {
    clearNvs("flightwall");
    ConfigManager::init();
    ModeRegistry::init(SCHED_MODE_TABLE, SCHED_MODE_COUNT);
    ModeOrchestrator::init();
    ModeRegistry::requestSwitch("classic_card");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
}

// ============================================================================
// minutesInWindow helper tests (AC #8, #11)
// ============================================================================

void test_minutes_in_window_same_day() {
    // 08:00–17:00 window
    TEST_ASSERT_TRUE(minutesInWindow(720, 480, 1020));   // 12:00 — IN
    TEST_ASSERT_FALSE(minutesInWindow(1200, 480, 1020)); // 20:00 — NOT
    TEST_ASSERT_FALSE(minutesInWindow(300, 480, 1020));  // 05:00 — NOT
}

void test_minutes_in_window_crosses_midnight() {
    // 23:00–07:00 window
    TEST_ASSERT_TRUE(minutesInWindow(120, 1380, 420));   // 02:00 — IN (after midnight)
    TEST_ASSERT_TRUE(minutesInWindow(1400, 1380, 420));  // 23:20 — IN (before midnight)
    TEST_ASSERT_FALSE(minutesInWindow(720, 1380, 420));  // 12:00 — NOT
}

void test_minutes_in_window_edge_cases() {
    // Zero-length window (start == end) → never in window
    TEST_ASSERT_FALSE(minutesInWindow(600, 600, 600));

    // Exactly at start → IN (inclusive)
    TEST_ASSERT_TRUE(minutesInWindow(480, 480, 1020));

    // Exactly at end → NOT (exclusive)
    TEST_ASSERT_FALSE(minutesInWindow(1020, 480, 1020));

    // Midnight-crossing: exactly at start → IN
    TEST_ASSERT_TRUE(minutesInWindow(1380, 1380, 420));

    // Midnight-crossing: exactly at end → NOT
    TEST_ASSERT_FALSE(minutesInWindow(420, 1380, 420));

    // Boundary: 0 and 1439
    TEST_ASSERT_TRUE(minutesInWindow(0, 0, 1439));
    TEST_ASSERT_FALSE(minutesInWindow(1439, 0, 1439));
}

// ============================================================================
// ConfigManager ModeScheduleConfig NVS tests (AC #1, #2)
// ============================================================================

void test_get_mode_schedule_empty_by_default() {
    clearNvs("flightwall");
    ConfigManager::init();
    ModeScheduleConfig cfg = ConfigManager::getModeSchedule();
    TEST_ASSERT_EQUAL_UINT8(0, cfg.ruleCount);
}

void test_set_get_mode_schedule_roundtrip() {
    clearNvs("flightwall");
    ConfigManager::init();

    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 2;
    cfg.rules[0].startMin = 480;   // 08:00
    cfg.rules[0].endMin = 1020;    // 17:00
    strncpy(cfg.rules[0].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;

    cfg.rules[1].startMin = 1380;  // 23:00
    cfg.rules[1].endMin = 420;     // 07:00
    strncpy(cfg.rules[1].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg.rules[1].enabled = 1;

    TEST_ASSERT_TRUE(ConfigManager::setModeSchedule(cfg));

    // Read back
    ModeScheduleConfig read = ConfigManager::getModeSchedule();
    TEST_ASSERT_EQUAL_UINT8(2, read.ruleCount);
    TEST_ASSERT_EQUAL_UINT16(480, read.rules[0].startMin);
    TEST_ASSERT_EQUAL_UINT16(1020, read.rules[0].endMin);
    TEST_ASSERT_EQUAL_STRING("departures_board", read.rules[0].modeId);
    TEST_ASSERT_EQUAL_UINT8(1, read.rules[0].enabled);

    TEST_ASSERT_EQUAL_UINT16(1380, read.rules[1].startMin);
    TEST_ASSERT_EQUAL_UINT16(420, read.rules[1].endMin);
    TEST_ASSERT_EQUAL_STRING("clock", read.rules[1].modeId);
    TEST_ASSERT_EQUAL_UINT8(1, read.rules[1].enabled);
}

void test_set_mode_schedule_rejects_out_of_range_time() {
    clearNvs("flightwall");
    ConfigManager::init();

    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 1440;  // OUT OF RANGE
    cfg.rules[0].endMin = 100;
    strncpy(cfg.rules[0].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;

    TEST_ASSERT_FALSE(ConfigManager::setModeSchedule(cfg));
}

void test_set_mode_schedule_rejects_empty_mode_id() {
    clearNvs("flightwall");
    ConfigManager::init();

    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    cfg.rules[0].modeId[0] = '\0';  // empty
    cfg.rules[0].enabled = 1;

    TEST_ASSERT_FALSE(ConfigManager::setModeSchedule(cfg));
}

void test_set_mode_schedule_rejects_too_many_rules() {
    clearNvs("flightwall");
    ConfigManager::init();

    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 9;  // exceeds MAX_SCHEDULE_RULES (8)
    TEST_ASSERT_FALSE(ConfigManager::setModeSchedule(cfg));
}

void test_set_mode_schedule_clears_old_rules() {
    clearNvs("flightwall");
    ConfigManager::init();

    // Write 3 rules
    ModeScheduleConfig cfg3 = {};
    cfg3.ruleCount = 3;
    for (int i = 0; i < 3; i++) {
        cfg3.rules[i].startMin = i * 100;
        cfg3.rules[i].endMin = i * 100 + 60;
        strncpy(cfg3.rules[i].modeId, "clock", MODE_ID_BUF_LEN - 1);
        cfg3.rules[i].enabled = 1;
    }
    TEST_ASSERT_TRUE(ConfigManager::setModeSchedule(cfg3));

    // Overwrite with 1 rule
    ModeScheduleConfig cfg1 = {};
    cfg1.ruleCount = 1;
    cfg1.rules[0].startMin = 0;
    cfg1.rules[0].endMin = 60;
    strncpy(cfg1.rules[0].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg1.rules[0].enabled = 1;
    TEST_ASSERT_TRUE(ConfigManager::setModeSchedule(cfg1));

    // Read back — should have only 1 rule
    ModeScheduleConfig read = ConfigManager::getModeSchedule();
    TEST_ASSERT_EQUAL_UINT8(1, read.ruleCount);
}

void test_nvs_key_lengths_within_limit() {
    // Verify all NVS keys are ≤15 chars
    // sched_r0_start = 14, sched_r7_start = 14 ✓
    // sched_r0_end   = 12 ✓
    // sched_r0_mode  = 13 ✓
    // sched_r0_ena   = 12 ✓
    // sched_r_count  = 13 ✓
    char key[16];
    for (int i = 0; i < 8; i++) {
        int len;
        len = snprintf(key, sizeof(key), "sched_r%d_start", i);
        TEST_ASSERT_TRUE_MESSAGE(len <= 15, "sched_rN_start key exceeds 15 chars");
        len = snprintf(key, sizeof(key), "sched_r%d_end", i);
        TEST_ASSERT_TRUE_MESSAGE(len <= 15, "sched_rN_end key exceeds 15 chars");
        len = snprintf(key, sizeof(key), "sched_r%d_mode", i);
        TEST_ASSERT_TRUE_MESSAGE(len <= 15, "sched_rN_mode key exceeds 15 chars");
        len = snprintf(key, sizeof(key), "sched_r%d_ena", i);
        TEST_ASSERT_TRUE_MESSAGE(len <= 15, "sched_rN_ena key exceeds 15 chars");
    }
    int len = snprintf(key, sizeof(key), "sched_r_count");
    TEST_ASSERT_TRUE_MESSAGE(len <= 15, "sched_r_count key exceeds 15 chars");
}

// ============================================================================
// ModeOrchestrator schedule evaluation tests (AC #3–#7, #9–#10)
// ============================================================================

void test_schedule_activates_matching_rule() {
    initAll();

    // Set up a schedule rule: 08:00–17:00 → departures_board
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    strncpy(cfg.rules[0].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    // Tick at 12:00 with NTP synced
    ModeOrchestrator::tick(5, true, 720);

    TEST_ASSERT_EQUAL(OrchestratorState::SCHEDULED, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("departures_board", ModeOrchestrator::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("scheduled", ModeOrchestrator::getStateString());
}

void test_schedule_first_enabled_rule_wins() {
    initAll();

    // Rule 0: 08:00–20:00 → clock (enabled)
    // Rule 1: 10:00–18:00 → departures_board (enabled)
    // At 12:00, both match — rule 0 should win (AC #5: first/lowest index)
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 2;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1200;
    strncpy(cfg.rules[0].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;

    cfg.rules[1].startMin = 600;
    cfg.rules[1].endMin = 1080;
    strncpy(cfg.rules[1].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[1].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    ModeOrchestrator::tick(5, true, 720);

    TEST_ASSERT_EQUAL(OrchestratorState::SCHEDULED, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
}

void test_schedule_disabled_rule_skipped() {
    initAll();

    // Rule 0: 08:00–20:00 → clock (DISABLED)
    // Rule 1: 10:00–18:00 → departures_board (enabled)
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 2;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1200;
    strncpy(cfg.rules[0].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 0;  // disabled

    cfg.rules[1].startMin = 600;
    cfg.rules[1].endMin = 1080;
    strncpy(cfg.rules[1].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[1].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    ModeOrchestrator::tick(5, true, 720);

    TEST_ASSERT_EQUAL(OrchestratorState::SCHEDULED, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("departures_board", ModeOrchestrator::getActiveModeId());
}

void test_schedule_no_match_stays_manual() {
    initAll();

    // Rule: 08:00–17:00 → clock
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    strncpy(cfg.rules[0].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    // Tick at 20:00 — no match
    ModeOrchestrator::tick(5, true, 1200);

    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getActiveModeId());
}

void test_schedule_exit_returns_to_manual() {
    initAll();

    // Rule: 08:00–17:00 → departures_board
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    strncpy(cfg.rules[0].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    // Enter SCHEDULED at 12:00
    ModeOrchestrator::tick(5, true, 720);
    TEST_ASSERT_EQUAL(OrchestratorState::SCHEDULED, ModeOrchestrator::getState());

    // Exit schedule at 18:00 — should return to MANUAL + manual mode (AC #4)
    ModeOrchestrator::tick(5, true, 1080);
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getActiveModeId());
}

void test_scheduled_zero_flights_no_idle_fallback() {
    initAll();

    // Rule: 08:00–17:00 → departures_board
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    strncpy(cfg.rules[0].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    // Tick with NTP synced, zero flights, at 12:00 (within schedule)
    // AC #6: SCHEDULED + flightCount == 0 → do NOT invoke idle fallback
    ModeOrchestrator::tick(0, true, 720);

    TEST_ASSERT_EQUAL(OrchestratorState::SCHEDULED, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("departures_board", ModeOrchestrator::getActiveModeId());
    // Explicitly NOT in IDLE_FALLBACK
    TEST_ASSERT_NOT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
}

void test_manual_zero_flights_still_triggers_idle_fallback() {
    initAll();

    // No schedule rules — MANUAL mode
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 0;
    ConfigManager::setModeSchedule(cfg);

    // Tick with zero flights (MANUAL + 0 flights → idle fallback as before)
    ModeOrchestrator::tick(0, true, 720);
    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
}

void test_no_ntp_skips_schedule_evaluation() {
    initAll();

    // Rule: 08:00–17:00 → departures_board
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    strncpy(cfg.rules[0].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    // Tick with NTP NOT synced — should remain MANUAL regardless of time
    ModeOrchestrator::tick(5, false, 720);
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getActiveModeId());
}

void test_schedule_state_reason_includes_rule_info() {
    initAll();

    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    strncpy(cfg.rules[0].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    ModeOrchestrator::tick(5, true, 720);

    // AC #10: state_reason should contain active rule index and mode id
    const char* reason = ModeOrchestrator::getStateReason();
    TEST_ASSERT_NOT_NULL(reason);
    TEST_ASSERT_TRUE(strstr(reason, "scheduled") != nullptr);
    TEST_ASSERT_TRUE(strstr(reason, "departures_board") != nullptr);
    TEST_ASSERT_TRUE(strstr(reason, "0") != nullptr);  // rule index 0
}

void test_manual_switch_overrides_schedule() {
    initAll();

    // Rule: 08:00–17:00 → departures_board
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    strncpy(cfg.rules[0].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    // Enter SCHEDULED
    ModeOrchestrator::tick(5, true, 720);
    TEST_ASSERT_EQUAL(OrchestratorState::SCHEDULED, ModeOrchestrator::getState());

    // User manually switches — should exit SCHEDULED
    ModeOrchestrator::onManualSwitch("live_flight", "Live Flight Card");
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("live_flight", ModeOrchestrator::getActiveModeId());

    // Note: next tick will re-enter SCHEDULED if rule still matches
    // This is correct — user override is transient until schedule window ends
    ModeOrchestrator::tick(5, true, 720);
    TEST_ASSERT_EQUAL(OrchestratorState::SCHEDULED, ModeOrchestrator::getState());
}

void test_schedule_cross_midnight_rule() {
    initAll();

    // Rule: 23:00–07:00 → clock (crosses midnight)
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 1380;
    cfg.rules[0].endMin = 420;
    strncpy(cfg.rules[0].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    // At 02:00 (after midnight) — should match
    ModeOrchestrator::tick(5, true, 120);
    TEST_ASSERT_EQUAL(OrchestratorState::SCHEDULED, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
}

// ============================================================================
// getActiveScheduleRuleIndex and JSON contract tests (Story dl-4.2)
// ============================================================================

void test_active_rule_index_default_negative_one() {
    initAll();
    TEST_ASSERT_EQUAL_INT8(-1, ModeOrchestrator::getActiveScheduleRuleIndex());
}

void test_active_rule_index_returns_matching_rule() {
    initAll();

    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 2;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 720;
    strncpy(cfg.rules[0].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;

    cfg.rules[1].startMin = 720;
    cfg.rules[1].endMin = 1020;
    strncpy(cfg.rules[1].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[1].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    // At 12:30 (750 min) — rule 1 should match
    ModeOrchestrator::tick(5, true, 750);
    TEST_ASSERT_EQUAL_INT8(1, ModeOrchestrator::getActiveScheduleRuleIndex());
}

void test_active_rule_index_resets_on_manual_switch() {
    initAll();

    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    strncpy(cfg.rules[0].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    ConfigManager::setModeSchedule(cfg);

    ModeOrchestrator::tick(5, true, 720);
    TEST_ASSERT_EQUAL_INT8(0, ModeOrchestrator::getActiveScheduleRuleIndex());

    ModeOrchestrator::onManualSwitch("classic_card", "Classic Card");
    TEST_ASSERT_EQUAL_INT8(-1, ModeOrchestrator::getActiveScheduleRuleIndex());
}

void test_schedule_json_serialize_roundtrip() {
    // Verify the API contract: ConfigManager set → get produces same data
    clearNvs("flightwall");
    ConfigManager::init();

    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 3;

    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 720;
    strncpy(cfg.rules[0].modeId, "departures_board", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;

    cfg.rules[1].startMin = 720;
    cfg.rules[1].endMin = 1020;
    strncpy(cfg.rules[1].modeId, "clock", MODE_ID_BUF_LEN - 1);
    cfg.rules[1].enabled = 0;

    cfg.rules[2].startMin = 1380;
    cfg.rules[2].endMin = 420;
    strncpy(cfg.rules[2].modeId, "live_flight", MODE_ID_BUF_LEN - 1);
    cfg.rules[2].enabled = 1;

    TEST_ASSERT_TRUE(ConfigManager::setModeSchedule(cfg));

    // Serialize to JSON (mirrors GET /api/schedule handler)
    JsonDocument doc;
    ModeScheduleConfig read = ConfigManager::getModeSchedule();
    JsonArray rules = doc["rules"].to<JsonArray>();
    for (uint8_t i = 0; i < read.ruleCount; i++) {
        JsonObject ruleObj = rules.add<JsonObject>();
        ruleObj["index"] = i;
        ruleObj["start_min"] = read.rules[i].startMin;
        ruleObj["end_min"] = read.rules[i].endMin;
        ruleObj["mode_id"] = (const char*)read.rules[i].modeId;
        ruleObj["enabled"] = read.rules[i].enabled;
    }

    TEST_ASSERT_EQUAL(3, rules.size());
    TEST_ASSERT_EQUAL(480, rules[0]["start_min"].as<int>());
    TEST_ASSERT_EQUAL(720, rules[0]["end_min"].as<int>());
    TEST_ASSERT_EQUAL_STRING("departures_board", rules[0]["mode_id"].as<const char*>());
    TEST_ASSERT_EQUAL(1, rules[0]["enabled"].as<int>());

    TEST_ASSERT_EQUAL(1380, rules[2]["start_min"].as<int>());
    TEST_ASSERT_EQUAL(420, rules[2]["end_min"].as<int>());
    TEST_ASSERT_EQUAL_STRING("live_flight", rules[2]["mode_id"].as<const char*>());
}

void test_schedule_compaction_no_orphans() {
    // AC #5: delete from middle, verify compacted order
    clearNvs("flightwall");
    ConfigManager::init();

    ModeScheduleConfig cfg3 = {};
    cfg3.ruleCount = 3;
    for (int i = 0; i < 3; i++) {
        cfg3.rules[i].startMin = (uint16_t)(i * 300);
        cfg3.rules[i].endMin = (uint16_t)(i * 300 + 120);
        strncpy(cfg3.rules[i].modeId, i == 1 ? "clock" : "departures_board", MODE_ID_BUF_LEN - 1);
        cfg3.rules[i].enabled = 1;
    }
    TEST_ASSERT_TRUE(ConfigManager::setModeSchedule(cfg3));

    // Simulate client deleting rule[1] (middle) and posting compacted array
    ModeScheduleConfig cfg2 = {};
    cfg2.ruleCount = 2;
    cfg2.rules[0] = cfg3.rules[0];  // original index 0
    cfg2.rules[1] = cfg3.rules[2];  // original index 2 → now index 1
    TEST_ASSERT_TRUE(ConfigManager::setModeSchedule(cfg2));

    ModeScheduleConfig read = ConfigManager::getModeSchedule();
    TEST_ASSERT_EQUAL_UINT8(2, read.ruleCount);
    TEST_ASSERT_EQUAL_UINT16(0, read.rules[0].startMin);
    TEST_ASSERT_EQUAL_STRING("departures_board", read.rules[0].modeId);
    TEST_ASSERT_EQUAL_UINT16(600, read.rules[1].startMin);
    TEST_ASSERT_EQUAL_STRING("departures_board", read.rules[1].modeId);
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // minutesInWindow helper
    RUN_TEST(test_minutes_in_window_same_day);
    RUN_TEST(test_minutes_in_window_crosses_midnight);
    RUN_TEST(test_minutes_in_window_edge_cases);

    // ConfigManager ModeScheduleConfig NVS
    RUN_TEST(test_get_mode_schedule_empty_by_default);
    RUN_TEST(test_set_get_mode_schedule_roundtrip);
    RUN_TEST(test_set_mode_schedule_rejects_out_of_range_time);
    RUN_TEST(test_set_mode_schedule_rejects_empty_mode_id);
    RUN_TEST(test_set_mode_schedule_rejects_too_many_rules);
    RUN_TEST(test_set_mode_schedule_clears_old_rules);
    RUN_TEST(test_nvs_key_lengths_within_limit);

    // ModeOrchestrator schedule evaluation
    RUN_TEST(test_schedule_activates_matching_rule);
    RUN_TEST(test_schedule_first_enabled_rule_wins);
    RUN_TEST(test_schedule_disabled_rule_skipped);
    RUN_TEST(test_schedule_no_match_stays_manual);
    RUN_TEST(test_schedule_exit_returns_to_manual);
    RUN_TEST(test_scheduled_zero_flights_no_idle_fallback);
    RUN_TEST(test_manual_zero_flights_still_triggers_idle_fallback);
    RUN_TEST(test_no_ntp_skips_schedule_evaluation);
    RUN_TEST(test_schedule_state_reason_includes_rule_info);
    RUN_TEST(test_manual_switch_overrides_schedule);
    RUN_TEST(test_schedule_cross_midnight_rule);

    // getActiveScheduleRuleIndex and JSON contract (Story dl-4.2)
    RUN_TEST(test_active_rule_index_default_negative_one);
    RUN_TEST(test_active_rule_index_returns_matching_rule);
    RUN_TEST(test_active_rule_index_resets_on_manual_switch);
    RUN_TEST(test_schedule_json_serialize_roundtrip);
    RUN_TEST(test_schedule_compaction_no_orphans);

    // Cleanup
    clearNvs("flightwall");

    UNITY_END();
}

void loop() {}
