<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T180942Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story le-1.2

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
<file id="bd2de04b" path="_bmad-output/implementation-artifacts/stories/le-1-2-widget-registry-and-core-widgets.md" label="DOCUMENTATION"><![CDATA[


# Story LE-1.2: WidgetRegistry and Core Widgets

branch: le-1-2-widget-registry-and-core-widgets
zone:
  - firmware/core/WidgetRegistry.*
  - firmware/widgets/**
  - firmware/test/test_widget_registry/**

Status: ready-for-review

---

## Story

As a **firmware engineer**,
I want **a static widget registry that dispatches by widget type to stateless render functions**,
So that **adding new widget types is a single-table change and the render hot path stays allocation-free**.

---

## Acceptance Criteria

1. **Given** a `WidgetSpec` with a known type **When** `WidgetRegistry::dispatch(type, spec, ctx)` is called **Then** the matching render function is invoked **And** returns `true` on successful render.

2. **Given** an unknown type identifier **When** `WidgetRegistry::isKnownType("foo")` is called **Then** it returns `false` **And** `dispatch()` for an unknown type returns `false` without touching the framebuffer.

3. **Given** the dispatch implementation **When** inspected **Then** it uses a `switch` on `WidgetType` enum or a fixed `std::array<Entry, N>` lookup — **not** a virtual-function vtable per widget.

4. **Given** a `TextWidget` spec with `value`, `align` (left/center/right), `color` **When** dispatched **Then** text renders via `DisplayUtils::drawTextLine` with `truncateToColumns` applied for width overflow **And** supports all three alignments.

5. **Given** a `ClockWidget` spec **When** dispatched **Then** HH:MM is recomputed only when `millis() - lastUpdateMs > 30000` (minute-resolution cache, validated pattern from spike) **And** subsequent frames reuse the cached string.

6. **Given** a `LogoWidget` V1 stub **When** dispatched **Then** a fixed-color 16×16 bitmap is blit via `DisplayUtils::drawBitmapRGB565` — **not** `fillRect` (spike measured 2.67× baseline with fillRect, 1.23× with bitmap blit).

7. **Given** widget minimum-dimension floors **When** a spec has `w` or `h` below the floor **Then** the render fn clips safely and does not write out-of-bounds (text min height = font size; clock min = 8×6; logo min = 8×8).

8. **Given** unit tests **When** `pio test -f test_widget_registry` runs **Then** cases pass for: dispatch-by-type, unknown-type returns false, bounds clipping, clock cache prevents repeated `getLocalTime()` calls (use a mockable time source or counter).

---

## Tasks / Subtasks

- [x] **Task 1: Define WidgetType enum and WidgetRegistry header** (AC: #1, #2, #3)
  - [x] Create `firmware/core/WidgetRegistry.h` with `#pragma once`
  - [x] Define `enum class WidgetType : uint8_t { Text = 0, Clock, Logo, FlightField, Metric, Unknown = 0xFF }`
  - [x] Define `WidgetSpec` struct (see Dev Notes for exact field layout)
  - [x] Declare `bool WidgetRegistry::dispatch(WidgetType type, const WidgetSpec& spec, const RenderContext& ctx)`
  - [x] Declare `bool WidgetRegistry::isKnownType(const char* typeStr)`
  - [x] Declare `WidgetType WidgetRegistry::fromString(const char* typeStr)`

- [x] **Task 2: Implement WidgetRegistry dispatch** (AC: #1, #2, #3)
  - [x] Create `firmware/core/WidgetRegistry.cpp`
  - [x] Implement `fromString()` using a sequence of `strcmp` comparisons — same pattern as `parseType()` in `CustomLayoutMode.cpp` but expanded to 5 known types
  - [x] Implement `isKnownType()` via `fromString() != WidgetType::Unknown`
  - [x] Implement `dispatch()` via `switch (type)` calling one free function per widget type (no virtual dispatch, no heap allocation)
  - [x] Unknown type case: return `false` without calling any render fn and without any matrix writes

- [x] **Task 3: TextWidget** (AC: #4, #7)
  - [x] Create `firmware/widgets/TextWidget.h` with `#pragma once` — declare `bool renderText(const WidgetSpec& spec, const RenderContext& ctx)`
  - [x] Create `firmware/widgets/TextWidget.cpp` — implement using `DisplayUtils::truncateToColumns` (char* overload) and `DisplayUtils::drawTextLine` (char* overload)
  - [x] Support `align` field: `left` = x as-is, `center` = offset by `(w - textPixels) / 2`, `right` = offset by `w - textPixels`
  - [x] Enforce min height: if `spec.h < CHAR_H` (8px), clip — do not write below `spec.y + spec.h`
  - [x] Vertically center single text line within widget height: `y_draw = spec.y + (spec.h > CHAR_H ? (spec.h - CHAR_H) / 2 : 0)`

- [x] **Task 4: ClockWidget with 30s cache** (AC: #5, #7)
  - [x] Create `firmware/widgets/ClockWidget.h` with `#pragma once` — declare `bool renderClock(const WidgetSpec& spec, const RenderContext& ctx)`
  - [x] Create `firmware/widgets/ClockWidget.cpp` — implement with `static char s_clockBuf[8]` and `static uint32_t s_lastClockUpdateMs`
  - [x] Refresh only when `s_lastClockUpdateMs == 0 || (millis() - s_lastClockUpdateMs) > 30000`
  - [x] Time format: `snprintf(s_clockBuf, sizeof(s_clockBuf), "%02d:%02d", tinfo.tm_hour, tinfo.tm_min)` — 24h only for V1
  - [x] Fallback to `"--:--"` when `getLocalTime(&tinfo, 0)` returns false
  - [x] Enforce min dimensions: if `spec.w < 6*CHAR_W` or `spec.h < CHAR_H`, skip render (return `true` — not an error)
  - [x] Use `DisplayUtils::truncateToColumns` (char* overload) and `DisplayUtils::drawTextLine` (char* overload)

- [x] **Task 5: LogoWidget V1 stub** (AC: #6, #7)
  - [x] Create `firmware/widgets/LogoWidget.h` with `#pragma once` — declare `bool renderLogo(const WidgetSpec& spec, const RenderContext& ctx)`
  - [x] Create `firmware/widgets/LogoWidget.cpp` — V1 stub using a static 16×16 RGB565 bitmap buffer (not `fillRect`)
  - [x] Declare `static const uint16_t kLogoPlaceholder[16 * 16]` in anonymous namespace — fill with spec color (computed once at call time)
  - [x] Call `DisplayUtils::drawBitmapRGB565(ctx.matrix, spec.x, spec.y, 16, 16, kLogoPlaceholder, spec.w, spec.h)` — zoneW/zoneH = spec.w/h for clipping
  - [x] **Must NOT call `fillRect`** — AC #6 and antipattern from spike measurements
  - [x] Enforce min dim: if `spec.w < 8` or `spec.h < 8`, return `true` without drawing
  - [x] Add comment: `// V1 stub — LE-1.5 replaces with real RGB565 bitmap from LittleFS via LogoManager`

- [x] **Task 6: Update platformio.ini build_src_filter** (AC: #8 — build gate)
  - [x] Add `+<../widgets/*.cpp>` to the `build_src_filter` list in `firmware/platformio.ini`
  - [x] Add `-I widgets` to the `build_flags` list in `firmware/platformio.ini`
  - [x] Verify both `esp32dev_le_baseline` and `esp32dev_le_spike` envs inherit the change via `extends`

- [x] **Task 7: Unity tests** (AC: #8)
  - [x] Create `firmware/test/test_widget_registry/test_main.cpp` (see Dev Notes for exact structure)
  - [x] `test_dispatch_text_returns_true` — dispatch WidgetType::Text with valid spec, assert return value is `true`
  - [x] `test_dispatch_clock_returns_true` — dispatch WidgetType::Clock with valid spec
  - [x] `test_dispatch_logo_returns_true` — dispatch WidgetType::Logo with spec w/h ≥ 16
  - [x] `test_dispatch_unknown_returns_false` — dispatch WidgetType::Unknown, assert `false`
  - [x] `test_is_known_type_true` — `isKnownType("text")`, `"clock"`, `"logo"`, `"flight_field"`, `"metric"` all return `true`
  - [x] `test_is_known_type_false` — `isKnownType("foo")`, `"CLOCK"`, `""` return `false`
  - [x] `test_from_string_roundtrip` — `fromString("text") == WidgetType::Text`, etc.
  - [x] `test_clock_cache_reuse` — call dispatch for clock 50× in tight loop; mock or stub `getLocalTime` to count calls; assert call count == 1
  - [x] `test_bounds_floor_text` — spec with h < CHAR_H (e.g. h=2); dispatch must not crash and must return `true`
  - [x] `test_bounds_floor_logo` — spec with w=4, h=4; dispatch must return `true` without drawing
  - [x] `test_logo_no_fillrect` — static assertion / grep: no `fillRect` in `firmware/widgets/LogoWidget.cpp`

- [x] **Task 8: Build and verify** (AC: all)
  - [x] `~/.platformio/penv/bin/pio run -e esp32dev` from `firmware/` — clean build, zero errors
  - [x] Binary size ≤ 1.5 MB OTA partition (`check_size.py` gate: "OK: Binary size within limits")
  - [x] `pio test -e esp32dev --filter test_widget_registry --without-uploading --without-testing` — test TU compiles cleanly
  - [x] `grep -r "fillRect" firmware/widgets/` returns zero results (logo path clean)
  - [x] `grep -r "virtual\|vtable" firmware/core/WidgetRegistry.cpp firmware/widgets/` returns zero results

---

## Dev Notes

### Architecture Context — Read This First

**LE-1.2 scope is strictly additive.** No existing files are modified except `platformio.ini` (one `build_src_filter` addition and one `build_flags` addition). The V0 spike `CustomLayoutMode` remains untouched under its `#ifdef LE_V0_SPIKE` guard. `LayoutStore`, `ModeRegistry`, and `main.cpp` are NOT touched — those are LE-1.3's job.

**Key architectural decision already locked in:** Static dispatch via `switch (type)` on `WidgetType` enum — NOT virtual functions. The V0 spike (`CustomLayoutMode.cpp`) proved this pattern; LE-1.2 promotes it to the production library. See `firmware/modes/CustomLayoutMode.cpp` lines 141–148 for the exact switch pattern to replicate.

**`RenderContext` already exists** in `firmware/interfaces/DisplayMode.h`. LE-1.2 does NOT create a new context struct — it reuses the existing one. The `RenderContext` contains:
- `FastLED_NeoMatrix* matrix` — the display adapter (NOT Adafruit GFX directly)
- `LayoutResult layoutResult` — current flight data (unused by text/clock/logo)
- `uint16_t textColor` — global text color override (widgets may use spec color instead)
- `uint8_t brightness` — display brightness 0–255
- `uint8_t* logoBuffer` — managed by LogoManager (unused by V1 stub)
- `uint32_t displayCycleMs` — frame period in ms

### New Directory: `firmware/widgets/`

Create a new directory `firmware/widgets/` for widget implementation files. This directory does NOT exist yet.

**CRITICAL — platformio.ini must be updated (Task 6).** The `build_src_filter` in `firmware/platformio.ini` currently lists explicit paths. Without adding `+<../widgets/*.cpp>`, the new widget .cpp files will be silently excluded from the build (same footgun that hit `modes/` and `utils/` in prior stories).

Current `build_src_filter` in `platformio.ini` (lines 23–29):
```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
    +<../utils/*.cpp>
    +<../modes/*.cpp>
```

Required addition:
```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
    +<../utils/*.cpp>
    +<../modes/*.cpp>
    +<../widgets/*.cpp>
```

Also add `-I widgets` to `build_flags` so `#include "widgets/TextWidget.h"` resolves from `firmware/` root.

### WidgetSpec Struct Definition

`WidgetSpec` is the production equivalent of the spike's `WidgetInstance`. Define it in `firmware/core/WidgetRegistry.h`. Field layout (keep compact for stack allocation in LE-1.3):

```cpp
struct WidgetSpec {
    WidgetType type;          // enum class WidgetType : uint8_t
    int16_t    x, y;          // top-left pixel offset on canvas
    uint16_t   w, h;          // bounding box (pixels)
    uint16_t   color;         // RGB565 packed color
    char       id[16];        // widget instance id from JSON (e.g. "w1")
    char       content[48];   // text content / clock format / logo_id
    uint8_t    align;         // 0=left, 1=center, 2=right (TextWidget)
    uint8_t    _reserved;     // pad to alignment
    // Total: ~80 bytes per instance. At 24 widgets (V1 cap): ~1920 bytes static.
};
```

**Why not a union?** The V0 spike used `char text[32]` for all widgets. The production `WidgetSpec` uses a slightly larger `content[48]` field that covers all V1 widget types (clock format strings like `"24h"`, logo ids like `"united_1"` up to 47 chars). A union would complicate JSON parsing at init time in LE-1.3.

**Note on `id` field:** Widget instance id (from JSON `"id"` field, e.g. `"w1"`). 15 chars + null. Unused by render functions but needed by LE-1.3 for layout diffing.

### Static Dispatch Implementation

The full dispatch pattern from `CustomLayoutMode.cpp` to replicate in `WidgetRegistry.cpp`:

```cpp
// In CustomLayoutMode.cpp (lines 141-148) — the REFERENCE pattern:
switch (w.type) {
    case WidgetType::TEXT:      renderText(ctx, w);      break;
    case WidgetType::CLOCK:     renderClock(ctx, w);     break;
    case WidgetType::LOGO_STUB: renderLogoStub(ctx, w);  break;
    default: break;
}
```

Production version in `WidgetRegistry.cpp`:

```cpp
bool WidgetRegistry::dispatch(WidgetType type, const WidgetSpec& spec, const RenderContext& ctx) {
    switch (type) {
        case WidgetType::Text:        return renderText(spec, ctx);
        case WidgetType::Clock:       return renderClock(spec, ctx);
        case WidgetType::Logo:        return renderLogo(spec, ctx);
        case WidgetType::FlightField: /* LE-1.8 */ return false;
        case WidgetType::Metric:      /* LE-1.8 */ return false;
        default:                      return false;
    }
}
```

**FlightField and Metric cases:** Return `false` (not yet implemented). The stubs must be present in the switch so the compiler warns on any new `WidgetType` enum value added without a case. Do NOT use `default:` as a catch-all for the known types.

### ClockWidget — 30s Cache Pattern

Exact clock cache from `CustomLayoutMode.cpp` lines 164–186 (validated in V0 spike):

```cpp
static char s_clockBuf[8] = "--:--";
static uint32_t s_lastClockUpdateMs = 0;
uint32_t nowMs = millis();
if (s_lastClockUpdateMs == 0 || (nowMs - s_lastClockUpdateMs) > 30000) {
    struct tm tinfo;
    if (getLocalTime(&tinfo, 0)) {
        snprintf(s_clockBuf, sizeof(s_clockBuf), "%02d:%02d",
                 tinfo.tm_hour, tinfo.tm_min);
        s_lastClockUpdateMs = nowMs;
    }
    // If getLocalTime fails, s_clockBuf retains "--:--" (or last valid time)
}
// Use s_clockBuf for rendering
```

**Why `static`?** These are function-scoped statics — they survive across `dispatch()` calls. This is intentional: the clock string only needs one refresh per ~30s regardless of how many frames are rendered. On a 20fps display, that is 600 calls per refresh window.

**Test hook:** For unit testing, the cache state needs to be observable. Consider extracting `s_lastClockUpdateMs` as a module-level `static` in `ClockWidget.cpp` with a `#ifdef UNIT_TEST` reset function, or design the test to wait 30s+ (impractical). Preferred approach: use a call counter instead of real time — the test injects a mock `millis()` stub or calls `dispatch()` multiple times and asserts `getLocalTime()` was called exactly once. See Task 7 `test_clock_cache_reuse` subtask.

### LogoWidget — Bitmap Blit (Not fillRect)

**Critical:** The V0 spike used `fillRect` for logo rendering. That was the WRONG approach. Spike measurements showed `fillRect(32×32)` = 2.67× render time baseline. Bitmap blit via `DisplayUtils::drawBitmapRGB565` dropped to 1.23×. The production `LogoWidget` **must** use `drawBitmapRGB565`.

V1 stub approach — pre-fill a static 16×16 buffer with the widget's color at render time:

```cpp
bool renderLogo(const WidgetSpec& spec, const RenderContext& ctx) {
    if (spec.w < 8 || spec.h < 8) return true;  // below min dimension floor
    // Build a 16×16 solid-color bitmap from spec.color.
    // Static buffer reused every frame — zero heap.
    static uint16_t s_buf[16 * 16];
    for (int i = 0; i < 16 * 16; ++i) s_buf[i] = spec.color;
    DisplayUtils::drawBitmapRGB565(ctx.matrix, spec.x, spec.y,
                                   16, 16, s_buf, spec.w, spec.h);
    return true;
    // LE-1.5 replaces this stub with real RGB565 from LittleFS via LogoManager.
}
```

**Note:** LE-1.5 will replace the static buffer fill with a call to `LogoManager::getLogoBuffer(spec.content)` where `spec.content` holds the `logo_id` from JSON (e.g. `"united_1"`). The `drawBitmapRGB565` call signature stays the same. This makes LE-1.5 a single-function swap.

### DisplayUtils — Always Use char* Overloads in Render Paths

`firmware/utils/DisplayUtils.h` provides TWO overload sets:

| Overload | Heap | Use in |
|---|---|---|
| `drawTextLine(matrix, x, y, const String& text, color)` | YES (String alloc) | Non-render paths only |
| `drawTextLine(matrix, x, y, const char* text, color)` | ZERO | **Widget render functions** |
| `truncateToColumns(const String&, int)` returns `String` | YES | Non-render paths only |
| `truncateToColumns(const char*, int, char* out, size_t)` | ZERO | **Widget render functions** |

**All widget render functions MUST use the `char*` overloads.** The `String`-based overloads trigger heap allocation on every frame (20fps = 20 alloc/dealloc cycles per second per widget). This violates the zero-heap render hot path constraint from the epic.

Example of correct usage in TextWidget:
```cpp
char out[48];
DisplayUtils::truncateToColumns(spec.content, spec.w / CHAR_W, out, sizeof(out));
DisplayUtils::drawTextLine(ctx.matrix, drawX, drawY, out, spec.color);
```

### LayoutStore Allowlist Sync

`firmware/core/LayoutStore.cpp` (Task 3 in LE-1.1) contains a hardcoded allowlist:
```cpp
static const char* kAllowedWidgetTypes[] = {
    "text", "clock", "logo", "flight_field", "metric"
};
```

This allowlist was written in LE-1.1 to match the epic's widget type set. In LE-1.2, `WidgetRegistry::fromString()` must map exactly these same 5 type strings to enum values. **These two lists must stay in sync.** If LE-1.8 adds a new widget type, both `kAllowedWidgetTypes[]` in `LayoutStore.cpp` AND the `fromString()` / `dispatch()` in `WidgetRegistry.cpp` must be updated together.

**For LE-1.2:** No changes to `LayoutStore.cpp` are needed — the allowlist already includes all 5 types. Just ensure `WidgetRegistry::fromString()` maps the same 5 strings identically.

### Font and Dimension Constants

From `CustomLayoutMode.cpp` (lines 57–59) — reuse these constants:
```cpp
static constexpr int CHAR_W = 6;  // 5x7 font + 1px spacing
static constexpr int CHAR_H = 8;  // matches ClassicCardMode baseline
```

Define these in `firmware/core/WidgetRegistry.h` (or in a shared header if preferred) so all widget files can reference them without magic numbers.

Minimum dimension floors (from AC #7):
- TextWidget: min height = `CHAR_H` (8px)
- ClockWidget: min = 8×6 (h=8, w=6×CHAR_W=36 for "HH:MM" in 5×7 font)
- LogoWidget: min = 8×8

### ArduinoJson v7 — Parse-Once Rule

`WidgetSpec` is pre-populated at layout init time (LE-1.3's job). The widget render functions in LE-1.2 receive a `const WidgetSpec&` — they do NOT parse JSON. There is no `JsonDocument` in any render path. This is enforced by the function signatures: `renderText(const WidgetSpec&, const RenderContext&)` takes no JSON input.

The parse-once rule from the epic: *"Parse JSON at mode init only. render() must not see JSON."*

### NVS Keys — No New Keys in LE-1.2

LE-1.2 does not add NVS keys. The widget registry is stateless. If ClockWidget needs a 12h/24h setting in the future (LE-1.7+), it will use `ConfigManager::getModeSetting("wclk", "fmt", 0)` following the established `m_{abbrev}_{key}` ≤ 15 chars pattern (see `ClockMode.h` line 10: `m_clock_format` = 13 chars).

For reference, the NVS key budget from ConfigManager:
```
Namespace: "flightwall"
Key                   Chars   Story
disp_mode             9       existing
layout_active         13      LE-1.1
[no new keys]         —       LE-1.2
```

### Antipatterns to Avoid (from LE-1.1 Code Review)

These patterns were found in LE-1.1 and must NOT be repeated in LE-1.2:

| Antipattern | Where it would manifest in LE-1.2 | Fix |
|---|---|---|
| Unbounded `readString()` without size guard | N/A — LE-1.2 has no file I/O | N/A |
| `fillRect` for logo rendering | `LogoWidget.cpp` | Use `drawBitmapRGB565` — see Task 5 |
| `String`-based overloads in render path | Any `render*()` function | Use `char*` overloads from DisplayUtils |
| Log messages omitting offending id | Any `Serial.printf` in widget code | Include type string or spec id in log |
| Virtual dispatch | `WidgetRegistry.h/.cpp` | Use `switch` on enum — no virtual |
| Heap allocation in render hot path | Any `render*()` function | Stack buffers only (see LogoWidget `static uint16_t s_buf[]`) |

### Test Infrastructure

Tests live at `firmware/test/test_widget_registry/test_main.cpp`. On-device Unity test (same pattern as `test_layout_store`):

```cpp
// Minimal test scaffold (follow le-1.1 pattern exactly):
#include <Arduino.h>
#include <unity.h>
#include "core/WidgetRegistry.h"
#include "widgets/TextWidget.h"
#include "widgets/ClockWidget.h"
#include "widgets/LogoWidget.h"

// Test helper — build a minimal valid WidgetSpec
static WidgetSpec makeSpec(WidgetType type, int16_t x, int16_t y,
                            uint16_t w, uint16_t h, const char* content = "") {
    WidgetSpec s{};
    s.type = type;
    s.x = x; s.y = y; s.w = w; s.h = h;
    s.color = 0xFFFF;  // white
    strncpy(s.content, content, sizeof(s.content) - 1);
    s.align = 0;
    return s;
}

// Test helper — build a minimal RenderContext with a null matrix
// (render fns that check ctx.matrix != nullptr will skip matrix writes)
static RenderContext makeCtx() {
    RenderContext ctx{};
    ctx.matrix = nullptr;  // test env has no real matrix
    return ctx;
}

void test_dispatch_text_returns_true() {
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Text, 0, 0, 80, 8, "Hello");
    // renderText must guard ctx.matrix == nullptr and return true
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Text, spec, ctx));
}

void test_dispatch_unknown_returns_false() {
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Unknown, 0, 0, 10, 10);
    TEST_ASSERT_FALSE(WidgetRegistry::dispatch(WidgetType::Unknown, spec, ctx));
}

// ... additional test functions per Task 7 subtasks ...

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_dispatch_text_returns_true);
    RUN_TEST(test_dispatch_clock_returns_true);
    RUN_TEST(test_dispatch_logo_returns_true);
    RUN_TEST(test_dispatch_unknown_returns_false);
    RUN_TEST(test_is_known_type_true);
    RUN_TEST(test_is_known_type_false);
    RUN_TEST(test_from_string_roundtrip);
    RUN_TEST(test_clock_cache_reuse);
    RUN_TEST(test_bounds_floor_text);
    RUN_TEST(test_bounds_floor_logo);
    UNITY_END();
}

void loop() {}
```

**IMPORTANT:** `ctx.matrix = nullptr` is intentional for tests. All render functions that call matrix methods MUST guard `if (ctx.matrix == nullptr) return true` at the top. This matches the existing pattern in `CustomLayoutMode::render()` (line 136: `if (ctx.matrix == nullptr) return`).

Run with:
```bash
cd firmware
pio test -e esp32dev --filter test_widget_registry --without-uploading --without-testing
```

### Project Structure After LE-1.2

```
firmware/
├── core/
│   ├── LayoutStore.h              ← UNTOUCHED (LE-1.1, done)
│   ├── LayoutStore.cpp            ← UNTOUCHED — allowlist already correct
│   ├── ConfigManager.h            ← UNTOUCHED (no new NVS keys in LE-1.2)
│   ├── ConfigManager.cpp          ← UNTOUCHED
│   ├── ModeRegistry.h             ← UNTOUCHED (LE-1.3's job)
│   ├── WidgetRegistry.h           ← NEW (Task 1)
│   └── WidgetRegistry.cpp         ← NEW (Task 2)
├── widgets/                       ← NEW DIRECTORY
│   ├── TextWidget.h               ← NEW (Task 3)
│   ├── TextWidget.cpp             ← NEW (Task 3)
│   ├── ClockWidget.h              ← NEW (Task 4)
│   ├── ClockWidget.cpp            ← NEW (Task 4)
│   ├── LogoWidget.h               ← NEW (Task 5)
│   └── LogoWidget.cpp             ← NEW (Task 5)
├── modes/
│   ├── CustomLayoutMode.h         ← UNTOUCHED (LE_V0_SPIKE guard, LE-1.3 scope)
│   └── CustomLayoutMode.cpp       ← UNTOUCHED
├── test/
│   └── test_widget_registry/
│       └── test_main.cpp          ← NEW (Task 7)
└── platformio.ini                 ← MODIFY: add widgets to build_src_filter + build_flags (Task 6)
```

### References

- `_bmad-output/planning-artifacts/epic-le-1-layout-editor.md` — Architecture section, widget type set, zero-heap constraint, V0 spike results, quality gates
- `_bmad-output/planning-artifacts/epics/epic-le-1.md` — Story index; LE-1.2 AC one-liners
- `_bmad-output/implementation-artifacts/antipatterns/epic-le-1-code-antipatterns.md` — LE-1.1 code review findings (fillRect, String heap, missing id in log messages)
- `firmware/modes/CustomLayoutMode.h` / `.cpp` — V0 spike reference patterns: `WidgetType` enum, `WidgetInstance` struct, `parseType()`, `renderClock()` cache, switch dispatch, `parseLayout()`
- `firmware/utils/DisplayUtils.h` — char* overload signatures to use in render functions
- `firmware/core/LayoutStore.h` / `.cpp` — `kAllowedWidgetTypes[]` allowlist to stay in sync with `fromString()`
- `firmware/interfaces/DisplayMode.h` — `RenderContext` definition (do not redefine)
- `firmware/modes/ClockMode.h` — NVS key naming pattern (`m_clock_format`, 13 chars)
- `firmware/test/test_layout_store/test_main.cpp` — Unity test scaffold to mirror
- `firmware/check_size.py` — Binary size gate (1.5 MB hard limit, 1.3 MB warning)

---

## File List

**New files:**
- `firmware/core/WidgetRegistry.h`
- `firmware/core/WidgetRegistry.cpp`
- `firmware/widgets/TextWidget.h`
- `firmware/widgets/TextWidget.cpp`
- `firmware/widgets/ClockWidget.h`
- `firmware/widgets/ClockWidget.cpp`
- `firmware/widgets/LogoWidget.h`
- `firmware/widgets/LogoWidget.cpp`
- `firmware/test/test_widget_registry/test_main.cpp`

**Modified files:**
- `firmware/platformio.ini` — add `+<../widgets/*.cpp>` to `build_src_filter`, add `-I widgets` to `build_flags`

**Untouched files (do not modify):**
- `firmware/core/LayoutStore.h` / `.cpp` — LE-1.1 complete
- `firmware/core/ConfigManager.h` / `.cpp` — no new NVS keys this story
- `firmware/core/ModeRegistry.h` — LE-1.3 scope
- `firmware/src/main.cpp` — LE-1.3 scope
- `firmware/modes/CustomLayoutMode.h` / `.cpp` — LE-1.3 scope
- `firmware/interfaces/DisplayMode.h` — `RenderContext` already defined; do not touch

---

## Dev Agent Record

### Implementation Plan

Followed the story task order verbatim:

1. **Task 1 / Task 2 — `firmware/core/WidgetRegistry.{h,cpp}`**
   - Global `enum class WidgetType : uint8_t` with the 5 known types plus `Unknown = 0xFF`.
   - `WidgetSpec` struct laid out per Dev Notes (80 bytes, `content[48]`, `id[16]`, `align`, `_reserved`).
   - `fromString()` is a straight strcmp sequence (null/empty guard → 5 strcmps).
   - `dispatch()` uses an enumerated `switch` with no `default` branch on the known types so any new `WidgetType` member added later triggers `-Wswitch` at this call site. `Unknown` and any out-of-range cast return `false`.
   - `WIDGET_CHAR_W` / `WIDGET_CHAR_H` constants (6/8) published from the header so all widget TUs can reference them without magic numbers.

2. **Task 3 — `firmware/widgets/TextWidget.{h,cpp}`**
   - Truncates via `DisplayUtils::truncateToColumns` (`char*` overload) into a 48-byte stack buffer.
   - Recomputes rendered pixel width from the truncated string to keep center/right alignment accurate even when the ellipsis suffix is injected.
   - Guards empty content, zero column count, and `ctx.matrix == nullptr` (test harness path) — all return `true` (success, nothing to draw) rather than `false`.
   - Vertical centering only applied when `spec.h > CHAR_H`, otherwise draws at `spec.y`.

3. **Task 4 — `firmware/widgets/ClockWidget.{h,cpp}`**
   - File-scope `s_clockBuf[8]` / `s_lastClockUpdateMs` statics implement the 30s minute-resolution cache from the V0 spike.
   - Non-blocking `getLocalTime(&tinfo, 0)` — cache timestamp only advances on success so a transient NTP miss retries next frame (no 30s stall).
   - Min dimensions enforced at the top (w ≥ 6·CHAR_W, h ≥ CHAR_H) — returns `true` (no-op) on undersized specs.
   - Added `ClockWidgetTest::resetCacheForTest()`, `getTimeCallCount()`, `resetTimeCallCount()` so the Unity suite can prove the cache window without sleeping 30s.

4. **Task 5 — `firmware/widgets/LogoWidget.{h,cpp}`**
   - V1 stub fills a file-scope `static uint16_t s_buf[16*16]` with `spec.color` and passes it to `DisplayUtils::drawBitmapRGB565`. Zero heap.
   - Explicitly does NOT call `fillRect` (AC #6). Inline header/source comments reference the spike's 2.67× vs 1.23× measurements.
   - Guards `spec.w < 8 || spec.h < 8` and `ctx.matrix == nullptr`.
   - `// V1 stub — LE-1.5 replaces with real RGB565 bitmap from LittleFS via LogoManager` comment in place.

5. **Task 6 — `firmware/platformio.ini`**
   - Added `+<../widgets/*.cpp>` to `build_src_filter` and `-I widgets` to `build_flags`.
   - The `esp32dev_le_baseline` and `esp32dev_le_spike` envs inherit both changes via `extends = env:esp32dev`.

6. **Task 7 — `firmware/test/test_widget_registry/test_main.cpp`**
   - 15 Unity test functions grouped by AC. Uses `ctx.matrix = nullptr` scaffold (matches the pattern documented in Dev Notes).
   - `test_clock_cache_reuse` dispatches 50 times and asserts `ClockWidgetTest::getTimeCallCount() ≤ 1`. On a test rig without NTP, `getLocalTime` returns false → call count stays 0, which still proves the cache window (no unbounded calls).
   - Added coverage for `FlightField` / `Metric` returning `false` (contract: enumerated but not yet implemented).

7. **Task 8 — Build & verify**
   - `pio run -e esp32dev` → SUCCESS, binary 1,277,920 bytes (1.22 MB), well within the 1.5 MB OTA partition.
   - `pio run -e esp32dev_le_spike` → SUCCESS (1.22 MB).
   - `pio test -e esp32dev --filter test_widget_registry --without-uploading --without-testing` → PASSED (TU compiles and links).
   - `pio test -e esp32dev --filter test_layout_store --without-uploading --without-testing` → PASSED (no regression in LE-1.1 tests).
   - `grep -n "fillRect(" firmware/widgets/` returns only comment references ("Why not fillRect?", "Must NOT use fillRect"). No actual calls.
   - `grep -rn "^\s*virtual " firmware/core/WidgetRegistry.* firmware/widgets/` returns zero declarations.

### Completion Notes

- All 8 tasks and 8 acceptance criteria satisfied.
- No existing files modified outside the story zone except `firmware/platformio.ini` (expected per Task 6).
- No new NVS keys, no new LittleFS paths, no ConfigManager changes.
- Main.cpp and CustomLayoutMode are untouched — LE-1.2 is strictly additive to LE-1.3's future wiring surface.
- Binary size delta: roughly +6 KB (1,271,349 → 1,277,920 bytes) — within the epic's render-hot-path budget.
- Antipattern table from LE-1.1 code review honored: no `fillRect`, no `String` overloads, no virtual dispatch, no heap allocation in render fns.
- LayoutStore allowlist (`text`, `clock`, `logo`, `flight_field`, `metric`) stays in lock-step with the new `WidgetRegistry::fromString()` mapping.

---

## Change Log

| Date | Version | Description | Author |
|---|---|---|---|
| 2026-04-17 | 0.1 | Draft story created by BMAD story writer | BMAD |
| 2026-04-17 | 1.0 | Upgraded to ready-for-dev with comprehensive Dev Notes: WidgetSpec field layout, static dispatch pattern, ClockWidget cache, LogoWidget bitmap blit, platformio.ini build_src_filter requirement, DisplayUtils char* overload mandate, LayoutStore allowlist sync reference, Unity test scaffold, antipattern table | BMAD Story Synthesis |
| 2026-04-17 | 1.1 | Implementation complete: WidgetRegistry + TextWidget/ClockWidget/LogoWidget + Unity tests. Build 1.22 MB, 0 fillRect, 0 virtual. Status → ready-for-review. | Dev Agent |
| 2026-04-17 | 1.2 | Code review synthesis: fixed ClockWidget NTP-down throttle bug (s_lastClockUpdateMs now advances on failure); tightened test_clock_cache_reuse to two-phase structure; fixed stale "always 32×32" comment in DisplayUtils; documented black-as-transparent behaviour in LogoWidget. | AI Code Review Synthesis |
| 2026-04-17 | 1.3 | Second code review synthesis (Reviewers A+B): gated ClockWidgetTest namespace behind PIO_UNIT_TESTING; corrected stale dispatch comment claiming a `default` branch; added V1 multi-clock limitation note to ClockWidget.h. 12 of 20 reviewer issues dismissed as false positives. | AI Code Review Synthesis |

---

## Senior Developer Review (AI)

### Review: 2026-04-17
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 5.2 (Reviewer A) → CHANGES REQUESTED (one real HIGH bug confirmed by two reviewers)
- **Issues Found:** 4 verified (1 High, 1 Medium, 2 Low)
- **Issues Fixed:** 4
- **Action Items Created:** 0

#### Review Follow-ups (AI)
<!-- All verified issues were fixed in this synthesis pass. No open action items. -->

### Review: 2026-04-17 (Round 2 — Reviewers A+B)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** Reviewer A: 5.1 (borderline) / Reviewer B: 10.0 (largely false positives upon source verification)
- **Issues Found:** 3 verified (0 High, 0 Medium, 3 Low)
- **Issues Fixed:** 3
- **Action Items Created:** 0

#### Review Follow-ups (AI)
<!-- All verified issues were fixed in this synthesis pass. No open action items. -->


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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial code review — Story LE-1.2 (widget-registry-and-core-widgets)**  
**Repo:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main`  
**Scope:** Story zone + tests + `DisplayUtils::drawBitmapRGB565` (cross-check AC #6 / transparency)

---

## Review summary

| Category | Issues found |
|----------|--------------|
| Git vs story discrepancies | 1 |
| AC implementation gaps | 2 |
| Task completion lies | 0 |
| SOLID violations | 0 (non-blocking) |
| Hidden bugs / safety | 2 |
| Performance footguns | 1 |
| Security vulnerabilities | 0 |
| **Total issues** | **9** |

---

## Evidence score summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| IMPORTANT | AC #4 claims three alignments are validated, but the Unity “alignment” test never exercises pixel placement (matrix is always `nullptr`). | `firmware/test/test_widget_registry/test_main.cpp` (e.g. `test_text_alignment_all_three`, ~215–228) | +1.0 |
| IMPORTANT | AC #7 (“clip safely / no OOB writes”) is not proven: all render paths that hit `DisplayUtils` are skipped when `ctx.matrix == nullptr`, so tests never assert clipping vs widget height/width on a real framebuffer. | `firmware/test/test_widget_registry/test_main.cpp` + widget `*.cpp` | +1.0 |
| MINOR | `LogoWidget` refills a 16×16 RGB565 buffer with a scalar loop on every render call — zero heap but unnecessary CPU at ~20 Hz. | `firmware/widgets/LogoWidget.cpp` (~43–45) | +0.3 |
| MINOR | `TextWidget` for `spec.h < WIDGET_CHAR_H` still calls `drawTextLine` at `spec.y`; Adafruit GFX typically paints full glyph height unless externally clipped — risk of violating “do not write below `spec.y + spec.h`” depending on matrix clipping. | `firmware/widgets/TextWidget.cpp` (~59–72) | +0.3 |
| MINOR | `WidgetRegistry::dispatch(type, …)` ignores `spec.type` — a mismatched `(type, spec)` pair silently renders the wrong widget class. | `firmware/core/WidgetRegistry.cpp` (~49–60) | +0.3 |
| MINOR | `LayoutStore` allowlist vs `WidgetRegistry::fromString()` remains a **manual dual-source** sync bomb (story warns; still no compile-time/shared table enforcement). | `firmware/core/WidgetRegistry.cpp` + `firmware/core/LayoutStore.cpp` (by policy) | +0.3 |
| MINOR | `setup()` in the Unity suite sleeps `delay(2000)` unconditionally — slows every on-device test run for a clock test that could be structured without wall-clock sleep (or gated). | `firmware/test/test_widget_registry/test_main.cpp` (~241) | +0.3 |
| MINOR | `DisplayMode.h` still carries a placeholder heap-baseline comment (`~XXX,XXX`) — misleading onboarding signal in a core interface header. | `firmware/interfaces/DisplayMode.h` (~7–9) | +0.3 |
| MINOR | Workspace snapshot (conversation `git_status`) shows many modified paths **outside** the story’s declared File List; if those changes ship under the LE-1.2 story label, that is a **scope / traceability** problem (not necessarily a widget bug). | repo hygiene vs story “File List” | +0.3 |

### Clean-pass categories ( −0.5 each )

- **Security:** No new network/NVS/file attack surface in LE-1.2 widget code paths (render is pre-parsed `WidgetSpec` only). **−0.5**  
- **SOLID:** No egregious SRP violations for a small static-dispatch façade; coupling is intentional. **−0.5**

### Evidence score: **2.7**

| Score | Verdict |
|-------|---------|
| **2.7** | **APPROVED** (per workflow: score &lt; 3 → acceptable with actionable follow-ups) |

---

## Architectural sins

- **Boundary / contract drift (minor):** `dispatch` takes `type` and `spec` independently with no `assert(type == spec.type)` (or debug check). This is a future footgun once LE-1.3 materializes specs from JSON.  
  - `firmware/core/WidgetRegistry.cpp` (~49–60)

- **Dual-source truth (maintainability):** widget type strings exist in both `LayoutStore` validation and `WidgetRegistry::fromString()` — correct today, fragile tomorrow.  
  - `firmware/core/WidgetRegistry.cpp` (~27–35)

---

## Pythonic crimes & readability

N/A (C++). Minor readability: comments in tests admit “smoke” behavior while story AC #4 reads like a functional guarantee — documentation mismatch.

---

## Performance & scalability

- **[Medium on hot path]** `renderLogo` overwrites `static uint16_t s_buf[256]` every call in a loop. Not heap-unsafe, but it burns cycles each frame; consider caching last `spec.color` / dirty flag, or `memset16` pattern if this remains hot.  
  - `firmware/widgets/LogoWidget.cpp` (~43–45)

---

## Correctness & safety

- **🐛 Potential AC #7 gap:** short text boxes (`h < 8`) may still emit an 8px-tall glyph from `drawTextLine` unless the GFX layer clips to the widget rectangle. Needs either explicit vertical clip math or a documented invariant that the matrix is always clipped.  
  - `firmware/widgets/TextWidget.cpp` (~59–72)

- **`drawBitmapRGB565` “black is transparent” behavior:** pixels `0x0000` are skipped (`if (pixel != 0)`), so `spec.color == 0` makes the stub invisible — already noted in story/antipatterns, but it is still a correctness trap for layout authors.  
  - `firmware/utils/DisplayUtils.cpp` (~131–137)

---

## Lying tests / weak tests

- **`test_text_alignment_all_three`:** claims to cover left/center/right, but uses `ctx.matrix == nullptr` for all three dispatches — it only proves “doesn’t crash / returns true”, **not** alignment math. This is a **false sense of coverage** vs AC #4.  
  - `firmware/test/test_widget_registry/test_main.cpp` (~215–228)

- **`test_clock_cache_reuse`:** Phase 2 assumes “first call arms cache”; `getTimeCallCount()` increments **only on successful** `getLocalTime()`. On rigs without time, count can remain `0` while still satisfying `≤ 1` — the test is **much weaker** than the story wording suggests (though not entirely vacuous after the two-phase structure).

---

## Maintainability issues

- **Placeholder metrics debt:** `DisplayMode.h` heap baseline comment is still template junk.  
  - `firmware/interfaces/DisplayMode.h` (~7–9)

- **Task 7 / AC #8 mismatch:** story asks for an automated `test_logo_no_fillrect` (static assert / grep). That is **not implemented in Unity** — it’s an external manual gate. Fine if CI runs grep, but the story checkbox over-claims “test” coverage.

---

## Suggested fixes (priority order)

1. **Replace alignment “smoke” with a real assertion strategy:** inject a tiny `FastLED_NeoMatrix` test double / capture buffer, or factor “compute drawX” into a `constexpr`/pure function unit-tested without hardware.  
2. **Add one device or host test** that uses a non-null matrix stub to verify **clipping** expectations for `h < CHAR_H` and right/center math (even 1–2 golden `drawX` values).  
3. **Micro-optimize `LogoWidget`** buffer fill (color change detection or bulk fill).  
4. **Add `if (type != spec.type) return false;`** (or `LOG_W` + return false) in `dispatch` for debug builds.  
5. **Remove `delay(2000)`** from Unity `setup` by driving clock cache via `ClockWidgetTest::resetCacheForTest()` + controlled `millis()` stub (if you add one) or split tests.  
6. **Centralize widget type strings** in one `constexpr` array consumed by both `LayoutStore` and `WidgetRegistry` to kill the dual-source risk.

---

**Issues fixed:** 0 (read-only review)  
**Action items created:** 0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

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
| 🟢 CLEAN PASS | 9 | - | - |

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

✅ No action items detected.

---

**Issues Fixed:** 0
**Action Items Created:** 0

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

# 🔥 ADVERSARIAL CODE REVIEW: Story LE-1.2

**Story:** `le-1-2-widget-registry-and-core-widgets`
**Reviewer:** AI Code Review (Adversarial Mode)
**Date:** 2026-04-17

---

## 📊 Executive Summary

| Category | Count | Status |
|----------|-------|--------|
| **Total Issues Found** | **9** | 🔴 NEEDS ATTENTION |
| Critical Issues | 3 | Blocking |
| Important Issues | 4 | Significant |
| Minor Issues | 2 | Documentation |
| **Evidence Score** | **5.8** | 🟠 MAJOR REWORK |
| **Verdict** | **CHANGES REQUIRED** | See details below |

---

## 🎯 Critical Findings (Must Fix)

### 1. 🔴 **CRITICAL: Acceptance Criterion #3 NOT Implemented**
- **Issue:** AC#3 requires dispatch uses "`switch` on `WidgetType` enum **OR** fixed `std::array<Entry, N>` lookup — **not** virtual-function vtable."
- **Actual Code:** `WidgetRegistry.cpp` lines 30-40 shows pure `switch` statement ✅ BUT `DisplayMode.h` defines `DisplayMode` as an abstract base class with **virtual methods** (`init`, `render`, `teardown`).
- **Problem:** The *dispatch mechanism* is non-virtual (✅), but the **modes themselves** use virtual dispatch for the render pattern. The AC is ambiguous, but the spirit is "stateless dispatch" — the virtual methods on DisplayMode contradict "no vtable per widget."
- **Severity:** 🔴 **CRITICAL** — AC interpretation dispute, blocks approval
- **Source:** `firmware/interfaces/DisplayMode.h:30-42`, `firmware/core/WidgetRegistry.cpp:30-40`
- **Fix Required:** Clarify AC#3 intent with product owner — is the goal (a) only the dispatch mechanism non-virtual, OR (b) entire widget system non-virtual?

---

### 2. 🔴 **CRITICAL: Task 8 "grep -r virtual returns zero results" LIES**
- **Issue:** Task 8, acceptance step "grep -r 'virtual|vtable' firmware/core/WidgetRegistry.cpp firmware/widgets/ returns zero results" is claimed [x] DONE.
- **Actual Code:** `firmware/interfaces/DisplayMode.h` (the interface all modes inherit from) contains:
  ```cpp
  virtual bool init(const RenderContext& ctx) = 0;
  virtual void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) = 0;
  virtual void teardown() = 0;
  virtual const ModeZoneDescriptor& getZoneDescriptor() const = 0;
  virtual const ModeSettingsSchema* getSettingsSchema() const = 0;
  ```
- **The grep command scope is wrong:** Task specifies `grep` in `firmware/core/WidgetRegistry.cpp firmware/widgets/` but the virtual keywords are in `firmware/interfaces/DisplayMode.h`. By narrowing the grep scope, the [x] completion claim is technically *true on that narrow path* but **misleading**.
- **Real test:** `grep -r "virtual" firmware/interfaces/DisplayMode.h firmware/widgets/` **FAILS** — finds 5 virtual keywords.
- **Severity:** 🔴 **CRITICAL** — False task completion claim
- **Source:** Task 8 line `grep -r "virtual\|vtable" firmware/core/WidgetRegistry.cpp firmware/widgets/` + actual `firmware/interfaces/DisplayMode.h:26-49`
- **Fix:** Re-scope grep to include `firmware/interfaces/` or accept virtual dispatch as intentional design (then update AC#3 language).

---

### 3. 🔴 **CRITICAL: ClockWidget NTP Throttle Bug (Even After Synthesis Fix)**
- **Issue:** Task 2.2 synthesis round claimed: "Added `s_lastClockUpdateMs` outside the `if (getLocalTime...)` to advance on both success and failure."
- **Actual Code Review:** `firmware/widgets/ClockWidget.cpp` lines 30-39 shows:
  ```cpp
  uint32_t nowMs = millis();
  if (s_lastClockUpdateMs == 0 || (nowMs - s_lastClockUpdateMs) > 30000) {
      struct tm tinfo;
      if (getLocalTime(&tinfo, 0)) {
          snprintf(s_clockBuf, sizeof(s_clockBuf), "%02d:%02d",
                   tinfo.tm_hour, tinfo.tm_min);
          s_timeCallCount++;
      }
      s_lastClockUpdateMs = nowMs;  // ← APPEARS FIXED (synthesis applied)
  }
  ```
- **BUT Wait:** The update of `s_lastClockUpdateMs` IS outside the inner `if` (line 39 outside the `if (getLocalTime...)` block at line 33). Synthesis claims this is fixed.
- **BUT THE REAL BUG:** Inspection of the actual implementation shows the **comment at line 21-24 claims NTP-down throttle is working**, but the code doesn't match the story's AC#5 requirement: *"Refresh only when `s_lastClockUpdateMs == 0 || (millis() - s_lastClockUpdateMs) > 30000`"*. 
- **Actual behavior:** If NTP is down (getLocalTime returns false continuously), s_lastClockUpdateMs advances every 30s, so getLocalTime() is called once every 30s (correct behavior for throttle). However, the displayed time stays "--:--" because the snprintf never executes. **This is correct per AC#5.**
- **Re-examination:** The code is actually correct. This finding is a FALSE POSITIVE. The synthesis fix was applied correctly.
- **Re-grading:** Demote to IMPORTANT (below).

---

### 3. 🟠 **IMPORTANT: platformio.ini Task 6 — Missing `-I widgets` Build Flag (Compiler Will Fail)**
- **Issue:** Task 6 requires adding BOTH `+<../widgets/*.cpp>` to `build_src_filter` AND `-I widgets` to `build_flags`.
- **Actual `platformio.ini`:** Lines 23-36 show `build_flags` section:
  ```ini
  build_flags =
      -I .
      -I include
      -I src
      -I adapters
      -I core
      -I models
      -I interfaces
      -I utils
      -I config
      -I modes
      -I widgets    # ← PRESENT
  ```
- **Status:** ✅ **ACTUALLY PRESENT** — the flag IS there. No issue.
- **Re-grading:** This is a FALSE POSITIVE. The implementation is correct.

---

## 🟠 Important Findings (Architectural/Test Issues)

### 4. 🟠 **IMPORTANT: AC#8 Test Coverage Incomplete — `test_clock_cache_reuse` Has Weak Assertion Logic**
- **Issue:** AC#8 task 7 requires `test_clock_cache_reuse` that "call dispatch for clock 50× in tight loop; mock or stub `getLocalTime` to count calls; assert call count == 1".
- **Actual Test:** In `firmware/test/test_widget_registry/test_main.cpp` (referenced in story, not included in file list), the story **claims** tests exist but DOES NOT provide actual test source code.
- **Problem:** Without seeing the test file, cannot verify:
  - Does the test actually call dispatch 50 times?
  - Is `getLocalTime` properly mocked/stubbed?
  - Is the assertion exactly `call_count == 1` or a weaker `<= 1`?
- **Story admits:** Dev Notes state "Weak assertions allow the test to pass even when cache is broken" — the two-phase test structure from synthesis was supposed to fix this, but the actual test code isn't provided for verification.
- **Severity:** 🟠 **IMPORTANT** — Cannot verify AC#8 compliance without test source
- **Source:** Story file line `Test helper — build a minimal valid WidgetSpec` through `test_clock_cache_reuse`
- **Fix:** Provide the actual test file or accept that AC#8 cannot be fully validated in this review.

---

### 5. 🟠 **IMPORTANT: AC#7 Bounds Checking Is Non-Enforcing (Silent No-Op on Undersized Widgets)**
- **Issue:** AC#7 states: "**When** a spec has `w` or `h` below the floor **Then** the render fn clips safely and does not write out-of-bounds (text min height = font size; clock min = 8×6; logo min = 8×8)."
- **Actual Code — TextWidget.cpp lines 20-25:**
  ```cpp
  if (maxCols <= 0) return true;  // no-op success
  ```
- **Actual Code — ClockWidget.cpp lines 26-28:**
  ```cpp
  if ((int)spec.w < 6 * WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) {
      return true;  // skip render — not an error
  }
  ```
- **Actual Code — LogoWidget.cpp lines 12-13:**
  ```cpp
  if ((int)spec.w < 8 || (int)spec.h < 8) return true;
  ```
- **The Problem:** All three widgets silently return `true` (success) when undersized, meaning they render *nothing* but claim success. The AC says "clips safely" — but "clipping" usually means partial rendering, not complete omission.
- **Counterargument:** For a clock displaying "--:--" in a 1-pixel-high zone, silent no-op *is* the safe fallback. The AC's language is loose.
- **Verdict:** **Technically compliant but misleading** — AC#7 doesn't mandate visual output, just "safe" handling (no crash/corruption). This is correct.
- **Re-grading:** COMPLIANT. Move to MINOR observation below.

---

### 6. 🟠 **IMPORTANT: LogoWidget.cpp `spec.color = 0x0000` Black Renders Invisible (Undocumented Gotcha)**
- **Issue:** LogoWidget.cpp lines 31-33:
  ```cpp
  for (int i = 0; i < kLogoW * kLogoH; ++i) {
      s_buf[i] = spec.color;
  }
  ```
- **Real Issue:** The story's own file header comment acknowledges "NOTE: drawBitmapRGB565 treats pixel value 0x0000 (black) as transparent..." BUT this is **not mentioned in the WidgetSpec contract** and a caller who specifies `color: 0x0000` (black) will silently produce an invisible logo.
- **Severity:** 🟠 **IMPORTANT** — Surprising behavior, potential UX confusion
- **Source:** `firmware/widgets/LogoWidget.cpp:11-18`, `firmware/utils/DisplayUtils.h` (undocumented black-as-transparent behavior)
- **Fix:** Add validation in `WidgetRegistry::dispatch()` to warn/reject `color == 0x0000` for Logo widgets, OR update JSON schema to forbid black color for logos, OR document this in WidgetSpec color field docs.

---

### 7. 🟠 **IMPORTANT: No NVS Key Collision Check with ConfigManager Keys**
- **Issue:** Story claims "Struct Placement Clarification" will prevent collisions, but `WidgetRegistry.h` (which does NOT include ConfigManager.h) has zero validation that custom layout JSON doesn't accidentally use an NVS key name like `brightness`, `timezone`, `disp_mode`.
- **Attack Vector:** A malicious or confused user could create a layout with a widget `id="timezone"`, which during LE-1.3 integration might conflict with ConfigManager's `timezone` NVS key.
- **Current Mitigation:** LayoutStore validates against `kAllowedWidgetTypes[]` but does NOT validate widget IDs against ConfigManager key names.
- **Severity:** 🟠 **IMPORTANT** — Potential cross-subsystem data corruption
- **Source:** `firmware/core/LayoutStore.h:30-47` (no collision check), `firmware/core/ConfigManager.h:7-26` (all NVS keys listed)
- **Fix:** Add a validation function in ConfigManager or LayoutStore that checks layout JSON widget IDs don't shadow NVS key names. Document this constraint in WidgetSpec.

---

## 🟡 Minor Findings (Style/Documentation)

### 8. 🟡 **MINOR: Missing Include Guard in LogoWidget.cpp (Not Critical, But Inconsistent)**
- **Issue:** All other widget headers have `#pragma once` at the top. `LogoWidget.h` line 1 shows:
  ```cpp
  #pragma once
  /*
  Purpose: V1 logo-widget stub...
  ```
  ✅ Correct.
- **BUT:** `LogoWidget.cpp` has no corresponding `#pragma once` — this is normal (`.cpp` files don't need guards), but the **header comment order** in `LogoWidget.cpp` puts `#include` statements AFTER comments (lines 10-11), which is unconventional.
- **Minor Style Issue:** Convention is comments THEN includes. This doesn't affect functionality.
- **Severity:** 🟡 **MINOR**
- **Source:** `firmware/widgets/LogoWidget.cpp:1-15`

---

### 9. 🟡 **MINOR: Dev Notes Reference "CustomLayoutMode.cpp lines 141-148" But File Not in Review**
- **Issue:** Story Dev Notes extensively reference `firmware/modes/CustomLayoutMode.cpp` as the "V0 spike reference pattern" (lines 164, 241, etc.) but CustomLayoutMode is **not in the LE-1.2 File List** — it's an LE-0.1 artifact.
- **Problem:** This creates a knowledge gap — reviewers cannot easily verify that the LE-1.2 dispatch pattern matches the V0 spike. The architecture doc includes CustomLayoutMode.h/.cpp, but it's not included in the Code Review file set.
- **Mitigation:** The story includes extensive inline code examples that replicate the pattern, so compliance can still be verified.
- **Severity:** 🟡 **MINOR** — Documentation/traceability issue
- **Source:** Dev Notes section, lines ~130-140 (reference to CustomLayoutMode)
- **Fix:** Either include CustomLayoutMode.cpp in the review set, or add a line: "Reference pattern is demonstrated in the provided code examples (exact line numbers omitted to scope review to LE-1.2 only)."

---

## 📋 Acceptance Criteria Audit

| AC # | Requirement | Status | Evidence |
|------|-------------|--------|----------|
| 1 | Dispatch returns true on match | ✅ PASS | `WidgetRegistry.cpp:35-40` switch cases return `renderText(...)` etc. |
| 2 | Unknown type returns false | ✅ PASS | Line 39: `case WidgetType::Unknown: return false;` |
| 3 | Uses switch/array, not vtable | ⚠️ **CONDITIONAL PASS** | Switch is used for dispatch (✅), BUT DisplayMode uses virtual methods (❌). See Finding #1. |
| 4 | TextWidget with alignment | ✅ PASS | `TextWidget.cpp:40-49` implements left/center/right via switch on `spec.align`. |
| 5 | ClockWidget 30s cache | ✅ PASS (Synthesis Applied) | `ClockWidget.cpp:30-39` implements cache window. |
| 6 | LogoWidget uses bitmap blit | ✅ PASS | `LogoWidget.cpp:28-33` calls `drawBitmapRGB565`, never calls `fillRect`. |
| 7 | Bounds clipping | ✅ PASS | All three widgets check `w`/`h` floor and return true (safe no-op). |
| 8 | Unit tests pass | ⚠️ **UNVERIFIABLE** | Test file not provided in review. Finding #4 above. |

**AC Verdict:** 6/8 fully passing, 1 conditional (AC#3 needs clarification), 1 unverifiable (AC#8 test file needed).

---

## 🔨 Task Completion Audit

| Task # | Description | Status | Issue |
|--------|-------------|--------|-------|
| 1 | WidgetRegistry header | ✅ DONE | `firmware/core/WidgetRegistry.h` exists with all declarations. |
| 2 | WidgetRegistry dispatch | ✅ DONE | `firmware/core/WidgetRegistry.cpp` implements. |
| 3 | TextWidget | ✅ DONE | Files created, implementation matches AC#4. |
| 4 | ClockWidget | ✅ DONE | Files created, 30s cache implemented (synthesis fix applied). |
| 5 | LogoWidget | ✅ DONE | Files created, uses bitmap blit (no fillRect). |
| 6 | platformio.ini | ✅ DONE | Both `build_src_filter` and `build_flags` updated. |
| 7 | Unity tests | ⚠️ **UNVERIFIABLE** | Test file `test_widget_registry/test_main.cpp` not provided. |
| 8 | Build verify | ⚠️ **PARTIAL** | grep scope issue (Finding #2) — task claim is technically true but misleading. |

**Task Verdict:** 6/8 fully verifiable and complete, 1 unverifiable (test file), 1 partially misleading (grep scope).

---

## Evidence Score Calculation

| Category | Count | Score |
|----------|-------|-------|
| 🔴 CRITICAL Issues | 2 | +6.0 |
| 🟠 IMPORTANT Issues | 5 | +5.0 |
| 🟡 MINOR Issues | 2 | +0.6 |
| ✅ CLEAN PASSES (categories with no issues) | 2 (Performance, Security) | -1.0 |

**Total Evidence Score: `5.8`**

| Score Range | Verdict |
|-------------|---------|
| score ≥ 7 | 🔴 REJECT |
| 3 ≤ score < 7 | 🟠 **MAJOR REWORK** ← **YOU ARE HERE** |
| score < 3 | 🟢 APPROVED |

**Verdict: 🟠 MAJOR REWORK REQUIRED**

---

## 🛠️ Required Fixes (Before Merge)

### Fix #1: Clarify AC#3 Virtual Dispatch Intent
**File:** `firmware/interfaces/DisplayMode.h` + `Story documentation`

**Issue:** AC#3 says "not virtual-function vtable" but DisplayMode uses 5 virtual methods. Need product owner decision.

**Options:**
- Option A: Accept virtual dispatch as intentional (violates AC#3 spirit, but practical)
- Option B: Remove virtual methods and use static dispatch everywhere (major refactor, likely not intended)
- Option C: Reword AC#3 to clarify "dispatch mechanism is non-virtual" (only the registry.dispatch() call, not the mode interface)

**Recommended:** Option C (least impact, clarifies intent)

---

### Fix #2: Correct Task 8 Grep Validation Scope
**File:** `Story documentation` + `firmware/` (re-run validation)

**Issue:** Task 8 grep command scope is too narrow, missing virtual keywords in DisplayMode.h.

**Action:** Re-run comprehensive grep validation:
```bash
grep -r "virtual" firmware/interfaces/DisplayMode.h firmware/core/WidgetRegistry.cpp firmware/widgets/
```

**Expected output:** 5 virtual keywords in DisplayMode.h (acceptable if AC#3 clarified as Option C above)

---

### Fix #3: Add LogoWidget Color Validation
**File:** `firmware/core/WidgetRegistry.cpp` OR `firmware/core/LayoutStore.cpp`

**Issue:** `spec.color = 0x0000` (black) renders invisible due to transparent-black convention in drawBitmapRGB565.

**Suggested Fix:**
```cpp
// In WidgetRegistry::dispatch or LayoutStore::validateSchema
if (type == WidgetType::Logo && spec.color == 0x0000) {
    SystemStatus::set(Subsystem::WIDGET, StatusLevel::WARNING,
        "Logo widget with black color (0x0000) will render invisible");
    LOG_W("Widget", "Logo %s: black color (0x0000) is transparent — use 0x0001+", spec.id);
}
```

---

### Fix #4: Provide AC#8 Test File for Verification
**File:** `firmware/test/test_widget_registry/test_main.cpp`

**Issue:** Story claims tests exist but file not provided in review. Need to verify:
- Test actually calls dispatch 50 times ✅
- getLocalTime mocked properly ✅
- Assertion is `call_count == 1` (not weaker) ✅

**Action:** Provide full test source code OR update story to mark AC#8 as "verified separately via pio test output" with actual test run log.

---

### Fix #5: Add NVS Key Collision Prevention
**File:** `firmware/core/LayoutStore.cpp` or new validation function

**Issue:** Widget IDs could collide with ConfigManager NVS keys (e.g., widget id="timezone").

**Suggested Fix:**
```cpp
static const char* RESERVED_WIDGET_IDS[] = {
    "brightness", "text_color_r", "text_color_g", "text_color_b",
    "timezone", "sched_enabled", "sched_dim_start", "sched_dim_end", "sched_dim_brt",
    "disp_mode", "layout_active", "upg_notif"
};

bool isSafeWidgetId(const char* id) {
    for (const auto& reserved : RESERVED_WIDGET_IDS) {
        if (strcmp(id, reserved) == 0) return false;
    }
    return isSafeLayoutId(id);  // existing validation
}
```

Add call in LayoutStore::validateSchema.

---

## Summary & Recommendations

**Current Status:** 🟠 **MAJOR REWORK** — Story is ~70% complete but has 3 critical/important blockers.

**Path to Approval:**
1. ✅ Clarify AC#3 (virtual dispatch intent) — 1 day
2. ✅ Fix Task 8 grep validation — 1 hour
3. ✅ Add LogoWidget color validation — 2 hours
4. ✅ Provide AC#8 test file — 1 hour
5. ✅ Add NVS collision prevention — 2 hours

**Estimated Total Rework:** 1-2 days for careful fixes + testing

**Recommend:** Request clarification on AC#3 from product owner before proceeding. If AC#3 is clarified to accept virtual dispatch (Option C), remaining fixes are low-risk implementation changes.

---

**Review Complete.** Evidence Score: **5.8** → **🟠 MAJOR REWORK REQUIRED**

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
<var name="session_id">91cfbeeb-3992-43b7-b9a8-b0bae0f86c84</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="bd2de04b">embedded in prompt, file id: bd2de04b</var>
<var name="story_id">le-1.2</var>
<var name="story_key">le-1-2-widget-registry-and-core-widgets</var>
<var name="story_num">2</var>
<var name="story_title">2-widget-registry-and-core-widgets</var>
<var name="template">False</var>
<var name="timestamp">20260417_1409</var>
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
      - Commit message format: fix(component): brief description (synthesis-le-1.2)
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