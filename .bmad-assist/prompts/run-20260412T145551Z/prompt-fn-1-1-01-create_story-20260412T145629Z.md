<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: fn-1.1 -->
<!-- Phase: create-story -->
<!-- Timestamp: 20260412T145629Z -->
<compiled-workflow>
<mission><![CDATA[

Create the next user story from epics+stories with enhanced context analysis and direct ready-for-dev marking

Target: Story fn-1.1 - partition-table-and-build-configuration
Create comprehensive developer context and implementation-ready story.

]]></mission>
<context>
<file id="git-intel" path="[git-intelligence]"><![CDATA[<git-intelligence>
Git intelligence extracted at compile time. Do NOT run additional git commands - use this embedded data instead.

### Recent Commits (last 5)
```
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

# UX Design Specification — TheFlightWall Delight Release

**Author:** Christian | **Date:** 2026-04-11

## Executive Summary

**Project Vision:** Transform TheFlightWall into ambient intelligence. The Delight Release builds on Foundation's configuration backbone and Display System's pluggable modes. Six capabilities converge: Clock Mode idle fallback, scheduling, transitions, OTA Pull, per-mode settings, and firmware updates. The wall adapts to time of day, fills dead air, transitions gracefully, and self-updates from GitHub.

**Key UX Challenge:** Three interaction cadences coexist in one dashboard: configure-once (Foundation), remote-control mode switching (Display System), and set-and-forget automation (Delight scheduling). Each cadence has different emotional registers and visit frequencies.

**Target Users:**
- **Christian:** Project owner, lived with wall through three releases. Success = visit purpose (enhancing experience), not visit frequency (fixing it). Phone-first, couch interaction.
- **Casey:** Self-service hobbyist, first encounter. Success = dashboard makes sense on first visit: Mode Picker with active mode, optionally discover scheduling. Must not regress Foundation/Display System onboarding.
- **Sam:** Non-technical guest, experiences wall passively. Success = something always displays (Clock fills dead air), changes feel intentional (smooth transitions), wall conveys finished product not prototype.
- **Jordan:** C++ contributor adds custom DisplayMode. UX = architecture docs + it works automatically. Indirect touchpoint: Mode Picker auto-discovers modes, scheduling includes custom modes.

## Core User Experience

### Defining Experiences

**1. The wall runs itself:** Christian configures schedule ("Departures 6am-11pm, Clock overnight"), walks away. Over weeks, wall transitions correctly, falls back to Clock when skies quiet, dims at night. Dashboard unopened for 7 days. Wall earned autonomy.

**2. One-tap firmware update:** Banner: "v2.4.1 available." Christian taps release notes, taps "Update Now." Device unreachable 60s. Wall goes dark. Dashboard shows countdown, polls for reconnection. Flights resume on new version. No USB cable, no PlatformIO.

**3. Silent success:** Christian stops checking if scheduled transitions worked. Week one: watches at 11pm. Week three: forgets to check. Wall just works. Trust converts to autonomy.

### Platform Strategy

| Surface | Changes |
|---|---|
| **Dashboard** | New Scheduling tab, per-mode settings panels, evolved Firmware card (OTA Pull), feature discovery |
| **LED Matrix** | Clock Mode, Departures Board multi-row, animated fade transitions, OTA progress display |
| **APIs** | Schedule (GET/POST rules), Mode Status (effective + reason), OTA (version check, trigger, status) |

**Constraints:**
- LittleFS ~896KB budget, dual-OTA partition table
- No frameworks—vanilla HTML/JS/CSS with existing design system
- ESPAsyncWebServer connection limits—no polling, no WebSocket
- NTP required for scheduling (no RTC on ESP32)
- OTA window: 30-60s device unresponsive, web server blocked
- ~10-12KB additional gzipped assets for Delight UI

**Dashboard section order:** Display → Mode Picker (enhanced) → Scheduling → Timing → Network & API → Firmware (evolved) → Night Mode → Hardware → Calibration → Location → Logos → System

<!-- TRUNCATED: Content exceeded token budget. See full document for details. -->

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
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="Architecture (fallback - epics file should have relevant sections)" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="default_output_file">_bmad-output/implementation-artifacts/stories/fn-1-1-partition-table-and-build-configuration.md</var>
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
<var name="story_id">fn-1.1</var>
<var name="story_key">fn-1-1-partition-table-and-build-configuration</var>
<var name="story_num">1</var>
<var name="story_title">partition-table-and-build-configuration</var>
<var name="timestamp">20260412_1056</var>
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
  <critical>Git Intelligence is EMBEDDED in &lt;git-intelligence&gt; at the start of the prompt - do NOT run git commands yourself, use the embedded data instead</critical>

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

    <action>Create file using the output-template format at: /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-1-partition-table-and-build-configuration.md</action>
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