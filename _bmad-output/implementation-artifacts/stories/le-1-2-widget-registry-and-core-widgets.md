
# Story LE-1.2: WidgetRegistry and Core Widgets

branch: le-1-2-widget-registry-and-core-widgets
zone:
  - firmware/core/WidgetRegistry.*
  - firmware/widgets/**
  - firmware/test/test_widget_registry/**

Status: draft

## Story

As a **firmware engineer**,
I want **a static widget registry that dispatches by widget type to stateless render fns**,
So that **adding new widget types is a single-table change and the render hot path stays allocation-free**.

## Acceptance Criteria

1. **Given** a `WidgetSpec` with a known type **When** `WidgetRegistry::dispatch(type, spec, ctx)` is called **Then** the matching render function is invoked **And** returns `true` on successful render.
2. **Given** an unknown type identifier **When** `WidgetRegistry::isKnownType("foo")` is called **Then** it returns `false` **And** `dispatch()` for an unknown type returns `false` without touching the framebuffer.
3. **Given** the dispatch implementation **When** inspected **Then** it uses a `switch` on `WidgetType` enum or a fixed `std::array<Entry, N>` lookup — **not** a virtual-function vtable per widget.
4. **Given** a `TextWidget` spec with `value`, `align` (left/center/right), `color` **When** dispatched **Then** text renders via `DisplayUtils::drawTextLine` with `truncateToColumns` applied for width overflow **And** supports all three alignments.
5. **Given** a `ClockWidget` spec **When** dispatched **Then** HH:MM is recomputed only when `millis() - lastUpdateMs > 30000` (minute-resolution cache, validated pattern from spike) **And** subsequent frames reuse the cached string.
6. **Given** a `LogoWidget` V1 stub **When** dispatched **Then** a fixed-color 16×16 bitmap is blit via `DisplayUtils::drawBitmapRGB565` — **not** `fillRect` (spike measured 2.67× baseline with fillRect, 1.23× with bitmap blit).
7. **Given** widget minimum-dimension floors **When** a spec has `w` or `h` below the floor **Then** the render fn clips safely and does not write out-of-bounds (text min height = font size; clock min = 8×6; logo min = 8×8).
8. **Given** unit tests **When** `pio test -f test_widget_registry` runs **Then** cases pass for: dispatch-by-type, unknown-type returns false, bounds clipping, clock cache prevents repeated `getLocalTime()` calls (use a mockable time source or counter).

## Tasks / Subtasks

- [ ] **Task 1: Define WidgetType enum and registry header** (AC: #1, #2, #3)
  - [ ] `firmware/core/WidgetRegistry.h` — `enum class WidgetType { Text, Clock, Logo, FlightField, Metric, Unknown }`
  - [ ] Declare `WidgetSpec` struct (x, y, w, h, color, type-specific fields via union or variant fields)
  - [ ] Declare `RenderContext` struct (pointer to display adapter, current flight, api context, frame millis)
  - [ ] `bool dispatch(WidgetType, const WidgetSpec&, const RenderContext&)`
  - [ ] `bool isKnownType(const char*)` + `WidgetType fromString(const char*)`

- [ ] **Task 2: Implement registry dispatch** (AC: #1, #2, #3)
  - [ ] `firmware/core/WidgetRegistry.cpp` — `switch (type)` over enum, call free function per widget
  - [ ] No virtual dispatch; no heap allocation

- [ ] **Task 3: TextWidget** (AC: #4, #7)
  - [ ] `firmware/widgets/TextWidget.h` / `.cpp` — `bool renderText(const WidgetSpec&, const RenderContext&)`
  - [ ] Use `DisplayUtils::drawTextLine` and `DisplayUtils::truncateToColumns`
  - [ ] Support align = left | center | right
  - [ ] Respect min height = font size

- [ ] **Task 4: ClockWidget with 30 s cache** (AC: #5, #7)
  - [ ] `firmware/widgets/ClockWidget.h` / `.cpp` — static (per-instance) `lastUpdateMs` and `cachedBuf[6]`
  - [ ] Recompute only when `millis() - lastUpdateMs > 30000`
  - [ ] Min dim 8×6; fallback `--:--` when time not synced

- [ ] **Task 5: LogoWidget V1 stub** (AC: #6, #7)
  - [ ] `firmware/widgets/LogoWidget.h` / `.cpp` — draw fixed-color 16×16 RGB565 buffer via `DisplayUtils::drawBitmapRGB565`
  - [ ] Min dim 8×8; LE-1.5 replaces the stub with real LittleFS-backed RGB565 pipeline
  - [ ] **Must not** call `fillRect` (per spike finding)

- [ ] **Task 6: Unit tests** (AC: #8)
  - [ ] `firmware/test/test_widget_registry/test_main.cpp` — dispatch returns true for known types
  - [ ] test: unknown type returns false + framebuffer untouched
  - [ ] test: bounds clipping when w/h at floor or exceed canvas edges
  - [ ] test: clock cache counter — invoke dispatch 100× within 1 s, verify recompute count == 1
  - [ ] Mock or inject a time source to observe cache behavior

- [ ] **Task 7: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] `pio test -f test_widget_registry --without-uploading --without-testing` passes
  - [ ] Grep: no `fillRect` calls inside `firmware/widgets/` for the logo path

## Dev Notes

**Validated patterns from V0 spike** (`_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`):

- **Bitmap blit, not fillRect.** Spike measured 2.67× baseline with `fillRect(32x32)`, dropped to 1.23× after swapping to `drawBitmapRGB565` of a smaller bitmap-like pattern. **`LogoWidget` must use `drawBitmapRGB565`.**
- **Cache clock at minute resolution.** Spike showed that moving `getLocalTime()` out of the per-frame path was essential.
- **Static dispatch.** Virtual function per widget was prototyped and rejected; switch/table is leaner.
- **Fixed widget array, cap ~24.** No heap in the render hot path.

**Existing utilities to reuse:** `firmware/utils/DisplayUtils.{h,cpp}` provides `drawTextLine`, `drawBitmapRGB565`, `truncateToColumns`.

**Follow-on stories:**
- LE-1.5 replaces the LogoWidget V1 stub with real RGB565-from-LittleFS loading.
- LE-1.8 adds `FlightFieldWidget` and `MetricWidget` under the same registry.

## File List

- `firmware/core/WidgetRegistry.h` (new)
- `firmware/core/WidgetRegistry.cpp` (new)
- `firmware/widgets/TextWidget.h` (new)
- `firmware/widgets/TextWidget.cpp` (new)
- `firmware/widgets/ClockWidget.h` (new)
- `firmware/widgets/ClockWidget.cpp` (new)
- `firmware/widgets/LogoWidget.h` (new — V1 stub)
- `firmware/widgets/LogoWidget.cpp` (new — V1 stub)
- `firmware/test/test_widget_registry/test_main.cpp` (new)
