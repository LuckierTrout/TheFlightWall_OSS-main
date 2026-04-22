/*
Purpose: WidgetRegistry static dispatch implementation (Story LE-1.2).

Implementation notes:
  - No virtual functions: dispatch is a compile-time `switch (type)` over
    WidgetType. Enum values without a matching case fall through to the
    post-switch `return false` (WidgetType::Unknown has an explicit case).
  - `fromString()` uses a linear sequence of strcmp comparisons. This is
    the same pattern as LayoutStore::isAllowedWidgetType() and mirrors
    CustomLayoutMode's parseType() from the V0 spike.
  - The allowlist here MUST stay in lock-step with LayoutStore's
    kAllowedWidgetTypes[]. If a new widget type is added in a later story,
    both lists must grow together or the Save→Load→Render loop breaks.
*/

#include "core/WidgetRegistry.h"

#include "widgets/TextWidget.h"
#include "widgets/ClockWidget.h"
#include "widgets/LogoWidget.h"
#include "widgets/FlightFieldWidget.h"
#include "widgets/MetricWidget.h"

#include <cstring>

// ------------------------------------------------------------------
// fromString
// ------------------------------------------------------------------
// ------------------------------------------------------------------
// widgetFontMetrics
//
// Returns the character advance (charW) and line height (charH) for a
// font + integer scale factor. Clamps textSize to the valid [1, 3] range
// so callers that receive the raw byte from a stored layout can pass it
// through without extra guards. Falls back to the default font when the
// enum value is unrecognized (forward-compat for future font additions).
//
// Keep these in sync with preview.js (dashboard WYSIWYG preview) and with
// the glyph metrics defined in Adafruit_GFX / Fonts/TomThumb.h.
// ------------------------------------------------------------------
WidgetFontMetrics widgetFontMetrics(WidgetFontId font, uint8_t textSize) {
    int scale = (int)textSize;
    if (scale < 1) scale = 1;
    if (scale > 3) scale = 3;

    switch (font) {
        case WidgetFontId::TomThumb:
            return { 4 * scale, 6 * scale };
        case WidgetFontId::Picopixel:
            // yAdvance=7 from the bundled Picopixel.h; xAdvance is variable
            // per glyph (3–5), but the truncation math treats it as a fixed
            // cell width. 4 is the most common xAdvance and renders most
            // strings correctly; wide glyphs ('M', 'W') may slightly overflow
            // their estimated slot, which drawTextLine clips by design.
            return { 4 * scale, 7 * scale };
        case WidgetFontId::Default:
        default:
            return { 6 * scale, 8 * scale };
    }
}

WidgetType WidgetRegistry::fromString(const char* typeStr) {
    if (typeStr == nullptr)                return WidgetType::Unknown;
    if (typeStr[0] == '\0')                return WidgetType::Unknown;
    if (strcmp(typeStr, "text")         == 0) return WidgetType::Text;
    if (strcmp(typeStr, "clock")        == 0) return WidgetType::Clock;
    if (strcmp(typeStr, "logo")         == 0) return WidgetType::Logo;
    if (strcmp(typeStr, "flight_field") == 0) return WidgetType::FlightField;
    if (strcmp(typeStr, "metric")       == 0) return WidgetType::Metric;
    return WidgetType::Unknown;
}

bool WidgetRegistry::isKnownType(const char* typeStr) {
    return fromString(typeStr) != WidgetType::Unknown;
}

// ------------------------------------------------------------------
// dispatch
//
// Explicitly enumerates every known WidgetType with no catch-all default,
// so adding a new enum value produces a -Wswitch warning at this site and
// cannot be silently ignored.
// WidgetType::Unknown is handled by its own explicit case. Any out-of-range
// uint8_t value cast to WidgetType (e.g. from memory corruption) falls
// through all case labels unmatched and hits the post-switch `return false`
// — valid C++, no undefined behavior.
// ------------------------------------------------------------------
bool WidgetRegistry::dispatch(WidgetType type,
                              const WidgetSpec& spec,
                              const RenderContext& ctx) {
    switch (type) {
        case WidgetType::Text:        return renderText(spec, ctx);
        case WidgetType::Clock:       return renderClock(spec, ctx);
        case WidgetType::Logo:        return renderLogo(spec, ctx);
        case WidgetType::FlightField: return renderFlightField(spec, ctx);
        case WidgetType::Metric:      return renderMetric(spec, ctx);
        case WidgetType::Unknown:     return false;
    }
    return false;
}
