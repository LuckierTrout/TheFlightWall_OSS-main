/*
  HUB75MatrixDisplay - master chain driver (hw-1.2).

  Drives one chain of 6x 64x64 panels arranged 3 wide x 2 tall (192x128)
  via MatrixPanel_I2S_DMA + VirtualMatrixPanel_T. Double-buffered;
  show() calls flipDMABuffer() for tear-free updates.
*/

#include "adapters/HUB75MatrixDisplay.h"

#include <cstring>

#include "adapters/HUB75PinMap.h"
#include "utils/DisplayUtils.h"
#include "utils/Log.h"

HUB75MatrixDisplay::HUB75MatrixDisplay()
{
    std::memset(_logoBuffer, 0, sizeof(_logoBuffer));
}

HUB75MatrixDisplay::~HUB75MatrixDisplay()
{
    delete _virtual;
    _virtual = nullptr;
    delete _dma;
    _dma = nullptr;
}

bool HUB75MatrixDisplay::initialize()
{
    HUB75_I2S_CFG::i2s_pins pins = {
        MasterChainPins::r1,  MasterChainPins::g1,  MasterChainPins::b1,
        MasterChainPins::r2,  MasterChainPins::g2,  MasterChainPins::b2,
        MasterChainPins::a,   MasterChainPins::b,   MasterChainPins::c,
        MasterChainPins::d,   MasterChainPins::e,
        MasterChainPins::lat, MasterChainPins::oe,  MasterChainPins::clk,
    };

    HUB75_I2S_CFG mxconfig(PANEL_W, PANEL_H, CHAIN_LEN, pins);
    mxconfig.double_buff = true;
    mxconfig.i2sspeed    = HUB75_I2S_CFG::HZ_10M;
    mxconfig.clkphase    = false;

    _dma = new MatrixPanel_I2S_DMA(mxconfig);
    if (_dma == nullptr || !_dma->begin()) {
        LOG_E("HUB75", "MatrixPanel_I2S_DMA begin() failed");
        delete _dma;
        _dma = nullptr;
        return false;
    }

    DisplayConfig disp = ConfigManager::getDisplay();
    _activeBrightness = disp.brightness;
    _dma->setBrightness8(_activeBrightness);
    _dma->clearScreen();

    _virtual = new VirtualPanel(ROWS, COLS, PANEL_W, PANEL_H);
    if (_virtual == nullptr) {
        LOG_E("HUB75", "VirtualMatrixPanel alloc failed");
        delete _dma;
        _dma = nullptr;
        return false;
    }
    _virtual->setDisplay(*_dma);
    _virtual->fillScreen(0);

    _layout = LayoutEngine::compute(COLS, ROWS, PANEL_W);

    LOG_I("HUB75", "initialized 192x128 (6x 64x64, 1/32 scan, double-buff)");
    return true;
}

bool HUB75MatrixDisplay::reconfigureFromConfig()
{
    // Canvas dimensions are fixed by the HW-1 master chain. Brightness
    // and other runtime knobs are applied via their dedicated setters.
    return true;
}

void HUB75MatrixDisplay::clear()
{
    if (_virtual != nullptr) {
        _virtual->fillScreen(0);
    }
}

void HUB75MatrixDisplay::displayFlights(const std::vector<FlightInfo> &flights)
{
    // Flight card draw path lives in modes/ClassicCardMode etc. (ds-1.5 pipeline).
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
    if (_virtual == nullptr) return;
    _virtual->fillScreen(0);
    _virtual->setTextColor(color);
    _virtual->setTextSize(1);
    _virtual->setCursor(2, 2);
    _virtual->print(message);
}

void HUB75MatrixDisplay::showLoading()
{
    if (_virtual == nullptr) return;
    _virtual->fillScreen(0);
}

void HUB75MatrixDisplay::updateBrightness(uint8_t brightness)
{
    _activeBrightness = brightness;
    if (_dma != nullptr) {
        _dma->setBrightness8(brightness);
    }
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
    // Per-panel blocks of primary/secondary colors so each of the 6 panels
    // in the 3x2 grid is individually identifiable, plus corner markers and
    // a row/col label in each panel's top-left so misrouted chains or
    // swapped-channel panels are immediately visible.
    if (_virtual == nullptr) return;

    struct Swatch { uint8_t r, g, b; };
    static const Swatch PANEL_COLORS[ROWS][COLS] = {
        { {128,   0,   0}, {  0, 128,   0}, {  0,   0, 128} },  // row 0: R  G  B
        { {128, 128,   0}, {  0, 128, 128}, {128,   0, 128} },  // row 1: Y  C  M
    };

    for (uint8_t r = 0; r < ROWS; ++r) {
        for (uint8_t c = 0; c < COLS; ++c) {
            const Swatch &s = PANEL_COLORS[r][c];
            uint16_t fill = DisplayUtils::rgb565(s.r, s.g, s.b);
            int16_t px = c * PANEL_W;
            int16_t py = r * PANEL_H;
            _virtual->fillRect(px, py, PANEL_W, PANEL_H, fill);
            _virtual->drawRect(px, py, PANEL_W, PANEL_H,
                               DisplayUtils::rgb565(255, 255, 255));
            // Label: "r,c" in panel's top-left in white
            _virtual->setTextColor(DisplayUtils::rgb565(255, 255, 255));
            _virtual->setTextSize(1);
            _virtual->setCursor(px + 2, py + 2);
            _virtual->print(r);
            _virtual->print(',');
            _virtual->print(c);
        }
    }

    // Corner markers across the whole 192x128 canvas
    _virtual->drawPixel(0,                 0,                  DisplayUtils::rgb565(255, 255, 255));
    _virtual->drawPixel(CANVAS_WIDTH - 1,  0,                  DisplayUtils::rgb565(255,   0,   0));
    _virtual->drawPixel(0,                 CANVAS_HEIGHT - 1,  DisplayUtils::rgb565(  0, 255,   0));
    _virtual->drawPixel(CANVAS_WIDTH - 1,  CANVAS_HEIGHT - 1,  DisplayUtils::rgb565(  0,   0, 255));
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
    if (_virtual == nullptr) return;
    _virtual->fillScreen(0);

    uint16_t white = DisplayUtils::rgb565(255, 255, 255);
    _virtual->drawRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, white);

    // Inter-panel seams so misaligned mounts are easy to spot
    uint16_t dim = DisplayUtils::rgb565(64, 64, 64);
    for (uint8_t c = 1; c < COLS; ++c) {
        _virtual->drawFastVLine(c * PANEL_W, 0, CANVAS_HEIGHT, dim);
    }
    for (uint8_t r = 1; r < ROWS; ++r) {
        _virtual->drawFastHLine(0, r * PANEL_H, CANVAS_WIDTH, dim);
    }

    // Corner markers
    _virtual->drawPixel(0,                 0,                  DisplayUtils::rgb565(255,   0,   0));
    _virtual->drawPixel(CANVAS_WIDTH - 1,  CANVAS_HEIGHT - 1,  DisplayUtils::rgb565(  0, 255, 255));
}

RenderContext HUB75MatrixDisplay::buildRenderContext() const
{
    RenderContext ctx = {};
    ctx.matrix = _virtual;  // Adafruit_GFX* via inheritance
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
    if (_dma != nullptr) {
        _dma->flipDMABuffer();
    }
}

void HUB75MatrixDisplay::displayFallbackCard(const std::vector<FlightInfo> &flights)
{
    // Fallback card draw path moved into ClassicCardMode (ds-1.5 pipeline).
    (void)flights;
}
