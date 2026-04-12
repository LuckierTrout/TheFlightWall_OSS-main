---
stepsCompleted: ['step-01-extract', 'step-01-confirmed', 'step-02-epics', 'step-03-stories']
inputDocuments:
  - '_bmad-output/planning-artifacts/prd-foundation.md'
  - '_bmad-output/planning-artifacts/architecture.md'
  - '_bmad-output/planning-artifacts/ux-design-specification-foundation.md'
---

# TheFlightWall Foundation Release - Epic Breakdown

## Overview

This document provides the complete epic and story breakdown for the TheFlightWall Foundation Release, decomposing the requirements from the PRD, Architecture, and UX Design Specification into implementable stories.

## Requirements Inventory

### Functional Requirements

**OTA Firmware Updates (FR1-FR11)**

- FR1: User can upload a firmware binary file to the device through the web dashboard
- FR2: Device can validate an uploaded firmware binary for correct size and ESP32 image format before writing to flash
- FR3: Device can write a validated firmware binary to the inactive OTA partition without disrupting current operation
- FR4: Device can reboot into newly flashed firmware after a successful OTA write
- FR5: Device can verify its own health after booting new firmware (WiFi connects OR 60-second timeout — per Architecture Decision F3) before marking the partition as valid
- FR6: Device can automatically roll back to the previous firmware partition if the new firmware fails to pass the health self-check
- FR7: Device can detect that a rollback occurred and notify the user through the dashboard
- FR8: Device can abort an in-progress firmware upload if the connection is interrupted, leaving the current firmware unaffected
- FR9: User can view the currently running firmware version on the dashboard and through the status API
- FR10: User can view a progress indicator showing percentage of bytes transferred, updated at least every 2 seconds, during a firmware upload
- FR11: Device can return a specific error message to the user when a firmware upload is rejected (oversized binary, invalid format, or transfer failure)

**Settings Migration (FR12-FR14)**

- FR12: User can export all current device settings as a downloadable JSON file from the dashboard
- FR13: User can import a previously exported settings JSON file during the setup wizard to pre-fill configuration fields
- FR14: Setup wizard can validate an imported settings file and ignore unrecognized keys without failing

**Night Mode & Brightness Schedule (FR15-FR24)**

- FR15: Device can synchronize its clock with an NTP time server after connecting to WiFi
- FR16: User can configure a timezone for the device through the dashboard or setup wizard
- FR17: Device can auto-detect the user's timezone from the browser during setup and suggest it as the default
- FR18: Device can convert an IANA timezone identifier (e.g., `America/Los_Angeles`) to its POSIX equivalent for use with `configTzTime()`
- FR19: User can define a brightness schedule with a start time, end time, and dim brightness level
- FR20: Device can automatically adjust LED brightness according to the configured schedule, including schedules that cross midnight
- FR21: Device can maintain correct schedule behavior across daylight saving time transitions
- FR22: User can view the current NTP sync status on the dashboard (synced vs. not synced, schedule active vs. inactive)
- FR23: User can enable or disable the brightness schedule without deleting the configured times
- FR24: Device must not activate the brightness schedule until NTP time has been successfully synchronized

**Onboarding & Setup Polish (FR25-FR29)**

- FR25: Setup wizard can display a panel position test pattern on the LED matrix and a matching visual preview in the browser simultaneously
- FR26: User can confirm or deny that the physical LED layout matches the expected preview during setup
- FR27: Setup wizard can return the user to hardware configuration settings if the user reports a layout mismatch
- FR28: Setup wizard can run an RGB color sequence test on the LED matrix after the user confirms layout correctness
- FR29: Setup wizard can transition to flight fetching after the user confirms both layout and color tests pass

**Dashboard UI (FR30-FR34)**

- FR30: User can access a Firmware card on the dashboard to upload firmware and view the current version
- FR31: User can access a Night Mode card on the dashboard to configure brightness schedule and timezone
- FR32: User can access a settings export function from the dashboard System card
- FR33: Dashboard can display a warning toast when a firmware rollback has been detected
- FR34: Dashboard can display NTP sync status and schedule state (Active / Inactive) within the Night Mode card

**System & Resilience (FR35-FR37)**

- FR35: Device can operate with a dual-OTA partition table while maintaining all existing functionality (flight data pipeline, web dashboard, logo management, calibration tools)
- FR36: Device can continue running current firmware if an OTA upload fails at any stage (validation, transfer, or boot)
- FR37: Device can persist all new configuration keys (timezone, schedule settings, schedule enabled flag) to non-volatile storage that survives power cycles and OTA updates

### NonFunctional Requirements

**Performance**

- NFR-P1: OTA firmware upload must complete within 30 seconds for a 1.5MB binary over local WiFi. Uploads exceeding 45 seconds should be treated as potential failures and logged
- NFR-P2: OTA upload progress reported to the dashboard must update at least every 2 seconds during transfer
- NFR-P3: Display settings changes triggered by the night mode scheduler must be reflected on the LED matrix within 1 second (consistent with existing hot-reload behavior)
- NFR-P4: NTP time synchronization must complete within 10 seconds of WiFi STA connection
- NFR-P5: Setup wizard Step 6 (Test Your Wall) pattern must render on the LED matrix within 500ms of being triggered
- NFR-P6: Dashboard pages (including new Firmware and Night Mode cards) must load within 3 seconds on the local network
- NFR-P7: OTA upload validation (size check + magic byte) must complete and return an error response within 1 second if the file is invalid

**Reliability**

- NFR-R1: Device must continue operating uninterrupted for 30+ days, including correct night mode schedule execution across midnight boundaries and DST transitions
- NFR-R2: Device must re-synchronize with NTP at least once every 24 hours while WiFi is connected. If NTP re-sync fails, the device continues using the last known time without disrupting schedule operation
- NFR-R3: OTA self-check must complete and mark the partition valid within 30 seconds of boot — if it doesn't, the watchdog must trigger rollback within 60 seconds total
- NFR-R4: Night mode schedule must survive power cycles — schedule configuration persists in NVS and resumes correct behavior after reboot once NTP re-syncs
- NFR-R5: NTP sync failure must not crash the device or degrade any functionality other than schedule activation
- NFR-R6: Settings export must produce a valid, complete JSON file that can be re-imported without data loss or corruption
- NFR-R7: A failed or interrupted OTA upload must never leave the device in a state where the current firmware stops running or the dashboard becomes inaccessible

**Resource Constraints**

- NFR-C1: Total firmware binary must not exceed 1.5MB (OTA partition size)
- NFR-C2: LittleFS must retain at least 500KB free after all web assets and bundled logos are stored
- NFR-C3: New configuration keys (timezone, schedule settings, schedule enabled) must add no more than 256 bytes to NVS usage
- NFR-C4: OTA upload must stream directly to flash — no full-binary RAM buffering
- NFR-C5: Night mode scheduler must not increase main loop cycle time by more than 1% — limited to a non-blocking time comparison per iteration with no I/O or blocking operations

### Additional Requirements

**From Architecture Decisions (F1-F7):**

- AR1: Custom dual-OTA partition table with exact offsets: nvs 20KB (0x9000), otadata 8KB (0xE000), app0 1.5MB (0x10000), app1 1.5MB (0x190000), spiffs 960KB (0x310000) — Decision F1
- AR2: Pre-implementation gate — measure current firmware binary size; if exceeding 1.3MB, optimize (disable Bluetooth, strip unused features) before adding Foundation code — Decision F1
- AR3: OTA upload handler uses ESPAsyncWebServer `onUpload` callback with `Update.h` streaming (begin → write per chunk → end) — Decision F2
- AR4: OTA self-check uses WiFi-OR-timeout strategy (60-second window), NOT self-HTTP-request — architectural simplification of FR5 per Decision F3. Story specs MUST reference Decision F3
- AR5: Rollback detection via `esp_ota_get_last_invalid_partition()` — NOT `esp_ota_check_rollback_is_possible()` — Decision F3
- AR6: IANA-to-POSIX timezone mapping is a browser-side JS table (~50-80 entries) in wizard.js and dashboard.js — zero firmware overhead — Decision F4
- AR7: Night mode scheduler is a non-blocking main loop check using `getLocalTime(&now, 0)` with timeout=0 — Decision F5
- AR8: Schedule times stored as `uint16` minutes since midnight (0-1439) in firmware — no string time representations — Decision F5
- AR9: 5 new NVS keys with 15-char abbreviations: `timezone`, `sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt` — Decision F6
- AR10: New `ScheduleConfig` struct in ConfigManager with `getSchedule()` getter — Decision F6
- AR11: All 5 new schedule keys are hot-reload (no reboot required). Timezone change calls `configTzTime()` immediately — Decision F6
- AR12: 2 new API endpoints: `POST /api/ota/upload` (multipart firmware upload), `GET /api/settings/export` (JSON file download) — Decision F7
- AR13: Updated `/api/status` response with: `firmware_version`, `rollback_detected`, `ntp_synced`, `schedule_active` — Decision F7
- AR14: Firmware version via build flag: `-D FW_VERSION=\"1.0.0\"` in platformio.ini — Decision F2
- AR15: NTP sync tracked via `sntp_set_time_sync_notification_cb` callback with `std::atomic<bool>` — no polling — Decision F5
- AR16: Settings import is client-side only (wizard JS reads file via FileReader, pre-fills fields) — no server-side import endpoint — Decision F7
- AR17: No new files — all Foundation changes fit into 13 existing files — File Change Map
- AR18: Multipart `Content-Length` is body size, not file size — use `UPDATE_SIZE_UNKNOWN` with `Update.begin(partition->size)` and let `Update.end()` validate — Gap Analysis

**From Architecture Enforcement Rules (12-16):**

- AR-E12: OTA upload must validate before writing — size check + magic byte on first chunk, fail-fast with `{ ok: false, error: "..." }` JSON response
- AR-E13: OTA upload must stream directly to flash via `Update.write()` per chunk — never buffer the full binary in RAM
- AR-E14: `getLocalTime()` must always use `timeout=0` (non-blocking) — never block the main loop waiting for time
- AR-E15: Schedule times stored as `uint16` minutes since midnight (0-1439) — no string time representations in firmware
- AR-E16: Settings import is client-side only — no server-side import endpoint or file parsing on the ESP32

### UX Design Requirements

- UX-DR1: Implement OTA Upload Zone component (`.ota-upload-zone`) with 5 states: empty, drag hover, file selected valid, file selected invalid, uploading. Must include `role="button"`, `tabindex="0"`, `aria-label`, Enter/Space keyboard support. Reuses structural pattern from `.logo-upload-zone` with different validation logic (size ≤ 1.5MB, first byte `0xE9`). ~15 lines CSS extending existing upload zone
- UX-DR2: Implement OTA Progress Bar component (`.ota-progress`) with `role="progressbar"`, `aria-valuenow`/`min`/`max` attributes. 8px height bar, percentage text centered, updates minimum every 2 seconds. Transitions to countdown phase on completion. ~10 lines CSS
- UX-DR3: Implement Persistent Banner component (`.banner-warning`) for OTA rollback notification with `role="alert"`, `aria-live="polite"`, NVS-backed persistence across page refreshes and power cycles. Hardcoded `#2d2738` background for captive portal WebKit compatibility. Dismiss via `POST /api/ota/ack-rollback`. ~10 lines CSS
- UX-DR4: Implement Timeline Bar component (`.schedule-timeline`) as 24-hour canvas visualization of night mode schedule with hour markers (00, 06, 12, 18, 24), dim period shading, "now" marker, and midnight-crossing support (two `fillRect` calls). Canvas is `aria-hidden="true"` — hour markers rendered as HTML text. ~15 lines CSS, ~25 lines JS
- UX-DR5: Implement Time Picker Row component (`.time-picker-row`) with side-by-side `<input type="time">` elements for dim start/end, proper `<label for>` associations, flexbox layout with gap. Enabled/disabled states tied to schedule toggle. ~5 lines CSS
- UX-DR6: Implement Schedule Status Line component (`.schedule-status`) with 5 states: active dimming (accent-dim dot), scheduled (success dot), waiting for clock sync (warning dot), clock not set (warning dot), disabled (text-secondary dot). Dot is `aria-hidden="true"`, text communicates full state. ~5 lines CSS
- UX-DR7: Follow three-tier button hierarchy: Primary (`.btn-primary`) for forward actions (Upload Firmware, Yes it matches, Apply Changes), Secondary (`.btn-secondary`) for safe alternatives (No — take me back, Download Settings), Danger reserved for destructive only. One primary per visible context. Minimum 44x44px touch targets on all interactive elements
- UX-DR8: Implement four-level feedback pattern: Toast (transient, 3s success / 5s error with `role="status"`/`role="alert"`), Status Line (persistent low urgency), Persistent Banner (persistent high urgency), Inline Progress (active operation). Toasts stack from top, max 2 visible. Error messages must be specific and actionable (cause + recovery)
- UX-DR9: Client-side-first validation: OTA file size ≤ 1.5MB + magic byte `0xE9` before upload; settings import valid JSON + recognized keys before pre-fill. Fail fast with specific error messages naming cause and constraint. Validation before commitment (before upload begins)
- UX-DR10: Firmware and Night Mode dashboard cards always expanded (not collapsed by default). Night Mode card includes sticky apply bar (`.apply-bar`) for unsaved changes with "Apply Changes" primary button. OTA upload is an immediate action — no apply bar
- UX-DR11: Wizard Step 6 navigation: linear progression, "No" jumps back to Step 4 (hardware settings) for guided correction, settings import pre-fills all steps at once, after Step 6 "Yes" → RGB color test → wizard terminates (AP→STA transition)
- UX-DR12: All Foundation components must meet WCAG 2.1 AA color contrast requirements per verified contrast table: body text ~9.5:1, secondary text ~6.2:1, progress bar fill ~3.2:1 (non-text), button text ~3.5:1 (large text), rollback banner ~10:1 (AAA)
- UX-DR13: Phone-first responsive design with single breakpoint at 768px. All Foundation components work at 320px minimum width. No new media queries. Canvas elements use `width: 100%` with `devicePixelRatio` for Retina. Time pickers side-by-side at 320px (~45% width each)
- UX-DR14: OTA upload post-sequence: progress bar (0-100%) → 3-second countdown → polling for device reachability → version resolution → success/failure toast. During reboot, show "Waiting for device..." with explanatory text
- UX-DR15: Cross-context navigation hints: Firmware card helper text linking to System card for pre-migration settings backup. "Run Panel Test" secondary button in Calibration card for post-setup panel testing
- UX-DR16: Dashboard page load: HTML/CSS renders immediately (local LittleFS), JS fetches `/api/status` then `/api/settings` to populate all fields. No skeleton screens, no loading spinners. If fetch fails, show default values + warning toast

### FR Coverage Map

| FR | Epic | Description |
|----|------|-------------|
| FR1 | Epic 1 | Upload firmware via dashboard |
| FR2 | Epic 1 | Validate firmware binary (size + format) |
| FR3 | Epic 1 | Write to inactive OTA partition |
| FR4 | Epic 1 | Reboot into new firmware |
| FR5 | Epic 1 | Self-check after boot (WiFi-OR-timeout per F3) |
| FR6 | Epic 1 | Auto-rollback on failed self-check |
| FR7 | Epic 1 | Detect rollback, notify user |
| FR8 | Epic 1 | Abort upload on connection loss |
| FR9 | Epic 1 | Display firmware version |
| FR10 | Epic 1 | Upload progress indicator |
| FR11 | Epic 1 | Specific error messages on rejection |
| FR12 | Epic 1 | Export settings as JSON |
| FR13 | Epic 1 | Import settings in wizard |
| FR14 | Epic 1 | Validate imported settings, ignore unknown keys |
| FR15 | Epic 2 | NTP time sync after WiFi |
| FR16 | Epic 2 | Configure timezone (dashboard/wizard) |
| FR17 | Epic 3 | Auto-detect timezone from browser in wizard |
| FR18 | Epic 2 | IANA-to-POSIX timezone conversion |
| FR19 | Epic 2 | Define brightness schedule |
| FR20 | Epic 2 | Auto-adjust brightness with midnight-crossing |
| FR21 | Epic 2 | Correct behavior across DST transitions |
| FR22 | Epic 2 | NTP sync status on dashboard |
| FR23 | Epic 2 | Enable/disable schedule without deleting times |
| FR24 | Epic 2 | Schedule inactive until NTP syncs |
| FR25 | Epic 3 | Panel position test pattern + canvas preview |
| FR26 | Epic 3 | User confirms/denies layout match |
| FR27 | Epic 3 | Return to hardware settings on "No" |
| FR28 | Epic 3 | RGB color test on "Yes" |
| FR29 | Epic 3 | Transition to flight fetching after confirmation |
| FR30 | Epic 1 | Dashboard Firmware card |
| FR31 | Epic 2 | Dashboard Night Mode card |
| FR32 | Epic 1 | Settings export in System card |
| FR33 | Epic 1 | Rollback warning toast |
| FR34 | Epic 2 | NTP sync status in Night Mode card |
| FR35 | Epic 1 | Dual-OTA coexistence with all existing features |
| FR36 | Epic 1 | Current firmware continues on OTA failure |
| FR37 | Epic 1 | Persist new config keys to NVS |

**Coverage: 37/37 FRs mapped. 0 gaps.**

## Epic List

### Epic 1: OTA Firmware Updates & Settings Migration — "Cut the Cord"

User can back up their settings, flash the dual-OTA partition firmware one final time via USB, restore settings in the wizard, then upload all future firmware from the dashboard with progress feedback, validation, and automatic rollback on failure. The USB cable goes in the drawer.

**User Journeys:** J1 (The Last USB Flash), J4 (The Bad Flash)
**FRs covered:** FR1, FR2, FR3, FR4, FR5, FR6, FR7, FR8, FR9, FR10, FR11, FR12, FR13, FR14, FR30, FR32, FR33, FR35, FR36, FR37
**NFRs addressed:** NFR-P1, NFR-P2, NFR-P6, NFR-P7, NFR-R3, NFR-R6, NFR-R7, NFR-C1, NFR-C2, NFR-C3, NFR-C4
**Architecture:** AR1-AR5, AR9-AR14, AR16-AR18, AR-E12, AR-E13
**UX Design:** UX-DR1, UX-DR2, UX-DR3, UX-DR7, UX-DR8, UX-DR9, UX-DR10, UX-DR14, UX-DR15, UX-DR16
**Depends on:** Nothing (standalone, first epic)

### Epic 2: Night Mode & Brightness Scheduling — "The Wall That Sleeps"

User configures a timezone and brightness schedule on the dashboard. The wall automatically dims at night and restores in the morning — correct across midnight boundaries and DST transitions. NTP sync status is visible, and the schedule degrades gracefully if NTP is unavailable.

**User Journey:** J2 (Living Room at Midnight)
**FRs covered:** FR15, FR16, FR18, FR19, FR20, FR21, FR22, FR23, FR24, FR31, FR34
**NFRs addressed:** NFR-P3, NFR-P4, NFR-R1, NFR-R2, NFR-R4, NFR-R5, NFR-C5
**Architecture:** AR6-AR8, AR10, AR11, AR15, AR-E14, AR-E15
**UX Design:** UX-DR4, UX-DR5, UX-DR6, UX-DR7, UX-DR8, UX-DR10, UX-DR12, UX-DR13
**Depends on:** Epic 1 (ConfigManager with schedule keys, partition table, `/api/status` extensions)

### Epic 3: Onboarding Polish — "Fresh Start Done Right"

New user completes the setup wizard with a "Test Your Wall" step that verifies panel layout matches expectations before showing garbled flight data. Timezone auto-detects from the browser. Settings can be imported to skip re-typing API keys after a partition migration.

**User Journey:** J3 (Fresh Start Done Right)
**FRs covered:** FR17, FR25, FR26, FR27, FR28, FR29
**NFRs addressed:** NFR-P5, NFR-P6
**Architecture:** AR6 (TZ_MAP reuse), AR-E16
**UX Design:** UX-DR9, UX-DR11, UX-DR12, UX-DR13, UX-DR15
**Depends on:** Epic 1 (settings export for import), Epic 2 (TZ_MAP for auto-detect)

---

## Epic 1: OTA Firmware Updates & Settings Migration — "Cut the Cord"

User can back up their settings, flash the dual-OTA partition firmware one final time via USB, restore settings in the wizard, then upload all future firmware from the dashboard with progress feedback, validation, and automatic rollback on failure.

### Story 1.1: Partition Table & Build Configuration

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

### Story 1.2: ConfigManager Expansion — Schedule Keys & SystemStatus

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

### Story 1.3: OTA Upload Endpoint

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

### Story 1.4: OTA Self-Check & Rollback Detection

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

### Story 1.5: Settings Export Endpoint

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

**Given** a settings export followed by a settings import (Story 1.7)
**When** the round-trip is tested
**Then** all config values survive the export → import cycle without data loss or corruption

**References:** FR12, AR12, NFR-R6

### Story 1.6: Dashboard Firmware Card & OTA Upload UI

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

### Story 1.7: Settings Import in Setup Wizard

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

## Epic 2: Night Mode & Brightness Scheduling — "The Wall That Sleeps"

User configures a timezone and brightness schedule on the dashboard. The wall automatically dims at night and restores in the morning — correct across midnight boundaries and DST transitions. NTP sync status is visible, and the schedule degrades gracefully if NTP is unavailable.

### Story 2.1: NTP Time Sync & Timezone Configuration

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

### Story 2.2: Night Mode Brightness Scheduler

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

### Story 2.3: Dashboard Night Mode Card

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

## Epic 3: Onboarding Polish — "Fresh Start Done Right"

New user completes the setup wizard with a "Test Your Wall" step that verifies panel layout matches expectations before showing garbled flight data. Timezone auto-detects from the browser.

### Story 3.1: Wizard Step 6 — Test Your Wall

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

### Story 3.2: Wizard Timezone Auto-Detect

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
