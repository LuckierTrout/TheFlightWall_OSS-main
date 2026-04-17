
# Story LE-1.6: Editor Canvas and Drag-Drop

branch: le-1-6-editor-canvas-and-drag-drop
zone:
  - firmware/data-src/editor.html
  - firmware/data-src/editor.js
  - firmware/data-src/editor.css
  - firmware/data-src/common.js
  - firmware/data/editor.html.gz
  - firmware/data/editor.js.gz
  - firmware/data/editor.css.gz
  - firmware/adapters/WebPortal.cpp

Status: draft

## Story

As a **layout author on my phone**,
I want **to drag and resize widgets on a canvas that honestly previews my LED wall**,
So that **I can build layouts that actually look good on the device**.

## Acceptance Criteria

1. **Given** a browser opens `http://flightwall.local/editor.html` **When** the page loads **Then** it is served from LittleFS (gzipped) by WebPortal **And** displays a 192×48 logical canvas rendered at 4× CSS scale (768×192 CSS pixels) with `imageSmoothingEnabled = false`.
2. **Given** the canvas is rendered **When** the device tile boundaries are inspected **Then** a ghosted grid overlays 10×2 tiles on 16-pixel boundaries (logical pixels).
3. **Given** `GET /api/widgets/types` returns widget metadata **When** the toolbox initializes **Then** each widget type appears as a draggable source item.
4. **Given** a user interacts with a widget **When** `pointerdown` / `pointermove` / `pointerup` fire **Then** drag and resize handlers update the widget's `x, y, w, h` and re-render the entire canvas on any change; canvas has `touch-action: none`.
5. **Given** snap controls **When** the user toggles modes **Then** default snap is 8 px, Tidy Mode snaps to 16 px (tile-aligned), Advanced snaps to 1 px.
6. **Given** a resize below a widget's minimum dimension **When** attempted **Then** the resize is refused **And** a toast or inline message explains the legibility floor (e.g., "Text widget cannot be shorter than 6 px").
7. **Given** the editor bundle is gzipped **When** measured **Then** `editor.html.gz + editor.js.gz + editor.css.gz` totals ≤ 20 KB compressed.
8. **Given** mobile Safari (iPhone) and Android Chrome **When** a user performs a drag **Then** the canvas is usable without native drag/selection interception (confirmed by manual smoke test on both platforms).
9. **Given** ES5 constraints **When** editor.js is inspected **Then** no arrow functions, no `let`/`const`, no template literals are present.

## Tasks / Subtasks

- [ ] **Task 1: Create editor HTML + CSS scaffold** (AC: #1, #2)
  - [ ] `firmware/data-src/editor.html` — skeleton with `<canvas id="editor" width="768" height="192">`, toolbox `<ul>`, property panel placeholder, control bar
  - [ ] `firmware/data-src/editor.css` — canvas styles (pixelated rendering, 4× scale), toolbox, `touch-action: none` on canvas, grid overlay styles

- [ ] **Task 2: Canvas render loop** (AC: #1, #2, #4)
  - [ ] `firmware/data-src/editor.js` — ES5 only. Module globals: `widgets[]`, `selectedIndex`, `snap`, `devicePixelRatio`
  - [ ] `render()` clears canvas, draws ghosted 16 px tile grid, draws each widget as a colored box with its type label
  - [ ] `imageSmoothingEnabled = false`; all coordinates snap to logical pixels

- [ ] **Task 3: Pointer Events drag + resize** (AC: #4, #5, #6)
  - [ ] Hand-rolled `pointerdown` / `pointermove` / `pointerup` handlers (no libraries)
  - [ ] Hit-test on pointerdown to set `selectedIndex`, determine drag vs. resize-handle mode
  - [ ] On move, compute delta in logical pixels, apply snap (8 / 16 / 1)
  - [ ] Enforce per-widget min-dimension using floors returned by `/api/widgets/types`; refuse sub-floor resize + toast

- [ ] **Task 4: Toolbox populated from API** (AC: #3)
  - [ ] `FW.get('/api/widgets/types')` on page load; render toolbox items
  - [ ] Tapping a toolbox item inserts a new widget at (8, 8) with default size

- [ ] **Task 5: Snap mode toggle UI** (AC: #5)
  - [ ] Three-segment control: Default (8) / Tidy (16) / Advanced (1)
  - [ ] Persist selection in `localStorage`

- [ ] **Task 6: WebPortal route for editor.html** (AC: #1)
  - [ ] `firmware/adapters/WebPortal.cpp` — add `/editor.html`, `/editor.js`, `/editor.css` routes serving gzipped LittleFS files
  - [ ] Captive-portal pattern already established; mirror it for editor assets

- [ ] **Task 7: Gzip the assets** (AC: #7)
  - [ ] `gzip -c data-src/editor.html > data/editor.html.gz`
  - [ ] `gzip -c data-src/editor.js > data/editor.js.gz`
  - [ ] `gzip -c data-src/editor.css > data/editor.css.gz`
  - [ ] Verify total ≤ 20 KB

- [ ] **Task 8: Mobile smoke test** (AC: #8)
  - [ ] Flash firmware + uploadfs, open on iPhone Safari, verify drag/resize works
  - [ ] Open on Android Chrome, verify the same
  - [ ] Document result in story file

- [ ] **Task 9: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] `pio run -t uploadfs` succeeds
  - [ ] `wc -c data/editor.*.gz` confirms ≤ 20 KB total
  - [ ] Grep confirms ES5 compliance: `rg "=>" data-src/editor.js` empty, no `let `/`const `/backticks

## Dev Notes

**ES5 constraints** (project CLAUDE.md): `FW.get()`, `FW.post()`, `FW.del()` return promises `{status, body}`. `FW.showToast(msg, severity)` for notifications. No arrow functions, no `let`/`const`, no template literals.

**Validated editor patterns** (`_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`):
- Hand-rolled Pointer Events (not hammer.js or similar).
- Single `<canvas>` at 4× CSS scale, `imageSmoothingEnabled = false`.
- Re-render entire canvas on any change (don't try incremental invalidation).

**Existing reference:** `_bmad-output/implementation-artifacts/stories/3-4-canvas-layout-preview-in-dashboard.md` — read-only canvas preview (done). This editor supersedes for custom layouts.

**Gzip pipeline** (project CLAUDE.md): no automated build script — gzip manually, upload via `pio run -t uploadfs`.

**Min-dimension floors from `/api/widgets/types`.** LE-1.2 declares the floors; LE-1.4 exposes them. The editor reads them at load to enforce legibility.

## File List

- `firmware/data-src/editor.html` (new)
- `firmware/data-src/editor.js` (new)
- `firmware/data-src/editor.css` (new)
- `firmware/data/editor.html.gz` (new — generated)
- `firmware/data/editor.js.gz` (new — generated)
- `firmware/data/editor.css.gz` (new — generated)
- `firmware/data-src/common.js` (modified — minor extensions if needed)
- `firmware/adapters/WebPortal.cpp` (modified — editor asset routes)
