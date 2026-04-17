<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T175601Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story le-1.2

You are synthesizing 2 independent code review findings.

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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial code review — Story LE-1.2 (widget-registry-and-core-widgets)**  
**Scope:** `firmware/core/WidgetRegistry.*`, `firmware/widgets/**`, `firmware/test/test_widget_registry/**`, `firmware/platformio.ini` (per story zone). Git snapshot at conversation start shows many other modified paths; findings below call out **story documentation vs. workspace reality** where relevant.

---

## Review summary

| Category | Issues found |
|----------|----------------|
| Git vs story discrepancies | 1 |
| AC implementation gaps | 0 (all ACs met in spirit; **2 partial** test/contract gaps) |
| Task completion lies | 0 (tasks appear implemented; **1 subtask wording** vs. reality) |
| SOLID / architecture | 2 |
| Hidden bugs / UB | 1 |
| Performance footguns | 2 |
| Security vulnerabilities | 0 |
| Test quality / weak assertions | 3 |
| **Total issues** | **10** |

---

## AC validation (story vs. code)

| AC | Verdict | Notes |
|----|---------|--------|
| AC1 Known type → dispatch → true | **IMPLEMENTED** | `WidgetRegistry::dispatch` + tests with `nullptr` matrix. |
| AC2 Unknown → false, no framebuffer | **IMPLEMENTED** | `Unknown` + default path `return false`; `isKnownType` tests include `nullptr` / `""`. |
| AC3 No vtable; switch / fixed table | **IMPLEMENTED** | `switch (type)` in `WidgetRegistry.cpp`; no virtuals in registry/widgets. |
| AC4 Text + align + truncate | **PARTIAL** | Code implements align + `truncateToColumns`; tests only smoke three alignments with **no pixel/matrix assertions** (test file admits this). |
| AC5 Clock 30s cache | **IMPLEMENTED** | Throttle + `getTimeCallCount`; see **weak test** below. |
| AC6 Logo bitmap not fillRect | **IMPLEMENTED** | `drawBitmapRGB565` path; **no in-TU static assert** — story Task 7 mentions static assert / grep; repo relies on **grep gate**, not a compile-time test. |
| AC7 Min floors / safe clip | **PARTIAL** | Undersized paths return early or rely on GFX clipping; **no on-device test** with a real matrix proving no OOB writes. |
| AC8 Unit tests | **IMPLEMENTED** | Richer than story list (extra FlightField/Metric, clock floor, alignment smoke). |

---

## Task audit ([x] vs. reality)

- **Task 7 / `test_logo_no_fillrect`:** Story text asks for “static assertion / grep”. **Implementation:** enforcement is **grep/CI**, not a Unity or `static_assert` in `test_main.cpp`. Not a false `[x]`, but **task wording oversells** what landed → **MINOR** documentation/test expectation gap.

---

## Git vs story

- **Story File List** claims only `platformio.ini` is modified among existing product files. **Workspace `git status`** shows many other modified files (`ConfigManager`, `main.cpp`, `CustomLayoutMode`, etc.). Either those edits belong to **other stories** and should not ship mixed with LE-1.2, or the **story File List is stale** — **process / scope transparency** issue (**MINOR**, treat as release-hygiene risk).

---

## Findings table (evidence scoring)

| Icon | Severity | Finding | Source | Score |
|------|----------|---------|--------|-------|
| 🟠 | IMPORTANT | `WidgetRegistry::dispatch` `switch (type)` has **no `default`** after exhaustive cases. In C++, switching on a `WidgetType` value that is **not a valid enumerator** is **undefined behavior** (e.g. corrupted `(WidgetType)uint8_t`). A defensive `default: return false;` after known cases preserves AC2 and removes UB. | `firmware/core/WidgetRegistry.cpp` | +1 |
| 🟠 | IMPORTANT | **ClockWidget** uses **file-scope static** `s_clockBuf` / `s_lastClockUpdateMs` — **one global clock** for the whole firmware. Two `Clock` widgets in one layout (LE-1.3) would show the **same** cached string and refresh cadence; layout cannot represent two timezones / formats independently. | `firmware/widgets/ClockWidget.cpp` | +1 |
| 🟠 | IMPORTANT | **LogoWidget** runs a **256-iteration loop** every `renderLogo()` call to refill `s_buf[]`, even when `spec.color` is unchanged — **hot-path CPU** at panel refresh rate; contradicts “stay allocation-free” spirit (CPU, not heap). Prefer one-time fill, color-keyed dirty flag, or `memset`/word fill pattern. | `firmware/widgets/LogoWidget.cpp` | +1 |
| 🟠 | IMPORTANT | **Alignment / AC4 tests** admit they cannot verify behavior: `test_text_alignment_all_three` only checks dispatch returns true with `matrix == nullptr` — **zero evidence** left/center/right math is correct. | `firmware/test/test_widget_registry/test_main.cpp` | +1 |
| 🟡 | MINOR | **Heavy coupling:** `WidgetRegistry.h` includes `DisplayMode.h` (FastLED, `vector`, flight layout) for every TU that only needs `WidgetSpec` / `WidgetType`. Increases compile time and blurs “widget pack” vs. “mode render context” boundaries. | `firmware/core/WidgetRegistry.h` | +0.3 |
| 🟡 | MINOR | **`dispatch(type, spec, ctx)` ignores `spec.type`**. A buggy caller can pass `WidgetType::Text` with `spec.type == Logo` with no assert. LE-1.3 should enforce equality or document “type is authoritative”. | `firmware/core/WidgetRegistry.h` / call sites | +0.3 |
| 🟡 | MINOR | **`ClockWidgetTest` in public header** — test-only surface area exported from `ClockWidget.h`. Risk of accidental use in firmware; prefer `#ifdef PIO_UNIT_TESTING` gating or test-only header. | `firmware/widgets/ClockWidget.h` | +0.3 |
| 🟡 | MINOR | **`test_clock_cache_reuse` phase 2:** “Arm cache” is **timestamp-based**, not “successful `getLocalTime`”. After throttle fix, first call can **fail** time read but still set `s_lastClockUpdateMs`; phase 2 still passes with `getTimeCallCount() == 0` — **does not prove** minute string refresh path. | `firmware/test/test_widget_registry/test_main.cpp` | +0.3 |
| 🟡 | MINOR | **`setup()` uses `delay(2000)`** — adds **2s** to every on-device test run for this suite; most other Unity suites use shorter delays. | `firmware/test/test_widget_registry/test_main.cpp` | +0.3 |
| 🟡 | MINOR | **AC6 / Task 7:** No `static_assert` or link-time test that `LogoWidget.cpp` excludes `fillRect`; reliance on **manual grep** is brittle if file moves or macro wraps `fillRect`. | Process | +0.3 |

**Clean-pass categories (no material findings):**  
- **Security** (no new trust boundaries, no network, no secrets in widget code): **−0.5**  
- **Type-safety / Python**: N/A / not applicable: **−0.5**

**Evidence score:**  
`4 × (+1) + 7 × (+0.3) + 2 × (−0.5) = 4 + 2.1 − 1.0 = 5.1`

**Verdict:** **MAJOR REWORK** (score **≥ 5.0** per your bracket — borderline; driven by **UB risk + perf + weak tests**).

---

## Architectural / abstraction notes

- **Single-responsibility / state:** “Stateless render functions” is **violated in spirit** by **global clock state** and **static logo bitmap scratch** — acceptable for V1, but document as **known limitation** before LE-1.3 multi-widget layouts.
- **RenderContext discipline:** Widgets correctly avoid `ConfigManager` / network; good alignment with hexagonal rules for this layer.

---

## Performance

- **LogoWidget** 256 writes × frame rate — see table (**IMPORTANT**).  
- **ClockWidget** `getLocalTime` throttled — good; counter only on success is fine for observability.

---

## Correctness & safety

- **Enum `switch` UB** without `default` — see table (**IMPORTANT**).  
- **TextWidget** relies on **Adafruit draw clipping** for `h < CHAR_H` instead of an explicit clip rect; likely OK on ESP32, but **AC7** “does not write out-of-bounds” is **assumption**, not proven in tests.

---

## Tests (“lying” / weak)

- **Alignment tests** — smoke-only (**IMPORTANT** weak coverage).  
- **Clock phase 2** — does not distinguish failed vs successful arm (**MINOR** misleading comment vs. behavior).  
- **`test_logo_no_fillrect`** — **not present** as automated test; grep-only (**MINOR** vs. story text).

---

## Suggested fixes (prioritized)

1. Add `default: return false;` (or equivalent) to `WidgetRegistry::dispatch` after all known `case`s to handle invalid `WidgetType` values without UB.  
2. Move clock cache to **caller-owned** or **per-`WidgetSpec.id`** static map (bounded table) before multi-clock layouts ship.  
3. Optimize **LogoWidget** buffer fill (or only refill when color changes).  
4. Add **one** framebuffer-backed test (even 32×32 stub matrix) for text **x** positions for left/center/right, or document AC4 as “manual visual” until LE-1.3.  
5. Gate `ClockWidgetTest` behind **`PIO_UNIT_TESTING`** or split headers.  
6. Trim **`delay(2000)`** to the minimum needed for hardware under test.  
7. Reconcile **story File List** with **actual git** before merge.

---

**Issues fixed (this review):** 0 (read-only)  
**Action items created:** 0 (orchestrator may open from this list)

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

# 🔥 ADVERSARIAL CODE REVIEW: Story LE-1.2 (Widget Registry and Core Widgets)

**Story:** le-1-2-widget-registry-and-core-widgets  
**Target:** Widget registry, core widget implementations (Text, Clock, Logo V1 stub)  
**Review Date:** 2026-04-17  
**Reviewer:** AI Senior Code Reviewer

---

## 📊 Review Summary

| Category | Count | Details |
|----------|-------|---------|
| **AC Implementation Gaps** | 2 | AC #8 test coverage incomplete; LogoWidget note accuracy issue |
| **Task Completion Audit** | 3 | Task 8 build verification claims unverifiable; compilation checks nominal |
| **Code Quality Issues** | 5 | Cache throttle bug, nullptr handling edge case, static buffer overflow risk, test architecture inconsistency, documentation drift |
| **Performance Footguns** | 2 | Repeated snprintf in hot path, malloc fallback pattern fragility |
| **Security Considerations** | 1 | Minimal (embedded, no network layer in widgets) |
| **Architecture Compliance** | 1 | RenderContext discipline mostly maintained |
| **TOTAL ISSUES FOUND** | **14** | **MAJOR REWORK REQUIRED** |

---

## 🏛️ Critical Findings (Must Fix Before Merge)

### 🔴 **[CRITICAL #1] ClockWidget Cache Throttle Bug - AC #5 Not Actually Met**

**Severity:** CRITICAL (Data Correctness)  
**File:** `firmware/widgets/ClockWidget.cpp`  
**Lines:** 36-41  
**Issue:**

The cache refresh logic has a critical bug that violates AC #5 ("minute-resolution cache"):

```cpp
if (s_lastClockUpdateMs == 0 || (nowMs - s_lastClockUpdateMs) > 30000) {
    struct tm tinfo;
    if (getLocalTime(&tinfo, 0)) {
        snprintf(s_clockBuf, sizeof(s_clockBuf), "%02d:%02d", ...);
        s_timeCallCount++;
    }
    // BUG: s_lastClockUpdateMs ONLY advances on success!
    s_lastClockUpdateMs = nowMs;  // This line is OUTSIDE the if block — correct!
}
```

**Wait, re-reading...** The code IS inside the outer block but OUTSIDE the `if (getLocalTime)` block. This is actually CORRECT per the dev notes. BUT:

**Real Bug:** When NTP is unreachable, `s_timeCallCount++` never fires, but `s_lastClockUpdateMs` advances. The test `test_clock_cache_reuse` in dev notes calls `resetTimeCallCount()` then dispatches 50 times, asserting `getTimeCallCount() <= 1`. On a device with no NTP:
- Dispatch #1: `getLocalTime()` fails, count stays 0, timer advances
- Dispatch #2-50: timer hasn't rolled 30s yet, skip refresh entirely, count stays 0
- **Test passes trivially** even though the cache never actually worked

**Proof:** Line 36-41 structure shows the `s_lastClockUpdateMs = nowMs` assignment is at line 41 (inside the refresh `if` but outside the `getLocalTime if`). The comment in dev notes says it advances on both success AND failure, which is correct implementation. BUT the test `test_clock_cache_reuse` cannot distinguish between "cache is working" vs "cache never runs because NTP unavailable".

**Impact:** AC #5 specifies "minute-resolution cache, validated pattern." The validation is impossible with the current test structure. A device with no NTP appears to pass the cache test but the cache never actually fires.

**Fix:** The test needs a phase that actually provokes a cache hit, not just confirms count <= 1. Something like:
```cpp
// Phase 1: Confirm throttle fires once
resetTimeCallCount();
resetCacheForTest();  // Clear s_lastClockUpdateMs
// Mock: inject a 31000ms time delta somehow
for (int i = 0; i < 50; i++) dispatch(...);
// With 30s+ elapsed, the first dispatch should call getLocalTime
// But without mocking millis(), this is impossible in unit test
```

**Root Cause:** The test infrastructure cannot mock `millis()` to simulate 30s passage, so it cannot actually validate the throttle mechanism.

---

### 🔴 **[CRITICAL #2] Task 8 Build Verification Claims Are Unverifiable**

**Severity:** HIGH (Incomplete Acceptance)  
**File:** Story document, Task 8, sub-items  
**Issue:**

Task 8 (Build and verify) lists this acceptance criterion:

```
[x] pio test -e esp32dev --filter test_widget_registry --without-uploading --without-testing 
    — test TU compiles cleanly
```

This is contradictory and unverifiable:
- `--without-testing` flag does NOT exist in PlatformIO (checked platform docs)
- The actual PlatformIO test flags are `--without-uploading` (correct) but testing is not disabled
- Claim: "test TU compiles cleanly" — but the flag used doesn't exist
- **Verification:** Running the exact command from Task 8 would fail with "Unknown flag: --without-testing"

The *intent* was probably to verify that the test source compiles without running the full suite (which requires hardware). But the actual command syntax is wrong.

**Proof:** Check PlatformIO docs for test targets. The correct pattern should be something like:
```bash
pio test -e esp32dev --filter test_widget_registry -v  # verbose compile, list tests found
```

But the story claims to have run `--without-testing` which doesn't exist.

**Impact:** Task 8 completion is claimed but cannot be reproduced. The build verification checklist is invalid.

**Fix:** Clarify the actual command used and provide evidence it ran successfully.

---

### 🔴 **[CRITICAL #3] LogoWidget Static Buffer Overflow Risk**

**Severity:** HIGH (Memory Safety)  
**File:** `firmware/widgets/LogoWidget.cpp`  
**Lines:** 26-32  
**Issue:**

```cpp
namespace {
    constexpr int kLogoW = 16;
    constexpr int kLogoH = 16;
}

bool renderLogo(const WidgetSpec& spec, const RenderContext& ctx) {
    // ...
    static uint16_t s_buf[kLogoW * kLogoH];  // 16*16=256 elements, 512 bytes
    for (int i = 0; i < kLogoW * kLogoH; ++i) {
        s_buf[i] = spec.color;
    }
```

The static buffer is correctly sized for 16×16 pixels. BUT:

**The Antipattern Issue:** The dev notes (AC #6) state:

> Why not fillRect? ... drawBitmapRGB565(16x16) = 1.23x baseline

But the actual call is:

```cpp
DisplayUtils::drawBitmapRGB565(ctx.matrix, spec.x, spec.y,
                               kLogoW, kLogoH, s_buf,
                               spec.w, spec.h);
```

The function receives `s_buf` (16×16) but `spec.w` and `spec.h` can be ANY size (AC #7 only enforces min 8×8). If `spec.w = 256` and `spec.h = 256`:

- `drawBitmapRGB565` will attempt to read from s_buf[16*16] onwards, which is out of bounds
- The "zoneW/zoneH for clipping" parameter is supposed to prevent writing OUT of bounds on the matrix
- But it doesn't prevent READ bounds violations on the source bitmap

**Proof:** No bounds check on `spec.w` or `spec.h` before passing to `drawBitmapRGB565`. The function assumes the source bitmap is large enough to fill the zone.

**Impact:** Large widgets with undersized bitmaps = memory safety violation. Potential crash or garbage output.

**Fix:** Enforce that `spec.w <= 16 && spec.h <= 16` in renderLogo, OR dynamically size the static buffer, OR validate in displayBitmapRGB565.

---

### 🟠 **[HIGH #4] TextWidget Truncation Buffer Vulnerability**

**Severity:** HIGH (Potential Buffer Overflow)  
**File:** `firmware/widgets/TextWidget.cpp`  
**Lines:** 20-26  
**Issue:**

```cpp
bool renderText(const WidgetSpec& spec, const RenderContext& ctx) {
    if (spec.content[0] == '\0') return true;

    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    if (maxCols <= 0) return true;

    char out[48];  // Fixed buffer
    DisplayUtils::truncateToColumns(spec.content, maxCols, out, sizeof(out));
```

`spec.content` is defined in WidgetRegistry.h as:

```cpp
char content[48];  // text content / clock format / logo_id
```

So `truncateToColumns` is called with a 48-byte input and a 48-byte output buffer. The `truncateToColumns` signature (from Dev Notes) is:

```cpp
truncateToColumns(const char*, int, char* out, size_t)
```

**The issue:** If `truncateToColumns` appends an ellipsis suffix ("..."), it could overflow the 48-byte buffer:
- Input: 48 bytes of text
- Append: "..." (3 chars)
- Result: 51 bytes written to 48-byte buffer

The DisplayUtils implementation is not provided in the review context, so this is an **assumption-based finding**. But it's a reasonable one given:
1. The spec.content is exactly 48 bytes
2. Truncation typically appends ellipsis
3. No size safety documented

**Proof:** Cannot verify without DisplayUtils implementation, but pattern is a red flag.

**Impact:** Stack buffer overflow if truncateToColumns doesn't account for ellipsis length.

**Fix:** Either (a) output buffer must be 51 bytes, or (b) truncateToColumns must reserve space for ellipsis, or (c) API documentation must clarify exact truncation behavior.

---

## 🟠 Important Findings (Should Fix)

### 🟠 **[HIGH #5] AC #8 Test Coverage Claim is Incomplete**

**Severity:** HIGH (AC Gap)  
**File:** Story document, AC #8  
**Issue:**

AC #8 states:

```
Given unit tests When `pio test -f test_widget_registry` runs Then cases pass for: 
  - dispatch-by-type ✓
  - unknown-type returns false ✓
  - bounds clipping ✓
  - clock cache prevents repeated getLocalTime() calls ✓ (UNVERIFIABLE)
  - [and 7 more test cases listed]
```

The story claims Task 7 (Unity tests) implements:

```
[x] test_clock_cache_reuse — call dispatch for clock 50× in tight loop; 
    mock or stub `getLocalTime` to count calls; assert call count == 1
```

**But there's no evidence of mocking infrastructure:**
- The test file `firmware/test/test_widget_registry/test_main.cpp` is described in dev notes but NOT provided in the code review files
- Without seeing the actual test, we cannot verify it actually mocks getLocalTime
- The Dev Notes say "use a mockable time source or counter" but the implementation just uses real getLocalTime()
- The namespace `ClockWidgetTest` has `getTimeCallCount()` (call counter) but this requires the implementation to actually call getLocalTime

**Red Flag:** The test infrastructure exists (resetCacheForTest, getTimeCallCount), but we have no evidence the dispatch loop actually exercises the cache. The "50 calls in tight loop" on real hardware with millis() would NOT trigger the 30s cache, so the test would be meaningless.

**Impact:** AC #8 may be satisfied on paper, but the actual test validity is questionable.

**Fix:** Provide the actual test file and demonstrate it validates the cache with <2 getLocalTime calls regardless of NTP state.

---

### 🟠 **[HIGH #6] WidgetRegistry::fromString Missing const Correctness**

**Severity:** MEDIUM (Code Quality)  
**File:** `firmware/core/WidgetRegistry.h`  
**Lines:** Declaration  
**Issue:**

```cpp
static WidgetType fromString(const char* typeStr);  // ✓ Correct
static bool isKnownType(const char* typeStr);       // ✓ Correct
```

Both methods are correctly `const char*` parameters. BUT:

The dispatch function signature is:

```cpp
static bool dispatch(WidgetType type,
                     const WidgetSpec& spec,
                     const RenderContext& ctx);
```

The `RenderContext` is passed as const reference, which is correct. But WidgetSpec is ALSO const reference, which means render functions receive read-only specs. This is correct too.

**Wait, re-reading...** Actually, I was looking for a violation and found none. The const correctness is correct throughout.

**Revised finding:** NO CONST CORRECTNESS ISSUE. Discard this finding.

---

### 🟠 **[HIGH #7] LogoWidget Comment Accuracy Issue**

**Severity:** MEDIUM (Documentation)  
**File:** `firmware/widgets/LogoWidget.cpp`  
**Lines:** 25-28  
**Issue:**

The dev notes state:

> NOTE: drawBitmapRGB565 treats pixel value 0x0000 (black) as transparent and skips drawing that pixel. A spec.color of black will therefore render as invisible in this V1 stub. LE-1.5 (real LittleFS bitmaps) is not affected because real logos will have non-zero background pixels.

**But the actual comment in the code says:**

```cpp
// A spec.color of black will therefore render as invisible...
// LE-1.5 (real LittleFS bitmaps) is not affected...
```

This is in the code. So the documentation is present. But:

**The Issue:** In `firmware/widgets/LogoWidget.cpp` line ~29:

```cpp
// NOTE: drawBitmapRGB565 treats pixel value 0x0000 (black) as transparent
// and skips drawing that pixel. A spec.color of black will therefore render
// as invisible in this V1 stub. LE-1.5 (real LittleFS bitmaps) is not
// affected because real logos will have non-zero background pixels.
```

This is only in the inline comment. But where does `drawBitmapRGB565` actually implement this? The function is in `firmware/utils/DisplayUtils.h` (not provided in review). **Without seeing the actual implementation, we cannot verify this behavior is real or just an assumption.**

**Impact:** If `drawBitmapRGB565` does NOT treat 0x0000 as transparent, the comment is misleading. Developers will create tests that don't catch bugs.

**Fix:** Either (a) verify drawBitmapRGB565 actually does this, or (b) update comment to say "EXPECTED to treat black as transparent", or (c) validate with a test.

---

## 🐛 Code Quality Issues

### 🟡 **[MEDIUM #8] Static Buffer Initialization in renderClock**

**Severity:** LOW (Defensive Programming)  
**File:** `firmware/widgets/ClockWidget.cpp`  
**Lines:** 18  
**Issue:**

```cpp
namespace {
    char     s_clockBuf[8] = "--:--";  // ✓ Initialized
    uint32_t s_lastClockUpdateMs = 0;  // ✓ Initialized
}
```

Both statics are initialized, which is good. But the size of `s_clockBuf` is exactly "HH:MM\0" (6 chars) plus one extra. Let's count:

- 'H' (1), 'H' (1), ':' (1), 'M' (1), 'M' (1), '\0' (1) = 6 bytes minimum
- Buffer size: 8 bytes
- Safety margin: 2 bytes

The `snprintf` line uses:

```cpp
snprintf(s_clockBuf, sizeof(s_clockBuf), "%02d:%02d", ...);
```

With `sizeof(s_clockBuf) = 8`, we're safe for "HH:MM\0" (6 bytes) plus 2-byte margin. But this is implicit and fragile. If someone changes the format string to include seconds without resizing the buffer, it silently corrupts.

**Fix:** Document the expected format length, or use a larger buffer with an assert, or document "24h format only, 5 chars + null".

---

### 🟡 **[MEDIUM #9] Missing nullptr Guard in WidgetRegistry::dispatch**

**Severity:** LOW (Robustness)  
**File:** `firmware/core/WidgetRegistry.cpp`  
**Lines:** 41-54  
**Issue:**

```cpp
bool WidgetRegistry::dispatch(WidgetType type,
                              const WidgetSpec& spec,
                              const RenderContext& ctx) {
    switch (type) {
        case WidgetType::Text:        return renderText(spec, ctx);
        case WidgetType::Clock:       return renderClock(spec, ctx);
        case WidgetType::Logo:        return renderLogo(spec, ctx);
        // ...
        default:                      return false;
    }
    return false;
}
```

If somehow an invalid WidgetType value (e.g., 0x42 from memory corruption) makes it past the enum, the final `return false` catches it. **But**: The render functions themselves have `if (ctx.matrix == nullptr) return true` guards, which is good for test harness.

**No actual issue here** — the dispatch is safe. The default case handles unknown types, and each render function guards nullptr. Carrying forward.

---

### 🟡 **[MEDIUM #10] snprintf Repeated in Hot Path**

**Severity:** LOW-MEDIUM (Performance)  
**File:** `firmware/widgets/ClockWidget.cpp`  
**Lines:** 36-41  
**Issue:**

On every cache refresh (every 30s):

```cpp
snprintf(s_clockBuf, sizeof(s_clockBuf), "%02d:%02d",
         tinfo.tm_hour, tinfo.tm_min);
```

`snprintf` is a non-trivial function (format parsing, va_args handling, etc.). Even though it only runs once per 30s, it's doing unnecessary work:

```cpp
// More efficient: direct digit math
s_clockBuf[0] = '0' + (tinfo.tm_hour / 10);
s_clockBuf[1] = '0' + (tinfo.tm_hour % 10);
s_clockBuf[2] = ':';
s_clockBuf[3] = '0' + (tinfo.tm_min / 10);
s_clockBuf[4] = '0' + (tinfo.tm_min % 10);
s_clockBuf[5] = '\0';
```

This trades readability for ~10-20% performance improvement on the refresh. Since it's only once per 30s, not critical, but worth noting.

**Impact:** Negligible for current design, but sets bad precedent.

---

## ⚡ Performance Analysis

### 🟡 **[MEDIUM #11] DisplayUtils::drawTextLine Called Per-Frame**

**Severity:** LOW-MEDIUM (Performance Potential)  
**File:** All widget implementations  
**Issue:**

Every render function calls `DisplayUtils::drawTextLine` every frame (~20fps):

```cpp
void renderText(...) {
    // ...
    DisplayUtils::drawTextLine(ctx.matrix, drawX, drawY, out, spec.color);
}

void renderClock(...) {
    // ...
    DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);
}
```

This is by design (modes are stateless), so it's necessary. But **if** `drawTextLine` does character-by-character glyph rendering (likely), then each widget re-renders its glyphs on every frame, every pixel, every call to FastLED.

**For 20 widgets at 20fps, that's 400 glyph renders per second.** If each glyph is 8×8 pixels at RGB565, that's ~13,000 pixel writes per second. This should be fine for a 160×32 matrix but worth monitoring.

**Impact:** No critical issue, but future optimization point if frame rate drops.

---

## 🔒 Security & Safety

### 🟡 **[MINOR #12] No Validation of WidgetSpec.align Values**

**Severity:** LOW (Robustness)  
**File:** `firmware/widgets/TextWidget.cpp`  
**Lines:** 27-32  
**Issue:**

```cpp
switch (spec.align) {
    case 1: drawX = spec.x + (int16_t)(((int)spec.w - textPixels) / 2); break;
    case 2: drawX = spec.x + (int16_t)((int)spec.w - textPixels);       break;
    case 0:
    default: /* left */ break;
}
```

If `spec.align = 0xFF` (garbage/corruption), it falls through to `default` and renders as left-aligned. This is safe (graceful degradation), but silently accepts invalid input.

**Better:** Validate that `spec.align <= 2` or log a warning.

**Impact:** Negligible for current use, but weak input validation.

---

## 🛠️ Suggested Auto-Fixes

### Fix #1: Strengthen ClockWidget Test to Validate Cache Actually Works

**File:** `firmware/test/test_widget_registry/test_main.cpp`  
**Issue:** AC #5 cache validation is unverifiable with current test structure

**Suggested Approach:**

```cpp
// Add a mock time source to the widget system:
// 1. Extract millis() into a mockable function
// 2. Inject via a RenderContext or global callback
// 3. Test can then advance time without sleeping

// Alternative (simpler): Add explicit test hook
namespace ClockWidgetTest {
    // Extern these so test can manipulate
    extern char s_clockBuf[8];
    extern uint32_t s_lastClockUpdateMs;
    
    void advanceCacheTimer(uint32_t ms) {
        s_lastClockUpdateMs = (uint32_t)((int32_t)s_lastClockUpdateMs - (int32_t)ms);
    }
}

// Test:
void test_clock_cache_reuse_with_time_advance() {
    ClockWidgetTest::resetCacheForTest();
    ClockWidgetTest::resetTimeCallCount();
    
    WidgetSpec spec = {...};
    RenderContext ctx = {...};
    
    // First dispatch: cache miss
    dispatch(WidgetType::Clock, spec, ctx);
    int count_after_first = ClockWidgetTest::getTimeCallCount();
    TEST_ASSERT_EQUAL(1, count_after_first);
    
    // Advance time by 15s (within cache window)
    ClockWidgetTest::advanceCacheTimer(15000);
    dispatch(WidgetType::Clock, spec, ctx);
    TEST_ASSERT_EQUAL(1, ClockWidgetTest::getTimeCallCount()); // Still 1
    
    // Advance time by 16s more (total 31s, beyond cache)
    ClockWidgetTest::advanceCacheTimer(16000);
    dispatch(WidgetType::Clock, spec, ctx);
    TEST_ASSERT_EQUAL(2, ClockWidgetTest::getTimeCallCount()); // Now 2
}
```

---

### Fix #2: Add Buffer Size Validation to renderLogo

**File:** `firmware/widgets/LogoWidget.cpp`  
**Issue:** No validation that spec dimensions match static buffer size

```cpp
bool renderLogo(const WidgetSpec& spec, const RenderContext& ctx) {
    // Enforce that widgets must fit within the 16x16 static buffer
    // (LE-1.5 replaces this with dynamic-size per logo)
    if (spec.w > 16 || spec.h > 16) {
        LOG_W("LogoWidget", "Widget size (%ux%u) exceeds V1 buffer (16x16)",
              spec.w, spec.h);
        return true;  // Silent no-op; not an error for AC #7
    }
    
    if ((int)spec.w < 8 || (int)spec.h < 8) return true;
    
    if (ctx.matrix == nullptr) return true;
    
    // ... rest of function
}
```

---

### Fix #3: Improve TextWidget Buffer Safety

**File:** `firmware/widgets/TextWidget.cpp`  
**Issue:** Fixed 48-byte buffer vulnerable if truncateToColumns adds ellipsis

```cpp
bool renderText(const WidgetSpec& spec, const RenderContext& ctx) {
    if (spec.content[0] == '\0') return true;

    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    if (maxCols <= 0) return true;

    if (ctx.matrix == nullptr) return true;

    // Account for ellipsis: if truncation happens, "..." is added
    // DisplayUtils::truncateToColumns must have 3 extra bytes
    char out[51];  // 48 + 3 for "..." + 1 null terminator (was 48)
    static_assert(sizeof(out) >= sizeof(spec.content) + 4,
                  "Output buffer must fit content + ellipsis + null");
    
    DisplayUtils::truncateToColumns(spec.content, maxCols, out, sizeof(out));
    
    // ... rest of function
}
```

---

## 📋 Summary Table

| ID | Severity | Category | Issue | Fix Status |
|----|----------|----------|-------|------------|
| #1 | 🔴 CRITICAL | Correctness | ClockWidget cache test is unverifiable | Suggested |
| #2 | 🔴 CRITICAL | Task Completion | Task 8 build verification uses non-existent PlatformIO flag | Requires investigation |
| #3 | 🔴 CRITICAL | Memory Safety | LogoWidget buffer OOB read if spec larger than 16×16 | Suggested |
| #4 | 🟠 HIGH | Memory Safety | TextWidget truncation could overflow output buffer | Suggested |
| #5 | 🟠 HIGH | AC Gap | AC #8 test coverage unverifiable without seeing actual test file | Suggested |
| #6 | 🟠 HIGH | Documentation | LogoWidget 0x0000 transparency behavior unverified | Requires verification |
| #7 | 🟡 MEDIUM | Code Quality | ClockWidget static buffer size is implicit/fragile | Suggested |
| #8 | 🟡 MEDIUM | Code Quality | snprintf in hot path less efficient than digit math | Low priority |
| #9 | 🟡 MEDIUM | Performance | drawTextLine called per-frame for 20+ widgets | Monitoring only |
| #10 | 🟡 MINOR | Robustness | No validation of spec.align values | Low priority |

---

## 📊 Evidence Score Calculation

| Finding | Severity | Points |
|---------|----------|--------|
| #1 CRITICAL: Unverifiable cache test | 🔴 | +3 |
| #2 CRITICAL: Task 8 fraud (non-existent flag) | 🔴 | +3 |
| #3 CRITICAL: LogoWidget buffer OOB | 🔴 | +3 |
| #4 HIGH: TextWidget buffer overflow | 🟠 | +1 |
| #5 HIGH: AC #8 unverifiable | 🟠 | +1 |
| #6 HIGH: Unverified 0x0000 behavior | 🟠 | +1 |
| Clean Pass: SOLID principles | ✅ | -0.5 |
| Clean Pass: Style compliance | ✅ | -0.5 |
| Clean Pass: Architecture pattern adherence | ✅ | -0.5 |

**Evidence Score:** 3 + 3 + 3 + 1 + 1 + 1 - 0.5 - 0.5 - 0.5 = **10.0**

**Verdict:** 🔴 **REJECT** — Critical memory safety issues, unverifiable tests, fraudulent task completion claims

---

## 🎯 Mandatory Action Items

1. **BLOCKER:** Verify Task 8 build command actually works (clarify non-existent `--without-testing` flag)
2. **BLOCKER:** Add bounds validation to renderLogo to prevent spec.w/h > 16
3. **BLOCKER:** Add buffer overflow protection to renderText (increase output buffer or document truncation guarantees)
4. **BLOCKER:** Provide actual test file `firmware/test/test_widget_registry/test_main.cpp` to validate cache test is real
5. **HIGH:** Verify drawBitmapRGB565 actually treats 0x0000 as transparent
6. **MEDIUM:** Implement proper cache test with simulated time advance or mock getLocalTime

---

**Status:** NOT READY FOR MERGE  
**Issues Found:** 10  
**Critical Issues:** 3  
**Action Items Required:** 6

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
<var name="reviewer_count">2</var>
<var name="session_id">06badf17-d2f6-4b8d-b10e-1950b3510da3</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="bd2de04b">embedded in prompt, file id: bd2de04b</var>
<var name="story_id">le-1.2</var>
<var name="story_key">le-1-2-widget-registry-and-core-widgets</var>
<var name="story_num">2</var>
<var name="story_title">2-widget-registry-and-core-widgets</var>
<var name="template">False</var>
<var name="timestamp">20260417_1356</var>
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