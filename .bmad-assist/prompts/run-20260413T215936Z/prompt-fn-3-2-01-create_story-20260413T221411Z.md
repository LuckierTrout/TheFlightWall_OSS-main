<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-3 -->
<!-- Story: fn-3.2 -->
<!-- Phase: create-story -->
<!-- Timestamp: 20260413T221411Z -->
<compiled-workflow>
<mission><![CDATA[

Create the next user story from epics+stories with enhanced context analysis and direct ready-for-dev marking

Target: Story fn-3.2 - wizard-timezone-auto-detect
Create comprehensive developer context and implementation-ready story.

]]></mission>
<context>
<file id="git-intel" path="[git-intelligence]"><![CDATA[<git-intelligence>
Git intelligence extracted at compile time. Do NOT run additional git commands - use this embedded data instead.

### Recent Commits (last 5)
```
e206a92 Update bmad-assist.yaml to skip story and epic prompts during execution. Adjust budget estimates for story creation, validation, development, and code review to accommodate increased complexity. Modify resource limits and context budgets to enhance performance. Update state.yaml to reflect the current epic and story status, and remove completed stories. Delete outdated prompt files to streamline the project structure.
3c703d8 new commit
898c09f Enhance AGENTS.md with updated BMAD workflow details, including project context handling and bmad-assist commands. Modify bmad-assist.yaml to adjust budget estimates for story creation and validation. Update state.yaml to reflect current epic and story status. Revise sprint-status.yaml to streamline development status and remove outdated comments. Update epics.md to reflect completed and new epics, and adjust firmware partition layout in custom_partitions.csv. Log firmware version at boot in main.cpp.
ffc1cce Update .gitignore to include .env file, enhance AGENTS.md with new user preferences and workspace facts, and modify continual learning state files. Adjust workflow configurations to specify story directories and improve error handling in API fetchers. Add positioning mode to NeoMatrixDisplay and update related documentation.
c5f944e Update AGENTS.md with information on bundled web sources for firmware portal. Modify continual learning state files to reflect the latest run details and add new transcript entries.
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

---
stepsCompleted: [1, 2, 3, 4, 5, 6, 7, 8]
lastStep: 8
status: 'complete'
completedAt: '2026-04-02'
extensionStarted: '2026-04-11'
extensionPrd: 'prd-foundation.md'
extensionStarted2: '2026-04-11'
extensionPrd2: 'prd-display-system.md'
extensionCompleted2: '2026-04-11'
extensionStatus2: 'complete'
extensionStarted3: '2026-04-11'
extensionPrd3: 'prd-delight.md'
extensionCompleted3: '2026-04-11'
extensionStatus3: 'complete'
inputDocuments: ['_bmad-output/planning-artifacts/prd.md', '_bmad-output/planning-artifacts/ux-design-specification.md', '_bmad-output/planning-artifacts/prd-validation-report.md', '_bmad-output/planning-artifacts/prd-foundation.md', '_bmad-output/planning-artifacts/prd-foundation-validation-report.md', '_bmad-output/planning-artifacts/prd-display-system.md', '_bmad-output/planning-artifacts/prd-display-system-validation-report.md', '_bmad-output/planning-artifacts/prd-delight.md', '_bmad-output/planning-artifacts/prd-delight-validation-report.md', 'docs/index.md', 'docs/project-overview.md', 'docs/architecture.md', 'docs/source-tree-analysis.md', 'docs/component-inventory.md', 'docs/development-guide.md', 'docs/api-contracts.md']
workflowType: 'architecture'
project_name: 'TheFlightWall_OSS-main'
user_name: 'Christian'
date: '2026-04-11'
---

# Architecture Decision Document

_This document builds collaboratively through step-by-step discovery. Sections are appended as we work through each architectural decision together._

## Project Context Analysis

### Requirements Overview

**Functional Requirements (48 FRs across 9 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| Device Setup & Onboarding | FR1-FR8 | New: captive portal, AP mode, wizard flow, WiFi transition |
| Configuration Management | FR9-FR14 | New: ConfigManager, NVS persistence, hot-reload + reboot paths |
| Display Calibration & Preview | FR15-FR18 | New: web-to-firmware communication for test patterns, canvas data endpoint |
| Flight Data Display | FR19-FR25 | Existing: fetch → enrich → display pipeline (must not regress) |
| Airline Logo Display | FR26-FR29 | New: LogoManager, LittleFS file I/O, ICAO lookup, fallback sprite |
| Responsive Display Layout | FR30-FR33 | New: LayoutEngine with zone calculation and breakpoint detection |
| Logo Management | FR34-FR37 | New: file upload endpoint, validation, LittleFS management |
| System Operations | FR38-FR48 | New: FreeRTOS tasks, mDNS, GPIO handlers, API usage tracking |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| Hot-reload latency | <1 second | ConfigManager must notify display task directly, no polling |
| Page load time | <3 seconds | Gzipped assets, minimal JS, no external dependencies except Leaflet |
| Time to first flight | <60 seconds | WiFi connect + OAuth + first fetch must be pipelined efficiently |
| Uptime | 30+ days | Heap fragmentation management, watchdog timers, graceful error recovery |
| WiFi recovery | <60 seconds | Auto-reconnect with configurable timeout → AP fallback |
| RAM ceiling | <280KB | Stream LittleFS files, limit concurrent web clients to 2-3, ArduinoJson filter API |
| Flash budget | 4MB total, 500KB headroom | 2MB app + 2MB LittleFS partition. Gzip all web assets. Monitor usage. |
| Concurrent operations | Web server + flight pipeline + display | FreeRTOS task pinning: display Core 0, WiFi/web/API Core 1 |

**UX Specification (architectural implications):**

| UX Decision | Architecture Requirement |
|-------------|------------------------|
| Two HTML pages (wizard + dashboard) | ESPAsyncWebServer serves different content based on WiFi mode |
| Single settings API endpoint | `POST /api/settings` handler parses JSON, applies to ConfigManager, returns applied keys + reboot flag |
| Gzipped static assets | LittleFS stores `.gz` files, ESPAsyncWebServer serves with `Content-Encoding: gzip` |
| Leaflet lazy-load | Dashboard JS dynamically loads Leaflet only when Location section opened |
| mDNS | `ESPmDNS.begin("flightwall")` on WiFi connect |
| WiFi scan in wizard | `WiFi.scanNetworks(true)` async in AP_STA mode, results via `/api/wifi/scan` |
| Toast + hot-reload | Settings POST response drives client-side toast; display settings bypass reboot |
| Triple feedback | Browser canvas (client-side) + LED wall (via firmware) update from same settings change |
| RGB565 logo preview | Client-side only — no architecture impact (FileReader API in browser) |

### Scale & Complexity

- **Primary domain:** IoT/Embedded with companion web UI
- **Complexity level:** Medium — multiple subsystems (web server, config manager, display engine, logo manager, WiFi manager) but single-user, no auth, no database, no cloud backend
- **Estimated new architectural components:** 6 (ConfigManager, WebPortal, WiFiManager, LogoManager, LayoutEngine, DisplayTask)
- **Existing components to preserve:** 5 (FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, NeoMatrixDisplay)

### Technical Constraints & Dependencies

| Constraint | Impact |
|-----------|--------|
| ESP32 single WiFi radio | AP and STA share hardware — AP_STA for scan, mode switching for setup→operation transition |
| 4MB flash total | Partition carefully: 2MB app + 2MB LittleFS. No OTA in MVP. Every KB of web assets matters. |
| 520KB SRAM | ~280KB usable after stack/WiFi. ESPAsyncWebServer + TLS + JSON parsing compete for heap. |
| FastLED disables interrupts | Display task MUST be on Core 0, WiFi/web on Core 1. Non-negotiable. |
| ESPAsyncWebServer connection limit | 2-3 concurrent clients max. No auto-refreshing dashboard. No WebSocket push. |
| ArduinoJson heap pressure | Use streaming/filter API to reduce per-parse allocation from ~16KB to ~2-4KB |
| NVS write endurance | ~100,000 write cycles per key. Not a concern for config changes, but don't write on every slider drag — debounce. |

### Cross-Cutting Concerns

1. **Configuration lifecycle** — Compile-time defaults → NVS read on boot → runtime changes via web UI → NVS write on save → hot-reload or reboot. Every component that reads config must support this chain.

2. **WiFi mode management** — AP mode (setup) → STA mode (operation) → AP fallback (WiFi failure). Web server behavior changes per mode (wizard vs. dashboard). mDNS only in STA mode.

3. **Concurrent task safety** — Display task (Core 0) and web server + flight pipeline (Core 1) share data structures (current flights, config values). Needs mutex or task-safe data passing.

4. **Flash budget tracking** — Firmware size + web assets + logos must fit in 4MB with headroom. Logo uploads could fill LittleFS. Need usage tracking and upload rejection when full.

5. **Error propagation** — API failures, WiFi drops, and NVS errors must propagate to both the LED display (status messages) and the web UI (System Health page) with specific error information.

## Technology Stack & Foundation

### Existing Stack (Locked)

| Layer | Technology | Version | Notes |
|-------|-----------|---------|-------|
| Language | C++ (Arduino) | C++11 | ESP32 Arduino framework dialect |
| Platform | ESP32 | espressif32 | PlatformIO managed |
| Build | PlatformIO | Latest | `platformio.ini` configuration |
| LED Control | FastLED | ^3.6.0 | WS2812B driver |
| Graphics | Adafruit GFX | ^1.11.9 | Text/shape rendering |
| Matrix | FastLED NeoMatrix | ^1.2 | Tiled matrix abstraction |
| JSON | ArduinoJson | ^7.4.2 | API response parsing |
| Networking | WiFi + HTTPClient | ESP32 core | Built-in Arduino ESP32 |
| TLS | WiFiClientSecure | ESP32 core | HTTPS for API calls |

### New Dependencies (To Add)

| Library | Source | Version | Notes |
|---------|--------|---------|-------|
| ESPAsyncWebServer | `mathieucarbou/ESPAsyncWebServer` | ^3.6.0 | **Use Carbou fork** — actively maintained, fixes memory leaks in original `me-no-dev` repo. Used by ESPHome. |
| AsyncTCP | `mathieucarbou/AsyncTCP` | Latest | **Use Carbou fork** — fixes race conditions that cause crashes under load. Required by ESPAsyncWebServer. |
| LittleFS | ESP32 core | Built-in | Replaces SPIFFS (better wear leveling, directory support). Requires `board_build.filesystem = littlefs` in platformio.ini. |
| ESPmDNS | ESP32 core | Built-in | `flightwall.local` device discovery. Zero-config networking. |
| Leaflet.js | Client-side | ~1.9 | Interactive map. ~25KB gzipped. Lazy-loaded. Stored in `firmware/data/leaflet/` or loaded from CDN. |

**platformio.ini additions:**
```ini
lib_deps =
    mathieucarbou/ESPAsyncWebServer @ ^3.6.0
    ; AsyncTCP pulled automatically as dependency
    fastled/FastLED @ ^3.6.0
    adafruit/Adafruit GFX Library @ ^1.11.9
    marcmerlin/FastLED NeoMatrix @ ^1.2
    bblanchon/ArduinoJson @ ^7.4.2

board_build.filesystem = littlefs
board_build.partitions = custom_partitions.csv
```

### Architecture Pattern

**Extending existing Hexagonal (Ports & Adapters):**
- New components follow the same pattern — abstract interfaces where swappability matters, concrete implementations where it doesn't
- ConfigManager is a new "core" component (not an adapter — it's internal)
- WebPortal is a new adapter (it's an interface to the outside world — the browser)
- WiFiManager is a new adapter (manages the WiFi hardware port)
- LogoManager is a new core component (internal file management)
- LayoutEngine is a new core component (internal display logic)

### Flash Partition Layout

**Custom partition table required** — the default ESP32 partition doesn't provide the 2MB/2MB split. Create `firmware/custom_partitions.csv`:

```
# Name,    Type,  SubType, Offset,   Size
nvs,       data,  nvs,     0x9000,   0x5000
otadata,   data,  ota,     0xe000,   0x2000
app0,      app,   ota_0,   0x10000,  0x200000
spiffs,    data,  spiffs,  0x210000, 0x1F0000
```

**MVP Partition Budget:**
```
4MB Flash Total
├── 2MB — Application (firmware)
│   └── ~1.2MB estimated firmware + ~800KB headroom
└── ~1.9MB — LittleFS
    ├── ~40KB — Web assets (gzipped HTML/JS/CSS)
    ├── ~28KB — Leaflet (gzipped JS + CSS)
    ├── ~100KB — Realistic logo usage (~50 logos at 2KB each)
    └── ~1.7MB — Unused headroom
```

**Phase 2 OTA Partition Strategy (architecture note):**

LittleFS is heavily over-provisioned for MVP. Realistic usage is ~170KB of ~1.9MB available. This slack is the OTA budget for Phase 2:

| Layout | App | OTA | LittleFS | Requires Reflash |
|--------|-----|-----|----------|-----------------|
| **MVP** | 2MB | — | 1.9MB | No (initial) |
| **Phase 2 (generous)** | 1.5MB | 1.5MB | 896KB | Yes |
| **Phase 2 (tight)** | 1.5MB | 1.5MB | 512KB | Yes |

Even the "tight" Phase 2 layout provides 512KB LittleFS — enough for web assets (68KB) + 200 logos (400KB) with headroom. Document this tradeoff now so Phase 2 OTA implementation isn't a surprise.

### Web Assets Location

Web assets live at **`firmware/data/`** (not project root `data/`). This matches the non-standard project structure where all source is under `firmware/`.

```
firmware/
├── data/                    ← LittleFS content, uploaded via `pio run -t uploadfs`
│   ├── style.css.gz
│   ├── common.js.gz
│   ├── wizard.html.gz
│   ├── wizard.js.gz
│   ├── dashboard.html.gz
│   ├── dashboard.js.gz
│   └── leaflet/
│       ├── leaflet.min.js.gz
│       └── leaflet.min.css.gz
├── custom_partitions.csv    ← Custom partition table
├── platformio.ini
├── src/
├── core/
├── adapters/
└── ...
```

**Upload command:** `cd firmware && pio run -t uploadfs`

### Development Workflow

**No change from existing:** PlatformIO build → USB flash → serial monitor. Web assets are pre-gzipped externally (using `gzip -9`) and placed in `firmware/data/`, uploaded to LittleFS via `pio run -t uploadfs`.

**First implementation story** should be: add ESPAsyncWebServer + LittleFS to `platformio.ini`, create custom partition table, verify firmware still builds and runs with the new dependencies, and serve a minimal "hello world" HTML page from LittleFS.

## Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. ConfigManager design — Singleton with category struct getters
2. Inter-task communication — Hybrid (atomic flags for config, queue for flight data)
3. WiFi state machine — Dedicated WiFiManager with callbacks
4. API endpoint design — 11 REST endpoints, consistent JSON response format
5. Component integration — Initialization order, dependency graph, read-only display task
6. Error handling — Centralized SystemStatus registry

**Deferred Decisions (Post-MVP):**
- OTA update mechanism (Phase 2 — requires repartitioning)
- WebSocket push for live dashboard updates (not needed — no auto-refresh)
- Bluetooth functionality (no use case defined)

### Decision 1: ConfigManager — Singleton with Category Struct Getters

**Pattern:** Centralized singleton, initialized first, accessed by all components. Config values grouped into category structs for clean API surface and efficient access.

```cpp
struct DisplayConfig {
    uint8_t brightness;
    uint8_t textColorR, textColorG, textColorB;
};

struct LocationConfig {
    double centerLat, centerLon, radiusKm;
};

struct HardwareConfig {
    uint8_t tilesX, tilesY, tilePixels, displayPin;
    uint8_t originCorner, scanDirection, zigzag;
};

struct TimingConfig {
    uint16_t fetchIntervalSeconds, displayCycleSeconds;
};

struct NetworkConfig {
    String wifiSsid, wifiPassword;
    String openSkyClientId, openSkyClientSecret;
    String aeroApiKey;
};

class ConfigManager {
public:
    static void init();
    static DisplayConfig getDisplay();
    static LocationConfig getLocation();
    static HardwareConfig getHardware();
    static TimingConfig getTiming();
    static NetworkConfig getNetwork();
    static ApplyResult applyJson(const JsonObject& settings);
    static void schedulePersist(uint16_t delayMs = 2000);
    static void onChange(std::function<void()> callback);
    static bool requiresReboot(const char* key);
};

struct ApplyResult {
    std::vector<String> applied;
    bool rebootRequired;
};
```

**Config value chain:** Compile-time default → NVS stored value → RAM cache (struct fields)

**NVS Write Debouncing:**
- `applyJson()` updates RAM cache instantly (hot-reload works immediately)
- `schedulePersist(2000)` queues NVS write after 2-second quiet period
- If user drags slider through 50 values in 3 seconds, NVS gets one write at the end
- Reboot-required keys bypass debouncing — persist to NVS immediately before restart

**Reboot-required keys:** `wifi_ssid`, `wifi_password`, `opensky_client_id`, `opensky_client_secret`, `aeroapi_key`, `display_pin`
**Hot-reload keys:** `brightness`, `text_color_r/g/b`, `fetch_interval`, `display_cycle`, `tiles_x`, `tiles_y`, `center_lat`, `center_lon`, `radius_km`, wiring flags

### Decision 2: Inter-Task Communication — Hybrid

**Config changes:** Atomic flag signals display task to re-read from ConfigManager
```cpp
std::atomic<bool> configChanged{false};

// Core 1: web server applies setting
ConfigManager::applyJson(settings);
configChanged.store(true);

// Core 0: display task checks each frame
if (configChanged.exchange(false)) {
    displayConfig = ConfigManager::getDisplay();
    hardwareConfig = ConfigManager::getHardware();
}
```

**Flight data:** FreeRTOS queue with overwrite semantics
```cpp
QueueHandle_t flightQueue;

// Core 1: flight pipeline completes
xQueueOverwrite(flightQueue, &enrichedFlights);

// Core 0: display task reads latest
FlightData flights;
if (xQueuePeek(flightQueue, &flights, 0) == pdTRUE) {
    renderFlights(flights);
}
```

**Why hybrid:** Config changes are lightweight signals (flag + struct copy). Flight data needs proper thread-safe transfer (queue). `xQueueOverwrite` ensures display always gets latest data.

### Decision 3: WiFi State Machine — WiFiManager

**States:**
```cpp
enum WiFiState {
    WIFI_AP_SETUP,
    WIFI_CONNECTING,
    WIFI_STA_CONNECTED,
    WIFI_STA_RECONNECTING,
    WIFI_AP_FALLBACK
};
```

**Transitions:**
```
POWER ON → [Has WiFi config?]
         No → AP_SETUP
         Yes → CONNECTING → Success → STA_CONNECTED
                          → Timeout → AP_FALLBACK
STA_CONNECTED → WiFi lost → STA_RECONNECTING → Reconnects → STA_CONNECTED
                                               → Timeout → AP_FALLBACK
```

**GPIO overrides:**
- Short press: show IP on LEDs for 5 seconds (no state change)
- Long press during boot: force AP_SETUP regardless of saved config

**Callbacks:** WebPortal, display task, and FlightDataFetcher register for state change notifications.

### Decision 4: Web API Endpoints

| Method | Endpoint | Purpose | Reboot |
|--------|----------|---------|--------|
| `GET` | `/` | wizard.html (AP) or dashboard.html (STA) | No |
| `GET` | `/api/status` | System health JSON | No |
| `GET` | `/api/settings` | All current config values | No |
| `POST` | `/api/settings` | Apply partial config update | Maybe |
| `GET` | `/api/wifi/scan` | Async WiFi scan results | No |
| `POST` | `/api/reboot` | Save + reboot | Yes |
| `POST` | `/api/reset` | Factory reset — erase NVS | Yes |
| `GET` | `/api/logos` | List uploaded logos | No |
| `POST` | `/api/logos` | Upload logo .bin (multipart) | No |
| `DELETE` | `/api/logos/:name` | Delete specific logo | No |
| `GET` | `/api/layout` | Current zone layout (initial load only) | No |

**`/api/layout` note:** Used for initial dashboard load only. During live editing, canvas calculates zones client-side using the shared zone algorithm.

**Response envelope:**
```json
{ "ok": true, "data": { ... } }
{ "ok": false, "error": "human-readable message", "code": "MACHINE_CODE" }
```

### Decision 5: Component Integration

**Dependency Graph:**
```
ConfigManager ← (everything depends on this)
SystemStatus ← (everything reports to this)
WiFiManager ← WebPortal
LogoManager ← WebPortal, DisplayTask
LayoutEngine ← DisplayTask, dashboard.js (shared algorithm)
FlightDataFetcher ← main loop
    ├── OpenSkyFetcher
    ├── AeroAPIFetcher
    └── FlightWallFetcher
DisplayTask (Core 0, read-only):
    ├── ConfigManager (atomic flag + struct copy)
    ├── flightQueue (FreeRTOS queue)
    ├── LogoManager (ICAO lookup)
    ├── LayoutEngine (zone positions)
    └── SystemStatus (error display)
```

**Initialization order:**
```
1. ConfigManager::init()
2. SystemStatus::init()
3. LogoManager::init()
4. LayoutEngine::init()
5. WiFiManager::init()
6. WebPortal::init()
7. FlightDataFetcher::init()
8. xTaskCreatePinnedToCore(displayTask, "display", 8192, NULL, 1, NULL, 0)
9. loop() runs flight fetch cycle on Core 1
```

**Rules:**
- ConfigManager initializes first — all others depend on it
- DisplayTask is read-only — never writes config, never calls APIs
- WebPortal is the only write path from the browser
- Existing adapters gain ConfigManager dependency but keep interface contracts unchanged

### Decision 6: Error Handling — SystemStatus Registry

```cpp
class SystemStatus {
public:
    static void init();
    static void set(Subsystem sys, StatusLevel level, const String& message);
    static SubsystemStatus get(Subsystem sys);
    static JsonObject toJson();
    static void setMetric(const String& key, const String& value);
    static String getMetric(const String& key);
};
```

**Subsystem health:**

| Subsystem | OK | Warning | Error |
|-----------|-----|---------|-------|
| WIFI | "Connected, -52 dBm" | "Weak signal, -78 dBm" | "Disconnected, retrying..." |
| OPENSKY | "Authenticated ✓" | "Token expiring in 5m" | "401 Unauthorized — check credentials" |
| AEROAPI | "Connected ✓" | "429 rate limited, retry 30s" | "503 Service unavailable" |
| CDN | "Reachable ✓" | — | "Unreachable — using ICAO codes" |
| NVS | "OK" | — | "Write failed — config may not persist" |
| LITTLEFS | "847KB / 1.9MB used" | "90%+ full" | "Mount failed" |

**API Call Counter — RAM + Hourly NVS Sync:**
- Running count in RAM, synced to NVS once per hour and on graceful reboot
- Loses at most 1 hour of data on unexpected reboot — acceptable for budget indicator
- Resets on 1st of month

### Shared Zone Calculation Algorithm

**Critical:** Implemented identically in C++ (LayoutEngine) and JavaScript (dashboard.js). If they diverge, preview won't match LEDs.

```
INPUT: tilesX, tilesY, tilePixels
COMPUTE:
    matrixWidth  = tilesX * tilePixels
    matrixHeight = tilesY * tilePixels

    if matrixHeight < 32:      mode = "compact"
    else if matrixHeight < 48: mode = "full"
    else:                      mode = "expanded"

    logoZone     = { x: 0, y: 0, w: matrixHeight, h: matrixHeight }
    flightZone   = { x: matrixHeight, y: 0, w: matrixWidth - matrixHeight, h: floor(matrixHeight/2) }
    telemetryZone = { x: matrixHeight, y: floor(matrixHeight/2), w: matrixWidth - matrixHeight, h: matrixHeight - floor(matrixHeight/2) }
OUTPUT: mode, logoZone, flightZone, telemetryZone
```

**Test vectors:**

| Config | Matrix | Mode | Logo | Flight Card | Telemetry |
|--------|--------|------|------|-------------|-----------|
| 10x2 @ 16px | 160x32 | full | 0,0 → 32x32 | 32,0 → 128x16 | 32,16 → 128x16 |
| 5x2 @ 16px | 80x32 | full | 0,0 → 32x32 | 32,0 → 48x16 | 32,16 → 48x16 |
| 12x3 @ 16px | 192x48 | expanded | 0,0 → 48x48 | 48,0 → 144x24 | 48,24 → 144x24 |
| 10x1 @ 16px | 160x16 | compact | 0,0 → 16x16 | 16,0 → 144x8 | 16,8 → 144x8 |

### Decision Impact — Implementation Sequence

1. ConfigManager + SystemStatus (foundation)
2. WiFiManager (enables web server and API calls)
3. WebPortal + API routes (enables browser interaction)
4. LogoManager (LittleFS file management)
5. LayoutEngine (zone calculation — shared algorithm in C++)
6. DisplayTask refactor (FreeRTOS, new layout, logos)
7. Existing adapter updates (ConfigManager integration)
8. Web assets (shared zone algorithm in JS, build dashboard)

## Implementation Patterns & Consistency Rules

### Existing Codebase Conventions (All New Code Must Follow)

The existing firmware establishes conventions that new components must match. These are rules derived from working code.

### Naming Patterns

**C++ Classes:** `PascalCase`
- Existing: `FlightDataFetcher`, `NeoMatrixDisplay`, `OpenSkyFetcher`, `AeroAPIFetcher`
- New: `ConfigManager`, `WiFiManager`, `WebPortal`, `LogoManager`, `LayoutEngine`, `SystemStatus`

**C++ Methods:** `camelCase`
- Existing: `fetchFlights()`, `fetchStateVectors()`, `displayMessage()`, `showLoading()`
- New: `applyJson()`, `schedulePersist()`, `getDisplay()`, `setState()`

**C++ Member Variables:** `_camelCase` (underscore prefix for private members)
- Existing: `_matrix`, `_leds`, `_matrixWidth`, `_currentFlightIndex`, `_stateFetcher`

**C++ Structs:** `PascalCase` name with `snake_case` fields
- Existing: `FlightInfo { String ident_icao; String operator_code; }`
- New: `DisplayConfig { uint8_t brightness; uint8_t text_color_r; }`
- **Rule:** Struct fields use `snake_case` to match NVS keys and JSON field names. One convention across all three layers — zero mental translation.

**Constants:** `UPPER_SNAKE_CASE` | **Namespaces:** `PascalCase` | **Files:** `PascalCase.h/.cpp` | **Headers:** `#pragma once`

### Config Structs (Corrected to snake_case Fields)

```cpp
struct DisplayConfig {
    uint8_t brightness;
    uint8_t text_color_r, text_color_g, text_color_b;
};

struct LocationConfig {
    double center_lat, center_lon, radius_km;
};

struct HardwareConfig {
    uint8_t tiles_x, tiles_y, tile_pixels, display_pin;
    uint8_t origin_corner, scan_dir, zigzag;
};

struct TimingConfig {
    uint16_t fetch_interval, display_cycle;
};

struct NetworkConfig {
    String wifi_ssid, wifi_password;
    String opensky_client_id, opensky_client_secret;
    String aeroapi_key;
};
```

### NVS Key Naming

**Convention:** `snake_case`, max 15 characters. Abbreviation rule: if natural name exceeds 15 chars, document abbreviation in `ConfigManager.h`:

```cpp
// NVS key abbreviations (15-char limit):
// os_client_id   = opensky_client_id
// os_client_sec  = opensky_client_secret
// scan_dir       = scan_direction
```

| NVS Key | Type | Default | Category |
|---------|------|---------|----------|
| `brightness` | uint8 | 5 | display |
| `text_color_r` | uint8 | 255 | display |
| `text_color_g` | uint8 | 255 | display |
| `text_color_b` | uint8 | 255 | display |
| `center_lat` | double | 37.7749 | location |
| `center_lon` | double | -122.4194 | location |
| `radius_km` | double | 10.0 | location |
| `tiles_x` | uint8 | 10 | hardware |
| `tiles_y` | uint8 | 2 | hardware |
| `tile_pixels` | uint8 | 16 | hardware |
| `display_pin` | uint8 | 25 | hardware |
| `origin_corner` | uint8 | 0 | hardware |
| `scan_dir` | uint8 | 0 | hardware |
| `zigzag` | uint8 | 0 | hardware |
| `fetch_interval` | uint16 | 30 | timing |
| `display_cycle` | uint16 | 3 | timing |
| `wifi_ssid` | string | "" | network |
| `wifi_password` | string | "" | network |
| `os_client_id` | string | "" | network |
| `os_client_sec` | string | "" | network |
| `aeroapi_key` | string | "" | network |
| `api_calls` | uint32 | 0 | metrics |
| `api_month` | uint8 | 0 | metrics |

**NVS namespace:** `"flightwall"`

### API Call Counter — Monthly Reset Strategy

**Primary (NTP):** On WiFi STA connect, `configTime(0, 0, "pool.ntp.org")` syncs time. Compare current month to `api_month` in NVS — if different, reset `api_calls` to 0 and update `api_month`.

**Fallback (manual):** If NTP fails, counter continues. Dashboard shows "Reset API Counter" button for manual reset.

### API JSON Field Names

**Convention:** `snake_case` in all JSON — matches struct fields and NVS keys.

```json
{ "brightness": 25, "text_color_r": 200, "fetch_interval": 120 }
```

### Logging Pattern — Compile-Time Log Levels

**File:** `utils/Log.h`

```cpp
#pragma once

#ifndef LOG_LEVEL
#define LOG_LEVEL 2  // Default: info
#endif

#define LOG_E(tag, msg) do { if (LOG_LEVEL >= 1) Serial.println("[" tag "] ERROR: " msg); } while(0)
#define LOG_I(tag, msg) do { if (LOG_LEVEL >= 2) Serial.println("[" tag "] " msg); } while(0)
#define LOG_V(tag, msg) do { if (LOG_LEVEL >= 3) Serial.println("[" tag "] " msg); } while(0)
```

**platformio.ini:** `-D LOG_LEVEL=3` for development, `-D LOG_LEVEL=1` for production.
**Tag format:** String literal matching class name. Macros compile to nothing at level 0.

### Error Handling Pattern

Boolean return + output parameter (matches existing codebase). Plus SystemStatus reporting and log macros:

```cpp
if (!fetchStateVectors(...)) {
    SystemStatus::set(OPENSKY, ERROR, "Fetch failed — HTTP " + String(httpCode));
    LOG_E("OpenSky", ("Fetch failed — HTTP " + String(httpCode)).c_str());
    return false;
}
```

**No exceptions.** No `try/catch`.

### Include Pattern

System/Arduino first → Library second → Project last. Forward declarations where possible.

### Memory Management

`String` for dynamic text, `const char*` for constants. Avoid `new`/`delete`. Stream LittleFS files.

### Web Asset Patterns

**JS Fetch Error Handling (mandatory):**

```javascript
async function applySettings(data) {
    try {
        const res = await fetch('/api/settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
        const json = await res.json();
        if (!json.ok) { showToast(json.error, 'error'); return null; }
        showToast('Settings saved', 'success');
        return json;
    } catch (err) {
        showToast('Connection failed', 'error');
        return null;
    }
}
```

**Rule:** Every `fetch()` call must check `json.ok` and call `showToast()` on failure.

### Enforcement Guidelines

**All AI agents implementing stories MUST:**

1. Follow naming conventions — check existing files before writing new code
2. Use `snake_case` for struct fields, NVS keys, and JSON fields
3. Use `#pragma once` in all headers
4. Use boolean return + output param for fallible operations
5. Report user-visible errors via `SystemStatus::set()`
6. Log via `LOG_E`/`LOG_I`/`LOG_V` from `utils/Log.h` — never raw `Serial.println`
7. Use `String` for dynamic text, `const char*` for constants
8. Stream LittleFS files — never buffer entire files in RAM
9. Place new classes in appropriate directory: `core/`, `adapters/`, `interfaces/`, `models/`, `utils/`
10. Every JS `fetch()` must check `json.ok` and handle failure with `showToast()`
11. Document NVS key abbreviations in `ConfigManager.h` if key exceeds 15 characters

## Project Structure & Boundaries

### Complete Project Directory Structure

```
firmware/
├── platformio.ini                     # UPDATED: new libs + partition + littlefs
├── custom_partitions.csv              # NEW: 2MB app + 1.9MB LittleFS
│
├── src/
│   └── main.cpp                       # UPDATED: init sequence + FreeRTOS task creation
│
├── core/
│   ├── FlightDataFetcher.h            # EXISTING (updated: ConfigManager dependency)
│   ├── FlightDataFetcher.cpp          # EXISTING (updated: ConfigManager dependency)
│   ├── ConfigManager.h                # NEW: singleton, category structs, NVS, debounce
│   ├── ConfigManager.cpp              # NEW
│   ├── LayoutEngine.h                 # NEW: zone calculation (shared algorithm)
│   ├── LayoutEngine.cpp               # NEW
│   ├── LogoManager.h                  # NEW: LittleFS CRUD, ICAO lookup, fallback
│   ├── LogoManager.cpp                # NEW
│   ├── SystemStatus.h                 # NEW: subsystem health registry
│   └── SystemStatus.cpp               # NEW
│
├── adapters/
│   ├── OpenSkyFetcher.h               # EXISTING (updated: ConfigManager)
│   ├── OpenSkyFetcher.cpp             # EXISTING (updated: ConfigManager)
│   ├── AeroAPIFetcher.h               # EXISTING (updated: ConfigManager)
│   ├── AeroAPIFetcher.cpp             # EXISTING (updated: ConfigManager)
│   ├── FlightWallFetcher.h            # EXISTING (unchanged)
│   ├── FlightWallFetcher.cpp          # EXISTING (unchanged)
│   ├── NeoMatrixDisplay.h             # EXISTING (updated: LayoutEngine, LogoManager)
│   ├── NeoMatrixDisplay.cpp           # EXISTING (updated: zones, logos, telemetry)
│   ├── WebPortal.h                    # NEW: ESPAsyncWebServer, API routes
│   ├── WebPortal.cpp                  # NEW
│   ├── WiFiManager.h                  # NEW: state machine, AP/STA, mDNS, scan
│   └── WiFiManager.cpp                # NEW
│
├── interfaces/
│   ├── BaseDisplay.h                  # EXISTING (unchanged)
│   ├── BaseFlightFetcher.h            # EXISTING (unchanged)
│   └── BaseStateVectorFetcher.h       # EXISTING (unchanged)
│
├── models/
│   ├── FlightInfo.h                   # EXISTING (unchanged)
│   ├── StateVector.h                  # EXISTING (unchanged)
│   └── AirportInfo.h                  # EXISTING (unchanged)
│
├── config/
│   ├── APIConfiguration.h             # EXISTING — compile-time defaults (see migration rule)
│   ├── WiFiConfiguration.h            # EXISTING — compile-time defaults
│   ├── UserConfiguration.h            # EXISTING — compile-time defaults
│   ├── HardwareConfiguration.h        # EXISTING — compile-time defaults
│   └── TimingConfiguration.h          # EXISTING — compile-time defaults
│
├── utils/
│   ├── GeoUtils.h                     # EXISTING (unchanged)
│   └── Log.h                          # NEW: compile-time log level macros
│
├── data/                              # NEW: LittleFS initial image → pio run -t uploadfs
│   ├── style.css.gz
│   ├── common.js.gz
│   ├── wizard.html.gz
│   ├── wizard.js.gz
│   ├── dashboard.html.gz
│   ├── dashboard.js.gz
│   └── leaflet/
│       ├── leaflet.min.js.gz
│       └── leaflet.min.css.gz
│
└── test/
    ├── test_config_manager/
    │   └── test_main.cpp
    └── test_layout_engine/
        └── test_main.cpp
```

### Config Header Migration Rule

Existing `config/` headers remain in place as compile-time default value source. **Only `ConfigManager.cpp` includes them.** Adapters migrate from direct config header reads to ConfigManager struct getters:

```cpp
// BEFORE: #include "config/APIConfiguration.h"
//         APIConfiguration::OPENSKY_CLIENT_ID
// AFTER:  #include "core/ConfigManager.h"
//         ConfigManager::getNetwork().opensky_client_id
```

### Runtime LittleFS Layout

```
LittleFS at runtime:
├── *.css.gz, *.html.gz, *.js.gz   ← from firmware/data/ (build-time)
├── leaflet/*.gz                     ← from firmware/data/ (build-time)
└── logos/                           ← created at runtime by LogoManager::init()
    ├── UAL.bin                      ← user-uploaded
    └── ...
```

Web assets at root, logos in `/logos/` subdirectory. LogoManager creates directory on init.

### Architectural Boundaries

**Adapter Boundary:** Browser ↔ WebPortal, WiFi Hardware ↔ WiFiManager, LEDs ↔ NeoMatrixDisplay, APIs ↔ Fetchers
**Core Boundary:** ConfigManager (config state), LayoutEngine (zones), LogoManager (files), SystemStatus (health), FlightDataFetcher (pipeline)
**Data Boundary:** Core 1 → Core 0 via atomic flag + FreeRTOS queue. Core 0 is read-only.

### NeoMatrixDisplay Refactoring Note

Do not split preemptively. If file exceeds ~400 lines after updates, consider extracting `FlightRenderer` for rendering logic, keeping NeoMatrixDisplay focused on hardware.

### Requirements to Structure Mapping

| Epic/Group | Key Files |
|-----------|-----------|
| Setup & Onboarding (FR1-8) | WiFiManager, WebPortal, wizard.html/js |
| Config Management (FR9-14) | ConfigManager, WebPortal, dashboard.html/js |
| Calibration & Preview (FR15-18) | WebPortal, LayoutEngine, dashboard.js |
| Flight Data (FR19-25) | FlightDataFetcher, NeoMatrixDisplay |
| Logo Display (FR26-29) | LogoManager, NeoMatrixDisplay |
| Responsive Layout (FR30-33) | LayoutEngine, NeoMatrixDisplay |
| Logo Management (FR34-37) | LogoManager, WebPortal, dashboard.js |
| System Operations (FR38-48) | SystemStatus, WiFiManager, main.cpp |

### Testing Expectations

**Unit tests required:** ConfigManager (NVS, defaults, applyJson, debounce), LayoutEngine (4 test vectors from shared algorithm)
**Not required:** Hardware-dependent adapters (WiFi, Web, Display, API fetchers)

### External Integration Points

| Service | Auth | Protocol |
|---------|------|----------|
| OpenSky Network | OAuth2 | HTTPS REST |
| FlightAware AeroAPI | API key | HTTPS REST |
| FlightWall CDN | None | HTTPS GET |
| OpenStreetMap | None | HTTPS tiles (client-side) |
| NTP | None | UDP |

### Data Flow

Boot → ConfigManager loads NVS → WiFiManager starts AP/STA → WebPortal serves UI → FlightDataFetcher runs fetch cycle (Core 1) → DisplayTask renders (Core 0) → User changes setting → POST /api/settings → ConfigManager updates RAM → atomic flag → display re-reads → NVS persists after 2s debounce

## Architecture Validation Results

### Coherence Validation ✅

**Decision Compatibility:** All technology choices compatible. mathieucarbou/ESPAsyncWebServer, LittleFS, ArduinoJson, and FastLED coexist on ESP32 Arduino without known conflicts.

**Pattern Consistency:** `snake_case` flows consistently across struct fields → NVS keys → JSON fields. Logging macros, error handling, and include patterns are uniform. No contradictions.

**Structure Alignment:** Directory structure maps directly to hexagonal architecture. Config migration rule is clear. New files in correct directories.

### Requirements Coverage ✅

**All 48 FRs covered.** Every functional requirement maps to at least one architectural component with a clear file location.

**All NFRs addressed.** Performance (hot-reload <1s, page load <3s), reliability (30+ day uptime, WiFi recovery), resource constraints (RAM, flash), concurrency (FreeRTOS dual-core) all have explicit architectural support.

### Implementation Readiness ✅

**Decision Completeness:** 6 core decisions with code examples, interfaces, and rationale. Versions pinned.
**Structure Completeness:** Every file listed as EXISTING/NEW/UPDATED. Migration rules documented.
**Pattern Completeness:** 11 enforcement rules, NVS key table, API endpoint table, JSON examples, JS fetch pattern, logging macros.

### Gap Analysis

| Priority | Gap | Resolution |
|----------|-----|-----------|
| Minor | Fallback airplane sprite storage | PROGMEM constant in LogoManager.h — 32x32 RGB565 array (~2KB flash) |
| Minor | GPIO reset pin not in HardwareConfig | Add `reset_pin` to HardwareConfig, default GPIO 0 (BOOT button), NVS key `reset_pin` |
| Minor | Watchdog timer | ESP32 task watchdog enabled by default. Rule: no task blocks >5 seconds |

No critical gaps.

### Architecture Completeness Checklist

- [x] Project context analyzed (48 FRs, NFRs, UX spec)
- [x] Scale assessed (Medium — IoT with web UI)
- [x] Constraints identified (flash, RAM, single radio, interrupt conflict)
- [x] Cross-cutting concerns mapped (config, WiFi, tasks, flash, errors)
- [x] Technology stack locked with specific forks and versions
- [x] Custom partition table specified
- [x] 6 core architectural decisions documented
- [x] Implementation patterns extracted from existing codebase
- [x] NVS key table with abbreviation rules
- [x] Logging macros with compile-time levels
- [x] 11 enforcement guidelines for AI agents
- [x] Complete directory tree (22 existing + 22 new files)
- [x] Architectural boundaries defined
- [x] Requirements mapped to files
- [x] Runtime LittleFS layout documented
- [x] Testing expectations set
- [x] Shared zone algorithm with test vectors

### Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION
**Confidence Level:** High

**Key Strengths:**
- Clean extension of existing hexagonal architecture — no breaking changes
- Comprehensive NVS key table eliminates config ambiguity
- Shared zone algorithm with test vectors ensures canvas/LED parity
- snake_case consistency across struct/NVS/JSON eliminates naming drift
- Party mode reviews caught critical issues (fork choice, debouncing, naming, logging)

**Areas for Future Enhancement:**
- ~~OTA firmware updates (Phase 2 — partition strategy documented)~~ → Addressed in Foundation Release below
- WebSocket push (deferred — not needed for set-and-forget UI)
- NeoMatrixDisplay split (if exceeds ~400 lines)

### Implementation Handoff

**First Implementation Priority:**
1. Add mathieucarbou/ESPAsyncWebServer + LittleFS to platformio.ini
2. Create custom_partitions.csv
3. Create utils/Log.h
4. Create core/ConfigManager.h/.cpp with NVS + category structs + fallbacks
5. Create core/SystemStatus.h/.cpp
6. Verify firmware builds and existing flight pipeline still works

---

# Foundation Release — Architecture Extension

_Extends the MVP architecture with OTA firmware updates, night mode/brightness scheduling, and onboarding polish. All MVP architectural decisions remain in effect — this section adds new decisions and updates existing ones where the Foundation PRD introduces changes._

## Foundation Release — Project Context Analysis

### Requirements Overview

**Functional Requirements (37 FRs across 6 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| OTA Firmware Updates | FR1-FR11 | **Major:** Partition table change (dual-OTA), new upload endpoint in WebPortal, self-check + rollback in boot sequence, `Update.h` integration |
| Settings Migration | FR12-FR14 | **Minor:** New `/api/settings/export` GET endpoint, client-side JSON import in wizard (~40 lines JS, no new server endpoint) |
| Night Mode & Brightness | FR15-FR24 | **Medium:** NTP integration via `configTzTime()`, 5 new ConfigManager keys, time-based brightness scheduler in main loop, IANA-to-POSIX timezone mapping |
| Onboarding Polish | FR25-FR29 | **Minor:** Wizard Step 6 extension — reuses existing calibration pattern and canvas preview endpoints |
| Dashboard UI | FR30-FR34 | **Minor:** New Firmware card and Night Mode card, rollback detection toast |
| System & Resilience | FR35-FR37 | **Medium:** Dual-OTA partition coexistence, OTA failure recovery path, new NVS config keys |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| OTA upload speed | ≤30s for 1.5MB binary | Stream chunks via `Update.write()` — no RAM buffering |
| NTP sync time | ≤10s after WiFi connect | `configTzTime()` with pool.ntp.org + time.nist.gov |
| Brightness hot-reload | ≤1s (unchanged from MVP) | Scheduler overrides brightness via existing ConfigManager hot-reload path |
| Firmware binary size | ≤1.5MB | Pre-implementation size gate at 1.3MB; reduced from 2MB MVP budget |
| LittleFS free space | ≥500KB after assets + logos | 896KB total - ~278KB used = ~618KB free |
| OTA self-check window | ≤30s boot-to-mark-valid | Sequential gate: WiFi (15s) + web server (2s) + buffer (13s) |
| Uptime with night mode | 30+ days | Non-blocking time comparison per loop iteration — no I/O, no blocking |
| Night mode scheduler overhead | ≤1% main loop impact | Single `localtime_r()` + integer comparison per iteration |

### Scale & Complexity

- **Scope:** Narrower than MVP — fewer new components, but partition change has wide blast radius
- **New architectural components:** 0 new classes (OTA upload in WebPortal, self-check in main.cpp, scheduler in main loop)
- **Updated components:** ConfigManager (5 new keys), WebPortal (2 new endpoints), main.cpp (self-check + scheduler), custom_partitions.csv, wizard HTML/JS
- **Complexity driver:** OTA self-check is the highest-risk code path — boot-time safety mechanism that must be bulletproof

### Technical Constraints (Foundation-Specific)

| Constraint | Impact |
|-----------|--------|
| Dual-OTA partitions require 3MB for two app slots | LittleFS shrinks from ~2MB to ~896KB (56% reduction) |
| Firmware binary must fit in 1.5MB partition | Tighter than MVP's 2MB budget — pre-implementation size gate required |
| One-time USB reflash required for partition migration | All NVS + LittleFS data erased — settings export mitigates |
| No built-in IANA-to-POSIX mapping on ESP32 | Need explicit mapping decision: browser-side, PROGMEM table, or LittleFS file |
| `Update.h` streams from ESPAsyncWebServer multipart callback | Must verify Carbou fork's `onUpload` callback chunk integration with `Update.write()` |
| NTP requires WiFi STA mode | Night mode schedule inactive in AP mode and until first NTP sync |
| ESP32 RTC drift ~5 ppm | ~0.4s/day — NTP re-sync every 24h keeps schedule accurate |

### Cross-Cutting Concerns (Foundation Additions)

1. **Flash budget governance** — LittleFS reduced 56%. Firmware binary capped at 1.5MB. Both limits must be tracked and enforced. Pre-implementation size gate at 1.3MB.

2. **OTA resilience chain** — Upload validation → stream-to-flash → reboot → self-check → mark valid OR watchdog rollback. This is a new failure recovery path that integrates with WiFiManager (WiFi connect check) and WebPortal (server start check) during boot.

3. **Time management (linear dependency, not cross-cutting)** — WiFi STA → `configTzTime()` → `localtime_r()` valid → scheduler compares time → overrides brightness via ConfigManager. NTP failure is *contained* — only the scheduler is affected. No cascade to other subsystems.

4. **Settings portability** — Export (`/api/settings/export`) bridges the one-time partition migration. Import is client-side only (JSON file → pre-fill wizard fields). No new server-side import endpoint.

### Existing Architecture Unchanged

The following MVP decisions remain in full effect with no modifications:

- Hexagonal (Ports & Adapters) pattern
- ConfigManager singleton with category struct getters
- Inter-task communication (atomic flags for config, FreeRTOS queue for flights)
- WiFi state machine (WiFiManager)
- SystemStatus error registry
- FreeRTOS dual-core task pinning (display Core 0, everything else Core 1)
- All naming conventions, NVS key patterns, logging macros, and enforcement rules
- Web asset patterns (fetch + json.ok + showToast)

## Foundation Release — Technology Stack

**Starter Template Evaluation: N/A (Brownfield Extension)**

No new external dependencies. All Foundation capabilities use libraries already in the ESP32 Arduino core:

| Library | Source | Purpose |
|---------|--------|---------|
| `Update.h` | ESP32 Arduino core | OTA flash write (stream chunks to inactive partition) |
| `esp_ota_ops.h` | ESP-IDF | `esp_ota_mark_app_valid_cancel_rollback()`, partition management |
| `time.h` / `configTzTime()` | ESP32 Arduino core | NTP sync with POSIX timezone, `localtime_r()` |
| `esp_sntp.h` | ESP-IDF | NTP sync status callback |

The MVP dependency stack (ESPAsyncWebServer Carbou fork, AsyncTCP, FastLED, ArduinoJson, LittleFS, ESPmDNS) remains unchanged.

## Foundation Release — Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. Partition table — dual OTA layout with exact offsets and sizes
2. OTA handler architecture — upload in WebPortal, self-check in main.cpp
3. OTA self-check — WiFi-OR-timeout strategy with 60-second window
4. IANA-to-POSIX timezone mapping — browser-side JS table
5. Night mode scheduler — non-blocking main loop check via ConfigManager

**Deferred Decisions (Post-Foundation):**
- OTA pull from GitHub releases (Delight Release)
- OTA firmware signing (Phase 3 — public distribution)
- Sunrise/sunset-aware brightness scheduling (Phase 2)

### Decision F1: Partition Table — Dual OTA Layout

**Updated `firmware/custom_partitions.csv`:**

```
# Name,    Type,  SubType, Offset,    Size,      Notes
nvs,       data,  nvs,     0x9000,    0x5000,    # 20KB — config persistence
otadata,   data,  ota,     0xE000,    0x2000,    # 8KB — OTA partition tracking
app0,      app,   ota_0,   0x10000,   0x180000,  # 1.5MB — active firmware
app1,      app,   ota_1,   0x190000,  0x180000,  # 1.5MB — OTA staging
spiffs,    data,  spiffs,  0x310000,  0xF0000,   # 960KB — LittleFS
```

**Flash Budget (Foundation):**

```
4MB Flash Total
├── 20KB  — NVS (config persistence)
├── 8KB   — OTA data (partition tracking)
├── 1.5MB — app0 (active firmware)
├── 1.5MB — app1 (OTA staging)
└── 960KB — LittleFS
    ├── ~80KB  — Web assets (gzipped HTML/JS/CSS)
    ├── ~198KB — 99 bundled logos (~2KB each)
    └── ~682KB — Free headroom
```

**Breaking change from MVP:** LittleFS shrinks from ~1.9MB to ~960KB. One-time USB reflash required — erases NVS + LittleFS. Settings export mitigates reconfiguration.

**Pre-implementation gate:** Measure current firmware binary size. If exceeding 1.3MB, optimize before adding Foundation code (disable Bluetooth with `CONFIG_BT_ENABLED=0`, strip unused library features).

### Decision F2: OTA Handler Architecture — WebPortal + main.cpp

**No new class.** OTA is split across two existing locations:

| Concern | Location | Implementation |
|---------|----------|---------------|
| Upload endpoint (`POST /api/ota/upload`) | `WebPortal.cpp` | ESPAsyncWebServer `onUpload` handler |
| Binary validation (size + magic byte) | `WebPortal.cpp` | Check `Content-Length` vs partition size, first byte == `0xE9` |
| Stream-to-flash | `WebPortal.cpp` | `Update.begin()` → `Update.write()` per chunk → `Update.end()` |
| Abort on failure | `WebPortal.cpp` | `Update.abort()` on stream error or WiFi interruption |
| Progress reporting | `WebPortal.cpp` | Bytes written / total sent via upload handler callback |
| Reboot after success | `WebPortal.cpp` | `ESP.restart()` after successful `Update.end(true)` |
| Self-check + mark valid | `main.cpp` | Boot sequence, after WiFiManager + WebPortal init |
| Rollback detection | `main.cpp` | `esp_ota_check_rollback_is_possible()` at boot → SystemStatus |
| Version string | `platformio.ini` | `-D FW_VERSION=\"1.0.0\"` build flag |

**Upload handler flow:**

```cpp
// In WebPortal::init()
server.on("/api/ota/upload", HTTP_POST,
    // Response handler (called after upload completes)
    [](AsyncWebServerRequest *request) {
        bool success = !Update.hasError();
        request->send(200, "application/json",
            success ? "{\"ok\":true,\"message\":\"Rebooting...\"}"
                    : "{\"ok\":false,\"error\":\"Update failed\"}");
        if (success) { delay(500); ESP.restart(); }
    },
    // Upload handler (called per chunk)
    [](AsyncWebServerRequest *request, String filename,
       size_t index, uint8_t *data, size_t len, bool final) {
        if (index == 0) {
            // First chunk — validate magic byte and begin
            // Note: request->contentLength() is the multipart body size, NOT the file size.
            // Use UPDATE_SIZE_UNKNOWN and let Update.h track actual bytes written.
            // Final size validation happens at Update.end().
            if (data[0] != 0xE9) {
                request->send(400, "application/json",
                    "{\"ok\":false,\"error\":\"Not a valid ESP32 firmware image\"}");
                return;
            }
            // Get max OTA partition size from the partition table
            const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
            if (!Update.begin(partition->size)) {
                request->send(500, "application/json",
                    "{\"ok\":false,\"error\":\"Failed to begin update\"}");
                return;
            }
        }
        // Write chunk
        if (Update.write(data, len) != len) {
            Update.abort();
            return;
        }
        if (final) {
            if (!Update.end(true)) {
                // Update.end() validates written size — rejects if corrupt
                return;
            }
        }
    }
);
```

**Firmware version:**

```ini
# platformio.ini
build_flags =
    -D FW_VERSION=\"1.0.0\"
    -D LOG_LEVEL=2
```

```cpp
// In /api/status response
doc["firmware_version"] = FW_VERSION;
doc["rollback_detected"] = rollbackDetected;
```

### Decision F3: OTA Self-Check — WiFi-OR-Timeout

**Strategy:** Mark firmware valid when WiFi connects OR after 60-second timeout — whichever comes first. No self-HTTP-request. No web server check.

**Rationale:**
- WiFi connection proves network stack works (catches most firmware bugs)
- 60-second timeout prevents false rollback during temporary network outages (router reboot, ISP flap)
- Crash-on-boot is caught by the watchdog timer regardless (firmware never reaches the check)
- No self-HTTP-request avoids complexity and race conditions with async server startup

**Timeout budget:**

| Scenario | Time to mark valid | Outcome |
|----------|-------------------|---------|
| Good firmware + WiFi works | 5-15s (WiFi connect time) | Mark valid ✅ |
| Good firmware + WiFi temporarily down | 60s (timeout) | Mark valid with warning ✅ |
| Bad firmware (crashes before 60s) | Never | Watchdog rollback ✅ |
| Bad firmware (WiFi code broken, doesn't crash) | 60s (timeout) | Mark valid ⚠️ — but device is reachable via AP fallback for re-flash |

**Implementation:**

```cpp
// main.cpp — global state
static uint32_t bootTime;
static bool otaMarkedValid = false;

// In setup(), after all init:
bootTime = millis();

// In loop(), before flight fetch:
if (!otaMarkedValid) {
    bool wifiUp = (WiFi.status() == WL_CONNECTED);
    bool timeoutExpired = (millis() - bootTime) > 60000;

    if (wifiUp || timeoutExpired) {
        esp_ota_mark_app_valid_cancel_rollback();
        otaMarkedValid = true;
        if (!wifiUp) {
            SystemStatus::set(OTA, WARNING, "Marked valid on timeout — WiFi not verified");
            LOG_I("OTA", "Marked valid on timeout, WiFi not connected");
        } else {
            LOG_I("OTA", "Marked valid, WiFi connected in " +
                  String((millis() - bootTime) / 1000) + "s");
        }
    }
}
```

**Rollback detection (also in main.cpp setup):**

```cpp
// In setup(), early — detect if bootloader rolled us back from a failed update
#include "esp_ota_ops.h"

const esp_partition_t* lastInvalid = esp_ota_get_last_invalid_partition();
if (lastInvalid != NULL) {
    // A previous OTA partition was marked invalid by the bootloader — rollback occurred
    rollbackDetected = true;
    SystemStatus::set(OTA, WARNING, "Firmware rolled back to previous version");
    LOG_I("OTA", "Rollback detected — previous partition was invalid");
}

// Note: esp_ota_check_rollback_is_possible() answers a different question
// ("can I roll back?"), NOT "did I just roll back?" — don't use it for detection.
// esp_ota_get_last_invalid_partition() returns non-NULL only when the bootloader
// actually invalidated a partition, which is the true rollback signal.
```

### Decision F4: IANA-to-POSIX Timezone Mapping — Browser-Side

**Strategy:** Browser converts IANA timezone to POSIX string client-side. ESP32 stores and uses the POSIX string directly. Zero firmware/LittleFS overhead.

**JS mapping table:** ~50-80 common timezones in a JS object. Generated from IANA `tzdata`. Stored in wizard.js and dashboard.js (compresses well in gzip).

```javascript
// timezone-map.js (embedded in wizard.js and dashboard.js)
const TZ_MAP = {
    "America/New_York":    "EST5EDT,M3.2.0,M11.1.0",
    "America/Chicago":     "CST6CDT,M3.2.0,M11.1.0",
    "America/Denver":      "MST7MDT,M3.2.0,M11.1.0",
    "America/Los_Angeles": "PST8PDT,M3.2.0,M11.1.0",
    "America/Anchorage":   "AKST9AKDT,M3.2.0,M11.1.0",
    "Pacific/Honolulu":    "HST10",
    "Europe/London":       "GMT0BST,M3.5.0/1,M10.5.0",
    "Europe/Berlin":       "CET-1CEST,M3.5.0,M10.5.0/3",
    "Europe/Paris":        "CET-1CEST,M3.5.0,M10.5.0/3",
    "Asia/Tokyo":          "JST-9",
    "Asia/Shanghai":       "CST-8",
    "Australia/Sydney":    "AEST-10AEDT,M10.1.0,M4.1.0/3",
    // ... ~50-80 entries total
};

function getTimezoneConfig() {
    const iana = Intl.DateTimeFormat().resolvedOptions().timeZone;
    const posix = TZ_MAP[iana] || null;
    return { iana, posix };
}
```

**Wizard flow:**
1. Auto-detect: `Intl.DateTimeFormat().resolvedOptions().timeZone` → `"America/Los_Angeles"`
2. Lookup: `TZ_MAP["America/Los_Angeles"]` → `"PST8PDT,M3.2.0,M11.1.0"`
3. Show dropdown with IANA names, pre-selected to detected timezone
4. Send POSIX string to ESP32 via `POST /api/settings { "timezone": "PST8PDT,M3.2.0,M11.1.0" }`
5. If lookup fails (unknown IANA): show text field for manual POSIX entry

**Dashboard timezone selector:** Same dropdown + mapping. User can change timezone post-setup.

**ESP32 usage:**
```cpp
String tz = ConfigManager::getSchedule().timezone;  // POSIX string
configTzTime(tz.c_str(), "pool.ntp.org", "time.nist.gov");
```

### Decision F5: Night Mode Scheduler — Non-Blocking Main Loop

**Architecture:** Brightness schedule check runs in the Core 1 main loop, adjacent to the flight fetch cycle. Non-blocking — single `getLocalTime()` + integer comparison per iteration.

**Schedule times stored as minutes since midnight** (uint16, 0-1439):
- 23:00 = 1380, 07:00 = 420, 00:00 = 0, 12:30 = 750

**Midnight-crossing logic:**

```cpp
bool inDimWindow;
if (dimStart <= dimEnd) {
    // Same-day: e.g., 09:00-17:00
    inDimWindow = (currentMinutes >= dimStart && currentMinutes < dimEnd);
} else {
    // Midnight crossing: e.g., 23:00-07:00
    inDimWindow = (currentMinutes >= dimStart || currentMinutes < dimEnd);
}
```

**Interaction with manual brightness:**
- When schedule is **enabled** and device is **in dim window**: scheduler overrides brightness
- When schedule is **enabled** and device is **outside dim window**: normal brightness from ConfigManager
- When schedule is **disabled**: scheduler does nothing, manual brightness applies
- If user manually changes brightness while in dim window: schedule re-overrides on next check (1-second loop)
- Dashboard should indicate when schedule is actively overriding brightness

**NTP sync tracking:**

```cpp
// Callback registered once in setup()
sntp_set_time_sync_notification_cb([](struct timeval *tv) {
    ntpSynced = true;
    LOG_I("NTP", "Time synchronized");
    SystemStatus::set(NTP, OK, "Clock synced");
});

// NTP re-sync happens automatically every 1 hour (LWIP default)
// If re-sync fails, device continues using last known time
```

**NTP re-sync interval:** ESP32 LWIP SNTP defaults to 1-hour re-sync. Sufficient for <5 ppm RTC drift (~0.4s/day). No custom interval needed.

### Decision F6: ConfigManager Expansion — New Keys & Struct

**5 new NVS keys:**

| NVS Key | Type | Default | Category | Notes |
|---------|------|---------|----------|-------|
| `timezone` | string | `"UTC0"` | schedule | POSIX timezone string, max 40 chars |
| `sched_enabled` | uint8 | 0 | schedule | 0 = disabled, 1 = enabled |
| `sched_dim_start` | uint16 | 1380 | schedule | Minutes since midnight (23:00) |
| `sched_dim_end` | uint16 | 420 | schedule | Minutes since midnight (07:00) |
| `sched_dim_brt` | uint8 | 10 | schedule | Brightness during dim window (0-255) |

```cpp
// NVS key abbreviations (15-char limit):
// sched_enabled   = schedule_enabled
// sched_dim_start = schedule_dim_start
// sched_dim_end   = schedule_dim_end
// sched_dim_brt   = schedule_dim_brightness
```

**New ConfigManager category struct:**

```cpp
struct ScheduleConfig {
    bool enabled;
    uint16_t dim_start;      // minutes since midnight (0-1439)
    uint16_t dim_end;        // minutes since midnight (0-1439)
    uint8_t dim_brightness;  // 0-255
    String timezone;         // POSIX timezone string
};
```

**ConfigManager additions:**
```cpp
static ScheduleConfig getSchedule();
// Reboot-required keys: unchanged (timezone change takes effect on next NTP sync, no reboot)
// Hot-reload keys: sched_enabled, sched_dim_start, sched_dim_end, sched_dim_brt, timezone
```

All 5 new keys are **hot-reload** — no reboot required. Timezone change calls `configTzTime()` with the new POSIX string immediately.

### Decision F7: API Endpoint Additions

**2 new endpoints:**

| Method | Endpoint | Purpose | Auth | Response |
|--------|----------|---------|------|----------|
| `POST` | `/api/ota/upload` | Multipart firmware upload | None (local network) | `{ ok: true, message: "Rebooting..." }` or `{ ok: false, error: "..." }` |
| `GET` | `/api/settings/export` | Download all config as JSON file | None | `Content-Disposition: attachment; filename=flightwall-settings.json` |

**Updated existing endpoint — `/api/status`:**

New fields in response:
```json
{
    "ok": true,
    "data": {
        "firmware_version": "1.0.0",
        "rollback_detected": false,
        "ntp_synced": true,
        "schedule_active": true,
        "uptime_seconds": 86400,
        // ... existing fields ...
    }
}
```

**Settings export format (`/api/settings/export`):**

```json
{
    "flightwall_settings_version": 1,
    "exported_at": "2026-04-11T15:30:00",
    "wifi_ssid": "MyNetwork",
    "wifi_password": "secret123",
    "os_client_id": "...",
    "os_client_sec": "...",
    "aeroapi_key": "...",
    "center_lat": 37.7749,
    "center_lon": -122.4194,
    "radius_km": 10.0,
    "tiles_x": 10,
    "tiles_y": 2,
    "tile_pixels": 16,
    "display_pin": 25,
    "origin_corner": 0,
    "scan_dir": 0,
    "zigzag": 0,
    "brightness": 40,
    "text_color_r": 255,
    "text_color_g": 255,
    "text_color_b": 255,
    "fetch_interval": 30,
    "display_cycle": 3,
    "timezone": "PST8PDT,M3.2.0,M11.1.0",
    "sched_enabled": 1,
    "sched_dim_start": 1380,
    "sched_dim_end": 420,
    "sched_dim_brt": 10
}
```

All NVS keys dumped as flat JSON. Includes `flightwall_settings_version` for forward compatibility. Import is client-side only — wizard JS reads the file and pre-fills form fields.

### Foundation Decision Impact — Implementation Sequence

**Sequential foundation (must be in order):**
1. **Partition table update** (`custom_partitions.csv`) — must be first, requires USB reflash
2. **ConfigManager expansion** (5 new keys + ScheduleConfig struct) — foundation for everything else

**OTA track (sequential, after step 2):**
3. **OTA upload endpoint** in WebPortal — core Foundation capability
4. **OTA self-check + rollback detection** in main.cpp — safety mechanism for all future OTA updates

**Time track (sequential, parallelizable with OTA track after step 2):**
5. **NTP integration** (`configTzTime()` after WiFi connect) — enables night mode
6. **Night mode scheduler** in main loop — uses NTP + ConfigManager

**Settings track (after step 2, parallelizable with both tracks above):**
7. **Settings export endpoint** — migration tool for partition change

**UI integration (after all tracks complete):**
8. **Dashboard cards** (Firmware, Night Mode) — UI for new capabilities
9. **Wizard Step 6** (Test Your Wall) + timezone auto-detect — onboarding polish
10. **Settings import** in wizard JS — client-side only, references exported JSON

**Parallelization note:** Steps 3-4, 5-6, and 7 are independent tracks that all depend on step 2 but not on each other. For solo dev, linear order is fine. For sprint planning, these represent parallelizable work streams.

### Cross-Decision Dependencies

```
custom_partitions.csv
    └── OTA upload endpoint (needs dual partitions)
        └── OTA self-check (validates after OTA reboot)

ConfigManager (ScheduleConfig)
    └── NTP integration (uses timezone from config)
        └── Night mode scheduler (uses NTP time + schedule config)

Settings export endpoint
    └── Settings import in wizard (reads export file)

Wizard Step 6 (Test Your Wall)
    └── Timezone auto-detect in wizard (IANA-to-POSIX JS mapping)
```

## Foundation Release — Implementation Patterns & Consistency Rules

### Foundation-Specific Conflict Points

The MVP architecture established 11 enforcement rules. The Foundation Release introduces 4 new pattern areas where AI agents could make inconsistent choices.

### OTA Upload Handler Pattern

**Validation-first, fail-fast:**
- Check `Content-Length` against `MAX_OTA_SIZE` before calling `Update.begin()` — reject oversized before allocating flash
- Check first byte == `0xE9` (ESP32 image magic byte) on first chunk — reject non-firmware immediately
- Every error returns `{ ok: false, error: "human-readable message" }` with appropriate HTTP status code

**Streaming pattern:**
```cpp
// First chunk (index == 0): validate and begin
if (contentLength > MAX_OTA_SIZE) { /* reject 400 */ }
if (data[0] != 0xE9) { /* reject 400 */ }
if (!Update.begin(contentLength)) { /* reject 500 */ }

// Every chunk: write and verify
if (Update.write(data, len) != len) { Update.abort(); return; }

// Final chunk: finalize
if (final) { Update.end(true); }
```

**Anti-pattern:** Never buffer the entire binary in RAM before writing. The binary can be 1.5MB — ESP32 has ~280KB usable RAM. Always stream chunk-by-chunk via `Update.write()`.

### Time Management Pattern

**Non-blocking time access (mandatory):**
```cpp
struct tm now;
if (!getLocalTime(&now, 0)) return;  // timeout=0, NON-BLOCKING
```

**Rule:** Always pass `timeout=0` to `getLocalTime()`. Never block the main loop waiting for time. If time isn't available, skip the time-dependent operation gracefully.

**NTP sync state tracking:**
```cpp
// Register callback once in setup()
sntp_set_time_sync_notification_cb([](struct timeval *tv) {
    ntpSynced = true;
});

// Check state via atomic bool — never poll NTP status
std::atomic<bool> ntpSynced{false};
```

**Anti-pattern:** Polling `sntp_get_sync_status()` in a loop. Use the callback to set a flag and check the flag.

### Schedule Time Convention

**Minutes since midnight (uint16, 0-1439):**
- Compact storage (2 bytes per time vs. 5-byte HH:MM string)
- Simple arithmetic for midnight-crossing comparison
- No string parsing at runtime on the ESP32

**Conversion (client-side JS only):**
```javascript
// HH:MM string → minutes since midnight
function timeToMinutes(hhmm) {
    const [h, m] = hhmm.split(':').map(Number);
    return h * 60 + m;
}

// minutes since midnight → HH:MM string
function minutesToTime(mins) {
    const h = String(Math.floor(mins / 60)).padStart(2, '0');
    const m = String(mins % 60).padStart(2, '0');
    return `${h}:${m}`;
}
```

**Anti-pattern:** Storing schedule times as HH:MM strings in NVS or passing time strings between firmware components. Always use `uint16` minutes in firmware.

### Settings Export/Import Boundary

**Export: server-side.**
- `GET /api/settings/export` dumps all ConfigManager keys as flat JSON
- `Content-Disposition: attachment; filename=flightwall-settings.json`
- Includes `flightwall_settings_version: 1` for forward compatibility

**Import: client-side only.**
- Wizard JS reads the file via `FileReader API`
- Pre-fills form fields for user review
- User confirms before settings are applied via normal `POST /api/settings`
- Unrecognized keys in the file are silently ignored
- No server-side import endpoint — no new attack surface

**Anti-pattern:** Building a `POST /api/settings/import` endpoint that blindly applies a JSON file. Import is always mediated through the wizard UI with user confirmation.

### Updated Enforcement Guidelines

**All AI agents implementing Foundation stories MUST also follow (in addition to MVP rules 1-11):**

12. OTA upload must validate before writing — size check + magic byte on first chunk, fail-fast with `{ ok: false, error: "..." }` JSON response
13. OTA upload must stream directly to flash via `Update.write()` per chunk — never buffer the full binary in RAM
14. `getLocalTime()` must always use `timeout=0` (non-blocking) — never block the main loop waiting for time
15. Schedule times stored as `uint16` minutes since midnight (0-1439) — no string time representations in firmware
16. Settings import is client-side only — no server-side import endpoint or file parsing on the ESP32

## Foundation Release — Project Structure & Boundaries

### File Change Map

No new files. All Foundation changes fit into existing components:

| File | Change | Foundation Purpose |
|------|--------|-------------------|
| `firmware/custom_partitions.csv` | UPDATED | Dual OTA partition layout (app0 1.5MB + app1 1.5MB + spiffs 960KB) |
| `firmware/platformio.ini` | UPDATED | `-D FW_VERSION=\"1.0.0\"` build flag, potential `-D CONFIG_BT_ENABLED=0` |
| `firmware/src/main.cpp` | UPDATED | OTA self-check + rollback detection at boot, NTP init after WiFi, brightness scheduler in loop() |
| `firmware/core/ConfigManager.h` | UPDATED | ScheduleConfig struct, `getSchedule()`, 5 new NVS key abbreviations |
| `firmware/core/ConfigManager.cpp` | UPDATED | ScheduleConfig NVS read/write/defaults, timezone hot-reload calls `configTzTime()` |
| `firmware/core/SystemStatus.h` | UPDATED | New subsystem entries: `OTA`, `NTP` |
| `firmware/adapters/WebPortal.h` | UPDATED | OTA upload + settings export endpoint declarations |
| `firmware/adapters/WebPortal.cpp` | UPDATED | `/api/ota/upload` multipart handler, `/api/settings/export` GET handler, updated `/api/status` response |
| `firmware/data-src/dashboard.html` | UPDATED | Firmware card, Night Mode card, rollback warning toast |
| `firmware/data-src/dashboard.js` | UPDATED | OTA upload UI with progress, night mode settings, timezone selector dropdown, TZ_MAP |
| `firmware/data-src/wizard.html` | UPDATED | Step 6 (Test Your Wall), timezone field in Location step |
| `firmware/data-src/wizard.js` | UPDATED | Step 6 confirmation/back-nav/RGB test logic, timezone auto-detect, TZ_MAP, settings import via FileReader |
| `firmware/data/*.gz` | REBUILT | Gzipped copies of all updated web assets |

### Requirements to File Mapping

| FR Group | Key Files |
|----------|-----------|
| OTA Updates (FR1-FR11) | `WebPortal.cpp` (upload endpoint, validation, streaming), `main.cpp` (self-check, rollback detection), `custom_partitions.csv` |
| Settings Migration (FR12-FR14) | `WebPortal.cpp` (`/api/settings/export`), `wizard.js` (FileReader import, pre-fill fields) |
| Night Mode (FR15-FR24) | `ConfigManager` (ScheduleConfig, 5 keys), `main.cpp` (NTP init, scheduler), `dashboard.js` (Night Mode card) |
| Onboarding Polish (FR25-FR29) | `wizard.html` (Step 6 UI), `wizard.js` (Step 6 logic, timezone auto-detect, TZ_MAP) |
| Dashboard UI (FR30-FR34) | `dashboard.html` (Firmware + Night Mode cards), `dashboard.js` (OTA upload, schedule settings, rollback toast) |
| System & Resilience (FR35-FR37) | `main.cpp` (self-check gate), `ConfigManager` (new NVS keys), `SystemStatus` (OTA + NTP subsystems) |

### Architectural Boundaries — No Changes

All Foundation changes respect the existing hexagonal architecture:

- **WebPortal** remains the sole browser adapter — OTA upload and settings export are new routes, not new boundary crossings
- **ConfigManager** remains the config core — 5 new keys follow the existing category struct pattern
- **main.cpp** remains the orchestrator — self-check and scheduler are boot/loop concerns, not new components
- **SystemStatus** remains the health registry — OTA and NTP are new subsystem entries
- **Core 0 (display) remains read-only** — brightness scheduler runs on Core 1 and uses the existing ConfigManager hot-reload path to affect display brightness

### Foundation Data Flow

```
Boot → ConfigManager::init() (loads 5 new schedule keys from NVS)
     → SystemStatus::init() (registers OTA + NTP subsystems)
     → WiFiManager::init() → WiFi connects
     → configTzTime(timezone, "pool.ntp.org", "time.nist.gov")  ← NEW
     → sntp callback → ntpSynced = true                          ← NEW
     → WebPortal::init() (registers /api/ota/upload, /api/settings/export)
     → OTA self-check: WiFi connected OR 60s timeout → mark valid ← NEW
     → Rollback detection → SystemStatus::set(OTA, WARNING, ...) ← NEW
     → loop():
         Flight fetch cycle (unchanged)
         checkBrightnessSchedule()  ← NEW (non-blocking, Core 1)
```

### External Integration Points — Foundation Additions

| Service | Protocol | Purpose | New? |
|---------|----------|---------|------|
| NTP (pool.ntp.org, time.nist.gov) | UDP port 123 | Time sync for night mode scheduler | **Yes** |

No other new external integrations. OTA is local (browser → ESP32 over LAN).

## Foundation Release — Architecture Validation Results

### Coherence Validation ✅

**Decision Compatibility:** All 7 Foundation decisions (F1-F7) are compatible with each other and with the 6 MVP decisions. No technology conflicts. Partition table change (F1) enables OTA (F2/F3), NTP (F4/F5) enables scheduler (F5), ConfigManager expansion (F6) supports both. Clean dependency chain with no cycles.

**Pattern Consistency:** Foundation patterns (rules 12-16) extend MVP patterns (rules 1-11) without contradictions. `snake_case` flows consistently through new NVS keys (`sched_enabled`, `sched_dim_start`, etc.) → JSON fields → ScheduleConfig struct fields. Logging and error handling follow existing macros and boolean-return patterns.

**Structure Alignment:** All Foundation changes fit into existing files and directories. No new classes, no new boundary crossings. WebPortal gets new routes (consistent pattern). ConfigManager gets a new category struct (consistent pattern). main.cpp gets boot-time and loop-time additions (consistent with existing init sequence and flight fetch loop).

### Requirements Coverage ✅

**All 37 Functional Requirements covered.** Every FR maps to at least one architectural decision with explicit code location:
- OTA (FR1-FR11): Decisions F1, F2, F3 — WebPortal.cpp, main.cpp, custom_partitions.csv
- Settings Migration (FR12-FR14): Decision F7 — WebPortal.cpp, wizard.js
- Night Mode (FR15-FR24): Decisions F4, F5, F6 — ConfigManager, main.cpp, dashboard.js, wizard.js
- Onboarding (FR25-FR29): Structure map — wizard.html/js (reuses existing calibration endpoints)
- Dashboard UI (FR30-FR34): Structure map — dashboard.html/js
- System (FR35-FR37): Decisions F1, F2, F3, F6 — main.cpp, ConfigManager

**All NFRs architecturally addressed:**
- Performance: streaming OTA (≤30s), non-blocking scheduler (≤1%), NTP (≤10s), hot-reload (≤1s)
- Reliability: self-check gate, watchdog rollback, NVS persistence, NTP re-sync, graceful NTP failure
- Resource constraints: 1.5MB firmware cap, 682KB LittleFS free, streaming (no RAM buffer), ≤256 bytes NVS

### Implementation Readiness ✅

**Decision Completeness:** 7 decisions with code examples, interface definitions, and rationale. Partition sizes specified with exact offsets. NVS key table complete with types, defaults, and abbreviations. API endpoints specified with request/response formats.

**Structure Completeness:** File change map covers all 13 affected files. Requirements-to-file mapping is explicit. No orphaned files, no unmapped FRs.

**Pattern Completeness:** 16 enforcement rules (11 MVP + 5 Foundation). OTA streaming, time management, schedule convention, and settings boundary patterns all specified with code examples and anti-patterns.

### Architectural Overrides

**FR5 override (self-check simplification):**

The PRD specifies FR5 as: *"Device can verify its own health after booting new firmware (WiFi, web server, OTA endpoint reachability) before marking the partition as valid."*

The architecture simplifies this to: WiFi connects OR 60-second timeout. No web server check, no self-HTTP-request to `/api/ota/upload`.

**Rationale:** The self-HTTP-request adds complexity and race conditions (async server may not be fully initialized when the check runs). WiFi connection proves the network stack works. If the web server failed to initialize, the firmware would crash before reaching the self-check — caught by the watchdog. The timeout path catches temporary network outages without false rollback. The PRD's intent (don't brick the device) is fully preserved by the simpler approach.

**Story specs MUST reference this architectural decision (F3), not the raw FR5 wording, to prevent re-implementation of the self-HTTP-request.**

### Gap Analysis

| Priority | Gap | Resolution |
|----------|-----|-----------|
| Minor | `MAX_OTA_SIZE` derivation | Use `esp_ota_get_next_update_partition(NULL)->size` to get partition capacity at runtime. No hardcoded constant. Already reflected in F2 code example. |
| Minor | Rollback detection API | Use `esp_ota_get_last_invalid_partition()` (returns non-NULL only after actual rollback). Do NOT use `esp_ota_check_rollback_is_possible()` which answers "can I roll back?" not "did I just roll back?" Already corrected in F3 code example. |
| Minor | Multipart Content-Length | `request->contentLength()` returns multipart body size, not file size. Use `UPDATE_SIZE_UNKNOWN` with `Update.begin(partition->size)` and let `Update.end()` validate. Already corrected in F2 code example. |
| Minor | 60s timeout path testability | Timeout path verified by code inspection + one-time manual test (boot with WiFi unavailable). Not expected in automated test coverage. |
| Minor | Settings export/import round-trip | Export includes `flightwall_settings_version` and `exported_at` metadata fields. Import must ignore these (they're not config keys). Wizard JS should silently skip unrecognized keys per FR14. |
| Minor | Manual brightness during dim window | Schedule re-overrides brightness on next check (~1s). Dashboard should indicate "Schedule active — brightness overridden" when in dim window. Document as known behavior, not a bug. |
| Minor | TZ_MAP completeness | Ship with ~50-80 common timezones. Manual POSIX entry fallback for unlisted zones. Expandable in future releases. |

No critical gaps.

### Foundation Architecture Completeness Checklist

- [x] Foundation PRD context analyzed (37 FRs, NFRs, constraints)
- [x] Scale assessed (narrow extension, partition change is main blast radius)
- [x] Constraints identified (1.5MB firmware cap, 960KB LittleFS, NTP dependency)
- [x] Cross-cutting concerns mapped (flash budget, OTA resilience, time management)
- [x] No new external dependencies (all ESP32 core libraries)
- [x] 7 Foundation decisions documented with code examples
- [x] Partition table specified with exact offsets
- [x] Implementation patterns extended (rules 12-16)
- [x] NVS key table expanded (5 new keys with abbreviations)
- [x] 2 new API endpoints specified with request/response formats
- [x] Complete file change map (13 files, all UPDATED)
- [x] Requirements-to-file mapping complete (37 FRs → specific files)
- [x] Architectural boundaries unchanged
- [x] Implementation sequence with parallelization annotations
- [x] FR5 architectural override documented with rationale
- [x] All party mode refinements incorporated (rollback API, multipart handling, parallelization)

### Foundation Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION
**Confidence Level:** High

**Key Strengths:**
- Clean extension of MVP architecture — no breaking changes to existing patterns
- OTA self-check is simplified and robust (WiFi-OR-timeout avoids false rollbacks)
- Rollback detection uses correct ESP-IDF API (`esp_ota_get_last_invalid_partition()`)
- Browser-side timezone mapping avoids firmware/LittleFS overhead
- Non-blocking scheduler with zero new inter-core communication
- All enforcement rules are additive (no changes to MVP rules 1-11)
- Party mode reviews caught 3 code-level issues before implementation (rollback API, multipart Content-Length, FR5 override)

**Areas for Future Enhancement:**
- OTA pull from GitHub releases (Delight Release — already in PRD pipeline)
- Firmware signing for public distribution (Phase 3)
- Sunrise/sunset-aware brightness scheduling (Phase 2)
- Expanded TZ_MAP coverage beyond initial ~50-80 timezones

### Foundation Implementation Handoff

**Pre-implementation gate:**
1. Measure current firmware binary size (`~/.platformio/penv/bin/pio run` → check `.pio/build/*/firmware.bin`)
2. If exceeding 1.3MB: disable Bluetooth (`-D CONFIG_BT_ENABLED=0`), review library compile flags
3. Proceed only when binary fits comfortably under 1.5MB with headroom for Foundation additions

**First implementation priority:**
1. Update `custom_partitions.csv` to dual-OTA layout
2. Add `-D FW_VERSION=\"1.0.0\"` to platformio.ini build_flags
3. Expand ConfigManager with ScheduleConfig struct and 5 new NVS keys
4. Add `/api/ota/upload` handler in WebPortal
5. Add OTA self-check + rollback detection in main.cpp setup()
6. Verify firmware builds, OTA partitions are recognized, and existing functionality works

---

# Display System Release — Architecture Extension

_Extends the MVP + Foundation architecture with a pluggable display mode system, mode registry, Classic Card migration, Live Flight Card mode, and Mode Picker UI. All prior architectural decisions remain in effect — this section adds new decisions and updates existing ones where the Display System PRD introduces changes._

## Display System Release — Project Context Analysis

### Requirements Overview

**Functional Requirements (36 FRs across 8 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| DisplayMode Interface | FR1-FR4 | **Major:** New abstract interface with lifecycle semantics (init/render/teardown), memory reporting, rendering context with zone bounds and read-only brightness |
| Mode Registry & Lifecycle | FR5-FR10 | **Major:** New `core/ModeRegistry` component — static mode table, serialized switch execution, heap validation before activation, mode enumeration for API/UI |
| Classic Card Mode | FR11-FR13 | **High-risk migration:** Extract rendering logic from `NeoMatrixDisplay::renderZoneFlight()` and sub-methods (~140 lines) into standalone mode class with pixel-parity validation |
| Live Flight Card Mode | FR14-FR16 | **Medium:** New mode using existing telemetry fields (altitude_kft, speed_mph, track_deg, vertical_rate_fps) already in FlightInfo; adaptive field dropping for smaller panels |
| Mode Picker UI | FR17-FR26 | **Medium:** New dashboard section with schematic wireframe previews, activation control, transition state feedback, one-time upgrade notification |
| Mode Persistence | FR27-FR29 | **Minor:** Single NVS key (`display_mode`) for active mode; boot restoration with Classic Card default on first upgrade |
| Mode Switch API | FR30-FR33 | **Minor:** 2 new HTTP endpoints following existing `/api/*` JSON envelope pattern; transition state reporting |
| Display Infrastructure | FR34-FR36 | **Major refactoring:** NeoMatrixDisplay splits responsibilities — hardware ownership vs. rendering delegation; frame commit (`FastLED.show()`) separated from mode rendering |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| Mode switch latency | <2 seconds (user-perceived) | Full lifecycle (teardown → heap check → init → first render) within budget; cooperative scheduling on display task |
| Per-frame budget | <16ms, non-blocking | Mode render must complete within 60fps budget; no network I/O, no filesystem I/O on hot path |
| Mode Picker load | <1 second | Static mode metadata; no heap allocation for enumeration |
| Registry enumeration | <10ms, zero heap | Mode list served from BSS/data segment; no dynamic allocation for API response |
| Heap stability | No net loss after 100 switches | Fixed-size heap allocations per mode; sequential teardown → init with no overlapping allocations |
| Rapid-switch safety | 10 toggles in 3 seconds | Switch serialization via state enum in display task; queue/ignore during in-progress transition |
| Stack discipline | ≤512 bytes stack-local per render frame | Large buffers heap-allocated at mode init, reused across frames |
| Heap floor | ≥30KB free after mode activation | Heap guard validates before init; empirical measurement during integration |
| NVS namespace safety | No collision with Foundation keys | `display_mode` key uses existing `"flightwall"` namespace with verified unique prefix |
| Pixel parity | Classic Card identical to pre-migration output | GFX draw-call sequence comparison; visual verification across 5+ flight cards |
| Foundation compatibility | OTA, night mode, NTP, settings, health all unaffected | Modes isolated from all subsystems via read-only rendering context |
| Cooperative scheduling | No new FreeRTOS tasks for mode rendering | Mode render runs within existing display task on Core 0; no interrupt-driven frame push |
| Dashboard consistency | Mode Picker matches existing card/navigation patterns | Same CSS, same JS fetch patterns, no third-party script runtime additions |

### Scale & Complexity

- **Primary domain:** IoT/Embedded with companion web UI (unchanged)
- **Complexity level:** Medium-High — plugin architecture on memory-constrained MCU with cross-core coordination
- **New architectural components:** 4 (ModeRegistry, ClassicCardMode, LiveFlightCardMode, DisplayMode interface)
- **New directory:** `firmware/modes/` for mode implementations
- **Updated components:** NeoMatrixDisplay (responsibility split), WebPortal (2 new endpoints), main.cpp (display task integration), dashboard HTML/JS (Mode Picker UI)
- **Existing components unchanged:** FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, WiFiManager, ConfigManager (no new keys except `display_mode`), LayoutEngine, LogoManager, SystemStatus

### Technical Constraints (Display System–Specific)

| Constraint | Impact |
|-----------|--------|
| ESP32 non-compacting heap allocator | Repeated mode switches risk heap fragmentation; modes must use fixed-size allocations and free all memory during teardown |
| Single display task on Core 0 | Mode rendering, switch execution, and lifecycle management all run cooperatively on one task — no parallel mode operations |
| Cross-core API boundary | Mode switch requests arrive from WebPortal (Core 1 async TCP) but execute on display task (Core 0) — requires atomic flag coordination |
| `FastLED.show()` disables interrupts | Frame commit must remain in display task after render returns; modes must NOT call `FastLED.show()` directly |
| 8KB display task stack | Per-frame rendering must stay within stack budget; large buffers allocated at mode init on heap |
| Shared logo buffer (2KB) | `_logoBuffer[1024]` in NeoMatrixDisplay is reused across frames; modes receive it via rendering context, not owned per-mode |
| Existing calibration/positioning mode flags | These are primitive boolean checks in the display task, NOT the template for the mode system — DisplayMode requires proper lifecycle semantics |

### Cross-Cutting Concerns (Display System Additions)

1. **Rendering context as isolation boundary** — Modes receive a bundled struct containing: matrix reference (GFX primitives), zone bounds (LayoutResult), pre-computed text color, brightness (read-only), flight data reference, shared logo buffer reference, and display cycle timing. Modes have zero dependency on ConfigManager, WiFiManager, or any other subsystem. This isolation is the architectural guarantee for NFR C2 (Foundation compatibility).

2. **Cross-core mode switch coordination** — Browser → POST API → WebPortal sets atomic switch request flag → display task checks flag at top of loop iteration (before any render call) → executes teardown/init → clears flag. Mirrors the existing `g_configChanged` atomic pattern. No new concurrency primitives, no mutexes, no FreeRTOS queues for mode switching.

3. **NeoMatrixDisplay responsibility split** — Post-refactoring, NeoMatrixDisplay owns: (a) hardware initialization (LED strip, matrix object, brightness), (b) frame commit (`FastLED.show()`), (c) shared resources (logo buffer, matrix dimensions), (d) emergency fallback renderer (`displaySingleFlightCard()` retained for FR36 — ensures a valid rendering path even when no mode can init). Rendering logic migrates to mode classes. Display task orchestrates: `registry.getActiveMode()->render(context)` then `display.show()`.

4. **Heap lifecycle management** — Only one mode active at any time. During switch: outgoing mode's `teardown()` frees all heap → registry measures `ESP.getFreeHeap()` → compares against incoming mode's `getMemoryRequirement()` → if sufficient, calls `init()` → if insufficient, re-initializes previous mode and returns error. No overlapping mode allocations. 100-switch stability validated empirically.

5. **Flight cycling state migration** — `_currentFlightIndex` and `_lastCycleMs` currently live in NeoMatrixDisplay and the display task. Post-extraction, each mode owns its own cycling state (index + timer). The rendering context passes the full flight vector; modes decide how to cycle. This costs ~12 bytes per mode (trivial for `getMemoryRequirement()`).

6. **Zone descriptor metadata for Mode Picker** — Each mode exposes static metadata describing its zone layout (grid dimensions, zone labels, row/col positions, spans). The GET API returns this alongside mode name/ID. Dashboard JS renders schematic wireframe previews client-side using CSS Grid with `<div>` elements — no canvas, no SVG, no server-rendered bitmaps. Zone layout is fully data-driven from each mode's `getZoneLayout()` metadata; new modes get schematics automatically with zero UI code changes.

7. **Upgrade notification via dual-source dismissal** — The one-time "New display modes available" banner (FR25-FR26) uses dual-source dismissal for robustness: firmware returns an `upgrade_notification` flag in the `GET /api/display/modes` response (set when no prior `display_mode` NVS key exists on first boot), AND browser checks `localStorage.getItem('flightwall_mode_notif_seen')`. Banner shows only when both conditions are true (API flag true AND localStorage not set). Dismissal clears both: `localStorage.setItem()` on client + `POST /api/display/ack-notification` to clear the firmware flag. Either source being cleared independently prevents reappearance.

8. **Heap baseline measurement prerequisite** — Before any Display System code is written, measure free heap after full Foundation Release boot (WiFi connected, web server active, NTP synced, flight data in queue). This establishes the budget ceiling for mode `getMemoryRequirement()` values and validates whether the 30KB floor (NFR S4) is achievable.

### Existing Architecture Unchanged

The following MVP + Foundation decisions remain in full effect with no modifications:

- Hexagonal (Ports & Adapters) pattern
- ConfigManager singleton with category struct getters (no new config structs — only one new NVS key)
- Inter-task communication (atomic flags for config, FreeRTOS queue for flights)
- WiFi state machine (WiFiManager)
- SystemStatus error registry
- FreeRTOS dual-core task pinning (display Core 0, everything else Core 1)
- All naming conventions, NVS key patterns, logging macros, and enforcement rules (1-16)
- Web asset patterns (fetch + json.ok + showToast)
- OTA self-check, rollback detection, night mode scheduler
- Flash partition layout (unchanged from Foundation)

## Display System Release — Technology Stack

**Starter Template Evaluation: N/A (Brownfield Extension)**

No new external dependencies. All Display System capabilities use the existing ESP32 Arduino framework and libraries already in the technology stack:

| Component | Implementation | Source |
|-----------|---------------|--------|
| DisplayMode interface | Abstract C++ class | New firmware code |
| ModeRegistry | Static C++ class | New firmware code |
| ClassicCardMode | C++ class (extracted from NeoMatrixDisplay) | New firmware code |
| LiveFlightCardMode | C++ class | New firmware code |
| Mode Switch API | ESPAsyncWebServer routes | Existing dependency |
| Mode Picker UI | HTML/JS/CSS in dashboard | Existing web asset pattern |
| NVS persistence | ESP32 Preferences API | Existing dependency |
| Rendering primitives | Adafruit GFX via FastLED_NeoMatrix | Existing dependency |

The MVP + Foundation dependency stack (ESPAsyncWebServer Carbou fork, AsyncTCP, FastLED, Adafruit GFX, FastLED NeoMatrix, ArduinoJson, LittleFS, ESPmDNS, Update.h, esp_ota_ops.h, time.h/configTzTime) remains unchanged.

## Display System Release — Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. D1 — DisplayMode interface with RenderContext (every mode and the registry depend on this contract)
2. D2 — ModeRegistry with static table, cooperative switch serialization, heap guard
3. D3 — NeoMatrixDisplay responsibility split (rendering extracted, hardware retained)
4. D4 — Display task integration via ModeRegistry::tick() + atomic flag

**Important Decisions (Shape Architecture):**
5. D5 — Mode Switch API endpoints (GET list + POST activate)
6. D6 — NVS persistence for active mode selection

**Supporting Decisions:**
7. D7 — Mode Picker UI with client-side wireframes and localStorage notification

**Deferred Decisions (Post-Display System):**
- Mode-specific user settings (per-mode NVS keys and UI panels)
- Animated transitions (blank transition only in this release)
- Mode scheduling (auto-switch by time of day)
- Server-rendered preview thumbnails

### Decision D1: DisplayMode Interface — Abstract Class with RenderContext

**The central abstraction. Every mode implements this contract.**

```cpp
// Forward declarations
struct RenderContext;
struct FlightInfo;
struct ModeZoneDescriptor;

class DisplayMode {
public:
    virtual ~DisplayMode() = default;
    virtual bool init(const RenderContext& ctx) = 0;
    virtual void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) = 0;
    virtual void teardown() = 0;
    virtual const char* getName() const = 0;
    virtual const ModeZoneDescriptor& getZoneDescriptor() const = 0;
};
```

**Note:** `getMemoryRequirement()` is intentionally NOT in the virtual interface. Memory requirement is a static property of the mode class, needed BEFORE instantiation for the heap guard. It is exposed via a static function pointer in `ModeEntry` (see D2). This avoids a throwaway allocation just to query memory needs.

**Rendering context — bundles everything a mode needs, nothing it shouldn't have:**

```cpp
struct RenderContext {
    Adafruit_NeoMatrix* matrix;     // GFX drawing primitives (write to buffer only)
    LayoutResult layout;             // zone bounds (logo, flight, telemetry)
    uint16_t textColor;              // pre-computed from DisplayConfig
    uint8_t brightness;              // read-only — managed by night mode scheduler
    uint16_t* logoBuffer;            // shared 2KB buffer from NeoMatrixDisplay
    uint16_t displayCycleMs;         // cycle interval for modes that rotate flights
};
```

**Zone descriptor — static metadata for Mode Picker UI wireframes:**

```cpp
struct ZoneRegion {
    const char* label;    // e.g., "Airline", "Route", "Altitude"
    uint8_t relX, relY;   // relative position within mode's canvas (0-100%)
    uint8_t relW, relH;   // relative dimensions (0-100%)
};

struct ModeZoneDescriptor {
    const char* description;        // human-readable mode description
    const ZoneRegion* regions;      // static array of labeled regions
    uint8_t regionCount;
};
```

**Key design choices:**
- `RenderContext` passed by const reference — modes cannot modify shared state
- No ConfigManager dependency in modes — text color and timing pre-computed by caller
- `logoBuffer` is a shared resource owned by NeoMatrixDisplay, not per-mode (avoids doubling 2KB heap usage)
- `getZoneDescriptor()` returns static data (PROGMEM-safe) — zero heap for API serialization
- `init()` returns bool — false means mode failed to initialize (heap allocation failure or resource unavailable)
- **Modes own their flight cycling state** — `_currentFlightIndex` and `_lastCycleMs` migrate from NeoMatrixDisplay into each mode's private members (~12 bytes per mode)
- **Empty flight vector is valid input to `render()`** — modes decide their own idle/loading state (ClassicCardMode shows "...", LiveFlightCardMode may show a different idle display)
- **Modes must NOT call `FastLED.show()`** — they write to the pixel buffer only; frame commit is the display task's responsibility

### Decision D2: ModeRegistry — Static Table with Cooperative Switch Serialization

```cpp
enum class SwitchState : uint8_t {
    IDLE,
    REQUESTED,
    SWITCHING
};

struct ModeEntry {
    const char* id;                           // e.g., "classic_card"
    const char* displayName;                  // e.g., "Classic Card"
    DisplayMode* (*factory)();                // factory function returning new instance
    uint32_t (*memoryRequirement)();          // static function — no instance needed
    uint8_t priority;                         // display order in Mode Picker
};

class ModeRegistry {
public:
    static void init(const ModeEntry* table, uint8_t count);
    static bool requestSwitch(const char* modeId);    // called from Core 1 (API)
    static void tick(const RenderContext& ctx,         // called from display task (Core 0)
                     const std::vector<FlightInfo>& flights);
    static DisplayMode* getActiveMode();
    static const char* getActiveModeId();
    static const ModeEntry* getModeTable();
    static uint8_t getModeCount();
    static SwitchState getSwitchState();
    static const char* getLastError();

private:
    static const ModeEntry* _table;
    static uint8_t _count;
    static DisplayMode* _activeMode;
    static uint8_t _activeModeIndex;
    static std::atomic<uint8_t> _requestedIndex;   // cross-core: Core 1 writes, Core 0 reads
    static SwitchState _switchState;
    static char _lastError[64];
    static bool _nvsWritePending;                   // debounced NVS persistence
    static unsigned long _lastSwitchMs;
};
```

**Static allocation:** `_table` is a pointer to a const array in BSS/data (defined in main.cpp). `_activeMode` is the only heap pointer — the single active mode instance. `_requestedIndex` is atomic for cross-core safety. `_lastError` is a fixed-size char array — no String allocation.

**Switch flow (inside `tick()`, runs on Core 0):**

1. Check `_requestedIndex` — if different from `_activeModeIndex`, begin switch
2. Set `_switchState = SWITCHING`
3. Call `_activeMode->teardown()` — frees mode's internal heap buffers, but **do NOT delete the mode object yet** (the ~32-64 byte shell remains for safe restoration)
4. Measure `ESP.getFreeHeap()`
5. Call `_table[requestedIndex].memoryRequirement()` — static function, no instantiation
6. **If heap sufficient:**
   - `delete _activeMode` — free the old shell
   - Create new mode via `_table[requestedIndex].factory()`
   - Call `init(ctx)` on new mode
   - If `init()` succeeds: update `_activeModeIndex`, set `_nvsWritePending = true`, clear error
   - If `init()` fails: `delete` the new mode, re-create previous mode via factory, call `init(ctx)`, set error "Mode initialization failed"
7. **If heap insufficient:**
   - Call `_activeMode->init(ctx)` to re-initialize the still-allocated previous mode
   - Set error "Insufficient memory for [mode name]"
8. Set `_switchState = IDLE`
9. **NVS debounce:** If `_nvsWritePending` and `millis() - _lastSwitchMs > 2000`, write to NVS via `ConfigManager::setDisplayMode()`, clear flag

**Key design choices:**
- **Teardown-before-delete pattern** — previous mode object shell stays allocated until new mode is confirmed, enabling safe restoration without re-allocation
- **`init()` failure treated identically to heap guard failure** — restore previous mode, set error, propagate to API
- **`tick()` is allowed to exceed the 16ms frame budget during SWITCHING state** — the wall goes briefly blank (fillScreen(0) during teardown), consistent with the PRD's "brief blank transition" design. This is a documented accepted tradeoff, not a bug
- **NVS write debounced** — 2-second quiet period after last switch, same pattern as ConfigManager's `schedulePersist()`. Prevents 10 NVS writes during rapid-switch stress test (NFR S2)

### Decision D3: NeoMatrixDisplay Refactoring — Responsibility Split

**Post-refactoring, NeoMatrixDisplay retains three roles:**

1. **Hardware owner** — LED strip initialization, matrix object creation, brightness control
2. **Frame committer** — `show()` method wrapping `FastLED.show()`, called by display task after mode render
3. **Shared resource provider** — logo buffer, matrix reference, dimensions, layout result

**What gets extracted from NeoMatrixDisplay:**
- `renderZoneFlight()`, `renderFlightZone()`, `renderTelemetryZone()`, `renderLogoZone()` → `ClassicCardMode`
- `_currentFlightIndex`, `_lastCycleMs` → per-mode internal state
- `displayFlights()` → no longer called from display task (see BaseDisplay note below)

**What stays in NeoMatrixDisplay:**
- Hardware lifecycle: `initialize()`, `clear()`, `rebuildMatrix()`, `reconfigureFromConfig()`
- Brightness control: `updateBrightness()`
- Frame commit: new `show()` method
- Rendering context factory: new `buildRenderContext()` method
- Status display: `displayMessage()`, `showLoading()`, `displayLoadingScreen()`
- Emergency fallback: `displaySingleFlightCard()` retained for FR36 (no-mode-active safety net)
- Calibration/positioning: `setCalibrationMode()`, `setPositioningMode()`, `renderCalibrationPattern()`, `renderPositioningPattern()`
- Shared resources: `_logoBuffer[1024]`, `_matrix`, `_layout`
- Helper methods: `drawTextLine()`, `truncateToColumns()`, `drawBitmapRGB565()`, `formatTelemetryValue()` — these are rendering utilities. Modes that need them can either reimplement or we extract them into a shared `utils/DisplayUtils.h`

**New public API additions:**

```cpp
// Rendering context factory — assembles context from NeoMatrixDisplay internals
RenderContext buildRenderContext() const;

// Frame commit — separated from rendering
void show();  // wraps FastLED.show()

// Emergency fallback (FR36 — ensures valid display when no mode can init)
void displayFallbackCard(const std::vector<FlightInfo>& flights);
```

**`buildRenderContext()` implementation:**

```cpp
RenderContext NeoMatrixDisplay::buildRenderContext() const {
    DisplayConfig disp = ConfigManager::getDisplay();
    TimingConfig timing = ConfigManager::getTiming();
    return {
        _matrix,
        _layout,
        _matrix->Color(disp.text_color_r, disp.text_color_g, disp.text_color_b),
        disp.brightness,
        const_cast<uint16_t*>(_logoBuffer),
        static_cast<uint16_t>(timing.display_cycle * 1000)
    };
}
```

**Optimization:** The display task caches the `RenderContext` and only rebuilds it when `g_configChanged` fires. Saves ~20 ConfigManager reads per second at 20fps with zero behavioral difference.

**BaseDisplay interface — retained, not deprecated:**

`BaseDisplay` defines `initialize()`, `clear()`, and `displayFlights()`. NeoMatrixDisplay continues to implement `initialize()` and `clear()` (called from `main.cpp setup()`). `displayFlights()` remains implemented but is no longer called from the display task — the task uses `ModeRegistry::tick()` + `show()` instead. No breaking interface change needed. The method can route to `displayFallbackCard()` internally for backward compatibility.

### Decision D4: Display Task Integration — Cached Context + ModeRegistry::tick()

**New globals in main.cpp:**

```cpp
// Static mode table — built at compile time, stored in BSS/data
static DisplayMode* classicCardFactory() { return new ClassicCardMode(); }
static DisplayMode* liveFlightCardFactory() { return new LiveFlightCardMode(); }
static uint32_t classicCardMemReq() { return ClassicCardMode::MEMORY_REQUIREMENT; }
static uint32_t liveFlightCardMemReq() { return LiveFlightCardMode::MEMORY_REQUIREMENT; }

static const ModeEntry MODE_TABLE[] = {
    { "classic_card", "Classic Card", classicCardFactory, classicCardMemReq, 0 },
    { "live_flight",  "Live Flight Card", liveFlightCardFactory, liveFlightCardMemReq, 1 },
};
static constexpr uint8_t MODE_COUNT = sizeof(MODE_TABLE) / sizeof(MODE_TABLE[0]);
```

**In `setup()`:**

```cpp
ModeRegistry::init(MODE_TABLE, MODE_COUNT);
// Restore last active mode from NVS, default to "classic_card"
String savedMode = ConfigManager::getDisplayMode();
if (!ModeRegistry::requestSwitch(savedMode.c_str())) {
    ModeRegistry::requestSwitch("classic_card");
}
```

**In `displayTask()` — revised rendering block:**

```cpp
// Cache rendering context — rebuild only on config change
static RenderContext cachedCtx;
static bool ctxInitialized = false;

if (g_configChanged.exchange(false) || !ctxInitialized) {
    // ... existing config change handling (brightness, hardware) ...
    cachedCtx = g_display.buildRenderContext();
    ctxInitialized = true;
}

// Priority order (unchanged from existing):
// 1. Calibration mode
// 2. Positioning mode
// 3. Status messages
// 4. Mode rendering (NEW — replaces direct renderFlight call)

if (g_display.isCalibrationMode()) {
    g_display.renderCalibrationPattern();
} else if (g_display.isPositioningMode()) {
    g_display.renderPositioningPattern();
} else if (statusMessageVisible) {
    // ... existing status message handling ...
} else {
    // Read latest flight data from queue
    FlightDisplayData *ptr = nullptr;
    std::vector<FlightInfo> empty;
    const auto& flights = (g_flightQueue && xQueuePeek(g_flightQueue, &ptr, 0) == pdTRUE && ptr)
                          ? ptr->flights : empty;

    // Tick mode registry — handles switch requests + renders active mode
    ModeRegistry::tick(cachedCtx, flights);
}

// Frame commit — always last, regardless of which path rendered
g_display.show();
```

**Cross-core coordination pattern:**

```
Browser → POST /api/display/mode → WebPortal handler (Core 1, async TCP task)
    → ModeRegistry::requestSwitch(modeId) → sets _requestedIndex atomic
Display task (Core 0) → ModeRegistry::tick() checks _requestedIndex at top of call
    → If changed: executes full switch lifecycle within tick()
    → Sets _switchState for API polling
```

This mirrors the existing `g_configChanged` pattern — no new concurrency primitives, no mutexes, no FreeRTOS queues for mode switching.

### Decision D5: Mode Switch API — Two Endpoints

Following existing `/api/*` JSON envelope pattern. Uses nested path `/api/display/` to scope mode operations (new pattern, but logically groups display-related endpoints for future extensibility).

**`GET /api/display/modes`** — list available modes + active mode + transition state:

```json
{
    "ok": true,
    "data": {
        "active": "live_flight",
        "switch_state": "idle",
        "modes": [
            {
                "id": "classic_card",
                "name": "Classic Card",
                "description": "Three-line flight card: airline, route, aircraft",
                "zones": [
                    { "label": "Logo", "x": 0, "y": 0, "w": 20, "h": 100 },
                    { "label": "Airline", "x": 20, "y": 0, "w": 80, "h": 33 },
                    { "label": "Route", "x": 20, "y": 33, "w": 80, "h": 33 },
                    { "label": "Aircraft", "x": 20, "y": 66, "w": 80, "h": 34 }
                ]
            },
            {
                "id": "live_flight",
                "name": "Live Flight Card",
                "description": "Enriched telemetry: altitude, speed, heading, vertical rate",
                "zones": [
                    { "label": "Logo", "x": 0, "y": 0, "w": 20, "h": 100 },
                    { "label": "Airline", "x": 20, "y": 0, "w": 40, "h": 50 },
                    { "label": "Route", "x": 60, "y": 0, "w": 40, "h": 50 },
                    { "label": "Alt", "x": 20, "y": 50, "w": 20, "h": 50 },
                    { "label": "Spd", "x": 40, "y": 50, "w": 20, "h": 50 },
                    { "label": "Hdg", "x": 60, "y": 50, "w": 20, "h": 50 },
                    { "label": "VRate", "x": 80, "y": 50, "w": 20, "h": 50 }
                ]
            }
        ]
    }
}
```

**`POST /api/display/mode`** — request mode switch:

Request: `{ "mode": "live_flight" }`

Success: `{ "ok": true, "data": { "switching_to": "live_flight" } }`

Error (heap guard): `{ "ok": false, "error": "Insufficient memory for Live Flight Card", "code": "HEAP_INSUFFICIENT" }`

Error (unknown mode): `{ "ok": false, "error": "Unknown mode: radar_sweep", "code": "MODE_NOT_FOUND" }`

Error (in progress): `{ "ok": false, "error": "Mode switch already in progress", "code": "SWITCH_IN_PROGRESS" }`

**Synchronous mode activation:** The POST endpoint blocks until the mode switch is complete (new mode rendering) or fails (heap guard, unknown mode). The response contains the confirmed active mode — no client-side polling needed. "Switching..." in the UI is the POST in flight; "Active" is the 200 response. This gives ESPHome-grade state honesty with zero client-side complexity. Sub-2-second switch means the POST typically returns in under 2 seconds.

**NFR P4 compliance:** `GET /api/display/modes` serializes the static mode table (BSS/data) + reads atomic `_activeModeIndex` and `_switchState`. Zero heap allocation. Completes in <10ms.

### Decision D6: NVS Persistence — Single Key with Debounced Write

**NVS key:** `display_mode` (12 chars, within 15-char limit)
**Namespace:** `"flightwall"` (existing — verified non-colliding with all 23+ existing keys per NFR S6)
**Type:** String (mode ID, e.g., `"classic_card"`, `"live_flight"`)
**Default:** `"classic_card"` (preserves pre-upgrade behavior per FR29)

**Persistence timing:** Debounced — `ModeRegistry::tick()` sets `_nvsWritePending` flag after successful switch. Write occurs when flag is set AND >2 seconds have elapsed since last switch. This prevents 10 NVS writes during rapid-switch stress testing (NFR S2) while ensuring the selection persists for power-cycle recovery (FR27-FR28).

**ConfigManager integration:**

```cpp
// New in ConfigManager
static String getDisplayMode();          // reads "display_mode" from NVS, returns "classic_card" if absent
static void setDisplayMode(const String& modeId);  // writes "display_mode" to NVS
```

Two new methods, no new config struct. The `display_mode` key is NOT hot-reload and NOT reboot-required — it's a fire-and-forget debounced write after each successful switch.

**Boot restoration:** In `setup()`, `ConfigManager::getDisplayMode()` reads from NVS → passed to `ModeRegistry::requestSwitch()`. If key doesn't exist (first boot after upgrade from Foundation Release), returns `"classic_card"` default.

### Decision D7: Mode Picker UI — Dashboard Section with Client-Side Wireframes

**Dashboard integration:**
- New section in `dashboard.html` positioned #2 in card order (after Display, before Timing) — "remote control" cadence demands top-of-fold placement
- Loads mode list via `GET /api/display/modes` on page load
- Renders mode cards with schematic wireframe previews from zone metadata
- Entire non-active mode card is the tap target (not a button within the card — Philips Hue scene picker pattern)
- Tap triggers synchronous `POST /api/display/mode` — returns after mode is rendering or fails

**Wireframe rendering:** CSS Grid with `<div>` elements, data-driven from zone metadata JSON. Each mode's API response includes `grid` (rows/cols) and `zones` array (label, row, col, rowSpan, colSpan). JS creates a grid container and positions zone divs via `grid-row`/`grid-column`. ~5 lines of rendering JS. No canvas, no SVG, no server-rendered bitmaps. New modes get schematics automatically — zero UI code changes.

**Transition feedback:**
- Tap non-active card → card enters "Switching..." state (pulsing border, dimmed, `.switching` CSS class). All sibling cards enter `.disabled` state (opacity 0.5, `pointer-events: none`).
- Synchronous POST returns → on success: new card gets `.active`, previous card returns to idle, siblings re-enabled. On error: tapped card shows scoped `--warning` error message, previous card remains `.active`, siblings re-enabled.
- No polling — "Switching..." is the POST in flight, "Active" is the 200 response.

**Upgrade notification (FR25-FR26):**
- Dual-source check: API returns `upgrade_notification: true` in `GET /api/display/modes` response AND browser checks `localStorage.getItem('flightwall_mode_notif_seen')`
- If both conditions met: show informational banner with "Try Live Flight Card" (primary action — activates mode immediately) + "Browse Modes" (scrolls to Mode Picker) + dismiss X
- All three actions clear both sources: `localStorage.setItem('flightwall_mode_notif_seen', '1')` + `POST /api/display/ack-notification` to clear firmware flag
- Either source cleared independently prevents reappearance

**Files affected:**
- `firmware/data-src/dashboard.html` — Mode Picker section container (`<div id="mode-picker">`) + notification banner markup
- `firmware/data-src/dashboard.js` — mode API calls, CSS Grid schematic rendering, card state management, notification logic (~40 lines total)
- `firmware/data-src/style.css` — mode card states (idle/switching/active/disabled/error), schematic zone styling, notification banner (~58 lines total)
- `firmware/data/*.gz` — rebuilt gzipped copies of all updated web assets

### Display System Decision Impact — Implementation Sequence

**Critical path (sequential):**
1. **DisplayMode interface + RenderContext** (`interfaces/DisplayMode.h`) — everything depends on this contract
2. **ModeRegistry** (`core/ModeRegistry.h/.cpp`) — switch logic, heap guard, tick()
3. **ClassicCardMode** (`modes/ClassicCardMode.h/.cpp`) — extraction from NeoMatrixDisplay with pixel-parity validation (highest-risk step)
4. **NeoMatrixDisplay refactoring** — remove extracted methods, add `show()` + `buildRenderContext()`, retain fallback
5. **Display task integration** (`main.cpp`) — replace renderFlight with ModeRegistry::tick() + show()
6. **LiveFlightCardMode** (`modes/LiveFlightCardMode.h/.cpp`) — second mode, validates the abstraction works

**Parallel with steps 3-6:**
7. **Mode Switch API** (`WebPortal.cpp`) — GET + POST endpoints, can be stubbed early
8. **NVS persistence** (`ConfigManager.h/.cpp`) — `getDisplayMode()` / `setDisplayMode()`, trivial addition

**After all firmware complete:**
9. **Mode Picker UI** (`dashboard.html/js/css`) — depends on working API endpoints
10. **Gzip rebuild** — all updated web assets

**Heap baseline measurement** must occur before step 1 — measure `ESP.getFreeHeap()` after full Foundation boot to establish the mode memory budget.

### Cross-Decision Dependencies

```
DisplayMode interface (D1)
    └── ModeRegistry (D2) — uses DisplayMode*, ModeEntry, RenderContext
        ├── ClassicCardMode — implements DisplayMode
        ├── LiveFlightCardMode — implements DisplayMode
        └── Display task (D4) — calls ModeRegistry::tick()
            └── NeoMatrixDisplay (D3) — provides RenderContext + show()

Mode Switch API (D5) — calls ModeRegistry::requestSwitch() + reads state
    └── Mode Picker UI (D7) — calls API, renders wireframes from zone descriptors

NVS persistence (D6) — called from ModeRegistry::tick() after successful switch
    └── Boot restoration in setup() — reads NVS, calls requestSwitch()
```

## Display System Release — Implementation Patterns & Consistency Rules

### Display System–Specific Conflict Points

The MVP architecture established 11 enforcement rules. The Foundation Release added 5 (rules 12-16). The Display System Release introduces 7 new pattern areas where AI agents could make inconsistent choices about the mode system.

### Mode Implementation Pattern

**File location:** All mode classes go in `firmware/modes/`. One `.h/.cpp` pair per mode. Mode files are NOT adapters (they don't touch hardware directly) and NOT core (they're pluggable rendering strategies).

```
firmware/modes/
├── ClassicCardMode.h
├── ClassicCardMode.cpp
├── LiveFlightCardMode.h
└── LiveFlightCardMode.cpp
```

**Class structure:**

```cpp
// modes/ClassicCardMode.h
#pragma once
#include "interfaces/DisplayMode.h"

class ClassicCardMode : public DisplayMode {
public:
    static constexpr uint32_t MEMORY_REQUIREMENT = 64;  // bytes — cycling state only

    bool init(const RenderContext& ctx) override;
    void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) override;
    void teardown() override;
    const char* getName() const override;
    const ModeZoneDescriptor& getZoneDescriptor() const override;

private:
    // Per-mode cycling state (migrated from NeoMatrixDisplay)
    size_t _currentFlightIndex = 0;
    unsigned long _lastCycleMs = 0;
};
```

**Rules:**
- Constructor does NO heap allocation — all allocation happens in `init()`
- `teardown()` frees ALL heap allocated by `init()` — the mode object shell (~32-64 bytes) remains for safe restoration
- `MEMORY_REQUIREMENT` is a `static constexpr` — reflects worst-case heap allocated during `init()`, NOT the mode object size
- Modes store their own cycling state (index + timer) — do not rely on external cycling

### Rendering Context Discipline

**Modes must ONLY access data through `RenderContext`.** This is the critical isolation boundary that guarantees NFR C2 (Foundation feature compatibility).

**Allowed:**
```cpp
void ClassicCardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    ctx.matrix->setCursor(x, y);                                       // ✅ GFX primitives via context
    ctx.matrix->setTextColor(ctx.textColor);                           // ✅ pre-computed color from context
    LogoManager::loadLogo(f.operator_icao, ctx.logoBuffer);            // ✅ shared buffer from context
    if (millis() - _lastCycleMs >= ctx.displayCycleMs) { }             // ✅ timing from context
    DisplayUtils::drawTextLine(ctx.matrix, x, y, text, ctx.textColor); // ✅ shared utility
}
```

**Forbidden:**
```cpp
void ClassicCardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    DisplayConfig disp = ConfigManager::getDisplay();  // ❌ NEVER read ConfigManager from modes
    FastLED.show();                                     // ❌ NEVER call FastLED.show() from modes
    ctx.matrix->setBrightness(50);                      // ❌ NEVER modify brightness from modes
    uint8_t brt = ConfigManager::getSchedule().dim_brightness;  // ❌ NEVER access schedule config
}
```

**Anti-pattern: caching RenderContext.** Modes receive `ctx` by const reference each call. Do not store a copy — the display task may update the cached context between frames (on config change). Always use the passed-in reference.

### Shared Rendering Utilities

Helper methods currently in NeoMatrixDisplay that modes need. Extract into `utils/DisplayUtils.h` as free functions:

```cpp
// utils/DisplayUtils.h
#pragma once
#include <Adafruit_NeoMatrix.h>
#include <Arduino.h>

namespace DisplayUtils {
    void drawTextLine(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                      const String& text, uint16_t color);
    String truncateToColumns(const String& text, int maxColumns);
    String formatTelemetryValue(double value, const char* suffix, int decimals = 0);
    void drawBitmapRGB565(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint16_t* bitmap,
                          uint16_t zoneW, uint16_t zoneH);
}
```

**Rule:** Modes use `DisplayUtils::` functions. Do NOT copy rendering helpers into each mode class. Do NOT add NeoMatrixDisplay as a dependency of modes.

### Mode Registration Pattern

**Adding a new mode requires exactly two touch points:**

1. Create `modes/NewMode.h/.cpp` implementing `DisplayMode`
2. Add one entry to `MODE_TABLE[]` in `main.cpp`:

```cpp
static DisplayMode* newModeFactory() { return new NewMode(); }
static uint32_t newModeMemReq() { return NewMode::MEMORY_REQUIREMENT; }

static const ModeEntry MODE_TABLE[] = {
    { "classic_card", "Classic Card", classicCardFactory, classicCardMemReq, 0 },
    { "live_flight",  "Live Flight Card", liveFlightCardFactory, liveFlightCardMemReq, 1 },
    { "new_mode",     "New Mode", newModeFactory, newModeMemReq, 2 },  // ← one line
};
```

**Anti-pattern:** Do NOT modify ModeRegistry, NeoMatrixDisplay, WebPortal, FlightDataFetcher, or dashboard JS to add a mode. If a mode addition requires changes beyond the two touch points above, the architecture has a defect.

### Mode Picker UI Patterns

**Mode list rendering:** Follow existing dashboard card pattern — same CSS classes, same card layout structure. Mode cards are rendered dynamically from the `GET /api/display/modes` JSON response. The HTML template contains only a `<div id="mode-picker">` container (~10 lines); JS builds cards from API data.

**API interaction:** Every `fetch()` call follows existing rule 10: check `json.ok`, call `showToast()` on failure. The POST is synchronous — it returns after the mode is rendering (or fails). No polling loop.

```javascript
async function activateMode(modeId) {
    const cards = document.querySelectorAll('.mode-card:not(.active)');
    const card = document.querySelector(`[data-mode="${modeId}"]`);
    card.classList.add('switching');
    cards.forEach(c => { if (c !== card) c.classList.add('disabled'); });
    try {
        const res = await fetch('/api/display/mode', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mode: modeId })
        });
        const json = await res.json();
        if (!json.ok) {
            card.classList.remove('switching');
            showCardError(card, json.error);
        } else {
            document.querySelector('.mode-card.active').classList.remove('active');
            card.classList.remove('switching');
            card.classList.add('active');
        }
    } catch (err) {
        card.classList.remove('switching');
        showToast('Connection failed', 'error');
    }
    cards.forEach(c => c.classList.remove('disabled'));
}
```

**Wireframe rendering:** CSS Grid with `<div>` zone elements. Grid dimensions and zone positions from API metadata. ~5 lines of JS to render. No canvas, no SVG, no external drawing libraries.

**Transition state:** Synchronous — card shows "Switching..." with `.switching` CSS class while POST is in flight. Sibling cards get `.disabled`. POST response resolves to "Active" or scoped error. No polling needed.

### Updated Enforcement Guidelines

**All AI agents implementing Display System stories MUST also follow (in addition to rules 1-16):**

17. Mode classes go in `firmware/modes/` — one `.h/.cpp` pair per mode
18. Modes must ONLY access data through `RenderContext` — zero `ConfigManager`, `WiFiManager`, or `SystemStatus` reads from inside a mode
19. Modes must NOT call `FastLED.show()` — frame commit is exclusively the display task's responsibility after `ModeRegistry::tick()` returns
20. All heap allocation in modes happens in `init()`, all deallocation in `teardown()` — constructors and per-frame `render()` must not allocate heap
21. Shared rendering helpers (`drawTextLine`, `truncateToColumns`, `formatTelemetryValue`, `drawBitmapRGB565`) must be called from `utils/DisplayUtils.h` — do not duplicate into mode classes
22. Adding a new mode requires exactly two touch points: new file in `modes/` + one line in `MODE_TABLE[]` in main.cpp — if more changes are needed, the architecture has a defect
23. `MEMORY_REQUIREMENT` is a `static constexpr` on each mode class — reflects worst-case heap from `init()`, not object size

## Display System Release — Project Structure & File Change Map

### Complete Project Directory Structure (Display System Additions)

```
firmware/
├── platformio.ini                          # UPDATED: build_src_filter + build_flags
├── custom_partitions.csv                   # UNCHANGED (Foundation layout)
│
├── interfaces/
│   ├── BaseDisplay.h                       # UNCHANGED (retained, not deprecated)
│   ├── BaseFlightFetcher.h                 # UNCHANGED
│   ├── BaseStateVectorFetcher.h            # UNCHANGED
│   └── DisplayMode.h                       # ← NEW: DisplayMode abstract class,
│                                           #         RenderContext, ModeZoneDescriptor,
│                                           #         ZoneRegion, SwitchState enum
│                                           #         Includes FlightInfo.h and LayoutEngine.h
│                                           #         directly (transitive for all modes)
│
├── core/
│   ├── ConfigManager.h                     # UPDATED: +getDisplayMode(), +setDisplayMode()
│   ├── ConfigManager.cpp                   # UPDATED: +getDisplayMode(), +setDisplayMode() impl
│   ├── ModeRegistry.h                      # ← NEW: ModeRegistry static class + ModeEntry struct
│   ├── ModeRegistry.cpp                    # ← NEW: init(), requestSwitch(), tick(),
│   │                                       #         switch lifecycle, NVS debounce
│   ├── FlightDataFetcher.h                 # UNCHANGED
│   ├── FlightDataFetcher.cpp               # UNCHANGED
│   ├── LayoutEngine.h                      # UNCHANGED
│   ├── LayoutEngine.cpp                    # UNCHANGED
│   ├── LogoManager.h                       # UNCHANGED
│   ├── LogoManager.cpp                     # UNCHANGED
│   ├── SystemStatus.h                      # UNCHANGED
│   └── SystemStatus.cpp                    # UNCHANGED
│
├── modes/                                  # ← NEW DIRECTORY
│   ├── ClassicCardMode.h                   # ← NEW: ClassicCardMode declaration
│   ├── ClassicCardMode.cpp                 # ← NEW: extracted from NeoMatrixDisplay
│   │                                       #         renderZoneFlight, renderFlightZone,
│   │                                       #         renderTelemetryZone, renderLogoZone
│   ├── LiveFlightCardMode.h                # ← NEW: LiveFlightCardMode declaration
│   └── LiveFlightCardMode.cpp              # ← NEW: telemetry-enriched flight card
│
├── adapters/
│   ├── NeoMatrixDisplay.h                  # UPDATED: +show(), +buildRenderContext(),
│   │                                       #          +displayFallbackCard()
│   │                                       #          -renderZoneFlight(), -renderFlightZone(),
│   │                                       #          -renderTelemetryZone(), -renderLogoZone()
│   ├── NeoMatrixDisplay.cpp                # UPDATED: ~140 lines extracted to ClassicCardMode;
│   │                                       #          +show(), +buildRenderContext(),
│   │                                       #          +displayFallbackCard() impls
│   ├── WebPortal.h                         # UPDATED: +_handleGetDisplayModes(),
│   │                                       #          +_handlePostDisplayMode()
│   ├── WebPortal.cpp                       # UPDATED: +GET /api/display/modes handler,
│   │                                       #          +POST /api/display/mode handler
│   ├── WiFiManager.h                       # UNCHANGED
│   ├── WiFiManager.cpp                     # UNCHANGED
│   ├── AeroAPIFetcher.h                    # UNCHANGED
│   ├── AeroAPIFetcher.cpp                  # UNCHANGED
│   ├── FlightWallFetcher.h                 # UNCHANGED
│   ├── FlightWallFetcher.cpp               # UNCHANGED
│   ├── OpenSkyFetcher.h                    # UNCHANGED
│   └── OpenSkyFetcher.cpp                  # UNCHANGED
│
├── utils/
│   ├── GeoUtils.h                          # UNCHANGED
│   ├── Log.h                               # UNCHANGED
│   ├── DisplayUtils.h                      # ← NEW: shared rendering free functions
│   └── DisplayUtils.cpp                    # ← NEW: drawTextLine, truncateToColumns,
│                                           #         formatTelemetryValue, drawBitmapRGB565
│
├── models/
│   ├── FlightInfo.h                        # UNCHANGED
│   ├── StateVector.h                       # UNCHANGED
│   └── AirportInfo.h                       # UNCHANGED
│
├── config/
│   ├── HardwareConfiguration.h             # UNCHANGED
│   ├── TimingConfiguration.h               # UNCHANGED
│   ├── WiFiConfiguration.h                 # UNCHANGED
│   ├── APIConfiguration.h                  # UNCHANGED
│   └── UserConfiguration.h                 # UNCHANGED
│
├── src/
│   └── main.cpp                            # UPDATED: +MODE_TABLE[], +factory functions,
│                                           #          +ModeRegistry::init() in setup(),
│                                           #          +heap baseline log after full init,
│                                           #          displayTask() replaces renderFlight
│                                           #          with ModeRegistry::tick()+show()
│
├── data-src/                               # Web asset sources
│   ├── dashboard.html                      # UPDATED: +Mode Picker section in nav + content
│   ├── dashboard.js                        # UPDATED: +mode API calls, +CSS Grid schematic
│   │                                       #          rendering, +card state mgmt, +notification
│   ├── style.css                           # UPDATED: +mode card styling
│   ├── health.html                         # UNCHANGED
│   ├── health.js                           # UNCHANGED
│   ├── common.js                           # UNCHANGED
│   ├── wizard.html                         # UNCHANGED
│   └── wizard.js                           # UNCHANGED
│
├── data/                                   # Gzipped assets (served by WebPortal)
│   ├── dashboard.html.gz                   # REBUILD after dashboard.html changes
│   ├── dashboard.js.gz                     # REBUILD after dashboard.js changes
│   ├── style.css.gz                        # REBUILD after style.css changes
│   ├── common.js.gz                        # UNCHANGED
│   ├── health.html.gz                      # UNCHANGED
│   ├── health.js.gz                        # UNCHANGED
│   ├── wizard.html.gz                      # UNCHANGED
│   ├── wizard.js.gz                        # UNCHANGED
│   ├── test.txt                            # UNCHANGED
│   └── logos/                              # UNCHANGED (99 ICAO .bin files)
│
└── test/
    ├── test_config_manager/test_main.cpp   # UNCHANGED (existing test)
    ├── test_layout_engine/test_main.cpp    # UNCHANGED (existing test)
    ├── test_logo_manager/test_main.cpp     # UNCHANGED (existing test)
    ├── test_telemetry_conversion/test_main.cpp  # UNCHANGED (existing test)
    └── test_mode_registry/test_main.cpp    # ← NEW: ModeRegistry switch lifecycle,
                                            #         heap guard, NVS debounce tests
```

### File Change Summary

| Action | Count | Files |
|--------|-------|-------|
| **NEW** | 10 | `interfaces/DisplayMode.h`, `core/ModeRegistry.h`, `core/ModeRegistry.cpp`, `modes/ClassicCardMode.h`, `modes/ClassicCardMode.cpp`, `modes/LiveFlightCardMode.h`, `modes/LiveFlightCardMode.cpp`, `utils/DisplayUtils.h`, `utils/DisplayUtils.cpp`, `test/test_mode_registry/test_main.cpp` |
| **UPDATED** | 12 | `platformio.ini`, `adapters/NeoMatrixDisplay.h`, `adapters/NeoMatrixDisplay.cpp`, `adapters/WebPortal.h`, `adapters/WebPortal.cpp`, `core/ConfigManager.h`, `core/ConfigManager.cpp`, `src/main.cpp`, `data-src/dashboard.html`, `data-src/dashboard.js`, `data-src/style.css`, `data/*.gz` (3 rebuilds) |
| **UNCHANGED** | 30+ | All other firmware files, config headers, models, fetchers, WiFiManager, LayoutEngine, LogoManager, SystemStatus, wizard assets, health assets |
| **NEW DIRECTORY** | 1 | `firmware/modes/` |

### PlatformIO Build Configuration Update

**Current `platformio.ini` `build_src_filter`:**

```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
```

**Required additions:**

```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
    +<../modes/*.cpp>          # ← NEW: ClassicCardMode, LiveFlightCardMode
    +<../utils/*.cpp>          # ← NEW: DisplayUtils
```

**Required `build_flags` addition:**

```ini
build_flags =
    -I .
    -I include
    -I src
    -I adapters
    -I core
    -I models
    -I interfaces
    -I utils
    -I config
    -I modes                   # ← NEW: mode headers
    -D LOG_LEVEL=3
```

Without these changes, mode and utility `.cpp` files will not compile. This was identified as a **blocker** during party mode review.

### Architectural Boundaries

**API Boundaries (HTTP):**

| Endpoint | Method | Handler | Cross-Core | Purpose |
|----------|--------|---------|------------|---------|
| `/api/display/modes` | GET | WebPortal (Core 1) | Read-only atomic | List modes, active mode, switch state |
| `/api/display/mode` | POST | WebPortal (Core 1) | Atomic write | Request mode switch |
| `/api/settings` | GET/POST | WebPortal (Core 1) | Unchanged | Existing settings (no new keys) |
| `/api/status` | GET | WebPortal (Core 1) | Unchanged | Health/status |

**Component Boundaries (Firmware):**

```
┌──────────────────────────────────────────────────────────────┐
│  Core 1 (WiFi/Web/API task)                                  │
│  ┌────────────────┐                                          │
│  │   WebPortal    │─── POST /api/display/mode ──────────┐   │
│  │  (HTTP handler)│─── GET /api/display/modes (read) ─┐  │   │
│  └────────────────┘                                   │  │   │
└───────────────────────────────────────────────────────┼──┼───┘
                                                        │  │
    atomic read ────────────────────────────────────────┘  │
    atomic write (requestSwitch) ──────────────────────────┘
                                                        │  │
┌───────────────────────────────────────────────────────┼──┼───┐
│  Core 0 (Display task)                                │  │   │
│  ┌────────────────┐   ┌──────────────┐                │  │   │
│  │  ModeRegistry  │──→│ ActiveMode   │ (DisplayMode*) │  │   │
│  │  ::tick()      │   │ .render()    │                │  │   │
│  └───────┬────────┘   └──────┬───────┘                │  │   │
│          │                   │                                │
│          │           ┌───────▼────────┐                       │
│          │           │  LogoManager   │ (static utility —     │
│          │           │  ::loadLogo()  │  accepted dependency) │
│          │           └────────────────┘                       │
│  ┌───────▼──────────────┐   ┌────────────────────────┐       │
│  │  NeoMatrixDisplay    │   │  DisplayUtils          │       │
│  │  .buildRenderContext()│   │  (free functions)      │       │
│  │  .show()             │   │  drawTextLine, etc.    │       │
│  └──────────────────────┘   └────────────────────────┘       │
└──────────────────────────────────────────────────────────────┘
```

**Data Boundaries:**

| Boundary | Direction | Mechanism |
|----------|-----------|-----------|
| WebPortal → ModeRegistry | Core 1 → Core 0 | `std::atomic<uint8_t> _requestedIndex` |
| ModeRegistry → ConfigManager | Core 0 → NVS | `setDisplayMode()` (debounced 2s) |
| ConfigManager → ModeRegistry | NVS → Core 0 | `getDisplayMode()` at boot only |
| NeoMatrixDisplay → Modes | Core 0 internal | `RenderContext` const ref (isolation barrier) |
| Modes → LogoManager | Core 0 internal | `LogoManager::loadLogo()` static call (accepted utility dependency — fills shared buffer from RenderContext) |
| Modes → NeoMatrixDisplay | **FORBIDDEN** | Modes never access NeoMatrixDisplay directly |
| Modes → ConfigManager | **FORBIDDEN** | Modes never read config directly (rule 18) |
| Dashboard JS → API | Browser → Core 1 | fetch() JSON envelope, synchronous POST for mode switch |

### Requirements to Structure Mapping

**FR Category → File Mapping:**

| FR Group | FRs | Primary Files | Supporting Files |
|----------|-----|---------------|-----------------|
| DisplayMode Interface | FR1-FR4 | `interfaces/DisplayMode.h` | `models/FlightInfo.h`, `core/LayoutEngine.h` (included transitively) |
| Mode Registry & Lifecycle | FR5-FR10 | `core/ModeRegistry.h`, `core/ModeRegistry.cpp` | `src/main.cpp` (MODE_TABLE), `test/test_mode_registry/test_main.cpp` |
| Classic Card Mode | FR11-FR13 | `modes/ClassicCardMode.h`, `modes/ClassicCardMode.cpp` | `utils/DisplayUtils.h/.cpp`, `adapters/NeoMatrixDisplay.cpp` (extraction source), `core/LogoManager.h` (loadLogo) |
| Live Flight Card Mode | FR14-FR16 | `modes/LiveFlightCardMode.h`, `modes/LiveFlightCardMode.cpp` | `utils/DisplayUtils.h/.cpp`, `models/FlightInfo.h` (telemetry fields), `core/LogoManager.h` (loadLogo) |
| Mode Picker UI | FR17-FR26 | `data-src/dashboard.html`, `data-src/dashboard.js`, `data-src/style.css` | `data/*.gz` (rebuild) |
| Mode Persistence | FR27-FR29 | `core/ConfigManager.h/.cpp` | `core/ModeRegistry.cpp` (debounce), `src/main.cpp` (boot restore) |
| Mode Switch API | FR30-FR33 | `adapters/WebPortal.h`, `adapters/WebPortal.cpp` | `core/ModeRegistry.h` (requestSwitch, getters) |
| Display Infrastructure | FR34-FR36 | `adapters/NeoMatrixDisplay.h/.cpp`, `src/main.cpp` | `core/ModeRegistry.cpp` (tick integration), `platformio.ini` (build config) |

**Cross-Cutting Concerns → File Mapping:**

| Concern | Files Touched |
|---------|--------------|
| RenderContext isolation (NFR C2) | `interfaces/DisplayMode.h` (struct def), `adapters/NeoMatrixDisplay.cpp` (buildRenderContext), all `modes/*.cpp` (consumers) |
| Cross-core coordination | `core/ModeRegistry.h` (atomic), `adapters/WebPortal.cpp` (requestSwitch caller), `src/main.cpp` (tick caller) |
| Heap lifecycle (NFR S1, S3, S4) | `core/ModeRegistry.cpp` (switch flow), all `modes/*.cpp` (init/teardown) |
| Heap baseline measurement | `src/main.cpp` — `ESP.getFreeHeap()` log after full init, before `ModeRegistry::init()` |
| NVS debounce | `core/ModeRegistry.cpp` (2s timer), `core/ConfigManager.cpp` (read/write) |
| Shared rendering utilities | `utils/DisplayUtils.h/.cpp` (extracted helpers), `modes/*.cpp` (consumers) |
| PlatformIO build discovery | `platformio.ini` (build_src_filter, build_flags) |

### Include Path Conventions for Mode Files

Mode source files use path-from-firmware-root includes, consistent with the existing codebase pattern in `main.cpp`:

```cpp
// modes/ClassicCardMode.cpp — canonical include style
#include "interfaces/DisplayMode.h"    // ✅ base class (transitively includes FlightInfo.h, LayoutEngine.h)
#include "utils/DisplayUtils.h"        // ✅ shared rendering helpers
#include "core/LogoManager.h"          // ✅ accepted static utility dependency
#include "modes/ClassicCardMode.h"     // ✅ own header
```

`DisplayMode.h` includes `"models/FlightInfo.h"` and `"core/LayoutEngine.h"` directly — modes receive these transitively and do not need to include them separately.

### Integration Points

**Internal Communication:**
- Display task calls `ModeRegistry::tick()` every frame (~20fps) — passes cached `RenderContext` + flight vector
- `ModeRegistry::tick()` delegates to `_activeMode->render()` during IDLE, or executes switch lifecycle during REQUESTED/SWITCHING
- After `tick()` returns, display task calls `g_display.show()` — separates rendering from frame commit
- WebPortal calls `ModeRegistry::requestSwitch()` from async handler — sets atomic flag only, returns immediately
- `ModeRegistry::tick()` calls `ConfigManager::setDisplayMode()` after debounce timer expires (2s post-switch)

**External Integrations:**
- No new external API calls or network dependencies
- Mode Picker UI communicates exclusively through existing `/api/*` pattern (fetch + JSON envelope)

**Data Flow (Mode Switch):**

```
Browser click "Activate"
  → POST /api/display/mode { "mode": "live_flight" }
  → WebPortal::_handlePostDisplayMode() [Core 1]
  → ModeRegistry::requestSwitch("live_flight") → sets _requestedIndex atomic
  → returns { "ok": true, "switching_to": "live_flight" }
  → Browser polls GET /api/display/modes at 500ms

Next display task tick [Core 0]:
  → ModeRegistry::tick() detects _requestedIndex != _activeModeIndex
  → _activeMode->teardown() → frees mode heap
  → ESP.getFreeHeap() check vs memoryRequirement()
  → factory() → new mode → init(ctx)
  → success: update index, set NVS pending, clear error
  → _switchState = IDLE

After 2s debounce [Core 0, inside tick()]:
  → ConfigManager::setDisplayMode("live_flight") → NVS write

Next browser poll:
  → GET /api/display/modes returns switch_state: "idle", active: "live_flight"
  → UI updates to show Live Flight Card as active
```

### Development Workflow Integration

**Build process:** PlatformIO compiles all `.cpp` files per `build_src_filter` in `platformio.ini`. The updated filter includes `modes/` and `utils/` directories alongside existing `adapters/` and `core/`.

**Gzip rebuild:** After modifying `data-src/*.html`, `data-src/*.js`, or `data-src/*.css`:
```bash
gzip -c firmware/data-src/dashboard.html > firmware/data/dashboard.html.gz
gzip -c firmware/data-src/dashboard.js > firmware/data/dashboard.js.gz
gzip -c firmware/data-src/style.css > firmware/data/style.css.gz
```

**Upload sequence:** `~/.platformio/penv/bin/pio run` (build) → `~/.platformio/penv/bin/pio run -t uploadfs` (flash filesystem) → `~/.platformio/penv/bin/pio run -t upload` (flash firmware)

**Heap baseline prerequisite:** Before implementing any Display System code, flash current Foundation firmware and record `ESP.getFreeHeap()` from serial monitor after full boot (WiFi connected, web server active, NTP synced, flight data in queue). This establishes the mode memory budget and validates whether the 30KB floor (NFR S4) is achievable.

### Party Mode Refinements Incorporated

- `platformio.ini` promoted from UNCHANGED to UPDATED (blocker — modes/ and utils/ wouldn't compile)
- `LogoManager::loadLogo()` documented as accepted static utility dependency from modes (not a rule 18 violation)
- `DisplayMode.h` specified to include `FlightInfo.h` and `LayoutEngine.h` directly for transitive access
- `test/test_mode_registry/test_main.cpp` added to NEW files (signals testing intent for highest-risk component)
- Heap baseline measurement location specified: `ESP.getFreeHeap()` log in `setup()` after full init
- `ModeRegistry → ConfigManager::setDisplayMode()` added to data boundaries table
- Pixel parity validation (NFR C1) noted as hardware visual validation — not automatable in unit tests

---

# Delight Release — Architecture Extension

_Extends the MVP + Foundation + Display System architecture with Clock Mode, Departures Board, animated transitions, mode scheduling, mode-specific settings, and OTA Pull from GitHub Releases. All prior architectural decisions remain in effect — this section adds new decisions and updates existing ones where the Delight PRD introduces changes._

## Delight Release — Project Context Analysis

### Requirements Overview

**Functional Requirements (43 FRs across 9 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| Clock Mode | FR1-FR4 | **Minor:** New DisplayMode implementation (rule 22 — two touch points); idle-fallback trigger integrated with mode orchestration |
| Departures Board | FR5-FR9 | **Minor:** New DisplayMode with multi-row rendering, dynamic row add/remove, configurable row count (rule 22 — two touch points) |
| Mode Transitions | FR10-FR12 | **Medium:** Fade transition phase added to ModeRegistry switch lifecycle; dual RGB565 frame buffers (~20KB transient allocation) |
| Mode Scheduling | FR13-FR17, FR36, FR39 | **Major:** Mode orchestration state machine (MANUAL / SCHEDULED / IDLE_FALLBACK); time-based rules stored in NVS; priority resolution between schedule, idle-fallback, and manual switching |
| Mode Configuration | FR18-FR20, FR40 | **Medium:** Per-mode settings with NVS key prefix convention; mode-scoped settings schema in static metadata; dynamic Mode Picker settings panels |
| OTA Firmware Updates | FR21-FR31, FR37-FR38 | **Major:** New `OTAUpdater` component — GitHub Releases API client, version comparison, binary download + SHA-256 verification; dashboard OTA Pull UI; shares pre-check sequence with Foundation's OTA Push |
| System Resilience | FR32-FR35 | **Medium:** Watchdog recovery to default mode, config key migration with safe defaults, heap pre-check for OTA + transitions, graceful API degradation |
| Contributor Extensibility | FR41-FR42 | **Minor:** Formalizes existing DisplayMode plugin contract from Display System; compile-time registry already established |
| Documentation & Onboarding | FR43 | **None:** Documentation validation, no architectural impact |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| Transition framerate | ≥15fps on 160x32 | Dual RGB565 frame buffers with blend-per-pixel; buffers transient during switch only |
| Transition duration | <1 second | Fixed frame count (e.g., 15 frames at 15fps = 1s max) |
| Idle fallback latency | ~30s (one poll interval) | Mode orchestrator monitors flight count after each fetch cycle |
| Departures Board row update | <50ms | In-place row mutation during render(), no full-screen redraw per row change |
| OTA download + reboot | <60s on 10Mbps | Stream-to-partition (same as Foundation push), add GitHub API + SHA-256 overhead |
| Schedule trigger accuracy | ±5 seconds | Non-blocking time check in main loop, same pattern as night mode brightness scheduler |
| Heap floor | ≥80KB free during normal operation | Transition buffers allocated only during switch, freed immediately after |
| Firmware binary size | ≤2,097,152 bytes (2 MiB OTA slot) | Monitor binary size; OTAUpdater adds ~10-15KB |
| No heap degradation | <5% loss over 24 hours | Transition buffers fully freed after each switch; mode init/teardown cycle tested |
| 30-day uptime | Continuous | Schedule timer, OTA check, and transition buffers must not leak memory |

### Scale & Complexity

- **Primary domain:** IoT/Embedded with companion web UI (unchanged)
- **Complexity level:** Medium — two new modes are straightforward plugin implementations (rule 22); mode orchestration state machine and OTA Pull add meaningful coordination complexity
- **New architectural component:** 1 (`core/OTAUpdater` — GitHub API client with version check, download, SHA-256 verification)
- **New lightweight constructs:** Mode scheduling logic (static class or main-loop function, same pattern as Foundation brightness scheduler); fade transition as private method in ModeRegistry
- **New mode implementations (rule 22):** ClockMode, DeparturesBoardMode (two touch points each — `modes/*.h/.cpp` + `MODE_TABLE[]` entry)
- **Updated components:** ModeRegistry (fade transition phase, idle-fallback hook), WebPortal (OTA Pull endpoints, schedule API, mode-settings API), ConfigManager (schedule rules, per-mode settings, OTA Pull keys), main.cpp (mode scheduler in loop, OTA pre-check integration), dashboard HTML/JS (Mode Picker settings panels, schedule UI, OTA Pull UI)
- **Existing components unchanged:** FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, WiFiManager, LayoutEngine, LogoManager, SystemStatus (add OTA_PULL subsystem entry), ClassicCardMode, LiveFlightCardMode, DisplayUtils

### Technical Constraints (Delight-Specific)

| Constraint | Impact |
|-----------|--------|
| Dual RGB565 frame buffers for fade = ~20KB heap | 160 x 32 x 2 bytes/pixel = 10,240 bytes per buffer, ~20KB total. Buffers are transient — allocate on switch, free after transition completes. Never coexist with OTA download. Well within 80KB heap floor budget. |
| NVS 15-char key limit for schedule rules | Multiple schedule rules need indexed keys (e.g., `sched_r0_start`, `sched_r0_end`, `sched_r0_mode`) — fixed max rule count with 3 keys per rule |
| NVS 15-char key limit for per-mode settings | Prefix convention: `m_` (2) + mode ID abbrev (≤5) + `_` (1) + setting name (≤7) = max 15 chars. Mode ID abbreviations enforced by documentation and code review. |
| GitHub Releases API rate limit (60 req/hr unauthenticated) | OTA check is manual and non-blocking; HTTP 429 → "try again later", no auto-retry |
| GitHub binary download over HTTPS (~1.5MB) | Uses same `Update.h` stream-to-partition path as Foundation push; WiFi must remain stable during download |
| NTP required for scheduling + Clock Mode | Schedule inactive until first NTP sync; Clock Mode falls back to uptime-based display if NTP unavailable |
| Per-mode settings expand NVS usage | Each mode adds 2-5 NVS keys; total NVS key count grows from ~28 to ~40+; well within NVS partition capacity (20KB) |
| OTA pre-check must tear down active mode + free transition buffers | Shared pre-check sequence for both Pull and Push OTA paths — extracted as common utility |

### Cross-Cutting Concerns (Delight Additions)

1. **Transition frame buffer lifecycle** — Dual RGB565 buffers (~20KB total) allocated only during mode switch, freed immediately after fade animation completes. Never coexist with OTA download. ModeRegistry's switch flow gains a fade phase between teardown and init: `teardown old → alloc buffers → capture outgoing frame → init new mode → render N blend frames → free buffers → IDLE`. Implemented as `ModeRegistry::_executeFadeTransition()` private method — not a separate component. If Growth phase adds wipe/scroll, extract at that point.

2. **Mode orchestration state machine** — Three mode-switch sources now exist. Modeled as explicit states with defined transitions:

    **States:**
    - `MANUAL` — user's last explicit selection (NVS persisted via `display_mode` key)
    - `SCHEDULED` — time-based rule is currently active
    - `IDLE_FALLBACK` — zero flights triggered Clock Mode

    **Priority:** `SCHEDULED > IDLE_FALLBACK > MANUAL`

    **Transitions:**
    - Schedule rule fires → enter `SCHEDULED`, switch to scheduled mode
    - Schedule rule expires → exit `SCHEDULED`, restore `MANUAL` selection
    - Zero flights detected (while not `SCHEDULED`) → enter `IDLE_FALLBACK`, switch to Clock Mode
    - Flights return (while `IDLE_FALLBACK`) → exit `IDLE_FALLBACK`, restore `MANUAL` selection
    - Zero flights detected (while `SCHEDULED`) → stay `SCHEDULED` (schedule wins per FR17)
    - User manual switch → update `MANUAL` selection in NVS; if in `IDLE_FALLBACK`, exit fallback; if in `SCHEDULED`, manual switch is temporary override until next schedule tick re-evaluates

3. **OTA Pull + OTA Push coexistence** — Both write to the inactive OTA partition. Both need the same pre-check sequence: verify ≥80KB free heap, tear down active mode (free mode heap + any transition buffers), suspend rendering, show progress on LEDs. Extract shared pre-OTA logic into a common function. Push (Foundation) receives binary from browser multipart upload via WebPortal. Pull (Delight) fetches binary from GitHub CDN via OTAUpdater. Post-write SHA-256 verification (Pull only — Push uses `Update.end()` built-in CRC), boot swap, and reboot are shared.

4. **OTA Pull failure modes** — The PRD specifies SHA-256 verification via companion `.sha256` release asset. Explicit failure handling:
    - `.sha256` asset missing from GitHub Release → reject update, show "Release missing integrity file — contact maintainer"
    - Download succeeds but SHA-256 mismatch → reject, do not activate partition, show "Firmware integrity check failed"
    - Download interrupted (WiFi drop, timeout) → firmware unchanged (inactive partition has partial write, never activated), show "Download failed — tap to retry"
    - GitHub API unreachable → show "Unable to check for updates — try again later", no auto-retry
    - HTTP 429 rate limited → same as unreachable, with explicit "rate limited" message per NFR20

5. **Per-mode NVS key namespacing** — Mode-specific settings use a strict prefix convention:
    - Format: `m_{abbrev}_{setting}` where mode ID abbreviation is ≤5 chars, setting name is ≤7 chars
    - Examples: `m_clock_format` (12/24h), `m_depbd_rows` (max rows), `m_depbd_fields` (field selection), `m_lfcrd_fields` (Live Flight Card telemetry fields)
    - Mode abbreviations: `clock` (5), `depbd` (5), `lfcrd` (5) — documented in ConfigManager alongside existing NVS key abbreviation table
    - Modes declare their settings schema in static metadata (same pattern as `ModeZoneDescriptor`) — name, type, default, min/max for the Mode Picker UI to render settings panels dynamically
    - **Rule:** New mode abbreviations must be ≤5 chars. Enforced by documentation and PR review.

6. **Schedule rules NVS storage** — Fixed maximum of 8 schedule rules with indexed NVS keys:
    - `sched_r{N}_start` (uint16, minutes since midnight)
    - `sched_r{N}_end` (uint16, minutes since midnight)
    - `sched_r{N}_mode` (string, mode ID)
    - `sched_r_count` (uint8, active rule count)
    - Total: up to 25 additional NVS keys at max capacity (8 rules x 3 keys + count)
    - Same minutes-since-midnight convention as Foundation's night mode (rule 15)

7. **NTP dependency expansion** — Foundation introduced NTP for brightness scheduling. Delight adds two more consumers: Clock Mode display and mode scheduling. All three share the same `configTzTime()` sync — no additional NTP configuration. Consequence of NTP failure expands: scheduling degrades gracefully (rules don't fire until NTP syncs), Clock Mode shows uptime-based time or a static indicator. Flight rendering unaffected.

### Existing Architecture Unchanged

The following MVP + Foundation + Display System decisions remain in full effect with no modifications:

- Hexagonal (Ports & Adapters) pattern
- ConfigManager singleton with category struct getters
- Inter-task communication (atomic flags for config, FreeRTOS queue for flights)
- WiFi state machine (WiFiManager)
- SystemStatus error registry
- FreeRTOS dual-core task pinning (display Core 0, everything else Core 1)
- All naming conventions, NVS key patterns, logging macros, and enforcement rules (1-23)
- Web asset patterns (fetch + json.ok + showToast)
- DisplayMode interface, RenderContext isolation, ModeRegistry static table
- OTA self-check, rollback detection, night mode brightness scheduler
- Flash partition layout (unchanged from Foundation — dual OTA 1.5MB + 960KB LittleFS)

## Delight Release — Technology Stack

**Starter Template Evaluation: N/A (Brownfield Extension)**

No new external dependencies required for the core Delight capabilities (Clock Mode, Departures Board, transitions, scheduling, mode-specific settings). All render through the existing DisplayMode interface using Adafruit GFX via FastLED NeoMatrix.

**One new external integration — GitHub Releases API:**

| Component | Implementation | Source |
|-----------|---------------|--------|
| ClockMode | C++ class implementing DisplayMode | New firmware code |
| DeparturesBoardMode | C++ class implementing DisplayMode | New firmware code |
| Fade transition | Private method in ModeRegistry | New firmware code |
| Mode scheduling | Static class or main-loop function | New firmware code |
| Mode-specific settings | ConfigManager expansion + Mode Picker UI | Existing pattern extension |
| OTAUpdater | GitHub Releases API client | New firmware code |
| GitHub Releases API | `api.github.com` REST endpoint | Unauthenticated HTTPS GET |
| SHA-256 verification | `mbedtls_sha256` | ESP32 core (built-in mbedTLS) |

**New library dependency: None.** SHA-256 is available via ESP32's built-in mbedTLS stack (`mbedtls/sha256.h`), already linked for HTTPS TLS. GitHub API responses parsed with existing ArduinoJson. Binary download uses existing `HTTPClient` + `Update.h`.

The MVP + Foundation + Display System dependency stack (ESPAsyncWebServer Carbou fork, AsyncTCP, FastLED, Adafruit GFX, FastLED NeoMatrix, ArduinoJson, LittleFS, ESPmDNS, Update.h, esp_ota_ops.h, time.h/configTzTime, mbedTLS) remains unchanged.

## Delight Release — Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. DL1 — Fade transition in ModeRegistry (RGB888 dual buffer lifecycle, blend algorithm)
2. DL2 — Mode orchestration state machine (MANUAL/SCHEDULED/IDLE_FALLBACK)
3. DL3 — OTAUpdater component (GitHub API client, incremental SHA-256, FreeRTOS download task)
4. DL4 — Schedule rules NVS storage (indexed keys, max 8 rules)

**Important Decisions (Shape Architecture):**
5. DL5 — Per-mode settings schema (NVS prefix convention, static metadata, nullable in ModeEntry)
6. DL6 — API endpoint additions (OTA Pull with status polling, schedule CRUD, mode settings)

**Deferred Decisions (Post-Delight):**
- Multiple transition types (wipe, scroll) — fade only in MVP; extract TransitionEngine if Growth adds more
- Departures Board layout variants — single layout in MVP
- Schedule templates ("Home office," "Weekend") — manual entry only
- OTA firmware signing — trust based on GitHub repo URL for now

### Decision DL1: Fade Transition — ModeRegistry Private Method with Transient RGB888 Buffers

**Approach:** Fade is a phase within the existing `ModeRegistry::tick()` switch lifecycle, implemented as `_executeFadeTransition()` private method. No separate component.

**Buffer strategy:**
- Two RGB888 buffers matching FastLED's native `CRGB` format: `160 * 32 * 3 = 15,360 bytes per buffer`, ~30KB total
- `malloc()` at transition start, `free()` immediately after last blend frame
- If `malloc()` fails (heap too low): skip fade, do instant cut (current behavior) — graceful degradation, not an error
- **Why RGB888, not RGB565:** FastLED stores pixels as `CRGB` (3 bytes RGB888). The `leds` array is directly accessible. Using RGB888 avoids color space conversion in both capture and blend — `memcpy(outgoing, leds, PIXEL_COUNT * 3)` captures the frame. Blend operates natively. No precision loss.

**Blend algorithm:**
```cpp
static constexpr uint8_t FADE_STEPS = 15;  // ~1 second at 15fps

void ModeRegistry::_executeFadeTransition(const RenderContext& ctx,
                                           const std::vector<FlightInfo>& flights) {
    const uint16_t pixelCount = ctx.layout.matrix_width * ctx.layout.matrix_height;
    const size_t bufSize = pixelCount * 3;

    uint8_t* outgoing = (uint8_t*)malloc(bufSize);
    uint8_t* incoming = (uint8_t*)malloc(bufSize);
    if (!outgoing || !incoming) {
        free(outgoing); free(incoming);
        LOG_I("ModeReg", "Fade buffer alloc failed, instant cut");
        return;  // caller proceeds with instant cut
    }

    // Capture outgoing frame from FastLED's CRGB array
    memcpy(outgoing, leds, bufSize);

    // Render one frame of new mode to capture incoming
    _activeMode->render(ctx, flights);
    memcpy(incoming, leds, bufSize);

    // Blend N frames
    for (uint8_t step = 1; step <= FADE_STEPS; step++) {
        for (uint16_t i = 0; i < pixelCount; i++) {
            uint16_t idx = i * 3;
            leds[i].r = (outgoing[idx]     * (FADE_STEPS - step) + incoming[idx]     * step) / FADE_STEPS;
            leds[i].g = (outgoing[idx + 1] * (FADE_STEPS - step) + incoming[idx + 1] * step) / FADE_STEPS;
            leds[i].b = (outgoing[idx + 2] * (FADE_STEPS - step) + incoming[idx + 2] * step) / FADE_STEPS;
        }
        FastLED.show();
        delay(1000 / FADE_STEPS);  // ~66ms per frame at 15fps
    }

    free(outgoing);
    free(incoming);
}
```

**Note:** `delay()` is acceptable inside the fade loop because this runs on Core 0 (display task) during `SWITCHING` state — no other rendering occurs. The main loop on Core 1 is unaffected.

**Updated switch flow in `tick()`:**
```
1. _switchState = SWITCHING
2. _activeMode->teardown()
3. delete _activeMode
4. Heap check against memoryRequirement()
5. If sufficient: factory() → new mode → init(ctx)
6. If init() succeeds: _executeFadeTransition(ctx, flights)
7. _switchState = IDLE, set NVS pending
---
If heap insufficient or init() fails: restore previous mode, set error
If fade buffer alloc fails: instant cut (no regression from current behavior)
```

### Decision DL2: Mode Orchestration — ModeOrchestrator Static Class

**Approach:** Lightweight `core/ModeOrchestrator.h/.cpp` static class. Runs in the Core 1 main loop alongside the existing brightness scheduler. Calls `ModeRegistry::requestSwitch()` when state transitions require a mode change.

```cpp
enum class OrchestratorState : uint8_t {
    MANUAL,          // User's explicit selection (default)
    SCHEDULED,       // Time-based rule is active
    IDLE_FALLBACK    // Zero flights → Clock Mode
};

class ModeOrchestrator {
public:
    static void init();
    static void tick();                              // called from Core 1 main loop, non-blocking
    static void onManualSwitch(const char* modeId);  // called from WebPortal on user switch
    static OrchestratorState getState();
    static const char* getActiveScheduleRuleName();  // for dashboard status
    static const char* getManualModeId();

private:
    static OrchestratorState _state;
    static char _manualModeId[16];                   // NVS-persisted user selection
    static int8_t _activeRuleIndex;                  // -1 if no rule active
    static unsigned long _lastTickMs;
};
```

**Flight count signaling:**
```cpp
// In main.cpp — global atomic, updated by FlightDataFetcher after queue write
std::atomic<uint8_t> g_flightCount{0};

// In FlightDataFetcher, after xQueueOverwrite:
g_flightCount.store(enrichedFlights.size());

// ModeOrchestrator reads this — no queue peek, no cross-core conflict
uint8_t flights = g_flightCount.load();
```

**State machine (inside `tick()`, runs ~1/sec on Core 1):**

```cpp
void ModeOrchestrator::tick() {
    if (millis() - _lastTickMs < 1000) return;  // 1-second interval
    _lastTickMs = millis();

    struct tm now;
    if (!getLocalTime(&now, 0)) return;  // non-blocking; skip if NTP not synced
    uint16_t currentMin = now.tm_hour * 60 + now.tm_min;

    // 1. Evaluate schedule rules (highest priority)
    ModeScheduleConfig sched = ConfigManager::getModeSchedule();
    int8_t matchingRule = -1;
    for (uint8_t i = 0; i < sched.rule_count; i++) {
        if (sched.rules[i].enabled && timeInWindow(currentMin,
                sched.rules[i].start_min, sched.rules[i].end_min)) {
            matchingRule = i;
            break;  // first match wins
        }
    }

    if (matchingRule >= 0) {
        if (_state != SCHEDULED || _activeRuleIndex != matchingRule) {
            _state = SCHEDULED;
            _activeRuleIndex = matchingRule;
            ModeRegistry::requestSwitch(sched.rules[matchingRule].mode_id);
        }
        return;  // schedule active — skip idle fallback check
    }

    // No schedule rule active — exit SCHEDULED if we were in it
    if (_state == SCHEDULED) {
        _state = MANUAL;
        _activeRuleIndex = -1;
        ModeRegistry::requestSwitch(_manualModeId);
        return;
    }

    // 2. Check idle fallback (only when not scheduled)
    uint8_t flights = g_flightCount.load();
    if (flights == 0 && _state == MANUAL) {
        _state = IDLE_FALLBACK;
        ModeRegistry::requestSwitch("clock");
    } else if (flights > 0 && _state == IDLE_FALLBACK) {
        _state = MANUAL;
        ModeRegistry::requestSwitch(_manualModeId);
    }
}
```

**Boot state:** Always starts as `MANUAL`. First `tick()` call (~1 second after boot) evaluates schedule rules and flight count. If a schedule matches, transitions to `SCHEDULED` within that first tick. If zero flights, transitions to `IDLE_FALLBACK`.

**Manual switch handling:**
```cpp
void ModeOrchestrator::onManualSwitch(const char* modeId) {
    strncpy(_manualModeId, modeId, 15);
    _manualModeId[15] = '\0';
    if (_state == IDLE_FALLBACK) {
        _state = MANUAL;
    }
    // If SCHEDULED: manual switch takes effect, but next tick may re-apply schedule
    // This is documented behavior per PRD Journey 1 climax: "schedule takes priority when explicitly configured"
}
```

**Midnight-crossing time window check (same convention as Foundation brightness scheduler):**
```cpp
static bool timeInWindow(uint16_t current, uint16_t start, uint16_t end) {
    if (start <= end) {
        return (current >= start && current < end);
    } else {
        return (current >= start || current < end);
    }
}
```

### Decision DL3: OTAUpdater — GitHub Releases API Client with Incremental SHA-256

**Approach:** New `core/OTAUpdater.h/.cpp` static class. HTTP client that checks GitHub Releases, downloads binary with incremental SHA-256 verification, streams to partition. Download runs in a one-shot FreeRTOS task to avoid blocking the async web server.

```cpp
enum class OTAState : uint8_t {
    IDLE,
    CHECKING,
    AVAILABLE,
    DOWNLOADING,
    VERIFYING,
    REBOOTING,
    ERROR
};

class OTAUpdater {
public:
    static void init(const char* repoOwner, const char* repoName);

    // Version check — called from API handler, runs synchronously (~1-2s)
    static bool checkForUpdate();

    // Download — spawns FreeRTOS task, returns immediately
    static bool startDownload();

    // State accessors for API polling
    static OTAState getState();
    static uint8_t getProgress();        // 0-100%
    static const char* getNewVersion();
    static const char* getReleaseNotes();
    static const char* getLastError();
    static const char* getInstalledVersion();  // returns FW_VERSION

private:
    static char _repoOwner[32];
    static char _repoName[32];
    static char _binUrl[256];
    static char _shaUrl[256];
    static char _newVersion[16];
    static char _releaseNotes[512];      // truncated if longer
    static char _lastError[128];
    static OTAState _state;
    static uint8_t _progress;
    static TaskHandle_t _downloadTask;

    static void _downloadTaskFunc(void* param);
};
```

**Check flow (`checkForUpdate`):**
1. `_state = CHECKING`
2. `GET https://api.github.com/repos/{owner}/{repo}/releases/latest`
3. Parse JSON via ArduinoJson: extract `tag_name`, `body` (release notes), iterate `assets[]` to find `.bin` and `.sha256` URLs
4. Compare `tag_name` with `FW_VERSION` — simple string inequality (not semver)
5. If different: store URLs, version, notes; `_state = AVAILABLE`; return true
6. If same: `_state = IDLE`; return false
7. If HTTP error or 429: `_state = ERROR`; set error message; return false

**Download flow (`_downloadTaskFunc`, runs in FreeRTOS task):**
1. `_state = DOWNLOADING`
2. Shared pre-OTA sequence: `ModeRegistry::prepareForOTA()` — teardown active mode, free buffers, show "Updating..." on LEDs
3. Verify ≥80KB free heap; if not, set error, `_state = ERROR`, return
4. Download `.sha256` file first (~64 bytes); parse as **plain 64-character hex string** (no filename, no prefix). If missing or unparseable: set error "Release missing integrity file", `_state = ERROR`, return
5. Get next OTA partition: `esp_ota_get_next_update_partition(NULL)`
6. `Update.begin(partition->size)`
7. Initialize `mbedtls_sha256_context`
8. Stream `.bin` via `HTTPClient` in chunks:
   - `Update.write(chunk, len)` — write to flash
   - `mbedtls_sha256_update(&ctx, chunk, len)` — feed to hasher
   - Update `_progress = (bytesWritten * 100) / totalSize`
9. After final chunk: `mbedtls_sha256_finish(&ctx, computedHash)`
10. `_state = VERIFYING`
11. Compare `computedHash` against downloaded expected hash
    - If mismatch: `Update.abort()`, set error "Firmware integrity check failed", `_state = ERROR`, return
12. `Update.end(true)` — finalize partition
13. `_state = REBOOTING`
14. `delay(500); ESP.restart()`

**Error recovery:**
- Download interrupted (WiFi drop): `Update.abort()`, `_state = ERROR`, "Download failed — tap to retry"
- Any HTTP error during download: `Update.abort()`, `_state = ERROR`, specific message
- The inactive partition is never activated on failure — firmware unchanged
- After error, user can call `startDownload()` again (retry)

**Shared pre-OTA extraction:**
```cpp
// New public method in ModeRegistry
static void prepareForOTA() {
    // Teardown active mode — frees mode heap + any transition buffers
    if (_activeMode) {
        _activeMode->teardown();
        delete _activeMode;
        _activeMode = nullptr;
    }
    _switchState = SWITCHING;  // prevents new switch requests during OTA
}

// Used by both:
// - WebPortal::_handleOTAUpload() (Foundation push) — call before Update.begin()
// - OTAUpdater::_downloadTaskFunc() (Delight pull) — call before download
```

### Decision DL4: Schedule Rules NVS Storage — Fixed Max with Indexed Keys

**Approach:** Fixed maximum of 8 rules with indexed NVS keys. New `ModeScheduleConfig` struct in ConfigManager.

```cpp
struct ScheduleRule {
    uint16_t start_min;     // minutes since midnight (0-1439)
    uint16_t end_min;       // minutes since midnight (0-1439)
    char mode_id[16];       // full mode ID string (e.g., "clock", "departures")
    bool enabled;
};

struct ModeScheduleConfig {
    ScheduleRule rules[8];
    uint8_t rule_count;     // 0-8, number of defined rules
};
```

**NVS keys (per rule):**

| Key Pattern | Type | Example | Chars |
|------------|------|---------|-------|
| `sched_r{N}_start` | uint16 | `sched_r0_start` = 360 | 15 |
| `sched_r{N}_end` | uint16 | `sched_r0_end` = 1320 | 13 |
| `sched_r{N}_mode` | string | `sched_r0_mode` = `"clock"` | 14 |
| `sched_r_count` | uint8 | 2 | 13 |

All keys within 15-char NVS limit. Mode ID stored as full string value (values have no length limit in NVS). N ranges 0-7.

**ConfigManager additions:**
```cpp
static ModeScheduleConfig getModeSchedule();
static void setModeSchedule(const ModeScheduleConfig& config);
```

All schedule keys are **hot-reload** — no reboot required. `setModeSchedule()` writes all rule keys in a batch, then `ModeOrchestrator::tick()` picks up changes on its next iteration (~1 second).

**Rule conflict resolution:** Rules evaluated in index order (0 first). First matching rule wins. Overlapping time windows resolved by index priority — user arranges rules via the dashboard, display order is priority order.

### Decision DL5: Per-Mode Settings Schema — Static Metadata with NVS Prefix Convention

**Approach:** Modes declare settings via static metadata struct (same pattern as `ModeZoneDescriptor`). ConfigManager reads/writes using the `m_{abbrev}_{setting}` NVS key prefix. Mode Picker UI renders settings panels dynamically from the schema returned by the API.

```cpp
struct ModeSettingDef {
    const char* key;          // NVS key suffix (≤7 chars)
    const char* label;        // UI display name
    const char* type;         // "uint8", "uint16", "bool", "enum"
    int32_t defaultValue;
    int32_t minValue;
    int32_t maxValue;
    const char* enumOptions;  // comma-separated for enum type, NULL otherwise
};

struct ModeSettingsSchema {
    const char* modeAbbrev;           // ≤5 chars, used for NVS key prefix
    const ModeSettingDef* settings;
    uint8_t settingCount;
};
```

**Clock Mode settings:**
```cpp
// NVS key: m_clock_format (14 chars)
static const ModeSettingDef CLOCK_SETTINGS[] = {
    { "format", "Time Format", "enum", 0, 0, 1, "24h,12h" }
};
static const ModeSettingsSchema CLOCK_SCHEMA = { "clock", CLOCK_SETTINGS, 1 };
```

**Departures Board settings:**
```cpp
// NVS keys: m_depbd_rows (12 chars), m_depbd_fields (14 chars)
static const ModeSettingDef DEPBD_SETTINGS[] = {
    { "rows", "Max Rows", "uint8", 4, 1, 4, NULL },
    { "fields", "Telemetry Fields", "enum", 0, 0, 2, "alt+spd,alt+hdg,spd+hdg" }
};
static const ModeSettingsSchema DEPBD_SCHEMA = { "depbd", DEPBD_SETTINGS, 2 };
```

**ModeEntry expansion (backward-compatible):**
```cpp
struct ModeEntry {
    const char* id;
    const char* displayName;
    DisplayMode* (*factory)();
    uint32_t (*memoryRequirement)();
    uint8_t priority;
    const ModeSettingsSchema* settingsSchema;  // NEW — nullable for modes with no settings
};
```

Existing modes (ClassicCardMode, LiveFlightCardMode) set `settingsSchema` to `nullptr`. API returns empty `settings: []` for nullable schemas. No breaking change to existing mode table entries.

**ConfigManager per-mode helpers:**
```cpp
static int32_t getModeSetting(const char* modeAbbrev, const char* settingKey, int32_t defaultValue);
static void setModeSetting(const char* modeAbbrev, const char* settingKey, int32_t value);
// Constructs NVS key as: "m_" + modeAbbrev + "_" + settingKey
```

**Key convention enforcement:**
- Mode abbreviation: ≤5 chars (documented alongside existing NVS abbreviation table in ConfigManager.h)
- Setting name: ≤7 chars
- Total: `m_` (2) + abbrev (≤5) + `_` (1) + setting (≤7) = ≤15 chars
- Abbreviations: `clock` (5), `depbd` (5), `lfcrd` (5), `clsic` (5)

### Decision DL6: API Endpoint Additions — OTA Pull with Status Polling + Schedule CRUD

**5 new endpoints:**

| Method | Endpoint | Purpose | Response |
|--------|----------|---------|----------|
| `GET` | `/api/ota/check` | Check GitHub for new firmware version | `{ ok, data: { available, version, release_notes, current_version } }` |
| `POST` | `/api/ota/pull` | Start firmware download (spawns FreeRTOS task) | `{ ok, data: { started: true } }` |
| `GET` | `/api/ota/status` | Poll download progress and state | `{ ok, data: { state, progress, error } }` |
| `GET` | `/api/schedule` | Get mode schedule rules | `{ ok, data: { rules: [...], orchestrator_state } }` |
| `POST` | `/api/schedule` | Save mode schedule rules | `{ ok, data: { applied: true } }` |

**Updated existing endpoints:**

| Endpoint | Changes |
|----------|---------|
| `GET /api/display/modes` | Add `settings` array per mode with schema + current values; `null` for modes without settings |
| `POST /api/display/mode` | Accept optional `settings` object for per-mode config changes |
| `GET /api/status` | Add `ota_available` (bool), `ota_version` (string or null), `orchestrator_state` ("manual"/"scheduled"/"idle_fallback"), `active_schedule_rule` (index or -1) |

**`GET /api/ota/check` response:**
```json
{
    "ok": true,
    "data": {
        "available": true,
        "version": "2.4.1",
        "current_version": "2.4.0",
        "release_notes": "Bug fix for Departures Board row alignment..."
    }
}
```

**`GET /api/ota/status` response (during download):**
```json
{
    "ok": true,
    "data": {
        "state": "downloading",
        "progress": 42
    }
}
```

**`GET /api/schedule` response:**
```json
{
    "ok": true,
    "data": {
        "orchestrator_state": "scheduled",
        "active_rule_index": 0,
        "rules": [
            { "start": 360, "end": 1320, "mode": "departures", "enabled": true },
            { "start": 1320, "end": 360, "mode": "clock", "enabled": true }
        ]
    }
}
```

**`POST /api/ota/pull` behavior:**
- Validates that `OTAUpdater::getState()` is `AVAILABLE` (must check first)
- Calls `OTAUpdater::startDownload()` which spawns a FreeRTOS task
- Returns immediately with `{ ok: true }`
- Dashboard polls `GET /api/ota/status` at 500ms interval for progress
- Matches the existing mode-switch polling pattern from Display System

### Delight Decision Impact — Implementation Sequence

**Sequential foundation:**
1. **ConfigManager expansion** — ModeScheduleConfig struct, per-mode setting helpers, `getModeSchedule()` / `setModeSchedule()`, `getModeSetting()` / `setModeSetting()`
2. **ModeEntry expansion** — add `settingsSchema` field, update MODE_TABLE for existing modes with `nullptr`

**Mode implementations (sequential, after step 2):**
3. **ClockMode** (`modes/ClockMode.h/.cpp`) — simplest mode, proves NTP time display works
4. **DeparturesBoardMode** (`modes/DeparturesBoardMode.h/.cpp`) — multi-row rendering, configurable rows

**Orchestration track (after step 3, needs Clock Mode for idle fallback):**
5. **ModeOrchestrator** (`core/ModeOrchestrator.h/.cpp`) — state machine, schedule evaluation, idle fallback
6. **Fade transition** — `ModeRegistry::_executeFadeTransition()` private method + buffer management

**OTA track (independent of steps 3-6, after step 1):**
7. **OTAUpdater** (`core/OTAUpdater.h/.cpp`) — GitHub API client, incremental SHA-256, FreeRTOS download task
8. **ModeRegistry::prepareForOTA()** — shared pre-OTA sequence for both Push and Pull

**API + UI (after all firmware tracks complete):**
9. **WebPortal endpoints** — OTA Pull (check/pull/status), schedule (GET/POST), mode settings expansion
10. **Dashboard UI** — OTA Pull UI (banner, release notes, progress), schedule timeline editor, per-mode settings panels in Mode Picker
11. **Gzip rebuild** — all updated web assets

### Cross-Decision Dependencies

```
ConfigManager (DL4 schedule keys + DL5 per-mode settings)
    ├── ModeOrchestrator (DL2) — reads schedule rules
    │   └── ModeRegistry::requestSwitch() — triggers mode changes
    │       └── _executeFadeTransition (DL1) — runs during switch
    └── OTAUpdater (DL3) — reads repo config
        └── ModeRegistry::prepareForOTA() — shared pre-OTA

ModeEntry.settingsSchema (DL5)
    └── GET /api/display/modes (DL6) — includes settings in response
        └── Mode Picker UI — renders per-mode settings panels

g_flightCount atomic (DL2)
    ← FlightDataFetcher (writes after queue update)
    → ModeOrchestrator::tick() (reads for idle fallback)
```

## Delight Release — Implementation Patterns & Consistency Rules

### Delight-Specific Conflict Points

The MVP architecture established 11 enforcement rules. The Foundation Release added 5 (rules 12-16). The Display System Release added 7 (rules 17-23). The Delight Release introduces 7 new pattern areas where AI agents could make inconsistent choices.

### Orchestrator Interaction Pattern

**Who calls `ModeRegistry::requestSwitch()` — and when:**

ModeOrchestrator is the *only* component that evaluates schedule rules and idle fallback. WebPortal routes manual switches through `ModeOrchestrator::onManualSwitch()`, which records the user's preference and then delegates to `requestSwitch()` if appropriate.

**`requestSwitch()` is called from exactly two methods:**
1. `ModeOrchestrator::tick()` — when state machine transitions require a mode change
2. `ModeOrchestrator::onManualSwitch()` — when user selects a mode via dashboard

**Allowed:**
```cpp
// WebPortal handler for POST /api/display/mode:
ModeOrchestrator::onManualSwitch(modeId);  // ✅ always go through orchestrator

// ModeOrchestrator::tick() — evaluates state machine:
ModeRegistry::requestSwitch(targetModeId);  // ✅ orchestrator owns the decision
```

**Forbidden:**
```cpp
// WebPortal directly calling ModeRegistry:
ModeRegistry::requestSwitch(modeId);  // ❌ bypasses orchestrator state tracking

// Mode class requesting its own switch:
ModeRegistry::requestSwitch("clock");  // ❌ modes never initiate switches

// Timer callback or config-change handler calling requestSwitch:
void onConfigChange() { ModeRegistry::requestSwitch(...); }  // ❌ only orchestrator calls this
```

**Anti-pattern:** Calling `requestSwitch()` from anywhere except the two orchestrator methods. The orchestrator maintains state (MANUAL/SCHEDULED/IDLE_FALLBACK) that must stay consistent with the actual active mode. A direct `requestSwitch()` from any other call site would desync the state machine.

### Cross-Core Atomic Signaling Pattern

**`std::atomic` globals declared in `main.cpp` only:**

DL2 introduces `std::atomic<uint8_t> g_flightCount` as a cross-core signaling mechanism. This is a lightweight pattern for Core 1 → Core 0 communication without queues.

**Allowed:**
```cpp
// main.cpp — declaration
std::atomic<uint8_t> g_flightCount{0};

// FlightDataFetcher (Core 1) — writer
g_flightCount.store(enrichedFlights.size());

// ModeOrchestrator (Core 1) — reader
uint8_t flights = g_flightCount.load();
```

**Forbidden:**
```cpp
// Mode class reading atomic:
void ClockMode::render(...) {
    uint8_t f = g_flightCount.load();  // ❌ modes use RenderContext, not globals
}

// Adapter reading atomic:
void WebPortal::handleStatus() {
    uint8_t f = g_flightCount.load();  // ❌ adapters use core API, not globals
}
```

**Anti-pattern:** Declaring new `std::atomic` globals outside `main.cpp`, or reading/writing them from modes or adapters. Cross-core atomics are system-level plumbing — only core orchestration code (`main.cpp`, `ModeOrchestrator`, `FlightDataFetcher`) should touch them.

### OTA Pull Streaming Pattern

**Incremental hash — never post-hoc:**

The SHA-256 hash must be computed incrementally during the download stream. Each chunk is fed to both `Update.write()` and `mbedtls_sha256_update()` in the same loop iteration.

```cpp
// Correct pattern:
while (remaining > 0) {
    int len = stream->readBytes(buf, chunkSize);
    Update.write(buf, len);                      // write to flash
    mbedtls_sha256_update(&ctx, buf, len);       // feed to hasher
    _progress = (written * 100) / totalSize;     // update progress
}
mbedtls_sha256_finish(&ctx, computed);
// compare computed vs expected BEFORE Update.end()
```

**Anti-pattern:** Downloading the full binary, calling `Update.end()`, then trying to read back the partition to verify. `Update.h` provides no read-back API. Hash *during* streaming, verify *before* finalize.

**Error path discipline — always `Update.abort()` after `Update.begin()`:**

Once `Update.begin()` has been called, every error path must call `Update.abort()` before returning. This releases the flash write lock and leaves the inactive partition unchanged.

```cpp
// Correct error handling:
if (hashMismatch) {
    Update.abort();                              // ✅ always abort on failure
    _state = OTAState::ERROR;
    strncpy(_lastError, "Integrity check failed", sizeof(_lastError));
    return;
}

// Anti-pattern:
if (downloadFailed) {
    _state = OTAState::ERROR;                    // ❌ forgot Update.abort()
    return;                                       // flash write lock still held
}
```

**FreeRTOS task lifecycle:**
- `startDownload()` spawns a one-shot task on Core 1 (pinned, 8KB stack, priority 1)
- Task sets `_downloadTask = nullptr` *before* calling `vTaskDelete(NULL)` — prevents dangling handle reads from main thread
- Only one download task at a time — `startDownload()` rejects if `_downloadTask != nullptr`

```cpp
// End of _downloadTaskFunc:
_downloadTask = nullptr;   // ✅ clear handle before delete
vTaskDelete(NULL);         // task self-deletes

// startDownload() guard:
if (_downloadTask != nullptr) {
    return false;          // download already in progress
}
```

**Anti-pattern:** Creating a persistent FreeRTOS task that polls for OTA. The download is a one-shot operation triggered by the user.

### Pre-OTA Sequence Pattern

**Shared between Foundation Push and Delight Pull:**

Both OTA paths (browser upload and GitHub download) must call `ModeRegistry::prepareForOTA()` before `Update.begin()`. This frees mode heap and prevents the display task from interfering during the flash write.

**`prepareForOTA()` must set `_switchState = SWITCHING`** to block the display task's `tick()` loop. Without this flag, Core 0 could call `tick()` on a deleted mode pointer during the flash write.

```cpp
// ModeRegistry::prepareForOTA():
static void prepareForOTA() {
    _switchState = SWITCHING;  // ✅ blocks display task tick() loop
    if (_activeMode) {
        _activeMode->teardown();
        delete _activeMode;
        _activeMode = nullptr;
    }
}

// Foundation Push (WebPortal OTA upload handler):
ModeRegistry::prepareForOTA();  // then Update.begin(contentLength)

// Delight Pull (OTAUpdater download task):
ModeRegistry::prepareForOTA();  // then Update.begin(partitionSize)
```

**Anti-pattern:** Calling `Update.begin()` without first tearing down the active mode. The mode may hold 20-30KB of heap (transition buffers, render state) that OTA needs. Also: implementing `prepareForOTA()` without setting `_switchState = SWITCHING`.

### Schedule Rule NVS Pattern

**Indexed key convention — matches Foundation brightness schedule approach:**

Schedule rules use indexed keys where N is 0-7. The count key `sched_r_count` tells how many rules are defined.

**Complete NVS key set per rule:**

| Key Pattern | Type | Example | Chars |
|------------|------|---------|-------|
| `sched_r{N}_start` | uint16 | `sched_r0_start` = 360 | 15 |
| `sched_r{N}_end` | uint16 | `sched_r0_end` = 1320 | 13 |
| `sched_r{N}_mode` | string | `sched_r0_mode` = `"clock"` | 14 |
| `sched_r{N}_ena` | uint8 | `sched_r0_ena` = 1 | 13 |
| `sched_r_count` | uint8 | 2 | 13 |

All keys within 15-char NVS limit. The enabled flag uses `_ena` (not `_enabled` which would be 17 chars).

**Read pattern (getModeSchedule):**
```cpp
ModeScheduleConfig config;
config.rule_count = nvs.getUChar("sched_r_count", 0);
for (uint8_t i = 0; i < config.rule_count; i++) {
    char key[16];
    snprintf(key, sizeof(key), "sched_r%d_start", i);
    config.rules[i].start_min = nvs.getUShort(key, 0);
    snprintf(key, sizeof(key), "sched_r%d_end", i);
    config.rules[i].end_min = nvs.getUShort(key, 0);
    snprintf(key, sizeof(key), "sched_r%d_mode", i);
    // ... string read for mode_id
    snprintf(key, sizeof(key), "sched_r%d_ena", i);
    config.rules[i].enabled = nvs.getUChar(key, 1);  // default enabled
}
```

**Write pattern (setModeSchedule) — rules always compacted:**
```cpp
// Rules are stored compacted — no index gaps.
// When a rule is deleted from the middle, higher-index rules shift down.
nvs.putUChar("sched_r_count", config.rule_count);
for (uint8_t i = 0; i < config.rule_count; i++) {
    char key[16];
    snprintf(key, sizeof(key), "sched_r%d_start", i);
    nvs.putUShort(key, config.rules[i].start_min);
    snprintf(key, sizeof(key), "sched_r%d_end", i);
    nvs.putUShort(key, config.rules[i].end_min);
    snprintf(key, sizeof(key), "sched_r%d_mode", i);
    // ... string write for mode_id
    snprintf(key, sizeof(key), "sched_r%d_ena", i);
    nvs.putUChar(key, config.rules[i].enabled ? 1 : 0);
}
```

**Anti-pattern:** Serializing schedule rules as a single JSON blob in one NVS string key. NVS string values are limited to ~4000 bytes and JSON parsing adds heap pressure. Use indexed keys — same pattern as the Foundation brightness schedule.

**Anti-pattern:** Leaving index gaps when deleting rules. Rules are always compacted — when rule 1 of 3 is deleted, rules 2 and 3 shift down to indices 0 and 1, and `sched_r_count` decrements to 2.

### Per-Mode Settings NVS Pattern

**Key construction — always via ConfigManager helpers:**

Mode settings use the `m_{abbrev}_{setting}` NVS key pattern. Never construct these keys manually in mode code or WebPortal handlers.

```cpp
// Correct — through ConfigManager:
int32_t fmt = ConfigManager::getModeSetting("clock", "format", 0);  // reads "m_clock_format"
ConfigManager::setModeSetting("depbd", "rows", 3);                   // writes "m_depbd_rows"

// Forbidden — manual key construction:
nvs.getUChar("m_clock_format", 0);  // ❌ bypasses ConfigManager, no default handling
```

**Schema declaration — co-located with mode class:**

Each mode's `ModeSettingDef[]` and `ModeSettingsSchema` are declared as `static const` in the mode's `.h` file, not in a centralized registry.

```cpp
// modes/ClockMode.h — settings schema co-located with mode
static const ModeSettingDef CLOCK_SETTINGS[] = {
    { "format", "Time Format", "enum", 0, 0, 1, "24h,12h" }
};
static const ModeSettingsSchema CLOCK_SCHEMA = { "clock", CLOCK_SETTINGS, 1 };
```

**Anti-pattern:** Putting all mode settings definitions in ConfigManager or a single registry file. Settings are owned by the mode — the schema lives next to the code that uses it.

**Anti-pattern:** Hardcoding setting names in API response builders. WebPortal handlers for `GET /api/display/modes` must iterate `ModeEntry.settingsSchema` dynamically to build the settings JSON. If someone adds a setting to the schema but the API handler has hardcoded field names, the new setting becomes invisible to the dashboard.

```cpp
// Correct — dynamic iteration:
if (entry.settingsSchema) {
    for (uint8_t i = 0; i < entry.settingsSchema->settingCount; i++) {
        const ModeSettingDef& def = entry.settingsSchema->settings[i];
        // build JSON from def.key, def.label, def.type, etc.
    }
}

// Forbidden — hardcoded:
json["settings"][0]["key"] = "format";  // ❌ breaks when schema changes
```

### Fade Buffer Lifecycle Pattern

**Allocate late, free early:**

Transition buffers are `malloc()`'d at the start of `_executeFadeTransition()` and `free()`'d immediately after the last blend frame. They never persist beyond the transition call.

**Rule:** If `malloc()` returns `nullptr` for either buffer, free both and return immediately. The caller treats this as "instant cut" — the mode still switches, just without the fade animation. This is graceful degradation, not an error.

```cpp
uint8_t* outgoing = (uint8_t*)malloc(bufSize);
uint8_t* incoming = (uint8_t*)malloc(bufSize);
if (!outgoing || !incoming) {
    free(outgoing); free(incoming);  // safe: free(nullptr) is no-op
    LOG_I("ModeReg", "Fade buffer alloc failed, instant cut");
    return;  // not an error — graceful degradation
}
// ... blend loop ...
free(outgoing);
free(incoming);
```

**Anti-pattern:** Pre-allocating persistent transition buffers at boot. The ~30KB would reduce available heap for the entire lifetime. Allocate only during the ~1-second transition window.

### Updated Enforcement Guidelines

**All AI agents implementing Delight stories MUST also follow (in addition to rules 1-23):**

24. Mode switches from WebPortal must go through `ModeOrchestrator::onManualSwitch()` — `ModeRegistry::requestSwitch()` is called from exactly two methods: `ModeOrchestrator::tick()` and `ModeOrchestrator::onManualSwitch()` — no other call sites
25. OTA SHA-256 verification must be computed incrementally during streaming via `mbedtls_sha256_update()` per chunk — never attempt post-download partition read-back. On any error after `Update.begin()`, always call `Update.abort()` before returning
26. Both OTA paths (Push and Pull) must call `ModeRegistry::prepareForOTA()` before `Update.begin()` — `prepareForOTA()` must set `_switchState = SWITCHING` to block the display task's `tick()` loop
27. Fade transition buffers are `malloc()`'d at transition start and `free()`'d immediately after the last blend frame — never persist transition buffers beyond the fade call, and never treat allocation failure as an error (use instant cut fallback)
28. Per-mode settings NVS keys must be read/written exclusively through `ConfigManager::getModeSetting()` / `setModeSetting()` — never construct `m_{abbrev}_{key}` keys manually. API handlers must iterate `settingsSchema` dynamically, never hardcode setting names
29. Mode settings schemas (`ModeSettingDef[]` + `ModeSettingsSchema`) are declared as `static const` in the mode's own `.h` file — never centralize settings definitions in ConfigManager or a registry
30. Cross-core `std::atomic` globals are declared in `main.cpp` only — modes and adapters must not read or write atomic globals directly; use `RenderContext` fields or core API methods instead

### Delight Release — Party Mode Refinements (Patterns)

**Reviewers:** Winston (Architect), Amelia (Developer), Quinn (QA)

**8 refinements incorporated:**

1. Rule 24 tightened to enumerate exactly two allowed `requestSwitch()` call sites (Winston)
2. New rule 30 added for cross-core atomic signaling discipline (Winston)
3. Schedule NVS pattern: added explicit `sched_r{N}_ena` key with 14-char name (Winston)
4. OTA task pattern: `_downloadTask = nullptr` before `vTaskDelete(NULL)` to prevent dangling handle (Amelia)
5. OTA error pattern: always `Update.abort()` on any error after `Update.begin()` — added to rule 25 (Amelia)
6. Pre-OTA pattern: explicitly state `prepareForOTA()` must set `_switchState = SWITCHING` — added to rule 26 (Amelia)
7. Per-mode settings anti-pattern: API handlers must iterate schema dynamically, never hardcode — added to rule 28 (Quinn)
8. Schedule delete convention: rules always compacted with no index gaps — added to NVS pattern (Quinn)

## Delight Release — Project Structure & File Change Map

### Complete Project Directory Structure (Delight Additions)

```
firmware/
├── platformio.ini                          # UNCHANGED (modes/ and utils/ already in build filter;
│                                           #  mbedTLS bundled in ESP-IDF, no extra flags needed)
├── custom_partitions.csv                   # UNCHANGED (Foundation dual-OTA layout)
│
├── interfaces/
│   ├── BaseDisplay.h                       # UNCHANGED
│   ├── BaseFlightFetcher.h                 # UNCHANGED
│   ├── BaseStateVectorFetcher.h            # UNCHANGED
│   └── DisplayMode.h                       # UPDATED: +ModeSettingDef struct,
│                                           #          +ModeSettingsSchema struct
│                                           #          (mode contract — settings declaration)
│
├── core/
│   ├── ConfigManager.h                     # UPDATED: +ModeScheduleConfig, +ScheduleRule structs,
│   │                                       #          +getModeSchedule(), +setModeSchedule(),
│   │                                       #          +getModeSetting(), +setModeSetting(),
│   │                                       #          +sched_r{N} NVS key abbreviation docs
│   ├── ConfigManager.cpp                   # UPDATED: schedule NVS read/write with compaction,
│   │                                       #          per-mode NVS helpers (m_{abbrev}_{key})
│   ├── ModeRegistry.h                      # UPDATED: +_executeFadeTransition() private,
│   │                                       #          +prepareForOTA() public static,
│   │                                       #          ModeEntry +settingsSchema field
│   ├── ModeRegistry.cpp                    # UPDATED: fade transition blend loop,
│   │                                       #          prepareForOTA() implementation
│   │                                       #          (sets _switchState = SWITCHING)
│   ├── ModeOrchestrator.h                  # ← NEW: OrchestratorState enum, static class decl
│   ├── ModeOrchestrator.cpp                # ← NEW: tick(), onManualSwitch(), state machine,
│   │                                       #         schedule evaluation, idle fallback,
│   │                                       #         timeInWindow() helper,
│   │                                       #         extern g_flightCount declaration
│   ├── OTAUpdater.h                        # ← NEW: OTAState enum, static class decl,
│   │                                       #         version/progress/error accessors
│   ├── OTAUpdater.cpp                      # ← NEW: checkForUpdate() GitHub API,
│   │                                       #         _downloadTaskFunc() FreeRTOS task,
│   │                                       #         incremental SHA-256 via mbedTLS,
│   │                                       #         startDownload() task spawning
│   ├── FlightDataFetcher.h                 # UNCHANGED
│   ├── FlightDataFetcher.cpp               # UPDATED: +g_flightCount.store() after queue write,
│   │                                       #          +extern std::atomic<uint8_t> g_flightCount
│   ├── LayoutEngine.h                      # UNCHANGED
│   ├── LayoutEngine.cpp                    # UNCHANGED
│   ├── LogoManager.h                       # UNCHANGED
│   ├── LogoManager.cpp                     # UNCHANGED
│   ├── SystemStatus.h                      # UPDATED: +OTA_PULL subsystem entry
│   └── SystemStatus.cpp                    # UPDATED: +OTA_PULL subsystem registration
│
├── modes/
│   ├── ClassicCardMode.h                   # UNCHANGED (settingsSchema nullptr is in MODE_TABLE
│   ├── ClassicCardMode.cpp                 #  in main.cpp, not in mode .h files)
│   ├── LiveFlightCardMode.h                # UNCHANGED
│   ├── LiveFlightCardMode.cpp              # UNCHANGED
│   ├── ClockMode.h                         # ← NEW: ClockMode class, CLOCK_SETTINGS[],
│   │                                       #         CLOCK_SCHEMA static const,
│   │                                       #         NTP time display, 12/24h format setting
│   ├── ClockMode.cpp                       # ← NEW: render() with time formatting,
│   │                                       #         getModeSetting() for format preference
│   ├── DeparturesBoardMode.h              # ← NEW: DeparturesBoardMode class, DEPBD_SETTINGS[],
│   │                                       #         DEPBD_SCHEMA static const,
│   │                                       #         configurable rows + telemetry fields
│   └── DeparturesBoardMode.cpp            # ← NEW: multi-row rendering, dynamic row
│                                           #         add/remove, in-place row mutation
│
├── adapters/
│   ├── NeoMatrixDisplay.h                  # UNCHANGED
│   ├── NeoMatrixDisplay.cpp                # UNCHANGED
│   ├── WebPortal.h                         # UPDATED: +_handleOTACheck(), +_handleOTAPull(),
│   │                                       #          +_handleOTAStatus(), +_handleGetSchedule(),
│   │                                       #          +_handlePostSchedule()
│   ├── WebPortal.cpp                       # UPDATED: 5 new route handlers,
│   │                                       #          updated /api/display/modes (settings array,
│   │                                       #          iterates settingsSchema dynamically),
│   │                                       #          updated POST /api/display/mode (settings obj,
│   │                                       #          routes through onManualSwitch()),
│   │                                       #          updated /api/status (ota + orchestrator)
│   ├── WiFiManager.h                       # UNCHANGED
│   ├── WiFiManager.cpp                     # UNCHANGED
│   ├── AeroAPIFetcher.h                    # UNCHANGED
│   ├── AeroAPIFetcher.cpp                  # UNCHANGED
│   ├── FlightWallFetcher.h                 # UNCHANGED
│   ├── FlightWallFetcher.cpp               # UNCHANGED
│   ├── OpenSkyFetcher.h                    # UNCHANGED
│   └── OpenSkyFetcher.cpp                  # UNCHANGED
│
├── utils/
│   ├── GeoUtils.h                          # UNCHANGED
│   ├── Log.h                               # UNCHANGED
│   ├── DisplayUtils.h                      # UNCHANGED
│   └── DisplayUtils.cpp                    # UNCHANGED
│
├── models/
│   ├── FlightInfo.h                        # UNCHANGED
│   ├── StateVector.h                       # UNCHANGED
│   └── AirportInfo.h                       # UNCHANGED
│
├── config/
│   ├── HardwareConfiguration.h             # UNCHANGED
│   ├── TimingConfiguration.h               # UNCHANGED
│   ├── WiFiConfiguration.h                 # UNCHANGED
│   ├── APIConfiguration.h                  # UNCHANGED
│   └── UserConfiguration.h                 # UNCHANGED
│
├── src/
│   └── main.cpp                            # UPDATED: +std::atomic<uint8_t> g_flightCount,
│                                           #          +ModeOrchestrator::init() in setup(),
│                                           #          +OTAUpdater::init() in setup(),
│                                           #          +ModeOrchestrator::tick() in Core 1 loop,
│                                           #          MODE_TABLE entries +settingsSchema field
│                                           #          (nullptr for existing, schema ptr for new)
│
├── data-src/
│   ├── dashboard.html                      # UPDATED: +OTA Pull section (check/download/progress),
│   │                                       #          +Schedule timeline editor section,
│   │                                       #          +per-mode settings panels in Mode Picker
│   ├── dashboard.js                        # UPDATED: +OTA Pull API calls (check/pull/status poll),
│   │                                       #          +schedule CRUD (GET/POST /api/schedule),
│   │                                       #          +per-mode settings rendering from schema,
│   │                                       #          +orchestrator state display
│   ├── style.css                           # UPDATED: +OTA progress bar, +schedule timeline,
│   │                                       #          +mode settings panel styling
│   ├── health.html                         # UNCHANGED
│   ├── health.js                           # UNCHANGED
│   ├── common.js                           # UNCHANGED
│   ├── wizard.html                         # UNCHANGED
│   └── wizard.js                           # UNCHANGED
│
├── data/
│   ├── dashboard.html.gz                   # REBUILD after dashboard.html changes
│   ├── dashboard.js.gz                     # REBUILD after dashboard.js changes
│   ├── style.css.gz                        # REBUILD after style.css changes
│   ├── common.js.gz                        # UNCHANGED
│   ├── health.html.gz                      # UNCHANGED
│   ├── health.js.gz                        # UNCHANGED
│   ├── wizard.html.gz                      # UNCHANGED
│   ├── wizard.js.gz                        # UNCHANGED
│   ├── test.txt                            # UNCHANGED
│   └── logos/                              # UNCHANGED
│
└── test/
    ├── test_config_manager/test_main.cpp   # UNCHANGED
    ├── test_layout_engine/test_main.cpp    # UNCHANGED
    ├── test_logo_manager/test_main.cpp     # UNCHANGED
    ├── test_telemetry_conversion/test_main.cpp  # UNCHANGED
    ├── test_mode_registry/test_main.cpp    # UPDATED: +fade transition lifecycle,
    │                                       #          +prepareForOTA() state tests
    ├── test_mode_orchestrator/test_main.cpp # ← NEW: state machine transitions,
    │                                       #         schedule evaluation, idle fallback,
    │                                       #         manual switch interaction
    ├── test_ota_updater/test_main.cpp      # ← NEW: version comparison, state transitions,
    │                                       #         SHA-256 verification logic (mock HTTP)
    └── test_config_schedule/test_main.cpp  # ← NEW: schedule NVS read/write,
                                            #         rule compaction on delete,
                                            #         index shifting, count tracking
```

### File Change Summary

| Action | Count | Files |
|--------|-------|-------|
| **NEW** | 8 | `core/ModeOrchestrator.h`, `core/ModeOrchestrator.cpp`, `core/OTAUpdater.h`, `core/OTAUpdater.cpp`, `modes/ClockMode.h`, `modes/ClockMode.cpp`, `modes/DeparturesBoardMode.h`, `modes/DeparturesBoardMode.cpp` |
| **NEW (tests)** | 3 | `test/test_mode_orchestrator/test_main.cpp`, `test/test_ota_updater/test_main.cpp`, `test/test_config_schedule/test_main.cpp` |
| **UPDATED** | 14 | `interfaces/DisplayMode.h`, `core/ConfigManager.h`, `core/ConfigManager.cpp`, `core/ModeRegistry.h`, `core/ModeRegistry.cpp`, `core/FlightDataFetcher.cpp`, `core/SystemStatus.h`, `core/SystemStatus.cpp`, `adapters/WebPortal.h`, `adapters/WebPortal.cpp`, `src/main.cpp`, `data-src/dashboard.html`, `data-src/dashboard.js`, `data-src/style.css` |
| **UPDATED (tests)** | 1 | `test/test_mode_registry/test_main.cpp` |
| **REBUILD** | 3 | `data/dashboard.html.gz`, `data/dashboard.js.gz`, `data/style.css.gz` |
| **UNCHANGED** | 30+ | All other firmware files, config headers, models, fetchers, WiFiManager, LayoutEngine, LogoManager, utils, wizard assets, health assets, existing mode .h/.cpp files |
| **NEW DIRECTORY** | 0 | All directories already exist from Display System |

### Struct Placement Clarification

Settings-related structs are split across two files by concern:

| Struct | File | Rationale |
|--------|------|-----------|
| `ModeSettingDef` | `interfaces/DisplayMode.h` | Part of the mode contract — modes declare settings schemas |
| `ModeSettingsSchema` | `interfaces/DisplayMode.h` | Part of the mode contract — modes declare settings schemas |
| `ModeEntry.settingsSchema` field | `core/ModeRegistry.h` | Registry owns mode table entries; field is nullable pointer |
| `ScheduleRule` | `core/ConfigManager.h` | Config storage concern — not part of mode contract |
| `ModeScheduleConfig` | `core/ConfigManager.h` | Config storage concern — not part of mode contract |

### Atomic Extern Pattern

`g_flightCount` is declared in `main.cpp` (rule 30). Files that need access use `extern`:

```cpp
// In core/FlightDataFetcher.cpp — writer:
extern std::atomic<uint8_t> g_flightCount;
// After xQueueOverwrite:
g_flightCount.store(enrichedFlights.size());

// In core/ModeOrchestrator.cpp — reader:
extern std::atomic<uint8_t> g_flightCount;
// In tick():
uint8_t flights = g_flightCount.load();
```

No other files should contain this extern declaration.

### Architectural Boundaries

**API Boundaries (HTTP):**

| Endpoint | Method | Handler | Cross-Core | Purpose |
|----------|--------|---------|------------|---------|
| `/api/ota/check` | GET | WebPortal (Core 1) | None | Check GitHub for new firmware |
| `/api/ota/pull` | POST | WebPortal (Core 1) | Spawns FreeRTOS task | Start firmware download |
| `/api/ota/status` | GET | WebPortal (Core 1) | Read OTAUpdater state | Poll download progress |
| `/api/schedule` | GET | WebPortal (Core 1) | None | Get mode schedule rules |
| `/api/schedule` | POST | WebPortal (Core 1) | None | Save mode schedule rules |
| `/api/display/modes` | GET | WebPortal (Core 1) | Read-only atomic | **UPDATED:** +settings array per mode |
| `/api/display/mode` | POST | WebPortal (Core 1) | Via ModeOrchestrator | **UPDATED:** → onManualSwitch(), +settings obj |
| `/api/status` | GET | WebPortal (Core 1) | Unchanged | **UPDATED:** +ota_available, +orchestrator_state |

**Component Boundaries (Firmware):**

```
┌──────────────────────────────────────────────────────────────────┐
│  Core 1 (WiFi/Web/API task + main loop)                          │
│  ┌────────────────┐   ┌──────────────────┐                       │
│  │   WebPortal    │   │ ModeOrchestrator │ (runs in main loop)   │
│  │  (HTTP routes) │   │   ::tick()       │                       │
│  └───────┬────────┘   └────────┬─────────┘                       │
│          │                     │                                  │
│   onManualSwitch()────────────→│                                  │
│          │                     │ requestSwitch()                  │
│          │                     ├─────────────────→ atomic write   │
│          │                     │                                  │
│  ┌───────┴────────┐   ┌───────┴──────────┐                       │
│  │  OTAUpdater    │   │ FlightDataFetcher│                       │
│  │  (FreeRTOS     │   │  g_flightCount   │                       │
│  │   download     │   │  .store()        │                       │
│  │   task)        │   └──────────────────┘                       │
│  └───────┬────────┘                                              │
└──────────┼───────────────────────────────────────────────────────┘
           │ prepareForOTA() — executes on caller's core (Core 1),
           │ but sets _switchState flag read by Core 0
           │
┌──────────┼───────────────────────────────────────────────────────┐
│  Core 0  │(Display task)                                         │
│  ┌───────▼──────────┐   ┌──────────────────┐                     │
│  │  ModeRegistry    │   │   ActiveMode     │ (DisplayMode*)      │
│  │  ::tick()        │──→│   .render()      │                     │
│  │  checks          │   └──────────────────┘                     │
│  │  _switchState    │                                            │
│  │  fade transition │                                            │
│  └──────────────────┘                                            │
│                                                                  │
│  ┌──────────────────┐   ┌──────────────────┐                     │
│  │ NeoMatrixDisplay │   │  DisplayUtils    │                     │
│  │ .show()          │   │  (free functions)│                     │
│  └──────────────────┘   └──────────────────┘                     │
└──────────────────────────────────────────────────────────────────┘
```

**Data Boundaries:**

| Boundary | Direction | Mechanism |
|----------|-----------|-----------|
| WebPortal → ModeOrchestrator | Core 1 internal | `onManualSwitch()` call (rule 24) |
| ModeOrchestrator → ModeRegistry | Core 1 → Core 0 | `requestSwitch()` atomic write |
| FlightDataFetcher → ModeOrchestrator | Core 1 internal | `g_flightCount` atomic (rule 30) |
| OTAUpdater → ModeRegistry | FreeRTOS task (Core 1) → Core 0 | `prepareForOTA()` executes on caller's core; sets `_switchState` flag that Core 0 reads — cross-core side effect, not cross-core call |
| OTAUpdater → Update.h | FreeRTOS task → flash | Stream-to-partition with incremental SHA-256 (rule 25) |
| ConfigManager → ModeOrchestrator | NVS → Core 1 | `getModeSchedule()` on tick() |
| ConfigManager → Modes | NVS → Core 0 | `getModeSetting()` called in mode `init()`, result cached; never in `render()` |
| Dashboard JS → OTA API | Browser → Core 1 | fetch() + 500ms polling for status |
| Dashboard JS → Schedule API | Browser → Core 1 | fetch() CRUD |
| Modes → ConfigManager | **Via init() only** | `getModeSetting()` in `init()`, cache result; never direct NVS access |
| Modes → NeoMatrixDisplay | **FORBIDDEN** | Modes never access NeoMatrixDisplay directly |
| Modes → g_flightCount | **FORBIDDEN** | Modes never read atomic globals (rule 30) |

### Requirements to Structure Mapping

**FR Group → File Mapping:**

| FR Group | FRs | Primary Files | Supporting Files |
|----------|-----|---------------|-----------------|
| Clock Mode | FR1-FR4 | `modes/ClockMode.h`, `modes/ClockMode.cpp` | `interfaces/DisplayMode.h` (schema structs), `core/ConfigManager` (getModeSetting), `main.cpp` (MODE_TABLE entry) |
| Departures Board | FR5-FR9 | `modes/DeparturesBoardMode.h`, `modes/DeparturesBoardMode.cpp` | `utils/DisplayUtils.h` (rendering), `core/LogoManager.h` (logos), `main.cpp` (MODE_TABLE entry) |
| Mode Transitions | FR10-FR12 | `core/ModeRegistry.h`, `core/ModeRegistry.cpp` | `interfaces/DisplayMode.h` (SwitchState) |
| Mode Scheduling | FR13-FR17, FR36, FR39 | `core/ModeOrchestrator.h`, `core/ModeOrchestrator.cpp` | `core/ConfigManager` (getModeSchedule), `main.cpp` (tick() in loop) |
| Mode Configuration | FR18-FR20, FR40 | `core/ConfigManager.h/.cpp`, mode `.h` files (schemas) | `adapters/WebPortal.cpp` (/api/display/modes iterates schema), `data-src/dashboard.js` (settings panels) |
| OTA Firmware Pull | FR21-FR31, FR37-FR38 | `core/OTAUpdater.h`, `core/OTAUpdater.cpp` | `core/ModeRegistry` (prepareForOTA), `adapters/WebPortal.cpp` (3 endpoints), `data-src/dashboard.js` (OTA UI) |
| System Resilience | FR32-FR35 | `core/ModeOrchestrator.h/.cpp`, `core/OTAUpdater.h/.cpp`, `core/ModeRegistry.h/.cpp`, `core/ConfigManager.h/.cpp` | **FR32** (watchdog → known-good mode): `ModeOrchestrator` boot/restore default mode, integrated from `main.cpp` loop. **FR33** (safe defaults for new Delight keys): `ConfigManager` first-read merge — absent keys get built-in defaults without clobbering existing NVS. **FR34** (heap / transition guards): OTA download blocked or warned below 80KB free heap — `OTAUpdater`; mode transition dual-buffer allocation or MVP fallback — `ModeRegistry` (`_executeFadeTransition`, alloc failure → instant cut). **FR35** (API + matrix responsiveness): flight pipelines — `FlightDataFetcher.cpp`, `OpenSkyFetcher.cpp`, `AeroAPIFetcher.cpp`; idle fallback and display continuity — `ModeOrchestrator`; GitHub/OTA unavailable — `OTAUpdater` + dashboard (`WebPortal`, `data-src/dashboard.js`). |
| Contributor Extensibility | FR41-FR42 | `interfaces/DisplayMode.h` (contract), `main.cpp` (MODE_TABLE) | Existing rule 22 (two touch points) |
| Documentation | FR43 | No firmware files | External documentation only |

**Cross-Cutting Concerns → File Mapping:**

| Concern | Files Touched |
|---------|--------------|
| Fade transition buffer lifecycle (DL1) | `core/ModeRegistry.h/.cpp` (_executeFadeTransition, malloc/free) |
| Mode orchestration state machine (DL2) | `core/ModeOrchestrator.h/.cpp`, `adapters/WebPortal.cpp` (onManualSwitch), `core/FlightDataFetcher.cpp` (g_flightCount), `src/main.cpp` (atomic decl + tick) |
| OTA Pull integrity (DL3) | `core/OTAUpdater.h/.cpp`, `core/ModeRegistry.cpp` (prepareForOTA) |
| Schedule NVS storage (DL4) | `core/ConfigManager.h/.cpp`, `core/ModeOrchestrator.cpp` |
| Per-mode settings schema (DL5) | `interfaces/DisplayMode.h` (structs), all new `modes/*.h` (schema decls), `core/ConfigManager.h/.cpp` (helpers), `adapters/WebPortal.cpp` (API iteration) |
| API endpoint additions (DL6) | `adapters/WebPortal.h/.cpp`, `data-src/dashboard.html/.js/.css` |
| Cross-core atomic signaling | `src/main.cpp` (declaration), `core/FlightDataFetcher.cpp` (extern + store), `core/ModeOrchestrator.cpp` (extern + load) |
| Gzip rebuild | `data/*.gz` (3 files) |

### Integration Points

**Internal Communication:**
- ModeOrchestrator runs in Core 1 main loop (~1/sec tick) — evaluates schedule rules, checks g_flightCount, calls `requestSwitch()` when state transitions require mode change
- OTAUpdater `checkForUpdate()` runs synchronously from WebPortal handler (~1-2s blocking) — acceptable for async web server single-request handling
- OTAUpdater `startDownload()` spawns FreeRTOS task, returns immediately — WebPortal handler responds with `{ ok: true }`; dashboard polls `/api/ota/status` at 500ms
- Fade transition runs on Core 0 inside `ModeRegistry::tick()` during SWITCHING state — `delay()` is acceptable since no other rendering occurs during transition

**External Integrations:**

| Service | Protocol | Purpose | New? |
|---------|----------|---------|------|
| GitHub Releases API | HTTPS GET | Check for new firmware version, download binary + .sha256 | **Yes** |
| NTP (pool.ntp.org) | UDP port 123 | Time for Clock Mode display + schedule evaluation | Existing (Foundation) |

**Data Flow (OTA Pull):**

```
Dashboard "Check for Updates" click
  → GET /api/ota/check
  → OTAUpdater::checkForUpdate()
  → GET https://api.github.com/repos/{owner}/{repo}/releases/latest
  → Parse JSON: tag_name, assets[].browser_download_url
  → Compare tag_name vs FW_VERSION
  → Return { available, version, release_notes }

Dashboard "Install Update" click
  → POST /api/ota/pull
  → OTAUpdater::startDownload() → spawns FreeRTOS task, returns immediately
  → { ok: true, data: { started: true } }
  → Dashboard polls GET /api/ota/status at 500ms

FreeRTOS download task (Core 1):
  → ModeRegistry::prepareForOTA()  (sets _switchState = SWITCHING on Core 1,
                                     blocking Core 0's tick() loop)
  → Download .sha256 file (64 hex chars)
  → Update.begin(partitionSize)
  → Stream .bin in chunks:
      Update.write(chunk) + mbedtls_sha256_update(chunk)
      _progress = bytesWritten * 100 / totalSize
  → mbedtls_sha256_finish() → compare
  → If match: Update.end(true) → ESP.restart()
  → If mismatch: Update.abort() → _state = ERROR
  → _downloadTask = nullptr → vTaskDelete(NULL)
```

**Data Flow (Mode Scheduling):**

```
ModeOrchestrator::tick() [Core 1, ~1/sec]:
  → getLocalTime(&now, 0)  // non-blocking (rule 14)
  → currentMin = hour * 60 + min
  → ConfigManager::getModeSchedule()  // read NVS
  → Evaluate rules in index order (first match wins)
  → If rule matches and state != SCHEDULED:
      _state = SCHEDULED → requestSwitch(rule.mode_id)
  → If no rule matches and state == SCHEDULED:
      _state = MANUAL → requestSwitch(_manualModeId)
  → If no rule matches and g_flightCount == 0 and state == MANUAL:
      _state = IDLE_FALLBACK → requestSwitch("clock")
  → If g_flightCount > 0 and state == IDLE_FALLBACK:
      _state = MANUAL → requestSwitch(_manualModeId)
```

### Delight Release — Party Mode Refinements (Structure)

**Reviewers:** Winston (Architect), Amelia (Developer), Quinn (QA)

**7 refinements incorporated:**

1. `SystemStatus.cpp` promoted to UPDATED — subsystem registration code lives in `.cpp` (Winston)
2. Boundary diagram: `prepareForOTA()` labeled as executing on caller's core (Core 1) with cross-core side effect via `_switchState` flag (Winston)
3. Removed `ClassicCardMode.h` and `LiveFlightCardMode.h` from UPDATED — settingsSchema is in MODE_TABLE in `main.cpp`, not in mode `.h` files (Amelia)
4. Struct placement clarified: `ModeSettingDef`/`ModeSettingsSchema` in `DisplayMode.h`, `settingsSchema` field added to `ModeEntry` in `ModeRegistry.h` (Amelia)
5. `extern std::atomic<uint8_t> g_flightCount` pattern documented for `FlightDataFetcher.cpp` and `ModeOrchestrator.cpp` (Amelia)
6. Added third test file: `test_config_schedule/test_main.cpp` for schedule NVS read/write + compaction logic (Quinn)
7. FR32-FR35 mapping clarified as "existing coverage, no Delight changes needed" for API degradation FRs (Quinn)

## Delight Release — Architecture Validation

### Coherence Validation

**Decision Compatibility:**

All 6 Delight decisions (DL1-DL6) form a consistent dependency chain with no contradictions:
- DL1 (fade transition) integrates into existing ModeRegistry switch lifecycle without modifying the DisplayMode interface
- DL2 (orchestrator) uses the existing `requestSwitch()` atomic mechanism from Display System — no new cross-core primitives needed
- DL3 (OTA Pull) shares `prepareForOTA()` with Foundation's OTA Push — both use the same `Update.h` stream-to-partition path
- DL4 (schedule NVS) and DL5 (per-mode settings NVS) use different key prefixes (`sched_r*` vs `m_*`) — no namespace collisions
- DL6 (API endpoints) follows the existing `{ ok, data/error }` JSON envelope from MVP — consistent pattern

No technology conflicts. No version incompatibilities. mbedTLS is bundled in ESP-IDF — no new external dependencies.

**Pattern Consistency:**

Rules 24-30 are additive to rules 1-23 with no contradictions:
- Rule 24 (orchestrator routing) extends rule 22 (two touch points) — modes don't initiate switches in either system
- Rule 25 (incremental SHA-256) extends rules 12-13 (OTA streaming) — same chunk-by-chunk discipline
- Rule 26 (prepareForOTA) applies to both Foundation Push and Delight Pull — unified pre-OTA sequence
- Rule 27 (fade buffers) follows rule 20 (heap in init/teardown) — transient allocation pattern
- Rule 28 (per-mode NVS helpers) follows rule 11 (NVS abbreviations in ConfigManager.h)
- Rule 30 (atomic globals in main.cpp) follows existing cross-core pattern

Naming conventions: `snake_case` flows consistently through new NVS keys (`sched_r0_start`, `m_clock_format`) → JSON fields → struct fields. PascalCase for classes (`ModeOrchestrator`, `OTAUpdater`), camelCase for methods (`onManualSwitch`, `checkForUpdate`).

**Structure Alignment:**

All Delight changes fit within existing hexagonal architecture boundaries:
- New core components (`ModeOrchestrator`, `OTAUpdater`) in `core/` — correct placement for coordination logic
- New modes (`ClockMode`, `DeparturesBoardMode`) in `modes/` — follows rule 17
- WebPortal gets new routes — same adapter pattern, no new boundary crossings
- ConfigManager gets new category helpers — consistent with existing struct pattern
- No new directories needed — Display System already created `modes/`, `utils/`

### Requirements Coverage

**All 43 Functional Requirements covered:**

| FR Group | FRs | Decision(s) | Coverage |
|----------|-----|-------------|----------|
| Clock Mode | FR1-FR4 | DL2, DL5 | ClockMode implementation + orchestrator idle fallback + 12/24h setting |
| Departures Board | FR5-FR9 | DL5 | DeparturesBoardMode implementation + configurable rows/fields settings |
| Mode Transitions | FR10-FR12 | DL1 | Fade transition with RGB888 dual buffers, double-buffering prevents tearing |
| Mode Scheduling | FR13-FR17, FR36, FR39 | DL2, DL4, DL6 | Orchestrator state machine + NVS persistence + schedule API endpoints |
| Mode Configuration | FR18-FR20, FR40 | DL5, DL6 | Per-mode settings schema + NVS helpers + Mode Picker UI + manual switch |
| OTA Firmware Pull | FR21-FR31, FR37-FR38 | DL3, DL6 | OTAUpdater component + GitHub API + SHA-256 + FreeRTOS task + 3 API endpoints |
| System Resilience | FR32-FR35 | DL1, DL3, existing | Watchdog → MANUAL boot, safe defaults, heap pre-check, API degradation (existing fetcher handling) |
| Contributor Extensibility | FR41-FR42 | Existing (rule 22) | DisplayMode contract + MODE_TABLE + Mode Picker lists all registered modes |
| Documentation | FR43 | N/A | No architectural impact — documentation validation only |

**All 21 Non-Functional Requirements architecturally addressed:**

| NFR | Target | Architectural Support |
|-----|--------|----------------------|
| NFR1 | ≥15fps transitions | DL1: FADE_STEPS=15, ~66ms/frame blend loop on Core 0 |
| NFR2 | <1 second transitions | DL1: 15 frames x ~66ms = ~1s max |
| NFR3 | ~30s idle fallback | DL2: ModeOrchestrator tick() checks g_flightCount each second |
| NFR4 | <50ms row update | DeparturesBoardMode in-place render(), no full-screen redraw |
| NFR5 | <60s OTA | DL3: stream-to-partition, ~10-15s for 1.5MB on 10Mbps |
| NFR6 | ±5s schedule accuracy | DL2: 1/sec tick with getLocalTime() non-blocking |
| NFR7 | <1s page load | Existing: gzipped assets, no external deps |
| NFR8 | <2s setting apply | DL5: ConfigManager hot-reload, no reboot |
| NFR9 | 30-day uptime | DL1 buffer lifecycle (malloc/free), DL3 task self-delete, no persistent allocations |
| NFR10 | 10s watchdog | Existing: FreeRTOS watchdog |
| NFR11 | Default mode after watchdog | DL2: ModeOrchestrator boots MANUAL state |
| NFR12 | OTA never bricks | DL3: Update.abort() on failure + Foundation dual-partition |
| NFR13 | Heap ceiling | Existing: MEMORY_REQUIREMENT check in switch flow |
| NFR14 | 2MB firmware | Monitor binary size; OTAUpdater adds ~10-15KB |
| NFR15 | NVS survives updates | NVS partition separate from app partitions |
| NFR16 | 80KB free heap | DL3: pre-download check; DL1: malloc fallback to instant cut |
| NFR17 | No heap degradation | DL1: malloc/free in single call; DL3: task self-deletes |
| NFR18 | API unavailability | Existing fetcher handling + DL3 check failure → ERROR state |
| NFR19 | Dashboard status line | DL6: GET /api/status + SystemStatus OTA_PULL entry |
| NFR20 | GitHub 429 handling | DL3: "try again later" message, no auto-retry |
| NFR21 | 10s network latency | Existing timeout handling in HTTPClient |

**Orphan check:** 0 FRs without architectural support. 0 decisions without FR traceability.

### Implementation Readiness

**Decision Completeness:** 6 decisions with code examples, interface definitions, state machine diagrams, and rationale. Implementation sequence specified with dependency graph. Cross-decision dependencies mapped.

**Structure Completeness:** Complete file change map — 11 new files (8 source + 3 test), 15 updated files. Requirements-to-file mapping explicit for all 43 FRs. Struct placement clarified across DisplayMode.h, ModeRegistry.h, and ConfigManager.h.

**Pattern Completeness:** 30 enforcement rules (11 MVP + 5 Foundation + 7 Display System + 7 Delight). All rules include code examples and anti-patterns. Cross-core signaling, OTA streaming, orchestrator routing, fade buffer lifecycle, per-mode settings NVS, and schedule NVS patterns all specified.

### Gap Analysis

**Critical Gaps:** 0

**Important Gaps:** 0

**Minor Notes (not blocking):**

1. **FR26 LED progress display** — Architecture specifies `prepareForOTA()` shows "Updating..." text on LEDs, but the exact progress bar rendering is an implementation detail. OTA download task can write progress directly to the LED array after `prepareForOTA()`.

2. **Clock Mode NTP fallback** — Architecture notes "falls back to uptime-based display if NTP unavailable" but doesn't specify exact fallback format. Implementation detail for ClockMode.render().

3. **Schedule rule naming** — Dashboard UI will need auto-generated labels (e.g., "Rule 1: 06:00-22:00 → Departures"). Pure UI rendering — no architectural decision needed.

### Architecture Completeness Checklist

**Requirements Analysis**
- [x] Project context analyzed (43 FRs, 21 NFRs, 5 party mode refinements from context review)
- [x] Scale and complexity assessed (medium — 2 new modes + orchestration + OTA)
- [x] Technical constraints identified (NVS key limits, heap budget, GitHub rate limit)
- [x] Cross-cutting concerns mapped (7 concerns including buffer lifecycle, state machine, OTA coexistence)

**Architectural Decisions**
- [x] 6 critical/important decisions documented with code examples
- [x] No new external dependencies (mbedTLS bundled in ESP-IDF)
- [x] Implementation sequence with dependency graph
- [x] Deferred decisions explicitly noted (multiple transitions, schedule templates, OTA signing)

**Implementation Patterns**
- [x] 7 new enforcement rules (24-30) with examples and anti-patterns
- [x] All rules additive — no changes to MVP (1-11), Foundation (12-16), or Display System (17-23)
- [x] Cross-core signaling pattern documented
- [x] OTA error discipline specified (Update.abort() on every error path)

**Project Structure**
- [x] Complete file change map (11 new + 15 updated + 3 rebuild)
- [x] Architectural boundaries with cross-core diagram
- [x] Data boundaries table with 12 entries
- [x] Requirements-to-file mapping for all FR groups
- [x] Atomic extern pattern documented

### Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION

**Confidence Level:** High — all FRs/NFRs covered, 3 party mode reviews with 23 total refinements incorporated, no critical or important gaps.

**Key Strengths:**
- Orchestrator state machine (DL2) cleanly separates three mode-switch sources with defined priority
- OTA Pull (DL3) reuses Foundation's streaming pattern — minimal new code surface
- Per-mode settings schema (DL5) follows existing ModeZoneDescriptor pattern — familiar to agents
- Fade transition (DL1) degrades gracefully on heap pressure — never blocks or crashes

**Architectural Overrides:**
- Context analysis section references "~20KB RGB565" buffers (from party mode round 1); decisions section corrects this to "~30KB RGB888" (party mode round 2). The decisions section is authoritative.
- FR35 API degradation marked "existing coverage" — no new Delight code needed for fetcher resilience

## Display System Release — Architecture Validation

### Coherence Validation

**Decision Compatibility:**

All 7 decisions (D1-D7) form a consistent dependency chain with no contradictions:
- D1 (interface) defines the contract that D2 (registry) manages and D3 (NeoMatrixDisplay) supports
- D4 (display task) integrates D2 and D3 at the system level
- D5 (API) and D7 (UI) operate through D2's public interface
- D6 (NVS) is called from D2's tick() debounce path
- The D1/D2 `getMemoryRequirement()` contradiction (instance method vs static function pointer) was caught and resolved in party mode review #2
- No version conflicts — all existing ESP32 Arduino libraries remain at current versions

**Pattern Consistency:**

- Enforcement rules 17-23 are additive to rules 1-16 with no conflicts
- Include path convention (path-from-firmware-root) is consistent with existing codebase
- API endpoint pattern (`/api/display/*`) follows existing `/api/*` JSON envelope convention
- NVS key pattern (`display_mode`) follows existing namespace convention
- Web asset pattern (fetch + json.ok + showToast) carried forward to Mode Picker

**Structure Alignment:**

- File change map covers all architectural decisions with explicit file locations
- PlatformIO build config updated for new directories (blocker caught in party mode #3)
- Boundaries clearly enforce: modes never touch ConfigManager/WiFiManager/SystemStatus (rule 18), never call FastLED.show() (rule 19)
- Two-touch-point extensibility pattern (rule 22) verified against file structure

### Requirements Coverage

**All 36 Functional Requirements covered.** Every FR maps to at least one architectural decision with explicit code location:
- DisplayMode Interface (FR1-FR4): Decision D1 — `interfaces/DisplayMode.h`
- Mode Registry & Lifecycle (FR5-FR10): Decision D2 — `core/ModeRegistry.h/.cpp`, `src/main.cpp`
- Classic Card Mode (FR11-FR13): Decision D3 — `modes/ClassicCardMode.h/.cpp`
- Live Flight Card Mode (FR14-FR16): New mode — `modes/LiveFlightCardMode.h/.cpp`
- Mode Picker UI (FR17-FR26): Decision D7 — `data-src/dashboard.html/js/css`
- Mode Persistence (FR27-FR29): Decision D6 — `core/ConfigManager.h/.cpp`, `src/main.cpp`
- Mode Switch API (FR30-FR33): Decision D5 — `adapters/WebPortal.h/.cpp`
- Display Infrastructure (FR34-FR36): Decisions D3 + D4 — `adapters/NeoMatrixDisplay.h/.cpp`, `src/main.cpp`

**All 12 NFRs architecturally addressed:**
- Performance: cooperative rendering within 16ms frame budget (P2), sub-2s mode switch (P1), zero-heap API enumeration (P4), <1s UI load (P3)
- Stability: sequential teardown→init for 100-switch heap stability (S1), switch serialization for rapid-switch safety (S2), heap-in-init discipline (S3), 30KB heap floor guard (S4), read-only brightness (S5), verified NVS namespace (S6)
- Compatibility: RenderContext isolation guarantees Foundation features untouched (C2), pixel parity via extraction validation (C1), cooperative scheduling preserved (C3), dashboard consistency (C4)

### Implementation Readiness

**Decision Completeness:** 7 decisions with code examples, struct definitions, switch flow pseudocode, API request/response formats, and rationale. Memory requirement pattern resolved (static constexpr + function pointer). Switch restoration pattern specified (teardown-but-don't-delete).

**Structure Completeness:** 10 NEW files + 12 UPDATED files + 1 new directory. PlatformIO build config updated. Include path conventions documented. Requirements-to-file mapping explicit. No orphaned files, no unmapped FRs.

**Pattern Completeness:** 23 enforcement rules (11 MVP + 5 Foundation + 7 Display System). Mode implementation pattern, rendering context discipline, shared utilities pattern, registration pattern, and UI patterns all specified with code examples and anti-patterns.

### Revised Implementation Sequence

**Critical path (three-phase ClassicCardMode extraction):**

1. **Heap baseline measurement** — `ESP.getFreeHeap()` log after full Foundation boot, before any Display System code
2. **DisplayMode interface + RenderContext** (`interfaces/DisplayMode.h`) — everything depends on this contract
3. **ModeRegistry** (`core/ModeRegistry.h/.cpp`) — switch logic, heap guard, tick(), terminal fallback path
4. **DisplayUtils extraction** (`utils/DisplayUtils.h/.cpp`) — extract shared helpers from NeoMatrixDisplay first
5. **ClassicCardMode — three-phase extraction:**
   - 5a: Create ClassicCardMode with COPIED rendering logic (NeoMatrixDisplay keeps its methods — both paths active)
   - 5b: Validate pixel parity — run both old and new paths against same flight data, compare output on hardware (human-in-the-loop gate)
   - 5c: Remove extracted methods from NeoMatrixDisplay, add `show()` + `buildRenderContext()` + `displayFallbackCard()`
6. **Display task integration** (`main.cpp`) — replace renderFlight with ModeRegistry::tick() + show()
7. **LiveFlightCardMode** (`modes/LiveFlightCardMode.h/.cpp`) — second mode, validates the abstraction works

**Parallel with steps 4-7:**
8. **Mode Switch API** (`WebPortal.cpp`) — GET + POST endpoints, can be stubbed early
9. **NVS persistence** (`ConfigManager.h/.cpp`) — `getDisplayMode()` / `setDisplayMode()`, trivial addition

**After all firmware complete:**
10. **Mode Picker UI** (`dashboard.html/js/css`) — depends on working API endpoints
11. **Gzip rebuild** — all updated web assets

### Architectural Overrides

**FR4 zone bounds — developer discipline, not code enforcement:**

FR4 states "display mode can render flight data within its allocated zone bounds without affecting pixels outside its zone." The architecture provides zone bounds via `LayoutResult` in `RenderContext`, but does not enforce clipping at the GFX layer — modes receive the full `Adafruit_NeoMatrix*` object. Adafruit GFX/NeoMatrix does not natively support clipping rectangles in the way needed. Zone discipline is enforced by developer practice, code review, and enforcement rule 18 (RenderContext discipline). Story acceptance criteria for each mode should include "visual verification that rendering stays within zone bounds."

**NFR P2 LogoManager exception — grandfathered filesystem I/O:**

NFR P2 prohibits filesystem I/O on the hot path. `LogoManager::loadLogo()` reads from LittleFS and is called from mode `render()` methods. This is inherited behavior from the pre-mode-system codebase — the same LittleFS reads occurred in `NeoMatrixDisplay::renderLogoZone()` before extraction. LittleFS reads for 2KB logo files on ESP32 flash complete in <1ms, well within the 16ms frame budget. NFR P2's filesystem prohibition applies to NEW mode code; LogoManager access is the single grandfathered exception.

### Terminal Fallback Path (FR36 Safety Net)

The D2 switch flow specifies teardown-but-don't-delete for safe restoration. Party mode review identified a gap: what happens if re-initialization of the previous mode ALSO fails?

**Complete terminal path:**

```
Switch requested → teardown current → heap check fails → re-init previous
                                                            ↓
                                                     init() succeeds → restored ✅
                                                     init() fails → _activeMode = nullptr
                                                            ↓
                                                     Display task detects nullptr:
                                                     g_display.displayFallbackCard(flights) → FR36 ✅
```

`displayFallbackCard()` uses `displaySingleFlightCard()` — the legacy single-flight renderer that has zero heap dependencies. This is the terminal safety net. If reached, the wall displays a degraded single-flight card, and the error propagates to the Mode Picker UI via `getLastError()`.

### Flight Cycling Pattern (Highest-Risk Extraction Point)

Party mode review identified flight cycling timing as the single most likely implementation error. The cycling check must use the configured interval from RenderContext, not the frame rate:

```cpp
void ClassicCardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    if (flights.empty()) {
        // Mode owns its idle state — show "..." or blank
        ctx.matrix->fillScreen(0);
        return;
    }

    // Advance flight index on configured interval (NOT every frame)
    // render() is called ~20fps (50ms). displayCycleMs is ~5000ms.
    // This check passes once every ~100 frames.
    if (millis() - _lastCycleMs >= ctx.displayCycleMs) {
        _currentFlightIndex = (_currentFlightIndex + 1) % flights.size();
        _lastCycleMs = millis();
    }

    // Bounds check in case flights vector shrank between cycles
    if (_currentFlightIndex >= flights.size()) {
        _currentFlightIndex = 0;
    }

    const auto& flight = flights[_currentFlightIndex];
    // ... render the current flight using DisplayUtils helpers ...
}
```

**Anti-pattern:** Do NOT use `delay()` or `vTaskDelay()` inside `render()`. Do NOT assume render() is called at any specific frequency. Always use `millis()` delta against `ctx.displayCycleMs`.

### NFR Validation Notes

**NFR S1/S2 stress testing:**

100-switch heap stability (S1) and 10-in-3-seconds rapid-switch safety (S2) require hardware stress testing that cannot be automated in unit tests. Implementation handoff requirement: add a `#ifdef DEBUG` serial command handler in `main.cpp` that accepts a `STRESS_SWITCH` command — triggers 100 sequential mode switches with before/after `ESP.getFreeHeap()` measurement and pass/fail output to serial. This is a one-time integration validation, not CI.

**NFR C1 pixel parity:**

Classic Card pixel parity is a human-in-the-loop validation gate. The implementing agent produces the extracted ClassicCardMode code. The human validates on physical hardware by comparing pre-extraction and post-extraction output for at least 5 distinct flight cards. Story acceptance criteria must reflect this split: agent delivers code, human confirms visual parity.

### Gap Analysis

| Priority | Gap | Resolution |
|----------|-----|-----------|
| Minor | `_switchState` read cross-core without atomic | ESP32 Xtensa: byte-sized enum reads are hardware-atomic. Safe in practice. Note for implementor: change to `std::atomic<SwitchState>` if portability desired. |
| Minor | FR7 "queuing" vs atomic overwrite | Architecture uses last-writer-wins during IDLE, SWITCH_IN_PROGRESS rejection during SWITCHING. Matches FR7 intent — concurrent requests serialized, not dropped silently. |
| Minor | `displayFallbackCard()` vs `displaySingleFlightCard()` naming | `displayFallbackCard()` wraps existing `displaySingleFlightCard()` logic. Implementor keeps internal method, adds public wrapper. |

No critical gaps. No important gaps.

### Display System Architecture Completeness Checklist

- [x] Display System PRD context analyzed (36 FRs, 12 NFRs, constraints)
- [x] Scale assessed (medium-high — plugin architecture on memory-constrained MCU)
- [x] Constraints identified (ESP32 heap, single display task, cross-core coordination)
- [x] Cross-cutting concerns mapped (RenderContext isolation, heap lifecycle, NVS debounce)
- [x] No new external dependencies (all existing ESP32 Arduino libraries)
- [x] 7 Display System decisions documented with code examples
- [x] Implementation patterns extended (rules 17-23)
- [x] NVS key table expanded (1 new key: `display_mode`)
- [x] 2 new API endpoints specified with request/response formats
- [x] Complete file change map (10 NEW + 12 UPDATED files)
- [x] Requirements-to-file mapping complete (36 FRs → specific files)
- [x] PlatformIO build config updated (blocker caught in party mode #3)
- [x] Architectural boundaries documented with forbidden paths
- [x] Three-phase ClassicCardMode extraction sequence specified
- [x] Terminal fallback path for FR36 documented
- [x] Flight cycling pattern with anti-patterns documented
- [x] NFR S1/S2 stress test mechanism specified
- [x] NFR C1 human-in-the-loop gate documented
- [x] All party mode refinements incorporated (3 sessions, 17+ refinements total)

### Display System Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION
**Confidence Level:** High

**Key Strengths:**
- Clean plugin architecture on ESP32 — modes isolated via RenderContext with zero subsystem coupling
- Three-phase extraction sequence prevents the highest-risk failure (ClassicCardMode pixel parity regression)
- Heap lifecycle management with terminal fallback path — wall always displays something, even after cascading failures
- Two-touch-point extensibility (rule 22) — adding a mode is one file + one line
- Cross-core coordination mirrors existing `g_configChanged` pattern — no new concurrency primitives
- NVS debounce prevents flash wear during rapid switching
- Party mode reviews across 3 sessions caught: build blocker (platformio.ini), D1/D2 contradiction (memory requirement), extraction ordering risk, terminal fallback gap, cycling timing as highest-risk point
- All 23 enforcement rules are additive — no changes to MVP (1-11) or Foundation (12-16) rules

**Areas for Future Enhancement:**
- Mode-specific user settings (per-mode NVS keys and UI panels)
- Animated transitions between modes (blank transition only in this release)
- Mode scheduling (auto-switch by time of day)
- GFX clipping enforcement for third-party mode safety
- Automated pixel parity testing (screenshot comparison tooling)

### Display System Implementation Handoff

**Pre-implementation gates:**
1. Measure heap baseline: `ESP.getFreeHeap()` after full Foundation boot (WiFi + web server + NTP + flight data). This establishes the mode memory budget.
2. If free heap is below 60KB: investigate memory reduction before proceeding (30KB floor + headroom for two modes).
3. Photograph/record current Classic Card display with 5+ distinct flights — this is the pixel parity reference.

**First implementation priority:**
1. Create `interfaces/DisplayMode.h` (RenderContext, ModeZoneDescriptor, DisplayMode abstract class)
2. Create `core/ModeRegistry.h/.cpp` (static table, tick(), switch lifecycle with terminal fallback)
3. Extract `utils/DisplayUtils.h/.cpp` from NeoMatrixDisplay helpers
4. Create `modes/ClassicCardMode.h/.cpp` (copy — not extract — rendering logic from NeoMatrixDisplay)
5. Validate pixel parity on hardware (human gate)
6. Refactor NeoMatrixDisplay (remove extracted methods, add show() + buildRenderContext())
7. Update `platformio.ini` build_src_filter and build_flags
8. Integrate display task in `main.cpp` (MODE_TABLE, ModeRegistry::init(), tick()+show())
9. Verify firmware builds and ClassicCardMode renders correctly through ModeRegistry

**NFR validation requirements:**
- Add `#ifdef DEBUG` serial stress test handler for S1/S2 validation
- NFR C1 pixel parity is a manual hardware gate — agent delivers code, human confirms


]]></file>
<file id="d7cf6d0a" path="_bmad-output/planning-artifacts/epics/index.md" label="EPIC"><![CDATA[

# Epics (sharded)

This index exists so `bmad-assist` treats `epics/` as the primary sharded epic source (see `resolve_doc_path` in bmad-assist). Epic files not linked below are still loaded as orphans in filename order.


]]></file>
<file id="46fc29ad" path="_bmad-output/implementation-artifacts/stories/fn-3-1-wizard-step-6-test-your-wall.md" label="DOCUMENTATION"><![CDATA[

# Story fn-3.1: Wizard Step 6 — Test Your Wall

Status: complete

## Story

As a new user completing the setup wizard,
I want the wizard to test my LED panel layout and let me confirm it looks correct before seeing flight data,
So that I catch wiring problems immediately instead of debugging garbled output later.

## Acceptance Criteria

1. **Automatic Pattern Trigger:**
   - Given the user completes Step 5 (Review) and taps "Next"
   - When Step 6 ("Test Your Wall") loads
   - Then the device auto-runs the panel positioning test pattern on the LED matrix via `POST /api/positioning/start`
   - And the pattern renders on the LED matrix within 500ms of being triggered
   - And a matching canvas preview renders in the browser showing the expected numbered panel layout

2. **Confirmation UI:**
   - Given Step 6 is displayed with canvas preview and LED pattern both visible
   - When rendered
   - Then the wizard asks: "Does your wall match this layout?"
   - And two buttons are shown: "Yes, it matches" (primary) and "No — take me back" (secondary)
   - And the primary button follows the one-primary-per-context rule
   - And the standard nav bar (Back/Next) is hidden for Step 6 (replaced by the confirmation buttons)

3. **Back Navigation (Guided Correction):**
   - Given the user taps "No — take me back"
   - When the wizard navigates
   - Then `POST /api/positioning/stop` is called to turn off the test pattern
   - Then the user returns to **Step 4 (Hardware configuration)** — not the immediately previous step (Step 5)
   - And all previously entered values are preserved (via existing `state` object)
   - And the user can change origin corner, scan direction, zigzag, or GPIO pin and return to Step 6 to re-test

4. **RGB Color Test:**
   - Given the user taps "Yes, it matches"
   - When the confirmation is received
   - Then `POST /api/positioning/stop` is called to turn off the positioning pattern
   - Then an RGB color test sequence runs on the LED matrix: solid red → solid green → solid blue (each held ~1 second)
   - And the wizard shows "Testing colors..." text during the sequence
   - And each color is sent via `POST /api/settings` with the respective RGB values at brightness 40, then the original brightness and text color values (from the wizard `state` object) are restored via a final `POST /api/settings`

5. **Completion Flow:**
   - Given the RGB color test completes
   - When the sequence finishes
   - Then the wizard displays "Your FlightWall is ready! Fetching your first flights..."
   - And all wizard settings are saved via `POST /api/settings` (same payload as current Step 5 `saveAndConnect()`)
   - And `POST /api/reboot` is called to trigger AP→STA transition
   - And the handoff message is displayed (phone loses AP connection — expected behavior)

6. **Accessibility & Responsive Design:**
   - Given all Step 6 UI when rendered
   - Then all buttons have 44x44px minimum touch targets
   - And all components meet WCAG 2.1 AA contrast requirements
   - And all components work at 320px minimum viewport width
   - And the canvas preview scales proportionally within the card width

7. **Dashboard Post-Setup Integration:**
   - Given the Calibration card on the dashboard
   - Then the existing "Panel Position" toggle button (already present in `#cal-pattern-toggle`) satisfies this requirement — tapping it calls `POST /api/positioning/start` and `POST /api/positioning/stop` via the existing `startPositioningMode()` / `stopPositioningMode()` functions in dashboard.js
   - No dashboard code changes are required for this AC

8. **Asset Deployment:**
   - Given updated wizard web assets (wizard.html, wizard.js)
   - When gzipped and placed in `firmware/data/`
   - Then the gzipped copies replace existing ones for LittleFS upload
   - And total gzipped web asset size remains under 50KB budget

## Tasks / Subtasks

- [x] Task 1: Add Step 6 HTML to wizard.html (AC: #1, #2, #6)
  - [x] 1.1: Add `<section id="step-6" class="step">` with heading "Test Your Wall"
  - [x] 1.2: Add `<canvas id="wizard-position-preview">` for the panel layout preview
  - [x] 1.3: Add description text: "Does your wall match this layout?"
  - [x] 1.4: Add "Yes, it matches" primary button and "No — take me back" secondary button
  - [x] 1.5: Add "Testing colors..." status text (hidden by default)
  - [x] 1.6: Add "Your FlightWall is ready!" completion text (hidden by default)
  - [x] 1.7: Update `<p id="progress">` counter — TOTAL_STEPS from 5 to 6

- [x] Task 2: Add Step 6 JS logic to wizard.js (AC: #1, #3, #4, #5)
  - [x] 2.1: Update `TOTAL_STEPS` constant from 5 to 6
  - [x] 2.2: Add `showStep(6)` handler: call `POST /api/positioning/start` (on failure show error toast via `FW.showToast()` and remain on Step 5), render canvas preview, hide nav bar, show confirmation buttons
  - [x] 2.3: Implement canvas rendering — port the tile positioning preview algorithm from dashboard.js `renderWiringCanvas()` (line 1947; draws numbered colored tiles matching the LED pattern)
  - [x] 2.4: Add "No — take me back" handler: call `POST /api/positioning/stop` (on failure show error toast but still navigate — user must not be stuck on Step 6), then `showStep(4)` (jump to Hardware step)
  - [x] 2.5: Add "Yes, it matches" handler: call `POST /api/positioning/stop`, then run RGB sequence
  - [x] 2.6: Implement RGB color test sequence: capture original brightness and text color from `state` before starting; POST brightness=40 + R/G/B values with 1s delays using `setTimeout` chain; restore original values via final `POST /api/settings` on completion or on any failure (show error toast on failure per Enforcement Rule 10)
  - [x] 2.7: After RGB sequence: call existing `saveAndConnect()` function (reuse Step 5 save + reboot logic)
  - [x] 2.8: Update `showStep()` to hide/show nav bar for Step 6 vs other steps
  - [x] 2.9: Update Step 5 (Review) to be a pass-through — "Next" on Step 5 now goes to Step 6 instead of calling `saveAndConnect()`
  - [x] 2.10: Move `saveAndConnect()` call from Step 5's "Next" handler to Step 6 completion

- [x] Task 3: Add Step 6 CSS to style.css (AC: #2, #6)
  - [x] 3.1: Style `.wizard-test-canvas` — responsive canvas container, proportional scaling
  - [x] 3.2: Style confirmation buttons — primary/secondary pair, 44px min touch targets
  - [x] 3.3: Style status text elements (testing colors, completion message)
  - [x] 3.4: Ensure 320px minimum viewport compatibility

- [x] Task 4: Verify dashboard Panel Position toggle satisfies AC #7 (no code changes required)
  - [x] 4.1: Confirm "Panel Position" toggle button (`#cal-pattern-toggle` data-pattern="1") in the Calibration card calls `startPositioningMode()` / `stopPositioningMode()` via the existing `activatePattern()` handler in dashboard.js
  - [x] 4.2: Confirm behavior matches AC #7 intent — no new dashboard HTML or JS required

- [x] Task 5: Rebuild gzipped assets (AC: #8)
  - [x] 5.1: `gzip -9 -c firmware/data-src/wizard.html > firmware/data/wizard.html.gz`
  - [x] 5.2: `gzip -9 -c firmware/data-src/wizard.js > firmware/data/wizard.js.gz`
  - [x] 5.3: `gzip -9 -c firmware/data-src/style.css > firmware/data/style.css.gz`
  - [x] 5.4: Verify total gzipped asset size remains under 50KB

## Dev Notes

### Critical Architecture Constraints

- **No new firmware endpoints required.** Step 6 reuses existing `POST /api/positioning/start`, `POST /api/positioning/stop`, and `POST /api/settings` endpoints. All changes are web asset only (HTML/JS/CSS). [Source: architecture.md#Decision-4-Web-API-Endpoints]
- **No new NVS keys.** This story uses existing config keys only.
- **Enforcement Rules 10:** Every JS `fetch()` call must check `json.ok` and handle failure with `showToast()`. Use `FW.post()` and `FW.get()` from common.js for all API calls. [Source: architecture.md#Enforcement-Guidelines]
- **Web asset budget:** Total gzipped assets must stay under 50KB. Current total is ~46KB. This story adds ~30-50 lines of HTML, ~100-150 lines of JS, ~30 lines of CSS — expect minimal size impact (~0.5-1KB gzipped). [Source: architecture.md#Web-Assets-Location]
- **Gzip build process:** No automated script. After editing `data-src/` files, manually run `gzip -9 -c data-src/X > data/X.gz` from `firmware/` directory. [Source: MEMORY.md]

### Existing Code Patterns to Reuse

**Step Navigation Pattern** (wizard.js lines 415-465):
- Steps are `<section id="step-N" class="step">` elements toggled via `.active` class
- `showStep(n)` handles display toggling, progress text update, Back button visibility, and field rehydration
- `TOTAL_STEPS` constant controls the step count (currently 5, must become 6)
- `validateStep(n)` is called before advancing — Step 6 needs no field validation (it's a visual confirmation)
- `saveCurrentStepState()` saves form fields to the `state` object before navigation

**Save & Connect Pattern** (wizard.js lines 582-634):
- `saveAndConnect()` builds a payload from `state` object, merges `importedExtras`, POSTs to `/api/settings`, then POSTs to `/api/reboot`, then calls `showHandoff()`
- This function must be called from Step 6 completion instead of Step 5
- The `btnNext` click handler (line 662) currently calls `saveAndConnect()` when `currentStep === TOTAL_STEPS` — changing TOTAL_STEPS to 6 means Step 5 "Next" now advances to Step 6, and Step 6's "Yes" button triggers save

**Canvas Positioning Preview** (dashboard.js `renderWiringCanvas()` line 1947):
- The dashboard already renders a tile positioning canvas showing numbered, color-coded tiles
- Algorithm: iterates tilesX * tilesY, draws colored rectangles with bright borders and white digit numbers
- This algorithm must be ported/adapted for the wizard canvas (cannot import from dashboard.js — they're separate gzipped files)
- The LED-side positioning pattern is rendered by `NeoMatrixDisplay::renderPositioningPattern()` (NeoMatrixDisplay.cpp line 692) — it draws numbered tiles with unique hues, bright borders, red corner markers, and 3x5 pixel font digits

**Button Styling Pattern** (style.css):
- Primary button: `.nav-btn-primary` class (blue background)
- Secondary button: `.nav-btn` class without primary modifier (gray/outline)
- Save button: `.nav-btn-save` class (green background, used on Step 5)
- All nav buttons already have proper touch target sizing (44px min)

### RGB Color Test Implementation

The RGB test sequence should:
1. Call `POST /api/settings` with `{ brightness: 40, text_color_r: 255, text_color_g: 0, text_color_b: 0 }` (red)
2. Wait ~1 second
3. Call `POST /api/settings` with `{ brightness: 40, text_color_r: 0, text_color_g: 255, text_color_b: 0 }` (green)
4. Wait ~1 second
5. Call `POST /api/settings` with `{ brightness: 40, text_color_r: 0, text_color_g: 0, text_color_b: 255 }` (blue)
6. Wait ~1 second
7. Proceed to save & connect

**Implementation approach:** Positioning mode must be stopped first (via `/api/positioning/stop`) before the RGB sequence begins, because calibration/positioning modes take display priority over normal rendering. With positioning stopped, the display task reads `text_color_r/g/b` from config via the `g_configChanged` atomic flag (hot-reload path, applies within <1 second). In AP mode without flight data, the display shows a loading/status screen rendered in the configured text color — this is sufficient to verify each RGB channel independently. No new firmware endpoints are required.

Before starting the sequence, capture the original brightness and text color values from `state` (populated by `hydrateDefaults()`). After the sequence completes — or on any API failure — restore these values via a final `POST /api/settings`. Wrap each API call with `json.ok` check and `FW.showToast()` on error per Enforcement Rule 10.

### Step 5 → Step 6 Flow Change

Currently Step 5 is the final step with "Save & Connect" button. After this story:
- Step 5 "Review" becomes a regular step with "Next →" button (not "Save & Connect")
- Step 6 "Test Your Wall" becomes the final step
- The "Save & Connect" action moves from the Step 5 "Next" handler to the Step 6 "Yes, it matches" → RGB test → completion flow

**Key change in `showStep()`:** The `btnNext` text/class logic (lines 424-429) checks `n < TOTAL_STEPS` vs `n === TOTAL_STEPS`. Changing `TOTAL_STEPS` from 5 to 6 automatically makes Step 5 a "Next →" step. Step 6 has its own buttons and hides the nav bar entirely.

**Key change in `btnNext` click handler** (line 662): When `currentStep === TOTAL_STEPS` (now 6), the handler should NOT call `saveAndConnect()` — Step 6 has its own "Yes" button that handles completion. Instead, treat Step 6's "Next" as unreachable (nav bar hidden) or guard with `if (currentStep === 6) return;`.

### Canvas Preview Algorithm

Port from dashboard.js `renderWiringCanvas()` (line 1947; the panel-level tile positioning preview). The algorithm:

1. Read tile dimensions from wizard `state` object: `tiles_x`, `tiles_y`, `tile_pixels`
2. Calculate canvas dimensions proportionally (scale to fit card width)
3. For each tile `(row, col)`:
   - Compute `gridIdx = row * tilesX + col`
   - Assign a unique hue: `hue = gridIdx * 360 / totalTiles`
   - Draw a filled rectangle with the tile's color
   - Draw a bright border around the tile
   - Draw a red corner marker at top-left (orientation guide)
   - Draw the tile number (gridIdx) centered in white

This MUST match the LED positioning pattern rendered by `NeoMatrixDisplay::renderPositioningPattern()` so the user can compare browser vs wall.

### Hardware Config Fields in Step 4

Step 4 currently only has: `tiles_x`, `tiles_y`, `tile_pixels`, `display_pin`. The positioning pattern also uses wiring flags (`origin_corner`, `scan_dir`, `zigzag`) which are NOT in the wizard. These default to 0/0/0.

For the "No — take me back" correction flow: the user can only adjust `tiles_x`, `tiles_y`, `tile_pixels`, and `display_pin` in the wizard. If the pattern shows tiles in the wrong ORDER (origin/scan issue), the wizard can't fix it — the user would need the dashboard's calibration card post-setup. The epic's AC says users can "change origin corner, scan direction, or GPIO pin" — this implies adding wiring flag fields to Step 4 or creating a minimal version.

**Decision needed:** The epic AC #3 explicitly says "change origin corner, scan direction." This means Step 4 needs **origin_corner** and **scan_dir** fields (and possibly **zigzag**) added. These are existing NVS keys (`origin_corner`, `scan_dir`, `zigzag`) already handled by `POST /api/settings`. Add them to Step 4's form fields, state object, validation, and the save payload.

### Adding Wiring Flags to Step 4

Add three new fields to Step 4 (Hardware):
- **Origin Corner:** `<select id="origin-corner">` with options: "Top-Left (0)", "Top-Right (1)", "Bottom-Left (2)", "Bottom-Right (3)"
- **Scan Direction:** `<select id="scan-dir">` with options: "Rows (0)", "Columns (1)"
- **Zigzag:** `<select id="zigzag">` with options: "Progressive (0)", "Zigzag (1)"

These must be:
1. Added to `state` object with defaults from `hydrateDefaults()` (read from `/api/settings`)
2. Added to `saveCurrentStepState()` for Step 4
3. Added to `showStep(4)` rehydration
4. Added to the `saveAndConnect()` payload
5. Added to `buildReview()` for Step 5 display
6. Added to `WIZARD_KEYS` array for settings import support

**Valid default values:** origin_corner=0, scan_dir=0, zigzag=0

### Project Structure Notes

- **Files to modify:**
  - `firmware/data-src/wizard.html` — add Step 6 section, add wiring fields to Step 4
  - `firmware/data-src/wizard.js` — add Step 6 logic, canvas rendering, RGB test, wiring state/validation
  - `firmware/data-src/style.css` — add Step 6 styles (canvas container, confirmation buttons)
- **Files to rebuild (gzip):**
  - `firmware/data/wizard.html.gz`
  - `firmware/data/wizard.js.gz`
  - `firmware/data/style.css.gz`
- **No firmware C++ changes required** — all existing endpoints sufficient
- **Alignment:** All paths match project structure conventions. Web sources in `data-src/`, gzipped in `data/`. [Source: _bmad-output/project-context.md]

### Existing Wizard State Keys

Current `WIZARD_KEYS` array (wizard.js):
```javascript
['wifi_ssid', 'wifi_password', 'os_client_id', 'os_client_sec',
 'aeroapi_key', 'center_lat', 'center_lon', 'radius_km',
 'tiles_x', 'tiles_y', 'tile_pixels', 'display_pin']
```

Add: `'origin_corner', 'scan_dir', 'zigzag'` — bringing total to 15 wizard keys.

### `hydrateDefaults()` Pattern

The existing function (wizard.js) calls `FW.get('/api/settings')` and populates `state` from the response. Add the three new wiring keys with fallback defaults of 0.

### References

- [Source: _bmad-output/planning-artifacts/epics/fn-3-onboarding-polish.md — Story fn-3.1 full spec]
- [Source: _bmad-output/planning-artifacts/prd-foundation.md — FR25-FR29, NFR-P5]
- [Source: _bmad-output/planning-artifacts/architecture.md — Decision 4 (API Endpoints), Decision F4 (Timezone), Enforcement Rules]
- [Source: firmware/data-src/wizard.js — existing step navigation, state management, saveAndConnect()]
- [Source: firmware/data-src/wizard.html — existing 5-step structure]
- [Source: firmware/data-src/dashboard.js — `renderWiringCanvas()` line 1947 (tile positioning preview), `renderCalibrationCanvas()` line 1654 (pixel scan order)]
- [Source: firmware/adapters/NeoMatrixDisplay.cpp — renderPositioningPattern() line 692]
- [Source: firmware/adapters/WebPortal.cpp — /api/positioning/start|stop, /api/calibration/start|stop]
- [Source: firmware/data-src/common.js — FW.post(), FW.get(), FW.showToast()]

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-5 (implementation) / claude-sonnet-4-6 (code review synthesis)

### Debug Log References

N/A — all changes are web asset only (HTML/JS/CSS + test infrastructure).

### Completion Notes List

**Implementation Validation — 2026-04-13:**
- All 5 tasks (31 subtasks) verified complete against source files on disk.
- Gzipped assets verified: `TOTAL_STEPS=6` and `cachedDeviceColors` confirmed in decompressed `wizard.js.gz`; `step-6` section confirmed in decompressed `wizard.html.gz`.
- Total gzipped asset size: 49.07 KB (under 50 KB budget — AC #8).
- Dashboard Panel Position toggle (AC #7) verified: `startPositioningMode()`/`stopPositioningMode()` in dashboard.js call `/api/positioning/start|stop` via existing `activatePattern()` handler. No dashboard changes required.
- WizardPage.ts TypeScript compilation: clean (pre-existing TS strictness issue in api-helpers.ts is unrelated).
- All 11 code review findings from prior synthesis pass confirmed resolved in source.

**Code Review Synthesis — 2026-04-13:**
- Fixed `enterStep6()` to navigate back to Step 5 when `/api/positioning/start` fails (AC1 compliance).
- Fixed `showSaveError()` to inject errors into the active Step 6 container instead of the hidden Step 5 section; restored `btnTestYes`/`btnTestNo` visibility on save failure so the user can retry.
- Fixed `saveAndConnect()` message sequencing: removed premature "Saving..." override; "Your FlightWall is ready!" now shows after successful reboot response (AC5).
- Fixed `renderPositionCanvas()` to render colored tile backgrounds with unique hues + bright white borders + red corner marker on tile 0, matching `NeoMatrixDisplay::renderPositioningPattern()`. Added 400px height cap to prevent unbounded canvas growth on extreme aspect ratios.
- Fixed `runRgbTest()` original-value capture: now resolves from `importedExtras` first, then from `cachedDeviceColors` (populated from `/api/settings` in `hydrateDefaults()`), then falls back to safe defaults. This prevents silently resetting device brightness/colors when the user has not imported a settings file.
- Fixed race condition in `runRgbTest()` error paths: restore POST and `finishStep6()` are now chained (awaited), preventing concurrent POSTs to `/api/settings`.
- Added `cachedDeviceColors` variable and populated it in `hydrateDefaults()`.
- Updated stale WIZARD_KEYS comment (12 → 15 keys).
- Rebuilt all three gzipped web assets (`wizard.html.gz`, `wizard.js.gz`, `style.css.gz`). Total gzipped size: 49.07 KB (under 50 KB budget).
- Updated `tests/e2e/page-objects/WizardPage.ts` for 6-step flow: updated type, `getCurrentStep()`, `nextButton` comment, added Step 6 locators, updated `completeWizard()` and `completeSetup()`.
- Added `/api/positioning/start` and `/api/positioning/stop` mock routes to `tests/e2e/mock-server/server.ts`.

**Final Validation — 2026-04-13:**
- Detected stale `wizard.js.gz` (source modified at 17:13, gz from 16:46). Rebuilt all three gz assets from current sources. MD5 verified match.
- Total gzipped asset size: 49.19 KB (under 50 KB budget — AC #8).
- All 8 Acceptance Criteria verified satisfied against source code.
- Firmware build: SUCCESS (1.17 MB, 78.3% of 1.5 MB partition).
- TypeScript compilation: clean (pre-existing api-helpers.ts issue is out of scope).
- No regressions introduced; all changes are web-asset-only (HTML/JS/CSS) + test infrastructure.

**Code Review Synthesis #2 — 2026-04-13 (4 validators):**
- Fixed CSS: added `max-height:70vh;object-fit:contain` to `.wizard-test-canvas canvas` — prevents unbounded vertical canvas growth for extreme tile aspect ratios (e.g., 1×10 config).
- Fixed JS: added `wizardTestStatus.textContent = 'Rebooting...'` in `saveAndConnect()` when `currentStep === 6` — gives user visible feedback during reboot wait (previously "Saving..." persisted through the reboot phase with no update).
- Fixed WizardPage.ts `backButton` locator from `/back|previous/i` regex to `#btn-back` ID — the regex erroneously matched "No — take me back" on Step 6, making `goBack()` click the wrong button in tests.
- Fixed WizardPage.ts `previewCanvas` locator from `#preview-canvas, canvas` (any canvas) to `#wizard-position-preview` (specific Step 6 canvas ID).
- Rebuilt `wizard.js.gz` and `style.css.gz`. Total gzipped assets: 49.22 KB (under 50 KB budget — AC #8).
- Confirmed false positives: red corner marker on EVERY tile is correct (firmware verified, lines 747-752 are inside the nested tile loop); canvas wiring-flag omission is correct (NeoMatrix handles physical remapping, visual grid order is always row-major); `enterStep6()` race guard using `currentStep === 6` is correct (a captured-step variable would reintroduce a regression); CORS wildcard in mock server is appropriate for local test tooling; CSRF not applicable to AP-mode IoT.

### File List

- `firmware/data-src/wizard.html` — Added Step 6 section (canvas, confirmation buttons, status text), wiring fields in Step 4, progress counter "of 6"
- `firmware/data-src/wizard.js` — TOTAL_STEPS=6, enterStep6(), renderPositionCanvas(), runRgbTest(), finishStep6(), cachedDeviceColors, wiring flags in state/WIZARD_KEYS/saveAndConnect, showStep(6) nav bar hide, Step 5 pass-through; added "Rebooting..." to wizardTestStatus in saveAndConnect()
- `firmware/data-src/style.css` — .wizard-test-canvas, .wizard-test-question, .wizard-test-actions, .wizard-test-status styles; added max-height:70vh;object-fit:contain to canvas rule
- `firmware/data/wizard.html.gz` — rebuilt from data-src/wizard.html
- `firmware/data/wizard.js.gz` — rebuilt from data-src/wizard.js
- `firmware/data/style.css.gz` — rebuilt from data-src/style.css
- `tests/e2e/page-objects/WizardPage.ts` — WizardStep type +test, getCurrentStep 1-6, Step 6 locators, completeWizard() via #btn-test-yes, completeSetup() 6-step flow; tightened backButton and previewCanvas locators
- `tests/e2e/mock-server/server.ts` — /api/positioning/start and /api/positioning/stop mock routes

## Senior Developer Review (AI)

### Review: 2026-04-13
- **Reviewer:** AI Code Review Synthesis (4 validators)
- **Evidence Score:** 9.0 (highest) → REJECT
- **Issues Found:** 11 verified
- **Issues Fixed:** 11
- **Action Items Created:** 0

#### Review Follow-ups (AI)

All verified issues were fixed in this synthesis pass. No outstanding action items.

> **Deferred / Dismissed:**
> - `TZ_MAP` / `getTimezoneConfig()` dead code in `wizard.js` — pre-existing from Story fn-2.1, out of scope for fn-3.1. Carry forward as a cleanup candidate.
> - RGB test shows text-in-color rather than solid-color fill — by design per Dev Notes ("sufficient to verify each RGB channel independently"); no firmware changes required per AC constraints.
> - Full E2E test *cases* for Step 6 not written in this pass — page-object infrastructure updated; dedicated test spec creation belongs in a separate test story.

### Review: 2026-04-13 (Synthesis #2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 2.5 → PASS
- **Issues Found:** 4 verified
- **Issues Fixed:** 4
- **Action Items Created:** 0

#### Review Follow-ups (AI)

All 4 verified issues fixed in this synthesis pass. No outstanding action items.

> **Deferred / Dismissed:**
> - No E2E test specs for Step 6 — explicitly deferred per Dev Agent Record; page-object infrastructure is in place for when test specs are written in a future story.
> - `saveAndConnect()` SOLID complexity — pre-existing function pattern, acceptable for embedded web UI scope.
> - RGB solid-fill vs. text-tint — dismissed per antipatterns doc ("by design").
> - Canvas ignores wiring flags — FALSE POSITIVE; NeoMatrix library handles physical wiring, visual grid order is always row-major in both firmware and canvas.
> - Red corner marker "tile 0 only" — DOUBLE FALSE POSITIVE; firmware draws on EVERY tile (verified in NeoMatrixDisplay.cpp lines 747-752 inside the nested tile loop); canvas implementation is correct.


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

/* Settings import zone (wizard) */
.settings-import-zone{
  border:2px dashed var(--border);border-radius:var(--radius);
  padding:12px;text-align:center;cursor:pointer;
  transition:border-color 0.2s,background 0.2s;
  margin-bottom:var(--gap);min-height:44px;
}
.settings-import-zone:focus-visible{outline:2px solid var(--primary);outline-offset:2px}
.settings-import-zone.drag-over{border-color:var(--primary);border-style:solid;background:rgba(88,166,255,0.08)}
.import-status{font-size:0.8125rem;color:var(--success);text-align:center;margin-top:8px;margin-bottom:var(--gap)}

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

/* Night Mode card (Story fn-2.3) */
.nm-status{display:flex;gap:8px;align-items:center;margin-bottom:12px;flex-wrap:wrap}
.nm-ntp-badge{
  font-size:0.75rem;font-weight:600;padding:2px 8px;
  border-radius:var(--radius);background:var(--border);color:var(--text-muted);
}
.nm-ntp-ok{background:var(--success);color:var(--bg)}
.nm-ntp-warn{background:#d29922;color:var(--bg)}
.nm-sched-status{font-size:0.8125rem;color:var(--text-muted)}
.nm-dimming{color:var(--primary);font-weight:600}
.nm-active{color:var(--success)}
.nm-inactive{color:var(--text-muted)}

.nm-toggle-label{
  display:flex;align-items:center;gap:8px;
  font-size:0.875rem;color:var(--text);
  cursor:pointer;margin-bottom:12px;
}
.nm-toggle-label input[type="checkbox"]{
  width:18px;height:18px;accent-color:var(--primary);cursor:pointer;
}

.nm-fields{transition:opacity 0.2s}
.nm-fields-disabled{opacity:0.5;pointer-events:none}
.nm-fields input[type="time"]{
  width:100%;padding:8px;font-size:1rem;
  background:var(--surface);color:var(--text);
  border:1px solid var(--border);border-radius:var(--radius);
  outline:none;
}
.nm-fields input[type="time"]:focus{border-color:var(--primary)}
/* Cursor-only rules for disabled inputs — opacity is handled by the .nm-fields-disabled
   container to avoid CSS opacity multiplication (container 0.5 × element 0.5 = 0.25). */
.nm-fields input[type="time"]:disabled{cursor:default}
.nm-fields select:disabled{cursor:default}
.nm-fields input[type="range"]:disabled{cursor:default}

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

/* Wizard Step 6: Test Your Wall */
.wizard-test-canvas{margin:var(--gap) 0}
.wizard-test-canvas canvas{
  display:block;width:100%;max-height:70vh;object-fit:contain;
  border:1px solid var(--border);border-radius:var(--radius);
  background:var(--bg);
}
.wizard-test-question{
  font-size:1rem;font-weight:600;text-align:center;
  margin:var(--gap) 0 12px;
}
.wizard-test-actions{
  display:flex;flex-direction:column;gap:8px;
}
.wizard-test-actions .nav-btn{
  width:100%;min-height:44px;text-align:center;
}
.wizard-test-status{
  text-align:center;font-size:1rem;font-weight:500;
  color:var(--primary);margin-top:var(--gap);
}


]]></file>
<file id="d5f806c1" path="firmware/data-src/wizard.html" label="HTML TEMPLATE"><![CDATA[

<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>FlightWall Setup</title>
<link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="wizard">
  <header class="wizard-header">
    <h1>FlightWall Setup</h1>
    <p id="progress">Step 1 of 6</p>
  </header>

  <main class="wizard-content">
    <!-- Step 1: WiFi -->
    <section id="step-1" class="step active">
      <h2>WiFi Network</h2>
      <div class="settings-import-zone" id="settings-import-zone" role="button" tabindex="0" aria-label="Select settings backup file to import">
        <span class="upload-prompt"><strong>Import settings</strong> from backup</span>
        <span class="upload-hint">Select a flightwall-settings.json file</span>
        <input type="file" id="import-file-input" accept=".json" hidden>
      </div>
      <p id="import-status" class="import-status" style="display:none"></p>
      <div id="scan-status" class="scan-status">
        <p class="scan-msg">Scanning for networks...</p>
        <div class="spinner"></div>
      </div>
      <div id="scan-results" class="scan-results" style="display:none"></div>
      <button type="button" id="btn-manual" class="link-btn">Enter manually instead</button>
      <div id="wifi-form" class="form-fields">
        <label for="wifi-ssid">Network Name (SSID)</label>
        <input type="text" id="wifi-ssid" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false" placeholder="Enter SSID">
        <label for="wifi-pass">Password</label>
        <input type="password" id="wifi-pass" autocomplete="off" placeholder="Enter password">
        <p id="wifi-err" class="field-error" style="display:none">SSID and password are required.</p>
      </div>
    </section>

    <!-- Step 2: API Keys -->
    <section id="step-2" class="step">
      <h2>API Keys</h2>
      <p class="step-desc">Enter your API credentials. These are used to fetch live flight data.</p>
      <div class="form-fields">
        <label for="os-client-id">OpenSky Client ID</label>
        <input type="text" id="os-client-id" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false" placeholder="Paste client ID">
        <label for="os-client-sec">OpenSky Client Secret</label>
        <input type="text" id="os-client-sec" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false" placeholder="Paste client secret">
        <label for="aeroapi-key">AeroAPI Key</label>
        <input type="text" id="aeroapi-key" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false" placeholder="Paste API key">
        <p id="api-err" class="field-error" style="display:none">All three API fields are required.</p>
      </div>
    </section>

    <!-- Step 3: Location -->
    <section id="step-3" class="step">
      <h2>Location</h2>
      <p class="step-desc">Set the center point and radius for flight tracking.</p>
      <button type="button" id="btn-geolocate" class="geo-btn">Use my location</button>
      <p id="geo-status" class="geo-status" style="display:none"></p>
      <div class="form-fields">
        <label for="center-lat">Latitude</label>
        <input type="text" id="center-lat" inputmode="decimal" autocomplete="off" placeholder="e.g. 37.7749">
        <label for="center-lon">Longitude</label>
        <input type="text" id="center-lon" inputmode="decimal" autocomplete="off" placeholder="e.g. -122.4194">
        <label for="radius-km">Radius (km)</label>
        <input type="text" id="radius-km" inputmode="decimal" autocomplete="off" placeholder="e.g. 50">
        <p id="loc-err" class="field-error" style="display:none"></p>
      </div>
      <p class="helper-copy">An interactive map is available later in the dashboard.</p>
    </section>

    <!-- Step 4: Hardware -->
    <section id="step-4" class="step">
      <h2>Hardware</h2>
      <p class="step-desc">Configure your LED matrix dimensions.</p>
      <div class="form-fields">
        <label for="tiles-x">Tiles Wide</label>
        <input type="text" id="tiles-x" inputmode="numeric" autocomplete="off" placeholder="10">
        <label for="tiles-y">Tiles High</label>
        <input type="text" id="tiles-y" inputmode="numeric" autocomplete="off" placeholder="2">
        <label for="tile-pixels">Pixels per Tile</label>
        <input type="text" id="tile-pixels" inputmode="numeric" autocomplete="off" placeholder="16">
        <label for="display-pin">Data Pin (GPIO)</label>
        <input type="text" id="display-pin" inputmode="numeric" autocomplete="off" placeholder="25">
        <label for="origin-corner">Origin Corner</label>
        <select id="origin-corner" class="cal-select">
          <option value="0">Top-Left</option>
          <option value="1">Top-Right</option>
          <option value="2">Bottom-Left</option>
          <option value="3">Bottom-Right</option>
        </select>
        <label for="scan-dir">Scan Direction</label>
        <select id="scan-dir" class="cal-select">
          <option value="0">Rows</option>
          <option value="1">Columns</option>
        </select>
        <label for="zigzag">Zigzag</label>
        <select id="zigzag" class="cal-select">
          <option value="0">Progressive</option>
          <option value="1">Zigzag</option>
        </select>
        <p id="hw-err" class="field-error" style="display:none"></p>
      </div>
      <p id="resolution-text" class="resolution-text"></p>
    </section>

    <!-- Step 5: Review -->
    <section id="step-5" class="step">
      <h2>Review</h2>
      <p class="step-desc">Confirm your settings before testing.</p>
      <div id="review-sections" class="review-sections"></div>
    </section>

    <!-- Step 6: Test Your Wall -->
    <section id="step-6" class="step">
      <h2>Test Your Wall</h2>
      <p class="step-desc">Check that your LED panel layout matches the preview below.</p>
      <div class="wizard-test-canvas">
        <canvas id="wizard-position-preview"></canvas>
      </div>
      <p class="wizard-test-question">Does your wall match this layout?</p>
      <div class="wizard-test-actions">
        <button type="button" id="btn-test-yes" class="nav-btn nav-btn-primary">Yes, it matches</button>
        <button type="button" id="btn-test-no" class="nav-btn">No — take me back</button>
      </div>
      <p id="wizard-test-status" class="wizard-test-status" style="display:none"></p>
    </section>
  </main>

  <nav class="wizard-nav">
    <button type="button" id="btn-back" class="nav-btn" style="visibility:hidden">&larr; Back</button>
    <button type="button" id="btn-next" class="nav-btn nav-btn-primary">Next &rarr;</button>
  </nav>
</div>

<script src="/common.js"></script>
<script src="/wizard.js"></script>
</body>
</html>


]]></file>
<file id="75a3699e" path="firmware/data-src/wizard.js" label="SOURCE CODE"><![CDATA[

/* FlightWall Setup Wizard — Step navigation, WiFi scan polling, validation, state preservation */

/**
 * IANA-to-POSIX timezone mapping (Story fn-2.1).
 * Shared with dashboard.js — used for settings import timezone resolution.
 * Browser auto-detects IANA via Intl API; ESP32 needs POSIX string for configTzTime().
 */
var TZ_MAP = {
  // North America
  "America/New_York":       "EST5EDT,M3.2.0,M11.1.0",
  "America/Chicago":        "CST6CDT,M3.2.0,M11.1.0",
  "America/Denver":         "MST7MDT,M3.2.0,M11.1.0",
  "America/Los_Angeles":    "PST8PDT,M3.2.0,M11.1.0",
  "America/Phoenix":        "MST7",
  "America/Anchorage":      "AKST9AKDT,M3.2.0,M11.1.0",
  "Pacific/Honolulu":       "HST10",
  "America/Toronto":        "EST5EDT,M3.2.0,M11.1.0",
  "America/Vancouver":      "PST8PDT,M3.2.0,M11.1.0",
  "America/Edmonton":       "MST7MDT,M3.2.0,M11.1.0",
  "America/Winnipeg":       "CST6CDT,M3.2.0,M11.1.0",
  "America/Halifax":        "AST4ADT,M3.2.0,M11.1.0",
  "America/St_Johns":       "NST3:30NDT,M3.2.0,M11.1.0",
  "America/Mexico_City":    "CST6",
  "America/Tijuana":        "PST8PDT,M3.2.0,M11.1.0",
  // Europe
  "Europe/London":          "GMT0BST,M3.5.0/1,M10.5.0",
  "Europe/Paris":           "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Berlin":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Madrid":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Rome":            "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Amsterdam":       "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Brussels":        "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Zurich":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Vienna":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Warsaw":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Stockholm":       "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Oslo":            "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Copenhagen":      "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Helsinki":        "EET-2EEST,M3.5.0/3,M10.5.0/4",
  "Europe/Athens":          "EET-2EEST,M3.5.0/3,M10.5.0/4",
  "Europe/Bucharest":       "EET-2EEST,M3.5.0/3,M10.5.0/4",
  "Europe/Istanbul":        "TRT-3",
  "Europe/Moscow":          "MSK-3",
  "Europe/Lisbon":          "WET0WEST,M3.5.0/1,M10.5.0",
  "Europe/Dublin":          "IST-1GMT0,M10.5.0,M3.5.0/1",
  // Asia
  "Asia/Tokyo":             "JST-9",
  "Asia/Shanghai":          "CST-8",
  "Asia/Hong_Kong":         "HKT-8",
  "Asia/Singapore":         "SGT-8",
  "Asia/Kolkata":           "IST-5:30",
  "Asia/Dubai":             "GST-4",
  "Asia/Seoul":             "KST-9",
  "Asia/Bangkok":           "ICT-7",
  "Asia/Jakarta":           "WIB-7",
  "Asia/Taipei":            "CST-8",
  "Asia/Karachi":           "PKT-5",
  "Asia/Dhaka":             "BST-6",
  "Asia/Riyadh":            "AST-3",
  "Asia/Tehran":            "IRST-3:30",
  // Oceania
  "Australia/Sydney":       "AEST-10AEDT,M10.1.0,M4.1.0/3",
  "Australia/Melbourne":    "AEST-10AEDT,M10.1.0,M4.1.0/3",
  "Australia/Perth":        "AWST-8",
  "Australia/Brisbane":     "AEST-10",
  "Australia/Adelaide":     "ACST-9:30ACDT,M10.1.0,M4.1.0/3",
  "Pacific/Auckland":       "NZST-12NZDT,M9.5.0,M4.1.0/3",
  "Pacific/Fiji":           "FJT-12",
  // South America
  "America/Sao_Paulo":      "BRT3",
  "America/Argentina/Buenos_Aires": "ART3",
  "America/Santiago":       "CLT4CLST,M9.1.6/24,M4.1.6/24",
  "America/Bogota":         "COT5",
  "America/Lima":           "PET5",
  "America/Caracas":        "VET4",
  // Africa / Middle East
  "Africa/Cairo":           "EET-2EEST,M4.5.5/0,M10.5.4/24",
  "Africa/Johannesburg":    "SAST-2",
  "Africa/Lagos":           "WAT-1",
  "Africa/Nairobi":         "EAT-3",
  "Africa/Casablanca":      "WET0WEST,M3.5.0/2,M10.5.0/3",
  "Asia/Jerusalem":         "IST-2IDT,M3.4.4/26,M10.5.0",
  // UTC
  "UTC":                    "UTC0",
  "Etc/UTC":                "UTC0",
  "Etc/GMT":                "GMT0"
};

/**
 * Detect browser timezone and return IANA + POSIX pair.
 * @returns {{ iana: string, posix: string }}
 */
function getTimezoneConfig() {
  var iana = "UTC";
  try {
    iana = Intl.DateTimeFormat().resolvedOptions().timeZone || "UTC";
  } catch (e) { /* Intl not available */ }
  var posix = TZ_MAP[iana] || "UTC0";
  return { iana: iana, posix: posix };
}

(function() {
  'use strict';

  var TOTAL_STEPS = 6;
  var currentStep = 1;

  // In-memory wizard state using exact firmware config key names
  var state = {
    wifi_ssid: '',
    wifi_password: '',
    os_client_id: '',
    os_client_sec: '',
    aeroapi_key: '',
    center_lat: '',
    center_lon: '',
    radius_km: '',
    tiles_x: '',
    tiles_y: '',
    tile_pixels: '',
    display_pin: '',
    origin_corner: '0',
    scan_dir: '0',
    zigzag: '0'
  };

  // Valid GPIO pins for display_pin (matches ConfigManager validation)
  var VALID_PINS = [0,2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33];

  // Keys that map to wizard form fields (15 keys)
  var WIZARD_KEYS = [
    'wifi_ssid', 'wifi_password',
    'os_client_id', 'os_client_sec', 'aeroapi_key',
    'center_lat', 'center_lon', 'radius_km',
    'tiles_x', 'tiles_y', 'tile_pixels', 'display_pin',
    'origin_corner', 'scan_dir', 'zigzag'
  ];

  // Non-wizard config keys preserved in POST payload
  var KNOWN_EXTRA_KEYS = [
    'brightness', 'text_color_r', 'text_color_g', 'text_color_b',
    'zone_logo_pct', 'zone_split_pct', 'zone_layout',
    'fetch_interval', 'display_cycle',
    'timezone', 'sched_enabled', 'sched_dim_start', 'sched_dim_end', 'sched_dim_brt'
  ];

  // Extra keys from import (not shown in wizard, but sent with POST)
  var importedExtras = {};

  // Device color/brightness values read from /api/settings at startup.
  // Used by runRgbTest() to restore originals even without a settings import.
  var cachedDeviceColors = { brightness: null, text_color_r: null, text_color_g: null, text_color_b: null };

  // DOM references — Import zone
  var importZone = document.getElementById('settings-import-zone');
  var importFileInput = document.getElementById('import-file-input');
  var importStatus = document.getElementById('import-status');

  // DOM references — Steps 1-2
  var progress = document.getElementById('progress');
  var btnBack = document.getElementById('btn-back');
  var btnNext = document.getElementById('btn-next');
  var scanStatus = document.getElementById('scan-status');
  var scanResults = document.getElementById('scan-results');
  var btnManual = document.getElementById('btn-manual');
  var wifiSsid = document.getElementById('wifi-ssid');
  var wifiPass = document.getElementById('wifi-pass');
  var wifiErr = document.getElementById('wifi-err');
  var osClientId = document.getElementById('os-client-id');
  var osClientSec = document.getElementById('os-client-sec');
  var aeroKey = document.getElementById('aeroapi-key');
  var apiErr = document.getElementById('api-err');

  // DOM references — Step 3
  var btnGeolocate = document.getElementById('btn-geolocate');
  var geoStatus = document.getElementById('geo-status');
  var centerLat = document.getElementById('center-lat');
  var centerLon = document.getElementById('center-lon');
  var radiusKm = document.getElementById('radius-km');
  var locErr = document.getElementById('loc-err');

  // DOM references — Step 4
  var tilesX = document.getElementById('tiles-x');
  var tilesY = document.getElementById('tiles-y');
  var tilePixels = document.getElementById('tile-pixels');
  var displayPin = document.getElementById('display-pin');
  var originCorner = document.getElementById('origin-corner');
  var scanDir = document.getElementById('scan-dir');
  var zigzagSel = document.getElementById('zigzag');
  var hwErr = document.getElementById('hw-err');
  var resolutionText = document.getElementById('resolution-text');

  // DOM references — Step 5
  var reviewSections = document.getElementById('review-sections');

  // DOM references — Step 6
  var wizardCanvas = document.getElementById('wizard-position-preview');
  var btnTestYes = document.getElementById('btn-test-yes');
  var btnTestNo = document.getElementById('btn-test-no');
  var wizardTestStatus = document.getElementById('wizard-test-status');
  var wizardNav = document.querySelector('.wizard-nav');

  // --- Bootstrap: hydrate defaults from firmware ---
  function hydrateDefaults() {
    FW.get('/api/settings').then(function(res) {
      if (!res.body.ok || !res.body.data) return;
      var d = res.body.data;
      // Only hydrate Step 3-4 keys if state is still empty (user hasn't typed yet)
      if (!state.center_lat && d.center_lat !== undefined) state.center_lat = String(d.center_lat);
      if (!state.center_lon && d.center_lon !== undefined) state.center_lon = String(d.center_lon);
      if (!state.radius_km && d.radius_km !== undefined) state.radius_km = String(d.radius_km);
      if (!state.tiles_x && d.tiles_x !== undefined) state.tiles_x = String(d.tiles_x);
      if (!state.tiles_y && d.tiles_y !== undefined) state.tiles_y = String(d.tiles_y);
      if (!state.tile_pixels && d.tile_pixels !== undefined) state.tile_pixels = String(d.tile_pixels);
      if (!state.display_pin && d.display_pin !== undefined) state.display_pin = String(d.display_pin);
      if (d.origin_corner !== undefined) state.origin_corner = String(d.origin_corner);
      if (d.scan_dir !== undefined) state.scan_dir = String(d.scan_dir);
      if (d.zigzag !== undefined) state.zigzag = String(d.zigzag);
      // Cache actual device brightness/color so runRgbTest() can restore originals
      // even when the user has not imported a settings file.
      if (d.brightness !== undefined) cachedDeviceColors.brightness = d.brightness;
      if (d.text_color_r !== undefined) cachedDeviceColors.text_color_r = d.text_color_r;
      if (d.text_color_g !== undefined) cachedDeviceColors.text_color_g = d.text_color_g;
      if (d.text_color_b !== undefined) cachedDeviceColors.text_color_b = d.text_color_b;
    }).catch(function() {
      // Settings endpoint unavailable — user will fill manually
    });
  }

  // --- WiFi Scan ---
  var scanTimer = null;
  var scanStartTime = 0;
  var SCAN_TIMEOUT_MS = 5000;
  var SCAN_POLL_MS = 800;
  var manualMode = false;

  function startScan() {
    clearTimeout(scanTimer);
    manualMode = false;
    scanStatus.style.display = '';
    scanResults.style.display = 'none';
    scanStatus.querySelector('.scan-msg').textContent = 'Scanning for networks...';
    var spinner = scanStatus.querySelector('.spinner');
    if (spinner) spinner.style.display = '';
    scanStartTime = Date.now();
    pollScan();
  }

  function pollScan() {
    FW.get('/api/wifi/scan').then(function(res) {
      if (manualMode) return;
      var d = res.body;
      if (d.ok && !d.scanning && d.data && d.data.length > 0) {
        showScanResults(d.data);
        return;
      }
      if (d.ok && !d.scanning) {
        showScanEmpty();
        return;
      }
      if (Date.now() - scanStartTime >= SCAN_TIMEOUT_MS) {
        showScanEmpty();
        return;
      }
      scanTimer = setTimeout(pollScan, SCAN_POLL_MS);
    }).catch(function() {
      if (manualMode) return;
      if (Date.now() - scanStartTime >= SCAN_TIMEOUT_MS) {
        showScanEmpty();
        return;
      }
      scanTimer = setTimeout(pollScan, SCAN_POLL_MS);
    });
  }

  function showScanResults(networks) {
    clearTimeout(scanTimer);
    scanStatus.style.display = 'none';
    scanResults.style.display = '';
    scanResults.innerHTML = '';
    // Deduplicate by SSID, keep strongest RSSI
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
      row.addEventListener('click', function() { selectSsid(n.ssid); });
      scanResults.appendChild(row);
    });
  }

  function showScanEmpty() {
    clearTimeout(scanTimer);
    scanStatus.querySelector('.scan-msg').textContent = 'No networks found - enter manually';
    var spinner = scanStatus.querySelector('.spinner');
    if (spinner) spinner.style.display = 'none';
    scanResults.style.display = 'none';
  }

  function selectSsid(ssid) {
    wifiSsid.value = ssid;
    state.wifi_ssid = ssid;
    wifiSsid.focus();
  }

  function enterManual() {
    manualMode = true;
    clearTimeout(scanTimer);
    scanStatus.style.display = 'none';
    scanResults.style.display = 'none';
    wifiSsid.focus();
  }

  function rssiLabel(rssi) {
    if (rssi >= -50) return 'Excellent';
    if (rssi >= -60) return 'Good';
    if (rssi >= -70) return 'Fair';
    return 'Weak';
  }

  function escHtml(s) {
    var el = document.createElement('span');
    el.textContent = s;
    return el.innerHTML;
  }

  // --- Geolocation (Step 3) ---
  function requestGeolocation() {
    if (!navigator.geolocation) {
      showGeoStatus('Geolocation is not available in this browser. Please enter coordinates manually.', true);
      return;
    }
    btnGeolocate.disabled = true;
    showGeoStatus('Requesting location...', false);
    navigator.geolocation.getCurrentPosition(
      function(pos) {
        centerLat.value = pos.coords.latitude.toFixed(6);
        centerLon.value = pos.coords.longitude.toFixed(6);
        state.center_lat = centerLat.value;
        state.center_lon = centerLon.value;
        showGeoStatus('Location detected.', false);
        btnGeolocate.disabled = false;
      },
      function(err) {
        var msg = 'Could not get location. Please enter coordinates manually.';
        if (err.code === 1) msg = 'Location permission denied. Please enter coordinates manually.';
        if (err.code === 2) msg = 'Location unavailable. Please enter coordinates manually.';
        showGeoStatus(msg, true);
        btnGeolocate.disabled = false;
      },
      { enableHighAccuracy: false, timeout: 10000, maximumAge: 300000 }
    );
  }

  function showGeoStatus(msg, isError) {
    geoStatus.textContent = msg;
    geoStatus.style.display = '';
    geoStatus.className = 'geo-status' + (isError ? ' geo-error' : ' geo-ok');
  }

  // --- Hardware resolution text (Step 4) ---
  function updateResolution() {
    var tx = parseStrictPositiveInt(tilesX.value.trim());
    var ty = parseStrictPositiveInt(tilesY.value.trim());
    var tp = parseStrictPositiveInt(tilePixels.value.trim());
    if (tx !== null && ty !== null && tp !== null) {
      resolutionText.textContent = 'Your display: ' + (tx * tp) + ' x ' + (ty * tp) + ' pixels';
    } else {
      resolutionText.textContent = '';
    }
  }

  // --- Review (Step 5) ---
  function buildReview() {
    reviewSections.innerHTML = '';
    var sections = [
      { title: 'WiFi', step: 1, items: [
        { label: 'Network', value: state.wifi_ssid },
        { label: 'Password', value: state.wifi_password ? '\u2022'.repeat(Math.min(state.wifi_password.length, 12)) : '' }
      ]},
      { title: 'API Keys', step: 2, items: [
        { label: 'OpenSky Client ID', value: maskKey(state.os_client_id) },
        { label: 'OpenSky Secret', value: maskKey(state.os_client_sec) },
        { label: 'AeroAPI Key', value: maskKey(state.aeroapi_key) }
      ]},
      { title: 'Location', step: 3, items: [
        { label: 'Latitude', value: state.center_lat },
        { label: 'Longitude', value: state.center_lon },
        { label: 'Radius', value: state.radius_km + ' km' }
      ]},
      { title: 'Hardware', step: 4, items: [
        { label: 'Display', value: state.tiles_x + ' x ' + state.tiles_y + ' tiles (' + (parseInt(state.tiles_x,10)*parseInt(state.tile_pixels,10)) + ' x ' + (parseInt(state.tiles_y,10)*parseInt(state.tile_pixels,10)) + ' px)' },
        { label: 'Pixels per tile', value: state.tile_pixels },
        { label: 'Data pin', value: 'GPIO ' + state.display_pin },
        { label: 'Origin', value: ['Top-Left','Top-Right','Bottom-Left','Bottom-Right'][parseInt(state.origin_corner,10)] || 'Top-Left' },
        { label: 'Scan', value: parseInt(state.scan_dir,10) === 1 ? 'Columns' : 'Rows' },
        { label: 'Zigzag', value: parseInt(state.zigzag,10) === 1 ? 'Yes' : 'No' }
      ]}
    ];

    sections.forEach(function(sec) {
      var card = document.createElement('div');
      card.className = 'review-card';
      card.addEventListener('click', function() {
        showStep(sec.step);
      });
      var header = '<div class="review-card-header"><span>' + escHtml(sec.title) + '</span><span class="review-edit">Edit</span></div>';
      var body = sec.items.map(function(it) {
        return '<div class="review-item"><span class="review-label">' + escHtml(it.label) + '</span><span class="review-value">' + escHtml(it.value) + '</span></div>';
      }).join('');
      card.innerHTML = header + body;
      reviewSections.appendChild(card);
    });
  }

  function maskKey(val) {
    if (!val || val.length < 6) return val || '';
    return val.substring(0, 3) + '\u2022'.repeat(Math.min(val.length - 6, 8)) + val.substring(val.length - 3);
  }

  function parseStrictNumber(value) {
    if (!/^[+-]?(?:\d+\.?\d*|\.\d+)$/.test(value)) return null;
    var parsed = Number(value);
    return isNaN(parsed) ? null : parsed;
  }

  function parseStrictPositiveInt(value) {
    if (!/^\d+$/.test(value)) return null;
    var parsed = Number(value);
    if (!isFinite(parsed) || parsed <= 0 || Math.floor(parsed) !== parsed) return null;
    return parsed;
  }

  // --- Navigation ---
  function showStep(n) {
    currentStep = n;
    progress.textContent = 'Step ' + n + ' of ' + TOTAL_STEPS;
    for (var i = 1; i <= TOTAL_STEPS; i++) {
      var sec = document.getElementById('step-' + i);
      if (sec) sec.classList.toggle('active', i === n);
    }
    btnBack.style.visibility = n === 1 ? 'hidden' : 'visible';

    // Hide nav bar on Step 6 (has its own buttons), show for all others
    if (n === 6) {
      wizardNav.style.display = 'none';
    } else {
      wizardNav.style.display = '';
      btnNext.textContent = 'Next \u2192';
      btnNext.className = 'nav-btn nav-btn-primary';
    }

    // Rehydrate fields from state when entering a step
    if (n === 1) {
      wifiSsid.value = state.wifi_ssid;
      wifiPass.value = state.wifi_password;
      wifiErr.style.display = 'none';
      clearInputErrors([wifiSsid, wifiPass]);
    }
    if (n === 2) {
      osClientId.value = state.os_client_id;
      osClientSec.value = state.os_client_sec;
      aeroKey.value = state.aeroapi_key;
      apiErr.style.display = 'none';
      clearInputErrors([osClientId, osClientSec, aeroKey]);
    }
    if (n === 3) {
      centerLat.value = state.center_lat;
      centerLon.value = state.center_lon;
      radiusKm.value = state.radius_km;
      locErr.style.display = 'none';
      clearInputErrors([centerLat, centerLon, radiusKm]);
    }
    if (n === 4) {
      tilesX.value = state.tiles_x;
      tilesY.value = state.tiles_y;
      tilePixels.value = state.tile_pixels;
      displayPin.value = state.display_pin;
      originCorner.value = state.origin_corner || '0';
      scanDir.value = state.scan_dir || '0';
      zigzagSel.value = state.zigzag || '0';
      hwErr.style.display = 'none';
      clearInputErrors([tilesX, tilesY, tilePixels, displayPin]);
      updateResolution();
    }
    if (n === 5) {
      buildReview();
    }
    if (n === 6) {
      enterStep6();
    }
  }

  function saveCurrentStepState() {
    if (currentStep === 1) {
      state.wifi_ssid = wifiSsid.value.trim();
      state.wifi_password = wifiPass.value;
    } else if (currentStep === 2) {
      state.os_client_id = osClientId.value.trim();
      state.os_client_sec = osClientSec.value.trim();
      state.aeroapi_key = aeroKey.value.trim();
    } else if (currentStep === 3) {
      state.center_lat = centerLat.value.trim();
      state.center_lon = centerLon.value.trim();
      state.radius_km = radiusKm.value.trim();
    } else if (currentStep === 4) {
      state.tiles_x = tilesX.value.trim();
      state.tiles_y = tilesY.value.trim();
      state.tile_pixels = tilePixels.value.trim();
      state.display_pin = displayPin.value.trim();
      state.origin_corner = originCorner.value;
      state.scan_dir = scanDir.value;
      state.zigzag = zigzagSel.value;
    }
  }

  function validateStep(n) {
    if (n === 1) {
      state.wifi_ssid = wifiSsid.value.trim();
      state.wifi_password = wifiPass.value;
      var missing = [];
      if (!state.wifi_ssid) missing.push(wifiSsid);
      if (!state.wifi_password) missing.push(wifiPass);
      if (missing.length > 0) {
        wifiErr.style.display = '';
        markInputErrors(missing);
        return false;
      }
      wifiErr.style.display = 'none';
      clearInputErrors([wifiSsid, wifiPass]);
      return true;
    }
    if (n === 2) {
      state.os_client_id = osClientId.value.trim();
      state.os_client_sec = osClientSec.value.trim();
      state.aeroapi_key = aeroKey.value.trim();
      var missing2 = [];
      if (!state.os_client_id) missing2.push(osClientId);
      if (!state.os_client_sec) missing2.push(osClientSec);
      if (!state.aeroapi_key) missing2.push(aeroKey);
      if (missing2.length > 0) {
        apiErr.style.display = '';
        markInputErrors(missing2);
        return false;
      }
      apiErr.style.display = 'none';
      clearInputErrors([osClientId, osClientSec, aeroKey]);
      return true;
    }
    if (n === 3) {
      state.center_lat = centerLat.value.trim();
      state.center_lon = centerLon.value.trim();
      state.radius_km = radiusKm.value.trim();
      var errs = [];
      var lat = parseStrictNumber(state.center_lat);
      var lon = parseStrictNumber(state.center_lon);
      var rad = parseStrictNumber(state.radius_km);
      if (lat === null || lat < -90 || lat > 90) errs.push(centerLat);
      if (lon === null || lon < -180 || lon > 180) errs.push(centerLon);
      if (rad === null || rad <= 0) errs.push(radiusKm);
      if (errs.length > 0) {
        locErr.textContent = 'Latitude (-90 to 90), longitude (-180 to 180), and a positive radius are required.';
        locErr.style.display = '';
        markInputErrors(errs);
        return false;
      }
      locErr.style.display = 'none';
      clearInputErrors([centerLat, centerLon, radiusKm]);
      return true;
    }
    if (n === 4) {
      state.tiles_x = tilesX.value.trim();
      state.tiles_y = tilesY.value.trim();
      state.tile_pixels = tilePixels.value.trim();
      state.display_pin = displayPin.value.trim();
      var errs4 = [];
      var tx = parseStrictPositiveInt(state.tiles_x);
      var ty = parseStrictPositiveInt(state.tiles_y);
      var tp = parseStrictPositiveInt(state.tile_pixels);
      // display_pin allows 0 (GPIO 0 is in VALID_PINS); use inclusive int parse
      var dpStr = state.display_pin.trim();
      var dp = (/^\d+$/.test(dpStr)) ? parseInt(dpStr, 10) : NaN;
      if (tx === null) errs4.push(tilesX);
      if (ty === null) errs4.push(tilesY);
      if (tp === null) errs4.push(tilePixels);
      if (isNaN(dp) || VALID_PINS.indexOf(dp) === -1) errs4.push(displayPin);
      if (errs4.length > 0) {
        var msg = 'All fields must be positive integers.';
        if (errs4.length === 1 && errs4[0] === displayPin) {
          msg = 'Invalid GPIO pin. Supported: ' + VALID_PINS.join(', ') + '.';
        }
        hwErr.textContent = msg;
        hwErr.style.display = '';
        markInputErrors(errs4);
        return false;
      }
      hwErr.style.display = 'none';
      clearInputErrors([tilesX, tilesY, tilePixels, displayPin]);
      return true;
    }
    // Step 5: no field validation — review only
    return true;
  }

  function markInputErrors(inputs) {
    inputs.forEach(function(el) { el.classList.add('input-error'); });
  }

  function clearInputErrors(inputs) {
    inputs.forEach(function(el) { el.classList.remove('input-error'); });
  }

  // --- Save & Connect (called from Step 6 completion) ---
  function saveAndConnect() {
    var rebootRequested = false;
    var rebootFailed = false;

    // finishStep6() already set the status text — do not overwrite it here.
    btnNext.disabled = true;

    var payload = {
      wifi_ssid: state.wifi_ssid,
      wifi_password: state.wifi_password,
      os_client_id: state.os_client_id,
      os_client_sec: state.os_client_sec,
      aeroapi_key: state.aeroapi_key,
      center_lat: Number(state.center_lat),
      center_lon: Number(state.center_lon),
      radius_km: Number(state.radius_km),
      tiles_x: Number(state.tiles_x),
      tiles_y: Number(state.tiles_y),
      tile_pixels: Number(state.tile_pixels),
      display_pin: Number(state.display_pin),
      origin_corner: Number(state.origin_corner),
      scan_dir: Number(state.scan_dir),
      zigzag: Number(state.zigzag)
    };

    // Merge imported extra keys (brightness, timing, schedule, etc.)
    var key;
    for (key in importedExtras) {
      if (importedExtras.hasOwnProperty(key)) {
        payload[key] = importedExtras[key];
      }
    }

    FW.post('/api/settings', payload).then(function(res) {
      if (!res.body.ok) {
        throw new Error('Save failed: ' + (res.body.error || 'Unknown error'));
      }
      // Settings saved — trigger reboot
      btnNext.textContent = 'Rebooting...';
      if (currentStep === 6 && wizardTestStatus) wizardTestStatus.textContent = 'Rebooting...';
      rebootRequested = true;
      return FW.post('/api/reboot', {});
    }).then(function(res) {
      if (!res || !res.body || !res.body.ok) {
        rebootFailed = true;
        throw new Error('Reboot failed: ' + ((res && res.body && res.body.error) || 'Unknown error'));
      }
      // Reboot accepted — device is leaving AP mode (showHandoff includes AC5 ready message)
      showHandoff();
    }).catch(function(err) {
      if (rebootRequested && !rebootFailed) {
        // Network loss after requesting reboot is expected because the device is leaving AP mode.
        showHandoff();
        return;
      }
      btnNext.disabled = false;
      btnNext.textContent = 'Save & Connect';
      // If we're on Step 6, restore its buttons so the user can retry
      if (currentStep === 6) {
        if (wizardTestStatus) {
          wizardTestStatus.textContent = '';
          wizardTestStatus.style.display = 'none';
        }
        btnTestYes.style.display = '';
        btnTestYes.disabled = false;
        btnTestNo.style.display = '';
        btnTestNo.disabled = false;
      }
      showSaveError(err && err.message ? err.message : 'Save failed: Network error');
    });
  }

  function showSaveError(msg) {
    var errEl = document.getElementById('save-err');
    if (!errEl) {
      errEl = document.createElement('p');
      errEl.id = 'save-err';
      errEl.className = 'field-error';
    }
    errEl.textContent = msg;
    errEl.style.display = '';
    // Append to the active step container so the error is always visible
    if (currentStep === 6) {
      wizardTestStatus.parentNode.insertBefore(errEl, wizardTestStatus.nextSibling);
    } else {
      reviewSections.parentNode.appendChild(errEl);
    }
  }

  function showHandoff() {
    // Replace wizard content with handoff message (AC5: show ready message here so it persists)
    var wizard = document.querySelector('.wizard');
    wizard.innerHTML =
      '<div class="handoff">' +
        '<h1>Configuration Saved</h1>' +
        '<p>Your FlightWall is ready! Fetching your first flights...</p>' +
        '<p>Your FlightWall is rebooting and connecting to <strong>' + escHtml(state.wifi_ssid) + '</strong>.</p>' +
        '<p>Look at the LED wall for progress.</p>' +
        '<p class="handoff-note">This page will no longer update because the device is leaving setup mode.</p>' +
      '</div>';
  }

  // --- Event Listeners ---
  btnNext.addEventListener('click', function() {
    if (!validateStep(currentStep)) return;
    saveCurrentStepState();
    if (currentStep < TOTAL_STEPS) {
      showStep(currentStep + 1);
    }
    // Step 6 has its own buttons — nav bar is hidden, so this branch is unreachable
  });

  btnBack.addEventListener('click', function() {
    saveCurrentStepState();
    if (currentStep > 1) {
      showStep(currentStep - 1);
    }
  });

  btnManual.addEventListener('click', enterManual);
  btnGeolocate.addEventListener('click', requestGeolocation);

  // Live resolution update on Step 4 inputs
  tilesX.addEventListener('input', updateResolution);
  tilesY.addEventListener('input', updateResolution);
  tilePixels.addEventListener('input', updateResolution);

  // --- Step 6: Test Your Wall ---

  function enterStep6() {
    // Reset UI state
    wizardTestStatus.style.display = 'none';
    wizardTestStatus.textContent = '';
    btnTestYes.disabled = false;
    btnTestNo.disabled = false;
    btnTestYes.style.display = '';
    btnTestNo.style.display = '';

    // Clear stale save error from a previous failed save attempt (re-entry case)
    var oldErr = document.getElementById('save-err');
    if (oldErr && oldErr.parentNode) oldErr.parentNode.removeChild(oldErr);

    // Render canvas preview
    renderPositionCanvas();

    // Trigger positioning pattern on LEDs; return to Step 5 if it fails (AC1).
    // Guard with currentStep === 6 to avoid yanking the user if they already navigated away
    // before this async callback fires (race condition fix).
    FW.post('/api/positioning/start', {}).then(function(res) {
      if (!res.body.ok) {
        FW.showToast('Could not start test pattern: ' + (res.body.error || 'Unknown error'), 'error');
        if (currentStep === 6) showStep(5);
      }
    }).catch(function() {
      FW.showToast('Could not start test pattern: Network error', 'error');
      if (currentStep === 6) showStep(5);
    });
  }

  // Render a canvas preview that faithfully matches NeoMatrixDisplay::renderPositioningPattern():
  // - gridIdx = row * tilesX + col (visual scan order, no wiring remapping — NeoMatrix lib handles that)
  // - tile fill: dim version of unique per-tile hue (hsl dim = low lightness)
  // - tile border: bright version of same hue (matching firmware's bright/dim HSV pair)
  // - red corner marker at top-left of EVERY tile (matches firmware — every tile has one)
  // - centered white digit label (matches firmware 3×5 font digits)
  function renderPositionCanvas() {
    if (!wizardCanvas || !wizardCanvas.getContext) return;
    var ctx = wizardCanvas.getContext('2d');

    var tx = parseInt(state.tiles_x, 10) || 1;
    var ty = parseInt(state.tiles_y, 10) || 1;
    var totalTiles = tx * ty;

    // Size canvas to fit container; cap height to prevent extreme aspect ratios
    var container = wizardCanvas.parentElement;
    var containerWidth = (container && container.clientWidth) || 300;
    var aspect = tx / ty;
    var MAX_CANVAS_HEIGHT = 400;
    var drawWidth = Math.min(containerWidth, 480);
    var drawHeight = Math.round(drawWidth / aspect);
    if (drawHeight > MAX_CANVAS_HEIGHT) {
      drawHeight = MAX_CANVAS_HEIGHT;
      drawWidth = Math.round(drawHeight * aspect);
      if (drawWidth > 480) drawWidth = 480;
    }
    if (drawHeight < 60) drawHeight = 60;

    wizardCanvas.width = drawWidth;
    wizardCanvas.height = drawHeight;

    var cellW = drawWidth / tx;
    var cellH = drawHeight / ty;

    // Dark background
    ctx.fillStyle = '#0d1117';
    ctx.fillRect(0, 0, drawWidth, drawHeight);

    var markerSize = Math.min(Math.max(3, cellW * 0.18), Math.max(3, cellH * 0.18), 8);
    var fontSize = Math.max(9, Math.min(cellW, cellH) * 0.38);

    for (var row = 0; row < ty; row++) {
      for (var col = 0; col < tx; col++) {
        // gridIdx: visual scan order — matches firmware (NeoMatrix lib handles physical wiring)
        var gridIdx = row * tx + col;
        var hue = Math.round(gridIdx * 360 / totalTiles);
        var x = col * cellW;
        var y = row * cellH;

        // Tile fill — dim version of hue (matches firmware dim.setHSV(hue, 200, 40))
        ctx.fillStyle = 'hsl(' + hue + ', 80%, 20%)';
        ctx.fillRect(x + 1, y + 1, cellW - 2, cellH - 2);

        // Bright border — bright version of same hue (matches firmware bright.setHSV(hue, 255, 200))
        ctx.strokeStyle = 'hsl(' + hue + ', 100%, 60%)';
        ctx.lineWidth = 1.5;
        ctx.strokeRect(x + 1, y + 1, cellW - 2, cellH - 2);

        // Red corner marker at top-left of EVERY tile (matches firmware — every tile has one)
        ctx.fillStyle = '#ff0000';
        ctx.fillRect(x + 2, y + 2, markerSize, markerSize);
      }
    }

    // Centered white tile-index digits (matches firmware white 3×5 pixel font)
    ctx.font = 'bold ' + Math.round(fontSize) + 'px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#ffffff';
    for (var r = 0; r < ty; r++) {
      for (var c = 0; c < tx; c++) {
        var idx = r * tx + c;
        ctx.fillText(String(idx), (c + 0.5) * cellW, (r + 0.5) * cellH);
      }
    }
  }

  function runRgbTest() {
    // Resolve original values: importedExtras wins (if user imported), then cachedDeviceColors
    // (read from /api/settings at startup), then safe fallbacks. This ensures we never
    // silently reset a device that was configured without a settings import (AC4).
    var origBrt = (importedExtras.brightness !== undefined) ? importedExtras.brightness :
                  (cachedDeviceColors.brightness !== null) ? cachedDeviceColors.brightness : 40;
    var origR   = (importedExtras.text_color_r !== undefined) ? importedExtras.text_color_r :
                  (cachedDeviceColors.text_color_r !== null) ? cachedDeviceColors.text_color_r : 255;
    var origG   = (importedExtras.text_color_g !== undefined) ? importedExtras.text_color_g :
                  (cachedDeviceColors.text_color_g !== null) ? cachedDeviceColors.text_color_g : 255;
    var origB   = (importedExtras.text_color_b !== undefined) ? importedExtras.text_color_b :
                  (cachedDeviceColors.text_color_b !== null) ? cachedDeviceColors.text_color_b : 255;
    var restorePayload = { brightness: origBrt, text_color_r: origR, text_color_g: origG, text_color_b: origB };

    wizardTestStatus.textContent = 'Testing colors...';
    wizardTestStatus.style.display = '';
    btnTestYes.disabled = true;
    btnTestNo.disabled = true;

    var colors = [
      { text_color_r: 255, text_color_g: 0, text_color_b: 0 },
      { text_color_r: 0, text_color_g: 255, text_color_b: 0 },
      { text_color_r: 0, text_color_g: 0, text_color_b: 255 }
    ];

    var step = 0;

    function restoreAndFinish(warnMsg) {
      // Always restore originals before calling finishStep6() to prevent the save POST
      // from racing with an in-flight restore POST (race condition fix).
      if (warnMsg) FW.showToast(warnMsg, 'warning');
      FW.post('/api/settings', restorePayload).then(function(res) {
        if (!res.body.ok) FW.showToast('Warning: could not restore original colors', 'warning');
        finishStep6();
      }).catch(function() {
        FW.showToast('Warning: could not restore original colors', 'warning');
        finishStep6();
      });
    }

    function nextColor() {
      if (step >= colors.length) {
        restoreAndFinish(null);
        return;
      }

      var payload = { brightness: 40 };
      payload.text_color_r = colors[step].text_color_r;
      payload.text_color_g = colors[step].text_color_g;
      payload.text_color_b = colors[step].text_color_b;

      FW.post('/api/settings', payload).then(function(res) {
        if (!res.body.ok) {
          FW.showToast('Color test failed: ' + (res.body.error || 'Unknown error'), 'error');
          restoreAndFinish(null);
          return;
        }
        step++;
        setTimeout(nextColor, 1000);
      }).catch(function() {
        FW.showToast('Color test failed: Network error', 'error');
        restoreAndFinish(null);
      });
    }

    nextColor();
  }

  function finishStep6() {
    // Show "Saving..." while the POST is in flight; saveAndConnect() updates to the
    // "ready" message after a successful reboot response (AC5 sequencing fix).
    wizardTestStatus.textContent = 'Saving...';
    wizardTestStatus.style.display = '';
    btnTestYes.style.display = 'none';
    btnTestNo.style.display = 'none';

    // Use existing saveAndConnect() which handles save + reboot + handoff
    saveAndConnect();
  }

  // Step 6 event handlers
  if (btnTestYes) {
    btnTestYes.addEventListener('click', function() {
      btnTestYes.disabled = true;
      btnTestNo.disabled = true;
      // Stop positioning pattern, then run RGB test
      FW.post('/api/positioning/stop', {}).then(function(res) {
        if (!res.body.ok) FW.showToast('Warning: could not stop test pattern', 'warning');
        runRgbTest();
      }).catch(function() {
        FW.showToast('Warning: could not stop test pattern', 'warning');
        runRgbTest();
      });
    });
  }

  if (btnTestNo) {
    btnTestNo.addEventListener('click', function() {
      // Debounce: prevent double-tap from firing concurrent stop POSTs
      btnTestNo.disabled = true;
      btnTestYes.disabled = true;
      // Stop positioning pattern, then go back to Step 4
      FW.post('/api/positioning/stop', {}).then(function(res) {
        if (!res.body.ok) FW.showToast('Warning: could not stop test pattern', 'warning');
        showStep(4);
      }).catch(function() {
        FW.showToast('Warning: could not stop test pattern', 'warning');
        showStep(4);
      });
    });
  }

  // --- Settings Import ---
  function processImportedSettings(text) {
    // Flush any in-progress DOM input to state FIRST — imported values then win over typed values
    saveCurrentStepState();
    // Reset UI and extras at the start of every attempt — covers re-import and error paths (AC #4)
    importStatus.style.display = 'none';
    importStatus.textContent = '';
    importedExtras = {};
    var parsed;
    try {
      parsed = JSON.parse(text);
    } catch (e) {
      FW.showToast('Could not read settings file \u2014 invalid format', 'error');
      return;
    }
    if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) {
      FW.showToast('Could not read settings file \u2014 invalid format', 'error');
      return;
    }
    var count = 0;
    var i, key, val;
    for (i = 0; i < WIZARD_KEYS.length; i++) {
      key = WIZARD_KEYS[i];
      if (Object.prototype.hasOwnProperty.call(parsed, key)) {
        val = parsed[key];
        // Skip null and non-primitive values to avoid "null" / "[object Object]" in state
        if (val !== null && typeof val !== 'object') {
          state[key] = String(val);
          count++;
        }
      }
    }
    for (i = 0; i < KNOWN_EXTRA_KEYS.length; i++) {
      key = KNOWN_EXTRA_KEYS[i];
      if (Object.prototype.hasOwnProperty.call(parsed, key)) {
        val = parsed[key];
        if (val !== null && typeof val !== 'object') {
          importedExtras[key] = val;
          count++;
        }
      }
    }
    if (count === 0) {
      FW.showToast('No recognized settings found in file', 'warning');
      return;
    }
    FW.showToast('Imported ' + count + ' settings', 'success');
    importStatus.textContent = count + ' settings imported';
    importStatus.style.display = '';
    showStep(currentStep);
  }

  function handleImportFile(file) {
    if (!file) return;
    // Reset state and UI on every new import attempt — covers early-return paths that never
    // reach processImportedSettings() (file-too-large, FileReader error), preventing stale
    // extras from a prior successful import from surviving into the POST payload (AC #4).
    importedExtras = {};
    importStatus.style.display = 'none';
    importStatus.textContent = '';
    if (file.size > 1024 * 1024) {
      FW.showToast('Settings file too large \u2014 maximum 1\u00a0MB', 'error');
      return;
    }
    var reader = new FileReader();
    reader.onload = function(e) { processImportedSettings(e.target.result); };
    reader.onerror = function() { FW.showToast('Could not read settings file \u2014 invalid format', 'error'); };
    reader.readAsText(file);
  }

  // Click / keyboard handler for import zone
  importZone.addEventListener('click', function() { importFileInput.click(); });
  importZone.addEventListener('keydown', function(e) {
    if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); importFileInput.click(); }
  });
  importFileInput.addEventListener('change', function() {
    if (importFileInput.files && importFileInput.files[0]) {
      handleImportFile(importFileInput.files[0]);
    }
    importFileInput.value = '';
  });

  // Drag-and-drop for import zone
  importZone.addEventListener('dragenter', function(e) { e.preventDefault(); importZone.classList.add('drag-over'); });
  importZone.addEventListener('dragover', function(e) { e.preventDefault(); });
  importZone.addEventListener('dragleave', function(e) {
    if (!importZone.contains(e.relatedTarget)) importZone.classList.remove('drag-over');
  });
  importZone.addEventListener('drop', function(e) {
    e.preventDefault();
    importZone.classList.remove('drag-over');
    if (e.dataTransfer.files && e.dataTransfer.files[0]) {
      handleImportFile(e.dataTransfer.files[0]);
    }
  });

  // --- Init ---
  hydrateDefaults();
  showStep(1);
  startScan();
})();


]]></file>
<file id="5e907ec9" path="tests/e2e/mock-server/server.ts" label="SOURCE CODE"><![CDATA[

/**
 * FlightWall Mock Server
 *
 * A lightweight mock server that simulates the ESP32 FlightWall web portal.
 * Used for local E2E test development without requiring actual hardware.
 *
 * Features:
 * - Serves static HTML/CSS/JS from firmware/data-src/
 * - Provides mock API endpoints matching the real device
 * - Simulates device state (settings, status, layout)
 * - Supports both AP mode (wizard) and STA mode (dashboard)
 *
 * Usage:
 *   npx tsx mock-server/server.ts
 *   npx tsx mock-server/server.ts --port 3001 --mode ap
 */
import http, { type IncomingMessage, type ServerResponse } from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

// ============================================================================
// Configuration
// ============================================================================

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '../../..');
const WEB_ASSETS_DIR = path.join(PROJECT_ROOT, 'firmware/data-src');

// Parse command line arguments
const args = process.argv.slice(2);
const PORT = parseInt(args.find((a) => a.startsWith('--port='))?.split('=')[1] ?? '3000', 10);
const MODE = (args.find((a) => a.startsWith('--mode='))?.split('=')[1] ?? 'sta') as 'ap' | 'sta';

// ============================================================================
// Mock Device State
// ============================================================================

interface DeviceState {
  settings: {
    brightness: number;
    text_color_r: number;
    text_color_g: number;
    text_color_b: number;
    fetch_interval: number;
    display_cycle: number;
    tiles_x: number;
    tiles_y: number;
    tile_pixels: number;
    display_pin: number;
    origin_corner: number;
    scan_dir: number;
    zigzag: number;
    center_lat: number;
    center_lon: number;
    radius_km: number;
    wifi_ssid: string;
    logo_width_pct: number;
    flight_height_pct: number;
    layout_mode: number;
  };
  status: {
    subsystems: Record<string, { level: string; message: string }>;
    wifi_detail: { ip: string; rssi: number; ssid: string; mode: string };
    device: {
      heap_free: number;
      heap_total: number;
      uptime_seconds: number;
      firmware_version: string;
    };
    flight: { current_count: number; last_fetch_ms: number; current_index: number };
    quota: { aeroapi_calls: number; aeroapi_limit: number };
  };
  wifiNetworks: Array<{ ssid: string; rssi: number; encryption: number; channel: number }>;
  logos: Array<{ name: string; size: number }>;
  isScanning: boolean;
}

const state: DeviceState = {
  settings: {
    brightness: 40,
    text_color_r: 255,
    text_color_g: 255,
    text_color_b: 255,
    fetch_interval: 30,
    display_cycle: 5,
    tiles_x: 10,
    tiles_y: 2,
    tile_pixels: 16,
    display_pin: 25,
    origin_corner: 0,
    scan_dir: 0,
    zigzag: 0,
    center_lat: 37.7749,
    center_lon: -122.4194,
    radius_km: 10,
    wifi_ssid: 'MockNetwork',
    logo_width_pct: 0,
    flight_height_pct: 0,
    layout_mode: 0,
  },
  status: {
    subsystems: {
      wifi: { level: 'ok', message: MODE === 'ap' ? 'AP Mode Active' : 'Connected, -52 dBm' },
      opensky: { level: MODE === 'ap' ? 'error' : 'ok', message: MODE === 'ap' ? 'Not configured' : 'Authenticated ✓' },
      aeroapi: { level: MODE === 'ap' ? 'error' : 'ok', message: MODE === 'ap' ? 'Not configured' : 'Connected ✓' },
      cdn: { level: MODE === 'ap' ? 'error' : 'ok', message: MODE === 'ap' ? 'No network' : 'Reachable ✓' },
      nvs: { level: 'ok', message: 'OK' },
      littlefs: { level: 'ok', message: '847KB / 1.9MB used' },
    },
    wifi_detail: {
      ip: MODE === 'ap' ? '192.168.4.1' : '192.168.1.100',
      rssi: MODE === 'ap' ? 0 : -52,
      ssid: MODE === 'ap' ? 'FlightWall-Setup' : 'MockNetwork',
      mode: MODE,
    },
    device: {
      heap_free: 180000,
      heap_total: 320000,
      uptime_seconds: 3600,
      firmware_version: '1.0.0-mock',
    },
    flight: {
      current_count: MODE === 'ap' ? 0 : 3,
      last_fetch_ms: MODE === 'ap' ? 0 : 5000,
      current_index: 0,
    },
    quota: {
      aeroapi_calls: 150,
      aeroapi_limit: 1000,
    },
  },
  wifiNetworks: [
    { ssid: 'HomeNetwork', rssi: -45, encryption: 3, channel: 6 },
    { ssid: 'Neighbor_5G', rssi: -65, encryption: 3, channel: 36 },
    { ssid: 'CoffeeShop', rssi: -70, encryption: 0, channel: 11 },
    { ssid: 'Airport_Free', rssi: -80, encryption: 0, channel: 1 },
  ],
  logos: [
    { name: 'UAL', size: 2048 },
    { name: 'DAL', size: 2048 },
    { name: 'AAL', size: 2048 },
    { name: 'SWA', size: 2048 },
  ],
  isScanning: false,
};

// ============================================================================
// Layout Calculation (mirrors firmware LayoutEngine)
// ============================================================================

function computeLayout() {
  const { tiles_x, tiles_y, tile_pixels, logo_width_pct, flight_height_pct, layout_mode } = state.settings;
  const mw = tiles_x * tile_pixels;
  const mh = tiles_y * tile_pixels;

  let mode: string;
  if (mh < 32) mode = 'compact';
  else if (mh < 48) mode = 'full';
  else mode = 'expanded';

  let logoW = mh;
  if (logo_width_pct > 0 && logo_width_pct <= 99) {
    logoW = Math.round((mw * logo_width_pct) / 100);
    logoW = Math.max(1, Math.min(mw - 1, logoW));
  }

  let splitY = Math.floor(mh / 2);
  if (flight_height_pct > 0 && flight_height_pct <= 99) {
    splitY = Math.round((mh * flight_height_pct) / 100);
    splitY = Math.max(1, Math.min(mh - 1, splitY));
  }

  let logoZone, flightZone, telemetryZone;
  if (layout_mode === 1) {
    logoZone = { x: 0, y: 0, w: logoW, h: splitY };
    flightZone = { x: logoW, y: 0, w: mw - logoW, h: splitY };
    telemetryZone = { x: 0, y: splitY, w: mw, h: mh - splitY };
  } else {
    logoZone = { x: 0, y: 0, w: logoW, h: mh };
    flightZone = { x: logoW, y: 0, w: mw - logoW, h: splitY };
    telemetryZone = { x: logoW, y: splitY, w: mw - logoW, h: mh - splitY };
  }

  return {
    matrix: { width: mw, height: mh, mode },
    logo_zone: logoZone,
    flight_zone: flightZone,
    telemetry_zone: telemetryZone,
    hardware: { tiles_x, tiles_y, tile_pixels },
  };
}

// ============================================================================
// HTTP Request Handling
// ============================================================================

const MIME_TYPES: Record<string, string> = {
  '.html': 'text/html',
  '.css': 'text/css',
  '.js': 'application/javascript',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.ico': 'image/x-icon',
};

function sendJson(res: ServerResponse, data: unknown, status = 200): void {
  res.writeHead(status, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(data));
}

function sendError(res: ServerResponse, error: string, code: string, status = 400): void {
  sendJson(res, { ok: false, error, code }, status);
}

function readBody(req: IncomingMessage): Promise<string> {
  return new Promise((resolve, reject) => {
    let body = '';
    req.on('data', (chunk) => (body += chunk.toString()));
    req.on('end', () => resolve(body));
    req.on('error', reject);
  });
}

async function handleRequest(req: IncomingMessage, res: ServerResponse): Promise<void> {
  const url = new URL(req.url ?? '/', `http://localhost:${PORT}`);
  const pathname = url.pathname;
  const method = req.method ?? 'GET';

  // CORS headers
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, DELETE, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  // API routes
  if (pathname.startsWith('/api/')) {
    await handleApiRoute(pathname, method, req, res);
    return;
  }

  // Static file serving
  await handleStaticFile(pathname, res);
}

async function handleApiRoute(
  pathname: string,
  method: string,
  req: IncomingMessage,
  res: ServerResponse
): Promise<void> {
  // GET /api/settings
  if (pathname === '/api/settings' && method === 'GET') {
    sendJson(res, { ok: true, data: state.settings });
    return;
  }

  // POST /api/settings
  if (pathname === '/api/settings' && method === 'POST') {
    const body = await readBody(req);
    let data: Record<string, unknown>;
    try {
      data = JSON.parse(body);
    } catch {
      sendError(res, 'Invalid JSON', 'INVALID_JSON');
      return;
    }

    if (Object.keys(data).length === 0) {
      sendError(res, 'No settings provided', 'EMPTY_PAYLOAD');
      return;
    }

    const rebootKeys = [
      'wifi_ssid',
      'wifi_password',
      'os_client_id',
      'os_client_sec',
      'aeroapi_key',
      'display_pin',
      'tiles_x',
      'tiles_y',
      'tile_pixels',
    ];

    const applied: string[] = [];
    let rebootRequired = false;

    for (const [key, value] of Object.entries(data)) {
      if (key in state.settings) {
        (state.settings as Record<string, unknown>)[key] = value;
        applied.push(key);
        if (rebootKeys.includes(key)) {
          rebootRequired = true;
        }
      }
    }

    sendJson(res, { ok: true, applied, reboot_required: rebootRequired });
    return;
  }

  // GET /api/status
  if (pathname === '/api/status' && method === 'GET') {
    // Update uptime
    state.status.device.uptime_seconds += 1;
    sendJson(res, { ok: true, data: state.status });
    return;
  }

  // GET /api/layout
  if (pathname === '/api/layout' && method === 'GET') {
    sendJson(res, { ok: true, data: computeLayout() });
    return;
  }

  // GET /api/wifi/scan
  if (pathname === '/api/wifi/scan' && method === 'GET') {
    if (!state.isScanning) {
      state.isScanning = true;
      sendJson(res, { ok: true, scanning: true, data: [] });
      setTimeout(() => {
        state.isScanning = false;
      }, 2000);
    } else {
      sendJson(res, { ok: true, scanning: false, data: state.wifiNetworks });
    }
    return;
  }

  // GET /api/logos
  if (pathname === '/api/logos' && method === 'GET') {
    sendJson(res, {
      ok: true,
      data: state.logos,
      storage: {
        used: state.logos.length * 2048,
        total: 1900000,
        logo_count: state.logos.length,
      },
    });
    return;
  }

  // POST /api/positioning/start — starts LED tile positioning pattern (Step 6)
  if (pathname === '/api/positioning/start' && method === 'POST') {
    sendJson(res, { ok: true, message: 'Positioning pattern started' });
    return;
  }

  // POST /api/positioning/stop — stops LED tile positioning pattern (Step 6)
  if (pathname === '/api/positioning/stop' && method === 'POST') {
    sendJson(res, { ok: true, message: 'Positioning pattern stopped' });
    return;
  }

  // POST /api/reboot
  if (pathname === '/api/reboot' && method === 'POST') {
    sendJson(res, { ok: true, message: 'Rebooting...' });
    return;
  }

  // POST /api/reset
  if (pathname === '/api/reset' && method === 'POST') {
    sendJson(res, { ok: true, message: 'Factory reset complete' });
    return;
  }

  // POST /api/calibration
  if (pathname === '/api/calibration' && method === 'POST') {
    sendJson(res, { ok: true, message: 'Calibration mode toggled' });
    return;
  }

  // 404 for unknown API routes
  sendError(res, 'Not found', 'NOT_FOUND', 404);
}

async function handleStaticFile(pathname: string, res: ServerResponse): Promise<void> {
  // Default to index page based on mode
  if (pathname === '/') {
    pathname = MODE === 'ap' ? '/wizard.html' : '/dashboard.html';
  }

  const filePath = path.join(WEB_ASSETS_DIR, pathname);
  const ext = path.extname(filePath);
  const mimeType = MIME_TYPES[ext] ?? 'application/octet-stream';

  try {
    const content = await fs.promises.readFile(filePath);
    res.writeHead(200, { 'Content-Type': mimeType });
    res.end(content);
  } catch {
    res.writeHead(404, { 'Content-Type': 'text/plain' });
    res.end('Not found');
  }
}

// ============================================================================
// Server Startup
// ============================================================================

const server = http.createServer(handleRequest);

server.listen(PORT, () => {
  console.log(`
╔════════════════════════════════════════════════════════════╗
║       FlightWall Mock Server                               ║
╠════════════════════════════════════════════════════════════╣
║  Mode:    ${MODE === 'ap' ? 'AP (Wizard)    ' : 'STA (Dashboard)'}                                ║
║  URL:     http://localhost:${PORT}                            ║
║  Assets:  ${WEB_ASSETS_DIR.slice(-40).padEnd(40)}   ║
╚════════════════════════════════════════════════════════════╝

API Endpoints:
  GET  /api/settings     - Current device settings
  POST /api/settings     - Apply settings (JSON body)
  GET  /api/status       - System health status
  GET  /api/layout       - Current display layout
  GET  /api/wifi/scan    - WiFi network scan
  GET  /api/logos        - Uploaded logos list
  POST /api/positioning/start - Start LED tile positioning pattern
  POST /api/positioning/stop  - Stop LED tile positioning pattern
  POST /api/reboot       - Trigger device reboot
  POST /api/reset        - Factory reset

Press Ctrl+C to stop.
  `);
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down...');
  server.close(() => {
    process.exit(0);
  });
});


]]></file>
<file id="36e50130" path="tests/e2e/page-objects/WizardPage.ts" label="SOURCE CODE"><![CDATA[

/**
 * Wizard Page Object
 *
 * Page object for the FlightWall Setup Wizard (AP mode).
 * Handles the multi-step onboarding flow for initial device configuration.
 *
 * Architecture Reference:
 * - Wizard served in AP mode (device acts as access point)
 * - Steps: WiFi (1) → API Keys (2) → Location (3) → Hardware (4) → Review (5) → Test Your Wall (6)
 * - Step 6 triggers positioning pattern, confirms layout, runs RGB test, then saves & reboots
 * - "Yes, it matches" on Step 6 completes the RGB test sequence, saves settings, triggers reboot
 */
import { type Locator, expect } from '@playwright/test';
import { BasePage } from './BasePage.js';

export type WizardStep =
  | 'wifi'
  | 'api'
  | 'location'
  | 'hardware'
  | 'review'
  | 'test';

export class WizardPage extends BasePage {
  // ============================================================================
  // Page Identification
  // ============================================================================

  get path(): string {
    return '/';
  }

  get pageIdentifier(): Locator {
    return this.page.locator('text=FlightWall Setup');
  }

  // ============================================================================
  // Step Navigation
  // ============================================================================

  get stepIndicator(): Locator {
    return this.page.locator('#progress');
  }

  get nextButton(): Locator {
    // Use ID — text is "Next →" for steps 1-5; nav bar hidden on step 6 (has its own buttons)
    return this.page.locator('#btn-next');
  }

  get backButton(): Locator {
    // Use exact ID — avoids matching "No — take me back" on Step 6 which also contains "back"
    return this.page.locator('#btn-back');
  }

  get skipButton(): Locator {
    return this.page.locator('button', { hasText: /skip/i });
  }

  // ============================================================================
  // Step 1: WiFi Configuration
  // ============================================================================

  get wifiSsidInput(): Locator {
    return this.page.locator('#wifi-ssid, [name="wifi_ssid"]');
  }

  get wifiPasswordInput(): Locator {
    return this.page.locator('#wifi-pass, [name="wifi_password"]');
  }

  get wifiScanButton(): Locator {
    return this.page.locator('button', { hasText: /scan/i });
  }

  get wifiNetworkList(): Locator {
    return this.page.locator('.wifi-networks, #scan-results');
  }

  // ============================================================================
  // Step 2: API Credentials
  // ============================================================================

  get openSkyClientIdInput(): Locator {
    return this.page.locator('#os-client-id, [name="os_client_id"]');
  }

  get openSkyClientSecretInput(): Locator {
    return this.page.locator('#os-client-sec, [name="os_client_sec"]');
  }

  get aeroApiKeyInput(): Locator {
    return this.page.locator('#aeroapi-key, [name="aeroapi_key"]');
  }

  // ============================================================================
  // Step 3: Location Configuration
  // ============================================================================

  get centerLatInput(): Locator {
    return this.page.locator('#center-lat, [name="center_lat"]');
  }

  get centerLonInput(): Locator {
    return this.page.locator('#center-lon, [name="center_lon"]');
  }

  get radiusKmInput(): Locator {
    return this.page.locator('#radius-km, [name="radius_km"]');
  }

  get useMyLocationButton(): Locator {
    return this.page.locator('button', { hasText: /my location|detect/i });
  }

  get mapContainer(): Locator {
    return this.page.locator('#map, .leaflet-container');
  }

  // ============================================================================
  // Step 4: Hardware Configuration
  // ============================================================================

  get tilesXInput(): Locator {
    return this.page.locator('#tiles-x, [name="tiles_x"]');
  }

  get tilesYInput(): Locator {
    return this.page.locator('#tiles-y, [name="tiles_y"]');
  }

  get tilePixelsInput(): Locator {
    // #tile-pixels is <input type="text">, not a <select>
    return this.page.locator('#tile-pixels, [name="tile_pixels"]');
  }

  get displayPinInput(): Locator {
    return this.page.locator('#display-pin, [name="display_pin"]');
  }

  get previewCanvas(): Locator {
    // Use the wizard Step 6 canvas ID; the wizard has no other canvas element
    return this.page.locator('#wizard-position-preview');
  }

  // ============================================================================
  // Step 5: Review
  // ============================================================================

  /** "Configuration Saved" heading shown after save + reboot handoff completes. */
  get completeMessage(): Locator {
    return this.page.locator('h1', { hasText: 'Configuration Saved' });
  }

  // ============================================================================
  // Step 6: Test Your Wall
  // ============================================================================

  get testPositionCanvas(): Locator {
    return this.page.locator('#wizard-position-preview');
  }

  get testYesButton(): Locator {
    return this.page.locator('#btn-test-yes');
  }

  get testNoButton(): Locator {
    return this.page.locator('#btn-test-no');
  }

  get testStatusText(): Locator {
    return this.page.locator('#wizard-test-status');
  }

  // ============================================================================
  // Settings Import/Export (Foundation Release)
  // ============================================================================

  get importSettingsButton(): Locator {
    // Import zone is a div[role="button"], not a <button> element
    return this.page.locator('#settings-import-zone');
  }

  get importFileInput(): Locator {
    return this.page.locator('input[type="file"]');
  }

  get importStatus(): Locator {
    return this.page.locator('#import-status');
  }

  // ============================================================================
  // Step Navigation Actions
  // ============================================================================

  /**
   * Navigate to the next wizard step.
   */
  async goNext(): Promise<void> {
    await this.nextButton.click();
    await this.page.waitForLoadState('networkidle');
  }

  /**
   * Navigate to the previous wizard step.
   */
  async goBack(): Promise<void> {
    await this.backButton.click();
    await this.page.waitForLoadState('networkidle');
  }

  /**
   * Skip the current optional step.
   */
  async skipStep(): Promise<void> {
    if (await this.skipButton.isVisible()) {
      await this.skipButton.click();
      await this.page.waitForLoadState('networkidle');
    }
  }

  /**
   * Get the current step number (1-based).
   */
  async getCurrentStep(): Promise<number> {
    // Primary: parse the "#progress" indicator text ("Step N of 6")
    const indicator = await this.stepIndicator.textContent();
    const match = indicator?.match(/step\s*(\d+)/i);
    if (match) {
      return parseInt(match[1], 10);
    }
    // Fallback: check which #step-N section is visible (6 steps total)
    for (let i = 1; i <= 6; i++) {
      if (await this.page.locator(`#step-${i}`).isVisible()) return i;
    }
    return 0;
  }

  // ============================================================================
  // Step 1: WiFi Actions
  // ============================================================================

  /**
   * Configure WiFi credentials.
   */
  async configureWifi(ssid: string, password: string): Promise<void> {
    await this.fillInput(this.wifiSsidInput, ssid);
    await this.fillInput(this.wifiPasswordInput, password);
  }

  /**
   * Wait for the automatic WiFi scan to complete and results to appear.
   * Scanning starts automatically when the wizard loads — there is no explicit scan button.
   */
  async scanForNetworks(): Promise<void> {
    // #scan-status hides and #scan-results shows when scan completes (or times out to empty)
    await expect(this.page.locator('#scan-results')).toBeVisible({ timeout: 10_000 });
  }

  /**
   * Select a network from the scan results.
   */
  async selectNetwork(ssid: string): Promise<void> {
    const networkItem = this.wifiNetworkList.locator(`text=${ssid}`);
    await networkItem.click();
    // Network SSID should be populated
    await expect(this.wifiSsidInput).toHaveValue(ssid);
  }

  // ============================================================================
  // Step 2: API Credentials Actions
  // ============================================================================

  /**
   * Configure API credentials.
   */
  async configureApiCredentials(
    openSkyClientId: string,
    openSkyClientSecret: string,
    aeroApiKey: string
  ): Promise<void> {
    await this.fillInput(this.openSkyClientIdInput, openSkyClientId);
    await this.fillInput(this.openSkyClientSecretInput, openSkyClientSecret);
    await this.fillInput(this.aeroApiKeyInput, aeroApiKey);
  }

  // ============================================================================
  // Step 3: Location Actions
  // ============================================================================

  /**
   * Configure location manually.
   */
  async configureLocation(
    lat: number,
    lon: number,
    radiusKm: number
  ): Promise<void> {
    await this.fillInput(this.centerLatInput, String(lat));
    await this.fillInput(this.centerLonInput, String(lon));
    await this.fillInput(this.radiusKmInput, String(radiusKm));
  }

  /**
   * Use browser geolocation to detect location.
   */
  async useMyLocation(): Promise<void> {
    await this.useMyLocationButton.click();
    // Wait for geolocation to complete (browser permission required)
    await this.page.waitForFunction(
      () => {
        const latInput = document.querySelector(
          '#center-lat, [name="center_lat"]'
        ) as HTMLInputElement;
        return latInput && latInput.value !== '';
      },
      { timeout: 10_000 }
    );
  }

  // ============================================================================
  // Step 4: Hardware Actions
  // ============================================================================

  /**
   * Configure matrix dimensions.
   */
  async configureMatrix(
    tilesX: number,
    tilesY: number,
    tilePixels: number
  ): Promise<void> {
    await this.fillInput(this.tilesXInput, String(tilesX));
    await this.fillInput(this.tilesYInput, String(tilesY));
    await this.fillInput(this.tilePixelsInput, String(tilePixels));
  }

  /**
   * Set the LED data pin.
   */
  async setDisplayPin(pin: number): Promise<void> {
    await this.fillInput(this.displayPinInput, String(pin));
  }

  // ============================================================================
  // Step 5: Review Actions
  // ============================================================================

  // Step 5 is now a pass-through review step — clicking Next advances to Step 6.

  // ============================================================================
  // Step 6: Test Your Wall Actions
  // ============================================================================

  /**
   * On Step 6, confirm the layout matches ("Yes, it matches") to run the RGB color
   * test sequence and then save + reboot. Waits for the handoff message.
   */
  async completeWizard(): Promise<void> {
    // Step 6 shows the "Yes, it matches" primary button; nav bar is hidden.
    await expect(this.testYesButton).toBeVisible({ timeout: 5_000 });
    await this.testYesButton.click();
    // Wait for the handoff message — RGB test → save → reboot sequence completes
    await expect(this.completeMessage).toBeVisible({ timeout: 15_000 });
  }

  // ============================================================================
  // Full Wizard Flow
  // ============================================================================

  /**
   * Complete the entire 6-step wizard with provided configuration.
   * Steps: WiFi (1) → API Keys (2) → Location (3) → Hardware (4) → Review (5) → Test Your Wall (6).
   * On Step 6, clicks "Yes, it matches" to trigger RGB test → save → reboot.
   */
  async completeSetup(config: {
    wifi: { ssid: string; password: string };
    api: {
      openSkyClientId: string;
      openSkyClientSecret: string;
      aeroApiKey: string;
    };
    location: { lat: number; lon: number; radiusKm: number };
    hardware: { tilesX: number; tilesY: number; tilePixels: number; displayPin?: number };
  }): Promise<void> {
    // Step 1: WiFi
    await this.configureWifi(config.wifi.ssid, config.wifi.password);
    await this.goNext();

    // Step 2: API Credentials
    await this.configureApiCredentials(
      config.api.openSkyClientId,
      config.api.openSkyClientSecret,
      config.api.aeroApiKey
    );
    await this.goNext();

    // Step 3: Location
    await this.configureLocation(
      config.location.lat,
      config.location.lon,
      config.location.radiusKm
    );
    await this.goNext();

    // Step 4: Hardware
    await this.configureMatrix(
      config.hardware.tilesX,
      config.hardware.tilesY,
      config.hardware.tilePixels
    );
    if (config.hardware.displayPin !== undefined) {
      await this.setDisplayPin(config.hardware.displayPin);
    }
    await this.goNext();

    // Step 5: Review — click Next to advance to Step 6
    await this.goNext();

    // Step 6: Test Your Wall — click "Yes, it matches" to complete
    await this.completeWizard();
  }

  // ============================================================================
  // Assertions
  // ============================================================================

  /**
   * Assert the wizard is on a specific step.
   */
  async expectStep(step: number): Promise<void> {
    const currentStep = await this.getCurrentStep();
    expect(currentStep).toBe(step);
  }

  /**
   * Assert WiFi scan results are displayed and contain at least one network.
   */
  async expectNetworksVisible(): Promise<void> {
    await expect(this.wifiNetworkList).toBeVisible();
    // Scan rows are .scan-row divs rendered by showScanResults() in wizard.js
    const networks = this.wifiNetworkList.locator('.scan-row');
    const count = await networks.count();
    expect(count).toBeGreaterThan(0);
  }

  /**
   * Assert the preview canvas is rendering.
   */
  async expectPreviewVisible(): Promise<void> {
    await expect(this.previewCanvas).toBeVisible();
  }
}


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="Architecture (fallback - epics file should have relevant sections)" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-13</var>
<var name="default_output_file">_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md</var>
<var name="description">Create the next user story from epics+stories with enhanced context analysis and direct ready-for-dev marking</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-3</var>
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
<var name="story_id">fn-3.2</var>
<var name="story_key">fn-3-2-wizard-timezone-auto-detect</var>
<var name="story_num">2</var>
<var name="story_title">wizard-timezone-auto-detect</var>
<var name="timestamp">20260413_1814</var>
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

    <action>Create file using the output-template format at: /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md</action>
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