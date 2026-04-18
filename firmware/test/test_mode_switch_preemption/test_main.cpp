/*
 * Purpose: Unity tests for BF-1 mode-switch preemption + REQUESTED-stall watchdog.
 *
 * Story: BF-1 — Mode Switch Auto-Yields Active Test Patterns.
 * Coverage: ModeRegistry contract that the Core-0 display task auto-yield
 *           depends on (hasPendingRequest, markPreempted, getRequestedModeId,
 *           getLastPreemptionSource, pollAndAdvanceStall + FAILED state).
 *
 * Notes:
 * - These tests exercise the ModeRegistry side only. The actual auto-yield code
 *   in main.cpp is excluded from test builds (see PIO_UNIT_TESTING guards in
 *   src/main.cpp), so we cannot drive g_display.isCalibrationMode() here. We
 *   test the registry-level contract that main.cpp consumes — if this contract
 *   stays sound, the auto-yield path can be validated end-to-end by the
 *   Python smoke test (tests/smoke/test_web_portal_smoke.py) on a live device.
 */
#include <Arduino.h>
#include <unity.h>

#include "core/ModeRegistry.h"
#include "core/ConfigManager.h"

#define TEST_WITH_FLIGHT_INFO
#define TEST_WITH_LAYOUT_ENGINE
#include "fixtures/test_helpers.h"

using namespace TestHelpers;

// ============================================================================
// Mock mode (minimal — we only need a switch target, not real rendering)
// ============================================================================

struct PreemptMockStats {
    int initCalls = 0;
    int teardownCalls = 0;
    bool initShouldFail = false;
    void reset() { initCalls = 0; teardownCalls = 0; initShouldFail = false; }
};

static PreemptMockStats g_modeAStats;
static PreemptMockStats g_modeBStats;

static const ZoneRegion g_zones[] = { {"Z", 0, 0, 100, 100} };
static const ModeZoneDescriptor g_descriptor = { "preempt mock", g_zones, 1 };

class PreemptModeA : public DisplayMode {
public:
    static constexpr uint32_t MEMORY_REQUIREMENT = 64;
    bool init(const RenderContext& ctx) override {
        (void)ctx; g_modeAStats.initCalls++;
        return !g_modeAStats.initShouldFail;
    }
    void render(const RenderContext& ctx, const std::vector<FlightInfo>& f) override {
        (void)ctx; (void)f;
    }
    void teardown() override { g_modeAStats.teardownCalls++; }
    const char* getName() const override { return "Preempt A"; }
    const ModeZoneDescriptor& getZoneDescriptor() const override { return g_descriptor; }
    const ModeSettingsSchema* getSettingsSchema() const override { return nullptr; }
};

class PreemptModeB : public DisplayMode {
public:
    static constexpr uint32_t MEMORY_REQUIREMENT = 64;
    bool init(const RenderContext& ctx) override {
        (void)ctx; g_modeBStats.initCalls++;
        return !g_modeBStats.initShouldFail;
    }
    void render(const RenderContext& ctx, const std::vector<FlightInfo>& f) override {
        (void)ctx; (void)f;
    }
    void teardown() override { g_modeBStats.teardownCalls++; }
    const char* getName() const override { return "Preempt B"; }
    const ModeZoneDescriptor& getZoneDescriptor() const override { return g_descriptor; }
    const ModeSettingsSchema* getSettingsSchema() const override { return nullptr; }
};

static DisplayMode* factoryA() { return new PreemptModeA(); }
static DisplayMode* factoryB() { return new PreemptModeB(); }
static uint32_t memReqA() { return PreemptModeA::MEMORY_REQUIREMENT; }
static uint32_t memReqB() { return PreemptModeB::MEMORY_REQUIREMENT; }

static const ModeEntry PREEMPT_TABLE[] = {
    { "preempt_a", "Preempt A", factoryA, memReqA, 0, &g_descriptor, nullptr },
    { "preempt_b", "Preempt B", factoryB, memReqB, 1, &g_descriptor, nullptr },
};
static constexpr uint8_t PREEMPT_COUNT = sizeof(PREEMPT_TABLE) / sizeof(PREEMPT_TABLE[0]);

// ============================================================================
// Helpers
// ============================================================================

static RenderContext makeCtx() {
    RenderContext ctx = {};
    ctx.matrix = nullptr;
    ctx.textColor = 0xFFFF;
    ctx.brightness = 40;
    ctx.logoBuffer = nullptr;
    ctx.displayCycleMs = 5000;
    return ctx;
}

static void initRegistryWithA() {
    clearNvs("flightwall");
    g_modeAStats.reset();
    g_modeBStats.reset();
    ConfigManager::init();
    ModeRegistry::init(PREEMPT_TABLE, PREEMPT_COUNT);
    ModeRegistry::requestSwitch("preempt_a");
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("preempt_a", ModeRegistry::getActiveModeId());
}

// ============================================================================
// AC #1 / #2: hasPendingRequest contract
// ============================================================================

void test_has_pending_request_false_after_init() {
    initRegistryWithA();
    TEST_ASSERT_FALSE(ModeRegistry::hasPendingRequest());
}

void test_has_pending_request_true_between_request_and_tick() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    // Before tick consumes the request, hasPendingRequest must be true so the
    // Core-0 auto-yield in main.cpp can detect the pending switch and yield
    // any active calibration / positioning pattern.
    TEST_ASSERT_TRUE(ModeRegistry::hasPendingRequest());
}

void test_has_pending_request_false_after_tick_consumes() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_FALSE(ModeRegistry::hasPendingRequest());
}

// ============================================================================
// AC #3: getRequestedModeId names the pending target
// ============================================================================

void test_get_requested_mode_id_returns_target_after_request() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    const char* id = ModeRegistry::getRequestedModeId();
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_EQUAL_STRING("preempt_b", id);
}

void test_get_requested_mode_id_null_after_consumed() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_NULL(ModeRegistry::getRequestedModeId());
}

// ============================================================================
// AC #1, #2, #4: markPreempted("calibration") path drives a clean transition
// and surfaces preemption source after the switch completes.
// (Mirrors the auto-yield sequence main.cpp will execute.)
// ============================================================================

void test_mode_switch_preempts_calibration() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    // Simulate the Core-0 yield: main.cpp checks hasPendingRequest() before its
    // calibration `continue`, marks preemption, then falls through to tick().
    TEST_ASSERT_TRUE(ModeRegistry::hasPendingRequest());
    ModeRegistry::markPreempted("calibration");
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("preempt_b", ModeRegistry::getActiveModeId());
    TEST_ASSERT_EQUAL(SwitchState::IDLE, ModeRegistry::getSwitchState());
    const char* src = ModeRegistry::getLastPreemptionSource();
    TEST_ASSERT_NOT_NULL(src);
    TEST_ASSERT_EQUAL_STRING("calibration", src);
}

void test_mode_switch_preempts_positioning() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    TEST_ASSERT_TRUE(ModeRegistry::hasPendingRequest());
    ModeRegistry::markPreempted("positioning");
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("preempt_b", ModeRegistry::getActiveModeId());
    const char* src = ModeRegistry::getLastPreemptionSource();
    TEST_ASSERT_NOT_NULL(src);
    TEST_ASSERT_EQUAL_STRING("positioning", src);
}

// AC #4 baseline: when no test pattern was active, no preemption source is set.
void test_no_preemption_when_no_test_pattern_active() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    // Note: no markPreempted() call — the auto-yield only fires when a test
    // pattern is active. Plain mode switches must leave preempted == nullptr
    // so the dashboard does not show a misleading toast.
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("preempt_b", ModeRegistry::getActiveModeId());
    TEST_ASSERT_NULL(ModeRegistry::getLastPreemptionSource());
}

// AC #4: a fresh requestSwitch() clears the preemption source so the next
// transition starts with a clean slate.
void test_preemption_source_cleared_by_next_request() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    ModeRegistry::markPreempted("calibration");
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_NOT_NULL(ModeRegistry::getLastPreemptionSource());

    // Issue a fresh request; the preemption source must reset before tick runs.
    ModeRegistry::requestSwitch("preempt_a");
    TEST_ASSERT_NULL(ModeRegistry::getLastPreemptionSource());
}

// ============================================================================
// AC #5: REQUESTED-stall watchdog promotes to FAILED + REQUESTED_STALL.
// ============================================================================

void test_requested_stall_watchdog_does_not_fire_within_budget() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    // Immediately after request, well within budget — must stay REQUESTED.
    ModeRegistry::pollAndAdvanceStall();
    TEST_ASSERT_EQUAL(SwitchState::REQUESTED, ModeRegistry::getSwitchState());
}

void test_requested_stall_watchdog_fires_after_budget() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    // Sleep past the 500 ms stall budget without ticking the registry. This is
    // the failure mode the watchdog protects against — Core 0 wedged on some
    // future early-return path, never calling tick(). The dashboard polls
    // /api/display/mode/status which calls pollAndAdvanceStall().
    delay(kRequestedStallLimitMs + 75);
    ModeRegistry::pollAndAdvanceStall();
    TEST_ASSERT_EQUAL(SwitchState::FAILED, ModeRegistry::getSwitchState());
    const char* code = ModeRegistry::getLastErrorCode();
    TEST_ASSERT_NOT_NULL(code);
    TEST_ASSERT_EQUAL_STRING("REQUESTED_STALL", code);
    // Active mode index untouched — executeSwitch never ran.
    TEST_ASSERT_EQUAL_STRING("preempt_a", ModeRegistry::getActiveModeId());
}

void test_requested_stall_watchdog_clears_pending_request() {
    initRegistryWithA();
    ModeRegistry::requestSwitch("preempt_b");
    delay(kRequestedStallLimitMs + 75);
    ModeRegistry::pollAndAdvanceStall();
    // After FAILED, hasPendingRequest must be false so a future tick does not
    // pick up the stale request and silently switch later.
    TEST_ASSERT_FALSE(ModeRegistry::hasPendingRequest());
    TEST_ASSERT_NULL(ModeRegistry::getRequestedModeId());
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // hasPendingRequest contract
    RUN_TEST(test_has_pending_request_false_after_init);
    RUN_TEST(test_has_pending_request_true_between_request_and_tick);
    RUN_TEST(test_has_pending_request_false_after_tick_consumes);

    // Requested mode id accessor
    RUN_TEST(test_get_requested_mode_id_returns_target_after_request);
    RUN_TEST(test_get_requested_mode_id_null_after_consumed);

    // Preemption source flow
    RUN_TEST(test_mode_switch_preempts_calibration);
    RUN_TEST(test_mode_switch_preempts_positioning);
    RUN_TEST(test_no_preemption_when_no_test_pattern_active);
    RUN_TEST(test_preemption_source_cleared_by_next_request);

    // Watchdog
    RUN_TEST(test_requested_stall_watchdog_does_not_fire_within_budget);
    RUN_TEST(test_requested_stall_watchdog_fires_after_budget);
    RUN_TEST(test_requested_stall_watchdog_clears_pending_request);

    clearNvs("flightwall");
    UNITY_END();
}

void loop() {}
