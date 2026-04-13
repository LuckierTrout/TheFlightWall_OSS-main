<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 6 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T003059Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-1.6

You are synthesizing 3 independent code review findings.

Your mission:
1. VERIFY each issue raised by reviewers
   - Cross-reference with project_context.md (ground truth)
   - Cross-reference with git diff and source files
   - Identify false positives (issues that aren't real problems)
   - Confirm valid issues with evidence

2. PRIORITIZE real issues by severity
   - Critical: Security vulnerabilities, data corruption risks
   - High: Bugs, logic errors, missing error handling
   - Medium: Code quality issues, performance concerns
   - Low: Style issues, minor improvements

3. SYNTHESIZE findings
   - Merge duplicate issues from different reviewers
   - Note reviewer consensus (if 3+ agree, high confidence)
   - Highlight unique insights from individual reviewers

4. APPLY source code fixes
   - You have WRITE PERMISSION to modify SOURCE CODE files
   - CRITICAL: Before using Edit tool, ALWAYS Read the target file first
   - Use EXACT content from Read tool output as old_string, NOT content from this prompt
   - If Read output is truncated, use offset/limit parameters to locate the target section
   - Apply fixes for verified issues
   - Do NOT modify the story file (only Dev Agent Record if needed)
   - Document what you changed and why

Output format:
## Synthesis Summary
## Issues Verified (by severity)
## Issues Dismissed (false positives with reasoning)
## Source Code Fixes Applied

]]></mission>
<context>
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

### File List

- `firmware/data-src/dashboard.html` (MODIFIED) — Added Firmware card HTML between Logos and System, System card id="system-card", Download Settings button
- `firmware/data-src/dashboard.js` (MODIFIED) — Added loadFirmwareStatus(), OTA upload with XHR progress, client-side validation, reboot countdown/polling, rollback banner, settings export click handler (~200 lines)
- `firmware/data-src/style.css` (MODIFIED) — Added ~35 lines OTA component styles (.ota-upload-zone, .ota-progress, .banner-warning, .btn-secondary, etc.)
- `firmware/adapters/WebPortal.cpp` (MODIFIED) — Added POST /api/ota/ack-rollback, GET /api/settings/export endpoints; added rollback_acknowledged to /api/status; clear ota_rb_ack on successful OTA completion; added #include <Preferences.h>
- `firmware/data/dashboard.html.gz` (REGENERATED) — 3,294 bytes
- `firmware/data/dashboard.js.gz` (REGENERATED) — 21,523 bytes
- `firmware/data/style.css.gz` (REGENERATED) — 4,616 bytes


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic fn-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #1 Automation Missing**: Binary size logging was manual, not automated in build process | Created `check_size.py` PlatformIO pre-action script that runs on every build, logs binary size, warns at 1.3MB threshold, fails at 1.5MB limit. Added `extra_scripts = pre:check_size.py` to platformio.ini. |
| critical | Silent Data Loss Risk**: LittleFS.begin(true) auto-formats on mount failure without notification | Changed to `LittleFS.begin(false)` with explicit error logging and user-visible instructions to reflash filesystem. Device continues boot but warns user of unavailable web assets/logos. |
| high | Missing Partition Runtime Validation**: No verification that running firmware matches expected partition layout | Added `validatePartitionLayout()` function that checks running app partition size (0x180000) and LittleFS partition size (0xF0000) against expectations. Logs warnings if mismatches detected. Called during setup before LittleFS mount. |

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Silent Exit in check_size.py if Binary Missing | Added explicit error logging when binary is missing |
| low | Magic Numbers for Timing/Thresholds | Named constants already exist for some values (AUTHENTICATING_DISPLAY_MS, BUTTON_DEBOUNCE_MS). Additional refactoring would require broader changes. |
| low | Interface Segregation - WiFiManager Callback | Changed to C++ comment syntax to clarify intent |

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Silent Exit in check_size.py When Binary Missing | Added explicit error logging and `env.Exit(1)` when binary is missing |
| high | Magic Numbers for Partition Sizes - No Cross-Reference | Added cross-reference comments in all 3 files to alert developers when updating partition sizes |
| medium | Partition Table Gap Not Documented | Added comment documenting reserved gap |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing `getSchedule()` implementation | Added `ConfigManager::getSchedule()` method at line 536 following existing getter pattern with ConfigLockGuard for thread safety. Method returns copy of `_schedule` struct. |
| critical | Schedule keys missing from `dumpSettingsJson()` API | Added `snapshot.schedule = _schedule` to snapshot capture at line 469 and added all 5 schedule keys to JSON output at lines 507-511. GET /api/settings now returns 27 total keys (22 existing + 5 new). |
| critical | OTA and NTP subsystems not added to SystemStatus | Added `OTA` and `NTP` to Subsystem enum, updated `SUBSYSTEM_COUNT` from 6 to 8, and added "ota" and "ntp" cases to `subsystemName()` switch. Also fixed stale comment "existing six" → "subsystems". |
| high | Required unit tests missing from test suite | Added 5 new test functions: `test_defaults_schedule()`, `test_nvs_write_read_roundtrip_schedule()`, `test_apply_json_schedule_hot_reload()`, `test_apply_json_schedule_validation()`, and `test_system_status_ota_ntp()`. All tests integrated into `setup()` test runner. |
| low | Stale comment in SystemStatus.cpp | Updated comment from "existing six" to "subsystems" to reflect new count of 8. |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Integer Overflow in `sched_enabled` Validation | Changed from `value.as<uint8_t>()` (wraps 256→0) to validating as `unsigned int` before cast. Now properly rejects values >1. Added type check `value.is<unsigned int>()`. |
| critical | Missing Validation for `sched_dim_brt` | Added bounds checking (0-255) and type validation. Previously accepted any value without validation, violating story requirements. |
| critical | Test Suite Contradicts Implementation | Renamed `test_apply_json_ignores_unknown_keys` → `test_apply_json_rejects_unknown_keys` and corrected assertions. applyJson uses all-or-nothing validation, not partial success. |
| high | Missing Test Coverage for AC #4 | Added `test_dump_settings_json_includes_schedule()` — verifies all 5 schedule keys present in JSON and total key count = 27. |
| high | Validation Test Coverage Gaps | Extended `test_apply_json_schedule_validation()` with 2 additional test cases: `sched_dim_brt > 255` rejection and `sched_enabled = 256` overflow rejection. |
| dismissed | `/api/settings` exposes secrets (wifi_password, API keys) in plaintext | FALSE POSITIVE: Pre-existing design, not introduced by this story. Requires separate security story to implement credential masking. Story scope was schedule keys + SystemStatus subsystems only. |
| dismissed | ConfigSnapshot heap allocation overhead in applyJson | FALSE POSITIVE: Necessary for atomic semantics — applyJson must validate all keys before committing any changes. Snapshot pattern is intentional design. |
| dismissed | SystemStatus mutex timeout fallback is unsafe | FALSE POSITIVE: Pre-existing pattern across SystemStatus implementation (lines 35-44, 53-58, 65-73). Requires broader refactor outside story scope. This story only added OTA/NTP subsystems. |
| dismissed | SystemStatus tight coupling to WiFi, LittleFS, ConfigManager | FALSE POSITIVE: Pre-existing architecture in `toExtendedJson()` method. Not introduced by this story — story only added 2 subsystems to existing enum. |
| dismissed | Hardcoded NVS namespace string | FALSE POSITIVE: Pre-existing pattern, not story scope. NVS namespace was defined before this story. |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Test baseline claims 27 keys but implementation has 29 | Updated test assertion from 27 to 29 to match actual implementation |
| critical | Missing negative number validation for schedule time fields | Added type validation with `value.is<int>()` and signed bounds checking before unsigned cast |
| critical | Missing type validation for timezone string | Added `value.is<const char*>()` type check before string conversion |
| high | Missing test coverage for AC #4 - JSON dump includes all schedule keys | Corrected key count assertion from 27 to 29 |
| high | Validation test coverage gaps | Already present in current code (lines 373-382) - FALSE POSITIVE on reviewer's part, but validation logic in implementation was incomplete (see fixes #6, #7) |
| high | Integer overflow in sched_enabled validation | Already has type check `value.is<unsigned int>()` - validation is correct. Reviewing again...actually the test exists to verify 256 is rejected (line 380). This is working correctly. |
| high | Missing validation for sched_dim_brt | Code already has type check and >255 rejection (lines 163-167) after Round 2 fixes. Verified correct. |
| medium | Test suite contradicts implementation (unknown key handling) | Test already renamed to `test_apply_json_rejects_unknown_keys` with correct assertions after Round 2 fixes |

## Story fn-1-3 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `clearOTAUpload()` unconditionally calls `Update.abort()` when `started == true`, including the success path where `clearOTAUpload()` is called immediately after `Update.end(true)` — aborting the just-written firmware | Set `state->started = false` after successful `Update.end(true)` so the completed-update flag is cleared before cleanup runs |
| high | No concurrent upload guard; a second client can call `Update.begin()` while an upload is in-flight, sharing the non-reentrant `Update` singleton and risking flash corruption | Added `g_otaInProgress` static bool; second upload receives `OTA_BUSY` / 409 Conflict immediately |
| medium | `SystemStatus::set()` is called on `WRITE_FAILED`/`VERIFY_FAILED` but silently omitted from `INVALID_FIRMWARE`, `NO_OTA_PARTITION`, and `BEGIN_FAILED` — OTA subsystem status does not reflect these failures | Added `SystemStatus::set(ERROR)` to all three missing error paths |
| low | Task 3 claimed partition info logging was complete, but code only emitted `LOG_I("OTA", "Update started")` with no label/size | Added `Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size);` |
| low | `NO_OTA_PARTITION`, `BEGIN_FAILED`, `WRITE_FAILED`, `VERIFY_FAILED` (server-side failures) return HTTP 400 (client error); `OTA_BUSY` conflicts returned as 400 | Error-code-to-HTTP mapping: `INVALID_FIRMWARE` → 400, `OTA_BUSY` → 409, server failures → 500 |
| dismissed | `POST /api/ota/upload` is unauthenticated / CSRF-vulnerable | FALSE POSITIVE: No endpoint in this device has authentication — `/api/reboot`, `/api/reset`, `/api/settings` are all unauthenticated. This is a pre-existing architectural design gap (LAN-only device), not introduced by this story. Requires a dedicated security story. |
| dismissed | Missing `return` statements after `NO_OTA_PARTITION` and `BEGIN_FAILED` — code falls through to write path | FALSE POSITIVE: FALSE POSITIVE. The actual code at lines 480 and 490 contains explicit `return;` statements. Validator C misread a code snippet. |
| dismissed | Task 8 (header declaration) incomplete — `_handleOTAUpload()` not in `WebPortal.h` | FALSE POSITIVE: FALSE POSITIVE. The task itself states "or keep inline like logo upload" as an explicit alternative. The inline lambda approach is the correct pattern and matches the logo upload implementation. |
| dismissed | `std::vector<OTAUploadState>` introduces heap churn on hot async path | FALSE POSITIVE: This is the established project pattern — `g_logoUploads` uses the same `std::vector` approach. OTA is single-flight (now enforced), so the vector holds at most one entry. Not worth a divergent pattern. |
| dismissed | Oversized binary not rejected before flash writes begin | FALSE POSITIVE: `Update.begin(partition->size)` tells the library the maximum expected size. `Update.write()` will fail and return fewer-than-requested bytes when the partition is full, which the existing write-failure path handles. `Update.end(true)` accepts partial writes correctly. The library itself is the bounds guard. --- |

## Story fn-1-4 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `g_otaSelfCheckDone = true` placed outside the `err == ESP_OK` block — self-check silently completes even when `esp_ota_mark_app_valid_cancel_rollback()` fails, leaving firmware in unvalidated state with no retry | Moved `g_otaSelfCheckDone = true` inside the success branch. On failure, function exits without setting the flag so the next loop iteration retries. |
| high | AC #2 requires "a WARNING log message" for the timeout path, but `LOG_I` was used — wrong log level AND there was no `LOG_W` macro in the project | Added `LOG_W` macro to `Log.h` at level `>= 1` (same severity tier as errors). Changed `LOG_I` → `LOG_W` on the timeout path. |
| medium | `isPendingVerify` OTA state computed via two IDF flash-read calls (`esp_ota_get_running_partition()` + `esp_ota_get_state_partition()`) on every `loop()` iteration for up to 60 seconds — state cannot change during this window | Introduced `static int8_t s_isPendingVerify = -1` cached on first call; subsequent iterations skip the IDF calls entirely. |
| medium | `("Firmware verified — WiFi connected in " + String(elapsedSec) + "s").c_str()` passes a pointer to a temporary `String`'s internal buffer to `SystemStatus::set(const String&)` — technically safe but fragile code smell | Extracted to named `String okMsg` variable before the call. Same pattern applied in the error path. |
| low | `OTA_SELF_CHECK_TIMEOUT_MS = 60000` had no inline comment explaining the rationale for the 60s value | Added 3-line comment citing Architecture Decision F3, typical WiFi connect time, and no-WiFi fallback scenario. |
| dismissed | AC #6 violated — rollback WARNING emitted on normal boots after a prior rollback | FALSE POSITIVE: Story Gotcha #2 explicitly states: *"g_rollbackDetected will be true on every subsequent boot until a new successful OTA… This means /api/status will return rollback_detected: true on every boot until that happens — this is correct and intentional API behavior."* The deferred `SystemStatus::set(WARNING)` is the mechanism that surfaces the persisted rollback state through the health API. Suppressing it would hide a real device condition. |
| dismissed | `g_bootStartMs` uninitialized read risk — if called before `setup()`, `millis() - 0` returns a large value triggering immediate timeout | FALSE POSITIVE: In Arduino on ESP32, `setup()` is guaranteed to execute before `loop()`. `g_bootStartMs` is set at the top of `setup()`. There is no code path where `performOtaSelfCheck()` can run before `setup()`. |
| dismissed | `SystemStatus::set()` best-effort mutex fallback is unsafe | FALSE POSITIVE: Pre-existing pattern across the entire `SystemStatus` implementation (lines 41–50, 59–64, 71–79). Not introduced by this story. Requires broader architectural refactor outside story scope. |
| dismissed | Magic numbers 2000/4000 in `tickStartupProgress()` | FALSE POSITIVE: Pre-existing code in `tickStartupProgress()` from a prior story. Not introduced by fn-1.4. Out of scope for this synthesis. |
| dismissed | LOG macros used inconsistently with `Serial.printf` — project logging pattern violated | FALSE POSITIVE: `Log.h` macros (`LOG_E`, `LOG_I`, `LOG_V`) only accept **fixed string literals** — they have no format-string support. `Serial.printf` is the only viable option when embedding runtime values (partition labels, elapsed time, error codes). The mixed usage is correct project practice, not an inconsistency. The sole actionable logging issue was the `LOG_I` vs `LOG_W` severity mismatch (fixed above). |
| dismissed | `FW_VERSION` format not validated at runtime | FALSE POSITIVE: `FW_VERSION` is a compile-time build flag enforced by the build system and team convention. Runtime validation of a compile-time constant is over-engineering and has no failure mode that warrants it. |
| dismissed | `FlightStatsSnapshot` location not explicit in story docs | FALSE POSITIVE: Documentation gap only, not a code defect. The struct is defined in `main.cpp` as confirmed by reading the source. No code fix needed. |

## Story fn-1-4 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `s_isPendingVerify` error caching — `s_isPendingVerify = 0` was set unconditionally before the conditional, so if `esp_ota_get_running_partition()` returned NULL or `esp_ota_get_state_partition()` returned non-`ESP_OK`, the pending-verify state was permanently cached as 0 (normal boot). A transient IDF probe error on the first loop iteration would suppress AC #1/#2 status and timing logs on a real post-OTA boot for the entire 60s window. | Removed `s_isPendingVerify = 0` pre-assignment; now only set on successful probe. State probe retries on next loop iteration if running partition is NULL or probe fails. |
| low | Rollback WARNING blocked by `err == ESP_OK` check — the `SystemStatus::set(WARNING, "Firmware rolled back...")` call was inside the `if (err == ESP_OK)` block, meaning if `esp_ota_mark_app_valid_cancel_rollback()` persistently failed, AC #4's required WARNING status was never emitted. | Moved rollback `SystemStatus::set()` before the mark_valid call, inside the WiFi/timeout condition but outside the success guard. Added `static bool s_rollbackStatusSet` to prevent repeated calls on retry iterations. |
| low | Boot timing starts 200ms late — `g_bootStartMs = millis()` was assigned after `Serial.begin(115200)` and `delay(200)`, understating the reported "WiFi connected in Xms" timing by 200ms. Story task requirement explicitly states capture before delays. | Moved `g_bootStartMs = millis()` to be the very first statement in `setup()`, before `Serial.begin()` and `delay(200)`. |
| low | Log spam on mark_valid failure — when `esp_ota_mark_app_valid_cancel_rollback()` failed, `Serial.printf()` and `SystemStatus::set(ERROR)` were called on every loop iteration (potentially 20/sec) since neither `g_otaSelfCheckDone` is set (correctly) nor the error path had a throttle. | Added `static bool s_markValidErrorLogged = false` guard so the error is logged and status set only on the first failure; subsequent retries are silent. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md`

## Findings

1. **🔴 Critical: the story marks “Cancel/reset” complete, but the UI has no cancel or re-select path once a valid file is chosen.**  
   The upload zone is hidden after validation, and the only remaining control is `Upload Firmware`; there is no cancel button and no alternate picker trigger. That makes Task 7’s final subtask a straight false claim.  
   Refs: [fn-1.6 story](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md:102), [dashboard.html](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.html:245), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:2699)

2. **🔴 Critical: AC #6 / Task 7 promises progress updates at least every 2 seconds, but the implementation never enforces that.**  
   The code only updates the bar inside `xhr.upload.onprogress`. If the browser emits sparse events or stalls mid-upload, the progress UI freezes with no timer-based fallback. That is both an AC gap and a task-completion lie.  
   Refs: [fn-1.6 story](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md:26), [fn-1.6 story](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md:97), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:2729)

3. **🟠 Important: the reboot polling timeout is wrong and can declare failure before the last poll finishes.**  
   `attempts` is incremented, the async `FW.get('/api/status')` is started, and then timeout is checked immediately. On attempt 20, the UI can show “Device unreachable” before the last request resolves. Because this uses `setInterval`, slow requests can also overlap. That is a real AC #8 edge-case failure.  
   Refs: [fn-1.6 story](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md:30), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:2794)

4. **🟠 Important: the success path does not verify that the device actually rebooted into new firmware.**  
   Any successful `/api/status` response ends the flow and shows `Updated to v...`. There is no version-change check, no uptime reset check, and no cache-busting on `FW.get`, so a stale or pre-reboot response can be treated as a successful upgrade.  
   Refs: [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:2799), [common.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/common.js:25)

5. **🟠 Important: there is no automated coverage for the new OTA/export/rollback behavior.**  
   The new server endpoints and the whole client-side firmware flow landed without unit or e2e coverage for rollback ack persistence, settings export, upload progress, reboot polling, or timeout handling. A search across `firmware/test` and `tests/e2e` found no matching firmware/ota/export/rollback tests.  
   Refs: [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:628), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:2587)

6. **🟡 Minor: the story’s file-level traceability is false.**  
   The story says `WebPortal.h` and `main.cpp` should not change, and lists `SystemStatus.cpp` as the place that should expose `rollback_acknowledged`. Git reality is the opposite: [WebPortal.h](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.h:42) and [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:592) changed, while `SystemStatus.cpp` did not. That makes the review trail unreliable.  
   Refs: [fn-1.6 story](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md:389)

7. **🟡 Minor: timeout warning state leaks into later runs.**  
   On timeout, the code hard-sets `otaRebootText.style.color = '#d29922'`, but neither `resetOtaUploadState()` nor `startRebootCountdown()` restores the normal color. After one timeout, later countdown/wait states stay amber even when the flow is healthy.  
   Refs: [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:2604), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:2816)

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 2 |
| SOLID Violations | 0 |
| Hidden Bugs | 3 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

## Evidence Score

| Severity | Source | Score |
|----------|--------|-------|
| 🔴 Critical | Missing cancel/reset path after file selection | +3 |
| 🔴 Critical | “Update at least every 2 seconds” not implemented | +3 |
| 🟠 Important | Timeout races final poll and overlaps requests | +1 |
| 🟠 Important | Success path accepts any `/api/status` response | +1 |
| 🟠 Important | No automated coverage for new behavior | +1 |
| 🟡 Minor | Story/file list does not match actual diff | +0.3 |
| 🟡 Minor | Timeout color state not reset | +0.3 |
| 🟢 Clean Pass | SOLID, Abstraction, Style, Type Safety, Security | -2.5 |

**Evidence Score: 7.1**

| Score | Verdict |
|-------|---------|
| **7.1** | **REJECT** |

## Additional Notes

No major new SOLID, style, or type-safety defects stood out in the reviewed diff. The failures here are mostly in behavioral correctness, incomplete task execution, missing regression coverage, and poor story-to-code traceability.

## Suggested Fixes

1. Add an explicit `Cancel` or `Choose Another File` control in the `.ota-file-info` state; never leave the user without a file-picker entry point.
2. Replace `setInterval` polling with a self-scheduling `setTimeout` loop that waits for each request to settle before issuing the next poll, and apply timeout only after the last in-flight request completes.
3. Validate upgrade success with a real state transition: compare old/new version, or at minimum require an uptime reset plus a fresh non-cached `/api/status` response.
4. Reset all OTA UI state in `resetOtaUploadState()`, including `otaRebootText.textContent` and `otaRebootText.style.color`.
5. Add tests for:
   - `POST /api/ota/ack-rollback`
   - `GET /api/settings/export`
   - rollback banner persistence/dismissal
   - OTA timeout behavior
   - reboot polling success/failure branches

**Review Actions**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires rework before this story should be accepted.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 0 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Potential dashboard load time regression (AC #1) due to increasing `dashboard.js` complexity. The file is already very large, and new JS adds to parse/execute time, without explicit load time measurements or optimizations. | `firmware/data-src/dashboard.js` | +1 |
| 🟠 IMPORTANT | Deviation from `ConfigManager` as the single source of truth for NVS interaction: direct `Preferences` access for `ota_rb_ack` in `WebPortal.cpp`. While justified as a transient flag, it's an architectural deviation. | `firmware/adapters/WebPortal.cpp` lines 520-523, 1146-1149 | +1 |
| 🟡 MINOR | Magic numbers for timer durations. | `firmware/adapters/WebPortal.cpp` lines 504, 551, 606 | +0.3 |
| 🟡 MINOR | `loadFirmwareStatus()` silently swallows errors from `/api/status`, potentially hiding issues with firmware information display. | `firmware/data-src/dashboard.js` line 2697 | +0.3 |
| 🟡 MINOR | Magic number `OTA_MAX_SIZE` for firmware file size limit. | `firmware/data-src/dashboard.js` line 2568 | +0.3 |
| 🟡 MINOR | Direct DOM style manipulation for `otaRebootText` color (`#d29922`) instead of using CSS classes/variables. | `firmware/data-src/dashboard.js` line 2795 | +0.3 |
| 🟡 MINOR | Duplicated drag-and-drop event listener setup logic between logo and OTA upload zones. | `firmware/data-src/dashboard.js` lines 1421-1436 and 2715-2733 | +0.3 |
| 🟡 MINOR | Polling timeout message could be more user-friendly for a general audience. | `firmware/data-src/dashboard.js` line 2794 | +0.3 |
| 🟡 MINOR | Hardcoded background color `#2d2738` in `.banner-warning` contradicts general use of CSS variables. | `firmware/data-src/style.css` line 527 | +0.3 |
| 🟢 CLEAN PASS | 3 |

### Evidence Score: 2.6

| Score | Verdict |
|-------|---------|
| **2.6** | **APPROVED** |

---

## 🏛️ Architectural Sins

- **Dependency Inversion / Consistency:** Deviation from `ConfigManager` as the single source of truth for NVS interaction. Direct `Preferences` access for `ota_rb_ack` in `WebPortal.cpp` bypasses `ConfigManager`'s NVS write patterns (debouncing, atomicity). While justified as a transient flag, it's an architectural deviation from the centralized NVS interface.
  - 📍 `firmware/adapters/WebPortal.cpp`:520
  - 💡 Fix: Consider encapsulating `ota_rb_ack` management within a dedicated NVS utility or extending `ConfigManager` with a `TransientFlag` category to maintain architectural consistency for all NVS interactions.

✅ No significant architectural violations detected beyond the noted NVS access pattern.

---

## 🐍 Pythonic Crimes &amp; Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance &amp; Scalability

- **[MEDIUM] Increased Load Time Risk:** Potential dashboard load time regression (AC #1) due to increasing `dashboard.js` complexity. The file is already very large, and new JS adds to parse/execute time, without explicit load time measurements or optimizations. This could push the dashboard load time over the 3-second target on slower devices or networks.
  - 📍 `firmware/data-src/dashboard.js`
  - 💡 Fix: Implement real-time performance monitoring for dashboard load. Explore techniques like lazy loading non-critical JS modules, code splitting, or refactoring large functions into smaller, more focused ones to reduce initial parse/execute time. Profile JavaScript execution to identify bottlenecks.

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug (Hidden Error):** `loadFirmwareStatus()` silently swallows errors from `/api/status`, potentially hiding issues with firmware information display (e.g., if the `/api/status` endpoint itself is broken or unreachable). The user would simply not see version info or the rollback banner without explicit feedback.
  - 📍 `firmware/data-src/dashboard.js`:2697
  - 🔄 Reproduction: Simulate a network error or a malformed response from `GET /api/status`. Observe that no user-facing toast or indication is given that the firmware card data could not be loaded.
- **🔒 [LOW] Security (Unauthenticated Endpoints):** `POST /api/ota/ack-rollback` and `GET /api/settings/export` are unauthenticated endpoints, consistent with the existing architectural design for the device (LAN-only). However, this means any user on the local network can dismiss rollback banners or download settings without authentication. While not introduced by *this* story, it's a standing security concern.
  - 📍 `firmware/adapters/WebPortal.cpp`:1145
  - ⚠️ Impact: Unauthorized access to system settings/state manipulation.

✅ Code appears correct and secure beyond the noted security concern which is an existing architectural decision.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Magic numbers for timer durations (500000 microseconds for OTA reboot, 1000000 microseconds for general reboot/reset). While commented, using named constants would improve readability and centralize values for easier modification.
  - 📍 `firmware/adapters/WebPortal.cpp`:504
  - 💥 Explosion radius: Low, but can lead to confusion if durations change and comments are not updated.
- **💣 Tech Debt:** Magic number `OTA_MAX_SIZE = 1572864` (1.5MB) for firmware file size limit in `dashboard.js`. A named constant derived from the build system's partition size (e.g., matching the C++ definition) would be more robust.
  - 📍 `firmware/data-src/dashboard.js`:2568
  - 💥 Explosion radius: Low, but if the partition size changes in the future, this JS constant would need manual update, increasing potential for mismatch errors.
- **💣 Tech Debt:** Direct DOM style manipulation for `otaRebootText` color (`#d29922`) instead of using CSS classes/variables. This bypasses the established CSS variable system and could lead to inconsistencies if the color scheme changes.
  - 📍 `firmware/data-src/dashboard.js`:2795
  - 💥 Explosion radius: Limited to this specific element, but sets a poor precedent.
- **💣 Tech Debt:** Duplicated drag-and-drop event listener setup logic between `logoUploadZone` and `otaUploadZone`. The core event binding and class toggling (`drag-over`) are identical. This is a missed opportunity for a reusable utility function or component, leading to more code to maintain.
  - 📍 `firmware/data-src/dashboard.js`:1421
  - 💥 Explosion radius: Increases code footprint, reduces reusability, and adds maintenance burden if drag-and-drop behavior needs to be updated.
- **💣 Tech Debt:** Polling timeout message "Device unreachable — try refreshing. The device may have changed IP address after reboot." in `dashboard.js` could be more user-friendly for a general audience. While fulfilling the AC, it's not optimal UX clarity.
  - 📍 `firmware/data-src/dashboard.js`:2794
  - 💥 Explosion radius: Impacts user experience, potentially increasing support queries.
- **💣 Tech Debt:** Hardcoded background color `#2d2738` in `.banner-warning` in `style.css`. While explicitly mentioned in the AC, it contradicts the general use of CSS variables for theming, creating an inconsistency in the stylesheet.
  - 📍 `firmware/data-src/style.css`:527
  - 💥 Explosion radius: Minor, but creates a one-off style that doesn't fit the design system.

---

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 0

✅ Code is approved and ready for deployment!

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| CRITICAL Issues | 3 |
| HIGH Issues | 3 |
| MEDIUM Issues | 3 |
| LOW Issues | 2 |
| **Total Issues** | **11** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Zero-byte file upload not rejected - passes client validation, server will accept | dashboard.js:2682-2710 | +3 |
| 🔴 CRITICAL | NVS write without error checking - Preferences can fail silently | WebPortal.cpp:629-633 | +3 |
| 🔴 CRITICAL | Polling race condition - success handler may not fire if response arrives after maxAttempts check | dashboard.js:2796-2821 | +3 |
| 🟠 HIGH | dismiss-btn lacks :focus style - WCAG 2.1 AA violation | style.css:571-575 | +1 |
| 🟠 HIGH | Upload button not disabled during async FileReader validation | dashboard.js:2700-2703 | +1 |
| 🟠 HIGH | dragleave removes class unconditionally - breaks on child element drag | dashboard.js:2663-2665 | +1 |
| 🟡 MEDIUM | No interval cleanup on page unload - memory leak potential | dashboard.js:2782,2797 | +0.3 |
| 🟡 MEDIUM | Duplicate dragenter/dragover handlers both add same class | dashboard.js:2655-2662 | +0.3 |
| 🟡 MEDIUM | aria-valuenow not set during initial upload state (remains at "0") | dashboard.js:2729-2734 | +0.3 |
| 🟢 LOW | Keyboard handler lacks repeat key protection | dashboard.js:2647-2651 | +0.1 |
| 🟢 LOW | Missing e.relatedTarget check in dragleave | dashboard.js:2663-2665 | +0.1 |

### Evidence Score: 13.1

| Score | Verdict |
|-------|---------|
| **13.1** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

**[9/10] Single Responsibility Violation:** The `loadFirmwareStatus()` function mixes UI rendering, API parsing, and conditional state logic without separation of concerns.
  - 📍 `dashboard.js:2615-2628`
  - 💡 Fix: Extract pure functions for data parsing and state computation

**[7/10] Inconsistent Error Handling Pattern:** NVS operations use direct `Preferences` API without the `ConfigLockGuard` pattern used elsewhere, creating potential race conditions with ConfigManager.
  - 📍 `WebPortal.cpp:617-621,629-633`
  - 💡 Fix: Wrap NVS ops in try/catch and use atomic transactions

**Over-engineering:** The `std::vector<OTAUploadState>` pattern for a single-flight upload adds unnecessary complexity when a simple global struct would suffice (established pattern per antipatterns doc, but still notable).
  - 📍 `WebPortal.cpp:95`

---

## 🐍 Code Quality & Readability

**Magic Numbers:**
- `1572864` (OTA_MAX_SIZE) has no inline comment explaining the 0x180000 hex derivation
- `3000` ms poll interval, `20` max attempts, `1000` countdown ms are all hardcoded
  - 📍 `dashboard.js:2602,2796,2782`

**Inconsistent Comment Style:**
- Mix of `/* best-effort */` and `// XHR required...` comment styles in same function scope
  - 📍 `dashboard.js:2627,2712`

---

## ⚡ Performance & Scalability

**[MEDIUM] Unnecessary ArrayBuffer allocation:**
- `file.slice(0, 4)` creates a new Blob; could use the File directly
  - 📍 `dashboard.js:2709`

**[LOW] polling creates new closure every 3 seconds:**
- The setInterval callback captures `attempts` and `pollInterval` creating closure overhead
  - 📍 `dashboard.js:2797`

---

## 🐛 Correctness & Safety

**🐛 CRITICAL BUG - Zero-byte File Acceptance:**
The client-side validation at line 2684 checks `file.size > OTA_MAX_SIZE` but does NOT check `file.size === 0`. A zero-byte file will:
1. Pass the size check (0 <= 1572864)
2. Cause FileReader to return empty ArrayBuffer
3. `bytes[0]` will be `undefined`, not `0xE9`
4. The comparison `undefined !== 0xE9` evaluates to `true`
5. File passes validation and gets uploaded
6. Server receives empty upload, Update.begin() may succeed but write 0 bytes
7. Device reboots with corrupted/cleared firmware

- 📍 `dashboard.js:2682-2710`
- 🔄 Reproduction: Create empty file `touch empty.bin`, select it in upload zone, observe it passes validation

**🐛 CRITICAL BUG - NVS Write Without Verification:**
The ack-rollback handler writes to NVS without checking return value:
```cpp
prefs.putUChar("ota_rb_ack", 1);  // Returns size_t, ignored
```
If the NVS partition is full or flash is worn, this silently fails but HTTP 200 OK is returned.

- 📍 `WebPortal.cpp:629-633`
- 🔄 Reproduction: Fill NVS partition, attempt to dismiss rollback banner

**🐛 CRITICAL BUG - Polling Race Condition:**
The polling logic checks `attempts >= maxAttempts` AFTER the async request starts. If:
1. attempts = 20 (last attempt)
2. FW.get() starts, takes >3s due to slow network
3. Interval fires again (attempts now 21 due to delayed previous callback)
4. maxAttempts check passes, timeout message shown
5. Original request returns success
6. Both success toast AND timeout message appear

- 📍 `dashboard.js:2796-2821`
- 🔄 Reproduction: Throttle network to 2G, start OTA, observe potential dual messaging

**🎭 Lying Accessibility:**
The progress bar claims `aria-valuenow="0"` during upload start, but actual upload progress is unknown. Screen readers report 0% when upload is actually in progress.

- 📍 `dashboard.js:2729-2734`

---

## 🔧 Maintainability Issues

**💣 Tech Debt - Hardcoded CSS color:**
`#d29922` (amber warning) is hardcoded in JS instead of using CSS variable:
```javascript
otaRebootText.style.color = '#d29922';
```
This breaks theming and dark mode support.

- 📍 `dashboard.js:2819`
- 💥 Explosion radius: Any theme changes require hunting through JS files

**💣 Missing Documentation:**
The `0xE9` magic byte check has no comment explaining it's the ESP32 app image magic byte.

- 📍 `dashboard.js:2694`

---

## 🔒 Security Vulnerabilities

**[MEDIUM] Unauthenticated Settings Export:**
The `/api/settings/export` endpoint returns ALL configuration including WiFi password and API keys without authentication. This is documented as acceptable in antipatterns (LAN-only device), but still a security gap.

- 📍 `WebPortal.cpp:643-670`

**[LOW] Timing Side-Channel:**
The rollback acknowledgment check uses NVS read on every `/api/status` call. Pattern of NVS access timing could reveal whether rollback was acknowledged.

- 📍 `WebPortal.cpp:772-776`

---

## ✅ Acceptance Criteria Verification

| AC | Status | Evidence |
|----|--------|----------|
| #1 - Version display, card expanded | ✅ PASS | dashboard.js:2619-2620, dashboard.html:234 |
| #2 - Upload zone accessibility | ✅ PASS | dashboard.html:241-242 |
| #3 - Drag states | ✅ PASS | dashboard.js:2655-2668 |
| #4 - Client validation (1.5MB, 0xE9) | ⚠️ PARTIAL | Size check OK, zero-byte bug found |
| #5 - Error toasts | ✅ PASS | dashboard.js:2685,2695 |
| #6 - Progress bar with XHR | ✅ PASS | dashboard.js:2720-2769 |
| #7 - Reboot countdown + polling | ✅ PASS | dashboard.js:2777-2822 |
| #8 - 60s timeout | ✅ PASS | dashboard.js:2816-2820 |
| #9 - Rollback banner | ✅ PASS | dashboard.html:237-240, dashboard.js:2622-2640 |
| #10 - Download Settings | ✅ PASS | dashboard.html:262, dashboard.js:2825-2831 |
| #11 - CSS ~35 lines | ✅ PASS | style.css:546-583 (~39 lines, acceptable) |
| #12 - Gzip assets | ✅ PASS | Documented in completion notes |

---

## 🛠️ Suggested Fixes

### Fix #1: Zero-byte File Check

**File:** `firmware/data-src/dashboard.js`
**Line:** 2682

```javascript
function validateAndSelectFile(file) {
  // Size check - must be > 0 and <= 1.5MB
  if (file.size === 0) {
    FW.showToast('File is empty', 'error');
    resetOtaUploadState();
    return;
  }
  if (file.size > OTA_MAX_SIZE) {
    FW.showToast('File too large \u2014 maximum 1.5MB for OTA partition', 'error');
    resetOtaUploadState();
    return;
  }
  // ... rest of validation
}
```

### Fix #2: NVS Write Error Check

**File:** `firmware/adapters/WebPortal.cpp`
**Lines:** 629-633

```cpp
_server->on("/api/ota/ack-rollback", HTTP_POST, [](AsyncWebServerRequest* request) {
    Preferences prefs;
    if (!prefs.begin("flightwall", false)) {
        _sendJsonError(request, 500, "NVS access failed", "NVS_ERROR");
        return;
    }
    size_t written = prefs.putUChar("ota_rb_ack", 1);
    prefs.end();
    
    if (written != sizeof(uint8_t)) {
        _sendJsonError(request, 500, "Failed to save acknowledgment", "NVS_WRITE_ERROR");
        return;
    }
    
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
});
```

### Fix #3: Polling Race Condition Fix

**File:** `firmware/data-src/dashboard.js`
**Lines:** 2794-2822

```javascript
function startRebootPolling() {
    var attempts = 0;
    var maxAttempts = 20;
    var pollInterval = setInterval(function() {
        // Check BEFORE incrementing to prevent race
        if (attempts >= maxAttempts) {
            clearInterval(pollInterval);
            otaRebootText.textContent = 'Device unreachable \u2014 try refreshing...';
            otaRebootText.style.color = '#d29922';
            return;
        }
        attempts++;
        
        FW.get('/api/status').then(function(res) {
            // Only process if interval still active
            if (attempts > maxAttempts) return;
            if (res.body && res.body.ok && res.body.data) {
                clearInterval(pollInterval);
                // ... success handling
            }
        }).catch(function() {});
    }, 3000);
}
```

### Fix #4: dismiss-btn Focus Style

**File:** `firmware/data-src/style.css`
**After line 575:**

```css
.banner-warning .dismiss-btn:focus {
    outline: 2px solid var(--primary);
    outline-offset: 2px;
}
```

---

## Review Actions

| Category | Count |
|----------|-------|
| Issues Found | 11 |
| CRITICAL | 3 |
| HIGH | 3 |
| MEDIUM | 3 |
| LOW | 2 |

**Verdict: MAJOR REWORK** ⚠️

The implementation functionally meets all 12 acceptance criteria, but 3 critical bugs related to zero-byte file handling, NVS error handling, and polling race conditions must be addressed before deployment. The code quality is generally good but has accessibility gaps and minor maintainability issues.

**Priority Fixes:**
1. Zero-byte file validation (CRITICAL)
2. NVS write error checking (CRITICAL)
3. Polling race condition (CRITICAL)
4. dismiss-btn focus indicator (HIGH - accessibility)
5. Upload button disabled state during validation (HIGH)

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-1</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">3</var>
<var name="session_id">65162522-0c67-4e0d-9ad1-e4936eba1c36</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="60733e24">embedded in prompt, file id: 60733e24</var>
<var name="story_id">fn-1.6</var>
<var name="story_key">fn-1-6-dashboard-firmware-card-and-ota-upload-ui</var>
<var name="story_num">6</var>
<var name="story_title">6-dashboard-firmware-card-and-ota-upload-ui</var>
<var name="template">False</var>
<var name="timestamp">20260412_2030</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="validator_count"></var>
</variables>
<instructions><workflow>
  <critical>Communicate all responses in English and generate all documents in English</critical>
  <critical>You are the MASTER SYNTHESIS agent for CODE REVIEW findings.</critical>
  <critical>You have WRITE PERMISSION to modify SOURCE CODE files and story Dev Agent Record section.</critical>
  <critical>DO NOT modify story context (AC, Dev Notes content) - only Dev Agent Record (task checkboxes, completion notes, file list).</critical>
  <critical>All context (project_context.md, story file, anonymized reviews) is EMBEDDED below - do NOT attempt to read files.</critical>

  <step n="1" goal="Analyze reviewer findings">
    <action>Read all anonymized reviewer outputs (Reviewer A, B, C, D, etc.)</action>
    <action>For each issue raised:
      - Cross-reference with embedded project_context.md and story file
      - Cross-reference with source code snippets provided in reviews
      - Determine if issue is valid or false positive
      - Note reviewer consensus (if 3+ reviewers agree, high confidence issue)
    </action>
    <action>Issues with low reviewer agreement (1-2 reviewers) require extra scrutiny</action>
    <action>Group related findings that address the same underlying problem</action>
  </step>

  <step n="1.5" goal="Review Deep Verify code analysis" conditional="[Deep Verify Findings] section present">
    <critical>Deep Verify analyzed the actual source code files for this story.
      DV findings are based on static analysis patterns and may identify issues reviewers missed.</critical>

    <action>Review each DV finding:
      - CRITICAL findings: Security vulnerabilities, race conditions, resource leaks - must address
      - ERROR findings: Bugs, missing error handling, boundary issues - should address
      - WARNING findings: Code quality concerns - consider addressing
    </action>

    <action>Cross-reference DV findings with reviewer findings:
      - DV + Reviewers agree: High confidence issue, prioritize in fix order
      - Only DV flags: Verify in source code - DV has precise line numbers
      - Only reviewers flag: May be design/logic issues DV can't detect
    </action>

    <action>DV findings may include evidence with:
      - Code quotes (exact text from source)
      - Line numbers (precise location, when available)
      - Pattern IDs (known antipattern reference)
      Use this evidence when applying fixes.</action>

    <action>DV patterns reference:
      - CC-*: Concurrency issues (race conditions, deadlocks)
      - SEC-*: Security vulnerabilities
      - DB-*: Database/storage issues
      - DT-*: Data transformation issues
      - GEN-*: General code quality (null handling, resource cleanup)
    </action>
  </step>

  <step n="2" goal="Verify issues and identify false positives">
    <action>For each issue, verify against embedded code context:
      - Does the issue actually exist in the current code?
      - Is the suggested fix appropriate for the codebase patterns?
      - Would the fix introduce new issues or regressions?
    </action>
    <action>Document false positives with clear reasoning:
      - Why the reviewer was wrong
      - What evidence contradicts the finding
      - Reference specific code or project_context.md patterns
    </action>
  </step>

  <step n="3" goal="Prioritize by severity">
    <action>For verified issues, assign severity:
      - Critical: Security vulnerabilities, data corruption, crashes
      - High: Bugs that break functionality, performance issues
      - Medium: Code quality issues, missing error handling
      - Low: Style issues, minor improvements, documentation
    </action>
    <action>Order fixes by severity - Critical first, then High, Medium, Low</action>
    <action>For disputed issues (reviewers disagree), note for manual resolution</action>
  </step>

  <step n="4" goal="Apply fixes to source code">
    <critical>This is SOURCE CODE modification, not story file modification</critical>
    <critical>Use Edit tool for all code changes - preserve surrounding code</critical>
    <critical>After applying each fix group, run: pytest -q --tb=line --no-header</critical>
    <critical>NEVER proceed to next fix if tests are broken - either revert or adjust</critical>

    <action>For each verified issue (starting with Critical):
      1. Identify the source file(s) from reviewer findings
      2. Apply fix using Edit tool - change ONLY the identified issue
      3. Preserve code style, indentation, and surrounding context
      4. Log the change for synthesis report
    </action>

    <action>After each logical fix group (related changes):
      - Run: pytest -q --tb=line --no-header
      - If tests pass, continue to next fix
      - If tests fail:
        a. Analyze which fix caused the failure
        b. Either revert the problematic fix OR adjust implementation
        c. Run tests again to confirm green state
        d. Log partial fix failure in synthesis report
    </action>

    <action>Atomic commit guidance (for user reference):
      - Commit message format: fix(component): brief description (synthesis-fn-1.6)
      - Group fixes by severity and affected component
      - Never commit unrelated changes together
      - User may batch or split commits as preferred
    </action>
  </step>

  <step n="5" goal="Refactor if needed">
    <critical>Only refactor code directly related to applied fixes</critical>
    <critical>Maximum scope: files already modified in Step 4</critical>

    <action>Review applied fixes for duplication patterns:
      - Same fix applied 2+ times across files = candidate for refactor
      - Only if duplication is in files already modified
    </action>

    <action>If refactoring:
      - Extract common logic to shared function/module
      - Update all call sites in modified files
      - Run tests after refactoring: pytest -q --tb=line --no-header
      - Log refactoring in synthesis report
    </action>

    <action>Do NOT refactor:
      - Unrelated code that "could be improved"
      - Files not touched in Step 4
      - Patterns that work but are just "not ideal"
    </action>

    <action>If broader refactoring needed:
      - Note it in synthesis report as "Suggested future improvement"
      - Do not apply - leave for dedicated refactoring story
    </action>
  </step>

  <step n="6" goal="Generate synthesis report">
    <critical>When updating story file, use atomic write pattern (temp file + rename).</critical>
    <action>Update story file Dev Agent Record section ONLY:
      - Mark completed tasks with [x] if fixes address them
      - Append to "Completion Notes List" subsection summarizing changes applied
      - Update file list with all modified files
    </action>

    <critical>Your synthesis report MUST be wrapped in HTML comment markers for extraction:</critical>
    <action>Produce structured output in this exact format (including the markers):</action>
    <output-format>
&lt;!-- CODE_REVIEW_SYNTHESIS_START --&gt;
## Synthesis Summary
[Brief overview: X issues verified, Y false positives dismissed, Z fixes applied to source files]

## Validations Quality
[For each reviewer: ID (A, B, C...), score (1-10), brief assessment]
[Note: Reviewers are anonymized - do not attempt to identify providers]

## Issues Verified (by severity)

### Critical
[Issues that required immediate fixes - list with evidence and fixes applied]
[Format: "- **Issue**: Description | **Source**: Reviewer(s) | **File**: path | **Fix**: What was changed"]
[If none: "No critical issues identified."]

### High
[Bugs and significant problems - same format]

### Medium
[Code quality issues - same format]

### Low
[Minor improvements - same format, note any deferred items]

## Issues Dismissed
[False positives with reasoning for each dismissal]
[Format: "- **Claimed Issue**: Description | **Raised by**: Reviewer(s) | **Dismissal Reason**: Why this is incorrect"]
[If none: "No false positives identified."]

## Changes Applied
[Complete list of modifications made to source files]
[Format for each change:
  **File**: [path/to/file.py]
  **Change**: [Brief description]
  **Before**:
  ```
  [2-3 lines of original code]
  ```
  **After**:
  ```
  [2-3 lines of updated code]
  ```
]
[If no changes: "No source code changes required."]

## Deep Verify Integration
[If DV findings were present, document how they were handled]

### DV Findings Fixed
[List DV findings that resulted in code changes]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **File**: {path} | **Fix**: {What was changed}"]

### DV Findings Dismissed
[List DV findings determined to be false positives]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Reason**: {Why this is not an issue}"]

### DV-Reviewer Overlap
[Note findings flagged by both DV and reviewers - highest confidence fixes]
[If no DV findings: "Deep Verify did not produce findings for this story."]

## Files Modified
[Simple list of all files that were modified]
- path/to/file1.py
- path/to/file2.py
[If none: "No files modified."]

## Suggested Future Improvements
[Broader refactorings or improvements identified in Step 5 but not applied]
[Format: "- **Scope**: Description | **Rationale**: Why deferred | **Effort**: Estimated complexity"]
[If none: "No future improvements identified."]

## Test Results
[Final test run output summary]
- Tests passed: X
- Tests failed: 0 (required for completion)
&lt;!-- CODE_REVIEW_SYNTHESIS_END --&gt;
    </output-format>

  </step>

  <step n="6.5" goal="Write Senior Developer Review section to story file for dev_story rework detection">
    <critical>This section enables dev_story to detect that a code review has occurred and extract action items.</critical>
    <critical>APPEND this section to the story file - do NOT replace existing content.</critical>

    <action>Determine the evidence verdict from the [Evidence Score] section:
      - REJECT: Evidence score exceeds reject threshold
      - PASS: Evidence score is below accept threshold
      - UNCERTAIN: Evidence score is between thresholds
    </action>

    <action>Map evidence verdict to review outcome:
      - PASS → "Approved"
      - REJECT → "Changes Requested"
      - UNCERTAIN → "Approved with Reservations"
    </action>

    <action>Append to story file "## Senior Developer Review (AI)" section:
      ```
      ## Senior Developer Review (AI)

      ### Review: {current_date}
      - **Reviewer:** AI Code Review Synthesis
      - **Evidence Score:** {evidence_score} → {evidence_verdict}
      - **Issues Found:** {total_verified_issues}
      - **Issues Fixed:** {fixes_applied_count}
      - **Action Items Created:** {remaining_unfixed_count}
      ```
    </action>

    <critical>When evidence verdict is REJECT, you MUST create Review Follow-ups tasks.
      If "Action Items Created" count is &gt; 0, there MUST be exactly that many [ ] [AI-Review] tasks.
      Do NOT skip this step. Do NOT claim all issues are fixed if you reported deferred items above.</critical>

    <action>Find the "## Tasks / Subtasks" section in the story file</action>
    <action>Append a "#### Review Follow-ups (AI)" subsection with checkbox tasks:
      ```
      #### Review Follow-ups (AI)
      - [ ] [AI-Review] {severity}: {brief description of unfixed issue} ({file path})
      ```
      One line per unfixed/deferred issue, prefixed with [AI-Review] tag.
      Order by severity: Critical first, then High, Medium, Low.
    </action>

    <critical>ATDD DEFECT CHECK: Search test directories (tests/**) for test.fixme() calls in test files related to this story.
      If ANY test.fixme() calls remain in story-related test files, this is a DEFECT — the dev_story agent failed to activate ATDD RED-phase tests.
      Create an additional [AI-Review] task:
      - [ ] [AI-Review] HIGH: Activate ATDD tests — convert all test.fixme() to test() and ensure they pass ({test file paths})
      Do NOT dismiss test.fixme() as "intentional TDD methodology". After dev_story completes, ALL test.fixme() tests for the story MUST be converted to test().</critical>
  </step>

  </workflow></instructions>
<output-template></output-template>
</compiled-workflow>