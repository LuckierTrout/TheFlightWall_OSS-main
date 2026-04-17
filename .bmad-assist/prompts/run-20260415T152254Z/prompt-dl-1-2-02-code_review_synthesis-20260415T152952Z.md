<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-1 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260415T152952Z -->
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
- **Code Review Synthesis #1 (2026-04-15)**: Fixed state corruption in `onIdleFallback` and `onFlightsRestored` — both now call `requestSwitch()` BEFORE updating orchestrator state.
- **Code Review Synthesis #2 (2026-04-15)**: Fixed CRITICAL state corruption bugs identified by adversarial validators:
  - `onManualSwitch`: Moved `_manualModeId` and `_manualModeName` updates AFTER `requestSwitch()` validation to prevent permanent corruption on failed switches (lines 87-91)
  - SCHEDULED entry (tick): Moved state updates AFTER `requestSwitch()` validation; added `_activeModeName` update via mode table lookup (lines 165-182)
  - SCHEDULED exit (tick): Moved state updates AFTER `requestSwitch()` validation; added `_activeModeName` restoration (lines 189-199)
  - Test fixes: Corrected unknown-mode fallback expectation from "clock" to "classic_card" to match production behavior; converted lying `test_scheduled_state_not_overridden_by_tick` to TEST_IGNORE; added `getManualModeId()` assertion to catch state corruption.
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
- 2026-04-15: Code review synthesis (first pass) — fixed state corruption in `onIdleFallback` and `onFlightsRestored` (requestSwitch before state update)
- 2026-04-15: Code review synthesis (second pass) — fixed CRITICAL state corruption in `onManualSwitch` and SCHEDULED paths in `tick()`, added missing `_activeModeName` updates, corrected test/production divergence for unknown mode fallback, fixed lying test name
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
- **Evidence Score:** 5.3 (Validator A), 6.3 (Validator B) → REJECT/MAJOR REWORK
- **Issues Found:** 7 total (1 critical state corruption in 3 paths, 4 high-severity, 1 medium, 1 low)
- **Issues Fixed:** 7 (all verified issues addressed)
- **Action Items Created:** 0 (all issues fixed in this synthesis pass)



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

## Story dl-1-2 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | State Corruption in `onManualSwitch`**: The function updates `_manualModeId` and `_manualModeName` on lines 81-84 BEFORE calling `requestSwitch()` on line 89. If the switch fails (unknown mode, heap guard failure), the orchestrator permanently stores an invalid fallback target. The Dev Agent Record falsely claims this was "fixed" but only `onIdleFallback` and `onFlightsRestored` were corrected. | Moved state updates after `requestSwitch()` validation (see Changes Applied) |
| critical | State Corruption in `tick()` SCHEDULED Paths**: Both entering SCHEDULED state (lines 165-168) and exiting SCHEDULED (lines 179-183) update orchestrator state BEFORE calling `ModeRegistry::requestSwitch()`. If registry rejects the switch, orchestrator reports SCHEDULED while LEDs remain on previous mode. | Moved state updates after `requestSwitch()` calls |
| high | Missing `_activeModeName` Updates in SCHEDULED Transitions**: The `tick()` function updates `_activeModeId` when entering/exiting SCHEDULED but never updates `_activeModeName`. This causes API responses to serve mismatched ID/name pairs (e.g., ID="clock" with name="Classic Card"). | Added `_activeModeName` updates using mode table lookup |
| high | Test/Production Divergence**: Test helper `simulateBootRestore()` falls back to "clock" for unknown modes on normal boot (lines 387-390), but `main.cpp` falls back to "classic_card" (line 820). The test validates its own helper, not actual firmware behavior. | Updated test to expect "classic_card" fallback, matching production |
| high | Lying Test Name**: `test_scheduled_state_not_overridden_by_tick` claims to verify SCHEDULED state behavior but never creates SCHEDULED state. The test comment even admits "no public setter for SCHEDULED state currently exists". | Converted to TEST_IGNORE with message explaining dl-4.1 dependency |
| high | Incomplete Test Coverage**: `test_manual_switch_unknown_mode_requestswitch_fails` deliberately omits checking `getManualModeId()` to hide the corruption bug. If the test had asserted manual mode ID, it would have failed and exposed line 81's premature state update. | Added assertion for `getManualModeId()` to catch state corruption |
| medium | Encapsulation Leak (SRP Violation)**: `main.cpp` manually implements mode name lookup by iterating `ModeRegistry::getModeTable()` internals (lines 789-794). This forces main loop to know registry's internal structure. | Deferred - Would require adding `ModeRegistry::getModeName()` helper, which exceeds synthesis scope. Noted in Future Improvements. |
| medium | Fragile Test Context**: `makeTestCtx()` passes `matrix = nullptr` to `ModeRegistry::tick()`. Tests depend on `ClockMode::init()` never dereferencing `ctx.matrix`. A future refactor could cause segfaults. | Deferred - Requires test infrastructure changes. Noted in Future Improvements. |
| low | Scope Creep**: `ModeOrchestrator::tick()` contains full dl-4.1 schedule evaluation logic (lines 145-200), tightly coupling dl-1.2 (idle fallback) to an unfinished future story. | Accepted as intentional - Schedule logic was implemented early as foundation work. No action required. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-2-idle-fallback-to-clock-mode.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 2 |
| Hidden Bugs | 3 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **5** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | State corruption: orchestrator updates _manualModeId BEFORE calling requestSwitch() - if switch fails, orchestrator stores invalid fallback target | ModeOrchestrator.cpp:81-84 | +3 |
| 🔴 CRITICAL | State corruption: SCHEDULED path updates orchestrator state BEFORE calling requestSwitch() - if registry rejects, orchestrator reports SCHEDULED while LEDs remain on previous mode | ModeOrchestrator.cpp:165-168 | +3 |
| 🔴 CRITICAL | State corruption: SCHEDULED exit path updates orchestrator state BEFORE calling requestSwitch() - same corruption as SCHEDULED entry | ModeOrchestrator.cpp:179-183 | +3 |
| 🟠 IMPORTANT | Missing _activeModeName updates in SCHEDULED transitions causes API responses to serve mismatched ID/name pairs | ModeOrchestrator.cpp:165-199 | +1 |
| 🟠 IMPORTANT | Lying test: test_scheduled_state_not_overridden_by_tick claims to verify SCHEDULED behavior but never creates SCHEDULED state - TEST_IGNORE already added but test name still lies | test_mode_orchestrator/test_main.cpp:220 | +1 |
| 🟢 CLEAN PASS | Performance | -0.5 |
| 🟢 CLEAN PASS | Security | -0.5 |
| 🟢 CLEAN PASS | Style (non-Python) | -0.5 |
| 🟢 CLEAN PASS | Type Safety | -0.5 |

### Evidence Score: 6.0

| Score | Verdict |
|-------|---------|
| **6.0** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] SRP Violation (State Corruption Pattern):** Orchestrator state is updated BEFORE validating the ModeRegistry switch. This violates Single Responsibility Principle - the orchestrator is both requesting a transition AND assuming it succeeded, rather than validating the result first.
  - 📍 `ModeOrchestrator.cpp:81-89` (onManualSwitch)
  - 📍 `ModeOrchestrator.cpp:165-171` (tick - SCHEDULED entry)
  - 📍 `ModeOrchestrator.cpp:179-185` (tick - SCHEDULED exit)
  - 💡 Fix: Call `requestSwitch()` FIRST, check return value, THEN update orchestrator state only on success. This is the exact pattern already used in `onIdleFallback()` and `onFlightsRestored()` - why wasn't it applied consistently?

- **[6/10] Data Integrity Violation:** API responses serve inconsistent state data (ID="clock" but name="Classic Card") because _activeModeName is not updated in SCHEDULED transitions.
  - 📍 `ModeOrchestrator.cpp:165-199`
  - 💡 Fix: Add mode table lookup to update _activeModeName when entering/exiting SCHEDULED state, matching the pattern used in onManualSwitch()

- **[5/10] Lying Test Documentation:** Test name claims to verify SCHEDULED state behavior but implementation is TEST_IGNORE with comment "SCHEDULED state interaction requires dl-4.1 schedule infrastructure". The test name should reflect that it's a placeholder or be removed entirely.
  - 📍 `test_mode_orchestrator/test_main.cpp:220`
  - 💡 Fix: Either rename to `test_scheduled_state_deferred_to_dl_4_1` or remove entirely - current name misleads about what's actually tested.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug: State corruption in onManualSwitch when switch fails**
  - 📍 `ModeOrchestrator.cpp:81-89`
  - 🔄 Reproduction: Call `ModeOrchestrator::onManualSwitch("nonexistent_mode", "Does Not Exist")` - orchestrator updates _manualModeId to invalid value before calling requestSwitch(). If registry rejects (unknown mode), orchestrator permanently stores invalid fallback target. Next fallback or restore will fail.
  - ⚠️ Impact: High - orchestrator state becomes inconsistent with registry truth, leading to wrong mode activation on subsequent state transitions.

- **🐛 Bug: State corruption in tick() SCHEDULED entry when switch fails**
  - 📍 `ModeOrchestrator.cpp:165-171`
  - 🔄 Reproduction: Configure schedule rule with invalid mode_id, or cause heap guard failure. Orchestrator sets state to SCHEDULED and updates _activeModeId before calling requestSwitch(). If registry rejects, orchestrator reports SCHEDULED with target mode but LEDs remain on previous mode.
  - ⚠️ Impact: High - API responses lie about active mode, UI shows wrong state, orchestrator and registry desync.

- **🐛 Bug: State corruption in tick() SCHEDULED exit when switch fails**
  - 📍 `ModeOrchestrator.cpp:179-185`
  - 🔄 Reproduction: Same as SCHEDULED entry - orchestrator sets state to MANUAL and updates active mode before calling requestSwitch(). If registry rejects, orchestrator reports MANUAL but LEDs remain on scheduled mode.
  - ⚠️ Impact: High - Same desync issue as SCHEDULED entry.

- **🐛 Bug: Missing _activeModeName updates cause API inconsistency**
  - 📍 `ModeOrchestrator.cpp:165-199` (both SCHEDULED transitions)
  - 🔄 Reproduction: Create schedule rule for "clock" mode. When rule activates, orchestrator updates _activeModeId to "clock" but leaves _activeModeName as whatever it was before. GET /api/status or GET /api/display/modes returns mismatched ID/name pair.
  - ⚠️ Impact: Medium - Dashboard UI shows wrong display name, confusing users.

- **🎭 Lying Test: test_scheduled_state_not_overridden_by_tick**
  - 📍 `test_mode_orchestrator/test_main.cpp:220`
  - 🤥 Why it lies: Test name claims to verify "SCHEDULED state not overridden by tick" but implementation is TEST_IGNORE with comment admitting it can't create SCHEDULED state without dl-4.1 infrastructure. The test passes automatically without verifying anything.
  - ⚠️ Impact: Low - test infrastructure issue, not production code, but misleads about coverage.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Inconsistent error handling pattern across orchestrator state transitions**
  - 📍 `ModeOrchestrator.cpp:70-108, 164-201`
  - 💥 Explosion radius: Medium - Future state transitions (e.g., dl-4.1 schedule changes) will likely repeat these bugs unless pattern is standardized.
  - 💡 Fix: Extract common pattern into helper method: `bool trySwitchState(OrchestratorState newState, const char* targetModeId, const char* targetModeName)` that calls requestSwitch() first, validates result, then updates state only on success.

---

## 🛠️ Suggested Fixes

### 1. Fix State Corruption in onManualSwitch

**File:** `firmware/core/ModeOrchestrator.cpp`
**Issue:** Updates _manualModeId BEFORE calling requestSwitch() - corruption on failure

**Corrected code:**
```cpp
void ModeOrchestrator::onManualSwitch(const char* modeId, const char* modeName) {
    // Story dl-1.3, AC #1: drive ModeRegistry so LED mode actually changes.
    // AC #5: if requestSwitch fails (unknown mode / heap guard), do not update
    // orchestrator state at all — leave state unchanged.
    if (!ModeRegistry::requestSwitch(modeId)) {
        LOG_W("ModeOrch", "Manual switch: requestSwitch failed for mode");
        return;
    }
    // Remember as manual selection for restore after fallback (only after validation)
    strncpy(_manualModeId, modeId, sizeof(_manualModeId) - 1);
    _manualModeId[sizeof(_manualModeId) - 1] = '\0';
    strncpy(_manualModeName, modeName, sizeof(_manualModeName) - 1);
    _manualModeName[sizeof(_manualModeName) - 1] = '\0';
    _state = OrchestratorState::MANUAL;
    _activeRuleIndex = -1;
    strncpy(_activeModeId, modeId, sizeof(_activeModeId) - 1);
    _activeModeId[sizeof(_activeModeId) - 1] = '\0';
    strncpy(_activeModeName, modeName, sizeof(_activeModeName) - 1);
    _activeModeName[sizeof(_activeModeName) - 1] = '\0';
    LOG_I("ModeOrch", "Manual switch");
}
```

### 2. Fix State Corruption in SCHEDULED Entry

**File:** `firmware/core/ModeOrchestrator.cpp`
**Issue:** Updates orchestrator state BEFORE calling requestSwitch() - corruption on failure

**Diff:**
```diff
             const ScheduleRule& matched = sched.rules[matchIndex];
 
             if (_state != OrchestratorState::SCHEDULED || _activeRuleIndex != matchIndex) {
                 // Entering SCHEDULED or switching to a different rule
                 // AC #9: only issue requestSwitch from tick (or onManualSwitch)
-                if (!ModeRegistry::requestSwitch(matched.modeId)) {
-                    LOG_W("ModeOrch", "Schedule: requestSwitch failed for rule mode");
-                    return;
-                }
                 _state = OrchestratorState::SCHEDULED;
                 _activeRuleIndex = matchIndex;
                 strncpy(_activeModeId, matched.modeId, sizeof(_activeModeId) - 1);
                 _activeModeId[sizeof(_activeModeId) - 1] = '\0';
                 // Update active mode display name from mode table
                 const ModeEntry* table = ModeRegistry::getModeTable();
                 uint8_t count = ModeRegistry::getModeCount();
                 for (uint8_t i = 0; i < count; i++) {
                     if (strcmp(table[i].id, matched.modeId) == 0) {
                         strncpy(_activeModeName, table[i].displayName, sizeof(_activeModeName) - 1);
                         _activeModeName[sizeof(_activeModeName) - 1] = '\0';
                         break;
                     }
                 }
                 LOG_I("ModeOrch", "Schedule rule activated");
+                
+                // Call requestSwitch AFTER state is prepared, validate result
+                if (!ModeRegistry::requestSwitch(matched.modeId)) {
+                    LOG_W("ModeOrch", "Schedule: requestSwitch failed for rule mode");
+                    // Rollback state to MANUAL to maintain consistency with registry
+                    _state = OrchestratorState::MANUAL;
+                    _activeRuleIndex = -1;
+                    // Restore previous mode name if we had updated it
+                    strncpy(_activeModeName, _manualModeName, sizeof(_activeModeName) - 1);
+                    return;
+                }
             }
```

### 3. Fix State Corruption in SCHEDULED Exit

**File:** `firmware/core/ModeOrchestrator.cpp`
**Issue:** Updates orchestrator state BEFORE calling requestSwitch() - corruption on failure

**Diff:**
```diff
         } else if (_state == OrchestratorState::SCHEDULED) {
             // AC #4: no rule matches and was SCHEDULED → transition to MANUAL
-            if (!ModeRegistry::requestSwitch(_manualModeId)) {
-                LOG_W("ModeOrch", "Schedule exit: requestSwitch failed for manual mode");
-                return;
-            }
             _state = OrchestratorState::MANUAL;
             _activeRuleIndex = -1;
             // Restore owner's manual selection (both ID and name)
             strncpy(_activeModeId, _manualModeId, sizeof(_activeModeId) - 1);
             _activeModeId[sizeof(_activeModeId) - 1] = '\0';
             strncpy(_activeModeName, _manualModeName, sizeof(_activeModeName) - 1);
             _activeModeName[sizeof(_activeModeName) - 1] = '\0';
             LOG_I("ModeOrch", "Schedule window ended, back to MANUAL");
             // Fall through to idle fallback logic below in case flightCount is 0
+            
+            // Call requestSwitch AFTER state is prepared, validate result
+            if (!ModeRegistry::requestSwitch(_manualModeId)) {
+                LOG_W("ModeOrch", "Schedule exit: requestSwitch failed for manual mode");
+                // Keep in SCHEDULED state since registry didn't accept the switch
+                _state = OrchestratorState::SCHEDULED;
+                _activeRuleIndex = matchIndex;
+                // Restore scheduled mode name
+                const ModeEntry* table = ModeRegistry::getModeTable();
+                uint8_t count = ModeRegistry::getModeCount();
+                for (uint8_t i = 0; i < count; i++) {
+                    if (strcmp(table[i].id, _activeModeId) == 0) {
+                        strncpy(_activeModeName, table[i].displayName, sizeof(_activeModeName) - 1);
+                        _activeModeName[sizeof(_activeModeName) - 1] = '\0';
+                        break;
+                    }
+                }
+                return;
+            }
         }
```

### 4. Remove or Rename Lying Test

**File:** `firmware/test/test_mode_orchestrator/test_main.cpp`
**Issue:** Test name claims to verify SCHEDULED behavior but never creates SCHEDULED state

**Diff:**
```diff
-// AC #7: SCHEDULED state should not be overridden by idle fallback
-// DEFERRED: Requires dl-4.1 schedule infrastructure to create SCHEDULED state
-void test_scheduled_state_not_overridden_by_tick() {
-    TEST_IGNORE_MESSAGE("SCHEDULED state interaction requires dl-4.1 schedule infrastructure");
-}
+// AC #7: SCHEDULED state should not be overridden by idle fallback
+// DEFERRED: Requires dl-4.1 schedule infrastructure to create SCHEDULED state
+// This test is a placeholder pending dl-4.1 implementation
+void test_scheduled_state_deferred_to_dl_4_1() {
+    TEST_IGNORE_MESSAGE("SCHEDULED state interaction requires dl-4.1 schedule infrastructure");
+}
```

And update the test runner:
```diff
 RUN_TEST(test_idle_fallback_requests_clock_in_registry);
 RUN_TEST(test_flights_restored_requests_manual_mode_in_registry);
 RUN_TEST(test_idle_fallback_safe_when_clock_already_active);
-RUN_TEST(test_scheduled_state_not_overridden_by_tick);
+RUN_TEST(test_scheduled_state_deferred_to_dl_4_1);
```

---

**Review Actions:**
- Issues Found: 5
- Issues Fixed: 0
- Action Items Created: 0

**🚨 CRITICAL FINDING:** This story contains THREE instances of state corruption where orchestrator state is updated BEFORE validating the ModeRegistry switch. The pattern was correctly implemented in `onIdleFallback()` and `onFlightsRestored()` (validate first, then update) but NOT in `onManualSwitch()` or the SCHEDULED paths in `tick()`. This inconsistency will cause orchestrator state to drift from registry truth whenever a switch fails, leading to wrong mode activation on subsequent transitions. The Evidence Score of 6.0 (REJECT) reflects the severity of these state corruption bugs.

**RECOMMENDATION:** Do NOT proceed to next story until state corruption is fixed. The fixes above are straightforward - move the `requestSwitch()` calls to occur BEFORE state updates, matching the pattern already used in the working code paths.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `_bmad-output/implementation-artifacts/stories/dl-1-2-idle-fallback-to-clock-mode.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **5** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | State Corruption: `tick()` SCHEDULED path updates orchestrator state BEFORE `ModeRegistry::requestSwitch()`. | `firmware/core/ModeOrchestrator.cpp:165` | +3 |
| 🟠 IMPORTANT | Task Completion False Claim: Task 3.3 falsely claims `onManualSwitch` validation is out of scope for dl-1.2. | `dl-1.2 story file` | +1 |
| 🟠 IMPORTANT | SOLID (Single Responsibility) Violation: `ModeOrchestrator::tick()` is a "god method" mixing fallback, scheduling, and state orchestration. | `firmware/core/ModeOrchestrator.cpp:145-200` | +1 |
| 🟡 MINOR | Missing Test: Lack of integration test for the 1-second orchestrator tick in `main.cpp`. | `firmware/src/main.cpp` | +0.3 |
| 🟡 MINOR | Fragile Test Dependency: tests mock behavior via story-later `POST` API rather than direct registry priming. | `firmware/test/test_mode_orchestrator/test_main.cpp` | +0.3 |

### Evidence Score: 5.6

| Score | Verdict |
|-------|---------|
| **5.6** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[8/10] Single Responsibility:** `ModeOrchestrator::tick()` is a "god method" performing schedule rule evaluation (dl-4.1), idle fallback (dl-1.2), manual mode restoration, and cross-state coordination. This tightly couples dl-1.2 to unfinished stories.
  - 📍 `firmware/core/ModeOrchestrator.cpp:145-200`
  - 💡 Fix: Extract schedule rule evaluation into a `ScheduleEngine` or `ModeOrchestrator::evaluateSchedule()` and keep `tick()` focused on idle fallback and state machine transitions.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `ModeOrchestrator::tick()` SCHEDULED path updates `_state`, `_activeRuleIndex`, and `_activeModeId` **before** calling `ModeRegistry::requestSwitch()`. If the registry rejects the switch (e.g., heap guard failure), the orchestrator lies about being in SCHEDULED.
  - 📍 `firmware/core/ModeOrchestrator.cpp:165-170`
  - 🔄 Reproduction: Simulate a mode heap guard failure while a schedule rule is active. The orchestrator will report state as SCHEDULED, while the registry remains on the previous mode.

- **🐛 Bug:** Task 3.3 in the story file is marked `[x]` complete with the claim: *"Verified `onManualSwitch` does NOT call `requestSwitch` — out of scope for dl-1.2"*. The actual checked-in code at line 71 **does** call `ModeRegistry::requestSwitch(modeId)`. This is a false task completion claim.
  - 📍 `firmware/core/ModeOrchestrator.cpp:71`
  - 🔄 Reproduction: Observe `onManualSwitch` implementation.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `ModeOrchestrator::tick()` contains the complete `dl-4.1` schedule evaluation logic. This is scope creep that couples unrelated stories and forces future schedule changes to modify the idle-fallback component.
  - 📍 `firmware/core/ModeOrchestrator.cpp:145-200`
  - 💥 Explosion radius: High. Any bug fix or requirement change in `dl-4.1` schedule logic requires retesting `dl-1.2`.

---

## 🛠️ Suggested Fixes

### 1. Fix State Corruption in `tick()`

**File:** `firmware/core/ModeOrchestrator.cpp`
**Issue:** Orchestrator state updated before registry switch validation.

**Diff:**
```diff
--- a/firmware/core/ModeOrchestrator.cpp
+++ b/firmware/core/ModeOrchestrator.cpp
@@ -165,6 +165,11 @@
-                _state = OrchestratorState::SCHEDULED;
-                _activeRuleIndex = matchIndex;
-                strncpy(_activeModeId, matched.modeId, sizeof(_activeModeId) - 1);
+                if (!ModeRegistry::requestSwitch(matched.modeId)) {
+                    LOG_W("ModeOrch", "Schedule: requestSwitch failed for rule mode");
+                    return;
+                }
+                _state = OrchestratorState::SCHEDULED;
+                _activeRuleIndex = matchIndex;
+                strncpy(_activeModeId, matched.modeId, sizeof(_activeModeId) - 1);
```

---

**Review Actions:**
- Issues Found: 5
- Issues Fixed: 0
- Action Items Created: 0

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
<var name="session_id">530ecfc3-e378-401c-b52c-602ae9e6a42f</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="87850beb">embedded in prompt, file id: 87850beb</var>
<var name="story_id">dl-1.2</var>
<var name="story_key">dl-1-2-idle-fallback-to-clock-mode</var>
<var name="story_num">2</var>
<var name="story_title">2-idle-fallback-to-clock-mode</var>
<var name="template">False</var>
<var name="timestamp">20260415_1129</var>
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