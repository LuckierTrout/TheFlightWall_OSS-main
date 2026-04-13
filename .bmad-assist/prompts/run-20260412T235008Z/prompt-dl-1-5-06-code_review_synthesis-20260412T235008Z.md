<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-1 -->
<!-- Story: 5 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260412T235008Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-1.5

You are synthesizing 3 independent code review findings.

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
<file id="af701597" path="_bmad-output/implementation-artifacts/stories/dl-1-5-story-5.md" label="DOCUMENTATION"><![CDATA[

# Story dl-1.5: Orchestrator State Feedback in Dashboard

Status: Ready for Review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want the dashboard Mode Picker to display the current orchestrator state and reason (e.g., "Active: Clock (idle fallback — no flights)"),
So that I can always understand whether the wall is following my manual choice or responding automatically to flight availability.

## Context & Rationale

Stories dl-1.1 through dl-1.4 build the orchestrator firmware (ClockMode, ModeOrchestrator, idle fallback, watchdog recovery). But without dashboard feedback, the owner has no way to know **why** the wall is showing a particular mode. The UX spec's defining experience principle states: "Delegation requires transparency — the status line is the emotional contract between automation and control." This story closes the loop by surfacing orchestrator state through the API and Mode Picker UI.

**Dependencies:** This story MUST be implemented after dl-1.1 (ClockMode + DisplayMode interface), dl-1.2 (ModeOrchestrator), and dl-1.3 (manual switching via orchestrator). It assumes ds-3-1 (Display mode API endpoints) and ds-3-3 (Mode Picker UI) are already shipped.

**What this story does NOT cover:**
- Scheduled mode state (`SCHEDULED` state, `scheduled_mode`, `next_change`) — that's dl-4
- Per-mode settings panels — that's dl-5
- "Resume Schedule" button — that's dl-4
- Transition animations — that's dl-3

## Acceptance Criteria

1. **Given** the Mode Picker section is visible on the dashboard
   **When** the dashboard loads or a mode switch completes
   **Then** a status line at the top of the Mode Picker shows: "Active: {ModeName} ({reason})" where reason reflects the current orchestrator state

2. **Given** the owner manually selected Live Flight Card via Mode Picker
   **When** the status line renders
   **Then** it shows: "Active: Live Flight Card (manual)"

3. **Given** zero flights are in range and idle fallback activated Clock Mode
   **When** the status line renders
   **Then** it shows: "Active: Clock (idle fallback — no flights)"

4. **Given** the owner manually selects a mode while in IDLE_FALLBACK state
   **When** the API responds and the dashboard updates
   **Then** the status line changes to "Active: {SelectedMode} (manual)" reflecting the orchestrator's transition from IDLE_FALLBACK to MANUAL state

5. **Given** the existing `GET /api/display/modes` endpoint
   **When** this story is implemented
   **Then** the response JSON includes two new fields: `orchestrator_state` (string: "manual" | "idle_fallback") and `state_reason` (string: human-readable reason for the current mode)

6. **Given** the Mode Picker UI exists from ds-3-3
   **When** this story is implemented
   **Then** a persistent status line element is added at the top of the Mode Picker card, styled with mode name in `font-weight: 600` and reason in `font-weight: 400` within parentheses, using existing CSS custom properties

7. **Given** the dashboard fetches mode state on load
   **When** the API returns orchestrator state
   **Then** the Mode Picker also updates the active mode card's subtitle text to show the `state_reason` string from the API (e.g., "manual" or "idle fallback — no flights"), providing a secondary visual confirmation

8. **Given** a mode switch is triggered (via Mode Picker tap or automatic idle fallback)
   **When** the dashboard next fetches mode state (after the POST response or on next page load)
   **Then** both the status line and the active mode card subtitle reflect the new orchestrator state within the same render cycle (no partial updates)

## Tasks / Subtasks

- [x] Task 1: Extend `GET /api/display/modes` response with orchestrator state (AC: #5)
  - [x] 1.1 Confirm `getState()` exists on `ModeOrchestrator` per `architecture.md#DL2`; implement `getStateReason()` as a new static public method returning the human-readable reason string for the current state (mapping defined in Dev Notes "Orchestrator State Enum & Reason Mapping")
  - [x] 1.2 In `WebPortal::_handleGetModes()` (or equivalent handler), read `ModeOrchestrator::getState()` and serialize as `orchestrator_state` string
  - [x] 1.3 Generate `state_reason` string from orchestrator state: `MANUAL` → "manual", `IDLE_FALLBACK` → "idle fallback — no flights", `SCHEDULED` → "scheduled", any unrecognized state → "unknown"
  - [x] 1.4 Add both fields to the JSON response object alongside existing `active` and `modes[]` fields

- [x] Task 2: Add status line to Mode Picker dashboard UI (AC: #1, #2, #3, #6)
  - [x] 2.1 Add `<div class="mode-status-line">` element at the top of the Mode Picker card in `dashboard.html`
  - [x] 2.2 Style with existing CSS tokens: use `--card-bg` for container, existing text color tokens for content
  - [x] 2.3 Add CSS class `.mode-status-line` with `font-size: 0.9rem`, `padding: 8px 0`, `border-bottom: 1px solid var(--settings-border)`
  - [x] 2.4 Mode name span: `font-weight: 600`. Reason span: `font-weight: 400`, in parentheses

- [x] Task 3: Update dashboard JavaScript to render orchestrator state (AC: #1, #7, #8)
  - [x] 3.1 In the mode fetch handler (after `GET /api/display/modes`), extract `orchestrator_state` and `state_reason` from response
  - [x] 3.2 Update status line text: `Active: ${activeMode.displayName} (${stateReason})`
  - [x] 3.3 Update active mode card subtitle with reason text
  - [x] 3.4 Ensure both UI elements update in the same DOM operation (batch update) to prevent partial render

- [x] Task 4: Update Mode Picker after POST /api/display/mode (AC: #4, #8)
  - [x] 4.1 After successful POST response, re-fetch `GET /api/display/modes` to get updated orchestrator state
  - [x] 4.2 Update status line and active card in the success callback
  - [x] 4.3 Ensure the "Switching..." intermediate state clears before showing new orchestrator state

- [x] Task 5: Gzip updated web assets (AC: all)
  - [x] 5.1 Regenerate `data/dashboard.html.gz` from `data-src/dashboard.html`
  - [x] 5.2 Regenerate `data/dashboard.js.gz` from `data-src/dashboard.js` (or equivalent JS file)
  - [x] 5.3 Regenerate `data/style.css.gz` from `data-src/style.css`
  - [x] 5.4 Verify total gzipped web assets remain well under 50KB budget

## Dev Notes

### Architecture Constraints

- **Response envelope pattern:** All API responses use `{ "ok": bool, "data": {...} }` or `{ "ok": false, "error": "message", "code": "..." }`. The new fields go inside the `data` object. [Source: architecture.md#D4]
- **ModeOrchestrator is static class:** All methods are static. Call `ModeOrchestrator::getState()` directly — no instance needed. [Source: architecture.md#DL2]
- **Rule 24 — requestSwitch() call sites:** `ModeRegistry::requestSwitch()` is called from exactly two methods: `ModeOrchestrator::tick()` and `ModeOrchestrator::onManualSwitch()`. The WebPortal handler must NOT call requestSwitch() directly — always go through the orchestrator. [Source: architecture.md#Enforcement Rules]
- **Rule 30 — cross-core atomics:** Only in `main.cpp`. ModeOrchestrator state is read/written from Core 1 only (orchestrator tick + web handler), so no atomic needed for orchestrator state getters. [Source: architecture.md#Enforcement Rules]
- **Web asset pattern:** Every `fetch()` in dashboard JS must check `json.ok` and call `showToast()` on failure. [Source: architecture.md#Enforcement Rules]
- **No background polling:** Dashboard fetches state on page load and after user actions. No `setInterval` polling for orchestrator state. [Source: UX spec#State Management Patterns]

### Orchestrator State Enum & Reason Mapping

From architecture.md#DL2, `ModeOrchestrator` has three states but only two are relevant for this story:

| OrchestratorState | API String | Reason String | When |
|---|---|---|---|
| `MANUAL` | `"manual"` | `"manual"` | Owner selected this mode |
| `IDLE_FALLBACK` | `"idle_fallback"` | `"idle fallback — no flights"` | Zero flights triggered Clock Mode |
| `SCHEDULED` | `"scheduled"` | `"scheduled"` | Schedule rule activated this mode |
| _(unknown)_ | `"unknown"` | `"unknown"` | Catch-all for any unrecognized future state |

The `SCHEDULED` state and its reason strings are NOT implemented in this story. If `getState()` returns `SCHEDULED` (shouldn't happen before dl-4), serialize as `"scheduled"` with reason `"scheduled"` as a safe fallback. Any unrecognized state value beyond these three must serialize `orchestrator_state` as `"unknown"` and `state_reason` as `"unknown"` — do not leave these fields empty or undefined.

### API Response Shape

Extend the existing `GET /api/display/modes` response:

```json
{
  "ok": true,
  "data": {
    "modes": [
      { "id": "classic_card", "name": "Classic Card", "active": false },
      { "id": "live_flight", "name": "Live Flight Card", "active": false },
      { "id": "clock", "name": "Clock", "active": true }
    ],
    "active": "clock",
    "switch_state": "idle",
    "orchestrator_state": "idle_fallback",
    "state_reason": "idle fallback — no flights"
  }
}
```

### Dashboard HTML Structure

```html
<!-- Inside Mode Picker card, at the top before mode cards list -->
<div class="mode-status-line" id="modeStatusLine">
  Active: <span class="mode-status-name" id="modeStatusName">—</span>
  (<span class="mode-status-reason" id="modeStatusReason">loading</span>)
</div>
```

### CSS Additions (~15 lines)

```css
.mode-status-line {
  font-size: 0.9rem;
  padding: 8px 0;
  margin-bottom: 12px;
  border-bottom: 1px solid var(--settings-border, rgba(255,255,255,0.1));
  color: var(--text-secondary, #aaa);
}
.mode-status-name {
  font-weight: 600;
  color: var(--text-primary, #fff);
}
.mode-status-reason {
  font-weight: 400;
}
```

### JavaScript Update Pattern

```javascript
// After fetching GET /api/display/modes
function updateModeStatus(data) {
  const activeMode = data.modes.find(m => m.id === data.active);
  const statusName = document.getElementById('modeStatusName');
  const statusReason = document.getElementById('modeStatusReason');
  if (statusName && activeMode) {
    statusName.textContent = activeMode.name;
  }
  if (statusReason) {
    statusReason.textContent = data.state_reason || 'unknown';
  }
}
```

### Source Files to Touch

| File | Change |
|---|---|
| `firmware/core/ModeOrchestrator.h` | Confirm `getState()` exists; add `getStateReason()` static public method if not present |
| `firmware/adapters/WebPortal.cpp` | Add orchestrator state fields to mode list API response |
| `firmware/adapters/WebPortal.h` | Possibly add `#include "core/ModeOrchestrator.h"` |
| `firmware/data-src/dashboard.html` | Add status line `<div>` to Mode Picker section |
| `firmware/data-src/style.css` (or equivalent) | Add `.mode-status-line` styles (~15 lines) |
| `firmware/data-src/dashboard.js` (or equivalent) | Add `updateModeStatus()` function, call after mode fetch |
| `firmware/data/*.gz` | Regenerate gzipped assets |

### Testing Approach

**Manual verification (no automated CI):**
1. Boot device with flights in range → status shows "Active: Classic Card (manual)"
2. Wait for zero flights → status shows "Active: Clock (idle fallback — no flights)"
3. Manually switch mode during idle fallback → status shows "Active: {mode} (manual)"
4. Verify API response shape via `curl http://flightwall.local/api/display/modes | jq`
5. Verify gzipped assets load correctly (no 404s, no corruption)
6. Check total gzipped web asset size stays under 50KB
7. Smoke-test the full dashboard UI (settings panel, mode picker tap, toast notifications) to confirm no regressions from shared JS changes in `dashboard.js`

### Project Structure Notes

- Alignment with hexagonal architecture: WebPortal (adapter) reads from ModeOrchestrator (core) — dependency direction is correct (adapter depends on core, not reverse)
- New CSS follows existing custom property naming: `--settings-border` already exists from Foundation release
- Status line pattern will be reused by dl-4 (scheduling) which adds "scheduled" state and "Resume Schedule" button — design the HTML structure to accommodate future extension
- The status line is the foundation for the UX spec's "Status line as universal confirmation" pattern across all journeys

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#DL2] — ModeOrchestrator state machine (MANUAL, SCHEDULED, IDLE_FALLBACK)
- [Source: _bmad-output/planning-artifacts/architecture.md#DL6] — API endpoint additions for Delight
- [Source: _bmad-output/planning-artifacts/architecture.md#DS5] — Existing mode switch API shape
- [Source: _bmad-output/planning-artifacts/architecture.md#Enforcement Rules] — Rules 24, 30
- [Source: _bmad-output/planning-artifacts/ux-design-specification-delight.md#Key Interaction Patterns] — Status line: "Active: [Mode] ([reason])"
- [Source: _bmad-output/planning-artifacts/ux-design-specification-delight.md#Component Strategy] — Mode Picker anatomy: "Status line at top of container, always visible"
- [Source: _bmad-output/planning-artifacts/ux-design-specification-delight.md#UX Consistency Patterns] — Inline feedback: "always visible while relevant"
- [Source: _bmad-output/planning-artifacts/ux-design-specification-delight.md#Experience Principles] — "Delegation requires transparency"
- [Source: _bmad-output/planning-artifacts/epics/epic-dl-1.md] — Epic dl-1 stories dl-1.2 (ModeOrchestrator) and dl-1.3 (manual switching)
- [Source: _bmad-output/planning-artifacts/prd.md#FR9] — User can view and edit all device settings from web dashboard
- [Source: firmware/adapters/WebPortal.h] — Current WebPortal API surface
- [Source: firmware/src/main.cpp#displayTask] — Display task loop structure (lines 272-418)

## Dev Agent Record

### Agent Model Used

Claude Opus 4 (via Claude Code)

### Debug Log References

- Build: `pio run` — SUCCESS (77.7% flash, 1.17MB)
- Test compilation: `pio test --without-uploading --without-testing -f test_mode_orchestrator` — PASSED
- All existing tests (config_manager, layout_engine, ota_self_check, telemetry_conversion) — compile PASSED
- Total gzipped web assets: 35.7KB (under 50KB budget)

### Completion Notes List

- Created `ModeOrchestrator` static class with full state machine (MANUAL, IDLE_FALLBACK, SCHEDULED states)
- Added `GET /api/display/modes` endpoint returning modes[], active, switch_state, orchestrator_state, state_reason
- Added `POST /api/display/mode` endpoint routing through orchestrator (Rule 24 compliance)
- Mode Picker card in dashboard with status line showing "Active: {Name} ({reason})"
- JavaScript: `loadDisplayModes()` on page load, `switchMode()` with switching state, re-fetch after POST (AC #4, #8)
- Batch DOM updates in `updateModeStatus()` — status line + card subtitles in single function call (AC #8)
- Integrated `ModeOrchestrator::tick()` in main.cpp loop after flight fetch (idle fallback logic)
- CSS uses existing design tokens (--bg, --border, --text, --text-muted, --primary)
- No background polling — state fetched on load and after user actions only

### Change Log

- 2026-04-12: Implemented story dl-1.5 — Orchestrator State Feedback in Dashboard

### File List

**New files:**
- `firmware/core/ModeOrchestrator.h` — Static class header with state enum and API
- `firmware/core/ModeOrchestrator.cpp` — Implementation: state transitions, reason strings
- `firmware/test/test_mode_orchestrator/test_main.cpp` — 12 Unity tests covering all state transitions

**Modified files:**
- `firmware/adapters/WebPortal.h` — Added `_handleGetDisplayModes` declaration
- `firmware/adapters/WebPortal.cpp` — Added ModeOrchestrator include, GET /api/display/modes handler, POST /api/display/mode handler
- `firmware/src/main.cpp` — Added ModeOrchestrator include, init() call, tick() call after flight fetch
- `firmware/data-src/dashboard.html` — Added Mode Picker card section with status line
- `firmware/data-src/style.css` — Added .mode-status-line, .mode-card styles (~18 lines)
- `firmware/data-src/dashboard.js` — Added Mode Picker JS: loadDisplayModes(), updateModeStatus(), renderModeCards(), switchMode()
- `firmware/data/dashboard.html.gz` — Regenerated
- `firmware/data/dashboard.js.gz` — Regenerated
- `firmware/data/style.css.gz` — Regenerated


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-5-story-5.md

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
| Security Vulnerabilities | 1 |
| **Total Issues** | **20** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | O - Open/Closed Principle Violation: The `WebPortal` hardcodes the list of display modes. Adding a new mode would require direct modification, violating OCP. | `firmware/adapters/WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | S - Single Responsibility Principle: The `ModeOrchestrator::tick` function is incomplete for the `SCHEDULED` state, which is part of its core enum. | `firmware/core/ModeOrchestrator.cpp:92` | +1 |
| 🟠 IMPORTANT | S - Single Responsibility Principle: The `displayTask` directly manages flight cycling logic, which architecture dictates should be owned by `DisplayMode` implementations. | `firmware/src/main.cpp:289` | +1 |
| 🟠 IMPORTANT | Buffer Overflow Risk (Silent Truncation): `ModeOrchestrator` uses fixed-size `char` arrays for mode IDs and names. Longer inputs are silently truncated without input validation. | `firmware/core/ModeOrchestrator.h` | +1 |
| 🔴 CRITICAL | Architectural Compliance (Bypassed ModeRegistry): `main.cpp`'s `displayTask` directly calls `g_display.renderFlight()` instead of `ModeRegistry::tick()`, bypassing the core architectural design for mode rendering. | `firmware/src/main.cpp:415` | +3 |
| 🔴 CRITICAL | Architectural Compliance (Missing `g_flightCount` atomic): The `std::atomic<uint8_t> g_flightCount` for signaling flight count, specified in `architecture.md`, is missing and not used for orchestrator signaling. | `architecture.md#DL2` | +3 |
| 🟠 IMPORTANT | Incomplete Test Coverage: No tests for the `SCHEDULED` state, despite it being part of the `ModeOrchestrator` enum and its getter methods. | `firmware/test/test_mode_orchestrator/test_main.cpp` | +1 |
| 🟡 MINOR | Weak Assertion: `test_idle_fallback_state_strings` uses `strstr` for substring checks instead of `TEST_ASSERT_EQUAL_STRING` for exact string comparison of reason. | `firmware/test/test_mode_orchestrator/test_main.cpp:71-74` | +0.3 |
| 🟠 IMPORTANT | Incomplete Test Coverage: Missing tests for fixed-size buffer truncation behavior in `ModeOrchestrator::onManualSwitch`. | `firmware/test/test_mode_orchestrator/test_main.cpp` | +1 |
| 🟠 IMPORTANT | Heap Fragmentation: Frequent use of `String output; serializeJson(doc, output);` in API handlers creates temporary `String` objects, risking heap fragmentation. | `firmware/adapters/WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | UI Performance: `renderModeCards` rebuilds the entire DOM list of mode cards instead of updating existing elements, which is inefficient. | `firmware/data-src/dashboard.js:1953` | +1 |
| 🟠 IMPORTANT | Magic Strings: Mode IDs and names are hardcoded in `WebPortal.cpp` and `ModeOrchestrator.cpp`, leading to duplication and maintenance risk. | `firmware/adapters/WebPortal.cpp:626` | +1 |
| 🟡 MINOR | Memory Efficiency: Redundant `JsonDocument` creation in `_sendJsonError` for every error response. | `firmware/adapters/WebPortal.cpp:1152` | +0.3 |
| 🟡 MINOR | Dead Code/Tech Debt: `g_statsLogosMatched` is explicitly commented as a placeholder and not updated, contributing to code clutter and potential confusion. | `firmware/src/main.cpp:164` | +0.3 |
| 🟡 MINOR | Legacy JavaScript: Extensive use of `var` instead of `const`/`let`, diverging from modern JS best practices. | `firmware/data-src/dashboard.js` | +0.3 |
| 🟡 MINOR | Hardcoded Color Value: `#fff` is used instead of a CSS variable for `color` in `.cal-pattern-btn.active`, violating theme consistency. | `firmware/data-src/style.css:487` | +0.3 |
| 🟡 MINOR | CSS Redundancy: Overlapping `font-weight` and `color` declarations in Mode Picker CSS. | `firmware/data-src/style.css:707` | +0.3 |
| 🟡 MINOR | CSS Robustness: Missing explicit `font-weight` for `.mode-status-line`'s base style, relying on inheritance. | `firmware/data-src/style.css:707` | +0.3 |
| 🟠 IMPORTANT | Input Validation: `_handlePostDisplayMode` lacks length validation for `mode_id` from client, risking issues with fixed-size buffers in `ModeOrchestrator`. | `firmware/adapters/WebPortal.cpp:684` | +1 |
| 🟡 MINOR | String Encoding Fragility: Usage of Unicode em-dash in a `const char*` literal without explicit UTF-8 handling can lead to display issues. | `firmware/core/ModeOrchestrator.cpp:25` | +0.3 |

### Evidence Score: 20.4

| Score | Verdict |
|-------|---------|
| **20.4** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[7/10] Open/Closed Principle Violation:** The `WebPortal` hardcodes the list of display modes. Adding a new mode would require direct modification of `WebPortal.cpp`, violating OCP.
  - 📍 `firmware/adapters/WebPortal.cpp:623`
  - 💡 Fix: Refactor `WebPortal` to query `ModeRegistry` dynamically for available modes and their properties, rather than hardcoding them.
- **[6/10] Single Responsibility Principle:** The `ModeOrchestrator::tick` function is designated to manage state transitions but explicitly ignores the `SCHEDULED` state. This incompleteness violates SRP by not fully encapsulating state management within `tick`.
  - 📍 `firmware/core/ModeOrchestrator.cpp:92`
- **[8/10] Single Responsibility Principle:** The `displayTask` in `main.cpp` contains specific flight cycling logic (`currentFlightIndex`, `lastCycleMs`) that, per architectural decision DS1, should be encapsulated within the individual `DisplayMode` implementations. This creates unnecessary coupling.
  - 📍 `firmware/src/main.cpp:289`
- **Architectural Deviation:** `main.cpp`'s `displayTask` directly calls `g_display.renderFlight()` instead of `ModeRegistry::tick()`, fundamentally bypassing the intended pluggable mode system as outlined in `architecture.md#DS4`.
  - 📍 `firmware/src/main.cpp:415`
- **Architectural Deviation:** The explicit architectural decision for cross-core flight count signaling via `std::atomic<uint8_t> g_flightCount` (`architecture.md#DL2`) is not implemented in `main.cpp`. The `ModeOrchestrator::tick` receives `flights.size()` directly, not an atomic signal.
  - 📍 `architecture.md#DL2`
✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes &amp; Readability

- **Legacy `var` usage:** Extensive use of `var` instead of `const` or `let` in JavaScript code, which is outdated and can lead to scoping issues.
  - 📍 `firmware/data-src/dashboard.js`
✅ Code follows style guidelines and is readable.

---

## ⚡ Performance &amp; Scalability

- **[High] Heap Fragmentation:** Frequent use of `String` objects for API response serialization (`String output; serializeJson(doc, output);`) in `WebPortal.cpp` can cause heap fragmentation over time on memory-constrained ESP32 devices.
  - 📍 `firmware/adapters/WebPortal.cpp`
  - 💡 Fix: Consider using `AsyncResponseStream` or pre-allocated `char` buffers with `serializeJson` to reduce dynamic `String` allocations.
- **[Medium] Inefficient DOM Updates:** The `renderModeCards` function in `dashboard.js` rebuilds the entire list of mode cards on every call (`innerHTML = ''`) rather than intelligently updating existing elements. This leads to inefficient DOM manipulation and potential UI flicker.
  - 📍 `firmware/data-src/dashboard.js:1953`
  - 💡 Fix: Implement a more granular DOM reconciliation strategy, updating only changed elements.
✅ No significant performance issues detected.

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug:** Buffer Overflow Risk (Silent Truncation): `ModeOrchestrator` uses fixed-size `char` arrays for mode IDs/names. `strncpy` with explicit null-termination mitigates direct overflow but silently truncates longer input strings, which can lead to data loss or unexpected behavior.
  - 📍 `firmware/core/ModeOrchestrator.h`
  - 🔄 Reproduction: Call `ModeOrchestrator::onManualSwitch` with a `modeId` or `modeName` longer than 31 characters. The stored value will be truncated without warning.
- **🔒 [High] Input Validation:** The `_handlePostDisplayMode` endpoint in `WebPortal.cpp` does not validate the length of the `mode_id` received from the client. This unchecked input, when passed to `ModeOrchestrator::onManualSwitch`, relies solely on `strncpy` to prevent buffer overflow, but is a general security/robustness risk.
  - 📍 `firmware/adapters/WebPortal.cpp:684`
  - ⚠️ Impact: Could lead to unexpected behavior, including silent data truncation or minor performance degradation if exceptionally long strings are sent.
- **🎭 Lying Test:** Missing `SCHEDULED` state tests
  - 📍 `firmware/test/test_mode_orchestrator/test_main.cpp`
  - 🤥 Why it lies: The `ModeOrchestrator` includes a `SCHEDULED` state and related logic in its getter methods, but the unit tests completely omit coverage for this state, giving a false impression of full test coverage for the orchestrator.
- **🎭 Lying Test:** Weak `idle_fallback` reason string test
  - 📍 `firmware/test/test_mode_orchestrator/test_main.cpp:71-74`
  - 🤥 Why it lies: Uses `strstr` to check for substrings instead of `TEST_ASSERT_EQUAL_STRING` for exact matching, allowing variations in the actual reason string to pass unnoticed.
- **🎭 Lying Test:** Missing buffer overflow/truncation tests
  - 📍 `firmware/test/test_mode_orchestrator/test_main.cpp`
  - 🤥 Why it lies: The test suite fails to include cases that validate the behavior of `ModeOrchestrator` when presented with input strings exceeding the fixed-size `char` array buffers, leaving a critical edge case untested.
✅ Code appears correct and secure.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Magic Strings for Mode IDs and Names: Mode IDs and display names are hardcoded and duplicated across `ModeOrchestrator.cpp` and `WebPortal.cpp`. This increases maintenance burden and risk of inconsistency.
  - 📍 `firmware/adapters/WebPortal.cpp:626`
  - 💥 Explosion radius: Changes to mode IDs/names require tracking down and updating multiple hardcoded instances, leading to potential bugs if one is missed.
- **💣 Tech Debt:** Redundant `JsonDocument` in `_sendJsonError`: The `_sendJsonError` helper function creates a new `JsonDocument` every time, which is inefficient for error responses and contributes to heap churn.
  - 📍 `firmware/adapters/WebPortal.cpp:1152`
  - 💥 Explosion radius: Minor, but contributes to overall system memory pressure and fragmentation on an embedded device.
- **💣 Tech Debt:** Unused/Incomplete Global `g_statsLogosMatched`: This atomic variable in `main.cpp` is declared and included in `FlightStatsSnapshot` but explicitly commented as a placeholder not used until Epic 3. It adds clutter and potential confusion.
  - 📍 `firmware/src/main.cpp:164`
  - 💥 Explosion radius: Very small, mainly impacts code readability and clarity for future developers.
- **💣 Tech Debt:** Legacy `var` usage in `dashboard.js`: The JavaScript codebase predominantly uses `var` for variable declarations instead of

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-5-story-5.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 0 |
| SOLID Violations | 2 |
| Hidden Bugs | 4 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

Scope reviewed: uncommitted product changes on `main` (`9` tracked files changed, `+294/-24`) plus `3` new product files.  
Product file list matches the story’s declared file list.  
Execution note: source review only; I did not rerun `pio run` or on-device tests in this read-only session.

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Dashboard state is not wired to the real renderer, so the UI can report modes the wall never shows | `firmware/src/main.cpp`, `firmware/core/ModeOrchestrator.cpp` | +3 |
| 🔴 CRITICAL | `/api/display/mode` body parsing is broken for multi-chunk async requests | `firmware/adapters/WebPortal.cpp` | +3 |
| 🟠 IMPORTANT | Orchestrator boot state always resets to `classic_card/manual`, ignoring persisted display mode | `firmware/core/ModeOrchestrator.cpp`, `firmware/src/main.cpp` | +1 |
| 🟠 IMPORTANT | `/api/display/modes` hardcodes the mode catalog and hardcodes `switch_state = "idle"` | `firmware/adapters/WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | Initial mode-load failures are silently swallowed instead of showing a toast | `firmware/data-src/dashboard.js` | +1 |
| 🟠 IMPORTANT | New tests only cover the in-memory state holder and miss the broken HTTP/display integration | `firmware/test/test_mode_orchestrator/test_main.cpp` | +1 |
| 🟠 IMPORTANT | Story/completion notes overstate the implementation as a “full state machine” | `dl-1-5-story-5.md`, `firmware/core/ModeOrchestrator.cpp` | +1 |
| 🟢 CLEAN PASS | Security, performance, style, and type-safety review found no additional material defects in this diff | review sweep | -2.0 |

### Evidence Score: 9.0

| Score | Verdict |
|-------|---------|
| **9.0** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[10/10] False source of truth:** [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:372) still renders the legacy `g_display.renderFlight()` / `showLoading()` path, while [ModeOrchestrator.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeOrchestrator.cpp:59) only mutates strings. AC #1/#3/#4 are therefore cosmetic: the dashboard can say `Clock` or `Live Flight Card` even though the LEDs never switch modes. Fix: make orchestration drive the actual mode/render pipeline and have the API read from that same owner.
- **[7/10] Open/Closed violation:** [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:841) hardcodes three modes in the adapter, and [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:865) hardcodes `switch_state = "idle"`. That duplicates mode metadata and guarantees drift as soon as a new mode or real switch lifecycle exists. Fix: serialize `ModeRegistry` data instead of rebuilding the catalog in `WebPortal`.

---

## 🐍 Pythonic Crimes & Readability

✅ No material language-style problems matter here; the failures are behavioral and architectural.

---

## ⚡ Performance & Scalability

✅ No material performance regression in this diff. The regenerated gzipped assets total `27,007` bytes, comfortably below the `50KB` story budget.

---

## 🐛 Correctness & Safety

- **🐛 Broken async body handling:** [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:297) deserializes `data` on the final chunk using `total` bytes without accumulating earlier chunks. The correct accumulation pattern already exists for `/api/settings` at [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:174). Reproduction: send `/api/display/mode` as a multi-packet POST; valid JSON intermittently fails or reads garbage.
- **🐛 Boot state lies after restart:** [ModeOrchestrator.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeOrchestrator.cpp:12) always resets to `classic_card/manual`, and [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:592) calls that on every boot. That ignores the persisted `display_mode` contract, so a reboot can make `/api/display/modes` report `Classic Card (manual)` even if the saved selection was something else.
- **🐛 Unsynchronized shared state:** [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:7) states the server runs on the AsyncTCP task, while [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:907) mutates orchestrator state from the loop. [ModeOrchestrator.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeOrchestrator.cpp:59) updates `_state`, `_activeModeId`, and `_activeModeName` in separate writes with no lock/message boundary, so GET/POST can observe mixed snapshots.
- **🐛 UX contract violation:** [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:2573) swallows initial `GET /api/display/modes` failures and leaves the status line stuck at `loading`. That directly violates the story note at [dl-1-5-story-5.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-5-story-5.md:98) requiring every `fetch()` failure to call `showToast()`.
- **🎭 Lying test coverage:** [test_mode_orchestrator/test_main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/test/test_mode_orchestrator/test_main.cpp:12) only proves the in-memory state container can flip enums and strings. It never exercises the HTTP handlers, multi-chunk request bodies, persisted boot restore, or any renderer integration, so the most important defects in this story still “pass.”

---

## 🔧 Maintainability Issues

- **💣 Story/implementation drift:** The completion notes in [dl-1-5-story-5.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-5-story-5.md:241) claim a “full state machine (MANUAL, IDLE_FALLBACK, SCHEDULED states),” but [ModeOrchestrator.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeOrchestrator.cpp:94) never evaluates schedule rules and cannot enter `SCHEDULED`. That will mislead any later story that treats dl-1.5 as a real orchestrator foundation.

---

## 🛠️ Suggested Fixes

1. **Fix the POST parser in `WebPortal.cpp`:** reuse the existing accumulated-body pattern from `/api/settings` for `/api/display/mode` so chunked requests are safe.
2. **Make the renderer the source of truth:** `ModeOrchestrator::onManualSwitch()` / `tick()` must request or restore the real display mode, not just update strings for the API.
3. **Restore persisted mode on boot:** initialize orchestrator state from persisted `display_mode` / registry state instead of hard-resetting to `classic_card`.
4. **Remove adapter hardcoding:** generate `modes[]`, `active`, and `switch_state` from the mode subsystem, not from hand-maintained literals in `WebPortal.cpp`.
5. **Add integration tests:** cover `GET /api/display/modes`, chunked `POST /api/display/mode`, persisted boot restore, and one end-to-end assertion that reported mode matches rendered mode.
6. **Fix the failure UX:** initial `loadDisplayModes()` failures must toast and clear placeholder text instead of silently leaving `loading`.

---

**Review Actions:**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** dl-1.5 - Orchestrator State Feedback in Dashboard

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| AC Implementation Gaps | 2 |
| Code Quality Issues | 4 |
| Architecture Compliance | 2 |
| Security/Edge Cases | 2 |
| Documentation/Comments | 1 |
| **Total Issues** | **11** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task 1.1 lies: `getStateReason()` exists but story claims it was "added new" | ModeOrchestrator.h already declares it | +3 |
| 🔴 CRITICAL | Missing NVS persistence for manual mode - breaks recovery after reboot | architecture.md#DS6 requires persistence | +3 |
| 🟠 IMPORTANT | Hardcoded mode list violates ModeRegistry architecture | WebPortal.cpp:722-746 | +1 |
| 🟠 IMPORTANT | SCHEDULED state transitions not implemented in tick() | ModeOrchestrator.cpp:76-84 | +1 |
| 🟠 IMPORTANT | No bounds checking on flightCount parameter (0-255 range) | ModeOrchestrator.cpp:80 | +1 |
| 🟠 IMPORTANT | JavaScript missing null checks for DOM elements | dashboard.js:1679-1707 | +1 |
| 🟡 MINOR | UTF-8 em-dash sequence may truncate on some displays | ModeOrchestrator.cpp:24 | +0.3 |
| 🟡 MINOR | CSS fallback values don't match design tokens | style.css:700-703 | +0.3 |
| 🟡 MINOR | Hardcoded mode name lookup duplicates ModeOrchestrator logic | WebPortal.cpp:356-362 | +0.3 |

### Evidence Score: 11.9

| Score | Verdict |
|-------|---------|
| **11.9** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

### [7/10] Single Responsibility Violation: WebPortal knows too much about modes
- **Issue:** `WebPortal::_handleGetDisplayModes()` hardcodes the mode list instead of querying ModeRegistry
- **Location:** `firmware/adapters/WebPortal.cpp:722-746`
- **Architecture Violation:** architecture.md#DS2 specifies ModeRegistry should provide mode list via static table
- **Current Code:**
```cpp
// Classic Card mode
JsonObject mClassic = modes.add<JsonObject>();
mClassic["id"] = "classic_card";
mClassic["name"] = "Classic Card";  // Hardcoded!
```
- **Fix:** Query ModeRegistry/ModeOrchestrator for available modes, don't hardcode

### [6/10] Open/Closed Violation: Adding a mode requires editing WebPortal
- **Issue:** To add "DeparturesBoardMode" (dl-2), you must edit WebPortal.cpp POST handler
- **Location:** `firmware/adapters/WebPortal.cpp:356-362`
- **Current Code:**
```cpp
if (strcmp(modeId, "classic_card") == 0) modeName = "Classic Card";
else if (strcmp(modeId, "live_flight") == 0) modeName = "Live Flight Card";
else if (strcmp(modeId, "clock") == 0) modeName = "Clock";
```
- **Fix:** ModeOrchestrator should provide mode name lookup, not WebPortal

### [5/10] Incomplete State Machine Implementation
- **Issue:** `SCHEDULED` state is defined but `tick()` doesn't handle SCHEDULED→MANUAL transitions
- **Location:** `firmware/core/ModeOrchestrator.cpp:76-84`
- **Current Code:** Only handles MANUAL↔IDLE_FALLBACK, ignores SCHEDULED entirely
- **Risk:** Future dl-4 story will need to refactor this anyway, creating tech debt now

---

## 🐍 Code Quality Issues

### Missing Null Pointer Checks in JavaScript
**Location:** `firmware/data-src/dashboard.js:1679-1707`

```javascript
function updateModeStatus(data) {
  var activeMode = null;
  if (data.modes && data.active) {
    for (var i = 0; i < data.modes.length; i++) {
      if (data.modes[i].id === data.active) {
        activeMode = data.modes[i];
        break;
      }
    }
  }
  // activeMode could still be null here!
  if (modeStatusName && activeMode) {  // Good check
    modeStatusName.textContent = activeMode.name;
  }
  if (modeStatusReason) {
    modeStatusReason.textContent = data.state_reason || 'unknown';
  }
  // But modeStatusName.textContent could be set to undefined if activeMode is null
```

**Fix:** Add explicit fallback for when active mode not found in modes array.

### Buffer Size Mismatch Risk
**Location:** `firmware/core/ModeOrchestrator.cpp:11-14`

```cpp
char ModeOrchestrator::_activeModeId[32] = "classic_card";
char ModeOrchestrator::_activeModeName[32] = "Classic Card";
```

The `_manualModeName` buffer is 32 bytes but mode names like "Departures Board Mode" (21 chars) leave only 10 bytes for future expansion. No documentation on max mode name length.

### strncpy Pattern Without Explicit Null Termination
**Location:** `firmware/core/ModeOrchestrator.cpp:58-65`

```cpp
strncpy(_activeModeId, modeId, sizeof(_activeModeId) - 1);
_activeModeId[sizeof(_activeModeId) - 1] = '\0';  // This line is GOOD
```

While the null termination IS present, the pattern is error-prone for future edits. Consider a helper function.

---

## 🐛 Correctness & Safety

### Race Condition in Mode Switching
**Location:** `firmware/data-src/dashboard.js:1717-1743`

```javascript
function switchMode(modeId) {
  if (modeSwitchInFlight) return;  // Good - prevents concurrent requests
  modeSwitchInFlight = true;
  // ... sets switching state visually
  FW.post('/api/display/mode', { mode_id: modeId }).then(function(res) {
    modeSwitchInFlight = false;
    // ...
  });
}
```

**Issue:** If the POST succeeds but the subsequent `loadDisplayModes()` fetch fails, the UI shows stale state. The `modeSwitchInFlight` flag is reset before the state is confirmed updated.

### Missing Input Validation on flightCount
**Location:** `firmware/core/ModeOrchestrator.cpp:80`

```cpp
void ModeOrchestrator::tick(uint8_t flightCount) {
```

`uint8_t` accepts 0-255, but the logic assumes binary states (0 or >0). No validation that `flightCount` is reasonable (e.g., > 100 flights might indicate corruption).

---

## 🛠️ Specific Issues by Acceptance Criterion

### AC #5: Partial Implementation
**Expected:** `orchestrator_state` and `state_reason` in response
**Actual:** ✅ Implemented BUT `switch_state` is hardcoded to "idle"

```cpp
data["switch_state"] = "idle";  // WebPortal.cpp:748
```

This doesn't reflect actual ModeRegistry switch state (SWITCHING vs IDLE).

### AC #7 & #8: Batch DOM Update Claim
**Story Claims:** "Ensure both UI elements update in the same DOM operation"
**Actual:** `updateModeStatus()` updates status line first, then queries DOM for cards again:

```javascript
// First update
if (modeStatusName && activeMode) {
  modeStatusName.textContent = activeMode.name;
}
// Later in same function - second DOM query
if (modeCardsList) {
  var cards = modeCardsList.querySelectorAll('.mode-card');  // Re-queries DOM!
```

This is two operations, not batched. Use DocumentFragment or requestAnimationFrame.

---

## 🎭 Task Completion Audit

### Task 1.1: MISLEADING
**Claim:** "implement `getStateReason()` as a new static public method"
**Reality:** The method ALREADY EXISTS in the header file (line 25). The task was to confirm it exists, not implement it. The story is documenting existing code as "new work."

### Task 5.4: Cannot Verify
**Claim:** "Verify total gzipped web assets remain well under 50KB budget"
**Problem:** No asset size check script or CI step is visible in provided files. Manual verification only.

---

## 🔧 Suggested Fixes

### 1. Fix Mode Name Lookup (WebPortal.cpp:356-362)

**Before:**
```cpp
if (strcmp(modeId, "classic_card") == 0) modeName = "Classic Card";
else if (strcmp(modeId, "live_flight") == 0) modeName = "Live Flight Card";
else if (strcmp(modeId, "clock") == 0) modeName = "Clock";
```

**After:**
```cpp
// Use ModeOrchestrator's stored name if available
const char* modeName = ModeOrchestrator::getModeDisplayName(modeId);
if (!modeName) {
    _sendJsonError(request, 400, "Unknown mode_id", "UNKNOWN_MODE");
    return;
}
```

### 2. Add SCHEDULED State Handling (ModeOrchestrator.cpp)

**Add to tick():**
```cpp
void ModeOrchestrator::tick(uint8_t flightCount) {
    // State priority: SCHEDULED > IDLE_FALLBACK > MANUAL
    if (_state == OrchestratorState::SCHEDULED) {
        // Scheduled mode manages itself - don't auto-transition
        return;
    }
    // ... existing logic
}
```

### 3. Fix JavaScript Race Condition (dashboard.js)

**Before:**
```javascript
}).then(function(res) {
  modeSwitchInFlight = false;
  if (!res.body || !res.body.ok) {
    // ... error handling
    return;
  }
  loadDisplayModes();
});
```

**After:**
```javascript
}).then(function(res) {
  if (!res.body || !res.body.ok) {
    modeSwitchInFlight = false;
    // ... error handling
    return;
  }
  // Only clear in-flight after successful state refresh
  return loadDisplayModes().finally(function() {
    modeSwitchInFlight = false;
  });
});
```

### 4. Add NVS Persistence for Manual Mode

**In ModeOrchestrator.cpp, add to onManualSwitch():**
```cpp
// Persist manual selection for reboot recovery
Preferences prefs;
prefs.begin("flightwall", false);
prefs.putString("last_mode", modeId);
prefs.end();
```

**In init(), add:**
```cpp
// Restore last manual mode from NVS
Preferences prefs;
prefs.begin("flightwall", true);
String lastMode = prefs.getString("last_mode", "classic_card");
prefs.end();
// ... restore to _manualModeId
```

---

## Summary

The implementation **functionally works** for the immediate story requirements but carries significant technical debt:

1. **Hardcoded mode lists** will require refactoring in dl-2 and dl-4
2. **Missing SCHEDULED state handling** defers work to future stories
3. **No NVS persistence** breaks the "resume manual mode after reboot" expectation
4. **JavaScript race conditions** may cause UI state desynchronization

**Recommendation:** Address the ModeRegistry integration and NVS persistence before marking this complete. The current implementation is a "works for now" solution that violates the architecture's extensibility goals.

**Verdict:** ⚠️ **MAJOR REWORK** required before merge. Address architectural violations and add missing persistence.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
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
<var name="reviewer_count">3</var>
<var name="session_id">cc162ac7-df22-45ad-be6f-a20230b52728</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="af701597">embedded in prompt, file id: af701597</var>
<var name="story_id">dl-1.5</var>
<var name="story_key">dl-1-5-story-5</var>
<var name="story_num">5</var>
<var name="story_title">5-story-5</var>
<var name="template">False</var>
<var name="timestamp">20260412_1950</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-1.5)
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