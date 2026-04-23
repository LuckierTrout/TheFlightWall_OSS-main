#pragma once

#include <Arduino.h>

/*
  Hardware configuration constants (post hw-1.3, revised 2026-04-23).

  The legacy tile/pin scheme (DISPLAY_PIN / DISPLAY_TILES_X/Y /
  DISPLAY_TILE_PIXEL_W/H / DISPLAY_MATRIX_WIDTH/HEIGHT) is retired:
  the HW-1 HUB75 migration fixes the wall at a uniform 4x3 grid of
  64x64 panels (256x192) driven through LCD_CAM + VirtualMatrixPanel_T
  on a single chain. Pin map lives in adapters/HUB75PinMap.h;
  there is no user-configurable "data pin".

  The initial dual-MCU plan (192x128 master + 192x32 slave = 192x160
  composite) was retired when the panel mix went uniform; there is no
  "composite" mode now, no slave, and no runtime canvas selection.
*/

namespace HardwareConfiguration
{
    // Uniform wall: 12x 64x64 panels arranged 4 wide x 3 tall.
    static constexpr uint16_t MASTER_CANVAS_WIDTH  = 256;
    static constexpr uint16_t MASTER_CANVAS_HEIGHT = 192;

    // Nominal "tile pixels" surfaced via /api/layout so the LE-1
    // editor's snap-grid keeps working. Matches the physical panel
    // size.
    static constexpr uint16_t NOMINAL_TILE_PIXELS = 64;
}
