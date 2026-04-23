# Story HW-1.3: Hardware configuration & NVS schema rewrite

branch: epic/le-1-layout-editor
zone:
  - firmware/config/HardwareConfiguration.h                 [rewrite]
  - firmware/core/ConfigManager.{cpp,h}                     [edit]
  - firmware/core/LayoutEngine.{cpp,h}                      [edit]
  - firmware/adapters/WebPortal.cpp                         [edit — /api/settings, /api/layout]
  - firmware/test/test_config_manager/test_main.cpp         [edit]
  - firmware/test/test_layout_engine/test_main.cpp          [edit]

Status: in-progress

## Story

As a **firmware engineer**,
I want **the hardware configuration surface rewritten: the WS2812B-era tile/pin keys retired from `HardwareConfig` and NVS, replaced with fixed canvas constants reflecting the HW-1 master chain, and a `slave_enabled` flag added for the upcoming slave work**,
So that **ConfigManager, WebPortal, and LayoutEngine stop advertising concepts that no longer apply and devices boot cleanly whether or not legacy NVS values are present**.

## Acceptance Criteria

1. `HardwareConfig` struct drops `tiles_x`, `tiles_y`, `tile_pixels`, `display_pin`; adds `bool slave_enabled` (default `false`).
2. `config/HardwareConfiguration.h` removes `DISPLAY_PIN`, `DISPLAY_TILE_PIXEL_W/H`, `DISPLAY_TILES_X/Y`, `DISPLAY_MATRIX_WIDTH/HEIGHT`; adds `MASTER_CANVAS_WIDTH = 192`, `MASTER_CANVAS_HEIGHT = 128`, `COMPOSITE_WIDTH = 192`, `COMPOSITE_HEIGHT = 160`, `NOMINAL_TILE_PIXELS = 64`.
3. `ConfigManager::loadDefaults()` no longer initializes the 4 retired fields; sets `slave_enabled = false`.
4. `ConfigManager::loadFromNvs()` no longer reads the 4 retired keys. When any of them exist in NVS (legacy install), they are **removed via `prefs.remove(key)`** so the schema drifts cleanly to the new shape on next boot. Reads `slave_enabled`.
5. `ConfigManager::persistToNvs()` no longer writes the 4 retired keys. Writes `slave_enabled`.
6. `ConfigManager::dumpSettingsJson()` no longer exposes the 4 retired keys in the `hardware` sub-object. Exposes `slave_enabled`.
7. `ConfigManager::applyJson()` **silently accepts and discards** the 4 retired keys if a client POSTs them (backward-compat bridge — the wizard and dashboard UI still send them until hw-1.4 cleans them up). They do not appear in `applied`, they do not trigger `reboot_required`, they do not cause the whole POST to fail. Accepts `slave_enabled` as a new settable key.
8. `ConfigManager::requiresReboot()` drops `display_pin`, `tiles_x`, `tiles_y`, `tile_pixels` from its reboot-key list; adds `slave_enabled`.
9. `isValidTileConfig()` and `maxHorizontalZonePadX()` helpers in ConfigManager are rewritten to take the fixed master canvas dimensions (or removed and inlined where they have callers).
10. `LayoutEngine::compute(const HardwareConfig& hw)` no longer reads `hw.tiles_x`/`hw.tiles_y`/`hw.tile_pixels`. It computes against the fixed `MASTER_CANVAS_WIDTH × MASTER_CANVAS_HEIGHT` (or `COMPOSITE_WIDTH × COMPOSITE_HEIGHT` if `hw.slave_enabled` is true — wired in hw-1.7; for hw-1.3, the compute path can look at `slave_enabled` and pick the right dimensions, even though the composite wrapper doesn't exist yet).
11. `LayoutEngine::compute(tilesX, tilesY, tilePixels)` **remains** as a backward-compat overload for direct callers (HUB75MatrixDisplay still uses it) — no change required.
12. `/api/settings` GET response's `hardware` object no longer includes the 4 retired keys; includes `slave_enabled`.
13. `/api/layout` returns `matrix.width = 192`, `matrix.height = 128` (or `160` when `slave_enabled`), `tile_pixels = 64` (nominal — keeps the LE-1 editor's snap-grid working).
14. `test_config_manager` suite is updated: tests for retired keys removed; a new test confirms POSTing any of the 4 retired keys succeeds (silently discarded) and does not set `reboot_required`; a new test confirms legacy NVS values are cleared on boot.
15. `test_layout_engine` suite is updated: the `compute(HardwareConfig)` test uses the new struct shape (no tile fields; drives via `slave_enabled`).
16. `pio run -e esp32dev` and `pio run -e esp32s3_n16r8` both compile. `pio test -f test_config_manager --without-uploading --without-testing` and `pio test -f test_layout_engine --without-uploading --without-testing` both compile.

## Dev Notes

### Scope boundary — backend only

The wizard (`data-src/wizard.js`), dashboard (`data-src/dashboard.js`), preview (`data-src/preview.js`), and editor (`data-src/editor.js`) web-JS still reference or POST the retired keys. Those are **hw-1.4 scope**, not this story. Until hw-1.4 lands, the retired-key POST values are silently discarded by `applyJson` so the UI continues to function unchanged (no 400 errors, no "reboot required" dialog triggered by values that no longer matter).

`tests/smoke/test_web_portal_smoke.py` is also hw-1.4 scope.

### Why silent-discard instead of reject

Option A (reject): clients POSTing retired keys get 400; forces hw-1.4 to land before hw-1.3 is usable; violates the epic's "always shippable" incremental-delivery principle.

Option B (silent-discard): UI continues to function after hw-1.3; retired key values are simply ignored; hw-1.4 removes the JS references at leisure. Chosen.

### NVS cleanup strategy

Rather than leaving orphaned keys in NVS, `loadFromNvs()` removes them via `prefs.remove(key)` on first boot of the new firmware. Idempotent — if they're already absent, `remove()` is a no-op.

### `LayoutEngine::compute(HardwareConfig)` transition

Existing code does `compute(hw.tiles_x, hw.tiles_y, hw.tile_pixels)` and then adjusts zone dimensions from zone_* fields. New version picks canvas dimensions from `slave_enabled` and calls the dimensioned overload: `compute(canvas_w / NOMINAL_TILE_PIXELS, canvas_h / NOMINAL_TILE_PIXELS, NOMINAL_TILE_PIXELS)` — this yields the same `matrixWidth × matrixHeight` output, just with hard-coded inputs that match hw-1.2's reality.

Note: since `COMPOSITE_HEIGHT = 160` is not evenly divisible by 64, the composite-mode path uses `compute(3, 160/64=2, 64)` which would report 192×128, losing 32 rows. To preserve 192×160, we add a direct-dimension overload `compute(width, height, zoneCfg)` or call the existing `compute` with `NOMINAL_TILE_PIXELS = 32` for composite mode (32 × 3 = 96 wide, 32 × 5 = 160 tall — still wrong). Simplest: add a direct `computeForCanvas(uint16_t w, uint16_t h, const HardwareConfig& hw)` helper that bypasses the tile math.

### `slave_enabled` reboot requirement

Adding/removing the slave changes the canvas dimensions reported by `/api/layout`, which downstream affects the Layout Engine's zone computation and the editor's canvas. Toggling it at runtime would require re-initializing the display adapter and re-laying out all modes mid-flight — complex and risky. Simpler to treat as reboot-required like the other structural keys.

## Tasks

- [x] Draft this story.
- [x] Rewrite `config/HardwareConfiguration.h` (tile/pin constants out; canvas + nominal tile constants in).
- [x] Update `HardwareConfig` struct in `ConfigManager.h` (drop 4 fields; add `slave_enabled`).
- [x] Update `ConfigManager.cpp`: defaults, loadFromNvs (with legacy-key cleanup), persistToNvs, dumpSettingsJson, applyJson (silent-discard for retired keys), requiresReboot, validation helpers (deleted `isSupportedDisplayPin` and `isValidTileConfig`; rewrote `maxHorizontalZonePadX` to read canvas dims from `HardwareConfiguration`).
- [x] Adapt `LayoutEngine::compute(HardwareConfig)` to use fixed canvas dimensions driven by `slave_enabled`.
- [x] Update `/api/layout` response in WebPortal (tile_pixels always 64 nominal; tiles_x/tiles_y derived from canvas; slave_enabled exposed).
- [x] Update `main.cpp` hardware-change detectors (`hardwareConfigChanged`, `hardwareGeometryChanged`) to the new field set.
- [x] Update `test_config_manager` tests (retired-key tests removed/replaced; new silent-discard + no-reboot test; zone_pad_x validation updated to the fixed 192x128 canvas).
- [x] Update `test_layout_engine` tests (HardwareConfig-overload tests rewritten for 192x128 master-only and 192x160 composite).
- [x] `pio run -e esp32dev` — passes.
- [x] `pio run -e esp32s3_n16r8` — passes (1.16 MB / 3 MB, 38.7%).
- [x] `pio test -e esp32s3_n16r8 -f test_config_manager --without-uploading --without-testing` — compiles.
- [x] `pio test -e esp32s3_n16r8 -f test_layout_engine --without-uploading --without-testing` — compiles.

## Change Log

| Date       | Version | Description |
|------------|---------|-------------|
| 2026-04-22 | 0.1     | Draft created. |
| 2026-04-22 | 0.2     | Implementation complete. Both envs compile; both test suites compile. Wizard/dashboard JS cleanup and smoke test update remain for hw-1.4. |
| 2026-04-23 | 0.3     | Scope revised for the uniform 12-panel decision. `slave_enabled` retired from HardwareConfig, NVS, applyJson, REBOOT_KEYS, dumpSettingsJson, /api/layout, /api/settings, and main.cpp diff functions (it's now in the legacy-key NVS cleanup list for clean migration). `HardwareConfiguration.h` dropped `COMPOSITE_WIDTH/HEIGHT` and bumped `MASTER_CANVAS_WIDTH/HEIGHT` to 256/192. `LayoutEngine::compute(HardwareConfig)` simplified to target the fixed 256×192 directly. `maxHorizontalZonePadX` collapsed to a constexpr compile-time constant. Tests updated accordingly. All builds and tests still compile. |
