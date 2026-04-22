#include "utils/DisplayUtils.h"

#include <cmath>

void DisplayUtils::drawTextLine(Adafruit_GFX* matrix, int16_t x, int16_t y,
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

// --- char*-based overloads (zero heap allocation) ---

void DisplayUtils::drawTextLine(Adafruit_GFX* matrix, int16_t x, int16_t y,
                                const char* text, uint16_t color)
{
    matrix->setCursor(x, y);
    matrix->setTextColor(color);
    while (*text) {
        matrix->write(*text++);
    }
}

void DisplayUtils::truncateToColumns(const char* text, int maxColumns,
                                      char* out, size_t outLen)
{
    if (outLen == 0) return;  // Guard: cannot write to zero-length buffer
    int len = (int)strlen(text);
    if (len <= maxColumns) {
        size_t copyLen = (size_t)len < outLen - 1 ? (size_t)len : outLen - 1;
        memcpy(out, text, copyLen);
        out[copyLen] = '\0';
    } else if (maxColumns <= 3) {
        size_t copyLen = (size_t)maxColumns < outLen - 1 ? (size_t)maxColumns : outLen - 1;
        memcpy(out, text, copyLen);
        out[copyLen] = '\0';
    } else {
        // Guard: outLen must accommodate at least the null terminator.
        // When outLen < 4, we cannot fit the "..." suffix — write as many chars as fit.
        if (outLen == 0) return;
        int prefixLen = maxColumns - 3;
        // Clamp prefixLen so that prefixLen + 3 + '\0' fits within outLen.
        int maxPrefix = (int)outLen - 4;  // reserve 3 for "..." + 1 for '\0'
        if (maxPrefix < 0) maxPrefix = 0;
        if (prefixLen > maxPrefix) prefixLen = maxPrefix;
        if (prefixLen < 0) prefixLen = 0;
        int dots = (int)outLen - 1 - prefixLen;  // remaining space for "..."
        if (dots > 3) dots = 3;
        if (dots < 0) dots = 0;
        memcpy(out, text, prefixLen);
        if (dots > 0) memcpy(out + prefixLen, "...", dots);
        out[prefixLen + dots] = '\0';
    }
}

void DisplayUtils::formatTelemetryValue(double value, const char* suffix, int decimals,
                                         char* out, size_t outLen)
{
    if (outLen == 0) return;  // Guard: cannot write to zero-length buffer
    if (isnan(value)) {
        strncpy(out, "--", outLen - 1);
        out[outLen - 1] = '\0';  // Safe: outLen > 0 guaranteed above
        return;
    }
    if (decimals == 0) {
        snprintf(out, outLen, "%d%s", (int)value, suffix);
    } else {
        char valBuf[16];
        // Use snprintf instead of dtostrf — snprintf is bounds-checked and avoids
        // potential overflow for unusual floating-point values (dtostrf has no limit).
        snprintf(valBuf, sizeof(valBuf), "%.*f", decimals, value);
        snprintf(out, outLen, "%s%s", valBuf, suffix);
    }
}

void DisplayUtils::drawBitmapRGB565(Adafruit_GFX* matrix, int16_t x, int16_t y,
                                     uint16_t w, uint16_t h, const uint16_t* bitmap,
                                     uint16_t zoneW, uint16_t zoneH)
{
    // Guard: null bitmap is a caller error — skip silently rather than hard-fault.
    if (bitmap == nullptr) return;

    // Render an RGB565 bitmap (w×h pixels) into the matrix, clipped to zone bounds.
    // Bitmap dimensions are caller-specified (e.g. 16×16 for LogoWidget V1 stub).
    // If zone is smaller than the bitmap, we center-crop. If larger, we center the bitmap.
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
                // Pass RGB565 pixel directly — no need to unpack/repack into R,G,B.
                matrix->drawPixel(destX + col, destY + row, pixel);
            }
        }
    }
}
