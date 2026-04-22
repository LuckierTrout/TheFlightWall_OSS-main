#pragma once
/*
Purpose: Static widget registry with switch-based dispatch by widget type.

Story LE-1.2 (widget-registry-and-core-widgets) — promotes the V0 spike's
dispatch pattern to a production library. Zero virtual dispatch, zero heap
allocation on the render hot path.

Responsibilities:
  - Define the production `WidgetType` enum and `WidgetSpec` struct used by
    render-time code (mode renderers, LE-1.3 integration).
  - Translate JSON widget type strings ("text", "clock", ...) to enum values.
  - Dispatch a pre-parsed `WidgetSpec` to the matching stateless render
    function via a compile-time `switch` on `WidgetType`.

Non-responsibilities:
  - Does NOT parse JSON. Widget specs are built by the caller at layout-init
    time (LE-1.3) from validated JSON.
  - Does NOT own any state. All methods are static; the render functions are
    free functions with function-scoped statics where caching is needed.
  - Does NOT allocate heap. Dispatch is a pointer-free switch; render
    functions use stack buffers or file-scoped static buffers only.

Architecture references:
  - firmware/modes/CustomLayoutMode.cpp (V0 spike reference pattern)
  - firmware/core/LayoutStore.cpp (kAllowedWidgetTypes[] — must stay in sync)
  - firmware/interfaces/DisplayMode.h (RenderContext is reused, not redefined)
*/

#include <Arduino.h>
#include <stdint.h>

#include "interfaces/DisplayMode.h"

// ------------------------------------------------------------------
// Font / dimension constants shared across widget render functions.
// Matches the 5x7 default Adafruit GFX font + 1px spacing used by
// ClassicCardMode and the V0 spike (CustomLayoutMode.cpp).
//
// Widgets that opt into per-widget font customization (starting with
// TextWidget) use widgetFontMetrics() below to compute advance and line
// height dynamically from the widget's font_id + text_size. Other widgets
// continue to reference WIDGET_CHAR_W / WIDGET_CHAR_H directly, which
// remain the default-font @1x metrics.
// ------------------------------------------------------------------
static constexpr int WIDGET_CHAR_W = 6;
static constexpr int WIDGET_CHAR_H = 8;

// ------------------------------------------------------------------
// WidgetFontId — enumerates the fonts TextWidget (and, incrementally,
// other widgets) may request. Values are stable across stored layouts.
// ------------------------------------------------------------------
enum class WidgetFontId : uint8_t {
    Default   = 0,  // Adafruit GFX classic 5x7 glyph, 6px advance, 8px line
    TomThumb  = 1,  // Compact 3x5 glyph, 4px advance, 6px line (GFX custom)
    Picopixel = 2,  // 4x5-ish glyph, ~4px advance, 7px line (GFX custom)
};

// ------------------------------------------------------------------
// WidgetTextTransform — per-widget ASCII case transform applied before
// truncation. Stored as a uint8_t on WidgetSpec (reclaims the former
// _reserved pad byte — no struct growth). Missing / unknown values fall
// back to None so pre-existing layouts render byte-identically.
// ------------------------------------------------------------------
enum class WidgetTextTransform : uint8_t {
    None  = 0,  // Render content verbatim (default, matches legacy behavior)
    Upper = 1,  // Uppercase ASCII A-Z; leaves non-ASCII bytes untouched
    Lower = 2,  // Lowercase ASCII a-z; leaves non-ASCII bytes untouched
};

// Character metrics for a given font + integer scale factor. Callers use
// these for truncation (maxCols = w / charW), centering (midY = (h-charH)/2),
// and the minimum-dimension floors that guard undersized widget zones.
struct WidgetFontMetrics {
    int charW;
    int charH;
};

WidgetFontMetrics widgetFontMetrics(WidgetFontId font, uint8_t textSize);

// ------------------------------------------------------------------
// WidgetType — production enum, wire-compatible with JSON type strings.
// Integer values are stable (persisted to NVS or used as table indices is
// not a current requirement, but the explicit numbering prevents accidental
// reordering across stories).
// LayoutStore::kAllowedWidgetTypes[] must contain the matching string set.
// ------------------------------------------------------------------
enum class WidgetType : uint8_t {
    Text        = 0,
    Clock       = 1,
    Logo        = 2,
    FlightField = 3,   // LE-1.8
    Metric      = 4,   // LE-1.8
    Unknown     = 0xFF,
};

// ------------------------------------------------------------------
// WidgetSpec — compact value type describing one widget instance.
//
// Pre-populated at layout init time (LE-1.3) from validated JSON. Widget
// render functions take this by const-ref and MUST NOT mutate it. Field
// layout is tuned for stack allocation when an entire layout (up to 24
// widgets) is materialised at once.
// ------------------------------------------------------------------
struct WidgetSpec {
    WidgetType type;
    int16_t    x;
    int16_t    y;
    uint16_t   w;
    uint16_t   h;
    uint16_t   color;        // RGB565
    char       id[16];       // widget instance id (e.g. "w1"), 15 chars + null
    char       content[48];  // text content / clock format / logo_id
    uint8_t    align;          // 0=left, 1=center, 2=right (TextWidget only)
    uint8_t    font_id;        // WidgetFontId cast to uint8_t; 0 = default 6x8
    uint8_t    text_size;      // integer GFX setTextSize; 1-3. 0 parses as 1.
    uint8_t    text_transform; // WidgetTextTransform cast to uint8_t; 0 = none
};

// ------------------------------------------------------------------
// WidgetRegistry — static dispatch façade.
// ------------------------------------------------------------------
class WidgetRegistry {
public:
    // Dispatch to the render function matching `type`. Returns the render
    // function's success flag. For Unknown or unimplemented types, returns
    // false without touching the framebuffer.
    //
    // `type` is passed explicitly (rather than reading `spec.type`) to keep
    // the dispatcher honest about the contract and to allow future callers
    // to force a type without mutating the spec.
    static bool dispatch(WidgetType type,
                         const WidgetSpec& spec,
                         const RenderContext& ctx);

    // Convert a JSON type string (e.g. "text") to its enum value. Returns
    // WidgetType::Unknown for null/empty/unknown input. Comparison is
    // strictly case-sensitive to match the LayoutStore allowlist.
    static WidgetType fromString(const char* typeStr);

    // True iff `typeStr` maps to a non-Unknown WidgetType. Cheap predicate
    // over fromString().
    static bool isKnownType(const char* typeStr);
};
