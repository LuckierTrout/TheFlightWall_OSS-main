## Epic dl-6: Update Awareness — OTA Version Check & Notification

The dashboard shows when a new firmware version is available. The owner can view release notes before deciding to update. GitHub API integration validated before any flash writes.

### Story dl-6.1: OTA Version Check Against GitHub Releases

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

### Story dl-6.2: Update Notification and Release Notes in Dashboard

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

