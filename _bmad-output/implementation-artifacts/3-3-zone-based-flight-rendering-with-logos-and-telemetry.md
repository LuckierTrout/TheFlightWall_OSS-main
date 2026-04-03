# Story 3.3: Zone-Based Flight Rendering with Logos & Telemetry

Status: ready-for-dev

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

- [ ] Task 1: Extend data model for display-ready telemetry (AC: #4, #5, #6)
  - [ ] Add telemetry fields to `firmware/models/FlightInfo.h` (nullable-friendly). Suggested fields:
    - [ ] `double altitude_kft = NAN;`
    - [ ] `double speed_mph = NAN;`
    - [ ] `double track_deg = NAN;`
    - [ ] `double vertical_rate_fps = NAN;`
  - [ ] Keep names aligned with epic acceptance wording.

- [ ] Task 2: Compute telemetry in pipeline, not renderer (AC: #5, #6)
  - [ ] Update `FlightDataFetcher::fetchFlights()` to join state-vector telemetry with enriched `FlightInfo`.
  - [ ] Map `StateVector` -> display units:
    - [ ] altitude: meters -> kft (`m * 3.28084 / 1000`)
    - [ ] speed: m/s -> mph (`mps * 2.23694`)
    - [ ] track: use heading degrees as-is (normalized 0-359 if needed)
    - [ ] vertical rate: m/s -> ft/s (`mps * 3.28084`)
  - [ ] If source values are NaN/null, leave destination fields as NaN so renderer can emit `--`.
  - [ ] Avoid O(n^2) scans where possible; if needed, build a small lookup by callsign/icao24 for state vectors.

- [ ] Task 3: Integrate zone rendering in `NeoMatrixDisplay` (AC: #1, #2, #3, #4, #7, #8, #9)
  - [ ] Pull current `HardwareConfig` and compute layout via `LayoutEngine::compute(hw)`.
  - [ ] Replace monolithic `displaySingleFlightCard()` path with zone-aware helpers:
    - [ ] `renderLogoZone(...)`
    - [ ] `renderFlightZone(...)`
    - [ ] `renderTelemetryZone(...)`
  - [ ] Keep existing cycle logic in `displayFlights()` and avoid extra `FastLED.show()` calls per sub-zone to reduce flicker.
  - [ ] Preserve no-flight fallback (`displayLoadingScreen()`).

- [ ] Task 4: Connect LogoManager to renderer (AC: #2)
  - [ ] Initialize/use `LogoManager` from a stable owner (display adapter or setup owner) without per-frame expensive setup.
  - [ ] For each rendered flight, resolve logo key from `operator_icao` and load into a reusable 32x32 RGB565 buffer.
  - [ ] Render fallback sprite when `loadLogo()` returns false.
  - [ ] Keep memory footprint predictable; avoid heap churn every frame.

- [ ] Task 5: Compact vs expanded behavior (AC: #8)
  - [ ] Compact mode: prioritize route + minimal telemetry formatting (abbreviated labels).
  - [ ] Expanded mode: allow fuller labels/spacing where zone size permits.
  - [ ] Full mode (32px height) should remain legible with balanced text density.

- [ ] Task 6: Validation and regression checks (AC: #6, #7, #9)
  - [ ] Unit test telemetry conversion helper(s) (new test file or existing pipeline tests).
  - [ ] Manual runtime checks:
    - [ ] null telemetry shows `--`
    - [ ] cycle remains smooth
    - [ ] no flights shows loading screen
  - [ ] `pio run` (and any relevant `pio test` targets).

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

—

### Debug Log References

### Completion Notes List

### File List
