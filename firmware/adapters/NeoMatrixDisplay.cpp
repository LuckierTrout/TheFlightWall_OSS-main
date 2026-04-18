/*
Purpose: Render flight info on a WS2812B NeoPixel matrix via FastLED_NeoMatrix.
Responsibilities:
- Initialize LED matrix based on HardwareConfiguration and user display settings.
- Render zone-based flight cards: logo zone, flight zone, telemetry zone (Story 3.3).
- Cycle through multiple flights at a configurable interval.
- Handle compact/full/expanded modes with appropriate content density.
Inputs: FlightInfo list; UserConfiguration (colors/brightness), TimingConfiguration (cycle),
        HardwareConfiguration (dimensions/pin/tiling).
Outputs: Visual output to LED matrix using FastLED.
*/
#include "adapters/NeoMatrixDisplay.h"

#include <Adafruit_GFX.h>
#include <FastLED_NeoMatrix.h>
#include <FastLED.h>
#include "utils/Log.h"
#include "utils/DisplayUtils.h"
#include "core/ConfigManager.h"
#include "core/LogoManager.h"

namespace {
template <uint8_t Pin>
void addStripController(CRGB *leds, uint32_t numPixels)
{
    FastLED.addLeds<WS2812B, Pin, GRB>(leds, numPixels);
}

bool configureStripForPin(uint8_t pin, CRGB *leds, uint32_t numPixels)
{
    switch (pin)
    {
        case 0:  addStripController<0>(leds, numPixels); return true;
        case 2:  addStripController<2>(leds, numPixels); return true;
        case 4:  addStripController<4>(leds, numPixels); return true;
        case 5:  addStripController<5>(leds, numPixels); return true;
        case 12: addStripController<12>(leds, numPixels); return true;
        case 13: addStripController<13>(leds, numPixels); return true;
        case 14: addStripController<14>(leds, numPixels); return true;
        case 15: addStripController<15>(leds, numPixels); return true;
        case 16: addStripController<16>(leds, numPixels); return true;
        case 17: addStripController<17>(leds, numPixels); return true;
        case 18: addStripController<18>(leds, numPixels); return true;
        case 19: addStripController<19>(leds, numPixels); return true;
        case 21: addStripController<21>(leds, numPixels); return true;
        case 22: addStripController<22>(leds, numPixels); return true;
        case 23: addStripController<23>(leds, numPixels); return true;
        case 25: addStripController<25>(leds, numPixels); return true;
        case 26: addStripController<26>(leds, numPixels); return true;
        case 27: addStripController<27>(leds, numPixels); return true;
        case 32: addStripController<32>(leds, numPixels); return true;
        case 33: addStripController<33>(leds, numPixels); return true;
        default:
            LOG_E("Display", "Unsupported display_pin; falling back to GPIO 25");
            addStripController<25>(leds, numPixels);
            return false;
    }
}
} // namespace

NeoMatrixDisplay::NeoMatrixDisplay() {}

NeoMatrixDisplay::~NeoMatrixDisplay()
{
    if (_leds)
    {
        delete[] _leds;
        _leds = nullptr;
    }
    if (_matrix)
    {
        delete _matrix;
        _matrix = nullptr;
    }
}

uint8_t NeoMatrixDisplay::buildMatrixFlags(const HardwareConfig &hw) const
{
    uint8_t flags = 0;

    switch (hw.origin_corner)
    {
        case 1:
            flags |= NEO_MATRIX_TOP + NEO_MATRIX_RIGHT;
            flags |= NEO_TILE_TOP + NEO_TILE_RIGHT;
            break;
        case 2:
            flags |= NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT;
            flags |= NEO_TILE_BOTTOM + NEO_TILE_LEFT;
            break;
        case 3:
            flags |= NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT;
            flags |= NEO_TILE_BOTTOM + NEO_TILE_RIGHT;
            break;
        case 0:
        default:
            flags |= NEO_MATRIX_TOP + NEO_MATRIX_LEFT;
            flags |= NEO_TILE_TOP + NEO_TILE_LEFT;
            break;
    }

    if (hw.scan_dir == 1)
    {
        flags |= NEO_MATRIX_COLUMNS + NEO_TILE_COLUMNS;
    }
    else
    {
        flags |= NEO_MATRIX_ROWS + NEO_TILE_ROWS;
    }

    if (hw.zigzag != 0)
    {
        flags |= NEO_MATRIX_ZIGZAG + NEO_TILE_ZIGZAG;
    }
    else
    {
        flags |= NEO_MATRIX_PROGRESSIVE + NEO_TILE_PROGRESSIVE;
    }

    return flags;
}

bool NeoMatrixDisplay::rebuildMatrix(const HardwareConfig &hw, const DisplayConfig &disp)
{
    if (_leds == nullptr)
    {
        return false;
    }

    if (_matrix != nullptr)
    {
        _matrix->fillScreen(0);
        FastLED.show();
        delete _matrix;
        _matrix = nullptr;
    }

    _matrix = new FastLED_NeoMatrix(
        _leds,
        hw.tile_pixels,
        hw.tile_pixels,
        hw.tiles_x,
        hw.tiles_y,
        buildMatrixFlags(hw));

    if (_matrix == nullptr)
    {
        return false;
    }

    _matrix->setTextWrap(false);
    _matrix->setTextSize(1);
    _matrix->setBrightness(disp.brightness);
    _activeBrightness = disp.brightness;

    _layout = LayoutEngine::compute(hw);
    _hardware = hw;
    clear();
    return true;
}

bool NeoMatrixDisplay::initialize()
{
    HardwareConfig hw = ConfigManager::getHardware();
    DisplayConfig disp = ConfigManager::getDisplay();

    _matrixWidth = hw.tile_pixels * hw.tiles_x;
    _matrixHeight = hw.tile_pixels * hw.tiles_y;
    _numPixels = (uint32_t)_matrixWidth * (uint32_t)_matrixHeight;

    if (_leds != nullptr)
    {
        delete[] _leds;
        _leds = nullptr;
    }
    _leds = new CRGB[_numPixels];

    configureStripForPin(hw.display_pin, _leds, _numPixels);
    return rebuildMatrix(hw, disp);
}

bool NeoMatrixDisplay::reconfigureFromConfig()
{
    HardwareConfig hw = ConfigManager::getHardware();
    if (hw.tiles_x != _hardware.tiles_x ||
        hw.tiles_y != _hardware.tiles_y ||
        hw.tile_pixels != _hardware.tile_pixels ||
        hw.display_pin != _hardware.display_pin)
    {
        return false;
    }

    _matrixWidth = hw.tile_pixels * hw.tiles_x;
    _matrixHeight = hw.tile_pixels * hw.tiles_y;
    _numPixels = (uint32_t)_matrixWidth * (uint32_t)_matrixHeight;
    return rebuildMatrix(hw, ConfigManager::getDisplay());
}

void NeoMatrixDisplay::clear()
{
    if (_matrix)
    {
        _matrix->fillScreen(0);
        FastLED.show();
    }
}

// --- Legacy full-screen card (kept for fallback if layout is invalid) ---

void NeoMatrixDisplay::displaySingleFlightCard(const FlightInfo &f)
{
    DisplayConfig disp = ConfigManager::getDisplay();
    // Border
    const uint16_t borderColor = _matrix->Color(disp.text_color_r,
                                                disp.text_color_g,
                                                disp.text_color_b);
    _matrix->drawRect(0, 0, _matrixWidth, _matrixHeight, borderColor);

    // Calculate columns for 6x8 default font (5x7 glyphs + 1px spacing)
    const int charWidth = 6;
    const int charHeight = 8;
    const int padding = 2;                                   // Small padding from border
    const int innerWidth = _matrixWidth - 2 - (2 * padding); // Account for border and padding
    const int innerHeight = _matrixHeight - 2 - (2 * padding);
    const int maxCols = innerWidth / charWidth;

    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                                                          : (f.operator_iata.length() ? f.operator_iata : (f.operator_icao.length() ? f.operator_icao : f.operator_code));

    String origin = f.origin.code_icao;
    String dest = f.destination.code_icao;
    // Only include ">" separator when at least one route code is present (mirrors ClassicCardMode fix)
    String line2 = (origin.length() > 0 || dest.length() > 0)
        ? (origin + String(">") + dest)
        : String("");

    String line3 = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short : f.aircraft_code;

    String line1 = DisplayUtils::truncateToColumns(airline, maxCols);
    line2 = DisplayUtils::truncateToColumns(line2, maxCols);
    line3 = DisplayUtils::truncateToColumns(line3, maxCols);

    const uint16_t textColor = _matrix->Color(disp.text_color_r,
                                              disp.text_color_g,
                                              disp.text_color_b);
    const int lineCount = 3;
    const int lineSpacing = 1; // 1px spacing between lines
    const int totalTextHeight = lineCount * charHeight + (lineCount - 1) * lineSpacing;
    const int topOffset = 1 + padding + (innerHeight - totalTextHeight) / 2; // center inside border with padding
    const int16_t startX = 1 + padding;                                      // left padding inside border

    int16_t y = topOffset;
    DisplayUtils::drawTextLine(_matrix, startX, y, line1, textColor);
    y += charHeight + lineSpacing;
    DisplayUtils::drawTextLine(_matrix, startX, y, line2, textColor);
    y += charHeight + lineSpacing;
    DisplayUtils::drawTextLine(_matrix, startX, y, line3, textColor);
}

// --- Zone-based rendering (Story 3.3) ---

void NeoMatrixDisplay::renderLogoZone(const FlightInfo &f, const Rect &zone)
{
    // Load logo by operator ICAO; fallback sprite if not found
    LogoManager::loadLogo(f.operator_icao, _logoBuffer);
    DisplayUtils::drawBitmapRGB565(_matrix, zone.x, zone.y, LOGO_WIDTH, LOGO_HEIGHT,
                                   _logoBuffer, zone.w, zone.h);
}

void NeoMatrixDisplay::renderFlightZone(const FlightInfo &f, const Rect &zone)
{
    DisplayConfig disp = ConfigManager::getDisplay();
    const uint16_t textColor = _matrix->Color(disp.text_color_r,
                                              disp.text_color_g,
                                              disp.text_color_b);
    const int charWidth = 6;
    const int charHeight = 8;
    const int maxCols = zone.w / charWidth;

    if (maxCols <= 0) return;

    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                     : (f.operator_iata.length() ? f.operator_iata
                     : (f.operator_icao.length() ? f.operator_icao : f.operator_code));
    String route = f.origin.code_icao + String(">") + f.destination.code_icao;
    String aircraft = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short : f.aircraft_code;

    // Determine how many lines fit
    int linesAvailable = zone.h / charHeight;
    if (linesAvailable <= 0) return;

    String line1;
    String line2;
    String line3;
    int linesToDraw = 1;

    if (linesAvailable == 1) {
        // Compact mode prioritizes route when only one row fits.
        String compactLine = route.length() ? route : airline;
        if (compactLine.length() == 0) {
            compactLine = aircraft;
        }
        line1 = DisplayUtils::truncateToColumns(compactLine, maxCols);
    } else if (linesAvailable == 2) {
        // Full 32px layouts cannot fit three 8px rows, so compress type into row two.
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
        line1 = DisplayUtils::truncateToColumns(airline, maxCols);
        line2 = DisplayUtils::truncateToColumns(route, maxCols);
        line3 = DisplayUtils::truncateToColumns(aircraft, maxCols);
        linesToDraw = line3.length() > 0 ? 3 : (line2.length() > 0 ? 2 : 1);
    }

    int16_t y = zone.y;
    int totalTextH = linesToDraw * charHeight;
    y = zone.y + (zone.h - totalTextH) / 2;

    DisplayUtils::drawTextLine(_matrix, zone.x, y, line1, textColor);
    if (linesToDraw >= 2 && line2.length() > 0) {
        y += charHeight;
        DisplayUtils::drawTextLine(_matrix, zone.x, y, line2, textColor);
    }
    if (linesToDraw >= 3 && line3.length() > 0) {
        y += charHeight;
        DisplayUtils::drawTextLine(_matrix, zone.x, y, line3, textColor);
    }
}

void NeoMatrixDisplay::renderTelemetryZone(const FlightInfo &f, const Rect &zone)
{
    DisplayConfig disp = ConfigManager::getDisplay();
    const uint16_t textColor = _matrix->Color(disp.text_color_r,
                                              disp.text_color_g,
                                              disp.text_color_b);
    const int charWidth = 6;
    const int charHeight = 8;
    const int maxCols = zone.w / charWidth;

    if (maxCols <= 0) return;

    int linesAvailable = zone.h / charHeight;
    if (linesAvailable <= 0) return;

    // Build telemetry strings based on available space
    // Compact: single line with abbreviated values
    // Full/Expanded: two lines with more detail
    String telLine1, telLine2;

    if (linesAvailable >= 2) {
        // Two-line format: altitude + speed on line 1, track + vrate on line 2
        String alt = DisplayUtils::formatTelemetryValue(f.altitude_kft, "kft", 1);
        String spd = DisplayUtils::formatTelemetryValue(f.speed_mph, "mph");
        telLine1 = DisplayUtils::truncateToColumns(alt + " " + spd, maxCols);

        String trk = DisplayUtils::formatTelemetryValue(f.track_deg, "d");
        String vr = DisplayUtils::formatTelemetryValue(f.vertical_rate_fps, "fps");
        telLine2 = DisplayUtils::truncateToColumns(trk + " " + vr, maxCols);
    } else {
        // Compact mode condenses all four values into one row with abbreviated labels.
        String alt = String("A") + DisplayUtils::formatTelemetryValue(f.altitude_kft, "k", 0);
        String spd = String("S") + DisplayUtils::formatTelemetryValue(f.speed_mph, "", 0);
        String trk = String("T") + DisplayUtils::formatTelemetryValue(f.track_deg, "", 0);
        String vr = String("V") + DisplayUtils::formatTelemetryValue(f.vertical_rate_fps, "", 0);
        telLine1 = DisplayUtils::truncateToColumns(alt + " " + spd + " " + trk + " " + vr, maxCols);
    }

    int linesToDraw = (linesAvailable >= 2 && telLine2.length() > 0) ? 2 : 1;
    int totalTextH = linesToDraw * charHeight;
    int16_t y = zone.y + (zone.h - totalTextH) / 2;

    DisplayUtils::drawTextLine(_matrix, zone.x, y, telLine1, textColor);
    if (linesToDraw >= 2) {
        y += charHeight;
        DisplayUtils::drawTextLine(_matrix, zone.x, y, telLine2, textColor);
    }
}

void NeoMatrixDisplay::renderZoneFlight(const FlightInfo &f)
{
    renderLogoZone(f, _layout.logoZone);
    renderFlightZone(f, _layout.flightZone);
    renderTelemetryZone(f, _layout.telemetryZone);
}

// --- Public display methods ---

void NeoMatrixDisplay::displayFlights(const std::vector<FlightInfo> &flights)
{
    // AC #7: delegate to displayFallbackCard + show() for compatibility
    displayFallbackCard(flights);
    show();
}

void NeoMatrixDisplay::displayLoadingScreen()
{
    if (_matrix == nullptr)
        return;

    _matrix->fillScreen(0);

    const uint16_t borderColor = _matrix->Color(255, 255, 255);
    _matrix->drawRect(0, 0, _matrixWidth, _matrixHeight, borderColor);

    const int charWidth = 6;
    const int charHeight = 8;
    const String loadingText = "...";
    const int textWidth = loadingText.length() * charWidth;

    const int16_t x = (_matrixWidth - textWidth) / 2;
    const int16_t y = (_matrixHeight - charHeight) / 2 - 2;

    DisplayConfig disp = ConfigManager::getDisplay();
    const uint16_t textColor = _matrix->Color(disp.text_color_r,
                                              disp.text_color_g,
                                              disp.text_color_b);
    DisplayUtils::drawTextLine(_matrix, x, y, loadingText, textColor);
}

void NeoMatrixDisplay::displayMessage(const String &message)
{
    if (_matrix == nullptr)
        return;

    _matrix->fillScreen(0);

    const int charWidth = 6;
    const int charHeight = 6;

    DisplayConfig disp = ConfigManager::getDisplay();
    const uint16_t textColor = _matrix->Color(disp.text_color_r,
                                              disp.text_color_g,
                                              disp.text_color_b);

    // Simple single-line message; truncate if needed
    const int innerWidth = _matrixWidth;
    const int maxCols = innerWidth / charWidth;
    String line = DisplayUtils::truncateToColumns(message, maxCols);

    const int16_t x = 0;
    const int16_t y = (_matrixHeight - charHeight) / 2;
    DisplayUtils::drawTextLine(_matrix, x, y, line, textColor);
}

void NeoMatrixDisplay::showLoading()
{
    displayLoadingScreen();
}

void NeoMatrixDisplay::updateBrightness(uint8_t brightness)
{
    if (_matrix)
    {
        _matrix->setBrightness(brightness);
    }
    _activeBrightness = brightness;  // Track actual brightness for buildRenderContext()
}

// --- Calibration mode (Story 4.2) ---

void NeoMatrixDisplay::setCalibrationMode(bool enabled)
{
    _calibrationMode.store(enabled, std::memory_order_release);
}

bool NeoMatrixDisplay::isCalibrationMode() const
{
    return _calibrationMode.load(std::memory_order_acquire);
}

void NeoMatrixDisplay::renderCalibrationPattern()
{
    if (_matrix == nullptr) return;

    _matrix->fillScreen(0);

    // Deterministic test pattern that reveals orientation/scan/zigzag effects.
    // Draws a gradient from pixel (0,0) corner to the opposite corner,
    // plus numbered quadrant markers and a directional arrow.
    // This makes it immediately obvious which corner is the origin
    // and which direction scanning progresses.

    uint16_t w = _matrixWidth;
    uint16_t h = _matrixHeight;

    if (w == 0 || h == 0) return;

    // Draw gradient: red at (0,0), green at (w-1,0), blue at (0,h-1), white at (w-1,h-1)
    // This creates a clear visual that shows all four corners distinctly.
    for (uint16_t y = 0; y < h; y++) {
        for (uint16_t x = 0; x < w; x++) {
            float fx = (float)x / (float)(w > 1 ? w - 1 : 1);
            float fy = (float)y / (float)(h > 1 ? h - 1 : 1);

            // Bilinear interpolation between four corner colors:
            // TL(0,0)=Red, TR(1,0)=Green, BL(0,1)=Blue, BR(1,1)=White
            uint8_t r = (uint8_t)((1.0f - fx) * (1.0f - fy) * 255 +  // TL Red
                                   fx * (1.0f - fy) * 0 +              // TR Green
                                   (1.0f - fx) * fy * 0 +              // BL Blue
                                   fx * fy * 255);                      // BR White
            uint8_t g = (uint8_t)((1.0f - fx) * (1.0f - fy) * 0 +
                                   fx * (1.0f - fy) * 255 +
                                   (1.0f - fx) * fy * 0 +
                                   fx * fy * 255);
            uint8_t b = (uint8_t)((1.0f - fx) * (1.0f - fy) * 0 +
                                   fx * (1.0f - fy) * 0 +
                                   (1.0f - fx) * fy * 255 +
                                   fx * fy * 255);

            _matrix->drawPixel(x, y, _matrix->Color(r, g, b));
        }
    }

    // Draw a bright white border on the first row and first column
    // to clearly indicate where pixel 0 starts
    for (uint16_t x = 0; x < w; x++) {
        _matrix->drawPixel(x, 0, _matrix->Color(255, 255, 255));
    }
    for (uint16_t y = 0; y < h; y++) {
        _matrix->drawPixel(0, y, _matrix->Color(255, 255, 255));
    }

    // Bright red pixel at (0,0) — the origin marker
    _matrix->drawPixel(0, 0, _matrix->Color(255, 0, 0));
    if (w > 1) _matrix->drawPixel(1, 0, _matrix->Color(255, 0, 0));
    if (h > 1) _matrix->drawPixel(0, 1, _matrix->Color(255, 0, 0));

    // Corner marker at (w-1, h-1) in bright cyan
    _matrix->drawPixel(w - 1, h - 1, _matrix->Color(0, 255, 255));
    if (w > 1) _matrix->drawPixel(w - 2, h - 1, _matrix->Color(0, 255, 255));
    if (h > 1) _matrix->drawPixel(w - 1, h - 2, _matrix->Color(0, 255, 255));
}

// --- Positioning mode (independent from calibration) ---

void NeoMatrixDisplay::setPositioningMode(bool enabled)
{
    _positioningMode.store(enabled, std::memory_order_release);
}

bool NeoMatrixDisplay::isPositioningMode() const
{
    return _positioningMode.load(std::memory_order_acquire);
}

void NeoMatrixDisplay::renderPositioningPattern()
{
    if (_matrix == nullptr) return;

    _matrix->fillScreen(0);

    uint8_t tilesX = _hardware.tiles_x;
    uint8_t tilesY = _hardware.tiles_y;
    uint8_t tp = _hardware.tile_pixels;
    uint8_t totalTiles = tilesX * tilesY;

    if (totalTiles == 0 || tp == 0) return;

    // 3x5 digit bitmaps: 3 columns, each byte = 5 row bits (bit0 = top row)
    static const uint8_t font[10][3] = {
        {0x1F, 0x11, 0x1F}, // 0
        {0x12, 0x1F, 0x10}, // 1
        {0x1D, 0x15, 0x17}, // 2
        {0x15, 0x15, 0x1F}, // 3
        {0x07, 0x04, 0x1F}, // 4
        {0x17, 0x15, 0x1D}, // 5
        {0x1F, 0x15, 0x1D}, // 6
        {0x01, 0x01, 0x1F}, // 7
        {0x1F, 0x15, 0x1F}, // 8
        {0x17, 0x15, 0x1F}, // 9
    };

    for (uint8_t row = 0; row < tilesY; row++) {
        for (uint8_t col = 0; col < tilesX; col++) {
            uint8_t gridIdx = row * tilesX + col;
            uint16_t tileX0 = col * tp;
            uint16_t tileY0 = row * tp;

            // Unique hue per tile
            uint8_t hue = gridIdx * 255 / totalTiles;
            CRGB bright, dim;
            bright.setHSV(hue, 255, 200);
            dim.setHSV(hue, 200, 40);

            uint16_t dimColor = _matrix->Color(dim.r, dim.g, dim.b);
            uint16_t brightColor = _matrix->Color(bright.r, bright.g, bright.b);

            // Fill tile with dim color
            for (uint8_t ly = 0; ly < tp; ly++)
                for (uint8_t lx = 0; lx < tp; lx++)
                    _matrix->drawPixel(tileX0 + lx, tileY0 + ly, dimColor);

            // Bright border
            for (uint8_t i = 0; i < tp; i++) {
                _matrix->drawPixel(tileX0 + i, tileY0, brightColor);
                _matrix->drawPixel(tileX0 + i, tileY0 + tp - 1, brightColor);
                _matrix->drawPixel(tileX0, tileY0 + i, brightColor);
                _matrix->drawPixel(tileX0 + tp - 1, tileY0 + i, brightColor);
            }

            // Red corner marker at top-left of tile (orientation guide)
            uint16_t red = _matrix->Color(255, 0, 0);
            uint8_t ms = (tp >= 8) ? 3 : 2;
            for (uint8_t my = 0; my < ms; my++)
                for (uint8_t mx = 0; mx < ms; mx++)
                    _matrix->drawPixel(tileX0 + mx, tileY0 + my, red);

            // Draw 3x5 digit(s) centered in tile
            uint16_t white = _matrix->Color(255, 255, 255);
            auto drawDigit = [&](uint16_t ox, uint16_t oy, uint8_t d) {
                if (d > 9) return;
                for (uint8_t cx = 0; cx < 3; cx++) {
                    uint8_t bits = font[d][cx];
                    for (uint8_t cy = 0; cy < 5; cy++) {
                        if (bits & (1 << cy)) {
                            _matrix->drawPixel(ox + cx, oy + cy, white);
                        }
                    }
                }
            };

            if (gridIdx < 10) {
                drawDigit(tileX0 + (tp - 3) / 2, tileY0 + (tp - 5) / 2, gridIdx);
            } else {
                uint16_t ox = tileX0 + (tp - 7) / 2;
                uint16_t oy = tileY0 + (tp - 5) / 2;
                drawDigit(ox, oy, gridIdx / 10);
                drawDigit(ox + 4, oy, gridIdx % 10);
            }
        }
    }

}

// --- Display pipeline API (Story ds-1.5, Architecture D3) ---

RenderContext NeoMatrixDisplay::buildRenderContext() const
{
    RenderContext ctx = {};
    ctx.matrix = _matrix;
    ctx.layout = _layout;
    ctx.logoBuffer = const_cast<uint16_t*>(_logoBuffer);

    DisplayConfig disp = ConfigManager::getDisplay();
    if (_matrix != nullptr) {
        ctx.textColor = _matrix->Color(disp.text_color_r,
                                       disp.text_color_g,
                                       disp.text_color_b);
    } else {
        ctx.textColor = 0;
    }
    // Use _activeBrightness (kept in sync by updateBrightness()) so scheduler dim override
    // is reflected in context, not just the static NVS value.
    ctx.brightness = _activeBrightness;

    TimingConfig timing = ConfigManager::getTiming();
    ctx.displayCycleMs = timing.display_cycle * 1000;

    return ctx;
}

void NeoMatrixDisplay::show()
{
    FastLED.show();
}

void NeoMatrixDisplay::displayFallbackCard(const std::vector<FlightInfo> &flights)
{
    if (_matrix == nullptr)
        return;

    _matrix->fillScreen(0);

    if (!flights.empty())
    {
        // AC #1: reuse displaySingleFlightCard draw logic per D3 (NeoMatrixDisplay owns hardware
        // and frame commit only). Zone rendering lives in ClassicCardMode / Story 3.3 modes.
        displaySingleFlightCard(flights[0]);
    }
    else
    {
        displayLoadingScreen();
    }
}
