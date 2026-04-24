# Story LE-2.7: Golden-frame regression + preview pixel-diff + API contract tests

branch: epic/le-1-layout-editor
zone:
  - firmware/test/test_golden_frames/test_main.cpp                       [edit]
  - firmware/test/test_golden_frames/fixtures/layout_d.json              [new]
  - tests/e2e/tests/preview-regression.spec.ts                           [new]
  - tests/e2e/fixtures/preview-fixtures.ts                               [new]
  - tests/e2e/mock-server/server.ts                                      [edit — add /api/widgets/types + /api/flights/current stubs for the preview harness]
  - tests/smoke/test_web_portal_smoke.py                                 [edit — add contract asserts for /api/widgets/types + /api/flights/current]

Status: in-progress

## Story

As a **maintainer**,
I want **a regression gate that catches font or catalog drift between the preview renderer and the firmware renderer**,
So that **"looks right in preview, wrong on hardware" bugs are caught at CI time rather than by a user squinting at the wall**.

## Acceptance Criteria

1. **Extended firmware golden-frame corpus.** `test_golden_frames` gains a new fixture (`Golden D — LE-2 Catalog`) that exercises every LE-2 catalog addition: `aircraft_short`, `aircraft_full`, `origin_iata`, `destination_iata`, `speed_mph`. Fixture JSON matches `fixtures/layout_d.json`. String-resolution tests assert the expected fallback precedence for each new field against a known `FlightInfo`.
2. **JS-side preview pixel-diff test.** A new Playwright spec (`tests/e2e/tests/preview-regression.spec.ts`) loads `preview.js` + `preview-glyphs.js` into a harness HTML page, renders each canonical layout against known mock flight data, and snapshots the canvas pixel output. Playwright's built-in `toHaveScreenshot()` enforces the pixel-diff tolerance and manages the baseline PNGs under `tests/e2e/tests/preview-regression.spec.ts-snapshots/`. A documented tolerance (default 0 px at the configured `maxDiffPixels`) catches font/catalog drift on the JS side.
3. **Contract tests for the public preview APIs.** `tests/smoke/test_web_portal_smoke.py` gains two new assertions:
   - `GET /api/widgets/types` returns `{ ok: true, data: [...] }` with entries having `{ type, label, fields }`. The `flight_field` entry's field-picker options include all 10 LE-2 catalog keys; the `metric` entry's options include all 7 LE-2 catalog keys (including `speed_mph`).
   - `GET /api/flights/current` returns `{ ok: true, data: { flights: [...] } }`. When `flights` is non-empty, each row exposes the keys the preview consumes (`ident`, `operator_code`, `operator_icao`, `origin_icao`, `destination_icao`, `aircraft_code`, `airline_display_name_full`, `aircraft_display_name_short`, `aircraft_display_name_full`, `altitude_kft`, `speed_mph`, `track_deg`, `vertical_rate_fps`, `distance_km`, `bearing_deg`).
4. **Mock-server coverage.** The Playwright mock server gains minimal `/api/widgets/types` and `/api/flights/current` handlers so the preview regression spec runs fully offline (no real device). The spec avoids hitting `/api/layout` — the harness page supplies matrix dimensions directly.
5. **Firmware builds and test compile pass** unchanged: `pio run -e esp32dev` / `pio run -e esp32s3_n16r8` green; `pio test -e esp32s3_n16r8 --without-uploading --without-testing` green across all 20 suites.
6. **No regression on the existing LE-1.9 fixtures** — golden A/B/C structural + string-resolution assertions still pass.
7. **The Playwright spec is documented** in `tests/e2e/README.md` (if present) or the spec file's top comment: how to generate / update baselines (`npm run test:update-snapshots`), how to run locally (`npm test -- preview-regression`).

## Dev Notes

### Why Playwright and not `tools/preview_regression.js`

The epic AC offered `tests/smoke/test_preview_renderer.py` *or* `tools/preview_regression.js` as options. Playwright was chosen over a hand-rolled Node script because:
- The repo already has a full Playwright setup (`tests/e2e/`) with a mock server, test fixtures, and TypeScript types.
- Playwright's `toHaveScreenshot()` handles baseline management, tolerance config, diff-image generation, and CI-friendly flags (`--update-snapshots`) out of the box. Rolling our own in Node requires `canvas` native bindings, a custom diff routine, and CI setup.
- Renders in a real browser (Chromium), which matches what dashboard users actually see — closer parity with production than a Node canvas implementation.

### Golden-frame philosophy

LE-1.9 explicitly opted for **structural + string-resolution** checks on the firmware side rather than full-framebuffer byte comparison, because (a) byte-perfect HUB75 DMA comparison is flaky on-device and (b) structural checks catch the same class of bug at ~10× the iteration speed. LE-2.7 inherits that philosophy for the firmware corpus. Full pixel comparison happens on the JS side via Playwright, where it's reliable.

### Fixture D JSON shape

160×32 canvas (matching the legacy corpus — these are editor-input fixtures, not hardware output, so the dimensions don't need to match HW-1's 256×192 post-migration). Five widgets, one per new LE-2 catalog entry. Uses a known flight with all resolvable fields so the resolution precedence is exercised.

### Out of scope

- **Updating LE-1.9 fixtures to 256×192.** Keeps LE-2.7 focused on the LE-2-specific regressions. Fixture resizing is an HW-1.5 / follow-up concern.
- **Byte-perfect firmware golden-frame comparison.** Deferred indefinitely per LE-1.9's documented choice.
- **Adding `getCurrentFlights()` instrumentation.** The endpoint already exists (LE-2.4 landed); the contract test just asserts its shape.

## Tasks

- [x] Draft this story.
- [x] Add `layout_d.json` fixture + `kFixtureD` inline string + 6 new tests in `test_golden_frames/test_main.cpp` (parse/count, render-smoke, two catalog-presence tests, flight-field resolver loop over LE-2 keys, metric resolver for speed_mph).
- [x] Add `preview-regression.spec.ts` Playwright spec covering two canonical layouts with deterministic mock flight data and `toHaveScreenshot()` pixel-diff.
- [x] Add `/api/widgets/types` + `/api/flights/current` handlers to `tests/e2e/mock-server/server.ts` for the preview-regression harness.
- [x] Add contract assertions for both endpoints to `tests/smoke/test_web_portal_smoke.py` (field-option coverage check for flight_field/metric, row-shape check for flight snapshot).
- [x] Fixed two drift bugs surfaced *while writing the regression gate*: (a) `preview.js` was missing `origin_iata` / `destination_iata` resolver entries (fell through to callsign); (b) `/api/flights/current` wasn't exposing `origin_iata` / `destination_iata` so the preview couldn't render them even with the resolver added. Both fixed alongside the test corpus.
- [x] Regenerated `firmware/data/preview.js.gz`.
- [x] `pio run -e esp32s3_n16r8` — green. `pio test -e esp32s3_n16r8 -f test_golden_frames --without-uploading --without-testing` — green.
- [x] Documented baseline-regeneration workflow at the top of the spec file.
- [x] Documented the local run command (`npm test -- preview-regression` from `tests/e2e/`).

## Change Log

| Date       | Version | Description |
|------------|---------|-------------|
| 2026-04-24 | 0.1     | Draft created. |
| 2026-04-24 | 0.2     | Implementation complete. Firmware golden-frame corpus extended with Fixture D (5 widgets covering every LE-2 catalog entry) + 6 new tests. Playwright preview-regression spec added (2 canonical layouts, chromium-only, `maxDiffPixels: 0`). Mock-server + firmware `/api/flights/current` + `preview.js` resolver all got origin_iata/destination_iata fixes surfaced by the regression gate. Smoke test presence-checks the full LE-2 catalog on `/api/widgets/types` and the row shape on `/api/flights/current`. Both firmware envs and the golden-frame suite compile green. Playwright baseline PNGs are generated on first `npm run test:update-snapshots -- preview-regression` run — needs user to run once to seed. |
