# Story HW-1.1: Toolchain and library swap (HUB75 in, FastLED out)

branch: epic/le-1-layout-editor
zone:
  - firmware/platformio.ini
  - firmware/adapters/NeoMatrixDisplay.{cpp,h}            [deleted]
  - firmware/adapters/HUB75MatrixDisplay.{cpp,h}          [new]
  - firmware/interfaces/DisplayMode.h
  - firmware/widgets/WidgetFont.{cpp,h}
  - firmware/src/main.cpp
  - firmware/modes/ClassicCardMode.cpp
  - firmware/modes/LiveFlightCardMode.cpp

Status: done (build-verified on esp32dev and esp32s3_n16r8; display-dead intermediate — HW-1.2 wires the real HUB75 panels)

## Story

As a **firmware engineer**,
I want **FastLED and FastLED NeoMatrix removed and the HUB75 DMA library added as the display-side dependencies, with a minimal stub keeping the project linkable**,
So that **the project compiles against the new display stack on the td-7-landed `[env:esp32s3_n16r8]` env, ready for HW-1.2's real panel integration**.

## Acceptance Criteria

1. `fastled/FastLED` and `marcmerlin/FastLED NeoMatrix` are removed from `lib_deps`.
2. `mrcodetastic/ESP32 HUB75 LED MATRIX PANEL DMA Display` is present in `lib_deps` and compiles.
3. A placeholder `HUB75MatrixDisplay` (implementing `BaseDisplay` + the public method surface previously provided by `NeoMatrixDisplay`) returns a **192×96 blank canvas** backed by `GFXcanvas16`. All rendering methods are no-ops or minimal stubs; `show()` does nothing (no panels wired yet).
4. `firmware/adapters/NeoMatrixDisplay.{cpp,h}` are deleted.
5. `firmware/interfaces/DisplayMode.h` no longer depends on `FastLED_NeoMatrix.h`; `RenderContext::matrix` is typed as `Adafruit_GFX*` (which all FastLED_NeoMatrix draw callers already use via inheritance).
6. `firmware/widgets/WidgetFont.{cpp,h}`'s forward declaration and API signature change from `FastLED_NeoMatrix*` to `Adafruit_GFX*`.
7. Mode / main call-sites using the FastLED_NeoMatrix-specific `Color(r,g,b)` helper are rewritten to call `Adafruit_GFX::color565(r,g,b)` (the static base-class equivalent).
8. `pio run -e esp32dev` succeeds (classic ESP32 path stays buildable during the migration).
9. `pio run -e esp32s3_n16r8` succeeds against the 16MB partition layout and passes the `check_size.py` 3MB OTA gate.
10. Binary is expected to be **display-dead** — firmware boots, WiFi/web portal work, modes tick, but nothing paints to a physical panel. HW-1.2 wires the real HUB75 chains.

## Dev Notes

### Why the RenderContext type had to change

`RenderContext::matrix` was typed as `FastLED_NeoMatrix*`. That class header lives inside the `marcmerlin/FastLED NeoMatrix` library, which this story removes. All modes and widgets drawing through `ctx.matrix->drawPixel / setCursor / print / fillScreen / fillRect / drawRect / setFont / setTextSize / setTextColor / setTextWrap / write / width / height` use methods inherited from the `Adafruit_GFX` base class — so retyping the pointer to `Adafruit_GFX*` is source-compatible for ~99% of the call sites.

The one FastLED_NeoMatrix-specific method we depended on was `Color(r,g,b) → uint16_t`. `Adafruit_GFX` provides the same functionality as a static method: `Adafruit_GFX::color565(r,g,b)`. Four call sites converted:
- `main.cpp` (3 — status-message colors, clear-color)
- `modes/ClassicCardMode.cpp` (1 — border color)
- `modes/LiveFlightCardMode.cpp` (4 — trend colors, border colors)

### HUB75MatrixDisplay stub design

The stub owns a `GFXcanvas16(192, 96)` as its internal drawing surface. `buildRenderContext()` returns that canvas as `Adafruit_GFX*` so modes draw happily into it. `show()` is a no-op — there's no panel to flush to. `reconfigureFromConfig()`, `updateBrightness()`, and the calibration / positioning toggles are stored-but-ignored (state flips so WebPortal's calibration UI doesn't crash; no visual effect).

The public method surface matches NeoMatrixDisplay so main.cpp and WebPortal compile without changes beyond the instantiation type.

### Why PSRAM stays disabled

Per td-7's investigation: enabling OPI PSRAM (`memory_type = qio_opi` + `BOARD_HAS_PSRAM`) panics the Wi-Fi driver on first station connect. A 192×96 `GFXcanvas16` is 36,864 bytes (2 bytes/pixel × 18,432 pixels). It fits comfortably in SRAM (~160KB usable), so PSRAM is not required for the stub or for HW-1.2's real framebuffer.

### Expected follow-up in HW-1.2

- Real `MatrixPanel_I2S_DMA` top (1/16 scan) + bottom (1/32 scan) chains.
- `show()` calls `flipDMABuffer()` on both chains.
- `renderCalibrationPattern()` validates the y=32 seam.
- Per-chain brightness trim.

## Tasks

- [x] Draft this story.
- [x] Add `Adafruit_GFX::color565` replacements for `FastLED_NeoMatrix::Color` call sites.
- [x] Retype `RenderContext::matrix` to `Adafruit_GFX*` (DisplayMode.h + WidgetFont.{h,cpp}).
- [x] Create `HUB75MatrixDisplay.{h,cpp}` stub.
- [x] Update `main.cpp` to instantiate `HUB75MatrixDisplay`.
- [x] Delete `NeoMatrixDisplay.{h,cpp}`.
- [x] Swap `lib_deps` in `platformio.ini`.
- [x] `pio run -e esp32dev` — passes.
- [x] `pio run -e esp32s3_n16r8` — passes against 3MB OTA slot.
- [ ] On-device smoke: manual user gate — no panel to look at yet, so verify (a) firmware boots to dashboard, (b) `/api/status` reports healthy, (c) mode switch API doesn't crash.

## Change Log

| Date       | Version | Description |
|------------|---------|-------------|
| 2026-04-22 | 0.1     | Draft created and executed in one pass. Build-verified on both envs. |
