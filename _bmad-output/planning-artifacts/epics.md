---
stepsCompleted: [1, 2, 3, 4]
status: 'complete'
completedAt: '2026-04-02'
inputDocuments: ['_bmad-output/planning-artifacts/prd.md', '_bmad-output/planning-artifacts/architecture.md', '_bmad-output/planning-artifacts/ux-design-specification.md']
---

# TheFlightWall_OSS-main - Epic Breakdown

## Overview

This document provides the complete epic and story breakdown for TheFlightWall, decomposing the requirements from the PRD, UX Design, and Architecture into implementable stories.

## Requirements Inventory

### Functional Requirements

FR1: User can complete initial device configuration entirely from a mobile browser without editing code or using a command line
FR2: Device can broadcast its own WiFi access point when no WiFi credentials are saved or when saved WiFi is unreachable
FR3: User can enter WiFi credentials through a captive portal setup wizard
FR4: User can enter API credentials (OpenSky client ID/secret, AeroAPI key) through the setup wizard
FR5: User can set geographic location (latitude, longitude, radius) through the setup wizard using text input with an optional browser geolocation prompt ("Use my location")
FR6: User can configure hardware tile layout (tile count X/Y, pixel dimensions, data pin) through the setup wizard
FR7: Device can transition from AP mode to STA mode and connect to the configured home WiFi network
FR8: Device can display connection status messages on the LED matrix during setup and transitions
FR9: User can view and edit all device settings from a web dashboard accessible on the local network
FR10: Device can persist all configuration to non-volatile storage that survives power cycles
FR11: Device can fall back to compile-time default values when no saved configuration exists
FR12: User can modify display settings (brightness, text color RGB) and see changes reflected on the LED matrix within 1 second
FR13: User can modify network/API settings and receive a status message ("Rebooting to apply changes...") indicating a reboot is required
FR14: Device can reboot and apply configuration changes automatically after user confirms
FR15: User can configure matrix wiring flags (origin corner, scan direction, zigzag/progressive) for both single tiles and multi-tile arrangements through the web UI
FR16: Device can render a test pattern on the LED matrix that updates in real-time as calibration settings are changed
FR17: User can view a live HTML5 canvas preview in the browser that accurately represents the matrix dimensions, zone layout, and calibration state
FR18: Preview canvas can update in real-time as tile count, dimensions, or wiring flags are changed
FR19: Device can fetch ADS-B state vectors from OpenSky Network within a configurable geographic radius
FR20: Device can enrich flight data with airline, aircraft, and route metadata from FlightAware AeroAPI
FR21: Device can resolve human-friendly airline and aircraft display names from the FlightWall CDN
FR22: Device can render a flight card showing airline name, route (origin > destination), and aircraft type
FR23: Device can cycle through multiple active flights at a configurable interval
FR24: Device can display live telemetry: altitude (kft), speed (mph), track (degrees), and vertical rate (ft/s)
FR25: Device can display a loading/waiting screen when no flights are currently in range
FR26: Device can render a 32x32 pixel airline logo bitmap in the left zone of the display
FR27: Device can look up airline logos by ICAO operator code from local storage
FR28: Device can display a generic airplane fallback icon when no matching airline logo is found
FR29: Logo zone size can scale proportionally based on matrix height
FR30: Device can calculate display zones (logo, flight card, telemetry) dynamically based on matrix dimensions
FR31: Device can select a layout mode (compact, full, expanded) based on matrix height breakpoints
FR32: Device can render the appropriate layout for the detected matrix dimensions without code changes
FR33: Display layout can adapt when tile configuration is changed via the web UI
FR34: User can upload airline logo bitmap files (.bin, 32x32 RGB565) to the device through the web UI
FR35: User can view a list of uploaded logos on the device
FR36: Device can validate uploaded logo files for correct size and format
FR37: User can delete uploaded logos from the device through the web UI
FR38: Device can run LED display updates without blocking or being blocked by WiFi and web server operations
FR39: Device can serve the web configuration UI while simultaneously running the flight data pipeline without either blocking the other
FR40: Device can automatically maintain valid API authentication for OpenSky Network before credentials expire
FR41: Device can detect WiFi disconnections and API failures, continue displaying last known flight data, and automatically retry connections at the configured fetch interval without requiring manual restart
FR42: User can reconfigure hardware tile layout, display pin, and wiring flags from the config dashboard after initial setup
FR43: User can adjust timing settings (fetch interval, display cycle duration) from the web UI
FR44: Device can display its local IP address on the LED matrix after connecting to WiFi
FR45: User can reset all settings to defaults from the web UI, returning the device to AP setup mode
FR46: User can force the device into AP setup mode by holding a designated GPIO button during boot
FR47: User can view and adjust the geographic capture area using an interactive map with a draggable circle radius overlay in the config dashboard
FR48: User can see estimated monthly API usage when adjusting the fetch interval, updated in real-time as the setting changes

### NonFunctional Requirements

NFR1: Display settings changes (brightness, color) must be reflected on the LED matrix within 1 second of submission
NFR2: Web config UI pages must load within 3 seconds on the local network
NFR3: Flight data fetch cycle must complete (API call + JSON parse + display update) within the configured fetch interval
NFR4: LED matrix frame rendering must maintain a consistent frame rate with no dropped frames during flight card transitions
NFR5: Captive portal setup wizard must be responsive — each page transition under 2 seconds
NFR6: Device must operate continuously without requiring manual restart (target: 30+ days of uninterrupted operation)
NFR7: Device must recover automatically from transient WiFi disconnections within 60 seconds of network availability without user intervention
NFR8: Device must recover from API failures (rate limits, timeouts, auth errors) by continuing to display last known data and retrying at the next fetch interval rather than showing error screens
NFR9: Configuration writes must be atomic — power loss during a config save must not corrupt stored settings
NFR10: Device must return to AP setup mode if stored WiFi network becomes permanently unreachable (after configurable retry timeout)
NFR11: OpenSky Network API: Authenticated access with automatic credential refresh, rate-limited to stay within 4,000 monthly requests on free tier
NFR12: FlightAware AeroAPI: API key authentication, handle HTTP 429 and 5xx by retrying at next fetch interval
NFR13: FlightWall CDN: Public HTTPS GET, fail silently if unavailable (fall back to ICAO codes)
NFR14: All external API calls use HTTPS (TLS)
NFR15: Total firmware + web assets + logos must fit within 4MB flash with at least 500KB headroom
NFR16: Runtime RAM usage must stay below 280KB to maintain stable heap for concurrent web server + API operations
NFR17: Device filesystem operations must stream data — no full-file RAM buffering for uploads or logo reads
NFR18: Web server limited to 2-3 concurrent client connections to prevent heap exhaustion

### Additional Requirements

From Architecture:
- AR1: Use mathieucarbou/ESPAsyncWebServer fork (not me-no-dev original) — fixes memory leaks
- AR2: Use mathieucarbou/AsyncTCP fork — fixes race conditions under load
- AR3: Custom partition table required: 2MB app + 1.9MB LittleFS (custom_partitions.csv)
- AR4: board_build.filesystem = littlefs in platformio.ini
- AR5: ConfigManager singleton with category struct getters (DisplayConfig, LocationConfig, HardwareConfig, TimingConfig, NetworkConfig)
- AR6: NVS write debouncing — RAM cache updates instantly, NVS persists after 2-second quiet period
- AR7: Hybrid inter-task communication: atomic flag for config changes, FreeRTOS queue for flight data
- AR8: WiFiManager state machine: AP_SETUP → CONNECTING → STA_CONNECTED → STA_RECONNECTING → AP_FALLBACK
- AR9: SystemStatus singleton for centralized subsystem health reporting
- AR10: 11 REST API endpoints with consistent JSON envelope { ok, data, error, code }
- AR11: Shared zone calculation algorithm — identical implementation in C++ (LayoutEngine) and JavaScript (dashboard.js)
- AR12: Compile-time log level macros (LOG_E/LOG_I/LOG_V) in utils/Log.h
- AR13: Config header migration: existing config/ headers become compile-time defaults, only ConfigManager includes them
- AR14: Runtime LittleFS layout: web assets at root, logos in /logos/ subdirectory
- AR15: Unit tests required for ConfigManager and LayoutEngine (Unity framework)
- AR16: FreeRTOS task pinning: display on Core 0, WiFi/web/API on Core 1
- AR17: mDNS registration (flightwall.local) on WiFi STA connect
- AR18: NTP time sync for API call counter monthly reset
- AR19: GPIO short-press shows IP on LEDs, long-press during boot forces AP mode
- AR20: Fallback airplane sprite as PROGMEM constant in LogoManager.h
- AR21: ESP32 task watchdog — no task blocks longer than 5 seconds

### UX Design Requirements

UX-DR1: Dark theme design system with CSS custom properties — 10 color tokens (bg-primary, bg-surface, bg-input, text-primary, text-secondary, accent, accent-dim, success, warning, error)
UX-DR2: System font stack (no custom web fonts) with 3-size type scale (lg/md/sm) and 4px-based spacing scale
UX-DR3: Single-column layout (max-width 480px centered) — no breakpoints, phone-first
UX-DR4: Setup wizard: 5-step linear flow with progress bar (WiFi → API Keys → Location → Hardware → Review)
UX-DR5: WiFi scan in wizard with async AP_STA scan, 5-second timeout, manual entry fallback always available
UX-DR6: Browser geolocation prompt ("Use my location") as primary location input in wizard, manual lat/lon as fallback
UX-DR7: Config dashboard: section-based card layout (Display, Timing, Location, Hardware, Logos, System)
UX-DR8: Hot-reload sliders for display settings — value updates on drag (oninput), POST fires on release (onchange)
UX-DR9: Fetch interval slider with real-time API usage estimate ("~2,160 calls/month")
UX-DR10: Display cycle slider with hot-reload to LED wall
UX-DR11: Interactive Leaflet map with draggable circle radius — lazy-loaded on Location section open
UX-DR12: Canvas layout preview — client-side zone rendering using shared algorithm, updates instantly on tile config changes
UX-DR13: Toast notification system — slide-in, 2-3s auto-dismiss, severity-colored (green/amber/red), respects prefers-reduced-motion
UX-DR14: Logo upload with client-side RGB565 preview canvas (32x32 decoded to 128x128), batch upload, file size validation
UX-DR15: Logo list with canvas preview thumbnails, file size, delete button with inline confirmation
UX-DR16: System Health page: WiFi status, API status with colored dots, API quota used/limit, device metrics (heap, uptime, LittleFS)
UX-DR17: Manual "Refresh Status" button on health page (no auto-refresh — protects ESP32 connection limit)
UX-DR18: Gzipped web assets served from LittleFS with common.js shared module (applySettings, fetchStatus, rebootDevice, showToast, decodeRGB565)
UX-DR19: Post-wizard LED progress states: "Connecting to WiFi..." → "WiFi Connected ✓" → "IP: x.x.x.x" → "Authenticating APIs..." → "Fetching flights..." → first flight card
UX-DR20: Inline confirmation pattern for destructive actions (factory reset, logo delete) — no modal dialogs
UX-DR21: Error specificity in all error states — named cause + recovery path, never generic "Something went wrong"
UX-DR22: WCAG AA contrast ratios on all text, 44x44px minimum touch targets, semantic HTML, visible focus rings
UX-DR23: Captive portal testing on iOS Safari and Android Chrome as primary validation targets
UX-DR24: Triple feedback pattern: form control → canvas preview (instant, client-side) → LED wall (50-200ms network latency)

### FR Coverage Map

| FR | Epic | Description |
|----|------|-------------|
| FR1 | Epic 1 | Initial config from mobile browser |
| FR2 | Epic 1 | Broadcast WiFi AP |
| FR3 | Epic 1 | WiFi credentials via wizard |
| FR4 | Epic 1 | API credentials via wizard |
| FR5 | Epic 1 | Location via wizard (text + geolocation) |
| FR6 | Epic 1 | Hardware tile config via wizard |
| FR7 | Epic 1 | AP → STA WiFi transition |
| FR8 | Epic 1 | LED connection status messages |
| FR9 | Epic 2 | Dashboard settings view/edit |
| FR10 | Epic 1 | NVS config persistence |
| FR11 | Epic 1 | Compile-time default fallback |
| FR12 | Epic 2 | Hot-reload display settings |
| FR13 | Epic 2 | Reboot feedback for network changes |
| FR14 | Epic 2 | Auto-reboot after confirm |
| FR15 | Epic 4 | Calibration wiring flags |
| FR16 | Epic 4 | Test pattern on LEDs |
| FR17 | Epic 3 | Canvas layout preview |
| FR18 | Epic 3 | Real-time canvas updates |
| FR19 | Epic 1* | Fetch ADS-B state vectors (*ConfigManager migration — existing code) |
| FR20 | Epic 1* | Enrich with AeroAPI (*ConfigManager migration — existing code) |
| FR21 | Epic 1* | Resolve display names (*unchanged — existing code) |
| FR22 | Epic 3 | Render flight card (updated for zone layout) |
| FR23 | Epic 1* | Cycle through flights (*ConfigManager migration — existing code) |
| FR24 | Epic 3 | Display telemetry |
| FR25 | Epic 1* | Loading/waiting screen (*unchanged — existing code) |
| FR26 | Epic 3 | Render airline logo bitmap |
| FR27 | Epic 3 | ICAO logo lookup from LittleFS |
| FR28 | Epic 3 | Fallback airplane icon |
| FR29 | Epic 3 | Logo zone scaling |
| FR30 | Epic 3 | Dynamic zone calculation |
| FR31 | Epic 3 | Layout mode selection (breakpoints) |
| FR32 | Epic 3 | Render layout without code changes |
| FR33 | Epic 3 | Layout adapts on tile config change |
| FR34 | Epic 4 | Upload logo files via web UI |
| FR35 | Epic 4 | View uploaded logos list |
| FR36 | Epic 4 | Validate logo files |
| FR37 | Epic 4 | Delete logos via web UI |
| FR38 | Epic 1 | Display updates don't block WiFi |
| FR39 | Epic 1 | Web server + flight pipeline concurrent |
| FR40 | Epic 1 | Auto API auth refresh |
| FR41 | Epic 1 | WiFi/API failure recovery |
| FR42 | Epic 2 | Reconfigure hardware from dashboard |
| FR43 | Epic 2 | Adjust timing from dashboard |
| FR44 | Epic 1 | Display IP on LEDs |
| FR45 | Epic 2 | Factory reset from web UI |
| FR46 | Epic 2 | GPIO button forced AP mode |
| FR47 | Epic 4 | Interactive map with circle radius |
| FR48 | Epic 4 | API usage estimate on fetch interval |

*Epic 1 entries marked with * are existing code updated via ConfigManager migration — not separate stories, covered by ConfigManager story acceptance criteria.

**Coverage: 48/48 FRs mapped. 0 gaps.**

## Epic List

### Epic 1: First-Boot Setup — "Plug in, configure from phone, see flights" (DONE)

User can power on the device, connect to its WiFi AP from a phone, complete a 5-step setup wizard (WiFi → API keys → location → hardware → review), and see live flights appear on the LED wall — no code editing, no laptop, no terminal. The device persists all settings in NVS, runs display and WiFi on separate cores, recovers from WiFi disconnections, and displays its IP address on the LEDs.

### Epic 2: Config Dashboard & Device Management — "Tweak settings from browser, manage the device" (DONE)

User can browse to flightwall.local, view and edit all device settings from a card-based dashboard, see display changes reflected on the LED wall within 1 second via hot-reload, receive clear reboot feedback for network/API changes, adjust timing controls, reset the device to defaults, and force AP mode via GPIO button.

### Epic 3: Enhanced Flight Display & Canvas Preview — "Logos, telemetry, responsive layout, and live preview" (DONE)

LED wall shows airline logo in left zone, flight card with route info, and live telemetry (altitude, speed, track, vertical rate) in a zone-based layout that adapts automatically to any panel configuration. Browser shows a live canvas preview of the matrix layout that updates instantly as tile settings change, using the same zone calculation algorithm as the firmware.

### Epic 4: Advanced Dashboard Tools — "Map, calibration, and logo management" (DONE)

Visual configuration tools: interactive map, display calibration with test pattern, logo upload with preview, logo list management.

### Epic 5: OTA Firmware Updates & Settings Migration — "Cut the Cord"

User can back up their settings, flash the dual-OTA partition firmware one final time via USB, restore settings in the wizard, then upload all future firmware from the dashboard with progress feedback, validation, and automatic rollback on failure.

### Epic 6: Night Mode & Brightness Scheduling — "The Wall That Sleeps"

User configures a timezone and brightness schedule on the dashboard. The wall automatically dims at night and restores in the morning — correct across midnight boundaries and DST transitions. NTP sync status is visible, and the schedule degrades gracefully if NTP is unavailable.

### Epic 7: Onboarding Polish — "Fresh Start Done Right"

New user completes the setup wizard with a "Test Your Wall" step that verifies panel layout matches expectations before showing garbled flight data. Timezone auto-detects from the browser.

### Epic 8: Display Mode Architecture & Classic Card Migration

The device renders flight data through a pluggable display mode system, with the existing Classic Card output migrated into the first mode — pixel-identical to the current Foundation Release output. The emergency fallback renderer ensures the display always works even if a mode fails to initialize.

### Epic 9: Live Flight Card Mode

Users see a richer flight display with real-time telemetry — altitude, ground speed, heading, and vertical rate — with the layout adapting gracefully when zone dimensions cannot fit all fields. A visual climb/descend/level indicator makes aircraft state immediately clear.

### Epic 10: Mode Switching API & Dashboard Mode Picker

Users gain control over how their FlightWall displays flight information — choosing between display modes from the web dashboard with schematic previews of each mode's layout. Clear transition feedback, error handling, and NVS persistence ensure the user's choice is respected across power cycles. An upgrade notification guides first-time users to explore the new modes.

### Epic 11: Clock Mode & Orchestration Foundation — "The Wall Never Sleeps"

The owner sees a functioning display 100% of the time. When no flights are in range, Clock Mode activates automatically. The owner can manually switch modes from the dashboard. Watchdog recovery reboots to Clock Mode as a known-good default. New config keys introduced by the Delight Release resolve to safe defaults on first read.

### Epic 12: Departures Board — "Multi-Flight Awareness"

The owner sees all tracked flights at a glance in an airport-style scrolling list. Rows appear and disappear dynamically as aircraft enter and leave the tracking radius. No cycling through one-at-a-time cards.

### Epic 13: Animated Transitions — "Polished Handoffs"

Mode switches feel intentional, not broken. Smooth fade animations replace hard cuts. A non-technical observer perceives the wall as a finished product, not a prototype.

### Epic 14: Mode Scheduling — "Ambient Intelligence"

The wall runs itself. The owner configures time-based rules once and forgets. Schedule and night mode brightness operate independently. The schedule takes priority over data-driven idle fallback when explicitly configured.

### Epic 15: Mode-Specific Settings — "Tailored Experience"

Each display mode is configurable through the Mode Picker. Clock Mode offers 12h/24h format, Departures Board configures row count and telemetry fields. Settings apply without reboot and persist across power cycles.

### Epic 16: OTA Version Check & Notification — "Update Awareness"

The dashboard shows when a new firmware version is available. The owner can view release notes before deciding to update. GitHub API integration validated before any flash writes.

### Epic 17: OTA Download & Safety — "Painless Updates"

One tap to install a firmware update over WiFi. A/B partition safety means zero bricked devices. Failed downloads leave firmware unchanged. SHA-256 verification ensures integrity. All settings preserved across updates. Self-service onboarding documentation validated.

---

## Epic 1: First-Boot Setup — "Plug in, configure from phone, see flights"

User can power on the device, connect to its WiFi AP from a phone, complete a 5-step setup wizard, and see live flights appear on the LED wall — no code editing, no laptop, no terminal.

### Story 1.1: Project Infrastructure

As a developer,
I want the firmware to use a custom partition table, LittleFS filesystem, and compile-time log macros,
So that the project foundation supports all subsequent features.

**Acceptance Criteria:**

**Given** the updated platformio.ini with mathieucarbou/ESPAsyncWebServer, board_build.filesystem=littlefs, and board_build.partitions=custom_partitions.csv
**When** the firmware builds via `pio run`
**Then** the build succeeds with no errors
**And** the custom partition table allocates ~2MB app + ~1.9MB LittleFS

**Given** custom_partitions.csv is created at firmware/custom_partitions.csv
**When** the device boots with the new partition layout
**Then** LittleFS mounts successfully
**And** `pio run -t uploadfs` uploads the firmware/data/ contents to LittleFS

**Given** utils/Log.h is created with LOG_E, LOG_I, LOG_V macros
**When** any source file includes Log.h and uses LOG_I("Tag", "message")
**Then** output appears on Serial at the configured LOG_LEVEL
**And** macros compile to nothing when LOG_LEVEL is below their threshold

**Given** all infrastructure changes
**When** the existing flight pipeline runs
**Then** flights still display on the LED matrix (no regression from partition/library changes)

**References:** AR1, AR2, AR3, AR4, AR12

### Story 1.2: ConfigManager & SystemStatus

As a developer,
I want a ConfigManager singleton that persists settings in NVS with compile-time fallbacks, and a SystemStatus singleton for health tracking,
So that all components have reliable configuration and error reporting.

**Acceptance Criteria:**

**Given** ConfigManager::init() runs on first boot (empty NVS)
**When** getDisplay() is called
**Then** it returns DisplayConfig with compile-time defaults from config/UserConfiguration.h (brightness=5, text_color_r/g/b=255)

**Given** ConfigManager has values in NVS
**When** getDisplay() is called
**Then** NVS values override compile-time defaults in the returned struct

**Given** applyJson() is called with {"brightness": 25, "text_color_r": 200}
**When** processing the JSON
**Then** RAM cache updates immediately
**And** onChange callbacks fire
**And** schedulePersist(2000) queues NVS write after 2-second quiet period
**And** the returned ApplyResult lists applied keys and reboot_required=false

**Given** applyJson() is called with {"wifi_ssid": "NewNetwork"}
**When** processing a reboot-required key
**Then** NVS write happens immediately (no debounce)
**And** ApplyResult returns reboot_required=true

**Given** SystemStatus::init() runs
**When** SystemStatus::set(WIFI, OK, "Connected") is called
**Then** get(WIFI) returns {level: OK, message: "Connected", timestamp: now}
**And** toJson() includes the WIFI status in the health snapshot

**Given** existing adapters (OpenSkyFetcher, AeroAPIFetcher, FlightDataFetcher)
**When** migrated to use ConfigManager instead of config/ headers
**Then** the fetch → enrich → display pipeline works identically to before
**And** config/ headers are only included by ConfigManager.cpp

**Given** ConfigManager source files
**When** unit tests run via `pio test`
**Then** tests pass for: NVS read/write, default fallback chain, applyJson parsing, reboot key detection, debounce timing

**References:** FR10, FR11, AR5, AR6, AR9, AR13, AR15

### Story 1.3: FreeRTOS Dual-Core Task Architecture

As a user,
I want the LED display to update smoothly without WiFi or web server interruptions,
So that the flight display never flickers or freezes during network operations.

**Acceptance Criteria:**

**Given** the firmware boots with FreeRTOS task configuration
**When** the display task starts on Core 0 and the main loop runs on Core 1
**Then** the LED matrix renders flight cards without visible flicker
**And** WiFi operations on Core 1 do not interrupt display output

**Given** the flight pipeline completes a fetch cycle on Core 1
**When** enriched flight data is written via xQueueOverwrite
**Then** the display task receives the latest data via xQueuePeek and renders new flights

**Given** configChanged atomic flag is set on Core 1
**When** the display task detects the flag on Core 0
**Then** it re-reads config via getDisplay()/getHardware() and applies within 1 second

**Given** both cores running for 30+ minutes
**When** under normal load
**Then** no crashes, watchdog resets, or heap exhaustion
**And** ESP.getFreeHeap() remains above 100KB

**References:** FR38, FR39, AR7, AR16, AR21, NFR1, NFR4, NFR6

### Story 1.4: WiFi Manager & Network Connectivity

As a user,
I want the device to manage WiFi automatically — AP for setup, STA for operation, and recovery from disconnections,
So that I never need to reflash to fix network issues.

**Acceptance Criteria:**

**Given** the device boots with no WiFi credentials in NVS
**When** WiFiManager initializes
**Then** state enters AP_SETUP, broadcasting "FlightWall-Setup"

**Given** the device boots with saved WiFi credentials
**When** WiFiManager connects successfully
**Then** state transitions CONNECTING → STA_CONNECTED
**And** mDNS registers "flightwall"
**And** NTP time sync runs
**And** LED matrix displays IP for 3 seconds

**Given** WiFi drops while in STA_CONNECTED
**When** reconnection attempts begin
**Then** state enters STA_RECONNECTING, recovers within 60 seconds if network available

**Given** retry timeout expires without reconnection
**When** WiFiManager gives up
**Then** state enters AP_FALLBACK, broadcasting "FlightWall-Setup"

**Given** state changes occur
**When** registered callbacks fire
**Then** WebPortal, display task, and FlightDataFetcher receive notifications

**References:** FR2, FR7, FR40, FR41, FR44, AR8, AR17, AR18, NFR7, NFR10

### Story 1.5: Web Portal & Settings API

As a user,
I want the device to serve web pages and API endpoints,
So that I can configure it from any browser.

**Acceptance Criteria:**

**Given** the device is in AP or STA mode
**When** a browser requests /
**Then** wizard.html.gz (AP) or dashboard.html.gz (STA) is served with gzip encoding

**Given** POST /api/settings with valid JSON
**When** known config keys are included
**Then** ConfigManager::applyJson() applies changes
**And** response: {"ok": true, "applied": [...], "reboot_required": false/true}

**Given** GET /api/settings
**Then** all current config values returned as JSON

**Given** GET /api/status
**Then** SystemStatus health snapshot returned

**Given** POST /api/reboot
**Then** NVS flushed, device reboots within 3 seconds

**Given** GET /api/wifi/scan
**Then** async scan triggers, results returned as [{ssid, rssi}]

**Given** web server and flight pipeline running simultaneously
**Then** neither blocks the other

**References:** FR39, AR10, UX-DR18

### Story 1.6: Setup Wizard — WiFi & API Keys (Steps 1-2)

As a user,
I want to enter WiFi credentials and API keys through a mobile-friendly wizard,
So that I can configure the device from my phone.

**Acceptance Criteria:**

**Given** user connects to FlightWall-Setup AP
**When** captive portal opens
**Then** wizard.html loads with Step 1, progress bar "Step 1 of 5"

**Given** wizard Step 1 loads
**When** async WiFi scan triggers
**Then** if networks found in 5s, tappable SSID list appears
**And** if empty/timeout, "No networks found — enter manually" with text fields
**And** "Enter manually instead" link always available

**Given** user selects network or enters SSID + password
**When** "Next →" tapped with non-empty fields
**Then** wizard advances to Step 2 (API Keys)

**Given** Step 2 shows three paste-friendly fields (autocorrect off, autocapitalize off)
**When** all filled and "Next →" tapped
**Then** wizard advances to Step 3

**Given** any step, "← Back" preserves all entered values

**References:** FR1, FR3, FR4, UX-DR4, UX-DR5, UX-DR22, UX-DR23

### Story 1.7: Setup Wizard — Location, Hardware & Review (Steps 3-5)

As a user,
I want to set location, confirm hardware, and review before connecting,
So that the device is configured correctly on the first attempt.

**Acceptance Criteria:**

**Given** Step 3 (Location)
**When** "Use My Location" tapped
**Then** geolocation permission requested; on grant, lat/lon auto-fill; on deny, manual fields appear

**Given** Step 4 (Hardware)
**When** form loads
**Then** defaults pre-filled (10x2, 16px, pin 25), resolution shown

**Given** Step 5 (Review)
**When** summary displayed
**Then** each section tappable to edit, "Save & Connect" button prominent

**Given** "Save & Connect" tapped
**When** settings POST to /api/settings
**Then** NVS saves, phone shows "Look at LED wall for progress", WiFiManager transitions AP → STA

**References:** FR1, FR5, FR6, UX-DR4, UX-DR6

### Story 1.8: LED Progress States & First Flight

As a user,
I want the LED wall to show clear progress during setup,
So that I know the device is working and can watch it come to life.

**Acceptance Criteria:**

**Given** "Save & Connect" triggered
**When** device processes configuration
**Then** LED displays sequence: "Saving config..." → "Connecting to WiFi..." → "WiFi Connected ✓" → "IP: [address]" → "Authenticating APIs..." → "Fetching flights..." → first flight card

**Given** WiFi connection fails
**When** timeout expires
**Then** LED shows "WiFi Failed — restarting setup", device returns to AP

**Given** complete flow measured
**When** WiFi connects within 10 seconds
**Then** total time from "Save & Connect" to first flight card is under 60 seconds

**References:** FR8, FR44, UX-DR19, NFR5

---

## Epic 2: Config Dashboard & Device Management — "Tweak settings from browser, manage the device"

User can browse to flightwall.local, view and edit all settings, see display changes within 1 second, receive clear reboot feedback, and manage the device.

### Story 2.1: Dashboard Layout, Display Settings & Toast System

As a user,
I want a config dashboard with a display settings card and instant LED feedback,
So that I can customize brightness and color from my phone.

**Acceptance Criteria:**

**Given** device in STA_CONNECTED state
**When** navigating to flightwall.local
**Then** dashboard loads within 3 seconds with dark theme, card-based layout, single-column 480px
**And** dashboard header shows "FlightWall" title, device IP address, and a link to System Health page

**Given** Display card visible
**When** brightness slider dragged and released
**Then** POST /api/settings fires, LED wall updates within 1 second, green toast "Applied"

**Given** RGB color inputs changed
**When** values post
**Then** LED text color updates within 1 second, toast confirms

**Given** dashboard.js loads
**When** page initializes
**Then** GET /api/settings populates all fields

**Given** the toast notification system (showToast in common.js)
**When** implemented in this story
**Then** it is available as a shared dependency for all subsequent dashboard stories
**And** toasts slide in, auto-dismiss after 2-3s, severity-colored (green/amber/red)
**And** respects prefers-reduced-motion

**References:** FR9, FR12, UX-DR1-3, UX-DR7, UX-DR8, UX-DR13, UX-DR22, NFR1, NFR2

### Story 2.2: Timing Controls & API Usage Estimate

As a user,
I want to adjust fetch frequency and display cycle with real-time API budget feedback,
So that I can balance freshness against my monthly API quota.

**Acceptance Criteria:**

**Given** Timing card visible
**When** fetch interval slider adjusted
**Then** label shows "Check for flights every: X min", estimate shows "~N calls/month"
**And** estimate turns amber if exceeds 4,000
**And** on release, setting posts, toast confirms

**Given** display cycle slider adjusted
**When** released
**Then** LED wall cycling speed changes immediately (hot-reload), toast confirms

**Given** dashboard loads
**When** GET /api/settings returns timing values
**Then** both sliders pre-populated

**References:** FR43, FR48, UX-DR9, UX-DR10

### Story 2.3: Network & API Settings with Reboot Flow

As a user,
I want to update WiFi or API credentials and have the device reboot cleanly,
So that I can change credentials without reflashing.

**Acceptance Criteria:**

**Given** Network card visible
**When** WiFi/API fields edited and "Apply" tapped
**Then** response includes reboot_required: true, amber toast "Rebooting to apply changes..."
**And** device saves to NVS immediately, reboots within 3 seconds

**Given** device reboots with correct new credentials
**When** WiFi reconnects
**Then** LED shows progress, dashboard accessible again

**Given** new credentials are wrong
**When** WiFi fails
**Then** device falls back to AP mode for reconfiguration

**Given** Hardware card changes (tiles, pin)
**When** posted
**Then** reboot_required: true returned

**References:** FR13, FR14, FR42, UX-DR20, UX-DR21

### Story 2.4: System Health Page

As a user,
I want detailed system status showing WiFi, APIs, heap, uptime, and storage,
So that I can diagnose issues.

**Acceptance Criteria:**

**Given** System Health page linked from dashboard header
**When** page loads via GET /api/status
**Then** cards show: WiFi (network, signal, IP), APIs (OpenSky/AeroAPI/CDN with colored dots), API Quota (used/limit, rate estimate, amber if over-pace), Device (uptime, heap, LittleFS), Flight Data (last fetch, flights in range, enriched, logos matched)

**Given** "Refresh" button tapped
**When** GET /api/status fires
**Then** all cards update. Page does NOT auto-refresh.

**References:** FR9, UX-DR16, UX-DR17, UX-DR21

### Story 2.5: Factory Reset & GPIO Button

As a user,
I want to reset the device from the web UI or force setup mode via hardware button,
So that I can recover from any configuration problem.

**Acceptance Criteria:**

**Given** System section at dashboard bottom
**When** "Reset to Defaults" (danger button) tapped
**Then** inline confirmation appears: "All settings will be erased..." with Confirm/Cancel (no modal)

**Given** Confirm tapped
**When** POST /api/reset fires
**Then** NVS erased, device reboots to AP_SETUP

**Given** device powering on
**When** GPIO button (default GPIO 0) held during boot
**Then** WiFiManager enters AP_SETUP regardless of saved config, LED shows "Setup Mode — Forced"

**Given** device running normally
**When** GPIO short-pressed
**Then** LED shows IP for 5 seconds, returns to flight display

**References:** FR45, FR46, AR19, UX-DR20

---

## Epic 3: Enhanced Flight Display & Canvas Preview — "Logos, telemetry, responsive layout, and live preview"

LED wall shows airline logo, flight card, and live telemetry in zone-based layout. Browser shows live canvas preview using shared algorithm.

### Story 3.1: Layout Engine & Shared Zone Algorithm (C++ and JavaScript)

As a user,
I want the display to calculate zones automatically for any panel configuration, with an identical algorithm in both firmware and browser,
So that the layout adapts without code changes and the canvas preview always matches the LEDs.

**Acceptance Criteria:**

**Given** LayoutEngine.h/.cpp created in core/
**When** LayoutEngine::init() receives HardwareConfig (tiles_x=10, tiles_y=2, tile_pixels=16)
**Then** matrixWidth=160, matrixHeight=32, mode="full"
**And** logoZone={x:0, y:0, w:32, h:32}, flightZone={x:32, y:0, w:128, h:16}, telemetryZone={x:32, y:16, w:128, h:16}

**Given** all 4 test vectors from the architecture spec
**When** LayoutEngine processes each
**Then** all outputs match exactly:
- 10x2@16 → 160x32, full
- 5x2@16 → 80x32, full
- 12x3@16 → 192x48, expanded
- 10x1@16 → 160x16, compact

**Given** configChanged flag fires with new tile dimensions
**When** LayoutEngine recalculates
**Then** new zones are available for the display task on the next frame

**Given** the same zone algorithm implemented in dashboard.js
**When** the JavaScript function processes all 4 test vectors
**Then** outputs match the C++ LayoutEngine exactly (verified in browser console)

**Given** GET /api/layout endpoint
**When** requested
**Then** returns current zone data as JSON for initial canvas load

**Given** LayoutEngine unit tests in test/test_layout_engine/
**When** run via `pio test`
**Then** all 4 test vectors pass

**References:** FR30, FR31, FR32, FR33, AR11, AR15

### Story 3.2: Logo Manager & Fallback Sprite

As a user,
I want the device to load airline logos from storage with a fallback icon,
So that the display shows real branding when available.

**Acceptance Criteria:**

**Given** LogoManager::init()
**When** LittleFS mounted
**Then** /logos/ directory created if missing, logo count tracked

**Given** flight with operator_icao "UAL"
**When** loadLogo("UAL", buffer) called
**Then** /logos/UAL.bin streamed into buffer (2048 bytes), returns true

**Given** no matching logo for "XYZ"
**When** loadLogo("XYZ", buffer) called
**Then** PROGMEM fallback sprite copied to buffer, returns false

**Given** LogoManager
**When** getLogoCount() and getLittleFSUsage() called
**Then** correct values returned for System Health reporting

**References:** FR27, FR28, AR14, AR20

### Story 3.3: Zone-Based Flight Rendering with Logos & Telemetry

As a user,
I want the LED wall to show a flight card with logo, route info, and live telemetry in a zone-based layout,
So that the display is information-rich and visually impressive.

**Acceptance Criteria:**

**Given** LayoutEngine zones calculated
**When** NeoMatrixDisplay renders a flight
**Then** logo zone shows airline logo (or fallback) scaled to zone dimensions
**And** flight card zone shows airline name (line 1), route origin > destination (line 2), aircraft type (line 3)
**And** telemetry zone shows altitude (kft), speed (mph), track (deg), vertical rate (ft/s)
**And** text truncates to fit zone width, no overflow between zones

**Given** FlightInfo model
**When** enrichment pipeline processes a flight
**Then** display-ready telemetry fields are computed: altitude_kft, speed_mph, track_deg, vertical_rate_fps (converted from API units)
**And** display renderer reads these fields directly (no math in renderer)

**Given** telemetry values are null from the API
**When** rendering
**Then** missing values show "--" instead of zero or blank

**Given** multiple flights in range
**When** display cycle elapses
**Then** next flight card renders with clean transition, no flicker

**Given** compact mode (height < 32px)
**When** rendering
**Then** content condensed to fit smaller zones

**Given** expanded mode (height >= 48px)
**When** rendering
**Then** larger zones accommodate all content comfortably

**Given** no flights in range
**When** display has no data
**Then** loading/waiting screen shows

**References:** FR22, FR24, FR25, FR26, FR27, FR28, FR29, FR32

### Story 3.4: Canvas Layout Preview in Dashboard

As a user,
I want a live canvas preview of my LED layout that updates instantly when I change tile settings,
So that I can visualize the zone arrangement before committing changes.

**Acceptance Criteria:**

**Given** dashboard Hardware section visible
**When** GET /api/layout returns current zone data
**Then** canvas initializes showing matrix grid with colored zones (logo, flight card, telemetry) and legend

**Given** tiles_x, tiles_y, or tile_pixels changed in form
**When** values update
**Then** canvas recalculates zones client-side (shared algorithm in JS), updates instantly (no server round-trip)
**And** resolution text updates ("Your display: 192 × 48 pixels")

**Given** JS zone algorithm
**When** any config processed
**Then** output matches C++ LayoutEngine identically

**Given** canvas rendered
**When** dimensions change
**Then** canvas scales proportionally within 480px container

**Given** triple feedback pattern
**When** tile setting changes
**Then** form updates (instant) → canvas updates (instant) → LED wall updates after POST (50-200ms)

**References:** FR17, FR18, AR11, UX-DR12, UX-DR24

---

## Epic 4: Advanced Dashboard Tools — "Map, calibration, and logo management"

Visual configuration tools: interactive map, display calibration with test pattern, logo upload with preview, logo list management.

**Note:** Stories 4.1–4.4 are **parallelizable** — no internal dependencies between them. They can be implemented in any order. All require Epic 2 (dashboard) and Epic 3 (LayoutEngine/LogoManager) to be complete.

### Story 4.1: Interactive Map with Circle Radius

As a user,
I want to see my capture area on a real map with a draggable circle,
So that I can visually understand and adjust my monitored airspace.

**Acceptance Criteria:**

**Given** dashboard Location card expanded
**When** section opens
**Then** Leaflet lazy-loads (~28KB gzipped), "Loading map..." shown until tiles render
**And** map centers on current center_lat/center_lon

**Given** map loaded
**When** rendered
**Then** draggable marker at center, resizable circle at radius, OSM tiles visible

**Given** marker dragged or radius slider adjusted
**When** released
**Then** form values update, POST /api/settings fires, toast confirms "Location updated"

**Given** tile server unreachable
**When** Leaflet fails
**Then** lat/lon/radius input fields shown as fallback (no error modal)

**References:** FR47, UX-DR11

### Story 4.2: Display Calibration & Test Pattern

As a user,
I want to adjust wiring flags and see a test pattern update in real-time,
So that I can configure panels correctly without trial-and-error.

**Acceptance Criteria:**

**Given** Calibration section visible
**When** origin corner dropdown changed
**Then** POST fires, LED test pattern updates immediately, canvas preview updates

**Given** scan direction or zigzag toggle changed
**When** setting applied
**Then** test pattern and canvas both update in real-time

**Given** calibration complete
**When** navigating away
**Then** test pattern stops, normal flight display resumes

**Given** triple feedback during calibration
**When** any wiring flag changes
**Then** form → canvas (instant) → LED test pattern (50-200ms)

**References:** FR15, FR16, UX-DR24

### Story 4.3: Logo Upload with RGB565 Preview

As a user,
I want to upload logo files and see a preview before committing,
So that I can verify logos are correct.

**Acceptance Criteria:**

**Given** Logos section upload area
**When** files selected or dragged
**Then** multiple .bin files accepted (batch upload)

**Given** each file validated client-side
**When** not .bin or not 2048 bytes
**Then** error shown: "[filename] — invalid format or size"

**Given** valid file
**When** RGB565 decoder runs
**Then** 32x32 canvas (scaled 128x128) shows decoded logo preview next to filename

**Given** upload proceeds
**When** each file POSTed to /api/logos
**Then** success: toast "N logos uploaded"; failure: red toast with specific error ("LittleFS full")
**And** successful files upload even if some fail

**References:** FR34, FR36, UX-DR14, NFR17

### Story 4.4: Logo List Management & Deletion

As a user,
I want to see uploaded logos with previews and delete ones I don't need,
So that I can manage my collection and free storage.

**Acceptance Criteria:**

**Given** Logos section loads
**When** GET /api/logos returns list
**Then** each row shows: canvas preview thumbnail, ICAO filename, file size, delete button

**Given** no logos uploaded
**When** section loads
**Then** empty state: "No logos uploaded yet. Drag .bin files here to add airline logos."

**Given** delete button tapped
**When** inline confirmation appears
**Then** "Delete [filename]?" with Confirm/Cancel (no modal)

**Given** Confirm tapped
**When** DELETE /api/logos/[filename] fires
**Then** file removed from LittleFS, list refreshes, toast "Logo deleted"

**Given** logo list populated
**When** scrolling
**Then** storage usage shown: "Storage: 847 KB / 1.9 MB used (28 logos)"

**References:** FR35, FR37, UX-DR15, UX-DR20

---

# --- Foundation Release (Epics 5-7) ---

## Epic 5: OTA Firmware Updates & Settings Migration — "Cut the Cord"

User can back up their settings, flash the dual-OTA partition firmware one final time via USB, restore settings in the wizard, then upload all future firmware from the dashboard with progress feedback, validation, and automatic rollback on failure.

### Story 5.1: Partition Table & Build Configuration

As a developer,
I want a dual-OTA partition table and firmware version build flag,
So that the device supports over-the-air updates with a known firmware identity.

**Acceptance Criteria:**

**Given** the current firmware binary
**When** built via `pio run`
**Then** the binary size is measured and logged
**And** if the binary exceeds 1.3MB, a warning is documented with optimization recommendations (disable Bluetooth via `-D CONFIG_BT_ENABLED=0`, strip unused library features) before proceeding

**Given** `firmware/custom_partitions.csv` updated with dual-OTA layout: nvs 20KB (0x9000), otadata 8KB (0xE000), app0 1.5MB (0x10000), app1 1.5MB (0x190000), spiffs 960KB (0x310000)
**When** `platformio.ini` references the custom partition table and includes `-D FW_VERSION=\"1.0.0\"` in build_flags
**Then** `pio run` builds successfully with no errors
**And** the compiled binary is under 1.5MB

**Given** the new partition layout is flashed via USB
**When** the device boots
**Then** LittleFS mounts successfully on the 960KB spiffs partition
**And** all existing functionality works: flight data pipeline, web dashboard, logo management, calibration tools
**And** `pio run -t uploadfs` uploads web assets and logos to LittleFS with at least 500KB free remaining

**Given** `FW_VERSION` is defined as a build flag
**When** any source file references `FW_VERSION`
**Then** the version string is available at compile time

**References:** FR35, AR1, AR2, AR14, NFR-C1, NFR-C2

### Story 5.2: ConfigManager Expansion — Schedule Keys & SystemStatus

As a developer,
I want ConfigManager to support 5 new schedule-related NVS keys and SystemStatus to track OTA and NTP subsystems,
So that night mode configuration and health reporting infrastructure is ready for all Foundation features.

**Acceptance Criteria:**

**Given** ConfigManager initialized on first boot (empty NVS for new keys)
**When** `getSchedule()` is called
**Then** it returns a `ScheduleConfig` struct with defaults: `timezone="UTC0"`, `enabled=false`, `dim_start=1380` (23:00), `dim_end=420` (07:00), `dim_brightness=10`

**Given** ConfigManager has schedule values in NVS
**When** `getSchedule()` is called
**Then** NVS values override defaults in the returned struct
**And** NVS keys use 15-char abbreviations: `timezone`, `sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt`

**Given** `applyJson()` is called with `{"timezone": "PST8PDT,M3.2.0,M11.1.0", "sched_enabled": 1}`
**When** processing schedule keys
**Then** RAM cache updates immediately
**And** `onChange` callbacks fire
**And** `reboot_required` is `false` (all 5 schedule keys are hot-reload)
**And** total NVS usage for new keys is under 256 bytes

**Given** `GET /api/settings` is called
**When** response is built
**Then** all 5 new schedule keys appear in the flat JSON response alongside existing keys

**Given** SystemStatus is initialized
**When** `SystemStatus::set(OTA, ...)` or `SystemStatus::set(NTP, ...)` is called
**Then** OTA and NTP subsystem entries appear in the health snapshot via `toJson()`

**References:** FR37, AR9, AR10, AR11, NFR-C3

### Story 5.3: OTA Upload Endpoint

As a user,
I want to upload a firmware binary to the device over the local network,
So that I can update the firmware without connecting a USB cable.

**Acceptance Criteria:**

**Given** `POST /api/ota/upload` receives a multipart file upload
**When** the first chunk arrives with first byte `0xE9` (ESP32 magic byte)
**Then** `Update.begin(partition->size)` is called using the next OTA partition's capacity (not `Content-Length`)
**And** each subsequent chunk is written via `Update.write(data, len)`
**And** the upload streams directly to flash with no full-binary RAM buffering

**Given** the first byte of the uploaded file is NOT `0xE9`
**When** the first chunk is processed
**Then** the endpoint returns HTTP 400 with `{ "ok": false, "error": "Not a valid ESP32 firmware image" }`
**And** no data is written to flash

**Given** `Update.write()` returns fewer bytes than expected
**When** a write failure is detected
**Then** `Update.abort()` is called
**And** the current firmware continues running unaffected
**And** an error response is returned

**Given** WiFi drops mid-upload
**When** the connection is interrupted
**Then** `Update.abort()` is called
**And** the inactive partition is NOT marked bootable
**And** the device continues running current firmware

**Given** the upload completes successfully
**When** `Update.end(true)` returns true on the final chunk
**Then** the endpoint returns `{ "ok": true, "message": "Rebooting..." }`
**And** `ESP.restart()` is called after a 500ms delay
**And** the device reboots into the newly written firmware

**Given** a 1.5MB firmware binary uploaded over local WiFi
**When** the transfer completes
**Then** the total upload time is under 30 seconds
**And** validation (size + magic byte) completes within 1 second for invalid files

**References:** FR1, FR2, FR3, FR4, FR8, FR11, AR3, AR12, AR18, AR-E12, AR-E13, NFR-P1, NFR-P7, NFR-R7, NFR-C4

### Story 5.4: OTA Self-Check & Rollback Detection

As a user,
I want the device to verify new firmware works after an OTA update and automatically roll back if it doesn't,
So that a bad firmware update never bricks my device.

**Acceptance Criteria:**

**Given** the device boots after an OTA update
**When** WiFi connects successfully
**Then** `esp_ota_mark_app_valid_cancel_rollback()` is called
**And** the partition is marked valid
**And** a log message records the time to mark valid (e.g., "Marked valid, WiFi connected in 8s")

**Given** the device boots after an OTA update and WiFi does not connect
**When** 60 seconds elapse since boot
**Then** `esp_ota_mark_app_valid_cancel_rollback()` is called on timeout
**And** `SystemStatus::set(OTA, WARNING, "Marked valid on timeout — WiFi not verified")` is recorded

**Given** the device boots after an OTA update and crashes before the self-check completes
**When** the watchdog fires and the device reboots
**Then** the bootloader detects the active partition was never marked valid
**And** the bootloader rolls back to the previous partition automatically

**Given** a rollback has occurred
**When** `esp_ota_get_last_invalid_partition()` returns non-NULL during boot
**Then** a `rollbackDetected` flag is set to `true`
**And** `SystemStatus::set(OTA, WARNING, "Firmware rolled back to previous version")` is recorded

**Given** `GET /api/status` is called
**When** the response is built
**Then** it includes `"firmware_version": "1.0.0"` (from `FW_VERSION` build flag)
**And** `"rollback_detected": true/false`

**Given** normal boot (not after OTA)
**When** the self-check runs
**Then** `esp_ota_mark_app_valid_cancel_rollback()` is called (idempotent — safe on non-OTA boots)

**References:** FR5, FR6, FR7, FR9, FR36, AR4, AR5, AR13, AR14, NFR-R3

### Story 5.5: Settings Export Endpoint

As a user,
I want to download all my device settings as a JSON file,
So that I can back up my configuration before a partition migration and reference it during reconfiguration.

**Acceptance Criteria:**

**Given** `GET /api/settings/export` is called
**When** the response is built
**Then** all ConfigManager keys are dumped as a flat JSON object
**And** `Content-Disposition: attachment; filename=flightwall-settings.json` header is set
**And** the response includes `"flightwall_settings_version": 1` and `"exported_at": "<ISO-8601 timestamp>"` metadata fields
**And** all 23+ config keys (existing MVP + 5 new schedule keys) are present

**Given** the exported JSON file
**When** inspected
**Then** it is valid JSON parseable by any standard JSON parser
**And** it includes WiFi credentials (acceptable — single-user local device)
**And** all values match the current NVS state

**Given** a settings export followed by a settings import (Story 5.7)
**When** the round-trip is tested
**Then** all config values survive the export → import cycle without data loss or corruption

**References:** FR12, AR12, NFR-R6

### Story 5.6: Dashboard Firmware Card & OTA Upload UI

As a user,
I want a Firmware card on the dashboard where I can upload firmware, see progress, view the current version, and be notified of rollbacks,
So that I can manage firmware updates from my phone.

**Acceptance Criteria:**

**Given** the dashboard loads
**When** `GET /api/status` returns firmware data
**Then** the Firmware card displays the current firmware version (e.g., "v1.0.0")
**And** the Firmware card is always expanded (not collapsed by default)
**And** the dashboard loads within 3 seconds on the local network

**Given** the Firmware card is visible
**When** no file is selected
**Then** the OTA Upload Zone (`.ota-upload-zone`) shows: dashed border, "Drag .bin file here or tap to select" text
**And** the zone has `role="button"`, `tabindex="0"`, `aria-label="Select firmware file for upload"`
**And** Enter/Space keyboard triggers the file picker

**Given** a file is dragged over the upload zone
**When** the drag enters the zone
**Then** the border becomes solid `--accent` and background lightens slightly

**Given** a valid `.bin` file is selected (≤1.5MB, first byte `0xE9`)
**When** client-side validation passes
**Then** the filename is displayed and the "Upload Firmware" primary button is enabled

**Given** an invalid file is selected (wrong size or missing magic byte)
**When** client-side validation fails
**Then** an error toast fires with a specific message (e.g., "File too large — maximum 1.5MB for OTA partition" or "Not a valid ESP32 firmware image")
**And** the upload zone resets to empty state

**Given** "Upload Firmware" is tapped
**When** the upload begins via XMLHttpRequest
**Then** the upload zone is replaced by the OTA Progress Bar (`.ota-progress`)
**And** the progress bar shows percentage (0-100%) with `role="progressbar"`, `aria-valuenow`/`min`/`max`
**And** the percentage updates at least every 2 seconds

**Given** the upload completes successfully
**When** the server responds with `{ "ok": true }`
**Then** a 3-second countdown displays ("Rebooting in 3... 2... 1...")
**And** polling begins for device reachability (GET /api/status)
**And** on successful poll, the new version string is displayed
**And** a success toast shows "Updated to v[new version]"

**Given** the device is unreachable after 60 seconds of polling
**When** the timeout expires
**Then** an amber message shows "Device unreachable — try refreshing" with explanatory text

**Given** `rollback_detected` is `true` in `/api/status`
**When** the dashboard loads
**Then** a Persistent Banner (`.banner-warning`) appears in the Firmware card: "Firmware rolled back to previous version"
**And** the banner has `role="alert"`, `aria-live="polite"`, hardcoded `#2d2738` background
**And** a dismiss button sends `POST /api/ota/ack-rollback` and removes the banner
**And** the banner persists across page refreshes until dismissed (NVS-backed)

**Given** the System card on the dashboard
**When** rendered
**Then** a "Download Settings" secondary button triggers `GET /api/settings/export`
**And** helper text in the Firmware card links to the System card: "Before migrating, download your settings backup from System below"

**Given** all new CSS for OTA components
**When** added to `style.css`
**Then** total addition is approximately 35 lines (~150 bytes gzipped)
**And** all components meet WCAG 2.1 AA contrast requirements
**And** all interactive elements have 44x44px minimum touch targets
**And** all components work at 320px minimum viewport width

**Given** updated web assets (dashboard.html, dashboard.js, style.css)
**When** gzipped and placed in `firmware/data/`
**Then** the gzipped copies replace existing ones for LittleFS upload

**References:** FR10, FR30, FR32, FR33, UX-DR1, UX-DR2, UX-DR3, UX-DR7, UX-DR8, UX-DR9, UX-DR10, UX-DR14, UX-DR15, UX-DR16, NFR-P2, NFR-P6

### Story 5.7: Settings Import in Setup Wizard

As a user,
I want to import a previously exported settings file in the setup wizard,
So that I can quickly reconfigure my device after a partition migration without retyping API keys and coordinates.

**Acceptance Criteria:**

**Given** the setup wizard loads
**When** an import option is available (file upload zone or button)
**Then** the user can select a `.json` file via tap-to-select (reusing `.logo-upload-zone` pattern)

**Given** a valid `flightwall-settings.json` file is selected
**When** the browser reads the file via FileReader API
**Then** the JSON is parsed client-side
**And** recognized config keys pre-fill their corresponding wizard form fields across all steps (WiFi, API keys, location, hardware)
**And** the user can navigate forward through pre-filled steps, reviewing each before confirming

**Given** the imported JSON contains unrecognized keys (e.g., `flightwall_settings_version`, `exported_at`, or future keys)
**When** processing the import
**Then** unrecognized keys are silently ignored without error
**And** all recognized keys are still pre-filled correctly

**Given** the imported file is not valid JSON
**When** parsing fails
**Then** an error toast shows "Could not read settings file — invalid format"
**And** the wizard continues without pre-fill (manual entry still available)

**Given** the imported file is valid JSON but contains no recognized config keys
**When** processing completes
**Then** a warning toast shows "No recognized settings found in file"
**And** all form fields remain at their defaults

**Given** settings are imported and the user completes the wizard
**When** "Save & Connect" is tapped
**Then** settings are applied via the normal `POST /api/settings` path (no new server-side import endpoint)

**References:** FR13, FR14, AR16, AR-E16, UX-DR9, UX-DR11

---

## Epic 6: Night Mode & Brightness Scheduling — "The Wall That Sleeps"

User configures a timezone and brightness schedule on the dashboard. The wall automatically dims at night and restores in the morning — correct across midnight boundaries and DST transitions. NTP sync status is visible, and the schedule degrades gracefully if NTP is unavailable.

### Story 6.1: NTP Time Sync & Timezone Configuration

As a user,
I want the device to sync its clock after connecting to WiFi using my configured timezone,
So that time-dependent features like night mode have an accurate local clock.

**Acceptance Criteria:**

**Given** the device connects to WiFi in STA mode
**When** WiFiManager reports `STA_CONNECTED`
**Then** `configTzTime()` is called with the POSIX timezone string from `ConfigManager::getSchedule().timezone` and NTP servers `"pool.ntp.org"`, `"time.nist.gov"`
**And** NTP synchronization completes within 10 seconds of WiFi connection

**Given** NTP synchronization succeeds
**When** the `sntp_set_time_sync_notification_cb` callback fires
**Then** a `std::atomic<bool> ntpSynced` flag is set to `true`
**And** `SystemStatus::set(NTP, OK, "Clock synced")` is recorded
**And** subsequent calls to `getLocalTime(&now, 0)` return the correct local time for the configured timezone

**Given** NTP is unreachable after WiFi connects
**When** the sync attempt fails
**Then** `ntpSynced` remains `false`
**And** `SystemStatus::set(NTP, WARNING, "Clock not set")` is recorded
**And** the device does NOT crash or degrade any functionality other than schedule activation
**And** NTP re-sync is automatically retried by LWIP SNTP (default ~1 hour interval)

**Given** the timezone config key is changed via `POST /api/settings`
**When** the hot-reload callback fires
**Then** `configTzTime()` is called immediately with the new POSIX timezone string
**And** no reboot is required

**Given** `dashboard.js` includes a `TZ_MAP` object
**When** the timezone mapping is loaded
**Then** it contains ~50-80 common IANA-to-POSIX entries (e.g., `"America/Los_Angeles": "PST8PDT,M3.2.0,M11.1.0"`)
**And** `getTimezoneConfig()` returns `{ iana, posix }` using `Intl.DateTimeFormat().resolvedOptions().timeZone`

**Given** `GET /api/status` is called
**When** the response is built
**Then** it includes `"ntp_synced": true/false` and `"schedule_active": true/false`

**References:** FR15, FR18, FR22, AR6, AR15, NFR-P4, NFR-R2, NFR-R5

### Story 6.2: Night Mode Brightness Scheduler

As a user,
I want the device to automatically dim the LEDs during my configured night hours and restore brightness in the morning,
So that the wall is livable 24/7 without daily manual adjustment.

**Acceptance Criteria:**

**Given** schedule is enabled (`sched_enabled=1`) and NTP has synced (`ntpSynced=true`)
**When** the current local time enters the dim window (e.g., current time is 23:15 with dim_start=1380, dim_end=420)
**Then** LED brightness is overridden to `sched_dim_brt` via the existing ConfigManager hot-reload path
**And** the brightness change is reflected on the LED matrix within 1 second

**Given** a same-day schedule (dim_start < dim_end, e.g., 09:00-17:00)
**When** the scheduler checks `currentMinutes >= dimStart && currentMinutes < dimEnd`
**Then** the device is in the dim window during the specified hours and at normal brightness outside

**Given** a midnight-crossing schedule (dim_start > dim_end, e.g., 23:00-07:00)
**When** the scheduler checks `currentMinutes >= dimStart || currentMinutes < dimEnd`
**Then** the device is in the dim window from 23:00 through midnight to 07:00

**Given** a DST transition occurs (e.g., clocks spring forward at 2:00 AM)
**When** `configTzTime()` with the POSIX timezone string handles the transition
**Then** the schedule continues to operate correctly in local time without drift or double-triggering

**Given** schedule is enabled but NTP has NOT synced
**When** the scheduler runs
**Then** it skips the brightness check entirely (no override applied)
**And** manual brightness settings remain in effect

**Given** schedule is disabled (`sched_enabled=0`)
**When** the scheduler runs
**Then** it does nothing — manual brightness applies
**And** the configured times and brightness are preserved in NVS for re-enabling

**Given** the scheduler runs in the main loop on Core 1
**When** `getLocalTime(&now, 0)` is called with timeout=0
**Then** the call is non-blocking (returns immediately if time unavailable)
**And** the total scheduler overhead is a single `localtime_r()` + integer comparison per iteration
**And** main loop cycle time increases by no more than 1%

**Given** the device runs for 30+ days with schedule enabled
**When** operating continuously
**Then** the schedule executes correctly across every midnight boundary and DST transition
**And** NTP re-syncs automatically (LWIP default ~1 hour) to prevent RTC drift

**References:** FR19, FR20, FR21, FR23, FR24, AR7, AR8, AR-E14, AR-E15, NFR-P3, NFR-R1, NFR-R4, NFR-C5

### Story 6.3: Dashboard Night Mode Card

As a user,
I want a Night Mode card on the dashboard where I can set my timezone, configure a dim schedule, see a visual timeline, and monitor NTP sync status,
So that I can set up and manage night mode from my phone.

**Acceptance Criteria:**

**Given** the dashboard loads
**When** the Night Mode card renders
**Then** the card is always expanded (not collapsed by default)
**And** it fetches current settings via `GET /api/settings` and status via `GET /api/status`

**Given** the Night Mode card is visible
**When** rendered with current settings
**Then** it displays: timezone selector dropdown, schedule enable/disable toggle, dim start time picker, dim end time picker, dim brightness slider with value + "%" suffix, 24-hour timeline bar, and schedule status line

**Given** the timezone selector dropdown
**When** rendered
**Then** it lists IANA timezone names from `TZ_MAP`, pre-selected to the current configured timezone
**And** if the user's browser-detected timezone is in `TZ_MAP`, it is suggested as default
**And** if the current timezone is not in `TZ_MAP`, a manual POSIX entry field is shown

**Given** the Time Picker Row (`.time-picker-row`)
**When** rendered with schedule enabled
**Then** two side-by-side `<input type="time">` elements show dim start and dim end
**And** each has a proper `<label for>` association
**And** values are converted from minutes-since-midnight to HH:MM for display and back on change
**And** at 320px viewport width, each picker gets ~45% card width with gap

**Given** the schedule toggle is switched OFF
**When** the toggle state changes
**Then** time pickers and brightness slider become visually disabled (reduced opacity)
**And** the configured times and brightness are NOT deleted — toggling ON restores them

**Given** the dim brightness slider
**When** adjusted
**Then** the value displays inline as the user drags (existing `.range-val` pattern)
**And** the value shows with "%" suffix

**Given** the Timeline Bar (`.schedule-timeline`)
**When** rendered with time picker values
**Then** a canvas (24px height, full card width) shows the dim period as a shaded `--accent-dim` region on a `--bg-input` track
**And** hour markers (00, 06, 12, 18, 24) are rendered as HTML text below the canvas (not canvas text)
**And** a "now" marker (1px `--accent` vertical line) shows the current hour
**And** midnight-crossing schedules render as two `fillRect` calls (00:00-end and start-24:00)
**And** the canvas is `aria-hidden="true"` (time pickers are the accessible data)
**And** the timeline updates in real-time as time picker values change

**Given** the Schedule Status Line (`.schedule-status`)
**When** rendered based on current state
**Then** it shows the correct state with colored dot prefix:
- Active dimming: `--accent-dim` dot, "Dimmed (10%) until 07:00"
- Scheduled: `--success` dot, "Schedule active — next dim at 23:00"
- Waiting: `--warning` dot, "Schedule saved — waiting for clock sync"
- Clock not set: `--warning` dot, "Clock not set — schedule inactive"
- Disabled: `--text-secondary` dot, "Schedule disabled"
**And** the dot is `aria-hidden="true"`, text communicates the full state

**Given** any Night Mode setting is changed
**When** the setting differs from the last-saved state
**Then** a sticky apply bar (`.apply-bar`) appears at the bottom with "Unsaved changes" label and "Apply Changes" primary button
**And** tapping "Apply Changes" sends `POST /api/settings` with the changed schedule keys
**And** on success, a green toast confirms "Night mode schedule saved"
**And** the apply bar disappears
**And** no reboot is required (hot-reload keys)

**Given** all new CSS for Night Mode components
**When** added to `style.css`
**Then** total addition is approximately 25 lines
**And** all components meet WCAG 2.1 AA contrast requirements
**And** all interactive elements have 44x44px minimum touch targets
**And** all components work at 320px minimum viewport width with no new media queries

**Given** updated web assets (dashboard.html, dashboard.js, style.css)
**When** gzipped and placed in `firmware/data/`
**Then** the gzipped copies replace existing ones for LittleFS upload

**References:** FR16, FR19, FR22, FR23, FR31, FR34, AR6, UX-DR4, UX-DR5, UX-DR6, UX-DR7, UX-DR8, UX-DR10, UX-DR12, UX-DR13, NFR-P6

---

## Epic 7: Onboarding Polish — "Fresh Start Done Right"

New user completes the setup wizard with a "Test Your Wall" step that verifies panel layout matches expectations before showing garbled flight data. Timezone auto-detects from the browser.

### Story 7.1: Wizard Step 6 — Test Your Wall

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

### Story 7.2: Wizard Timezone Auto-Detect

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

---

# --- Display System Release (Epics 8-10) ---

## Epic 8: Display Mode Architecture & Classic Card Migration

The device renders flight data through a pluggable display mode system, with the existing Classic Card output migrated into the first mode — pixel-identical to the current Foundation Release output. The emergency fallback renderer ensures the display always works even if a mode fails to initialize.

### Story 8.1: Heap Baseline Measurement & Core Abstractions

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

### Story 8.2: DisplayUtils Extraction

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

### Story 8.3: ModeRegistry & Static Registration System

As a firmware developer,
I want a ModeRegistry that manages a static table of display modes with activation lifecycle, heap validation, and fallback,
So that modes can be registered at compile time and switched safely at runtime.

**Acceptance Criteria:**

**Given** the DisplayMode interface exists from Story 8.1
**When** ModeRegistry is implemented
**Then** a MODE_TABLE[] static array in main.cpp holds pointers to all registered DisplayMode instances
**And** ModeRegistry::enumerate() returns mode count and names with zero heap allocation and completes under 10ms (NFR P4)
**And** ModeRegistry::activate(id) executes the full lifecycle: teardown current → validate heap → init new → begin rendering (FR6)
**And** if ESP.getFreeHeap() after teardown is below the requested mode's getMemoryRequirement() + 30KB safety margin, activation is rejected and the previous mode is restored (FR8, FR9, NFR S4)
**And** concurrent activate() calls are serialized with queue-of-one semantics — a switch initiated during an in-progress transition replaces any pending request (latest wins, previous pending discarded), not an unbounded queue (FR7)
**And** mode switch requests from WebPortal (Core 1) are communicated to the display task (Core 0) via an atomic flag pattern matching existing g_configChanged (AR2)
**And** adding a new mode requires exactly two touch points: one .h/.cpp pair in modes/ and one line in MODE_TABLE[] (AR5)

**Requirements:** FR5, FR6, FR7, FR8, FR9, FR10, AR2, AR5, NFR P4, NFR S1, NFR S2, NFR S4

### Story 8.4: Classic Card Mode Implementation

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

### Story 8.5: Display Pipeline Integration

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

## Epic 9: Live Flight Card Mode

Users see a richer flight display with real-time telemetry — altitude, ground speed, heading, and vertical rate — with the layout adapting gracefully when zone dimensions cannot fit all fields. A visual climb/descend/level indicator makes aircraft state immediately clear.

### Story 9.1: Live Flight Card Mode — Enriched Telemetry Rendering

As a flight enthusiast,
I want to see a richer flight card displaying airline, route, altitude, ground speed, heading, and vertical rate,
So that I get detailed real-time information about aircraft overhead at a glance.

**Acceptance Criteria:**

**Given** the mode system from Epic 8 is operational and ClassicCardMode is the active default
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

### Story 9.2: Adaptive Field Dropping & Vertical Direction Indicator

As a flight enthusiast,
I want the Live Flight Card to gracefully adapt when the display zone is too small for all fields, and to show a visual climb/descend/level indicator,
So that I always see the most important information and can instantly tell what an aircraft is doing.

**Acceptance Criteria:**

**Given** LiveFlightCardMode renders the full telemetry layout from Story 9.1
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

## Epic 10: Mode Switching API & Dashboard Mode Picker

Users gain control over how their FlightWall displays flight information — choosing between display modes from the web dashboard with schematic previews of each mode's layout. Clear transition feedback, error handling, and NVS persistence ensure the user's choice is respected across power cycles. An upgrade notification guides first-time users to explore the new modes.

### Story 10.1: Display Mode API Endpoints

As an external client (web dashboard or third-party tool),
I want HTTP API endpoints to list available modes, retrieve the active mode, and request a mode change,
So that the mode system can be controlled programmatically from any client.

**Acceptance Criteria:**

**Given** the mode system from Epic 8 is operational
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

### Story 10.2: NVS Mode Persistence & Boot Restore

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

### Story 10.3: Mode Picker Section — Cards & Schematic Previews

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

### Story 10.4: Mode Switching Flow & Transition States

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

### Story 10.5: Accessibility & Responsive Layout

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

### Story 10.6: Upgrade Notification Banner

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

---

# --- Delight Release (Epics 11-17) ---

## Epic 11: Clock Mode & Orchestration Foundation — "The Wall Never Sleeps"

The owner sees a functioning display 100% of the time. When no flights are in range, Clock Mode activates automatically. The owner can manually switch modes from the dashboard. Watchdog recovery reboots to Clock Mode as a known-good default. New config keys introduced by the Delight Release resolve to safe defaults on first read.

### Story 11.1: Clock Mode Time Display

As an owner,
I want the wall to display the current time in a large-format layout on the LED matrix,
So that the wall always shows something useful even when I'm not tracking flights.

**Acceptance Criteria:**

**Given** the device has booted and NTP time is synced
**When** Clock Mode is the active display mode
**Then** the LED matrix displays the current time in large-format digits readable from across the room
**And** the time updates every second without flickering or artifacts

**Given** Clock Mode is active
**When** the `m_clock_format` NVS key is absent or set to default (0)
**Then** the clock displays time in 24-hour format (e.g., "14:30")

**Given** Clock Mode is active
**When** the `m_clock_format` NVS key is set to 1
**Then** the clock displays time in 12-hour format (e.g., "2:30")

**Given** the device is upgrading from a prior firmware version that lacks Delight config keys
**When** `ConfigManager::getModeSetting("clock", "format", 0)` is called and the NVS key `m_clock_format` does not exist
**Then** the method returns the default value (0 = 24h) without error and without overwriting any existing stored NVS keys (FR33)

**Given** NTP has not synced yet after boot
**When** Clock Mode is active
**Then** the display shows a static time indicator or uptime-based time rather than a blank screen

**Given** ClockMode is implemented
**When** the firmware is compiled
**Then** `ClockMode` implements the `DisplayMode` interface (`init()`, `render()`, `teardown()`, `getName()`, `getZoneDescriptor()`), declares `CLOCK_SETTINGS[]` and `CLOCK_SCHEMA` as `static const` in `ClockMode.h` (rule 29), is registered in `MODE_TABLE[]` in `main.cpp` with a non-null `settingsSchema` pointer, and `ModeEntry` struct includes the new `settingsSchema` field (AR8)

**Given** `ConfigManager` does not yet have per-mode setting helpers
**When** this story is implemented
**Then** `ConfigManager::getModeSetting(abbrev, key, default)` and `setModeSetting(abbrev, key, value)` are added, constructing NVS keys as `m_{abbrev}_{key}` (rule 28), and `ModeSettingDef` + `ModeSettingsSchema` structs are added to `interfaces/DisplayMode.h`

### Story 11.2: Idle Fallback to Clock Mode

As an owner,
I want the wall to automatically switch to Clock Mode when no flights are in range and switch back when flights return,
So that the wall never shows a blank screen or loading spinner — guests always see a functioning display.

**Acceptance Criteria:**

**Given** a display mode other than Clock Mode is active and showing flight data
**When** the flight data pipeline returns zero aircraft in range
**Then** within one poll interval (~30s, NFR3) the wall switches to Clock Mode automatically

**Given** Clock Mode is active due to idle fallback (zero flights)
**When** a new flight enters tracking range
**Then** the wall switches back to the owner's previously active mode (the MANUAL selection)

**Given** the system has just booted
**When** the first `ModeOrchestrator::tick()` executes (~1 second after boot) and zero flights are in range
**Then** the orchestrator transitions from MANUAL to IDLE_FALLBACK state and requests a switch to Clock Mode

**Given** `ModeOrchestrator` is implemented
**When** the firmware is compiled
**Then** `core/ModeOrchestrator.h/.cpp` exists as a static class with `OrchestratorState` enum (MANUAL, SCHEDULED, IDLE_FALLBACK), `init()`, `tick()`, `onManualSwitch()`, `getState()`, and `getManualModeId()` methods

**Given** `g_flightCount` does not yet exist
**When** this story is implemented
**Then** `std::atomic<uint8_t> g_flightCount` is declared in `main.cpp` (rule 30), `FlightDataFetcher.cpp` stores the flight count after queue write via `g_flightCount.store()`, and `ModeOrchestrator::tick()` reads it via `g_flightCount.load()`

**Given** `ModeOrchestrator::tick()` is implemented
**When** it is wired into the Core 1 main loop
**Then** it executes at ~1-second intervals (non-blocking) and does not interfere with the existing brightness scheduler or flight data pipeline

### Story 11.3: Manual Mode Switching via Orchestrator

As an owner,
I want to manually switch between available display modes from the dashboard,
So that I can choose what the wall shows right now without waiting for automatic triggers.

**Acceptance Criteria:**

**Given** the owner is on the dashboard and the Mode Picker is visible
**When** the owner selects a different display mode
**Then** `POST /api/display/mode` routes through `ModeOrchestrator::onManualSwitch()` (rule 24) and the wall switches to the selected mode

**Given** the wall is in IDLE_FALLBACK state (showing Clock Mode because zero flights)
**When** the owner manually selects a different mode via the dashboard
**Then** the orchestrator exits IDLE_FALLBACK, enters MANUAL state, and switches to the selected mode

**Given** the wall is in MANUAL state
**When** `ModeOrchestrator::onManualSwitch()` is called
**Then** the manual mode selection is stored in `_manualModeId` (persisted via NVS `display_mode` key) so it survives power cycles

**Given** `WebPortal::POST /api/display/mode` currently calls `ModeRegistry::requestSwitch()` directly
**When** this story is implemented
**Then** the handler is updated to call `ModeOrchestrator::onManualSwitch(modeId)` instead, and `ModeRegistry::requestSwitch()` is called from exactly two methods: `ModeOrchestrator::tick()` and `ModeOrchestrator::onManualSwitch()` — no other call sites (rule 24)

### Story 11.4: Watchdog Recovery to Default Mode

As an owner,
I want the device to recover automatically from a crash by rebooting into Clock Mode,
So that a display mode bug causes a brief reboot, not a bricked or stuck device.

**Acceptance Criteria:**

**Given** the watchdog timer detects an unresponsive main loop
**When** the watchdog triggers a reboot
**Then** the device reboots within 10 seconds (NFR10) and restarts in a known-good default mode without user intervention (NFR11)

**Given** the device is booting after a watchdog reboot
**When** the NVS-persisted `display_mode` key references a mode whose `init()` fails
**Then** the system falls back to Clock Mode ("clock") as the default and logs the failure

**Given** the device is booting after a watchdog reboot
**When** the NVS-persisted `display_mode` key references a mode ID not found in `MODE_TABLE`
**Then** the system falls back to Clock Mode ("clock") as the default

**Given** Clock Mode is the known-good default
**When** the device boots normally (not a watchdog reboot)
**Then** the previously active mode is restored from NVS as before — Clock Mode default only applies when the stored mode is invalid or fails init

---

## Epic 12: Departures Board — "Multi-Flight Awareness"

The owner sees all tracked flights at a glance in an airport-style scrolling list. Rows appear and disappear dynamically as aircraft enter and leave the tracking radius. No cycling through one-at-a-time cards.

### Story 12.1: Departures Board Multi-Row Rendering

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
**Then** the display shows an empty state (no rows) rather than a blank or corrupted screen — the idle fallback to Clock Mode is handled by ModeOrchestrator (Epic 11), not by Departures Board itself

**Given** Departures Board is the second new mode added to MODE_TABLE
**When** the dashboard Mode Picker queries `GET /api/display/modes`
**Then** the response includes DeparturesBoardMode alongside ClockMode and existing modes, validating FR41 (compile-time registry extensibility) and FR42 (Mode Picker discovery)

### Story 12.2: Dynamic Row Add and Remove on Flight Changes

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

## Epic 13: Animated Transitions — "Polished Handoffs"

Mode switches feel intentional, not broken. Smooth fade animations replace hard cuts. A non-technical observer perceives the wall as a finished product, not a prototype.

### Story 13.1: Fade Transition Between Display Modes

As an owner,
I want mode switches to use a smooth fade animation instead of a hard cut,
So that the wall feels like a polished product and transitions look intentional to guests.

**Acceptance Criteria:**

**Given** any mode switch is triggered (manual, idle fallback, or scheduled)
**When** ModeRegistry's `tick()` executes the switch lifecycle
**Then** `_executeFadeTransition()` is called after the new mode's `init()` succeeds, producing a smooth crossfade from the outgoing frame to the incoming frame

**Given** a fade transition is executing
**When** the blend loop runs
**Then** the transition renders at minimum 15fps (NFR1) on the 160x32 LED matrix with no visible tearing or artifacts (FR12)

**Given** a fade transition is executing
**When** the blend loop completes all frames
**Then** the total transition duration is under 1 second (NFR2) — approximately 15 frames at ~66ms per frame

**Given** a mode switch is triggered
**When** `_executeFadeTransition()` allocates dual RGB888 buffers
**Then** two buffers of `160 * 32 * 3 = 15,360 bytes` each (~30KB total) are `malloc()`'d at transition start (AR1)

**Given** the fade transition has rendered its last blend frame
**When** the transition completes
**Then** both RGB888 buffers are `free()`'d immediately — buffers never persist beyond the fade call (rule 27) and no progressive heap degradation occurs (NFR17)

**Given** a mode switch is triggered
**When** `malloc()` returns `nullptr` for either buffer (heap too low)
**Then** both buffers are freed (`free(nullptr)` is a no-op), the transition falls back to an instant cut (current behavior), the mode still switches successfully, and a log message is emitted — this is graceful degradation, not an error (FR34, rule 27)

**Given** the fade transition is running on Core 0 (display task)
**When** `delay()` is called between blend frames
**Then** the Core 1 main loop (web server, flight pipeline, orchestrator) is unaffected — `SWITCHING` state prevents the display task's `tick()` from interfering

**Given** the updated switch flow in `ModeRegistry::tick()`
**When** a mode switch completes
**Then** the sequence is: `_switchState = SWITCHING` → `teardown()` → `delete old mode` → heap check → `factory()` → `init()` → `_executeFadeTransition()` → `_switchState = IDLE` → set NVS pending — matching the architecture decision DL1

---

## Epic 14: Mode Scheduling — "Ambient Intelligence"

The wall runs itself. The owner configures time-based rules once and forgets. Schedule and night mode brightness operate independently. The schedule takes priority over data-driven idle fallback when explicitly configured.

### Story 14.1: Schedule Rules Storage and Orchestrator Integration

As an owner,
I want to define time-based rules that automatically switch display modes at configured times,
So that the wall shows the right content at the right time without my intervention — Departures Board during the day, Clock Mode at night.

**Acceptance Criteria:**

**Given** the owner has defined one or more schedule rules
**When** the current time matches a rule's time window
**Then** the ModeOrchestrator transitions to SCHEDULED state and requests a switch to the rule's configured mode within 5 seconds of the configured time (NFR6)

**Given** a schedule rule is active (SCHEDULED state)
**When** the current time exits that rule's time window and no other rule matches
**Then** the orchestrator exits SCHEDULED state, returns to MANUAL, and restores the owner's manual mode selection

**Given** a schedule rule is active (SCHEDULED state)
**When** the flight data pipeline returns zero flights
**Then** the orchestrator stays in SCHEDULED state — the schedule takes priority over idle fallback (FR17)

**Given** the night mode brightness scheduler is active
**When** a mode schedule rule fires
**Then** the mode switches without resetting brightness, and a brightness change does not trigger a mode switch — the two schedulers operate independently (FR15)

**Given** schedule rules have been saved
**When** the device reboots or loses power
**Then** all schedule rules are restored from NVS on boot (FR36)

**Given** multiple schedule rules have overlapping time windows
**When** `ModeOrchestrator::tick()` evaluates rules
**Then** the first matching rule (lowest index) wins — rule priority is determined by index order

**Given** a schedule rule spans midnight (e.g., start 1320, end 360)
**When** `timeInWindow()` evaluates the current time
**Then** the midnight-crossing is handled correctly — same convention as Foundation brightness scheduler

**Given** `ConfigManager` does not yet have schedule storage
**When** this story is implemented
**Then** `ModeScheduleConfig` and `ScheduleRule` structs are added to `ConfigManager.h`, `getModeSchedule()` and `setModeSchedule()` methods are implemented using indexed NVS keys (`sched_r{N}_start`, `sched_r{N}_end`, `sched_r{N}_mode`, `sched_r{N}_ena`, `sched_r_count`) with a fixed max of 8 rules (AR4), all keys within 15-char NVS limit

**Given** NTP has not synced yet after boot
**When** `ModeOrchestrator::tick()` calls `getLocalTime(&now, 0)`
**Then** the non-blocking call returns false, tick skips schedule evaluation, and the device continues in MANUAL state until NTP syncs

### Story 14.2: Schedule Management Dashboard UI

As an owner,
I want to create, edit, and delete mode schedule rules through the dashboard,
So that I can configure the wall's automated behavior quickly without touching firmware.

**Acceptance Criteria:**

**Given** the owner opens the dashboard
**When** the schedule section loads
**Then** `GET /api/schedule` returns all configured rules with start time, end time, mode ID, enabled status, the current orchestrator state ("manual"/"scheduled"/"idle_fallback"), and the active rule index (-1 if none)

**Given** the owner creates a new schedule rule via the dashboard
**When** they set a start time, end time, and target mode and submit
**Then** `POST /api/schedule` saves the updated rule set to NVS via `ConfigManager::setModeSchedule()`, the response confirms `{ ok: true, data: { applied: true } }`, and ModeOrchestrator picks up the change on its next tick (~1 second)

**Given** the owner edits an existing schedule rule
**When** they change the start time, end time, mode, or enabled status and submit
**Then** the rule is updated in-place at its current index and persisted to NVS

**Given** the owner deletes a schedule rule from the middle of the list
**When** the deletion is submitted
**Then** higher-index rules shift down to fill the gap (compaction — no index gaps), `sched_r_count` decrements, and the compacted list is persisted to NVS

**Given** the owner has configured schedule rules
**When** viewing the schedule section
**Then** all rules are displayed with their current status — active rules visually distinguished from inactive ones (FR39), and the current orchestrator state is shown

**Given** the owner has made an error in a schedule rule (e.g., wrong times)
**When** they edit the entry inline
**Then** the correction is saved and takes effect within one orchestrator tick (~1 second) — no reboot required

**Given** the schedule management dashboard section
**When** it loads
**Then** times are displayed in the owner's local format, mode names match the display names from `GET /api/display/modes`, and the UI follows existing dashboard CSS and JS fetch patterns (NFR7: page load <1 second)

---

## Epic 15: Mode-Specific Settings — "Tailored Experience"

Each display mode is configurable through the Mode Picker. Clock Mode offers 12h/24h format, Departures Board configures row count and telemetry fields. Settings apply without reboot and persist across power cycles.

### Story 15.1: Per-Mode Settings Panels in Mode Picker

As an owner,
I want to configure each display mode's settings through the Mode Picker on the dashboard,
So that I can tailor Clock Mode's time format, Departures Board's row count, and other mode-specific options without touching firmware.

**Acceptance Criteria:**

**Given** the owner opens the Mode Picker on the dashboard
**When** `GET /api/display/modes` responds
**Then** each mode in the response includes a `settings` array containing the schema (key, label, type, default, min, max, enumOptions) and current values for each setting — or `null`/empty array for modes with no settings (ClassicCardMode, LiveFlightCardMode)

**Given** the API handler builds the settings array for a mode
**When** iterating settings
**Then** the handler iterates `ModeEntry.settingsSchema` dynamically (rule 28) — never hardcodes setting names — so that adding a setting to a mode's schema automatically surfaces it in the API response

**Given** the Mode Picker displays a mode with settings (e.g., Clock Mode)
**When** the settings panel renders
**Then** the UI dynamically generates form controls from the schema: dropdowns for `enum` type, number inputs for `uint8`/`uint16` with min/max constraints, toggles for `bool` type

**Given** the owner changes Clock Mode's time format from 24h to 12h in the Mode Picker
**When** the setting is submitted via `POST /api/display/mode` with a `settings` object
**Then** the setting is persisted to NVS via `ConfigManager::setModeSetting("clock", "format", 1)` and applied to the display within 2 seconds (NFR8) without requiring a device reboot (FR19)

**Given** the owner changes Departures Board's max rows from 4 to 2
**When** the setting is submitted
**Then** `ConfigManager::setModeSetting("depbd", "rows", 2)` persists the value and Departures Board renders at most 2 rows on the next frame

**Given** mode-specific settings have been saved
**When** the device reboots or loses power
**Then** all per-mode settings are restored from NVS (FR20) — `ConfigManager::getModeSetting()` reads the stored values using the `m_{abbrev}_{key}` NVS key convention

**Given** a mode with no settings schema (e.g., ClassicCardMode with `settingsSchema = nullptr`)
**When** the Mode Picker renders that mode's panel
**Then** no settings controls are shown — the panel displays only the mode name, description, and activation button

**Given** the Mode Picker settings panels
**When** the dashboard loads
**Then** the UI follows existing dashboard CSS and JS fetch patterns, loads within 1 second on a local network (NFR7), and uses the same `fetch + json.ok + showToast` pattern as other dashboard sections

---

## Epic 16: OTA Version Check & Notification — "Update Awareness"

The dashboard shows when a new firmware version is available. The owner can view release notes before deciding to update. GitHub API integration validated before any flash writes.

### Story 16.1: OTA Version Check Against GitHub Releases

As an owner,
I want to check whether a newer firmware version is available on GitHub,
So that I know when updates exist without manually browsing the repository.

**Acceptance Criteria:**

**Given** the OTAUpdater has not been initialized
**When** `OTAUpdater::init(repoOwner, repoName)` is called during `setup()`
**Then** the repo owner and name are stored in static char arrays and OTAUpdater state is set to IDLE

**Given** the owner triggers a version check
**When** `OTAUpdater::checkForUpdate()` executes
**Then** it sends `GET https://api.github.com/repos/{owner}/{repo}/releases/latest` over HTTPS, parses the JSON response via ArduinoJson to extract `tag_name`, `body` (release notes), and iterates `assets[]` to find `.bin` and `.sha256` download URLs

**Given** the GitHub API responds with a `tag_name` different from the compiled `FW_VERSION`
**When** `checkForUpdate()` compares versions
**Then** it stores the new version string, release notes (truncated to 512 chars if longer), binary URL, and SHA URL, sets state to `AVAILABLE`, and returns `true`

**Given** the GitHub API responds with a `tag_name` matching `FW_VERSION`
**When** `checkForUpdate()` compares versions
**Then** it sets state to `IDLE` and returns `false` — firmware is up to date

**Given** the GitHub API returns HTTP 429 (rate limit exceeded)
**When** `checkForUpdate()` handles the error
**Then** it sets state to `ERROR`, stores "Rate limited — try again later" in `_lastError`, and returns `false` — no automatic retry loop (NFR20)

**Given** the GitHub API is unreachable (network error, DNS failure, timeout)
**When** `checkForUpdate()` handles the error
**Then** it sets state to `ERROR`, stores a descriptive error message, and returns `false` — the device continues normal operation (NFR21: tolerates up to 10s latency without watchdog trigger)

**Given** `core/OTAUpdater` does not yet exist
**When** this story is implemented
**Then** `core/OTAUpdater.h` and `core/OTAUpdater.cpp` are created as a static class with `OTAState` enum (IDLE, CHECKING, AVAILABLE, DOWNLOADING, VERIFYING, REBOOTING, ERROR), `init()`, `checkForUpdate()`, and state/version/error accessors (AR3). `OTAUpdater::init()` is called in `main.cpp setup()`

### Story 16.2: Update Notification and Release Notes in Dashboard

As an owner,
I want to see a notification on the dashboard when a firmware update is available and read the release notes before deciding to install,
So that I can make an informed decision about whether and when to update.

**Acceptance Criteria:**

**Given** the owner opens the dashboard
**When** the OTA section loads
**Then** a "Check for Updates" button is available and the current firmware version is displayed

**Given** the owner taps "Check for Updates"
**When** `GET /api/ota/check` is called
**Then** the endpoint calls `OTAUpdater::checkForUpdate()` and returns `{ ok: true, data: { available: bool, version: string, current_version: string, release_notes: string } }`

**Given** a newer firmware version is available
**When** the check response returns `available: true`
**Then** the dashboard displays a banner: "Firmware {version} available — you're running {current_version}" with a "View Release Notes" link and an "Update Now" button (FR38)

**Given** the owner taps "View Release Notes"
**When** the release notes section expands
**Then** the GitHub Release description (from the `body` field) is displayed in the dashboard (FR23)

**Given** no update is available
**When** the check response returns `available: false`
**Then** the dashboard shows "Firmware is up to date" with the current version — no update banner or button

**Given** the version check fails (network error or rate limit)
**When** the check response returns an error state
**Then** the dashboard shows the error message from `OTAUpdater::getLastError()` (e.g., "Rate limited — try again later" or "Unable to check for updates") and no update banner appears

**Given** `GET /api/status` is called
**When** OTAUpdater state is AVAILABLE
**Then** the response includes `ota_available: true` and `ota_version: "{version}"` so the dashboard status bar can show update availability persistently

**Given** `GET /api/status` is called
**When** OTAUpdater state is IDLE or ERROR
**Then** the response includes `ota_available: false` and `ota_version: null`

**Given** the OTA dashboard section
**When** it loads
**Then** it follows existing dashboard CSS and JS fetch patterns, loads within 1 second on a local network (NFR7), and does not automatically poll GitHub — the check is only triggered by the owner's explicit action

---

## Epic 17: OTA Download & Safety — "Painless Updates"

One tap to install a firmware update over WiFi. A/B partition safety means zero bricked devices. Failed downloads leave firmware unchanged. SHA-256 verification ensures integrity. All settings preserved across updates. Self-service onboarding documentation validated.

### Story 17.1: OTA Download with Incremental SHA-256 Verification

As an owner,
I want to download and install a firmware update over WiFi with one tap,
So that I can update my device without finding a USB cable, opening PlatformIO, or pulling from git.

**Acceptance Criteria:**

**Given** OTAUpdater state is AVAILABLE (version check completed in Epic 16)
**When** `OTAUpdater::startDownload()` is called
**Then** a one-shot FreeRTOS task is spawned on Core 1 (pinned, 8KB stack, priority 1) and the method returns immediately

**Given** the download task starts
**When** the pre-OTA sequence executes
**Then** `ModeRegistry::prepareForOTA()` is called (rule 26) which tears down the active mode, frees mode heap, sets `_switchState = SWITCHING` to block the display task, and the LED matrix shows a static "Updating..." progress screen (FR26)

**Given** `prepareForOTA()` has executed
**When** the download task checks heap
**Then** it verifies at least 80KB free heap (FR34, NFR16) — if insufficient, it sets state to ERROR with "Not enough memory — restart device and try again" and returns without calling `Update.begin()`

**Given** heap is sufficient
**When** the download task begins
**Then** it first downloads the companion `.sha256` release asset (~64 bytes), parses it as a plain 64-character hex string — if the asset is missing or unparseable, it sets state to ERROR with "Release missing integrity file" and returns (AR9)

**Given** the SHA-256 hash is downloaded
**When** the binary download starts
**Then** the task gets the next OTA partition via `esp_ota_get_next_update_partition(NULL)`, calls `Update.begin(partition->size)`, initializes `mbedtls_sha256_context`, and streams the `.bin` asset via `HTTPClient` in chunks

**Given** the binary is streaming
**When** each chunk is received
**Then** `Update.write(chunk, len)` writes to flash AND `mbedtls_sha256_update(&ctx, chunk, len)` feeds the hasher in the same loop iteration (rule 25 — incremental, never post-hoc), and `_progress` is updated as `(bytesWritten * 100) / totalSize`

**Given** all chunks have been written
**When** `mbedtls_sha256_finish()` produces the computed hash
**Then** the computed hash is compared against the expected hash from the `.sha256` file BEFORE calling `Update.end()` — if mismatch: `Update.abort()`, state = ERROR, "Firmware integrity check failed" (AR9)

**Given** SHA-256 verification passes
**When** `Update.end(true)` finalizes the partition
**Then** `esp_ota_set_boot_partition()` marks the new partition for boot, state = REBOOTING, `delay(500)`, `ESP.restart()` — total elapsed time under 60 seconds on 10Mbps WiFi (NFR5)

**Given** the device reboots into the new firmware
**When** the owner opens the dashboard
**Then** all user settings and configuration (WiFi credentials, API keys, night mode schedule, mode preferences, schedule rules) are preserved — NVS is on a separate partition from firmware (FR29, NFR15)

**Given** the download task is running
**When** `startDownload()` is called again
**Then** it returns `false` — only one download task at a time (`_downloadTask != nullptr` guard)

**Given** the download task completes (success or error)
**When** the task exits
**Then** `_downloadTask = nullptr` is set BEFORE `vTaskDelete(NULL)` to prevent dangling handle reads from the main thread

**Given** `ModeRegistry::prepareForOTA()` is implemented
**When** both Foundation Push (WebPortal OTA upload) and Delight Pull use it
**Then** both paths call `prepareForOTA()` before `Update.begin()` (AR10, rule 26) — the Foundation Push handler is updated to use the shared method if it doesn't already

### Story 17.2: OTA Failure Handling, Rollback, and Retry

As an owner,
I want firmware updates to fail safely — never bricking the device, always showing clear status, and allowing retry,
So that I can update with confidence knowing the worst case is a brief reboot back to the previous version.

**Acceptance Criteria:**

**Given** the WiFi connection drops during the binary download
**When** the HTTP stream fails
**Then** `Update.abort()` is called (rule 25), firmware is unchanged (inactive partition has partial write, never activated), state = ERROR, error = "Download failed — tap to retry" (FR31)

**Given** any HTTP error occurs during the download
**When** the download task handles the error
**Then** `Update.abort()` is called, state = ERROR with a specific error message — every error path after `Update.begin()` calls `Update.abort()` (rule 25)

**Given** the new firmware partition fails boot validation
**When** the bootloader detects corruption or a failed self-check
**Then** the bootloader automatically reverts to the previous partition (A/B scheme) — the device boots back to the previous working firmware (FR28, NFR12)

**Given** a firmware update has failed
**When** the dashboard displays the failure
**Then** the status message states: (a) the failure phase — download, verify, or boot; (b) the outcome — retriable (firmware unchanged) or rolled back to previous version; (c) the recommended next action — retry, reboot, or none (FR30)

**Given** the LED matrix is showing OTA progress
**When** a failure occurs
**Then** the LED matrix failure indication is distinct from the in-progress OTA state — the user can tell the difference between "updating" and "failed" (FR30)

**Given** a firmware update has failed with a retriable error
**When** the owner taps "Retry" on the dashboard
**Then** `OTAUpdater::startDownload()` is called again without requiring a device restart (FR37) — the device is still running the original firmware

**Given** OpenSky or AeroAPI is unavailable
**When** an OTA check or download is in progress or idle
**Then** flight display is unaffected — the OTA subsystem and flight data pipeline are independent (FR35, NFR18)

**Given** GitHub Releases API is unavailable
**When** the owner attempts an OTA check
**Then** the dashboard shows "Unable to check for updates — try again later", flight rendering and mode scheduling continue unaffected (FR35, NFR18, NFR19)

**Given** the device is in any state (normal operation, OTA in progress, OTA failed)
**When** any failure occurs
**Then** the LED matrix does not remain blank for more than 1 second except during the intentional OTA progress display, and the device does not enter a crash-loop (FR35, NFR19)

**Given** OTA_PULL does not yet exist as a SystemStatus subsystem
**When** this story is implemented
**Then** `SystemStatus.h` is updated with an `OTA_PULL` subsystem entry and `SystemStatus.cpp` registers it, enabling the health page to show OTA Pull status

### Story 17.3: OTA Pull Dashboard UI and Status Polling

As an owner,
I want a clear dashboard interface for downloading firmware updates with real-time progress feedback,
So that I can see exactly what's happening during the update and know when it's complete.

**Acceptance Criteria:**

**Given** the OTA check has found an available update (Epic 16, Story 16.2)
**When** the owner taps "Update Now"
**Then** `POST /api/ota/pull` is called, which validates `OTAUpdater::getState() == AVAILABLE`, calls `startDownload()`, and returns `{ ok: true, data: { started: true } }`

**Given** `POST /api/ota/pull` is called when state is not AVAILABLE
**When** the endpoint validates state
**Then** it returns `{ ok: false, error: "No update available — check for updates first" }` or `{ ok: false, error: "Download already in progress" }` as appropriate

**Given** a firmware download is in progress
**When** the dashboard polls `GET /api/ota/status` at 500ms intervals
**Then** the response returns `{ ok: true, data: { state: "downloading", progress: N } }` where N is 0-100, and the dashboard displays a progress bar matching the existing mode-switch polling pattern from Display System

**Given** the download completes and enters verification
**When** the dashboard polls status
**Then** the response shows `{ state: "verifying" }` and the progress bar indicates the verification phase

**Given** verification passes and reboot is imminent
**When** the dashboard polls status
**Then** the response shows `{ state: "rebooting" }` and the dashboard displays "Rebooting..." — the device will disconnect momentarily

**Given** the device has rebooted into new firmware
**When** the dashboard reconnects
**Then** the status bar reads "Running {new_version}" and the OTA section no longer shows an update banner — the update is complete

**Given** an error occurs during download or verification
**When** the dashboard polls status
**Then** the response shows `{ state: "error", error: "..." }` and the dashboard displays the error message with a "Retry" button (FR37)

**Given** the firmware binary size
**When** the build completes
**Then** the total firmware image including OTAUpdater code stays within 2,097,152 bytes (2 MiB OTA slot, NFR14)

**Given** this is the final story in the Delight Release
**When** the release is complete
**Then** the repository README links to the current setup and device-flash guides such that a competent hobbyist can flash Delight firmware and filesystem, complete first boot, and reach the dashboard using only published documents (FR43, Journey 0)
