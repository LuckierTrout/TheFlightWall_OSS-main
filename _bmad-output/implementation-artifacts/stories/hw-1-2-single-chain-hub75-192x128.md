# Story HW-1.2: Master HUB75MatrixDisplay — single chain, 192×128 (6× 64×64)

branch: epic/le-1-layout-editor
zone:
  - firmware/adapters/HUB75MatrixDisplay.{cpp,h}          [rewrite: stub → real]
  - firmware/adapters/HUB75PinMap.h                        [new]
  - firmware/platformio.ini                                [no change expected]
  - firmware/src/main.cpp                                  [no change expected — class name preserved]

Status: in-progress

## Story

As a **firmware engineer**,
I want **the `HUB75MatrixDisplay` stub replaced with a real driver that pushes pixels to 6× 64×64 panels in a 3×2 arrangement through `MatrixPanel_I2S_DMA` + `VirtualMatrixPanel_T`**,
So that **the master S3 renders a live 192×128 wall end-to-end on real hardware — usable and shippable even before slave work lands**.

## Acceptance Criteria

1. `HUB75MatrixDisplay::initialize()` constructs one `MatrixPanel_I2S_DMA` configured for `PANEL_RES_X=64`, `PANEL_RES_Y=64`, `PANEL_CHAIN=6`, 1/32 scan, `double_buff=true`, pin map supplied by a `constexpr HUB75_PinMap` in `adapters/HUB75PinMap.h`, and wraps it in `VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>(rows=2, cols=3, 64, 64)`.
2. Canvas dimensions reported by `CANVAS_WIDTH`/`CANVAS_HEIGHT` change from 192×96 (stub) to **192×128**.
3. `buildRenderContext()` returns the `VirtualMatrixPanel_T*` as `Adafruit_GFX*` in `ctx.matrix`, so existing modes/widgets draw into real hardware without call-site changes.
4. `show()` calls `dma_display->flipDMABuffer()` — real double-buffered flush, no more no-op.
5. `updateBrightness(b)` calls `dma_display->setBrightness8(b)`.
6. `clear()` calls `virtualDisp->clearScreen()` (or `dma_display->clearScreen()`).
7. `displayMessage()` draws into the virtual canvas; `renderCalibrationPattern()` and `renderPositioningPattern()` paint test patterns that validate panel order, orientation, color channel order, and the inter-panel seam across all 6 panels (corner markers + edge rulers).
8. `LayoutEngine::compute(3, 2, 64)` is called to populate `_layout` with the real 192×128 geometry (not the stub's `compute(1,1,192)` + manual override).
9. A new `adapters/HUB75PinMap.h` file defines `HUB75_PinMap MASTER_CHAIN_PINS` as a `constexpr` struct with all 14 pins (R1, G1, B1, R2, G2, B2, A, B, C, D, E, LAT, OE, CLK). Values are **placeholders** for now (labeled `// TODO(hw-1.2): fill from WatangTech (E) silkscreen`), chosen from ESP32-S3 GPIOs known to be safe (not strapping, not flash/PSRAM, not the native USB D+/D-). The placeholder values let the code compile; hardware bring-up requires swapping the real numbers in this one file.
10. `pio run -e esp32dev` compiles.
11. `pio run -e esp32s3_n16r8` compiles and passes the `check_size.py` 3MB OTA gate.
12. `main.cpp` needs no changes — the class name, header path, and public surface are preserved.
13. **Bring-up smoke (manual, after wiring):** all 6 panels light up, calibration pattern shows each panel's row/col label in its correct grid position with no ghosting or swapped channels; adjusting brightness via the dashboard visibly changes panel output; confirmed no WiFi panic (the td-7 PSRAM interaction is unaffected because PSRAM stays disabled).

## Dev Notes

### What changes vs. the stub

The hw-1.1 stub owned a `GFXcanvas16(192, 96)` and drew into it with a no-op `show()`. This story swaps that memory-backed canvas for a real `VirtualMatrixPanel_T` backed by a `MatrixPanel_I2S_DMA`. The public method surface is unchanged — `main.cpp`, `WebPortal`, `ModeRegistry`, and all `DisplayMode` subclasses keep working against the same API.

### Library reference (mrfaptastic HUB75 DMA)

Confirmed by reading `.pio/libdeps/esp32s3_n16r8/ESP32 HUB75 LED MATRIX PANEL DMA Display/examples/VirtualMatrixPanel/VirtualMatrixPanel.ino`:

```cpp
HUB75_I2S_CFG::i2s_pins pins = {R1, G1, B1, R2, G2, B2, A, B, C, D, E, LAT, OE, CLK};
HUB75_I2S_CFG mxconfig(64, 64, 6, pins);
mxconfig.double_buff = true;
mxconfig.i2sspeed    = HUB75_I2S_CFG::HZ_10M;
mxconfig.clkphase    = false;
MatrixPanel_I2S_DMA* dma = new MatrixPanel_I2S_DMA(mxconfig);
dma->begin();
dma->setBrightness8(128);
dma->clearScreen();

VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>* vdisp =
    new VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>(/*rows=*/2, /*cols=*/3, /*pw=*/64, /*ph=*/64);
vdisp->setDisplay(*dma);
// vdisp inherits Adafruit_GFX — drawPixel / color565 / print / etc. all work
// show-equivalent: dma->flipDMABuffer();
```

### Chain-type choice: `CHAIN_TOP_LEFT_DOWN`

The mrfaptastic library template parameter determines physical → chain-order mapping. Default for this story is `CHAIN_TOP_LEFT_DOWN` (serpentine: top-left → top-right → jumper down → bottom-right → bottom-left). If the user's ribbon routing ends up being a Z-pattern or starting from a different corner, the fix is a single template-argument change in `HUB75MatrixDisplay.h` — no other code touches this. Options the library exposes:

- `CHAIN_TOP_LEFT_DOWN`, `CHAIN_TOP_RIGHT_DOWN`, `CHAIN_BOTTOM_LEFT_UP`, `CHAIN_BOTTOM_RIGHT_UP` (serpentine)
- `CHAIN_TOP_LEFT_DOWN_ZZ`, `CHAIN_TOP_RIGHT_DOWN_ZZ`, `CHAIN_BOTTOM_LEFT_UP_ZZ`, `CHAIN_BOTTOM_RIGHT_UP_ZZ` (zigzag / Z-pattern)

### Pin map — placeholders until silkscreen photo

`adapters/HUB75PinMap.h` holds the master chain's pin map as a `constexpr` struct. For hw-1.2 the values are **placeholders**: GPIOs chosen to be safe on the ESP32-S3 (not strapping pins 0/3/45/46, not flash/PSRAM pins 26-32, not native USB 19/20) but not yet matched to the WatangTech (E) adapter's silkscreen. Real numbers come from a photo of the adapter's board markings; swap them in, reflash, re-run the bring-up smoke.

### LayoutEngine fit

`LayoutEngine::compute(tilesX, tilesY, tilePixels)` is designed for this exact shape: `compute(3, 2, 64)` → `matrixWidth=192, matrixHeight=128`, with the internal guard `matrixWidth >= matrixHeight` (128 ≤ 192 ✓) satisfied. The stub's workaround (`compute(1,1,192)` plus `_layout.matrixHeight = 96` override) goes away.

### What's deferred to later hw-1 stories

- NVS schema rewrite + retirement of `display_pin`/`tiles_x`/`tiles_y`/`tile_pixels` → **hw-1.3**
- Wizard + dashboard UI for fixed 192×128 + slave toggle → **hw-1.4**
- Slave firmware scaffold (second S3) → **hw-1.5**
- Inter-MCU SPI protocol → **hw-1.6**
- `CompositeHUB75Display` wrapper (192×160 composite) → **hw-1.7**

The master S3 at 192×128 is a complete, useful product after this story plus hw-1.3/hw-1.4.

## Tasks

- [x] Draft this story.
- [x] Revise `planning-artifacts/epics/epic-hw-1.md` to dual-MCU direction (landed in same commit).
- [x] Create `firmware/adapters/HUB75PinMap.h` with `MasterChainPins` (placeholders).
- [x] Rewrite `HUB75MatrixDisplay.{h,cpp}`: remove `GFXcanvas16` member; add `MatrixPanel_I2S_DMA* _dma` and `VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>* _virtual`.
- [x] Update `CANVAS_WIDTH=192`, `CANVAS_HEIGHT=128` constants.
- [x] Implement `initialize()` → build `HUB75_I2S_CFG`, `MatrixPanel_I2S_DMA`, `VirtualMatrixPanel_T`, call `setDisplay()`.
- [x] Implement `show()` → `_dma->flipDMABuffer()`.
- [x] Wire `updateBrightness`, `clear`, `displayMessage`, `showLoading`, `displayFlights`, `displayFallbackCard` to draw into `_virtual`.
- [x] Rewrite `renderCalibrationPattern()` and `renderPositioningPattern()` for 192×128 with per-panel labels.
- [x] Update `_layout = LayoutEngine::compute(3, 2, 64);` and remove the `_layout.matrixWidth/Height` override.
- [x] Bump project to `gnu++17` (required by `VirtualMatrixPanel_T`'s `if constexpr`).
- [x] `pio run -e esp32dev` — passes (1.23 MB / 1.5 MB, 81.4%).
- [x] `pio run -e esp32s3_n16r8` — passes (1.16 MB / 3 MB, 38.7%, check_size gate green).
- [ ] On-device smoke gate (manual, after user wires panels and provides silkscreen pin map): all 6 panels lit, calibration pattern correct, brightness adjustable, no WiFi panic.

## Change Log

| Date       | Version | Description |
|------------|---------|-------------|
| 2026-04-22 | 0.1     | Draft created; epic revised to dual-MCU direction. |
| 2026-04-22 | 0.2     | Implementation complete: HUB75PinMap.h, rewritten HUB75MatrixDisplay, gnu++17 bump. Both envs compile; awaiting on-device bring-up after user photographs WatangTech (E) silkscreen for pin map. |
| 2026-04-23 | 0.3     | Scope revised: uniform 12-panel wall (4×3 of 64×64 = 256×192) in place of the dual-MCU 192×128 master + 192×32 slave plan. Constants bumped (`COLS=4`, `ROWS=3`, `CHAIN_LEN=12`, `CANVAS_WIDTH=256`, `CANVAS_HEIGHT=192`). Calibration pattern swatch table expanded from 2×3 to 3×4. No structural changes — single-chain single-MCU drops all slave-related stories (see epic-hw-1.md). Filename retained for history; story title is now effectively "Master HUB75MatrixDisplay — single chain, 256×192". |
