---
stepsCompleted: [1, 2, 3, 4]
completedDate: '2026-04-11'
inputDocuments: ['_bmad-output/planning-artifacts/prd-display-system.md', '_bmad-output/planning-artifacts/architecture.md', '_bmad-output/planning-artifacts/ux-design-specification-display-system.md']
---

# TheFlightWall Display System Release - Epic Breakdown

## Overview

This document provides the complete epic and story breakdown for the Display System Release, decomposing the requirements from the PRD, UX Design Specification, and Architecture decisions into implementable stories.

## Requirements Inventory

### Functional Requirements

FR1: Device can render flight data through interchangeable display modes that support activation, per-frame drawing, teardown, and a reported worst-case heap requirement before activation
FR2: Device can report the memory requirement of a display mode before activation
FR3: Display mode can receive a rendering context containing the matrix object, zone bounds, and current brightness level
FR4: Display mode can render flight data within its allocated zone bounds without affecting pixels outside its zone
FR5: Device can maintain a registry of all display modes shipped in the firmware build
FR6: Device can activate a display mode by executing its full lifecycle: teardown current mode, validate heap, initialize new mode, begin rendering
FR7: Device can serialize concurrent mode switch requests, queuing a switch initiated during an in-progress transition
FR8: Device can validate that sufficient free heap exists before activating a requested mode
FR9: Device can restore the previous mode when a requested mode activation fails due to insufficient heap
FR10: Device can enumerate all registered modes with their names for external consumers (API, UI)
FR11: Device can render a three-line flight card (airline name, route, aircraft type) pixel-identical to the Classic Card baseline
FR12: When two or more flights are available, device can cycle through them at a configurable interval in Classic Card mode
FR13: Classic Card mode satisfies the same pixel-parity acceptance criteria as FR11
FR14: Device can render an enriched flight card displaying airline, route, altitude, ground speed, heading, and vertical rate
FR15: Device can adaptively drop telemetry fields in priority order when zone dimensions are insufficient for the full layout
FR16: Device can indicate flight vertical direction (climbing, descending, level) as a distinct visual element
FR17: User can view a list of all available display modes in the web dashboard
FR18: User can view a schematic wireframe preview of each mode's zone layout with labeled content regions
FR19: User can activate a display mode by selecting it in the Mode Picker
FR20: User can see the currently active mode indicated in the Mode Picker
FR21: User can see a "Switching..." transition state in the Mode Picker during a mode change
FR22: User can see the Mode Picker update to "Active" only after the firmware confirms the new mode is rendering
FR23: User can see an error message in the Mode Picker when a mode activation fails
FR24: User can access the Mode Picker from the dashboard navigation at all times
FR25: User can see a one-time "New display modes available" notification after upgrading from Foundation Release
FR26: User can dismiss the upgrade notification, and it does not reappear on subsequent visits
FR27: Device can store the active display mode selection in NVS
FR28: Device can restore the last-active display mode from NVS on boot
FR29: Device can default to Classic Card mode on first boot after upgrading from Foundation Release
FR30: External client can retrieve the current active mode and list of available modes via HTTP API
FR31: External client can request a mode change via HTTP API
FR32: API can return a transition state distinguishing between "switch initiated" and "switch complete"
FR33: API can return an error response with reason when a mode switch fails
FR34: Device can render all flight output through the currently active display mode
FR35: Device can ensure only the shared display pipeline commits the matrix each frame
FR36: Device can always retain at least one valid active display mode

### NonFunctional Requirements

NFR P1: Mode switch full lifecycle under 2 seconds perceived
NFR P2: Per-frame display work within 16ms, non-blocking
NFR P3: Mode Picker UI load within 1 second
NFR P4: Mode registry enumeration under 10ms, zero heap allocation
NFR S1: No net heap loss after 100 consecutive switches
NFR S2: Rapid switching (10 toggles in 3 seconds) safe — no undefined states, corruption, or watchdog resets
NFR S3: Stack-local allocations during per-frame drawing under 512 bytes
NFR S4: Mode activation must leave minimum 30KB free heap
NFR S5: Modes must not override brightness (read-only in rendering context)
NFR S6: display_mode NVS key must not collide with existing Foundation Release keys
NFR C1: Classic Card pixel-identical to baseline — verified across 5+ flight cards
NFR C2: Mode system must not interfere with Foundation Release features (OTA, night mode, NTP, dashboard, health)
NFR C3: Preserve cooperative scheduling — no new concurrent rendering tasks, no blocking delays on display hot path
NFR C4: Mode Picker UI matches existing dashboard styling, navigation, and technology patterns

### Additional Requirements (Architecture)

AR1: Heap baseline measurement — measure ESP.getFreeHeap() after full Foundation boot before writing any Display System code to establish the mode memory budget ceiling
AR2: Cross-core atomic flag coordination — mode switch requests from WebPortal (Core 1) execute on display task (Core 0) via atomic flag pattern matching existing g_configChanged
AR3: NeoMatrixDisplay responsibility split — post-refactoring owns hardware init, frame commit (FastLED.show()), shared resources (logo buffer), and emergency fallback renderer. Rendering logic migrates to mode classes.
AR4: DisplayUtils extraction — shared rendering helpers (drawTextLine, truncateToColumns, formatTelemetryValue, drawBitmapRGB565) extracted from NeoMatrixDisplay into utils/DisplayUtils.h as free functions
AR5: MODE_TABLE static registration — adding a new mode requires exactly two touch points: one new .h/.cpp pair in modes/ and one line in MODE_TABLE[] in main.cpp
AR6: RenderContext isolation enforcement — modes access data ONLY through const RenderContext reference. No ConfigManager, no FastLED.show(), no brightness modification.
AR7: NVS key namespace — display_mode key in "flightwall" namespace, verified non-colliding with all 23+ existing keys
AR8: Emergency fallback renderer — displaySingleFlightCard() retained in NeoMatrixDisplay for FR36 safety net when no mode can init

### UX Design Requirements

UX-DR1: Mode Card component with 5 CSS states — idle (1px accent-dim border, tappable), switching (pulsing border, opacity 0.7, "Switching..." label), active (4px accent left border, status dot, "Active" label), disabled (opacity 0.5, pointer-events none), error (warning text below schematic, 5s auto-dismiss)
UX-DR2: Mode Schematic Preview rendered as CSS Grid from zone metadata JSON — grid container 80px height, zone divs positioned via grid-row/grid-column, data-driven from getZoneLayout() API response
UX-DR3: Upgrade Notification Banner with dual-source dismissal — firmware upgrade_notification API flag + localStorage check. Three actions: "Try Live Flight Card" (activates immediately), "Browse Modes" (scrolls to Mode Picker), dismiss X. All three clear both sources.
UX-DR4: Entire non-active mode card is the tap target (Philips Hue scene picker pattern) — not a button within the card
UX-DR5: Reduced-motion fallback — @media (prefers-reduced-motion: reduce) disables pulsing border animation, static accent-dim border, full opacity, state communicated via text only
UX-DR6: Focus management — after mode switch completes, focus moves from now-active card to the previously-active (now idle) card. After notification banner removal, focus moves to the Mode Picker section.
UX-DR7: Keyboard navigation — idle cards are role="button" tabindex="0", Enter/Space triggers activation. Active card has aria-current="true", not in tab order as action.
UX-DR8: ARIA attributes for all states — switching: aria-busy="true", disabled: aria-disabled="true", error: role="alert" aria-live="polite", schematic container: aria-label with layout description, zone divs: aria-hidden="true"
UX-DR9: Responsive layout — phone-first single column, desktop (min-width: 1024px, NEW breakpoint) two-column mode card grid. Inherit existing 600px breakpoint.
UX-DR10: Disabled-during-switch — while any card is switching, all sibling cards get .disabled (pointer-events none, opacity 0.5). Prevents ambiguous multi-action state.
UX-DR11: Scoped card-level error messages — "Not enough memory to activate this mode. Current mode restored." in --warning text on the failed card. Auto-dismiss after 5 seconds or next interaction.
UX-DR12: Data-driven rendering — Mode Picker section uses <div id="mode-picker"> placeholder (~10 lines HTML). JS builds cards dynamically from GET /api/display/modes response. Zero UI code changes for new modes.

### FR Coverage Map

FR1: Epic 1 — DisplayMode interface with lifecycle (init, render, teardown, heap requirement)
FR2: Epic 1 — Memory requirement reporting before activation
FR3: Epic 1 — RenderContext struct (matrix, zone bounds, brightness)
FR4: Epic 1 — Zone-bounded rendering enforcement
FR5: Epic 1 — Static mode registry
FR6: Epic 1 — Full mode activation lifecycle
FR7: Epic 1 — Concurrent switch serialization
FR8: Epic 1 — Heap validation before activation
FR9: Epic 1 — Fallback to previous mode on failure
FR10: Epic 1 — Mode enumeration for external consumers
FR11: Epic 1 — Classic Card pixel-identical three-line rendering
FR12: Epic 1 — Flight cycling at configurable interval
FR13: Epic 1 — Classic Card pixel-parity acceptance
FR14: Epic 2 — Enriched flight card with telemetry fields
FR15: Epic 2 — Adaptive field dropping by priority
FR16: Epic 2 — Vertical direction visual indicator
FR17: Epic 3 — Mode list in dashboard
FR18: Epic 3 — Schematic wireframe preview per mode
FR19: Epic 3 — Activate mode from Mode Picker
FR20: Epic 3 — Active mode indication in Mode Picker
FR21: Epic 3 — Switching transition state
FR22: Epic 3 — Active confirmation after firmware confirms
FR23: Epic 3 — Error message on activation failure
FR24: Epic 3 — Mode Picker accessible from dashboard nav
FR25: Epic 3 — Upgrade notification after Foundation upgrade
FR26: Epic 3 — Dismiss upgrade notification permanently
FR27: Epic 3 — Store active mode in NVS
FR28: Epic 3 — Restore mode from NVS on boot
FR29: Epic 3 — Default to Classic Card on first boot after upgrade
FR30: Epic 3 — HTTP API for mode list and active mode
FR31: Epic 3 — HTTP API for mode change request
FR32: Epic 3 — API transition state in response
FR33: Epic 3 — API error response with reason
FR34: Epic 1 — All flight output through active display mode
FR35: Epic 1 — Shared pipeline owns frame commit
FR36: Epic 1 — Always retain at least one valid active mode

### NFR Coverage Map

NFR P1: Epic 1 (mode switch lifecycle) + Epic 3 (perceived by user)
NFR P2: Epic 1 (per-frame 16ms) + Epic 2 (new mode rendering)
NFR P3: Epic 3 — Mode Picker UI load within 1 second
NFR P4: Epic 1 — Registry enumeration under 10ms
NFR S1: Epic 1 — No net heap loss after 100 consecutive switches
NFR S2: Epic 1 — Rapid switching safety
NFR S3: Epic 1 + Epic 2 — Stack-local allocations under 512 bytes
NFR S4: Epic 1 — 30KB minimum free heap after activation
NFR S5: Epic 1 — Read-only brightness in RenderContext
NFR S6: Epic 3 — NVS key non-collision
NFR C1: Epic 1 — Classic Card pixel-identical baseline
NFR C2: Epic 1 — No interference with Foundation features
NFR C3: Epic 1 — Cooperative scheduling preserved
NFR C4: Epic 3 — Mode Picker matches dashboard styling

### AR Coverage Map

AR1: Epic 1 — Heap baseline measurement
AR2: Epic 1 — Cross-core atomic flag coordination
AR3: Epic 1 — NeoMatrixDisplay responsibility split
AR4: Epic 1 — DisplayUtils extraction
AR5: Epic 1 — MODE_TABLE static registration
AR6: Epic 1 — RenderContext isolation enforcement
AR7: Epic 3 — NVS key namespace verification
AR8: Epic 1 — Emergency fallback renderer

### UX-DR Coverage Map

UX-DR1: Epic 3 — Mode Card with 5 CSS states
UX-DR2: Epic 3 — Mode Schematic Preview via CSS Grid
UX-DR3: Epic 3 — Upgrade Notification Banner with dual-source dismissal
UX-DR4: Epic 3 — Entire card as tap target
UX-DR5: Epic 3 — Reduced-motion fallback
UX-DR6: Epic 3 — Focus management after mode switch and banner removal
UX-DR7: Epic 3 — Keyboard navigation
UX-DR8: Epic 3 — ARIA attributes for all states
UX-DR9: Epic 3 — Responsive layout with 1024px breakpoint
UX-DR10: Epic 3 — Disabled-during-switch for sibling cards
UX-DR11: Epic 3 — Scoped card-level error messages
UX-DR12: Epic 3 — Data-driven rendering from API response

## Epic List

**Dependency Note:** Epic 1 is foundational — Epics 2 and 3 both depend on Epic 1 but not on each other. After Epic 1 is complete, Epics 2 and 3 can be developed in parallel.

### Epic 1: Display Mode Architecture & Classic Card Migration
The device renders flight data through a pluggable display mode system, with the existing Classic Card output migrated into the first mode — pixel-identical to the current Foundation Release output. The emergency fallback renderer ensures the display always works even if a mode fails to initialize.
**Verifiable intermediate state:** After Epic 1 completion, `pio run` builds cleanly, device boots, and Classic Card renders identically to Foundation Release. This is the regression anchor for Epics 2 and 3.
**Implementation note:** The heap baseline measurement (AR1) must be the very first story — every subsequent memory decision depends on that measurement.
**FRs covered:** FR1, FR2, FR3, FR4, FR5, FR6, FR7, FR8, FR9, FR10, FR11, FR12, FR13, FR34, FR35, FR36
**NFRs covered:** P1, P2, P4, S1, S2, S3, S4, S5, C1, C2, C3
**ARs covered:** AR1, AR2, AR3, AR4, AR5, AR6, AR8

### Epic 2: Live Flight Card Mode
Users see a richer flight display with real-time telemetry — altitude, ground speed, heading, and vertical rate — with the layout adapting gracefully when zone dimensions cannot fit all fields. A visual climb/descend/level indicator makes aircraft state immediately clear.
**FRs covered:** FR14, FR15, FR16
**NFRs covered:** P2, S3

### Epic 3: Mode Switching API & Dashboard Mode Picker
Users gain control over how their FlightWall displays flight information — choosing between display modes from the web dashboard with schematic previews of each mode's layout. Clear transition feedback, error handling, and NVS persistence ensure the user's choice is respected across power cycles. An upgrade notification guides first-time users to explore the new modes.
**FRs covered:** FR17, FR18, FR19, FR20, FR21, FR22, FR23, FR24, FR25, FR26, FR27, FR28, FR29, FR30, FR31, FR32, FR33
**NFRs covered:** P1, P3, S6, C4
**ARs covered:** AR7
**UX-DRs covered:** UX-DR1, UX-DR2, UX-DR3, UX-DR4, UX-DR5, UX-DR6, UX-DR7, UX-DR8, UX-DR9, UX-DR10, UX-DR11, UX-DR12

---

## Epic 1: Display Mode Architecture & Classic Card Migration

The device renders flight data through a pluggable display mode system, with the existing Classic Card output migrated into the first mode — pixel-identical to the current Foundation Release output. The emergency fallback renderer ensures the display always works even if a mode fails to initialize.

### Story 1.1: Heap Baseline Measurement & Core Abstractions

As a firmware developer,
I want to measure the available heap after Foundation boot and define the DisplayMode interface and RenderContext struct,
So that all subsequent mode implementations have a documented memory budget and a stable contract to code against.

**Acceptance Criteria:**

**Given** the device has completed Foundation Release boot (WiFi connected, NTP synced, all existing tasks running)
**When** the heap baseline measurement executes
**Then** ESP.getFreeHeap() is logged to Serial and the value is documented as a constant in a header file (e.g., HEAP_BASELINE_AFTER_BOOT)
**And** the DisplayMode abstract class is defined in interfaces/DisplayMode.h with pure virtual methods: init(), render(const RenderContext&), teardown(), getMemoryRequirement(), getName(), getZoneLayout()
**And** the RenderContext struct is defined with: matrix reference, zone bounds (x, y, width, height), current brightness level — all members are const, and the render() method receives RenderContext as `const&` to enforce read-only access at compile time (NFR S5)
**And** the ZoneDescriptor struct is defined for getZoneLayout() return values with: label, gridRow, gridColumn fields
**And** the existing device behavior is completely unchanged — this story adds only new files and a Serial.println

**Requirements:** FR1, FR2, FR3, AR1, AR6

### Story 1.2: DisplayUtils Extraction

As a firmware developer,
I want shared rendering helper functions extracted from NeoMatrixDisplay into a standalone utility header,
So that both NeoMatrixDisplay and future display mode classes can use them without circular dependencies.

**Acceptance Criteria:**

**Given** NeoMatrixDisplay contains rendering helper functions (drawTextLine, truncateToColumns, formatTelemetryValue, drawBitmapRGB565)
**When** the extraction is complete
**Then** utils/DisplayUtils.h contains these functions as free functions (not class methods)
**And** NeoMatrixDisplay.cpp includes DisplayUtils.h and delegates to the extracted functions
**And** all existing rendering output is pixel-identical to pre-extraction behavior
**And** the project compiles cleanly with no new warnings
**And** no existing #include dependencies are broken

**Requirements:** AR4

### Story 1.3: ModeRegistry & Static Registration System

As a firmware developer,
I want a ModeRegistry that manages a static table of display modes with activation lifecycle, heap validation, and fallback,
So that modes can be registered at compile time and switched safely at runtime.

**Acceptance Criteria:**

**Given** the DisplayMode interface exists from Story 1.1
**When** ModeRegistry is implemented
**Then** a MODE_TABLE[] static array in main.cpp holds pointers to all registered DisplayMode instances
**And** ModeRegistry::enumerate() returns mode count and names with zero heap allocation and completes under 10ms (NFR P4)
**And** ModeRegistry::activate(id) executes the full lifecycle: teardown current → validate heap → init new → begin rendering (FR6)
**And** if ESP.getFreeHeap() after teardown is below the requested mode's getMemoryRequirement() + 30KB safety margin, activation is rejected and the previous mode is restored (FR8, FR9, NFR S4)
**And** concurrent activate() calls are serialized with queue-of-one semantics — a switch initiated during an in-progress transition replaces any pending request (latest wins, previous pending discarded), not an unbounded queue (FR7)
**And** mode switch requests from WebPortal (Core 1) are communicated to the display task (Core 0) via an atomic flag pattern matching existing g_configChanged (AR2)
**And** adding a new mode requires exactly two touch points: one .h/.cpp pair in modes/ and one line in MODE_TABLE[] (AR5)

**Requirements:** FR5, FR6, FR7, FR8, FR9, FR10, AR2, AR5, NFR P4, NFR S1, NFR S2, NFR S4

### Story 1.4: Classic Card Mode Implementation

As a FlightWall user,
I want the existing three-line flight card display migrated into a ClassicCardMode that renders through the new mode system,
So that the familiar display continues working identically while enabling the mode architecture.

**Acceptance Criteria:**

**Given** the DisplayMode interface and DisplayUtils exist from previous stories
**When** ClassicCardMode is implemented in modes/ClassicCardMode.h/.cpp
**Then** render() draws the three-line flight card (airline name, route, aircraft type) using DisplayUtils functions
**And** the output is pixel-identical to the current Foundation Release rendering when compared across 5+ different flight cards (NFR C1)
**And** when two or more flights are available, the mode cycles through them at the configurable interval (FR12)
**And** all rendering stays within the zone bounds provided in RenderContext — no pixels are written outside the zone (FR4)
**And** the mode reports its memory requirement via getMemoryRequirement()
**And** getName() returns "Classic Card" and getZoneLayout() returns zone metadata describing the layout regions
**And** stack-local allocations during render() are under 512 bytes (NFR S3)
**And** brightness is read from RenderContext but never modified (NFR S5)

**Requirements:** FR4, FR11, FR12, FR13, NFR C1, NFR S3, NFR S5

### Story 1.5: Display Pipeline Integration

As a FlightWall user,
I want all flight display output to flow through the mode system with NeoMatrixDisplay owning only hardware and frame commit,
So that the device boots, activates Classic Card mode, and renders identically to the Foundation Release.

**Acceptance Criteria:**

**Given** ModeRegistry, ClassicCardMode, and DisplayUtils are complete from previous stories
**When** the display pipeline is refactored
**Then** NeoMatrixDisplay owns only: hardware init (FastLED.addLeds), frame commit (FastLED.show()), shared resources (logo buffer), and emergency fallback renderer (AR3)
**And** the display task in main.cpp calls ModeRegistry to get the active mode and invokes render() each frame
**And** only the shared display pipeline calls FastLED.show() — modes never commit the matrix directly (FR35)
**And** ClassicCardMode is registered in MODE_TABLE[] and activated as the default mode on boot
**And** if no mode can initialize, the emergency fallback renderer (displaySingleFlightCard) activates to ensure the display always works (FR36, AR8)
**And** per-frame display work completes within 16ms with no blocking delays (NFR P2, NFR C3)
**And** no new concurrent rendering tasks are created — cooperative scheduling is preserved (NFR C3)
**And** all Foundation Release features continue working: OTA, night mode, NTP, dashboard, health endpoint (NFR C2)
**And** after 100 consecutive mode switches (Classic Card → fallback → Classic Card), free heap returns to within 1KB of starting value (NFR S1)
**And** 10 rapid switch toggles in 3 seconds produce no undefined states, corruption, or watchdog resets (NFR S2)
**And** `pio run` builds cleanly and the device renders pixel-identically to Foundation Release

**Requirements:** FR34, FR35, FR36, AR3, AR8, NFR P1, NFR P2, NFR C2, NFR C3, NFR S1, NFR S2

---

## Epic 2: Live Flight Card Mode

Users see a richer flight display with real-time telemetry — altitude, ground speed, heading, and vertical rate — with the layout adapting gracefully when zone dimensions cannot fit all fields. A visual climb/descend/level indicator makes aircraft state immediately clear.

### Story 2.1: Live Flight Card Mode — Enriched Telemetry Rendering

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

### Story 2.2: Adaptive Field Dropping & Vertical Direction Indicator

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

## Epic 3: Mode Switching API & Dashboard Mode Picker

Users gain control over how their FlightWall displays flight information — choosing between display modes from the web dashboard with schematic previews of each mode's layout. Clear transition feedback, error handling, and NVS persistence ensure the user's choice is respected across power cycles. An upgrade notification guides first-time users to explore the new modes.

### Story 3.1: Display Mode API Endpoints

As an external client (web dashboard or third-party tool),
I want HTTP API endpoints to list available modes, retrieve the active mode, and request a mode change,
So that the mode system can be controlled programmatically from any client.

**Acceptance Criteria:**

**Given** the mode system from Epic 1 is operational
**When** GET /api/display/modes is called
**Then** the response is JSON: `{ ok: true, modes: [{ id, name, zone_layout: [...] }], active_mode: "<id>", upgrade_notification: <bool> }`
**And** each mode entry includes the zone layout metadata from getZoneLayout() for schematic rendering
**And** the response is generated under 10ms with zero heap allocation for enumeration (NFR P4)

**Given** a valid mode ID
**When** POST /api/display/mode is called with `{ mode: "<id>" }`
**Then** the request blocks until the mode switch completes (synchronous activation)
**And** on success, the response is: `{ ok: true, active_mode: "<id>", status: "active" }`
**And** on failure (e.g., insufficient heap), the response is: `{ ok: false, error: "<reason>", active_mode: "<previous_id>", status: "failed" }` (FR33)
**And** the response distinguishes between switch-complete and switch-failed states (FR32)

**Given** an invalid mode ID
**When** POST /api/display/mode is called
**Then** the response is: `{ ok: false, error: "Unknown mode" }` with HTTP 400

**Given** the upgrade notification flag is active
**When** POST /api/display/notification/dismiss is called
**Then** the firmware clears the upgrade_notification flag in NVS
**And** subsequent GET /api/display/modes responses return `upgrade_notification: false`

**Requirements:** FR30, FR31, FR32, FR33

### Story 3.2: NVS Mode Persistence & Boot Restore

As a FlightWall user,
I want my display mode selection saved across power cycles,
So that the device boots into my preferred mode without requiring manual reselection.

**Acceptance Criteria:**

**Given** a mode switch is successfully completed (via API or any trigger)
**When** the new mode is confirmed active
**Then** the mode ID is written to NVS key "display_mode" in the "flightwall" namespace
**And** the key does not collide with any of the existing 23+ Foundation Release NVS keys (NFR S6, AR7)

**Given** the device boots and NVS contains a valid "display_mode" value
**When** the mode system initializes
**Then** the stored mode is activated instead of the default
**And** if the stored mode fails to activate (e.g., insufficient heap), Classic Card is activated as fallback

**Given** the device boots after upgrading from Foundation Release (no "display_mode" key in NVS)
**When** the mode system initializes
**Then** Classic Card mode is activated as the default (FR29)
**And** the upgrade_notification flag in the API response is set to true

**Given** the device boots with an NVS value referencing a mode ID that no longer exists in firmware
**When** the mode system initializes
**Then** Classic Card mode is activated as fallback and the NVS key is updated to reflect Classic Card

**Requirements:** FR27, FR28, FR29, AR7, NFR S6

### Story 3.3: Mode Picker Section — Cards & Schematic Previews

As a FlightWall user,
I want to see all available display modes in the web dashboard with schematic previews of each mode's zone layout,
So that I can understand what each mode looks like before switching.

**Acceptance Criteria:**

**Given** the dashboard loads and the Mode Picker section exists
**When** the page initializes
**Then** JS fetches GET /api/display/modes and dynamically builds mode cards from the response (UX-DR12)
**And** the Mode Picker section uses a `<div id="mode-picker">` placeholder with ~10 lines of static HTML
**And** each mode card displays the mode name and a CSS Grid schematic preview rendered from the zone_layout metadata (UX-DR2)
**And** the schematic grid container is 80px height with zone divs positioned via grid-row/grid-column
**And** zone divs have labeled content regions and aria-hidden="true" (UX-DR8)
**And** the schematic container has an aria-label describing the layout (UX-DR8)

**Given** the API response indicates an active mode
**When** mode cards are rendered
**Then** the active mode card shows a 4px accent left border, status dot, and "Active" label (UX-DR1 active state)
**And** the active card has aria-current="true" (UX-DR7)
**And** non-active mode cards show a 1px accent-dim border and are styled as tappable (UX-DR1 idle state)
**And** the entire non-active card is the tap target — not a button within the card (UX-DR4)

**Given** the Mode Picker section is rendered
**When** the user views the dashboard navigation
**Then** the Mode Picker is accessible from the dashboard nav at all times (FR24)
**And** the UI matches existing dashboard styling, dark theme, and CSS custom properties (NFR C4)
**And** the Mode Picker loads within 1 second of page load (NFR P3)

**Requirements:** FR17, FR18, FR20, FR24, UX-DR1 (idle/active), UX-DR2, UX-DR4, UX-DR8 (partial), UX-DR12, NFR P3, NFR C4

### Story 3.4: Mode Switching Flow & Transition States

As a FlightWall user,
I want to tap a mode card to activate that mode with clear visual feedback during the switch,
So that I know the switch is happening, when it completes, and if anything goes wrong.

**Acceptance Criteria:**

**Given** a non-active mode card is tapped or activated via keyboard
**When** the mode switch is initiated
**Then** the tapped card enters the "switching" state: pulsing accent border, opacity 0.7, "Switching..." label (UX-DR1 switching state)
**And** all sibling non-active cards enter the "disabled" state: opacity 0.5, pointer-events none (UX-DR10)
**And** a POST /api/display/mode request is sent to the firmware

**Given** the firmware confirms the mode switch succeeded
**When** the POST response returns with `ok: true`
**Then** the previously-active card loses its active styling and becomes idle
**And** the switched card transitions from "switching" to "active" with 4px accent border, status dot, "Active" label (FR22)
**And** all disabled sibling cards return to idle state
**And** focus moves from the now-active card to the previously-active (now idle) card (UX-DR6)

**Given** the firmware returns a failure (e.g., insufficient heap)
**When** the POST response returns with `ok: false`
**Then** the failed card shows a scoped error message: "Not enough memory to activate this mode. Current mode restored." (UX-DR11)
**And** the error message auto-dismisses after 5 seconds or on the next user interaction (UX-DR11)
**And** the error element has role="alert" and aria-live="polite" (UX-DR8)
**And** the card returns to idle state and all disabled siblings return to idle

**Given** the user has prefers-reduced-motion enabled
**When** a mode switch is in progress
**Then** the pulsing border animation is replaced with a static accent-dim border at full opacity (UX-DR5)
**And** the switching state is communicated via text label only

**Requirements:** FR19, FR21, FR22, FR23, UX-DR1 (switching/disabled/error), UX-DR5, UX-DR6 (partial), UX-DR10, UX-DR11, NFR P1

### Story 3.5: Accessibility & Responsive Layout

As a FlightWall user with accessibility needs or using a phone,
I want the Mode Picker to be fully keyboard-navigable, screen-reader friendly, and responsive across devices,
So that I can control display modes regardless of how I access the dashboard.

**Acceptance Criteria:**

**Given** the Mode Picker is rendered with idle mode cards
**When** the user navigates via keyboard
**Then** idle cards have role="button" and tabindex="0" (UX-DR7)
**And** pressing Enter or Space on a focused idle card triggers mode activation (UX-DR7)
**And** the active card has aria-current="true" and is not in the tab order as an actionable element (UX-DR7)

**Given** a mode switch completes
**When** focus management executes
**Then** focus moves from the now-active card to the previously-active (now idle) card (UX-DR6)

**Given** the notification banner is dismissed or removed
**When** focus management executes
**Then** focus moves to the Mode Picker section heading (UX-DR6)

**Given** a card is in the "switching" state
**When** a screen reader queries the card
**Then** aria-busy="true" is set on the switching card (UX-DR8)

**Given** a card is in the "disabled" state
**When** a screen reader queries the card
**Then** aria-disabled="true" is set on the disabled card (UX-DR8)

**Given** the dashboard is viewed on a phone (viewport < 600px)
**When** the Mode Picker renders
**Then** mode cards display in a single column, phone-first layout (UX-DR9)

**Given** the dashboard is viewed on a desktop (viewport >= 1024px)
**When** the Mode Picker renders
**Then** mode cards display in a two-column grid layout (UX-DR9)
**And** the 1024px breakpoint is additive — the existing 600px breakpoint is inherited unchanged

**Requirements:** UX-DR6, UX-DR7, UX-DR8, UX-DR9

### Story 3.6: Upgrade Notification Banner

As a FlightWall user who just upgraded from Foundation Release,
I want to see a one-time notification about new display modes with actions to try them,
So that I discover the new feature without needing to find it myself.

**Acceptance Criteria:**

**Given** the device has been upgraded from Foundation Release (upgrade_notification flag is true in API response)
**And** localStorage does not contain "mode_notif_seen"
**When** the dashboard loads
**Then** an upgrade notification banner is displayed with the message "New display modes available"
**And** the banner has three actions: "Try Live Flight Card" button, "Browse Modes" link, and a dismiss X button

**Given** the user clicks "Try Live Flight Card"
**When** the action executes
**Then** the Live Flight Card mode is activated immediately (POST /api/display/mode)
**And** localStorage "mode_notif_seen" is set to "true"
**And** a POST to the firmware clears the upgrade_notification flag
**And** the banner is removed from the DOM

**Given** the user clicks "Browse Modes"
**When** the action executes
**Then** the page scrolls to the Mode Picker section
**And** localStorage "mode_notif_seen" is set to "true"
**And** a POST to the firmware clears the upgrade_notification flag
**And** the banner is removed from the DOM

**Given** the user clicks the dismiss X button
**When** the action executes
**Then** localStorage "mode_notif_seen" is set to "true"
**And** a POST to the firmware clears the upgrade_notification flag
**And** the banner is removed from the DOM
**And** focus moves to the Mode Picker section heading (UX-DR6)

**Given** the user revisits the dashboard after dismissing the notification
**When** the dashboard loads
**Then** the banner does not appear (either localStorage or API flag prevents it)

**Given** the device was not upgraded from Foundation Release (upgrade_notification is false)
**When** the dashboard loads
**Then** no upgrade notification banner is displayed

**Given** the "Try Live Flight Card" action is triggered but the Live Flight Card mode is not registered in firmware
**When** the activation request fails or the mode ID is not found
**Then** the banner is still dismissed (both sources cleared) and the user is shown a toast: "Mode not available"
**And** the Mode Picker remains visible for manual selection

**Requirements:** FR25, FR26, UX-DR3, UX-DR6 (partial)
