
# Story LE-1.8: Flight Field and Metric Widgets

branch: le-1-8-flight-field-and-metric-widgets
zone:
  - firmware/widgets/FlightFieldWidget.*
  - firmware/widgets/MetricWidget.*
  - firmware/core/WidgetRegistry.*
  - firmware/test/test_flight_field_widget/**
  - firmware/test/test_metric_widget/**

Status: draft

## Story

As a **layout author**,
I want **widgets that bind to current flight fields (callsign, altitude, speed, route) and to generic numeric metrics (e.g., weather)**,
So that **my custom layout displays live data, not just static text**.

## Acceptance Criteria

1. **Given** a `FlightFieldWidget` spec with `field = "callsign" | "altitude_kft" | "speed_mph" | "route" | "aircraft_type"` **When** dispatched **Then** the widget reads the field from the `RenderContext.currentFlight` and renders via `DisplayUtils::drawTextLine`.
2. **Given** no current flight **When** dispatched **Then** the widget renders "—" (em dash) fallback.
3. **Given** a `MetricWidget` spec with `key = "wind_kt" | "temp_f" | ...` **When** dispatched **Then** the widget reads the named metric from `RenderContext.apiContext` **And** renders via `drawTextLine`.
4. **Given** a `format` field on either widget (e.g., `"%d kt"`, `"%.1f°"`) **When** dispatched **Then** the value is formatted per the printf-style string; missing `format` defaults to `%s` for strings, `%d` for ints, `%.1f` for floats.
5. **Given** both widget types **When** invoked repeatedly **Then** the rendered string is recomputed only when the source value changes (cached similarly to ClockWidget) **And** no heap allocation occurs per render.
6. **Given** `GET /api/widgets/types` **When** returned **Then** the response includes field-level introspection for `flight_field` (enum of allowed field names) and `metric` (list of known metric keys), enabling editor dropdowns.
7. **Given** unit tests **When** `pio test -f test_flight_field_widget` and `test_metric_widget` run **Then** cases pass for: field mapping, format string handling, fallback behavior, no-alloc per render (assert via instrumented heap counter).

## Tasks / Subtasks

- [ ] **Task 1: ApiContext shared struct** (AC: #3)
  - [ ] Add `ApiContext` struct (in an existing header such as `interfaces/DisplayMode.h` or a new `core/ApiContext.h`) with named numeric/string metrics populated by Core-1 fetchers
  - [ ] Thread-safe read pattern: `std::atomic` or a double-buffered snapshot populated on Core-1 and read on Core-0

- [ ] **Task 2: FlightFieldWidget** (AC: #1, #2, #4, #5)
  - [ ] `firmware/widgets/FlightFieldWidget.h` / `.cpp` — render free fn
  - [ ] Field dispatch via small switch on enum; format per `spec.format`
  - [ ] Cache last rendered string; recompute only when source value changes (hash by pointer or value snapshot)

- [ ] **Task 3: MetricWidget** (AC: #3, #4, #5)
  - [ ] `firmware/widgets/MetricWidget.h` / `.cpp` — render free fn
  - [ ] Look up metric by name from `ApiContext`; same caching pattern

- [ ] **Task 4: Register in WidgetRegistry** (AC: all)
  - [ ] `firmware/core/WidgetRegistry.cpp` — add `WidgetType::FlightField` and `WidgetType::Metric` to switch/table
  - [ ] `WidgetRegistry::describeTypes()` (LE-1.4) extended to advertise field/metric options

- [ ] **Task 5: Unit tests** (AC: #7)
  - [ ] `firmware/test/test_flight_field_widget/test_main.cpp` — each field maps correctly; fallback on null flight; format strings respected
  - [ ] `firmware/test/test_metric_widget/test_main.cpp` — metric lookup; missing metric fallback; format strings
  - [ ] Heap-alloc assertion: snapshot `ESP.getFreeHeap()` before and after 1000 dispatch iterations; delta == 0 (or within noise)

- [ ] **Task 6: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] `pio test -f test_flight_field_widget --without-uploading --without-testing` passes
  - [ ] `pio test -f test_metric_widget --without-uploading --without-testing` passes

## Dev Notes

**Depends on:** LE-1.2 (WidgetRegistry), LE-1.4 (widget type introspection endpoint).

**Validated spike patterns** (`_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`): cache time-derived / API-derived state; recompute only on change; fixed array, no per-frame alloc.

**Currently-displayed-flight source.** Existing flight rotation logic in `ModeOrchestrator` or the flight queue exposes the currently-displayed flight. Expose a const pointer in `RenderContext` so widgets never mutate.

**Format string safety.** Use `snprintf` into a fixed stack buffer (e.g., `char buf[32]`). Never pass user-supplied format strings that could include `%s` aliasing unintended memory — validate format at `init()` / `save()` schema check (LE-1.1) to allow only a whitelist of format specifiers.

**Example format values:** `"%d kt"`, `"%.1f°F"`, `"%s"`, `"ALT %d"`.

## File List

- `firmware/widgets/FlightFieldWidget.h` (new)
- `firmware/widgets/FlightFieldWidget.cpp` (new)
- `firmware/widgets/MetricWidget.h` (new)
- `firmware/widgets/MetricWidget.cpp` (new)
- `firmware/core/WidgetRegistry.h` (modified)
- `firmware/core/WidgetRegistry.cpp` (modified)
- `firmware/core/ApiContext.h` (new — or extension of existing header)
- `firmware/test/test_flight_field_widget/test_main.cpp` (new)
- `firmware/test/test_metric_widget/test_main.cpp` (new)
