# Story HW-1.4: Wizard & dashboard UI for fixed 256×192 HUB75 wall

branch: epic/le-1-layout-editor
zone:
  - firmware/data-src/wizard.{html,js}                [edit — step 4 rewrite]
  - firmware/data-src/dashboard.{html,js}             [edit — drop 4 hardware inputs]
  - firmware/data/*.gz                                 [regenerate]
  - tests/smoke/test_web_portal_smoke.py              [edit — drop retired-key assertions]

Status: in-progress

## Story

As an **owner**,
I want **the setup wizard and dashboard to reflect the fixed 256×192 HUB75 wall instead of the WS2812B-era tile/pin configuration**,
So that **first-boot setup is a read-only hardware summary and the smoke test passes end-to-end with the retired-key references cleaned up**.

## Acceptance Criteria

1. `data-src/wizard.html` step 4 removes the four tile/pin `<input>` controls (`tiles-x`, `tiles-y`, `tile-pixels`, `display-pin`) and the `resolution-text` element; adds a read-only summary paragraph ("256 × 192 pixels — 12 HUB75 panels in a 4 wide × 3 tall grid"); retains the `origin-corner` / `scan-dir` / `zigzag` selectors for now (those knobs still exist in `HardwareConfig` even though they're effectively no-ops under `VirtualMatrixPanel_T`; retiring them is out of scope for hw-1.4).
2. `data-src/wizard.js` drops `state.tiles_x/tiles_y/tile_pixels/display_pin`, the `VALID_PINS` array, the `WIZARD_KEYS` entries for these four, the `hydrateDefaults` reads, the step-4 form rehydration, the step-4 save path, and the step-4 validation path (validation becomes a no-op returning `true`). The review card for Hardware shows only "Display: 256 × 192 (4×3 of 64×64 panels)" plus origin/scan/zigzag.
3. `data-src/wizard.js` final POST payload no longer includes `tiles_x/tiles_y/tile_pixels/display_pin`.
4. `renderPositionCanvas()` uses fixed `4×3` grid constants (matching the real hardware) instead of reading `state.tiles_x/state.tiles_y`.
5. `updateResolution()` function is removed along with its input-event listeners.
6. `data-src/dashboard.html` hardware card removes the `d-tiles-x`, `d-tiles-y`, `d-tile-pixels`, `d-display-pin` inputs and their labels.
7. `data-src/dashboard.js` drops the four hardware input DOM refs, stops reading them into the POST payload, stops hydrating them from `/api/settings`, and any preview code that needs tile math reads it from `/api/layout` (which still exposes `tiles_x/tiles_y/tile_pixels` as derived/nominal values).
8. `firmware/data/wizard.html.gz`, `firmware/data/wizard.js.gz`, `firmware/data/dashboard.html.gz`, `firmware/data/dashboard.js.gz` are regenerated from the updated sources.
9. `tests/smoke/test_web_portal_smoke.py` drops `tiles_x/tiles_y/tile_pixels` from the `/api/settings` GET presence assertions (they're no longer in the response).
10. `pio run -e esp32dev` and `pio run -e esp32s3_n16r8` still compile (no firmware code changes).
11. No existing test suite regresses (`test_config_manager`, `test_layout_engine`).
12. `preview.js` / `editor.js` are left untouched — they read `tile_pixels` and `tiles_x/y` from `/api/layout`, which still exposes them (derived from the fixed canvas dimensions), so their code keeps working without edits. A deeper rewrite to read `matrix.width/height` directly is out of scope.

## Dev Notes

### Out-of-scope cleanups (deferred)

- `origin_corner` / `scan_dir` / `zigzag` selectors remain in the wizard/dashboard UI and in `HardwareConfig`. With HUB75 + `VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>` these are effectively no-ops (the chain orientation is a compile-time template parameter), but retiring them touches `applyJson`, persistence, /api/settings, NVS cleanup, tests, and the UI — a bigger sweep than belongs in this story.
- `preview.js` / `editor.js` rewriting from tile-based to matrix-dim-based geometry could be cleaner but isn't broken — deferred.

### Why keep `/api/layout` hardware block unchanged

`/api/layout` exposes `hardware.tiles_x`, `hardware.tiles_y`, `hardware.tile_pixels` as **derived** values (computed from `MASTER_CANVAS_WIDTH / NOMINAL_TILE_PIXELS`). The LE-1 editor and the dashboard preview read these for their snap-grid math. Leaving them in the response means `preview.js` and `editor.js` need no change.

### POST silent-discard safety net

Even with the UI cleanup, the firmware-side `applyJson` keeps its silent-discard of the retired keys from hw-1.3. Clients that missed the update (or cached old JS) continue to function.

## Tasks

- [x] Draft this story.
- [x] Edit `data-src/wizard.html` step 4 (removed 4 inputs, added summary paragraph).
- [x] Edit `data-src/wizard.js` (state, keys, hydrate, rehydrate, save, validate, payload, renderPositionCanvas, event listeners, updateResolution).
- [x] Edit `data-src/dashboard.html` hardware card (removed 4 inputs).
- [x] Edit `data-src/dashboard.js` (DOM refs, payload, hydrate, parseHardwareDimensionsFromInputs now returns fixed 256×192, listeners deleted).
- [x] Regenerate gzipped assets: wizard.html.gz, wizard.js.gz, dashboard.html.gz, dashboard.js.gz.
- [x] Edit `tests/smoke/test_web_portal_smoke.py` presence-check list.
- [x] `pio run -e esp32dev` — passes.
- [x] `pio run -e esp32s3_n16r8` — passes (1.16 MB / 3 MB, 38.7%).
- [x] `pio test -f test_config_manager --without-uploading --without-testing` — passes.
- [x] `pio test -f test_layout_engine --without-uploading --without-testing` — passes.

## Change Log

| Date       | Version | Description |
|------------|---------|-------------|
| 2026-04-24 | 0.1     | Draft created. |
| 2026-04-24 | 0.2     | Implementation complete. Step 4 hardware step converted to a read-only summary. Dashboard hardware card drops 4 inputs; canvas dimensions now hardcoded in `parseHardwareDimensionsFromInputs()` to 256×192. Gzipped assets regenerated. Smoke test presence-check updated. Both envs + both test suites compile. |
