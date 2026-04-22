/*
  HUB75MatrixDisplay — HW-1.1 stub implementation.

  Draws into a GFXcanvas16(192, 96) so modes and widgets compile and
  run through their render paths; show() does not push pixels to any
  physical panel. HW-1.2 replaces this class with a real two-chain
  MatrixPanel_I2S_DMA composite.
*/

#include "adapters/HUB75MatrixDisplay.h"

#include <Adafruit_GFX.h>
#include <cstring>

#include "utils/DisplayUtils.h"
#include "utils/Log.h"

HUB75MatrixDisplay::HUB75MatrixDisplay()
{
    std::memset(_logoBuffer, 0, sizeof(_logoBuffer));
}

HUB75MatrixDisplay::~HUB75MatrixDisplay()
{
    delete _canvas;
    _canvas = nullptr;
}

bool HUB75MatrixDisplay::initialize()
{
    if (_canvas == nullptr) {
        _canvas = new GFXcanvas16(CANVAS_WIDTH, CANVAS_HEIGHT);
    }
    if (_canvas == nullptr) {
        LOG_E("HUB75", "initialize - canvas alloc failed");
        return false;
    }

    // Compute layout zones against the virtual canvas. The engine still
    // takes tile-ish inputs; we feed it a 1-tile-of-192x96 arrangement so
    // zones are computed against the correct matrix dimensions.
    _layout = LayoutEngine::compute(1, 1, CANVAS_WIDTH);
    _layout.matrixWidth = CANVAS_WIDTH;
    _layout.matrixHeight = CANVAS_HEIGHT;

    _canvas->fillScreen(0);
    DisplayConfig disp = ConfigManager::getDisplay();
    _activeBrightness = disp.brightness;

    LOG_I("HUB75", "stub initialized (192x96, display-dead)");
    return true;
}

bool HUB75MatrixDisplay::reconfigureFromConfig()
{
    // Stub: dimensions are fixed by the HW-1 composite target. No reconfig.
    return true;
}

void HUB75MatrixDisplay::clear()
{
    if (_canvas != nullptr) {
        _canvas->fillScreen(0);
    }
}

void HUB75MatrixDisplay::displayFlights(const std::vector<FlightInfo> &flights)
{
    // Stub: draw path for flight cards lives in modes/ClassicCardMode etc.
    // The pipeline no longer routes through the adapter for flight cards
    // (ds-1.5 pipeline), so this is left as a no-op.
    (void)flights;
}

void HUB75MatrixDisplay::displayMessage(const String &message)
{
    DisplayConfig disp = ConfigManager::getDisplay();
    uint16_t color = DisplayUtils::rgb565(disp.text_color_r,
                                            disp.text_color_g,
                                            disp.text_color_b);
    displayMessage(message, color);
}

void HUB75MatrixDisplay::displayMessage(const String &message, uint16_t color)
{
    if (_canvas == nullptr) return;
    _canvas->fillScreen(0);
    _canvas->setTextColor(color);
    _canvas->setTextSize(1);
    _canvas->setCursor(2, 2);
    _canvas->print(message);
}

void HUB75MatrixDisplay::showLoading()
{
    if (_canvas == nullptr) return;
    _canvas->fillScreen(0);
}

void HUB75MatrixDisplay::updateBrightness(uint8_t brightness)
{
    _activeBrightness = brightness;
    // Stub: no physical panel to modulate.
}

void HUB75MatrixDisplay::setCalibrationMode(bool enabled)
{
    _calibrationMode.store(enabled, std::memory_order_release);
}

bool HUB75MatrixDisplay::isCalibrationMode() const
{
    return _calibrationMode.load(std::memory_order_acquire);
}

void HUB75MatrixDisplay::renderCalibrationPattern()
{
    if (_canvas == nullptr) return;
    // Simple gradient placeholder — HW-1.2 will replace with a pattern
    // that also validates the y=32 seam between top and bottom chains.
    for (int16_t y = 0; y < CANVAS_HEIGHT; ++y) {
        for (int16_t x = 0; x < CANVAS_WIDTH; ++x) {
            uint8_t r = (uint8_t)((x * 255) / CANVAS_WIDTH);
            uint8_t g = (uint8_t)((y * 255) / CANVAS_HEIGHT);
            _canvas->drawPixel(x, y, DisplayUtils::rgb565(r, g, 128));
        }
    }
}

void HUB75MatrixDisplay::setPositioningMode(bool enabled)
{
    _positioningMode.store(enabled, std::memory_order_release);
}

bool HUB75MatrixDisplay::isPositioningMode() const
{
    return _positioningMode.load(std::memory_order_acquire);
}

void HUB75MatrixDisplay::renderPositioningPattern()
{
    if (_canvas == nullptr) return;
    _canvas->fillScreen(0);
    uint16_t white = DisplayUtils::rgb565(255, 255, 255);
    _canvas->drawRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, white);
    // Corner markers
    _canvas->drawPixel(0, 0, DisplayUtils::rgb565(255, 0, 0));
    _canvas->drawPixel(CANVAS_WIDTH - 1, CANVAS_HEIGHT - 1,
                       DisplayUtils::rgb565(0, 255, 255));
}

RenderContext HUB75MatrixDisplay::buildRenderContext() const
{
    RenderContext ctx = {};
    ctx.matrix = _canvas;
    ctx.layout = _layout;
    ctx.logoBuffer = const_cast<uint16_t*>(_logoBuffer);

    DisplayConfig disp = ConfigManager::getDisplay();
    ctx.textColor = DisplayUtils::rgb565(disp.text_color_r,
                                           disp.text_color_g,
                                           disp.text_color_b);
    ctx.brightness = _activeBrightness;

    TimingConfig timing = ConfigManager::getTiming();
    ctx.displayCycleMs = timing.display_cycle * 1000;
    return ctx;
}

void HUB75MatrixDisplay::show()
{
    // No-op: no panel wired. HW-1.2 calls flipDMABuffer() on both chains.
}

void HUB75MatrixDisplay::displayFallbackCard(const std::vector<FlightInfo> &flights)
{
    // Stub: fallback card draw path moved into ClassicCardMode (ds-1.5 pipeline).
    (void)flights;
}
