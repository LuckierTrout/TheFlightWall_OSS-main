<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 7 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T031650Z -->
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

**2026-04-12: Code review synthesis round 2 — 3 issues fixed**
- HIGH (wizard.js): `saveCurrentStepState()` was placed AFTER import populated `state`, causing DOM field reads (empty strings) to overwrite imported values before `showStep()` rehydrated. AC #2 was broken — Step 1 WiFi fields never showed imported values. Fixed by moving `saveCurrentStepState()` to the VERY FIRST line of `processImportedSettings()`, before JSON parsing. Imported values now correctly win over user-typed DOM values.
- HIGH (WizardPage.ts): `configureMatrix()` called `tilePixelsSelect.selectOption()` on `#tile-pixels` which is `<input type="text">`, not `<select>`. Playwright's `selectOption()` throws on text inputs. Fixed to `fillInput()`.
- MEDIUM (WizardPage.ts): `stepIndicator` getter targeted `.step-indicator, .wizard-steps` — neither of these selectors exists in the 5-step wizard HTML. The real element is `#progress`. Fixed locator.
- wizard.js.gz regenerated at 6404 bytes (within 50KB budget, under 10KB per file).

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
| `firmware/data-src/wizard.js` | Modified (synthesis) — fixed importStatus reset, saveCurrentStepState() before rehydration, Object.prototype.hasOwnProperty.call(), null/object type guards, 1 MB file-size check |
| `firmware/data/wizard.js.gz` | Regenerated (synthesis) — 6402 bytes |
| `tests/e2e/page-objects/WizardPage.ts` | Modified (synthesis) — fixed importSettingsButton locator from 'button' to '#settings-import-zone' |
| `firmware/data-src/wizard.js` | Modified (synthesis round 2) — moved saveCurrentStepState() to top of processImportedSettings() so imported values win over empty DOM fields (AC #2 fix) |
| `firmware/data/wizard.js.gz` | Regenerated (synthesis round 2) — 6404 bytes |
| `tests/e2e/page-objects/WizardPage.ts` | Modified (synthesis round 2) — fixed stepIndicator locator (#progress), fixed configureMatrix() tilePixels from selectOption() to fillInput() |

## Senior Developer Review (AI)

### Review: 2026-04-12
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 4.5 / 9.7 / 3.8 (three reviewers, all MAJOR REWORK) → Changes Requested
- **Issues Found:** 6 verified (after dismissing 13 false positives across 3 reviewers)
- **Issues Fixed:** 6
- **Action Items Created:** 0

### Review: 2026-04-12 (Round 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 6.9 / 5.3 / 5.8 (three reviewers, all MAJOR REWORK) → Approved after fixes
- **Issues Found:** 3 verified (after dismissing 12 false positives across 3 reviewers)
- **Issues Fixed:** 3
- **Action Items Created:** 0


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

## Story fn-1-7 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `importStatus` element not hidden on error/zero-count paths — stale "N settings imported" badge persists after a failed re-import, violating AC #4 ("import zone resets to its initial state") | Moved `importStatus.style.display = 'none'`, `importStatus.textContent = ''`, and `importedExtras = {}` to the very top of `processImportedSettings()`, before the try/catch. Covers all error exit paths. |
| high | `showStep(currentStep)` called from `processImportedSettings()` without first flushing in-progress input — if the user typed values into Step 1 that hadn't been saved to `state`, the subsequent rehydration would overwrite those typed values with the (empty) stale state | Added `saveCurrentStepState()` call immediately before `showStep(currentStep)` in the success path. |
| medium | `parsed.hasOwnProperty(key)` is unsafe on user-controlled JSON — a file containing `{"hasOwnProperty": 1}` turns the method lookup into a TypeError and bypasses all normal toast/reset behavior | Replaced both occurrences with `Object.prototype.hasOwnProperty.call(parsed, key)`. |
| medium | No file-size guard in `handleImportFile()` — a multi-MB file passed via drag-and-drop could hang the browser tab on `FileReader.readAsText()` | Added `if (file.size > 1024 * 1024)` check with error toast before the FileReader is created. |
| low | `String(null)` → `"null"`, `String({})` → `"[object Object]"` — a hand-crafted import file with `null` or nested-object values could silently write garbage strings into wizard `state` fields, which would then pass through to the POST payload | Added `if (val !== null && typeof val !== 'object')` type guard in both the WIZARD_KEYS and KNOWN_EXTRA_KEYS loops before assignment. |
| low | `WizardPage.importSettingsButton` locator targets `<button>` elements, but the import zone is a `<div role="button">` — the locator would never match and any test using it would fail | Replaced `this.page.locator('button', { hasText: /import/i })` with `this.page.locator('#settings-import-zone')`. |
| dismissed | Hardcoded `WIZARD_KEYS`/`KNOWN_EXTRA_KEYS` violates OCP; fix with `/api/settings/schema` endpoint | FALSE POSITIVE: The story explicitly states "Do NOT create a new server-side endpoint." This is a client-side-only change by design. A schema endpoint would add firmware code, flash usage, and a network round-trip to the boot path for no practical gain on a single-firmware device. |
| dismissed | `importedExtras` is a "global mutable state" SRP violation | FALSE POSITIVE: The entire `wizard.js` runs inside an IIFE — `importedExtras` is a module-level closure variable, not a true global. This is the established pattern for the project's vanilla-JS embedded UI (see `state`, `scanTimer`, `manualMode`). |
| dismissed | Missing API-key format validation in `validateStep(2)` | FALSE POSITIVE: Pre-existing pattern present before fn-1.7 and outside story scope. No change introduced by this story. |
| dismissed | Missing `min`/`max` HTML attributes on numeric inputs | FALSE POSITIVE: Pre-existing pattern; not introduced by fn-1.7. |
| dismissed | `saveAndConnect` catch block uses `showSaveError` instead of `FW.showToast` | FALSE POSITIVE: Pre-existing intentional pattern — `showSaveError` renders inline within the review section because Step 5 is still visible. Not introduced by fn-1.7. |
| dismissed | Hardcoded `TOTAL_STEPS` | FALSE POSITIVE: Pre-existing; not introduced by fn-1.7. |
| dismissed | Missing cache-busting on `hydrateDefaults()` fetch | FALSE POSITIVE: Pre-existing; not introduced by fn-1.7. ESP32 AsyncWebServer does not cache API responses. |
| dismissed | Em dash `\u2014` in error message is inconsistent with other error messages using `-` | FALSE POSITIVE: The AC #4 spec text reads "Could not read settings file — invalid format" — the em dash is the spec-required format. Correct as written. |
| dismissed | Re-import mixes wizard state keys from old and new files | FALSE POSITIVE: The story explicitly specifies "reset `importedExtras = {}` and re-process." Wizard state keys are intentionally additive so that a targeted re-import (WiFi-only file) doesn't wipe hardware/location already present. This is a documented design decision. |
| dismissed | MIME-type validation missing on drag-and-drop | FALSE POSITIVE: This is a single-user trusted LAN device with no authentication on any endpoint. The `accept=".json"` file-picker filter is adequate UX guidance. A strict MIME check would also cause false positives on some OS/browser combinations that assign `application/octet-stream` to `.json` files. |
| dismissed | FileReader `onerror` message says "invalid format" when the actual cause could be a permission/IO error | FALSE POSITIVE: AC #4 specifies exactly: "an error toast shows 'Could not read settings file — invalid format'." No distinction is required by the spec. |
| dismissed | Drag-over CSS class not cleared when FileReader errors | FALSE POSITIVE: False positive. The `drop` event handler calls `importZone.classList.remove('drag-over')` synchronously *before* `handleImportFile()` is called. The FileReader fires asynchronously; by that point `.drag-over` is already gone. |
| dismissed | `KNOWN_EXTRA_KEYS` needs `display_mode` and future `m_*`/`sched_r*` keys | FALSE POSITIVE: Future firmware keys are a known design tradeoff documented in the story's Risk Mitigation section. The list is intentionally bounded to the 30 keys that `ConfigManager::applyJson()` accepts today. Adding speculative future keys would cause 400 errors if those keys don't exist in current firmware. --- |


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
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | The story relies on manual test cases for the import functionality, with no evidence of corresponding automated end-to-end tests (e.g., using Playwright's `WizardPage.ts` object) for critical scenarios like successful import, various error cases (invalid JSON, no recognized keys), and drag-and-drop. This leaves the feature vulnerable to regressions and reduces confidence in its long-term stability. | _bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md | +1 |
| 🟠 IMPORTANT | The `KNOWN_EXTRA_KEYS` array in `firmware/data-src/wizard.js` is a hardcoded, client-side list of configuration keys that are not part of the wizard's direct fields but should be preserved during an import. This design forces the client-side code to be perfectly in sync with all NVS keys the firmware's `ConfigManager` supports. If the firmware adds new configurable keys in future releases (e.g., for new Delight Release features) and `KNOWN_EXTRA_KEYS` is not manually updated in `wizard.js`, importing a settings file containing these new keys will result in them being silently omitted from the `POST /api/settings` payload. This can lead to silent data loss or unexpected device behavior, especially in an evolving OSS project, making it a significant maintainability and forward-compatibility risk. The dismissal of a schema API in `Dev Notes` does not negate this architectural vulnerability. | firmware/data-src/wizard.js | +1 |
| 🟡 MINOR | The `wizard.js` file encapsulates a broad range of responsibilities including WiFi scanning, geolocation, multi-step form state management, validation for four distinct steps, review generation, persistence to an API, and now settings import/drag-and-drop. This violates the Single Responsibility Principle, making the file large (450+ lines), complex, and harder to maintain or extend without unintended side effects. | firmware/data-src/wizard.js | +0.3 |
| 🟡 MINOR | The `importFileInput.value = '';` is called immediately after `handleImportFile` is invoked, which then uses the asynchronous `FileReader.readAsText()`. If a user rapidly selects multiple files, the `importFileInput.value` could be cleared before the `change` event listener processes the file, potentially causing the second (or subsequent) file selection to be missed or mishandled. This creates a minor race condition. | firmware/data-src/wizard.js | +0.3 |
| 🟡 MINOR | The `state` object (lines 13-24) and `WIZARD_KEYS` array (lines 28-33) in `firmware/data-src/wizard.js` effectively define the same set of 12 wizard-managed keys. This redundancy is a violation of the DRY principle and introduces a minor risk of inconsistency if `state` keys were to change without updating `WIZARD_KEYS`. | firmware/data-src/wizard.js | +0.3 |
| 🟡 MINOR | The `TOTAL_STEPS` variable (line 10 in `firmware/data-src/wizard.js`) is a magic number (hardcoded `5`) representing the total number of wizard steps. While not immediately problematic, any future modification to the wizard's step count would require a manual update of this value, increasing the risk of subtle bugs if overlooked. | firmware/data-src/wizard.js | +0.3 |

### Evidence Score: 3.9

| Score | Verdict |
|-------|---------|
| **3.9** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[6/10] Single Responsibility Principle:** The `wizard.js` file encapsulates a broad range of responsibilities including WiFi scanning, geolocation, multi-step form state management, validation for four distinct steps, review generation, persistence to an API, and now settings import/drag-and-drop. This violates the Single Responsibility Principle, making the file large (450+ lines), complex, and harder to maintain or extend without unintended side effects.
  - 📍 `firmware/data-src/wizard.js`
  - 💡 Fix: Refactor `wizard.js` into smaller, more focused modules, each handling a single responsibility (e.g., `wifi-scanner.js`, `geolocation-manager.js`, `form-validator.js`, `settings-importer.js`), and orchestrate them in a main `wizard-app.js` file.

---

## 🐍 Pythonic Crimes &amp; Readability

- **DRY Violation:** The `state` object (lines 13-24) and `WIZARD_KEYS` array (lines 28-33) in `firmware/data-src/wizard.js` effectively define the same set of 12 wizard-managed keys. This redundancy is a violation of the DRY principle and introduces a minor risk of inconsistency if `state` keys were to change without updating `WIZARD_KEYS`.
  - 📍 `firmware/data-src/wizard.js`
- **Magic Number:** The `TOTAL_STEPS` variable (line 10 in `firmware/data-src/wizard.js`) is a magic number (hardcoded `5`) representing the total number of wizard steps. While not immediately problematic, any future modification to the wizard's step count would require a manual update of this value, increasing the risk of subtle bugs if overlooked.
  - 📍 `firmware/data-src/wizard.js`

---

## ⚡ Performance &amp; Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug:** The `importFileInput.value = '';` is called immediately after `handleImportFile` is invoked, which then uses the asynchronous `FileReader.readAsText()`. If a user rapidly selects multiple files, the `importFileInput.value` could be cleared before the `change` event listener processes the file, potentially causing the second (or subsequent) file selection to be missed or mishandled. This creates a minor race condition.
  - 📍 `firmware/data-src/wizard.js:429`
  - 🔄 Reproduction: Rapidly select two different JSON files for import. The second import might fail or not trigger due to the input's value being prematurely cleared.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** The `KNOWN_EXTRA_KEYS` array in `firmware/data-src/wizard.js` is a hardcoded, client-side list of configuration keys that are not part of the wizard's direct fields but should be preserved during an import. This design forces the client-side code to be perfectly in sync with all NVS keys the firmware's `ConfigManager` supports. If the firmware adds new configurable keys in future releases (e.g., for new Delight Release features) and `KNOWN_EXTRA_KEYS` is not manually updated in `wizard.js`, importing a settings file containing these new keys will result in them being silently omitted from the `POST /api/settings` payload. This can lead to silent data loss or unexpected device behavior, especially in an evolving OSS project, making it a significant maintainability and forward-compatibility risk. The dismissal of a schema API in `Dev Notes` does not negate this architectural vulnerability.
  - 📍 `firmware/data-src/wizard.js`
  - 💥 Explosion radius: Future firmware updates introducing new configurable NVS keys will lead to silent data loss during settings import, causing device misconfiguration, difficult-to-diagnose bugs, and a poor user experience. Requires manual JS updates with every firmware change.

---

## 🛠️ Suggested Fixes

### 1. Implement comprehensive E2E tests for settings import

**File:** `tests/e2e/page-objects/WizardPage.ts`
**Issue:** The story relies on manual test cases for the import functionality, with no evidence of corresponding automated end-to-end tests (e.g., using Playwright's `WizardPage.ts` object) for critical scenarios like successful import, various error cases (invalid JSON, no recognized keys), and drag-and-drop. This leaves the feature vulnerable to regressions and reduces confidence in its long-term stability.

**Corrected code:**
```typescript
import { type Locator, expect } from ' @playwright/test';
import { BasePage } from './BasePage.js';

export type WizardStep =
  | 'wifi'
  | 'api'
  | 'location'
  | 'hardware'
  | 'test'
  | 'complete';

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
    return this.page.locator('button', { hasText: /next|continue/i });
  }

  get backButton(): Locator {
    return this.page.locator('button', { hasText: /back|previous/i });
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

  get tilePixelsInput(): Locator { // Renamed from tilePixelsSelect to reflect input type
    return this.page.locator('#tile-pixels, [name="tile_pixels"]');
  }

  get displayPinInput(): Locator {
    return this.page.locator('#display-pin, [name="display_pin"]');
  }

  get previewCanvas(): Locator {
    return this.page.locator('#preview-canvas, canvas');
  }

  // ============================================================================
  // Step 5: Test Display
  // ============================================================================

  get testPatternButton(): Locator {
    return this.page.locator('button', { hasText: /test pattern|show test/i });
  }

  get calibrationModeToggle(): Locator {
    return this.page.locator('#calibration-mode, [name="calibration"]');
  }

  // ============================================================================
  // Step 6: Complete
  // ============================================================================

  get completeMessage(): Locator {
    return this.page.locator('text=/configuration saved/i'); // Updated to match actual text
  }

  get rebootButton(): Locator {
    return this.page.locator('button', { hasText: /save & connect/i }); // Updated to match actual text
  }

  // ============================================================================
  // Settings Import/Export (Foundation Release)
  // ============================================================================

  get importSettingsButton(): Locator {
    // Import zone is a div[role="button"], not a <button> element
    return this.page.locator('#settings-import-zone');
  }

  get importFileInput(): Locator {
    return this.page.locator('input[type="file"]#import-file-input'); // Added #id for specificity
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
    // Try to extract from step indicator or URL
    const indicator = await this.stepIndicator.textContent();
    const match = indicator?.match(/step\s*(\d+)/i);
    if (match) {
      return parseInt(match[1], 10);
    }
    // Fallback: check which step elements are visible
    if (await this.page.locator('#step-1').isVisible()) return 1;
    if (await this.page.locator('#step-2').isVisible()) return 2;
    if (await this.page.locator('#step-3').isVisible()) return 3;
    if (await this.page.locator('#step-4').isVisible()) return 4;
    if (await this.page.locator('#step-5').isVisible()) return 5;
    // if (await this.completeMessage.isVisible()) return 6; // Removed, as complete message is on step 5, after save.
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
   * Scan for available WiFi networks.
   */
  async scanForNetworks(): Promise<void> {
    // Assuming the wizard automatically starts scanning.
    // If there's a button to explicitly start scan, it needs to be added here.
    // For now, wait for scan status to disappear or results to appear.
    await this.page.locator('#scan-status', { hasText: 'Scanning for networks...' }).waitFor({ state: 'hidden' });
    await expect(this.wifiNetworkList).toBeVisible();
  }

  /**
   * Select a network from the scan results.
   */
  async selectNetwork(ssid: string): Promise<void> {
    const networkItem = this.wifiNetworkList.locator(`.scan-row .ssid:has-text("${ssid}")`);
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
    await this.fillInput(this.tilePixelsInput, String(tilePixels)); // Changed to tilePixelsInput
  }

  /**
   * Set the LED data pin.
   */
  async setDisplayPin(pin: number): Promise<void> {
    await this.fillInput(this.displayPinInput, String(pin));
  }

  // ============================================================================
  // Step 5: Review Actions (Test Display is no longer a separate step)
  // ============================================================================

  // There is no explicit "test display" step in wizard.html, it's part of review.
  // The testPatternButton is not present in wizard.html

  // ============================================================================
  // Step 5: Complete Actions (Review & Connect)
  // ============================================================================

  /**
   * Complete the wizard and reboot the device.
   */
  async completeWizard(): Promise<void> {
    await expect(this.nextButton).toHaveText(/Save & Connect/i); // Expect 'Save & Connect' button text
    await this.nextButton.click();
    // Device will reboot, connection will be lost. Wait for the handoff message.
    await expect(this.completeMessage).toBeVisible();
    await this.page.waitForLoadState('networkidle');
  }

  // ============================================================================
  // Full Wizard Flow
  // ============================================================================

  /**
   * Complete the entire wizard with provided configuration.
   */
  async completeSetup(config: {
    wifi: { ssid: string; password: string };
    api: {
      openSkyClientId: string;
      openSkyClientSecret: string;
      aeroApiKey: string;
    };
    location: { lat: number; lon: number; radiusKm: number };
    hardware: { tilesX: number; tilesY: number; tilePixels: number; displayPin: number };
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
    await this.setDisplayPin(config.hardware.displayPin);
    await this.goNext();

    // Step 5: Review & Connect (Final step, no skip here)
    await this.completeWizard();
  }

  /**
   * Imports settings from a JSON file.
   */
  async importSettings(filePath: string, expectedToastMessage: string, toastType: 'success' | 'warning' | 'error'): Promise<void> {
    await this.importSettingsButton.click();
    await this.importFileInput.setInputFiles(filePath);
    await expect(this.page.locator(`.toast-${toastType}:has-text("${expectedToastMessage}")`)).toBeVisible();
  }

  /**
   * Imports settings from a JSON string (simulating file content).
   */
  async importSettingsFromJson(jsonContent: string, expectedToastMessage: string, toastType: 'success' | 'warning' | 'error'): Promise<void> {
    // This would require a way to inject content into FileReader, typically not directly possible in Playwright.
    // A test helper or mock would be needed. For now, we simulate file upload.
    // The actual implementation should use importSettings with a temporary file.
    // For demonstration, let's assume a mock can be set up if needed for client-side content.
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
   * Assert WiFi scan results are displayed.
   */
  async expectNetworksVisible(): Promise<void> {
    await expect(this.wifiNetworkList).toBeVisible();
    const networks = this.wifiNetworkList.locator('.scan-row'); // Updated selector
    await expect(networks).toHaveCount(
      expect.any(Number) as unknown as number
    );
  }

  /**
   * Assert the preview canvas is rendering.
   */
  async expectPreviewVisible(): Promise<void> {
    await expect(this.previewCanvas).toBeVisible();
  }

  /**
   * Assert a specific form field has a given value.
   */
  async expectFieldHasValue(locator: Locator, value: string): Promise<void> {
    await expect(locator).toHaveValue(value);
  }
}

```

---

**Review Actions:**
- Issues Found: 6
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
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **4** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Failed re-imports can still submit stale hidden settings from the previous import. | `firmware/data-src/wizard.js:503`, `firmware/data-src/wizard.js:591`, `firmware/data-src/wizard.js:639` | +3 |
| 🟠 IMPORTANT | Extra imported keys are counted as success before client-side value validation, but the server rejects the whole payload on one bad hidden extra. | `firmware/data-src/wizard.js:608`, `firmware/core/ConfigManager.cpp:135`, `firmware/adapters/WebPortal.cpp:754` | +1 |
| 🟠 IMPORTANT | `WizardPage` still models an obsolete 6-step wizard, so `completeSetup()` cannot finish the real 5-step flow. | `tests/e2e/page-objects/WizardPage.ts:15`, `tests/e2e/page-objects/WizardPage.ts:363`, `tests/e2e/page-objects/WizardPage.ts:380`, `firmware/data-src/wizard.html:92` | +1 |
| 🟠 IMPORTANT | The Wi-Fi page-object helpers do not match the real DOM and include a bogus count assertion, so they cannot validate Step 1 correctly. | `tests/e2e/page-objects/WizardPage.ts:68`, `tests/e2e/page-objects/WizardPage.ts:242`, `tests/e2e/page-objects/WizardPage.ts:440`, `firmware/data-src/wizard.js:166` | +1 |
| 🟢 CLEAN PASS | No new auth/injection/secret-handling regression was introduced in story scope. | `firmware/data-src/wizard.js`, `firmware/data-src/wizard.html` | -0.5 |

### Evidence Score: 5.5

| Score | Verdict |
|-------|---------|
| **5.5** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[8/10] Wrong abstraction:** `tests/e2e/page-objects/WizardPage.ts:15`, `:144`, `:156`, `:380` codifies a 6-step wizard with `test` and `complete` phases that do not exist in `firmware/data-src/wizard.html:92-103`. Fix: collapse the page object to the actual 5-step contract and make the final step submit through the real `Save & Connect` button.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `firmware/data-src/wizard.js:591-648` clears `importedExtras` only inside `processImportedSettings()`. If a user successfully imports once, then selects an oversized file or hits `FileReader.onerror`, the old extras survive and are still merged by `saveAndConnect()` at `wizard.js:503-509`. Reproduction: import a valid backup with `brightness`, then select a >1 MB file; the UI errors, but the old hidden extras still POST.
- **🐛 Bug:** `firmware/data-src/wizard.js:608-625` treats any recognized extra key as imported without validating value shape. `ConfigManager` explicitly rejects bad hidden values such as `timezone: 123` or `sched_enabled: true` at `firmware/core/ConfigManager.cpp:135-169`, and `WebPortal` rejects the entire request when one key fails at `firmware/adapters/WebPortal.cpp:754-757`. Result: the user sees a success toast, but Step 5 save fails with a generic error and no visible field to correct.

---

## 🎭 Lying Tests

- **Lying Test Harness:** `tests/e2e/page-objects/WizardPage.ts:380-423` calls `skipStep()` and then waits for a nonexistent completion screen in `completeWizard()`. Against the shipped wizard in `firmware/data-src/wizard.html:92-103`, this helper can never complete a real onboarding run.
- **Lying Test Assertion:** `tests/e2e/page-objects/WizardPage.ts:242-250` clicks a nonexistent scan button, and `:440-445` asserts `toHaveCount(expect.any(Number) as unknown as number)` against `li, .network-item` elements that are never rendered. The real scan rows are `.scan-row` from `firmware/data-src/wizard.js:166-171`.

---

## 🔧 Suggested Fixes

1. Reset `importedExtras` and `importStatus` in `handleImportFile()` before every early return, and repeat the cleanup in `reader.onerror`.
2. Validate `KNOWN_EXTRA_KEYS` with the same type/range rules the server enforces; ignore invalid extras and report them separately instead of counting them as imported.
3. Rewrite `WizardPage` to the real 5-step flow and make `completeSetup()` click the actual Step 5 `Save & Connect` button.
4. Replace the Wi-Fi helpers with auto-scan-aware logic that waits for `.scan-row`, and use a real numeric assertion instead of `expect.any(Number)`.

---

**Review Actions:**
- Issues Found: 4
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 0 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task 3 claimed completion but contained 5 significant bugs - unsafe hasOwnProperty, missing type guards, no file size check, UI reset issues, data loss on rehydration | `_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md:Task 3 [x]` | +3 |
| 🔴 CRITICAL | WizardPage.ts importSettingsButton locator used incorrect selector - lying test would fail to interact with import zone | `tests/e2e/page-objects/WizardPage.ts:168-171` | +3 |
| 🟠 IMPORTANT | WizardPage.ts modified but not listed in story's "Files to modify" section | `_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md:File List` | +1 |
| 🟠 IMPORTANT | Missing `display_mode` in KNOWN_EXTRA_KEYS - architecture DS6 defines this NVS key but it's excluded from import | `firmware/data-src/wizard.js:36-42` | +1 |
| 🟡 MINOR | Error message string "Could not read settings file — invalid format" repeated 3× violates DRY | `firmware/data-src/wizard.js:599,603,647` | +0.3 |
| 🟡 MINOR | Import zone uses block elements (`<p>`) inside `role="button"` container - ARIA violation | `firmware/data-src/wizard.html:20-24` | +0.3 |
| 🟢 CLEAN PASS | Security vulnerabilities | n/a | -0.5 |
| 🟢 CLEAN PASS | Performance issues | n/a | -0.5 |
| 🟢 CLEAN PASS | Style/readability (beyond noted issues) | n/a | -0.5 |
| 🟢 CLEAN PASS | Type safety (for JavaScript context) | n/a | -0.5 |

### Evidence Score: 6.6

| Score | Verdict |
|-------|---------|
| **6.6** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[5/10] Single Responsibility Violation:** The `processImportedSettings()` function handles JSON parsing, validation, state mutation, UI updates, and error handling all in one 50-line function. This makes testing difficult and violates SRP.
  - 📍 `firmware/data-src/wizard.js:588-637`
  - 💡 Fix: Split into `parseSettingsFile()`, `validateSettings()`, `applySettingsToState()`, and `updateImportUI()` functions

- **Over-engineering:** The `KNOWN_EXTRA_KEYS` whitelist approach is defensive but creates maintenance burden. Every new config key requires updating this client-side array, creating a deployment coordination problem between firmware and web assets.
  - 📍 `firmware/data-src/wizard.js:36-42`

---

## 🐍 Code Quality & Readability

- **DRY Violation:** Error message "Could not read settings file — invalid format" is duplicated across three code paths (JSON parse failure, type check failure, FileReader error).
  - 📍 `firmware/data-src/wizard.js:599,603,647`

- **ARIA Violation:** The import zone uses `<p>` block elements inside a `role="button"` container. Buttons should only contain phrasing content per ARIA spec.
  - 📍 `firmware/data-src/wizard.html:20-24`

- **Misleading Naming:** `tilePixelsSelect` in WizardPage.ts implies a `<select>` element but the actual DOM uses `<input type="text">`. This naming creates confusion for developers.
  - 📍 `tests/e2e/page-objects/WizardPage.ts:128-130`

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected. File size check (1MB limit) prevents memory exhaustion from large files.

---

## 🐛 Correctness & Safety

✅ No active hidden bugs detected in final code. Previous critical bugs (hasOwnProperty vulnerability, type guards, file size check, UI reset, saveCurrentStepState placement) have been addressed in synthesis.

- **🎭 Lying Test (FIXED in synthesis):** `importSettingsButton` locator initially targeted `'button'` but import zone is a `div[role="button"]`. Test would fail to locate element. Fixed to `'#settings-import-zone'`.
  - 📍 `tests/e2e/page-objects/WizardPage.ts:168-171`

---

## 🔧 Maintainability Issues

- **💣 Config Key Synchronization Debt:** `KNOWN_EXTRA_KEYS` is missing `display_mode` (defined in Architecture DS6) and potentially future `m_*` mode settings keys. When firmware adds new config keys, this client-side array must be manually updated or imports will silently drop those settings.
  - 📍 `firmware/data-src/wizard.js:36-42`
  - 💥 Explosion radius: User exports settings with new firmware, imports on older firmware web UI - new settings are silently lost. When they upgrade firmware, those settings are missing.

- **💣 Task Completion Documentation Lie:** Task 3 was marked [x] complete but contained 5 significant bugs that required synthesis fixes. This indicates inadequate self-testing before task completion.
  - 📍 `_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md:Task 3`
  - 💥 Explosion radius: Future stories may trust incomplete implementations, leading to cascading bugs.

- **💣 File List Incompleteness:** `tests/e2e/page-objects/WizardPage.ts` was modified (import zone locators added) but not listed in "Files to modify" section. Documentation doesn't match actual changes.
  - 📍 `_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md:File List`
  - 💥 Explosion radius: Code review may miss test file changes, leading to untested code paths.

---

## 🛠️ Suggested Fixes

### 1. Extract Error Message Constant

**File:** `firmware/data-src/wizard.js`

```javascript
// Add near top of IIFE
var IMPORT_ERROR_INVALID_FORMAT = 'Could not read settings file \u2014 invalid format';

// Replace occurrences at lines 599, 603, 647
FW.showToast(IMPORT_ERROR_INVALID_FORMAT, 'error');
```

### 2. Add Missing Config Keys to KNOWN_EXTRA_KEYS

**File:** `firmware/data-src/wizard.js`

```javascript
var KNOWN_EXTRA_KEYS = [
  'brightness', 'text_color_r', 'text_color_g', 'text_color_b',
  'origin_corner', 'scan_dir', 'zigzag',
  'zone_logo_pct', 'zone_split_pct', 'zone_layout',
  'fetch_interval', 'display_cycle',
  'timezone', 'sched_enabled', 'sched_dim_start', 'sched_dim_end', 'sched_dim_brt',
  'display_mode'  // ADD: Architecture DS6 NVS key
];
```

### 3. Fix HTML Semantics for Import Zone

**File:** `firmware/data-src/wizard.html`

```html
<!-- Change from <p> to <span> with display:block -->
<div class="settings-import-zone" id="settings-import-zone" role="button" tabindex="0" aria-label="Select settings backup file to import">
  <span class="upload-prompt" style="display:block">Have a backup? <strong>Import settings</strong></span>
  <span class="upload-hint" style="display:block">Select a flightwall-settings.json file</span>
  <input type="file" id="import-file-input" accept=".json" hidden>
</div>
```

### 4. Rename Misleading Locator

**File:** `tests/e2e/page-objects/WizardPage.ts`

```typescript
// Rename from tilePixelsSelect to tilePixelsInput for clarity
get tilePixelsInput(): Locator {
  return this.page.locator('#tile-pixels, [name="tile_pixels"]');
}
```

---

## 📋 AC Compliance Verification

| AC | Status | Evidence |
|----|--------|----------|
| #1 - Import zone at top of Step 1 with accessibility | ✅ IMPLEMENTED | `wizard.html:20-24` - proper role, tabindex, aria-label |
| #2 - Valid JSON pre-fills form fields | ✅ IMPLEMENTED | `wizard.js:608-617,633-636` - WIZARD_KEYS loop, String coercion, success toast |
| #3 - Unrecognized keys silently ignored | ✅ IMPLEMENTED | `wizard.js:608-628` - only processes WIZARD_KEYS + KNOWN_EXTRA_KEYS |
| #4 - Invalid JSON shows error toast | ✅ IMPLEMENTED | `wizard.js:596-604,647` - try/catch, error toast, input reset |
| #5 - No recognized keys shows warning | ✅ IMPLEMENTED | `wizard.js:629-631` - count === 0 check with warning toast |
| #6 - Settings applied via POST with extras | ✅ IMPLEMENTED | `wizard.js:504-508` - importedExtras merged into payload |
| #7 - Wizard works without import | ✅ IMPLEMENTED | No blocking import code; normal flow unaffected |
| #8 - Pre-filled fields visually normal | ✅ IMPLEMENTED | Uses same state/hydration mechanism as manual entry |
| #9 - Gzipped assets regenerated | ✅ CLAIMED | Story claims completion; no verification possible in review |

---

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0 (synthesis already addressed functional bugs)
- Action Items Created: 4 suggested fixes

⚠️ Address the identified documentation gaps and minor code quality issues before proceeding. The core functionality is now correct after synthesis fixes.

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
<var name="session_id">daef53a7-8251-4d88-9172-02d796b993f6</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="ff10d1c0">embedded in prompt, file id: ff10d1c0</var>
<var name="story_id">fn-1.7</var>
<var name="story_key">fn-1-7-settings-import-in-setup-wizard</var>
<var name="story_num">7</var>
<var name="story_title">7-settings-import-in-setup-wizard</var>
<var name="template">False</var>
<var name="timestamp">20260412_2316</var>
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