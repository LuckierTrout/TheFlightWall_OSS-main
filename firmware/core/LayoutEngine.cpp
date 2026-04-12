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
    // Use custom zone percentages when non-zero, otherwise auto layout.
    uint16_t logoW = mh; // default: square logo
    uint16_t splitY = mh / 2; // default: 50/50

    result.logoZone      = { 0,     0,      logoW,                                    mh };
    result.flightZone    = { logoW, 0,      static_cast<uint16_t>(mw - logoW),        splitY };
    result.telemetryZone = { logoW, splitY, static_cast<uint16_t>(mw - logoW),        static_cast<uint16_t>(mh - splitY) };

    return result;
}

LayoutResult LayoutEngine::compute(const HardwareConfig& hw) {
    LayoutResult result = compute(hw.tiles_x, hw.tiles_y, hw.tile_pixels);
    if (!result.valid) return result;

    uint16_t mw = result.matrixWidth;
    uint16_t mh = result.matrixHeight;
    bool customized = false;

    uint16_t logoW = result.logoZone.w;
    uint16_t splitY = result.flightZone.h;

    if (hw.zone_logo_pct > 0 && hw.zone_logo_pct <= 99) {
        logoW = (uint16_t)((uint32_t)mw * hw.zone_logo_pct / 100);
        if (logoW < 1) logoW = 1;
        if (logoW >= mw) logoW = mw - 1;
        customized = true;
    }
    if (hw.zone_split_pct > 0 && hw.zone_split_pct <= 99) {
        splitY = (uint16_t)((uint32_t)mh * hw.zone_split_pct / 100);
        if (splitY < 1) splitY = 1;
        if (splitY >= mh) splitY = mh - 1;
        customized = true;
    }

    if (customized || hw.zone_layout == 1) {
        if (hw.zone_layout == 1) {
            // Full-width bottom: logo top-left, flight top-right, telemetry spans full width
            result.logoZone      = { 0,     0,      logoW,                             splitY };
            result.flightZone    = { logoW, 0,      static_cast<uint16_t>(mw - logoW), splitY };
            result.telemetryZone = { 0,     splitY, mw,                                static_cast<uint16_t>(mh - splitY) };
        } else {
            // Classic: logo full-height left, flight/telemetry stacked right
            result.logoZone      = { 0,     0,      logoW,                             mh };
            result.flightZone    = { logoW, 0,      static_cast<uint16_t>(mw - logoW), splitY };
            result.telemetryZone = { logoW, splitY, static_cast<uint16_t>(mw - logoW), static_cast<uint16_t>(mh - splitY) };
        }
    }

    return result;
}
