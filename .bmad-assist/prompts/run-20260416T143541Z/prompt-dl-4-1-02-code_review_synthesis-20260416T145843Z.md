<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-4 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T145843Z -->
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

Status: review

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

### File List

- firmware/core/ConfigManager.h (modified — added ModeScheduleConfig struct, getModeSchedule/setModeSchedule declarations, NVS key documentation)
- firmware/core/ConfigManager.cpp (modified — added getModeSchedule/setModeSchedule implementations)
- firmware/core/ModeOrchestrator.h (modified — extended tick signature, added _activeRuleIndex/_stateReasonBuf, added ConfigManager.h include)
- firmware/core/ModeOrchestrator.cpp (modified — added schedule evaluation in tick(), extended getStateReason() for SCHEDULED)
- firmware/utils/TimeUtils.h (new — shared minutesInWindow helper)
- firmware/src/main.cpp (modified — added TimeUtils.h include, refactored tickNightScheduler to use minutesInWindow, extended orchestrator tick call with NTP/time params)
- firmware/test/test_mode_schedule/test_main.cpp (new — 21 unit tests for schedule system)

## Change Log

- **2026-04-14**: Implemented all 5 tasks — ModeScheduleConfig structs/NVS, shared minutesInWindow helper, ModeOrchestrator schedule evaluation with SCHEDULED state, main.cpp wiring, and 21 unit tests. Build passes with no new warnings. Status: review.

## Previous story intelligence

- **dl-1.x** defines **orchestrator** + **idle** semantics — **dl-4.1** adds **time** dimension and **priority** over **idle**.

## Git intelligence summary

Touches **`ConfigManager.*`**, **`ModeOrchestrator.*`**, **`main.cpp`**, **`utils/`** or small **shared** time helper, tests.

## Project context reference

`_bmad-output/project-context.md` — **NVS** **15-char** keys, **NTP** gating.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-4-1-schedule-rules-storage-and-orchestrator-integration.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **5** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | AC #2: Rule validation logic is missing from `ConfigManager::setModeSchedule`. The current implementation validates range but fails to check for mode ID existence (documented as "deferred" in Dev Notes but required for AC compliance). | AC #2 | +3.0 |
| 🟠 IMPORTANT | SOLID: `ModeOrchestrator` tick signature is tightly coupled to NTP logic. Violates SRP by forcing orchestrator to know about NTP sync status and POSIX timezone strings. | SOLID | +1.0 |
| 🟡 MINOR | Bug: `g_flightCount` is an atomic but the update in `FlightDataFetcher` happens after a potentially blocking `fetchFlights()` call, leading to a race condition where orchestrator ticks with stale counts. | Bug | +0.3 |
| 🟡 MINOR | Bug: `OTAUpdater` inclusion in `main.cpp` without explicit build flag guard. Risks unnecessary inclusion if OTA features are stripped via compile flags. | Bug | +0.3 |
| 🟡 MINOR | Git Discrepancy: `firmware/utils/TimeUtils.h` was added but not listed in Story File List. | Discrepancy | +0.3 |

### Evidence Score: 4.9

| Score | Verdict |
|-------|---------|
| **4.9** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[6/10] SOLID Violation (Responsibility):** The `ModeOrchestrator::tick` function has become a "God Method" for schedule evaluation, state management, and NTP synchronization. It should receive the results of NTP evaluation rather than taking NTP dependencies directly.
  - 📍 `firmware/core/ModeOrchestrator.cpp:115`
  - 💡 Fix: Pass a `TimeContext` struct into `tick()` containing current time and NTP validity status.

- **[Abstraction Issue]:** The orchestrator is performing implicit conversions between NTP sync state and scheduling logic inside the loop. This leaks timing architecture into business orchestration.
  - 📍 `firmware/src/main.cpp:928`

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug (Race Condition):** `g_flightCount.store(static_cast<uint8_t>(flights.size()))` happens at the end of the `fetchFlights` block in `main.cpp`. The `ModeOrchestrator::tick()` running on the same loop task (Core 1) might trigger evaluation *before* the atomic is updated, or the update might be delayed by network I/O, causing the orchestrator to act on stale state for the duration of the fetch.
  - 📍 `firmware/src/main.cpp:1284`
  - 🔄 Reproduction: Simulate a slow network fetch. `tick()` will operate on the count from the *previous* fetch cycle for the entire duration of the new fetch.

- **🐛 Bug (Watchdog/State):** The watchdog recovery boot (`g_wdtRecoveryBoot`) correctly forces "clock" mode, but the `ModeOrchestrator` state machine is not explicitly updated in the `main.cpp` boot sequence to reflect this forced state, potentially leading to immediate incorrect schedule re-evaluation on the first tick.
  - 📍 `firmware/src/main.cpp:1134`
  - 🔄 Reproduction: WDT reboots during a scheduled window; orchestrator tick evaluates schedule and may override the forced "clock" mode before it has a chance to run.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** The validation logic for mode IDs existence is missing from `ConfigManager::setModeSchedule`. While deferred in Dev Notes, it is an explicit AC requirement (#2).
  - 📍 `firmware/core/ConfigManager.cpp:665`
  - 💥 Explosion radius: High - invalid modes in NVS will cause `requestSwitch` to fail at runtime, potentially leaving the wall in an inconsistent state or forcing fallback.

---

## 🛠️ Suggested Fixes

### 1. Fix AC #2: Rule Validation
**File:** `firmware/core/ConfigManager.cpp`
**Issue:** `setModeSchedule` accepts mode IDs without verifying they exist in `MODE_TABLE`.

**Corrected code:**
```cpp
// Add a helper to ModeRegistry to validate mode ID existence
// Update setModeSchedule to call this validator
for (uint8_t i = 0; i < config.ruleCount; i++) {
    if (!ModeRegistry::isValidModeId(config.rules[i].modeId)) {
        LOG_E("ConfigManager", "Invalid mode ID in schedule");
        return false;
    }
}
```

### 2. Fix Race Condition: Flight Count Updates
**File:** `firmware/src/main.cpp`
**Issue:** Flight count update is too late.

**Corrected code:**
```cpp
// Update g_flightCount IMMEDIATELY after fetch, before processing or rendering
// Also, trigger an immediate orchestrator tick to reflect the new state.
g_flightCount.store(...);
ModeOrchestrator::tick(g_flightCount.load(), ...); 
```

---

## Review Actions Summary

- Issues Found: 5
- Issues Fixed: 0
- Action Items Created: 2

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-4-1-schedule-rules-storage-and-orchestrator-integration.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | AC #2 requires modeId validation at setModeSchedule() time OR explicit documentation of deferred validation, but completion notes claim "validated at orchestrator evaluation time" yet invalid modeIds cause silent failure with no user feedback or error reporting | ConfigManager.cpp:874-877, ModeOrchestrator.cpp:103-106 | +3 |
| 🔴 CRITICAL | invalidateScheduleCache() is declared and defined but NEVER called anywhere, causing schedule cache to become stale after setModeSchedule() changes - configuration updates won't take effect until reboot | ModeOrchestrator.cpp:84-86, ConfigManager.cpp:898-900 | +3 |
| 🔴 IMPORTANT | Task 1 claims modeId validation, but setModeSchedule() only checks if modeId is non-empty string - does NOT verify modeId exists in ModeRegistry table as required by AC #2 | Story line 82, ConfigManager.cpp:874-877 | +1 |
| 🔴 IMPORTANT | Error message "Schedule rule modeId is empty" lacks context (which rule index?) making debugging multi-rule schedules difficult | ConfigManager.cpp:875 | +1 |
| 🟠 IMPORTANT | Test suite has NO tests for invalid modeId scenarios - what happens when schedule contains non-existent modeIds? Only tests happy paths with valid modes | test_mode_schedule/test_main.cpp | +1 |
| 🟠 IMPORTANT | ModeOrchestrator.requestSwitch() failure on schedule exit doesn't update state - could get stuck in SCHEDULED with no matching rule and no error reporting | ModeOrchestrator.cpp:112-118 | +1 |
| 🟡 MINOR | Cache invalidation not documented - story completion notes don't mention invalidateScheduleCache() or the cache strategy, creating maintenance burden | Story lines 82-96 | +0.3 |
| 🟡 MINOR | No integration with SystemStatus for schedule errors - schedule activation failures only logged to serial, not surfaced to UI | ModeOrchestrator.cpp:103-106 | +0.3 |
| 🟢 CLEAN PASS | Security vulnerabilities | 0 | -0.5 |
| 🟢 CLEAN PASS | Style violations | 0 | -0.5 |

### Evidence Score: 8.6

| Score | Verdict |
|-------|---------|
| **8.6** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[7/10] Dependency Violation:** AC #2 requires modeId validation "in ModeRegistry table or reject/clamp — document", but implementation only checks non-empty string. Invalid modeIds are persisted to NVS and fail silently at runtime.
  - 📍 `ConfigManager.cpp:874-877`
  - 💡 Fix: Either (1) Add ModeRegistry lookup in setModeSchedule() with forward declaration, or (2) Document clearly in ConfigManager.h that validation is deferred to runtime and add explicit error handling in ModeOrchestrator when requestSwitch fails for invalid modeId.

- **[6/10] Stale Cache Anti-Pattern:** invalidateScheduleCache() exists but is never called, creating a subtle cache coherency bug. Schedule updates via setModeSchedule() won't take effect until reboot.
  - 📍 `ConfigManager.cpp:898-900`, `ModeOrchestrator.cpp:70-72`
  - 💡 Fix: Call ModeOrchestrator::invalidateScheduleCache() at end of ConfigManager::setModeSchedule() after successful persist, or remove the cache entirely since ConfigManager::getModeSchedule() is fast enough.

- **[5/10] Incomplete Abstraction:** Schedule cache is unnecessary complexity. Architecture Decision DL2 shows calling ConfigManager::getModeSchedule() directly in tick(). Cache adds maintenance burden without measurable performance benefit (8 rules × ~100 bytes = negligible).
  - 📍 `ModeOrchestrator.cpp:48-71`
  - 💡 Fix: Remove s_cachedSchedule and s_scheduleCacheValid. Call ConfigManager::getModeSchedule() directly in every tick() as the architecture specifies.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Performance

- **[MEDIUM] Unnecessary Cache:** Schedule cache optimization for 8 rules is premature optimization. ConfigManager::getModeSchedule() reads ~100 bytes from NVS and is fast enough to call every second. Cache introduces coherency bugs without measurable performance benefit.
  - 📍 `ModeOrchestrator.cpp:48-71`
  - 💡 Fix: Remove cache, call ConfigManager::getModeSchedule() directly per architecture Decision DL2.

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug: Stale Schedule Cache:** invalidateScheduleCache() is never called. When setModeSchedule() updates rules, the orchestrator's cached schedule becomes stale until reboot. Schedule changes won't take effect.
  - 📍 `ModeOrchestrator.cpp:84-86`, `ConfigManager.cpp:898-900`
  - 🔄 Reproduction: 1. Boot device, check current schedule. 2. Call setModeSchedule() to add/modify rules. 3. Wait for 1s tick - schedule evaluation uses old cached rules. 4. Reboot - new rules finally take effect.

- **🐛 Bug: Silent Invalid ModeId Failure:** Invalid modeId passes setModeSchedule() validation (only checks non-empty) but fails silently at runtime when ModeOrchestrator::requestSwitch() is called. Only logs warning to serial, no error reporting to UI.
  - 📍 `ConfigManager.cpp:874-877`, `ModeOrchestrator.cpp:103-106`
  - 🔄 Reproduction: 1. Set schedule rule with modeId="nonexistent_mode". 2. setModeSchedule() returns true. 3. Tick() evaluates schedule, requestSwitch("nonexistent_mode") fails. 4. Warning logged, but state not updated, user sees no error in UI.

- **🐛 Bug: State Corruption on Schedule Exit Failure:** When exiting SCHEDULED state and restoring MANUAL mode, if ModeRegistry::requestSwitch() fails, orchestrator remains stuck in SCHEDULED with no active rule. No error reporting.
  - 📍 `ModeOrchestrator.cpp:112-118`
  - 🔄 Reproduction: 1. Set schedule with valid rule (e.g., 08:00-17:00 → clock). 2. Wait until rule activates (SCHEDULED state). 3. Change saved manual modeId to invalid value via direct NVS edit. 4. Wait until 17:01 - schedule exit tries requestSwitch(invalid_mode), fails. 5. Orchestrator stuck in SCHEDULED, no active rule, no error.

- **🎭 Lying Test: test_schedule_activates_matching_rule:** Test assumes all modeIds in schedule are valid (departures_board, clock, live_flight). Doesn't test what happens with invalid modeId - hides a real-world failure scenario.
  - 📍 `test_mode_schedule/test_main.cpp:248-261`
  - 🤥 Why it lies: Tests only happy paths with valid mode IDs from MODE_TABLE. Never exercises the critical failure path where schedule contains invalid modeId, which is exactly what AC #2 is supposed to validate against.

- **🎭 Lying Test: test_set_get_mode_schedule_roundtrip:** Test validates data round-trips but doesn't validate modeId existence. Creates false confidence that the system rejects invalid configurations.
  - 📍 `test_mode_schedule/test_main.cpp:128-148`
  - 🤥 Why it lies: Verifies that rules can be saved and loaded, but doesn't test the AC #2 requirement that modeId must exist in ModeRegistry. Test passes even with invalid modeIds that would fail at runtime.

✅ No security vulnerabilities detected.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Undocumented Cache Coherency:** invalidateScheduleCache() exists but when/where to call it is undocumented. Future developers adding setModeSchedule() callers won't know they must invalidate cache.
  - 📍 `ModeOrchestrator.h:58`, `ModeOrchestrator.cpp:84-86`
  - 💥 Explosion radius: Medium - schedule updates won't take effect until reboot, confusing users and requiring support intervention.

- **💣 Tech Debt: Poor Error Context:** Schedule validation errors don't include rule index or modeId value, making multi-rule schedule debugging painful.
  - 📍 `ConfigManager.cpp:875`
  - 💥 Explosion radius: Low - increases troubleshooting time for production issues.

- **💣 Tech Debt: Missing Error Reporting:** Schedule errors (invalid modeId, failed requestSwitch) only logged to serial, not surfaced to SystemStatus or UI. Dashboard users can't see why their schedule isn't working.
  - 📍 `ModeOrchestrator.cpp:103-106, 115-118`
  - 💥 Explosion radius: High - users can't diagnose why their configured schedule isn't activating, leading to support tickets and lost trust.

---

## 🛠️ Suggested Fixes

### 1. Fix Stale Cache Bug

**File:** `firmware/core/ConfigManager.cpp`
**Issue:** invalidateScheduleCache() is never called, causing schedule updates to not take effect

**Corrected code:**
```cpp
bool ConfigManager::setModeSchedule(const ModeScheduleConfig& config) {
    // ... existing validation code ...

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for mode schedule write");
        return false;
    }

    // ... existing persistence code ...

    prefs.putUChar("sched_r_count", config.ruleCount);
    prefs.end();

    // FIX: Invalidate orchestrator cache so new rules take effect immediately
    ModeOrchestrator::invalidateScheduleCache();

    LOG_I("ConfigManager", "Mode schedule persisted");
    return true;
}
```

### 2. Add ModeId Validation or Document Deferred Validation

**File:** `firmware/core/ConfigManager.h`
**Issue:** AC #2 requires modeId validation or explicit documentation of deferred validation

**Add after line 107:**
```cpp
    /**
     * Set mode schedule rules.
     * 
     * VALIDATION NOTE: modeId existence in ModeRegistry is NOT validated here
     * to avoid circular dependency. Invalid modeIds will be persisted but will
     * fail silently at runtime when ModeOrchestrator::tick() attempts to switch
     * to the non-existent mode. Callers should verify modeIds against
     * ModeRegistry::getModeTable() before calling this method.
     * 
     * @param config Schedule configuration to persist
     * @return true on success, false if validation fails or NVS write fails
     */
    static bool setModeSchedule(const ModeScheduleConfig& config);
```

### 3. Improve Error Context

**File:** `firmware/core/ConfigManager.cpp`
**Issue:** Error message lacks rule index context

**Diff:**
```diff
-        if (strlen(r.modeId) == 0) {
-            LOG_E("ConfigManager", "Schedule rule modeId is empty");
-            return false;
-        }
+        if (strlen(r.modeId) == 0) {
+            LOG_E("ConfigManager", "Schedule rule %d: modeId is empty", i);
+            return false;
+        }
```

### 4. Add Invalid ModeId Test

**File:** `firmware/test/test_mode_schedule/test_main.cpp`
**Issue:** No tests for invalid modeId scenarios

**Add after line 468:**
```cpp
void test_schedule_invalid_modeId_fails_silently_at_runtime() {
    initAll();

    // Set rule with invalid modeId
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = 1;
    cfg.rules[0].startMin = 480;
    cfg.rules[0].endMin = 1020;
    strncpy(cfg.rules[0].modeId, "nonexistent_mode", MODE_ID_BUF_LEN - 1);
    cfg.rules[0].enabled = 1;
    
    // setModeSchedule() should succeed (only checks non-empty)
    TEST_ASSERT_TRUE(ConfigManager::setModeSchedule(cfg));
    
    // Tick with NTP synced and time in window
    ModeOrchestrator::tick(5, true, 720);
    
    // Orchestrator should NOT enter SCHEDULED state (requestSwitch failed)
    // and should remain in MANUAL state
    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getActiveModeId());
}
```

---

## Review Actions Summary

- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Address the cache invalidation bug, add missing validation or documentation, and improve error reporting before proceeding. The silent failure mode for invalid modeIds is particularly concerning for production reliability.

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
<var name="session_id">be140063-a46d-410a-bb27-7ae3cc0abf48</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="4f121dba">embedded in prompt, file id: 4f121dba</var>
<var name="story_id">dl-4.1</var>
<var name="story_key">dl-4-1-schedule-rules-storage-and-orchestrator-integration</var>
<var name="story_num">1</var>
<var name="story_title">1-schedule-rules-storage-and-orchestrator-integration</var>
<var name="template">False</var>
<var name="timestamp">20260416_1058</var>
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