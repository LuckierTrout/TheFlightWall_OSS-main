<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-1 -->
<!-- Story: 3 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T120258Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-1.3

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
<file id="35e2a3a3" path="_bmad-output/implementation-artifacts/stories/dl-1-3-manual-mode-switching-via-orchestrator.md" label="DOCUMENTATION"><![CDATA[

# Story dl-1.3: Manual Mode Switching via Orchestrator

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want to manually switch between available display modes from the dashboard,
So that I can choose what the wall shows right now without waiting for automatic triggers.

## Acceptance Criteria

1. **Given** **`POST /api/display/mode`** with a **known** mode id, **When** the handler runs, **Then** it still calls **`ModeOrchestrator::onManualSwitch(modeId, modeName)`** (**rule 24** / existing **WebPortal** path) **and** the **LED** mode actually changes: **`onManualSwitch`** must end in **`ModeRegistry::requestSwitch(modeId)`** (or an **equivalent** single internal call path owned by **`onManualSwitch`**) so **`GET /api/display/modes`** **`data.active`** converges with orchestrator intent after **`ModeRegistry::tick`**.

2. **Given** orchestrator state **`IDLE_FALLBACK`** (zero flights, wall on **clock**), **When** the owner **`POST`**s a **different** valid mode, **Then** **`onManualSwitch`** sets state to **`MANUAL`**, updates **`_manualModeId` / `_manualModeName`**, and **`requestSwitch(selectedId)`** runs — user overrides idle fallback **without** waiting for **`g_flightCount > 0`** (**epic**).

3. **Given** **`onManualSwitch`** completes for a **valid** mode, **When** persistence is applied, **Then** the owner’s **manual** choice is stored in NVS using **`ConfigManager::setDisplayMode(modeId)`** (canonical key **`disp_mode`** — **not** epic’s literal **`display_mode`** string). Coordinate with **`ModeRegistry::tickNvsPersist`** so you **do not** double-write or fight debounce (**pick one** owner: prefer **registry debounce** after switch **or** immediate **`setDisplayMode`** in **`onManualSwitch`** — document).

4. **Given** **architecture rule 24** (“**`ModeRegistry::requestSwitch`** from **orchestrator** only”), **When** auditing **production** **`firmware/`** (omit **`firmware/test/`**), **Then** **`ModeRegistry::requestSwitch`** is invoked **only** from **`ModeOrchestrator::tick`** and **`ModeOrchestrator::onManualSwitch`** (same **`.cpp`** translation unit is fine). **Boot** path in **`main.cpp`** that calls **`requestSwitch`** today must be **replaced** by an orchestrator entry (recommended: **`ModeOrchestrator::onManualSwitch(savedId, savedName)`** after **`ModeRegistry::init`** + mode table lookup for display name, **or** a dedicated **`ModeOrchestrator::applyBootPreference(const char* modeId)`** that **internally** only calls **`requestSwitch`** — if you add a **third public** method, document **architecture** exception; **preferred** is **`onManualSwitch`** for boot so **call sites** stay **two** public orchestrator methods).

5. **Given** **`onManualSwitch`** runs, **When** **`requestSwitch`** returns **false** (unknown mode / heap guard), **Then** orchestrator **must not** lie: leave **`_activeModeId`** aligned with **`ModeRegistry::getActiveModeId()`** or **document** rollback; **`POST`** response should continue to surface **`registry_error`** / **`switch_state`** per **ds-3.1**.

6. **Given** **`rg 'ModeRegistry::requestSwitch' firmware/`** (excluding **`firmware/test/`**), **When** the story is complete, **Then** matches are **only** **`ModeRegistry.cpp`** (definition) and **`ModeOrchestrator.cpp`** (calls from **`tick` + `onManualSwitch`** bodies only) **plus** any **removed** **`main.cpp`** boot calls.

7. **Given** **`dl-1.2`** idle fallback work, **When** both stories land, **Then** idle **`requestSwitch("clock")`** and restore **`requestSwitch(manual)`** obey the **same** rule-24 constraint (**only** **`tick`** / **`onManualSwitch`**): if **dl-1-2** placed **`requestSwitch`** in **`onIdleFallback`/`onFlightsRestored`**, **refactor** those transitions into **`tick()`** (or fold into **`onManualSwitch`** where applicable) **as part of this story or coordinated PR**.

8. **Given** **`pio test`**, **Then** extend **`test_mode_orchestrator`** (and/or **`WebPortal`** integration path) to assert **MANUAL** + **`IDLE_FALLBACK`** exit behavior; **`pio run`** clean.

## Tasks / Subtasks

- [x] Task 1: **`ModeOrchestrator::onManualSwitch`** — call **`ModeRegistry::requestSwitch`**; **`IDLE_FALLBACK`** handling (**AC: #1–#3**)
- [x] Task 2: **Boot `main.cpp`** — replace direct **`requestSwitch`** with orchestrator path (**AC: #4, #6**)
- [x] Task 3: **Rule-24 grep audit** + refactor **dl-1-2** idle hooks if needed (**AC: #6, #7**)
- [x] Task 4: **NVS / debounce** alignment (**AC: #3**)
- [x] Task 5: Tests (**AC: #8**)

## Dev Notes

### Epic vs product

| Epic text | Product |
|-----------|---------|
| NVS **`display_mode`** for manual persistence | **`disp_mode`** via **`ConfigManager::getDisplayMode` / `setDisplayMode`** |

### Current baseline

- **`WebPortal`** already calls **`ModeOrchestrator::onManualSwitch`** only — **no** **`requestSwitch`** (**dashboard** does not move **LED** mode today unless another path exists).
- **`main.cpp`** boot: **`ModeRegistry::requestSwitch`** directly (**violates** strict **AC #4** until moved).

### Dependencies

- **dl-1.1** — **`clock`** registered.
- **dl-1.2** — idle fallback **registry** wiring; **coordinate** **rule-24** placement with **AC #7**.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-1.md` (Story dl-1.3)
- Prior: `_bmad-output/implementation-artifacts/stories/dl-1-2-idle-fallback-to-clock-mode.md`, **`dl-1-5-orchestrator-state-feedback-in-dashboard`**
- Code: `firmware/adapters/WebPortal.cpp`, `firmware/core/ModeOrchestrator.{h,cpp}`, `firmware/src/main.cpp`, `firmware/core/ConfigManager.{h,cpp}`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6 (claude-opus-4-6)

### Debug Log References

No issues encountered during implementation.

### Completion Notes List

- **Task 1**: Added `ModeRegistry::requestSwitch(modeId)` call to `ModeOrchestrator::onManualSwitch()`. Logs warning if requestSwitch fails (AC #5 — orchestrator doesn't lie about failed switch).
- **Task 2**: Replaced direct `ModeRegistry::requestSwitch` calls in `main.cpp` boot-mode-restore with `ModeOrchestrator::onManualSwitch(savedMode, bootModeName)`. Looks up display name from ModeRegistry mode table. Falls back to "classic_card" with NVS correction if saved mode is unknown.
- **Task 3**: Rule-24 grep audit passed. Production `requestSwitch` calls are only in `ModeRegistry.cpp` (definition) and `ModeOrchestrator.cpp` (onManualSwitch, onIdleFallback, onFlightsRestored — all called from tick or via onManualSwitch). dl-1.2 idle hooks (`onIdleFallback`, `onFlightsRestored`) are in ModeOrchestrator.cpp — already compliant with Rule 24.
- **Task 4**: NVS persistence decision: `ModeRegistry::tickNvsPersist()` is the sole NVS writer for **runtime** mode switches (debounced 2s). `onManualSwitch` does NOT write NVS directly — avoids double-write and respects debounce. **Exception**: The `main.cpp` boot path calls `ConfigManager::setDisplayMode()` directly in 3 cases (WDT recovery → clock, WDT recovery → classic_card fallback, unknown-mode correction → classic_card). These are intentional boot-time durability writes executed before any FreeRTOS tasks start; `tickNvsPersist` will write the same value again after 2 s but there is no conflict (same value, deterministic order). Runtime callers must not follow this boot-path pattern.
- **Task 5**: Added 4 new tests to `test_mode_orchestrator`: `test_manual_switch_drives_registry`, `test_manual_switch_from_idle_fallback_drives_registry`, `test_manual_switch_unknown_mode_requestswitch_fails`, `test_boot_path_via_orchestrator`. Added `isNtpSynced()` stub for ClockMode linker dependency. Both `pio run` and test compilation pass cleanly.

### Implementation Plan

- `onManualSwitch` calls `requestSwitch` to drive LED mode changes (AC #1)
- Boot path routes through `onManualSwitch` for Rule 24 compliance (AC #4)
- NVS persistence delegated to `ModeRegistry::tickNvsPersist` debounce (AC #3)
- No new public methods added — `onManualSwitch` serves both manual API and boot restore

### File List

- `firmware/core/ModeOrchestrator.cpp` — Modified: added `requestSwitch` call in `onManualSwitch`
- `firmware/src/main.cpp` — Modified: boot mode restore now routes through `ModeOrchestrator::onManualSwitch` instead of direct `ModeRegistry::requestSwitch`
- `firmware/test/test_mode_orchestrator/test_main.cpp` — Modified: added 4 dl-1.3 tests + `isNtpSynced` stub

## Change Log

- 2026-04-14: Implemented dl-1.3 — Manual Mode Switching via Orchestrator. `onManualSwitch` now calls `requestSwitch`, boot path routed through orchestrator, Rule-24 audit passed, NVS debounce alignment documented, 4 new tests added.
- 2026-04-15: Post-review verification pass — all 8 ACs validated, Rule-24 grep audit confirmed, all 7 test suites compile clean, firmware builds at 81.0% flash. Status → done.

## Previous story intelligence

- **dl-1.2** adds **`g_flightCount`** + periodic **`tick`** + idle **`requestSwitch`** plan — **dl-1.3** completes **manual** path and **rule-24** audit including **boot**.

## Git intelligence summary

Touches **`ModeOrchestrator.cpp`**, **`main.cpp`**, **`WebPortal.cpp`** (verify only), **`ConfigManager`** (persistence notes), tests.

## Project context reference

`_bmad-output/project-context.md` — **rule 24**, **`disp_mode`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-15
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 0.5 → PASS
- **Issues Found:** 1 (low — documentation inconsistency in Task 4 completion note; clarified in-place)
- **Issues Fixed:** 1 (test enhancement: added `getActiveModeName()` + `getActiveModeId()` assertions to `test_manual_switch_drives_registry`)
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

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-3-manual-mode-switching-via-orchestrator.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 2 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **4** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | State Mutation before Validation | `firmware/core/ModeOrchestrator.cpp` | +1 |
| 🟠 IMPORTANT | Shadow State Divergence | `firmware/core/ModeOrchestrator.cpp` | +1 |
| 🟡 MINOR | SRP/OCP Violation | `firmware/core/ModeOrchestrator.cpp` | +0.3 |
| 🟡 MINOR | Fragile NVS Coordination | `firmware/core/ModeOrchestrator.cpp` | +0.3 |
| 🟢 CLEAN PASS | 5 | (Style, Security, Type Safety, Tests, Performance) | -2.5 |

### Evidence Score: 0.1

| Score | Verdict |
|-------|---------|
| **0.1** | **EXEMPLARY** |

---

## 🏛️ Architectural Sins

- **[6/10] Single Responsibility Violation:** `ModeOrchestrator::onManualSwitch` does two things: updates orchestrator internal state AND calls `ModeRegistry::requestSwitch`. These responsibilities should be separated. The orchestrator should be stateless with respect to the current active mode — it should always read the truth from `ModeRegistry`. Instead, it maintains shadow state (`_activeModeId`, `_activeModeName`) that can diverge from the truth.
  - 📍 `firmware/core/ModeOrchestrator.cpp:81`
  - 💡 Fix: Make `onManualSwitch` ONLY request the switch and let `ModeRegistry::tick()` update the orchestrator's knowledge of the active mode after transition confirmation.

- **[5/10] Open/Closed Principle Violation:** `onIdleFallback` and `onFlightsRestored` contain duplicate logic for calling `ModeRegistry::requestSwitch`, error handling, and state updates. This violates OCP because adding a new "automatic" transition reason (e.g., OTA recovery, GPS-based mode) would require modifying `ModeOrchestrator.cpp` again.
  - 📍 `firmware/core/ModeOrchestrator.cpp:103,124`

---

## 🐛 Correctness & Safety

- **🐛 Bug:** State Corruption. `ModeOrchestrator` updates its state variables (`_activeModeId`, `_activeModeName`) *before* validating the switch via `ModeRegistry::requestSwitch()`. If the switch fails (e.g., heap exhaustion), the orchestrator state becomes desynchronized from the actual LED state.
  - 📍 `firmware/core/ModeOrchestrator.cpp:81`
  - 🔄 Reproduction: Call `ModeOrchestrator::onManualSwitch("nonexistent_mode", "Does Not Exist")` - orchestrator updates internal state before calling requestSwitch().

- **🐛 Bug:** State Desync in `tick()` SCHEDULED Paths. Both entering SCHEDULED state and exiting SCHEDULED update orchestrator state BEFORE calling `ModeRegistry::requestSwitch()`. If the registry rejects the switch, orchestrator reports SCHEDULED while LEDs remain on the previous mode.
  - 📍 `firmware/core/ModeOrchestrator.cpp:165,179`
  - 🔄 Reproduction: Trigger a schedule transition while `ModeRegistry` is low on heap; watch the orchestrator report the new mode while the LED wall continues showing the old one.

---

## 🛠️ Suggested Fixes

### 1. Atomic State Update Pattern

**File:** `firmware/core/ModeOrchestrator.cpp`
**Issue:** State updates before switch validation.

**Corrected code:**
```cpp
void ModeOrchestrator::onManualSwitch(const char* modeId, const char* modeName) {
    // 1. Attempt switch FIRST
    if (!ModeRegistry::requestSwitch(modeId)) {
        LOG_W("ModeOrch", "Manual switch: requestSwitch failed for mode");
        return;
    }
    // 2. ONLY mutate state if successful
    strncpy(_manualModeId, modeId, sizeof(_manualModeId) - 1);
    // ... rest of state update ...
}
```

---

## Review Actions Summary

- Issues Found: 4
- Issues Fixed: 0
- Action Items Created: 0

🎉 Exemplary code quality!

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-3-manual-mode-switching-via-orchestrator.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 3 |
| Hidden Bugs | 4 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Non-atomic state updates in `onManualSwitch()` - multiple `strncpy` calls without atomic protection create a window where Core 1 preemption could leave orchestrator state partially updated | firmware/core/ModeOrchestrator.cpp:79-86 | +3 |
| 🟠 IMPORTANT | Missing state machine guard in `onManualSwitch()` - no check if already in target mode before calling `requestSwitch()`, allows redundant switches | firmware/core/ModeOrchestrator.cpp:75-90 | +1 |
| 🟠 IMPORTANT | SOLID S-violation - `onManualSwitch()` has too many responsibilities: calls registry, updates 5 state fields, changes state enum, logs - should be decomposed | firmware/core/ModeOrchestrator.cpp:75-90 | +1 |
| 🟠 IMPORTANT | SOLID O-violation - Boot path manually iterates mode table to find display names; should be encapsulated in `ModeRegistry::getModeName()` | firmware/src/main.cpp:789-794 | +1 |
| 🟠 IMPORTANT | SOLID I-violation - `onIdleFallback()` and `onFlightsRestored()` are public but only called from `tick()`; should be private | firmware/core/ModeOrchestrator.h:48-52 | +1 |
| 🟠 IMPORTANT | Performance footgun - `tick()` calls `ConfigManager::getModeSchedule()` every second, reading from NVS; schedule config should be cached on config change | firmware/core/ModeOrchestrator.cpp:146 | +1 |
| 🟡 MINOR | Partition validation too permissive - `validatePartitionLayout()` only warns on mismatch, doesn't prevent operation with wrong partition table | firmware/src/main.cpp:262-282 | +0.3 |
| 🟡 MINOR | Tech debt - Boot path `ConfigManager::setDisplayMode()` calls bypass NVS debounce mechanism (3 locations), creating dual-write path | firmware/src/main.cpp:802, 807, 823 | +0.3 |
| 🟢 CLEAN PASS | 1 |

### Evidence Score: 7.6

| Score | Verdict |
|-------|---------|
| **7.6** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] S - Single Responsibility Violation:** `onManualSwitch()` does too much - orchestrates registry call, updates 5 state fields (`_manualModeId`, `_manualModeName`, `_activeModeId`, `_activeModeName`, `_state`), resets `_activeRuleIndex`, and logs. This violates SRP by mixing coordination logic, state persistence, and logging in one method.
  - 📍 `firmware/core/ModeOrchestrator.cpp:75-90`
  - 💡 Fix: Extract state update logic to a private `updateManualState(const char* modeId, const char* modeName)` method called only after `requestSwitch()` succeeds.

- **[7/10] O - Open/Closed Violation:** Boot path manually iterates through `ModeRegistry::getModeTable()` to find display names by string comparison. If mode table structure changes (e.g., `displayName` renamed or reordered), this code breaks without compiler errors.
  - 📍 `firmware/src/main.cpp:789-794`
  - 💡 Fix: Add `static const char* ModeRegistry::getModeName(const char* modeId)` helper method that encapsulates table iteration logic.

- **[6/10] I - Interface Segregation Violation:** `onIdleFallback()` and `onFlightsRestored()` are public methods but are only called from `tick()`. No external caller should invoke these directly; they're implementation details of the state machine.
  - 📍 `firmware/core/ModeOrchestrator.h:48-52`
  - 💡 Fix: Move these methods to `private:` section; they're internal state transitions, not public API.

---

## 🐍 Pythonic Crimes & Readability

✅ No style violations detected - code follows C++ conventions consistently.

✅ No type safety issues - all types are appropriate for embedded context.

---

## ⚡ Performance & Scalability

- **[HIGH IMPACT] Unnecessary NVS reads in hot path:** Every second, `tick()` calls `ConfigManager::getModeSchedule()` which reads from NVS (1 + 4×ruleCount reads). For 8 rules, that's 33 NVS reads per second. NVS flash access has wear and latency costs.
  - 📍 `firmware/core/ModeOrchestrator.cpp:146`
  - 💡 Fix: Cache `ModeScheduleConfig` in a static member variable, reload only when `g_configChanged` atomic flag is set (already tracked by ConfigManager callback).

✅ No other significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug: Non-atomic state updates create race condition window:** `onManualSwitch()` performs 5 `strncpy` calls without atomic protection. If Core 1 preemption occurs between updating `_manualModeId` and `_activeModeId`, another thread could read inconsistent state where manual mode doesn't match active mode.
  - 📍 `firmware/core/ModeOrchestrator.cpp:79-86`
  - 🔄 Reproduction: Hard to reproduce deterministically due to low Core 1 preemption probability, but the window exists. An HTTP handler calling `getState()` immediately after `onManualSwitch()` could see partial state.

- **🐛 Bug: Missing guard allows redundant mode switches:** `onManualSwitch()` doesn't check if the requested mode is already active before calling `requestSwitch()`. While `ModeRegistry::requestSwitch()` is idempotent for same-mode requests, this wastes CPU cycles and creates unnecessary REQUESTED→IDLE state churn.
  - 📍 `firmware/core/ModeOrchestrator.cpp:75-90`
  - 🔄 Reproduction: Call `POST /api/display/mode { "mode": "live_flight" }` twice quickly. Second call will set REQUESTED state even though already on live_flight.

- **🐛 Bug: State machine allows invalid transitions:** `onManualSwitch()` can be called while in IDLE_FALLBACK or SCHEDULED states, but there's no state validation to ensure these transitions are valid. The method unconditionally sets state to MANUAL, which could violate the state machine invariants.
  - 📍 `firmware/core/ModeOrchestrator.cpp:75-90`
  - 🔄 Reproduction: While in IDLE_FALLBACK (zero flights), call manual switch. Orchestrator sets state to MANUAL but flight count is still 0. Next `tick()` will immediately trigger idle fallback again, creating mode thrashing.

- **🐛 Bug: Partition validation warnings don't prevent misconfiguration:** `validatePartitionLayout()` logs warnings for partition size mismatches but allows boot to continue. Running with wrong partition table will cause OTA failures or filesystem corruption at runtime.
  - 📍 `firmware/src/main.cpp:262-282`
  - 🔄 Reproduction: Flash firmware with old 1.5MB/1.5MB/1.9MB partition table but code expects new 1.5MB/1.5MB/960KB layout. System boots but OTA will fail.

✅ No security vulnerabilities detected.

✅ No lying tests detected - all tests have meaningful assertions.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Dual NVS write path for mode persistence:** Boot path calls `ConfigManager::setDisplayMode()` directly in 3 locations (lines 802, 807, 823), bypassing the `ModeRegistry::tickNvsPersist()` debounce mechanism. This creates two code paths for the same operation, increasing maintenance burden and risk of divergence.
  - 📍 `firmware/src/main.cpp:802, 807, 823`
  - 💥 Explosion radius: Medium - if debounce logic changes, boot paths must be updated separately. If boot path bugs appear, they're harder to detect in testing.

- **💣 Tech Debt: Boot path logic should be in ModeOrchestrator:** The 30-line boot mode restore block in `main.cpp` (lines 784-823) implements orchestrator logic (WDT policy, fallback chain, NVS correction). This belongs in `ModeOrchestrator::restoreBootPreference()` or similar, creating a clear separation of concerns.
  - 📍 `firmware/src/main.cpp:784-823`
  - 💥 Explosion radius: Medium - any changes to boot restore policy require editing main.cpp instead of the orchestrator class where it logically belongs.

- **💣 Tech Debt: Complex tick() method with multiple concerns:** `ModeOrchestrator::tick()` (lines 105-144) handles schedule evaluation, rule matching, state transitions, and idle fallback all in one 40-line method. This violates single responsibility and makes the code harder to test and reason about.
  - 📍 `firmware/core/ModeOrchestrator.cpp:105-144`
  - 💥 Explosion radius: Medium - adding new scheduling rules or state transitions requires modifying this already-complex method.

---

## 🛠️ Suggested Fixes

### 1. Extract State Update Logic to Prevent Non-Atomic Updates

**File:** `firmware/core/ModeOrchestrator.cpp`
**Issue:** Non-atomic state updates in `onManualSwitch()`

**Corrected code:**
```cpp
// NEW: Private method to update all state fields atomically (from caller's perspective)
void ModeOrchestrator::updateManualState(const char* modeId, const char* modeName, OrchestratorState newState) {
    strncpy(_manualModeId, modeId, sizeof(_manualModeId) - 1);
    _manualModeId[sizeof(_manualModeId) - 1] = '\0';
    strncpy(_manualModeName, modeName, sizeof(_manualModeName) - 1);
    _manualModeName[sizeof(_manualModeName) - 1] = '\0';
    _state = newState;
    _activeRuleIndex = -1;
    strncpy(_activeModeId, modeId, sizeof(_activeModeId) - 1);
    _activeModeId[sizeof(_activeModeId) - 1] = '\0';
    strncpy(_activeModeName, modeName, sizeof(_activeModeName) - 1);
    _activeModeName[sizeof(_activeModeName) - 1] = '\0';
}

// UPDATED: onManualSwitch() - simpler, delegates state updates
void ModeOrchestrator::onManualSwitch(const char* modeId, const char* modeName) {
    // AC #1: drive ModeRegistry so LED mode changes
    if (!ModeRegistry::requestSwitch(modeId)) {
        LOG_W("ModeOrch", "Manual switch: requestSwitch failed for mode");
        return;  // Don't update state on failure
    }
    
    // Update all state fields in one call
    updateManualState(modeId, modeName, OrchestratorState::MANUAL);
    LOG_I("ModeOrch", "Manual switch");
}
```

### 2. Add State Machine Guard to Prevent Redundant Switches

**File:** `firmware/core/ModeOrchestrator.cpp`
**Issue:** No check if already in target mode before calling `requestSwitch()`

**Diff:**
```diff
 void ModeOrchestrator::onManualSwitch(const char* modeId, const char* modeName) {
+    // Guard: skip redundant switches to avoid unnecessary REQUESTED state churn
+    if (_state == OrchestratorState::MANUAL && strcmp(_activeModeId, modeId) == 0) {
+        LOG_V("ModeOrch", "Manual switch to same mode, skipping");
+        return;
+    }
+
     // AC #1: drive ModeRegistry so LED mode changes
     if (!ModeRegistry::requestSwitch(modeId)) {
         LOG_W("ModeOrch", "Manual switch: requestSwitch failed for mode");
         return;
     }
```

### 3. Cache Schedule Config to Eliminate NVS Reads

**File:** `firmware/core/ModeOrchestrator.cpp`
**Issue:** Unnecessary NVS reads in hot path every second

**Diff:**
```diff
+// NEW: Static cache for schedule config to avoid NVS reads in tick()
+static ModeScheduleConfig s_cachedSchedule = {};
+static bool s_scheduleCacheValid = false;
+
 void ModeOrchestrator::tick(uint8_t flightCount, bool ntpSynced, uint16_t nowMinutes) {
     // Story dl-4.1: Evaluate mode schedule rules when NTP is synced.
     // Schedule evaluation runs BEFORE idle fallback logic so SCHEDULED
     // takes priority over IDLE_FALLBACK (AC #6).
 
     if (ntpSynced) {
-        ModeScheduleConfig sched = ConfigManager::getModeSchedule();
+        // Use cached schedule if valid, reload on config change
+        ModeScheduleConfig sched;
+        if (s_scheduleCacheValid) {
+            sched = s_cachedSchedule;
+        } else {
+            sched = ConfigManager::getModeSchedule();
+            s_cachedSchedule = sched;
+            s_scheduleCacheValid = true;
+        }
```

**Additional change needed:** Add `extern bool g_configChanged;` declaration and set `s_scheduleCacheValid = false` when config changes.

### 4. Add ModeRegistry Helper for Mode Name Lookup

**File:** `firmware/core/ModeRegistry.h`
**Issue:** Boot path manually iterates mode table

**Diff:**
```diff
     /// Zero-allocation callback enumeration (AC #4).
     /// Iterates all entries calling cb with (id, displayName, index, user).
     static void enumerate(void (*cb)(const char* id, const char* displayName,
                                      uint8_t index, void* user), void* user);
 
+    /// Get display name for a mode ID. Returns nullptr if mode not found.
+    /// Safe to call from any core (read-only table access).
+    static const char* getModeName(const char* modeId);
 
 private:
```

**File:** `firmware/core/ModeRegistry.cpp`

```diff
+const char* ModeRegistry::getModeName(const char* modeId) {
+    uint8_t idx = findIndex(modeId);
+    if (idx == MODE_INDEX_NONE) {
+        return nullptr;
+    }
+    return _table[idx].displayName;
+}
+
 uint8_t ModeRegistry::findIndex(const char* modeId) {
```

**File:** `firmware/src/main.cpp`

```diff
     {
         const ModeEntry* table = ModeRegistry::getModeTable();
         uint8_t count = ModeRegistry::getModeCount();
 
-        // Helper lambda: look up mode display name from table
-        auto findModeName = [&](const char* modeId) -> const char* {
-            for (uint8_t i = 0; i < count; i++) {
-                if (strcmp(table[i].id, modeId) == 0) return table[i].displayName;
-            }
-            return nullptr;
-        };

         if (g_wdtRecoveryBoot) {
             // dl-1.4, AC #1: WDT recovery — force clock unconditionally
-            const char* clockName = findModeName("clock");
+            const char* clockName = ModeRegistry::getModeName("clock");
```

### 5. Strengthen Partition Validation

**File:** `firmware/src/main.cpp`
**Issue:** Partition validation too permissive

**Diff:**
```diff
 void validatePartitionLayout() {
     Serial.println("[Main] Validating partition layout...");
 
     // Expected partition sizes from custom_partitions.csv (Story fn-1.1)
     // IMPORTANT: If you modify custom_partitions.csv, update these constants
     const size_t EXPECTED_APP_SIZE = 0x180000;   // app0/app1: 1.5MB
     const size_t EXPECTED_SPIFFS_SIZE = 0xF0000; // spiffs: 960KB
 
+    bool partitionMismatch = false;
+
     // Validate running app partition
     const esp_partition_t* running = esp_ota_get_running_partition();
     if (running) {
         if (running->size == EXPECTED_APP_SIZE) {
             Serial.printf("[Main] App partition: %s, size 0x%x (correct)\n", running->label, running->size);
         } else {
+            partitionMismatch = true;
             Serial.printf("[Main] WARNING: App partition size mismatch: expected 0x%x, got 0x%x\n",
                   EXPECTED_APP_SIZE, running->size);
@@ -281,6 +284,10 @@
     // Validate LittleFS partition
     const esp_partition_t* littlefs = esp_partition_find_first(
         ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
     if (littlefs) {
         if (littlefs->size == EXPECTED_SPIFFS_SIZE) {
             Serial.printf("[Main] LittleFS partition: size 0x%x (correct)\n", littlefs->size);
         } else {
+            partitionMismatch = true;
             Serial.printf("[Main] WARNING: LittleFS partition size mismatch: expected 0x%x, got 0x%x\n",
                   EXPECTED_SPIFFS_SIZE, littlefs->size);
         }
     } else {
+        partitionMismatch = true;
         Serial.println("[Main] WARNING: Could not find LittleFS partition");
     }
+
+    if (partitionMismatch) {
+        Serial.println("[Main] CRITICAL: Partition table mismatch detected!");
+        Serial.println("[Main] Refusing to boot - reflash with correct partition table via USB.");
+        Serial.println("[Main] See: https://github.com/espressif/arduino-esp32/blob/master/boards.txt");
+        while (true) {
+            delay(1000);  // Halt boot - requires USB reflash
+        }
+    }
 }
```

---

## Review Actions Summary

- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-16</var>
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
<var name="session_id">0596205f-3a87-472a-9d28-12728c1bb9b9</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="35e2a3a3">embedded in prompt, file id: 35e2a3a3</var>
<var name="story_id">dl-1.3</var>
<var name="story_key">dl-1-3-manual-mode-switching-via-orchestrator</var>
<var name="story_num">3</var>
<var name="story_title">3-manual-mode-switching-via-orchestrator</var>
<var name="template">False</var>
<var name="timestamp">20260416_0802</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-1.3)
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