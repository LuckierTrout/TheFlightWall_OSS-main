#pragma once
/*
  HUB75MatrixDisplay - master chain driver (hw-1.2).

  Drives 6x 64x64 HUB75 panels arranged as a 3-wide x 2-tall grid
  (192x128 virtual canvas) via the mrfaptastic ESP32-HUB75-MatrixPanel-DMA
  library. Uses a single MatrixPanel_I2S_DMA instance (one chain of 6
  panels at 1/32 scan) wrapped by VirtualMatrixPanel_T so modes and
  widgets draw into one contiguous Adafruit_GFX surface.

  Public surface mirrors the hw-1.1 stub so main.cpp / WebPortal /
  ModeRegistry compile unchanged. show() now really flips DMA buffers;
  updateBrightness really dims the panels.

  The top-strip chain (3x 64x32 via a slave S3 over SPI) is NOT owned
  here - it will arrive as a separate composite wrapper in hw-1.7.
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

    // 6x 64x64 panels, 3 cols x 2 rows = 192x128.
    static constexpr uint16_t PANEL_W       = 64;
    static constexpr uint16_t PANEL_H       = 64;
    static constexpr uint8_t  COLS          = 3;
    static constexpr uint8_t  ROWS          = 2;
    static constexpr uint8_t  CHAIN_LEN     = COLS * ROWS;  // 6
    static constexpr uint16_t CANVAS_WIDTH  = PANEL_W * COLS;  // 192
    static constexpr uint16_t CANVAS_HEIGHT = PANEL_H * ROWS;  // 128

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
