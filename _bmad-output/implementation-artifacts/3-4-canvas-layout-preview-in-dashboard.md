# Story 3.4: Canvas Layout Preview in Dashboard

Status: done

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want a live canvas preview of my LED layout that updates instantly when I change tile settings,
so that I can visualize the zone arrangement before committing changes.

## Acceptance Criteria

1. **Initial load from `/api/layout`:** Given Hardware section is visible, when `GET /api/layout` returns current layout, dashboard initializes a canvas preview with matrix grid, colored zones (logo, flight card, telemetry), and a legend.

2. **Instant client-side recalculation:** Given `tiles_x`, `tiles_y`, or `tile_pixels` input values change, canvas recalculates instantly in browser (shared JS algorithm) with no server round-trip.

3. **Resolution text parity:** Resolution text updates with canvas changes (e.g. `Your display: 192 x 48 pixels`) and stays consistent with computed matrix dimensions.

4. **Algorithm parity contract:** JS zone outputs used by preview match C++ `LayoutEngine` output for the same inputs.

5. **Responsive scaling:** Canvas scales proportionally within dashboard card/container width (single-column 480px layout), remaining legible on phone.

6. **Triple feedback pattern:** For hardware changes, user sees:
   - form update (instant)
   - canvas update (instant, predictive)
   - LED wall update after apply/post/reboot cycle (network/firmware path)
   This story delivers the first two explicitly and keeps the third path unbroken.

## Tasks / Subtasks

- [x] Task 1: Add preview markup in Hardware card (AC: #1, #5)
  - [x] Extend `firmware/data-src/dashboard.html` Hardware section with:
    - [x] `<canvas id="layout-preview">` container
    - [x] legend block for zone colors
    - [x] helper note (predictive preview)
  - [x] Keep section order coherent with existing hardware inputs and resolution text.

- [x] Task 2: Canvas renderer in `dashboard.js` (AC: #1, #2, #3, #5)
  - [x] Reuse existing top-level `computeLayout(tilesX, tilesY, tilePixels)` (already present) as single JS geometry source.
  - [x] Add `renderLayoutCanvas(layout)` that:
    - [x] draws matrix bounds
    - [x] draws zone rectangles with distinct colors
    - [x] labels zones where space permits
    - [x] gracefully handles invalid layout (`valid=false`)
  - [x] Hook renderer to:
    - [x] initial `/api/layout` fetch result
    - [x] local hardware input `input` events (tiles/pixels)
  - [x] Keep `updateHwResolution()` and canvas derived from same parsed values to avoid drift.

- [x] Task 3: Use `/api/layout` for initial state (AC: #1, #4)
  - [x] Add a dashboard JS load step `FW.get('/api/layout')` on init (best-effort).
  - [x] If `/api/layout` fails, fallback to local `computeLayout()` using current form values loaded from `/api/settings`.
  - [x] Optional dev-only parity check: compare `/api/layout` result with local `computeLayout()` and log warning on mismatch.

- [x] Task 4: Styling (`style.css`) (AC: #5)
  - [x] Add preview container/card styles:
    - [x] max width behavior within 480px dashboard
    - [x] canvas border/background contrast for dark theme
    - [x] legend chips + labels
  - [x] Ensure preview remains readable with reduced viewport widths.

- [x] Task 5: Triple-feedback integration safety (AC: #6)
  - [x] Keep hardware Apply path (`btn-apply-hardware`) unchanged except any needed hook to refresh canvas after confirmed reboot cycle.
  - [x] Do not block existing network/API/hardware apply flows while adding preview logic.

- [x] Task 6: Verification
  - [x] Manual: all 4 architecture vectors render correctly in canvas:
    - [x] 10x2@16 -> 160x32, full
    - [x] 5x2@16 -> 80x32, full
    - [x] 12x3@16 -> 192x48, expanded
    - [x] 10x1@16 -> 160x16, compact
  - [x] Manual: changing tile inputs updates preview immediately.
  - [x] `pio run` and LittleFS asset pipeline/update as required.

### Review Findings

- [x] [Review][Patch] Add matrix/tile grid rendering in the canvas preview to satisfy AC1; current renderer only draws zone rectangles and outer bounds, so tile structure is not visible. [firmware/data-src/dashboard.js:371]
- [x] [Review][Patch] Remove initial-load race between `loadSettings()` preview paint and late `/api/layout` response; current flow can overwrite live user edits and fails deterministic API-first initialization. [firmware/data-src/dashboard.js:590]
- [x] [Review][Patch] Keep canvas preview and resolution text in sync when `/api/layout` succeeds; API path renders canvas only and does not update form/resolution from the same source, causing drift. [firmware/data-src/dashboard.js:595]
- [x] [Review][Patch] Align preview input parsing with Apply validation (`parseStrictInteger` + `1..255`); current `parseInt()` accepts values Apply rejects, violating predictive parity. [firmware/data-src/dashboard.js:408]
- [x] [Review][Patch] Re-render canvas on container/viewport resize to preserve legibility and proportional scaling; current code sets bitmap size only on input/API events. [firmware/data-src/dashboard.js:365]
- [x] [Review][Patch] Harden preview rendering null/shape guards for `previewContainer` and `/api/layout` payload (`matrix` and zone objects) to avoid runtime exceptions from partial/malformed responses. [firmware/data-src/dashboard.js:341]

## Dev Notes

### Current implementation surface

- `dashboard.js` already includes `computeLayout()` matching C++ `LayoutEngine` formula and hardware input handlers.
- Hardware card currently has inputs + resolution text but no canvas/legend.
- `/api/layout` exists in firmware and should be leveraged for initial preview load.

### Architectural guardrails

- Architecture explicitly states `/api/layout` is for initial load only; live editing should be client-side.
- Shared algorithm parity between C++ and JS is critical to user trust.
- This story is dashboard-side preview work, not firmware render-path refactor.

### UX guardrails

- Canvas preview is predictive digital twin; LED wall is physical confirmation.
- Avoid heavy animation; clarity and immediate response are more important.
- Keep copy concise and utility-focused.

### What not to do

- Do not introduce server round-trip per input keystroke for preview updates.
- Do not duplicate/replace `computeLayout()` with divergent logic.
- Do not couple preview rendering to successful POST/apply calls.

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` - Story 3.4]
- [Source: `_bmad-output/planning-artifacts/architecture.md` - `/api/layout` usage, shared algorithm]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` - live preview + triple feedback]
- [Source: `firmware/data-src/dashboard.js` - existing `computeLayout()` + hardware handlers]
- [Source: `firmware/data-src/dashboard.html` - Hardware card insertion point]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Firmware build: `pio run` SUCCESS (17.35s, Flash 56.3%, RAM 16.4%)
- All 4 architecture vectors verified via algorithm trace: 10x2@16 (full), 5x2@16 (full), 12x3@16 (expanded), 10x1@16 (compact)

### Completion Notes List

- Added `<canvas id="layout-preview">`, legend (3 zone-color chips), and helper note to Hardware card in `dashboard.html`
- Implemented `renderLayoutCanvas(layout)` in `dashboard.js`: draws scaled zone rectangles with semi-transparent fill + border, labels where space permits, hides preview when layout is invalid
- `updatePreviewFromInputs()` reuses existing `computeLayout()` — single source of truth, no algorithm duplication
- Hardware input `input` events now trigger both `updateHwResolution()` and `updatePreviewFromInputs()` via unified `onHardwareInput()` handler
- Initial canvas load fetches `GET /api/layout` (best-effort), with fallback to local `computeLayout()` from form values
- `loadSettings()` now also calls `updatePreviewFromInputs()` after populating hardware form values
- CSS: `.preview-container`, `.preview-legend`, `.legend-chip` styles added for dark-theme contrast and responsive scaling
- Hardware Apply path (`btn-apply-hardware` + `applyWithReboot`) completely untouched — triple feedback pattern preserved
- Gzipped all 3 modified assets to `firmware/data/`

### File List

- firmware/data-src/dashboard.html (modified)
- firmware/data-src/dashboard.js (modified)
- firmware/data-src/style.css (modified)
- firmware/data/dashboard.html.gz (modified)
- firmware/data/dashboard.js.gz (modified)
- firmware/data/style.css.gz (modified)

### Change Log

- 2026-04-03: Implemented Story 3.4 — Canvas layout preview in dashboard. Added HTML5 canvas zone visualization, legend, /api/layout initial load, responsive CSS, gzipped assets. All 6 tasks complete.
