/*
Purpose: V0 feasibility spike — implements CustomLayoutMode.
Parses embedded v0_spike_layout.h JSON once at init(), dispatches per-frame
renders via a switch on widget type. Zero heap alloc per frame.

See CustomLayoutMode.h and v0_spike_layout.h for schema.
*/

#include "modes/CustomLayoutMode.h"
#include "modes/v0_spike_layout.h"
#include "utils/DisplayUtils.h"
#include "utils/Log.h"

#include <ArduinoJson.h>
#include <cstring>
#include <cstdio>
#include <time.h>

// Zone descriptor (placeholder — CustomLayoutMode doesn't use fixed zones).
const ZoneRegion kCustomZones[] = {
    {"Custom", 0, 0, 100, 100}
};
const ModeZoneDescriptor CustomLayoutMode::_descriptor = {
    "V0 spike: generic renderer over embedded JSON layout",
    kCustomZones,
    1
};

// 5x7 default font + 1px spacing (matches ClassicCardMode for apples-to-apples).
static constexpr int CHAR_W = 6;
static constexpr int CHAR_H = 8;

static CustomLayoutMode::WidgetType parseType(const char* s) {
    if (s == nullptr) return CustomLayoutMode::WidgetType::UNKNOWN;
    if (strcmp(s, "text") == 0) return CustomLayoutMode::WidgetType::TEXT;
    if (strcmp(s, "clock") == 0) return CustomLayoutMode::WidgetType::CLOCK;
    if (strcmp(s, "logo_stub") == 0) return CustomLayoutMode::WidgetType::LOGO_STUB;
    return CustomLayoutMode::WidgetType::UNKNOWN;
}

// Pack r,g,b (0-255) into RGB565 without needing the matrix object.
// Adafruit GFX convention: RRRRRGGGGGGBBBBB.
static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8)
         | ((uint16_t)(g & 0xFC) << 3)
         | ((uint16_t)(b & 0xF8) >> 3);
}

bool CustomLayoutMode::parseLayout() {
    // Scoped JsonDocument — freed at end of function, keeps heap pressure
    // transient. ArduinoJson v7 default capacity is dynamic.
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, v0spike::kLayoutJson);
    if (err) {
        Serial.printf("[LE-V0] parseLayout: deserializeJson failed: %s\n", err.c_str());
        return false;
    }

    JsonArrayConst arr = doc["widgets"].as<JsonArrayConst>();
    _widgetCount = 0;
    for (JsonObjectConst obj : arr) {
        if (_widgetCount >= MAX_WIDGETS) {
            Serial.printf("[LE-V0] parseLayout: widget cap %u hit — truncating\n",
                          (unsigned)MAX_WIDGETS);
            break;
        }
        WidgetInstance& w = _widgets[_widgetCount];
        w.type  = parseType(obj["t"] | (const char*)nullptr);
        w.x     = (int16_t)(obj["x"] | 0);
        w.y     = (int16_t)(obj["y"] | 0);
        w.w     = (uint16_t)(obj["w"] | 0);
        w.h     = (uint16_t)(obj["h"] | 0);
        uint8_t r = (uint8_t)(obj["r"] | 255);
        uint8_t g = (uint8_t)(obj["g"] | 255);
        uint8_t b = (uint8_t)(obj["b"] | 255);
        w.color = rgb565(r, g, b);
        const char* s = obj["s"] | "";
        strncpy(w.text, s, sizeof(w.text) - 1);
        w.text[sizeof(w.text) - 1] = '\0';

        if (w.type == WidgetType::UNKNOWN) {
            const char* tstr = obj["t"] | "?";
            Serial.printf("[LE-V0] parseLayout: unknown widget type '%s' — skipping\n", tstr);
            continue;
        }
        _widgetCount++;
    }
    Serial.printf("[LE-V0] parseLayout: %u widgets parsed\n", (unsigned)_widgetCount);
    return true;
}

bool CustomLayoutMode::init(const RenderContext& ctx) {
    (void)ctx;
    if (!_parsed) {
        (void)parseLayout();
        _parsed = true;
    }
    return true;
}

void CustomLayoutMode::teardown() {
    // Keep _widgets[] and _parsed intact — cheap to re-use on next activation.
}

void CustomLayoutMode::render(const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights) {
    (void)flights;
    if (ctx.matrix == nullptr) return;

    ctx.matrix->fillScreen(0);

    for (size_t i = 0; i < _widgetCount; ++i) {
        const WidgetInstance& w = _widgets[i];
        switch (w.type) {
            case WidgetType::TEXT:      renderText(ctx, w);      break;
            case WidgetType::CLOCK:     renderClock(ctx, w);     break;
            case WidgetType::LOGO_STUB: renderLogoStub(ctx, w);  break;
            default: break;
        }
    }
    // No FastLED.show() — pipeline owns frame commit (FR35).
}

void CustomLayoutMode::renderText(const RenderContext& ctx, const WidgetInstance& w) {
    if (w.text[0] == '\0') return;
    // Truncate to column width using DisplayUtils (zero heap).
    int maxCols = w.w / CHAR_W;
    if (maxCols <= 0) return;
    char out[32];
    DisplayUtils::truncateToColumns(w.text, maxCols, out, sizeof(out));
    // Vertically center single line in widget height.
    int16_t y = w.y + (int16_t)(w.h > CHAR_H ? (w.h - CHAR_H) / 2 : 0);
    DisplayUtils::drawTextLine(ctx.matrix, w.x, y, out, w.color);
}

void CustomLayoutMode::renderClock(const RenderContext& ctx, const WidgetInstance& w) {
    // Optimization: cache the clock string at minute resolution. getLocalTime()
    // is a syscall that costs several hundred μs; the displayed HH:MM only
    // changes once per minute, so we can refresh the cached string every ~30s.
    static char s_clockBuf[16] = "--:--";
    static uint32_t s_lastClockUpdateMs = 0;
    uint32_t nowMs = millis();
    if (s_lastClockUpdateMs == 0 || (nowMs - s_lastClockUpdateMs) > 30000) {
        struct tm tinfo;
        if (getLocalTime(&tinfo, 0)) {
            snprintf(s_clockBuf, sizeof(s_clockBuf), "%02d:%02d",
                     tinfo.tm_hour, tinfo.tm_min);
            s_lastClockUpdateMs = nowMs;
        }
    }
    const char* buf = s_clockBuf;
    int maxCols = w.w / CHAR_W;
    if (maxCols <= 0) return;
    char out[16];
    DisplayUtils::truncateToColumns(buf, maxCols, out, sizeof(out));
    int16_t y = w.y + (int16_t)(w.h > CHAR_H ? (w.h - CHAR_H) / 2 : 0);
    DisplayUtils::drawTextLine(ctx.matrix, w.x, y, out, w.color);
}

void CustomLayoutMode::renderLogoStub(const RenderContext& ctx, const WidgetInstance& w) {
    // Stub: fill the widget rect with its color. A real V1 logo widget blits
    // RGB565 bitmap from LittleFS (see Story 4-3 LogoManager), which is out
    // of scope for the render-budget spike — filesystem cost is orthogonal.
    ctx.matrix->fillRect(w.x, w.y, w.w, w.h, w.color);
}

const char* CustomLayoutMode::getName() const {
    return "Custom Layout (V0 spike)";
}

const ModeZoneDescriptor& CustomLayoutMode::getZoneDescriptor() const {
    return _descriptor;
}

const ModeSettingsSchema* CustomLayoutMode::getSettingsSchema() const {
    return nullptr;
}
