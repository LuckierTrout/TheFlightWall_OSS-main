# Story dl-1.5: Orchestrator State Feedback in Dashboard

Status: review

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

#### Review Follow-ups (AI)
- [ ] [AI-Review] HIGH: Display renderer not wired to orchestrator state — when IDLE_FALLBACK activates, LEDs still render flights/loading instead of ClockMode. Requires completing dl-1.1 (ClockMode + DisplayMode interface) and wiring orchestrator state to the display task (`firmware/src/main.cpp` displayTask, `firmware/core/`)
- [ ] [AI-Review] MEDIUM: `switch_state` hardcoded to `"idle"` in `_handleGetDisplayModes` — actual switch lifecycle tracking requires ModeRegistry (ds-1-3). Track until ModeRegistry is implemented (`firmware/adapters/WebPortal.cpp:865`)
- [ ] [AI-Review] MEDIUM: NVS persistence for manual mode selection missing — reboot resets to `classic_card/manual` regardless of prior selection. Implement as part of ds-3-2 (`firmware/core/ModeOrchestrator.cpp:init()`)
- [ ] [AI-Review] LOW: Hardcoded mode catalog in `_handleGetDisplayModes` and POST handler — will need to be replaced with ModeRegistry query when ds-1-3 is implemented (`firmware/adapters/WebPortal.cpp:841-868`)

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
- **[Synthesis 2026-04-12]** Fixed UB in POST /api/display/mode: `deserializeJson(reqDoc, data, total)` → `deserializeJson(reqDoc, data, len)` to avoid reading past chunk buffer on multi-chunk bodies
- **[Synthesis 2026-04-12]** Fixed silent catch in `loadDisplayModes()`: replaced no-op catch with `showToast` per architecture Rule (every fetch must toast on failure)

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
- `firmware/data/dashboard.js.gz` — Regenerated (re-regenerated by synthesis after showToast fix)
- `firmware/data/style.css.gz` — Regenerated

## Senior Developer Review (AI)

### Review: 2026-04-12
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 9.0–20.4 (across 3 reviewers) → REJECT
- **Issues Found:** 6 verified (2 fixed, 4 deferred to future stories)
- **Issues Fixed:** 2
- **Action Items Created:** 4
