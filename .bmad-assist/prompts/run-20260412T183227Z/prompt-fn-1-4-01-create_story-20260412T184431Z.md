<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: fn-1.4 -->
<!-- Phase: create-story -->
<!-- Timestamp: 20260412T184431Z -->
<compiled-workflow>
<mission><![CDATA[

Create the next user story from epics+stories with enhanced context analysis and direct ready-for-dev marking

Target: Story fn-1.4 - ota-self-check-and-rollback-detection
Create comprehensive developer context and implementation-ready story.

]]></mission>
<context>
<file id="git-intel" path="[git-intelligence]"><![CDATA[<git-intelligence>
Git intelligence extracted at compile time. Do NOT run additional git commands - use this embedded data instead.

### Recent Commits (last 5)
```
898c09f Enhance AGENTS.md with updated BMAD workflow details, including project context handling and bmad-assist commands. Modify bmad-assist.yaml to adjust budget estimates for story creation and validation. Update state.yaml to reflect current epic and story status. Revise sprint-status.yaml to streamline development status and remove outdated comments. Update epics.md to reflect completed and new epics, and adjust firmware partition layout in custom_partitions.csv. Log firmware version at boot in main.cpp.
ffc1cce Update .gitignore to include .env file, enhance AGENTS.md with new user preferences and workspace facts, and modify continual learning state files. Adjust workflow configurations to specify story directories and improve error handling in API fetchers. Add positioning mode to NeoMatrixDisplay and update related documentation.
c5f944e Update AGENTS.md with information on bundled web sources for firmware portal. Modify continual learning state files to reflect the latest run details and add new transcript entries.
6d509a0 Update continual learning state and enhance NeoMatrixDisplay with zone-based rendering. Added telemetry fields and improved logo management. Marked several implementation artifacts as done, including dashboard layout preview and flight rendering features.
c55ee67 first commit
```

### Related Story Commits
```
(no output)
```

### Recently Modified Files (excluding docs/)
```
(no output)
```

</git-intelligence>]]></file>
<file id="ed7fe483" path="_bmad-output/project-context.md" label="PROJECT CONTEXT"><![CDATA[

---
project_name: TheFlightWall_OSS-main
date: '2026-04-12'
---

# Project Context for AI Agents

Lean rules for implementing FlightWall (ESP32 LED flight display + captive-portal web UI). Prefer existing patterns in `firmware/` over new abstractions.

## Technology Stack

- **Firmware:** C++11, ESP32 (Arduino/PlatformIO), FastLED + Adafruit GFX + FastLED NeoMatrix, ArduinoJson ^7.4.2.
- **Web on device:** ESPAsyncWebServer (**mathieucarbou fork**), AsyncTCP (**Carbou fork**), LittleFS (`board_build.filesystem = littlefs`), custom `custom_partitions.csv` (~2MB app + ~2MB LittleFS).
- **Dashboard assets:** Editable sources under `firmware/data-src/`; served bundles are **gzip** under `firmware/data/`. After editing a source file, regenerate the matching `.gz` from `firmware/` (e.g. `gzip -9 -c data-src/common.js > data/common.js.gz`).

## Critical Implementation Rules

- **Core pinning:** Display/task driving LEDs on **Core 0**; WiFi, HTTP server, and flight fetch pipeline on **Core 1** (FastLED + WiFi ISR constraints).
- **Config:** `ConfigManager` + NVS; debounce writes; atomic saves; use category getters; `POST /api/settings` JSON envelope `{ ok, data, error, code }` pattern for REST responses.
- **Heap / concurrency:** Cap concurrent web clients (~2–3); stream LittleFS reads; use ArduinoJson filter/streaming for large JSON; avoid full-file RAM buffering for uploads.
- **WiFi:** WiFiManager-style state machine (AP setup → STA → reconnect / AP fallback); mDNS `flightwall.local` in STA.
- **Structure:** Extend hexagonal layout — `firmware/core/`, `firmware/adapters/` (e.g. `WebPortal.cpp`), `firmware/interfaces/`, `firmware/models/`, `firmware/config/`, `firmware/utils/`.
- **Tooling:** Build from `firmware/` with `pio run`. On macOS serial: use `/dev/cu.*` (not `tty.*`); release serial monitor before upload.
- **Scope for code reviews:** Product code under `firmware/` and tests under `firmware/test/` and repo `tests/`; do not treat BMAD-only paths as product defects unless the task says so.

## Planning Artifacts

- Requirements and design: `_bmad-output/planning-artifacts/` (`architecture.md`, `epics.md`, PRDs).
- Stories and sprint line items: `_bmad-output/implementation-artifacts/` (e.g. `sprint-status.yaml`, per-story markdown).


]]></file>
<file id="b46b3d7a" path="_bmad-output/planning-artifacts/prd.md" label="PRD"><![CDATA[

---
stepsCompleted: ['step-01-init', 'step-02-discovery', 'step-02b-vision', 'step-02c-executive-summary', 'step-03-success', 'step-04-journeys', 'step-05-domain-skipped', 'step-06-innovation-skipped', 'step-07-project-type', 'step-08-scoping', 'step-09-functional', 'step-10-nonfunctional', 'step-11-polish', 'step-12-complete']
inputDocuments: ['docs/index.md', 'docs/project-overview.md', 'docs/architecture.md', 'docs/source-tree-analysis.md', 'docs/component-inventory.md', 'docs/development-guide.md', 'docs/api-contracts.md']
documentCounts:
  briefs: 0
  research: 0
  brainstorming: 0
  projectDocs: 7
workflowType: 'prd'
projectType: 'brownfield'
classification:
  projectType: iot_embedded
  domain: general
  complexity: low-medium
  projectContext: brownfield
lastEdited: '2026-04-02'
editHistory:
  - date: '2026-04-02'
    changes: 'Added Update Mechanism section, Journey 5 Recovery & Maintenance, updated traceability table. Fixed subjective language and implementation leakage in FRs/NFRs.'
  - date: '2026-04-02'
    changes: 'Added FR47 (interactive map with circle radius), FR48 (API usage estimate on fetch interval), updated FR5 (geolocation prompt), added Web UI Dependencies section (Leaflet.js), updated Journey Requirements Summary table.'
---

# Product Requirements Document - TheFlightWall

**Author:** Christian
**Date:** 2026-04-02

## Executive Summary

TheFlightWall is an ESP32-powered LED wall that displays live flight information for aircraft passing near your location. The existing firmware fetches ADS-B position data from OpenSky Network, enriches it with airline, aircraft, and route metadata from FlightAware AeroAPI and a CDN lookup service, and renders flight cards on a WS2812B NeoPixel LED matrix.

This PRD defines two major enhancements that transform TheFlightWall from a developer-oriented maker project into a self-contained, configurable product:

1. **Web Configuration Portal** — An ESP32-hosted web interface with captive portal setup wizard and local network config UI. All settings (WiFi, API keys, location, display hardware, timing) move from compile-time C++ headers to persistent NVS storage, editable from any browser. Includes a live HTML5 canvas preview of the matrix layout and a display calibration tool.

2. **Enhanced Display with Airline Logos** — A responsive zone-based display layout supporting airline logo bitmaps (32x32 RGB565), a three-line flight card (airline, route, aircraft), and live telemetry (altitude, speed, track, vertical rate). The layout adapts dynamically to any panel configuration, from compact single-row setups to expanded multi-row arrangements.

The target user is the project owner (personal project). The goal is a device you plug in, configure from your phone, and mount on the wall — no PlatformIO, no code editing, no reflashing required.

## What Makes This Special

The product is not just the LED display — it's the unified device-plus-web-UI experience. The display is the output; the web interface is the product surface. Together they turn custom hardware into something anyone with the components can set up and customize without writing a line of code. The live browser preview of the matrix layout means configuration is visual and immediate, not trial-and-error.

## Project Classification

- **Project Type:** IoT/Embedded — ESP32 device with WiFi connectivity, external API integrations, LED matrix display, and browser-based companion interface
- **Domain:** General (aviation data consumer) — no regulatory, safety, or compliance requirements
- **Complexity:** Low-Medium — hardware/firmware with multiple external APIs and a web UI component, but single-user personal project with no multi-tenancy, auth, or scale concerns
- **Project Context:** Brownfield — enhancing existing working firmware with hexagonal (Ports & Adapters) architecture

## Success Criteria

### User Success

- First-boot setup: connect to ESP32 AP from phone, complete setup wizard (WiFi, API keys, location, hardware), and see live flights on display — no code editing or reflashing
- Change any setting from the web UI and see it reflected on the LED wall
- Display-only settings (brightness, color, layout) apply within 1 second (hot-reload). Network/API settings require reboot with clear user feedback ("Rebooting to apply changes...")
- Display shows airline logo, flight card, and live telemetry matching the reference layout
- Swap to a different panel arrangement, reconfigure tile count in web UI, and the layout adapts automatically

### Technical Success

- All configuration persisted in NVS — survives power cycles
- Captive portal AP mode works on first boot and when saved WiFi is unavailable
- Existing fetch → enrich → display pipeline works identically after refactor (regression baseline)
- Responsive layout renders correctly across different matrix dimensions (compact/full/expanded breakpoints)
- Logo bitmaps load from LittleFS by ICAO code with fallback airplane sprite
- Live preview canvas accurately represents actual LED matrix layout and zone positions
- Firmware + web assets (HTML/JS/CSS) + bundled logos fit within 4MB flash with at least 500KB headroom

### Measurable Outcomes

- **Post-Epic 1 smoke test:** Flash firmware → ESP32 broadcasts AP → connect from phone → complete setup wizard → ESP32 joins home WiFi → flights appear on display
- **Post-Epic 2 smoke test:** Same flow, but display shows logo + flight card + telemetry in new layout. Verified with at least two tile configurations (e.g., 10x2 and 5x2)

## Product Scope & Phased Development

### MVP Strategy

**Approach:** Problem-solving MVP — deliver the two core capabilities (web config + enhanced display) that transform the project from "editable source code" to "configurable product."

**Resource Requirements:** Solo developer (Christian) with AI-assisted development. No team dependencies.

### MVP Feature Set (Phase 1)

**Core User Journeys Supported:**
- Journey 1 (First-Time Setup) — full captive portal → config → live flights
- Journey 2 (Tweaking Settings) — config UI with hot-reload and reboot flow
- Journey 3 (New Panel Layout) — responsive layout + calibration + live preview
- Journey 4 (Adding Logos) — logo upload + LittleFS management

**Must-Have Capabilities (Epic 1 — Web Config Portal):**
- ConfigManager: NVS read/write with typed getters and compile-time defaults as fallback
- WebPortal: ESPAsyncWebServer + LittleFS serving HTML/JS/CSS
- Captive portal: AP mode setup wizard (WiFi → API keys → location → hardware → display calibration)
- Config UI: Full settings dashboard on local network, organized by section
- Logo management: Upload page for .bin files to LittleFS
- Live preview: HTML5 canvas showing matrix layout, zones, and calibration in real-time
- Apply flow: Hot-reload for display settings (<1s), reboot with feedback for network/API changes

**Must-Have Capabilities (Epic 2 — Enhanced Display):**
- Responsive layout engine: Zone-based with breakpoints (compact 16px / full 32px+ / expanded 48px+)
- LogoManager: LittleFS lookup by ICAO code, fallback airplane sprite in PROGMEM
- New display layout: Logo panel (square, matrix height) + flight card (airline, route, aircraft) + telemetry
- Telemetry rendering: Altitude (kft), speed (mph), track (deg), vertical rate (ft/s) with unit conversions
- FreeRTOS task pinning: Display on Core 0, WiFi on Core 1

### Phase 2 — Growth (Post-MVP)

- OTA firmware updates via web UI (requires partition table change + one-time reflash)
- Remote logo CDN fetch + LittleFS caching
- Bluetooth enablement when a use case is identified
- ArduinoJson streaming parser optimization

### Phase 3 — Expansion (Future)

- Multiple display profiles / themes
- Flight history and statistics tracking
- Community-shared logo packs
- Additional data source integrations

## User Journeys

### Journey 1: First-Time Setup — "Unboxing Day"

Christian just finished soldering the last panel, wiring the ESP32, and loading the firmware. He powers it on for the first time. The display flashes "FlightWall" and then broadcasts a WiFi network: `FlightWall-Setup`.

He pulls out his phone, connects to the AP, and a browser automatically opens the setup wizard. Step one: enter home WiFi credentials. Step two: paste in OpenSky and AeroAPI keys. Step three: type in his latitude, longitude, and a 10km radius. Step four: confirm the tile layout (10x2 at 16x16) and see the live preview canvas show the zone layout — logo area on the left, flight card top-right, telemetry bottom.

He hits "Save & Connect." The display shows "Connecting to WiFi..." then "WiFi OK" and switches to the loading screen. Thirty seconds later, the first flight card appears — a Southwest 737 descending into the nearby airport, complete with logo, route, and altitude readout.

**The moment:** It works. From bare hardware to live flight data, entirely from his phone. No laptop, no IDE, no terminal.

**Reveals requirements for:** Captive portal, setup wizard flow, NVS persistence, WiFi STA transition, first-fetch display

### Journey 2: Tweaking Settings — "Saturday Afternoon Customization"

Christian is sitting on the couch looking at the FlightWall. The text is a bit bright for the evening. He opens his phone browser, types in the ESP32's local IP, and lands on the config dashboard. He drops the brightness from 50 to 15, changes the text color from white to amber. The LED wall updates instantly — no reboot, no waiting for the next fetch cycle.

He also decides to widen his search radius from 10km to 25km to catch more flights. He updates the location setting and hits "Apply." The page shows "Rebooting to apply network changes..." and the wall briefly goes dark, then comes back with flights from a wider area.

**The moment:** The feedback loop is instant for visual changes and transparent for deeper changes. It feels like controlling a smart home device, not reprogramming firmware.

**Reveals requirements for:** Config UI sections, hot-reload for display settings, reboot flow for API/network settings, clear user feedback

### Journey 3: New Panel Layout — "The Upgrade"

Christian bought more LED panels and expanded from 10x2 to 12x3. He opens the config UI, changes Tiles X to 12 and Tiles Y to 3. The live preview canvas immediately shows the new 192x48 resolution with the expanded layout — the logo area is now 48x48, more room for the flight card, and the telemetry section has space for additional data.

He checks the layout mode indicator: "Expanded (48px+ height)." The wiring flags need adjusting for the new panels — he toggles the tile origin and scan direction using the calibration dropdowns while a test pattern renders on the actual LEDs until it looks correct.

He saves, the display reinitializes, and the next flight card fills the larger display beautifully.

**The moment:** The hardware changed but the software adapted — no code changes, just a few clicks.

**Reveals requirements for:** Responsive layout engine, breakpoint detection, live preview canvas with real-time updates, display calibration tool, hot-reload for hardware config

### Journey 4: Adding Airline Logos — "Logo Day"

Christian has prepared a batch of airline logo `.bin` files (32x32 RGB565) for the top 30 airlines in his area. He opens the config UI, navigates to the Logo Management section, and drags the files onto the upload area. A progress indicator shows them uploading to LittleFS. The page confirms: "28 logos uploaded, 2 failed (file too large)."

He fixes the two oversized files, re-uploads, and all 30 are now stored. The next time a United flight appears, the blue globe logo renders in the left panel instead of the generic airplane icon.

**The moment:** The display transforms from functional to polished — real airline branding on a homemade LED wall.

**Reveals requirements for:** Logo upload UI, LittleFS file management, file validation (size/format), LogoManager ICAO lookup, fallback sprite

### Journey 5: Recovery & Maintenance — "Something's Wrong"

Christian notices the FlightWall hasn't updated in a while. He grabs his phone to check — but he can't remember the ESP32's IP address. He glances at the LED wall and sees the IP displayed briefly after the last reboot: `192.168.1.47`. He types it into his browser and the config dashboard loads. The WiFi section shows the device is connected, but the API status shows repeated failures — his AeroAPI key expired.

He pastes in the new key, hits Apply, and the device reboots. Flights reappear within a minute.

A month later, Christian has been experimenting with settings and wants a clean slate. He opens the config dashboard, scrolls to the bottom, and hits "Reset to Defaults." The page confirms: "All settings will be erased. Device will restart in setup mode." He confirms, and the FlightWall reboots into AP mode, broadcasting `FlightWall-Setup` again — ready for a fresh configuration.

One day the WiFi router is replaced and the FlightWall can't connect. It retries for a while, then automatically falls back to AP mode. But Christian doesn't want to wait — he holds the reset button on the ESP32 during boot, forcing it into setup mode immediately. He connects from his phone, enters the new WiFi credentials, and the wall is back online.

**The moment:** The device recovers from problems without needing a laptop, USB cable, or reflash. Every recovery path works from a phone or the device itself.

**Reveals requirements for:** IP address display on LED matrix, factory reset via web UI, GPIO button forced AP mode, automatic AP fallback on WiFi failure

### Journey Requirements Summary

| Capability | J1 Setup | J2 Tweak | J3 Layout | J4 Logos | J5 Recovery |
|-----------|:--------:|:--------:|:---------:|:--------:|:-----------:|
| Captive portal AP mode | x | | | | x |
| Setup wizard flow | x | | | | |
| NVS config persistence | x | x | x | | |
| WiFi STA connection | x | | | | |
| Config UI dashboard | | x | x | x | x |
| Hot-reload display settings | | x | x | | |
| Reboot flow with feedback | | x | | | x |
| Live preview canvas | | | x | | |
| Display calibration tool | | | x | | |
| Responsive layout engine | | | x | | |
| Logo upload + LittleFS mgmt | | | | x | |
| LogoManager + fallback sprite | | | | x | |
| Telemetry rendering | x | | x | | |
| Interactive map + circle radius | | x | x | | |
| API usage estimate on fetch interval | | x | | | |
| IP address display on matrix | | | | | x |
| Factory reset via web UI | | | | | x |
| GPIO button forced AP mode | | | | | x |

## Functional Requirements

### Device Setup & Onboarding

- **FR1:** User can complete initial device configuration entirely from a mobile browser without editing code or using a command line
- **FR2:** Device can broadcast its own WiFi access point when no WiFi credentials are saved or when saved WiFi is unreachable
- **FR3:** User can enter WiFi credentials through a captive portal setup wizard
- **FR4:** User can enter API credentials (OpenSky client ID/secret, AeroAPI key) through the setup wizard
- **FR5:** User can set geographic location (latitude, longitude, radius) through the setup wizard using text input with an optional browser geolocation prompt ("Use my location")
- **FR6:** User can configure hardware tile layout (tile count X/Y, pixel dimensions, data pin) through the setup wizard
- **FR7:** Device can transition from AP mode to STA mode and connect to the configured home WiFi network
- **FR8:** Device can display connection status messages on the LED matrix during setup and transitions

### Configuration Management

- **FR9:** User can view and edit all device settings from a web dashboard accessible on the local network
- **FR10:** Device can persist all configuration to non-volatile storage that survives power cycles
- **FR11:** Device can fall back to compile-time default values when no saved configuration exists
- **FR12:** User can modify display settings (brightness, text color RGB) and see changes reflected on the LED matrix within 1 second
- **FR13:** User can modify network/API settings and receive a status message ("Rebooting to apply changes...") indicating a reboot is required
- **FR14:** Device can reboot and apply configuration changes automatically after user confirms

### Display Calibration & Preview

- **FR15:** User can configure matrix wiring flags (origin corner, scan direction, zigzag/progressive) for both single tiles and multi-tile arrangements through the web UI
- **FR16:** Device can render a test pattern on the LED matrix that updates in real-time as calibration settings are changed
- **FR17:** User can view a live HTML5 canvas preview in the browser that accurately represents the matrix dimensions, zone layout, and calibration state
- **FR18:** Preview canvas can update in real-time as tile count, dimensions, or wiring flags are changed

### Flight Data Display

- **FR19:** Device can fetch ADS-B state vectors from OpenSky Network within a configurable geographic radius
- **FR20:** Device can enrich flight data with airline, aircraft, and route metadata from FlightAware AeroAPI
- **FR21:** Device can resolve human-friendly airline and aircraft display names from the FlightWall CDN
- **FR22:** Device can render a flight card showing airline name, route (origin > destination), and aircraft type
- **FR23:** Device can cycle through multiple active flights at a configurable interval
- **FR24:** Device can display live telemetry: altitude (kft), speed (mph), track (degrees), and vertical rate (ft/s)
- **FR25:** Device can display a loading/waiting screen when no flights are currently in range

### Airline Logo Display

- **FR26:** Device can render a 32x32 pixel airline logo bitmap in the left zone of the display
- **FR27:** Device can look up airline logos by ICAO operator code from local storage
- **FR28:** Device can display a generic airplane fallback icon when no matching airline logo is found
- **FR29:** Logo zone size can scale proportionally based on matrix height

### Responsive Display Layout

- **FR30:** Device can calculate display zones (logo, flight card, telemetry) dynamically based on matrix dimensions
- **FR31:** Device can select a layout mode (compact, full, expanded) based on matrix height breakpoints
- **FR32:** Device can render the appropriate layout for the detected matrix dimensions without code changes
- **FR33:** Display layout can adapt when tile configuration is changed via the web UI

### Logo Management

- **FR34:** User can upload airline logo bitmap files (.bin, 32x32 RGB565) to the device through the web UI
- **FR35:** User can view a list of uploaded logos on the device
- **FR36:** Device can validate uploaded logo files for correct size and format
- **FR37:** User can delete uploaded logos from the device through the web UI

### System Operations

- **FR38:** Device can run LED display updates without blocking or being blocked by WiFi and web server operations
- **FR39:** Device can serve the web configuration UI while simultaneously running the flight data pipeline without either blocking the other
- **FR40:** Device can automatically maintain valid API authentication for OpenSky Network before credentials expire
- **FR41:** Device can detect WiFi disconnections and API failures, continue displaying last known flight data, and automatically retry connections at the configured fetch interval without requiring manual restart
- **FR42:** User can reconfigure hardware tile layout, display pin, and wiring flags from the config dashboard after initial setup
- **FR43:** User can adjust timing settings (fetch interval, display cycle duration) from the web UI
- **FR47:** User can view and adjust the geographic capture area using an interactive map with a draggable circle radius overlay in the config dashboard
- **FR48:** User can see estimated monthly API usage when adjusting the fetch interval, updated in real-time as the setting changes
- **FR44:** Device can display its local IP address on the LED matrix after connecting to WiFi
- **FR45:** User can reset all settings to defaults from the web UI, returning the device to AP setup mode
- **FR46:** User can force the device into AP setup mode by holding a designated GPIO button during boot

## IoT/Embedded Technical Requirements

### Hardware Reference

| Component | Specification | Notes |
|-----------|--------------|-------|
| MCU | ESP-WROOM-32, 4MB flash, 520KB SRAM | Dual-core Tensilica LX6 @ 240MHz |
| LED Panels | BTF-LIGHTING WS2812B ECO 16x16 | 5050SMD, GRB color order, DC5V, alloy wires |
| PSU | Aclorol 5V 20A (100W) | AC input via NEMA 5-15P 18AWG cord |
| Level Shifter | HiLetgo 4-channel 3.3V-5V bi-directional | ESP32 GPIO → WS2812B data line |
| Data Pin | GPIO 25 (configurable) | Single data line to daisy-chained panels |

### Memory Budget

- **RAM:** ~170-240KB estimated usage, ~280-350KB headroom. Heap fragmentation is the primary risk — monitor with `ESP.getFreeHeap()` and `ESP.getMaxAllocHeap()`
- **Flash partition (MVP, no OTA):** ~2MB firmware, ~2MB LittleFS (web assets + logos)
- **Flash partition (post-MVP with OTA):** ~1.5MB firmware, ~1.5MB OTA, ~896KB LittleFS — requires custom partition table
- Limit concurrent web server clients to 2-3 to manage RAM pressure
- Stream files from LittleFS; do not load full files into RAM
- Consider ArduinoJson streaming/filter API to reduce heap pressure from 16KB → ~2-4KB per parse

### Critical Implementation Constraints

1. **FastLED + WiFi interrupt conflict (HARD REQUIREMENT):** FastLED disables interrupts during pixel output, disrupting the WiFi/async web server stack. The display update task **must** be pinned to Core 0 via `xTaskCreatePinnedToCore()`, keeping WiFi on Core 1. Alternatively, use the RMT/I2S peripheral driver for DMA-based interrupt-free LED output.
2. **Heap concurrency:** API fetch operations and web server file serving may compete for heap simultaneously. Acceptable for single-user use — document as known behavior.

### Web UI Dependencies

- **Leaflet.js** (~40KB gzipped) — Interactive map for location configuration with circle radius overlay. Loaded from CDN or bundled in LittleFS. Map tiles fetched at runtime from OpenStreetMap tile servers (requires internet — only used in config dashboard, not captive portal)
- No frontend frameworks — vanilla HTML/JS/CSS with a minimal reusable stylesheet for consistent spacing, typography, and color

### Connectivity

- **WiFi STA:** Primary mode — connects to home network for API calls and serves config UI
- **WiFi AP:** Setup mode — captive portal for first-boot configuration and WiFi fallback
- **Bluetooth:** Capability reserved, no defined use case. Compiled disabled for MVP (`CONFIG_BT_ENABLED 0`) to reclaim ~60KB RAM/flash. Enable when a use case emerges.

### Power Profile

- Full white at max brightness: ~192W theoretical (3200 pixels × 60mA) — exceeds PSU
- Typical operation at brightness 5-50/255: ~3-10W — well within 100W budget
- No battery operation; always wall-powered

### Security Model

- Local network only — no internet-facing services
- No authentication on web config UI (single-user, trusted network)
- API credentials stored in NVS (improvement over current compile-time headers)
- TLS for external API calls (currently using `setInsecure()` — acceptable for personal use)

### Update Mechanism

- **MVP (Phase 1):** USB flash via PlatformIO — connect ESP32 to computer, build and upload firmware. All configuration persists in NVS across reflashes; uploaded logos persist in LittleFS across reflashes
- **Post-MVP (Phase 2):** OTA firmware updates via web UI — requires custom partition table (1.5MB app + 1.5MB OTA + 896KB LittleFS) and one-time full reflash to apply new partition layout. After repartitioning, logos must be re-uploaded and NVS config re-entered
- **No automatic update checks** — user-initiated only. Device does not phone home or check for updates

### Known Trade-offs

- **MVP partition layout:** Simple 2MB app + 2MB LittleFS. Adding OTA later requires repartitioning and one-time full reflash — uploaded logos and NVS config must be re-entered. Conscious decision to maximize LittleFS space for MVP.

## Non-Functional Requirements

### Performance

- Display settings changes (brightness, color) must be reflected on the LED matrix within 1 second of submission
- Web config UI pages must load within 3 seconds on the local network
- Flight data fetch cycle must complete (API call + JSON parse + display update) within the configured fetch interval
- LED matrix frame rendering must maintain a consistent frame rate with no dropped frames during flight card transitions
- Captive portal setup wizard must be responsive — each page transition under 2 seconds

### Reliability

- Device must operate continuously without requiring manual restart (target: 30+ days of uninterrupted operation)
- Device must recover automatically from transient WiFi disconnections within 60 seconds of network availability without user intervention
- Device must recover from API failures (rate limits, timeouts, auth errors) by continuing to display last known data and retrying at the next fetch interval rather than showing error screens
- Configuration writes must be atomic — power loss during a config save must not corrupt stored settings
- Device must return to AP setup mode if stored WiFi network becomes permanently unreachable (after configurable retry timeout)

### Integration

- OpenSky Network API: Authenticated access with automatic credential refresh, rate-limited to stay within 4,000 monthly requests on free tier
- FlightAware AeroAPI: API key authentication, handle HTTP 429 (rate limit) and 5xx (server error) by retrying at next fetch interval
- FlightWall CDN: Public HTTPS GET for airline/aircraft name lookup, fail silently if unavailable (display falls back to ICAO codes)
- All external API calls use HTTPS (TLS)

### Resource Constraints

- Total firmware + web assets + logos must fit within 4MB flash with at least 500KB headroom
- Runtime RAM usage must stay below 280KB to maintain stable heap for concurrent web server + API operations
- Device filesystem operations must stream data — no full-file RAM buffering for uploads or logo reads
- Web server limited to 2-3 concurrent client connections to prevent heap exhaustion

## Risk Mitigation Strategy

| Risk | Impact | Mitigation |
|------|--------|------------|
| Heap fragmentation from ESPAsyncWebServer + TLS + JSON | Display glitches or crashes | Limit concurrent clients to 2-3, stream from LittleFS, monitor free heap |
| FastLED interrupt conflict with WiFi stack | Web server instability, WiFi drops | Hard requirement: pin display task to Core 0 |
| 4MB flash budget exceeded | Can't fit firmware + web assets + logos | Monitor partition usage, gzip web assets, cap logo count |
| NVS corruption on power loss during write | Config lost, device stuck | Use NVS commit transactions, fallback to compile-time defaults |
| Solo developer blocked on technical issue | Progress stalls | AI-assisted debugging, ESP32/FastLED community forums |


]]></file>
<file id="02546960" path="_bmad-output/planning-artifacts/ux-design-specification-delight.md" label="UX DESIGN"><![CDATA[

---
stepsCompleted: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14]
inputDocuments: ['_bmad-output/planning-artifacts/prd-delight.md', '_bmad-output/planning-artifacts/ux-design-specification-foundation.md', '_bmad-output/planning-artifacts/ux-design-specification-display-system.md', 'docs/architecture.md', 'docs/index.md']
---

# UX Design Specification — TheFlightWall Delight Release

**Author:** Christian
**Date:** 2026-04-11

---

## Executive Summary

### Project Vision

The Delight Release transforms TheFlightWall from a display platform into an ambient intelligence device — the wall reads the room. Building on the Foundation Release's configuration backbone and the Display System Release's pluggable mode architecture, six capabilities converge to create a wall that never looks dead, adapts to time of day, transitions gracefully between states, and updates its own firmware from GitHub.

The UX challenge unique to this release is managing three distinct interaction cadences within one dashboard: configure-once settings (Foundation), remote-control mode switching (Display System), and set-and-forget automation (Delight scheduling). Each cadence has different emotional registers, different visit frequencies, and different success metrics. The design must accommodate all three without any single cadence feeling bolted-on.

Two new UX surfaces ship: the web dashboard gains scheduling UI, mode-specific settings, and an OTA Pull experience; the LED matrix gains Clock Mode, Departures Board rendering, and animated transitions between all modes.

### Target Users

**Primary: Christian (project owner)** — Has lived with the wall through three releases. Knows the system deeply but wants it to run unattended. The Delight Release's success is measured in visit *purpose*, not visit frequency — Christian should interact with the dashboard to enhance the experience (tweaking schedules, demoing modes for friends, checking release notes) rather than to fix it (blank screens, stale data, missed updates). Phone-first, couch interaction.

**Secondary: Casey (self-service hobbyist)** — First encounter with TheFlightWall. Follows published guides to flash Delight firmware. Casey's first 10 minutes define the experience: find the dashboard via mDNS or LAN IP, complete the setup wizard (if new device), see Mode Picker with an active mode, and optionally discover scheduling as a "level up." The dashboard must make sense on first visit — Mode Picker and active mode immediately, scheduling and OTA discoverable but not mandatory. The Delight UX must not regress the Foundation/Display System onboarding flow.

**Tertiary: Sam (non-technical guest)** — Never touches the dashboard. Experiences the wall passively. Success means three things: (1) something is always displayed — Clock Mode fills dead air when no flights are overhead; (2) changes feel intentional, not glitchy — transitions reduce the "did it crash?" perception; (3) the wall conveys a finished product, not a prototype. Sam's reaction should be "where do I get one?" not "is it broken?" Note: reliability edge cases (WiFi drops, API failures) are system concerns addressed in NFRs, not UX deliverables — but the UX must degrade gracefully when they occur.

**Quaternary: Jordan (contributor)** — C++ hobbyist who adds a custom DisplayMode. UX touchpoint is indirect: the Mode Picker auto-discovers registered modes, scheduling includes custom modes, transitions work automatically. Jordan's UX is the architecture docs + the fact that it just works. Note: this persona is validated when at least one external contributor successfully adds a mode using only published documentation.

### Key Design Challenges

1. **Three interaction cadences in one dashboard** — Configure-once (Foundation cards), remote-control (Mode Picker), and set-and-forget (scheduling) coexist in the same dashboard. The scheduling UI must feel distinct from Mode Picker's instant-action feel — schedules are about the future, not the present. A fourth implicit cadence — first-time discovery — also exists: Casey's Journey 0 ends at "pick a mode and see it work," with scheduling and transitions as progressive discoveries. The dashboard's information architecture must make these cadence boundaries intuitive without requiring a tutorial.

2. **OTA Pull vs. OTA Upload mental model shift** — Foundation's OTA was "I compiled this, I upload it." Delight's OTA is "a version exists on GitHub, I install it." The interaction shifts from file management to software update management — version comparison, release notes, progress through phases the user doesn't control (download, verify, reboot). The Firmware card must evolve from upload zone to update center. Trust is the critical emotion: Christian is handing control to the device for 30-60 seconds during download, and the ESP32 may be unresponsive to dashboard requests during this window. The UX must set expectations, bridge the gap, and provide clear recovery paths for every failure mode.

3. **Temporal configuration without live feedback** — Scheduling asks users to reason about future time ranges ("9am-5pm show Departures, evenings Clock") while configuring a device they're not standing in front of. Unlike Mode Picker's instant visual feedback, schedule changes won't be testable until the configured time arrives — potentially hours or days later. The UI must help users visualize their schedule's behavior and validate correctness without waiting. Additionally, when schedule and idle fallback conflict (schedule says Clock, but flights are overhead), the schedule wins — but this priority hierarchy isn't obvious without encountering it. The dashboard should show both "scheduled mode" and "effective mode" with reason when they differ. NTP sync dependency (ESP32 has no RTC) adds a prerequisite: scheduling only works when time is synced, and the UI must surface sync status transparently.

4. **Transition configuration without in-dashboard preview** — Mode transitions (fade, potentially wipe/scroll in Growth) happen on the physical LED matrix, not in the browser. There's no way to preview "slow fade vs. quick cut" without watching the actual wall change modes. The transition settings UI must help users understand their options through descriptive labels, speed indicators, and explicit "watch the wall" guidance — setting expectations for an experience that can only be validated by looking up from the phone.

5. **Adding depth to the Mode Picker without breaking its simplicity** — The Display System's Mode Picker was deliberately minimal: see mode, tap activate, wall changes. Now each mode gets settings (12h/24h for Clock, row count for Departures Board, telemetry field selection for Live Flight Card), and a scheduling tab adds temporal configuration. The risk isn't just UI clutter — it's breaking the "one tap = wall changes" immediacy by introducing forms that require save/apply. Progressive disclosure must add capability without losing the "remote control" directness.

6. **Feature discoverability in a progressively enhanced dashboard** — Delight adds three opt-in ambient systems (idle fallback, scheduling, transitions) that won't be visible in Casey's "I just flashed it" state. Unlike Mode Picker's center-stage prominence, these are capabilities users must discover exist before they can configure them. Clock Mode's idle fallback is automatic and invisible until the user notices it. Scheduling and transitions require explicit configuration. The dashboard must surface these features — potentially through in-context hints, a "what's new" banner on first post-update load, or progressive disclosure within Mode Picker cards — without overwhelming the initial "pick a mode, see it work" goal.

7. **OTA Pull dashboard behavior during download** — During OTA download (up to 60 seconds on slower connections), the ESP32 tears down the active DisplayMode to free heap and streams the firmware binary to the inactive partition. The web server may be unresponsive during this window. The dashboard must handle this gracefully: client-side progress estimation (not real-time streaming), clear messaging that the display will go blank, timeout handling if the device doesn't respond after the expected window, and distinct states for download, verification, reboot, and reconnection. The LittleFS budget (~896KB total) constrains how rich the OTA UI can be — progress states must be communicated with minimal additional HTML/JS.

### Design Opportunities

1. **"The wall reads the room" narrative** — Clock idle fallback + scheduling + transitions create genuine ambient intelligence. The design goal is to create the perception that the wall adapts to context — filling dead air when skies are quiet, showing flights when they appear, dimming at night, switching modes at the configured times. The UX should frame scheduling as "teaching the wall your preferences" not "programming a timer." The emotional register is cozy automation, not cron job configuration. The delight moment is when Christian walks away from the dashboard and the wall just works — schedule triggers, idle fallback activates, transitions smooth the changes — and the experience feels earned ("I taught it") not opaque ("it's making decisions without me").

2. **OTA Pull as the consumer-grade moment** — This is when TheFlightWall stops requiring PlatformIO for updates. The trust earned by Foundation's OTA upload compounds here, but the improvement is a 10x reduction in time-to-value: Foundation OTA required compile + manual upload (20+ minutes); Delight OTA is "see new version, tap install, done" (2 minutes). Release notes + one-tap install + automatic rollback mirrors the iOS/Android update experience. Bulletproof error handling is essential: plain-language messages for "GitHub unreachable," "download failed," "firmware rejected (integrity check)," and "rolled back to previous version" — never error codes.

3. **Progressive disclosure ladder from novice to power user** — Foundation established Mode Picker as the "remote control" — immediate, visual, low-commitment. Delight adds depth: per-mode settings (one level deeper), scheduling (autonomous behavior), transitions (ambient polish). The opportunity is creating a ladder where Casey's Journey 0 ends at "pick a mode and go," Christian's Journey 1 explores per-mode tuning and scheduling, and power users discover the full automation stack — all without the initial simplicity feeling like a "lite" version. Each rung of the ladder should feel like a natural discovery, not a hidden advanced panel.

4. **The maker-to-appliance emotional arc** — Journey 0 (Casey flashes firmware) through Journey 1 (Christian configures scheduling) represents a progression from "I assembled a kit" to "I have ambient automation." The UX should honor both moments: the initial maker pride of "I flashed an ESP32 and it works" AND the later domestic integration of "my wall knows my routine." Scheduling UI copy, OTA update framing, and dashboard polish should bridge DIY authenticity with consumer-grade refinement — the wall grew up, and the UX should reflect that maturity.

## Core User Experience

### Defining Experience

The Delight Release has two defining interactions, each serving a different user and a different emotional register:

**1. The wall runs itself.** Christian configures a mode schedule ("Departures Board 6am-11pm, Clock overnight"), sets per-mode preferences (12h clock, 4-row departures board), and walks away. Over the following weeks, the wall transitions between modes at the right times, falls back to Clock when skies are quiet, and dims at night. Christian doesn't open the dashboard. The wall earned its autonomy. This is the ambient intelligence payoff — the culmination of three releases of infrastructure.

**2. One-tap firmware update.** A banner appears on the dashboard: "v2.4.1 available." Christian taps "View Release Notes," skims the changelog, taps "Update Now." The dashboard warns "Installing — device unreachable for ~60 seconds." The wall goes dark. The dashboard shows a client-side countdown and polls for reconnection. Flights resume on the new version. No USB cable, no PlatformIO, no git pull. The FlightWall became a consumer-grade updatable device.

The supporting experiences — mode-specific settings, Departures Board live rendering, smooth transitions — are high-value but exist in service of these two defining moments. If scheduling works and OTA works, the Delight Release delivered its promise.

A third, quieter defining moment emerges over time: **the first time Christian doesn't check if a scheduled transition worked.** Week one, he watches the wall at 11pm to confirm the schedule fires. Week two, he glances. Week three, he forgets to check and only notices the next morning that the clock was showing. This is when trust converts to autonomy — the wall has earned the right to run unattended.

### Platform Strategy

| Surface | Delight Release Changes | Context |
|---------|------------------------|---------|
| Dashboard | New Scheduling tab in Mode Picker, per-mode settings panels, evolved Firmware card (OTA Pull replacing upload), "what's new" feature discovery | Local network, phone or laptop browser |
| LED Matrix | Clock Mode rendering, Departures Board (multi-row flight list), animated fade transitions between all modes, OTA progress display | Physical hardware, viewed from 2-10 meters |
| Mode Switch API | Unchanged from Display System — GET modes + active, POST activate | Consumed by Mode Picker UI |
| Schedule API | New — GET/POST schedule rules, GET effective mode + reason | Consumed by Scheduling tab |
| OTA API | New — GET version check (GitHub Releases), POST trigger update, GET /api/ota/status (post-reboot result) | Consumed by Firmware card |
| Mode Status API | New — GET /api/mode/status returns active_mode, scheduled_mode, reason, ntp_synced, next_change | Consumed by dashboard status indicators |

**Constraints carried forward:**
- All web assets from ESP32 LittleFS (~896KB budget with dual-OTA partition table)
- No frontend frameworks — vanilla HTML/JS/CSS with the existing design system
- ESPAsyncWebServer connection limits — no auto-polling, no WebSocket
- Same CSS custom properties, card layout, toast notification, and button patterns
- Single-user dashboard assumption (home network, ~4 concurrent connection limit)

**New constraints:**
- NTP sync required for scheduling — ESP32 has no RTC; UI must surface sync status and degrade gracefully when unavailable. Schedule rules save but show "Waiting for time sync" status until NTP succeeds.
- OTA Pull creates a 30-60s window where ESP32 is downloading firmware and the web server is completely blocked — dashboard must handle this entirely client-side with countdown + polling after expected completion
- LittleFS budget for new Delight UI: ~10-12KB additional gzipped web assets (scheduling timeline, OTA Pull UI, mode settings panels). Current total usage is ~9KB; 896KB capacity provides ample headroom.
- "Effective mode" vs. "scheduled mode" may differ — new /api/mode/status endpoint must communicate both with human-readable reason
- Visual timeline component requires JavaScript (CSS Grid base + JS overlays for rule segments and time marker) — no charting library, vanilla JS only

**Dashboard section ordering (top to bottom):**
1. Display (existing)
2. Mode Picker (existing, enhanced with per-mode settings)
3. Scheduling (new — within or adjacent to Mode Picker)
4. Timing (existing)
5. Network & API (existing)
6. Firmware (existing, evolved for OTA Pull)
7. Night Mode (Foundation)
8. Hardware (existing)
9. Calibration (existing)
10. Location (existing)
11. Logos (existing)
12. System (existing)

### Key Interaction Patterns

#### Mode Scheduling Flow

1. **Browse** — Scheduling tab shows a visual timeline (24-hour CSS Grid bar with JS-positioned rule overlays). Configured rules display as colored segments. Empty state shows "No schedule — the wall stays in the mode you pick" with an "Add Rule" prompt.
2. **Create** — User selects a mode from a dropdown, sets start and end time with native `<input type="time">`. Visual timeline updates immediately to show the new rule's coverage. Midnight-crossing is handled natively (23:00-07:00 shades both ends of the bar with a connecting indicator).
3. **Validate** — Timeline shows all rules simultaneously. Overlapping rules are highlighted with resolution guidance. Conflict-free rules save immediately.
4. **Save** — Rules persist to NVS immediately. No "save changes" button — each rule is saved on creation/edit. The wall begins following the schedule on the next trigger time.
5. **View** — All configured rules listed with mode name, time range, and enabled/disabled status (FR39). Current time marker on timeline shows "you are here."
6. **Edit/Delete** — Each rule has inline edit (tap to modify times/mode) and delete (swipe or tap trash icon) with single-action confirmation (FR16). Edits save immediately.
7. **Monitor** — Dashboard status line shows current effective mode and why: "Active: Departures Board (scheduled)" or "Active: Clock (scheduled Departures, no flights)" or "Active: Live Flight Card (manual override until next scheduled change)."

**NTP Failure Handling:** If NTP sync has not succeeded, the scheduling tab shows an amber status: "Time sync unavailable — schedule saved but inactive." Rules save to NVS and activate automatically when the first NTP sync succeeds. No schedule-related buttons are disabled — users can still create and edit rules.

#### Override Resolution Flow

Manual mode activation via Mode Picker during an active schedule creates a temporary override:
- Dashboard shows: "Active: Live Flight Card (manual override — schedule resumes at next change)"
- Override persists until the next scheduled transition fires, at which point the schedule reclaims control
- "Resume Schedule" button available in Mode Picker for immediate return to scheduled behavior
- Override does NOT persist across reboots — reboot returns to scheduled mode

#### OTA Pull Flow

1. **Discovery** — Firmware card shows current version. "Check for Updates" button queries GitHub Releases API. If a newer version exists: banner reads "v2.4.1 available — you're running v2.4.0." If GitHub is unreachable: "Unable to check for updates — try again later." If rate-limited (HTTP 429): same message, no auto-retry.
2. **Evaluate** — "View Release Notes" shows the GitHub Release description rendered in-dashboard. User reads the changelog before deciding.
3. **Commit** — "Update Now" button. No confirmation dialog — the release notes served as informed consent.
4. **Pre-download** — ESP32 verifies minimum 80KB free heap (FR34). If insufficient: "Not enough memory — restart device and try again." ESP32 tears down active DisplayMode to free heap. LED matrix shows static "Updating..." progress screen.
5. **Download** — ESP32 sends HTTP 202 response, then begins blocking firmware download to inactive OTA partition. Dashboard shows client-side estimated state: "Downloading firmware... device unreachable during install (typically 60 seconds)." No real-time progress is possible during this phase — the web server is blocked.
6. **Verification** — ESP32 downloads companion .sha256 file and verifies integrity. On mismatch: aborts, stays on current partition.
7. **Reboot** — Dashboard shows "Rebooting... reconnecting in 15s" with client-side countdown.
8. **Resolution** — Dashboard polls /api/ota/status after countdown. Three outcomes:
   - **New version confirmed** — green toast: "Updated to v2.4.1." Resume normal operation.
   - **Rollback detected** — persistent amber banner: "Update installed but failed self-check — rolled back to v2.4.0. Previous version restored." Includes explanation of what happened and guidance.
   - **Unreachable after 60s** — amber guidance: "Device unreachable — check WiFi or try refreshing."
9. **Failure states** — Plain language per FR30 (failure phase + outcome + next action):
   - Download phase: "Download failed — firmware unchanged. Tap to retry." (retriable)
   - Verify phase: "Integrity check failed — firmware unchanged. Tap to retry download." (retriable)
   - Boot phase: "Update installed but failed self-check — rolled back to v2.4.0." (rolled back, no action needed)
   - GitHub unavailable: "GitHub unreachable — try again later." (retriable later)
   - Heap insufficient: "Not enough memory — restart device and try again." (retriable after reboot)

#### Mode-Specific Settings Flow

1. **Access** — Each mode card in Mode Picker gains an expandable settings panel (collapsed by default). Settings icon visible on mobile (persistent) and desktop (always visible). First tap shows brief tooltip: "Configure this mode's display options."
2. **Configure** — Settings are mode-specific: Clock Mode shows 12h/24h toggle; Departures Board shows max rows slider (1-4) and telemetry field selection; Live Flight Card shows telemetry field checkboxes from the same configurable field set.
3. **Apply** — Changes apply immediately to the active mode via mode restart (teardown + re-init with new config, ~100ms). Cross-fade transition smooths the layout change. If configuring an inactive mode, changes are saved and take effect when that mode activates.
4. **Persist** — All mode settings saved to NVS, surviving power cycles, reboots, and OTA updates.
5. **Reset** — Each mode settings panel includes "Reset to Defaults" with before/after preview. Defaults optimize for immediate visual impact (e.g., Departures Board defaults to 3 rows, Clock defaults to 12h).

#### First-Time User Journey (Casey)

Casey's path through Delight features follows progressive discovery:
1. **Dashboard loads** — Mode Picker shows available modes with active mode highlighted. Casey sees what the wall is doing right now. This is Casey's layer one — complete and useful.
2. **"What's new" banner** (if upgrade) — "Your wall now has scheduling, per-mode settings, and one-tap updates." Direct links to each feature. Banner dismisses permanently after first interaction.
3. **Settings discovery** — Casey taps a mode card and notices the settings icon. Expands it, sees 12h/24h toggle. Tries it. Wall changes. The settings panel was one tap deeper than activation — discoverable but not blocking.
4. **Scheduling discovery** — Casey notices the "Scheduling" tab adjacent to Mode Picker. Opens it, sees empty timeline with "Add Rule" prompt. Creates a rule. The schedule timeline confirms visually. Casey now understands the wall can run itself.
5. **OTA discovery** — Casey scrolls to Firmware card, sees current version. "Check for Updates" is self-explanatory. First banner includes one-sentence explainer: "Install updates from GitHub — no cable needed."

### Effortless Interactions

- **Scheduling setup** — Pick a mode, set two times, done. Visual timeline confirms the schedule. Rules save automatically. Total setup: under 2 minutes including one mistake and correction. The schedule runs indefinitely without maintenance.
- **OTA Pull** — Banner says update available. Tap release notes. Tap install. Wait 60 seconds. Done. No cable, no compile, no file picker. The update workflow is shorter than explaining how the old one worked.
- **Idle fallback** — Zero configuration. Clock Mode activates automatically when no flights are in range. Deactivates when flights return. The user discovers this by noticing the wall shows the time instead of a blank screen. The best UX for idle fallback is no UX at all.
- **Mode-specific settings** — Expand the settings panel on any mode card. Flip a toggle or move a slider. Change takes effect immediately on the wall with a smooth cross-fade (if that mode is active). Collapse and forget. No save button, no confirmation, no reboot.
- **Transition experience** — Automatic. When modes change (schedule trigger, idle fallback, manual switch), the wall fades rather than hard-cutting. No configuration needed for the default behavior. Transition speed/style settings available for those who want them, invisible for those who don't.
- **Schedule editing** — Tap an existing rule in the schedule list. Modify times or mode inline. Change saves immediately. Delete with one tap + confirmation. The visual timeline updates in real time.
- **Feature discovery** — On first dashboard visit after upgrade, a "what's new" banner highlights new capabilities with direct action links. Banner appears after successful OTA reconnection (not during OTA progress). Dismisses permanently after interaction.

### Critical Success Moments

1. **The first scheduled transition** — Christian configures "Departures Board by day, Clock overnight." That evening at 11pm, the wall fades from the departures list to a clean clock display. No tap, no command. The schedule worked. This is the moment the wall becomes autonomous — it followed the owner's instructions without being told again.

2. **The Departures Board with multiple flights** — Three planes overhead. The wall shows all three simultaneously — callsigns, altitudes, speeds — in a departures-board format. Previously, flights cycled one at a time. Now the full picture is visible at a glance. This is day-one value delivery — visible immediately after Delight firmware is running.

3. **The guest notices the clock** — No flights in range. Sam walks in and sees a clean clock display. "Nice clock." When a plane enters range, the wall fades to the flight display. Sam doesn't realize the wall just made an autonomous decision. Nobody configured this interaction — idle fallback activated automatically. This is the moment the wall stops looking broken during quiet skies.

4. **The schedule prevents a midnight disturbance** — 2am. The wall shows Clock Mode in dim night-mode brightness. Without scheduling, it would have been cycling through flight data at full brightness. The overnight schedule protected the room. This validates scheduling as protective automation, not just convenience.

5. **The first OTA Pull update** — Dashboard shows "v2.4.1 available." Christian taps install, waits 60 seconds, sees the wall reboot and resume. New version confirmed. The USB cable drawer stays closed. This is the moment TheFlightWall becomes a consumer-updatable device.

6. **The friend demo, Delight edition** — Friends are over. Christian opens the Mode Picker, switches between modes. The wall transitions smoothly each time. Then Christian says "and it updates itself now" and shows the Firmware card. Then "and it follows a schedule" and shows the timeline. The wall is no longer a project to explain — it's a product to show off.

7. **The first schedule edit** — Christian realizes "Departures Board at 6am" is too early — the wall was tracking pre-dawn cargo flights nobody cares about. Opens the scheduling tab, finds the rule, changes start time to 7:30am. Saves instantly. The visual timeline shifts. This validates that schedules are living, editable preferences — not carved-in-stone automation.

8. **The graceful OTA failure** — An update fails mid-download. Dashboard shows "Download failed — firmware unchanged. Tap to retry." The wall resumed its previous display. The firmware is intact. Christian retries after reconnecting WiFi. No panic, no brick, no USB recovery. Trust in the system deepens.

### Experience Principles

1. **Autonomy through configuration** — The Delight Release's highest UX goal is earned autonomy: the wall runs itself because the owner told it how. Scheduling, idle fallback, and transitions all serve this principle. Every "set and forget" interaction should feel like granting permission, not surrendering control. The owner can always override manually. Priority hierarchy: explicit schedule > idle fallback automation > manual override (temporary until next scheduled change).

2. **The wall and the phone both confirm** — Inherited from Display System and extended: the LED matrix and the dashboard must independently confirm every state change. The wall transitions — that's the primary confirmation. The dashboard shows "Active: Clock (scheduled)" — that's the secondary. Neither channel should require the other to feel complete. During OTA, when the wall goes dark, the dashboard alone carries the trust burden.

3. **Honest feedback, calm tone** — Every system state has a plain-language representation. "Active: Departures Board (scheduled)" not "mode_id=2, trigger=cron." Failure messages always include three elements per FR30: what failed (phase), what happened (outcome), and what to do next (action). "Download failed — firmware unchanged. Tap to retry." The emotional register is professional competence — never alarm, never dismissal.

4. **Progressive disclosure, not hidden complexity** — Scheduling, per-mode settings, and transitions are power features built on a simple foundation. The Mode Picker's "tap to activate" simplicity is the entry point. Settings expand from mode cards. Scheduling reveals from a tab. Transitions happen automatically with optional tuning. Each layer adds capability without requiring it. Casey's experience is complete at layer one. Christian's experience deepens through all three. Defaults optimize for immediate visual impact — configuration refines for personal taste over time.

5. **Invisible when working** — Idle fallback, scheduled transitions, NVS persistence, OTA integrity checks, watchdog recovery — the best features are the ones users never notice because they just work. Night mode dims at night. Clock appears when skies are empty. Settings survive reboots. The absence of friction is the highest form of UX success. Watchdog recovery from a crash results in a clean reboot to default mode — the user sees a brief blank and then normal operation, never a brick.

6. **Ambient legibility — wall state is self-evident** — The LED matrix must communicate its current state without requiring the dashboard or an explanation from the owner. Departures Board is recognizable as a departures board from across the room. Clock Mode reads as a clock. Transitions appear intentional. No display state should require the owner to say "oh, it's supposed to do that." Sam's confusion is a UX bug, not a complexity issue. When the system degrades (API failures, stale data), the wall continues showing the last good state rather than an error screen — degradation is calm and invisible to passive observers.

## Desired Emotional Response

### Primary Emotional Goals

**Earned trust in automation** — "I set it up, and now it handles itself." The Delight Release's defining emotional arc moves beyond trust in individual actions (Foundation: "I can update safely", Display System: "I can switch modes reliably") to trust in autonomous behavior. The wall makes decisions — when to show the clock, when to show departures, when to transition — based on rules the owner configured. The emotional goal is quiet confidence that the wall is doing the right thing even when you're not watching.

**Ambient pride** — "That's not a project, that's furniture." The Delight Release's polish (smooth transitions, no blank screens, time display when idle) transforms the wall from a conversation-starter-that-needs-explanation into a conversation-starter-that-speaks-for-itself. Ambient pride manifests through three observable behaviors: (1) unsolicited guest engagement — guests ask "what is that?" without prompting; (2) owner-initiated demonstration — Christian shows friends the Mode Picker and schedule; (3) aesthetic integration — the wall looks like it belongs in the room, not like a hobby project mounted on the wall.

**Effortless authority** — "I'm in charge, but I don't have to be involved." Scheduling, idle fallback, and manual override create a layered control model. The owner's preferences are always respected (schedule takes priority, manual override available anytime), but the system handles moment-to-moment decisions. The feeling is delegation, not abdication — the owner gave instructions and the wall follows them.

### Emotional Journey Map

| Stage | Persona | Feeling | Design Implication |
|-------|---------|---------|-------------------|
| First dashboard visit after upgrade | Christian/Casey | Curiosity — "what's new?" | "What's new" banner with direct action links. Inviting, not demanding. Discovery should feel like finding a gift, not reading a changelog. |
| First scheduling setup | Christian | Satisfaction — "that was easy" | Under 2 minutes. Visual timeline gives instant confirmation. Rules save automatically. No friction between intent and result. |
| First scheduled transition (observed) | Christian | Validation — "it listened to me" | Wall changes at the configured time. Dashboard confirms with reason. The emotional beat is: I taught it, it learned. |
| Active verification week | Christian | Building trust — "I'm checking it's consistent" | Dashboard shows schedule history: last 7 days of transitions with timestamps. Users don't jump from "it worked once" to "I trust it." They audit consistency first. Schedule history provides the evidence. |
| First scheduled transition (unobserved) | Christian | Growing trust — "it's been running itself" | Christian checks the dashboard the next morning. Status shows Clock Mode ran overnight, Departures Board started at 7:30am. Everything worked. No intervention needed. |
| First time not checking | Christian | Earned autonomy — "I forgot to check and that's fine" | This is the emotional summit. The schedule has been running for weeks. Christian didn't verify. The wall just works. Trust is complete. |
| First OTA Pull | Christian | Frictionless maintenance — "I didn't have to stop what I was doing" | Christian is on the couch. Dashboard shows update available. Taps install, goes back to what he was doing. 60 seconds later, glances at wall — flights resume on new version. Maintenance happened without becoming a task. No ladder, no unplugging, no context-switching into "maintainer mode." |
| OTA failure/rollback | Christian | Calm acceptance — "the system caught it" | Persistent amber banner with plain language. Previous version restored. No panic. The emotional response to failure is: "good, it handles that." |
| Compound failures | Christian | Contained concern — "something's actually wrong, but I know where to look" | OTA failed twice. Dashboard shows clear status with diagnostics: WiFi signal, GitHub connection status, suggested action. User feels confused but not abandoned. There's a path from "multiple things broke" to "ah, it's the WiFi." |
| Departures Board first view | Christian/Casey | Delight — "it can show all of them at once" | Multi-flight display is immediately more impressive than one-at-a-time cycling. The emotional reaction is visible capability — the wall can do more than I thought. |
| Guest encounters the wall | Sam (tech-curious) | Ambient appreciation — "that's cool, is that real-time?" | No explanation needed. The wall is showing something self-evidently interesting (flights) or useful (time). Transitions feel intentional. The guest doesn't ask "is it broken?" |
| Guest encounters the wall | Sam (tech-averse) | Comfortable ignorance — "oh, it shows flights" | The wall doesn't demand attention or explanation. Clock Mode idle state serves both curious and indifferent observers. Tech-averse guests can comfortably ignore it. Design for the least engaged person in the room. |
| Guest ambient trust (recurring) | Sam | Habituation — "it's just always there" | Over days/weeks, Sam stops noticing the wall consciously but glances at it for information. The wall becomes a trusted ambient data source — like a clock or weather display. Consistent uptime and no jarring error states earn this. |
| Mode switch during friend demo | Christian | Showmanship — "watch this" | Rapid switching works cleanly. Transitions are smooth. The wall is the star, the phone is the remote. Pride in demonstrating a polished system. |
| Schedule prevents midnight brightness | Christian | Protective comfort — "it knows not to wake me" | The overnight schedule kept Clock Mode at dim brightness. Without it, flights would have lit up the bedroom. The schedule protected the space. |
| Schedule needs adjustment | Christian | Iteration, not frustration — "easy fix" | Christian realizes "Departures at 6am" is too early. Opens scheduling tab, taps the rule, adjusts time. Most schedule tweaks are small shifts, not complete rewrites. Quick adjustment prevents abandoning schedules due to minor misalignment. |
| API failure during scheduled mode | Christian | Unnoticed — system handled it | Departures Board was scheduled but API failed. Wall fell back to Clock (idle fallback). Dashboard shows "Active: Clock (Departures scheduled, no data)." User only notices if checking dashboard. The wall never showed an error to Sam. |
| Mode-specific settings tweak | Christian | Precision — "I can fine-tune this" | Expanding the settings panel on a mode card reveals per-mode options. Changing 12h to 24h and seeing the wall update instantly via cross-fade feels like control at the detail level. |
| Casey discovers scheduling | Casey | Progressive revelation — "wait, it can do that?" | Casey notices the Scheduling tab after a few dashboard visits. Opens it, sees empty timeline. Creates first rule. The wall gains a new dimension Casey didn't know existed. This is Casey's layer-two discovery. |

### Micro-Emotions

**Prioritize:**
- **Quiet confidence** over active monitoring — the schedule is running; don't feel the need to verify
- **Delegation** over abdication — I set the rules, the wall follows them, I can override anytime
- **Ambient pride** over active showing-off — the wall speaks for itself; I don't need to explain it
- **Calm competence** over surprise — failures are handled, rollbacks are automatic, the system is resilient
- **Precision** over complexity — per-mode settings add depth without adding cognitive load
- **Iteration** over perfection — schedules are living preferences, easily adjusted; the first configuration doesn't need to be the final one

**Avoid:**
- **Anxiety** during OTA download — the 60-second unresponsive window must feel managed, not abandoned. Client-side countdown and clear messaging prevent "is it bricked?" panic
- **Confusion** about effective mode — "why is it showing the clock when I scheduled Departures?" The reason display ("no flights in range") prevents debugging a correctly-working system
- **Frustration** from schedule conflicts — overlapping rules detected immediately, not discovered by surprise at 3am
- **Disconnection** from automation — the wall shouldn't feel like it's making decisions the owner doesn't understand. Every autonomous behavior has a visible rule or preference behind it
- **Escalating helplessness** from compound failures — when multiple things go wrong, the dashboard should guide investigation (WiFi signal, API status, suggested action), not leave the user to guess
- **Notification fatigue** — "what's new" banner appears once. No badges, no recurring prompts

### Design Implications

| Emotion | UX Approach |
|---------|-------------|
| Earned trust | Status line always shows current mode + reason. "Active: Departures Board (scheduled)" communicates that the schedule is working. Schedule history (last 7 days) provides audit trail during trust-building phase. |
| Ambient pride | Transitions, idle fallback, and display density create a product that looks finished without explanation. Sam's positive reaction (both tech-curious and tech-averse) is the metric. |
| Effortless authority | Manual override available anytime with one tap. "Resume Schedule" button returns to automation. The owner never feels locked out of their own wall. |
| Calm competence | Every failure state names cause + outcome + action. Rollback is automatic. The emotional arc of failure is: surprise → information → reassurance → resolution. For compound failures: diagnostics guide investigation. Never: surprise → confusion → panic → abandonment. |
| Precision | Mode-specific settings panels expand from mode cards. Changes apply immediately via cross-fade. The settings are one level deeper than activation — available for those who want fine control, invisible for those who don't. |
| Delegation comfort | Schedule timeline shows all rules visually. "Effective mode" vs "scheduled mode" with reason provides transparency. The owner can see the wall's decision-making logic at any time. |
| Schedule iteration | Quick time-adjust interactions (±30min nudge buttons alongside full edit) prevent schedule abandonment. Most tweaks are minor time shifts, not complete rewrites. |
| Frictionless maintenance | OTA Pull positioned as disruption avoidance — no physical access, no context-switching into "maintainer mode." Updates happen during normal dashboard use, not as a separate task. |

### Emotional Design Principles

1. **Trust compounds across releases** — Foundation built trust in safe updates. Display System built trust in reliable switching. Delight builds trust in autonomous behavior. Each release raises the trust baseline. The emotional contract is: we've earned your trust through three releases of keeping our promises. Now trust us to run the wall while you're not watching.

2. **Ambient over active** — The highest emotional achievement is when the wall becomes background. Not ignored — appreciated at a glance, impressive to guests, useful as a clock — but not requiring attention. The schedule runs. The transitions are smooth. The idle fallback fills gaps. The emotional goal is: the wall is part of the room, not a gadget on the wall.

3. **Failure is a trust opportunity** — Every graceful failure deepens trust more than ten successful operations. OTA rollback, schedule-vs-fallback resolution, API failure handling — these edge cases are where the wall proves it's robust. For compound failures, the system guides investigation rather than leaving the user to guess. The emotional design principle is: handle failures so well that users feel more confident after experiencing one.

4. **Delegation requires transparency** — Autonomous behavior earns trust only when the owner can see the logic. "Active: Clock (scheduled Departures, no flights)" is delegation with transparency. "Active: Clock" alone would feel opaque. The status line is the emotional contract between automation and control — it says: "here's what I'm doing and why." Schedule history extends this transparency backward in time.

5. **Delight is proportional and persona-specific** — The "Delight" Release name refers to the outcome users feel, not the emotional register of the UI. Delight manifests differently for each persona: for Christian, delight is earned autonomy (the wall runs itself). For Casey, delight is progressive revelation (discovering capabilities existed). For Sam, delight is ambient confidence (the wall "just works" as background). For Jordan, delight is creative agency (the plugin architecture lets them contribute). The UI's emotional register is sophisticated restraint — no gamification, no excessive feedback, no forced tutorials. A scheduled transition at 11pm is a gentle fade, not fanfare. An OTA update succeeding is a green toast, not confetti. The product is mature; the emotions should match.

## UX Pattern Analysis & Inspiration

### Inspiring Products Analysis

**Google Nest Thermostat (Schedule Automation)**
- Schedule creation: set temperatures for time blocks. Visual daily/weekly view shows all rules at a glance. "Away" mode auto-activates based on presence — data-driven fallback.
- Schedule editing: tap a time block, drag edges to adjust. Quick time-shift is the primary edit, not rebuilding rules.
- History: "Energy History" shows what happened and when. Users can verify automation worked without being present.
- Learning: Nest learns from manual overrides and adjusts schedule suggestions. The UI shows "based on your adjustments."
- **Lesson for FlightWall:** The visual schedule timeline maps directly. Nest's time-block drag-to-adjust is our "quick adjust" pattern. Schedule history ("last 7 transitions with timestamps") mirrors Energy History — the audit trail that builds trust. The key insight: Nest treats manual overrides as input, not rebellion. FlightWall's override resolution should feel the same: "you overrode, the schedule resumes next trigger."

**iOS/Android System Updates (Consumer OTA)**
- Update available: notification badge + banner in Settings showing version number and "what's new" summary. No urgency framing — "install tonight" is the default suggestion.
- Download progress: real progress bar (the OS can track it). "Preparing Update..." then "Installing..." with estimated time.
- Reboot: "Your device will restart. This will take a few minutes." Clear expectation-setting. After reboot: brief setup screen, then normal operation.
- Failure: "Unable to Install Update. An error occurred installing iOS 18.3." Retry available. Device unchanged.
- Rollback: iOS doesn't expose rollback to users. Android's A/B partitions roll back silently. Neither explains the mechanism.
- **Lesson for FlightWall:** The update banner pattern ("v2.4.1 available") maps directly from iOS Settings. The critical difference: FlightWall can't show real-time download progress (ESP32 web server is blocked). We must set expectations: "device unreachable for ~60 seconds" rather than showing a progress bar that stalls. Rollback communication is where FlightWall differentiates — see Innovate section below.

**Chromecast Ambient Mode / Apple TV Screensaver (Idle Fallback)**
- Both devices switch to a pleasant ambient display when idle — photos, artwork, time, or weather. No "no content" error screen.
- The transition from active content to ambient mode is seamless. No jarring cut, no loading indicator.
- User configuration: Chromecast lets users choose ambient mode content (photos, art, weather). Apple TV auto-selects screensavers.
- Resume: when new content becomes available (cast starts, remote pressed), the idle display yields immediately.
- **Lesson for FlightWall:** Clock Mode idle fallback IS the ambient mode. The pattern is identical: active content (flights) yields to ambient display (clock) when idle, and resumes when content returns. The critical insight from both products: the idle state should feel intentional, not like a fallback. Clock Mode should look like a feature, not a "no data" state. The fade transition between flights and clock mirrors Chromecast/Apple TV's seamless yield.

**Home Assistant Update Dashboard (Open Source Device OTA)**
- Shows available updates for all devices in one view. Each update shows current version, available version, and changelog.
- "Update All" for batch updates, or per-device "Update" buttons.
- Progress shown per device. Clear "update failed" / "update successful" states.
- No rollback exposed in UI — HA doesn't support automatic rollback for most integrations.
- **Lesson for FlightWall:** Single-device simplicity is our advantage. Home Assistant's per-device update pattern is overkill for FlightWall's one-device model. But the changelog presentation (inline, scannable, before the install button) maps directly to our "View Release Notes" pattern. The "Update All" anti-pattern warns against future scope creep — FlightWall should never try to update multiple things simultaneously.

**UniFi Controller (Passive OTA Discovery & Rollback Visibility)**
- Always-visible "Updates" tab in sidebar navigation. Shows "3 devices need updates" badge. Persistent, passive discovery — no push interruption. Users check it when they want to, not when the system demands.
- When firmware upgrade fails, the device reverts to previous firmware and shows an amber "Upgrade Failed - Reverted to vX.X.X" banner in the controller UI.
- **Lesson for FlightWall:** FlightWall's dashboard is demand-loaded (user visits when they want to configure). Unlike iOS push notifications, there's no way to alert the user between visits. The Firmware card must be immediately scannable on every dashboard load — version comparison ("v2.3.1 → v2.4.1 available") as the first visual element, before changelog, before install button. UniFi's rollback visibility confirms the pattern is established in the embedded device category; FlightWall's innovation is adding triage guidance alongside the rollback notification (see Innovate section).

**IFTTT Activity Log (Schedule Debugging & Trigger Reasoning)**
- Shows every automation trigger with timestamp, conditions evaluated, and action taken.
- Explicit "why" for each trigger: "Trigger fired because: time is 8:00 AM AND location is Home."
- Shows failed/skipped triggers: "Trigger skipped because: location is Away."
- **Lesson for FlightWall:** The schedule history pattern from Nest Energy History shows WHAT happened. IFTTT adds the WHY. When a user sees "8:02 AM: Flight Mode (scheduled)" in the transition history, they should be able to see the trigger source: "Triggered by: Weekday Morning rule (8:00-12:00) | Overrode: Clock Mode (idle fallback)." This explicit trigger reasoning transforms schedule history from a trust-building audit trail into a debugging tool when schedules don't behave as expected.

**Digital Signage Systems (Vestaboard, LaMetric) — Scheduled Content Switching**
- Vestaboard: owners schedule messages to appear at specific times. Simple time + content rules. Visual schedule shows today's planned messages.
- LaMetric: apps cycle on a timer. Each app (weather, clock, notifications) gets a time slot. Manual override via physical button.
- Both: the physical display is the primary feedback. The phone/web app is the control surface.
- **Lesson for FlightWall:** These are the closest product category peers — scheduled content on a physical display controlled from a phone. The key shared pattern: the display is the truth, the app is the remote. LaMetric's manual button override maps to our Mode Picker manual override during a schedule. Vestaboard's "today's schedule" view maps to our visual timeline. The critical differentiator: FlightWall's content is live data (flights), not static messages — adding data-driven idle fallback as a scheduling layer that signage systems don't need.

**Philips Hue Scene Editor / Camera App Preset Modes (Mode-Specific Settings)**
- Philips Hue: each scene (mode) has its own settings (brightness, colors per light). Editing a scene changes it permanently — all future activations use the new settings. Scene picker and scene editor are separate UI surfaces.
- Camera app preset modes: "Portrait Mode" has depth settings, "Night Mode" has exposure settings. Switching modes switches available settings. Mode picker shows preview, settings are in a separate panel.
- **Lesson for FlightWall:** Mode-specific settings (Clock 12h/24h, Departures Board row count, Live Flight Card telemetry fields) follow the same pattern: mode picker is for switching, mode settings panel (accessed via gear icon in mode card) is for configuring. Changes persist across all future activations. Without this reference pattern, the risk is "scattered settings" — mode-specific options mixed into global settings, confusing users about what affects what.

### Transferable UX Patterns

**Schedule Automation Patterns:**
- **Visual time-block editing** (Google Nest) — Tap a time block, drag edges to adjust start/end times. For FlightWall: the visual timeline's rules should be directly editable — tap to select, adjust times inline. Most schedule edits are minor time shifts.
- **Schedule history with trigger reasoning** (Google Nest + IFTTT) — Show what happened, when, and why. For FlightWall: "Last 7 days" transition log with timestamps and trigger source ("scheduled rule: Weekday Morning" or "idle fallback: no flights"). Users audit consistency and debug unexpected behavior.
- **Manual override as input, not rebellion** (Google Nest) — Override doesn't break the schedule; it's a temporary deviation. For FlightWall: override resolves at next scheduled trigger. The system treats overrides as normal, not as errors.
- **Explicit override resolution communication** (Google Nest "hold until next change" + Sonos "alarm snoozed until [time]") — When an override is active, communicate when the schedule will resume: "Manual override until next schedule trigger (8am tomorrow)."

**OTA Update Patterns:**
- **Passive discovery in persistent UI** (UniFi Controller) — Version comparison as the first visual element on every dashboard load. "v2.3.1 → v2.4.1 available" is the entry point before changelog or install button. No push notifications; discovery happens when the user visits.
- **Pre-install changelog** (iOS, Home Assistant) — Show what's changing before committing. For FlightWall: "View Release Notes" inline in the Firmware card. The changelog serves as informed consent — no confirmation dialog needed after reading it.
- **Expectation-setting for duration** (iOS "This will take a few minutes") — For FlightWall: "Device unreachable for ~60 seconds" is our equivalent. Honest about the duration, honest about the user's inability to interact during it.
- **Rollback with triage path** (UniFi/Synology show rollback status; FlightWall adds actionable next steps) — UniFi shows "Upgrade Failed - Reverted to vX.X.X." Synology shows "Update failed. System restored to version X.X.X." FlightWall adds the triage layer: "Self-check failed → rolled back to v2.4.0 → view logs → try again later or report issue." Same rollback visibility, with built-in investigation path.

**Idle Fallback Patterns:**
- **Ambient mode as feature, not fallback** (Chromecast, Apple TV) — The idle state should look intentional. For FlightWall: Clock Mode is a feature, not a "no data" screen. It should be designed as a desirable display state, not a placeholder.
- **Seamless yield to new content** (Chromecast) — When flights return, Clock Mode yields immediately with a fade transition. No "loading flights..." intermediate state. The data drives the transition.

**Mode-Specific Settings Patterns:**
- **Mode picker is for switching, settings panel is for configuring** (Philips Hue Scenes, Camera App Modes) — These are separate UI surfaces. The mode card activates with one tap; the gear icon opens per-mode settings. This prevents mode-specific options from cluttering the activation flow.
- **Settings persist across activations** (Philips Hue) — Changing Clock Mode to 24h persists for all future Clock Mode activations. No "save for this session" vs. "save permanently" ambiguity.

**Physical Display + Phone Remote Patterns:**
- **Display is truth, app is control** (LaMetric, Vestaboard, Apple TV Remote) — The LED matrix shows what's actually happening. The dashboard shows what you can control. These are complementary channels, not mirrors.
- **Manual override via both channels** (LaMetric button + app) — For FlightWall: Mode Picker (app) and scheduled transitions (automatic). Both override paths exist, neither requires the other.

### Anti-Patterns to Avoid

- **Fake progress bars during blocked operations** — iOS can show real download progress. FlightWall's ESP32 cannot serve web requests during OTA download. Showing a smooth-filling progress bar would be dishonest. Instead: "Installing update... device unreachable (typically 60 seconds)" with a client-side countdown timer. Honest > polished.
- **Hidden rollback** — iOS and Android hide rollback from users. For a maker/hobbyist product, this feels opaque. FlightWall should explicitly communicate: "Update failed self-check — rolled back to v2.4.0. Your device is unchanged." Transparency matches the audience.
- **Schedule-as-rigid-rule** — Some home automation systems treat schedules as inviolable. If a scheduled action conflicts with the current state, they force it. FlightWall's schedule should feel like a preference, not a command — manual override is always available, and the schedule resumes at the next trigger.
- **"Smart" learning without transparency** — Nest's learning thermostat adjusts schedules based on user behavior. For FlightWall, this would feel opaque — the wall shouldn't start changing modes on its own without a visible rule. All automation should trace back to an explicit schedule rule or the idle fallback behavior. No invisible learning.
- **Idle state as error state** — Some IoT dashboards show "No Data" or a spinning loader when content is unavailable. Clock Mode should feel like a feature, not an error. The idle state is the wall being useful (showing time), not the wall failing (showing nothing).
- **Background polling for schedule status** — Home Assistant polls devices continuously. FlightWall's ESP32 can't handle this. The dashboard should fetch schedule/mode status on page load and after user actions — never in the background. State is demand-driven.
- **Ambiguous override resolution** — Some smart home systems (e.g., SmartThings v1) don't communicate when a schedule will resume after manual override. User overrides a scheduled action, and the system gives no indication of whether the schedule resumes at midnight, at the next trigger, or never. FlightWall must show in Mode Picker: "Manual override — schedule resumes at next change (8am tomorrow)" when a manual override is active during a schedule window. Nest's "hold until next change" and Sonos's "alarm snoozed until [time]" are positive examples of explicit resume communication.
- **Scattered mode-specific settings** — Mixing mode-specific settings (Clock 12h/24h, Departures row count) into global settings panels creates confusion about what affects what. Mode-specific settings belong in mode-specific UI surfaces (gear icon in mode card), not in a general Settings page.

### Design Inspiration Strategy

**Adopt directly:**
- Google Nest's visual schedule timeline with time-block editing
- iOS's "what's new" update banner pattern (version comparison + changelog)
- Chromecast's "ambient mode as feature, not fallback" philosophy for Clock Mode
- LaMetric's "display is truth, app is control" channel separation
- UniFi Controller's passive discovery pattern (version comparison as first visual element in Firmware card)
- Philips Hue's mode picker / mode settings separation pattern

**Adapt:**
- Google Nest's schedule history + IFTTT's trigger reasoning — combined into a simplified "last 7 transitions with trigger source" log. Same trust-building function plus debugging capability, lighter implementation than either product's full analytics.
- iOS's update progress — adapted for ESP32's blocking download. Client-side countdown replaces real-time progress bar. Same emotional function (patience during wait), different mechanism (honest expectation vs. real-time tracking).
- Vestaboard's daily schedule view — adapted as a 24-hour visual timeline with mode-colored segments. Same visual communication, different content (modes vs. messages).
- Google Nest's "hold until next change" — adapted as explicit override resolution in Mode Picker status line: "Manual override — schedule resumes at next change (8am tomorrow)."

**Innovate:**
- **Rollback communication with built-in triage path** — UniFi and Synology show rollback status ("Reverted to vX.X.X"). FlightWall adds actionable guidance in the same UI element: "Self-check failed → rolled back to v2.4.0 → view logs → try again later or report issue." The innovation isn't showing rollback — it's making the rollback notification a starting point for investigation rather than just a status report.
- **Data-driven idle fallback as scheduling layer** — Chromecast's idle mode is timer-based. FlightWall's Clock Mode fallback is data-driven (no flights in range). This adds a real-time intelligence layer to the schedule that static signage systems lack.
- **Schedule + override coexistence with explicit resolution** — Most scheduling systems treat override as conflict. FlightWall treats it as temporary deviation with explicit communication: override resolves at next scheduled trigger, then schedule resumes. Mode Picker shows both the override status and the next resume time. This mirrors Nest's "manual adjustment" philosophy but applies it to display modes with full transparency about the resolution timeline.

**Avoid:**
- Fake progress indicators (be honest about blocked state)
- Hidden rollback mechanisms (be transparent about recovery)
- Background polling (demand-driven state updates only)
- Implicit learning or behavior prediction (all automation traces to explicit rules)
- Idle state as error or placeholder (Clock Mode is a feature)
- Ambiguous override resolution (always show when schedule resumes)
- Scattered mode-specific settings (keep settings in mode-specific UI surfaces)

## Design System Foundation

### Design System Choice

**Custom Design System (continued from Foundation & Display System Releases)**

TheFlightWall uses an existing custom design system established in the Foundation Release and extended in the Display System Release. The Delight Release continues this system with targeted extensions — no design system migration or framework adoption.

### Rationale for Selection

1. **Existing investment** — Three releases of CSS custom properties, card layouts, toast notifications, button patterns, and responsive breakpoints. The design system is proven, consistent, and familiar to the sole developer.

2. **Platform constraints eliminate alternatives** — ESP32 LittleFS (~896KB) serves gzipped HTML/JS/CSS. No frontend framework (React, Vue, Svelte) is viable at this scale. No design system library (MUI, Chakra, Ant Design) can be included — each exceeds the entire LittleFS budget. The custom system is not a choice; it's the only viable path given embedded constraints.

3. **Vanilla HTML/JS/CSS by design** — ESPAsyncWebServer serves static files. No build step, no bundler, no transpiler. The design system must be native CSS custom properties, native HTML elements, and vanilla JavaScript. This constraint is permanent for the ESP32 platform.

4. **Single-developer team** — No cross-team consistency challenge. No design handoff. No Figma-to-code translation needed. The developer IS the designer. The custom system's lack of documentation overhead is an advantage, not a gap.

5. **Incremental extension model** — Each release adds components within the existing system. Foundation added cards, toasts, buttons. Display System added Mode Picker, schematic previews. Delight adds schedule timeline, OTA progress states, mode settings panels. The pattern is consistent: new components inherit existing tokens, layouts, and interaction patterns.

### Implementation Approach

**Extend, don't replace.** The Delight Release adds new UI components to the existing design system:

| New Component | Extends | CSS Custom Properties |
|---|---|---|
| Schedule Timeline | Card layout (existing) | `--timeline-bg`, `--rule-color-*`, `--time-marker` |
| Schedule Rule Editor | Form patterns (existing) | Inherits existing `--input-*`, `--btn-*` tokens |
| OTA Progress States | Toast/banner patterns (existing) | `--ota-pending`, `--ota-success`, `--ota-warning`, `--ota-error` |
| Mode Settings Panel | Card expansion (new) | `--settings-panel-bg`, `--settings-border` |
| Firmware Card (evolved) | Card layout (existing) | Reuses existing card tokens, adds `--version-highlight` |
| "What's New" Banner | Toast pattern (existing) | `--banner-info`, `--banner-dismiss` |

**Component budget:** ~10-12KB additional gzipped web assets for all Delight UI. Current total is ~9KB of ~896KB capacity.

### Customization Strategy

**Design token extension, not override:**
- All existing CSS custom properties remain unchanged — no Foundation or Display System regressions
- New tokens follow existing naming conventions (`--component-property` pattern)
- Semantic color tokens (success/warning/error/info) already exist and map directly to OTA states
- Schedule timeline requires 3-4 new mode-specific color tokens (one per display mode, used in timeline segments)
- Dark mode / night mode: inherit existing night-mode CSS class behavior; new components respect the same toggle

**Component inheritance pattern:**
- New components are built from existing primitives: cards, buttons, inputs, toasts
- Schedule Rule Editor composes `<input type="time">`, `<select>`, and existing button patterns
- Mode Settings Panel is a collapsible card section using existing card + disclosure triangle pattern
- OTA states map to existing toast variants (success=green, warning=amber, error=red)
- No new interaction primitives — all new interactions are compositions of existing patterns

## Defining Experience Deep Dive

### User Mental Model

**Christian's mental model evolves across releases:**
- Foundation: "I'm configuring a device" — settings, WiFi, upload firmware. The mental model is IT administration.
- Display System: "I'm controlling a display" — pick a mode, see it change. The mental model is a remote control.
- Delight: "I'm teaching the wall my preferences" — set a schedule, let it run. The mental model shifts to delegation. The wall becomes an agent that follows instructions.

This progression matters because the Delight dashboard must support ALL THREE mental models simultaneously. The settings cards (Foundation cadence) still exist. The Mode Picker (Display System cadence) still works. The new scheduling UI must feel like a natural evolution, not a replacement. The schedule is an extension of Mode Picker — "instead of picking now, pick when."

Note: the "delegation" mental model implies trust, but the schedule-history log and status-line reasons are *verification* mechanisms. Christian is delegating and auditing simultaneously during the trust-building phase. The monitoring features serve this "trust but verify" stage, which is temporary — a mature user eventually stops checking the history log. The "walk away" success criterion captures this arc from verification to earned autonomy.

**Casey's mental model is discovery-based:**
- First visit: "What can this thing do?" — Mode Picker answers this immediately. Available modes are visible, one is active.
- Second visit: "What else can it do?" — Settings panels and scheduling tab become visible exploration targets.
- Casey doesn't arrive with a "scheduling" mental model. The timeline must be self-explaining: colored blocks on a 24-hour bar communicate "these modes run at these times" without requiring the word "schedule."

**Sam's mental model is inference-based:**
- Sam never sees the dashboard. All mental models come from observing the physical display.
- "It shows flights" — when Live Flight Card or Departures Board is active.
- "It shows the time" — when Clock Mode is active.
- "It changes on its own" — when a transition happens. Smooth transitions make this feel intentional rather than broken.
- Sam's mental model should never reach "something went wrong." If it does, the display-side UX has failed.

**Jordan's mental model is architecture-based:**
- Jordan encounters OTA Pull from the other side — as the person whose code gets distributed through it. Jordan's mental model is "release pipeline" — how firmware binaries are tagged in GitHub Releases, how the device discovers versions, and what rollback means for contributed code.
- Jordan encounters Scheduling and Mode-Specific Settings as extension points. Jordan's mental model is "plugin architecture" — "how do I register a new mode so it appears in the scheduler and has its own settings panel?"
- Jordan has no visibility into individual device updates. When a release containing Jordan's contribution is pulled by a device, Jordan doesn't know. The UX implication: contributor documentation must explain how the release pipeline works from code to device, even though the contributor has no runtime touchpoint.

### Success Criteria

**The "walk away" test (Christian):**
- Configure schedule in under 2 minutes (including one correction)
- Walk away from dashboard for 7 days
- Return and verify: schedule history shows correct transitions, no manual intervention was needed
- Success metric: device logs show zero manual mode changes and zero dashboard visits for 7 consecutive days

**The "what does it do" test (Casey):**
- First dashboard visit: understand active mode in under 10 seconds
- First scheduling attempt: create a working rule in under 60 seconds
- Progressive discovery: find at least one additional feature (settings, scheduling, OTA) by second visit without being told it exists

**The "is it broken" test (Sam):**
- Observe the wall for 5 minutes during a mode transition
- Sam does not ask anyone about the display's status or attempt to interact with it during the observation period
- After a week: stop noticing transitions consciously (habituation = success)

**The "it just works" test (OTA):**
- See update available → install → resume normal operation in under 3 minutes total
- If failure occurs: understand what happened and what to do in under 30 seconds of reading the error message
- Never reach for a USB cable

**The "recovery" test (scheduling failures):**
- Create an overlapping schedule rule: overlap detected and highlighted within 1 second
- Experience NTP sync failure during scheduled transition: wall falls back gracefully, dashboard shows reason within 5 seconds of page load
- Edit a schedule rule that causes a conflict: resolution guidance appears inline, no data loss

### Novel vs. Established Patterns

**Established patterns (adopt directly):**
- Mode Picker → remote control pattern (tap to activate). Proven in Display System Release. No changes needed.
- Settings panels → expandable card pattern. Common in iOS Settings, Android device settings, UniFi. Familiar interaction.
- OTA update → software update pattern. iOS/Android model: banner → changelog → install → wait → done. Well-understood.
- Toast notifications → ephemeral feedback pattern. Green = success, amber = warning, red = error. Universal.

**Adapted patterns (familiar mechanism, new context):**
- Schedule timeline → adapted from Google Nest thermostat's time-block editor. The mechanism (colored blocks on a time axis) is familiar from calendar apps. The context (display modes instead of temperatures) is new but the interaction is identical.
- Override resolution → adapted from Nest's "hold until next change." The mechanism (temporary deviation with explicit resume) is established. The context (display modes instead of HVAC) is new but the mental model transfers directly.
- Idle fallback → adapted from Chromecast ambient mode. The mechanism (pleasant display when idle) is established. The context (data-driven activation based on flight availability) adds intelligence that Chromecast's timer-based approach lacks.

**Novel patterns (require user education):**
- **"Effective mode" vs. "scheduled mode" with reason** — No consumer product shows "here's what's running, here's what was scheduled, here's why they differ." This is novel for FlightWall. Education approach: the status line always shows the reason in parentheses. Users learn by reading: "Active: Clock (scheduled Departures, no flights)." No tutorial needed — the explanation is embedded in the status.
- **Schedule + data-driven fallback coexistence** — Scheduling systems set modes on a timer. FlightWall's schedule coexists with data-driven idle fallback (Clock activates when no flights are in range, regardless of schedule). This creates a priority hierarchy (schedule > idle fallback > manual override) that no single reference product demonstrates. Education approach: the schedule timeline shows the "as configured" state; the status line shows the "as running" state with reason when they differ.
- **Absent-device interaction during OTA** — No consumer product in FlightWall's category requires the user to trust a client-side timer with no server confirmation during a critical operation. The dashboard loses its data source entirely during firmware download — the user is staring at a web page with no backend. This is fundamentally different from the "software update" pattern where the device maintains a connection. Education approach: the countdown timer includes a reassurance line ("The device is installing firmware and will reconnect. This is normal.") and the post-countdown polling has three explicit states (success, rollback, unreachable) so the user never faces an ambiguous blank screen.
- **Rollback communication with triage path** — OTA rollback communication exists (UniFi, Synology) but including a triage path (view logs, suggested action) in the same notification is novel. Education approach: the amber banner IS the triage path — it reads as a narrative: "what happened → what the system did → what you can do."

### Experience Mechanics

**Scheduling Mechanics (the defining interaction):**

1. **Initiation** — User taps "Scheduling" tab adjacent to Mode Picker. Tab is always visible (not hidden in a menu). Empty state shows a 24-hour timeline bar with "Add Rule" prompt. Secondary entry point: a "Schedule this mode" action available in each mode card's settings panel bridges Casey's discovery-based mental model to the scheduling feature. After repeated manual mode switches in one session, a subtle contextual prompt in Mode Picker may also appear: "Want this to happen automatically? See Scheduling."

2. **Rule Creation:**
   - Select mode from dropdown (only registered modes appear, including contributor-added custom modes)
   - Set start time and end time with `<input type="time">`
   - Timeline updates immediately: colored segment appears at the configured times
   - Rule saves automatically on creation (no "Save" button)
   - Midnight-crossing handled natively: 23:00-07:00 wraps across the timeline with a visual indicator

3. **Visual Confirmation:**
   - 24-hour CSS Grid timeline with JS-positioned mode-colored segments
   - Current time marker ("you are here") moves across the timeline
   - Overlapping rules highlighted with amber border and resolution guidance
   - Gap periods (no scheduled mode) shown as subtle striped background (CSS `repeating-linear-gradient`) — these are when idle fallback governs

4. **Editing:**
   - Tap existing rule segment → inline editor with mode, start time, end time
   - Changes save immediately
   - Delete via trash icon with single-action confirmation
   - Most edits are time shifts (±30 min adjustments), not complete rewrites

5. **Monitoring:**
   - Dashboard status line: "Active: [Mode] ([reason])"
   - Reasons: "scheduled", "idle fallback", "manual override — schedule resumes at [time]"
   - Schedule history: last 7 transitions with timestamp, trigger source ("Weekday Morning rule" or "idle fallback: no flights"), and outcome
   - NTP sync status shown when scheduling is active

6. **Edge Cases and Recovery:**
   - **NTP unavailable during scheduled transition:** Schedule rules remain saved in NVS. The system holds the current mode until NTP re-syncs, then immediately evaluates rules against the current time and transitions to the correct mode. Dashboard shows: "Time sync lost — holding current mode until sync resumes."
   - **No schedule rules and no idle fallback configured:** The wall stays in the last manually-selected mode indefinitely. Status line shows: "Active: [Mode] (manual)." This is the Foundation-era behavior — no regression.
   - **Mode-specific settings changed between scheduled activations:** Settings persist in NVS independently of schedule rules. When a scheduled mode activates, it always uses the current saved settings for that mode. If Christian changes Departures Board "max rows" from 4 to 3, the next scheduled activation of Departures Board uses 3 rows. No ambiguity — settings and schedules are orthogonal.
   - **Mode unregistered while scheduled:** If a contributor's custom mode is removed but still has schedule rules, the system skips that rule and logs a warning. Dashboard shows the rule with an amber indicator: "Mode not available." The schedule continues with remaining rules.

**OTA Pull Mechanics (the second defining interaction):**

1. **Discovery** — Firmware card shows "v2.4.0" (current) with version comparison as the first visual element (passive discovery pattern from UniFi). "Check for Updates" button. Result: "v2.4.1 available" banner.

2. **Evaluation** — "View Release Notes" expands inline changelog. User reads at their pace. No urgency framing.

3. **Commitment** — "Update Now" button. No confirmation dialog (release notes served as informed consent). Button disables after tap to prevent double-trigger. Guard: if the user has unsaved changes in an active schedule editor, the dashboard warns: "Updating will restart the device. Save your changes first." This prevents data loss during mid-edit OTA.

4. **Execution** — ESP32 returns HTTP 202, tears down active DisplayMode, begins blocking firmware download. Dashboard shows: "Installing firmware — device unreachable during install (typically 60 seconds). This is normal." Client-side countdown timer with plain text updates (no animation). The dashboard has no backend during this phase — this is the "absent-device interaction" pattern.

5. **Resolution** — Dashboard polls /api/ota/status after countdown (3-5 second polling interval):
   - Success: green toast "Updated to v2.4.1"
   - Rollback: persistent amber banner with triage path: "Self-check failed → rolled back to v2.4.0 → view logs → try again later or report issue"
   - Unreachable after 90s: amber guidance "Device unreachable — check WiFi or try refreshing"

## Visual Design Foundation

### Color System

**Existing semantic color tokens (inherited, no changes):**
- `--color-primary`: Action buttons, active states, links
- `--color-success` / `--toast-success`: Green — confirmations, OTA success, healthy states
- `--color-warning` / `--toast-warning`: Amber — OTA rollback, NTP sync pending, schedule conflicts
- `--color-error` / `--toast-error`: Red — failures, error states
- `--color-info`: Blue — informational banners, "what's new" feature discovery

**New color tokens for Delight:**

| Token | Purpose | Derivation |
|---|---|---|
| `--mode-color-flight` | Flight modes in schedule timeline | Derived from `--color-primary` |
| `--mode-color-clock` | Clock Mode in schedule timeline | Cool neutral (blue-gray) — suggests calm, ambient |
| `--mode-color-departures` | Departures Board in schedule timeline | Warm accent — suggests activity, multiple flights |
| `--mode-color-custom` | Custom/contributed modes in timeline | Neutral — doesn't compete with core modes |
| `--timeline-bg` | Schedule timeline background | `--card-bg` with reduced opacity via `rgba()` |
| `--timeline-marker` | Current time "you are here" marker | `--color-primary` with high contrast |
| `--timeline-gap` | Unscheduled time periods | CSS `repeating-linear-gradient(45deg, transparent, transparent 4px, var(--timeline-gap-color) 4px, var(--timeline-gap-color) 5px)` — universally supported, zero asset cost |
| `--ota-pending` | OTA download in progress | `--color-info` variant |
| `--ota-countdown` | Client-side countdown text | High readability, inherits system font |
| `--version-highlight` | "v2.4.1 available" banner accent | `--color-info` — informational, not urgent |
| `--settings-panel-bg` | Mode settings expanded panel | Slightly inset from card background |
| `--settings-border` | Mode settings panel separator | Subtle border matching existing card dividers |

**Mode color assignment strategy:** Each display mode gets a unique, distinguishable color for the schedule timeline. Colors must be distinguishable at small segment sizes (minimum 30-minute rule = ~2% of timeline width). Mode colors also appear as subtle accents in Mode Picker cards to create visual continuity between "this mode" in the picker and "this time block" in the timeline.

**Mode color constraint:** With four mode colors times two display modes (normal and night-mode), all eight combinations must satisfy WCAG AA 4.5:1 contrast against the timeline background while remaining mutually distinguishable. The "cool neutral blue-gray" for Clock and "neutral" for custom modes may be difficult to satisfy simultaneously — final hex values should be tested with a contrast checker across all combinations before implementation.

**Dark mode / Night mode:** All new tokens inherit the existing night-mode CSS class toggle. New components respect the same `body.night-mode` selector. Schedule timeline colors maintain contrast ratios in both normal and night-mode states. OTA state colors (success/warning/error) are unchanged between modes — they must be recognizable regardless of ambient brightness.

### Typography System

**Existing type system (inherited, no changes):**
- System font stack: `-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif`
- No custom web fonts (zero font loading overhead, LittleFS budget preserved)
- Responsive font sizes via CSS custom properties and `clamp()` where supported

**Delight-specific typography considerations:**

- **Schedule timeline time labels:** Tabular numerals for time alignment (6:00, 12:00, 18:00). Use `font-variant-numeric: tabular-nums` on timeline axis labels — well-supported across system fonts on modern browsers.
- **Version numbers:** OTA version comparisons ("v2.4.0 → v2.4.1") use `font-variant-numeric: tabular-nums` on the inherited system font stack for digit alignment. No dedicated monospace font stack — the system font with tabular numerals achieves the alignment goal reliably across all platforms without fragile font fallback chains.
- **Status line text:** "Active: Departures Board (scheduled)" — mode name in semi-bold, reason in regular weight, parenthesized. Consistent pattern across all status displays.
- **Release notes rendering:** GitHub Release markdown rendered in-dashboard. Headings, lists, code blocks must render with existing typography. No new type styles needed — reuse existing markdown rendering patterns.
- **Schedule rule labels:** Mode name + time range ("Departures Board 8:00-18:00") must be readable at mobile widths. Truncation strategy: mode name truncates first, time range is always fully visible.

### Spacing & Layout Foundation

**Existing spacing system (inherited):**
- Base unit: 8px grid
- Card padding: 16px (2 units)
- Card gap: 12px (1.5 units)
- Section spacing: 24px (3 units)
- Touch targets: minimum 44px x 44px (iOS HIG / WCAG compliance)

**Delight-specific layout considerations:**

- **Schedule timeline dimensions:**
  - Full-width within card (minus card padding)
  - Segment area: 32px height (colored rule blocks, interactive tap targets)
  - Time label area: 20px height below segment area, outside the segment tap zone
  - Total minimum height: 56px (32px segments + 4px gap + 20px labels)
  - Time labels: 6-hour intervals (00:00, 06:00, 12:00, 18:00, 24:00) for minimal clutter
  - Current time marker: 2px wide, full segment area height, high-contrast color
  - Rule segments: minimum 30-minute visual width (~2% of timeline) to remain tappable on mobile

- **Mode settings panel:**
  - Expands within existing mode card (no separate page/modal)
  - Indented 8px from card edge to signal "child of this mode"
  - Collapsible: disclosure triangle + "Settings" label
  - Content: form elements (toggles, sliders, checkboxes) using existing form patterns
  - No max-height scroll container — panel grows with content, managed by collapse/expand. Design constraint: each mode should target 3-5 settings fields maximum. If a mode requires more, group with sub-headings. No nested scroll containers.
  - Accordion behavior: only one mode's settings panel open at a time. Opening a new panel closes the previous one. Prevents cumulative card height growth.

- **OTA Firmware card evolution:**
  - Same card dimensions as existing Firmware card
  - Version comparison: "v2.4.0 → v2.4.1" as first line (passive discovery pattern)
  - Release notes: expandable section within card (same pattern as mode settings)
  - Progress states: full-width amber/green banner at top of card during OTA

- **"What's new" banner:**
  - Full dashboard width, above first card
  - Dismissible (X button, persists across sessions via localStorage)
  - Height: single line on desktop, wraps to 2 lines on mobile
  - Links to scheduling tab, mode settings, firmware card (scroll-to anchors)

- **Loading and empty states:**
  - Schedule timeline loading: single gray bar at full timeline width, 32px height, with CSS pulse animation (opacity 0.4 to 0.7, 1.5s cycle). Respects `prefers-reduced-motion` (static gray bar instead).
  - Schedule timeline empty: "No schedule configured" centered text within timeline area. "Add first rule" action link.
  - OTA card up-to-date: "You're on the latest version (v2.4.1)" with checkmark icon. No expandable sections shown.
  - Mode settings empty: "No configurable settings for this mode" as muted text. Disclosure triangle hidden (not collapsed, hidden — no affordance for an empty panel).

**Mobile-first layout principles (continued):**
- All new components mobile-first, desktop enhanced
- Schedule timeline: horizontal scroll if timeline width exceeds viewport on very narrow screens (unlikely for 24-hour bar, but handled)
- Mode settings: full-width on mobile, may share row with mode info on desktop
- Touch targets: all interactive timeline elements (rules, time markers) meet 44px minimum
- Form inputs: native `<input type="time">` for maximum mobile keyboard compatibility

### Accessibility Considerations

**Color contrast:**
- All mode colors in schedule timeline maintain 4.5:1 contrast ratio against timeline background in both normal and night modes
- OTA state colors (success/warning/error) maintain 4.5:1 against card background
- Time labels on timeline maintain 4.5:1 against timeline background
- Current time marker maintains 3:1 contrast ratio (non-text element, WCAG 2.1 Level AA)

**Keyboard and screen reader:**
- Schedule timeline rules are focusable via keyboard (Tab to navigate, Enter to edit)
- ARIA labels on timeline: each rule segment announces "Departures Board, 8:00 AM to 6:00 PM"
- OTA progress states use `aria-live="polite"` for status updates (polling interval 3-5 seconds to avoid overwhelming screen reader users)
- Mode settings panels use `aria-expanded` for disclosure state
- "What's new" banner uses `role="banner"` with dismiss action accessible via keyboard

**Motion and transitions:**
- `prefers-reduced-motion` media query respected: disables fade transitions on the dashboard, replaces with instant state changes
- Note: LED matrix transitions (physical hardware) are not affected by this preference — they are configured separately via transition settings
- Client-side countdown timer during OTA does not use animation — plain text updates
- Loading skeleton pulse animation disabled under `prefers-reduced-motion`

**Touch targets:**
- All interactive elements minimum 44px x 44px
- Schedule timeline rule segments: tap target achieved via segment height (32px) plus full card width — 44px minimum met via the combined dimensions
- Padding around close/dismiss buttons: 8px minimum padding extends touch target beyond visual bounds

## Design Direction Decision

### Design Directions Explored

The Delight Release inherits an established visual direction from two prior releases. Rather than exploring new design directions, this step documents the existing direction and evaluates whether the Delight features require any visual evolution.

**Existing direction (Foundation + Display System):**
- Dark card-based dashboard on dark background — matches the "control room" aesthetic of monitoring flight data
- High-contrast text and controls — optimized for quick scanning during phone-on-couch interaction
- Minimal chrome — no decorative elements, borders, or backgrounds beyond functional card containers
- Semantic color — green/amber/red for system states, blue for informational, primary color for actions
- System fonts — no custom typography, fast rendering, platform-native feel
- Single-column mobile, cards-stack layout — no sidebar navigation, no tabs at dashboard level (tabs only within Mode Picker area)

**Alternative directions considered and rejected:**
1. **Light theme as default** — Rejected: dark background better matches the LED wall's aesthetic context (viewed in the same room as a glowing display). Light theme would create visual disconnect between the dashboard and the wall.
2. **Sidebar navigation** — Rejected: single-page dashboard with card stacking is simpler for ESP32 to serve and simpler for the user to scan. No page routing needed. All dashboard content fits in a single scroll.
3. **Tab-based primary navigation** — Rejected: tabs would hide Foundation/Display System sections behind navigation, breaking the "everything visible on scroll" principle. Tabs used only within Mode Picker for mode selection vs. scheduling — contained scope.

### Chosen Direction

**Continue the existing dark, card-based, single-column direction with three Delight-specific evolutions:**

1. **Schedule timeline as a new visual primitive** — The 24-hour colored timeline is the only genuinely new visual element. It introduces horizontal spatial representation (time flows left-to-right) into an otherwise vertical dashboard. This is intentional — the timeline's horizontal axis creates visual distinction from the card stack, signaling "this is different from settings."

2. **Amber as the secondary attention color** — Foundation used amber sparingly (errors only). Display System added amber for mode switching states. Delight elevates amber to a persistent UI role: OTA rollback banners, NTP sync status, schedule conflict indicators, override status. Amber becomes "the system is handling something — pay attention but don't panic."

3. **Version comparison as card header pattern** — The Firmware card's evolution to show "v2.4.0 → v2.4.1 available" as its first line establishes a new pattern: cards can lead with a comparative status rather than just a label. This pattern is specific to the Firmware card and not intended for reuse — it serves the passive discovery goal.

### Design Rationale

- **Consistency over novelty** — Users who upgraded from Foundation/Display System should feel the dashboard evolved, not changed. No visual regression, no relearning.
- **The timeline is the singular new visual bet** — All other Delight UI (settings panels, OTA states, banners) are compositions of existing patterns. The schedule timeline is the only component that introduces a new visual grammar. This concentrates design risk in one place.
- **Amber elevation is proportional to the Delight Release's scope** — Delight adds more states that require attention-without-alarm (OTA rollback, NTP pending, override active). Amber is the right color for "autonomous system reporting status" — between green (all good, ignore) and red (broken, act now).

### Implementation Approach

No HTML mockup/visualizer generated for this step — the visual direction is already implemented in the existing dashboard. The Delight implementation will extend the existing `style.css` with:
- ~12 new CSS custom properties (defined in Visual Design Foundation)
- Schedule timeline component CSS (~200 lines)
- Mode settings panel extension CSS (~50 lines)
- OTA progress state variants (~80 lines)
- "What's new" banner CSS (~30 lines)

Total new CSS: ~360 lines uncompressed, well within the gzip budget.

## User Journey Flows

### Journey 0: Casey Flashes and Discovers (First-Time Setup)

```
Entry: Casey follows published flash guide -> device boots -> connects to AP
  |
Casey connects phone to FlightWall AP -> opens 192.168.4.1
  |
Setup Wizard: WiFi credentials -> API keys -> location -> save
  |
Device reboots -> connects to home WiFi
  |
Casey tries flightwall.local in browser
  |-- SUCCESS: Dashboard loads
  |-- FAILURE: Browser shows "site can't be reached"
  |   Casey returns to flash guide -> guide includes:
  |   "If flightwall.local doesn't work, find your device's IP
  |    in your router's connected devices list, or reconnect to
  |    FlightWall AP where the portal shows the assigned IP."
  |   Casey enters IP directly -> Dashboard loads
  |
Dashboard loads -> Mode Picker shows available modes -> one is active
  |
Casey sees Live Flight Card active (default) -> wall showing flights
  |
[FIRST SUCCESS MOMENT: "It works!"]
  |
Casey explores Mode Picker -> taps Departures Board -> wall transitions (fade)
  |
[SECOND SUCCESS MOMENT: "I can control it!"]
  |
Casey notices settings gear icon on a mode card -> taps -> sees 12h/24h toggle
  |
[PROGRESSIVE DISCOVERY: "Wait, there's more"]
  |
Casey notices Scheduling tab -> opens -> sees empty timeline with "Add Rule"
  |
Creates first rule: Departures Board 8:00-18:00
  |
[THIRD SUCCESS MOMENT: "It can run itself!"]
  |
Casey scrolls to Firmware card -> sees "Check for Updates" -> understands OTA exists
  |
[FOURTH SUCCESS MOMENT: "It updates itself!"]
```

**Error paths:**
- WiFi credentials wrong -> setup wizard shows error, retry
- mDNS failure -> flash guide fallback to direct IP (most common first-time failure on non-Apple networks)
- Browser cached AP portal page after reboot -> guide instructs to close and reopen browser tab
- No flights in range -> wall shows Clock Mode (idle fallback) -> Casey learns idle fallback exists organically
- API key invalid -> dashboard shows clear error in API card with "how to get a key" link
- Device connects to WiFi but cannot reach API endpoints -> Network card shows connectivity diagnostics

**Discovery bridges:**
- Mode Picker -> Settings: gear icon visible on each mode card
- Mode Picker -> Scheduling: "Schedule this mode" action in mode settings panel
- Mode Picker -> Scheduling (structural): when a mode is manually selected, Mode Picker shows persistent subtle text below active indicator: "Manually selected · [Schedule modes instead ->]". When a mode is schedule-active: "Scheduled · Weekday Morning rule · [Edit schedule ->]"

### Journey 1: Christian Configures Mode Scheduling

```
Entry: Christian opens dashboard (habitual, phone on couch)
  |
Mode Picker visible -> current mode active -> status line shows reason
  |
Christian taps Scheduling tab
  |
Timeline shows: empty (first time) or existing rules
  |
Taps "Add Rule" -> dropdown: select mode, set start/end time
  |
Timeline updates immediately -> colored segment appears
  |
[VALIDATION: Visual timeline confirms intent]
  |
Adds second rule -> timeline checks for overlap
  |-- NO CONFLICT: Rule appears, saves immediately
  |-- OVERLAP DETECTED: Conflicting segment highlights amber
  |   Inline message: "Overlaps with [existing rule]. New rule will
  |   replace the overlapping portion."
  |   Timeline preview shows result before confirming
  |   Christian confirms -> existing rule auto-trims -> both visible
  |
Reviews: "Departures Board 7:30-22:00, Clock 22:00-7:30"
  |
Rules save automatically -> no "Save" button
  |
[SUCCESS: Schedule configured in under 2 minutes]
  |
That evening at 22:00 -> wall fades from Departures to Clock
  |
[DEFINING MOMENT: "It listened to me"]
  |
Next morning -> Christian checks dashboard
  |
Status line: "Active: Departures Board (scheduled)"
  |
Schedule history: "22:00 -> Clock (Weeknight rule), 07:30 -> Departures Board (Weekday Morning rule)"
  |
[TRUST BUILDING: Audit trail confirms automation worked]
  |
Day 7: Christian doesn't check -> wall just works
  |
[EARNED AUTONOMY: Trust is complete]
```

**Edit path:**
- Christian notices 7:30 is too early (cargo flights) -> opens Scheduling
- Taps the Departures Board rule segment on timeline
- Inline editor: changes start time from 7:30 to 8:30 (saves on blur, 300ms debounce)
- Timeline shifts immediately -> [ITERATION: Easy adjustment, schedule feels alive]

**Override path:**
- Friends are over -> Christian taps Live Flight Card in Mode Picker (overrides schedule)
- Status line: "Active: Live Flight Card (manual override — schedule resumes at 22:00)"
- Friends leave -> Christian taps "Resume Schedule" or waits for 22:00 trigger
- Schedule reclaims control -> [DELEGATION RESTORED]

### Journey 2: Christian Performs OTA Pull Update

```
Entry: Christian opens dashboard (routine visit)
  |
Firmware card shows: "v2.4.0 -> v2.4.1 available" (version comparison first)
  |
[DISCOVERY: Passive, no push notification]
  |
Christian taps "View Release Notes" -> inline changelog expands
  |
Reads: "Added clock font size option, fixed departures board refresh"
  |
[EVALUATION: Informed consent via changelog]
  |
Taps "Update Now" -> button disables
  |
Dashboard: "Installing firmware — device unreachable during install
(typically 60 seconds). This is normal."
  |
LED matrix: shows static "Updating..." screen (fade in, intentional feel)
  |
Client-side countdown: 60... 55... 50...
  |
[ABSENT-DEVICE PHASE: No backend, trust the timer]
  |
Countdown reaches 0 -> dashboard polls /api/ota/status every 3-5 seconds
  |
  |-- SUCCESS: Green toast "Updated to v2.4.1" -> flights resume
  |   [FRICTIONLESS MAINTENANCE: No USB, no compile]
  |
  |-- ROLLBACK: Amber banner with triage path
  |   [TRUST OPPORTUNITY: System caught it, previous version safe]
  |
  |-- UNREACHABLE (90s): Amber "Device unreachable — check WiFi"
      [GUIDED RECOVERY: Clear next steps]
```

**Failure recovery paths:**
- Download failed -> "Download failed — firmware unchanged. Tap to retry." (retriable)
- Integrity check failed -> "Integrity check failed — firmware unchanged. Tap to retry download." (retriable)
- Heap insufficient -> "Update Now" button disabled with tooltip: "Not enough memory. Restart device and try again." (checked via /api/health, threshold: 80KB free heap)
- GitHub unreachable -> "GitHub unreachable — try again later." (retriable later)

### Journey 3: Sam Experiences the Wall Passively

```
Entry: Sam walks into room -> wall is showing flights (Departures Board)
  |
Sam glances -> sees airline logos, flight numbers, altitudes
  |
[AMBIENT RECOGNITION: "That's cool, is that real-time?"]
  |
Planes leave the area -> wall fades to Clock Mode
  |
[SMOOTH TRANSITION: Feels intentional, not broken]
  |
Sam glances again -> sees clean time display
  |
[AMBIENT UTILITY: Wall is useful even without flights]
  |
New flight enters range -> wall fades back to flight display
  |
[DATA-DRIVEN INTELLIGENCE: Wall responds to real-world events]
  |
Over days -> Sam habituates -> glances at wall for time or flight info
  |
[AMBIENT TRUST: Wall becomes trusted background information source]
  |
One day -> Sam recognizes a friend's flight route on Departures Board
  |
[SERENDIPITOUS DELIGHT: "Hey, that's the flight to Denver!"]
```

**Sam's OTA experience (when Christian triggers update while Sam is present):**
- Wall shows flights -> brief fade -> wall shows "Updating..." with pulsing dot animation
- Sam's interpretation: "Hmm, something's happening"
- 60 seconds later -> wall fades back to flights (or Clock if no flights)
- Sam's interpretation: "Okay, it's back"
- Design requirement: the "Updating..." screen uses the same fade transition entering and exiting, making it feel as intentional as a mode change. Simple pulsing dot animation, not a text-heavy progress screen. Sam has no context for percentages.

**Sam never experiences:**
- Error messages (Clock Mode fills dead air, API failures show last good data)
- Loading states (transitions smooth over data fetches)
- "Is it broken?" moments (transitions are always intentional, including OTA)
- Dashboard interaction (Sam's UX is entirely the physical display)

### Journey 4: Jordan Contributes a Custom Display Mode

```
Entry: Jordan finds TheFlightWall repo on GitHub -> reads CONTRIBUTING.md
  |
Reads architecture docs -> understands DisplayMode interface
  |
Creates new mode: WeatherOverlayMode (hypothetical)
  |
Registers mode in mode registry -> builds locally -> tests
  |
Mode appears in Mode Picker automatically -> scheduling includes it
  |
[EXTENSION POINT VALIDATION: Plugin architecture works]
  |
Jordan's mode-specific settings appear in settings panel
  |
[INTEGRATION DEPTH: Custom mode is a first-class citizen]
  |
Jordan opens PR -> review -> merged -> tagged in GitHub Release
  |
[CONTRIBUTOR RECOGNITION]
  |
Christian's device pulls the release via OTA -> Jordan's mode is now live
  |
[RELEASE PIPELINE COMPLETION: Code reached hardware]
```

**Jordan's touchpoints with Delight UX:**
- Mode registration -> mode appears in scheduler dropdown and Mode Picker (automatic)
- Mode settings -> custom mode can register its own settings (rendered dynamically in settings panel)
- Mode color -> assigned from fallback palette (cycle through 5 predefined accent colors for unrecognized modes)
- OTA -> Jordan's code distributed via the same update mechanism
- Jordan has no visibility into individual device updates — this is by design

### Journey 5: Christian's Device Recovers from Power Loss

```
Entry: Power restored after outage -> device boots -> connects to WiFi
  |
NTP sync -> clock corrects -> schedule evaluates current time
  |
Correct mode activates based on schedule (no manual intervention)
  |
Christian notices wall is back -> opens dashboard (curiosity, not necessity)
  |
Status line: "Active: Departures Board (scheduled — Weekday Morning rule)"
  |
No indication of outage on main dashboard (wall "just works")
  |
System section (scrolled, not prominent): "Last boot: 12 minutes ago.
  Uptime before: 14 days. Schedule resumed automatically."
  |
[TRUST CONFIRMATION: Device remembered and recovered without help]
```

This journey validates the "earned autonomy" terminal state from Journey 1. If the device forgets its schedule after power loss, trust resets to zero. NVS persistence ensures it remembers.

### Journey Patterns

**Common patterns across all journeys:**

1. **Status line as universal confirmation** — Every journey ends with the user being able to verify state via the dashboard status line: "Active: [Mode] ([reason])." This is the single most important UI element across all journeys.

2. **Progressive escalation of trust** — Casey trusts "it shows flights" (mode works). Christian trusts "it follows my schedule" (automation works). Sam trusts "it always shows something" (availability works). Each persona's trust ceiling is different, but the escalation pattern is the same: see it work once -> see it work repeatedly -> stop checking.

3. **Failure as trust accelerator** — Journey 2's OTA failure paths and Journey 5's power recovery are designed to build more trust than success paths. "Rolled back to v2.4.0" tells the user the system has self-healing capability. Graceful failure advances trust faster than perfect operation.

4. **Absent-interaction success** — Journey 3 (Sam) and the terminal state of Journey 1 (Christian stops checking) both measure success by the absence of interaction. The wall succeeds when people stop thinking about it.

### Flow Optimization Principles

1. **Minimize steps to value** — Casey's Journey 0 reaches "It works!" in 5 steps. Scheduling adds 3 more. OTA is 3 steps. Recovery (Journey 5) is zero steps — the wall recovers autonomously.

2. **Every error message is a mini-journey** — Per FR30, error messages contain three elements: what failed, what happened, what to do. Each error message is a complete recovery journey in one sentence.

3. **Visual timeline as journey compression** — The schedule timeline compresses Journey 1's temporal experience into a single visual element: past (history), present (time marker), future (configured rules).

4. **Dual-channel confirmation** — LED matrix and dashboard independently confirm every state change. During OTA, the dashboard carries the full burden alone.

5. **Discovery bridges replace tutorials** — Structural bridges (persistent contextual text in Mode Picker, gear icons, "Schedule this mode" links) connect features through interaction, not instruction. No behavioral tracking or session state needed.

## Component Strategy

### Existing Design System Components

**Inherited from Foundation/Display System (no changes needed):**

| Component | Usage | Pattern |
|---|---|---|
| Card | Container for all dashboard sections | Consistent padding, border, shadow |
| Button (Primary) | Main actions: "Activate", "Update Now", "Add Rule" | Solid fill, high contrast |
| Button (Secondary) | Supporting: "Check for Updates", "View Release Notes" | Outline, lower visual weight |
| Button (Danger) | Destructive: "Delete Rule", "Reset to Defaults" | Red variant, single-action confirm |
| Toast (Success) | Confirmations: "Updated to v2.4.1", "Rule saved" | Green, auto-dismiss 5s |
| Toast (Warning) | Alerts: "Rolled back to v2.4.0", "NTP sync pending" | Amber, persistent |
| Toast (Error) | Failures: "Download failed", "GitHub unreachable" | Red, persistent |
| Input (Text/Select/Toggle) | Various settings inputs | Standard patterns |
| Status Badge | Active mode indicator | Green dot + "Active" label |
| Disclosure Triangle | Expandable sections | Chevron rotation |
| Toast (Info) | "What's new" feature links | Blue, dismiss with X |

### New Custom Components (Delight Release)

#### Mode Picker

**Purpose:** Primary navigation surface for viewing, activating, and managing display modes. This component is new to the Delight Release — it provides the visual inventory of available modes and serves as the hub for mode activation, settings access, and scheduling entry.

**Anatomy:**
- Container: card-width, positioned second in dashboard card stack (below Display card)
- Tab bar: horizontal tabs (Modes | Scheduling) fixed at top
- Mode cards: one per registered mode, stacked vertically within the Modes tab
- Active indicator: status badge on the currently active mode card
- Status line: "Active: [Mode] ([reason])" at the top of the container, always visible regardless of active tab

**States:**
- Loading: skeleton cards (CSS pulse animation)
- Populated: mode cards with names, active indicator, settings gear icon
- Mode switching: brief spinner on the activated card's button (confirmed update, ~200ms)
- Override active: status line shows "Manual override — schedule resumes at [time]"

**Per-mode card content:**
- Mode name + description
- "Activate" button (primary, disabled if already active)
- Gear icon -> expands Mode Settings Panel
- Structural bridge text: "Manually selected · Schedule modes instead ->" (when manual) or "Scheduled · [rule name] · Edit schedule ->" (when scheduled)

#### Schedule Timeline

**Purpose:** Visualize 24-hour schedule as colored time blocks.

**Anatomy:**
- Container: full-width within card, 56px total height (32px segments + 4px gap + 20px labels)
- Segment area: CSS Grid/flexbox with percentage-width colored blocks per rule
- Time axis: labels at 00:00, 06:00, 12:00, 18:00, 24:00
- Time marker: 2px-wide high-contrast line at current time
- Gap regions: CSS `repeating-linear-gradient` diagonal stripe

**States:**
- Empty (no rules): "No schedule configured" + "Add first rule" link
- Empty (scheduling disabled): muted/grayed timeline with "Scheduling is paused" + toggle to re-enable (FR38)
- Loading: gray pulsing bar (respects `prefers-reduced-motion`)
- Populated: mode-colored segments positioned by time range
- Conflict: overlapping segments highlighted amber + inline resolution text
- NTP unavailable: amber overlay: "Time sync unavailable — schedule saved but inactive"
- Stale time: amber indicator if NTP last synced >4 hours ago

**Mode-to-color mapping:** Each mode has a CSS custom property (`--mode-color-clock`, `--mode-color-departures`, `--mode-color-liveflight`). Custom/unrecognized modes cycle through 5 predefined accent colors from a fallback palette.

**Interaction:**
- Tap segment -> inline editor opens
- Tap gap -> "Add Rule" with time pre-filled to tapped position
- Keyboard: Tab through segments, Enter to edit
- ARIA: each segment announces "Departures Board, 8:00 AM to 6:00 PM"

#### Schedule Rule Editor (Inline)

**Purpose:** Create and edit individual schedule rules within the timeline context.

**Anatomy:**
- Mode selector: native `<select>` with all registered modes
- Start/end time: native `<input type="time">` (step="900" as hint; any minute value accepted)
- Delete button: trash icon with inline undo (5s timer)
- Enable/disable toggle: per-rule on/off (FR39)

**States:**
- Creating: empty fields, "Add Rule" submit
- Editing: populated from existing rule
- Conflict: amber border + resolution guidance
- Midnight-crossing: visual indicator when end < start
- Saving: brief spinner on changed field

**Save strategy:** Save on field blur with 300ms debounce. Show brief saving indicator. If API returns conflict, revert field and show conflict state. If device unreachable during save, show "Unable to save — check connection" with retry.

#### Mode Settings Panel

**Purpose:** Per-mode configuration accessible from each mode card.

**Anatomy:**
- Trigger: gear icon + "Settings" on mode card
- Container: indented 8px, accordion (one open at a time)
- Content: mode-specific form elements
- Footer: "Reset to Defaults" (danger) + "Schedule this mode" link

**States:**
- Collapsed: gear icon visible
- Expanded: form elements visible, other panels auto-collapse
- Empty: "No configurable settings" muted text, gear icon hidden
- Saving: brief spinner on changed field
- Mode unavailable: "Requires [dependency] to be configured" with link to relevant settings

**Per-mode content:**
- Clock Mode: 12h/24h toggle
- Departures Board: max rows slider (1-4), telemetry field checkboxes
- Live Flight Card: telemetry field checkboxes
- All modes: transition speed select (Fast / Normal / Slow / Off) — satisfies FR29
- Custom modes: settings registered via mode API, rendered dynamically

#### OTA Progress Display

**Purpose:** Firmware update lifecycle from discovery through resolution.

**States:**
- Up-to-date: "You're on the latest version (v2.4.1)" + checkmark
- Update available: version comparison + "View Release Notes" + "Update Now"
- Heap guard: "Update Now" disabled with tooltip when free heap < 80KB (checked via /api/health)
- Downloading: "Installing... device unreachable (typically 60s). This is normal." + countdown
- Polling: "Reconnecting..." after countdown
- Success: green toast
- Rollback: persistent amber banner with triage
- Unreachable: persistent amber banner
- Error: persistent red banner per failure type

#### "What's New" Feature Discovery Banner

**Purpose:** One-time notification after upgrade highlighting Delight capabilities.

**States:**
- Visible: first dashboard load after OTA upgrade (not fresh install)
- Dismissed: permanently hidden via localStorage
- Auto-dismissed: after any feature link clicked

### Component Implementation Strategy

**Build order (dependency-driven):**

1. **Phase 1 — Mode Picker + Core Scheduling:** Mode Picker (new), Schedule Timeline, Rule Editor. Mode Picker is the primary navigation surface and a dependency for Phases 2-4.

2. **Phase 2 — Mode enhancement:** Mode Settings Panel (extends Mode Picker cards), including transition speed setting (FR29).

3. **Phase 3 — OTA experience:** OTA Progress Display (Firmware card evolution).

4. **Phase 4 — Polish:** "What's New" Banner.

### Implementation Roadmap

| Phase | Components | Dependency | Est. Size |
|---|---|---|---|
| 1 | Mode Picker + Schedule Timeline + Rule Editor | Mode Switch API + Schedule CRUD API | ~2.5KB gz |
| 2 | Mode Settings Panel + Transition Settings | Mode registration API | ~0.8KB gz |
| 3 | OTA Progress Display | OTA Pull API + GitHub | ~0.8KB gz |
| 4 | "What's New" Banner | OTA functional | ~0.2KB gz |
| **Total** | | | **~4.3KB gz** |

**Web asset budget:** Total compressed web assets (all files in `firmware/data/`) must not exceed 50KB. Current baseline is ~8.3KB. This leaves headroom for all Delight components plus future additions. If assets approach 40KB, evaluate lazy loading or server-side generation.

## UX Consistency Patterns

### Button Hierarchy

**Primary actions (one per context):**
- "Activate" (Mode Picker) — switches the wall immediately
- "Update Now" (Firmware card) — initiates OTA download
- "Add Rule" (Scheduling) — creates a new schedule rule

**Secondary actions:**
- "Check for Updates" — queries GitHub, no state change
- "View Release Notes" — expands changelog
- "Resume Schedule" — returns to scheduled behavior after override

**Danger actions:**
- "Delete Rule" — removes schedule rule (inline confirm with 5s undo timer)
- "Reset to Defaults" — reverts mode settings (shows before/after preview)

**Button behavior rules:**
- Only ONE primary action visible per card/context at a time
- Primary: solid fill; Secondary: outline; Danger: red variant
- Irreversible actions use inline confirmation with undo timer (not modal dialogs)
- Long-running operations (OTA, Check for Updates) disable after first tap with loading indicator
- No "Save" buttons anywhere in Delight UI — all changes auto-save (explicit design decision)

### Feedback Patterns

**Ephemeral (auto-dismiss):**
- Success toast: "Rule saved", "Updated to v2.4.1" — green, 5s
- Info toast: "Schedule takes effect at next trigger" — blue, 5s

**Persistent (user-dismiss):**
- Warning banner: OTA rollback, NTP pending, schedule conflict — amber
- Error banner: OTA failed, GitHub unreachable — red, with "Retry"
- "What's new" banner: feature discovery — blue, dismiss permanently

**Inline (always visible while relevant):**
- Status line: "Active: Departures Board (scheduled)"
- Override indicator: "Manual override — schedule resumes at 22:00"
- NTP status: "Time sync unavailable — schedule saved but inactive"
- Version comparison: "v2.4.0 -> v2.4.1 available"
- Schedule conflict: amber highlight on overlapping timeline segments

**Escalation model:**
1. Inline (always visible, no action) -> 2. Ephemeral (auto-clears) -> 3. Persistent (user-dismissed) -> 4. Blocking (prevents action: OTA download, heap insufficient with disabled button + tooltip)

**Rules:**
- Max 2 toasts stacked simultaneously
- Persistent banners stack vertically above cards, most recent on top
- All feedback: plain language, never error codes
- All errors: three elements per FR30 (what failed, what happened, what to do)
- Heap-guard blocking: "Update Now" disabled when free heap < 80KB, tooltip explains

### Form Patterns

**Auto-save everywhere:**
- Schedule rules: save on field blur, 300ms debounce
- Mode settings: save on toggle/slider/checkbox change (optimistic)
- WiFi/API settings: save on Apply (reboot-required exception)

**Input patterns:**
- Time: native `<input type="time">` with `step="900"` hint (15-min increments suggested, any minute value accepted)
- Mode selection: native `<select>` with all registered modes
- Toggles: on/off switch for binary settings
- Sliders: range input for numeric (max rows 1-4)
- Checkboxes: multi-select for telemetry fields

**Validation:**
- Inline only (no modal dialogs)
- Schedule overlap: immediate amber highlight + resolution text
- Midnight-crossing: interpreted automatically, not flagged as error
- Zero-duration rules (start == end): rejected with inline "Start and end times must differ"
- Invalid API response: error text below field

**Form layout:**
- Labels above inputs (mobile-first)
- One input per row on mobile
- Logical grouping of related settings
- No required fields (all settings have sensible defaults)

### Navigation Patterns

**Single-page dashboard (continued):**
- No page routing, no client-side navigation
- Scroll-to-anchor for cross-references

**Within-card navigation:**
- Mode Picker tabs: Modes | Scheduling (horizontal)
- Expandable sections: disclosure triangle
- Accordion: one mode settings panel open at a time

**Hierarchy:**
1. Scroll position (primary)
2. Tabs (secondary, within Mode Picker)
3. Expand/collapse (tertiary)

**Rules:**
- No hamburger, sidebar, or top nav
- Tab state persisted in URL hash (#scheduling)
- Expand state NOT persisted (collapsed on load)
- Smooth scroll-to-anchor (respects `prefers-reduced-motion`)

### State Management Patterns

**State sources:**
- API responses (page load + after user actions)
- localStorage (banner dismiss, active tab)
- Client-side timer (OTA countdown only)

**Rules:**
- Fetch on page load — never assume stale state is current
- Fetch after user actions that change server state
- Never poll in background (ESPAsyncWebServer constraint)
- localStorage for UI-only state, never for data

**Optimistic vs. confirmed:**
- Mode activation: **confirmed** (button shows spinner, waits for API ~200ms, then updates status line. Avoids confusing revert on failure.)
- Schedule rule save: confirmed (waits for API, shows saved state)
- OTA trigger: confirmed (shows "Installing..." only after HTTP 202)
- Mode settings: optimistic (toggle flips immediately, reverts on failure — less jarring for input elements)

## Responsive Design & Accessibility

### Responsive Strategy

**Primary context: phone, one-handed, on the couch, looking at the LED wall across the room.**

**Mobile (320px - 767px) — primary target:**
- Single-column card stack, full width minus padding
- All form elements full-width
- Touch targets 44px minimum
- Schedule timeline full-width within card
- Mode Picker: mode cards stack vertically

**Tablet/Desktop (768px+) — enhanced:**
- Two-column layout: Mode Picker + Scheduling side by side
- Settings cards in 2-column grid
- Schedule timeline: more horizontal resolution
- Mode settings: may show alongside mode info

### Breakpoint Strategy

**Single breakpoint (inherited, no changes):**
- Mobile: max-width 767px (single column)
- Enhanced: min-width 768px (two-column where beneficial)

No additional breakpoints. All components designed to work at 320px and scale up.

**Component-specific responsive behavior:**
- Schedule timeline: at 320px, 30-minute segment is ~7px wide. Mode abbreviation (3 chars) on segments wider than 40px; color-only for narrower. Full segment height (32px) provides tappable target regardless.
- Mode settings panel: same layout all widths
- OTA progress: same layout all widths
- "What's new" banner: single line at 768px+, wraps to 2 lines on mobile

### Accessibility Strategy

**WCAG 2.1 Level AA compliance target.**

1. **Color is never the only indicator** — Timeline segments have text labels; OTA states have descriptive text. No information by color alone.

2. **Keyboard navigability** — All interactive elements reachable via Tab. Timeline segments focusable. Settings panels via Enter. Focus order = visual order.

3. **Screen reader** — ARIA labels on custom components. `aria-live="polite"` on OTA status and schedule confirmations. `aria-expanded` on disclosures. Timeline segments announce mode + time range. Note: aria-live regions only update on user-initiated actions (no background polling), which is correct behavior.

4. **Motion control** — `prefers-reduced-motion` respected for all CSS transitions/animations. Dashboard fades become instant. LED matrix transitions are separate.

5. **Touch targets** — 44px minimum on all interactive elements. Extended hit areas via padding.

### Testing Strategy

**Responsive:** iPhone SE (375px), iPhone 14 (390px), iPad (768px), Desktop (1280px). Chrome + Safari (iOS required) + Firefox.

**Accessibility:** Lighthouse 95+ target (validate early — gzipped ESP32 serving may need header tuning). VoiceOver navigation test. Keyboard-only interaction test. Color contrast checker for all mode/state colors.

**No automated CI** — single-developer project. Manual testing + Lighthouse audit before each release.

### Implementation Guidelines

**Responsive CSS:**
- Mobile-first: base styles are mobile, `@media (min-width: 768px)` for enhanced
- `rem` for spacing, `%` for widths, `px` for borders
- No `vw`/`vh` on form elements (iOS Safari zoom issues)
- `clamp()` for font sizes where supported

**Accessibility:**
- Semantic HTML: `<button>` for actions, `<a>` for navigation, `<input>` for forms
- `role` attributes only when semantic HTML insufficient
- `aria-label` on icon-only buttons (trash, gear, dismiss X)
- `tabindex="0"` on custom interactive elements
- `aria-live="polite"` on status regions
- No skip link needed (simple single-page structure)


]]></file>
<file id="893ad01d" path="_bmad-output/planning-artifacts/architecture.md" label="ARCHITECTURE"><![CDATA[

# Architecture Decision Document — The FlightWall OSS Project

## Project Context Analysis

### Requirements Overview

**Functional Requirements:** 48 FRs across 9 groups (Device Setup, Configuration Management, Display Calibration, Flight Data Display, Airline Logo Display, Responsive Layout, Logo Management, System Operations).

**Non-Functional Requirements:** Hot-reload latency <1s, page load <3s, 30+ days uptime, WiFi recovery <60s, RAM ceiling <280KB, flash budget 4MB total with 500KB headroom, concurrent operations across dual cores.

**UX Specification:** Two HTML pages (wizard + dashboard), single settings API endpoint, gzipped static assets, Leaflet lazy-load, mDNS, WiFi scan, toast notifications with hot-reload, triple feedback (browser/LEDs), RGB565 logo preview.

### Scale & Complexity

Medium IoT/Embedded with web UI. 6 new architectural components (ConfigManager, WebPortal, WiFiManager, LogoManager, LayoutEngine, DisplayTask). 5 existing components preserved (FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, NeoMatrixDisplay).

### Technical Constraints

ESP32 single WiFi radio (AP_STA mode required), 4MB flash total (2MB app + 2MB LittleFS), 520KB SRAM (~280KB usable), FastLED disables interrupts (display on Core 0, WiFi/web on Core 1), ESPAsyncWebServer 2-3 concurrent clients max, ArduinoJson streaming/filter API for heap pressure, NVS ~100K write cycles per key.

## Technology Stack

**Existing (Locked):** C++ Arduino, ESP32, PlatformIO, FastLED ^3.6.0, Adafruit GFX ^1.11.9, FastLED NeoMatrix ^1.2, ArduinoJson ^7.4.2, WiFi + HTTPClient (ESP32 core), TLS (WiFiClientSecure).

**New Dependencies:** ESPAsyncWebServer (mathieucarbou fork ^3.6.0 — actively maintained, fixes memory leaks), AsyncTCP (mathieucarbou fork — fixes race conditions), LittleFS (ESP32 core, replaces SPIFFS), ESPmDNS (ESP32 core), Leaflet.js ~1.9 (lazy-loaded client-side).

**PlatformIO additions:**
```ini
lib_deps =
    mathieucarbou/ESPAsyncWebServer @ ^3.6.0
    fastled/FastLED @ ^3.6.0
    adafruit/Adafruit GFX Library @ ^1.11.9
    marcmerlin/FastLED NeoMatrix @ ^1.2
    bblanchon/ArduinoJson @ ^7.4.2
board_build.filesystem = littlefs
board_build.partitions = custom_partitions.csv
```

**Flash Partition Layout:** Custom `firmware/custom_partitions.csv` — 20KB NVS + 8KB OTA + 2MB app0 + ~1.9MB spiffs (LittleFS).

## Core Architectural Decisions

### Decision 1: ConfigManager — Singleton with Category Struct Getters

Centralized singleton, initialized first. Config values grouped into category structs. Config value chain: compile-time default → NVS stored value → RAM cache (struct fields).

**5 category structs:** DisplayConfig (brightness, text colors), LocationConfig (center, radius), HardwareConfig (tiles, pins, wiring), TimingConfig (fetch/display intervals), NetworkConfig (WiFi, API keys).

**NVS Write Debouncing:** Hot-reload keys update RAM instantly; NVS write debounced to 2-second quiet period. Reboot-required keys persist immediately.

**Reboot-required keys:** `wifi_ssid`, `wifi_password`, `opensky_client_id`, `opensky_client_secret`, `aeroapi_key`, `display_pin`.

**Hot-reload keys:** All display, location, hardware timing settings.

### Decision 2: Inter-Task Communication — Hybrid

**Config changes:** Atomic flag signals display task to re-read from ConfigManager.

**Flight data:** FreeRTOS queue with overwrite semantics — `xQueueOverwrite()` ensures display always gets latest data.

Atomic flags for lightweight signals. Queues for proper thread-safe data transfer.

### Decision 3: WiFi State Machine — WiFiManager

**States:** WIFI_AP_SETUP → CONNECTING → STA_CONNECTED, with fallback to AP_FALLBACK and reconnect states.

**GPIO overrides:** Short press shows IP on LEDs; long press during boot forces AP setup.

**Callbacks:** WebPortal, display task, FlightDataFetcher register for state change notifications.

### Decision 4: Web API Endpoints

11 REST endpoints: `GET /` (wizard/dashboard), `GET /api/status` (health), `GET/POST /api/settings` (config), `GET /api/wifi/scan` (async scan), `POST /api/reboot`, `POST /api/reset`, `GET /api/logos`, `POST/DELETE /api/logos/:name`, `GET /api/layout` (zones).

**Response envelope:** `{ ok: true/false, data: {...}, error: "message", code: "CODE" }`.

### Decision 5: Component Integration

**Dependency graph:** ConfigManager ← all; SystemStatus ← all; FlightDataFetcher (Core 1) → display task (Core 0) via atomic + queue.

**Initialization order:** ConfigManager → SystemStatus → LogoManager → LayoutEngine → WiFiManager → WebPortal → FlightDataFetcher → displayTask on Core 0.

**Rules:** ConfigManager initialized first; DisplayTask read-only; WebPortal only write path from browser; existing adapters preserve interface contracts.

### Decision 6: Error Handling — SystemStatus Registry

Centralized subsystem health tracking (WiFi, OpenSky, AeroAPI, CDN, NVS, LittleFS) with human-readable status strings. API call counter (RAM + hourly NVS sync, resets monthly via NTP).

## Implementation Patterns

### Naming Conventions

**C++ Classes:** PascalCase. **Methods:** camelCase. **Member Variables:** _camelCase. **Structs:** PascalCase name, snake_case fields. **Constants:** UPPER_SNAKE_CASE. **Files:** PascalCase.h/.cpp with `#pragma once`.

### Config Structs — All Fields snake_case

Struct fields use `snake_case` to match NVS keys and JSON field names — single convention across all three layers.

### NVS Keys & Abbreviations

15-char limit. Table includes: `brightness`, `text_color_{r,g,b}`, `center_lat`, `center_lon`, `radius_km`, `tiles_{x,y}`, `tile_pixels`, `display_pin`, `origin_corner`, `scan_dir`, `zigzag`, `fetch_interval`, `display_cycle`, `wifi_ssid`, `wifi_password`, `os_client_id`, `os_client_sec`, `aeroapi_key`, `api_calls`, `api_month`. Abbreviations documented in ConfigManager.h.

### Logging Pattern

**File:** `utils/Log.h`. Compile-time log levels: `LOG_E`, `LOG_I`, `LOG_V`. Macros compile to nothing at level 0.

### Error Handling Pattern

Boolean return + output parameter (matches existing codebase). Plus SystemStatus reporting and log macros.

### Zone Calculation Algorithm (C++ + JavaScript Parity)

Shared identical implementation in LayoutEngine.h and dashboard.js. Test vectors validate: 160x32 full mode, 80x32 full mode, 192x48 expanded mode, 160x16 compact mode.

### Web Asset Patterns

All `fetch()` calls check `json.ok` and call `showToast()` on failure. Mandatory pattern for every API interaction.

## Project Structure

### Directory Layout

```
firmware/
├── platformio.ini
├── custom_partitions.csv
├── src/main.cpp
├── core/ (ConfigManager, FlightDataFetcher, LayoutEngine, LogoManager, SystemStatus)
├── adapters/ (OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, NeoMatrixDisplay, WebPortal, WiFiManager)
├── interfaces/ (BaseDisplay, BaseFlightFetcher, BaseStateVectorFetcher)
├── models/ (FlightInfo, StateVector, AirportInfo)
├── config/ (APIConfiguration, WiFiConfiguration, UserConfiguration, HardwareConfiguration, TimingConfiguration)
├── utils/ (GeoUtils, Log)
├── data/ (LittleFS content — gzipped HTML/JS/CSS, Leaflet, logos)
└── test/ (unit tests for ConfigManager, LayoutEngine)
```

### Requirements to Structure Mapping

Device Setup/Onboarding → WiFiManager, WebPortal, wizard HTML/JS. Configuration Management → ConfigManager, WebPortal, dashboard HTML/JS. Display Calibration → WebPortal, LayoutEngine, dashboard.js. Flight Data → FlightDataFetcher, NeoMatrixDisplay. Airline Logo → LogoManager, NeoMatrixDisplay. Responsive Layout → LayoutEngine, NeoMatrixDisplay. Logo Management → LogoManager, WebPortal, dashboard.js. System Operations → SystemStatus, WiFiManager, main.cpp.

## Enforcement Guidelines

All new code must:

1. Follow naming conventions (PascalCase classes, camelCase methods, _camelCase private vars, snake_case struct fields)
2. Use `#pragma once` in all headers
3. Use boolean return + output parameter for fallible operations
4. Report user-visible errors via `SystemStatus::set()`
5. Log via `LOG_E/I/V` from `utils/Log.h`
6. Use `String` for dynamic text, `const char*` for constants
7. Stream LittleFS files — never buffer entire files in RAM
8. Place classes in: `core/` (internal), `adapters/` (hardware/network), `interfaces/`, `models/`, `utils/`
9. Every JS `fetch()` must check `json.ok` + call `showToast()` on error
10. Document NVS key abbreviations if key exceeds 15 characters
11. No `new`/`delete` — use automatic storage; stream LittleFS files

## Architecture Validation

### Coherence ✅

All technology choices compatible on ESP32 Arduino. `snake_case` flows consistently through struct fields → NVS keys → JSON fields. Logging, error handling, include patterns are uniform.

### Requirements Coverage ✅

All 48 FRs mapped to architectural components. All NFRs addressed (hot-reload <1s, page load <3s, uptime 30+ days, WiFi recovery <60s, RAM <280KB, flash 4MB with headroom).

### Implementation Readiness ✅

6 core decisions with code examples, interfaces, rationale. 11 enforcement guidelines. NVS key table complete with abbreviations. API endpoint table with response formats. Shared zone algorithm with test vectors. Complete directory tree (44 files total, all EXISTING/NEW/UPDATED specified).

---

# Foundation Release — Architecture Extension

_Extends MVP architecture with OTA firmware updates, night mode/brightness scheduling, and onboarding polish. All MVP decisions remain in effect._

## Foundation Requirements Overview

**37 FRs across 6 groups:** OTA Firmware Updates (FR1-FR11), Settings Migration (FR12-FR14), Night Mode & Brightness Scheduling (FR15-FR24), Onboarding Polish (FR25-FR29), Dashboard UI (FR30-FR34), System & Resilience (FR35-FR37).

**NFRs:** OTA upload ≤30s, NTP sync ≤10s, brightness hot-reload ≤1s, firmware ≤1.5MB, LittleFS ≥500KB free, self-check ≤30s, 30+ days uptime, night mode ≤1% loop impact.

## Foundation Decisions

### Decision F1: Partition Table — Dual OTA Layout

**Updated `custom_partitions.csv`:** 20KB NVS + 8KB OTA + 1.5MB app0 + 1.5MB app1 + 960KB spiffs (LittleFS).

LittleFS shrinks from ~1.9MB to ~960KB (56% reduction). Breaking change — one-time USB reflash required, erases NVS + LittleFS. Settings export mitigates reconfiguration.

**Flash budget (Foundation):** 1.5MB firmware + 1.5MB OTA staging. LittleFS: ~80KB web assets + ~198KB logos (~2KB each) + ~682KB free.

### Decision F2: OTA Handler Architecture — WebPortal + main.cpp

No new class. OTA split across two locations:

| Concern | Location | Implementation |
|---------|----------|---------------|
| Upload endpoint | WebPortal.cpp | POST handler with stream-to-partition |
| Binary validation | WebPortal.cpp | Magic byte + size check |
| Stream-to-flash | WebPortal.cpp | `Update.write()` per chunk |
| Abort on failure | WebPortal.cpp | `Update.abort()` |
| Progress reporting | WebPortal.cpp | Bytes written / total |
| Reboot | WebPortal.cpp | `ESP.restart()` after success |
| Self-check | main.cpp | Boot sequence after init |
| Rollback detection | main.cpp | `esp_ota_get_last_invalid_partition()` |
| Version string | platformio.ini | `-D FW_VERSION=\"1.0.0\"` |

**Upload handler flow:** First chunk validates magic byte 0xE9 and calls `Update.begin()`. Per chunk: `Update.write()` validates write success. Final chunk: `Update.end(true)` finalizes and validates partition. On error anywhere: `Update.abort()`, return error JSON.

### Decision F3: OTA Self-Check — WiFi-OR-Timeout Strategy

Boot after all init: if WiFi connects OR 60-second timeout expires — whichever comes first — mark firmware valid. No self-HTTP-request. No web server check.

**Rationale:** WiFi proves network stack works. Timeout prevents false rollback on temporary outages. Crash-on-boot caught by watchdog regardless.

**Timeout budget:** Good firmware + WiFi = 5-15s, Good firmware no WiFi = 60s, Bad firmware = watchdog rollback.

### Decision F4: IANA-to-POSIX Timezone Mapping — Browser-Side JS

Browser converts IANA timezone to POSIX string client-side via TZ_MAP object (~50-80 entries). ESP32 stores and uses POSIX string. Zero firmware/LittleFS overhead.

**Wizard flow:** Auto-detect via `Intl.DateTimeFormat().resolvedOptions().timeZone`, show dropdown with IANA names pre-selected, send POSIX string to ESP32, fallback to manual POSIX entry if unknown.

### Decision F5: Night Mode Scheduler — Non-Blocking Main Loop

Brightness schedule check runs in Core 1 main loop (1-second interval). Non-blocking time check via `getLocalTime()` with `timeout=0`. Schedule times stored as minutes since midnight (uint16, 0-1439). Midnight-crossing logic: if dimStart ≤ dimEnd, same-day window; else midnight crossing.

**Interaction with manual brightness:** When scheduled and in dim window, scheduler overrides brightness. Dashboard indicates when schedule is active.

**NTP re-sync:** ESP32 LWIP defaults to 1-hour re-sync. Sufficient for <5 ppm RTC drift (~0.4s/day).

### Decision F6: ConfigManager Expansion — 5 New NVS Keys

| NVS Key | Type | Default | Category |
|---------|------|---------|----------|
| `timezone` | string | "UTC0" | schedule |
| `sched_enabled` | uint8 | 0 | schedule |
| `sched_dim_start` | uint16 | 1380 | schedule |
| `sched_dim_end` | uint16 | 420 | schedule |
| `sched_dim_brt` | uint8 | 10 | schedule |

New `ScheduleConfig` struct with all fields. 5 new hot-reload keys — no reboot required. Timezone change calls `configTzTime()` immediately.

### Decision F7: API Endpoint Additions

2 new endpoints: `POST /api/ota/upload` (multipart firmware), `GET /api/settings/export` (JSON download).

Updated `/api/status` response: add `firmware_version`, `rollback_detected`, `ntp_synced`, `schedule_active`, `uptime_seconds`.

## Foundation Patterns & Enforcement

### Rules 12-16 (Foundation-Specific)

12. OTA upload validates before writing — size + magic byte on first chunk, fail-fast with JSON error
13. OTA upload streams directly via `Update.write()` per chunk — never buffer full binary in RAM
14. `getLocalTime()` always uses `timeout=0` (non-blocking) — never block main loop
15. Schedule times stored as `uint16` minutes since midnight (0-1439) — no string time representations
16. Settings import client-side only — no server-side import endpoint

## Foundation Implementation Sequence

**Sequential foundation:**
1. Partition table update (custom_partitions.csv) — USB reflash required
2. ConfigManager expansion (ScheduleConfig, 5 new keys)

**OTA track (sequential, after step 2):**
3. OTA upload endpoint (WebPortal)
4. OTA self-check + rollback detection (main.cpp)

**Time track (parallelizable with OTA after step 2):**
5. NTP integration (`configTzTime()` after WiFi connect)
6. Night mode scheduler (main loop)

**Settings track (after step 2):**
7. Settings export endpoint (WebPortal)

**UI integration (after all tracks):**
8. Dashboard cards (Firmware, Night Mode), rollback toast
9. Wizard Step 6 (Test Your Wall), timezone auto-detect
10. Settings import in wizard JS

## Foundation Validation

**Coherence ✅:** All 7 decisions compatible. Partition change enables OTA, NTP enables night mode, ConfigManager expansion supports both. No contradictions.

**Requirements ✅:** All 37 FRs covered. OTA, settings migration, night mode, onboarding, dashboard UI, resilience all architecturally addressed.

**Implementation Readiness ✅:** 7 decisions with code examples. Partition sizes exact. NVS key table with abbreviations. API endpoints specified. Implementation sequence with parallelization noted.

---

# Display System Release — Architecture Extension

_Extends MVP + Foundation with pluggable display modes, mode registry, Classic Card migration, Live Flight Card mode, and Mode Picker UI. All prior decisions remain in effect._

## Display System Requirements Overview

**36 FRs across 8 groups:** DisplayMode Interface (FR1-FR4), Mode Registry & Lifecycle (FR5-FR10), Classic Card Mode (FR11-FR13), Live Flight Card Mode (FR14-FR16), Mode Picker UI (FR17-FR26), Mode Persistence (FR27-FR29), Mode Switch API (FR30-FR33), Display Infrastructure (FR34-FR36).

**NFRs:** Mode switch <2s, 60fps hot path, <10ms enumeration, no heap degradation after 100 switches, 30KB free heap floor, pixel parity with pre-migration output.

## Display System Decisions

### Decision D1: DisplayMode Interface — Abstract Class with RenderContext

**Central abstraction** — every mode implements this contract.

```cpp
class DisplayMode {
    virtual bool init(const RenderContext& ctx) = 0;
    virtual void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) = 0;
    virtual void teardown() = 0;
    virtual const char* getName() const = 0;
    virtual const ModeZoneDescriptor& getZoneDescriptor() const = 0;
};
```

**RenderContext** bundles everything modes need, nothing they shouldn't:

```cpp
struct RenderContext {
    Adafruit_NeoMatrix* matrix;
    LayoutResult layout;
    uint16_t textColor;
    uint8_t brightness;
    uint16_t* logoBuffer;
    uint16_t displayCycleMs;
};
```

**Zone descriptor** — static metadata for Mode Picker UI wireframes.

**Key design:** `getMemoryRequirement()` is static function pointer in `ModeEntry` (not virtual) — needed before instantiation for heap guard.

### Decision D2: ModeRegistry — Static Table with Cooperative Switch Serialization

**Lifecycle:**
1. Teardown old mode, free heap
2. Measure `ESP.getFreeHeap()`
3. Check against `memoryRequirement()`
4. If sufficient: factory() → init() on new mode
5. If success: update index, set NVS pending, clear error
6. If failure: restore previous mode, set error

**Serialization:** Atomic `_requestedIndex` for cross-core signaling. `_switchState = SWITCHING` blocks new requests.

**NVS debounce:** 2-second quiet period after switch. No reboot required — hot-reload.

### Decision D3: NeoMatrixDisplay Responsibility Split

**Retained:** Hardware initialization, matrix object, brightness control, frame commit (`show()`), shared logo buffer, layout result, status messages, fallback renderer.

**Extracted:** `renderZoneFlight()`, `renderFlightZone()`, `renderTelemetryZone()`, `renderLogoZone()` → ClassicCardMode.

**New methods:** `show()` (frame commit), `buildRenderContext()` (assembles context), `displayFallbackCard()` (FR36 safety net).

### Decision D4: Display Task Integration — Cached Context + Tick

**Cache RenderContext** — rebuild only on config change.

**Replace renderFlight with:**
```cpp
ModeRegistry::tick(cachedCtx, flights);
g_display.show();
```

Tick handles mode-switch checks + state transitions + rendering in one call.

### Decision D5: Mode Switch API — Two Endpoints

`GET /api/display/modes` — list modes + active + switch state + zone metadata for Mode Picker wireframes.

`POST /api/display/mode` — request switch, returns after mode renders or fails.

**Zone descriptors** used by dashboard to build CSS Grid wireframe previews dynamically.

### Decision D6: NVS Persistence — Single Key with Debounced Write

`display_mode` key (12 chars) stores active mode ID in `"flightwall"` namespace. Debounced 2-second write after switch. Boot reads NVS, defaults to "classic_card".

### Decision D7: Mode Picker UI — Client-Side Wireframes + localStorage Notification

Mode list loaded via API on page load. Wireframe rendering via CSS Grid, data-driven from zone metadata. Schematic `<div>` elements positioned per zone with labels.

**Upgrade notification:** Dual-source dismissal — API flag + localStorage key. Dismissed via `localStorage.setItem()` + POST to clear firmware flag.

## Display System Patterns & Enforcement

### Rules 17-23 (Display System–Specific)

17. Mode classes in `firmware/modes/` — one `.h/.cpp` pair per mode
18. Modes ONLY access data through RenderContext — zero ConfigManager/WiFiManager/SystemStatus reads
19. Modes NEVER call `FastLED.show()` — frame commit exclusive to display task
20. Heap allocation in init(), deallocation in teardown() — constructors and render() must not allocate
21. Shared rendering helpers from `utils/DisplayUtils.h` — no duplication in mode classes
22. Adding new mode: exactly two touch points — new file in `modes/` + one line in MODE_TABLE[] in main.cpp
23. `MEMORY_REQUIREMENT` is `static constexpr` on each mode — reflects worst-case heap from init()

## Display System Implementation Sequence

**Critical path (three-phase ClassicCardMode extraction):**
1. DisplayMode interface + RenderContext
2. ModeRegistry (switch logic, heap guard, tick())
3. ClassicCardMode (COPY rendering logic first, validate pixel parity, then extract)
4. NeoMatrixDisplay refactoring (remove methods, add show()+buildRenderContext())
5. Display task integration (MODE_TABLE, ModeRegistry::init(), tick()+show())
6. LiveFlightCardMode (validates abstraction)
7. Mode Switch API (WebPortal endpoints)
8. NVS persistence (ConfigManager methods)
9. Mode Picker UI (dashboard HTML/JS)

## Display System Validation

**Coherence ✅:** All 7 decisions compatible, form consistent dependency chain. No contradictions with MVP + Foundation.

**Requirements ✅:** All 36 FRs covered. Interface, registry, modes, API, UI, persistence all explicitly mapped.

**Implementation Readiness ✅:** 7 decisions with code examples, switch flow, struct definitions. Three-phase extraction prevents highest-risk failure (pixel parity regression). Terminal fallback documented.

---

# Delight Release — Architecture Extension

_Extends MVP + Foundation + Display System with Clock Mode, Departures Board, animated fade transitions, mode scheduling, per-mode settings, and OTA Pull from GitHub. All prior decisions remain in effect._

## Delight Requirements Overview

**43 FRs across 9 groups:** Clock Mode (FR1-FR4), Departures Board (FR5-FR9), Mode Transitions (FR10-FR12), Mode Scheduling (FR13-FR17, FR36, FR39), Mode Configuration (FR18-FR20, FR40), OTA Pull (FR21-FR31, FR37-FR38), System Resilience (FR32-FR35), Extensibility (FR41-FR42), Documentation (FR43).

**NFRs:** ≥15fps transitions, <1s fade, ~30s idle fallback, <50ms row update, <60s OTA, ±5s schedule accuracy, 30-day uptime, no heap degradation.

## Delight Decisions

### Decision DL1: Fade Transition — Dual RGB888 Buffers in ModeRegistry

Fade is private method `_executeFadeTransition()` in ModeRegistry. Dual RGB888 buffers (~30KB total) allocated on switch, freed after fade completes.

**Blend algorithm:** 15 frames at 15fps = ~1s max. Blend per-pixel: `(outgoing * (15-step) + incoming * step) / 15`.

**Graceful degradation:** If malloc() fails, skip fade, use instant cut — not an error.

### Decision DL2: Mode Orchestration — Static Class with State Machine

`MANUAL` (user selection) / `SCHEDULED` (time-based) / `IDLE_FALLBACK` (zero flights).

**Priority:** `SCHEDULED > IDLE_FALLBACK > MANUAL`.

**Tick runs Core 1 main loop, ~1/sec:** Evaluates schedule rules, checks `g_flightCount` atomic, calls `ModeRegistry::requestSwitch()`.

**`onManualSwitch()`** called from WebPortal — updates MANUAL selection, exits IDLE_FALLBACK if needed.

### Decision DL3: OTAUpdater — GitHub Releases API Client with Incremental SHA-256

Static class: `checkForUpdate()` (synchronous, ~1-2s), `startDownload()` (spawns FreeRTOS task, returns immediately).

**Download flow:** Check GitHub latest release, download .sha256 file, validate format, get OTA partition, `Update.begin()`, stream .bin in chunks with incremental `mbedtls_sha256_update()`, verify hash, `Update.end(true)`, reboot.

**Error handling:** On any error after `Update.begin()`, always `Update.abort()`. Never attempt partition read-back. Inactive partition unchanged on failure.

### Decision DL4: Schedule Rules NVS — Fixed Max 8 with Indexed Keys

```
sched_r{N}_start   = uint16 (minutes since midnight)
sched_r{N}_end     = uint16
sched_r{N}_mode    = string (mode ID)
sched_r{N}_ena     = uint8 (enabled flag)
sched_r_count      = uint8 (active rule count)
```

Max 8 rules, indexed 0-7. Rules compacted on delete — no gaps. All within 15-char NVS limit.

### Decision DL5: Per-Mode Settings — Static Metadata with NVS Prefix Convention

Modes declare settings via `ModeSettingDef[]` and `ModeSettingsSchema` static const in mode `.h` files.

**NVS key pattern:** `m_{abbrev}_{setting}` (≤15 chars total). Mode abbreviations: `clock` (5), `depbd` (5), `lfcrd` (5) — documented.

**Mode abbreviation rule:** ≤5 chars, enforced by documentation + PR review.

**ConfigManager helpers:** `getModeSetting()`, `setModeSetting()` — read/write via m_{abbrev}_{key} convention.

### Decision DL6: API Endpoints — OTA Pull + Schedule + Mode Settings

**New endpoints:**
- `GET /api/ota/check` — check GitHub for update
- `POST /api/ota/pull` — start download task
- `GET /api/ota/status` — poll progress + state
- `GET /api/schedule` — get mode schedule rules
- `POST /api/schedule` — save schedule rules

**Updated endpoints:**
- `GET /api/display/modes` — add settings array per mode + current values
- `POST /api/display/mode` — accept optional settings object
- `GET /api/status` — add ota_available, ota_version, orchestrator_state, active_schedule_rule

## Delight Patterns & Enforcement

### Rules 24-30 (Delight-Specific)

24. Mode switches from WebPortal via `ModeOrchestrator::onManualSwitch()` — only two call sites: `tick()` and `onManualSwitch()`
25. OTA SHA-256 via `mbedtls_sha256_update()` per chunk — never post-download read-back; always `Update.abort()` after `Update.begin()` on error
26. Both OTA paths (Push + Pull) call `ModeRegistry::prepareForOTA()` before `Update.begin()` — sets `_switchState = SWITCHING`
27. Fade buffers malloc/free in single call — never persist beyond transition; malloc failure uses instant cut (graceful degradation)
28. Per-mode NVS via `ConfigManager::getModeSetting()/setModeSetting()` only — never construct `m_{abbrev}_{key}` manually; API handlers iterate schema dynamically
29. Mode settings schemas declared `static const` in mode `.h` files — never centralize in ConfigManager
30. Cross-core `std::atomic` globals in `main.cpp` only — modes/adapters never read/write atomics directly

## Delight Implementation Sequence

**Sequential foundation:**
1. ConfigManager expansion (ModeScheduleConfig, per-mode helpers)
2. ModeEntry expansion (settingsSchema field, nullptr for existing modes)

**Mode implementations (after step 2):**
3. ClockMode
4. DeparturesBoardMode

**Orchestration track (after step 3):**
5. ModeOrchestrator
6. Fade transition in ModeRegistry

**OTA track (after step 2):**
7. OTAUpdater
8. ModeRegistry::prepareForOTA()

**API + UI (after all firmware):**
9. WebPortal endpoints (OTA Pull, schedule, mode settings)
10. Dashboard UI (OTA Pull, schedule timeline, per-mode settings panels)

## Delight Validation

**Coherence ✅:** All 6 decisions form consistent dependency chain. Orchestrator uses existing `requestSwitch()` atomic. OTA Pull reuses Foundation streaming pattern. Per-mode settings follow ModeZoneDescriptor pattern.

**Requirements ✅:** All 43 FRs covered. Clock Mode, Departures Board, transitions, scheduling, settings, OTA Pull, resilience all architecturally addressed.

**Implementation Readiness ✅:** 6 decisions with code examples, state machine diagrams, API request/response formats. Cross-core signaling pattern documented. OTA error discipline specified. Terminal fallback path from Display System retained.

]]></file>
<file id="d7cf6d0a" path="_bmad-output/planning-artifacts/epics/index.md" label="EPIC"><![CDATA[

# Epics (sharded)

This index exists so `bmad-assist` treats `epics/` as the primary sharded epic source (see `resolve_doc_path` in bmad-assist). Epic files not linked below are still loaded as orphans in filename order.


]]></file>
<file id="ab213a12" path="_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md" label="DOCUMENTATION"><![CDATA[

# Story fn-1.3: OTA Upload Endpoint

Status: complete

## Story

As a **user**,
I want **to upload a firmware binary to the device over the local network**,
So that **I can update the firmware without connecting a USB cable**.

## Acceptance Criteria

1. **Given** `POST /api/ota/upload` receives a multipart file upload **When** the first chunk arrives with first byte `0xE9` (ESP32 magic byte) **Then** `Update.begin(partition->size)` is called using the next OTA partition's capacity (not `Content-Length`) **And** each subsequent chunk is written via `Update.write(data, len)` **And** the upload streams directly to flash with no full-binary RAM buffering

2. **Given** the first byte of the uploaded file is NOT `0xE9` **When** the first chunk is processed **Then** the endpoint returns HTTP 400 with `{ "ok": false, "error": "Not a valid ESP32 firmware image", "code": "INVALID_FIRMWARE" }` **And** no data is written to flash **And** validation completes within 1 second

3. **Given** `Update.write()` returns fewer bytes than expected **When** a write failure is detected **Then** `Update.abort()` is called **And** the current firmware continues running unaffected **And** an error response `{ "ok": false, "error": "Write failed — flash may be worn or corrupted", "code": "WRITE_FAILED" }` is returned

4. **Given** WiFi drops mid-upload **When** the connection is interrupted **Then** `Update.abort()` is called via `request->onDisconnect()` **And** the inactive partition is NOT marked bootable **And** the device continues running current firmware

5. **Given** the upload completes successfully **When** `Update.end(true)` returns true on the final chunk **Then** the endpoint returns `{ "ok": true, "message": "Rebooting..." }` **And** `ESP.restart()` is called after a 500ms delay using `esp_timer_start_once()` **And** the device reboots into the newly written firmware

6. **Given** a 1.5MB firmware binary uploaded over local WiFi **When** the transfer completes **Then** the total upload time is under 30 seconds

## Tasks / Subtasks

- [x] **Task 1: Add OTA upload state management struct** (AC: #1, #4)
  - [x] Create `OTAUploadState` struct similar to `LogoUploadState` with fields: `request`, `valid`, `started`, `bytesWritten`, `error`, `errorCode`
  - [x] Add global `std::vector<OTAUploadState> g_otaUploads` and helper functions `findOTAUpload()`, `clearOTAUpload()`
  - [x] Helper must call `Update.abort()` if `started` is true when cleaning up

- [x] **Task 2: Register POST /api/ota/upload route** (AC: #1, #2, #3, #4, #5)
  - [x] Add route using same pattern as `POST /api/logos` (lines 256-352 in WebPortal.cpp)
  - [x] Use upload handler signature: `(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final)`
  - [x] Register `request->onDisconnect()` to call `clearOTAUpload(request)` for WiFi interruption handling

- [x] **Task 3: Implement first-chunk validation and Update.begin()** (AC: #1, #2)
  - [x] On `index == 0`: check `data[0] == 0xE9` magic byte
  - [x] If invalid: set `state.valid = false`, `state.error = "Not a valid ESP32 firmware image"`, `state.errorCode = "INVALID_FIRMWARE"`, return early
  - [x] If valid: get next OTA partition via `esp_ota_get_next_update_partition(NULL)`
  - [x] Call `Update.begin(partition->size)` (NOT Content-Length, per AR18)
  - [x] Set `state.started = true` after successful `Update.begin()`
  - [x] Log partition info: `Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size)`

- [x] **Task 4: Implement streaming write per chunk** (AC: #1, #3)
  - [x] For each chunk (including first after validation): call `Update.write(data, len)`
  - [x] Check return value equals `len`; if not, set `state.valid = false`, `state.error = "Write failed — flash may be worn or corrupted"`, `state.errorCode = "WRITE_FAILED"`
  - [x] Call `Update.abort()` immediately on write failure
  - [x] Update `state.bytesWritten += len` for debugging/logging

- [x] **Task 5: Implement final chunk handling and reboot** (AC: #5)
  - [x] On `final == true`: call `Update.end(true)`
  - [x] If `Update.end()` returns false: set error `"Firmware verification failed"`, code `"VERIFY_FAILED"`, call `Update.abort()`
  - [x] If success: state remains valid for request handler

- [x] **Task 6: Implement request handler for response** (AC: #2, #3, #5)
  - [x] Check `state->valid` — if false, return `{ "ok": false, "error": state->error, "code": state->errorCode }` with HTTP 400
  - [x] If valid: return `{ "ok": true, "message": "Rebooting..." }` with HTTP 200
  - [x] Schedule reboot using existing `esp_timer_start_once()` pattern (see lines 491-498 in WebPortal.cpp)
  - [x] Use 500ms delay (500000 microseconds) to allow response to be sent
  - [x] Call `clearOTAUpload(request)` after sending response

- [x] **Task 7: Add OTA subsystem status updates** (AC: #3, #5)
  - [x] On successful upload start: `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Upload in progress")`
  - [x] On write failure: `SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Write failed")`
  - [x] On verification failure: `SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Verification failed")`
  - [x] On success before reboot: `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Update complete — rebooting")`

- [x] **Task 8: Add WebPortal header declarations** (AC: #1-#5)
  - [x] Add `_handleOTAUpload()` declaration in WebPortal.h (or keep inline like logo upload)
  - [x] Add `static void` helper for `_handleOTAUploadChunk()` if refactoring for readability

- [x] **Task 9: Add #include <Update.h>** (AC: #1, #3, #5)
  - [x] Add `#include <Update.h>` to WebPortal.cpp (ESP32 Arduino core library)
  - [x] Verify it compiles — Update.h is part of ESP32 Arduino core, no lib_deps needed

- [x] **Task 10: Build and test** (AC: #1-#6)
  - [x] Run `pio run` — verify clean build with no errors
  - [x] Verify binary size remains under 1.5MB limit (current: ~1.21MB = 76.9%)
  - [x] Manual test: upload invalid file (wrong magic byte) — expect 400 error < 1 second
  - [x] Manual test: upload valid firmware — expect success, reboot
  - [x] Manual test: disconnect WiFi mid-upload — verify device continues running

## Dev Notes

### Critical Architecture Constraints

**ESP32 Magic Byte Validation (HARD REQUIREMENT per AR-E12):**
ESP32 firmware binaries start with byte `0xE9`. Validate on first chunk BEFORE calling `Update.begin()`:
```cpp
if (index == 0 && len > 0 && data[0] != 0xE9) {
    state.valid = false;
    state.error = "Not a valid ESP32 firmware image";
    state.errorCode = "INVALID_FIRMWARE";
    return; // Early exit — no flash write
}
```

**Update.begin() Size Parameter (CRITICAL per AR18):**
Multipart `Content-Length` is the BODY size, NOT the file size. Do NOT use `total` from the upload handler. Instead, use the OTA partition capacity:
```cpp
const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
if (!partition) {
    state.valid = false;
    state.error = "No OTA partition available";
    state.errorCode = "NO_OTA_PARTITION";
    return;
}
if (!Update.begin(partition->size)) {
    state.valid = false;
    state.error = "Could not begin OTA update";
    state.errorCode = "BEGIN_FAILED";
    return;
}
```

**Streaming Architecture (HARD REQUIREMENT per AR-E13, NFR-C4):**
- Stream directly to flash via `Update.write()` per chunk
- NEVER buffer the full binary in RAM
- Each chunk is typically 4KB-16KB from ESPAsyncWebServer
- Total RAM impact: ~50-100 bytes for state struct only

**Abort on Any Error (CRITICAL):**
After `Update.begin()` is called, ANY error MUST call `Update.abort()`:
```cpp
if (writtenBytes != len) {
    Update.abort();  // MUST call before returning error
    state.valid = false;
    // ... set error
}
```

**Disconnect Handling Pattern:**
Follow the existing pattern from logo upload (WebPortal.cpp line 307):
```cpp
request->onDisconnect([request]() {
    auto* state = findOTAUpload(request);
    if (state && state->started) {
        Update.abort();  // Critical: abort in-progress update
    }
    clearOTAUpload(request);
});
```

**Reboot Timer Pattern:**
Use the existing `esp_timer_start_once()` pattern (WebPortal.cpp lines 491-498):
```cpp
static esp_timer_handle_t otaRebootTimer = nullptr;
if (!otaRebootTimer) {
    esp_timer_create_args_t args = {};
    args.callback = [](void*) { ESP.restart(); };
    args.name = "ota_reboot";
    esp_timer_create(&args, &otaRebootTimer);
}
esp_timer_start_once(otaRebootTimer, 500000); // 500ms in microseconds
```

### Response Format (Follow JSON Envelope Pattern)

**Success Response:**
```json
{
  "ok": true,
  "message": "Rebooting..."
}
```

**Error Responses:**
```json
{
  "ok": false,
  "error": "Not a valid ESP32 firmware image",
  "code": "INVALID_FIRMWARE"
}
```

Error codes:
| Code | Trigger |
|------|---------|
| `INVALID_FIRMWARE` | First byte != 0xE9 |
| `NO_OTA_PARTITION` | `esp_ota_get_next_update_partition()` returns NULL |
| `BEGIN_FAILED` | `Update.begin()` returns false |
| `WRITE_FAILED` | `Update.write()` returns fewer bytes than expected |
| `VERIFY_FAILED` | `Update.end(true)` returns false |
| `UPLOAD_ERROR` | Generic fallback for unexpected failures |

### OTAUploadState Struct

```cpp
struct OTAUploadState {
    AsyncWebServerRequest* request;
    bool valid;           // false if any validation/write failed
    bool started;         // true after Update.begin() succeeds
    size_t bytesWritten;  // for debugging/logging only
    String error;         // human-readable error message
    String errorCode;     // machine-readable error code
};
```

### ESP32 Update.h Library Reference

The `Update` class is a singleton from ESP32 Arduino core (`#include <Update.h>`):

| Method | Purpose |
|--------|---------|
| `Update.begin(size_t size)` | Start update with expected size (use partition size) |
| `Update.write(uint8_t* data, size_t len)` | Write chunk to flash, returns bytes written |
| `Update.end(bool evenIfRemaining)` | Finalize update, `true` = accept even if not all bytes written |
| `Update.abort()` | Cancel in-progress update, mark partition invalid |
| `Update.hasError()` | Check if error occurred |
| `Update.getError()` | Get error code |
| `Update.errorString()` | Get human-readable error string |

**Update library behavior:**
- `Update.begin()` erases flash blocks as needed (first call may take 2-3 seconds)
- `Update.write()` writes in 4KB flash block sizes internally
- `Update.end(true)` sets the boot partition to the newly written firmware
- After `Update.end(true)`, the next reboot loads the new firmware
- If new firmware crashes before self-check, bootloader rolls back automatically (Story fn-1.4)

Sources:
- [ESP32 Forum - OTA and Update.h](https://www.esp32.com/viewtopic.php?t=16512)
- [GitHub - arduino-esp32 Update examples](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/HTTPS_OTA_Update/HTTPS_OTA_Update.ino)
- [Last Minute Engineers - ESP32 OTA Web Updater](https://lastminuteengineers.com/esp32-ota-web-updater-arduino-ide/)

### Performance Requirements (NFR-P1, NFR-P7)

| Metric | Target |
|--------|--------|
| 1.5MB upload time | < 30 seconds over local WiFi |
| Invalid file rejection | < 1 second (fail on first chunk) |

The ESPAsyncWebServer receives chunks of 4-16KB depending on network conditions. At typical 10KB/chunk, a 1.5MB file arrives in ~150 chunks. Flash write speed is ~100-400KB/s depending on ESP32 flash quality.

### Partition Layout Reference

From `firmware/custom_partitions.csv` (Story fn-1.1):
```
# Name,   Type, SubType, Offset,    Size,    Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xE000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x180000,
app1,     app,  ota_1,   0x190000, 0x180000,
spiffs,   data, spiffs,  0x310000, 0xF0000,
```

- **app0**: 0x10000 to 0x18FFFF (1.5MB) — first OTA partition
- **app1**: 0x190000 to 0x30FFFF (1.5MB) — second OTA partition
- Running partition determined by `esp_ota_get_running_partition()`
- Next partition determined by `esp_ota_get_next_update_partition(NULL)`

### Project Structure Notes

**Files to modify:**
1. `firmware/adapters/WebPortal.cpp` — Add POST /api/ota/upload route + upload handler
2. `firmware/adapters/WebPortal.h` — Optional: add handler declaration if not using inline lambdas

**Files NOT to modify (handled by other stories):**
- `main.cpp` — Self-check logic is Story fn-1.4
- `dashboard.js` — UI is Story fn-1.6
- `ConfigManager.cpp` — Already has OTA subsystem from Story fn-1.2

**Code location guidance:**
- Insert new route registration after line 352 (after `POST /api/logos`) in `_registerRoutes()`
- Follow the exact pattern of logo upload handler for consistency
- Place `OTAUploadState` struct and helpers in the anonymous namespace (lines 38-98)

### Testing Strategy

**Automated build verification:**
```bash
cd firmware && pio run
# Expect: SUCCESS, binary < 1.5MB
```

**Manual test cases:**
1. **Invalid file test**: Upload a `.txt` file or random binary without 0xE9 header
   - Expect: HTTP 400, error message, device continues running
   - Verify: Response time < 1 second

2. **Valid firmware test**: Upload a valid `.bin` from `pio run` output
   - Expect: HTTP 200, "Rebooting..." message, device reboots in ~500ms
   - Verify: New firmware runs (check FW_VERSION in Serial output)

3. **Disconnect test**: Start upload, disconnect WiFi mid-transfer
   - Expect: Device continues running current firmware
   - Verify: No corruption, no crash

4. **Oversized file test**: Upload a file > 1.5MB (if possible to construct)
   - Expect: Upload proceeds but `Update.end()` may fail or fill partition
   - Note: This is an edge case; client-side validation (Story fn-1.6) prevents this

### Previous Story Intelligence (fn-1.2)

**From fn-1.2 completion notes:**
- Binary size: 1,209,440 bytes (1.15MB) — 76.9% of 1.5MB partition
- SystemStatus now has OTA and NTP subsystems ready (`Subsystem::OTA`)
- ConfigManager patterns established for new structs

**Lessons learned from fn-1.1 and fn-1.2:**
1. **Explicit error handling** — Always log and surface errors, never silent failures
2. **Thread safety** — OTA upload runs on AsyncTCP task (same as web server), no mutex needed for Upload singleton
3. **Test on hardware** — Unit tests build-pass but hardware verification essential

### References

- [Source: architecture.md#Decision F2: OTA Handler Architecture — WebPortal + main.cpp]
- [Source: epic-fn-1.md#Story fn-1.3: OTA Upload Endpoint]
- [Source: WebPortal.cpp lines 256-352 for logo upload pattern]
- [Source: WebPortal.cpp lines 491-498 for reboot timer pattern]
- [Source: prd.md#Technical Success criteria for OTA]
- [Source: architecture.md#AR-E12, AR-E13 enforcement rules]

### Dependencies

**This story depends on:**
- fn-1.1 (Partition Table & Build Configuration) — COMPLETE
- fn-1.2 (ConfigManager Expansion) — COMPLETE (provides SystemStatus::OTA)

**Stories that depend on this:**
- fn-1.4: OTA Self-Check & Rollback Detection — uses the firmware written by this endpoint
- fn-1.6: Dashboard Firmware Card & OTA Upload UI — calls this endpoint

### Risk Mitigation

1. **Flash wear concern**: Flash has ~100K write cycles. OTA updates are infrequent. Log write failures with helpful message.
2. **Partial write corruption**: `Update.abort()` marks partition invalid; bootloader ignores it. Safe.
3. **Power loss during write**: ESP32 bootloader handles this; rolls back to previous partition.
4. **Large file RAM exhaustion**: Streaming architecture prevents this. ESPAsyncWebServer chunks are 4-16KB max.

## Dev Agent Record

### Agent Model Used

Claude Opus 4 (Story Creation)

### Debug Log References

N/A — Story creation phase

### Completion Notes List

**2026-04-12: Ultimate context engine analysis completed**
- Comprehensive analysis of epic-fn-1.md extracted all 6 acceptance criteria with BDD format
- WebPortal.cpp patterns analyzed: logo upload (lines 256-352), reboot timer (lines 491-498)
- Existing OTA infrastructure verified: partition table ready, esp_ota_ops.h included in main.cpp
- ESP32 Update.h library research completed via web search
- All architecture requirements mapped: AR3, AR12, AR18, AR-E12, AR-E13
- Performance requirements documented: NFR-P1 (30s upload), NFR-P7 (1s validation)
- 10 tasks created with explicit code references and line numbers

**2026-04-12: Implementation completed**
- Added `OTAUploadState` struct in anonymous namespace (lines 83-91)
- Added helper functions `findOTAUpload()` and `clearOTAUpload()` (lines 95-114)
- Implemented `POST /api/ota/upload` route following logo upload pattern (lines 399-531)
- Added `#include <Update.h>` and `#include <esp_ota_ops.h>` (lines 27-28)
- Magic byte validation (0xE9) on first chunk prevents invalid firmware writes
- Partition size used for `Update.begin()` instead of Content-Length per AR18
- WiFi disconnect handling via `request->onDisconnect()` calls `Update.abort()`
- SystemStatus updates for OTA subsystem on all state transitions
- 500ms reboot delay using `esp_timer_start_once()` pattern
- Build successful: 1,216,384 bytes (77.3% of partition)
- All 6 acceptance criteria satisfied

**2026-04-12: Code review synthesis — 7 fixes applied**
- CRITICAL: Added `state->started = false` after `Update.end(true)` so `clearOTAUpload()` does NOT call `Update.abort()` on a successfully completed update (was aborting the newly written firmware)
- HIGH: Added `g_otaInProgress` flag + concurrent-upload guard; second upload during active OTA now returns `OTA_BUSY` (409 Conflict) instead of corrupting the Update singleton
- MEDIUM: Added `SystemStatus::set(ERROR)` for all previously-missing error paths: `INVALID_FIRMWARE`, `NO_OTA_PARTITION`, `BEGIN_FAILED` — now consistent with `WRITE_FAILED`/`VERIFY_FAILED`
- LOW: Added `Serial.printf("[OTA] Writing to %s, size 0x%x\n", ...)` partition log completing Task 3 requirement
- LOW: Server-side failures (`NO_OTA_PARTITION`, `BEGIN_FAILED`, `WRITE_FAILED`, `VERIFY_FAILED`) now return HTTP 500; concurrent conflict returns 409; client errors (`INVALID_FIRMWARE`) keep 400
- Build successful post-fixes: 1,217,104 bytes (77.4% of 1.5MB partition)

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Implementation complete - OTA upload endpoint with streaming, validation, and error handling | Claude |
| 2026-04-12 | Code review synthesis: 7 fixes — critical abort bug, concurrent guard, SystemStatus consistency, HTTP codes | Claude |

### File List

- `firmware/adapters/WebPortal.cpp` (MODIFIED) - Added OTA upload route, state struct, helpers; patched by code review synthesis

## Senior Developer Review (AI)

### Review: 2026-04-12
- **Reviewer:** AI Code Review Synthesis (4 validators)
- **Evidence Score:** 12 → REJECT (pre-fix); all critical/high issues resolved
- **Issues Found:** 8 verified (5 false positives dismissed)
- **Issues Fixed:** 7
- **Action Items Created:** 1

#### Review Follow-ups (AI)
- [ ] [AI-Review] MEDIUM: Add automated unit/integration tests for OTA endpoint — invalid magic byte rejection, write failure path, disconnect cleanup, and success-path must not abort (`firmware/test/` or `tests/smoke/test_web_portal_smoke.py`)


]]></file>
<file id="17deba99" path="firmware/adapters/WebPortal.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: Web server adapter serving gzipped HTML pages and REST API endpoints.
Responsibilities:
- Serve wizard.html.gz in AP mode, dashboard.html.gz in STA mode via GET /.
- Expose JSON API: GET/POST /api/settings, GET /api/status, GET /api/layout, POST /api/reboot, POST /api/reset, GET /api/wifi/scan.
- Use consistent JSON envelope: { "ok": bool, "data": ..., "error": "...", "code": "..." }.
Architecture: ESPAsyncWebServer (mathieucarbou fork) — non-blocking, runs on AsyncTCP task.

JSON key alignment (matches ConfigManager::updateCacheFromKey / NVS):
  Display:   brightness, text_color_r, text_color_g, text_color_b
  Location:  center_lat, center_lon, radius_km
  Hardware:  tiles_x, tiles_y, tile_pixels, display_pin, origin_corner, scan_dir, zigzag
  Timing:    fetch_interval, display_cycle
  Network:   wifi_ssid, wifi_password, os_client_id, os_client_sec, aeroapi_key

GET /api/status extended JSON (Story 2.4):
  data.subsystems   — existing six subsystem objects (wifi, opensky, aeroapi, cdn, nvs, littlefs)
  data.wifi_detail  — SSID, RSSI, IP, mode
  data.device       — uptime_ms, free_heap, fs_total, fs_used
  data.flight       — last_fetch_ms, state_vectors, enriched_flights, logos_matched
  data.quota        — fetches_since_boot, limit, fetch_interval_s, estimated_monthly_polls, over_pace
*/
#include "adapters/WebPortal.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <vector>
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"
#include "core/LogoManager.h"
#include "utils/Log.h"

// Defined in main.cpp — provides thread-safe flight stats for the health page
extern FlightStatsSnapshot getFlightStatsSnapshot();

#include "core/LayoutEngine.h"

namespace {
struct PendingRequestBody {
    AsyncWebServerRequest* request;
    String body;
};

std::vector<PendingRequestBody> g_pendingBodies;
constexpr size_t MAX_SETTINGS_BODY_BYTES = 4096;

// Logo upload state — streams file data directly to LittleFS
struct LogoUploadState {
    AsyncWebServerRequest* request;
    String filename;
    String path;
    File file;
    bool valid;
    bool written;
    String error;
    String errorCode;
};

std::vector<LogoUploadState> g_logoUploads;

LogoUploadState* findLogoUpload(AsyncWebServerRequest* request) {
    for (auto& u : g_logoUploads) {
        if (u.request == request) return &u;
    }
    return nullptr;
}

void clearLogoUpload(AsyncWebServerRequest* request, bool removeFile = false) {
    for (auto it = g_logoUploads.begin(); it != g_logoUploads.end(); ++it) {
        if (it->request == request) {
            if (it->file) it->file.close();
            if (removeFile && it->path.length()) {
                LittleFS.remove(it->path);
            }
            g_logoUploads.erase(it);
            return;
        }
    }
}

// OTA upload state — streams firmware directly to flash via Update library
struct OTAUploadState {
    AsyncWebServerRequest* request;
    bool valid;           // false if any validation/write failed
    bool started;         // true after Update.begin() succeeds
    size_t bytesWritten;  // for debugging/logging only
    String error;         // human-readable error message
    String errorCode;     // machine-readable error code
};

std::vector<OTAUploadState> g_otaUploads;
static bool g_otaInProgress = false;  // Enforce single-flight OTA — Update is a singleton

OTAUploadState* findOTAUpload(AsyncWebServerRequest* request) {
    for (auto& u : g_otaUploads) {
        if (u.request == request) return &u;
    }
    return nullptr;
}

void clearOTAUpload(AsyncWebServerRequest* request) {
    for (auto it = g_otaUploads.begin(); it != g_otaUploads.end(); ++it) {
        if (it->request == request) {
            // CRITICAL: abort in-progress update on cleanup (started=false means already completed)
            if (it->started) {
                Update.abort();
                LOG_I("OTA", "Upload aborted during cleanup");
            }
            g_otaUploads.erase(it);
            g_otaInProgress = false;
            return;
        }
    }
}

PendingRequestBody* findPendingBody(AsyncWebServerRequest* request) {
    for (auto& pending : g_pendingBodies) {
        if (pending.request == request) {
            return &pending;
        }
    }
    return nullptr;
}

void clearPendingBody(AsyncWebServerRequest* request) {
    for (auto it = g_pendingBodies.begin(); it != g_pendingBodies.end(); ++it) {
        if (it->request == request) {
            g_pendingBodies.erase(it);
            return;
        }
    }
}
} // namespace

void WebPortal::init(AsyncWebServer& server, WiFiManager& wifiMgr) {
    _server = &server;
    _wifiMgr = &wifiMgr;
    _registerRoutes();
    LOG_I("WebPortal", "Routes registered");
}

void WebPortal::begin() {
    if (!_server) return;
    _server->begin();
    LOG_I("WebPortal", "Server started on port 80");
}

void WebPortal::onReboot(RebootCallback callback) {
    _rebootCallback = callback;
}

void WebPortal::onCalibration(CalibrationCallback callback) {
    _calibrationCallback = callback;
}

void WebPortal::onPositioning(PositioningCallback callback) {
    _positioningCallback = callback;
}

void WebPortal::_registerRoutes() {
    // GET / — serve wizard or dashboard based on WiFi mode
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleRoot(request);
    });

    // GET /api/settings
    _server->on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetSettings(request);
    });

    // POST /api/settings — uses body handler for JSON parsing
    _server->on("/api/settings", HTTP_POST,
        // request handler (called after body is received)
        [](AsyncWebServerRequest* request) {
            // no-op: response sent in body handler
        },
        nullptr, // upload handler
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (data == nullptr || total == 0) {
                clearPendingBody(request);
                _sendJsonError(request, 400, "Empty settings object", "EMPTY_PAYLOAD");
                return;
            }

            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }

            if (index == 0) {
                clearPendingBody(request);
                PendingRequestBody pending{request, String()};
                pending.body.reserve(total);
                g_pendingBodies.push_back(pending);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }

            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }

            for (size_t i = 0; i < len; ++i) {
                pending->body += static_cast<char>(data[i]);
            }

            if (index + len == total) {
                _handlePostSettings(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // GET /api/status
    _server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetStatus(request);
    });

    // POST /api/reboot
    _server->on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostReboot(request);
    });

    // POST /api/reset — factory reset (erase NVS + restart)
    _server->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostReset(request);
    });

    // GET /api/wifi/scan
    _server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetWifiScan(request);
    });

    // GET /api/layout (Story 3.1)
    _server->on("/api/layout", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLayout(request);
    });

    // POST /api/calibration/start (Story 4.2) — gradient pattern
    _server->on("/api/calibration/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostCalibrationStart(request);
    });

    // POST /api/positioning/start — panel positioning guide (independent from calibration)
    _server->on("/api/positioning/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (_positioningCallback) {
            _positioningCallback(true);
        }
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["message"] = "Positioning mode started";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // POST /api/positioning/stop
    _server->on("/api/positioning/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (_positioningCallback) {
            _positioningCallback(false);
        }
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["message"] = "Positioning mode stopped";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // POST /api/calibration/stop (Story 4.2)
    _server->on("/api/calibration/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostCalibrationStop(request);
    });

    // GET /api/logos — list uploaded logos
    _server->on("/api/logos", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLogos(request);
    });

    // POST /api/logos — multipart file upload (streaming to LittleFS)
    _server->on("/api/logos", HTTP_POST,
        // Request handler — called after upload completes
        [this](AsyncWebServerRequest* request) {
            auto* state = findLogoUpload(request);
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();

            if (!state || !state->valid) {
                root["ok"] = false;
                root["error"] = (state && state->error.length()) ? state->error : "Upload failed";
                root["code"] = (state && state->errorCode.length()) ? state->errorCode : "UPLOAD_ERROR";
                String output;
                serializeJson(doc, output);
                clearLogoUpload(request, true);
                request->send(400, "application/json", output);
                return;
            }

            root["ok"] = true;
            JsonObject data = root["data"].to<JsonObject>();
            data["filename"] = state->filename;
            data["size"] = LOGO_BUFFER_BYTES;

            String output;
            serializeJson(doc, output);
            clearLogoUpload(request);
            LogoManager::scanLogoCount();
            request->send(200, "application/json", output);
        },
        // Upload handler — called for each chunk of file data
        [this](AsyncWebServerRequest* request, const String& filename,
               size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                // First chunk — validate and open file for writing
                clearLogoUpload(request);
                LogoUploadState state;
                state.request = request;
                state.filename = filename;
                state.path = String("/logos/") + filename;
                state.valid = true;
                state.written = false;

                // Validate filename before touching LittleFS.
                if (!LogoManager::isSafeLogoFilename(filename)) {
                    state.valid = false;
                    state.error = filename + " - invalid filename";
                    state.errorCode = "INVALID_NAME";
                    g_logoUploads.push_back(state);
                    return;
                }

                request->onDisconnect([request]() {
                    clearLogoUpload(request, true);
                });

                // Open file for streaming write
                state.file = LittleFS.open(state.path, "w");
                if (!state.file) {
                    state.valid = false;
                    state.error = "LittleFS full or write error";
                    state.errorCode = "FS_WRITE_ERROR";
                }
                g_logoUploads.push_back(state);
            }

            auto* state = findLogoUpload(request);
            if (!state || !state->valid) return;

            // Stream write chunk to file
            if (state->file && len > 0) {
                size_t written = state->file.write(data, len);
                if (written != len) {
                    state->valid = false;
                    state->error = "LittleFS full";
                    state->errorCode = "FS_FULL";
                    state->file.close();
                    LittleFS.remove(state->path);
                    return;
                }
            }

            if (final) {
                size_t totalSize = index + len;
                if (state->file) state->file.close();

                if (totalSize != LOGO_BUFFER_BYTES) {
                    state->valid = false;
                    state->error = state->filename + " - invalid size (" + String((unsigned long)totalSize) + " bytes, expected 2048)";
                    state->errorCode = "INVALID_SIZE";
                    LittleFS.remove(state->path);
                    return;
                }

                state->written = true;
            }
        }
    );

    // DELETE /api/logos/:name
    _server->on("/api/logos/*", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        _handleDeleteLogo(request);
    });

    // GET /logos/:name — serve raw logo binary for thumbnail rendering
    _server->on("/logos/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLogoFile(request);
    });

    // POST /api/ota/upload — firmware OTA upload (streaming to flash)
    _server->on("/api/ota/upload", HTTP_POST,
        // Request handler — called after upload completes
        [this](AsyncWebServerRequest* request) {
            auto* state = findOTAUpload(request);
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();

            if (!state || !state->valid) {
                root["ok"] = false;
                root["error"] = (state && state->error.length()) ? state->error : "Upload failed";
                const String errCode = (state && state->errorCode.length()) ? state->errorCode : "UPLOAD_ERROR";
                root["code"] = errCode;
                // Map error codes to semantically correct HTTP status codes
                int httpCode = 400;  // default: client error (e.g. bad firmware file)
                if (errCode == "OTA_BUSY") httpCode = 409;  // Conflict
                else if (errCode == "NO_OTA_PARTITION" || errCode == "BEGIN_FAILED" ||
                         errCode == "WRITE_FAILED"     || errCode == "VERIFY_FAILED") httpCode = 500;
                String output;
                serializeJson(doc, output);
                clearOTAUpload(request);
                request->send(httpCode, "application/json", output);
                return;
            }

            // Success — schedule reboot
            root["ok"] = true;
            root["message"] = "Rebooting...";
            String output;
            serializeJson(doc, output);
            clearOTAUpload(request);

            SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Update complete — rebooting");
            LOG_I("OTA", "Upload complete, scheduling reboot");

            request->send(200, "application/json", output);

            // Schedule reboot after 500ms to allow response to be sent
            static esp_timer_handle_t otaRebootTimer = nullptr;
            if (!otaRebootTimer) {
                esp_timer_create_args_t args = {};
                args.callback = [](void*) { ESP.restart(); };
                args.name = "ota_reboot";
                esp_timer_create(&args, &otaRebootTimer);
            }
            esp_timer_start_once(otaRebootTimer, 500000); // 500ms in microseconds
        },
        // Upload handler — called for each chunk of firmware data
        [this](AsyncWebServerRequest* request, const String& filename,
               size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                // First chunk — validate magic byte and begin update
                clearOTAUpload(request);

                // Reject concurrent OTA uploads — Update singleton is not re-entrant
                if (g_otaInProgress) {
                    OTAUploadState busy;
                    busy.request = request;
                    busy.valid = false;
                    busy.started = false;
                    busy.bytesWritten = 0;
                    busy.error = "Another OTA update is already in progress";
                    busy.errorCode = "OTA_BUSY";
                    g_otaUploads.push_back(busy);
                    LOG_I("OTA", "Rejected concurrent OTA upload");
                    return;
                }

                OTAUploadState state;
                state.request = request;
                state.valid = true;
                state.started = false;
                state.bytesWritten = 0;

                // Register disconnect handler for WiFi interruption
                request->onDisconnect([request]() {
                    auto* s = findOTAUpload(request);
                    if (s && s->started) {
                        Update.abort();
                        LOG_I("OTA", "Upload aborted due to disconnect");
                    }
                    clearOTAUpload(request);
                });

                // Validate ESP32 magic byte (0xE9)
                if (len == 0 || data[0] != 0xE9) {
                    state.valid = false;
                    state.error = "Not a valid ESP32 firmware image";
                    state.errorCode = "INVALID_FIRMWARE";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Invalid firmware image");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "Invalid firmware magic byte");
                    return;
                }

                // Get next OTA partition
                const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
                if (!partition) {
                    state.valid = false;
                    state.error = "No OTA partition available";
                    state.errorCode = "NO_OTA_PARTITION";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "No OTA partition found");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "No OTA partition found");
                    return;
                }

                // Begin update with partition size (NOT Content-Length per AR18)
                if (!Update.begin(partition->size)) {
                    state.valid = false;
                    state.error = "Could not begin OTA update";
                    state.errorCode = "BEGIN_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Could not begin OTA update");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "Update.begin() failed");
                    return;
                }

                state.started = true;
                g_otaInProgress = true;
                g_otaUploads.push_back(state);
                Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size);
                SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Upload in progress");
                LOG_I("OTA", "Update started");
            }

            auto* state = findOTAUpload(request);
            if (!state || !state->valid) return;

            // Stream write chunk to flash
            if (len > 0) {
                size_t written = Update.write(data, len);
                if (written != len) {
                    Update.abort();
                    state->valid = false;
                    state->error = "Write failed — flash may be worn or corrupted";
                    state->errorCode = "WRITE_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Write failed");
                    LOG_E("OTA", "Flash write failed");
                    return;
                }
                state->bytesWritten += len;
            }

            if (final) {
                // Finalize update
                if (!Update.end(true)) {
                    Update.abort();
                    state->valid = false;
                    state->error = "Firmware verification failed";
                    state->errorCode = "VERIFY_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Verification failed");
                    LOG_E("OTA", "Firmware verification failed");
                    return;
                }
                // Clear started so clearOTAUpload does NOT call Update.abort() on a completed update
                state->started = false;
                LOG_I("OTA", "Update finalized successfully");
            }
        }
    );

    // Shared static assets (gzipped on LittleFS)
    _server->on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/style.css.gz", "text/css");
    });
    _server->on("/common.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/common.js.gz", "application/javascript");
    });
    _server->on("/wizard.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/wizard.js.gz", "application/javascript");
    });
    _server->on("/dashboard.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/dashboard.js.gz", "application/javascript");
    });
    _server->on("/health.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/health.html.gz", "text/html");
    });
    _server->on("/health.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/health.js.gz", "application/javascript");
    });
}

void WebPortal::_handleRoot(AsyncWebServerRequest* request) {
    WiFiState state = _wifiMgr->getState();
    const char* file;
    if (state == WiFiState::AP_SETUP || state == WiFiState::AP_FALLBACK) {
        file = "/wizard.html.gz";
    } else {
        // STA_CONNECTED, CONNECTING, STA_RECONNECTING — serve dashboard
        file = "/dashboard.html.gz";
    }

    if (!LittleFS.exists(file)) {
        _sendJsonError(request, 404, "Asset not found", "ASSET_MISSING");
        return;
    }

    AsyncWebServerResponse* response = request->beginResponse(LittleFS, file, "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void WebPortal::_handleGetSettings(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    ConfigManager::dumpSettingsJson(data);

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostSettings(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "PARSE_ERROR");
        return;
    }

    if (!doc.is<JsonObject>()) {
        _sendJsonError(request, 400, "Expected JSON object", "INVALID_PAYLOAD");
        return;
    }

    JsonObject settings = doc.as<JsonObject>();
    if (settings.size() == 0) {
        _sendJsonError(request, 400, "Empty settings object", "EMPTY_PAYLOAD");
        return;
    }

    ApplyResult result = ConfigManager::applyJson(settings);
    if (result.applied.size() != settings.size()) {
        _sendJsonError(request, 400, "Unknown or invalid settings key", "INVALID_SETTING");
        return;
    }

    JsonDocument respDoc;
    JsonObject resp = respDoc.to<JsonObject>();
    resp["ok"] = true;
    JsonArray applied = resp["applied"].to<JsonArray>();
    for (const String& key : result.applied) {
        applied.add(key);
    }
    resp["reboot_required"] = result.reboot_required;

    String output;
    serializeJson(respDoc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    FlightStatsSnapshot stats = getFlightStatsSnapshot();
    SystemStatus::toExtendedJson(data, stats);

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostReboot(AsyncWebServerRequest* request) {
    // Flush all pending NVS writes before restart
    ConfigManager::persistAllNow();

    // Notify main coordinator to show "Saving config..." on LED
    if (_rebootCallback) {
        _rebootCallback();
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Rebooting...";

    String output;
    serializeJson(doc, output);

    // Send response first, then schedule restart
    request->send(200, "application/json", output);

    // Delay restart slightly so the HTTP response can be sent
    // Use a one-shot timer to avoid blocking the async callback
    static esp_timer_handle_t rebootTimer = nullptr;
    if (!rebootTimer) {
        esp_timer_create_args_t args = {};
        args.callback = [](void*) { ESP.restart(); };
        args.name = "reboot";
        esp_timer_create(&args, &rebootTimer);
    }
    esp_timer_start_once(rebootTimer, 1000000); // 1 second in microseconds
}

void WebPortal::_handlePostReset(AsyncWebServerRequest* request) {
    // Erase NVS config and restore compile-time defaults
    if (!ConfigManager::factoryReset()) {
        _sendJsonError(request, 500, "Factory reset failed", "RESET_FAILED");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Factory reset complete. Rebooting...";

    String output;
    serializeJson(doc, output);

    // Send response first, then schedule restart (reuse reboot timer pattern)
    request->send(200, "application/json", output);

    static esp_timer_handle_t resetTimer = nullptr;
    if (!resetTimer) {
        esp_timer_create_args_t args = {};
        args.callback = [](void*) { ESP.restart(); };
        args.name = "reset";
        esp_timer_create(&args, &resetTimer);
    }
    esp_timer_start_once(resetTimer, 1000000); // 1 second in microseconds
}

void WebPortal::_handleGetWifiScan(AsyncWebServerRequest* request) {
    int16_t scanResult = WiFi.scanComplete();

    if (scanResult == WIFI_SCAN_FAILED) {
        // No scan running — kick off an async scan
        WiFi.scanNetworks(true); // async=true
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["scanning"] = true;
        root["data"].to<JsonArray>();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    if (scanResult == WIFI_SCAN_RUNNING) {
        // Scan still in progress
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["scanning"] = true;
        root["data"].to<JsonArray>();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    // Scan complete — return results
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["scanning"] = false;
    JsonArray data = root["data"].to<JsonArray>();

    for (int i = 0; i < scanResult; i++) {
        JsonObject net = data.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
    }

    // Free scan memory for next scan
    WiFi.scanDelete();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLayout(AsyncWebServerRequest* request) {
    HardwareConfig hw = ConfigManager::getHardware();
    LayoutResult layout = LayoutEngine::compute(hw);

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    data["mode"] = layout.mode;

    JsonObject matrix = data["matrix"].to<JsonObject>();
    matrix["width"] = layout.matrixWidth;
    matrix["height"] = layout.matrixHeight;

    JsonObject logo = data["logo_zone"].to<JsonObject>();
    logo["x"] = layout.logoZone.x;
    logo["y"] = layout.logoZone.y;
    logo["w"] = layout.logoZone.w;
    logo["h"] = layout.logoZone.h;

    JsonObject flight = data["flight_zone"].to<JsonObject>();
    flight["x"] = layout.flightZone.x;
    flight["y"] = layout.flightZone.y;
    flight["w"] = layout.flightZone.w;
    flight["h"] = layout.flightZone.h;

    JsonObject telemetry = data["telemetry_zone"].to<JsonObject>();
    telemetry["x"] = layout.telemetryZone.x;
    telemetry["y"] = layout.telemetryZone.y;
    telemetry["w"] = layout.telemetryZone.w;
    telemetry["h"] = layout.telemetryZone.h;

    JsonObject hardware = data["hardware"].to<JsonObject>();
    hardware["tiles_x"] = hw.tiles_x;
    hardware["tiles_y"] = hw.tiles_y;
    hardware["tile_pixels"] = hw.tile_pixels;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_serveGzAsset(AsyncWebServerRequest* request, const char* path, const char* contentType) {
    if (!LittleFS.exists(path)) {
        request->send(404, "text/plain", "Not found");
        return;
    }
    AsyncWebServerResponse* response = request->beginResponse(LittleFS, path, contentType);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void WebPortal::_handlePostCalibrationStart(AsyncWebServerRequest* request) {
    if (_calibrationCallback) {
        _calibrationCallback(true);
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Calibration mode started";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostCalibrationStop(AsyncWebServerRequest* request) {
    if (_calibrationCallback) {
        _calibrationCallback(false);
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Calibration mode stopped";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLogos(AsyncWebServerRequest* request) {
    std::vector<LogoEntry> logos;
    if (!LogoManager::listLogos(logos)) {
        _sendJsonError(request, 500, "Logo storage unavailable. Reboot the device and try again.", "STORAGE_UNAVAILABLE");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonArray data = root["data"].to<JsonArray>();

    for (const auto& logo : logos) {
        JsonObject entry = data.add<JsonObject>();
        entry["name"] = logo.name;
        entry["size"] = logo.size;
    }

    // Storage usage metadata
    size_t usedBytes = 0, totalBytes = 0;
    LogoManager::getLittleFSUsage(usedBytes, totalBytes);
    JsonObject storage = root["storage"].to<JsonObject>();
    storage["used"] = usedBytes;
    storage["total"] = totalBytes;
    storage["logo_count"] = LogoManager::getLogoCount();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleDeleteLogo(AsyncWebServerRequest* request) {
    // Extract filename from URL: /api/logos/FILENAME
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0 || lastSlash == (int)(url.length() - 1)) {
        _sendJsonError(request, 400, "Missing logo filename", "MISSING_FILENAME");
        return;
    }
    String filename = url.substring(lastSlash + 1);
    int queryStart = filename.indexOf('?');
    if (queryStart >= 0) {
        filename = filename.substring(0, queryStart);
    }

    if (!LogoManager::isSafeLogoFilename(filename)) {
        _sendJsonError(request, 400, "Invalid logo filename", "INVALID_NAME");
        return;
    }

    if (!LogoManager::hasLogo(filename)) {
        _sendJsonError(request, 404, "Logo not found", "NOT_FOUND");
        return;
    }

    if (!LogoManager::deleteLogo(filename)) {
        _sendJsonError(request, 500, "Could not delete logo. Check storage health and try again.", "FS_DELETE_ERROR");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Logo deleted";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLogoFile(AsyncWebServerRequest* request) {
    // Extract filename from URL: /logos/FILENAME
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0 || lastSlash == (int)(url.length() - 1)) {
        request->send(404, "text/plain", "Not found");
        return;
    }
    String filename = url.substring(lastSlash + 1);
    int queryStart = filename.indexOf('?');
    if (queryStart >= 0) {
        filename = filename.substring(0, queryStart);
    }

    if (!LogoManager::isSafeLogoFilename(filename)) {
        request->send(404, "text/plain", "Not found");
        return;
    }

    String path = String("/logos/") + filename;
    if (!LittleFS.exists(path)) {
        request->send(404, "text/plain", "Not found");
        return;
    }

    request->send(LittleFS, path, "application/octet-stream");
}

void WebPortal::_sendJsonError(AsyncWebServerRequest* request, int httpCode, const char* error, const char* code) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = false;
    root["error"] = error;
    root["code"] = code;
    String output;
    serializeJson(doc, output);
    request->send(httpCode, "application/json", output);
}


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic fn-1 - Story Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during validation of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent story-writing mistakes (unclear AC, missing Notes, unrealistic scope).

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Incorrect Partition Size Specifications in AC2**: Acceptance Criterion 2 listed offset values (0x10000, 0x190000, 0x310000) as if they were sizes, conflicting with the correct size values in the Dev Notes table (0x180000, 0x180000, 0xF0000). This would have caused immediate build failure or incorrect partition table creation. | Updated AC2 to use correct format: "nvs 0x5000/20KB (offset 0x9000), otadata 0x2000/8KB (offset 0xE000), app0 0x180000/1.5MB (offset 0x10000), app1 0x180000/1.5MB (offset 0x190000), spiffs 0xF0000/960KB (offset 0x310000)" |
| high | Ambiguous Documentation Location in AC1**: The phrase "a warning is documented" in AC1 did not specify where or how the warning should be recorded, creating ambiguity for an LLM agent. | Updated AC1 to specify: "a warning is logged to stdout and noted in the 'Completion Notes List' section of this story with optimization recommendations" |
| high | Overly Broad Functionality Verification in AC3**: The criterion "all existing functionality works: flight data pipeline, web dashboard, logo management, calibration tools" lacked specific verification steps. | Updated AC3 to enumerate specific checks: "the following existing functionality is verified to work correctly: device boots successfully; web dashboard accessible at device IP; flight data fetches and displays on LED matrix; logo management functions (upload, list, delete); calibration tools function" |
| high | Missing Runtime Verification in AC4**: AC4 only verified compile-time availability of FW_VERSION but not whether the runtime value matched the build flag. | Updated AC4 to include: "And the version string accurately reflects the value defined in the build flag when accessed at runtime" |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | Ambiguous instruction in Task 5 regarding ConfigSnapshot usage for getSchedule() method | Split Task 5 into two clear sub-tasks: (1) implement getter following direct return pattern with ConfigLockGuard, (2) add ScheduleConfig field to ConfigSnapshot struct for loadFromNvs() operations |

## Story fn-1-3 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing Firmware Integrity Check (SHA256/MD5) | Added AC7 for firmware integrity verification with SHA256 hash, added Task 11 for implementation, added error code INTEGRITY_FAILED, updated Dev Notes with integrity verification section |
| critical | Lack of Version Check/Downgrade Protection | Added AC8 for firmware version validation, added Task 12 for version comparison implementation, added error codes VERSION_CHECK_FAILED and DOWNGRADE_BLOCKED, added Dev Notes section on version validation |
| critical | Missing RAM/Heap Impact Analysis for OTA State | Added subtask to Task 10 to measure and log heap usage during OTA upload, strengthened existing Dev Notes reference to heap constraints |
| high | Explicit Documentation for esp_ota_get_next_update_partition(NULL) | Enhanced AC1 to explicitly reference esp_ota_get_next_update_partition(NULL) |
| high | Specify UI Feedback for Reboot | Added note to AC5 referencing Story fn-1.6 requirement for UI reboot message, added cross-reference in Dependencies section |
| high | Guidance for OTAUploadState Helper Functions | Added comprehensive helper function implementation examples in Dev Notes with findOTAUpload() and clearOTAUpload() code snippets |
| high | Cross-reference UI handling of error messages | Added Dev Notes section on UI Error Handling Requirements with explicit fn-1.6 cross-reference |


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="Architecture (fallback - epics file should have relevant sections)" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="default_output_file">_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md</var>
<var name="description">Create the next user story from epics+stories with enhanced context analysis and direct ready-for-dev marking</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-1</var>
<var name="epics_file" file_id="d7cf6d0a" description="Enhanced epics+stories file with BDD and source hints" load_strategy="EMBEDDED" sharded="true" token_approx="55">embedded in prompt, file id: d7cf6d0a</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="name">create-story</var>
<var name="output_folder">_bmad-output/implementation-artifacts</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="prd_file" description="PRD (fallback - epics file should have most content)" load_strategy="SELECTIVE_LOAD" token_approx="3576">_bmad-output/planning-artifacts/prd-delight-validation-report.md</var>
<var name="project_context">{project-root}/docs/project-context.md</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_dir">_bmad-output/implementation-artifacts/stories</var>
<var name="story_id">fn-1.4</var>
<var name="story_key">fn-1-4-ota-self-check-and-rollback-detection</var>
<var name="story_num">4</var>
<var name="story_title">ota-self-check-and-rollback-detection</var>
<var name="timestamp">20260412_1444</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="ux_file" file_id="02546960" description="UX design (fallback - epics file should have relevant sections)" load_strategy="EMBEDDED" token_approx="30146">embedded in prompt, file id: 02546960</var>
</variables>
<instructions><workflow>
  <critical>🚫 SCOPE LIMITATION: Your ONLY task is to create the story markdown file. Do NOT read, modify, or update any sprint tracking files - that is handled programmatically by bmad-assist.</critical>
<critical>You MUST have already loaded and processed: the &lt;workflow-yaml&gt; section embedded above</critical>
  <critical>Communicate all responses in English and generate all documents in English</critical>

  <critical>🔥 CRITICAL MISSION: You are creating the ULTIMATE story context engine that prevents LLM developer mistakes, omissions or
    disasters! 🔥</critical>
  <critical>Your purpose is NOT to copy from epics - it's to create a comprehensive, optimized story file that gives the DEV agent
    EVERYTHING needed for flawless implementation</critical>
  <critical>COMMON LLM MISTAKES TO PREVENT: reinventing wheels, wrong libraries, wrong file locations, breaking regressions, ignoring UX,
    vague implementations, lying about completion, not learning from past work</critical>
  <critical>🚨 EXHAUSTIVE ANALYSIS REQUIRED: You must thoroughly analyze ALL artifacts to extract critical context - do NOT be lazy or skim!
    This is the most important function in the entire development process!</critical>
  <critical>🔬 UTILIZE SUBPROCESSES AND SUBAGENTS: Use research subagents, subprocesses or parallel processing if available to thoroughly
    analyze different artifacts simultaneously and thoroughly</critical>
  <critical>❓ SAVE QUESTIONS: If you think of questions or clarifications during analysis, save them for the end after the complete story is
    written</critical>
  <critical>🎯 ZERO USER INTERVENTION: Process should be fully automated except for initial epic/story selection or missing documents</critical>
  <critical>📦 Git Intelligence is EMBEDDED in &lt;git-intelligence&gt; at the start of the prompt - do NOT run git commands yourself, use the embedded data instead</critical>

  <step n="1" goal="Architecture analysis for developer guardrails">
    <critical>🏗️ ARCHITECTURE INTELLIGENCE - Extract everything the developer MUST follow!</critical> **ARCHITECTURE DOCUMENT ANALYSIS:** <action>Systematically
    analyze architecture content for story-relevant requirements:</action>

    <!-- Load architecture - single file or sharded -->
    <check if="architecture file is single file">
      <action>Load complete {architecture_content}</action>
    </check>
    <check if="architecture is sharded to folder">
      <action>Load architecture index and scan all architecture files</action>
    </check> **CRITICAL ARCHITECTURE EXTRACTION:** <action>For
    each architecture section, determine if relevant to this story:</action> - **Technical Stack:** Languages, frameworks, libraries with
    versions - **Code Structure:** Folder organization, naming conventions, file patterns - **API Patterns:** Service structure, endpoint
    patterns, data contracts - **Database Schemas:** Tables, relationships, constraints relevant to story - **Security Requirements:**
    Authentication patterns, authorization rules - **Performance Requirements:** Caching strategies, optimization patterns - **Testing
    Standards:** Testing frameworks, coverage expectations, test patterns - **Deployment Patterns:** Environment configurations, build
    processes - **Integration Patterns:** External service integrations, data flows <action>Extract any story-specific requirements that the
    developer MUST follow</action>
    <action>Identify any architectural decisions that override previous patterns</action>
  </step>

  <step n="2" goal="Web research for latest technical specifics">
    <critical>🌐 ENSURE LATEST TECH KNOWLEDGE - Prevent outdated implementations!</critical> **WEB INTELLIGENCE:** <action>Identify specific
    technical areas that require latest version knowledge:</action>

    <!-- Check for libraries/frameworks mentioned in architecture -->
    <action>From architecture analysis, identify specific libraries, APIs, or
    frameworks</action>
    <action>For each critical technology, research latest stable version and key changes:
      - Latest API documentation and breaking changes
      - Security vulnerabilities or updates
      - Performance improvements or deprecations
      - Best practices for current version
    </action>
    **EXTERNAL CONTEXT INCLUSION:** <action>Include in story any critical latest information the developer needs:
      - Specific library versions and why chosen
      - API endpoints with parameters and authentication
      - Recent security patches or considerations
      - Performance optimization techniques
      - Migration considerations if upgrading
    </action>
  </step>

  <step n="3" goal="Create comprehensive story file">
    <critical>📝 CREATE ULTIMATE STORY FILE - The developer's master implementation guide!</critical>

    <action>Create file using the output-template format at: /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md</action>
    <!-- Story foundation from epics analysis -->
    <!-- Developer context section - MOST IMPORTANT PART -->
    <!-- Previous story intelligence -->
    <!-- Git intelligence -->
    <!-- Latest technical specifics -->
    <!-- Project context reference -->
    <!-- Final status update -->
    <!-- CRITICAL: Set status to ready-for-dev -->
    <action>Set story Status to: "ready-for-dev"</action>
    <action>Add completion note: "Ultimate
    context engine analysis completed - comprehensive developer guide created"</action>
  </step>

  <step n="4" goal="Validate and finalize">
    <!-- checklist validation is separate workflow -->
    <action>Save story document unconditionally</action>

    <action>Report completion</action>
    </step>

</workflow></instructions>
<output-template><![CDATA[

# Story {{epic_num}}.{{story_num}}: {{story_title}}

Status: ready-for-dev

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a {{role}},
I want {{action}},
so that {{benefit}}.

## Acceptance Criteria

1. [Add acceptance criteria from epics/PRD]

## Tasks / Subtasks

- [ ] Task 1 (AC: #)
  - [ ] Subtask 1.1
- [ ] Task 2 (AC: #)
  - [ ] Subtask 2.1

## Dev Notes

- Relevant architecture patterns and constraints
- Source tree components to touch
- Testing standards summary

### Project Structure Notes

- Alignment with unified project structure (paths, modules, naming)
- Detected conflicts or variances (with rationale)

### References

- Cite all technical details with source paths and sections, e.g. [Source: docs/&lt;file&gt;.md#Section]

## Dev Agent Record

### Agent Model Used

{{agent_model_name_version}}

### Debug Log References

### Completion Notes List

### File List

]]></output-template>
</compiled-workflow>