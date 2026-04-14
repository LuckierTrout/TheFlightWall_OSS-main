<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-2 -->
<!-- Story: 2 -->
<!-- Phase: validate-story-synthesis -->
<!-- Timestamp: 20260413T135308Z -->
<compiled-workflow>
<mission><![CDATA[

Master Synthesis: Story fn-2.2

You are synthesizing 2 independent validator reviews.

Your mission:
1. VERIFY each issue raised by validators
   - Cross-reference with story content
   - Identify false positives (issues that aren't real problems)
   - Confirm valid issues with evidence

2. PRIORITIZE real issues by severity
   - Critical: Blocks implementation or causes major problems
   - High: Significant gaps or ambiguities
   - Medium: Improvements that would help
   - Low: Nice-to-have suggestions

3. SYNTHESIZE findings
   - Merge duplicate issues from different validators
   - Note validator consensus (if 3+ agree, high confidence)
   - Highlight unique insights from individual validators

4. APPLY changes to story file
   - You have WRITE PERMISSION to modify the story
   - CRITICAL: Before using Edit tool, ALWAYS Read the target file first
   - Use EXACT content from Read tool output as old_string, NOT content from this prompt
   - If Read output is truncated, use offset/limit parameters to locate the target section
   - Apply fixes for verified issues
   - Document what you changed and why

Output format:
## Synthesis Summary
## Issues Verified (by severity)
## Issues Dismissed (false positives with reasoning)
## Changes Applied

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
    static bool g_schedDimming = false;  // true when scheduler is actively overriding brightness
    static constexpr unsigned long SCHED_CHECK_INTERVAL_MS = 1000;
    ```
  - [ ] 1.2 Add public accessor for WebPortal (~after `isNtpSynced()` at line 70):
    ```cpp
    bool isScheduleDimming() { return g_schedDimming; }
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

        // Guard: NTP must be synced and schedule must be enabled
        if (sched.sched_enabled != 1 || !g_ntpSynced.load()) {
            if (g_schedDimming) {
                // Was dimming but schedule disabled or NTP lost — restore brightness
                g_schedDimming = false;
                g_configChanged.store(true);  // Signal display task to re-read brightness
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

        if (inDimWindow && !g_schedDimming) {
            g_schedDimming = true;
            g_configChanged.store(true);  // Signal display task
            LOG_I("Scheduler", "Entering dim window — brightness override active");
        } else if (!inDimWindow && g_schedDimming) {
            g_schedDimming = false;
            g_configChanged.store(true);  // Signal display task
            LOG_I("Scheduler", "Exiting dim window — brightness restored");
        }

        // If already in dim window, re-assert override on every check
        // This handles the case where manual brightness was just changed
        // (AC #6: schedule re-overrides manual changes within ~1s)
        if (g_schedDimming) {
            g_configChanged.store(true);
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
  - [ ] 3.1 In the `displayTask()` function's config-change handler (line 314-346), after `g_display.updateBrightness(localDisp.brightness);` (line 344), add scheduler override logic:
    ```cpp
    // Apply scheduler brightness override if active (Story fn-2.2)
    // g_schedDimming is written on Core 1, read here on Core 0 — bool reads
    // are atomic on ESP32 (32-bit aligned), but use a local copy for clarity.
    if (g_schedDimming) {
        ScheduleConfig sched = ConfigManager::getSchedule();
        g_display.updateBrightness(sched.sched_dim_brt);
    }
    ```
  - [ ] 3.2 The `g_schedDimming` variable is a `bool` (1 byte) written on Core 1 and read on Core 0. On ESP32, naturally aligned reads/writes of <= 32 bits are atomic. However, for consistency with the project pattern of using `g_configChanged` atomic to signal display task updates, the scheduler sets `g_configChanged = true` which triggers the existing config-change block where our override logic runs. No additional atomic needed.

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
- **Enforcement Rule #30:** Cross-core atomics live in `main.cpp` only. The `g_schedDimming` bool is written on Core 1 and read on Core 0 via the `g_configChanged` signaling pattern.
- **Core pinning:** Scheduler runs on Core 1 (loop task). Display task runs on Core 0. Communication via `g_configChanged` atomic flag + ConfigManager getters (mutex-protected).
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
1. Scheduler sets `g_schedDimming = true` and signals via `g_configChanged`
2. Display task's config-change handler reads `localDisp.brightness` as normal
3. Then checks `g_schedDimming` — if true, overrides with `sched.sched_dim_brt`
4. This means: ConfigManager always holds the user's "intended" brightness, and the scheduler is a runtime overlay

This design ensures:
- User's brightness preference is never lost
- Exiting the dim window cleanly restores the user's brightness
- No NVS writes for transient brightness overrides
- Manual brightness changes via API persist to NVS as normal (they just get re-overridden in ~1s if still in dim window)

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
static bool g_schedDimming = false;
```

**Display task config-change handler (main.cpp ~line 314-345):**
```cpp
if (g_configChanged.exchange(false)) {
    DisplayConfig newDisp = ConfigManager::getDisplay();
    // ... hardware change checks ...
    localDisp = newDisp;
    g_display.updateBrightness(localDisp.brightness);
    // ADD AFTER: scheduler override
    if (g_schedDimming) {
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
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

# 🎯 Story Context Validation Report

**Story:** fn-2-2-night-mode-brightness-scheduler
**Story Title:** Night Mode Brightness Scheduler
**Story File:** _bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md
**Validated:** 2026-04-13
**Validator:** Quality Competition Engine

---

## Executive Summary

### Issues Overview

| Category | Found | Applied |
|----------|-------|---------|
| 🚨 Critical Issues | 2 | 0 |
| ⚡ Enhancements | 3 | 0 |
| ✨ Optimizations | 2 | 0 |
| 🤖 LLM Optimizations | 1 | 0 |

**Overall Assessment:** MAJOR REWORK REQUIRED - Race condition in cross-core signaling could cause schedule state changes to be lost, violating multiple acceptance criteria and PRD requirements.

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Race condition: g_configChanged flag consumed before schedule state change applied | Task 1.3 lines 99, 128-132, 139-141 + Task 3.1 line 314 | +1 |
| 🟠 IMPORTANT | Missing schedule state tracking - no way to detect state transition vs steady state | Task 1.3 lines 126-141 | +1 |
| 🟡 MINOR | g_schedDimming bool read on Core 0 not protected by atomic | Task 3.1 line 167, Dev Notes line 300 | +0.3 |
| 🟢 CLEAN PASS | INVEST criteria - no major violations | AC #1-8 | -0.5 |
| 🟢 CLEAN PASS | Technical alignment with architecture | Dev Notes, References | -0.5 |

### Evidence Score: 1.3

| Score | Verdict |
|-------|---------|
| **1.3** | **PASS** (with critical issues) |

---

## 🎯 Ruthless Story Validation 2.2

### INVEST Criteria Assessment

| Criterion | Status | Severity | Details |
|-----------|--------|----------|---------|
| **I**ndependent | PASS | 0/10 | Clear dependency on fn-2.1 (NTP sync) which is DONE. No circular dependencies. |
| **N**egotiable | PASS | 0/10 | Correctly focuses on WHAT (scheduler behavior) not HOW (implementation details in Tasks). |
| **V**aluable | PASS | 0/10 | Clear business value - automatic brightness scheduling for livability. |
| **E**stimable | PASS | 2/10 | Generally clear, but cross-core race condition adds uncertainty in implementation complexity. |
| **S**mall | PASS | 0/10 | Appropriately sized - single feature with clear boundaries. |
| **T**estable | WARNING | 4/10 | AC criteria are testable but MISSING critical test for race condition scenario. |

### INVEST Violations

- **[4/10] Testable:** Missing test for scenario where schedule state change AND config change occur between display task iterations - this is where the race condition manifests.

### Acceptance Criteria Issues

- **Ambiguous requirement:** AC#6 says "schedule re-overrides to sched_dim_brt" but doesn't specify HOW the override is signaled or confirmed. The current implementation relies on g_configChanged which may have already been consumed.
- **Untestable scenario:** AC#6 cannot be reliably verified without testing the race condition scenario mentioned above.
- **Missing test case:** No test for rapid config+schedule changes occurring within the same 1-second scheduler interval.

### Hidden Risks and Dependencies

- **Cross-core signaling complexity:** The story assumes g_configChanged is sufficient for all state changes, but doesn't account for the case where both config changes AND schedule state changes happen simultaneously.
- **Impact:** Schedule state changes could be lost, causing LED brightness to not reflect scheduler state.
- **Mitigation:** Add dedicated g_scheduleChanged atomic flag for schedule state transitions (see Critical Issue #1).

- **Bool read atomicity assumption:** Dev Notes line 300 claims "bool reads are atomic on ESP32" but this assumes natural alignment and compiler behavior.
- **Impact:** On rare compiler optimizations or misaligned memory, Core 0 could read stale g_schedDimming value.
- **Mitigation:** Make g_schedDimming atomic (std::atomic<bool>) for safety.

### Estimation Reality-Check

**Assessment:** UNDERESTIMATED

The story appears straightforward (~1-2 hours) but the cross-core race condition adds significant implementation complexity. Developer must:
1. Understand the g_configChanged.exchange(false) semantics
2. Recognize that setting it every second (line 139-141) defeats the purpose
3. Design a solution that doesn't break existing config change handling
4. Test the race condition scenario

This requires deeper analysis than the current story provides. Estimate should be 2-3 hours, not 1-2.

### Technical Alignment

**Status:** ALIGNED

The story correctly references Architecture Decision F5 (Night Mode scheduler), uses correct Enforcement Rules (#14, #15, #30), and follows established patterns (g_configChanged, ConfigManager getters). However, the implementation in Task 1.3 has the critical race condition mentioned above.

### Evidence Score: 1.3 → PASS

---

## 🚨 Critical Issues (Must Fix)

These are essential requirements, security concerns, or blocking issues that could cause implementation disasters.

### 1. Race Condition: g_configChanged Flag Consumption Race

**Impact:** Schedule state changes (entering/exiting dim window) could be LOST if g_configChanged is consumed by another config change before display task processes the schedule transition. This violates AC#2, #3, #5 and PRD requirement "Display settings changes triggered by night mode scheduler must be reflected within 1 second".

**Source:** Task 1.3 lines 99, 128-132, 139-141; Task 3.1 line 314

**Problem:**

The scheduler sets `g_configChanged = true` when schedule state changes (lines 99, 128, 132) and ALSO when already in dim window (line 140). The display task uses `g_configChanged.exchange(false)` (line 314 in existing code) which clears the flag after reading it.

**Scenario where state change is lost:**

```
T=0ms:   Scheduler tick → g_configChanged = true (entering dim window)
T=100ms: User changes brightness via API → g_configChanged already true, no-op
T=500ms: Display task runs → g_configChanged.exchange(false) = true → applies user's brightness (NOT dim brightness!)
T=1000ms: Scheduler tick → already in dim window, sets g_configChanged = true again
T=1500ms: Display task runs → NOW applies dim brightness
```

Result: Brightness was wrong for 1 second (T=500-1500ms), violating the PRD requirement that changes reflect within 1 second.

**Worse scenario - state transition LOST entirely:**

```
T=0ms:   Scheduler tick → NOT in dim window, g_configChanged false
T=100ms: User changes brightness → ConfigManager.onChange sets g_configChanged = true
T=500ms: Display task runs → g_configChanged.exchange(false) = true → applies new brightness, clears flag
T=900ms: Scheduler tick → entering dim window, sets g_configChanged = true
T=1000ms: User changes brightness AGAIN → ConfigManager.onChange sets g_configChanged = true (already true, no-op)
T=1500ms: Display task runs → g_configChanged.exchange(false) = true → applies second brightness change, CLEARS flag
T=2000ms: Scheduler tick → IN dim window, sets g_configChanged = true (again)
```

Result: The T=900ms state transition was "acknowledged" by setting the flag, but the flag was immediately consumed by the T=1000ms brightness change. The dim brightness was applied at T=2000ms-2500ms, but there's no guarantee this always happens within 1 second.

**Root Cause:** The story incorrectly assumes that setting `g_configChanged` every second (line 139-141 "re-assert override on every check") is sufficient. This defeats the purpose of the flag - it should only be set when state CHANGES, not when state REMAINS the same.

**Recommended Fix:**

Add dedicated atomic flag for schedule state changes:

```cpp
// In Task 1.1, add to main.cpp alongside other globals:
static std::atomic<bool> g_scheduleChanged(false);  // Schedule state transition flag

// In Task 1.3, modify tickNightScheduler():
void tickNightScheduler() {
    const unsigned long now = millis();
    if (now - g_lastSchedCheckMs < SCHED_CHECK_INTERVAL_MS) return;
    g_lastSchedCheckMs = now;

    ScheduleConfig sched = ConfigManager::getSchedule();
    bool wasDimming = g_schedDimming;  // Capture previous state

    if (sched.sched_enabled != 1 || !g_ntpSynced.load()) {
        if (wasDimming) {
            g_schedDimming = false;
            g_scheduleChanged.store(true);  // Use dedicated flag
            LOG_I("Scheduler", "Schedule inactive — brightness restored");
        }
        return;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        return;
    }

    uint16_t nowMinutes = (uint16_t)(timeinfo.tm_hour * 60 + timeinfo.tm_min);
    uint16_t dimStart = sched.sched_dim_start;
    uint16_t dimEnd = sched.sched_dim_end;

    bool inDimWindow;
    if (dimStart <= dimEnd) {
        inDimWindow = (nowMinutes >= dimStart && nowMinutes < dimEnd);
    } else {
        inDimWindow = (nowMinutes >= dimStart || nowMinutes < dimEnd);
    }

    // State transition detection
    if (inDimWindow && !wasDimming) {
        g_schedDimming = true;
        g_scheduleChanged.store(true);  // Signal state change
        LOG_I("Scheduler", "Entering dim window — brightness override active");
    } else if (!inDimWindow && wasDimming) {
        g_schedDimming = false;
        g_scheduleChanged.store(true);  // Signal state change
        LOG_I("Scheduler", "Exiting dim window — brightness restored");
    }
    // NOTE: Removed the "re-assert on every check" logic (old lines 136-141)
    // If already in dim window, NO flag needed - state hasn't changed
}

// In Task 3.1, modify display task config-change handler:
if (g_configChanged.exchange(false) || g_scheduleChanged.exchange(false)) {
    DisplayConfig newDisp = ConfigManager::getDisplay();
    // ... existing hardware checks ...
    localDisp = newDisp;
    g_display.updateBrightness(localDisp.brightness);
    
    // Apply scheduler brightness override if active
    if (g_schedDimming) {
        ScheduleConfig sched = ConfigManager::getSchedule();
        g_display.updateBrightness(sched.sched_dim_brt);
    }
}
```

**Also update Task 1.2:**
```cpp
bool isScheduleDimming() { return g_schedDimming; }  // No atomic load needed - read-only accessor
```

**Key changes:**
1. Added `g_scheduleChanged` atomic flag dedicated to schedule state transitions
2. Added `wasDimming` local variable to detect actual state changes
3. Removed the problematic "re-assert on every check" logic
4. Display task checks BOTH flags: `g_configChanged || g_scheduleChanged`
5. `g_schedDimming` remains bool (not atomic) because it's only modified by scheduler and display task only reads it when signaled

### 2. Missing Atomic Protection for g_schedDimming

**Impact:** While ESP32 naturally aligned reads are typically atomic, there's no guarantee of memory ordering. If the compiler optimizes or memory is misaligned, Core 0 display task could read a stale `g_schedDimming` value, causing incorrect brightness to be applied.

**Source:** Task 1.1 line 75; Task 3.1 line 167; Dev Notes line 300

**Problem:**

Task 1.1 declares `g_schedDimming` as plain `bool`, and Dev Notes line 300 claims "bool reads are atomic on ESP32". However:
- This relies on compiler not reordering reads
- This relies on memory being naturally aligned (not guaranteed for all compiler flags)
- There's no memory barrier to ensure Core 0 sees the value written by Core 1
- The ESP32 Xtensa architecture doesn't guarantee atomicity for 8-bit reads in all cases

**Recommended Fix:**

Change `g_schedDimming` to `std::atomic<bool>`:

```cpp
// In Task 1.1, change line 75:
static std::atomic<bool> g_schedDimming(false);  // true when scheduler is actively overriding brightness
```

And in Task 1.3, use `store()`/`load()` methods:
```cpp
// State transition detection
if (inDimWindow && !g_schedDimming.load()) {
    g_schedDimming.store(true);
    g_scheduleChanged.store(true);
    LOG_I("Scheduler", "Entering dim window — brightness override active");
} else if (!inDimWindow && g_schedDimming.load()) {
    g_schedDimming.store(false);
    g_scheduleChanged.store(true);
    LOG_I("Scheduler", "Exiting dim window — brightness restored");
}
```

And in Task 3.1:
```cpp
if (g_schedDimming.load()) {
    ScheduleConfig sched = ConfigManager::getSchedule();
    g_display.updateBrightness(sched.sched_dim_brt);
}
```

This ensures proper memory ordering and atomic access across cores.

---

## ⚡ Enhancement Opportunities (Should Add)

Additional guidance that would significantly help the developer avoid mistakes.

### 1. Missing Test for Race Condition Scenario

**Benefit:** Ensures the critical race condition is actually prevented, not just theoretically addressed.

**Source:** Task 5 (unit tests section)

**Current Gap:**

The tests cover the time window logic (same-day, midnight-crossing, edge cases) but DO NOT test the cross-core signaling scenario that could cause state changes to be lost. This is the most important scenario to test!

**Suggested Addition:**

Add to Task 5.1:

```cpp
void test_schedule_config_change_race_condition() {
    // Simulate scenario where config change AND schedule state change
    // occur between display task iterations.
    // This test verifies that g_scheduleChanged flag prevents state loss.
    
    // Setup: enable schedule, set window for 08:00-17:00
    ConfigManager::applyJson("{\"sched_enabled\":1,\"sched_dim_start\":480,\"sched_dim_end\":1020,\"sched_dim_brt\":10}");
    
    // Simulate: scheduler tick at 07:59 → NOT in window, g_scheduleChanged should be false
    // (In real code, this would call tickNightScheduler())
    // TEST_ASSERT_FALSE(g_scheduleChanged.load());
    
    // Simulate: user changes brightness at 08:00 → g_configChanged = true
    // Simulate: display task consumes g_configChanged → flag cleared
    
    // Simulate: scheduler tick at 08:00:500 → NOW in window, g_scheduleChanged should be true
    // TEST_ASSERT_TRUE(g_scheduleChanged.load());
    TEST_ASSERT_TRUE(g_schedDimming.load());
    
    // Verify: if both flags were set simultaneously, display task handles both
    // (This validates the || operator in the config-change handler)
}
```

### 2. Missing Integration Test for 1-Second Response Time

**Benefit:** Verifies PRD requirement "Display settings changes must be reflected within 1 second" is actually met in practice, not just in theory.

**Source:** PRD line 380, AC #2, #3, #5

**Current Gap:**

The story has unit tests for time window logic but NO integration test that verifies the end-to-end 1-second response time. This is a critical NFR from the PRD.

**Suggested Addition:**

Add to Task 5 as a separate integration test task:

```cpp
// Task 5.3: Integration test for 1-second response time
// This requires a test harness that:
// 1. Enables schedule with window starting 1 second from now
// 2. Simulates time passage (mock getLocalTime)
// 3. Verifies brightness changes within 1000ms of window entry
// 4. Repeats for exit scenario
//
// Note: This test requires mocking NTP time, which may not be available
// in current test framework. Document as "manual verification required" if not automatable.
```

### 3. Missing Log for NTP Sync Regained

**Benefit:** Completes the logging story - AC#4 says "log only on state transition (NTP lost/regained)" but the code only logs when schedule becomes inactive, not when NTP is regained.

**Source:** Task 1.3 lines 94-102; AC #4

**Current Gap:**

The code logs "Schedule inactive — brightness restored" when schedule becomes inactive (lines 100), but there's no log when NTP sync is regained after being lost. This makes debugging difficult.

**Suggested Addition:**

Modify Task 1.3 to add NTP state tracking:

```cpp
// In Task 1.1, add to globals:
static bool g_wasNtpSynced = false;  // Track previous NTP state for logging

// In Task 1.3, modify the guard block:
bool isNtpSynced = g_ntpSynced.load();
if (sched.sched_enabled != 1 || !isNtpSynced) {
    if (g_schedDimming) {
        g_schedDimming = false;
        g_scheduleChanged.store(true);
        LOG_I("Scheduler", "Schedule inactive — brightness restored");
    }
    
    // Log NTP state transition
    if (isNtpSynced && !g_wasNtpSynced) {
        LOG_I("Scheduler", "NTP synced — schedule now active");
    }
    g_wasNtpSynced = isNtpSynced;
    return;
}
g_wasNtpSynced = true;  // NTP is synced when we reach here
```

---

## ✨ Optimizations (Nice to Have)

Performance hints, development tips, and additional context for complex scenarios.

### 1. Optimize ConfigManager Reads in Scheduler

**Value:** Reduces mutex contention - ConfigManager::getSchedule() acquires a mutex, and calling it on every scheduler tick (1/sec) is unnecessary overhead.

**Suggestion:**

In Task 1.3, cache the schedule config and only re-read when schedule key changes (via onChange callback):

```cpp
// In Task 1.1, add to globals:
static ScheduleConfig g_cachedSchedule;  // Cached schedule config

// In ConfigManager onChange callback (already exists in main.cpp), add:
ConfigManager::onChange([]() {
    // ... existing onChange code ...
    
    // Update cached schedule config when any schedule key changes
    g_cachedSchedule = ConfigManager::getSchedule();
    
    g_configChanged.store(true);
    g_layout = LayoutEngine::compute(ConfigManager::getHardware());
});

// In Task 1.3, use cached config:
void tickNightScheduler() {
    const unsigned long now = millis();
    if (now - g_lastSchedCheckMs < SCHED_CHECK_INTERVAL_MS) return;
    g_lastSchedCheckMs = now;

    const ScheduleConfig& sched = g_cachedSchedule;  // Use cached config, no mutex lock
    // ... rest of function uses sched instead of ConfigManager::getSchedule() ...
}
```

### 2. Add Compile-Time Assertion for SCHED_CHECK_INTERVAL_MS

**Value:** Prevents accidental modification that could violate the 1-second response time requirement.

**Suggestion:**

In Task 1.1, add static assertion:

```cpp
#include <static_assert>

// In Task 1.1, after line 76:
static_assert(SCHED_CHECK_INTERVAL_MS >= 1000, "SCHED_CHECK_INTERVAL_MS must be >= 1000ms for 1-second response time");
static_assert(SCHED_CHECK_INTERVAL_MS < 5000, "SCHED_CHECK_INTERVAL_MS must be < 5000ms to avoid long delays");
```

This ensures the interval stays within acceptable bounds even if someone modifies it later.

---

## 🤖 LLM Optimization Improvements

Token efficiency and clarity improvements for better dev agent processing.

### 1. Simplify Midnight-Crossing Logic Explanation

**Issue:** The Dev Notes section (lines 305-319) repeats the same code pattern that's already in Task 1.3, wasting ~15 tokens.

**Token Impact:** ~15 tokens saved

**Current:**
```
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

The `dimStart <= dimEnd` check covers the same-day case naturally. ...
```

**Optimized:**
```
### Midnight-Crossing Logic Pattern

See Task 1.3 lines 116-124 for implementation. Start is inclusive (>=), end is exclusive (<).
```

**Rationale:** The code is already shown in Task 1.3 with detailed comments. Repeating it adds no value. Dev agent can reference the task code directly.

---

## 🏆 Competition Results

### Quality Metrics

| Metric | Score |
|--------|-------|
| Requirements Coverage | 85% |
| Architecture Alignment | 90% |
| Previous Story Integration | 95% |
| LLM Optimization Score | 85% |
| **Overall Quality Score** | **89%** |

### Disaster Prevention Assessment

- **Race Conditions:** FAILED - Critical g_configChanged consumption race identified
- **State Management:** FAILED - Missing dedicated schedule state tracking
- **Atomic Safety:** WARNING - g_schedDimming not protected
- **Test Coverage:** PASSED - Time window logic covered, but missing race condition test
- **Integration Scenarios:** WARNING - No test for 1-second response time requirement

### Competition Outcome

🏆 **Validator identified 2 critical issues + 3 enhancements + 2 optimizations + 1 LLM optimization** that significantly improve the story context. The original create-story produced good structure but missed a critical cross-core race condition that would cause schedule state changes to be lost.

---

**Report Generated:** 2026-04-13
**Validation Engine:** BMAD Method Quality Competition v1.0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

# 🎯 Story Context Validation Report

<!-- report_header -->

**Story:** fn-2.2 - night-mode-brightness-scheduler
**Story File:** _bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md
**Validated:** 2026-04-13
**Validator:** Quality Competition Engine

---

<!-- executive_summary -->

## Executive Summary

### Issues Overview

| Category | Found | Applied |
|----------|-------|---------|
| 🚨 Critical Issues | 2 | 0 |
| ⚡ Enhancements | 1 | 0 |
| ✨ Optimizations | 1 | 0 |
| 🤖 LLM Optimizations | 1 | 0 |

**Overall Assessment:** The story is well-detailed and largely aligns with the architectural vision. However, critical issues were identified regarding architectural logging patterns and the scoping of state variables, which could lead to compile errors or runtime bugs. Several enhancements and LLM optimizations are also suggested for improved clarity and adherence to best practices.

---

<!-- evidence_score_summary -->

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🚨 CRITICAL | Logging Conflict: `Serial.println()` contradicts `architecture.md` | `Disaster Prevention (Technical Spec)` | +3 |
| 🚨 CRITICAL | Global Static Variable Scope Misalignment | `LLM-Dev-Agent Optimization` | +3 |
| ⚡ IMPORTANT | AC4 NTP Loss Logging: Incomplete logging on NTP state change | `Acceptance Criteria Deep Analysis` | +1 |
| ✨ MINOR | Overly Prescriptive `tickNightScheduler()` in Task | `LLM-Dev-Agent Optimization` | +0.3 |
| 🟢 CLEAN PASS | 5 | | -2.5 |

### Evidence Score: 6.8

| Score | Verdict |
|-------|---------|
| **6.8** | **MAJOR REWORK** |

---

<!-- story_quality_gate -->

## 🎯 Ruthless Story Validation fn-2.2

### INVEST Criteria Assessment

| Criterion | Status | Severity | Details |
|-----------|--------|----------|---------|
| **I**ndependent | ✅ Pass | 0/10 | Dependencies on `fn-2.1` and `ConfigManager` are pre-requisites from prior work, not parallel development. |
| **N**egotiable | ⚠️ Warning | 3/10 | The story includes very detailed pseudocode for `tickNightScheduler()` within the task description, making it overly prescriptive for *how* to implement rather than just *what* to achieve. |
| **V**aluable | ✅ Pass | 0/10 | Clearly states the benefit of automatic dimming for comfort and energy saving. |
| **E**stimable | ✅ Pass | 0/10 | Detailed acceptance criteria and implementation guidance make the story highly estimable. |
| **S**mall | ✅ Pass | 0/10 | The scope of changes to existing files is manageable for a single sprint. |
| **T**estable | ✅ Pass | 0/10 | All acceptance criteria are specific and measurable, with explicit test cases provided. |

### INVEST Violations

- **[3/10] Negotiable:** The story provides extensive pseudocode within the task description, prescribing the exact implementation details rather than focusing on the desired outcome. This reduces the flexibility for the developer to make technical decisions.

### Acceptance Criteria Issues

- **Incomplete Scenario:** AC4 "NTP Not Synced Disables Scheduler" specifies "no log spam — log only on state transition (NTP lost/regained)". The pseudocode for `tickNightScheduler()` correctly logs when the scheduler transitions *out of* a dimming state due to NTP loss, but it does not include explicit logging for the event when `g_ntpSynced` *becomes* `false` and the scheduler *becomes* inactive.
  - *Quote:* "And: no log spam — log only on state transition (NTP lost/regained)"
  - *Recommendation:* Add a logging statement in `tickNightScheduler()` that specifically captures and logs the event when `g_ntpSynced` changes from true to false, indicating NTP loss and subsequent scheduler inactivity. This ensures AC4's logging requirement is fully met for both states.

### Hidden Risks and Dependencies

✅ No hidden dependencies or blockers identified.

### Estimation Reality-Check

**Assessment:** Realistic

The story's detailed breakdown and inclusion of pseudocode reduce ambiguity, making the estimation straightforward and realistic.

### Technical Alignment

**Status:** Misaligned

- **Logging Conflict:** The story's proposed solution for logging dynamic strings in Task 1.3 and 3.3 via `Serial.println()` directly contradicts the `architecture.md` rule "Never use raw `Serial.println` in production paths." While an existing pattern in `WiFiManager.cpp` uses `Serial.println` with a log level guard, the architectural mandate is clear.
  - *Architecture Reference:* `_bmad-output/planning-artifacts/architecture.md` (section "Logging Pattern — Compile-Time Log Levels", and enforcement rule 6 "Log via LOG_E/LOG_I/LOG_V from `utils/Log.h` — never raw `Serial.println`")
  - *Recommendation:* Reconcile the logging strategy for dynamic strings. Either:
    1.  Update the `LOG_I` macro to support `String` concatenation (similar to how `LOG_E` is presented in `architecture.md` with `("..." + String(...)).c_str()`).
    2.  Use `snprintf` into a fixed-size buffer before passing to `LOG_I` if dynamic `String` concatenation is undesirable due to heap considerations.
    3.  Clarify and update the `architecture.md` if `Serial.println` with `LOG_LEVEL` guards is an officially accepted exception for dynamic `LOG_I` messages.

### Evidence Score: 6.8 → MAJOR REWORK

---

<!-- critical_issues_section -->

## 🚨 Critical Issues (Must Fix)

These are essential requirements, security concerns, or blocking issues that could cause implementation disasters.

### 1. Logging Conflict: `Serial.println()` contradicts `architecture.md`

**Impact:** Architectural misalignment, potential for inconsistent logging patterns, and difficulty in controlling log verbosity across the codebase if raw `Serial.println` usage becomes widespread, undermining the `utils/Log.h` macro system.

**Source:** `Disaster Prevention (Technical Spec)`, `Technical Alignment`

**Problem:**
The story tasks (Task 1.3 and 3.3) explicitly instruct the developer to use `Serial.println()` with a conditional guard (`#if LOG_LEVEL >= 2`) for dynamic log messages, citing `LOG_I` macro limitations for string concatenation. However, `architecture.md`'s enforcement rule #6 clearly states: "Log via `LOG_E`/`LOG_I`/`LOG_V` from `utils/Log.h` — **never raw `Serial.println`**". This creates a direct contradiction, signaling an architectural pattern break.

**Recommended Fix:**
The `architecture.md` implies that `LOG_E` (and by extension `LOG_I`, etc.) should use the `("..." + String(...)).c_str()` pattern for dynamic logging.
1.  Verify if `LOG_I` can indeed be used with `String` concatenation in the intended way (e.g., `LOG_I("Scheduler", ("Entering dim window — brightness override active, NTP synced: " + String(g_ntpSynced.load())).c_str());`).
2.  If direct `String` concatenation within `LOG_I` is not feasible or desired (e.g., due to heap impact for temporary `String` objects), then explicitly define a helper function or a `snprintf` pattern that allows dynamic messages to be logged through `LOG_I` without using raw `Serial.println`.
3.  Update the story's tasks and `Dev Notes` to reflect the correct architectural logging pattern for dynamic messages.

### 2. Global Static Variable Scope Misalignment

**Impact:** Potential compile-time errors or runtime bugs due to incorrect variable scoping. If `g_lastSchedCheckMs`, `g_schedDimming`, and `SCHED_CHECK_INTERVAL_MS` are declared `static` *inside* `tickNightScheduler()`, they become local to that function and are re-initialized on every call, breaking the scheduler's state. `isScheduleDimming()` would also not be able to access `g_schedDimming`.

**Source:** `LLM-Dev-Agent Optimization`, `Implementation Disasters`

**Problem:**
Task 1.1 states to "Add scheduler state variables after the existing `g_ntpSynced` declaration (~line 67)" with `static` keyword. This implies global static variables (translation unit scope) in `main.cpp`. However, the provided pseudocode for `tickNightScheduler()` in Task 1.3 includes the declaration of these same `static` variables *within the function body*. Declaring them `static` inside a function makes them local to that function but persistent across calls, which is not the intended global state accessible by `isScheduleDimming()` or meant to be set alongside `g_ntpSynced`. For `isScheduleDimming()` to access `g_schedDimming`, `g_schedDimming` must be at translation unit scope.

**Recommended Fix:**
1.  **Correct Scoping:** Move the declarations for `static unsigned long g_lastSchedCheckMs = 0;`, `static bool g_schedDimming = false;`, and `static constexpr unsigned long SCHED_CHECK_INTERVAL_MS = 1000;` to be global static variables in `firmware/src/main.cpp`, alongside `g_ntpSynced`.
2.  **Update Pseudocode:** Ensure the `tickNightScheduler()` pseudocode in Task 1.3 accesses these variables without re-declaring them locally.
3.  **Update Task 1.1:** Clearly state that these variables should be declared at translation unit scope in `main.cpp`.

---

<!-- enhancements_section -->

## ⚡ Enhancement Opportunities (Should Add)

Additional guidance that would significantly help the developer avoid mistakes.

### 1. AC4 NTP Loss Logging: Incomplete logging on NTP state change

**Benefit:** Provides better debugging information and a more complete audit trail for the scheduler's state, improving adherence to AC4's requirement of logging "on state transition (NTP lost/regained)".

**Source:** `Acceptance Criteria Deep Analysis`

**Current Gap:**
AC4 requires logging on state transition (NTP lost/regained). The current pseudocode for `tickNightScheduler()` (Task 1.3) only logs when the scheduler exits a *dimming state* due to NTP loss or `sched_enabled` becoming false: `LOG_I("Scheduler", "Schedule inactive — brightness restored");`. It does not explicitly log the event of NTP *becoming* unsynced if the scheduler was already inactive or not dimming.

**Suggested Addition:**
Add a logging statement in `tickNightScheduler()` that explicitly reports when NTP synchronization is detected as lost, causing the scheduler to become inactive. This would complement the existing logging when the scheduler *recovers* from NTP loss. A boolean flag (e.g., `s_wasNtpSynced`) could be used to detect the transition.

---

<!-- optimizations_section -->

## ✨ Optimizations (Nice to Have)

Performance hints, development tips, and additional context for complex scenarios.

### 1. Overly Prescriptive `tickNightScheduler()` in Task

**Value:** Improves readability and conciseness of the task list, especially for an expert developer agent. Separates high-level instructions from detailed implementation pseudocode.

**Suggestion:**
Relocate the extensive pseudocode for `tickNightScheduler()` (currently in Task 1.3) to a dedicated "Implementation Details" or "Pseudocode Reference" section within the story. The task can then simply refer to this section (e.g., "Implement `tickNightScheduler()` as described in the 'Implementation Details' section, ensuring it adheres to AC1, AC4, AC5."). This makes the task list more scannable and token-efficient.

---

<!-- llm_optimizations_section -->

## 🤖 LLM Optimization Improvements

Token efficiency and clarity improvements for better dev agent processing.

### 1. Global Static Variable Scope Misalignment (LLM Optimization)

**Issue:** Ambiguity issues
**Token Impact:** Minor

**Current:**
```
  - [ ] 1.1 Add scheduler state variables after the existing `g_ntpSynced` declaration (~line 67):
    ```cpp
    // Night mode scheduler state (Story fn-2.2)
    static unsigned long g_lastSchedCheckMs = 0;
    static bool g_schedDimming = false;  // true when scheduler is actively overriding brightness
    static constexpr unsigned long SCHED_CHECK_INTERVAL_MS = 1000;
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
        ...
```

**Optimized:**
```
  - [ ] 1.1 Add the following global static variables in `firmware/src/main.cpp` after `g_ntpSynced` declaration (~line 67):
    ```cpp
    static unsigned long g_lastSchedCheckMs = 0;
    static bool g_schedDimming = false; // true when scheduler is actively overriding brightness
    static constexpr unsigned long SCHED_CHECK_INTERVAL_MS = 1000;
    ```
  - [ ] 1.3 Implement `tickNightScheduler()` function before `loop()` (~line 860), which accesses the global static variables `g_lastSchedCheckMs`, `g_schedDimming`, and `SCHED_CHECK_INTERVAL_MS`. The function logic should be as follows:
    ```cpp
    // (Pseudocode moved to a dedicated section or referenced)
    ```

**Rationale:** The current phrasing and pseudocode create an ambiguity regarding the scope of the scheduler state variables. Task 1.1 implies global statics, but the `tickNightScheduler()` pseudocode re-declares them `static` *inside* the function, which makes them function-local. Clarifying that these variables are global statics and that `tickNightScheduler()` *accesses* them (not re-declares them) ensures correct interpretation by an LLM-Dev-Agent, preventing potential compile errors or runtime bugs. This also helps reduce verbosity by separating declaration from access within the pseudocode.

---

## 🏆 Competition Results

### Quality Metrics

| Metric | Score |
|--------|-------|
| Requirements Coverage | 95% |
| Architecture Alignment | 80% |
| Previous Story Integration | 100% |
| LLM Optimization Score | 70% |
| **Overall Quality Score** | **86%** |

### Disaster Prevention Assessment

- **Reinvention Prevention:** ✅ Pass - No risks of reinventing wheels or duplicating functionality.
- **Technical Specification:** 🔴 Fail - Logging conflict between story's implementation and architectural rules.
- **File Structure:** ✅ Pass - All changes adhere to established file structure and conventions.
- **Regression:** ✅ Pass - Adequate measures are in place to prevent regressions.
- **Implementation:** 🟠 Fail - Ambiguity in variable scoping could lead to implementation errors.

### Competition Outcome

🏆 **Validator identified 2 critical issues, 1 enhancement, and 1 optimization** that enhance the story context, leading to a "Major Rework" verdict. The LLM-Dev-Agent Optimization section also highlighted a critical ambiguity in variable scoping.

---

**Report Generated:** 2026-04-13
**Validation Engine:** BMAD Method Quality Competition v1.0

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-13</var>
<var name="description">Master synthesizes validator findings and applies changes to story file</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-2</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/validate-story-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/validate-story-synthesis/instructions.xml</var>
<var name="name">validate-story-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="session_id">d1dec625-296a-4e17-9fa0-24ff5883c6ea</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="89294d9b">embedded in prompt, file id: 89294d9b</var>
<var name="story_id">fn-2.2</var>
<var name="story_key">fn-2-2-night-mode-brightness-scheduler</var>
<var name="story_num">2</var>
<var name="story_title">2-night-mode-brightness-scheduler</var>
<var name="template">False</var>
<var name="timestamp">20260413_0953</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="validator_count">2</var>
</variables>
<instructions><workflow>
  <critical>Communicate all responses in English and generate all documents in English</critical>

  <critical>You are the MASTER SYNTHESIS agent. Your role is to evaluate validator findings
    and produce a definitive synthesis with applied fixes.</critical>
  <critical>You have WRITE PERMISSION to modify the story file being validated.</critical>
  <critical>All context (project_context.md, story file, anonymized validations) is EMBEDDED below - do NOT attempt to read files.</critical>
  <critical>Apply changes to story file directly using atomic write pattern (temp file + rename).</critical>

  <step n="1" goal="Analyze validator findings">
    <action>Read all anonymized validator outputs (Validator A, B, C, D, etc.)</action>
    <action>For each issue raised:
      - Cross-reference with story content and project_context.md
      - Determine if issue is valid or false positive
      - Note validator consensus (if 3+ validators agree, high confidence issue)
    </action>
    <action>Issues with low validator agreement (1-2 validators) require extra scrutiny</action>
  </step>

  <step n="1.5" goal="Review Deep Verify technical findings" conditional="[Deep Verify Findings] section present">
    <critical>Deep Verify provides automated technical analysis that complements validator reviews.
      DV findings focus on: patterns, boundary cases, assumptions, temporal issues, security, and worst-case scenarios.</critical>

    <action>Review each DV finding:
      - CRITICAL findings: Must be addressed - these indicate serious technical issues
      - ERROR findings: Should be addressed unless clearly false positive
      - WARNING findings: Consider addressing, document if dismissed
    </action>

    <action>Cross-reference DV findings with validator findings:
      - If validators AND DV flag same issue: High confidence, prioritize fix
      - If only DV flags issue: Verify technically valid, may be edge case validators missed
      - If only validators flag issue: Normal processing per step 1
    </action>

    <action>For each DV finding, determine:
      - Is this a genuine issue in the story specification?
      - Does the story need to address this edge case/scenario?
      - Is this already covered but DV missed it? (false positive)
    </action>

    <action>DV findings with patterns (CC-*, SEC-*, DB-*, DT-*, GEN-*) reference known antipatterns.
      Treat pattern-matched findings as higher confidence.</action>
  </step>

  <step n="2" goal="Verify and prioritize issues">
    <action>For verified issues, assign severity:
      - Critical: Blocks implementation or causes major problems
      - High: Significant gaps or ambiguities that need attention
      - Medium: Improvements that would help quality
      - Low: Nice-to-have suggestions
    </action>
    <action>Document false positives with clear reasoning for dismissal:
      - Why the validator was wrong
      - What evidence contradicts the finding
      - Reference specific story content or project_context.md
    </action>
  </step>

  <step n="3" goal="Apply changes to story file">
    <action>For each verified issue (starting with Critical, then High), apply fix directly to story file</action>
    <action>Changes should be natural improvements:
      - DO NOT add review metadata or synthesis comments to story
      - DO NOT reference the synthesis or validation process
      - Preserve story structure, formatting, and style
      - Make changes look like they were always there
    </action>
    <action>For each change, log in synthesis output:
      - File path modified
      - Section/line reference (e.g., "AC4", "Task 2.3")
      - Brief description of change
      - Before snippet (2-3 lines context)
      - After snippet (2-3 lines context)
    </action>
    <action>Use atomic write pattern for story modifications to prevent corruption</action>
  </step>

  <step n="4" goal="Generate synthesis report">
    <critical>Your synthesis report MUST be wrapped in HTML comment markers for extraction:</critical>
    <action>Produce structured output in this exact format (including the markers):</action>
    <output-format>
&lt;!-- VALIDATION_SYNTHESIS_START --&gt;
## Synthesis Summary
[Brief overview: X issues verified, Y false positives dismissed, Z changes applied to story file]

## Validations Quality
[For each validator: name, score, comments]
[Summary of validation quality - 1-10 scale]

## Issues Verified (by severity)

### Critical
[Issues that block implementation - list with evidence and fixes applied]
[Format: "- **Issue**: Description | **Source**: Validator(s) | **Fix**: What was changed"]

### High
[Significant gaps requiring attention]

### Medium
[Quality improvements]

### Low
[Nice-to-have suggestions - may be deferred]

## Issues Dismissed
[False positives with reasoning for each dismissal]
[Format: "- **Claimed Issue**: Description | **Raised by**: Validator(s) | **Dismissal Reason**: Why this is incorrect"]

## Deep Verify Integration
[If DV findings were present, document how they were handled]

### DV Findings Addressed
[List DV findings that resulted in story changes]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Action**: {What was changed}"]

### DV Findings Dismissed
[List DV findings determined to be false positives or not applicable]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Reason**: {Why dismissed}"]

### DV-Validator Overlap
[Note any findings flagged by both DV and validators - these are high confidence]
[If no DV findings: "Deep Verify did not produce findings for this story."]

## Changes Applied
[Complete list of modifications made to story file]
[Format for each change:
  **Location**: [File path] - [Section/line]
  **Change**: [Brief description]
  **Before**:
  ```
  [2-3 lines of original content]
  ```
  **After**:
  ```
  [2-3 lines of updated content]
  ```
]
&lt;!-- VALIDATION_SYNTHESIS_END --&gt;
    </output-format>

  </step>

  <step n="5" goal="Final verification">
    <action>Verify all Critical and High issues have been addressed</action>
    <action>Confirm story file changes are coherent and preserve structure</action>
    <action>Ensure synthesis report is complete with all sections populated</action>
  </step>
</workflow></instructions>
<output-template></output-template>
</compiled-workflow>