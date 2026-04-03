#pragma once

#include <Arduino.h>

namespace UserConfiguration
{
    // Location configuration
    static const double CENTER_LAT = 37.7749; // San Francisco (example)
    static const double CENTER_LON = -122.4194;
    static const double RADIUS_KM = 10.0; // Search radius in km

    // Display customization
    // Brightness controls overall display brightness (0-255)
    static const uint8_t DISPLAY_BRIGHTNESS = 5;

    // RGB color for all text rendering on the LED matrix
    static const uint8_t TEXT_COLOR_R = 255;
    static const uint8_t TEXT_COLOR_G = 255;
    static const uint8_t TEXT_COLOR_B = 255;
}
