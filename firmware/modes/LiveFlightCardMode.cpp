/*
Purpose: LiveFlightCardMode implementation — enriched telemetry flight card
         rendering via the DisplayMode system.
Story ds-2.1: replaces ds-1.3 stub with real rendering via DisplayUtils + RenderContext.
Story ds-2.2: adaptive field dropping (priority: vrate→hdg→spd→alt→route→airline) +
              vertical trend indicator (5×5 glyph, telemetry zone leading column).
Sources: ClassicCardMode (ds-1.4) for cycling/idle/fallback/clamp patterns;
         NeoMatrixDisplay::renderFlightZone, renderTelemetryZone for telemetry layout.

Review follow-ups addressed (synthesis-ds-2.1):
- Cycling state moved before matrix null guard for testability (mirrors ClassicCardMode)
- Fixed cycling: zero-interval guard, multi-step catch-up, _lastCycleMs resets on
  single-flight and empty-flight states
- Added clampZone() and applied to all zones in renderZoneFlight()
- Replaced String temporaries with char[] + snprintf (zero heap on hot path)
*/

#include "modes/LiveFlightCardMode.h"
#include "utils/DisplayUtils.h"
#include "core/LogoManager.h"

#include <cstring>
#include <cmath>

// Zone descriptors for Mode Picker UI — reflect real LayoutEngine zones:
// Logo (left strip), Flight (center-right, top portion), Telemetry (center-right, bottom)
const ZoneRegion LiveFlightCardMode::_zones[] = {
    {"Logo",      0,  0, 25, 100},
    {"Flight",   25,  0, 75,  50},
    {"Telemetry",25, 50, 75,  50}
};

const ModeZoneDescriptor LiveFlightCardMode::_descriptor = {
    "Live flight card with adaptive telemetry layout (priority drop: vrate->hdg->spd->alt) and vertical trend indicator",
    _zones,
    3
};

// Character dimensions for default 5x7 Adafruit GFX font (+1px spacing)
static constexpr int CHAR_WIDTH = 6;
static constexpr int CHAR_HEIGHT = 8;

// Maximum buffer size for text lines.
// The compact telemetry format "A%s S%s T%s V%s" with four 15-char values
// can reach 68 characters; 72 provides safe headroom without silent truncation.
static constexpr size_t LINE_BUF_SIZE = 72;

// ds-2.2: Trend glyph occupies 5px at the leading edge of the telemetry zone (AC #8).
// Glyph placement documented choice: telemetry zone leading column, not flight zone edge,
// because the trend relates to telemetry data and accent colors contrast with text color.
static constexpr int TREND_GLYPH_WIDTH = 5;  // px wide
static constexpr int TREND_GLYPH_GAP   = 1;  // px gap between glyph and text

// ============================================================================
// ds-2.2: Vertical trend indicator helpers
// ============================================================================

// classifyVerticalTrend — pure, no hardware dependency; defined as class method for testing.
VerticalTrend LiveFlightCardMode::classifyVerticalTrend(float vrate_fps) {
    if (std::isnan(vrate_fps)) return VerticalTrend::UNKNOWN;
    if (vrate_fps >  VRATE_LEVEL_EPS) return VerticalTrend::CLIMBING;
    if (vrate_fps < -VRATE_LEVEL_EPS) return VerticalTrend::DESCENDING;
    return VerticalTrend::LEVEL;
}

// drawVerticalTrend — file-static (hardware-dependent; not declared in header).
// Draws a 5×5 directional glyph centered vertically in [zoneY, zoneY+zoneH) at column x.
// Colors: CLIMBING=green, DESCENDING=red, LEVEL=amber, UNKNOWN=no-op (AC #6).
// Pixels clipped to zone bounds for safety on very short zones.
static void drawVerticalTrend(Adafruit_GFX* matrix, VerticalTrend trend,
                               int16_t x, int16_t zoneY, int16_t zoneH) {
    if (trend == VerticalTrend::UNKNOWN) return;
    if (zoneH <= 0) return;

    uint16_t color;
    if      (trend == VerticalTrend::CLIMBING)   color = DisplayUtils::rgb565(0, 220, 0);    // green
    else if (trend == VerticalTrend::DESCENDING) color = DisplayUtils::rgb565(220, 0, 0);    // red
    else                                         color = DisplayUtils::rgb565(220, 140, 0);    // amber

    // 5×5 glyph patterns (static const → .rodata, no stack/heap cost).
    // Up-arrow (climbing): apex at top, shaft below
    // Down-arrow (descending): shaft at top, apex at bottom
    // Level-bar: single horizontal bar across centre row
    static const uint8_t CLIMB_PATTERN[5][5] = {
        {0,0,1,0,0},
        {0,1,1,1,0},
        {1,1,1,1,1},
        {0,0,1,0,0},
        {0,0,1,0,0}
    };
    static const uint8_t DESCEND_PATTERN[5][5] = {
        {0,0,1,0,0},
        {0,0,1,0,0},
        {1,1,1,1,1},
        {0,1,1,1,0},
        {0,0,1,0,0}
    };
    static const uint8_t LEVEL_PATTERN[5][5] = {
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,1,1,1,1},
        {0,0,0,0,0},
        {0,0,0,0,0}
    };

    const uint8_t (*pat)[5] =
        (trend == VerticalTrend::CLIMBING)   ? CLIMB_PATTERN :
        (trend == VerticalTrend::DESCENDING) ? DESCEND_PATTERN : LEVEL_PATTERN;

    static constexpr int GLYPH_H = 5;
    const int16_t startY = zoneY + (zoneH >= GLYPH_H ? (zoneH - GLYPH_H) / 2 : 0);

    for (int row = 0; row < GLYPH_H; ++row) {
        const int16_t py = startY + row;
        if (py < zoneY || py >= zoneY + zoneH) continue;  // clip to zone
        for (int col = 0; col < TREND_GLYPH_WIDTH; ++col) {
            if (pat[row][col]) {
                matrix->drawPixel(x + col, py, color);
            }
        }
    }
}

// ============================================================================
// ds-2.2: Priority field selection for telemetry zone
// ============================================================================

// computeTelemetryFields — pure, no hardware dependency; defined as class method for testing.
// Given available display lines and text columns (after reserving glyph space),
// returns which fields to render using strict priority drop order (AC #1):
//   lowest priority (first to drop): vrate → hdg → spd → alt
//
// Two-line layout thresholds (per-line, each line holds a pair or single value):
//   MIN_COLS_PAIR   = 7: "Aval Sval" or "Tval Vval" abbreviated minimum (~7 chars)
//   MIN_COLS_SINGLE = 3: "Aval" abbreviated minimum (~3 chars)
//
// One-line compact thresholds (all enabled fields packed into one line):
//   >=15 cols: all 4 fields  (min "A-- S-- T-- V--" = 15 chars)
//   >=11 cols: alt+spd+hdg   (min "A-- S-- T--"     = 11 chars)
//   >= 7 cols: alt+spd       (min "A-- S--"          = 7 chars)
//   >= 3 cols: alt only      (min "A--"              = 3 chars)
TelemetryFieldSet LiveFlightCardMode::computeTelemetryFields(int linesAvailable, int textCols) {
    if (linesAvailable <= 0 || textCols <= 0) return {false, false, false, false};

    if (linesAvailable >= 2) {
        // Two-line: line1 = alt[+spd], line2 = hdg[+vrate]
        const bool canFitPair   = (textCols >= 7);
        const bool canFitSingle = (textCols >= 3);
        return {
            /* alt   */ canFitSingle,
            /* spd   */ canFitPair,
            /* hdg   */ canFitSingle,
            /* vrate */ canFitPair
        };
    }

    // One-line compact
    const bool vrate = (textCols >= 15);
    const bool hdg   = (textCols >= 11);
    const bool spd   = (textCols >= 7);
    const bool alt   = (textCols >= 3);
    return {alt, spd, hdg, vrate};
}

// Clamp a zone rect to matrix dimensions — prevents out-of-bounds draw calls
// from malformed RenderContext (mirrors ClassicCardMode review item ds-1.4 #4).
static Rect clampZone(const Rect& zone, uint16_t matW, uint16_t matH) {
    Rect c = zone;
    if (c.x >= matW || c.y >= matH) { c.x = 0; c.y = 0; c.w = 0; c.h = 0; return c; }
    if (c.x + c.w > matW) c.w = matW - c.x;
    if (c.y + c.h > matH) c.h = matH - c.y;
    return c;
}

bool LiveFlightCardMode::init(const RenderContext& ctx) {
    (void)ctx;
    _currentFlightIndex = 0;
    _lastCycleMs = 0;
    return true;
}

void LiveFlightCardMode::teardown() {
    _currentFlightIndex = 0;
    _lastCycleMs = 0;
}

void LiveFlightCardMode::render(const RenderContext& ctx,
                                const std::vector<FlightInfo>& flights) {
    // --- Cycling state update (pure logic, no matrix needed) ---
    // Moved before matrix null guard so tests can verify cycling without hardware.
    // Multi-step catch-up prevents rapid strobing after long pauses (OTA, WiFi reconnect).
    if (!flights.empty()) {
        if (flights.size() > 1) {
            const unsigned long now = millis();
            if (_lastCycleMs == 0) {
                // First render after init (or after returning from ≤1 flight) — anchor timer
                _lastCycleMs = now;
            }
            if (ctx.displayCycleMs > 0 && now - _lastCycleMs >= ctx.displayCycleMs) {
                const unsigned long steps =
                    (now - _lastCycleMs) / (unsigned long)ctx.displayCycleMs;
                _currentFlightIndex = (_currentFlightIndex + (size_t)steps) % flights.size();
                _lastCycleMs += steps * (unsigned long)ctx.displayCycleMs;
            }
        } else {
            _currentFlightIndex = 0;
            _lastCycleMs = 0;  // Reset so next multi-flight entry gets a fresh anchor
        }
    } else {
        _lastCycleMs = 0;  // Reset so returning flights don't strobe on first frames
    }

    // --- Matrix guard: all draw calls below require a valid matrix ---
    if (ctx.matrix == nullptr) return;

    ctx.matrix->fillScreen(0);

    if (!flights.empty()) {
        const size_t index = _currentFlightIndex % flights.size();

        if (ctx.layout.valid) {
            renderZoneFlight(ctx, flights[index]);
        } else {
            renderSingleFlightCard(ctx, flights[index]);
        }
    } else {
        renderLoadingScreen(ctx);
    }
    // No FastLED.show() — frame commit is the pipeline's responsibility
}

const char* LiveFlightCardMode::getName() const {
    return "Live Flight Card";
}

const ModeZoneDescriptor& LiveFlightCardMode::getZoneDescriptor() const {
    return _descriptor;
}

const ModeSettingsSchema* LiveFlightCardMode::getSettingsSchema() const {
    return nullptr;  // No per-mode settings until Delight release
}

// --- Zone rendering ---

void LiveFlightCardMode::renderZoneFlight(const RenderContext& ctx, const FlightInfo& f) {
    // Clamp zones against matrix dimensions (mirrors ClassicCardMode)
    uint16_t mw = ctx.layout.matrixWidth ? ctx.layout.matrixWidth : ctx.matrix->width();
    uint16_t mh = ctx.layout.matrixHeight ? ctx.layout.matrixHeight : ctx.matrix->height();

    Rect logo     = clampZone(ctx.layout.logoZone,      mw, mh);
    Rect flight   = clampZone(ctx.layout.flightZone,    mw, mh);
    Rect telemetry = clampZone(ctx.layout.telemetryZone, mw, mh);

    if (logo.w > 0 && logo.h > 0)      renderLogoZone(ctx, f, logo);
    if (flight.w > 0 && flight.h > 0)  renderFlightZone(ctx, f, flight);
    if (telemetry.w > 0 && telemetry.h > 0) renderTelemetryZone(ctx, f, telemetry);
}

void LiveFlightCardMode::renderLogoZone(const RenderContext& ctx, const FlightInfo& f,
                                         const Rect& zone) {
    if (ctx.logoBuffer == nullptr) return;  // No buffer supplied — skip logo zone silently
    LogoManager::loadLogo(f.operator_icao, ctx.logoBuffer);
    // Logo renders flush with its zone — the universal panel frame and
    // LayoutEngine's 1 px inset already separate the logo from the edge.
    DisplayUtils::drawBitmapRGB565(ctx.matrix, zone.x, zone.y,
                                    LOGO_WIDTH, LOGO_HEIGHT,
                                    ctx.logoBuffer, zone.w, zone.h);
}

void LiveFlightCardMode::renderFlightZone(const RenderContext& ctx, const FlightInfo& f,
                                            const Rect& zone) {
    const int maxCols = zone.w / CHAR_WIDTH;
    if (maxCols <= 0) return;

    int linesAvailable = zone.h / CHAR_HEIGHT;
    if (linesAvailable <= 0) return;

    // Airline name precedence: full display name > IATA > ICAO > operator_code
    const char* airline = f.airline_display_name_full.length() ? f.airline_display_name_full.c_str()
                         : (f.operator_iata.length() ? f.operator_iata.c_str()
                         : (f.operator_icao.length() ? f.operator_icao.c_str()
                         : f.operator_code.c_str()));

    // Route string: prefer IATA (DCA, JFK — no K prefix) with ICAO fallback
    // (KDCA, KJFK) when IATA is absent, so non-US airports and partially-
    // enriched flights still render something.
    char route[LINE_BUF_SIZE] = "";
    const char* originCode = f.origin.code_iata.length() > 0
        ? f.origin.code_iata.c_str() : f.origin.code_icao.c_str();
    const char* destCode = f.destination.code_iata.length() > 0
        ? f.destination.code_iata.c_str() : f.destination.code_icao.c_str();
    if (originCode[0] != '\0' || destCode[0] != '\0') {
        snprintf(route, sizeof(route), "%s>%s", originCode, destCode);
    }

    char line1[LINE_BUF_SIZE] = "";
    char line2[LINE_BUF_SIZE] = "";
    int linesToDraw = 1;

    if (linesAvailable >= 2) {
        // Two-line: airline on line 1, route on line 2
        DisplayUtils::truncateToColumns(airline, maxCols, line1, sizeof(line1));
        DisplayUtils::truncateToColumns(route, maxCols, line2, sizeof(line2));
        linesToDraw = (strlen(line2) > 0) ? 2 : 1;
    } else {
        // One-line: airline only (route dropped, airline is minimum viable display)
        DisplayUtils::truncateToColumns(airline, maxCols, line1, sizeof(line1));
    }

    int totalTextH = linesToDraw * CHAR_HEIGHT;
    int16_t y = zone.y + (zone.h - totalTextH) / 2;

    DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, line1, ctx.textColor);
    if (linesToDraw >= 2 && strlen(line2) > 0) {
        y += CHAR_HEIGHT;
        DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, line2, ctx.textColor);
    }
}

void LiveFlightCardMode::renderTelemetryZone(const RenderContext& ctx, const FlightInfo& f,
                                               const Rect& zone) {
    // --- ds-2.2: Vertical trend glyph (AC #4-8) ---
    // Draw glyph at leading column regardless of whether numeric vrate is shown (AC #7).
    // UNKNOWN (NaN vrate) → no glyph drawn (AC #6).
    const VerticalTrend trend = classifyVerticalTrend(static_cast<float>(f.vertical_rate_fps));
    if (zone.w >= TREND_GLYPH_WIDTH && zone.h > 0) {
        drawVerticalTrend(ctx.matrix, trend, zone.x, zone.y, zone.h);
    }

    // --- ds-2.2: Adaptive text layout (AC #1-3) ---
    // Text column starts after the glyph reservation.
    const int16_t textX = static_cast<int16_t>(zone.x + TREND_GLYPH_WIDTH + TREND_GLYPH_GAP);
    const int textW = static_cast<int>(zone.w) - TREND_GLYPH_WIDTH - TREND_GLYPH_GAP;
    if (textW <= 0) return;

    const int textCols      = textW / CHAR_WIDTH;
    const int linesAvailable = zone.h / CHAR_HEIGHT;
    if (textCols <= 0 || linesAvailable <= 0) return;

    // Compute which fields fit given available lines and text columns.
    // Drop order (lowest priority first): vrate → hdg → spd → alt (AC #1).
    const TelemetryFieldSet fs = computeTelemetryFields(linesAvailable, textCols);

    // Format only the enabled fields to avoid unnecessary snprintf calls.
    char alt[16] = "", spd[16] = "", trk[16] = "", vr[16] = "";
    if (fs.alt)   DisplayUtils::formatTelemetryValue(f.altitude_kft,       "kft", 1, alt, sizeof(alt));
    if (fs.spd)   DisplayUtils::formatTelemetryValue(f.speed_mph,          "mph", 0, spd, sizeof(spd));
    if (fs.hdg)   DisplayUtils::formatTelemetryValue(f.track_deg,          "d",   0, trk, sizeof(trk));
    if (fs.vrate) DisplayUtils::formatTelemetryValue(f.vertical_rate_fps,  "fps", 0, vr,  sizeof(vr));

    char telLine1[LINE_BUF_SIZE] = "";
    char telLine2[LINE_BUF_SIZE] = "";
    // combined is reused for both lines to avoid double-buffering (stack budget AC #10)
    char combined[LINE_BUF_SIZE] = "";

    if (linesAvailable >= 2) {
        // Two-line enriched: line 1 = alt[+spd], line 2 = hdg[+vrate]
        if (fs.alt && fs.spd) {
            snprintf(combined, sizeof(combined), "%s %s", alt, spd);
        } else if (fs.alt) {
            snprintf(combined, sizeof(combined), "%s", alt);
        }
        DisplayUtils::truncateToColumns(combined, textCols, telLine1, sizeof(telLine1));

        // Build line 2 (reuse combined)
        combined[0] = '\0';
        if (fs.hdg && fs.vrate) {
            snprintf(combined, sizeof(combined), "%s %s", trk, vr);
        } else if (fs.hdg) {
            snprintf(combined, sizeof(combined), "%s", trk);
        } else if (fs.vrate) {
            snprintf(combined, sizeof(combined), "%s", vr);
        }
        if (combined[0] != '\0') {
            DisplayUtils::truncateToColumns(combined, textCols, telLine2, sizeof(telLine2));
        }
    } else {
        // Compact single-line: pack only enabled fields with "X%s" prefix notation.
        // Fields omitted here are dropped by computeTelemetryFields — not truncated.
        int pos = 0;
        if (fs.alt) {
            int n = snprintf(combined + pos, LINE_BUF_SIZE - (size_t)pos, "A%s", alt);
            if (n > 0) pos += n;
        }
        if (fs.spd && pos < (int)LINE_BUF_SIZE - 1) {
            int n = snprintf(combined + pos, LINE_BUF_SIZE - (size_t)pos,
                             "%sS%s", pos > 0 ? " " : "", spd);
            if (n > 0) pos += n;
        }
        if (fs.hdg && pos < (int)LINE_BUF_SIZE - 1) {
            int n = snprintf(combined + pos, LINE_BUF_SIZE - (size_t)pos,
                             "%sT%s", pos > 0 ? " " : "", trk);
            if (n > 0) pos += n;
        }
        if (fs.vrate && pos < (int)LINE_BUF_SIZE - 1) {
            snprintf(combined + pos, LINE_BUF_SIZE - (size_t)pos,
                     "%sV%s", pos > 0 ? " " : "", vr);
        }
        DisplayUtils::truncateToColumns(combined, textCols, telLine1, sizeof(telLine1));
    }

    // Reflow: draw only lines that have content — no intentional blank gaps (AC #2).
    const int linesToDraw = (linesAvailable >= 2 && strlen(telLine2) > 0) ? 2 : 1;
    const int totalTextH  = linesToDraw * CHAR_HEIGHT;
    int16_t y = static_cast<int16_t>(zone.y + (zone.h - totalTextH) / 2);

    if (strlen(telLine1) > 0) {
        DisplayUtils::drawTextLine(ctx.matrix, textX, y, telLine1, ctx.textColor);
    }
    if (linesToDraw >= 2) {
        y = static_cast<int16_t>(y + CHAR_HEIGHT);
        DisplayUtils::drawTextLine(ctx.matrix, textX, y, telLine2, ctx.textColor);
    }
}

// --- Fallback full-screen card (mirrored from ClassicCardMode) ---

void LiveFlightCardMode::renderSingleFlightCard(const RenderContext& ctx, const FlightInfo& f) {
    uint16_t mw = ctx.layout.matrixWidth;
    uint16_t mh = ctx.layout.matrixHeight;
    if (mw == 0) mw = ctx.matrix->width();
    if (mh == 0) mh = ctx.matrix->height();

    // Border
    ctx.matrix->drawRect(0, 0, mw, mh, ctx.textColor);

    const int padding = 2;
    const int innerWidth  = mw - 2 - (2 * padding);
    const int innerHeight = mh - 2 - (2 * padding);
    if (innerWidth <= 0 || innerHeight <= 0) return;  // Matrix too small for bordered card
    const int maxCols = innerWidth / CHAR_WIDTH;
    if (maxCols <= 0) return;

    const char* airlineSrc = f.airline_display_name_full.length() ? f.airline_display_name_full.c_str()
                             : (f.operator_iata.length() ? f.operator_iata.c_str()
                             : (f.operator_icao.length() ? f.operator_icao.c_str()
                             : f.operator_code.c_str()));

    // IATA-first route (see main render() for rationale).
    char route[LINE_BUF_SIZE] = "";
    const char* originCode2 = f.origin.code_iata.length() > 0
        ? f.origin.code_iata.c_str() : f.origin.code_icao.c_str();
    const char* destCode2 = f.destination.code_iata.length() > 0
        ? f.destination.code_iata.c_str() : f.destination.code_icao.c_str();
    if (originCode2[0] != '\0' || destCode2[0] != '\0') {
        snprintf(route, sizeof(route), "%s>%s", originCode2, destCode2);
    }

    // Enriched telemetry summary: all four fields in compact abbreviated form
    // (ds-2.1 distinguishes itself by showing heading + vrate; must appear in fallback too)
    char alt[16], spd[16], trk[16], vr[16];
    DisplayUtils::formatTelemetryValue(f.altitude_kft,       "k",  0, alt, sizeof(alt));
    DisplayUtils::formatTelemetryValue(f.speed_mph,          "",   0, spd, sizeof(spd));
    DisplayUtils::formatTelemetryValue(f.track_deg,          "d",  0, trk, sizeof(trk));
    DisplayUtils::formatTelemetryValue(f.vertical_rate_fps,  "",   0, vr,  sizeof(vr));

    char telStr[LINE_BUF_SIZE];
    snprintf(telStr, sizeof(telStr), "A%s S%s T%s V%s", alt, spd, trk, vr);

    char line1[LINE_BUF_SIZE], line2[LINE_BUF_SIZE], line3[LINE_BUF_SIZE];
    DisplayUtils::truncateToColumns(airlineSrc, maxCols, line1, sizeof(line1));
    DisplayUtils::truncateToColumns(route,      maxCols, line2, sizeof(line2));
    DisplayUtils::truncateToColumns(telStr,     maxCols, line3, sizeof(line3));

    const int lineCount = 3;
    const int lineSpacing = 1;
    const int totalTextHeight = lineCount * CHAR_HEIGHT + (lineCount - 1) * lineSpacing;
    const int topOffset = 1 + padding + (innerHeight - totalTextHeight) / 2;
    const int16_t startX = 1 + padding;

    int16_t y = topOffset;
    DisplayUtils::drawTextLine(ctx.matrix, startX, y, line1, ctx.textColor);
    y += CHAR_HEIGHT + lineSpacing;
    DisplayUtils::drawTextLine(ctx.matrix, startX, y, line2, ctx.textColor);
    y += CHAR_HEIGHT + lineSpacing;
    DisplayUtils::drawTextLine(ctx.matrix, startX, y, line3, ctx.textColor);
}

// --- Idle/loading screen (mirrored from ClassicCardMode) ---

void LiveFlightCardMode::renderLoadingScreen(const RenderContext& ctx) {
    uint16_t mw = ctx.layout.matrixWidth;
    uint16_t mh = ctx.layout.matrixHeight;
    if (mw == 0) mw = ctx.matrix->width();
    if (mh == 0) mh = ctx.matrix->height();

    // White border (matches legacy loading screen)
    const uint16_t borderColor = DisplayUtils::rgb565(255, 255, 255);
    ctx.matrix->drawRect(0, 0, mw, mh, borderColor);

    // Centered "..." text using user theme color
    const int textWidth = 3 * CHAR_WIDTH;
    const int16_t x = (mw - textWidth) / 2;
    const int16_t y = (mh - CHAR_HEIGHT) / 2 - 2;

    DisplayUtils::drawTextLine(ctx.matrix, x, y, "...", ctx.textColor);
    // No FastLED.show() — pipeline responsibility
}
