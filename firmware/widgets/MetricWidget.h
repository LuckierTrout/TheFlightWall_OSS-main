#pragma once
/*
Purpose: Telemetry-metric widget render function (Story LE-1.8, AC #2/#4/#6/#7/#8).

Stateless free function. Resolves a telemetry-metric key in spec.content
(e.g. "alt", "speed", "track", "vsi") against ctx.currentFlight and
renders the resulting numeric value with its canonical suffix using the
DisplayUtils char* overloads for zero heap allocation on the render hot
path.

Supported metric keys (AC #2):
  - "alt"    → altitude_kft     0 decimals, suffix "k"   (e.g. "34k")
  - "speed"  → speed_mph        0 decimals, suffix "mph" (e.g. "487mph")
  - "track"  → track_deg        0 decimals, suffix "°"   (e.g. "247°")
  - "vsi"    → vertical_rate_fps 1 decimal,  suffix "fps" (e.g. "-12.5fps")

Fallback behavior (AC #7/#10):
  - ctx.currentFlight == nullptr     → render "--"
  - resolved value is NAN            → render "--"
  - unknown metric key               → render "--"
*/

#include "core/WidgetRegistry.h"

bool renderMetric(const WidgetSpec& spec, const RenderContext& ctx);
