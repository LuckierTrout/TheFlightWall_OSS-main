/*
Purpose: Zone calculation engine for LED matrix layout.
Responsibilities:
- Compute matrix dimensions and zone positions from hardware config.
- Produce identical results to the JS implementation in dashboard.js.
Architecture: Pure computation — no hardware dependencies. Shared algorithm
  defined in architecture.md must be matched exactly.
*/
#include "core/LayoutEngine.h"
#include "utils/Log.h"

LayoutResult LayoutEngine::compute(uint8_t tilesX, uint8_t tilesY, uint8_t tilePixels) {
    LayoutResult result = {};

    // Guard against invalid geometry
    if (tilesX == 0 || tilesY == 0 || tilePixels == 0) {
        LOG_E("LayoutEngine", "Invalid geometry: zero dimension in tiles/pixels");
        result.valid = false;
        result.mode = "compact";
        return result;
    }

    uint16_t mw = static_cast<uint16_t>(tilesX) * tilePixels;
    uint16_t mh = static_cast<uint16_t>(tilesY) * tilePixels;

    // Guard: matrixWidth must be >= matrixHeight for zone layout to make sense
    if (mw < mh) {
        LOG_E("LayoutEngine", "Invalid geometry: matrixWidth < matrixHeight");
        result.valid = false;
        result.mode = "compact";
        result.matrixWidth = mw;
        result.matrixHeight = mh;
        return result;
    }

    result.matrixWidth = mw;
    result.matrixHeight = mh;
    result.valid = true;

    // Mode breakpoints
    if (mh < 32) {
        result.mode = "compact";
    } else if (mh < 48) {
        result.mode = "full";
    } else {
        result.mode = "expanded";
    }

    // Zone calculation (shared algorithm — must match dashboard.js exactly)
    uint16_t halfH = mh / 2;

    result.logoZone      = { 0,  0,     mh,        mh };
    result.flightZone    = { mh, 0,     static_cast<uint16_t>(mw - mh), halfH };
    result.telemetryZone = { mh, halfH, static_cast<uint16_t>(mw - mh), static_cast<uint16_t>(mh - halfH) };

    return result;
}

LayoutResult LayoutEngine::compute(const HardwareConfig& hw) {
    return compute(hw.tiles_x, hw.tiles_y, hw.tile_pixels);
}
