<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-3 -->
<!-- Story: 3 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260415T002600Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story ds-3.3

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
<file id="25219341" path="_bmad-output/implementation-artifacts/stories/ds-3-3-mode-picker-section-cards-and-schematic-previews.md" label="DOCUMENTATION"><![CDATA[

# Story ds-3.3: Mode Picker Section — Cards & Schematic Previews

Status: Ready for Review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user,
I want to see all available display modes in the web dashboard with schematic previews of each mode's zone layout,
So that I can understand what each mode looks like before switching.

## Acceptance Criteria

1. **Given** the dashboard loads and the Mode Picker section exists, **When** the page initializes, **Then** JavaScript calls **`GET /api/display/modes`**, reads **`res.body.data`** (envelope **`{ ok, data }`** — not top-level **`modes`**), and builds mode cards from **`data.modes`** dynamically (**UX-DR12**).

2. **Given** the epic placeholder contract, **When** the Mode Picker markup is finalized, **Then** the section includes a root element **`id="mode-picker"`** (may be the existing **`#mode-picker-card`** renamed, or an inner wrapper) with **minimal static HTML** (~10 lines): heading, optional status host, and an **empty** container for injected cards (e.g. **`#modeCardsList`**). **Do not** hardcode mode names in HTML.

3. **Given** each mode object from the API includes **`zone_layout`** (array of **`{ label, relX, relY, relW, relH }`** — **`uint8` 0–100** semantics per **`ZoneRegion`** in **`DisplayMode.h`**), **When** a card is rendered, **Then** it shows **mode name** and a **CSS Grid** schematic:
   - Container **height 80px**, **`display: grid`**, **`grid-template-columns: repeat(100, minmax(0, 1fr))`**, **`grid-template-rows: repeat(100, minmax(0, 1fr))`** (or equivalent **100×100** logical grid so integers map 1:1).
   - Each zone is a **child `div`** with **`grid-column`** / **`grid-row`** derived from **`relX, relY, relW, relH`** using **1-based** grid lines: e.g. **`grid-column: (relX + 1) / span relW`** only if **`relW > 0`**; clamp spans so lines stay within **1..101**. Prefer **`grid-column-start` / `grid-column-end`** if clearer for your layout math.
   - Zone cells show **visible label text** (abbreviate long **`label`** strings for the 80px-tall mini preview if needed).
   - Zone cells: **`aria-hidden="true"`** (decorative preview; **UX-DR8** partial).
   - Schematic wrapper: **`aria-label`** describing the layout (e.g. **"Schematic preview of zone layout for {mode name}"**).

4. **Given** **`data.active`** matches a mode **`id`**, **When** cards render, **Then** the active card has:
   - **`aria-current="true"`** (**UX-DR7**),
   - **4px** solid **left** accent border using **`var(--primary)`** (distinct from idle),
   - a **status dot** (small circular indicator) and visible **"Active"** label (**UX-DR1** active).

5. **Given** a non-active mode, **When** its card renders, **Then** it uses **1px** border with a **dimmed** accent (**UX-DR1** idle): add **`--accent-dim`** (or reuse **`var(--border)`** with a documented token) in **`:root`** if no suitable token exists; card remains **`cursor: pointer`** / tappable.

6. **Given** **UX-DR4**, **When** the user selects a mode, **Then** the **entire card** is the interaction surface — **no** inner **`<button>`** for “Switch”. (Click handler may remain on the card **`div`**; keyboard **`role="tabindex"`** is **ds-3.5** unless you add it early without conflicting with **ds-3.4**.)

7. **Given** **FR24**, **When** the user views the dashboard header navigation, **Then** a link to the Mode Picker is **always** available (e.g. **`href="#mode-picker"`** next to System Health) so the section is discoverable without scrolling the full card stack.

8. **Given** **NFR C4**, **When** styling the picker, **Then** reuse **dark theme** and existing **CSS custom properties** from **`style.css`** (`--bg`, `--surface`, `--border`, `--text`, `--primary`, …).

9. **Given** **NFR P3** (Mode Picker “loads” within **~1s** of page load), **When** measuring in dev tools, **Then** first successful paint of mode cards (including schematic) occurs **≤ 1000ms** after **`DOMContentLoaded`** on a warm LAN connection to the device. If **`loadSettings()`** contends for the ESP32 HTTP stack, **start `loadDisplayModes()` in parallel** with settings (already parallel today — **preserve or improve**, do not serialize behind a slow path).

10. **Given** **`POST /api/display/mode`** (**ds-3.1**), **When** this story touches the switch request, **Then** prefer body **`{ "mode": "<id>" }`**; keep **`mode_id`** only if needed for backward compatibility during transition (**firmware accepts both**).

11. **Given** bundled portal workflow, **When** **`dashboard.html`**, **`dashboard.js`**, or **`style.css`** under **`firmware/data-src/`** change, **Then** regenerate the matching **`.gz`** under **`firmware/data/`** per **`AGENTS.md`**.

12. **Given** **`pio run`** (or repo CI equivalent), **Then** no new build warnings from touched artifacts.

## Tasks / Subtasks

- [x] Task 1: Markup — **`id="mode-picker"`** + nav (AC: #2, #7)
  - [x] 1.1: Add **`#mode-picker`** to **`dashboard.html`**; keep **`#modeCardsList`** as injection target
  - [x] 1.2: Extend **`<nav>`** in **`dash-header`** with **"Display Modes"** → **`#mode-picker`**

- [x] Task 2: Schematic renderer (AC: #1, #3, #9)
  - [x] 2.1: In **`renderModeCards`**, after name row, append schematic **`div`** built from **`mode.zone_layout`**
  - [x] 2.2: Handle **missing** or **empty** **`zone_layout`** gracefully (empty schematic + **`aria-label`** still valid)

- [x] Task 3: Card chrome — active / idle (AC: #4, #5, #6)
  - [x] 3.1: Update **`style.css`** — **`.mode-card.active`**, **`.mode-card`**, dot + **"Active"** row
  - [x] 3.2: Set **`aria-current`** only on active card; remove from others on re-render

- [x] Task 4: POST body + gzip (AC: #10, #11, #12)
  - [x] 4.1: **`switchMode`**: send **`mode`** key
  - [x] 4.2: Regenerate **`data/dashboard.js.gz`**, **`data/dashboard.html.gz`**, **`data/style.css.gz`** as applicable

## Dev Notes

### Current implementation (delta from epic)

- **`dashboard.html`**: section **`#mode-picker-card`** with **`#modeCardsList`** — **no** schematic, **no** **`#mode-picker`** id, **no** header nav link to modes.
- **`dashboard.js`**: **`renderModeCards`**, **`loadDisplayModes`**, **`switchMode`** — basic list; **`switchMode`** uses **`mode_id`** only; switching animation is **partial** (full **UX-DR1** switching/disabled/error is **ds-3.4**).
- **`style.css`**: **`.mode-card`** / **`.active`** — uniform border, **no** 4px left rail, **no** status dot, **no** schematic styles.

### API contract (do not guess)

- Success: **`res.body.ok === true`**, payload in **`res.body.data`**
- Fields: **`data.modes[]`**, **`data.active`**, optional **`data.description`** on mode objects from firmware if populated
- Zone keys: **`relX`, `relY`, `relW`, `relH`** are **percentage 0–100** of logical display [Source: `firmware/interfaces/DisplayMode.h`]

### Out of scope (later stories)

- **ds-3.4** — pulsing border, sibling **disabled** state, scoped card **error** copy, **`prefers-reduced-motion`**, focus choreography after switch
- **ds-3.5** — **`role="button"`**, **`tabindex`**, full keyboard model
- **ds-3.6** — upgrade **banner** (may scroll to **`#mode-picker`** — stable id matters)

### Dependencies

- **ds-3.1** — **`GET /api/display/modes`** with **`zone_layout`** from **`ModeRegistry`**
- **dl-1.5** — status line **`orchestrator_state` / `state_reason`**; keep **`updateModeStatus`** working after DOM changes

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.3)
- Prior API story: `_bmad-output/implementation-artifacts/stories/ds-3-1-display-mode-api-endpoints.md`
- Prior persistence story: `_bmad-output/implementation-artifacts/stories/ds-3-2-nvs-mode-persistence-and-boot-restore.md`
- Sources: `firmware/data-src/dashboard.html`, `dashboard.js`, `style.css`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, 0 warnings, Flash 78.8%, RAM 16.7%
- Test build: `pio test --without-uploading --without-testing` — all 9 test suites compile OK (no device for runtime)

### Completion Notes List

- **Task 1:** Renamed `#mode-picker-card` → `#mode-picker` section id. Added "Display Modes" nav link in `dash-header` alongside "System Health".
- **Task 2:** Added `buildSchematic()` function that creates a 100×100 CSS Grid from `zone_layout` array. Labels are truncated to 7 chars + ellipsis for the 80px preview. Missing/empty `zone_layout` returns empty wrapper with valid `aria-label`. Zone cells have `aria-hidden="true"`.
- **Task 3:** Active card now has 4px solid left border (`var(--primary)`), green status dot + "Active" text, `aria-current="true"`. Idle cards use `var(--border)` for left border (dimmed). `updateModeStatus` manages `aria-current` on status polling. Card header restructured with flex row for name + active indicator.
- **Task 4:** `switchMode` POST body changed from `{ mode_id }` to `{ mode }`. All 3 gzip files regenerated.

### Change Log

- 2026-04-14: Implemented story ds-3.3 — mode picker cards with schematic previews, active/idle chrome, nav link, POST body update

### File List

- `firmware/data-src/dashboard.html` (modified)
- `firmware/data-src/dashboard.js` (modified)
- `firmware/data-src/style.css` (modified)
- `firmware/data/dashboard.html.gz` (regenerated)
- `firmware/data/dashboard.js.gz` (regenerated)
- `firmware/data/style.css.gz` (regenerated)

## Previous story intelligence

- **ds-3.2** standardizes **`disp_mode`** NVS and boot restore; picker is **read-only** of **`data.active`** except where **ds-3.4** extends **`POST`** UX.
- **ds-3.1** documents **`data.*`** envelope and **`zone_layout`** shape — UI **must** consume that shape.

## Git intelligence summary

Expect diffs under **`firmware/data-src/`** and matching **`firmware/data/*.gz`**.

## Project context reference

`_bmad-output/project-context.md` — regenerate **gzip** after **`data-src`** edits.

## Definition of Done Validation

- [x] All acceptance criteria verified against source code (12/12 pass)
- [x] All tasks and subtasks marked complete (4/4 tasks, all subtasks)
- [x] `pio run` builds with 0 warnings (Flash 80.6%, RAM 17.1%)
- [x] Gzip assets regenerated and current (Apr 14 2026)
- [x] No new dependencies introduced
- [x] ES5 syntax maintained throughout (no arrow functions, no let/const, no template literals)
- [x] CSS uses existing dark-theme custom properties (no hardcoded colors)
- [x] ARIA attributes applied per UX-DR requirements
- [x] Parallel loading preserved (loadDisplayModes not serialized behind loadSettings)

### Validation Date
2026-04-14

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.


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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-3-3-mode-picker-section-cards-and-schematic-previews.md

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
| 🔴 CRITICAL | Massive Scope Violation / Tech Debt Bomb (Implemented out-of-scope stories ds-3.4, ds-3.5, ds-3.6, dl-5.1) | `dashboard.js` | +3.0 |
| 🔴 CRITICAL | Orchestrator Bypass Blocked (Same-mode active click prevented) | `dashboard.js` | +3.0 |
| 🟠 IMPORTANT | Focus Choreography focuses wrong card (previous instead of new) | `dashboard.js` | +1.0 |
| 🟠 IMPORTANT | AC5 Violation (4px idle border instead of 1px) | `style.css` | +1.0 |
| 🟠 IMPORTANT | SRP Violation (God Method `switchMode`) | `dashboard.js` | +1.0 |
| 🟡 MINOR | Redundant DOM operations in `updateModeStatus` | `dashboard.js` | +0.3 |
| 🟢 CLEAN PASS | Style Guidelines (ES5 syntax preserved) | | -0.5 |
| 🟢 CLEAN PASS | Type Safety (Standard DOM methods) | | -0.5 |
| 🟢 CLEAN PASS | Security / XSS (Safe `textContent` usage) | | -0.5 |

### Evidence Score: 7.8

| Score | Verdict |
|-------|---------|
| **7.8** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] Single Responsibility Principle (SRP):** `switchMode` is a 120-line God Method that manages DOM class manipulation, ARIA state, HTTP POSTs, interval polling, scoped error rendering, and focus choreography.
  - 📍 `firmware/data-src/dashboard.js:1427`
  - 💡 Fix: Extract network polling into `pollSwitchState()`, DOM manipulation into `setCardState()`, and error handling into separate functions.

✅ No other significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable (Legacy ES5 enforced without template literals/arrows).

---

## ⚡ Performance & Scalability

- **[Low] Redundant DOM Operations:** `updateModeStatus` manually iterates over all mode cards to add `.active` and `aria-current` immediately after `renderModeCards` completely destroys the DOM, rebuilds it, and sets those exact same attributes.
  - 📍 `firmware/data-src/dashboard.js:1255`
  - 💡 Fix: `renderModeCards` already sets the correct initial state. `updateModeStatus` is only needed during polling or background updates. Avoid running it redundantly inside `loadDisplayModes`.

---

## 🐛 Correctness & Safety

- **🐛 Bug (Orchestrator Bypass Blocked):** The UI blocks `POST /api/display/mode` if the clicked card is currently active. If the `ModeOrchestrator` is in `IDLE_FALLBACK` or `SCHEDULED` state, the user MUST be able to click the active mode to send the manual switch event and lock it into `MANUAL` state. The UI returning early effectively breaks the `ModeOrchestrator` state machine—a bug explicitly warned against in the `ds-3.1` ANTIPATTERNS file!
  - 📍 `firmware/data-src/dashboard.js:1435`
  - 🔄 Reproduction: Wait for Idle Fallback to activate Clock Mode -> Click "Clock Mode" to lock it manually -> Nothing happens, request is blocked by JS.

- **🐛 Bug (Focus Choreography Logic Error):** The out-of-scope focus choreography feature attempts to focus the `previousActiveId` card after a successful switch, instead of the newly activated card.
  - 📍 `firmware/data-src/dashboard.js:1546`
  - 🔄 Reproduction: Switch from Classic Card to Clock Mode -> Keyboard focus is bizarrely thrown back to Classic Card.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt (Massive Scope Violation):** The developer agent hallucinated and implemented features from `ds-3.4`, `ds-3.5`, `ds-3.6`, and `dl-5.1` (Delight release!). The code even contains comments like `// Per-mode settings panel (Story dl-5.1, AC #3)` and `// Upgrade Notification Banner (Story ds-3.6)`. This completely destroys story boundaries, bloats the PR with thousands of lines of unreviewed code, and guarantees merge conflicts for future stories.
  - 📍 `firmware/data-src/dashboard.js:1385`
  - 💥 Explosion radius: Severe project management regression; invalidates completion checks for 4 subsequent stories.

- **💣 Tech Debt (AC5 Gap):** Idle cards are rendered with a 4px left border (`border-left: 4px solid var(--border)`), violating AC5 which strictly requires a 1px border with a dimmed accent. This makes the UI look visually heavy and reduces contrast between active/idle states.
  - 📍 `firmware/data-src/style.css:482`
  - 💥 Explosion radius: High visual regression that breaks UX design specs.

---

## 🛠️ Suggested Fixes

### 1. Fix Orchestrator Bypass Blocker

**File:** `firmware/data-src/dashboard.js`
**Issue:** Remove the early return that prevents sending a manual switch event for the currently active mode.

**Diff:**
```diff
-      // Don't switch to the already-active mode
-      if (cards[i].getAttribute('data-mode-id') === modeId && cards[i].classList.contains('active')) {
-        modeSwitchInFlight = false;
-        return;
-      }
```

### 2. Fix AC5 Idle Card Border

**File:** `firmware/data-src/style.css`
**Issue:** Use 1px border for idle cards.

**Diff:**
```diff
 .mode-card{
   padding:12px;background:var(--bg);
-  border:1px solid var(--border);border-left:4px solid var(--border);
+  border:1px solid var(--border);
   border-radius:var(--radius);
   cursor:pointer;transition:border-color 0.15s;
 }
```

### 3. Remove Out-of-Scope Code

**File:** `firmware/data-src/dashboard.js`
**Issue:** Remove or revert out-of-scope implementations for `ds-3.4` (polling, focus choreography), `ds-3.5` (keyboard roles), `ds-3.6` (upgrade banners), and `dl-5.1` (mode settings panels) to maintain strict story boundaries.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

I'm ready to help! What would you like me to do?

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
<var name="session_id">823b1e53-28ab-4c45-a83c-38feae7972c7</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="25219341">embedded in prompt, file id: 25219341</var>
<var name="story_id">ds-3.3</var>
<var name="story_key">ds-3-3-mode-picker-section-cards-and-schematic-previews</var>
<var name="story_num">3</var>
<var name="story_title">3-mode-picker-section-cards-and-schematic-previews</var>
<var name="template">False</var>
<var name="timestamp">20260414_2026</var>
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
      - Commit message format: fix(component): brief description (synthesis-ds-3.3)
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