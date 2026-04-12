## Epic dl-2: Multi-Flight Awareness — Departures Board

The owner sees all tracked flights at a glance in an airport-style scrolling list. Rows appear and disappear dynamically as aircraft enter and leave the tracking radius. No cycling through one-at-a-time cards.

### Story dl-2.1: Departures Board Multi-Row Rendering

As an owner,
I want to see multiple flights displayed simultaneously in a list format on the LED matrix,
So that I can see all tracked flights at a glance without cycling through one-at-a-time cards.

**Acceptance Criteria:**

**Given** Departures Board mode is active and flights are in range
**When** the flight data pipeline provides N flights (where N >= 1)
**Then** the LED matrix renders up to the configured maximum rows (default 4) simultaneously, each showing callsign, altitude, and ground speed

**Given** Departures Board mode is active
**When** rendering a single row
**Then** the row displays the flight callsign and at least two telemetry fields from the owner-configurable field set (default: altitude and ground speed), matching the same field set available in Live Flight Card / Mode Picker (FR6)

**Given** more flights are in range than the configured maximum rows
**When** Departures Board renders
**Then** only the first N rows (up to max) are displayed and remaining flights are not shown (no scrolling in MVP)

**Given** the `m_depbd_rows` NVS key is absent
**When** `ConfigManager::getModeSetting("depbd", "rows", 4)` is called
**Then** the method returns the default value (4) without error (FR33 safe defaults)

**Given** the `m_depbd_rows` NVS key is set to a value between 1 and 4
**When** Departures Board renders
**Then** the display renders exactly that many rows maximum

**Given** DeparturesBoardMode is implemented
**When** the firmware is compiled
**Then** `DeparturesBoardMode` implements the `DisplayMode` interface, declares `DEPBD_SETTINGS[]` and `DEPBD_SCHEMA` as `static const` in `DeparturesBoardMode.h` (rule 29), and is registered in `MODE_TABLE[]` in `main.cpp` with a non-null `settingsSchema` pointer

**Given** zero flights are in range
**When** Departures Board mode is active (e.g., forced via manual switch)
**Then** the display shows an empty state (no rows) rather than a blank or corrupted screen — the idle fallback to Clock Mode is handled by ModeOrchestrator (Epic 1), not by Departures Board itself

**Given** Departures Board is the second new mode added to MODE_TABLE
**When** the dashboard Mode Picker queries `GET /api/display/modes`
**Then** the response includes DeparturesBoardMode alongside ClockMode and existing modes, validating FR41 (compile-time registry extensibility) and FR42 (Mode Picker discovery)

### Story dl-2.2: Dynamic Row Add and Remove on Flight Changes

As an owner,
I want Departures Board rows to appear and disappear in real-time as flights enter and leave tracking range,
So that the display always reflects the current sky without manual refresh.

**Acceptance Criteria:**

**Given** Departures Board mode is active and showing N rows
**When** a new flight enters tracking range and the flight data vector updates
**Then** a new row appears on the display within one frame of the flight list change (NFR4: <50ms at ≥20fps)

**Given** Departures Board mode is active and showing N rows
**When** a flight leaves tracking range and the flight data vector updates
**Then** the corresponding row is removed from the display within one frame of the flight list change (NFR4)

**Given** Departures Board mode is active and showing the maximum number of rows
**When** an additional flight enters range
**Then** the new flight is not displayed (row count stays at max) and no visual corruption occurs

**Given** Departures Board mode is active and showing N rows
**When** a flight leaves range and a new flight enters range in the same poll cycle
**Then** the departing row is removed and the arriving row is added, resulting in the correct current flight list

**Given** row additions and removals occur
**When** the display updates
**Then** rows are mutated in-place during `render()` — no full-screen redraw per individual row change — and peak heap usage remains under the ESP32 usable heap ceiling (NFR13) with no progressive heap degradation (NFR17)

---

