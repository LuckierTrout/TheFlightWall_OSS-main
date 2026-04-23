#pragma once

#include <Arduino.h>

/*
  Hardware configuration constants (post hw-1.3).

  The legacy tile/pin scheme (DISPLAY_PIN / DISPLAY_TILES_X/Y /
  DISPLAY_TILE_PIXEL_W/H / DISPLAY_MATRIX_WIDTH/HEIGHT) is retired:
  the HW-1 HUB75 migration fixes the master chain at 3x2 of 64x64
  panels (192x128) driven through LCD_CAM + VirtualMatrixPanel_T,
  and the optional slave chain adds a 192x32 top strip for a
  192x160 composite. Pin map for the master chain lives in
  adapters/HUB75PinMap.h; there is no user-configurable "data pin".
*/

namespace HardwareConfiguration
{
    // Master chain: 6x 64x64 panels arranged 3 wide x 2 tall.
    static constexpr uint16_t MASTER_CANVAS_WIDTH  = 192;
    static constexpr uint16_t MASTER_CANVAS_HEIGHT = 128;

    // Composite (master + slave top strip): 192x160. Only reported
    // when HardwareConfig::slave_enabled is true.
    static constexpr uint16_t COMPOSITE_WIDTH  = 192;
    static constexpr uint16_t COMPOSITE_HEIGHT = 160;

    // Nominal "tile pixels" surfaced via /api/layout so the LE-1
    // editor's snap-grid keeps working. Matches the physical panel
    // size on the master chain.
    static constexpr uint16_t NOMINAL_TILE_PIXELS = 64;
}
