<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-4 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T151344Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-4.1

You are synthesizing 2 independent code review findings.

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
<file id="4f121dba" path="_bmad-output/implementation-artifacts/stories/dl-4-1-schedule-rules-storage-and-orchestrator-integration.md" label="DOCUMENTATION"><![CDATA[

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


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic dl-4 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-4-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `invalidateScheduleCache()` declared and defined but NEVER called after `setModeSchedule()`, causing schedule changes to not take effect until device reboot | Added `ModeOrchestrator::invalidateScheduleCache()` call at end of `setModeSchedule()` after successful NVS persist, with comment explaining AC #2 requirement. |
| critical | AC #2 requires modeId validation but implementation only checks non-empty string — invalid modeIds are persisted to NVS and fail silently at runtime with no user feedback | Added comprehensive documentation to ConfigManager.h explaining deferred validation strategy (modeId existence cannot be checked at `setModeSchedule()` time due to circular dependency). Added inline comment in `.cpp` explaining this is intentional architectural decision per AC #2. |
| high | Missing test coverage for invalid modeId scenarios — test suite only covers happy paths with valid mode IDs | DEFERRED — Added as action item below. Test would verify that schedule with invalid modeId is accepted by `setModeSchedule()` but fails gracefully at runtime when orchestrator tick evaluates. |
| dismissed | SOLID violation — `ModeOrchestrator::tick` signature tightly coupled to NTP logic, forcing orchestrator to know about NTP sync status | FALSE POSITIVE: False positive. The orchestrator receives **results** of NTP evaluation (bool + uint16_t), not NTP internals. This is dependency injection via parameters, which is correct SOLID design. The tick signature is clean and testable. |
| dismissed | Race condition on `g_flightCount` — atomic update happens after potentially blocking `fetchFlights()` call, leading to stale counts | FALSE POSITIVE: False positive. Line 1288 in main.cpp updates `g_flightCount` immediately after queue publish (line 1281), which is the correct pattern. The fetch operation completes before the atomic store, so orchestrator always operates on the most recent snapshot. This is by design. |
| dismissed | WDT recovery boot forces "clock" mode but orchestrator state machine not updated, leading to immediate schedule re-evaluation | FALSE POSITIVE: Not verified in source code. WDT recovery logic (lines 1131-1138 in main.cpp) is separate from orchestrator initialization. The claim lacks evidence of a concrete bug. |
| dismissed | Premature optimization — schedule cache for 8 rules (~100 bytes) adds maintenance burden without measurable performance benefit | FALSE POSITIVE: While the cache does add complexity, it's a reasonable optimization for a 1Hz tick rate to avoid repeated NVS reads. The **bug** was that the cache wasn't being invalidated, which has now been fixed. The cache itself is not the problem. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-4-1-schedule-rules-storage-and-orchestrator-integration.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **2** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | AC #2 deferred modeId validation strategy is incomplete and potentially dangerous as invalid NVS state persists indefinitely | ConfigManager.cpp | +3 |
| 🟠 IMPORTANT | Test suite lacks coverage for invalid modeId scenarios | Test suite | +1 |
| 🟢 CLEAN PASS | 7 | -3.5 |

### Evidence Score: 0.5

| Score | Verdict |
|-------|---------|
| **0.5** | **APPROVED** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug: Improper ModeId Validation Strategy (AC #2):**
  - 📍 `firmware/core/ConfigManager.cpp`
  - 🔄 Reproduction: `ConfigManager::setModeSchedule()` persists any `modeId` string provided, even if it does not exist in `ModeRegistry::MODE_TABLE`. The `ModeOrchestrator::tick()` evaluation logic will fail to switch to the invalid mode (per `ModeRegistry::requestSwitch()` check), leaving the system in a potentially inconsistent state where the schedule believes it is `SCHEDULED` (because the rule matched) but the actual mode did not change. The invalid modeId remains in NVS indefinitely.
  - 💡 Fix: `ModeOrchestrator` should proactively validate schedule rules against `ModeRegistry` at schedule-save time, OR the system should implement a "rollback to safe mode" mechanism if an invalid mode is encountered during `ModeOrchestrator::tick()`.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Deferred Test Coverage:**
  - 📍 `firmware/test/test_mode_schedule/test_main.cpp`
  - 💥 Explosion radius: High. The absence of tests for invalid modeId scenarios creates a blind spot where system behavior on misconfiguration is undefined and untested.

---

## 🛠️ Suggested Fixes

### 1. Robust ModeID Validation (AC #2 Improvement)

**File:** `firmware/core/ConfigManager.cpp`
**Issue:** `ModeId` validation is deferred to runtime in `ModeOrchestrator`, leaving invalid rules persisted in NVS.

**Corrected logic (conceptual):**
```cpp
// Add a validation hook to ModeRegistry that ConfigManager can call without circular dependencies.
// Or, implement a post-write validation in ModeOrchestrator::tick() that disables rules
// with invalid modeIds and marks them in NVS.
```

---

## Review Actions Summary

- Issues Found: 2
- Issues Fixed: 0
- Action Items Created: 1

✅ Code is approved and ready for deployment!

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** dl-4.1 - schedule-rules-storage-and-orchestrator-integration

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 3 |
| Hidden Bugs | 1 |
| Performance Footguns | 2 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Cache invalidation bug in setModeSchedule() - schedule changes don't take effect until reboot | ConfigManager.cpp:925 | +3 |
| 🟠 IMPORTANT | SOLID violation - ModeOrchestrator::tick() signature tightly coupled to NTP, passing bool+uint16_t instead of encapsulated time | ModeOrchestrator.h:45 | +1 |
| 🟠 IMPORTANT | SOLID violation - ConfigManager.h documents deferred modeId validation but provides no API for callers to validate, forcing duplicate validation logic | ConfigManager.h:158-163 | +1 |
| 🟠 IMPORTANT | SOLID violation - ModeOrchestrator::tick() has too many responsibilities (schedule eval + idle fallback + manual override), violates Single Responsibility | ModeOrchestrator.cpp:137-211 | +1 |
| 🟡 MINOR | Tech debt - static schedule cache (s_cachedSchedule) in ModeOrchestrator.cpp could be encapsulated in a class for better testability | ModeOrchestrator.cpp:20-21 | +0.3 |
| 🟡 MINOR | Performance - ModeOrchestrator::tick() rebuilds schedule cache on every call when invalid, could be lazy-loaded | ModeOrchestrator.cpp:137 | +0.3 |
| 🟢 CLEAN PASS | Security (0 issues) | | -0.5 |
| 🟢 CLEAN PASS | Style violations (0 issues) | | -0.5 |
| 🟢 CLEAN PASS | Type safety (0 issues) | | -0.5 |

### Evidence Score: 5.1

| Score | Verdict |
|-------|---------|
| **5.1** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[7/10] SOLID - Single Responsibility:** ModeOrchestrator::tick() handles three distinct concerns: schedule evaluation, idle fallback logic, and manual override transitions. This violates SRP - each should be a separate method with clear responsibility boundaries.
  - 📍 `ModeOrchestrator.cpp:137-211`
  - 💡 Fix: Extract schedule evaluation into `_evaluateSchedule()`, idle fallback into `_handleIdleFallback()`, and orchestrate them in `tick()`.

- **[6/10] SOLID - Open/Closed:** ModeOrchestrator::tick() signature is tightly coupled to NTP implementation details, requiring changes if time source changes. Passing `bool ntpSynced` and `uint16_t nowMinutes` as separate parameters exposes internal representation.
  - 📍 `ModeOrchestrator.h:45`
  - 💡 Fix: Create a `TimeContext` struct encapsulating NTP sync status and current time, making the interface more flexible for future time sources (e.g., manual time override, GPS time).

- **[5/10] SOLID - Dependency Inversion:** ConfigManager.h documents that modeId validation is deferred to avoid circular dependency, but provides no API for callers to validate. This forces duplicate validation logic in WebPortal (dl-4.2), violating DRY and creating maintenance burden.
  - 📍 `ConfigManager.h:158-163`
  - 💡 Fix: Add `static bool isValidModeId(const char* modeId)` method that queries ModeRegistry internally (using forward declaration to avoid circular dep), allowing callers to validate before calling setModeSchedule().

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows C++ style guidelines and is readable. No style violations detected.

---

## ⚡ Performance & Scalability

- **[Medium Impact] Cache rebuild inefficiency:** ModeOrchestrator::tick() rebuilds the entire schedule cache from NVS on every call when invalid. For a 1Hz tick rate with 8 rules (~100 bytes), this is negligible, but it's unnecessary work.
  - 📍 `ModeOrchestrator.cpp:137`
  - 💡 Fix: Lazy-load cache only when needed and rebuild once after `invalidateScheduleCache()` call, not on every tick iteration.

- **[Low Impact] Static cache state:** Using file-level static variables (`s_cachedSchedule`, `s_scheduleCacheValid`) makes unit testing difficult and couples the cache lifetime to program startup.
  - 📍 `ModeOrchestrator.cpp:20-21`
  - 💡 Fix: Encapsulate cache in a `ScheduleCache` class with `invalidate()`, `reload()`, and `get()` methods for better testability and encapsulation.

---

## 🐛 Correctness & Safety

- **🐛 Bug: Cache invalidation not called after setModeSchedule():** The `ModeOrchestrator::invalidateScheduleCache()` method exists but is NEVER called after `ConfigManager::setModeSchedule()` succeeds. This means schedule changes made via API (dl-4.2) won't take effect until device reboot, violating AC #2 which requires immediate visibility.
  - 📍 `ConfigManager.cpp:925` (missing call), `ModeOrchestrator.cpp:72-74` (method exists but unused)
  - 🔄 Reproduction: Call `setModeSchedule()` with new rules, then call `ModeOrchestrator::tick()` - the old cached schedule will still be used.
  - 💡 Fix: Add `ModeOrchestrator::invalidateScheduleCache()` call at end of `ConfigManager::setModeSchedule()` after successful NVS persist.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Hardcoded schedule cache pattern:** The file-level static cache pattern (`s_cachedSchedule`, `s_scheduleCacheValid`) is a maintenance burden. Adding new cached fields requires scattered modifications across multiple files.
  - 📍 `ModeOrchestrator.cpp:20-21`, `ModeOrchestrator.cpp:72-74`, `ModeOrchestrator.cpp:137-141`
  - 💥 Explosion radius: Medium - affects testability and makes future cache-related changes error-prone.

---

## 🛠️ Suggested Fixes

### 1. Cache Invalidation Bug

**File:** `firmware/core/ConfigManager.cpp`
**Issue:** Schedule changes not visible until reboot due to missing cache invalidation call

**Diff:**
```diff
     // Invalidate ModeOrchestrator schedule cache so new rules take effect immediately
     // (Story dl-4.1, AC #2: changes must be visible to orchestrator on next tick)
     ModeOrchestrator::invalidateScheduleCache();
 
     LOG_I("ConfigManager", "Mode schedule persisted");
     return true;
```

---

### 2. Extract Schedule Evaluation

**File:** `firmware/core/ModeOrchestrator.cpp`
**Issue:** tick() method violates Single Responsibility by handling schedule eval, idle fallback, and manual overrides

**Diff:**
```diff
 void ModeOrchestrator::tick(uint8_t flightCount, bool ntpSynced, uint16_t nowMinutes) {
     // Story dl-4.1: Evaluate mode schedule rules when NTP is synced.
     // Schedule evaluation runs BEFORE idle fallback logic so SCHEDULED
     // takes priority over IDLE_FALLBACK (AC #6).
 
     if (ntpSynced) {
-        // Use cached schedule if valid, reload from NVS on first call or after invalidation
-        if (!s_scheduleCacheValid) {
-            s_cachedSchedule = ConfigManager::getModeSchedule();
-            s_scheduleCacheValid = true;
-        }
-        ModeScheduleConfig sched = s_cachedSchedule;
-
-        // Find the first (lowest index) enabled rule whose window matches now (AC #5)
-        int8_t matchIndex = -1;
-        for (uint8_t i = 0; i < sched.ruleCount; i++) {
-            const ScheduleRule& r = sched.rules[i];
-            if (r.enabled == 1 && minutesInWindow(nowMinutes, r.startMin, r.endMin)) {
-                matchIndex = (int8_t)i;
-                break;
-            }
-        }
-
-        if (matchIndex >= 0) {
-            // A schedule rule matches — enter or stay in SCHEDULED state
-            const ScheduleRule& matched = sched.rules[matchIndex];
-
-            if (_state != OrchestratorState::SCHEDULED || _activeRuleIndex != matchIndex) {
-                // Entering SCHEDULED or switching to a different rule
-                // AC #9: only issue requestSwitch from tick (or onManualSwitch)
-                if (!ModeRegistry::requestSwitch(matched.modeId)) {
-                    LOG_W("ModeOrch", "Schedule: requestSwitch failed for rule mode");
-                    return;
-                }
-                _state = OrchestratorState::SCHEDULED;
-                _activeRuleIndex = matchIndex;
-                strncpy(_activeModeId, matched.modeId, sizeof(_activeModeId) - 1);
-                _activeModeId[sizeof(_activeModeId) - 1] = '\0';
-                // Update active mode display name from mode table
-                const ModeEntry* table = ModeRegistry::getModeTable();
-                uint8_t count = ModeRegistry::getModeCount();
-                for (uint8_t i = 0; i < count; i++) {
-                    if (strcmp(table[i].id, matched.modeId) == 0) {
-                        strncpy(_activeModeName, table[i].displayName, sizeof(_activeModeName) - 1);
-                        _activeModeName[sizeof(_activeModeName) - 1] = '\0';
-                        break;
-                    }
-                }
-                LOG_I("ModeOrch", "Schedule rule activated");
-            }
-            // AC #6: SCHEDULED + flightCount == 0 → do NOT invoke idle fallback
-            return;
-        } else if (_state == OrchestratorState::SCHEDULED) {
-            // AC #4: no rule matches and was SCHEDULED → transition to MANUAL
-            if (!ModeRegistry::requestSwitch(_manualModeId)) {
-                LOG_W("ModeOrch", "Schedule exit: requestSwitch failed for manual mode");
-                return;
-            }
-            _state = OrchestratorState::MANUAL;
-            _activeRuleIndex = -1;
-            // Restore owner's manual selection (both ID and name)
-            strncpy(_activeModeId, _manualModeId, sizeof(_activeModeId) - 1);
-            _activeModeId[sizeof(_activeModeId) - 1] = '\0';
-            strncpy(_activeModeName, _manualModeName, sizeof(_activeModeName) - 1);
-            _activeModeName[sizeof(_activeModeName) - 1] = '\0';
-            LOG_I("ModeOrch", "Schedule window ended, back to MANUAL");
-            // Fall through to idle fallback logic below in case flightCount is 0
-        }
+        _evaluateSchedule(nowMinutes);
     }
 
     // State transition logic per architecture.md#DL2
     // AC #6: SCHEDULED state is handled above and returns early — never reaches here
     if (flightCount == 0 && _state == OrchestratorState::IDLE_FALLBACK) {
         onIdleFallback();
     } else if (flightCount > 0 && _state == OrchestratorState::IDLE_FALLBACK) {
         onFlightsRestored();
     }
 }
+
+void ModeOrchestrator::_evaluateSchedule(uint16_t nowMinutes) {
+    // Use cached schedule if valid, reload from NVS on first call or after invalidation
+    if (!s_scheduleCacheValid) {
+        s_cachedSchedule = ConfigManager::getModeSchedule();
+        s_scheduleCacheValid = true;
+    }
+    ModeScheduleConfig sched = s_cachedSchedule;
+
+    // Find the first (lowest index) enabled rule whose window matches now (AC #5)
+    int8_t matchIndex = -1;
+    for (uint8_t i = 0; i < sched.ruleCount; i++) {
+        const ScheduleRule& r = sched.rules[i];
+        if (r.enabled == 1 && minutesInWindow(nowMinutes, r.startMin, r.endMin)) {
+            matchIndex = (int8_t)i;
+            break;
+        }
+    }
+
+    if (matchIndex >= 0) {
+        // A schedule rule matches — enter or stay in SCHEDULED state
+        const ScheduleRule& matched = sched.rules[matchIndex];
+
+        if (_state != OrchestratorState::SCHEDULED || _activeRuleIndex != matchIndex) {
+            // Entering SCHEDULED or switching to a different rule
+            // AC #9: only issue requestSwitch from tick (or onManualSwitch)
+            if (!ModeRegistry::requestSwitch(matched.modeId)) {
+                LOG_W("ModeOrch", "Schedule: requestSwitch failed for rule mode");
+                return;
+            }
+            _state = OrchestratorState::SCHEDULED;
+            _activeRuleIndex = matchIndex;
+            strncpy(_activeModeId, matched.modeId, sizeof(_activeModeId) - 1);
+            _activeModeId[sizeof(_activeModeId) - 1] = '\0';
+            // Update active mode display name from mode table
+            const ModeEntry* table = ModeRegistry::getModeTable();
+            uint8_t count = ModeRegistry::getModeCount();
+            for (uint8_t i = 0; i < count; i++) {
+                if (strcmp(table[i].id, matched.modeId) == 0) {
+                    strncpy(_activeModeName, table[i].displayName, sizeof(_activeModeName) - 1);
+                    _activeModeName[sizeof(_activeModeName) - 1] = '\0';
+                    break;
+                }
+            }
+            LOG_I("ModeOrch", "Schedule rule activated");
+        }
+        // AC #6: SCHEDULED + flightCount == 0 → do NOT invoke idle fallback
+        return;
+    } else if (_state == OrchestratorState::SCHEDULED) {
+        // AC #4: no rule matches and was SCHEDULED → transition to MANUAL
+        if (!ModeRegistry::requestSwitch(_manualModeId)) {
+            LOG_W("ModeOrch", "Schedule exit: requestSwitch failed for manual mode");
+            return;
+        }
+        _state = OrchestratorState::MANUAL;
+        _activeRuleIndex = -1;
+        // Restore owner's manual selection (both ID and name)
+        strncpy(_activeModeId, _manualModeId, sizeof(_activeModeId) - 1);
+        _activeModeId[sizeof(_activeModeId) - 1] = '\0';
+        strncpy(_activeModeName, _manualModeName, sizeof(_manualModeName) - 1);
+        _activeModeName[sizeof(_activeModeName) - 1] = '\0';
+        LOG_I("ModeOrch", "Schedule window ended, back to MANUAL");
+    }
+}
```

---

### 3. Add ModeId Validation API

**File:** `firmware/core/ConfigManager.h` and `ConfigManager.cpp`
**Issue:** No API for callers to validate modeIds before calling setModeSchedule(), forcing duplicate validation logic

**Diff in ConfigManager.h:**
```diff
     // VALIDATION NOTE (AC #2): modeId existence in ModeRegistry is NOT validated
     // here to avoid circular dependency. Invalid modeIds will be persisted but
     // will fail when ModeOrchestrator::tick() attempts to switch. Callers
     // should verify modeIds against ModeRegistry::getModeTable() before calling
     // setModeSchedule().
+    //
+    // Helper API: Callers can use isValidModeId() to validate before persisting.
+    // This avoids duplicate validation logic in WebPortal and other consumers.
+    static bool isValidModeId(const char* modeId);
     static ModeScheduleConfig getModeSchedule();
     static bool setModeSchedule(const ModeScheduleConfig& config);
```

**Diff in ConfigManager.cpp:**
```diff
 #include "core/ConfigManager.h"
 #include "core/ModeOrchestrator.h"
+#include "core/ModeRegistry.h"
 #include <Preferences.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/semphr.h>
 #include "utils/Log.h"
```

```diff
     LOG_I("ConfigManager", "Mode schedule persisted");
     return true;
 }
+
+// ModeId validation helper (Story dl-4.1, AC #2)
+// Allows callers to validate modeIds before calling setModeSchedule().
+// Avoids circular dependency by using forward declaration and inline call.
+bool ConfigManager::isValidModeId(const char* modeId) {
+    if (modeId == nullptr || strlen(modeId) == 0) return false;
+    
+    const ModeEntry* table = ModeRegistry::getModeTable();
+    uint8_t count = ModeRegistry::getModeCount();
+    
+    for (uint8_t i = 0; i < count; i++) {
+        if (strcmp(table[i].id, modeId) == 0) {
+            return true;
+        }
+    }
+    return false;
+}
```

---

## Review Actions Summary

- Issues Found: 6
- Issues Fixed: 0
- Action Items Created: 3

⚠️ Address the identified issues before proceeding. The cache invalidation bug (CRITICAL) must be fixed before deploying to production. The SOLID violations are code quality concerns that should be refactored to improve maintainability.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-16</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">dl-4</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">2</var>
<var name="session_id">306b8bb8-19d3-41c5-8518-e2d064c0b7f7</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="4f121dba">embedded in prompt, file id: 4f121dba</var>
<var name="story_id">dl-4.1</var>
<var name="story_key">dl-4-1-schedule-rules-storage-and-orchestrator-integration</var>
<var name="story_num">1</var>
<var name="story_title">1-schedule-rules-storage-and-orchestrator-integration</var>
<var name="template">False</var>
<var name="timestamp">20260416_1113</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-4.1)
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