/*
Purpose: ClassicCardMode implementation — pixel-identical port of NeoMatrixDisplay
         zone-based classic layout into the DisplayMode system.
Story ds-1.4: replaces ds-1.3 stub with real rendering via DisplayUtils + RenderContext.
Sources: NeoMatrixDisplay::renderZoneFlight, displayFlights, displaySingleFlightCard,
         displayLoadingScreen — ported to use ctx.matrix, ctx.layout, ctx.textColor,
         ctx.logoBuffer, ctx.displayCycleMs instead of ConfigManager / private members.

Review follow-ups addressed:
- Cycling state moved before matrix null guard for testability
- String temporaries replaced with char[] + snprintf (zero heap on hot path)
- Zone bounds clamped against matrix dimensions before draw calls
*/

#include "modes/ClassicCardMode.h"
#include "utils/DisplayUtils.h"
#include "core/LogoManager.h"

#include <Fonts/Picopixel.h>

#include <cstring>
#include <cmath>

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

// Maximum buffer size for text lines (generous for any display width)
static constexpr size_t LINE_BUF_SIZE = 48;

// Clamp a zone rect to matrix dimensions — prevents out-of-bounds draw calls
// from malformed RenderContext (review item ds-1.4 #4).
static Rect clampZone(const Rect& zone, uint16_t matW, uint16_t matH) {
    Rect c = zone;
    if (c.x >= matW || c.y >= matH) { c.x = 0; c.y = 0; c.w = 0; c.h = 0; return c; }
    if (c.x + c.w > matW) c.w = matW - c.x;
    if (c.y + c.h > matH) c.h = matH - c.y;
    return c;
}

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
    // --- Cycling state update (pure logic, no matrix needed) ---
    // Moved before matrix null guard so tests can verify cycling without hardware.
    if (!flights.empty()) {
        if (flights.size() > 1) {
            const unsigned long now = millis();
            if (_lastCycleMs == 0) {
                // First render after init (or after returning from ≤1 flight) — anchor timer
                _lastCycleMs = now;
            }
            if (ctx.displayCycleMs > 0 && now - _lastCycleMs >= ctx.displayCycleMs) {
                // Multi-step advancement: catches up all missed cycles at once, preventing
                // rapid strobing through flights after long pauses (OTA, WiFi reconnect).
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
    // Clamp zones against matrix dimensions (review item ds-1.4 #4)
    uint16_t mw = ctx.layout.matrixWidth ? ctx.layout.matrixWidth : ctx.matrix->width();
    uint16_t mh = ctx.layout.matrixHeight ? ctx.layout.matrixHeight : ctx.matrix->height();

    Rect logo = clampZone(ctx.layout.logoZone, mw, mh);
    Rect flight = clampZone(ctx.layout.flightZone, mw, mh);
    Rect telemetry = clampZone(ctx.layout.telemetryZone, mw, mh);

    if (logo.w > 0 && logo.h > 0) renderLogoZone(ctx, f, logo);
    if (flight.w > 0 && flight.h > 0) renderFlightZone(ctx, f, flight);
    if (telemetry.w > 0 && telemetry.h > 0) renderTelemetryZone(ctx, f, telemetry);
}

void ClassicCardMode::renderLogoZone(const RenderContext& ctx, const FlightInfo& f,
                                      const Rect& zone) {
    if (ctx.logoBuffer == nullptr) return;  // No buffer supplied — skip logo zone silently
    // Load logo by operator ICAO; fallback sprite if not found
    LogoManager::loadLogo(f.operator_icao, ctx.logoBuffer);
    // The universal panel frame (main.cpp) + LayoutEngine's 1 px zone inset
    // already provide the only visual gap needed. No extra padding here or
    // the top/left edges end up with 2 px of blank before the logo.
    DisplayUtils::drawBitmapRGB565(ctx.matrix, zone.x, zone.y,
                                    LOGO_WIDTH, LOGO_HEIGHT,
                                    ctx.logoBuffer, zone.w, zone.h);
}

void ClassicCardMode::renderFlightZone(const RenderContext& ctx, const FlightInfo& f,
                                        const Rect& zone) {
    const int maxCols = zone.w / CHAR_WIDTH;
    if (maxCols <= 0) return;

    int linesAvailable = zone.h / CHAR_HEIGHT;
    if (linesAvailable <= 0) return;

    // Source strings from FlightInfo — read c_str() pointers (no heap alloc)
    const char* airline = f.airline_display_name_full.length() ? f.airline_display_name_full.c_str()
                          : (f.operator_iata.length() ? f.operator_iata.c_str()
                          : (f.operator_icao.length() ? f.operator_icao.c_str()
                          : f.operator_code.c_str()));

    const char* aircraftSrc = f.aircraft_display_name_short.length()
                              ? f.aircraft_display_name_short.c_str()
                              : f.aircraft_code.c_str();

    // Build route string in fixed buffer — only if at least one code is set.
    // Prefer IATA codes (DCA, JFK, LAX — 3 letters, no K prefix for US
    // airports) and fall back to ICAO (KDCA, KJFK) only when IATA is absent
    // (e.g. some non-US airports or missing enrichment data).
    // Initialise to "" so fallback logic works correctly for flights with no origin/dest.
    char route[LINE_BUF_SIZE] = "";
    const char* originCode = f.origin.code_iata.length() > 0
        ? f.origin.code_iata.c_str() : f.origin.code_icao.c_str();
    const char* destCode = f.destination.code_iata.length() > 0
        ? f.destination.code_iata.c_str() : f.destination.code_icao.c_str();
    if (originCode[0] != '\0' || destCode[0] != '\0') {
        snprintf(route, sizeof(route), "%s>%s", originCode, destCode);
    }

    // Cache lengths — avoid redundant O(n) strlen() per frame at ~20fps (review item ds-1.4 #3)
    const int routeLen = (int)strlen(route);
    const int aircraftLen = (int)strlen(aircraftSrc);

    // The airline line uses Picopixel (4px advance vs default 6px) so that
    // long names like "United Airlines" / "American Airlines" fit in the
    // narrow flight zone. Other lines stay on the default 6x8 font. The
    // airline line is truncated against Picopixel's advance separately so
    // we get ~50% more horizontal capacity than the other lines.
    constexpr int AIRLINE_CHAR_W = 4;           // Picopixel xAdvance
    constexpr int AIRLINE_BASELINE_Y = 5;       // cap height offset: baseline is
                                                // ~5 px below the glyph's top
    const int airlineMaxCols = zone.w / AIRLINE_CHAR_W;

    // Truncated line buffers
    char line1[LINE_BUF_SIZE] = "";
    char line2[LINE_BUF_SIZE] = "";
    char line3[LINE_BUF_SIZE] = "";
    int linesToDraw = 1;
    bool line1IsAirline = false;

    if (linesAvailable == 1) {
        // Compact: route priority when only one row fits
        const char* compactSrc = (routeLen > 0) ? route : airline;
        if (strlen(compactSrc) == 0) compactSrc = aircraftSrc;
        if (compactSrc == airline) {
            DisplayUtils::truncateToColumns(compactSrc, airlineMaxCols, line1, sizeof(line1));
            line1IsAirline = true;
        } else {
            DisplayUtils::truncateToColumns(compactSrc, maxCols, line1, sizeof(line1));
        }
    } else if (linesAvailable == 2) {
        // Full: airline + route+aircraft compressed into row two
        DisplayUtils::truncateToColumns(airline, airlineMaxCols, line1, sizeof(line1));
        line1IsAirline = true;
        char detailLine[LINE_BUF_SIZE];
        if (aircraftLen > 0 && routeLen > 0) {
            snprintf(detailLine, sizeof(detailLine), "%s %s", route, aircraftSrc);
        } else if (aircraftLen > 0) {
            strncpy(detailLine, aircraftSrc, sizeof(detailLine) - 1);
            detailLine[sizeof(detailLine) - 1] = '\0';
        } else {
            strncpy(detailLine, route, sizeof(detailLine) - 1);
            detailLine[sizeof(detailLine) - 1] = '\0';
        }
        DisplayUtils::truncateToColumns(detailLine, maxCols, line2, sizeof(line2));
        linesToDraw = (strlen(line2) > 0) ? 2 : 1;
    } else {
        // Expanded: three separate lines
        DisplayUtils::truncateToColumns(airline, airlineMaxCols, line1, sizeof(line1));
        line1IsAirline = true;
        DisplayUtils::truncateToColumns(route, maxCols, line2, sizeof(line2));
        DisplayUtils::truncateToColumns(aircraftSrc, maxCols, line3, sizeof(line3));
        linesToDraw = (strlen(line3) > 0) ? 3 : ((strlen(line2) > 0) ? 2 : 1);
    }

    int totalTextH = linesToDraw * CHAR_HEIGHT;
    int16_t y = zone.y + (zone.h - totalTextH) / 2;

    if (line1IsAirline) {
        // Picopixel uses a baseline cursor; shift setCursor down so glyphs
        // appear within the same CHAR_HEIGHT slot as the default font.
        ctx.matrix->setFont(&Picopixel);
        DisplayUtils::drawTextLine(ctx.matrix, zone.x,
                                   y + AIRLINE_BASELINE_Y, line1, ctx.textColor);
        ctx.matrix->setFont(nullptr);
    } else {
        DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, line1, ctx.textColor);
    }
    if (linesToDraw >= 2 && strlen(line2) > 0) {
        y += CHAR_HEIGHT;
        DisplayUtils::drawTextLine(ctx.matrix, zone.x, y, line2, ctx.textColor);
    }
    if (linesToDraw >= 3 && strlen(line3) > 0) {
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

    char telLine1[LINE_BUF_SIZE] = "";
    char telLine2[LINE_BUF_SIZE] = "";

    if (linesAvailable >= 2) {
        // Two-line: altitude+speed on line 1, track+vrate on line 2
        char alt[16], spd[16], trk[16], vr[16];
        DisplayUtils::formatTelemetryValue(f.altitude_kft, "kft", 1, alt, sizeof(alt));
        DisplayUtils::formatTelemetryValue(f.speed_mph, "mph", 0, spd, sizeof(spd));

        char combined[LINE_BUF_SIZE];
        snprintf(combined, sizeof(combined), "%s %s", alt, spd);
        DisplayUtils::truncateToColumns(combined, maxCols, telLine1, sizeof(telLine1));

        DisplayUtils::formatTelemetryValue(f.track_deg, "d", 0, trk, sizeof(trk));
        DisplayUtils::formatTelemetryValue(f.vertical_rate_fps, "fps", 0, vr, sizeof(vr));

        snprintf(combined, sizeof(combined), "%s %s", trk, vr);
        DisplayUtils::truncateToColumns(combined, maxCols, telLine2, sizeof(telLine2));
    } else {
        // Compact: abbreviated single row
        char alt[16], spd[16], trk[16], vr[16];
        DisplayUtils::formatTelemetryValue(f.altitude_kft, "k", 0, alt, sizeof(alt));
        DisplayUtils::formatTelemetryValue(f.speed_mph, "", 0, spd, sizeof(spd));
        DisplayUtils::formatTelemetryValue(f.track_deg, "", 0, trk, sizeof(trk));
        DisplayUtils::formatTelemetryValue(f.vertical_rate_fps, "", 0, vr, sizeof(vr));

        char combined[LINE_BUF_SIZE];
        snprintf(combined, sizeof(combined), "A%s S%s T%s V%s", alt, spd, trk, vr);
        DisplayUtils::truncateToColumns(combined, maxCols, telLine1, sizeof(telLine1));
    }

    int linesToDraw = (linesAvailable >= 2) ? 2 : 1;  // telLine2 is always set when linesAvailable >= 2
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
    if (innerWidth <= 0 || innerHeight <= 0) return;  // Matrix too small for bordered card
    const int maxCols = innerWidth / CHAR_WIDTH;
    if (maxCols <= 0) return;

    // Build text in fixed buffers — zero heap allocation
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

    const char* aircraftSrc = f.aircraft_display_name_short.length()
                              ? f.aircraft_display_name_short.c_str()
                              : f.aircraft_code.c_str();

    char line1[LINE_BUF_SIZE], line2[LINE_BUF_SIZE], line3[LINE_BUF_SIZE];
    DisplayUtils::truncateToColumns(airlineSrc, maxCols, line1, sizeof(line1));
    DisplayUtils::truncateToColumns(route, maxCols, line2, sizeof(line2));
    DisplayUtils::truncateToColumns(aircraftSrc, maxCols, line3, sizeof(line3));

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
    const uint16_t borderColor = DisplayUtils::rgb565(255, 255, 255);
    ctx.matrix->drawRect(0, 0, mw, mh, borderColor);

    // Centered "..." text using user theme color
    const int textWidth = 3 * CHAR_WIDTH;  // 3 chars
    const int16_t x = (mw - textWidth) / 2;
    const int16_t y = (mh - CHAR_HEIGHT) / 2 - 2;

    DisplayUtils::drawTextLine(ctx.matrix, x, y, "...", ctx.textColor);
    // No FastLED.show() — pipeline responsibility
}
