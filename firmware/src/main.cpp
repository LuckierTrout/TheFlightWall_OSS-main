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
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_sntp.h"
#include "utils/Log.h"
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

// Firmware version defined in platformio.ini build_flags
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"  // Fallback for IDE/testing
#endif

#ifndef PIO_UNIT_TESTING

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

// Public accessor for WebPortal and other modules (declared extern in consumers)
bool isNtpSynced() { return g_ntpSynced.load(); }

// SNTP sync notification callback — fires on LWIP task (Core 1) when time syncs
static void onNtpSync(struct timeval* tv) {
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
           lhs.zigzag != rhs.zigzag;
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
           lhs.zigzag != rhs.zigzag;
}

// --- Display task (Task 2) ---

void displayTask(void *pvParameters)
{
    LOG_I("DisplayTask", "Display task started");

    // Read initial config as local copies
    DisplayConfig localDisp = ConfigManager::getDisplay();
    HardwareConfig localHw = ConfigManager::getHardware();
    TimingConfig localTiming = ConfigManager::getTiming();

    // Flight cycling state (owned by display task)
    size_t currentFlightIndex = 0;
    unsigned long lastCycleMs = millis();
    bool statusMessageVisible = false;
    unsigned long statusMessageUntilMs = 0;

    // Subscribe to task watchdog
    esp_task_wdt_add(NULL);

    for (;;)
    {
        // Check for config changes (atomic flag from Core 1)
        if (g_configChanged.exchange(false))
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
            LOG_I("DisplayTask", "Config change detected, display settings updated");
        }

        // Calibration mode (Story 4.2): render test pattern instead of flights
        // Checked before status messages so test patterns override persistent banners
        if (g_display.isCalibrationMode())
        {
            statusMessageVisible = false;
            g_display.renderCalibrationPattern();
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // Positioning mode: render panel position guide instead of flights
        if (g_display.isPositioningMode())
        {
            statusMessageVisible = false;
            g_display.renderPositioningPattern();
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        DisplayStatusMessage statusMessage = {};
        if (g_displayMessageQueue != nullptr &&
            xQueueReceive(g_displayMessageQueue, &statusMessage, 0) == pdTRUE)
        {
            statusMessageVisible = true;
            statusMessageUntilMs = statusMessage.durationMs == 0
                ? 0
                : millis() + statusMessage.durationMs;
            g_display.displayMessage(String(statusMessage.text));
        }

        if (statusMessageVisible)
        {
            if (statusMessageUntilMs == 0 || millis() < statusMessageUntilMs)
            {
                esp_task_wdt_reset();
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            statusMessageVisible = false;
        }

        // Read latest flight data from queue (peek, don't remove)
        FlightDisplayData *ptr = nullptr;
        if (g_flightQueue != nullptr && xQueuePeek(g_flightQueue, &ptr, 0) == pdTRUE && ptr != nullptr)
        {
            const auto &flights = ptr->flights;

            if (!flights.empty())
            {
                const unsigned long now = millis();
                const unsigned long cycleMs = localTiming.display_cycle * 1000UL;

                if (flights.size() > 1)
                {
                    if (now - lastCycleMs >= cycleMs)
                    {
                        lastCycleMs = now;
                        currentFlightIndex = (currentFlightIndex + 1) % flights.size();
                    }
                }
                else
                {
                    currentFlightIndex = 0;
                }

                g_display.renderFlight(flights, currentFlightIndex % flights.size());
            }
            else
            {
                g_display.showLoading();
            }
        }

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
    ModeOrchestrator::init();

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
    g_display.showLoading();

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
    ConfigManager::onChange([]() {
        g_configChanged.store(true);
        g_layout = LayoutEngine::compute(ConfigManager::getHardware());

        // Timezone hot-reload (Story fn-2.1): re-apply POSIX TZ on any config change.
        // configTzTime() internally calls setenv("TZ",...) + tzset() + restarts SNTP.
        // Does NOT reset g_ntpSynced — only the SNTP callback controls that flag.
        ScheduleConfig sched = ConfigManager::getSchedule();
        configTzTime(sched.timezone.c_str(), "pool.ntp.org", "time.nist.gov");
#if LOG_LEVEL >= 2
        Serial.println("[Main] Timezone hot-reloaded: " + sched.timezone);
#endif
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
                // Show IP address for discovery (use phase time to gate, not a flag)
                if (elapsed >= 2000 && elapsed < 2100)
                {
                    queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 2000);
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

// --- loop() (Task 4) ---

void loop()
{
    ConfigManager::tick();
    g_wifiManager.tick();

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
        size_t enriched = g_fetcher->fetchFlights(states, flights);

        // Update flight stats snapshot for /api/status (Story 2.4)
        g_statsFetchesSinceBoot.fetch_add(1);
        g_statsLastFetchMs.store(millis());
        g_statsStateVectors.store(static_cast<uint16_t>(states.size()));
        g_statsEnrichedFlights.store(static_cast<uint16_t>(enriched));
        // g_statsLogosMatched stays 0 until Epic 3

        // Tick orchestrator for idle fallback logic (Story dl-1.5)
        ModeOrchestrator::tick(static_cast<uint8_t>(flights.size() > 255 ? 255 : flights.size()));

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
                // (display task will show showLoading() when flight list is empty)
                queueDisplayMessage(String(""), 1);
                LOG_I("Main", "Startup complete: no flights yet, showing loading");
            }
        }
    }
}

#endif // PIO_UNIT_TESTING
