<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-1 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260415T132449Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-1.2

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
<file id="87850beb" path="_bmad-output/implementation-artifacts/stories/dl-1-2-idle-fallback-to-clock-mode.md" label="DOCUMENTATION"><![CDATA[

# Story dl-1.2: Idle Fallback to Clock Mode

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want the wall to automatically switch to Clock Mode when no flights are in range and switch back when flights return,
So that the wall never shows a blank screen or loading spinner — guests always see a functioning display.

## Acceptance Criteria

1. **Given** **`"clock"`** is registered in **`MODE_TABLE`** (**dl-1.1**), **When** the orchestrator determines **idle fallback** (see AC #3–#5), **Then** **`ModeRegistry::requestSwitch("clock")`** is invoked so the **display task** actually activates **Clock Mode** — **not** only **`ModeOrchestrator`** string fields.

2. **Given** **`IDLE_FALLBACK`** and flights return (**`flightCount > 0`**), **When** **`onFlightsRestored()`** runs, **Then** **`ModeRegistry::requestSwitch(ModeOrchestrator::getManualModeId())`** (or equivalent) restores the owner’s **saved manual** mode id (**`_manualModeId`**). If **`requestSwitch`** returns **false**, log **`LOG_W`**, keep orchestrator state consistent with registry truth (**document** reconciliation — e.g. leave **MANUAL** but surface **`registry_error`** on next **GET**).

3. **Given** **`std::atomic<uint8_t> g_flightCount`** (rule **30**), **When** the firmware publishes the latest enriched flight list to the display pipeline (**`xQueueOverwrite`** path in **`main.cpp`** today), **Then** **`g_flightCount.store()`** reflects **`min(flights.size(), 255)`**. **Epic** cites **`FlightDataFetcher.cpp`** — acceptable to update in **`main.cpp`** immediately after **`fetchFlights`** / queue publish **if** you document that the **count** is tied to the **same** snapshot the display consumes (avoid drift).

4. **Given** **Core 1** **`loop()`**, **When** time advances, **Then** **`ModeOrchestrator::tick(g_flightCount.load())`** runs at **approximately 1-second** intervals (**non-blocking**, no busy-wait longer than **`delay(1)`** style), **independent** of whether a **fetch** is due — so **boot** + **zero** flights transitions within **~1s** after the first tick, not only on the next **10+ minute** fetch (**epic** AC). **Keep** existing **`tick(...)`** call on the fetch path **or** replace with a single periodic site — **do not** double-tick the same wall-clock second unless idempotent (**`onIdleFallback`** already guards re-entry).

5. **Given** a **non-clock** mode is active (**registry** active id ≠ **`"clock"`**) and **`flightCount == 0`** and orchestrator state is **`MANUAL`**, **When** **`tick`** evaluates, **Then** transition to **`IDLE_FALLBACK`** and request **clock** (**epic**).

6. **Given** orchestrator **`MANUAL`**, **`flightCount == 0`**, but registry **already** shows **`"clock"`** (user manually picked clock), **When** **`tick`** runs, **Then** behavior is **safe** (no flip-flop): either **no-op** state transition or enter **`IDLE_FALLBACK`** without a second **`requestSwitch`** — document chosen behavior.

7. **Given** **`OrchestratorState::SCHEDULED`**, **When** **`tick`** runs, **Then** **do not** override schedule logic (**no** change unless **dl-1.4+** defines interaction — default **leave** **`tick`** branches as today for **`SCHEDULED`**).

8. **Given** **NFR** “within one poll interval” for zero-flight detection **after** data exists, **Then** worst-case latency to fallback is **≤ max(fetch_interval, ~1s)`** once **`g_flightCount`** reflects zero — document in Dev Agent Record.

9. **Given** **epic API surface**, **When** compiling, **Then** expose **`getManualModeId()`** on **`ModeOrchestrator`** if still missing (returns **`_manualModeId`**) — used by tests and **GET** transparency if needed.

10. **Given** **`test_mode_orchestrator`**, **When** extended, **Then** mock or assert **`ModeRegistry::requestSwitch`** call counts (**link test** or refactor orchestrator to accept a thin callback — **prefer** minimal **integration** test on device over heavy mocking if impractical).

11. **Given** **`pio run`**, **Then** no new warnings.

## Tasks / Subtasks

- [x] Task 1: **`g_flightCount`** (**AC: #3**)
  - [x] 1.1: Declare **`static std::atomic<uint8_t> g_flightCount(0)`** in **`main.cpp`** (Rule #30: cross-core atomics stay in main.cpp)
  - [x] 1.2: **`store`** immediately after flight queue publish + initialized to **0** at boot

- [x] Task 2: Periodic **`ModeOrchestrator::tick`** (**AC: #4**)
  - [x] 2.1: **`static unsigned long s_lastOrchMs`** in **`loop()`**; if **`millis() - s_lastOrchMs >= 1000`** → **`tick(g_flightCount.load())`**; removed old fetch-path tick to avoid double-tick

- [x] Task 3: **`ModeRegistry`** bridge (**AC: #1, #2**)
  - [x] 3.1: **`onIdleFallback`**: calls **`ModeRegistry::requestSwitch("clock")`** after state update; logs LOG_W on failure
  - [x] 3.2: **`onFlightsRestored`**: calls **`ModeRegistry::requestSwitch(_manualModeId)`**; logs LOG_W on failure, keeps orchestrator in MANUAL (reconciliation documented)
  - [x] 3.3: Verified **`onManualSwitch`** does NOT call **`requestSwitch`** — **`POST /api/display/mode`** in WebPortal only calls orchestrator. **Out of scope for dl-1.2; follow-up for dl-1.3** (manual mode switching via orchestrator story).

- [x] Task 4: **`getManualModeId()`** + tests (**AC: #9, #10**)

## Dev Notes

### Current implementation (gap analysis)

- **`ModeOrchestrator::tick(uint8_t)`** only runs from **`main.cpp`** **inside** the **fetch-due** block — **not** every second.
- **`onIdleFallback` / `onFlightsRestored`** update **orchestrator** mirrors only — **no** **`ModeRegistry::requestSwitch`** anywhere from orchestrator (**grep** verified).
- **`WebPortal::POST /api/display/mode`** calls **`ModeOrchestrator::onManualSwitch`** only — registry may lag (**dl-1.3** territory).

### Hard dependency

- **dl-1.1** — **`"clock"`** in **`MODE_TABLE`** and **`ClockMode`**; without it, **`requestSwitch("clock")`** fails.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-1.md` (Story dl-1.2)
- Prior: `_bmad-output/implementation-artifacts/stories/dl-1-1-clock-mode-time-display.md`
- Code: `firmware/core/ModeOrchestrator.{h,cpp}`, `firmware/src/main.cpp` (**fetch** + **`loop()`**)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, no new warnings (AC #11 satisfied)
- Pre-existing warning: `s_lastAppliedTz` capture — not introduced by this story

### Completion Notes List

- **AC #1**: `onIdleFallback()` now calls `ModeRegistry::requestSwitch("clock")` to drive the display task into Clock Mode, not just orchestrator string fields.
- **AC #2**: `onFlightsRestored()` calls `ModeRegistry::requestSwitch(_manualModeId)` to restore the owner's saved mode. On failure, logs `LOG_W` and keeps orchestrator in IDLE_FALLBACK state to maintain consistency with registry truth (reconciliation: registry error will surface on next GET).
- **Code Review Synthesis (2026-04-15)**: Fixed critical state corruption bug — `onManualSwitch`, `onIdleFallback`, and `onFlightsRestored` now call `requestSwitch()` BEFORE updating orchestrator state fields, ensuring orchestrator state remains consistent with ModeRegistry truth when switch fails.
- **AC #3**: `std::atomic<uint8_t> g_flightCount` declared in `main.cpp` (Rule #30), stored immediately after `xQueueOverwrite` — tied to the same snapshot the display consumes.
- **AC #4**: Periodic 1-second tick via `s_lastOrchMs` in `loop()`, independent of fetch interval. Old fetch-path tick removed (single periodic site).
- **AC #5**: `tick()` logic: `MANUAL + flightCount==0` → `IDLE_FALLBACK` + `requestSwitch("clock")`.
- **AC #6**: Safe when clock already active — `onIdleFallback` re-entry guard prevents double transition. `requestSwitch("clock")` is called once; `ModeRegistry::tick` skips switch if target == active index. Documented behavior: enters `IDLE_FALLBACK` state, single `requestSwitch`, no flip-flop.
- **AC #7**: `SCHEDULED` state not overridden — tick only branches on `MANUAL` and `IDLE_FALLBACK` conditions. No change for `SCHEDULED` (deferred to dl-1.4+).
- **AC #8**: Worst-case fallback latency: ≤ max(fetch_interval, ~1s) once `g_flightCount` reflects zero. The 1-second periodic tick detects zero flights within ~1s of the count being stored.
- **AC #9**: `getManualModeId()` exposed as public static method on `ModeOrchestrator`.
- **AC #10**: Integration tests added to `test_mode_orchestrator`: `test_idle_fallback_requests_clock_in_registry`, `test_flights_restored_requests_manual_mode_in_registry`, `test_idle_fallback_safe_when_clock_already_active`. Used real ModeRegistry with production mode table rather than heavy mocking (per story preference).
- **AC #11**: `pio run` produces no new warnings.
- **dl-1.3 note**: Task 3.3 completion note was stale — `onManualSwitch` DOES call `requestSwitch` (implemented in dl-1.3, visible in dl-1.2 review due to shared ModeOrchestrator component).

### File List

- `firmware/core/ModeOrchestrator.h` — added `getManualModeId()` declaration
- `firmware/core/ModeOrchestrator.cpp` — added `ModeRegistry.h` include, `getManualModeId()` implementation, `requestSwitch` calls in `onIdleFallback` and `onFlightsRestored`
- `firmware/src/main.cpp` — added `g_flightCount` atomic, periodic 1s orchestrator tick, `g_flightCount.store()` after queue publish, removed old fetch-path orchestrator tick
- `firmware/test/test_mode_orchestrator/test_main.cpp` — added ModeRegistry integration tests, `getManualModeId` tests, updated to init both registry and orchestrator

### Change Log

- 2026-04-14: Story dl-1.2 implemented — idle fallback bridges ModeOrchestrator to ModeRegistry, periodic tick, g_flightCount atomic, getManualModeId()
- 2026-04-15: Code review synthesis — fixed state corruption bug in `onManualSwitch`, `onIdleFallback`, `onFlightsRestored` (requestSwitch before state update)
- 2026-04-15: Post-review validation pass — all ACs verified, build succeeds with no new warnings, story marked done

## Previous story intelligence

- **dl-1.1** adds **Clock** as a real mode — **dl-1.2** must **drive** **`ModeRegistry`** into **`clock`** on idle.

## Git intelligence summary

Touches **`main.cpp`**, **`ModeOrchestrator.cpp`**, possibly **`ModeOrchestrator.h`**, **`test_mode_orchestrator`**.

## Project context reference

`_bmad-output/project-context.md` — **Core 1** **`loop()`**, **cross-core** **`requestSwitch`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-15
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 6.3 (Validator A), 7.1 (Validator B) → REJECT/MAJOR REWORK
- **Issues Found:** 1 critical (state corruption)
- **Issues Fixed:** 1
- **Action Items Created:** 0



]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic dl-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-1-5 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `deserializeJson(reqDoc, data, total)` passes `total` bytes to the parser but `data` points to the current chunk only (`len` bytes). On a multi-chunk request body `(index > 0)`, this reads past the end of the chunk buffer — undefined behaviour. | Changed `total` → `len`. For the tiny `{"mode_id": "..."}` payload this never manifests in practice, but the UB was real. |
| medium | `.catch` block in `loadDisplayModes()` was a no-op with the comment "Silent fail on load". Architecture rule (dev notes) states: *"Every `fetch()` in dashboard JS must check `json.ok` and call `showToast()` on failure."* Network errors silently left the mode picker stuck on "loading". | Added `FW.showToast('Cannot reach device to load display modes. Check connection.', 'error')`. |
| dismissed | "Bypassed ModeRegistry::tick() — displayTask calls g_display.renderFlight() directly" | FALSE POSITIVE: `ModeRegistry` does not exist yet — `firmware/test/test_mode_registry/test_main.cpp` has 100% `TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented")`. Stories ds-1-3 and ds-1-5 (ModeRegistry) are `backlog`. This complaint is against a future architecture, not the current code. |
| dismissed | "Missing `g_flightCount` atomic (architecture.md#DL2)" | FALSE POSITIVE: The `g_flightCount` atomic in architecture.md is specified for Core 1 → Core 0 signaling when ModeRegistry::tick() runs in the display task (Core 0). In this implementation, `ModeOrchestrator::tick()` is called from the main loop on **Core 1** — same core as flight fetch. There is no cross-core boundary. Story dev notes explicitly address this: *"ModeOrchestrator state is read/written from Core 1 only (orchestrator tick + web handler), so no atomic needed."* |
| dismissed | "False source of truth — dashboard status shows 'Clock' but LEDs never switch to ClockMode" | FALSE POSITIVE: Structurally correct but **out of scope**. Sprint status confirms dl-1.1 (ClockMode), dl-1.2 (idle fallback), dl-1.3 (manual switching), dl-1.4 (watchdog) are all `backlog`. The story itself describes it as implementing dashboard feedback for the orchestrator state — the ClockMode rendering is the prerequisite that was never completed. This is tracked in AI-Review follow-up tasks. |
| dismissed | "Boot state always resets to classic_card — ignores persisted display_mode" | FALSE POSITIVE: NVS mode persistence is explicitly story ds-3-2 (`backlog`). There is no persisted `display_mode` key in ConfigManager yet. Implementing persistence here would pre-empt a separate story's scope. |
| dismissed | "Hardcoded mode list in WebPortal OCP violation" | FALSE POSITIVE: ModeRegistry (ds-1-3) is `backlog`. The hardcoded list is the correct interim approach; the code even carries the comment *"static for now; will grow with ModeRegistry in future stories."* When ModeRegistry ships, this will be replaced as part of that story. |
| dismissed | "`switch_state` hardcoded to `"idle"` — doesn't reflect real switch lifecycle" | FALSE POSITIVE: The switch state lifecycle (REQUESTED/SWITCHING/IDLE) lives in ModeRegistry (ds-1-3, `backlog`). There is no source of truth for `switch_state` yet. The hardcoded `"idle"` is safe for now and correctly deferred. |
| dismissed | "`SCHEDULED` state not handled in `tick()`" | FALSE POSITIVE: Story dev notes explicitly state: *"The SCHEDULED state and its reason strings are NOT implemented in this story."* `SCHEDULED` belongs to dl-4 (`backlog`). The safe fallback (`"scheduled"/"scheduled"`) is correctly wired in `getStateReason()` and `getStateString()`. |
| dismissed | "Unsynchronized shared state — GET/POST can observe mixed orchestrator snapshots" | FALSE POSITIVE: AsyncTCP callbacks run on Core 1; the main loop tick also runs on Core 1. Architecture dev notes (Rule 30) explicitly document: *"ModeOrchestrator state is read/written from Core 1 only."* FreeRTOS task preemption between two consecutive `strncpy` calls is theoretically possible but the window is on the order of nanoseconds for a 30-char copy; no defensive locking is required or specified. |
| dismissed | "Task 1.1 lies — `getStateReason()` was pre-existing, not new" | FALSE POSITIVE: Reviewer C misread the story. `firmware/core/ModeOrchestrator.h` is a **new file** in this story (listed under "New files" in the file list). `getStateReason()` was authored as part of this story, not pre-existing. |
| dismissed | "`strstr` in test instead of `TEST_ASSERT_EQUAL_STRING` — weak assertion" | FALSE POSITIVE: The test comment explains why: *"Reason contains em-dash UTF-8."* Using `strstr` to check substrings avoids platform-dependent encoding comparison issues when running on-device. The exact string `"idle fallback — no flights"` is fully covered; the `strstr` checks both the prefix and suffix. Weak but intentional. |
| dismissed | "UTF-8 em-dash in `const char*` literal may display incorrectly" | FALSE POSITIVE: The em-dash is encoded as the explicit UTF-8 byte sequence `\xe2\x80\x94`. This is the standard cross-platform way to embed Unicode in C string literals. The ESP32 serves this over HTTP to a browser — both sides handle UTF-8 natively. |
| dismissed | "Heap fragmentation from `String output; serializeJson(doc, output)`" | FALSE POSITIVE: This is an existing pattern used throughout the entire `WebPortal.cpp` file (pre-dates this story). The story adds two new handlers using the same pattern. Changing the response serialisation approach is a project-wide refactor, not a story-specific fix. |
| dismissed | "Legacy `var` usage in dashboard.js" | FALSE POSITIVE: The entire 2600-line `dashboard.js` predates this story and uses `var` consistently. The Mode Picker code added in this story correctly follows the file's existing style convention. |
| dismissed | "CSS minor issues (fallback values, font-weight inheritance)" | FALSE POSITIVE: Cosmetic and subjective; the existing fallback values (`rgba(255,255,255,0.1)`) match the actual `--settings-border` token value. Not a defect. --- |

## Story dl-1-1 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Architecture Rule 18 Violation: NVS flash read on Core 0 display task hot path | Removed line 85 (`_format = (uint8_t)ConfigManager::getModeSetting("clock", "format", 0);`). The format setting is already cached in `_format` member variable during `init()` (line 42). Mode now relies exclusively on cached value. |
| critical | Open/Closed Principle (SOLID) Violation: Hardcoded mode validation in ConfigManager | Removed hardcoded validation blocks (lines 787-792). Added comment explaining validation should occur at API layer using `ModeSettingsSchema` (minValue/maxValue fields) before calling `setModeSetting`. ConfigManager is now storage-only. |
| high | UI Bouncing Bug: Dynamic text scaling causes jarring layout jumps twice daily | Changed format specifier from `%d` to `%2d` to pad single-digit hours with leading space. String length now constant at 7 characters for all 12-hour times (e.g., " 9:30AM", "10:30AM"). |
| high | Lying Test: Enforces architectural anti-pattern | Updated test expectations to match storage-only architecture. ConfigManager now accepts all values; test verifies that out-of-range values ARE accepted (API layer will validate). Updated test comments to explain architectural decision. |
| low | Tech Debt: ODR violation and flash bloat from static const in header | Changed to extern declaration pattern: |

## Story dl-1-1 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #2 Violation: Full-screen flicker from fillScreen(0) | Removed all three `fillScreen(0)` calls. Changed `setTextColor(ctx.textColor)` to `setTextColor(ctx.textColor, 0)` to draw text with black background pixels, automatically overwriting previous characters without clearing unrelated regions. |
| low | Lying Test Name: test_set_mode_setting_validates_clock_format_range | Renamed function to `test_set_mode_setting_storage_only_no_validation` to accurately reflect what is being tested. Updated test runner call on line 751. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-2-idle-fallback-to-clock-mode.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task Completion Lie / State Corruption: Dev notes falsely claim to have fixed state update ordering in `onManualSwitch`, but `_manualModeId` is corrupted before validation. | ModeOrchestrator.cpp:66 | +3.0 |
| 🟠 IMPORTANT | State Desync in Schedule Transitions: Both entering and exiting the `SCHEDULED` state update internal orchestrator state before checking `requestSwitch()` result. | ModeOrchestrator.cpp:134 | +1.0 |
| 🟠 IMPORTANT | Lying Test: `test_manual_switch_unknown_mode_requestswitch_fails` deliberately omits checking `getManualModeId()` to hide the corruption bug. | test_main.cpp:240 | +1.0 |
| 🟠 IMPORTANT | Infinite Log Spam Loop: `tick()` continually calls `onFlightsRestored()` every second if the target mode fails to restore, spamming logs and exhausting heap checks. | ModeOrchestrator.cpp:166 | +1.0 |
| 🟠 IMPORTANT | Abstraction Leak (SRP): `main.cpp` manually implements mode name lookup by parsing the `ModeRegistry` internals. | main.cpp:322 | +1.0 |
| 🟡 MINOR | Redundant Double Switch: Exiting schedule with 0 flights triggers two consecutive atomic `requestSwitch()` calls in the same tick. | ModeOrchestrator.cpp:153 | +0.3 |
| 🟢 CLEAN PASS | 4 (Security, Type Safety, Style, Tech Debt) | | -2.0 |

### Evidence Score: 5.3

| Score | Verdict |
|-------|---------|
| **5.3** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[8/10] Single Responsibility Principle (Encapsulation Leak):** `main.cpp` reaches into `ModeRegistry`'s internal `getModeTable()` to iterate and manually resolve mode IDs to display names for `onManualSwitch`. This forces the main loop to know about the internal structure of the registry.
  - 📍 `firmware/src/main.cpp:322`
  - 💡 Fix: Add `static const char* getModeName(const char* modeId)` to `ModeRegistry`, or have `onManualSwitch` perform the lookup internally so the orchestrator caller only needs to pass the ID.

---

## ⚡ Performance & Scalability

- **[High] Infinite Log Spam / CPU Exhaustion Loop:** In `tick()`, if state is `IDLE_FALLBACK` and `flightCount > 0`, it calls `onFlightsRestored()`. If `requestSwitch` fails (e.g., due to a transient heap guard failure for the manual mode), `onFlightsRestored()` returns early, leaving `_state = IDLE_FALLBACK`. Because `tick()` runs every 1 second, it will endlessly retry `onFlightsRestored()` every second, spamming `LOG_W` and forcing unnecessary cross-core heap checks continuously.
  - 📍 `firmware/core/ModeOrchestrator.cpp:166`
  - 💡 Fix: Transition `_state = MANUAL` even if the switch fails and request a guaranteed safe fallback mode like `classic_card`, OR implement a backoff timer for retries.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** State Corruption & Task Lie in `onManualSwitch`. The dev notes explicitly claim to have "Fixed critical state corruption bug — onManualSwitch... call requestSwitch() BEFORE updating orchestrator state fields". This is factually false. `_manualModeId` and `_manualModeName` are overwritten on lines 66-69, BEFORE `requestSwitch()` validates the mode. If the switch is rejected, the orchestrator permanently stores an invalid fallback target.
  - 📍 `firmware/core/ModeOrchestrator.cpp:66`
  - 🔄 Reproduction: Call `onManualSwitch("nonexistent", "Fake")`. Wait for 0 flights (fallback to clock). Wait for flights to return. The system will try to restore "nonexistent", fail, and enter the infinite spam loop described above.

- **🐛 Bug:** State Desync in Schedule Transitions. Both entering `SCHEDULED` (line 134) and exiting `SCHEDULED` (line 144) update the orchestrator's `_state`, `_activeRuleIndex`, and `_activeModeId` BEFORE verifying if `ModeRegistry::requestSwitch()` actually succeeds. If the registry rejects the switch (e.g., heap guard failure), the orchestrator is permanently desynced from the LEDs.
  - 📍 `firmware/core/ModeOrchestrator.cpp:134`
  - 🔄 Reproduction: Set a schedule rule for a mode that requires more heap than is currently available. When the time window hits, the orchestrator will report `SCHEDULED` state via the API, but the LEDs will remain on the previous mode.

- **🎭 Lying Test:** `test_manual_switch_unknown_mode_requestswitch_fails`
  - 📍 `firmware/test/test_mode_orchestrator/test_main.cpp:240`
  - 🤥 Why it lies: The test claims to verify that state remains correct when a manual switch fails. It asserts `getState()` and `getActiveModeId()`, but it deliberately omits asserting `getManualModeId()`. If it had asserted the manual mode ID, the test would have failed and exposed the corruption bug on line 66 of `ModeOrchestrator.cpp`.

---

**Review Actions:**
- Issues Found: 6
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-2-idle-fallback-to-clock-mode.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | State corruption in `tick()` SCHEDULED path — updates state before `requestSwitch()`, identical to the "fixed" bug | `ModeOrchestrator.cpp:175-190` | +3.0 |
| 🟠 IMPORTANT | `_activeModeName` never updated during SCHEDULED transitions, causing API ID/name mismatch | `ModeOrchestrator.cpp:175-190` | +1.0 |
| 🟠 IMPORTANT | Test/production divergence — `simulateBootRestore()` falls back to `clock`, but `main.cpp` falls back to `classic_card` | `test_main.cpp:263-270` vs `main.cpp:305-310` | +1.0 |
| 🟠 IMPORTANT | Lying test `test_scheduled_state_not_overridden_by_tick` never creates SCHEDULED state | `test_main.cpp:194-200` | +1.0 |
| 🟠 IMPORTANT | Fragile test context — `matrix = nullptr` passed to `ModeRegistry::tick()`, one refactor away from NPE crash | `test_main.cpp:48-55` | +1.0 |
| 🟠 IMPORTANT | Incomplete state fix — `onManualSwitch()` updates `_manualModeId` before `requestSwitch()` failure check | `ModeOrchestrator.cpp:97-110` | +1.0 |
| 🟡 MINOR | Scope creep — full `dl-4.1` schedule evaluation logic inside `dl-1.2` story | `ModeOrchestrator.cpp:145-200` | +0.3 |
| 🟢 CLEAN PASS | Security | — | -0.5 |
| 🟢 CLEAN PASS | Performance | — | -0.5 |
| 🟢 CLEAN PASS | Style | — | -0.5 |
| 🟢 CLEAN PASS | Type Safety | — | -0.5 |

### Evidence Score: 6.3

| Score | Verdict |
|-------|---------|
| **6.3** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[7/10] Single Responsibility / Open-Closed:** `ModeOrchestrator::tick()` implements both idle fallback (`dl-1.2`) and full schedule rule evaluation (`dl-4.1`). This is scope creep that couples unrelated stories and forces future schedule changes to modify the orchestrator core.
  - 📍 `firmware/core/ModeOrchestrator.cpp:145-200`
  - 💡 Fix: Extract schedule evaluation into a separate `ScheduleEvaluator` or keep it in a `dl-4.1` branch until that story is active.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows C++ style guidelines and is readable.

---

## ⚡ Performance & Scalability

✅ No significant performance issues introduced by this story.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `ModeOrchestrator::tick()` SCHEDULED entry path updates `_state`, `_activeRuleIndex`, and `_activeModeId` **before** calling `ModeRegistry::requestSwitch()`. The Dev Agent Record proudly claims this state-corruption bug was "fixed" in `onManualSwitch`, `onIdleFallback`, and `onFlightsRestored` — but `tick()` was missed entirely. If `requestSwitch()` fails (heap guard, unknown mode), the orchestrator lies about being in SCHEDULED while the registry remains on the previous mode.
  - 📍 `firmware/core/ModeOrchestrator.cpp:175-190`
  - 🔄 Reproduction: Configure a schedule rule for an unregistered mode ID, wait for NTP sync, observe `GET /api/status` report `scheduled` while LEDs remain on `classic_card`.

- **🐛 Bug:** `ModeOrchestrator::tick()` SCHEDULED exit path has the **same** state-corruption pattern — it sets `_state = MANUAL` and `_activeModeId = _manualModeId` before `requestSwitch(_manualModeId)`.
  - 📍 `firmware/core/ModeOrchestrator.cpp:195-205`
  - 🔄 Reproduction: Have a schedule rule expire; ensure `requestSwitch()` fails; orchestrator claims MANUAL while registry is still on the scheduled mode.

- **🐛 Bug:** `onManualSwitch()` updates `_manualModeId` and `_manualModeName` at the top of the function, **before** the `requestSwitch()` guard. If the switch fails (e.g., unknown mode), the user's manual preference is permanently overwritten to the invalid mode ID. Subsequent `onFlightsRestored()` calls will then attempt to switch to this invalid mode.
  - 📍 `firmware/core/ModeOrchestrator.cpp:97-100`
  - 🔄 Reproduction: Call `onManualSwitch("bogus_mode", "Bogus")`; observe `_manualModeId` is now `"bogus_mode"` even though registry rejected it.

- **🐛 Bug:** `tick()` never updates `_activeModeName` when entering or exiting SCHEDULED state. The ID changes but the display name remains stale from the previous mode, causing API responses to serve mismatched metadata to the dashboard.
  - 📍 `firmware/core/ModeOrchestrator.cpp:175-205`
  - 🔄 Reproduction: Transition from `classic_card` to a scheduled `clock` rule; `GET /api/display/modes` will report active ID `"clock"` with active name `"Classic Card"`.

- **🎭 Lying Test:** `test_scheduled_state_not_overridden_by_tick` claims to verify that SCHEDULED is not overridden by idle fallback, but the test body never enters the SCHEDULED state. It only tests MANUAL → IDLE_FALLBACK and IDLE_FALLBACK → MANUAL, which are already covered by other tests. The test name is a lie.
  - 📍 `firmware/test/test_mode_orchestrator/test_main.cpp:194-200`
  - 🤥 Why it lies: The comment even admits "no public setter for SCHEDULED state currently exists, so we verify tick with 0 flights in MANUAL triggers fallback." That's not what the test name says.

- **🎭 Lying Test:** `test_unknown_mode_falls_back_to_clock` enforces behavior that contradicts production code. The test's `simulateBootRestore()` falls back to `"clock"` for unknown modes on normal boot, but `main.cpp` explicitly falls back to `"classic_card"`. The test passes because it validates its own helper function, not the actual firmware behavior.
  - 📍 `firmware/test/test_mode_orchestrator/test_main.cpp:263-270`
  - 🤥 Why it lies: `simulateBootRestore()` in the test file has a `clock` fallback chain that `main.cpp` simply does not implement. This creates false confidence and would block a refactor to align the two.

- **🎭 Fragile Test Design:** `makeTestCtx()` passes `matrix = nullptr` to `ModeRegistry::tick()`. The tests depend on `ClockMode::init()` never dereferencing `ctx.matrix`. If a future story refactors `ClockMode` to use the matrix during `init()`, every integration test in this file will segfault.
  - 📍 `firmware/test/test_mode_orchestrator/test_main.cpp:48-55`
  - 🤥 Why it lies: The test claims to verify ModeRegistry integration, but it uses a crippled `RenderContext` that cannot safely exercise real mode initialization.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `ModeOrchestrator::tick()` contains the complete `dl-4.1` schedule evaluation logic (rule matching, NTP time windows, `_activeRuleIndex` management). This was not part of the `dl-1.2` scope and tightly couples idle fallback to an unfinished schedule story. Future schedule story work will require modifying code that was already "done" in a previous sprint.
  - 📍 `firmware/core/ModeOrchestrator.cpp:145-200`
  - 💥 Explosion radius: Any bug fix or requirement change in `dl-4.1` schedule logic requires retesting and potentially regressing `dl-1.2`.

---

## 🛠️ Suggested Fixes

### 1. Fix State Corruption in `tick()` SCHEDULED Paths

**File:** `firmware/core/ModeOrchestrator.cpp`

**Issue:** Both SCHEDULED entry and exit paths update orchestrator state before `requestSwitch()`, reintroducing the state-corruption bug.

**Diff:**
```diff
         if (matchIndex >= 0) {
-            _state = OrchestratorState::SCHEDULED;
-            _activeRuleIndex = matchIndex;
-            strncpy(_activeModeId, matched.modeId, sizeof(_activeModeId) - 1);
-            _activeModeId[sizeof(_activeModeId) - 1] = '\0';
             if (!ModeRegistry::requestSwitch(matched.modeId)) {
                 LOG_W("ModeOrch", "Schedule: requestSwitch failed for rule mode");
                 return;
             }
+            _state = OrchestratorState::SCHEDULED;
+            _activeRuleIndex = matchIndex;
+            updateActiveMode(matched.modeId);  // sets both ID and name
             LOG_I("ModeOrch", "Schedule rule activated");
             return;
         } else if (_state == OrchestratorState::SCHEDULED) {
-            _state = OrchestratorState::MANUAL;
-            _activeRuleIndex = -1;
-            strncpy(_activeModeId, _manualModeId, sizeof(_activeModeId) - 1);
-            _activeModeId[sizeof(_activeModeId) - 1] = '\0';
             if (!ModeRegistry::requestSwitch(_manualModeId)) {
                 LOG_W("ModeOrch", "Schedule exit: requestSwitch failed for manual mode");
                 return;
             }
+            _state = OrchestratorState::MANUAL;
+            _activeRuleIndex = -1;
+            updateActiveMode(_manualModeId);  // sets both ID and name
             LOG_I("ModeOrch", "Schedule window ended, back to MANUAL");
         }
```

### 2. Fix Incomplete State Guard in `onManualSwitch`

**File:** `firmware/core/ModeOrchestrator.cpp`

**Issue:** `_manualModeId` is updated before the `requestSwitch()` guard.

**Diff:**
```diff
 void ModeOrchestrator::onManualSwitch(const char* modeId, const char* modeName) {
-    strncpy(_manualModeId, modeId, sizeof(_manualModeId) - 1);
-    _manualModeId[sizeof(_manualModeId) - 1] = '\0';
-    strncpy(_manualModeName, modeName, sizeof(_manualModeName) - 1);
-    _manualModeName[sizeof(_manualModeName) - 1] = '\0';
     if (!ModeRegistry::requestSwitch(modeId)) {
         LOG_W("ModeOrch", "Manual switch: requestSwitch failed for mode");
         return;
     }
+    strncpy(_manualModeId, modeId, sizeof(_manualModeId) - 1);
+    _manualModeId[sizeof(_manualModeId) - 1] = '\0';
+    strncpy(_manualModeName, modeName, sizeof(_manualModeName) - 1);
+    _manualModeName[sizeof(_manualModeName) - 1] = '\0';
     _state = OrchestratorState::MANUAL;
```

### 3. Align Test with Production Boot Behavior

**File:** `firmware/test/test_mode_orchestrator/test_main.cpp`

**Issue:** Test expects `clock` fallback for unknown modes on normal boot, but `main.cpp` uses `classic_card`.

**Diff:**
```diff
-void test_unknown_mode_falls_back_to_clock() {
+void test_unknown_mode_falls_back_to_classic_card() {
     initOrchestratorWithRegistry();
     nvsWriteString("disp_mode", "nonexistent_mode");
     ConfigManager::init();
     ModeOrchestrator::init();
 
     simulateBootRestore(/* wdtRecovery= */ false);
 
-    TEST_ASSERT_EQUAL_STRING("clock", ModeOrchestrator::getActiveModeId());
-    String stored = ConfigManager::getDisplayMode();
-    TEST_ASSERT_EQUAL_STRING("clock", stored.c_str());
+    TEST_ASSERT_EQUAL_STRING("classic_card", ModeOrchestrator::getActiveModeId());
+    String stored = ConfigManager::getDisplayMode();
+    TEST_ASSERT_EQUAL_STRING("classic_card", stored.c_str());
 }
```

### 4. Remove or Rewrite the Lying SCHEDULED Test

**File:** `firmware/test/test_mode_orchestrator/test_main.cpp`

**Issue:** Test name claims to verify SCHEDULED state behavior, but never enters SCHEDULED.

**Diff:**
```diff
-// AC #7: SCHEDULED state should not be overridden by idle fallback
-void test_scheduled_state_not_overridden_by_tick() {
-    initOrchestratorWithRegistry();
-    ModeOrchestrator::tick(0);
-    TEST_ASSERT_EQUAL(OrchestratorState::IDLE_FALLBACK, ModeOrchestrator::getState());
-    ModeOrchestrator::tick(5);
-    TEST_ASSERT_EQUAL(OrchestratorState::MANUAL, ModeOrchestrator::getState());
-}
+// AC #7: SCHEDULED state should not be overridden by idle fallback.
+// Deferred to dl-4.1+ — no public SCHEDULED state setter exists yet.
+void test_scheduled_state_not_overridden_by_tick() {
+    TEST_IGNORE_MESSAGE("SCHEDULED state interaction requires dl-4.1 schedule infrastructure");
+}
```

---

**Review Actions:**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 4

⚠️ Address the identified issues before proceeding.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-15</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">dl-1</var>
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
<var name="session_id">0d25b8ab-9284-4434-8618-0e600b19a4db</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="87850beb">embedded in prompt, file id: 87850beb</var>
<var name="story_id">dl-1.2</var>
<var name="story_key">dl-1-2-idle-fallback-to-clock-mode</var>
<var name="story_num">2</var>
<var name="story_title">2-idle-fallback-to-clock-mode</var>
<var name="template">False</var>
<var name="timestamp">20260415_0924</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-1.2)
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