#pragma once
/*
  HUB75MatrixDisplay - uniform 12-panel driver (hw-1.2, revised 2026-04-23).

  Drives 12x 64x64 HUB75 panels arranged as a 4-wide x 3-tall grid
  (256x192 virtual canvas) via the mrfaptastic ESP32-HUB75-MatrixPanel-DMA
  library. Uses a single MatrixPanel_I2S_DMA instance (one chain of 12
  panels at 1/32 scan) wrapped by VirtualMatrixPanel_T so modes and
  widgets draw into one contiguous Adafruit_GFX surface.

  Public surface mirrors the hw-1.1 stub so main.cpp / WebPortal /
  ModeRegistry compile unchanged. show() flips DMA buffers;
  updateBrightness dims the panels.

  NOTE: 256x192 double-buffered 16bpp = ~196 KB. The mrfaptastic library
  auto-reduces color depth (likely to 5-6 bpc) to keep the framebuffer in
  SRAM and maintain >=60 Hz refresh. Expected and acceptable for the
  FlightWall use case (flight cards, tickers, logos).
*/

#include <atomic>
#include <stdint.h>
#include <vector>

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-HUB75-VirtualMatrixPanel_T.hpp>

#include "interfaces/BaseDisplay.h"
#include "interfaces/DisplayMode.h"
#include "core/ConfigManager.h"
#include "core/LayoutEngine.h"

class HUB75MatrixDisplay : public BaseDisplay
{
public:
    HUB75MatrixDisplay();
    ~HUB75MatrixDisplay() override;

    bool initialize() override;
    void clear() override;
    void displayFlights(const std::vector<FlightInfo> &flights) override;

    void displayMessage(const String &message);
    void displayMessage(const String &message, uint16_t color);
    void showLoading();
    void updateBrightness(uint8_t brightness);
    bool reconfigureFromConfig();

    RenderContext buildRenderContext() const;
    void show();
    void displayFallbackCard(const std::vector<FlightInfo> &flights);

    void setCalibrationMode(bool enabled);
    bool isCalibrationMode() const;
    void renderCalibrationPattern();

    void setPositioningMode(bool enabled);
    bool isPositioningMode() const;
    void renderPositioningPattern();

    // 12x 64x64 panels, 4 cols x 3 rows = 256x192.
    static constexpr uint16_t PANEL_W       = 64;
    static constexpr uint16_t PANEL_H       = 64;
    static constexpr uint8_t  COLS          = 4;
    static constexpr uint8_t  ROWS          = 3;
    static constexpr uint8_t  CHAIN_LEN     = COLS * ROWS;  // 12
    static constexpr uint16_t CANVAS_WIDTH  = PANEL_W * COLS;  // 256
    static constexpr uint16_t CANVAS_HEIGHT = PANEL_H * ROWS;  // 192

    // Chain routing preset from the mrfaptastic lib. Serpentine start top-left,
    // wrap down. Adjust here if ribbon routing ends up Z-pattern or starts
    // from a different corner - single-line change, no other callers.
    using VirtualPanel = VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>;

private:
    MatrixPanel_I2S_DMA *_dma     = nullptr;
    VirtualPanel        *_virtual = nullptr;

    LayoutResult _layout = {};
    uint8_t _activeBrightness = 128;

    uint16_t _logoBuffer[1024];  // 32x32 RGB565 scratch, matches prior contract

    std::atomic<bool> _calibrationMode{false};
    std::atomic<bool> _positioningMode{false};
};
