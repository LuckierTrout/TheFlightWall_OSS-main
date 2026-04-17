#pragma once

#include <FastLED_NeoMatrix.h>
#include <Arduino.h>

namespace DisplayUtils {
    // String-based API (original)
    void drawTextLine(FastLED_NeoMatrix* matrix, int16_t x, int16_t y,
                      const String& text, uint16_t color);
    String truncateToColumns(const String& text, int maxColumns);
    String formatTelemetryValue(double value, const char* suffix, int decimals = 0);
    void drawBitmapRGB565(FastLED_NeoMatrix* matrix, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint16_t* bitmap,
                          uint16_t zoneW, uint16_t zoneH);

    // char*-based API — zero heap allocation for hot-path rendering at 20fps.
    // Produces identical pixel output to the String versions.
    void drawTextLine(FastLED_NeoMatrix* matrix, int16_t x, int16_t y,
                      const char* text, uint16_t color);
    void truncateToColumns(const char* text, int maxColumns,
                           char* out, size_t outLen);
    void formatTelemetryValue(double value, const char* suffix, int decimals,
                              char* out, size_t outLen);
}
