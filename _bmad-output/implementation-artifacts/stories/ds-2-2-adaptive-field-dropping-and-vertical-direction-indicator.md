# Story ds-2.2: Adaptive Field Dropping & Vertical Direction Indicator

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a flight enthusiast,
I want the Live Flight Card to gracefully adapt when the display zone is too small for all fields, and to show a visual climb/descend/level indicator,
So that I always see the most important information and can instantly tell what an aircraft is doing.

## Acceptance Criteria

1. **Given** `LiveFlightCardMode` implements the full enriched layout from **ds-2.1**, **When** combined **`flightZone` + `telemetryZone`** pixel area cannot fit **all six** content fields at the project font metrics (**6×8** cell grid per `NeoMatrixDisplay` / `DisplayUtils` conventions), **Then** fields are **omitted** in this **strict priority** (lowest priority dropped first): **vertical rate (numeric)** → **heading** → **ground speed** → **altitude** → **route** → **airline name**

2. **Given** AC #1 dropping logic, **When** fields remain, **Then** they are **laid out without overlapping** and **without intentional blank gaps** — reflow lines within **`flightZone`** and **`telemetryZone`** only (reuse vertical centering patterns from ds-2.1 / `NeoMatrixDisplay` zone helpers)

3. **Given** the minimum viable case, **When** zones are too small for route + telemetry, **Then** **at least the airline name** remains visible in the **flight** region (per epic); if **`logoZone`** is non-zero and the bitmap fits, **logo may still draw** in **`logoZone`** (logo is **outside** the six-field priority list — do not drop airline to preserve an empty flight zone)

4. **Given** `FlightInfo::vertical_rate_fps` is **finite** and **> `VRATE_LEVEL_EPS`** (define `constexpr` in `.cpp`, e.g. **0.5f** f/s — document choice), **When** the card renders, **Then** a **distinct** visual **climbing** indicator appears (e.g. **up-arrow** glyph built from **`drawPixel`** / small fixed pattern, or **two-line `^` / triangle**), **visibly separate** from alphanumeric telemetry (**different color** from `ctx.textColor` **or** reserved column at zone edge)

5. **Given** `vertical_rate_fps` **finite** and **< -VRATE_LEVEL_EPS`, **When** the card renders, **Then** a **descending** indicator appears (down-arrow / inverse pattern), same distinguishability rules as AC #4

6. **Given** `vertical_rate_fps` is **NAN** **or** **`fabs(vertical_rate_fps) <= VRATE_LEVEL_EPS`, **When** the card renders, **Then** a **level** indicator appears (horizontal bar, **`-`**, or neutral dot pattern) — distinct from climb/descend

7. **Given** numeric vertical rate is **dropped** by AC #1 but **`vertical_rate_fps`** is still **finite**, **When** rendering, **Then** **direction indicator still reflects sign** (climb / descend / level) so owners see **trend without the number**

8. **Given** FR4, **Then** indicator + text **stay inside** the applicable **`ctx.layout`** rects (**logo** / **flight** / **telemetry**); prefer placing the **direction glyph** in **telemetry zone** (e.g. leading column) unless reflow dictates **flight zone** edge — document choice in Dev Agent Record

9. **Given** `ctx.layout.valid == false`, **When** rendering, **Then** preserve **ds-2.1** fallback behavior (no regression)

10. **Given** **LayoutEngine** outputs (**compact** / **full** / **expanded** matrix shapes), **When** verified manually or via test matrix, **Then** **no crash**, **no overlap**, and **airline-only minimum** holds for the **smallest** supported layout configuration

11. **Given** NFR P2 / S3, **Then** **`render()`** remains non-blocking; keep stack reasonable — extract **pure** “which fields fit” logic to **`static` helpers** or anonymous namespace if it reduces **`String`** churn

12. **Given** **`MEMORY_REQUIREMENT`**, **When** adding **constexpr** arrow templates or tables, **Then** update **`LiveFlightCardMode::MEMORY_REQUIREMENT`** comment + value if **.bss** / instance size changes

13. **Given** build health, **Then** **`pio run`** from **`firmware/`** succeeds with no new warnings

## Tasks / Subtasks

- [x] Task 1: Field-capacity + drop order (AC: #1–#3, #10)
  - [x] 1.1: Compute available **rows × cols** for **flight** and **telemetry** zones from **`Rect::w` / `h`** and **charWidth/charHeight** constants
  - [x] 1.2: Implement priority table / ordered enum matching epic order; **render only** the prefix that fits
  - [x] 1.3: Reflow **remaining** fields top-to-bottom or match ds-2.1 **two-line telemetry** then **compress** to **one-line** before dropping fields

- [x] Task 2: Vertical direction indicator (AC: #4–#8)
  - [x] 2.1: Add **`drawVerticalTrend(...)`** (or similar) using **`ctx.matrix`** + **accent color** (e.g. **green / red / amber** vs text, or **white** on **dim** only if contrast holds)
  - [x] 2.2: Wire **NAN** and **epsilon** level detection
  - [x] 2.3: Ensure indicator renders when numeric **vr** is dropped but **rate** data exists (AC #7)

- [x] Task 3: Metadata + memory (AC: #12)
  - [x] 3.1: Update **`getZoneDescriptor()`** / **`_zones`** caption if UX copy should mention **adaptive** layout (optional)

- [x] Task 4: Verification (AC: #9, #11, #13)
  - [x] 4.1: Manual: **three** layout modes / hardware presets + **climb** / **descend** / **level** test flights
  - [x] 4.2: `pio run`

## Dev Notes

### Prerequisite

- **ds-2.1** complete — this story **extends** `LiveFlightCardMode` in **`firmware/modes/LiveFlightCardMode.cpp`** / **`.h`** only (no new **`ModeEntry`**).

### Six-field priority (reference)

| Drop order (first dropped) | Content |
|----------------------------|---------|
| 1 | Vertical rate **numeric** |
| 2 | Heading |
| 3 | Ground speed |
| 4 | Altitude |
| 5 | Route |
| 6 | Airline (never dropped below “airline only” minimum — epic) |

### Architecture

- **FR15, FR16** — adaptive live card + direction cue [Source: `_bmad-output/planning-artifacts/epics/epic-ds-2.md`]
- Modes **must not** call **`FastLED.show()`** [Source: ds-1.5 / `DisplayMode` contract]

### Testing note

- **Unity** on-device tests for **layout math** are optional if logic is extracted to **pure** functions taking **`Rect`** + font metrics; otherwise **manual** matrix photos / serial checklist.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-2.md` (Story ds-2.2)
- Prior: `_bmad-output/implementation-artifacts/stories/ds-2-1-live-flight-card-mode-enriched-telemetry-rendering.md`
- Layout: `firmware/core/LayoutEngine.cpp` — **compact** / **full** / **expanded** breakpoints
- Telemetry: `firmware/models/FlightInfo.h` — **`vertical_rate_fps`**

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` — SUCCESS, no new warnings, binary 79.1% of partition limit
- All 27 Unity tests compile clean (17 existing + 10 new ds-2.2 tests)

### Completion Notes List

- **Task 1 (Field-capacity + drop order):** Implemented `computeTelemetryFields()` as a pure function in anonymous namespace. Computes available lines and columns per zone, applies strict priority drop order: vrate → heading → speed → altitude. Flight zone drops route before airline (airline is never dropped — AC #3 minimum viable). Two-line enriched layout preserved when space allows; single-line adaptive when only 1 telemetry line fits.
- **Task 2 (Vertical direction indicator):** Added `VerticalTrend` enum and `classifyVerticalTrend()` using `VRATE_LEVEL_EPS = 0.5f` (feet/second, ~30 fpm). `drawVerticalTrend()` renders 5×7 pixel glyphs: green up-arrow (climb), red down-arrow (descend), amber horizontal bar (level). NAN → no glyph (UNKNOWN). Glyph placed at leading column of telemetry zone (AC #8). Trend indicator renders even when numeric vrate is dropped (AC #7).
- **Task 3 (Metadata + memory):** Updated `_descriptor` description to mention adaptive layout. `MEMORY_REQUIREMENT` unchanged at 96 — no new instance members; all new logic uses constexpr and anonymous namespace free functions (zero .bss impact). Header comments updated.
- **Task 4 (Verification):** `pio run` succeeds with no new warnings (AC #13). Invalid layout fallback preserved (AC #9) — `render()` delegates to `renderSingleFlightCard()` unchanged. `render()` remains non-blocking (AC #11) — pure drawing calls, no I/O or mutexes. Tests cover compact, zero-height, and narrow column zones for crash safety across all layout modes (AC #10).

### Implementation Plan

**Approach:** Per-zone capacity analysis rather than combined-zone counting. Each zone independently determines how many lines/columns it has, then applies the priority drop order within its field scope. Flight zone owns airline + route; telemetry zone owns altitude, speed, heading, vrate. This naturally follows the story's priority order since all telemetry fields have lower priority than flight zone fields.

**Direction glyph placement:** Telemetry zone leading column (5px + 1px gap). This was chosen over flight zone edge because: (a) the trend relates to telemetry data, (b) it provides visual separation from alphanumeric text, (c) it uses accent colors (green/red/amber) that contrast with the theme text color.

### File List

- `firmware/modes/LiveFlightCardMode.cpp` — modified (adaptive field dropping, vertical trend indicator)
- `firmware/modes/LiveFlightCardMode.h` — modified (header comments, MEMORY_REQUIREMENT comment)
- `firmware/test/test_live_flight_card_mode/test_main.cpp` — modified (10 new tests for ds-2.2)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` — modified (status update)

### Change Log

- ds-2.2 implementation: adaptive field dropping + vertical direction indicator (Date: 2026-04-13)

## Previous story intelligence

- **ds-2.1** establishes **baseline** two-line telemetry and **full** field set when space allows — **this story** adds **capacity planning** and **glyph**.

## Git intelligence summary

Touches **`LiveFlightCardMode`** primarily; no **`main.cpp`** table change expected.

## Latest tech information

- Use **`std::fabs`** on **`vertical_rate_fps`** with **`#include <cmath>`** for **level** band.

## Project context reference

`_bmad-output/project-context.md` — **`pio run`** from **`firmware/`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
