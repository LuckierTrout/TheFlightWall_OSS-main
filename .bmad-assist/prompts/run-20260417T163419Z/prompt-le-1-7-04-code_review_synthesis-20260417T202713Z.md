<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 7 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T202713Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story le-1.7

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
<file id="ca498508" path="_bmad-output/implementation-artifacts/stories/le-1-7-editor-property-panel-and-save-ux.md" label="UX DESIGN"><![CDATA[


# Story LE-1.7: Editor Property Panel and Save UX

branch: le-1-7-editor-property-panel-and-save-ux
zone:
  - firmware/data-src/editor.html
  - firmware/data-src/editor.js
  - firmware/data-src/editor.css
  - firmware/data-src/common.js
  - firmware/data/editor.html.gz
  - firmware/data/editor.js.gz
  - firmware/data/editor.css.gz
  - firmware/data/common.js.gz
  - tests/smoke/test_editor_smoke.py

Status: ready-for-dev

## Story

As a **layout author on my phone**,
I want **a property panel that shows per-widget controls when I select a widget, and Save / Save & Activate buttons that persist my layout to the device**,
So that **I can configure widget content, color, and alignment without guessing field names, then push the layout to the LED wall in under 30 seconds**.

## Acceptance Criteria

1. **Given** a widget is selected on the canvas **When** the property panel renders **Then** it shows a field-per-type set driven by the `fields` array from `GET /api/widgets/types`: `content` (text input for text/logo/flight_field/metric; `<select>` for clock with options "12h"/"24h"), `color` (color input, hex string), and `align` (three-segment L/C/R — text type only). Position and dimension fields (x, y, w, h) are **read-only** numeric display only — they are controlled by drag/resize on the canvas, not typed in the panel.

2. **Given** no widget is selected **When** the property panel renders **Then** it shows the placeholder message "Select a widget" (the `<p class="props-placeholder">` element currently in the markup) and all form controls are hidden.

3. **Given** the user changes any property panel control **When** the change event fires **Then** the corresponding field on the in-memory widget object is immediately mutated, `render()` is called to redraw the canvas, and the `dirty` flag is set to `true`.

4. **Given** the editor has been opened to a new, unsaved layout **When** the user taps "Save as New" **Then**: (a) the name field (text input in the control bar) is validated — it must be non-empty and ≤ 32 characters, otherwise a `FW.showToast` error fires and the save is aborted; (b) `FW.post('/api/layouts', body)` is called where `body` is the full layout JSON object; (c) on success, the returned `data.id` is stored as the current layout id, the `dirty` flag is cleared, and `FW.showToast('Layout saved', 'success')` fires.

5. **Given** the editor has a known layout id (was previously saved or loaded) **When** the user taps "Save" **Then** `FW.put('/api/layouts/' + id, body)` is called; on success the `dirty` flag is cleared and `FW.showToast('Layout saved', 'success')` fires. If the id is unknown (new layout) the "Save" button behaves identically to "Save as New".

6. **Given** a save has just succeeded and the user taps "Save & Activate" (which combines save + activate in one action) **When** the activate step runs **Then** `FW.post('/api/layouts/' + id + '/activate', {})` is called; on success `FW.showToast('Layout active', 'success')` fires and the LED wall switches to `custom_layout` mode within 2 seconds (the ESP32 ModeOrchestrator switch is sub-500 ms; HTTP round-trip adds < 200 ms on local network).

7. **Given** any save or activate API call fails (non-2xx HTTP or `ok: false` in the JSON envelope) **When** the error response arrives **Then** `FW.showToast('<action> failed — <error field from response body>', 'error')` fires; the `dirty` flag is **not** cleared; no further actions in the chain execute.

8. **Given** the `dirty` flag is `true` **When** the user navigates away from the page (triggers `beforeunload`) **Then** the browser native confirmation dialog is presented; the user can choose to stay or leave. The `dirty` flag is also checked before any destructive action (deleting a widget, loading a different layout) — if dirty, a `window.confirm()` dialog asks "Discard unsaved changes?"; the action proceeds only if confirmed.

9. **Given** `common.js` is inspected **When** `FW.put` is looked up **Then** it is found to be a function that issues `fetch(url, {method:'PUT', headers:{'Content-Type':'application/json'}, body:JSON.stringify(data)})` and returns `{status, body}` — the same contract as `FW.post`. If `FW.put` is absent from the current `common.js`, it must be added in this story.

10. **Given** a Python smoke test at `tests/smoke/test_editor_smoke.py` **When** run against a live device **Then** it: (a) GETs `/editor.html` and verifies HTTP 200 with `Content-Encoding: gzip`; (b) POSTs a canned layout JSON to `/api/layouts` and verifies HTTP 200 with `ok: true`; (c) POSTs to `/api/layouts/<returned-id>/activate` and verifies HTTP 200 with `ok: true`; (d) GETs `/api/status` and verifies `data.display_mode.active` equals `"custom_layout"` (or similar — see Dev Notes for exact field path).

11. **Given** `editor.js` after all modifications **When** inspected for ES5 compliance **Then** `grep -E "=>|let |const |` (backtick)` over `firmware/data-src/editor.js` returns empty — no arrow functions, no `let`, no `const`, no template literals.

12. **Given** the three gzip assets **When** measured after gzip-9 compression **Then** `editor.html.gz + editor.js.gz + editor.css.gz` total ≤ 30 KB compressed (budget increase from LE-1.6's 20 KB to accommodate property panel HTML/CSS; LE-1.6 used only 5,063 bytes so there is ≥ 25 KB headroom remaining).

## Tasks / Subtasks

- [ ] **Task 1: Add `FW.put` to `common.js` if absent** (AC: #9)
  - [ ] Open `firmware/data-src/common.js`; check if a `put` function is exported from the `FW` IIFE
  - [ ] If absent, add (ES5 only — no `const`/`let`/arrow):
    ```javascript
    function put(url, data) {
      return fetchJson(url, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
      });
    }
    ```
    and include `put: put` in the return object
  - [ ] Regenerate `firmware/data/common.js.gz`:
    ```bash
    gzip -9 -c firmware/data-src/common.js > firmware/data/common.js.gz
    ```
  - [ ] Verify `FW.put` is now accessible from `editor.js` during integration
  - [ ] **Do NOT** remove or alter existing `FW.get`, `FW.post`, `FW.del`, or `FW.showToast` — only add `put`

- [ ] **Task 2: Property panel HTML in `editor.html`** (AC: #1, #2)
  - [ ] Open `firmware/data-src/editor.html`; replace the property panel placeholder comment inside `#editor-props`:
    ```html
    <div class="editor-props" id="editor-props">
      <p class="props-placeholder" id="props-placeholder">Select a widget</p>
      <div class="props-form" id="props-form" style="display:none">
        <div class="props-field" id="props-field-content">
          <label class="props-label" for="prop-content">Content</label>
          <input  type="text" id="prop-content"  class="props-input" autocomplete="off">
          <select id="prop-content-select" class="props-input" style="display:none"></select>
        </div>
        <div class="props-field" id="props-field-color">
          <label class="props-label" for="prop-color">Color</label>
          <input  type="color" id="prop-color" class="props-color-input" value="#ffffff">
        </div>
        <div class="props-field" id="props-field-align">
          <label class="props-label">Align</label>
          <div class="align-control" id="align-control">
            <button type="button" class="align-btn" data-align="left">L</button>
            <button type="button" class="align-btn" data-align="center">C</button>
            <button type="button" class="align-btn" data-align="right">R</button>
          </div>
        </div>
        <div class="props-field props-readout" id="props-field-pos">
          <label class="props-label">Position</label>
          <span id="prop-pos-readout" class="props-readout-value">—</span>
        </div>
        <div class="props-field props-readout" id="props-field-size">
          <label class="props-label">Size</label>
          <span id="prop-size-readout" class="props-readout-value">—</span>
        </div>
      </div>
    </div>
    ```
  - [ ] Add layout name input + save buttons to the `editor-controls` bar (before `#snap-control`):
    ```html
    <input type="text" id="layout-name" class="layout-name-input" placeholder="Layout name" maxlength="32" autocomplete="off">
    <div class="save-control">
      <button type="button" id="btn-save-new" class="save-btn">Save as New</button>
      <button type="button" id="btn-save"     class="save-btn">Save</button>
      <button type="button" id="btn-activate" class="save-btn save-btn-primary">Save &amp; Activate</button>
    </div>
    ```
  - [ ] Script tags remain at bottom: `/common.js` then `/editor.js` — no changes needed

- [ ] **Task 3: Property panel CSS in `editor.css`** (AC: #1, #2)
  - [ ] Open `firmware/data-src/editor.css`; append styles for the new classes:
    ```css
    /* --- Property panel form --- */
    .props-form { display: flex; flex-direction: column; gap: 10px; }
    .props-field { display: flex; flex-direction: column; gap: 4px; }
    .props-label { font-size: 0.8rem; color: #8b949e; text-transform: uppercase; letter-spacing: 0.04em; }
    .props-input { background: #0d1117; border: 1px solid #30363d; border-radius: 4px; color: #c9d1d9; padding: 5px 8px; font-size: 0.9rem; width: 100%; box-sizing: border-box; }
    .props-input:focus { outline: none; border-color: #58a6ff; }
    .props-color-input { height: 36px; padding: 2px; background: #0d1117; border: 1px solid #30363d; border-radius: 4px; cursor: pointer; width: 100%; box-sizing: border-box; }
    .props-readout { }
    .props-readout-value { font-size: 0.85rem; color: #c9d1d9; font-variant-numeric: tabular-nums; }
    /* --- Align control (L/C/R segment) --- */
    .align-control { display: inline-flex; border: 1px solid #30363d; border-radius: 4px; overflow: hidden; width: 100%; }
    .align-btn { flex: 1; padding: 6px 0; background: #21262d; color: #c9d1d9; border: none; border-right: 1px solid #30363d; cursor: pointer; font-size: 0.9rem; }
    .align-btn:last-child { border-right: none; }
    .align-btn:hover { background: #2d333b; }
    .align-btn.active { background: var(--accent, #58a6ff); color: #000; }
    /* --- Layout name + save control bar additions --- */
    .layout-name-input { background: #0d1117; border: 1px solid #30363d; border-radius: 4px; color: #c9d1d9; padding: 6px 10px; font-size: 0.9rem; width: 140px; box-sizing: border-box; }
    .layout-name-input:focus { outline: none; border-color: #58a6ff; }
    .save-control { display: inline-flex; gap: 6px; }
    .save-btn { padding: 6px 12px; background: #21262d; color: #c9d1d9; border: 1px solid #30363d; border-radius: 4px; cursor: pointer; font-size: 0.9rem; }
    .save-btn:hover { background: #2d333b; }
    .save-btn-primary { background: #238636; border-color: #2ea043; color: #fff; }
    .save-btn-primary:hover { background: #2ea043; }
    /* --- Mobile: hide props panel if canvas is narrow --- */
    @media (max-width: 720px) {
      .editor-props { display: none; }
      .layout-name-input { width: 100px; }
    }
    ```

- [ ] **Task 4: Property panel JS in `editor.js` — panel show/hide and field population** (AC: #1, #2, #3)
  - [ ] Add module-level globals (at the top of the IIFE, with the existing vars):
    ```javascript
    var dirty = false;            /* true when widgets changed since last save */
    var currentLayoutId = null;   /* string id after first save, or null for new */
    ```
  - [ ] Add `showPropsPanel(widget)` function — populates fields from a widget object:
    ```javascript
    function showPropsPanel(w) {
      var placeholder = document.getElementById('props-placeholder');
      var form = document.getElementById('props-form');
      if (!w) {
        if (placeholder) placeholder.style.display = '';
        if (form) form.style.display = 'none';
        return;
      }
      if (placeholder) placeholder.style.display = 'none';
      if (form) form.style.display = '';

      /* content field — text input or select depending on widget type */
      var meta = widgetTypeMeta[w.type] || {};
      var contentField  = document.getElementById('props-field-content');
      var contentInput  = document.getElementById('prop-content');
      var contentSelect = document.getElementById('prop-content-select');
      if (contentField) {
        /* clock uses a select for "12h"/"24h"; all others use text */
        var useSelect = (w.type === 'clock');
        contentInput.style.display  = useSelect ? 'none' : '';
        contentSelect.style.display = useSelect ? '' : 'none';
        if (useSelect) {
          /* Rebuild options only if needed */
          if (contentSelect.options.length === 0) {
            var clockOpts = ['12h', '24h'];
            for (var i = 0; i < clockOpts.length; i++) {
              var opt = document.createElement('option');
              opt.value = clockOpts[i];
              opt.textContent = clockOpts[i];
              contentSelect.appendChild(opt);
            }
          }
          contentSelect.value = w.content || '24h';
        } else {
          contentInput.value = w.content || '';
        }
      }

      /* color */
      var colorInput = document.getElementById('prop-color');
      if (colorInput) colorInput.value = w.color || '#ffffff';

      /* align — only shown for text widget */
      var alignField = document.getElementById('props-field-align');
      if (alignField) {
        alignField.style.display = (w.type === 'text') ? '' : 'none';
        updateAlignButtons(w.align || 'left');
      }

      /* position + size readouts */
      var posReadout  = document.getElementById('prop-pos-readout');
      var sizeReadout = document.getElementById('prop-size-readout');
      if (posReadout)  posReadout.textContent  = w.x + ', ' + w.y;
      if (sizeReadout) sizeReadout.textContent = w.w + ' × ' + w.h;
    }
    ```
  - [ ] Add `updateAlignButtons(align)` function:
    ```javascript
    function updateAlignButtons(align) {
      var btns = document.querySelectorAll('#align-control .align-btn');
      for (var i = 0; i < btns.length; i++) {
        var b = btns[i];
        if (b.getAttribute('data-align') === align) {
          b.className = 'align-btn active';
        } else {
          b.className = 'align-btn';
        }
      }
    }
    ```
  - [ ] Call `showPropsPanel(widgets[selectedIndex] || null)` at the end of `onPointerDown` (replacing `render()` call sequence — render first, then show panel)
  - [ ] Call `showPropsPanel(null)` when `selectedIndex = -1` (miss in `onPointerDown`)
  - [ ] After `addWidget()` succeeds, call `showPropsPanel(widgets[selectedIndex])`
  - [ ] Update position/size readouts during drag: in `onPointerMove`, after updating widget state, refresh the readout spans:
    ```javascript
    var posReadout  = document.getElementById('prop-pos-readout');
    var sizeReadout = document.getElementById('prop-size-readout');
    if (posReadout && selectedIndex >= 0 && widgets[selectedIndex]) {
      var sw = widgets[selectedIndex];
      posReadout.textContent  = sw.x + ', ' + sw.y;
      sizeReadout.textContent = sw.w + ' \u00d7 ' + sw.h;
    }
    ```

- [ ] **Task 5: Property panel JS — control event handlers** (AC: #3)
  - [ ] Add `bindPropsPanelEvents()` function (call from `init()`):
    ```javascript
    function bindPropsPanelEvents() {
      var contentInput  = document.getElementById('prop-content');
      var contentSelect = document.getElementById('prop-content-select');
      var colorInput    = document.getElementById('prop-color');
      var alignCtrl     = document.getElementById('align-control');

      if (contentInput) {
        contentInput.addEventListener('input', function() {
          if (selectedIndex < 0 || !widgets[selectedIndex]) return;
          widgets[selectedIndex].content = contentInput.value;
          dirty = true;
          render();
        });
      }
      if (contentSelect) {
        contentSelect.addEventListener('change', function() {
          if (selectedIndex < 0 || !widgets[selectedIndex]) return;
          widgets[selectedIndex].content = contentSelect.value;
          dirty = true;
          render();
        });
      }
      if (colorInput) {
        colorInput.addEventListener('input', function() {
          if (selectedIndex < 0 || !widgets[selectedIndex]) return;
          widgets[selectedIndex].color = colorInput.value;
          dirty = true;
          render();
        });
      }
      if (alignCtrl) {
        alignCtrl.addEventListener('click', function(e) {
          var t = e.target;
          if (!t || !t.classList || !t.classList.contains('align-btn')) return;
          if (selectedIndex < 0 || !widgets[selectedIndex]) return;
          if (widgets[selectedIndex].type !== 'text') return;
          var a = t.getAttribute('data-align');
          widgets[selectedIndex].align = a;
          dirty = true;
          updateAlignButtons(a);
          render();
        });
      }
    }
    ```
  - [ ] **Important:** The `dirty = true` line must also be set in `onPointerUp` after a completed drag/resize — add `if (dragState) dirty = true;` at the top of `onPointerUp` before clearing `dragState`

- [ ] **Task 6: Save / Save-as-New / Save & Activate buttons** (AC: #4, #5, #6, #7)
  - [ ] Add `getLayoutBody()` helper — assembles the full layout JSON object from current editor state:
    ```javascript
    function getLayoutBody() {
      var nameEl = document.getElementById('layout-name');
      var name = (nameEl && nameEl.value) ? nameEl.value : 'My Layout';
      var id = currentLayoutId || ('layout-' + Date.now());
      return {
        id: id,
        name: name,
        version: 1,
        canvas: { width: deviceW, height: deviceH },
        widgets: widgets
      };
    }
    ```
  - [ ] Add `saveLayout(andActivate)` function — the core save path:
    ```javascript
    function saveLayout(andActivate) {
      var nameEl = document.getElementById('layout-name');
      var name = nameEl ? nameEl.value.trim() : '';
      if (!name) {
        FW.showToast('Enter a layout name before saving', 'error');
        return;
      }
      var body = getLayoutBody();
      var savePromise;
      if (currentLayoutId) {
        /* PUT — overwrite existing */
        savePromise = FW.put('/api/layouts/' + currentLayoutId, body);
      } else {
        /* POST — create new */
        savePromise = FW.post('/api/layouts', body);
      }
      savePromise.then(function(res) {
        if (!res || !res.body || !res.body.ok) {
          var errMsg = (res && res.body && res.body.error) ? res.body.error : 'Unknown error';
          FW.showToast('Save failed \u2014 ' + errMsg, 'error');
          return null;
        }
        /* Store returned id (from POST response; PUT echoes the same id) */
        if (res.body.data && res.body.data.id) {
          currentLayoutId = res.body.data.id;
        }
        dirty = false;
        FW.showToast('Layout saved', 'success');
        if (!andActivate) return null;
        /* Chain activate */
        FW.showToast('Applying\u2026', 'success');
        return FW.post('/api/layouts/' + currentLayoutId + '/activate', {});
      }).then(function(res) {
        if (res === null || res === undefined) return;
        if (!res.body || !res.body.ok) {
          var errMsg = (res.body && res.body.error) ? res.body.error : 'Unknown error';
          FW.showToast('Activate failed \u2014 ' + errMsg, 'error');
          return;
        }
        FW.showToast('Layout active', 'success');
      }).catch(function(err) {
        FW.showToast('Network error \u2014 check device connection', 'error');
      });
    }
    ```
  - [ ] Add `saveAsNew()` function:
    ```javascript
    function saveAsNew() {
      var nameEl = document.getElementById('layout-name');
      var name = nameEl ? nameEl.value.trim() : '';
      if (!name) {
        FW.showToast('Enter a layout name before saving', 'error');
        return;
      }
      /* Force a new id by clearing currentLayoutId */
      currentLayoutId = null;
      saveLayout(false);
    }
    ```
  - [ ] Add `bindSaveButtons()` function (call from `init()`):
    ```javascript
    function bindSaveButtons() {
      var btnNew      = document.getElementById('btn-save-new');
      var btnSave     = document.getElementById('btn-save');
      var btnActivate = document.getElementById('btn-activate');
      if (btnNew)      btnNew.addEventListener('click',      saveAsNew);
      if (btnSave)     btnSave.addEventListener('click',     function() { saveLayout(false); });
      if (btnActivate) btnActivate.addEventListener('click', function() { saveLayout(true); });
    }
    ```

- [ ] **Task 7: Unsaved-changes guard** (AC: #8)
  - [ ] Add `beforeunload` handler (call from `init()`):
    ```javascript
    function bindUnloadGuard() {
      window.addEventListener('beforeunload', function(e) {
        if (!dirty) return;
        e.preventDefault();
        /* returnValue required for Chrome/Edge */
        e.returnValue = 'You have unsaved changes. Leave anyway?';
      });
    }
    ```
  - [ ] In `onPointerDown`, when the user clicks on empty canvas (miss / deselect), **do NOT** clear dirty — only interaction mutations set dirty. Deselect is non-destructive.
  - [ ] If a "Delete selected widget" button is added in future stories, it must call `window.confirm('Delete this widget? Unsaved changes will be lost.')` when dirty before removing.

- [ ] **Task 8: Smoke test `tests/smoke/test_editor_smoke.py`** (AC: #10)
  - [ ] Create `tests/smoke/test_editor_smoke.py` following the same structure as `test_web_portal_smoke.py`:
    - Accepts `--base-url`, `--timeout` CLI args
    - `TestEditorAsset`: GETs `/editor.html`, asserts HTTP 200 and `Content-Encoding: gzip`
    - `TestEditorLayoutCRUD`: POSTs a canned layout JSON body, asserts HTTP 200 and `ok: true`; extracts returned `id`
    - `TestEditorActivate`: POSTs `/api/layouts/<id>/activate`, asserts HTTP 200 and `ok: true`
    - `TestEditorModeSwitch`: GETs `/api/status` and asserts `data.display_mode.active == "custom_layout"` (see Dev Notes for exact field path)
    - Cleanup in tearDown: DELETE `/api/layouts/<id>` if id was created (non-destructive contract)
  - [ ] Canned test layout body (use this exact structure — must pass `LayoutStore::save()` validation):
    ```python
    CANNED_LAYOUT = {
        "id": "smoke-test-layout",
        "name": "Smoke Test",
        "version": 1,
        "canvas": {"width": 192, "height": 48},
        "widgets": [
            {
                "id": "w1",
                "type": "text",
                "x": 0, "y": 0, "w": 64, "h": 8,
                "color": "#FFFFFF",
                "content": "SMOKE",
                "align": "left"
            }
        ]
    }
    ```

- [ ] **Task 9: Gzip all modified assets** (AC: #12)
  - [ ] From `firmware/` directory, regenerate all changed `.gz` files:
    ```bash
    gzip -9 -c data-src/common.js   > data/common.js.gz
    gzip -9 -c data-src/editor.html > data/editor.html.gz
    gzip -9 -c data-src/editor.js   > data/editor.js.gz
    gzip -9 -c data-src/editor.css  > data/editor.css.gz
    ```
  - [ ] Verify totals: `wc -c data/editor.html.gz data/editor.js.gz data/editor.css.gz` — sum must be ≤ 30,720 bytes (30 KB)
  - [ ] If sum exceeds 30 KB: check for dead code in `editor.js`; the property panel additions should add ~4–6 KB source which compresses to ~2 KB — well within budget
  - [ ] If `common.js` was not modified (FW.put already existed), skip regenerating `common.js.gz`

- [ ] **Task 10: Build and ES5 verification** (AC: #11, #12)
  - [ ] ES5 compliance grep: `grep -nP "=>|\`|(?<![a-zA-Z_])let |(?<![a-zA-Z_])const " firmware/data-src/editor.js` — must return empty
  - [ ] Also run on `common.js` if modified: `grep -nP "=>|\`|(?<![a-zA-Z_])let |(?<![a-zA-Z_])const " firmware/data-src/common.js`
  - [ ] Run `~/.platformio/penv/bin/pio run -e esp32dev` from `firmware/` — must succeed with binary ≤ 1,382,400 bytes (88% of 1,572,864)
  - [ ] Run `~/.platformio/penv/bin/pio run -e esp32dev -t buildfs` — LittleFS image must build successfully
  - [ ] Confirm `wc -c firmware/data/editor.html.gz firmware/data/editor.js.gz firmware/data/editor.css.gz` total ≤ 30,720 bytes

---

## Dev Notes

### This Story Is Web-Only — Read This First

LE-1.7 is a **web-only story**. Zero firmware C++ changes are required. All API endpoints needed by the property panel and save UX are already implemented and battle-tested:

| API | Implemented in | Purpose |
|---|---|---|
| `GET /api/widgets/types` | LE-1.4 / WebPortal.cpp:2070+ | Field schemas that drive the property panel |
| `GET /api/layouts` | LE-1.4 / WebPortal.cpp:1830+ | List saved layouts (future "load" UX) |
| `POST /api/layouts` | LE-1.4 / WebPortal.cpp:1888+ | Create new layout |
| `PUT /api/layouts/:id` | LE-1.4 / WebPortal.cpp:1936+ | Overwrite existing layout |
| `POST /api/layouts/:id/activate` | LE-1.4 / WebPortal.cpp:2042+ | Switch LED wall to custom_layout |
| `DELETE /api/layouts/:id` | LE-1.4 / WebPortal.cpp:2015+ | Delete layout (smoke test cleanup) |

**No WebPortal.cpp changes needed for LE-1.7.**

---

### `FW.put` — Does It Already Exist?

Check `firmware/data-src/common.js` for `put:` in the return object. As of LE-1.6, `common.js` exports:
```javascript
return { get: get, post: post, del: del, showToast: showToast };
```
**`put` is absent.** Task 1 must add it. The implementation mirrors `FW.post` exactly, using `method: 'PUT'` instead of `'POST'`. The same `parseJsonResponse` / `fetchJson` helpers are reused — no new network code.

```javascript
function put(url, data) {
  return fetchJson(url, {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  });
}
```

After adding `put`, update the return statement:
```javascript
return { get: get, post: post, put: put, del: del, showToast: showToast };
```

**Do NOT change any existing function or the IIFE structure.** Only add the `put` function and update the return object.

---

### Layout JSON Schema — What `LayoutStore::save()` Validates

`LayoutStore::save()` enforces a V1 schema. Any body POSTed or PUT to `/api/layouts` must pass these checks or the firmware returns `400 LAYOUT_INVALID`:

**Required top-level fields:**
```json
{
  "id":      "<string — matches URL id; [A-Za-z0-9_-], ≤ 32 chars>",
  "name":    "<non-empty string>",
  "version": 1,
  "canvas":  { "width": <int>, "height": <int> },
  "widgets": [ ... ]
}
```

**Widget object fields (each entry in `widgets[]`):**
```json
{
  "id":      "<string — instance id, e.g. 'w1'>",
  "type":    "<one of: text, clock, logo, flight_field, metric>",
  "x": 0, "y": 0, "w": 32, "h": 8,
  "color":   "#FFFFFF",
  "content": "<string>",
  "align":   "<one of: left, center, right>"
}
```

**Limits (from LayoutStore.h):**
- Per-file max: **8,192 bytes** (`LAYOUT_STORE_MAX_BYTES`)
- Directory-wide max: **65,536 bytes** (`LAYOUT_STORE_TOTAL_BYTES`)
- Maximum layouts: **16** files (`LAYOUT_STORE_MAX_FILES`)
- Maximum id length: **32 chars** (`LAYOUT_STORE_MAX_ID_LEN`)
- Id charset: `[A-Za-z0-9_-]` only — `LayoutStore::isSafeLayoutId()` enforces this

**`getLayoutBody()` must produce a conformant body.** The id used in the body must match the id in the URL for PUT requests (`_handlePutLayout` verifies `docId == id` and returns `400 LAYOUT_INVALID` if they differ).

**Generating a safe id from the name** (when saving as new): use the layout name with non-safe characters replaced:
```javascript
function nameToId(name) {
  /* Replace any char not in [A-Za-z0-9_-] with '-'; collapse runs; trim */
  return name.replace(/[^A-Za-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '').substring(0, 32) || ('layout-' + Date.now());
}
```

---

### `GET /api/widgets/types` — Property Panel Field Schema

The response drives which controls appear in the property panel. Confirmed schema (from `WebPortal.cpp:_handleGetWidgetTypes`, implemented LE-1.4):

```json
{
  "ok": true,
  "data": [
    {
      "type": "text",
      "label": "Text",
      "fields": [
        { "key": "content", "kind": "string",  "default": "" },
        { "key": "align",   "kind": "select",  "default": "left", "options": ["left","center","right"] },
        { "key": "color",   "kind": "color",   "default": "#FFFFFF" },
        { "key": "x",       "kind": "int",     "default": 0 },
        { "key": "y",       "kind": "int",     "default": 0 },
        { "key": "w",       "kind": "int",     "default": 32 },
        { "key": "h",       "kind": "int",     "default": 8 }
      ]
    },
    {
      "type": "clock",
      "label": "Clock",
      "fields": [
        { "key": "content", "kind": "select",  "default": "24h", "options": ["12h","24h"] },
        { "key": "color",   "kind": "color",   "default": "#FFFFFF" },
        { "key": "x",       "kind": "int",     "default": 0 },
        { "key": "y",       "kind": "int",     "default": 0 },
        { "key": "w",       "kind": "int",     "default": 48 },
        { "key": "h",       "kind": "int",     "default": 8 }
      ]
    },
    {
      "type": "logo",
      "label": "Logo",
      "fields": [
        { "key": "content", "kind": "string",  "default": "" },
        { "key": "color",   "kind": "color",   "default": "#0000FF" },
        { "key": "x",       "kind": "int",     "default": 0 },
        { "key": "y",       "kind": "int",     "default": 0 },
        { "key": "w",       "kind": "int",     "default": 16 },
        { "key": "h",       "kind": "int",     "default": 16 }
      ]
    },
    {
      "type": "flight_field",
      "label": "Flight Field",
      "note": "LE-1.8 — not yet implemented, renders nothing",
      "fields": [ ... "w" default 48, "h" default 8 ]
    },
    {
      "type": "metric",
      "label": "Metric",
      "note": "LE-1.8 — not yet implemented, renders nothing",
      "fields": [ ... "w" default 48, "h" default 8 ]
    }
  ]
}
```

**Property panel mapping by `kind`:**
| `kind` | Control | Notes |
|---|---|---|
| `"string"` | `<input type="text">` | All types except clock |
| `"select"` | `<select>` with `options[]` | clock content (12h/24h), text align (but align uses the L/C/R segment control, not a `<select>`) |
| `"color"` | `<input type="color">` | Yields a hex string like `#ffffff` |
| `"int"` | Read-only `<span>` | x, y, w, h — these are drag-controlled on canvas |

**`align` is `kind: "select"` in the API** but the V1 panel renders it as the three-segment L/C/R button group (as spec'd in the epic). This is intentional — three large tap targets are phone-friendlier than a `<select>` for three values. The `align` field for non-text types is **hidden** (only the `text` widget type has meaningful alignment).

---

### POST vs PUT — Which to Call When

The API distinguishes create vs overwrite:

| Scenario | Call | Notes |
|---|---|---|
| New layout (never been saved) | `POST /api/layouts` | Body must contain the `id` field. Server validates `isSafeLayoutId`. If the id already exists on device, `LayoutStore::save()` **overwrites** it (it's an upsert at the storage layer, but the HTTP layer treats POST as "create new"). |
| Known layout id | `PUT /api/layouts/:id` | Body `id` field must equal the URL id or the server returns `400 LAYOUT_INVALID`. Server returns `404 LAYOUT_NOT_FOUND` if the id does not exist — cannot create via PUT. |

**`saveLayout()` strategy:**
- If `currentLayoutId` is `null` → POST (create)
- If `currentLayoutId` is a string → PUT (overwrite)
- `saveAsNew()` temporarily clears `currentLayoutId` to force POST even if the layout was previously saved

**POST response on success:**
```json
{ "ok": true, "data": { "id": "my-layout", "bytes": 312 } }
```
Store `res.body.data.id` → `currentLayoutId` after POST.

**PUT response on success:** Same shape — `data.id` echoes the URL id.

---

### Activate Flow — `POST /api/layouts/:id/activate`

`_handlePostLayoutActivate` (WebPortal.cpp:2042):
1. Validates id is safe
2. Verifies layout exists on LittleFS via `LayoutStore::load()`
3. Calls `LayoutStore::setActiveId(id)` — persists to NVS
4. Calls `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")` — **Rule 24**: always route mode switches through the orchestrator; never call `ModeRegistry::requestSwitch` directly
5. Returns `{ "ok": true, "data": { "active_id": "<id>" } }`

**The LED wall switches immediately** — `onManualSwitch` queues a mode change to Core 0 display task which picks it up on the next tick (typically < 100 ms). The 2-second budget in AC #6 is very conservative.

---

### `GET /api/status` — Exact Field Path for Smoke Test

`_handleGetStatus` returns a large JSON blob. The display mode path (for the smoke test assertion) is:
```json
{
  "ok": true,
  "data": {
    "display_mode": {
      "active": "custom_layout",
      ...
    },
    ...
  }
}
```

Verify with: `assert data['display_mode']['active'] == 'custom_layout'`

**Note:** There may be a short delay (< 500 ms) between the activate POST returning and the mode switch completing. The smoke test should poll `/api/status` up to 3 times with a 1-second delay before asserting. Alternatively, assert after a 2-second `time.sleep(2)`.

---

### ES5 Constraint — No Exceptions

All `firmware/data-src/*.js` files must be ES5 only. This applies to additions in `editor.js` and `common.js`. The table from LE-1.6 applies unchanged:

| FORBIDDEN | USE INSTEAD |
|---|---|
| `const x = ...` | `var x = ...` |
| `let x = ...` | `var x = ...` |
| `(x) => x + 1` | `function(x) { return x + 1; }` |
| `` `string ${var}` `` | `'string ' + var` |
| `class Foo { ... }` | `function Foo() { ... }` |
| `Array.from(...)` | `Array.prototype.slice.call(...)` |
| `Object.assign(...)` | Manual property copy loop |
| `.forEach(x => ...)` | `for (var i = 0; ...)` loop |

**Unicode escapes in strings** are ES5-safe and preferred over literal multi-byte characters in `.js` files: `'\u00d7'` for ×, `'\u2014'` for —, `'\u2026'` for ….

**Promises are allowed** — they are ES6 spec but available in all target browsers (iOS 9+, Android 4.1+ WebView). The existing `editor.js` already uses Promise chains.

---

### `dirty` Flag Lifecycle

```
Editor opens          → dirty = false
addWidget()           → dirty = true
onPointerUp (drag)    → dirty = true
props panel change    → dirty = true
saveLayout() OK       → dirty = false
saveAsNew() OK        → dirty = false (after POST succeeds)
page navigation       → beforeunload checks dirty
```

**Subtle rule:** Snapping, selecting a widget, or opening the property panel do **not** set dirty — only mutations to widget data (position, size, content, color, align) or widget list (add, future delete) do.

---

### Color Input and RGB565

The `<input type="color">` yields values like `"#ffffff"`, `"#ff0000"`. These are stored directly in `widget.color` and in the saved JSON. The firmware's `WidgetSpec::color` is `uint16_t` RGB565 — the **conversion from hex string to RGB565 is done by the firmware at layout-load time** (`CustomLayoutMode::init()`), not by the editor. The editor works entirely in hex strings.

When rendering on the canvas in the editor (`drawWidget()`), `ctx.fillStyle = w.color` works correctly with hex strings natively.

---

### Property Panel — Only 3 Editable Fields in V1

The property panel deliberately exposes only:
1. **Content** — what the widget shows (text string, clock format, logo id)
2. **Color** — widget fill color (hex string → RGB565 on device)
3. **Align** — text alignment (text widget only)

**x, y, w, h are read-only** in the panel — position and size are set by drag and resize on the canvas. Showing them as editable numeric inputs would introduce input-validation complexity and is deferred. They appear as readouts for orientation only.

**No kerning, no line-spacing, no font picker, no opacity slider** — the ESP32 hardware uses a fixed 5×7 Adafruit GFX font with 1px spacing. These parameters do not exist on the device, so the editor must not offer them.

---

### Layout Name → Safe Id Generation

`LayoutStore::isSafeLayoutId()` enforces `[A-Za-z0-9_-]` only. The editor name input allows any text (for human readability). Before POSTing, derive the id:

```javascript
function nameToId(name) {
  var id = name.replace(/[^A-Za-z0-9_-]+/g, '-')
               .replace(/^-+|-+$/g, '')
               .substring(0, 32);
  if (!id) id = 'layout-' + Date.now();
  return id;
}
```

The `id` field in the POST body is derived from the name this way. Store the result in `currentLayoutId` after a successful POST so subsequent edits can PUT to the correct URL.

**Example:** Name `"My Home Wall!"` → id `"My-Home-Wall"` (safe, ≤ 32 chars).

---

### `getLayoutBody()` — Exact Structure Required

```javascript
function getLayoutBody() {
  var nameEl = document.getElementById('layout-name');
  var name = (nameEl && nameEl.value.trim()) ? nameEl.value.trim() : 'My Layout';
  /* Derive or reuse id */
  var id = currentLayoutId ? currentLayoutId : nameToId(name);
  return {
    id: id,
    name: name,
    version: 1,
    canvas: { width: deviceW, height: deviceH },
    widgets: widgets
  };
}
```

- `version: 1` is required by `LayoutStore::save()` schema validation
- `canvas.width` / `canvas.height` must match `deviceW` / `deviceH` (set by `initCanvas()` from `GET /api/layout`)
- `widgets` is the in-memory array — each entry already has all required fields (`id`, `type`, `x`, `y`, `w`, `h`, `color`, `content`, `align`) from `addWidget()`

---

### `saveLayout()` — Promise Chain Correctness

The save → activate chain uses `.then()` returning `null` to short-circuit the activate step when `andActivate` is false:

```javascript
savePromise.then(function(res) {
  if (!res.body.ok) { /* toast error */ return null; }
  /* ... store id, clear dirty ... */
  if (!andActivate) return null;   /* <-- chain stops here for "Save" */
  return FW.post('/api/layouts/' + currentLayoutId + '/activate', {});
}).then(function(res) {
  if (res === null || res === undefined) return;  /* <-- catches the null */
  /* ... activate success handling ... */
}).catch(function(err) {
  FW.showToast('Network error \u2014 check device connection', 'error');
});
```

The `.catch()` at the end handles fetch-level failures (network timeout, device offline). It does **not** need to distinguish save vs activate errors — the inner `.then()` handlers already showed specific error toasts for API-level errors.

---

### `PUT /api/layouts/:id` — Body Id Must Match URL Id

`_handlePutLayout` (WebPortal.cpp:1936) explicitly validates:
```cpp
if (strcmp(docId, id.c_str()) != 0) {
    _sendJsonError(request, 400, "Body id does not match URL id", "LAYOUT_INVALID");
    return;
}
```

`getLayoutBody()` must produce `body.id === currentLayoutId` when doing a PUT. If these differ (e.g. user changed the name input after saving), the PUT will be rejected. **Do not** re-derive the id from the name on PUT — reuse `currentLayoutId` as `body.id`.

---

### `beforeunload` — Browser Compatibility

The `beforeunload` handler follows the modern pattern:
```javascript
window.addEventListener('beforeunload', function(e) {
  if (!dirty) return;
  e.preventDefault();
  e.returnValue = 'You have unsaved changes. Leave anyway?';
});
```

- `e.preventDefault()` is required by Chrome ≥ 119 (spec change)
- `e.returnValue` (the custom message) is **ignored by all modern browsers** — they show their own generic message; the assignment is for legacy browsers only
- Setting `e.returnValue = ''` (empty string) in older Chromium also triggered the dialog; the exact value doesn't matter on modern browsers

---

### Gzip — Use `-9 -c`, Never `-k`

```bash
# From firmware/ directory:
gzip -9 -c data-src/editor.html > data/editor.html.gz
gzip -9 -c data-src/editor.js   > data/editor.js.gz
gzip -9 -c data-src/editor.css  > data/editor.css.gz
gzip -9 -c data-src/common.js   > data/common.js.gz   # only if common.js modified
```

- `-9`: maximum compression
- `-c`: write to stdout (keeps source file unchanged)
- `>`: redirect into `data/`

**Do NOT use** `gzip -k` (writes `.gz` alongside source in `data-src/`).

**Both source and `.gz` must be committed.** The `.gz` files in `firmware/data/` are the deployable artifacts; `data-src/` files are the source of truth. Both are version-tracked.

---

### LittleFS Budget

Current state after LE-1.6 (measured 2026-04-17):
- LittleFS partition: 960 KB = 983,040 bytes
- Editor assets (LE-1.6): 5,063 bytes total (editor.html.gz: 569, editor.js.gz: 3,582, editor.css.gz: 912)
- Remaining non-editor web assets: ~62,564 bytes
- Logo files: ~404 KB

LE-1.7 additions (estimated):
- Property panel HTML additions: ~800 bytes source → ~300 bytes gzipped (incremental)
- Property panel JS additions: ~3–4 KB source → ~1.5 KB gzipped (incremental)
- Property panel CSS additions: ~1 KB source → ~400 bytes gzipped (incremental)
- `common.js` addition (FW.put ~5 lines): < 50 bytes gzipped (incremental)

**Estimated LE-1.7 total editor assets: ~7 KB gzipped** — well within the 30 KB budget.

---

### Smoke Test — `test_editor_smoke.py` Pattern

Follow the same class-per-scenario structure as `test_web_portal_smoke.py`:
```python
class TestEditorAsset(unittest.TestCase):
    def test_editor_html_served_gzip(self):
        r = client.get('/editor.html')
        self.assertEqual(r.status, 200)
        self.assertEqual(r.encoding, 'gzip')
        self.assertIn('text/html', r.content_type)
```

The smoke test should be **non-destructive by default** — the cleanup DELETE in tearDown ensures any test-created layout is removed. Use a clearly namespaced test id like `"smoke-test-layout"` to avoid colliding with real user layouts.

**`GET /api/status` field path for mode check:**
```python
body = json.loads(r.decoded_body)
self.assertTrue(body['ok'])
self.assertEqual(body['data']['display_mode']['active'], 'custom_layout')
```

---

### Antipattern Prevention Table

| DO NOT | DO INSTEAD | Reason |
|---|---|---|
| PUT to a non-existent id | POST to create, then PUT to update | `_handlePutLayout` returns 404 on miss |
| Body `id` ≠ URL `id` in PUT | Keep `body.id === currentLayoutId` | Firmware returns `400 LAYOUT_INVALID` |
| Re-derive id from name on PUT | Reuse `currentLayoutId` as `body.id` | Name edits after first save must not change the stored id |
| Omit `version: 1` from body | Always include `version: 1` | `LayoutStore::save()` schema validation rejects missing version |
| Use `let`/`const`/`=>` in `editor.js` or `common.js` | Use `var`/`function` | ES5 constraint — grep gate AC #11 |
| Use `Object.assign` to merge widget state | Manual `w.color = val` field-by-field | `Object.assign` is ES6 |
| Call `ModeRegistry::requestSwitch` directly | Call `POST /api/layouts/:id/activate` | Rule 24: all mode switches route through ModeOrchestrator |
| Skip `.catch()` on save Promise chain | Always add `.catch()` | Network errors (device offline) must show a toast, not silently fail |
| Forget to regenerate `.gz` after changing source | Always run `gzip -9 -c ...` after every source edit | Device serves only `.gz` files; stale `.gz` = stale UI on device |
| Set `dirty = true` on deselect | Only set on data mutations | Deselect is non-destructive; spurious dirty guard is annoying |
| Allow empty layout name | Validate non-empty before POST/PUT | Firmware rejects `name: ""` as `LAYOUT_INVALID` |

---

### Sprint Status Update

After all tasks pass, update `_bmad-output/implementation-artifacts/sprint-status.yaml`:
```yaml
le-1-7-editor-property-panel-and-save-ux: done
```

---

## File List

Files **modified**:
- `firmware/data-src/editor.html` — add property panel form elements + layout name input + save buttons
- `firmware/data-src/editor.js` — add property panel show/hide, field wiring, save/activate functions, dirty flag, beforeunload guard
- `firmware/data-src/editor.css` — add styles for props-form, align-control, layout-name-input, save-control
- `firmware/data-src/common.js` — **only if `FW.put` is absent** — add `put()` function and update return object
- `firmware/data/editor.html.gz` — regenerated after source edit
- `firmware/data/editor.js.gz` — regenerated after source edit
- `firmware/data/editor.css.gz` — regenerated after source edit
- `firmware/data/common.js.gz` — **only if common.js modified** — regenerated

Files **created**:
- `tests/smoke/test_editor_smoke.py` — smoke tests for editor asset serving, layout CRUD, activate, mode switch

Files **NOT modified** (verify unchanged before committing):
- `firmware/adapters/WebPortal.cpp` — no firmware changes needed
- `firmware/core/LayoutStore.*` — no changes
- `firmware/core/WidgetRegistry.h` — no changes
- `firmware/modes/CustomLayoutMode.cpp` — no changes
- `firmware/data-src/dashboard.html`, `dashboard.js`, `wizard.html`, `wizard.js`, `health.html`, `health.js`, `style.css` — unchanged

---

## Change Log

| Date | Version | Description | Author |
|---|---|---|---|
| 2026-04-17 | 0.1 | Draft story created (9 AC, 8 tasks, minimal dev notes) | BMAD |
| 2026-04-17 | 1.0 | Upgraded to ready-for-dev. Key additions: (1) Verified `FW.put` absent from `common.js` — Task 1 added as explicit implementation step with exact code; (2) Full LayoutStore JSON schema documented (required fields, id constraints, byte caps) with `nameToId()` helper; (3) POST vs PUT decision matrix documented with exact `_handlePutLayout` body-id validation requirement; (4) `GET /api/widgets/types` full response shape documented for all 5 widget types with exact field `kind` → control mapping; (5) Property panel fields limited to content/color/align with explicit rationale for read-only x/y/w/h; (6) `saveLayout()` Promise chain with null short-circuit for andActivate=false documented; (7) `dirty` flag lifecycle table; (8) `beforeunload` browser compatibility notes; (9) Activate flow mechanics documented (Rule 24, ModeOrchestrator, sub-500ms switch); (10) `GET /api/status` exact field path for smoke test; (11) Complete antipattern prevention table (14 entries); (12) LittleFS budget analysis for LE-1.7 additions; (13) ES5 constraint table carried forward from LE-1.6; (14) Gzip command reminder; (15) Smoke test pattern documented. AC expanded from 9 to 12; Tasks expanded from 8 to 10. Status changed to ready-for-dev. | BMAD Story Synthesis |



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

## Story le-1-6 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `loadLayout()` accesses `d.matrix.width`, `d.matrix.height`, and `d.hardware.tile_pixels` without first verifying the nested structure exists and is correctly typed. A malformed-but-`ok: true` API response (e.g., `{"ok":true,"data":{}}`) causes a `TypeError` crash before the canvas is ever initialized. | Added nested-field guard before `initCanvas()` call — checks `d.matrix`, `d.hardware`, and type of all three numeric fields. Displays a specific "Invalid layout data — check device firmware" toast before returning `null`. |
| low | `releasePointerCapture` was called *after* `dragState = null`, which is semantically backwards — spec-correct order is to release the capture first, then clear state. Functionally correct due to the early-return guard at line 206, but ordering violates the spec contract. | Moved `releasePointerCapture` call to before `dragState = null`. |
| low | `initToolbox(res.body.data)` was called without verifying `data` is array-like. If the API returns `{"ok":true,"data":null}`, `types.length` throws a `TypeError` inside `initToolbox`, silently breaking toolbox initialization with no error feedback. | Extracted `types` variable, added `typeof types.length !== 'number'` guard before `initToolbox()`. Silently returns (API already confirmed `ok:true`, toolbox remains empty until device reboots). |
| low | `addWidget()` clamped widget *position* (x,y) into canvas bounds but not widget *dimensions* (w,h). If a future widget type has `defaultW > deviceW`, the widget is created overflowing the canvas with no visible feedback, giving a dishonest canvas preview. | Added two dimension-clamp lines before the position-clamp lines in `addWidget()`. Uses `meta.minW`/`meta.minH` floors. Only triggers when `deviceW > 0` (i.e., after canvas is initialized). |
| dismissed | Task 10 checkbox `[x]` marked complete but not performed | FALSE POSITIVE: Task 10 is `[ ]` (unchecked) in the actual story file at line 198. This was corrected in Round 1 synthesis. False positive — the reviewer was working from stale context. |
| dismissed | Story `Status: done` while AC#10 is unverified — story should be `in-progress` | FALSE POSITIVE: The existing `[AI-Review] LOW` action item in the Senior Developer Review section already captures this. The story conventions in this project allow `Status: done` with a pending hardware smoke test flagged as an AI-Review action item. The Dev Agent Record is explicit about the deferral. No change to story status warranted. |
| dismissed | AC#3 violation — toolbox items are clickable, not draggable; HTML5 drag-drop not implemented | FALSE POSITIVE: Task 5 of the story — the authoritative implementation specification — explicitly says "**Clicking** a non-disabled toolbox item calls `addWidget(type)`." The story Dev Notes show the `addWidget()` function body that `initToolbox` is required to call. The AC#3 "draggable" language is aspirational UX phrasing; Task 5 resolved the implementation as click-to-add. Implementation exactly matches Task 5. The AC wording is a documentation inconsistency in the story (not a code defect), already noted in prior synthesis. |
| dismissed | Hit-test boundary uses inclusive `<=` instead of exclusive `<` on widget right/bottom edges | FALSE POSITIVE: The story Dev Notes *explicitly* use `<=` in the documented hit-test code block: `"if cx >= w.x * SCALE && cx <= (w.x + w.w) * SCALE"`. The implementation faithfully follows the authoritative story spec. The practical impact is 1 CSS pixel = 0.25 logical pixels at 4× scale — imperceptible in use. The Dev Notes are authoritative; the `<=` is intentional design. False positive. |
| dismissed | `selectedIndex` mutation before widget reference capture creates race condition in `onPointerDown` | FALSE POSITIVE: JavaScript's event loop is single-threaded. No event can fire between two consecutive synchronous lines (`selectedIndex = hit.index` then `var w = widgets[selectedIndex]`). `setPointerCapture` prevents subsequent pointer events, but even without it, `onToolboxClick` cannot interleave with synchronous code execution. This is a fundamental misunderstanding of JS concurrency. False positive. |
| dismissed | `initToolbox` accumulates duplicate `click` listeners on repeated calls | FALSE POSITIVE: `initToolbox` is called exactly once — from the `loadLayout().then()` chain, called once from `init()`, called once from `DOMContentLoaded`. There is no code path that re-invokes `loadLayout()` or `initToolbox()` after page load. False positive for current code; only a theoretical future risk. |
| dismissed | Weak numeric parsing in `buildWidgetTypeMeta` — `f['default']` could be a string causing `NaN` propagation | FALSE POSITIVE: `FW.get()` uses `JSON.parse` under the hood. The ArduinoJson-serialized API response emits `"default": 32` (number literal), which JSON.parse correctly deserializes as a JS number. The API contract (from `WebPortal.cpp`) sends integers, not strings, for `w`/`h` fields. String coercion would require the server to malform the JSON. False positive for this API contract. |
| dismissed | Canvas resize during drag causes coordinate mismatch — `initCanvas()` could be called while `dragState` is active | FALSE POSITIVE: `initCanvas()` is only called from `loadLayout()`, which is only called once from `init()` at page load. There is no polling, no "refresh layout" button, and no WebSocket that could trigger a second `loadLayout()` call. False positive for the current architecture; theoretical future concern. |
| dismissed | Story File List omits `tests/smoke/test_web_portal_smoke.py` and `sprint-status.yaml` from modified-files section | FALSE POSITIVE: Documentation completeness note, not a code defect. The File List documents product source files; the smoke test and sprint YAML are tracked separately in the Dev Agent Record. Pre-existing issue already logged in prior synthesis; no action required. |
| dismissed | AC#7 story text claims logo minimum height is 8px but API returns h=16 for logo type | FALSE POSITIVE: This AC text documentation error was already identified and dismissed in the prior synthesis round (visible in the antipatterns table: "AC7 logo minimum height: story says h=8, API returns h=16 — implementation is wrong → FALSE POSITIVE: code is correct, AC7 story text is wrong"). `buildWidgetTypeMeta()` correctly reads `minH = meta.defaultH` from the API response. No code change needed. |
| dismissed | `FW.*` usage without hard guard — hard crash if `common.js` fails to load | FALSE POSITIVE: `common.js` is loaded synchronously before `editor.js` via `<script src="/common.js"></script>`. If `common.js` fails to load, the browser would show a network error before `editor.js` runs. The `FW.showToast` guard in `onPointerMove` (`typeof FW !== 'undefined' && FW.showToast`) already demonstrates defensive coding where it matters. Adding `FW` null checks everywhere is disproportionate to the risk on a LAN-only device page where `common.js` is served from the same device. |
| dismissed | Editor toolbox not keyboard-accessible (WCAG gap) | FALSE POSITIVE: Confirmed gap but out of LE-1.6 scope. The project context notes this is a LAN-only appliance UI. The story has no accessibility ACs. Deferred. |
| dismissed | `e.preventDefault()` only fires on canvas hit — miss path allows browser gestures | FALSE POSITIVE: `touch-action: none` on `#editor-canvas` (CSS line 87) unconditionally prevents browser scroll/zoom on the canvas, regardless of hit-test outcome. `e.preventDefault()` is documented as "belt-and-braces" belt-and-braces for older iOS. The antipatterns table explicitly forbids `e.preventDefault()` globally on body. Already addressed in prior synthesis. |
| dismissed | Toast suppression `toastedFloor` flag is fragile / no issue with toast logic | FALSE POSITIVE: Reviewer C explicitly concluded "No Issue Found" on this item. Confirmed: logic is correct. False positive. --- |

## Story le-1-6 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | Missing `.catch()` on `loadLayout()` promise chain — a `fetch()` network transport failure (connection refused, DNS fail) produces an unhandled promise rejection with no user-visible feedback | Added `.catch(function(err) { FW.showToast('Editor failed to load — check device connection', 'error'); })` at the end of the `.then().then()` chain in `loadLayout()`. This follows the project-wide pattern established across `dashboard.js` and `wizard.js`. Also regenerated `firmware/data/editor.js.gz` (3,958 bytes; total gzip 5,439 bytes = 26.6% of 20 KB budget, still well within AC8). |
| low | Issue (DEFERRED — action item from prior rounds)**: AC10 mobile smoke test not yet performed on real hardware | Ongoing `[AI-Review]` action item tracking this; cannot be resolved without physical device. |
| dismissed | hitTest `<=` boundary allows clicks 1–2 CSS pixels outside widget to register as hits | FALSE POSITIVE: The story Dev Notes *explicitly* specify `<=` in the documented hit-test code block: `"if cx >= w.x * SCALE && cx <= (w.x + w.w) * SCALE"`. This was already dismissed in Round 2 synthesis. The `<=` is by-design per the authoritative story spec. The practical impact is 1 CSS pixel = 0.25 logical pixels at 4× scale — imperceptible in use. |
| dismissed | Snap control race condition — SNAP variable and DOM active state can diverge under rapid clicks | FALSE POSITIVE: JavaScript's event loop is strictly single-threaded. The `SNAP = snapVal` → `localStorage.setItem()` → `updateSnapButtons()` → `render()` sequence executes atomically within a single event callstack. No other event can interleave. The proposed `snapUpdateInProgress` flag is a no-op lock that solves a problem that cannot exist in the JS execution model. False positive. |
| dismissed | Negative dimension defense missing in `addWidget()` — `deviceW - w.w` can be negative | FALSE POSITIVE: Lines 286–290 already handle this with `Math.max(meta.minW \|\| 6, deviceW)` and `Math.max(0, deviceW - w.w)`. These guards were added in Round 2 synthesis. Reading the current source confirms they are present. False positive. |
| dismissed | API response schema guard missing in `loadLayout()` — accessing `d.matrix.width` without checking nested structure | FALSE POSITIVE: Lines 346–351 of the current `editor.js` already contain the exact nested-field guard checking `d.matrix`, `d.hardware`, and the numeric types of all three fields. This was fixed in Round 2 synthesis. Reading the current source confirms it is present. False positive. |
| dismissed | `initToolbox` called without array-like guard — `types.length` would throw if `data` is `null` | FALSE POSITIVE: Line 358 of the current `editor.js` contains `if (!types \|\| typeof types.length !== 'number') return;` before the `initToolbox()` call. This was fixed in Round 2 synthesis. False positive. |
| dismissed | `releasePointerCapture` called after `dragState = null` — wrong spec order | FALSE POSITIVE: Lines 210–212 of the current `editor.js` show `releasePointerCapture` is called *before* `dragState = null`. The ordering fix was applied in Round 2 synthesis. False positive. |
| dismissed | Task 10 checkbox is `[x]` (complete) but mobile smoke test was never performed | FALSE POSITIVE: Task 10 is `[ ]` (unchecked) in the actual story file at line 198. This was corrected in Round 1 synthesis. The reviewer was working from stale context. False positive. |
| dismissed | AC3 drag-and-drop wording vs click-to-add implementation — AC traceability invalid | FALSE POSITIVE: This was dismissed in prior synthesis rounds. Task 5 of the story (the authoritative implementation spec) explicitly says "**Clicking** a non-disabled toolbox item calls `addWidget(type)`." The AC3 "draggable" language is aspirational UX phrasing; Task 5 resolved the implementation as click-to-add. The AC wording is a documentation inconsistency in the story text, not a code defect. |
| dismissed | Story `Status: done` while AC10 is unverified — Definition-of-Done violation | FALSE POSITIVE: Not a code defect. The existing `[AI-Review] LOW` action item in the Senior Developer Review section already captures this per project convention. This is a governance observation, not a source code issue. |
| dismissed | API response `data` type validation — `entry.type`/`entry.label` could be undefined, rendering "undefined" in toolbox | FALSE POSITIVE: `FW.get()` uses `JSON.parse` under the hood. The ArduinoJson-serialized API response emits correctly typed string values for `type` and `label`. The fallback `entry.label \|\| entry.type` in `initToolbox()` provides an additional safety net. The scenario requires the server to malform its own JSON, which is not an application-layer bug. The `console.warn` Reviewer B suggests would be ES5-valid but adds complexity for a hypothetical device firmware bug scenario. Dismissed — the API contract is well-defined and tested by `test_get_widget_types_contract`. |
| dismissed | initToolbox listener accumulates on repeated calls | FALSE POSITIVE: `initToolbox()` is called exactly once per page lifecycle. Already dismissed in prior synthesis rounds. False positive. |
| dismissed | `e.preventDefault()` only fires on hit — miss path allows browser gestures | FALSE POSITIVE: `touch-action: none` on `#editor-canvas` (CSS line 87) unconditionally prevents browser scroll/zoom on the canvas regardless of hit-test outcome. Already dismissed in prior synthesis rounds. False positive. |
| dismissed | Resize handle overlap with widget body — selected widget's handle intercepts click intended for unselected overlapping widget | FALSE POSITIVE: LOW, acknowledged design limitation. Widgets in a real FlightWall layout should not overlap (the editor is for a 192×48 LED matrix — overlapping widgets would produce meaningless display output). This edge case has negligible real-world impact. Accepted as documented behavior. |
| dismissed | Pointer cancel edge case — brief window between `pointercancel` and `onPointerMove` guard | FALSE POSITIVE: Reviewer B self-assessed this as LOW and ultimately concluded "NO FIX REQUIRED — existing guard is sufficient." The `if (dragState === null) return` guard at line 171 makes this safe. The event loop guarantees sequential execution. False positive. |
| dismissed | Story File List omits `tests/smoke/test_web_portal_smoke.py` and `sprint-status.yaml` | FALSE POSITIVE: Documentation completeness note, not a code defect. Already dismissed in prior synthesis rounds. |
| dismissed | AC7 logo minimum height: story text claims h=8, API returns h=16 | FALSE POSITIVE: The code is correct — `buildWidgetTypeMeta()` reads `minH = meta.defaultH` from the API response which returns h=16 for logo. The AC7 story text is wrong. Already dismissed in prior synthesis rounds. No code change needed. |
| dismissed | SOLID violation — `editor.js` IIFE mixes concerns (rendering, state, events, API, DOM) | FALSE POSITIVE: The project explicitly uses an IIFE module pattern for all `data-src/*.js` files (ES5 constraint, no modules). Extracting sub-modules would require either multiple `<script>` tags (increasing LittleFS usage) or a build system (not in scope for this embedded project). Out of scope for LE-1.6. |
| dismissed | Non-array `data` from widget types response returns silently with no toast | FALSE POSITIVE: Correct behavior. If `ok:true` is returned but `data` is null/non-array, this is a firmware bug that should not surface to the user as an error toast (the device is functioning, just with a malformed response shape). The toolbox remains empty (canvas is still functional). Silent return is appropriate here; the `ok:true` check already confirmed server health. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/le-1-7-editor-property-panel-and-save-ux.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 4 |
| Task Completion Lies | 1 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task marked complete but implementation is missing (Task 1, 6, 7). | Task Audit | +3 |
| 🟠 IMPORTANT | AC#4/5: Save/Save-As-New API flow unimplemented. | AC Validation | +1 |
| 🟠 IMPORTANT | AC#8: Beforeunload guard missing from `editor.js`. | AC Validation | +1 |
| 🟠 IMPORTANT | AC#10: Smoke test `test_editor_smoke.py` is entirely absent. | AC Validation | +1 |
| 🟡 MINOR | Task audit confirms partial implementation only. | Task Audit | +0.3 |
| 🟡 MINOR | Missing Error Handling on `FW.put` chain. | Code Quality | +0.3 |
| 🟡 MINOR | Potential Z-index/Ordering issue with panel UI. | Hidden Bugs | +0.3 |

### Evidence Score: 6.9

| Score | Verdict |
|-------|---------|
| **6.9** | **MAJOR REWORK** |

---

## 🐛 Correctness & Safety

- **🐛 AC Validation Failure:**
  - **Task Completion Lies:** Tasks 1, 6, and 7 are marked `[x]` in the story file, but `FW.put` is not added to `common.js`, Save/Save-As-New UI is missing, and the `beforeunload` guard does not exist in `editor.js`. This is a CRITICAL finding.
  - **AC #4/5/6 Missing:** The UI controls (Save/Save-As-New/Activate) and their associated API logic (`FW.put`, `FW.post` logic) are completely absent.
  - **AC #8 Missing:** No `window.addEventListener('beforeunload', ...)` guard implemented.
  - **AC #10 Missing:** The smoke test file `tests/smoke/test_editor_smoke.py` does not exist in the repo.

- **🐛 Hidden Bug:** `onPointerUp` does not include `if (dragState) dirty = true;`. The story explicitly called for this in Task 5 to ensure mutation completion triggers the dirty flag. Currently, dragging a widget moves it but leaves `dirty = false`, bypassing the unsaved changes guard.

- **🐛 Hidden Bug:** The Property Panel HTML insertion logic is missing from `editor.js`. The `showPropsPanel` function and binding logic were never added, leaving the editor in a broken state where the props panel remains empty regardless of widget selection.

---

## 🛠️ Suggested Fixes

### 1. Missing Implementation - AC #4, #5, #6, #8, #10
**File:** `firmware/data-src/common.js`, `firmware/data-src/editor.js`, `tests/smoke/test_editor_smoke.py`
**Issue:** Major functional requirements and task list marked complete but missing.

**Corrected code:** Implement `FW.put` in `common.js`, add `bindSaveButtons`, `saveLayout`, `beforeunload` guard, and `showPropsPanel` in `editor.js`, and create the required smoke test suite.

---

**Issues Fixed:** 0
**Action Items Created:** 0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial code review complete, Christian.**

**Story:** `_bmad-output/implementation-artifacts/stories/le-1-7-editor-property-panel-and-save-ux.md`

---

## Review summary

| Category | Issues found |
|----------|----------------|
| Git vs story discrepancies | 2 |
| AC implementation gaps | 10 |
| Task completion lies (`[x]` but not done) | 0 |
| SOLID / boundary violations | 2 |
| Hidden bugs / spec traps (in story text + current code) | 3 |
| Performance footguns | 1 |
| Security vulnerabilities | 0 |
| **Total issues** | **10** |

---

## Evidence score summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| CRITICAL | LE-1.7 acceptance criteria are largely unimplemented in the product files in scope (`editor.*`, `common.js`, dedicated smoke test). | `firmware/data-src/editor.html`, `firmware/data-src/editor.js`, `firmware/data-src/editor.css`, `firmware/data-src/common.js` | +3 |
| IMPORTANT | `FW.put` is absent — AC9 / Task 1 not satisfied. | `firmware/data-src/common.js` (return object ends at `del`, `showToast`) | +1 |
| IMPORTANT | No `tests/smoke/test_editor_smoke.py` — AC10 / Task 8 not satisfied (not evidenced in repo snapshot; only `test_web_portal_smoke.py` appears touched). | Missing file / story gap | +1 |
| IMPORTANT | Editor still has LE-1.6 placeholder props markup only — AC1–AC2 not met. | `firmware/data-src/editor.html` | +1 |
| IMPORTANT | No `dirty`, `beforeunload`, save/activate flows in editor — AC3–AC8 not met. | `firmware/data-src/editor.js` | +1 |
| MINOR | Story’s own mobile CSS snippet hides `.editor-props` under 720px while the story persona is “layout author on my phone”. | Story Task 3 CSS vs AC1 narrative | +0.3 |
| MINOR | Story pseudocode uses `FW.showToast('Applying…', 'success')` for an in-flight operation — misleading severity vs existing toast semantics. | Story Task 6 | +0.3 |
| MINOR | Early `getLayoutBody()` sketch in tasks uses `Date.now()` in ids; dev notes require `nameToId()` / safe charset — easy footgun if implemented verbatim. | Story Task 6 vs Dev Notes | +0.3 |
| MINOR | `WebPortal::_handleGetWidgetTypes` still hard-codes schemas with an explicit “sync bomb” comment deferred to LE-1.7 — remains unaddressed while editor depends on it. | `WebPortal.cpp` (widget types handler + SYNC-RISK NOTE) | +0.3 |
| CLEAN PASS | No committed `test_editor_smoke.py` to evaluate — no “lying test” artifact to dissect. | — | −0.5 |
| CLEAN PASS | No new auth/session surface in this story scope — no new credential paths to attack. | — | −0.5 |

### Evidence score: **7.2**

| Score | Verdict |
|-------|---------|
| **7.2** | **REJECT** (≥ 7.0 per rubric) |

---

## Architectural / boundary issues

- **[7/10] Single Responsibility / DRY — widget schema triple-source:** `WebPortal::_handleGetWidgetTypes` documents the same type strings living in `LayoutStore`, `WidgetRegistry`, and the HTTP handler — LE-1.7 was the stated consolidation milestone; it is still three sources of truth. **Fix:** one generated or shared descriptor table consumed by all three (or codegen), not another hard-coded HTML panel.
- **[6/10] Abstraction mismatch — AC1 vs Task 2 HTML:** AC1 requires the panel be “driven by the `fields` array” from `GET /api/widgets/types`, while Task 2 hard-wires specific inputs (`prop-content`, clock select, etc.). That is not a data-driven renderer; it encodes the schema twice. **Fix:** generate controls from `fields[]` (kind → control), with special-case only where UX demands (align L/C/R).

---

## Python / smoke-test quality

- **Missing smoke module:** AC10 requires `tests/smoke/test_editor_smoke.py` with gzip asset checks, POST layout, activate, status poll, DELETE cleanup. No implementation was evidenced in the provided tree/status for that file.
- **Style / structure (pre-implementation):** When added, mirror `test_web_portal_smoke.py` patterns (`--base-url`, timeouts, clear failure messages); avoid duplicating HTTP helpers inconsistently.

---

## Performance and scalability

- **LOW — future `JSON.stringify(widgets)` payload:** Once save is implemented, posting the full in-memory `widgets` array on every save can approach `LAYOUT_STORE_MAX_BYTES` (8192) quickly if users iterate; no client-side pre-serialization size check is specified. **Fix:** preflight `JSON.stringify(body).length` and block with toast before hitting the device with 413/parse pain.

---

## Correctness and safety

- **Bug / UX contradiction (story):** Task 3’s `@media (max-width: 720px) { .editor-props { display: none; } }` directly fights the story’s “phone layout author” framing and AC1 (panel on selection). **Fix:** keep a collapsible drawer or bottom sheet; do not `display:none` the only editor for props on phones.
- **Spec hazard (story pseudocode):** `saveLayout` shows a “success” toast for “Applying…” during a chained activate — users will read that as completion. Use `'info'`/`'warning'` or a non-toast inline status consistent with `style.css` conventions.
- **ID safety hazard:** Any implementation that copies the early `getLayoutBody()` id fallback (`'layout-' + Date.now()`) risks non-safe IDs vs `LayoutStore::isSafeLayoutId()` — follow the story’s later `nameToId()` dev note, not the first task sketch.

---

## Maintainability / tech debt

- **Deferred LE-1.7 note still open in firmware:** The SYNC-RISK NOTE in `WebPortal.cpp` explicitly calls out editor UI surfacing risk if lists drift — still accurate; LE-1.7 hasn’t resolved it.
- **No `FW.put`:** `common.js` only exports `get`, `post`, `del`, `showToast` — blocks PUT save path entirely.

---

## Suggested fixes (priority order)

1. **Implement `put()` in `common.js`** and regenerate `firmware/data/common.js.gz`; extend return object with `put: put` only.
2. **Replace editor props placeholder** with the form structure from the story, plus layout name + save controls in `editor.html`; wire in `editor.js` with `dirty`, `showPropsPanel`, prop listeners, drag-end `dirty`, save/activate chains, `beforeunload`, and `confirm` guards for destructive navigation (when load/delete exists).
3. **Add `tests/smoke/test_editor_smoke.py`** per AC10; wire into the same runner conventions as `test_web_portal_smoke.py`.
4. **Reconcile mobile UX** — do not hide the entire props panel on narrow viewports without a replacement surface.
5. **Address schema duplication** — either truly drive the panel from `fields[]` or rewrite AC1 to match the intentional static mapping (pick one; do not pretend both).

---

**Issues fixed:** 0 (read-only review)  
**Action items created:** 0 (orchestrator may file from this report)

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

# 🔥 ADVERSARIAL CODE REVIEW: Story le-1.7
## Editor Property Panel and Save UX

**Reviewer:** Senior Developer (Adversarial Mode)  
**Date:** 2026-04-17  
**Story:** le-1-7-editor-property-panel-and-save-ux  
**Status:** ⚠️ **MAJOR ISSUES FOUND** - 8 Specific Problems Identified

---

## 📊 Review Summary

| Category | Count | Status |
|----------|-------|--------|
| **AC Implementation Gaps** | 3 | 🔴 CRITICAL |
| **Task Completion Lies** | 2 | 🔴 CRITICAL |
| **Missing FW.put() Implementation** | 1 | 🔴 CRITICAL |
| **ES5 Compliance Violations** | 2 | 🟠 HIGH |
| **Smoke Test Gaps** | 1 | 🟠 HIGH |
| **Promise Chain Error Handling** | 1 | 🟡 MEDIUM |
| **Performance/UX Issues** | 1 | 🟡 MEDIUM |
| **Total Issues** | **11** | |

---

## 🔴 CRITICAL FINDINGS

### 1. **FW.put() Function Missing - AC #9 Unimplemented**
**Severity:** CRITICAL  
**Files:** `firmware/data-src/common.js`  
**Issue:** Story AC #9 explicitly requires: *"Given `common.js` is inspected **When** `FW.put` is looked up **THEN** it is found to be a function..."* The provided `common.js` (lines 1-47) contains ONLY `get`, `post`, `del`, and `showToast`. **`FW.put` is completely absent.**

**Evidence:**
- Read `firmware/data-src/common.js` → line 1 to 47 → NO `put` function definition
- Read `firmware/data-src/common.js` → line 45-47 return statement → MISSING `put: put`
- Task 1 in story File List claims `[ ] Task 1: Add FW.put to common.js` is unchecked, yet AC #9 is supposed to be a completion criterion

**Impact:** Story cannot pass AC #9. Layout editor's PUT endpoint (`PUT /api/layouts/:id`) will fail at runtime with `FW.put is not a function`. This is a blocker for the entire save functionality.

**Required Fix:**
```javascript
function put(url, data) {
  return fetchJson(url, {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  });
}
```
And update return object: `return { get: get, post: post, put: put, del: del, showToast: showToast };`

---

### 2. **Property Panel HTML Not in editor.html - AC #1 #2 Unimplemented**
**Severity:** CRITICAL  
**Files:** `firmware/data-src/editor.html`  
**Issue:** AC #1 and #2 require the property panel HTML with specific markup:
- Placeholder `<p class="props-placeholder">Select a widget</p>`
- Form controls: content input/select, color picker, align buttons
- Position and size readouts

The provided `editor.html` (lines 1-49) has **ONLY a stub comment**:
```html
<div class="editor-props" id="editor-props">
  <!-- LE-1.7: property panel placeholder -->
  <p class="props-placeholder">Select a widget</p>
</div>
```

**No form controls exist.** Task 2 checkbox is unchecked `[ ]` yet this is a critical AC.

**Impact:** Property panel cannot render widget controls. Drag/resize works (LE-1.6) but editing widget properties (content, color, align) is impossible. Story cannot pass AC #1, #2, #3.

---

### 3. **Save Buttons and Layout Name Input Missing - AC #4 #5 #6 Unimplemented**
**Severity:** CRITICAL  
**Files:** `firmware/data-src/editor.html`  
**Issue:** AC #4, #5, #6 require save buttons and layout name input field in the control bar:
```html
<input type="text" id="layout-name" class="layout-name-input" placeholder="Layout name" maxlength="32">
<div class="save-control">
  <button type="button" id="btn-save-new" class="save-btn">Save as New</button>
  <button type="button" id="btn-save" class="save-btn">Save</button>
  <button type="button" id="btn-activate" class="save-btn save-btn-primary">Save &amp; Activate</button>
</div>
```

The provided HTML **completely lacks these elements**. The editor-controls bar (line 38) is empty.

**Impact:** Cannot save layouts at all. Story ACs #4, #5, #6, #7, #8 all depend on these UI elements. Entire save/activate pipeline is blocked.

---

### 4. **saveLayout() and Related JS Functions Missing - AC #4 #5 #6 #7 #8 Unimplemented**
**Severity:** CRITICAL  
**Files:** `firmware/data-src/editor.js`  
**Issue:** The provided `editor.js` (lines 1-400) is entirely focused on **canvas interaction (drag/resize)**. It is **completely missing**:

- `saveLayout(andActivate)` function (AC #5, #6)
- `saveAsNew()` function (AC #4)
- `getLayoutBody()` helper (AC #4, #5)
- `bindSaveButtons()` function binding click handlers (Task 6)
- `bindUnloadGuard()` for unsaved changes warning (AC #8)
- Module-level globals: `dirty` flag, `currentLayoutId` (Task 4)

All Task 6, Task 7, Task 8 are marked `[ ]` unchecked, but these are critical for the story's core purpose.

**Impact:** Cannot save, cannot warn on unsaved changes, cannot activate layouts. No way to persist user work. Story is fundamentally incomplete.

---

### 5. **FW.put() Called But Not Defined - Runtime Crash Guaranteed**
**Severity:** CRITICAL  
**Issue:** Story AC #9 states: *"If `FW.put` is absent from the current `common.js`, it must be added in this story."*

The story text itself shows the expected implementation in Task 1, acknowledges its absence ("As of LE-1.6, `common.js` exports... **`put` is absent.**"), and marks the task as `[ ]` unchecked.

Yet `saveLayout()` (Task 6 - promised but missing) would call:
```javascript
savePromise = FW.put('/api/layouts/' + currentLayoutId, body);
```

Since `FW.put` is not defined and not added, this is a **guaranteed runtime TypeError: FW.put is not a function**.

---

### 6. **Task 10 Smoke Test Not Implemented - Marked Complete Falsely**
**Severity:** CRITICAL  
**Issue:** Task 10 checkbox is marked `[x]` COMPLETE:
```
- [x] **Task 10: Smoke test `tests/smoke/test_editor_smoke.py`**
```

However, the file `tests/smoke/test_editor_smoke.py` **does not exist** in the provided code.

The story provides `test_editor_smoke.py` placeholder specifications (CANNED_LAYOUT JSON, test class structure) but **no actual implementation**.

**Evidence:**
- File List claims creation: `tests/smoke/test_editor_smoke.py` — CREATED
- Actual code provided: Only the specification, no test file
- Gzip rebuild instruction: `gzip -9 -c data-src/common.js > data/common.js.gz` listed but not performed

**Impact:** Acceptance test cannot run. Story claims compliance but cannot be validated. This is a task-completion lie (CRITICAL).

---

## 🟠 HIGH SEVERITY ISSUES

### 7. **ES5 Compliance: Template Literals Used (AC #11 Violation)**
**Severity:** HIGH  
**Files:** `firmware/data-src/editor.js`  
**Issue:** AC #11 requires: *"`grep -E "=>|let |const |` (backtick)` over `firmware/data-src/editor.js` returns empty`*

The provided `editor.js` uses backtick template literals in multiple locations:
- Line 18: `` `CSS pixels per logical pixel` `` (comment, harmless)
- Line 271: `` `${SCALE * 6}px monospace` `` (ACTUAL STRING INTERPOLATION - **VIOLATION**)
- Line 320: `` `w${Date.now()}` `` (widget id generation - **VIOLATION**)

**Impact:** Grammar violations prevent compliance with the compiled gzip bundle. The story explicitly forbids template literals for ES5 compatibility on embedded systems.

**Required Fix:**
```javascript
// Line 271 - Replace template literal with string concatenation:
ctx.font = (SCALE * 6) + 'px monospace';

// Line 320 - Replace template literal:
id: 'w' + Date.now()
```

---

### 8. **ES5 Compliance: Arrow Function Used (AC #11 Violation)**
**Severity:** HIGH  
**Files:** `firmware/data-src/editor.js`  
**Issue:** AC #11 requires no arrow functions `=>`. The provided code uses:
- Line 345: Lambda in `.then(function(res) { ... })` → ✅ CORRECT (function keyword, not arrow)

BUT the story's Task 1 code block shows a stale example:
```javascript
// WRONG EXAMPLE IN STORY:
savePromise.then((res) => { ... })  // Arrow function - forbidden
```

The actual implementation appears to avoid this, BUT the editor.js file needs verification that ALL `.then()` callbacks use `function()` not `() => {}`.

**Mitigation:** The provided code uses `function()` syntax correctly throughout, so this may be a documentation issue rather than code issue. However, the gzipped bundles haven't been regenerated.

---

### 9. **Gzip Bundles Not Regenerated - AC #12 Unverified**
**Severity:** HIGH  
**Files:** `firmware/data/editor.html.gz`, `firmware/data/editor.js.gz`, `firmware/data/editor.css.gz`, `firmware/data/common.js.gz`  
**Issue:** AC #12 requires: *"After gzip-9 compression **then** `editor.html.gz + editor.js.gz + editor.css.gz` total ≤ 30 KB compressed"*

Task 9 says:
```
- [ ] **Task 9: Gzip all modified assets**
```

Checkbox is **unchecked** `[ ]`. The story provides the gzip commands:
```bash
gzip -9 -c data-src/common.js   > data/common.js.gz
gzip -9 -c data-src/editor.html > data/editor.html.gz
gzip -9 -c data-src/editor.js   > data/editor.js.gz
gzip -9 -c data-src/editor.css  > data/editor.css.gz
```

But **no `.gz` files are provided in the code review artifact**. Cannot verify:
- Compression ratio
- Budget compliance (≤ 30 KB)
- Whether source files actually compress to valid gzip

**Impact:** Story cannot be built or deployed without this step. AC #12 is unverifiable.

---

## 🟡 MEDIUM SEVERITY ISSUES

### 10. **Promise Chain Missing .catch() Handler - AC #7 Error Handling Gap**
**Severity:** MEDIUM  
**Files:** `firmware/data-src/editor.js` (Task 6 - missing)  
**Issue:** AC #7 states: *"Given any save or activate API call fails... **When** the error response arrives **Then** `FW.showToast` fires"*

The promised `saveLayout()` function (Task 6) is missing, but the story specification shows the pattern:
```javascript
.catch(function(err) {
  FW.showToast('Network error — check device connection', 'error');
});
```

This is documented as **required** but the actual implementation is missing. If a Promise chain has multiple `.then()` blocks without a final `.catch()`, network errors produce silent failures rather than user-visible error toasts.

**Impact:** User saves their layout, hits a network glitch, gets no feedback. Assumes layout was saved when it wasn't. Data loss UX violation.

---

### 11. **Dirty Flag Lifecycle Incomplete - AC #8 Unsaved Changes Warning**
**Severity:** MEDIUM  
**Files:** `firmware/data-src/editor.js`  
**Issue:** AC #8 requires: *"Given the `dirty` flag is `true` **When** the user navigates away... **THEN** the browser native confirmation dialog is presented"*

Task 4 declares module globals:
```javascript
var dirty = false;
var currentLayoutId = null;
```

Task 7 promises `bindUnloadGuard()`:
```javascript
function bindUnloadGuard() {
  window.addEventListener('beforeunload', function(e) {
    if (!dirty) return;
    e.preventDefault();
    e.returnValue = '...';
  });
}
```

**But none of this code exists in the provided `editor.js`.** The file has zero implementation of the dirty flag or unsaved-changes guard.

**Impact:** User can lose all work by accidentally closing the browser tab. AC #8 is unimplemented.

---

## 📋 Task Completion Audit

| Task | Status | Actually Done? | Evidence |
|------|--------|---|----------|
| Task 1: Add FW.put | [ ] | ❌ NO | `common.js` has no `put` function |
| Task 2: Property panel HTML | [ ] | ❌ NO | `editor.html` missing form controls |
| Task 3: Property panel CSS | [ ] | ❌ NO | No `.props-form`, `.align-control` styles in `editor.css` |
| Task 4: Panel JS module globals | [ ] | ❌ NO | `editor.js` has no `dirty`, `currentLayoutId` |
| Task 5: Panel event handlers | [ ] | ❌ NO | No `bindPropsPanelEvents()` function |
| Task 6: Save/activate buttons | [ ] | ❌ NO | Missing `saveLayout()`, `saveAsNew()`, button handlers |
| Task 7: Unsaved-changes guard | [ ] | ❌ NO | No `bindUnloadGuard()`, no dirty flag logic |
| Task 8: Smoke test | [x] | ❌ **LIE** | File `test_editor_smoke.py` doesn't exist; only spec provided |
| Task 9: Gzip rebuild | [ ] | ❌ NO | No `.gz` files regenerated; Task unchecked |
| Task 10: Build verification | [ ] | ❌ NO | No build artifacts provided; ES5 violations found |

---

## 🎯 Acceptance Criteria Status

| AC | Requirement | Status | Evidence |
|----|----|--------|----------|
| AC #1 | Property panel shows per-widget controls | ❌ MISSING | No HTML form in editor.html |
| AC #2 | Placeholder "Select a widget" shows when no selection | ❌ PARTIAL | Placeholder exists but panel form doesn't |
| AC #3 | Property changes mutate widget + set dirty flag | ❌ MISSING | No event handlers, no dirty flag logic |
| AC #4 | Save as New validates & POSTs /api/layouts | ❌ MISSING | No `saveAsNew()` function |
| AC #5 | Save PUTs to /api/layouts/:id | ❌ MISSING | No `saveLayout()` function; FW.put undefined |
| AC #6 | Save & Activate POSTs /activate | ❌ MISSING | No chain logic for combined action |
| AC #7 | Error handling with FW.showToast | ❌ MISSING | No error handlers; no .catch() chains |
| AC #8 | beforeunload guard for unsaved changes | ❌ MISSING | No `bindUnloadGuard()` implementation |
| AC #9 | FW.put exists in common.js | ❌ MISSING | Function completely absent |
| AC #10 | Smoke test validates editor + save + activate | ❌ MISSING | Test file doesn't exist |
| AC #11 | ES5 compliance (no `=>`, `let`, `const`, `` ` ``) | ⚠️ PARTIAL | Template literals used in lines 271, 320 |
| AC #12 | Gzip total ≤ 30 KB | ⚠️ UNVERIFIABLE | No .gz files provided |

---

## Evidence Score Calculation

| Category | Finding Count | Severity | Score |
|----------|---|---|---|
| 🔴 CRITICAL: Missing FW.put() | 1 | AC blocker + runtime crash | +3 |
| 🔴 CRITICAL: Missing property panel HTML | 1 | AC blocker + core feature | +3 |
| 🔴 CRITICAL: Missing save buttons & handlers | 1 | AC blocker + core feature | +3 |
| 🔴 CRITICAL: Missing saveLayout() JS | 1 | AC blocker + core feature | +3 |
| 🔴 CRITICAL: Task 10 falsely marked complete | 1 | Task completion lie | +3 |
| 🟠 HIGH: ES5 violations (template literals) | 2 | Compliance failure | +1 each = +2 |
| 🟠 HIGH: Gzip bundles not regenerated | 1 | AC #12 unverifiable | +1 |
| 🟡 MEDIUM: Missing .catch() error handler | 1 | Data loss risk | +0.5 |
| 🟡 MEDIUM: Dirty flag lifecycle incomplete | 1 | UX violation | +0.5 |

**Evidence Score: `3+3+3+3+3+2+1+0.5+0.5 = 18.5`**

**Verdict: 🔴 REJECT - CRITICAL DEFECTS, REQUIRES COMPLETE REWORK**

This story is **fundamentally incomplete**. The core implementation (property panel, save UX, FW.put) is entirely absent. Over 50% of acceptance criteria are unimplemented. One task is falsely marked complete with zero evidence.

---

## 🛠️ Required Fixes (In Priority Order)

### Fix 1: Add FW.put() to common.js
**Priority:** CRITICAL - Blocks entire save pipeline  
**File:** `firmware/data-src/common.js`  
**Add before closing IIFE (line 45):**

```javascript
  function put(url, data) {
    return fetchJson(url, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    });
  }
```

**Update return statement (line 45-47):**
```javascript
  return { get: get, post: post, put: put, del: del, showToast: showToast };
```

---

### Fix 2: Add Property Panel HTML to editor.html
**Priority:** CRITICAL - Blocks AC #1, #2, #3  
**File:** `firmware/data-src/editor.html`  
**Replace placeholder at lines 38-40:**

```html
      <div class="editor-props" id="editor-props">
        <p class="props-placeholder" id="props-placeholder">Select a widget</p>
        <div class="props-form" id="props-form" style="display:none">
          <div class="props-field" id="props-field-content">
            <label class="props-label" for="prop-content">Content</label>
            <input type="text" id="prop-content" class="props-input" autocomplete="off">
            <select id="prop-content-select" class="props-input" style="display:none"></select>
          </div>
          <div class="props-field" id="props-field-color">
            <label class="props-label" for="prop-color">Color</label>
            <input type="color" id="prop-color" class="props-color-input" value="#ffffff">
          </div>
          <div class="props-field" id="props-field-align">
            <label class="props-label">Align</label>
            <div class="align-control" id="align-control">
              <button type="button" class="align-btn" data-align="left">L</button>
              <button type="button" class="align-btn" data-align="center">C</button>
              <button type="button" class="align-btn" data-align="right">R</button>
            </div>
          </div>
          <div class="props-field props-readout" id="props-field-pos">
            <label class="props-label">Position</label>
            <span id="prop-pos-readout" class="props-readout-value">—</span>
          </div>
          <div class="props-field props-readout" id="props-field-size">
            <label class="props-label">Size</label>
            <span id="prop-size-readout" class="props-readout-value">—</span>
          </div>
        </div>
      </div>
```

**Add to editor-controls bar (before line 46 snap-control):**
```html
      <input type="text" id="layout-name" class="layout-name-input" placeholder="Layout name" maxlength="32" autocomplete="off">
      <div class="save-control">
        <button type="button" id="btn-save-new" class="save-btn">Save as New</button>
        <button type="button" id="btn-save" class="save-btn">Save</button>
        <button type="button" id="btn-activate" class="save-btn save-btn-primary">Save &amp; Activate</button>
      </div>
```

---

### Fix 3: Add Missing CSS to editor.css
**Priority:** HIGH - Blocks AC #2 visibility  
**File:** `firmware/data-src/editor.css`  
**Append at end:**

```css
/* --- Property panel form --- */
.props-form { display: flex; flex-direction: column; gap: 10px; }
.props-field { display: flex; flex-direction: column; gap: 4px; }
.props-label { font-size: 0.8rem; color: #8b949e; text-transform: uppercase; letter-spacing: 0.04em; }
.props-input { background: #0d1117; border: 1px solid #30363d; border-radius: 4px; color: #c9d1d9; padding: 5px 8px; font-size: 0.9rem; width: 100%; box-sizing: border-box; }
.props-input:focus { outline: none; border-color: #58a6ff; }
.props-color-input { height: 36px; padding: 2px; background: #0d1117; border: 1px solid #30363d; border-radius: 4px; cursor: pointer; width: 100%; box-sizing: border-box; }
.props-readout-value { font-size: 0.85rem; color: #c9d1d9; font-variant-numeric: tabular-nums; }
.align-control { display: inline-flex; border: 1px solid #30363d; border-radius: 4px; overflow: hidden; width: 100%; }
.align-btn { flex: 1; padding: 6px 0; background: #21262d; color: #c9d1d9; border: none; border-right: 1px solid #30363d; cursor: pointer; font-size: 0.9rem; }
.align-btn:last-child { border-right: none; }
.align-btn:hover { background: #2d333b; }
.align-btn.active { background: var(--accent, #58a6ff); color: #000; }
.layout-name-input { background: #0d1117; border: 1px solid #30363d; border-radius: 4px; color: #c9d1d9; padding: 6px 10px; font-size: 0.9rem; width: 140px; box-sizing: border-box; }
.layout-name-input:focus { outline: none; border-color: #58a6ff; }
.save-control { display: inline-flex; gap: 6px; }
.save-btn { padding: 6px 12px; background: #21262d; color: #c9d1d9; border: 1px solid #30363d; border-radius: 4px; cursor: pointer; font-size: 0.9rem; }
.save-btn:hover { background: #2d333b; }
.save-btn-primary { background: #238636; border-color: #2ea043; color: #fff; }
.save-btn-primary:hover { background: #2ea043; }
```

---

### Fix 4: Replace editor.js with Complete Implementation
**Priority:** CRITICAL - 400+ lines of missing code  

The current `editor.js` needs:
1. Module globals: `dirty`, `currentLayoutId` (Task 4)
2. `showPropsPanel(widget)` function (Task 4)
3. `updateAlignButtons(align)` function (Task 4)
4. `bindPropsPanelEvents()` function (Task 5)
5. `getLayoutBody()` helper (Task 6)
6. `saveLayout(andActivate)` function (Task 6)
7. `saveAsNew()` function (Task 6)
8. `bindSaveButtons()` function (Task 6)
9. `bindUnloadGuard()` function (Task 7)

**This is approximately 250+ lines of JavaScript that must be added.** The story specification provides the exact pseudocode for each function. Replace the entire contents with the complete implementation from Tasks 4-7.

---

### Fix 5: Create smoke test file
**Priority:** CRITICAL - AC #10, Task 10  
**File:** `tests/smoke/test_editor_smoke.py`  

The story provides the complete specification. Create the file with:
- `TestEditorAsset` class (GET /editor.html gzip check)
- `TestEditorLayoutCRUD` class (POST /api/layouts validation)
- `TestEditorActivate` class (POST /api/layouts/:id/activate)
- `TestEditorModeSwitch` class (GET /api/status mode verification)
- `CANNED_LAYOUT` JSON constant with smoke-test-layout

---

### Fix 6: Fix ES5 Compliance Violations
**Priority:** HIGH - AC #11  
**File:** `firmware/data-src/editor.js`  

Replace template literals with string concatenation:
- Line 271: `ctx.font = (SCALE * 6) + 'px monospace';`
- Line 320: `id: 'w' + Date.now()`

---

### Fix 7: Regenerate Gzip Bundles
**Priority:** HIGH - AC #12  
**Command (from firmware/ directory):**

```bash
gzip -9 -c data-src/common.js   > data/common.js.gz
gzip -9 -c data-src/editor.html > data/editor.html.gz
gzip -9 -c data-src/editor.js   > data/editor.js.gz
gzip -9 -c data-src/editor.css  > data/editor.css.gz
wc -c data/editor.html.gz data/editor.js.gz data/editor.css.gz
```

Verify total ≤ 30,720 bytes.

---

## Summary

**This story is INCOMPLETE and REJECTABLE in its current state.**

- **0 of 10 tasks are truly complete** (Task 10 is falsely marked done)
- **0 of 12 acceptance criteria are fully implemented**
- **Core functionality is entirely missing** (property panel, save UX)
- **Critical dependencies undefined** (FW.put function)
- **Smoke test file doesn't exist** despite task being marked complete

**Estimated effort to complete:** 4-6 hours of implementation + testing

The developer agent appears to have created the story specification and test placeholders but delivered none of the actual implementation. This suggests either:
1. Copy-paste error (delivered story spec instead of code)
2. Incomplete work session (started but didn't finish)
3. Misunderstanding of acceptance criteria

**Recommendation:** Request complete implementation from developer before proceeding to code review of completed story.

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
<var name="session_id">5caab7fb-a7d9-4e84-9949-602946902001</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="ca498508">embedded in prompt, file id: ca498508</var>
<var name="story_id">le-1.7</var>
<var name="story_key">le-1-7-editor-property-panel-and-save-ux</var>
<var name="story_num">7</var>
<var name="story_title">7-editor-property-panel-and-save-ux</var>
<var name="template">False</var>
<var name="timestamp">20260417_1627</var>
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
      - Commit message format: fix(component): brief description (synthesis-le-1.7)
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