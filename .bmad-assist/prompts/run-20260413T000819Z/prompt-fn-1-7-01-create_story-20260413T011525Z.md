<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: fn-1.7 -->
<!-- Phase: create-story -->
<!-- Timestamp: 20260413T011525Z -->
<compiled-workflow>
<mission><![CDATA[

Create the next user story from epics+stories with enhanced context analysis and direct ready-for-dev marking

Target: Story fn-1.7 - settings-import-in-setup-wizard
Create comprehensive developer context and implementation-ready story.

]]></mission>
<context>
<file id="git-intel" path="[git-intelligence]"><![CDATA[<git-intelligence>
Git intelligence extracted at compile time. Do NOT run additional git commands - use this embedded data instead.

### Recent Commits (last 5)
```
3c703d8 new commit
898c09f Enhance AGENTS.md with updated BMAD workflow details, including project context handling and bmad-assist commands. Modify bmad-assist.yaml to adjust budget estimates for story creation and validation. Update state.yaml to reflect current epic and story status. Revise sprint-status.yaml to streamline development status and remove outdated comments. Update epics.md to reflect completed and new epics, and adjust firmware partition layout in custom_partitions.csv. Log firmware version at boot in main.cpp.
ffc1cce Update .gitignore to include .env file, enhance AGENTS.md with new user preferences and workspace facts, and modify continual learning state files. Adjust workflow configurations to specify story directories and improve error handling in API fetchers. Add positioning mode to NeoMatrixDisplay and update related documentation.
c5f944e Update AGENTS.md with information on bundled web sources for firmware portal. Modify continual learning state files to reflect the latest run details and add new transcript entries.
6d509a0 Update continual learning state and enhance NeoMatrixDisplay with zone-based rendering. Added telemetry fields and improved logo management. Marked several implementation artifacts as done, including dashboard layout preview and flight rendering features.
```

### Related Story Commits
```
(no output)
```

### Recently Modified Files (excluding docs/)
```
.agent/skills/bmad-advanced-elicitation/methods.csv
.agent/skills/bmad-agent-analyst/bmad-skill-manifest.yaml
.agent/skills/bmad-agent-architect/bmad-skill-manifest.yaml
.agent/skills/bmad-agent-builder/assets/init-sanctum-template.py
.agent/skills/bmad-agent-builder/references/sample-init-sanctum.py
.agent/skills/bmad-agent-builder/scripts/generate-html-report.py
.agent/skills/bmad-agent-builder/scripts/prepass-execution-deps.py
.agent/skills/bmad-agent-builder/scripts/prepass-prompt-metrics.py
.agent/skills/bmad-agent-builder/scripts/prepass-sanctum-architecture.py
.agent/skills/bmad-agent-builder/scripts/prepass-structure-capabilities.py
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

# Architecture Decision Document — TheFlightWall OSS

## Project Context

**Functional Requirements:** 48 FRs across 9 groups: Device Setup & Onboarding (FR1-8), Configuration (FR9-14), Calibration (FR15-18), Flight Data (FR19-25), Logo Display (FR26-29), Responsive Layout (FR30-33), Logo Management (FR34-37), System Ops (FR38-48).

**Key Non-Functional Requirements:**
| NFR | Target | Driver |
|-----|--------|--------|
| Hot-reload latency | <1s | ConfigManager atomic flag, no polling |
| Page load | <3s | Gzipped assets, minimal JS, ~280KB RAM ceiling |
| WiFi recovery | <60s | Auto-reconnect + AP fallback |
| Flash budget | 4MB total: 2MB app + 2MB LittleFS | Monitor usage, gzip web assets |
| Concurrent ops | Web + flight + display | FreeRTOS task pinning: display Core 0, WiFi/web/API Core 1 |

**Technology Stack (Locked):**
- Language: C++11 (Arduino ESP32)
- Platform: ESP32 with PlatformIO
- Dependencies: FastLED ^3.6.0, Adafruit GFX ^1.11.9, FastLED NeoMatrix ^1.2, ArduinoJson ^7.4.2, **ESPAsyncWebServer (mathieucarbou fork) ^3.6.0**, **AsyncTCP (mathieucarbou fork)**, LittleFS (built-in), ESPmDNS (built-in)

## Core Architectural Decisions

### D1: ConfigManager — Singleton with Category Struct Getters
Central singleton initialized first. Config values grouped into structs (DisplayConfig, LocationConfig, HardwareConfig, TimingConfig, NetworkConfig) for clean API and efficient access.

**NVS Key Abbreviations (15-char limit):**
| Key | Type | Default | Category |
|-----|------|---------|----------|
| brightness | uint8 | 5 | display |
| text_color_r/g/b | uint8 | 255 | display |
| center_lat/lon | double | 37.7749/-122.4194 | location |
| radius_km | double | 10.0 | location |
| tiles_x, tiles_y, tile_pixels | uint8 | 10,2,16 | hardware |
| display_pin | uint8 | 25 | hardware |
| origin_corner, scan_dir, zigzag | uint8 | 0 | hardware |
| fetch_interval, display_cycle | uint16 | 30,3 | timing |
| wifi_ssid, wifi_password | string | "" | network |
| os_client_id, os_client_sec | string | "" | network |
| aeroapi_key | string | "" | network |

**Config flow:** Compile-time defaults → NVS read on boot → RAM cache (struct fields) → runtime changes via web UI → debounced NVS write (2s quiet period) → hot-reload via atomic flag (config) or FreeRTOS queue (flight data).

**Reboot-required keys:** wifi_ssid, wifi_password, opensky credentials, aeroapi_key, display_pin. **Hot-reload keys:** brightness, text color, fetch/display intervals, layout params.

### D2: Inter-Task Communication — Hybrid (Atomic Flags + FreeRTOS Queue)
**Config changes:** `std::atomic<bool> configChanged` signals display task to re-read from ConfigManager.
**Flight data:** `QueueHandle_t flightQueue` with `xQueueOverwrite()` — display task always gets latest data.

### D3: WiFi State Machine — WiFiManager
**States:** WIFI_AP_SETUP → WIFI_CONNECTING → WIFI_STA_CONNECTED ↔ WIFI_STA_RECONNECTING → WIFI_AP_FALLBACK

**Transitions:** No WiFi config = AP_SETUP. Connected successfully = STA_CONNECTED. WiFi lost = STA_RECONNECTING (configurable timeout) → AP_FALLBACK. Long press GPIO 0 during boot = force AP_SETUP.

### D4: Web API Endpoints (11 REST)
| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/` | wizard.html (AP) or dashboard.html (STA) |
| GET/POST | `/api/settings` | Get/apply config (reboot flag in response) |
| GET | `/api/status` | System health |
| GET | `/api/wifi/scan` | Async WiFi scan |
| POST | `/api/reboot` | Save + reboot |
| POST | `/api/reset` | Factory reset |
| GET/POST/DELETE | `/api/logos` | Logo management |
| GET | `/api/layout` | Zone layout (initial load) |

Response envelope: `{ "ok": bool, "data": {...} }` or `{ "ok": false, "error": "message", "code": "..." }`

### D5: Component Integration
**Init sequence:** ConfigManager → SystemStatus → LogoManager → LayoutEngine → WiFiManager → WebPortal → FlightDataFetcher → displayTask on Core 0.

**Dependency graph:** ConfigManager (all depend on). DisplayTask is read-only (atomic flag + queue peel). WebPortal is only write path.

### D6: SystemStatus Registry
Health tracking for subsystems (WIFI, OPENSKY, AEROAPI, CDN, NVS, LITTLEFS) with levels (OK/Warning/Error) and human-readable messages. API call counter with monthly NTP-based reset.

### D7: Shared Zone Calculation Algorithm
**Critical:** Implemented identically in C++ (LayoutEngine) and JavaScript (dashboard.js) to ensure canvas preview matches LEDs.

**Test vectors:**
| Config | Matrix | Mode | Logo Zone | Flight Card | Telemetry |
|--------|--------|------|-----------|-------------|-----------|
| 10x2 @ 16px | 160x32 | full | 0,0→32x32 | 32,0→128x16 | 32,16→128x16 |
| 5x2 @ 16px | 80x32 | full | 0,0→32x32 | 32,0→48x16 | 32,16→48x16 |

## Project Structure

**Complete directory tree (44 files, 13 existing → updated, 5 new):**

```
firmware/
├── platformio.ini (board_build.filesystem=littlefs, custom_partitions.csv)
├── custom_partitions.csv (nvs 20KB, otadata 8KB, app0 2MB, spiffs 2MB)
├── src/main.cpp (init sequence, loop, display task, FreeRTOS)
├── core/
│   ├── ConfigManager.h/.cpp
│   ├── FlightDataFetcher.h/.cpp (updated: ConfigManager)
│   ├── LayoutEngine.h/.cpp
│   ├── LogoManager.h/.cpp
│   └── SystemStatus.h/.cpp
├── adapters/
│   ├── NeoMatrixDisplay.h/.cpp
│   ├── WebPortal.h/.cpp (11 endpoints)
│   ├── WiFiManager.h/.cpp
│   ├── OpenSkyFetcher.h/.cpp (updated: ConfigManager)
│   ├── AeroAPIFetcher.h/.cpp (updated: ConfigManager)
│   └── FlightWallFetcher.h/.cpp
├── interfaces/ (unchanged: BaseDisplay, BaseFlightFetcher, BaseStateVectorFetcher)
├── models/ (unchanged: FlightInfo, StateVector, AirportInfo)
├── config/ (unchanged: compile-time defaults, migrated to ConfigManager)
├── utils/
│   └── Log.h (LOG_E/I/V compile-time macros, LOG_LEVEL build flag)
├── data/ (LittleFS: *.gz web assets + logos/)
└── test/ (test_config_manager, test_layout_engine)
```

## Foundation Release — OTA, Night Mode, Settings Export

**4 new NVS keys (schedule):**
| Key | Type | Default | Purpose |
|-----|------|---------|---------|
| timezone | string | "UTC0" | POSIX timezone (from browser-side IANA-to-POSIX mapping) |
| sched_enabled | uint8 | 0 | Schedule enable flag |
| sched_dim_start, sched_dim_end | uint16 | 1380, 420 | Minutes since midnight (23:00 - 07:00) |
| sched_dim_brt | uint8 | 10 | Brightness during dim window |

**F1: Dual-OTA Partition Table**
```
nvs (20KB) | otadata (8KB) | app0 (1.5MB) | app1 (1.5MB) | spiffs (960KB)
```
Flash budget: 4MB total. LittleFS reduces 56% (2MB→960KB). One-time USB reflash required.

**F2: OTA Handler (WebPortal + main.cpp)**
Upload via `POST /api/ota/upload` multipart. Stream-to-partition via `Update.write()` per chunk. No RAM buffering of binary. Validate magic byte 0xE9 on first chunk. Reboot after successful `Update.end(true)`.

**F3: OTA Self-Check — WiFi-OR-Timeout (60s)**
Mark firmware valid when WiFi connects OR after 60-second timeout (whichever first). No self-HTTP-request. Watchdog handles crash-on-boot. If timeout path taken (WiFi down), device remains reachable via AP fallback for re-flash.

**F4: IANA-to-POSIX Timezone Mapping — Browser-Side**
JS object in wizard.js/dashboard.js: `{ "America/Los_Angeles": "PST8PDT,M3.2.0,M11.1.0", ... }`. Auto-detect via `Intl.DateTimeFormat()`. Send POSIX string to ESP32 via POST /api/settings.

**F5: Night Mode Scheduler — Non-Blocking Main Loop**
Schedule times as `uint16` minutes since midnight (0-1439). Midnight-crossing logic: if (dimStart <= dimEnd) then (current >= start && current < end), else (current >= start || current < end). Brightness scheduler overrides ConfigManager brightness. NTP sync via `configTzTime()` after WiFi connect. LWIP auto-resync every 1 hour.

**F6: ConfigManager Expansion — 5 new keys + ScheduleConfig struct**
All hot-reload, no reboot required. Timezone change calls `configTzTime()` immediately.

**F7: API Endpoint Additions**
| Endpoint | Purpose |
|----------|---------|
| POST `/api/ota/upload` | Multipart firmware upload |
| GET `/api/settings/export` | Download JSON config file |
| Updated `/api/status` | +firmware_version, +rollback_detected |

**Settings export:** Flat JSON with `flightwall_settings_version: 1`, all NVS keys. Import is client-side only (wizard pre-fills form fields).

## Display System Release — Mode System, Classic Card, Live Flight Card

**4 new components: DisplayMode interface, ModeRegistry, ClassicCardMode, LiveFlightCardMode.**

### DS1: DisplayMode Interface — Abstract Class with RenderContext
```cpp
class DisplayMode {
    bool init(const RenderContext& ctx);
    void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights);
    void teardown();
    const char* getName() const;
    const ModeZoneDescriptor& getZoneDescriptor() const;  // static metadata
};

struct RenderContext {
    Adafruit_NeoMatrix* matrix;     // GFX primitives
    LayoutResult layout;             // zone bounds
    uint16_t textColor;              // pre-computed
    uint8_t brightness;              // read-only
    uint16_t* logoBuffer;            // shared 2KB
    uint16_t displayCycleMs;         // cycle timing
};
```

**Key rules:** Modes receive RenderContext const ref (cannot modify shared state). Modes own flight cycling state (_currentFlightIndex, _lastCycleMs). Empty flight vector is valid (modes decide idle state). Modes must NOT call FastLED.show() — frame commit is display task's responsibility.

### DS2: ModeRegistry — Static Table with Cooperative Switch Serialization
```cpp
struct ModeEntry {
    const char* id;           // e.g., "classic_card"
    const char* displayName;
    DisplayMode* (*factory)();  // factory function
    uint32_t (*memoryRequirement)();  // static function, no instance
    uint8_t priority;
};

class ModeRegistry {
    static void init(const ModeEntry* table, uint8_t count);
    static bool requestSwitch(const char* modeId);  // Core 1
    static void tick(const RenderContext& ctx, const std::vector<FlightInfo>& flights);  // Core 0
    static DisplayMode* getActiveMode();
    static SwitchState getSwitchState();
};
```

**Switch flow (in tick()):**
1. _switchState = SWITCHING
2. Teardown current mode
3. Heap check: `ESP.getFreeHeap()` vs `memoryRequirement()`
4. If sufficient: factory() → new mode → init(). If init() fails: restore previous.
5. If insufficient: re-init previous, set error
6. Debounced NVS write (2s quiet period)

**Switch restoration pattern:** Teardown-but-don't-delete previous mode shell, enabling safe re-init on failure without re-allocation.

### DS3: NeoMatrixDisplay Responsibility Split
**Retained:** Hardware init, brightness control, frame commit (show()), fallback card rendering, calibration/positioning modes, shared resources (logo buffer, matrix, layout).

**Extracted:** renderZoneFlight(), renderFlightZone(), renderTelemetryZone(), renderLogoZone() → ClassicCardMode.

**New methods:** `RenderContext buildRenderContext()` (assembles context from internals), `show()` (wraps FastLED.show()).

### DS4: Display Task Integration
**In setup():** ModeRegistry::init(MODE_TABLE, MODE_COUNT). Restore last mode from NVS, default to "classic_card".

**In displayTask():**
```cpp
if (g_configChanged.exchange(false)) {
    cachedCtx = g_display.buildRenderContext();
}
if (calibrationMode) { ... }
else if (positioningMode) { ... }
else if (statusMessage) { ... }
else {
    ModeRegistry::tick(cachedCtx, flights);  // mode rendering
}
g_display.show();  // frame commit after mode renders
```

### DS5: Mode Switch API
| Endpoint | Response |
|----------|----------|
| GET `/api/display/modes` | modes[], active, switch_state, upgrade_notification |
| POST `/api/display/mode` | { switching_to: "mode_id" } or error |

Response includes mode metadata (zones, description) for wireframe UI.

### DS6: NVS Persistence — Single Key
**Key:** `display_mode` (string, mode ID). **Default:** `"classic_card"`. **Debounce:** 2s after successful switch. Boot restoration reads from NVS, defaults to classic_card if absent.

### DS7: Mode Picker UI
Dashboard section with CSS Grid wireframe schematics. Each mode card is entire tap target. Tap triggers synchronous POST (returns after mode is rendering or fails). Transition feedback: card in "Switching..." state during POST, sibling cards disabled. All three dismiss actions (Try, Browse, X) clear notification via localStorage + POST /api/display/ack-notification.

**NVS key pattern:** `display_mode` (unique with all MVP + Foundation keys, within namespace "flightwall").

**Enforcement:** ClassicCardMode extraction — 3-phase (copy, validate parity, delete from source). Terminal fallback: if mode init fails twice, displayFallbackCard() renders single-flight legacy card.

## Delight Release — Clock Mode, Departures Board, Scheduling, OTA Pull

**New components: ModeOrchestrator (state machine), OTAUpdater (GitHub API client), ClockMode, DeparturesBoardMode.**

### DL1: Fade Transition — RGB888 Dual Buffers (Transient)
**Buffers:** ~30KB RGB888 (two CRGB arrays, 160x32 @ 3 bytes/pixel). Allocate at transition start, free immediately after fade completes. If malloc() fails: instant cut (graceful degradation, not error).

**Blend:** 15 frames @ 66ms/frame = ~1s. Per-pixel linear interpolation: `r = (out*15 + in*step)/15`.

### DL2: Mode Orchestration — State Machine
**States:** MANUAL (user selection), SCHEDULED (time-based rule active), IDLE_FALLBACK (zero flights).
**Priority:** SCHEDULED > IDLE_FALLBACK > MANUAL.

**ModeOrchestrator::tick() (Core 1, ~1/sec):**
1. Evaluate schedule rules (first match wins)
2. If rule matches: SCHEDULED state, switch to rule mode
3. If no rule && was SCHEDULED: MANUAL state, restore user selection
4. If zero flights && MANUAL state: IDLE_FALLBACK, switch to Clock Mode
5. If flights > 0 && IDLE_FALLBACK state: MANUAL, restore selection

**Flight count signaling:** `std::atomic<uint8_t> g_flightCount` updated by FlightDataFetcher after queue write.

### DL3: OTAUpdater — GitHub Releases API Client with SHA-256
**Check (synchronous, ~1-2s):** GET releases/latest, parse JSON, compare tag_name vs FW_VERSION.

**Download (FreeRTOS task, Core 1):**
1. ModeRegistry::prepareForOTA() (teardown mode, set _switchState = SWITCHING)
2. Download .sha256 (64-char hex string)
3. Update.begin(partitionSize)
4. Stream .bin: Update.write() + mbedtls_sha256_update() per chunk
5. mbedtls_sha256_finish(), compare vs downloaded hash
6. If match: Update.end(true) → ESP.restart(). If mismatch: Update.abort() → ERROR.

**Error paths:** All errors after Update.begin() must call Update.abort(). Task self-deletes via `_downloadTask = nullptr; vTaskDelete(NULL)`.

### DL4: Schedule Rules NVS Storage
**Max 8 rules, indexed keys:**
```
sched_r{N}_start (uint16)    minutes since midnight
sched_r{N}_end (uint16)      minutes since midnight
sched_r{N}_mode (string)     mode ID
sched_r{N}_ena (uint8)       enabled flag
sched_r_count (uint8)        active rule count
```
Rules always compacted (no index gaps on delete).

### DL5: Per-Mode Settings Schema
**Mode declares settings as static const:**
```cpp
struct ModeSettingDef {
    const char* key;      // ≤7 chars (NVS suffix)
    const char* label;    // UI display name
    const char* type;     // "uint8", "enum", etc.
    int32_t defaultValue, minValue, maxValue;
    const char* enumOptions;
};

struct ModeSettingsSchema {
    const char* modeAbbrev;   // ≤5 chars for NVS prefix
    const ModeSettingDef* settings;
    uint8_t settingCount;
};
```

**NVS key format:** `m_{abbrev}_{key}` (total ≤15 chars). ConfigManager helpers: `getModeSetting()` / `setModeSetting()`. API iterates settingsSchema dynamically (never hardcode field names).

### DL6: API Endpoint Additions
| Endpoint | Purpose |
|----------|---------|
| GET `/api/ota/check` | Check GitHub for update |
| POST `/api/ota/pull` | Start download (spawns FreeRTOS task) |
| GET `/api/ota/status` | Poll progress (state, %) |
| GET `/api/schedule` | Get rules |
| POST `/api/schedule` | Save rules |

Updated `/api/display/modes`: includes `settings` array per mode (schema + current values, null for modes without settings).

## Implementation Patterns & Consistency Rules

**MVP Naming Conventions:**
- Classes: PascalCase (ConfigManager, FlightDataFetcher)
- Methods: camelCase (applyJson, fetchFlights)
- Private members: _camelCase (_matrix, _currentFlightIndex)
- Struct fields: snake_case (brightness, text_color_r, center_lat)
- Constants: UPPER_SNAKE_CASE
- Namespaces: PascalCase
- Files: PascalCase.h/.cpp
- Headers: #pragma once

**NVS Key Convention:** snake_case, max 15 chars. Abbreviations documented in ConfigManager.h.

**Logging Pattern:** #include "utils/Log.h", use LOG_E/LOG_I/LOG_V macros (compile-time levels).

**Error Handling:** Boolean return + output parameter (no exceptions). SystemStatus reporting + log macros. JSON API responses: { "ok": bool, "data": {...} } or { "ok": false, "error": "message", "code": "..." }.

**Memory:** String for dynamic text, const char* for constants. Avoid new/delete. Stream LittleFS files.

**Web Assets:** Every fetch() must check json.ok and call showToast() on failure.

**Enforcement Rules (30 total):**

1-11 MVP: naming, NVS keys, includes, memory, logging, error handling, web patterns.

12-16 Foundation: OTA validation (header before write), streaming (no RAM buffer), getLocalTime non-blocking (timeout=0), schedule times as uint16 minutes, settings import client-side only.

17-23 Display System: modes in firmware/modes/, RenderContext isolation (no ConfigManager/WiFiManager from modes), modes never call FastLED.show(), heap allocation only in init/teardown, shared utils from DisplayUtils.h, adding mode = 2 touch points (file + MODE_TABLE line), MEMORY_REQUIREMENT static constexpr.

24-30 Delight: ModeRegistry::requestSwitch() called from exactly 2 methods (ModeOrchestrator::tick/onManualSwitch), OTA SHA-256 incremental + Update.abort() on errors, prepareForOTA() sets _switchState=SWITCHING, fade buffers malloc/free in single call + alloc failure uses instant cut, per-mode settings via ConfigManager helpers + dynamic schema iteration, settings schemas in mode .h files not centralized, cross-core atomics in main.cpp only (modes/adapters don't access directly).

## Validation Summary

**Coverage:** 48 MVP + 37 Foundation + 36 Display System + 43 Delight = 164 FRs. All mapped to specific files and decisions. 21 MVP NFRs + 8 Foundation + 12 Display System + 21 Delight NFRs all addressed architecturally.

**No critical gaps.** Party mode reviews across 3+ sessions incorporated 30+ refinements. Key catches: platformio.ini build filter blocker, DisplayMode interface vs instance methods contradiction, terminal fallback for mode init failure, flight cycling timing as highest-risk extraction point.

**Readiness:** All decisions have code examples, struct definitions, API formats, state diagrams. File change maps complete. Enforcement rules with anti-patterns. Boot/loop integration points specified. Implementation sequence with dependencies. Tests required for ConfigManager, LayoutEngine, ModeRegistry, ModeOrchestrator, OTAUpdater, schedule NVS.

**MVP implementation handoff:** Add ESPAsyncWebServer Carbou fork + LittleFS to platformio.ini, create custom_partitions.csv, build and verify existing flight pipeline still works, then implement ConfigManager + SystemStatus as foundation.

**Foundation handoff:** Pre-implementation size gate (binary ≤1.3MB), then update partitions, expand ConfigManager (schedule), add OTA upload + self-check, integrate NTP + scheduler.

**Display System handoff:** Measure heap baseline post-Foundation, create DisplayMode interface, ModeRegistry, extract ClassicCardMode with pixel-parity validation gate (human confirmation required), then LiveFlightCardMode, then Mode Picker UI.

**Delight handoff:** ModeOrchestrator state machine, ClockMode + DeparturesBoardMode, OTAUpdater with GitHub API + SHA-256, schedule CRUD, mode settings schemas, UI for OTA Pull + scheduling.

]]></file>
<file id="d7cf6d0a" path="_bmad-output/planning-artifacts/epics/index.md" label="EPIC"><![CDATA[

# Epics (sharded)

This index exists so `bmad-assist` treats `epics/` as the primary sharded epic source (see `resolve_doc_path` in bmad-assist). Epic files not linked below are still loaded as orphans in filename order.


]]></file>
<file id="60733e24" path="_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md" label="DOCUMENTATION"><![CDATA[


# Story fn-1.6: Dashboard Firmware Card & OTA Upload UI

Status: Ready for Review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a **user**,
I want **a Firmware card on the dashboard where I can upload firmware, see progress, view the current version, and be notified of rollbacks**,
So that **I can manage firmware updates from my phone**.

## Acceptance Criteria

1. **Given** the dashboard loads **When** `GET /api/status` returns firmware data **Then** the Firmware card displays the current firmware version (e.g., "v1.0.0") **And** the Firmware card is always expanded (not collapsed by default) **And** the dashboard loads within 3 seconds on the local network

2. **Given** the Firmware card is visible **When** no file is selected **Then** the OTA Upload Zone (`.ota-upload-zone`) shows: dashed border, "Drag .bin file here or tap to select" text **And** the zone has `role="button"`, `tabindex="0"`, `aria-label="Select firmware file for upload"` **And** Enter/Space keyboard triggers the file picker

3. **Given** a file is dragged over the upload zone **When** the drag enters the zone **Then** the border becomes solid `--accent` and background lightens slightly

4. **Given** a valid `.bin` file is selected (≤1.5MB, first byte `0xE9`) **When** client-side validation passes **Then** the filename is displayed and the "Upload Firmware" primary button is enabled

5. **Given** an invalid file is selected (wrong size or missing magic byte) **When** client-side validation fails **Then** an error toast fires with a specific message (e.g., "File too large — maximum 1.5MB for OTA partition" or "Not a valid ESP32 firmware image") **And** the upload zone resets to empty state

6. **Given** "Upload Firmware" is tapped **When** the upload begins via XMLHttpRequest **Then** the upload zone is replaced by the OTA Progress Bar (`.ota-progress`) **And** the progress bar shows percentage (0-100%) with `role="progressbar"`, `aria-valuenow`/`min`/`max` **And** the percentage updates at least every 2 seconds

7. **Given** the upload completes successfully **When** the server responds with `{ "ok": true }` **Then** a 3-second countdown displays ("Rebooting in 3... 2... 1...") **And** polling begins for device reachability (`GET /api/status`) **And** on successful poll, the new version string is displayed **And** a success toast shows "Updated to v[new version]"

8. **Given** the device is unreachable after 60 seconds of polling **When** the timeout expires **Then** an amber message shows "Device unreachable — try refreshing" with explanatory text

9. **Given** `rollback_detected` is `true` in `/api/status` **When** the dashboard loads **Then** a Persistent Banner (`.banner-warning`) appears in the Firmware card: "Firmware rolled back to previous version" **And** the banner has `role="alert"`, `aria-live="polite"`, hardcoded `#2d2738` background **And** a dismiss button sends `POST /api/ota/ack-rollback` and removes the banner **And** the banner persists across page refreshes until dismissed (NVS-backed)

10. **Given** the System card on the dashboard **When** rendered **Then** a "Download Settings" secondary button triggers `GET /api/settings/export` **And** helper text in the Firmware card links to the System card: "Before migrating, download your settings backup from System below"

11. **Given** all new CSS for OTA components **When** added to `style.css` **Then** total addition is approximately 35 lines (~150 bytes gzipped) **And** all components meet WCAG 2.1 AA contrast requirements **And** all interactive elements have 44x44px minimum touch targets **And** all components work at 320px minimum viewport width

12. **Given** updated web assets (dashboard.html, dashboard.js, style.css) **When** gzipped and placed in `firmware/data/` **Then** the gzipped copies replace existing ones for LittleFS upload

## Tasks / Subtasks

- [x] **Task 1: Add Firmware card HTML to dashboard.html** (AC: #1, #2, #3, #10)
  - [x] Add new `<section class="card" id="firmware-card">` between the Logos card and System card
  - [x] Card content: `<h2>Firmware</h2>`, version display `<p id="fw-version">`, rollback banner placeholder `<div id="rollback-banner">`, OTA upload zone, progress bar placeholder, reboot countdown placeholder
  - [x] OTA Upload Zone: `<div class="ota-upload-zone" id="ota-upload-zone" role="button" tabindex="0" aria-label="Select firmware file for upload">` with prompt text "Drag .bin file here or tap to select" and hidden file input `<input type="file" id="ota-file-input" accept=".bin" hidden>`
  - [x] File selection display: `<div class="ota-file-info" id="ota-file-info" style="display:none">` with filename span and "Upload Firmware" primary button
  - [x] Progress bar: `<div class="ota-progress" id="ota-progress" style="display:none">` with inner bar, percentage text, and `role="progressbar"` attributes
  - [x] Reboot countdown: `<div class="ota-reboot" id="ota-reboot" style="display:none">` with countdown text
  - [x] Add helper text: `<p class="helper-copy">Before migrating, download your settings backup from <a href="#system-card">System</a> below.</p>`
  - [x] Add `id="system-card"` to the existing System card `<section>` for the anchor link

- [x] **Task 2: Add "Download Settings" button to System card** (AC: #10)
  - [x] In the System card section (currently has only Reset to Defaults), add a "Download Settings" secondary button: `<button type="button" class="btn-secondary" id="btn-export-settings">Download Settings</button>`
  - [x] Add helper text below: `<p class="helper-copy">Download your configuration as a JSON file before partition migration or reflash.</p>`
  - [x] Place this ABOVE the danger zone reset button (less destructive action first)

- [x] **Task 3: Add new endpoint POST /api/ota/ack-rollback** (AC: #9)
  - [x] In `WebPortal.cpp`, in `_registerRoutes()`, add handler for `POST /api/ota/ack-rollback`
  - [x] Handler: set NVS key `ota_rb_ack` to `1` (uint8), respond `{ "ok": true }`
  - [x] In `WebPortal.h`, no new method needed — can be inline lambda in `_registerRoutes()` (same pattern as positioning handlers)

- [x] **Task 4: Add GET /api/settings/export endpoint** (AC: #10)
  - [x] In `WebPortal.cpp`, in `_registerRoutes()`, add handler for `GET /api/settings/export`
  - [x] Handler builds JSON using existing `ConfigManager::dumpSettingsJson()` method
  - [x] Add metadata fields: `"flightwall_settings_version": 1`, `"exported_at": "<ISO-8601 timestamp>"` (use `time()` with NTP or `millis()` fallback)
  - [x] Set response header: `Content-Disposition: attachment; filename=flightwall-settings.json`
  - [x] Response content-type: `application/json`

- [x] **Task 5: Expose rollback acknowledgment state in /api/status** (AC: #9)
  - [x] Add NVS key `ota_rb_ack` (uint8, default 0) — stored directly via `Preferences` in WebPortal (not via ConfigManager; this is a simple transient flag, not a user config key)
  - [x] In `SystemStatus::toExtendedJson()`, add `"rollback_acknowledged"` field: `true` if NVS key `ota_rb_ack` == 1
  - [x] When a NEW OTA upload completes successfully (fn-1.3 handler, after `Update.end(true)` returns true and before the reboot delay), clear the `ota_rb_ack` key by setting it to `0` — so a subsequent rollback will show the banner again. Do NOT clear at upload start; if the upload fails the ack must remain intact.

- [x] **Task 6: Add OTA CSS to style.css** (AC: #11)
  - [x] `.ota-upload-zone` — Reuse pattern from `.logo-upload-zone`: dashed border `2px dashed var(--border)`, padding, min-height 80px, cursor pointer, text-align center, flex column
  - [x] `.ota-upload-zone.drag-over` — Solid border `var(--primary)`, background rgba tint
  - [x] `.ota-file-info` — Filename + button row; flexbox with gap
  - [x] `.ota-progress` — Container for progress bar: height 28px, border-radius, background `var(--border)`
  - [x] `.ota-progress-bar` — Inner fill: height 100%, background `var(--primary)`, transition width 0.3s, border-radius
  - [x] `.ota-progress-text` — Centered percentage text over bar: position absolute, color white
  - [x] `.ota-reboot` — Countdown text: text-align center, padding, font-size larger
  - [x] `.banner-warning` — Persistent amber banner: background `#2d2738`, border-left 4px solid `var(--warning, #d29922)`, padding 12px 16px, border-radius, display flex, justify-content space-between, align-items center
  - [x] `.banner-warning .dismiss-btn` — Small X button: min 44x44px touch target
  - [x] `.btn-secondary` — If not already existing: outline style button, border `var(--border)`, background transparent, color `var(--text)`
  - [x] Target: ~35 lines, ~150 bytes gzipped
  - [x] All interactive elements ≥ 44x44px touch targets
  - [x] Verify contrast ratios meet WCAG 2.1 AA (4.5:1 for text, 3:1 for UI components)

- [x] **Task 7: Implement firmware card JavaScript in dashboard.js** (AC: #1, #2, #3, #4, #5, #6, #7, #8, #9)
  - [x] **Version display**: In `loadSettings()` or a new `loadFirmwareStatus()` function, call `GET /api/status`, extract `firmware_version` and `rollback_detected`, populate `#fw-version` text
  - [x] **Rollback banner**: If `rollback_detected === true` AND not acknowledged (check via `rollback_acknowledged` field in status response), show `.banner-warning` with dismiss button. Dismiss handler: `FW.post('/api/ota/ack-rollback')`, then hide banner.
  - [x] **File selection via click**: Click on `.ota-upload-zone` or Enter/Space key → trigger hidden `#ota-file-input` click
  - [x] **Drag and drop**: `dragenter`/`dragover` on zone → add `.drag-over` class, prevent default. `dragleave`/`drop` → remove class. On `drop`, extract `e.dataTransfer.files[0]`
  - [x] **Client-side validation**: Read first 4 bytes via `FileReader.readAsArrayBuffer()`. Check: (a) file size ≤ 1,572,864 bytes (1.5MB = 0x180000), (b) first byte === 0xE9 (ESP32 magic). On failure: `FW.showToast(message, 'error')` and reset zone.
  - [x] **File info display**: On valid file, hide upload zone, show `#ota-file-info` with filename and enabled "Upload Firmware" button
  - [x] **Upload via XMLHttpRequest**: Use `XMLHttpRequest` (not `fetch`) for upload progress events. `xhr.upload.onprogress` → update progress bar width and `aria-valuenow`. `FormData` with file appended. POST to `/api/ota/upload`.
  - [x] **Progress bar**: Hide file info, show `#ota-progress`. Update bar width as percentage. Ensure updates at least every 2 seconds (XHR progress events are browser-driven; add a minimum interval check if needed).
  - [x] **Success handler**: On `xhr.status === 200` and `response.ok === true`, show countdown "Rebooting in 3... 2... 1..." via `setInterval` decrementing. After countdown, begin polling.
  - [x] **Reboot polling**: `setInterval` every 3 seconds calling `GET /api/status`. On success: extract new `firmware_version`, show success toast "Updated to v[version]", update version display, reset OTA zone to initial state. Clear interval.
  - [x] **Polling timeout**: After 60 seconds (20 attempts at 3s each), stop polling, show amber message "Device unreachable — try refreshing. The device may have changed IP address after reboot."
  - [x] **Error handling**: On upload failure (non-200 response), parse error JSON, show error toast with specific message from server. Reset upload zone to initial state.
  - [x] **Cancel/reset**: Provide a way to cancel file selection (click zone again or select new file)

- [x] **Task 8: Add settings export JavaScript** (AC: #10)
  - [x] Click handler on `#btn-export-settings`: trigger `window.location.href = '/api/settings/export'` — browser downloads the JSON file via Content-Disposition header
  - [x] Alternative: `fetch('/api/settings/export')` then create blob URL and trigger download — but direct navigation is simpler and works universally

- [x] **Task 9: Gzip updated web assets** (AC: #12)
  - [x] From `firmware/` directory, regenerate gzipped assets:
    ```
    gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
    gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz
    gzip -9 -c data-src/style.css > data/style.css.gz
    ```
  - [x] Verify gzipped files replaced in `firmware/data/`

- [x] **Task 10: Build and verify** (AC: #1-#12)
  - [x] Run `~/.platformio/penv/bin/pio run` from `firmware/` — verify clean build
  - [x] Verify binary size remains under 1.5MB limit
  - [x] Measure total gzipped web asset size (should remain well under 50KB budget)

## Dev Notes

### Critical Architecture Constraints

**Web Asset Build Pipeline (MANUAL)**
Per project memory: No automated gzip build script. After editing any file in `firmware/data-src/`, manually run:
```bash
cd firmware
gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz  
gzip -9 -c data-src/style.css > data/style.css.gz
```

**Dashboard Card Ordering**
The dashboard currently has these cards in order:
1. Display
2. Display Mode (Mode Picker)
3. Timing
4. Network & API
5. Hardware
6. Calibration (collapsible)
7. Location (collapsible)
8. Logos
9. System (danger zone)

The Firmware card should be inserted between **Logos** and **System** — this matches the UX spec's dashboard ordering (Firmware before System) and keeps the danger zone at the bottom.

**Existing API Patterns**
- All API calls use `FW.get()` / `FW.post()` from `common.js`
- Error responses always: `{ "ok": false, "error": "message", "code": "CODE" }`
- Success responses: `{ "ok": true, "data": {...} }` or `{ "ok": true, "message": "..." }`
- Toast notifications: `FW.showToast(message, 'success'|'warning'|'error')`

**OTA Upload is ALREADY IMPLEMENTED server-side (fn-1.3)**
The `POST /api/ota/upload` endpoint exists in `WebPortal.cpp`. It:
- Accepts multipart form upload
- Validates magic byte `0xE9` on first chunk
- Streams directly to flash via `Update.write()`
- Returns `{ "ok": true, "message": "Rebooting..." }` on success
- Returns error with code (INVALID_FIRMWARE, OTA_BUSY, etc.) on failure
- Reboots after 500ms delay on success

**You do NOT need to modify the upload endpoint** — only create the client-side UI.

**OTA Self-Check (fn-1.4) provides version and rollback data**
`GET /api/status` returns:
```json
{
  "ok": true,
  "data": {
    "firmware_version": "1.0.0",
    "rollback_detected": false,
    ...
  }
}
```

**Rollback persistence behavior (fn-1.4 Gotcha #2):**
`rollback_detected` remains `true` on every boot until a new successful OTA clears the invalid partition. The banner must handle this — show persistently until user acknowledges via `POST /api/ota/ack-rollback`.

### XMLHttpRequest vs Fetch for Upload

Use `XMLHttpRequest` instead of `fetch()` for the firmware upload because:
1. `xhr.upload.onprogress` provides real upload progress events (bytes sent / total)
2. `fetch()` does not support upload progress in any browser
3. The existing logo upload in `dashboard.js` uses `fetch()` but without progress — OTA needs progress

**Pattern:**
```javascript
function uploadFirmware(file) {
  const xhr = new XMLHttpRequest();
  const formData = new FormData();
  formData.append('firmware', file, file.name);
  
  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      updateProgressBar(pct);
    }
  };
  
  xhr.onload = function() {
    if (xhr.status === 200) {
      const resp = JSON.parse(xhr.responseText);
      if (resp.ok) startRebootCountdown();
      else showUploadError(resp.error);
    } else {
      try {
        const resp = JSON.parse(xhr.responseText);
        showUploadError(resp.error || 'Upload failed');
      } catch(e) {
        showUploadError('Upload failed — status ' + xhr.status);
      }
    }
  };
  
  xhr.onerror = function() {
    showUploadError('Connection lost during upload');
  };
  
  xhr.open('POST', '/api/ota/upload');
  xhr.send(formData);
}
```

### Client-Side File Validation

The ESP32 magic byte validation MUST happen client-side before upload to provide immediate feedback (AC #5). Read the first byte using FileReader:

```javascript
function validateFirmwareFile(file) {
  return new Promise((resolve, reject) => {
    if (file.size > 1572864) {  // 1.5MB = 0x180000
      reject('File too large — maximum 1.5MB for OTA partition');
      return;
    }
    
    const reader = new FileReader();
    reader.onload = function(e) {
      const bytes = new Uint8Array(e.target.result);
      if (bytes[0] !== 0xE9) {
        reject('Not a valid ESP32 firmware image');
        return;
      }
      resolve();
    };
    reader.onerror = function() {
      reject('Could not read file');
    };
    reader.readAsArrayBuffer(file.slice(0, 4));  // Only read first 4 bytes
  });
}
```

### Reboot Polling Strategy

After successful upload, the device reboots after 500ms. The dashboard must:
1. Show 3-second countdown ("Rebooting in 3... 2... 1...")
2. After countdown, poll `GET /api/status` every 3 seconds
3. On success: extract `firmware_version`, show success toast
4. On timeout (60s / 20 polls): show amber warning

The device typically takes 5-15s to reboot and reconnect to WiFi. Total expected wait: countdown (3s) + reboot (5-15s) = 8-18s.

**Important:** The device's IP may change after reboot if DHCP reassigns. The polling should use the current window location's hostname. If the device got a new IP, the polling will time out and the user needs to find the new IP (via router or mDNS `flightwall.local`).

### NVS Key for Rollback Acknowledgment

**Key:** `ota_rb_ack` (uint8, default 0)
- Set to `1` when user dismisses the rollback banner (POST /api/ota/ack-rollback)
- Reset to `0` when a new OTA upload begins (in the OTA upload handler, fn-1.3 code)
- Read in `/api/status` response or as a separate query

**Implementation choice:** Rather than adding this to ConfigManager's formal config structs (overkill for a simple flag), use direct NVS access in WebPortal:
```cpp
#include <Preferences.h>
// In ack-rollback handler:
Preferences prefs;
prefs.begin("flightwall", false);
prefs.putUChar("ota_rb_ack", 1);
prefs.end();
```

And expose in the status response by adding to `FlightStatsSnapshot`:
```cpp
// In main.cpp getFlightStatsSnapshot():
Preferences prefs;
prefs.begin("flightwall", true);  // read-only
s.rollback_acked = prefs.getUChar("ota_rb_ack", 0) == 1;
prefs.end();
```

Then in `SystemStatus::toExtendedJson()`:
```cpp
obj["rollback_acknowledged"] = stats.rollback_acked;
```

**ALTERNATIVE (simpler):** Add the ack check directly in the ack-rollback POST handler and the status GET handler in WebPortal.cpp, avoiding any changes to FlightStatsSnapshot or SystemStatus. The handler reads/writes NVS directly. The status handler reads NVS when building the response. This keeps the scope minimal.

### Settings Export Endpoint

The data layer already exists: `ConfigManager::dumpSettingsJson(JsonObject& out)` writes all config keys to a JSON object. The endpoint just needs to:
1. Create a JsonDocument
2. Call `dumpSettingsJson()` on it
3. Add metadata (`flightwall_settings_version`, `exported_at`)
4. Set Content-Disposition header
5. Send response

```cpp
// In _registerRoutes():
_server->on("/api/settings/export", HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["flightwall_settings_version"] = 1;
    
    // Timestamp — use NTP time if available, else uptime
    time_t now;
    time(&now);
    if (now > 1000000000) {  // NTP synced (past year 2001)
        char buf[32];
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
        root["exported_at"] = String(buf);
    } else {
        root["exported_at"] = String("uptime_ms_") + String(millis());
    }
    
    ConfigManager::dumpSettingsJson(root);
    
    String output;
    serializeJson(doc, output);
    
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", output);
    response->addHeader("Content-Disposition", "attachment; filename=flightwall-settings.json");
    request->send(response);
});
```

### Reset ota_rb_ack on New OTA Upload

When a new OTA upload **completes successfully** (in the existing upload handler in WebPortal.cpp, after `Update.end(true)` returns `true` and before the 500ms reboot delay), clear the rollback ack:
```cpp
// In the OTA upload handler, after Update.end(true) succeeds, before ESP.restart():
Preferences prefs;
prefs.begin("flightwall", false);
prefs.putUChar("ota_rb_ack", 0);
prefs.end();
```

**Important:** Clear the flag only on successful completion — NOT at upload start (`index == 0`). If the upload fails mid-stream, the existing acknowledgment must remain so the rollback banner stays dismissed until the user explicitly re-acknowledges or a good OTA succeeds.

### Dashboard Load Sequence

The dashboard currently calls `loadSettings()` on DOMContentLoaded. For the Firmware card, add a new `loadFirmwareStatus()` call that:
1. Fetches `GET /api/status` (the health endpoint already exists)
2. Extracts `firmware_version` → displays in `#fw-version`
3. Extracts `rollback_detected` and `rollback_acknowledged` → shows/hides banner
4. This can run in parallel with `loadSettings()` since they hit different endpoints

### CSS Design System Tokens to Reuse

From the existing `style.css`:
- `--bg: #0d1117` — background
- `--surface: #161b22` — card background
- `--border: #30363d` — borders, dashed upload zone border
- `--text: #e6edf3` — primary text
- `--text-muted: #8b949e` — helper text, secondary info
- `--primary: #58a6ff` — buttons, progress bar fill, drag-over border
- `--error: #f85149` — error states
- `--success: #3fb950` — success toast
- `--radius: 8px` — border radius
- `--gap: 16px` — spacing

New token: rollback banner background `#2d2738` (hardcoded per AC #9).

### Accessibility Requirements Summary

- Upload zone: `role="button"`, `tabindex="0"`, `aria-label`
- Progress bar: `role="progressbar"`, `aria-valuenow`, `aria-valuemin="0"`, `aria-valuemax="100"`
- Rollback banner: `role="alert"`, `aria-live="polite"`
- All touch targets ≥ 44x44px
- Works at 320px viewport width
- WCAG 2.1 AA contrast ratios

### Project Structure Notes

**Files to modify:**
1. `firmware/data-src/dashboard.html` — Add Firmware card HTML, System card ID, Download Settings button
2. `firmware/data-src/dashboard.js` — Add firmware status loading, OTA upload UI logic, settings export trigger
3. `firmware/data-src/style.css` — Add OTA component styles (~35 lines)
4. `firmware/adapters/WebPortal.cpp` — Add `POST /api/ota/ack-rollback` and `GET /api/settings/export` endpoints, clear `ota_rb_ack` on successful OTA completion (before reboot)
5. `firmware/core/SystemStatus.cpp` — Add `rollback_acknowledged` to `toExtendedJson()` (or handle in WebPortal directly)
6. `firmware/data/dashboard.html.gz` — Regenerated
7. `firmware/data/dashboard.js.gz` — Regenerated
8. `firmware/data/style.css.gz` — Regenerated

**Files NOT to modify:**
- `firmware/adapters/WebPortal.h` — No new class methods needed; new endpoints use inline lambdas
- `firmware/core/ConfigManager.h/.cpp` — `ota_rb_ack` is a simple NVS flag, not a formal config key
- `firmware/src/main.cpp` — No changes needed; `FlightStatsSnapshot` could optionally be extended but the simpler approach is NVS-direct in WebPortal
- `firmware/data-src/common.js` — `FW.get/post/showToast` already sufficient
- `firmware/data-src/health.html/.js` — Health page already displays `firmware_version` from `/api/status`

### References

- [Source: epic-fn-1.md#Story fn-1.6 — All acceptance criteria]
- [Source: architecture.md#D4 — REST endpoint patterns, JSON envelope]
- [Source: architecture.md#F7 — POST /api/ota/upload, GET /api/settings/export endpoints]
- [Source: prd.md#Update Mechanism — OTA via web UI]
- [Source: prd.md#FR10 — Persist configuration to NVS]
- [Source: prd.md#NFR Performance — Dashboard loads within 3 seconds]
- [Source: ux-design-specification-delight.md#OTA Progress Display — States and patterns]
- [Source: fn-1-3 story — OTA upload endpoint implementation details]
- [Source: fn-1-4 story — Rollback detection, firmware_version in /api/status]
- [Source: WebPortal.cpp lines 458-620 — Existing OTA upload handler]
- [Source: SystemStatus.cpp lines 89-90 — firmware_version/rollback_detected in toExtendedJson()]
- [Source: ConfigManager.cpp line 469 — dumpSettingsJson() for settings export]
- [Source: dashboard.js — Logo upload zone pattern for reuse]

### Dependencies

**This story depends on:**
- fn-1.1 (Partition Table) — COMPLETE (partition size 0x180000 = 1.5MB used for file validation)
- fn-1.3 (OTA Upload Endpoint) — COMPLETE (POST /api/ota/upload server-side handler)
- fn-1.4 (OTA Self-Check) — COMPLETE (firmware_version and rollback_detected in /api/status)

**Stories that depend on this:**
- fn-1.7: Settings Import in Setup Wizard — uses the exported JSON format from `/api/settings/export`

**Cross-reference (same epic, not a dependency):**
- fn-1.5: Settings Export Endpoint — marked "done" in sprint status, but `GET /api/settings/export` is NOT yet implemented in WebPortal.cpp. This story (fn-1.6) implements that endpoint as Task 4. The `ConfigManager::dumpSettingsJson()` data layer method exists from fn-1.2.

### Previous Story Intelligence

**From fn-1.1:** Binary size 1.15MB (76.8%). Partition size 0x180000 (1,572,864 bytes) — this is the file size limit for client-side validation. `FW_VERSION` build flag established.

**From fn-1.2:** `ConfigManager::dumpSettingsJson()` method exists and dumps all 27+ config keys as flat JSON. NVS namespace is `"flightwall"`. SUBSYSTEM_COUNT = 8.

**From fn-1.3:** OTA upload endpoint complete. Key patterns:
- `g_otaInProgress` flag prevents concurrent uploads (returns 409)
- `clearOTAUpload()` resets `g_otaInProgress = false` on completion/error
- After `Update.end(true)`, `state->started = false` to prevent cleanup from aborting
- 500ms delay before `ESP.restart()` allows response transmission
- Error codes: INVALID_FIRMWARE, NO_OTA_PARTITION, BEGIN_FAILED, WRITE_FAILED, VERIFY_FAILED, OTA_BUSY

**From fn-1.4:** `rollback_detected` persists across reboots until next successful OTA. `firmware_version` comes from `FW_VERSION` build flag. Both are in `/api/status` response under `data` object.

**Antipatterns from previous stories to avoid:**
- Don't hardcode version strings — always use the API response value
- Don't assume the device IP stays the same after reboot (DHCP reassignment possible)
- Ensure all error paths reset the upload UI to initial state (learned from fn-1.3 cleanup patterns)
- Use named String variables for `SystemStatus::set()` calls — avoid `.c_str()` on temporaries

### Existing Logo Upload Zone Pattern (Reuse Reference)

The existing logo upload zone in `dashboard.html` and `dashboard.js` provides the exact pattern to follow:

**HTML (lines 218-224 of dashboard.html):**
```html
<div class="logo-upload-zone" id="logo-upload-zone">
  <p class="upload-prompt">Drag & drop .bin logo files here or</p>
  <label class="upload-label" for="logo-file-input">Choose files</label>
  <input type="file" id="logo-file-input" accept=".bin" multiple hidden>
  <p class="upload-hint">32×32 RGB565, 2048 bytes each</p>
</div>
```

**CSS (style.css) — `.logo-upload-zone` styles are already defined and can be reused/adapted for `.ota-upload-zone`.**

**JS patterns in dashboard.js:**
- Drag events: `dragenter`, `dragover` (prevent default), `dragleave`, `drop`
- File validation before upload
- Upload via fetch/FormData
- Error handling with `FW.showToast()`

The OTA upload zone differs from logos in:
1. Single file only (no `multiple` attribute)
2. Uses XMLHttpRequest instead of fetch (for progress events)
3. Has a progress bar phase
4. Has a reboot countdown phase
5. Binary validation (magic byte check via FileReader)

### Testing Strategy

**Build verification:**
```bash
cd firmware && ~/.platformio/penv/bin/pio run
# Expect: SUCCESS, binary < 1.5MB
```

**Gzip verification:**
```bash
cd firmware
gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz
gzip -9 -c data-src/style.css > data/style.css.gz
ls -la data/dashboard.html.gz data/dashboard.js.gz data/style.css.gz
# Verify files exist and are reasonable size
```

**Manual test cases:**

1. **Dashboard load — version display:**
   - Load dashboard in browser
   - Expect: Firmware card visible with version (e.g., "v1.0.0")
   - Expect: Card is expanded, not collapsed

2. **Upload zone — drag and drop:**
   - Drag a .bin file over the upload zone
   - Expect: Border becomes solid, background lightens
   - Drop file
   - Expect: If valid, filename shown with "Upload Firmware" button
   - Expect: If invalid, error toast and zone resets

3. **Upload zone — click to select:**
   - Click/tap the upload zone
   - Expect: File picker opens

4. **Upload zone — keyboard:**
   - Tab to upload zone, press Enter or Space
   - Expect: File picker opens

5. **Client-side validation — oversized file:**
   - Select a file > 1.5MB
   - Expect: Toast "File too large — maximum 1.5MB for OTA partition"

6. **Client-side validation — wrong magic byte:**
   - Select a non-firmware .bin file (e.g., a logo file)
   - Expect: Toast "Not a valid ESP32 firmware image"

7. **Successful upload flow:**
   - Select valid firmware file, tap "Upload Firmware"
   - Expect: Progress bar appears, fills 0-100%
   - Expect: "Rebooting in 3... 2... 1..." countdown
   - Expect: Polling starts, eventually shows "Updated to v[version]"

8. **Upload error handling:**
   - Upload while another upload is in progress (if possible)
   - Expect: Error toast with server error message

9. **Rollback banner:**
   - If `rollback_detected` is true, expect persistent amber banner
   - Click dismiss → banner disappears
   - Refresh page → banner stays dismissed

10. **Download Settings:**
    - Click "Download Settings" in System card
    - Expect: JSON file downloads with all config keys

11. **Responsive — 320px viewport:**
    - Resize browser to 320px width
    - Expect: All Firmware card elements visible and usable

### Risk Mitigation

1. **Dashboard.js file size:** At 87KB, this is already the largest web asset. Adding ~200 lines of OTA UI logic is proportional (~3% increase). Monitor gzip output stays reasonable.

2. **XHR vs Fetch consistency:** The rest of the dashboard uses `FW.get()`/`FW.post()` from common.js (which use `fetch`). The OTA upload is the only place using raw `XMLHttpRequest`. Document this inconsistency clearly in a comment: `// XHR required for upload progress events — fetch() does not support upload progress`

3. **IP change after reboot:** If DHCP assigns a new IP, polling will fail after 60s. The timeout message should suggest checking the router's device list or using mDNS (`flightwall.local`).

4. **NVS write during OTA:** The `ota_rb_ack` NVS write in the ack-rollback handler is small and fast. The NVS write clearing the flag on successful OTA completion happens after `Update.end(true)` but before `ESP.restart()`, when no streaming is active — no heap conflict.

5. **Settings export without NTP:** If NTP hasn't synced, `time()` returns near-zero. The export includes an `exported_at` field — use an uptime-based fallback string rather than a misleading timestamp.

## Dev Agent Record

### Agent Model Used

Claude Opus 4 (Story Creation), Claude Opus 4.6 (Implementation)

### Debug Log References

N/A

### Completion Notes List

**2026-04-12: Implementation completed**
- All 10 tasks and subtasks implemented and verified
- Firmware card HTML added between Logos and System cards with full accessibility attributes (role, tabindex, aria-label, aria-live)
- OTA upload zone with drag-and-drop, click, and keyboard (Enter/Space) triggers
- Client-side validation: file size ≤1.5MB check, ESP32 magic byte (0xE9) check via FileReader
- XHR-based upload with real-time progress bar (not fetch — XHR needed for upload.onprogress)
- 3-second reboot countdown + polling (3s intervals, 60s timeout with amber warning)
- Rollback banner with NVS-backed persistence (ota_rb_ack key), dismiss via POST /api/ota/ack-rollback
- ota_rb_ack cleared only on successful OTA completion (not at upload start)
- GET /api/settings/export endpoint with Content-Disposition header, metadata fields, NTP-aware timestamp
- Download Settings button in System card (above danger zone)
- ~35 lines CSS added for OTA components, banner-warning, btn-secondary
- Build succeeds: 1.17MB binary (78.1%), 38KB total gzipped web assets
- No regressions — all existing code paths preserved

**2026-04-12: Ultimate context engine analysis completed**
- Comprehensive analysis of epic-fn-1.md extracted all 12 acceptance criteria with BDD format
- Full dashboard.html structure analyzed (258 lines, 9 cards) — Firmware card insertion point identified between Logos and System
- Full dashboard.js analyzed (87KB) — existing patterns for upload zones, settings loading, toast notifications, and mode picker documented
- Full style.css analyzed (20KB) — CSS custom properties and component classes documented for reuse
- WebPortal.cpp analyzed (1065 lines) — existing OTA upload handler (fn-1.3) fully mapped, new endpoint insertion points identified
- SystemStatus.cpp analyzed — firmware_version and rollback_detected already in toExtendedJson() (fn-1.4)
- ConfigManager.cpp analyzed — dumpSettingsJson() method exists for settings export data layer
- common.js analyzed — FW.get/post/del/showToast API documented
- All 4 prerequisite stories (fn-1.1 through fn-1.4) completion notes reviewed for binary size, patterns, and lessons learned
- fn-1.5 discrepancy noted: marked "done" in sprint-status.yaml but GET /api/settings/export endpoint NOT implemented in WebPortal.cpp; this story includes that implementation
- 10 tasks created with explicit code references, patterns, and implementation guidance
- XMLHttpRequest usage justified for upload progress events (fetch limitation)
- Rollback banner NVS persistence pattern designed (ota_rb_ack key)
- Client-side validation patterns provided with exact byte values and FileReader approach
- Antipatterns from fn-1.3/fn-1.4 incorporated (cleanup patterns, temporary String avoidance)

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Validation synthesis: removed conflicting localStorage reference from Task 5; corrected ota_rb_ack reset timing from upload-start to successful-completion in Task 5, Dev Notes, Project Structure, and Risk Mitigation | BMad |
| 2026-04-12 | All 10 tasks implemented: Firmware card HTML/CSS/JS, OTA upload UI with progress, rollback banner, settings export endpoint, ack-rollback endpoint. Build verified at 1.17MB (78.1%). | Claude Opus 4.6 |
| 2026-04-12 | Code review synthesis fixes: cancel button for file-info state, polling race condition guard, resetOtaUploadState color/text reset, zero-byte file check, dragleave child-element fix, dismiss-btn focus style, NVS write error checking. Build re-verified at 1.17MB (78.1%). | AI Synthesis |
| 2026-04-12 | Second code review synthesis fixes: WCAG AA contrast fix for progress text (text-shadow), upload button disabled during upload, cache-busting for polling requests, --warning CSS variable added. Build re-verified at 1.22MB (77.7%). | AI Synthesis |
| 2026-04-12 | Third code review synthesis fixes: drag-over border-style solid (AC #3), touch target min-height for OTA buttons (AC #11), NVS begin() check in OTA success path, loadFirmwareStatus error toast, XHR timeout (120s), polling converted from setInterval to recursive setTimeout. Build re-verified at 1.17MB (78.2%). | AI Synthesis |

### File List

- `firmware/data-src/dashboard.html` (MODIFIED) — Added Firmware card HTML between Logos and System, System card id="system-card", Download Settings button, Cancel button in ota-file-info
- `firmware/data-src/dashboard.js` (MODIFIED) — Added loadFirmwareStatus(), OTA upload with XHR progress, client-side validation, reboot countdown/polling, rollback banner, settings export click handler; synthesis fixes: polling done-guard, zero-byte check, dragleave relatedTarget, resetOtaUploadState color/text reset, cancel button handler
- `firmware/data-src/style.css` (MODIFIED) — Added ~35 lines OTA component styles (.ota-upload-zone, .ota-progress, .banner-warning, .btn-secondary, etc.); synthesis fixes: dismiss-btn :focus-visible, .btn-ota-cancel override
- `firmware/adapters/WebPortal.cpp` (MODIFIED) — Added POST /api/ota/ack-rollback, GET /api/settings/export endpoints; added rollback_acknowledged to /api/status; clear ota_rb_ack on successful OTA completion; added #include <Preferences.h>; synthesis fix: NVS write error handling in ack-rollback handler
- `firmware/data/dashboard.html.gz` (REGENERATED) — 3,313 bytes
- `firmware/data/dashboard.js.gz` (REGENERATED) — 22,206 bytes (updated, Pass 3)
- `firmware/data/style.css.gz` (REGENERATED) — 4,722 bytes (updated, Pass 3)

## Senior Developer Review (AI)

### Review: 2026-04-12 (Pass 1)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** A=7.1 (REJECT) / B=2.6 (APPROVED) / C=13.1 (MAJOR REWORK) → weighted: ~7.6 → **Changes Requested**
- **Issues Found:** 7 verified (3 critical, 2 high, 2 medium/low)
- **Issues Fixed:** 7 (all verified issues addressed in this synthesis pass)
- **Action Items Created:** 0

#### Review Follow-ups (AI) — Pass 1
_All verified issues were fixed in this synthesis pass. No open action items remain._

### Review: 2026-04-12 (Pass 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** A=6.9 (MAJOR REWORK) / B=7.3 (REJECT) / C=7.9 (MAJOR REWORK) → weighted: ~7.4 → **Changes Requested**
- **Issues Found:** 4 verified (1 high, 1 medium, 2 low); 14 dismissed as false positives
- **Issues Fixed:** 4 (all verified issues addressed in this synthesis pass)
- **Action Items Created:** 0

#### Review Follow-ups (AI) — Pass 2
_All verified issues were fixed in this synthesis pass. No open action items remain._

### Review: 2026-04-12 (Pass 3)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** A=5.3 (MAJOR REWORK) / B=3.5 (MAJOR REWORK) / C=0.0 (APPROVED) → weighted: ~3.0 → **Changes Requested**
- **Issues Found:** 6 verified (2 high, 4 medium/low); 10 dismissed as false positives
- **Issues Fixed:** 6 (all verified issues addressed in this synthesis pass)
- **Action Items Created:** 0

#### Review Follow-ups (AI) — Pass 3
_All verified issues were fixed in this synthesis pass. No open action items remain._


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
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <vector>
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"
#include "core/LogoManager.h"
#include "utils/Log.h"

// Defined in main.cpp — provides thread-safe flight stats for the health page
extern FlightStatsSnapshot getFlightStatsSnapshot();

#include "core/LayoutEngine.h"
#include "core/ModeOrchestrator.h"

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

    // GET /api/display/modes (Story dl-1.5) — mode list with orchestrator state
    _server->on("/api/display/modes", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetDisplayModes(request);
    });

    // POST /api/display/mode (Story dl-1.5) — manual mode switch
    _server->on("/api/display/mode", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (total == 0 || data == nullptr) {
                _sendJsonError(request, 400, "Empty request body", "EMPTY_PAYLOAD");
                return;
            }
            if (index + len == total) {
                // Parse the mode_id from request body (use `len` not `total`: data points to the
                // current chunk only; passing `total` would read past the buffer on multi-chunk bodies)
                JsonDocument reqDoc;
                DeserializationError err = deserializeJson(reqDoc, data, len);
                if (err) {
                    _sendJsonError(request, 400, "Invalid JSON", "INVALID_JSON");
                    return;
                }
                const char* modeId = reqDoc["mode_id"] | (const char*)nullptr;
                if (!modeId) {
                    _sendJsonError(request, 400, "Missing mode_id field", "MISSING_FIELD");
                    return;
                }

                // Resolve mode name from ID (simple lookup)
                const char* modeName = nullptr;
                if (strcmp(modeId, "classic_card") == 0) modeName = "Classic Card";
                else if (strcmp(modeId, "live_flight") == 0) modeName = "Live Flight Card";
                else if (strcmp(modeId, "clock") == 0) modeName = "Clock";
                else {
                    _sendJsonError(request, 400, "Unknown mode_id", "UNKNOWN_MODE");
                    return;
                }

                // Switch via orchestrator (Rule 24: always go through orchestrator)
                ModeOrchestrator::onManualSwitch(modeId, modeName);

                JsonDocument doc;
                JsonObject root = doc.to<JsonObject>();
                root["ok"] = true;
                JsonObject respData = root["data"].to<JsonObject>();
                respData["switching_to"] = modeId;
                respData["orchestrator_state"] = ModeOrchestrator::getStateString();
                respData["state_reason"] = ModeOrchestrator::getStateReason();
                String output;
                serializeJson(doc, output);
                request->send(200, "application/json", output);
            }
        }
    );

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

                // Reset rollback acknowledgment so a future rollback shows the banner again (Story fn-1.6)
                Preferences otaPrefs;
                if (otaPrefs.begin("flightwall", false)) {
                    otaPrefs.putUChar("ota_rb_ack", 0);
                    otaPrefs.end();
                } else {
                    LOG_E("OTA", "Failed to open NVS to clear rollback ack — banner may stay dismissed after rollback");
                }

                LOG_I("OTA", "Update finalized successfully");
            }
        }
    );

    // POST /api/ota/ack-rollback — dismiss rollback banner (Story fn-1.6)
    _server->on("/api/ota/ack-rollback", HTTP_POST, [](AsyncWebServerRequest* request) {
        Preferences prefs;
        if (!prefs.begin("flightwall", false)) {
            request->send(500, "application/json", "{\"ok\":false,\"error\":\"NVS access failed\",\"code\":\"NVS_ERROR\"}");
            return;
        }
        size_t written = prefs.putUChar("ota_rb_ack", 1);
        prefs.end();
        if (written == 0) {
            request->send(500, "application/json", "{\"ok\":false,\"error\":\"Failed to save acknowledgment\",\"code\":\"NVS_WRITE_ERROR\"}");
            return;
        }

        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // GET /api/settings/export — download config as JSON file (Story fn-1.6)
    _server->on("/api/settings/export", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["flightwall_settings_version"] = 1;

        // Timestamp — use NTP time if available, else uptime
        time_t now;
        time(&now);
        if (now > 1000000000) {  // NTP synced (past year 2001)
            char buf[32];
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
            root["exported_at"] = String(buf);
        } else {
            root["exported_at"] = String("uptime_ms_") + String(millis());
        }

        ConfigManager::dumpSettingsJson(root);

        String output;
        serializeJson(doc, output);

        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", output);
        response->addHeader("Content-Disposition", "attachment; filename=flightwall-settings.json");
        request->send(response);
    });

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

    // Rollback acknowledgment state (Story fn-1.6) — NVS-backed, not via ConfigManager
    Preferences prefs;
    prefs.begin("flightwall", true);  // read-only
    data["rollback_acknowledged"] = prefs.getUChar("ota_rb_ack", 0) == 1;
    prefs.end();

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

void WebPortal::_handleGetDisplayModes(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // Mode list (static for now; will grow with ModeRegistry in future stories)
    JsonArray modes = data["modes"].to<JsonArray>();

    const char* activeId = ModeOrchestrator::getActiveModeId();

    // Classic Card mode
    JsonObject mClassic = modes.add<JsonObject>();
    mClassic["id"] = "classic_card";
    mClassic["name"] = "Classic Card";
    mClassic["active"] = (strcmp(activeId, "classic_card") == 0);

    // Live Flight Card mode
    JsonObject mLive = modes.add<JsonObject>();
    mLive["id"] = "live_flight";
    mLive["name"] = "Live Flight Card";
    mLive["active"] = (strcmp(activeId, "live_flight") == 0);

    // Clock mode
    JsonObject mClock = modes.add<JsonObject>();
    mClock["id"] = "clock";
    mClock["name"] = "Clock";
    mClock["active"] = (strcmp(activeId, "clock") == 0);

    data["active"] = activeId;
    data["switch_state"] = "idle";
    data["orchestrator_state"] = ModeOrchestrator::getStateString();
    data["state_reason"] = ModeOrchestrator::getStateReason();

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
<file id="a8a8403c" path="firmware/data-src/dashboard.html" label="HTML TEMPLATE"><![CDATA[

<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>FlightWall</title>
<link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="dashboard">
  <header class="dash-header">
    <h1>FlightWall</h1>
    <p class="dash-ip" id="device-ip"></p>
    <nav><a href="/health.html">System Health</a></nav>
  </header>

  <main class="dash-cards">
    <!-- Display Settings Card -->
    <section class="card">
      <h2>Display</h2>

      <label for="brightness">Brightness</label>
      <div class="range-row">
        <input type="range" id="brightness" min="0" max="255" value="128">
        <span class="range-val" id="brightness-val">128</span>
      </div>

      <label>Text Color</label>
      <div class="rgb-row">
        <div class="rgb-field">
          <input type="number" id="color-r" min="0" max="255" value="255">
          <span class="rgb-label">R</span>
        </div>
        <div class="rgb-field">
          <input type="number" id="color-g" min="0" max="255" value="255">
          <span class="rgb-label">G</span>
        </div>
        <div class="rgb-field">
          <input type="number" id="color-b" min="0" max="255" value="255">
          <span class="rgb-label">B</span>
        </div>
      </div>
      <div class="color-preview" id="color-preview"></div>
    </section>

    <!-- Mode Picker Card (Story dl-1.5) -->
    <section class="card" id="mode-picker-card">
      <h2>Display Mode</h2>
      <div class="mode-status-line" id="modeStatusLine">
        Active: <span class="mode-status-name" id="modeStatusName">&mdash;</span>
        (<span class="mode-status-reason" id="modeStatusReason">loading</span>)
      </div>
      <div class="mode-cards-list" id="modeCardsList"></div>
    </section>

    <!-- Timing Card -->
    <section class="card">
      <h2>Timing</h2>

      <label for="fetch-interval">Check for flights every: <span id="fetch-label">10 min</span></label>
      <div class="range-row">
        <input type="range" id="fetch-interval" min="30" max="3600" step="30" value="600">
      </div>
      <p class="estimate-line" id="fetch-estimate"></p>
      <p class="estimate-note">OpenSky poll estimate; AeroAPI/CDN calls not included.</p>

      <label for="display-cycle">Cycle flights every: <span id="cycle-label">3 s</span></label>
      <div class="range-row">
        <input type="range" id="display-cycle" min="1" max="120" step="1" value="3">
      </div>
    </section>

    <!-- Network & API Card -->
    <section class="card">
      <h2>Network &amp; API</h2>
      <div class="form-fields">
        <label for="d-wifi-ssid">WiFi Network</label>
        <div class="scan-row-inline" id="d-scan-area" style="display:none">
          <button type="button" class="link-btn" id="d-btn-scan">Scan for networks</button>
          <div id="d-scan-results" class="scan-results" style="display:none"></div>
        </div>
        <input type="text" id="d-wifi-ssid" autocomplete="off" autocapitalize="off" spellcheck="false">

        <label for="d-wifi-pass">WiFi Password</label>
        <input type="password" id="d-wifi-pass" autocomplete="off" autocapitalize="off" spellcheck="false">

        <label for="d-os-client-id">OpenSky Client ID</label>
        <input type="text" id="d-os-client-id" autocomplete="off" autocapitalize="off" spellcheck="false">

        <label for="d-os-client-sec">OpenSky Client Secret</label>
        <input type="password" id="d-os-client-sec" autocomplete="off" autocapitalize="off" spellcheck="false">

        <label for="d-aeroapi-key">AeroAPI Key</label>
        <input type="password" id="d-aeroapi-key" autocomplete="off" autocapitalize="off" spellcheck="false">
      </div>
    </section>

    <!-- Hardware Card -->
    <section class="card">
      <h2>Hardware</h2>
      <div class="form-fields">
        <label for="d-tiles-x">Tiles X</label>
        <input type="number" id="d-tiles-x" inputmode="numeric" min="1" step="1" autocomplete="off">

        <label for="d-tiles-y">Tiles Y</label>
        <input type="number" id="d-tiles-y" inputmode="numeric" min="1" step="1" autocomplete="off">

        <label for="d-tile-pixels">Pixels per tile</label>
        <input type="number" id="d-tile-pixels" inputmode="numeric" min="1" step="1" autocomplete="off">

        <label for="d-display-pin">Display data pin (GPIO)</label>
        <input type="number" id="d-display-pin" inputmode="numeric" min="0" step="1" autocomplete="off">

        <label for="d-origin-corner">Origin corner</label>
        <input type="number" id="d-origin-corner" inputmode="numeric" min="0" step="1" autocomplete="off">

        <label for="d-scan-dir">Scan direction</label>
        <input type="number" id="d-scan-dir" inputmode="numeric" min="0" step="1" autocomplete="off">

        <label for="d-zigzag">Zigzag</label>
        <input type="number" id="d-zigzag" inputmode="numeric" min="0" step="1" autocomplete="off">
      </div>
      <p class="resolution-text" id="d-resolution-text"></p>

      <label for="d-zone-layout">Layout Style</label>
      <select id="d-zone-layout">
        <option value="0">Classic</option>
        <option value="1">Full-width bottom</option>
      </select>

      <div class="preview-container" id="preview-container">
        <canvas id="layout-preview"></canvas>
        <div class="preview-legend" id="preview-legend">
          <span class="legend-chip" style="background:#58a6ff"></span> Logo
          <span class="legend-chip" style="background:#3fb950"></span> Flight
          <span class="legend-chip" style="background:#d29922"></span> Telemetry
        </div>
        <h3 class="wiring-label">Panel Wiring Path</h3>
        <canvas id="wiring-preview"></canvas>
        <div class="preview-legend" id="wiring-legend">
          <span class="legend-chip" style="background:#3fb950"></span> Data in
          <span class="legend-chip" style="background:#58a6ff"></span> Cable path
        </div>
        <p class="helper-copy">Predictive preview — actual LED wall updates after Apply.</p>
      </div>

    </section>

    <!-- Calibration Card -->
    <section class="card" id="calibration-card">
      <h2 class="calibration-toggle">Calibration</h2>
      <div class="calibration-body" id="calibration-body" style="display:none">
        <p class="helper-copy" style="margin-top:0;margin-bottom:12px">Adjust wiring flags and see the test pattern update in real-time on the LED wall and canvas preview.</p>

        <label>Test Pattern</label>
        <div class="cal-pattern-toggle" id="cal-pattern-toggle">
          <button type="button" class="cal-pattern-btn active" data-pattern="0">Scan Order</button>
          <button type="button" class="cal-pattern-btn" data-pattern="1">Panel Position</button>
        </div>

        <div class="form-fields">
          <label for="cal-origin-corner">Origin Corner</label>
          <select id="cal-origin-corner" class="cal-select">
            <option value="0">0 — Top-Left</option>
            <option value="1">1 — Top-Right</option>
            <option value="2">2 — Bottom-Left</option>
            <option value="3">3 — Bottom-Right</option>
          </select>

          <label for="cal-scan-dir">Scan Direction</label>
          <select id="cal-scan-dir" class="cal-select">
            <option value="0">0 — Rows (Horizontal)</option>
            <option value="1">1 — Columns (Vertical)</option>
          </select>

          <label for="cal-zigzag">Zigzag</label>
          <select id="cal-zigzag" class="cal-select">
            <option value="0">0 — Progressive (same direction)</option>
            <option value="1">1 — Zigzag (alternating)</option>
          </select>
        </div>

        <div class="cal-preview-container" id="cal-preview-container">
          <canvas id="cal-preview-canvas"></canvas>
          <div class="preview-legend">
            <span class="legend-chip" style="background:#f85149"></span> Pixel 0 (start)
            <span class="legend-chip" style="background:#58a6ff"></span> Pixel N (end)
          </div>
        </div>

        <p class="helper-copy">Form &rarr; Canvas preview (instant) &rarr; LED wall (50-200ms)</p>
      </div>
    </section>

    <!-- Location Card -->
    <section class="card" id="location-card">
      <h2 class="location-toggle">Location</h2>
      <div class="location-body" id="location-body" style="display:none">
        <div id="location-map-wrap">
          <div id="location-map"></div>
          <p class="map-loading" id="map-loading">Loading map...</p>
        </div>
        <div class="form-fields" id="location-fallback">
          <label for="d-center-lat">Latitude</label>
          <input type="number" id="d-center-lat" step="any" autocomplete="off">
          <label for="d-center-lon">Longitude</label>
          <input type="number" id="d-center-lon" step="any" autocomplete="off">
          <label for="d-radius-km">Radius (km)</label>
          <input type="number" id="d-radius-km" step="any" min="0.1" autocomplete="off">
        </div>
        <p class="helper-copy" id="location-helper">Drag the center marker or radius handle to update your capture area.</p>
      </div>
    </section>

    <!-- Logos Card -->
    <section class="card" id="logos-card">
      <h2>Logos</h2>
      <div class="logo-upload-zone" id="logo-upload-zone">
        <p class="upload-prompt">Drag &amp; drop <code>.bin</code> logo files here or</p>
        <label class="upload-label" for="logo-file-input">Choose files</label>
        <input type="file" id="logo-file-input" accept=".bin" multiple hidden>
        <p class="upload-hint">32×32 RGB565, 2048 bytes each</p>
      </div>
      <div class="logo-file-list" id="logo-file-list"></div>
      <button type="button" class="apply-btn" id="btn-upload-logos" style="display:none">Upload</button>
      <p class="logo-storage-summary" id="logo-storage-summary"></p>
      <div class="logo-list-container" id="logo-list-container">
        <p class="logo-empty-state" id="logo-empty-state" style="display:none">No logos uploaded yet. Drag .bin files here to add airline logos.</p>
        <div class="logo-list" id="logo-list"></div>
      </div>
    </section>

    <!-- Firmware Card (Story fn-1.6) -->
    <section class="card" id="firmware-card">
      <h2>Firmware</h2>
      <p id="fw-version" class="fw-version-text"></p>
      <div id="rollback-banner" class="banner-warning" role="alert" aria-live="polite" style="display:none">
        <span>Firmware rolled back to previous version</span>
        <button type="button" class="dismiss-btn" id="btn-dismiss-rollback" aria-label="Dismiss rollback warning">&times;</button>
      </div>
      <div class="ota-upload-zone" id="ota-upload-zone" role="button" tabindex="0" aria-label="Select firmware file for upload">
        <p class="upload-prompt">Drag .bin file here or tap to select</p>
        <input type="file" id="ota-file-input" accept=".bin" hidden>
      </div>
      <div class="ota-file-info" id="ota-file-info" style="display:none">
        <span id="ota-file-name"></span>
        <button type="button" class="apply-btn" id="btn-upload-firmware">Upload Firmware</button>
        <button type="button" class="btn-secondary btn-ota-cancel" id="btn-cancel-ota">Cancel</button>
      </div>
      <div class="ota-progress" id="ota-progress" style="display:none" role="progressbar" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100">
        <div class="ota-progress-bar" id="ota-progress-bar"></div>
        <span class="ota-progress-text" id="ota-progress-text">0%</span>
      </div>
      <div class="ota-reboot" id="ota-reboot" style="display:none">
        <p id="ota-reboot-text"></p>
      </div>
      <p class="helper-copy">Before migrating, download your settings backup from <a href="#system-card">System</a> below.</p>
    </section>

    <!-- System / Danger Zone Card -->
    <section class="card card-danger" id="system-card">
      <h2>System</h2>
      <button type="button" class="btn-secondary" id="btn-export-settings">Download Settings</button>
      <p class="helper-copy" style="margin-top:4px">Download your configuration as a JSON file before partition migration or reflash.</p>
      <div id="reset-default" class="reset-row">
        <button type="button" class="btn-danger" id="btn-reset">Reset to Defaults</button>
      </div>
      <div id="reset-confirm" class="reset-row" style="display:none">
        <p class="reset-warning">All settings will be erased and the device will restart in setup mode.</p>
        <div class="reset-actions">
          <button type="button" class="btn-danger" id="btn-reset-confirm">Confirm</button>
          <button type="button" class="btn-cancel" id="btn-reset-cancel">Cancel</button>
        </div>
      </div>
    </section>
  </main>

  <div class="apply-bar" id="apply-bar" style="display:none">
    <span class="apply-bar-text">Unsaved changes</span>
    <button type="button" class="apply-bar-btn" id="btn-apply-all">Apply Changes</button>
  </div>
</div>

<script src="/common.js"></script>
<script src="/dashboard.js"></script>
</body>
</html>


]]></file>
<file id="6ca8ed8c" path="firmware/data-src/dashboard.js" label="SOURCE CODE"><![CDATA[

/* FlightWall Dashboard — Display, Timing, Network/API, and Hardware settings */

/**
 * Shared zone calculation algorithm (must match C++ LayoutEngine::compute exactly).
 * @param {number} tilesX - Number of horizontal tiles
 * @param {number} tilesY - Number of vertical tiles
 * @param {number} tilePixels - Pixels per tile edge
 * @returns {object} { matrixWidth, matrixHeight, mode, logoZone, flightZone, telemetryZone, valid }
 */
function computeLayout(tilesX, tilesY, tilePixels, logoWidthPct, flightHeightPct, layoutMode) {
  if (!tilesX || !tilesY || !tilePixels) {
    return { matrixWidth: 0, matrixHeight: 0, mode: 'compact',
      logoZone: {x:0,y:0,w:0,h:0}, flightZone: {x:0,y:0,w:0,h:0},
      telemetryZone: {x:0,y:0,w:0,h:0}, valid: false };
  }

  var mw = tilesX * tilePixels;
  var mh = tilesY * tilePixels;

  if (mw < mh) {
    return { matrixWidth: mw, matrixHeight: mh, mode: 'compact',
      logoZone: {x:0,y:0,w:0,h:0}, flightZone: {x:0,y:0,w:0,h:0},
      telemetryZone: {x:0,y:0,w:0,h:0}, valid: false };
  }

  var mode;
  if (mh < 32) { mode = 'compact'; }
  else if (mh < 48) { mode = 'full'; }
  else { mode = 'expanded'; }

  var logoW = mh; // default: square logo
  if (logoWidthPct > 0 && logoWidthPct <= 99) {
    logoW = Math.round(mw * logoWidthPct / 100);
    logoW = Math.max(1, Math.min(mw - 1, logoW));
  }

  var splitY = Math.floor(mh / 2); // default: 50/50
  if (flightHeightPct > 0 && flightHeightPct <= 99) {
    splitY = Math.round(mh * flightHeightPct / 100);
    splitY = Math.max(1, Math.min(mh - 1, splitY));
  }

  var logoZone, flightZone, telemetryZone;
  if (layoutMode === 1) {
    // Full-width bottom: logo top-left, flight top-right, telemetry spans full width
    logoZone      = { x: 0,     y: 0,      w: logoW,      h: splitY };
    flightZone    = { x: logoW, y: 0,      w: mw - logoW, h: splitY };
    telemetryZone = { x: 0,     y: splitY, w: mw,         h: mh - splitY };
  } else {
    // Classic: logo full-height left, flight/telemetry stacked right
    logoZone      = { x: 0,     y: 0,      w: logoW,      h: mh };
    flightZone    = { x: logoW, y: 0,      w: mw - logoW, h: splitY };
    telemetryZone = { x: logoW, y: splitY, w: mw - logoW, h: mh - splitY };
  }

  return {
    matrixWidth: mw,
    matrixHeight: mh,
    mode: mode,
    logoZone: logoZone,
    flightZone: flightZone,
    telemetryZone: telemetryZone,
    valid: true
  };
}

(function() {
  'use strict';

  // --- Display card DOM ---
  var brightness = document.getElementById('brightness');
  var brightnessVal = document.getElementById('brightness-val');
  var colorR = document.getElementById('color-r');
  var colorG = document.getElementById('color-g');
  var colorB = document.getElementById('color-b');
  var colorPreview = document.getElementById('color-preview');
  var deviceIp = document.getElementById('device-ip');

  // --- Timing card DOM ---
  var fetchInterval = document.getElementById('fetch-interval');
  var fetchLabel = document.getElementById('fetch-label');
  var fetchEstimate = document.getElementById('fetch-estimate');
  var displayCycle = document.getElementById('display-cycle');
  var cycleLabel = document.getElementById('cycle-label');

  // --- Network & API card DOM ---
  var dWifiSsid = document.getElementById('d-wifi-ssid');
  var dWifiPass = document.getElementById('d-wifi-pass');
  var dOsClientId = document.getElementById('d-os-client-id');
  var dOsClientSec = document.getElementById('d-os-client-sec');
  var dAeroKey = document.getElementById('d-aeroapi-key');
  var dBtnScan = document.getElementById('d-btn-scan');
  var dScanArea = document.getElementById('d-scan-area');
  var dScanResults = document.getElementById('d-scan-results');

  // --- Hardware card DOM ---
  var dTilesX = document.getElementById('d-tiles-x');
  var dTilesY = document.getElementById('d-tiles-y');
  var dTilePixels = document.getElementById('d-tile-pixels');
  var dDisplayPin = document.getElementById('d-display-pin');
  var dOriginCorner = document.getElementById('d-origin-corner');
  var dScanDir = document.getElementById('d-scan-dir');
  var dZigzag = document.getElementById('d-zigzag');
  var dResText = document.getElementById('d-resolution-text');

  // --- Unified apply bar ---
  var applyBar = document.getElementById('apply-bar');
  var btnApplyAll = document.getElementById('btn-apply-all');
  var dirtySections = { display: false, timing: false, network: false, hardware: false };

  function markSectionDirty(section) {
    dirtySections[section] = true;
    applyBar.style.display = '';
  }

  function clearDirtyState() {
    dirtySections.display = false;
    dirtySections.timing = false;
    dirtySections.network = false;
    dirtySections.hardware = false;
    applyBar.style.display = 'none';
  }

  function collectPayload() {
    var payload = {};
    if (dirtySections.display) {
      payload.brightness = parseInt(brightness.value, 10);
      payload.text_color_r = clamp(parseInt(colorR.value, 10) || 0);
      payload.text_color_g = clamp(parseInt(colorG.value, 10) || 0);
      payload.text_color_b = clamp(parseInt(colorB.value, 10) || 0);
    }
    if (dirtySections.timing) {
      payload.fetch_interval = parseInt(fetchInterval.value, 10);
      payload.display_cycle = parseInt(displayCycle.value, 10);
    }
    if (dirtySections.network) {
      payload.wifi_ssid = dWifiSsid.value;
      payload.wifi_password = dWifiPass.value;
      payload.os_client_id = dOsClientId.value.trim();
      payload.os_client_sec = dOsClientSec.value.trim();
      payload.aeroapi_key = dAeroKey.value.trim();
    }
    if (dirtySections.hardware) {
      var tilesX = parseUint8Field(dTilesX, 'Tiles X', false);
      if (tilesX === null) return null;
      var tilesY = parseUint8Field(dTilesY, 'Tiles Y', false);
      if (tilesY === null) return null;
      var tilePixels = parseUint8Field(dTilePixels, 'Pixels per tile', false);
      if (tilePixels === null) return null;
      var dp = parseUint8Field(dDisplayPin, 'Display data pin', true);
      if (dp === null || VALID_PINS.indexOf(dp) === -1) {
        FW.showToast('Invalid GPIO pin. Supported: ' + VALID_PINS.join(', '), 'error');
        return null;
      }
      var originCorner = parseUint8Field(dOriginCorner, 'Origin corner', true);
      if (originCorner === null) return null;
      var scanDir = parseUint8Field(dScanDir, 'Scan direction', true);
      if (scanDir === null) return null;
      var zigzag = parseUint8Field(dZigzag, 'Zigzag', true);
      if (zigzag === null) return null;
      payload.tiles_x = tilesX;
      payload.tiles_y = tilesY;
      payload.tile_pixels = tilePixels;
      payload.display_pin = dp;
      payload.origin_corner = originCorner;
      payload.scan_dir = scanDir;
      payload.zigzag = zigzag;
      payload.zone_logo_pct = customLogoPct;
      payload.zone_split_pct = customSplitPct;
      payload.zone_layout = zoneLayout;
    }
    return payload;
  }

  var VALID_PINS = [0,2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33];

  var debounceTimer = null;
  var DEBOUNCE_MS = 400;
  var SECONDS_PER_MONTH = 2592000;
  var OPENSKY_WARN_THRESHOLD = 4000;
  var previewLastLayout = null;
  var hardwareInputDirty = false;
  var suppressHardwareInputHandler = false;
  var customLogoPct = 0;   // 0 = auto
  var customSplitPct = 0;  // 0 = auto
  var zoneLayout = 0;      // 0 = classic, 1 = full-width bottom
  var dZoneLayout = document.getElementById('d-zone-layout');

  deviceIp.textContent = window.location.hostname || '';

  // --- Load current settings ---
  function loadSettings() {
    return FW.get('/api/settings').then(function(res) {
      if (!res.body.ok || !res.body.data) {
        FW.showToast('Failed to load settings', 'error');
        return;
      }
      var d = res.body.data;

      // Display
      if (d.brightness !== undefined) {
        brightness.value = d.brightness;
        brightnessVal.textContent = d.brightness;
      }
      if (d.text_color_r !== undefined) colorR.value = d.text_color_r;
      if (d.text_color_g !== undefined) colorG.value = d.text_color_g;
      if (d.text_color_b !== undefined) colorB.value = d.text_color_b;
      updatePreview();

      // Timing
      if (d.fetch_interval !== undefined) {
        fetchInterval.value = d.fetch_interval;
        fetchLabel.textContent = formatInterval(d.fetch_interval);
        updateFetchEstimate(d.fetch_interval);
      }
      if (d.display_cycle !== undefined) {
        displayCycle.value = d.display_cycle;
        updateCycleLabel(d.display_cycle);
      }

      // Network & API
      if (d.wifi_ssid !== undefined) dWifiSsid.value = d.wifi_ssid;
      if (d.wifi_password !== undefined) dWifiPass.value = d.wifi_password;
      if (d.os_client_id !== undefined) dOsClientId.value = d.os_client_id;
      if (d.os_client_sec !== undefined) dOsClientSec.value = d.os_client_sec;
      if (d.aeroapi_key !== undefined) dAeroKey.value = d.aeroapi_key;

      // Location
      var loadedLocation = normalizeLocationValues({
        center_lat: d.center_lat,
        center_lon: d.center_lon,
        radius_km: d.radius_km
      });
      if (loadedLocation) {
        writeLocationFields(loadedLocation);
        rememberValidLocation(loadedLocation);
        if (mapInstance) {
          updateMapFromValues(loadedLocation, { fit: false, pan: false });
        } else if (leafletLoaded && isLocationCardOpen() && !mapFailureHandled) {
          maybeInitLocationMap();
        }
      }

      // Hardware
      if (d.tiles_x !== undefined) dTilesX.value = d.tiles_x;
      if (d.tiles_y !== undefined) dTilesY.value = d.tiles_y;
      if (d.tile_pixels !== undefined) dTilePixels.value = d.tile_pixels;
      if (d.display_pin !== undefined) dDisplayPin.value = d.display_pin;
      if (d.origin_corner !== undefined) dOriginCorner.value = d.origin_corner;
      if (d.scan_dir !== undefined) dScanDir.value = d.scan_dir;
      if (d.zigzag !== undefined) dZigzag.value = d.zigzag;
      if (d.zone_logo_pct !== undefined) customLogoPct = d.zone_logo_pct;
      if (d.zone_split_pct !== undefined) customSplitPct = d.zone_split_pct;
      if (d.zone_layout !== undefined) {
        zoneLayout = d.zone_layout;
        if (dZoneLayout) dZoneLayout.value = zoneLayout;
      }
      updateHwResolution();

      // Sync calibration selectors from loaded hardware values
      syncCalibrationFromSettings();

      // Show scan button now that we're in STA mode
      dScanArea.style.display = '';
    }).catch(function() {
      FW.showToast('Cannot reach device', 'error');
    });
  }

  // --- Apply settings (hot-reload, no reboot) ---
  function applySettings(payload) {
    FW.post('/api/settings', payload).then(function(res) {
      if (res.body.ok) {
        FW.showToast('Applied', 'success');
      } else {
        FW.showToast(res.body.error || 'Save failed', 'error');
      }
    }).catch(function() {
      FW.showToast('Network error', 'error');
    });
  }

  // --- Apply settings with reboot awareness ---
  function applyWithReboot(payload, btn, originalText) {
    btn.disabled = true;
    btn.textContent = 'Applying...';
    var rebootRequested = false;

    FW.post('/api/settings', payload).then(function(res) {
      if (!res.body.ok) {
        throw new Error(res.body.error || 'Save failed');
      }
      if (res.body.reboot_required) {
        FW.showToast('Rebooting to apply changes...', 'warning');
        rebootRequested = true;
        return FW.post('/api/reboot', {});
      }
      FW.showToast('Applied', 'success');
      btn.disabled = false;
      btn.textContent = originalText;
      return null;
    }).then(function(res) {
      if (!rebootRequested) return;
      if (!res || !res.body || !res.body.ok) {
        throw new Error((res && res.body && res.body.error) || 'Reboot failed');
      }
      btn.textContent = 'Rebooting...';
    }).catch(function(err) {
      if (rebootRequested && err && err.name === 'TypeError') {
        // Connection loss after reboot request is expected
        btn.textContent = 'Rebooting...';
        return;
      }
      FW.showToast(err && err.message ? err.message : 'Network error', 'error');
      btn.disabled = false;
      btn.textContent = originalText;
    });
  }

  function debouncedApply(payload) {
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(function() {
      applySettings(payload);
    }, DEBOUNCE_MS);
  }

  // --- Color preview ---
  function updatePreview() {
    var r = clamp(parseInt(colorR.value, 10) || 0);
    var g = clamp(parseInt(colorG.value, 10) || 0);
    var b = clamp(parseInt(colorB.value, 10) || 0);
    colorPreview.style.background = 'rgb(' + r + ',' + g + ',' + b + ')';
  }

  function clamp(v) {
    return Math.max(0, Math.min(255, v));
  }

  function parseStrictInteger(raw) {
    var text = String(raw === undefined || raw === null ? '' : raw).trim();
    if (!/^\d+$/.test(text)) return null;
    return parseInt(text, 10);
  }

  function parseUint8Field(el, label, allowZero) {
    var value = parseStrictInteger(el.value);
    var min = allowZero ? 0 : 1;
    if (value === null || value < min || value > 255) {
      FW.showToast(label + ' must be a whole number from ' + min + ' to 255', 'error');
      return null;
    }
    return value;
  }

  // --- Brightness slider ---
  brightness.addEventListener('input', function() {
    brightnessVal.textContent = brightness.value;
  });
  brightness.addEventListener('change', function() {
    markSectionDirty('display');
  });

  // --- RGB inputs ---
  function onColorChange() {
    var r = clamp(parseInt(colorR.value, 10) || 0);
    var g = clamp(parseInt(colorG.value, 10) || 0);
    var b = clamp(parseInt(colorB.value, 10) || 0);
    colorR.value = r;
    colorG.value = g;
    colorB.value = b;
    updatePreview();
    markSectionDirty('display');
  }

  colorR.addEventListener('change', onColorChange);
  colorG.addEventListener('change', onColorChange);
  colorB.addEventListener('change', onColorChange);
  colorR.addEventListener('input', updatePreview);
  colorG.addEventListener('input', updatePreview);
  colorB.addEventListener('input', updatePreview);

  // --- Timing: fetch interval ---
  function formatInterval(seconds) {
    var s = parseInt(seconds, 10);
    var min = Math.floor(s / 60);
    var rem = s % 60;
    if (min === 0) return s + ' s';
    if (rem === 0) return min + ' min';
    return min + ' min ' + rem + ' s';
  }

  function updateFetchEstimate(seconds) {
    var s = parseInt(seconds, 10);
    if (s <= 0) s = 1;
    var n = Math.round(SECONDS_PER_MONTH / s);
    fetchEstimate.textContent = '~' + n.toLocaleString() + ' calls/month';
    if (n > OPENSKY_WARN_THRESHOLD) {
      fetchEstimate.classList.add('estimate-warning');
    } else {
      fetchEstimate.classList.remove('estimate-warning');
    }
  }

  fetchInterval.addEventListener('input', function() {
    fetchLabel.textContent = formatInterval(fetchInterval.value);
    updateFetchEstimate(fetchInterval.value);
  });
  fetchInterval.addEventListener('change', function() {
    markSectionDirty('timing');
  });

  // --- Timing: display cycle ---
  function updateCycleLabel(seconds) {
    cycleLabel.textContent = parseInt(seconds, 10) + ' s';
  }

  displayCycle.addEventListener('input', function() {
    updateCycleLabel(displayCycle.value);
  });
  displayCycle.addEventListener('change', function() {
    markSectionDirty('timing');
  });

  // --- Network & API: mark dirty on change ---
  function onNetworkInput() { markSectionDirty('network'); }
  dWifiSsid.addEventListener('input', onNetworkInput);
  dWifiPass.addEventListener('input', onNetworkInput);
  dOsClientId.addEventListener('input', onNetworkInput);
  dOsClientSec.addEventListener('input', onNetworkInput);
  dAeroKey.addEventListener('input', onNetworkInput);

  // --- Hardware: Resolution text ---
  function updateHwResolution() {
    var dims = parseHardwareDimensionsFromInputs();
    setResolutionText(dims);
  }

  function setResolutionText(dims) {
    if (!dims || dims.matrixWidth <= 0 || dims.matrixHeight <= 0) {
      dResText.textContent = '';
      return;
    }
    dResText.textContent = 'Display: ' + dims.matrixWidth + ' x ' + dims.matrixHeight + ' pixels';
  }

  function parseHardwareDimensionsFromInputs() {
    var tx = parseStrictInteger(dTilesX.value);
    var ty = parseStrictInteger(dTilesY.value);
    var tp = parseStrictInteger(dTilePixels.value);
    if (tx === null || ty === null || tp === null) return null;
    if (tx < 1 || ty < 1 || tp < 1 || tx > 255 || ty > 255 || tp > 255) return null;
    return {
      tilesX: tx,
      tilesY: ty,
      tilePixels: tp,
      matrixWidth: tx * tp,
      matrixHeight: ty * tp
    };
  }

  function normalizeZone(zone) {
    if (!zone || typeof zone !== 'object') return null;
    var x = Number(zone.x);
    var y = Number(zone.y);
    var w = Number(zone.w);
    var h = Number(zone.h);
    if (!Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(w) || !Number.isFinite(h)) return null;
    return { x: x, y: y, w: w, h: h };
  }

  function normalizeLayoutFromApi(data) {
    if (!data || typeof data !== 'object' || !data.matrix || !data.hardware) return null;
    var mw = Number(data.matrix.width);
    var mh = Number(data.matrix.height);
    var tx = Number(data.hardware.tiles_x);
    var ty = Number(data.hardware.tiles_y);
    var tp = Number(data.hardware.tile_pixels);
    var logo = normalizeZone(data.logo_zone);
    var flight = normalizeZone(data.flight_zone);
    var telemetry = normalizeZone(data.telemetry_zone);
    if (!Number.isFinite(mw) || !Number.isFinite(mh)) return null;
    if (!Number.isFinite(tx) || !Number.isFinite(ty) || !Number.isFinite(tp)) return null;
    if (!logo || !flight || !telemetry) return null;
    return {
      matrixWidth: mw,
      matrixHeight: mh,
      mode: String(data.mode || 'compact'),
      logoZone: logo,
      flightZone: flight,
      telemetryZone: telemetry,
      valid: mw > 0 && mh > 0 && mw >= mh,
      hardware: { tilesX: tx, tilesY: ty, tilePixels: tp }
    };
  }

  // --- Canvas layout preview ---
  var layoutCanvas = document.getElementById('layout-preview');
  var previewContainer = document.getElementById('preview-container');

  var ZONE_COLORS = {
    logo: '#58a6ff',
    flight: '#3fb950',
    telemetry: '#d29922'
  };

  function renderLayoutCanvas(layout) {
    if (!layoutCanvas || !layoutCanvas.getContext || !previewContainer) return;
    var ctx = layoutCanvas.getContext('2d');

    if (!layout || !layout.valid) {
      layoutCanvas.width = 0;
      layoutCanvas.height = 0;
      previewContainer.style.display = 'none';
      previewLastLayout = null;
      return;
    }

    previewLastLayout = layout;
    previewContainer.style.display = '';

    // Scale canvas to fit container width while preserving aspect ratio
    var containerWidth = previewContainer.clientWidth || 300;
    var aspect = layout.matrixWidth / layout.matrixHeight;
    var drawWidth = Math.min(containerWidth, 480);
    var drawHeight = Math.round(drawWidth / aspect);

    layoutCanvas.width = drawWidth;
    layoutCanvas.height = drawHeight;

    var sx = drawWidth / layout.matrixWidth;
    var sy = drawHeight / layout.matrixHeight;

    // Background (matrix bounds)
    ctx.fillStyle = '#0d1117';
    ctx.fillRect(0, 0, drawWidth, drawHeight);

    // Draw tile grid when hardware dimensions are known.
    if (layout.hardware && layout.hardware.tilePixels > 0) {
      var tileStepX = layout.hardware.tilePixels * sx;
      var tileStepY = layout.hardware.tilePixels * sy;
      if (tileStepX >= 2 && tileStepY >= 2) {
        ctx.strokeStyle = '#21262d';
        ctx.lineWidth = 1;
        for (var gx = tileStepX; gx < drawWidth; gx += tileStepX) {
          var x = Math.round(gx) + 0.5;
          ctx.beginPath();
          ctx.moveTo(x, 0);
          ctx.lineTo(x, drawHeight);
          ctx.stroke();
        }
        for (var gy = tileStepY; gy < drawHeight; gy += tileStepY) {
          var y = Math.round(gy) + 0.5;
          ctx.beginPath();
          ctx.moveTo(0, y);
          ctx.lineTo(drawWidth, y);
          ctx.stroke();
        }
      }
    }

    // Draw zones
    var zones = [
      { zone: layout.logoZone, color: ZONE_COLORS.logo, label: 'Logo' },
      { zone: layout.flightZone, color: ZONE_COLORS.flight, label: 'Flight' },
      { zone: layout.telemetryZone, color: ZONE_COLORS.telemetry, label: 'Telemetry' }
    ];

    // Helper: truncate text to fit pixel columns (mirrors firmware truncateToColumns)
    function truncPreview(text, maxCols) {
      if (text.length <= maxCols) return text;
      if (maxCols <= 3) return text.substring(0, maxCols);
      return text.substring(0, maxCols - 3) + '...';
    }

    zones.forEach(function(z) {
      if (!z.zone || z.zone.w <= 0 || z.zone.h <= 0) return;
      var rx = Math.round(z.zone.x * sx);
      var ry = Math.round(z.zone.y * sy);
      var rw = Math.round(z.zone.w * sx);
      var rh = Math.round(z.zone.h * sy);
      if (rw <= 0 || rh <= 0) return;

      ctx.fillStyle = z.color;
      ctx.globalAlpha = 0.3;
      ctx.fillRect(rx, ry, rw, rh);
      ctx.globalAlpha = 1.0;
      ctx.strokeStyle = z.color;
      ctx.lineWidth = 2;
      if (rw > 2 && rh > 2) {
        ctx.strokeRect(rx + 1, ry + 1, rw - 2, rh - 2);
      }

      // Firmware uses 6x8px characters. Compute how many lines/cols fit.
      var charW = 6, charH = 8;
      var maxCols = Math.floor(z.zone.w / charW);
      var linesAvail = Math.floor(z.zone.h / charH);
      var fontSize = Math.max(7, Math.min(13, Math.round(charH * sy)));
      var lineH = fontSize * 1.15;

      if (maxCols <= 0 || linesAvail <= 0 || rw < 20 || rh < 10) {
        // Too small for text — just show zone color label
        if (rw > 30 && rh > 12) {
          ctx.fillStyle = z.color;
          ctx.font = '9px sans-serif';
          ctx.textAlign = 'center';
          ctx.textBaseline = 'middle';
          ctx.fillText(z.label, rx + rw / 2, ry + rh / 2);
        }
        return;
      }

      ctx.save();
      ctx.beginPath();
      ctx.rect(rx, ry, rw, rh);
      ctx.clip();
      ctx.font = fontSize + 'px monospace';
      ctx.textAlign = 'left';
      ctx.textBaseline = 'top';
      ctx.fillStyle = '#e6edf3';

      var lines = [];

      if (z.label === 'Logo') {
        // Logo zone: show icon placeholder
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillStyle = z.color;
        ctx.font = Math.min(fontSize + 2, Math.floor(rh * 0.35)) + 'px sans-serif';
        ctx.fillText('Logo', rx + rw / 2, ry + rh / 2);
        ctx.restore();
        return;
      }

      if (z.label === 'Flight') {
        // Mirror firmware renderFlightZone logic with sample data
        var airline = 'United 1234';
        var route = 'SFO>LAX';
        var aircraft = 'B737';

        if (linesAvail === 1) {
          lines.push(truncPreview(route, maxCols));
        } else if (linesAvail === 2) {
          lines.push(truncPreview(airline, maxCols));
          var detail = route + ' ' + aircraft;
          lines.push(truncPreview(detail, maxCols));
        } else {
          lines.push(truncPreview(airline, maxCols));
          lines.push(truncPreview(route, maxCols));
          lines.push(truncPreview(aircraft, maxCols));
        }
      }

      if (z.label === 'Telemetry') {
        // Mirror firmware renderTelemetryZone logic with sample data
        if (linesAvail >= 2) {
          lines.push(truncPreview('28.3kft 450mph', maxCols));
          lines.push(truncPreview('045d -12fps', maxCols));
        } else {
          lines.push(truncPreview('A28k S450 T045 V-12', maxCols));
        }
      }

      // Draw lines vertically centered in zone (mirrors firmware centering)
      var totalH = lines.length * lineH;
      var startY = ry + (rh - totalH) / 2;
      var padX = Math.round(2 * sx);
      for (var i = 0; i < lines.length; i++) {
        ctx.fillText(lines[i], rx + padX, startY + i * lineH);
      }

      ctx.restore();
    });

    // Matrix outline
    ctx.strokeStyle = '#30363d';
    ctx.lineWidth = 1;
    ctx.strokeRect(0, 0, drawWidth, drawHeight);

    // Draw zone divider drag handles
    var logoX = Math.round(layout.logoZone.w * sx);
    var splitYPos = Math.round(layout.flightZone.h * sy);

    ctx.save();
    ctx.setLineDash([4, 4]);
    ctx.strokeStyle = '#fff';
    ctx.globalAlpha = 0.4;
    ctx.lineWidth = 2;

    // Logo divider (vertical) — in mode 1, only extends to splitY
    var logoVEnd = (layout.zoneLayout === 1) ? splitYPos : drawHeight;
    ctx.beginPath();
    ctx.moveTo(logoX, 0);
    ctx.lineTo(logoX, logoVEnd);
    ctx.stroke();

    // Split divider (horizontal) — in mode 1, spans full width
    var splitXStart = (layout.zoneLayout === 1) ? 0 : logoX;
    ctx.beginPath();
    ctx.moveTo(splitXStart, splitYPos);
    ctx.lineTo(drawWidth, splitYPos);
    ctx.stroke();

    ctx.setLineDash([]);
    ctx.globalAlpha = 0.7;

    // Grab handles
    var ghw = 4, ghh = 16;
    ctx.fillStyle = '#fff';
    ctx.fillRect(logoX - ghw / 2, logoVEnd / 2 - ghh / 2, ghw, ghh);
    var infoMidX = (layout.zoneLayout === 1) ? drawWidth / 2 : logoX + (drawWidth - logoX) / 2;
    ctx.fillRect(infoMidX - ghh / 2, splitYPos - ghw / 2, ghh, ghw);

    ctx.restore();
  }

  function updatePreviewFromInputs() {
    var dims = parseHardwareDimensionsFromInputs();
    var layout;
    if (!dims) {
      layout = { valid: false };
    } else {
      layout = computeLayout(dims.tilesX, dims.tilesY, dims.tilePixels, customLogoPct, customSplitPct, zoneLayout);
      layout.zoneLayout = zoneLayout;
      layout.hardware = {
        tilesX: dims.tilesX,
        tilesY: dims.tilesY,
        tilePixels: dims.tilePixels
      };
    }
    renderLayoutCanvas(layout);
    renderWiringCanvas();
  }

  function onHardwareInput() {
    if (!suppressHardwareInputHandler) {
      hardwareInputDirty = true;
      markSectionDirty('hardware');
    }
    updateHwResolution();
    updatePreviewFromInputs();
  }

  dTilesX.addEventListener('input', onHardwareInput);
  dTilesY.addEventListener('input', onHardwareInput);
  dTilePixels.addEventListener('input', onHardwareInput);
  window.addEventListener('resize', function() {
    if (previewLastLayout) {
      renderLayoutCanvas(previewLastLayout);
    }
    renderWiringCanvas();
  });

  // --- Zone drag interaction on layout preview canvas ---
  var zoneDragTarget = null; // 'logo' or 'split'

  function canvasPos(e) {
    var rect = layoutCanvas.getBoundingClientRect();
    var cx = e.touches ? e.touches[0].clientX : e.clientX;
    var cy = e.touches ? e.touches[0].clientY : e.clientY;
    return { x: cx - rect.left, y: cy - rect.top };
  }

  function hitTestDivider(pos) {
    if (!previewLastLayout || !previewLastLayout.valid) return null;
    var layout = previewLastLayout;
    var sx = layoutCanvas.width / layout.matrixWidth;
    var sy = layoutCanvas.height / layout.matrixHeight;
    var threshold = 10;

    var logoX = layout.logoZone.w * sx;
    var logoVEnd = (layout.zoneLayout === 1) ? (layout.flightZone.h * sy) : (layoutCanvas.height);
    if (Math.abs(pos.x - logoX) < threshold && pos.y <= logoVEnd + threshold) return 'logo';

    var splitY = layout.flightZone.h * sy;
    var splitXStart = (layout.zoneLayout === 1) ? 0 : logoX;
    if (pos.x >= splitXStart - threshold && Math.abs(pos.y - splitY) < threshold) return 'split';

    return null;
  }

  function onZoneDragStart(e) {
    var pos = canvasPos(e);
    var target = hitTestDivider(pos);
    if (!target) return;
    e.preventDefault();
    zoneDragTarget = target;
  }

  function onZoneDragMove(e) {
    if (!zoneDragTarget) {
      // Update cursor on hover
      if (layoutCanvas && previewLastLayout && previewLastLayout.valid) {
        var hoverPos = e.touches ? null : canvasPos(e);
        if (hoverPos) {
          var hover = hitTestDivider(hoverPos);
          layoutCanvas.style.cursor = hover ? (hover === 'logo' ? 'col-resize' : 'row-resize') : '';
        }
      }
      return;
    }
    e.preventDefault();

    var pos = canvasPos(e);
    var layout = previewLastLayout;
    if (!layout || !layout.valid) return;
    var sx = layoutCanvas.width / layout.matrixWidth;
    var sy = layoutCanvas.height / layout.matrixHeight;

    if (zoneDragTarget === 'logo') {
      var newLogoW = Math.round(pos.x / sx);
      newLogoW = Math.max(Math.round(layout.matrixWidth * 0.05), Math.min(Math.round(layout.matrixWidth * 0.95), newLogoW));
      customLogoPct = Math.round(newLogoW / layout.matrixWidth * 100);
    } else {
      var newSplitY = Math.round(pos.y / sy);
      newSplitY = Math.max(Math.round(layout.matrixHeight * 0.1), Math.min(Math.round(layout.matrixHeight * 0.9), newSplitY));
      customSplitPct = Math.round(newSplitY / layout.matrixHeight * 100);
    }

    updatePreviewFromInputs();
  }

  function onZoneDragEnd() {
    if (zoneDragTarget) {
      zoneDragTarget = null;
      markSectionDirty('hardware');
    }
  }

  layoutCanvas.addEventListener('mousedown', onZoneDragStart);
  layoutCanvas.addEventListener('touchstart', onZoneDragStart, { passive: false });
  document.addEventListener('mousemove', onZoneDragMove);
  document.addEventListener('touchmove', onZoneDragMove, { passive: false });
  document.addEventListener('mouseup', onZoneDragEnd);
  document.addEventListener('touchend', onZoneDragEnd);

  // --- Hardware: mark dirty on change ---
  function onHardwareDirty() { markSectionDirty('hardware'); }
  dDisplayPin.addEventListener('input', onHardwareDirty);
  dOriginCorner.addEventListener('input', onHardwareDirty);
  dScanDir.addEventListener('input', onHardwareDirty);
  dZigzag.addEventListener('input', onHardwareDirty);

  // --- Layout mode selector ---
  if (dZoneLayout) {
    dZoneLayout.addEventListener('change', function() {
      zoneLayout = parseInt(dZoneLayout.value, 10) || 0;
      markSectionDirty('hardware');
      updatePreviewFromInputs();
    });
  }

  // --- WiFi Scan (Task 3 — optional SSID picker) ---
  var scanTimer = null;
  var scanStartTime = 0;
  var SCAN_TIMEOUT_MS = 5000;
  var SCAN_POLL_MS = 800;

  function escHtml(s) {
    var el = document.createElement('span');
    el.textContent = s;
    return el.innerHTML;
  }

  function rssiLabel(rssi) {
    if (rssi >= -50) return 'Excellent';
    if (rssi >= -60) return 'Good';
    if (rssi >= -70) return 'Fair';
    return 'Weak';
  }

  function startWifiScan() {
    clearTimeout(scanTimer);
    dBtnScan.disabled = true;
    dBtnScan.textContent = 'Scanning...';
    dScanResults.style.display = 'none';
    dScanResults.innerHTML = '';
    scanStartTime = Date.now();
    pollWifiScan();
  }

  function pollWifiScan() {
    FW.get('/api/wifi/scan').then(function(res) {
      var d = res.body;
      if (d.ok && !d.scanning && d.data && d.data.length > 0) {
        showWifiResults(d.data);
        return;
      }
      if (d.ok && !d.scanning) {
        finishScan('No networks found');
        return;
      }
      if (Date.now() - scanStartTime >= SCAN_TIMEOUT_MS) {
        finishScan('Scan timed out');
        return;
      }
      scanTimer = setTimeout(pollWifiScan, SCAN_POLL_MS);
    }).catch(function() {
      if (Date.now() - scanStartTime >= SCAN_TIMEOUT_MS) {
        finishScan('Scan failed');
        return;
      }
      scanTimer = setTimeout(pollWifiScan, SCAN_POLL_MS);
    });
  }

  function showWifiResults(networks) {
    clearTimeout(scanTimer);
    dBtnScan.disabled = false;
    dBtnScan.textContent = 'Scan for networks';
    dScanResults.style.display = '';
    dScanResults.innerHTML = '';

    var seen = {};
    networks.forEach(function(n) {
      if (!n.ssid) return;
      if (!seen[n.ssid] || n.rssi > seen[n.ssid].rssi) {
        seen[n.ssid] = n;
      }
    });
    var unique = Object.keys(seen).map(function(k) { return seen[k]; });
    unique.sort(function(a, b) { return b.rssi - a.rssi; });

    unique.forEach(function(n) {
      var row = document.createElement('div');
      row.className = 'scan-row';
      row.innerHTML = '<span class="ssid">' + escHtml(n.ssid) + '</span><span class="rssi">' + rssiLabel(n.rssi) + '</span>';
      row.addEventListener('click', function() {
        dWifiSsid.value = n.ssid;
        dScanResults.style.display = 'none';
        markSectionDirty('network');
      });
      dScanResults.appendChild(row);
    });
  }

  function finishScan(msg) {
    clearTimeout(scanTimer);
    dBtnScan.disabled = false;
    dBtnScan.textContent = 'Scan for networks';
    if (msg) FW.showToast(msg, 'warning');
  }

  dBtnScan.addEventListener('click', startWifiScan);

  // --- System: Factory Reset ---
  var btnReset = document.getElementById('btn-reset');
  var btnResetConfirm = document.getElementById('btn-reset-confirm');
  var btnResetCancel = document.getElementById('btn-reset-cancel');
  var resetDefault = document.getElementById('reset-default');
  var resetConfirm = document.getElementById('reset-confirm');

  btnReset.addEventListener('click', function() {
    resetDefault.style.display = 'none';
    resetConfirm.style.display = '';
  });

  btnResetCancel.addEventListener('click', function() {
    resetConfirm.style.display = 'none';
    resetDefault.style.display = '';
  });

  btnResetConfirm.addEventListener('click', function() {
    btnResetConfirm.disabled = true;
    btnResetCancel.disabled = true;
    btnResetConfirm.textContent = 'Resetting...';

    FW.post('/api/reset', {}).then(function(res) {
      if (res.body.ok) {
        FW.showToast('Factory reset complete. Rebooting...', 'warning');
        btnResetConfirm.textContent = 'Rebooting...';
      } else {
        throw new Error(res.body.error || 'Reset failed');
      }
    }).catch(function(err) {
      if (err && err.name === 'TypeError') {
        // Connection loss after reset is expected
        FW.showToast('Device is restarting...', 'warning');
        btnResetConfirm.textContent = 'Rebooting...';
        return;
      }
      FW.showToast(err && err.message ? err.message : 'Reset failed', 'error');
      btnResetConfirm.disabled = false;
      btnResetCancel.disabled = false;
      btnResetConfirm.textContent = 'Confirm';
    });
  });

  // --- Location card ---
  var locationToggle = document.querySelector('.location-toggle');
  var locationBody = document.getElementById('location-body');
  var locationMap = document.getElementById('location-map');
  var mapLoading = document.getElementById('map-loading');
  var locationFallback = document.getElementById('location-fallback');
  var locationHelper = document.getElementById('location-helper');
  var dCenterLat = document.getElementById('d-center-lat');
  var dCenterLon = document.getElementById('d-center-lon');
  var dRadiusKm = document.getElementById('d-radius-km');

  var leafletLoaded = false;
  var leafletLoadAttempted = false;
  var mapInstance = null;
  var mapMarker = null;
  var mapCircle = null;
  var mapTileLayer = null;
  var mapRadiusHandle = null;
  var mapLoadTimeout = null;
  var mapFailureHandled = false;
  var mapTileLoadSucceeded = false;
  var mapTileErrorCount = 0;
  var lastValidLocation = null;
  var LOCATION_INITIAL_ZOOM = 10;
  var LOCATION_TILE_ERROR_THRESHOLD = 2;
  var locationDebounce = null;

  function isLocationCardOpen() {
    return locationBody.style.display !== 'none';
  }

  function showLocationLoading(message) {
    mapLoading.textContent = message || 'Loading map...';
    mapLoading.style.display = '';
  }

  function hideLocationLoading() {
    mapLoading.style.display = 'none';
  }

  function cloneLocationValues(values) {
    return {
      center_lat: values.center_lat,
      center_lon: values.center_lon,
      radius_km: values.radius_km
    };
  }

  function formatCoordinate(value) {
    return Number(value).toFixed(6);
  }

  function formatRadius(value) {
    return (Math.round(Number(value) * 100) / 100).toString();
  }

  function normalizeLocationValues(values) {
    if (!values) return null;
    var lat = Number(values.center_lat);
    var lon = Number(values.center_lon);
    var radius = Number(values.radius_km);
    if (!Number.isFinite(lat) || !Number.isFinite(lon) || !Number.isFinite(radius)) return null;
    return {
      center_lat: clampLat(lat),
      center_lon: clampLon(lon),
      radius_km: clampRadius(radius)
    };
  }

  function readLocationValuesFromFields() {
    return normalizeLocationValues({
      center_lat: dCenterLat.value,
      center_lon: dCenterLon.value,
      radius_km: dRadiusKm.value
    });
  }

  function writeLocationFields(values) {
    dCenterLat.value = formatCoordinate(values.center_lat);
    dCenterLon.value = formatCoordinate(values.center_lon);
    dRadiusKm.value = formatRadius(values.radius_km);
  }

  function rememberValidLocation(values) {
    if (!values) return;
    lastValidLocation = cloneLocationValues(values);
  }

  function getRadiusHandleLatLng(centerLatLng, radiusMeters) {
    var bounds = L.circle(centerLatLng, { radius: radiusMeters }).getBounds();
    return L.latLng(centerLatLng.lat, bounds.getEast());
  }

  function updateMapFromValues(values, options) {
    if (!mapInstance || !mapMarker || !mapCircle) return;
    options = options || {};

    var latlng = L.latLng(values.center_lat, values.center_lon);
    var radiusMeters = values.radius_km * 1000;

    mapMarker.setLatLng(latlng);
    mapCircle.setLatLng(latlng);
    mapCircle.setRadius(radiusMeters);

    if (mapRadiusHandle) {
      mapRadiusHandle.setLatLng(getRadiusHandleLatLng(latlng, radiusMeters));
    }

    if (options.fit) {
      try { mapInstance.fitBounds(mapCircle.getBounds().pad(0.1)); } catch (e) { /* ignore */ }
    } else if (options.pan) {
      mapInstance.panTo(latlng);
    }
  }

  function restoreLastValidLocation() {
    if (!lastValidLocation) return false;
    writeLocationFields(lastValidLocation);
    updateMapFromValues(lastValidLocation, { fit: false, pan: false });
    return true;
  }

  function syncLocationFieldsFromMap(centerLatLng, radiusKm) {
    var values = normalizeLocationValues({
      center_lat: centerLatLng.lat,
      center_lon: centerLatLng.lng,
      radius_km: radiusKm
    });
    if (!values) return null;
    writeLocationFields(values);
    rememberValidLocation(values);
    return values;
  }

  function persistLocation(values) {
    var nextValues = normalizeLocationValues(values || readLocationValuesFromFields());
    if (!nextValues) {
      if (restoreLastValidLocation()) {
        FW.showToast('Enter valid latitude, longitude, and radius values', 'error');
      } else {
        FW.showToast('Location settings are not ready yet', 'error');
      }
      return;
    }

    rememberValidLocation(nextValues);

    FW.post('/api/settings', nextValues).then(function(res) {
      if (res.body.ok) {
        FW.showToast('Location updated', 'success');
      } else {
        FW.showToast(res.body.error || 'Save failed', 'error');
      }
    }).catch(function() {
      FW.showToast('Network error', 'error');
    });
  }

  function queueLocationPersist(values) {
    clearTimeout(locationDebounce);
    locationDebounce = setTimeout(function() {
      persistLocation(values);
    }, DEBOUNCE_MS);
  }

  function destroyLocationMap() {
    if (mapInstance) {
      mapInstance.remove();
    }
    mapInstance = null;
    mapMarker = null;
    mapCircle = null;
    mapTileLayer = null;
    mapRadiusHandle = null;
  }

  function onLeafletFail(helperText) {
    if (mapFailureHandled) return;
    mapFailureHandled = true;
    clearTimeout(mapLoadTimeout);
    destroyLocationMap();
    hideLocationLoading();
    locationMap.style.display = 'none';
    locationHelper.textContent = helperText || 'Map unavailable. Enter coordinates manually.';
    FW.showToast('Map could not be loaded', 'warning');
  }

  function maybeInitLocationMap() {
    if (!leafletLoaded || mapFailureHandled || mapInstance || !isLocationCardOpen()) return;

    if (!lastValidLocation) {
      settingsPromise.then(function() {
        if (!lastValidLocation) {
          onLeafletFail('Map unavailable until location settings load. Enter coordinates manually.');
          return;
        }
        maybeInitLocationMap();
      });
      return;
    }

    initMap(lastValidLocation);
  }

  function toggleLocationCard() {
    var shouldOpen = !isLocationCardOpen();

    if (shouldOpen && isCalibrationOpen()) {
      setCalibrationOpen(false);
    }

    locationBody.style.display = shouldOpen ? '' : 'none';
    locationToggle.classList.toggle('open', shouldOpen);

    if (!shouldOpen) return;

    if (mapInstance) {
      setTimeout(function() {
        if (!mapInstance || !isLocationCardOpen()) return;
        mapInstance.invalidateSize();
        if (lastValidLocation) {
          updateMapFromValues(lastValidLocation, { fit: true, pan: false });
        }
      }, 200);
      return;
    }

    if (!leafletLoadAttempted) {
      leafletLoadAttempted = true;
      loadLeaflet();
      return;
    }

    maybeInitLocationMap();
  }

  locationToggle.addEventListener('click', toggleLocationCard);

  function loadLeaflet() {
    if (leafletLoaded) {
      maybeInitLocationMap();
      return;
    }

    mapFailureHandled = false;
    mapTileLoadSucceeded = false;
    mapTileErrorCount = 0;
    showLocationLoading('Loading map...');
    locationMap.style.display = 'none';

    var link = document.createElement('link');
    link.rel = 'stylesheet';
    link.href = 'https://unpkg.com/leaflet@1.9.4/dist/leaflet.css';

    var script = document.createElement('script');
    script.src = 'https://unpkg.com/leaflet@1.9.4/dist/leaflet.js';

    mapLoadTimeout = setTimeout(function() {
      onLeafletFail('Map timed out. Enter coordinates manually.');
    }, 10000);

    script.onload = function() {
      clearTimeout(mapLoadTimeout);
      if (mapFailureHandled) return;
      leafletLoaded = true;
      maybeInitLocationMap();
    };

    script.onerror = function() {
      clearTimeout(mapLoadTimeout);
      onLeafletFail('Map assets could not be loaded. Enter coordinates manually.');
    };

    link.onerror = function() {
      clearTimeout(mapLoadTimeout);
      onLeafletFail('Map assets could not be loaded. Enter coordinates manually.');
    };

    document.head.appendChild(link);
    document.head.appendChild(script);
  }

  function clampLat(v) { return Math.max(-90, Math.min(90, v)); }
  function clampLon(v) { return Math.max(-180, Math.min(180, v)); }
  function clampRadius(v) { return Math.max(0.1, Math.min(500, v)); }

  function initMap(initialValues) {
    var values = normalizeLocationValues(initialValues);
    if (!values) {
      onLeafletFail('Map unavailable until location settings load. Enter coordinates manually.');
      return;
    }

    locationMap.style.display = '';
    showLocationLoading('Loading map...');
    mapTileLoadSucceeded = false;
    mapTileErrorCount = 0;

    var lat = values.center_lat;
    var lon = values.center_lon;
    var radiusM = values.radius_km * 1000;

    mapInstance = L.map(locationMap, { zoomControl: true }).setView([lat, lon], LOCATION_INITIAL_ZOOM);

    mapTileLayer = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      maxZoom: 18,
      attribution: '&copy; OpenStreetMap'
    });

    mapTileLayer.on('tileload', function() {
      mapTileLoadSucceeded = true;
      hideLocationLoading();
    });
    mapTileLayer.on('load', function() {
      mapTileLoadSucceeded = true;
      hideLocationLoading();
    });
    mapTileLayer.on('tileerror', function() {
      if (mapFailureHandled || mapTileLoadSucceeded) return;
      mapTileErrorCount += 1;
      if (mapTileErrorCount >= LOCATION_TILE_ERROR_THRESHOLD) {
        onLeafletFail('Map tiles could not be loaded. Enter coordinates manually.');
      }
    });
    mapTileLayer.addTo(mapInstance);

    mapMarker = L.marker([lat, lon], { draggable: true, autoPan: true }).addTo(mapInstance);
    mapCircle = L.circle([lat, lon], { radius: radiusM, color: '#58a6ff', fillOpacity: 0.15 }).addTo(mapInstance);
    mapRadiusHandle = L.marker(getRadiusHandleLatLng(L.latLng(lat, lon), radiusM), {
      draggable: true,
      autoPan: true,
      icon: L.divIcon({
        className: 'location-radius-handle',
        iconSize: [16, 16],
        iconAnchor: [8, 8]
      })
    }).addTo(mapInstance);

    mapMarker.on('drag', function() {
      var pos = mapMarker.getLatLng();
      mapCircle.setLatLng(pos);
      var dragValues = syncLocationFieldsFromMap(pos, mapCircle.getRadius() / 1000);
      if (dragValues && mapRadiusHandle) {
        mapRadiusHandle.setLatLng(getRadiusHandleLatLng(pos, dragValues.radius_km * 1000));
      }
    });
    mapMarker.on('dragend', function() {
      var pos = mapMarker.getLatLng();
      var dragValues = syncLocationFieldsFromMap(pos, mapCircle.getRadius() / 1000);
      if (!dragValues) return;
      updateMapFromValues(dragValues, { fit: false, pan: false });
      persistLocation(dragValues);
    });

    mapRadiusHandle.on('drag', function() {
      var center = mapMarker.getLatLng();
      var radiusKm = clampRadius(center.distanceTo(mapRadiusHandle.getLatLng()) / 1000);
      mapCircle.setRadius(radiusKm * 1000);
      syncLocationFieldsFromMap(center, radiusKm);
    });
    mapRadiusHandle.on('dragend', function() {
      var center = mapMarker.getLatLng();
      var radiusKm = clampRadius(center.distanceTo(mapRadiusHandle.getLatLng()) / 1000);
      var radiusValues = syncLocationFieldsFromMap(center, radiusKm);
      if (!radiusValues) return;
      updateMapFromValues(radiusValues, { fit: false, pan: false });
      persistLocation(radiusValues);
    });

    rememberValidLocation(values);
    updateMapFromValues(values, { fit: true, pan: false });
    setTimeout(function() {
      if (!mapInstance || !isLocationCardOpen()) return;
      mapInstance.invalidateSize();
      updateMapFromValues(values, { fit: true, pan: false });
    }, 200);
  }

  function handleLocationFieldChange() {
    var values = readLocationValuesFromFields();
    if (!values) {
      if (restoreLastValidLocation()) {
        FW.showToast('Enter valid latitude, longitude, and radius values', 'error');
      } else {
        FW.showToast('Location settings are not ready yet', 'error');
      }
      return;
    }

    writeLocationFields(values);
    rememberValidLocation(values);
    updateMapFromValues(values, { fit: false, pan: true });
    queueLocationPersist(values);

    if (!mapInstance && leafletLoaded && !mapFailureHandled && isLocationCardOpen()) {
      maybeInitLocationMap();
    }
  }

  dCenterLat.addEventListener('change', handleLocationFieldChange);
  dCenterLon.addEventListener('change', handleLocationFieldChange);
  dRadiusKm.addEventListener('change', handleLocationFieldChange);

  // --- Calibration card (Story 4.2) ---
  var calToggle = document.querySelector('.calibration-toggle');
  var calBody = document.getElementById('calibration-body');
  var calOrigin = document.getElementById('cal-origin-corner');
  var calScanDir = document.getElementById('cal-scan-dir');
  var calZigzag = document.getElementById('cal-zigzag');
  var calCanvas = document.getElementById('cal-preview-canvas');
  var calPreviewContainer = document.getElementById('cal-preview-container');
  var wiringCanvas = document.getElementById('wiring-preview');
  var wiringLegend = document.getElementById('wiring-legend');
  var calibrationActive = false;
  var positioningActive = false;
  var calPattern = 0; // 0=scan order, 1=panel position
  var calPatternToggle = document.getElementById('cal-pattern-toggle');
  var calDebounce = null;
  var CAL_DEBOUNCE_MS = 50;

  function isCalibrationOpen() {
    return calBody.style.display !== 'none';
  }

  function readCalibrationValues() {
    return {
      origin_corner: parseInt(calOrigin.value, 10),
      scan_dir: parseInt(calScanDir.value, 10),
      zigzag: parseInt(calZigzag.value, 10)
    };
  }

  function syncCalibrationFromSettings() {
    calOrigin.value = dOriginCorner.value || '0';
    calScanDir.value = dScanDir.value || '0';
    calZigzag.value = dZigzag.value || '0';
  }

  function renderCalibrationCanvas() {
    if (!calCanvas || !calCanvas.getContext || !calPreviewContainer) return;
    var ctx = calCanvas.getContext('2d');
    var dims = parseHardwareDimensionsFromInputs();

    if (!dims || dims.matrixWidth <= 0 || dims.matrixHeight <= 0) {
      calCanvas.width = 0;
      calCanvas.height = 0;
      calPreviewContainer.style.display = 'none';
      return;
    }

    calPreviewContainer.style.display = '';

    var mw = dims.matrixWidth;
    var mh = dims.matrixHeight;
    var originCorner = parseInt(calOrigin.value, 10) || 0;
    var scanDir = parseInt(calScanDir.value, 10) || 0;
    var zigzag = parseInt(calZigzag.value, 10) || 0;

    var containerWidth = calPreviewContainer.clientWidth || 300;
    var aspect = mw / mh;
    var drawWidth = Math.min(containerWidth, 480);
    var drawHeight = Math.round(drawWidth / aspect);

    calCanvas.width = drawWidth;
    calCanvas.height = drawHeight;

    var sx = drawWidth / mw;
    var sy = drawHeight / mh;

    // Background
    ctx.fillStyle = '#0d1117';
    ctx.fillRect(0, 0, drawWidth, drawHeight);

    // Tile grid
    var tp = dims.tilePixels;
    var tileStepX = tp * sx;
    var tileStepY = tp * sy;
    if (tileStepX >= 2 && tileStepY >= 2) {
      ctx.strokeStyle = '#21262d';
      ctx.lineWidth = 1;
      for (var gx = tileStepX; gx < drawWidth; gx += tileStepX) {
        var x = Math.round(gx) + 0.5;
        ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, drawHeight); ctx.stroke();
      }
      for (var gy = tileStepY; gy < drawHeight; gy += tileStepY) {
        var y = Math.round(gy) + 0.5;
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(drawWidth, y); ctx.stroke();
      }
    }

    // Position pattern mode: colored tiles with numbers
    if (calPattern === 1) {
      var tilesX = dims.tilesX;
      var tilesY = dims.tilesY;
      var totalTiles = tilesX * tilesY;
      for (var tRow = 0; tRow < tilesY; tRow++) {
        for (var tCol = 0; tCol < tilesX; tCol++) {
          var tIdx = tRow * tilesX + tCol;
          var tileHue = tIdx / totalTiles;

          // Dim fill
          var dimR = Math.round(40 * Math.max(0, Math.cos(tileHue * Math.PI * 2)));
          var dimG = Math.round(40 * Math.max(0, Math.cos((tileHue - 0.333) * Math.PI * 2)));
          var dimB = Math.round(40 * Math.max(0, Math.cos((tileHue - 0.667) * Math.PI * 2)));
          // Bright border
          var brtR = Math.round(200 * Math.max(0, Math.cos(tileHue * Math.PI * 2)));
          var brtG = Math.round(200 * Math.max(0, Math.cos((tileHue - 0.333) * Math.PI * 2)));
          var brtB = Math.round(200 * Math.max(0, Math.cos((tileHue - 0.667) * Math.PI * 2)));

          var tpx = tCol * tp * sx;
          var tpy = tRow * tp * sy;
          var tw = tp * sx;
          var th = tp * sy;

          // Dim fill
          ctx.fillStyle = 'rgb(' + (dimR + 20) + ',' + (dimG + 20) + ',' + (dimB + 20) + ')';
          ctx.fillRect(tpx, tpy, tw, th);

          // Bright border
          ctx.strokeStyle = 'rgb(' + (brtR + 55) + ',' + (brtG + 55) + ',' + (brtB + 55) + ')';
          ctx.lineWidth = Math.max(1, Math.round(sx));
          ctx.strokeRect(tpx + 0.5, tpy + 0.5, tw - 1, th - 1);

          // Red corner marker
          var mSize = Math.max(3, Math.round(Math.min(tw, th) * 0.15));
          ctx.fillStyle = '#f00';
          ctx.fillRect(tpx, tpy, mSize, mSize);

          // Tile number centered
          var fontSize = Math.max(10, Math.round(Math.min(tw, th) * 0.35));
          ctx.font = 'bold ' + fontSize + 'px sans-serif';
          ctx.fillStyle = '#fff';
          ctx.textAlign = 'center';
          ctx.textBaseline = 'middle';
          ctx.fillText(String(tIdx), tpx + tw / 2, tpy + th / 2);
        }
      }

      // Matrix outline
      ctx.strokeStyle = '#30363d';
      ctx.lineWidth = 1;
      ctx.strokeRect(0, 0, drawWidth, drawHeight);
      return;
    }

    // Draw calibration test pattern: numbered pixel traversal showing scan order
    // Simulate the NeoMatrix pixel mapping based on origin_corner, scan_dir, zigzag
    var totalPixels = mw * mh;
    var tilesX = dims.tilesX;
    var tilesY = dims.tilesY;

    // Build pixel order map to visualize scan direction
    // For each logical pixel index (0..N-1), compute the (px, py) position
    // This simulates how NeoMatrix maps pixel indices to XY coordinates
    var pixelCount = Math.min(totalPixels, 4096); // Cap for performance
    var stepSize = totalPixels > 4096 ? Math.ceil(totalPixels / 4096) : 1;
    var positions = [];

    for (var py = 0; py < mh; py++) {
      for (var px = 0; px < mw; px++) {
        // Determine tile and pixel within tile
        var tileCol = Math.floor(px / tp);
        var tileRow = Math.floor(py / tp);
        var localX = px % tp;
        var localY = py % tp;

        // Apply origin corner transform
        var effTileCol = tileCol;
        var effTileRow = tileRow;
        var effLocalX = localX;
        var effLocalY = localY;

        // Origin corner: 0=TL, 1=TR, 2=BL, 3=BR
        if (originCorner === 1 || originCorner === 3) {
          effTileCol = tilesX - 1 - tileCol;
          effLocalX = tp - 1 - localX;
        }
        if (originCorner === 2 || originCorner === 3) {
          effTileRow = tilesY - 1 - tileRow;
          effLocalY = tp - 1 - localY;
        }

        // Scan direction: 0=rows, 1=columns
        var tileIndex, localIndex;
        if (scanDir === 0) {
          // Row-major tile order
          if (zigzag && (effTileRow % 2 === 1)) {
            effTileCol = tilesX - 1 - effTileCol;
          }
          tileIndex = effTileRow * tilesX + effTileCol;
          // Local pixel within tile: row-major
          if (zigzag && (effLocalY % 2 === 1)) {
            effLocalX = tp - 1 - effLocalX;
          }
          localIndex = effLocalY * tp + effLocalX;
        } else {
          // Column-major tile order
          if (zigzag && (effTileCol % 2 === 1)) {
            effTileRow = tilesY - 1 - effTileRow;
          }
          tileIndex = effTileCol * tilesY + effTileRow;
          // Local pixel within tile: column-major
          if (zigzag && (effLocalX % 2 === 1)) {
            effLocalY = tp - 1 - effLocalY;
          }
          localIndex = effLocalX * tp + effLocalY;
        }

        var pixelIndex = tileIndex * (tp * tp) + localIndex;
        positions.push({ px: px, py: py, idx: pixelIndex });
      }
    }

    // Draw pixels colored by their index in the scan order (gradient red -> blue)
    for (var i = 0; i < positions.length; i++) {
      var p = positions[i];
      var t = totalPixels > 1 ? p.idx / (totalPixels - 1) : 0;
      t = Math.max(0, Math.min(1, t));
      // Red (start) -> Blue (end) gradient
      var r = Math.round(248 * (1 - t));
      var g = Math.round(81 * (1 - t) + 166 * t);
      var b = Math.round(73 * (1 - t) + 255 * t);
      ctx.fillStyle = 'rgb(' + r + ',' + g + ',' + b + ')';
      var rx = Math.round(p.px * sx);
      var ry = Math.round(p.py * sy);
      var rw = Math.max(1, Math.round(sx));
      var rh = Math.max(1, Math.round(sy));
      ctx.fillRect(rx, ry, rw, rh);
    }

    // Draw pixel 0 marker (start) and last pixel marker
    if (positions.length > 0) {
      // Find pixel 0 position
      var startPixel = null;
      var endPixel = null;
      for (var j = 0; j < positions.length; j++) {
        if (positions[j].idx === 0) startPixel = positions[j];
        if (positions[j].idx === totalPixels - 1) endPixel = positions[j];
      }

      if (startPixel) {
        var markerSize = Math.max(4, Math.round(Math.min(sx, sy) * 2));
        ctx.fillStyle = '#f85149';
        ctx.fillRect(
          Math.round(startPixel.px * sx) - 1,
          Math.round(startPixel.py * sy) - 1,
          markerSize, markerSize
        );
        ctx.fillStyle = '#fff';
        ctx.font = Math.max(8, Math.round(Math.min(sx, sy) * 3)) + 'px sans-serif';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'top';
        ctx.fillText('0', Math.round(startPixel.px * sx) + markerSize + 2, Math.round(startPixel.py * sy));
      }
    }

    // Matrix outline
    ctx.strokeStyle = '#30363d';
    ctx.lineWidth = 1;
    ctx.strokeRect(0, 0, drawWidth, drawHeight);
  }

  function startCalibrationMode() {
    if (calibrationActive) return;
    calibrationActive = true;
    FW.post('/api/calibration/start', {}).then(function(res) {
      if (!res.body.ok) {
        FW.showToast(res.body.error || 'Calibration start failed', 'error');
        calibrationActive = false;
      }
    }).catch(function() {
      FW.showToast('Network error starting calibration', 'error');
      calibrationActive = false;
    });
  }

  function stopCalibrationMode() {
    if (!calibrationActive) return;
    calibrationActive = false;
    FW.post('/api/calibration/stop', {}).catch(function() {});
  }

  function startPositioningMode() {
    if (positioningActive) return;
    positioningActive = true;
    FW.post('/api/positioning/start', {}).then(function(res) {
      if (!res.body.ok) {
        FW.showToast(res.body.error || 'Positioning start failed', 'error');
        positioningActive = false;
      }
    }).catch(function() {
      FW.showToast('Network error starting positioning', 'error');
      positioningActive = false;
    });
  }

  function stopPositioningMode() {
    if (!positioningActive) return;
    positioningActive = false;
    FW.post('/api/positioning/stop', {}).catch(function() {});
  }

  function activatePattern() {
    if (calPattern === 1) {
      stopCalibrationMode();
      startPositioningMode();
    } else {
      stopPositioningMode();
      startCalibrationMode();
    }
  }

  function stopAllTestPatterns() {
    stopCalibrationMode();
    stopPositioningMode();
  }

  function setCalibrationOpen(shouldOpen) {
    calBody.style.display = shouldOpen ? '' : 'none';
    calToggle.classList.toggle('open', shouldOpen);

    if (shouldOpen) {
      syncCalibrationFromSettings();
      renderCalibrationCanvas();
      activatePattern();
    } else {
      stopAllTestPatterns();
    }
  }

  // --- Wiring diagram canvas ---
  function renderWiringCanvas() {
    if (!wiringCanvas || !wiringCanvas.getContext || !previewContainer) return;
    var ctx = wiringCanvas.getContext('2d');
    var dims = parseHardwareDimensionsFromInputs();

    if (!dims || dims.tilesX <= 0 || dims.tilesY <= 0) {
      wiringCanvas.width = 0;
      wiringCanvas.height = 0;
      wiringCanvas.style.display = 'none';
      if (wiringLegend) wiringLegend.style.display = 'none';
      return;
    }

    wiringCanvas.style.display = '';
    if (wiringLegend) wiringLegend.style.display = '';

    var tilesX = dims.tilesX;
    var tilesY = dims.tilesY;
    var originCorner = parseInt(calOrigin.value, 10) || 0;
    var scanDir = parseInt(calScanDir.value, 10) || 0;
    var zigzag = parseInt(calZigzag.value, 10) || 0;

    var containerWidth = previewContainer.clientWidth || 300;
    var aspect = tilesX / tilesY;
    var drawWidth = Math.min(containerWidth, 480);
    var drawHeight = Math.round(drawWidth / aspect);
    if (drawHeight < 60) drawHeight = 60;

    wiringCanvas.width = drawWidth;
    wiringCanvas.height = drawHeight;

    var cellW = drawWidth / tilesX;
    var cellH = drawHeight / tilesY;

    // Background
    ctx.fillStyle = '#0d1117';
    ctx.fillRect(0, 0, drawWidth, drawHeight);

    // Tile grid lines
    ctx.strokeStyle = '#30363d';
    ctx.lineWidth = 1;
    for (var gx = 0; gx <= tilesX; gx++) {
      var x = Math.round(gx * cellW) + 0.5;
      ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, drawHeight); ctx.stroke();
    }
    for (var gy = 0; gy <= tilesY; gy++) {
      var y = Math.round(gy * cellH) + 0.5;
      ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(drawWidth, y); ctx.stroke();
    }

    // Build tile traversal order (same algorithm as calibration but at tile level)
    var tilePositions = [];
    for (var tRow = 0; tRow < tilesY; tRow++) {
      for (var tCol = 0; tCol < tilesX; tCol++) {
        var effCol = tCol;
        var effRow = tRow;

        // Origin corner transform
        if (originCorner === 1 || originCorner === 3) {
          effCol = tilesX - 1 - tCol;
        }
        if (originCorner === 2 || originCorner === 3) {
          effRow = tilesY - 1 - tRow;
        }

        // Scan direction + zigzag
        var tileIndex;
        if (scanDir === 0) {
          // Row-major
          if (zigzag && (effRow % 2 === 1)) {
            effCol = tilesX - 1 - effCol;
          }
          tileIndex = effRow * tilesX + effCol;
        } else {
          // Column-major
          if (zigzag && (effCol % 2 === 1)) {
            effRow = tilesY - 1 - effRow;
          }
          tileIndex = effCol * tilesY + effRow;
        }

        tilePositions.push({
          col: tCol, row: tRow,
          cx: (tCol + 0.5) * cellW,
          cy: (tRow + 0.5) * cellH,
          idx: tileIndex
        });
      }
    }

    // Sort by tile index to get wiring order
    tilePositions.sort(function(a, b) { return a.idx - b.idx; });

    // Draw cable path (thick blue polyline)
    if (tilePositions.length > 1) {
      ctx.strokeStyle = '#58a6ff';
      ctx.lineWidth = 3;
      ctx.lineJoin = 'round';
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(tilePositions[0].cx, tilePositions[0].cy);
      for (var i = 1; i < tilePositions.length; i++) {
        ctx.lineTo(tilePositions[i].cx, tilePositions[i].cy);
      }
      ctx.stroke();

      // Draw arrowheads along path at each segment midpoint
      ctx.fillStyle = '#58a6ff';
      for (var j = 0; j < tilePositions.length - 1; j++) {
        var ax = tilePositions[j].cx;
        var ay = tilePositions[j].cy;
        var bx = tilePositions[j + 1].cx;
        var by = tilePositions[j + 1].cy;
        var mx = (ax + bx) / 2;
        var my = (ay + by) / 2;
        var angle = Math.atan2(by - ay, bx - ax);
        var arrowSize = Math.min(cellW, cellH) * 0.18;
        if (arrowSize < 4) arrowSize = 4;
        ctx.save();
        ctx.translate(mx, my);
        ctx.rotate(angle);
        ctx.beginPath();
        ctx.moveTo(arrowSize, 0);
        ctx.lineTo(-arrowSize * 0.6, -arrowSize * 0.6);
        ctx.lineTo(-arrowSize * 0.6, arrowSize * 0.6);
        ctx.closePath();
        ctx.fill();
        ctx.restore();
      }
    }

    // Draw tile index numbers
    var fontSize = Math.max(10, Math.min(cellW, cellH) * 0.35);
    ctx.font = 'bold ' + Math.round(fontSize) + 'px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#e6edf3';
    for (var k = 0; k < tilePositions.length; k++) {
      var tp = tilePositions[k];
      ctx.fillText(String(tp.idx), tp.cx, tp.cy);
    }

    // Green marker at tile 0 (data input)
    if (tilePositions.length > 0) {
      var t0 = tilePositions[0];
      var markerR = Math.max(5, Math.min(cellW, cellH) * 0.15);
      ctx.beginPath();
      ctx.arc(t0.cx, t0.cy - cellH * 0.35, markerR, 0, Math.PI * 2);
      ctx.fillStyle = '#3fb950';
      ctx.fill();
      ctx.fillStyle = '#fff';
      ctx.font = 'bold ' + Math.round(markerR * 1.1) + 'px sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText('IN', t0.cx, t0.cy - cellH * 0.35);
    }

    // Matrix outline
    ctx.strokeStyle = '#58a6ff';
    ctx.lineWidth = 2;
    ctx.strokeRect(1, 1, drawWidth - 2, drawHeight - 2);
  }

  function onCalibrationChange() {
    var values = readCalibrationValues();

    // Sync back to hardware fields so Apply picks them up
    dOriginCorner.value = values.origin_corner;
    dScanDir.value = values.scan_dir;
    dZigzag.value = values.zigzag;

    // Instant canvas update
    renderCalibrationCanvas();
    renderWiringCanvas();

    // Debounced server POST (persists + updates LED test pattern)
    clearTimeout(calDebounce);
    calDebounce = setTimeout(function() {
      FW.post('/api/settings', {
        origin_corner: values.origin_corner,
        scan_dir: values.scan_dir,
        zigzag: values.zigzag
      }).then(function(res) {
        if (!res.body.ok) {
          FW.showToast(res.body.error || 'Save failed', 'error');
        } else if (res.body.reboot_required) {
          FW.showToast('Calibration saved. Reboot required to apply live mapping.', 'warning');
        } else {
          FW.showToast('Calibration updated', 'success');
        }
      }).catch(function() {
        FW.showToast('Network error', 'error');
      });
    }, CAL_DEBOUNCE_MS);
  }

  calOrigin.addEventListener('change', onCalibrationChange);
  calScanDir.addEventListener('change', onCalibrationChange);
  calZigzag.addEventListener('change', onCalibrationChange);

  function toggleCalibrationCard() {
    var shouldOpen = !isCalibrationOpen();
    setCalibrationOpen(shouldOpen);
  }

  calToggle.addEventListener('click', toggleCalibrationCard);

  // Pattern toggle buttons
  if (calPatternToggle) {
    var patternBtns = calPatternToggle.querySelectorAll('.cal-pattern-btn');
    for (var pi = 0; pi < patternBtns.length; pi++) {
      patternBtns[pi].addEventListener('click', function() {
        calPattern = parseInt(this.getAttribute('data-pattern'), 10) || 0;
        var all = calPatternToggle.querySelectorAll('.cal-pattern-btn');
        for (var j = 0; j < all.length; j++) all[j].classList.remove('active');
        this.classList.add('active');
        renderCalibrationCanvas();
        activatePattern();
      });
    }
  }

  // Stop all test patterns on page unload
  window.addEventListener('beforeunload', function() {
    try {
      if (calibrationActive) {
        var xhr1 = new XMLHttpRequest();
        xhr1.open('POST', '/api/calibration/stop', false);
        xhr1.setRequestHeader('Content-Type', 'application/json');
        xhr1.send('{}');
      }
      if (positioningActive) {
        var xhr2 = new XMLHttpRequest();
        xhr2.open('POST', '/api/positioning/stop', false);
        xhr2.setRequestHeader('Content-Type', 'application/json');
        xhr2.send('{}');
      }
    } catch (e) { /* best effort */ }
  });

  // Resize handler for calibration canvas
  window.addEventListener('resize', function() {
    if (isCalibrationOpen()) {
      renderCalibrationCanvas();
    }
  });

  // --- Logo upload (Story 4.3) ---
  var logoUploadZone = document.getElementById('logo-upload-zone');
  var logoFileInput = document.getElementById('logo-file-input');
  var logoFileList = document.getElementById('logo-file-list');
  var btnUploadLogos = document.getElementById('btn-upload-logos');
  var LOGO_PREVIEW_SIZE = 128;
  var logoPendingFiles = []; // { file, valid, error, previewDataUrl, row }

  function resetLogoUploadState() {
    logoPendingFiles = [];
    logoFileList.innerHTML = '';
    btnUploadLogos.style.display = 'none';
  }

  // Drag and drop
  logoUploadZone.addEventListener('dragover', function(e) {
    e.preventDefault();
    logoUploadZone.classList.add('drag-over');
  });
  logoUploadZone.addEventListener('dragleave', function() {
    logoUploadZone.classList.remove('drag-over');
  });
  logoUploadZone.addEventListener('drop', function(e) {
    e.preventDefault();
    logoUploadZone.classList.remove('drag-over');
    if (e.dataTransfer && e.dataTransfer.files) {
      processLogoFiles(e.dataTransfer.files);
    }
  });

  // File picker
  logoFileInput.addEventListener('change', function() {
    if (logoFileInput.files && logoFileInput.files.length) {
      processLogoFiles(logoFileInput.files);
    }
    logoFileInput.value = '';
  });

  function processLogoFiles(fileList) {
    resetLogoUploadState();
    var files = Array.prototype.slice.call(fileList);

    files.forEach(function(file) {
      var entry = { file: file, valid: false, error: '', previewDataUrl: null, row: null };

      // Validate extension
      if (!file.name.toLowerCase().endsWith('.bin')) {
        entry.error = file.name + ' - invalid format or size (.bin only)';
        logoPendingFiles.push(entry);
        renderLogoRow(entry);
        return;
      }

      // Validate size
      if (file.size !== 2048) {
        entry.error = file.name + ' - invalid format or size (' + file.size + ' bytes, expected 2048)';
        logoPendingFiles.push(entry);
        renderLogoRow(entry);
        return;
      }

      // Valid — decode RGB565 for preview
      entry.valid = true;
      logoPendingFiles.push(entry);
      renderLogoRow(entry);
      decodeRgb565Preview(entry);
    });

    updateUploadButton();
  }

  function decodeRgb565Preview(entry) {
    var reader = new FileReader();
    reader.onload = function() {
      if (reader.result.byteLength !== 2048) {
        entry.valid = false;
        entry.error = 'Decode error: unexpected pixel count';
        updateRowState(entry);
        updateUploadButton();
        return;
      }

      // Decode RGB565 (big-endian) -> RGBA for canvas
      var view = new DataView(reader.result);
      var canvas = document.createElement('canvas');
      canvas.width = 32;
      canvas.height = 32;
      var ctx = canvas.getContext('2d');
      var imgData = ctx.createImageData(32, 32);
      var pixels = imgData.data;

      for (var i = 0; i < 1024; i++) {
        var px = view.getUint16(i * 2, false); // big-endian
        var r5 = (px >> 11) & 0x1F;
        var g6 = (px >> 5) & 0x3F;
        var b5 = px & 0x1F;
        pixels[i * 4]     = (r5 << 3) | (r5 >> 2);
        pixels[i * 4 + 1] = (g6 << 2) | (g6 >> 4);
        pixels[i * 4 + 2] = (b5 << 3) | (b5 >> 2);
        pixels[i * 4 + 3] = 255;
      }

      ctx.putImageData(imgData, 0, 0);

      // Render scaled preview into the row's canvas
      if (entry.row) {
        var previewCanvas = entry.row.querySelector('.logo-file-preview');
        if (previewCanvas && previewCanvas.getContext) {
          var pctx = previewCanvas.getContext('2d');
          previewCanvas.width = LOGO_PREVIEW_SIZE;
          previewCanvas.height = LOGO_PREVIEW_SIZE;
          pctx.imageSmoothingEnabled = false;
          pctx.drawImage(canvas, 0, 0, LOGO_PREVIEW_SIZE, LOGO_PREVIEW_SIZE);
        }
      }
    };
    reader.onerror = function() {
      entry.valid = false;
      entry.error = 'File read error';
      updateRowState(entry);
      updateUploadButton();
    };
    reader.readAsArrayBuffer(entry.file);
  }

  function renderLogoRow(entry) {
    var row = document.createElement('div');
    row.className = 'logo-file-row' + (entry.valid ? '' : ' file-error');

    var preview = document.createElement('canvas');
    preview.className = 'logo-file-preview';
    preview.width = LOGO_PREVIEW_SIZE;
    preview.height = LOGO_PREVIEW_SIZE;

    var info = document.createElement('div');
    info.className = 'logo-file-info';
    var nameEl = document.createElement('span');
    nameEl.className = 'logo-file-name';
    nameEl.textContent = entry.file.name;
    var statusEl = document.createElement('span');
    statusEl.className = 'logo-file-status' + (entry.error ? ' status-error' : '');
    statusEl.textContent = entry.error || (entry.valid ? 'Ready' : '');
    info.appendChild(nameEl);
    info.appendChild(statusEl);

    var removeBtn = document.createElement('button');
    removeBtn.className = 'logo-file-remove';
    removeBtn.textContent = '\u00D7';
    removeBtn.setAttribute('aria-label', 'Remove');
    removeBtn.addEventListener('click', function() {
      var idx = logoPendingFiles.indexOf(entry);
      if (idx >= 0) logoPendingFiles.splice(idx, 1);
      row.parentNode.removeChild(row);
      updateUploadButton();
    });

    row.appendChild(preview);
    row.appendChild(info);
    row.appendChild(removeBtn);

    entry.row = row;
    logoFileList.appendChild(row);
  }

  function updateRowState(entry) {
    if (!entry.row) return;
    entry.row.className = 'logo-file-row' + (entry.valid ? '' : ' file-error');
    var statusEl = entry.row.querySelector('.logo-file-status');
    if (statusEl) {
      statusEl.className = 'logo-file-status' + (entry.error ? ' status-error' : '');
      statusEl.textContent = entry.error || (entry.valid ? 'Ready' : '');
    }
  }

  function updateUploadButton() {
    var hasValid = logoPendingFiles.some(function(e) { return e.valid; });
    btnUploadLogos.style.display = hasValid ? '' : 'none';
  }

  // Upload queue: POST one file at a time to /api/logos
  btnUploadLogos.addEventListener('click', function() {
    var validFiles = logoPendingFiles.filter(function(e) { return e.valid; });
    if (validFiles.length === 0) return;

    btnUploadLogos.disabled = true;
    btnUploadLogos.textContent = 'Uploading...';

    var successes = 0;
    var failures = 0;
    var idx = 0;
    var failureMessages = [];

    function uploadNext() {
      if (idx >= validFiles.length) {
        // All done
        btnUploadLogos.disabled = false;
        btnUploadLogos.textContent = 'Upload';

        if (successes > 0 && failures === 0) {
          FW.showToast(successes + ' logo' + (successes > 1 ? 's' : '') + ' uploaded', 'success');
        } else if (successes > 0 && failures > 0) {
          FW.showToast(successes + ' uploaded, ' + failures + ' failed: ' + failureMessages.join('; '), 'error');
        } else {
          FW.showToast(failureMessages.join('; ') || 'All uploads failed', 'error');
        }

        // Remove successfully uploaded entries
        logoPendingFiles = logoPendingFiles.filter(function(e) {
          if (e._uploaded) {
            if (e.row && e.row.parentNode) e.row.parentNode.removeChild(e.row);
            return false;
          }
          return true;
        });
        updateUploadButton();
        // Refresh logo list after upload completes
        if (successes > 0) loadLogoList();
        return;
      }

      var entry = validFiles[idx];
      idx++;

      var statusEl = entry.row ? entry.row.querySelector('.logo-file-status') : null;
      if (statusEl) {
        statusEl.className = 'logo-file-status';
        statusEl.textContent = 'Uploading...';
      }

      var formData = new FormData();
      formData.append('file', entry.file, entry.file.name);

      fetch('/api/logos', {
        method: 'POST',
        body: formData
      }).then(function(res) {
        return res.json().then(function(body) {
          return { status: res.status, body: body };
        });
      }).then(function(res) {
        if (res.body.ok) {
          successes++;
          entry._uploaded = true;
          if (entry.row) {
            entry.row.className = 'logo-file-row file-ok';
          }
          if (statusEl) {
            statusEl.className = 'logo-file-status status-ok';
            statusEl.textContent = 'Uploaded';
          }
        } else {
          failures++;
          entry.valid = false;
          entry.error = res.body.error || 'Upload failed';
          failureMessages.push(entry.file.name + ' - ' + entry.error);
          if (entry.row) {
            entry.row.className = 'logo-file-row file-error';
          }
          if (statusEl) {
            statusEl.className = 'logo-file-status status-error';
            statusEl.textContent = entry.error;
          }
        }
        uploadNext();
      }).catch(function() {
        failures++;
        entry.valid = false;
        entry.error = 'Network error';
        failureMessages.push(entry.file.name + ' - Network error');
        if (entry.row) {
          entry.row.className = 'logo-file-row file-error';
        }
        if (statusEl) {
          statusEl.className = 'logo-file-status status-error';
          statusEl.textContent = 'Network error';
        }
        uploadNext();
      });
    }

    uploadNext();
  });

  // --- Logo list management (Story 4.4) ---
  var logoListEl = document.getElementById('logo-list');
  var logoEmptyState = document.getElementById('logo-empty-state');
  var logoStorageSummary = document.getElementById('logo-storage-summary');
  var logoListConfirmingRow = null; // only one row in confirm state at a time
  var logoDeleteInFlight = false;

  function formatBytes(bytes) {
    if (bytes < 1024) return bytes + ' B';
    var kb = bytes / 1024;
    if (kb < 1024) return Math.round(kb) + ' KB';
    var mb = kb / 1024;
    return (Math.round(mb * 10) / 10) + ' MB';
  }

  function loadLogoList() {
    FW.get('/api/logos').then(function(res) {
      if (!res.body.ok) {
        FW.showToast(res.body.error || 'Could not load logos. Check the device and try again.', 'error');
        return;
      }

      var logos = res.body.data || [];
      var storage = res.body.storage || {};

      // Sort deterministically by name for stable order
      logos.sort(function(a, b) {
        return a.name.localeCompare(b.name);
      });

      // Storage summary
      if (storage.used !== undefined && storage.total !== undefined) {
        logoStorageSummary.textContent = 'Storage: ' + formatBytes(storage.used) + ' / ' + formatBytes(storage.total) + ' used (' + (storage.logo_count || 0) + ' logos)';
        logoStorageSummary.style.display = '';
      } else {
        logoStorageSummary.style.display = 'none';
      }

      // Empty state vs list
      logoListEl.innerHTML = '';
      logoListConfirmingRow = null;
      if (logos.length === 0) {
        logoEmptyState.style.display = '';
        return;
      }
      logoEmptyState.style.display = 'none';

      logos.forEach(function(logo) {
        renderLogoListRow(logo);
      });
    }).catch(function() {
      FW.showToast('Cannot reach the device to load logos. Check connection and try again.', 'error');
    });
  }

  function renderLogoListRow(logo) {
    var row = document.createElement('div');
    row.className = 'logo-list-row';
    row.setAttribute('data-filename', logo.name);

    // Thumbnail canvas — load binary from device and decode RGB565
    var thumb = document.createElement('canvas');
    thumb.className = 'logo-list-thumb';
    thumb.width = 48;
    thumb.height = 48;

    var info = document.createElement('div');
    info.className = 'logo-list-info';
    var nameEl = document.createElement('span');
    nameEl.className = 'logo-list-name';
    nameEl.textContent = logo.name;
    var sizeEl = document.createElement('span');
    sizeEl.className = 'logo-list-size';
    sizeEl.textContent = formatBytes(logo.size);
    info.appendChild(nameEl);
    info.appendChild(sizeEl);

    var actions = document.createElement('div');
    actions.className = 'logo-list-actions';

    var deleteBtn = document.createElement('button');
    deleteBtn.className = 'logo-list-delete';
    deleteBtn.textContent = 'Delete';
    deleteBtn.type = 'button';
    deleteBtn.addEventListener('click', function() {
      showInlineConfirm(row, logo.name, actions);
    });
    actions.appendChild(deleteBtn);

    row.appendChild(thumb);
    row.appendChild(info);
    row.appendChild(actions);
    logoListEl.appendChild(row);

    // Load thumbnail preview from device
    loadLogoThumbnail(thumb, logo.name);
  }

  function loadLogoThumbnail(canvas, filename) {
    fetch('/logos/' + encodeURIComponent(filename))
      .then(function(res) {
        if (!res.ok) return null;
        return res.arrayBuffer();
      })
      .then(function(buf) {
        if (!buf || buf.byteLength !== 2048) return;
        var view = new DataView(buf);
        var ctx = canvas.getContext('2d');
        var imgData = ctx.createImageData(32, 32);
        var d = imgData.data;
        for (var i = 0; i < 1024; i++) {
          var px = view.getUint16(i * 2, false); // big-endian
          var r5 = (px >> 11) & 0x1F;
          var g6 = (px >> 5) & 0x3F;
          var b5 = px & 0x1F;
          d[i * 4]     = (r5 << 3) | (r5 >> 2);
          d[i * 4 + 1] = (g6 << 2) | (g6 >> 4);
          d[i * 4 + 2] = (b5 << 3) | (b5 >> 2);
          d[i * 4 + 3] = 255;
        }
        // Draw 32x32 to offscreen then scale to canvas
        var offscreen = document.createElement('canvas');
        offscreen.width = 32;
        offscreen.height = 32;
        offscreen.getContext('2d').putImageData(imgData, 0, 0);
        canvas.width = 48;
        canvas.height = 48;
        ctx = canvas.getContext('2d');
        ctx.imageSmoothingEnabled = false;
        ctx.drawImage(offscreen, 0, 0, 48, 48);
      })
      .catch(function() { /* thumbnail load failed — leave blank */ });
  }

  function showInlineConfirm(row, filename, actionsEl) {
    // Dismiss any other confirming row first
    if (logoListConfirmingRow && logoListConfirmingRow !== row) {
      resetRowActions(logoListConfirmingRow);
    }
    logoListConfirmingRow = row;

    actionsEl.innerHTML = '';

    var text = document.createElement('span');
    text.className = 'logo-list-confirm-text';
    text.textContent = 'Delete ' + filename + '?';

    var confirmBtn = document.createElement('button');
    confirmBtn.className = 'logo-list-confirm-btn';
    confirmBtn.textContent = 'Confirm';
    confirmBtn.type = 'button';
    confirmBtn.addEventListener('click', function() {
      executeDelete(row, filename, actionsEl, confirmBtn);
    });

    var cancelBtn = document.createElement('button');
    cancelBtn.className = 'logo-list-cancel-btn';
    cancelBtn.textContent = 'Cancel';
    cancelBtn.type = 'button';
    cancelBtn.addEventListener('click', function() {
      resetRowActions(row);
      logoListConfirmingRow = null;
    });

    actionsEl.appendChild(text);
    actionsEl.appendChild(confirmBtn);
    actionsEl.appendChild(cancelBtn);
  }

  function resetRowActions(row) {
    var actionsEl = row.querySelector('.logo-list-actions');
    if (!actionsEl) return;
    var filename = row.getAttribute('data-filename');
    actionsEl.innerHTML = '';
    var deleteBtn = document.createElement('button');
    deleteBtn.className = 'logo-list-delete';
    deleteBtn.textContent = 'Delete';
    deleteBtn.type = 'button';
    deleteBtn.addEventListener('click', function() {
      showInlineConfirm(row, filename, actionsEl);
    });
    actionsEl.appendChild(deleteBtn);
  }

  function executeDelete(row, filename, actionsEl, confirmBtn) {
    if (logoDeleteInFlight) return;
    logoDeleteInFlight = true;
    confirmBtn.disabled = true;
    confirmBtn.textContent = 'Deleting...';

    FW.del('/api/logos/' + encodeURIComponent(filename))
      .then(function(res) {
        logoDeleteInFlight = false;
        logoListConfirmingRow = null;
        if (res.body.ok) {
          FW.showToast('Logo deleted', 'success');
          loadLogoList();
        } else {
          var errMsg = res.body.error || 'Delete failed';
          if (res.body.code === 'NOT_FOUND') errMsg = filename + ' not found';
          FW.showToast(errMsg, 'error');
          resetRowActions(row);
        }
      })
      .catch(function() {
        logoDeleteInFlight = false;
        logoListConfirmingRow = null;
        FW.showToast('Cannot reach the device to delete ' + filename + '. Check connection and try again.', 'error');
        resetRowActions(row);
      });
  }

  // --- Unified Apply ---
  btnApplyAll.addEventListener('click', function() {
    var payload = collectPayload();
    if (payload === null) return; // validation failed

    if (Object.keys(payload).length === 0) {
      clearDirtyState();
      return;
    }

    applyWithReboot(payload, btnApplyAll, 'Apply Changes');

    // Clear dirty state after successful send (applyWithReboot handles UI)
    // We clear immediately since applyWithReboot manages the button state
    clearDirtyState();
  });

  // --- Mode Picker (Story dl-1.5) ---
  var modeStatusName = document.getElementById('modeStatusName');
  var modeStatusReason = document.getElementById('modeStatusReason');
  var modeCardsList = document.getElementById('modeCardsList');
  var modeSwitchInFlight = false;

  function updateModeStatus(data) {
    var activeMode = null;
    if (data.modes && data.active) {
      for (var i = 0; i < data.modes.length; i++) {
        if (data.modes[i].id === data.active) {
          activeMode = data.modes[i];
          break;
        }
      }
    }
    // Batch DOM update — status line + card subtitles in one operation
    if (modeStatusName && activeMode) {
      modeStatusName.textContent = activeMode.name;
    }
    if (modeStatusReason) {
      modeStatusReason.textContent = data.state_reason || 'unknown';
    }
    // Update mode cards (active state + subtitle)
    if (modeCardsList) {
      var cards = modeCardsList.querySelectorAll('.mode-card');
      for (var j = 0; j < cards.length; j++) {
        var cardId = cards[j].getAttribute('data-mode-id');
        var subtitle = cards[j].querySelector('.mode-card-subtitle');
        if (cardId === data.active) {
          cards[j].classList.add('active');
          if (subtitle) subtitle.textContent = data.state_reason || '';
        } else {
          cards[j].classList.remove('active');
          if (subtitle) subtitle.textContent = '';
        }
        cards[j].classList.remove('switching');
      }
    }
  }

  function renderModeCards(data) {
    if (!modeCardsList || !data.modes) return;
    modeCardsList.innerHTML = '';
    for (var i = 0; i < data.modes.length; i++) {
      var mode = data.modes[i];
      var card = document.createElement('div');
      card.className = 'mode-card' + (mode.id === data.active ? ' active' : '');
      card.setAttribute('data-mode-id', mode.id);
      var nameEl = document.createElement('div');
      nameEl.className = 'mode-card-name';
      nameEl.textContent = mode.name;
      card.appendChild(nameEl);
      var subtitleEl = document.createElement('div');
      subtitleEl.className = 'mode-card-subtitle';
      subtitleEl.textContent = (mode.id === data.active) ? (data.state_reason || '') : '';
      card.appendChild(subtitleEl);
      card.addEventListener('click', (function(modeId) {
        return function() { switchMode(modeId); };
      })(mode.id));
      modeCardsList.appendChild(card);
    }
  }

  function switchMode(modeId) {
    if (modeSwitchInFlight) return;
    modeSwitchInFlight = true;
    // Set switching state on the target card
    if (modeCardsList) {
      var cards = modeCardsList.querySelectorAll('.mode-card');
      for (var i = 0; i < cards.length; i++) {
        if (cards[i].getAttribute('data-mode-id') === modeId) {
          cards[i].classList.add('switching');
          var sub = cards[i].querySelector('.mode-card-subtitle');
          if (sub) sub.textContent = 'Switching...';
        }
      }
    }
    FW.post('/api/display/mode', { mode_id: modeId }).then(function(res) {
      modeSwitchInFlight = false;
      if (!res.body || !res.body.ok) {
        var errMsg = (res.body && res.body.error) ? res.body.error : 'Mode switch failed';
        FW.showToast(errMsg, 'error');
        loadDisplayModes(); // refresh to clear switching state
        return;
      }
      // Re-fetch full mode state for consistent update (AC #4, #8)
      loadDisplayModes();
    }).catch(function() {
      modeSwitchInFlight = false;
      FW.showToast('Cannot reach device. Check connection.', 'error');
      loadDisplayModes();
    });
  }

  function loadDisplayModes() {
    FW.get('/api/display/modes').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) {
        FW.showToast('Failed to load display modes', 'error');
        return;
      }
      var data = res.body.data;
      renderModeCards(data);
      updateModeStatus(data);
    }).catch(function() {
      FW.showToast('Cannot reach device to load display modes. Check connection.', 'error');
    });
  }

  // --- Firmware card / OTA Upload (Story fn-1.6) ---
  var otaUploadZone = document.getElementById('ota-upload-zone');
  var otaFileInput = document.getElementById('ota-file-input');
  var otaFileInfo = document.getElementById('ota-file-info');
  var otaFileName = document.getElementById('ota-file-name');
  var btnUploadFirmware = document.getElementById('btn-upload-firmware');
  var otaProgress = document.getElementById('ota-progress');
  var otaProgressBar = document.getElementById('ota-progress-bar');
  var otaProgressText = document.getElementById('ota-progress-text');
  var otaReboot = document.getElementById('ota-reboot');
  var otaRebootText = document.getElementById('ota-reboot-text');
  var fwVersion = document.getElementById('fw-version');
  var rollbackBanner = document.getElementById('rollback-banner');
  var btnDismissRollback = document.getElementById('btn-dismiss-rollback');
  var otaPendingFile = null;
  var OTA_MAX_SIZE = 1572864; // 1.5MB = 0x180000
  var btnCancelOta = document.getElementById('btn-cancel-ota');

  function resetOtaUploadState() {
    otaPendingFile = null;
    otaUploadZone.style.display = '';
    otaFileInfo.style.display = 'none';
    otaProgress.style.display = 'none';
    otaReboot.style.display = 'none';
    otaProgressBar.style.width = '0%';
    otaProgress.setAttribute('aria-valuenow', '0');
    otaProgressText.textContent = '0%';
    // Reset reboot text and color so a subsequent upload starts clean
    otaRebootText.textContent = '';
    otaRebootText.style.color = '';
  }

  // Cancel file selection — return to upload zone
  if (btnCancelOta) {
    btnCancelOta.addEventListener('click', function() {
      resetOtaUploadState();
    });
  }

  function loadFirmwareStatus() {
    FW.get('/api/status').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) return;
      var d = res.body.data;
      if (d.firmware_version) {
        fwVersion.textContent = 'Version: v' + d.firmware_version;
      }
      if (d.rollback_detected && !d.rollback_acknowledged) {
        rollbackBanner.style.display = '';
      } else {
        rollbackBanner.style.display = 'none';
      }
    }).catch(function() {
      FW.showToast('Could not load firmware status \u2014 check connection', 'error');
    });
  }

  // Rollback banner dismiss
  if (btnDismissRollback) {
    btnDismissRollback.addEventListener('click', function() {
      FW.post('/api/ota/ack-rollback', {}).then(function(res) {
        if (res.body.ok) {
          rollbackBanner.style.display = 'none';
        }
      }).catch(function() {
        FW.showToast('Could not dismiss rollback banner', 'error');
      });
    });
  }

  // Click/keyboard to open file picker
  otaUploadZone.addEventListener('click', function() {
    otaFileInput.click();
  });
  otaUploadZone.addEventListener('keydown', function(e) {
    if (e.key === 'Enter' || e.key === ' ') {
      e.preventDefault();
      otaFileInput.click();
    }
  });

  // Drag and drop
  otaUploadZone.addEventListener('dragenter', function(e) {
    e.preventDefault();
    otaUploadZone.classList.add('drag-over');
  });
  otaUploadZone.addEventListener('dragover', function(e) {
    e.preventDefault();
    otaUploadZone.classList.add('drag-over');
  });
  otaUploadZone.addEventListener('dragleave', function(e) {
    // Only remove drag-over when the cursor truly leaves the zone, not
    // when it moves over a child element (which fires dragleave on parent).
    if (!otaUploadZone.contains(e.relatedTarget)) {
      otaUploadZone.classList.remove('drag-over');
    }
  });
  otaUploadZone.addEventListener('drop', function(e) {
    e.preventDefault();
    otaUploadZone.classList.remove('drag-over');
    if (e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files.length > 0) {
      validateAndSelectFile(e.dataTransfer.files[0]);
    }
  });

  // File input change
  otaFileInput.addEventListener('change', function() {
    if (otaFileInput.files && otaFileInput.files.length > 0) {
      validateAndSelectFile(otaFileInput.files[0]);
    }
    otaFileInput.value = '';
  });

  function validateAndSelectFile(file) {
    // Size check — reject empty files and files exceeding the OTA partition limit
    if (file.size === 0) {
      FW.showToast('File is empty \u2014 select a valid firmware .bin file', 'error');
      resetOtaUploadState();
      return;
    }
    if (file.size > OTA_MAX_SIZE) {
      FW.showToast('File too large \u2014 maximum 1.5MB for OTA partition', 'error');
      resetOtaUploadState();
      return;
    }

    // Magic byte check
    var reader = new FileReader();
    reader.onload = function(e) {
      var bytes = new Uint8Array(e.target.result);
      if (bytes[0] !== 0xE9) {
        FW.showToast('Not a valid ESP32 firmware image', 'error');
        resetOtaUploadState();
        return;
      }
      // Valid — show file info
      otaPendingFile = file;
      otaUploadZone.style.display = 'none';
      otaFileInfo.style.display = '';
      otaFileName.textContent = file.name;
    };
    reader.onerror = function() {
      FW.showToast('Could not read file', 'error');
      resetOtaUploadState();
    };
    reader.readAsArrayBuffer(file.slice(0, 4));
  }

  // Upload firmware via XMLHttpRequest (not fetch — XHR required for upload progress events)
  if (btnUploadFirmware) {
    btnUploadFirmware.addEventListener('click', function() {
      if (!otaPendingFile) return;
      uploadFirmware(otaPendingFile);
    });
  }

  function uploadFirmware(file) {
    // Prevent double-submit (e.g. rapid double-tap on slow connections)
    if (btnUploadFirmware) btnUploadFirmware.disabled = true;

    var xhr = new XMLHttpRequest();
    var formData = new FormData();
    formData.append('firmware', file, file.name);

    // Show progress bar
    otaFileInfo.style.display = 'none';
    otaProgress.style.display = '';

    xhr.upload.onprogress = function(e) {
      if (e.lengthComputable) {
        var pct = Math.round((e.loaded / e.total) * 100);
        updateOtaProgress(pct);
      }
    };

    xhr.onload = function() {
      if (btnUploadFirmware) btnUploadFirmware.disabled = false;
      if (xhr.status === 200) {
        try {
          var resp = JSON.parse(xhr.responseText);
          if (resp.ok) {
            updateOtaProgress(100);
            startRebootCountdown();
          } else {
            FW.showToast(resp.error || 'Upload failed', 'error');
            resetOtaUploadState();
          }
        } catch (e) {
          FW.showToast('Upload failed \u2014 invalid response', 'error');
          resetOtaUploadState();
        }
      } else {
        try {
          var errResp = JSON.parse(xhr.responseText);
          FW.showToast(errResp.error || 'Upload failed', 'error');
        } catch (e) {
          FW.showToast('Upload failed \u2014 status ' + xhr.status, 'error');
        }
        resetOtaUploadState();
      }
    };

    xhr.onerror = function() {
      if (btnUploadFirmware) btnUploadFirmware.disabled = false;
      FW.showToast('Connection lost during upload', 'error');
      resetOtaUploadState();
    };

    // Timeout for stalled uploads (2 minutes)
    xhr.timeout = 120000;
    xhr.ontimeout = function() {
      if (btnUploadFirmware) btnUploadFirmware.disabled = false;
      FW.showToast('Upload timed out \u2014 try again', 'error');
      resetOtaUploadState();
    };

    xhr.open('POST', '/api/ota/upload');
    xhr.send(formData);
  }

  function updateOtaProgress(pct) {
    otaProgressBar.style.width = pct + '%';
    otaProgress.setAttribute('aria-valuenow', String(pct));
    otaProgressText.textContent = pct + '%';
  }

  function startRebootCountdown() {
    otaProgress.style.display = 'none';
    otaReboot.style.display = '';
    var count = 3;
    otaRebootText.textContent = 'Rebooting in ' + count + '...';
    var countdownInterval = setInterval(function() {
      count--;
      if (count > 0) {
        otaRebootText.textContent = 'Rebooting in ' + count + '...';
      } else {
        clearInterval(countdownInterval);
        otaRebootText.textContent = 'Waiting for device...';
        startRebootPolling();
      }
    }, 1000);
  }

  function startRebootPolling() {
    var attempts = 0;
    var maxAttempts = 20;
    var done = false;

    function poll() {
      if (done) return;
      // Check timeout BEFORE issuing the next request so a slow final request
      // cannot trigger both the success toast and the timeout message.
      if (attempts >= maxAttempts) {
        done = true;
        otaRebootText.textContent = 'Device unreachable \u2014 try refreshing. The device may have changed IP address after reboot.';
        otaRebootText.style.color = getComputedStyle(document.documentElement).getPropertyValue('--warning').trim() || '#d29922';
        return;
      }
      attempts++;
      // Use recursive setTimeout (not setInterval) so the next poll only fires
      // AFTER the current request resolves — prevents concurrent in-flight
      // requests piling up against the resource-constrained ESP32.
      FW.get('/api/status?_=' + Date.now()).then(function(res) {
        if (done) return;  // timeout already fired; discard late response
        if (res.body && res.body.ok && res.body.data) {
          done = true;
          var newVersion = res.body.data.firmware_version || '';
          FW.showToast('Updated to v' + newVersion, 'success');
          fwVersion.textContent = 'Version: v' + newVersion;
          resetOtaUploadState();
          // Check rollback state after update
          if (res.body.data.rollback_detected && !res.body.data.rollback_acknowledged) {
            rollbackBanner.style.display = '';
          } else {
            rollbackBanner.style.display = 'none';
          }
        } else {
          // Partial or not-yet-ready response — retry after delay
          setTimeout(poll, 3000);
        }
      }).catch(function() {
        // Device not yet reachable — retry after delay
        if (!done) setTimeout(poll, 3000);
      });
    }

    poll();
  }

  // --- Settings Export (Story fn-1.6) ---
  var btnExportSettings = document.getElementById('btn-export-settings');
  if (btnExportSettings) {
    btnExportSettings.addEventListener('click', function() {
      // Direct navigation triggers browser download via Content-Disposition header
      window.location.href = '/api/settings/export';
    });
  }

  // --- Init ---
  var settingsPromise = loadSettings();

  // Load firmware status (version, rollback) on page load (Story fn-1.6)
  loadFirmwareStatus();

  // Load display modes on page load (Story dl-1.5)
  loadDisplayModes();

  // Load the logo list on page load
  loadLogoList();

  // Fetch /api/layout for initial canvas (best-effort)
  FW.get('/api/layout').then(function(res) {
    if (!res.body || !res.body.ok || !res.body.data || hardwareInputDirty) return;
    var layout = normalizeLayoutFromApi(res.body.data);
    if (!layout) return;

    suppressHardwareInputHandler = true;
    dTilesX.value = layout.hardware.tilesX;
    dTilesY.value = layout.hardware.tilesY;
    dTilePixels.value = layout.hardware.tilePixels;
    suppressHardwareInputHandler = false;
    setResolutionText({
      matrixWidth: layout.matrixWidth,
      matrixHeight: layout.matrixHeight
    });
    renderLayoutCanvas(layout);
    renderWiringCanvas();
  }).catch(function() {
    // Fallback: render from settings-loaded form values.
    settingsPromise.then(function() {
      if (!hardwareInputDirty) {
        updateHwResolution();
        updatePreviewFromInputs();
      }
    });
  });
})();


]]></file>
<file id="6a793299" path="firmware/data-src/style.css" label="STYLESHEET"><![CDATA[

/* FlightWall shared design tokens and layout */
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#0d1117;--surface:#161b22;--border:#30363d;
  --text:#e6edf3;--text-muted:#8b949e;
  --primary:#58a6ff;--primary-hover:#79c0ff;
  --error:#f85149;--success:#3fb950;--warning:#d29922;
  --radius:8px;--gap:16px;
}
html{font-size:16px}
body{
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Helvetica,Arial,sans-serif;
  background:var(--bg);color:var(--text);
  min-height:100vh;display:flex;justify-content:center;
  padding:0;margin:0;
  -webkit-text-size-adjust:100%;
}

/* Wizard layout */
.wizard{
  width:100%;max-width:480px;min-height:100vh;
  display:flex;flex-direction:column;
  padding:var(--gap);
}
.wizard-header{text-align:center;padding:var(--gap) 0;flex-shrink:0}
.wizard-header h1{font-size:1.25rem;font-weight:600;margin-bottom:4px}
.wizard-header #progress{color:var(--text-muted);font-size:0.875rem}
.wizard-content{flex:1;overflow-y:auto;padding-bottom:var(--gap)}

.step{display:none}
.step.active{display:block}
.step h2{font-size:1.125rem;margin-bottom:8px}
.step-desc{color:var(--text-muted);font-size:0.875rem;margin-bottom:var(--gap)}
.placeholder-note{color:var(--text-muted);font-style:italic;margin-top:var(--gap)}

/* Form fields */
.form-fields{display:flex;flex-direction:column;gap:8px}
.form-fields label{font-size:0.875rem;color:var(--text-muted);margin-top:8px}
.form-fields input[type="text"],
.form-fields input[type="password"]{
  width:100%;padding:12px;font-size:1rem;
  background:var(--surface);color:var(--text);
  border:1px solid var(--border);border-radius:var(--radius);
  outline:none;transition:border-color 0.15s;
}
.form-fields input:focus{border-color:var(--primary)}
.form-fields input.input-error{border-color:var(--error)}
.field-error{color:var(--error);font-size:0.8125rem;margin-top:4px}

/* Scan states */
.scan-status{text-align:center;padding:var(--gap) 0}
.scan-msg{color:var(--text-muted);margin-bottom:8px}
.spinner{
  width:24px;height:24px;margin:0 auto;
  border:3px solid var(--border);border-top-color:var(--primary);
  border-radius:50%;animation:spin 0.8s linear infinite;
}
@keyframes spin{to{transform:rotate(360deg)}}

/* Scan results */
.scan-results{display:flex;flex-direction:column;gap:4px;margin-bottom:var(--gap)}
.scan-row{
  padding:12px;background:var(--surface);
  border:1px solid var(--border);border-radius:var(--radius);
  cursor:pointer;display:flex;justify-content:space-between;align-items:center;
  transition:border-color 0.15s;
}
.scan-row:hover,.scan-row:active{border-color:var(--primary)}
.scan-row .ssid{font-size:0.9375rem}
.scan-row .rssi{font-size:0.75rem;color:var(--text-muted)}

/* Link button */
.link-btn{
  background:none;border:none;color:var(--primary);
  font-size:0.875rem;cursor:pointer;padding:8px 0;
  text-decoration:underline;
}
.link-btn:hover{color:var(--primary-hover)}

/* Wizard navigation */
.wizard-nav{
  display:flex;justify-content:space-between;
  padding:var(--gap) 0;flex-shrink:0;
  border-top:1px solid var(--border);
}
.nav-btn{
  padding:12px 24px;font-size:1rem;border-radius:var(--radius);
  border:1px solid var(--border);background:var(--surface);
  color:var(--text);cursor:pointer;min-width:110px;
  transition:background 0.15s,border-color 0.15s;
}
.nav-btn:hover{border-color:var(--text-muted)}
.nav-btn-primary{
  background:var(--primary);color:#0d1117;border-color:var(--primary);
  font-weight:600;
}
.nav-btn-primary:hover{background:var(--primary-hover);border-color:var(--primary-hover)}
.nav-btn-save{
  background:var(--success);color:#0d1117;border-color:var(--success);
  font-weight:600;
}
.nav-btn-save:hover{background:#46d05a;border-color:#46d05a}
.nav-btn:disabled{opacity:0.5;cursor:default}

/* Geolocation button (Step 3) */
.geo-btn{
  display:block;width:100%;padding:12px;font-size:1rem;
  background:var(--surface);color:var(--primary);
  border:1px solid var(--primary);border-radius:var(--radius);
  cursor:pointer;margin-bottom:8px;
  transition:background 0.15s,color 0.15s;
}
.geo-btn:hover{background:var(--primary);color:#0d1117}
.geo-btn:disabled{opacity:0.5;cursor:default}
.geo-status{font-size:0.8125rem;margin-bottom:8px}
.geo-ok{color:var(--success)}
.geo-error{color:var(--text-muted)}
.helper-copy{color:var(--text-muted);font-size:0.8125rem;font-style:italic;margin-top:var(--gap)}

/* Resolution text (Step 4) */
.resolution-text{
  color:var(--primary);font-size:0.9375rem;
  font-weight:500;margin-top:var(--gap);text-align:center;
}

/* Review cards (Step 5) */
.review-sections{display:flex;flex-direction:column;gap:8px}
.review-card{
  background:var(--surface);border:1px solid var(--border);
  border-radius:var(--radius);padding:12px;cursor:pointer;
  transition:border-color 0.15s;
}
.review-card:hover,.review-card:active{border-color:var(--primary)}
.review-card-header{
  display:flex;justify-content:space-between;align-items:center;
  margin-bottom:8px;
}
.review-card-header span:first-child{font-weight:600;font-size:0.9375rem}
.review-edit{color:var(--primary);font-size:0.8125rem}
.review-item{
  display:flex;justify-content:space-between;align-items:baseline;
  padding:2px 0;font-size:0.875rem;
}
.review-label{color:var(--text-muted)}
.review-value{color:var(--text);text-align:right;word-break:break-all;max-width:60%}

/* Dashboard layout */
.dashboard{
  width:100%;max-width:480px;
  display:flex;flex-direction:column;
  padding:var(--gap);
}
.dash-header{text-align:center;padding:var(--gap) 0}
.dash-header h1{font-size:1.25rem;font-weight:600;margin-bottom:4px}
.dash-header .dash-ip{color:var(--text-muted);font-size:0.8125rem}
.dash-header nav{margin-top:8px}
.dash-header nav a{color:var(--primary);font-size:0.875rem;text-decoration:none}
.dash-header nav a:hover{color:var(--primary-hover);text-decoration:underline}
.dash-cards{display:flex;flex-direction:column;gap:var(--gap)}

/* Cards */
.card{
  background:var(--surface);border:1px solid var(--border);
  border-radius:var(--radius);padding:var(--gap);
}
.card h2{font-size:1rem;font-weight:600;margin-bottom:12px}
.card label{display:block;font-size:0.875rem;color:var(--text-muted);margin-bottom:4px;margin-top:12px}
.card label:first-of-type{margin-top:0}
.card input[type="range"]{width:100%;accent-color:var(--primary)}
.card .range-row{display:flex;align-items:center;gap:8px}
.card .range-val{font-size:0.875rem;min-width:32px;text-align:right;color:var(--text)}
.card .rgb-row{display:flex;gap:8px}
.card .rgb-row .rgb-field{flex:1;display:flex;flex-direction:column}
.card .rgb-row input[type="number"]{
  width:100%;padding:8px;font-size:1rem;
  background:var(--surface);color:var(--text);
  border:1px solid var(--border);border-radius:var(--radius);
  outline:none;text-align:center;
  -moz-appearance:textfield;
}
.card .rgb-row input[type="number"]::-webkit-inner-spin-button,
.card .rgb-row input[type="number"]::-webkit-outer-spin-button{-webkit-appearance:none;margin:0}
.card .rgb-row input:focus{border-color:var(--primary)}
.card .rgb-row .rgb-label{font-size:0.75rem;color:var(--text-muted);text-align:center;margin-top:4px}
.color-preview{
  height:24px;border-radius:4px;margin-top:8px;
  border:1px solid var(--border);
}

/* Apply button (dashboard cards) */
.apply-btn{
  display:block;width:100%;padding:12px;font-size:1rem;
  margin-top:var(--gap);
  background:var(--primary);color:#0d1117;
  border:1px solid var(--primary);border-radius:var(--radius);
  cursor:pointer;font-weight:600;
  transition:background 0.15s,border-color 0.15s;
}
.apply-btn:hover{background:var(--primary-hover);border-color:var(--primary-hover)}
.apply-btn:disabled{opacity:0.5;cursor:default}

/* Unified apply bar */
.apply-bar{
  position:sticky;bottom:0;z-index:100;
  background:var(--surface);
  border:1px solid var(--primary);border-radius:var(--radius);
  padding:12px var(--gap);margin-top:var(--gap);
  display:flex;justify-content:space-between;align-items:center;
  box-shadow:0 -4px 12px rgba(0,0,0,0.4);
}
.apply-bar-text{font-size:0.875rem;color:var(--primary);font-weight:500}
.apply-bar-btn{
  padding:10px 24px;font-size:1rem;
  background:var(--primary);color:#0d1117;
  border:1px solid var(--primary);border-radius:var(--radius);
  cursor:pointer;font-weight:600;
  transition:background 0.15s,border-color 0.15s;
  white-space:nowrap;
}
.apply-bar-btn:hover{background:var(--primary-hover);border-color:var(--primary-hover)}
.apply-bar-btn:disabled{opacity:0.5;cursor:default}

/* Card form fields (dashboard) */
.card .form-fields{display:flex;flex-direction:column;gap:4px}
.card .form-fields label{font-size:0.875rem;color:var(--text-muted);margin-top:8px}
.card .form-fields label:first-child{margin-top:0}
.card .form-fields input[type="text"],
.card .form-fields input[type="password"]{
  width:100%;padding:10px;font-size:1rem;
  background:var(--surface);color:var(--text);
  border:1px solid var(--border);border-radius:var(--radius);
  outline:none;transition:border-color 0.15s;
}
.card .form-fields input:focus{border-color:var(--primary)}

/* Scan inline (dashboard WiFi) */
.scan-row-inline{margin-bottom:4px}

/* Estimate line */
.estimate-line{font-size:0.875rem;color:var(--text-muted);margin-top:4px}
.estimate-warning{color:#d29922;font-weight:600}
.estimate-note{font-size:0.75rem;color:var(--text-muted);font-style:italic;margin-top:2px}

/* Toast system */
#toast-container{
  position:fixed;top:16px;left:50%;transform:translateX(-50%);
  z-index:9999;display:flex;flex-direction:column;gap:8px;
  width:calc(100% - 32px);max-width:400px;pointer-events:none;
}
.toast{
  padding:12px 16px;border-radius:var(--radius);
  font-size:0.875rem;font-weight:500;
  transform:translateY(-20px);opacity:0;
  transition:transform 0.25s ease,opacity 0.25s ease;
  pointer-events:auto;
}
.toast-visible{transform:translateY(0);opacity:1}
.toast-exit{transform:translateY(-20px);opacity:0}
.toast-success{background:var(--success);color:#0d1117}
.toast-warning{background:#d29922;color:#0d1117}
.toast-error{background:var(--error);color:#fff}

@media (prefers-reduced-motion:reduce){
  .toast{transition:none;transform:none}
  .toast-visible{opacity:1}
  .toast-exit{opacity:0;transition:opacity 0.15s ease}
}

/* Handoff screen */
.handoff{
  display:flex;flex-direction:column;align-items:center;justify-content:center;
  text-align:center;min-height:80vh;padding:var(--gap);gap:var(--gap);
}
.handoff h1{font-size:1.5rem;color:var(--success)}
.handoff p{font-size:1rem;color:var(--text);max-width:320px}
.handoff-note{color:var(--text-muted)!important;font-size:0.875rem!important}

/* Health page (Story 2.4) */
.health-loading{color:var(--text-muted);font-size:0.875rem}
.status-row{
  display:flex;justify-content:space-between;align-items:baseline;
  padding:6px 0;border-bottom:1px solid var(--border);
}
.status-row:last-child{border-bottom:none}
.status-label{font-size:0.875rem;color:var(--text-muted)}
.status-value{font-size:0.875rem;color:var(--text);text-align:right}
.status-dot{
  display:inline-block;width:8px;height:8px;border-radius:50%;
  margin-right:6px;vertical-align:middle;
}
.card.over-pace{border-color:#d29922}
.card.over-pace h2{color:#d29922}

/* Layout preview (Story 3.4) */
.preview-container{margin-top:var(--gap)}
.preview-container canvas{
  display:block;width:100%;
  border:1px solid var(--border);border-radius:var(--radius);
  background:var(--bg);
}
.preview-legend{
  display:flex;align-items:center;gap:12px;
  margin-top:8px;font-size:0.75rem;color:var(--text-muted);flex-wrap:wrap;
}
.legend-chip{
  display:inline-block;width:12px;height:12px;border-radius:2px;
  margin-right:4px;vertical-align:middle;
}

.wiring-label{
  font-size:0.8125rem;font-weight:600;color:var(--text-muted);
  margin:var(--gap) 0 6px;
}

/* Location card (Story 4.1) */
.location-toggle{cursor:pointer;user-select:none}
.location-toggle::after{
  content:' \25B6';font-size:0.75em;color:var(--text-muted);
  transition:transform 0.2s;display:inline-block;margin-left:6px;
}
#location-card .location-toggle.open::after{
  transform:rotate(90deg);
}
#location-map{
  height:260px;border-radius:var(--radius);
  border:1px solid var(--border);background:var(--bg);
}
.location-radius-handle{
  width:16px;height:16px;border-radius:50%;
  background:var(--primary);border:2px solid #fff;
  box-shadow:0 0 0 2px rgba(13,17,23,0.85);
}
.map-loading{
  color:var(--text-muted);font-size:0.875rem;
  text-align:center;padding:var(--gap) 0;
}
#location-fallback input[type="number"]{
  width:100%;padding:10px;font-size:1rem;
  background:var(--surface);color:var(--text);
  border:1px solid var(--border);border-radius:var(--radius);
  outline:none;transition:border-color 0.15s;
  -moz-appearance:textfield;
}
#location-fallback input[type="number"]::-webkit-inner-spin-button,
#location-fallback input[type="number"]::-webkit-outer-spin-button{-webkit-appearance:none;margin:0}
#location-fallback input:focus{border-color:var(--primary)}

/* Leaflet dark-theme overrides */
.leaflet-container{background:var(--bg)!important}
.leaflet-control-zoom a{
  background:var(--surface)!important;color:var(--text)!important;
  border-color:var(--border)!important;
}
.leaflet-control-attribution{
  background:rgba(22,27,34,0.8)!important;color:var(--text-muted)!important;
  font-size:0.625rem!important;
}
.leaflet-control-attribution a{color:var(--primary)!important}

/* Calibration card (Story 4.2) */
.calibration-toggle{cursor:pointer;user-select:none}
.calibration-toggle::after{
  content:' \25B6';font-size:0.75em;color:var(--text-muted);
  transition:transform 0.2s;display:inline-block;margin-left:6px;
}
#calibration-card .calibration-toggle.open::after{
  transform:rotate(90deg);
}
.cal-select{
  width:100%;padding:10px;font-size:1rem;
  background:var(--surface);color:var(--text);
  border:1px solid var(--border);border-radius:var(--radius);
  outline:none;transition:border-color 0.15s;
  -webkit-appearance:none;appearance:none;
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='8'%3E%3Cpath d='M1 1l5 5 5-5' stroke='%238b949e' stroke-width='1.5' fill='none'/%3E%3C/svg%3E");
  background-repeat:no-repeat;background-position:right 10px center;
  padding-right:30px;
}
.cal-select:focus{border-color:var(--primary)}
.cal-preview-container{margin-top:var(--gap)}
.cal-preview-container canvas{
  display:block;width:100%;
  border:1px solid var(--border);border-radius:var(--radius);
  background:var(--bg);
}

/* Calibration pattern toggle */
.cal-pattern-toggle{display:flex;gap:0;margin-bottom:var(--gap)}
.cal-pattern-btn{
  flex:1;padding:6px 0;border:1px solid var(--border);
  background:var(--bg);color:var(--text-muted);cursor:pointer;
  font-size:0.8125rem;
}
.cal-pattern-btn:first-child{border-radius:var(--radius) 0 0 var(--radius)}
.cal-pattern-btn:last-child{border-radius:0 var(--radius) var(--radius) 0}
.cal-pattern-btn.active{background:var(--primary);color:#fff;border-color:var(--primary)}

/* Logo upload (Story 4.3) */
.logo-upload-zone{
  border:2px dashed var(--border);border-radius:var(--radius);
  padding:var(--gap);text-align:center;
  transition:border-color 0.2s,background 0.2s;
}
.logo-upload-zone.drag-over{
  border-color:var(--primary);background:rgba(88,166,255,0.08);
}
.upload-prompt{font-size:0.875rem;color:var(--text-muted);margin-bottom:8px}
.upload-prompt code{color:var(--primary);font-size:0.8125rem}
.upload-label{
  display:inline-block;padding:8px 16px;font-size:0.875rem;
  background:var(--surface);color:var(--primary);
  border:1px solid var(--primary);border-radius:var(--radius);
  cursor:pointer;transition:background 0.15s;
}
.upload-label:hover{background:var(--primary);color:#0d1117}
.upload-hint{font-size:0.75rem;color:var(--text-muted);margin-top:8px}
.logo-file-list{display:flex;flex-direction:column;gap:6px;margin-top:var(--gap)}
.logo-file-row{
  display:flex;align-items:center;gap:8px;
  padding:8px;background:var(--surface);
  border:1px solid var(--border);border-radius:var(--radius);
  font-size:0.875rem;
}
.logo-file-row.file-error{border-color:var(--error)}
.logo-file-row.file-ok{border-color:var(--success)}
.logo-file-preview{
  width:128px;height:128px;border-radius:4px;
  background:var(--bg);flex-shrink:0;
  image-rendering:pixelated;
}
.logo-file-info{flex:1;min-width:0}
.logo-file-name{
  display:block;font-weight:500;
  overflow:hidden;text-overflow:ellipsis;white-space:nowrap;
}
.logo-file-status{display:block;font-size:0.75rem;color:var(--text-muted);margin-top:2px}
.logo-file-status.status-error{color:var(--error)}
.logo-file-status.status-ok{color:var(--success)}
.logo-file-remove{
  background:none;border:none;color:var(--text-muted);
  font-size:1.125rem;cursor:pointer;padding:4px;line-height:1;
}
.logo-file-remove:hover{color:var(--error)}

/* Logo list management (Story 4.4) */
.logo-storage-summary{
  font-size:0.8125rem;color:var(--text-muted);margin-top:var(--gap);
  text-align:center;
}
.logo-list-container{margin-top:var(--gap)}
.logo-empty-state{
  font-size:0.875rem;color:var(--text-muted);font-style:italic;
  text-align:center;padding:var(--gap) 0;
}
.logo-list{display:flex;flex-direction:column;gap:6px}
.logo-list-row{
  display:flex;align-items:center;gap:8px;
  padding:8px;background:var(--surface);
  border:1px solid var(--border);border-radius:var(--radius);
  font-size:0.875rem;
}
.logo-list-thumb{
  width:48px;height:48px;border-radius:4px;
  background:var(--bg);flex-shrink:0;
  image-rendering:pixelated;
}
.logo-list-info{flex:1;min-width:0}
.logo-list-name{
  display:block;font-weight:500;
  overflow:hidden;text-overflow:ellipsis;white-space:nowrap;
}
.logo-list-size{display:block;font-size:0.75rem;color:var(--text-muted);margin-top:2px}
.logo-list-actions{flex-shrink:0;display:flex;align-items:center;gap:6px}
.logo-list-delete{
  background:none;border:1px solid var(--error);color:var(--error);
  font-size:0.75rem;padding:4px 8px;border-radius:var(--radius);
  cursor:pointer;transition:background 0.15s,color 0.15s;
  white-space:nowrap;
}
.logo-list-delete:hover{background:var(--error);color:#fff}
.logo-list-delete:disabled{opacity:0.5;cursor:default}
.logo-list-confirm-text{
  font-size:0.75rem;color:var(--error);white-space:nowrap;
}
.logo-list-confirm-btn{
  background:var(--error);border:1px solid var(--error);color:#fff;
  font-size:0.75rem;padding:4px 8px;border-radius:var(--radius);
  cursor:pointer;font-weight:600;white-space:nowrap;
}
.logo-list-confirm-btn:disabled{opacity:0.5;cursor:default}
.logo-list-cancel-btn{
  background:none;border:1px solid var(--border);color:var(--text);
  font-size:0.75rem;padding:4px 8px;border-radius:var(--radius);
  cursor:pointer;white-space:nowrap;
}
.logo-list-cancel-btn:hover{border-color:var(--text-muted)}

/* Mode Picker (Story dl-1.5) */
.mode-status-line{
  font-size:0.9rem;padding:8px 0;margin-bottom:12px;
  border-bottom:1px solid var(--border);
  color:var(--text-muted);
}
.mode-status-name{font-weight:600;color:var(--text)}
.mode-status-reason{font-weight:400}
.mode-cards-list{display:flex;flex-direction:column;gap:8px}
.mode-card{
  padding:12px;background:var(--bg);
  border:1px solid var(--border);border-radius:var(--radius);
  cursor:pointer;transition:border-color 0.15s;
}
.mode-card:hover{border-color:var(--primary)}
.mode-card.active{border-color:var(--primary);background:rgba(88,166,255,0.08)}
.mode-card-name{font-size:0.9375rem;font-weight:500}
.mode-card-subtitle{font-size:0.75rem;color:var(--text-muted);margin-top:2px}
.mode-card.switching{opacity:0.6;pointer-events:none}

/* Danger zone / System card (Story 2.5) */
.card-danger{border-color:var(--error)}
.card-danger h2{color:var(--error)}
.btn-danger{
  display:block;width:100%;padding:12px;font-size:1rem;
  background:var(--error);color:#fff;
  border:1px solid var(--error);border-radius:var(--radius);
  cursor:pointer;font-weight:600;
  transition:background 0.15s,border-color 0.15s;
}
.btn-danger:hover{background:#e5443c;border-color:#e5443c}
.btn-danger:disabled{opacity:0.5;cursor:default}
.btn-cancel{
  display:block;width:100%;padding:12px;font-size:1rem;
  background:var(--surface);color:var(--text);
  border:1px solid var(--border);border-radius:var(--radius);
  cursor:pointer;font-weight:500;
  transition:border-color 0.15s;
}
.btn-cancel:hover{border-color:var(--text-muted)}
.btn-cancel:disabled{opacity:0.5;cursor:default}
.reset-row{margin-top:4px}
.reset-warning{
  font-size:0.875rem;color:var(--error);margin-bottom:12px;
}
.reset-actions{display:flex;gap:8px}
.reset-actions .btn-danger,.reset-actions .btn-cancel{flex:1}

/* Firmware card / OTA Upload (Story fn-1.6) */
.fw-version-text{font-size:0.9375rem;color:var(--text);margin-bottom:12px}
.ota-upload-zone{
  border:2px dashed var(--border);border-radius:var(--radius);
  padding:var(--gap);min-height:80px;cursor:pointer;
  text-align:center;display:flex;flex-direction:column;
  align-items:center;justify-content:center;
  transition:border-color 0.2s,background 0.2s;
}
.ota-upload-zone.drag-over{border-color:var(--primary);border-style:solid;background:rgba(88,166,255,0.08)}
.ota-file-info{display:flex;align-items:center;gap:8px;margin-top:var(--gap);flex-wrap:wrap}
.ota-file-info span{font-size:0.875rem;word-break:break-all}
.ota-progress{
  position:relative;height:28px;border-radius:var(--radius);
  background:var(--border);margin-top:var(--gap);overflow:hidden;
}
.ota-progress-bar{height:100%;background:var(--primary);transition:width 0.3s;border-radius:var(--radius)}
.ota-progress-text{position:absolute;top:0;left:0;right:0;bottom:0;display:flex;align-items:center;justify-content:center;color:#fff;font-size:0.875rem;font-weight:600;text-shadow:0 1px 2px rgba(0,0,0,0.65),0 -1px 2px rgba(0,0,0,0.65),1px 0 2px rgba(0,0,0,0.65),-1px 0 2px rgba(0,0,0,0.65)}
.ota-reboot{text-align:center;padding:var(--gap);font-size:1.125rem;color:var(--primary)}
.banner-warning{
  background:#2d2738;border-left:4px solid #d29922;
  padding:12px 16px;border-radius:var(--radius);
  display:flex;justify-content:space-between;align-items:center;
  margin-bottom:12px;font-size:0.875rem;
}
.banner-warning .dismiss-btn{
  background:none;border:none;color:var(--text);cursor:pointer;
  font-size:1.25rem;min-width:44px;min-height:44px;
  display:flex;align-items:center;justify-content:center;
  border-radius:var(--radius);
}
.banner-warning .dismiss-btn:focus-visible{outline:2px solid var(--primary);outline-offset:2px}
.ota-file-info .apply-btn{min-height:44px;margin-top:0;width:auto}
.btn-ota-cancel{width:auto;padding:10px 16px;margin-top:0;min-height:44px}
.btn-secondary{
  display:block;width:100%;padding:12px;font-size:1rem;min-height:44px;
  background:transparent;color:var(--text);
  border:1px solid var(--border);border-radius:var(--radius);
  cursor:pointer;font-weight:500;
  transition:border-color 0.15s,background 0.15s;
}
.btn-secondary:hover{border-color:var(--text-muted);background:rgba(255,255,255,0.03)}


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

## Story fn-1-4 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Rollback flag persistence API behavior was left as an open "consider whether" question rather than a definitive spec | Updated Critical Gotcha #2 to establish that `rollback_detected` remains `true` on every boot until a new successful OTA clears the invalid partition slot. This is declared intentional API behavior, and fn-1.6 is noted as the consumer that should display the persistent state. |
| medium | Log message time unit in AC1 shows `"8s"` (seconds) but Task 2 and the implementation pattern use `%lums` (milliseconds) — two different outputs for the same event | Updated AC1 log message example from `"Marked valid, WiFi connected in 8s"` to `"Firmware marked valid — WiFi connected in 8432ms"` to match the implementation pattern. Added explicit parenthetical `(X = elapsed seconds)` to the SystemStatus message format to clarify the unit distinction between the developer log and the user-facing status. |
| low | AC6 "no WARNING or ERROR status is set" was ambiguous about whether a StatusLevel::OK call is also omitted for normal boots | Updated AC6 to explicitly state "no `SystemStatus::set` call is made for the OTA subsystem" — eliminating the gap between "no WARNING or ERROR" and the implementation which makes no OTA status call at all on normal boots. |
| low | FW_VERSION note in the SystemStatus extension section was self-contradictory — stated the macro is "already available project-wide" then immediately suggested a local conditional define | Rewrote the note to explain the fallback guard is for non-PlatformIO build contexts (e.g., unit test harnesses), resolving the apparent contradiction. |
| dismissed | `"firmware_version": "1.0.0"` in AC5 hardcodes a specific version, which is misleading | FALSE POSITIVE: The value is immediately qualified with `(from FW_VERSION build flag)` — this is a concrete example in an AC, not a hardcoded literal. This is standard practice for illustrative acceptance criteria. No change warranted. |
| dismissed | `String(...).c_str()` usage in `performOtaSelfCheck()` should be replaced with direct String construction | FALSE POSITIVE: The pattern is valid, idiomatic Arduino/ESP32 embedded C++. `SystemStatus::set` accepting `const char*` or `const String&` is implementation-dependent, and `.c_str()` is universally compatible. The validator's alternative produces the same compiled output. The story's implementation pattern should not be changed for a style preference that has no correctness implication. --- |

## Story fn-1-6 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | Task 5 listed `localStorage` as an "Alternative simpler approach" for rollback acknowledgment persistence, directly contradicting AC #9's explicit "NVS-backed" requirement. A developer following the alternative path would violate the acceptance criteria. | Removed the localStorage alternative entirely from Task 5; replaced with a clean implementation note that frames direct `Preferences` access as the chosen pattern (not ConfigManager, not localStorage), and clarifies why. |
| medium | `ota_rb_ack` reset timing was inconsistent across the story — Task 5 said "when upload **succeeds**" but the Dev Notes code snippet said "at the `index == 0` **first-chunk block**" (upload start). If cleared at start: a failed upload would have already erased the ack, so a rollback banner that was previously dismissed would never reappear if the next attempt also fails. The correct behavior is clear-on-success only. | Harmonized Task 5, Dev Notes "Reset ota_rb_ack" section, Project Structure Notes, and Risk Mitigation #4 — all now consistently say "after `Update.end(true)` succeeds, before `ESP.restart()`". Added explicit warning: "Do NOT clear at upload start." |
| dismissed | Direct `Preferences` access for `ota_rb_ack` is a Critical architectural violation that must be routed through `ConfigManager`. | FALSE POSITIVE: The project context explicitly says "Prefer existing patterns in firmware/ over new abstractions." The AC says "NVS-backed" — it doesn't say "ConfigManager-backed." Adding a new ConfigManager struct for a simple transient flag would require modifying `ConfigManager.h`, `.cpp`, and structs across the board, which IS overkill. The story correctly justifies this deviation in Dev Notes. Direct `Preferences` access is standard ESP32 Arduino practice. |
| dismissed | Rollback ack reset logic is a Critical gap — ACs don't state that a new successful OTA resets acknowledgment. | FALSE POSITIVE: Task 5 and Dev Notes both document this behavior. The real problem was the *timing inconsistency* between Task 5 ("succeeds") and the Dev Notes code comment ("upload starts"), which IS addressed above — but it was a Medium inconsistency, not a Critical gap. |
| dismissed | "1.5MB" should be "1.5 MiB (1,572,864 bytes)" for technical precision. | FALSE POSITIVE: The entire fn-1 epic consistently uses "1.5MB" (from fn-1.1 partition work). Changing just this story's error message text would create inconsistency. The validation code already uses the exact byte count (`1572864`). User-facing error messages correctly mirror how OS file managers display sizes. |
| dismissed | AC #7 polling termination criteria are incomplete — must specify version-change detection. | FALSE POSITIVE: Since the server validates the OTA before responding `{ ok: true }` and rebooting, any successful `/api/status` poll after the reboot sequence proves the update installed. Requiring a version-change check would actually be incorrect in edge cases (e.g., reflashing the same version). Task 7 already specifies both stop conditions. |
| dismissed | CSS verification methods and FW_VERSION macro source are not explicitly stated. | FALSE POSITIVE: Testing Strategy already includes `ls -la` gzip verification. FW_VERSION source is documented in "Previous Story Intelligence" (from fn-1.1 and fn-1.4). These are already covered. --- |


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="Architecture (fallback - epics file should have relevant sections)" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="default_output_file">_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md</var>
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
<var name="story_id">fn-1.7</var>
<var name="story_key">fn-1-7-settings-import-in-setup-wizard</var>
<var name="story_num">7</var>
<var name="story_title">settings-import-in-setup-wizard</var>
<var name="timestamp">20260412_2115</var>
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
  <critical>📡 Git Intelligence is EMBEDDED in &lt;git-intelligence&gt; at the start of the prompt - do NOT run git commands yourself, use the embedded data instead</critical>

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

    <action>Create file using the output-template format at: /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md</action>
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