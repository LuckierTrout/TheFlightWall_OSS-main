<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-3 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T224405Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-3.2

You are synthesizing 4 independent code review findings.

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
<file id="1dffb869" path="_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md" label="DOCUMENTATION"><![CDATA[

# Story fn-3.2: Wizard Timezone Auto-Detect

Status: ready-for-dev

## Story

As a new user completing the setup wizard,
I want the wizard to auto-detect my timezone from my browser and let me confirm or change it,
So that my FlightWall displays correct local times for flights without manual POSIX string entry.

## Acceptance Criteria

1. **Auto-Detect at Wizard Init:**
   - Given the wizard page loads for the first time (IIFE initialization)
   - When the IIFE init section runs
   - Then the browser's timezone is detected via `Intl.DateTimeFormat().resolvedOptions().timeZone`
   - And if the detected IANA timezone exists in `TZ_MAP`, it is stored in `state.timezone`
   - And if `state.timezone` is empty (no prior value from hydration or import), the auto-detected IANA name is applied
   - And if `state.timezone` already holds a value (from `hydrateDefaults()`, import, or a prior session), it is preserved — auto-detect does NOT overwrite

   **Step 3 Navigation:**
   - Given the user navigates to Step 3 (Location) at any point during the wizard
   - When `showStep(3)` is called (initial entry or return from another step)
   - Then the timezone dropdown displays the current value of `state.timezone`
   - And no new auto-detection occurs on step entry — the dropdown reflects state as established at init

2. **Timezone Dropdown:**
   - Given Step 3 is displayed
   - When rendered
   - Then a `<select>` dropdown is shown below the Radius field, labeled "Timezone"
   - And the dropdown is populated with all IANA timezone names from `TZ_MAP` keys (sorted alphabetically)
   - And each option displays the IANA name (e.g., "America/Los_Angeles")
   - And the dropdown uses the existing `.cal-select` CSS class for consistent styling

3. **Fallback for Unknown Timezones:**
   - Given the browser reports an IANA timezone not present in `TZ_MAP`
   - When auto-detection runs
   - Then the dropdown defaults to "UTC" as a safe fallback
   - And the auto-detected (unmapped) IANA name is NOT shown in the dropdown (no phantom entries)

4. **POSIX String Sent to Device:**
   - Given the user completes the wizard and settings are saved
   - When `saveAndConnect()` builds the payload
   - Then `payload.timezone` is set to the POSIX string from `TZ_MAP[selectedIANA]` (e.g., "PST8PDT,M3.2.0,M11.1.0")
   - And the POSIX string (not the IANA name) is what gets stored in NVS via `POST /api/settings`

5. **Review Step Shows Timezone:**
   - Given the user reaches Step 5 (Review)
   - When `buildReview()` renders the Location section
   - Then a "Timezone" row is displayed showing the selected IANA timezone name
   - And clicking the Location review card navigates back to Step 3 where the timezone dropdown is editable

6. **Settings Import Compatibility:**
   - Given the user imports settings that include a `timezone` POSIX value
   - When the import is processed
   - Then the POSIX string is reverse-looked up to its IANA name via `POSIX_TO_IANA` and stored in `state.timezone`
   - And if no reverse match exists, `state.timezone` retains the auto-detected value
   - And the `timezone` key is a first-class wizard field handled through the `WIZARD_KEYS` import path (not `importedExtras`)
   - And the user may then adjust the dropdown; the wizard form value takes precedence in the final payload

7. **Asset Budget:**
   - Given updated wizard web assets (wizard.html, wizard.js)
   - When gzipped and placed in `firmware/data/`
   - Then total gzipped web asset size remains under 50KB budget

## Tasks / Subtasks

- [ ] Task 1: Add timezone dropdown HTML to Step 3 in wizard.html (AC: #2)
  - [ ] 1.1: Add `<label for="wizard-timezone">Timezone</label>` after the radius field and before `<p id="loc-err">` in the Step 3 `<div class="form-fields">`
  - [ ] 1.2: Add `<select id="wizard-timezone" class="cal-select"></select>` — empty options, populated by JS at init

- [ ] Task 2: Wire timezone into wizard.js state and WIZARD_KEYS (AC: #1, #4, #6)
  - [ ] 2.1: Add `timezone: ''` to the `state` object (line ~108, after `radius_km`)
  - [ ] 2.2: Add `'timezone'` to the `WIZARD_KEYS` array (line ~130, after `'radius_km'`)
  - [ ] 2.3: Remove `'timezone'` from `KNOWN_EXTRA_KEYS` array (line ~143) — it is now a wizard key, not an extra. **Critical:** If timezone remains in both `WIZARD_KEYS` and `KNOWN_EXTRA_KEYS`, an imported timezone could override the user's wizard selection via `importedExtras` merge in `saveAndConnect()`
  - [ ] 2.4: Update `hydrateDefaults()` — after the existing hardware key assignments (line ~218), add timezone reverse-lookup from device settings:
    ```js
    if (d.timezone) {
      state.timezone = POSIX_TO_IANA[d.timezone] || getTimezoneConfig().iana;
    }
    ```
    `d.timezone` is the POSIX string returned by `GET /api/settings`. The `POSIX_TO_IANA` map is built in Task 3.1. This ensures that if the user reloads the wizard with an existing device timezone, the dropdown shows the correct IANA name rather than the raw POSIX string.
    **Ordering note:** `hydrateDefaults()` runs before IIFE dropdown population and before auto-detect (Task 3). When it sets `state.timezone`, Task 3.3 will preserve that value (because `state.timezone` will be non-empty) and simply reflect it in the dropdown.

- [ ] Task 3: Populate timezone dropdown and auto-detect on init (AC: #1, #2, #3)
  - [ ] 3.1: In the IIFE init section (DOM references area, line ~174), declare the element and build the shared reverse map — both are used by import handling (Task 6) and hydration (Task 2.4):
    ```js
    var wizardTimezone = document.getElementById('wizard-timezone');
    var POSIX_TO_IANA = {};
    Object.keys(TZ_MAP).forEach(function(iana) {
      if (!POSIX_TO_IANA[TZ_MAP[iana]]) POSIX_TO_IANA[TZ_MAP[iana]] = iana;
    });
    ```
    `POSIX_TO_IANA` is used in Task 2.4 (`hydrateDefaults`) and Task 6.1 (import). Building it once here avoids rebuilding it on every import.
  - [ ] 3.2: Populate the dropdown from `TZ_MAP` keys sorted alphabetically:
    ```js
    var tzKeys = Object.keys(TZ_MAP).sort();
    tzKeys.forEach(function(iana) {
      var opt = document.createElement('option');
      opt.value = iana;
      opt.textContent = iana;
      wizardTimezone.appendChild(opt);
    });
    ```
  - [ ] 3.3: Auto-detect browser timezone and set initial state:
    ```js
    var detected = getTimezoneConfig();
    // Only auto-fill if state.timezone is empty (no prior selection or import)
    if (!state.timezone) {
      state.timezone = detected.iana;
    }
    wizardTimezone.value = state.timezone;
    ```
    Note: `getTimezoneConfig()` already exists at wizard.js lines 92-99 and handles the TZ_MAP lookup + UTC fallback.

- [ ] Task 4: Wire timezone into step navigation lifecycle (AC: #1, #5)
  - [ ] 4.1: Update `saveCurrentStepState()` — in the `currentStep === 3` block (line ~511), add:
    ```js
    state.timezone = wizardTimezone.value;
    ```
  - [ ] 4.2: Update `showStep(3)` rehydration block (line ~476) — add after `radiusKm.value`:
    ```js
    wizardTimezone.value = state.timezone || getTimezoneConfig().iana;
    ```
  - [ ] 4.3: Update `buildReview()` — add a Timezone item to the Location section (line ~395):
    ```js
    { label: 'Timezone', value: state.timezone || 'UTC' }
    ```

- [ ] Task 5: Include timezone POSIX value in save payload (AC: #4)
  - [ ] 5.1: In `saveAndConnect()` payload construction (line ~630), add after `radius_km`:
    ```js
    timezone: TZ_MAP[state.timezone] || 'UTC0',
    ```
    This converts the IANA name stored in `state.timezone` to the POSIX string the firmware expects.

- [ ] Task 6: Handle settings import timezone hydration (AC: #6)
  - [ ] 6.1: In `processImportedSettings()`, handle `timezone` as a **special case** within the `WIZARD_KEYS` import loop. Because imported settings files contain the POSIX string (e.g., `"PST8PDT,M3.2.0,M11.1.0"`) but `state.timezone` must hold an IANA name, the generic `state[key] = String(parsed[key])` assignment must NOT apply to `timezone`. Instead, use the `POSIX_TO_IANA` reverse map built in Task 3.1:
    ```js
    // In the WIZARD_KEYS import loop:
    if (key === 'timezone') {
      if (parsed.timezone) {
        state.timezone = POSIX_TO_IANA[parsed.timezone] || getTimezoneConfig().iana;
      }
      // If parsed.timezone is absent, leave state.timezone at its current value (auto-detected or hydrated)
    } else if (parsed[key] !== undefined) {
      state[key] = String(parsed[key]);
    }
    ```
    **Why this matters:** If the generic assignment runs, `state.timezone` will hold a POSIX string. The dropdown (`#wizard-timezone`) uses IANA names as option values, so `wizardTimezone.value = "PST8PDT,..."` will find no matching option and appear blank. On save, `TZ_MAP["PST8PDT,..."]` returns `undefined`, causing the payload to send `'UTC0'` and silently reset the device timezone.
    Note: Multiple IANA zones map to the same POSIX string (e.g., America/New_York and America/Toronto both map to EST5EDT). The reverse lookup picks the first alphabetically, which is acceptable for display purposes.

- [ ] Task 7: Update E2E test page objects and mock server (AC: #1, #2)
  - [ ] 7.1: In `tests/e2e/page-objects/WizardPage.ts`, add locator: `timezoneSelect = this.page.locator('#wizard-timezone');`
  - [ ] 7.2: Add helper method to `WizardPage`:
    ```ts
    async selectTimezone(iana: string) {
      await this.timezoneSelect.selectOption(iana);
    }
    ```
  - [ ] 7.3: Update `configureLocation()` method to optionally accept and set timezone
  - [ ] 7.4: In `tests/e2e/mock-server/server.ts`, add `timezone: 'UTC0'` to the default settings state if not already present

- [ ] Task 8: Rebuild gzipped assets (AC: #7)
  - [ ] 8.1: `gzip -9 -c firmware/data-src/wizard.html > firmware/data/wizard.html.gz`
  - [ ] 8.2: `gzip -9 -c firmware/data-src/wizard.js > firmware/data/wizard.js.gz`
  - [ ] 8.3: Verify total gzipped asset size remains under 50KB

## Dev Notes

### Critical Architecture Constraints

- **Architecture Decision F4:** Browser-side IANA-to-POSIX mapping via JS table. Zero firmware overhead — the `TZ_MAP` object in wizard.js (lines 7-86) already contains 60+ entries. The firmware only sees the resulting POSIX string via `POST /api/settings`. [Source: architecture.md#Decision-F4]
- **NVS key `timezone`:** String type, default "UTC0", max 40 chars, stored in ScheduleConfig struct. Hot-reload — no reboot required. The firmware calls `configTzTime(tz.c_str(), "pool.ntp.org", "time.nist.gov")` immediately on change. [Source: architecture.md#NVS-Key-Table]
- **Enforcement Rule 10:** Every JS `fetch()` call must check `json.ok` and handle failure with `FW.showToast()`. This story adds no new API calls — timezone is included in the existing `saveAndConnect()` payload. [Source: architecture.md#Enforcement-Guidelines]
- **Web asset budget:** Total gzipped assets are at ~49.22KB of 50KB budget. This story adds ~5 lines of HTML and ~20 lines of JS. The TZ_MAP already exists and is already gzipped. Expected impact: <0.1KB additional gzipped. [Source: fn-3.1 dev notes]
- **Gzip build process:** No automated script. After editing `data-src/` files, manually run `gzip -9 -c data-src/X > data/X.gz` from `firmware/` directory. [Source: MEMORY.md]

### Existing Code to Reuse (DO NOT Reinvent)

**`TZ_MAP` (wizard.js lines 7-86):** Already fully implemented with 60+ IANA-to-POSIX entries. DO NOT create a new mapping table. Reference this object directly.

**`getTimezoneConfig()` (wizard.js lines 92-99):** Already implemented — detects browser IANA timezone via `Intl.DateTimeFormat().resolvedOptions().timeZone`, looks up POSIX string in `TZ_MAP`, returns `{ iana, posix }`. Currently dead code relative to the wizard UI. This story wires it in. DO NOT rewrite this function.

**Dashboard timezone dropdown (dashboard.js lines 1042-1076):** Reference implementation for how to populate a `<select>` from `TZ_MAP` keys. Uses `Object.keys(TZ_MAP).sort()`, creates `<option>` elements. The wizard implementation should follow this same pattern. The dashboard uses `#nm-timezone` with `.cal-select` class — the wizard uses `#wizard-timezone` with the same class.

**`KNOWN_EXTRA_KEYS` (wizard.js line 139-144):** Currently includes `'timezone'`. This story MOVES timezone from extras to a first-class wizard key. Failure to remove it from `KNOWN_EXTRA_KEYS` would cause the imported POSIX value to overwrite the user's IANA-derived POSIX value in `saveAndConnect()` payload merge.

**`POSIX_TO_IANA` reverse map (built in Task 3.1):** A one-time reverse lookup of `TZ_MAP` built at IIFE init. Used by both `hydrateDefaults()` (Task 2.4) and `processImportedSettings()` (Task 6.1). Since `TZ_MAP` is not one-to-one (multiple IANA zones share a POSIX string, e.g., America/New_York and America/Toronto both map to `"EST5EDT,..."`), `POSIX_TO_IANA` takes the first IANA name encountered — build it with `if (!POSIX_TO_IANA[posix])` to get the alphabetically-first match. This is acceptable for display purposes.

### State Flow

Three paths populate `state.timezone` at IIFE init — all must produce an IANA name, never a POSIX string:

```
Path A (fresh wizard):
Browser auto-detect → getTimezoneConfig() → { iana: "America/Los_Angeles", posix: "PST8PDT,..." }
  → state.timezone = "America/Los_Angeles"  (only if state.timezone is empty)

Path B (returning user / page reload):
GET /api/settings → d.timezone = "PST8PDT,M3.2.0,M11.1.0"  (POSIX)
  → hydrateDefaults() → POSIX_TO_IANA["PST8PDT,..."] → "America/Los_Angeles"
  → state.timezone = "America/Los_Angeles"  (auto-detect skipped — state is non-empty)

Path C (settings import):
Imported JSON → parsed.timezone = "PST8PDT,M3.2.0,M11.1.0"  (POSIX)
  → processImportedSettings() → POSIX_TO_IANA["PST8PDT,..."] → "America/Los_Angeles"
  → state.timezone = "America/Los_Angeles"

All paths converge:
state.timezone = "America/Los_Angeles"  ← IANA name always stored in state
                        ↓
Dropdown shows IANA name ← user can change selection
                        ↓
saveAndConnect() → payload.timezone = TZ_MAP[state.timezone] → "PST8PDT,M3.2.0,M11.1.0"
                        ↓
POST /api/settings → firmware stores POSIX string in NVS → configTzTime() applies it
```

### Key Difference from Dashboard

The **dashboard** stores the IANA name in the dropdown value and converts to POSIX on save. The **wizard** must do the same: `state.timezone` holds the IANA name (for dropdown display and review), and `TZ_MAP[state.timezone]` produces the POSIX string only at save time. Do NOT store the POSIX string in `state.timezone` — that would make the dropdown and review display unreadable.

### Files Modified

| File | Change |
|------|--------|
| `firmware/data-src/wizard.html` | Add timezone `<select>` to Step 3 |
| `firmware/data-src/wizard.js` | Add `timezone` to state/WIZARD_KEYS, remove from KNOWN_EXTRA_KEYS, populate dropdown, wire auto-detect, add to review, add to payload |
| `firmware/data/wizard.html.gz` | Rebuild gzip |
| `firmware/data/wizard.js.gz` | Rebuild gzip |
| `tests/e2e/page-objects/WizardPage.ts` | Add timezone locator and helper |
| `tests/e2e/mock-server/server.ts` | Add timezone to default state |

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-fn-3.md` → Story fn-3.2
- PRD Requirements: FR17 (auto-detect timezone), FR18 (timezone config)
- Architecture: Decision F4 (browser-side IANA-to-POSIX mapping)
- UX Design Rule: UX-DR11 (timezone pre-fill from browser)
- Previous Story: fn-3.1 (wizard Step 6 — Test Your Wall) — established Step 6 as final step, TOTAL_STEPS=6


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic fn-3 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story fn-3-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Stale deployable gz assets (`wizard.js.gz` with `TOTAL_STEPS=5`, no Step 6) | Rebuilt all three from current `data-src/` sources; verified `TOTAL_STEPS=6` and `cachedDeviceColors` present in decompressed output. |
| critical | `saveAndConnect()` error path appends error to hidden Step 5 container, re-enables hidden nav btn, leaves Step 6 buttons hidden → user UI-locked with no recovery | `showSaveError()` now inserts error adjacent to `wizardTestStatus` when `currentStep === 6`; error catch block restores `btnTestYes`/`btnTestNo` visibility and enables them; `saveAndConnect()` shows "Your FlightWall is ready!" on successful reboot response before calling `showHandoff()`. |
| high | `enterStep6()` fires `POST /api/positioning/start` asynchronously and stays on Step 6 regardless of outcome; AC1 requires returning to Step 5 on failure | Both `.then` (non-ok) and `.catch` branches now call `showStep(5)` after the error toast. |
| high | `renderPositionCanvas()` draws a blue cable-path wiring diagram; firmware draws colored tile fills + bright white borders + red corner marker on tile 0 + centered digit numbers → user cannot compare browser to wall | Complete rewrite of `renderPositionCanvas()` — per-tile HSL fills (unique hue per `idx * 360 / totalTiles`), 1.5px white border per tile, red `fillRect` corner marker on tile 0, white centered digit label. Matches `NeoMatrixDisplay::renderPositioningPattern()`. |
| high | `runRgbTest()` reads original brightness/color from `importedExtras` only; in a normal session `importedExtras={}` so restores silently revert to hardcoded 40/255/255/255 | Added `cachedDeviceColors` variable populated in `hydrateDefaults()` from `/api/settings` response. `runRgbTest()` now resolves: `importedExtras` → `cachedDeviceColors` → safe fallback. |
| high | RGB test error paths fire restore POST then immediately call `finishStep6()` without awaiting; `finishStep6()` → `saveAndConnect()` fires another POST to `/api/settings` — race condition | Extracted `restoreAndFinish()` helper that chains `finishStep6()` inside `.then`/`.catch` of the restore POST, guaranteeing serial execution. |
| high | `finishStep6()` sets "Your FlightWall is ready!" then `saveAndConnect()` immediately overwrites it with "Saving..." → AC5 completion message never persists | `finishStep6()` now sets "Saving..."; `saveAndConnect()` no longer sets status text (removed the override); "Your FlightWall is ready!" is set in the reboot success handler just before `showHandoff()`. |
| medium | Canvas `drawHeight` has no upper bound; a 1×10 tile config yields drawHeight ≈ 4800px | Added `MAX_CANVAS_HEIGHT = 400` cap; `drawWidth` is scaled down proportionally if height exceeds cap. |
| medium | `WizardPage.ts` describes 5-step flow; `getCurrentStep()` iterates only to 5; `completeWizard()` clicks `#btn-next` on Step 5 (now a pass-through) instead of `#btn-test-yes` | Updated file header comment, `WizardStep` type (added `'test'`), `getCurrentStep()` loop (1→6), `nextButton` comment, added Step 6 locators (`testYesButton`, `testNoButton`, `testStatusText`, `testPositionCanvas`), rewrote `completeWizard()` to click `#btn-test-yes`, added Step 5 `goNext()` to `completeSetup()`. |
| medium | Mock server has no `/api/positioning/start` or `/api/positioning/stop` routes → any E2E test advancing through Step 6 receives 404 | Added both routes returning `{ ok: true }`. |
| low | Stale comment `// Keys that map to wizard form fields (12 keys)` — array now holds 15 | Updated to "15 keys". |
| dismissed | `TZ_MAP` / `getTimezoneConfig()` are dead code in wizard.js, violating "Foundation Architecture" | FALSE POSITIVE: These symbols were introduced by Story fn-2.1 (per file comment: "Story fn-2.1. Shared with dashboard.js — used for settings import timezone resolution."). They predate fn-3.1 and are not a regression introduced by this story. Project-context scope rules exclude pre-existing issues from the reviewed story's verdict. Carry forward as a fn-cleanup candidate. |
| dismissed | Touch targets fail WCAG 44×44px because `.wizard-test-actions .nav-btn` only sets `min-height:44px` not `min-width:44px` | FALSE POSITIVE: The buttons use `width:100%` (inherited from `.wizard-test-actions .nav-btn { width:100% }`), so rendered width is always the full card width (minimum 320px on the minimum supported viewport). Touch target width is satisfied by layout, not an explicit `min-width` declaration. The `min-height:44px` ensures the vertical dimension. This is a false positive. --- |

## Story fn-3-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | Canvas `width:100%` with no `max-height` causes unbounded vertical stretching for extreme tile aspect ratios (e.g., 1×10 yields a 400px-wide × ~4800px-tall rendered canvas) | Added `max-height:70vh;object-fit:contain` to `.wizard-test-canvas canvas` |
| low | `saveAndConnect()` sets `btnNext.textContent = 'Rebooting...'` on the hidden nav element; `wizardTestStatus` remains stuck on "Saving…" for the entire reboot-wait period with no visual update | Added `if (currentStep === 6 && wizardTestStatus) wizardTestStatus.textContent = 'Rebooting...'` immediately after `btnNext.textContent = 'Rebooting...'` |
| low | `backButton` locator uses `/back | Changed to `this.page.locator('#btn-back')` (exact ID) |
| low | `previewCanvas` locator uses `#preview-canvas, canvas` (matches any canvas); the wizard has only one canvas element (`#wizard-position-preview`) so the fallback is overly broad | Changed to `this.page.locator('#wizard-position-preview')` |
| dismissed | Race condition in `enterStep6()` — "async callback uses stale `currentStep`; should capture step into a variable" | FALSE POSITIVE: The current `if (currentStep === 6)` check is *correct*. If the user clicks "No — take me back" before the `/api/positioning/start` callback fires, `currentStep` becomes 4, and the guard correctly suppresses the Step 5 redirect. Reviewer A's suggested fix (capturing step into `enterStep`) would check `enterStep === 6` (always true at callback time), causing the back-navigation to incorrectly overwrite the user's already-completed "No" action with a forced Step 5 jump. |
| dismissed | Red corner marker should render on tile 0 only; current canvas renders it on every tile | FALSE POSITIVE: Directly contradicted by firmware source. `NeoMatrixDisplay::renderPositioningPattern()` lines 747-752 are **inside** the nested `for (row) / for (col)` loop — the red corner marker is drawn on *every* tile. The antipatterns document that claimed "tile 0 only" was itself incorrect. The canvas implementation is correct. |
| dismissed | Canvas preview ignores `origin_corner`, `scan_dir`, `zigzag` — browser disagrees with wall for non-default wiring | FALSE POSITIVE: Both firmware and canvas use `gridIdx = row * tilesX + col` (visual row-major order). The NeoMatrix library handles the physical LED strand remapping internally per wiring flags. Tile *numbers* are always assigned in visual order; only the physical pixel addressing changes. The canvas correctly matches the firmware's visual numbering. |
| dismissed | `saveAndConnect()` SOLID violation — does too many things; should split into `buildSavePayload()`, `handleReboot()`, etc. | FALSE POSITIVE: Pre-existing function pattern that predates fn-3.1. Architectural refactoring is out of scope for a code review targeting this story's delta. |
| dismissed | CORS wildcard `Access-Control-Allow-Origin: '*'` in mock server is a security vulnerability | FALSE POSITIVE: The mock server is a **local test-only tool** (`tests/e2e/mock-server/server.ts`), never shipped on the device. `*` is standard for local dev servers. Not a defect in product code. |
| dismissed | No CSRF protection on `/api/settings` | FALSE POSITIVE: The device operates exclusively in AP mode (isolated WiFi access point, no internet routing). Cross-origin requests are physically impossible in this network topology. Not applicable. |
| dismissed | RGB test produces tinted text instead of solid-color fill — AC4 requires "solid red → solid green → solid blue" | FALSE POSITIVE: Explicitly dismissed in the antipatterns document: "by design per Dev Notes; sufficient to verify each RGB channel independently; no firmware changes required per AC constraints." This dismissal pre-dates this synthesis pass. |
| dismissed | AC5 completion text "Your FlightWall is ready!" is never shown | FALSE POSITIVE: Partially incorrect. `showHandoff()` replaces the entire wizard DOM with content including `<p>Your FlightWall is ready! Fetching your first flights...</p>`. The AC5 copy IS shown — it is the handoff screen. The "Rebooting…" gap was the real defect (fixed above). |
| dismissed | `rebootRequested` flag makes post-reboot network loss appear as success | FALSE POSITIVE: This is intentional and documented in the catch handler: "Network loss after requesting reboot is expected because the device is leaving AP mode." The device drops the TCP connection when it leaves AP mode; treating that disconnect as successful handoff is correct behavior. |
| dismissed | No E2E test specs for Step 6 | FALSE POSITIVE: Explicitly deferred per Dev Agent Record: "Full E2E test cases for Step 6 not written in this pass — page-object infrastructure updated; dedicated test spec creation belongs in a separate test story." |
| dismissed | `validateStep(4)` doesn't validate `origin_corner`, `scan_dir`, `zigzag` | FALSE POSITIVE: These are `<select>` elements whose values are browser-constrained to the provided `<option>` values (0 or 1). No user input can produce an out-of-range value without DOM manipulation, and firmware validates all NVS writes. Low-risk gap that doesn't warrant validation code. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-3.2-wizard-timezone-auto-detect.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 2 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Missing timezone in saveAndConnect() payload - violates AC4 | wizard.js:645-661 | +3 |
| 🔴 CRITICAL | Missing timezone special case in import processing - violates AC6 | wizard.js:1007-1017 | +3 |
| 🟠 IMPORTANT | Missing timezone selector in E2E page object (Task 7.1-7.2) | WizardPage.ts | +1 |
| 🟠 IMPORTANT | Missing timezone in mock server default state (Task 7.4) | server.ts:79-101 | +1 |
| 🟡 MINOR | No explicit asset budget verification (Task 8.3) | N/A | +0.3 |
| 🟡 MINOR | No E2E test spec for wizard timezone | N/A | +0.3 |
| 🟢 CLEAN PASS | 6 categories with no issues | N/A | -3 |

### Evidence Score: 5.6

| Score | Verdict |
|-------|---------|
| **5.6** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** Timezone never sent to device in saveAndConnect()
  - 📍 `firmware/data-src/wizard.js:645-661`
  - 🔄 Reproduction: User completes wizard with timezone selected → saveAndConnect() builds payload → POST /api/settings → timezone field missing from payload → device timezone not saved → device uses default "UTC0" instead of user's selection
  - ⚠️ Impact: AC4 violated - POSIX string not sent to device, user's timezone selection is silently discarded

- **🐛 Bug:** Imported timezone (POSIX string) stored directly in state, breaking dropdown
  - 📍 `firmware/data-src/wizard.js:1007-1017`
  - 🔄 Reproduction: User imports settings file with timezone: "PST8PDT,M3.2.0,M11.1.0" → processImportedSettings() generic loop stores POSIX string in state.timezone → showStep(3) sets dropdown value to POSIX string → dropdown has no matching option → appears blank → user saves → TZ_MAP[POSIX_string] returns undefined → device gets "UTC0" fallback
  - ⚠️ Impact: AC6 violated - settings import breaks timezone functionality, user's imported timezone is lost

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Complex import handling with implicit type conversion
  - 📍 `firmware/data-src/wizard.js:1007-1027`
  - 💥 Explosion radius: Medium - affects all WIZARD_KEYS imports, special handling required for timezone to avoid state corruption

---

## 🛠️ Suggested Fixes

### 1. Add timezone to saveAndConnect() payload

**File:** `firmware/data-src/wizard.js`
**Issue:** AC4 violated - timezone never sent to device

**Corrected code:**
```javascript
var payload = {
  wifi_ssid: state.wifi_ssid,
  wifi_password: state.wifi_password,
  os_client_id: state.os_client_id,
  os_client_sec: state.os_client_sec,
  aeroapi_key: state.aeroapi_key,
  center_lat: Number(state.center_lat),
  center_lon: Number(state.center_lon),
  radius_km: Number(state.radius_km),
  timezone: TZ_MAP[state.timezone] || 'UTC0',
  tiles_x: Number(state.tiles_x),
  tiles_y: Number(state.tiles_y),
  tile_pixels: Number(state.tile_pixels),
  display_pin: Number(state.display_pin),
  origin_corner: Number(state.origin_corner),
  scan_dir: Number(state.scan_dir),
  zigzag: Number(state.zigzag)
};
```

### 2. Add timezone special case in import processing

**File:** `firmware/data-src/wizard.js`
**Issue:** AC6 violated - imported POSIX string not converted to IANA name

**Diff:**
```diff
     for (i = 0; i < WIZARD_KEYS.length; i++) {
       key = WIZARD_KEYS[i];
       if (Object.prototype.hasOwnProperty.call(parsed, key)) {
         val = parsed[key];
         // Skip null and non-primitive values to avoid "null" / "[object Object]" in state
         if (val !== null && typeof val !== 'object') {
+          // Special case: timezone needs POSIX → IANA conversion
+          if (key === 'timezone') {
+            if (parsed.timezone) {
+              state.timezone = POSIX_TO_IANA[parsed.timezone] || getTimezoneConfig().iana;
+            }
+          } else {
             state[key] = String(val);
+          }
           count++;
         }
       }
     }
```

### 3. Add timezone selector to E2E page object

**File:** `tests/e2e/page-objects/WizardPage.ts`
**Issue:** Task 7.1-7.2 NOT DONE

**Diff:**
```diff
  get radiusKmInput(): Locator {
    return this.page.locator('#radius-km, [name="radius_km"]');
  }

+  get timezoneSelect(): Locator {
+    return this.page.locator('#wizard-timezone');
+  }

   get useMyLocationButton(): Locator {
```

**Add helper method:**
```diff
  async configureLocation(
    lat: number,
    lon: number,
    radiusKm: number
  ): Promise<void> {
    await this.fillInput(this.centerLatInput, String(lat));
    await this.fillInput(this.centerLonInput, String(lon));
    await this.fillInput(this.radiusKmInput, String(radiusKm));
  }

+  async selectTimezone(iana: string): Promise<void> {
+    await this.timezoneSelect.selectOption(iana);
+  }
```

### 4. Add timezone to mock server default state

**File:** `tests/e2e/mock-server/server.ts`
**Issue:** Task 7.4 NOT DONE

**Diff:**
```diff
   interface DeviceState {
     settings: {
       brightness: number;
       text_color_r: number;
       text_color_g: number;
       text_color_b: number;
       fetch_interval: number;
       display_cycle: number;
       tiles_x: number;
       tiles_y: number;
       tile_pixels: number;
       display_pin: number;
       origin_corner: number;
       scan_dir: number;
       zigzag: number;
       center_lat: number;
       center_lon: number;
       radius_km: number;
       wifi_ssid: string;
+      timezone: string;
       logo_width_pct: number;
       flight_height_pct: number;
       layout_mode: number;
     };
```

```diff
   const state: DeviceState = {
     settings: {
       brightness: 40,
       text_color_r: 255,
       text_color_g: 255,
       text_color_b: 255,
       fetch_interval: 30,
       display_cycle: 5,
       tiles_x: 10,
       tiles_y: 2,
       tile_pixels: 16,
       display_pin: 25,
       origin_corner: 0,
       scan_dir: 0,
       zigzag: 0,
       center_lat: 37.7749,
       center_lon: -122.4194,
       radius_km: 10,
       wifi_ssid: 'MockNetwork',
+      timezone: 'UTC0',
       logo_width_pct: 0,
       flight_height_pct: 0,
       layout_mode: 0,
     },
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

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 4 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **12** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task 5.1 marked complete (implicitly) but `timezone` is missing from `saveAndConnect` payload. Settings will never be saved to device. | wizard.js:651 | +3 |
| 🔴 CRITICAL | Task 6.1 marked complete (implicitly) but `processImportedSettings` lacks POSIX-to-IANA conversion. Broken dropdown state after import. | wizard.js:995 | +3 |
| 🟠 IMPORTANT | Task 7 completely ignored. E2E test infrastructure (WizardPage.ts, server.ts) not updated for timezone. | WizardPage.ts | +1 |
| 🟠 IMPORTANT | AC #3 violation: No validation that auto-detected timezone exists in `TZ_MAP`. Leads to broken dropdown for travelers in unmapped zones. | wizard.js:1097 | +1 |
| 🟠 IMPORTANT | `mock-server/server.ts` state interface and initial state missing `timezone`. Breaks `hydrateDefaults` during local development. | server.ts:40 | +1 |
| 🟡 MINOR | Documentation gap: All tasks in story file are unchecked despite partial implementation. | fn-3.2.md | +0.3 |
| 🟡 MINOR | Inconsistent implementation: `hydrateDefaults` uses `POSIX_TO_IANA` but `processImportedSettings` does not. | wizard.js:218 | +0.3 |
| 🟡 MINOR | `POSIX_TO_IANA` built without sorting `TZ_MAP` keys. Reverse-lookup selection for shared POSIX strings is non-deterministic. | wizard.js:174 | +0.3 |

### Evidence Score: 9.4

| Score | Verdict |
|-------|---------|
| **9.4** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[4/10] DRY Violation:** `hydrateDefaults` and `processImportedSettings` duplicate logic for timezone hydration but do so inconsistently (one correctly converts, one doesn't).
  - 📍 `firmware/data-src/wizard.js:218` and `995`
  - 💡 Fix: Extract `resolveTimezone(posix)` helper.

---

## ⚡ Performance & Scalability

- **[Low] O(N) Map Creation:** `POSIX_TO_IANA` is built via `Object.keys(TZ_MAP).forEach`. While negligible for 60 entries, the lack of key sorting makes the "first alphabetical match" claim in the story false.
  - 📍 `firmware/data-src/wizard.js:174`
  - 💡 Fix: `Object.keys(TZ_MAP).sort().forEach(...)`

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `timezone` is omitted from the final payload in `saveAndConnect`. The user selects a timezone in Step 3, sees it in Step 5, but it is never sent to the firmware in Step 6.
  - 📍 `firmware/data-src/wizard.js:651-670`
  - 🔄 Reproduction: Complete wizard, check browser Network tab for `POST /api/settings` payload. `timezone` will be missing.
- **🐛 Bug:** Unmapped timezones from `Intl.DateTimeFormat()` are assigned directly to `state.timezone`. This results in an invalid `<select>` value that doesn't match any `<option>`, appearing blank to the user.
  - 📍 `firmware/data-src/wizard.js:1097`
  - 🔄 Reproduction: Mock `Intl` to return "Mars/Base_Alpha", load wizard. Dropdown is broken.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** E2E test infrastructure is now out of sync with the product. `WizardPage.ts` locators and actions do not support the new timezone field, and the mock server will strip `timezone` from incoming POSTs because it's not in the allowed settings state.
  - 📍 `tests/e2e/page-objects/WizardPage.ts`, `tests/e2e/mock-server/server.ts`
  - 💥 Explosion radius: Breaks all future wizard E2E tests that attempt to validate complete configuration.

---

## 🛠️ Suggested Fixes

### 1. Fix Save Payload

**File:** `firmware/data-src/wizard.js`
**Issue:** Timezone missing from save payload.

```diff
@@ -665,7 +665,8 @@
       tile_pixels: Number(state.tile_pixels),
       display_pin: Number(state.display_pin),
       origin_corner: Number(state.origin_corner),
       scan_dir: Number(state.scan_dir),
-      zigzag: Number(state.zigzag)
+      zigzag: Number(state.zigzag),
+      timezone: TZ_MAP[state.timezone] || 'UTC0'
     };
```

### 2. Fix Import Logic

**File:** `firmware/data-src/wizard.js`
**Issue:** `processImportedSettings` treats timezone as a generic string instead of a POSIX-to-IANA lookup.

```javascript
    for (i = 0; i < WIZARD_KEYS.length; i++) {
      key = WIZARD_KEYS[i];
      if (Object.prototype.hasOwnProperty.call(parsed, key)) {
        val = parsed[key];
        if (val !== null && typeof val !== 'object') {
          if (key === 'timezone') {
            state.timezone = POSIX_TO_IANA[val] || getTimezoneConfig().iana;
          } else {
            state[key] = String(val);
          }
          count++;
        }
      }
    }
```

---

**Review Actions:**
- Issues Found: 12
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review: fn-3.2 wizard-timezone-auto-detect**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md`

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 2 |
| AC Implementation Gaps | 6 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 5 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **8** |

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Deployable gzip bundles are stale; the device-served wizard still lacks the timezone feature | `firmware/data/wizard.html.gz` (decompressed) `63-69`; `firmware/data/wizard.js.gz` (decompressed) `109-145`, `972-1020` | +3 |
| 🟠 IMPORTANT | Wizard save path never posts `timezone` to `/api/settings` | `firmware/data-src/wizard.js:645-661` | +1 |
| 🟠 IMPORTANT | Unknown browser IANA zones fall through to a blank `<select>` instead of `UTC` | `firmware/data-src/wizard.js:93-99`, `489-494`, `1092-1097` | +1 |
| 🟠 IMPORTANT | Imported POSIX timezone values are copied into `state.timezone` without reverse lookup | `firmware/data-src/wizard.js:1007-1015`, `489-494` | +1 |
| 🟠 IMPORTANT | Initialization order is wrong; async hydration races auto-detect and never re-syncs the visible dropdown | `firmware/data-src/wizard.js:212-240`, `1083-1100` | +1 |
| 🟠 IMPORTANT | Mock server cannot exercise the real shipped behavior: it omits `timezone` and serves `data-src`, not deployable assets | `tests/e2e/mock-server/server.ts:28`, `79-101`, `391-398` | +1 |
| 🟠 IMPORTANT | Wizard E2E scaffolding is incomplete; page object has no timezone API and there is no wizard spec coverage | `tests/e2e/page-objects/WizardPage.ts:99-113`, `293-301` | +1 |
| 🟡 MINOR | Current gzip set is 50,404 bytes, over the story’s stated 50KB budget | `firmware/data/*.gz` size check | +0.3 |
| 🟢 CLEAN PASSES | Security, style/readability, type safety | review sweep | -1.5 |

### Evidence Score: 7.8

| Score | Verdict |
|-------|---------|
| **7.8** | **REJECT** |

## 🏛️ Architectural Sins

1. **[7/10] Temporal coupling in initialization.** The story explicitly depends on “hydrate first, then auto-detect only if empty,” but the implementation does the opposite. `hydrateDefaults()` is async and runs after auto-detect, so the state contract is timing-dependent instead of deterministic.  
   📍 `firmware/data-src/wizard.js:212-240`, `1083-1100`  
   💡 Fix: make `hydrateDefaults()` return a promise, wait for it, then apply auto-detect only if `state.timezone` is still empty, and immediately sync the visible `<select>`.

2. **Boundary breach in test infrastructure.** The mock server serves `firmware/data-src/`, while the actual device serves `firmware/data/*.gz`. That means local E2E can pass against a different artifact than production.  
   📍 `tests/e2e/mock-server/server.ts:28`, `391-398`  
   💡 Fix: either serve the decompressed `data/*.gz` equivalents in the mock server or add a parity test that fails when `data-src` and deployable gzip output diverge.

## ⚡ Performance & Scalability

1. **[Medium] Asset budget breach.** The current gzip set totals 50,404 bytes (`common 808 + dashboard.html 3547 + dashboard.js 25578 + health.html 512 + health.js 1712 + style.css 5239 + wizard.html 1910 + wizard.js 11098`). The story’s AC7 says “under 50KB”; current state is already over that cap.  
   📍 `firmware/data/*.gz`  
   💡 Fix: enforce a size gate in CI or at least a local check before story sign-off.

## 🐛 Correctness & Safety

1. **🐛 Deployed feature is not actually shipped.** The source files contain a timezone dropdown, but the flashed assets do not. `wizard.html.gz` still has no timezone control in Step 3, and `wizard.js.gz` still excludes `timezone` from `state`/`WIZARD_KEYS` while treating it as an extra import key. On-device behavior therefore fails AC2, AC4, AC5, and AC6 regardless of what `data-src` says.  
   📍 `firmware/data/wizard.html.gz` (decompressed) `63-69`; `firmware/data/wizard.js.gz` (decompressed) `109-145`, `972-1020`  
   🔄 Reproduction: flash/upload the current `firmware/data/` filesystem image and open the wizard; Step 3 has no timezone dropdown.

2. **🐛 `saveAndConnect()` drops timezone entirely.** The wizard payload contains WiFi, API, location, and hardware fields, but never includes `timezone`. Since `KNOWN_EXTRA_KEYS` also removed `timezone`, nothing later adds it back. The firmware never receives the POSIX string from the wizard.  
   📍 `firmware/data-src/wizard.js:140-146`, `645-661`  
   🔄 Reproduction: choose any non-default timezone in the source build, finish the wizard, and inspect the `POST /api/settings` body; no `timezone` key is sent.

3. **🐛 Unknown browser timezones violate the fallback AC.** `getTimezoneConfig()` returns `{ iana: actualBrowserZone, posix: 'UTC0' }` for unknown zones, but init stores `detected.iana`, not `UTC`. Because that value is not present in the `<select>`, the control renders blank instead of defaulting to `UTC`.  
   📍 `firmware/data-src/wizard.js:93-99`, `489-494`, `1092-1097`  
   🔄 Reproduction: stub `Intl.DateTimeFormat().resolvedOptions().timeZone` to an unmapped zone like `Antarctica/Troll`; the dropdown has no selected option.

4. **🐛 Import path breaks timezone display/editability.** `processImportedSettings()` treats `timezone` like a generic wizard key and stores the raw POSIX string in `state.timezone`. Step 3 later tries to set `<select>.value` to that POSIX string, which matches no option. The imported timezone becomes invisible and uneditable instead of being reverse-mapped to IANA.  
   📍 `firmware/data-src/wizard.js:1007-1015`, `489-494`  
   🔄 Reproduction: import `{ "timezone": "PST8PDT,M3.2.0,M11.1.0" }`; Step 3 timezone select appears blank.

5. **🐛 Returning-user hydration is racey.** The code auto-detects before `hydrateDefaults()` completes, then never updates the currently visible select when the async `/api/settings` response arrives. A fast user can reach Step 3, see the browser timezone, and save it even when the device already had a different configured timezone.  
   📍 `firmware/data-src/wizard.js:212-240`, `1083-1100`  
   🔄 Reproduction: delay `/api/settings`, click through to Step 3 quickly, then let hydration resolve; the visible dropdown stays stale.

## 🎭 Lying Tests / Missing Coverage

1. **The mock server masks the deployment bug.** It serves `firmware/data-src/` and not the actual shipped gzip assets, so it would never catch the stale `wizard.html.gz` / `wizard.js.gz` regression. It also has no `timezone` in `state.settings`, so hydration behavior for existing device timezone is untestable.  
   📍 `tests/e2e/mock-server/server.ts:28`, `79-101`, `391-398`  
   🤥 Why it lies: test runs can “pass” while the device still ships the old timezone-less wizard.

2. **The page object was not updated for the new control.** There is no `timezoneSelect` locator, no `selectTimezone()` helper, and `configureLocation()` still accepts only `(lat, lon, radiusKm)`. On top of that, the repo has no wizard spec file at all under `tests/e2e/tests/`; the only spec present is `dashboard.spec.ts`.  
   📍 `tests/e2e/page-objects/WizardPage.ts:99-113`, `293-301`  
   🤥 Why it lies: the story claims E2E support was added, but the test API cannot even express the new feature.

## 🔧 Maintainability Issues

1. **Story/file-list traceability is unreliable.** The story says the mock server and page object were updated for timezone support, but the actual code changes in those files are unrelated to timezone. That makes the implementation record untrustworthy during review.  
   📍 `tests/e2e/mock-server/server.ts:79-101`; `tests/e2e/page-objects/WizardPage.ts:293-301`  
   💥 Explosion radius: future reviewers will assume the feature is covered when it is not.

## 🛠️ Suggested Fixes

### 1. Wire timezone into the save payload

**File:** `firmware/data-src/wizard.js`  
**Issue:** The wizard never sends the firmware’s required POSIX timezone string.

```diff
 var payload = {
   wifi_ssid: state.wifi_ssid,
   wifi_password: state.wifi_password,
   os_client_id: state.os_client_id,
   os_client_sec: state.os_client_sec,
   aeroapi_key: state.aeroapi_key,
   center_lat: Number(state.center_lat),
   center_lon: Number(state.center_lon),
   radius_km: Number(state.radius_km),
+  timezone: TZ_MAP[state.timezone] || 'UTC0',
   tiles_x: Number(state.tiles_x),
   tiles_y: Number(state.tiles_y),
   tile_pixels: Number(state.tile_pixels),
   display_pin: Number(state.display_pin),
   origin_corner: Number(state.origin_corner),
```

### 2. Special-case imported timezone values

**File:** `firmware/data-src/wizard.js`  
**Issue:** Import stores POSIX in UI state instead of reverse-mapping to IANA.

```diff
 for (i = 0; i < WIZARD_KEYS.length; i++) {
   key = WIZARD_KEYS[i];
   if (Object.prototype.hasOwnProperty.call(parsed, key)) {
     val = parsed[key];
     if (val !== null && typeof val !== 'object') {
-      state[key] = String(val);
-      count++;
+      if (key === 'timezone') {
+        state.timezone = POSIX_TO_IANA[String(val)] || 'UTC';
+      } else {
+        state[key] = String(val);
+      }
+      count++;
     }
   }
 }
```

### 3. Make initialization deterministic

**File:** `firmware/data-src/wizard.js`  
**Issue:** Auto-detect runs before async hydration and never re-syncs the visible select.

```diff
-function hydrateDefaults() {
-  FW.get('/api/settings').then(function(res) {
+function hydrateDefaults() {
+  return FW.get('/api/settings').then(function(res) {
     ...
   }).catch(function() {});
 }

-// Auto-detect browser timezone; only set if state.timezone is empty
-var detected = getTimezoneConfig();
-if (!state.timezone) {
-  state.timezone = detected.iana;
-}
-wizardTimezone.value = state.timezone;
-
-hydrateDefaults();
-showStep(1);
+hydrateDefaults().finally(function() {
+  var detected = getTimezoneConfig();
+  if (!state.timezone) {
+    state.timezone = TZ_MAP[detected.iana] ? detected.iana : 'UTC';
+  }
+  wizardTimezone.value = state.timezone;
+  showStep(1);
+});
```

### 4. Fix the test surface and deployable parity

**Files:** `tests/e2e/mock-server/server.ts`, `tests/e2e/page-objects/WizardPage.ts`, deploy pipeline  
**Issue:** Tests cannot express or verify the feature, and they do not run against shipped assets.

```diff
// tests/e2e/mock-server/server.ts
 settings: {
+  timezone: 'UTC0',
   brightness: 40,
   ...
 }

// tests/e2e/page-objects/WizardPage.ts
+get timezoneSelect(): Locator {
+  return this.page.locator('#wizard-timezone');
+}
+
+async selectTimezone(iana: string): Promise<void> {
+  await this.timezoneSelect.selectOption(iana);
+}
 
-async configureLocation(lat: number, lon: number, radiusKm: number): Promise<void> {
+async configureLocation(lat: number, lon: number, radiusKm: number, timezone?: string): Promise<void> {
   ...
+  if (timezone) await this.selectTimezone(timezone);
 }
```

Also add a wizard E2E that intercepts `POST /api/settings` and asserts `payload.timezone === TZ_MAP[selectedIana]`, and add a parity check that fails if `data-src/wizard.*` and decompressed `data/wizard.*.gz` diverge.

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 8

🚫 Code requires significant rework. The current branch does not satisfy fn-3.2 as a shipped feature.

]]></file>
<file id="f63a150c" path="[Validator D]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 4 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 3 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Missing timezone in save payload — wizard never sends timezone to device | `saveAndConnect()` | +3 |
| 🔴 CRITICAL | hydrateDefaults overwrites auto-detected timezone due to wrong init ordering | `wizard.js:1083-1099` | +3 |
| 🟠 IMPORTANT | Unknown timezone fallback missing — unmapped IANA stored raw, dropdown blank | `wizard.js:1094-1097` | +1 |
| 🟠 IMPORTANT | Import lacks POSIX→IANA reverse lookup for timezone | `processImportedSettings()` | +1 |
| 🟡 MINOR | Mock server default state missing `timezone` property | `server.ts:39-101` | +0.3 |
| 🟡 MINOR | WizardPage missing timezone locator and helper methods | `WizardPage.ts` | +0.3 |
| 🟡 MINOR | Undocumented modified files in git workspace | `git status` | +0.3 |

### Evidence Score: 4.9

| Score | Verdict |
|-------|---------|
| **4.9** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows existing JS style conventions.

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

### 🐛 Bug: Missing timezone in `saveAndConnect()` payload
- **Severity:** CRITICAL
- **Source:** `firmware/data-src/wizard.js:645-661`
- **Impact:** The wizard never sends `timezone` to the firmware. The backend (`ConfigManager.cpp`, `main.cpp`) is fully wired for timezone hot-reload, but the frontend omits the key entirely. Users completing setup will have their device remain at the factory default `UTC0` regardless of browser detection or dropdown selection.
- **AC Violated:** #4

### 🐛 Bug: `hydrateDefaults()` races auto-detect and overwrites browser timezone
- **Severity:** CRITICAL
- **Source:** `firmware/data-src/wizard.js:1083-1099`
- **Impact:** The IIFE init block runs auto-detection **before** the async `hydrateDefaults()` callback returns. On a fresh device, the flow is:
  1. `state.timezone = ''`
  2. Auto-detect sets `state.timezone = 'America/Los_Angeles'`
  3. `hydrateDefaults()` fires
  4. Callback receives `d.timezone = 'UTC0'` (device default)
  5. `POSIX_TO_IANA['UTC0']` resolves to `'UTC'`
  6. `state.timezone` is overwritten to `'UTC'`
  7. `wizardTimezone.value` is **not** updated in the callback, so UI and state diverge. When the user later visits Step 3, `showStep(3)` syncs the dropdown to the stale `'UTC'` value, silently discarding the browser's timezone.
- **AC Violated:** #1

### 🐛 Bug: Unknown timezone fallback not implemented
- **Severity:** HIGH
- **Source:** `firmware/data-src/wizard.js:1094-1097`, `wizard.js:493`
- **Impact:** `getTimezoneConfig()` returns the raw browser IANA name even when it is absent from `TZ_MAP`. The init code stores this raw name in `state.timezone` and attempts to set `wizardTimezone.value` to it. Because no matching `<option>` exists, the browser renders a blank dropdown instead of defaulting to `UTC` as required.
- **AC Violated:** #3

### 🐛 Bug: Settings import stores POSIX string directly into `state.timezone`
- **Severity:** HIGH
- **Source:** `firmware/data-src/wizard.js:1007-1017`
- **Impact:** `processImportedSettings()` iterates `WIZARD_KEYS` with a generic `state[key] = String(val)` assignment. Imported settings files contain the POSIX string (e.g. `"PST8PDT,M3.2.0,M11.1.0"`), so `state.timezone` becomes a POSIX string. The dropdown expects IANA values, so it appears blank after import. There is no special-case reverse lookup using `POSIX_TO_IANA` as specified in Task 6.1.
- **AC Violated:** #6

### 🔒 [low] Mock server missing timezone round-trip support
- **Severity:** MEDIUM
- **Source:** `tests/e2e/mock-server/server.ts:39-61`, `server.ts:295-303`
- **Impact:** The `DeviceState.settings` interface and default object omit `timezone`. Because the mock server's `POST /api/settings` handler only persists keys that exist in `state.settings`, any E2E test that exercises timezone save/restore will silently drop the value. This breaks testability of the feature.
- **Fix:** Add `timezone: 'UTC0'` to the settings interface and default state.

### 🔒 [low] E2E page object lacks timezone locators
- **Severity:** MEDIUM
- **Source:** `tests/e2e/page-objects/WizardPage.ts`
- **Impact:** `WizardPage.ts` has no `timezoneSelect` locator, no `selectTimezone()` helper, and `configureLocation()` does not accept a timezone parameter. The Task 7 E2E infrastructure was not implemented, making automated validation of this story impossible.
- **Fix:** Add locator, helper, and extend `configureLocation()` signature.

### 🔒 [low] Git workspace contains undocumented modified files
- **Severity:** MEDIUM
- **Source:** `git status`
- **Impact:** The story file list claims only `wizard.*`, `WizardPage.ts`, and `server.ts` were modified. Git shows additional modifications to `dashboard.html/js/css` (and gzips), `WebPortal.cpp`, `ConfigManager.cpp`, `main.cpp`, and `test_config_manager/test_main.cpp`. These files are not accounted for in the story scope, creating ambiguity about whether unrelated changes leaked into the commit or the story documentation is incomplete.

---

## 🔧 Maintainability Issues

✅ No new tech debt bombs introduced.

---

## 🛠️ Suggested Fixes

### 1. Add `timezone` to the save payload

**File:** `firmware/data-src/wizard.js`

**Issue:** `saveAndConnect()` builds the POST payload without `timezone`.

**Diff:**
```diff
       center_lat: Number(state.center_lat),
       center_lon: Number(state.center_lon),
       radius_km: Number(state.radius_km),
+      timezone: TZ_MAP[state.timezone] || 'UTC0',
       tiles_x: Number(state.tiles_x),
```

---

### 2. Fix init ordering so auto-detect respects `hydrateDefaults()`

**File:** `firmware/data-src/wizard.js`

**Issue:** Auto-detect runs synchronously before the async `hydrateDefaults()` callback, allowing the device default to clobber the browser timezone.

**Fix:** Move the auto-detect and dropdown-value assignment into the `hydrateDefaults()` success callback.

**Diff:**
```diff
   // --- Bootstrap: hydrate defaults from firmware ---
   function hydrateDefaults() {
     FW.get('/api/settings').then(function(res) {
       if (!res.body.ok || !res.body.data) return;
       var d = res.body.data;
       // ... existing hardware hydration ...
       if (d.timezone) {
         state.timezone = POSIX_TO_IANA[d.timezone] || getTimezoneConfig().iana;
       }
       // Cache actual device brightness/color ...
       // ...
+
+      // Only auto-detect if state is still empty after hydration
+      if (!state.timezone) {
+        state.timezone = getTimezoneConfig().iana;
+      }
+      wizardTimezone.value = state.timezone;
     }).catch(function() {
       // Settings endpoint unavailable
     });
   }
```

Then remove the duplicate auto-detect from the bottom init block:

```diff
   // --- Init ---
-  // Auto-detect browser timezone; only set if state.timezone is empty
-  var detected = getTimezoneConfig();
-  if (!state.timezone) {
-    state.timezone = detected.iana;
-  }
-  wizardTimezone.value = state.timezone;
-
   hydrateDefaults();
   showStep(1);
   startScan();
```

---

### 3. Guard auto-detect against unmapped IANA zones

**File:** `firmware/data-src/wizard.js`

**Issue:** Browser timezones not in `TZ_MAP` are stored raw, causing the dropdown to render blank.

**Diff:**
```diff
-  if (!state.timezone) {
-    state.timezone = detected.iana;
-  }
+  if (!state.timezone) {
+    state.timezone = TZ_MAP[detected.iana] ? detected.iana : 'UTC';
+  }
```

And in `showStep(3)`:
```diff
-      wizardTimezone.value = state.timezone || getTimezoneConfig().iana;
+      wizardTimezone.value = TZ_MAP[state.timezone] ? state.timezone : 'UTC';
```

---

### 4. Reverse-lookup timezone on settings import

**File:** `firmware/data-src/wizard.js`

**Issue:** `processImportedSettings()` assigns the POSIX string directly to `state.timezone`.

**Diff:**
```diff
     for (i = 0; i < WIZARD_KEYS.length; i++) {
       key = WIZARD_KEYS[i];
       if (Object.prototype.hasOwnProperty.call(parsed, key)) {
         val = parsed[key];
-        // Skip null and non-primitive values to avoid "null" / "[object Object]" in state
-        if (val !== null && typeof val !== 'object') {
-          state[key] = String(val);
-          count++;
+        if (key === 'timezone') {
+          if (val) {
+            state.timezone = POSIX_TO_IANA[String(val)] || getTimezoneConfig().iana;
+            count++;
+          }
+        } else if (val !== null && typeof val !== 'object') {
+          state[key] = String(val);
+          count++;
         }
       }
     }
```

---

### 5. Update mock server default state for timezone

**File:** `tests/e2e/mock-server/server.ts`

**Diff:**
```diff
   interface DeviceState {
     settings: {
       // ...
+      timezone: string;
       wifi_ssid: string;
       // ...
     };
   }
 
   const state: DeviceState = {
     settings: {
       // ...
+      timezone: 'UTC0',
       wifi_ssid: 'MockNetwork',
       // ...
     },
   };
```

---

### 6. Add timezone E2E support to `WizardPage`

**File:** `tests/e2e/page-objects/WizardPage.ts`

**Diff:**
```diff
+  get timezoneSelect(): Locator {
+    return this.page.locator('#wizard-timezone');
+  }
+
+  async selectTimezone(iana: string): Promise<void> {
+    await this.timezoneSelect.selectOption(iana);
+  }
+
   async configureLocation(
     lat: number,
     lon: number,
-    radiusKm: number
+    radiusKm: number,
+    timezone?: string
   ): Promise<void> {
     await this.fillInput(this.centerLatInput, String(lat));
     await this.fillInput(this.centerLonInput, String(lon));
     await this.fillInput(this.radiusKmInput, String(radiusKm));
+    if (timezone !== undefined) {
+      await this.selectTimezone(timezone);
+    }
   }
```

---

**Review Actions:**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 6

⚠️ Address the identified issues before proceeding. The two CRITICAL bugs (missing payload field and race-condition overwrite) mean the timezone feature is non-functional in the current implementation despite the firmware backend being fully ready.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-13</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-3</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">4</var>
<var name="session_id">481b2954-7457-4449-92cc-8af2a1b47e7b</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="1dffb869">embedded in prompt, file id: 1dffb869</var>
<var name="story_id">fn-3.2</var>
<var name="story_key">fn-3-2-wizard-timezone-auto-detect</var>
<var name="story_num">2</var>
<var name="story_title">2-wizard-timezone-auto-detect</var>
<var name="template">False</var>
<var name="timestamp">20260413_1844</var>
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
      - Commit message format: fix(component): brief description (synthesis-fn-3.2)
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