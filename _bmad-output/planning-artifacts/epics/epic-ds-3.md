## Epic ds-3: Mode Switching API & Dashboard Mode Picker

Users gain control over how their FlightWall displays flight information — choosing between display modes from the web dashboard with schematic previews of each mode's layout. Clear transition feedback, error handling, and NVS persistence ensure the user's choice is respected across power cycles. An upgrade notification guides first-time users to explore the new modes.

### Story ds-3.1: Display Mode API Endpoints

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

### Story ds-3.2: NVS Mode Persistence & Boot Restore

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

### Story ds-3.3: Mode Picker Section — Cards & Schematic Previews

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

### Story ds-3.4: Mode Switching Flow & Transition States

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

### Story ds-3.5: Accessibility & Responsive Layout

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

### Story ds-3.6: Upgrade Notification Banner

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

