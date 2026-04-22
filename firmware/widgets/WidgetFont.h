#pragma once
/*
  Shared font helpers for text-rendering widgets (Text, Clock, Metric,
  FlightField). Keeping these in one place means adding a new font to the
  catalog only requires:
    1. Add an enum value in core/WidgetRegistry.h
    2. Teach widgetFontMetrics() the advance/line height
    3. Teach applyWidgetFont() how to call setFont()
    4. Update the editor schema and preview.js glyph tables
  Every widget automatically picks up the new font through the shared
  resolveWidgetFontId() + applyWidgetFont() pair.
*/

#include "core/WidgetRegistry.h"

class FastLED_NeoMatrix;

// Map a persisted font_id byte to its enum value. Unknown values fall back
// to the default 6x8 font so malformed layouts never crash.
WidgetFontId resolveWidgetFontId(uint8_t raw);

// Apply (or restore) a GFX font ahead of a drawTextLine call and return the
// Y offset the caller should add to its drawY. Custom GFX fonts use a
// baseline cursor, so a TextWidget that centered its draw using top-left
// semantics needs to shift down by (charH - 1). Default (nullptr) font
// returns zero.
int16_t applyWidgetFont(FastLED_NeoMatrix* matrix, WidgetFontId fontId, int charH);

// In-place ASCII case transform on a null-terminated buffer. Called by
// text-rendering widgets (Text, Metric, FlightField) before truncation so
// the rendered width reflects the transformed content. `mode` is a
// WidgetTextTransform cast to uint8_t; unknown values are treated as None
// and leave `buf` unchanged. Only the ASCII A-Z / a-z range is toggled,
// matching the upstream data's ASCII-only guarantee (callsigns, ICAOs,
// airline names are all ASCII). No-op on null / empty input.
void applyTextTransform(char* buf, uint8_t mode);
