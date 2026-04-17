
# Story LE-1.7: Editor Property Panel and Save UX

branch: le-1-7-editor-property-panel-and-save-ux
zone:
  - firmware/data-src/editor.html
  - firmware/data-src/editor.js
  - firmware/data-src/editor.css
  - firmware/data/editor.html.gz
  - firmware/data/editor.js.gz
  - firmware/data/editor.css.gz

Status: draft

## Story

As a **layout author**,
I want **per-widget property controls (size preset, color palette, alignment) and a save/activate flow**,
So that **I can fully configure my layout and push it to the LED wall in under 30 seconds**.

## Acceptance Criteria

1. **Given** a widget is selected **When** the property panel opens **Then** it exposes three controls: Size preset (S/M/L → 4×6 / 5×7 / 6×10), Alignment (L/C/R), and Color palette (8 LED-friendly swatches).
2. **Given** the V1 scope boundary **When** the property panel is inspected **Then** there is **no** kerning slider, **no** line-spacing slider, and **no** custom hex picker.
3. **Given** a layout is unsaved-new **When** the user taps "Save as New" **Then** `POST /api/layouts` persists it and the editor receives the new id.
4. **Given** a layout has a known id **When** the user taps "Save" **Then** `PUT /api/layouts/:id` overwrites it **And** the success toast reads "Layout saved".
5. **Given** a saved layout **When** the user taps "Save & Activate" **Then** save happens as above **And** `POST /api/layouts/:id/activate` is called **And** the LED wall switches to `custom_layout` within 2 seconds.
6. **Given** a save+activate flow **When** in flight **Then** the UI shows toasts "Layout saved" → "Applying…" → "Active" with visible progression.
7. **Given** the "Night Mode Preview" toggle **When** enabled **Then** the canvas dims to ~20% brightness visually (CSS filter, not real device state change).
8. **Given** unsaved changes **When** the user attempts to leave the page or tap a destructive control **Then** a confirm dialog warns about unsaved work.
9. **Given** an end-to-end smoke test **When** executed **Then** it: loads `/editor.html`, adds a text widget, sets color, taps Save & Activate, calls `GET /api/status`, and confirms `"mode": "custom_layout"` is active.

## Tasks / Subtasks

- [ ] **Task 1: Property panel UI** (AC: #1, #2)
  - [ ] `firmware/data-src/editor.html` — add property panel section (hidden until a widget is selected)
  - [ ] Size preset: three-segment control S/M/L (wired to 4×6, 5×7, 6×10 widget dimensions)
  - [ ] Alignment: three-segment control L/C/R
  - [ ] Color palette: 8 swatches (LED-friendly primaries + accents). No hex picker in V1.

- [ ] **Task 2: Panel wiring** (AC: #1)
  - [ ] `firmware/data-src/editor.js` — on widget select, populate panel from widget state
  - [ ] On control change, mutate widget, call `render()`, mark dirty

- [ ] **Task 3: Save / Save-as-New / Save & Activate** (AC: #3, #4, #5, #6)
  - [ ] Three buttons in control bar
  - [ ] Save as New: `FW.post('/api/layouts', body)` → store returned id, update dirty flag
  - [ ] Save: `FW.put('/api/layouts/' + id, body)`
  - [ ] Save & Activate: save path, then `FW.post('/api/layouts/' + id + '/activate')`
  - [ ] Toast sequence per AC #6 using `FW.showToast`

- [ ] **Task 4: Night Mode Preview** (AC: #7)
  - [ ] Toggle in control bar; adds a CSS class on canvas that drops brightness to ~20% via `filter: brightness(0.2)`

- [ ] **Task 5: Unsaved-changes guard** (AC: #8)
  - [ ] `dirty` flag set on any mutation, cleared on successful save
  - [ ] `beforeunload` handler prompts when dirty
  - [ ] Destructive controls (Load other / Delete) also gate on dirty

- [ ] **Task 6: E2E smoke test** (AC: #9)
  - [ ] Extend `tests/smoke/test_web_portal_smoke.py` with an editor scenario (or add a separate `test_editor_smoke.py`)
  - [ ] Asserts: GET /editor.html 200, POST /api/layouts 200, POST /api/layouts/:id/activate 200, GET /api/status returns `mode: custom_layout`
  - [ ] Provide a canned test-layout JSON body

- [ ] **Task 7: Gzip and upload** (AC: all)
  - [ ] `gzip -c data-src/editor.html > data/editor.html.gz`
  - [ ] `gzip -c data-src/editor.js > data/editor.js.gz`
  - [ ] `gzip -c data-src/editor.css > data/editor.css.gz`
  - [ ] Confirm total compressed remains ≤ 20 KB (LE-1.6 budget)

- [ ] **Task 8: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] `pio run -t uploadfs` succeeds
  - [ ] ES5 compliance verified: grep for arrow fns, `let`, `const`, template literals
  - [ ] Smoke test passes against live device (or documented as pending hardware)

## Dev Notes

**Sally's "honest editor" principles** — V1 deliberately restricts controls: only presets and palette, no freeform typography. Adding kerning/line-spacing/hex picker is out of scope for LE-1.

**Depends on:** LE-1.6 (canvas + drag-drop), LE-1.4 (REST API routes for save/activate).

**FW client helpers** (project CLAUDE.md): `FW.get()`, `FW.post()`, `FW.put?()` — confirm PUT helper exists; if not, add it to `common.js` as an ES5 function returning `{status, body}`.

**Activation timing target.** AC #5 requires the LED wall to update within 2 seconds of the activate POST returning. Measured end-to-end: HTTP round trip + ModeOrchestrator switch + first render ~= well under 2 s on ESP32 (existing mode switches are sub-500 ms).

## File List

- `firmware/data-src/editor.html` (modified)
- `firmware/data-src/editor.js` (modified)
- `firmware/data-src/editor.css` (modified)
- `firmware/data/editor.html.gz` (regenerated)
- `firmware/data/editor.js.gz` (regenerated)
- `firmware/data/editor.css.gz` (regenerated)
- `firmware/data-src/common.js` (modified if PUT helper missing)
- `tests/smoke/test_editor_smoke.py` (new) — or extension of existing smoke suite
