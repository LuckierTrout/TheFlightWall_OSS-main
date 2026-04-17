#pragma once
/*
Purpose: Flight-field widget render function (Story LE-1.8, AC #1/#3/#6/#7/#8).

Stateless free function. Resolves a field key in spec.content (e.g. "airline",
"ident", "origin_icao") against ctx.currentFlight and renders the resulting
string clipped to spec.w / spec.h using the DisplayUtils char* overloads for
zero heap allocation on the render hot path.

Supported field keys (AC #1):
  - "airline"      → FlightInfo.airline_display_name_full
  - "ident"        → FlightInfo.ident_icao (fallback: ident_iata, then ident)
  - "origin_icao"  → FlightInfo.origin.code_icao
  - "dest_icao"    → FlightInfo.destination.code_icao
  - "aircraft"     → FlightInfo.aircraft_display_name_short
                      (fallback: aircraft_code)

Fallback behavior (AC #7/#10):
  - ctx.currentFlight == nullptr → render "--"
  - unknown field key             → render "--"
  - resolved value is empty       → render "--"
*/

#include "core/WidgetRegistry.h"

bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx);
