<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: le-1.7 -->
<!-- Phase: create-story -->
<!-- Timestamp: 20260417T201814Z -->
<compiled-workflow>
<mission><![CDATA[

Create the next user story from epics+stories with enhanced context analysis and direct ready-for-dev marking

Target: Story le-1.7 - editor-property-panel-and-save-ux
Create comprehensive developer context and implementation-ready story.

]]></mission>
<context>
<file id="git-intel" path="[git-intelligence]"><![CDATA[<git-intelligence>
Git intelligence extracted at compile time. Do NOT run additional git commands - use this embedded data instead.

### Recent Commits (last 5)
```
81bf6a1 dl-7-3: OTA pull and dashboard stack, mode schedule, tests, BMad prompts
767e217 Update bmad-assist.yaml to use gemini-3-flash-preview model for multi-provider and validation phases. Modify state.yaml to reflect the current epic and story status, including new completed stories. Change loop structure to review_remediate for improved workflow. Update running.lock with new run details and adjust continual learning state files to include additional transcript entries. Remove outdated implementation artifacts related to project infrastructure and configuration management.
e206a92 Update bmad-assist.yaml to skip story and epic prompts during execution. Adjust budget estimates for story creation, validation, development, and code review to accommodate increased complexity. Modify resource limits and context budgets to enhance performance. Update state.yaml to reflect the current epic and story status, and remove completed stories. Delete outdated prompt files to streamline the project structure.
3c703d8 new commit
898c09f Enhance AGENTS.md with updated BMAD workflow details, including project context handling and bmad-assist commands. Modify bmad-assist.yaml to adjust budget estimates for story creation and validation. Update state.yaml to reflect current epic and story status. Revise sprint-status.yaml to streamline development status and remove outdated comments. Update epics.md to reflect completed and new epics, and adjust firmware partition layout in custom_partitions.csv. Log firmware version at boot in main.cpp.
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

## BMAD / bmad-assist

- **`bmad-assist.yaml`** at repo root configures providers and phases; `paths.project_knowledge` points at `_bmad-output/planning-artifacts/`, `paths.output_folder` at `_bmad-output/`.
- **This file** (`project-context.md`) is resolved at `_bmad-output/project-context.md` or `docs/project-context.md` (see `bmad-assist` compiler `find_project_context_file`).
- Keep **`sprint-status.yaml`** story keys aligned with `.bmad-assist/state.yaml` (`current_story`, `current_epic`) when using `bmad-assist run` so phases do not skip with “story not found”.


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
<file id="92fbb450" path="_bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md" label="DOCUMENTATION"><![CDATA[


# Story LE-1.6: Editor Canvas and Drag-Drop

branch: le-1-6-editor-canvas-and-drag-drop
zone:
  - firmware/data-src/editor.html
  - firmware/data-src/editor.js
  - firmware/data-src/editor.css
  - firmware/data/editor.html.gz
  - firmware/data/editor.js.gz
  - firmware/data/editor.css.gz
  - firmware/adapters/WebPortal.cpp

Status: done

## Story

As a **layout author on my phone**,
I want **to drag and resize widgets on a canvas that honestly previews my LED wall**,
So that **I can build layouts that actually look good on the device without guessing pixel coordinates**.

## Acceptance Criteria

1. **Given** a browser opens `http://flightwall.local/editor.html` **When** the page loads **Then** it is served gzip-encoded from LittleFS by `WebPortal` with `Content-Encoding: gzip` and `Content-Type: text/html` **And** renders a canvas showing the correct device pixel dimensions obtained from `GET /api/layout` → `data.matrix.width` × `data.matrix.height` at 4× CSS scale **And** `imageSmoothingEnabled = false` is set on the canvas context.

2. **Given** the canvas is rendered **When** tile boundaries are inspected **Then** a ghosted grid (semi-transparent overlay) divides the canvas into tiles using `data.hardware.tile_pixels`-wide logical-pixel boundaries; the grid redraws whenever the canvas is redrawn.

3. **Given** `GET /api/widgets/types` is called on page load **When** the response is received **Then** each widget type entry in `data[]` produces one draggable toolbox item showing `entry.label`; any entry with a `note` field containing "not yet implemented" is shown with a visual disabled/muted style and cannot be dropped onto the canvas.

4. **Given** a user presses (`pointerdown`) on the canvas over an existing widget **When** they move (`pointermove`) and release (`pointerup`) **Then** the widget's `x` and `y` are updated by the delta in logical canvas pixels (pointer position ÷ CSS scale factor) **And** the canvas is fully re-rendered on every `pointermove` **And** the canvas element has `touch-action: none` set so the browser does not intercept the touch for scrolling.

5. **Given** a user presses on a resize handle (a 6×6 logical-pixel corner square at the bottom-right of the selected widget) **When** they drag **Then** the widget's `w` and `h` are updated by the delta; snap is applied (see AC #6); the canvas re-renders on every `pointermove`.

6. **Given** a snap mode control (three segments: "Default" / "Tidy" / "Fine") **When** the mode is "Default" **Then** all coordinate deltas snap to the nearest 8 logical pixels; **When** "Tidy" **Then** snap to 16 logical pixels (one tile boundary); **When** "Fine" **Then** snap to 1 logical pixel. **And** the selection persists in `localStorage` under key `fw_editor_snap`.

7. **Given** a resize operation would reduce a widget's `w` or `h` below its minimum floor **When** attempted **Then** the resize is clamped to the minimum floor **And** `FW.showToast` is called with an explanatory message (e.g. `"Text widget minimum height: 6 px"`). Minimum floors are the `h.default` value from the `/api/widgets/types` entry for each type (text: h=8, clock: h=8, logo: h=8).

8. **Given** the three gzip assets **When** measured after gzip-9 compression **Then** `editor.html.gz + editor.js.gz + editor.css.gz` total ≤ 20 KB compressed. (Current non-editor web assets total 62,564 bytes; LittleFS partition is 960 KB = 983,040 bytes; ample room exists.)

9. **Given** `editor.js` **When** inspected for ES5 compliance **Then** `grep -E "=>|let |const |` (backtick)` over `firmware/data-src/editor.js` returns empty — no arrow functions, no `let`, no `const`, no template literals.

10. **Given** mobile Safari (iPhone ≥ iOS 15) and Android Chrome (≥ 110) **When** a user performs a drag or resize **Then** the interaction completes correctly without native drag-selection or scroll interception (manual smoke test; document result in Dev Agent Record).

## Tasks / Subtasks

- [x] **Task 1: Create `editor.html` — page scaffold with canvas, toolbox, and control bar** (AC: #1, #2, #3)
  - [x]Create `firmware/data-src/editor.html` — standalone page (does NOT extend `dashboard.html`)
  - [x]`<head>`: charset UTF-8, viewport `width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no`, title "FlightWall Editor", `<link rel="stylesheet" href="/style.css">`, `<link rel="stylesheet" href="/editor.css">`
  - [x]`<body>` structure:
    ```html
    <div class="editor-layout">
      <header class="editor-header">
        <a href="/" class="editor-back">← Dashboard</a>
        <h1>Layout Editor</h1>
      </header>
      <div class="editor-main">
        <div class="editor-toolbox" id="editor-toolbox">
          <!-- populated by JS from /api/widgets/types -->
        </div>
        <div class="editor-canvas-wrap">
          <canvas id="editor-canvas"></canvas>
        </div>
        <div class="editor-props" id="editor-props">
          <!-- LE-1.7: property panel placeholder -->
          <p class="props-placeholder">Select a widget</p>
        </div>
      </div>
      <div class="editor-controls">
        <div class="snap-control" id="snap-control">
          <button type="button" class="snap-btn" data-snap="8">Default</button>
          <button type="button" class="snap-btn" data-snap="16">Tidy</button>
          <button type="button" class="snap-btn" data-snap="1">Fine</button>
        </div>
        <span class="editor-status" id="editor-status"></span>
      </div>
    </div>
    ```
  - [x]Script tags at bottom: `<script src="/common.js"></script>` then `<script src="/editor.js"></script>`
  - [x]**No inline scripts or styles**

- [x] **Task 2: Create `editor.css` — canvas and layout styles** (AC: #1, #2, #4)
  - [x]Create `firmware/data-src/editor.css`
  - [x]Canvas pixelated rendering: `#editor-canvas { image-rendering: pixelated; image-rendering: crisp-edges; touch-action: none; cursor: crosshair; }`
  - [x]Canvas CSS dimensions set to 4× the logical device dimensions (set dynamically by JS after `/api/layout` — do not hard-code in CSS; canvas element `width`/`height` attributes are set by JS too)
  - [x]Layout: `editor-layout` as a flex column; `editor-main` as a flex row with toolbox | canvas-wrap | props; controls bar at bottom
  - [x]Snap button active state: `.snap-btn.active { background: var(--accent, #58a6ff); color: #000; }`
  - [x]Toolbox item styles: `.toolbox-item { ... cursor: grab; }` and `.toolbox-item.disabled { opacity: 0.4; cursor: not-allowed; }`
  - [x]Mobile-friendly: on narrow viewports, toolbox becomes a horizontal row above the canvas

- [x] **Task 3: Create `editor.js` — module globals and canvas render loop** (AC: #1, #2, #9)
  - [x]Create `firmware/data-src/editor.js` — **ES5 only** (`var`, function declarations, no arrow functions, no `let`/`const`, no template literals, no `class`)
  - [x]Module globals at top of IIFE:
    ```javascript
    var SCALE = 4;                // CSS pixels per logical pixel
    var SNAP = 8;                 // current snap grid (8 / 16 / 1)
    var deviceW = 0;              // logical canvas width (from /api/layout)
    var deviceH = 0;              // logical canvas height
    var tilePixels = 16;          // tile size in logical pixels (from /api/layout hardware.tile_pixels)
    var widgets = [];             // [{type, x, y, w, h, color, content, align, id}]
    var selectedIndex = -1;       // index into widgets[], -1 = none
    var dragState = null;         // {mode: 'move'|'resize', startX, startY, origX, origY, origW, origH}
    var widgetTypeMeta = {};      // keyed by type string, value = entry from /api/widgets/types
    ```
  - [x]`render()` function — called on every state change:
    ```javascript
    function render() {
      var canvas = document.getElementById('editor-canvas');
      var ctx = canvas.getContext('2d');
      ctx.imageSmoothingEnabled = false;
      // 1. Clear
      ctx.fillStyle = '#111';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      // 2. Draw ghosted tile grid
      drawGrid(ctx);
      // 3. Draw widgets
      for (var i = 0; i < widgets.length; i++) {
        drawWidget(ctx, widgets[i], i === selectedIndex);
      }
    }
    ```
  - [x]`drawGrid(ctx)`: draws vertical and horizontal lines every `tilePixels * SCALE` CSS pixels using `ctx.strokeStyle = 'rgba(255,255,255,0.1)'` and `ctx.lineWidth = 1`
  - [x]`drawWidget(ctx, w, isSelected)`: draws a filled rectangle at `(w.x * SCALE, w.y * SCALE, w.w * SCALE, w.h * SCALE)` filled with the widget's color at 80% opacity; draws the type string label centered in the box; if `isSelected`, draws a 1-CSS-px white selection outline + a 6×6 CSS-px resize handle square at the bottom-right corner
  - [x]`initCanvas(matrixW, matrixH, tilePx)`: sets `deviceW`, `deviceH`, `tilePixels`, sets canvas `width` = `matrixW * SCALE`, `height` = `matrixH * SCALE`, sets CSS `width`/`height` in same values (no CSS scaling — the 4× expansion is in the logical canvas pixels themselves), calls `render()`
  - [x]On `DOMContentLoaded`: call `loadLayout()` which calls `FW.get('/api/layout')` then `FW.get('/api/widgets/types')` and initializes everything

- [x] **Task 4: Implement pointer events for drag and resize** (AC: #4, #5, #6, #7)
  - [x]`hitTest(cx, cy)`: given CSS coordinates, returns `{index, mode}` where `mode` is `'resize'` if the pointer is within 8 CSS px of the bottom-right corner of the selected widget, `'move'` if inside any widget bounding box, `null` if no hit
  - [x]`canvas.addEventListener('pointerdown', onPointerDown)`: call `hitTest`; if hit, set `selectedIndex`, set `dragState = {mode, startX: e.clientX, startY: e.clientY, origX: w.x, origY: w.y, origW: w.w, origH: w.h}`; call `canvas.setPointerCapture(e.pointerId)`; call `render()`
  - [x]`canvas.addEventListener('pointermove', onPointerMove)`: if `dragState == null` return; compute delta `dx = (e.clientX - dragState.startX) / SCALE`, `dy = (e.clientY - dragState.startY) / SCALE`; apply snap: `dx = Math.round(dx / SNAP) * SNAP`; apply to widget; clamp to canvas bounds; call `render()`
  - [x]`canvas.addEventListener('pointerup', onPointerUp)`: if `dragState == null` return; clear `dragState = null`; update status bar
  - [x]Min-floor enforcement in `onPointerMove` during resize: use `widgetTypeMeta[type].minH` and `widgetTypeMeta[type].minW` (see Task 6 for how these are derived from the API); if clamped, call `FW.showToast('...')` at most once per drag gesture (use a `toastedFloor` flag on `dragState`)
  - [x]**Critical**: canvas must have `touch-action: none` (set in CSS — Task 2). Also call `e.preventDefault()` inside `onPointerDown` to be belt-and-braces on iOS Safari.

- [x] **Task 5: Toolbox population from `/api/widgets/types`** (AC: #3)
  - [x]`initToolbox(types)`: iterates `types` array; for each entry, creates a `<div class="toolbox-item" data-type="...">LABEL</div>`; if `entry.note` contains "not yet implemented", add class `disabled`; append to `#editor-toolbox`
  - [x]Clicking a non-disabled toolbox item calls `addWidget(type)`:
    ```javascript
    function addWidget(type) {
      var meta = widgetTypeMeta[type];
      if (!meta) return;
      var w = {
        type: type,
        x: snapTo(8, SNAP),
        y: snapTo(8, SNAP),
        w: meta.defaultW,
        h: meta.defaultH,
        color: meta.defaultColor || '#FFFFFF',
        content: meta.defaultContent || '',
        align: meta.defaultAlign || 'left',
        id: 'w' + Date.now()
      };
      widgets.push(w);
      selectedIndex = widgets.length - 1;
      render();
    }
    ```
  - [x]`widgetTypeMeta` is populated in `loadLayout()` from the `GET /api/widgets/types` response; for each entry, store `{defaultW, defaultH, defaultColor, defaultContent, defaultAlign, minW, minH}` by reading the `fields` array (the field with `key === 'w'` → `default` value is `defaultW`, etc.)

- [x] **Task 6: Snap mode control** (AC: #6)
  - [x]On `DOMContentLoaded`, read `localStorage.getItem('fw_editor_snap')` → parse as integer; default to `8` if absent or invalid; set `SNAP` to the value; add `active` class to the matching `.snap-btn`
  - [x]Each `.snap-btn` click: read `data-snap` attribute, set `SNAP = parseInt(snapVal, 10)`, save to `localStorage.setItem('fw_editor_snap', SNAP)`, update button active states, call `render()`
  - [x]Helper `snapTo(val, grid)`: `return Math.round(val / grid) * grid`

- [x] **Task 7: Add WebPortal routes for editor assets** (AC: #1)
  - [x]Open `firmware/adapters/WebPortal.cpp`
  - [x]In `WebPortal::begin()` (the route registration block near line 886), after the existing static asset registrations, add:
    ```cpp
    _server->on("/editor.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.html.gz", "text/html");
    });
    _server->on("/editor.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.js.gz", "application/javascript");
    });
    _server->on("/editor.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.css.gz", "text/css");
    });
    ```
  - [x]**No other changes to `WebPortal.cpp`** — `_serveGzAsset()` already handles the `Content-Encoding: gzip` header and the `LittleFS.exists()` guard (line 1391–1399)
  - [x]**Do NOT** add `/editor` → `editor.html.gz` redirect or any captive-portal change — editor is a standalone page accessed only via direct URL

- [x] **Task 8: Gzip the assets** (AC: #8)
  - [x]From `firmware/` directory, run:
    ```bash
    gzip -9 -c data-src/editor.html > data/editor.html.gz
    gzip -9 -c data-src/editor.js   > data/editor.js.gz
    gzip -9 -c data-src/editor.css  > data/editor.css.gz
    ```
  - [x]Verify total: `wc -c data/editor.html.gz data/editor.js.gz data/editor.css.gz` — sum must be ≤ 20,480 bytes (20 KB)
  - [x]If sum exceeds 20 KB: first check for dead code or verbosity in `editor.js`; the 20 KB gate is achievable — dashboard.js source is 154 KB and compresses to 34,920 bytes; editor.js should be far smaller
  - [x]**Important**: gzip must be re-run every time `editor.html`, `editor.js`, or `editor.css` is modified. The `.gz` files in `firmware/data/` are what gets uploaded to the device.

- [x] **Task 9: Build verification and ES5 audit** (AC: #9)
  - [x]Run `~/.platformio/penv/bin/pio run -e esp32dev` from `firmware/` — must succeed with binary ≤ 1,382,400 bytes
  - [x]ES5 compliance grep: `grep -P "=>|`|(?<![a-zA-Z])let |(?<![a-zA-Z])const " firmware/data-src/editor.js` — must return empty
  - [x]`wc -c firmware/data/editor.html.gz firmware/data/editor.js.gz firmware/data/editor.css.gz` — confirm total ≤ 20,480 bytes
  - [x]`pio run -t uploadfs` — verify LittleFS image builds and uploads successfully

- [ ] **Task 10: Mobile smoke test** (AC: #10)
  - [ ]Flash device with new firmware + `uploadfs`; navigate to `http://flightwall.local/editor.html`
  - [ ]iPhone Safari (iOS 15+): drag a widget, verify no scroll interception, verify drag follows finger
  - [ ]Android Chrome (110+): same test
  - [ ]Document platform, OS version, test result, and any issues found in Dev Agent Record

---

## Dev Notes

### The Change Is Purely Web Assets — Read This First

LE-1.6 is a **web-only story**. Zero firmware C++ logic changes are required beyond adding 3 static route registrations in `WebPortal.cpp`. All existing API endpoints needed by the editor are already implemented:

| API | Story | Purpose |
|---|---|---|
| `GET /api/layout` | 3.1 | Canvas dimensions + tile size |
| `GET /api/widgets/types` | LE-1.4 | Widget palette + field schemas |
| `GET /api/layouts` | LE-1.4 | List saved layouts (LE-1.7 will use) |
| `POST /api/layouts` | LE-1.4 | Save new layout (LE-1.7 will use) |
| `PUT /api/layouts/:id` | LE-1.4 | Update layout (LE-1.7 will use) |
| `POST /api/layouts/:id/activate` | LE-1.4 | Activate layout |

**LE-1.6 uses `GET /api/layout` (canvas size) and `GET /api/widgets/types` (toolbox). Save/load is LE-1.7.**

---

### `GET /api/layout` — Canvas Dimension Source

The editor must call `GET /api/layout` on load to get the correct canvas dimensions. **Never hard-code 192×48** — the device config is user-changeable.

**Response shape** (implemented in `WebPortal.cpp` line 1348):
```json
{
  "ok": true,
  "data": {
    "mode": "full",
    "matrix": { "width": 192, "height": 48 },
    "logo_zone":      { "x": 0,  "y": 0, "w": 48, "h": 48 },
    "flight_zone":    { "x": 48, "y": 0, "w": 144, "h": 24 },
    "telemetry_zone": { "x": 48, "y": 24, "w": 144, "h": 24 },
    "hardware": { "tiles_x": 12, "tiles_y": 3, "tile_pixels": 16 }
  }
}
```

**Canvas initialization pattern:**
```javascript
FW.get('/api/layout').then(function(res) {
  if (!res.body.ok) { FW.showToast('Cannot load layout info', 'error'); return; }
  var d = res.body.data;
  initCanvas(d.matrix.width, d.matrix.height, d.hardware.tile_pixels);
});
```

`initCanvas` sets the canvas `width` and `height` attributes (logical pixel count × SCALE). The tile grid uses `d.hardware.tile_pixels` as the grid interval.

---

### `GET /api/widgets/types` — Toolbox Schema

Response shape (implemented in `WebPortal.cpp` line 2060; complete handler shown above in codebase read):
```json
{
  "ok": true,
  "data": [
    {
      "type": "text",
      "label": "Text",
      "fields": [
        { "key": "content", "kind": "string", "default": "" },
        { "key": "align",   "kind": "select", "default": "left", "options": ["left","center","right"] },
        { "key": "color",   "kind": "color",  "default": "#FFFFFF" },
        { "key": "x", "kind": "int", "default": 0 },
        { "key": "y", "kind": "int", "default": 0 },
        { "key": "w", "kind": "int", "default": 32 },
        { "key": "h", "kind": "int", "default": 8 }
      ]
    },
    {
      "type": "clock",
      "label": "Clock",
      "fields": [ ... "w" default 48, "h" default 8 ]
    },
    {
      "type": "logo",
      "label": "Logo",
      "fields": [ ... "w" default 16, "h" default 16 ]
    },
    {
      "type": "flight_field",
      "label": "Flight Field",
      "note": "LE-1.8 \u2014 not yet implemented, renders nothing",
      "fields": [ ... ]
    },
    {
      "type": "metric",
      "label": "Metric",
      "note": "LE-1.8 \u2014 not yet implemented, renders nothing",
      "fields": [ ... ]
    }
  ]
}
```

**Toolbox population pattern:**
```javascript
FW.get('/api/widgets/types').then(function(res) {
  if (!res.body.ok) return;
  var types = res.body.data;
  for (var i = 0; i < types.length; i++) {
    buildWidgetTypeMeta(types[i]);   // populate widgetTypeMeta map
    addToolboxItem(types[i]);        // create DOM element
  }
});
```

**`buildWidgetTypeMeta(entry)` — extract defaults from fields array:**
```javascript
function buildWidgetTypeMeta(entry) {
  var meta = { defaultW: 32, defaultH: 8, defaultColor: '#FFFFFF',
               defaultContent: '', defaultAlign: 'left',
               minW: 6, minH: 6 };
  if (!entry.fields) { widgetTypeMeta[entry.type] = meta; return; }
  for (var i = 0; i < entry.fields.length; i++) {
    var f = entry.fields[i];
    if (f.key === 'w')       meta.defaultW = f['default'] || 32;
    if (f.key === 'h')       meta.defaultH = f['default'] || 8;
    if (f.key === 'color')   meta.defaultColor = f['default'] || '#FFFFFF';
    if (f.key === 'content') meta.defaultContent = f['default'] || '';
    if (f.key === 'align')   meta.defaultAlign = f['default'] || 'left';
  }
  // Min floors — h.default is the firmware minimum (Adafruit GFX 5×7 font, 1-row floor)
  // w minimum: at least 6 px for any type (one character wide at 5+1 px spacing)
  meta.minH = meta.defaultH;  // firmware enforces w<8||h<8 → early return; use same floor
  meta.minW = 6;
  widgetTypeMeta[entry.type] = meta;
}
```

**Disabled toolbox items** (flight_field, metric): check `entry.note && entry.note.indexOf('not yet implemented') !== -1`.

---

### Canvas Scale Strategy — 4× Logical Pixels

The LED matrix is 192×48 logical pixels (typical config). This is tiny on a phone screen. The solution:
- Canvas `width` attribute = `matrixW * SCALE` = 768
- Canvas `height` attribute = `matrixH * SCALE` = 192
- All coordinates in `widgets[]` are stored in **logical pixels** (0–191, 0–47)
- All canvas drawing uses **CSS coordinates** = logical × SCALE
- All pointer event coordinates come in CSS pixels and must be divided by SCALE before storing

**CSS vs. logical conversion helpers:**
```javascript
function toCss(logical) { return logical * SCALE; }
function toLogical(css) { return css / SCALE; }
```

**Device pixel ratio:** Do NOT multiply canvas dimensions by `window.devicePixelRatio`. The 4× scale is intentional for legibility; HiDPI scaling would make the canvas 8× or 12× the LED dimensions — too large for mobile screens. Keep SCALE fixed at 4.

**`imageSmoothingEnabled = false`**: must be set after every call to `canvas.getContext('2d')` — browsers can reset it. Set it inside `render()` before any drawing:
```javascript
var ctx = canvas.getContext('2d');
ctx.imageSmoothingEnabled = false;
```

---

### Pointer Event Architecture — Hand-Rolled, No Libraries

From the spike report and architecture constraints: **no pointer/touch libraries** (hammer.js etc. are excluded by the LittleFS budget and no-frameworks rule).

**The three-handler pattern:**
```javascript
var canvas = document.getElementById('editor-canvas');
canvas.addEventListener('pointerdown', onPointerDown);
canvas.addEventListener('pointermove', onPointerMove);
canvas.addEventListener('pointerup',   onPointerUp);
// Also handle pointercancel to avoid stuck drag states on iOS:
canvas.addEventListener('pointercancel', onPointerUp);
```

**Pointer capture** (`canvas.setPointerCapture(e.pointerId)`) is critical for mobile — ensures `pointermove` events are delivered even when the finger moves off the canvas element. Call inside `onPointerDown` after setting `dragState`.

**iOS Safari quirk**: `touch-action: none` on the canvas element prevents native scroll/zoom. Additionally, `e.preventDefault()` inside `onPointerDown` provides belt-and-braces prevention on older iOS versions. Both are required (CSS alone is insufficient on some iOS 15 versions).

**Coordinate extraction for mobile:**
```javascript
function getCanvasPos(e) {
  var rect = e.target.getBoundingClientRect();
  return {
    x: e.clientX - rect.left,   // CSS pixels relative to canvas
    y: e.clientY - rect.top
  };
}
```

---

### Hit Testing — Resize vs. Move

The selected widget displays a 6×6 CSS-pixel resize handle at its bottom-right corner. Hit priority:
1. Check resize handle first (6×6 square at bottom-right of selected widget in CSS coordinates)
2. Check widget body (any widget, last-drawn is topmost)
3. Miss → deselect (set `selectedIndex = -1`)

**Resize handle position** (in CSS pixels):
```javascript
var handleSize = 6;  // CSS pixels
var rx = w.x * SCALE + w.w * SCALE - handleSize;
var ry = w.y * SCALE + w.h * SCALE - handleSize;
// Hit if: cx >= rx && cx <= rx+handleSize && cy >= ry && cy <= ry+handleSize
```

**Widget body hit** (in CSS pixels):
```javascript
// Check from last widget to first (topmost rendered = last in array)
for (var i = widgets.length - 1; i >= 0; i--) {
  var w = widgets[i];
  if (cx >= w.x * SCALE && cx <= (w.x + w.w) * SCALE &&
      cy >= w.y * SCALE && cy <= (w.y + w.h) * SCALE) {
    return { index: i, mode: 'move' };
  }
}
```

---

### Snap, Clamp, and Floor Logic

**Snap helper:**
```javascript
function snapTo(val, grid) {
  return Math.round(val / grid) * grid;
}
```

**Move clamping** (keep widget fully within canvas):
```javascript
w.x = Math.max(0, Math.min(deviceW - w.w, snapTo(newX, SNAP)));
w.y = Math.max(0, Math.min(deviceH - w.h, snapTo(newY, SNAP)));
```

**Resize clamping** (min floor + canvas boundary):
```javascript
var meta = widgetTypeMeta[w.type] || { minW: 6, minH: 6 };
var newW = Math.max(meta.minW, Math.min(deviceW - w.x, snapTo(dragState.origW + dw, SNAP)));
var newH = Math.max(meta.minH, Math.min(deviceH - w.y, snapTo(dragState.origH + dh, SNAP)));
if (newW !== w.w || newH !== w.h) {
  if ((snapTo(dragState.origW + dw, SNAP) < meta.minW ||
       snapTo(dragState.origH + dh, SNAP) < meta.minH) &&
      !dragState.toastedFloor) {
    FW.showToast(w.type + ' minimum: ' + meta.minW + 'w \u00d7 ' + meta.minH + 'h px', 'warning');
    dragState.toastedFloor = true;
  }
}
w.w = newW;
w.h = newH;
```

---

### WebPortal Route Pattern — Copy Exactly

The three new routes in `WebPortal.cpp` follow the exact same pattern as the existing static asset routes (lines 887–904). The `_serveGzAsset` helper (lines 1391–1399) handles:
- `LittleFS.exists(path)` check → 404 if missing
- `request->beginResponse(LittleFS, path, contentType)` — streaming from LittleFS
- `response->addHeader("Content-Encoding", "gzip")` — tells browser to decompress

**Insertion point**: after the `health.js` route block (line 903), before the closing `}` of `begin()`:
```cpp
// Layout Editor assets (LE-1.6)
_server->on("/editor.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.html.gz", "text/html");
});
_server->on("/editor.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.js.gz", "application/javascript");
});
_server->on("/editor.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.css.gz", "text/css");
});
```

**Captive portal is unaffected.** The captive portal only serves `/` → `wizard.html.gz` or `dashboard.html.gz`. `/editor.html` is a direct-access page, not served from the captive portal redirect.

---

### `_serveGzAsset` Is a Static Method — No `this`

Looking at the existing registrations (lines 887–904), the lambdas use `[](AsyncWebServerRequest* request)` — no `this` capture — because `_serveGzAsset` is a static method. Use the same `[]` capture (not `[this]`).

```cpp
// CORRECT — _serveGzAsset is static
_server->on("/editor.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.html.gz", "text/html");
});

// WRONG — do not capture [this]
_server->on("/editor.html", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _serveGzAsset(request, "/editor.html.gz", "text/html");
});
```

---

### Gzip Command — Use `-9 -c` (not `-k`)

The correct gzip command per project convention:
```bash
# From firmware/ directory:
gzip -9 -c data-src/editor.html > data/editor.html.gz
gzip -9 -c data-src/editor.js   > data/editor.js.gz
gzip -9 -c data-src/editor.css  > data/editor.css.gz
```

- `-9`: maximum compression
- `-c`: write to stdout (keeps the source file unchanged)
- `>`: redirect to the `.gz` file in `data/`

**Do NOT use** `gzip -k` (which writes `.gz` alongside the source in `data-src/`).

The `.gz` files in `firmware/data/` are tracked by version control (they are the deployable artifacts). The source files in `firmware/data-src/` are also tracked. Both must be committed.

---

### LittleFS Budget Analysis

Current state (measured 2026-04-17):
- LittleFS partition: 960 KB = 983,040 bytes
- Existing web assets in `firmware/data/*.gz`: **62,564 bytes** total
- Logo files in `firmware/data/logos/`: ~404 KB (99 files × ~2 KB each)
- Remaining headroom: ~983,040 − 62,564 − 404,000 ≈ **516 KB**

Editor budget target (AC #8): editor.html.gz + editor.js.gz + editor.css.gz ≤ **20,480 bytes**. This leaves ~496 KB for logos and future assets. Very comfortable.

**Sanity check**: `dashboard.js` (154,698 bytes source) compresses to 34,920 bytes — a 4.4× compression ratio. `editor.js` will be much smaller than `dashboard.js`, so 20 KB compressed is achievable even at moderate compression.

---

### ES5 Constraint — No Exceptions

The project prohibits ES6+ syntax in all `firmware/data-src/*.js` files (CLAUDE.md, project convention). The ESP32 serves these files to any browser including old Android WebViews. Violations:

| FORBIDDEN | USE INSTEAD |
|---|---|
| `const x = ...` | `var x = ...` |
| `let x = ...` | `var x = ...` |
| `(x) => x + 1` | `function(x) { return x + 1; }` |
| `` `string ${var}` `` | `'string ' + var` |
| `class Foo { ... }` | `function Foo() { ... }` constructor pattern |
| `Array.from(...)` | `Array.prototype.slice.call(...)` |
| `Object.assign(...)` | manual property copy loop |

**`FW.get()`/`FW.post()`/`FW.del()`** are available from `common.js` (loaded before `editor.js`). They return Promises and resolve to `{status, body}`. **Promise itself is ES6** — but it is available in all target browsers (iOS 9+, Android 4.1+ WebView). The project already uses Promise in `dashboard.js` and `wizard.js`. Promises are allowed; arrow functions and `let`/`const` are not.

---

### `FW.showToast` Severity Values

```javascript
FW.showToast('message', 'success');  // green
FW.showToast('message', 'error');    // red
FW.showToast('message', 'warning'); // yellow/orange (if styled; falls back to error styling)
```

For the floor toast (AC #7), use `'warning'` or `'error'`. Use `FW.showToast(msg, 'error')` if `'warning'` is not styled in `style.css`.

---

### `common.js` — No Changes Required

`common.js` already provides everything `editor.js` needs: `FW.get()`, `FW.post()`, `FW.del()`, `FW.showToast()`. No modifications to `common.js` are needed for LE-1.6.

---

### Widget Color Rendering on Canvas

`WidgetSpec::color` is stored in layouts as a hex string (`"#FFFFFF"`, `"#0000FF"`). On the canvas, draw widgets using `ctx.fillStyle = w.color`. The canvas will interpret hex color strings natively. No RGB565 conversion needed for the editor preview — the editor is a web canvas, not the LED matrix.

**Widget preview coloring** — suggestion for clarity:
```javascript
function drawWidget(ctx, w, isSelected) {
  var x = w.x * SCALE, y = w.y * SCALE;
  var cw = w.w * SCALE, ch = w.h * SCALE;
  // Fill with widget color at 80% opacity
  ctx.globalAlpha = 0.8;
  ctx.fillStyle = w.color || '#444';
  ctx.fillRect(x, y, cw, ch);
  ctx.globalAlpha = 1.0;
  // Type label (white text, centered)
  ctx.fillStyle = '#FFF';
  ctx.font = (SCALE * 6) + 'px monospace';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText(w.type, x + cw / 2, y + ch / 2);
  // Selection outline
  if (isSelected) {
    ctx.strokeStyle = '#FFF';
    ctx.lineWidth = 1;
    ctx.strokeRect(x, y, cw, ch);
    // Resize handle
    ctx.fillStyle = '#FFF';
    ctx.fillRect(x + cw - 6, y + ch - 6, 6, 6);
  }
}
```

---

### `loadLayout()` — Initialization Sequence

Both API calls must complete before the canvas and toolbox are usable. Use sequential `.then()` chaining (no `Promise.all` — that is ES6 but available in all targets; however sequential chaining is cleaner and avoids any doubt):

```javascript
function loadLayout() {
  FW.get('/api/layout').then(function(res) {
    if (!res.body.ok) {
      FW.showToast('Cannot load layout info — check device connection', 'error');
      return;
    }
    var d = res.body.data;
    initCanvas(d.matrix.width, d.matrix.height, d.hardware.tile_pixels);

    return FW.get('/api/widgets/types');
  }).then(function(res) {
    if (!res || !res.body.ok) return;
    initToolbox(res.body.data);
    updateSnapButtons();
  });
}
```

---

### Antipattern Prevention Table

| DO NOT | DO INSTEAD | Reason |
|---|---|---|
| `canvas.width = 768` (hard-coded) | `canvas.width = d.matrix.width * SCALE` | Device config varies; always read from `/api/layout` |
| `let`, `const`, arrow functions | `var`, `function` | ES5 constraint — project rule; grep gate AC #9 |
| Import any JS library | Use hand-rolled pointer events | LittleFS budget; no-frameworks rule |
| `window.devicePixelRatio` scaling | Fixed 4× SCALE | HiDPI scaling oversizes the canvas on retina phones |
| `ctx.imageSmoothingEnabled = false` once at init | Set it inside `render()` every time | Browsers can reset this property |
| Lambda `[this]` capture in WebPortal routes | Lambda `[]` (no capture) | `_serveGzAsset` is static; `this` is unused |
| `gzip -k data-src/editor.js` | `gzip -9 -c data-src/editor.js > data/editor.js.gz` | `-k` writes to wrong directory |
| Save layout JSON in LE-1.6 | Defer to LE-1.7 | Save/load UX is LE-1.7's scope; LE-1.6 is canvas + drag only |
| `e.preventDefault()` globally on body | `e.preventDefault()` inside `onPointerDown` only | Global preventDefault breaks scrolling on the rest of the page |
| Commit only `.gz` files without source | Commit both `data-src/*.{html,js,css}` and `data/*.gz` | Source is the truth; `.gz` are derived artifacts both tracked |

---

### File Size Targets (Informational)

| File | Source estimate | Compressed target |
|---|---|---|
| `editor.html` | ~3 KB | ~1 KB |
| `editor.js` | ~6–10 KB | ~3–5 KB |
| `editor.css` | ~2–3 KB | ~1 KB |
| **Total** | **~11–16 KB** | **≤ 20 KB** |

For reference, `dashboard.html` is 16,753 bytes source → 4,031 bytes compressed. The editor HTML will be simpler than dashboard. `dashboard.js` is 154,698 bytes → 34,920 bytes. `editor.js` will be far smaller (no settings forms, no IANA timezone map, no OTA, no schedule logic).

---

### Sprint Status Update

After all tasks pass, update `_bmad-output/implementation-artifacts/sprint-status.yaml`:
```yaml
le-1-6-editor-canvas-and-drag-drop: done
```

---

## File List

Files **created**:
- `firmware/data-src/editor.html` — editor page scaffold
- `firmware/data-src/editor.js` — canvas render loop + pointer events (ES5)
- `firmware/data-src/editor.css` — canvas and layout styles
- `firmware/data/editor.html.gz` — gzip-compressed HTML (generated, tracked)
- `firmware/data/editor.js.gz` — gzip-compressed JS (generated, tracked)
- `firmware/data/editor.css.gz` — gzip-compressed CSS (generated, tracked)

Files **modified**:
- `firmware/adapters/WebPortal.cpp` — add 3 static routes (`/editor.html`, `/editor.js`, `/editor.css`) in `begin()`

Files **NOT modified** (verify these are unchanged before committing):
- `firmware/data-src/common.js` — no extensions needed; `FW.get/post/del/showToast` already sufficient
- `firmware/data-src/dashboard.html` — editor is a separate page
- `firmware/data-src/dashboard.js` — no changes
- `firmware/core/WidgetRegistry.h` — no changes
- `firmware/core/LayoutStore.*` — no changes
- `firmware/modes/CustomLayoutMode.cpp` — no changes
- Any firmware `.cpp`/`.h` other than `WebPortal.cpp`

---

## Change Log

| Date | Version | Description | Author |
|---|---|---|---|
| 2026-04-17 | 0.1 | Draft story created (9 AC, 9 tasks, minimal dev notes) | BMAD |
| 2026-04-17 | 1.0 | Upgraded to ready-for-dev. Key additions: (1) Full `/api/layout` response shape documented with actual WebPortal.cpp line references; (2) Complete `GET /api/widgets/types` response shape documented with all 5 widget type schemas showing actual default values from WebPortal.cpp; (3) Canvas scale strategy documented with conversion helpers; (4) Pointer event architecture documented with iOS Safari `pointercancel` requirement; (5) Hit-test algorithm documented with resize-handle priority; (6) `_serveGzAsset` static method + `[]` capture requirement documented; (7) Gzip command corrected to `-9 -c ... >` pattern; (8) LittleFS budget analysis documented (516 KB remaining, well within limits); (9) Complete `buildWidgetTypeMeta()` helper for deriving min-floor and defaults from API fields; (10) `loadLayout()` initialization sequence with chained Promises; (11) ES5 constraint table with full forbidden/allowed comparison; (12) Antipattern prevention table (9 entries); (13) AC expanded from 9 to 10 with explicit mobile smoke test separation. Tasks expanded from 9 to 10 with Task 10 as explicit smoke test capture. Status changed from draft to ready-for-dev. | BMAD Story Synthesis |
| 2026-04-17 | 2.0 | Implementation complete. Created `editor.html` (1.2 KB), `editor.css` (2.7 KB), `editor.js` (9.0 KB ES5 IIFE). Added 3 GET routes to `WebPortal.cpp`. Gzip total = 5,063 bytes (24.7% of 20 KB budget). `pio run -e esp32dev` SUCCESS at 82.8% flash. ES5 grep clean. AC 1–9 verified; AC 10 (mobile smoke test) deferred to on-device session. Status → done. | Dev Agent |

---

## Dev Agent Record

### Implementation Plan

Implemented in strict task order per the story:
1. `editor.html` — page scaffold with explicit body structure from Task 1.
2. `editor.css` — pixelated canvas, `touch-action:none`, snap-button active state, mobile responsive collapse.
3. `editor.js` — single ES5 IIFE; module globals at top; `render()` → `drawGrid()` + `drawWidget()`; `initCanvas()`; pointer events with `setPointerCapture`; toolbox driven by `/api/widgets/types`; snap control with `localStorage` persistence under `fw_editor_snap`.
4. `WebPortal.cpp` — three GET routes added immediately after the `health.js` route, using `[]` capture (no `this`) since `_serveGzAsset` is static.
5. Gzipped via `gzip -9 -c data-src/X > data/X.gz` from `firmware/`.
6. Build + ES5 grep verified.

### Key Decisions

- **`SCALE = 4` is fixed**, not multiplied by `devicePixelRatio` — keeps the canvas legible on phones without becoming oversized on retina displays (per Dev Notes).
- **Hit-test priority**: resize handle of the *currently selected* widget is checked first; then widget bodies in topmost-first order; otherwise miss → deselect. This makes the 6-px handle reachable without it being shadowed by the body hit.
- **`pointercancel` mapped to `onPointerUp`** to avoid stuck drag states on iOS Safari (per Dev Notes).
- **Min-floor toast fires once per gesture** via `dragState.toastedFloor` flag to prevent toast spam during a sustained resize that holds at the floor.
- **`buildWidgetTypeMeta`** uses bracket access `f['default']` because `default` is an ES5 reserved-word property name access pattern (still allowed via dot, but bracket is safer for legacy parsers and matches the story's Dev Notes example).
- **Snap value validation** restricted to the legitimate set `{1, 8, 16}` when reading both `localStorage` and click handlers — guards against tampered storage.
- **Both source and `.gz` are committed**, per project convention; sources are the truth, `.gz` are derived but version-tracked deployment artifacts.
- **No changes to `common.js`** — its `FW.get/showToast` API was already sufficient; touching it would bloat scope.

### Completion Notes

| AC | Status | Verification |
|---|---|---|
| 1. `/editor.html` served gzip-encoded with correct dimensions from `/api/layout` at 4× scale; `imageSmoothingEnabled = false` | ✅ | Route added in `WebPortal.cpp` lines 905–914; `initCanvas()` reads `d.matrix.width`/`height` and `d.hardware.tile_pixels`; `render()` sets `imageSmoothingEnabled = false` on every redraw. |
| 2. Ghosted tile grid using `tile_pixels` boundaries | ✅ | `drawGrid()` strokes `rgba(255,255,255,0.1)` lines every `tilePixels * SCALE` CSS px; called inside `render()`. |
| 3. Toolbox from `/api/widgets/types`; `note` containing "not yet implemented" → disabled | ✅ | `initToolbox()` calls `isDisabledEntry()` and adds `disabled` class; `onToolboxClick()` short-circuits on disabled items. |
| 4. Drag updates x/y by delta ÷ SCALE; canvas re-renders on every move; `touch-action: none` set | ✅ | `onPointerMove()` divides `dxCss/SCALE`; `render()` called every move; CSS sets `touch-action: none`; `e.preventDefault()` belt-and-braces in `onPointerDown`. |
| 5. 6×6 corner resize handle resizes; canvas re-renders | ✅ | `HANDLE_SIZE = 6`; `hitTest` returns `mode='resize'` when within handle bounds; `onPointerMove()` computes `newW`/`newH` and rerenders. |
| 6. Snap modes 8/16/1 with localStorage persistence under `fw_editor_snap` | ✅ | `initSnap()` reads `localStorage.getItem('fw_editor_snap')`, validates ∈ {1,8,16}, defaults to 8; click handler writes `setItem` and updates active class. |
| 7. Min-floor clamp + `FW.showToast` once per gesture | ✅ | Resize branch in `onPointerMove()` clamps via `clamp(rawW, meta.minW, maxW)`; toasts only when `rawW<minW || rawH<minH` and `!toastedFloor`, then sets the flag. |
| 8. ≤ 20 KB total gzip | ✅ | `wc -c` total = **5,063 bytes** (569 + 3,582 + 912). 24.7% of budget. |
| 9. ES5 audit grep returns empty | ✅ | `grep -nE '=>|\`|(^\|[^a-zA-Z_])let \|(^\|[^a-zA-Z_])const ' firmware/data-src/editor.js` → no matches. |
| 10. Mobile smoke test (manual) | ⏭ | Deferred — manual on-device test required after firmware flash + uploadfs. Captured below in Mobile Smoke Test section. |

**Build verification**:
- `pio run -e esp32dev`: SUCCESS — Flash: 1,302,672 / 1,572,864 bytes (82.8% — within the 1,382,400 gate).
- `pio run -e esp32dev -t buildfs`: SUCCESS — LittleFS image includes `/editor.html.gz`, `/editor.js.gz`, `/editor.css.gz` alongside existing assets.

### Files Modified

- `firmware/adapters/WebPortal.cpp` — added 3 GET route registrations at the end of `WebPortal::begin()` (lines 905–914) for `/editor.html`, `/editor.js`, `/editor.css`. No other lines changed.
- `_bmad-output/implementation-artifacts/sprint-status.yaml` — `le-1-6-editor-canvas-and-drag-drop` flipped from `in-progress` to `done`.

### Files Created

- `firmware/data-src/editor.html` (1,234 bytes source)
- `firmware/data-src/editor.css` (~2.7 KB source)
- `firmware/data-src/editor.js` (~9.0 KB source)
- `firmware/data/editor.html.gz` (569 bytes)
- `firmware/data/editor.js.gz` (3,582 bytes)
- `firmware/data/editor.css.gz` (912 bytes)

### Files Verified Unchanged

- `firmware/data-src/common.js` — unchanged; `FW.get/post/del/showToast` were already sufficient.
- `firmware/data-src/dashboard.html`, `dashboard.js`, `wizard.html`, `wizard.js`, `health.html`, `health.js`, `style.css` — unchanged.
- `firmware/core/WidgetRegistry.h`, `firmware/core/LayoutStore.*`, `firmware/modes/CustomLayoutMode.cpp` — unchanged.
- All `firmware/*.cpp`/`.h` other than `firmware/adapters/WebPortal.cpp` — unchanged.

### Mobile Smoke Test (AC #10)

⏭ **Pending hardware**: Cannot be executed in this implementation session — requires physical ESP32 device with firmware flashed and `uploadfs` completed, then access via iPhone Safari (iOS 15+) and Android Chrome (110+).

Test script for the operator to run after flashing:
1. `pio run -e esp32dev -t upload && pio run -e esp32dev -t uploadfs`
2. Wait for device boot, navigate phone browser to `http://flightwall.local/editor.html`.
3. Verify canvas renders with the correct device dimensions and the ghosted tile grid is visible.
4. Tap a non-disabled toolbox item (e.g. "Text") → verify a widget appears at (8, 8).
5. Drag the widget with one finger; confirm the page does not scroll and the widget follows the finger.
6. Tap a corner resize handle and drag; confirm the widget resizes; drag toward zero to verify the floor toast fires (once).
7. Tap "Tidy" snap mode, drag again; confirm 16-px snap. Reload the page; confirm "Tidy" remains active (localStorage persistence).
8. Repeat steps 4–7 on Android Chrome.

Document results in this section after on-device verification.

### Sprint Status

`_bmad-output/implementation-artifacts/sprint-status.yaml` updated: `le-1-6-editor-canvas-and-drag-drop: done`.

---

## Senior Developer Review (AI)

### Review: 2026-04-17
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 3.6 (weighted across 3 reviewers; majority findings dismissed as false positives) → **Approved with Reservations**
- **Issues Found:** 3 verified (1 medium, 2 low)
- **Issues Fixed:** 2 (releasePointerCapture in editor.js; editor smoke tests added to test_web_portal_smoke.py)
- **Action Items Created:** 1

#### Review Follow-ups (AI)

- [ ] [AI-Review] LOW: AC10 (mobile smoke test) must be executed on real hardware (iPhone Safari iOS 15+ and Android Chrome 110+) with results documented in Dev Agent Record. Story status should remain `done` only after this test passes. (`_bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md` Dev Agent Record → Mobile Smoke Test section)

### Review: 2026-04-17 (Round 2 — 3-Reviewer Synthesis)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 3.1 (weighted across 3 reviewers; 14 of 18 distinct findings dismissed as false positives or prior-round-fixed) → **Approved with Reservations**
- **Issues Found:** 4 verified (1 medium, 3 low)
- **Issues Fixed:** 4 (API schema guard in loadLayout; array guard for widget types; releasePointerCapture ordering; addWidget dimension clamp)
- **Action Items Created:** 1

#### Review Follow-ups (AI)

- [ ] [AI-Review] LOW: AC10 (mobile smoke test) must be executed on real hardware (iPhone Safari iOS 15+ and Android Chrome 110+) with results documented in Dev Agent Record. Story cannot be considered fully closed until this is completed. (`_bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md` Dev Agent Record → Mobile Smoke Test section)

### Review: 2026-04-17 (Round 3 — 3-Reviewer Synthesis)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 2.1 (weighted across 3 reviewers; 17 of 19 distinct findings dismissed as false positives or already-fixed by prior rounds) → **Approved with Reservations**
- **Issues Found:** 1 verified (1 low)
- **Issues Fixed:** 1 (added `.catch()` handler to `loadLayout()` promise chain in `editor.js`)
- **Action Items Created:** 1

#### Review Follow-ups (AI)

- [ ] [AI-Review] LOW: AC10 (mobile smoke test) must be executed on real hardware (iPhone Safari iOS 15+ and Android Chrome 110+) with results documented in Dev Agent Record. Story cannot be considered fully closed until this is completed. (`_bmad-output/implementation-artifacts/stories/le-1-6-editor-canvas-and-drag-drop.md` Dev Agent Record → Mobile Smoke Test section)


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
#include "core/OTAUpdater.h"
#include "utils/Log.h"

// Defined in main.cpp — provides thread-safe flight stats for the health page
extern FlightStatsSnapshot getFlightStatsSnapshot();

// Defined in main.cpp — NTP sync status accessor (Story fn-2.1)
extern bool isNtpSynced();

// Defined in main.cpp — Night mode scheduler dimming state (Story fn-2.2)
extern bool isScheduleDimming();

#include "core/LayoutEngine.h"
#include "core/LayoutStore.h"
#include "core/ModeOrchestrator.h"
#include "core/ModeRegistry.h"

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

// Shared SwitchState → string mapping used by GET and POST mode handlers.
// Centralised here to avoid copy-paste drift between the two call sites.
const char* switchStateToString(SwitchState ss) {
    switch (ss) {
        case SwitchState::REQUESTED: return "requested";
        case SwitchState::SWITCHING: return "switching";
        default:                     return "idle";
    }
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
                // push_back first, then reserve on the stored element — avoids capacity
                // loss from the String copy constructor during push_back (synthesis ds-3.2 Pass 3).
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }

            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }

            pending->body.concat(reinterpret_cast<const char*>(data), len);

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

    // POST /api/display/mode (Story ds-3.1) — manual mode switch
    // AC #5: accept "mode" with fallback to "mode_id" for backward compat
    // AC #6: orchestrator-only path (Rule 24)
    // AC #7: async response with switch_state — no bounded wait in async handler
    //   Strategy: return switch_state: "requested" with client re-fetch (ds-3.4 UX).
    //   Architecture D5 "synchronous" intent is not feasible in ESPAsyncWebServer body
    //   handler because tick() runs on Core 0 display task. Explicit deviation documented.
    _server->on("/api/display/mode", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (total == 0 || data == nullptr) {
                _sendJsonError(request, 400, "Empty request body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                // push_back first, then reserve on the stored element — avoids capacity
                // loss from the String copy constructor during push_back (synthesis ds-3.2 Pass 3).
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePostDisplayMode(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // GET /api/schedule (Story dl-4.2) — mode schedule rules
    _server->on("/api/schedule", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetSchedule(request);
    });

    // POST /api/schedule (Story dl-4.2) — replace mode schedule rules
    _server->on("/api/schedule", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (total == 0 || data == nullptr) {
                _sendJsonError(request, 400, "Empty request body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                // push_back first, then reserve on the stored element — avoids capacity
                // loss from the String copy constructor during push_back (synthesis ds-3.2 Pass 3).
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePostSchedule(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // GET /api/ota/check (Story dl-6.2) — check GitHub for firmware update
    _server->on("/api/ota/check", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetOtaCheck(request);
    });

    // POST /api/ota/pull (Story dl-7.3, AC #1–#2) — start pull OTA download
    _server->on("/api/ota/pull", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostOtaPull(request);
    });

    // GET /api/ota/status (Story dl-7.3, AC #1, #3) — OTA state/progress polling
    _server->on("/api/ota/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetOtaStatus(request);
    });

    // POST /api/display/notification/dismiss (Story ds-3.1, AC #10; ds-3.2, AC #4)
    // Clears upgrade_notification NVS flag so GET returns false thereafter.
    // Uses ConfigManager::setUpgNotif() to keep NVS writes centralized (AR7).
    _server->on("/api/display/notification/dismiss", HTTP_POST, [](AsyncWebServerRequest* request) {
        ConfigManager::setUpgNotif(false);

        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
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

                // Story dl-7.1, AC #9: tear down display mode before flash write
                // so push and pull OTA share the same safe teardown path.
                if (!ModeRegistry::prepareForOTA()) {
                    // prepareForOTA failed — abort upload with clear error
                    state.valid = false;
                    state.error = "Could not prepare display for OTA";
                    state.errorCode = "PREPARE_OTA_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Display teardown failed");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "prepareForOTA() failed — aborting upload");
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

    // ─── Layout CRUD (LE-1.4) ───
    // Register specific paths before wildcards. POST /api/layouts/*/activate
    // is a distinct path from POST /api/layouts (exact) and POST /api/layouts/*
    // would overlap with activate — but ESPAsyncWebServer matches the longer
    // activate path first. Register activate ahead of the generic PUT/DELETE
    // wildcards for safety.
    _server->on("/api/layouts", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLayouts(request);
    });

    _server->on("/api/layouts", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (data == nullptr || total == 0) {
                clearPendingBody(request);
                _sendJsonError(request, 400, "Empty body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > LAYOUT_STORE_MAX_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() { clearPendingBody(request); });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePostLayout(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // Register activate BEFORE generic wildcards so longer/more specific
    // path matches first on the mathieucarbou fork.
    _server->on("/api/layouts/*/activate", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostLayoutActivate(request);
    });

    _server->on("/api/layouts/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLayoutById(request);
    });

    _server->on("/api/layouts/*", HTTP_PUT,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (data == nullptr || total == 0) {
                clearPendingBody(request);
                _sendJsonError(request, 400, "Empty body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > LAYOUT_STORE_MAX_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() { clearPendingBody(request); });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePutLayout(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    _server->on("/api/layouts/*", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        _handleDeleteLayout(request);
    });

    _server->on("/api/widgets/types", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetWidgetTypes(request);
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
    // Layout Editor assets (LE-1.6)
    _server->on("/editor.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.html.gz", "text/html");
    });
    _server->on("/editor.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.js.gz", "application/javascript");
    });
    _server->on("/editor.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.css.gz", "text/css");
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

    // NTP and schedule status (Story fn-2.1, fn-2.2)
    data["ntp_synced"] = isNtpSynced();
    data["schedule_active"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();
    data["schedule_dimming"] = isScheduleDimming();

    // Rollback acknowledgment state (Story fn-1.6) — NVS-backed, not via ConfigManager
    Preferences prefs;
    prefs.begin("flightwall", true);  // read-only
    data["rollback_acknowledged"] = prefs.getUChar("ota_rb_ack", 0) == 1;
    prefs.end();

    // OTA availability from last check (Story dl-6.2, AC #3)
    bool otaAvail = (OTAUpdater::getState() == OTAState::AVAILABLE);
    data["ota_available"] = otaAvail;
    data["ota_version"] = otaAvail ? (const char*)OTAUpdater::getRemoteVersion()
                                   : (const char*)nullptr;

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
    // Capacity: ~256 base + ~200 per mode (with zone_layout) — 3 modes ≈ 900 bytes
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // Build modes array from ModeRegistry (AC #1, #2, #4 — no heap allocation per request)
    JsonArray modes = data["modes"].to<JsonArray>();
    const ModeEntry* table = ModeRegistry::getModeTable();
    const uint8_t count = ModeRegistry::getModeCount();

    // Active mode from ModeRegistry (truth source); fall back to "classic_card" if nullptr
    const char* registryActiveId = ModeRegistry::getActiveModeId();
    const char* activeId = registryActiveId ? registryActiveId : "classic_card";

    for (uint8_t i = 0; i < count; i++) {
        const ModeEntry& entry = table[i];
        JsonObject modeObj = modes.add<JsonObject>();
        modeObj["id"] = entry.id;
        modeObj["name"] = entry.displayName;
        modeObj["active"] = (strcmp(activeId, entry.id) == 0);

        // Zone layout from static descriptor (AC #2) — walks BSS/static data only
        JsonArray zoneLayout = modeObj["zone_layout"].to<JsonArray>();
        if (entry.zoneDescriptor != nullptr) {
            if (entry.zoneDescriptor->description != nullptr) {
                modeObj["description"] = entry.zoneDescriptor->description;
            }
            for (uint8_t z = 0; z < entry.zoneDescriptor->regionCount; z++) {
                const ZoneRegion& region = entry.zoneDescriptor->regions[z];
                JsonObject zoneObj = zoneLayout.add<JsonObject>();
                zoneObj["label"] = region.label;
                zoneObj["relX"] = region.relX;
                zoneObj["relY"] = region.relY;
                zoneObj["relW"] = region.relW;
                zoneObj["relH"] = region.relH;
            }
        }

        // Per-mode settings from settingsSchema (Story dl-5.1, AC #1–#2)
        // Loops ModeEntry.settingsSchema only — no hardcoded key lists (rule 28).
        // Adding a ModeSettingDef to any schema auto-appears here.
        if (entry.settingsSchema != nullptr && entry.settingsSchema->settingCount > 0) {
            JsonArray settings = modeObj["settings"].to<JsonArray>();
            const ModeSettingsSchema* schema = entry.settingsSchema;
            for (uint8_t s = 0; s < schema->settingCount; s++) {
                const ModeSettingDef& def = schema->settings[s];
                JsonObject settingObj = settings.add<JsonObject>();
                settingObj["key"] = def.key;
                settingObj["label"] = def.label;
                settingObj["type"] = def.type;
                settingObj["default"] = def.defaultValue;
                settingObj["min"] = def.minValue;
                settingObj["max"] = def.maxValue;
                if (def.enumOptions != nullptr) {
                    settingObj["enumOptions"] = def.enumOptions;
                } else {
                    settingObj["enumOptions"] = (const char*)nullptr;
                }
                // Current persisted value via ConfigManager (uses modeAbbrev for NVS key)
                settingObj["value"] = ConfigManager::getModeSetting(
                    schema->modeAbbrev, def.key, def.defaultValue);
            }
        } else {
            modeObj["settings"] = (const char*)nullptr;  // null for modes without settings
        }
    }

    data["active"] = activeId;

    // Switch state from ModeRegistry (AC #1, replacing stub "idle")
    data["switch_state"] = switchStateToString(ModeRegistry::getSwitchState());

    // Orchestrator transparency (AC #3 — retain dl-1.5 fields)
    data["orchestrator_state"] = ModeOrchestrator::getStateString();
    data["state_reason"] = ModeOrchestrator::getStateReason();

    // Registry error if present (AC #9)
    // Use copyLastError() — thread-safe snapshot for Core 1 (synthesis ds-3.2 Pass 3).
    // getLastError() returns a raw pointer to a Core 0 mutable buffer; unsafe here.
    char regErrBuf[64];
    ModeRegistry::copyLastError(regErrBuf, sizeof(regErrBuf));
    if (regErrBuf[0] != '\0') {
        data["registry_error"] = regErrBuf;
    }

    // Upgrade notification NVS flag (AC #10; ds-3.2, AC #4)
    // Uses ConfigManager::getUpgNotif() to keep NVS reads centralized (AR7).
    data["upgrade_notification"] = ConfigManager::getUpgNotif();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

// POST /api/display/mode (Story ds-3.1)
// AC #5: accept "mode" with fallback to "mode_id" for one release.
// AC #6 / Rule 24: always route user intent through orchestrator — even when re-selecting
//   the current mode — so the orchestrator exits IDLE_FALLBACK / SCHEDULED and returns
//   to MANUAL state as the user explicitly intended.
//   ModeRegistry::tick() is idempotent when requested == active (line 367 guard), so
//   calling onManualSwitch for the same mode does not restart the running DisplayMode.
// AC #7: async strategy (documented deviation from architecture D5 — bounded wait not
//   feasible in ESPAsyncWebServer body handler; client re-fetches via ds-3.4 UX).
void WebPortal::_handlePostDisplayMode(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument reqDoc;
    DeserializationError err = deserializeJson(reqDoc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "INVALID_JSON");
        return;
    }

    // AC #5: prefer "mode", fall back to "mode_id" for one release
    const char* modeId = reqDoc["mode"] | (const char*)nullptr;
    if (!modeId) {
        modeId = reqDoc["mode_id"] | (const char*)nullptr;
    }
    if (!modeId) {
        _sendJsonError(request, 400, "Missing mode or mode_id field", "MISSING_FIELD");
        return;
    }

    // Resolve mode from ModeRegistry (AC #8 — unknown mode → 400)
    const ModeEntry* table = ModeRegistry::getModeTable();
    const uint8_t count = ModeRegistry::getModeCount();
    const ModeEntry* matchedEntry = nullptr;
    for (uint8_t i = 0; i < count; i++) {
        if (strcmp(table[i].id, modeId) == 0) {
            matchedEntry = &table[i];
            break;
        }
    }
    if (!matchedEntry) {
        _sendJsonError(request, 400, "Unknown mode_id", "UNKNOWN_MODE");
        return;
    }

    // Story dl-5.1, AC #4: handle optional "settings" object
    bool hasSettings = reqDoc["settings"].is<JsonObject>();
    bool settingsApplied = false;
    if (hasSettings) {
        JsonObject settingsObj = reqDoc["settings"].as<JsonObject>();

        // Validate all keys exist in schema before applying any (no partial apply)
        if (matchedEntry->settingsSchema == nullptr || matchedEntry->settingsSchema->settingCount == 0) {
            if (settingsObj.size() > 0) {
                _sendJsonError(request, 400, "Mode has no configurable settings", "UNKNOWN_SETTING");
                return;
            }
        } else {
            const ModeSettingsSchema* schema = matchedEntry->settingsSchema;
            // Pre-validate: check all keys exist in schema
            for (JsonPair kv : settingsObj) {
                bool found = false;
                for (uint8_t s = 0; s < schema->settingCount; s++) {
                    if (strcmp(kv.key().c_str(), schema->settings[s].key) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    _sendJsonError(request, 400, "Unknown setting key", "UNKNOWN_SETTING");
                    return;
                }
            }
            // Pre-validate values against schema min/max BEFORE writing any setting
            // to NVS — prevents partial apply where early keys succeed but a later
            // key fails, leaving NVS in an inconsistent state (no partial apply rule).
            // Also reject non-numeric values: as<int32_t>() silently returns 0 for strings,
            // which could pass range checks (e.g. format=0 is valid). is<int32_t>() guards this.
            for (JsonPair kv : settingsObj) {
                if (!kv.value().is<int32_t>()) {
                    _sendJsonError(request, 400, "Setting value must be numeric", "INVALID_SETTING_TYPE");
                    return;
                }
                int32_t val = kv.value().as<int32_t>();
                for (uint8_t s = 0; s < schema->settingCount; s++) {
                    if (strcmp(kv.key().c_str(), schema->settings[s].key) == 0) {
                        if (val < schema->settings[s].minValue || val > schema->settings[s].maxValue) {
                            _sendJsonError(request, 400, "Setting value out of range", "INVALID_SETTING_VALUE");
                            return;
                        }
                        break;
                    }
                }
            }
            // Apply all settings (all values pre-validated — no partial apply risk)
            for (JsonPair kv : settingsObj) {
                int32_t val = kv.value().as<int32_t>();
                if (!ConfigManager::setModeSetting(schema->modeAbbrev, kv.key().c_str(), val)) {
                    _sendJsonError(request, 400, "Setting validation failed", "INVALID_SETTING_VALUE");
                    return;
                }
            }
            if (settingsObj.size() > 0) settingsApplied = true;
        }
    }

    // AC #6 / Rule 24: always call onManualSwitch so the orchestrator transitions to
    // MANUAL state regardless of whether the requested mode is already active.
    ModeOrchestrator::onManualSwitch(modeId, matchedEntry->displayName);

    // Build truthful response (AC #7, #9)
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject respData = root["data"].to<JsonObject>();
    respData["switching_to"] = modeId;
    respData["active"] = ModeRegistry::getActiveModeId()
                           ? ModeRegistry::getActiveModeId() : "classic_card";
    respData["settings_applied"] = settingsApplied;
    respData["switch_state"] = switchStateToString(ModeRegistry::getSwitchState());
    respData["orchestrator_state"] = ModeOrchestrator::getStateString();
    respData["state_reason"] = ModeOrchestrator::getStateReason();

    // Registry error if present (AC #9 — HEAP_INSUFFICIENT etc.)
    // Use copyLastError() — thread-safe snapshot for Core 1 (synthesis ds-3.2 Pass 3).
    char regErrBuf2[64];
    ModeRegistry::copyLastError(regErrBuf2, sizeof(regErrBuf2));
    if (regErrBuf2[0] != '\0') {
        respData["registry_error"] = regErrBuf2;
    }

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

void WebPortal::_handleGetOtaCheck(AsyncWebServerRequest* request) {
    // AC #2: if WiFi not connected, return error rather than hitting GitHub
    if (WiFi.status() != WL_CONNECTED) {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        JsonObject data = root["data"].to<JsonObject>();
        data["available"] = false;
        data["version"] = (const char*)nullptr;
        data["current_version"] = FW_VERSION;
        data["release_notes"] = (const char*)nullptr;
        data["error"] = "WiFi not connected";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    // Perform the check — this makes an HTTPS request to GitHub (blocks ~1-10s)
    bool updateAvailable = OTAUpdater::checkForUpdate();

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    if (OTAUpdater::getState() == OTAState::AVAILABLE) {
        data["available"] = true;
        data["version"] = OTAUpdater::getRemoteVersion();
        data["current_version"] = FW_VERSION;
        data["release_notes"] = OTAUpdater::getReleaseNotes();
    } else if (OTAUpdater::getState() == OTAState::ERROR) {
        // Rate limit / network / parse failure — include error message
        data["available"] = false;
        data["version"] = (const char*)nullptr;
        data["current_version"] = FW_VERSION;
        data["release_notes"] = (const char*)nullptr;
        data["error"] = OTAUpdater::getLastError();
    } else {
        // Up to date (IDLE state after successful check)
        data["available"] = false;
        data["version"] = (const char*)nullptr;
        data["current_version"] = FW_VERSION;
        data["release_notes"] = (const char*)nullptr;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostOtaPull(AsyncWebServerRequest* request) {
    OTAState state = OTAUpdater::getState();

    // Guard: already downloading/verifying (AC #2)
    if (state == OTAState::DOWNLOADING || state == OTAState::VERIFYING) {
        _sendJsonError(request, 409, "Download already in progress", "OTA_BUSY");
        return;
    }

    // Guard: no update available (AC #2)
    if (state != OTAState::AVAILABLE) {
        _sendJsonError(request, 400, "No update available \u2014 check for updates first", "NO_UPDATE");
        return;
    }

    // Start the download (AC #2)
    bool started = OTAUpdater::startDownload();

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    if (started) {
        root["ok"] = true;
        JsonObject data = root["data"].to<JsonObject>();
        data["started"] = true;
    } else {
        root["ok"] = false;
        root["error"] = OTAUpdater::getLastError();
        root["code"] = "OTA_START_FAILED";
    }

    String output;
    serializeJson(doc, output);
    request->send(started ? 200 : 500, "application/json", output);
}

void WebPortal::_handleGetOtaStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // AC #3: stable contract — lowercase state string
    data["state"] = OTAUpdater::getStateString();

    // progress: required for downloading, null otherwise
    OTAState state = OTAUpdater::getState();
    if (state == OTAState::DOWNLOADING) {
        data["progress"] = OTAUpdater::getProgress();
    } else if (state == OTAState::VERIFYING) {
        data["progress"] = (const char*)nullptr;  // indeterminate
    } else {
        data["progress"] = (const char*)nullptr;
    }

    // error: non-null only when state == error
    if (state == OTAState::ERROR) {
        data["error"] = OTAUpdater::getLastError();
    } else {
        data["error"] = (const char*)nullptr;
    }

    // Phase from dl-7.2 structured failure
    data["failure_phase"] = OTAUpdater::getFailurePhaseString();
    data["retriable"] = OTAUpdater::isRetriable();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetSchedule(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // Build rules array from ConfigManager (AC #1)
    ModeScheduleConfig sched = ConfigManager::getModeSchedule();
    JsonArray rules = data["rules"].to<JsonArray>();
    for (uint8_t i = 0; i < sched.ruleCount; i++) {
        const ScheduleRule& r = sched.rules[i];
        JsonObject ruleObj = rules.add<JsonObject>();
        ruleObj["index"] = i;
        ruleObj["start_min"] = r.startMin;
        ruleObj["end_min"] = r.endMin;
        ruleObj["mode_id"] = r.modeId;
        ruleObj["enabled"] = r.enabled;
    }

    // Orchestrator state and active rule index (AC #1)
    data["orchestrator_state"] = ModeOrchestrator::getStateString();
    data["active_rule_index"] = ModeOrchestrator::getActiveScheduleRuleIndex();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostSchedule(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument reqDoc;
    DeserializationError err = deserializeJson(reqDoc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "INVALID_JSON");
        return;
    }

    if (!reqDoc["rules"].is<JsonArray>()) {
        _sendJsonError(request, 400, "Missing or invalid 'rules' array", "INVALID_SCHEDULE");
        return;
    }

    JsonArray rulesArr = reqDoc["rules"].as<JsonArray>();
    if (rulesArr.size() > MAX_SCHEDULE_RULES) {
        _sendJsonError(request, 400, "Too many rules (max 8)", "INVALID_SCHEDULE");
        return;
    }

    // Build ModeScheduleConfig from JSON (AC #2–#5)
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = rulesArr.size();

    // Build mode ID lookup table for validation
    const ModeEntry* modeTable = ModeRegistry::getModeTable();
    const uint8_t modeCount = ModeRegistry::getModeCount();

    for (size_t i = 0; i < rulesArr.size(); i++) {
        JsonObject ruleObj = rulesArr[i];

        // Validate required fields
        if (!ruleObj["start_min"].is<int>() || !ruleObj["end_min"].is<int>() ||
            !ruleObj["mode_id"].is<const char*>() || !ruleObj["enabled"].is<int>()) {
            _sendJsonError(request, 400, "Rule missing required fields", "INVALID_SCHEDULE");
            return;
        }

        int startMin = ruleObj["start_min"].as<int>();
        int endMin = ruleObj["end_min"].as<int>();
        const char* modeId = ruleObj["mode_id"].as<const char*>();
        int enabled = ruleObj["enabled"].as<int>();

        // Range validation (AC #3)
        if (startMin < 0 || startMin > 1439 || endMin < 0 || endMin > 1439) {
            _sendJsonError(request, 400, "start_min/end_min out of range (0-1439)", "INVALID_SCHEDULE");
            return;
        }

        if (enabled < 0 || enabled > 1) {
            _sendJsonError(request, 400, "enabled must be 0 or 1", "INVALID_SCHEDULE");
            return;
        }

        if (!modeId || strlen(modeId) == 0 || strlen(modeId) >= MODE_ID_BUF_LEN) {
            _sendJsonError(request, 400, "Invalid mode_id", "INVALID_SCHEDULE");
            return;
        }

        // Validate mode_id exists in ModeRegistry (AC #3)
        bool modeFound = false;
        for (uint8_t m = 0; m < modeCount; m++) {
            if (strcmp(modeTable[m].id, modeId) == 0) {
                modeFound = true;
                break;
            }
        }
        if (!modeFound) {
            _sendJsonError(request, 400, "Unknown mode_id", "UNKNOWN_MODE");
            return;
        }

        cfg.rules[i].startMin = (uint16_t)startMin;
        cfg.rules[i].endMin = (uint16_t)endMin;
        strncpy(cfg.rules[i].modeId, modeId, MODE_ID_BUF_LEN - 1);
        cfg.rules[i].modeId[MODE_ID_BUF_LEN - 1] = '\0';
        cfg.rules[i].enabled = (uint8_t)enabled;
    }

    // Persist via ConfigManager (AC #2) — orchestrator tick picks up on next cycle (Rule 24)
    if (!ConfigManager::setModeSchedule(cfg)) {
        _sendJsonError(request, 500, "Failed to save schedule", "NVS_ERROR");
        return;
    }

    JsonDocument respDoc;
    JsonObject resp = respDoc.to<JsonObject>();
    resp["ok"] = true;
    JsonObject respData = resp["data"].to<JsonObject>();
    respData["applied"] = true;

    String output;
    serializeJson(respDoc, output);
    request->send(200, "application/json", output);
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

// ──────────────────────────────────────────────────────────────────────────
// Layout CRUD + activation (Story LE-1.4)
// ──────────────────────────────────────────────────────────────────────────

namespace {

// Shared helpers for layout handlers — keep path parsing consistent.

// Extract a layout id segment from a URL of the form /api/layouts/<id>
// (or /api/layouts/<id>/activate when activateSegment = true).
// Strips any query string. Returns empty String on failure.
String extractLayoutIdFromUrl(const String& url, bool activateSegment) {
    if (activateSegment) {
        const char* prefix = "/api/layouts/";
        const char* suffix = "/activate";
        size_t prefixLen = strlen(prefix);
        size_t suffixLen = strlen(suffix);
        if (!url.startsWith(prefix) || !url.endsWith(suffix)) {
            return String();
        }
        if (url.length() < prefixLen + suffixLen + 1) return String();
        return url.substring(prefixLen, url.length() - suffixLen);
    }
    // Simple "/api/layouts/<id>" path: take everything after the final '/'.
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0 || lastSlash == (int)(url.length() - 1)) return String();
    String id = url.substring(lastSlash + 1);
    int queryStart = id.indexOf('?');
    if (queryStart >= 0) id = id.substring(0, queryStart);
    return id;
}

} // namespace

void WebPortal::_handleGetLayouts(AsyncWebServerRequest* request) {
    std::vector<LayoutEntry> entries;
    if (!LayoutStore::list(entries)) {
        _sendJsonError(request, 500, "Layout storage unavailable", "STORAGE_UNAVAILABLE");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    data["active_id"] = LayoutStore::getActiveId();
    JsonArray arr = data["layouts"].to<JsonArray>();
    for (const auto& e : entries) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = e.id;
        obj["name"] = e.name;
        obj["bytes"] = e.sizeBytes;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLayoutById(AsyncWebServerRequest* request) {
    String id = extractLayoutIdFromUrl(request->url(), false);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Missing layout id", "MISSING_ID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    String body;
    if (!LayoutStore::load(id.c_str(), body)) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }
    // Parse body so it's embedded as a JSON object (not a double-encoded
    // string) under data.
    JsonDocument doc;
    JsonDocument layoutDoc;
    DeserializationError err = deserializeJson(layoutDoc, body);
    if (err) {
        _sendJsonError(request, 500, "Stored layout is corrupt", "LAYOUT_CORRUPT");
        return;
    }
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["data"] = layoutDoc.as<JsonVariant>();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostLayout(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "LAYOUT_INVALID");
        return;
    }
    if (!doc.is<JsonObject>()) {
        _sendJsonError(request, 400, "Expected JSON object", "LAYOUT_INVALID");
        return;
    }
    const char* id = doc["id"] | (const char*)nullptr;
    if (id == nullptr || *id == '\0') {
        _sendJsonError(request, 400, "Missing id in layout body", "LAYOUT_INVALID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id)) {
        _sendJsonError(request, 400, "Invalid layout id", "LAYOUT_INVALID");
        return;
    }
    String jsonStr;
    serializeJson(doc, jsonStr);

    LayoutStoreError result = LayoutStore::save(id, jsonStr.c_str());
    switch (result) {
        case LayoutStoreError::OK: {
            JsonDocument respDoc;
            JsonObject root = respDoc.to<JsonObject>();
            root["ok"] = true;
            JsonObject respData = root["data"].to<JsonObject>();
            respData["id"] = id;
            respData["bytes"] = jsonStr.length();
            String output;
            serializeJson(respDoc, output);
            request->send(200, "application/json", output);
            return;
        }
        case LayoutStoreError::TOO_LARGE:
            _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
            return;
        case LayoutStoreError::INVALID_SCHEMA:
            _sendJsonError(request, 400, "Layout schema invalid", "LAYOUT_INVALID");
            return;
        case LayoutStoreError::FS_FULL:
            _sendJsonError(request, 507, "Filesystem full", "FS_FULL");
            return;
        case LayoutStoreError::IO_ERROR:
        default:
            _sendJsonError(request, 500, "Layout IO error", "IO_ERROR");
            return;
    }
}

void WebPortal::_handlePutLayout(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    String id = extractLayoutIdFromUrl(request->url(), false);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Missing layout id", "MISSING_ID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    // Must exist already — PUT is overwrite-only. Caller should use POST to
    // create. Check via load() (fills a default on miss; use return value).
    String existing;
    if (!LayoutStore::load(id.c_str(), existing)) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "LAYOUT_INVALID");
        return;
    }
    if (!doc.is<JsonObject>()) {
        _sendJsonError(request, 400, "Expected JSON object", "LAYOUT_INVALID");
        return;
    }
    const char* docId = doc["id"] | (const char*)nullptr;
    if (docId == nullptr) {
        _sendJsonError(request, 400, "Missing id field in layout body", "LAYOUT_INVALID");
        return;
    }
    if (strcmp(docId, id.c_str()) != 0) {
        _sendJsonError(request, 400, "Body id does not match URL id", "LAYOUT_INVALID");
        return;
    }
    String jsonStr;
    serializeJson(doc, jsonStr);

    LayoutStoreError result = LayoutStore::save(id.c_str(), jsonStr.c_str());
    switch (result) {
        case LayoutStoreError::OK: {
            JsonDocument respDoc;
            JsonObject root = respDoc.to<JsonObject>();
            root["ok"] = true;
            JsonObject respData = root["data"].to<JsonObject>();
            respData["id"] = id;
            respData["bytes"] = jsonStr.length();
            String output;
            serializeJson(respDoc, output);
            request->send(200, "application/json", output);
            return;
        }
        case LayoutStoreError::TOO_LARGE:
            _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
            return;
        case LayoutStoreError::INVALID_SCHEMA:
            _sendJsonError(request, 400, "Layout schema invalid", "LAYOUT_INVALID");
            return;
        case LayoutStoreError::FS_FULL:
            _sendJsonError(request, 507, "Filesystem full", "FS_FULL");
            return;
        case LayoutStoreError::IO_ERROR:
        default:
            _sendJsonError(request, 500, "Layout IO error", "IO_ERROR");
            return;
    }
}

void WebPortal::_handleDeleteLayout(AsyncWebServerRequest* request) {
    String id = extractLayoutIdFromUrl(request->url(), false);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Missing layout id", "MISSING_ID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    String activeId = LayoutStore::getActiveId();
    if (activeId == id) {
        _sendJsonError(request, 409, "Cannot delete active layout", "LAYOUT_ACTIVE");
        return;
    }
    if (!LayoutStore::remove(id.c_str())) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostLayoutActivate(AsyncWebServerRequest* request) {
    String id = extractLayoutIdFromUrl(request->url(), true);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Invalid activate path", "INVALID_PATH");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    // Verify the layout actually exists — load() returns false on miss.
    String tmp;
    if (!LayoutStore::load(id.c_str(), tmp)) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }
    if (!LayoutStore::setActiveId(id.c_str())) {
        _sendJsonError(request, 500, "Failed to persist active layout id", "NVS_ERROR");
        return;
    }
    // Rule #24: route mode switch through orchestrator. Never call
    // ModeRegistry::requestSwitch directly from WebPortal.
    ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout");

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    data["active_id"] = id;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetWidgetTypes(AsyncWebServerRequest* request) {
    // ─────────────────────────────────────────────────────────────────────
    // SYNC-RISK NOTE (LE-1.4 review follow-up):
    // The widget type strings below ("text", "clock", "logo", "flight_field",
    // "metric") and their field schemas are hard-coded here. The same type
    // strings also live in:
    //   • core/LayoutStore.cpp  — kAllowedWidgetTypes[] (save() validation)
    //   • core/WidgetRegistry.cpp — WidgetRegistry::fromString() (render dispatch)
    // All three lists MUST stay in lock-step. If a new widget type is added
    // without updating this handler, the editor UI (LE-1.7) will have no
    // way to surface it even though LayoutStore accepts it and the
    // renderer draws it. Consolidate in LE-1.7 when the widget system
    // stabilises (e.g. promote to a single WidgetType descriptor table
    // consumed by all three call sites).
    // ─────────────────────────────────────────────────────────────────────
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonArray data = root["data"].to<JsonArray>();

    // Helper lambdas to tighten the repetitive build-up. JsonDocument is
    // scoped to this function — no static allocations (AR rule).
    auto addField = [](JsonArray& fields, const char* key, const char* kind,
                       const char* defaultStr, int defaultInt, bool useInt,
                       const char* const* options, size_t optCount) {
        JsonObject f = fields.add<JsonObject>();
        f["key"] = key;
        f["kind"] = kind;
        if (useInt) f["default"] = defaultInt;
        else        f["default"] = defaultStr;
        if (options != nullptr && optCount > 0) {
            JsonArray arr = f["options"].to<JsonArray>();
            for (size_t i = 0; i < optCount; ++i) arr.add(options[i]);
        }
    };

    // ── text ─────────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "text";
        obj["label"] = "Text";
        JsonArray fields = obj["fields"].to<JsonArray>();
        addField(fields, "content", "string", "", 0, false, nullptr, 0);
        static const char* const alignOptions[] = { "left", "center", "right" };
        addField(fields, "align", "select", "left", 0, false, alignOptions, 3);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 32, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
    }
    // ── clock ────────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "clock";
        obj["label"] = "Clock";
        JsonArray fields = obj["fields"].to<JsonArray>();
        static const char* const clockFmtOptions[] = { "12h", "24h" };
        addField(fields, "content", "select", "24h", 0, false, clockFmtOptions, 2);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
    }
    // ── logo ─────────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "logo";
        obj["label"] = "Logo";
        // LE-1.5: real RGB565 bitmap pipeline — upload via POST /api/logos,
        // then set content to the ICAO/id (e.g. "UAL"). Falls back to
        // PROGMEM airplane sprite when logo_id is not found on LittleFS.
        JsonArray fields = obj["fields"].to<JsonArray>();
        addField(fields, "content", "string", "", 0, false, nullptr, 0);
        addField(fields, "color", "color", "#0000FF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 16, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 16, true, nullptr, 0);
    }
    // ── flight_field ─────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "flight_field";
        obj["label"] = "Flight Field";
        obj["note"] = "LE-1.8 \u2014 not yet implemented, renders nothing";
        JsonArray fields = obj["fields"].to<JsonArray>();
        addField(fields, "content", "string", "", 0, false, nullptr, 0);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
    }
    // ── metric ───────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "metric";
        obj["label"] = "Metric";
        obj["note"] = "LE-1.8 \u2014 not yet implemented, renders nothing";
        JsonArray fields = obj["fields"].to<JsonArray>();
        addField(fields, "content", "string", "", 0, false, nullptr, 0);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}


]]></file>
<file id="bff86d84" path="firmware/core/WidgetRegistry.h" label="SOURCE CODE"><![CDATA[

#pragma once
/*
Purpose: Static widget registry with switch-based dispatch by widget type.

Story LE-1.2 (widget-registry-and-core-widgets) — promotes the V0 spike's
dispatch pattern to a production library. Zero virtual dispatch, zero heap
allocation on the render hot path.

Responsibilities:
  - Define the production `WidgetType` enum and `WidgetSpec` struct used by
    render-time code (mode renderers, LE-1.3 integration).
  - Translate JSON widget type strings ("text", "clock", ...) to enum values.
  - Dispatch a pre-parsed `WidgetSpec` to the matching stateless render
    function via a compile-time `switch` on `WidgetType`.

Non-responsibilities:
  - Does NOT parse JSON. Widget specs are built by the caller at layout-init
    time (LE-1.3) from validated JSON.
  - Does NOT own any state. All methods are static; the render functions are
    free functions with function-scoped statics where caching is needed.
  - Does NOT allocate heap. Dispatch is a pointer-free switch; render
    functions use stack buffers or file-scoped static buffers only.

Architecture references:
  - firmware/modes/CustomLayoutMode.cpp (V0 spike reference pattern)
  - firmware/core/LayoutStore.cpp (kAllowedWidgetTypes[] — must stay in sync)
  - firmware/interfaces/DisplayMode.h (RenderContext is reused, not redefined)
*/

#include <Arduino.h>
#include <stdint.h>

#include "interfaces/DisplayMode.h"

// ------------------------------------------------------------------
// Font / dimension constants shared across widget render functions.
// Matches the 5x7 default Adafruit GFX font + 1px spacing used by
// ClassicCardMode and the V0 spike (CustomLayoutMode.cpp).
// ------------------------------------------------------------------
static constexpr int WIDGET_CHAR_W = 6;
static constexpr int WIDGET_CHAR_H = 8;

// ------------------------------------------------------------------
// WidgetType — production enum, wire-compatible with JSON type strings.
// Integer values are stable (persisted to NVS or used as table indices is
// not a current requirement, but the explicit numbering prevents accidental
// reordering across stories).
// LayoutStore::kAllowedWidgetTypes[] must contain the matching string set.
// ------------------------------------------------------------------
enum class WidgetType : uint8_t {
    Text        = 0,
    Clock       = 1,
    Logo        = 2,
    FlightField = 3,   // LE-1.8
    Metric      = 4,   // LE-1.8
    Unknown     = 0xFF,
};

// ------------------------------------------------------------------
// WidgetSpec — compact value type describing one widget instance.
//
// Pre-populated at layout init time (LE-1.3) from validated JSON. Widget
// render functions take this by const-ref and MUST NOT mutate it. Field
// layout is tuned for stack allocation when an entire layout (up to 24
// widgets) is materialised at once.
// ------------------------------------------------------------------
struct WidgetSpec {
    WidgetType type;
    int16_t    x;
    int16_t    y;
    uint16_t   w;
    uint16_t   h;
    uint16_t   color;        // RGB565
    char       id[16];       // widget instance id (e.g. "w1"), 15 chars + null
    char       content[48];  // text content / clock format / logo_id
    uint8_t    align;        // 0=left, 1=center, 2=right (TextWidget only)
    uint8_t    _reserved;    // pad for alignment
};

// ------------------------------------------------------------------
// WidgetRegistry — static dispatch façade.
// ------------------------------------------------------------------
class WidgetRegistry {
public:
    // Dispatch to the render function matching `type`. Returns the render
    // function's success flag. For Unknown or unimplemented types, returns
    // false without touching the framebuffer.
    //
    // `type` is passed explicitly (rather than reading `spec.type`) to keep
    // the dispatcher honest about the contract and to allow future callers
    // to force a type without mutating the spec.
    static bool dispatch(WidgetType type,
                         const WidgetSpec& spec,
                         const RenderContext& ctx);

    // Convert a JSON type string (e.g. "text") to its enum value. Returns
    // WidgetType::Unknown for null/empty/unknown input. Comparison is
    // strictly case-sensitive to match the LayoutStore allowlist.
    static WidgetType fromString(const char* typeStr);

    // True iff `typeStr` maps to a non-Unknown WidgetType. Cheap predicate
    // over fromString().
    static bool isKnownType(const char* typeStr);
};


]]></file>
<file id="54ecacb1" path="firmware/data-src/common.js" label="SOURCE CODE"><![CDATA[

/* FlightWall shared HTTP helpers */
var FW = (function() {
  'use strict';

  function parseJsonResponse(res) {
    return res.text().then(function(text) {
      if (!text) {
        return {
          status: res.status,
          body: { ok: false, error: 'Empty server response (HTTP ' + res.status + ')', code: 'EMPTY_RESPONSE' }
        };
      }

      try {
        return { status: res.status, body: JSON.parse(text) };
      } catch (err) {
        return {
          status: res.status,
          body: { ok: false, error: 'Invalid server response (HTTP ' + res.status + ')', code: 'INVALID_RESPONSE' }
        };
      }
    });
  }

  function fetchJson(url, opts) {
    return fetch(url, opts || {}).then(function(res) {
      return parseJsonResponse(res);
    });
  }

  function get(url) {
    return fetchJson(url);
  }

  function post(url, data) {
    return fetchJson(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    });
  }

  function del(url) {
    return fetchJson(url, { method: 'DELETE' });
  }

  /* Toast notification system */
  var toastContainer = null;

  function ensureToastContainer() {
    if (toastContainer) return toastContainer;
    toastContainer = document.getElementById('toast-container');
    if (!toastContainer) {
      toastContainer = document.createElement('div');
      toastContainer.id = 'toast-container';
      document.body.appendChild(toastContainer);
    }
    return toastContainer;
  }

  function showToast(message, severity) {
    var container = ensureToastContainer();
    var toast = document.createElement('div');
    toast.className = 'toast toast-' + (severity || 'success');
    toast.textContent = message;
    container.appendChild(toast);
    // Trigger reflow for animation
    toast.offsetHeight;
    toast.classList.add('toast-visible');
    setTimeout(function() {
      toast.classList.remove('toast-visible');
      toast.classList.add('toast-exit');
      setTimeout(function() {
        if (toast.parentNode) toast.parentNode.removeChild(toast);
      }, 300);
    }, 2500);
  }

  return { get: get, post: post, del: del, showToast: showToast };
})();


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
    <nav><a href="/health.html">System Health</a> <a href="#mode-picker">Display Modes</a></nav>
  </header>

  <!-- Upgrade notification banner host (ds-3.6) — populated by JS -->
  <div id="mode-upgrade-banner-host"></div>

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

    <!-- Mode Picker (Story ds-3.3) -->
    <section class="card" id="mode-picker">
      <h2 id="mode-picker-heading" tabindex="-1">Display Mode</h2>
      <div class="mode-status-line" id="modeStatusLine" aria-live="polite">
        Active: <span class="mode-status-name" id="modeStatusName">&mdash;</span>
        <span class="mode-status-reason" id="modeStatusReason">loading</span>
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

    <!-- Firmware Card (Story fn-1.6, dl-6.2) -->
    <section class="card" id="firmware-card">
      <h2>Firmware</h2>
      <p id="fw-version" class="fw-version-text"></p>
      <div id="rollback-banner" class="banner-warning" role="alert" aria-live="polite" style="display:none">
        <span>Firmware rolled back to previous version</span>
        <button type="button" class="dismiss-btn" id="btn-dismiss-rollback" aria-label="Dismiss rollback warning">&times;</button>
      </div>
      <button type="button" class="btn-secondary" id="btn-check-update">Check for Updates</button>
      <div id="ota-update-banner" class="ota-update-banner" role="alert" aria-live="polite" style="display:none">
        <p class="ota-update-text" id="ota-update-text"></p>
        <div class="ota-update-actions">
          <button type="button" class="ota-notes-toggle" id="btn-toggle-notes" aria-expanded="false" aria-controls="ota-release-notes">View Release Notes</button>
          <button type="button" class="apply-btn ota-update-now-btn" id="btn-update-now">Update Now</button>
        </div>
        <div class="ota-release-notes" id="ota-release-notes" role="region" aria-label="Release notes" style="display:none">
          <pre id="ota-notes-content"></pre>
        </div>
      </div>
      <div id="ota-uptodate" style="display:none">
        <p class="ota-uptodate-text" id="ota-uptodate-text"></p>
      </div>
      <div id="ota-pull-progress" style="display:none" aria-live="polite">
        <p class="ota-pull-status" id="ota-pull-status"></p>
        <div class="ota-progress" id="ota-pull-bar-wrap" role="progressbar" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100">
          <div class="ota-progress-bar" id="ota-pull-bar"></div>
          <span class="ota-progress-text" id="ota-pull-pct">0%</span>
        </div>
        <p id="ota-pull-error" class="ota-pull-error" style="display:none"></p>
        <button type="button" class="btn-secondary" id="btn-ota-retry" style="display:none">Retry</button>
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

    <!-- Night Mode Card (Story fn-2.3) -->
    <section class="card" id="nightmode-card">
      <h2>Night Mode</h2>

      <div class="nm-status" id="nm-status" aria-live="polite">
        <span class="nm-ntp-badge" id="nm-ntp-badge">NTP: --</span>
        <span class="nm-sched-status" id="nm-sched-status"></span>
      </div>

      <label class="nm-toggle-label">
        <input type="checkbox" id="nm-enabled">
        <span>Enable Schedule</span>
      </label>

      <div class="nm-fields" id="nm-fields">
        <label for="nm-dim-start">Dim Start</label>
        <input type="time" id="nm-dim-start" value="23:00">

        <label for="nm-dim-end">Dim End</label>
        <input type="time" id="nm-dim-end" value="07:00">

        <label for="nm-dim-brt">Dim Brightness</label>
        <div class="range-row">
          <input type="range" id="nm-dim-brt" min="0" max="255" value="10">
          <span class="range-val" id="nm-dim-brt-val">10</span>
        </div>

        <label for="nm-timezone">Timezone</label>
        <select id="nm-timezone" class="cal-select"></select>
      </div>
    </section>

    <!-- Mode Schedule Card (Story dl-4.2) -->
    <section class="card" id="schedule-card">
      <h2>Mode Schedule</h2>

      <div class="sched-status" id="sched-status" aria-live="polite">
        <span class="sched-orch-badge" id="sched-orch-badge">State: --</span>
      </div>

      <div class="sched-rules-list" id="sched-rules-list">
        <p class="sched-empty-state" id="sched-empty-state">No schedule rules configured. Add a rule to automatically switch display modes at set times.</p>
      </div>

      <button type="button" class="btn-secondary" id="btn-add-rule">+ Add Rule</button>
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
 * IANA-to-POSIX timezone mapping (Story fn-2.1).
 * Browser auto-detects IANA via Intl API; ESP32 needs POSIX string for configTzTime().
 * POSIX sign convention is inverted from UTC offset (west=positive, east=negative).
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
 * Falls back to UTC if browser timezone is not in TZ_MAP.
 * @returns {{ iana: string, posix: string }}
 */
function getTimezoneConfig() {
  var iana = "UTC";
  try {
    iana = Intl.DateTimeFormat().resolvedOptions().timeZone || "UTC";
  } catch (e) {
    // Intl API not available — fall back to UTC
  }
  var posix = TZ_MAP[iana] || "UTC0";
  return { iana: iana, posix: posix };
}

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
  var dirtySections = { display: false, timing: false, network: false, hardware: false, nightmode: false };
  // nmTimezoneDirty tracks explicit user changes to the timezone dropdown separately from
  // the nightmode dirty section, so posixToIana()'s 'UTC' fallback cannot silently overwrite
  // a custom POSIX string stored on the device when other Night Mode fields are changed.
  var nmTimezoneDirty = false;

  function markSectionDirty(section) {
    dirtySections[section] = true;
    applyBar.style.display = '';
  }

  function clearDirtyState() {
    dirtySections.display = false;
    dirtySections.timing = false;
    dirtySections.network = false;
    dirtySections.hardware = false;
    dirtySections.nightmode = false;
    nmTimezoneDirty = false;
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
    if (dirtySections.nightmode) {
      // Use cached DOM references (nmEnabled, nmDimStart, etc.) — consistent with all
      // other sections and avoids repeated getElementById lookups on the Apply hot path.
      payload.sched_enabled = nmEnabled.checked ? 1 : 0;
      payload.sched_dim_start = timeToMinutes(nmDimStart.value);
      payload.sched_dim_end = timeToMinutes(nmDimEnd.value);
      payload.sched_dim_brt = parseInt(nmDimBrt.value, 10);
      // Only include timezone when the user (or auto-suggest) explicitly changed the dropdown.
      // This prevents posixToIana()'s "UTC" fallback from silently overwriting a custom
      // POSIX string stored on the device when the user changes an unrelated Night Mode field.
      if (nmTimezoneDirty && nmTimezone && nmTimezone.value) {
        payload.timezone = TZ_MAP[nmTimezone.value] || 'UTC0';
      }
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

  // --- Night Mode time helpers (Story fn-2.3) ---
  function timeToMinutes(timeStr) {
    if (!timeStr || typeof timeStr !== 'string') return 0;
    var parts = timeStr.split(':');
    // Use < 2 (not !== 2) so that HH:MM:SS emitted by some browsers (e.g. when
    // step < 60 is inferred) still parses correctly — we only need hours and minutes.
    if (parts.length < 2) return 0;
    var h = parseInt(parts[0], 10);
    var m = parseInt(parts[1], 10);
    if (isNaN(h) || isNaN(m)) return 0;
    var total = h * 60 + m;
    if (total < 0 || total > 1439) return 0;
    return total;
  }

  function minutesToTime(mins) {
    var h = Math.floor(mins / 60);
    var m = mins % 60;
    return (h < 10 ? '0' : '') + h + ':' + (m < 10 ? '0' : '') + m;
  }

  function posixToIana(posix) {
    var keys = Object.keys(TZ_MAP);
    for (var i = 0; i < keys.length; i++) {
      if (TZ_MAP[keys[i]] === posix) return keys[i];
    }
    return 'UTC';
  }

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

      // Night Mode (Story fn-2.3)
      // Use typeof guards (not !==undefined) so that a null value from a malformed API
      // response does not reach parseInt() / minutesToTime() and produce NaN display.
      if (d.sched_enabled !== undefined && d.sched_enabled !== null) {
        nmEnabled.checked = (d.sched_enabled === 1 || d.sched_enabled === '1' || d.sched_enabled === true);
      }
      if (typeof d.sched_dim_start === 'number') {
        nmDimStart.value = minutesToTime(d.sched_dim_start);
      }
      if (typeof d.sched_dim_end === 'number') {
        nmDimEnd.value = minutesToTime(d.sched_dim_end);
      }
      if (typeof d.sched_dim_brt === 'number') {
        nmDimBrt.value = d.sched_dim_brt;
        nmDimBrtVal.textContent = d.sched_dim_brt;
      }
      if (d.timezone !== undefined) {
        var iana = posixToIana(d.timezone);
        nmTimezone.value = iana;
        // If device is still on default UTC0 and browser has a different zone, auto-suggest.
        // Also mark nmTimezoneDirty so the auto-suggested value is included in the payload
        // when the user confirms by clicking Apply.
        if (d.timezone === 'UTC0') {
          var detected = getTimezoneConfig();
          if (detected.iana !== 'UTC' && TZ_MAP[detected.iana]) {
            nmTimezone.value = detected.iana;
            nmTimezoneDirty = true;
            markSectionDirty('nightmode');
          }
        }
      }
      updateNmFieldState();

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

  // --- Night Mode card (Story fn-2.3) ---
  var nmEnabled = document.getElementById('nm-enabled');
  var nmDimStart = document.getElementById('nm-dim-start');
  var nmDimEnd = document.getElementById('nm-dim-end');
  var nmDimBrt = document.getElementById('nm-dim-brt');
  var nmDimBrtVal = document.getElementById('nm-dim-brt-val');
  var nmTimezone = document.getElementById('nm-timezone');
  var nmFields = document.getElementById('nm-fields');

  // Populate timezone dropdown from TZ_MAP
  (function() {
    var keys = Object.keys(TZ_MAP).sort();
    for (var i = 0; i < keys.length; i++) {
      var opt = document.createElement('option');
      opt.value = keys[i];
      opt.textContent = keys[i].replace(/_/g, ' ');
      nmTimezone.appendChild(opt);
    }
  })();

  function updateNmFieldState() {
    var on = nmEnabled.checked;
    nmDimStart.disabled = !on;
    nmDimEnd.disabled = !on;
    nmDimBrt.disabled = !on;
    nmTimezone.disabled = !on;
    nmFields.classList.toggle('nm-fields-disabled', !on);
  }

  nmEnabled.addEventListener('change', function() {
    updateNmFieldState();
    markSectionDirty('nightmode');
  });

  nmDimStart.addEventListener('change', function() { markSectionDirty('nightmode'); });
  nmDimEnd.addEventListener('change', function() { markSectionDirty('nightmode'); });
  nmDimBrt.addEventListener('input', function() {
    nmDimBrtVal.textContent = nmDimBrt.value;
  });
  nmDimBrt.addEventListener('change', function() { markSectionDirty('nightmode'); });
  nmTimezone.addEventListener('change', function() { nmTimezoneDirty = true; markSectionDirty('nightmode'); });

  updateNmFieldState();

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

  // --- Mode Picker (Story dl-1.5, ds-3.4) ---
  var modeStatusName = document.getElementById('modeStatusName');
  var modeStatusReason = document.getElementById('modeStatusReason');
  var modeCardsList = document.getElementById('modeCardsList');
  var modePicker = document.getElementById('mode-picker');
  var modeSwitchInFlight = false;
  var SWITCH_POLL_INTERVAL = 200;  // ms between GET polls during switch
  var SWITCH_POLL_TIMEOUT = 2000;  // max ms to poll before treating as failure
  var ERROR_DISMISS_MS = 5000;     // auto-dismiss scoped error after 5s
  // Keyboard support: Enter/Space on mode cards triggers switchMode (ds-3.5 AC #2)
  if (modeCardsList) {
    modeCardsList.addEventListener('keydown', function(e) {
      var card = e.target.closest('.mode-card');
      if (!card) return;
      if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault(); // Prevent Space page scroll
        var modeId = card.getAttribute('data-mode-id');
        if (modeId) switchMode(modeId);
      }
    });
  }

  // Exposed for ds-3.6 banner dismiss — moves focus to the mode picker section heading (AC #4)
  window.focusModePickerHeading = function() {
    var heading = document.getElementById('mode-picker-heading');
    if (heading && typeof heading.focus === 'function') {
      heading.focus({ preventScroll: true });
    }
  };

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
    if (modeStatusName && activeMode) {
      modeStatusName.textContent = activeMode.name;
    }
    if (modeStatusReason) {
      // Wrap reason in parens only when non-empty; avoids orphaned "()" on screen
      var reason = data.state_reason || '';
      modeStatusReason.textContent = reason ? '(' + reason + ')' : '';
    }
    // Update mode cards (active state + subtitle + aria-current + tabindex)
    if (modeCardsList) {
      var cards = modeCardsList.querySelectorAll('.mode-card');
      for (var j = 0; j < cards.length; j++) {
        var cardId = cards[j].getAttribute('data-mode-id');
        var subtitle = cards[j].querySelector('.mode-card-subtitle');
        if (cardId === data.active) {
          cards[j].classList.add('active');
          cards[j].setAttribute('aria-current', 'true');
          cards[j].setAttribute('tabindex', '-1');
          cards[j].removeAttribute('role'); // active cards are not actionable button targets
          if (subtitle) subtitle.textContent = data.state_reason || '';
        } else {
          cards[j].classList.remove('active');
          cards[j].removeAttribute('aria-current');
          cards[j].setAttribute('tabindex', '0');
          cards[j].setAttribute('role', 'button'); // idle cards are keyboard-activatable (AC #1)
          if (subtitle) subtitle.textContent = '';
        }
        cards[j].classList.remove('switching');
        cards[j].classList.remove('disabled');
        cards[j].removeAttribute('aria-busy');
        cards[j].removeAttribute('aria-disabled');
      }
    }
  }

  /** Find a card DOM element by mode id */
  function getModeCard(id) {
    if (!modeCardsList) return null;
    return modeCardsList.querySelector('.mode-card[data-mode-id="' + id + '"]');
  }

  /** Clear any existing scoped error alert from a card */
  function clearCardError(card) {
    if (!card) return;
    var existing = card.querySelector('.mode-card-alert');
    if (existing) existing.parentNode.removeChild(existing);
  }

  /** Show a scoped inline error on a card with auto-dismiss (AC #4) */
  function showCardError(card, message) {
    if (!card) return;
    clearCardError(card);
    var alert = document.createElement('div');
    alert.className = 'mode-card-alert';
    alert.setAttribute('role', 'alert');
    alert.setAttribute('aria-live', 'polite');
    alert.textContent = message;
    card.appendChild(alert);

    // Auto-dismiss after ERROR_DISMISS_MS or on next click/keydown in mode-picker
    var dismissed = false;
    function dismiss() {
      if (dismissed) return;
      dismissed = true;
      clearTimeout(timer);
      if (modePicker) modePicker.removeEventListener('click', dismiss);
      if (modePicker) modePicker.removeEventListener('keydown', dismiss);
      // Remove only this specific alert element — avoids clearing a newer error
      // injected by a subsequent showCardError call on the same card.
      if (alert.parentNode) alert.parentNode.removeChild(alert);
    }
    // Stop click propagation on the alert itself so that clicking the error message
    // does not bubble to the parent .mode-card and accidentally re-trigger switchMode.
    alert.addEventListener('click', function(e) {
      e.stopPropagation();
      dismiss();
    });
    var timer = setTimeout(dismiss, ERROR_DISMISS_MS);
    if (modePicker) {
      modePicker.addEventListener('click', dismiss, { once: true });
      modePicker.addEventListener('keydown', dismiss, { once: true });
    }
  }

  /** Clear switching/disabled states on all cards and restore interactivity */
  function clearSwitchingStates() {
    if (!modeCardsList) return;
    var cards = modeCardsList.querySelectorAll('.mode-card');
    for (var i = 0; i < cards.length; i++) {
      cards[i].classList.remove('switching');
      cards[i].classList.remove('disabled');
      cards[i].removeAttribute('aria-busy');
      cards[i].removeAttribute('aria-disabled');
      // Restore tabindex and role: active card gets tabindex=-1 (no role); idle gets tabindex=0 + role=button
      var isActive = cards[i].classList.contains('active');
      cards[i].setAttribute('tabindex', isActive ? '-1' : '0');
      if (isActive) {
        cards[i].removeAttribute('role');
      } else {
        cards[i].setAttribute('role', 'button');
      }
      var sub = cards[i].querySelector('.mode-card-subtitle');
      if (sub && sub.textContent === 'Switching...') {
        // Restore the subtitle that was shown before the switch was initiated;
        // if the device goes offline mid-failure the original state_reason is preserved.
        sub.textContent = sub.getAttribute('data-prev-text') || '';
        sub.removeAttribute('data-prev-text');
      }
    }
  }

  function buildSchematic(zoneLayout, modeName) {
    var wrap = document.createElement('div');
    wrap.className = 'mode-schematic';
    wrap.setAttribute('role', 'img');
    wrap.setAttribute('aria-label', 'Schematic preview of zone layout for ' + modeName);
    if (!zoneLayout || !zoneLayout.length) return wrap;
    for (var i = 0; i < zoneLayout.length; i++) {
      var z = zoneLayout[i];
      if (!z.relW || !z.relH) continue;
      var cell = document.createElement('div');
      cell.className = 'mode-schematic-zone';
      cell.setAttribute('aria-hidden', 'true');
      // Guard relX/relY: undefined (e.g. JSON omitting 0-value fields) → 0
      var rx = (z.relX !== undefined && z.relX !== null) ? z.relX : 0;
      var ry = (z.relY !== undefined && z.relY !== null) ? z.relY : 0;
      var cs = Math.max(1, rx + 1);
      var ce = Math.min(101, cs + z.relW);
      var rs = Math.max(1, ry + 1);
      var re = Math.min(101, rs + z.relH);
      cell.style.gridColumn = cs + ' / ' + ce;
      cell.style.gridRow = rs + ' / ' + re;
      var label = z.label || '';
      if (label.length > 8) label = label.substring(0, 7) + '\u2026';
      cell.textContent = label;
      wrap.appendChild(cell);
    }
    return wrap;
  }

  /**
   * Build a settings panel for a mode card (Story dl-5.1, AC #3)
   * @param {object} mode - Mode object from GET /api/display/modes
   * @returns {HTMLElement|null} - Settings panel element or null if no settings
   *
   * Control types:
   * - enum → <select> (options from enumOptions split on comma, trimmed)
   * - uint8/uint16 → number input with min/max
   * - bool → checkbox (value 0/1 for JSON)
   */
  function buildModeSettingsPanel(mode) {
    // AC #3: Empty settings / null → no settings block
    if (!mode.settings || !mode.settings.length) return null;

    var panel = document.createElement('div');
    panel.className = 'mode-settings-panel';

    // Stop click propagation so clicking settings doesn't trigger mode switch
    panel.addEventListener('click', function(e) {
      e.stopPropagation();
    });

    // Track setting values for Apply button
    var settingValues = {};

    for (var i = 0; i < mode.settings.length; i++) {
      var setting = mode.settings[i];
      var row = document.createElement('div');
      row.className = 'mode-setting-row';

      var label = document.createElement('label');
      label.className = 'mode-setting-label';
      label.textContent = setting.label;

      var control;

      if (setting.type === 'enum') {
        // AC #3: enum → <select> with options from enumOptions split on comma
        control = document.createElement('select');
        control.className = 'mode-setting-input mode-setting-select';
        if (setting.enumOptions) {
          var options = setting.enumOptions.split(',');
          for (var j = 0; j < options.length; j++) {
            var opt = document.createElement('option');
            opt.value = j;  // Numeric value for JSON
            opt.textContent = options[j].trim();
            if (j === setting.value) opt.selected = true;
            control.appendChild(opt);
          }
        }
        settingValues[setting.key] = setting.value;
        control.addEventListener('change', (function(key) {
          return function(e) {
            settingValues[key] = parseInt(e.target.value, 10);
          };
        })(setting.key));

      } else if (setting.type === 'bool') {
        // AC #3: bool → checkbox (values 0/1)
        control = document.createElement('input');
        control.type = 'checkbox';
        control.className = 'mode-setting-checkbox';
        control.checked = setting.value === 1;
        settingValues[setting.key] = setting.value;
        control.addEventListener('change', (function(key) {
          return function(e) {
            settingValues[key] = e.target.checked ? 1 : 0;
          };
        })(setting.key));

      } else {
        // AC #3: uint8/uint16 → number input with min/max
        control = document.createElement('input');
        control.type = 'number';
        control.className = 'mode-setting-input mode-setting-number';
        control.min = setting.min;
        control.max = setting.max;
        control.value = setting.value;
        settingValues[setting.key] = setting.value;
        control.addEventListener('change', (function(key, min, max) {
          return function(e) {
            var val = parseInt(e.target.value, 10);
            if (isNaN(val)) val = min;
            if (val < min) val = min;
            if (val > max) val = max;
            e.target.value = val;
            settingValues[key] = val;
          };
        })(setting.key, setting.min, setting.max));
      }

      // Associate label with control
      var controlId = 'mode-setting-' + mode.id + '-' + setting.key;
      control.id = controlId;
      label.setAttribute('for', controlId);

      row.appendChild(label);
      row.appendChild(control);
      panel.appendChild(row);
    }

    // Apply button (AC #4: POST { mode, settings })
    var actionsRow = document.createElement('div');
    actionsRow.className = 'mode-setting-actions';

    var applyBtn = document.createElement('button');
    applyBtn.type = 'button';
    applyBtn.className = 'mode-setting-apply';
    applyBtn.textContent = 'Apply';

    applyBtn.addEventListener('click', function() {
      applyBtn.disabled = true;
      applyBtn.textContent = 'Applying...';

      // AC #4: POST /api/display/mode with { mode, settings }
      FW.post('/api/display/mode', {
        mode: mode.id,
        settings: settingValues
      }).then(function(res) {
        applyBtn.disabled = false;
        applyBtn.textContent = 'Apply';
        if (res.body.ok) {
          FW.showToast('Settings applied', 'success');
          // Reload modes to refresh current values
          loadDisplayModes();
        } else {
          FW.showToast(res.body.error || 'Failed to apply settings', 'error');
        }
      }).catch(function() {
        applyBtn.disabled = false;
        applyBtn.textContent = 'Apply';
        FW.showToast('Network error', 'error');
      });
    });

    actionsRow.appendChild(applyBtn);
    panel.appendChild(actionsRow);

    return panel;
  }

  function renderModeCards(data) {
    if (!modeCardsList || !data.modes) return;
    modeCardsList.innerHTML = '';
    for (var i = 0; i < data.modes.length; i++) {
      var mode = data.modes[i];
      var isActive = mode.id === data.active;
      var card = document.createElement('div');
      card.className = 'mode-card' + (isActive ? ' active' : '');
      card.setAttribute('data-mode-id', mode.id);
      // Active card: tabindex="-1" (not in tab order as actionable); idle: tabindex="0"
      card.setAttribute('tabindex', isActive ? '-1' : '0');
      if (isActive) {
        card.setAttribute('aria-current', 'true');
      } else {
        // Idle cards expose role="button" so screen readers announce them as interactive (AC #1)
        card.setAttribute('role', 'button');
      }
      var headerRow = document.createElement('div');
      headerRow.className = 'mode-card-header';
      var nameEl = document.createElement('span');
      nameEl.className = 'mode-card-name';
      nameEl.textContent = mode.name;
      headerRow.appendChild(nameEl);
      if (isActive) {
        var dotWrap = document.createElement('span');
        dotWrap.className = 'mode-active-indicator';
        var dot = document.createElement('span');
        dot.className = 'mode-active-dot';
        dotWrap.appendChild(dot);
        dotWrap.appendChild(document.createTextNode(' Active'));
        headerRow.appendChild(dotWrap);
      }
      card.appendChild(headerRow);
      var subtitleEl = document.createElement('div');
      subtitleEl.className = 'mode-card-subtitle';
      subtitleEl.textContent = isActive ? (data.state_reason || '') : '';
      card.appendChild(subtitleEl);
      card.appendChild(buildSchematic(mode.zone_layout, mode.name));

      // Story dl-5.1, AC #3: Per-mode settings panel
      var settingsPanel = buildModeSettingsPanel(mode);
      if (settingsPanel) {
        card.appendChild(settingsPanel);
      }

      card.addEventListener('click', (function(modeId) {
        return function() {
          switchMode(modeId);
        };
      })(mode.id));
      modeCardsList.appendChild(card);
    }
  }

  /**
   * switchMode — full orchestration (ds-3.4)
   * 1. Track previousActiveId, apply switching + disabled states
   * 2. POST /api/display/mode { mode: id }
   * 3. Poll GET /api/display/modes until settled or timeout
   * 4. Finalize UI: active/idle cards, focus management, error handling
   */
  function switchMode(modeId) {
    if (modeSwitchInFlight) return;
    modeSwitchInFlight = true;

    // Track previous active mode before switch
    var previousActiveId = null;
    var cards = modeCardsList ? modeCardsList.querySelectorAll('.mode-card') : [];
    for (var i = 0; i < cards.length; i++) {
      if (cards[i].classList.contains('active')) {
        previousActiveId = cards[i].getAttribute('data-mode-id');
      }
      // NOTE: Do NOT short-circuit for the already-active mode. The orchestrator
      // may be in IDLE_FALLBACK or SCHEDULED state; the user clicking the active
      // card must trigger a POST so the orchestrator transitions to MANUAL.
      // (See ANTIPATTERNS ds-3.1: "Orchestrator bypassed when user re-selects
      // the currently active mode".)
    }

    // Apply switching state to target, disabled to all others (AC #1, #2)
    for (var j = 0; j < cards.length; j++) {
      var cid = cards[j].getAttribute('data-mode-id');
      if (cid === modeId) {
        cards[j].classList.add('switching');
        cards[j].classList.remove('active');
        cards[j].setAttribute('aria-busy', 'true');
        cards[j].setAttribute('tabindex', '-1');
        var sub = cards[j].querySelector('.mode-card-subtitle');
        if (sub) {
          // Store original subtitle so clearSwitchingStates can restore it on failure
          sub.setAttribute('data-prev-text', sub.textContent);
          sub.textContent = 'Switching...';
        }
      } else {
        cards[j].classList.add('disabled');
        // Do NOT remove .active here — per AC #3 the previously active card retains
        // its active chrome until the firmware confirms success (UX-DR1).
        cards[j].setAttribute('aria-disabled', 'true');
        cards[j].setAttribute('tabindex', '-1');
      }
    }

    // POST mode switch request (AC #1)
    FW.post('/api/display/mode', { mode: modeId }).then(function(res) {
      // HTTP error from firmware (AC #8)
      if (!res.body || !res.body.ok) {
        clearSwitchingStates();
        var errMsg = (res.body && res.body.error) ? res.body.error : 'Mode switch failed';
        // Keep modeSwitchInFlight=true until reload completes so a concurrent switch
        // cannot race with the DOM rebuild (renderModeCards wipes switching/disabled classes).
        loadDisplayModes().then(function() {
          modeSwitchInFlight = false;
          var errCard = getModeCard(modeId);
          showCardError(errCard, errMsg);
          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
        }, function() {
          modeSwitchInFlight = false;
          var errCard = getModeCard(modeId);
          showCardError(errCard, errMsg);
          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
        });
        return;
      }

      // Check response for immediate registry error
      var respData = res.body.data || res.body;
      if (respData.registry_error) {
        clearSwitchingStates();
        var heapMsg = (respData.registry_error.indexOf('HEAP') !== -1 || respData.registry_error.indexOf('heap') !== -1)
          ? 'Not enough memory to activate this mode. Current mode restored.'
          : respData.registry_error;
        // Keep modeSwitchInFlight=true until reload completes to prevent concurrent switch races.
        loadDisplayModes().then(function() {
          modeSwitchInFlight = false;
          var errCard = getModeCard(modeId);
          showCardError(errCard, heapMsg);
          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
        }, function() {
          modeSwitchInFlight = false;
          var errCard = getModeCard(modeId);
          showCardError(errCard, heapMsg);
          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
        });
        return;
      }

      // Check switch_state — if already idle and active matches target, finalize now
      var switchState = respData.switch_state || 'idle';
      if (switchState === 'idle' && respData.active === modeId) {
        finalizeModeSwitch(modeId, previousActiveId);
        return;
      }

      // Start polling GET /api/display/modes until settled (AC #7)
      // Use attempt counter instead of Date.now() diff so background-tab
      // setTimeout throttling (≥1000ms) does not falsely trigger timeout.
      var pollAttempts = 0;
      var maxPollAttempts = Math.ceil(SWITCH_POLL_TIMEOUT / SWITCH_POLL_INTERVAL);
      function pollSwitch() {
        if (pollAttempts++ >= maxPollAttempts) {
          // Timeout — treat as failure
          clearSwitchingStates();
          // Keep modeSwitchInFlight=true until reload completes to prevent concurrent switch races.
          loadDisplayModes().then(function() {
            modeSwitchInFlight = false;
            var errCard = getModeCard(modeId);
            showCardError(errCard, 'Mode switch timed out. Current mode restored.');
            if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
          }, function() {
            modeSwitchInFlight = false;
            var errCard = getModeCard(modeId);
            showCardError(errCard, 'Mode switch timed out. Current mode restored.');
            if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
          });
          return;
        }
        FW.get('/api/display/modes').then(function(pollRes) {
          if (!pollRes.body || !pollRes.body.ok || !pollRes.body.data) {
            // Poll failed — retry on next interval
            setTimeout(pollSwitch, SWITCH_POLL_INTERVAL);
            return;
          }
          var d = pollRes.body.data;
          var ss = d.switch_state || 'idle';

          // Check for registry error during switch
          if (d.registry_error) {
            modeSwitchInFlight = false;
            clearSwitchingStates();
            var msg = (d.registry_error.indexOf('HEAP') !== -1 || d.registry_error.indexOf('heap') !== -1)
              ? 'Not enough memory to activate this mode. Current mode restored.'
              : d.registry_error;
            // Render cards first (rebuilds DOM), then attach error to freshly rendered card
            renderModeCards(d);
            updateModeStatus(d);
            var errCard = getModeCard(modeId);
            showCardError(errCard, msg);
            // Restore keyboard focus to the affected card so users can read the inline error (ds-3.5)
            if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
            return;
          }

          // Success: switch_state is idle AND active matches target
          if (ss === 'idle' && d.active === modeId) {
            renderModeCards(d);
            updateModeStatus(d);
            finalizeModeSwitch(modeId, previousActiveId, true); // cards already rendered
            return;
          }

          // switch_state idle but active doesn't match — failure
          if (ss === 'idle' && d.active !== modeId) {
            modeSwitchInFlight = false;
            clearSwitchingStates();
            // Render cards first (rebuilds DOM), then attach error to freshly rendered card
            renderModeCards(d);
            updateModeStatus(d);
            var errCard2 = getModeCard(modeId);
            showCardError(errCard2, 'Mode switch failed. Current mode restored.');
            // Restore keyboard focus to the affected card so users can read the inline error (ds-3.5)
            if (errCard2 && typeof errCard2.focus === 'function') errCard2.focus({ preventScroll: true });
            return;
          }

          // Still switching — poll again
          setTimeout(pollSwitch, SWITCH_POLL_INTERVAL);
        }).catch(function() {
          // Network error during poll — retry
          setTimeout(pollSwitch, SWITCH_POLL_INTERVAL);
        });
      }
      setTimeout(pollSwitch, SWITCH_POLL_INTERVAL);

    }).catch(function() {
      // Network failure (AC #6) — clear states, reload.
      // loadDisplayModes() shows its own connectivity toast on failure, so we do
      // not emit a separate toast here to avoid double-toasting when the device
      // is unreachable.
      modeSwitchInFlight = false;
      clearSwitchingStates();
      loadDisplayModes();
    });
  }

  /** Finalize a successful mode switch — update UI, move focus (AC #3, UX-DR6).
   *  @param {boolean} [skipReload] Pass true when cards were already re-rendered
   *    by the caller (poll-success path) to avoid a redundant GET /api/display/modes. */
  function finalizeModeSwitch(newActiveId, previousActiveId, skipReload) {
    function doFocus() {
      // Focus the newly activated card so keyboard users land on the confirmed
      // active selection. (Focusing previousActiveId was a logic error — the
      // previous card is no longer the relevant target after a successful switch.)
      if (newActiveId) {
        var newCard = getModeCard(newActiveId);
        if (newCard && typeof newCard.focus === 'function') {
          newCard.focus({ preventScroll: true });
        }
      }
    }
    if (skipReload) {
      // DOM already rebuilt by the poll success path — safe to unlock immediately.
      modeSwitchInFlight = false;
      doFocus();
    } else {
      // Keep modeSwitchInFlight=true until the DOM rebuild completes so a
      // concurrent switch cannot race with the loadDisplayModes() GET response
      // and overwrite the new switching/disabled visual state mid-render.
      clearSwitchingStates();
      loadDisplayModes().then(function() {
        modeSwitchInFlight = false;
        doFocus();
      }, function() {
        modeSwitchInFlight = false;
      });
    }
  }

  // --- Upgrade Notification Banner (Story ds-3.6) ---

  function dismissAndClearUpgrade() {
    localStorage.setItem('mode_notif_seen', 'true');
    FW.post('/api/display/notification/dismiss', {}).catch(function() {});
    var host = document.getElementById('mode-upgrade-banner-host');
    if (host) {
      var banner = host.querySelector('.banner-info');
      if (banner) banner.parentNode.removeChild(banner);
    }
  }

  function maybeShowModeUpgradeBanner(data) {
    if (!data || !data.upgrade_notification) return;
    // Check both current key and legacy key (D7 documented 'flightwall_mode_notif_seen')
    if (localStorage.getItem('mode_notif_seen') === 'true') return;
    if (localStorage.getItem('flightwall_mode_notif_seen') === 'true') return;
    var host = document.getElementById('mode-upgrade-banner-host');
    if (!host) return;
    // Clear any stale banner before inserting a fresh one
    host.innerHTML = '';

    var banner = document.createElement('div');
    banner.className = 'banner-info';
    banner.setAttribute('role', 'region');
    banner.setAttribute('aria-labelledby', 'mode-upgrade-banner-title');

    var title = document.createElement('span');
    title.id = 'mode-upgrade-banner-title';
    title.textContent = 'New display modes available';

    var actions = document.createElement('div');
    actions.className = 'banner-actions';

    // Primary action — Try Live Flight Card (AC #6, #7)
    // Delegates to switchMode() to reuse ds-3.4 polling, in-flight locking,
    // error handling, and focus management (Synthesis fix: avoid duplication)
    var tryBtn = document.createElement('button');
    tryBtn.type = 'button';
    tryBtn.className = 'btn-primary-inline';
    tryBtn.textContent = 'Try Live Flight Card';
    tryBtn.addEventListener('click', function() {
      var hasLiveFlight = data.modes && data.modes.some(function(m) { return m.id === 'live_flight'; });
      if (!hasLiveFlight) {
        // Mode not registered in last GET — dismiss + toast (AC #7)
        FW.showToast('Mode not available', 'error');
        dismissAndClearUpgrade();
        return;
      }
      // Dismiss banner immediately before initiating switch (AC #4, #7)
      // The switchMode() orchestrator handles all error feedback via mode picker
      dismissAndClearUpgrade();
      // Delegate to switchMode() — reuses ds-3.4 polling, timeout handling,
      // in-flight guard, registry_error detection, and focus choreography
      switchMode('live_flight');
    });

    // Secondary action — Browse Modes (AC #8)
    var browseBtn = document.createElement('button');
    browseBtn.type = 'button';
    browseBtn.className = 'link-btn';
    browseBtn.textContent = 'Browse Modes';
    browseBtn.addEventListener('click', function() {
      var picker = document.getElementById('mode-picker');
      if (picker) picker.scrollIntoView({ behavior: 'smooth', block: 'start' });
      dismissAndClearUpgrade();
    });

    // Dismiss control — × (AC #4, #5)
    var dismissBtn = document.createElement('button');
    dismissBtn.type = 'button';
    dismissBtn.className = 'dismiss-btn';
    dismissBtn.setAttribute('aria-label', 'Dismiss new display modes notification');
    dismissBtn.innerHTML = '&times;';
    dismissBtn.addEventListener('click', function() {
      dismissAndClearUpgrade();
      // Move focus to #mode-picker-heading per AC #5 / UX-DR6
      var heading = document.getElementById('mode-picker-heading');
      if (heading) heading.focus();
    });

    actions.appendChild(tryBtn);
    actions.appendChild(browseBtn);
    banner.appendChild(title);
    banner.appendChild(actions);
    banner.appendChild(dismissBtn);
    host.appendChild(banner);
  }

  function loadDisplayModes() {
    return FW.get('/api/display/modes').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) {
        FW.showToast('Failed to load display modes', 'error');
        return null;
      }
      var data = res.body.data;
      renderModeCards(data);
      updateModeStatus(data);
      maybeShowModeUpgradeBanner(data); // AC #1 — show banner on first eligible load
      return data; // expose data so callers can reuse modes list (e.g. loadScheduleRules)
    }).catch(function() {
      FW.showToast('Cannot reach device to load display modes. Check connection.', 'error');
      return null;
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

  // --- OTA Update Check (Story dl-6.2) ---
  var btnCheckUpdate = document.getElementById('btn-check-update');
  var otaUpdateBanner = document.getElementById('ota-update-banner');
  var otaUpdateText = document.getElementById('ota-update-text');
  var btnToggleNotes = document.getElementById('btn-toggle-notes');
  var otaReleaseNotes = document.getElementById('ota-release-notes');
  var otaNotesContent = document.getElementById('ota-notes-content');
  var btnUpdateNow = document.getElementById('btn-update-now');
  var otaUptodate = document.getElementById('ota-uptodate');
  var otaUptodateText = document.getElementById('ota-uptodate-text');

  function hideOtaResults() {
    otaUpdateBanner.style.display = 'none';
    otaUptodate.style.display = 'none';
    otaReleaseNotes.style.display = 'none';
    if (btnToggleNotes) {
      btnToggleNotes.setAttribute('aria-expanded', 'false');
      btnToggleNotes.textContent = 'View Release Notes';
    }
  }

  if (btnCheckUpdate) {
    btnCheckUpdate.addEventListener('click', function() {
      btnCheckUpdate.disabled = true;
      btnCheckUpdate.textContent = 'Checking\u2026';
      hideOtaResults();

      FW.get('/api/ota/check').then(function(res) {
        btnCheckUpdate.disabled = false;
        btnCheckUpdate.textContent = 'Check for Updates';

        if (!res.body || !res.body.ok || !res.body.data) {
          FW.showToast('Update check failed', 'error');
          return;
        }

        var d = res.body.data;

        if (d.error) {
          // AC #7: check failed — show error toast, no banner
          FW.showToast(d.error, 'error');
          return;
        }

        if (d.available) {
          // AC #5: update available — show banner
          otaUpdateText.textContent = 'Firmware ' + d.version + ' available \u2014 you\u2019re running ' + d.current_version;
          if (d.release_notes) {
            otaNotesContent.textContent = d.release_notes;
            btnToggleNotes.style.display = '';
          } else {
            btnToggleNotes.style.display = 'none';
          }
          otaUpdateBanner.style.display = '';
          otaUptodate.style.display = 'none';
        } else {
          // AC #6: up to date
          otaUptodateText.textContent = 'Firmware is up to date (v' + d.current_version + ')';
          otaUptodate.style.display = '';
          otaUpdateBanner.style.display = 'none';
        }
      }).catch(function() {
        btnCheckUpdate.disabled = false;
        btnCheckUpdate.textContent = 'Check for Updates';
        FW.showToast('Cannot reach device', 'error');
      });
    });
  }

  // Release notes expand/collapse (AC #8)
  if (btnToggleNotes) {
    btnToggleNotes.addEventListener('click', function() {
      var expanded = btnToggleNotes.getAttribute('aria-expanded') === 'true';
      btnToggleNotes.setAttribute('aria-expanded', String(!expanded));
      otaReleaseNotes.style.display = expanded ? 'none' : '';
      btnToggleNotes.textContent = expanded ? 'View Release Notes' : 'Hide Release Notes';
    });
    btnToggleNotes.addEventListener('keydown', function(e) {
      if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault();
        btnToggleNotes.click();
      }
    });
  }

  // --- OTA Pull Download (Story dl-7.3, AC #4–#9) ---
  var otaPullProgress = document.getElementById('ota-pull-progress');
  var otaPullStatus = document.getElementById('ota-pull-status');
  var otaPullBarWrap = document.getElementById('ota-pull-bar-wrap');
  var otaPullBar = document.getElementById('ota-pull-bar');
  var otaPullPct = document.getElementById('ota-pull-pct');
  var otaPullError = document.getElementById('ota-pull-error');
  var btnOtaRetry = document.getElementById('btn-ota-retry');
  var OTA_POLL_INTERVAL = 500; // AC #4: 500ms poll interval
  var otaPollTimer = null;

  function resetOtaPullState() {
    if (otaPollTimer) { clearInterval(otaPollTimer); otaPollTimer = null; }
    otaPullProgress.style.display = 'none';
    otaPullBar.style.width = '0%';
    otaPullBarWrap.setAttribute('aria-valuenow', '0');
    otaPullBarWrap.classList.remove('indeterminate');
    otaPullPct.textContent = '0%';
    otaPullStatus.textContent = '';
    otaPullError.style.display = 'none';
    otaPullError.textContent = '';
    btnOtaRetry.style.display = 'none';
  }

  function startOtaPull() {
    resetOtaPullState();
    otaPullProgress.style.display = '';
    otaPullStatus.textContent = 'Starting download\u2026';
    otaUpdateBanner.style.display = 'none';
    if (btnUpdateNow) btnUpdateNow.disabled = true;

    FW.post('/api/ota/pull', {}).then(function(res) {
      if (!res.body || !res.body.ok) {
        var errMsg = (res.body && res.body.error) ? res.body.error : 'Could not start download';
        otaPullStatus.textContent = '';
        otaPullError.textContent = errMsg;
        otaPullError.style.display = '';
        if (btnUpdateNow) btnUpdateNow.disabled = false;
        return;
      }
      // Download started — begin polling (AC #4)
      startOtaPollLoop();
    }).catch(function() {
      otaPullStatus.textContent = '';
      otaPullError.textContent = 'Cannot reach device \u2014 check connection';
      otaPullError.style.display = '';
      if (btnUpdateNow) btnUpdateNow.disabled = false;
    });
  }

  function startOtaPollLoop() {
    if (otaPollTimer) clearTimeout(otaPollTimer);
    otaPollTimer = null;
    pollOtaStatus(); // Start first poll immediately
  }

  function pollOtaStatus() {
    FW.get('/api/ota/status').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) return;
      var d = res.body.data;

      if (d.state === 'downloading') {
        // AC #5: progress bar 0–100
        var pct = (d.progress != null) ? d.progress : 0;
        otaPullBar.style.width = pct + '%';
        otaPullBarWrap.setAttribute('aria-valuenow', String(pct));
        otaPullBarWrap.classList.remove('indeterminate');
        otaPullPct.textContent = pct + '%';
        otaPullStatus.textContent = 'Downloading firmware\u2026';
        otaPullError.style.display = 'none';
        btnOtaRetry.style.display = 'none';
        // Schedule next poll (recursive setTimeout pattern)
        otaPollTimer = setTimeout(pollOtaStatus, OTA_POLL_INTERVAL);

      } else if (d.state === 'verifying') {
        // AC #6: indeterminate verification phase
        otaPullBarWrap.classList.add('indeterminate');
        otaPullPct.textContent = 'Verifying\u2026';
        otaPullStatus.textContent = 'Verifying firmware integrity\u2026';
        otaPullError.style.display = 'none';
        btnOtaRetry.style.display = 'none';
        // Continue polling during verification
        otaPollTimer = setTimeout(pollOtaStatus, OTA_POLL_INTERVAL);

      } else if (d.state === 'rebooting') {
        // AC #7: stop polling, show rebooting message
        if (otaPollTimer) { clearTimeout(otaPollTimer); otaPollTimer = null; }
        otaPullBarWrap.classList.remove('indeterminate');
        otaPullBar.style.width = '100%';
        otaPullPct.textContent = '100%';
        otaPullStatus.textContent = 'Rebooting\u2026';
        // Grace period then poll for device return (reuse existing reboot polling)
        setTimeout(function() {
          otaPullStatus.textContent = 'Waiting for device\u2026';
          startOtaPullRebootPolling();
        }, 3000);

      } else if (d.state === 'error') {
        // AC #8: error with optional retry
        if (otaPollTimer) { clearTimeout(otaPollTimer); otaPollTimer = null; }
        otaPullBarWrap.classList.remove('indeterminate');
        otaPullStatus.textContent = 'Update failed';
        otaPullError.textContent = d.error || 'Download failed';
        otaPullError.style.display = '';
        // Show retry if retriable (AC #8)
        if (d.retriable) {
          btnOtaRetry.style.display = '';
        }
        if (btnUpdateNow) btnUpdateNow.disabled = false;

      } else if (d.state === 'idle' || d.state === 'available') {
        // Terminal idle after success or no-op — stop polling
        if (otaPollTimer) { clearTimeout(otaPollTimer); otaPollTimer = null; }
        if (btnUpdateNow) btnUpdateNow.disabled = false;
      }
    }).catch(function() {
      // Fetch failure during poll — may be device rebooting (AC #7)
      if (otaPollTimer) { clearTimeout(otaPollTimer); otaPollTimer = null; }
      otaPullStatus.textContent = 'Device disconnected \u2014 may be rebooting\u2026';
      setTimeout(function() {
        startOtaPullRebootPolling();
      }, 2000);
    });
  }

  function startOtaPullRebootPolling() {
    var attempts = 0;
    var maxAttempts = 20;
    var done = false;
    function poll() {
      if (done) return;
      if (attempts >= maxAttempts) {
        done = true;
        otaPullStatus.textContent = 'Device unreachable \u2014 try refreshing.';
        otaPullStatus.style.color = getComputedStyle(document.documentElement).getPropertyValue('--warning').trim() || '#d29922';
        if (btnUpdateNow) btnUpdateNow.disabled = false;
        return;
      }
      attempts++;
      FW.get('/api/status?_=' + Date.now()).then(function(res) {
        if (done) return;
        if (res.body && res.body.ok && res.body.data) {
          done = true;
          // AC #9: new firmware version, hide update banner
          var newVersion = res.body.data.firmware_version || '';
          FW.showToast('Updated to v' + newVersion, 'success');
          fwVersion.textContent = 'Version: v' + newVersion;
          resetOtaPullState();
          hideOtaResults();
          if (btnUpdateNow) btnUpdateNow.disabled = false;
        } else {
          setTimeout(poll, 3000);
        }
      }).catch(function() {
        if (!done) setTimeout(poll, 3000);
      });
    }
    poll();
  }

  // "Update Now" triggers OTA pull download (AC #4)
  if (btnUpdateNow) {
    btnUpdateNow.addEventListener('click', function() {
      startOtaPull();
    });
  }

  // Retry button (AC #8)
  if (btnOtaRetry) {
    btnOtaRetry.addEventListener('click', function() {
      startOtaPull();
    });
  }

  // --- Settings Export (Story fn-1.6) ---
  var btnExportSettings = document.getElementById('btn-export-settings');
  if (btnExportSettings) {
    btnExportSettings.addEventListener('click', function() {
      // Direct navigation triggers browser download via Content-Disposition header
      window.location.href = '/api/settings/export';
    });
  }

  // --- Night Mode status polling (Story fn-2.3) ---
  var nmNtpBadge = document.getElementById('nm-ntp-badge');
  var nmSchedStatus = document.getElementById('nm-sched-status');

  function updateNmStatus() {
    // Guard: abort silently if the Night Mode card is not present in this HTML build.
    if (!nmNtpBadge || !nmSchedStatus) return;
    // Use recursive setTimeout (not setInterval) so the next poll only fires
    // AFTER the current request resolves — prevents concurrent in-flight
    // requests piling up against the resource-constrained ESP32 web stack.
    FW.get('/api/status').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) {
        // Silently reset indicators; background poll should not toast on transient failures.
        nmNtpBadge.textContent = 'NTP: --';
        nmNtpBadge.className = 'nm-ntp-badge';
        nmSchedStatus.textContent = '';
        setTimeout(updateNmStatus, 10000);
        return;
      }
      var s = res.body.data;

      // NTP badge
      if (s.ntp_synced) {
        nmNtpBadge.textContent = 'NTP Synced';
        nmNtpBadge.className = 'nm-ntp-badge nm-ntp-ok';
      } else {
        nmNtpBadge.textContent = 'NTP Not Synced';
        nmNtpBadge.className = 'nm-ntp-badge nm-ntp-warn';
      }

      // Schedule status
      if (!s.ntp_synced) {
        nmSchedStatus.textContent = 'Schedule paused \u2014 waiting for NTP sync';
        nmSchedStatus.className = 'nm-sched-status nm-inactive';
      } else if (s.schedule_dimming) {
        nmSchedStatus.textContent = 'Currently dimming';
        nmSchedStatus.className = 'nm-sched-status nm-dimming';
      } else if (s.schedule_active) {
        nmSchedStatus.textContent = 'Schedule active (not in dim window)';
        nmSchedStatus.className = 'nm-sched-status nm-active';
      } else {
        nmSchedStatus.textContent = 'Schedule inactive';
        nmSchedStatus.className = 'nm-sched-status nm-inactive';
      }

      setTimeout(updateNmStatus, 10000);
    }).catch(function() {
      nmNtpBadge.textContent = 'NTP: --';
      nmNtpBadge.className = 'nm-ntp-badge';
      nmSchedStatus.textContent = '';
      setTimeout(updateNmStatus, 10000);
    });
  }

  // Kick off initial status poll; subsequent polls are scheduled recursively.
  updateNmStatus();

  // --- Mode Schedule card (Story dl-4.2) ---
  var schedRulesList = document.getElementById('sched-rules-list');
  var schedEmptyState = document.getElementById('sched-empty-state');
  var schedOrchBadge = document.getElementById('sched-orch-badge');
  var btnAddRule = document.getElementById('btn-add-rule');
  var schedModes = [];      // populated from /api/display/modes
  var schedRulesData = [];  // current schedule rules from API
  var schedActiveIdx = -1;
  var schedOrchState = 'manual';

  /** Load schedule rules. Pass pre-loaded modesData to skip duplicate GET /api/display/modes. */
  function loadScheduleRules(modesData) {
    // Reuse caller-supplied modes list if available; otherwise fetch independently
    var modesStep;
    if (modesData && modesData.modes) {
      schedModes = modesData.modes;
      modesStep = Promise.resolve();
    } else {
      modesStep = FW.get('/api/display/modes').then(function(modesRes) {
        if (modesRes.body && modesRes.body.ok && modesRes.body.data && modesRes.body.data.modes) {
          schedModes = modesRes.body.data.modes;
        }
      });
    }
    return modesStep.then(function() {
      return FW.get('/api/schedule');
    }).then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) {
        FW.showToast('Failed to load schedule', 'error');
        return;
      }
      var d = res.body.data;
      schedRulesData = d.rules || [];
      schedActiveIdx = typeof d.active_rule_index === 'number' ? d.active_rule_index : -1;
      schedOrchState = d.orchestrator_state || 'manual';
      renderScheduleUI();
    }).catch(function() {
      FW.showToast('Cannot load schedule', 'error');
    });
  }

  function renderScheduleUI() {
    // Update orchestrator badge
    if (schedOrchBadge) {
      if (schedOrchState === 'scheduled') {
        schedOrchBadge.textContent = 'Scheduled';
        schedOrchBadge.className = 'sched-orch-badge sched-orch-scheduled';
      } else {
        schedOrchBadge.textContent = 'State: ' + schedOrchState.replace(/_/g, ' ');
        schedOrchBadge.className = 'sched-orch-badge sched-orch-manual';
      }
    }

    // Clear existing rows (keep empty state element)
    var rows = schedRulesList.querySelectorAll('.sched-rule-row');
    for (var i = 0; i < rows.length; i++) rows[i].remove();

    if (schedRulesData.length === 0) {
      if (schedEmptyState) schedEmptyState.style.display = '';
      return;
    }
    if (schedEmptyState) schedEmptyState.style.display = 'none';

    for (var r = 0; r < schedRulesData.length; r++) {
      schedRulesList.appendChild(buildRuleRow(r, schedRulesData[r]));
    }
  }

  function buildRuleRow(idx, rule) {
    var row = document.createElement('div');
    row.className = 'sched-rule-row';
    if (schedOrchState === 'scheduled' && idx === schedActiveIdx) {
      row.className += ' sched-rule-active';
    }

    // Start time
    var times = document.createElement('div');
    times.className = 'sched-rule-times';
    var startInput = document.createElement('input');
    startInput.type = 'time';
    startInput.value = minutesToTime(rule.start_min);
    startInput.setAttribute('data-idx', idx);
    startInput.setAttribute('data-field', 'start_min');
    startInput.addEventListener('change', onScheduleRuleChange);
    var sep = document.createElement('span');
    sep.textContent = '\u2013';
    var endInput = document.createElement('input');
    endInput.type = 'time';
    endInput.value = minutesToTime(rule.end_min);
    endInput.setAttribute('data-idx', idx);
    endInput.setAttribute('data-field', 'end_min');
    endInput.addEventListener('change', onScheduleRuleChange);
    times.appendChild(startInput);
    times.appendChild(sep);
    times.appendChild(endInput);

    // Mode dropdown
    var modeWrap = document.createElement('div');
    modeWrap.className = 'sched-rule-mode';
    var modeSelect = document.createElement('select');
    for (var m = 0; m < schedModes.length; m++) {
      var opt = document.createElement('option');
      opt.value = schedModes[m].id;
      opt.textContent = schedModes[m].name;
      if (schedModes[m].id === rule.mode_id) opt.selected = true;
      modeSelect.appendChild(opt);
    }
    modeSelect.setAttribute('data-idx', idx);
    modeSelect.setAttribute('data-field', 'mode_id');
    modeSelect.addEventListener('change', onScheduleRuleChange);
    modeWrap.appendChild(modeSelect);

    // Enabled toggle
    var toggleWrap = document.createElement('div');
    toggleWrap.className = 'sched-rule-toggle';
    var enabledCb = document.createElement('input');
    enabledCb.type = 'checkbox';
    enabledCb.checked = rule.enabled === 1;
    enabledCb.setAttribute('data-idx', idx);
    enabledCb.setAttribute('data-field', 'enabled');
    enabledCb.addEventListener('change', onScheduleRuleChange);
    var enabledLabel = document.createElement('span');
    enabledLabel.textContent = 'On';
    enabledLabel.style.fontSize = '0.8125rem';
    toggleWrap.appendChild(enabledCb);
    toggleWrap.appendChild(enabledLabel);

    // Delete button
    var delBtn = document.createElement('button');
    delBtn.className = 'sched-rule-del';
    delBtn.textContent = '\u00d7';
    delBtn.title = 'Delete rule';
    delBtn.setAttribute('data-idx', idx);
    delBtn.addEventListener('click', onScheduleRuleDelete);

    row.appendChild(times);
    row.appendChild(modeWrap);
    row.appendChild(toggleWrap);
    row.appendChild(delBtn);
    return row;
  }

  function onScheduleRuleChange(e) {
    var idx = parseInt(e.target.getAttribute('data-idx'), 10);
    var field = e.target.getAttribute('data-field');
    if (idx < 0 || idx >= schedRulesData.length) return;

    if (field === 'start_min') {
      schedRulesData[idx].start_min = timeToMinutes(e.target.value);
    } else if (field === 'end_min') {
      schedRulesData[idx].end_min = timeToMinutes(e.target.value);
    } else if (field === 'mode_id') {
      schedRulesData[idx].mode_id = e.target.value;
    } else if (field === 'enabled') {
      schedRulesData[idx].enabled = e.target.checked ? 1 : 0;
    }
    saveScheduleRules();
  }

  function onScheduleRuleDelete(e) {
    var idx = parseInt(e.target.getAttribute('data-idx'), 10);
    if (idx < 0 || idx >= schedRulesData.length) return;
    schedRulesData.splice(idx, 1);
    saveScheduleRules();
  }

  function saveScheduleRules() {
    // Build compacted rules payload (AC #5: no gaps, full replacement)
    var payload = { rules: [] };
    for (var i = 0; i < schedRulesData.length; i++) {
      var r = schedRulesData[i];
      payload.rules.push({
        start_min: r.start_min,
        end_min: r.end_min,
        mode_id: r.mode_id,
        enabled: r.enabled
      });
    }
    FW.post('/api/schedule', payload).then(function(res) {
      if (res.body && res.body.ok) {
        FW.showToast('Schedule saved', 'success');
        // Re-fetch to get updated active_rule_index / orchestrator_state
        loadScheduleRules();
      } else {
        FW.showToast(res.body.error || 'Failed to save schedule', 'error');
      }
    }).catch(function() {
      FW.showToast('Network error saving schedule', 'error');
    });
  }

  if (btnAddRule) {
    btnAddRule.addEventListener('click', function() {
      if (schedRulesData.length >= 8) {
        FW.showToast('Maximum 8 rules allowed', 'error');
        return;
      }
      // Default new rule: 08:00–17:00, first available mode, enabled
      var defaultModeId = schedModes.length > 0 ? schedModes[0].id : 'classic_card';
      schedRulesData.push({
        start_min: 480,
        end_min: 1020,
        mode_id: defaultModeId,
        enabled: 1
      });
      saveScheduleRules();
    });
  }

  // --- Init ---
  var settingsPromise = loadSettings();

  // Load firmware status (version, rollback) on page load (Story fn-1.6)
  loadFirmwareStatus();

  // Load display modes on page load (Story dl-1.5)
  loadDisplayModes();

  // Load schedule rules on page load (Story dl-4.2)
  loadScheduleRules();

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
<file id="a7471f3e" path="firmware/data-src/editor.css" label="STYLESHEET"><![CDATA[

/* FlightWall Layout Editor styles (LE-1.6) */

.editor-layout {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
  background: #0d1117;
  color: #c9d1d9;
}

.editor-header {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 8px 16px;
  background: #161b22;
  border-bottom: 1px solid #30363d;
}

.editor-header h1 {
  margin: 0;
  font-size: 1.1rem;
  font-weight: 600;
}

.editor-back {
  color: #58a6ff;
  text-decoration: none;
  font-size: 0.95rem;
}

.editor-back:hover {
  text-decoration: underline;
}

.editor-main {
  display: flex;
  flex-direction: row;
  flex: 1;
  min-height: 0;
}

.editor-toolbox {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 8px;
  width: 120px;
  background: #161b22;
  border-right: 1px solid #30363d;
  overflow-y: auto;
}

.toolbox-item {
  padding: 8px 10px;
  background: #21262d;
  border: 1px solid #30363d;
  border-radius: 4px;
  cursor: grab;
  user-select: none;
  font-size: 0.9rem;
  text-align: center;
}

.toolbox-item:hover {
  background: #2d333b;
}

.toolbox-item.disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.editor-canvas-wrap {
  flex: 1;
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  padding: 12px;
  overflow: auto;
  background: #0d1117;
}

#editor-canvas {
  image-rendering: pixelated;
  image-rendering: crisp-edges;
  touch-action: none;
  cursor: crosshair;
  display: block;
  background: #111;
  border: 1px solid #30363d;
}

.editor-props {
  width: 220px;
  padding: 12px;
  background: #161b22;
  border-left: 1px solid #30363d;
  overflow-y: auto;
}

.props-placeholder {
  color: #8b949e;
  font-style: italic;
  margin: 0;
}

.editor-controls {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 8px 16px;
  background: #161b22;
  border-top: 1px solid #30363d;
}

.snap-control {
  display: inline-flex;
  border: 1px solid #30363d;
  border-radius: 4px;
  overflow: hidden;
}

.snap-btn {
  padding: 6px 12px;
  background: #21262d;
  color: #c9d1d9;
  border: none;
  border-right: 1px solid #30363d;
  cursor: pointer;
  font-size: 0.9rem;
}

.snap-btn:last-child {
  border-right: none;
}

.snap-btn:hover {
  background: #2d333b;
}

.snap-btn.active {
  background: var(--accent, #58a6ff);
  color: #000;
}

.editor-status {
  color: #8b949e;
  font-size: 0.85rem;
}

/* Mobile: stack toolbox horizontally above the canvas, hide props panel by default */
@media (max-width: 720px) {
  .editor-main {
    flex-direction: column;
  }
  .editor-toolbox {
    flex-direction: row;
    width: auto;
    border-right: none;
    border-bottom: 1px solid #30363d;
    overflow-x: auto;
    overflow-y: hidden;
  }
  .toolbox-item {
    flex: 0 0 auto;
  }
  .editor-props {
    width: auto;
    border-left: none;
    border-top: 1px solid #30363d;
  }
}


]]></file>
<file id="9fa75dbd" path="firmware/data-src/editor.html" label="HTML TEMPLATE"><![CDATA[

<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
  <title>FlightWall Editor</title>
  <link rel="stylesheet" href="/style.css">
  <link rel="stylesheet" href="/editor.css">
</head>
<body>
  <div class="editor-layout">
    <header class="editor-header">
      <a href="/" class="editor-back">&larr; Dashboard</a>
      <h1>Layout Editor</h1>
    </header>
    <div class="editor-main">
      <div class="editor-toolbox" id="editor-toolbox">
        <!-- populated by JS from /api/widgets/types -->
      </div>
      <div class="editor-canvas-wrap">
        <canvas id="editor-canvas"></canvas>
      </div>
      <div class="editor-props" id="editor-props">
        <!-- LE-1.7: property panel placeholder -->
        <p class="props-placeholder">Select a widget</p>
      </div>
    </div>
    <div class="editor-controls">
      <div class="snap-control" id="snap-control">
        <button type="button" class="snap-btn" data-snap="8">Default</button>
        <button type="button" class="snap-btn" data-snap="16">Tidy</button>
        <button type="button" class="snap-btn" data-snap="1">Fine</button>
      </div>
      <span class="editor-status" id="editor-status"></span>
    </div>
  </div>
  <script src="/common.js"></script>
  <script src="/editor.js"></script>
</body>
</html>


]]></file>
<file id="f03fe8e0" path="firmware/data-src/editor.js" label="SOURCE CODE"><![CDATA[

/* FlightWall Layout Editor (LE-1.6) - ES5 only */
(function() {
  'use strict';

  /* ---------- Module globals ---------- */
  var SCALE = 4;                /* CSS pixels per logical pixel */
  var SNAP = 8;                 /* current snap grid: 8 / 16 / 1 */
  var deviceW = 0;              /* logical canvas width (from /api/layout) */
  var deviceH = 0;              /* logical canvas height */
  var tilePixels = 16;          /* tile size in logical pixels */
  var widgets = [];             /* [{type,x,y,w,h,color,content,align,id}] */
  var selectedIndex = -1;       /* -1 = no selection */
  var dragState = null;         /* {mode,startX,startY,origX,origY,origW,origH,toastedFloor} */
  var widgetTypeMeta = {};      /* keyed by type string */
  var HANDLE_SIZE = 6;          /* CSS pixels for resize handle */

  /* ---------- Utilities ---------- */
  function snapTo(val, grid) {
    return Math.round(val / grid) * grid;
  }

  function clamp(val, lo, hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
  }

  function getCanvasPos(e) {
    var canvas = document.getElementById('editor-canvas');
    var rect = canvas.getBoundingClientRect();
    return { x: e.clientX - rect.left, y: e.clientY - rect.top };
  }

  function setStatus(msg) {
    var el = document.getElementById('editor-status');
    if (el) el.textContent = msg || '';
  }

  /* ---------- Render ---------- */
  function render() {
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    var ctx = canvas.getContext('2d');
    ctx.imageSmoothingEnabled = false;
    /* 1. Clear */
    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    /* 2. Tile grid */
    drawGrid(ctx);
    /* 3. Widgets */
    for (var i = 0; i < widgets.length; i++) {
      drawWidget(ctx, widgets[i], i === selectedIndex);
    }
  }

  function drawGrid(ctx) {
    var canvas = ctx.canvas;
    var step = tilePixels * SCALE;
    if (step <= 0) return;
    ctx.strokeStyle = 'rgba(255,255,255,0.1)';
    ctx.lineWidth = 1;
    var x;
    for (x = step; x < canvas.width; x += step) {
      ctx.beginPath();
      ctx.moveTo(x + 0.5, 0);
      ctx.lineTo(x + 0.5, canvas.height);
      ctx.stroke();
    }
    var y;
    for (y = step; y < canvas.height; y += step) {
      ctx.beginPath();
      ctx.moveTo(0, y + 0.5);
      ctx.lineTo(canvas.width, y + 0.5);
      ctx.stroke();
    }
  }

  function drawWidget(ctx, w, isSelected) {
    var x = w.x * SCALE;
    var y = w.y * SCALE;
    var cw = w.w * SCALE;
    var ch = w.h * SCALE;
    /* Body fill at 80% opacity */
    ctx.globalAlpha = 0.8;
    ctx.fillStyle = w.color || '#444';
    ctx.fillRect(x, y, cw, ch);
    ctx.globalAlpha = 1.0;
    /* Type label */
    ctx.fillStyle = '#FFF';
    ctx.font = (SCALE * 6) + 'px monospace';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(w.type, x + cw / 2, y + ch / 2);
    /* Selection outline + resize handle */
    if (isSelected) {
      ctx.strokeStyle = '#FFF';
      ctx.lineWidth = 1;
      ctx.strokeRect(x + 0.5, y + 0.5, cw - 1, ch - 1);
      ctx.fillStyle = '#FFF';
      ctx.fillRect(x + cw - HANDLE_SIZE, y + ch - HANDLE_SIZE, HANDLE_SIZE, HANDLE_SIZE);
    }
  }

  /* ---------- Canvas init ---------- */
  function initCanvas(matrixW, matrixH, tilePx) {
    deviceW = matrixW;
    deviceH = matrixH;
    tilePixels = tilePx || 16;
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    canvas.width = matrixW * SCALE;
    canvas.height = matrixH * SCALE;
    canvas.style.width = (matrixW * SCALE) + 'px';
    canvas.style.height = (matrixH * SCALE) + 'px';
    render();
  }

  /* ---------- Hit testing ---------- */
  function hitTest(cx, cy) {
    /* 1. Resize handle of selected widget */
    if (selectedIndex >= 0 && selectedIndex < widgets.length) {
      var sw = widgets[selectedIndex];
      var rx = sw.x * SCALE + sw.w * SCALE - HANDLE_SIZE;
      var ry = sw.y * SCALE + sw.h * SCALE - HANDLE_SIZE;
      if (cx >= rx && cx <= rx + HANDLE_SIZE && cy >= ry && cy <= ry + HANDLE_SIZE) {
        return { index: selectedIndex, mode: 'resize' };
      }
    }
    /* 2. Widget body (topmost first) */
    for (var i = widgets.length - 1; i >= 0; i--) {
      var w = widgets[i];
      var x0 = w.x * SCALE;
      var y0 = w.y * SCALE;
      var x1 = (w.x + w.w) * SCALE;
      var y1 = (w.y + w.h) * SCALE;
      if (cx >= x0 && cx <= x1 && cy >= y0 && cy <= y1) {
        return { index: i, mode: 'move' };
      }
    }
    return null;
  }

  /* ---------- Pointer events ---------- */
  function onPointerDown(e) {
    var canvas = document.getElementById('editor-canvas');
    var p = getCanvasPos(e);
    var hit = hitTest(p.x, p.y);
    if (hit) {
      selectedIndex = hit.index;
      var w = widgets[selectedIndex];
      dragState = {
        mode: hit.mode,
        startX: e.clientX,
        startY: e.clientY,
        origX: w.x,
        origY: w.y,
        origW: w.w,
        origH: w.h,
        toastedFloor: false
      };
      try { canvas.setPointerCapture(e.pointerId); } catch (err) { /* ignore */ }
      e.preventDefault();
    } else {
      selectedIndex = -1;
      dragState = null;
    }
    render();
  }

  function onPointerMove(e) {
    if (dragState === null) return;
    var w = widgets[selectedIndex];
    if (!w) { dragState = null; return; }
    var dxCss = e.clientX - dragState.startX;
    var dyCss = e.clientY - dragState.startY;
    var dx = dxCss / SCALE;
    var dy = dyCss / SCALE;

    if (dragState.mode === 'move') {
      var nx = snapTo(dragState.origX + dx, SNAP);
      var ny = snapTo(dragState.origY + dy, SNAP);
      w.x = clamp(nx, 0, deviceW - w.w);
      w.y = clamp(ny, 0, deviceH - w.h);
    } else if (dragState.mode === 'resize') {
      var meta = widgetTypeMeta[w.type] || { minW: 6, minH: 6 };
      var rawW = snapTo(dragState.origW + dx, SNAP);
      var rawH = snapTo(dragState.origH + dy, SNAP);
      var maxW = deviceW - w.x;
      var maxH = deviceH - w.y;
      var newW = clamp(rawW, meta.minW, maxW);
      var newH = clamp(rawH, meta.minH, maxH);
      if ((rawW < meta.minW || rawH < meta.minH) && !dragState.toastedFloor) {
        if (typeof FW !== 'undefined' && FW.showToast) {
          FW.showToast(w.type + ' minimum: ' + meta.minW + 'w \u00d7 ' + meta.minH + 'h px', 'warning');
        }
        dragState.toastedFloor = true;
      }
      w.w = newW;
      w.h = newH;
    }
    render();
    setStatus(w.type + ' @ ' + w.x + ',' + w.y + ' ' + w.w + 'x' + w.h);
  }

  function onPointerUp(e) {
    if (dragState === null) return;
    /* Release pointer capture before clearing dragState (spec-correct order) */
    var canvas = document.getElementById('editor-canvas');
    if (canvas && e && e.pointerId != null) {
      try { canvas.releasePointerCapture(e.pointerId); } catch (err) { /* ignore */ }
    }
    dragState = null;
    if (selectedIndex >= 0 && selectedIndex < widgets.length) {
      var w = widgets[selectedIndex];
      setStatus(w.type + ' @ ' + w.x + ',' + w.y + ' ' + w.w + 'x' + w.h);
    }
  }

  /* ---------- Toolbox ---------- */
  function buildWidgetTypeMeta(entry) {
    var meta = {
      defaultW: 32, defaultH: 8, defaultColor: '#FFFFFF',
      defaultContent: '', defaultAlign: 'left',
      minW: 6, minH: 6
    };
    if (entry.fields) {
      for (var i = 0; i < entry.fields.length; i++) {
        var f = entry.fields[i];
        if (f.key === 'w')       meta.defaultW = (f['default'] != null) ? f['default'] : 32;
        if (f.key === 'h')       meta.defaultH = (f['default'] != null) ? f['default'] : 8;
        if (f.key === 'color')   meta.defaultColor = f['default'] || '#FFFFFF';
        if (f.key === 'content') meta.defaultContent = (f['default'] != null) ? f['default'] : '';
        if (f.key === 'align')   meta.defaultAlign = f['default'] || 'left';
      }
    }
    /* Min floors: firmware enforces w<8||h<8 early-return; use h.default as floor */
    meta.minH = meta.defaultH;
    meta.minW = 6;
    widgetTypeMeta[entry.type] = meta;
  }

  function isDisabledEntry(entry) {
    return !!(entry.note && entry.note.indexOf('not yet implemented') !== -1);
  }

  function initToolbox(types) {
    var box = document.getElementById('editor-toolbox');
    if (!box) return;
    box.innerHTML = '';
    for (var i = 0; i < types.length; i++) {
      buildWidgetTypeMeta(types[i]);
      var entry = types[i];
      var item = document.createElement('div');
      item.className = 'toolbox-item';
      if (isDisabledEntry(entry)) item.className += ' disabled';
      item.setAttribute('data-type', entry.type);
      item.textContent = entry.label || entry.type;
      box.appendChild(item);
    }
    box.addEventListener('click', onToolboxClick);
  }

  function onToolboxClick(e) {
    var t = e.target;
    if (!t || !t.classList || !t.classList.contains('toolbox-item')) return;
    if (t.classList.contains('disabled')) return;
    var type = t.getAttribute('data-type');
    if (type) addWidget(type);
  }

  function addWidget(type) {
    var meta = widgetTypeMeta[type];
    if (!meta) return;
    var w = {
      type: type,
      x: snapTo(8, SNAP),
      y: snapTo(8, SNAP),
      w: meta.defaultW,
      h: meta.defaultH,
      color: meta.defaultColor || '#FFFFFF',
      content: meta.defaultContent || '',
      align: meta.defaultAlign || 'left',
      id: 'w' + Date.now()
    };
    /* Clamp dimensions to canvas bounds (defensive: guards future widget types) */
    if (deviceW > 0 && w.w > deviceW) w.w = Math.max(meta.minW || 6, deviceW);
    if (deviceH > 0 && w.h > deviceH) w.h = Math.max(meta.minH || 6, deviceH);
    /* Clamp position so widget stays within canvas */
    if (w.x + w.w > deviceW) w.x = Math.max(0, deviceW - w.w);
    if (w.y + w.h > deviceH) w.y = Math.max(0, deviceH - w.h);
    widgets.push(w);
    selectedIndex = widgets.length - 1;
    render();
    setStatus('Added ' + type);
  }

  /* ---------- Snap control ---------- */
  function updateSnapButtons() {
    var btns = document.querySelectorAll('#snap-control .snap-btn');
    for (var i = 0; i < btns.length; i++) {
      var b = btns[i];
      var v = parseInt(b.getAttribute('data-snap'), 10);
      if (v === SNAP) {
        b.className = 'snap-btn active';
      } else {
        b.className = 'snap-btn';
      }
    }
  }

  function initSnap() {
    var stored = null;
    try { stored = window.localStorage.getItem('fw_editor_snap'); } catch (err) { /* ignore */ }
    var v = parseInt(stored, 10);
    if (v === 1 || v === 8 || v === 16) {
      SNAP = v;
    } else {
      SNAP = 8;
    }
    var ctrl = document.getElementById('snap-control');
    if (ctrl) {
      ctrl.addEventListener('click', function(e) {
        var t = e.target;
        if (!t || !t.classList || !t.classList.contains('snap-btn')) return;
        var snapVal = parseInt(t.getAttribute('data-snap'), 10);
        if (snapVal === 1 || snapVal === 8 || snapVal === 16) {
          SNAP = snapVal;
          try { window.localStorage.setItem('fw_editor_snap', String(SNAP)); } catch (err) { /* ignore */ }
          updateSnapButtons();
          render();
        }
      });
    }
    updateSnapButtons();
  }

  /* ---------- Init / load ---------- */
  function loadLayout() {
    FW.get('/api/layout').then(function(res) {
      if (!res.body || !res.body.ok) {
        FW.showToast('Cannot load layout info \u2014 check device connection', 'error');
        return null;
      }
      var d = res.body.data;
      /* Guard against malformed API response before destructuring nested fields */
      if (!d || !d.matrix || typeof d.matrix.width !== 'number' ||
          typeof d.matrix.height !== 'number' || !d.hardware ||
          typeof d.hardware.tile_pixels !== 'number') {
        FW.showToast('Invalid layout data \u2014 check device firmware', 'error');
        return null;
      }
      initCanvas(d.matrix.width, d.matrix.height, d.hardware.tile_pixels);
      return FW.get('/api/widgets/types');
    }).then(function(res) {
      if (!res || !res.body || !res.body.ok) return;
      var types = res.body.data;
      /* Guard against non-array response before iterating */
      if (!types || typeof types.length !== 'number') return;
      initToolbox(types);
      updateSnapButtons();
    }).catch(function(err) {
      /* Network transport failure (fetch rejected) — show error toast */
      FW.showToast('Editor failed to load \u2014 check device connection', 'error');
    });
  }

  function bindCanvasEvents() {
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    canvas.addEventListener('pointerdown', onPointerDown);
    canvas.addEventListener('pointermove', onPointerMove);
    canvas.addEventListener('pointerup', onPointerUp);
    canvas.addEventListener('pointercancel', onPointerUp);
  }

  function init() {
    bindCanvasEvents();
    initSnap();
    loadLayout();
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();


]]></file>
<file id="a3910ae1" path="firmware/modes/CustomLayoutMode.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: Production CustomLayoutMode implementation (Story le-1.3).

Loads an active user-authored layout from LayoutStore on init(), parses the
JSON into a fixed WidgetSpec[] array (ArduinoJson v7 JsonDocument is scoped
to parseFromJson() so it's freed before render() ever runs), then dispatches
per-widget renders via WidgetRegistry::dispatch() with zero heap activity on
the render hot path.

Non-fatal error handling:
  - LayoutStore::load() returning false is NOT fatal — its `out` string is
    already populated with the baked-in default layout JSON. We parse that
    and succeed.
  - deserializeJson() failure IS fatal for init() — we set _invalid = true
    and return false. render() then draws a visible error indicator.

Parse-once rule (epic LE-1 Architecture): deserializeJson() lives strictly
inside parseFromJson() and the JsonDocument is destroyed before the function
returns. render() never sees JSON.
*/

#include "modes/CustomLayoutMode.h"

#include "core/LayoutStore.h"
#include "utils/DisplayUtils.h"
#include "utils/Log.h"

#include <ArduinoJson.h>
#include <FastLED_NeoMatrix.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Guard against accidental future downward drift of the heap budget
// (Task 5). Must be evaluated after WidgetSpec is a complete type.
static_assert(CustomLayoutMode::MEMORY_REQUIREMENT
              >= CustomLayoutMode::MAX_WIDGETS * sizeof(WidgetSpec),
              "CustomLayoutMode::MEMORY_REQUIREMENT must cover the full "
              "WidgetSpec array");

// ------------------------------------------------------------------
// Zone descriptor — CustomLayoutMode fills the whole canvas with a
// layout the user authors; no fixed zones.
// ------------------------------------------------------------------
static const ZoneRegion kCustomZones[] = {
    {"Custom", 0, 0, 100, 100}
};

const ModeZoneDescriptor CustomLayoutMode::_descriptor = {
    "Renders a user-authored layout (widgets) loaded from LittleFS.",
    kCustomZones,
    1
};

// ------------------------------------------------------------------
// Helpers — local-linkage so the translation unit stays self-contained.
// ------------------------------------------------------------------
namespace {

// Pack r, g, b (0-255) into RGB565 using the Adafruit GFX convention
// (RRRRR GGGGGG BBBBB).
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8)
         | ((uint16_t)(g & 0xFC) << 3)
         | ((uint16_t)(b & 0xF8) >> 3);
}

// Parse a "#RRGGBB" hex color string into RGB565. Returns white (0xFFFF)
// for null / malformed input so an authoring mistake does not hide the
// widget entirely. Accepts exact 6 hex digits after the '#'.
uint16_t parseHexColor(const char* hex) {
    if (hex == nullptr || hex[0] != '#') return 0xFFFF;
    // Require exactly "#RRGGBB".
    if (strlen(hex) != 7) return 0xFFFF;
    for (int i = 1; i < 7; ++i) {
        char c = hex[i];
        bool okHex = (c >= '0' && c <= '9')
                  || (c >= 'a' && c <= 'f')
                  || (c >= 'A' && c <= 'F');
        if (!okHex) return 0xFFFF;
    }
    char* endptr = nullptr;
    long v = strtol(hex + 1, &endptr, 16);
    if (endptr == nullptr || *endptr != '\0') return 0xFFFF;
    uint8_t r = (uint8_t)((v >> 16) & 0xFF);
    uint8_t g = (uint8_t)((v >>  8) & 0xFF);
    uint8_t b = (uint8_t)((v >>  0) & 0xFF);
    return rgb565(r, g, b);
}

// Map the JSON "align" string to WidgetSpec.align byte encoding.
uint8_t parseAlign(const char* a) {
    if (a == nullptr) return 0;
    if (strcmp(a, "center") == 0) return 1;
    if (strcmp(a, "right")  == 0) return 2;
    return 0;  // "left" or anything else → 0
}

// Copy a null-terminated string into a fixed destination buffer, always
// leaving a null terminator. Used for the id[] and content[] fields.
void copyFixed(char* dst, size_t dstLen, const char* src) {
    if (dstLen == 0) return;
    if (src == nullptr) { dst[0] = '\0'; return; }
    strncpy(dst, src, dstLen - 1);
    dst[dstLen - 1] = '\0';
}

}  // namespace

// ------------------------------------------------------------------
// init / teardown
// ------------------------------------------------------------------

bool CustomLayoutMode::init(const RenderContext& ctx) {
    (void)ctx;
    _widgetCount = 0;
    _invalid     = false;

    String json;
    bool found = LayoutStore::load(LayoutStore::getActiveId().c_str(), json);
    // NOTE: load() returning false is non-fatal. LayoutStore always populates
    // `json` — with the baked-in default layout on miss. Only a
    // deserializeJson() failure in parseFromJson() is treated as fatal.
    (void)found;

    if (!parseFromJson(json)) {
        _invalid = true;
        Serial.printf("[CustomLayoutMode] init: parse failed — error indicator will render\n");
        // Runtime heap log (Task 5).
        Serial.printf("[CustomLayoutMode] Free heap after init: %u\n", ESP.getFreeHeap());
        return false;
    }

    Serial.printf("[CustomLayoutMode] init: %u widgets loaded (found=%d)\n",
                  (unsigned)_widgetCount, (int)found);
    Serial.printf("[CustomLayoutMode] Free heap after init: %u\n", ESP.getFreeHeap());
    return true;
}

void CustomLayoutMode::teardown() {
    _widgetCount = 0;
    _invalid     = false;
    // _widgets[] contents left as-is — will be overwritten by the next init().
}

// ------------------------------------------------------------------
// parseFromJson — scoped JsonDocument; freed on return.
// ------------------------------------------------------------------

bool CustomLayoutMode::parseFromJson(const String& json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[CustomLayoutMode] parse failed: %s\n", err.c_str());
        return false;
    }

    JsonArrayConst arr = doc["widgets"].as<JsonArrayConst>();
    _widgetCount = 0;
    for (JsonObjectConst obj : arr) {
        if (_widgetCount >= MAX_WIDGETS) {
            Serial.printf("[CustomLayoutMode] widget cap %u hit — truncating\n",
                          (unsigned)MAX_WIDGETS);
            break;
        }

        const char* typeStr = obj["type"] | (const char*)nullptr;
        WidgetType t = WidgetRegistry::fromString(typeStr);
        if (t == WidgetType::Unknown) {
            Serial.printf("[CustomLayoutMode] unknown widget type '%s' — skipping\n",
                          typeStr ? typeStr : "?");
            continue;
        }

        WidgetSpec& w = _widgets[_widgetCount];
        w.type  = t;
        w.x     = (int16_t)(obj["x"] | 0);
        w.y     = (int16_t)(obj["y"] | 0);
        w.w     = (uint16_t)(obj["w"] | 0);
        w.h     = (uint16_t)(obj["h"] | 0);
        w.color = parseHexColor(obj["color"] | (const char*)nullptr);
        w.align = parseAlign(obj["align"] | (const char*)nullptr);
        w._reserved = 0;

        copyFixed(w.id,      sizeof(w.id),      obj["id"]      | (const char*)"");
        copyFixed(w.content, sizeof(w.content), obj["content"] | (const char*)"");

        _widgetCount++;
    }

    return true;
    // doc destructs here — no retained JSON state (epic parse-once rule).
}

// ------------------------------------------------------------------
// render — zero heap per frame; error indicator on _invalid.
// ------------------------------------------------------------------

void CustomLayoutMode::render(const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights) {
    (void)flights;
    if (ctx.matrix == nullptr) return;

    // Clear the canvas once per frame (pipeline owns the final show()).
    ctx.matrix->fillScreen(0);

    if (_invalid) {
        // AC #5 — draw a visible 1px red border + "LAYOUT ERR" text so a
        // broken layout is immediately obvious on the physical wall.
        const uint16_t red = rgb565(255, 0, 0);
        const int16_t maxX = ctx.matrix->width();
        const int16_t maxY = ctx.matrix->height();
        if (maxX > 0 && maxY > 0) {
            ctx.matrix->drawRect(0, 0, maxX, maxY, red);
            DisplayUtils::drawTextLine(ctx.matrix, 2, 2, "LAYOUT ERR", red);
        }
        return;
    }

    for (size_t i = 0; i < _widgetCount; ++i) {
        const WidgetSpec& w = _widgets[i];
        // Dispatch returns false for unimplemented/unknown types — we
        // intentionally ignore that here; the dispatcher is stateless and
        // never throws.
        (void)WidgetRegistry::dispatch(w.type, w, ctx);
    }
    // No FastLED.show() — pipeline owns frame commit (FR35).
}

// ------------------------------------------------------------------
// Metadata
// ------------------------------------------------------------------

const char* CustomLayoutMode::getName() const {
    return "Custom Layout";
}

const ModeZoneDescriptor& CustomLayoutMode::getZoneDescriptor() const {
    return _descriptor;
}

const ModeSettingsSchema* CustomLayoutMode::getSettingsSchema() const {
    return nullptr;
}


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="Architecture (fallback - epics file should have relevant sections)" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-17</var>
<var name="default_output_file">_bmad-output/implementation-artifacts/stories/le-1-7-editor-property-panel-and-save-ux.md</var>
<var name="description">Create the next user story from epics+stories with enhanced context analysis and direct ready-for-dev marking</var>
<var name="document_output_language">English</var>
<var name="epic_num">le-1</var>
<var name="epics_file" file_id="d7cf6d0a" description="Enhanced epics+stories file with BDD and source hints" load_strategy="EMBEDDED" sharded="true" token_approx="55">embedded in prompt, file id: d7cf6d0a</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="name">create-story</var>
<var name="output_folder">_bmad-output/implementation-artifacts</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="prd_file" description="PRD (fallback - epics file should have most content)" load_strategy="SELECTIVE_LOAD" token_approx="3576">_bmad-output/planning-artifacts/prd-delight-validation-report.md</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="712">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_dir">_bmad-output/implementation-artifacts/stories</var>
<var name="story_id">le-1.7</var>
<var name="story_key">le-1-7-editor-property-panel-and-save-ux</var>
<var name="story_num">7</var>
<var name="story_title">editor-property-panel-and-save-ux</var>
<var name="timestamp">20260417_1618</var>
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

  <step n="1" goal="Load and analyze core artifacts">
    <critical>🔬 EXHAUSTIVE ARTIFACT ANALYSIS - This is where you prevent future developer fuckups!</critical>

    <!-- Load all available content through discovery protocol -->
    <invoke-protocol name="discover_inputs" />
    <note>Available content: {epics_content}, {prd_content}, {architecture_content}, {ux_content},
    {'_value': '/Users/christianlee/App-Development/TheFlightWall_OSS-main/docs/project-context.md', '_token_approx': '712'}</note>

    <!-- Analyze epics file for story foundation -->
    <action>From {epics_content}, extract Epic le-1 complete context:</action> **EPIC ANALYSIS:** - Epic
    objectives and business value - ALL stories in this epic for cross-story context - Our specific story's requirements, user story
    statement, acceptance criteria - Technical requirements and constraints - Dependencies on other stories/epics - Source hints pointing to
    original documents <!-- Extract specific story requirements -->
    <action>Extract our story (le-1-7) details:</action> **STORY FOUNDATION:** - User story statement
    (As a, I want, so that) - Detailed acceptance criteria (already BDD formatted) - Technical requirements specific to this story -
    Business context and value - Success criteria <!-- Previous story analysis for context continuity -->
    <check if="story_num &gt; 1">
      <action>Load previous story file: /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/le-1-{{previous_story_num}}-*.md</action> **PREVIOUS STORY INTELLIGENCE:** -
    Dev notes and learnings from previous story - Review feedback and corrections needed - Files that were created/modified and their
    patterns - Testing approaches that worked/didn't work - Problems encountered and solutions found - Code patterns established <action>Extract
    all learnings that could impact current story implementation</action>
    </check>

    <!-- Git intelligence from embedded data -->
    <check if="previous story exists AND git-intelligence data is available">
      <action>Analyze embedded &lt;git-intelligence&gt; data for relevance to current story:
        - Files created/modified
        - Code patterns and conventions used
        - Library dependencies added/changed
        - Architecture decisions implemented
        - Testing approaches used
      </action>
      <action>Extract actionable insights for current story implementation</action>
    </check>
  </step>

  <step n="2" goal="Architecture analysis for developer guardrails">
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

  <step n="3" goal="Web research for latest technical specifics">
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

  <step n="4" goal="Create comprehensive story file">
    <critical>📝 CREATE ULTIMATE STORY FILE - The developer's master implementation guide!</critical>

    <action>Create file using the output-template format at: /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/le-1-7-editor-property-panel-and-save-ux.md</action>
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

  <step n="5" goal="Validate and finalize">
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