<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-2 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T153346Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-2.2

You are synthesizing 3 independent code review findings.

Your mission:
1. VERIFY each issue raised by reviewers
   - Cross-reference with project_context.md (ground truth)
   - Cross-reference with git diff and source files
   - Identify false positives (issues that aren't real problems)
   - Confirm valid issues with evidence

2. PRIORITIZE real issues by severity
   - Critical: Security vulnerabilities, data corruption risks
   - High: Bugs, logic errors, missing error handling
   - Medium: Code quality issues, performance concerns
   - Low: Style issues, minor improvements

3. SYNTHESIZE findings
   - Merge duplicate issues from different reviewers
   - Note reviewer consensus (if 3+ agree, high confidence)
   - Highlight unique insights from individual reviewers

4. APPLY source code fixes
   - You have WRITE PERMISSION to modify SOURCE CODE files
   - CRITICAL: Before using Edit tool, ALWAYS Read the target file first
   - Use EXACT content from Read tool output as old_string, NOT content from this prompt
   - If Read output is truncated, use offset/limit parameters to locate the target section
   - Apply fixes for verified issues
   - Do NOT modify the story file (only Dev Agent Record if needed)
   - Document what you changed and why

Output format:
## Synthesis Summary
## Issues Verified (by severity)
## Issues Dismissed (false positives with reasoning)
## Source Code Fixes Applied

]]></mission>
<context>
<file id="ed7fe483" path="_bmad-output/project-context.md" label="PROJECT CONTEXT"><![CDATA[

---
project_name: TheFlightWall_OSS-main
date: '2026-04-12'
---

# Project Context for AI Agents

Lean rules for implementing FlightWall (ESP32 LED flight display + captive-portal web UI). Prefer existing patterns in `firmware/` over new abstractions.

## Technology Stack

- **Firmware:** C++11, ESP32 (Arduino/PlatformIO), FastLED + Adafruit GFX + FastLED NeoMatrix, ArduinoJson ^7.4.2.
- **Web on device:** ESPAsyncWebServer (**mathieucarbou fork**), AsyncTCP (**Carbou fork**), LittleFS (`board_build.filesystem = littlefs`), custom `custom_partitions.csv` (~2MB app + ~2MB LittleFS).
- **Dashboard assets:** Editable sources under `firmware/data-src/`; served bundles are **gzip** under `firmware/data/`. After editing a source file, regenerate the matching `.gz` from `firmware/` (e.g. `gzip -9 -c data-src/common.js > data/common.js.gz`).

## Critical Implementation Rules

- **Core pinning:** Display/task driving LEDs on **Core 0**; WiFi, HTTP server, and flight fetch pipeline on **Core 1** (FastLED + WiFi ISR constraints).
- **Config:** `ConfigManager` + NVS; debounce writes; atomic saves; use category getters; `POST /api/settings` JSON envelope `{ ok, data, error, code }` pattern for REST responses.
- **Heap / concurrency:** Cap concurrent web clients (~2–3); stream LittleFS reads; use ArduinoJson filter/streaming for large JSON; avoid full-file RAM buffering for uploads.
- **WiFi:** WiFiManager-style state machine (AP setup → STA → reconnect / AP fallback); mDNS `flightwall.local` in STA.
- **Structure:** Extend hexagonal layout — `firmware/core/`, `firmware/adapters/` (e.g. `WebPortal.cpp`), `firmware/interfaces/`, `firmware/models/`, `firmware/config/`, `firmware/utils/`.
- **Tooling:** Build from `firmware/` with `pio run`. On macOS serial: use `/dev/cu.*` (not `tty.*`); release serial monitor before upload.
- **Scope for code reviews:** Product code under `firmware/` and tests under `firmware/test/` and repo `tests/`; do not treat BMAD-only paths as product defects unless the task says so.

## Planning Artifacts

- Requirements and design: `_bmad-output/planning-artifacts/` (`architecture.md`, `epics.md`, PRDs).
- Stories and sprint line items: `_bmad-output/implementation-artifacts/` (e.g. `sprint-status.yaml`, per-story markdown).


]]></file>
<file id="89294d9b" path="_bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md" label="DOCUMENTATION"><![CDATA[

# Story fn-2.2: Night Mode Brightness Scheduler

Status: ready-for-review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As the **device owner**,
I want the ESP32 to automatically dim the LED display during a scheduled time window (e.g., nighttime),
so that the display does not disturb sleep or waste energy during hours when I am not actively watching it.

## Acceptance Criteria

1. **Scheduler Runs Non-Blocking in Main Loop**
   - Given: `loop()` executing on Core 1
   - When: ~1 second has elapsed since the last scheduler check
   - Then: scheduler reads current local time via `getLocalTime(&now, 0)` with timeout=0 (Enforcement Rule #14)
   - And: scheduler does NOT block the main loop or delay other loop() operations (WiFi tick, fetch cycle, OTA self-check)

2. **Dim Window Same-Day (start < end)**
   - Given: `sched_enabled == 1`, `g_ntpSynced == true`, `sched_dim_start = 1320` (22:00), `sched_dim_end = 420` (07:00) — wait, this crosses midnight. Better example: `sched_dim_start = 480` (08:00), `sched_dim_end = 1020` (17:00)
   - When: current local time is 12:00 (720 minutes since midnight)
   - Then: scheduler determines current time IS within the dim window
   - And: display brightness overridden to `sched_dim_brt`

3. **Dim Window Crosses Midnight (start > end)**
   - Given: `sched_enabled == 1`, `g_ntpSynced == true`, `sched_dim_start = 1380` (23:00), `sched_dim_end = 420` (07:00)
   - When: current local time is 02:00 (120 minutes since midnight)
   - Then: scheduler determines current time IS within the dim window (midnight-crossing logic)
   - And: display brightness overridden to `sched_dim_brt`
   - Also When: current local time is 12:00 (720 minutes since midnight)
   - Then: scheduler determines current time is NOT within the dim window
   - And: display brightness restored to `ConfigManager::getDisplay().brightness`

4. **NTP Not Synced Disables Scheduler**
   - Given: `sched_enabled == 1` but `g_ntpSynced == false`
   - When: scheduler check runs
   - Then: scheduler skips time comparison entirely (no `getLocalTime` call)
   - And: display brightness remains at the user's configured brightness (no override)
   - And: no log spam — log only on state transition (NTP lost/regained)

5. **Schedule Disabled Restores Brightness**
   - Given: scheduler was previously overriding brightness (in dim window)
   - When: `sched_enabled` changed to `0` via `POST /api/settings`
   - Then: on next scheduler check (~1s), brightness restored to `ConfigManager::getDisplay().brightness`
   - And: override flag cleared so no stale dim state persists

6. **Manual Brightness Change During Dim Window**
   - Given: scheduler is currently overriding brightness to `sched_dim_brt`
   - When: user changes `brightness` via `POST /api/settings` (dashboard slider)
   - Then: the new manual brightness is applied momentarily
   - But: on the next scheduler check (~1s), if still in dim window, scheduler re-overrides to `sched_dim_brt`
   - Note: this is the intended behavior per Architecture Decision F5 — schedule takes priority over manual brightness during dim window

7. **Config Hot-Reload Updates Scheduler**
   - Given: scheduler is running
   - When: any schedule key (`sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt`) changed via `POST /api/settings`
   - Then: on next scheduler check (~1s), new config values are used immediately
   - And: no reboot required (all schedule keys are hot-reload)

8. **WebPortal /api/status Reflects Scheduler State**
   - Given: `GET /api/status` called
   - When: response built
   - Then: `schedule_active` field reflects whether the scheduler is currently overriding brightness (not just whether it is enabled)
   - And: new field `schedule_dimming` = `true` when scheduler is actively in the dim window and overriding brightness

## Tasks / Subtasks

- [x] **Task 1: Add scheduler state variables and helper function in main.cpp** (AC: #1, #4, #5)
  - [x] 1.1 Add scheduler state variables after the existing `g_ntpSynced` declaration (~line 67):
    ```cpp
    // Night mode scheduler state (Story fn-2.2)
    static unsigned long g_lastSchedCheckMs = 0;
    static std::atomic<bool> g_schedDimming(false);    // true when scheduler is actively overriding brightness
    static std::atomic<bool> g_scheduleChanged(false); // dedicated flag for schedule state transitions (Core 1→Core 0)
    static bool g_lastNtpSynced = false;               // for NTP state transition logging (AC #4)
    static constexpr unsigned long SCHED_CHECK_INTERVAL_MS = 1000;
    static_assert(SCHED_CHECK_INTERVAL_MS <= 2000,
        "SCHED_CHECK_INTERVAL_MS must be <=2s to meet 1-second display response NFR");
    ```
  - [x] 1.2 Add public accessor for WebPortal (~after `isNtpSynced()` at line 70):
    ```cpp
    bool isScheduleDimming() { return g_schedDimming.load(); }
    ```
  - [x] 1.3 Add the `tickNightScheduler()` function before `loop()` (~line 860, before the `loop()` function):
    ```cpp
    // Night mode brightness scheduler (Story fn-2.2, Architecture Decision F5)
    // Runs ~1/sec in Core 1 main loop. Non-blocking per Enforcement Rule #14.
    // Uses minutes-since-midnight comparison per Enforcement Rule #15.
    static void tickNightScheduler() {
        const unsigned long now = millis();
        if (now - g_lastSchedCheckMs < SCHED_CHECK_INTERVAL_MS) return;
        g_lastSchedCheckMs = now;

        ScheduleConfig sched = ConfigManager::getSchedule();
        bool wasDimming = g_schedDimming.load();  // Capture previous state for transition detection
        bool ntpSynced  = g_ntpSynced.load();

        // Log NTP state transitions (AC #4: log only on lost/regained, not every tick)
        if (ntpSynced && !g_lastNtpSynced) {
            LOG_I("Scheduler", "NTP synced — schedule active");
        } else if (!ntpSynced && g_lastNtpSynced) {
            LOG_I("Scheduler", "NTP lost — schedule suspended");
        }
        g_lastNtpSynced = ntpSynced;

        // Guard: NTP must be synced and schedule must be enabled
        if (sched.sched_enabled != 1 || !ntpSynced) {
            if (wasDimming) {
                // Was dimming but schedule disabled or NTP lost — restore brightness
                g_schedDimming.store(false);
                g_scheduleChanged.store(true);  // Signal display task via dedicated flag
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
        uint16_t dimStart = sched.sched_dim_start;
        uint16_t dimEnd = sched.sched_dim_end;

        // Determine if current time is within the dim window
        bool inDimWindow;
        if (dimStart <= dimEnd) {
            // Same-day window: e.g., 08:00 - 17:00
            inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
        } else {
            // Midnight-crossing window: e.g., 23:00 - 07:00
            inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
        }

        // Only signal on actual state transitions — do NOT re-assert every tick.
        // AC #6 (re-override manual brightness) is handled by the display task's
        // config-change handler: it checks g_schedDimming.load() after every
        // g_configChanged or g_scheduleChanged, so any API brightness write
        // immediately re-applies the dim override without waiting for the next tick.
        if (inDimWindow && !wasDimming) {
            g_schedDimming.store(true);
            g_scheduleChanged.store(true);  // Signal display task
            LOG_I("Scheduler", "Entering dim window — brightness override active");
        } else if (!inDimWindow && wasDimming) {
            g_schedDimming.store(false);
            g_scheduleChanged.store(true);  // Signal display task
            LOG_I("Scheduler", "Exiting dim window — brightness restored");
        }
    }
    ```

- [x] **Task 2: Integrate scheduler into loop()** (AC: #1, #7)
  - [x] 2.1 In `loop()` (line 862), add `tickNightScheduler()` call after `g_wifiManager.tick()` and before the OTA self-check block:
    ```cpp
    void loop()
    {
        ConfigManager::tick();
        g_wifiManager.tick();

        // Night mode brightness scheduler (Story fn-2.2)
        tickNightScheduler();

        // OTA self-check: mark firmware valid once WiFi connects or timeout expires (Story fn-1.4)
        ...
    ```
    Insert at line 866, between `g_wifiManager.tick();` (line 865) and the OTA self-check comment (line 867).

- [x] **Task 3: Modify display task to apply scheduler brightness override** (AC: #2, #3, #5, #6)
  - [x] 3.1 In the `displayTask()` function's config-change handler (line 314-346), expand the condition to also check `g_scheduleChanged` and add scheduler override logic after `g_display.updateBrightness(localDisp.brightness);` (line 344):
    ```cpp
    // Check both config changes AND dedicated schedule state transitions (Story fn-2.2)
    if (g_configChanged.exchange(false) || g_scheduleChanged.exchange(false)) {
        DisplayConfig newDisp = ConfigManager::getDisplay();
        // ... existing hardware change checks ...
        localDisp = newDisp;
        g_display.updateBrightness(localDisp.brightness);

        // Apply scheduler brightness override if active (Story fn-2.2)
        // g_schedDimming is std::atomic<bool> — use .load() for proper memory ordering.
        // This also handles AC #6: any manual brightness API change triggers g_configChanged,
        // causing this handler to run and immediately re-apply the dim override if still in window.
        if (g_schedDimming.load()) {
            ScheduleConfig sched = ConfigManager::getSchedule();
            g_display.updateBrightness(sched.sched_dim_brt);
        }
    }
    ```
  - [x] 3.2 `g_schedDimming` is declared as `std::atomic<bool>` (see Task 1.1). The dedicated `g_scheduleChanged` atomic flag ensures schedule state transitions (enter/exit dim window) are never lost due to a concurrent `g_configChanged` consumption by the display task. Both flags are checked with `||` in the same `exchange(false)` condition, so either trigger causes the display task to re-evaluate brightness.

- [x] **Task 4: Update WebPortal /api/status with scheduler dimming state** (AC: #8)
  - [x] 4.1 In `WebPortal.cpp`, add `extern bool isScheduleDimming();` declaration near the existing `extern bool isNtpSynced();` (line 40):
    ```cpp
    // Defined in main.cpp — Night mode scheduler dimming state (Story fn-2.2)
    extern bool isScheduleDimming();
    ```
  - [x] 4.2 In `_handleGetStatus()` (line 787), update `schedule_active` semantics and add `schedule_dimming` field:
    ```cpp
    // NTP and schedule status (Story fn-2.1, fn-2.2)
    data["ntp_synced"] = isNtpSynced();
    data["schedule_active"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();
    data["schedule_dimming"] = isScheduleDimming();
    ```
    This addresses the fn-2.1 review follow-up: `schedule_active` keeps its current meaning (enabled + NTP synced = scheduler can run), while `schedule_dimming` tells the dashboard whether the scheduler is currently overriding brightness.

- [x] **Task 5: Unit tests** (AC: #1, #2, #3, #4, #5)
  - [x] 5.1 In `firmware/test/test_config_manager/test_main.cpp`, add new test cases at end of file:
    ```cpp
    // --- Night mode scheduler logic tests (Story fn-2.2) ---

    void test_schedule_dim_window_same_day() {
        // dimStart=480 (08:00), dimEnd=1020 (17:00)
        // nowMinutes=720 (12:00) → IN window
        uint16_t dimStart = 480, dimEnd = 1020, nowMinutes = 720;
        bool inDimWindow;
        if (dimStart <= dimEnd) {
            inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
        } else {
            inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
        }
        TEST_ASSERT_TRUE(inDimWindow);

        // nowMinutes=1200 (20:00) → NOT in window
        nowMinutes = 1200;
        if (dimStart <= dimEnd) {
            inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
        } else {
            inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
        }
        TEST_ASSERT_FALSE(inDimWindow);
    }

    void test_schedule_dim_window_crosses_midnight() {
        // dimStart=1380 (23:00), dimEnd=420 (07:00)
        uint16_t dimStart = 1380, dimEnd = 420;

        // nowMinutes=120 (02:00) → IN window (after midnight)
        uint16_t nowMinutes = 120;
        bool inDimWindow;
        if (dimStart <= dimEnd) {
            inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
        } else {
            inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
        }
        TEST_ASSERT_TRUE(inDimWindow);

        // nowMinutes=1400 (23:20) → IN window (before midnight)
        nowMinutes = 1400;
        if (dimStart <= dimEnd) {
            inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
        } else {
            inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
        }
        TEST_ASSERT_TRUE(inDimWindow);

        // nowMinutes=720 (12:00) → NOT in window
        nowMinutes = 720;
        if (dimStart <= dimEnd) {
            inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
        } else {
            inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
        }
        TEST_ASSERT_FALSE(inDimWindow);
    }

    void test_schedule_dim_window_edge_cases() {
        // dimStart == dimEnd (zero-length window) → never in window
        uint16_t dimStart = 600, dimEnd = 600, nowMinutes = 600;
        bool inDimWindow;
        if (dimStart <= dimEnd) {
            inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
        } else {
            inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
        }
        TEST_ASSERT_FALSE(inDimWindow);

        // Exactly at dimStart → IN window (inclusive start)
        dimStart = 480; dimEnd = 1020; nowMinutes = 480;
        if (dimStart <= dimEnd) {
            inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
        } else {
            inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
        }
        TEST_ASSERT_TRUE(inDimWindow);

        // Exactly at dimEnd → NOT in window (exclusive end)
        nowMinutes = 1020;
        if (dimStart <= dimEnd) {
            inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
        } else {
            inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
        }
        TEST_ASSERT_FALSE(inDimWindow);
    }

    void test_schedule_default_dim_brightness() {
        // Default schedule config should have dim brightness of 10
        ScheduleConfig sched = ConfigManager::getSchedule();
        TEST_ASSERT_EQUAL_UINT8(10, sched.sched_dim_brt);
    }
    ```
  - [x] 5.2 Register the new test functions in the test runner `RUN_TEST()` block at end of `main()`:
    ```cpp
    RUN_TEST(test_schedule_dim_window_same_day);
    RUN_TEST(test_schedule_dim_window_crosses_midnight);
    RUN_TEST(test_schedule_dim_window_edge_cases);
    RUN_TEST(test_schedule_default_dim_brightness);
    ```

## Dev Notes

### Architecture Constraints (MUST FOLLOW)

- **Architecture Decision F5:** Night mode scheduler runs in Core 1 main loop at ~1/sec interval. Non-blocking. Uses `getLocalTime(&now, 0)` with timeout=0 per Enforcement Rule #14.
- **Enforcement Rule #14:** All `getLocalTime()` calls MUST use timeout=0. Never block on time reads.
- **Enforcement Rule #15:** Schedule times stored as `uint16_t` minutes since midnight (0-1439). Use integer arithmetic only — no floating-point time math.
- **Enforcement Rule #30:** Cross-core atomics live in `main.cpp` only. `g_schedDimming` (`std::atomic<bool>`) is written on Core 1 and read on Core 0. `g_scheduleChanged` (`std::atomic<bool>`) is the dedicated signal flag for schedule state transitions, keeping it separate from `g_configChanged` to prevent flag consumption races.
- **Core pinning:** Scheduler runs on Core 1 (loop task). Display task runs on Core 0. Communication via `g_configChanged` (config changes) and `g_scheduleChanged` (schedule state transitions) atomic flags + ConfigManager getters (mutex-protected).
- **No new files:** All modifications to existing files only. No new header files or source files.
- **Logging:** Use `LOG_I`/`LOG_E`/`LOG_V` macros from `utils/Log.h`. Log only on state transitions (enter/exit dim window) to avoid log spam.

### Midnight-Crossing Logic Pattern

```
Minutes-since-midnight comparison (Rule #15):

if (dimStart <= dimEnd) {
    // Same-day window: e.g., 08:00 (480) to 17:00 (1020)
    inDimWindow = (now >= dimStart && now < dimEnd);
} else {
    // Crosses midnight: e.g., 23:00 (1380) to 07:00 (420)
    inDimWindow = (now >= dimStart || now < dimEnd);
}
```

The `dimStart <= dimEnd` check covers the same-day case naturally. The `dimStart > dimEnd` case inverts to an OR condition because the "inside" region wraps around midnight. Start is inclusive (`>=`), end is exclusive (`<`).

### Brightness Override Mechanism

The scheduler does NOT modify `ConfigManager::_display.brightness`. Instead:
1. Scheduler sets `g_schedDimming.store(true)` and signals the display task via `g_scheduleChanged.store(true)`
2. Display task's config-change handler triggers on `g_configChanged.exchange(false) || g_scheduleChanged.exchange(false)`
3. Handler reads `localDisp.brightness` as normal, then checks `g_schedDimming.load()` — if true, overrides with `sched.sched_dim_brt`
4. This means: ConfigManager always holds the user's "intended" brightness, and the scheduler is a runtime overlay

This design ensures:
- User's brightness preference is never lost
- Exiting the dim window cleanly restores the user's brightness
- No NVS writes for transient brightness overrides
- Manual brightness changes via API persist to NVS as normal; because they trigger `g_configChanged`, the display task's handler runs immediately and re-applies the dim override (not "on the next scheduler tick" — it's immediate, which is better than AC#6 describes)
- Schedule state transitions are never lost to a concurrent `g_configChanged` consumption (dedicated `g_scheduleChanged` flag prevents this race)

### Interaction with ModeOrchestrator

`ModeOrchestrator::tick()` is called at line 953 inside the fetch block (runs once per fetch interval). The night scheduler runs independently at ~1/sec in the main loop body. They do not conflict — ModeOrchestrator controls display mode (flight rendering vs idle), while the night scheduler only controls brightness level.

### Source Files to Modify

| File | Change | Lines |
|------|--------|-------|
| `firmware/src/main.cpp` | Add scheduler state vars, `isScheduleDimming()` accessor, `tickNightScheduler()` function, call in loop(), display task override | ~67, ~70, ~860, ~866, ~344 |
| `firmware/adapters/WebPortal.cpp` | Add `extern isScheduleDimming()`, add `schedule_dimming` to /api/status | ~40, ~787 |
| `firmware/test/test_config_manager/test_main.cpp` | Add 4 unit tests for dim window logic | End of file |

### Existing Code Patterns to Follow

**Global state pattern (main.cpp ~line 63-67):**
```cpp
static std::atomic<bool> g_configChanged(false);
static std::atomic<bool> g_ntpSynced(false);
// ADD alongside:
static unsigned long g_lastSchedCheckMs = 0;
static std::atomic<bool> g_schedDimming(false);    // std::atomic for safe cross-core read on Core 0
static std::atomic<bool> g_scheduleChanged(false); // dedicated schedule state transition signal
static bool g_lastNtpSynced = false;               // NTP transition logging (Core 1 only — no atomic needed)
```

**Display task config-change handler (main.cpp ~line 314-345):**
```cpp
if (g_configChanged.exchange(false) || g_scheduleChanged.exchange(false)) {
    DisplayConfig newDisp = ConfigManager::getDisplay();
    // ... hardware change checks ...
    localDisp = newDisp;
    g_display.updateBrightness(localDisp.brightness);
    // ADD AFTER: scheduler override
    if (g_schedDimming.load()) {
        ScheduleConfig sched = ConfigManager::getSchedule();
        g_display.updateBrightness(sched.sched_dim_brt);
    }
}
```

**WebPortal extern accessor pattern (WebPortal.cpp ~line 40):**
```cpp
extern bool isNtpSynced();
// ADD:
extern bool isScheduleDimming();
```

### Existing Infrastructure Already in Place

- `ScheduleConfig` struct with all 5 fields — ConfigManager.h lines 47-53
- `ConfigManager::getSchedule()` getter with mutex guard — ConfigManager.cpp line 560
- `g_ntpSynced` atomic flag and `isNtpSynced()` accessor — main.cpp lines 67, 70
- `g_configChanged` atomic flag for Core 1→Core 0 signaling — main.cpp line 63
- All 5 schedule keys are hot-reload (NOT in REBOOT_KEYS) — ConfigManager.cpp line 206
- NTP sync via SNTP callback — main.cpp lines 75-82
- Timezone hot-reload in onChange callback — main.cpp lines 654-672
- `/api/status` already includes `ntp_synced` and `schedule_active` — WebPortal.cpp line 786-787

### What This Story Does NOT Include

- Dashboard Night Mode UI card (fn-2.3)
- Timezone dropdown in dashboard (fn-2.3)
- Gradual brightness transitions / fade effects (not in scope)
- User notification when scheduler overrides brightness (fn-2.3 UI will show this)
- Sunrise/sunset-based scheduling (not in scope — uses fixed times)

### Dependencies

| Dependency | Status | Notes |
|------------|--------|-------|
| fn-2.1 (NTP Time Sync) | DONE | Provides `g_ntpSynced`, `isNtpSynced()`, timezone config, SNTP callback |
| fn-1.2 (ConfigManager expansion) | DONE | ScheduleConfig struct, all 5 NVS keys, validation |
| ConfigManager onChange | DONE | Hot-reload callback mechanism for schedule keys |
| SystemStatus NTP subsystem | DONE | NTP sync state tracking |

### fn-2.1 Review Follow-ups Addressed

- **`schedule_active` semantics rename** (fn-2.1 review LOW): Resolved by keeping `schedule_active` as-is (enabled + NTP synced) and adding new `schedule_dimming` field for actual override state. This is a non-breaking change.

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#F5] — Night Mode scheduler: Core 1 main loop, ~1/sec, non-blocking
- [Source: _bmad-output/planning-artifacts/architecture.md#F6] — ConfigManager expansion, ScheduleConfig struct
- [Source: _bmad-output/planning-artifacts/prd.md#FR40] — Auto-maintain API auth
- [Source: _bmad-output/planning-artifacts/prd.md#NFR-Reliability] — 30+ day continuous operation
- [Source: firmware/core/ConfigManager.h:47-53] — ScheduleConfig struct definition
- [Source: firmware/src/main.cpp:63] — g_configChanged atomic pattern
- [Source: firmware/src/main.cpp:67] — g_ntpSynced atomic flag
- [Source: firmware/src/main.cpp:70] — isNtpSynced() accessor
- [Source: firmware/src/main.cpp:314-346] — Display task config-change handler
- [Source: firmware/src/main.cpp:654-672] — ConfigManager onChange callback with timezone hot-reload
- [Source: firmware/src/main.cpp:862-1003] — loop() function
- [Source: firmware/adapters/WebPortal.cpp:40] — extern isNtpSynced() declaration
- [Source: firmware/adapters/WebPortal.cpp:785-787] — /api/status NTP and schedule fields
- [Source: firmware/core/ConfigManager.cpp:134-172] — Schedule key validation in updateConfigValue
- [Source: firmware/core/ConfigManager.cpp:206] — REBOOT_KEYS array (schedule keys NOT included)
- [Source: _bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md] — Predecessor story, patterns established

## Dev Agent Record

### Implementation Plan
- Implement non-blocking brightness scheduler in Core 1 main loop (~1/sec tick)
- Use dedicated `g_scheduleChanged` atomic flag (separate from `g_configChanged`) to signal display task on Core 0
- Display task applies dim brightness override when `g_schedDimming` is true, restoring user brightness otherwise
- Expose `schedule_dimming` field in `/api/status` for dashboard consumption
- Unit tests validate dim window arithmetic (same-day, midnight-crossing, edge cases, defaults)

### Completion Notes
All 5 tasks implemented and verified:
- **Task 1:** Scheduler state variables (`g_schedDimming`, `g_scheduleChanged`, `g_lastNtpSynced`, `SCHED_CHECK_INTERVAL_MS` with `static_assert`), `isScheduleDimming()` accessor, and `tickNightScheduler()` function in main.cpp
- **Task 2:** `tickNightScheduler()` integrated into `loop()` after `g_wifiManager.tick()` (line 955)
- **Task 3:** Display task config-change handler expanded to check `g_scheduleChanged` alongside `g_configChanged`, with dim brightness override via `g_schedDimming.load()` (lines 324-367)
- **Task 4:** `extern bool isScheduleDimming()` in WebPortal.cpp (line 43), `schedule_dimming` field in `/api/status` (line 791)
- **Task 5:** 4 unit tests (`test_schedule_dim_window_same_day`, `_crosses_midnight`, `_edge_cases`, `_default_dim_brightness`) with `isInDimWindow()` helper function and registered in test runner

**Review fixes applied:**
- ✅ HIGH: Added `isScheduleDimming()` extern + `schedule_dimming` to `/api/status` (was missing)
- ✅ MEDIUM: Added NVS `loadFromNvs()` validation for schedule keys (sched_enabled ≤1, sched_dim_start/end ≤1439)
- ✅ LOW: Added `static_assert(SCHED_CHECK_INTERVAL_MS <= 2000, ...)` guard

**Build verified:** Firmware compiles successfully (1.17 MB / 1.50 MB, 78.3% usage)

## File List

| File | Action |
|------|--------|
| `firmware/src/main.cpp` | Modified — scheduler state vars, `isScheduleDimming()`, `tickNightScheduler()`, loop integration, display task override |
| `firmware/adapters/WebPortal.cpp` | Modified — `extern isScheduleDimming()`, `schedule_dimming` in `/api/status` |
| `firmware/core/ConfigManager.cpp` | Modified — NVS `loadFromNvs()` validation for schedule keys (review fix) |
| `firmware/test/test_config_manager/test_main.cpp` | Modified — 4 new unit tests for dim window logic |

## Change Log

| Date | Change |
|------|--------|
| 2026-04-13 | Implemented night mode brightness scheduler (Tasks 1-5), all ACs satisfied |
| 2026-04-13 | Applied code review fixes: WebPortal schedule_dimming field (HIGH), NVS validation (MEDIUM), static_assert (LOW) |

## Senior Developer Review (AI)

### Review: 2026-04-13
- **Reviewer:** AI Code Review Synthesis (3 validators)
- **Evidence Score:** 5.2 / 5.8 / 3.8 → MAJOR REWORK (pre-fix); APPROVED (post-fix)
- **Issues Found:** 4 verified (1 High, 1 Medium, 2 Low)
- **Issues Fixed:** 3 (all critical/high/medium + 1 low)
- **Action Items Created:** 0 (all verified issues resolved; deferred items logged as future improvements)


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic fn-2 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `onNtpSync()` sets `g_ntpSynced=true` unconditionally — but `sntp_set_time_sync_notification_cb` can fire on non-COMPLETED events (e.g., `SNTP_SYNC_STATUS_RESET` during network changes). Without a status check, the flag could flip to `true` spuriously. | Added `sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED` early-return guard ✅ Applied |
| high | `configTzTime()` called on **every** config write in the `onChange` lambda — including unrelated writes (brightness, fetch_interval, text colors, etc.). This restarts SNTP and discards in-flight NTP requests on hot paths. | Added `static String s_lastAppliedTz` cache inside the lambda; `configTzTime()` only fires when the timezone value actually changes ✅ Applied |
| medium | `TZ_MAP` with ~75 entries is copy-pasted verbatim into both `dashboard.js` and `wizard.js`. Any DST rule correction requires dual edits and dual `gzip` regenerations. | Deferred to fn-2.3 (timezone dropdown UI) — moving to `common.js` must be validated against wizard page's script loading order; project "no new files" rule for this story blocks immediate action. Follow-up task created. |
| low | `schedule_active` in `/api/status` is computed as `sched_enabled && ntp_synced`, not "currently inside the dim window". The field name implies real-time window state but delivers config+sync state. | Deferred to fn-2.2 (Night Mode scheduler) where the actual in-window calculation will be implemented. Follow-up task created. |
| low | `getTimezoneConfig()` silently falls back to `"UTC0"` for IANA zones not in `TZ_MAP` (e.g., `Asia/Manila`). The caller gets no signal that the zone was unmapped. | Deferred to fn-2.3 — no UI element currently consumes `getTimezoneConfig()`. Follow-up task created. |
| dismissed | `extern bool isNtpSynced()` in `WebPortal.cpp` breaks hexagonal architecture | FALSE POSITIVE: FALSE POSITIVE. The file already declares `extern FlightStatsSnapshot getFlightStatsSnapshot()` at line 37 using the identical pattern. The story's Dev Notes explicitly prescribe this approach ("aligns with project rule: no new files"). This is the established project convention, not a new violation. |
| dismissed | Race condition between `tzset()` (from `configTzTime()`) and `localtime_r()` across Core 0/Core 1 | FALSE POSITIVE: Theoretical but low-impact. Hot-reload is a rare user-triggered action; the `tzset()` modification window is microseconds; the worst-case consequence is a single garbled time display frame during a settings write. Accepted design risk per story scope. A mutex solution would require significant infrastructure not justified by this failure mode. |
| dismissed | Missing POSIX TZ format validation in ConfigManager | FALSE POSITIVE: Partially valid concern, but implemented validation (non-empty string ≤40 chars) is sufficient for the story AC. A malformed TZ string passed to `configTzTime()` → `tzset()` causes silent UTC fallback — the device stays functional and the LWIP stack doesn't crash. Full POSIX TZ syntax validation is non-trivial on ESP32 newlib and is outside story scope. Accepted risk. |
| dismissed | `g_ntpSynced` never resets to `false` after initial sync (stale success on WiFi reconnect) | FALSE POSITIVE: Out-of-story-scope design concern. The story ACs define `ntpSynced=false` only as the pre-sync default (AC #3). LWIP auto-retries maintain clock accuracy. The flag is correctly "sticky" for the current story's contract. If cross-story NTP state invalidation is needed (e.g., for fn-2.2 scheduling), that story should define the semantics. |
| dismissed | Raw `Serial.println` violates logging contract (should use `LOG_I`) | FALSE POSITIVE: Acknowledged in the Dev Agent Record as a known technical constraint. `LOG_I` is a 2-argument macro (tag + string literal only); it cannot format dynamic runtime strings. The `#if LOG_LEVEL >= 2` guard follows the pre-existing pattern already in `WiFiManager.cpp:140`. Not a new violation. |
| dismissed | Unit tests are "lying" — `test_ntp_status_in_json_output` doesn't test the actual `/api/status` HTTP endpoint | FALSE POSITIVE: Valid quality observation but not a "lying test". The test name and inline comment accurately describe what it tests (`SystemStatus::toJson()` NTP fields). Full HTTP endpoint testing is not achievable in Unity/embedded unit tests without a host simulator. The story explicitly notes "unit tests cover callback logic only" (AC #1). Coverage gap is real but architectural, not deceptive. --- |

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| dismissed | `g_ntpSynced` never resets to `false` after WiFi reconnect where NTP fails (Validators B, C) | FALSE POSITIVE: Already adjudicated as out-of-scope in the Round 1 antipatterns doc. AC #3 defines `ntpSynced=false` only as the pre-first-sync default. The "sticky true" design is intentional for this story contract; WiFi-reconnect NTP invalidation is a concern for a future story defining that semantic. |
| dismissed | AC #2 is "only half-implemented" — `getLocalTime()` not called in callback (Validator B) | FALSE POSITIVE: The AC states `getLocalTime()` *returns correct time* — this is a functional postcondition, not a requirement for the callback to call it. `configTzTime()` + SNTP completion makes `getLocalTime()` correct by design. The callback's job is flag + status, which is implemented. |
| dismissed | `test_ntp_status_in_json_output` is a lying test for AC #6 (Validators B, C) | FALSE POSITIVE: Already adjudicated in Round 1 antipatterns doc. Full HTTP endpoint testing is not achievable in Unity/embedded unit tests without a host simulator. The test name accurately describes what it tests (`SystemStatus::toJson()` NTP fields); the story AC #1 explicitly scopes: "unit tests cover callback logic only." |
| dismissed | `schedule_active` field is semantically misleading (Validators B, C) | FALSE POSITIVE: Already tracked as a deferred follow-up from Round 1. Will be corrected in fn-2.2 (Night Mode scheduler) when real in-window logic lands. |
| dismissed | `extern bool isNtpSynced()` in `WebPortal.cpp` breaks hexagonal architecture (Validator C) | FALSE POSITIVE: Already dismissed as FALSE POSITIVE in Round 1 antipatterns doc. `WebPortal.cpp:37` already uses the identical `extern FlightStatsSnapshot getFlightStatsSnapshot()` pattern — this is the established project convention, not a new violation. |
| dismissed | `TZ_MAP` duplication and silent UTC fallback in `getTimezoneConfig()` (Validators B, C) | FALSE POSITIVE: Already tracked as deferred follow-ups from Round 1. Deferred to fn-2.3 when the timezone dropdown UI is built and `common.js` sharing can be validated against wizard page script-load order. --- |

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `test_ntp_status_in_json_output` calls `SystemStatus::toJson()` but production `WebPortal::_handleGetStatus()` calls `SystemStatus::toExtendedJson()`. The NTP subsystem lives at `data["subsystems"]["ntp"]` in production output (due to `toExtendedJson`'s nesting), not `data["ntp"]` as the test asserted. Test gave false confidence for a different JSON path than what the API actually returns. | Updated test to call `toExtendedJson(obj, dummy)` and assert `obj["subsystems"]["ntp"]`. |
| medium | `ConfigManager::loadFromNvs()` loads the `timezone` key from NVS without any validation, while `applyJson()` already guards against empty-string timezone via `if (tz.length() == 0) return false`. A device with corrupted NVS (manual write, older firmware) carrying `timezone=""` would pass the empty string to `configTzTime("")`, silently falling back to UTC with no logging. | Added `if (tz.length() > 0 && tz.length() <= 40)` guard in `loadFromNvs()`, mirroring the `applyJson` validation. Also added `test_nvs_invalid_timezone_ignored` regression test. |
| dismissed | `g_ntpSynced` never resets to `false` after WiFi reconnect where NTP fails (Validators A, B, C) | FALSE POSITIVE: Adjudicated as out-of-scope in Round 1 and Round 2 antipatterns docs. AC #3 defines `ntpSynced=false` only as the pre-first-sync default; "sticky true" is the intentional contract for this story. WiFi-reconnect NTP invalidation is a concern for a future story. |
| dismissed | Cross-core race: `configTzTime()` → `setenv()`/`tzset()` on Core 1 unsafe against future `localtime_r()` on Core 0 | FALSE POSITIVE: Currently Core 0 display task makes **no** `localtime_r()` calls — there is no ClockMode or scheduler implemented yet. No actual race condition exists in the current code. Future stories (fn-2.2) that add Core 0 time access must address this; flagging it now as a bug is premature. Already dismissed in Round 1. |
| dismissed | SNTP thrashing on rapid timezone changes — missing debounce | FALSE POSITIVE: The `s_lastAppliedTz` cache (added in Round 2) already prevents SNTP restart on every unrelated config write. Rapid alternating between two different timezone values is a pathological case (API script abuse) with the only consequence being delayed NTP sync convergence — the device remains functional. A 5s debounce would require non-blocking timer infrastructure not warranted for this failure mode. |
| dismissed | `schedule_active` field semantics mislead API consumers | FALSE POSITIVE: Already tracked as Round 1 deferred action item. Will be corrected in fn-2.2 (Night Mode scheduler) when actual in-window logic lands. No breaking change needed now. |
| dismissed | `TZ_MAP` duplicated across `dashboard.js` and `wizard.js` | FALSE POSITIVE: Already tracked as Round 1 deferred action item. Moving to `common.js` must be validated against wizard page script-load order; project "no new files" rule blocks action in this story. Deferred to fn-2.3. |
| dismissed | `getTimezoneConfig()` silent UTC fallback for unmapped IANA zones | FALSE POSITIVE: Already tracked as Round 1 deferred action item. No UI element currently consumes `getTimezoneConfig()`. fn-2.3 will add the dropdown and can surface the unsupported-zone warning at that point. |
| dismissed | Task 7 overclaims test coverage — tests never exercise `onNtpSync()` callback or the actual HTTP endpoint | FALSE POSITIVE: Adjudicated as false positive in Round 1 and Round 2. Embedded Unity tests on ESP32 cannot instantiate the HTTP server or simulate LWIP callbacks without a full device simulator. AC #1 explicitly scopes: "unit tests cover callback logic only." The test names accurately describe what they test. |
| dismissed | Story file list does not match current git diff | FALSE POSITIVE: Minor audit trail issue — the story's File List was incomplete (missing `ConfigManager.cpp` which was added in Round 2). Fixed in this round by updating the File List to include `firmware/core/ConfigManager.cpp`. No code change required. --- |

## Story fn-2-2 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `extern bool isScheduleDimming()` missing from WebPortal.cpp + `schedule_dimming` field absent from `/api/status` — AC #8 violation (Task 4 never applied) | Added `extern bool isScheduleDimming();` declaration after `isNtpSynced()`, added `data["schedule_dimming"] = isScheduleDimming();` in `_handleGetStatus()` |
| medium | `ConfigManager::loadFromNvs()` reads `sched_enabled`, `sched_dim_start`, `sched_dim_end` from NVS without range validation — corrupted NVS (value `sched_dim_start=65000`) feeds impossible minutes values into `tickNightScheduler()` every second | Added conditional validation matching `applyJson` rules: `sched_enabled` ≤ 1 else reset to 0; `sched_dim_start`/`sched_dim_end` ≤ 1439 else clamp to defaults (1380/420); `sched_dim_brt` left as-is (uint8_t range 0–255 is always valid per `applyJson`) |
| low | `static_assert(SCHED_CHECK_INTERVAL_MS <= 2000, ...)` specified in story Task 1.1 was omitted from the implementation | Added `static_assert` immediately after the constant declaration |
| dismissed | `schedule_active` in `/api/status` should be changed to `isScheduleDimming()` | FALSE POSITIVE: Story Task 4.2 and the fn-2.1 Review Follow-ups section both explicitly define `schedule_active` as "enabled + NTP synced = scheduler can run" and `schedule_dimming` as the real-time override state. Changing `schedule_active` would be a breaking API change outside story scope. The new `schedule_dimming` field resolves the semantic gap as designed. |
| dismissed | `ScheduleConfig` copies `String timezone` in `tickNightScheduler()` every second — ~2.6M heap allocations over 30 days | FALSE POSITIVE: The fix (a new `getScheduleDimmingParams()` function) contradicts the story's "No new files / no new functions beyond what's specified" constraint. The 1/sec String copy is a known ESP32 heap-fragmentation risk but is a future-story optimization concern, not a blocking bug for this story's NFR. |
| dismissed | `g_lastNtpSynced` (plain bool) race with `g_ntpSynced` (atomic) can produce duplicate/missing transition log lines | FALSE POSITIVE: `g_lastNtpSynced` is read and written exclusively by `tickNightScheduler()` on Core 1's Arduino loop task. The SNTP callback also runs on Core 1 but on the LWIP task — preemption is possible but the consequence is at most a duplicate or missing log line. The story's own comment documents "Core 1 only — no atomic needed". Accepted design risk for a non-functional log side-effect. |
| dismissed | Redundant `ConfigManager::getSchedule()` at lines 327 and 363 | FALSE POSITIVE: FALSE POSITIVE. Reading the actual code at lines 326–365, line 327 is `DisplayConfig newDisp = ConfigManager::getDisplay()` — not a `getSchedule()` call. There is exactly one `getSchedule()` call at line 363, inside the `if (g_schedDimming.load())` guard, so it only executes when actually dimming. |
| dismissed | Race between config read (line 356) and `g_schedDimming.load()` check (line 362) — one frame of wrong brightness if schedule transitions mid-handler | FALSE POSITIVE: The race window is microseconds; consequence is at most one display frame (~50ms) of wrong brightness, self-correcting on the next scheduler tick (1s) or config change. This is an accepted design risk for a transient display artifact — no functional correctness or data integrity issue. |
| dismissed | `g_ntpSynced` never resets to `false` after WiFi disconnect — scheduler silently continues post-disconnect | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.1 antipatterns (Rounds 1, 2, and 3). AC #4 defines `ntpSynced=false` only as the pre-first-sync default. "Sticky true" post-sync is the intentional contract. WiFi-reconnect NTP invalidation semantics are a future-story concern. |
| dismissed | AC #6 behavioral mismatch — manual brightness is re-overridden immediately (in the same config-change handler), not "on the next scheduler tick" as the story describes | FALSE POSITIVE: Intentional documented deviation. The story Dev Notes ("Brightness Override Mechanism") explicitly say: "because they trigger `g_configChanged`, the display task's handler runs immediately and re-applies the dim override (not 'on the next scheduler tick' — it's immediate, which is better than AC#6 describes)." The code comment at line 360 also documents this. Not a bug. |
| dismissed | Unit tests only test a local `isInDimWindow()` helper, never calling `tickNightScheduler()` or asserting `g_schedDimming` transitions | FALSE POSITIVE: Architectural limitation already established across fn-2.1 reviews. Unity tests on ESP32 cannot instantiate the main loop, LWIP scheduler, or FastLED display. The local `isInDimWindow()` helper is the production arithmetic extracted as a testable unit — this is correct TDD discipline. The story explicitly scopes "unit tests validate the exact same arithmetic the scheduler uses." Full integration testing requires hardware. |
| dismissed | Story file list drift — `WebPortal.cpp` listed but untouched; `ConfigManager.cpp` unlisted but changed | FALSE POSITIVE: Audit trail concern, not a product defect. Fixes applied in this synthesis bring `WebPortal.cpp` in line with the story. `ConfigManager.cpp` change (NVS validation) is a synthesis-added defensive fix beyond the original story spec. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Short-circuit `||` prevents `g_scheduleChanged` from being atomically cleared in display task | `firmware/src/main.cpp` | +1.0 |
| 🟠 IMPORTANT | `tickNightScheduler()` copies `String timezone` every second causing heap churn | `firmware/src/main.cpp` | +1.0 |
| 🟠 IMPORTANT | Unit tests only validate a copy-pasted helper, not the actual scheduler state machine | `firmware/test/test_config_manager/test_main.cpp` | +1.0 |
| 🟠 IMPORTANT | Missing test coverage for AC #6 (manual brightness re-override during dim window) | `firmware/test/test_config_manager/test_main.cpp` | +1.0 |
| 🟡 MINOR | `getLocalTime()` result trusted without bounds-checking `tm_hour`/`tm_min` | `firmware/src/main.cpp` | +0.3 |
| 🟡 MINOR | Non-atomic `g_lastNtpSynced` race with LWIP task on Core 1 | `firmware/src/main.cpp` | +0.3 |
| 🟡 MINOR | Misleading test name `test_schedule_default_dim_brightness` tests 4 unrelated fields | `firmware/test/test_config_manager/test_main.cpp` | +0.3 |
| 🟡 MINOR | Silent clamping of corrupted NVS schedule values without any log warning | `firmware/core/ConfigManager.cpp` | +0.3 |
| 🟢 CLEAN PASS | 5 categories with no significant issues | — | -2.5 |

### Evidence Score: 2.7

| Score | Verdict |
|-------|---------|
| **2.7** | **APPROVED** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows C++ project style guidelines and is readable.

---

## ⚡ Performance & Scalability

- **[MEDIUM] Heap allocation every second from `String timezone` copy:** `tickNightScheduler()` calls `ConfigManager::getSchedule()` every 1000ms, which copies the full `ScheduleConfig` including the `String timezone` member. On ESP32 this causes a heap allocation every second (~2.6M allocations over 30 days), increasing fragmentation risk on a device with only ~80KB free heap headroom.
  - 📍 `firmware/src/main.cpp:325`
  - 💡 Fix: Add a lightweight `getScheduleDimmingParams()` getter that returns only the scalar fields (`sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt`) without the `String`.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** Short-circuit `||` in display task prevents clearing `g_scheduleChanged`. The condition `g_configChanged.exchange(false) || g_scheduleChanged.exchange(false)` uses short-circuit evaluation. If `g_configChanged` is true, `g_scheduleChanged` is never atomically cleared, leaving a stale flag that triggers an unnecessary extra config handler run on the next display frame (~50ms later). While currently harmless, it is a concurrency hygiene defect.
  - 📍 `firmware/src/main.cpp:352`
  - 🔄 Reproduction: Trigger a config change (e.g., brightness slider) at the exact moment the scheduler enters the dim window. `g_scheduleChanged` remains true and the handler re-runs on the next iteration.
  - 💡 Fix: Evaluate both atomics before the condition: `bool configChanged = g_configChanged.exchange(false); bool scheduleChanged = g_scheduleChanged.exchange(false); if (configChanged || scheduleChanged) { ... }`

- **🐛 Bug:** `getLocalTime()` result used without defensive bounds checking. After `getLocalTime(&timeinfo, 0)` succeeds, the code directly computes `timeinfo.tm_hour * 60 + timeinfo.tm_min` without validating the fields are within expected ranges (0-23 and 0-59). In the pathological case of RTC corruption or a `tzset()` race, `nowMinutes` could be out of range.
  - 📍 `firmware/src/main.cpp:335`
  - 🔄 Reproduction: Corrupt the RTC or trigger a `tzset()` race during `getLocalTime()`; scheduler computes an invalid minute value and may incorrectly enter/exit the dim window.

- **🎭 Lying Test:** `test_schedule_dim_window_same_day`, `test_schedule_dim_window_crosses_midnight`, `test_schedule_dim_window_edge_cases`
  - 📍 `firmware/test/test_config_manager/test_main.cpp:266-312`
  - 🤥 Why it lies: These tests validate a local `isInDimWindow()` helper that copy-pastes the production arithmetic from `tickNightScheduler()`. They do NOT test the actual scheduler state machine (NTP transitions, enable/disable transitions, `g_schedDimming` flag lifecycle, or display task override logic). A bug in `tickNightScheduler()`'s flag signaling would pass these tests with flying colors.

- **🎭 Lying Test:** Missing test for AC #6 (manual brightness re-override)
  - 📍 `firmware/test/test_config_manager/test_main.cpp`
  - 🤥 Why it lies: Acceptance Criterion #6 explicitly requires that manual brightness changes during a dim window are re-overridden. There is zero test coverage for this interaction between `POST /api/settings` brightness changes and the display task's dim override. The story claims "All 5 tasks implemented and verified" but this critical behavioral contract is untested.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `test_schedule_default_dim_brightness` tests 4 unrelated defaults. The test name implies it only checks `sched_dim_brt`, but it also asserts `sched_dim_start`, `sched_dim_end`, and `sched_enabled`. This is misleading for future maintainers.
  - 📍 `firmware/test/test_config_manager/test_main.cpp:314-320`
  - 💥 Explosion radius: Low — causes confusion when adding new schedule defaults.

- **💣 Tech Debt:** Silent clamping of corrupted NVS schedule values. `ConfigManager::loadFromNvs()` silently resets `sched_enabled` to 0, `sched_dim_start` to 1380, and `sched_dim_end` to 420 when they exceed valid ranges, with no `LOG_W` or `LOG_E`. This makes detecting NVS corruption or flash wear impossible from logs.
  - 📍 `firmware/core/ConfigManager.cpp:227-240`
  - 💥 Explosion radius: Low — debugging corrupted NVS data becomes difficult.

- **💣 Tech Debt:** Non-atomic `g_lastNtpSynced` shared between Arduino loop task and LWIP task. Both tasks run on Core 1. Preemption can cause duplicate or missing "NTP synced/lost" log lines, and the variable is documented as "Core 1 only — no atomic needed" which is technically incorrect when multiple tasks share the same core.
  - 📍 `firmware/src/main.cpp:69`
  - 💥 Explosion radius: Low — only affects log output, but sets a bad concurrency hygiene precedent.

---

## 🛠️ Suggested Fixes

### 1. Fix short-circuit `||` in display task config-change handler

**File:** `firmware/src/main.cpp`
**Issue:** `g_scheduleChanged` is not cleared when `g_configChanged` is also true due to `||` short-circuit evaluation.

**Diff:**
```cpp
// Before:
if (g_configChanged.exchange(false) || g_scheduleChanged.exchange(false)) {

// After:
bool configChanged = g_configChanged.exchange(false);
bool scheduleChanged = g_scheduleChanged.exchange(false);
if (configChanged || scheduleChanged) {
```

### 2. Eliminate per-second `String` allocation in scheduler

**File:** `firmware/src/main.cpp`
**Issue:** `ConfigManager::getSchedule()` copies the `timezone` String every second.

**Diff:**
```cpp
// In ConfigManager.h/.cpp, add:
struct ScheduleDimmingParams {
    uint8_t sched_enabled;
    uint16_t sched_dim_start;
    uint16_t sched_dim_end;
    uint8_t sched_dim_brt;
};
static ScheduleDimmingParams getScheduleDimmingParams();

// In tickNightScheduler():
auto sched = ConfigManager::getScheduleDimmingParams();
```

### 3. Add defensive bounds check after `getLocalTime()`

**File:** `firmware/src/main.cpp`
**Issue:** Unchecked `timeinfo` fields.

**Diff:**
```cpp
struct tm timeinfo;
if (!getLocalTime(&timeinfo, 0)) {
    return;
}
if (timeinfo.tm_hour < 0 || timeinfo.tm_hour > 23 ||
    timeinfo.tm_min < 0 || timeinfo.tm_min > 59) {
    LOG_E("Scheduler", "Invalid time from getLocalTime()");
    return;
}
```

### 4. Log when clamping corrupted NVS values

**File:** `firmware/core/ConfigManager.cpp`
**Issue:** Silent fallback to defaults on corrupted NVS data.

**Diff:**
```cpp
if (prefs.isKey("sched_enabled")) {
    uint8_t v = prefs.getUChar("sched_enabled", snapshot.schedule.sched_enabled);
    if (v > 1) {
        LOG_W("ConfigManager", "Corrupted sched_enabled in NVS (%u), resetting to 0", v);
        v = 0;
    }
    snapshot.schedule.sched_enabled = v;
}
// Similar for sched_dim_start, sched_dim_end...
```

### 5. Rename misleading test

**File:** `firmware/test/test_config_manager/test_main.cpp`
**Issue:** Test name doesn't match what it actually asserts.

**Diff:**
```cpp
// Before:
void test_schedule_default_dim_brightness() {

// After:
void test_schedule_defaults() {
```

---

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 5

✅ Code is approved and ready for deployment! (Score: 2.7 — minor issues identified, none blocking.)

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

I'll conduct a thorough adversarial code review of Story fn-2.2. Let me start by reading the story file and then examining all the source files.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review**

**Story:** [fn-2-2-night-mode-brightness-scheduler.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md:1)

Static review only. I did not run `pio` or Unity in this read-only sandbox.

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 1 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **5** |

## Findings

1. **🔴 CRITICAL: Task 5 is marked complete, but the tests do not cover the scheduler behavior the story claims.**  
   The story says Task 5 covers AC #1, #2, #3, #4, and #5, but the added tests only exercise a local `isInDimWindow()` helper and default config values. Nothing tests `tickNightScheduler()`, the `getLocalTime(&now, 0)` non-blocking contract, the `g_scheduleChanged` handoff to Core 0, or brightness restoration when the schedule is disabled. That is a task-completion lie, not just a testing gap.  
   Sources: [story file](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md:210), [test_main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/test/test_config_manager/test_main.cpp:571)

2. **🟠 IMPORTANT: AC #8 is still not implemented as written.**  
   AC #8 says `schedule_active` must reflect whether the scheduler is currently overriding brightness. The implementation still reports `schedule_active = sched_enabled && ntp_synced`, which is only “scheduler can run,” not “scheduler is currently dimming.” Adding `schedule_dimming` helps, but it does not satisfy the acceptance criterion the story declares.  
   Sources: [story file](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md:62), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:788)

3. **🟠 IMPORTANT: The main-loop scheduler copies a heap-backed `String` every second on a 30-day uptime path.**  
   `tickNightScheduler()` pulls `ScheduleConfig` by value once per second. That struct contains `String timezone`, so the hot path copies a dynamic string even though the scheduler only needs `sched_enabled`, `sched_dim_start`, `sched_dim_end`, and `sched_dim_brt`. On ESP32 that is avoidable heap churn in exactly the kind of long-lived loop the architecture says must stay cheap and stable.  
   Sources: [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:887), [ConfigManager.h](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ConfigManager.h:47)

4. **🟠 IMPORTANT: The “NTP lost/regained” path is effectively dead code.**  
   The story and comments talk about logging only on NTP lost/regained transitions, but the only write to `g_ntpSynced` is `store(true)` in `onNtpSync()`. There is no path that clears it to `false`, so the branch logging `"NTP lost — schedule suspended"` cannot represent a real runtime loss of sync. At minimum the comments and acceptance-criteria mapping are misleading; at worst the status model is wrong for post-boot time-loss scenarios.  
   Sources: [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:86), [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:896)

5. **🟡 MINOR: Corrupt NVS schedule repair is silent and hard-codes duplicated defaults.**  
   The new NVS guards clamp invalid `sched_dim_start` and `sched_dim_end` to literal `1380` and `420` with no log or health signal. That means corrupt schedule state quietly mutates into a specific night window, and the fallback values are duplicated instead of reusing the configured defaults already loaded into `snapshot.schedule`. This is exactly the kind of silent repair that becomes hard to diagnose in the field.  
   Sources: [ConfigManager.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ConfigManager.cpp:405)

## Evidence Score

| Severity | Source | Score |
|----------|--------|-------|
| 🔴 CRITICAL | [test_main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/test/test_config_manager/test_main.cpp:571), [story file](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md:210) | +3.0 |
| 🟠 IMPORTANT | [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:788) | +1.0 |
| 🟠 IMPORTANT | [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:887), [ConfigManager.h](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ConfigManager.h:47) | +1.0 |
| 🟠 IMPORTANT | [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:86), [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:896) | +1.0 |
| 🟡 MINOR | [ConfigManager.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ConfigManager.cpp:405) | +0.3 |
| 🟢 CLEAN PASS | SOLID, Abstraction, Style, Type Safety, Security | -2.5 |

**Evidence Score: 3.8**

| Score | Verdict |
|-------|---------|
| **3.8** | **MAJOR REWORK** |

## Suggested Fixes

1. Extract a pure scheduler evaluator and a narrow schedule getter that excludes `timezone`, then test that evaluator directly instead of testing a copied local helper.
2. Resolve the API contract mismatch explicitly: either make `schedule_active` mean actual dimming, or rename the current field to `schedule_enabled` and update the story/clients accordingly.
3. Decide whether `g_ntpSynced` is intentionally sticky. If yes, delete the dead “lost/regained” logic and comments. If no, add an explicit reset path and test it.
4. When repairing corrupt NVS schedule values, reuse `snapshot.schedule` defaults and emit a log or `SystemStatus` warning so the field behavior is diagnosable.

**Review Actions:** Issues Found: 5, Issues Fixed: 0, Action Items Created: 0

⚠️ Address the identified issues before proceeding.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-13</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-2</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">3</var>
<var name="session_id">4bd8a8b4-3d2d-475a-916d-0861c77468ce</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="89294d9b">embedded in prompt, file id: 89294d9b</var>
<var name="story_id">fn-2.2</var>
<var name="story_key">fn-2-2-night-mode-brightness-scheduler</var>
<var name="story_num">2</var>
<var name="story_title">2-night-mode-brightness-scheduler</var>
<var name="template">False</var>
<var name="timestamp">20260413_1133</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="validator_count"></var>
</variables>
<instructions><workflow>
  <critical>Communicate all responses in English and generate all documents in English</critical>
  <critical>You are the MASTER SYNTHESIS agent for CODE REVIEW findings.</critical>
  <critical>You have WRITE PERMISSION to modify SOURCE CODE files and story Dev Agent Record section.</critical>
  <critical>DO NOT modify story context (AC, Dev Notes content) - only Dev Agent Record (task checkboxes, completion notes, file list).</critical>
  <critical>All context (project_context.md, story file, anonymized reviews) is EMBEDDED below - do NOT attempt to read files.</critical>

  <step n="1" goal="Analyze reviewer findings">
    <action>Read all anonymized reviewer outputs (Reviewer A, B, C, D, etc.)</action>
    <action>For each issue raised:
      - Cross-reference with embedded project_context.md and story file
      - Cross-reference with source code snippets provided in reviews
      - Determine if issue is valid or false positive
      - Note reviewer consensus (if 3+ reviewers agree, high confidence issue)
    </action>
    <action>Issues with low reviewer agreement (1-2 reviewers) require extra scrutiny</action>
    <action>Group related findings that address the same underlying problem</action>
  </step>

  <step n="1.5" goal="Review Deep Verify code analysis" conditional="[Deep Verify Findings] section present">
    <critical>Deep Verify analyzed the actual source code files for this story.
      DV findings are based on static analysis patterns and may identify issues reviewers missed.</critical>

    <action>Review each DV finding:
      - CRITICAL findings: Security vulnerabilities, race conditions, resource leaks - must address
      - ERROR findings: Bugs, missing error handling, boundary issues - should address
      - WARNING findings: Code quality concerns - consider addressing
    </action>

    <action>Cross-reference DV findings with reviewer findings:
      - DV + Reviewers agree: High confidence issue, prioritize in fix order
      - Only DV flags: Verify in source code - DV has precise line numbers
      - Only reviewers flag: May be design/logic issues DV can't detect
    </action>

    <action>DV findings may include evidence with:
      - Code quotes (exact text from source)
      - Line numbers (precise location, when available)
      - Pattern IDs (known antipattern reference)
      Use this evidence when applying fixes.</action>

    <action>DV patterns reference:
      - CC-*: Concurrency issues (race conditions, deadlocks)
      - SEC-*: Security vulnerabilities
      - DB-*: Database/storage issues
      - DT-*: Data transformation issues
      - GEN-*: General code quality (null handling, resource cleanup)
    </action>
  </step>

  <step n="2" goal="Verify issues and identify false positives">
    <action>For each issue, verify against embedded code context:
      - Does the issue actually exist in the current code?
      - Is the suggested fix appropriate for the codebase patterns?
      - Would the fix introduce new issues or regressions?
    </action>
    <action>Document false positives with clear reasoning:
      - Why the reviewer was wrong
      - What evidence contradicts the finding
      - Reference specific code or project_context.md patterns
    </action>
  </step>

  <step n="3" goal="Prioritize by severity">
    <action>For verified issues, assign severity:
      - Critical: Security vulnerabilities, data corruption, crashes
      - High: Bugs that break functionality, performance issues
      - Medium: Code quality issues, missing error handling
      - Low: Style issues, minor improvements, documentation
    </action>
    <action>Order fixes by severity - Critical first, then High, Medium, Low</action>
    <action>For disputed issues (reviewers disagree), note for manual resolution</action>
  </step>

  <step n="4" goal="Apply fixes to source code">
    <critical>This is SOURCE CODE modification, not story file modification</critical>
    <critical>Use Edit tool for all code changes - preserve surrounding code</critical>
    <critical>After applying each fix group, run: pytest -q --tb=line --no-header</critical>
    <critical>NEVER proceed to next fix if tests are broken - either revert or adjust</critical>

    <action>For each verified issue (starting with Critical):
      1. Identify the source file(s) from reviewer findings
      2. Apply fix using Edit tool - change ONLY the identified issue
      3. Preserve code style, indentation, and surrounding context
      4. Log the change for synthesis report
    </action>

    <action>After each logical fix group (related changes):
      - Run: pytest -q --tb=line --no-header
      - If tests pass, continue to next fix
      - If tests fail:
        a. Analyze which fix caused the failure
        b. Either revert the problematic fix OR adjust implementation
        c. Run tests again to confirm green state
        d. Log partial fix failure in synthesis report
    </action>

    <action>Atomic commit guidance (for user reference):
      - Commit message format: fix(component): brief description (synthesis-fn-2.2)
      - Group fixes by severity and affected component
      - Never commit unrelated changes together
      - User may batch or split commits as preferred
    </action>
  </step>

  <step n="5" goal="Refactor if needed">
    <critical>Only refactor code directly related to applied fixes</critical>
    <critical>Maximum scope: files already modified in Step 4</critical>

    <action>Review applied fixes for duplication patterns:
      - Same fix applied 2+ times across files = candidate for refactor
      - Only if duplication is in files already modified
    </action>

    <action>If refactoring:
      - Extract common logic to shared function/module
      - Update all call sites in modified files
      - Run tests after refactoring: pytest -q --tb=line --no-header
      - Log refactoring in synthesis report
    </action>

    <action>Do NOT refactor:
      - Unrelated code that "could be improved"
      - Files not touched in Step 4
      - Patterns that work but are just "not ideal"
    </action>

    <action>If broader refactoring needed:
      - Note it in synthesis report as "Suggested future improvement"
      - Do not apply - leave for dedicated refactoring story
    </action>
  </step>

  <step n="6" goal="Generate synthesis report">
    <critical>When updating story file, use atomic write pattern (temp file + rename).</critical>
    <action>Update story file Dev Agent Record section ONLY:
      - Mark completed tasks with [x] if fixes address them
      - Append to "Completion Notes List" subsection summarizing changes applied
      - Update file list with all modified files
    </action>

    <critical>Your synthesis report MUST be wrapped in HTML comment markers for extraction:</critical>
    <action>Produce structured output in this exact format (including the markers):</action>
    <output-format>
&lt;!-- CODE_REVIEW_SYNTHESIS_START --&gt;
## Synthesis Summary
[Brief overview: X issues verified, Y false positives dismissed, Z fixes applied to source files]

## Validations Quality
[For each reviewer: ID (A, B, C...), score (1-10), brief assessment]
[Note: Reviewers are anonymized - do not attempt to identify providers]

## Issues Verified (by severity)

### Critical
[Issues that required immediate fixes - list with evidence and fixes applied]
[Format: "- **Issue**: Description | **Source**: Reviewer(s) | **File**: path | **Fix**: What was changed"]
[If none: "No critical issues identified."]

### High
[Bugs and significant problems - same format]

### Medium
[Code quality issues - same format]

### Low
[Minor improvements - same format, note any deferred items]

## Issues Dismissed
[False positives with reasoning for each dismissal]
[Format: "- **Claimed Issue**: Description | **Raised by**: Reviewer(s) | **Dismissal Reason**: Why this is incorrect"]
[If none: "No false positives identified."]

## Changes Applied
[Complete list of modifications made to source files]
[Format for each change:
  **File**: [path/to/file.py]
  **Change**: [Brief description]
  **Before**:
  ```
  [2-3 lines of original code]
  ```
  **After**:
  ```
  [2-3 lines of updated code]
  ```
]
[If no changes: "No source code changes required."]

## Deep Verify Integration
[If DV findings were present, document how they were handled]

### DV Findings Fixed
[List DV findings that resulted in code changes]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **File**: {path} | **Fix**: {What was changed}"]

### DV Findings Dismissed
[List DV findings determined to be false positives]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Reason**: {Why this is not an issue}"]

### DV-Reviewer Overlap
[Note findings flagged by both DV and reviewers - highest confidence fixes]
[If no DV findings: "Deep Verify did not produce findings for this story."]

## Files Modified
[Simple list of all files that were modified]
- path/to/file1.py
- path/to/file2.py
[If none: "No files modified."]

## Suggested Future Improvements
[Broader refactorings or improvements identified in Step 5 but not applied]
[Format: "- **Scope**: Description | **Rationale**: Why deferred | **Effort**: Estimated complexity"]
[If none: "No future improvements identified."]

## Test Results
[Final test run output summary]
- Tests passed: X
- Tests failed: 0 (required for completion)
&lt;!-- CODE_REVIEW_SYNTHESIS_END --&gt;
    </output-format>

  </step>

  <step n="6.5" goal="Write Senior Developer Review section to story file for dev_story rework detection">
    <critical>This section enables dev_story to detect that a code review has occurred and extract action items.</critical>
    <critical>APPEND this section to the story file - do NOT replace existing content.</critical>

    <action>Determine the evidence verdict from the [Evidence Score] section:
      - REJECT: Evidence score exceeds reject threshold
      - PASS: Evidence score is below accept threshold
      - UNCERTAIN: Evidence score is between thresholds
    </action>

    <action>Map evidence verdict to review outcome:
      - PASS → "Approved"
      - REJECT → "Changes Requested"
      - UNCERTAIN → "Approved with Reservations"
    </action>

    <action>Append to story file "## Senior Developer Review (AI)" section:
      ```
      ## Senior Developer Review (AI)

      ### Review: {current_date}
      - **Reviewer:** AI Code Review Synthesis
      - **Evidence Score:** {evidence_score} → {evidence_verdict}
      - **Issues Found:** {total_verified_issues}
      - **Issues Fixed:** {fixes_applied_count}
      - **Action Items Created:** {remaining_unfixed_count}
      ```
    </action>

    <critical>When evidence verdict is REJECT, you MUST create Review Follow-ups tasks.
      If "Action Items Created" count is &gt; 0, there MUST be exactly that many [ ] [AI-Review] tasks.
      Do NOT skip this step. Do NOT claim all issues are fixed if you reported deferred items above.</critical>

    <action>Find the "## Tasks / Subtasks" section in the story file</action>
    <action>Append a "#### Review Follow-ups (AI)" subsection with checkbox tasks:
      ```
      #### Review Follow-ups (AI)
      - [ ] [AI-Review] {severity}: {brief description of unfixed issue} ({file path})
      ```
      One line per unfixed/deferred issue, prefixed with [AI-Review] tag.
      Order by severity: Critical first, then High, Medium, Low.
    </action>

    <critical>ATDD DEFECT CHECK: Search test directories (tests/**) for test.fixme() calls in test files related to this story.
      If ANY test.fixme() calls remain in story-related test files, this is a DEFECT — the dev_story agent failed to activate ATDD RED-phase tests.
      Create an additional [AI-Review] task:
      - [ ] [AI-Review] HIGH: Activate ATDD tests — convert all test.fixme() to test() and ensure they pass ({test file paths})
      Do NOT dismiss test.fixme() as "intentional TDD methodology". After dev_story completes, ALL test.fixme() tests for the story MUST be converted to test().</critical>
  </step>

  </workflow></instructions>
<output-template></output-template>
</compiled-workflow>