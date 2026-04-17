/*
Purpose: Firmware entry point for ESP32.
Responsibilities:
- Initialize serial, connect to Wi-Fi, and construct fetchers and display.
- Periodically fetch state vectors (OpenSky), enrich flights (AeroAPI), and push to queue.
- Display task on Core 0 reads queue and renders to LED matrix independently.
Configuration: ConfigManager (NVS-backed with compile-time fallbacks).
Architecture: Producer-Consumer dual-core (Core 1 = fetch/network, Core 0 = display).
*/
#include <vector>
#include <atomic>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_task_wdt.h"
#include "esp_system.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_sntp.h"
#include "utils/Log.h"
#include "utils/TimeUtils.h"
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"
#include "adapters/OpenSkyFetcher.h"
#include "adapters/AeroAPIFetcher.h"
#include "core/FlightDataFetcher.h"
#include "adapters/NeoMatrixDisplay.h"
#include "adapters/WiFiManager.h"
#include "adapters/WebPortal.h"
#include "core/LayoutEngine.h"
#include "core/LogoManager.h"
#include "core/ModeOrchestrator.h"
#include "core/OTAUpdater.h"
#include "core/ModeRegistry.h"
#include "modes/ClassicCardMode.h"
#include "modes/LiveFlightCardMode.h"
#include "modes/ClockMode.h"
#include "modes/DeparturesBoardMode.h"
#ifdef LE_V0_SPIKE
#include "modes/CustomLayoutMode.h"
#endif

// Firmware version defined in platformio.ini build_flags
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"  // Fallback for IDE/testing
#endif

// GitHub repo coordinates for OTA version check (Story dl-6.1, AC #8).
// Defined via -D build_flags in platformio.ini so forks can override without editing .cpp.
#ifndef GITHUB_REPO_OWNER
#define GITHUB_REPO_OWNER "LuckierTrout"
#endif
#ifndef GITHUB_REPO_NAME
#define GITHUB_REPO_NAME "TheFlightWall_OSS-main"
#endif

#ifndef PIO_UNIT_TESTING

// --- Mode table (Story ds-1.3, AC #2) ---
// Static factory functions and memory requirement wrappers for ModeRegistry.
// Adding a future mode = new modes/*.h/.cpp + one ModeEntry line here (AR5).

static DisplayMode* classicCardFactory() { return new ClassicCardMode(); }
static DisplayMode* liveFlightCardFactory() { return new LiveFlightCardMode(); }
static DisplayMode* clockFactory() { return new ClockMode(); }
static DisplayMode* departuresBoardFactory() { return new DeparturesBoardMode(); }
static uint32_t classicCardMemReq() { return ClassicCardMode::MEMORY_REQUIREMENT; }
static uint32_t liveFlightCardMemReq() { return LiveFlightCardMode::MEMORY_REQUIREMENT; }
static uint32_t clockMemReq() { return ClockMode::MEMORY_REQUIREMENT; }
static uint32_t departuresBoardMemReq() { return DeparturesBoardMode::MEMORY_REQUIREMENT; }
#ifdef LE_V0_SPIKE
static DisplayMode* customLayoutFactory() { return new CustomLayoutMode(); }
static uint32_t customLayoutMemReq() { return CustomLayoutMode::MEMORY_REQUIREMENT; }
#endif

static const ModeEntry MODE_TABLE[] = {
    { "classic_card",      "Classic Card",       classicCardFactory,      classicCardMemReq,      0, &ClassicCardMode::_descriptor,         nullptr },
    { "live_flight",       "Live Flight Card",   liveFlightCardFactory,   liveFlightCardMemReq,   1, &LiveFlightCardMode::_descriptor,      nullptr },
    { "clock",             "Clock",              clockFactory,            clockMemReq,            2, &ClockMode::_descriptor,               &CLOCK_SCHEMA },
    { "departures_board",  "Departures Board",   departuresBoardFactory,  departuresBoardMemReq,  3, &DeparturesBoardMode::_descriptor,     &DEPBD_SCHEMA },
#ifdef LE_V0_SPIKE
    { "custom_layout",     "Custom Layout (V0)", customLayoutFactory,     customLayoutMemReq,     4, &CustomLayoutMode::_descriptor,        nullptr },
#endif
};
static constexpr uint8_t MODE_COUNT = sizeof(MODE_TABLE) / sizeof(MODE_TABLE[0]);

// --- Shared data structures (Task 1) ---

struct FlightDisplayData {
    std::vector<FlightInfo> flights;
};

struct DisplayStatusMessage {
    char text[64];
    uint32_t durationMs;
};

// Double buffer for safe cross-core data transfer (no memcpy of String/vector)
static FlightDisplayData g_flightBuf[2];
static uint8_t g_writeBuf = 0;

// FreeRTOS queue holding a pointer to the current read buffer
static QueueHandle_t g_flightQueue = nullptr;
static QueueHandle_t g_displayMessageQueue = nullptr;

// Atomic flag for config change notification (Core 1 sets, Core 0 reads)
static std::atomic<bool> g_configChanged(false);

// NTP sync flag (Story fn-2.1) — set by SNTP callback on Core 1, read cross-core
// Rule #30: cross-core atomics live in main.cpp only
static std::atomic<bool> g_ntpSynced(false);

// Flight count for orchestrator idle fallback (Story dl-1.2, AC #3)
// Rule #30: cross-core atomics live in main.cpp only
// Updated after each fetch; read by periodic orchestrator tick in loop().
static std::atomic<uint8_t> g_flightCount(0);

// Night mode scheduler state (Story fn-2.2)
// Rule #30: cross-core atomics live in main.cpp only
static unsigned long g_lastSchedCheckMs = 0;
static std::atomic<bool> g_schedDimming(false);    // true when scheduler is actively overriding brightness
static std::atomic<bool> g_scheduleChanged(false); // dedicated flag for schedule state transitions (Core 1→Core 0)
static bool g_lastNtpSynced = false;               // for NTP state transition logging (Core 1 only — no atomic needed)
static constexpr unsigned long SCHED_CHECK_INTERVAL_MS = 1000;
static_assert(SCHED_CHECK_INTERVAL_MS <= 2000,
    "SCHED_CHECK_INTERVAL_MS must be <=2s to meet 1-second display response NFR");

// Public accessor for WebPortal and other modules (declared extern in consumers)
bool isNtpSynced() { return g_ntpSynced.load(); }
bool isScheduleDimming() { return g_schedDimming.load(); }

// SNTP sync notification callback — fires on LWIP task (Core 1) when time syncs.
// Guard: only flag success when sync is actually COMPLETED; the callback can also
// fire on intermediate or reset events (e.g., SNTP_SYNC_STATUS_RESET).
static void onNtpSync(struct timeval* tv) {
    if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        return;  // Ignore reset or in-progress SNTP events
    }
    g_ntpSynced.store(true);
    SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Clock synced");
    LOG_I("NTP", "Time synchronized");
}

// --- Startup progress coordinator (Story 1.8) ---
// Owns the ordered LED status sequence during post-wizard startup.
// Advances intentionally through phases; the single-entry overwrite queue
// means only one message is active at a time, so we drive transitions
// from loop() as each real event completes.

enum class StartupPhase : uint8_t {
    IDLE,               // Normal operation — coordinator inactive
    SAVING_CONFIG,      // "Saving config..." (set by reboot callback)
    CONNECTING_WIFI,    // "Connecting to WiFi..."
    WIFI_CONNECTED,     // "WiFi Connected ✓" then "IP: x.x.x.x"
    WIFI_FAILED,        // WiFi did not connect — fallback to AP/setup
    AUTHENTICATING,     // "Authenticating APIs..."
    FETCHING_FLIGHTS,   // "Fetching flights..."
    COMPLETE            // First flight data ready — hand off to normal rendering
};

static StartupPhase g_startupPhase = StartupPhase::IDLE;
static unsigned long g_phaseEnteredMs = 0;
static bool g_firstFetchDone = false;  // One-shot flag: first fetch after startup
static constexpr unsigned long AUTHENTICATING_DISPLAY_MS = 1000UL;

static void enterPhase(StartupPhase phase)
{
    g_startupPhase = phase;
    g_phaseEnteredMs = millis();
}

// --- Existing globals ---

static OpenSkyFetcher g_openSky;
static AeroAPIFetcher g_aeroApi;
static FlightDataFetcher *g_fetcher = nullptr;
static NeoMatrixDisplay g_display;
static WiFiManager g_wifiManager;
static AsyncWebServer g_webServer(80);
static WebPortal g_webPortal;

static bool g_forcedApSetup = false;  // Set if boot button held during startup

// --- Watchdog recovery detection (Story dl-1.4, AC #1) ---
// Set true when esp_reset_reason() indicates a watchdog reset.
// Policy: after WDT reboot, always force "clock" mode (simplest safe default).
static bool g_wdtRecoveryBoot = false;

// GPIO 0 is the "BOOT" button on most ESP32 devkits.
// Holding it low during boot forces AP setup mode.
// NOTE: GPIO 0 is a strapping pin — holding it low at hardware reset can
// trigger UART download mode in ROM. This detection runs after ROM boot,
// during Arduino setup(), so it only triggers when firmware is already running.
static constexpr gpio_num_t BOARD_BOOT_BUTTON_GPIO = GPIO_NUM_0;

// --- Boot button short-press state (Task 5) ---
static bool g_buttonLastState = HIGH;       // Previous debounced state
static unsigned long g_buttonLastChangeMs = 0;
static unsigned long g_buttonPressStartMs = 0;
static constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;
static constexpr unsigned long BUTTON_SHORT_PRESS_MAX_MS = 1000;  // Presses longer than this are ignored (long press)

static unsigned long g_lastFetchMs = 0;

// --- OTA self-check & rollback detection state (Story fn-1.4) ---
static bool g_rollbackDetected = false;
static bool g_otaSelfCheckDone = false;
static unsigned long g_bootStartMs = 0;
// 60s per Architecture Decision F3: allows good firmware to connect WiFi even on slow
// networks, while ensuring bootloader-triggered rollback if firmware crashes before this.
// Typical WiFi connect: 5–15s. No-WiFi fallback: marks valid after 60s.
static constexpr unsigned long OTA_SELF_CHECK_TIMEOUT_MS = 60000;
// Transient esp_ota_get_state_partition failures on early loop iterations can skip AC #1/#2
// messaging if WiFi is already up; retry a few times per visit before giving up for this call.
static constexpr int OTA_PENDING_VERIFY_PROBE_ATTEMPTS = 5;

// --- Flight stats snapshot for /api/status health page (Story 2.4) ---
// Written from loop() on Core 1 after each fetch; read from HTTP handler on async TCP task.
// Use std::atomic for each field to avoid torn reads on 32-bit ESP32.
static std::atomic<unsigned long> g_statsLastFetchMs(0);
static std::atomic<uint16_t> g_statsStateVectors(0);
static std::atomic<uint16_t> g_statsEnrichedFlights(0);
static std::atomic<uint16_t> g_statsLogosMatched(0);    // Placeholder 0 until Epic 3
static std::atomic<uint32_t> g_statsFetchesSinceBoot(0);

FlightStatsSnapshot getFlightStatsSnapshot() {
    FlightStatsSnapshot s;
    s.last_fetch_ms = g_statsLastFetchMs.load();
    s.state_vectors = g_statsStateVectors.load();
    s.enriched_flights = g_statsEnrichedFlights.load();
    s.logos_matched = g_statsLogosMatched.load();
    s.fetches_since_boot = g_statsFetchesSinceBoot.load();
    s.rollback_detected = g_rollbackDetected;
    return s;
}

// --- Layout engine state (Story 3.1) ---
// Computed once at boot from HardwareConfig; recomputed on config changes.
// Read by WebPortal for GET /api/layout.
static LayoutResult g_layout;

LayoutResult getCurrentLayout() {
    return g_layout;
}

static void queueDisplayMessage(const String &message, uint32_t durationMs = 0)
{
    if (g_displayMessageQueue == nullptr)
    {
        return;
    }

    DisplayStatusMessage statusMessage = {};
    snprintf(statusMessage.text, sizeof(statusMessage.text), "%s", message.c_str());
    statusMessage.durationMs = durationMs;
    xQueueOverwrite(g_displayMessageQueue, &statusMessage);
}

static void showInitialWiFiMessage()
{
    switch (g_wifiManager.getState())
    {
        case WiFiState::AP_SETUP:
            g_display.displayMessage(String("Setup Mode"));
            break;
        case WiFiState::CONNECTING:
            g_display.displayMessage(String("Connecting to WiFi..."));
            break;
        case WiFiState::STA_CONNECTED:
            g_display.displayMessage(String("IP: ") + g_wifiManager.getLocalIP());
            break;
        case WiFiState::STA_RECONNECTING:
            g_display.displayMessage(String("WiFi Lost..."));
            break;
        case WiFiState::AP_FALLBACK:
            g_display.displayMessage(String("WiFi Failed"));
            break;
    }
    g_display.show();  // Setup-time: commit message before display task processes frames
}

static void queueWiFiStateMessage(WiFiState state)
{
    // During startup progress, the coordinator drives display messages
    // so we intercept WiFi events to advance the progress sequence
    if (g_startupPhase != StartupPhase::IDLE &&
        g_startupPhase != StartupPhase::COMPLETE)
    {
        switch (state)
        {
            case WiFiState::STA_CONNECTED:
                enterPhase(StartupPhase::WIFI_CONNECTED);
                queueDisplayMessage(String("WiFi Connected ✓"), 2000);
                LOG_I("Main", "Startup: WiFi connected");
                return;
            case WiFiState::AP_FALLBACK:
                enterPhase(StartupPhase::WIFI_FAILED);
                queueDisplayMessage(String("WiFi Failed - Reopen Setup"));
                LOG_I("Main", "Startup: WiFi failed, returning to setup");
                return;
            case WiFiState::CONNECTING:
            case WiFiState::STA_RECONNECTING:
                // Already showing connecting message — don't overwrite
                return;
            default:
                break;
        }
    }

    // Normal (non-startup) WiFi state messages
    switch (state)
    {
        case WiFiState::AP_SETUP:
            queueDisplayMessage(String("Setup Mode"));
            break;
        case WiFiState::CONNECTING:
            queueDisplayMessage(String("Connecting to WiFi..."));
            break;
        case WiFiState::STA_CONNECTED:
            queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 3000);
            break;
        case WiFiState::STA_RECONNECTING:
            queueDisplayMessage(String("WiFi Lost..."));
            break;
        case WiFiState::AP_FALLBACK:
            queueDisplayMessage(String("WiFi Failed"));
            break;
    }
}

static bool hardwareConfigChanged(const HardwareConfig &lhs, const HardwareConfig &rhs)
{
    return lhs.tiles_x != rhs.tiles_x ||
           lhs.tiles_y != rhs.tiles_y ||
           lhs.tile_pixels != rhs.tile_pixels ||
           lhs.display_pin != rhs.display_pin ||
           lhs.origin_corner != rhs.origin_corner ||
           lhs.scan_dir != rhs.scan_dir ||
           lhs.zigzag != rhs.zigzag ||
           lhs.zone_logo_pct != rhs.zone_logo_pct ||     // ds-3.2 synthesis: hot-reload zone layout
           lhs.zone_split_pct != rhs.zone_split_pct ||
           lhs.zone_layout != rhs.zone_layout;
}

static bool hardwareGeometryChanged(const HardwareConfig &lhs, const HardwareConfig &rhs)
{
    return lhs.tiles_x != rhs.tiles_x ||
           lhs.tiles_y != rhs.tiles_y ||
           lhs.tile_pixels != rhs.tile_pixels ||
           lhs.display_pin != rhs.display_pin;
}

static bool hardwareMappingChanged(const HardwareConfig &lhs, const HardwareConfig &rhs)
{
    return lhs.origin_corner != rhs.origin_corner ||
           lhs.scan_dir != rhs.scan_dir ||
           lhs.zigzag != rhs.zigzag ||
           lhs.zone_logo_pct != rhs.zone_logo_pct ||     // ds-3.2 synthesis: zone changes update _layout via reconfigureFromConfig()
           lhs.zone_split_pct != rhs.zone_split_pct ||
           lhs.zone_layout != rhs.zone_layout;
}

// --- Display task (Task 2) ---

#ifdef LE_V0_METRICS
// Layout-editor V0 feasibility spike instrumentation.
// Compiled only when -DLE_V0_METRICS is set (via [env:esp32dev_le_baseline]
// and [env:esp32dev_le_spike] in platformio.ini). Measures ModeRegistry::tick()
// cost (which wraps the active mode's render()) and logs stats + heap every 10s.
// See story le-0-1-layout-editor-v0-feasibility-spike.md.
static constexpr size_t LE_V0_RING_SIZE = 128;
static uint32_t s_leSamples[LE_V0_RING_SIZE];
static size_t   s_leCount = 0;
static size_t   s_leIdx = 0;
static unsigned long s_leLastStatsMs = 0;

static uint32_t s_leRecordCalls = 0;  // diagnostic: confirms record() is invoked
static void le_v0_record(uint32_t us) {
    s_leSamples[s_leIdx] = us;
    s_leIdx = (s_leIdx + 1) % LE_V0_RING_SIZE;
    if (s_leCount < LE_V0_RING_SIZE) s_leCount++;
    s_leRecordCalls++;
    // Fire an early sanity log so we know the instrumented path is being hit.
    if (s_leRecordCalls == 1 || s_leRecordCalls == 20 || s_leRecordCalls == 100) {
        Serial.printf("[LE-V0] record-hit #%u (first sample us=%u)\n",
                      (unsigned)s_leRecordCalls, (unsigned)us);
    }
}

static void le_v0_maybe_log(unsigned long nowMs) {
    if (s_leLastStatsMs == 0) { s_leLastStatsMs = nowMs; return; }
    // Shortened to 2s for spike debugging — produces data faster if the device
    // reset-loops before the original 10s cadence can fire.
    if (nowMs - s_leLastStatsMs < 2000UL) return;
    s_leLastStatsMs = nowMs;
    if (s_leCount == 0) return;

    uint32_t tmp[LE_V0_RING_SIZE];
    size_t n = s_leCount;
    memcpy(tmp, s_leSamples, n * sizeof(uint32_t));
    // Insertion sort (n<=128, fine at 0.1Hz log cadence).
    for (size_t i = 1; i < n; ++i) {
        uint32_t k = tmp[i];
        size_t j = i;
        while (j > 0 && tmp[j-1] > k) { tmp[j] = tmp[j-1]; --j; }
        tmp[j] = k;
    }
    size_t p50 = n / 2;
    size_t p95 = (n * 95) / 100; if (p95 >= n) p95 = n - 1;
    uint64_t sum = 0; for (size_t i = 0; i < n; ++i) sum += tmp[i];
    uint32_t mean = (uint32_t)(sum / n);
    const char* modeId = ModeRegistry::getActiveModeId();
    if (modeId == nullptr) modeId = "none";

    Serial.printf("[LE-V0] mode=%s n=%u min=%uus mean=%uus p50=%uus p95=%uus max=%uus "
                  "heap_free=%u heap_min=%u\n",
                  modeId, (unsigned)n, tmp[0], mean, tmp[p50], tmp[p95], tmp[n-1],
                  (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap());
}
#endif // LE_V0_METRICS

void displayTask(void *pvParameters)
{
    LOG_I("DisplayTask", "Display task started");

    // Read initial config as local copies
    DisplayConfig localDisp = ConfigManager::getDisplay();
    HardwareConfig localHw = ConfigManager::getHardware();
    TimingConfig localTiming = ConfigManager::getTiming();

    // Cached RenderContext for mode system (Story ds-1.5, AC #3)
    // Rebuilt on config/schedule changes or first frame
    static RenderContext cachedCtx = {};
    bool ctxInitialized = false;

    bool statusMessageVisible = false;
    unsigned long statusMessageStartMs = 0;   // millis() when message was queued
    unsigned long statusMessageDurationMs = 0; // 0 = indefinite; overflow-safe via subtraction
    char lastStatusText[64] = {0};  // Stores last status message for redraw after rebuildMatrix

    // Empty flight vector for frames when queue has no data (AC #6)
    static std::vector<FlightInfo> emptyFlights;

    // Subscribe to task watchdog
    esp_task_wdt_add(NULL);

    for (;;)
    {
        // Check for config changes OR schedule state transitions (Story fn-2.2)
        // g_configChanged: fired by ConfigManager onChange (any setting change)
        // g_scheduleChanged: fired by tickNightScheduler() on dim window enter/exit
        // IMPORTANT: evaluate BOTH atomics unconditionally before the `if` to avoid the
        // C++ short-circuit `||` leaving g_scheduleChanged stale when g_configChanged is also true.
        bool _cfgChg  = g_configChanged.exchange(false);
        bool _schedChg = g_scheduleChanged.exchange(false);
        if (_cfgChg || _schedChg)
        {
            DisplayConfig newDisp = ConfigManager::getDisplay();
            HardwareConfig newHw = ConfigManager::getHardware();
            TimingConfig newTiming = ConfigManager::getTiming();

            if (hardwareConfigChanged(localHw, newHw))
            {
                if (hardwareGeometryChanged(localHw, newHw))
                {
                    localHw = newHw;
                    LOG_I("DisplayTask", "Display geometry changed; reboot required to apply layout");
                    // No automatic restart — dashboard sends POST /api/reboot after apply
                }
                else if (hardwareMappingChanged(localHw, newHw))
                {
                    localHw = newHw;
                    if (g_display.reconfigureFromConfig())
                    {
                        LOG_I("DisplayTask", "Applied matrix mapping change without reboot");
                        // rebuildMatrix() → clear() erases the framebuffer. If a status
                        // message was visible, redraw it so it's not silently lost.
                        if (statusMessageVisible && lastStatusText[0] != '\0') {
                            g_display.displayMessage(String(lastStatusText));
                            g_display.show();
                        }
                    }
                    else
                    {
                        LOG_E("DisplayTask", "Failed to reconfigure matrix mapping at runtime");
                    }
                }
            }

            localDisp = newDisp;
            localTiming = newTiming;
            g_display.updateBrightness(localDisp.brightness);

            // Apply scheduler brightness override if active (Story fn-2.2)
            // g_schedDimming is std::atomic<bool> — .load() for cross-core safety.
            // This also handles AC #6: any manual brightness API change triggers g_configChanged,
            // causing this handler to run and immediately re-apply the dim override if still in window.
            if (g_schedDimming.load()) {
                ScheduleConfig sched = ConfigManager::getSchedule();
                g_display.updateBrightness(sched.sched_dim_brt);
            }

            // Rebuild cached RenderContext after config/schedule updates (AC #3)
            cachedCtx = g_display.buildRenderContext();
            ctxInitialized = true;

            LOG_I("DisplayTask", "Config change detected, display settings updated");
        }

        // Build RenderContext on first frame if not yet initialized (AC #3)
        if (!ctxInitialized) {
            cachedCtx = g_display.buildRenderContext();
            ctxInitialized = true;
        }

        // Calibration mode (Story 4.2): render test pattern instead of flights
        // Checked before status messages so test patterns override persistent banners
        if (g_display.isCalibrationMode())
        {
            statusMessageVisible = false;
            g_display.renderCalibrationPattern();
            g_display.show();
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // Positioning mode: render panel position guide instead of flights
        if (g_display.isPositioningMode())
        {
            statusMessageVisible = false;
            g_display.renderPositioningPattern();
            g_display.show();
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

#ifdef LE_V0_METRICS
        // V0 spike measurement hack: bypass persistent startup status messages
        // so the mode render path (and instrumented ModeRegistry::tick) runs
        // from the first frame. The device still receives status messages but
        // they never persist across loop iterations in the spike build. This
        // lets us measure render cost without waiting for network fetch.
        statusMessageVisible = false;
#endif

        DisplayStatusMessage statusMessage = {};
        if (g_displayMessageQueue != nullptr &&
            xQueueReceive(g_displayMessageQueue, &statusMessage, 0) == pdTRUE)
        {
            statusMessageVisible = true;
            statusMessageStartMs = millis();
            statusMessageDurationMs = statusMessage.durationMs; // 0 = indefinite
            strncpy(lastStatusText, statusMessage.text, sizeof(lastStatusText) - 1);
            lastStatusText[sizeof(lastStatusText) - 1] = '\0';
            g_display.displayMessage(String(statusMessage.text));
            g_display.show();
        }

        if (statusMessageVisible)
        {
            // Overflow-safe: unsigned subtraction wraps correctly at the 49-day millis() rollover.
            if (statusMessageDurationMs == 0 || (millis() - statusMessageStartMs) < statusMessageDurationMs)
            {
                // Commit any hardware-state changes (e.g. scheduler brightness update) that
                // occurred this iteration but would otherwise be skipped by the continue below.
                // Without this show(), a brightness change from the night scheduler won't reach
                // the LEDs until the status message clears (which could be minutes for AP setup).
                g_display.show();
                esp_task_wdt_reset();
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            statusMessageVisible = false;
        }

        // Story dl-7.1, AC #2 / dl-7.2, AC #8: During OTA, mode is torn down —
        // show static message instead of calling tick() which would dereference null.
        // When OTA fails, show distinct "Update failed" visual before pipeline resumes.
        if (ModeRegistry::isOtaMode()) {
            if (OTAUpdater::getState() == OTAState::ERROR) {
                g_display.displayMessage(String("Update failed"));
            } else {
                g_display.displayMessage(String("Updating..."));
            }
            g_display.show();
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        // Flight pipeline: ModeRegistry::tick draws via active mode (Story ds-1.5, AC #2/#3/#5/#6)
        FlightDisplayData *ptr = nullptr;
        const std::vector<FlightInfo>* flightsPtr = &emptyFlights;

        if (g_flightQueue != nullptr && xQueuePeek(g_flightQueue, &ptr, 0) == pdTRUE && ptr != nullptr)
        {
            flightsPtr = &(ptr->flights);
        }

        // Mode system renders; fallback if no active mode (AC #5)
#ifdef LE_V0_METRICS
        uint32_t _leT0 = micros();
#endif
        ModeRegistry::tick(cachedCtx, *flightsPtr);
#ifdef LE_V0_METRICS
        le_v0_record(micros() - _leT0);
        le_v0_maybe_log(millis());
#endif
        if (ModeRegistry::getActiveMode() == nullptr) {
            g_display.displayFallbackCard(*flightsPtr);
        }

        // Single frame commit (FR35 — one show() per frame)
        g_display.show();

        // Log stack high watermark at verbose level for tuning
#if LOG_LEVEL >= 3
        static unsigned long lastStackLogMs = 0;
        const unsigned long nowMs = millis();
        if (nowMs - lastStackLogMs >= 30000UL)
        {
            lastStackLogMs = nowMs;
            Serial.println("[DisplayTask] Stack HWM: " + String(uxTaskGetStackHighWaterMark(NULL)) + " bytes");
        }
#endif

        // Reset watchdog and yield (~20fps frame rate)
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --- OTA rollback detection (Story fn-1.4, Task 1) ---
// Called once in setup(), before SystemStatus::init().
// SystemStatus::set() is deferred to loop() since SystemStatus isn't ready yet.

static void detectRollback() {
    const esp_partition_t* invalid = esp_ota_get_last_invalid_partition();
    if (invalid != NULL) {
        g_rollbackDetected = true;
        Serial.printf("[OTA] Rollback detected: partition '%s' was invalid\n", invalid->label);
    }
}

// --- OTA self-check (Story fn-1.4, Task 2) ---
// Called from loop() until complete. Marks firmware valid via WiFi-OR-Timeout strategy.
// Architecture Decision F3: WiFi connected OR 60s timeout — whichever comes first.

static void tryResolveOtaPendingVerifyCache(int8_t& cache) {
    if (cache != -1) return;
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    for (int attempt = 0; attempt < OTA_PENDING_VERIFY_PROBE_ATTEMPTS && cache == -1; ++attempt) {
        if (running && esp_ota_get_state_partition(running, &state) == ESP_OK) {
            cache = (state == ESP_OTA_IMG_PENDING_VERIFY) ? 1 : 0;
        }
    }
}

static void performOtaSelfCheck() {
    if (g_otaSelfCheckDone) return;

    // Cache pending-verify state once resolved — OTA partition state cannot change while we're
    // running, so avoid repeated IDF flash reads on every loop iteration.
    static int8_t s_isPendingVerify = -1;  // -1 = unchecked, 0 = already valid, 1 = pending
    tryResolveOtaPendingVerifyCache(s_isPendingVerify);
    // If running is NULL or state probe fails, s_isPendingVerify stays -1 and retries next call.

    unsigned long elapsed = millis() - g_bootStartMs;
    bool wifiConnected = (g_wifiManager.getState() == WiFiState::STA_CONNECTED);

    if (wifiConnected || elapsed >= OTA_SELF_CHECK_TIMEOUT_MS) {
        // WiFi may already be up on the first completion visit; probe again so we do not
        // treat a pending-verify boot as "normal" after a single transient read failure.
        tryResolveOtaPendingVerifyCache(s_isPendingVerify);
        const bool isPendingVerify = (s_isPendingVerify == 1);

        // Deferred rollback status — report as soon as WiFi/timeout condition fires,
        // independent of mark_valid result to satisfy AC #4 even if mark_valid fails.
        // Static guard prevents repeated SystemStatus::set calls on retry iterations.
        // Note: if mark_valid subsequently fails, its ERROR status will overwrite this WARNING
        // in the OTA slot, but rollback_detected remains surfaced via FlightStatsSnapshot.
        if (g_rollbackDetected) {
            static bool s_rollbackStatusSet = false;
            if (!s_rollbackStatusSet) {
                SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
                    "Firmware rolled back to previous version");
                s_rollbackStatusSet = true;
            }
        }

        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        if (err == ESP_OK) {
            if (isPendingVerify) {
                if (wifiConnected) {
                    unsigned long elapsedSec = elapsed / 1000;
                    String okMsg = "Firmware verified — WiFi connected in " + String(elapsedSec) + "s";
                    Serial.printf("[OTA] Firmware marked valid — WiFi connected in %lums\n", elapsed);
                    SystemStatus::set(Subsystem::OTA, StatusLevel::OK, okMsg);
                } else {
                    LOG_W("OTA", "Firmware marked valid on timeout — WiFi not verified");
                    SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
                        "Marked valid on timeout — WiFi not verified");
                }
            } else {
                LOG_V("OTA", "Self-check: already valid (normal boot)");
                // AC #6: No SystemStatus::set call on normal boot.
                // (Rollback WARNING above only fires when g_rollbackDetected is true — a distinct
                // condition from a fresh normal boot with no rollback history.)
            }

            g_otaSelfCheckDone = true;
        } else {
            // mark_valid failed — log but do NOT set done flag; allow retry next loop iteration.
            // This call is idempotent and should always succeed on valid partitions; persistent
            // failure indicates a flash/NVS hardware problem requiring investigation.
            // Static guard prevents log spam when condition fires on every loop during retry.
            static bool s_markValidErrorLogged = false;
            if (!s_markValidErrorLogged) {
                String errMsg = "Failed to mark firmware valid: error " + String(err);
                Serial.printf("[OTA] ERROR: esp_ota_mark_app_valid_cancel_rollback() failed: %d\n", err);
                SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, errMsg);
                s_markValidErrorLogged = true;
            }
        }
    }
}

// --- Partition validation helper (Story fn-1.1) ---

void validatePartitionLayout() {
    Serial.println("[Main] Validating partition layout...");

    // Expected partition sizes from custom_partitions.csv (Story fn-1.1)
    // IMPORTANT: If you modify custom_partitions.csv, update these constants
    const size_t EXPECTED_APP_SIZE = 0x180000;   // app0/app1: 1.5MB
    const size_t EXPECTED_SPIFFS_SIZE = 0xF0000; // spiffs: 960KB

    // Validate running app partition
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running) {
        if (running->size == EXPECTED_APP_SIZE) {
            Serial.printf("[Main] App partition: %s, size 0x%x (correct)\n", running->label, running->size);
        } else {
            Serial.printf("[Main] WARNING: App partition size mismatch: expected 0x%x, got 0x%x\n",
                  EXPECTED_APP_SIZE, running->size);
            Serial.println("WARNING: Partition table may not match firmware expectations!");
            Serial.println("Reflash with new partition table via USB if OTA updates fail.");
        }
    } else {
        Serial.println("[Main] WARNING: Could not determine running partition");
    }

    // Validate LittleFS partition
    const esp_partition_t* littlefs = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
    if (littlefs) {
        if (littlefs->size == EXPECTED_SPIFFS_SIZE) {
            Serial.printf("[Main] LittleFS partition: size 0x%x (correct)\n", littlefs->size);
        } else {
            Serial.printf("[Main] WARNING: LittleFS partition size mismatch: expected 0x%x, got 0x%x\n",
                  EXPECTED_SPIFFS_SIZE, littlefs->size);
        }
    } else {
        Serial.println("[Main] WARNING: Could not find LittleFS partition");
    }
}

// --- setup() (Task 3) ---

void setup()
{
    // Record boot time BEFORE Serial/delay for accurate OTA self-check timing (Story fn-1.4).
    // Story task requirement: capture millis() at top of setup(), before any delays.
    g_bootStartMs = millis();
    Serial.begin(115200);
    delay(200);

    // Log firmware version at boot (Story fn-1.1)
    Serial.println();
    Serial.println("=================================");
    Serial.printf("FlightWall Firmware v%s\n", FW_VERSION);
    Serial.println("=================================");

    // Detect rollback before anything else (Story fn-1.4, Task 1)
    // Must run before SystemStatus::init() — defers SystemStatus::set() to loop()
    detectRollback();

    // Watchdog recovery detection (Story dl-1.4, AC #1)
    // Read reset reason early; if watchdog-triggered, force clock mode at boot.
    // Policy: always force "clock" after WDT for simplicity (AC #1 preferred approach).
    {
        esp_reset_reason_t reason = esp_reset_reason();
        g_wdtRecoveryBoot = (reason == ESP_RST_TASK_WDT ||
                             reason == ESP_RST_INT_WDT  ||
                             reason == ESP_RST_WDT);
        if (g_wdtRecoveryBoot) {
            Serial.printf("[Main] WDT recovery boot detected (reason=%d) — will force clock mode\n",
                          (int)reason);
        }
    }

    // Validate partition layout matches expectations (Story fn-1.1)
    validatePartitionLayout();

    // Mount LittleFS without auto-format to prevent silent data loss
    if (!LittleFS.begin(false)) {
        LOG_E("Main", "LittleFS mount failed - filesystem may be corrupted or unformatted");
        Serial.println("WARNING: LittleFS mount failed!");
        Serial.println("To format filesystem, reflash with: pio run -t uploadfs");
        // Continue boot - device will function but web assets/logos unavailable
    } else {
        LOG_I("Main", "LittleFS mounted successfully");
    }

    ConfigManager::init();
    SystemStatus::init();
    OTAUpdater::init(GITHUB_REPO_OWNER, GITHUB_REPO_NAME);
    ModeOrchestrator::init();

    // ModeRegistry init (Story ds-1.3, AC #2)
    // Registers mode table; requestSwitch deferred until after g_display.initialize()
    // so buildRenderContext() has a valid matrix on first tick (Story ds-1.5, AC #4).
    ModeRegistry::init(MODE_TABLE, MODE_COUNT);

    // NTP: register SNTP sync callback BEFORE WiFiManager starts (Story fn-2.1)
    // Callback fires on LWIP task (Core 1) when NTP sync completes
    sntp_set_time_sync_notification_cb(onNtpSync);
    SystemStatus::set(Subsystem::NTP, StatusLevel::WARNING, "Clock not set");
    LOG_I("Main", "SNTP callback registered, initial NTP status: WARNING");

    // Logo manager initialization (Story 3.2) — after LittleFS mount
    if (!LogoManager::init()) {
        LOG_E("Main", "LogoManager init failed — fallback sprites will be used");
    }

    // Compute initial layout from hardware config (Story 3.1)
    g_layout = LayoutEngine::compute(ConfigManager::getHardware());
    LOG_I("Main", "Layout computed");
    Serial.println("[Main] Layout: " + String(g_layout.matrixWidth) + "x" + String(g_layout.matrixHeight) + " mode=" + g_layout.mode);

    // Display initialization before task creation (setup runs on Core 1)
    g_display.initialize();

    // Upgrade notification (Story ds-3.2, AC #4): detect Foundation Release upgrade.
    // If disp_mode key is absent, this is the first boot after firmware that includes
    // mode persistence — set upg_notif=1 so GET /api/display/modes reports it.
    // Coordinate with POST /api/display/notification/dismiss (clears flag) and ds-3.6 banner.
    if (!ConfigManager::hasDisplayMode()) {
        ConfigManager::setUpgNotif(true);
        LOG_I("Main", "First boot with mode persistence — upgrade notification set");
    }

    // Boot mode restore (Story ds-1.5, AC #4; ds-3.2, AC #3; dl-1.3, AC #4; dl-1.4):
    // Route through ModeOrchestrator::onManualSwitch (Rule 24).
    //
    // dl-1.4 policy: After WDT reboot, always force "clock" mode and persist to NVS.
    // Normal boot: restore NVS-saved mode; fall back to "clock" if registered,
    // else "classic_card" for unknown IDs (AC #3). Correct NVS to activated mode.
    {
        const ModeEntry* table = ModeRegistry::getModeTable();
        uint8_t count = ModeRegistry::getModeCount();

        // Helper lambda: look up mode display name from table
        auto findModeName = [&](const char* modeId) -> const char* {
            for (uint8_t i = 0; i < count; i++) {
                if (strcmp(table[i].id, modeId) == 0) return table[i].displayName;
            }
            return nullptr;
        };

#ifdef LE_V0_METRICS
        // V0 spike: force a deterministic mode per build variant so baseline vs
        // spike measurements compare apples-to-apples. Calls ModeRegistry::requestSwitch
        // directly — bypasses ModeOrchestrator's same-mode guard (which would skip
        // the registry activation when the orchestrator's initial _activeModeId
        // already matches the target).
        (void)findModeName;
        #ifdef LE_V0_SPIKE
            if (!ModeRegistry::requestSwitch("custom_layout")) {
                Serial.println("[LE-V0] Boot override FAILED: custom_layout not registered");
            } else {
                ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout (V0)");
                ConfigManager::setDisplayMode("custom_layout");
                Serial.println("[LE-V0] Boot override: forced mode = custom_layout");
            }
        #else
            if (!ModeRegistry::requestSwitch("classic_card")) {
                Serial.println("[LE-V0] Boot override FAILED: classic_card not registered");
            } else {
                ModeOrchestrator::onManualSwitch("classic_card", "Classic Card");
                ConfigManager::setDisplayMode("classic_card");
                Serial.println("[LE-V0] Boot override: forced mode = classic_card");
            }
        #endif
#else
        if (g_wdtRecoveryBoot) {
            // dl-1.4, AC #1: WDT recovery — force clock unconditionally
            const char* clockName = findModeName("clock");
            if (clockName) {
                LOG_W("Main", "WDT recovery: forcing clock mode");
                ModeOrchestrator::onManualSwitch("clock", clockName);
                ConfigManager::setDisplayMode("clock");
            } else {
                // Clock not registered (shouldn't happen after dl-1.1) — use classic_card
                LOG_W("Main", "WDT recovery: clock not registered, falling back to classic_card");
                ModeOrchestrator::onManualSwitch("classic_card", "Classic Card");
                ConfigManager::setDisplayMode("classic_card");
            }
        } else {
            // Normal boot: restore saved mode with fallback chain
            String savedMode = ConfigManager::getDisplayMode();
            const char* bootModeName = findModeName(savedMode.c_str());

            if (bootModeName) {
                // AC #2: valid saved mode on normal boot — restore it
                ModeOrchestrator::onManualSwitch(savedMode.c_str(), bootModeName);
            } else {
                // AC #3: unknown mode ID — fall back directly to "classic_card" and correct NVS
                LOG_W("Main", "Saved display mode invalid, correcting NVS to classic_card");
                ModeOrchestrator::onManualSwitch("classic_card", "Classic Card");
                ConfigManager::setDisplayMode("classic_card");
            }
        }
#endif
    }

    g_display.showLoading();
    g_display.show();  // Setup-time: commit loading screen before display task starts

    // Create flight data queue (length 1, stores a pointer)
    g_flightQueue = xQueueCreate(1, sizeof(FlightDisplayData *));
    if (g_flightQueue == nullptr) {
        LOG_E("Main", "Failed to create flight data queue");
    } else {
        LOG_I("Main", "Flight data queue created");
    }

    g_displayMessageQueue = xQueueCreate(1, sizeof(DisplayStatusMessage));
    if (g_displayMessageQueue == nullptr) {
        LOG_E("Main", "Failed to create display message queue");
    } else {
        LOG_I("Main", "Display message queue created");
    }

    // Register config change callback (sets atomic flag for display task + recompute layout)
    // Seed TZ cache BEFORE registering onChange so the first unrelated config write
    // (e.g., brightness) does not restart SNTP. The previous pattern initialised
    // s_lastAppliedTz inside the lambda (default ""), which mismatched the default
    // "UTC0", causing configTzTime() to fire unnecessarily on the very first write.
    static String s_lastAppliedTz = ConfigManager::getSchedule().timezone;
    ConfigManager::onChange([&s_lastAppliedTz]() {
        g_configChanged.store(true);
        g_layout = LayoutEngine::compute(ConfigManager::getHardware());

        // Timezone hot-reload (Story fn-2.1): re-apply POSIX TZ only when timezone
        // actually changes. Calling configTzTime() on every config write (brightness,
        // fetch_interval, etc.) would restart SNTP unnecessarily and discard in-flight
        // NTP requests. Compare against cached value to guard the restart.
        // Does NOT reset g_ntpSynced — only the SNTP callback controls that flag.
        ScheduleConfig sched = ConfigManager::getSchedule();
        if (sched.timezone != s_lastAppliedTz) {
            s_lastAppliedTz = sched.timezone;
            configTzTime(sched.timezone.c_str(), "pool.ntp.org", "time.nist.gov");
#if LOG_LEVEL >= 2
            Serial.println("[Main] Timezone hot-reloaded: " + sched.timezone);
#endif
        }
    });
    LOG_I("Main", "Config change callback registered");

    // Create display task pinned to Core 0 (priority 1, 8KB stack)
    BaseType_t taskResult = xTaskCreatePinnedToCore(
        displayTask,
        "display",
        8192,
        NULL,
        1,
        NULL,
        0
    );
    if (taskResult == pdPASS) {
        LOG_I("Main", "Display task created on Core 0 (8KB stack, priority 1)");
    } else {
        LOG_E("Main", "Failed to create display task");
    }

    // Boot button GPIO sampling — detect held-low for forced AP setup
    // Configure as input with pull-up (BOOT button is active-low)
    pinMode(BOARD_BOOT_BUTTON_GPIO, INPUT_PULLUP);
    delay(50); // Allow pull-up to settle
    if (digitalRead(BOARD_BOOT_BUTTON_GPIO) == LOW) {
        // Button is held — sample for ~500ms to distinguish from noise
        unsigned long holdStart = millis();
        bool held = true;
        while (millis() - holdStart < 500) {
            if (digitalRead(BOARD_BOOT_BUTTON_GPIO) == HIGH) {
                held = false;
                break;
            }
            delay(10);
        }
        if (held) {
            g_forcedApSetup = true;
            LOG_I("Main", "Boot button held — forcing AP setup mode");
        }
    }

    // WiFiManager initialization (non-blocking, event-driven)
    // Per architecture Decision 5: WiFiManager before WebPortal
    g_wifiManager.init(g_forcedApSetup);

    // Activate startup progress coordinator when WiFi credentials exist
    // (i.e. this is a post-setup boot, not first-time AP mode)
    if (g_forcedApSetup)
    {
        // Forced AP via boot button — show distinct message
        queueDisplayMessage(String("Setup Mode — Forced"));
        LOG_I("Main", "Displaying forced setup message");
    }
    else if (g_wifiManager.getState() == WiFiState::CONNECTING)
    {
        enterPhase(StartupPhase::CONNECTING_WIFI);
        queueDisplayMessage(String("Connecting to WiFi..."));
        LOG_I("Main", "Startup progress active: connecting to WiFi");
    }
    else
    {
        showInitialWiFiMessage();
    }

    g_wifiManager.onStateChange([](WiFiState oldState, WiFiState newState) {
        (void)oldState;
        queueWiFiStateMessage(newState);
    });

    // WebPortal initialization — register routes then start server
    // Server serves in both AP and STA modes; GET / routes dynamically based on WiFi state
    g_webPortal.init(g_webServer, g_wifiManager);

    // Register calibration callback (Story 4.2) — toggles gradient test pattern
    g_webPortal.onCalibration([](bool enabled) {
        g_display.setCalibrationMode(enabled);
        if (enabled) {
            LOG_I("Main", "Calibration mode started");
        } else {
            LOG_I("Main", "Calibration mode stopped");
        }
    });

    // Register positioning callback — toggles panel positioning guide
    g_webPortal.onPositioning([](bool enabled) {
        g_display.setPositioningMode(enabled);
        if (enabled) {
            LOG_I("Main", "Positioning mode started");
        } else {
            LOG_I("Main", "Positioning mode stopped");
        }
    });

    // Register reboot callback so "Saving config..." shows on LED before restart
    g_webPortal.onReboot([]() {
        enterPhase(StartupPhase::SAVING_CONFIG);
        queueDisplayMessage(String("Saving config..."));
        LOG_I("Main", "Startup: saving config before reboot");
    });

    g_webPortal.begin();
    LOG_I("Main", "WebPortal started");

    g_fetcher = new FlightDataFetcher(&g_openSky, &g_aeroApi);
    if (g_fetcher == nullptr)
    {
        LOG_E("Main", "Failed to create FlightDataFetcher");
    }

    // Enroll loop() task in TWDT for Core 1 stall detection (Story dl-1.4, AC #5).
    // Default timeout 5s (meets NFR10). esp_task_wdt_reset() called at top of loop()
    // and around blocking HTTP fetches to avoid false positives.
    esp_task_wdt_add(NULL);
    LOG_I("Main", "Loop task enrolled in TWDT (5s timeout)");

    // Heap baseline measurement (Story ds-1.1, AR1)
    // Logged after all Foundation init to establish the mode memory budget.
#if LOG_LEVEL >= 2
    Serial.printf("[HeapBaseline] Free heap after full init: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("[HeapBaseline] Max alloc block: %u bytes\n", ESP.getMaxAllocHeap());
#endif

    LOG_I("Main", "Setup complete");
}

// --- Startup progress coordinator tick (Story 1.8) ---
// Drives the progress state machine from loop() on Core 1.
// Each phase transition waits for a real event (WiFi connect, fetch complete)
// rather than blasting the queue. Returns true if a first-fetch should run now.

static bool tickStartupProgress()
{
    if (g_startupPhase == StartupPhase::IDLE ||
        g_startupPhase == StartupPhase::COMPLETE)
    {
        return false;
    }

    const unsigned long elapsed = millis() - g_phaseEnteredMs;

    switch (g_startupPhase)
    {
        case StartupPhase::SAVING_CONFIG:
            // Display persists until reboot — no transition needed here
            break;

        case StartupPhase::CONNECTING_WIFI:
            // Waiting for WiFi callback to advance to WIFI_CONNECTED or WIFI_FAILED
            break;

        case StartupPhase::WIFI_CONNECTED:
            // Show "WiFi Connected ✓" briefly, then IP, then move to auth
            if (elapsed < 2000)
            {
                // Still showing "WiFi Connected"
            }
            else if (elapsed < 4000)
            {
                // Show IP address for discovery — guard with static flag so loop() running at
                // thousands of iterations/second does not spam snprintf + xQueueOverwrite
                static bool ipShown = false;
                if (!ipShown && elapsed >= 2000)
                {
                    queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 2000);
                    ipShown = true;
                }
            }
            else
            {
                // Advance to authentication phase
                enterPhase(StartupPhase::AUTHENTICATING);
                queueDisplayMessage(String("Authenticating APIs..."));
                LOG_I("Main", "Startup: authenticating APIs");
            }
            break;

        case StartupPhase::WIFI_FAILED:
            // Terminal state — device returns to AP/setup mode
            // WiFiManager handles the AP fallback; message stays visible
            if (elapsed >= 5000)
            {
                // After showing failure message, return to idle
                // (device is now in AP mode, user must re-run wizard)
                enterPhase(StartupPhase::IDLE);
            }
            break;

        case StartupPhase::AUTHENTICATING:
            // Hold the authentication message briefly before starting the first fetch.
            if (elapsed >= AUTHENTICATING_DISPLAY_MS)
            {
                return true;
            }
            break;

        case StartupPhase::FETCHING_FLIGHTS:
            // Waiting for fetch to complete — transition driven by loop()
            break;

        default:
            break;
    }

    return false;
}

// Night mode brightness scheduler (Story fn-2.2, Architecture Decision F5)
// Runs ~1/sec in Core 1 main loop. Non-blocking per Enforcement Rule #14.
// Uses minutes-since-midnight comparison per Enforcement Rule #15.
static void tickNightScheduler() {
    const unsigned long now = millis();
    if (now - g_lastSchedCheckMs < SCHED_CHECK_INTERVAL_MS) return;
    g_lastSchedCheckMs = now;

    ScheduleConfig sched = ConfigManager::getSchedule();
    bool wasDimming = g_schedDimming.load();
    bool ntpSynced  = g_ntpSynced.load();

    // Log NTP state transitions (AC #4: log only on lost/regained, not every tick)
    // Note: sched_enabled check happens below — log message stays neutral so it is
    // accurate even when the schedule is disabled (sched_enabled == 0).
    if (ntpSynced && !g_lastNtpSynced) {
        LOG_I("Scheduler", "NTP synced — local time available");
    } else if (!ntpSynced && g_lastNtpSynced) {
        LOG_I("Scheduler", "NTP lost — schedule suspended");
    }
    g_lastNtpSynced = ntpSynced;

    // Guard: NTP must be synced and schedule must be enabled
    if (sched.sched_enabled != 1 || !ntpSynced) {
        if (wasDimming) {
            g_schedDimming.store(false);
            g_scheduleChanged.store(true);
            LOG_I("Scheduler", "Schedule inactive — brightness restored");
        }
        return;
    }

    // Get local time non-blocking (Rule #14: timeout=0)
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        return;  // Time not available yet — skip this cycle
    }

    // Convert to minutes since midnight (Rule #15)
    uint16_t nowMinutes = (uint16_t)(timeinfo.tm_hour * 60 + timeinfo.tm_min);

    // Determine if current time is within the dim window
    // Uses shared minutesInWindow() (Story dl-4.1, AC #8) — same convention for both schedulers
    bool inDimWindow = minutesInWindow(nowMinutes, sched.sched_dim_start, sched.sched_dim_end);

    // Signal only on state transitions — not every tick.
    // AC #6 re-override is handled by display task's config-change handler:
    // it checks g_schedDimming after g_configChanged or g_scheduleChanged.
    if (inDimWindow && !wasDimming) {
        g_schedDimming.store(true);
        g_scheduleChanged.store(true);
        LOG_I("Scheduler", "Entering dim window — brightness override active");
    } else if (!inDimWindow && wasDimming) {
        g_schedDimming.store(false);
        g_scheduleChanged.store(true);
        LOG_I("Scheduler", "Exiting dim window — brightness restored");
    }
}

// --- loop() (Task 4) ---

void loop()
{
    // Reset loop task watchdog each iteration (Story dl-1.4, AC #5).
    // Enrolled in setup() via esp_task_wdt_add(NULL) on the loopTask.
    // Default TWDT timeout is 5s (CONFIG_ESP_TASK_WDT_TIMEOUT_S=5), meets NFR10 (≤10s).
    esp_task_wdt_reset();

    ConfigManager::tick();
    g_wifiManager.tick();

    // Night mode brightness scheduler (Story fn-2.2)
    tickNightScheduler();

    // Periodic orchestrator tick for idle fallback + mode schedule (Story dl-1.2, dl-4.1)
    // Runs ~1/sec independent of fetch interval so boot + zero flights transitions
    // within ~1s, not only on the next 10+ minute fetch.
    // Story dl-4.1, AC #3: evaluate schedule rules each ~1s tick when NTP synced.
    {
        static unsigned long s_lastOrchMs = 0;
        const unsigned long nowOrch = millis();
        if (nowOrch - s_lastOrchMs >= 1000) {
            s_lastOrchMs = nowOrch;

            // Compute NTP-gated current time for schedule evaluation (AC #3)
            bool orchNtpSynced = g_ntpSynced.load();
            uint16_t orchNowMin = 0;
            if (orchNtpSynced) {
                struct tm orchTimeinfo;
                if (getLocalTime(&orchTimeinfo, 0)) {
                    orchNowMin = (uint16_t)(orchTimeinfo.tm_hour * 60 + orchTimeinfo.tm_min);
                } else {
                    orchNtpSynced = false;  // getLocalTime failed — skip schedule this tick
                }
            }

            ModeOrchestrator::tick(g_flightCount.load(), orchNtpSynced, orchNowMin);
        }
    }

    // OTA self-check: mark firmware valid once WiFi connects or timeout expires (Story fn-1.4)
    if (!g_otaSelfCheckDone) {
        performOtaSelfCheck();
    }

    // Boot button short-press detection (millis debounce, no ISR)
    // Only active during normal operation — skip if forced AP setup
    if (!g_forcedApSetup) {
        bool raw = digitalRead(BOARD_BOOT_BUTTON_GPIO);
        unsigned long now_btn = millis();

        if (raw != g_buttonLastState && (now_btn - g_buttonLastChangeMs >= BUTTON_DEBOUNCE_MS)) {
            g_buttonLastChangeMs = now_btn;

            if (raw == LOW) {
                // Button pressed (active-low)
                g_buttonPressStartMs = now_btn;
            } else {
                // Button released — check for short press
                unsigned long pressDuration = now_btn - g_buttonPressStartMs;
                if (pressDuration > 0 && pressDuration <= BUTTON_SHORT_PRESS_MAX_MS) {
                    // Short press detected — show IP or status message
                    if (g_wifiManager.getState() == WiFiState::STA_CONNECTED) {
                        queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 5000);
                    } else {
                        queueDisplayMessage(String("No IP — Setup mode"), 5000);
                    }
                    LOG_I("Main", "Short press: showing IP/status");
                }
                // Long presses are ignored during normal operation
            }

            g_buttonLastState = raw;
        }
    }

    if (g_fetcher == nullptr)
    {
        delay(1000);
        return;
    }

    // Drive startup progress coordinator
    bool triggerFirstFetch = tickStartupProgress();

    // Determine if we should fetch now
    const unsigned long intervalMs = ConfigManager::getTiming().fetch_interval * 1000UL;
    const unsigned long now = millis();
    bool normalFetchDue = (now - g_lastFetchMs >= intervalMs);

    // First-fetch one-shot: when startup coordinator says go, fetch immediately
    // regardless of the normal interval timer
    if (triggerFirstFetch && !g_firstFetchDone)
    {
        normalFetchDue = true;
        LOG_I("Main", "Startup: triggering first fetch immediately");
    }

    if (normalFetchDue)
    {
        g_lastFetchMs = now;

        // Show "Fetching flights..." during startup progress
        if (g_startupPhase == StartupPhase::AUTHENTICATING ||
            g_startupPhase == StartupPhase::FETCHING_FLIGHTS)
        {
            if (g_startupPhase == StartupPhase::AUTHENTICATING)
            {
                enterPhase(StartupPhase::FETCHING_FLIGHTS);
                queueDisplayMessage(String("Fetching flights..."));
                LOG_I("Main", "Startup: fetching flights");
            }
        }

        std::vector<StateVector> states;
        std::vector<FlightInfo> flights;
        // Reset WDT before potentially long HTTP fetch (Story dl-1.4, AC #5)
        esp_task_wdt_reset();
        size_t enriched = g_fetcher->fetchFlights(states, flights);
        esp_task_wdt_reset();  // Reset after fetch completes

        // Update flight stats snapshot for /api/status (Story 2.4)
        g_statsFetchesSinceBoot.fetch_add(1);
        g_statsLastFetchMs.store(millis());
        g_statsStateVectors.store(static_cast<uint16_t>(states.size()));
        g_statsEnrichedFlights.store(static_cast<uint16_t>(enriched));
        // g_statsLogosMatched stays 0 until Epic 3

#if LOG_LEVEL >= 2
        Serial.println("[Main] Fetch: " + String((int)states.size()) + " state vectors, " + String((int)enriched) + " enriched flights");
#endif

#if LOG_LEVEL >= 3
        for (const auto &s : states)
        {
            Serial.println("[Main] " + s.callsign + " @ " + String(s.distance_km, 1) + "km bearing " + String(s.bearing_deg, 1));
        }

        for (const auto &f : flights)
        {
            Serial.println("[Main] Flight: " + f.ident + " " + f.airline_display_name_full + " " + f.origin.code_icao + "->" + f.destination.code_icao);
        }
#endif

        // Write flight data to double buffer and push pointer to queue
        g_flightBuf[g_writeBuf].flights = flights;
        FlightDisplayData *ptr = &g_flightBuf[g_writeBuf];
        if (g_flightQueue != nullptr)
        {
            xQueueOverwrite(g_flightQueue, &ptr);
        }
        g_writeBuf ^= 1; // swap buffer for next write

        // Update flight count for orchestrator idle fallback (Story dl-1.2, AC #3)
        // Stored immediately after queue publish — tied to the same snapshot
        // the display task consumes, avoiding drift.
        g_flightCount.store(static_cast<uint8_t>(flights.size() > 255 ? 255 : flights.size()));

        // Complete startup progress after first fetch
        if (!g_firstFetchDone &&
            (g_startupPhase == StartupPhase::FETCHING_FLIGHTS ||
             g_startupPhase == StartupPhase::AUTHENTICATING))
        {
            g_firstFetchDone = true;
            enterPhase(StartupPhase::COMPLETE);

            if (!flights.empty())
            {
                // Clear progress message so display task renders flight data
                queueDisplayMessage(String(""), 1);
                LOG_I("Main", "Startup complete: first flight data ready");
            }
            else
            {
                // No flights available — fall back to normal loading screen
                // (display task calls displayFallbackCard(emptyFlights) → displayLoadingScreen)
                queueDisplayMessage(String(""), 1);
                LOG_I("Main", "Startup complete: no flights yet, showing loading");
            }
        }
    }
}

#endif // PIO_UNIT_TESTING
