/*
Purpose: MetricWidget implementation (LE-1.8 renderer + LE-1.10 field catalog).

Binds a `WidgetSpec.content` metric-id string to a FlightInfo telemetry value,
applies per-unit formatting, and draws the result with a canonical suffix.

Zero heap allocation on the render hot path:
  - Metric resolution returns (value, suffix, decimals) via output parameters.
  - DisplayUtils::formatTelemetryValue handles NaN → "--" and writes into a
    stack buffer; drawing uses the char* overload of drawTextLine.

LE-1.10 catalog (order matters — kCatalog[0] is the unknown-id fallback):
  altitude_ft       → altitude_kft * 1000          (int ft)
  speed_kts         → speed_mph * 0.8689762        (int kts)
  heading_deg       → track_deg                    (int deg)
  vertical_rate_fpm → vertical_rate_fps * 60       (signed int fpm)
  distance_nm       → distance_km * 0.5399568     (1-decimal nm)
  bearing_deg       → bearing_deg                  (int deg; ° suffix)

Conversion constants:
  mph→kts  0.8689762  (NIST exact; same factor already used elsewhere)
  kft→ft   1000
  fps→fpm  60
  km→nm    0.5399568 (same factor as story Dev Notes)
*/

#include "widgets/MetricWidget.h"

#include "utils/DisplayUtils.h"
#include "widgets/WidgetFont.h"

#include <cmath>
#include <cstring>

namespace {

// Degree glyph — Adafruit GFX default font (see LE-1.8 MetricWidget for the
// CP437 offset rationale; we keep that behavior here since it works on the
// real device and the existing font atlas hasn't changed).
static const char kDegreeSuffix[] = { (char)0xF7, '\0' };

// ---------------------------------------------------------------------------
// LE-1.10 catalog — 6 entries. Order matters: kCatalog[0] is unknown-id fallback.
// ---------------------------------------------------------------------------
static constexpr FieldDescriptor kMetricCatalog[] = {
    { "altitude_ft",       "Altitude (ft)",       "ft"  },
    { "speed_kts",         "Speed (kts)",         "kts" },
    { "speed_mph",         "Ground speed (mph)",  "mph" },
    { "heading_deg",       "Heading (deg)",       "deg" },
    { "vertical_rate_fpm", "Vert Rate (fpm)",     "fpm" },
    { "distance_nm",       "Distance (nm)",       "nm"  },
    { "bearing_deg",       "Bearing (deg)",       "deg" },
};
static constexpr size_t kMetricCatalogCount =
    sizeof(kMetricCatalog) / sizeof(kMetricCatalog[0]);

// Resolve a metric id. Outputs raw numeric value + suffix + decimals. Returns
// false for unknown ids; caller handles fallback to kCatalog[0].id.
bool resolveMetric(const char* key, const FlightInfo& f,
                   double& outValue, const char*& outSuffix, int& outDecimals) {
    if (key == nullptr) return false;

    if (strcmp(key, "altitude_ft") == 0) {
        outValue    = (isnan(f.altitude_kft) ? NAN : f.altitude_kft * 1000.0);
        outSuffix   = "ft";
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "speed_kts") == 0) {
        outValue    = (isnan(f.speed_mph) ? NAN : f.speed_mph * 0.8689762);
        outSuffix   = "kts";
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "speed_mph") == 0) {
        // Ground speed already lives in FlightInfo in mph (converted from
        // the upstream feed's m/s at fetch time), so just passthrough.
        outValue    = f.speed_mph;
        outSuffix   = "mph";
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "heading_deg") == 0) {
        outValue    = f.track_deg;
        outSuffix   = kDegreeSuffix;
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "vertical_rate_fpm") == 0) {
        outValue    = (isnan(f.vertical_rate_fps) ? NAN : f.vertical_rate_fps * 60.0);
        outSuffix   = "fpm";
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "distance_nm") == 0) {
        outValue =
            (isnan(f.distance_km) ? NAN : f.distance_km * 0.5399568);
        outSuffix   = "nm";
        outDecimals = 1;
        return true;
    }
    if (strcmp(key, "bearing_deg") == 0) {
        outValue    = f.bearing_deg;
        outSuffix   = kDegreeSuffix;
        outDecimals = 0;
        return true;
    }
    return false;
}

// Resolve `key`; if `key` is unknown, retry with kCatalog[0].id.
bool resolveMetricOrFallback(const char* key, const FlightInfo& f,
                             double& outValue, const char*& outSuffix, int& outDecimals) {
    if (resolveMetric(key, f, outValue, outSuffix, outDecimals)) return true;
    return resolveMetric(kMetricCatalog[0].id, f, outValue, outSuffix, outDecimals);
}

}  // namespace

// ---------------------------------------------------------------------------
// Catalog accessors — read by WebPortal (/api/widgets/types, save whitelist).
// ---------------------------------------------------------------------------
namespace MetricWidgetCatalog {

const FieldDescriptor* catalog(size_t& outCount) {
    outCount = kMetricCatalogCount;
    return kMetricCatalog;
}

bool isKnownFieldId(const char* fieldId) {
    if (fieldId == nullptr) return false;
    for (size_t i = 0; i < kMetricCatalogCount; ++i) {
        if (strcmp(fieldId, kMetricCatalog[i].id) == 0) return true;
    }
    return false;
}

}  // namespace MetricWidgetCatalog

// ---------------------------------------------------------------------------
// Render entry point (unchanged signature from LE-1.8).
// ---------------------------------------------------------------------------
bool renderMetric(const WidgetSpec& spec, const RenderContext& ctx) {
    WidgetFontId fontId = resolveWidgetFontId(spec.font_id);
    uint8_t textSize = spec.text_size == 0 ? 1 : spec.text_size;
    WidgetFontMetrics metrics = widgetFontMetrics(fontId, textSize);

    // Minimum dimension floor — at least one glyph must fit under the
    // chosen font + scale.
    if ((int)spec.w < metrics.charW || (int)spec.h < metrics.charH) {
        return true;  // skip render — not an error
    }

    char buf[24];

    if (ctx.currentFlight == nullptr) {
        strcpy(buf, "--");
    } else {
        double value = NAN;
        const char* suffix = "";
        int decimals = 0;
        // resolveMetricOrFallback always succeeds (fallback to kCatalog[0]);
        // if the FlightInfo field is NaN, value stays NaN and
        // formatTelemetryValue() writes "--".
        resolveMetricOrFallback(spec.content, *ctx.currentFlight,
                                value, suffix, decimals);
        DisplayUtils::formatTelemetryValue(value, suffix, decimals,
                                           buf, sizeof(buf));
    }

    // Hardware-free test path.
    if (ctx.matrix == nullptr) return true;

    int maxCols = (int)spec.w / metrics.charW;
    if (maxCols <= 0) return true;

    // Transform before truncation so the visual width drives the ellipsis.
    // Note: the degree glyph 0xF7 is not in the ASCII range the transform
    // touches, so bearing/heading suffixes survive untouched.
    applyTextTransform(buf, spec.text_transform);

    char out[24];
    DisplayUtils::truncateToColumns(buf, maxCols, out, sizeof(out));

    int16_t drawY = spec.y;
    if ((int)spec.h > metrics.charH) {
        drawY = spec.y + (int16_t)(((int)spec.h - metrics.charH) / 2);
    }

    ctx.matrix->setTextSize(textSize);
    drawY += applyWidgetFont(ctx.matrix, fontId, metrics.charH);
    DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);

    ctx.matrix->setFont(nullptr);
    ctx.matrix->setTextSize(1);
    return true;
}
