/*
Purpose: FlightFieldWidget implementation (LE-1.8 + LE-1.10 field catalog).

Binds a `WidgetSpec.content` field-id string (e.g. "callsign", "aircraft_type")
to a FlightInfo member and draws it as a single text line. Falls back to "--"
when ctx.currentFlight is null, the resolved value is empty, or the field id is
unknown (after a catalog-id fallback to kCatalog[0]).

Zero heap allocation on the render hot path:
  - Field resolution hands back `const char*` into FlightInfo's Arduino String
    storage via `.c_str()` — no String temporaries.
  - Truncation/drawing uses the DisplayUtils char* overloads with a stack buffer.

LE-1.10 catalog (single source of truth for /api/widgets/types + save-time
whitelist):
  callsign         → ident_icao (fallback ident_iata → ident)
  airline          → airline_display_name_full
  aircraft_type    → aircraft_display_name_short (fallback aircraft_code)
  origin_icao      → origin.code_icao
  destination_icao → destination.code_icao
  flight_number    → ident (labelled alias of callsign)
*/

#include "widgets/FlightFieldWidget.h"

#include "utils/DisplayUtils.h"
#include "widgets/WidgetFont.h"

#include <cstring>

namespace {

// ---------------------------------------------------------------------------
// LE-1.10 catalog — 6 firmware-owned field ids. Order matters: kCatalog[0] is
// the fallback target when a widget has an unknown or missing field id.
// ---------------------------------------------------------------------------
static constexpr FieldDescriptor kFlightFieldCatalog[] = {
    { "callsign",         "Callsign",         nullptr },
    { "airline",          "Airline",          nullptr },
    { "aircraft_type",    "Aircraft",         nullptr },
    { "aircraft_short",   "Aircraft (model)", nullptr },
    { "aircraft_full",    "Aircraft (full)",  nullptr },
    { "origin_icao",      "Origin (ICAO)",    nullptr },
    { "origin_iata",      "Origin (IATA)",    nullptr },
    { "destination_icao", "Destination (ICAO)", nullptr },
    { "destination_iata", "Destination (IATA)", nullptr },
    { "flight_number",    "Flight #",         nullptr },
};
static constexpr size_t kFlightFieldCatalogCount =
    sizeof(kFlightFieldCatalog) / sizeof(kFlightFieldCatalog[0]);

// Resolve a field id against a FlightInfo. Returns a C-string pointer into the
// FlightInfo's String storage (valid for the lifetime of the FlightInfo), or
// nullptr if the key is unknown. Caller handles empty-string → "--" fallback.
//
// Unknown ids are caught upstream by resolveFieldOrFallback(), which retries
// with kCatalog[0].id — this function itself is strict strcmp.
const char* resolveField(const char* key, const FlightInfo& f) {
    if (key == nullptr) return nullptr;

    if (strcmp(key, "callsign") == 0 || strcmp(key, "flight_number") == 0) {
        // callsign and flight_number resolve identically — distinct labels,
        // same backing field (AC #6 clarification: acceptable alias).
        if (f.ident_icao.length() > 0) return f.ident_icao.c_str();
        if (f.ident_iata.length() > 0) return f.ident_iata.c_str();
        return f.ident.c_str();
    }
    if (strcmp(key, "airline") == 0) {
        // Precedence mirrors the aircraft resolvers: prefer the CDN-resolved
        // display name, fall back to the ICAO code so the widget renders
        // "UAL" immediately rather than "--" while the per-cycle CDN budget
        // works through fresh callsigns. Once the cache warms up the field
        // auto-upgrades to "United Airlines" on the next render.
        if (f.airline_display_name_full.length() > 0) {
            return f.airline_display_name_full.c_str();
        }
        return f.operator_icao.c_str();
    }
    if (strcmp(key, "aircraft_type") == 0) {
        // AC #6 precedence: display_name_short → aircraft_code → empty.
        if (f.aircraft_display_name_short.length() > 0) {
            return f.aircraft_display_name_short.c_str();
        }
        return f.aircraft_code.c_str();
    }
    if (strcmp(key, "aircraft_short") == 0) {
        // Prefer the marketing name ("737-800") with a fallback to the raw
        // ICAO code so the widget never collapses to empty when CDN lookup
        // missed.
        if (f.aircraft_display_name_short.length() > 0) {
            return f.aircraft_display_name_short.c_str();
        }
        return f.aircraft_code.c_str();
    }
    if (strcmp(key, "aircraft_full") == 0) {
        // Prefer the full name ("Boeing 737-800"); fall back to the short
        // form and then the raw code. Three-layer fallback matches the
        // firmware's tolerance for sparse CDN data.
        if (f.aircraft_display_name_full.length() > 0) {
            return f.aircraft_display_name_full.c_str();
        }
        if (f.aircraft_display_name_short.length() > 0) {
            return f.aircraft_display_name_short.c_str();
        }
        return f.aircraft_code.c_str();
    }
    if (strcmp(key, "origin_icao") == 0) {
        return f.origin.code_icao.c_str();
    }
    if (strcmp(key, "origin_iata") == 0) {
        return f.origin.code_iata.c_str();
    }
    if (strcmp(key, "destination_icao") == 0) {
        return f.destination.code_icao.c_str();
    }
    if (strcmp(key, "destination_iata") == 0) {
        return f.destination.code_iata.c_str();
    }
    return nullptr;
}

// Resolve `key`; if `key` is unknown or null, retry with kCatalog[0].id.
// Returns nullptr only when the fallback id is itself unresolvable (which
// would be a bug in the catalog/resolver sync, not a data issue).
const char* resolveFieldOrFallback(const char* key, const FlightInfo& f) {
    const char* v = resolveField(key, f);
    if (v != nullptr) return v;
    return resolveField(kFlightFieldCatalog[0].id, f);
}

}  // namespace

// ---------------------------------------------------------------------------
// Catalog accessors — read by WebPortal (/api/widgets/types, save whitelist).
// ---------------------------------------------------------------------------
namespace FlightFieldWidgetCatalog {

const FieldDescriptor* catalog(size_t& outCount) {
    outCount = kFlightFieldCatalogCount;
    return kFlightFieldCatalog;
}

bool isKnownFieldId(const char* fieldId) {
    if (fieldId == nullptr) return false;
    for (size_t i = 0; i < kFlightFieldCatalogCount; ++i) {
        if (strcmp(fieldId, kFlightFieldCatalog[i].id) == 0) return true;
    }
    return false;
}

}  // namespace FlightFieldWidgetCatalog

// ---------------------------------------------------------------------------
// Render entry point (unchanged signature from LE-1.8).
// ---------------------------------------------------------------------------
bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx) {
    WidgetFontId fontId = resolveWidgetFontId(spec.font_id);
    uint8_t textSize = spec.text_size == 0 ? 1 : spec.text_size;
    WidgetFontMetrics metrics = widgetFontMetrics(fontId, textSize);

    // Minimum dimension floor — at least one glyph must fit under the
    // chosen font + scale.
    if ((int)spec.w < metrics.charW || (int)spec.h < metrics.charH) {
        return true;  // skip render — not an error
    }

    // Resolve the field value. Null flight → "--". Resolver handles unknown-id
    // fallback to kCatalog[0]; empty resolved value also maps to "--".
    const char* value = nullptr;
    if (ctx.currentFlight != nullptr) {
        value = resolveFieldOrFallback(spec.content, *ctx.currentFlight);
    }
    if (value == nullptr || value[0] == '\0') {
        value = "--";
    }

    // Hardware-free test path.
    if (ctx.matrix == nullptr) return true;

    int maxCols = (int)spec.w / metrics.charW;
    if (maxCols <= 0) return true;

    // Copy into a scratch buffer so the case transform doesn't mutate the
    // FlightInfo String storage `value` points into (another widget might
    // read the same field in the same frame).
    char scratch[48];
    size_t valueLen = strlen(value);
    if (valueLen >= sizeof(scratch)) valueLen = sizeof(scratch) - 1;
    memcpy(scratch, value, valueLen);
    scratch[valueLen] = '\0';
    applyTextTransform(scratch, spec.text_transform);

    char out[48];
    DisplayUtils::truncateToColumns(scratch, maxCols, out, sizeof(out));

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
