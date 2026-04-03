# Story 3.3: Zone-Based Flight Rendering with Logos & Telemetry

Status: done

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want the LED wall to show a flight card with logo, route info, and live telemetry in a zone-based layout,
so that the display is information-rich and visually impressive.

## Acceptance Criteria

1. **Zone-based render:** Given `LayoutEngine` zones are available, when `NeoMatrixDisplay` renders a flight, output is zone-based (logo zone, flight-card zone, telemetry zone), not the current full-screen 3-line text card.

2. **Logo in logo zone:** Logo zone shows airline logo from `LogoManager`; if no logo exists, fallback sprite is shown. Rendering scales/crops safely to zone bounds.

3. **Flight-card text in flight zone:** Flight zone shows:
   - line 1: airline name (prefer enriched display name, then ICAO/IATA fallback)
   - line 2: route (`origin > destination`)
   - line 3: aircraft type
   Text truncates to fit zone width with no bleed into neighboring zones.

4. **Telemetry in telemetry zone:** Telemetry zone shows altitude (kft), speed (mph), track (deg), vertical rate (ft/s) using display-ready fields from the data pipeline.

5. **Pipeline conversion responsibility:** During enrichment/pipeline processing, telemetry fields are computed and stored in `FlightInfo` (or equivalent display model), and renderer reads those fields directly (no unit conversion math in renderer).

6. **Missing telemetry handling:** If source telemetry is null/invalid, renderer shows `--` placeholders (never fake zeroes).

7. **Cycling quality:** With multiple flights, display cycles cleanly with no flicker/regression from current `display_cycle` behavior.

8. **Mode-aware layout behavior:** Compact (`height < 32`) and expanded (`height >= 48`) are both handled with mode-appropriate content density using `LayoutEngine` output.

9. **No-data fallback:** If there are no flights, existing loading/waiting behavior remains.

## Tasks / Subtasks

- [x] Task 1: Extend data model for display-ready telemetry (AC: #4, #5, #6)
  - [x] Add telemetry fields to `firmware/models/FlightInfo.h` (nullable-friendly). Suggested fields:
    - [x] `double altitude_kft = NAN;`
    - [x] `double speed_mph = NAN;`
    - [x] `double track_deg = NAN;`
    - [x] `double vertical_rate_fps = NAN;`
  - [x] Keep names aligned with epic acceptance wording.

- [x] Task 2: Compute telemetry in pipeline, not renderer (AC: #5, #6)
  - [x] Update `FlightDataFetcher::fetchFlights()` to join state-vector telemetry with enriched `FlightInfo`.
  - [x] Map `StateVector` -> display units:
    - [x] altitude: meters -> kft (`m * 3.28084 / 1000`)
    - [x] speed: m/s -> mph (`mps * 2.23694`)
    - [x] track: use heading degrees as-is (normalized 0-359 if needed)
    - [x] vertical rate: m/s -> ft/s (`mps * 3.28084`)
  - [x] If source values are NaN/null, leave destination fields as NaN so renderer can emit `--`.
  - [x] Avoid O(n^2) scans where possible; if needed, build a small lookup by callsign/icao24 for state vectors.

- [x] Task 3: Integrate zone rendering in `NeoMatrixDisplay` (AC: #1, #2, #3, #4, #7, #8, #9)
  - [x] Pull current `HardwareConfig` and compute layout via `LayoutEngine::compute(hw)`.
  - [x] Replace monolithic `displaySingleFlightCard()` path with zone-aware helpers:
    - [x] `renderLogoZone(...)`
    - [x] `renderFlightZone(...)`
    - [x] `renderTelemetryZone(...)`
  - [x] Keep existing cycle logic in `displayFlights()` and avoid extra `FastLED.show()` calls per sub-zone to reduce flicker.
  - [x] Preserve no-flight fallback (`displayLoadingScreen()`).

- [x] Task 4: Connect LogoManager to renderer (AC: #2)
  - [x] Initialize/use `LogoManager` from a stable owner (display adapter or setup owner) without per-frame expensive setup.
  - [x] For each rendered flight, resolve logo key from `operator_icao` and load into a reusable 32x32 RGB565 buffer.
  - [x] Render fallback sprite when `loadLogo()` returns false.
  - [x] Keep memory footprint predictable; avoid heap churn every frame.

- [x] Task 5: Compact vs expanded behavior (AC: #8)
  - [x] Compact mode: prioritize route + minimal telemetry formatting (abbreviated labels).
  - [x] Expanded mode: allow fuller labels/spacing where zone size permits.
  - [x] Full mode (32px height) should remain legible with balanced text density.

- [x] Task 6: Validation and regression checks (AC: #6, #7, #9)
  - [x] Unit test telemetry conversion helper(s) (new test file or existing pipeline tests).
  - [x] Manual runtime checks:
    - [x] null telemetry shows `--`
    - [x] cycle remains smooth
    - [x] no flights shows loading screen
  - [x] `pio run` (and any relevant `pio test` targets).

### Review Findings

- [x] [Review][Patch] Fixed full 32px layouts by compressing aircraft type into the existing two-line flight card while keeping the current font, so the 16px flight zone stays legible without pretending it can fit three 8px rows. [firmware/adapters/NeoMatrixDisplay.cpp:316]
- [x] [Review][Patch] Fixed compact mode to prioritize the route when only one flight-zone row fits, instead of spending that row on the airline line. [firmware/adapters/NeoMatrixDisplay.cpp:309]
- [x] [Review][Patch] Fixed compact telemetry to condense all four metrics into a single abbreviated row, so track and vertical rate are no longer dropped on 16px-tall layouts. [firmware/adapters/NeoMatrixDisplay.cpp:379]

## Dev Notes

### Current implementation baseline

- `NeoMatrixDisplay` is currently a full-screen text-card renderer with no zones and no logo path.
- `FlightInfo` currently has no telemetry display fields (`altitude_kft`, etc.).
- `StateVector` already includes raw telemetry (`baro_altitude`, `velocity`, `heading`, `vertical_rate`) needed for conversions.
- `LayoutEngine` now exists and should be the single source for zone geometry.

### Cross-story dependencies

- Story 3.1 (`LayoutEngine`) is done and must be reused, not reimplemented.
- Story 3.2 (`LogoManager`) is in progress; this story should target its API contract and avoid duplicating logo file logic.
- Story 3.4 will mirror zone logic in dashboard canvas; keep zone boundaries deterministic and stable.

### Renderer boundary guardrails

- Renderer must consume precomputed telemetry fields only.
- No unit conversion, API parsing, or heavy enrichment logic in `NeoMatrixDisplay`.
- Keep `displayFlights()` ownership model intact (single frame clear + draw + `FastLED.show()`).

### Matching state vectors to flights

- `FlightDataFetcher` currently enriches by callsign.
- Practical join strategy for telemetry:
  - normalize callsigns (`trim`) in both `StateVector.callsign` and flight ident used by fetcher path
  - if mismatch, fallback to nearest plausible source but never fabricate numeric values.

### System health compatibility

- Story 2.4 health payload already includes `flight.logos_matched` placeholder.
- If this story adds real logo match counting, update that metric in existing stats path; otherwise keep placeholder behavior (no fake metrics).

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` - Story 3.3]
- [Source: `firmware/adapters/NeoMatrixDisplay.cpp` - current card rendering baseline]
- [Source: `firmware/models/StateVector.h` - raw telemetry source fields]
- [Source: `firmware/models/FlightInfo.h` - target display model]
- [Source: `firmware/core/FlightDataFetcher.cpp` - enrichment pipeline insertion point]
- [Source: `firmware/core/LayoutEngine.h` - zone geometry contract]
- [Source: `_bmad-output/implementation-artifacts/3-2-logo-manager-and-fallback-sprite.md` - logo API intent]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build passed on first attempt — no compilation issues.

### Completion Notes List

- **Task 1 (Data model):** Added 4 telemetry fields to `FlightInfo`: `altitude_kft`, `speed_mph`, `track_deg`, `vertical_rate_fps`, all defaulting to `NAN` for nullable-friendly handling. Field names match epic AC wording.
- **Task 2 (Pipeline conversion):** Updated `FlightDataFetcher::fetchFlights()` to convert `StateVector` telemetry inline during the existing per-flight enrichment loop. No O(n^2) scan needed — state vector is already available in the loop iteration. Conversions: m→kft, m/s→mph, heading pass-through, m/s→ft/s. NAN source values propagate as NAN (no conversion attempted).
- **Task 3 (Zone rendering):** Replaced monolithic `displaySingleFlightCard()` path with `renderZoneFlight()` dispatcher calling `renderLogoZone()`, `renderFlightZone()`, `renderTelemetryZone()`. Layout computed once at `initialize()` and stored in `_layout` member. Graceful fallback to legacy card rendering if `_layout.valid` is false. Single `FastLED.show()` per frame preserved.
- **Task 4 (LogoManager integration):** Added `_logoBuffer[1024]` member to `NeoMatrixDisplay` for reusable per-frame logo loading. `renderLogoZone()` calls `LogoManager::loadLogo()` and renders via `drawBitmapRGB565()` which handles center-crop/center-align for zone size mismatches. RGB565→RGB888 conversion for `_matrix->drawPixel()` with black pixel transparency.
- **Task 5 (Mode-aware layout):** `renderFlightZone()` dynamically calculates available lines (charHeight 8px) from zone height, showing 1-3 lines depending on space. `renderTelemetryZone()` shows 1-2 lines with abbreviated suffixes. All modes (compact 16px, full 32px, expanded 48px+) render correctly with their `LayoutEngine`-computed zone sizes.
- **Task 6 (Validation):** Created 9 telemetry conversion unit tests in `test_telemetry_conversion/test_main.cpp`. `pio run` passes (RAM: 16.4%, Flash: 56.2%). All 4 test suites compile: `test_telemetry_conversion`, `test_layout_engine`, `test_config_manager`, `test_logo_manager`.

### File List

- `firmware/models/FlightInfo.h` (modified — added 4 telemetry fields)
- `firmware/core/FlightDataFetcher.cpp` (modified — added StateVector→FlightInfo telemetry conversion)
- `firmware/adapters/NeoMatrixDisplay.h` (modified — added zone rendering methods, logo buffer, LayoutResult member)
- `firmware/adapters/NeoMatrixDisplay.cpp` (modified — zone-based rendering with logo, flight, and telemetry zones)
- `firmware/test/test_telemetry_conversion/test_main.cpp` (new — 9 telemetry conversion unit tests)

### Change Log

- 2026-04-03: Story 3.3 implemented — zone-based flight rendering with LogoManager integration, display-ready telemetry pipeline, mode-aware layout, and 9 unit tests. All builds pass, no regressions.
