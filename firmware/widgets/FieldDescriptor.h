#pragma once
/*
Purpose: Shared FieldDescriptor POD for widgets that expose a firmware-owned
catalog of selectable fields (Story LE-1.10).

Each widget that supports a field picker (FlightField, Metric) declares a
compile-time `kCatalog[]` of these and exposes a `catalog(size_t& outCount)`
accessor. WebPortal's /api/widgets/types handler reads these arrays to emit
`field_options: [{id, label, unit?}]` to the editor, and the layout save path
uses `isKnownFieldId()` to reject unknown ids with HTTP 400.

Single source of truth: the widget owns its catalog. No duplication in
WebPortal or the editor UI.
*/

#include <stdint.h>
#include <stddef.h>

struct FieldDescriptor {
    const char* id;    // wire id (stored in WidgetSpec.content)
    const char* label; // human-readable label for the editor dropdown
    const char* unit;  // optional unit string (nullptr for FlightField; "ft"/"kts"/... for Metric)
};
