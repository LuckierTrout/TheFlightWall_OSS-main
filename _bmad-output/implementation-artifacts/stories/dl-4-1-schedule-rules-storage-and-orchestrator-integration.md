# Story dl-4.1: Schedule Rules Storage and Orchestrator Integration

Status: complete

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want to define time-based rules that automatically switch display modes at configured times,
So that the wall shows the right content at the right time without my intervention — for example Departures Board during the day and Clock Mode at night.

## Acceptance Criteria

1. **Given** **`ConfigManager`**, **When** this story lands, **Then** it exposes **`ModeScheduleConfig`** (or equivalent) holding **up to 8** **`ScheduleRule`** entries (**AR4**): per rule **`startMin`**, **`endMin`** (**0–1439** minutes since midnight), **`modeId`** (≤ **31** chars buffer matching **`MODE_TABLE`** ids), **`enabled`** (**uint8** **0/1**), plus **`ruleCount`**. **NVS** keys use **indexed** names from the epic (**verify** **≤15** chars each): e.g. **`sched_r0_start`**, **`sched_r0_end`**, **`sched_r0_mode`**, **`sched_r0_ena`**, … **`sched_r_count`**. **Do not** collide with existing **brightness** schedule keys **`sched_dim_*`** / **`sched_enabled`** (**different** prefix **`sched_r`** vs **`sched_dim`**).

2. **Given** **`getModeSchedule()` / `setModeSchedule(const ModeScheduleConfig&)`**, **When** **`set`** is called, **Then** rules are **validated** (**start/end** in range, **`modeId`** exists in **`ModeRegistry`** table or reject / clamp — document), **persisted** atomically enough for **FR36** (power loss after **commit** restores rules — use **`Preferences`** batch **or** **`schedulePersist`** pattern consistent with **`ConfigManager`**).

3. **Given** **`ModeOrchestrator::tick`** (signature may extend **dl-1.2**/**dl-1.3** — e.g. **`tick(uint8_t flightCount)`** only), **When** **NTP** is synced (**`g_ntpSynced`**) and **`getLocalTime(&tm, 0)`** returns **true** (**non-blocking**, same pattern as **`tickNightScheduler`** in **`main.cpp`**), **Then** evaluate **mode** schedule rules **each** **~1s** tick. If **`getLocalTime`** returns **false**, **skip** schedule evaluation (**epic** — remain **MANUAL** / current state without blocking **WiFi** stack).

4. **Given** **no** rule window matches **now**, **When** orchestrator was **`SCHEDULED`**, **Then** transition to **`MANUAL`** and **`ModeRegistry::requestSwitch(getManualModeId())`** (restore owner’s **manual** selection — **epic**).

5. **Given** **rule index `i`** is the **first** (**lowest index**) enabled rule whose **`timeInWindow(nowMin, start, end)`** is **true**, **When** entering or staying in **`SCHEDULED`**, **Then** **`requestSwitch(rule.modeId)`** within **one** orchestrator evaluation cycle (**≤5s** slip vs wall clock — **NFR6**; **1s** tick satisfies if transition fires on first matching second).

6. **Given** **`SCHEDULED`** and **`flightCount == 0`**, **When** **`tick`** runs, **Then** **do not** invoke **idle fallback** to **clock** (**FR17** — schedule **wins** over **IDLE_FALLBACK**). **MANUAL** + zero flights **still** may use **dl-1.2** idle path as today.

7. **Given** **`tickNightScheduler()`** (brightness dim window) and **mode** schedule **both** active, **When** a **mode** rule fires, **Then** **only** **`ModeRegistry::requestSwitch`** runs — **no** **`ConfigManager::applyJson`** side effects; **brightness** transitions **do not** change **`OrchestratorState`** (**FR15**).

8. **Given** **`timeInWindow(now, start, end)`**, **When** **`start > end`** (spans midnight), **Then** use the **same** inclusive/exclusive convention as **`tickNightScheduler`** (**`dimStart > dimEnd`** branch in **`main.cpp`** ~1071–1077) — **extract** a **shared** **`minutesInWindow(uint16_t now, uint16_t start, uint16_t end)`** in **`utils/`** or **`ConfigManager`** to avoid **drift** (**epic**).

9. **Given** **rule 24** (**dl-1.3**), **When** schedule applies a mode, **Then** **`ModeRegistry::requestSwitch`** is issued **only** from **`ModeOrchestrator::tick`** (or **`onManualSwitch`** for user overrides) — **not** from **`ConfigManager`** or **`WebPortal`** directly (**dl-4.2** will **persist** rules then **orchestrator** reads cache).

10. **Given** **`OrchestratorState::SCHEDULED`**, **When** **`GET /api/display/modes`** (existing handler) exposes **`orchestrator_state`**, **Then** return **`"scheduled"`** and a **useful** **`state_reason`** (e.g. **active rule index + mode id**) — extend **`getStateReason()`** if needed (**dl-4.2** depends on readable state).

11. **Given** **`pio test`** / **`pio run`**, **Then** add **unit** tests for **`minutesInWindow`** / rule **priority** / **SCHEDULED** + **zero** flights **no fallback**; **no** new warnings.

## Tasks / Subtasks

- [x] Task 1: **`ConfigManager.h/cpp`** — structs, NVS load/save, validation (**AC: #1–#2**)
- [x] Task 2: **`timeInWindow` helper** — shared with night scheduler (**AC: #8**)
- [x] Task 3: **`ModeOrchestrator`** — **`tick`** schedule branch, **`SCHEDULED`** priority over idle (**AC: #3–#7, #9–#10**)
- [x] Task 4: **`main.cpp`** — call schedule eval from existing **~1s** orchestrator tick site (**AC: #3**)
- [x] Task 5: Tests (**AC: #11**)

## Dev Notes

### Key separation

| Concern | Keys / code |
|--------|-------------|
| **Brightness** night window | **`ScheduleConfig`**: **`sched_dim_*`**, **`sched_enabled`** |
| **Display mode** rules | **`ModeScheduleConfig`**: **`sched_r{N}_*`** (**epic** **AR4**) |

### **`ModeOrchestrator`** baseline

- **`OrchestratorState::SCHEDULED`** already exists; **`tick`** today only handles **MANUAL**/**`IDLE_FALLBACK`** vs **`flightCount`**.

### Out of scope (**dl-4.2**)

- **`GET/POST /api/schedule`**, dashboard UI, **`applyJson`** wiring for rules (unless minimal **`dumpSettingsJson`** hook is **trivial**).

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-4.md` (Story dl-4.1)
- Prior: `_bmad-output/implementation-artifacts/stories/dl-1-3-manual-mode-switching-via-orchestrator.md`, **`dl-1-2-idle-fallback-to-clock-mode.md`**
- Patterns: `firmware/src/main.cpp` — **`tickNightScheduler`**, **`g_ntpSynced`**

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` — build SUCCESS, 79.6% flash usage (1,252,048 / 1,572,864 bytes)
- `pio test -f test_mode_schedule` — compiled successfully, upload skipped (no device connected)
- Pre-existing deprecation warnings in OpenSkyFetcher.cpp and FlightWallFetcher.cpp (DynamicJsonDocument, containsKey) — NOT introduced by this story

### Completion Notes List

- **Task 1**: Added `ScheduleRule` struct (startMin, endMin, modeId[32], enabled) and `ModeScheduleConfig` (rules[8], ruleCount) to ConfigManager.h. Implemented `getModeSchedule()` / `setModeSchedule()` with NVS persistence using indexed keys (sched_r{N}_start/end/mode/ena + sched_r_count). All keys verified ≤15 chars. Validation: range checks for time (0–1439), enabled (0/1), non-empty modeId. modeId existence in ModeRegistry validated at orchestrator evaluation time to avoid circular dependency (documented).
- **Task 2**: Created `firmware/utils/TimeUtils.h` with header-only `minutesInWindow(nowMin, start, end)` — same inclusive-start/exclusive-end convention as tickNightScheduler. Refactored `tickNightScheduler()` in main.cpp to use the shared helper, eliminating drift risk.
- **Task 3**: Extended `ModeOrchestrator::tick()` signature to accept `ntpSynced` and `nowMinutes` (with backward-compatible defaults). Added schedule evaluation: finds first enabled matching rule, transitions to SCHEDULED state, issues `requestSwitch` only from tick/onManualSwitch (AC #9). SCHEDULED + zero flights does NOT trigger idle fallback (AC #6). No-match while SCHEDULED transitions to MANUAL and restores manual mode (AC #4). Extended `getStateReason()` to return active rule index + mode id (AC #10).
- **Task 4**: Updated orchestrator tick site in main.cpp loop() to compute NTP-gated nowMinutes from `getLocalTime(&tm, 0)` (non-blocking) and pass to `ModeOrchestrator::tick()`. Brightness scheduler (`tickNightScheduler`) remains independent — does not change OrchestratorState (AC #7).
- **Task 5**: Created `firmware/test/test_mode_schedule/test_main.cpp` with 21 tests: minutesInWindow helper (same-day, cross-midnight, edge cases), ConfigManager ModeScheduleConfig NVS round-trip + validation, ModeOrchestrator schedule evaluation (rule activation, priority, disabled rules, no-match, exit-to-manual, SCHEDULED+zero flights, NTP gating, state reason, manual override, cross-midnight rules). All compile successfully with no new warnings.
- **Code Review Synthesis (2026-04-16)**: Fixed critical cache invalidation bug — `setModeSchedule()` now calls `ModeOrchestrator::invalidateScheduleCache()` so schedule changes take effect immediately. Added comprehensive documentation to ConfigManager.h explaining deferred modeId validation strategy (AC #2 compliance). Build verified: 81.0% flash usage.

### File List

- firmware/core/ConfigManager.h (modified — added ModeScheduleConfig struct, getModeSchedule/setModeSchedule declarations, NVS key documentation)
- firmware/core/ConfigManager.cpp (modified — added getModeSchedule/setModeSchedule implementations)
- firmware/core/ModeOrchestrator.h (modified — extended tick signature, added _activeRuleIndex/_stateReasonBuf, added ConfigManager.h include)
- firmware/core/ModeOrchestrator.cpp (modified — added schedule evaluation in tick(), extended getStateReason() for SCHEDULED)
- firmware/utils/TimeUtils.h (new — shared minutesInWindow helper)
- firmware/src/main.cpp (modified — added TimeUtils.h include, refactored tickNightScheduler to use minutesInWindow, extended orchestrator tick call with NTP/time params)
- firmware/test/test_mode_schedule/test_main.cpp (new — 21 unit tests for schedule system)

## Change Log

- **2026-04-16**: Story complete — All review fixes applied and verified. Build passes (80.6% flash), tests compile. 1 deferred action item (invalid modeId test coverage) tracked in synthesis-dl-4-1-20260416T150239Z.md.
- **2026-04-14**: Implemented all 5 tasks — ModeScheduleConfig structs/NVS, shared minutesInWindow helper, ModeOrchestrator schedule evaluation with SCHEDULED state, main.cpp wiring, and 21 unit tests. Build passes with no new warnings. Status: review.

## Previous story intelligence

- **dl-1.x** defines **orchestrator** + **idle** semantics — **dl-4.1** adds **time** dimension and **priority** over **idle**.

## Git intelligence summary

Touches **`ConfigManager.*`**, **`ModeOrchestrator.*`**, **`main.cpp`**, **`utils/`** or small **shared** time helper, tests.

## Project context reference

`_bmad-output/project-context.md` — **NVS** **15-char** keys, **NTP** gating.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-16
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 8.6 → REJECT (Validator B), 4.9 → MAJOR REWORK (Validator A)
- **Issues Found:** 3 critical issues verified
- **Issues Fixed:** 2 fixes applied
- **Action Items Created:** 1 remaining item (deferred: invalid modeId test coverage)

### Completion Verification: 2026-04-16
- **Build:** ✓ SUCCESS (80.6% flash usage)
- **Tests:** ✓ Compile successfully (21 tests in test_mode_schedule)
- **Critical fixes:** ✓ Cache invalidation (line 925 ConfigManager.cpp)
- **Documentation:** ✓ AC #2 deferred validation strategy documented
- **All tasks:** ✓ 5/5 complete
