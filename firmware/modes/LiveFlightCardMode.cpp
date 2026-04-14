/*
Purpose: LiveFlightCardMode implementation — enriched telemetry flight card
         rendering via the DisplayMode system.
Story ds-2.1: replaces ds-1.3 stub with real rendering via DisplayUtils + RenderContext.
Story ds-2.2: adaptive field dropping + vertical direction indicator.
Sources: ClassicCardMode (ds-1.4) for cycling/idle/fallback patterns;
         NeoMatrixDisplay::renderFlightZone, renderTelemetryZone for telemetry layout.
*/

#include "modes/LiveFlightCardMode.h"
#include "utils/DisplayUtils.h"
#include "core/LogoManager.h"

#include <cmath>

// Zone descriptors for Mode Picker UI — reflect real LayoutEngine zones:
// Logo (left strip), Flight (center-right, top portion), Telemetry (center-right, bottom)
const ZoneRegion LiveFlightCardMode::_zones[] = {
    {"Logo",      0,  0, 25, 100},
    {"Flight",   25,  0, 75,  50},
    {"Telemetry",25, 50, 75,  50}
};

const ModeZoneDescriptor LiveFlightCardMode::_descriptor = {
    "Live flight card with adaptive layout, telemetry overlay, and vertical trend indicator",
    _zones,
    3
};

// Character dimensions for default 5x7 Adafruit GFX font (+1px spacing)
static constexpr int CHAR_WIDTH = 6;
static constexpr int CHAR_HEIGHT = 8;

// Vertical rate level-flight epsilon threshold (feet per second).
// Values within +/- this band are treated as level flight.
// 0.5 fps ~= 30 fpm — below typical instrument precision.
static constexpr float VRATE_LEVEL_EPS = 0.5f;

// Width in pixels reserved for the vertical trend indicator glyph
static constexpr int TREND_GLYPH_WIDTH = 5;

// --- Vertical Trend Indicator ---
// Draws a small directional glyph (up-arrow / down-arrow / level-bar)
// in the telemetry zone to show climb/descend/level state.
// Uses accent colors for contrast: green=climb, red=descend, amber=level.
// Placed at the leading column of the specified position.
namespace {

enum class VerticalTrend : uint8_t { CLIMB, DESCEND, LEVEL, UNKNOWN };

VerticalTrend classifyVerticalTrend(float vrate_fps) {
    if (std::isnan(vrate_fps)) return VerticalTrend::UNKNOWN;
    if (vrate_fps > VRATE_LEVEL_EPS) return VerticalTrend::CLIMB;
    if (vrate_fps < -VRATE_LEVEL_EPS) return VerticalTrend::DESCEND;
    return VerticalTrend::LEVEL;
}

void drawVerticalTrend(FastLED_NeoMatrix* matrix, int16_t x, int16_t y,
                       VerticalTrend trend) {
    // Glyph is 5px wide x 7px tall (fits in one character cell).
    // Up-arrow (climb):    centered triangle pointing up
    // Down-arrow (descend): centered triangle pointing down
    // Level bar:           horizontal line in center
    uint16_t color;
    switch (trend) {
        case VerticalTrend::CLIMB:
            color = matrix->Color(0, 255, 0);    // green
            // Up-arrow: 5-wide triangle
            matrix->drawPixel(x + 2, y,     color);  // tip
            matrix->drawPixel(x + 1, y + 1, color);
            matrix->drawPixel(x + 2, y + 1, color);
            matrix->drawPixel(x + 3, y + 1, color);
            matrix->drawPixel(x,     y + 2, color);
            matrix->drawPixel(x + 1, y + 2, color);
            matrix->drawPixel(x + 2, y + 2, color);
            matrix->drawPixel(x + 3, y + 2, color);
            matrix->drawPixel(x + 4, y + 2, color);
            // Stem
            matrix->drawPixel(x + 2, y + 3, color);
            matrix->drawPixel(x + 2, y + 4, color);
            matrix->drawPixel(x + 2, y + 5, color);
            matrix->drawPixel(x + 2, y + 6, color);
            break;

        case VerticalTrend::DESCEND:
            color = matrix->Color(255, 0, 0);    // red
            // Down-arrow: inverted triangle
            matrix->drawPixel(x + 2, y,     color);  // stem top
            matrix->drawPixel(x + 2, y + 1, color);
            matrix->drawPixel(x + 2, y + 2, color);
            matrix->drawPixel(x + 2, y + 3, color);
            matrix->drawPixel(x,     y + 4, color);
            matrix->drawPixel(x + 1, y + 4, color);
            matrix->drawPixel(x + 2, y + 4, color);
            matrix->drawPixel(x + 3, y + 4, color);
            matrix->drawPixel(x + 4, y + 4, color);
            matrix->drawPixel(x + 1, y + 5, color);
            matrix->drawPixel(x + 2, y + 5, color);
            matrix->drawPixel(x + 3, y + 5, color);
            matrix->drawPixel(x + 2, y + 6, color);  // tip
            break;

        case VerticalTrend::LEVEL:
            color = matrix->Color(255, 191, 0);  // amber
            // Horizontal bar centered vertically
            for (int i = 0; i < 5; ++i) {
                matrix->drawPixel(x + i, y + 3, color);
            }
            break;

        case VerticalTrend::UNKNOWN:
            // No glyph when data is unavailable
            break;
    }
}

// Bitfield flags for which telemetry fields are visible
static constexpr uint8_t FIELD_ALTITUDE = 0x01;
static constexpr uint8_t FIELD_SPEED    = 0x02;
static constexpr uint8_t FIELD_HEADING  = 0x04;
static constexpr uint8_t FIELD_VRATE    = 0x08;

// Compute which telemetry fields fit in the available zone.
// Drop order (lowest priority first): vrate -> heading -> speed -> altitude
// Returns a bitmask of FIELD_* constants.
uint8_t computeTelemetryFields(int linesAvailable, int maxCols) {
    if (linesAvailable <= 0 || maxCols <= 0) return 0;

    if (linesAvailable >= 2) {
        // Two lines: all four fields fit (alt+spd line 1, hdg+vrate line 2)
        return FIELD_ALTITUDE | FIELD_SPEED | FIELD_HEADING | FIELD_VRATE;
    }

    // Single line: try fitting fields left-to-right, dropping lowest priority first.
    // Each formatted field is ~4-8 chars. We try progressively smaller sets.
    // Rough field widths: alt ~7 ("35.0kft"), spd ~6 ("450mph"), hdg ~4 ("90d"), vr ~5 ("0fps")
    // With separating spaces: all4 ~25, no_vr ~19, no_vr_hdg ~14, no_vr_hdg_spd ~7
    // We use conservative estimates — if columns are tight, drop more fields.

    // Try all four on one line (very unlikely on compact displays)
    if (maxCols >= 25) return FIELD_ALTITUDE | FIELD_SPEED | FIELD_HEADING | FIELD_VRATE;
    // Drop vrate
    if (maxCols >= 19) return FIELD_ALTITUDE | FIELD_SPEED | FIELD_HEADING;
    // Drop heading
    if (maxCols >= 14) return FIELD_ALTITUDE | FIELD_SPEED;
    // Drop speed
    if (maxCols >= 7) return FIELD_ALTITUDE;
    // Nothing fits reasonably
    return 0;
}

} // anonymous namespace

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
    if (ctx.matrix == nullptr) return;

    ctx.matrix->fillScreen(0);

    if (!flights.empty()) {
        // Cycling logic — mirrored from ClassicCardMode
        const unsigned long now = millis();

        if (flights.size() > 1) {
            if (_lastCycleMs == 0) {
                // First render after init — anchor the cycle timer
                _lastCycleMs = now;
            }
            if (now - _lastCycleMs >= ctx.displayCycleMs) {
                _lastCycleMs = now;
                _currentFlightIndex = (_currentFlightIndex + 1) % flights.size();
            }
        } else {
            _currentFlightIndex = 0;
        }

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
    renderLogoZone(ctx, f, ctx.layout.logoZone);
    renderFlightZone(ctx, f, ctx.layout.flightZone);
    renderTelemetryZone(ctx, f, ctx.layout.telemetryZone);
}

void LiveFlightCardMode::renderLogoZone(const RenderContext& ctx, const FlightInfo& f,
                                         const Rect& zone) {
    LogoManager::loadLogo(f.operator_icao, ctx.logoBuffer);
    DisplayUtils::drawBitmapRGB565(ctx.matrix, zone.x, zone.y,
                                    LOGO_WIDTH, LOGO_HEIGHT,
                                    ctx.logoBuffer, zone.w, zone.h);
}

void LiveFlightCardMode::renderFlightZone(const RenderContext& ctx, const FlightInfo& f,
                                            const Rect& zone) {
    const int maxCols = zone.w / CHAR_WIDTH;
    if (maxCols <= 0) return;

    // Airline name precedence: full display name > IATA > ICAO > operator_code
    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                     : (f.operator_iata.length() ? f.operator_iata
                     : (f.operator_icao.length() ? f.operator_icao : f.operator_code));

    int linesAvailable = zone.h / CHAR_HEIGHT;
    if (linesAvailable <= 0) return;

    // Adaptive field dropping (AC #1-#3):
    // Flight zone contains: airline (priority 6, never dropped) + route (priority 5).
    // Route is dropped when only 1 line fits in the flight zone.
    // Airline is always shown — it is the minimum viable display (AC #3).

    String line1;
    String line2;
    int linesToDraw = 1;

    if (linesAvailable >= 2) {
        // Two-line: airline on line 1, route on line 2
        String route = f.origin.code_icao + String(">") + f.destination.code_icao;
        line1 = DisplayUtils::truncateToColumns(airline, maxCols);
        line2 = DisplayUtils::truncateToColumns(route, maxCols);
        linesToDraw = line2.length() > 0 ? 2 : 1;
    } else {
        // One-line: airline only (route dropped per priority order)
        // AC #3: airline must always remain visible
        line1 = DisplayUtils::truncateToColumns(airline, maxCols);
    }

    int totalTextH = linesToDraw * CHAR_HEIGHT;
    int16_t y = zone.y + (zone.h - totalTextH) / 2;

    DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, line1, ctx.textColor);
    if (linesToDraw >= 2 && line2.length() > 0) {
        y += CHAR_HEIGHT;
        DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, line2, ctx.textColor);
    }
}

void LiveFlightCardMode::renderTelemetryZone(const RenderContext& ctx, const FlightInfo& f,
                                               const Rect& zone) {
    const int maxCols = zone.w / CHAR_WIDTH;
    if (maxCols <= 0) return;

    int linesAvailable = zone.h / CHAR_HEIGHT;
    if (linesAvailable <= 0) return;

    // Classify vertical trend — always computed even if numeric vrate is dropped (AC #7)
    VerticalTrend trend = classifyVerticalTrend(static_cast<float>(f.vertical_rate_fps));

    // Reserve leading column for trend indicator if we have trend data (AC #8)
    bool showTrendGlyph = (trend != VerticalTrend::UNKNOWN);
    int trendOffset = showTrendGlyph ? (TREND_GLYPH_WIDTH + 1) : 0;  // +1px gap
    int textCols = (zone.w - trendOffset) / CHAR_WIDTH;
    if (textCols <= 0) textCols = 1;

    // Compute which telemetry fields fit (adaptive dropping per AC #1)
    uint8_t fields = computeTelemetryFields(linesAvailable, textCols);

    String telLine1, telLine2;

    if (linesAvailable >= 2 && fields == (FIELD_ALTITUDE | FIELD_SPEED | FIELD_HEADING | FIELD_VRATE)) {
        // Two-line enriched: altitude+speed on line 1, heading+vrate on line 2
        String alt = DisplayUtils::formatTelemetryValue(f.altitude_kft, "kft", 1);
        String spd = DisplayUtils::formatTelemetryValue(f.speed_mph, "mph");
        telLine1 = DisplayUtils::truncateToColumns(alt + " " + spd, textCols);

        String trk = DisplayUtils::formatTelemetryValue(f.track_deg, "d");
        String vr = DisplayUtils::formatTelemetryValue(f.vertical_rate_fps, "fps");
        telLine2 = DisplayUtils::truncateToColumns(trk + " " + vr, textCols);
    } else {
        // Single-line adaptive: build string from highest-priority fields that fit
        String parts;
        if (fields & FIELD_ALTITUDE) {
            String alt = DisplayUtils::formatTelemetryValue(f.altitude_kft, "kft", 1);
            parts += alt;
        }
        if (fields & FIELD_SPEED) {
            if (parts.length()) parts += " ";
            String spd = DisplayUtils::formatTelemetryValue(f.speed_mph, "mph");
            parts += spd;
        }
        if (fields & FIELD_HEADING) {
            if (parts.length()) parts += " ";
            String trk = DisplayUtils::formatTelemetryValue(f.track_deg, "d");
            parts += trk;
        }
        if (fields & FIELD_VRATE) {
            if (parts.length()) parts += " ";
            String vr = DisplayUtils::formatTelemetryValue(f.vertical_rate_fps, "fps");
            parts += vr;
        }
        telLine1 = DisplayUtils::truncateToColumns(parts, textCols);
    }

    // Compute vertical centering
    int linesToDraw = (linesAvailable >= 2 && telLine2.length() > 0) ? 2 : 1;
    int totalTextH = linesToDraw * CHAR_HEIGHT;
    int16_t y = zone.y + (zone.h - totalTextH) / 2;
    int16_t textX = zone.x + trendOffset;

    // Draw trend indicator in leading column (AC #4-#8)
    if (showTrendGlyph) {
        // Vertically center the glyph alongside the text block
        int16_t glyphY = y;  // align with first text line
        drawVerticalTrend(ctx.matrix, zone.x, glyphY, trend);
    }

    // Draw text
    DisplayUtils::drawTextLine(ctx.matrix, textX, y, telLine1, ctx.textColor);
    if (linesToDraw >= 2) {
        y += CHAR_HEIGHT;
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
    const int innerWidth = mw - 2 - (2 * padding);
    const int maxCols = innerWidth / CHAR_WIDTH;

    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                     : (f.operator_iata.length() ? f.operator_iata
                     : (f.operator_icao.length() ? f.operator_icao : f.operator_code));
    String line2 = f.origin.code_icao + String(">") + f.destination.code_icao;
    String line3 = DisplayUtils::formatTelemetryValue(f.altitude_kft, "kft", 1)
                   + " " + DisplayUtils::formatTelemetryValue(f.speed_mph, "mph");

    String line1 = DisplayUtils::truncateToColumns(airline, maxCols);
    line2 = DisplayUtils::truncateToColumns(line2, maxCols);
    line3 = DisplayUtils::truncateToColumns(line3, maxCols);

    const int innerHeight = mh - 2 - (2 * padding);
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
    const uint16_t borderColor = ctx.matrix->Color(255, 255, 255);
    ctx.matrix->drawRect(0, 0, mw, mh, borderColor);

    // Centered "..." text using user theme color
    const char loadingText[] = "...";
    const int textWidth = 3 * CHAR_WIDTH;
    const int16_t x = (mw - textWidth) / 2;
    const int16_t y = (mh - CHAR_HEIGHT) / 2 - 2;

    DisplayUtils::drawTextLine(ctx.matrix, x, y, String(loadingText), ctx.textColor);
    // No FastLED.show() — pipeline responsibility
}
