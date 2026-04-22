#pragma once
/*
  HUB75MatrixDisplay — HW-1.1 placeholder stub.

  Presents a 192x96 virtual canvas (the HW-1 composite target) as an
  Adafruit_GFX surface so modes, widgets, and the Layout Engine compile
  and link against the new display stack. No physical panels are wired
  up yet — that's HW-1.2's job. The stub owns a GFXcanvas16 as its
  drawable buffer; all render methods draw into it, show() is a no-op.

  Surface mirrors the former NeoMatrixDisplay so main.cpp / WebPortal /
  ModeRegistry don't need call-site changes beyond the instantiation type.
*/

#include <atomic>
#include <stdint.h>
#include <vector>
#include <Adafruit_GFX.h>

#include "interfaces/BaseDisplay.h"
#include "interfaces/DisplayMode.h"
#include "core/ConfigManager.h"
#include "core/LayoutEngine.h"

// Forward declaration keeps this header light — GFXcanvas16's real
// definition comes in via Adafruit_GFX.h at the .cpp level.
class GFXcanvas16;

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

    // HW-1.1 stub returns the HW-1 composite target dimensions (192 x 96)
    // even though no panel is attached. HW-1.2 will replace this class
    // with real MatrixPanel_I2S_DMA chains.
    static constexpr uint16_t CANVAS_WIDTH  = 192;
    static constexpr uint16_t CANVAS_HEIGHT = 96;

private:
    GFXcanvas16 *_canvas = nullptr;

    LayoutResult _layout = {};
    uint8_t _activeBrightness = 128;

    uint16_t _logoBuffer[1024];  // 32x32 RGB565 scratch, matches prior contract

    std::atomic<bool> _calibrationMode{false};
    std::atomic<bool> _positioningMode{false};
};
