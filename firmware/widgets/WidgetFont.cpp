#include "widgets/WidgetFont.h"

#include <Adafruit_GFX.h>
#include <Fonts/Picopixel.h>
#include <Fonts/TomThumb.h>

#include <cstddef>

WidgetFontId resolveWidgetFontId(uint8_t raw) {
    if (raw == (uint8_t)WidgetFontId::TomThumb)  return WidgetFontId::TomThumb;
    if (raw == (uint8_t)WidgetFontId::Picopixel) return WidgetFontId::Picopixel;
    return WidgetFontId::Default;
}

int16_t applyWidgetFont(Adafruit_GFX* matrix, WidgetFontId fontId, int charH) {
    switch (fontId) {
        case WidgetFontId::TomThumb:
            matrix->setFont(&TomThumb);
            return (int16_t)(charH - 1);
        case WidgetFontId::Picopixel:
            matrix->setFont(&Picopixel);
            return (int16_t)(charH - 1);
        case WidgetFontId::Default:
        default:
            matrix->setFont(nullptr);
            return 0;
    }
}

void applyTextTransform(char* buf, uint8_t mode) {
    if (buf == nullptr || buf[0] == '\0') return;
    if (mode == (uint8_t)WidgetTextTransform::None) return;
    // ASCII-only toggle. Non-ASCII bytes (e.g. the 0xF7 degree glyph used by
    // MetricWidget suffixes) fall outside both ranges and stay untouched.
    if (mode == (uint8_t)WidgetTextTransform::Upper) {
        for (size_t i = 0; buf[i] != '\0'; ++i) {
            char c = buf[i];
            if (c >= 'a' && c <= 'z') buf[i] = (char)(c - ('a' - 'A'));
        }
    } else if (mode == (uint8_t)WidgetTextTransform::Lower) {
        for (size_t i = 0; buf[i] != '\0'; ++i) {
            char c = buf[i];
            if (c >= 'A' && c <= 'Z') buf[i] = (char)(c + ('a' - 'A'));
        }
    }
    // Unknown mode → no-op (defensive, matches the enum's fallback contract).
}
