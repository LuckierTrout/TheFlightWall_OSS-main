/*
Purpose: Production CustomLayoutMode implementation (Story le-1.3).

Loads an active user-authored layout from LayoutStore on init(), parses the
JSON into a fixed WidgetSpec[] array (ArduinoJson v7 JsonDocument is scoped
to parseFromJson() so it's freed before render() ever runs), then dispatches
per-widget renders via WidgetRegistry::dispatch() with zero heap activity on
the render hot path.

Non-fatal error handling:
  - LayoutStore::load() returning false is NOT fatal — its `out` string is
    already populated with the baked-in default layout JSON. We parse that
    and succeed.
  - deserializeJson() failure IS fatal for init() — we set _invalid = true
    and return false. render() then draws a visible error indicator.

Parse-once rule (epic LE-1 Architecture): deserializeJson() lives strictly
inside parseFromJson() and the JsonDocument is destroyed before the function
returns. render() never sees JSON.
*/

#include "modes/CustomLayoutMode.h"

#include "core/LayoutStore.h"
#include "utils/DisplayUtils.h"
#include "utils/Log.h"

#include <ArduinoJson.h>
#include <FastLED_NeoMatrix.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Guard against accidental future downward drift of the heap budget
// (Task 5). Must be evaluated after WidgetSpec is a complete type.
static_assert(CustomLayoutMode::MEMORY_REQUIREMENT
              >= CustomLayoutMode::MAX_WIDGETS * sizeof(WidgetSpec),
              "CustomLayoutMode::MEMORY_REQUIREMENT must cover the full "
              "WidgetSpec array");

// ------------------------------------------------------------------
// Zone descriptor — CustomLayoutMode fills the whole canvas with a
// layout the user authors; no fixed zones.
// ------------------------------------------------------------------
static const ZoneRegion kCustomZones[] = {
    {"Custom", 0, 0, 100, 100}
};

const ModeZoneDescriptor CustomLayoutMode::_descriptor = {
    "Renders a user-authored layout (widgets) loaded from LittleFS.",
    kCustomZones,
    1
};

// ------------------------------------------------------------------
// Helpers — local-linkage so the translation unit stays self-contained.
// ------------------------------------------------------------------
namespace {

// Pack r, g, b (0-255) into RGB565 using the Adafruit GFX convention
// (RRRRR GGGGGG BBBBB).
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8)
         | ((uint16_t)(g & 0xFC) << 3)
         | ((uint16_t)(b & 0xF8) >> 3);
}

// Parse a "#RRGGBB" hex color string into RGB565. Returns white (0xFFFF)
// for null / malformed input so an authoring mistake does not hide the
// widget entirely. Accepts exact 6 hex digits after the '#'.
uint16_t parseHexColor(const char* hex) {
    if (hex == nullptr || hex[0] != '#') return 0xFFFF;
    // Require exactly "#RRGGBB".
    if (strlen(hex) != 7) return 0xFFFF;
    for (int i = 1; i < 7; ++i) {
        char c = hex[i];
        bool okHex = (c >= '0' && c <= '9')
                  || (c >= 'a' && c <= 'f')
                  || (c >= 'A' && c <= 'F');
        if (!okHex) return 0xFFFF;
    }
    char* endptr = nullptr;
    long v = strtol(hex + 1, &endptr, 16);
    if (endptr == nullptr || *endptr != '\0') return 0xFFFF;
    uint8_t r = (uint8_t)((v >> 16) & 0xFF);
    uint8_t g = (uint8_t)((v >>  8) & 0xFF);
    uint8_t b = (uint8_t)((v >>  0) & 0xFF);
    return rgb565(r, g, b);
}

// Map the JSON "align" string to WidgetSpec.align byte encoding.
uint8_t parseAlign(const char* a) {
    if (a == nullptr) return 0;
    if (strcmp(a, "center") == 0) return 1;
    if (strcmp(a, "right")  == 0) return 2;
    return 0;  // "left" or anything else → 0
}

// Copy a null-terminated string into a fixed destination buffer, always
// leaving a null terminator. Used for the id[] and content[] fields.
void copyFixed(char* dst, size_t dstLen, const char* src) {
    if (dstLen == 0) return;
    if (src == nullptr) { dst[0] = '\0'; return; }
    strncpy(dst, src, dstLen - 1);
    dst[dstLen - 1] = '\0';
}

}  // namespace

// ------------------------------------------------------------------
// init / teardown
// ------------------------------------------------------------------

bool CustomLayoutMode::init(const RenderContext& ctx) {
    (void)ctx;
    _widgetCount = 0;
    _invalid     = false;

    String json;
    bool found = LayoutStore::load(LayoutStore::getActiveId().c_str(), json);
    // NOTE: load() returning false is non-fatal. LayoutStore always populates
    // `json` — with the baked-in default layout on miss. Only a
    // deserializeJson() failure in parseFromJson() is treated as fatal.
    (void)found;

    if (!parseFromJson(json)) {
        _invalid = true;
        Serial.printf("[CustomLayoutMode] init: parse failed — error indicator will render\n");
        // Runtime heap log (Task 5).
        Serial.printf("[CustomLayoutMode] Free heap after init: %u\n", ESP.getFreeHeap());
        return false;
    }

    Serial.printf("[CustomLayoutMode] init: %u widgets loaded (found=%d)\n",
                  (unsigned)_widgetCount, (int)found);
    Serial.printf("[CustomLayoutMode] Free heap after init: %u\n", ESP.getFreeHeap());
    return true;
}

void CustomLayoutMode::teardown() {
    _widgetCount = 0;
    _invalid     = false;
    // _widgets[] contents left as-is — will be overwritten by the next init().
}

// ------------------------------------------------------------------
// parseFromJson — scoped JsonDocument; freed on return.
// ------------------------------------------------------------------

bool CustomLayoutMode::parseFromJson(const String& json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[CustomLayoutMode] parse failed: %s\n", err.c_str());
        return false;
    }

    JsonArrayConst arr = doc["widgets"].as<JsonArrayConst>();
    _widgetCount = 0;
    for (JsonObjectConst obj : arr) {
        if (_widgetCount >= MAX_WIDGETS) {
            Serial.printf("[CustomLayoutMode] widget cap %u hit — truncating\n",
                          (unsigned)MAX_WIDGETS);
            break;
        }

        const char* typeStr = obj["type"] | (const char*)nullptr;
        WidgetType t = WidgetRegistry::fromString(typeStr);
        if (t == WidgetType::Unknown) {
            Serial.printf("[CustomLayoutMode] unknown widget type '%s' — skipping\n",
                          typeStr ? typeStr : "?");
            continue;
        }

        WidgetSpec& w = _widgets[_widgetCount];
        w.type  = t;
        w.x     = (int16_t)(obj["x"] | 0);
        w.y     = (int16_t)(obj["y"] | 0);
        w.w     = (uint16_t)(obj["w"] | 0);
        w.h     = (uint16_t)(obj["h"] | 0);
        w.color = parseHexColor(obj["color"] | (const char*)nullptr);
        w.align = parseAlign(obj["align"] | (const char*)nullptr);
        w._reserved = 0;

        copyFixed(w.id,      sizeof(w.id),      obj["id"]      | (const char*)"");
        copyFixed(w.content, sizeof(w.content), obj["content"] | (const char*)"");

        _widgetCount++;
    }

    return true;
    // doc destructs here — no retained JSON state (epic parse-once rule).
}

// ------------------------------------------------------------------
// render — zero heap per frame; error indicator on _invalid.
// ------------------------------------------------------------------

void CustomLayoutMode::render(const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights) {
    if (ctx.matrix == nullptr) return;

    // LE-1.8: shallow-copy the RenderContext and inject currentFlight so that
    // widgets (FlightFieldWidget, MetricWidget) can bind to the flight being
    // displayed without changing the DisplayMode virtual interface. flights[0]
    // is the front-of-queue flight handed to us by ModeOrchestrator after the
    // display_cycle rotation. Empty flights vector → nullptr (widgets fall
    // back to placeholder strings; see AC #7/#10).
    RenderContext widgetCtx = ctx;
    widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];

    // Clear the canvas once per frame (pipeline owns the final show()).
    ctx.matrix->fillScreen(0);

    if (_invalid) {
        // AC #5 — draw a visible 1px red border + "LAYOUT ERR" text so a
        // broken layout is immediately obvious on the physical wall.
        const uint16_t red = rgb565(255, 0, 0);
        const int16_t maxX = ctx.matrix->width();
        const int16_t maxY = ctx.matrix->height();
        if (maxX > 0 && maxY > 0) {
            ctx.matrix->drawRect(0, 0, maxX, maxY, red);
            DisplayUtils::drawTextLine(ctx.matrix, 2, 2, "LAYOUT ERR", red);
        }
        return;
    }

    for (size_t i = 0; i < _widgetCount; ++i) {
        const WidgetSpec& w = _widgets[i];
        // Dispatch returns false for unimplemented/unknown types — we
        // intentionally ignore that here; the dispatcher is stateless and
        // never throws.
        (void)WidgetRegistry::dispatch(w.type, w, widgetCtx);
    }
    // No FastLED.show() — pipeline owns frame commit (FR35).
}

// ------------------------------------------------------------------
// Metadata
// ------------------------------------------------------------------

const char* CustomLayoutMode::getName() const {
    return "Custom Layout";
}

const ModeZoneDescriptor& CustomLayoutMode::getZoneDescriptor() const {
    return _descriptor;
}

const ModeSettingsSchema* CustomLayoutMode::getSettingsSchema() const {
    return nullptr;
}
