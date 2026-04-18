#pragma once
/*
Purpose: Flight-identity field widget (LE-1.8 renderer + LE-1.10 field catalog).

Stateless free function `renderFlightField()` resolves a field id from
`spec.content` against `ctx.currentFlight` and draws a single text line,
clipped via the DisplayUtils char* overloads for zero heap allocation on
the render hot path.

LE-1.10 catalog (single source of truth — WebPortal /api/widgets/types and
the layout-save whitelist both read through `FlightFieldWidgetCatalog`):
  - "callsign"         → FlightInfo.ident_icao (fallback ident_iata → ident)
  - "airline"          → FlightInfo.airline_display_name_full
  - "aircraft_type"    → FlightInfo.aircraft_display_name_short
                          (fallback: aircraft_code)
  - "origin_icao"      → FlightInfo.origin.code_icao
  - "destination_icao" → FlightInfo.destination.code_icao
  - "flight_number"    → FlightInfo.ident (alias of callsign with distinct label)

Fallback behavior:
  - ctx.currentFlight == nullptr        → render "--"
  - unknown or missing field id         → resolver uses kCatalog[0] (callsign)
  - resolved value is empty             → render "--"
*/

#include "core/WidgetRegistry.h"
#include "widgets/FieldDescriptor.h"

bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx);

namespace FlightFieldWidgetCatalog {

// Pointer to the static catalog array; also writes the entry count.
// Pointer remains valid for the program lifetime.
const FieldDescriptor* catalog(size_t& outCount);

// True iff `fieldId` (non-null C string) matches an id in the catalog.
bool isKnownFieldId(const char* fieldId);

}  // namespace FlightFieldWidgetCatalog
