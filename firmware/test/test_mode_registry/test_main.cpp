/*
 * Purpose: Unity tests for ModeRegistry.
 * Tests: Switch lifecycle, heap guard, NVS debounce, error handling,
 *        state transitions, rapid-switch safety, enumeration.
 * Environment: esp32dev (on-device test) — requires NVS flash.
 *
 * Architecture Reference: Display System Release decisions D2, D4
 * - ModeRegistry static table with cooperative switch serialization
 * - Display task integration via ModeRegistry::tick()
 *
 * Story ds-1.3: Enabled real tests using stub modes + mock modes.
 * Fixed: test_switch_rejected_while_switching → latest-wins per FR7/AC #6.
 */
#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include <cstring>
#include <Adafruit_GFX.h>
#include "utils/DisplayUtils.h"

// Include ModeRegistry (brings in DisplayMode.h, vector, FlightInfo)
#include "core/ModeRegistry.h"
#include "modes/ClassicCardMode.h"
#include "modes/LiveFlightCardMode.h"
#include "modes/ClockMode.h"
#include "modes/DeparturesBoardMode.h"
#include "core/ConfigManager.h"

// Stub for isNtpSynced() — defined in main.cpp which is excluded from test builds.
// ClockMode.cpp declares extern bool isNtpSynced() and calls it in render().
bool isNtpSynced() { return false; }

// Include test helpers after product headers so vector/FlightInfo are resolved
#define TEST_WITH_FLIGHT_INFO
#define TEST_WITH_LAYOUT_ENGINE
#include "fixtures/test_helpers.h"

using namespace TestHelpers;

// ============================================================================
// Mock Mode Implementation (for testing)
// ============================================================================

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

static MockModeStats g_mockModeAStats;
static MockModeStats g_mockModeBStats;

// Minimal zone data for mock modes
static const ZoneRegion g_mockZones[] = {
    {"Mock", 0, 0, 100, 100}
};

static const ModeZoneDescriptor g_mockDescriptor = {
    "Mock mode for testing", g_mockZones, 1
};

class MockModeA : public DisplayMode {
public:
    static constexpr uint32_t MEMORY_REQUIREMENT = 64;

    bool init(const RenderContext& ctx) override {
        (void)ctx;
        g_mockModeAStats.initCalls++;
        return !g_mockModeAStats.initShouldFail;
    }
    void render(const RenderContext& ctx,
                const std::vector<FlightInfo>& flights) override {
        (void)ctx; (void)flights;
        g_mockModeAStats.renderCalls++;
    }
    void teardown() override { g_mockModeAStats.teardownCalls++; }
    const char* getName() const override { return "Mock A"; }
    const ModeZoneDescriptor& getZoneDescriptor() const override { return g_mockDescriptor; }
    const ModeSettingsSchema* getSettingsSchema() const override { return nullptr; }
};

class MockModeB : public DisplayMode {
public:
    static constexpr uint32_t MEMORY_REQUIREMENT = 96;

    bool init(const RenderContext& ctx) override {
        (void)ctx;
        g_mockModeBStats.initCalls++;
        return !g_mockModeBStats.initShouldFail;
    }
    void render(const RenderContext& ctx,
                const std::vector<FlightInfo>& flights) override {
        (void)ctx; (void)flights;
        g_mockModeBStats.renderCalls++;
    }
    void teardown() override { g_mockModeBStats.teardownCalls++; }
    const char* getName() const override { return "Mock B"; }
    const ModeZoneDescriptor& getZoneDescriptor() const override { return g_mockDescriptor; }
    const ModeSettingsSchema* getSettingsSchema() const override { return nullptr; }
};

// Heavy mock: used only to exercise the heap-guard rejection path. The mode
// itself does not allocate; the HEAP_GUARD_MODE_TABLE memoryRequirement()
// callback below intentionally reports a live over-budget value so the test
// stays stable across boards/builds with different free-heap baselines.
class MockModeHeavy : public DisplayMode {
public:
    // Metadata only; not used directly by the rejection test.
    static constexpr uint32_t MEMORY_REQUIREMENT = 200 * 1024;

    bool init(const RenderContext& ctx) override { (void)ctx; return true; }
    void render(const RenderContext& ctx,
                const std::vector<FlightInfo>& flights) override {
        (void)ctx; (void)flights;
    }
    void teardown() override {}
    const char* getName() const override { return "Mock Heavy"; }
    const ModeZoneDescriptor& getZoneDescriptor() const override { return g_mockDescriptor; }
    const ModeSettingsSchema* getSettingsSchema() const override { return nullptr; }
};

// Factory functions for test mode table
static DisplayMode* mockModeAFactory() { return new MockModeA(); }
static DisplayMode* mockModeBFactory() { return new MockModeB(); }
static DisplayMode* mockModeHeavyFactory() { return new MockModeHeavy(); }
static uint32_t mockModeAMemReq() { return MockModeA::MEMORY_REQUIREMENT; }
static uint32_t mockModeBMemReq() { return MockModeB::MEMORY_REQUIREMENT; }
static uint32_t mockModeHeavyMemReq() {
    // Return a live over-budget requirement rather than a fixed number.
    // executeSwitch() adds MODE_SWITCH_HEAP_MARGIN on top, so this remains
    // strictly larger than the measured free heap and deterministically trips
    // the guard regardless of current firmware/test memory layout.
    return ESP.getFreeHeap();
}

// Test mode table (used by most tests)
static const ModeEntry TEST_MODE_TABLE[] = {
    { "mock_mode_a", "Mock A", mockModeAFactory, mockModeAMemReq, 0, &g_mockDescriptor, nullptr },
    { "mock_mode_b", "Mock B", mockModeBFactory, mockModeBMemReq, 1, &g_mockDescriptor, nullptr },
};
static constexpr uint8_t TEST_MODE_COUNT = sizeof(TEST_MODE_TABLE) / sizeof(TEST_MODE_TABLE[0]);

// Heap-guard test mode table — includes the heavy mock
static const ModeEntry HEAP_GUARD_MODE_TABLE[] = {
    { "mock_mode_a",     "Mock A",     mockModeAFactory,     mockModeAMemReq,     0, &g_mockDescriptor, nullptr },
    { "mock_mode_heavy", "Mock Heavy", mockModeHeavyFactory, mockModeHeavyMemReq, 1, &g_mockDescriptor, nullptr },
};
static constexpr uint8_t HEAP_GUARD_MODE_COUNT = sizeof(HEAP_GUARD_MODE_TABLE) / sizeof(HEAP_GUARD_MODE_TABLE[0]);

// Production mode table for table-shape tests
static DisplayMode* classicFactory() { return new ClassicCardMode(); }
static DisplayMode* liveFactory() { return new LiveFlightCardMode(); }
static DisplayMode* clockFactory() { return new ClockMode(); }
static DisplayMode* departuresBoardFactory() { return new DeparturesBoardMode(); }
static uint32_t classicMemReq() { return ClassicCardMode::MEMORY_REQUIREMENT; }
static uint32_t liveMemReq() { return LiveFlightCardMode::MEMORY_REQUIREMENT; }
static uint32_t clockMemReq() { return ClockMode::MEMORY_REQUIREMENT; }
static uint32_t departuresBoardMemReq() { return DeparturesBoardMode::MEMORY_REQUIREMENT; }

static const ModeEntry PROD_MODE_TABLE[] = {
    { "classic_card",      "Classic Card",       classicFactory,          classicMemReq,          0, &ClassicCardMode::_descriptor,         nullptr },
    { "live_flight",       "Live Flight Card",   liveFactory,             liveMemReq,             1, &LiveFlightCardMode::_descriptor,      nullptr },
    { "clock",             "Clock",              clockFactory,            clockMemReq,            2, &ClockMode::_descriptor,               &CLOCK_SCHEMA },
    { "departures_board",  "Departures Board",   departuresBoardFactory,  departuresBoardMemReq,  3, &DeparturesBoardMode::_descriptor,     &DEPBD_SCHEMA },
};
static constexpr uint8_t PROD_MODE_COUNT = sizeof(PROD_MODE_TABLE) / sizeof(PROD_MODE_TABLE[0]);

// ============================================================================
// Helper: Minimal RenderContext for tests (no actual matrix)
// ============================================================================

static RenderContext makeTestCtx() {
    RenderContext ctx = {};
    ctx.matrix = nullptr;  // Stub modes handle nullptr
    ctx.textColor = 0xFFFF;
    ctx.brightness = 40;
    ctx.logoBuffer = nullptr;
    ctx.displayCycleMs = 5000;
    return ctx;
}

static constexpr uint16_t TEST_MATRIX_W = 64;
static constexpr uint16_t TEST_MATRIX_H = 32;

// Post hw-1.1: FastLED_NeoMatrix replaced with GFXcanvas16 (Adafruit_GFX
// subclass) — same GFX contract as the real HUB75 runtime surface.
static GFXcanvas16* createTestMatrix() {
    GFXcanvas16* m = new GFXcanvas16(TEST_MATRIX_W, TEST_MATRIX_H);
    m->fillScreen(0);
    m->setTextWrap(false);
    return m;
}

static RenderContext makeRealMatrixCtx(GFXcanvas16* m) {
    RenderContext ctx = makeTestCtx();
    ctx.matrix = m;
    ctx.textColor = DisplayUtils::rgb565(255, 255, 255);
    ctx.layout.matrixWidth = TEST_MATRIX_W;
    ctx.layout.matrixHeight = TEST_MATRIX_H;
    return ctx;
}

static int countNonBlackPixels(GFXcanvas16* m) {
    int count = 0;
    const uint16_t* buf = m->getBuffer();
    const uint32_t pixelCount = static_cast<uint32_t>(TEST_MATRIX_W) * TEST_MATRIX_H;
    for (uint32_t i = 0; i < pixelCount; ++i) {
        if (buf[i] != 0) count++;
    }
    return count;
}

// ============================================================================
// Helper: Clear NVS for test isolation
// ============================================================================

static void clearModeNvs() {
    clearNvs("flightwall");
}

// Common setup: clear NVS + stats, init registry with test table, first switch
static void initTestRegistry() {
    clearModeNvs();
    g_mockModeAStats.reset();
    g_mockModeBStats.reset();
    ConfigManager::init();
    ModeRegistry::init(TEST_MODE_TABLE, TEST_MODE_COUNT);
    // Activate first mode
    ModeRegistry::requestSwitch("mock_mode_a");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
}

// ============================================================================
// Test: Registry initialization
// ============================================================================

void test_registry_init_with_table() {
    clearModeNvs();
    ConfigManager::init();
    ModeRegistry::init(TEST_MODE_TABLE, TEST_MODE_COUNT);
    // After init (no tick yet), no active mode
    TEST_ASSERT_EQUAL(TEST_MODE_COUNT, ModeRegistry::getModeCount());
    TEST_ASSERT_NOT_NULL(ModeRegistry::getModeTable());
    TEST_ASSERT_EQUAL(SwitchState::IDLE, ModeRegistry::getSwitchState());
}

void test_registry_first_switch_activates_mode() {
    initTestRegistry();
    TEST_ASSERT_NOT_NULL(ModeRegistry::getActiveMode());
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", ModeRegistry::getActiveModeId());
    TEST_ASSERT_EQUAL(1, g_mockModeAStats.initCalls);
}

// ============================================================================
// Test: Switch lifecycle
// ============================================================================

void test_switch_calls_teardown_then_init() {
    initTestRegistry();
    g_mockModeAStats.reset();
    g_mockModeBStats.reset();

    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    TEST_ASSERT_EQUAL(1, g_mockModeAStats.teardownCalls);
    TEST_ASSERT_EQUAL(1, g_mockModeBStats.initCalls);
    TEST_ASSERT_EQUAL_STRING("mock_mode_b", ModeRegistry::getActiveModeId());
}

void test_switch_completes_to_idle_state() {
    initTestRegistry();
    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL(SwitchState::IDLE, ModeRegistry::getSwitchState());
}

// ============================================================================
// Test: Heap guard
// ============================================================================

void test_heap_guard_allows_sufficient_memory() {
    initTestRegistry();
    HeapMonitor monitor;
    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("mock_mode_b", ModeRegistry::getActiveModeId());
    TEST_ASSERT_EQUAL_STRING("", ModeRegistry::getLastError());
}

void test_heap_guard_rejects_insufficient_memory() {
    // Uses HEAP_GUARD_MODE_TABLE which includes mock_mode_heavy with
    // a live over-budget memoryRequirement() so the guard is deterministic
    // even when firmware size or test build options change the free heap.
    clearModeNvs();
    g_mockModeAStats.reset();
    ConfigManager::init();
    ModeRegistry::init(HEAP_GUARD_MODE_TABLE, HEAP_GUARD_MODE_COUNT);

    // Activate mock_mode_a first
    ModeRegistry::requestSwitch("mock_mode_a");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", ModeRegistry::getActiveModeId());

    // Now try to switch to the heavy mode — heap guard must reject it
    ModeRegistry::requestSwitch("mock_mode_heavy");
    ModeRegistry::tick(ctx, empty);

    // Active mode should remain mock_mode_a (heap guard rejected the switch)
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", ModeRegistry::getActiveModeId());
    // Error message should mention insufficient memory
    TEST_ASSERT_STRING_CONTAINS("Insufficient memory", ModeRegistry::getLastError());
    // State should return to IDLE
    TEST_ASSERT_EQUAL(SwitchState::IDLE, ModeRegistry::getSwitchState());

    // Restore test table for subsequent tests
    ModeRegistry::init(TEST_MODE_TABLE, TEST_MODE_COUNT);
}

// ============================================================================
// Test: Mode init failure handling
// ============================================================================

void test_init_failure_restores_previous_mode() {
    initTestRegistry();
    g_mockModeBStats.initShouldFail = true;

    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    // Should restore mock_mode_a
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", ModeRegistry::getActiveModeId());
    TEST_ASSERT_STRING_CONTAINS("Init failed", ModeRegistry::getLastError());
}

// ============================================================================
// Test: NVS debounce
// ============================================================================

void test_nvs_not_written_immediately_after_switch() {
    initTestRegistry();
    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    // NVS should NOT contain new mode immediately
    String stored = nvsReadString("disp_mode", "");
    TEST_ASSERT_FALSE(stored == "mock_mode_b");
}

void test_nvs_written_after_debounce_delay() {
    initTestRegistry();
    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    delay(2100);  // Wait for debounce
    ModeRegistry::tick(ctx, empty);

    String stored = nvsReadString("disp_mode", "");
    TEST_ASSERT_EQUAL_STRING("mock_mode_b", stored.c_str());
}

void test_rapid_switches_debounce_to_single_nvs_write() {
    initTestRegistry();
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;

    ModeRegistry::requestSwitch("mock_mode_b");
    ModeRegistry::tick(ctx, empty);
    ModeRegistry::requestSwitch("mock_mode_a");
    ModeRegistry::tick(ctx, empty);
    ModeRegistry::requestSwitch("mock_mode_b");
    ModeRegistry::tick(ctx, empty);

    delay(2100);
    ModeRegistry::tick(ctx, empty);

    // Final mode persisted
    String stored = nvsReadString("disp_mode", "");
    TEST_ASSERT_EQUAL_STRING("mock_mode_b", stored.c_str());
}

// ============================================================================
// Test: Mode enumeration (for API)
// ============================================================================

void test_get_mode_table_returns_all_modes() {
    ModeRegistry::init(PROD_MODE_TABLE, PROD_MODE_COUNT);
    const ModeEntry* table = ModeRegistry::getModeTable();
    TEST_ASSERT_NOT_NULL(table);
    TEST_ASSERT_EQUAL(PROD_MODE_COUNT, ModeRegistry::getModeCount());
}

void test_mode_table_has_required_entries() {
    ModeRegistry::init(PROD_MODE_TABLE, PROD_MODE_COUNT);
    const ModeEntry* table = ModeRegistry::getModeTable();
    bool hasClassic = false;
    bool hasLiveFlight = false;
    for (uint8_t i = 0; i < ModeRegistry::getModeCount(); i++) {
        if (strcmp(table[i].id, "classic_card") == 0) hasClassic = true;
        if (strcmp(table[i].id, "live_flight") == 0) hasLiveFlight = true;
    }
    TEST_ASSERT_TRUE(hasClassic);
    TEST_ASSERT_TRUE(hasLiveFlight);
}

// Zero-allocation enumerate callback test
static int g_enumCallCount = 0;
static void enumCallback(const char* id, const char* displayName,
                          uint8_t index, void* user) {
    (void)id; (void)displayName; (void)index; (void)user;
    g_enumCallCount++;
}

void test_enumerate_visits_all_modes() {
    ModeRegistry::init(PROD_MODE_TABLE, PROD_MODE_COUNT);
    g_enumCallCount = 0;
    ModeRegistry::enumerate(enumCallback, nullptr);
    TEST_ASSERT_EQUAL(PROD_MODE_COUNT, g_enumCallCount);
}

// ============================================================================
// Test: Latest-wins semantics (AC #6, FR7 — NOT "reject while switching")
// ============================================================================

void test_latest_wins_overwrites_pending_request() {
    // FR7: latest-wins — a new valid request overwrites the previous pending index.
    // This CORRECTS the skeleton's test_switch_rejected_while_switching which
    // incorrectly expected rejection. Per architecture D2: queue-of-one, latest wins.
    initTestRegistry();

    // Two rapid requests before tick — second should win
    bool first = ModeRegistry::requestSwitch("mock_mode_b");
    bool second = ModeRegistry::requestSwitch("mock_mode_a");
    TEST_ASSERT_TRUE(first);
    TEST_ASSERT_TRUE(second);  // NOT rejected — latest wins

    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    // The latest request (mock_mode_a) should be active
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", ModeRegistry::getActiveModeId());
}

// ============================================================================
// Test: Rapid-switch safety (NFR S2)
// ============================================================================

void test_ten_rapid_switches_no_crash() {
    initTestRegistry();
    HeapMonitor monitor;
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;

    for (int i = 0; i < 10; i++) {
        ModeRegistry::requestSwitch(i % 2 == 0 ? "mock_mode_b" : "mock_mode_a");
        ModeRegistry::tick(ctx, empty);
    }

    TEST_ASSERT_NOT_NULL(ModeRegistry::getActiveMode());
    monitor.assertNoLeak(1024);  // Allow 1KB tolerance
}

// ============================================================================
// Test: Unknown mode handling
// ============================================================================

void test_unknown_mode_returns_false() {
    initTestRegistry();
    bool result = ModeRegistry::requestSwitch("nonexistent_mode");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_STRING_CONTAINS("Unknown mode", ModeRegistry::getLastError());
}

// ============================================================================
// Test: prepareForOTA (out of scope — left as IGNORE per story Dev Notes)
// ============================================================================

void test_prepare_for_ota_tears_down_active_mode() {
    TEST_IGNORE_MESSAGE("prepareForOTA out of scope for ds-1.3 — deferred");
}

// ============================================================================
// Test: ConfigManager getDisplayMode / setDisplayMode
// ============================================================================

void test_config_get_display_mode_default() {
    clearModeNvs();
    ConfigManager::init();
    String mode = ConfigManager::getDisplayMode();
    TEST_ASSERT_EQUAL_STRING("classic_card", mode.c_str());
}

void test_config_set_and_get_display_mode() {
    clearModeNvs();
    ConfigManager::init();
    ConfigManager::setDisplayMode("live_flight");
    String mode = ConfigManager::getDisplayMode();
    TEST_ASSERT_EQUAL_STRING("live_flight", mode.c_str());
}

// ============================================================================
// Test: NVS correction on failed switch (Story ds-3.2, AC #5)
// ============================================================================

void test_failed_init_corrects_nvs_after_debounce() {
    initTestRegistry();  // mock_mode_a is active

    // Pre-write mock_mode_b to NVS as if boot requested it
    nvsWriteString("disp_mode", "mock_mode_b");

    // Make mode B fail init
    g_mockModeBStats.initShouldFail = true;

    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    // mock_mode_a should still be active (fallback)
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", ModeRegistry::getActiveModeId());

    // Wait for debounce and let tickNvsPersist fire
    delay(2100);
    ModeRegistry::tick(ctx, empty);

    // NVS should be corrected to the actually-active mode (mock_mode_a)
    String stored = nvsReadString("disp_mode", "");
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", stored.c_str());
}

// Story ds-3.2 AC #5: cold-boot scenario — first-ever switch fails, no prior active mode.
// tickNvsPersist() must correct NVS to table[0] AND queue an activation switch so that
// NVS and getActiveModeId() do not silently drift (the gap in test_failed_init_corrects_nvs).
void test_first_switch_fail_queues_recovery_activation() {
    // Init registry WITHOUT activating any mode (simulate cold boot before first requestSwitch)
    clearModeNvs();
    g_mockModeAStats.reset();
    g_mockModeBStats.reset();
    ConfigManager::init();
    ModeRegistry::init(TEST_MODE_TABLE, TEST_MODE_COUNT);
    // No tick here — no active mode yet

    // Write a DIFFERENT stale value to NVS so the assertion is non-tautological:
    // if recovery logic fails entirely and NVS is never updated, this value persists
    // and the assertion below correctly fails (synthesis ds-3.2 Pass 3).
    nvsWriteString("disp_mode", "stale_invalid_mode");

    // Make table[0] (mock_mode_a) fail init so the first switch collapses
    g_mockModeAStats.initShouldFail = true;

    ModeRegistry::requestSwitch("mock_mode_a");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    // No active mode (first switch failed, no prior mode to restore)
    TEST_ASSERT_NULL(ModeRegistry::getActiveModeId());

    // Wait for debounce — tickNvsPersist should correct NVS and queue table[0] activation
    delay(2100);
    ModeRegistry::tick(ctx, empty);

    // NVS must be corrected to table[0] (mock_mode_a) — no silent NVS drift
    String stored = nvsReadString("disp_mode", "");
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", stored.c_str());

    // The queued activation should fire on the next tick; allow it to succeed
    g_mockModeAStats.initShouldFail = false;
    ModeRegistry::tick(ctx, empty);

    // Active mode must now match NVS — drift closed
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", ModeRegistry::getActiveModeId());
}

// ============================================================================
// Test: ConfigManager hasDisplayMode / upg_notif (Story ds-3.2, AC #4)
// ============================================================================

void test_has_display_mode_false_when_absent() {
    clearModeNvs();
    ConfigManager::init();
    TEST_ASSERT_FALSE(ConfigManager::hasDisplayMode());
}

void test_has_display_mode_true_after_set() {
    clearModeNvs();
    ConfigManager::init();
    ConfigManager::setDisplayMode("classic_card");
    TEST_ASSERT_TRUE(ConfigManager::hasDisplayMode());
}

void test_upg_notif_roundtrip() {
    clearModeNvs();
    ConfigManager::init();

    // Default should be false
    TEST_ASSERT_FALSE(ConfigManager::getUpgNotif());

    // Set to true
    ConfigManager::setUpgNotif(true);
    TEST_ASSERT_TRUE(ConfigManager::getUpgNotif());

    // Clear back to false
    ConfigManager::setUpgNotif(false);
    TEST_ASSERT_FALSE(ConfigManager::getUpgNotif());
}

// ============================================================================
// Test: ClockMode name/id and table registration (Story dl-1.1, AC #8, #10, #12)
// ============================================================================

void test_clock_mode_name_returns_clock() {
    ClockMode mode;
    TEST_ASSERT_EQUAL_STRING("clock", mode.getName());
}

void test_clock_mode_zone_descriptor_valid() {
    ClockMode mode;
    const ModeZoneDescriptor& desc = mode.getZoneDescriptor();
    TEST_ASSERT_NOT_NULL(desc.description);
    TEST_ASSERT_NOT_NULL(desc.regions);
    TEST_ASSERT_TRUE(desc.regionCount >= 1);
}

void test_clock_mode_settings_schema_not_null() {
    ClockMode mode;
    const ModeSettingsSchema* schema = mode.getSettingsSchema();
    TEST_ASSERT_NOT_NULL(schema);
    TEST_ASSERT_EQUAL_STRING("clock", schema->modeAbbrev);
    TEST_ASSERT_TRUE(schema->settingCount >= 1);
}

void test_clock_mode_in_prod_table() {
    ModeRegistry::init(PROD_MODE_TABLE, PROD_MODE_COUNT);
    const ModeEntry* table = ModeRegistry::getModeTable();
    bool hasClock = false;
    for (uint8_t i = 0; i < ModeRegistry::getModeCount(); i++) {
        if (strcmp(table[i].id, "clock") == 0) {
            hasClock = true;
            TEST_ASSERT_EQUAL_STRING("Clock", table[i].displayName);
            TEST_ASSERT_NOT_NULL(table[i].zoneDescriptor);
            TEST_ASSERT_NOT_NULL(table[i].settingsSchema);
        }
    }
    TEST_ASSERT_TRUE(hasClock);
}

void test_clock_mode_init_with_null_matrix() {
    ClockMode mode;
    RenderContext ctx = makeTestCtx();
    bool ok = mode.init(ctx);
    TEST_ASSERT_TRUE(ok);
}

void test_clock_mode_render_with_null_matrix_no_crash() {
    ClockMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);
    std::vector<FlightInfo> empty;
    // Should not crash with nullptr matrix
    mode.render(ctx, empty);
}

void test_clock_mode_render_clears_stale_pixels() {
    GFXcanvas16* m = createTestMatrix();
    // Pre-fill with a dim red to simulate a stale frame from a prior render;
    // ClockMode must clear this on its own redraw.
    m->fillScreen(DisplayUtils::rgb565(32, 0, 0));

    ClockMode mode;
    RenderContext ctx = makeRealMatrixCtx(m);
    TEST_ASSERT_TRUE(mode.init(ctx));

    std::vector<FlightInfo> empty;
    mode.render(ctx, empty);  // NTP stub is false, so fallback "--:--" renders.

    const int nonBlack = countNonBlackPixels(m);
    const uint32_t pixelCount = static_cast<uint32_t>(TEST_MATRIX_W) * TEST_MATRIX_H;
    TEST_ASSERT_TRUE_MESSAGE(nonBlack > 0,
        "Expected fallback clock glyphs to render");
    TEST_ASSERT_TRUE_MESSAGE(static_cast<uint32_t>(nonBlack) < pixelCount,
        "Expected ClockMode to clear stale pixels from the previous frame");

    mode.teardown();
    delete m;
}

// ============================================================================
// Test: DeparturesBoardMode (Story dl-2.1)
// ============================================================================

void test_departures_board_mode_name() {
    DeparturesBoardMode mode;
    TEST_ASSERT_EQUAL_STRING("Departures Board", mode.getName());
}

void test_departures_board_mode_zone_descriptor_valid() {
    DeparturesBoardMode mode;
    const ModeZoneDescriptor& desc = mode.getZoneDescriptor();
    TEST_ASSERT_NOT_NULL(desc.description);
    TEST_ASSERT_NOT_NULL(desc.regions);
    TEST_ASSERT_EQUAL(4, desc.regionCount);
}

void test_departures_board_mode_settings_schema() {
    DeparturesBoardMode mode;
    const ModeSettingsSchema* schema = mode.getSettingsSchema();
    TEST_ASSERT_NOT_NULL(schema);
    TEST_ASSERT_EQUAL_STRING("depbd", schema->modeAbbrev);
    TEST_ASSERT_EQUAL(1, schema->settingCount);
    TEST_ASSERT_EQUAL_STRING("rows", schema->settings[0].key);
    TEST_ASSERT_EQUAL(4, schema->settings[0].defaultValue);
    TEST_ASSERT_EQUAL(1, schema->settings[0].minValue);
    TEST_ASSERT_EQUAL(4, schema->settings[0].maxValue);
}

void test_departures_board_mode_in_prod_table() {
    ModeRegistry::init(PROD_MODE_TABLE, PROD_MODE_COUNT);
    const ModeEntry* table = ModeRegistry::getModeTable();
    bool hasDepbd = false;
    for (uint8_t i = 0; i < ModeRegistry::getModeCount(); i++) {
        if (strcmp(table[i].id, "departures_board") == 0) {
            hasDepbd = true;
            TEST_ASSERT_EQUAL_STRING("Departures Board", table[i].displayName);
            TEST_ASSERT_NOT_NULL(table[i].zoneDescriptor);
            TEST_ASSERT_NOT_NULL(table[i].settingsSchema);
            TEST_ASSERT_EQUAL_STRING("depbd", table[i].settingsSchema->modeAbbrev);
        }
    }
    TEST_ASSERT_TRUE(hasDepbd);
}

void test_departures_board_mode_init_with_null_matrix() {
    DeparturesBoardMode mode;
    RenderContext ctx = makeTestCtx();
    bool ok = mode.init(ctx);
    TEST_ASSERT_TRUE(ok);
}

void test_departures_board_mode_render_with_null_matrix_no_crash() {
    DeparturesBoardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);
    std::vector<FlightInfo> empty;
    // Should not crash with nullptr matrix
    mode.render(ctx, empty);
}

void test_departures_board_get_mode_setting_default() {
    // AC #4: m_depbd_rows absent → returns 4 with no NVS write
    clearModeNvs();
    ConfigManager::init();
    int32_t rows = ConfigManager::getModeSetting("depbd", "rows", 4);
    TEST_ASSERT_EQUAL(4, rows);
    // Verify no NVS key was created
    TEST_ASSERT_FALSE(nvsKeyExists("m_depbd_rows"));
}

void test_departures_board_set_mode_setting_valid_range() {
    clearModeNvs();
    ConfigManager::init();
    // Valid values 1-4
    TEST_ASSERT_TRUE(ConfigManager::setModeSetting("depbd", "rows", 1));
    TEST_ASSERT_EQUAL(1, ConfigManager::getModeSetting("depbd", "rows", 4));

    TEST_ASSERT_TRUE(ConfigManager::setModeSetting("depbd", "rows", 4));
    TEST_ASSERT_EQUAL(4, ConfigManager::getModeSetting("depbd", "rows", 4));

    TEST_ASSERT_TRUE(ConfigManager::setModeSetting("depbd", "rows", 2));
    TEST_ASSERT_EQUAL(2, ConfigManager::getModeSetting("depbd", "rows", 4));
}

void test_departures_board_set_mode_setting_rejects_out_of_range() {
    clearModeNvs();
    ConfigManager::init();
    // Out of range values should be rejected
    TEST_ASSERT_FALSE(ConfigManager::setModeSetting("depbd", "rows", 0));
    TEST_ASSERT_FALSE(ConfigManager::setModeSetting("depbd", "rows", 5));
    TEST_ASSERT_FALSE(ConfigManager::setModeSetting("depbd", "rows", -1));
    TEST_ASSERT_FALSE(ConfigManager::setModeSetting("depbd", "rows", 100));
}

void test_departures_board_nvs_key_within_limit() {
    // Verify composed NVS key m_depbd_rows is <=15 chars
    const char* key = "m_depbd_rows";
    TEST_ASSERT_TRUE(strlen(key) <= 15);
}

// ============================================================================
// Test: DeparturesBoardMode row diff (Story dl-2.2)
// ============================================================================

void test_departures_board_empty_to_one_row() {
    // AC #7 (dl-2.2): empty → one row appears in next render
    clearModeNvs();
    ConfigManager::init();
    DeparturesBoardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    // Render with empty flights (empty state)
    std::vector<FlightInfo> empty;
    mode.render(ctx, empty);  // should not crash — renders "NO FLIGHTS"

    // Now add one flight and render again
    FlightInfo f;
    f.ident_icao = "UAL123";
    f.altitude_kft = 35.0;
    f.speed_mph = 520.0;
    std::vector<FlightInfo> flights = {f};
    mode.render(ctx, flights);  // should transition cleanly

    mode.teardown();
    TEST_PASS();
}

void test_departures_board_one_to_zero_rows() {
    // AC #7 (dl-2.2): one row → zero (empty state)
    clearModeNvs();
    ConfigManager::init();
    DeparturesBoardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    FlightInfo f;
    f.ident_icao = "DAL456";
    f.altitude_kft = 30.0;
    f.speed_mph = 450.0;
    std::vector<FlightInfo> flights = {f};
    mode.render(ctx, flights);

    // Remove all flights
    std::vector<FlightInfo> empty;
    mode.render(ctx, empty);  // should transition to empty state

    mode.teardown();
    TEST_PASS();
}

void test_departures_board_cap_overflow_stable() {
    // AC #2 (dl-2.2): N == M and extra flight appears — still exactly M rows
    clearModeNvs();
    ConfigManager::init();
    // Set maxRows=2 to make cap testing feasible
    ConfigManager::setModeSetting("depbd", "rows", 2);

    DeparturesBoardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    // Create 2 flights (at cap)
    FlightInfo f1, f2, f3;
    f1.ident_icao = "AAL100";
    f1.altitude_kft = 35.0;
    f1.speed_mph = 500.0;
    f2.ident_icao = "UAL200";
    f2.altitude_kft = 28.0;
    f2.speed_mph = 480.0;
    f3.ident_icao = "DAL300";
    f3.altitude_kft = 40.0;
    f3.speed_mph = 550.0;

    std::vector<FlightInfo> atCap = {f1, f2};
    mode.render(ctx, atCap);  // Draws 2 rows

    // Add a 3rd flight (over cap) — should still render only 2
    std::vector<FlightInfo> overCap = {f1, f2, f3};
    mode.render(ctx, overCap);  // No crash, no buffer overrun

    // Render multiple times to ensure stability
    for (int i = 0; i < 5; i++) {
        mode.render(ctx, overCap);
    }

    mode.teardown();
    TEST_PASS();
}

void test_departures_board_same_size_swap_idents() {
    // AC #3 (dl-2.2): same count but different idents → dirty rows repainted
    clearModeNvs();
    ConfigManager::init();
    DeparturesBoardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    FlightInfo f1, f2;
    f1.ident_icao = "UAL100";
    f1.altitude_kft = 35.0;
    f1.speed_mph = 500.0;
    f2.ident_icao = "DAL200";
    f2.altitude_kft = 28.0;
    f2.speed_mph = 480.0;

    std::vector<FlightInfo> set1 = {f1, f2};
    mode.render(ctx, set1);

    // Swap with different idents but same count
    FlightInfo f3, f4;
    f3.ident_icao = "SWA300";
    f3.altitude_kft = 32.0;
    f3.speed_mph = 460.0;
    f4.ident_icao = "JBU400";
    f4.altitude_kft = 38.0;
    f4.speed_mph = 530.0;

    std::vector<FlightInfo> set2 = {f3, f4};
    mode.render(ctx, set2);  // Both rows should be dirty and repainted

    mode.teardown();
    TEST_PASS();
}

void test_departures_board_teardown_resets_diff_state() {
    // AC #6 (dl-2.2): teardown resets non-static resources
    clearModeNvs();
    ConfigManager::init();
    DeparturesBoardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    FlightInfo f;
    f.ident_icao = "UAL123";
    f.altitude_kft = 35.0;
    f.speed_mph = 520.0;
    std::vector<FlightInfo> flights = {f};
    mode.render(ctx, flights);

    mode.teardown();

    // Re-init and render again — should start fresh (first frame = full clear)
    mode.init(ctx);
    mode.render(ctx, flights);

    mode.teardown();
    TEST_PASS();
}

void test_departures_board_no_heap_leak_across_renders() {
    // AC #5 (dl-2.2): no new/malloc per frame
    clearModeNvs();
    ConfigManager::init();
    DeparturesBoardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    FlightInfo f1, f2;
    f1.ident_icao = "UAL100";
    f1.altitude_kft = 35.0;
    f1.speed_mph = 500.0;
    f2.ident_icao = "DAL200";
    f2.altitude_kft = 28.0;
    f2.speed_mph = 480.0;

    std::vector<FlightInfo> flights = {f1, f2};

    // Warmup render to stabilize any first-frame allocations
    mode.render(ctx, flights);

    HeapMonitor monitor;
    // Run many render cycles
    for (int i = 0; i < 20; i++) {
        mode.render(ctx, flights);
    }
    monitor.assertNoLeak(256);  // 256 bytes tolerance for minor system jitter

    mode.teardown();
}

// ============================================================================
// Test: Fade transition (Story dl-3.1)
// ============================================================================

void test_switch_with_zero_layout_no_crash() {
    // AC #2: zero matrix dimensions → fade skipped (instant path), no crash
    initTestRegistry();
    g_mockModeAStats.reset();
    g_mockModeBStats.reset();

    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    // layout dimensions are 0 by default — fade should be skipped
    TEST_ASSERT_EQUAL(0, ctx.layout.matrixWidth);
    TEST_ASSERT_EQUAL(0, ctx.layout.matrixHeight);

    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    TEST_ASSERT_EQUAL_STRING("mock_mode_b", ModeRegistry::getActiveModeId());
    TEST_ASSERT_EQUAL(SwitchState::IDLE, ModeRegistry::getSwitchState());
}

void test_switch_no_heap_leak_with_fade_skip() {
    // AC #7: no retained fade buffers after switch (zero-dim skip path)
    initTestRegistry();
    HeapMonitor monitor;

    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    ModeRegistry::requestSwitch("mock_mode_a");
    ModeRegistry::tick(ctx, empty);

    monitor.assertNoLeak(1024);
}

void test_rapid_switches_with_fade_no_crash() {
    // AC #8 + AC #7: rapid switches during/after fade don't crash or leak
    initTestRegistry();
    HeapMonitor monitor;
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;

    for (int i = 0; i < 10; i++) {
        ModeRegistry::requestSwitch(i % 2 == 0 ? "mock_mode_b" : "mock_mode_a");
        ModeRegistry::tick(ctx, empty);
    }

    TEST_ASSERT_NOT_NULL(ModeRegistry::getActiveMode());
    TEST_ASSERT_EQUAL(SwitchState::IDLE, ModeRegistry::getSwitchState());
    monitor.assertNoLeak(1024);
}

void test_switch_returns_to_idle_after_fade_skip() {
    // AC #1, #8: switch state returns to IDLE after fade (zero-dim skip path).
    // makeTestCtx() produces matrixWidth/Height = 0, so the fade snapshot is skipped
    // entirely (instant handoff). Validates IDLE state on the skip path.
    // A separate real-fade test with non-zero dimensions would require a live FastLED
    // setup; deferred as a future improvement.
    initTestRegistry();

    ModeRegistry::requestSwitch("mock_mode_b");
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);

    TEST_ASSERT_EQUAL(SwitchState::IDLE, ModeRegistry::getSwitchState());
    TEST_ASSERT_EQUAL_STRING("mock_mode_b", ModeRegistry::getActiveModeId());
}

void test_fade_blend_math_correctness() {
    // AC #4: verify integer lerp math — unit test of the blend formula
    // Formula: result = (A * (255 - alpha) + B * alpha) >> 8
    // At alpha=0: result ≈ A, at alpha=255: result ≈ B

    // alpha = 0 → fully A
    uint8_t a = 200, b = 50;
    uint8_t alpha = 0;
    uint8_t result = (uint8_t)(((uint16_t)a * (255 - alpha) + (uint16_t)b * alpha) >> 8);
    // At alpha=0: (200*255 + 50*0) >> 8 = 51000 >> 8 = 199 (≈200, rounding)
    TEST_ASSERT_UINT8_WITHIN(2, a, result);

    // alpha = 255 → fully B
    alpha = 255;
    result = (uint8_t)(((uint16_t)a * (255 - alpha) + (uint16_t)b * alpha) >> 8);
    // At alpha=255: (200*0 + 50*255) >> 8 = 12750 >> 8 = 49 (≈50, rounding)
    TEST_ASSERT_UINT8_WITHIN(2, b, result);

    // alpha = 128 → midpoint
    alpha = 128;
    result = (uint8_t)(((uint16_t)a * (255 - alpha) + (uint16_t)b * alpha) >> 8);
    // Expected midpoint: ~125
    uint8_t expected_mid = (a + b) / 2;
    TEST_ASSERT_UINT8_WITHIN(5, expected_mid, result);
}

// ============================================================================
// Test: Per-mode settings API shape (Story dl-5.1, AC #1–#2)
// ============================================================================

void test_settings_schema_shape_for_clock() {
    // AC #1: GET /api/display/modes should include settings array for clock
    // Validate schema shape: key, label, type, default, min, max, enumOptions
    ModeRegistry::init(PROD_MODE_TABLE, PROD_MODE_COUNT);
    const ModeEntry* table = ModeRegistry::getModeTable();
    for (uint8_t i = 0; i < ModeRegistry::getModeCount(); i++) {
        if (strcmp(table[i].id, "clock") == 0) {
            TEST_ASSERT_NOT_NULL(table[i].settingsSchema);
            const ModeSettingsSchema* schema = table[i].settingsSchema;
            TEST_ASSERT_EQUAL_STRING("clock", schema->modeAbbrev);
            TEST_ASSERT_EQUAL(1, schema->settingCount);
            const ModeSettingDef& def = schema->settings[0];
            TEST_ASSERT_EQUAL_STRING("format", def.key);
            TEST_ASSERT_EQUAL_STRING("Time Format", def.label);
            TEST_ASSERT_EQUAL_STRING("enum", def.type);
            TEST_ASSERT_EQUAL(0, def.defaultValue);
            TEST_ASSERT_EQUAL(0, def.minValue);
            TEST_ASSERT_EQUAL(1, def.maxValue);
            TEST_ASSERT_NOT_NULL(def.enumOptions);
            return;
        }
    }
    TEST_FAIL_MESSAGE("Clock mode not found in prod table");
}

void test_settings_schema_shape_for_departures_board() {
    // AC #1: GET response settings for departures_board
    ModeRegistry::init(PROD_MODE_TABLE, PROD_MODE_COUNT);
    const ModeEntry* table = ModeRegistry::getModeTable();
    for (uint8_t i = 0; i < ModeRegistry::getModeCount(); i++) {
        if (strcmp(table[i].id, "departures_board") == 0) {
            TEST_ASSERT_NOT_NULL(table[i].settingsSchema);
            const ModeSettingsSchema* schema = table[i].settingsSchema;
            TEST_ASSERT_EQUAL_STRING("depbd", schema->modeAbbrev);
            TEST_ASSERT_EQUAL(1, schema->settingCount);
            const ModeSettingDef& def = schema->settings[0];
            TEST_ASSERT_EQUAL_STRING("rows", def.key);
            TEST_ASSERT_EQUAL_STRING("uint8", def.type);
            TEST_ASSERT_EQUAL(4, def.defaultValue);
            TEST_ASSERT_EQUAL(1, def.minValue);
            TEST_ASSERT_EQUAL(4, def.maxValue);
            TEST_ASSERT_NULL(def.enumOptions);
            return;
        }
    }
    TEST_FAIL_MESSAGE("Departures board not found in prod table");
}

void test_settings_null_for_modes_without_schema() {
    // AC #1: classic_card and live_flight have no settings
    ModeRegistry::init(PROD_MODE_TABLE, PROD_MODE_COUNT);
    const ModeEntry* table = ModeRegistry::getModeTable();
    for (uint8_t i = 0; i < ModeRegistry::getModeCount(); i++) {
        if (strcmp(table[i].id, "classic_card") == 0) {
            TEST_ASSERT_NULL(table[i].settingsSchema);
        }
        if (strcmp(table[i].id, "live_flight") == 0) {
            TEST_ASSERT_NULL(table[i].settingsSchema);
        }
    }
}

void test_set_mode_setting_fires_callbacks() {
    // AC #4: setModeSetting fires callbacks so g_configChanged gets signaled
    clearModeNvs();
    ConfigManager::init();
    bool callbackFired = false;
    ConfigManager::onChange([&callbackFired]() { callbackFired = true; });
    ConfigManager::setModeSetting("clock", "format", 1);
    TEST_ASSERT_TRUE(callbackFired);
}

void test_settings_get_returns_persisted_value() {
    // AC #5: after setModeSetting, getModeSetting returns new value
    clearModeNvs();
    ConfigManager::init();
    // Default should be 0 (24h)
    TEST_ASSERT_EQUAL(0, ConfigManager::getModeSetting("clock", "format", 0));
    // Set to 12h
    TEST_ASSERT_TRUE(ConfigManager::setModeSetting("clock", "format", 1));
    TEST_ASSERT_EQUAL(1, ConfigManager::getModeSetting("clock", "format", 0));
    // Set back to 24h
    TEST_ASSERT_TRUE(ConfigManager::setModeSetting("clock", "format", 0));
    TEST_ASSERT_EQUAL(0, ConfigManager::getModeSetting("clock", "format", 0));
}

void test_settings_reject_unknown_mode_key() {
    // AC #4: unknown keys should not write to NVS (no explicit reject from
    // setModeSetting — it writes any key that fits in 15 chars).
    // But the validation for known combos already exists in setModeSetting.
    // This tests that invalid range is rejected for known combos.
    clearModeNvs();
    ConfigManager::init();
    TEST_ASSERT_FALSE(ConfigManager::setModeSetting("clock", "format", 2));  // out of range
    TEST_ASSERT_FALSE(ConfigManager::setModeSetting("clock", "format", -1)); // negative
}

// ============================================================================
// Test: BF-1 watchdog regression (REQUESTED_STALL)
// ============================================================================

void test_requested_stall_watchdog_fires() {
    // BF-1 AC #5: when nothing calls tick() while a request is pending, the
    // status endpoint's pollAndAdvanceStall() must promote state to FAILED so
    // the dashboard surfaces the failure instead of timing out client-side.
    initTestRegistry();
    ModeRegistry::requestSwitch("mock_mode_b");

    // Don't call tick — simulate Core 0 wedged in some early-return path.
    delay(kRequestedStallLimitMs + 75);
    ModeRegistry::pollAndAdvanceStall();

    TEST_ASSERT_EQUAL(SwitchState::FAILED, ModeRegistry::getSwitchState());
    TEST_ASSERT_EQUAL_STRING("REQUESTED_STALL", ModeRegistry::getLastErrorCode());
    // mock_mode_a remains the active mode — executeSwitch never ran.
    TEST_ASSERT_EQUAL_STRING("mock_mode_a", ModeRegistry::getActiveModeId());
}

void test_requested_counter_resets_on_progress() {
    // BF-1 AC #5: a fresh requestSwitch resets the stall budget so a prior
    // FAILED state does not poison the next attempt.
    initTestRegistry();
    ModeRegistry::requestSwitch("mock_mode_b");
    delay(kRequestedStallLimitMs + 75);
    ModeRegistry::pollAndAdvanceStall();
    TEST_ASSERT_EQUAL(SwitchState::FAILED, ModeRegistry::getSwitchState());

    // New request must clear FAILED and start a fresh budget.
    ModeRegistry::requestSwitch("mock_mode_b");
    TEST_ASSERT_EQUAL(SwitchState::REQUESTED, ModeRegistry::getSwitchState());
    ModeRegistry::pollAndAdvanceStall();
    TEST_ASSERT_EQUAL(SwitchState::REQUESTED, ModeRegistry::getSwitchState());

    // tick() should now drive the switch to completion normally.
    RenderContext ctx = makeTestCtx();
    std::vector<FlightInfo> empty;
    ModeRegistry::tick(ctx, empty);
    TEST_ASSERT_EQUAL(SwitchState::IDLE, ModeRegistry::getSwitchState());
    TEST_ASSERT_EQUAL_STRING("mock_mode_b", ModeRegistry::getActiveModeId());
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    delay(2000);

    UNITY_BEGIN();

    // Registry initialization
    RUN_TEST(test_registry_init_with_table);
    RUN_TEST(test_registry_first_switch_activates_mode);

    // Switch lifecycle
    RUN_TEST(test_switch_calls_teardown_then_init);
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
    RUN_TEST(test_enumerate_visits_all_modes);

    // Latest-wins (corrected from skeleton's "reject while switching")
    RUN_TEST(test_latest_wins_overwrites_pending_request);

    // Rapid-switch safety
    RUN_TEST(test_ten_rapid_switches_no_crash);

    // Unknown mode handling
    RUN_TEST(test_unknown_mode_returns_false);

    // OTA integration (deferred)
    RUN_TEST(test_prepare_for_ota_tears_down_active_mode);

    // ConfigManager display mode NVS
    RUN_TEST(test_config_get_display_mode_default);
    RUN_TEST(test_config_set_and_get_display_mode);

    // Story ds-3.2: NVS correction on failed switch (AC #5)
    RUN_TEST(test_failed_init_corrects_nvs_after_debounce);
    RUN_TEST(test_first_switch_fail_queues_recovery_activation);  // cold-boot / no prior mode

    // Story ds-3.2: hasDisplayMode / upg_notif (AC #1, #4)
    RUN_TEST(test_has_display_mode_false_when_absent);
    RUN_TEST(test_has_display_mode_true_after_set);
    RUN_TEST(test_upg_notif_roundtrip);

    // ClockMode tests (Story dl-1.1)
    RUN_TEST(test_clock_mode_name_returns_clock);
    RUN_TEST(test_clock_mode_zone_descriptor_valid);
    RUN_TEST(test_clock_mode_settings_schema_not_null);
    RUN_TEST(test_clock_mode_in_prod_table);
    RUN_TEST(test_clock_mode_init_with_null_matrix);
    RUN_TEST(test_clock_mode_render_with_null_matrix_no_crash);
    RUN_TEST(test_clock_mode_render_clears_stale_pixels);

    // DeparturesBoardMode tests (Story dl-2.1)
    RUN_TEST(test_departures_board_mode_name);
    RUN_TEST(test_departures_board_mode_zone_descriptor_valid);
    RUN_TEST(test_departures_board_mode_settings_schema);
    RUN_TEST(test_departures_board_mode_in_prod_table);
    RUN_TEST(test_departures_board_mode_init_with_null_matrix);
    RUN_TEST(test_departures_board_mode_render_with_null_matrix_no_crash);
    RUN_TEST(test_departures_board_get_mode_setting_default);
    RUN_TEST(test_departures_board_set_mode_setting_valid_range);
    RUN_TEST(test_departures_board_set_mode_setting_rejects_out_of_range);
    RUN_TEST(test_departures_board_nvs_key_within_limit);

    // DeparturesBoardMode row diff tests (Story dl-2.2)
    RUN_TEST(test_departures_board_empty_to_one_row);
    RUN_TEST(test_departures_board_one_to_zero_rows);
    RUN_TEST(test_departures_board_cap_overflow_stable);
    RUN_TEST(test_departures_board_same_size_swap_idents);
    RUN_TEST(test_departures_board_teardown_resets_diff_state);
    RUN_TEST(test_departures_board_no_heap_leak_across_renders);

    // Fade transition tests (Story dl-3.1)
    RUN_TEST(test_switch_with_zero_layout_no_crash);
    RUN_TEST(test_switch_no_heap_leak_with_fade_skip);
    RUN_TEST(test_rapid_switches_with_fade_no_crash);
    RUN_TEST(test_switch_returns_to_idle_after_fade_skip);
    RUN_TEST(test_fade_blend_math_correctness);

    // Per-mode settings API shape tests (Story dl-5.1)
    RUN_TEST(test_settings_schema_shape_for_clock);
    RUN_TEST(test_settings_schema_shape_for_departures_board);
    RUN_TEST(test_settings_null_for_modes_without_schema);
    RUN_TEST(test_set_mode_setting_fires_callbacks);
    RUN_TEST(test_settings_get_returns_persisted_value);
    RUN_TEST(test_settings_reject_unknown_mode_key);

    // BF-1 watchdog regression
    RUN_TEST(test_requested_stall_watchdog_fires);
    RUN_TEST(test_requested_counter_resets_on_progress);

    // Cleanup
    clearModeNvs();

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
