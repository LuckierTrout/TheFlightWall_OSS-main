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

### Epic 1: First-Boot Setup — "Plug in, configure from phone, see flights"

User can power on the device, connect to its WiFi AP from a phone, complete a 5-step setup wizard (WiFi → API keys → location → hardware → review), and see live flights appear on the LED wall — no code editing, no laptop, no terminal. The device persists all settings in NVS, runs display and WiFi on separate cores, recovers from WiFi disconnections, and displays its IP address on the LEDs.

**FRs covered:** FR1, FR2, FR3, FR4, FR5, FR6, FR7, FR8, FR10, FR11, FR38, FR39, FR40, FR41, FR44
**Existing FRs migrated:** FR19, FR20, FR21, FR23, FR25 (ConfigManager migration — acceptance criteria on ConfigManager story)
**Note:** Largest and highest-risk epic. Stories sequence infrastructure build internally.

### Epic 2: Config Dashboard & Device Management — "Tweak settings from browser, manage the device"

User can browse to flightwall.local, view and edit all device settings from a card-based dashboard, see display changes reflected on the LED wall within 1 second via hot-reload, receive clear reboot feedback for network/API changes, adjust timing controls, reset the device to defaults, and force AP mode via GPIO button.

**FRs covered:** FR9, FR12, FR13, FR14, FR42, FR43, FR45, FR46
**Builds on:** Epic 1 (ConfigManager, WebPortal, WiFiManager)

### Epic 3: Enhanced Flight Display & Canvas Preview — "Logos, telemetry, responsive layout, and live preview"

LED wall shows airline logo in left zone, flight card with route info, and live telemetry (altitude, speed, track, vertical rate) in a zone-based layout that adapts automatically to any panel configuration. Browser shows a live canvas preview of the matrix layout that updates instantly as tile settings change, using the same zone calculation algorithm as the firmware.

**FRs covered:** FR17, FR18, FR22 (updated), FR24, FR26, FR27, FR28, FR29, FR30, FR31, FR32, FR33
**Builds on:** Epic 1 (ConfigManager)
**Independent of:** Epic 2 (can be developed in parallel)

### Epic 4: Advanced Dashboard Tools — "Map, calibration, and logo management"

User can configure the geographic capture area using an interactive map with draggable circle radius, calibrate display wiring with a live test pattern on the LEDs, upload airline logo files with client-side RGB565 preview, manage uploaded logos (view list, delete), and see estimated API usage when adjusting fetch interval.

**FRs covered:** FR15, FR16, FR34, FR35, FR36, FR37, FR47, FR48
**Builds on:** Epic 2 (dashboard) + Epic 3 (LayoutEngine, LogoManager)

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
