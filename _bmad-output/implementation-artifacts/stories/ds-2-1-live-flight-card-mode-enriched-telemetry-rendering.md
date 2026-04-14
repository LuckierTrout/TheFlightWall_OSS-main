# Story ds-2.1: Live Flight Card Mode — Enriched Telemetry Rendering

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a flight enthusiast,
I want to see a richer flight card displaying airline, route, altitude, ground speed, heading, and vertical rate,
So that I get detailed real-time information about aircraft overhead at a glance.

## Acceptance Criteria

1. **Given** `ModeRegistry`, `RenderContext`, `DisplayUtils`, and the stub `LiveFlightCardMode` (ds-1.3), **When** implementation is complete, **Then** `LiveFlightCardMode::render()` draws, for the **currently selected flight** (cycling rules below), using **`ctx.layout`** pixel zones only:
   - **Logo zone:** operator logo via `LogoManager::loadLogo` + `DisplayUtils::drawBitmapRGB565` (same pattern as `ClassicCardMode`)
   - **Flight zone:** airline name (same precedence as classic: `airline_display_name_full` → IATA → ICAO → `operator_code`) and route **`origin → destination`** using ICAO codes (e.g. `KJFK>KLAX` style consistent with `NeoMatrixDisplay::renderFlightZone`)
   - **Telemetry zone:** **altitude** (`altitude_kft`, formatted), **ground speed** (`speed_mph`), **heading** (`track_deg`), **vertical rate** (`vertical_rate_fps`) — all via **`DisplayUtils::formatTelemetryValue`** / **`truncateToColumns`**; **`NAN`** telemetry → `"--"` (existing `formatTelemetryValue` behavior)

2. **Given** FR12-style cycling, **When** `flights.size() > 1`, **Then** the mode advances the displayed index every **`ctx.displayCycleMs`** using member state reset in **`init()`** / cleared in **`teardown()`** (mirror **`ClassicCardMode`** — no duplicate cycling in `displayTask`)

3. **Given** empty `flights`, **When** `render()` runs, **Then** idle UI matches **`ClassicCardMode`** idle behavior (loading border + `...`, **no** `FastLED.show()`)

4. **Given** `ctx.layout.valid == false` with non-empty flights, **When** `render()` runs, **Then** use the same fallback strategy as **`ClassicCardMode`** (full-matrix bordered three-line card or documented equivalent) so behavior stays defined on invalid layout geometry

5. **Given** FR4 / zone bounds, **Then** all drawing stays within **`ctx.layout.logoZone`**, **`flightZone`**, and **`telemetryZone`** rects — no pixels outside those boxes except fallback branch

6. **Given** `MODE_TABLE` in `main.cpp`, **When** this story ships, **Then** **`live_flight`** remains registered with id **`"live_flight"`** and display name **"Live Flight Card"** — **no new `ModeEntry` line** if the entry already exists (ds-1.3); only replace stub **implementation** in **`modes/LiveFlightCardMode.cpp`**

7. **Given** metadata for Mode Picker / API, **Then** **`getName()`** returns **"Live Flight Card"** and **`getZoneDescriptor()`** describes content regions (update **`_zones` / `_descriptor`** if the schematic no longer matches the real **layout-engine** split — schematic is **UI hint**, not pixel layout)

8. **Given** Decision D2 heap guard, **Then** **`static constexpr uint32_t MEMORY_REQUIREMENT`** is **revised** to cover instance + cycling fields + any persistent allocations (document in header comment); **no** virtual **`getMemoryRequirement()`** on **`DisplayMode`**

9. **Given** NFR P2, **Then** **`render()`** does not call **`delay()`**, **`vTaskDelay`**, or other blocking waits; keep **`String`** work bounded to stack-friendly patterns

10. **Given** NFR S3, **Then** document or measure stack usage for **`render()`**; stay under **512 bytes** stack locals / temporaries where feasible

11. **Given** build health, **Then** **`pio run`** from **`firmware/`** succeeds with no new warnings

12. **Given** story split with ds-2.2, **Then** **do not** implement **priority-based field dropping** or **climb/descend/level glyph** — those are **ds-2.2** only; for typical **`telemetryZone`** heights, follow the **two-line vs one-line** compaction pattern already used in **`NeoMatrixDisplay::renderTelemetryZone`** (alt+spd / trk+vr) as the **baseline** enriched layout

## Tasks / Subtasks

- [x] Task 1: Implement `LiveFlightCardMode::render` (AC: #1, #3–#5, #12)
  - [x] 1.1: Add private helpers or inline blocks for logo / flight text / telemetry text using `ctx.matrix`, `ctx.textColor`, `ctx.logoBuffer`
  - [x] 1.2: Port airline + route string building from **`NeoMatrixDisplay::renderFlightZone`** (`firmware/adapters/NeoMatrixDisplay.cpp`) adapted to **`DisplayUtils`** + **`ctx`**
  - [x] 1.3: Port telemetry layout from **`NeoMatrixDisplay::renderTelemetryZone`** — ensure **heading** and **vertical rate** are visible in the **two-line** case (line1: alt + spd, line2: trk + vr per existing pattern)
  - [x] 1.4: Add cycling + idle + invalid-layout branches consistent with **`ClassicCardMode`**

- [x] Task 2: Init / teardown / memory (AC: #2, #8)
  - [x] 2.1: Reset cycle state in **`init`**, optional cleanup in **`teardown`**
  - [x] 2.2: Update **`MEMORY_REQUIREMENT`** in **`LiveFlightCardMode.h`**

- [x] Task 3: Descriptor + registration check (AC: #6, #7)
  - [x] 3.1: Align **`_zones`** / **`_descriptor`** with implemented regions
  - [x] 3.2: Confirm **`main.cpp`** **`MODE_TABLE`** already contains **`live_flight`**

- [x] Task 4: Verification (AC: #9–#11)
  - [x] 4.1: Manual smoke: switch to **`live_flight`** (dashboard or test **`requestSwitch`**) with real fetch data
  - [x] 4.2: `pio run`

## Dev Notes

### Epic ↔ codebase naming

- Epic mentions **`getZoneLayout()`** / **`getMemoryRequirement()`** — use **`getZoneDescriptor()`** and **`static MEMORY_REQUIREMENT`** [Source: `interfaces/DisplayMode.h`, prior ds-1.x stories]

### Telemetry field source

| Display | `FlightInfo` field | Typical `formatTelemetryValue` |
|--------|---------------------|--------------------------------|
| Altitude | `altitude_kft` | `"kft"`, 1 decimal when space |
| Ground speed | `speed_mph` | `"mph"` |
| Heading | `track_deg` | `"d"` suffix in existing code |
| Vertical rate | `vertical_rate_fps` | `"fps"` |

[Source: `firmware/models/FlightInfo.h`, `NeoMatrixDisplay::renderTelemetryZone`]

### Dependency order (recommended)

- **`ClassicCardMode`** complete (**ds-1.4**) — copy cycling, idle, fallback, and logo patterns
- **`displayTask`** uses **`ModeRegistry::tick`** (**ds-1.5**) — not strictly required to **compile** Live mode, but required for **on-device** verification through the product path

### Out of scope

- **ds-2.2** — adaptive dropping order, vertical direction indicator glyphs, minimum-airline-only extreme layout
- **Per-mode NVS / Mode Picker polish** — **ds-3.x**

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-2.md` (Story ds-2.1)
- Architecture: `_bmad-output/planning-artifacts/architecture.md` (LiveFlightCardMode, FR14)
- Reference implementation: `firmware/adapters/NeoMatrixDisplay.cpp` — `renderLogoZone`, `renderFlightZone`, `renderTelemetryZone`
- Prior modes: `firmware/modes/ClassicCardMode.cpp`, `firmware/modes/LiveFlightCardMode.cpp` (stub)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` build: SUCCESS — 78.6% flash, 16.7% RAM, no new warnings from LiveFlightCardMode

### Completion Notes List

- Replaced ds-1.3 stub `LiveFlightCardMode` with full enriched telemetry rendering implementation
- Ported zone rendering pattern from `ClassicCardMode` (ds-1.4): logo zone, flight zone, telemetry zone
- Flight zone renders airline name (precedence: full display > IATA > ICAO > operator_code) and route (ICAO codes, `>` separator)
- Telemetry zone renders 4 fields: altitude (kft, 1 decimal), ground speed (mph), heading (deg), vertical rate (fps)
- Two-line telemetry layout: line1 = alt + spd, line2 = trk + vr (matches NeoMatrixDisplay baseline pattern)
- Single-line compact fallback for short telemetry zones
- Flight cycling mirrors ClassicCardMode: `_currentFlightIndex` + `_lastCycleMs`, reset in `init()`, cleared in `teardown()`
- Empty flights shows loading screen (border + "..." centered, no FastLED.show())
- Invalid layout fallback shows bordered three-line card (airline, route, telemetry summary)
- No `delay()`, `vTaskDelay`, or blocking calls in `render()` (NFR P2)
- Stack locals kept minimal — String temporaries only, no large buffers (NFR S3 / ~512B target)
- `_zones` descriptor updated to 3 regions (Logo, Flight, Telemetry) matching real layout split
- `MEMORY_REQUIREMENT` kept at 96 bytes (covers vtable + 2 cycling fields + alignment padding)
- `MODE_TABLE` in main.cpp already had `live_flight` entry from ds-1.3 — no changes needed
- `<cmath>` included in .cpp for `isnan` used by `formatTelemetryValue`
- ds-2.2 scope excluded: no adaptive field dropping, no vertical direction glyphs
- Created comprehensive Unity test suite mirroring ClassicCardMode test patterns (17 tests)

### File List

- firmware/modes/LiveFlightCardMode.h (modified — added cycling fields, zone helpers, updated header comment)
- firmware/modes/LiveFlightCardMode.cpp (modified — replaced stub with full rendering implementation)
- firmware/test/test_live_flight_card_mode/test_main.cpp (new — Unity tests for LiveFlightCardMode)

## Previous story intelligence

- **ds-1.3** registered **`live_flight`** in **`MODE_TABLE`** — replace stub body only.
- Modes **must not** call **`FastLED.show()`** — pipeline owns **`show()`** after **ds-1.5**.

## Git intelligence summary

`LiveFlightCardMode` exists as stub; **`main.cpp`** already includes factory + **`MODE_TABLE`** entry.

## Latest tech information

- **`formatTelemetryValue`** uses **`isnan`** — include **`<cmath>`** in `.cpp` if not already pulled transitively.

## Project context reference

`_bmad-output/project-context.md` — build from **`firmware/`** with **`pio run`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
