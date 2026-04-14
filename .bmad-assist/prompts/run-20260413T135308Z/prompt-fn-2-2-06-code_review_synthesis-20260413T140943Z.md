<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-2 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T140943Z -->
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

Status: ready-for-dev

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

- [ ] **Task 1: Add scheduler state variables and helper function in main.cpp** (AC: #1, #4, #5)
  - [ ] 1.1 Add scheduler state variables after the existing `g_ntpSynced` declaration (~line 67):
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
  - [ ] 1.2 Add public accessor for WebPortal (~after `isNtpSynced()` at line 70):
    ```cpp
    bool isScheduleDimming() { return g_schedDimming.load(); }
    ```
  - [ ] 1.3 Add the `tickNightScheduler()` function before `loop()` (~line 860, before the `loop()` function):
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

- [ ] **Task 2: Integrate scheduler into loop()** (AC: #1, #7)
  - [ ] 2.1 In `loop()` (line 862), add `tickNightScheduler()` call after `g_wifiManager.tick()` and before the OTA self-check block:
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

- [ ] **Task 3: Modify display task to apply scheduler brightness override** (AC: #2, #3, #5, #6)
  - [ ] 3.1 In the `displayTask()` function's config-change handler (line 314-346), expand the condition to also check `g_scheduleChanged` and add scheduler override logic after `g_display.updateBrightness(localDisp.brightness);` (line 344):
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
  - [ ] 3.2 `g_schedDimming` is declared as `std::atomic<bool>` (see Task 1.1). The dedicated `g_scheduleChanged` atomic flag ensures schedule state transitions (enter/exit dim window) are never lost due to a concurrent `g_configChanged` consumption by the display task. Both flags are checked with `||` in the same `exchange(false)` condition, so either trigger causes the display task to re-evaluate brightness.

- [ ] **Task 4: Update WebPortal /api/status with scheduler dimming state** (AC: #8)
  - [ ] 4.1 In `WebPortal.cpp`, add `extern bool isScheduleDimming();` declaration near the existing `extern bool isNtpSynced();` (line 40):
    ```cpp
    // Defined in main.cpp — Night mode scheduler dimming state (Story fn-2.2)
    extern bool isScheduleDimming();
    ```
  - [ ] 4.2 In `_handleGetStatus()` (line 787), update `schedule_active` semantics and add `schedule_dimming` field:
    ```cpp
    // NTP and schedule status (Story fn-2.1, fn-2.2)
    data["ntp_synced"] = isNtpSynced();
    data["schedule_active"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();
    data["schedule_dimming"] = isScheduleDimming();
    ```
    This addresses the fn-2.1 review follow-up: `schedule_active` keeps its current meaning (enabled + NTP synced = scheduler can run), while `schedule_dimming` tells the dashboard whether the scheduler is currently overriding brightness.

- [ ] **Task 5: Unit tests** (AC: #1, #2, #3, #4, #5)
  - [ ] 5.1 In `firmware/test/test_config_manager/test_main.cpp`, add new test cases at end of file:
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
  - [ ] 5.2 Register the new test functions in the test runner `RUN_TEST()` block at end of `main()`:
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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
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
| 🔴 CRITICAL | Missing `schedule_dimming` field in `/api/status` — direct AC #8 violation | `firmware/adapters/WebPortal.cpp:787` | +3.0 |
| 🟠 IMPORTANT | `tickNightScheduler()` copies `ScheduleConfig` (incl. `String timezone`) every second, causing ~2.6M heap allocations over 30 days | `firmware/src/main.cpp:890` | +1.0 |
| 🟠 IMPORTANT | `schedule_active` semantics contradict AC #8: returns `enabled && ntpSynced` instead of "currently overriding brightness" | `firmware/adapters/WebPortal.cpp:787` | +1.0 |
| 🟠 IMPORTANT | Missing `extern bool isScheduleDimming();` in WebPortal.cpp — would fail to compile if the missing field were added | `firmware/adapters/WebPortal.cpp:40` | +1.0 |
| 🟡 MINOR | Missing `static_assert(SCHED_CHECK_INTERVAL_MS <= 2000, ...)` from story spec Task 1.1 | `firmware/src/main.cpp:75` | +0.3 |
| 🟡 MINOR | Unit test helper `isInDimWindow` duplicates production logic — tests the copy, not the real scheduler | `firmware/test/test_config_manager/test_main.cpp:573` | +0.3 |
| 🟡 MINOR | Race between atomic `g_ntpSynced` and non-atomic `g_lastNtpSynced` can duplicate or skip transition logs | `firmware/src/main.cpp:895` | +0.3 |
| 🟡 MINOR | `ConfigManager::loadFromNvs()` loads `sched_dim_start/end` without range validation, letting corrupted NVS break window logic | `firmware/core/ConfigManager.cpp:403` | +0.3 |

### Evidence Score: 5.2

| Score | Verdict |
|-------|---------|
| **5.2** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

- **[HIGH] Unnecessary heap allocation in hot loop:** `tickNightScheduler()` calls `ConfigManager::getSchedule()` every second. `ScheduleConfig` contains a `String timezone`, so each call allocates a small heap block to copy the string. Over 30 days of uptime that's ~2.6M allocations — a textbook heap-fragmentation risk on the ESP32 non-compacting allocator.
  - 📍 `firmware/src/main.cpp:890`
  - 💡 Fix: Add a targeted `ConfigManager::getScheduleDimmingParams(bool&, uint16_t&, uint16_t&, uint8_t&)` that returns only the needed fields without copying the `String`, or split `ScheduleConfig` into a tz-free subset.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `GET /api/status` does **not** include the `schedule_dimming` field required by AC #8, and `schedule_active` still reports `enabled && ntpSynced` rather than whether the scheduler is actively overriding brightness. The implementor followed the story's Task 4.2 description but missed the AC requirement; the field is simply absent.
  - 📍 `firmware/adapters/WebPortal.cpp:787`
  - 🔄 Reproduction: Call `GET /api/status` while in the dim window — response lacks `schedule_dimming`, and `schedule_active` is `true` even if the window is not active.

- **🐛 Bug:** `extern bool isScheduleDimming();` is missing from `WebPortal.cpp`. Even if someone tries to fix the missing field locally, it won't link because the accessor is not declared in the translation unit.
  - 📍 `firmware/adapters/WebPortal.cpp:40`

- **🐛 Bug:** `ConfigManager::loadFromNvs()` reads `sched_dim_start`, `sched_dim_end`, and `sched_dim_brt` directly from NVS without range guards. A corrupted NVS write (e.g., manual tooling, flash wear) can load values >1439 into the minute fields, causing the dim-window arithmetic in `tickNightScheduler()` to behave unpredictably (e.g., an invalid `dimEnd` can make the window always-on).
  - 📍 `firmware/core/ConfigManager.cpp:403`

- **🐛 Bug:** `g_lastNtpSynced` is a plain `bool` while `g_ntpSynced` is `std::atomic<bool>`. The LWIP SNTP callback (which sets the atomic) runs on Core 1 and can preempt `tickNightScheduler()` between the atomic load and the update of `g_lastNtpSynced`, causing a duplicate or missing "NTP lost/regained" log line.
  - 📍 `firmware/src/main.cpp:895`

---

## 🛠️ Suggested Fixes

### 1. Add `schedule_dimming` to `/api/status` and fix `schedule_active` semantics

**File:** `firmware/adapters/WebPortal.cpp`

Add the `extern` declaration after `isNtpSynced()` (around line 41):
```cpp
extern bool isScheduleDimming();
```

Update `_handleGetStatus()` (around line 787) to match AC #8:
```cpp
data["schedule_active"] = isScheduleDimming();  // true only when actively overriding
data["schedule_dimming"] = isScheduleDimming();
```

### 2. Eliminate per-second `String` copy in `tickNightScheduler`

**File:** `firmware/core/ConfigManager.h`
Add a targeted getter that does not copy the timezone string:
```cpp
static void getScheduleDimmingParams(uint8_t& enabled, uint16_t& start, uint16_t& end, uint8_t& brt);
```

**File:** `firmware/core/ConfigManager.cpp`
Implement it by reading the mutex-protected fields directly into references.

**File:** `firmware/src/main.cpp`
Replace `ScheduleConfig sched = ConfigManager::getSchedule();` in `tickNightScheduler()` with the new getter.

### 3. Add `static_assert` for scheduler interval

**File:** `firmware/src/main.cpp`
After line 75:
```cpp
static_assert(SCHED_CHECK_INTERVAL_MS <= 2000,
    "SCHED_CHECK_INTERVAL_MS must be <=2s to meet 1-second display response NFR");
```

### 4. Validate schedule numeric ranges in `loadFromNvs`

**File:** `firmware/core/ConfigManager.cpp`
After reading `sched_dim_start`, `sched_dim_end`, and `sched_dim_brt` from NVS, clamp or reject out-of-range values (same rules as `applyJson`):
```cpp
if (snapshot.schedule.sched_dim_start > 1439) snapshot.schedule.sched_dim_start = 1380;
if (snapshot.schedule.sched_dim_end > 1439)   snapshot.schedule.sched_dim_end = 420;
if (snapshot.schedule.sched_dim_brt > 255)    snapshot.schedule.sched_dim_brt = 10;
```

### 5. Make `g_lastNtpSynced` atomic

**File:** `firmware/src/main.cpp`
Change `static bool g_lastNtpSynced = false;` to `static std::atomic<bool> g_lastNtpSynced(false);` and use `.load()` / `.store()` in `tickNightScheduler()`.

---

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 5

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Story claims WebPortal.cpp was modified but git shows NO changes — file is clean | git status | +3 |
| 🔴 CRITICAL | AC #8 requires `schedule_dimming` field in `/api/status` response but it's missing | WebPortal.cpp:787 | +3 |
| 🟠 IMPORTANT | WebPortal.cpp missing `extern bool isScheduleDimming();` declaration needed to access scheduler state | WebPortal.cpp:40 | +1 |
| 🟠 IMPORTANT | Display task calls `ConfigManager::getSchedule()` twice unnecessarily in same code block | main.cpp:327,363 | +1 |
| 🟠 IMPORTANT | Potential race condition: schedule state transition could occur between config read and brightness override | main.cpp:326-365 | +1 |
| 🟡 MINOR | Dim window logic duplicated between production code and unit test helper | main.cpp:925, test_main.cpp:573 | +0.3 |
| 🟢 CLEAN PASS | SOLID Principles | — | -0.5 |
| 🟢 CLEAN PASS | Security | — | -0.5 |
| 🟢 CLEAN PASS | Type Safety | — | -0.5 |
| 🟢 CLEAN PASS | Lying Tests | — | -0.5 |
| 🟢 CLEAN PASS | Tech Debt | — | -0.5 |

### Evidence Score: 5.8

| Score | Verdict |
|-------|---------|
| **5.8** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

- **[MEDIUM] Redundant ConfigManager call:** Display task handler calls `ConfigManager::getSchedule()` twice in the same function (lines 327 and 363). First call is for a potential early return check that never happens, second call actually uses the result. This is wasteful - call once and cache the result.
  - 📍 `firmware/src/main.cpp:327,363`
  - 💡 Fix: Remove the first `ConfigManager::getSchedule()` call at line 327, or cache the result in a local variable and reuse it at line 363.

---

## 🐛 Correctness & Safety

- **🐛 Bug: AC #8 NOT IMPLEMENTED - Missing schedule_dimming field in /api/status:** Acceptance Criterion #8 explicitly requires the `/api/status` response to include a `schedule_dimming` field that reflects "whether the scheduler is currently overriding brightness (not just whether it is enabled)." The current implementation at WebPortal.cpp:785-787 only includes `ntp_synced` and `schedule_active` but is missing the required `schedule_dimming` field. This breaks the API contract and prevents dashboard UI from showing whether the dim override is actively applied.
  - 📍 `firmware/adapters/WebPortal.cpp:787`
  - 🔄 Reproduction: Call `GET /api/status` and observe the response does NOT contain `schedule_dimming` field
  - ⚠️ Impact: Dashboard cannot accurately display dimming status to users

- **🐛 Bug: Missing extern declaration prevents scheduler state access:** The WebPortal.cpp file is missing the `extern bool isScheduleDimming();` declaration at line 40 (where `extern bool isNtpSynced();` already exists). This means even if AC #8 were implemented, WebPortal couldn't access the scheduler's dimming state to include it in the `/api/status` response. The function exists in main.cpp:79 but is not declared in the WebPortal compilation unit.
  - 📍 `firmware/adapters/WebPortal.cpp:40`
  - 🔄 Reproduction: Try to call `isScheduleDimming()` from WebPortal.cpp - it will fail to compile
  - ⚠️ Impact: Blocks implementation of AC #8 and violates the architectural separation of concerns

- **🐛 Bug: Potential race condition in config-change handler:** The display task's config-change handler at main.cpp:326-365 reads `ConfigManager::getDisplay()` at line 328, then later checks `g_schedDimming.load()` at line 362. In a multi-core system, a schedule state transition (triggered by `tickNightScheduler()` on Core 1) could occur between these two operations. If `g_schedDimming` changes from false to true during this window, the display task would apply the user's brightness from line 328 instead of the dim brightness from line 364, even though the scheduler is now active. This race is narrow but real.
  - 📍 `firmware/src/main.cpp:326-365`
  - 🔄 Reproduction: 1) User changes brightness via API, triggering `g_configChanged` 2) Display task starts handling config-change, reads user brightness 3) Scheduler's next tick fires on Core 1, sets `g_schedDimming = true` 4) Display task checks `g_schedDimming` (now true) but already applied wrong brightness
  - ⚠️ Impact: Display could show wrong brightness for one frame (~50ms) when config changes coincide with schedule transition

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Duplicate dim window logic between production and test code:** The dim window comparison logic (the midnight-crossing check) is implemented twice: once in `tickNightScheduler()` at main.cpp:925-929, and once as a helper function `isInDimWindow()` in test_main.cpp:573-579. Future bug fixes or enhancements must be made in both places, increasing maintenance burden and risk of divergence.
  - 📍 `firmware/src/main.cpp:925-929`, `firmware/test/test_config_manager/test_main.cpp:573-579`
  - 💥 Explosion radius: Medium - algorithmic logic duplicated in two files, requires dual maintenance

---

## 🛠️ Suggested Fixes

### 1. Add missing extern declaration and implement AC #8

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** Missing `extern bool isScheduleDimming();` declaration and `schedule_dimming` field in `/api/status`

**Diff:**
```diff
 // Defined in main.cpp — NTP sync status accessor (Story fn-2.1)
 extern bool isNtpSynced();
 
+// Defined in main.cpp — Night mode scheduler dimming state (Story fn-2.2)
+extern bool isScheduleDimming();
+
 #include "core/LayoutEngine.h"
 #include "core/ModeOrchestrator.h"
```

```diff
     // NTP and schedule status (Story fn-2.1)
     data["ntp_synced"] = isNtpSynced();
     data["schedule_active"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();
+    data["schedule_dimming"] = isScheduleDimming();
 
     // Rollback acknowledgment state (Story fn-1.6) — NVS-backed, not via ConfigManager
```

### 2. Remove redundant ConfigManager::getSchedule() call

**File:** `firmware/src/main.cpp`
**Issue:** ConfigManager::getSchedule() called twice unnecessarily

**Corrected code:**
```cpp
// Check both config changes AND dedicated schedule state transitions (Story fn-2.2)
// g_configChanged: fired by ConfigManager onChange (any setting change)
// g_scheduleChanged: fired by tickNightScheduler() on dim window enter/exit
// Both use exchange(false) to atomically read-and-clear; OR ensures either trigger runs this block.
if (g_configChanged.exchange(false) || g_scheduleChanged.exchange(false))
{
    DisplayConfig newDisp = ConfigManager::getDisplay();
    HardwareConfig newHw = ConfigManager::getHardware();
    TimingConfig newTiming = ConfigManager::getTiming();
    ScheduleConfig newSched = ConfigManager::getSchedule();  // Cache once, use twice

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

    // Apply scheduler brightness override if active (Story fn-2.2)
    // g_schedDimming is std::atomic<bool> — .load() for cross-core safety.
    // This also handles AC #6: any manual brightness API change triggers g_configChanged,
    // causing this handler to run and immediately re-apply the dim override if still in window.
    if (g_schedDimming.load()) {
        g_display.updateBrightness(newSched.sched_dim_brt);  // Use cached value
    }

    LOG_I("DisplayTask", "Config change detected, display settings updated");
}
```

### 3. Fix race condition by checking g_schedDimming before applying brightness

**File:** `firmware/src/main.cpp`
**Issue:** Race condition between config read and schedule state check

**Diff:**
```cpp
    localDisp = newDisp;
    localTiming = newTiming;

    // Check schedule state BEFORE applying any brightness to avoid race
    // (Story fn-2.2: scheduler takes priority over manual brightness)
    if (g_schedDimming.load()) {
        ScheduleConfig sched = ConfigManager::getSchedule();  // Get schedule for dim brightness
        g_display.updateBrightness(sched.sched_dim_brt);
    } else {
        g_display.updateBrightness(localDisp.brightness);  // Use user's configured brightness
    }

    LOG_I("DisplayTask", "Config change detected, display settings updated");
```

---

**Review Actions:**
- Issues Found: 6
- Issues Fixed: 0
- Action Items Created: 3

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md`

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 2 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 0 |
| SOLID / Abstraction Issues | 1 |
| Hidden Bugs | 2 |
| Lying Tests | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Scheduler gates on an NTP flag that is only ever set `true`, never cleared on disconnect/reset | `firmware/src/main.cpp:84`, `firmware/adapters/WiFiManager.cpp:144` | +1 |
| 🟠 IMPORTANT | `/api/status` was not updated with the new live dimming field; AC #8 is still missing | `firmware/adapters/WebPortal.cpp:785` | +1 |
| 🟠 IMPORTANT | Manual brightness is re-overridden immediately, not on the next scheduler tick as the story/architecture says | `firmware/src/main.cpp:356`, `firmware/src/main.cpp:931` | +1 |
| 🟠 IMPORTANT | New tests only restate the dim-window arithmetic and never exercise the production scheduler or API contract | `firmware/test/test_config_manager/test_main.cpp:571` | +1 |
| 🟠 IMPORTANT | Schedule values loaded from NVS still bypass validation, so corrupted persisted values can drive invalid runtime behavior | `firmware/core/ConfigManager.cpp:402` | +1 |
| 🟡 MINOR | The implementation drifted from the story file list: `WebPortal.cpp` is listed but untouched, while `ConfigManager.cpp` changed and is not listed | current git diff | +0.3 |
| 🟢 CLEAN PASS | No concrete security, performance, or type-safety regressions found in the changed code | review scope | -1.5 |

**Evidence Score: 3.8**

| Score | Verdict |
|-------|---------|
| **3.8** | **MAJOR REWORK** |

## Findings

1. **AC #4 is not actually implemented: the scheduler never sees NTP become false after the first successful sync.** [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:84) only does `g_ntpSynced.store(true)`, and [WiFiManager.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WiFiManager.cpp:144) never clears it on disconnect or reconnection failure. The branch in [tickNightScheduler()](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:903) that is supposed to suspend scheduling on NTP loss is therefore effectively dead after the first sync. Repro: boot once with valid WiFi/NTP, then disconnect WiFi; the scheduler still treats NTP as available.

2. **AC #8 is still missing in product code.** [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:785) still exposes only `ntp_synced` and the old `schedule_active` calculation. There is no `extern bool isScheduleDimming();`, no `schedule_dimming` field, and no changed status contract at all. The story explicitly calls for live dimming state, but the HTTP API remains blind to it.

3. **The manual-brightness behavior no longer matches the story contract or Architecture F5.** In [displayTask()](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:356), any `g_configChanged` event immediately reapplies `sched_dim_brt` if `g_schedDimming` is true. That means a dashboard brightness change inside the dim window is not applied “momentarily” and then corrected on the next scheduler tick; it is re-overridden in the same config-update path. The code comment at [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:931) explicitly admits this behavioral change.

4. **The new tests are weak enough to miss the two biggest implementation defects above.** The added cases in [test_config_manager/test_main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/test/test_config_manager/test_main.cpp:571) only test a local `isInDimWindow()` helper and default config values. They never call `tickNightScheduler()`, never assert `g_schedDimming` transitions, never verify the cross-core brightness override path, and never cover `/api/status`. These tests would still pass with the current missing `schedule_dimming` field and the stale `g_ntpSynced` bug.

5. **Persisted schedule values are still trusted blindly on boot.** [ConfigManager.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ConfigManager.cpp:402) reads `sched_enabled`, `sched_dim_start`, `sched_dim_end`, and `sched_dim_brt` straight from NVS with no bounds validation, even though the same file now validates `timezone` on load at [ConfigManager.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ConfigManager.cpp:397). With this story, those persisted values are now consumed every second in [tickNightScheduler()](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:918). Repro: write `sched_dim_start=65000` into NVS, reboot, and the scheduler operates on an impossible window.

6. **The implementation is no longer auditable from the story file list.** The current diff changes `firmware/src/main.cpp`, `firmware/test/test_config_manager/test_main.cpp`, and `firmware/core/ConfigManager.cpp`, but not `firmware/adapters/WebPortal.cpp`. The story’s “Source Files to Modify” says the opposite important part: it expects `WebPortal.cpp` work and does not list `ConfigManager.cpp`. That drift is not cosmetic; it directly hid the fact that the status API contract was never finished.

## Architectural / Maintainability Notes

- **Under-engineered testability:** [tickNightScheduler()](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:885) is a static `main.cpp` blob that mixes time gating, NTP state, window math, and Core 0 signaling. The tests responded by copy-pasting the arithmetic instead of exercising production logic. Extract the decision logic into a small pure helper and test that helper directly.

## Suggested Fixes

1. Add a real NTP-invalid path. Clear `g_ntpSynced` on WiFi disconnect and on SNTP reset/incomplete events, then update the scheduler and `/api/status` off that real state.
2. Finish the HTTP contract in `WebPortal::_handleGetStatus()`: wire `isScheduleDimming()` in, add `schedule_dimming`, and add an API-level regression test.
3. Decide whether AC #6 or the current “immediate re-override” behavior is authoritative. Right now the code and story disagree. Pick one and align both.
4. Validate `sched_enabled`, `sched_dim_start`, and `sched_dim_end` during `loadFromNvs()` the same way `applyJson()` validates them.
5. Replace the arithmetic-only tests with production-path tests that exercise scheduler state transitions, not just copied formulas.

**Review Actions:** Issues Found: 6. Issues Fixed: 0. Action Items Created: 0.

**Verification:** I did not run a build or test suite in this review; the workspace is read-only, so this is a source-level audit only.

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
<var name="session_id">c52b565c-666c-4488-8e8b-96656c922dfd</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="89294d9b">embedded in prompt, file id: 89294d9b</var>
<var name="story_id">fn-2.2</var>
<var name="story_key">fn-2-2-night-mode-brightness-scheduler</var>
<var name="story_num">2</var>
<var name="story_title">2-night-mode-brightness-scheduler</var>
<var name="template">False</var>
<var name="timestamp">20260413_1009</var>
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