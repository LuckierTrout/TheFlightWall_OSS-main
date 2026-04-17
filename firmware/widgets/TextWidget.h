#pragma once
/*
Purpose: Static-text widget render function (Story LE-1.2, AC #4/#7).

Stateless free function. Reads spec.content (null-terminated), spec.color,
and spec.align to render a single text line clipped to spec.w / spec.h.
Uses DisplayUtils char* overloads for zero heap allocation on the render
hot path.
*/

#include "core/WidgetRegistry.h"

bool renderText(const WidgetSpec& spec, const RenderContext& ctx);
