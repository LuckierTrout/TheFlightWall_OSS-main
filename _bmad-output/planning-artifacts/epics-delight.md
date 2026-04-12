---
stepsCompleted: ['step-01', 'step-02', 'step-03', 'step-04']
inputDocuments: ['_bmad-output/planning-artifacts/prd-delight.md', '_bmad-output/planning-artifacts/architecture.md']
---

# TheFlightWall_OSS-main — Delight Release Epic Breakdown

## Overview

This document provides the complete epic and story breakdown for the TheFlightWall Delight Release, decomposing the requirements from the PRD and Architecture decisions into implementable stories.

## Requirements Inventory

### Functional Requirements

**Clock Mode**
- FR1: The system can display the current time on the LED matrix in a large-format layout
- FR2: The owner can select between 12-hour and 24-hour time format for Clock Mode
- FR3: The system can automatically activate Clock Mode when the flight data pipeline returns zero aircraft in range (idle fallback)
- FR4: The system can exit Clock Mode idle fallback and restore the previously active mode when flight data becomes available

**Departures Board**
- FR5: The system can display multiple flights simultaneously in a list format on the LED matrix
- FR6: The system can show, for each Departures Board row, the callsign and at least two telemetry fields chosen from the same owner-configurable field set as Live Flight Card / Mode Picker (default: callsign, altitude, ground speed)
- FR7: The system can add a row when a new flight enters tracking range
- FR8: The system can remove a row when a flight leaves tracking range
- FR9: The owner can configure the maximum number of visible rows on the Departures Board

**Mode Transitions**
- FR10: The system can perform animated visual transitions when switching between display modes
- FR11: The system can execute a fade transition between any two display modes
- FR12: The system can complete mode transitions without visible tearing or artifacts on the LED matrix

**Mode Scheduling**
- FR13: The owner can define time-based rules that automatically switch between display modes
- FR14: The system can execute scheduled mode switches at the configured times
- FR15: The system can operate the mode scheduler independently from the night mode brightness scheduler
- FR16: The owner can edit and delete existing schedule rules through the dashboard
- FR17: The system can resolve conflicts between scheduled mode switches and data-driven idle fallback, with the explicit schedule taking priority
- FR36: The system can persist schedule rules across power cycles and reboots
- FR39: The owner can view all configured schedule rules and their current status

**Mode Configuration**
- FR18: The owner can view and modify settings specific to each display mode through the Mode Picker
- FR19: The system can apply mode-specific setting changes without requiring a device reboot
- FR20: The system can persist mode-specific settings across power cycles and reboots
- FR40: The owner can manually switch between available display modes from the dashboard

**OTA Firmware Updates**
- FR21: The owner can check for available firmware updates from the dashboard
- FR22: The system can compare the installed firmware version against the latest version available on GitHub Releases
- FR23: The owner can view release notes for an available update before choosing to install
- FR24: The owner can initiate a firmware update with a single action from the dashboard
- FR25: The system can download and install firmware updates over WiFi without physical access to the device
- FR26: The system can display update progress on the LED matrix during the download and install process
- FR27: The system can verify firmware integrity before activating a new version
- FR28: The system can automatically revert to the previous firmware version if a new version fails to boot
- FR29: The system can preserve all user settings and configuration across firmware updates
- FR30: When a firmware update fails, the dashboard shows a status message stating (a) failure phase (download/verify/boot), (b) outcome (retriable or rolled back), (c) recommended next action
- FR31: The system can handle interrupted downloads without affecting the currently running firmware
- FR37: The owner can retry a failed firmware update without restarting the device
- FR38: The system can display a notification in the dashboard when a newer firmware version is available

**System Resilience**
- FR32: The system can recover from an unresponsive display mode by rebooting into a known-good default mode
- FR33: When upgrading from a prior firmware version, the system can supply safe default values for any Delight-introduced config keys on first read without overwriting existing stored keys
- FR34: Before starting an OTA firmware download, the system verifies at least 80KB free heap; before starting a mode transition, the system verifies buffer allocation can succeed or applies fallback
- FR35: When OpenSky or AeroAPI is unavailable, the device remains responsive with last flight snapshot or empty-flight rules; when GitHub Releases is unavailable, OTA shows unavailable state; no blank LED matrix for more than 1 second except during intentional OTA progress

**Contributor Extensibility**
- FR41: A developer can add a new display mode by implementing the documented DisplayMode lifecycle (setup, loop, teardown, metadata/settings schema) and registering in the compile-time mode registry, without modifying core source files beyond the registry hook
- FR42: The dashboard Mode Picker lists every compile-time registered display mode for owner selection, with manual switching and scheduling integration

**Documentation & Onboarding**
- FR43: The repository README links to current setup and device-flash guides for self-service onboarding (Journey 0)

### NonFunctional Requirements

**Performance**
- NFR1: Mode transitions render at minimum 15fps on the 160x32 LED matrix
- NFR2: Mode transitions complete in under 1 second
- NFR3: Clock Mode idle fallback activates within one flight-pipeline poll interval (~30s) of zero flights in range
- NFR4: Departures Board row additions/removals appear within 50ms of flight list change (one frame at ≥20fps)
- NFR5: OTA firmware download, verification, and reboot complete in under 60 seconds on 10Mbps WiFi
- NFR6: Mode schedule triggers fire within 5 seconds of the configured time
- NFR7: Dashboard pages load within 1 second on local network
- NFR8: Mode-specific setting changes apply to the display within 2 seconds of submission

**Reliability**
- NFR9: The device operates continuously for minimum 30 days without manual intervention
- NFR10: Watchdog timer reboots the device within 10 seconds of detecting an unresponsive main loop
- NFR11: After a watchdog reboot, the system restarts in a known-good default mode
- NFR12: OTA update failure never renders the device unbootable — previous firmware partition remains available
- NFR13: Peak heap usage of any single active DisplayMode plus shared overhead stays under ~320KB usable heap ceiling
- NFR14: Firmware image fits within 2,097,152 bytes (2 MiB OTA slot)
- NFR15: Configuration data (NVS) survives firmware updates, power cycles, and watchdog reboots
- NFR16: System maintains minimum 80KB free heap during normal operation for OTA readiness
- NFR17: No progressive heap degradation — free heap after 24 hours within 5% of boot value

**Integration**
- NFR18: When OpenSky/AeroAPI unavailable: active display continues with last-known data or Clock idle fallback; watchdog does not trigger. When GitHub Releases unavailable: OTA check degrades, flight rendering unchanged
- NFR19: For each integration failure, dashboard shows non-empty status line naming affected integration; LED matrix not blank >1 second except during OTA progress; no crash-loop
- NFR20: GitHub Releases API rate limit exhaustion (HTTP 429) results in "try again later" message with no automatic retry loop
- NFR21: System tolerates network latency up to 10 seconds on API calls without triggering watchdog recovery

### Additional Requirements

**From Architecture — Delight Decisions (DL1-DL6):**

- AR1: Fade transition implemented as `ModeRegistry::_executeFadeTransition()` private method with transient RGB888 dual buffers (~30KB total, 160x32x3 bytes per buffer). Buffers `malloc()`'d at transition start, `free()`'d immediately after last blend frame. Allocation failure = instant cut (graceful degradation, not error)
- AR2: Mode orchestration via `core/ModeOrchestrator` static class with three states: MANUAL, SCHEDULED, IDLE_FALLBACK. Priority: SCHEDULED > IDLE_FALLBACK > MANUAL. `ModeRegistry::requestSwitch()` called from exactly two methods: `ModeOrchestrator::tick()` and `ModeOrchestrator::onManualSwitch()` — no other call sites (rule 24)
- AR3: `core/OTAUpdater` static class — GitHub Releases API client with incremental SHA-256 verification via `mbedtls_sha256`. Download runs in one-shot FreeRTOS task (Core 1, 8KB stack). Shared pre-OTA sequence: `ModeRegistry::prepareForOTA()` tears down active mode and sets `_switchState = SWITCHING` (rule 26)
- AR4: Schedule rules stored as indexed NVS keys: `sched_r{N}_start`, `sched_r{N}_end`, `sched_r{N}_mode`, `sched_r{N}_ena` + `sched_r_count`. Fixed max 8 rules. Rules always compacted (no index gaps on delete). Minutes-since-midnight convention matching Foundation brightness scheduler
- AR5: Per-mode settings schema via `ModeSettingDef[]` + `ModeSettingsSchema` declared as `static const` in each mode's `.h` file (rule 29). NVS key format: `m_{abbrev}_{setting}` where abbrev ≤5 chars, setting ≤7 chars. Read/write exclusively through `ConfigManager::getModeSetting()`/`setModeSetting()` (rule 28)
- AR6: 5 new API endpoints: `GET /api/ota/check`, `POST /api/ota/pull`, `GET /api/ota/status`, `GET /api/schedule`, `POST /api/schedule`. 3 updated endpoints: `GET /api/display/modes` (+settings), `POST /api/display/mode` (+settings, routes through `onManualSwitch()`), `GET /api/status` (+ota_available, +orchestrator_state)
- AR7: `std::atomic<uint8_t> g_flightCount` declared in `main.cpp` only (rule 30). Written by FlightDataFetcher after queue write, read by ModeOrchestrator. Modes and adapters must not access atomic globals directly
- AR8: `ModeEntry` struct expanded with nullable `settingsSchema` field. Existing modes (ClassicCard, LiveFlightCard) set `nullptr`. API handlers must iterate `settingsSchema` dynamically — never hardcode setting names (rule 28)
- AR9: OTA SHA-256 computed incrementally during streaming — never post-download read-back. On any error after `Update.begin()`, always `Update.abort()` (rule 25). SHA hash expected as plain 64-char hex string from companion `.sha256` release asset
- AR10: `ModeRegistry::prepareForOTA()` shared between Foundation Push and Delight Pull OTA paths. Must be called before `Update.begin()` for both paths (rule 26)

**From Architecture — New Files:**
- 8 new source files: `core/ModeOrchestrator.h/.cpp`, `core/OTAUpdater.h/.cpp`, `modes/ClockMode.h/.cpp`, `modes/DeparturesBoardMode.h/.cpp`
- 3 new test files: `test_mode_orchestrator`, `test_ota_updater`, `test_config_schedule`
- 14 updated files including ConfigManager, ModeRegistry, WebPortal, main.cpp, DisplayMode.h, dashboard assets
- 3 gzip rebuilds: `dashboard.html.gz`, `dashboard.js.gz`, `style.css.gz`

**From Architecture — Implementation Sequence:**
- Sequential foundation: ConfigManager expansion → ModeEntry expansion
- Mode implementations: ClockMode → DeparturesBoardMode
- Orchestration track: ModeOrchestrator → Fade transition
- OTA track (independent after foundation): OTAUpdater → prepareForOTA()
- API + UI last: WebPortal endpoints → Dashboard UI → Gzip rebuild

### UX Design Requirements

**Primary artifact:** [`ux-design-specification-delight.md`](ux-design-specification-delight.md) — scheduling UI, per-mode settings, OTA Pull flows, and LED-matrix behaviors for Delight. Stories should align with that spec; it extends Foundation and Display System patterns (same dashboard stack, no third-party script additions per architecture) rather than replacing them.

### FR Coverage Map

| FR | Epic | Description |
|----|------|-------------|
| FR1 | Epic 1 | Clock Mode time display |
| FR2 | Epic 1 | 12h/24h format selection |
| FR3 | Epic 1 | Automatic idle fallback |
| FR4 | Epic 1 | Exit idle fallback on flight data |
| FR5 | Epic 2 | Multi-flight list display |
| FR6 | Epic 2 | Row callsign + telemetry fields |
| FR7 | Epic 2 | Dynamic row addition |
| FR8 | Epic 2 | Dynamic row removal |
| FR9 | Epic 2 | Configurable max rows |
| FR10 | Epic 3 | Animated mode transitions |
| FR11 | Epic 3 | Fade transition |
| FR12 | Epic 3 | No tearing/artifacts |
| FR13 | Epic 4 | Time-based scheduling rules |
| FR14 | Epic 4 | Scheduled mode execution |
| FR15 | Epic 4 | Independent from night mode |
| FR16 | Epic 4 | Edit/delete schedule rules |
| FR17 | Epic 4 | Schedule vs. idle-fallback priority |
| FR18 | Epic 5 | Per-mode settings in Mode Picker |
| FR19 | Epic 5 | Hot-reload mode settings |
| FR20 | Epic 5 | Persist mode settings |
| FR21 | Epic 6 | Check for updates from dashboard |
| FR22 | Epic 6 | Version comparison |
| FR23 | Epic 6 | View release notes |
| FR24 | Epic 7 | One-action update initiation |
| FR25 | Epic 7 | WiFi firmware update |
| FR26 | Epic 7 | LED progress display |
| FR27 | Epic 7 | Firmware integrity verification |
| FR28 | Epic 7 | Auto-revert on boot failure |
| FR29 | Epic 7 | Config preservation across updates |
| FR30 | Epic 7 | Failure status messages |
| FR31 | Epic 7 | Handle interrupted downloads |
| FR32 | Epic 1 | Watchdog recovery to default mode |
| FR33 | Epic 1 | Safe defaults for new config keys |
| FR34 | Epic 3 + 7 | Transition buffer check (E3) + OTA heap pre-check (E7) |
| FR35 | Epic 7 | Graceful API degradation |
| FR36 | Epic 4 | Schedule persistence |
| FR37 | Epic 7 | Retry failed updates |
| FR38 | Epic 6 | Update available notification |
| FR39 | Epic 4 | View schedule rules and status |
| FR40 | Epic 1 | Manual mode switching from dashboard |
| FR41 | Epic 1-2 | Contributor extensibility (cross-cutting validation) |
| FR42 | Epic 1-2 | Mode Picker lists all registered modes (cross-cutting validation) |
| FR43 | Epic 7 | README links for self-service onboarding |

## Epic List

### Epic 1: The Wall Never Sleeps — Clock Mode & Orchestration Foundation
The owner sees a functioning display 100% of the time. When no flights are in range, Clock Mode activates automatically. The owner can manually switch modes from the dashboard. Watchdog recovery reboots to Clock Mode as a known-good default. New config keys introduced by the Delight Release resolve to safe defaults on first read.
**FRs covered:** FR1, FR2, FR3, FR4, FR32, FR33, FR40
**NFRs addressed:** NFR3, NFR10, NFR11, NFR13, NFR16
**ARs:** AR2 (ModeOrchestrator idle-fallback + manual switch), AR5 (Clock Mode settings schema), AR7 (g_flightCount atomic), AR8 (ModeEntry settingsSchema expansion)
**Cross-cutting validation:** FR41, FR42 (first new mode proves compile-time registry and Mode Picker discovery)

### Epic 2: Multi-Flight Awareness — Departures Board
The owner sees all tracked flights at a glance in an airport-style scrolling list. Rows appear and disappear dynamically as aircraft enter and leave the tracking radius. No cycling through one-at-a-time cards.
**FRs covered:** FR5, FR6, FR7, FR8, FR9
**NFRs addressed:** NFR4, NFR13, NFR17
**ARs:** AR5 (Departures Board settings schema)
**Cross-cutting validation:** FR41, FR42 (second new mode validates contributor extensibility contract)

### Epic 3: Polished Handoffs — Animated Transitions
Mode switches feel intentional, not broken. Smooth fade animations replace hard cuts. A non-technical observer perceives the wall as a finished product, not a prototype.
**FRs covered:** FR10, FR11, FR12, FR34 (transition buffer allocation check)
**NFRs addressed:** NFR1, NFR2, NFR17
**ARs:** AR1 (fade transition with RGB888 dual buffers, graceful degradation to instant cut on alloc failure)

### Epic 4: Ambient Intelligence — Mode Scheduling
The wall runs itself. The owner configures time-based rules once and forgets. Schedule and night mode brightness operate independently. The schedule takes priority over data-driven idle fallback when explicitly configured.
**FRs covered:** FR13, FR14, FR15, FR16, FR17, FR36, FR39
**NFRs addressed:** NFR6, NFR9, NFR15
**ARs:** AR2 (full ModeOrchestrator SCHEDULED state), AR4 (schedule NVS storage with indexed keys), AR6 (schedule API endpoints)

### Epic 5: Tailored Experience — Mode-Specific Settings
Each display mode is configurable through the Mode Picker. Clock Mode offers 12h/24h format, Departures Board configures row count and telemetry fields. Settings apply without reboot and persist across power cycles.
**FRs covered:** FR18, FR19, FR20
**NFRs addressed:** NFR7, NFR8
**ARs:** AR5 (per-mode settings schema with NVS prefix convention), AR6 (updated display/modes API with settings array), AR8 (dynamic schema iteration in API handlers)

### Epic 6: Update Awareness — OTA Version Check & Notification
The dashboard shows when a new firmware version is available. The owner can view release notes before deciding to update. GitHub API integration validated before any flash writes.
**FRs covered:** FR21, FR22, FR23, FR38
**NFRs addressed:** NFR7, NFR20, NFR21
**ARs:** AR3 (OTAUpdater — GitHub API client, version comparison), AR6 (OTA check endpoint + status API update)

### Epic 7: Painless Updates — OTA Download & Safety
One tap to install a firmware update over WiFi. A/B partition safety means zero bricked devices. Failed downloads leave firmware unchanged. SHA-256 verification ensures integrity. All settings preserved across updates. Self-service onboarding documentation validated.
**FRs covered:** FR24, FR25, FR26, FR27, FR28, FR29, FR30, FR31, FR34 (OTA heap pre-check), FR35, FR37, FR43
**NFRs addressed:** NFR5, NFR12, NFR14, NFR16, NFR18, NFR19
**ARs:** AR3 (OTAUpdater download task + FreeRTOS), AR6 (OTA pull/status endpoints), AR9 (incremental SHA-256), AR10 (shared prepareForOTA)

---

## Epic 1: The Wall Never Sleeps — Clock Mode & Orchestration Foundation

The owner sees a functioning display 100% of the time. When no flights are in range, Clock Mode activates automatically. The owner can manually switch modes from the dashboard. Watchdog recovery reboots to Clock Mode as a known-good default. New config keys introduced by the Delight Release resolve to safe defaults on first read.

### Story 1.1: Clock Mode Time Display

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

### Story 1.2: Idle Fallback to Clock Mode

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

### Story 1.3: Manual Mode Switching via Orchestrator

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

### Story 1.4: Watchdog Recovery to Default Mode

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

## Epic 2: Multi-Flight Awareness — Departures Board

The owner sees all tracked flights at a glance in an airport-style scrolling list. Rows appear and disappear dynamically as aircraft enter and leave the tracking radius. No cycling through one-at-a-time cards.

### Story 2.1: Departures Board Multi-Row Rendering

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
**Then** the display shows an empty state (no rows) rather than a blank or corrupted screen — the idle fallback to Clock Mode is handled by ModeOrchestrator (Epic 1), not by Departures Board itself

**Given** Departures Board is the second new mode added to MODE_TABLE
**When** the dashboard Mode Picker queries `GET /api/display/modes`
**Then** the response includes DeparturesBoardMode alongside ClockMode and existing modes, validating FR41 (compile-time registry extensibility) and FR42 (Mode Picker discovery)

### Story 2.2: Dynamic Row Add and Remove on Flight Changes

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

## Epic 3: Polished Handoffs — Animated Transitions

Mode switches feel intentional, not broken. Smooth fade animations replace hard cuts. A non-technical observer perceives the wall as a finished product, not a prototype.

### Story 3.1: Fade Transition Between Display Modes

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

## Epic 4: Ambient Intelligence — Mode Scheduling

The wall runs itself. The owner configures time-based rules once and forgets. Schedule and night mode brightness operate independently. The schedule takes priority over data-driven idle fallback when explicitly configured.

### Story 4.1: Schedule Rules Storage and Orchestrator Integration

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

### Story 4.2: Schedule Management Dashboard UI

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

## Epic 5: Tailored Experience — Mode-Specific Settings

Each display mode is configurable through the Mode Picker. Clock Mode offers 12h/24h format, Departures Board configures row count and telemetry fields. Settings apply without reboot and persist across power cycles.

### Story 5.1: Per-Mode Settings Panels in Mode Picker

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

## Epic 6: Update Awareness — OTA Version Check & Notification

The dashboard shows when a new firmware version is available. The owner can view release notes before deciding to update. GitHub API integration validated before any flash writes.

### Story 6.1: OTA Version Check Against GitHub Releases

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

### Story 6.2: Update Notification and Release Notes in Dashboard

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

## Epic 7: Painless Updates — OTA Download & Safety

One tap to install a firmware update over WiFi. A/B partition safety means zero bricked devices. Failed downloads leave firmware unchanged. SHA-256 verification ensures integrity. All settings preserved across updates. Self-service onboarding documentation validated.

### Story 7.1: OTA Download with Incremental SHA-256 Verification

As an owner,
I want to download and install a firmware update over WiFi with one tap,
So that I can update my device without finding a USB cable, opening PlatformIO, or pulling from git.

**Acceptance Criteria:**

**Given** OTAUpdater state is AVAILABLE (version check completed in Epic 6)
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

### Story 7.2: OTA Failure Handling, Rollback, and Retry

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

### Story 7.3: OTA Pull Dashboard UI and Status Polling

As an owner,
I want a clear dashboard interface for downloading firmware updates with real-time progress feedback,
So that I can see exactly what's happening during the update and know when it's complete.

**Acceptance Criteria:**

**Given** the OTA check has found an available update (Epic 6, Story 6.2)
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
