/*
Purpose: ClassicCardMode implementation — pixel-identical port of NeoMatrixDisplay
         zone-based classic layout into the DisplayMode system.
Story ds-1.4: replaces ds-1.3 stub with real rendering via DisplayUtils + RenderContext.
Sources: NeoMatrixDisplay::renderZoneFlight, displayFlights, displaySingleFlightCard,
         displayLoadingScreen — ported to use ctx.matrix, ctx.layout, ctx.textColor,
         ctx.logoBuffer, ctx.displayCycleMs instead of ConfigManager / private members.
*/

#include "modes/ClassicCardMode.h"
#include "utils/DisplayUtils.h"
#include "core/LogoManager.h"

// Zone descriptors for Mode Picker UI — reflect real LayoutEngine zones:
// Logo (left strip), Flight (center-right, top portion), Telemetry (center-right, bottom)
const ZoneRegion ClassicCardMode::_zones[] = {
    {"Logo",      0,  0, 25, 100},
    {"Flight",   25,  0, 75,  60},
    {"Telemetry",25, 60, 75,  40}
};

const ModeZoneDescriptor ClassicCardMode::_descriptor = {
    "Classic card layout with airline logo, flight info, and telemetry",
    _zones,
    3
};

// Character dimensions for default 5x7 Adafruit GFX font (+1px spacing)
static constexpr int CHAR_WIDTH = 6;
static constexpr int CHAR_HEIGHT = 8;

bool ClassicCardMode::init(const RenderContext& ctx) {
    (void)ctx;
    _currentFlightIndex = 0;
    _lastCycleMs = 0;
    return true;
}

void ClassicCardMode::teardown() {
    _currentFlightIndex = 0;
    _lastCycleMs = 0;
}

void ClassicCardMode::render(const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights) {
    if (ctx.matrix == nullptr) return;

    ctx.matrix->fillScreen(0);

    if (!flights.empty()) {
        // Cycling logic — ported from NeoMatrixDisplay::displayFlights
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
    // No FastLED.show() — frame commit is the pipeline's responsibility (FR35 prep)
}

const char* ClassicCardMode::getName() const {
    return "Classic Card";
}

const ModeZoneDescriptor& ClassicCardMode::getZoneDescriptor() const {
    return _descriptor;
}

const ModeSettingsSchema* ClassicCardMode::getSettingsSchema() const {
    return nullptr;  // No per-mode settings until Delight release
}

// --- Zone rendering (ported from NeoMatrixDisplay zone methods) ---

void ClassicCardMode::renderZoneFlight(const RenderContext& ctx, const FlightInfo& f) {
    renderLogoZone(ctx, f, ctx.layout.logoZone);
    renderFlightZone(ctx, f, ctx.layout.flightZone);
    renderTelemetryZone(ctx, f, ctx.layout.telemetryZone);
}

void ClassicCardMode::renderLogoZone(const RenderContext& ctx, const FlightInfo& f,
                                      const Rect& zone) {
    // Load logo by operator ICAO; fallback sprite if not found
    LogoManager::loadLogo(f.operator_icao, ctx.logoBuffer);
    DisplayUtils::drawBitmapRGB565(ctx.matrix, zone.x, zone.y,
                                    LOGO_WIDTH, LOGO_HEIGHT,
                                    ctx.logoBuffer, zone.w, zone.h);
}

void ClassicCardMode::renderFlightZone(const RenderContext& ctx, const FlightInfo& f,
                                        const Rect& zone) {
    const int maxCols = zone.w / CHAR_WIDTH;
    if (maxCols <= 0) return;

    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                     : (f.operator_iata.length() ? f.operator_iata
                     : (f.operator_icao.length() ? f.operator_icao : f.operator_code));
    String route = f.origin.code_icao + String(">") + f.destination.code_icao;
    String aircraft = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short
                                                              : f.aircraft_code;

    int linesAvailable = zone.h / CHAR_HEIGHT;
    if (linesAvailable <= 0) return;

    String line1;
    String line2;
    String line3;
    int linesToDraw = 1;

    if (linesAvailable == 1) {
        // Compact: route priority when only one row fits
        String compactLine = route.length() ? route : airline;
        if (compactLine.length() == 0) {
            compactLine = aircraft;
        }
        line1 = DisplayUtils::truncateToColumns(compactLine, maxCols);
    } else if (linesAvailable == 2) {
        // Full: airline + route+aircraft compressed into row two
        line1 = DisplayUtils::truncateToColumns(airline, maxCols);
        String detailLine = route;
        if (aircraft.length() > 0) {
            if (detailLine.length() > 0) {
                detailLine += " ";
            }
            detailLine += aircraft;
        }
        line2 = DisplayUtils::truncateToColumns(detailLine, maxCols);
        linesToDraw = line2.length() > 0 ? 2 : 1;
    } else {
        // Expanded: three separate lines
        line1 = DisplayUtils::truncateToColumns(airline, maxCols);
        line2 = DisplayUtils::truncateToColumns(route, maxCols);
        line3 = DisplayUtils::truncateToColumns(aircraft, maxCols);
        linesToDraw = line3.length() > 0 ? 3 : (line2.length() > 0 ? 2 : 1);
    }

    int totalTextH = linesToDraw * CHAR_HEIGHT;
    int16_t y = zone.y + (zone.h - totalTextH) / 2;

    DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, line1, ctx.textColor);
    if (linesToDraw >= 2 && line2.length() > 0) {
        y += CHAR_HEIGHT;
        DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, line2, ctx.textColor);
    }
    if (linesToDraw >= 3 && line3.length() > 0) {
        y += CHAR_HEIGHT;
        DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, line3, ctx.textColor);
    }
}

void ClassicCardMode::renderTelemetryZone(const RenderContext& ctx, const FlightInfo& f,
                                            const Rect& zone) {
    const int maxCols = zone.w / CHAR_WIDTH;
    if (maxCols <= 0) return;

    int linesAvailable = zone.h / CHAR_HEIGHT;
    if (linesAvailable <= 0) return;

    String telLine1, telLine2;

    if (linesAvailable >= 2) {
        // Two-line: altitude+speed on line 1, track+vrate on line 2
        String alt = DisplayUtils::formatTelemetryValue(f.altitude_kft, "kft", 1);
        String spd = DisplayUtils::formatTelemetryValue(f.speed_mph, "mph");
        telLine1 = DisplayUtils::truncateToColumns(alt + " " + spd, maxCols);

        String trk = DisplayUtils::formatTelemetryValue(f.track_deg, "d");
        String vr = DisplayUtils::formatTelemetryValue(f.vertical_rate_fps, "fps");
        telLine2 = DisplayUtils::truncateToColumns(trk + " " + vr, maxCols);
    } else {
        // Compact: abbreviated single row
        String alt = String("A") + DisplayUtils::formatTelemetryValue(f.altitude_kft, "k", 0);
        String spd = String("S") + DisplayUtils::formatTelemetryValue(f.speed_mph, "", 0);
        String trk = String("T") + DisplayUtils::formatTelemetryValue(f.track_deg, "", 0);
        String vr = String("V") + DisplayUtils::formatTelemetryValue(f.vertical_rate_fps, "", 0);
        telLine1 = DisplayUtils::truncateToColumns(alt + " " + spd + " " + trk + " " + vr, maxCols);
    }

    int linesToDraw = (linesAvailable >= 2 && telLine2.length() > 0) ? 2 : 1;
    int totalTextH = linesToDraw * CHAR_HEIGHT;
    int16_t y = zone.y + (zone.h - totalTextH) / 2;

    DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, telLine1, ctx.textColor);
    if (linesToDraw >= 2) {
        y += CHAR_HEIGHT;
        DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, telLine2, ctx.textColor);
    }
}

// --- Fallback full-screen card (ported from NeoMatrixDisplay::displaySingleFlightCard) ---

void ClassicCardMode::renderSingleFlightCard(const RenderContext& ctx, const FlightInfo& f) {
    // Determine matrix dimensions: prefer layout fields, fall back to GFX dimensions
    uint16_t mw = ctx.layout.matrixWidth;
    uint16_t mh = ctx.layout.matrixHeight;
    if (mw == 0) mw = ctx.matrix->width();
    if (mh == 0) mh = ctx.matrix->height();

    // Border
    ctx.matrix->drawRect(0, 0, mw, mh, ctx.textColor);

    const int padding = 2;
    const int innerWidth = mw - 2 - (2 * padding);
    const int innerHeight = mh - 2 - (2 * padding);
    const int maxCols = innerWidth / CHAR_WIDTH;

    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                     : (f.operator_iata.length() ? f.operator_iata
                     : (f.operator_icao.length() ? f.operator_icao : f.operator_code));
    String line2 = f.origin.code_icao + String(">") + f.destination.code_icao;
    String line3 = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short
                                                           : f.aircraft_code;

    String line1 = DisplayUtils::truncateToColumns(airline, maxCols);
    line2 = DisplayUtils::truncateToColumns(line2, maxCols);
    line3 = DisplayUtils::truncateToColumns(line3, maxCols);

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

// --- Idle/loading screen (ported from NeoMatrixDisplay::displayLoadingScreen) ---

void ClassicCardMode::renderLoadingScreen(const RenderContext& ctx) {
    // Determine matrix dimensions
    uint16_t mw = ctx.layout.matrixWidth;
    uint16_t mh = ctx.layout.matrixHeight;
    if (mw == 0) mw = ctx.matrix->width();
    if (mh == 0) mh = ctx.matrix->height();

    // White border (matches legacy loading screen)
    const uint16_t borderColor = ctx.matrix->Color(255, 255, 255);
    ctx.matrix->drawRect(0, 0, mw, mh, borderColor);

    // Centered "..." text using user theme color
    const char loadingText[] = "...";
    const int textWidth = 3 * CHAR_WIDTH;  // 3 chars
    const int16_t x = (mw - textWidth) / 2;
    const int16_t y = (mh - CHAR_HEIGHT) / 2 - 2;

    DisplayUtils::drawTextLine(ctx.matrix, x, y, String(loadingText), ctx.textColor);
    // No FastLED.show() — pipeline responsibility
}
