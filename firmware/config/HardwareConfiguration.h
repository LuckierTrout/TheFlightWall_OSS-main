#pragma once

#include <Arduino.h>

namespace HardwareConfiguration
{
    // Display configuration (FastLED NeoMatrix)
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    static const uint8_t DISPLAY_PIN = 21;
#else
    static const uint8_t DISPLAY_PIN = 25;
#endif

    // Physical tile size (pixels per 16x16 tile commonly)
    static const uint16_t DISPLAY_TILE_PIXEL_W = 16;
    static const uint16_t DISPLAY_TILE_PIXEL_H = 16;

    // Tile arrangement (number of tiles horizontally and vertically)
    static const uint8_t DISPLAY_TILES_X = 1;  // Single 16x16 panel
    static const uint8_t DISPLAY_TILES_Y = 1;

    // Derived matrix dimensions
    static const uint16_t DISPLAY_MATRIX_WIDTH = DISPLAY_TILE_PIXEL_W * DISPLAY_TILES_X;
    static const uint16_t DISPLAY_MATRIX_HEIGHT = DISPLAY_TILE_PIXEL_H * DISPLAY_TILES_Y;
}
