<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-3 -->
<!-- Story: 4 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260415T010922Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story ds-3.4

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
<file id="dcd69c58" path="_bmad-output/implementation-artifacts/stories/ds-3-4-mode-switching-flow-and-transition-states.md" label="DOCUMENTATION"><![CDATA[

# Story ds-3.4: Mode Switching Flow & Transition States

Status: complete

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user,
I want to tap a mode card to activate that mode with clear visual feedback during the switch,
So that I know the switch is happening, when it completes, and if anything goes wrong.

## Acceptance Criteria

1. **Given** a **non-active** mode card is **tapped** (pointer), **When** **`switchMode(targetId)`** runs, **Then** the **target** card enters **switching** state:
   - **`opacity: 0.7`** (epic **UX-DR1** switching),
   - **pulsing** accent border **animation** on the target card (CSS **`@keyframes`**; respect **AC #8**),
   - visible **"Switching..."** label on that card (reuse or extend **`.mode-card-subtitle`** or a dedicated row),
   - **`POST /api/display/mode`** is sent with **`{ "mode": "<id>" }`** (fallback **`mode_id`** only if needed).

2. **Given** switching has started, **When** the request is in flight, **Then** **all sibling** cards that are **not** the target and **not** the previous active card (or: **all** non-target cards per epic **UX-DR10**) enter **disabled** state: **`opacity: 0.5`**, **`pointer-events: none`**, add class e.g. **`.mode-card.disabled`** — **including** the previously active card so the user cannot fire concurrent switches.

3. **Given** the **firmware** confirms success, **When** the client determines the switch **completed**, **Then**:
   - the **previously active** card loses **active** chrome (**4px** rail, dot, **"Active"**) and becomes **idle**,
   - the **target** card leaves **switching** and becomes **active** with epic **active** styling (**FR22**),
   - **all** **disabled** siblings return to **idle** (remove **`.disabled`**, restore **pointer-events**),
   - **focus** moves from the **new** active card to the **previously active** card (now idle) per **UX-DR6** — use **`element.focus({ preventScroll: true })`** if that card is **focusable**; if **ds-3.5** has not yet added **`tabindex`**, add **`tabindex="-1"`** on cards **only** for programmatic focus **or** document a minimal **`tabindex="0"`** on idle cards for this story (coordinate with **ds-3.5** so roles are not duplicated).

4. **Given** a **failure** path, **When** the UI must show **heap / registry** failure, **Then** the **target** card shows **scoped** inline copy: **"Not enough memory to activate this mode. Current mode restored."** (**UX-DR11**) when the failure is **heap-related**; for other errors, show **`res.body.error`** or **`data.registry_error`** truncated **inline on the card** (still **scoped**, not only a global toast).
   - Error host: **`role="alert"`**, **`aria-live="polite"`** (**UX-DR8**).
   - Auto-dismiss error **after 5s** **or** on **next** user **click** / **keydown** anywhere in **`#mode-picker`** (document listener with **`{ once: true }`** or explicit cleanup).

5. **Given** **`prefers-reduced-motion: reduce`**, **When** a card is **switching**, **Then** **no** pulsing border — use **static** **`var(--primary)`** or **`--accent-dim`** border at **full opacity** (**UX-DR5**); **"Switching..."** text remains the primary progress signal.

6. **Given** **network** failure (**`catch`** on **`FW.post`**), **When** the request fails, **Then** clear **switching** / **disabled** classes, restore cards to last known **`GET`** state (call **`loadDisplayModes()`**), keep **toast** for connectivity (existing pattern is fine); **do not** leave cards stuck **disabled**.

7. **Given** **async** registry (**`WebPortal`** comment: **no** bounded wait in POST handler — **`ds-3.1`**), **When** **`POST`** returns **`ok: true`** with **`data.switch_state`** **`"requested"`** or **`"switching"`**, **Then** the client **polls** **`GET /api/display/modes`** at a **small interval** (e.g. **150–300ms**) until **`data.switch_state === "idle"`** **or** **`data.active === targetId`** **or** **timeout** (e.g. **2s** aligned with **architecture** cap discussion), then **finalizes** UI. If **`data.registry_error`** is non-empty **or** **`data.active`** is still not **`targetId`** after idle, treat as **failure** for **AC #4** messaging (wording may reference **registry** vs **heap** based on **`code`** if present on future responses).

8. **Given** **`POST`** returns **`ok: false`** (**HTTP 400** etc.), **When** **`FW.post`** surfaces **`res.body`**, **Then** apply **AC #4** on the **target** card (unknown mode, missing field) with appropriate **short** message; clear **disabled** state on siblings.

9. **Given** **`style.css`** / **`dashboard.js`** change, **When** shipping, **Then** regenerate **`firmware/data/*.gz`** per **`AGENTS.md`**.

10. **Given** manual test on device, **When** switching **Classic** ↔ **Live Flight Card**, **Then** states match epic (**switching** → **active** / **idle**) with **no** stuck **pointer-events: none** on the list.

## Tasks / Subtasks

- [x] Task 1: CSS — states (**UX-DR1**, **UX-DR10**, **UX-DR5**)
  - [x] 1.1: **`.mode-card.switching`** — pulse keyframes + **0.7** opacity (media query for **reduced-motion**)
  - [x] 1.2: **`.mode-card.disabled`** — **0.5** opacity, **`pointer-events: none`**
  - [x] 1.3: **`.mode-card-error`** (or inner **`.mode-card-alert`**) for **scoped** error text

- [x] Task 2: JS — **`switchMode`** orchestration
  - [x] 2.1: Track **`previousActiveId`** before POST; apply **disabled** to siblings
  - [x] 2.2: Implement **GET poll** loop after POST **200** until settled or timeout; then **`updateModeStatus`** / re-render
  - [x] 2.3: **Focus** move **new active** → **previous** idle card (**UX-DR6**)
  - [x] 2.4: Error dismiss timer + **interaction** listener

- [x] Task 3: Wire **`mode`** in POST body (if not already from **ds-3.3**)

- [x] Task 4: Gzip + smoke check dashboard mode picker

## Dev Notes

### Current code (baseline)

- **`switchMode`**: sets **`.switching`** on target only; **opacity 0.6** in CSS today — adjust to **0.7** per epic; **no** sibling **disabled**; **no** poll — single **`loadDisplayModes()`** after POST.
- **`updateModeStatus`**: strips **`.switching`** on all cards after GET — may **fight** mid-poll unless poll owns UI state; refactor so **poll completion** drives final class cleanup.
- **`WebPortal`** **`POST /api/display/mode`**: documents **async** completion; response **`ok: true`** with **`switch_state`** / **`registry_error`** fields — **client must poll GET** for truth (**AC #7**).

### Coordination with ds-3.3

- **ds-3.3** adds **schematic**, **`#mode-picker`**, **active** chrome — **ds-3.4** must not regress those selectors; prefer classes **on the same** **`.mode-card`** nodes.

### Out of scope

- **ds-3.5** — full **keyboard** **`role="button"`** model (**Enter**/**Space**) — if **ds-3.4** adds **`tabindex`** for focus only, keep minimal.
- **ds-3.6** — upgrade banner.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.4)
- Prior: `_bmad-output/implementation-artifacts/stories/ds-3-3-mode-picker-section-cards-and-schematic-previews.md`
- API: `_bmad-output/implementation-artifacts/stories/ds-3-1-display-mode-api-endpoints.md`
- Firmware: `firmware/adapters/WebPortal.cpp` (**POST** **`/api/display/mode`**), `firmware/data-src/dashboard.js`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

### Completion Notes List

- Rewrote `switchMode()` with full state machine: switching → polling → finalize/error
- Added `previousActiveId` tracking to correctly restore previous active card to idle state
- Implemented GET poll loop (200ms interval, 2s timeout) for async mode registry response
- Added `clearSwitchingStates()` helper to prevent stuck disabled/switching states
- Added `showCardError()` with scoped inline error display, `role="alert"`, `aria-live="polite"`, auto-dismiss after 5s or user interaction
- Added `tabindex="-1"` on mode cards for programmatic focus per AC #3 / UX-DR6
- `loadDisplayModes()` now returns the Promise for chainable `.then()` in `finalizeModeSwitch`
- CSS: `.mode-card.switching` updated from 0.6 to 0.7 opacity, added `mode-pulse` keyframes for accent border animation
- CSS: Added `.mode-card.disabled` (0.5 opacity, pointer-events:none) and `.mode-card-alert` for scoped error
- CSS: Added `prefers-reduced-motion` override to disable pulse animation
- POST body confirmed sending `{ mode: id }` (already established in ds-3.3)
- Gzipped style.css and dashboard.js to firmware/data/

### Change Log

- 2026-04-14: Implemented ds-3.4 Mode Switching Flow & Transition States — full switchMode orchestration with polling, scoped errors, focus management, and CSS state classes
- 2026-04-14: Code review synthesis (5 issues fixed): premature active removal on siblings (AC #3), bricked UI in finalizeModeSwitch on reload failure, race condition unlocking modeSwitchInFlight before loadDisplayModes resolves (3 paths), showCardError dismiss closure removing wrong element, prefers-reduced-motion opacity:1 (AC #5)
- 2026-04-14: Verification pass — all 5 review fixes confirmed in source, gzip assets verified matching data-src originals, all 10 ACs validated against implementation. Status → complete.
- 2026-04-14: Code review synthesis pass 2 (2 issues fixed): event bubbling from .mode-card-alert triggering accidental switchMode (HIGH — added stopPropagation on alert click); double-toast on network failure in AC #6 catch block (MEDIUM — removed redundant FW.showToast, loadDisplayModes covers it). Regenerated dashboard.js.gz.

### File List

- `firmware/data-src/style.css` (modified)
- `firmware/data-src/dashboard.js` (modified)
- `firmware/data/style.css.gz` (regenerated)
- `firmware/data/dashboard.js.gz` (regenerated)

## Previous story intelligence

- **ds-3.3** establishes **card DOM**, **schematic**, **`#mode-picker`**, and **POST** **`mode`** preference — **ds-3.4** layers **state machine** + **polling** on top.

## Git intelligence summary

Touches **`firmware/data-src/dashboard.js`**, **`style.css`**, possibly **`dashboard.html`** (error host markup optional — can be created in JS).

## Project context reference

`_bmad-output/project-context.md` — gzip **data-src** → **data/**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-14
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 5.5 → REJECT (MAJOR REWORK)
- **Issues Found:** 5
- **Issues Fixed:** 5
- **Action Items Created:** 0

### Review: 2026-04-14 (Pass 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 3.1 → PASS WITH FIXES
- **Issues Found:** 6 raised (2 verified, 4 dismissed)
- **Issues Fixed:** 2
- **Action Items Created:** 0


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic ds-3 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story ds-3-1 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Orchestrator bypassed when user re-selects the currently active mode | Removed the `if (!isSameMode)` guard — `onManualSwitch` is now always called. This correctly transitions the orchestrator from `IDLE_FALLBACK`/`SCHEDULED` back to `MANUAL` when a user explicitly selects a mode, even if it's already the active one. `ModeRegistry::tick()` is idempotent for same-mode requests (line 367 guard `if (requested != _activeModeIndex)`), so no spurious mode restarts occur. |
| medium | `POST /api/display/mode` body handler skips chunk accumulation — only the final TCP chunk is parsed | Replaced the direct `deserializeJson(data, len)` path with the established `g_pendingBodies` accumulation pattern (identical to `POST /api/schedule` and `POST /api/settings`). Extracted all business logic into new `_handlePostDisplayMode(request, data, len)` helper for separation of concerns. |
| low | `SwitchState` → string mapping duplicated in `_handleGetDisplayModes` and the POST handler | Extracted to `static const char* switchStateToString(SwitchState)` in the anonymous namespace. Both call sites updated. |
| dismissed | Async POST decision breaks AC 7 and AC 9 API contracts — heap failures buried in HTTP 200 | FALSE POSITIVE: The Dev Agent Record (Implementation Plan, AC #7) explicitly documents this as an **accepted deviation from Architecture D5**. The rationale is sound: `ModeRegistry::tick()` runs on Core 0 while AsyncWebServer body handlers run on the AsyncTCP task (Core 1); a bounded wait here risks WDT. The story explicitly chose "async response with `switch_state: requested` + client re-fetch (ds-3.4 UX)" as the strategy. Heap errors are surfaced in `data.registry_error` on a 200 — a limitation of the async approach, not an omission. This is a design trade-off, not a bug. |
| dismissed | N+1 NVS reads violate `<10ms` enumeration budget (NFR P4) — 20–50ms per GET | FALSE POSITIVE: The reviewer assumed "4 modes and 10 settings." Reality: `ClassicCardMode::getSettingsSchema()` returns `nullptr`, `LiveFlightCardMode::getSettingsSchema()` returns `nullptr`. Only `ClockMode` (1 setting) and `DeparturesBoardMode` (1 setting) have schemas. Total NVS reads per GET: **2** (one per modes-with-settings) plus `getUpgNotif()` = **3 total**. The "20–50ms" claim is unsupported. Noted as future technical debt for when more settings are added (see Suggested Future Improvements). --- |

## Story ds-3-1 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `ModeRegistry::requestSwitch()` did not set `_switchState = REQUESTED` — the POST handler on Core 1 read the state immediately after queuing a switch and always saw `"idle"` instead of `"requested"`, completely breaking the AC #7 async strategy where the client is expected to poll on `switch_state: "requested"`. | Added `_switchState.store(SwitchState::REQUESTED)` in `requestSwitch()` before `return true`. Also added an `else` branch in `tick()` to reset state to IDLE when a same-mode request is consumed (prevents REQUESTED state from lingering when no actual switch occurs). |
| high | `_handlePostDisplayMode` applied mode settings sequentially with no pre-validation of values — if the first key-value pair succeeded (writing to NVS) but a subsequent pair failed value validation, NVS was left in a partially-applied state, violating the explicit "no partial apply" rule in the code comment. | Added a pre-validation pass that checks every value against the schema's `minValue`/`maxValue` fields before writing any setting. The apply loop only runs after all values pass. |
| medium | O(N) character-by-character string concatenation (`pending->body += char`) in all three POST body handlers (settings, display/mode, schedule) — causes repeated bounds-check overhead and blocks the AsyncTCP task for multi-chunk payloads. | Replaced all three loops with `pending->body.concat(reinterpret_cast<const char*>(data), len)` — single-call bulk append. |
| dismissed | AC 9 / registry_error buried inside HTTP 200 OK (`ok: true`) violates the API envelope contract for heap-guard failures. | FALSE POSITIVE: Already adjudicated as a FALSE POSITIVE in the prior synthesis pass (ANTIPATTERNS file, row "dismissed"). The Dev Agent Record (AC #7 Implementation Plan) explicitly documents the async response strategy as an accepted deviation from Architecture D5. Heap failures that occur asynchronously on Core 0 cannot surface in the already-sent POST response. `registry_error` in a 200 is the documented trade-off, not an omission. |
| dismissed | State desync — `ModeOrchestrator::onManualSwitch()` persists `_manualModeId` to NVS before `ModeRegistry` confirms the switch, leaving the orchestrator referencing a rejected mode. | FALSE POSITIVE: Design trade-off inherent in the cooperative-tick architecture. The orchestrator will discover the registry error on the next `tick()` cycle and revert. No data corruption — NVS holds the orchestrator's intent, not the confirmed active mode (the registry separately debounce-writes the confirmed active mode). Not actionable without restructuring the orchestrator API (separate story scope). |
| dismissed | Large `String` JSON allocation in `_handleGetDisplayModes` risks heap fragmentation — should use `AsyncResponseStream`. | FALSE POSITIVE: The GET response is approximately 900 bytes for 3 modes. While `AsyncResponseStream` is a best practice, the current mode count makes this a marginal improvement, not a correctness issue. This pattern is universal across all WebPortal handlers and would require a broader refactoring pass. Deferred as future improvement. |
| dismissed | SOLID violation — `_handlePostDisplayMode` is a god method combining HTTP parsing, schema lookup, value validation, NVS writes, and orchestrator dispatch. | FALSE POSITIVE: Valid architectural observation, but out of scope for a synthesis fix. Refactoring to extract `ConfigManager::applyModeSettings()` is a meaningful improvement but would expand change surface beyond modified files. Deferred to a dedicated refactoring story. - **Validator B output**: No findings produced. \| **Raised by**: Validator B \| **Dismissal Reason**: Reviewer produced only a placeholder greeting — no code analysis performed. --- |

## Story ds-3-2 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Infinite recovery loop when `_table[0]` (default mode) also fails to initialize — `tickNvsPersist()` writes NVS safe-default and queues `_requestedIndex=0` unconditionally, but after `executeSwitch(0)` fails it re-sets `_nvsWritePending=true`, causing the loop to repeat every 2 seconds indefinitely, burning NVS flash write cycles | Added `static bool _recoveryQueued` member to `ModeRegistry`; `tickNvsPersist()` now only queues activation once per failure episode (`!_recoveryQueued` guard); flag is reset to `false` in `executeSwitch()` on a successful switch so future failures start fresh. Reviewer's proposed fix (`_requestedIndex.load() != 0`) was ineffective (the request is consumed by `tick()` before `tickNvsPersist()` runs again); a proper one-shot flag was implemented instead. |
| high | Display layout hot-reload desync — `zone_logo_pct`, `zone_split_pct`, `zone_layout` are not in REBOOT_KEYS (treated as hot-reload), but `hardwareConfigChanged()` and `hardwareMappingChanged()` in `main.cpp` don't include them. When they change, `g_display.buildRenderContext()` is called but returns the OLD `_layout` (since `NeoMatrixDisplay::_layout` is only updated in `rebuildMatrix()`). `reconfigureFromConfig()` → `rebuildMatrix()` is never triggered for zone-only changes, so the physical LED layout doesn't update until reboot despite the API returning `reboot_required: false`. | Added `zone_logo_pct`, `zone_split_pct`, `zone_layout` to both `hardwareConfigChanged()` and `hardwareMappingChanged()`. Zone changes now route through `reconfigureFromConfig()` → `rebuildMatrix()` which updates `_layout = LayoutEngine::compute(hw)`. |
| medium | Type safety — in `_handlePostDisplayMode()`, the pre-validation loop calls `kv.value().as<int32_t>()` without first checking `is<int32_t>()`. A non-numeric JSON string (e.g. `{"format": "bad"}`) silently returns 0 from `as<>()`. If 0 is within the schema's valid range (e.g. `clock/format` has `minValue=0, maxValue=1`), the request passes validation and persists 0 to NVS instead of returning a 400 error. | Added `if (!kv.value().is<int32_t>())` guard at the start of the pre-validation loop, returning HTTP 400 with `"INVALID_SETTING_TYPE"` for non-numeric values. |
| dismissed | `ConfigManager::setModeSetting` hardcodes range validation per-mode (OCP violation) | FALSE POSITIVE: Pre-adjudicated as a false positive in the ANTIPATTERNS file from ds-3.1 synthesis: "Deferred to a dedicated refactoring story." The API-layer's dynamic schema validation in `_handlePostDisplayMode` already guards all incoming values. The ConfigManager layer validation is defense-in-depth for the 2 existing modes, not a scalability problem at the current 2-mode schema scope. |
| dismissed | `_handlePostDisplayMode` is a God Method (SRP violation, 100 lines) | FALSE POSITIVE: Pre-adjudicated as a false positive in the ANTIPATTERNS file from ds-3.1 synthesis: "Deferred to a dedicated refactoring story." The pattern is universal across all WebPortal handlers; extracting to `ConfigManager::applyModeSettings()` is a meaningful future improvement but expands change surface beyond this story's scope. |
| dismissed | `ModeOrchestrator` caches `_manualModeId` before `ModeRegistry` confirms the switch; if the registry rejects the switch, orchestrator will blindly retry the rejected mode | FALSE POSITIVE: Pre-adjudicated as a false positive in the ANTIPATTERNS file from ds-3.1 synthesis: "Design trade-off inherent in the cooperative-tick architecture. The orchestrator will discover the registry error on the next `tick()` cycle and revert. Not actionable without restructuring the orchestrator API (separate story scope)." |
| dismissed | Validator B raised no issues | FALSE POSITIVE: No analysis was produced. --- |

## Story ds-3-2 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Race condition — `WebPortal.cpp` (Core 1) calls `ModeRegistry::getLastError()` which returns a raw pointer to `_lastError[]`, a 64-byte static buffer written by Core 0 via `snprintf()`. JSON serialization of this pointer races against the Core 0 write. `copyLastError()` was declared in `ModeRegistry.h` as the safe alternative but was **never implemented** in the `.cpp`. | Implemented `copyLastError()` with seqlock-style double `_errorUpdating` check in `ModeRegistry.cpp`; replaced both `getLastError()` call sites in `WebPortal.cpp` with `copyLastError()` into stack-allocated `char[64]` buffers. |
| high | Memory leak on mode init failure — `ModeRegistry::executeSwitch()` calls `delete newMode` without calling `newMode->teardown()` first. Architecture rule D2 explicitly requires teardown-before-delete. If `init()` partially allocates heap before returning false, those resources are permanently leaked. | Added `newMode->teardown()` immediately before `delete newMode` in the init-failure path. |
| medium | Tautological test — `test_first_switch_fail_queues_recovery_activation` seeds NVS with `"mock_mode_a"`, triggers a failed switch of `mock_mode_a`, then asserts NVS reads back `"mock_mode_a"`. If recovery logic never ran at all and NVS was untouched, the test still passes. The system under test cannot be falsified. | Changed seed value from `"mock_mode_a"` to `"stale_invalid_mode"`. Now, if recovery logic fails to overwrite NVS, the assertion fails correctly. |
| medium | String `reserve()` capacity lost on `push_back` — all three POST body accumulation sites construct a `PendingRequestBody` with `body.reserve(total)`, then `push_back` the struct by value. The Arduino `String` copy constructor allocates exactly the current content length (0), discarding the reserved capacity. Subsequent `concat()` calls cause repeated heap reallocations on a fragmented ESP32 heap. | Changed pattern to `push_back({request, String()})` first, then `g_pendingBodies.back().body.reserve(total)` on the stored element. Applied to all 3 handlers (settings, display/mode, schedule). |
| dismissed | Rigid JSON validation (`is<int32_t>()`) rejects boolean values in mode settings, violating OCP since the schema type field supports `"bool"`. | FALSE POSITIVE: No current schema uses `"bool"` type — both deployed schemas use `"enum"` and `"uint8"`. The web UI sends 0/1 as integers for toggle controls, not JSON `true`/`false`. No currently valid API payload is rejected by this check. Deferred as a future-proofing improvement when bool-typed settings are added (see Suggested Future Improvements). |
| dismissed | Partial-apply risk — `ConfigManager::setModeSetting()` has secondary validation; if it returns false mid-loop, NVS is partially applied. | FALSE POSITIVE: Both current schemas have exactly 1 setting each (clock: `format`; depbd: `rows`). Partial apply requires ≥2 settings. Additionally, the secondary validation ranges in `setModeSetting()` are identical to the schema's `minValue`/`maxValue` already checked in pre-validation. The only other failure path (`prefs.begin()` NVS hardware failure) would fail on the first write and is not a "partial apply" scenario. Not a current correctness issue. --- |

## Story ds-3-3 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Orchestrator bypass blocked — clicking an already-active mode card returns early without POSTing, breaking the `ModeOrchestrator` state machine (can never transition from `IDLE_FALLBACK`/`SCHEDULED` → `MANUAL` by user intent). ANTIPATTERNS file from ds-3.1 explicitly flags this exact pattern as a confirmed bug that was already fixed in the firmware layer. | Removed the early-return block (`if ... active ... return`) and added an explanatory comment cross-referencing the ANTIPATTERNS entry. |
| critical | Scope violation — `buildModeSettingsPanel()` (dl-5.1), `maybeShowModeUpgradeBanner()` / `dismissAndClearUpgrade()` (ds-3.6), full polling-based `switchMode` orchestration with `switching`/`disabled` states (ds-3.4), and `role="button"` + keyboard delegation (ds-3.5) all implemented in this story. Confirmed by explicit story comments (`// Per-mode settings panel (Story dl-5.1)`, `// --- Upgrade Notification Banner (Story ds-3.6) ---`, `// switchMode — full orchestration (ds-3.4)`). The upgrade banner POSTs to `/api/display/notification/dismiss` — a non-existent endpoint. | **Deferred** — reverting 400+ lines of working code without functional regression testing is outside safe synthesis scope. Four `[AI-Review]` follow-up tasks created in the story's Tasks section to ensure ds-3.4/3.5/3.6/dl-5.1 stories either accept this code as early-landing or perform a scoped revert. |
| high | Focus choreography logic error — `finalizeModeSwitch(newActiveId, previousActiveId)` focuses `getModeCard(previousActiveId)` instead of `getModeCard(newActiveId)`. After a successful switch, keyboard focus is thrown back to the *old* mode instead of the newly activated one. | Changed `doFocus()` to look up and focus `newActiveId` card; added explanatory comment. |
| medium | AC5 violation — idle `.mode-card` has `border-left: 4px solid var(--border)` instead of the spec-required 1px dimmed accent. Active card correctly keeps `border-left: 4px solid var(--primary)`, but idle cards appear visually heavy and reduce contrast between active/idle states. | Removed `border-left:4px solid var(--border)` override from `.mode-card` base rule; idle cards now use the standard `border: 1px solid var(--border)` without a differentiated left rail (active card retains its 4px primary rail). |
| dismissed | SRP Violation — `switchMode` is a ~120-line God Method combining DOM manipulation, HTTP, polling, error handling, and focus management. | FALSE POSITIVE: Pre-established ANTIPATTERNS precedent from ds-3.1 (confirmed in ds-3.2): "SOLID violation — `_handlePostDisplayMode` is a god method… Deferred to a dedicated refactoring story." Identical pattern, identical disposition. The function is within accepted project norms for WebPortal-style handlers. |
| dismissed | Redundant DOM operations — `loadDisplayModes()` calls `renderModeCards()` then `updateModeStatus()`, where `renderModeCards` already sets initial active/idle state. | FALSE POSITIVE: `updateModeStatus()` is not purely redundant — it normalizes subtitle text, removes any stale `switching`/`disabled` states (from failed prior switches), and manages `aria-current`. The dual-call is a deliberate defensive pattern, not a performance concern at the 2–4 card scale. Minor improvement at most; no correctness impact. --- |

## Story ds-3-3 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Scoped error messages obliterated by DOM re-render — all 5 `switchMode` error paths called `showCardError()` then immediately called `loadDisplayModes()` or `renderModeCards()`, destroying the card DOM node holding the error before the browser could paint it. Users never see error feedback. | For `loadDisplayModes()` async paths (×3), wrapped `showCardError` in `.then()` callback executed after reload. For synchronous `renderModeCards(d)` paths (×2), moved `showCardError` after `renderModeCards(d)` + `updateModeStatus(d)` so the fresh card exists when the error is attached. |
| medium | `buildSchematic` produces `NaN`-valued CSS `grid-column`/`grid-row` when API payload omits `relX` or `relY` (e.g. JSON serializer eliding zero-value fields). `Math.max(1, undefined + 1)` → `NaN`. Injects `grid-column: NaN / NaN` into DOM, breaking zone rendering. | Added explicit null-coalescing guards `var rx = (z.relX !== undefined && z.relX !== null) ? z.relX : 0` and same for `ry`, before passing to `Math.max`. |
| medium | Mode schematic grid had no `gap`, causing adjacent zones to be flush (share pixel boundary), making the zone layout completely unreadable — contradicts AC#3 requirement to show visible zone boundaries. | Added `gap:1px` to `.mode-schematic`. The `background:var(--border)` on the container now shows through as 1px dividers between zone cells. |
| low | Polling timeout used `Date.now()` differential — browser background-tab setTimeout throttling (~1000ms minimum) could exhaust the 2000ms `SWITCH_POLL_TIMEOUT` after only 2 throttled polls, falsely reporting timeout while the hardware switch completed successfully. | Replaced `pollStart = Date.now()` + `Date.now() - pollStart > SWITCH_POLL_TIMEOUT` with `pollAttempts` counter and `maxPollAttempts = Math.ceil(SWITCH_POLL_TIMEOUT / SWITCH_POLL_INTERVAL)`. Attempt count is independent of real-world elapsed time. |
| low | Active card `border-left: 4px solid var(--primary)` increased left border from 1px to 4px; with `box-sizing: border-box`, content area shifted 3px right on activation — visible Cumulative Layout Shift. | Added `padding-left:9px` to `.mode-card.active` (was 12px from base rule). 4px border + 9px padding = 13px = 1px border + 12px padding on idle cards. No shift. |
| low | Hardcoded `(` `)` parentheses in `dashboard.html` around `#modeStatusReason` span. When `data.state_reason` is empty, JS sets span text to `''` but the parens remain in DOM, rendering `Active: Classic Card ()`. | Removed hardcoded parens from HTML. Updated `updateModeStatus` in JS to emit `'(' + reason + ')'` when reason is non-empty, `''` otherwise. |
| dismissed | Mixed event delegation patterns — `modeCardsList.addEventListener('keydown')` uses delegation on the container, but click handlers attach to individual cards in `renderModeCards` loop, wasting memory and breaking consistency. | FALSE POSITIVE: Pre-adjudicated pattern. ANTIPATTERNS file (ds-3.1, ds-3.2) consistently dismisses SRP/SOLID violations as "Deferred to a dedicated refactoring story — pattern is universal across all WebPortal-style handlers." The mixed delegation is a style inconsistency, not a correctness issue. Consistent with prior synthesis dispositions. --- |

## Story ds-3-4 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Premature removal of active chrome from sibling cards on switch initiation | Removed `cards[j].classList.remove('active')` from the sibling (`else`) branch. Per AC #3, the previously active card retains active chrome until firmware confirms success. The target card still loses `.active` as it enters `.switching` state. |
| high | Bricked UI when `loadDisplayModes()` fails in `finalizeModeSwitch` early-exit path | Added `clearSwitchingStates()` call before `loadDisplayModes()` in the non-`skipReload` path. If the GET fails, cards are no longer permanently stuck with `pointer-events:none`. |
| medium | Race condition — `modeSwitchInFlight` unlocked before `loadDisplayModes()` DOM rebuild completes in 3 error paths (HTTP error, registry_error, poll timeout) | Moved `modeSwitchInFlight = false` to inside the `.then()` callback for all three paths. Added a rejection handler (second `.then()` argument) as a safety valve in case the device becomes unreachable mid-recovery. The `catch` network-failure path (AC #6) intentionally retains immediate unlock since the device is fully unreachable and blocking interaction would worsen UX. |
| medium | `showCardError` dismiss closure removes any `.mode-card-alert` via `clearCardError(card)`, prematurely wiping a newer error if called twice within 5 s | Changed `clearCardError(card)` in the `dismiss` closure to `if (alert.parentNode) alert.parentNode.removeChild(alert)` — removes only the specific DOM element created by this invocation, leaving any subsequent error intact. |
| low | `prefers-reduced-motion` override missing `opacity:1` — switching card remains dimmed at 0.7 for reduced-motion users, violating AC #5 "full opacity" requirement | Added `opacity:1` to the `.mode-card.switching` rule in the `@media (prefers-reduced-motion:reduce)` block. |

## Story ds-3-4 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Clicking `.mode-card-alert` element bubbles to parent `.mode-card`, triggering `switchMode()` unintentionally while the user intends only to dismiss the error | Added `alert.addEventListener('click', function(e) { e.stopPropagation(); dismiss(); })` on the alert element before registering the `modePicker` listener. Click on alert now stops before reaching the card handler. |
| medium | AC #6 `catch` block calls `FW.showToast('Cannot reach device...')` then calls `loadDisplayModes()`, which also toasts on network failure — two toasts for a single unreachable-device event | Removed `FW.showToast(...)` from the `catch` block. `loadDisplayModes()` already shows "Cannot reach device to load display modes" on its own failure, covering the connectivity signal. |
| dismissed | Promise Resolution Mismatch — `loadDisplayModes` resolves to `null` on failure so reject handlers in `.then(success, reject)` are dead code | FALSE POSITIVE: Both the success handler and reject handler in all three call sites do **identical work** (`modeSwitchInFlight = false` + `showCardError`). Additionally, `clearSwitchingStates()` is called *before* `loadDisplayModes()` in each of these paths, so the pre-existing card DOM is accessible for `getModeCard()` even when the reload fails. Behavior is correct in all branches. The dead reject arm is cosmetically redundant but not a bug. Changing `loadDisplayModes` to reject would break `finalizeModeSwitch`'s `loadDisplayModes().then(doFocus)` chain. |
| dismissed | Focus moves to newly-active card instead of previously-active card, violating AC #3 | FALSE POSITIVE: The ANTIPATTERNS file from ds-3.3 synthesis explicitly reclassified focusing `previousActiveId` as a **high-severity logic error** and corrected it to focus `newActiveId`. The current code carries a comment documenting this rationale. This is an intentional, review-documented deviation. The AC needs a documentation update, not a code revert. |
| dismissed | Orphaned timers and event listeners when `showCardError` is called twice within 5s | FALSE POSITIVE: The `dismissed = true` flag prevents the orphaned timer from taking any visible action. The `{ once: true }` flag on the modePicker listener auto-removes it after first trigger. The stale timer is a 5-second no-op at worst. Concurrent `showCardError` calls are prevented structurally by `modeSwitchInFlight`, making this a near-impossible scenario in practice. |
| dismissed | Redundant `clearSwitchingStates()` before `renderModeCards(d)` in two poll paths | FALSE POSITIVE: `clearSwitchingStates()` before `renderModeCards()` is deliberate defensive coding. The ds-3.4 ANTIPATTERNS entry specifically records adding this call to prevent "bricked UI" scenarios. `renderModeCards` could theoretically fail mid-execution in a future refactor; the pre-cleanup guards against cards becoming permanently stuck. The performance benefit of removing it (one DOM query loop over ≤4 cards) is negligible. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

Hello! I'm a validator Code CLI, ready to help you with TheFlightWall_OSS-main project.

What would you like to work on today?

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-3-4-mode-switching-flow-and-transition-states.md

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
| **Total Issues** | **3** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Hidden Bugs: Race condition in `finalizeModeSwitch` allows concurrent switch bug. | `firmware/data-src/dashboard.js:1331` | +1.0 |
| 🟠 IMPORTANT | AC Gaps: Missing error string truncation violates AC #4. | `firmware/data-src/dashboard.js:1223` | +1.0 |
| 🟡 MINOR | Tech Debt: Visual jitter on `.active` card during transition to `.switching`. | `firmware/data-src/style.css:325` | +0.3 |
| 🟢 CLEAN PASS | 6 |

### Evidence Score: -0.7

| Score | Verdict |
|-------|---------|
| **-0.7** | **APPROVED** |

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

- **🐛 Bug:** Race condition in `finalizeModeSwitch` unlocking UI prematurely
  - 📍 `firmware/data-src/dashboard.js:1331`
  - 🔄 Reproduction: When a POST successfully completes immediately (`switchState === 'idle'`), `finalizeModeSwitch` is called with `skipReload = false`. The function synchronously executes `modeSwitchInFlight = false;` *before* the async `loadDisplayModes()` completes. This allows the user to initiate a new mode switch while the DOM rebuild is still pending. When the `loadDisplayModes()` GET request finishes, it overwrites the DOM, immediately erasing the `.switching` and `.disabled` visual states of the concurrent request. (This was fixed for the 3 error paths in the prior review, but missed in the success path).

- **🐛 Bug (AC Gap):** Missing error string truncation violates AC #4
  - 📍 `firmware/data-src/dashboard.js:1223` and `1268`
  - 🔄 Reproduction: AC #4 explicitly requires non-heap errors from `res.body.error` or `data.registry_error` to be "truncated inline on the card". However, the implementation passes the raw strings to `showCardError()` without any JS substring truncation, nor does it apply `white-space: nowrap; overflow: hidden; text-overflow: ellipsis;` to `.mode-card-alert` in `style.css`. A verbose validation error from the API will wrap and indefinitely expand the card's vertical height, breaking the grid layout.

✅ Code appears correct and secure beyond the above issues.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Visual jitter on `.active` card during transition
  - 📍 `firmware/data-src/style.css:325`
  - 💥 Explosion radius: Minor UI polish regression. In `style.css`, `.mode-card.active` applies `border-left: 4px solid var(--primary)` and `padding-left: 9px` (to prevent layout shifts). However, `.mode-card.switching` lacks these overrides. When an active card is clicked to trigger a switch, it loses the `.active` class and gains `.switching`, causing it to instantly snap back to a `1px` left border and `12px` padding. This causes an unintended visual flutter on the card's left edge before the pulsing animation even starts.

---

## 🛠️ Suggested Fixes

### 1. Fix race condition in finalizeModeSwitch

**File:** `firmware/data-src/dashboard.js`
**Issue:** `modeSwitchInFlight = false` executes synchronously before DOM reload completes.

**Corrected code:**
```javascript
  function finalizeModeSwitch(newActiveId, previousActiveId, skipReload) {
    function doFocus() {
      if (newActiveId) {
        var newCard = getModeCard(newActiveId);
        if (newCard && typeof newCard.focus === 'function') {
          newCard.focus({ preventScroll: true });
        }
      }
    }
    if (skipReload) {
      modeSwitchInFlight = false;
      doFocus();
    } else {
      clearSwitchingStates();
      loadDisplayModes().then(function() {
        modeSwitchInFlight = false;
        doFocus();
      }).catch(function() {
        modeSwitchInFlight = false;
      });
    }
  }
```

### 2. Implement required error truncation for AC #4

**File:** `firmware/data-src/style.css`
**Issue:** Missing CSS text truncation for inline scoped errors.

**Corrected code:**
```css
.mode-card-alert{
  font-size:0.8125rem;color:var(--error);margin-top:6px;
  padding:6px 8px;border-radius:4px;
  background:rgba(248,81,73,0.1);
  white-space:nowrap;
  overflow:hidden;
  text-overflow:ellipsis;
}
```

### 3. Fix visual layout shift on active card transition

**File:** `firmware/data-src/style.css`
**Issue:** Missing padding and border width overrides on the switching state causes layout jitter.

**Corrected code:**
```css
.mode-card.switching{
  opacity:0.7;
  border-color:var(--primary);border-left-color:var(--primary);
  animation:mode-pulse 1.2s ease-in-out infinite;
}
.mode-card.switching[aria-current="true"],
.mode-card.switching.active{
  border-left-width: 4px;
  padding-left: 9px;
}
```

**Review Actions:**
- Issues Found: 3
- Issues Fixed: 0
- Action Items Created: 0

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-14</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">ds-3</var>
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
<var name="session_id">47d62ce6-a8d3-42ee-9ede-d32dba44a5e1</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="dcd69c58">embedded in prompt, file id: dcd69c58</var>
<var name="story_id">ds-3.4</var>
<var name="story_key">ds-3-4-mode-switching-flow-and-transition-states</var>
<var name="story_num">4</var>
<var name="story_title">4-mode-switching-flow-and-transition-states</var>
<var name="template">False</var>
<var name="timestamp">20260414_2109</var>
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
      - Commit message format: fix(component): brief description (synthesis-ds-3.4)
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