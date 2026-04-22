#pragma once

#include <Adafruit_GFX.h>
#include <Arduino.h>

namespace DisplayUtils {
    // Pack 8-8-8 RGB into 16-bit 5-6-5 format. FastLED_NeoMatrix previously
    // provided this as an instance method (`matrix->Color(r,g,b)`); after
    // the HW-1.1 lib swap we need a type-agnostic free helper since neither
    // Adafruit_GFX base nor GFXcanvas16 exposes one. MatrixPanel_I2S_DMA
    // has color565() as an instance method, but callers that take
    // Adafruit_GFX* can't reach it.
    inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
        return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }

    // String-based API (original)
    void drawTextLine(Adafruit_GFX* matrix, int16_t x, int16_t y,
                      const String& text, uint16_t color);
    String truncateToColumns(const String& text, int maxColumns);
    String formatTelemetryValue(double value, const char* suffix, int decimals = 0);
    void drawBitmapRGB565(Adafruit_GFX* matrix, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint16_t* bitmap,
                          uint16_t zoneW, uint16_t zoneH);

    // char*-based API — zero heap allocation for hot-path rendering at 20fps.
    // Produces identical pixel output to the String versions.
    void drawTextLine(Adafruit_GFX* matrix, int16_t x, int16_t y,
                      const char* text, uint16_t color);
    void truncateToColumns(const char* text, int maxColumns,
                           char* out, size_t outLen);
    void formatTelemetryValue(double value, const char* suffix, int decimals,
                              char* out, size_t outLen);
}
