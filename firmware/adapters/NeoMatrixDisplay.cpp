/*
Purpose: Render flight info on a WS2812B NeoPixel matrix via FastLED_NeoMatrix.
Responsibilities:
- Initialize LED matrix based on HardwareConfiguration and user display settings.
- Render a bordered, three-line flight “card” and a minimal loading screen.
- Cycle through multiple flights at a configurable interval.
Inputs: FlightInfo list; UserConfiguration (colors/brightness), TimingConfiguration (cycle),
        HardwareConfiguration (dimensions/pin/tiling).
Outputs: Visual output to LED matrix using FastLED.
*/
#include "adapters/NeoMatrixDisplay.h"

#include <Adafruit_GFX.h>
#include <FastLED_NeoMatrix.h>
#include <FastLED.h>
#include "utils/Log.h"
#include "core/ConfigManager.h"

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

bool NeoMatrixDisplay::initialize()
{
    HardwareConfig hw = ConfigManager::getHardware();
    DisplayConfig disp = ConfigManager::getDisplay();

    _matrixWidth = hw.tile_pixels * hw.tiles_x;
    _matrixHeight = hw.tile_pixels * hw.tiles_y;
    _numPixels = (uint32_t)_matrixWidth * (uint32_t)_matrixHeight;

    _leds = new CRGB[_numPixels];

    _matrix = new FastLED_NeoMatrix(
        _leds,
        hw.tile_pixels,
        hw.tile_pixels,
        hw.tiles_x,
        hw.tiles_y,
        NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT +
            NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG +
            NEO_TILE_TOP + NEO_TILE_RIGHT + NEO_TILE_COLUMNS + NEO_TILE_ZIGZAG);

    configureStripForPin(hw.display_pin, _leds, _numPixels);
    _matrix->setTextWrap(false);
    _matrix->setTextSize(1);
    _matrix->setBrightness(disp.brightness);
    clear();
    _currentFlightIndex = 0;
    _lastCycleMs = millis();
    return true;
}

void NeoMatrixDisplay::clear()
{
    if (_matrix)
    {
        _matrix->fillScreen(0);
        FastLED.show();
    }
}

String NeoMatrixDisplay::makeFlightLine(const FlightInfo &f)
{
    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                                                          : (f.operator_iata.length() ? f.operator_iata : f.operator_icao);
    if (airline.length() == 0)
    {
        airline = f.operator_code;
    }
    String origin = f.origin.code_icao;
    String dest = f.destination.code_icao;
    String route = origin + "-" + dest;
    String type = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short : f.aircraft_code;
    String ident = f.ident.length() ? f.ident : f.ident_icao;
    String line = airline;
    if (ident.length())
    {
        line += " ";
        line += ident;
    }
    if (type.length())
    {
        line += " ";
        line += type;
    }
    if (route.length() > 1)
    {
        line += " ";
        line += route;
    }
    return line;
}

void NeoMatrixDisplay::drawTextLine(int16_t x, int16_t y, const String &text, uint16_t color)
{
    _matrix->setCursor(x, y);
    _matrix->setTextColor(color);
    for (size_t i = 0; i < (size_t)text.length(); ++i)
    {
        _matrix->write(text[i]);
    }
}

String NeoMatrixDisplay::truncateToColumns(const String &text, int maxColumns)
{
    if ((int)text.length() <= maxColumns)
        return text;
    if (maxColumns <= 3)
        return text.substring(0, maxColumns);
    return text.substring(0, maxColumns - 3) + String("...");
}

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

    // Lines per display:
    // 1: airline
    // 2: route
    // 3: aircraft

    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                                                          : (f.operator_iata.length() ? f.operator_iata : (f.operator_icao.length() ? f.operator_icao : f.operator_code));

    String origin = f.origin.code_icao;
    String dest = f.destination.code_icao;
    String line2 = origin + String(">") + dest;

    String line3 = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short : f.aircraft_code;

    String line1 = truncateToColumns(airline, maxCols);
    line2 = truncateToColumns(line2, maxCols);
    line3 = truncateToColumns(line3, maxCols);

    const uint16_t textColor = _matrix->Color(disp.text_color_r,
                                              disp.text_color_g,
                                              disp.text_color_b);
    const int lineCount = 3;
    const int lineSpacing = 1; // 1px spacing between lines
    const int totalTextHeight = lineCount * charHeight + (lineCount - 1) * lineSpacing;
    const int topOffset = 1 + padding + (innerHeight - totalTextHeight) / 2; // center inside border with padding
    const int16_t startX = 1 + padding;                                      // left padding inside border

    int16_t y = topOffset;
    drawTextLine(startX, y, line1, textColor);
    y += charHeight + lineSpacing;
    drawTextLine(startX, y, line2, textColor);
    y += charHeight + lineSpacing;
    drawTextLine(startX, y, line3, textColor);
}

void NeoMatrixDisplay::displayFlights(const std::vector<FlightInfo> &flights)
{
    if (_matrix == nullptr)
        return;

    _matrix->fillScreen(0);

    if (!flights.empty())
    {
        const unsigned long now = millis();
        const unsigned long intervalMs = ConfigManager::getTiming().display_cycle * 1000UL;

        if (flights.size() > 1)
        {
            if (now - _lastCycleMs >= intervalMs)
            {
                _lastCycleMs = now;
                _currentFlightIndex = (_currentFlightIndex + 1) % flights.size();
            }
        }
        else
        {
            _currentFlightIndex = 0;
        }

        const size_t index = _currentFlightIndex % flights.size();
        displaySingleFlightCard(flights[index]);
    }
    else
    {
        displayLoadingScreen();
    }

    FastLED.show();
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
    drawTextLine(x, y, loadingText, textColor);

    FastLED.show();
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
    String line = truncateToColumns(message, maxCols);

    const int16_t x = 0;
    const int16_t y = (_matrixHeight - charHeight) / 2;
    drawTextLine(x, y, line, textColor);
    FastLED.show();
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
}

void NeoMatrixDisplay::renderFlight(const std::vector<FlightInfo> &flights, size_t index)
{
    if (_matrix == nullptr)
        return;

    _matrix->fillScreen(0);

    if (!flights.empty() && index < flights.size())
    {
        displaySingleFlightCard(flights[index]);
    }
    else
    {
        displayLoadingScreen();
    }

    FastLED.show();
}
