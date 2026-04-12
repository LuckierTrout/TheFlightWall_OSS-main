#pragma once

#include <stdint.h>
#include <vector>
#include "interfaces/BaseDisplay.h"
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
    void renderFlight(const std::vector<FlightInfo> &flights, size_t index);
    bool reconfigureFromConfig();

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

    size_t _currentFlightIndex = 0;
    unsigned long _lastCycleMs = 0;

    // Layout zones (computed once at init from hardware config)
    LayoutResult _layout = {};

    // Reusable logo buffer (avoids heap churn per frame)
    uint16_t _logoBuffer[1024]; // 32x32 RGB565

    // Calibration mode (Story 4.2)
    volatile bool _calibrationMode = false;

    // Positioning mode — independent from calibration
    volatile bool _positioningMode = false;

    void drawTextLine(int16_t x, int16_t y, const String &text, uint16_t color);
    String makeFlightLine(const FlightInfo &f);
    String truncateToColumns(const String &text, int maxColumns);
    void displaySingleFlightCard(const FlightInfo &f);
    void displayLoadingScreen();
    bool rebuildMatrix(const HardwareConfig &hw, const DisplayConfig &disp);
    uint8_t buildMatrixFlags(const HardwareConfig &hw) const;

    // Zone-based rendering helpers (Story 3.3)
    void renderZoneFlight(const FlightInfo &f);
    void renderLogoZone(const FlightInfo &f, const Rect &zone);
    void renderFlightZone(const FlightInfo &f, const Rect &zone);
    void renderTelemetryZone(const FlightInfo &f, const Rect &zone);
    void drawBitmapRGB565(int16_t x, int16_t y, uint16_t w, uint16_t h,
                          const uint16_t *bitmap, uint16_t zoneW, uint16_t zoneH);
    String formatTelemetryValue(double value, const char* suffix, int decimals = 0);
};
