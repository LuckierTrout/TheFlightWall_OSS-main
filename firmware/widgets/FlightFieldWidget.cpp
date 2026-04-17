/*
Purpose: FlightFieldWidget implementation (Story LE-1.8).

Binds a WidgetSpec.content field-key string (e.g. "airline") to a
FlightInfo member and draws it as a single text line. Falls back to
"--" when ctx.currentFlight is null, the key is unknown, or the
resolved value is empty — AC #7/#10.

Zero heap allocation on the render hot path:
  - Field resolution hands back a `const char*` into the FlightInfo's
    Arduino String storage via `.c_str()` — no String temporaries.
  - Truncation/drawing uses the DisplayUtils char* overloads with a
    stack buffer.
*/

#include "widgets/FlightFieldWidget.h"

#include "utils/DisplayUtils.h"

#include <cstring>

namespace {

// Resolve a field key against a FlightInfo. Returns a pointer to the
// underlying String storage (valid for the lifetime of the FlightInfo)
// or nullptr if the key is unknown. Caller is responsible for the
// empty-string fallback. Keys come straight from spec.content so they
// must be matched exactly (strcmp).
const char* resolveField(const char* key, const FlightInfo& f) {
    if (key == nullptr) return nullptr;

    if (strcmp(key, "airline") == 0) {
        return f.airline_display_name_full.c_str();
    }
    if (strcmp(key, "ident") == 0) {
        // Prefer ICAO → IATA → raw ident so the widget always shows
        // something when the flight has any identifier at all.
        if (f.ident_icao.length() > 0) return f.ident_icao.c_str();
        if (f.ident_iata.length() > 0) return f.ident_iata.c_str();
        return f.ident.c_str();
    }
    if (strcmp(key, "origin_icao") == 0) {
        return f.origin.code_icao.c_str();
    }
    if (strcmp(key, "dest_icao") == 0) {
        return f.destination.code_icao.c_str();
    }
    if (strcmp(key, "aircraft") == 0) {
        if (f.aircraft_display_name_short.length() > 0) {
            return f.aircraft_display_name_short.c_str();
        }
        return f.aircraft_code.c_str();
    }
    return nullptr;
}

}  // namespace

bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx) {
    // Minimum dimension floor — at least one 5x7 glyph must fit.
    if ((int)spec.w < WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) {
        return true;  // skip render — not an error
    }

    // Resolve the field value. Fallback to "--" when the flight is
    // unavailable, the key is unknown, or the value is empty.
    const char* value = nullptr;
    if (ctx.currentFlight != nullptr) {
        value = resolveField(spec.content, *ctx.currentFlight);
    }
    if (value == nullptr || value[0] == '\0') {
        value = "--";
    }

    // Hardware-free test path.
    if (ctx.matrix == nullptr) return true;

    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    if (maxCols <= 0) return true;

    char out[48];
    DisplayUtils::truncateToColumns(value, maxCols, out, sizeof(out));

    int16_t drawY = spec.y;
    if ((int)spec.h > WIDGET_CHAR_H) {
        drawY = spec.y + (int16_t)(((int)spec.h - WIDGET_CHAR_H) / 2);
    }

    DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);
    return true;
}
