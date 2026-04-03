#pragma once

#include <cstdint>
#include <cmath>
#include "core/ConfigManager.h"

struct Rect {
    uint16_t x, y, w, h;
};

struct LayoutResult {
    uint16_t matrixWidth;
    uint16_t matrixHeight;
    const char* mode;  // "compact", "full", or "expanded"
    Rect logoZone;
    Rect flightZone;
    Rect telemetryZone;
    bool valid;
};

class LayoutEngine {
public:
    static LayoutResult compute(uint8_t tilesX, uint8_t tilesY, uint8_t tilePixels);
    static LayoutResult compute(const HardwareConfig& hw);
};
