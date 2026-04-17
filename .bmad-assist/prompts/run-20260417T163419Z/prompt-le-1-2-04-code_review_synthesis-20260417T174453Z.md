<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T174453Z -->
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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial code review complete, Christian.**

**Story:** `_bmad-output/implementation-artifacts/stories/le-1-2-widget-registry-and-core-widgets.md`  
**Scope:** `firmware/core/WidgetRegistry.*`, `firmware/widgets/**`, `firmware/test/test_widget_registry/**`, `firmware/platformio.ini` (per story zone). Git reality was **not** re-verified with shell (read-only constraint); findings are from the story text plus the implementation sources read in this session.

---

## Review summary

| Category | Issues found |
|----------|----------------|
| Git vs story discrepancies | Not verified (no `git diff` in read-only pass) |
| AC implementation gaps | 2 partial / risk items |
| Task completion / test honesty | 1 |
| SOLID / boundaries | 2 |
| Hidden bugs | 1 concrete |
| Performance footguns | 2 |
| Security vulnerabilities | 0 |
| **Total distinct issues** | **10** |

---

## Evidence score summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| IMPORTANT | Clock cache does not throttle `getLocalTime()` when time never becomes valid — failure path can call it every `renderClock` | `firmware/widgets/ClockWidget.cpp` | +1 |
| IMPORTANT | Single global clock string/cache — multiple clock widgets in one layout would share state (LE-1.3 collision) | `firmware/widgets/ClockWidget.cpp` | +1 |
| IMPORTANT | `isKnownType("flight_field"|"metric")` is true but `dispatch` returns false — “known” does not mean “renderable” | `firmware/core/WidgetRegistry.cpp` | +1 |
| IMPORTANT | AC #7 “clip safely / no OOB” for text is delegated to GFX only; no explicit vertical clip in widget | `firmware/widgets/TextWidget.cpp` | +1 |
| IMPORTANT | `test_clock_cache_reuse` accepts `getTimeCallCount() == 0` and never asserts the success path (≤1 is trivially true on failure storm until counter logic changes) | `firmware/test/test_widget_registry/test_main.cpp` | +1 |
| MINOR | `LogoWidget` refills 256× `uint16_t` every frame — zero heap but unnecessary CPU on hot path | `firmware/widgets/LogoWidget.cpp` | +0.3 |
| MINOR | `DisplayUtils::drawBitmapRGB565` comment still claims “always 32×32”; `LogoWidget` passes 16×16 | `firmware/utils/DisplayUtils.cpp` | +0.3 |
| MINOR | `WidgetRegistry.cpp` top comment says enum values fall through `default`; actual code is fully enumerated cases + trailing `return false` | `firmware/core/WidgetRegistry.cpp` | +0.3 |
| MINOR | Story Task 7 calls for a static assertion / in-test guard for “no fillRect”; delivered gate is grep-only (no compile-time test in TU) | Story vs `test_main.cpp` | +0.3 |
| MINOR | `drawBitmapRGB565` treats RGB565 `0` as transparent — `spec.color == 0` yields an “invisible” stub | `firmware/utils/DisplayUtils.cpp` + `LogoWidget.cpp` | +0.3 |

| Clean pass | Reason |
|------------|--------|
| Security | No new trust boundary, no parsing of untrusted input in LE-1.2 render path |
| Type safety | C++ POD `WidgetSpec` + enum dispatch; no `void*` abuse in scope |
| PEP8 | N/A (firmware C++) |

**Clean pass count:** 3 → **−1.5** adjustment  

### Evidence score: **5.2**

| Score | Verdict |
|-------|---------|
| **5.2** | **MAJOR REWORK** (address clock failure semantics, tighten tests/API clarity, small perf/doc fixes) |

---

## Architectural sins

- **[6/10] Single Responsibility / isolation:** `ClockWidget` uses module-global statics for cache and test hooks. That is acceptable for a single widget on the wall today but violates the story’s “stateless render” narrative as soon as LE-1.3 lays out **two** clocks — one buffer, one minute, one `getLocalTime` policy for both.  
  - Fix: move cache into caller-owned state (LE-1.3 layout instance) or key cache by `spec.id` in a fixed table.

- **[5/10] Dependency direction:** `widgets/*.h` include `core/WidgetRegistry.h`, which pulls `interfaces/DisplayMode.h` (matrix + `LayoutResult` + flight model types). Widgets only need `WidgetSpec` + `RenderContext`; the include graph is heavier than necessary for a reusable widget layer.  
  - Fix: split `WidgetSpec` / constants into `core/WidgetSpec.h` (or `widgets/WidgetTypes.h`) included by both registry and widgets; keep `RenderContext` forward or thin.

- **API semantics:** “Known type” for `flight_field` / `metric` vs `dispatch` → `false` is a future footgun for LE-1.3 orchestration (“save succeeded, render skipped silently”). Document or split `isRegisteredType` vs `isRenderableType`.

---

## Pythonic crimes and readability

- N/A for C++. Minor: misleading comments (`DisplayUtils` 32×32 note, `WidgetRegistry.cpp` “default” narrative) reduce trust in onboarding readers.

---

## Performance and scalability

- **[Medium]** Failed or not-yet-valid `getLocalTime`: `s_lastClockUpdateMs` is only updated on **success**. While `s_lastClockUpdateMs` stays `0`, the condition `s_lastClockUpdateMs == 0 || (nowMs - s_lastClockUpdateMs) > 30000` stays true **every frame**, so `getLocalTime` runs at display rate (e.g. ~20 Hz) — the opposite of the spike’s intent.  
  - Fix: on failure, set `s_lastClockUpdateMs = nowMs` (backoff) or use a separate “last attempt” timestamp.

- **[Low]** `LogoWidget`: full `for` fill of 256 pixels each invocation — cheap vs heap but not free; precompute once per color change or document as acceptable for V1 stub only.

---

## Correctness and safety

- **Bug (clock throttle on error):** As above — AC #5 text says “subsequent frames reuse the cached string”; it does not say “unless time is broken,” but the implementation still violates the **performance** contract of the spike when NTP is down. Reproduction: run firmware before NTP sync (or mock `getLocalTime` false), attach serial counter — calls scale with frame rate, not with 30 s.

- **`drawBitmapRGB565` + color 0:** If `spec.color == 0x0000`, every pixel is skipped as “transparent”; logo stub draws nothing. Edge case for tests/layouts using black.

---

## Lying tests

- **`test_clock_cache_reuse`:** Asserts `getTimeCallCount() <= 1`. Count `0` passes even when **no** successful time read occurred, and the test does **not** catch the “call `getLocalTime` every frame on failure” bug because the counter only increments on success. This is exactly the class of test that passes while the hot path misbehaves.

- **`test_text_alignment_all_three`:** Comment admits no pixel assertions — it only proves dispatch does not crash with `matrix == nullptr`; it does **not** prove center/right x math (AC #4 alignment) is correct.

---

## Maintainability issues

- **Dual source of truth:** `LayoutStore.cpp` allowlist vs `WidgetRegistry::fromString()` — story warns correctly; still a **tech-debt bomb** with explosion radius LE-1.8 (new type added in one list only).

- **Test `delay(2000)` in `setup()`:** Adds 2 s to every on-device test run for this suite — small friction at scale.

---

## Suggested fixes (prioritized)

1. **ClockWidget:** On `getLocalTime` failure, update a `lastAttemptMs` (or set `s_lastClockUpdateMs = nowMs` with documented backoff) so the refresh branch does not run every frame; keep retry at ≤1 Hz or use the 30 s window for **attempts**, not only successes.

2. **Tests:** Add a compile-time or link-time guard you actually run in CI: e.g. a trivial TU that `#include`s logo cpp symbols only if needed, or a test that fails if a preprocessor flag is wrong — or at minimum assert `getTimeCallCount() == 1` under a injected/mock time path (ESP32: weak symbol override for `getLocalTime` in test build).

3. **API/docs:** Clarify `isKnownType` vs renderability; or rename / add `isRenderable(WidgetType)`.

4. **LogoWidget:** Document black-as-transparent; or `if (spec.color == 0) use 0x0001` for stub only.

5. **DisplayUtils:** Fix the “32×32” comment to match parameterized `w,h` behavior.

6. **LE-1.3 planning:** If multiple clocks are allowed, remove global static cache from `ClockWidget.cpp` in that story.

---

**Issues fixed in this review:** 0 (read-only)  
**Action items:** Treat clock backoff + test strengthening as merge blockers if you care about NTP-flap behavior and AC #5 as a **systems** requirement, not only a happy-path demo.

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
| 🟢 CLEAN PASS | AC Implementation | All | -0.5 |
| 🟢 CLEAN PASS | Task Completion | All | -0.5 |
| 🟢 CLEAN PASS | Code Quality (SOLID) | All | -0.5 |
| 🟢 CLEAN PASS | Hidden Bugs / Safety | All | -0.5 |
| 🟢 CLEAN PASS | Performance | All | -0.5 |
| 🟢 CLEAN PASS | Security | All | -0.5 |
| 🟢 CLEAN PASS | Tests | All | -0.5 |
| 🟢 CLEAN PASS | Style | All | -0.5 |

### Evidence Score: -4.0

| Score | Verdict |
|-------|---------|
| **-4.0** | **EXEMPLARY** |

---

## 🏛️ Architectural Sins

✅ No architectural violations detected. The implementation strictly adheres to the static dispatch pattern (decision D2/V0 spike) and respects the zero-heap render hot path requirement.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows the project's C++ style guidelines: 
- Proper use of `pragma once`
- Consistent naming (`PascalCase` for types, `camelCase` for functions, `_member` for private members)
- Zero heap allocation in render paths using `char*` overloads
- Clear, purpose-driven directory structure (`firmware/widgets/`)

---

## ⚡ Performance & Scalability

✅ No performance issues. The `WidgetRegistry` dispatch and render functions are optimized for the ESP32:
- **Zero dynamic memory allocation** on the render path.
- **`switch` dispatch** replaces virtual function overhead.
- **Clock cache mechanism** correctly reduces expensive `getLocalTime` syscalls.
- **Bitmap blit optimization** in `LogoWidget` confirms adherence to the 1.23x render-time requirement.

---

## 🐛 Correctness & Safety

✅ The implementation is robust:
- **`WidgetRegistry` dispatch** handles unknown types and out-of-range enums safely.
- **`TextWidget` and `ClockWidget`** correctly handle minimum dimension floors and `ctx.matrix == nullptr` (allowing for portable unit testing).
- **`DisplayUtils char*` overloads** ensure zero-heap performance during widget rendering.
- **Test coverage** is comprehensive, covering all ACs, including edge cases like empty content, undersized widgets, and cache re-use validation.

---

## 🔧 Maintainability Issues

✅ The code is highly maintainable:
- The **two-touch-point extensibility pattern** (adding a file in `widgets/` + one line in `WidgetRegistry` switch) is perfectly executed, ensuring low friction for future widget additions.
- The **`platformio.ini` updates** are clean and idiomatic.
- **Comments and documentation** are excellent, clearly explaining the "why" (especially around performance choices like avoiding `fillRect`).

---

## 🛠️ Suggested Fixes

✅ No actionable maintainability issues found.

---

**Issues Fixed:** 0
**Action Items Created:** 0

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

# 🔥 ADVERSARIAL CODE REVIEW: Story LE-1.2 - widget-registry-and-core-widgets

**Story:** le-1-2-widget-registry-and-core-widgets  
**Reviewed:** 2026-04-17  
**Reviewer:** Senior Code Auditor (Adversarial Mode)

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| AC Implementation Gaps | 2 |
| Task Completion Lies | 1 |
| SOLID Violations | 3 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Test Quality Issues | 3 |
| Security Issues | 1 |
| Type Safety Issues | 2 |
| **Total Issues** | **15** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task marked [x] but incomplete: AC #8 build gate documentation missing | Task 8 | +3 |
| 🔴 CRITICAL | AC #8 "grep returns zero results" claim is untestable and incomplete | ClockWidget.cpp | +3 |
| 🟠 IMPORTANT | renderClock static cache not reset between test runs - tests will fail after first run | ClockWidget.cpp:36 | +1 |
| 🟠 IMPORTANT | WidgetRegistry::dispatch() has no bounds checking on WidgetType enum value cast | WidgetRegistry.cpp | +1 |
| 🟠 IMPORTANT | TextWidget alignment calculation can overflow with large spec.w | TextWidget.cpp:64 | +1 |
| 🟠 IMPORTANT | LogoWidget static buffer not cleared - can leak previous widget's color data | LogoWidget.cpp:30 | +1 |
| 🟠 IMPORTANT | Missing validation: WidgetSpec.content can be uninitialized | WidgetRegistry.h:65 | +1 |
| 🟠 IMPORTANT | ClockWidget::renderClock missing getLocalTime() null guard | ClockWidget.cpp:46 | +1 |
| 🟡 MINOR | Memory Safety: char array buffer overflows not prevented in DisplayUtils calls | Multiple files | +0.3 |
| 🟡 MINOR | Test infrastructure assumes ctx.matrix == nullptr is safe but never actually runs tests | test_widget_registry/test_main.cpp | +0.3 |

### Evidence Score: 11.6

| Score | Verdict |
|-------|---------|
| **11.6** | **🔴 MAJOR REWORK** |

---

## 🏛️ Architectural Sins

### 1. Static Dispatch Missing Enum Validation [7/10]
- **Issue:** `WidgetRegistry::dispatch()` accepts arbitrary `uint8_t` cast as `WidgetType` without validation. A caller can pass `static_cast<WidgetType>(99)` and hit the `default` case silently.
- 📍 `firmware/core/WidgetRegistry.cpp:52-60`
- **Code:**
```cpp
bool WidgetRegistry::dispatch(WidgetType type, const WidgetSpec& spec, const RenderContext& ctx) {
    switch (type) {
        case WidgetType::Text:        return renderText(spec, ctx);
        // ... no bounds checking before switch
    }
    return false;  // silently ignores invalid types
}
```
- 💡 **Fix:** Add explicit validation:
```cpp
if (static_cast<uint8_t>(type) > static_cast<uint8_t>(WidgetType::Metric)) {
    return false;  // or log error
}
```

### 2. Static Cache Violates Single Responsibility [6/10]
- **Issue:** `renderClock()` owns both rendering logic AND cache management. Cache is function-scoped static, making it impossible to reset between tests without `ClockWidgetTest::resetCacheForTest()` - a production function called only by tests. This violates SRP.
- 📍 `firmware/widgets/ClockWidget.cpp:36-40`
- 💡 **Fix:** Extract cache into a separate struct or move to ClockWidgetTest as a test fixture.

### 3. Widget Alignment Calculation Not Saturating [5/10]
- **Issue:** TextWidget alignment calculation can produce negative drawX:
```cpp
int16_t drawX = spec.x + (int16_t)((int)spec.w - textPixels);  // if textPixels > w, this is negative
```
- 📍 `firmware/widgets/TextWidget.cpp:64`
- 💡 **Fix:** Saturate at spec.x: `drawX = std::max(spec.x, spec.x + (int16_t)((int)spec.w - textPixels));`

---

## 🐍 Pythonic Crimes & Readability

### 1. Inconsistent Error Handling Across Widgets [4/10]
- **Issue:** Some widgets check `ctx.matrix == nullptr` at start, others have varied guards:
  - `renderText()`: Early return ✅
  - `renderClock()`: Dimension check BEFORE matrix check ❌
  - `renderLogo()`: Matrix check BEFORE dimension check ✅
- 📍 Multiple files in `firmware/widgets/`
- 💡 **Fix:** Standardize guard order in ALL render functions:
```cpp
bool renderWidget(const WidgetSpec& spec, const RenderContext& ctx) {
    if (ctx.matrix == nullptr) return true;  // TEST HARNESS FIRST
    if (dimension_too_small()) return true;   // THEN CHECKS
    // ... actual rendering ...
}
```

### 2. Magic Constants in Multiple Files [3/10]
- **Issue:** `WIDGET_CHAR_W = 6` and `WIDGET_CHAR_H = 8` are defined in header but re-derived:
  - `"HH:MM"` is 5 chars hardcoded in comment instead of `5 * WIDGET_CHAR_W`
  - `kLogoW = 16, kLogoH = 16` are local constants instead of shared
- 📍 `firmware/widgets/ClockWidget.cpp:50`, `firmware/widgets/LogoWidget.cpp:13-14`

### 3. Type Safety: WidgetSpec.content Initialization [2/10]
- **Issue:** `WidgetSpec` struct has `char content[48]` with NO guaranteed null-termination. A caller could pass `spec.content` uninitialized.
- 📍 `firmware/core/WidgetRegistry.h:65`
- 💡 **Fix:** Add initialization in struct definition:
```cpp
struct WidgetSpec {
    // ...
    char content[48] = {};  // C++11 zero-initialization
};
```

---

## ⚡ Performance & Scalability

### 1. Clock Cache Not Reset Between Test Runs [7/10]
- **Issue:** The static `s_lastClockUpdateMs` and `s_clockBuf` persist across Unity test iterations. Run `test_dispatch_clock_returns_true` twice and `s_lastClockUpdateMs` is non-zero, causing the second run to skip cache refresh.
- 📍 `firmware/widgets/ClockWidget.cpp:36-37`
- **Evidence:** Task 7 `test_clock_cache_reuse` explicitly uses `ClockWidgetTest::resetCacheForTest()` - this is a PRODUCTION BAND-AID for broken test isolation.
- 💡 **Fix:** 
  - Add `TEST_SETUP()` that calls `ClockWidgetTest::resetCacheForTest()` in every clock test
  - OR use `static` initialization guard instead of persistent state

---

## 🐛 Correctness & Safety

### 1. CRITICAL: AC #8 Task Marked [x] But Incomplete [10/10]
- **Issue:** Task 8 states: "✅ `grep -r "fillRect" firmware/widgets/` returns zero results (logo path clean)"`
- **Reality:** AC #8 test requirement `test_logo_no_fillrect` is NOT IMPLEMENTED. The .cpp files exist but there's NO ACTUAL TEST that validates fillRect is absent.
- 📍 `firmware/test/test_widget_registry/test_main.cpp` (MISSING implementation)
- **Evidence:** Story dev notes Task 8 says "✅ `pio test ... --filter test_widget_registry` — test TU compiles cleanly" but the actual test code shown in the story is NOT PRESENT IN THE SOURCE FILE.
- 💡 **Fix:** Add actual test:
```cpp
void test_logo_no_fillrect() {
    // This test exists in story but NOT in actual firmware/test/test_widget_registry/test_main.cpp
    // Need to add grep-based verification or actual render test
}
```

### 2. CRITICAL: Clock getLocalTime() Null Handling [9/10]
- **Issue:** In ClockWidget.cpp, `getLocalTime(&tinfo, 0)` can fail but the code continues:
```cpp
if (s_lastClockUpdateMs == 0 || (nowMs - s_lastClockUpdateMs) > 30000) {
    struct tm tinfo;
    if (getLocalTime(&tinfo, 0)) {  // If this fails...
        snprintf(s_clockBuf, ...);
    }
    // s_lastClockUpdateMs is NOT advanced, but what if...
}
// Next frame, we retry immediately, then 30s later we retry again
// This is CORRECT per design, BUT there's no guard against infinite retry loop
```
- 📍 `firmware/widgets/ClockWidget.cpp:41-49`
- **Hidden Bug:** If NTP never syncs (WiFi down, network unreachable), `getLocalTime` always fails and we retry every frame indefinitely. The "--:--" placeholder is fine, but there's NO LOGGING of this condition. A user sees frozen clock and no indication why.
- 💡 **Fix:** Add event logging on first failure:
```cpp
static bool s_getLocalTimeFailed = false;
if (!getLocalTime(&tinfo, 0)) {
    if (!s_getLocalTimeFailed) {
        LOG_W("ClockWidget", "getLocalTime failed - NTP not synced yet");
        s_getLocalTimeFailed = true;
    }
}
```

### 3. LogoWidget Static Buffer Color Persistence [7/10]
- **Issue:** `renderLogo()` fills a static buffer with `spec.color` on EVERY CALL. But if a widget is skipped (undersized zone), the previous color data remains in the buffer:
```cpp
static uint16_t s_buf[kLogoW * kLogoH];
for (int i = 0; i < kLogoW * kLogoH; ++i) {
    s_buf[i] = spec.color;  // Always fills
}
```
- If widget 1 (red) renders, then widget 2 is undersized and skipped, then widget 3 (blue) renders with larger zone, the blue widget reads from a clean buffer BUT if widget 4 is undersized, it returns without filling. Does drawBitmapRGB565 guarantee clearing? No.
- 📍 `firmware/widgets/LogoWidget.cpp:29-33`
- 💡 **Fix:** Only fill what you'll render:
```cpp
if ((int)spec.w < 8 || (int)spec.h < 8) return true;
// NOW fill the buffer
static uint16_t s_buf[kLogoW * kLogoH];
for (int i = 0; i < kLogoW * kLogoH; ++i) s_buf[i] = spec.color;
```

### 4. WidgetSpec Buffer Overflow Risk [6/10]
- **Issue:** `WidgetSpec.content[48]` is passed to `DisplayUtils::truncateToColumns(content, ...)`. If `content` is not null-terminated, truncateToColumns will read past the buffer.
- 📍 `firmware/widgets/TextWidget.cpp:47`, `firmware/widgets/ClockWidget.cpp:52`
- **Caller Responsibility:** LE-1.3 must guarantee `content` is null-terminated, but there's no assertion in the render functions to verify this.
- 💡 **Fix:** Add debug assertion:
```cpp
assert(spec.content[sizeof(spec.content) - 1] == '\0' && "content must be null-terminated");
```

---

## 🎭 Test Quality Issues

### 1. Tests Assume Hardware-Free Path But Never Verify [8/10]
- **Issue:** The story dev notes say: "Test harness uses `ctx.matrix = nullptr` to exercise dispatch without real hardware."
- **Reality:** The test scaffold shown in Dev Notes shows:
```cpp
static RenderContext makeCtx() {
    RenderContext ctx{};
    ctx.matrix = nullptr;  // test env has no real matrix
    return ctx;
}
```
- BUT the actual test file `firmware/test/test_widget_registry/test_main.cpp` is NOT PROVIDED in this review. The story CLAIMS tests exist but we cannot verify them without seeing the actual file.
- 📍 Story File List claims `firmware/test/test_widget_registry/test_main.cpp` but file is not in the context
- 🎭 **Lying Claim:** "✅ `pio test -e esp32dev --filter test_widget_registry --without-uploading --without-testing` — test TU compiles cleanly" - WE CANNOT VERIFY THIS, the test file is absent from review materials.

### 2. Test Isolation Broken by Static Cache [7/10]
- **Issue:** `test_clock_cache_reuse` depends on `ClockWidgetTest::resetCacheForTest()` being called. If a different test runs before it and caches the time, the test fails.
- **Evidence:** Story AC #8 subtask says "use a mockable time source or counter" but the implementation uses `static uint32_t s_lastClockUpdateMs` - NOT mockable without linker tricks.
- 📍 `firmware/widgets/ClockWidget.cpp` + missing `firmware/test/test_widget_registry/test_main.cpp`

### 3. platformio.ini build_src_filter Missing Validation [5/10]
- **Issue:** Task 6 claims to add `+<../widgets/*.cpp>` to build_src_filter. 
- **Reality:** The provided `firmware/platformio.ini` DOES have the addition ✅, but there's NO ASSERTION that the build actually includes these files. The story says "verify both `esp32dev_le_baseline` and `esp32dev_le_spike` envs inherit the change via `extends`" but provides NO TEST OUTPUT showing the build actually found these files.
- 📍 `firmware/platformio.ini:45`

---

## 🔒 Security Issues

### 1. String Handling in truncateToColumns [6/10]
- **Issue:** `DisplayUtils::truncateToColumns` signature (from story) is `(const char*, int, char* out, size_t)`. But there's NO VALIDATION in the callers that `out` buffer is actually large enough or that `spec.content` is null-terminated.
- **Attack Vector:** Malformed JSON (LE-1.3 job) could pass `content[48]` without null terminator, causing `strlen()` in truncateToColumns to read past buffer end.
- 📍 `firmware/widgets/TextWidget.cpp:47`, `firmware/widgets/ClockWidget.cpp:52`
- 💡 **Fix:** Add bounds check:
```cpp
if (spec.content[47] != '\0') {
    spec.content[47] = '\0';  // Force null termination as safety net
}
```

---

## 💣 Tech Debt & Maintainability

### 1. Incomplete Story Documentation [5/10]
- **Issue:** The story File List claims 9 new files exist:
  - ✅ `firmware/core/WidgetRegistry.h`
  - ✅ `firmware/core/WidgetRegistry.cpp`
  - ✅ `firmware/widgets/TextWidget.h`
  - ✅ `firmware/widgets/TextWidget.cpp`
  - ✅ `firmware/widgets/ClockWidget.h`
  - ✅ `firmware/widgets/ClockWidget.cpp`
  - ✅ `firmware/widgets/LogoWidget.h`
  - ✅ `firmware/widgets/LogoWidget.cpp`
  - ❓ `firmware/test/test_widget_registry/test_main.cpp` (NOT PROVIDED IN REVIEW)

The test file is referenced in Task 7 but NOT INCLUDED IN THE CODE REVIEW CONTEXT. This is a **CRITICAL DOCUMENTATION GAP** - we cannot verify tests are actually implemented.

### 2. AC #8 "Build and verify" Incomplete [7/10]
- **Issue:** Task 8 has 6 subtasks:
  1. ✅ Build claims success
  2. ✅ Binary size within limits claim
  3. ❓ Test compile claim (test file not provided)
  4. ✅ grep for fillRect claim
  5. ✅ grep for virtual claim
  6. ❌ **NO CLAIM that tests actually run and PASS**
- Task says "verify" but provides no test output, no passing assertion counts, no CI output.

---

## 🛠️ Suggested Fixes

### Fix 1: Add WidgetType Enum Bounds Validation

**File:** `firmware/core/WidgetRegistry.cpp`

**Issue:** Dispatch accepts arbitrary uint8_t values without validation, silently failing for invalid types.

**Corrected code:**
```cpp
bool WidgetRegistry::dispatch(WidgetType type,
                              const WidgetSpec& spec,
                              const RenderContext& ctx) {
    // Validate enum value is in known range
    const uint8_t typeValue = static_cast<uint8_t>(type);
    const uint8_t maxKnownType = static_cast<uint8_t>(WidgetType::Metric);
    
    if (typeValue > maxKnownType && typeValue != static_cast<uint8_t>(WidgetType::Unknown)) {
        LOG_E("WidgetRegistry", "Invalid WidgetType: " + String(typeValue));
        return false;
    }

    switch (type) {
        case WidgetType::Text:        return renderText(spec, ctx);
        case WidgetType::Clock:       return renderClock(spec, ctx);
        case WidgetType::Logo:        return renderLogo(spec, ctx);
        case WidgetType::FlightField: return false;
        case WidgetType::Metric:      return false;
        case WidgetType::Unknown:     return false;
    }
    return false;
}
```

---

### Fix 2: Reset Clock Cache Between Tests

**File:** `firmware/widgets/ClockWidget.cpp` and test suite

**Issue:** Static cache persists across test runs, causing test failures on second execution.

**Changes needed:**
```cpp
// In ClockWidget.cpp - add guard for test mode:
#ifdef UNIT_TEST
    namespace ClockWidgetTest {
        void resetCacheForTest() {
            s_lastClockUpdateMs = 0;
            strcpy(s_clockBuf, "--:--");
        }
        uint32_t getTimeCallCount() { return s_timeCallCount; }
        void resetTimeCallCount() { s_timeCallCount = 0; }
    }
#endif

// In test suite (missing file):
void setUp() {
    ClockWidgetTest::resetCacheForTest();
    ClockWidgetTest::resetTimeCallCount();
}

void test_clock_cache_reuse() {
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Clock, 0, 0, 80, 8);
    
    // First call should update cache
    TEST_ASSERT_TRUE(renderClock(spec, ctx));
    TEST_ASSERT_EQUAL_UINT32(1, ClockWidgetTest::getTimeCallCount());
    
    // Next 49 calls should reuse cache
    for (int i = 0; i < 49; i++) {
        TEST_ASSERT_TRUE(renderClock(spec, ctx));
    }
    
    // Call count should still be 1 (cache hit)
    TEST_ASSERT_EQUAL_UINT32(1, ClockWidgetTest::getTimeCallCount());
}
```

---

### Fix 3: Standardize Widget Guard Order

**File:** `firmware/widgets/ClockWidget.cpp`

**Issue:** Inconsistent order of null checks vs dimension checks across widget render functions.

**Corrected code:**
```cpp
bool renderClock(const WidgetSpec& spec, const RenderContext& ctx) {
    // ALWAYS check ctx.matrix first (test harness path)
    if (ctx.matrix == nullptr) return true;
    
    // THEN check dimensions (AC #7)
    if ((int)spec.w < 6 * WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) {
        return true;
    }
    
    // THEN do cache refresh and rendering...
    uint32_t nowMs = millis();
    if (s_lastClockUpdateMs == 0 || (nowMs - s_lastClockUpdateMs) > 30000) {
        struct tm tinfo;
        if (getLocalTime(&tinfo, 0)) {
            snprintf(s_clockBuf, sizeof(s_clockBuf), "%02d:%02d",
                     tinfo.tm_hour, tinfo.tm_min);
            s_lastClockUpdateMs = nowMs;
        }
    }

    // ... rest of rendering ...
}
```

---

## ⚠️ Critical Action Items

1. **BLOCKER:** Provide `firmware/test/test_widget_registry/test_main.cpp` source code - this file is referenced in story but not included in review
2. **BLOCKER:** Confirm AC #8 `test_logo_no_fillrect` is actually implemented (static analysis requirement from AC #8)
3. **HIGH:** Add enum bounds validation to `WidgetRegistry::dispatch()`
4. **HIGH:** Fix clock cache test isolation with `setUp()` / `tearDown()` lifecycle
5. **HIGH:** Standardize widget null-check and dimension-check order
6. **MEDIUM:** Add safe null-termination guarantee for `WidgetSpec.content`
7. **MEDIUM:** Add NTP sync failure logging in ClockWidget
8. **MEDIUM:** Reorder LogoWidget dimension check before buffer fill
9. **MINOR:** Extract magic numbers (5, 16) into named constants

---

## Summary

**Total Issues Found:** 15  
**Critical Issues:** 3  
**Important Issues:** 7  
**Minor Issues:** 5  

**Verdict:** 🔴 **MAJOR REWORK REQUIRED**

The story claims completion but several critical components are incomplete or have test isolation issues. Most concerning:

1. **AC #8 test file is missing from review** - we cannot verify tests compile or pass
2. **Clock cache breaks test isolation** - tests will fail on second run without manual reset
3. **Type safety gaps** - WidgetSpec buffers not null-terminated, dispatch doesn't validate enum
4. **Inconsistent error handling** - widgets have different guard orders and validation patterns

Recommend: **Return to dev for fixes before merge**

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
<var name="session_id">00970e77-282c-4793-a6f2-a630a5409169</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="bd2de04b">embedded in prompt, file id: bd2de04b</var>
<var name="story_id">le-1.2</var>
<var name="story_key">le-1-2-widget-registry-and-core-widgets</var>
<var name="story_num">2</var>
<var name="story_title">2-widget-registry-and-core-widgets</var>
<var name="template">False</var>
<var name="timestamp">20260417_1344</var>
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