## Epic dl-7: Painless Updates — OTA Download & Safety

One tap to install a firmware update over WiFi. A/B partition safety means zero bricked devices. Failed downloads leave firmware unchanged. SHA-256 verification ensures integrity. All settings preserved across updates. Self-service onboarding documentation validated.

### Story dl-7.1: OTA Download with Incremental SHA-256 Verification

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

### Story dl-7.2: OTA Failure Handling, Rollback, and Retry

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

### Story dl-7.3: OTA Pull Dashboard UI and Status Polling

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

