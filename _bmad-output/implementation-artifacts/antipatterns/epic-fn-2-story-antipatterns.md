# Epic fn-2 - Story Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during validation of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent story-writing mistakes (unclear AC, missing Notes, unrealistic scope).

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Task 4.3 lacked a concrete location for the `isNtpSynced()` extern declaration, leaving the developer to guess between a new header (violates "no new files" rule), an existing header (wrong ownership), or inline in `WebPortal.cpp` (correct). | Task 4.3 now specifies `extern bool isNtpSynced();` goes at the top of `firmware/adapters/WebPortal.cpp`, with rationale tied to the no-new-files project rule. |
| medium | AC #1 timing criterion ("within 10 seconds") could mislead a dev agent into writing a flaky unit test, since Task 7 only defines unit tests and NTP timing depends on external network conditions. | Added inline clarification to AC #1 marking this as an integration test / device observation criterion, not a unit test requirement. |
| low | Task 5.1 description was slightly verbose ("with ~60 common IANA-to-POSIX mappings"). | Minor wording tightened to "~60 IANA-to-POSIX mappings" in parentheses. |
| dismissed | `TZ_MAP` static content creates a long-term maintenance burden if IANA DST rules change. | FALSE POSITIVE: The static map is the correct architectural choice for a resource-constrained ESP32 device. Dynamic IANA data fetching is impractical on this platform. The story scope is intentionally limited; updating the map for DST rule changes is a routine maintenance activity no different from any other firmware constant. The "What This Story Does NOT Include" section already scopes this correctly. No story change warranted. --- |

## Story fn-2-2 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `g_schedDimming` declared as plain `bool` across cores | Changed to `std::atomic<bool> g_schedDimming(false)` in Task 1.1, updated all reads to `.load()` and writes to `.store()` throughout Tasks 1.2, 1.3, 3.1, Dev Notes patterns. Provides proper memory ordering on Xtensa dual-core. |
| critical | "Re-assert g_configChanged every second" anti-pattern — floods display task with unnecessary wakeups and conflates schedule state transitions with config changes on a shared flag | Added dedicated `std::atomic<bool> g_scheduleChanged(false)` flag. `tickNightScheduler()` now only signals on actual state transitions (enter/exit dim window). Display task condition expanded to `g_configChanged.exchange(false) \|\| g_scheduleChanged.exchange(false)`. AC#6 (re-override manual brightness) is now handled immediately by the config-change handler checking `g_schedDimming.load()` — which is better than the "wait ~1s" the re-assert approach implied. |
| high | AC#4 NTP logging incomplete — code only logged when transitioning from dimming→inactive, but not when NTP transitions lost↔regained while scheduler was already idle | Added `static bool g_lastNtpSynced = false` to Task 1.1 globals. Added NTP state transition detection block at the top of `tickNightScheduler()` that logs "NTP synced — schedule active" and "NTP lost — schedule suspended" on each respective edge. |
| medium | No compile-time guard against accidentally increasing `SCHED_CHECK_INTERVAL_MS` beyond the 1-second response NFR | Added `static_assert(SCHED_CHECK_INTERVAL_MS <= 2000, ...)` after the constant declaration in Task 1.1. |
| dismissed | Logging uses `Serial.println()` contradicting Enforcement Rule #6 | FALSE POSITIVE: The story uses `LOG_I` macros throughout (e.g., `LOG_I("Scheduler", "Schedule inactive — brightness restored")`). There is no `Serial.println` anywhere in the story's code blocks. Validator B hallucinated this issue entirely. |
| dismissed | Variable scope misalignment — `static void tickNightScheduler()` re-declares state variables as function-local statics | FALSE POSITIVE: False positive. `static void tickNightScheduler()` gives the function *file-scope linkage*, not variable scope. The function body references `g_lastSchedCheckMs`, `g_schedDimming`, etc. by name without any redeclaration — these are the globals from Task 1.1. This is standard C++. No compile error or scope issue exists. |
| dismissed | Missing race condition unit test in Task 5 | FALSE POSITIVE: The race condition is resolved architecturally (dedicated `g_scheduleChanged` flag + `std::atomic<bool>`). A unit test for this in the native PlatformIO test harness would require access to FreeRTOS internals and ESP32 core assignments that aren't available in native simulation. The time-window logic tests remain the appropriate scope for this test file. |
| dismissed | Missing integration test for 1-second response time | FALSE POSITIVE: This is a valid NFR concern but requires hardware-in-the-loop testing, which is outside the scope of the `test_config_manager` native unit test suite. The story's architecture (scheduler at ~1s + display task triggered by atomic flag) structurally guarantees the NFR. Manual verification during hardware testing is the appropriate gate. |
| dismissed | Cache ConfigManager reads in scheduler to reduce mutex contention | FALSE POSITIVE: ConfigManager mutex acquisition at 1/sec is negligible on ESP32. Adding a `g_cachedSchedule` introduces a synchronization complexity (when to invalidate, race between cache write in onChange and read in tickNightScheduler) that far outweighs the benefit of saving one mutex acquire per second. --- |

## Story fn-2-3 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | AC #6 requires "a brief note explains the schedule will not run until NTP syncs" — Task 6.1 schedule status block ran the `schedule_active`/`schedule_dimming` checks independently of `ntp_synced`, producing no explanatory text when NTP is absent. | Added `!s.ntp_synced` as the first branch in the schedule status block, emitting `'Schedule paused — waiting for NTP sync'` |
| medium | AC #7 specifies the status text `'Schedule active (not in dim window)'` — Task 6.1 code had `'Schedule active'` (truncated). | Updated the `schedule_active` branch text to match AC exactly. |
| low | `updateNmFieldState()` mutated `nmFields.style.opacity` inline rather than toggling a CSS class. Inline style manipulation bypasses the CSS `transition:opacity 0.2s` rule already on `.nm-fields` and gives no hook for `pointer-events` blocking. | Changed to `nmFields.classList.toggle('nm-fields-disabled', !on)` and added `.nm-fields-disabled{opacity:0.5;pointer-events:none}` to the Task 7.1 CSS block. |
| dismissed | `TZ_MAP` is duplicated in `dashboard.js` and `wizard.js` — should be extracted to `common.js`. | FALSE POSITIVE: The duplication is pre-existing technical debt (both files existed before fn-2.3). This story correctly *reuses* the already-present `TZ_MAP` in `dashboard.js` without creating a new copy. Refactoring to `common.js` would require out-of-scope modifications to `wizard.js` and `common.js`, and directly contradicts the story's explicit "No new files" constraint. Filed as future tech-debt, not a defect in this story. |
| dismissed | "Update Firmware" OTA push UI is still present in `dashboard.html` despite an OTA Pull story. | FALSE POSITIVE: Completely out of scope. fn-2.3 does not touch the Firmware card and makes no claims about OTA UI. This is a separate product concern for a different story. --- |
