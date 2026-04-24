# Story HW-1.6: Test suite cleanup (FastLED_NeoMatrix → GFXcanvas16)

branch: epic/le-1-layout-editor
zone:
  - firmware/test/test_classic_card_mode/test_main.cpp   [edit]
  - firmware/test/test_mode_registry/test_main.cpp       [edit]

Status: in-progress

## Story

As a **maintainer**,
I want **the test suites that created real `FastLED_NeoMatrix` instances ported to use `GFXcanvas16` (pure Adafruit_GFX) instead**,
So that **the test suite stays green and meaningful after the HW-1 adapter swap that deleted FastLED from `lib_deps`**.

## Acceptance Criteria

1. `test_classic_card_mode/test_main.cpp` no longer references `FastLED_NeoMatrix`, `CRGB`, or `testLeds`. `createTestMatrix()` returns a `GFXcanvas16*` initialized to a 64×32 in-memory RGB565 buffer. `countNonBlackPixels(m)` walks `m->getBuffer()`. `makeRealMatrixCtx(m)` sets `ctx.matrix = m` and `ctx.textColor = DisplayUtils::rgb565(255, 255, 255)`. Buffer resets that previously did `memset(testLeds, …)` now call `m->fillScreen(0)`.
2. `test_mode_registry/test_main.cpp` applies the same treatment and removes `#include <FastLED.h>` + `#include <FastLED_NeoMatrix.h>`.
3. `pio test -e esp32s3_n16r8 --without-uploading --without-testing` across **all 20 test suites** compiles with zero errors.
4. No test semantics regress: each ported test still verifies the same behaviour (pixel count after render, buffer clear before render, textColor in render context) — the only change is the underlying buffer type and the canvas-reset primitive.
5. `tests/smoke/test_web_portal_smoke.py` was cleaned up in hw-1.4 (dropped retired settings keys from presence check) — no further change here.

## Dev Notes

### Why `GFXcanvas16` is the right replacement

`GFXcanvas16` is an `Adafruit_GFX` subclass backed by a `uint16_t[]` RGB565 pixel buffer. It supports every method the tests actually use (`drawPixel`, `fillRect`, `setCursor`, `setTextColor`, `setTextSize`, `print`, `getTextBounds`, `fillScreen`, etc.) and exposes the raw buffer via `getBuffer()` for pixel-level assertions. It's the same GFX contract the real HUB75 runtime surface (`VirtualMatrixPanel_T`) presents, so `ctx.matrix` behaves identically.

### Why `DisplayUtils::rgb565` instead of `color565`

`color565` is an instance method on `Adafruit_SPITFT` specifically, not on the `Adafruit_GFX` base class. The project already has a `DisplayUtils::rgb565(r, g, b)` helper that all production call sites use; the tests now do the same.

### What was NOT changed

- Test logic / assertions. The ported tests verify the same render behaviour as before.
- `fixtures/test_helpers.h` / `fixtures/test_fixtures.h` — clean already (grep found no FastLED/CRGB references).
- Other test suites (`test_live_flight_card_mode`, `test_widget_registry`, etc.) were clean already — they use `ctx.matrix = nullptr` or a lightweight mock rather than a real GFX instance.

## Tasks

- [x] Draft this story.
- [x] Port `test_classic_card_mode/test_main.cpp`: `GFXcanvas16*` helpers, remove `testLeds`, replace `memset` with `fillScreen(0)`, replace `m->Color` with `DisplayUtils::rgb565`.
- [x] Port `test_mode_registry/test_main.cpp`: same treatment plus remove `#include <FastLED.h>` / `<FastLED_NeoMatrix.h>`.
- [x] Full test compile pass — 20/20 suites green.

## Change Log

| Date       | Version | Description |
|------------|---------|-------------|
| 2026-04-24 | 0.1     | Draft + implementation landed in one pass. Two test files updated; all 20 suites compile. |
