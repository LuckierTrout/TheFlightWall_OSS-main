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

namespace {
// Universal frame inset: every computed zone leaves a 1 px gutter on all
// four sides of the panel. main.cpp's display loop paints a matching 1 px
// black rectangle around the panel on every frame, so content rendered from
// these zones sits *inside* the frame instead of underneath it.
static constexpr uint16_t FRAME_INSET_PX = 1;

// Horizontal gap between the logo zone and the flight/telemetry zones to
// the right of it. Without this the airline logo bitmap butts directly
// against the first glyph of the callsign text — readable but cramped.
static constexpr uint16_t LOGO_GAP_PX = 2;

uint16_t maxHorizontalInsetX(uint16_t matrixWidth, uint16_t matrixHeight) {
    if (matrixWidth <= matrixHeight) return 0;
    return static_cast<uint16_t>((matrixWidth - matrixHeight - 1U) / 2U);
}

uint16_t clampHorizontalInsetX(uint16_t matrixWidth, uint16_t matrixHeight, uint8_t insetX) {
    const uint16_t maxInset = maxHorizontalInsetX(matrixWidth, matrixHeight);
    return insetX > maxInset ? maxInset : insetX;
}

void applyZoneLayout(LayoutResult& result, uint16_t logoW, uint16_t splitY,
                     uint8_t zoneLayout, uint8_t insetX) {
    // Apply FRAME_INSET_PX to the usable area so every zone lands inside the
    // 1 px frame drawn by the display loop. Matrix dimensions stay at the
    // real hardware size (callers that render the frame itself still need
    // panel coords), but the derived content zones start at (FRAME_INSET_PX,
    // FRAME_INSET_PX) and shrink by 2*FRAME_INSET_PX on each axis.
    const uint16_t mw = result.matrixWidth;
    const uint16_t mh = result.matrixHeight;
    if (mw <= 2 * FRAME_INSET_PX || mh <= 2 * FRAME_INSET_PX) {
        result.logoZone = {0, 0, 0, 0};
        result.flightZone = {0, 0, 0, 0};
        result.telemetryZone = {0, 0, 0, 0};
        return;
    }
    const uint16_t usableW = static_cast<uint16_t>(mw - 2 * FRAME_INSET_PX);
    const uint16_t usableH = static_cast<uint16_t>(mh - 2 * FRAME_INSET_PX);

    const uint16_t padX = clampHorizontalInsetX(usableW, usableH, insetX);
    const uint16_t contentX = static_cast<uint16_t>(FRAME_INSET_PX + padX);
    const uint16_t contentY = FRAME_INSET_PX;
    const uint16_t contentW = static_cast<uint16_t>(usableW - 2U * padX);
    const uint16_t contentH = usableH;
    uint16_t clampedLogoW = logoW;

    if (contentW == 0) {
        result.logoZone = {0, 0, 0, 0};
        result.flightZone = {0, 0, 0, 0};
        result.telemetryZone = {0, 0, 0, 0};
        return;
    }

    if (clampedLogoW < 1) clampedLogoW = 1;
    if (clampedLogoW >= contentW) clampedLogoW = contentW - 1;

    // Clamp split/logo sizes against the usable (inset) dimensions so a
    // caller that computed them against the raw matrix doesn't overflow
    // into the reserved frame area.
    uint16_t clampedSplitY = splitY;
    if (clampedSplitY > contentH) clampedSplitY = contentH;

    // Reserve a small horizontal gap between the logo zone and whatever
    // sits to its right, so the airline logo doesn't butt against the
    // first glyph of the callsign/route. The gap is carved out of the
    // flight/telemetry side; logo keeps its requested width. If the
    // content is too narrow to afford a gap (edge case: tiny panels),
    // we silently skip it so the right-hand zones still have width.
    const uint16_t gap =
        (contentW > static_cast<uint16_t>(clampedLogoW + LOGO_GAP_PX + 1))
        ? LOGO_GAP_PX : 0;
    const uint16_t rightX  = static_cast<uint16_t>(contentX + clampedLogoW + gap);
    const uint16_t rightW  = static_cast<uint16_t>(contentW - clampedLogoW - gap);

    if (zoneLayout == 1) {
        result.logoZone = {contentX, contentY, clampedLogoW, clampedSplitY};
        result.flightZone = {rightX, contentY, rightW, clampedSplitY};
        result.telemetryZone = {contentX, static_cast<uint16_t>(contentY + clampedSplitY),
                                contentW,
                                static_cast<uint16_t>(contentH - clampedSplitY)};
    } else {
        result.logoZone = {contentX, contentY, clampedLogoW, contentH};
        result.flightZone = {rightX, contentY, rightW, clampedSplitY};
        result.telemetryZone = {rightX,
                                static_cast<uint16_t>(contentY + clampedSplitY),
                                rightW,
                                static_cast<uint16_t>(contentH - clampedSplitY)};
    }
}
}  // namespace

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

    // Zone calculation (shared algorithm — must match dashboard.js exactly).
    // Auto layout uses a square logo, 50/50 split, and no outer gutter.
    applyZoneLayout(result, mh, static_cast<uint16_t>(mh / 2), 0, 0);

    return result;
}

LayoutResult LayoutEngine::compute(const HardwareConfig& hw) {
    LayoutResult result = compute(hw.tiles_x, hw.tiles_y, hw.tile_pixels);
    if (!result.valid) return result;

    uint16_t mw = result.matrixWidth;
    uint16_t mh = result.matrixHeight;
    uint16_t logoW = result.logoZone.w;
    uint16_t splitY = result.flightZone.h;

    if (hw.zone_logo_pct > 0 && hw.zone_logo_pct <= 99) {
        logoW = (uint16_t)((uint32_t)mw * hw.zone_logo_pct / 100);
        if (logoW < 1) logoW = 1;
        if (logoW >= mw) logoW = mw - 1;
    }
    if (hw.zone_split_pct > 0 && hw.zone_split_pct <= 99) {
        splitY = (uint16_t)((uint32_t)mh * hw.zone_split_pct / 100);
        if (splitY < 1) splitY = 1;
        if (splitY >= mh) splitY = mh - 1;
    }
    applyZoneLayout(result, logoW, splitY, hw.zone_layout, hw.zone_pad_x);

    return result;
}
