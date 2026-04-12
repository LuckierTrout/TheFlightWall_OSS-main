## Epic ds-2: Live Flight Card Mode

Users see a richer flight display with real-time telemetry — altitude, ground speed, heading, and vertical rate — with the layout adapting gracefully when zone dimensions cannot fit all fields. A visual climb/descend/level indicator makes aircraft state immediately clear.

### Story ds-2.1: Live Flight Card Mode — Enriched Telemetry Rendering

As a flight enthusiast,
I want to see a richer flight card displaying airline, route, altitude, ground speed, heading, and vertical rate,
So that I get detailed real-time information about aircraft overhead at a glance.

**Acceptance Criteria:**

**Given** the mode system from Epic 1 is operational and ClassicCardMode is the active default
**When** LiveFlightCardMode is implemented in modes/LiveFlightCardMode.h/.cpp
**Then** render() draws an enriched flight card with: airline name, route (origin → destination), altitude, ground speed, heading, and vertical rate
**And** the mode is registered in MODE_TABLE[] in main.cpp (one line addition per AR5)
**And** getName() returns "Live Flight Card" and getZoneLayout() returns zone metadata describing all content regions
**And** getMemoryRequirement() reports the mode's worst-case heap need
**And** per-frame rendering completes within 16ms (NFR P2)
**And** stack-local allocations during render() are under 512 bytes (NFR S3)
**And** telemetry values are formatted using DisplayUtils helpers (e.g., formatTelemetryValue)
**And** the mode renders only within its zone bounds

**Requirements:** FR14, NFR P2, NFR S3

### Story ds-2.2: Adaptive Field Dropping & Vertical Direction Indicator

As a flight enthusiast,
I want the Live Flight Card to gracefully adapt when the display zone is too small for all fields, and to show a visual climb/descend/level indicator,
So that I always see the most important information and can instantly tell what an aircraft is doing.

**Acceptance Criteria:**

**Given** LiveFlightCardMode renders the full telemetry layout from Story 2.1
**When** the zone dimensions are insufficient to display all six telemetry fields
**Then** fields are dropped in priority order (lowest priority dropped first): vertical rate → heading → ground speed → altitude → route → airline name
**And** the remaining fields are repositioned to use available space without gaps or overlapping text
**And** at minimum zone size, at least the airline name is always displayed

**Given** a flight has a positive vertical rate
**When** the Live Flight Card renders
**Then** a distinct visual element (e.g., up-arrow glyph or indicator) is displayed indicating "climbing"

**Given** a flight has a negative vertical rate
**When** the Live Flight Card renders
**Then** a distinct visual element is displayed indicating "descending"

**Given** a flight has zero or near-zero vertical rate
**When** the Live Flight Card renders
**Then** a distinct visual element is displayed indicating "level"

**And** the vertical direction indicator is visually distinguishable from surrounding text at typical viewing distance
**And** adaptive dropping and direction indicator work correctly across all registered zone configurations

**Requirements:** FR15, FR16

---

