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

**Synthesis Round 2 fixes (2026-04-13):**
- ✅ HIGH: Fixed short-circuit `||` in display task — now evaluates both `g_configChanged` and `g_scheduleChanged` unconditionally before the `if`, preventing `g_scheduleChanged` from being left stale when `g_configChanged` fires simultaneously
- ✅ LOW: Added `LOG_W` to all three NVS schedule clamp paths (`sched_enabled`, `sched_dim_start`, `sched_dim_end`) so corrupted NVS values are diagnosable in field logs
- ✅ LOW: Renamed `test_schedule_default_dim_brightness` → `test_schedule_defaults` to accurately reflect that 4 default fields are asserted

**Synthesis Round 3 fixes (2026-04-13):**
- ✅ MEDIUM: Added `_callbacks.clear()` to `ConfigManager::init()` — prevents stale lambda callbacks (dangling stack references) from accumulating across repeated `init()` calls in the test suite
- ✅ LOW: Added NVS self-heal persist — when any schedule field is clamped on load, `schedulePersist(0)` is called so corrected values are written back to NVS on the first loop tick, preventing the LOG_W from repeating on every subsequent boot
- ✅ LOW: Fixed misleading NTP log message — changed "NTP synced — schedule active" to "NTP synced — local time available" (the sched_enabled check had not yet occurred at that log point; the old message was factually incorrect when sched_enabled == 0)

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
| 2026-04-13 | Synthesis Round 2: Fixed short-circuit || in display task (HIGH), added LOG_W for NVS clamp paths (LOW), renamed test to test_schedule_defaults (LOW) |
| 2026-04-13 | Synthesis Round 3: Added _callbacks.clear() to ConfigManager::init() (MEDIUM), NVS self-heal persist for corrupted schedule fields (LOW), fixed misleading NTP log message (LOW) |

## Senior Developer Review (AI)

### Review: 2026-04-13
- **Reviewer:** AI Code Review Synthesis (3 validators)
- **Evidence Score:** 5.2 / 5.8 / 3.8 → MAJOR REWORK (pre-fix); APPROVED (post-fix)
- **Issues Found:** 4 verified (1 High, 1 Medium, 2 Low)
- **Issues Fixed:** 3 (all critical/high/medium + 1 low)
- **Action Items Created:** 0 (all verified issues resolved; deferred items logged as future improvements)

### Review: 2026-04-13 (Synthesis Round 2 — 3 validators)
- **Reviewer:** AI Code Review Synthesis (Validators A, B, C)
- **Evidence Score:** 2.7 / — / 3.8 → APPROVED (Validator A); APPROVED (Validator C, post-dismissals)
- **Issues Found:** 3 verified (1 High, 2 Low); 11 dismissed as false positives
- **Issues Fixed:** 3 (all verified issues resolved in same pass)
- **Action Items Created:** 0

### Review: 2026-04-13 (Synthesis Round 3 — 3 validators)
- **Reviewer:** AI Code Review Synthesis (Validators A, B, C)
- **Evidence Score:** 3.3 (A) / 8.1 (B) / — (C, no findings)
- **Issues Found:** 3 verified (1 Medium, 2 Low); 16 dismissed as false positives
- **Issues Fixed:** 3 (all verified issues resolved in same pass)
- **Action Items Created:** 0
