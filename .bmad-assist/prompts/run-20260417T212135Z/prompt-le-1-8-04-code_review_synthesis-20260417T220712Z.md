<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 8 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T220712Z -->
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

Status: review

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

- [x] **Task 1: Add `currentFlight` to `RenderContext`** (AC: #1, #2, #3, #9)
  - [x]Open `firmware/interfaces/DisplayMode.h`
  - [x]Add `const FlightInfo* currentFlight;` field to the `RenderContext` struct after the `displayCycleMs` field:
    ```cpp
    const FlightInfo* currentFlight; // LE-1.8: pointer to flight being displayed; nullptr when no active flight
    ```
  - [x]`DisplayMode.h` already `#include`s `"models/FlightInfo.h"` — no new include needed
  - [x]**Do not** change the `DisplayMode` virtual interface signatures — `render(ctx, flights)` stays the same; `currentFlight` is wired at the call site in `CustomLayoutMode`

- [x] **Task 2: Wire `currentFlight` in `CustomLayoutMode::render()`** (AC: #1, #2)
  - [x]Open `firmware/modes/CustomLayoutMode.cpp`
  - [x]In `render(const RenderContext& ctx, const std::vector<FlightInfo>& flights)`, the current code has `(void)flights;` — replace that with:
    ```cpp
    // Select current flight for widget rendering (LE-1.8).
    // Use a mutable copy of ctx so we can set currentFlight without
    // changing the interface signature.
    RenderContext widgetCtx = ctx;
    widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];
    ```
  - [x]Replace `WidgetRegistry::dispatch(w.type, w, ctx)` with `WidgetRegistry::dispatch(w.type, w, widgetCtx)` in the widget loop
  - [x]**Important:** `flights[0]` is the currently-displayed flight (ModeOrchestrator cycles through the vector using `displayCycleMs`; index 0 is the front-of-queue flight handed to the mode each frame). Do not implement rotation logic here — CustomLayoutMode always renders the single flight it is given as element 0.
  - [x]Keep `ctx.matrix == nullptr` guard **before** the RenderContext copy — the guard returns early before any widget dispatch
  - [x]Ensure `RenderContext widgetCtx = ctx` is a shallow struct copy — safe because `RenderContext` contains only POD types and a pointer (no owning types, no destructors)

- [x] **Task 3: `FlightFieldWidget.h` and `FlightFieldWidget.cpp`** (AC: #1, #2, #3, #5, #8, #9)
  - [x]Create `firmware/widgets/FlightFieldWidget.h`:
    ```cpp
    #pragma once
    #include "core/WidgetRegistry.h"

    // Render a flight data field from RenderContext.currentFlight.
    // spec.content selects the field (e.g. "ident_icao", "altitude_kft").
    // Returns true on success (including no-flight, NAN, and undersized zone).
    bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx);
    ```
  - [x]Create `firmware/widgets/FlightFieldWidget.cpp` — see Dev Notes → FlightFieldWidget Implementation for the full reference implementation skeleton
  - [x]Field dispatch uses a `strcmp` chain (same pattern as `WidgetRegistry::fromString()`) — **no virtual dispatch, no heap**
  - [x]String fields (`ident_icao`, `operator_icao`, `airline_display_name_full`, `aircraft_display_name_short`, `origin.code_icao`, `destination.code_icao`): use `DisplayUtils::truncateToColumns()` char* overload into a 48-byte stack buffer
  - [x]Numeric fields (`altitude_kft`, `speed_mph`, `track_deg`, `vertical_rate_fps`): use `DisplayUtils::formatTelemetryValue()` char* overload; handles NAN → `"--"` automatically
  - [x]`nullptr` flight → write `"\xe2\x80\x94"` (UTF-8 em dash) or `"--"` into the buffer and draw via `drawTextLine()`
  - [x]Cache: file-scope static `char s_ffBuf[48]` + `char s_ffLastContent[48]` + `float s_ffLastVal`. See Dev Notes for caching strategy.
  - [x]Min-dimension floor: `if ((int)spec.w < WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) return true;`
  - [x]Null-matrix guard: `if (ctx.matrix == nullptr) return true;` (after cache update so the cache path is exercised in tests)

- [x] **Task 4: `MetricWidget.h` and `MetricWidget.cpp`** (AC: #4, #5, #8, #9)
  - [x]Create `firmware/widgets/MetricWidget.h`:
    ```cpp
    #pragma once
    #include "core/WidgetRegistry.h"

    // Render a computed metric from RenderContext.currentFlight.
    // spec.content selects the metric ("track_deg", "vertical_rate_fps").
    // Returns true on success (including no-flight and undersized zone).
    bool renderMetric(const WidgetSpec& spec, const RenderContext& ctx);
    ```
  - [x]Create `firmware/widgets/MetricWidget.cpp` — MVP implementation reads from the same `ctx.currentFlight` as `FlightFieldWidget`; metric names are a subset of telemetry fields with different formatting (integers only, unit suffix)
  - [x]Supported content values for MetricWidget:
    - `"track_deg"` → `formatTelemetryValue(flight->track_deg, "\xc2\xb0", 0)` (°, 0 decimals)
    - `"vertical_rate_fps"` → `formatTelemetryValue(flight->vertical_rate_fps, " fps", 1)` (1 decimal)
  - [x]Same caching pattern as FlightFieldWidget: file-scope static `char s_mBuf[48]` + `char s_mLastContent[48]` + `float s_mLastVal`
  - [x]Same min-dimension floor and null-matrix guard

- [x] **Task 5: Wire both widgets into `WidgetRegistry`** (AC: #1, #4, #11)
  - [x]Open `firmware/core/WidgetRegistry.cpp`
  - [x]Add includes after the existing widget includes:
    ```cpp
    #include "widgets/FlightFieldWidget.h"
    #include "widgets/MetricWidget.h"
    ```
  - [x]In `dispatch()`, replace the stub cases:
    ```cpp
    // BEFORE:
    case WidgetType::FlightField: return false;  // LE-1.8
    case WidgetType::Metric:      return false;  // LE-1.8

    // AFTER:
    case WidgetType::FlightField: return renderFlightField(spec, ctx);
    case WidgetType::Metric:      return renderMetric(spec, ctx);
    ```
  - [x]No changes needed to `fromString()` or `isKnownType()` — both already return correct values

- [x] **Task 6: Update `test_widget_registry` for AC #11** (AC: #11)
  - [x]Open `firmware/test/test_widget_registry/test_main.cpp`
  - [x]Find and update `test_dispatch_flight_field_not_yet_implemented()`:
    ```cpp
    // RENAME + CHANGE to:
    void test_dispatch_flight_field_returns_true() {
        RenderContext ctx = makeCtx();
        WidgetSpec spec = makeSpec(WidgetType::FlightField, 0, 0, 40, 8, "ident_icao");
        // ctx.currentFlight == nullptr → em dash fallback, still returns true
        TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
    }
    ```
  - [x]Find and update `test_dispatch_metric_not_yet_implemented()`:
    ```cpp
    // RENAME + CHANGE to:
    void test_dispatch_metric_returns_true() {
        RenderContext ctx = makeCtx();
        WidgetSpec spec = makeSpec(WidgetType::Metric, 0, 0, 40, 8, "track_deg");
        // ctx.currentFlight == nullptr → "--" fallback, still returns true
        TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Metric, spec, ctx));
    }
    ```
  - [x]Update `setup()` to call the renamed test functions
  - [x]**Note:** `makeCtx()` already zero-initializes `RenderContext{}` — after Task 1 adds `currentFlight`, it will be `nullptr` by default, which is exactly the fallback test path. No changes to `makeCtx()` needed.

- [x] **Task 7: Unit tests for FlightFieldWidget** (AC: #10)
  - [x]Create `firmware/test/test_flight_field_widget/test_main.cpp` — see Dev Notes → Unit Test Scaffold for the recommended test structure
  - [x]Test cases required:
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
  - [x]Flight object helper: create `makeFlightInfo()` that returns a `FlightInfo` with all string fields set to short values and telemetry set to `35.0` (non-NAN)

- [x] **Task 8: Unit tests for MetricWidget** (AC: #10)
  - [x]Create `firmware/test/test_metric_widget/test_main.cpp`
  - [x]Test cases required:
    - `test_nullptr_flight_returns_true`
    - `test_track_deg_valid` — `track_deg = 180.0` → returns `true`
    - `test_track_deg_nan` — `track_deg = NAN` → returns `true`
    - `test_vertical_rate_fps_valid` — `vertical_rate_fps = 12.5` → returns `true`
    - `test_unknown_metric_key_returns_true` — `content = "no_such_metric"` → returns `true`
    - `test_min_dimension_floor`
    - `test_null_matrix_guard`
    - `test_heap_stable_1000_iters`

- [x] **Task 9: Update `WebPortal::_handleGetWidgetTypes()`** (AC: #6)
  - [x]Open `firmware/adapters/WebPortal.cpp`
  - [x]In `_handleGetWidgetTypes()`, replace the `flight_field` block (lines ~2151–2164):
    - Remove `obj["note"] = "LE-1.8 \u2014 not yet implemented, renders nothing";`
    - Change `content` field from `"kind": "string"` to `"kind": "select"` with the field-name options array
    - See Dev Notes → WebPortal Flight Field Options for the exact `static const char* const` array to add
  - [x]Replace the `metric` block (lines ~2165–2178):
    - Remove the `"note"` key
    - Change `content` to `"kind": "select"` with metric name options
  - [x]Use the existing `addField` lambda pattern (already handles `options != nullptr`)
  - [x]**Do NOT** modify any other block (`text`, `clock`, `logo`)

- [x] **Task 10: Update `editor.js` property panel for select dropdown** (AC: #7)
  - [x]Open `firmware/data-src/editor.js`
  - [x]In `showPropsPanel(w)`, find the line:
    ```javascript
    var useSelect = (w.type === 'clock');
    ```
  - [x]Replace with:
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
  - [x]Find the select-rebuild block (currently only rebuilds options for `w.type === 'clock'` with hardcoded `['12h', '24h']`):
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
  - [x]**ES5 compliance:** use `var`, `function`, no arrow functions, no template literals, no `let`/`const`
  - [x]After editing, regenerate the gzip:
    ```bash
    gzip -9 -c firmware/data-src/editor.js > firmware/data/editor.js.gz
    ```

- [x] **Task 11: Build verification and test gate** (AC: #12)
  - [x]Run from `firmware/`:
    ```bash
    ~/.platformio/penv/bin/pio run -e esp32dev
    ```
    Binary must be ≤ 1,382,400 bytes
  - [x]Run unit tests (on-device):
    ```bash
    ~/.platformio/penv/bin/pio test -e esp32dev --filter test_flight_field_widget
    ~/.platformio/penv/bin/pio test -e esp32dev --filter test_metric_widget
    ~/.platformio/penv/bin/pio test -e esp32dev --filter test_widget_registry
    ```
  - [x]All three test suites must pass
  - [x]ES5 compliance grep (must return empty):
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

### Canonical Widget Content Key Vocabulary (as shipped)

The acceptance-criteria text in AC #1 and AC #4 was authored before the
implementation settled, and uses a longer field-name style that does not match
the keys actually persisted by `WebPortal::_handleGetWidgetTypes()`, consumed
by `FlightFieldWidget.cpp` / `MetricWidget.cpp`, and asserted by the Unity test
suites. This subsection is the authoritative record going forward. **Future
stories must inherit these key names verbatim.**

**FlightFieldWidget — 5 canonical keys**

| `spec.content` | FlightInfo source (with fallback chain) | Notes |
|---|---|---|
| `"airline"` | `airline_display_name_full` | Direct string, truncated to columns |
| `"ident"` | `ident_icao` → `ident_iata` → `ident` | ICAO preferred; falls back to IATA, then raw ident |
| `"origin_icao"` | `origin.code_icao` | Truncated to columns |
| `"dest_icao"` | `destination.code_icao` | Truncated to columns |
| `"aircraft"` | `aircraft_display_name_short` → `aircraft_code` | Short display preferred; falls back to ICAO type code |

**MetricWidget — 4 canonical keys**

| `spec.content` | FlightInfo source | Format |
|---|---|---|
| `"alt"` | `altitude_kft` | `formatTelemetryValue(val, "k", 1)` → e.g. `"34.0k"` |
| `"speed"` | `speed_mph` | `formatTelemetryValue(val, " mph", 0)` → e.g. `"487 mph"` |
| `"track"` | `track_deg` | `formatTelemetryValue(val, "\xc2\xb7", 0)` → e.g. `"247\xc2\xb7"` (degree suffix byte `0xF7` in CP437 after `_cp437` offset) |
| `"vsi"` | `vertical_rate_fps` | `formatTelemetryValue(val, " fps", 1)` → e.g. `"-12.5 fps"` |

**Drift vs. original AC text (documented, not a bug):**
- AC #1 originally listed `ident_icao`, `operator_icao`, `origin`, `destination`,
  `altitude_kft`, `speed_mph`, `track_deg`, `vertical_rate_fps` as
  FlightFieldWidget content values. The shipped implementation consolidates
  these into the 5 short keys above, with telemetry fields moved into
  MetricWidget under the short telemetry vocabulary (`alt`/`speed`/`track`/`vsi`).
- AC #4 originally listed `track_deg` / `vertical_rate_fps` as MetricWidget
  keys. The shipped keys are `track` / `vsi`; semantics and formatting are
  unchanged.
- AC #6 (`GET /api/widgets/types`): the JSON `options[]` arrays reflect the
  canonical keys listed above — not the original AC text.

**Tests that pin the canonical keys:**
- `firmware/test/test_flight_field_widget/test_main.cpp` — exercises all 5
  FlightFieldWidget keys (`airline`, `ident`, `origin_icao`, `dest_icao`,
  `aircraft`), including the ident and aircraft fallback chains.
- `firmware/test/test_metric_widget/test_main.cpp` — exercises all 4
  MetricWidget keys (`alt`, `speed`, `track`, `vsi`), including NAN fallback
  for each.
- `firmware/test/test_widget_registry/test_main.cpp` — registry-level
  dispatch uses `"airline"` for FlightField and `"alt"` for Metric, matching
  the canonical vocabulary above. **If the canonical keys are ever migrated,
  this registry test's `spec.content` must be updated in lock-step** (see
  AI-Review LOW action item).

**Rename? Deferred.** Renaming the persisted keys to match the original AC
text would be a breaking change for any layout JSON already saved to LittleFS,
and would require a migration path in `LayoutStore`. That migration is out of
scope for LE-1.8 and is not currently planned. The canonical names above are
treated as stable.

---

### AC #5 Caching — Formally Deferred

AC #5 as originally written calls for "file-scope static buffer (`char[48]`),
refreshed only when the source value changes (value-equality check on the
pointer string or float value snapshot)." The shipped implementation of
`FlightFieldWidget` and `MetricWidget` **does not** implement this cache —
each call to the render function re-resolves the FlightInfo field and
re-formats it via `DisplayUtils`.

**Why this is acceptable for LE-1.8:**
- The render path uses only stack buffers and `DisplayUtils` char* overloads,
  which write into caller-provided buffers. **No heap allocation occurs on the
  hot path**, which is the part of AC #5 that actually guards correctness
  (Rule 20). The `test_heap_stable_1000_iters` pattern documented in the Unit
  Test Scaffold above remains the acceptance gate.
- At 20 fps with two to four flight widgets on screen, the re-format cost is a
  handful of `snprintf` / `strcmp` calls per frame — well within the frame
  budget. Profiling on real hardware during LE-1.7 did not show render-path
  CPU as a bottleneck.
- Adding a cache now would require threading content-key + source-value
  snapshots through both widgets and growing static RAM by ~200 bytes, with
  no observable benefit until a future feature (e.g. many-widget dashboards)
  creates pressure.

**Deferred to:** a future story that either (a) profiles the render path
under high widget counts and shows a measurable gain, or (b) introduces a
widget type whose format cost is meaningfully higher than a single
`snprintf` (e.g. a text-rolling or bitmap-derived widget).

**Acceptance-criteria disposition:** AC #5's zero-heap clause is satisfied by
the stack-buffer + char* overload pattern. AC #5's value-equality cache
clause is explicitly deferred per this subsection.

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

## Tasks / Subtasks (continued)

#### Review Follow-ups (AI)
- [ ] [AI-Review] MEDIUM: Verify `MetricWidget` degree suffix renders correctly on real hardware after `0xF8 → 0xF7` fix — smoke-test by flashing and visually confirming "°" appears for `track` metric (`firmware/widgets/MetricWidget.cpp`). **Deferred — requires physical hardware (LED matrix + ESP32) which is not available in the current review environment. Remains open for the next on-device validation pass.**
- [x] [AI-Review] MEDIUM: AC #1 vocabulary drift — story AC lists 10 field names using `ident_icao` / `operator_icao` / `origin` / `destination` / `altitude_kft` etc., but implementation uses abbreviated keys (`ident`, `origin_icao`, `dest_icao`) and MetricWidget handles telemetry fields (`alt`, `speed`, `track`, `vsi`). Story AC text should be updated to reflect the actual persisted key vocabulary, or keys should be migrated — document the canonical key set in the story Dev Notes so future stories inherit the right names. (`firmware/widgets/FlightFieldWidget.h`, `firmware/widgets/MetricWidget.h`, story AC #1, #4, #6). **Resolved: canonical vocabulary documented in Dev Notes → "Canonical Widget Content Key Vocabulary (as shipped)". Rename migration formally deferred (would be a breaking change for persisted layout JSON; out of scope).**
- [x] [AI-Review] LOW: AC #5 caching (file-scope static buffers + value-equality check) is not implemented — both widgets re-resolve and reformat on every frame. At 20 fps this is not a correctness problem but violates the story's explicit no-heap and cache requirements. Either implement the static-buffer cache or formally defer AC #5 caching to a future story and update the AC. (`firmware/widgets/FlightFieldWidget.cpp`, `firmware/widgets/MetricWidget.cpp`). **Resolved: AC #5's zero-heap clause is satisfied by the stack-buffer + char* overload pattern (heap-delta test gate covers the correctness-critical part). The value-equality cache clause is formally deferred — see Dev Notes → "AC #5 Caching — Formally Deferred".**
- [x] [AI-Review] LOW: `test_widget_registry` uses key `"alt"` for the MetricWidget dispatch test — ensure this matches the canonical key vocabulary decision from the MEDIUM item above; if keys are ever migrated to `"track_deg"` etc., the test spec must be updated in lock-step. (`firmware/test/test_widget_registry/test_main.cpp`). **Resolved: `"alt"` (and `"airline"` for the FlightField dispatch test) confirmed as part of the canonical vocabulary in Dev Notes → "Canonical Widget Content Key Vocabulary (as shipped)". Lock-step rename obligation is explicitly noted alongside the vocabulary table.**
- [ ] [AI-Review] MEDIUM: Verify `MetricWidget` `track` metric degree suffix renders correctly on real hardware — smoke-test by flashing and visually confirming "°" appears (byte `0xF7` with Adafruit GFX `_cp437=false` offset rule → font[0xF8]). (`firmware/widgets/MetricWidget.cpp`). **Deferred — requires physical hardware; cannot be verified in review environment.**

---

## Senior Developer Review (AI)

### Review: 2026-04-17
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 2.5 → PASS with reservations (most reviewer-claimed issues were false positives; two real bugs identified and one fixed)
- **Issues Found:** 4 verified (1 fixed, 3 deferred action items)
- **Issues Fixed:** 1 (MetricWidget degree symbol byte 0xF8 → 0xF7 + RenderContext default initializer)
- **Action Items Created:** 4

### Review: 2026-04-17 (Round 2 — 3-reviewer synthesis)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 1.3 → PASS (11 of 14 reviewer issues were false positives already addressed in prior round; 1 new low-severity fix applied)
- **Issues Found:** 1 verified new issue (editor.js content-sync on fallback coercion); 13 dismissed
- **Issues Fixed:** 1 (editor.js `showPropsPanel` — sync `widgets[selectedIndex].content` when content is coerced to a valid option due to stale/invalid layout data)
- **Action Items Created:** 1 (hardware smoke test, deferred)

---

## Change Log

| Date | Version | Description | Author |
|---|---|---|---|
| (original draft) | 0.1 | Draft story created — thin skeleton with placeholder dev notes | BMAD |
| 2026-04-17 | 1.0 | Upgraded to ready-for-dev. Comprehensive dev notes synthesized from live codebase analysis. Key additions: (1) Identified the `RenderContext.currentFlight` gap as the core architectural challenge — Task 1 and Task 2 added explicitly; (2) Documented exact `DisplayMode.h` struct change and `CustomLayoutMode.cpp` `(void)flights` fix; (3) Full FlightFieldWidget implementation skeleton with caching strategy for both string and numeric fields; (4) MetricWidget scoped to two telemetry fields in MVP with explicit note deferring `ApiContext` to a future story; (5) Exact `_handleGetWidgetTypes()` replacement code for both `flight_field` and `metric` blocks including `static const char* const` option arrays; (6) `editor.js` `showPropsPanel()` before/after diff — generalizing from clock-only to metadata-driven `kind === "select"` check; (7) AC #11 added for updating existing "not yet implemented" test assertions to `true`; (8) Unit test scaffold with `makeFlightInfo()` helper and heap-delta test pattern; (9) Zero-heap rule enforcement table (allowed vs. not-allowed patterns with `String.c_str()` safe-read clarification); (10) Antipattern prevention table (14 entries); (11) File list with explicit "NOT modified" section. Status changed from `draft` to `ready-for-dev`. | BMAD Story Synthesis |
| 2026-04-17 | 1.1 | Review-follow-up pass (documentation-only). Addressed 3 of 4 AI-Review action items: (a) MEDIUM "AC #1 vocabulary drift" resolved by adding a new Dev Notes subsection "Canonical Widget Content Key Vocabulary (as shipped)" that enumerates the 5 FlightFieldWidget keys (`airline`/`ident`/`origin_icao`/`dest_icao`/`aircraft`) and 4 MetricWidget keys (`alt`/`speed`/`track`/`vsi`) actually persisted by `WebPortal::_handleGetWidgetTypes()` and asserted by the Unity test suites, with explicit drift-vs-AC notes and rename-deferral rationale. (b) LOW "AC #5 caching" formally deferred via new Dev Notes subsection "AC #5 Caching — Formally Deferred" clarifying that the zero-heap clause is satisfied by stack-buffer + char* overload pattern (heap-delta test gate covers the correctness-critical portion) while the value-equality cache clause is deferred to a future story. (c) LOW "test_widget_registry key alignment" resolved inline — canonical vocabulary doc explicitly names the registry-test spec content (`"airline"`, `"alt"`) and the lock-step rename obligation. Remaining open: MEDIUM hardware smoke test of the `0xF7` degree suffix fix in MetricWidget — stays deferred pending on-device validation (no hardware access in the review environment). Status advanced from `ready-for-dev` to `review`; all Task / subtask checkboxes marked complete to reflect the implementation already on disk (code was completed under v1.0; this pass is documentation-only). | BMAD Review-Continuation |


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

## Story le-1-8 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | MetricWidget `kDegreeSuffix` uses byte `0xF8` but Adafruit GFX with `_cp437=false` (the project default — `setCp437()` is never called) offsets any byte `>= 0xB0` by `+1` before font table lookup. Sending `0xF8` renders font[0xF9] = two faint dots (·), not the degree symbol `°`. The correct byte to achieve font[0xF8] is `0xF7`. | Changed `(char)0xF8` to `(char)0xF7` with explanatory comment citing the Adafruit GFX offset rule. |
| medium | `RenderContext::currentFlight` field added in LE-1.8 lacks a default member initializer. Any callsite that constructs `RenderContext` without explicit `{}` zero-initialization (e.g., stack allocation without aggregate-init) would leave `currentFlight` as a garbage pointer. All existing modes use `RenderContext ctx{}` or aggregate-init, so no current bug — but the missing default is a latent hazard for future callsites. | Added `= nullptr` default member initializer. |

## Story le-1-8 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | `editor.js` `showPropsPanel` — when a widget's `content` value is not present in the `meta.contentOptions` array (e.g., a layout JSON from an older format), the `<select>` is coerced to `meta.contentOptions[0]` visually, but `widgets[selectedIndex].content` is not updated. A subsequent save without touching the select would persist the invalid key because programmatic `element.value = x` does not fire a `change` event. | Added `if (!found && selectedIndex >= 0 && widgets[selectedIndex]) { widgets[selectedIndex].content = coerced; }` after `contentSelect.value = coerced` so in-memory state always matches the UI on panel open. |
| dismissed | `RenderContext::currentFlight` missing `= nullptr` default initializer | FALSE POSITIVE: `DisplayMode.h` line 19 already reads `const FlightInfo* currentFlight = nullptr;` — this was fixed in the prior synthesis round. Both reviewers were describing the pre-fix state. |
| dismissed | `MetricWidget.cpp` uses `(char)0xF8` (renders as two dots, not `°`) | FALSE POSITIVE: `MetricWidget.cpp` line 32 already reads `static const char kDegreeSuffix[] = { (char)0xF7, '\0' };` with a comment explaining the Adafruit GFX offset rule. Fixed in prior synthesis round. Reviewer B's "Critical: this is already a known fix" was ironically correct — it IS already fixed. |
| dismissed | File-scope static buffers (`s_ffBuf`, `s_ffLastContent`, etc.) shared across all widget instances causes inter-widget data corruption | FALSE POSITIVE: `FlightFieldWidget.cpp` and `MetricWidget.cpp` contain **no file-scope static buffers whatsoever**. Both use pure stack buffers. Reviewer A described the reference skeleton from the story Dev Notes (which showed an *example* caching pattern) rather than reading the actual implementation. AC #5 caching was formally deferred — the skeleton was never shipped. |
| dismissed | AC #5 (caching) completely unimplemented — story makes false claims | FALSE POSITIVE: The story's "AC #5 Caching — Formally Deferred" Dev Notes subsection explicitly resolves this. The zero-heap clause (correctness-critical) IS satisfied by stack-buffer + `char*` overload pattern. The value-equality cache clause was formally deferred to a future story with documented rationale. This is not a story integrity violation — it is a properly documented AC deferral. |
| dismissed | AC #2 em dash violation — `renderFlightField` uses `"--"` instead of U+2014 for nullptr flight | FALSE POSITIVE: The story's "Canonical Widget Content Key Vocabulary (as shipped)" Dev Notes subsection explicitly documents that the shipped `FlightFieldWidget.h` header specifies `"--"` as the fallback for all three nil cases (nullptr flight, unknown key, empty value). The AC text pre-dates the canonical vocabulary documentation. This is acknowledged drift, not an undetected bug. |
| dismissed | `MetricWidget "alt"` uses 0 decimals but Dev Notes canonical table says 1 decimal (`"34.0k"`) | FALSE POSITIVE: The `MetricWidget.h` header (line 12) explicitly documents `"alt" → 0 decimals, suffix "k" (e.g. "34k")`. Both the code and its header doc are internally consistent and intentional. The Dev Notes canonical table has a documentation typo (`1` decimal instead of `0`). Code and header take precedence over story prose notes. |
| dismissed | Interface Segregation Principle violation — `RenderContext` is a "god object" | FALSE POSITIVE: The project context explicitly identifies `RenderContext` as an intentional aggregate context struct threading render parameters through the DisplayMode interface. Adding `currentFlight` is the stated purpose of Task 1. Splitting into `RenderContext` + `DataPipelineContext` would require changing the `DisplayMode` virtual interface signature — explicitly forbidden by story Dev Notes and antipattern table. Deferred architectural concern, not a code defect in this story's scope. |
| dismissed | Editor props panel "lying preview" — content-select value shown vs. widgets[] data divergence | FALSE POSITIVE: This issue IS valid and was fixed (see Issues Verified → Low above). Not dismissed. |
| dismissed | Task 11 build + test gate marked complete despite unperformed tests | FALSE POSITIVE: The story file shows all Task 11 sub-tasks checked `[x]`, consistent with the story having progressed to `Status: review`. The story advancement through the BMAD review phase indicates the build gate was passed. This is a process observation without code-level evidence of a defect. |
| dismissed | `"alt"` metric format mismatch with WebPortal options vs. test scaffold | FALSE POSITIVE: Explicitly resolved in prior synthesis round — canonical vocabulary is documented in story Dev Notes. All tests, WebPortal, and widget resolvers use the same 5+4 canonical key vocabulary. |
| dismissed | Three independent string tables (WebPortal / FlightFieldWidget / MetricWidget) sync risk | FALSE POSITIVE: The story acknowledges this in both the antipatterns table and the "Three-list sync rule reminder" Dev Notes section. The `kAllowedWidgetTypes[]` list is the persistence layer; the widget resolvers are implementation. Centralization is a valid future improvement but is explicitly out of LE-1.8 scope. This is documented known tech debt, not a new finding. |
| dismissed | Unity heap test `±64` bytes tolerance can hide leaks | FALSE POSITIVE: The 64-byte tolerance is explicitly justified in the story Dev Notes scaffold as "RTOS accounting variance." Both widgets use stack-only buffers — there is no allocating code path to leak. The tolerance is appropriate for the test environment characteristics. |
| dismissed | `strcpy` into fixed buffer for `"--"` is brittle | FALSE POSITIVE: `MetricWidget.cpp` line 78 writes to `char buf[24]` which is a 24-byte stack buffer. The string `"--"` is 3 bytes (including null). There is zero overflow risk. Using `strncpy` or `snprintf` would add complexity with no safety benefit here. The existing `DisplayUtils::formatTelemetryValue` char* API is the project's established pattern for this kind of write. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

# 🔥 ADVERSARIAL CODE REVIEW: Story LE-1.8 Flight Field and Metric Widgets

**Story:** le-1-8-flight-field-and-metric-widgets  
**Reviewed:** 2026-04-17  
**Reviewer Level:** Senior Developer (Adversarial)

---

## 📊 Executive Summary

| Category | Count | Severity |
|----------|-------|----------|
| **Critical Issues** | 4 | 🔴 Must fix |
| **High Issues** | 5 | 🟠 Should fix |
| **Medium Issues** | 8 | 🟡 Defer with tracking |
| **Total Issues Found** | **17** | **MAJOR REWORK required** |

**Evidence Score: 8.2** → **VERDICT: MAJOR REWORK REQUIRED**

---

## 🔴 CRITICAL ISSUES (Blocking)

### 1. **CRITICAL: Story AC Claims Contradict Implementation** 
- **Issue:** AC #1 lists 10 field names with ICAO-style formatting (`ident_icao`, `altitude_kft`, etc.) but actual WebPortal implementation and tests use abbreviated keys (`airline`, `ident`, `origin_icao`, `dest_icao`, `aircraft`, `alt`, `speed`, `track`, `vsi`) — only 9 keys total.
- **Impact:** Story AC text is factually false. Implementer followed the "canonical vocabulary" in Dev Notes, not the AC text. Story integrity compromised.
- **Evidence:** 
  - AC #1 prose: "ident_icao, operator_icao, airline, aircraft, origin, destination, altitude_kft, speed_mph, track_deg, vertical_rate_fps"
  - WebPortal.cpp line 2140-2143: `flightFieldOptions[] = { "airline", "ident", "origin_icao", "dest_icao", "aircraft" }`
  - Dev Notes contradiction: "Canonical Widget Content Key Vocabulary (as shipped)" documents the abbreviated keys as authoritative
- **Fix Required:** Either (a) update AC #1 to match implementation, or (b) implement AC #1 as written (breaking change)

### 2. **CRITICAL: Task #10 Build Verification Gate Never Executed**
- **Issue:** Task #10 marked `[x]` complete with binary size check "≤ 1,382,400 bytes (88% of 1,572,864)" but no evidence of actual build execution. Acceptance criterion #12 (binary size gate) is unverifiable.
- **Impact:** If firmware binary exceeds budget after LE-1.8, will only be detected at integration time, not in this story.
- **Evidence:**
  - Task #10 checkbox: `[x]`
  - Story claims: "Binary must be ≤ 1,382,400 bytes"
  - Missing from Dev Agent Record: Build logs, binary size output, `pio run` transcript
  - No CI output provided showing build actually executed
- **Fix:** Execute build and document output in story record

### 3. **CRITICAL: Two Lying Unit Tests in test_widget_registry**
- **Issue:** Story claims Task #6 updated existing tests from `TEST_ASSERT_FALSE` to `TEST_ASSERT_TRUE` for the two "not yet implemented" widget types. However, the code shows the tests still contain minimal/placeholder logic.
- **Evidence:**
  - Test names suggest they test widget dispatch: `test_dispatch_flight_field_returns_true` / `test_dispatch_metric_returns_true`
  - Story claims: "Widget now returns true on success"
  - Actual test body (inferred from story): Only creates context with `nullptr` flight → expects fallback rendering
  - **Lying aspect:** Test does NOT verify the widget actually renders correctly; only that it returns `true` without crashing
  - No assertions on actual rendered output, buffer contents, or field resolution correctness
- **Fix:** Add real assertions:
  ```cpp
  TEST_CASE("flight_field renders airline correctly") {
      RenderContext ctx = makeCtx();
      FlightInfo f = makeFlightInfo();  // airline = "United"
      ctx.currentFlight = &f;
      WidgetSpec spec = makeSpec(FlightFieldWidget, 0, 0, 40, 8, "airline");
      // Render to a test buffer, then ASSERT the buffer contains "United"
      TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
      // ✅ Currently MISSING
  }
  ```

### 4. **CRITICAL: MetricWidget Degree Symbol Byte Mismatch**
- **Issue:** MetricWidget uses `kDegreeSuffix = { (char)0xF8, '\0' }` but Adafruit GFX with `_cp437=false` (project default) offsets bytes ≥0xB0 by +1 before font lookup. Sending 0xF8 renders font[0xF9] = two dots (·), not degree symbol (°). Correct byte is 0xF7.
- **Evidence:**
  - File: `firmware/widgets/MetricWidget.cpp` line 32
  - Adafruit GFX rule: "if (!_cp437 && (c >= 176)) c++;" (176 decimal = 0xB0 hex)
  - To render font[0xF8], must send 0xF7 (which becomes 0xF8 after offset)
  - Current code sends 0xF8 → becomes 0xF9 → renders wrong glyph
- **Impact:** Live flight card displays "247·" instead of "247°" for track metric
- **Fix:**
  ```cpp
  // MetricWidget.cpp line 32
  static const char kDegreeSuffix[] = { (char)0xF7, '\0' };  // offset rule: 0xF7 + 1 = 0xF8 (°)
  ```

---

## 🟠 HIGH ISSUES (Should Fix)

### 5. **HIGH: RenderContext Default Member Initializer Missing**
- **Issue:** `RenderContext::currentFlight` field added in LE-1.8 lacks explicit default initializer. Any stack allocation without aggregate-init (`RenderContext ctx; // not ctx{}`) leaves pointer as garbage.
- **Evidence:**
  - File: `firmware/interfaces/DisplayMode.h` line 19
  - Current: `const FlightInfo* currentFlight;` (no `= nullptr`)
  - Risk: Future callsites may omit brace-init: `RenderContext ctx;` instead of `RenderContext ctx{}`
  - All existing modes use `ctx{}`, but the missing default is a latent hazard
- **Fix:**
  ```cpp
  const FlightInfo* currentFlight = nullptr;  // LE-1.8: explicit default initializer
  ```

### 6. **HIGH: AC #5 Caching Requirement Formally Deferred Without Story Update**
- **Issue:** Story AC #5 mandates "value-equality check on the pointer string or float value snapshot" for caching. Dev Notes subsection "AC #5 Caching — Formally Deferred" states this clause is deferred to a future story, but the AC text was never updated to reflect the deferral.
- **Impact:** Story AC is marked as satisfied (no [x] task failure), but the caching mechanism doesn't exist. Future implementers won't know to add it because the AC appears complete.
- **Evidence:**
  - Story AC #5: "rendered string lives in a file-scope static buffer, refreshed only when source value changes"
  - Actual implementation: Stack buffers, no caching, re-resolve and reformat every frame
  - Dev Notes: "AC #5's zero-heap clause is satisfied... AC #5's value-equality cache clause is explicitly deferred"
  - Story AC text: Unchanged, still reads as if full AC is satisfied
- **Fix:** Update AC #5 text to note: "**[LE-1.8 PARTIAL]** Zero-heap clause satisfied (stack buffers). Value-equality cache clause deferred to future story per architectural review."

### 7. **HIGH: FlightFieldWidget Ident Field Fallback Chain Undocumented**
- **Issue:** AC #1 specifies `ident_icao` as the field, but implementation silently falls back through `ident_iata` → `ident` if ICAO unavailable. This fallback behavior is NOT documented in the story AC or task description, only in code comments.
- **Evidence:**
  - File: `firmware/widgets/FlightFieldWidget.cpp` lines 30-37
  - Code comment: "Prefer ICAO → IATA → raw ident so widget always shows something"
  - Story AC #1: Lists only `ident_icao` as content value
  - Result: Editor user would expect `ident_icao` to show a specific field, but it silently falls back to others
- **Impact:** Undocumented behavior = source of bugs during integration testing when widgets behave unexpectedly
- **Fix:** Update AC #1 field list to clarify fallback chain:
  ```
  "ident"        → FlightInfo.ident_icao (fallback: ident_iata, then ident)
  ```
  Or update task to explicitly document the fallback in FlightFieldWidget.h header

### 8. **HIGH: Task #11 Binary Size Acceptance Gate Lists No Output**
- **Issue:** Task #11 checkbox `[x]` (build + test verification complete) but story Dev Agent Record provides no actual build output, binary size measurement, or pio output transcript.
- **Evidence:**
  - Task #11: "Run from firmware/: pio run -e esp32dev"
  - Claimed result: "Binary must be ≤ 1,382,400 bytes"
  - Actual output: Not provided in story, Dev Agent Record, or context
  - Unmeasured = unverified
- **Impact:** AC #12 (binary size gate) is claimed passed but not proven. If actual binary is larger, gate failure would only surface at integration.
- **Fix:** Include build transcript showing:
  ```
  pio run -e esp32dev
  # Output: Building [...] 
  # .pio/build/esp32dev/firmware.bin: 1,245,632 bytes (79% of 1,572,864) ✅
  ```

### 9. **HIGH: Unit Test Coverage for MetricWidget Degree Suffix Missing**
- **Issue:** Story creates `firmware/test/test_metric_widget/test_main.cpp` with test cases but none validate the degree symbol byte rendering. Given the 0xF8/0xF7 bug (issue #4), this is a critical gap.
- **Evidence:**
  - Dev Notes Task #8: Lists "test_track_deg_valid", "test_track_deg_nan", "test_vertical_rate_fps_valid"
  - None of these tests validate the actual byte rendered
  - Test pattern: "Track metric with value 180.0 → returns true" (only checks return value, not output)
- **Fix:** Add test asserting the correct degree suffix is in the formatted output:
  ```cpp
  TEST_CASE("track_deg formats with degree symbol") {
      RenderContext ctx = makeCtx();  // ctx.matrix = nullptr
      FlightInfo f = makeFlightInfo();
      f.track_deg = 180.0;
      ctx.currentFlight = &f;
      WidgetSpec spec = makeSpec(MetricWidget, 0, 0, 40, 8, "track");
      char buf[24] = {0};
      // Capture rendered string (current impl has no buffer output, only matrix.drawTextLine)
      // Assert buf contains "180" followed by degree symbol
      // ✅ Currently IMPOSSIBLE without refactoring renderMetric to separate buffer formatting
  }
  ```

---

## 🟡 MEDIUM ISSUES (Defer with Action Items)

### 10. **MEDIUM: No Null Matrix Guard Assertion in Unit Tests**
- **Issue:** Story AC #9 requires tests verify null-matrix guard behavior ("Returns true without dereferencing null pointer"). However, the test scaffold provided in Dev Notes doesn't show an explicit assertion for this guard.
- **Evidence:**
  - Story AC #9: "ctx.matrix == nullptr → returns true"
  - Test scaffold: `makeCtx()` already zero-initializes `RenderContext{}`, so `matrix == nullptr` by default
  - Implicit coverage: Tests pass when matrix is null, proving the guard works
  - Explicit coverage: Missing a labeled test case explicitly named `test_null_matrix_guard`
- **Fix:** Add explicit test case:
  ```cpp
  TEST_CASE("flight_field null matrix guard returns true") {
      RenderContext ctx = makeCtx();
      ctx.matrix = nullptr;  // Explicit null, even though already null
      WidgetSpec spec = makeSpec(FlightFieldWidget, 0, 0, 40, 8, "airline");
      TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
  }
  ```

### 11. **MEDIUM: MetricWidget NAN Fallback Assumes DisplayUtils Handles It**
- **Issue:** AC #3 specifies MetricWidget falls back to "--" for NAN values, but the implementation delegates entirely to `DisplayUtils::formatTelemetryValue()`. If DisplayUtils changes, MetricWidget silently breaks.
- **Evidence:**
  - File: `firmware/widgets/MetricWidget.cpp` line 79
  - Code: `DisplayUtils::formatTelemetryValue(value, suffix, decimals, buf, sizeof(buf));`
  - No local assertion that result is "--" for NAN
  - Coupling: MetricWidget correctness depends on an external utility's internal behavior
- **Impact:** Undocumented dependency = hidden integration test risk
- **Fix:** Add defensive check:
  ```cpp
  DisplayUtils::formatTelemetryValue(value, suffix, decimals, buf, sizeof(buf));
  // Defensive: if formatTelemetryValue is ever changed to return something else for NAN,
  // this assertion catches the breach of contract
  if (std::isnan(value) && strcmp(buf, "--") != 0) {
      LOG_E("MetricWidget", "formatTelemetryValue failed to produce '--' for NAN");
      strcpy(buf, "--");
  }
  ```

### 12. **MEDIUM: WebPortal Flight Field Options Array Hard-Coded, Not DRY**
- **Issue:** The `flightFieldOptions[]` static array in WebPortal.cpp is identical to the list of content values the FlightFieldWidget actually supports. If a new field is added to FlightFieldWidget, the options array must be manually updated in WebPortal. No automatic sync.
- **Evidence:**
  - File: `firmware/adapters/WebPortal.cpp` line 2140-2143
  - File: `firmware/widgets/FlightFieldWidget.h` (no options list, only function signature)
  - Sync point: None. Manual edit required in two places.
  - Risk: Developer adds new field to FlightFieldWidget, forgets to update WebPortal options → editor has no dropdown option
- **Impact:** DRY violation = future maintenance burden and error-prone
- **Fix:** Document in story antipattern table or add a TODO note:
  ```cpp
  // ⚠️ TODO (LE-1.8): Flight field options hard-coded here AND in FlightFieldWidget.cpp.
  // If new fields are added to FlightFieldWidget::resolveField(), must also update
  // this options array. Consider extracting to a shared constant or adding a compile-time check.
  static const char* const flightFieldOptions[] = {
      "airline", "ident", "origin_icao", "dest_icao", "aircraft"
  };
  ```

### 13. **MEDIUM: MetricWidget "vsi" Decimal Precision Inconsistent with Standard Aviation Display**
- **Issue:** Story specifies vertical rate formats as "1 decimal" (e.g., "12.5 fps"), but most aviation display units show vertical rate as integer feet per second (integer, no decimals). The 1-decimal choice is undocumented and may cause confusion with flight crew familiar with standard displays.
- **Evidence:**
  - AC #4: "vertical_rate_fps" formatted with 1 decimal as example
  - Standard aviation: VSI typically shows integer feet-per-minute (or integer feet-per-second on digital displays)
  - Story decision: No rationale provided for 1 decimal choice
- **Impact:** Low severity but affects UX consistency with pilot expectations
- **Fix:** Document the design choice in the Dev Notes or story comments:
  ```cpp
  // ℹ️ Note: Vertical rate formatted as 1 decimal (e.g., "12.5 fps") rather than
  // integer per aviation standard VSI displays. Chosen for finer-grained LED display feedback.
  // If this causes integration confusion, reconsider reverting to 0 decimals ("13 fps").
  ```

### 14. **MEDIUM: FlightFieldWidget Aircraft Fallback Chain Silently Truncates**
- **Issue:** `aircraft_display_name_short` can be longer than expected, and if it exceeds the widget's width, `DisplayUtils::truncateToColumns()` silently cuts it off mid-word. The truncation is undocumented in AC or task.
- **Evidence:**
  - File: `firmware/widgets/FlightFieldWidget.cpp` lines 62-67
  - Code: Falls back to `aircraft_display_name_short`, then calls `truncateToColumns()` without warning
  - Example: "Boeing 747-8" in a 2-column widget → renders as "Bo" (ugly)
  - Story AC: No mention of truncation or expected behavior
- **Impact:** UX surprise: incomplete aircraft type names on small widgets
- **Fix:** Document in AC or Dev Notes:
  ```
  AC #1 clarification: "aircraft" field displays aircraft_display_name_short truncated to 
  widget width. On narrow widgets (<24px), expect truncated names (e.g., "Bo" for "Boeing 747-8").
  ```

### 15. **MEDIUM: WidgetRegistry Three-List Sync Bomb Acknowledged But Not Fixed**
- **Issue:** The story's own antipatterns table documents the "CRITICAL" sync risk between `kAllowedWidgetTypes[]` in LayoutStore, `WidgetRegistry::fromString()`, and `WebPortal::_handleGetWidgetTypes()`. The story acknowledges the problem but defers fixing it to a "future story," leaving a known tech debt bomb in place.
- **Evidence:**
  - Story Dev Notes antipatterns: "Three-list sync rule reminder... must stay in lock-step"
  - Dev Notes: "Centralization is a valid future improvement but is explicitly out of LE-1.8 scope"
  - Reality: The sync point remains manual, error-prone, and undocumented in code comments
- **Impact:** Future story that adds a widget type (e.g., "schedule_widget") will forget to update one of the three lists → silent corruption
- **Fix:** Add compile-time or runtime assertions (out of LE-1.8 scope, but document for next story):
  ```cpp
  // ⚠️ [AI-Review] TODO (LE-1.9+): Unify widget type registry
  // Currently duplicated in three places, prone to sync errors:
  //   1. core/LayoutStore.cpp → kAllowedWidgetTypes[]
  //   2. core/WidgetRegistry.cpp → fromString() switch
  //   3. adapters/WebPortal.cpp → _handleGetWidgetTypes() response
  // Consolidate into a single source of truth with compile-time/runtime assertions.
  ```

### 16. **MEDIUM: AC #7 Editor Dropdown Population Not Explained**
- **Issue:** Story AC #7 specifies editor.js `showPropsPanel()` must render content field as dropdown populated from `options[]` in the API metadata. However, no explanation of what happens if API returns zero options (e.g., malformed response or future widget with no options). Silent fallback to text input? Error?
- **Evidence:**
  - Story AC #7: "content field renders as <select> dropdown populated from options[]"
  - No AC clause for: "when options[] is null/empty"
  - Editor implementation: Likely renders nothing or reverts to text input (unspecified behavior)
- **Impact:** Undefined behavior in edge cases = integration test surprise
- **Fix:** Update AC #7:
  ```
  AC #7 clarification: If API response has no options array or options array is empty,
  fall back to text input (<input type="text">) instead of <select>.
  ```

### 17. **MEDIUM: No Acceptance Criteria for WebPortal `GET /api/widgets/types` Response Completeness**
- **Issue:** Story adds new AC #6 requirement that `GET /api/widgets/types` includes correct options for flight_field and metric widgets. However, AC #6 doesn't specify what happens if a future widget type is added but the response schema isn't updated. No versioning, no optional fields, no schema evolution strategy.
- **Evidence:**
  - Story AC #6: "Neither [flight_field nor metric] entry contains 'note': 'LE-1.8 — not yet implemented'"
  - What's missing: "If a new widget type is added, ensure /api/widgets/types response includes it with correct options"
  - Schema contract: Unversioned, no `schema_version` field in response
- **Impact:** Future widget additions could silently break the editor if the API endpoint isn't updated
- **Fix:** Add AC clause:
  ```
  AC #6 Schema Completeness: GET /api/widgets/types response MUST include all widget types
  registered in core/WidgetRegistry.h. Add runtime assertion (or compile-time check if possible)
  that no widget type is missing from the response.
  ```

---

## 📋 Summary of All Issues by Severity

| Severity | Count | Issues |
|----------|-------|--------|
| 🔴 CRITICAL | 4 | AC contradictions, task lies, degree symbol bug, build gate unverified |
| 🟠 HIGH | 5 | Missing default initializer, AC deferral not updated, undocumented fallback, test coverage gaps, coupling |
| 🟡 MEDIUM | 8 | Null guard assertion, NAN fallback coupling, options array DRY, decimal precision, truncation, sync bomb, dropdown edge case, schema evolution |
| **TOTAL** | **17** | **MAJOR REWORK REQUIRED** |

---

## 🎯 Evidence Score Calculation

| Category | Points | Rationale |
|----------|--------|-----------|
| CRITICAL: AC text vs code mismatch | +3 | Story integrity failure |
| CRITICAL: Task completion lies (tests) | +3 | Unverifiable acceptance |
| CRITICAL: Degree symbol bug | +3 | Functional bug |
| CRITICAL: Build gate unverified | +2 | AC gate not proven |
| HIGH: Missing default initializer | +1 | Latent memory safety |
| HIGH: AC deferral not updated | +1 | Documentation debt |
| HIGH: Undocumented fallbacks (2) | +2 | Hidden behavior |
| HIGH: Test coverage gaps (2) | +1 | Incomplete validation |
| MEDIUM: 8 issues × 0.3 each | +2.4 | Minor gaps |
| **Subtotal** | **18.4** | |
| Clean categories: 0 | -0 | No clean passes |
| **Evidence Score** | **18.4** | **MAJOR REWORK** |

**Verdict: REJECT** — Multiple critical issues (AC text doesn't match code, task completion lies, functional bugs) require substantial rework before approval.

---

## ✅ Required Actions Before Re-review

1. **Fix the degree symbol byte** (issue #4) — Change 0xF8 to 0xF7
2. **Add default initializer** (issue #5) — `= nullptr` for currentFlight
3. **Update AC #1 documentation** (issue #1) — Clarify abbreviated field names match implementation
4. **Execute build verification** (issue #8) — Run pio and document binary size
5. **Enhance unit tests** (issues #3, #9, #10) — Replace lying tests with real assertions
6. **Document undocumented behaviors** (issues #6, #7, #12, #13, #14) — Update story AC or code comments

---

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**ADVERSARIAL CODE REVIEW COMPLETE, Christian.**

**Story:** `_bmad-output/implementation-artifacts/stories/le-1-8-flight-field-and-metric-widgets.md`  
**Scope:** Product code under `firmware/` as embedded in review bundle (widgets, `DisplayMode.h`, `CustomLayoutMode.cpp`, `WebPortal.cpp`, `WidgetRegistry.*`, `editor.js`, `LayoutStore.*`). Unity tests and `editor.js.gz` / build artifacts were **not** fully present in the prompt corpus; several findings are therefore **contract / traceability / UX** issues with explicit verification gaps.

---

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 (gzip + smoke/tests likely touched; story “File List” is narrow) |
| AC Implementation Gaps | 3 (AC #1/#2/#4 literal text vs shipped keys; AC #5 cache clause; AC #12 unverified here) |
| Task Completion Lies | 0 (no proof tasks are falsely checked without CI logs; **Task 11 remains evidence-starved**) |
| SOLID / Boundary | 2 (triple source of truth; `RenderContext` grows again) |
| Hidden Bugs / UX | 2 (`dirty` not set on select coercion; `flights[0]` contract assumption) |
| Performance | 1 (AC #5 value-equality cache explicitly deferred → sustained `snprintf`/strcmp work per widget per frame) |
| Security | 0 new meaningful LAN threat beyond existing portal model |
| **Total Issues** | **10** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| IMPORTANT | `showPropsPanel` coerces invalid `content` to a valid `<select>` value but does **not** set `dirty = true`, so the operator can believe the layout was “repaired” while disk JSON and in-app `dirty` state disagree until some unrelated edit. | `firmware/data-src/editor.js` (coercion block after `contentSelect.value = coerced`) | +1.0 |
| IMPORTANT | **Three independent string tables** for the same vocabulary (`WebPortal::_handleGetWidgetTypes` option arrays vs `FlightFieldWidget::resolveField` vs `MetricWidget::resolveMetric`) — classic sync bomb; drift produces “saved valid layout” that renders wrong field/metric. | `WebPortal.cpp`, `FlightFieldWidget.cpp`, `MetricWidget.cpp` | +1.0 |
| IMPORTANT | **Acceptance criteria table is not authoritative** for LE-1.8: AC #1/#4 still describe the *pre-ship* vocabulary (`ident_icao`, `altitude_kft`, …) while firmware + `_handleGetWidgetTypes()` ship the **canonical 5 + 4** short keys. Any reviewer or tool that gates on AC text will false-fail or false-pass. | Story markdown AC #1/#4 vs `WebPortal::_handleGetWidgetTypes` | +1.0 |
| MINOR | `CustomLayoutMode` documents that `flights[0]` is the orchestrator’s “front-of-queue” flight. If any caller ever passes a vector where index `0` is not the active flight (or passes multiple flights without that invariant), widgets silently bind to the wrong row. **Not disproven** from bundle alone — needs `main.cpp` / pipeline trace. | `firmware/modes/CustomLayoutMode.cpp` | +0.3 |
| MINOR | `MetricWidget.cpp` uses `strcpy` for constant `"--"` (safe today) but violates the project’s general “bounded writes everywhere” hygiene; `memcpy`/`strlcpy`/`snprintf` would be consistent with the rest of the embedded style. | `firmware/widgets/MetricWidget.cpp` | +0.3 |
| MINOR | `WidgetRegistry.h` header still claims renderers may use **function-scoped statics for caching**; LE-1.8 explicitly deferred caching. Stale guidance misleads the next contributor into reintroducing global mutable widget state. | `firmware/core/WidgetRegistry.h` | +0.3 |
| MINOR | AC #5 **value-equality cache** is formally deferred in story prose, but that means the **letter of AC #5 is not met**—only the zero-heap slice is met. Flag for release-notes / AC amendment, not silent closure. | Story + widgets | +0.3 |
| MINOR | AC #12 (binary ≤ 1,382,400 bytes) cannot be validated from the embedded transcript; if CI is not pinned, this is a **process hole**. | Build artifact / `pio run` log (missing here) | +0.3 |
| MINOR | AC #2 asked for U+2014 for `nullptr` flight; `FlightFieldWidget` uses `"--"` for null/unknown/empty (per header comment). That is a deliberate product choice but **AC literal mismatch**. | `firmware/widgets/FlightFieldWidget.cpp` | +0.3 |
| MINOR | Original AC #3 (“telemetry NAN → `formatTelemetryValue` in **FlightField**”) no longer maps 1:1 because telemetry moved to `MetricWidget`; readers of AC-only traceability will be misled. | Story AC #3 vs split implementation | +0.3 |

**Clean passes (categories with no *new* substantive issue in this story slice):**  
- **Security** (no new exposure beyond always-on LAN portal) → −0.5  
- **SOLID** (no egregious new god-classes; issues are mostly coupling/sync) → −0.5  
- **Type safety** (C++11; no new `auto` foot-guns introduced in shown code) → −0.5  

**Raw sum:** +4.8  
**Clean adjustments:** −1.5  

### Evidence Score: **3.3**

| Score | Verdict |
|-------|---------|
| **3.3** | **MAJOR REWORK** (documentation + one real UX/state bug + sync risk; not a wholesale rewrite of firmware math) |

---

## Architectural / Maintainability Sins

- **[7/10] Open/Closed + DRY failure (sync bomb):** Every new flight-field key or metric requires coordinated edits in **three** places (`WebPortal` JSON schema, `FlightFieldWidget`, `MetricWidget`). This violates the spirit of the story’s own “three-list sync rule” comment in `WebPortal.cpp`—the rule is acknowledged, not solved.
- **[5/10] Interface pollution:** `RenderContext` now carries `currentFlight`, which is correct for LE-1.8, but it further enlarges a cross-mode aggregate. Any future field will tempt more “just one more pointer.” Not wrong for this story, but the slope is real.

---

## Pythonic Crimes & Readability

- N/A (firmware C++ + ES5). Minor C++ nit: prefer bounded copies over `strcpy` in embedded code for consistency, even when currently safe.

---

## Performance & Scalability

- **[Medium] Per-frame reformat:** With caching explicitly deferred, each visible `flight_field` / `metric` widget pays full `strcmp` + `formatTelemetryValue` + truncation **every** `render()` at display cadence. Probably fine at 2–4 widgets, but this is exactly the footgun the deferred AC #5 text was trying to prevent for denser layouts.

---

## Correctness & Safety

- **Bug (editor state):** Missing `dirty = true` when `widgets[selectedIndex].content` is coerced to match `<select>` options means **UI shows canonical value while `dirty` may still be false**; user leaves editor without saving → stale invalid keys remain on device. This is more serious than pure cosmetics.
- **Contract risk:** `CustomLayoutMode` assumes `flights[0]` semantics. Needs a single authoritative comment + assertion in the producer (`main`/orchestrator path), not only in the consumer.

---

## Lying / Weak Tests (risk register)

- The story mandates heap-delta tests with ±64 byte slack and many “returns true” smoke paths. Without the actual `firmware/test/test_flight_field_widget/test_main.cpp` and `test_metric_widget/test_main.cpp` in this bundle, the review **cannot** grade assertion strength. **Action:** spot-check those tests for (a) framebuffer assertions absent by design vs (b) assertions that never fail.

---

## Suggested Fixes (priority order)

1. **`editor.js`:** After coercing `widgets[selectedIndex].content`, set `dirty = true` (and optionally toast once) so persistence state matches operator perception.
2. **Documentation hygiene:** Rewrite AC #1/#2/#3/#4 in the story to match **shipped** keys and placeholders, or add a single “AC superseded by Dev Notes canonical table” banner at the top of the AC section.
3. **Tech debt reduction (choose one):**
   - Promote canonical option arrays to **one** `PROGMEM`/`constexpr` table in a tiny `widgets/WidgetContentCatalog.h` consumed by both `WebPortal` and the widget `.cpp` files, **or**
   - Codegen from one JSON at build time (heavier).
4. **`CustomLayoutMode` / producer:** Add a one-line `assert`/`LOG_V` guard documenting the invariant that `flights` is either empty or `[0]` is the active flight for this mode.
5. **`WidgetRegistry.h`:** Update the caching sentence to reflect LE-1.8 reality (stack-only renderers; caching deferred).
6. **Build gate:** Attach `pio run` size line to story or CI artifact to close AC #12 evidentially.

---

**Review Actions**

- Issues Found: **10**  
- Issues Fixed: **0** (read-only review)  
- Action Items Created: **6** (above)

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/le-1-8-flight-field-and-metric-widgets.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **3** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | AC #5 Caching is formally deferred, yet the story's Dev Notes and AC still reference it. This creates a documentation/expectations mismatch for future maintainers. | Story/Dev Notes | +1 |
| 🟠 IMPORTANT | SOLID: `WidgetRegistry` depends on `FlightFieldWidget` and `MetricWidget`. This violates the Dependency Inversion Principle — adding a widget requires modifying the registry. | WidgetRegistry.cpp | +1 |
| 🔴 CRITICAL | AC #6/7 Requirement Conflict: `editor.js` property panel integration relies on `widgetTypeMeta` which is built during `initToolbox()` from `GET /api/widgets/types`. If the API schema changes (e.g., adding a new widget type), the `showPropsPanel` logic needs to be updated. The current implementation is fragile. | editor.js | +3 |

### Evidence Score: 5.0

| Score | Verdict |
|-------|---------|
| **5.0** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[6/10] Dependency Inversion Principle Violation:** `WidgetRegistry` is explicitly coupled to `FlightFieldWidget` and `MetricWidget`.
  - 📍 `firmware/core/WidgetRegistry.cpp:16`
  - 💡 Fix: Implement a registration-based pattern (e.g., `WidgetRegistry::register(type, factory)`) where widgets register themselves on boot.

---

## 🐛 Correctness & Safety

- **🐛 Bug: editor.js select-sync:** `showPropsPanel` sets `widgets[selectedIndex].content` when coerced, but does not trigger a UI refresh of the rest of the props panel, nor does it mark the layout as `dirty` if the coercion happened during load/initialization.
  - 📍 `firmware/data-src/editor.js:397`
  - 🔄 Reproduction: Open a layout with an invalid/stale widget content field. The props panel will show the coerced (valid) value, but the underlying JSON object `widgets[]` array will not contain this coerced value until the user manually triggers an `input` event on the dropdown, making the saved layout technically corrupted/stale.

---

## 🛠️ Suggested Fixes

### 1. Registry Registration Pattern (SOLID)

**File:** `firmware/core/WidgetRegistry.cpp`
**Issue:** Static coupled dispatch pattern.

**Diff:**
```diff
- #include "widgets/FlightFieldWidget.h"
- #include "widgets/MetricWidget.h"
+ // Use a registration-based pattern instead.
```
*Correction: Implement a `registerWidget(type, renderFunc)` method in `WidgetRegistry` and call it from each widget's constructor or a static registration block.*

---

**Review Actions:**
- Issues Found: 3
- Issues Fixed: 0
- Action Items Created: 0

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
<var name="session_id">10a8be82-bf8f-43a1-85a0-6b164d049d1d</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="41af4780">embedded in prompt, file id: 41af4780</var>
<var name="story_id">le-1.8</var>
<var name="story_key">le-1-8-flight-field-and-metric-widgets</var>
<var name="story_num">8</var>
<var name="story_title">8-flight-field-and-metric-widgets</var>
<var name="template">False</var>
<var name="timestamp">20260417_1807</var>
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