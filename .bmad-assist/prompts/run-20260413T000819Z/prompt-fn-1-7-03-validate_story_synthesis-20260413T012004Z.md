<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 7 -->
<!-- Phase: validate-story-synthesis -->
<!-- Timestamp: 20260413T012004Z -->
<compiled-workflow>
<mission><![CDATA[

Master Synthesis: Story fn-1.7

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
<file id="ff10d1c0" path="_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md" label="DOCUMENTATION"><![CDATA[



# Story fn-1.7: Settings Import in Setup Wizard

Status: ready-for-dev

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

- [ ] **Task 1: Add import zone HTML to wizard.html** (AC: #1, #7)
  - [ ] Add a settings import section at the top of Step 1 (`#step-1`), BEFORE the `#scan-status` div
  - [ ] HTML structure:
    ```html
    <div class="settings-import-zone" id="settings-import-zone" role="button" tabindex="0" aria-label="Select settings backup file to import">
      <p class="upload-prompt">Have a backup? <strong>Import settings</strong></p>
      <p class="upload-hint">Select a flightwall-settings.json file</p>
      <input type="file" id="import-file-input" accept=".json" hidden>
    </div>
    ```
  - [ ] The import zone should be compact — 2 lines of text max, dashed border, same visual pattern as `.logo-upload-zone` but more minimal
  - [ ] Add a hidden success indicator `<p id="import-status" class="import-status" style="display:none"></p>` below the import zone to show "12 settings imported" after successful import

- [ ] **Task 2: Add import zone CSS to style.css** (AC: #1)
  - [ ] `.settings-import-zone` — Reuse pattern from `.logo-upload-zone`: dashed border `2px dashed var(--border)`, padding `12px`, border-radius `var(--radius)`, cursor pointer, text-align center, transition for border-color
  - [ ] `.settings-import-zone.drag-over` — Solid border `var(--primary)`, background `rgba(88,166,255,0.08)`
  - [ ] `.import-status` — font-size `0.8125rem`, color `var(--success)`, text-align center, margin-top `8px`
  - [ ] Target: ~15 lines of CSS, minimal footprint
  - [ ] All interactive elements meet 44x44px minimum touch targets
  - [ ] Works at 320px minimum viewport width

- [ ] **Task 3: Implement settings import logic in wizard.js** (AC: #2, #3, #4, #5, #6, #7, #8)
  - [ ] Add DOM references for import elements: `settings-import-zone`, `import-file-input`, `import-status`
  - [ ] Add `importedExtras` object (initially empty `{}`) to hold non-wizard keys from import
  - [ ] Define `WIZARD_KEYS` constant: the 12 keys that map to wizard form fields:
    ```javascript
    var WIZARD_KEYS = [
      'wifi_ssid', 'wifi_password',
      'os_client_id', 'os_client_sec', 'aeroapi_key',
      'center_lat', 'center_lon', 'radius_km',
      'tiles_x', 'tiles_y', 'tile_pixels', 'display_pin'
    ];
    ```
  - [ ] Define `KNOWN_EXTRA_KEYS` constant: non-wizard config keys that should be preserved:
    ```javascript
    var KNOWN_EXTRA_KEYS = [
      'brightness', 'text_color_r', 'text_color_g', 'text_color_b',
      'origin_corner', 'scan_dir', 'zigzag',
      'zone_logo_pct', 'zone_split_pct', 'zone_layout',
      'fetch_interval', 'display_cycle',
      'timezone', 'sched_enabled', 'sched_dim_start', 'sched_dim_end', 'sched_dim_brt'
    ];
    ```
  - [ ] **Click/keyboard handler**: click on `.settings-import-zone` or Enter/Space key triggers hidden `#import-file-input` click
  - [ ] **File input change handler**: when a file is selected, read via `FileReader.readAsText()`, then call `processImportedSettings(text)`
  - [ ] **`processImportedSettings(text)` function**:
    1. Try `JSON.parse(text)` — on failure: `FW.showToast('Could not read settings file — invalid format', 'error')`, return
    2. Check parsed object is a plain object (not array/null) — on failure: same error toast, return
    3. Iterate keys — for each key in `WIZARD_KEYS`, if present in parsed object, assign `state[key] = String(parsed[key])`
    4. Iterate keys — for each key in `KNOWN_EXTRA_KEYS`, if present in parsed object, assign `importedExtras[key] = parsed[key]`
    5. Count total recognized keys (wizard + extras)
    6. If count === 0: `FW.showToast('No recognized settings found in file', 'warning')`, return
    7. If count > 0: `FW.showToast('Imported ' + count + ' settings', 'success')`
    8. Show import status indicator with count
    9. Rehydrate current step's form fields from updated state (call `showStep(currentStep)`)
  - [ ] **Modify `saveAndConnect()` payload**: merge `importedExtras` into the payload object before POSTing:
    ```javascript
    // After building the existing 12-key payload:
    var key;
    for (key in importedExtras) {
      if (importedExtras.hasOwnProperty(key)) {
        payload[key] = importedExtras[key];
      }
    }
    ```
  - [ ] **Guard against re-import**: if the user selects a new file after already importing, reset `importedExtras = {}` and re-process

- [ ] **Task 4: Add drag-and-drop support for import zone** (AC: #1)
  - [ ] `dragenter`/`dragover` on zone — add `.drag-over` class, prevent default
  - [ ] `dragleave` — remove `.drag-over` class (use `contains(e.relatedTarget)` check to avoid child-element flicker)
  - [ ] `drop` — remove `.drag-over` class, extract `e.dataTransfer.files[0]`, read via FileReader
  - [ ] Pattern matches the existing logo upload and OTA upload drag-and-drop implementations

- [ ] **Task 5: Gzip updated web assets** (AC: #9)
  - [ ] From `firmware/` directory, regenerate gzipped assets:
    ```
    gzip -9 -c data-src/wizard.html > data/wizard.html.gz
    gzip -9 -c data-src/wizard.js > data/wizard.js.gz
    gzip -9 -c data-src/style.css > data/style.css.gz
    ```
  - [ ] Verify gzipped files replaced in `firmware/data/`

- [ ] **Task 6: Build and verify** (AC: #1-#9)
  - [ ] Run `~/.platformio/penv/bin/pio run` from `firmware/` — verify clean build
  - [ ] Verify binary size remains under 1.5MB limit
  - [ ] Measure total gzipped web asset size (should remain well under 50KB budget)

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

Claude Opus 4.6 (Story Creation)

### Debug Log References

N/A

### Completion Notes List

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

### File List

_(To be populated during implementation)_


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

# 🎯 Story Context Validation Report

**Story:** fn-1-7-settings-import-in-setup-wizard - settings-import-in-setup-wizard
**Story File:** _bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md
**Validated:** 2026-04-12
**Validator:** Quality Competition Engine

---

## Executive Summary

### Issues Overview

| Category | Found | Applied |
|----------|-------|---------|
| 🚨 Critical Issues | 0 | 0 |
| ⚡ Enhancements | 1 | 0 |
| ✨ Optimizations | 1 | 0 |
| 🤖 LLM Optimizations | 0 | 0 |

**Overall Assessment:** The story is exceptionally well-defined, providing clear, detailed, and actionable instructions. It demonstrates strong architectural alignment and reuses existing patterns effectively, significantly mitigating disaster risks. Minor enhancements for user experience and maintenance are identified, but the core specification is robust.

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Improved User Control over Import UI Visibility | Enhancement 1 | +1 |
| 🟠 IMPORTANT | Dynamically Generate `KNOWN_EXTRA_KEYS` from Server Schema | Optimization 1 | +1 |
| 🟡 MINOR | Story is prescriptive in implementation details | INVEST Negotiation | +0.3 |
| 🟢 CLEAN PASS | 7 | | -3.5 |

### Evidence Score: -1.2

| Score | Verdict |
|-------|---------|
| **-1.2** | **PASS** |

---

## 🎯 Ruthless Story Validation fn-1.7

### INVEST Criteria Assessment

| Criterion | Status | Severity | Details |
|-----------|--------|----------|---------|
| **I**ndependent | ✅ Pass | 0/10 | The story's dependencies on fn-1.5 and fn-1.6 are explicitly stated as complete, and no future dependencies are introduced. |
| **N**egotiable | ⚠️ Minor | 3/10 | The story is quite prescriptive regarding implementation details (e.g., specific JavaScript constant names, exact merging logic for payload, specific toast messages). While beneficial for LLM agents, it reduces negotiability for a human developer. |
| **V**aluable | ✅ Pass | 0/10 | The story clearly states the value proposition for the user: "quickly reconfigure my device after a partition migration without retyping API keys and coordinates." |
| **E**stimable | ✅ Pass | 0/10 | The scope is well-defined, dependencies are clear, and implementation details are granular, making the story highly estimable. |
| **S**mall | ✅ Pass | 0/10 | The story involves client-side modifications to a few web assets and is a self-contained feature, appropriately sized for a single sprint. |
| **T**estable | ✅ Pass | 0/10 | All Acceptance Criteria are specific, measurable, and include various success, error, and edge-case scenarios, supported by a comprehensive testing strategy. |

### INVEST Violations

- **[3/10] Negotiable:** The story is highly prescriptive about specific implementation choices (e.g., variable names, explicit JSON parsing steps, payload merging logic). While useful for LLM execution, it somewhat reduces the "negotiable" aspect for a human developer who might have alternative, equally valid approaches.

### Acceptance Criteria Issues

✅ Acceptance criteria are well-defined and testable.

### Hidden Risks and Dependencies

✅ No hidden dependencies or blockers identified.

### Estimation Reality-Check

**Assessment:** Realistic

The story's scope is clearly defined as client-side only, leveraging existing server-side endpoints and UI patterns. The detailed tasks, explicit dependencies, and comprehensive risk mitigation plan provide a solid foundation for a realistic estimate. The estimated increase in `wizard.js` file size is also within acceptable limits.

### Technical Alignment

**Status:** ✅ Aligned

✅ Story aligns with architecture.md patterns.

---

## 🚨 Critical Issues (Must Fix)

✅ No critical issues found - the original story covered essential requirements.

---

## ⚡ Enhancement Opportunities (Should Add)

### 1. Improved User Control over Import UI Visibility

**Benefit:** Enhances user experience by allowing easy dismissal/hiding of the import UI if they switch to manual setup after an import attempt, making AC 7's "no interference" even more robust.
**Source:** Acceptance Criteria 7, Task 1

**Current Gap:**
The story provides no explicit mechanism for a user to hide or clear the import zone/status once a file is selected or imported, if they then decide to manually configure WiFi or change their mind. While the current setup ensures the WiFi scan runs "below" the import zone, offering a way to dismiss the import UI provides more granular user control and reduces potential visual clutter for users who opt for manual entry.

**Suggested Addition:**
After a successful import, or even after a file selection, provide a small "Clear" or "Dismiss" button/icon within or adjacent to the import zone (or as part of a success message). This would reset the UI to its initial state (showing "Import settings" prompt) and clear any `importedExtras`, allowing the user to seamlessly switch to manual entry without the import UI remaining prominent. This aligns with good UX for optional features.

---

## ✨ Optimizations (Nice to Have)

### 1. Dynamically Generate `KNOWN_EXTRA_KEYS` from Server Schema

**Value:** Improves maintainability, reduces the risk of bugs due to out-of-sync key lists between client and server, and makes the client more resilient to future `ConfigManager` changes.

**Suggestion:**
Instead of hardcoding `KNOWN_EXTRA_KEYS` in `wizard.js`, have the `wizard.js` client make an initial `GET /api/settings/export` request (or a similar `/api/schema` endpoint if one is created) to dynamically retrieve the full list of recognized configuration keys. Then, subtract the `WIZARD_KEYS` from this full list to derive `KNOWN_EXTRA_KEYS` at runtime. This ensures the client's key filtering is always in sync with the server's `ConfigManager`, reducing manual maintenance overhead and potential for errors when the firmware's `ConfigManager` schema evolves. The story acknowledges this as a risk in "Risk Mitigation 1", and this optimization directly addresses it.

---

## 🤖 LLM Optimization Improvements

✅ Story content is well-optimized for LLM processing.

---

## 🏆 Competition Results

### Quality Metrics

| Metric | Score |
|--------|-------|
| Requirements Coverage | 100% |
| Architecture Alignment | 100% |
| Previous Story Integration | 100% |
| LLM Optimization Score | 100% |
| **Overall Quality Score** | **95%** |

### Disaster Prevention Assessment

- **Reinvention Prevention:** ✅ Pass - Explicitly reuses existing patterns and clarified client-side scope.
- **Technical Specification:** ✅ Pass - API contracts are clear, and potential issues (e.g., unknown keys in POST payload) are explicitly mitigated.
- **File Structure:** ✅ Pass - Adheres to established `firmware/data-src/` and `firmware/data/` structure.
- **Regression:** ✅ Pass - Demonstrates awareness of existing wizard behavior and outlines testing to prevent regressions.
- **Implementation:** ✅ Pass - Highly detailed tasks and explicit instructions prevent vagueness and ensure comprehensive completion.

### Competition Outcome

✅ **Original create-story produced high-quality output** with minimal gaps identified. The story is exceptionally robust and ready for development. The validator found minor opportunities for UX enhancement and code maintainability, but no critical flaws.

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
<var name="session_id">7fdc2943-0f5f-4105-ac55-439e33deef36</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="ff10d1c0">embedded in prompt, file id: ff10d1c0</var>
<var name="story_id">fn-1.7</var>
<var name="story_key">fn-1-7-settings-import-in-setup-wizard</var>
<var name="story_num">7</var>
<var name="story_title">7-settings-import-in-setup-wizard</var>
<var name="template">False</var>
<var name="timestamp">20260412_2120</var>
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