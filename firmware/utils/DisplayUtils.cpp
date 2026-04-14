#include "utils/DisplayUtils.h"

#include <cmath>

void DisplayUtils::drawTextLine(FastLED_NeoMatrix* matrix, int16_t x, int16_t y,
                                const String& text, uint16_t color)
{
    matrix->setCursor(x, y);
    matrix->setTextColor(color);
    for (size_t i = 0; i < (size_t)text.length(); ++i)
    {
        matrix->write(text[i]);
    }
}

String DisplayUtils::truncateToColumns(const String& text, int maxColumns)
{
    if ((int)text.length() <= maxColumns)
        return text;
    if (maxColumns <= 3)
        return text.substring(0, maxColumns);
    return text.substring(0, maxColumns - 3) + String("...");
}

String DisplayUtils::formatTelemetryValue(double value, const char* suffix, int decimals)
{
    if (isnan(value)) {
        return String("--");
    }
    if (decimals == 0) {
        return String((int)value) + suffix;
    }
    return String(value, decimals) + suffix;
}

void DisplayUtils::drawBitmapRGB565(FastLED_NeoMatrix* matrix, int16_t x, int16_t y,
                                     uint16_t w, uint16_t h, const uint16_t* bitmap,
                                     uint16_t zoneW, uint16_t zoneH)
{
    // Render RGB565 bitmap into the matrix, clipped to zone bounds.
    // Bitmap is always 32x32 (LOGO_WIDTH x LOGO_HEIGHT).
    // If zone is smaller, we center-crop. If larger, we center the bitmap.
    int16_t offsetX = 0, offsetY = 0;
    uint16_t drawW = w, drawH = h;

    if (zoneW < w) {
        offsetX = (w - zoneW) / 2; // crop from center
        drawW = zoneW;
    }
    if (zoneH < h) {
        offsetY = (h - zoneH) / 2;
        drawH = zoneH;
    }

    int16_t destX = x;
    int16_t destY = y;
    if (zoneW > w) {
        destX = x + (zoneW - w) / 2; // center in zone
    }
    if (zoneH > h) {
        destY = y + (zoneH - h) / 2;
    }

    for (uint16_t row = 0; row < drawH; row++) {
        for (uint16_t col = 0; col < drawW; col++) {
            uint16_t pixel = bitmap[(row + offsetY) * w + (col + offsetX)];
            if (pixel != 0) {
                // Convert RGB565 to R, G, B components
                uint8_t r = ((pixel >> 11) & 0x1F) << 3;
                uint8_t g = ((pixel >> 5) & 0x3F) << 2;
                uint8_t b = (pixel & 0x1F) << 3;
                matrix->drawPixel(destX + col, destY + row, matrix->Color(r, g, b));
            }
        }
    }
}
