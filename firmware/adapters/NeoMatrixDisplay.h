#pragma once

#include <atomic>
#include <stdint.h>
#include <vector>
#include "interfaces/BaseDisplay.h"
#include "interfaces/DisplayMode.h"
#include "core/ConfigManager.h"
#include "core/LayoutEngine.h"

class FastLED_NeoMatrix;
struct CRGB;

class NeoMatrixDisplay : public BaseDisplay
{
public:
    NeoMatrixDisplay();
    ~NeoMatrixDisplay() override;

    bool initialize() override;
    void clear() override;
    void displayFlights(const std::vector<FlightInfo> &flights) override;
    void displayMessage(const String &message);
    void showLoading();
    void updateBrightness(uint8_t brightness);
    bool reconfigureFromConfig();

    // Display pipeline API (Story ds-1.5, Architecture D3)
    RenderContext buildRenderContext() const;
    void show();
    void displayFallbackCard(const std::vector<FlightInfo> &flights);

    // Calibration mode (Story 4.2)
    void setCalibrationMode(bool enabled);
    bool isCalibrationMode() const;
    void renderCalibrationPattern();

    // Positioning mode — independent from calibration
    void setPositioningMode(bool enabled);
    bool isPositioningMode() const;
    void renderPositioningPattern();

private:
    FastLED_NeoMatrix *_matrix = nullptr;
    CRGB *_leds = nullptr;

    uint16_t _matrixWidth = 0;
    uint16_t _matrixHeight = 0;
    uint32_t _numPixels = 0;
    HardwareConfig _hardware = {};
    uint8_t _activeBrightness = 128;  // Tracks real brightness (incl. scheduler override)

    // Layout zones (computed once at init from hardware config)
    LayoutResult _layout = {};

    // Reusable logo buffer (avoids heap churn per frame)
    uint16_t _logoBuffer[1024]; // 32x32 RGB565

    // Calibration mode (Story 4.2; BF-1 / TD-1: atomic for cross-core safety —
    // Core 0 reader (display task) vs Core 1 writer (WebPortal calibration toggle).
    // volatile alone is not a concurrency primitive on Xtensa.)
    std::atomic<bool> _calibrationMode{false};

    // Positioning mode — independent from calibration; same Core 0/Core 1 split.
    std::atomic<bool> _positioningMode{false};

    void displaySingleFlightCard(const FlightInfo &f);
    void displayLoadingScreen();
    bool rebuildMatrix(const HardwareConfig &hw, const DisplayConfig &disp);
    uint8_t buildMatrixFlags(const HardwareConfig &hw) const;

    // Zone-based rendering helpers (Story 3.3)
    void renderZoneFlight(const FlightInfo &f);
    void renderLogoZone(const FlightInfo &f, const Rect &zone);
    void renderFlightZone(const FlightInfo &f, const Rect &zone);
    void renderTelemetryZone(const FlightInfo &f, const Rect &zone);
};
