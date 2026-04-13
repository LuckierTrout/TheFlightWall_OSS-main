<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 7 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T013156Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-1.7

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
<file id="ff10d1c0" path="_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md" label="DOCUMENTATION"><![CDATA[



# Story fn-1.7: Settings Import in Setup Wizard

Status: Ready for Review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a **user**,
I want **to import a previously exported settings file in the setup wizard**,
So that **I can quickly reconfigure my device after a partition migration without retyping API keys and coordinates**.

## Acceptance Criteria

1. **Given** the setup wizard loads **When** an import option is available **Then** the user can select a `.json` file via a file upload zone (reusing `.logo-upload-zone` CSS pattern) **And** the import zone appears at the top of Step 1 before the WiFi form fields **And** the import zone has `role="button"`, `tabindex="0"`, and descriptive `aria-label`

2. **Given** a valid `flightwall-settings.json` file is selected **When** the browser reads the file via FileReader API **Then** the JSON is parsed client-side **And** recognized config keys pre-fill their corresponding wizard form fields across all steps (WiFi, API keys, location, hardware) **And** the user can navigate forward through pre-filled steps, reviewing each before confirming **And** a success toast shows the count of imported keys (e.g., "Imported 12 settings")

3. **Given** the imported JSON contains unrecognized keys (e.g., `flightwall_settings_version`, `exported_at`, or future keys) **When** processing the import **Then** unrecognized keys are silently ignored without error **And** all recognized keys are still pre-filled correctly

4. **Given** the imported file is not valid JSON **When** parsing fails **Then** an error toast shows "Could not read settings file — invalid format" **And** the wizard continues without pre-fill (manual entry still available) **And** the import zone resets to its initial state

5. **Given** the imported file is valid JSON but contains no recognized config keys **When** processing completes **Then** a warning toast shows "No recognized settings found in file" **And** all form fields remain at their defaults

6. **Given** settings are imported and the user completes the wizard **When** "Save & Connect" is tapped **Then** settings are applied via the normal `POST /api/settings` path (no new server-side import endpoint) **And** any imported keys that are NOT part of the wizard's 12 core fields (e.g., brightness, timing, calibration, schedule settings) are included in the POST payload alongside the wizard fields

7. **Given** the import zone is visible **When** the user opts not to import **Then** the wizard functions identically to its current behavior — WiFi scan runs, steps proceed normally, no import-related UI interferes

8. **Given** a settings file is imported **When** the user navigates to a pre-filled step **Then** the pre-filled fields are visually indistinguishable from user-typed values (no special styling needed — they're just pre-populated form values)

9. **Given** updated web assets (wizard.html, wizard.js, style.css) **When** gzipped and placed in `firmware/data/` **Then** the gzipped copies replace existing ones for LittleFS upload

## Tasks / Subtasks

- [x] **Task 1: Add import zone HTML to wizard.html** (AC: #1, #7)
  - [x] Add a settings import section at the top of Step 1 (`#step-1`), BEFORE the `#scan-status` div
  - [x] HTML structure:
    ```html
    <div class="settings-import-zone" id="settings-import-zone" role="button" tabindex="0" aria-label="Select settings backup file to import">
      <p class="upload-prompt">Have a backup? <strong>Import settings</strong></p>
      <p class="upload-hint">Select a flightwall-settings.json file</p>
      <input type="file" id="import-file-input" accept=".json" hidden>
    </div>
    ```
  - [x] The import zone should be compact — 2 lines of text max, dashed border, same visual pattern as `.logo-upload-zone` but more minimal
  - [x] Add a hidden success indicator `<p id="import-status" class="import-status" style="display:none"></p>` below the import zone to show "12 settings imported" after successful import

- [x] **Task 2: Add import zone CSS to style.css** (AC: #1)
  - [x] `.settings-import-zone` — Reuse pattern from `.logo-upload-zone`: dashed border `2px dashed var(--border)`, padding `12px`, border-radius `var(--radius)`, cursor pointer, text-align center, transition for border-color
  - [x] `.settings-import-zone.drag-over` — Solid border `var(--primary)`, background `rgba(88,166,255,0.08)`
  - [x] `.import-status` — font-size `0.8125rem`, color `var(--success)`, text-align center, margin-top `8px`
  - [x] Target: ~15 lines of CSS, minimal footprint
  - [x] All interactive elements meet 44x44px minimum touch targets
  - [x] Works at 320px minimum viewport width

- [x] **Task 3: Implement settings import logic in wizard.js** (AC: #2, #3, #4, #5, #6, #7, #8)
  - [x] Add DOM references for import elements: `settings-import-zone`, `import-file-input`, `import-status`
  - [x] Add `importedExtras` object (initially empty `{}`) to hold non-wizard keys from import
  - [x] Define `WIZARD_KEYS` constant: the 12 keys that map to wizard form fields:
    ```javascript
    var WIZARD_KEYS = [
      'wifi_ssid', 'wifi_password',
      'os_client_id', 'os_client_sec', 'aeroapi_key',
      'center_lat', 'center_lon', 'radius_km',
      'tiles_x', 'tiles_y', 'tile_pixels', 'display_pin'
    ];
    ```
  - [x] Define `KNOWN_EXTRA_KEYS` constant: non-wizard config keys that should be preserved:
    ```javascript
    var KNOWN_EXTRA_KEYS = [
      'brightness', 'text_color_r', 'text_color_g', 'text_color_b',
      'origin_corner', 'scan_dir', 'zigzag',
      'zone_logo_pct', 'zone_split_pct', 'zone_layout',
      'fetch_interval', 'display_cycle',
      'timezone', 'sched_enabled', 'sched_dim_start', 'sched_dim_end', 'sched_dim_brt'
    ];
    ```
  - [x] **Click/keyboard handler**: click on `.settings-import-zone` or Enter/Space key triggers hidden `#import-file-input` click
  - [x] **File input change handler**: when a file is selected, read via `FileReader.readAsText()`, then call `processImportedSettings(text)`
  - [x] **`processImportedSettings(text)` function**:
    1. Try `JSON.parse(text)` — on failure: `FW.showToast('Could not read settings file — invalid format', 'error')`, return
    2. Check parsed object is a plain object (not array/null) — on failure: same error toast, return
    3. Iterate keys — for each key in `WIZARD_KEYS`, if present in parsed object, assign `state[key] = String(parsed[key])`
    4. Iterate keys — for each key in `KNOWN_EXTRA_KEYS`, if present in parsed object, assign `importedExtras[key] = parsed[key]`
    5. Count total recognized keys (wizard + extras)
    6. If count === 0: `FW.showToast('No recognized settings found in file', 'warning')`, return
    7. If count > 0: `FW.showToast('Imported ' + count + ' settings', 'success')`
    8. Show import status indicator with count
    9. Rehydrate current step's form fields from updated state (call `showStep(currentStep)`)
  - [x] **Modify `saveAndConnect()` payload**: merge `importedExtras` into the payload object before POSTing:
    ```javascript
    // After building the existing 12-key payload:
    var key;
    for (key in importedExtras) {
      if (importedExtras.hasOwnProperty(key)) {
        payload[key] = importedExtras[key];
      }
    }
    ```
  - [x] **Guard against re-import**: if the user selects a new file after already importing, reset `importedExtras = {}` and re-process

- [x] **Task 4: Add drag-and-drop support for import zone** (AC: #1)
  - [x] `dragenter`/`dragover` on zone — add `.drag-over` class, prevent default
  - [x] `dragleave` — remove `.drag-over` class (use `contains(e.relatedTarget)` check to avoid child-element flicker)
  - [x] `drop` — remove `.drag-over` class, extract `e.dataTransfer.files[0]`, read via FileReader
  - [x] Pattern matches the existing logo upload and OTA upload drag-and-drop implementations

- [x] **Task 5: Gzip updated web assets** (AC: #9)
  - [x] From `firmware/` directory, regenerate gzipped assets:
    ```
    gzip -9 -c data-src/wizard.html > data/wizard.html.gz
    gzip -9 -c data-src/wizard.js > data/wizard.js.gz
    gzip -9 -c data-src/style.css > data/style.css.gz
    ```
  - [x] Verify gzipped files replaced in `firmware/data/`

- [x] **Task 6: Build and verify** (AC: #1-#9)
  - [x] Run `~/.platformio/penv/bin/pio run` from `firmware/` — verify clean build
  - [x] Verify binary size remains under 1.5MB limit
  - [x] Measure total gzipped web asset size (should remain well under 50KB budget)

## Dev Notes

### Critical Architecture Constraints

**Web Asset Build Pipeline (MANUAL)**
Per project memory: No automated gzip build script. After editing any file in `firmware/data-src/`, manually run:
```bash
cd firmware
gzip -9 -c data-src/wizard.html > data/wizard.html.gz
gzip -9 -c data-src/wizard.js > data/wizard.js.gz
gzip -9 -c data-src/style.css > data/style.css.gz
```

**This is a CLIENT-SIDE ONLY change — NO new server-side endpoints or firmware modifications are needed.** The import reads a JSON file in the browser, pre-fills the wizard state object, and the existing `POST /api/settings` path handles persistence. The only firmware files that change are the gzipped web assets.

### Settings Export Format (from fn-1.6)

The `GET /api/settings/export` endpoint (implemented in fn-1.6, Task 4) returns a JSON file with this structure:
```json
{
  "flightwall_settings_version": 1,
  "exported_at": "2026-04-12T15:30:45",
  "brightness": 5,
  "text_color_r": 255, "text_color_g": 255, "text_color_b": 255,
  "center_lat": 37.7749, "center_lon": -122.4194, "radius_km": 10.0,
  "tiles_x": 10, "tiles_y": 2, "tile_pixels": 16, "display_pin": 25,
  "origin_corner": 0, "scan_dir": 0, "zigzag": 0,
  "zone_logo_pct": 0, "zone_split_pct": 0, "zone_layout": 0,
  "fetch_interval": 600, "display_cycle": 3,
  "wifi_ssid": "MyNetwork", "wifi_password": "secret",
  "os_client_id": "abc123", "os_client_sec": "def456",
  "aeroapi_key": "ghi789",
  "timezone": "PST8PDT,M3.2.0,M11.1.0",
  "sched_enabled": 0,
  "sched_dim_start": 1380, "sched_dim_end": 420, "sched_dim_brt": 10
}
```

**Key mapping:**
- 12 keys map directly to wizard `state` fields (wifi, API, location, hardware)
- 18 keys are "extras" that the wizard doesn't display but should preserve in the POST payload
- 2 keys are metadata (`flightwall_settings_version`, `exported_at`) — silently ignored

### Wizard State Architecture

The wizard maintains an in-memory `state` object with 12 keys (wizard.js lines 9-22). Each step rehydrates form fields from `state` when entered (lines 308-339) and saves fields back to `state` when leaving (lines 342-359).

**Import strategy:** Pre-fill `state` with wizard-relevant keys from the imported JSON. Store non-wizard keys in a separate `importedExtras` object. When `saveAndConnect()` builds the POST payload (lines 463-476), merge `importedExtras` into it.

**Hydration guard (wizard.js lines 62-77):** The existing `hydrateDefaults()` function checks `if (!state.center_lat && ...)` before populating from `/api/settings`. After import, these fields will already be non-empty, so `hydrateDefaults()` will correctly skip them. **No modification to `hydrateDefaults()` is needed.**

### Import Zone Placement

The import zone goes at the TOP of Step 1 (`#step-1`), before `#scan-status`. This placement:
1. Is the first thing the user sees on wizard load
2. Allows import before any WiFi scanning happens
3. Doesn't interfere with the scan flow (scan still runs in the background)
4. Pre-fills ALL steps, not just the current one

After a successful import, the import zone can optionally collapse or show a success state, but it should NOT be removed — the user might want to re-import a different file.

### Type Coercion for State Fields

The wizard `state` object stores all values as **strings** (see lines 9-22). The import JSON has numeric values for location/hardware keys. When importing:
- Wizard-state keys must be converted to strings: `state[key] = String(parsed[key])`
- Extra keys should preserve their original types (numbers stay numbers) since they go directly into the POST payload which already handles the numeric conversion

### Existing Patterns to Reuse

**File upload zone CSS:** Reuse `.logo-upload-zone` pattern from style.css (lines ~310-320) — dashed border, drag-over state, centered text.

**Drag-and-drop:** Reuse the pattern from the OTA upload zone in dashboard.js — `dragenter`/`dragover`/`dragleave`/`drop` with `contains(e.relatedTarget)` guard for dragleave.

**Toast notifications:** Use `FW.showToast(message, severity)` from common.js — already available in wizard context.

**FileReader API:** Already used in dashboard.js for OTA magic byte validation. Here we use `readAsText()` instead of `readAsArrayBuffer()`.

### POST /api/settings Behavior with Extra Keys

The `ConfigManager::applyJson()` method (called by `POST /api/settings`) processes each key individually and returns which keys were applied. Unknown keys cause the endpoint to return a 400 error (WebPortal.cpp line 207: "Unknown or invalid settings key"). 

**Important:** The validation checks `if (result.applied.size() != settings.size())` — if ANY key is unrecognized, the entire request fails. This means:
- `importedExtras` must ONLY contain keys that ConfigManager recognizes
- The `KNOWN_EXTRA_KEYS` list must exactly match ConfigManager's accepted keys
- Metadata keys like `flightwall_settings_version` and `exported_at` must NOT be included in `importedExtras`

### What NOT to Do

- **Do NOT create a new server-side endpoint** — import is purely client-side
- **Do NOT modify `hydrateDefaults()`** — the existing guard logic handles pre-filled state correctly
- **Do NOT modify wizard step count or flow** — the import is additive UI at the top of Step 1
- **Do NOT add the import zone to dashboard.html** — import is wizard-only (the dashboard has direct settings editing)
- **Do NOT include `flightwall_settings_version` or `exported_at` in `importedExtras`** — these are metadata keys, not config keys, and will cause the POST to fail with "Unknown or invalid settings key"

### Project Structure Notes

**Files to modify:**
1. `firmware/data-src/wizard.html` — Add import zone HTML to Step 1
2. `firmware/data-src/wizard.js` — Add import logic, modify `saveAndConnect()` payload
3. `firmware/data-src/style.css` — Add import zone styles (~15 lines)
4. `firmware/data/wizard.html.gz` — Regenerated
5. `firmware/data/wizard.js.gz` — Regenerated
6. `firmware/data/style.css.gz` — Regenerated

**Files NOT to modify:**
- `firmware/adapters/WebPortal.cpp` — No new endpoints needed
- `firmware/adapters/WebPortal.h` — No changes
- `firmware/core/ConfigManager.h/.cpp` — No changes
- `firmware/data-src/dashboard.html` — Import is wizard-only
- `firmware/data-src/dashboard.js` — Import is wizard-only
- `firmware/data-src/common.js` — `FW.showToast` already available
- `firmware/src/main.cpp` — No changes

### References

- [Source: epic-fn-1.md#Story fn-1.7 — All acceptance criteria, story definition]
- [Source: architecture.md#D4 — POST /api/settings endpoint, JSON envelope pattern]
- [Source: architecture.md#F7 — GET /api/settings/export format definition]
- [Source: prd.md#FR1 — User can complete initial configuration from mobile browser]
- [Source: prd.md#FR10 — Persist all configuration to NVS]
- [Source: wizard.js lines 9-22 — Wizard state object with 12 keys]
- [Source: wizard.js lines 62-77 — hydrateDefaults() guard logic]
- [Source: wizard.js lines 342-359 — saveCurrentStepState() state persistence]
- [Source: wizard.js lines 457-501 — saveAndConnect() POST payload construction]
- [Source: wizard.js lines 290-339 — showStep() rehydration from state]
- [Source: wizard.html lines 18-33 — Step 1 HTML structure, insertion point for import zone]
- [Source: WebPortal.cpp lines 653-680 — GET /api/settings/export endpoint implementation]
- [Source: WebPortal.cpp lines 200-208 — POST /api/settings validation (applied.size != settings.size = 400)]
- [Source: ConfigManager.cpp lines 469-521 — dumpSettingsJson() exports all 30 config keys]
- [Source: style.css lines ~310-320 — .logo-upload-zone pattern for reuse]
- [Source: dashboard.js OTA upload zone — drag-and-drop pattern with relatedTarget guard]
- [Source: fn-1-6 story — Settings export endpoint implementation details, CSS patterns]

### Dependencies

**This story depends on:**
- fn-1.5 (Settings Export Endpoint) — COMPLETE (defines the JSON export format this story imports)
- fn-1.6 (Dashboard Firmware Card & OTA Upload UI) — COMPLETE (implemented `GET /api/settings/export`, established file upload UI patterns)

**No stories depend on this story** — fn-1.7 is the final story in Epic fn-1.

### Previous Story Intelligence

**From fn-1.1:** Binary size 1.15MB (76.8%). Partition size 0x180000 (1,572,864 bytes). `FW_VERSION` build flag established.

**From fn-1.2:** `ConfigManager::dumpSettingsJson()` method exports all 30 config keys as flat JSON. NVS namespace is `"flightwall"`. ConfigManager accepts 30 known keys via `applyJson()`.

**From fn-1.6:** `GET /api/settings/export` implemented with `Content-Disposition: attachment` header. Exports `flightwall_settings_version: 1` and `exported_at` metadata alongside all config keys. OTA upload zone established drag-and-drop patterns with `contains(e.relatedTarget)` dragleave guard. Build verified at 1.17MB (78.2%), 38KB total gzipped web assets.

**Antipatterns from previous stories to avoid:**
- Don't include metadata keys (`flightwall_settings_version`, `exported_at`) in the POST payload — they'll cause a 400 error from ConfigManager
- Ensure all error paths reset UI to initial state (learned from fn-1.3/fn-1.6 cleanup patterns)
- Use `contains(e.relatedTarget)` for dragleave events (learned from fn-1.6 code review)
- Don't modify files outside the stated scope — this is a client-side-only change

### Testing Strategy

**Build verification:**
```bash
cd firmware && ~/.platformio/penv/bin/pio run
# Expect: SUCCESS, binary < 1.5MB
```

**Gzip verification:**
```bash
cd firmware
gzip -9 -c data-src/wizard.html > data/wizard.html.gz
gzip -9 -c data-src/wizard.js > data/wizard.js.gz
gzip -9 -c data-src/style.css > data/style.css.gz
ls -la data/wizard.html.gz data/wizard.js.gz data/style.css.gz
# Verify files exist and are reasonable size
```

**Manual test cases:**

1. **Wizard loads — import zone visible:**
   - Open wizard (connect to FlightWall AP, navigate to 192.168.4.1)
   - Expect: Import zone visible at top of Step 1 before WiFi scan
   - Expect: WiFi scan still runs normally below the import zone

2. **Import valid settings file:**
   - Export settings from dashboard (`Download Settings` in System card)
   - Reset device to AP mode
   - Open wizard, tap import zone, select the exported .json file
   - Expect: Success toast "Imported N settings"
   - Navigate through all 5 steps
   - Expect: WiFi SSID/password, API keys, location, hardware all pre-filled
   - Tap "Save & Connect"
   - Expect: Settings applied and device reboots

3. **Import file with extra keys only (no wizard keys):**
   - Create a JSON file with only `{"brightness": 50, "fetch_interval": 120}`
   - Import it
   - Expect: Success toast "Imported 2 settings"
   - Wizard fields remain at defaults (extras don't appear in wizard steps)
   - Complete wizard with manual entry
   - Expect: POST payload includes brightness=50 and fetch_interval=120 alongside wizard values

4. **Import invalid JSON:**
   - Select a non-JSON file (e.g., a .bin logo file, or a text file with invalid JSON)
   - Expect: Error toast "Could not read settings file — invalid format"
   - Wizard continues normally

5. **Import JSON with no recognized keys:**
   - Create `{"foo": "bar", "baz": 123}` file
   - Import it
   - Expect: Warning toast "No recognized settings found in file"

6. **Import file with metadata keys:**
   - Create `{"flightwall_settings_version": 1, "exported_at": "2026-01-01", "brightness": 50}`
   - Import it
   - Expect: Success toast "Imported 1 settings" (only brightness counted)
   - Expect: `flightwall_settings_version` and `exported_at` NOT in POST payload

7. **Skip import — wizard works normally:**
   - Don't tap the import zone at all
   - Navigate through all steps manually
   - Expect: Wizard behaves identically to pre-import behavior

8. **Import zone — keyboard access:**
   - Tab to import zone, press Enter or Space
   - Expect: File picker opens

9. **Import zone — drag and drop:**
   - Drag a .json file over the import zone
   - Expect: Border becomes solid, background lightens
   - Drop file
   - Expect: File is processed

10. **Responsive — 320px viewport:**
    - Resize browser to 320px width
    - Expect: Import zone visible and functional, doesn't overflow

### Risk Mitigation

1. **POST /api/settings key validation:** The server rejects the entire request if any key is unknown. The `KNOWN_EXTRA_KEYS` list must be kept in sync with ConfigManager's accepted keys. If a future firmware version adds new config keys but the user imports an old settings file, that's fine — missing keys just aren't sent. If a user imports a settings file from a NEWER firmware version with unknown keys, the extras filtering via `KNOWN_EXTRA_KEYS` ensures only recognized keys are sent.

2. **Wizard.js file size:** Currently ~14KB unminified (~3.5KB gzipped). Adding ~80-100 lines of import logic is proportional (~7% increase). Well within the gzip budget.

3. **WiFi password in settings export:** The exported JSON includes `wifi_password` in plaintext. This is by design (per architecture.md Security Model: "No authentication on web config UI, single-user trusted network"). The import simply reads it back. No additional security concern beyond what the export already accepts.

4. **Type safety:** The wizard state stores strings; the export has numbers for location/hardware. The `String()` conversion in import handles this safely. The POST payload in `saveAndConnect()` already converts state strings to `Number()` (wizard.js lines 469-476), so the round-trip is: export(number) → import(String) → state(string) → payload(Number) → POST.

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6 (Implementation)

### Debug Log References

N/A

### Completion Notes List

**2026-04-12: Implementation complete — all 6 tasks, 9 ACs satisfied**
- Task 1: Added import zone HTML to wizard.html Step 1, before scan-status div. Includes `role="button"`, `tabindex="0"`, `aria-label`, hidden file input, and status indicator.
- Task 2: Added 10 lines of CSS for `.settings-import-zone`, `.drag-over` state, focus-visible outline, and `.import-status`. Reuses `.logo-upload-zone` dashed-border pattern. 44px min-height touch target.
- Task 3: Added `WIZARD_KEYS` (12), `KNOWN_EXTRA_KEYS` (17), `importedExtras` object. `processImportedSettings()` handles JSON parse errors (AC #4), no-recognized-keys (AC #5), metadata key filtering (AC #3), wizard key pre-fill with `String()` coercion (AC #2, #8). Modified `saveAndConnect()` to merge extras into POST payload (AC #6). Re-import resets extras (guard against stale state).
- Task 4: Drag-and-drop with `dragenter`/`dragover`/`dragleave`/`drop`. Uses `contains(e.relatedTarget)` guard per fn-1.6 antipattern lesson.
- Task 5: Gzipped all 3 modified assets. wizard.html.gz=1.6KB, wizard.js.gz=6.2KB, style.css.gz=4.8KB. Total web assets 40.1KB (under 50KB budget).
- Task 6: Build SUCCESS — 1.22MB (77.7% of 1.5MB partition). No regressions.
- No server-side changes required — purely client-side import via FileReader + existing POST /api/settings.

**2026-04-12: Ultimate context engine analysis completed**
- Comprehensive analysis of epic-fn-1.md extracted all 6 acceptance criteria with BDD format, expanded to 9 ACs with additional edge cases
- Full wizard.html structure analyzed (103 lines, 5 steps) — import zone insertion point identified at top of Step 1 before scan-status
- Full wizard.js analyzed (558 lines) — state management, hydration, validation, saveAndConnect payload all documented
- Full style.css analyzed — `.logo-upload-zone` CSS pattern identified for reuse
- WebPortal.cpp POST /api/settings validation analyzed — critical finding: unknown keys cause entire request to fail (line 207)
- ConfigManager.cpp dumpSettingsJson() analyzed — all 30 exportable config keys mapped to wizard-keys (12) and extra-keys (18)
- Settings export format fully documented from fn-1.6 implementation
- 6 tasks created with explicit code patterns, insertion points, and key-mapping logic
- POST payload key-filtering design prevents metadata keys from causing server-side 400 errors
- hydrateDefaults() guard logic analyzed — no modification needed (import pre-fills state, guard skips hydration correctly)
- All dependency stories (fn-1.5, fn-1.6) verified complete with relevant implementation details extracted
- Antipatterns from fn-1.6 code reviews incorporated (dragleave relatedTarget, UI reset on error)

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Implementation complete — all 6 tasks done, build verified at 1.22MB (77.7%) | Claude Opus 4.6 |

### File List

| File | Action |
|------|--------|
| `firmware/data-src/wizard.html` | Modified — added import zone HTML to Step 1 |
| `firmware/data-src/wizard.js` | Modified — added import logic, WIZARD_KEYS, KNOWN_EXTRA_KEYS, drag-and-drop, payload merge |
| `firmware/data-src/style.css` | Modified — added .settings-import-zone, .drag-over, .import-status CSS |
| `firmware/data/wizard.html.gz` | Regenerated |
| `firmware/data/wizard.js.gz` | Regenerated |
| `firmware/data/style.css.gz` | Regenerated |


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

## Story fn-1-6 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | No cancel/re-select path once a valid file is chosen | Added `<button id="btn-cancel-ota">Cancel</button>` to `.ota-file-info`, wired to `resetOtaUploadState()` in JS. |
| critical | Polling race condition — timeout check fires immediately after async `FW.get()` starts, allowing both "Device unreachable" message AND success toast to show simultaneously on the final attempt | Restructured `startRebootPolling()` to check `attempts >= maxAttempts` **before** issuing the request; added `done` boolean guard so late-arriving responses are discarded after timeout. |
| high | `resetOtaUploadState()` does not reset `otaRebootText.style.color` or `otaRebootText.textContent` — amber timeout color and stale text persist into subsequent upload attempts | Added `otaRebootText.textContent = ''` and `otaRebootText.style.color = ''` to `resetOtaUploadState()`. |
| high | `.banner-warning .dismiss-btn` has no `:focus` style — WCAG 2.1 AA violation (visible focus indicator required) | Added `.banner-warning .dismiss-btn:focus-visible { outline: 2px solid var(--primary); outline-offset: 2px }` rule. |
| high | `prefs.putUChar("ota_rb_ack", 1)` return value ignored in ack-rollback handler — silent failure if NVS write fails | Added `prefs.begin()` return check and `written == 0` check; return 500 error responses with inline JSON (not `_sendJsonError` — lambda is `[]`, no `this` capture). |
| medium | `dragleave` removes `.drag-over` class when cursor moves over child elements — causes visual flicker mid-drag | Added `e.relatedTarget` guard: `if (!otaUploadZone.contains(e.relatedTarget))`. |
| low | Zero-byte file produces misleading error "Not a valid ESP32 firmware image" instead of "File is empty" | Added explicit `file.size === 0` check with message "File is empty — select a valid firmware .bin file" before the magic byte check. |

## Story fn-1-6 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | WCAG AA contrast failure — white text (#fff) over `--primary` (#58a6ff) on `.ota-progress-text` yields 2.40:1 contrast ratio, below the 4.5:1 AA minimum. Text overlays both filled (#58a6ff) and unfilled (#30363d) progress bar regions, creating variable backgrounds | Added four-direction `text-shadow: rgba(0,0,0,0.65)` to create a dark halo that ensures readability on both light-blue and dark-grey backgrounds |
| medium | `btnUploadFirmware` not disabled at start of `uploadFirmware()` — rapid double-tap could enqueue two concurrent uploads before the `otaFileInfo` section is hidden. Server guards with `g_otaInProgress` (returning 409), but the second request still generates an error toast, confusing UX | `btnUploadFirmware.disabled = true` at function entry, re-enabled in both `xhr.onload` and `xhr.onerror` |
| low | Polling `FW.get('/api/status')` lacks cache-busting — browsers with aggressive cache policies could serve a stale 200 from the pre-reboot device, falsely triggering "Updated to v..." before the new firmware is running | Changed to `FW.get('/api/status?_=' + Date.now())` to guarantee a fresh request |
| low | `#d29922` amber color hardcoded in JS for the "Device unreachable" timeout message, bypassing the design system. `--warning` CSS variable was also absent, making `var(--warning)` fallback impossible | Added `--warning: #d29922` to `:root` CSS variables; updated JS to `getComputedStyle(document.documentElement).getPropertyValue('--warning').trim() \|\| '#d29922'` |
| dismissed | Server-side OTA file size check missing (DoS vulnerability) | FALSE POSITIVE: Explicitly dismissed as FALSE POSITIVE in fn-1.3 antipatterns: "Update.begin(partition->size) tells the library the maximum expected size. Update.write() will fail when the partition is full." Client-side 1.5MB check provides UX feedback; server-side is handled by the Update library. |
| dismissed | `_sendJsonError` should replace inline JSON in ack-rollback handler | FALSE POSITIVE: `_sendJsonError` is a `WebPortal` class member function. The `/api/ota/ack-rollback` handler is a `[]` lambda (no `this` capture) — calling `_sendJsonError` is syntactically impossible without restructuring the handler. Inline JSON strings are the correct pattern for captureless lambdas. |
| dismissed | `OTA_MAX_SIZE` should be driven by API (`/api/status.ota_max_size`) | FALSE POSITIVE: Over-engineering. The 1.5MB partition size is a compile-time constant (custom_partitions.csv) that cannot change at runtime. Adding an API field and HTTP round-trip for this is disproportionate; the constant is clearly documented with its hex value. |
| dismissed | Drag-over border uses `--primary` instead of `--accent` (AC #3) | FALSE POSITIVE: `--accent` does not exist in the project's CSS design system (`:root` defines `--primary`, `--primary-hover`, `--error`, `--success`, `--warning` — no `--accent`). The Dev Notes explicitly lists `--primary` as the correct token for "drag-over border." The AC spec contains a stale/incorrect variable name. |
| dismissed | Race condition in `startRebootPolling` — timeout and success could fire simultaneously | FALSE POSITIVE: Already fixed in Pass 1 synthesis. Lines 2818-2831 show: `done` flag prevents double-finish; `attempts >= maxAttempts` check fires BEFORE the next request is issued; `if (done) return` discards late-arriving responses. |
| dismissed | `FileReader.readAsArrayBuffer(file.slice(0, 4))` reads 4 bytes but only `bytes[0]` is used | FALSE POSITIVE: Negligible inefficiency. Reading 4 bytes on a modern browser is identical cost to reading 1 byte (minimum slice size). Functionally correct; no impact on correctness or performance. |
| dismissed | `FW.get()` reboot polling is "falsely optimistic" — could return cached pre-reboot 200 | FALSE POSITIVE: The server calls `request->send(200, ...)` before scheduling the 500ms reboot timer (WebPortal.cpp:494). The full 50-byte JSON response will be in the TCP send buffer before the timer fires. `xhr.onerror` cannot fire on a response that was successfully sent and received. Only genuine network failures during upload reach `onerror`. |
| dismissed | Story file omits WebPortal.h and main.cpp from modified file list | FALSE POSITIVE: Documentation concern, not a code defect. The synthesis mission is to verify and fix SOURCE CODE — story record auditing is out of scope. No code correction needed. |
| dismissed | Settings export silently backs up stale persisted config when form has unsaved edits | FALSE POSITIVE: AC #10 specifies "triggers `GET /api/settings/export`" — exporting persisted server config is the intended behavior. The helper text instructs users to download settings as a backup "before migration or reflash" — not mid-edit. A warning when `applyBar` is visible would be a UX enhancement, not a bug fix. Out of scope. |
| dismissed | No automated test coverage for new OTA/export endpoints | FALSE POSITIVE: Valid concern but outside synthesis scope. ESP32 integration tests require hardware-in-the-loop or mock server infrastructure not present in this repo. Would need a dedicated testing story. |
| dismissed | CSRF protection missing on POST /api/ota/upload | FALSE POSITIVE: Explicitly dismissed as FALSE POSITIVE in fn-1.3 antipatterns: "No endpoint in this device has authentication — `/api/reboot`, `/api/reset`, `/api/settings` are all unauthenticated. This is a pre-existing architectural design gap (LAN-only device), not introduced by this story." |
| dismissed | No upload cancellation mechanism | FALSE POSITIVE: ALREADY IMPLEMENTED in Pass 1 synthesis. `btn-cancel-ota` exists in dashboard.html:248 and is wired to `resetOtaUploadState()` in dashboard.js:2620-2623. |
| dismissed | Unthrottled XHR progress events violate AC #6 ("every 2 seconds") | FALSE POSITIVE: Misread. AC #6 states "updates at least every 2 seconds" — this is a MINIMUM frequency requirement. The XHR progress handler fires MORE frequently (on each browser progress event), which satisfies "at least every 2s." Throttling to 500ms would still satisfy the AC. Throttling to 2s would be the minimum required and is not needed since XHR events are already frequent enough. |
| dismissed | XHR object not explicitly nulled after upload — memory leak | FALSE POSITIVE: JavaScript GC handles local variable cleanup. On an embedded single-page dashboard with one upload per session, a 1-2KB XHR object is negligible. Not a real leak; the object is properly scoped. |
| dismissed | CSS line count is 41 lines, not "~35 lines" as claimed in AC #11 | FALSE POSITIVE: AC #11 says "approximately 35 lines" — 41 lines is within reasonable approximation range and has no functional impact. --- |

## Story fn-1-6 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `.ota-upload-zone.drag-over` only changes `border-color`, not `border-style` — border stays `dashed` during drag, violating AC #3 ("border becomes solid") | Added `border-style:solid` to `.ota-upload-zone.drag-over` rule |
| high | OTA buttons below 44×44px touch target minimum — `.btn-ota-cancel` renders at ~36px height with `padding:10px`; `#btn-upload-firmware` renders at ~40px with `padding:12px` — violates AC #11 | Added `min-height:44px` to `.btn-secondary`, `.btn-ota-cancel`, and `.ota-file-info .apply-btn` override |
| medium | OTA success path `otaPrefs.begin()` return value not checked before `putUChar()` — silent NVS failure leaves rollback banner permanently dismissed after a good-OTA-then-rollback sequence | Wrapped NVS calls in `if (otaPrefs.begin(...))` with `LOG_E` on failure path |
| medium | `loadFirmwareStatus()` catch block completely silent — firmware card shows blank version and hidden banner with no user feedback on network failure | Added `FW.showToast('Could not load firmware status — check connection', 'error')` to catch |
| medium | Reboot polling uses `setInterval` — if `FW.get()` hangs >3s, the next poll fires before resolution, piling concurrent requests on the ESP32 during recovery | Replaced `setInterval` with recursive `setTimeout` — next poll only fires after current request resolves (success or error) |
| low | No `xhr.timeout` on the upload XHR — a stalled upload on a degraded network hangs indefinitely with no feedback | Added `xhr.timeout = 120000` and `xhr.ontimeout` handler showing error toast and resetting UI |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 2 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 2 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Implicit dependency on ConfigManager key recognition (Architectural Coupling) | firmware/data-src/wizard.js:23 | +3 |
| 🟠 IMPORTANT | Global Mutable State (Single Responsibility Principle) | firmware/data-src/wizard.js:40 | +1 |
| 🟠 IMPORTANT | Missing API Key Format Validation | firmware/data-src/wizard.js:387 | +1 |
| 🟡 MINOR | Missing explicit `min`, `max` attributes on numeric inputs | firmware/data-src/wizard.html:79 | +0.3 |
| 🟡 MINOR | `saveAndConnect` catch block inconsistent with `FW.showToast` pattern | firmware/data-src/wizard.js:538 | +0.3 |
| 🟡 MINOR | Hardcoded `TOTAL_STEPS` | firmware/data-src/wizard.js:7 | +0.3 |
| 🟡 MINOR | Missing cache-busting for initial `/api/settings` fetch | firmware/data-src/wizard.js:63 | +0.3 |
| 🟡 MINOR | Inconsistent Unicode character usage in error messages | firmware/data-src/wizard.js:744 | +0.3 |

### Evidence Score: 4.5

| Score | Verdict |
|-------|---------|
| **4.5** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[9/10] Dependency Inversion / Open/Closed Principle Violation:** The `wizard.js` explicitly defines `WIZARD_KEYS` and `KNOWN_EXTRA_KEYS` as hardcoded string arrays. This creates a brittle, compile-time dependency on the `ConfigManager`'s key structure on the firmware side. Any change to a key name in `ConfigManager` will silently break the import logic or cause API `POST /api/settings` failures. This violates the Open/Closed Principle (code should be open for extension, but closed for modification) and Dependency Inversion (high-level module `wizard.js` depends on low-level detail `ConfigManager` key names).
  - 📍 `firmware/data-src/wizard.js:23`
  - 💡 Fix: Implement a `/api/settings/schema` endpoint on the firmware side that `wizard.js` can fetch dynamically on boot. This schema would define all recognized keys, their types, and possibly their UI grouping (wizard vs. extra). `wizard.js` would then build `WIZARD_KEYS` and `KNOWN_EXTRA_KEYS` from this schema, making the system more robust and maintainable.
- **[7/10] Global Mutable State (Single Responsibility Principle):** The `importedExtras` object is declared as a global mutable variable (`var importedExtras = {};`). While it is reset on re-import, relying on global state for data that directly influences a critical API payload (`POST /api/settings`) increases coupling and reduces testability. It forces functions to have side effects and violates the Single Responsibility Principle for `wizard.js` as a whole, which now manages both UI logic and a shared, global data repository for import.
  - 📍 `firmware/data-src/wizard.js:40`
  - 💡 Fix: Encapsulate `importedExtras` within the `processImportedSettings` closure or pass it as an argument where needed. Re-evaluate the overall state management pattern to avoid reliance on global mutable variables.

---

## 🐍 Pythonic Crimes &amp; Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance &amp; Scalability

- **[Minor] Missing cache-busting for initial `/api/settings` fetch:** The `hydrateDefaults()` function fetches `/api/settings` on wizard load. Aggressive browser caching could potentially serve stale `/api/settings` data on initial load if the firmware configuration has changed (e.g., after a manual NVS clear or device reset) without the browser performing a fresh request. This could lead to the wizard starting with incorrect defaults.
  - 📍 `firmware/data-src/wizard.js:63`
  - 💡 Fix: Add a cache-busting parameter (e.g., `?_=' + Date.now()`) to the `/api/settings` request in `hydrateDefaults()` to ensure a fresh fetch of the latest configuration.

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug / 🔒 [High] Security:** Missing API Key Format Validation. The `validateStep(2)` function only checks if the API key fields (`os_client_id`, `os_client_sec`, `aeroapi_key`) are non-empty. It does not perform any format validation (e.g., expected length, character set, UUID format). This leaves the system vulnerable to sending malformed credentials to the backend, which could result in unnecessary API calls, rate limiting, or potential (albeit low) injection vectors if the backend doesn't sanitize inputs correctly. While `parseStrictNumber` exists for numeric fields, no equivalent is used for strings that represent API keys.
  - 📍 `firmware/data-src/wizard.js:387`
  - 🔄 Reproduction: Enter "test" into an API key field. The wizard will pass validation but the backend might reject the invalid format.
- **🐛 Bug:** `saveAndConnect` catch block inconsistency with `FW.showToast` pattern. The project's "Enforcement Rules" state "Every fetch() must check json.ok and call showToast() on failure." The `saveAndConnect` function's `catch` block calls `showSaveError` which renders an inline error, but does not use `FW.showToast`. This deviates from the established project pattern for error communication on fetch failures, leading to inconsistent user feedback.
  - 📍 `firmware/data-src/wizard.js:538`
  - 🔄 Reproduction: Simulate a network error during `FW.post('/api/settings')`. An inline error appears, but no toast.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Hardcoded `TOTAL_STEPS`. The `TOTAL_STEPS` constant (value `5`) is hardcoded at the top of `wizard.js`. While simple, this creates a maintenance overhead. If steps are added or removed in the future, this value needs manual synchronization, which is prone to human error.
  - 📍 `firmware/data-src/wizard.js:7`
  - 💥 Explosion radius: Minor, but a common source of bugs in multi-step forms.
- **💣 Tech Debt:** Missing `min`/`max` HTML attributes for numeric inputs. The `wizard.html` inputs for `tiles-x`, `tiles-y`, `tile-pixels`, `display-pin`, `center-lat`, `center-lon`, `radius-km` are validated in `wizard.js` for numeric ranges and positive integers. However, the HTML `input` elements lack `min` and `max` attributes. This could provide immediate browser-level validation feedback to the user, improving UX and reducing the need for JavaScript validation to catch basic out-of-range inputs.
  - 📍 `firmware/data-src/wizard.html:79`
  - 💥 Explosion radius: Minor, impacts user experience and redundant JS validation.
- **💣 Tech Debt:** Inconsistent Unicode character usage in error messages. The error message `Could not read settings file \u2014 invalid format` uses the Unicode character `\u2014` (em dash), whereas other error messages in the `FW.showToast` system or `showSaveError` might use standard hyphens (`-`). This is a minor stylistic inconsistency that affects the uniformity of error messages.
  - 📍 `firmware/data-src/wizard.js:744`
  - 💥 Explosion radius: Negligible, purely cosmetic.

---

## 🛠️ Suggested Fixes

### 1. Architectural Fix: Dynamically Fetch Config Schema

**File:** (Conceptual, impacts `WebPortal.cpp` and `wizard.js`)
**Issue:** Hardcoded `WIZARD_KEYS` and `KNOWN_EXTRA_KEYS` in `wizard.js` create a brittle architectural dependency on firmware's `ConfigManager`. This violates the Open/Closed Principle, leading to high maintenance overhead and runtime errors if key names change.

**Conceptual Diff for `WebPortal.cpp` (Firmware-side):**
```diff
--- a/firmware/adapters/WebPortal.cpp
+++ b/firmware/adapters/WebPortal.cpp
@@ -X,X +X,X @@
 #include "core/ConfigManager.h"
 #include "core/SystemStatus.h"
 #include "utils/Log.h"
+#include <ArduinoJson.h> // For schema generation
 
 // ... existing WebPortal methods ...
 
+void WebPortal::_handleGetSettingsSchema(AsyncWebServerRequest *request) {
+    JsonDocument doc;
+    doc["wizard_keys"] = JsonArray();
+    doc["known_extra_keys"] = JsonArray();
+
+    // Populate wizard keys (example, actual logic depends on ConfigManager)
+    // This would likely be driven by ConfigManager metadata
+    doc["wizard_keys"].add("wifi_ssid");
+    doc["wizard_keys"].add("wifi_password");
+    // ... add all 12 wizard keys
+
+    // Populate known extra keys
+    doc["known_extra_keys"].add("brightness");
+    doc["known_extra_keys"].add("text_color_r");
+    // ... add all 17 known extra keys
+
+    String jsonResponse;
+    serializeJson(doc, jsonResponse);
+    request->send(200, "application/json", jsonResponse);
+}
+
 // In _initApiEndpoints (example setup)
 void WebPortal::_initApiEndpoints() {
     // ... existing endpoints ...
     _server.on("/api/settings/schema", HTTP_GET, std::bind(&WebPortal::_handleGetSettingsSchema, this, std::placeholders::_1));
     // ...
 }
```

**Conceptual Changes for `wizard.js` (Client-side):**
```javascript
// Remove hardcoded WIZARD_KEYS and KNOWN_EXTRA_KEYS
// var WIZARD_KEYS = [...];
// var KNOWN_EXTRA_KEYS = [...];

var dynamicWizardKeys = [];
var dynamicKnownExtraKeys = [];

// ... existing code ...

// Modify hydrateDefaults to fetch schema first
function hydrateDefaults() {
  FW.get('/api/settings/schema?_=' + Date.now()).then(function(schemaRes) {
    if (!schemaRes.body.ok || !schemaRes.body.data) {
      console.error("Failed to load settings schema, falling back to defaults.");
      return;
    }
    dynamicWizardKeys = schemaRes.body.data.wizard_keys || [];
    dynamicKnownExtraKeys = schemaRes.body.data.known_extra_keys || [];
    
    return FW.get('/api/settings?_=' + Date.now()); // Fetch current settings after schema
  }).then(function(settingsRes) {
    if (!settingsRes.body.ok || !settingsRes.body.data) return;
    var d = settingsRes.body.data;
    // Iterate dynamicWizardKeys to hydrate state
    dynamicWizardKeys.forEach(function(key) {
      if (!state[key] && d[key] !== undefined) {
        state[key] = String(d[key]);
      }
    });
  }).catch(function(err) {
    console.error("Error during initial settings load:", err);
    // Settings endpoint unavailable — user will fill manually
  });
}

function processImportedSettings(text) {
    // ... existing JSON parsing ...
    // Use dynamicWizardKeys and dynamicKnownExtraKeys
    for (i = 0; i < dynamicWizardKeys.length; i++) {
      key = dynamicWizardKeys[i];
      if (parsed.hasOwnProperty(key)) {
        state[key] = String(parsed[key]);
        count++;
      }
    }
    for (i = 0; i < dynamicKnownExtraKeys.length; i++) {
      key = dynamicKnownExtraKeys[i];
      if (parsed.hasOwnProperty(key)) {
        importedExtras[key] = parsed[key];
        count++;
      }
    }
    // ... rest of the function ...
}
```

---

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 0 |
| Hidden Bugs | 3 |
| Security Vulnerabilities | 1 |
| Code Quality Issues | 3 |
| **Total Issues** | **9** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | importStatus not reset on error/re-import violates AC #4 | wizard.js:623-625, 618-620 | +3 |
| 🔴 CRITICAL | No file size limit allows DoS via browser memory exhaustion | wizard.js:628-633 | +3 |
| 🟠 IMPORTANT | No MIME type validation on drag-and-drop bypasses security | wizard.js:654-658 | +1 |
| 🟠 IMPORTANT | Object-to-string coercion creates false validation passes | wizard.js:607 | +1 |
| 🟠 IMPORTANT | Missing support for dynamic NVS keys (m_*, sched_r*) limits forward compatibility | architecture.md:2822, 2859 | +1 |
| 🟡 MINOR | FileReader error shows misleading "invalid format" message | wizard.js:632 | +0.3 |
| 🟡 MINOR | No null value handling - `null` becomes "null" string | wizard.js:607 | +0.3 |
| 🟡 MINOR | Drag-over class not cleared on FileReader error | wizard.js:628-633 | +0.3 |
| 🟢 CLEAN PASS | SOLID principles | N/A | -0.5 |

### Evidence Score: **9.7**

| Score | Verdict |
|-------|---------|
| **9.7** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

**[7/10] Open/Closed Violation:** The `KNOWN_EXTRA_KEYS` hardcoded list approach breaks forward compatibility with Display System and Delight Release features.

- 📍 `wizard.js:35-42`
- 💡 **Fix:** Implement prefix-based key matching (e.g., `m_*` for per-mode settings, `sched_r*` for schedule rules) or add `display_mode` key to the list:
```javascript
var KNOWN_EXTRA_KEYS = [
  // ... existing keys ...
  'display_mode'  // Missing Display System key
];
```

**[6/10] Liskov Substitution Risk:** The import process treats all JSON values as coercible to strings, but complex objects/arrays produce garbage strings.

- 📍 `wizard.js:607`
- 💡 **Fix:** Add type validation:
```javascript
if (parsed.hasOwnProperty(key)) {
  var val = parsed[key];
  if (val === null || typeof val === 'object') {
    // Skip or warn about non-primitive values
    continue;
  }
  state[key] = String(val);
}
```

---

## 🐍 Code Quality Issues

**Type Safety:** No validation that imported values are primitives before String() coercion.

- 📍 `wizard.js:607`
- Objects become `"[object Object]"`, arrays become comma-joined strings

**Error Handling:** FileReader error handler shows "invalid format" which is misleading for actual file read errors (e.g., permission denied, disk I/O).

- 📍 `wizard.js:632`

**State Management:** `importStatus` element is never hidden once shown, violating AC #4's "import zone resets to its initial state" requirement.

- 📍 `wizard.js:623-625, 618-620`

---

## ⚡ Performance & Scalability

**[HIGH] Denial of Service via File Upload:** No file size validation before `readAsText()`.

- 📍 `wizard.js:628-633`
- A user could accidentally or maliciously drop a multi-gigabyte file, crashing the browser tab
- 💡 **Fix:** Add size check:
```javascript
function handleImportFile(file) {
  if (!file) return;
  if (file.size > 1024 * 1024) { // 1MB limit
    FW.showToast('Settings file too large (max 1MB)', 'error');
    return;
  }
  // ... rest of function
}
```

---

## 🐛 Correctness & Safety

**🐛 Bug: Stale success message on re-import error**

When a user successfully imports settings (showing "12 settings imported"), then tries to import an invalid JSON file:
1. `processImportedSettings` returns early at line 594 or 598
2. `importStatus` element retains previous success message
3. User sees both error toast AND stale success message

**Reproduction:**
1. Import valid settings file → see "X settings imported"
2. Try to import invalid JSON → error toast appears
3. Observe: success message still visible below (should be hidden)

**🐛 Bug: Drag-over visual state stuck on read error**

If `FileReader` fails (rare but possible with permission issues), the `.drag-over` class is never removed, leaving the zone visually highlighted.

**🔒 [MEDIUM] Security: No MIME type validation on drag-and-drop**

The `accept=".json"` attribute only filters the file picker dialog. Drag-and-drop accepts any file type, including `.exe`, `.bin`, or malicious files.

- 📍 `wizard.js:654-658`
- 💡 **Fix:** Add type check in `handleImportFile`:
```javascript
if (!file.type || !file.type.includes('json') || !file.name.endsWith('.json')) {
  FW.showToast('Please select a JSON file', 'error');
  return;
}
```

**🐛 Bug: null values become "null" strings**

`String(null)` produces the string `"null"`, which could pass form validation incorrectly if a user exports a file with explicit null values.

---

## 🔧 Maintainability Issues

**💣 Tech Debt: Hardcoded key list won't scale with firmware evolution**

The `KNOWN_EXTRA_KEYS` array requires manual updates for every new NVS key. Future keys like `display_mode` (Display System), `sched_r*` schedule rules, and `m_*` per-mode settings will be silently dropped during import.

- 📍 `wizard.js:35-42`
- 💥 **Explosion radius:** Users upgrading firmware will lose settings when importing backups from newer versions

---

## 🛠️ Suggested Fixes

### 1. Fix importStatus reset on error

**File:** `firmware/data-src/wizard.js`

**Issue:** AC #4 requires import zone resets to initial state on error.

```javascript
function processImportedSettings(text) {
  // Reset status at start
  importStatus.style.display = 'none';
  importStatus.textContent = '';
  
  var parsed;
  try {
    parsed = JSON.parse(text);
  } catch (e) {
    FW.showToast('Could not read settings file — invalid format', 'error');
    importZone.classList.remove('drag-over'); // Clear visual state
    return;
  }
  // ... rest of function
}
```

### 2. Add file size and type validation

**File:** `firmware/data-src/wizard.js`

```javascript
function handleImportFile(file) {
  if (!file) return;
  
  // Size limit: 1MB
  if (file.size > 1024 * 1024) {
    FW.showToast('Settings file too large (max 1MB)', 'error');
    importZone.classList.remove('drag-over');
    return;
  }
  
  // Type validation
  if (!file.name.toLowerCase().endsWith('.json')) {
    FW.showToast('Please select a .json file', 'error');
    importZone.classList.remove('drag-over');
    return;
  }
  
  // ... rest of function
}
```

### 3. Fix type coercion vulnerability

**File:** `firmware/data-src/wizard.js:604-610`

```javascript
for (i = 0; i < WIZARD_KEYS.length; i++) {
  key = WIZARD_KEYS[i];
  if (parsed.hasOwnProperty(key)) {
    var val = parsed[key];
    // Skip null, objects, arrays
    if (val === null || typeof val === 'object') {
      continue;
    }
    state[key] = String(val);
    count++;
  }
}
```

### 4. Add missing `display_mode` key to KNOWN_EXTRA_KEYS

**File:** `firmware/data-src/wizard.js:35-42`

```javascript
var KNOWN_EXTRA_KEYS = [
  // ... existing 17 keys ...
  'display_mode'  // Display System release key
];
```

---

**Review Actions:**
- Issues Found: 9
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding. The import functionality has correctness gaps (stale UI state, type coercion bugs) and security concerns (no file validation). The architecture needs forward compatibility planning for dynamic NVS keys.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**ADVERSARIAL CODE REVIEW COMPLETE**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md`

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 4 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| Total Issues | 6 |

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Invalid/no-op import paths do not reset prior imported state or UI | `firmware/data-src/wizard.js:590-625` | +1.0 |
| 🟠 IMPORTANT | Re-import mixes old and new backups because wizard state is never reset | `firmware/data-src/wizard.js:600-616` | +1.0 |
| 🟠 IMPORTANT | Importing can wipe in-progress WiFi edits by rehydrating Step 1 from stale state | `firmware/data-src/wizard.js:333-338`, `367-370`, `625` | +1.0 |
| 🟠 IMPORTANT | Recognized extra keys are never value-validated client-side; save fails late and atomically | `firmware/data-src/wizard.js:604-625`, `firmware/core/ConfigManager.cpp:104-170`, `firmware/adapters/WebPortal.cpp:754-757` | +1.0 |
| 🟡 MINOR | `parsed.hasOwnProperty(...)` can crash on user-controlled JSON | `firmware/data-src/wizard.js:606`, `613` | +0.3 |
| 🟠 IMPORTANT | No automation added; existing wizard page object cannot locate the shipped import control | `tests/e2e/page-objects/WizardPage.ts:168-173`, `firmware/data-src/wizard.html:20` | +1.0 |
| 🟢 CLEAN PASS | Performance, Security, Style | review sweep | -1.5 |

### Evidence Score: 3.8

| Score | Verdict |
|-------|---------|
| 3.8 | MAJOR REWORK |

## Correctness & Safety

- High: AC #4 is not fully implemented. On parse failure, non-object payloads, or zero-recognized-key imports, `processImportedSettings()` exits early without restoring the import zone to its initial state; after a prior successful import, the success badge and/or prior imported state remain active. Refs: [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:590), [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:623). Reproduction: import a valid backup, then import malformed JSON; the old imported data still influences the final POST.

- High: Re-import is not fresh. The code only resets `importedExtras`, then overwrites keys that exist in the new JSON; omitted wizard keys keep their old values. Refs: [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:600), [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:604). Reproduction: import backup A, then import backup B containing only WiFi keys; API/location/hardware from A silently survive into the final payload.

- High: Importing from Step 1 can erase the user’s typed WiFi credentials. Step 1 inputs are only copied into `state` on validation/navigation, but a successful import immediately calls `showStep(currentStep)`, which rehydrates the inputs from stale `state`. Refs: [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:333), [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:367), [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:625). Reproduction: type SSID/password manually, import a file with only `brightness`, and watch the typed fields revert.

- High: The import path validates names but not values. Any recognized extra key is counted as successfully imported even when the firmware will reject it and abort the entire `/api/settings` POST. Refs: [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:604), [ConfigManager.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ConfigManager.cpp:104), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:754). Reproduction: import `{"zone_layout":2}` or `{"sched_enabled":"1"}`; the wizard shows success, but Save & Connect fails at the end.

- Medium: `parsed.hasOwnProperty(key)` is unsafe on imported JSON. A file containing `"hasOwnProperty": 1` turns that lookup into a TypeError and bypasses the normal toast/reset behavior. Refs: [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:606), [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:613).

## Maintainability Issues

- Medium: This shipped with no usable automation. There are no story-related changes under `tests/e2e` or `firmware/test`, the only E2E spec in the tree is `dashboard.spec.ts`, and the existing wizard page object still looks for a literal `<button>` while the implementation uses a `<div role="button">`. Refs: [WizardPage.ts](/Users/christianlee/App-Development/TheFlightWall_OSS-main/tests/e2e/page-objects/WizardPage.ts:168), [wizard.html](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.html:20), [dashboard.spec.ts](/Users/christianlee/App-Development/TheFlightWall_OSS-main/tests/e2e/tests/dashboard.spec.ts:1).

## Performance & Scalability

✅ No significant performance regressions found in this client-side change.

## Security

✅ No new direct security vulnerability stood out in the reviewed import path.

## Suggested Fixes

1. Add a single `resetImportedState()` path that clears `importedExtras`, restores or preserves intended wizard defaults explicitly, and hides `#import-status` on parse failure, read failure, and zero-recognized-key imports.
2. Save the current Step 1 inputs into `state` before any import-driven re-render, or stop rehydrating the current step unless imported keys actually changed its fields.
3. Validate imported values before showing success, especially hidden extra keys that the user cannot inspect in the wizard UI.
4. Replace `parsed.hasOwnProperty(...)` with `Object.prototype.hasOwnProperty.call(parsed, key)` and add a Playwright wizard import spec using a role-based locator for the control.

**Review Actions:** Issues Found: 6. Issues Fixed: 0. Action Items Created: 6.

⚠️ Address the identified issues before proceeding.

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
<var name="session_id">bc6f4e19-a85f-4ba7-b7bb-025011401bef</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="ff10d1c0">embedded in prompt, file id: ff10d1c0</var>
<var name="story_id">fn-1.7</var>
<var name="story_key">fn-1-7-settings-import-in-setup-wizard</var>
<var name="story_num">7</var>
<var name="story_title">7-settings-import-in-setup-wizard</var>
<var name="template">False</var>
<var name="timestamp">20260412_2131</var>
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
      - Commit message format: fix(component): brief description (synthesis-fn-1.7)
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