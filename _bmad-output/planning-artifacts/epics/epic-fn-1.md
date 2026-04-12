## Epic fn-1: OTA Firmware Updates & Settings Migration — "Cut the Cord"

User can back up their settings, flash the dual-OTA partition firmware one final time via USB, restore settings in the wizard, then upload all future firmware from the dashboard with progress feedback, validation, and automatic rollback on failure.

### Story fn-1.1: Partition Table & Build Configuration

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

### Story fn-1.2: ConfigManager Expansion — Schedule Keys & SystemStatus

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

### Story fn-1.3: OTA Upload Endpoint

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

### Story fn-1.4: OTA Self-Check & Rollback Detection

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

### Story fn-1.5: Settings Export Endpoint

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

### Story fn-1.6: Dashboard Firmware Card & OTA Upload UI

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

### Story fn-1.7: Settings Import in Setup Wizard

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

