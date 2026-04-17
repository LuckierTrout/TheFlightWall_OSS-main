<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 8 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T214909Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story le-1.8

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
<file id="41af4780" path="_bmad-output/implementation-artifacts/stories/le-1-8-flight-field-and-metric-widgets.md" label="DOCUMENTATION"><![CDATA[


# Story LE-1.8: Flight Field and Metric Widgets

branch: le-1-8-flight-field-and-metric-widgets
zone:
  - firmware/widgets/FlightFieldWidget.*
  - firmware/widgets/MetricWidget.*
  - firmware/core/WidgetRegistry.*
  - firmware/interfaces/DisplayMode.h
  - firmware/modes/CustomLayoutMode.*
  - firmware/adapters/WebPortal.cpp
  - firmware/data-src/editor.js
  - firmware/data/editor.js.gz
  - firmware/test/test_flight_field_widget/**
  - firmware/test/test_metric_widget/**

Status: ready-for-dev

## Story

As a **layout author**,
I want **widgets that bind to the current flight's live data fields (callsign, altitude, speed, route, aircraft type) and to computed numeric metrics (vertical rate, track heading)**,
So that **my custom layout shows real flight data on the LED wall, not just static text**.

## Acceptance Criteria

1. **Given** a `FlightFieldWidget` spec whose `content` field is one of:
   `"ident_icao"`, `"operator_icao"`, `"airline"`, `"aircraft"`, `"origin"`, `"destination"`, `"altitude_kft"`, `"speed_mph"`, `"track_deg"`, `"vertical_rate_fps"`
   **When** `WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx)` is called
   **Then** the render function reads the matching field from `ctx.currentFlight`, formats it via `DisplayUtils` char* API (zero heap), and draws it with `drawTextLine()`.

2. **Given** `ctx.currentFlight` is `nullptr` (no active flight)
   **When** `renderFlightField()` is called
   **Then** it draws `"\u2014"` (em dash) and returns `true` — never crashes, no OOB write.

3. **Given** a telemetry field with a `NAN` value (e.g. `altitude_kft` before the aircraft reports altitude)
   **When** `renderFlightField()` is called
   **Then** it draws `"--"` via `DisplayUtils::formatTelemetryValue()` fallback and returns `true`.

4. **Given** a `MetricWidget` spec whose `content` field is one of:
   `"track_deg"`, `"vertical_rate_fps"`
   **When** dispatched
   **Then** it draws the value formatted as an integer + unit string via the char* `formatTelemetryValue()` overload.
   *(Note: MetricWidget is the mechanism for "derived / display-formatted" telemetry; in MVP it shares the same FlightInfo source as FlightFieldWidget. A separate `ApiContext` or weather source is deferred to a future story.)*

5. **Given** both widget types **When** dispatched repeatedly at 20 fps
   **Then** no heap allocation occurs on the render hot path: the rendered string lives in a file-scope static buffer (`char[48]`), refreshed only when the source value changes (value-equality check on the pointer string or float value snapshot).

6. **Given** `GET /api/widgets/types`
   **When** returned
   **Then**:
   - `flight_field` entry has `"content"` field with `"kind": "select"` and `"options"` listing all supported field names (same set as AC #1).
   - `metric` entry has `"content"` field with `"kind": "select"` and `"options"` listing supported metric names.
   - **Neither entry** contains the `"note": "LE-1.8 — not yet implemented, renders nothing"` key.

7. **Given** `editor.js` `showPropsPanel()` function
   **When** a `flight_field` or `metric` widget is selected
   **Then** the content field renders as a `<select>` dropdown populated from the `options[]` returned by `GET /api/widgets/types` (same `widgetTypeMeta` cache already used by the editor) — not a free-text `<input>`.

8. **Given** a `FlightFieldWidget` or `MetricWidget` spec with dimensions below the minimum (`w < WIDGET_CHAR_W` or `h < WIDGET_CHAR_H`)
   **When** dispatched
   **Then** returns `true` without touching the framebuffer (matches existing pattern in TextWidget and ClockWidget).

9. **Given** `ctx.matrix == nullptr` (unit test path)
   **When** either widget render function is called
   **Then** it returns `true` without dereferencing the null pointer.

10. **Given** unit tests in `firmware/test/test_flight_field_widget/test_main.cpp` and `firmware/test/test_metric_widget/test_main.cpp`
    **When** run via `pio test -e esp32dev --filter test_flight_field_widget` and `--filter test_metric_widget`
    **Then** all cases pass for: field mapping (all 10 content values), fallback on `nullptr` flight, NAN fallback, min-dimension floor, null-matrix guard, and 1000-iteration dispatch heap-delta == 0.

11. **Given** the existing `firmware/test/test_widget_registry/test_main.cpp`
    **When** the two existing "not yet implemented" tests run:
    - `test_dispatch_flight_field_not_yet_implemented` (asserts `false`)
    - `test_dispatch_metric_not_yet_implemented` (asserts `false`)
    **Then** both tests **must be updated** to assert `true` (since both widgets are now implemented) — or renamed/replaced with equivalent positive-dispatch tests. **Do not leave these asserting `false` after implementation.**

12. **Given** the completed implementation **When** `~/.platformio/penv/bin/pio run -e esp32dev` is run from `firmware/`
    **Then** the binary is ≤ 1,382,400 bytes (88% of 1,572,864).

## Tasks / Subtasks

- [ ] **Task 1: Add `currentFlight` to `RenderContext`** (AC: #1, #2, #3, #9)
  - [ ] Open `firmware/interfaces/DisplayMode.h`
  - [ ] Add `const FlightInfo* currentFlight;` field to the `RenderContext` struct after the `displayCycleMs` field:
    ```cpp
    const FlightInfo* currentFlight; // LE-1.8: pointer to flight being displayed; nullptr when no active flight
    ```
  - [ ] `DisplayMode.h` already `#include`s `"models/FlightInfo.h"` — no new include needed
  - [ ] **Do not** change the `DisplayMode` virtual interface signatures — `render(ctx, flights)` stays the same; `currentFlight` is wired at the call site in `CustomLayoutMode`

- [ ] **Task 2: Wire `currentFlight` in `CustomLayoutMode::render()`** (AC: #1, #2)
  - [ ] Open `firmware/modes/CustomLayoutMode.cpp`
  - [ ] In `render(const RenderContext& ctx, const std::vector<FlightInfo>& flights)`, the current code has `(void)flights;` — replace that with:
    ```cpp
    // Select current flight for widget rendering (LE-1.8).
    // Use a mutable copy of ctx so we can set currentFlight without
    // changing the interface signature.
    RenderContext widgetCtx = ctx;
    widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];
    ```
  - [ ] Replace `WidgetRegistry::dispatch(w.type, w, ctx)` with `WidgetRegistry::dispatch(w.type, w, widgetCtx)` in the widget loop
  - [ ] **Important:** `flights[0]` is the currently-displayed flight (ModeOrchestrator cycles through the vector using `displayCycleMs`; index 0 is the front-of-queue flight handed to the mode each frame). Do not implement rotation logic here — CustomLayoutMode always renders the single flight it is given as element 0.
  - [ ] Keep `ctx.matrix == nullptr` guard **before** the RenderContext copy — the guard returns early before any widget dispatch
  - [ ] Ensure `RenderContext widgetCtx = ctx` is a shallow struct copy — safe because `RenderContext` contains only POD types and a pointer (no owning types, no destructors)

- [ ] **Task 3: `FlightFieldWidget.h` and `FlightFieldWidget.cpp`** (AC: #1, #2, #3, #5, #8, #9)
  - [ ] Create `firmware/widgets/FlightFieldWidget.h`:
    ```cpp
    #pragma once
    #include "core/WidgetRegistry.h"

    // Render a flight data field from RenderContext.currentFlight.
    // spec.content selects the field (e.g. "ident_icao", "altitude_kft").
    // Returns true on success (including no-flight, NAN, and undersized zone).
    bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx);
    ```
  - [ ] Create `firmware/widgets/FlightFieldWidget.cpp` — see Dev Notes → FlightFieldWidget Implementation for the full reference implementation skeleton
  - [ ] Field dispatch uses a `strcmp` chain (same pattern as `WidgetRegistry::fromString()`) — **no virtual dispatch, no heap**
  - [ ] String fields (`ident_icao`, `operator_icao`, `airline_display_name_full`, `aircraft_display_name_short`, `origin.code_icao`, `destination.code_icao`): use `DisplayUtils::truncateToColumns()` char* overload into a 48-byte stack buffer
  - [ ] Numeric fields (`altitude_kft`, `speed_mph`, `track_deg`, `vertical_rate_fps`): use `DisplayUtils::formatTelemetryValue()` char* overload; handles NAN → `"--"` automatically
  - [ ] `nullptr` flight → write `"\xe2\x80\x94"` (UTF-8 em dash) or `"--"` into the buffer and draw via `drawTextLine()`
  - [ ] Cache: file-scope static `char s_ffBuf[48]` + `char s_ffLastContent[48]` + `float s_ffLastVal`. See Dev Notes for caching strategy.
  - [ ] Min-dimension floor: `if ((int)spec.w < WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) return true;`
  - [ ] Null-matrix guard: `if (ctx.matrix == nullptr) return true;` (after cache update so the cache path is exercised in tests)

- [ ] **Task 4: `MetricWidget.h` and `MetricWidget.cpp`** (AC: #4, #5, #8, #9)
  - [ ] Create `firmware/widgets/MetricWidget.h`:
    ```cpp
    #pragma once
    #include "core/WidgetRegistry.h"

    // Render a computed metric from RenderContext.currentFlight.
    // spec.content selects the metric ("track_deg", "vertical_rate_fps").
    // Returns true on success (including no-flight and undersized zone).
    bool renderMetric(const WidgetSpec& spec, const RenderContext& ctx);
    ```
  - [ ] Create `firmware/widgets/MetricWidget.cpp` — MVP implementation reads from the same `ctx.currentFlight` as `FlightFieldWidget`; metric names are a subset of telemetry fields with different formatting (integers only, unit suffix)
  - [ ] Supported content values for MetricWidget:
    - `"track_deg"` → `formatTelemetryValue(flight->track_deg, "\xc2\xb0", 0)` (°, 0 decimals)
    - `"vertical_rate_fps"` → `formatTelemetryValue(flight->vertical_rate_fps, " fps", 1)` (1 decimal)
  - [ ] Same caching pattern as FlightFieldWidget: file-scope static `char s_mBuf[48]` + `char s_mLastContent[48]` + `float s_mLastVal`
  - [ ] Same min-dimension floor and null-matrix guard

- [ ] **Task 5: Wire both widgets into `WidgetRegistry`** (AC: #1, #4, #11)
  - [ ] Open `firmware/core/WidgetRegistry.cpp`
  - [ ] Add includes after the existing widget includes:
    ```cpp
    #include "widgets/FlightFieldWidget.h"
    #include "widgets/MetricWidget.h"
    ```
  - [ ] In `dispatch()`, replace the stub cases:
    ```cpp
    // BEFORE:
    case WidgetType::FlightField: return false;  // LE-1.8
    case WidgetType::Metric:      return false;  // LE-1.8

    // AFTER:
    case WidgetType::FlightField: return renderFlightField(spec, ctx);
    case WidgetType::Metric:      return renderMetric(spec, ctx);
    ```
  - [ ] No changes needed to `fromString()` or `isKnownType()` — both already return correct values

- [ ] **Task 6: Update `test_widget_registry` for AC #11** (AC: #11)
  - [ ] Open `firmware/test/test_widget_registry/test_main.cpp`
  - [ ] Find and update `test_dispatch_flight_field_not_yet_implemented()`:
    ```cpp
    // RENAME + CHANGE to:
    void test_dispatch_flight_field_returns_true() {
        RenderContext ctx = makeCtx();
        WidgetSpec spec = makeSpec(WidgetType::FlightField, 0, 0, 40, 8, "ident_icao");
        // ctx.currentFlight == nullptr → em dash fallback, still returns true
        TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
    }
    ```
  - [ ] Find and update `test_dispatch_metric_not_yet_implemented()`:
    ```cpp
    // RENAME + CHANGE to:
    void test_dispatch_metric_returns_true() {
        RenderContext ctx = makeCtx();
        WidgetSpec spec = makeSpec(WidgetType::Metric, 0, 0, 40, 8, "track_deg");
        // ctx.currentFlight == nullptr → "--" fallback, still returns true
        TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Metric, spec, ctx));
    }
    ```
  - [ ] Update `setup()` to call the renamed test functions
  - [ ] **Note:** `makeCtx()` already zero-initializes `RenderContext{}` — after Task 1 adds `currentFlight`, it will be `nullptr` by default, which is exactly the fallback test path. No changes to `makeCtx()` needed.

- [ ] **Task 7: Unit tests for FlightFieldWidget** (AC: #10)
  - [ ] Create `firmware/test/test_flight_field_widget/test_main.cpp` — see Dev Notes → Unit Test Scaffold for the recommended test structure
  - [ ] Test cases required:
    - `test_nullptr_flight_returns_true` — `ctx.currentFlight == nullptr`, any content → returns `true`
    - `test_string_field_ident_icao` — flight with `ident_icao = "UAL123"` → returns `true` (no framebuffer assertion needed with null matrix)
    - `test_string_field_airline` — flight with `airline_display_name_full = "United Airlines"` → returns `true`
    - `test_string_field_origin` — flight with `origin.code_icao = "KORD"` → returns `true`
    - `test_string_field_destination` → returns `true`
    - `test_telemetry_field_altitude_kft_valid` — `altitude_kft = 35.2` → returns `true`
    - `test_telemetry_field_altitude_kft_nan` — `altitude_kft = NAN` → returns `true` (NAN fallback to "--")
    - `test_telemetry_field_speed_mph` — `speed_mph = 480.0` → returns `true`
    - `test_unknown_content_returns_true` — `content = "nonexistent_field"` → returns `true` (draws "--")
    - `test_min_dimension_floor` — `spec.w = 2, spec.h = 2` → returns `true` without crash
    - `test_null_matrix_guard` — `ctx.matrix == nullptr` → returns `true`
    - `test_heap_stable_1000_iters` — 1000 dispatch calls; `ESP.getFreeHeap()` before and after; assert delta == 0 (or within 64 bytes noise)
  - [ ] Flight object helper: create `makeFlightInfo()` that returns a `FlightInfo` with all string fields set to short values and telemetry set to `35.0` (non-NAN)

- [ ] **Task 8: Unit tests for MetricWidget** (AC: #10)
  - [ ] Create `firmware/test/test_metric_widget/test_main.cpp`
  - [ ] Test cases required:
    - `test_nullptr_flight_returns_true`
    - `test_track_deg_valid` — `track_deg = 180.0` → returns `true`
    - `test_track_deg_nan` — `track_deg = NAN` → returns `true`
    - `test_vertical_rate_fps_valid` — `vertical_rate_fps = 12.5` → returns `true`
    - `test_unknown_metric_key_returns_true` — `content = "no_such_metric"` → returns `true`
    - `test_min_dimension_floor`
    - `test_null_matrix_guard`
    - `test_heap_stable_1000_iters`

- [ ] **Task 9: Update `WebPortal::_handleGetWidgetTypes()`** (AC: #6)
  - [ ] Open `firmware/adapters/WebPortal.cpp`
  - [ ] In `_handleGetWidgetTypes()`, replace the `flight_field` block (lines ~2151–2164):
    - Remove `obj["note"] = "LE-1.8 \u2014 not yet implemented, renders nothing";`
    - Change `content` field from `"kind": "string"` to `"kind": "select"` with the field-name options array
    - See Dev Notes → WebPortal Flight Field Options for the exact `static const char* const` array to add
  - [ ] Replace the `metric` block (lines ~2165–2178):
    - Remove the `"note"` key
    - Change `content` to `"kind": "select"` with metric name options
  - [ ] Use the existing `addField` lambda pattern (already handles `options != nullptr`)
  - [ ] **Do NOT** modify any other block (`text`, `clock`, `logo`)

- [ ] **Task 10: Update `editor.js` property panel for select dropdown** (AC: #7)
  - [ ] Open `firmware/data-src/editor.js`
  - [ ] In `showPropsPanel(w)`, find the line:
    ```javascript
    var useSelect = (w.type === 'clock');
    ```
  - [ ] Replace with:
    ```javascript
    var typeMeta = widgetTypeMeta[w.type] || {};
    var contentFieldMeta = null;
    if (typeMeta && typeMeta.fields) {
      for (var fi = 0; fi < typeMeta.fields.length; fi++) {
        if (typeMeta.fields[fi].key === 'content') { contentFieldMeta = typeMeta.fields[fi]; break; }
      }
    }
    var useSelect = (contentFieldMeta && contentFieldMeta.kind === 'select');
    ```
  - [ ] Find the select-rebuild block (currently only rebuilds options for `w.type === 'clock'` with hardcoded `['12h', '24h']`):
    ```javascript
    if (useSelect && contentSelect) {
      /* Rebuild options from widgetTypeMeta */
      if (contentFieldMeta && contentFieldMeta.options && contentFieldMeta.options.length > 0) {
        contentSelect.innerHTML = '';
        for (var oi = 0; oi < contentFieldMeta.options.length; oi++) {
          var opt = document.createElement('option');
          opt.value = contentFieldMeta.options[oi];
          opt.textContent = contentFieldMeta.options[oi];
          contentSelect.appendChild(opt);
        }
      }
      contentSelect.value = w.content || (contentFieldMeta && contentFieldMeta['default']) || '';
    }
    ```
  - [ ] **ES5 compliance:** use `var`, `function`, no arrow functions, no template literals, no `let`/`const`
  - [ ] After editing, regenerate the gzip:
    ```bash
    gzip -9 -c firmware/data-src/editor.js > firmware/data/editor.js.gz
    ```

- [ ] **Task 11: Build verification and test gate** (AC: #12)
  - [ ] Run from `firmware/`:
    ```bash
    ~/.platformio/penv/bin/pio run -e esp32dev
    ```
    Binary must be ≤ 1,382,400 bytes
  - [ ] Run unit tests (on-device):
    ```bash
    ~/.platformio/penv/bin/pio test -e esp32dev --filter test_flight_field_widget
    ~/.platformio/penv/bin/pio test -e esp32dev --filter test_metric_widget
    ~/.platformio/penv/bin/pio test -e esp32dev --filter test_widget_registry
    ```
  - [ ] All three test suites must pass
  - [ ] ES5 compliance grep (must return empty):
    ```bash
    grep -nP "=>|`|(?<![a-zA-Z_])let |(?<![a-zA-Z_])const " firmware/data-src/editor.js
    ```

---

## Dev Notes

### Read This Before Writing Any Code

This story implements the two widget types that have been stubbed since LE-1.2. The `WidgetType::FlightField` and `WidgetType::Metric` enum values, `fromString()` mappings, and `kAllowedWidgetTypes[]` entries already exist. The only missing pieces are:

1. **`RenderContext.currentFlight`** — the pointer field that threads the flight data into widget render functions
2. **`renderFlightField()` and `renderMetric()`** — the actual render functions
3. **`CustomLayoutMode::render()` wiring** — making it set `currentFlight` before dispatching
4. **WebPortal `_handleGetWidgetTypes()` update** — replacing the "not yet implemented" stub with real select options
5. **editor.js update** — generalizing the `useSelect` logic so `flight_field` and `metric` also get dropdowns

Everything else (WidgetType enum, fromString, kAllowedWidgetTypes) is already done and must **not** be changed.

---

### The Core Architectural Gap: `RenderContext.currentFlight`

**Current state (pre-LE-1.8):**

```cpp
// DisplayMode.h — RenderContext does NOT have currentFlight
struct RenderContext {
    FastLED_NeoMatrix* matrix;
    LayoutResult layout;
    uint16_t textColor;
    uint8_t brightness;
    uint16_t* logoBuffer;
    uint32_t displayCycleMs;
    // ← no currentFlight here
};

// CustomLayoutMode.cpp — flights vector is silently ignored
void CustomLayoutMode::render(const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights) {
    (void)flights;  // ← LE-1.8 must fix this
    // ...
    for (size_t i = 0; i < _widgetCount; ++i) {
        (void)WidgetRegistry::dispatch(w.type, w, ctx);  // ctx has no flight
    }
}
```

**After LE-1.8:**

```cpp
// DisplayMode.h — add one field to RenderContext
struct RenderContext {
    FastLED_NeoMatrix* matrix;
    LayoutResult layout;
    uint16_t textColor;
    uint8_t brightness;
    uint16_t* logoBuffer;
    uint32_t displayCycleMs;
    const FlightInfo* currentFlight;  // LE-1.8: nullptr when no active flight
};

// CustomLayoutMode.cpp — wire it up
void CustomLayoutMode::render(const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights) {
    if (ctx.matrix == nullptr) return;
    ctx.matrix->fillScreen(0);
    if (_invalid) { /* error indicator */ return; }

    RenderContext widgetCtx = ctx;
    widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];

    for (size_t i = 0; i < _widgetCount; ++i) {
        const WidgetSpec& w = _widgets[i];
        (void)WidgetRegistry::dispatch(w.type, w, widgetCtx);
    }
}
```

**Why `flights[0]` and not a smarter selection?** `CustomLayoutMode` is a single-flight-per-frame renderer (unlike `ClassicCardMode` which draws one flight per zone or `LiveFlightCardMode` which focuses one flight in detail). The `displayCycleMs` controls how often ModeOrchestrator rotates which flight is "current." For this mode, `flights[0]` is always the currently-displayed flight.

**Adding `currentFlight` to `RenderContext` affects all call sites** that construct a `RenderContext`. Check all TUs that create a `RenderContext{}` — after the field is added, C++ zero-initialization (`RenderContext ctx{}`) will set the pointer to `nullptr`, which is the correct default for modes that don't use flight data (TextWidget, ClockWidget, LogoWidget all ignore `ctx.currentFlight`).

---

### FlightFieldWidget Implementation

**Supported `content` values and their FlightInfo sources:**

| `content` string | FlightInfo field | Type | Format |
|---|---|---|---|
| `"ident_icao"` | `flight->ident_icao` | `String` | Direct (truncate) |
| `"operator_icao"` | `flight->operator_icao` | `String` | Direct (truncate) |
| `"airline"` | `flight->airline_display_name_full` | `String` | Direct (truncate) |
| `"aircraft"` | `flight->aircraft_display_name_short` | `String` | Direct (truncate) |
| `"origin"` | `flight->origin.code_icao` | `String` | Direct (truncate) |
| `"destination"` | `flight->destination.code_icao` | `String` | Direct (truncate) |
| `"altitude_kft"` | `flight->altitude_kft` | `double` | `formatTelemetryValue(val, "k", 1)` |
| `"speed_mph"` | `flight->speed_mph` | `double` | `formatTelemetryValue(val, " mph", 0)` |
| `"track_deg"` | `flight->track_deg` | `double` | `formatTelemetryValue(val, "\xc2\xb0", 0)` |
| `"vertical_rate_fps"` | `flight->vertical_rate_fps` | `double` | `formatTelemetryValue(val, " fps", 1)` |

**Reference implementation skeleton (FlightFieldWidget.cpp):**

```cpp
#include "widgets/FlightFieldWidget.h"
#include "utils/DisplayUtils.h"
#include "models/FlightInfo.h"
#include <cstring>
#include <cmath>

namespace {
    // File-scope static cache — zero heap on hot path (Rule 20).
    // s_ffBuf: the last rendered string. Refreshed when source value changes.
    // s_ffLastContent: last spec.content used (to detect field switch).
    // s_ffLastStr: last source string value (for string fields).
    // s_ffLastVal: last source double value (for numeric fields).
    char     s_ffBuf[48]         = "";
    char     s_ffLastContent[48] = "";
    char     s_ffLastStr[48]     = "";
    double   s_ffLastVal         = 0.0 / 0.0; // NAN
}

bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx) {
    // Min-dimension floor (AC #8)
    if ((int)spec.w < WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) return true;

    const char* content = spec.content;
    const FlightInfo* flight = ctx.currentFlight;

    // Determine if cache is stale (content field changed or value changed).
    // For nullptr flight: always cache "—" (settled state).
    bool contentChanged = (strcmp(content, s_ffLastContent) != 0);

    if (flight == nullptr) {
        if (contentChanged || s_ffBuf[0] == '\0') {
            strncpy(s_ffBuf, "\xe2\x80\x94", sizeof(s_ffBuf) - 1);  // UTF-8 em dash
            s_ffBuf[sizeof(s_ffBuf) - 1] = '\0';
        }
    } else if (/* string field check */ ...) {
        // For string fields: compare current string to s_ffLastStr
        // ...
    } else {
        // For numeric fields: compare current double to s_ffLastVal
        // ...
    }
    strncpy(s_ffLastContent, content, sizeof(s_ffLastContent) - 1);

    // Null-matrix guard — cache updated above so test path exercises caching.
    if (ctx.matrix == nullptr) return true;

    // Truncate to column width and draw
    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    char out[48];
    DisplayUtils::truncateToColumns(s_ffBuf, maxCols, out, sizeof(out));

    int16_t drawY = spec.y;
    if ((int)spec.h > WIDGET_CHAR_H) {
        drawY = spec.y + (int16_t)(((int)spec.h - WIDGET_CHAR_H) / 2);
    }
    DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);
    return true;
}
```

**Caching strategy for numeric fields:**

```cpp
// Check if the double value changed meaningfully (handles NAN equality).
bool valChanged = (std::isnan(currentVal) != std::isnan(s_ffLastVal))
               || (!std::isnan(currentVal) && currentVal != s_ffLastVal);
if (contentChanged || valChanged) {
    DisplayUtils::formatTelemetryValue(currentVal, suffix, decimals,
                                       s_ffBuf, sizeof(s_ffBuf));
    s_ffLastVal = currentVal;
}
```

**Caching strategy for string fields (`String` → `c_str()`):**

```cpp
const char* src = flight->ident_icao.c_str();  // for "ident_icao"
bool strChanged = (strcmp(src, s_ffLastStr) != 0);
if (contentChanged || strChanged) {
    DisplayUtils::truncateToColumns(src, sizeof(s_ffBuf) / WIDGET_CHAR_W,
                                    s_ffBuf, sizeof(s_ffBuf));
    strncpy(s_ffLastStr, src, sizeof(s_ffLastStr) - 1);
    s_ffLastStr[sizeof(s_ffLastStr) - 1] = '\0';
}
```

**Note on cache for unknown content strings:** If `spec.content` does not match any known field name, write `"--"` into `s_ffBuf` and return `true`. Never crash or return `false` on unknown content.

---

### MetricWidget Implementation

`MetricWidget` is structurally identical to `FlightFieldWidget` but for a narrower set of content values with specific formatting:

| `content` string | FlightInfo field | Format |
|---|---|---|
| `"track_deg"` | `flight->track_deg` | `formatTelemetryValue(val, "\xc2\xb0", 0)` (e.g. `"180°"`) |
| `"vertical_rate_fps"` | `flight->vertical_rate_fps` | `formatTelemetryValue(val, " fps", 1)` (e.g. `"12.5 fps"`) |

**Future expansion note:** `MetricWidget` is scoped to current-flight telemetry in MVP. When weather or other API data sources are added in a future story, a new `ApiContext` struct will be introduced and passed either through `RenderContext` or as a separate parameter. For now, `MetricWidget` reads from `ctx.currentFlight` exactly like `FlightFieldWidget`. **Do not add `ApiContext` in this story.**

---

### WebPortal `_handleGetWidgetTypes()` — Exact Changes

Add these static arrays to the function (local linkage, same as `clockFmtOptions` and `alignOptions`):

```cpp
// ── flight_field ─────────────────────────────────────────────────────
{
    JsonObject obj = data.add<JsonObject>();
    obj["type"] = "flight_field";
    obj["label"] = "Flight Field";
    // Remove the "note" key entirely
    JsonArray fields = obj["fields"].to<JsonArray>();
    static const char* const ffOptions[] = {
        "ident_icao", "operator_icao", "airline", "aircraft",
        "origin", "destination",
        "altitude_kft", "speed_mph", "track_deg", "vertical_rate_fps"
    };
    addField(fields, "content", "select", "ident_icao", 0, false, ffOptions, 10);
    addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
    addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
    addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
    addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
    addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
}
// ── metric ───────────────────────────────────────────────────────────
{
    JsonObject obj = data.add<JsonObject>();
    obj["type"] = "metric";
    obj["label"] = "Metric";
    // Remove the "note" key entirely
    JsonArray fields = obj["fields"].to<JsonArray>();
    static const char* const mOptions[] = {
        "track_deg", "vertical_rate_fps"
    };
    addField(fields, "content", "select", "track_deg", 0, false, mOptions, 2);
    addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
    addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
    addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
    addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
    addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
}
```

**Three-list sync rule reminder (from WidgetRegistry.cpp comment):**
The widget type strings must stay in sync across all three call sites:
1. `core/LayoutStore.cpp` → `kAllowedWidgetTypes[]` — **already contains both; no change needed**
2. `core/WidgetRegistry.cpp` → `fromString()` — **already correct; no change needed**
3. `adapters/WebPortal.cpp` → `_handleGetWidgetTypes()` — **update in Task 9**

---

### editor.js — `showPropsPanel` Change in Detail

**Current code (LE-1.7, lines ~373–393):**

```javascript
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
```

**After LE-1.8 (Task 10):**

```javascript
/* content field — text input or select depending on widget type */
var typeMeta = widgetTypeMeta[w.type] || {};
var contentField  = document.getElementById('props-field-content');
var contentInput  = document.getElementById('prop-content');
var contentSelect = document.getElementById('prop-content-select');
if (contentField) {
  /* Determine whether to use a select from the API metadata */
  var contentFieldMeta = null;
  if (typeMeta && typeMeta.fields) {
    for (var fi = 0; fi < typeMeta.fields.length; fi++) {
      if (typeMeta.fields[fi].key === 'content') {
        contentFieldMeta = typeMeta.fields[fi];
        break;
      }
    }
  }
  var useSelect = (contentFieldMeta && contentFieldMeta.kind === 'select');
  if (contentInput)  contentInput.style.display  = useSelect ? 'none' : '';
  if (contentSelect) contentSelect.style.display = useSelect ? '' : 'none';
  if (useSelect && contentSelect) {
    /* Rebuild options from API metadata (works for clock, flight_field, metric) */
    if (contentFieldMeta && contentFieldMeta.options && contentFieldMeta.options.length > 0) {
      contentSelect.innerHTML = '';
      for (var oi = 0; oi < contentFieldMeta.options.length; oi++) {
        var opt = document.createElement('option');
        opt.value = contentFieldMeta.options[oi];
        opt.textContent = contentFieldMeta.options[oi];
        contentSelect.appendChild(opt);
      }
    }
    contentSelect.value = w.content || (contentFieldMeta && contentFieldMeta['default']) || '';
  } else {
    if (contentInput) contentInput.value = w.content || '';
  }
}
```

**Key behavioral changes:**
- `useSelect` is now driven by the API metadata `kind === 'select'`, not by hardcoded `w.type === 'clock'`
- The options rebuild now uses `contentFieldMeta.options[]` from the API instead of hardcoded `['12h', '24h']`
- Clock still works identically (API returns `kind: "select"` with options `["12h", "24h"]`)
- `flight_field` and `metric` now show a dropdown instead of a text input

**`widgetTypeMeta` is already populated** by the editor's `initWidgetTypes()` call which GETs `/api/widgets/types` on page load and stores each entry keyed by `entry.type`. After Task 9 updates the WebPortal endpoint, `widgetTypeMeta['flight_field'].fields[0]` will have `kind: "select"` and `options: ["ident_icao", ...]` — the editor code above just reads them.

---

### Unit Test Scaffold

**Directory structure to create:**

```
firmware/test/test_flight_field_widget/test_main.cpp
firmware/test/test_metric_widget/test_main.cpp
```

**`platformio.ini` test filter pattern** (existing `[env:esp32dev]` already declares `test_filter = test_*` — no change needed; new test dirs are auto-discovered).

**Minimal FlightInfo helper for tests:**

```cpp
#include "models/FlightInfo.h"

static FlightInfo makeFlightInfo() {
    FlightInfo f;
    f.ident_icao = "UAL123";
    f.ident_iata = "UA123";
    f.ident = "UAL123";
    f.operator_icao = "UAL";
    f.operator_iata = "UA";
    f.origin.code_icao = "KORD";
    f.destination.code_icao = "KSFO";
    f.aircraft_code = "B739";
    f.airline_display_name_full = "United Airlines";
    f.aircraft_display_name_short = "Boeing 737-900";
    f.altitude_kft = 35.0;
    f.speed_mph = 480.0;
    f.track_deg = 270.0;
    f.vertical_rate_fps = 0.0;
    return f;
}
```

**Heap-delta test pattern (on-device only):**

```cpp
void test_heap_stable_1000_iters() {
    FlightInfo flight = makeFlightInfo();
    RenderContext ctx{};
    ctx.matrix = nullptr;
    ctx.currentFlight = &flight;

    WidgetSpec spec{};
    spec.type = WidgetType::FlightField;
    spec.x = 0; spec.y = 0; spec.w = 80; spec.h = 8;
    spec.color = 0xFFFF;
    strncpy(spec.content, "altitude_kft", sizeof(spec.content) - 1);

    uint32_t heapBefore = ESP.getFreeHeap();
    for (int i = 0; i < 1000; i++) {
        WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx);
    }
    uint32_t heapAfter = ESP.getFreeHeap();
    // Allow up to 64 bytes noise (RTOS accounting variance).
    TEST_ASSERT_UINT32_WITHIN(64, heapBefore, heapAfter);
}
```

**RenderContext construction in tests:** After Task 1 adds `currentFlight`, `RenderContext ctx{}` zero-initializes the pointer to `nullptr`. Tests that want a valid flight set `ctx.currentFlight = &someFlightInfo`.

---

### Zero-Heap Rule on the Render Hot Path

From BMAD Architecture Rule 20: "Render functions must not allocate heap (no `new`, no `std::string` construction, no `String` objects on the hot path)."

**Allowed:**
- Stack buffers: `char out[48]`
- File-scope static buffers: `static char s_ffBuf[48]`
- Calling `DisplayUtils` char* overloads (they write into caller-provided buffers)
- Comparing `flight->ident_icao.c_str()` (reading a `String`'s internal buffer, not constructing a new `String`)

**Not allowed:**
- `String s = flight->ident_icao + " (" + flight->operator_icao + ")";` — constructs two temp `String` objects on heap
- `snprintf(new char[48], ...)` — heap allocation
- `std::vector`, `std::map`, `std::string` in render path

**`FlightInfo` fields are Arduino `String` objects** (heap-allocated). You may read them via `.c_str()` for `strcmp`/`strcpy` — this does NOT copy the heap buffer, it just returns a pointer to the existing allocation. This is safe and allocation-free on the read path.

---

### Antipattern Prevention Table

| DO NOT | DO INSTEAD | Reason |
|---|---|---|
| Use `String` temporaries in render fn | Use `.c_str()` + `strncpy` into stack/static buffers | String ctor/dtor = heap alloc per frame |
| Dereference `ctx.currentFlight` without nullptr check | Always check `if (ctx.currentFlight == nullptr)` first | Other modes and tests pass `nullptr` |
| Return `false` on unknown `content` value | Write `"--"` to buf and return `true` | `false` from dispatch is "hard error"; unknown field is just a graceful no-op |
| Add `ApiContext` or weather data in this story | Mark as TODO and file a deferred-work note | Scope: MVP is flight data only |
| Change `WidgetType` enum values | Never change existing enum integer values | They may be serialized/persisted |
| Change `fromString()` | No changes needed — already handles `"flight_field"` and `"metric"` | Already correct from LE-1.2 |
| Change `kAllowedWidgetTypes[]` | No changes needed | Already includes both types from LE-1.1 |
| Call `flights[i]` without bounds check | Always check `flights.empty()` before indexing | `flights` can be empty if no API data yet |
| Leave `(void)flights;` in CustomLayoutMode | Replace with RenderContext copy + `currentFlight` assignment | This is the entire purpose of this story |
| Change `DisplayMode` virtual interface | Only add field to `RenderContext` struct | Changing the virtual `render(...)` signature would break all mode implementations |
| Use `let`/`const`/`=>` in `editor.js` | `var`/`function` only | ES5 constraint |
| Re-populate `<select>` options on every `showPropsPanel` call | Use `contentSelect.innerHTML = ''` + rebuild | Avoids duplicate options accumulating on repeated widget selections |
| Forget to regenerate `.gz` after editor.js edit | `gzip -9 -c data-src/editor.js > data/editor.js.gz` | Device serves only `.gz` files |

---

### Relationship to Existing Widget Tests

`test_widget_registry/test_main.cpp` currently has two tests that **assert `false`** for `FlightField` and `Metric` dispatch (they are "stub" tests from LE-1.2):

```cpp
void test_dispatch_flight_field_not_yet_implemented() {
    // ...
    TEST_ASSERT_FALSE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
}
void test_dispatch_metric_not_yet_implemented() {
    // ...
    TEST_ASSERT_FALSE(WidgetRegistry::dispatch(WidgetType::Metric, spec, ctx));
}
```

**These tests will FAIL after Task 5** (wiring the real render functions). Task 6 must update them to `TEST_ASSERT_TRUE` **before** running `pio test`. Do not skip this — failing tests are a hard gate.

**Note on `makeCtx()` in test_widget_registry:** After Task 1 adds `currentFlight`, the zero-init `RenderContext ctx{}` in `makeCtx()` will have `currentFlight = nullptr`. The updated FlightField and Metric tests will exercise the nullptr-flight fallback path, which is correct for the registry-level dispatch test (field-specific tests live in the new test suites).

---

### `DisplayUtils::formatTelemetryValue` — char* Overload Reference

```cpp
// Header (utils/DisplayUtils.h):
void formatTelemetryValue(double value, const char* suffix, int decimals,
                           char* out, size_t outLen);
```

Behavior:
- If `value` is `NAN`: writes `"--"` into `out`
- Otherwise: `snprintf(out, outLen, "%.*f%s", decimals, value, suffix)`
- Safe: always null-terminates `out`

**Usage for altitude:**
```cpp
DisplayUtils::formatTelemetryValue(flight->altitude_kft, "k", 1, s_ffBuf, sizeof(s_ffBuf));
// flight->altitude_kft = 35.2 → s_ffBuf = "35.2k"
// flight->altitude_kft = NAN  → s_ffBuf = "--"
```

---

### LittleFS Budget Impact

LE-1.8 adds no new LittleFS-stored web assets (only regenerates `editor.js.gz` which is already in the partition). The two new `.cpp` files for FlightFieldWidget and MetricWidget are firmware source only.

Estimated flash delta: ~2–4 KB of compiled code for both widgets + test stubs. Well within the 88% budget (current baseline from LE-1.7: 82.4%).

---

### Sprint Status Update

After all tasks pass, update `_bmad-output/implementation-artifacts/sprint-status.yaml`:
```yaml
le-1-8-flight-field-and-metric-widgets: done
```

---

## File List

Files **created** (new):
- `firmware/widgets/FlightFieldWidget.h` — render function declaration
- `firmware/widgets/FlightFieldWidget.cpp` — render function implementation
- `firmware/widgets/MetricWidget.h` — render function declaration
- `firmware/widgets/MetricWidget.cpp` — render function implementation
- `firmware/test/test_flight_field_widget/test_main.cpp` — Unity test suite
- `firmware/test/test_metric_widget/test_main.cpp` — Unity test suite

Files **modified**:
- `firmware/interfaces/DisplayMode.h` — add `const FlightInfo* currentFlight` to `RenderContext`
- `firmware/core/WidgetRegistry.cpp` — wire `renderFlightField()` and `renderMetric()` into `dispatch()` switch; add two new includes
- `firmware/modes/CustomLayoutMode.cpp` — replace `(void)flights;` with `RenderContext widgetCtx = ctx; widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];`; dispatch with `widgetCtx` instead of `ctx`
- `firmware/adapters/WebPortal.cpp` — `_handleGetWidgetTypes()`: replace `"kind": "string"` content field with `"kind": "select"` + options for `flight_field` and `metric`; remove `"note"` keys
- `firmware/data-src/editor.js` — `showPropsPanel()`: replace hardcoded `clock`-only `useSelect` logic with metadata-driven `kind === 'select'` check; generalize option rebuild
- `firmware/data/editor.js.gz` — regenerated after `editor.js` edit
- `firmware/test/test_widget_registry/test_main.cpp` — update two "not yet implemented" tests to `TEST_ASSERT_TRUE`

Files **NOT modified** (verify unchanged):
- `firmware/core/WidgetRegistry.h` — enum and struct already correct; no changes
- `firmware/core/LayoutStore.cpp` — `kAllowedWidgetTypes[]` already contains both types; no changes
- `firmware/core/LayoutStore.h` — no changes
- `firmware/data-src/editor.html` — HTML structure unchanged; `<select id="prop-content-select">` already exists from LE-1.7
- `firmware/data-src/editor.css` — no new styles needed
- `firmware/data-src/common.js` — no changes
- All other display modes (`ClassicCardMode`, `LiveFlightCardMode`, `ClockMode`, etc.) — `RenderContext{}` zero-init sets `currentFlight = nullptr` automatically; these modes never read `currentFlight`

---

## Change Log

| Date | Version | Description | Author |
|---|---|---|---|
| (original draft) | 0.1 | Draft story created — thin skeleton with placeholder dev notes | BMAD |
| 2026-04-17 | 1.0 | Upgraded to ready-for-dev. Comprehensive dev notes synthesized from live codebase analysis. Key additions: (1) Identified the `RenderContext.currentFlight` gap as the core architectural challenge — Task 1 and Task 2 added explicitly; (2) Documented exact `DisplayMode.h` struct change and `CustomLayoutMode.cpp` `(void)flights` fix; (3) Full FlightFieldWidget implementation skeleton with caching strategy for both string and numeric fields; (4) MetricWidget scoped to two telemetry fields in MVP with explicit note deferring `ApiContext` to a future story; (5) Exact `_handleGetWidgetTypes()` replacement code for both `flight_field` and `metric` blocks including `static const char* const` option arrays; (6) `editor.js` `showPropsPanel()` before/after diff — generalizing from clock-only to metadata-driven `kind === "select"` check; (7) AC #11 added for updating existing "not yet implemented" test assertions to `true`; (8) Unit test scaffold with `makeFlightInfo()` helper and heap-delta test pattern; (9) Zero-heap rule enforcement table (allowed vs. not-allowed patterns with `String.c_str()` safe-read clarification); (10) Antipattern prevention table (14 entries); (11) File list with explicit "NOT modified" section. Status changed from `draft` to `ready-for-dev`. | BMAD Story Synthesis |


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

## Story le-1-7 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Smoke test class alphabetical ordering causes `TestEditorActivate` (now `TestEditorDActivate`) to run before `TestEditorLayoutCRUD` (now `TestEditorCLayoutCRUD`) — `_created_layout_id` is `None` when activate test runs, causing it to silently `skipTest()`, so the activate step never fires before `TestEditorModeSwitch` asserts the mode | Renamed `TestEditorLayoutCRUD` → `TestEditorCLayoutCRUD` and `TestEditorActivate` → `TestEditorDActivate` so alphabetical execution order is: Asset → CRUD → DActivate → ModeSwitch. Also removed dead `if not _created_layout_id: pass` branch from `TestEditorModeSwitch`. |
| medium | AC#4 requires name ≤ 32 characters JS-side validation before save. HTML `maxlength="32"` prevents normal input but a paste-attack or devtools bypass would reach the server with a misleading "Enter a layout name" error instead of a length-specific message | Added explicit `if (name.length > 32)` guard with targeted error toast in `saveLayout()`. Also regenerated `firmware/data/editor.js.gz` (5,811 bytes; total 8,078 / 30,720 = 26.3% budget). |
| low | `FW.showToast('Applying…', 'success')` uses `'success'` severity for an in-progress state — operator sees a green "success" toast while the activate may still fail | Changed severity from `'success'` to `'info'` for the intermediate Applying toast. |
| dismissed | "Task 4 — showPropsPanel function missing" | FALSE POSITIVE: `showPropsPanel(w)` is fully implemented at lines 362–414 of `editor.js`. Complete with clock/text branching, align display logic, and position/size readouts. The reviewer's evidence ("only references: Line 395") referenced a call site, not searching for the definition. False positive verified by direct code read. |
| dismissed | "Task 5 — position/size readouts not updated during drag" | FALSE POSITIVE: Lines 206–210 of `onPointerMove` in `editor.js` explicitly query and update both `prop-pos-readout` and `prop-size-readout` after every pointer move event. Already implemented. |
| dismissed | "AC#9 FW.put integration unverified" | FALSE POSITIVE: `FW.put` is fully present in `common.js` at lines 47–53 and included in the return object at line 87. The function mirrors `FW.post` using `method: 'PUT'` as required. No integration test is required beyond what the smoke suite covers. |
| dismissed | "Test file TestEditorModeSwitch passes even if activate never ran (lying pass)" | FALSE POSITIVE: Addressed by the class rename fix — with correct ordering, `TestEditorDActivate` now fires before `TestEditorModeSwitch`. The assertion is no longer potentially stale. |
| dismissed | "AC1 not driven by fields array — hard-coded type checks" | FALSE POSITIVE: Story Dev Notes explicitly specify V1 behavior: "The property panel deliberately exposes only: Content, Color, Align" and the align → L/C/R button group is "intentionally" not a `<select>`. Clock → select is a fixed V1 design. The `widgetTypeMeta` built from the API response is used for defaults and min-floors; the panel control type is intentionally fixed. This is by-design, not a defect. |
| dismissed | "saveLayout promise chain handles undefined/string return incorrectly" | FALSE POSITIVE: `parseJsonResponse` in `common.js` always returns a `{status, body}` envelope — even on empty responses or parse errors it synthesizes a proper envelope with `ok: false`. It never returns `undefined` or a raw string. The `.catch()` at the end handles actual fetch rejections (network down). |
| dismissed | "Race condition in LayoutStore::save — concurrent client reads" | FALSE POSITIVE: LE-1.7 is a web-only story. No firmware C++ changes were made. Concurrent file access in `LayoutStore` is a firmware concern outside this story's scope. Project context explicitly states firmware files are not a LE-1.7 concern. |
| dismissed | "JSON.stringify performance footgun on large widgets during pointermove" | FALSE POSITIVE: `JSON.stringify` is only called inside `saveLayout()` and `getLayoutBody()` — both are user-triggered on button click, not on `pointermove`. Factually incorrect claim. |
| dismissed | "SOLID violation — WebPortal mixes routing and orchestration" | FALSE POSITIVE: Pre-existing architecture documented in project-context.md as the intentional hexagonal adapter pattern. Not introduced by LE-1.7 (web-only story, no WebPortal changes). |
| dismissed | "Dirty flag race condition in saveAsNew async flow" | FALSE POSITIVE: JavaScript's event loop is single-threaded. No event can fire between `currentLayoutId = null` and the Promise chain executing. `saveLayout()` executes synchronously up to the first `fetch()` call, at which point the `currentLayoutId = null` state is fully set before any async resolution occurs. |
| dismissed | "Layout ID uniqueness not guaranteed — two similar names produce same ID" | FALSE POSITIVE: Story Dev Notes explicitly document that `LayoutStore::save()` performs an upsert — a same-id POST overwrites the existing layout. This is documented behavior, not a bug. For V1 single-user appliance use case, this is acceptable. A `Date.now()` suffix would increase the ID length and reduce human-readability; rejected per story antipattern table which defers complexity. |
| dismissed | "RenderContext isolation violated — editor builds widgets without validation" | FALSE POSITIVE: The firmware's `LayoutStore::save()` + `CustomLayoutMode::init()` perform server-side validation at layout-load time. `addWidget()` uses `meta.minW`/`meta.minH` from the API response to enforce floors. The `w=6, h=6` concern is incorrect — `minH = meta.defaultH` which is 8 for text (matching firmware floor). Dual-layer validation is the correct architecture for a web editor. |
| dismissed | "Missing error boundary for layout load failure when malformed nested structure" | FALSE POSITIVE: Lines 568–574 of `editor.js` contain an explicit nested-field guard checking `d.matrix`, `d.hardware`, and numeric types of all three fields, showing a specific error toast before returning null. Already implemented. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**ADVERSARIAL CODE REVIEW COMPLETE, Christian**

**Story:** `_bmad-output/implementation-artifacts/stories/le-1-8-flight-field-and-metric-widgets.md`  
**Scope:** Story acceptance criteria vs embedded implementation (`WebPortal.cpp`, `FlightFieldWidget.*`, `MetricWidget.*`, `CustomLayoutMode.cpp`, `DisplayMode.h`, `editor.js`, `WidgetRegistry.cpp`) — read-only; no build or git commands executed in this pass.

---

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 (not reconciled here; working tree shows many LE-1 files untracked/modified — story “File List” cannot be assumed complete vs `git status`) |
| AC Implementation Gaps | 6 (AC #1–#6 materially diverge or are unimplemented as written) |
| Task Completion Lies | 0 (story tasks are `[ ]`, status `ready-for-dev` — no false “done” claims in the story text itself) |
| SOLID / Boundary | 2 (schema triple-source debt; comment truth vs runtime) |
| Hidden Bugs / Safety | 2 (buffer/`strcpy` discipline; `String` lifetime assumptions) |
| Tests | 2 (likely weak or misaligned with story AC if they follow shipped keys; heap delta noise) |
| Security | 0 (LAN appliance; no new auth surface in this slice) |
| **Total Issues** | **10** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| CRITICAL | Story AC #1 / #3 / #6 require full flight-field set + `formatTelemetryValue` for telemetry + API option list parity; `FlightFieldWidget` only resolves five string-like keys (`airline`, `ident`, `origin_icao`, `dest_icao`, `aircraft`) and never touches `altitude_kft` / `speed_mph` / `track_deg` / `vertical_rate_fps` | `firmware/widgets/FlightFieldWidget.cpp`, `firmware/adapters/WebPortal.cpp` | +3.0 |
| IMPORTANT | AC #2 requires UTF-8 em dash for `nullptr` flight; renderer substitutes ASCII `"--"` for null/unknown/empty | `firmware/widgets/FlightFieldWidget.cpp` | +1.0 |
| IMPORTANT | AC #4 limits `MetricWidget` to `track_deg` + `vertical_rate_fps` with explicit unit formatting; implementation exposes four keys (`alt`, `speed`, `track`, `vsi`) and different suffix rules (e.g. `"mph"` / `"fps"` concatenation vs story’s spaced / degree UTF-8 contract) | `firmware/widgets/MetricWidget.cpp`, story AC #4 | +1.0 |
| IMPORTANT | AC #5 (file-scope `char[48]` cache + value-equality refresh) not implemented in either widget — every frame re-resolves and reformats | `firmware/widgets/FlightFieldWidget.cpp`, `firmware/widgets/MetricWidget.cpp` | +1.0 |
| IMPORTANT | “Three-list sync” is worse than drift: WebPortal advertises keys the firmware renderer does not implement per story (and story advertises yet another set) — editor will author layouts that silently render `"--"` on device | `firmware/adapters/WebPortal.cpp`, story §AC #1 / #6 | +1.0 |
| MINOR | `MetricWidget` uses `strcpy` into a 24-byte stack buffer after `formatTelemetryValue` — safe today only by informal length budget; no `strlcpy` / length assert | `firmware/widgets/MetricWidget.cpp` | +0.3 |
| MINOR | `CustomLayoutMode.cpp` narrative about `flights[0]` / “ModeOrchestrator cycles” may be misleading if the producer does not actually rotate element 0; risk of wrong flight bound to widgets without a code-side invariant | `firmware/modes/CustomLayoutMode.cpp` | +0.3 |
| CLEAN PASS | No meaningful new security exposure in these endpoints (read-only metadata + same-origin editor) | — | −0.5 |
| CLEAN PASS | No obvious new blocking I/O on widget render path beyond existing GFX draw (telemetry omission is correctness, not perf) | — | −0.5 |
| CLEAN PASS | No Python in scope | — | −0.5 |

### Evidence Score: **5.6**

| Score | Verdict |
|-------|---------|
| **5.6** | **MAJOR REWORK** (story AC and shipped API/renderer contracts are not aligned; ship-blocking if LE-1.8 is gated on the written ACs) |

---

## Architectural Sins

- **[7/10] Open/Closed + single source of truth failure:** Widget field vocabularies are still triple-sourced (`LayoutStore` allowlist type strings, `WidgetRegistry::fromString`, `WebPortal::_handleGetWidgetTypes`) and now a **fourth** implicit source: the `strcmp` key sets inside `FlightFieldWidget` / `MetricWidget`. Any one edit desynchronizes editor defaults, save validation, and renderers.  
  - **Fix:** Generate options from one table shared by WebPortal + widgets, or drive options from firmware metadata that is literally what `resolveField` consumes.

- **[6/10] Boundary / contract leak:** The story frames LE-1.8 as binding **canonical** `FlightInfo` fields (`ident_icao`, … `vertical_rate_fps`). The implementation invents abbreviated editor keys (`ident`, `alt`, …). That is not a rename — it is a **different persisted language** in saved layouts unless you also migrate JSON.

---

## Pythonic Crimes & Readability

- N/A (C++/ES5).  
- **ES5 note:** `editor.js` uses `buildWidgetTypeMeta` indirection (`contentKind` / `contentOptions`) rather than the story’s “scan `fields[]` for `key === 'content'`” snippet — acceptable if equivalent, but it must stay aligned with whatever JSON `_handleGetWidgetTypes` emits (`kind` vs `contentKind` mismatch would silently break dropdowns).

---

## Performance & Scalability

- **\[Low\]** Missing AC #5 caches: extra `snprintf` / string walks per widget per frame — probably fine at 20 fps for one line, but it violates the story’s explicit heap-stability contract and makes the “1000-iteration heap delta == 0” test less meaningful if other subsystems allocate during test.

---

## Correctness & Safety

- **Bug / spec miss:** `FlightFieldWidget` cannot satisfy AC #1 / #3 as written: no numeric branch, no `NAN` path through `DisplayUtils::formatTelemetryValue` for flight fields.  
- **Bug / spec miss:** AC #2 em dash behavior not met (`"--"` substitution).  
- **Buffer discipline:** `strcpy(buf, "--")` and `strcpy` after `formatTelemetryValue` in `MetricWidget.cpp` — tighten with bounded copy or static assert on max formatted width.  
- **Lying-test risk:** If Unity tests were written against the **story** keys (`ident_icao`, …) while firmware implements **`ident` / `alt`**, tests may “pass” while AC fails — or vice versa. Tests must assert the **product contract** (pick story vs implementation and enforce one).

---

## Maintainability Issues

- **Tech debt bomb:** Three competing vocabularies (story AC, WebPortal JSON, widget `strcmp` tables) will cause support tickets (“I picked `ident_icao` in the API docs but editor shows `ident`”) and complicates LayoutStore migration if you ever normalize saved layouts.

---

## Suggested Fixes (priority order)

1. **Pick one canonical `content` vocabulary** and make `WebPortal`, `FlightFieldWidget`, `MetricWidget`, and `LayoutStore` validation agree (prefer story snake_case field names if LE-1.8 is the contract).  
2. **Implement `FlightField` numeric fields** with `DisplayUtils::formatTelemetryValue(...)` and suffix/decimals per story table; unknown keys → `"--"` or em dash per finalized UX decision.  
3. **Implement AC #5 caches** (static buffers + last-value snapshots) or revise the story AC if caching is deferred — do not leave AC and code disagreeing.  
4. **Fix AC #2** literally (UTF-8 em dash) or formally change AC to `"--"` everywhere (including tests and editor preview if any).  
5. **Rewrite `CustomLayoutMode` comment** to cite the actual invariant (who rotates `flights`, ordering guarantees), or pass an explicit `currentFlight` index from the mode orchestration layer.

---

## Review Actions

- Issues Found: **10**  
- Issues Fixed: **0** (read-only review)  
- Action Items Created: **0** (orchestrator may file follow-ups)

**Bottom line:** As written, LE-1.8’s acceptance criteria are **not satisfied** by the embedded implementation; the gap is larger than naming nits (telemetry flight fields are absent; metric widget scope/format diverges; caching absent; API options diverge from story). Treat as **MAJOR REWORK** until AC/story or code converge with an explicit migration plan for any already-saved layouts.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/le-1-8-flight-field-and-metric-widgets.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **4** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task Completion Lie: AC #11 tests update not performed. | `firmware/test/test_widget_registry/test_main.cpp` | +3 |
| 🟠 IMPORTANT | AC #6 incomplete: `WebPortal` does not remove `"note"` field. | `firmware/adapters/WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | Hidden Bug: `MetricWidget` uses `formatTelemetryValue` with 0-decimals for `vsi` in some code paths (violates AC #4). | `firmware/widgets/MetricWidget.cpp` | +1 |
| 🟠 IMPORTANT | Hidden Bug: `FlightFieldWidget` cache relies on `strcmp` for `content` (a pointer), but `spec.content` can point to different addresses with the same string content across runs. | `firmware/widgets/FlightFieldWidget.cpp` | +1 |

### Evidence Score: 6.0

| Score | Verdict |
|-------|---------|
| **6.0** | **MAJOR REWORK** |

---

## 🐛 Correctness & Safety

- **🐛 Bug (Incomplete AC #6):** The `WebPortal` implementation for `_handleGetWidgetTypes()` failed to remove the `"note"` field from both the `flight_field` and `metric` blocks as required by AC #6. This violates the story requirements explicitly.
  - 📍 `firmware/adapters/WebPortal.cpp:2160, 2172`

- **🎭 Lying Test (AC #11):** The tests `test_dispatch_flight_field_not_yet_implemented` and `test_dispatch_metric_not_yet_implemented` in `firmware/test/test_widget_registry/test_main.cpp` were not updated to `TEST_ASSERT_TRUE`. They still exist as stubs and have NOT been renamed/updated per AC #11. This is a critical task completion lie.
  - 📍 `firmware/test/test_widget_registry/test_main.cpp`

- **🐛 Bug (MetricWidget):** AC #4 explicitly requires `vertical_rate_fps` formatted with 1 decimal. However, `resolveMetric` maps `vsi` (the metric key) but the logic implementation uses `formatTelemetryValue` with `0` decimals if the key is mismanaged or the formatting logic is genericized inappropriately, risking visual corruption of vertical rate data.
  - 📍 `firmware/widgets/MetricWidget.cpp:48`

- **🐛 Bug (FlightFieldWidget):** The cache check `strcmp(content, s_ffLastContent)` correctly compares string *content*, but `FlightFieldWidget` uses `content` as a pointer from `WidgetSpec`. If `WidgetRegistry` is re-initialized (e.g., during a layout reload) and `spec` points to a new location in the `WidgetSpec[]` array, `strcmp` is necessary, but the code relies on `s_ffLastContent` which is static. The caching strategy is flawed if multiple `FlightFieldWidget` instances (with different specs) are ever dispatched (though the system currently allows only one).
  - 📍 `firmware/widgets/FlightFieldWidget.cpp`

---

## 🛠️ Suggested Fixes

### 1. Fix AC #11 (Task Completion Lie)
**File:** `firmware/test/test_widget_registry/test_main.cpp`
**Issue:** Tests in `WidgetRegistry` still assert false for newly implemented widgets.

```diff
--- a/firmware/test/test_widget_registry/test_main.cpp
+++ b/firmware/test/test_widget_registry/test_main.cpp
-void test_dispatch_flight_field_not_yet_implemented() {
+void test_dispatch_flight_field_returns_true() {
     // ...
-    TEST_ASSERT_FALSE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
+    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
 }
-void test_dispatch_metric_not_yet_implemented() {
+void test_dispatch_metric_returns_true() {
     // ...
-    TEST_ASSERT_FALSE(WidgetRegistry::dispatch(WidgetType::Metric, spec, ctx));
+    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Metric, spec, ctx));
 }
```

### 2. Remove "note" fields (AC #6)
**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** `WebPortal` failed to strip the `"note"` field from API responses.

```diff
--- a/firmware/adapters/WebPortal.cpp
+++ b/firmware/adapters/WebPortal.cpp
-    obj["note"] = "LE-1.8 — not yet implemented, renders nothing";
```

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW REPORT**

**Story:** le-1.8-flight-field-and-metric-widgets
**Target:** Implement FlightFieldWidget and MetricWidget with zero-heap rendering

---

## 📊 Review Summary

| Category | Issues Found | Severity |
|----------|--------------|----------|
| AC Implementation Gaps | 4 | HIGH |
| Task Completion Gaps | 3 | HIGH |
| Code Quality Issues | 5 | MEDIUM |
| Missing Test Coverage | 2 | HIGH |
| Documentation Errors | 3 | MEDIUM |
| **Total Issues** | **17** | — |

---

## 🔴 CRITICAL FINDINGS

### 1. **AC #11 NOT ACTUALLY FIXED - Task 6 Incomplete** 
**Severity: 🔴 CRITICAL**

The story claims Task 6 is complete (`[x]` marked), but two tests in `firmware/test/test_widget_registry/test_main.cpp` are still declared as "NOT YET IMPLEMENTED" stubs:

```cpp
// Line evidence from context:
// "test_dispatch_flight_field_not_yet_implemented()" 
// "test_dispatch_metric_not_yet_implemented()"
// Both assert FALSE
```

**AC #11 explicitly requires:**
> "Both tests **must be updated** to assert `true` (since both widgets are now implemented) — or renamed/replaced with equivalent positive-dispatch tests. **Do not leave these asserting `false` after implementation.**"

**Finding:** The story is INCOMPLETE. Two acceptance criteria validations remain as stub tests that prove nothing.

**Impact:** Automated test suite will fail. The two widget types are claimed "implemented" but the registry test suite still marks them unimplemented.

**Fix:** Update both test functions to:
```cpp
void test_dispatch_flight_field_returns_true() {
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::FlightField, 0, 0, 40, 8, "ident_icao");
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
}

void test_dispatch_metric_returns_true() {
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Metric, 0, 0, 40, 8, "track_deg");
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Metric, spec, ctx));
}
```

---

### 2. **DisplayMode Interface Not Updated - AC #1 Breaks All Modes**
**Severity: 🔴 CRITICAL**

**Story Task 1 claims:** "Add `const FlightInfo* currentFlight` field to `RenderContext` struct"

**Finding:** The `DisplayMode.h` file in the context shows the struct DOES have `currentFlight` added (line visible in interface definition). However, **EVERY EXISTING MODE** that doesn't explicitly set this field will have **uninitialized garbage pointer values** on the render hot path.

**Evidence from Story File:**
> "RenderContext ctx{};" zero-initializes to `nullptr` by default

**Reality Check:** When existing modes (ClassicCardMode, LiveFlightCardMode, ClockMode) are called with an RenderContext that is NOT zero-initialized (e.g., allocated on stack somewhere), `currentFlight` will contain uninitialized memory.

**Impact:** CRITICAL - Any code path that constructs RenderContext{} manually without explicit `= nullptr` for currentFlight creates a dangling pointer waiting for a segfault.

**Required Fix:** Document in RenderContext struct definition:
```cpp
struct RenderContext {
    // ... existing fields ...
    const FlightInfo* currentFlight = nullptr;  // LE-1.8: MUST be explicitly initialized
};
```

---

### 3. **CustomLayoutMode Never Wired to Use currentFlight - AC #1 False**
**Severity: 🔴 CRITICAL**

**Story Task 2 claims:** "Wire `currentFlight` in `CustomLayoutMode::render()`"

**Task instructions show exact replacement code:**
```cpp
RenderContext widgetCtx = ctx;
widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];
// ... use widgetCtx instead of ctx for dispatch
```

**Finding:** The provided `CustomLayoutMode.cpp` in the context file shows the proper implementation IS present (lines visible). However, the story File List says **NO changes to CustomLayoutMode.cpp should be needed** because "modes never read currentFlight directly."

**This is a contradiction:** Either:
1. CustomLayoutMode DOES need modification to wire currentFlight (contradicts story File List)
2. Modes don't use currentFlight (contradicts AC #1 requirement)

**Impact:** If CustomLayoutMode.cpp wasn't actually modified, flights won't render with field data. If it was modified, the story's File List is incomplete.

**Required Action:** Confirm CustomLayoutMode.cpp was actually modified to inject currentFlight into the RenderContext before dispatch.

---

## 🟠 HIGH SEVERITY ISSUES

### 4. **Zero-Heap Validation Never Tested - AC #5 Uncertified**
**Severity: 🟠 HIGH**

**AC #5 requires:**
> "No heap allocation occurs on the render hot path... heap-delta == 0"

**Story Task 7 specifies test:**
```cpp
TEST_ASSERT_UINT32_WITHIN(64, heapBefore, heapAfter);
```

**Finding:** The test scaffold provided in Dev Notes shows the pattern, but NO ACTUAL TEST IMPLEMENTATION is shown in the context. The story references `firmware/test/test_flight_field_widget/test_main.cpp` (Task 7) but this file is NOT provided in the context for review.

**Evidence Gap:** Cannot verify:
1. The test file exists
2. The test actually runs
3. The 1000-iteration heap-delta assertion passes
4. The file-scope static buffers are actually present and working

**Impact:** Cannot confirm the "zero-heap" guarantee is real. This is a non-functional requirement that has NO PROOF.

**Required:** Provide test file content or confirm it exists and passes locally.

---

### 5. **WebPortal Widget Options Not Fully Updated - AC #6 Incomplete**
**Severity: 🟠 HIGH**

**Story Task 9 claims:** Update `_handleGetWidgetTypes()` to replace stub flight_field and metric blocks

**Actual WebPortal.cpp context shows (line ~2150+):**
```cpp
// ── flight_field ─────────────────────────────────────────────────────
{
    JsonObject obj = data.add<JsonObject>();
    obj["type"] = "flight_field";
    obj["label"] = "Flight Field";
    JsonArray fields = obj["fields"].to<JsonArray>();
    static const char* const flightFieldOptions[] = {
        "airline", "ident", "origin_icao", "dest_icao", "aircraft"
    };
    addField(fields, "content", "select", "airline", 0, false,
             flightFieldOptions, 5);
```

**BUT AC #6 specifies supported fields are:**
```
"ident_icao", "operator_icao", "airline", "aircraft", "origin", "destination",
"altitude_kft", "speed_mph", "track_deg", "vertical_rate_fps"
```

**Finding:** The API options array only has **5 fields** but AC #1 in FlightFieldWidget story specifies **10 supported fields**. The WebPortal is missing:
- "ident_icao"
- "operator_icao" 
- "origin" (only has "origin_icao")
- "destination" (only has "dest_icao")
- ALL 5 telemetry fields (altitude_kft, speed_mph, track_deg, vertical_rate_fps)

**Impact:** Editors cannot select most fields because they're not in the API dropdown options. AC #6 claim "content field with options listing all supported field names (same set as AC #1)" is FALSE.

**Required Fix:** Expand flightFieldOptions array to match AC #1:
```cpp
static const char* const flightFieldOptions[] = {
    "ident_icao", "operator_icao", "airline", "aircraft",
    "origin", "destination",
    "altitude_kft", "speed_mph", "track_deg", "vertical_rate_fps"
};
```

---

### 6. **MetricWidget Options Incomplete - AC #6 False Again**
**Severity: 🟠 HIGH**

**Story AC #4 specifies supported metrics:**
```
"track_deg", "vertical_rate_fps"
```

**WebPortal.cpp shows:**
```cpp
static const char* const metricOptions[] = {
    "track_deg", "vertical_rate_fps"
};
```

**BUT editor.js shows (line ~2310):**
```javascript
static const char* const metricOptions[] = {
    "alt", "speed", "track", "vsi"
};
```

**Finding:** There's a MISMATCH. The WebPortal serves 2 metric names but editor.js shows 4. The AC specifies 2 fields but the abbreviations are DIFFERENT (story says "track_deg"/"vertical_rate_fps", but dashboard shows "track"/"vsi").

**Evidence:** Neither source matches the other. No canonical list exists.

**Impact:** Editor and firmware disagree on metric keys. Saving layouts with "alt" from the editor will break at render time because MetricWidget's resolveMetric() only handles "altitude_kft" not "alt".

**Required:** Standardize metric names across all three sources:
1. AC specification
2. WebPortal API options
3. editor.js dropdown
4. MetricWidget.cpp resolveMetric()

---

## 🟡 MEDIUM SEVERITY ISSUES

### 7. **FlightFieldWidget resolveField() Is Incomplete vs AC #1**
**Severity: 🟡 MEDIUM**

**AC #1 specifies 10 supported field keys:**
```
"ident_icao", "operator_icao", "airline", "aircraft", "origin", "destination",
"altitude_kft", "speed_mph", "track_deg", "vertical_rate_fps"
```

**FlightFieldWidget.cpp provided resolveField() function handles:**
- "airline"
- "ident" (with fallback chain)
- "origin_icao"
- "dest_icao" 
- "aircraft"

**Missing implementations:**
- "ident_icao" (implemented as "ident"? - unclear)
- "operator_icao" (NOT PRESENT)
- "origin" (only has "origin_icao")
- "destination" (only has "dest_icao")
- "altitude_kft" (NOT PRESENT)
- "speed_mph" (NOT PRESENT)
- "track_deg" (NOT PRESENT)
- "vertical_rate_fps" (NOT PRESENT)

**Finding:** The FlightFieldWidget implementation only handles 5 of 10 required fields. The telemetry fields (which would use `formatTelemetryValue()`) are completely missing.

**Impact:** AC #1 claim "supported field names (same set as AC #1)" is DEMONSTRABLY FALSE. Developers will select unsupported fields from the API and get "--" fallback silently.

**Required:** Add telemetry field handling using `formatTelemetryValue()`:
```cpp
if (strcmp(key, "altitude_kft") == 0) {
    // value, suffix, decimals
    double val = f.altitude_kft;
    // Format with formatTelemetryValue()
}
// ... similar for speed_mph, track_deg, vertical_rate_fps
```

---

### 8. **Test Coverage Lies - Heap Delta Test Not Real**
**Severity: 🟡 MEDIUM**

**AC #10 specifies:**
> "1000-iteration dispatch heap-delta == 0"

**Story Task 7 Dev Notes show pattern:**
```cpp
TEST_ASSERT_UINT32_WITHIN(64, heapBefore, heapAfter);
```

**Finding:** This test pattern is "weak" because `heapBefore` and `heapAfter` would be identical if the heap is already fragmented or if ESP32's heap tracking is coarse-grained. The assertion `WITHIN(64, X, X)` always passes when both are the same value.

**Reality:** ESP32 FreeRTOS heap allocation/deallocation don't return memory to the same address on every cycle. A proper zero-heap test would need to:
1. Measure heap before
2. Run 1000 iterations
3. Measure heap after
4. Assert difference == 0

**But the provided pattern doesn't prove static buffers are actually being reused - it just proves heap didn't grow overall.**

**Impact:** Test passes trivially even if there ARE allocations happening per render (as long as they're freed by iteration end).

**Finding:** The test is a "lying test" - it gives false confidence without actually verifying the zero-heap requirement.

---

### 9. **CustomLayoutMode Error Indicator (AC #5) Never Tested**
**Severity: 🟡 MEDIUM**

**AC #5 specifies:**
> "render `"\u2014"` (em dash) and returns `true` — never crashes, no OOB write"

For the `nullptr` flight case. But there's also:
> "Display should draw a visible 1px red border + "LAYOUT ERR" text"

When `_invalid = true` per the implementation notes.

**Finding:** No test validates the error rendering path. AC #5 assumes silent em-dash, but the implementation shows a red border + "LAYOUT ERR" text. These are contradictory behaviors not defined in AC #5.

**Impact:** Spec drift. The implementation goes beyond AC #5 without updating the AC.

**Required:** Either:
1. Update AC #5 to document the red border error UI
2. Remove the error border code and stick to AC #5's silent em-dash behavior

---

### 10. **DisplayUtils Helper Methods Not Documented**
**Severity: 🟡 MEDIUM**

**Story Task 3 (FlightFieldWidget) references:**
```cpp
DisplayUtils::truncateToColumns(value, maxCols, out, sizeof(out));
DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);
```

**Story Task 4 (MetricWidget) references:**
```cpp
DisplayUtils::formatTelemetryValue(value, suffix, decimals, buf, sizeof(buf));
```

**Finding:** These methods are claimed to exist but NO signatures or documentation are provided in the context. The story assumes they're available but doesn't verify they're actually implemented in DisplayUtils.h/.cpp or describe their exact behavior.

**Example unknown behaviors:**
- Does `truncateToColumns()` accept a max column count or a pixel width?
- Does `formatTelemetryValue()` handle NAN values or return an error?
- What suffix strings work? UTF-8? Single char?

**Impact:** Implementation is guessing at API contracts. If DisplayUtils signatures differ, the code breaks.

**Required:** Provide or verify DisplayUtils function signatures match story assumptions.

---

### 11. **MetricWidget Suffix String UTF-8 Issue**
**Severity: 🟡 MEDIUM**

**MetricWidget.cpp shows:**
```cpp
static const char kDegreeSuffix[] = { (char)0xF8, '\0' };
```

**Finding:** This is a single-byte CP437 character code, not UTF-8. The degree symbol in UTF-8 is `0xC2 0xB0` (two bytes). Using `0xF8` assumes the display uses CP437 or similar legacy encoding.

**Story AC #4 specifies:**
```
"track_deg" → formatTelemetryValue(val, "\xc2\xb0", 0)  // °
```

**This is contradictory:**
- AC shows UTF-8 `"\xc2\xb0"` (correct)
- Implementation shows single-byte `0xF8` (wrong)

**Impact:** Degree symbol will render as the wrong character or garbage depending on the display's font encoding.

**Required:** Use proper UTF-8:
```cpp
static const char kDegreeSuffix[] = { (char)0xC2, (char)0xB0, '\0' };
```

Or accept the AC's spec `"\xc2\xb0"` directly.

---

## 🔧 Missing Implementation Details

### 12. **AC #2 (MetricWidget null flight) Not Actually Tested**
**Severity: 🟡 MEDIUM**

**AC #2 specifies:**
> "ctx.currentFlight is nullptr → renders "--""

**Story Task 8 Dev Notes show test pattern:**
```cpp
TEST_ASSERT_TRUE(renderMetric(spec, ctx));
```

With `ctx.currentFlight = nullptr`.

**Finding:** The provided context doesn't show the actual test implementation. We have the pattern but no evidence the test exists or passes.

**Impact:** Cannot verify AC #2 is implemented.

---

### 13. **AC #3 NAN Fallback Never Tested on Real FlightInfo**
**Severity: 🟡 MEDIUM**

**AC #3 specifies:**
> "altitude_kft with NAN value → renders "--""

**Story notes describe pattern but actual test NOT PROVIDED.**

**Finding:** No test shows a real FlightInfo with altitude_kft = NAN being rendered.

**Impact:** NAN handling is assumed but unverified.

---

## 📋 SUMMARY OF CRITICAL GAPS

| Issue | Severity | Status |
|-------|----------|--------|
| Test stubs still assert FALSE | CRITICAL | Task 6 marked [x] but NOT DONE |
| RenderContext pointer not properly initialized | CRITICAL | Uninitialized garbage risk |
| CustomLayoutMode wiring ambiguous | CRITICAL | AC #1 implementation unclear |
| FlightField API options incomplete | HIGH | 5 of 10 fields missing |
| Metric field names inconsistent across layers | HIGH | Editor/API/Code mismatch |
| FlightFieldWidget missing 5 of 10 fields | HIGH | Telemetry fields not handled |
| AC #5 spec drift (error UI undefined) | MEDIUM | Implementation beyond AC |
| UTF-8 encoding issue in degree suffix | MEDIUM | AC shows UTF-8, code uses CP437 |
| Heap delta test is weak/lying | MEDIUM | False confidence |
| DisplayUtils methods not documented | MEDIUM | Unknown API contracts |

---

## 🎯 Evidence Score Calculation

**Findings:**
- 3 CRITICAL issues: +9 points
- 5 HIGH issues: +5 points  
- 5 MEDIUM issues: +1.5 points
- 0 clean pass categories: 0 points

**Evidence Score: 15.5**

**Verdict: 🔴 REJECT - MAJOR REWORK REQUIRED**

The story has critical completion gaps, unverified AC implementations, and conflicting specifications across firmware/API/UI layers. Multiple acceptance criteria are not actually implemented despite being marked complete.

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
<var name="session_id">e70979a0-6120-4077-acd0-e9a4251573cd</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="41af4780">embedded in prompt, file id: 41af4780</var>
<var name="story_id">le-1.8</var>
<var name="story_key">le-1-8-flight-field-and-metric-widgets</var>
<var name="story_num">8</var>
<var name="story_title">8-flight-field-and-metric-widgets</var>
<var name="template">False</var>
<var name="timestamp">20260417_1749</var>
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
      - Commit message format: fix(component): brief description (synthesis-le-1.8)
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