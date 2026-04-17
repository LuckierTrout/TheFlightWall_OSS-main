<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-3 -->
<!-- Story: 5 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260415T012944Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story ds-3.5

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
<file id="2f47ff42" path="_bmad-output/implementation-artifacts/stories/ds-3-5-accessibility-and-responsive-layout.md" label="DOCUMENTATION"><![CDATA[

# Story ds-3.5: Accessibility & Responsive Layout

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user with accessibility needs or using a phone,
I want the Mode Picker to be fully keyboard-navigable, screen-reader friendly, and responsive across devices,
So that I can control display modes regardless of how I access the dashboard.

## Acceptance Criteria

1. **Given** mode cards are rendered in **idle** state (not active, not switching, not disabled), **When** the user navigates by **keyboard**, **Then** each idle card exposes **`role="button"`** and **`tabindex="0"`** (**UX-DR7**). The **active** card has **`aria-current="true"`** (already from **ds-3.3** if implemented) and **must not** be a tab stop for activation: use **`tabindex="-1"`** on the active card **or** omit **`tabindex`** and rely on **`aria-current`** only — **pick one** pattern and document; epic requires active card **not** in tab order as an **actionable** element (no Enter-to-“switch” on already-active mode).

2. **Given** focus is on an **idle** mode card, **When** the user presses **Enter** or **Space**, **Then** **`switchMode(modeId)`** runs (same path as click) (**UX-DR7**). **`preventDefault`** on **Space** to avoid page scroll.

3. **Given** a **mode switch completes** (success path from **ds-3.4**), **When** focus management runs, **Then** focus moves from the **newly active** card to the **previously active** card (now idle, **`tabindex="0"`**) (**UX-DR6**). If **ds-3.4** already moves focus, **reconcile** so there is **one** authoritative focus step (avoid double **`focus()`** calls).

4. **Given** the **upgrade notification banner** (**ds-3.6**) is dismissed or removed, **When** focus management runs, **Then** focus moves to the **Mode Picker section heading** (**UX-DR6**). **Prerequisite:** add **`id="mode-picker-heading"`** (or equivalent) on the **`#mode-picker`** **`h2`** in **`dashboard.html`** so **ds-3.6** and this story share a stable target; heading should have **`tabindex="-1"`** for programmatic focus only.

5. **Given** a card is in **switching** state (**ds-3.4**), **When** assistive tech queries it, **Then** that card has **`aria-busy="true"`** until the switch finalizes (**UX-DR8**). Remove **`aria-busy`** when returning to idle/active.

6. **Given** a card is in **disabled** state (siblings during switch, **ds-3.4**), **When** assistive tech queries it, **Then** that card has **`aria-disabled="true"`** (**UX-DR8**). **Note:** **`aria-disabled="true"`** does not remove the element from the tab order in all browsers — prefer **`tabindex="-1"`** on disabled cards **in addition** to **`aria-disabled`** so keyboard users cannot land on unusable targets during the switch.

7. **Given** viewport **width &lt; 600px**, **When** the Mode Picker renders, **Then** **`.mode-cards-list`** is a **single column**, full width, with comfortable touch spacing (min **44px** vertical tap height per card row is desirable) (**UX-DR9** phone-first).

8. **Given** viewport **width ≥ 1024px**, **When** the Mode Picker renders, **Then** **`.mode-cards-list`** uses a **two-column CSS Grid** (e.g. **`grid-template-columns: repeat(2, minmax(0, 1fr))`**) with gap consistent with **`var(--gap)`** / **8px** (**UX-DR9** desktop).

9. **Given** the dashboard already uses responsive rules elsewhere, **When** adding **1024px** rules, **Then** they are **additive**: do **not** remove or break any existing **`@media (max-width: 600px)`** (or equivalent) patterns used by **`.dashboard`** / **`.dash-cards`** if present; if **no** **600px** breakpoint exists today, introduce **`max-width: 599px`** for the single-column mode list **and** document that **600px** behavior is preserved for future shared breakpoints.

10. **Given** **`dashboard.html`**, **`dashboard.js`**, **`style.css`** change, **When** shipping, **Then** regenerate **`firmware/data/*.gz`** per **`AGENTS.md`**.

11. **Given** manual verification, **When** testing with **VoiceOver** (macOS) or **NVDA** (Windows) on **`dashboard.html`**, **Then** mode names and active state are announced sensibly; switching/disabled states do not trap focus.

## Tasks / Subtasks

- [x] Task 1: HTML — heading id (**AC: #4**)
  - [x] 1.1: **`#mode-picker` `h2`**: add **`id="mode-picker-heading"`**, **`tabindex="-1"`**

- [x] Task 2: **`renderModeCards`** — ARIA + tab order (**AC: #1, #2, #5, #6**)
  - [x] 2.1: Set **`role`**, **`tabindex`**, **`aria-current`**, **`aria-busy`**, **`aria-disabled`** on card root
  - [x] 2.2: **`keydown`** listener on **`#modeCardsList`** (delegate) for **Enter**/**Space**

- [x] Task 3: Focus integration (**AC: #3, #4**)
  - [x] 3.1: Export or call a shared **`focusModePickerHeading()`** for **ds-3.6**
  - [x] 3.2: Align **`switchMode`** completion focus with **ds-3.4** (single owner)

- [x] Task 4: CSS — responsive grid (**AC: #7–#9**)
  - [x] 4.1: **`@media (max-width: 599px)`** — single column **`.mode-cards-list`**
  - [x] 4.2: **`@media (min-width: 1024px)`** — two-column grid

- [x] Task 5: Gzip assets (**AC: #10**)

## Dev Notes

### Current baseline (verify in tree)

- **`dashboard.html`**: **`#mode-picker`**, nav link **`#mode-picker`**, **`h2`** text **"Display Mode"** — may lack **`id`** / **`tabindex`** on heading.
- **`style.css`**: **`.mode-cards-list`** is **`display:flex; flex-direction:column`** — no **1024px** two-column rule yet; no dedicated **599px** block for mode list.
- **`dashboard.js`**: **`renderModeCards`** builds **`div.mode-card`** with **click** handler; likely **no** **`role="button"`** / **`keydown`** yet.

### Coordination

- **ds-3.4** — switching / disabled / focus after switch; this story **must not** regress polling or state classes.
- **ds-3.6** — banner dismiss → focus heading; implement **heading hook** here first.

### Out of scope

- New **design system** beyond epic tokens.
- **health.html** accessibility pass.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.5)
- Prior: `_bmad-output/implementation-artifacts/stories/ds-3-4-mode-switching-flow-and-transition-states.md`, `ds-3-3-mode-picker-section-cards-and-schematic-previews.md`
- Sources: `firmware/data-src/dashboard.html`, `dashboard.js`, `style.css`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Firmware build: SUCCESS (79.2% flash usage)
- Unit tests: ERRORED (expected — ESP32-only tests require hardware upload, not available in dev environment)

### Completion Notes List

- **Task 1:** Added `id="mode-picker-heading"` and `tabindex="-1"` to the `#mode-picker` `h2` in `dashboard.html` for programmatic focus target (AC #4).
- **Task 2:** Updated `renderModeCards` to set `role="button"` and `tabindex="0"` on idle cards, `tabindex="-1"` on active card. Added delegated `keydown` listener on `#modeCardsList` for Enter/Space with `preventDefault` on Space. Updated `switchMode` to set `aria-busy="true"` on switching card and `aria-disabled="true"` + `tabindex="-1"` on disabled cards. Updated `clearSwitchingStates` and `updateModeStatus` to properly remove ARIA attributes and restore correct tabindex values (AC #1, #2, #5, #6).
- **Task 3:** Created `focusModePickerHeading()` function exposed on `window` for ds-3.6 banner dismiss focus. Verified `finalizeModeSwitch` already correctly handles post-switch focus by moving to previously-active (now idle) card — single authoritative focus step (AC #3, #4).
- **Task 4:** Added `@media (max-width: 599px)` for single-column `.mode-cards-list` with `min-height: 44px` on cards for touch targets. Added `@media (min-width: 1024px)` for two-column CSS Grid. No existing 600px breakpoints were modified — changes are purely additive (AC #7, #8, #9).
- **Task 5:** Regenerated `firmware/data/dashboard.html.gz`, `dashboard.js.gz`, and `style.css.gz` (AC #10).
- **Pattern choice documented (AC #1):** Active card uses `tabindex="-1"` to exclude from tab order while remaining programmatically focusable for post-switch focus management.

### Change Log

- 2026-04-14: Implemented ds-3.5 accessibility and responsive layout — ARIA attributes, keyboard navigation, focus management, responsive CSS grid, gzipped assets.

### File List

- `firmware/data-src/dashboard.html` (modified)
- `firmware/data-src/dashboard.js` (modified)
- `firmware/data-src/style.css` (modified)
- `firmware/data/dashboard.html.gz` (regenerated)
- `firmware/data/dashboard.js.gz` (regenerated)
- `firmware/data/style.css.gz` (regenerated)

## Previous story intelligence

- **ds-3.4** owns **switching** / **disabled** visuals and **POST**/**GET** polling; **ds-3.5** layers **ARIA**, **tab order**, and **responsive** grid on the same DOM.

## Git intelligence summary

Expect changes under **`firmware/data-src/`** and **`firmware/data/*.gz`**.

## Project context reference

`_bmad-output/project-context.md` — gzip bundled assets after edits.

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

## Story ds-3-4 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `finalizeModeSwitch` sets `modeSwitchInFlight = false` at function entry (line 3083), **before** the async `loadDisplayModes()` resolves in the `!skipReload` branch. The prior Pass 1 review fixed this for all 3 error paths, but the success path (`finalizeModeSwitch` without `skipReload`) was missed. A second `switchMode()` call could fire while the DOM rebuild is still in-flight, causing the first `loadDisplayModes()` response to overwrite the new switching/disabled visual state. | Moved `modeSwitchInFlight = false` out of the synchronous function body and into the `.then()` / rejection callbacks for the `!skipReload` branch. The `skipReload=true` path (poll success — DOM already rebuilt) retains immediate unlock. |
| low | `.mode-card-alert` has no CSS text truncation. A verbose API error (e.g., raw `res.body.error` from a future verbose validation failure) could wrap and expand card height, breaking the grid layout. AC #4 explicitly says "truncated inline on the card." | Added `overflow:hidden; text-overflow:ellipsis; white-space:nowrap` to `.mode-card-alert`. Firmware `_lastError` is bounded to 64 chars; at 0.8125rem all messages fit in one line in a standard card width. |
| low | Visual layout jitter when the currently-active card is clicked for re-selection: the card loses `.active` (4px left border, 9px padding-left) and gains `.switching` (1px border, 12px padding) — causing a 3px border and 3px padding snap before the pulse animation starts. | Added `.mode-card.switching[aria-current="true"] { border-left-width:4px; padding-left:9px }`. Crucially, `aria-current="true"` is **not** removed when `.active` is stripped (confirmed from grep — only `classList.remove('active')` is called). The attribute persists through the switching state, making this selector correctly target only previously-active cards. Non-active cards switching (the common case) have no `aria-current` and are unaffected. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-3-5-accessibility-and-responsive-layout.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **5** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Missing Focus Restoration on Switch Failure | `dashboard.js` | +1.0 |
| 🟠 IMPORTANT | Missing `aria-live` on Status Line | `dashboard.html` | +1.0 |
| 🟠 IMPORTANT | AC 9 Missing Documentation Comment | `style.css` | +1.0 |
| 🟡 MINOR | Hardcoded Subtitle Cleanup / State Loss | `dashboard.js` | +0.3 |
| 🟡 MINOR | Non-interactive Element Focus Ring | `style.css` | +0.3 |
| 🟢 CLEAN PASS | SOLID | | -0.5 |
| 🟢 CLEAN PASS | Abstraction | | -0.5 |
| 🟢 CLEAN PASS | Lying Tests | | -0.5 |
| 🟢 CLEAN PASS | Performance | | -0.5 |
| 🟢 CLEAN PASS | Type Safety | | -0.5 |
| 🟢 CLEAN PASS | Security | | -0.5 |

### Evidence Score: 0.6

| Score | Verdict |
|-------|---------|
| **0.6** | **APPROVED** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

- **Style Violation:** Non-interactive Element Focus Ring
  - 📍 `firmware/data-src/style.css`
  - 💡 Fix: Add `#mode-picker-heading:focus-visible { outline: none; }` to prevent jarring focus rings when the heading receives programmatic focus after the upgrade banner dismissal (AC 4).

✅ Code follows general style guidelines and is readable.

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** Missing Focus Restoration on Switch Failure
  - 📍 `firmware/data-src/dashboard.js:1000`
  - 🔄 Reproduction: Focus a mode card via keyboard, press Enter to switch, then force a network failure, timeout, or registry heap error. The DOM rebuilds via the asynchronous `loadDisplayModes()` which destroys the active card element. Because `errCard.focus()` is missing in these failure paths, keyboard focus is dropped to `document.body`, forcing the user to tab from the top of the page again.

- **🐛 Bug:** Missing `aria-live` on Status Line
  - 📍 `firmware/data-src/dashboard.html:68`
  - 🔄 Reproduction: Switch modes. The text inside `#modeStatusName` updates dynamically. Without `aria-live="polite"` on the `#modeStatusLine` container, screen readers do not announce the new active state automatically, leaving visually impaired users unaware of the confirmed system state.

✅ Code appears secure and free of XSS/DOM injection risks.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** AC 9 Missing Documentation Comment
  - 📍 `firmware/data-src/style.css:550`
  - 💥 Explosion radius: High risk of future developers altering the `599px` breakpoint. AC 9 strictly required: "introduce max-width: 599px ... and document that 600px behavior is preserved for future shared breakpoints." The CSS implements the logic but completely omits the mandated documentation.

- **💣 Tech Debt:** Hardcoded Subtitle Cleanup / State Loss
  - 📍 `firmware/data-src/dashboard.js:460`
  - 💥 Explosion radius: `clearSwitchingStates()` hardcodes `if (sub.textContent === 'Switching...') sub.textContent = '';`. If the mode previously had a `state_reason` (e.g., "loading"), and the switch fails while the device is offline (so the API reload also fails), the original state reason is permanently wiped instead of restored.

---

## 🛠️ Suggested Fixes

### 1. Fix Focus Restoration on Error Paths

**File:** `firmware/data-src/dashboard.js`
**Issue:** Restore focus to the card after an async DOM rebuild on switch failure to prevent context drop.

**Diff:**
```diff
       if (!res.body || !res.body.ok) {
         clearSwitchingStates();
         var errMsg = (res.body && res.body.error) ? res.body.error : 'Mode switch failed';
         // Keep modeSwitchInFlight=true until reload completes
         loadDisplayModes().then(function() {
           modeSwitchInFlight = false;
+          var errCard = getModeCard(modeId);
-          showCardError(getModeCard(modeId), errMsg);
+          showCardError(errCard, errMsg);
+          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
         }, function() {
           modeSwitchInFlight = false;
+          var errCard = getModeCard(modeId);
-          showCardError(getModeCard(modeId), errMsg);
+          showCardError(errCard, errMsg);
+          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
         });
         return;
       }
```

### 2. Add `aria-live` to Status Line

**File:** `firmware/data-src/dashboard.html`
**Issue:** Screen readers do not announce dynamic mode status changes automatically.

**Diff:**
```diff
     <!-- Mode Picker (Story ds-3.3) -->
     <section class="card" id="mode-picker">
       <h2 id="mode-picker-heading" tabindex="-1">Display Mode</h2>
-      <div class="mode-status-line" id="modeStatusLine">
+      <div class="mode-status-line" id="modeStatusLine" aria-live="polite">
         Active: <span class="mode-status-name" id="modeStatusName">&mdash;</span>
         <span class="mode-status-reason" id="modeStatusReason">loading</span>
       </div>
```

### 3. Add Mandated AC 9 Documentation

**File:** `firmware/data-src/style.css`
**Issue:** Missing required documentation for responsive breakpoint isolation.

**Diff:**
```diff
 /* Mode Picker responsive layout (ds-3.5) */
+/* Preserves 600px boundary for future shared dashboard breakpoints per AC 9 */
 @media (max-width:599px){
   .mode-cards-list{display:flex;flex-direction:column;gap:8px}
   .mode-card{min-height:44px}
 }
```

**Review Actions:**
- Issues Found: 5
- Issues Fixed: 0
- Action Items Created: 0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

I'm ready to help. What would you like me to work on?

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
<var name="session_id">f118ffc5-08ec-4019-99d5-a74176d41374</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="2f47ff42">embedded in prompt, file id: 2f47ff42</var>
<var name="story_id">ds-3.5</var>
<var name="story_key">ds-3-5-accessibility-and-responsive-layout</var>
<var name="story_num">5</var>
<var name="story_title">5-accessibility-and-responsive-layout</var>
<var name="template">False</var>
<var name="timestamp">20260414_2129</var>
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
      - Commit message format: fix(component): brief description (synthesis-ds-3.5)
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