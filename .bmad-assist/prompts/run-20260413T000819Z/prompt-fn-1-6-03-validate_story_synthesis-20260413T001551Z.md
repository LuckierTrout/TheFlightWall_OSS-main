<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 6 -->
<!-- Phase: validate-story-synthesis -->
<!-- Timestamp: 20260413T001551Z -->
<compiled-workflow>
<mission><![CDATA[

Master Synthesis: Story fn-1.6

You are synthesizing 2 independent validator reviews.

Your mission:
1. VERIFY each issue raised by validators
   - Cross-reference with story content
   - Identify false positives (issues that aren't real problems)
   - Confirm valid issues with evidence

2. PRIORITIZE real issues by severity
   - Critical: Blocks implementation or causes major problems
   - High: Significant gaps or ambiguities
   - Medium: Improvements that would help
   - Low: Nice-to-have suggestions

3. SYNTHESIZE findings
   - Merge duplicate issues from different validators
   - Note validator consensus (if 3+ agree, high confidence)
   - Highlight unique insights from individual validators

4. APPLY changes to story file
   - You have WRITE PERMISSION to modify the story
   - CRITICAL: Before using Edit tool, ALWAYS Read the target file first
   - Use EXACT content from Read tool output as old_string, NOT content from this prompt
   - If Read output is truncated, use offset/limit parameters to locate the target section
   - Apply fixes for verified issues
   - Document what you changed and why

Output format:
## Synthesis Summary
## Issues Verified (by severity)
## Issues Dismissed (false positives with reasoning)
## Changes Applied

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

Status: ready-for-dev

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

- [ ] **Task 1: Add Firmware card HTML to dashboard.html** (AC: #1, #2, #3, #10)
  - [ ] Add new `<section class="card" id="firmware-card">` between the Logos card and System card
  - [ ] Card content: `<h2>Firmware</h2>`, version display `<p id="fw-version">`, rollback banner placeholder `<div id="rollback-banner">`, OTA upload zone, progress bar placeholder, reboot countdown placeholder
  - [ ] OTA Upload Zone: `<div class="ota-upload-zone" id="ota-upload-zone" role="button" tabindex="0" aria-label="Select firmware file for upload">` with prompt text "Drag .bin file here or tap to select" and hidden file input `<input type="file" id="ota-file-input" accept=".bin" hidden>`
  - [ ] File selection display: `<div class="ota-file-info" id="ota-file-info" style="display:none">` with filename span and "Upload Firmware" primary button
  - [ ] Progress bar: `<div class="ota-progress" id="ota-progress" style="display:none">` with inner bar, percentage text, and `role="progressbar"` attributes
  - [ ] Reboot countdown: `<div class="ota-reboot" id="ota-reboot" style="display:none">` with countdown text
  - [ ] Add helper text: `<p class="helper-copy">Before migrating, download your settings backup from <a href="#system-card">System</a> below.</p>`
  - [ ] Add `id="system-card"` to the existing System card `<section>` for the anchor link

- [ ] **Task 2: Add "Download Settings" button to System card** (AC: #10)
  - [ ] In the System card section (currently has only Reset to Defaults), add a "Download Settings" secondary button: `<button type="button" class="btn-secondary" id="btn-export-settings">Download Settings</button>`
  - [ ] Add helper text below: `<p class="helper-copy">Download your configuration as a JSON file before partition migration or reflash.</p>`
  - [ ] Place this ABOVE the danger zone reset button (less destructive action first)

- [ ] **Task 3: Add new endpoint POST /api/ota/ack-rollback** (AC: #9)
  - [ ] In `WebPortal.cpp`, in `_registerRoutes()`, add handler for `POST /api/ota/ack-rollback`
  - [ ] Handler: set NVS key `ota_rb_ack` to `1` (uint8), respond `{ "ok": true }`
  - [ ] In `WebPortal.h`, no new method needed — can be inline lambda in `_registerRoutes()` (same pattern as positioning handlers)

- [ ] **Task 4: Add GET /api/settings/export endpoint** (AC: #10)
  - [ ] In `WebPortal.cpp`, in `_registerRoutes()`, add handler for `GET /api/settings/export`
  - [ ] Handler builds JSON using existing `ConfigManager::dumpSettingsJson()` method
  - [ ] Add metadata fields: `"flightwall_settings_version": 1`, `"exported_at": "<ISO-8601 timestamp>"` (use `time()` with NTP or `millis()` fallback)
  - [ ] Set response header: `Content-Disposition: attachment; filename=flightwall-settings.json`
  - [ ] Response content-type: `application/json`

- [ ] **Task 5: Expose rollback acknowledgment state in /api/status** (AC: #9)
  - [ ] Add NVS key `ota_rb_ack` (uint8, default 0) to ConfigManager — or read directly in WebPortal since it's a simple boolean flag
  - [ ] In `SystemStatus::toExtendedJson()`, add `"rollback_acknowledged"` field: `true` if NVS key `ota_rb_ack` == 1
  - [ ] Alternative simpler approach: The rollback banner logic can be entirely client-side by checking `rollback_detected` from `/api/status` and storing acknowledgment in `localStorage`. BUT the epic AC says "NVS-backed" persistence, so the POST endpoint approach is required.
  - [ ] When a NEW OTA upload succeeds (fn-1.3 handler), clear the `ota_rb_ack` key by setting it to `0` — so the next rollback will show the banner again

- [ ] **Task 6: Add OTA CSS to style.css** (AC: #11)
  - [ ] `.ota-upload-zone` — Reuse pattern from `.logo-upload-zone`: dashed border `2px dashed var(--border)`, padding, min-height 80px, cursor pointer, text-align center, flex column
  - [ ] `.ota-upload-zone.drag-over` — Solid border `var(--primary)`, background rgba tint
  - [ ] `.ota-file-info` — Filename + button row; flexbox with gap
  - [ ] `.ota-progress` — Container for progress bar: height 28px, border-radius, background `var(--border)`
  - [ ] `.ota-progress-bar` — Inner fill: height 100%, background `var(--primary)`, transition width 0.3s, border-radius
  - [ ] `.ota-progress-text` — Centered percentage text over bar: position absolute, color white
  - [ ] `.ota-reboot` — Countdown text: text-align center, padding, font-size larger
  - [ ] `.banner-warning` — Persistent amber banner: background `#2d2738`, border-left 4px solid `var(--warning, #d29922)`, padding 12px 16px, border-radius, display flex, justify-content space-between, align-items center
  - [ ] `.banner-warning .dismiss-btn` — Small X button: min 44x44px touch target
  - [ ] `.btn-secondary` — If not already existing: outline style button, border `var(--border)`, background transparent, color `var(--text)`
  - [ ] Target: ~35 lines, ~150 bytes gzipped
  - [ ] All interactive elements ≥ 44x44px touch targets
  - [ ] Verify contrast ratios meet WCAG 2.1 AA (4.5:1 for text, 3:1 for UI components)

- [ ] **Task 7: Implement firmware card JavaScript in dashboard.js** (AC: #1, #2, #3, #4, #5, #6, #7, #8, #9)
  - [ ] **Version display**: In `loadSettings()` or a new `loadFirmwareStatus()` function, call `GET /api/status`, extract `firmware_version` and `rollback_detected`, populate `#fw-version` text
  - [ ] **Rollback banner**: If `rollback_detected === true` AND not acknowledged (check via `rollback_acknowledged` field in status response), show `.banner-warning` with dismiss button. Dismiss handler: `FW.post('/api/ota/ack-rollback')`, then hide banner.
  - [ ] **File selection via click**: Click on `.ota-upload-zone` or Enter/Space key → trigger hidden `#ota-file-input` click
  - [ ] **Drag and drop**: `dragenter`/`dragover` on zone → add `.drag-over` class, prevent default. `dragleave`/`drop` → remove class. On `drop`, extract `e.dataTransfer.files[0]`
  - [ ] **Client-side validation**: Read first 4 bytes via `FileReader.readAsArrayBuffer()`. Check: (a) file size ≤ 1,572,864 bytes (1.5MB = 0x180000), (b) first byte === 0xE9 (ESP32 magic). On failure: `FW.showToast(message, 'error')` and reset zone.
  - [ ] **File info display**: On valid file, hide upload zone, show `#ota-file-info` with filename and enabled "Upload Firmware" button
  - [ ] **Upload via XMLHttpRequest**: Use `XMLHttpRequest` (not `fetch`) for upload progress events. `xhr.upload.onprogress` → update progress bar width and `aria-valuenow`. `FormData` with file appended. POST to `/api/ota/upload`.
  - [ ] **Progress bar**: Hide file info, show `#ota-progress`. Update bar width as percentage. Ensure updates at least every 2 seconds (XHR progress events are browser-driven; add a minimum interval check if needed).
  - [ ] **Success handler**: On `xhr.status === 200` and `response.ok === true`, show countdown "Rebooting in 3... 2... 1..." via `setInterval` decrementing. After countdown, begin polling.
  - [ ] **Reboot polling**: `setInterval` every 3 seconds calling `GET /api/status`. On success: extract new `firmware_version`, show success toast "Updated to v[version]", update version display, reset OTA zone to initial state. Clear interval.
  - [ ] **Polling timeout**: After 60 seconds (20 attempts at 3s each), stop polling, show amber message "Device unreachable — try refreshing. The device may have changed IP address after reboot."
  - [ ] **Error handling**: On upload failure (non-200 response), parse error JSON, show error toast with specific message from server. Reset upload zone to initial state.
  - [ ] **Cancel/reset**: Provide a way to cancel file selection (click zone again or select new file)

- [ ] **Task 8: Add settings export JavaScript** (AC: #10)
  - [ ] Click handler on `#btn-export-settings`: trigger `window.location.href = '/api/settings/export'` — browser downloads the JSON file via Content-Disposition header
  - [ ] Alternative: `fetch('/api/settings/export')` then create blob URL and trigger download — but direct navigation is simpler and works universally

- [ ] **Task 9: Gzip updated web assets** (AC: #12)
  - [ ] From `firmware/` directory, regenerate gzipped assets:
    ```
    gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
    gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz
    gzip -9 -c data-src/style.css > data/style.css.gz
    ```
  - [ ] Verify gzipped files replaced in `firmware/data/`

- [ ] **Task 10: Build and verify** (AC: #1-#12)
  - [ ] Run `~/.platformio/penv/bin/pio run` from `firmware/` — verify clean build
  - [ ] Verify binary size remains under 1.5MB limit
  - [ ] Measure total gzipped web asset size (should remain well under 50KB budget)

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

When a new OTA upload starts (in the existing upload handler in WebPortal.cpp, at the `index == 0` first-chunk block), clear the rollback ack:
```cpp
// In the OTA upload handler, after setting g_otaInProgress = true:
Preferences prefs;
prefs.begin("flightwall", false);
prefs.putUChar("ota_rb_ack", 0);
prefs.end();
```

This ensures that if the new upload causes another rollback, the banner will reappear.

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
4. `firmware/adapters/WebPortal.cpp` — Add `POST /api/ota/ack-rollback` and `GET /api/settings/export` endpoints, clear `ota_rb_ack` on new OTA start
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

4. **NVS write during OTA:** The `ota_rb_ack` NVS write in the ack-rollback handler is small and fast. The NVS write during OTA upload start (clearing the flag) happens before the streaming begins, so no heap conflict.

5. **Settings export without NTP:** If NTP hasn't synced, `time()` returns near-zero. The export includes an `exported_at` field — use an uptime-based fallback string rather than a misleading timestamp.

## Dev Agent Record

### Agent Model Used

Claude Opus 4 (Story Creation)

### Debug Log References

N/A — Story creation phase

### Completion Notes List

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

### File List

- `firmware/data-src/dashboard.html` (MODIFIED) — Add Firmware card HTML, System card ID, Download Settings button
- `firmware/data-src/dashboard.js` (MODIFIED) — Add firmware status loading, OTA upload UI logic, reboot polling, settings export trigger
- `firmware/data-src/style.css` (MODIFIED) — Add ~35 lines of OTA component styles
- `firmware/adapters/WebPortal.cpp` (MODIFIED) — Add POST /api/ota/ack-rollback, GET /api/settings/export endpoints; clear ota_rb_ack on new OTA
- `firmware/data/dashboard.html.gz` (REGENERATED) — Gzipped dashboard HTML
- `firmware/data/dashboard.js.gz` (REGENERATED) — Gzipped dashboard JS
- `firmware/data/style.css.gz` (REGENERATED) — Gzipped CSS


]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

# 🎯 Story Context Validation Report

<!-- report_header -->

**Story:** fn-1-6-dashboard-firmware-card-and-ota-upload-ui - Dashboard Firmware Card & OTA Upload UI
**Story File:** _bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md
**Validated:** 2026-04-12
**Validator:** Quality Competition Engine

---

<!-- executive_summary -->

## Executive Summary

### Issues Overview

| Category | Found | Applied |
|----------|-------|---------|
| 🚨 Critical Issues | 2 | 0 |
| ⚡ Enhancements | 4 | 0 |
| ✨ Optimizations | 1 | 0 |
| 🤖 LLM Optimizations | 0 | 0 |

**Overall Assessment:** MAJOR REWORK

---

<!-- evidence_score_summary -->

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | AC #9 specifies NVS-backed persistence for the rollback banner, but the implementation detail in Task 5 (Alternative simpler approach) and the Dev Notes (Implementation choice) contradict this by suggesting direct NVS access outside of `ConfigManager`, and the story even presents `localStorage` as an alternative, despite the AC explicitly stating "NVS-backed". This creates ambiguity and potential for non-compliance with the architectural pattern for persistent settings. | AC #9, Task 5, Dev Notes | +3 |
| 🔴 CRITICAL | AC #10 states: "helper text in the Firmware card links to the System card: 'Before migrating, download your settings backup from System below'". This implies `POST /api/ota/ack-rollback` should also reset `ota_rb_ack` key to 0, which is currently done in the OTA upload handler, not the `ack-rollback` handler. | AC #10, Dev Notes | +3 |
| 🟠 IMPORTANT | AC #5 validation message "File too large — maximum 1.5MB for OTA partition" uses "1.5MB", but the actual size limit is 1,572,864 bytes (0x180000), which is precisely 1.5MiB, not 1.5MB. While "1.5MB" is commonly understood, for precision in a technical context (especially for validation messages), it should either state 1.5 MiB or the exact byte count. | AC #5, Dev Notes | +1 |
| 🟠 IMPORTANT | AC #7 states "polling begins for device reachability (`GET /api/status`)". It doesn't explicitly state that the polling should *only* stop when the `firmware_version` changes or when a timeout occurs, leading to potential infinite polling or incorrect termination. | AC #7 | +1 |
| 🟠 IMPORTANT | The CSS section in Task 6 provides an approximate line count ("approximately 35 lines") and gzipped size ("~150 bytes gzipped") but doesn't mention how to *verify* these estimations. | Task 6 | +1 |
| 🟠 IMPORTANT | The story mentions `FW_VERSION` macro for `firmware_version` in `GET /api/status` (AC #5), but doesn't explicitly state where `FW_VERSION` is defined or how it's managed, especially in relation to `platformio.ini`. | AC #5, Task 4, Dev Notes | +1 |
| 🟡 MINOR | The term "Firmware card" is used inconsistently in AC #1 and AC #2. AC #1 refers to "the Firmware card" and then "the Firmware card", while AC #2 says "the Firmware card" and then ".ota-upload-zone". It would be clearer to consistently refer to the specific UI element being described. | AC #1, AC #2 | +0.3 |

### Evidence Score: 8.3

| Score | Verdict |
|-------|---------|
| **8.3** | **REJECT** |

---

<!-- story_quality_gate -->

## 🎯 Ruthless Story Validation fn-1.6

### INVEST Criteria Assessment

| Criterion | Status | Severity | Details |
|-----------|--------|----------|---------|
| **I**ndependent | ✅ PASS | 0/10 | Story has clearly defined dependencies on fn-1.1, fn-1.3, fn-1.4 which are all marked COMPLETE. There are no hidden or circular dependencies that would block independent development of this story. |
| **N**egotiable | ✅ PASS | 0/10 | The story specifies "what" needs to be built (Firmware card, upload UI, specific API endpoints) but allows flexibility in "how" the client-side JavaScript or CSS is implemented, as long as it meets the ACs and aesthetic guidelines. |
| **V**aluable | ✅ PASS | 0/10 | The story delivers clear value by enabling firmware updates over-the-air, reducing the need for physical device access, and providing critical feedback on firmware status. This directly contributes to a robust user experience and device maintainability. |
| **E**stimable | 🟠 FAIL | 7/10 | The story relies heavily on client-side JavaScript implementation, which can be complex to estimate due to browser inconsistencies and interactions with existing UI components. The estimate relies on assumptions about `dashboard.js` structure and potential refactoring needed. The lack of detailed interaction flows beyond basic state transitions could lead to underestimation. |
| **S**mall | 🟠 FAIL | 5/10 | The story is quite large, encompassing significant UI development (new card, upload zone, progress bar, countdown, rollback banner), new API endpoints, integration with existing APIs, and manual asset gzipping. This could easily exceed a single sprint. |
| **T**estable | 🟠 FAIL | 6/10 | While most ACs are testable, AC #5's client-side validation for "1.5MB" uses an imprecise measurement. AC #7's polling mechanism lacks explicit criteria for stopping, which could be an issue. AC #11's WCAG and touch target requirements are general and would require specialized manual testing which is not explicitly called out as a test strategy. |

### INVEST Violations

- **[7/10] Estimable:** The story combines significant UI work (new card, multiple interactive states for upload, progress, countdown, banner) with new API endpoints and client-side validation logic. This breadth, especially in a file like `dashboard.js` which is already noted as the largest web asset, makes it highly susceptible to underestimation. The reliance on `XMLHttpRequest` for progress adds another layer of complexity compared to standard `fetch`.
- **[5/10] Small:** The scope is extensive. It involves HTML, CSS, JavaScript, and C++ (for new API endpoints and NVS interaction). The JavaScript component alone includes file selection, client-side validation (magic byte, size), XHR for progress, reboot countdown, device polling, and persistent banner logic. This is arguably multiple smaller stories.
- **[6/10] Testable:** AC #5 refers to "1.5MB" which is ambiguous. AC #7's polling mechanism lacks explicit termination conditions. AC #11's accessibility checks are critical but vague for automated testing.

### Acceptance Criteria Issues

- **Ambiguous Criteria:** AC #5 specifies "File too large — maximum 1.5MB for OTA partition". This is ambiguous. Is it 1.5 megabytes (1,500,000 bytes) or 1.5 mebibytes (1,572,864 bytes)? Given `0x180000` is used in the code for 1.5MiB, the AC should use precise terminology (e.g., 1.5 MiB or the exact byte count).
  - *Quote:* "File too large — maximum 1.5MB for OTA partition"
  - *Recommendation:* Change "1.5MB" to "1.5 MiB (1,572,864 bytes)" or "approximately 1.5 MB".
- **Incomplete Scenario:** AC #7 states "polling begins for device reachability (`GET /api/status`)". It doesn't explicitly define the criteria for when this polling *stops* successfully beyond "on successful poll, the new version string is displayed". It doesn't mention if the new version string *must be different* from the old one, which could be a critical success condition.
  - *Quote:* "polling begins for device reachability (`GET /api/status`) **And** on successful poll, the new version string is displayed **And** a success toast shows "Updated to v[new version]""
  - *Recommendation:* Clarify that polling stops when `GET /api/status` returns successfully AND the `firmware_version` in the response is different from the version *before* the update.
- **Conflicting/Ambiguous Persistence:** AC #9 states that the rollback banner persists across page refreshes "until dismissed (NVS-backed)". However, Task 5 proposes reading directly from NVS, bypassing `ConfigManager`, and even mentions `localStorage` as an alternative. The Dev Notes "Implementation choice" then suggests direct NVS access in WebPortal, avoiding `ConfigManager`. This creates conflicting guidance about how this "NVS-backed" persistence should be achieved and whether it should leverage existing `ConfigManager` patterns.
  - *Quote:* "a Persistent Banner (`.banner-warning`) appears in the Firmware card: "Firmware rolled back to previous version" **And** the banner has `role="alert"`, `aria-live="polite"`, hardcoded `#2d2738` background **And** a dismiss button sends `POST /api/ota/ack-rollback` and removes the banner **And** the banner persists across page refreshes until dismissed (NVS-backed)"
  - *Recommendation:* Reiterate the architectural preference for `ConfigManager` for all NVS access unless explicitly justified otherwise, or clearly state that for simple flags like this, direct `Preferences` access is acceptable and becomes the new pattern for such use cases. Remove the mention of `localStorage` as it directly conflicts with "NVS-backed".
- **Missing Action for `ack-rollback`:** AC #9 and AC #10 collectively imply a need to clear the `ota_rb_ack` NVS key upon *successful OTA upload* to ensure the banner reappears on subsequent rollbacks. While the Dev Notes mention this, AC #9 should explicitly state the behavior of `POST /api/ota/ack-rollback` beyond just dismissing the banner, i.e., it marks the *current* `rollback_detected` state as acknowledged, but a *new* successful OTA should re-enable the warning if another rollback happens.
  - *Quote:* "a dismiss button sends `POST /api/ota/ack-rollback` and removes the banner"
  - *Recommendation:* Add to AC #9: "and subsequent successful OTA uploads will reset the acknowledgment flag, allowing the banner to reappear if a new rollback is detected."

### Hidden Risks and Dependencies

- **Implicit CSS Verification Process:** AC #11 makes specific claims about CSS line count, gzipped size, WCAG contrast, and touch targets. These imply a manual verification process or specific tooling that isn't detailed. The developer is left to guess how to "verify" these or if they're simply aspirational targets.
  - *Impact:* Leads to incomplete or unreliable verification, potential for non-compliance with NFRs.
  - *Mitigation:* Explicitly state the verification methods for AC #11 (e.g., "manual audit using browser dev tools for contrast/touch targets", "use `wcag-checker` CLI tool", "verify gzipped size with `ls -l data/*.gz`").
- **External Tooling Dependency:** The manual `gzip` command in the Dev Notes highlights a dependency on the `gzip` utility. This is a common tool but could be a hidden dependency for developers in restricted environments or those unfamiliar with the build process.
  - *Impact:* Build failures or incorrect web asset deployment if `gzip` is not available or used incorrectly.
  - *Mitigation:* Add a task or note about ensuring `gzip` is installed and available in the development environment.

### Estimation Reality-Check

**Assessment:** Underestimated

The story is deceptively large. The combination of extensive client-side JavaScript logic (file handling, validation, XHR, progress, polling, countdown, banner display, NVS interaction), new C++ API endpoints (`ack-rollback`, `export-settings`), and detailed accessibility/UI requirements for both normal and error states suggests this would likely take more than a typical single-sprint story. The size of `dashboard.js` (87KB) before these additions indicates a high existing complexity, making further additions riskier and more time-consuming than implied. The UI polish and accessibility aspects in AC #11 (WCAG, touch targets) are also non-trivial to implement and verify.

### Technical Alignment

**Status:** Minor Conflicts

- **NVS Access Pattern:** The choice to directly use `Preferences` for `ota_rb_ack` instead of integrating it into `ConfigManager` for consistency (as implied by the "NVS-backed" requirement and the pattern of previous stories like fn-1.2) creates a minor architectural inconsistency. While the story justifies it by saying it's "overkill for a simple flag," this opens the door for ad-hoc NVS access, potentially fragmenting configuration management.
  - *Issue Type:* Pattern Conflict
  - *Description:* Direct NVS access for `ota_rb_ack` flag bypasses the established `ConfigManager` pattern for persistent settings, creating a deviation from the central singleton config approach.
  - *Architecture Reference:* architecture.md#D1: ConfigManager — Singleton with Category Struct Getters
  - *Recommendation:* Integrate `ota_rb_ack` into `ConfigManager` as a dedicated field within an appropriate struct (e.g., `ScheduleConfig` or a new `OtaConfig`) to maintain architectural consistency.
- **Firmware Version Source:** While `FW_VERSION` is a build flag, the story doesn't explicitly link this to `platformio.ini` where such flags are typically defined in this project. This is a minor gap in clarity about how this version string is propagated.
  - *Issue Type:* Context Gap
  - *Description:* The source and management of the `FW_VERSION` macro, particularly its link to `platformio.ini`, is not explicitly stated.
  - *Architecture Reference:* project_context.md: Tooling (Build from `firmware/` with `pio run`); architecture.md: Technology Stack (Platform: ESP32 with PlatformIO)
  - *Recommendation:* Add a note in the Dev Notes or tasks to explicitly state that `FW_VERSION` is a build flag configured in `platformio.ini`.

---

<!-- critical_issues_section -->

## 🚨 Critical Issues (Must Fix)

These are essential requirements, security concerns, or blocking issues that could cause implementation disasters.

### 1. NVS-backed persistence pattern conflict for rollback acknowledgment

**Impact:** Introduces an inconsistent pattern for NVS storage, potentially leading to fragmented configuration management and developer confusion for future features requiring NVS persistence. Violates the principle of a central `ConfigManager` for device settings.

**Source:** AC #9, Task 5, Dev Notes

**Problem:**
AC #9 explicitly requires the rollback acknowledgment banner to be "NVS-backed". However, the story proposes implementing this by directly accessing NVS `Preferences` (`prefs.begin("flightwall", false); prefs.putUChar("ota_rb_ack", 1);`) instead of integrating it into the `ConfigManager` as per the established architectural pattern (architecture.md#D1: ConfigManager — Singleton with Category Struct Getters). The Dev Notes even explicitly dismiss using `ConfigManager` as "overkill for a simple flag". This introduces an alternative, less managed NVS access method for a persistent setting, creating an architectural deviation.

**Recommended Fix:**
Integrate the `ota_rb_ack` flag into the existing `ConfigManager` structure. This would involve:
1. Adding `uint8_t ota_rb_ack;` to a relevant `ConfigManager` struct (e.g., `OtaConfig` if one is created, or `NetworkConfig` if broadly considered a network/firmware setting).
2. Implementing `loadFromNvs()`, `persistToNvs()`, `updateConfigValue()` handlers for this key in `ConfigManager.cpp`.
3. Modifying the `POST /api/ota/ack-rollback` handler and the OTA upload handler to use `ConfigManager::applyJson()` or `ConfigManager::setOtaRbAck()` (a new dedicated method) to update and persist this setting.
This maintains a single source of truth and consistent access pattern for all device configurations.

### 2. Rollback Banner Acknowledgment Reset Logic

**Impact:** The rollback banner might not reappear when a new valid OTA is installed, even if a subsequent (bad) firmware update causes another rollback. This defeats the purpose of the banner in alerting the user to critical device state changes.

**Source:** AC #10, Dev Notes ("Reset ota_rb_ack on New OTA Upload")

**Problem:**
AC #9 defines the behavior of the `POST /api/ota/ack-rollback` endpoint to dismiss the banner. The Dev Notes, under "Reset ota_rb_ack on New OTA Upload", correctly state that `ota_rb_ack` should be cleared to `0` when a *new OTA upload starts*. However, AC #9 or the tasks do not explicitly state that the `ack-rollback` operation only applies to the *currently detected* rollback and that a *new successful OTA installation* (not just the start of an upload) should implicitly reset this acknowledgment, ensuring that if a future firmware causes another rollback, the user is re-notified.

**Recommended Fix:**
Modify AC #9 to state that "the banner persists across page refreshes until dismissed (NVS-backed) **OR** until a successful OTA update is completed."
Correspondingly, update the `fn-1.3 (OTA Upload Endpoint)` story or add a task here to ensure that `ota_rb_ack` is explicitly cleared to `0` by the *successful completion* of an OTA update (i.e., after `Update.end(true)` returns true and before reboot), not just at the start of an upload. This ensures that a new, successfully installed firmware always "clears the slate" for rollback acknowledgments.

---

<!-- enhancements_section -->

## ⚡ Enhancement Opportunities (Should Add)

Additional guidance that would significantly help the developer avoid mistakes.

### 1. Clarify "1.5MB" vs "1.5 MiB" in Client-Side Validation

**Benefit:** Improves precision in technical communication, prevents potential off-by-a-few-bytes errors in client-side validation logic, and aligns with the exact byte count used in the firmware.

**Source:** AC #5, Dev Notes ("Client-Side File Validation")

**Current Gap:**
AC #5 and the client-side validation code specify "1.5MB" as the maximum file size (with the code using `1572864` bytes). This is technically "1.5 MiB" (mebibytes), not "1.5 MB" (megabytes). While often used interchangeably, in a technical context like file size limits for firmware, precision is important.

**Suggested Addition:**
Update AC #5 to explicitly state "1.5 MiB (1,572,864 bytes)" or "approximately 1.5 MB (1,572,864 bytes)" in the error message for clarity. This ensures the developer implementing the client-side validation correctly uses the exact byte count.

### 2. Explicit Polling Termination for Device Reachability

**Benefit:** Prevents potential infinite polling loops, clarifies success conditions for developers, and ensures a robust client-side recovery mechanism.

**Source:** AC #7

**Current Gap:**
AC #7 states "polling begins for device reachability (`GET /api/status`) **And** on successful poll, the new version string is displayed". It does not explicitly state that the polling should *stop* only when the `firmware_version` changes or after a specific timeout. This leaves an ambiguity in the polling logic's termination condition.

**Suggested Addition:**
Clarify AC #7: "polling begins for device reachability (`GET /api/status`) **until** a successful poll returns a `firmware_version` different from the version prior to the update, **OR** until the polling timeout (60 seconds) expires. On successful version change, the new version string is displayed **And** a success toast shows 'Updated to v[new version]'. The polling interval should be 3 seconds, for a maximum of 20 attempts."

### 3. Verification Method for CSS Requirements

**Benefit:** Provides clear guidance for verifying the aesthetic and accessibility requirements, ensuring higher quality and adherence to design standards.

**Source:** AC #11, Task 6

**Current Gap:**
AC #11 states WCAG contrast requirements, touch target sizes, and estimated line/gzipped size for the new CSS. However, the story does not specify *how* these should be verified (e.g., using browser dev tools for contrast, specific accessibility checkers, or manual inspection).

**Suggested Addition:**
Add a task under "Task 10: Build and verify" or in the Dev Notes' "Testing Strategy" section for CSS validation: "Manually verify WCAG 2.1 AA contrast ratios using browser developer tools. Manually check all interactive elements for ≥ 44x44px minimum touch targets in various mobile browser simulations. Use `du -sh firmware/data/*.gz` or `ls -l firmware/data/*.gz` to confirm gzipped file sizes against estimates."

### 4. Explicit `FW_VERSION` Macro Source

**Benefit:** Improves clarity for developers regarding build configuration and how versioning is managed in the project.

**Source:** AC #5, Task 4, Dev Notes

**Current Gap:**
AC #5 mentions `FW_VERSION` is used for the `firmware_version` in `/api/status`, and Task 4 references it as a compile-time build flag. However, the story doesn't explicitly state where this `FW_VERSION` macro is *defined* (e.g., in `platformio.ini` as a build flag). This missing detail can lead to confusion during build configuration or when troubleshooting versioning issues.

**Suggested Addition:**
Add a Dev Note explicitly stating: "`FW_VERSION` is a compile-time macro passed via `build_flags` in `platformio.ini` (e.g., `-D FW_VERSION=\"1.0.0\"`). The value should be updated in `platformio.ini` for new releases."

---

<!-- optimizations_section -->

## ✨ Optimizations (Nice to Have)

Performance hints, development tips, and additional context for complex scenarios.

### 1. Document XHR Requirement Justification in Code

**Value:** Improves code readability and maintainability by explaining the rationale behind using `XMLHttpRequest` instead of the more modern `fetch()`, helping future developers understand architectural choices.

**Source:** Dev Notes ("XMLHttpRequest vs Fetch for Upload")

**Suggestion:**
Add a comment to the `dashboard.js` code where `XMLHttpRequest` is used for firmware upload, explicitly stating: `// XHR required for upload progress events — fetch() does not support upload progress`. This directly brings the Dev Note's justification into the code.

---

<!-- llm_optimizations_section -->

## 🤖 LLM Optimization Improvements

Token efficiency and clarity improvements for better dev agent processing.

✅ Story content is well-optimized for LLM processing.

---

<!-- competition_results -->

## 🏆 Competition Results

### Quality Metrics

| Metric | Score |
|--------|-------|
| Requirements Coverage | 80% |
| Architecture Alignment | 70% |
| Previous Story Integration | 100% |
| LLM Optimization Score | 100% |
| **Overall Quality Score** | **70%** |

### Disaster Prevention Assessment

- **Reinvention Prevention:** ✅ Status: Good. The story clearly references existing patterns (logo upload, reboot timer, API structures) and server-side OTA implementation from fn-1.3, reducing the risk of redundant development.
- **Technical Specification:** 🟠 Status: Needs attention. Ambiguity in "1.5MB" vs "1.5 MiB" (AC #5) and the lack of explicit `FW_VERSION` source (AC #5, Task 4) are minor gaps that could cause implementation issues or confusion.
- **File Structure:** ✅ Status: Good. The story provides clear guidance on file modifications and asset placement, aligning with existing project structure.
- **Regression:** 🟠 Status: Needs attention. The polling termination for AC #7 is not fully specified, potentially leading to unintended behavior or incorrect status display, which could be a regression in user experience after an update. The NVS pattern conflict for `ota_rb_ack` could lead to future maintenance regressions.
- **Implementation:** 🔴 Status: Critical. The conflicting guidance on NVS-backed persistence for `ota_rb_ack` (AC #9) and the insufficient logic for resetting this flag after a *successful* OTA (not just start of upload) are critical implementation flaws that could compromise the integrity of the rollback warning system.

### Competition Outcome

🏆 **Validator identified 7 improvements** that enhance the story context.

---

**Report Generated:** 2026-04-12
**Validation Engine:** BMAD Method Quality Competition v1.0

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="description">Master synthesizes validator findings and applies changes to story file</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-1</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/validate-story-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/validate-story-synthesis/instructions.xml</var>
<var name="name">validate-story-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="session_id">df6c9845-7894-4d2f-9c8d-b1587460e42b</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="60733e24">embedded in prompt, file id: 60733e24</var>
<var name="story_id">fn-1.6</var>
<var name="story_key">fn-1-6-dashboard-firmware-card-and-ota-upload-ui</var>
<var name="story_num">6</var>
<var name="story_title">6-dashboard-firmware-card-and-ota-upload-ui</var>
<var name="template">False</var>
<var name="timestamp">20260412_2015</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="validator_count">2</var>
</variables>
<instructions><workflow>
  <critical>Communicate all responses in English and generate all documents in English</critical>

  <critical>You are the MASTER SYNTHESIS agent. Your role is to evaluate validator findings
    and produce a definitive synthesis with applied fixes.</critical>
  <critical>You have WRITE PERMISSION to modify the story file being validated.</critical>
  <critical>All context (project_context.md, story file, anonymized validations) is EMBEDDED below - do NOT attempt to read files.</critical>
  <critical>Apply changes to story file directly using atomic write pattern (temp file + rename).</critical>

  <step n="1" goal="Analyze validator findings">
    <action>Read all anonymized validator outputs (Validator A, B, C, D, etc.)</action>
    <action>For each issue raised:
      - Cross-reference with story content and project_context.md
      - Determine if issue is valid or false positive
      - Note validator consensus (if 3+ validators agree, high confidence issue)
    </action>
    <action>Issues with low validator agreement (1-2 validators) require extra scrutiny</action>
  </step>

  <step n="1.5" goal="Review Deep Verify technical findings" conditional="[Deep Verify Findings] section present">
    <critical>Deep Verify provides automated technical analysis that complements validator reviews.
      DV findings focus on: patterns, boundary cases, assumptions, temporal issues, security, and worst-case scenarios.</critical>

    <action>Review each DV finding:
      - CRITICAL findings: Must be addressed - these indicate serious technical issues
      - ERROR findings: Should be addressed unless clearly false positive
      - WARNING findings: Consider addressing, document if dismissed
    </action>

    <action>Cross-reference DV findings with validator findings:
      - If validators AND DV flag same issue: High confidence, prioritize fix
      - If only DV flags issue: Verify technically valid, may be edge case validators missed
      - If only validators flag issue: Normal processing per step 1
    </action>

    <action>For each DV finding, determine:
      - Is this a genuine issue in the story specification?
      - Does the story need to address this edge case/scenario?
      - Is this already covered but DV missed it? (false positive)
    </action>

    <action>DV findings with patterns (CC-*, SEC-*, DB-*, DT-*, GEN-*) reference known antipatterns.
      Treat pattern-matched findings as higher confidence.</action>
  </step>

  <step n="2" goal="Verify and prioritize issues">
    <action>For verified issues, assign severity:
      - Critical: Blocks implementation or causes major problems
      - High: Significant gaps or ambiguities that need attention
      - Medium: Improvements that would help quality
      - Low: Nice-to-have suggestions
    </action>
    <action>Document false positives with clear reasoning for dismissal:
      - Why the validator was wrong
      - What evidence contradicts the finding
      - Reference specific story content or project_context.md
    </action>
  </step>

  <step n="3" goal="Apply changes to story file">
    <action>For each verified issue (starting with Critical, then High), apply fix directly to story file</action>
    <action>Changes should be natural improvements:
      - DO NOT add review metadata or synthesis comments to story
      - DO NOT reference the synthesis or validation process
      - Preserve story structure, formatting, and style
      - Make changes look like they were always there
    </action>
    <action>For each change, log in synthesis output:
      - File path modified
      - Section/line reference (e.g., "AC4", "Task 2.3")
      - Brief description of change
      - Before snippet (2-3 lines context)
      - After snippet (2-3 lines context)
    </action>
    <action>Use atomic write pattern for story modifications to prevent corruption</action>
  </step>

  <step n="4" goal="Generate synthesis report">
    <critical>Your synthesis report MUST be wrapped in HTML comment markers for extraction:</critical>
    <action>Produce structured output in this exact format (including the markers):</action>
    <output-format>
&lt;!-- VALIDATION_SYNTHESIS_START --&gt;
## Synthesis Summary
[Brief overview: X issues verified, Y false positives dismissed, Z changes applied to story file]

## Validations Quality
[For each validator: name, score, comments]
[Summary of validation quality - 1-10 scale]

## Issues Verified (by severity)

### Critical
[Issues that block implementation - list with evidence and fixes applied]
[Format: "- **Issue**: Description | **Source**: Validator(s) | **Fix**: What was changed"]

### High
[Significant gaps requiring attention]

### Medium
[Quality improvements]

### Low
[Nice-to-have suggestions - may be deferred]

## Issues Dismissed
[False positives with reasoning for each dismissal]
[Format: "- **Claimed Issue**: Description | **Raised by**: Validator(s) | **Dismissal Reason**: Why this is incorrect"]

## Deep Verify Integration
[If DV findings were present, document how they were handled]

### DV Findings Addressed
[List DV findings that resulted in story changes]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Action**: {What was changed}"]

### DV Findings Dismissed
[List DV findings determined to be false positives or not applicable]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Reason**: {Why dismissed}"]

### DV-Validator Overlap
[Note any findings flagged by both DV and validators - these are high confidence]
[If no DV findings: "Deep Verify did not produce findings for this story."]

## Changes Applied
[Complete list of modifications made to story file]
[Format for each change:
  **Location**: [File path] - [Section/line]
  **Change**: [Brief description]
  **Before**:
  ```
  [2-3 lines of original content]
  ```
  **After**:
  ```
  [2-3 lines of updated content]
  ```
]
&lt;!-- VALIDATION_SYNTHESIS_END --&gt;
    </output-format>

  </step>

  <step n="5" goal="Final verification">
    <action>Verify all Critical and High issues have been addressed</action>
    <action>Confirm story file changes are coherent and preserve structure</action>
    <action>Ensure synthesis report is complete with all sections populated</action>
  </step>
</workflow></instructions>
<output-template></output-template>
</compiled-workflow>