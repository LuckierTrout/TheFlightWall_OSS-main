<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 6 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T200537Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story le-1.6

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

## BMAD / bmad-assist

- **`bmad-assist.yaml`** at repo root configures providers and phases; `paths.project_knowledge` points at `_bmad-output/planning-artifacts/`, `paths.output_folder` at `_bmad-output/`.
- **This file** (`project-context.md`) is resolved at `_bmad-output/project-context.md` or `docs/project-context.md` (see `bmad-assist` compiler `find_project_context_file`).
- Keep **`sprint-status.yaml`** story keys aligned with `.bmad-assist/state.yaml` (`current_story`, `current_epic`) when using `bmad-assist run` so phases do not skip with “story not found”.


]]></file>
<file id="92fbb450" path="_bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md" label="DOCUMENTATION"><![CDATA[


# Story LE-1.6: Editor Canvas and Drag-Drop

branch: le-1-6-editor-canvas-and-drag-drop
zone:
  - firmware/data-src/editor.html
  - firmware/data-src/editor.js
  - firmware/data-src/editor.css
  - firmware/data/editor.html.gz
  - firmware/data/editor.js.gz
  - firmware/data/editor.css.gz
  - firmware/adapters/WebPortal.cpp

Status: done

## Story

As a **layout author on my phone**,
I want **to drag and resize widgets on a canvas that honestly previews my LED wall**,
So that **I can build layouts that actually look good on the device without guessing pixel coordinates**.

## Acceptance Criteria

1. **Given** a browser opens `http://flightwall.local/editor.html` **When** the page loads **Then** it is served gzip-encoded from LittleFS by `WebPortal` with `Content-Encoding: gzip` and `Content-Type: text/html` **And** renders a canvas showing the correct device pixel dimensions obtained from `GET /api/layout` → `data.matrix.width` × `data.matrix.height` at 4× CSS scale **And** `imageSmoothingEnabled = false` is set on the canvas context.

2. **Given** the canvas is rendered **When** tile boundaries are inspected **Then** a ghosted grid (semi-transparent overlay) divides the canvas into tiles using `data.hardware.tile_pixels`-wide logical-pixel boundaries; the grid redraws whenever the canvas is redrawn.

3. **Given** `GET /api/widgets/types` is called on page load **When** the response is received **Then** each widget type entry in `data[]` produces one draggable toolbox item showing `entry.label`; any entry with a `note` field containing "not yet implemented" is shown with a visual disabled/muted style and cannot be dropped onto the canvas.

4. **Given** a user presses (`pointerdown`) on the canvas over an existing widget **When** they move (`pointermove`) and release (`pointerup`) **Then** the widget's `x` and `y` are updated by the delta in logical canvas pixels (pointer position ÷ CSS scale factor) **And** the canvas is fully re-rendered on every `pointermove` **And** the canvas element has `touch-action: none` set so the browser does not intercept the touch for scrolling.

5. **Given** a user presses on a resize handle (a 6×6 logical-pixel corner square at the bottom-right of the selected widget) **When** they drag **Then** the widget's `w` and `h` are updated by the delta; snap is applied (see AC #6); the canvas re-renders on every `pointermove`.

6. **Given** a snap mode control (three segments: "Default" / "Tidy" / "Fine") **When** the mode is "Default" **Then** all coordinate deltas snap to the nearest 8 logical pixels; **When** "Tidy" **Then** snap to 16 logical pixels (one tile boundary); **When** "Fine" **Then** snap to 1 logical pixel. **And** the selection persists in `localStorage` under key `fw_editor_snap`.

7. **Given** a resize operation would reduce a widget's `w` or `h` below its minimum floor **When** attempted **Then** the resize is clamped to the minimum floor **And** `FW.showToast` is called with an explanatory message (e.g. `"Text widget minimum height: 6 px"`). Minimum floors are the `h.default` value from the `/api/widgets/types` entry for each type (text: h=8, clock: h=8, logo: h=8).

8. **Given** the three gzip assets **When** measured after gzip-9 compression **Then** `editor.html.gz + editor.js.gz + editor.css.gz` total ≤ 20 KB compressed. (Current non-editor web assets total 62,564 bytes; LittleFS partition is 960 KB = 983,040 bytes; ample room exists.)

9. **Given** `editor.js` **When** inspected for ES5 compliance **Then** `grep -E "=>|let |const |` (backtick)` over `firmware/data-src/editor.js` returns empty — no arrow functions, no `let`, no `const`, no template literals.

10. **Given** mobile Safari (iPhone ≥ iOS 15) and Android Chrome (≥ 110) **When** a user performs a drag or resize **Then** the interaction completes correctly without native drag-selection or scroll interception (manual smoke test; document result in Dev Agent Record).

## Tasks / Subtasks

- [x] **Task 1: Create `editor.html` — page scaffold with canvas, toolbox, and control bar** (AC: #1, #2, #3)
  - [x]Create `firmware/data-src/editor.html` — standalone page (does NOT extend `dashboard.html`)
  - [x]`<head>`: charset UTF-8, viewport `width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no`, title "FlightWall Editor", `<link rel="stylesheet" href="/style.css">`, `<link rel="stylesheet" href="/editor.css">`
  - [x]`<body>` structure:
    ```html
    <div class="editor-layout">
      <header class="editor-header">
        <a href="/" class="editor-back">← Dashboard</a>
        <h1>Layout Editor</h1>
      </header>
      <div class="editor-main">
        <div class="editor-toolbox" id="editor-toolbox">
          <!-- populated by JS from /api/widgets/types -->
        </div>
        <div class="editor-canvas-wrap">
          <canvas id="editor-canvas"></canvas>
        </div>
        <div class="editor-props" id="editor-props">
          <!-- LE-1.7: property panel placeholder -->
          <p class="props-placeholder">Select a widget</p>
        </div>
      </div>
      <div class="editor-controls">
        <div class="snap-control" id="snap-control">
          <button type="button" class="snap-btn" data-snap="8">Default</button>
          <button type="button" class="snap-btn" data-snap="16">Tidy</button>
          <button type="button" class="snap-btn" data-snap="1">Fine</button>
        </div>
        <span class="editor-status" id="editor-status"></span>
      </div>
    </div>
    ```
  - [x]Script tags at bottom: `<script src="/common.js"></script>` then `<script src="/editor.js"></script>`
  - [x]**No inline scripts or styles**

- [x] **Task 2: Create `editor.css` — canvas and layout styles** (AC: #1, #2, #4)
  - [x]Create `firmware/data-src/editor.css`
  - [x]Canvas pixelated rendering: `#editor-canvas { image-rendering: pixelated; image-rendering: crisp-edges; touch-action: none; cursor: crosshair; }`
  - [x]Canvas CSS dimensions set to 4× the logical device dimensions (set dynamically by JS after `/api/layout` — do not hard-code in CSS; canvas element `width`/`height` attributes are set by JS too)
  - [x]Layout: `editor-layout` as a flex column; `editor-main` as a flex row with toolbox | canvas-wrap | props; controls bar at bottom
  - [x]Snap button active state: `.snap-btn.active { background: var(--accent, #58a6ff); color: #000; }`
  - [x]Toolbox item styles: `.toolbox-item { ... cursor: grab; }` and `.toolbox-item.disabled { opacity: 0.4; cursor: not-allowed; }`
  - [x]Mobile-friendly: on narrow viewports, toolbox becomes a horizontal row above the canvas

- [x] **Task 3: Create `editor.js` — module globals and canvas render loop** (AC: #1, #2, #9)
  - [x]Create `firmware/data-src/editor.js` — **ES5 only** (`var`, function declarations, no arrow functions, no `let`/`const`, no template literals, no `class`)
  - [x]Module globals at top of IIFE:
    ```javascript
    var SCALE = 4;                // CSS pixels per logical pixel
    var SNAP = 8;                 // current snap grid (8 / 16 / 1)
    var deviceW = 0;              // logical canvas width (from /api/layout)
    var deviceH = 0;              // logical canvas height
    var tilePixels = 16;          // tile size in logical pixels (from /api/layout hardware.tile_pixels)
    var widgets = [];             // [{type, x, y, w, h, color, content, align, id}]
    var selectedIndex = -1;       // index into widgets[], -1 = none
    var dragState = null;         // {mode: 'move'|'resize', startX, startY, origX, origY, origW, origH}
    var widgetTypeMeta = {};      // keyed by type string, value = entry from /api/widgets/types
    ```
  - [x]`render()` function — called on every state change:
    ```javascript
    function render() {
      var canvas = document.getElementById('editor-canvas');
      var ctx = canvas.getContext('2d');
      ctx.imageSmoothingEnabled = false;
      // 1. Clear
      ctx.fillStyle = '#111';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      // 2. Draw ghosted tile grid
      drawGrid(ctx);
      // 3. Draw widgets
      for (var i = 0; i < widgets.length; i++) {
        drawWidget(ctx, widgets[i], i === selectedIndex);
      }
    }
    ```
  - [x]`drawGrid(ctx)`: draws vertical and horizontal lines every `tilePixels * SCALE` CSS pixels using `ctx.strokeStyle = 'rgba(255,255,255,0.1)'` and `ctx.lineWidth = 1`
  - [x]`drawWidget(ctx, w, isSelected)`: draws a filled rectangle at `(w.x * SCALE, w.y * SCALE, w.w * SCALE, w.h * SCALE)` filled with the widget's color at 80% opacity; draws the type string label centered in the box; if `isSelected`, draws a 1-CSS-px white selection outline + a 6×6 CSS-px resize handle square at the bottom-right corner
  - [x]`initCanvas(matrixW, matrixH, tilePx)`: sets `deviceW`, `deviceH`, `tilePixels`, sets canvas `width` = `matrixW * SCALE`, `height` = `matrixH * SCALE`, sets CSS `width`/`height` in same values (no CSS scaling — the 4× expansion is in the logical canvas pixels themselves), calls `render()`
  - [x]On `DOMContentLoaded`: call `loadLayout()` which calls `FW.get('/api/layout')` then `FW.get('/api/widgets/types')` and initializes everything

- [x] **Task 4: Implement pointer events for drag and resize** (AC: #4, #5, #6, #7)
  - [x]`hitTest(cx, cy)`: given CSS coordinates, returns `{index, mode}` where `mode` is `'resize'` if the pointer is within 8 CSS px of the bottom-right corner of the selected widget, `'move'` if inside any widget bounding box, `null` if no hit
  - [x]`canvas.addEventListener('pointerdown', onPointerDown)`: call `hitTest`; if hit, set `selectedIndex`, set `dragState = {mode, startX: e.clientX, startY: e.clientY, origX: w.x, origY: w.y, origW: w.w, origH: w.h}`; call `canvas.setPointerCapture(e.pointerId)`; call `render()`
  - [x]`canvas.addEventListener('pointermove', onPointerMove)`: if `dragState == null` return; compute delta `dx = (e.clientX - dragState.startX) / SCALE`, `dy = (e.clientY - dragState.startY) / SCALE`; apply snap: `dx = Math.round(dx / SNAP) * SNAP`; apply to widget; clamp to canvas bounds; call `render()`
  - [x]`canvas.addEventListener('pointerup', onPointerUp)`: if `dragState == null` return; clear `dragState = null`; update status bar
  - [x]Min-floor enforcement in `onPointerMove` during resize: use `widgetTypeMeta[type].minH` and `widgetTypeMeta[type].minW` (see Task 6 for how these are derived from the API); if clamped, call `FW.showToast('...')` at most once per drag gesture (use a `toastedFloor` flag on `dragState`)
  - [x]**Critical**: canvas must have `touch-action: none` (set in CSS — Task 2). Also call `e.preventDefault()` inside `onPointerDown` to be belt-and-braces on iOS Safari.

- [x] **Task 5: Toolbox population from `/api/widgets/types`** (AC: #3)
  - [x]`initToolbox(types)`: iterates `types` array; for each entry, creates a `<div class="toolbox-item" data-type="...">LABEL</div>`; if `entry.note` contains "not yet implemented", add class `disabled`; append to `#editor-toolbox`
  - [x]Clicking a non-disabled toolbox item calls `addWidget(type)`:
    ```javascript
    function addWidget(type) {
      var meta = widgetTypeMeta[type];
      if (!meta) return;
      var w = {
        type: type,
        x: snapTo(8, SNAP),
        y: snapTo(8, SNAP),
        w: meta.defaultW,
        h: meta.defaultH,
        color: meta.defaultColor || '#FFFFFF',
        content: meta.defaultContent || '',
        align: meta.defaultAlign || 'left',
        id: 'w' + Date.now()
      };
      widgets.push(w);
      selectedIndex = widgets.length - 1;
      render();
    }
    ```
  - [x]`widgetTypeMeta` is populated in `loadLayout()` from the `GET /api/widgets/types` response; for each entry, store `{defaultW, defaultH, defaultColor, defaultContent, defaultAlign, minW, minH}` by reading the `fields` array (the field with `key === 'w'` → `default` value is `defaultW`, etc.)

- [x] **Task 6: Snap mode control** (AC: #6)
  - [x]On `DOMContentLoaded`, read `localStorage.getItem('fw_editor_snap')` → parse as integer; default to `8` if absent or invalid; set `SNAP` to the value; add `active` class to the matching `.snap-btn`
  - [x]Each `.snap-btn` click: read `data-snap` attribute, set `SNAP = parseInt(snapVal, 10)`, save to `localStorage.setItem('fw_editor_snap', SNAP)`, update button active states, call `render()`
  - [x]Helper `snapTo(val, grid)`: `return Math.round(val / grid) * grid`

- [x] **Task 7: Add WebPortal routes for editor assets** (AC: #1)
  - [x]Open `firmware/adapters/WebPortal.cpp`
  - [x]In `WebPortal::begin()` (the route registration block near line 886), after the existing static asset registrations, add:
    ```cpp
    _server->on("/editor.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.html.gz", "text/html");
    });
    _server->on("/editor.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.js.gz", "application/javascript");
    });
    _server->on("/editor.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.css.gz", "text/css");
    });
    ```
  - [x]**No other changes to `WebPortal.cpp`** — `_serveGzAsset()` already handles the `Content-Encoding: gzip` header and the `LittleFS.exists()` guard (line 1391–1399)
  - [x]**Do NOT** add `/editor` → `editor.html.gz` redirect or any captive-portal change — editor is a standalone page accessed only via direct URL

- [x] **Task 8: Gzip the assets** (AC: #8)
  - [x]From `firmware/` directory, run:
    ```bash
    gzip -9 -c data-src/editor.html > data/editor.html.gz
    gzip -9 -c data-src/editor.js   > data/editor.js.gz
    gzip -9 -c data-src/editor.css  > data/editor.css.gz
    ```
  - [x]Verify total: `wc -c data/editor.html.gz data/editor.js.gz data/editor.css.gz` — sum must be ≤ 20,480 bytes (20 KB)
  - [x]If sum exceeds 20 KB: first check for dead code or verbosity in `editor.js`; the 20 KB gate is achievable — dashboard.js source is 154 KB and compresses to 34,920 bytes; editor.js should be far smaller
  - [x]**Important**: gzip must be re-run every time `editor.html`, `editor.js`, or `editor.css` is modified. The `.gz` files in `firmware/data/` are what gets uploaded to the device.

- [x] **Task 9: Build verification and ES5 audit** (AC: #9)
  - [x]Run `~/.platformio/penv/bin/pio run -e esp32dev` from `firmware/` — must succeed with binary ≤ 1,382,400 bytes
  - [x]ES5 compliance grep: `grep -P "=>|`|(?<![a-zA-Z])let |(?<![a-zA-Z])const " firmware/data-src/editor.js` — must return empty
  - [x]`wc -c firmware/data/editor.html.gz firmware/data/editor.js.gz firmware/data/editor.css.gz` — confirm total ≤ 20,480 bytes
  - [x]`pio run -t uploadfs` — verify LittleFS image builds and uploads successfully

- [ ] **Task 10: Mobile smoke test** (AC: #10)
  - [ ]Flash device with new firmware + `uploadfs`; navigate to `http://flightwall.local/editor.html`
  - [ ]iPhone Safari (iOS 15+): drag a widget, verify no scroll interception, verify drag follows finger
  - [ ]Android Chrome (110+): same test
  - [ ]Document platform, OS version, test result, and any issues found in Dev Agent Record

---

## Dev Notes

### The Change Is Purely Web Assets — Read This First

LE-1.6 is a **web-only story**. Zero firmware C++ logic changes are required beyond adding 3 static route registrations in `WebPortal.cpp`. All existing API endpoints needed by the editor are already implemented:

| API | Story | Purpose |
|---|---|---|
| `GET /api/layout` | 3.1 | Canvas dimensions + tile size |
| `GET /api/widgets/types` | LE-1.4 | Widget palette + field schemas |
| `GET /api/layouts` | LE-1.4 | List saved layouts (LE-1.7 will use) |
| `POST /api/layouts` | LE-1.4 | Save new layout (LE-1.7 will use) |
| `PUT /api/layouts/:id` | LE-1.4 | Update layout (LE-1.7 will use) |
| `POST /api/layouts/:id/activate` | LE-1.4 | Activate layout |

**LE-1.6 uses `GET /api/layout` (canvas size) and `GET /api/widgets/types` (toolbox). Save/load is LE-1.7.**

---

### `GET /api/layout` — Canvas Dimension Source

The editor must call `GET /api/layout` on load to get the correct canvas dimensions. **Never hard-code 192×48** — the device config is user-changeable.

**Response shape** (implemented in `WebPortal.cpp` line 1348):
```json
{
  "ok": true,
  "data": {
    "mode": "full",
    "matrix": { "width": 192, "height": 48 },
    "logo_zone":      { "x": 0,  "y": 0, "w": 48, "h": 48 },
    "flight_zone":    { "x": 48, "y": 0, "w": 144, "h": 24 },
    "telemetry_zone": { "x": 48, "y": 24, "w": 144, "h": 24 },
    "hardware": { "tiles_x": 12, "tiles_y": 3, "tile_pixels": 16 }
  }
}
```

**Canvas initialization pattern:**
```javascript
FW.get('/api/layout').then(function(res) {
  if (!res.body.ok) { FW.showToast('Cannot load layout info', 'error'); return; }
  var d = res.body.data;
  initCanvas(d.matrix.width, d.matrix.height, d.hardware.tile_pixels);
});
```

`initCanvas` sets the canvas `width` and `height` attributes (logical pixel count × SCALE). The tile grid uses `d.hardware.tile_pixels` as the grid interval.

---

### `GET /api/widgets/types` — Toolbox Schema

Response shape (implemented in `WebPortal.cpp` line 2060; complete handler shown above in codebase read):
```json
{
  "ok": true,
  "data": [
    {
      "type": "text",
      "label": "Text",
      "fields": [
        { "key": "content", "kind": "string", "default": "" },
        { "key": "align",   "kind": "select", "default": "left", "options": ["left","center","right"] },
        { "key": "color",   "kind": "color",  "default": "#FFFFFF" },
        { "key": "x", "kind": "int", "default": 0 },
        { "key": "y", "kind": "int", "default": 0 },
        { "key": "w", "kind": "int", "default": 32 },
        { "key": "h", "kind": "int", "default": 8 }
      ]
    },
    {
      "type": "clock",
      "label": "Clock",
      "fields": [ ... "w" default 48, "h" default 8 ]
    },
    {
      "type": "logo",
      "label": "Logo",
      "fields": [ ... "w" default 16, "h" default 16 ]
    },
    {
      "type": "flight_field",
      "label": "Flight Field",
      "note": "LE-1.8 \u2014 not yet implemented, renders nothing",
      "fields": [ ... ]
    },
    {
      "type": "metric",
      "label": "Metric",
      "note": "LE-1.8 \u2014 not yet implemented, renders nothing",
      "fields": [ ... ]
    }
  ]
}
```

**Toolbox population pattern:**
```javascript
FW.get('/api/widgets/types').then(function(res) {
  if (!res.body.ok) return;
  var types = res.body.data;
  for (var i = 0; i < types.length; i++) {
    buildWidgetTypeMeta(types[i]);   // populate widgetTypeMeta map
    addToolboxItem(types[i]);        // create DOM element
  }
});
```

**`buildWidgetTypeMeta(entry)` — extract defaults from fields array:**
```javascript
function buildWidgetTypeMeta(entry) {
  var meta = { defaultW: 32, defaultH: 8, defaultColor: '#FFFFFF',
               defaultContent: '', defaultAlign: 'left',
               minW: 6, minH: 6 };
  if (!entry.fields) { widgetTypeMeta[entry.type] = meta; return; }
  for (var i = 0; i < entry.fields.length; i++) {
    var f = entry.fields[i];
    if (f.key === 'w')       meta.defaultW = f['default'] || 32;
    if (f.key === 'h')       meta.defaultH = f['default'] || 8;
    if (f.key === 'color')   meta.defaultColor = f['default'] || '#FFFFFF';
    if (f.key === 'content') meta.defaultContent = f['default'] || '';
    if (f.key === 'align')   meta.defaultAlign = f['default'] || 'left';
  }
  // Min floors — h.default is the firmware minimum (Adafruit GFX 5×7 font, 1-row floor)
  // w minimum: at least 6 px for any type (one character wide at 5+1 px spacing)
  meta.minH = meta.defaultH;  // firmware enforces w<8||h<8 → early return; use same floor
  meta.minW = 6;
  widgetTypeMeta[entry.type] = meta;
}
```

**Disabled toolbox items** (flight_field, metric): check `entry.note && entry.note.indexOf('not yet implemented') !== -1`.

---

### Canvas Scale Strategy — 4× Logical Pixels

The LED matrix is 192×48 logical pixels (typical config). This is tiny on a phone screen. The solution:
- Canvas `width` attribute = `matrixW * SCALE` = 768
- Canvas `height` attribute = `matrixH * SCALE` = 192
- All coordinates in `widgets[]` are stored in **logical pixels** (0–191, 0–47)
- All canvas drawing uses **CSS coordinates** = logical × SCALE
- All pointer event coordinates come in CSS pixels and must be divided by SCALE before storing

**CSS vs. logical conversion helpers:**
```javascript
function toCss(logical) { return logical * SCALE; }
function toLogical(css) { return css / SCALE; }
```

**Device pixel ratio:** Do NOT multiply canvas dimensions by `window.devicePixelRatio`. The 4× scale is intentional for legibility; HiDPI scaling would make the canvas 8× or 12× the LED dimensions — too large for mobile screens. Keep SCALE fixed at 4.

**`imageSmoothingEnabled = false`**: must be set after every call to `canvas.getContext('2d')` — browsers can reset it. Set it inside `render()` before any drawing:
```javascript
var ctx = canvas.getContext('2d');
ctx.imageSmoothingEnabled = false;
```

---

### Pointer Event Architecture — Hand-Rolled, No Libraries

From the spike report and architecture constraints: **no pointer/touch libraries** (hammer.js etc. are excluded by the LittleFS budget and no-frameworks rule).

**The three-handler pattern:**
```javascript
var canvas = document.getElementById('editor-canvas');
canvas.addEventListener('pointerdown', onPointerDown);
canvas.addEventListener('pointermove', onPointerMove);
canvas.addEventListener('pointerup',   onPointerUp);
// Also handle pointercancel to avoid stuck drag states on iOS:
canvas.addEventListener('pointercancel', onPointerUp);
```

**Pointer capture** (`canvas.setPointerCapture(e.pointerId)`) is critical for mobile — ensures `pointermove` events are delivered even when the finger moves off the canvas element. Call inside `onPointerDown` after setting `dragState`.

**iOS Safari quirk**: `touch-action: none` on the canvas element prevents native scroll/zoom. Additionally, `e.preventDefault()` inside `onPointerDown` provides belt-and-braces prevention on older iOS versions. Both are required (CSS alone is insufficient on some iOS 15 versions).

**Coordinate extraction for mobile:**
```javascript
function getCanvasPos(e) {
  var rect = e.target.getBoundingClientRect();
  return {
    x: e.clientX - rect.left,   // CSS pixels relative to canvas
    y: e.clientY - rect.top
  };
}
```

---

### Hit Testing — Resize vs. Move

The selected widget displays a 6×6 CSS-pixel resize handle at its bottom-right corner. Hit priority:
1. Check resize handle first (6×6 square at bottom-right of selected widget in CSS coordinates)
2. Check widget body (any widget, last-drawn is topmost)
3. Miss → deselect (set `selectedIndex = -1`)

**Resize handle position** (in CSS pixels):
```javascript
var handleSize = 6;  // CSS pixels
var rx = w.x * SCALE + w.w * SCALE - handleSize;
var ry = w.y * SCALE + w.h * SCALE - handleSize;
// Hit if: cx >= rx && cx <= rx+handleSize && cy >= ry && cy <= ry+handleSize
```

**Widget body hit** (in CSS pixels):
```javascript
// Check from last widget to first (topmost rendered = last in array)
for (var i = widgets.length - 1; i >= 0; i--) {
  var w = widgets[i];
  if (cx >= w.x * SCALE && cx <= (w.x + w.w) * SCALE &&
      cy >= w.y * SCALE && cy <= (w.y + w.h) * SCALE) {
    return { index: i, mode: 'move' };
  }
}
```

---

### Snap, Clamp, and Floor Logic

**Snap helper:**
```javascript
function snapTo(val, grid) {
  return Math.round(val / grid) * grid;
}
```

**Move clamping** (keep widget fully within canvas):
```javascript
w.x = Math.max(0, Math.min(deviceW - w.w, snapTo(newX, SNAP)));
w.y = Math.max(0, Math.min(deviceH - w.h, snapTo(newY, SNAP)));
```

**Resize clamping** (min floor + canvas boundary):
```javascript
var meta = widgetTypeMeta[w.type] || { minW: 6, minH: 6 };
var newW = Math.max(meta.minW, Math.min(deviceW - w.x, snapTo(dragState.origW + dw, SNAP)));
var newH = Math.max(meta.minH, Math.min(deviceH - w.y, snapTo(dragState.origH + dh, SNAP)));
if (newW !== w.w || newH !== w.h) {
  if ((snapTo(dragState.origW + dw, SNAP) < meta.minW ||
       snapTo(dragState.origH + dh, SNAP) < meta.minH) &&
      !dragState.toastedFloor) {
    FW.showToast(w.type + ' minimum: ' + meta.minW + 'w \u00d7 ' + meta.minH + 'h px', 'warning');
    dragState.toastedFloor = true;
  }
}
w.w = newW;
w.h = newH;
```

---

### WebPortal Route Pattern — Copy Exactly

The three new routes in `WebPortal.cpp` follow the exact same pattern as the existing static asset routes (lines 887–904). The `_serveGzAsset` helper (lines 1391–1399) handles:
- `LittleFS.exists(path)` check → 404 if missing
- `request->beginResponse(LittleFS, path, contentType)` — streaming from LittleFS
- `response->addHeader("Content-Encoding", "gzip")` — tells browser to decompress

**Insertion point**: after the `health.js` route block (line 903), before the closing `}` of `begin()`:
```cpp
// Layout Editor assets (LE-1.6)
_server->on("/editor.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.html.gz", "text/html");
});
_server->on("/editor.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.js.gz", "application/javascript");
});
_server->on("/editor.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.css.gz", "text/css");
});
```

**Captive portal is unaffected.** The captive portal only serves `/` → `wizard.html.gz` or `dashboard.html.gz`. `/editor.html` is a direct-access page, not served from the captive portal redirect.

---

### `_serveGzAsset` Is a Static Method — No `this`

Looking at the existing registrations (lines 887–904), the lambdas use `[](AsyncWebServerRequest* request)` — no `this` capture — because `_serveGzAsset` is a static method. Use the same `[]` capture (not `[this]`).

```cpp
// CORRECT — _serveGzAsset is static
_server->on("/editor.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.html.gz", "text/html");
});

// WRONG — do not capture [this]
_server->on("/editor.html", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.html.gz", "text/html");
});
```

---

### Gzip Command — Use `-9 -c` (not `-k`)

The correct gzip command per project convention:
```bash
# From firmware/ directory:
gzip -9 -c data-src/editor.html > data/editor.html.gz
gzip -9 -c data-src/editor.js   > data/editor.js.gz
gzip -9 -c data-src/editor.css  > data/editor.css.gz
```

- `-9`: maximum compression
- `-c`: write to stdout (keeps the source file unchanged)
- `>`: redirect to the `.gz` file in `data/`

**Do NOT use** `gzip -k` (which writes `.gz` alongside the source in `data-src/`).

The `.gz` files in `firmware/data/` are tracked by version control (they are the deployable artifacts). The source files in `firmware/data-src/` are also tracked. Both must be committed.

---

### LittleFS Budget Analysis

Current state (measured 2026-04-17):
- LittleFS partition: 960 KB = 983,040 bytes
- Existing web assets in `firmware/data/*.gz`: **62,564 bytes** total
- Logo files in `firmware/data/logos/`: ~404 KB (99 files × ~2 KB each)
- Remaining headroom: ~983,040 − 62,564 − 404,000 ≈ **516 KB**

Editor budget target (AC #8): editor.html.gz + editor.js.gz + editor.css.gz ≤ **20,480 bytes**. This leaves ~496 KB for logos and future assets. Very comfortable.

**Sanity check**: `dashboard.js` (154,698 bytes source) compresses to 34,920 bytes — a 4.4× compression ratio. `editor.js` will be much smaller than `dashboard.js`, so 20 KB compressed is achievable even at moderate compression.

---

### ES5 Constraint — No Exceptions

The project prohibits ES6+ syntax in all `firmware/data-src/*.js` files (CLAUDE.md, project convention). The ESP32 serves these files to any browser including old Android WebViews. Violations:

| FORBIDDEN | USE INSTEAD |
|---|---|
| `const x = ...` | `var x = ...` |
| `let x = ...` | `var x = ...` |
| `(x) => x + 1` | `function(x) { return x + 1; }` |
| `` `string ${var}` `` | `'string ' + var` |
| `class Foo { ... }` | `function Foo() { ... }` constructor pattern |
| `Array.from(...)` | `Array.prototype.slice.call(...)` |
| `Object.assign(...)` | manual property copy loop |

**`FW.get()`/`FW.post()`/`FW.del()`** are available from `common.js` (loaded before `editor.js`). They return Promises and resolve to `{status, body}`. **Promise itself is ES6** — but it is available in all target browsers (iOS 9+, Android 4.1+ WebView). The project already uses Promise in `dashboard.js` and `wizard.js`. Promises are allowed; arrow functions and `let`/`const` are not.

---

### `FW.showToast` Severity Values

```javascript
FW.showToast('message', 'success');  // green
FW.showToast('message', 'error');    // red
FW.showToast('message', 'warning'); // yellow/orange (if styled; falls back to error styling)
```

For the floor toast (AC #7), use `'warning'` or `'error'`. Use `FW.showToast(msg, 'error')` if `'warning'` is not styled in `style.css`.

---

### `common.js` — No Changes Required

`common.js` already provides everything `editor.js` needs: `FW.get()`, `FW.post()`, `FW.del()`, `FW.showToast()`. No modifications to `common.js` are needed for LE-1.6.

---

### Widget Color Rendering on Canvas

`WidgetSpec::color` is stored in layouts as a hex string (`"#FFFFFF"`, `"#0000FF"`). On the canvas, draw widgets using `ctx.fillStyle = w.color`. The canvas will interpret hex color strings natively. No RGB565 conversion needed for the editor preview — the editor is a web canvas, not the LED matrix.

**Widget preview coloring** — suggestion for clarity:
```javascript
function drawWidget(ctx, w, isSelected) {
  var x = w.x * SCALE, y = w.y * SCALE;
  var cw = w.w * SCALE, ch = w.h * SCALE;
  // Fill with widget color at 80% opacity
  ctx.globalAlpha = 0.8;
  ctx.fillStyle = w.color || '#444';
  ctx.fillRect(x, y, cw, ch);
  ctx.globalAlpha = 1.0;
  // Type label (white text, centered)
  ctx.fillStyle = '#FFF';
  ctx.font = (SCALE * 6) + 'px monospace';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText(w.type, x + cw / 2, y + ch / 2);
  // Selection outline
  if (isSelected) {
    ctx.strokeStyle = '#FFF';
    ctx.lineWidth = 1;
    ctx.strokeRect(x, y, cw, ch);
    // Resize handle
    ctx.fillStyle = '#FFF';
    ctx.fillRect(x + cw - 6, y + ch - 6, 6, 6);
  }
}
```

---

### `loadLayout()` — Initialization Sequence

Both API calls must complete before the canvas and toolbox are usable. Use sequential `.then()` chaining (no `Promise.all` — that is ES6 but available in all targets; however sequential chaining is cleaner and avoids any doubt):

```javascript
function loadLayout() {
  FW.get('/api/layout').then(function(res) {
    if (!res.body.ok) {
      FW.showToast('Cannot load layout info — check device connection', 'error');
      return;
    }
    var d = res.body.data;
    initCanvas(d.matrix.width, d.matrix.height, d.hardware.tile_pixels);

    return FW.get('/api/widgets/types');
  }).then(function(res) {
    if (!res || !res.body.ok) return;
    initToolbox(res.body.data);
    updateSnapButtons();
  });
}
```

---

### Antipattern Prevention Table

| DO NOT | DO INSTEAD | Reason |
|---|---|---|
| `canvas.width = 768` (hard-coded) | `canvas.width = d.matrix.width * SCALE` | Device config varies; always read from `/api/layout` |
| `let`, `const`, arrow functions | `var`, `function` | ES5 constraint — project rule; grep gate AC #9 |
| Import any JS library | Use hand-rolled pointer events | LittleFS budget; no-frameworks rule |
| `window.devicePixelRatio` scaling | Fixed 4× SCALE | HiDPI scaling oversizes the canvas on retina phones |
| `ctx.imageSmoothingEnabled = false` once at init | Set it inside `render()` every time | Browsers can reset this property |
| Lambda `[this]` capture in WebPortal routes | Lambda `[]` (no capture) | `_serveGzAsset` is static; `this` is unused |
| `gzip -k data-src/editor.js` | `gzip -9 -c data-src/editor.js > data/editor.js.gz` | `-k` writes to wrong directory |
| Save layout JSON in LE-1.6 | Defer to LE-1.7 | Save/load UX is LE-1.7's scope; LE-1.6 is canvas + drag only |
| `e.preventDefault()` globally on body | `e.preventDefault()` inside `onPointerDown` only | Global preventDefault breaks scrolling on the rest of the page |
| Commit only `.gz` files without source | Commit both `data-src/*.{html,js,css}` and `data/*.gz` | Source is the truth; `.gz` are derived artifacts both tracked |

---

### File Size Targets (Informational)

| File | Source estimate | Compressed target |
|---|---|---|
| `editor.html` | ~3 KB | ~1 KB |
| `editor.js` | ~6–10 KB | ~3–5 KB |
| `editor.css` | ~2–3 KB | ~1 KB |
| **Total** | **~11–16 KB** | **≤ 20 KB** |

For reference, `dashboard.html` is 16,753 bytes source → 4,031 bytes compressed. The editor HTML will be simpler than dashboard. `dashboard.js` is 154,698 bytes → 34,920 bytes. `editor.js` will be far smaller (no settings forms, no IANA timezone map, no OTA, no schedule logic).

---

### Sprint Status Update

After all tasks pass, update `_bmad-output/implementation-artifacts/sprint-status.yaml`:
```yaml
le-1-6-editor-canvas-and-drag-drop: done
```

---

## File List

Files **created**:
- `firmware/data-src/editor.html` — editor page scaffold
- `firmware/data-src/editor.js` — canvas render loop + pointer events (ES5)
- `firmware/data-src/editor.css` — canvas and layout styles
- `firmware/data/editor.html.gz` — gzip-compressed HTML (generated, tracked)
- `firmware/data/editor.js.gz` — gzip-compressed JS (generated, tracked)
- `firmware/data/editor.css.gz` — gzip-compressed CSS (generated, tracked)

Files **modified**:
- `firmware/adapters/WebPortal.cpp` — add 3 static routes (`/editor.html`, `/editor.js`, `/editor.css`) in `begin()`

Files **NOT modified** (verify these are unchanged before committing):
- `firmware/data-src/common.js` — no extensions needed; `FW.get/post/del/showToast` already sufficient
- `firmware/data-src/dashboard.html` — editor is a separate page
- `firmware/data-src/dashboard.js` — no changes
- `firmware/core/WidgetRegistry.h` — no changes
- `firmware/core/LayoutStore.*` — no changes
- `firmware/modes/CustomLayoutMode.cpp` — no changes
- Any firmware `.cpp`/`.h` other than `WebPortal.cpp`

---

## Change Log

| Date | Version | Description | Author |
|---|---|---|---|
| 2026-04-17 | 0.1 | Draft story created (9 AC, 9 tasks, minimal dev notes) | BMAD |
| 2026-04-17 | 1.0 | Upgraded to ready-for-dev. Key additions: (1) Full `/api/layout` response shape documented with actual WebPortal.cpp line references; (2) Complete `GET /api/widgets/types` response shape documented with all 5 widget type schemas showing actual default values from WebPortal.cpp; (3) Canvas scale strategy documented with conversion helpers; (4) Pointer event architecture documented with iOS Safari `pointercancel` requirement; (5) Hit-test algorithm documented with resize-handle priority; (6) `_serveGzAsset` static method + `[]` capture requirement documented; (7) Gzip command corrected to `-9 -c ... >` pattern; (8) LittleFS budget analysis documented (516 KB remaining, well within limits); (9) Complete `buildWidgetTypeMeta()` helper for deriving min-floor and defaults from API fields; (10) `loadLayout()` initialization sequence with chained Promises; (11) ES5 constraint table with full forbidden/allowed comparison; (12) Antipattern prevention table (9 entries); (13) AC expanded from 9 to 10 with explicit mobile smoke test separation. Tasks expanded from 9 to 10 with Task 10 as explicit smoke test capture. Status changed from draft to ready-for-dev. | BMAD Story Synthesis |
| 2026-04-17 | 2.0 | Implementation complete. Created `editor.html` (1.2 KB), `editor.css` (2.7 KB), `editor.js` (9.0 KB ES5 IIFE). Added 3 GET routes to `WebPortal.cpp`. Gzip total = 5,063 bytes (24.7% of 20 KB budget). `pio run -e esp32dev` SUCCESS at 82.8% flash. ES5 grep clean. AC 1–9 verified; AC 10 (mobile smoke test) deferred to on-device session. Status → done. | Dev Agent |

---

## Dev Agent Record

### Implementation Plan

Implemented in strict task order per the story:
1. `editor.html` — page scaffold with explicit body structure from Task 1.
2. `editor.css` — pixelated canvas, `touch-action:none`, snap-button active state, mobile responsive collapse.
3. `editor.js` — single ES5 IIFE; module globals at top; `render()` → `drawGrid()` + `drawWidget()`; `initCanvas()`; pointer events with `setPointerCapture`; toolbox driven by `/api/widgets/types`; snap control with `localStorage` persistence under `fw_editor_snap`.
4. `WebPortal.cpp` — three GET routes added immediately after the `health.js` route, using `[]` capture (no `this`) since `_serveGzAsset` is static.
5. Gzipped via `gzip -9 -c data-src/X > data/X.gz` from `firmware/`.
6. Build + ES5 grep verified.

### Key Decisions

- **`SCALE = 4` is fixed**, not multiplied by `devicePixelRatio` — keeps the canvas legible on phones without becoming oversized on retina displays (per Dev Notes).
- **Hit-test priority**: resize handle of the *currently selected* widget is checked first; then widget bodies in topmost-first order; otherwise miss → deselect. This makes the 6-px handle reachable without it being shadowed by the body hit.
- **`pointercancel` mapped to `onPointerUp`** to avoid stuck drag states on iOS Safari (per Dev Notes).
- **Min-floor toast fires once per gesture** via `dragState.toastedFloor` flag to prevent toast spam during a sustained resize that holds at the floor.
- **`buildWidgetTypeMeta`** uses bracket access `f['default']` because `default` is an ES5 reserved-word property name access pattern (still allowed via dot, but bracket is safer for legacy parsers and matches the story's Dev Notes example).
- **Snap value validation** restricted to the legitimate set `{1, 8, 16}` when reading both `localStorage` and click handlers — guards against tampered storage.
- **Both source and `.gz` are committed**, per project convention; sources are the truth, `.gz` are derived but version-tracked deployment artifacts.
- **No changes to `common.js`** — its `FW.get/showToast` API was already sufficient; touching it would bloat scope.

### Completion Notes

| AC | Status | Verification |
|---|---|---|
| 1. `/editor.html` served gzip-encoded with correct dimensions from `/api/layout` at 4× scale; `imageSmoothingEnabled = false` | ✅ | Route added in `WebPortal.cpp` lines 905–914; `initCanvas()` reads `d.matrix.width`/`height` and `d.hardware.tile_pixels`; `render()` sets `imageSmoothingEnabled = false` on every redraw. |
| 2. Ghosted tile grid using `tile_pixels` boundaries | ✅ | `drawGrid()` strokes `rgba(255,255,255,0.1)` lines every `tilePixels * SCALE` CSS px; called inside `render()`. |
| 3. Toolbox from `/api/widgets/types`; `note` containing "not yet implemented" → disabled | ✅ | `initToolbox()` calls `isDisabledEntry()` and adds `disabled` class; `onToolboxClick()` short-circuits on disabled items. |
| 4. Drag updates x/y by delta ÷ SCALE; canvas re-renders on every move; `touch-action: none` set | ✅ | `onPointerMove()` divides `dxCss/SCALE`; `render()` called every move; CSS sets `touch-action: none`; `e.preventDefault()` belt-and-braces in `onPointerDown`. |
| 5. 6×6 corner resize handle resizes; canvas re-renders | ✅ | `HANDLE_SIZE = 6`; `hitTest` returns `mode='resize'` when within handle bounds; `onPointerMove()` computes `newW`/`newH` and rerenders. |
| 6. Snap modes 8/16/1 with localStorage persistence under `fw_editor_snap` | ✅ | `initSnap()` reads `localStorage.getItem('fw_editor_snap')`, validates ∈ {1,8,16}, defaults to 8; click handler writes `setItem` and updates active class. |
| 7. Min-floor clamp + `FW.showToast` once per gesture | ✅ | Resize branch in `onPointerMove()` clamps via `clamp(rawW, meta.minW, maxW)`; toasts only when `rawW<minW || rawH<minH` and `!toastedFloor`, then sets the flag. |
| 8. ≤ 20 KB total gzip | ✅ | `wc -c` total = **5,063 bytes** (569 + 3,582 + 912). 24.7% of budget. |
| 9. ES5 audit grep returns empty | ✅ | `grep -nE '=>|\`|(^\|[^a-zA-Z_])let \|(^\|[^a-zA-Z_])const ' firmware/data-src/editor.js` → no matches. |
| 10. Mobile smoke test (manual) | ⏭ | Deferred — manual on-device test required after firmware flash + uploadfs. Captured below in Mobile Smoke Test section. |

**Build verification**:
- `pio run -e esp32dev`: SUCCESS — Flash: 1,302,672 / 1,572,864 bytes (82.8% — within the 1,382,400 gate).
- `pio run -e esp32dev -t buildfs`: SUCCESS — LittleFS image includes `/editor.html.gz`, `/editor.js.gz`, `/editor.css.gz` alongside existing assets.

### Files Modified

- `firmware/adapters/WebPortal.cpp` — added 3 GET route registrations at the end of `WebPortal::begin()` (lines 905–914) for `/editor.html`, `/editor.js`, `/editor.css`. No other lines changed.
- `_bmad-output/implementation-artifacts/sprint-status.yaml` — `le-1-6-editor-canvas-and-drag-drop` flipped from `in-progress` to `done`.

### Files Created

- `firmware/data-src/editor.html` (1,234 bytes source)
- `firmware/data-src/editor.css` (~2.7 KB source)
- `firmware/data-src/editor.js` (~9.0 KB source)
- `firmware/data/editor.html.gz` (569 bytes)
- `firmware/data/editor.js.gz` (3,582 bytes)
- `firmware/data/editor.css.gz` (912 bytes)

### Files Verified Unchanged

- `firmware/data-src/common.js` — unchanged; `FW.get/post/del/showToast` were already sufficient.
- `firmware/data-src/dashboard.html`, `dashboard.js`, `wizard.html`, `wizard.js`, `health.html`, `health.js`, `style.css` — unchanged.
- `firmware/core/WidgetRegistry.h`, `firmware/core/LayoutStore.*`, `firmware/modes/CustomLayoutMode.cpp` — unchanged.
- All `firmware/*.cpp`/`.h` other than `firmware/adapters/WebPortal.cpp` — unchanged.

### Mobile Smoke Test (AC #10)

⏭ **Pending hardware**: Cannot be executed in this implementation session — requires physical ESP32 device with firmware flashed and `uploadfs` completed, then access via iPhone Safari (iOS 15+) and Android Chrome (110+).

Test script for the operator to run after flashing:
1. `pio run -e esp32dev -t upload && pio run -e esp32dev -t uploadfs`
2. Wait for device boot, navigate phone browser to `http://flightwall.local/editor.html`.
3. Verify canvas renders with the correct device dimensions and the ghosted tile grid is visible.
4. Tap a non-disabled toolbox item (e.g. "Text") → verify a widget appears at (8, 8).
5. Drag the widget with one finger; confirm the page does not scroll and the widget follows the finger.
6. Tap a corner resize handle and drag; confirm the widget resizes; drag toward zero to verify the floor toast fires (once).
7. Tap "Tidy" snap mode, drag again; confirm 16-px snap. Reload the page; confirm "Tidy" remains active (localStorage persistence).
8. Repeat steps 4–7 on Android Chrome.

Document results in this section after on-device verification.

### Sprint Status

`_bmad-output/implementation-artifacts/sprint-status.yaml` updated: `le-1-6-editor-canvas-and-drag-drop: done`.

---

## Senior Developer Review (AI)

### Review: 2026-04-17
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 3.6 (weighted across 3 reviewers; majority findings dismissed as false positives) → **Approved with Reservations**
- **Issues Found:** 3 verified (1 medium, 2 low)
- **Issues Fixed:** 2 (releasePointerCapture in editor.js; editor smoke tests added to test_web_portal_smoke.py)
- **Action Items Created:** 1

#### Review Follow-ups (AI)

- [ ] [AI-Review] LOW: AC10 (mobile smoke test) must be executed on real hardware (iPhone Safari iOS 15+ and Android Chrome 110+) with results documented in Dev Agent Record. Story status should remain `done` only after this test passes. (`_bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md` Dev Agent Record → Mobile Smoke Test section)


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic le-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story le-1-1 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `load()` unbounded `readString()` — if a layout file is larger than `LAYOUT_STORE_MAX_BYTES` (e.g., manual upload or corruption), `f.readString()` allocates the full file to heap, risking OOM on ESP32. | Added `f.size() > LAYOUT_STORE_MAX_BYTES` guard before `readString()`; falls back to default and logs. |
| high | `list()` unbounded `readString()` in per-entry loop — same heap risk as above, amplified up to 16× in the worst case. | Wrapped the `readString()`+`JsonDocument` block in `if (entry.size() <= LAYOUT_STORE_MAX_BYTES)` guard; oversized entries still appear in results with empty `name`. |
| high | `_default` id not blocked — `isSafeLayoutId("_default")` returns `true` because underscore is allowed, allowing a caller to write `_default.json` which would shadow the baked-in fallback. | Added `if (strcmp(id, "_default") == 0) return false;` with explanatory comment. |
| high | Document `id` vs filename desync — `save("foo", {"id":"bar",...})` succeeds, creating `foo.json` whose JSON claims it is `bar`. Downstream active-id / UI reads the filename as truth but the JSON disagrees. | `validateSchema()` signature extended to `validateSchema(json, pathId)`; rejects if `doc["id"] != pathId`. Call site in `save()` updated. |
| medium | AC6 log messages omit the offending id — AC6 specifies `"layout_active '<id>' missing — using default"` but implementation logged generic fixed strings. | Replaced `LOG_W` (macro accepts only string literals; no variadic support) with `Serial.printf` for all four `load()` warning paths, embedding `id` and file size as appropriate. |
| medium | No AC5 NVS round-trip test — `setActiveId`/`getActiveId` had zero Unity coverage despite being a complete AC. | Added `test_active_id_roundtrip` exercising write→read→re-write→read with `LayoutStore::{set,get}ActiveId`. |
| medium | No AC6 end-to-end test (active-id → missing file → default) — the path where `getActiveId()` returns a stale NVS value and `load()` falls back was not tested. | Added `test_load_with_missing_active_id_uses_default` using `ConfigManager::setLayoutActiveId("nonexistent")` then asserting `load()` returns false and out contains `"_default"`. |
| medium | No test asserting `_default` cannot be saved (reservation). | Added `test_save_rejects_reserved_default_id`. |
| medium | No test for doc id vs filename mismatch (new validation rule). | Added `test_save_rejects_id_mismatch`. |
| dismissed | "CRITICAL: V0 cleanup unverifiable — main.cpp incomplete, no CI output" | FALSE POSITIVE: `grep -r "LE_V0_METRICS\\|le_v0_record\\|v0_spike_layout" firmware/src firmware/core firmware/modes` returns zero results (verified in synthesis environment). `git status` confirms `main.cpp` was modified. Binary size 1.22 MB is consistent with V0 instrumentation removal. The reviewer was working from a truncated code snippet but the actual file is clean. |
| dismissed | "Widget nullptr edge case — `(const char*)nullptr` cast is a logic error" | FALSE POSITIVE: `isAllowedWidgetType(nullptr)` returns `false` (line 89 of `LayoutStore.cpp`), causing `validateSchema` to return `INVALID_SCHEMA`. This is the correct behavior. The concern is purely stylistic — the logic is sound. |
| dismissed | "AC #5 documentation error — 12 vs 13 chars" | FALSE POSITIVE: The Dev Agent Record already acknowledges this discrepancy and notes 13 chars is within the 15-char NVS limit. The implementation is correct; only the AC text has a benign counting error. This is a story documentation note, not a code defect. |
| dismissed | "Redundant `LittleFS.exists(LAYOUTS_DIR)` check in `save()`" | FALSE POSITIVE: The inline comment at line 241-247 explains the rationale: "fresh devices reach save() before init() only via tests that explicitly skip init()". This is intentional defensive coding that protects against test harness misuse. The single `exists()` call has negligible performance impact. |
| dismissed | "`setLayoutActiveId` return check — partial write risk from `written == strlen(id)`" | FALSE POSITIVE: `LayoutStore::setActiveId()` (the only caller) pre-validates id with `isSafeLayoutId()` which rejects empty strings and enforces `strlen > 0`. Empty-string NVS writes are structurally prevented at the public API boundary. The theoretical partial-write scenario would require `putString` to return a value inconsistent with actual NVS behaviour, which is platform-specific noise not an application bug. |
| dismissed | "JsonDocument not guaranteed freed in `list()` loop — memory leak risk" | FALSE POSITIVE: `JsonDocument doc` is stack-allocated within the loop body. C++ RAII guarantees destruction at end of scope regardless of exit path (including `continue`, exceptions are not used in this codebase). The added size guard in the synthesis fix further reduces the window in which a `JsonDocument` is allocated at all. |
| dismissed | "`isNull()` vs explicit null edge cases are surprising in ArduinoJson v7" | FALSE POSITIVE: `hasRequiredTopLevelFields` correctly uses `isNull()` to detect missing or null keys. The ArduinoJson v7 documentation explicitly states `isNull()` returns true for missing keys. The hypothetical `"widgets": null` JSON input would correctly fail the `!doc["widgets"].isNull()` check. Low-value concern for this codebase. --- |

## Story le-1-2 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `ClockWidget` does not advance `s_lastClockUpdateMs` when `getLocalTime()` fails. With NTP unreachable, the refresh condition `(s_lastClockUpdateMs == 0 | Moved `s_lastClockUpdateMs = nowMs` outside the `if (getLocalTime...)` branch so it advances on both success and failure. |
| medium | `DisplayUtils::drawBitmapRGB565()` had a stale comment "Bitmap is always 32×32 (LOGO_WIDTH x LOGO_HEIGHT)" — the function signature accepts arbitrary `w`, `h` dimensions and LogoWidget passes 16×16. The incorrect comment degrades trust for onboarding readers. | Rewrote comment to "Render an RGB565 bitmap (w×h pixels)… Bitmap dimensions are caller-specified (e.g. 16×16 for LogoWidget V1 stub)." |
| low | `test_clock_cache_reuse` accepted `getTimeCallCount() <= 1` but count `0` trivially satisfies that assertion even when 50 calls all hit the refresh branch (failure storm). After the clock throttle bug was fixed, the test structure should confirm the throttle actually fires. | Restructured to two phases — Phase 1 (50 calls from cold state: count ≤ 1), Phase 2 (reset, arm cache on first call, 49 more calls: count still ≤ 1) — making the test meaningful regardless of NTP state. |
| low | `drawBitmapRGB565` skips pixels where `pixel == 0x0000` (treating black as transparent). A `spec.color = 0x0000` logo stub renders as invisible. This is undocumented surprising behaviour for callers. | Added inline NOTE comment documenting the black-as-transparent behaviour and why it does not affect LE-1.5 real bitmaps. |

## Story le-1-2 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | `ClockWidgetTest` namespace compiled unconditionally into production firmware | Gated both the header declaration and the `.cpp` implementation behind `#ifdef PIO_UNIT_TESTING` so the test-introspection API is stripped from production binary builds. |
| low | Stale/incorrect comment in `WidgetRegistry.cpp` claiming "The final `default` only exists to handle WidgetType::Unknown..." when no `default:` label exists in the switch | Rewrote both the file-header comment (line 5–7) and the inline dispatch comment to accurately describe that `WidgetType::Unknown` has its own explicit `case`, and that out-of-range values fall through to the post-switch `return false` — valid C++, no UB. |
| low | Single global clock cache is a V1 limitation invisible to LE-1.3 implementors — two Clock widgets in one layout silently share state | Added a clearly-labelled `V1 KNOWN LIMITATION` block in the file header documenting the shared cache behavior and the LE-1.7+ migration path. |

## Story le-1-2 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | Stale `~XXX,XXX bytes / ~YY,YYY bytes` placeholder in `DisplayMode.h` heap baseline comment degrades onboarding trust | Replaced `~XXX,XXX bytes / Max alloc: ~YY,YYY bytes` placeholder values and `(Update with actual values...)` instruction with a cleaner comment that still communicates "not yet measured" without the template noise. |
| dismissed | AC#3 violated — DisplayMode uses virtual methods, contradicting "no vtable per widget" | FALSE POSITIVE: AC#3 is explicitly scoped to the *WidgetRegistry dispatch mechanism* ("not virtual-function vtable per widget"). `DisplayMode` is a pre-existing, pre-LE-1.2 abstract interface for the mode system — entirely separate from the widget render dispatch path. LE-1.2 dev notes explicitly designate `DisplayMode.h` as "Untouched". The `WidgetRegistry::dispatch()` switch confirmed zero virtual calls. The AC language "per widget" is unambiguous. |
| dismissed | Task 8 grep validation is misleadingly narrow (missing `DisplayMode.h` in scope) | FALSE POSITIVE: Task 8's grep scope (`firmware/core/WidgetRegistry.cpp firmware/widgets/`) is intentionally narrow and correct — it verifies that LE-1.2's new widget code has no virtual dispatch. `DisplayMode.h` pre-dates LE-1.2 and is out of the story zone. The completion claim is accurate for the stated purpose. |
| dismissed | ClockWidget NTP throttle bug still present despite synthesis fix | FALSE POSITIVE: Reviewer C self-corrected this to FALSE POSITIVE upon re-reading the code. Confirmed: `s_lastClockUpdateMs = nowMs` at line 62 of `ClockWidget.cpp` is correctly placed *outside* the `if (getLocalTime...)` block, advancing the timer on both success and failure. |
| dismissed | platformio.ini missing `-I widgets` build flag | FALSE POSITIVE: `firmware/platformio.ini` line 43 shows `-I widgets` is present. Direct source verification confirms the flag is there. False positive. |
| dismissed | AC#8 unverifiable — test file not provided | FALSE POSITIVE: `firmware/test/test_widget_registry/test_main.cpp` is fully visible in the review context (275 lines, complete). The two-phase `test_clock_cache_reuse` is implemented exactly as the synthesis round 2 record describes, with `TEST_ASSERT_LESS_OR_EQUAL(1u, ...)` assertions in both phases. False positive. |
| dismissed | AC#7 "silent no-op on undersized widgets" violates the clipping requirement | FALSE POSITIVE: AC#7 requires "clips safely and does not write out-of-bounds." Silent no-op (returning `true` without drawing) IS the safest possible clip for an entire dimension below the font floor. The AC does not mandate partial rendering. All three widget implementations (Text, Clock, Logo) correctly return `true` as a no-op for below-floor specs. Fully compliant. |
| dismissed | LogoWidget `spec.color = 0x0000` renders invisible — undocumented gotcha | FALSE POSITIVE: Already addressed in a prior synthesis round. `LogoWidget.cpp` lines 39–42 contain an explicit `NOTE:` comment documenting the black-as-transparent behavior and explaining LE-1.5 is unaffected. The antipatterns table also documents this. Nothing new to fix. |
| dismissed | Widget `id` fields could collide with ConfigManager NVS keys (e.g., widget `id="timezone"`) | FALSE POSITIVE: Architectural confusion. Widget `id` fields (e.g., `"w1"`) are JSON properties stored inside layout documents on LittleFS. They are never written to NVS. ConfigManager's NVS keys (`"timezone"`, `"disp_mode"`, etc.) are entirely separate storage. The two namespaces cannot collide. LayoutStore's `isSafeLayoutId()` governs layout *file* identifiers, not widget instance ids within a layout. False positive. |
| dismissed | `test_text_alignment_all_three` is a "lying test" — only proves no-crash, not pixel math | FALSE POSITIVE: Valid observation, but by-design. The test file header explicitly labels this as a "smoke test" (line 212: "pixel-accurate assertions require a real framebuffer which we don't have in the test env"). The null-matrix scaffold is the documented test contract for all hardware-free unit tests in this project (see dev notes → Test Infrastructure). Alignment math IS present in `TextWidget.cpp` (lines 53–59) and is exercised on real hardware via the build verification. No lying — the smoke test boundary is honest and documented. |
| dismissed | `test_clock_cache_reuse` is weak — count=0 satisfies `≤1` even when cache is broken | FALSE POSITIVE: The two-phase test structure addresses this. Phase 2 explicitly arms the cache first (call 1), then runs 49 more calls — proving the cache throttle fires within a single `millis()` window. On NTP-down rigs, `getTimeCallCount()` = 0 for all 50 calls is still meaningful: it proves `getLocalTime()` was not called 50 times. The prior synthesis already restructured the test and the comment in the test explains this reasoning explicitly at line 159–161. |
| dismissed | `dispatch()` ignores `spec.type` — `(type, spec)` mismatch silently renders wrong widget | FALSE POSITIVE: Design choice, not a bug. The header comment at lines 89–91 of `WidgetRegistry.h` explicitly documents: "type is passed explicitly (rather than reading spec.type) to allow future callers to force a type without mutating the spec." LE-1.3 will populate specs from JSON; the type will always match. If a mismatch is desired for testing or fallback, this design supports it. A `debug-assert` could be added in the future but is out of LE-1.2 scope. |
| dismissed | `LayoutStore` / `WidgetRegistry` dual-source type string sync bomb | FALSE POSITIVE: Acknowledged design limitation. Already documented in `WidgetRegistry.cpp` (lines 11–13: "The allowlist here MUST stay in lock-step with LayoutStore's kAllowedWidgetTypes[]"). Centralization is a valid future improvement but is explicitly out of LE-1.2 scope per the story's architecture notes. Not a bug in the current implementation. |
| dismissed | `delay(2000)` in `setup()` slows every on-device test run | FALSE POSITIVE: Standard ESP32 Unity practice — the 2s delay allows the serial monitor to connect before test output begins. Removing it risks losing the first test results on some host configurations. Consistent with the `test_layout_store` scaffold which this test mirrors. Low-impact, by-convention. |
| dismissed | Include order in `LogoWidget.cpp` (comments before includes is unconventional) | FALSE POSITIVE: Style-only. The pattern (file header comment → includes) is consistent with `TextWidget.cpp`, `ClockWidget.cpp`, and `WidgetRegistry.cpp`. Not an inconsistency — it IS the project's established pattern. No change warranted. --- |

## Story le-1-3 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #7 is functionally broken — `ModeRegistry::requestSwitch("custom_layout")` is a no-op when `custom_layout` is already the active mode. `ModeRegistry::tick()` lines 408–411 contain an explicit same-mode idempotency guard: `if (requested != _activeModeIndex)` — the else branch just resets state to IDLE without calling `executeSwitch()`. Layout never reloads. | Added `ModeRegistry::requestForceReload()` to `ModeRegistry.h/.cpp` which atomically clears `_activeModeIndex` to `MODE_INDEX_NONE` before storing the request index, forcing `tick()` to take the `executeSwitch` path. Updated `main.cpp` hook to call `requestForceReload` instead of `requestSwitch`. |
| high | `ConfigManager::setLayoutActiveId()` does not call `fireCallbacks()`, so changing the active layout id in NVS never sets `g_configChanged`. This means the entire AC #7 re-init chain never fires even if the same-mode guard were fixed — the display task's `_cfgChg` is never `true` after a layout activation. | Added `fireCallbacks()` call in `setLayoutActiveId()` after successful NVS write. Also tightened the return path — previously returned `true` even on partial write; now returns `false` if `written != strlen(id)` (which was already the boolean expression but was lost in the refactor to add the callback). |
| medium | Misleading `_activeModeIndex` write in `requestForceReload()` races with Core 0 reading it between the two stores. Analysis: both `_activeModeIndex` and `_requestedIndex` are `std::atomic<uint8_t>`, and Core 0 only reads `_activeModeIndex` *after* it has already consumed and cleared `_requestedIndex`. The narrow window where Core 0 could observe `_activeModeIndex == MODE_INDEX_NONE` without a corresponding pending request is benign — it would simply render a tick with no active mode (same as startup). This is acceptable for an infrequent layout-reload path. Documented in the implementation comment. | Documented the race window and its benign nature in the implementation comments. No code change needed. |
| low | `test_render_invalid_does_not_crash` uses `ctx.matrix = nullptr`, so `render()` short-circuits at line 202 (`if (ctx.matrix == nullptr) return`) before reaching the `_invalid` branch and error-indicator drawing code. AC #5 error UI is not exercised in tests. | Deferred — requires either a matrix stub/mock or on-hardware test harness. Created [AI-Review] action item. |
| low | Log line in `init()` failure path (`"init: parse failed — error indicator will render"`) does not match the AC #5 specified literal (`"parse failed: %s"`). The `deserializeJson` error *is* logged in `parseFromJson()`, but the `init()` wrapper logs a different string. | Not applied — the error string *is* printed (in `parseFromJson`) and the `init()` wrapper adds context. The AC wording is guidance, not a literal string contract. Dismissed as minor documentation imprecision. |

## Story le-1-4 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | `_handlePutLayout` gave the same `"Body id does not match URL id"` error message for two distinct failure modes: (a) the `id` field is missing from the JSON body entirely, and (b) the `id` field is present but differs from the URL path segment. The misleading message in case (a) makes debugging harder for API clients. | Split into two separate checks with distinct error messages ("Missing id field in layout body" vs "Body id does not match URL id"), both returning `LAYOUT_INVALID` with HTTP 400. **Applied.** |
| dismissed | AC #9 / Task #10 — "Tests are lying tests / TEST_PASS() stubs that exercise nothing" | FALSE POSITIVE: The test file header (lines 1–29) explicitly documents why compile-only is the contract: `AsyncWebServerRequest*` cannot be constructed outside the ESPAsyncWebServer stack on ESP32. The story Dev Notes (Task 10) explicitly state "compile-only acceptable per AC #9." The AC #9 wording says "exercised by at least one test case" — the test file exercises the underlying layer APIs (LayoutStore, ModeOrchestrator symbols) that each handler depends on, satisfying the spirit of the AC within the constraints of the embedded test stack. This is a story documentation ambiguity, not a code defect. The 6 tests are not assertionless stubs: `test_layout_store_api_visible` has 4 real assertions verifying isSafeLayoutId and constant values; `test_activate_orchestrator_handoff_compiles` asserts function pointer non-null. `TEST_PASS()` in the remaining tests is correctly used for compile-time verification where there is no runtime observable behavior to assert. |
| dismissed | AC #6 / CRITICAL — "Hardcoded `custom_layout` mode ID not validated against ModeRegistry at runtime" | FALSE POSITIVE: The existing `_handlePostDisplayMode` (line 1318) applies exactly the same pattern without runtime validation of the hardcoded `custom_layout` string. The mode table is set up once in `main.cpp` and `custom_layout` is a core architectural constant for this product, not a user-configurable value. Adding a ModeRegistry lookup on every activate call would add heap churn and latency for a defensive check that would only fire during development (and would be immediately visible in testing). The established project pattern (confirmed in LE-1.3 Dev Notes) does not validate this string at the call site. |
| dismissed | Reviewer C CRITICAL — "`setActiveId` persist success not atomic with `onManualSwitch` — orchestrator called before/regardless of persistence" | FALSE POSITIVE: Direct code inspection (lines 2038–2044) confirms the implementation is already correct: `if (!LayoutStore::setActiveId(id.c_str())) { _sendJsonError(...); return; }` — the early `return` on failure means `ModeOrchestrator::onManualSwitch()` is only reached when `setActiveId` succeeds. Reviewer C's description of the bug does not match the actual code. False positive. |
| dismissed | AC #1 response shape — "implementation nests under `data.layouts` and adds `data.active_id`; story AC #1 says a flat top-level array" | FALSE POSITIVE: This is a real documentation drift between the story's AC text and the implementation, but it is NOT a code defect. The `api-layouts-spec.md` that was created as part of this story correctly documents the richer `{active_id, layouts:[...]}` shape. The editor client needs `active_id` alongside the list for a good UX; the implementation is correct and intentional per Task 3's dev notes ("Also include `active_id: LayoutStore::getActiveId()` at the top level of `data` for editor convenience"). An [AI-Review] action item was created to update the AC text; no code change required. |
| dismissed | DRY violation — ID validation duplicated 4× instead of shared function | FALSE POSITIVE: The `isSafeLayoutId()` validation is a single-line call (`LayoutStore::isSafeLayoutId(id.c_str())`) that is correctly placed in each handler independently. It is not duplicated logic — it is a validation guard that each handler must own because each extracts its own `id` variable. Extracting it into `extractLayoutIdFromUrl` would couple URL parsing to ID validation, creating its own concerns. The existing pattern is consistent with how `_handleDeleteLogo`, `_handleGetLogoFile`, etc. handle their own validation. Not a DRY violation. |
| dismissed | NVS write atomicity for cross-core safety not documented | FALSE POSITIVE: This concern is addressed in the LE-1.3 synthesis antipatterns record, which explicitly analyzed the `ConfigManager::setLayoutActiveId()` + `fireCallbacks()` chain as a LE-1.3 fix. The `Preferences::putString()` call is handled within the existing ConfigManager atomic-write pattern (debounce + NVS handle). The concern about cross-core partial writes on string values is noted in LE-1.3 context as a known benign window (same analysis as the `setLayoutActiveId` return-check dismissal in the le-1-1 antipatterns table). No new risk introduced by LE-1.4. |
| dismissed | `GET /api/widgets/types` — widget metadata hard-coded in WebPortal creates dual-source sync risk | FALSE POSITIVE: While a cheaper existence check (e.g. `LittleFS.exists()`) would work, using `LayoutStore::load()` is consistent with the established codebase pattern (the activate handler also uses `load()` for existence verification). The 8 KiB read is bounded, and PUT operations are rare user-initiated writes. The performance concern is valid but low-impact on the use case. Noted as a future improvement. |
| dismissed | `_handleGetLayoutById` uses two `JsonDocument` instances (extra heap allocation) | FALSE POSITIVE: The two-document pattern (`doc` for the outer envelope + `layoutDoc` for the nested parsed layout) is required because ArduinoJson v7 does not support copying a deserialized variant into a different document's object graph without a separate parse. The alternative (parse into one doc and re-build the envelope) would be more complex and error-prone. The extra allocation is bounded by `LAYOUT_STORE_MAX_BYTES` (8 KiB) and is immediately freed when the handler returns. Not a problem in practice on an ESP32 with 327 KB RAM (17.4% used per build log). |
| dismissed | SRP/DI violations — WebPortal is a "Fat Controller"; handlers should be injectable | FALSE POSITIVE: The project context explicitly names `firmware/adapters/WebPortal.cpp` as an adapter in the hexagonal architecture. Adapter classes in this project are intentionally responsible for routing + protocol translation + domain call delegation. Introducing an intermediate `LayoutController` or interface injection layer on an ESP32 with limited heap would add abstractions with zero testability benefit (test stack still can't mock `AsyncWebServerRequest`). This is an appropriate architectural choice for the embedded context. --- |

## Story le-1-5 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `_handleGetWidgetTypes` logo widget schema was stale post-LE-1.5 — the `obj["note"]` still said `"LE-1.5 stub — renders solid color block until real bitmap pipeline lands"` (false after LE-1.5 shipped the real pipeline), and the `content` field (the logo ICAO/id that `renderLogo()` now reads) was missing entirely from the field list. This means a layout editor following the API schema would produce logo widgets without a `content` field, which silently falls back to the airplane sprite on every render. | Removed stale `note` string, added `content` (string, default `""`) field before `color`, updated inline comment to describe the real LE-1.5 pipeline. |
| dismissed | Task 3 is "lying" — `scanLogoCount()` call at WebPortal.cpp line 466 was supposedly added by LE-1.5, contradicting "no changes needed." | FALSE POSITIVE: Git history is conclusive. Commit `6d509a0` ("Update continual learning state and enhance NeoMatrixDisplay with zone-based rendering. Added telemetry fields and improved logo management.") added both the `POST /api/logos` multipart handler **and** the `scanLogoCount()` call within it — this predates LE-1.5. The LE-1.5 story's claim that Task 3 required no changes is accurate. Reviewer C independently confirmed the upload handler existed at line 440–535 but then incorrectly attributed `scanLogoCount()` at line 466 to LE-1.5 without checking the git log. |
| dismissed | AC #4 is "partially false" — no changes were needed but `scanLogoCount()` was added | FALSE POSITIVE: Same as above. `scanLogoCount()` predates LE-1.5. AC #4 is accurate. |
| dismissed | AC #6 is "not literally true" — `LogoManager::loadLogo()` builds `String icao` and `String path` objects (heap allocations per frame). | FALSE POSITIVE: This is an accurate technical observation, but it is not a defect introduced by LE-1.5. The Dev Agent Record explicitly acknowledges: *"Implicit `String(spec.content)` at the call to `loadLogo()` is the only allocation — this is unavoidable due to the `LogoManager::loadLogo(const String&, ...)` signature, and it's identical to the established ClassicCardMode pattern that has been in production since ds-1.4."* AC #6's intent is "no `new`/`malloc` in `renderLogo()` itself, no second 2KB static buffer" — and that holds. The String allocation lives inside `LogoManager::loadLogo()`, which is documented as the shared pattern across all render modes. This is design-level acknowledged debt, not a LE-1.5 regression. |
| dismissed | Guard order in `renderLogo()` diverges from ClassicCardMode canonical pattern (dimension floor before buffer guard). | FALSE POSITIVE: The Dev Notes explicitly document and justify this ordering in Option 1 (dimension floor first → buffer guard → loadLogo → matrix guard). The ordering is intentional: the dimension floor short-circuits cheaply before the null pointer check, and `test_undersized_spec_returns_true` asserting the buffer is *untouched* when `w<8`/`h<8` is a stronger test contract. ClassicCardMode doesn't have a dimension floor guard at all (it's not needed for the fixed zone sizes it operates on). The "canonical" pattern doesn't apply identically because the context differs. |
| dismissed | `test_missing_logo_uses_fallback` is a weak/lying test — only proves buffer was changed, not that fallback sprite bytes are correct. | FALSE POSITIVE: The test correctly uses a sentinel (`0x5A5A`) and asserts at least one pixel differs from sentinel. The fallback sprite contains `0x0000` and `0xFFFF` pixels — neither matches `0x5A5A`. The assertion pattern is sound for proving the fallback loaded. Comparing to exact PROGMEM bytes via `memcpy_P` in a test would be stronger but is complexity for marginal gain; the current approach definitively proves the fallback fired. Not a "lying test." |
| dismissed | `test_null_logobuffer_returns_true` comment overclaims "must not call loadLogo" without proving non-call. | FALSE POSITIVE: The comment says "can't verify non-call without mocks" — it is honest about the limitation. The implementation guard at line 42 of `LogoWidget.cpp` (`if (ctx.logoBuffer == nullptr) return true;`) precedes the `loadLogo()` call at line 48, making the "no call" property structurally guaranteed by code order, not just test assertion. The comment is informative, not overclaiming. |
| dismissed | AC #7 doesn't verify AC #6 heap claim. | FALSE POSITIVE: AC #7 specifies correctness tests (LittleFS fixture, fallback, null buffer, undersized spec). Heap profiling on-device requires ESP32 heap instrumentation hooks beyond the scope of Unity tests and LE-1.5. The AC #6 "zero heap in render path" is verified by code inspection, not by a test assertion. This is the same approach used across the codebase. |
| dismissed | Per-frame LittleFS I/O is a performance antipattern without measurement data. | FALSE POSITIVE: The Dev Notes acknowledge this explicitly and cite ClassicCardMode as prior art: *"Flash reads on ESP32 with LittleFS are fast enough for 30fps display budget. ClassicCardMode has been doing this (LittleFS read every render frame) since Story ds-1.4 without measurable render-time regression."* This is an accepted design constraint, not a new LE-1.5 regression. Future caching is deferred intentionally. |
| dismissed | `cleanLogosDir()` path normalization is fragile (LittleFS name format edge cases). | FALSE POSITIVE: The test file already handles the path normalization edge case explicitly at lines 41–43: `if (!path.startsWith("/")) { path = String("/logos/") + path; }`. This mirrors the pattern from `test_logo_manager`. The test helper is robust enough for its purpose. --- |

## Story le-1-6 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | No smoke test coverage for editor assets — `/editor.html`, `/editor.js`, `/editor.css`, and `/api/widgets/types` had zero automated test coverage despite the project having a live-device smoke suite (`tests/smoke/test_web_portal_smoke.py`) that covers every other page (`/health.html`) and API endpoint. | Added `test_editor_page_loads`, `test_editor_js_loads`, `test_editor_css_loads`, `test_get_widget_types_contract` to the smoke suite. |
| low | `releasePointerCapture` not called in `onPointerUp` / `pointercancel` handler — pointer capture is automatically released on most browsers when `pointerup` fires, but explicit release is the spec-correct pattern and avoids potential stuck-capture on exotic browsers. | Added `canvas.releasePointerCapture(e.pointerId)` with try/catch inside `onPointerUp`, matching the same defensive pattern used for `setPointerCapture`. |
| low | Task 10 checkbox `[x]` is premature — the Dev Agent Record explicitly states AC10 is "⏭ Pending hardware" with no device test results documented. Marking the task complete while explicitly noting it was not performed is a story integrity violation. | Reverted Task 10 and all sub-tasks from `[x]` to `[ ]`. Added `[AI-Review]` follow-up in the new Senior Developer Review section. |
| dismissed | "Data Loss — feature is save-less (no layout persistence)" | FALSE POSITIVE: FALSE POSITIVE. The story explicitly scopes LE-1.6 to canvas + drag only. The story Dev Notes state: *"LE-1.6 uses GET /api/layout (canvas size) and GET /api/widgets/types (toolbox). Save/load is LE-1.7."* The antipattern table entry says "Save layout JSON in LE-1.6 → Defer to LE-1.7." This is intentional design, not a defect. |
| dismissed | "Race condition in drag state synchronization — network events could mutate widgets[] between selectedIndex and dragState assignment" | FALSE POSITIVE: FALSE POSITIVE. The editor is a single-threaded JavaScript page with no WebSockets, no `EventSource`, no `setInterval`-driven polls, and no background mutation of `widgets[]`. The only code that mutates `widgets[]` is user interaction (`addWidget`, drag/resize). JavaScript's event loop makes the selectedIndex→dragState sequence atomic in practice. No race condition vector exists. |
| dismissed | "AC8 gzip size unverifiable — no build artifacts" | FALSE POSITIVE: FALSE POSITIVE. The Dev Agent Record explicitly documents: *"wc -c total = 5,063 bytes (569 + 3,582 + 912). 24.7% of budget."* The gzip files exist at `firmware/data/editor.{html,js,css}.gz` (confirmed: 5,089 bytes total after synthesis fix, still 24.9% of 20 KB budget). |
| dismissed | "Resize handle is 24 logical pixels — 4× too large per AC5" | FALSE POSITIVE: FALSE POSITIVE with inverted math. Reviewer B claimed "6 CSS px × SCALE=4 = 24 logical pixels." The correct conversion is 1 logical pixel = 4 CSS pixels, so 6 CSS px = **1.5 logical px** (too small, not too large). Crucially, the story's Dev Notes explicitly specify `handleSize = 6 // CSS pixels` in the resize handle position code block, and Task 3 says "6×6 CSS-px resize handle square." The Dev Notes are the authoritative implementation reference. The AC5 text that says "6×6 logical-pixel" contradicts the Dev Notes — this is a documentation inconsistency in the story text, not an implementation bug. |
| dismissed | "Silent pointer capture failure — no fallback when setPointerCapture fails" | FALSE POSITIVE: FALSE POSITIVE. The code already handles this: `try { canvas.setPointerCapture(e.pointerId); } catch (err) { /* ignore */ }`. The `pointercancel` event is also wired to `onPointerUp` (line 352) which clears `dragState`, preventing stuck drag states. The suggested complex fallback (document-level event handlers with cleanup closures) would substantially increase complexity and gzip size for a low-probability edge case on a LAN-only appliance UI. |
| dismissed | "Implicit frontend/backend validation coupling — widget constraints not in API" | FALSE POSITIVE: FALSE POSITIVE. The editor **does** read constraints from the API dynamically via `buildWidgetTypeMeta()` which derives `minH = meta.defaultH` (from the `h` field's default value) and `minW = 6` as a constant. This is the documented approach per the story Dev Notes. Adding explicit `min_w`/`min_h` fields to the API would require firmware changes (out of LE-1.6 scope) and is deferred per the story's antipattern table. |
| dismissed | "Toast styling — `.toast-warning` class not in editor.css" | FALSE POSITIVE: FALSE POSITIVE. `editor.html` loads both `style.css` (global styles) AND `editor.css`. The toast system is implemented in `common.js` and styled in `style.css` — not editor.css. This is the same pattern used by dashboard, wizard, and health pages. No additional toast styles are needed in `editor.css`. |
| dismissed | "Performance footgun — status update every `pointermove` frame" | FALSE POSITIVE: DISMISSED as acceptable. `setStatus()` performs a single `textContent` assignment on a `<span>`. Modern browsers batch DOM updates within an animation frame. This is the standard pattern for real-time position readouts in canvas editors and is consistent with the project's existing approach. Adding debounce would introduce `setTimeout` complexity for unmeasured benefit on an appliance UI. |
| dismissed | "`e.preventDefault()` only fires on hit — miss path allows browser gestures" | FALSE POSITIVE: FALSE POSITIVE. The canvas has `touch-action: none` (editor.css line 87) which unconditionally prevents browser scroll/zoom interception for all touch events on the canvas element, regardless of hit testing. `e.preventDefault()` inside the hit branch is the explicitly documented "belt-and-braces" measure. The antipatterns table entry states: "DO NOT: `e.preventDefault()` globally on body; DO INSTEAD: `e.preventDefault()` inside `onPointerDown` only." Calling it on miss would prevent tap-to-deselect on some touch browsers without benefit. |
| dismissed | "Toolbox click listener accumulates on repeated `initToolbox()` calls" | FALSE POSITIVE: FALSE POSITIVE in this implementation. `initToolbox()` is called exactly once — from the `loadLayout()` `.then()` chain, which is itself called once from `init()`, which is called once from `DOMContentLoaded`. There is no code path that invokes `initToolbox()` more than once per page lifecycle. |
| dismissed | "Inclusive hit-test rectangles create shared-edge ambiguity between adjacent widgets" | FALSE POSITIVE: FALSE POSITIVE. The hit-test uses `<=` on both edges, which is standard canvas hit-testing. Adjacent widgets on a shared edge are an extremely uncommon layout scenario, and when they occur, the topmost widget (last in array) wins because the loop traverses from end to start. This is predictable and consistent behavior; the UX impact is negligible. |
| dismissed | "Story 'Files modified' section omits `sprint-status.yaml`" | FALSE POSITIVE: DISMISSED as a documentation completeness note rather than a code defect. The story `File List` documents the directly created/modified source files. `sprint-status.yaml` is an artifact file. The Dev Agent Record explicitly notes the sprint status update in its own section. |
| dismissed | "AC7 logo minimum height: story says h=8, API returns h=16 — implementation is wrong" | FALSE POSITIVE: PARTIALLY DISMISSED — the code is correct, the AC7 story text is wrong. `buildWidgetTypeMeta()` correctly reads `minH = meta.defaultH` from the API response, which returns `h=16` for the logo widget (WebPortal.cpp line 2149). This means the logo min floor is 16 (correct firmware behavior), not 8 as incorrectly stated in AC7. The implementation correctly follows the API truth. The AC7 story text error (`logo: h=8`) is a documentation inaccuracy; no code fix needed. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 0 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **3** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|
| 🔴 CRITICAL | Task marked [x] but not performed: Mobile Smoke Test (AC#10) | Story `[ ]` Task 10 | +3 |
| 🟠 IMPORTANT | Mobile smoke test deferred to real hardware, AC#10 remains unverified | AC#10 | +1 |
| 🟡 MINOR | Resize handle hit-test ambiguity due to inclusion of handle bounds in widget body check | editor.js | +0.3 |

### Evidence Score: 4.3

| Score | Verdict |
|-------|---------|
| **4.3** | **MAJOR REWORK** |

---

## 🐛 Correctness & Safety

- **🐛 Bug:** Resize handle hit-test ambiguity. The `hitTest` function checks the resize handle first, then the widget body. However, because the resize handle is *inside* the widget body area (it's the bottom-right 6px), both checks pass. While the current priority-based order (`if (hitHandle) return handle; if (hitBody) return body;`) technically works for the *resize* mode, the body hit-test is overly broad and could conflict if future features added more handles.
  - 📍 `firmware/data-src/editor.js:122`
  - 🔄 Reproduction: Open editor, add widget, click bottom-right corner — it correctly triggers resize mode, but the body hit-test logic is theoretically unsound.

---

## 🛠️ Suggested Fixes

### 1. Close Task 10 or Document deferral

**File:** `_bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md`
**Issue:** Task 10 is marked as complete [x] but the Dev Agent Record explicitly states the mobile smoke test was deferred to physical hardware and not performed.

**Corrected status:** Revert Task 10 to `[ ]` and update the status of the story to `in-progress` until verified on device.

---

**Issues Fixed:** 0
**Action Items Created:** 1

- [AI-Review] CRITICAL: Revert Task 10 in `le-1-6-editor-canvas-and-drag-drop.md` as it is not verified. Story must remain `in-progress` until physical mobile device verification is performed per AC#10.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial code review complete, Christian.**

**Story:** `_bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md`  
**Scope reviewed (from embedded evidence):** `firmware/data-src/editor.{html,js,css}`, `firmware/adapters/WebPortal.cpp` (editor routes + `_serveGzAsset`), `firmware/data-src/common.js` (toast/fetch helpers), `_handleGetWidgetTypes` excerpt, story tasks/ACs, antipatterns appendix. **Git/workspace state was not re-read in this session** (read-only validator constraint); discrepancies are flagged as *claims-risk* where the story itself contradicts execution notes.

---

## Review summary

| Category | Issues found |
|----------|--------------|
| Git vs story discrepancies | 1 (cannot fully verify without `git diff`; story File List vs Dev Agent Record mismatch is explicit) |
| AC implementation gaps | 1 (AC10) |
| Task completion lies | 0 (Task 10 is open `[ ]` in the embedded story; no `[x]` lie for Task 10) |
| SOLID / architecture | 1 (minor boundary note) |
| Hidden bugs / correctness hazards | 2 (listener duplication hazard; weak numeric parsing) |
| Performance | 0 material (full redraw per move is expected at this scale) |
| Security | 0 material for LAN-only editor scope |
| Test quality / automation honesty | 2 |
| **Total issues** | **10** |

---

## Evidence score summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | AC10 is not satisfied, yet the story is marked **Status: done**—this is a requirements closure gap, not a “nice-to-have”. | Story header vs AC10 + Dev Agent Record | +1.0 |
| 🟠 IMPORTANT | Traceability gap: story **File List** omits changes the Dev Agent Record claims (`tests/smoke/test_web_portal_smoke.py`, sprint status). | Story “File List” vs Dev Agent Record | +1.0 |
| 🟠 IMPORTANT | Automated coverage likely **shallow** if smoke only checks reachability; it does not prove gzip contract, JSON schema stability, or editor runtime behavior. | Test strategy critique | +1.0 |
| 🟠 IMPORTANT | `initToolbox()` binds `click` on `#editor-toolbox` after `innerHTML` reset—**safe only because `loadLayout()` runs once**; a future “reload layout” call becomes a **duplicate listener** footgun. | `firmware/data-src/editor.js` (`initToolbox`) | +1.0 |
| 🟡 MINOR | Story AC7 text conflicts with shipped API truth (logo `h` default/floor is **16**, not “8 px” as AC7 claims). | Story AC7 vs `WebPortal::_handleGetWidgetTypes` excerpt | +0.3 |
| 🟡 MINOR | `buildWidgetTypeMeta()` assigns `defaultW/defaultH` without `parseInt`—**stringly-typed JSON** would silently poison geometry (`NaN` propagation). | `firmware/data-src/editor.js` | +0.3 |
| 🟡 MINOR | `FW.*` usage without a hard guard if `common.js` fails—**hard crash** instead of graceful degradation. | `firmware/data-src/editor.js` | +0.3 |
| 🟡 MINOR | Editor UX is pointer-first; toolbox is not keyboard-operable (WCAG gap). Acceptable for v1, but it’s a real product limitation. | `firmware/data-src/editor.html` / toolbox markup | +0.3 |
| 🟡 MINOR | ES5 “grep gate” is **easily fooled** (multiline constructs, build-time transforms, copy/paste drift). | Story AC9 / process | +0.3 |
| 🟡 MINOR | Structural tech debt remains: widget schema triplication is explicitly warned in firmware (`WebPortal` comment)—editor increases coupling surface. | `WebPortal.cpp` comment block near `_handleGetWidgetTypes` | +0.3 |

**Clean passes (categories with no material issue in-scope):**

- **Security (LAN appliance assumptions):** no new auth boundary introduced by static editor assets beyond existing portal threat model → **-0.5**
- **Performance (hot path scale):** canvas is small; full redraw per move is reasonable → **-0.5**
- **SOLID (overall shape):** thin IIFE + imperative UI is consistent with repo’s embedded JS style → **-0.5**

### Evidence score

Sum of finding scores: \(1.0 + 1.0 + 1.0 + 1.0 + 6 \times 0.3 = 5.8\)  
Clean passes: \(3 \times -0.5 = -1.5\)  
**Evidence score = 4.3**

| Score | Verdict |
|-------|---------|
| **4.3** | **MAJOR REWORK** (per your rubric: \(3 \le \text{score} < 7\)) |

Interpretation: this is not “rewrite the editor,” but **do not treat the story as fully closed** until AC10 is executed + recorded, automation proves the wire contract, and documentation matches API truth.

---

## Architectural sins

- **[3/10] Dependency inversion / duplication hazard (D):** the editor’s palette is driven by a **hard-coded** widget schema in `WebPortal::_handleGetWidgetTypes`, while validation/render dispatch also depend on parallel lists (`LayoutStore`, `WidgetRegistry`). The firmware itself documents this sync bomb. The editor **inherits** that fragility—any schema drift becomes “UI lies about device capabilities.”
  - **Location:** `WebPortal.cpp` (sync-risk comment + `_handleGetWidgetTypes` implementation)
  - **Fix:** centralize a single descriptor table consumed by API + validator + dispatch (deferred is fine, but track as explicit epic debt).

---

## Pythonic crimes & readability

- **N/A (primary deliverable is embedded JS/C++).**  
- **JS readability note:** `editor.js` is appropriately small and linear; no “crime” beyond the hazards listed above.

---

## Performance & scalability

- **No significant issue** for the intended matrix sizes and LAN-only usage. Full redraw each `pointermove` is coarse but predictable here.

---

## Correctness & safety

- **🐛 Duplicate event listener risk (`initToolbox`)**  
  - **Reproduction:** call `loadLayout()` twice without full page reload (future feature: “refresh hardware layout”). Each call runs `initToolbox()` → another `click` listener on the same `#editor-toolbox`.
  - **Location:** `firmware/data-src/editor.js` (`initToolbox`)

- **🐛 Weak numeric parsing for schema defaults**  
  - **Reproduction:** if any path emits `"32"` as a string for `w`, arithmetic becomes `NaN` and the editor “works” until first render/move.
  - **Location:** `firmware/data-src/editor.js` (`buildWidgetTypeMeta`)

- **🎭 “Done” vs AC10**  
  - **Why it’s dishonest at the requirements level:** AC10 is an acceptance criterion, not a separate epic. “Deferred manual” can be a **task** state, but it cannot simultaneously be a **done** story without redefining “done.”

---

## Maintainability issues

- **💣 Tech debt:** triplicated widget type knowledge (explicitly warned in firmware). Explosion radius: **every new widget type** requires coordinated edits across C++ surfaces *and* editor expectations.
- **💣 Process debt:** story File List not matching Dev Agent Record undermines auditability (release engineering + review time sink).

---

## Suggested fixes (prioritized)

### 1) Close AC10 honestly (story + sprint hygiene)

**Issue:** AC10 remains unmet while story status is `done`.  
**Fix:** keep story `in-progress` / `blocked-qa` until the Dev Agent Record contains **dated** results for iOS Safari + Android Chrome; only then mark `done`.

### 2) Make smoke tests prove the wire contract, not just “200 OK”

**Issue:** reachability checks are not contract checks.  
**Fix (Python smoke):** assert for editor assets:

- HTTP 200
- `Content-Encoding` contains `gzip` (or whatever the server actually emits—**must match** AC1)
- body bytes decompress to plausible HTML/JS/CSS markers (`<canvas`, `FW`, `.editor-layout`)

### 3) Harden `initToolbox` against duplicate listeners

**Issue:** future `loadLayout()` reuse breaks silently.  
**Fix:** attach the toolbox click delegate **once** outside `initToolbox`, or use `removeEventListener` / `{ once: true }` pattern, or rebuild handler with a stable function reference.

### 4) Parse ints defensively in `buildWidgetTypeMeta`

**Issue:** string defaults → `NaN`.  
**Fix:** `parseInt(..., 10)` with `isNaN` fallback to existing defaults.

### 5) Correct story AC7 text to match API/firmware truth

**Issue:** AC7 claims logo minimum height 8px; shipped schema default height for logo is **16** in the embedded `WebPortal` excerpt.  
**Fix:** update AC7 to “derive from `/api/widgets/types`” (normative) and remove incorrect literals.

---

**Issues fixed in this review:** 0 (read-only validator)  
**Action items created:** 5 (above)

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

# ADVERSARIAL CODE REVIEW: Story LE-1.6 (Editor Canvas and Drag-Drop)

**Reviewer:** Senior Developer Code Review (Adversarial Assessment)  
**Date:** 2024  
**Story:** LE-1.6 - editor-canvas-and-drag-drop  
**Status:** APPROVED WITH CRITICAL ISSUES  
**Evidence Score:** 3.2/5.0 (Multiple AC violations identified; some claims unverified by implementation)

---

## EXECUTIVE SUMMARY

Story LE-1.6 implementation contains **8 confirmed defects** (3 HIGH severity, 4 MEDIUM, 1 LOW) across code quality, acceptance criteria compliance, and architectural integrity. Most critically:

1. **AC #3 VIOLATION (HIGH):** Toolbox items are clickable, not draggable. AC explicitly requires "draggable toolbox item" and "cannot be dropped onto the canvas" but implementation uses click-to-add only.

2. **Hit Test Bug (HIGH):** Boundary condition uses inclusive `<=` instead of exclusive `<`, causing off-by-one hits at widget right/bottom edges.

3. **Widget Overflow (HIGH):** `addWidget()` does not validate that widget default dimensions fit within device canvas, allowing creation of oversized widgets.

4. **Schema Validation Missing (MEDIUM):** `/api/layout` response not validated before destructuring; undefined matrix/hardware fields cause NaN canvas dimensions.

5. **Hit Test Race Condition (MEDIUM):** `selectedIndex` mutation before widget reference capture creates fragility if events interleave.

6. **Pointer Capture Ordering (MEDIUM):** `releasePointerCapture` called after `dragState` cleared; logic is correct but ordering is backwards.

7. **Canvas Resize Without Bounds (MEDIUM):** Calling `initCanvas()` multiple times while dragging could cause coordinate mismatch with selectedIndex.

8. **Edge-Case Canvas Positioning (LOW):** Pointer coordinate extraction relies on integer CSS pixels; fractional coordinates from transform scaling could cause snap rounding artifacts.

---

## DETAILED FINDINGS

### **ISSUE #1: AC #3 VIOLATION - No Drag-and-Drop Implementation (HIGH)**

**Acceptance Criteria Claim:**
> "Given `GET /api/widgets/types` is called on page load When the response is received Then each widget type entry in `data[]` produces one **draggable toolbox item** showing `entry.label`; any entry with a `note` field containing "not yet implemented" is shown with a visual disabled/muted style and **cannot be dropped onto the canvas**."

**Evidence:**

The AC uses "draggable" and "dropped onto the canvas" (lines 28-29 of story). The CSS has `cursor: grab` on `.toolbox-item` (editor.css:59), suggesting drag semantics.

**Implementation Review:**

```javascript
// editor.js lines 262-268
function onToolboxClick(e) {
  var t = e.target;
  if (!t || !t.classList || !t.classList.contains('toolbox-item')) return;
  if (t.classList.contains('disabled')) return;
  var type = t.getAttribute('data-type');
  if (type) addWidget(type);
}
```

The implementation:
- ✗ No `dragstart` / `dragend` event handlers
- ✗ No `drop` target implementation
- ✓ Uses `click` events instead (not in AC)
- ✓ Disabled items block the click (indirectly meets AC intent)

**Root Cause:**
AC #3 explicitly specifies "draggable toolbox item" and drag-drop semantics. Implementation skipped drag-drop entirely and substituted click-to-add.

**Impact:** 
Mobile users expect tap-to-add (which works), but the AC contract is broken. If a future story requires actual drag-drop (e.g., drag from toolbox to canvas position), the implementation cannot satisfy it without major refactoring.

**Fix Priority:** HIGH — AC violation  
**Recommended Fix:** Either:
1. Implement drag-drop: `dragstart` → set `dataTransfer.setData('type', type)`, `dragover`/`drop` on canvas to add at drop point, OR
2. Update AC to clarify click-to-add is acceptable and adjust "cannot be dropped" language

---

### **ISSUE #2: Hit Test Boundary Off-by-One (HIGH)**

**Claim:** AC #4 states "the widget's `x` and `y` are updated by the delta in logical canvas pixels". AC #5 states "user presses on a resize handle".

**Evidence:**

```javascript
// editor.js lines 130-138 (widget body hit test)
for (var i = widgets.length - 1; i >= 0; i--) {
  var w = widgets[i];
  var x0 = w.x * SCALE;
  var y0 = w.y * SCALE;
  var x1 = (w.x + w.w) * SCALE;
  var y1 = (w.y + w.h) * SCALE;
  if (cx >= x0 && cx <= x1 && cy >= y0 && cy <= y1) {  // ← ISSUE
    return { index: i, mode: 'move' };
  }
}
```

**Problem:**
- Canvas `fillRect(x, y, w, h)` draws pixels from `[x, x+w)` (inclusive-exclusive)
- Hit test uses `cx <= x1` (inclusive), which includes the right/bottom boundary
- For widget at (0, 0) with w=32:
  - x0 = 0, x1 = 128
  - Pointer at x=128 **hits the widget** (exclusive boundary of next pixel)
  - But fillRect only fills [0, 128), so x=128 is NOT part of the widget

**Correct Implementation:**
```javascript
if (cx >= x0 && cx < x1 && cy >= y0 && cy < y1) {  // exclusive upper bound
```

**Impact:** 
Edge pixels register as hits when they shouldn't. User can select widgets by clicking the boundary outside their visual bounds. Unlikely to affect UX (1-2 pixels), but violates rectangle semantics.

**Fix Priority:** MEDIUM (low practical impact, but correctness violation)

---

### **ISSUE #3: addWidget() Allows Oversized Widgets (HIGH)**

**Claim:** AC #1 states canvas is "correct device pixel dimensions obtained from GET /api/layout". AC #7 implies widgets have size constraints.

**Evidence:**

```javascript
// editor.js lines 270-291
function addWidget(type) {
  var meta = widgetTypeMeta[type];
  if (!meta) return;
  var w = {
    type: type,
    x: snapTo(8, SNAP),
    y: snapTo(8, SNAP),
    w: meta.defaultW,  // ← Not validated against device width
    h: meta.defaultH,  // ← Not validated against device height
    color: meta.defaultColor || '#FFFFFF',
    content: meta.defaultContent || '',
    align: meta.defaultAlign || 'left',
    id: 'w' + Date.now()
  };
  /* Clamp into canvas */
  if (w.x + w.w > deviceW) w.x = Math.max(0, deviceW - w.w);
  if (w.y + w.h > deviceH) w.y = Math.max(0, deviceH - w.h);
  widgets.push(w);
  selectedIndex = widgets.length - 1;
  render();
}
```

**Problem:**
- Lines 285-286 only clamp **position** (x, y), not **dimensions** (w, h)
- If `meta.defaultW > deviceW`, widget is created with width exceeding canvas
- Example: device is 48×48 pixels, logo widget defaults to w=16, h=16 (OK), but if a future widget has w=100, it would be created oversized
- No validation that `w.w <= deviceW - w.x` or `w.h <= deviceH - w.y`

**Impact:**
Widgets can exceed device boundaries. Canvas still renders them, but they overflow off-screen with no user feedback. Editor misleads about actual layout.

**Fix Priority:** HIGH (breaks AC #1 promise of "correct canvas preview")

**Recommended Fix:**
```javascript
// Clamp dimensions to fit within canvas bounds
if (w.x + w.w > deviceW) {
  w.w = Math.max(meta.minW, deviceW - w.x);
}
if (w.y + w.h > deviceH) {
  w.h = Math.max(meta.minH, deviceH - w.y);
}
```

---

### **ISSUE #4: Missing /api/layout Response Schema Validation (MEDIUM)**

**Claim:** AC #1 states "renders a canvas showing the correct device pixel dimensions obtained from `GET /api/layout` → `data.matrix.width` × `data.matrix.height` at 4× CSS scale".

**Evidence:**

```javascript
// editor.js lines 334-342
function loadLayout() {
  FW.get('/api/layout').then(function(res) {
    if (!res.body || !res.body.ok) {
      FW.showToast('Cannot load layout info — check device connection', 'error');
      return null;
    }
    var d = res.body.data;  // ← No validation of d's schema
    initCanvas(d.matrix.width, d.matrix.height, d.hardware.tile_pixels);
    // ...
  })
}
```

**Problem:**
- `d` is not validated before access
- If API returns `{"ok": true, "data": {}}` (valid JSON but missing fields):
  - `d.matrix` is undefined → `d.matrix.width` throws TypeError
  - OR if graceful, `initCanvas` receives undefined parameters:
    - `canvas.width = undefined * SCALE = NaN`
    - Canvas becomes unusable
- No guards against:
  - Missing `data.hardware` object
  - Non-numeric `width`, `height`, `tile_pixels` values

**Impact:**
Malformed API responses (even technically valid "ok: true" responses) crash the page. Poor error handling.

**Fix Priority:** MEDIUM (defensive programming)

**Recommended Fix:**
```javascript
var d = res.body.data;
if (!d || typeof d.matrix !== 'object' || typeof d.matrix.width !== 'number' ||
    typeof d.matrix.height !== 'number' || !d.hardware || 
    typeof d.hardware.tile_pixels !== 'number') {
  FW.showToast('Invalid layout data received', 'error');
  return null;
}
```

---

### **ISSUE #5: Hit Test Fragility - selectedIndex Mutation Before Widget Capture (MEDIUM)**

**Claim:** AC #4 states drag updates widget "x" and "y".

**Evidence:**

```javascript
// editor.js lines 144-167
function onPointerDown(e) {
  var canvas = document.getElementById('editor-canvas');
  var p = getCanvasPos(e);
  var hit = hitTest(p.x, p.y);
  if (hit) {
    selectedIndex = hit.index;  // ← Mutation BEFORE capture
    var w = widgets[selectedIndex];  // ← THEN read widget
    dragState = {
      mode: hit.mode,
      startX: e.clientX,
      startY: e.clientY,
      origX: w.x,  // ← Could stale-reference if widgets mutated
      origY: w.y,
      origW: w.w,
      origH: w.h,
      toastedFloor: false
    };
```

**Problem:**
- Global `selectedIndex` is mutated before the widget reference is captured
- If another event (e.g., `onToolboxClick` calling `addWidget`) fires between lines 149 and 150, `widgets[]` could be modified
- Next line (150) would read the wrong widget reference
- In single-threaded JS, this is mitigated by `setPointerCapture` on line 161, which prevents other pointer events, but does NOT prevent toolbox clicks

**Example Scenario:**
1. `onPointerDown` hits widget 0, line 149 sets `selectedIndex = 0`
2. Before line 150 executes, a `click` event on toolbox fires `addWidget('text')`, which:
   - Line 287: `widgets.push(w)` — array grows
   - Line 288: `selectedIndex = widgets.length - 1 = 1` — global is overwritten!
3. Line 150 reads `w = widgets[selectedIndex]` but selectedIndex is now 1, not 0

**Impact:**
Pointer capture is not set, so subsequent drag operates on wrong widget. This is a rare race condition, but correctness issue.

**Fix Priority:** MEDIUM (low probability, but high consequence)

**Recommended Fix:**
```javascript
if (hit) {
  var w = widgets[hit.index];  // Capture FIRST (no mutation)
  dragState = {
    // ... origX, origY, origW, origH from w
  };
  selectedIndex = hit.index;  // THEN mutate
  canvas.setPointerCapture(e.pointerId);
```

---

### **ISSUE #6: Pointer Capture Release After dragState Clear (MEDIUM)**

**Claim:** AC #4 states drag/resize should work correctly.

**Evidence:**

```javascript
// editor.js lines 205-216
function onPointerUp(e) {
  if (dragState === null) return;  // ← Early return if no drag
  dragState = null;  // ← Clear state FIRST
  var canvas = document.getElementById('editor-canvas');
  if (canvas && e && e.pointerId != null) {
    try { canvas.releasePointerCapture(e.pointerId); } catch (err) { /* ignore */ }
  }
  // ...
}
```

**Problem:**
- `dragState` is cleared on line 207
- Pointer capture is released on line 210
- The check at line 206 means if dragState was null (no drag in progress), we return before releasing
- This is actually correct behavior (you only release capture if it was set), but the ordering is backwards
- Should check AND clear after capture release:

**Correct Pattern:**
```javascript
if (dragState === null) return;  // Only release if drag was active
var canvas = document.getElementById('editor-canvas');
if (canvas && e && e.pointerId != null) {
  try { canvas.releasePointerCapture(e.pointerId); } catch (err) { }
}
dragState = null;  // Clear AFTER releasing
```

**Impact:** 
Low—the current code works correctly because the early return prevents release of non-existent captures. But the ordering is semantically backwards.

**Fix Priority:** LOW (cosmetic/ordering issue)

---

### **ISSUE #7: Canvas Resize Without Coordinate Bounds Check (MEDIUM)**

**Claim:** AC #1 states canvas shows "correct device pixel dimensions".

**Evidence:**

```javascript
// editor.js lines 105-116
function initCanvas(matrixW, matrixH, tilePx) {
  deviceW = matrixW;
  deviceH = matrixH;
  tilePixels = tilePx || 16;
  var canvas = document.getElementById('editor-canvas');
  if (!canvas) return;
  canvas.width = matrixW * SCALE;
  canvas.height = matrixH * SCALE;
  canvas.style.width = (matrixW * SCALE) + 'px';
  canvas.style.height = (matrixH * SCALE) + 'px';
  render();
}
```

**Problem:**
- `initCanvas()` is called in `loadLayout()` (line 341)
- If `/api/layout` changes and `loadLayout()` is called again (or if API is polled), canvas is resized
- If resized DURING a drag operation:
  - `deviceW`/`deviceH` globals updated
  - `dragState` still references `origX`, `origY` in OLD coordinate system
  - `widgets[selectedIndex].x` and `.y` are now scaled differently
  - Pointer move calculations use new `deviceW`/`deviceH` but old `dragState.origX/Y`

**Impact:**
Unlikely in practice (layout doesn't change dynamically), but if it did, drag would become incoherent.

**Fix Priority:** LOW (design assumes static layout)

**Recommended Fix:**
Add guard in `loadLayout()`:
```javascript
if (dragState) {
  FW.showToast('Cannot reload layout during drag', 'warning');
  return;
}
```

---

### **ISSUE #8: Floating-Point Coordinate Rounding in Snap (LOW)**

**Claim:** AC #6 states "snap to the nearest 8 logical pixels".

**Evidence:**

```javascript
// editor.js lines 18-20
function snapTo(val, grid) {
  return Math.round(val / grid) * grid;
}

// Called with:
// editor.js lines 180-181
var nx = snapTo(dragState.origX + dx, SNAP);
```

**Problem:**
- `dx` is computed from CSS pixel deltas divided by SCALE (line 176)
- If canvas has CSS transforms or is positioned at fractional pixels:
  - `e.clientX` could be fractional (in DPI-scaled browsers)
  - `rect.left` from `getBoundingClientRect()` could be fractional
  - `dx = (fractional) / SCALE` yields fractional value
- `snapTo()` rounds, which is correct for snapping, but:
  - value 7.9999 → Math.round(7.9999/8) = Math.round(0.9999) = 1, result = 8
  - This is actually correct snapping behavior (7.9999 is closer to 8)

**Impact:**
Very low—snap math is actually robust. Fractional coordinates are correctly rounded to the nearest grid point.

**Fix Priority:** LOW (no action needed)

---

### **ISSUE #9: Missing Validation on /api/widgets/types Response (MEDIUM)**

**Claim:** AC #3 states "each widget type entry in `data[]` produces one toolbox item".

**Evidence:**

```javascript
// editor.js lines 343-346
}).then(function(res) {
  if (!res || !res.body || !res.body.ok) return;
  initToolbox(res.body.data);  // ← data not validated
  updateSnapButtons();
});

// editor.js lines 245-260
function initToolbox(types) {
  var box = document.getElementById('editor-toolbox');
  if (!box) return;
  box.innerHTML = '';
  for (var i = 0; i < types.length; i++) {  // ← Assumes types is array
    buildWidgetTypeMeta(types[i]);
    // ...
  }
  box.addEventListener('click', onToolboxClick);
}
```

**Problem:**
- `types` is assumed to be an array but not validated
- If API returns `{"ok": true, "data": null}` or `{"ok": true, "data": {}}` (object instead of array):
  - `types.length` is undefined or 0 (object), loop doesn't execute or iterates over properties
  - `for...in` would iterate wrong items
  - `buildWidgetTypeMeta(types[i])` would receive `undefined`

**Impact:**
Malformed API response breaks toolbox initialization. User sees empty toolbox with no error message.

**Fix Priority:** MEDIUM (defensive)

**Recommended Fix:**
```javascript
if (!res.body.data || !Array.isArray(res.body.data)) {
  FW.showToast('Invalid widget types response', 'error');
  return;
}
initToolbox(res.body.data);
```

---

### **ISSUE #10: selectedIndex Can Be -1 During onPointerMove (LOW)**

**Claim:** AC #4 states drag updates widget coordinates.

**Evidence:**

```javascript
// editor.js lines 170-173
function onPointerMove(e) {
  if (dragState === null) return;
  var w = widgets[selectedIndex];  // ← selectedIndex might be -1
  if (!w) { dragState = null; return; }  // ← Guard exists but fragile
```

**Problem:**
- `onPointerMove` reads `widgets[selectedIndex]` without checking if `selectedIndex >= 0`
- If `selectedIndex === -1` (deselected), `widgets[-1]` returns `undefined`
- Line 173 catches this with `if (!w)` and clears dragState
- But this should be impossible if drag is active (dragState would have captured the index)

**Root Cause:**
`dragState` is independent of `selectedIndex`. If `dragState` is set but `selectedIndex` changes (e.g., from other events), they desynchronize.

**Impact:**
Low—defensive guard exists. But design is fragile.

**Fix Priority:** LOW (guard present, though design could be tighter)

---

### **ISSUE #11: No Maximum Widget Size Validation (MEDIUM)**

**Claim:** AC #1 promises canvas "honestly previews" the LED wall.

**Evidence:**
(See ISSUE #3—same root cause)

The story's AC #7 specifies **minimum** floors but never specifies maximum dimensions. If a widget is added with w=1000 when device is 192 pixels wide, `addWidget()` clamps position but not size, resulting in a 1000-pixel-wide widget that exceeds the device.

**Impact:**
Canvas preview is dishonest—widgets can exceed device bounds.

**Fix Priority:** MEDIUM (architectural issue)

---

### **ISSUE #12: Toast Suppression Logic (LOW)**

**Claim:** AC #7 states "`FW.showToast` is called with an explanatory message".

**Evidence:**

```javascript
// editor.js lines 192-196
if ((rawW < meta.minW || rawH < meta.minH) && !dragState.toastedFloor) {
  if (typeof FW !== 'undefined' && FW.showToast) {
    FW.showToast(w.type + ' minimum: ' + meta.minW + 'w × ' + meta.minH + 'h px', 'warning');
  }
  dragState.toastedFloor = true;
}
```

**Observation:**
- Toast fires once per drag gesture (good)
- Check `typeof FW !== 'undefined'` is defensive against missing global
- Logic is sound

**No Issue Found** ✓

---

## QUALITY ASSESSMENT

### SOLID Principles

| Principle | Status | Evidence |
|-----------|--------|----------|
| **Single Responsibility** | ⚠ VIOLATED | `render()` handles clearing, grid drawing, and widget rendering. `loadLayout()` handles both layout and toolbox initialization. `onPointerMove()` handles move AND resize modes. |
| **Open/Closed** | ⚠ VIOLATED | Adding a new snap mode requires modifying multiple functions (initSnap, updateSnapButtons, HTML). No extension points. |
| **Liskov Substitution** | ✓ N/A | No inheritance/polymorphism. |
| **Interface Segregation** | ✓ OK | Separation of pointer handlers and toolbox handlers. |
| **Dependency Inversion** | ⚠ VIOLATED | Direct dependence on global FW object; no abstraction layer for API calls. |

### Test Coverage

**No automated tests found.** Story AC #10 (mobile smoke test) is manual-only and marked pending. Critical paths have no unit test coverage:
- Hit test boundary conditions
- Widget overflow scenarios
- API response malformation
- Snap rounding edge cases

**Impact:** Zero regression test safety. Any refactoring risks breaking drag/snap behavior.

### Performance

- ✓ `render()` calls are efficient (single canvas redraw per move)
- ✓ Grid drawing uses simple line loops (acceptable for 12×3 tile grid)
- ✓ No unnecessary DOM manipulation (events delegated on toolbox)
- ⚠ Widget drawing doesn't batch—draws each widget individually (acceptable for small counts, <100)

### Security

- ⚠ **No XSS mitigation:** `setStatus()` uses `textContent =` (safe), but `setStatus()` is called with unsanitized user input (widget coordinates). Safe because output is numeric, but no validation that coordinates are numbers.
- ✓ `localStorage` writes validated to {1,8,16} only.
- ⚠ **No CSRF token** on canvas actions, but they're read-only (no state mutations sent to server in LE-1.6).

### Code Style & Maintainability

- ✓ Consistent ES5 compliance
- ✓ Clear variable names
- ✗ No JSDoc comments explaining complex functions (hitTest, onPointerMove, etc.)
- ✗ "use strict" inside IIFE only (not enforced globally)
- ✗ Comment density low (5 lines of comments for 370 lines of code)

---

## ACCEPTANCE CRITERIA VERIFICATION

| AC | Status | Notes |
|----|--------|-------|
| 1. Gzip serving, canvas dimensions, imageSmoothingEnabled | ✓ PASS | Route registered, `initCanvas` sets dimensions, `render()` sets imageSmoothingEnabled. |
| 2. Tile grid | ✓ PASS | `drawGrid()` draws every `tilePixels * SCALE` CSS pixels. |
| 3. Toolbox from API, disabled items | ✗ **FAIL** | AC requires "draggable" items and "cannot be dropped"—implementation uses click-to-add. Disabled items work but semantics are wrong. |
| 4. Drag updates x/y by delta÷SCALE, canvas re-renders, touch-action:none | ✓ PASS | Delta calculation correct, re-render on every move, CSS + preventDefault. |
| 5. Resize handle, 6×6 corner | ✓ PASS | `hitTest` detects resize handle, `onPointerMove` resizes. |
| 6. Snap modes 8/16/1, localStorage persistence | ✓ PASS | Three modes implemented, localStorage key `fw_editor_snap`. |
| 7. Resize floor clamp, FW.showToast | ✓ PASS | `clamp()` enforces min, toast fires once per gesture. |
| 8. ≤20 KB gzip | ✓ PASS | Story Dev Agent Record claims 5,063 bytes total (24.7% of budget). |
| 9. ES5 compliance | ✓ PASS | No arrow functions, let/const, or template literals. `grep` verified empty. |
| 10. Mobile smoke test | ⏭ PENDING | Manual test not yet executed. |

**AC Score:** 8/10 verified ✓, 1/10 violated ✗, 1/10 pending ⏳

---

## SUMMARY OF FINDINGS

### Critical Issues (Must Fix)
1. **AC #3 Violation - No Drag-Drop** (HIGH): Toolbox items are clickable, not draggable. AC requires drag-drop semantics.
2. **Hit Test Boundary Bug** (HIGH): Off-by-one inclusion of widget boundary in hit test (`<=` should be `<`).
3. **Widget Overflow** (HIGH): `addWidget()` doesn't validate widget dimensions against device bounds.

### Medium Issues (Should Fix)
4. **API Response Schema Validation** (MEDIUM): `/api/layout` data not validated before use.
5. **Hit Test Race Condition** (MEDIUM): `selectedIndex` mutated before widget reference captured.
6. **Pointer Capture Ordering** (MEDIUM): `releasePointerCapture` called after dragState clear (semantically backwards).
7. **Toolbox API Response Validation** (MEDIUM): Assumes response is array without checking.
8. **Canvas Resize During Drag** (MEDIUM): `initCanvas()` can resize canvas while drag in progress.

### Low Issues (Nice to Fix)
9. **Floating-Point Snap** (LOW): Math is actually sound; no issue.
10. **Pointer Capture Guard** (LOW): Early return guards against crash but design is fragile.

### Observations
- **Test Coverage:** Zero automated tests; manual AC #10 test pending.
- **SOLID Violations:** Single Responsibility, Open/Closed, Dependency Inversion all compromised.
- **Documentation:** Minimal inline comments, no JSDoc.

---

## EVIDENCE SCORE CALCULATION

**Scoring Rubric (0-5):**
- 5.0 = All ACs met, no defects, excellent code quality
- 4.0 = All ACs met, minor issues only
- 3.0 = Most ACs met, some defects, acceptable quality
- 2.0 = Multiple AC violations, significant defects
- 1.0 = Major AC violations, architectural issues
- 0.0 = Incomplete/unusable

**This Review:**
- AC Compliance: 8/10 verified ✓ = **80%**
- Critical Defects: 3 (AC violation, boundary bug, overflow) = **-1.0**
- Medium Defects: 4 (validation, race, ordering, API check) = **-0.5**
- Code Quality: SOLID violations, no tests = **-0.3**

**Score: 3.2/5.0 — APPROVED WITH RESERVATIONS**

**Interpretation:** Story is functionally usable but contains one AC violation (drag-drop) and two high-severity bugs (hit test, widget overflow) that should be fixed before merge. Test coverage is insufficient.

---

## RECOMMENDATIONS

1. **Blocking Issues (Before Merge):**
   - Fix AC #3: Either implement drag-drop OR update AC to clarify click-to-add is acceptable
   - Fix hit test boundary: Change `<=` to `<` in widget body hit test
   - Fix widget overflow: Add dimension validation in `addWidget()`

2. **High-Priority (Before next release):**
   - Add schema validation for `/api/layout` and `/api/widgets/types` responses
   - Add automated tests for hit test, snap, and pointer event logic
   - Document AC #10 (mobile smoke test) results

3. **Medium-Priority (Refactoring):**
   - Consolidate pointer event handler state management (separate dragState from selectedIndex)
   - Add JSDoc comments to complex functions
   - Extract snap logic into testable utility functions

4. **Low-Priority (Nice to Have):**
   - Add keyboard shortcuts for delete widget, duplicate widget
   - Implement undo/redo (scope creep for LE-1.6, defer to future story)

---

## SIGN-OFF

**Senior Developer:** Adversarial Review Agent  
**Date:** 2024  
**Verdict:** **APPROVED WITH CRITICAL ISSUES**

**Conditions for Merge:**
- [ ] Fix AC #3 (drag-drop or update AC)
- [ ] Fix hit test boundary bug
- [ ] Fix widget overflow validation
- [ ] Add schema validation for API responses
- [ ] Execute and document AC #10 mobile smoke test
- [ ] Add at least 5 unit tests covering hit test, snap, pointer events

**Risk Level:** MEDIUM (AC violation + boundary bugs require fixes; otherwise functional)

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-17</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">le-1</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="712">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">3</var>
<var name="session_id">873ab922-1af3-4237-bc98-b3c47d11312e</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="92fbb450">embedded in prompt, file id: 92fbb450</var>
<var name="story_id">le-1.6</var>
<var name="story_key">le-1-6-editor-canvas-and-drag-drop</var>
<var name="story_num">6</var>
<var name="story_title">6-editor-canvas-and-drag-drop</var>
<var name="template">False</var>
<var name="timestamp">20260417_1605</var>
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
      - Commit message format: fix(component): brief description (synthesis-le-1.6)
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