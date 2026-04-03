# Story 3.4: Canvas Layout Preview in Dashboard

Status: ready-for-dev

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

- [ ] Task 1: Add preview markup in Hardware card (AC: #1, #5)
  - [ ] Extend `firmware/data-src/dashboard.html` Hardware section with:
    - [ ] `<canvas id="layout-preview">` container
    - [ ] legend block for zone colors
    - [ ] helper note (predictive preview)
  - [ ] Keep section order coherent with existing hardware inputs and resolution text.

- [ ] Task 2: Canvas renderer in `dashboard.js` (AC: #1, #2, #3, #5)
  - [ ] Reuse existing top-level `computeLayout(tilesX, tilesY, tilePixels)` (already present) as single JS geometry source.
  - [ ] Add `renderLayoutCanvas(layout)` that:
    - [ ] draws matrix bounds
    - [ ] draws zone rectangles with distinct colors
    - [ ] labels zones where space permits
    - [ ] gracefully handles invalid layout (`valid=false`)
  - [ ] Hook renderer to:
    - [ ] initial `/api/layout` fetch result
    - [ ] local hardware input `input` events (tiles/pixels)
  - [ ] Keep `updateHwResolution()` and canvas derived from same parsed values to avoid drift.

- [ ] Task 3: Use `/api/layout` for initial state (AC: #1, #4)
  - [ ] Add a dashboard JS load step `FW.get('/api/layout')` on init (best-effort).
  - [ ] If `/api/layout` fails, fallback to local `computeLayout()` using current form values loaded from `/api/settings`.
  - [ ] Optional dev-only parity check: compare `/api/layout` result with local `computeLayout()` and log warning on mismatch.

- [ ] Task 4: Styling (`style.css`) (AC: #5)
  - [ ] Add preview container/card styles:
    - [ ] max width behavior within 480px dashboard
    - [ ] canvas border/background contrast for dark theme
    - [ ] legend chips + labels
  - [ ] Ensure preview remains readable with reduced viewport widths.

- [ ] Task 5: Triple-feedback integration safety (AC: #6)
  - [ ] Keep hardware Apply path (`btn-apply-hardware`) unchanged except any needed hook to refresh canvas after confirmed reboot cycle.
  - [ ] Do not block existing network/API/hardware apply flows while adding preview logic.

- [ ] Task 6: Verification
  - [ ] Manual: all 4 architecture vectors render correctly in canvas:
    - [ ] 10x2@16 -> 160x32, full
    - [ ] 5x2@16 -> 80x32, full
    - [ ] 12x3@16 -> 192x48, expanded
    - [ ] 10x1@16 -> 160x16, compact
  - [ ] Manual: changing tile inputs updates preview immediately.
  - [ ] `pio run` and LittleFS asset pipeline/update as required.

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

—

### Debug Log References

### Completion Notes List

### File List
