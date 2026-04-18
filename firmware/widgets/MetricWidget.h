#pragma once
/*
Purpose: Telemetry-metric widget (LE-1.8 renderer + LE-1.10 field catalog).

Stateless free function `renderMetric()` resolves a metric id from
`spec.content` against `ctx.currentFlight` telemetry and draws the formatted
numeric value with a canonical suffix, using the DisplayUtils char* overloads
for zero heap allocation on the render hot path.

LE-1.10 catalog (single source of truth — WebPortal /api/widgets/types and the
layout-save whitelist both read through `MetricWidgetCatalog`):

  | id                  | source (FlightInfo)          | formatting                     | suffix |
  |---------------------|------------------------------|--------------------------------|--------|
  | "altitude_ft"       | altitude_kft * 1000          | integer feet                   | "ft"   |
  | "speed_kts"         | speed_mph * 0.8689762        | integer knots                  | "kts"  |
  | "heading_deg"       | track_deg                    | integer 0-359                  | "°"    |
  | "vertical_rate_fpm" | vertical_rate_fps * 60       | signed integer fpm             | "fpm"  |
  | "distance_nm"       | distance_km * 0.5399568      | 1-decimal nm                   | "nm"   |
  | "bearing_deg"       | bearing_deg                  | integer 0-359                  | "°"    |

`distance_km` / `bearing_deg` are copied from `StateVector` into `FlightInfo`
in `FlightDataFetcher` when enriching.

Fallback behavior:
  - ctx.currentFlight == nullptr           → render "--"
  - resolved value is NaN                  → render "--"
  - unknown or missing field id            → resolver uses kCatalog[0] (altitude_ft)
*/

#include "core/WidgetRegistry.h"
#include "widgets/FieldDescriptor.h"

bool renderMetric(const WidgetSpec& spec, const RenderContext& ctx);

namespace MetricWidgetCatalog {

// Pointer to the static catalog array; also writes the entry count.
// Pointer remains valid for the program lifetime.
const FieldDescriptor* catalog(size_t& outCount);

// True iff `fieldId` (non-null C string) matches an id in the catalog.
bool isKnownFieldId(const char* fieldId);

}  // namespace MetricWidgetCatalog
