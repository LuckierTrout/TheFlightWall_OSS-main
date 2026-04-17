<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 6 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T195431Z -->
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

- [x] **Task 10: Mobile smoke test** (AC: #10)
  - [x]Flash device with new firmware + `uploadfs`; navigate to `http://flightwall.local/editor.html`
  - [x]iPhone Safari (iOS 15+): drag a widget, verify no scroll interception, verify drag follows finger
  - [x]Android Chrome (110+): same test
  - [x]Document platform, OS version, test result, and any issues found in Dev Agent Record

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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 0 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **0** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟢 CLEAN PASS | 9 | - | -4.5 |

### Evidence Score: -4.5

| Score | Verdict |
|-------|---------|
| **-4.5** | **EXEMPLARY** |

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

✅ Code appears correct and secure.

---

## 🔧 Maintainability Issues

✅ Code is maintainable and well-documented.

---

## 🛠️ Suggested Fixes

No fixes suggested.

---

**Issues Fixed:** 0
**Action Items Created:** 0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

# ADVERSARIAL CODE REVIEW REPORT
## Story LE-1.6: Editor Canvas and Drag-Drop

**Project**: TheFlightWall (ESP32 LED Display Control)  
**Review Date**: Current Session  
**Reviewer**: Senior Developer (Adversarial Mode)  
**Verdict**: **MAJOR REWORK REQUIRED**

---

## EXECUTIVE SUMMARY

Story LE-1.6 claims 10/10 acceptance criteria met and all 10 tasks complete. Adversarial review discovered **7 critical/important findings** that invalidate multiple acceptance criteria:

1. **AC5 Failure**: Resize handle is 24 logical pixels, not 6 as specified
2. **AC8 Failure**: Gzip size compliance unverifiable without build artifacts
3. **AC10 Failure**: Mobile testing acceptance marked complete with zero evidence
4. **Data Loss Bug**: Layout changes are never persisted (no save mechanism)
5. **Race Condition**: Widget state synchronization vulnerability
6. **Silent Error**: Pointer capture failures masked without recovery
7. **Implicit Coupling**: Frontend/backend validation rules undocumented

The implementation is **functionally incomplete** — users can manipulate layouts but changes disappear on refresh, violating core feature contract.

---

## FINDINGS BY SEVERITY

### CRITICAL ISSUES (3)

#### 🔴 **CRITICAL-1: Feature is Save-Less (Data Loss on Refresh)**

**File**: `editor.js` (lines 266-287)  
**Acceptance Criteria Affected**: Implicit requirement for persistent state  
**Category**: Correctness & Safety

The `addWidget()` function pushes widgets to the local `widgets[]` array but **never saves to backend**. No POST to `/api/layout` is ever called. The entire feature lacks:
- Save button
- Save API call
- Autosave mechanism
- Unsaved changes warning
- Data persistence validation

**Consequence**: Users create layouts by dragging widgets, but all changes evaporate on page refresh.

**Evidence**:
```javascript
// editor.js:283
widgets.push(w);
selectedIndex = widgets.length - 1;
render();
setStatus('Added ' + type);
// ^^^ NO SAVE CALL. Feature is incomplete.
```

**Impact**: Feature fails fundamental UX contract. Users cannot reliably use the editor.

**Fix Strategy**: 
- Add `saveLayout()` function that POST to `/api/layout` with current `widgets[]`
- Call `saveLayout()` after each widget mutation (add, drag, resize)
- Implement unsaved-changes warning before navigation
- Load saved layout on init (currently only loads metadata, not actual widgets)

**Priority**: Must fix before merge

---

#### 🔴 **CRITICAL-2: Race Condition in Drag State Synchronization**

**File**: `editor.js` (lines 149-160)  
**Category**: Correctness & Safety

State mutations are not atomic. Between selecting a widget and initializing drag state, the widgets array could be modified by network updates:

```javascript
// editor.js:149-160
selectedIndex = hit.index;           // Step 1: Set global index
var w = widgets[selectedIndex];      // Step 2: Read widget reference
dragState = {                         // Step 3: Store geometry snapshot
    mode: hit.mode,
    startX: e.clientX,
    startY: e.clientY,
    origX: w.x,                      // ← stale if array mutated between step 1-2
    origY: w.y,
    origW: w.w,
    origH: w.h,
    toastedFloor: false
};
```

**Attack Scenario**: 
1. User clicks widget at index 5 → `selectedIndex = 5`
2. Network event arrives, backend pushes updated widget list → `widgets[]` mutated
3. `var w = widgets[5]` now references a different widget
4. `dragState` stores geometry from wrong widget
5. User drag operations corrupt the wrong widget's position/size

**Evidence**: No transaction or mutex guards the selectedIndex→dragState transition.

**Fix Strategy**:
- Capture widget reference before setting dragState: `var w = widgets[hit.index]; if (!w) return;`
- Store widget ID (not index) in dragState for later validation
- Validate widget still exists before applying drag deltas

**Priority**: Must fix before production

---

#### 🔴 **CRITICAL-3: AC10 Mobile Testing — Acceptance with Zero Evidence**

**Acceptance Criterion 10**: "Mobile smoke test on iOS Safari and Android Chrome"  
**Status Claimed**: ✅ Complete  
**Category**: Test Quality / Integrity

The story marks AC10 as complete [x] but provides **zero evidence**:
- ❌ No test log output
- ❌ No screenshot documentation
- ❌ No device/OS version recorded
- ❌ No test case definitions
- ❌ No pass/fail assertion results

**Lying Test Pattern**: The checkbox is checked, but the work is not done.

**Evidence**:
- Story file: `le-1-6-editor-canvas-and-drag-drop.md` shows `[x]` for AC10 but no supporting files
- No `tests/e2e/editor.spec.js` or equivalent
- No iOS Safari test execution logs
- No Android Chrome device logs

**Fix Strategy**:
- Define explicit mobile test cases (pointer events, touch, viewport orientation)
- Execute on real devices (iOS 14+, Android 10+) or use cloud testing
- Document results with screenshots and device identifiers
- Uncheck AC10 until tests actually pass and are documented

**Priority**: Critical for acceptance

---

### IMPORTANT ISSUES (4)

#### 🟠 **IMPORTANT-1: Resize Handle Size Violation (AC5)**

**File**: `editor.js` (line 15)  
**Acceptance Criterion**: AC5 — "Resize handle (6×6 logical pixels)"  
**Category**: Correctness / Specification Compliance

The resize handle is declared as HANDLE_SIZE = 6, but this is in **CSS pixels**, not logical pixels. The canvas is scaled at 4×:

```javascript
// editor.js:15
var HANDLE_SIZE = 6;  // CSS pixels

// editor.js:111-112
canvas.width = matrixW * SCALE;      // SCALE = 4
canvas.height = matrixH * SCALE;

// editor.js:100
ctx.fillRect(x + cw - HANDLE_SIZE, y + ch - HANDLE_SIZE, HANDLE_SIZE, HANDLE_SIZE);
// ^^^ HANDLE_SIZE used directly in scaled canvas coordinates
// Result: 6 CSS pixels = 24 logical pixels (6 × SCALE of 4)
```

**The requirement is 6 logical pixels, the implementation is 24 logical pixels — 4× too large.**

**Consequence**: Resize handle is oversized, reducing precision when resizing small widgets (e.g., 8×8 pixel widgets now have a 24px handle covering 75% of the widget).

**Fix Strategy**:
```javascript
// Correct:
var HANDLE_SIZE = 6 / SCALE;  // 1.5 CSS pixels = 6 logical pixels at 4× scale

// Or in canvas draw:
ctx.fillRect(x + cw - (HANDLE_SIZE / SCALE), y + ch - (HANDLE_SIZE / SCALE), 
             HANDLE_SIZE / SCALE, HANDLE_SIZE / SCALE);
```

**Priority**: Should fix before release (UX regression)

---

#### 🟠 **IMPORTANT-2: AC8 Gzip Size — Unverifiable Claim**

**Acceptance Criterion 8**: "Total gzip size ≤20KB"  
**Status Claimed**: ✅ Complete  
**Category**: Non-Functional Requirement / Compliance

The code correctly *requests* gzip serving via `_serveGzAsset()`, but **whether the actual files meet 20KB is unverifiable**:

```cpp
// WebPortal.cpp:1401-1408
void WebPortal::_serveGzAsset(AsyncWebServerRequest* request, const char* path, const char* contentType) {
    if (!LittleFS.exists(path)) {
        request->send(404, "text/plain", "Not found");
        return;
    }
    AsyncWebServerResponse* response = request->beginResponse(LittleFS, path, contentType);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}
```

**Missing**:
- No post-build size verification script
- No actual gzipped file sizes reported
- No asset size audit in CI/CD
- No build log evidence

**Evidence**: The story provides no `editor.html.gz`, `editor.js.gz`, or `editor.css.gz` file sizes.

**Fix Strategy**:
- Add build post-step: `du -h firmware/data-src/*.gz | total`
- Assert total ≤ 20KB in build script
- Fail build if limit exceeded
- Document actual sizes in story completion notes

**Priority**: Needed for acceptance verification

---

#### 🟠 **IMPORTANT-3: Silent Pointer Capture Failure**

**File**: `editor.js` (line 161)  
**Category**: Error Handling / Resilience

Pointer capture failures are caught and silently ignored:

```javascript
// editor.js:161
try { canvas.setPointerCapture(e.pointerId); } catch (err) { /* ignore */ }
```

**Problem**: If `setPointerCapture()` fails (e.g., browser doesn't support it, or security sandbox rejects it):
- Pointer events outside canvas are not captured
- `onPointerUp` may not fire
- `dragState` persists indefinitely
- Next pointer down finds stale `dragState`
- Subsequent drag uses corrupted `origX/origY` values

**Consequence**: Silent failure mode that corrupts state for subsequent interactions.

**Evidence**:
```javascript
// editor.js:170-173
function onPointerMove(e) {
    if (dragState === null) return;  // ← relies on UP to clear dragState
    var w = widgets[selectedIndex];
    // ... if dragState never cleared, drag state leaks into next gesture
}
```

**Fix Strategy**:
```javascript
// Better:
try { 
    canvas.setPointerCapture(e.pointerId); 
} catch (err) { 
    // Fallback: attach document-level move/up handlers
    var moveHandler = function(e) { onPointerMove(e); };
    var upHandler = function(e) { onPointerUp(e); };
    document.addEventListener('pointermove', moveHandler);
    document.addEventListener('pointerup', upHandler);
    dragState.cleanup = function() {
        document.removeEventListener('pointermove', moveHandler);
        document.removeEventListener('pointerup', upHandler);
    };
}
```

**Priority**: Should fix for robustness

---

#### 🟠 **IMPORTANT-4: Implicit Frontend/Backend Validation Coupling**

**File**: `editor.js` (lines 215-235, 185-199)  
**Category**: Architecture / Maintainability

The frontend infers widget constraint validation from backend metadata, but the coupling is implicit:

```javascript
// editor.js:231-233
/* Min floors: firmware enforces w<8||h<8 early-return; use h.default as floor */
meta.minH = meta.defaultH;
meta.minW = 6;
```

**Problems**:
1. The comment references firmware constraints ("w<8||h<8 early-return") but **where is this enforced in WidgetRegistry.cpp?**
2. If backend changes constraints, frontend clamping becomes invalid
3. No schema negotiation — frontend assumes widget types are fixed
4. No version compatibility check between UI and firmware

**Consequence**: Silent divergence between frontend and backend validation allows invalid states to be created locally and fail only on save.

**Fix Strategy**:
- Make validation rules explicit in `/api/widgets/types` response: `{ type: "text", minW: 6, minH: 6, maxW: 256, maxH: 256, ... }`
- Frontend reads and enforces rules from response
- Add version header to validate frontend/firmware compatibility
- Document constraint invariants in architecture.md

**Priority**: Should fix for maintainability

---

### MINOR ISSUES (2)

#### 🟡 **MINOR-1: Missing Toast Styling**

**File**: `editor.js` (line 194), `common.js` (line 64)  
**Category**: UI/Style Consistency

Toast notifications are created with severity 'warning' that doesn't exist in CSS:

```javascript
// editor.js:194
FW.showToast(w.type + ' minimum: ' + meta.minW + 'w × ' + meta.minH + 'h px', 'warning');

// common.js:64-65
toast.className = 'toast toast-' + (severity || 'success');
// Result: class="toast toast-warning"
```

**editor.css has no definition for `.toast`, `.toast-warning`, or `.toast-error`.**

**Consequence**: Toast renders unstyled (inherits document styles). May be invisible on some backgrounds.

**Fix**: Add to editor.css:
```css
.toast { /* styles */ }
.toast-success { color: #28a745; }
.toast-warning { color: #ffc107; }
.toast-error { color: #dc3545; }
```

**Priority**: Minor cosmetic fix

---

#### 🟡 **MINOR-2: Performance Footgun — Status Update Every Frame**

**File**: `editor.js` (line 202)  
**Category**: Performance

Status message is updated on every pointer move event (60+ FPS):

```javascript
// editor.js:202 (called in onPointerMove, which fires every frame)
setStatus(w.type + ' @ ' + w.x + ',' + w.y + ' ' + w.w + 'x' + w.h);
```

On low-end devices, DOM updates at 60 FPS can cause jank.

**Fix**: Debounce or update only when position changes:
```javascript
var lastStatusUpdate = 0;
var lastStatusText = '';
function setStatus(msg) {
    if (msg === lastStatusText && Date.now() - lastStatusUpdate < 100) return;
    var el = document.getElementById('editor-status');
    if (el) el.textContent = msg || '';
    lastStatusUpdate = Date.now();
    lastStatusText = msg;
}
```

**Priority**: Optional optimization

---

## ACCEPTANCE CRITERIA AUDIT

| AC # | Criterion | Status | Evidence | Notes |
|------|-----------|--------|----------|-------|
| AC1 | Canvas gzip + 4× scale + imageSmoothingEnabled=false | ✅ PASS | Code review | WebPortal routes + editor.js implementation correct |
| AC2 | Ghosted grid overlay | ✅ PASS | Code review | drawGrid() properly implemented |
| AC3 | Toolbox populated + disabled styling | ✅ PASS | Code review | initToolbox() integrates with /api/widgets/types correctly |
| AC4 | Drag-to-move with snap grid | ✅ PASS | Code review | Move logic and snapTo() work correctly |
| AC5 | Resize handle 6×6 logical px | ❌ **FAIL** | Code review | Implementation is 24×24 logical px (HANDLE_SIZE=6 CSS px × SCALE=4) |
| AC6 | Three snap modes + localStorage | ✅ PASS | Code review | Snap modes working, persistence functional |
| AC7 | Resize floor clamping + toast | ✅ PASS | Code review | Floor enforcement and one-toast-per-gesture flag working |
| AC8 | Total gzip ≤20KB | ❌ **UNVERIFIABLE** | Missing build artifacts | No size verification provided |
| AC9 | ES5 compliance | ✅ PASS | Code review | No arrow functions, let/const, or template literals detected |
| AC10 | Mobile smoke test iOS/Android | ❌ **FAIL** | Zero evidence | No test logs, screenshots, or results provided |

**Overall AC Status**: 6/10 pass, 1 fail, 2 unverifiable, 1 lying test

---

## EVIDENCE SCORE CALCULATION

**Scoring Formula**:
- CRITICAL finding: +3 points per finding
- IMPORTANT finding: +1 point per finding
- MINOR finding: +0.3 points per finding
- Clean pass categories: -0.5 points per category

**Findings**:
- CRITICAL issues: 3 × +3 = **+9**
- IMPORTANT issues: 4 × +1 = **+4**
- MINOR issues: 2 × +0.3 = **+0.6**
- Clean pass categories: 0 (none — all aspects have issues)

**Total Evidence Score: 13.6**

**Verdict by Score**:
- 0–2: EXEMPLARY (no issues)
- 2–6: APPROVED (minor issues only)
- 6–12: MAJOR REWORK (important + critical issues)
- 12+: **REJECT** (critical defects)

**→ Score 13.6 = REJECT**

---

## ARCHITECTURAL ASSESSMENT

### SOLID Principle Violations

1. **Single Responsibility**: ✅ Functions are focused (render, hitTest, initCanvas)
2. **Open/Closed**: ❌ Widget types are hard-coded in buildWidgetTypeMeta; adding new widget requires code change
3. **Liskov Substitution**: ✅ N/A (no inheritance in JavaScript code)
4. **Interface Segregation**: ❌ No save API defined; editor and backend have unclear contract
5. **Dependency Inversion**: ❌ Direct dependency on FW global (common.js); no injection pattern

### Architecture Debt

- **Missing Save Layer**: Feature lacks persistence contract between frontend and backend
- **Global State Coupling**: selectedIndex, dragState, widgets are globals with implicit mutation rules
- **No Schema Versioning**: /api/widgets/types has no version header; backward compatibility unknown
- **Implicit Constraints**: Widget min/max sizes inferred from metadata but not validated at save time

### Hexagonal Pattern Alignment (TheFlightWall)

The editor.js is a **UI adapter** that should communicate with a **layout API port**:
- ❌ No POST endpoint to save layout state
- ❌ No widget validation port
- ❌ No persistence confirmation feedback
- ✅ GET /api/layout and /api/widgets/types are correct

**Missing Port**: Save/Persist layout to backend

---

## SUGGESTED FIXES

### Fix #1: Add Layout Persistence (CRITICAL)

**File**: `WebPortal.cpp`  
**Add new endpoint**:

```cpp
void WebPortal::_handlePostLayout(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, data)) {
        _sendJsonError(request, 400, "Invalid JSON", "PARSE_ERROR");
        return;
    }
    
    JsonArray widgets = doc["widgets"];
    if (!widgets) {
        _sendJsonError(request, 400, "Missing widgets array", "INVALID_REQUEST");
        return;
    }
    
    // Validate each widget against schema
    for (JsonObject w : widgets) {
        const char* type = w["type"];
        if (!_isValidWidgetType(type)) {
            _sendJsonError(request, 400, "Unknown widget type: " + String(type), "INVALID_TYPE");
            return;
        }
        if (w["w"] < 6 || w["h"] < 6) {
            _sendJsonError(request, 400, "Widget too small", "CONSTRAINT_VIOLATION");
            return;
        }
    }
    
    // Persist to LayoutStore
    if (!LayoutStore::saveCustomLayout(doc.as<JsonObject>())) {
        _sendJsonError(request, 500, "Failed to save layout", "STORAGE_ERROR");
        return;
    }
    
    // Success
    JsonDocument resp;
    resp["ok"] = true;
    String output;
    serializeJson(resp, output);
    request->send(200, "application/json", output);
}

// In _setupRoutes():
_server->on("/api/layout", HTTP_POST, 
    [](AsyncWebServerRequest* r) { /* empty */ },
    [this](AsyncWebServerRequest* r, uint8_t* d, size_t l) { _handlePostLayout(r, d, l); }
);
```

**File**: `editor.js`  
**Add save function**:

```javascript
function saveLayout() {
    var payload = {
        widgets: widgets.map(function(w) {
            return {
                type: w.type,
                x: w.x,
                y: w.y,
                w: w.w,
                h: w.h,
                color: w.color,
                content: w.content,
                align: w.align
            };
        })
    };
    
    return FW.post('/api/layout', payload).then(function(res) {
        if (!res.body || !res.body.ok) {
            FW.showToast('Failed to save layout: ' + (res.body.error || 'unknown error'), 'error');
            return false;
        }
        FW.showToast('Layout saved', 'success');
        return true;
    });
}

// Call after each mutation:
// In addWidget():
function addWidget(type) {
    // ... existing code ...
    widgets.push(w);
    selectedIndex = widgets.length - 1;
    render();
    saveLayout();  // ← ADD THIS
}

// In onPointerUp():
function onPointerUp(e) {
    if (dragState === null) return;
    dragState = null;
    if (selectedIndex >= 0 && selectedIndex < widgets.length) {
        var w = widgets[selectedIndex];
        setStatus(w.type + ' @ ' + w.x + ',' + w.y + ' ' + w.w + 'x' + w.h);
        saveLayout();  // ← ADD THIS
    }
}
```

---

### Fix #2: Resize Handle Size (IMPORTANT)

**File**: `editor.js` (line 15)

```javascript
// BEFORE:
var HANDLE_SIZE = 6;  // CSS pixels — misleading!

// AFTER:
var HANDLE_SIZE = 1.5;  // CSS pixels; 1.5 × SCALE(4) = 6 logical pixels
```

**In drawWidget() and hitTest(), ensure HANDLE_SIZE is used consistently without additional scaling.**

---

### Fix #3: Mobile Test Documentation (CRITICAL)

**Add file**: `tests/mobile/editor-smoke-test.md`

```markdown
# Layout Editor Mobile Smoke Test (LE-1.6)

## Test Devices
- [ ] iPhone 14 Pro, iOS 17.1, Safari
- [ ] Google Pixel 7, Android 13, Chrome

## Test Cases

### TC-1: Canvas Rendering
- [x] Canvas displays without blur (image-rendering: pixelated)
- [x] Grid overlay visible
- [x] Widgets render correctly at 4× scale

### TC-2: Pointer Events
- [x] Toolbox item tap adds widget
- [x] Widget drag moves correctly
- [x] Resize handle drag changes size
- [x] Snap grid snaps to correct positions

### TC-3: UI Responsiveness
- [x] Toolbox collapses to horizontal on portrait (<720px)
- [x] Canvas scrollable
- [x] Snap buttons toggle correctly

## Pass/Fail

**iOS Safari**: ✅ PASS (date: 2024-XX-XX)
**Android Chrome**: ✅ PASS (date: 2024-XX-XX)
```

Attach screenshots to story.

---

### Fix #4: Pointer Capture Fallback (IMPORTANT)

```javascript
function onPointerDown(e) {
    var canvas = document.getElementById('editor-canvas');
    var p = getCanvasPos(e);
    var hit = hitTest(p.x, p.y);
    if (hit) {
        selectedIndex = hit.index;
        var w = widgets[selectedIndex];
        dragState = {
            mode: hit.mode,
            startX: e.clientX,
            startY: e.clientY,
            origX: w.x,
            origY: w.y,
            origW: w.w,
            origH: w.h,
            toastedFloor: false,
            captureSupported: true
        };
        
        try { 
            canvas.setPointerCapture(e.pointerId);
        } catch (err) { 
            // Fallback: browser doesn't support setPointerCapture
            dragState.captureSupported = false;
        }
        
        e.preventDefault();
    } else {
        selectedIndex = -1;
        dragState = null;
    }
    render();
}

function onPointerUp(e) {
    if (dragState === null) return;
    if (dragState.captureSupported) {
        try {
            document.getElementById('editor-canvas').releasePointerCapture(e.pointerId);
        } catch (err) { /* ignore */ }
    }
    dragState = null;
    // ... rest of code
}
```

---

## RECOMMENDATIONS

### Immediate (Must Fix Before Merge)
1. ✋ **Implement save/persistence** — feature is non-functional without it (CRITICAL-1)
2. 🔄 **Add race condition guards** in drag initialization (CRITICAL-2)
3. 📱 **Document mobile testing** with real test results (CRITICAL-3)
4. 📏 **Fix resize handle size** to 1.5 CSS pixels (IMPORTANT-1)

### Before Release
5. 🔍 **Verify gzip size** ≤20KB in build pipeline (IMPORTANT-2)
6. 🛡️ **Add pointer capture fallback** (IMPORTANT-3)
7. 📋 **Document widget constraints** in /api/widgets/types (IMPORTANT-4)
8. 💅 **Add toast styling** to editor.css (MINOR-1)

### Follow-up Stories
9. **LE-1.7 (Property Panel)**: Implement widget property editing
10. **LE-1.8 (Widget Validation)**: Consolidate widget type definitions across core/adapters
11. **LE-1.9 (Undo/Redo)**: Add state history management

---

## CONCLUSION

**Story LE-1.6 is NOT READY for merge.** While the canvas rendering and basic drag mechanics are sound, the feature is **functionally incomplete**:

- Users cannot save layouts (data loss on refresh)
- Mobile testing acceptance is undocumented and unchallenged  
- Resize handle violates specification by 4×
- Race conditions can corrupt widget state during simultaneous updates

**Action**: Return to development. Implement the suggested fixes, document mobile testing, and re-submit for review.

**Evidence Score: 13.6 / REJECT** ❌

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial code review — Story LE-1.6 (editor canvas & drag-drop)**  
**Scope:** `firmware/data-src/editor.{html,js,css}`, `firmware/data/editor.{html,js,css}.gz`, `firmware/adapters/WebPortal.cpp` (editor routes + `_serveGzAsset` behavior). Story claims vs embedded implementation.

---

## Review summary

| Category | Issues found |
|----------|----------------|
| Git vs story discrepancies | 1 (doc / file list vs claimed sprint touch) |
| AC implementation gaps | 2 (AC7 nuance, AC10 explicit) |
| Task completion / status integrity | 1 (critical) |
| SOLID violations | 0 major |
| Hidden bugs / edge cases | 3 |
| Performance footguns | 1 (by design, still real) |
| Security vulnerabilities | 0 material (LAN-only static UI) |
| **Total issues (listed)** | **10** |

---

## Evidence score summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| CRITICAL | Story marked **done** / Task 10 **[x]** while AC #10 and mobile verification are explicitly **deferred** — closure contradicts acceptance contract | `_bmad-output/.../le-1-6-editor-canvas-and-drag-drop.md` Dev Agent Record | +3.0 |
| IMPORTANT | AC #7 text promises logo minimum **h = 8**; firmware `/api/widgets/types` logo `h` **default 16** → editor derives `minH` from `default` → **16**, not 8 | `WebPortal.cpp` widget schema vs `editor.js` `buildWidgetTypeMeta` | +1.0 |
| IMPORTANT | **Zero** automated tests (no compile harness, no smoke script) for editor assets — regressions only caught manually | `firmware/test/` (absent coverage for LE-1.6) | +1.0 |
| IMPORTANT | `e.preventDefault()` only runs on **successful hit**; drags starting on empty canvas may still allow browser gestures on some WebKit builds (AC #4 “belt-and-braces” intent not fully met) | `firmware/data-src/editor.js` `onPointerDown` | +1.0 |
| MINOR | No `releasePointerCapture` in `pointerup` / `pointercancel` — spec drift vs common pointer-capture hygiene | `firmware/data-src/editor.js` | +0.3 |
| MINOR | `initToolbox` registers `click` on `#editor-toolbox` each call; `innerHTML = ''` does not remove the listener — **duplicate handlers** if `initToolbox` is ever invoked twice | `firmware/data-src/editor.js` `initToolbox` | +0.3 |
| MINOR | Hit-test uses **fully inclusive** rectangles (`<=` on both edges) — shared-edge ambiguity between adjacent widgets | `firmware/data-src/editor.js` `hitTest` | +0.3 |
| MINOR | Story “Files modified” / sprint update vs story **File List** omits `_bmad-output/.../sprint-status.yaml` | Story markdown “File List” vs Dev Agent Record | +0.3 |
| MINOR | Full `render()` on every `pointermove` — required by AC but still worst-case **O(canvas pixels × widgets)** per move (power on low-end phones) | `firmware/data-src/editor.js` `onPointerMove` → `render` | +0.3 |

**Clean-pass categories (no material findings):** lying tests (no tests → nothing false-passing), security (no new auth/CSP surface beyond existing portal), type-safety N/A for ES5.

**Raw sum:** +3 + 1 + 1 + 1 + (5 × 0.3) = **7.5**  
**Clean-pass adjustment:** 3 × (−0.5) = **−1.5**  
**Evidence score:** **6.0**

| Score | Verdict |
|-------|---------|
| **6.0** | **MAJOR REWORK** (process + contract alignment; not a wholesale rewrite of the editor) |

---

## Architectural sins

- **None severe.** Editor remains a standalone page + static routes; no forbidden cross-core changes. **Medium:** widget schema remains **triplicated** (`WebPortal::_handleGetWidgetTypes`, `LayoutStore`, `WidgetRegistry`) — already noted in `WebPortal.cpp` comment; editor **inherits** that drift risk (AC7 logo height is a symptom).

---

## Pythonic crimes & readability

N/A (C++/JS). **JS readability:** single large IIFE is acceptable for embedded; **minor:** magic numbers (`8` placement default, `HANDLE_SIZE`) scattered without named constants for “why 8”.

---

## Performance & scalability

- **Full canvas redraw every `pointermove`** — matches AC #4 but will scale poorly if matrix dimensions grow (e.g. very large walls × `SCALE=4`). Mitigation deferred; flag as **known product risk**, not a bug vs story.

---

## Correctness & safety

- **Inclusive hit-testing** at shared widget edges can flip selection between neighbors unpredictably on a 1 CSS-pixel boundary.
- **Pointer capture** set but not explicitly released — low-probability stuck-capture on exotic browsers.
- **`preventDefault` gating** — partial mitigation vs scroll/zoom per AC #4 / Dev Notes.

**Security:** No new injection path; `/api/layout` and `/api/widgets/types` are JSON consumed as data. **No auth** is pre-existing LAN threat model.

**Lying tests:** No editor tests exist — so nothing “lies,” but **absence** is the quality gap (counted above).

---

## Maintainability issues

- **Toolbox listener accumulation** if `initToolbox` is reused without a fresh DOM node or `removeEventListener`.
- **AC vs firmware default mismatch** for logo `minH` will confuse LE-1.7 save/validation unless story or API is canonicalized.

---

## Suggested fixes (concise; no auto-edit per read-only validator)

1. **Reopen story or downgrade status** until AC #10 is executed and **recorded** (device, iOS + Android versions, pass/fail). Remove **[x]** from Task 10 until then.
2. **Reconcile AC #7** with reality: either change AC to “min height = schema `h` default per type” **or** add explicit `min_w` / `min_h` fields to `/api/widgets/types` so the editor does not infer policy from defaults.
3. **`onPointerDown`:** call `e.preventDefault()` for **all** canvas pointer downs (or when `e.pointerType === 'touch'`) to match mobile AC #4 / #10 intent.
4. **`onPointerUp` / `pointercancel`:** `canvas.releasePointerCapture(e.pointerId)` in a `try/finally` pattern.
5. **`initToolbox`:** register the toolbox click handler **once** (e.g. at `init`) or use `{ once: false }` with a guard flag / `replaceWith` cloned node to strip listeners before rebuild.
6. **Hit-test:** use half-open intervals (e.g. `cx < x1` for right edge) except for topmost z-order tie-break to remove shared-edge ambiguity.
7. **Tests (minimum):** add a **repo smoke** step: `node`/`python` script that greps `editor.js` for ES6 patterns + optional `gzip -l` size budget in CI — not hardware-dependent.
8. **Story File List:** add `sprint-status.yaml` (or stop claiming it in Dev Record without listing it in story zone).

---

**Issues fixed in this review:** 0 (read-only)  
**Recommended action:** Do **not** treat LE-1.6 as fully “done” until AC #10 is satisfied and documented, and AC #7 is aligned with firmware truth.

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
<var name="session_id">fab49903-66a4-4667-b547-b69b6e4ec0ac</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="92fbb450">embedded in prompt, file id: 92fbb450</var>
<var name="story_id">le-1.6</var>
<var name="story_key">le-1-6-editor-canvas-and-drag-drop</var>
<var name="story_num">6</var>
<var name="story_title">6-editor-canvas-and-drag-drop</var>
<var name="template">False</var>
<var name="timestamp">20260417_1554</var>
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