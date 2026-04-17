/*
Purpose: MetricWidget implementation (Story LE-1.8).

Binds a WidgetSpec.content metric-key string (e.g. "alt") to a FlightInfo
telemetry value and draws it with a canonical suffix. Falls back to "--"
when ctx.currentFlight is null, the key is unknown, or the resolved
value is NAN — AC #7/#10.

Zero heap allocation on the render hot path:
  - Metric resolution returns a (value, suffix, decimals) tuple by output
    parameters; no String construction.
  - DisplayUtils::formatTelemetryValue handles NAN → "--" and fills a
    stack buffer; drawing uses the char* overload of drawTextLine.
*/

#include "widgets/MetricWidget.h"

#include "utils/DisplayUtils.h"

#include <cmath>
#include <cstring>

namespace {

// Degree glyph "°" for the default Adafruit GFX built-in font.
// Adafruit GFX, when _cp437 == false (the default — setCp437() is never
// called in this firmware), applies an offset: any byte value >= 0xB0 is
// incremented by 1 before the font-table lookup (see Adafruit_GFX.cpp
// "if (!_cp437 && (c >= 176)) c++;"). The degree symbol sits at font
// index 0xF8, so to reach it without CP437 mode we must send 0xF7.
// Sending 0xF8 would render font[0xF9] which is two faint dots, not "°".
static const char kDegreeSuffix[] = { (char)0xF7, '\0' };

// Resolve a metric key. Outputs the value, the display suffix, and the
// decimal count. Returns false for unknown keys.
bool resolveMetric(const char* key, const FlightInfo& f,
                   double& outValue, const char*& outSuffix, int& outDecimals) {
    if (key == nullptr) return false;
    if (strcmp(key, "alt") == 0) {
        outValue    = f.altitude_kft;
        outSuffix   = "k";
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "speed") == 0) {
        outValue    = f.speed_mph;
        outSuffix   = "mph";
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "track") == 0) {
        outValue    = f.track_deg;
        outSuffix   = kDegreeSuffix;
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "vsi") == 0) {
        outValue    = f.vertical_rate_fps;
        outSuffix   = "fps";
        outDecimals = 1;
        return true;
    }
    return false;
}

}  // namespace

bool renderMetric(const WidgetSpec& spec, const RenderContext& ctx) {
    // Minimum dimension floor — at least one 5x7 glyph must fit.
    if ((int)spec.w < WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) {
        return true;  // skip render — not an error
    }

    char buf[24];

    if (ctx.currentFlight == nullptr) {
        // AC #7 — no flight: render "--" placeholder.
        strcpy(buf, "--");
    } else {
        double value = NAN;
        const char* suffix = "";
        int decimals = 0;
        if (!resolveMetric(spec.content, *ctx.currentFlight,
                           value, suffix, decimals)) {
            // AC #10 — unknown metric key falls back to "--".
            strcpy(buf, "--");
        } else {
            // formatTelemetryValue() writes "--" when value is NAN (AC #7).
            DisplayUtils::formatTelemetryValue(value, suffix, decimals,
                                               buf, sizeof(buf));
        }
    }

    // Hardware-free test path.
    if (ctx.matrix == nullptr) return true;

    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    if (maxCols <= 0) return true;

    char out[24];
    DisplayUtils::truncateToColumns(buf, maxCols, out, sizeof(out));

    int16_t drawY = spec.y;
    if ((int)spec.h > WIDGET_CHAR_H) {
        drawY = spec.y + (int16_t)(((int)spec.h - WIDGET_CHAR_H) / 2);
    }

    DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);
    return true;
}
