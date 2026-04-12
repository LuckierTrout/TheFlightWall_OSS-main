## Epic fn-3: Onboarding Polish — "Fresh Start Done Right"

New user completes the setup wizard with a "Test Your Wall" step that verifies panel layout matches expectations before showing garbled flight data. Timezone auto-detects from the browser.

### Story fn-3.1: Wizard Step 6 — Test Your Wall

As a new user,
I want the setup wizard to test my LED panel layout and let me confirm it looks correct before seeing flight data,
So that I catch wiring problems immediately instead of debugging garbled output.

**Acceptance Criteria:**

**Given** the user completes Step 5 (Review) in the setup wizard
**When** Step 6 ("Test Your Wall") loads
**Then** the device auto-runs the panel position test pattern on the LED matrix (reusing the existing calibration position endpoint)
**And** the pattern renders on the LED matrix within 500ms of being triggered
**And** a matching canvas preview renders in the browser showing the expected numbered panel layout

**Given** Step 6 is displayed
**When** the canvas preview and LED pattern are both visible
**Then** the wizard asks: "Does your wall match this layout?"
**And** two buttons are shown: "Yes, it matches" (primary) and "No — take me back" (secondary)
**And** the primary button follows the one-primary-per-context rule

**Given** the user taps "No — take me back"
**When** the wizard navigates
**Then** the user returns to Step 4 (Hardware configuration) — not the immediately previous step
**And** all previously entered values are preserved
**And** the user can change origin corner, scan direction, or GPIO pin and return to Step 6 to re-test

**Given** the user taps "Yes, it matches"
**When** the confirmation is received
**Then** an RGB color test sequence runs on the LED matrix: solid red → solid green → solid blue (each held ~1 second)
**And** the wizard shows "Testing colors..." during the sequence

**Given** the RGB color test completes
**When** the sequence finishes
**Then** the wizard displays "Your FlightWall is ready! Fetching your first flights..."
**And** settings are saved via `POST /api/settings`
**And** the device transitions from AP mode to STA mode (WiFi connects to configured network)
**And** the wizard terminates (phone loses AP connection — expected behavior)

**Given** all Step 6 UI
**When** rendered
**Then** all buttons have 44x44px minimum touch targets
**And** all components meet WCAG 2.1 AA contrast requirements
**And** all components work at 320px minimum viewport width

**Given** a "Run Panel Test" secondary button in the Calibration card on the dashboard
**When** tapped post-setup
**Then** it triggers the same calibration position pattern as Step 6 for post-setup panel testing

**Given** updated wizard web assets (wizard.html, wizard.js)
**When** gzipped and placed in `firmware/data/`
**Then** the gzipped copies replace existing ones for LittleFS upload

**References:** FR25, FR26, FR27, FR28, FR29, UX-DR11, UX-DR12, UX-DR13, UX-DR15, NFR-P5

### Story fn-3.2: Wizard Timezone Auto-Detect

As a new user,
I want the setup wizard to automatically detect my timezone from my phone's browser,
So that I don't have to manually look up or configure timezone settings.

**Acceptance Criteria:**

**Given** the wizard loads Step 3 (Location)
**When** the page initializes
**Then** `Intl.DateTimeFormat().resolvedOptions().timeZone` is called to detect the browser's IANA timezone (e.g., `"America/New_York"`)
**And** the detected IANA timezone is looked up in the `TZ_MAP` (shared from Epic 2 / dashboard.js, duplicated in wizard.js)

**Given** the detected timezone is found in `TZ_MAP`
**When** the lookup succeeds
**Then** the timezone field is pre-filled with the IANA name displayed and the POSIX string stored for submission
**And** the user can change the selection via a dropdown if the auto-detected timezone is wrong

**Given** the detected timezone is NOT found in `TZ_MAP`
**When** the lookup fails
**Then** the timezone dropdown shows "Select timezone..." with no pre-selection
**And** a manual POSIX entry field is available as fallback
**And** no error is shown — the field simply isn't pre-filled

**Given** the user completes the wizard with a timezone selected
**When** settings are saved via `POST /api/settings`
**Then** the `timezone` key is set to the POSIX string (e.g., `"EST5EDT,M3.2.0,M11.1.0"`)
**And** the device calls `configTzTime()` with the saved POSIX string after WiFi connects

**Given** the wizard `TZ_MAP` in wizard.js
**When** compared to the dashboard.js `TZ_MAP`
**Then** both contain the same entries (same source data, duplicated for separate gzipped files)

**References:** FR17, AR6, UX-DR11

