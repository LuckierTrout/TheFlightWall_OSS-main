---
branch: le-0-1-v0-feasibility-spike
zone:
  - firmware/modes/CustomLayoutMode.*
  - firmware/modes/v0_spike_layout.h
  - firmware/src/main.cpp
  - firmware/test/test_custom_layout_v0/**
  - _bmad-output/planning-artifacts/layout-editor-v0-spike-report.md
---

# Story LE-0.1 — Layout Editor V0 Feasibility Spike

Status: draft

<!-- Spike story. NOT V1. Measurement exercise with hard go/no-go gates. Does not ship to users. -->

## Story

As **Christian (product owner)**,
I want **an on-device measurement of whether a generic widget renderer fits within the ESP32's render-time, binary-size, and heap budgets**,
So that **the go/no-go decision on the full layout editor is driven by real numbers, not speculation**.

## Acceptance Criteria

1. **Given** the spike branch is built **When** the project tree is inspected **Then** `firmware/modes/CustomLayoutMode.h` and `firmware/modes/CustomLayoutMode.cpp` exist **And** `CustomLayoutMode` inherits from the `DisplayMode` interface **And** it is registered as the 5th entry in the static mode table in `firmware/src/main.cpp` **And** registration is wrapped in `#ifdef LE_V0_SPIKE` so non-spike builds do not include it.

2. **Given** the spike build is flashed **When** `CustomLayoutMode` initializes **Then** it parses a JSON layout embedded as `const char[]` in `firmware/modes/v0_spike_layout.h` (no LittleFS read) **And** the layout targets a 192×48 canvas **And** the layout contains at least one `text_static` widget, one `clock` widget, and one `logo` widget **And** the JSON is valid (verified offline via `python3 -m json.tool` before embedding) **And** the three widgets render legibly on the physical panel.

3. **Given** `CustomLayoutMode::setup()` runs **When** the layout JSON is parsed **Then** parsing uses ArduinoJson v7 `JsonDocument` with a filter document **And** parsing happens exactly once (not per frame) **And** the `JsonDocument` goes out of scope / is freed after `std::vector<WidgetInstance>` (or fixed-capacity array, N≤8) is populated **And** no JSON APIs are called inside `render()`.

4. **Given** `CustomLayoutMode::render()` is called **When** widgets are dispatched **Then** dispatch uses a static lookup (`std::array<Entry,N>` linear scan OR `switch` on widget-type enum) **And** no heap allocation occurs per frame **And** three render functions exist and are wired: `renderTextStatic`, `renderClock`, `renderLogoStub` (logo may draw a solid-color rect for the spike — RGB565 blit deferred to V1).

5. **Given** the spike build runs `CustomLayoutMode` for 60 seconds on hardware **When** serial logs are captured **Then** `CustomLayoutMode::render()` is bracketed with `micros()` **And** per-frame render time (EXCLUDING `FastLED.show()`) is logged **And** min / mean / p95 / max are reported over the 60s window. **PASS gate:** p95 render time ≤ (1.2 × mean render time of `ClassicCardMode` measured with the same instrumentation macro on the same build).

6. **Given** `pio run` output from both the baseline build (no `LE_V0_SPIKE`) and the spike build (with `LE_V0_SPIKE`) **When** binary sizes are compared **Then** Δbinary is reported in bytes and KB. **PASS gate:** Δbinary ≤ 25 KB.

7. **Given** the spike build runs on hardware **When** heap is logged via `ESP.getFreeHeap()` and `ESP.getMinFreeHeap()` at three checkpoints — (a) before first mode switch, (b) after switch to `CustomLayoutMode` with layout loaded, (c) after 60s of rendering **Then** values are emitted to serial with labels. **PASS gate:** free-heap at checkpoint (b) ≥ 30 KB margin AND drift from (b) to (c) < 2 KB (no monotonic leak).

8. **Given** the AC #5 measurement **When** render-time numbers are reported **Then** renderer cost is reported SEPARATELY from `FastLED.show()` cost (FastLED.show() is fixed overhead independent of renderer).

9. **Given** Tasks 6 and 7 have captured measurements **When** the spike is concluded **Then** a report exists at `_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md` **And** it contains: Methodology, Measurements (render-time, Δbinary, heap), a Gate-results table (AC #5, #6, #7 each marked PASS/FAIL with numbers), and a 3-sentence "GO / NO-GO / CONDITIONAL-GO" verdict **And** serial-log excerpts (or screenshots) are attached as evidence.

10. **Given** a main-branch build (no `LE_V0_SPIKE` flag) **When** `pio run` is invoked **Then** `CustomLayoutMode` is NOT registered in the mode table **And** the binary does not link the spike render code **And** binary size matches the pre-spike baseline within ±256 bytes (accounting for unrelated build noise).

## Tasks / Subtasks

- [ ] **Task 1: Scaffold `CustomLayoutMode` + feature flag** (AC: #1, #10)
  - [ ] Create `firmware/modes/CustomLayoutMode.h` — class inherits `DisplayMode`, declares `setup()`, `teardown()`, `render()`, `getName()`
  - [ ] Create `firmware/modes/CustomLayoutMode.cpp` — minimal stub bodies, `#include "v0_spike_layout.h"` placeholder
  - [ ] In `firmware/src/main.cpp`, wrap the 5th `ModeEntry` + factory + memory-requirement function in `#ifdef LE_V0_SPIKE` / `#endif`
  - [ ] In `firmware/platformio.ini`, add a new env `[env:esp32dev_le_spike]` that `extends = env:esp32dev` and adds `build_flags = ${env:esp32dev.build_flags} -DLE_V0_SPIKE`
  - [ ] **Verify:** `pio run -e esp32dev` builds clean (spike code excluded); `pio run -e esp32dev_le_spike` builds clean (spike code included)

- [ ] **Task 2: Embed hand-coded JSON layout** (AC: #2)
  - [ ] Create `firmware/modes/v0_spike_layout.h` declaring `static const char V0_SPIKE_LAYOUT[] PROGMEM = R"JSON({...})JSON";`
  - [ ] Layout must include: 1× `text_static` (e.g., "FLIGHTWALL"), 1× `clock` (HH:MM), 1× `logo` (bounds only, stub render)
  - [ ] Target canvas 192×48; widgets positioned non-overlapping
  - [ ] **Verify:** extract the JSON string and pipe through `python3 -m json.tool` — must parse without error

- [ ] **Task 3: Parse layout once at mode init** (AC: #3)
  - [ ] Define `struct WidgetInstance { WidgetType type; int16_t x, y, w, h; char payload[32]; };` in `CustomLayoutMode.h` (fixed-size payload avoids heap)
  - [ ] In `CustomLayoutMode::setup()`, construct a local `JsonDocument`, configure filter doc (whitelist: `widgets[].type`, `widgets[].x`, `widgets[].y`, `widgets[].w`, `widgets[].h`, `widgets[].text`)
  - [ ] Iterate `doc["widgets"]`, populate `m_widgets` (fixed-capacity array, N=8, track count), then let `JsonDocument` drop at end of scope
  - [ ] Log `Serial.printf("[LE_V0] Parsed %u widgets", count)` as evidence
  - [ ] **Verify:** add an assert/check that `render()` does not call any `ArduinoJson` symbol (grep)

- [ ] **Task 4: Widget dispatch + 3 render functions** (AC: #4)
  - [ ] Implement static dispatch in `CustomLayoutMode::render()` using a `switch` on `WidgetInstance::type`
  - [ ] `renderTextStatic(const WidgetInstance&, BaseDisplay&)` — draw `payload` at (x,y) using existing 5×7 font from `firmware/utils/DisplayUtils`
  - [ ] `renderClock(const WidgetInstance&, BaseDisplay&)` — format HH:MM from `TimeUtils::getLocalTime()`, draw at (x,y)
  - [ ] `renderLogoStub(const WidgetInstance&, BaseDisplay&)` — fill rect (x,y,w,h) with a solid color (actual RGB565 blit deferred to V1)
  - [ ] **Verify:** `pio test -e esp32dev_le_spike -f test_custom_layout_v0 --without-uploading --without-testing` compiles clean

- [ ] **Task 5: Instrumentation macros + heap + render-time logging** (AC: #5, #7, #8)
  - [ ] Add file-scope macro `#define RENDER_TIMING_PROBE(name) ...` in a new minor header (or inline in `main.cpp`) that captures `micros()` before/after a render call and pushes into a circular buffer of size 256
  - [ ] Apply `RENDER_TIMING_PROBE("custom_layout")` around `CustomLayoutMode::render()` body (internal) AND `RENDER_TIMING_PROBE("classic_card")` around `ClassicCardMode::render()` body for apples-to-apples baseline
  - [ ] Log summary (min/mean/p95/max) every 10s via a Core 0 ticker
  - [ ] Emit heap at three checkpoints with tags: `[LE_V0][HEAP][pre_switch]`, `[LE_V0][HEAP][post_switch]`, `[LE_V0][HEAP][t+60s]` using `ESP.getFreeHeap()` and `ESP.getMinFreeHeap()`
  - [ ] Explicitly exclude `FastLED.show()` from the probe — probe wraps only the mode's render call, not the display flush

- [ ] **Task 6: Capture baseline measurement (ClassicCardMode)** (AC: #5 baseline, #10)
  - [ ] `cd firmware && ~/.platformio/penv/bin/pio run -e esp32dev` — record binary size from output
  - [ ] Flash, let `ClassicCardMode` run for 60s, capture serial log
  - [ ] Record ClassicCardMode render-time min/mean/p95/max
  - [ ] Save log excerpt for spike report appendix

- [ ] **Task 7: Capture spike measurement (CustomLayoutMode)** (AC: #5, #6, #7)
  - [ ] `cd firmware && ~/.platformio/penv/bin/pio run -e esp32dev_le_spike` — record binary size
  - [ ] Compute Δbinary = spike_size − baseline_size
  - [ ] Flash, switch to `CustomLayoutMode`, let run 60s, capture serial log
  - [ ] Record CustomLayoutMode render-time min/mean/p95/max
  - [ ] Record three heap checkpoints
  - [ ] Compute gate results: (AC #5) p95_custom ≤ 1.2 × mean_classic ? (AC #6) Δbinary ≤ 25 KB ? (AC #7) heap_post ≥ 30KB AND drift < 2KB ?

- [ ] **Task 8: Write spike report** (AC: #9)
  - [ ] Create `_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`
  - [ ] Sections: `## Methodology` | `## Measurements` | `## Gate Results` (table: Gate, Measured, Threshold, PASS/FAIL) | `## Verdict` (GO / NO-GO / CONDITIONAL-GO, 3 sentences) | `## Appendix: Serial Logs`
  - [ ] Paste serial-log excerpts for both baseline and spike runs as evidence
  - [ ] If CONDITIONAL-GO, enumerate the specific optimization(s) required before V1

- [ ] **Task 9: Verify feature flag isolates spike code** (AC: #10)
  - [ ] `pio run -e esp32dev` (no spike flag) — confirm `CustomLayoutMode` NOT in registered modes list at boot
  - [ ] Compare binary size against pre-spike baseline commit — Δ should be ≤ 256 bytes (noise from instrumentation macro if left linked in classic build)
  - [ ] If instrumentation macro affects non-spike builds meaningfully, guard it too with `#ifdef LE_V0_SPIKE`

## Dev Notes

### Why this spike exists

Mary's market-research evidence for the layout-editor concept was inconclusive. The remaining unknown is **technical feasibility at this form factor**: does a data-driven, JSON-described renderer fit inside the ESP32's render-time / binary / heap budgets without degrading the existing display experience? Winston's round-table V0 scope: *"Hard-coded JSON in LittleFS. No editor. Firmware loads + renders 3 widget types (text, logo, clock). Prove the render loop fits in frame budget."* We simplify further by embedding the JSON as a `const char[]` in flash — this isolates the render-budget question from LittleFS I/O.

### Why no LittleFS in the spike

LittleFS read timing is a separately-characterized path (well understood, bounded, cacheable-at-init). Mixing it in would muddy the render-budget signal. If the render budget FAILS here, filesystem integration is moot. If it PASSES, LittleFS is a known-solved follow-on.

### Why we measure ClassicCardMode as baseline

Relative gate, not absolute. Firmware version, FastLED version, and ambient compile flags drift over time; absolute microsecond thresholds go stale. A relative threshold ("spike p95 ≤ 1.2× classic mean") stays meaningful across builds. `ClassicCardMode` is chosen as the established, shipping reference mode.

### Why the feature flag (`LE_V0_SPIKE`)

We may need to leave spike code in the tree while the GO/NO-GO decision is being made, without that code shipping to users via main. The `#ifdef` + dedicated PlatformIO env gives us: (a) zero binary-size impact on production builds, (b) a single-command reproducible spike build, (c) a clean deletion path if the verdict is NO-GO.

### Scope anti-goals (explicit "do NOT implement" list)

- NO editor UI (HTML/JS drag-and-drop) — that's V1
- NO REST endpoints for layout CRUD — that's V1
- NO LittleFS layout read — deferred
- NO `LogoStore` or `WidgetRegistry` abstraction — inline dispatch is fine for 3 widget types
- NO additional widget types beyond `text_static` / `clock` / `logo` (stub) — 3 is enough to prove dispatch cost
- NO persistence — the `const char[]` IS the persistence for the spike
- NO RGB565 logo blit — `renderLogoStub` draws a colored rect and that's sufficient to measure dispatch cost
- NO integration with `ModeOrchestrator` scheduling rules
- NO automated frame-rate self-regulation — just measure what we get

### Decision path after spike

- **All gates PASS** → write V1 epic `LE-1 Layout Editor MVP` (editor UI, REST API, LittleFS persistence, full widget library)
- **Any gate FAIL** → spike report is the evidence to KILL the layout-editor concept; pivot to Mary's Mode SDK or John's V0 remix-templates direction
- **Marginal** (e.g., AC #6 lands at 23 KB of 25 KB, or AC #5 lands at 1.18× instead of ≤ 1.2×) → CONDITIONAL-GO with a follow-up optimization story scoped explicitly against the measured shortfall

### Instrumentation macro sketch

```cpp
// Conceptual — exact impl deferred to Task 5
struct RenderProbe {
    const char* name;
    uint32_t samples[256];
    uint16_t head = 0;
    uint16_t count = 0;
    uint32_t start_us = 0;
};
#define RENDER_TIMING_PROBE_BEGIN(probe) (probe).start_us = micros()
#define RENDER_TIMING_PROBE_END(probe)   do { \
    uint32_t dt = micros() - (probe).start_us; \
    (probe).samples[(probe).head] = dt; \
    (probe).head = ((probe).head + 1) % 256; \
    if ((probe).count < 256) (probe).count++; \
} while(0)
```

Summary logger runs every 10s, computes sorted copy for p95. OK to use a small stack buffer — 256 × 4 bytes = 1 KB; negligible.

### Relevant existing code pointers

- Mode interface: `firmware/interfaces/DisplayMode.h`
- Mode table + factory pattern: `firmware/src/main.cpp` (static `ModeEntry` array)
- ClassicCardMode for instrumentation reference: `firmware/modes/ClassicCardMode.cpp`
- Fonts + text rendering: `firmware/utils/DisplayUtils.{h,cpp}` (4×6, 5×7, 6×10 bitmap fonts)
- Time formatting: `firmware/utils/TimeUtils.h`
- ArduinoJson v7 filter doc pattern: see existing usage in `firmware/adapters/*Fetcher.cpp`
- PlatformIO env inheritance pattern: `firmware/platformio.ini` existing `[env:esp32dev]`

### Gotchas

1. **Do NOT measure the first frame.** Warm up for ~2 seconds before capturing, to let cache effects settle.
2. **`FastLED.show()` is single-threaded and blocks** — it must stay OUTSIDE the probe. If it's inside, every mode looks the same and the measurement is worthless.
3. **Heap logging must pin to a known task** — `ESP.getFreeHeap()` is process-wide, but calling it from Core 0 vs. Core 1 makes no functional difference. Just be consistent.
4. **`pio run -e esp32dev_le_spike` will produce a different binary path** — check `.pio/build/esp32dev_le_spike/firmware.bin` for the size number, not the default path.
5. **Instrumentation code size.** If `RENDER_TIMING_PROBE` macro is unconditionally linked into ClassicCardMode on production builds, that shifts the baseline. Either accept the shift (small) or `#ifdef LE_V0_SPIKE` the macro out of non-spike builds. See Task 9 verification.

### References

- [Source: _bmad-output/planning-artifacts/layout-editor-evidence-hunt.md]
- [Source: _bmad-output/planning-artifacts/architecture.md — Hexagonal architecture, Core 0 render constraints]
- [Source: _bmad-output/planning-artifacts/epics.md — LE epic placeholder]
- [Source: Winston round-table V0 scope — "Hard-coded JSON in LittleFS. No editor. 3 widget types..."]

### Dependencies

**This story depends on:**
- `ds-1.3` (ModeRegistry and static registration system) — COMPLETE
- `ds-1.4` (ClassicCardMode) — COMPLETE (used as baseline)
- `ds-1.5` (Display pipeline integration) — COMPLETE

**Stories that depend on this:**
- `LE-1.x` (Layout Editor MVP) — epic gated on PASS verdict from this spike

## File List

- `firmware/modes/CustomLayoutMode.h` (new)
- `firmware/modes/CustomLayoutMode.cpp` (new)
- `firmware/modes/v0_spike_layout.h` (new)
- `firmware/src/main.cpp` (modified — feature-flagged 5th mode entry + `RENDER_TIMING_PROBE` applied to `ClassicCardMode::render` for baseline)
- `firmware/platformio.ini` (modified — new `[env:esp32dev_le_spike]` env extending `esp32dev` with `-DLE_V0_SPIKE`)
- `firmware/test/test_custom_layout_v0/test_main.cpp` (new — minimal JSON-parse + widget-dispatch unit test; compile-only acceptable)
- `_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md` (new — populated during Task 8)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.7 (1M context) — Story Creation (Amelia)

### Debug Log References

N/A — Story creation phase

### Completion Notes List

_(to be populated during dev-story execution)_

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-17 | Story created — V0 feasibility-spike scope defined with hard numeric gates | Amelia |

### File List

_(to be populated during dev-story execution)_
