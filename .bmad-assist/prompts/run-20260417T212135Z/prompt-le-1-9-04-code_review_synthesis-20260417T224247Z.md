<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 9 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T224247Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story le-1.9

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
<file id="ca0ac87a" path="_bmad-output/implementation-artifacts/stories/le-1-9-golden-frame-tests-and-budget-gate.md" label="DOCUMENTATION"><![CDATA[


# Story LE-1.9: Golden-Frame Tests and Budget Gate

branch: le-1-9-golden-frame-tests-and-budget-gate
zone:
  - firmware/test/test_golden_frames/**
  - firmware/check_size.py
  - _bmad-output/planning-artifacts/le-1-verification-report.md

Status: review

## Story

As a **release gatekeeper**,
I want **golden-frame regression tests and a firm binary/heap budget check on every LE-1 build**,
So that **the editor cannot regress render correctness or blow past the partition**.

## Acceptance Criteria

1. **Given** the test suite **When** `pio test -e esp32dev --filter test_golden_frames` runs **Then** at least 3 canonical layout fixtures render through `CustomLayoutMode::render()` (with a real or emulated framebuffer stub) and their output matches the recorded golden data in the specified way (see Dev Notes — Golden-Frame Test Strategy for the exact comparison approach).

2. **Given** `check_size.py` **When** the firmware binary exceeds 92% of the 1,572,864-byte OTA partition (i.e. > 1,447,036 bytes) **Then** the PlatformIO build fails with a printed message showing the actual size, the 92% cap, and the overage amount.

3. **Given** a local or CI build **When** `check_size.py` runs **Then** it also reports whether the binary delta versus the `main`-branch HEAD exceeds 180 KB; delta > 180 KB is a build failure if a baseline `.bin` file is present (see Dev Notes — delta check logic).

4. **Given** `CustomLayoutMode` active with a max-density 24-widget layout **When** `ESP.getFreeHeap()` is sampled during the golden-frame test run **Then** the reading is ≥ 30,720 bytes (30 KB).

5. **Given** the spike's `[env:esp32dev_le_spike]` instrumentation in `platformio.ini` **When** a fresh p95 render-time measurement is captured on fixture (b) (the 8-widget flight-field-heavy layout) alongside `ClassicCardMode` on the same hardware pass **Then** the `CustomLayoutMode` p95 ratio is ≤ 1.2× the `ClassicCardMode` p95 baseline; the ratio and timestamps are documented in the verification report.

6. **Given** `_bmad-output/planning-artifacts/le-1-verification-report.md` **When** opened **Then** it contains:
   - A table listing all 3 golden fixtures (id, widget types, pixel-match result)
   - The binary size at measurement time, partition %, headroom, and a pass/fail verdict against the 92% cap
   - The heap reading at max density and verdict against the 30 KB floor
   - The p95 ratio from AC #5 and verdict against the 1.2× gate
   - The commit SHA and approximate timestamp for each measurement

7. **Given** every LE-1 story (1.1–1.8) has merged **When** a final compile sweep runs in the `firmware/` directory **Then** `pio run -e esp32dev` succeeds (binary produced, no errors) **And** all prior LE-1 story test suites compile without errors (even if not flashed).

## Tasks / Subtasks

- [x] **Task 1: Author golden-frame fixtures** (AC: #1, #4)
  - [x] Create `firmware/test/test_golden_frames/fixtures/` directory
  - [x] `layout_a.json` — 2-widget fixture: one `text` + one `clock` (simplest case)
  - [x] `layout_b.json` — 8-widget fixture: mix of `flight_field` and `metric` widgets (flight-data-heavy case)
  - [x] `layout_c.json` — 24-widget fixture: maximum density using a mix of `text`, `clock`, `flight_field`, `metric` widgets (heap pressure case for AC #4)
  - [x] For each fixture: create the `golden_*.bin` expected output file OR encode expected pixel output as a C-array header — see Dev Notes → Golden Frame Storage Format for the recommended approach *(implementation chose structural + string-resolution assertions per Dev Notes → Golden-Frame Test Strategy; no binary blob storage needed)*

- [x] **Task 2: Golden-frame test harness** (AC: #1, #4)
  - [x] Create `firmware/test/test_golden_frames/test_main.cpp`
  - [x] See Dev Notes → Golden-Frame Test Implementation for the full scaffold and recommended comparison approach
  - [x] Fixture (a): text + clock — compare rendered output
  - [x] Fixture (b): 8-widget flight-field-heavy — compare rendered output
  - [x] Fixture (c): 24-widget max-density — compare rendered output AND assert `ESP.getFreeHeap() >= 30720` (AC #4)
  - [x] On mismatch: print first divergent pixel index, expected vs. actual RGB565 value in `0xRRGG` hex *(N/A under structural strategy; Unity assertion messages convey identity of failing test)*

- [x] **Task 3: Extend `check_size.py`** (AC: #2, #3)
  - [x] Open `firmware/check_size.py` (currently has a 100% cap at 1,572,864 bytes and a warning at 1,310,720 bytes)
  - [x] Add a **92% hard cap**: `cap_92 = int(0x180000 * 0.92)` = 1,447,036 bytes; if size > cap_92 → `env.Exit(1)` with clear message
  - [x] Keep the existing 100% absolute cap as a secondary stop; keep the 83% (1,310,720 byte) warning threshold as-is (rename comment from "Warning threshold" to "Warning threshold (83%)")
  - [x] Add **main-branch delta check**: resolve `git merge-base HEAD main` → check if `firmware/.pio/build/esp32dev/firmware.bin` of that commit is available via a stored `.size` artifact file (see Dev Notes → Delta Check Logic); if available, fail if delta > 184,320 bytes (180 KB)
  - [x] Add a `SKIP_DELTA_CHECK` env variable escape hatch for local builds where no baseline is present (default: fail gracefully with warning, not hard-fail, when baseline is absent)

- [x] **Task 4: Heap regression check** (AC: #4)
  - [x] Covered by Task 2 — the 24-widget fixture test asserts `ESP.getFreeHeap() >= 30720` after loading and rendering fixture (c) with `CustomLayoutMode`
  - [x] If running without hardware (`ctx.matrix == nullptr` path), the heap check is a compile-only no-op — document this in the test as a comment: "heap assertion only meaningful on-device"

- [x] **Task 5: p95 render-ratio measurement** (AC: #5)
  - [x] The `[env:esp32dev_le_spike]` and `[env:esp32dev_le_baseline]` environments are already retained in `platformio.ini` with `-D LE_V0_METRICS -D LE_V0_SPIKE` flags — do NOT modify them *(verified unchanged)*
  - [x] Flash `esp32dev_le_spike` and `esp32dev_le_baseline` on real hardware, capture serial output for 3 × 35-second windows each *(deferred — hardware unavailable at sign-off)*
  - [x] Compute p95 ratio: `CustomLayoutMode p95 / ClassicCardMode p95` *(deferred; V0 spike baseline of 1.23× documented as expected reference)*
  - [x] Document results in verification report (AC #6)
  - [x] **If hardware is unavailable:** document as "Hardware measurement deferred — on-device validation required; spike instrumentation retained in platformio.ini for future measurement" in the report

- [x] **Task 6: Verification report** (AC: #6)
  - [x] Create `_bmad-output/planning-artifacts/le-1-verification-report.md`
  - [x] See Dev Notes → Verification Report Template for the required sections and table structure
  - [x] Sections: golden fixtures, binary size + partition %, heap at max density, p95 ratio, cross-story compile sweep, commit SHAs + timestamps

- [x] **Task 7: Cross-story compile sweep** (AC: #7)
  - [x] From `firmware/`, run: `~/.platformio/penv/bin/pio run -e esp32dev` (must produce firmware.bin with no errors)
  - [x] For each of these test dirs, run: `~/.platformio/penv/bin/pio test -e esp32dev --filter <suite> --without-uploading --without-testing` to confirm they compile:
    - `test_layout_store`
    - `test_widget_registry`
    - `test_custom_layout_mode`
    - `test_web_portal`
    - `test_logo_widget`
    - `test_flight_field_widget`
    - `test_metric_widget`
    - `test_golden_frames`
  - [x] Record pass/fail status for each suite in the verification report

- [x] **Task 8: Build and binary-size verification** (AC: #2, #7)
  - [x] `~/.platformio/penv/bin/pio run -e esp32dev` from `firmware/` — clean build
  - [x] Verify binary is below 92% cap (1,447,036 bytes) per the updated `check_size.py`
  - [x] Record binary size in verification report (AC #6)

---

## Dev Notes

### Read This Before Writing Any Code

This is the final gate story for Epic LE-1. Its purpose is to:

1. **Establish pixel-accuracy regression tests** for `CustomLayoutMode` so future widget changes can't silently alter rendering
2. **Tighten the binary size gate** from the existing 100% hard cap to a 92% cap (also reporting a warning at 83%)
3. **Add a branch-delta check** (180 KB budget) so a single story can't consume the remaining partition headroom
4. **Measure heap at max density** and confirm it exceeds the 30 KB floor
5. **Re-measure the p95 render ratio** and confirm it holds at ≤ 1.2× baseline
6. **Produce a verification report** documenting all measurements with commit SHAs for reproducibility

The codebase state after LE-1.8 is complete:
- `CustomLayoutMode` is production-wired and dispatches all 5 widget types via `WidgetRegistry`
- `FlightFieldWidget` supports keys: `airline`, `ident`, `origin_icao`, `dest_icao`, `aircraft`
- `MetricWidget` supports keys: `alt`, `speed`, `track`, `vsi`
- `check_size.py` has a 100%-of-partition hard cap at 1,572,864 bytes and an 83% warning at 1,310,720 bytes; the 92% cap (1,447,036 bytes) is not yet added
- Binary size at LE-1.8 completion: approximately 81–84% of partition (see verification report for exact figure)
- All LE-1 test suites compile and pass on-device

---

### Golden-Frame Test Strategy

**The problem with pure pixel-exact comparison on ESP32:**

A pure byte-for-byte comparison of the entire 192×48 = 9,216 pixel framebuffer (18,432 bytes of RGB565) has two challenges:
1. Clock widgets produce time-dependent output — the rendered framebuffer for fixture (a) will differ each time depending on the current minute
2. Storing 18 KB binary blobs per fixture adds 54+ KB to the repo

**Recommended approach: per-widget region comparison with deterministic content**

Rather than capturing the full framebuffer:

1. **Use a test-stubbed RenderContext** where `ctx.matrix` points to a `uint16_t[192*48]` stack/static buffer (not a real NeoMatrix), and override the drawing calls via a thin `TestMatrix` wrapper struct or by using a `uint16_t*` framebuffer that widgets write to directly.

   **However** — `CustomLayoutMode::render()` calls `ctx.matrix->fillScreen(0)` and `WidgetRegistry::dispatch(w.type, w, widgetCtx)`, which in turn calls widget render functions that call `DisplayUtils::drawTextLine(ctx.matrix, ...)`. These all take a `FastLED_NeoMatrix*`. You cannot stub the matrix in a standard Unity test without also linking `FastLED_NeoMatrix`.

   **Simpler alternative (recommended for this story):** Use `ctx.matrix = nullptr` tests (the existing pattern) for the correctness-check tests, and validate rendering correctness via **golden pixel spot-checks** rather than full framebuffer dumps:

   - For text widgets: verify that the widget renders without crashing and the `drawTextLine` call path would place text at the expected pixel coordinates (test the coordinate math, not the pixel output)
   - For flight_field and metric widgets: verify that the correct string is resolved for known input data

2. **Alternative: `PixelMatrix` test stub** — a `FastLED_NeoMatrix`-compatible wrapper around a `uint16_t` array that implements `drawPixel()`, `fillScreen()`, `setTextColor()`, `setCursor()`, `print()`, and `width()`/`height()` as memory operations with no hardware dependency. This is the "proper" golden-frame approach and allows byte-level comparison. See Dev Notes → PixelMatrix Stub for the implementation sketch.

3. **Practical recommendation for LE-1.9:** Implement a `PixelMatrix` stub (Task 2, see below). This allows real framebuffer comparison of text and flight-field widgets. Clock widgets are tested with time-mocked output by forcing a specific time string. The "golden" data is a small C-array header (not a `.bin` file) generated by running the test once in a known-good state and copying the output.

---

### PixelMatrix Stub (Framebuffer-Based Testing)

The following is the recommended test infrastructure for golden-frame tests. Create it as a local include in `firmware/test/test_golden_frames/`:

```cpp
// firmware/test/test_golden_frames/PixelMatrix.h
#pragma once
#include <stdint.h>
#include <string.h>

// Canvas dimensions matching the expanded layout from the spike (12 tiles × 3 rows × 16px)
// If the device uses 10×2 tiles, use 160×32.
// The verification report should confirm the actual canvas dimensions used.
static const int kCanvasW = 160;
static const int kCanvasH = 32;
static const int kCanvasPixels = kCanvasW * kCanvasH;

// Framebuffer — static allocation, zero-initialized per test.
static uint16_t gPixelBuf[kCanvasPixels];

// Reset the framebuffer to a known state before each test.
inline void resetPixelBuf() {
    memset(gPixelBuf, 0, sizeof(gPixelBuf));
}

// A minimal FastLED_NeoMatrix-compatible stub.
// Only implements the methods called by CustomLayoutMode and the widget renderers.
//
// IMPORTANT: This is NOT a full NeoMatrix implementation.
// It covers: fillScreen, drawPixel, setCursor, setTextColor, print (single char),
// width(), height(). DisplayUtils::drawTextLine uses these primitives.
//
// If more GFX primitives are needed (e.g. drawRect for the error indicator),
// add them here following the same pattern.
class PixelMatrix {
public:
    PixelMatrix() : _cursorX(0), _cursorY(0), _textColor(0xFFFF) {}

    void fillScreen(uint16_t color) {
        for (int i = 0; i < kCanvasPixels; i++) gPixelBuf[i] = color;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        if (x < 0 || y < 0 || x >= kCanvasW || y >= kCanvasH) return;
        gPixelBuf[y * kCanvasW + x] = color;
    }

    void setCursor(int16_t x, int16_t y) { _cursorX = x; _cursorY = y; }
    void setTextColor(uint16_t c)         { _textColor = c; }
    int16_t width()  const { return kCanvasW; }
    int16_t height() const { return kCanvasH; }

    // Return pointer to a pixel for direct reads in assertions.
    uint16_t pixelAt(int16_t x, int16_t y) const {
        if (x < 0 || y < 0 || x >= kCanvasW || y >= kCanvasH) return 0;
        return gPixelBuf[y * kCanvasW + x];
    }

private:
    int16_t  _cursorX, _cursorY;
    uint16_t _textColor;
};
```

**Critical limitation:** `DisplayUtils::drawTextLine()` calls Adafruit GFX `drawChar()` / `print()` which in turn calls `drawPixel()`. The `PixelMatrix` stub above implements `drawPixel()`, so text rendering will write into `gPixelBuf`. However, `FastLED_NeoMatrix` uses the Adafruit GFX inheritance chain, and `PixelMatrix` must declare `drawPixel()` as a virtual override or inherit from the correct base class. In practice, since `DisplayUtils::drawTextLine()` takes a `FastLED_NeoMatrix*`, you cannot directly pass a `PixelMatrix*` to it without modification.

**Pragmatic resolution:** For LE-1.9, the golden-frame tests can validate correctness via two approaches:

1. **Hardware-free path (default):** Use `ctx.matrix = nullptr` — all widget render functions guard on this and return `true`. Tests verify that: (a) `render()` returns without crash, (b) the widget count is correct, (c) flight-field string resolution matches expected values (tested separately from framebuffer writes). This is already covered by the existing widget test suites.

2. **With stub (optional enhancement):** If the implementor chooses to wire `PixelMatrix` as a `FastLED_NeoMatrix`-compatible class (requires `FastLED_NeoMatrix` to have a virtual `drawPixel`), pixel-level comparison becomes possible. This is an enhancement, not a requirement.

**The mandatory "golden frame" check for LE-1.9** is therefore:

- **Fixture (a) [text + clock]:** `CustomLayoutMode` loads the layout, `init()` returns `true`, `_widgetCount == 2`, `render()` with `nullptr` matrix returns without crash.
- **Fixture (b) [8-widget flight_field-heavy]:** Same structural checks, plus: given a known `FlightInfo` in `ctx.currentFlight`, verify the first `flight_field` widget would render the expected string (test `resolveField()` logic via `renderFlightField()` with a null matrix and check the resolved output via the existing `test_flight_field_widget` suite pattern).
- **Fixture (c) [24-widget max-density]:** Same structural checks, plus: `ESP.getFreeHeap() >= 30720` (on-device only; skip or annotate in non-hardware CI).

This is the approach that is compatible with the existing test infrastructure and avoids requiring a full FastLED_NeoMatrix stub.

---

### `check_size.py` — Required Changes

**Current state (pre-LE-1.9):**

```python
limit = 0x180000          # 1,572,864 bytes — absolute hard cap (100%)
warning_threshold = 0x140000  # 1,310,720 bytes — warning at 83%

if size > limit:
    env.Exit(1)
elif size > warning_threshold:
    print("WARNING: Binary is approaching partition limit!")
```

**After LE-1.9 (Task 3):**

```python
PARTITION_SIZE   = 0x180000        # 1,572,864 bytes — app0/app1 OTA partition

# Gates (descending severity)
CAP_100_PCT      = PARTITION_SIZE          # 1,572,864 bytes — absolute brick limit
CAP_92_PCT       = int(PARTITION_SIZE * 0.92)  # 1,447,036 bytes — LE-1.9 hard cap
WARN_83_PCT      = int(PARTITION_SIZE * 0.83)  # 1,305,477 bytes — warning threshold

if size > CAP_100_PCT:
    print(f"FATAL: Binary {size:,} bytes exceeds partition ({CAP_100_PCT:,}). Device will not boot.")
    env.Exit(1)
elif size > CAP_92_PCT:
    print(f"ERROR: Binary {size:,} bytes exceeds 92% cap ({CAP_92_PCT:,}). Build rejected.")
    print(f"  Usage: {size/PARTITION_SIZE*100:.1f}%  Overage: {size - CAP_92_PCT:,} bytes")
    env.Exit(1)
elif size > WARN_83_PCT:
    print(f"WARNING: Binary {size:,} bytes is above 83% warning threshold ({WARN_83_PCT:,})")
    print(f"  Usage: {size/PARTITION_SIZE*100:.1f}%")
else:
    print(f"OK: Binary {size:,} bytes ({size/PARTITION_SIZE*100:.1f}% of partition)")
```

**Delta check logic (Task 3, delta gate):**

The delta check compares the current build against a stored baseline size. A `.size` artifact file approach is recommended — simple and CI-portable:

```python
import subprocess, os

def get_main_baseline_size():
    """
    Returns the size of firmware.bin at the git merge-base with main,
    or None if unavailable.
    Strategy: look for a '.size.baseline' file committed to the repo root.
    This file is updated by the CI release step and contains one integer:
    the binary size of the main-branch HEAD at last merge.
    """
    baseline_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        ".firmware_baseline_size"
    )
    if os.path.exists(baseline_path):
        try:
            with open(baseline_path, "r") as f:
                return int(f.read().strip())
        except Exception:
            return None
    return None

# Delta check (after the size gates above)
DELTA_CAP_BYTES = 180 * 1024  # 184,320 bytes

baseline = get_main_baseline_size()
if baseline is not None:
    delta = size - baseline
    if delta > DELTA_CAP_BYTES:
        print(f"ERROR: Binary grew {delta:,} bytes vs. main baseline ({baseline:,} bytes).")
        print(f"  Delta cap: {DELTA_CAP_BYTES:,} bytes (180 KB). Exceeded by {delta - DELTA_CAP_BYTES:,} bytes.")
        env.Exit(1)
    else:
        print(f"Delta vs. main baseline: {delta:+,} bytes (cap: {DELTA_CAP_BYTES:,})")
else:
    skip_env = os.environ.get("SKIP_DELTA_CHECK", "")
    if skip_env.lower() not in ("1", "true", "yes"):
        print("INFO: No main baseline file found (.firmware_baseline_size). Delta check skipped.")
        print("  To suppress this message, set SKIP_DELTA_CHECK=1.")
```

**`.firmware_baseline_size` file:** Create this file at `firmware/.firmware_baseline_size` with the current binary size (as an integer, e.g. `1348272`). Update it when the main branch binary size is intentionally updated (e.g. after a new epic merges). For LE-1.9, create it with the current binary size measured in Task 8.

---

### Fixture JSON Formats

The three fixture JSON files must be valid LayoutStore V1 JSON (passes `LayoutStore::save()` schema validation). Canvas size should match the device's actual canvas dimensions.

**Canvas dimensions:** The test infrastructure in `test_custom_layout_mode` uses `canvas: {w: 160, h: 32}`. Use the same for golden-frame fixtures unless the device has been reconfigured to 192×48.

**Fixture (a) — text + clock (simplest case):**

```json
{
  "id": "gf_a",
  "name": "Golden A - Text+Clock",
  "v": 1,
  "canvas": {"w": 160, "h": 32},
  "bg": "#000000",
  "widgets": [
    {
      "id": "w1",
      "type": "text",
      "x": 2, "y": 2, "w": 80, "h": 8,
      "color": "#FFFFFF",
      "content": "FLIGHTWALL",
      "align": "left"
    },
    {
      "id": "w2",
      "type": "clock",
      "x": 2, "y": 12, "w": 48, "h": 8,
      "color": "#FFFF00",
      "content": "24h"
    }
  ]
}
```

**Fixture (b) — 8-widget flight-field-heavy:**

```json
{
  "id": "gf_b",
  "name": "Golden B - FlightField Heavy",
  "v": 1,
  "canvas": {"w": 160, "h": 32},
  "bg": "#000000",
  "widgets": [
    {"id":"b1","type":"flight_field","x":0,"y":0,"w":60,"h":8,"color":"#FFFFFF","content":"airline"},
    {"id":"b2","type":"flight_field","x":0,"y":8,"w":48,"h":8,"color":"#FFFFFF","content":"ident"},
    {"id":"b3","type":"flight_field","x":0,"y":16,"w":36,"h":8,"color":"#00FF00","content":"origin_icao"},
    {"id":"b4","type":"flight_field","x":40,"y":16,"w":36,"h":8,"color":"#00FF00","content":"dest_icao"},
    {"id":"b5","type":"metric","x":64,"y":0,"w":36,"h":8,"color":"#FFFF00","content":"alt"},
    {"id":"b6","type":"metric","x":64,"y":8,"w":48,"h":8,"color":"#FFFF00","content":"speed"},
    {"id":"b7","type":"metric","x":64,"y":16,"w":36,"h":8,"color":"#FF8800","content":"track"},
    {"id":"b8","type":"metric","x":64,"y":24,"w":48,"h":8,"color":"#FF8800","content":"vsi"}
  ]
}
```

**Fixture (c) — 24-widget max-density:**

```json
{
  "id": "gf_c",
  "name": "Golden C - Max Density",
  "v": 1,
  "canvas": {"w": 160, "h": 32},
  "bg": "#000000",
  "widgets": [
    {"id":"c01","type":"text","x":0,"y":0,"w":40,"h":8,"color":"#FFFFFF","content":"AA"},
    {"id":"c02","type":"text","x":40,"y":0,"w":40,"h":8,"color":"#FFFFFF","content":"BB"},
    {"id":"c03","type":"text","x":80,"y":0,"w":40,"h":8,"color":"#FFFFFF","content":"CC"},
    {"id":"c04","type":"text","x":120,"y":0,"w":40,"h":8,"color":"#FFFFFF","content":"DD"},
    {"id":"c05","type":"text","x":0,"y":8,"w":40,"h":8,"color":"#FFFF00","content":"EE"},
    {"id":"c06","type":"text","x":40,"y":8,"w":40,"h":8,"color":"#FFFF00","content":"FF"},
    {"id":"c07","type":"flight_field","x":80,"y":8,"w":40,"h":8,"color":"#00FF00","content":"airline"},
    {"id":"c08","type":"flight_field","x":120,"y":8,"w":40,"h":8,"color":"#00FF00","content":"ident"},
    {"id":"c09","type":"flight_field","x":0,"y":16,"w":40,"h":8,"color":"#00FFFF","content":"origin_icao"},
    {"id":"c10","type":"flight_field","x":40,"y":16,"w":40,"h":8,"color":"#00FFFF","content":"dest_icao"},
    {"id":"c11","type":"flight_field","x":80,"y":16,"w":40,"h":8,"color":"#FF8800","content":"aircraft"},
    {"id":"c12","type":"metric","x":120,"y":16,"w":40,"h":8,"color":"#FF8800","content":"alt"},
    {"id":"c13","type":"metric","x":0,"y":24,"w":40,"h":8,"color":"#FF00FF","content":"speed"},
    {"id":"c14","type":"metric","x":40,"y":24,"w":40,"h":8,"color":"#FF00FF","content":"track"},
    {"id":"c15","type":"metric","x":80,"y":24,"w":40,"h":8,"color":"#FF00FF","content":"vsi"},
    {"id":"c16","type":"clock","x":120,"y":24,"w":40,"h":8,"color":"#FFFFFF","content":"24h"},
    {"id":"c17","type":"text","x":0,"y":0,"w":20,"h":8,"color":"#888888","content":"a"},
    {"id":"c18","type":"text","x":20,"y":0,"w":20,"h":8,"color":"#888888","content":"b"},
    {"id":"c19","type":"text","x":40,"y":0,"w":20,"h":8,"color":"#888888","content":"c"},
    {"id":"c20","type":"text","x":60,"y":0,"w":20,"h":8,"color":"#888888","content":"d"},
    {"id":"c21","type":"text","x":80,"y":0,"w":20,"h":8,"color":"#888888","content":"e"},
    {"id":"c22","type":"text","x":100,"y":0,"w":20,"h":8,"color":"#888888","content":"f"},
    {"id":"c23","type":"text","x":120,"y":0,"w":20,"h":8,"color":"#888888","content":"g"},
    {"id":"c24","type":"text","x":140,"y":0,"w":20,"h":8,"color":"#888888","content":"h"}
  ]
}
```

> **Note:** Fixture (c) has 24 widgets which is exactly `MAX_WIDGETS`. LayoutStore schema validation requires each widget to have `x`, `y`, `w`, `h` within the canvas bounds (`w: 160, h: 32`). Verify all coordinates above are in-bounds.

---

### Golden-Frame Test Implementation

**Complete scaffold for `firmware/test/test_golden_frames/test_main.cpp`:**

```cpp
/*
Purpose: Golden-frame regression tests for CustomLayoutMode (Story LE-1.9).

Tests load fixture layouts, run them through CustomLayoutMode, and verify:
  (a) The layout parses correctly and _widgetCount matches the fixture definition
  (b) render() with nullptr matrix completes without crash (hardware-free path)
  (c) On-device: ESP.getFreeHeap() stays >= 30 KB for the max-density fixture
  (d) String resolution for flight_field and metric widgets matches expected values
      given a known FlightInfo — delegates to the same logic tested by
      test_flight_field_widget, exercised here at the integration level.

Golden-frame philosophy: LE-1.9 uses structural + string-resolution checks
rather than full framebuffer byte comparison. Rationale:
  1. Clock widget output is time-dependent; byte-exact buffers require
     a mock clock or regeneration on every test run.
  2. FastLED_NeoMatrix drawPixel() is not easily stubbed without linking
     the full GFX stack in native/test builds.
  3. The per-widget unit tests (test_flight_field_widget, test_metric_widget)
     already cover the rendering contract; this suite validates integration.

Environment: esp32dev (on-device). Run with:
    pio test -e esp32dev --filter test_golden_frames
*/
#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include <vector>

#include "core/ConfigManager.h"
#include "core/LayoutStore.h"
#include "core/WidgetRegistry.h"
#include "modes/CustomLayoutMode.h"
#include "models/FlightInfo.h"
#include "interfaces/DisplayMode.h"
#include "widgets/FlightFieldWidget.h"
#include "widgets/MetricWidget.h"

// -----------------------------------------------------------------------
// Fixture JSON strings (inline — avoids LittleFS dependency for
// the parse/structural tests; the LittleFS round-trip is covered
// separately by test_custom_layout_mode).
// These must exactly match the fixture JSON in fixtures/*.json.
// -----------------------------------------------------------------------

// Fixture A: text + clock (2 widgets)
static const char kFixtureA[] =
    "{\"id\":\"gf_a\",\"name\":\"Golden A\",\"v\":1,"
    "\"canvas\":{\"w\":160,\"h\":32},\"bg\":\"#000000\","
    "\"widgets\":["
    "{\"id\":\"w1\",\"type\":\"text\",\"x\":2,\"y\":2,\"w\":80,\"h\":8,"
    "\"color\":\"#FFFFFF\",\"content\":\"FLIGHTWALL\",\"align\":\"left\"},"
    "{\"id\":\"w2\",\"type\":\"clock\",\"x\":2,\"y\":12,\"w\":48,\"h\":8,"
    "\"color\":\"#FFFF00\",\"content\":\"24h\"}"
    "]}";

// Fixture B: 8 flight_field + metric widgets
static const char kFixtureB[] =
    "{\"id\":\"gf_b\",\"name\":\"Golden B\",\"v\":1,"
    "\"canvas\":{\"w\":160,\"h\":32},\"bg\":\"#000000\","
    "\"widgets\":["
    "{\"id\":\"b1\",\"type\":\"flight_field\",\"x\":0,\"y\":0,\"w\":60,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"airline\"},"
    "{\"id\":\"b2\",\"type\":\"flight_field\",\"x\":0,\"y\":8,\"w\":48,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"ident\"},"
    "{\"id\":\"b3\",\"type\":\"flight_field\",\"x\":0,\"y\":16,\"w\":36,\"h\":8,\"color\":\"#00FF00\",\"content\":\"origin_icao\"},"
    "{\"id\":\"b4\",\"type\":\"flight_field\",\"x\":40,\"y\":16,\"w\":36,\"h\":8,\"color\":\"#00FF00\",\"content\":\"dest_icao\"},"
    "{\"id\":\"b5\",\"type\":\"metric\",\"x\":64,\"y\":0,\"w\":36,\"h\":8,\"color\":\"#FFFF00\",\"content\":\"alt\"},"
    "{\"id\":\"b6\",\"type\":\"metric\",\"x\":64,\"y\":8,\"w\":48,\"h\":8,\"color\":\"#FFFF00\",\"content\":\"speed\"},"
    "{\"id\":\"b7\",\"type\":\"metric\",\"x\":64,\"y\":16,\"w\":36,\"h\":8,\"color\":\"#FF8800\",\"content\":\"track\"},"
    "{\"id\":\"b8\",\"type\":\"metric\",\"x\":64,\"y\":24,\"w\":48,\"h\":8,\"color\":\"#FF8800\",\"content\":\"vsi\"}"
    "]}";

// Fixture C: 24-widget max-density (see Dev Notes → Fixture JSON Formats)
// (abbreviated inline — paste full JSON here in implementation)
static const char kFixtureC[] =
    "{\"id\":\"gf_c\",\"name\":\"Golden C\",\"v\":1,"
    "\"canvas\":{\"w\":160,\"h\":32},\"bg\":\"#000000\","
    "\"widgets\":["
    /* ... 24 widget objects as shown in Dev Notes ... */
    "]}";

// -----------------------------------------------------------------------
// Test helpers
// -----------------------------------------------------------------------

static RenderContext makeCtx() {
    RenderContext ctx{};
    ctx.matrix         = nullptr;  // hardware-free path
    ctx.currentFlight  = nullptr;
    return ctx;
}

static FlightInfo makeKnownFlight() {
    FlightInfo f;
    f.ident                       = "UAL123";
    f.ident_icao                  = "UAL123";
    f.ident_iata                  = "UA123";
    f.operator_icao               = "UAL";
    f.origin.code_icao            = "KSFO";
    f.destination.code_icao       = "KLAX";
    f.aircraft_code               = "B738";
    f.airline_display_name_full   = "United Airlines";
    f.aircraft_display_name_short = "737";
    f.altitude_kft                = 34.0;
    f.speed_mph                   = 487.0;
    f.track_deg                   = 247.0;
    f.vertical_rate_fps           = -12.5;
    return f;
}

// Parse a layout from inline JSON — use CustomLayoutMode's parse path
// by temporarily saving to LittleFS (same as test_custom_layout_mode).
static bool loadFixture(CustomLayoutMode& mode, const char* json, const char* id) {
    LayoutStore::save(id, json);
    LayoutStore::setActiveId(id);
    RenderContext ctx = makeCtx();
    return mode.init(ctx);
}

static void cleanLayouts() {
    // Same cleanup as test_custom_layout_mode
    if (LittleFS.exists("/layouts")) {
        File dir = LittleFS.open("/layouts");
        if (dir && dir.isDirectory()) {
            File e = dir.openNextFile();
            while (e) {
                String p = String("/layouts/") + e.name();
                e.close();
                LittleFS.remove(p);
                e = dir.openNextFile();
            }
        }
        LittleFS.rmdir("/layouts");
    }
}

// -----------------------------------------------------------------------
// Golden-frame tests
// -----------------------------------------------------------------------

// --- Fixture A: text + clock ---

void test_golden_a_parse_and_widget_count() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureA, "gf_a"));
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)mode.testWidgetCount());
    TEST_ASSERT_FALSE(mode.testInvalid());
}

void test_golden_a_render_does_not_crash() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    loadFixture(mode, kFixtureA, "gf_a");
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> flights;
    mode.render(ctx, flights);
    TEST_PASS();
}

// --- Fixture B: 8-widget flight-field-heavy ---

void test_golden_b_parse_and_widget_count() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureB, "gf_b"));
    TEST_ASSERT_EQUAL_UINT(8, (unsigned)mode.testWidgetCount());
    TEST_ASSERT_FALSE(mode.testInvalid());
}

void test_golden_b_render_with_flight_does_not_crash() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    loadFixture(mode, kFixtureB, "gf_b");
    FlightInfo flight = makeKnownFlight();
    RenderContext ctx = makeCtx();
    ctx.currentFlight = &flight;
    std::vector<FlightInfo> flights;
    flights.push_back(flight);
    mode.render(ctx, flights);
    TEST_PASS();
}

// Verify that flight_field "airline" resolves to "United Airlines" for the
// known flight — integration-level check confirming CustomLayoutMode passes
// currentFlight to the widget renderers correctly.
void test_golden_b_flight_field_resolves_airline() {
    FlightInfo f = makeKnownFlight();
    // Construct a WidgetSpec matching fixture B widget b1
    WidgetSpec spec{};
    spec.type  = WidgetType::FlightField;
    spec.x = 0; spec.y = 0; spec.w = 60; spec.h = 8;
    spec.color = 0xFFFF;
    strncpy(spec.id, "b1", sizeof(spec.id) - 1);
    strncpy(spec.content, "airline", sizeof(spec.content) - 1);
    RenderContext ctx = makeCtx();
    ctx.currentFlight = &f;
    // renderFlightField with nullptr matrix returns true and resolves the field
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
    // (Pixel output cannot be verified without a real matrix — correctness of
    // the string resolution itself is validated by test_flight_field_widget.)
}

// --- Fixture C: 24-widget max-density ---

void test_golden_c_parse_and_widget_count() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureC, "gf_c"));
    TEST_ASSERT_EQUAL_UINT(24, (unsigned)mode.testWidgetCount());
    TEST_ASSERT_FALSE(mode.testInvalid());
}

void test_golden_c_render_does_not_crash() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    loadFixture(mode, kFixtureC, "gf_c");
    FlightInfo flight = makeKnownFlight();
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> flights;
    flights.push_back(flight);
    mode.render(ctx, flights);
    TEST_PASS();
}

// AC #4 — heap floor at max density (on-device only; meaningful with real LittleFS
// and RTOS heap accounting — not a compile-time check).
void test_golden_c_heap_floor() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    loadFixture(mode, kFixtureC, "gf_c");

    uint32_t heapBefore = ESP.getFreeHeap();
    FlightInfo flight = makeKnownFlight();
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> flights;
    flights.push_back(flight);
    mode.render(ctx, flights);
    uint32_t heapAfter = ESP.getFreeHeap();

    // AC #4: heap must remain above 30 KB floor.
    // Note: heapBefore is the heap AFTER loading a 24-widget layout; heapAfter
    // is after render(). Both should be well above 30 KB. If either is below,
    // the firmware is approaching an OOM condition.
    TEST_ASSERT_GREATER_OR_EQUAL(30720u, heapAfter);
    // Heap must not drop more than 2 KB across the render call (steady state).
    TEST_ASSERT_UINT32_WITHIN(2048, heapBefore, heapAfter);
}

// -----------------------------------------------------------------------
// Unity driver
// -----------------------------------------------------------------------

void setup() {
    delay(2000);

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed — aborting tests");
        return;
    }
    ConfigManager::init();

    UNITY_BEGIN();

    RUN_TEST(test_golden_a_parse_and_widget_count);
    RUN_TEST(test_golden_a_render_does_not_crash);

    RUN_TEST(test_golden_b_parse_and_widget_count);
    RUN_TEST(test_golden_b_render_with_flight_does_not_crash);
    RUN_TEST(test_golden_b_flight_field_resolves_airline);

    RUN_TEST(test_golden_c_parse_and_widget_count);
    RUN_TEST(test_golden_c_render_does_not_crash);
    RUN_TEST(test_golden_c_heap_floor);

    cleanLayouts();
    UNITY_END();
}

void loop() {}
```

---

### Verification Report Template

Create `_bmad-output/planning-artifacts/le-1-verification-report.md` with this structure:

```markdown
# LE-1 Verification Report

**Generated:** <ISO timestamp>
**Commit SHA:** <git rev-parse HEAD>
**Build environment:** `esp32dev` (PlatformIO, ESP32 Dev Module)
**Firmware version:** 1.0.0

## 1. Golden Fixtures

| Fixture ID | Widget Types | Widget Count | Parse | Render (null matrix) | Flight-field resolution |
|---|---|---|---|---|---|
| gf_a | text, clock | 2 | PASS | PASS | N/A |
| gf_b | flight_field ×4, metric ×4 | 8 | PASS | PASS | PASS (airline → "United Airlines") |
| gf_c | text ×12+, flight_field ×5, metric ×3, clock ×1 | 24 | PASS | PASS | PASS |

**Test command:** `pio test -e esp32dev --filter test_golden_frames`
**Result:** ALL PASS / FAIL (list failures)

## 2. Binary Size

| Metric | Value | Verdict |
|---|---|---|
| Binary size | ___ bytes | |
| Partition size | 1,572,864 bytes | |
| Usage % | ___% | |
| 92% cap (1,447,036 bytes) | | PASS / FAIL |
| 83% warning (1,305,477 bytes) | | WITHIN / ABOVE WARNING |
| Delta vs. main baseline | ___ bytes | PASS / FAIL (cap: 184,320 bytes) |

**Build command:** `pio run -e esp32dev`

## 3. Heap at Max Density (Fixture C)

| Metric | Value | Verdict |
|---|---|---|
| Free heap after 24-widget render | ___ bytes | PASS (≥ 30,720) / FAIL |
| Heap delta across render | ___ bytes | PASS (≤ 2,048) / FAIL |

**Note:** On-device measurement only; compile-only build shows N/A.

## 4. p95 Render Ratio (CustomLayoutMode / ClassicCardMode)

| Build | p50 (μs) | p95 (μs) | Source |
|---|---|---|---|
| ClassicCardMode baseline | ~950 | ~1,022–1,652 | Spike report 2026-04-17 |
| CustomLayoutMode (fixture b) | ___ | ___ | This measurement |
| Ratio (p95) | ___ × | PASS (≤ 1.2×) / FAIL |

**Note:** If hardware is unavailable for re-measurement, document as:
"Hardware measurement deferred — spike instrumentation retained in
`[env:esp32dev_le_spike]` / `[env:esp32dev_le_baseline]` in platformio.ini."

## 5. Cross-Story Compile Sweep

| Test Suite | Compile | Run |
|---|---|---|
| test_layout_store | PASS / FAIL | on-device |
| test_widget_registry | PASS / FAIL | on-device |
| test_custom_layout_mode | PASS / FAIL | on-device |
| test_web_portal | PASS / FAIL | on-device |
| test_logo_widget | PASS / FAIL | on-device |
| test_flight_field_widget | PASS / FAIL | on-device |
| test_metric_widget | PASS / FAIL | on-device |
| test_golden_frames | PASS / FAIL | on-device |

## 6. Epic LE-1 Completion Sign-Off

All LE-1 stories (1.1–1.9) are merged. This report documents the final gate state.

- [ ] Binary size gate: PASS
- [ ] Heap floor gate: PASS
- [ ] p95 ratio gate: PASS (or deferred with rationale)
- [ ] Golden frames: PASS
- [ ] Cross-story compile sweep: PASS
```

---

### Current `check_size.py` State

The current `firmware/check_size.py` is registered via `extra_scripts = pre:check_size.py` in `platformio.ini` but uses `AddPostAction` so it runs after the binary is built. The existing gates:

- Hard cap: 100% (1,572,864 bytes) → `env.Exit(1)`
- Warning: 83% (1,310,720 bytes) → print only

The story adds:
- 92% hard cap (1,447,036 bytes) at the same `AddPostAction` hook location
- Delta cap (184,320 bytes) against `.firmware_baseline_size`
- `SKIP_DELTA_CHECK` env variable escape hatch

**Do NOT remove** the existing 100% absolute cap — keep it as the final backstop. The 92% cap is the new actionable gate; the 100% cap prevents shipping a brick.

---

### `platformio.ini` — Spike Environments Must Be Preserved

The `[env:esp32dev_le_baseline]` and `[env:esp32dev_le_spike]` environments in `platformio.ini` are the instrumentation tool for AC #5. They are already correctly defined:

```ini
[env:esp32dev_le_baseline]
extends = env:esp32dev
build_flags =
    ${env:esp32dev.build_flags}
    -D LE_V0_METRICS

[env:esp32dev_le_spike]
extends = env:esp32dev
build_flags =
    ${env:esp32dev.build_flags}
    -D LE_V0_METRICS
    -D LE_V0_SPIKE
```

**Do NOT modify these environments.** They are preserved for the p95 re-measurement and for future profiling. The story only reads from them, it does not change them.

---

### CustomLayoutMode Test-Helper Methods

For the golden-frame tests, `CustomLayoutMode` must expose its internal state for test assertions. These test accessors are already present (added in LE-1.3):

```cpp
// CustomLayoutMode.h — test accessor methods (existing, do not add)
size_t testWidgetCount() const { return _widgetCount; }
bool   testInvalid()     const { return _invalid; }
void   testForceInvalid(bool v) { _invalid = v; }
```

These are already declared in `CustomLayoutMode.h` and implemented inline. The golden-frame test harness uses them exactly as `test_custom_layout_mode` does.

---

### `LayoutStore::save()` Schema Constraints

Fixture JSON must pass `LayoutStore::save()` validation. Key constraints:
- `"v": 1` is required
- `"id"`, `"name"`, `"canvas"` fields are required
- `"canvas"` must have `"w"` and `"h"` both > 0
- Each widget must have: `"id"`, `"type"`, `"x"`, `"y"`, `"w"`, `"h"`, `"color"`
- `"type"` must be in `kAllowedWidgetTypes[]` = `{"text", "clock", "logo", "flight_field", "metric"}`
- Widget bounds must be within canvas: `x + w <= canvas.w` and `y + h <= canvas.h`
- `LAYOUT_STORE_MAX_BYTES = 8192` — total fixture JSON must be < 8 KB
- Maximum 24 widgets per layout (`MAX_WIDGETS = 24` in `CustomLayoutMode.h`)

Verify fixture (c) JSON fits in 8,192 bytes before committing.

---

### Spike Report Reference — p95 Baseline

From the LE-0.1 spike report (`_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`):

- **ClassicCardMode** steady-state: p50 ≈ 950 μs, p95 ≈ 1,022–1,652 μs
- **CustomLayoutMode (optimized V0, 3 widgets)** steady-state: p50 ≈ 1,200 μs, p95 ≈ 1,260–2,118 μs
- **Ratio**: p95 ≈ 1.23× (within noise of the 1.2× gate)

For LE-1.9 AC #5, the measurement uses fixture (b) (8 widgets, including `flight_field` and `metric`), which is a heavier workload than the 3-widget V0 spike. The expectation is that the production LE-1.8 widget implementations (which do only `strcmp` chains + `DisplayUtils::truncateToColumns`) are lighter than the V0 `fillRect` logo widget, so the p95 ratio with 8 real widgets should be similar to or better than the V0 measurement.

If the p95 ratio exceeds 1.2×, the story does **not** fail — it **documents** the overage with root-cause analysis and creates a follow-up task. AC #5 is a measurement gate, not a hard build-fail gate.

---

### Antipattern Prevention Table

| DO NOT | DO INSTEAD | Reason |
|---|---|---|
| Use `let`/`const`/`=>`/template literals in any JS changes | No JS changes in LE-1.9 — this story is firmware + test only | ES5 constraint applies if any editor.js work is done |
| Modify `[env:esp32dev_le_baseline]` or `[env:esp32dev_le_spike]` in platformio.ini | Read them; they are already correctly configured | Modifying breaks future apples-to-apples comparisons |
| Store full 18 KB golden framebuffer binaries | Use structural + string-resolution checks as documented | Clock widgets are time-dependent; full buffer comparison requires a mock clock |
| Use `LayoutStore::save()` with invalid JSON | Use exact fixture JSON from Dev Notes — already validated | Schema validation rejects unknown types, out-of-bounds coords |
| Call `ESP.getFreeHeap()` before `LittleFS.begin()` | Call heap check only after full initialization | LittleFS uses heap for SPIFFS structures; early reading is misleading |
| Hard-fail the build if `.firmware_baseline_size` is absent | Print a warning and skip the delta check | Developers without CI baseline should still be able to build locally |
| Add new `#include` directives to `DisplayMode.h` or `WidgetRegistry.h` | Only create new test files; do not touch core headers | These headers are included everywhere; adding includes can cause build time regression and circular deps |
| Rename `check_size.py` or change the `extra_scripts` path | Edit `check_size.py` in-place | The path is hardcoded in `platformio.ini` |
| Delete or comment out the 100% absolute cap in `check_size.py` | Keep it — add the 92% cap above it | The 100% cap prevents bricking the device; it's the last-resort backstop |
| Create `test_golden_frames/` outside of `firmware/test/` | Create it at `firmware/test/test_golden_frames/` | PlatformIO auto-discovers `test_*` directories under `firmware/test/` |

---

### File List

Files **created** (new):
- `firmware/test/test_golden_frames/test_main.cpp` — Unity test suite (Tasks 1, 2)
- `firmware/test/test_golden_frames/fixtures/layout_a.json` — fixture (a) definition
- `firmware/test/test_golden_frames/fixtures/layout_b.json` — fixture (b) definition
- `firmware/test/test_golden_frames/fixtures/layout_c.json` — fixture (c) definition
- `firmware/.firmware_baseline_size` — integer byte count of main-branch binary (Task 3)
- `_bmad-output/planning-artifacts/le-1-verification-report.md` — measurement report (Task 6)

Files **modified**:
- `firmware/check_size.py` — add 92% hard cap + delta check + SKIP_DELTA_CHECK escape hatch (Task 3)

Files **NOT modified** (verify unchanged):
- `firmware/platformio.ini` — spike envs already present and correct; do not touch
- `firmware/interfaces/DisplayMode.h` — `RenderContext.currentFlight` already present from LE-1.8
- `firmware/core/WidgetRegistry.h` — enum and struct correct; no changes
- `firmware/core/WidgetRegistry.cpp` — `renderFlightField` and `renderMetric` wired from LE-1.8
- `firmware/widgets/FlightFieldWidget.cpp` — production implementation from LE-1.8
- `firmware/widgets/MetricWidget.cpp` — production implementation from LE-1.8
- `firmware/modes/CustomLayoutMode.cpp` — production implementation from LE-1.8
- `firmware/test/test_custom_layout_mode/test_main.cpp` — already covers AC #9; no changes
- `firmware/test/test_widget_registry/test_main.cpp` — updated in LE-1.8; no changes
- `firmware/test/test_flight_field_widget/test_main.cpp` — complete from LE-1.8; no changes
- `firmware/test/test_metric_widget/test_main.cpp` — complete from LE-1.8; no changes
- All other display modes, adapters, web assets — unchanged

---

## Dev Agent Record

### Implementation Summary

Story LE-1.9 is the final gate story for Epic LE-1. All eight tasks executed TDD-first with the structural + string-resolution golden-frame strategy documented in Dev Notes (full framebuffer byte comparison was ruled out due to clock widget time-dependence and the FastLED_NeoMatrix linking requirement). Hardware-free test path (`ctx.matrix = nullptr`) reused from `test_custom_layout_mode`, ensuring the suite compiles under the same `esp32dev` environment as the rest of the LE-1 tests.

### Key Decisions

- **Golden-frame strategy:** Structural parse + widget-count + null-matrix render + string-resolution assertions. No `.bin` blobs committed; fixture JSON files serve as the authoritative schema reference and inline C string literals drive the Unity tests.
- **Fixture storage:** Files in `firmware/test/test_golden_frames/fixtures/` plus mirrored inline `kFixture{A,B,C}` constants in `test_main.cpp`. The dual representation avoids requiring a LittleFS image upload during compile-only CI while keeping the JSON canonically versioned in-repo.
- **`check_size.py` SCons compatibility:** `__file__` is undefined in the SCons post-action context, so `get_main_baseline_size()` now takes an explicit `project_dir` parameter derived from `env.subst("$PROJECT_DIR")` at call time.
- **Baseline seeding:** `firmware/.firmware_baseline_size` populated with the current measured size (1,303,488 B) so delta check is active immediately; subsequent builds against the same SHA show +0 B delta, far under the 180 KB cap.
- **p95 deferral:** Spike environments (`esp32dev_le_spike`, `esp32dev_le_baseline`) left untouched in `platformio.ini`. AC #5 documented as measurement gate, not build-blocker, per story Dev Notes.

### Build Evidence

- Product binary: **1,303,488 bytes / 82.9% of 1,572,864 B partition / 143,546 B headroom under 92% cap**
- Delta vs. baseline: **+0 bytes** (PASS; cap 184,320 B)
- `pio run -e esp32dev`: SUCCESS (11.57 s)
- Compile sweep PASS across 8 suites: `test_layout_store`, `test_widget_registry`, `test_custom_layout_mode`, `test_web_portal`, `test_logo_widget`, `test_flight_field_widget`, `test_metric_widget`, `test_golden_frames` (new)
- Commit SHA at report time: `81bf6a1cb828198862b3c55825a69c4756c327b1`

### Acceptance Criteria Status

- AC #1 — Golden-frame suite (3 fixtures, structural + string-resolution): **PASS**
- AC #2 — 92% binary-size cap: **PASS** (82.9%, 143 KB headroom)
- AC #3 — 180 KB delta cap: **PASS** (+0 B delta against seeded baseline)
- AC #4 — Heap floor 30 KB at max density: **Assertion encoded** (`test_golden_c_heap_floor`), numeric verdict pending on-device run
- AC #5 — p95 ratio ≤ 1.2×: **Deferred** (hardware unavailable); V0 spike reference 1.23× documented
- AC #6 — Verification report: **DELIVERED** at `_bmad-output/planning-artifacts/le-1-verification-report.md`
- AC #7 — Cross-story compile sweep: **PASS**

### On-Device Follow-Ups

1. Flash + run `test_golden_frames` to confirm Unity assertions (primarily `test_golden_c_heap_floor`)
2. Flash `esp32dev_le_baseline` + `esp32dev_le_spike`; capture p95 samples on fixture B for ratio measurement

Both items are measurement gates; neither blocks build or merge.

### Review-Resolution Notes (2026-04-17, v1.2)

✅ Resolved review finding [LOW]: Aligned inline `kFixture{A,B,C}` `name` string literals in `firmware/test/test_golden_frames/test_main.cpp` with the canonical `fixtures/layout_{a,b,c}.json` files to eliminate dual-source drift. Post-fix re-validation: product build PASS (`pio run -e esp32dev`, 1,303,488 B / 82.9% / +0 B delta vs. baseline, all LE-1.9 budget gates green); `test_golden_frames` compile-check PASS (`pio test -e esp32dev --filter test_golden_frames --without-uploading --without-testing`, 17.82 s).

### File List

**Created:**
- `firmware/test/test_golden_frames/test_main.cpp`
- `firmware/test/test_golden_frames/fixtures/layout_a.json`
- `firmware/test/test_golden_frames/fixtures/layout_b.json`
- `firmware/test/test_golden_frames/fixtures/layout_c.json`
- `firmware/.firmware_baseline_size`
- `_bmad-output/planning-artifacts/le-1-verification-report.md`

**Modified:**
- `firmware/check_size.py` — added 92% hard cap, 180 KB delta cap via `.firmware_baseline_size` artifact, `SKIP_DELTA_CHECK` escape hatch

**Not modified (verified unchanged):**
- `firmware/platformio.ini` (spike envs preserved)
- `firmware/interfaces/DisplayMode.h`
- `firmware/core/WidgetRegistry.{h,cpp}`
- `firmware/widgets/FlightFieldWidget.cpp`, `firmware/widgets/MetricWidget.cpp`
- `firmware/modes/CustomLayoutMode.cpp`
- All prior LE-1 test suites (`test_layout_store`, `test_widget_registry`, `test_custom_layout_mode`, `test_web_portal`, `test_logo_widget`, `test_flight_field_widget`, `test_metric_widget`)

---

## Senior Developer Review (AI)

### Review: 2026-04-17 (Round 1)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 1.3 → APPROVED with Reservations
- **Issues Found:** 5 verified (2 fixed in source code; 3 deferred/documentation)
- **Issues Fixed:** 2 (check_size.py docstring/comment typo; silent exception swallowing)
- **Action Items Created:** 3

#### Review Follow-ups (AI)
- [ ] [AI-Review] HIGH: AC #4 heap floor (30 KB) not yet validated on physical hardware — flash + run `test_golden_frames` on ESP32 device and record actual `ESP.getFreeHeap()` readings in verification report Section 3 (`firmware/test/test_golden_frames/test_main.cpp`)
- [ ] [AI-Review] HIGH: AC #5 p95 render-ratio measurement deferred — flash `esp32dev_le_baseline` and `esp32dev_le_spike`, capture 3×35-second serial windows on fixture B, compute `CustomLayoutMode p95 / ClassicCardMode p95` ratio and record in verification report Section 4 (`firmware/platformio.ini`, `_bmad-output/planning-artifacts/le-1-verification-report.md`)
- [x] [AI-Review] LOW: Inline kFixture{A,B,C} C strings in test_main.cpp have shorter `name` fields than the canonical `fixtures/layout_{a,b,c}.json` files (e.g. "Golden A" vs "Golden A - Text+Clock") — align names to eliminate dual-source drift or add a CI checksum guard (`firmware/test/test_golden_frames/test_main.cpp`, `firmware/test/test_golden_frames/fixtures/`) — **RESOLVED 2026-04-17**: inline `kFixture{A,B,C}` `name` strings updated to match canonical JSON ("Golden A - Text+Clock", "Golden B - FlightField Heavy", "Golden C - Max Density"). Product build SUCCESS (1,303,488 B / 82.9% / +0 B delta); `test_golden_frames` compile-check PASS (17.82 s).

### Review: 2026-04-17 (Round 2 — Synthesis of 3 Reviewers)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 1.1 → APPROVED with Reservations
- **Issues Found:** 2 confirmed new (both already tracked as open AI-Review items from Round 1; no new code defects found)
- **Issues Fixed:** 0 (all Round 2 reviewer findings were false positives or already addressed in Round 1)
- **Action Items Created:** 0 (all open items already captured in Round 1 follow-ups above)

---

## Change Log

| Date | Version | Description | Author |
|---|---|---|---|
| (original draft) | 0.1 | Draft story created — thin skeleton with placeholder dev notes | BMAD |
| 2026-04-17 | 1.0 | Upgraded to ready-for-dev. Comprehensive dev notes synthesized from live codebase analysis (post-LE-1.8 state). Key additions: (1) Golden-frame test strategy clarifying that full framebuffer byte comparison is impractical with clock widgets + FastLED_NeoMatrix dependency, and documenting the structural + string-resolution check approach that is compatible with the existing test infrastructure; (2) `PixelMatrix` stub design sketch for optional pixel-level tests; (3) Complete `test_golden_frames/test_main.cpp` scaffold with all 8 required tests and exact LittleFS round-trip pattern from `test_custom_layout_mode`; (4) All three fixture JSON definitions with coordinates validated against 160×32 canvas and LayoutStore schema constraints; (5) Exact `check_size.py` before/after showing the 92% cap addition, delta check logic, `.firmware_baseline_size` artifact approach, and `SKIP_DELTA_CHECK` escape hatch; (6) Verification report template with all required sections from AC #6; (7) Reference to spike report baseline figures (ClassicCardMode p95 ≈ 1,022–1,652 μs, CustomLayoutMode p95 ≈ 1,260–2,118 μs at 1.23× ratio in V0); (8) Antipattern prevention table with 9 entries; (9) File list with explicit "NOT modified" section. Status changed from `draft` to `ready-for-dev`. | BMAD Story Synthesis |
| 2026-04-17 | 1.1 | Implementation complete — all 8 tasks executed. Fixtures A/B/C authored (449/935/2410 B, all under 8 KB schema cap). `test_main.cpp` implements 9 Unity tests covering structural parse, null-matrix render, flight-field/metric string resolution, and fixture-C heap floor (≥ 30 KB) + heap-delta-during-render tolerance (±2 KB). `check_size.py` extended with 92% hard cap, main-branch delta cap (180 KB), `SKIP_DELTA_CHECK` escape hatch, and `.firmware_baseline_size` artifact (seeded with 1,303,488 B). Verification report produced. Product build: 1,303,488 B / 82.9% / +0 B delta / 143 KB headroom under 92% cap. Compile sweep PASS across all 7 prior LE-1 test suites + new `test_golden_frames`. p95 render-ratio measurement deferred (hardware unavailable) with rationale and preserved spike envs. Status: review. | Dev Agent |
| 2026-04-17 | 1.2 | Addressed code review findings — 1 item resolved. [AI-Review] LOW item closed: inline `kFixture{A,B,C}` `name` string literals in `firmware/test/test_golden_frames/test_main.cpp` aligned with canonical JSON fixtures ("Golden A - Text+Clock", "Golden B - FlightField Heavy", "Golden C - Max Density") — eliminates dual-source drift. Re-validated: product build PASS (1,303,488 B / 82.9% / +0 B delta vs. baseline; all LE-1.9 budget gates green); `test_golden_frames` compile-check PASS (17.82 s). Two remaining [AI-Review] HIGH items (AC #4 heap floor on-device verification, AC #5 p95 ratio measurement) remain open as documented measurement gates requiring physical hardware — not build-blocking per story Dev Notes. | Dev Agent |


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

## Story le-1-8 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | `editor.js` `showPropsPanel` — Round 2 fix applied `widgets[selectedIndex].content = coerced` when content was coerced to a valid option, but did NOT set `dirty = true`. A subsequent save without user interaction would NOT persist the coerced (correct) key because the dirty flag was false and the save guard returned early. The operator sees the canonical dropdown value but the JSON on device retains the stale/invalid key. | Added `dirty = true;` inside the `!found` coercion guard, and regenerated `editor.js.gz`. |

## Story le-1-9 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | AC #4 heap floor (≥ 30 KB) not empirically validated | Not a code defect — assertion is correctly encoded in `test_golden_c_heap_floor`; on-device execution is the outstanding action. Documented as `[AI-Review] HIGH` action item. |
| high | AC #5 p95 render-ratio measurement deferred, no empirical data | Not a code defect — story Dev Notes explicitly designate this as a "measurement gate, not build-blocker." Documented as `[AI-Review] HIGH` action item. |
| medium | `get_main_baseline_size()` swallows all exceptions with bare `except Exception`, silently skipping the delta check even when the baseline file exists but contains corrupt data, with no operator notification | Changed `except Exception` to `except (ValueError, OSError) as e` with an explicit `print(f"WARNING: ...")` before returning `None`. **APPLIED.** |
| low | `check_size.py` docstring and inline comment both claim `CAP_92_PCT` is `1,447,036 bytes` — the actual Python value of `int(0x180000 * 0.92)` is `1,447,034`. The comment was copied from the story Dev Notes prose which contains a typo. The code computes the correct value; the comment creates false audit documentation. | Updated docstring line to `1,447,034 bytes` and the inline comment to `# 1,447,034 bytes`. **APPLIED.** |
| low | Inline `kFixture{A,B,C}` C string literals in `test_main.cpp` have abbreviated `name` fields ("Golden A") compared to the canonical `fixtures/layout_{a,b,c}.json` files ("Golden A - Text+Clock") — dual-source drift without a checksum guard | Not a functional defect (`name` is metadata only; LayoutStore keys off `id`, not `name`). Documented as `[AI-Review] LOW` action item to either align names or add a guard. |
| dismissed | Fixture JSON files not committed to repo ("CRITICAL #3") | FALSE POSITIVE: Files `layout_a.json` (449 B), `layout_b.json` (935 B), and `layout_c.json` (2,410 B) exist at `firmware/test/test_golden_frames/fixtures/`. They are untracked by git because the story branch is in `review` status and has not yet been merged/committed, which is normal. They are present on disk. FALSE POSITIVE. |
| dismissed | 92% cap value is wrong — code uses `1,447,036` but should be `1,447,034` | FALSE POSITIVE: Reviewer A inverted the error. The story Dev Notes *prose* contains the typo (`1,447,036`); the **code** correctly computes `int(0x180000 * 0.92) = 1,447,034`. The verification report also correctly states `1,447,034`. The comment typo was fixed as part of this synthesis. FALSE POSITIVE (in the reverse direction — the code was correct, not the prose). |
| dismissed | Reviewer C says verification report shows wrong cap value `1,447,034` vs correct `1,447,036` | FALSE POSITIVE: Python `int(0x180000 * 0.92) = 1,447,034`. The verification report is CORRECT. Reviewer C treated the story Dev Notes prose typo as the ground truth. FALSE POSITIVE. |
| dismissed | Binary size measurement not from clean build / potential non-determinism | FALSE POSITIVE: ESP32/PlatformIO builds are deterministic for the same toolchain version and source tree. The seeding pattern (baseline = current build) is the documented approach from the story Dev Notes. No evidence of non-determinism. FALSE POSITIVE. |
| dismissed | `.firmware_baseline_size` has no integrity check / can be bypassed | FALSE POSITIVE: The delta cap is a developer-governance tool, not a security boundary. The story Dev Notes explicitly say "maintained manually or by a CI release step." Adding cryptographic signatures to a plain-text CI artifact file would be disproportionate to the risk. Any such edit would also appear in git diff and be visible during code review. FALSE POSITIVE. |
| dismissed | Fixture (c) widget count `24 == MAX_WIDGETS` not asserted as a constant reference | FALSE POSITIVE: `test_golden_c_parse_and_widget_count()` asserts `mode.testWidgetCount() == 24`. This is semantically correct — MAX_WIDGETS is also 24 (verified in `CustomLayoutMode.h:23`). A `TEST_ASSERT_EQUAL(MAX_WIDGETS, mode.testWidgetCount())` would be marginally stronger, but the current assertion already gates against regression when fixtures change. Low severity; deferred per project patterns. FALSE POSITIVE (as a blocking issue). |
| dismissed | "Golden frame" tests cannot detect regressions in clock rendering (AC #1 incomplete) | FALSE POSITIVE: The story Dev Notes Golden-Frame Test Strategy section explicitly documents this limitation and provides the architectural rationale (clock widget time-dependence; FastLED_NeoMatrix GFX stack linking requirement). The story's own AC text says "matches the recorded golden data in the specified way (see Dev Notes — Golden-Frame Test Strategy for the exact comparison approach)." The structural + string-resolution approach IS the specified approach. The tests implement the AC as defined. FALSE POSITIVE. |
| dismissed | `check_size.py` error messages use human-readable comma formatting, breaking CI parsing | FALSE POSITIVE: `check_size.py` is a PlatformIO SCons script, not a CI-parsed artifact. PlatformIO CI integrations capture exit codes, not parse stdout text. The readable format is correct for operator and CI log readability. No CI system in the project context is configured to parse this output numerically. FALSE POSITIVE. |
| dismissed | `MEMORY_REQUIREMENT` static assert is a false sense of security | FALSE POSITIVE: The assert correctly enforces a minimum bound on the declared requirement constant. The story Dev Notes document what `MEMORY_REQUIREMENT` accounts for. The claimed risk ("a mode could allocate more than declared") would require intentional code changes that would be caught in review. Not a bug in the current implementation. FALSE POSITIVE. |
| dismissed | PixelMatrix stub described in Dev Notes but never implemented | FALSE POSITIVE: The Dev Notes correctly document the PixelMatrix stub as a "recommended approach" and "optional enhancement", then explicitly state: "Practical recommendation for LE-1.9: Implement a `PixelMatrix` stub... if the implementor chooses to wire PixelMatrix as a FastLED_NeoMatrix-compatible class." The story explicitly documents the "hardware-free path (default)" as the chosen approach. The Dev Notes are not a spec — they are a design reference. FALSE POSITIVE. |
| dismissed | Fixture JSON not versioned or migratable for future schema evolution | FALSE POSITIVE: Architectural concern for future stories, not a defect in the current implementation. The fixtures are V1 JSON passing LayoutStore schema validation. Future schema changes would require updating all fixtures regardless of whether they are in-file or inline. FALSE POSITIVE as a current defect. |
| dismissed | Heap baseline measurement omits WiFi/web server production overhead | FALSE POSITIVE: The test runs on-device with full RTOS + LittleFS + NVS + ConfigManager initialized (by `setup()`), which is representative of the operational heap floor under the display-task workload. The story specifically tests the heap impact of the render path, not the combined WiFi+display total. AC #4 targets display-path headroom. FALSE POSITIVE as a gate violation. |
| dismissed | Verification report missing PlatformIO/toolchain version | FALSE POSITIVE: Low-impact documentation gap. The commit SHA is sufficient for reproducibility within the same toolchain version. Adding toolchain metadata is a future improvement, not a story defect. FALSE POSITIVE. |
| dismissed | Test binary heap vs product binary heap not cross-verified | FALSE POSITIVE: Unity test binaries for ESP32 include the Unity harness and test infrastructure but exclude production subsystems (WiFi, AsyncWebServer, OTA). The difference is well-understood and works in the conservative direction (test binary uses MORE heap for Unity overhead relative to display path, so the 30 KB floor measured in tests is a pessimistic bound). FALSE POSITIVE. |
| dismissed | `test_golden_*_render_does_not_crash` tests are vacuous — `CustomLayoutMode::render` returns immediately on null matrix | FALSE POSITIVE: `CustomLayoutMode::render()` short-circuits on `ctx.matrix == nullptr` at line 201 of `CustomLayoutMode.cpp`. This is documented in the test comment ("Null-matrix short-circuits in CustomLayoutMode::render() before any dispatch — reaching TEST_PASS is itself the assertion."). The test IS the no-crash contract under the hardware-free path. The story explicitly designates this as the AC #1 approach. The `renderFlightField`/`renderMetric` direct-call tests (`test_golden_b_flight_field_resolves_airline`, `test_golden_b_metric_resolves_alt`) provide the integration smoke. FALSE POSITIVE as a test honesty violation (within the documented approach). |
| dismissed | AC #3 implementation (baseline file) doesn't match AC #3 wording ("git merge-base with main") | FALSE POSITIVE: The story Dev Notes explicitly document the chosen implementation: "A `.size` artifact file approach is recommended — simple and CI-portable." The Delta Check Logic section in Dev Notes is the authoritative specification for what was implemented, and AC #3 says "see Dev Notes — delta check logic." The baseline-file approach is the story's own documented design. FALSE POSITIVE. |
| dismissed | Delta gate is neutralized (+0 B forever) | FALSE POSITIVE: The story Dev Notes explicitly address this: "For LE-1.9, create it with the current binary size measured in Task 8." This is the intended seeding behavior. Future merges after LE-1.9 will show positive deltas against the 1,303,488 B baseline. The gate is functioning exactly as designed. FALSE POSITIVE. |
| dismissed | `SKIP_DELTA_CHECK` only suppresses an INFO message, not an actual hard-fail | FALSE POSITIVE: Story Dev Notes say: "default: fail gracefully with warning, not hard-fail, when baseline is absent." When the baseline is absent, the current code does exactly that (INFO print, no `env.Exit`). `SKIP_DELTA_CHECK` suppresses the INFO message — consistent with the documented escape hatch purpose. The name is slightly misleading but the behavior matches the spec. FALSE POSITIVE. |
| dismissed | `CustomLayoutMode::init()` `Serial.printf` heap logs are too noisy for production | FALSE POSITIVE: The logs are intentional per the story's Task 5 ("Add heap log on every successful init") and are consistent with all other modes in the codebase. They appear only on layout activation, not per-frame. The project does not yet have log-level macros for this class of output. Out of scope for LE-1.9. FALSE POSITIVE. |
| dismissed | Integration tests call `renderFlightField`/`renderMetric` directly, bypassing `WidgetRegistry::dispatch` | FALSE POSITIVE: The story Dev Notes Golden-Frame Test Implementation explicitly designs the fixture B integration tests this way: "verify that the first `flight_field` widget would render the expected string (test `resolveField()` logic via `renderFlightField()` with a null matrix...)." `WidgetRegistry::dispatch` wiring is covered by `test_widget_registry`. FALSE POSITIVE. --- |

## Story le-1-9 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | AC #4 heap floor (≥ 30 KB) not validated on physical hardware | Not a code defect — assertion correctly encoded in `test_golden_c_heap_floor()`; numeric verdict requires on-device flash. Already tracked as `[AI-Review] HIGH` from Round 1. |
| high | AC #5 p95 render-ratio measurement deferred with no empirical data | Not a code defect — story Dev Notes explicitly designate this a "measurement gate, not build-blocker." Spike environments preserved. Already tracked as `[AI-Review] HIGH` from Round 1. |
| low | Inline `kFixture{A,B,C}` C strings use abbreviated `name` fields ("Golden A") vs. canonical `.json` files ("Golden A - Text+Clock") | Not a functional defect — `LayoutStore` keys off `id`, not `name`. Already tracked as `[AI-Review] LOW` from Round 1. No code change applied. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/le-1-9-golden-frame-tests-and-budget-gate.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **3** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | AC #4 (Heap Floor) not yet validated on physical hardware; test harness encoded, but verdict pending on-device execution. | `firmware/test/test_golden_frames/test_main.cpp` | +1 |
| 🟠 IMPORTANT | AC #5 (p95 ratio) measurement deferred; spike environments exist but require physical hardware for empirical validation. | `firmware/platformio.ini` | +1 |
| 🟠 IMPORTANT | `get_main_baseline_size()` uses bare `except Exception` which swallows all errors (including `KeyboardInterrupt`) and defaults to skipping the delta check; dangerous practice. | `firmware/check_size.py:88-104` | +1 |

### Evidence Score: 3.0

| Score | Verdict |
|-------|---------|
| **3.0** | **MAJOR REWORK** |

---

## 🐍 Pythonic Crimes & Readability

- **Exception Swallowing:**
  - `check_size.py` uses `except Exception` inside `get_main_baseline_size()`. This is an antipattern as it masks all errors (e.g., `KeyboardInterrupt`, `MemoryError`) and silently falls back to skipping the delta check.
  - 📍 `firmware/check_size.py:101-103` (implied scope)
  - 💡 Fix: Be explicit with `except (ValueError, OSError) as e`.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** AC #4 (Heap Floor) not yet validated on physical hardware. The test harness (`test_golden_c_heap_floor`) is correctly encoded, but requires physical hardware to generate the `ESP.getFreeHeap()` output.
  - 📍 `firmware/test/test_golden_frames/test_main.cpp`
  - 🔄 Reproduction: `pio test -e esp32dev --filter test_golden_frames` on physical ESP32 hardware.

---

## ⚡ Performance & Scalability

- **Deferred Measurement:** AC #5 (p95 Render Ratio) requires physical hardware measurement using `esp32dev_le_baseline` and `esp32dev_le_spike`. This gate cannot be verified in the build/test environment.

---

**Review Actions:**
- Issues Found: 3
- Issues Fixed: 0
- Action Items Created: 0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**ADVERSARIAL CODE REVIEW COMPLETE, Christian**

**Story:** `_bmad-output/implementation-artifacts/stories/le-1-9-golden-frame-tests-and-budget-gate.md`  
**Scope:** Story claims vs embedded implementation evidence (LE-1.9 zone: `firmware/test/test_golden_frames/**`, `firmware/check_size.py`, verification report; cross-read `CustomLayoutMode.cpp` where tests interact).

---

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story / File List Integrity | 1 |
| AC Implementation Gaps / Weak Gates | 4 |
| Task Completion Integrity | 1 |
| Test Honesty / Coverage | 2 |
| Architecture / Conventions | 1 |
| Python / Tooling Quality | 1 |
| **Total Issues** | **10** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| CRITICAL | Task 5 contains `[x]` subtasks explicitly “deferred / hardware unavailable” while parent Task 5 is also `[x]` — completed checkboxes contradict undone work (story integrity / sprint hygiene). | `le-1-9-golden-frame-tests-and-budget-gate.md` (Tasks / Subtasks, Task 5) | +3.0 |
| IMPORTANT | AC #1 requires exercising `CustomLayoutMode::render()` for fixtures; `CustomLayoutMode::render()` returns immediately when `ctx.matrix == nullptr`, so the “render” tests do not traverse dispatch / flight binding in `CustomLayoutMode` at all. | `firmware/modes/CustomLayoutMode.cpp` (~201–205), `firmware/test/test_golden_frames/test_main.cpp` | +1.0 |
| IMPORTANT | “Golden-frame” suite does not assert resolved strings (only `render*()` return true on null matrix); AC #1’s “matches golden data” is satisfied only by narrative, not by observable framebuffer or string oracle at integration boundary. | `firmware/test/test_golden_frames/test_main.cpp` | +1.0 |
| IMPORTANT | AC #4 asks heap under max-density *while `CustomLayoutMode` is active through render*; with null-matrix early return, the heap test largely measures post-`init()` steady state, not 24-widget render pressure. | `CustomLayoutMode.cpp`, `test_main.cpp` (`test_golden_c_heap_floor`) | +1.0 |
| IMPORTANT | AC #5 is not closed with hardware numbers; verification report marks verdict TBD/deferred — story cannot claim full AC closure without revising AC or status. | `_bmad-output/planning-artifacts/le-1-verification-report.md` §4, §6 | +1.0 |
| MINOR | AC #3 prose references `git merge-base HEAD main`; implementation is a committed integer baseline file only — traceability to `main` is procedural, not enforced by tooling. | `firmware/check_size.py`, story AC #3 | +0.3 |
| MINOR | `CustomLayoutMode` uses raw `Serial.printf` for operational logs; project architecture prefers `LOG_*` macros from `utils/Log.h` for consistent compile-time log levels. | `firmware/modes/CustomLayoutMode.cpp` | +0.3 |
| MINOR | `SKIP_DELTA_CHECK` name implies it can skip an enforced delta failure; it only silences the informational message when baseline is absent — operator confusion risk. | `firmware/check_size.py` | +0.3 |
| CLEAN PASS | No meaningful new network attack surface in `check_size.py` (local post-build gate; no secrets). | `firmware/check_size.py` | −0.5 |
| CLEAN PASS | No classic memory-safety defect introduced in the Python gate (straight size math + file read). | `firmware/check_size.py` | −0.5 |

### Evidence Score: **7.2**

| Score | Verdict |
|-------|---------|
| **7.2** | **MAJOR REWORK** (process + measurement gates: treat as “merge with documented follow-ups” or tighten story/AC before calling LE-1.9 done) |

---

## Architectural / Process Sins

- **[7/10] Gate honesty vs hexagonal logging discipline:** Production `CustomLayoutMode` continues to emit unconditional `Serial.printf` diagnostics on init/parse paths (`CustomLayoutMode.cpp`), which bypasses the repo’s stated logging convention (`LOG_*` in `utils/Log.h`). Not a functional break, but it is a consistency and operability footgun (noisy serial, no compile-time level gating).

- **[6/10] Story ↔ workspace integrity:** The story’s “Files **NOT modified**” list asserts `firmware/modes/CustomLayoutMode.cpp` unchanged for LE-1.9, yet the workspace `git status` snapshot shows `CustomLayoutMode.cpp` modified. Even if changes are “from earlier LE-1 stories,” the LE-1.9 story file is misleading as an audit artifact.

---

## Python / Tooling Readability

- **Doc/operator mismatch:** `SKIP_DELTA_CHECK` behavior is narrower than the name suggests (baseline-missing path only). Prefer renaming (`SILENCE_BASELINE_INFO=1`) or documenting prominently in `platformio.ini` / developer docs.

- **Baseline governance:** `.firmware_baseline_size` is correct for a delta gate, but it is not mechanically tied to `origin/main` HEAD; reviewers must treat it as a release-management artifact (human drift risk).

---

## Performance and Scalability

- **False confidence on heap gate:** With `render()` early-returning on `nullptr` matrix, the “max density render” heap assertion is not measuring the hot path that production uses (matrix non-null, `fillScreen`, per-widget dispatch). This is performance-test design debt: it can PASS while a real-device render regresses.

---

## Correctness and Safety

- **Integration vacuity:** `test_golden_b_render_with_flight_does_not_crash` sets `flights` and `ctx.currentFlight`, but `CustomLayoutMode::render()` never reaches the `widgetCtx.currentFlight = …` assignment when `matrix == nullptr`. The test proves “no crash,” not “CustomLayoutMode binds flight data correctly during render.”

- **“Golden frame” semantics:** Without pixel OR string oracle at the `CustomLayoutMode` boundary, regressions in `WidgetRegistry::dispatch` wiring or `widgetCtx` construction can slip past these tests while standalone widget tests still pass.

---

## Lying / Weak Tests (explicit)

- **`test_golden_a_render_does_not_crash` / `test_golden_b_render_with_flight_does_not_crash` / `test_golden_c_render_does_not_crash`:** These are structurally weak for AC #1’s stated intent (“through `CustomLayoutMode::render()`”), because `CustomLayoutMode::render()` is a no-op at the top when `matrix == nullptr`.

- **`test_golden_b_flight_field_resolves_airline` / `test_golden_b_metric_resolves_alt`:** Useful as widget-level smoke, but they do not prove `CustomLayoutMode` passes the same `RenderContext` shape/fields as production `ModeRegistry` wiring.

---

## Maintainability / Tech Debt

- **Dual-source fixtures:** Inline `kFixtureA/B/C` plus `fixtures/layout_*.json` is an ongoing sync tax; the story acknowledges drift risk. A cheap guard is a CI step that hashes both sources or generates one from the other at build time.

- **Verification report completeness:** Section 3 remains TBD for on-device heap numbers while story sign-off partially checks AC #4 — this is documentation debt that becomes release risk if the epic is declared “closed.”

---

## Suggested Fixes (no auto-apply; approval required)

1. **Fix the Task 5 checkbox integrity:** Either uncheck deferred subtasks, or split “measurement” into a new story with `[ ]` tasks; do not mark `[x]` what is explicitly not executed.

2. **Make AC #1 honest at the integration boundary (pick one):**
   - Add a minimal `FastLED_NeoMatrix`-compatible test stub (PixelMatrix approach), **or**
   - Change tests to call `WidgetRegistry::dispatch` under a synthetic `RenderContext` assembled the same way `CustomLayoutMode` does (still not full GFX, but at least exercises dispatch wiring), **or**
   - Narrow AC #1 text to explicitly match the null-matrix contract (if that is truly intended).

3. **Make AC #4 meaningful:** Gate heap after a real render path (non-null matrix stub that exercises `fillScreen` + dispatch), or move the heap assertion to an on-device integration harness that uses the production matrix.

4. **Close AC #5 or downgrade it:** Either capture the spike serial windows and fill §4, or change epic/story status language so “deferred” is not coexisting with “all tasks `[x]`.”

5. **Replace `Serial.printf` in `CustomLayoutMode` with `LOG_I` / `LOG_E` equivalents** (where macro limitations allow), or document a narrow exception with rationale.

6. **Rename or document `SKIP_DELTA_CHECK`** to match behavior; optionally add a `DELTA_BASELINE_REF=git_sha` comment inside `.firmware_baseline_size` format (even as `# comment` lines) to reduce baseline drift ambiguity.

---

## Review Actions

- Issues Found: **10**  
- Issues Fixed: **0** (read-only review)  
- Action Items Created: **6** (embedded above as suggested fixes)

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW REPORT - Story LE-1.9**

**Story:** le-1-9-golden-frame-tests-and-budget-gate

---

## 📊 Review Summary

| Category | Count |
|----------|-------|
| AC Implementation Gaps | 2 |
| Task Completion Issues | 1 |
| Code Quality Issues | 3 |
| Hidden Bugs | 2 |
| Test Quality Issues | 2 |
| Performance Concerns | 1 |
| Security/Robustness | 1 |
| **Total Issues Found** | **12** |

---

## Evidence Score Breakdown

| Severity | Finding | Source | Points |
|----------|---------|--------|--------|
| 🔴 CRITICAL | AC #4 heap floor assertion never executed on real hardware - test marked complete but unmeasured | test_golden_frames/test_main.cpp:1 | +3 |
| 🟠 IMPORTANT | AC #5 p95 measurement completely deferred with zero empirical data collected | story Dev Notes | +1 |
| 🟠 IMPORTANT | `check_size.py` docstring error: claims 1,447,036 when code correctly computes 1,447,034 bytes | firmware/check_size.py:10 | +1 |
| 🟠 IMPORTANT | Heap baseline must be measured BEFORE implementation but fixture relies on post-init heap state | test_golden_c_heap_floor | +1 |
| 🟡 MINOR | `kFixture{A,B,C}` inline C strings have abbreviated names vs canonical JSON files (dual-source drift) | test_golden_frames/test_main.cpp:40-80 | +0.3 |
| 🟢 CLEAN | Security validation in check_size.py is adequate | firmware/check_size.py | -0.5 |
| 🟢 CLEAN | JSON fixture formats are correctly validated against LayoutStore schema | fixtures/*.json | -0.5 |

**Evidence Score: 5.3** → **MAJOR REWORK** (Significant issues, esp. unmeasured ACs)

---

## 🏛️ Critical Architectural Issues

### 1. **AC #4 is Incomplete - Assertion Exists But Never Executes**

**Severity:** 🔴 CRITICAL (Build gate bypass)

**Issue:** Story claims AC #4 is complete:
```
- [x] **Task 4: Heap regression check** (AC: #4)
      - [x] Covered by Task 2 — the 24-widget fixture test asserts `ESP.getFreeHeap() >= 30720`
```

However, examining `firmware/test/test_golden_frames/test_main.cpp` line 303:
```cpp
TEST_ASSERT_GREATER_OR_EQUAL(30720u, heapAfter);
```

**The assertion is BACKWARDS.** `TEST_ASSERT_GREATER_OR_EQUAL(30720u, heapAfter)` means:
- Assert: `30720 >= heapAfter` (must be ≤30KB)
- **This FAILS when heap is healthy (>30KB)** and PASSES when heap is critically low!

The AC requirement is "≥ 30,720 bytes" but the assertion enforces "≤ 30,720 bytes" — a 180-degree logic inversion.

**File:** `firmware/test/test_golden_frames/test_main.cpp:303`

**Impact:** The safety gate for AC #4 is broken. The fixture could be consuming 300KB of heap and the test would pass.

---

### 2. **AC #5 Deferred But Story Marked Complete**

**Severity:** 🟠 IMPORTANT (Promise unkept)

The story claims in the Acceptance Criteria checklist:
```
- AC #5 — p95 render-ratio measurement: **Deferred**
```

But the story status is `Status: review` and the Record claims completion. The [AI-Review] HIGH action items acknowledge the deferral, but **the story sign-off in AC section is misleading.**

Reading the actual AC #5 requirement:
> "Given the spike's `[env:esp32dev_le_spike]` instrumentation in `platformio.ini` **When a fresh p95 render-time measurement is captured on fixture (b)** Then the `CustomLayoutMode` p95 ratio is ≤ 1.2×"

**Zero empirical data exists.** The verification report Section 4 admits: "TBD on-device validation required; spike instrumentation retained in platformio.ini for future measurement."

This is a gate being bypassed. The story should be `Status: incomplete` with a hard blocker, or AC #5 should be explicitly deferred in the AC checklist itself (not just in Dev Notes).

**File:** `_bmad-output/planning-artifacts/le-1-verification-report.md:Section 4`

---

## 🐛 Hidden Bugs & Test Quality Issues

### 3. **Fixture (C) Heap Floor Test Logic Error**

**Severity:** 🔴 CRITICAL (Measurement useless)

In `firmware/test/test_golden_frames/test_main.cpp`, the `test_golden_c_heap_floor` function (line 288-305):

```cpp
uint32_t heapBefore = ESP.getFreeHeap();
// ... render fixture C ...
uint32_t heapAfter = ESP.getFreeHeap();

TEST_ASSERT_GREATER_OR_EQUAL(30720u, heapAfter);
```

**Three problems:**

1. **Assertion is backwards** (see Issue #1 above)
2. **Heap measurement timing is wrong** — `heapBefore` is sampled AFTER `loadFixture()` (which allocates ~4KB for WidgetSpec array), so `heapBefore` is already post-allocation
3. **No heap baseline established** — The story says "measured on hardware after flashing" but the test runs post-initialization with ConfigManager/LittleFS/WiFi already consuming heap. This is not a measurement of "heap used by the render path" — it's a measurement of "remaining heap after full boot"

**Impact:** The 30 KB floor measurement is unreliable and cannot be trusted for budget validation.

**Fix:** Reverse assertion to `TEST_ASSERT_LESS_OR_EQUAL(30720u, heapAfter)` and document the measurement context clearly.

---

### 4. **`check_size.py` Docstring Contains Hardcoded Wrong Value**

**Severity:** 🟠 IMPORTANT (Documentation misleads auditors)

File: `firmware/check_size.py` line 10-12

The docstring states:
```python
"""
... CAP_92_PCT       (1,447,036 bytes) — LE-1.9 hard cap
"""
```

But the actual code (line 24) computes:
```python
CAP_92_PCT = int(PARTITION_SIZE * 0.92)  # 1,447,034 bytes
```

`int(0x180000 * 0.92) = 1,447,034`, NOT `1,447,036`.

The story's own Dev Notes contain the typo (`1,447,036`), which was carelessly copied into the docstring. The code is CORRECT, the documentation is WRONG.

**Impact:** During an audit, someone reading the docstring will think the cap is set to `1,447,036`, but the actual gate is `1,447,034` — a 2-byte discrepancy that could cause confusion if someone manually computes margins.

**File:** `firmware/check_size.py:10`

---

### 5. **Lying Test: `test_golden_a_render_does_not_crash`**

**Severity:** 🟡 MINOR (False confidence)

File: `firmware/test/test_golden_frames/test_main.cpp:100-106`

```cpp
void test_golden_a_render_does_not_crash() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureA, "gf_a"));
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> flights;
    mode.render(ctx, flights);
    TEST_PASS();
}
```

This test **does not actually test rendering**. The `ctx.matrix = nullptr` (from `makeCtx()`) causes `CustomLayoutMode::render()` to short-circuit at line 201:
```cpp
if (ctx.matrix == nullptr) return;
```

**The test proves:** "Render returns without crash when matrix is null."
**The test does NOT prove:** "Render works correctly, produces valid output, or doesn't corrupt memory."

The test comment admits this: `// hardware-free test path — widgets no-op safely` — but then TEST_PASS is misleading for someone reading the test in isolation.

**Impact:** AC #1 requirements include "render through `CustomLayoutMode::render()`" but this test doesn't actually exercise the rendering logic.

---

### 6. **Lying Test: `test_golden_c_heap_floor` Executes Backward Assertion**

**Severity:** 🔴 CRITICAL (Gate is inverted)

See Issue #3 above. The assertion `TEST_ASSERT_GREATER_OR_EQUAL(30720u, heapAfter)` is logically backwards and will fail when heap is healthy.

---

## 🔧 Maintainability & Process Issues

### 7. **Fixture JSON Inline C Strings Have Dual-Source Drift**

**Severity:** 🟡 MINOR (Future maintenance risk)

The `kFixture{A,B,C}` inline C string literals in `test_main.cpp` (lines 40, 85, 130) have different `name` values than the canonical JSON fixture files:

- **Inline C string**: `"Golden A"` → **Canonical JSON**: `"Golden A - Text+Clock"`
- **Inline C string**: `"Golden B"` → **Canonical JSON**: `"Golden B - FlightField Heavy"`
- **Inline C string**: `"Golden C"` → **Canonical JSON**: `"Golden C - Max Density"`

While `LayoutStore::save()` keys off the `id` field (not `name`), having two sources of truth creates future maintenance risk if someone changes one without updating the other.

**File:** `firmware/test/test_golden_frames/test_main.cpp:41, 86, 131`

---

### 8. **Missing `.firmware_baseline_size` File Integrity Check**

**Severity:** 🟠 IMPORTANT (Data integrity risk)

File: `firmware/check_size.py:45-57`

The `get_main_baseline_size()` function reads the baseline file but has NO validation that the content is actually a valid integer:

```python
with open(baseline_path, "r") as f:
    return int(f.read().strip())  # Crashes if not an integer!
```

If someone accidentally writes `"2MB"` or leaves the file empty or contains a SHA hash, this crashes with an unhelpful error instead of gracefully skipping the delta check.

**File:** `firmware/check_size.py:51-53`

**Fix:** Add explicit error handling:
```python
try:
    return int(f.read().strip())
except (ValueError, TypeError):
    print(f"WARNING: .firmware_baseline_size is corrupted (not an integer)")
    return None
```

---

## ⚡ Performance & Correctness Concerns

### 9. **Heap Baseline Measurement Prerequisite Not Enforced**

**Severity:** 🟠 IMPORTANT (False sense of safety)

The story says: "Before any Display System code is written, measure free heap after full Foundation Release boot."

But the `test_golden_c_heap_floor` test runs AFTER all the initialization code. It measures heap with WiFi, LittleFS, NVS, ConfigManager, and potentially flight fetcher already consuming memory.

There is NO separate "baseline" measurement captured before test fixtures are loaded. The test conflates:
- Heap remaining after full boot (baseline)
- Heap remaining after loading 24-widget fixture (with heap allocations)

**Impact:** AC #4 compliance cannot be verified because there's no reference point for "normal idle heap usage."

---

### 10. **`check_size.py` `except Exception` Too Broad**

**Severity:** 🟠 IMPORTANT (Debugging nightmares)

File: `firmware/check_size.py:46`

```python
try:
    with open(baseline_path, "r") as f:
        return int(f.read().strip())
except Exception:  # Catches EVERYTHING: KeyboardInterrupt, MemoryError, etc!
    return None
```

This catches `KeyboardInterrupt` and `SystemExit`, silently suppressing user intent to abort the build. Should catch only `(ValueError, OSError)`.

**File:** `firmware/check_size.py:46`

---

## 📋 Task Completion Audit

### 11. **Task 5: Measurement Date Stamp Missing**

**Severity:** 🟡 MINOR (Incomplete documentation)

Task 5 specification:
> "**When a fresh p95 render-time measurement is captured on fixture (b)** ... the ratio and timestamps are documented in the verification report"

The verification report Section 4 states: "Hardware measurement deferred — on-device validation required."

**There is NO timestamp for when (or if) this measurement was actually captured.** The report lacks the "approximate timestamp for each measurement" required by AC #6.

**File:** `_bmad-output/planning-artifacts/le-1-verification-report.md:Section 4`

---

### 12. **Task 8: Binary Size Measurement Not Marked With Timestamp**

**Severity:** 🟡 MINOR (Audit trail incomplete)

Task 8 requires verification report to include "commit SHA and approximate timestamp for each measurement."

The verification report Section 2 states:
```
Commit SHA: 81bf6a1cb828198862b3c55825a69c4756c327b1
```

But provides no timestamp for when this binary size was measured. The "2026-04-17T18:16Z" at the top of the report is the report generation time, not the measurement time.

**File:** `_bmad-output/planning-artifacts/le-1-verification-report.md:Section 2`

---

## 🔒 Security & Robustness

### 13. **`check_size.py` Silent Failure on Missing NVS File**

**Severity:** 🟠 IMPORTANT (Process failure hidden)

The delta check gracefully returns `None` when `.firmware_baseline_size` is missing, which then silently skips the check with only an INFO log:

```python
print("INFO: No main baseline file found (.firmware_baseline_size). Delta check skipped.")
```

On a CI/CD system that doesn't display stdout by default, this message could be missed entirely, and the delta gate would silently never fire. A developer could merge a 500 KB firmware addition without any gate tripping.

**File:** `firmware/check_size.py:58-60`

**Fix:** Unless `SKIP_DELTA_CHECK=1` is explicitly set, the missing baseline should be a WARNING at minimum, or configurable as a hard gate.

---

## 📊 Summary & Recommendations

### Critical Issues Requiring Immediate Fix:

1. **AC #4 assertion is backwards** — Test will fail when heap is healthy. This is a blocker.
2. **AC #5 measurement is completely missing** — Story claims completion but provides zero data.
3. **Fixture C test logic is wrong** — Heap baseline is post-allocation, not pre-allocation.

### Important Issues for Next Sprint:

4. Documentation drift (check_size.py docstring)
5. Error handling gaps (baseline file parsing)
6. Test quality issues (lying tests with null matrix)

### Recommendations:

- **Do NOT merge this story** without fixing the backward assertion and re-measuring AC #4 on real hardware
- **Explicitly mark AC #5 as deferred** in the AC checklist, not just in Dev Notes
- **Add CI guard** for missing `.firmware_baseline_size` — make it a hard gate with explicit skip mechanism
- **Implement heap baseline measurement** as a separate pre-test step, not post-initialization

---

**Evidence Score: 5.3 (MAJOR REWORK)**

**Verdict: REJECT pending fixes to critical assertions and AC measurement gaps**

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
<var name="session_id">978421dc-747a-460a-8006-9333fbcca1b9</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="ca0ac87a">embedded in prompt, file id: ca0ac87a</var>
<var name="story_id">le-1.9</var>
<var name="story_key">le-1-9-golden-frame-tests-and-budget-gate</var>
<var name="story_num">9</var>
<var name="story_title">9-golden-frame-tests-and-budget-gate</var>
<var name="template">False</var>
<var name="timestamp">20260417_1842</var>
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
      - Commit message format: fix(component): brief description (synthesis-le-1.9)
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