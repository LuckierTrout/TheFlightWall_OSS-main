#pragma once
/*
Purpose: Logo widget render function (Story LE-1.5 — real RGB565 pipeline).

Renders a 32×32 RGB565 logo loaded from LittleFS via LogoManager into the
caller-supplied shared buffer (ctx.logoBuffer). Missing logos fall back to a
PROGMEM airplane sprite — no heap allocation, no per-frame malloc.

NOTE: Must render via the RGB565 bitmap path (DisplayUtils::drawBitmapRGB565).
Spike measurements (epic LE-1) showed per-pixel rectangle fills at ~2.67×
baseline vs ~1.23× for the bitmap blit on a 32×32 region. See AC #6 / AC #9.
*/

#include "core/WidgetRegistry.h"

bool renderLogo(const WidgetSpec& spec, const RenderContext& ctx);
