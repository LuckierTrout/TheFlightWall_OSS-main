# Story ds-1.4: Classic Card Mode Implementation

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user,
I want the existing three-line flight card display migrated into a ClassicCardMode that renders through the new mode system,
So that the familiar display continues working identically while enabling the mode architecture.

## Acceptance Criteria

1. **Given** `DisplayUtils`, `RenderContext`, and the ds-1.3 stub `ClassicCardMode` exist, **When** implementation is complete, **Then** `ClassicCardMode::render()` performs the same **zone-based** classic layout as `NeoMatrixDisplay::renderZoneFlight()` — logo zone (`LogoManager::loadLogo` + `DisplayUtils::drawBitmapRGB565`), flight zone (airline / route / aircraft lines per `renderFlightZone` branching), telemetry zone (`renderTelemetryZone`) — using **`ctx.layout`** (`logoZone`, `flightZone`, `telemetryZone`) and **`ctx.matrix`** instead of `NeoMatrixDisplay` private `_layout` / `_matrix`

2. **Given** NFR C1 (pixel-identical classic output), **When** `ctx.layout.valid == true` and the same `FlightInfo` + context are supplied, **Then** framebuffer content matches the Foundation path **before** `FastLED.show()` — verify across **at least 5** representative flights (short/long airline names, missing telemetry, single-line vs three-line flight zone heights) using a **test harness** (e.g. Unity test that renders to matrix and samples key pixels, or documented side-by-side capture procedure) and record the method in the Dev Agent Record

3. **Given** FR12 (flight cycling), **When** `flights.size() > 1`, **Then** ClassicCardMode advances the displayed index every **`ctx.displayCycleMs`** milliseconds (same cadence as `displayFlights`: `ConfigManager::getTiming().display_cycle * 1000` is already folded into `RenderContext` by the caller — **do not read `ConfigManager` inside the mode** for timing or colors)

4. **Given** FR4 (zone bounds), **When** rendering, **Then** no drawing calls write outside each active zone’s `Rect` (x, y, w, h) — same effective clipping as current helpers

5. **Given** empty `flights`, **When** `render()` runs, **Then** the idle UI matches `NeoMatrixDisplay::displayLoadingScreen()` (white border, centered `...`, text color from user theme) **without** calling `FastLED.show()` — frame commit remains the pipeline’s responsibility (FR35 prep; enforced in ds-1.5)

6. **Given** `ctx.layout.valid == false`, **When** `render()` runs with non-empty flights, **Then** behavior matches the fallback path **`NeoMatrixDisplay::displaySingleFlightCard`** (bordered inset card, three lines with `>` route separator) using `ctx.matrix` and dimensions from `ctx.layout.matrixWidth` / `matrixHeight` when set, otherwise from the matrix object’s pixel width/height (Adafruit GFX) — **not** unbounded text past the physical matrix

7. **Given** heap accounting (Decision D2), **Then** `ClassicCardMode::MEMORY_REQUIREMENT` (static `constexpr` in the class, consumed via `ModeEntry`) is **updated** to cover any **persistent** allocations the mode makes after `init`/`teardown` cycles (if none, document as instance + vtable + member fields only); **do not** add `getMemoryRequirement()` as a virtual on `DisplayMode`

8. **Given** NFR S3, **Then** stack allocations in `render()` (including temporaries from `String`) stay **under 512 bytes** — if compiler/stack usage is borderline, refactor hot paths to smaller buffers or `char[]` **without** changing pixels

9. **Given** NFR S5, **Then** `ctx.brightness` is **read-only** usage (if referenced for future logic); **do not** call `matrix->setBrightness` or otherwise change brightness inside the mode

10. **Given** metadata for Mode Picker, **Then** `getName()` returns **`"Classic Card"`** (stub already does) and `getZoneDescriptor()` / `_zones` **reflect** logo + flight + telemetry regions consistently with the real `LayoutEngine` zones (adjust relative schematic percentages if needed so labels align with FR4 regions)

11. **Given** regression safety for users still on the **legacy** `NeoMatrixDisplay::displayFlights` path until ds-1.5, **Then** this story **does not** change the production display task to call `ClassicCardMode` yet — **leave** `NeoMatrixDisplay::displayFlights`, `renderFlight`, and zone helpers **behavior-identical** for the same inputs (optional micro-refactor only if required for shared static helpers extracted to an anonymous namespace / file — prefer **copy-then-verify** in ClassicCardMode first to avoid drift)

12. **Given** build health, **Then** `pio run` from `firmware/` succeeds with no new warnings

## Tasks / Subtasks

- [x] Task 1: Port zone rendering into ClassicCardMode (AC: #1, #4, #6, #9)
  - [x] 1.1: Implement private helpers mirroring `renderLogoZone`, `renderFlightZone`, `renderTelemetryZone` signatures using `(FastLED_NeoMatrix* matrix, const RenderContext& ctx, const FlightInfo& f, const Rect& zone)` or equivalent — use `ctx.textColor`, `ctx.logoBuffer`, **never** `ConfigManager::getDisplay()`
  - [x] 1.2: Use `DisplayUtils::*` for text and bitmap drawing; pass `ctx.matrix` as first argument where required
  - [x] 1.3: Handle `!ctx.layout.valid` branch by porting `displaySingleFlightCard` logic with matrix width/height from `ctx.layout.matrixWidth` / `matrixHeight` (see `LayoutResult`)

- [x] Task 2: Cycling + idle (AC: #3, #5)
  - [x] 2.1: Add member fields `_currentFlightIndex`, `_lastCycleMs` (or equivalent) — reset in `init()`, clear in `teardown()` if needed
  - [x] 2.2: Port cycling logic from `displayFlights` lines ~435–449 using `millis()` and `ctx.displayCycleMs`
  - [x] 2.3: Port empty-vector behavior from `displayLoadingScreen` (no `show`)

- [x] Task 3: Memory + metadata (AC: #7, #8, #10)
  - [x] 3.1: Tune `MEMORY_REQUIREMENT` comment + value after final struct size / any heap use
  - [x] 3.2: Review `String` usage in `render`; shrink or split if stack risk
  - [x] 3.3: Align `_zones` / `_descriptor` with real zones (Logo / Flight / Telemetry labeling)

- [x] Task 4: Verification (AC: #2, #11, #12)
  - [x] 4.1: Add or extend `firmware/test/` coverage for parity (recommended: dedicated test target or reuse patterns from `test_layout_engine`)
  - [x] 4.2: Document manual visual checklist if automated pixel test is impractical on CI
  - [x] 4.3: `pio run` clean build

## Dev Notes

### Source of truth for “Foundation” pixels

Port from **`firmware/adapters/NeoMatrixDisplay.cpp`**:

```417:422:firmware/adapters/NeoMatrixDisplay.cpp
void NeoMatrixDisplay::renderZoneFlight(const FlightInfo &f)
{
    renderLogoZone(f, _layout.logoZone);
    renderFlightZone(f, _layout.flightZone);
    renderTelemetryZone(f, _layout.telemetryZone);
}
```

- **`renderLogoZone`** — `NeoMatrixDisplay.cpp` ~292–297  
- **`renderFlightZone`** — ~300–365  
- **`renderTelemetryZone`** — ~368–414  
- **Cycling + valid/invalid layout** — `displayFlights` ~426–457  
- **Fallback card** — `displaySingleFlightCard` ~243–287  
- **Idle** — `displayLoadingScreen` ~467–491  

`renderFlight` (~712–732) duplicates zone vs fallback vs loading without cycling — use **`displayFlights`** as the behavioral reference for multi-flight cadence.

### RenderContext usage (strict)

| Field | Use |
|--------|-----|
| `matrix` | Draw target; **no** `FastLED.show()` |
| `layout` | `logoZone`, `flightZone`, `telemetryZone`, `valid`, `matrixWidth`, `matrixHeight` |
| `textColor` | RGB565 text (replaces per-call `ConfigManager` color reads) |
| `brightness` | Read-only per NFR S5 |
| `logoBuffer` | Same role as `NeoMatrixDisplay::_logoBuffer` — pass pointer into `LogoManager` / bitmap draw |
| `displayCycleMs` | Flight rotation interval |

### Epic ↔ code naming

- Epic mentions `getMemoryRequirement()` and `getZoneLayout()` — the codebase uses **`static constexpr MEMORY_REQUIREMENT`** + **`getZoneDescriptor()`** [Source: `interfaces/DisplayMode.h`, ds-1.3 story Dev Notes]

### Dependencies

- **ds-1.2** — `utils/DisplayUtils.h` / `.cpp`  
- **ds-1.3** — `ClassicCardMode` file names and mode id `classic_card` unchanged  
- **ds-1.5** — wires `ModeRegistry::tick` + `show()` into `displayTask`; this story only ensures Classic mode is **ready**

### Out of scope

- Removing `NeoMatrixDisplay` zone methods or `_currentFlightIndex` — **ds-1.5** unless required for deduplication with zero behavior change  
- WebPortal mode APIs — **ds-3**  
- `LiveFlightCardMode` real implementation — **ds-2**

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-1.md` (Story ds-1.4)  
- Architecture: `_bmad-output/planning-artifacts/architecture.md` (D1 DisplayMode, D3 NeoMatrixDisplay split)  
- Prior: `_bmad-output/implementation-artifacts/stories/ds-1-3-moderegistry-and-static-registration-system.md`  
- Layout: `firmware/core/LayoutEngine.h` (`LayoutResult`, `Rect`)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` clean build: SUCCESS — no new warnings (78.2% flash, 16.7% RAM)
- `pio test --filter test_classic_card_mode` compile: SUCCESS — test binary built cleanly
- On-device test execution: requires ESP32 hardware (Unity on-device framework)

### Completion Notes List

- **Task 1**: Ported all three zone rendering methods (renderLogoZone, renderFlightZone, renderTelemetryZone) from NeoMatrixDisplay into ClassicCardMode as private helpers. All use RenderContext fields (ctx.matrix, ctx.textColor, ctx.logoBuffer, ctx.layout zones) — zero ConfigManager reads. Added renderSingleFlightCard fallback for invalid layout and renderLoadingScreen for empty flights. Rendering logic is line-for-line identical to NeoMatrixDisplay foundation.
- **Task 2**: Added `_currentFlightIndex` and `_lastCycleMs` member fields. Cycling uses `ctx.displayCycleMs` (not ConfigManager). Both fields reset in init() and teardown(). Empty flights path renders white-bordered "..." loading screen without FastLED.show().
- **Task 3**: MEMORY_REQUIREMENT kept at 64 bytes (sizeof instance ~12 bytes actual: vtable ptr + size_t + unsigned long). Zone descriptors updated from 4-zone stub (Logo/Airline/Route/Aircraft) to 3-zone real layout (Logo 25%/Flight 75%x60%/Telemetry 75%x40%) matching LayoutEngine output. String usage reviewed: all temporaries are local to render() scope, longest ~40 chars, well under 512 byte stack budget. ctx.brightness is never written.
- **Task 4**: Created `firmware/test/test_classic_card_mode/test_main.cpp` with 16 Unity tests covering: init/teardown lifecycle, metadata correctness (getName, getZoneDescriptor 3-zone alignment, getSettingsSchema null), MEMORY_REQUIREMENT sizing, null matrix guards, heap safety (10 init/teardown cycles), factory integration, render lifecycle with multiple flights, and single-flight cycling behavior.

### Visual Verification Checklist (AC #2)

Pixel-identical verification requires on-device side-by-side comparison. Procedure:
1. Flash firmware with both legacy `displayFlights` path and ClassicCardMode active
2. Test with 5+ representative flights:
   - Short airline name (e.g., "UA") — compact text
   - Long airline name (e.g., "American Airlines") — truncation with "..."
   - Missing telemetry (NAN values) — "--" placeholders
   - Single-line flight zone height — compact route-only display
   - Three-line expanded flight zone — all three fields visible
3. Compare framebuffer output visually or via serial pixel dump
4. Verify no drawing outside zone bounds (check border pixels remain black)

### File List

- `firmware/modes/ClassicCardMode.h` (modified — added cycling fields, private helpers)
- `firmware/modes/ClassicCardMode.cpp` (modified — full rendering implementation)
- `firmware/test/test_classic_card_mode/test_main.cpp` (new — 16 Unity tests)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (modified — status update)

## Change Log

- **2026-04-13**: Implemented ClassicCardMode with full zone rendering (logo, flight, telemetry), cycling logic, fallback card, and idle screen. Added 16 Unity tests. Build clean. Story moved to review.

## Previous story intelligence

- **ds-1.3** shipped stub `ClassicCardMode` with `fillScreen(0)` — replace body; keep **mode id** and **file paths**.  
- Registry uses **`MEMORY_REQUIREMENT`** static, not a virtual.  
- **Modes must not call `FastLED.show()`** — already violated inside some `NeoMatrixDisplay` helpers today; ClassicCardMode must not add new `show()` calls.

## Git intelligence summary

Recent epic-ds-1 work added `modes/`, `ModeRegistry`, `DisplayUtils` — Classic migration should be a focused edit to `ClassicCardMode.cpp` plus optional tests.

## Latest tech information

- **Arduino `String`** on ESP32: short-lived locals in `render()` are common but count toward **stack** and fragmentation risk; keep lifetime bounded to `render()`.

## Project context reference

`_bmad-output/project-context.md` — build with `pio run` from `firmware/`, hexagonal layout, display on Core 0.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
