<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-3 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T231408Z -->
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

## Senior Developer Review (AI)

### Review: 2026-04-13
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 7.8 → REJECT
- **Issues Found:** 8 verified
- **Issues Fixed:** 8 (all applied in this synthesis pass)
- **Action Items Created:** 1 (deferred improvement)

#### Review Follow-ups (AI)
- [ ] [AI-Review] LOW: Consider making wizard init fully deterministic — move auto-detect inside `hydrateDefaults()` callback so hydration always completes before fallback fires; current fix (syncing wizardTimezone.value in hydration) is correct but leaves a brief UI flash for fresh devices (`firmware/data-src/wizard.js`, `hydrateDefaults()`)


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

## Story fn-3-2 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `timezone` missing from `saveAndConnect()` payload — POSIX string never sent to firmware | Added `timezone: TZ_MAP[state.timezone] \|\| 'UTC0'` after `radius_km` in payload object |
| critical | Stale gz deployables — `wizard.js.gz` had `timezone` in `KNOWN_EXTRA_KEYS` (not `WIZARD_KEYS`), `wizard.html.gz` had no timezone dropdown at all | Rebuilt both from current `data-src/` sources after all source fixes applied; verified timezone in WIZARD_KEYS and `TZ_MAP[state.timezone] \|\| 'UTC0'` in payload in decompressed output |
| high | `processImportedSettings()` stores POSIX string directly in `state.timezone` — dropdown appears blank after import, save sends `UTC0` | Added `if (key === 'timezone')` special case that reverse-maps via `POSIX_TO_IANA` before the generic `state[key] = String(val)` branch |
| high | Unknown browser IANA timezone stored raw in `state.timezone` — unmapped zones (e.g. `Antarctica/Troll`) produce no matching `<option>`, dropdown renders blank instead of defaulting to UTC (AC3 violation) | Changed `state.timezone = detected.iana` to `state.timezone = TZ_MAP[detected.iana] ? detected.iana : 'UTC'` |
| high | `hydrateDefaults()` sets `state.timezone` but never updates `wizardTimezone.value` — after async hydration fires, state and the visible `<select>` diverge; on Step 3 entry the dropdown syncs to the stale `state.timezone` (e.g. "UTC" from factory default), discarding auto-detected browser timezone | Added `if (wizardTimezone) wizardTimezone.value = state.timezone;` immediately after the `state.timezone` assignment in `hydrateDefaults()` |
| high | `showStep(3)` fallback `wizardTimezone.value = state.timezone | Changed to `wizardTimezone.value = state.timezone \|\| 'UTC'` |
| medium | `DeviceState.settings` in mock server missing `timezone` field — `POST /api/settings` handler uses `key in state.settings` guard, so `timezone` is silently dropped and never round-trips through `GET /api/settings` in E2E tests | Added `timezone: string` to `DeviceState.settings` interface and `timezone: 'UTC0'` to initial state object |
| medium | `WizardPage.ts` has no `timezoneSelect` locator, no `selectTimezone()` helper, and `configureLocation()` has no timezone parameter — Task 7 not implemented | Added `timezoneSelect` getter, `selectTimezone(iana)` async helper, updated `configureLocation()` with optional `timezone?` parameter |
| low | `POSIX_TO_IANA` reverse map built without sorting — insertion order (V8 deterministic, but not alphabetically-first) used instead of sorted order; story spec explicitly says "alphabetically-first match" | Changed `Object.keys(TZ_MAP).forEach` to `Object.keys(TZ_MAP).sort().forEach` |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | `hydrateDefaults` clobbers auto-detected browser timezone with device default `UTC` on fresh devices. | wizard.js:231 | +1 |
| 🟠 IMPORTANT | Race condition in `hydrateDefaults` overwriting manual user changes. | wizard.js:231 | +1 |
| 🟠 IMPORTANT | Mock Server divergence on reboot keys vs Architecture. | server.ts:211 | +1 |
| 🟡 MINOR | Incorrect count in `WIZARD_KEYS` comment (15 vs 16). | wizard.js:133 | +0.3 |
| 🟡 MINOR | Unnecessary fallback selectors in `WizardPage.ts`. | WizardPage.ts:85 | +0.3 |
| 🟡 MINOR | Code duplication for POSIX-to-IANA resolution. | wizard.js:231, 1020 | +0.3 |
| 🟡 MINOR | User-unfriendly mapping for `UTC0` (maps to `Etc/UTC`). | wizard.js:188 | +0.3 |
| 🟡 MINOR | Asset budget risk (headroom < 0.5KB). | wizard.js | +0.3 |
| 🟢 CLEAN PASS | SOLID | - | -0.5 |
| 🟢 CLEAN PASS | Abstraction | - | -0.5 |
| 🟢 CLEAN PASS | Lying Tests | - | -0.5 |
| 🟢 CLEAN PASS | Performance | - | -0.5 |
| 🟢 CLEAN PASS | Style | - | -0.5 |
| 🟢 CLEAN PASS | Type Safety | - | -0.5 |
| 🟢 CLEAN PASS | Security | - | -0.5 |

### Evidence Score: 0.8

| Score | Verdict |
|-------|---------|
| **0.8** | **APPROVED** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

- **[2/10] Incorrect comment count:** `WIZARD_KEYS` comment says 15 keys, but the array contains 16.
  - 📍 `firmware/data-src/wizard.js:133`
- **[3/10] Code Duplication:** The logic to resolve a POSIX string to an IANA name (using `POSIX_TO_IANA` with `getTimezoneConfig().iana` fallback) is duplicated in `hydrateDefaults()` and `processImportedSettings()`.
  - 📍 `firmware/data-src/wizard.js:231` and `firmware/data-src/wizard.js:1020`
  - 💡 Fix: Extract into a `resolveIana(posix)` helper.

---

## ⚡ Performance & Scalability

- **[LOW] Asset budget risk:** Total gzipped assets are extremely close to the 50KB ceiling (~49.22KB before this story). The additions in this story reduce the remaining headroom to <0.5KB.
  - 📍 `firmware/data-src/wizard.js`
  - 💡 Fix: Audit `TZ_MAP` for less common zones to strip if necessary, or optimize existing JS.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `hydrateDefaults` clobbers auto-detected browser timezone with device default `UTC` on fresh devices.
  - 📍 `firmware/data-src/wizard.js:231`
  - 🔄 Reproduction: 1. Launch wizard in a timezone like `America/Los_Angeles`. 2. IIFE Init sets `state.timezone` to LA correctly. 3. `hydrateDefaults` fires (async). 4. Fresh device returns `timezone: "UTC0"`. 5. `hydrateDefaults` resolves and sets `state.timezone = "UTC"`, overwriting the detected LA time.
- **🐛 Bug:** Race condition in `hydrateDefaults` overwriting user changes.
  - 📍 `firmware/data-src/wizard.js:231`
  - 🔄 Reproduction: If a user enters Step 3 and selects a timezone before the async `hydrateDefaults` resolves, the resolve handler will overwrite the user's manual selection with the device's saved value.
- **🎭 Lying Test:** Mock Server divergence on reboot keys.
  - 📍 `tests/e2e/mock-server/server.ts:211`
  - 🤥 Why it lies: The mock server lists `tiles_x`, `tiles_y`, and `tile_pixels` as `rebootKeys`, but the system architecture (`architecture.md` Decision 1) specifies these as hot-reload keys. This could lead to E2E tests passing/failing differently than real hardware.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Unnecessary fallback selectors in `WizardPage.ts`.
  - 📍 `tests/e2e/page-objects/WizardPage.ts:85, 89, 93, 101, etc.`
  - 💥 Explosion radius: MINOR. Maintenance overhead of keeping dead fallback selectors. The `wizard.html` does not use `name` attributes for these fields.

---

## 🛠️ Suggested Fixes

### 1. Fix Timezone Clobbering in Hydration

**File:** `firmware/data-src/wizard.js`
**Issue:** `hydrateDefaults` overwrites auto-detected timezone with the default `UTC0` value from unconfigured devices.

**Corrected code:**
```javascript
      // Only overwrite if the device has a non-default timezone
      if (d.timezone && d.timezone !== 'UTC0') {
        state.timezone = POSIX_TO_IANA[d.timezone] || state.timezone || getTimezoneConfig().iana;
        if (wizardTimezone) wizardTimezone.value = state.timezone;
      }
```

### 2. Update WIZARD_KEYS Count

**File:** `firmware/data-src/wizard.js`
**Issue:** Stale comment count.

**Corrected code:**
```javascript
  // Keys that map to wizard form fields (16 keys)
  var WIZARD_KEYS = [
```

### 3. Fix Mock Server Reboot Keys

**File:** `tests/e2e/mock-server/server.ts`
**Issue:** Divergence from architecture on hot-reload vs reboot keys.

**Corrected code:**
```typescript
    const rebootKeys = [
      'wifi_ssid',
      'wifi_password',
      'os_client_id',
      'os_client_sec',
      'aeroapi_key',
      'display_pin'
    ];
```

---

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 0

✅ Code is approved and ready for deployment!

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 2 |
| Hidden Bugs | 3 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **10** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | `hydrateDefaults()` async callback overwrites user-modified timezone dropdown without guarding against prior interaction | `wizard.js:231-232` | +3 |
| 🔴 CRITICAL | `hydrateDefaults()` falls back to raw browser IANA without `TZ_MAP` validation, causing blank dropdown for unmapped zones | `wizard.js:231` | +3 |
| 🔴 CRITICAL | `processImportedSettings()` repeats the same unmapped-IANA fallback bug on timezone import | `wizard.js:1020` | +3 |
| 🟠 IMPORTANT | `validateStep(3)` validates lat/lon/radius but ignores timezone, allowing invalid values to pass silently | `wizard.js:577-596` | +1 |
| 🟠 IMPORTANT | `completeSetup()` type and implementation exclude timezone, blocking full E2E wizard-flow coverage | `WizardPage.ts:391-418` | +1 |
| 🟠 IMPORTANT | No wizard E2E test specs exist; timezone auto-detect, import, and selection paths are completely untested | `tests/e2e/tests/` | +1 |
| 🟠 IMPORTANT | Mock server `POST /api/settings` accepts arbitrary timezone strings without POSIX validation | `server.ts:297-304` | +1 |
| 🟡 MINOR | `showStep(3)` rehydrates dropdown from `state.timezone` without validating against `TZ_MAP` | `wizard.js:496` | +0.3 |
| 🟡 MINOR | Timezone `<select>` lacks real-time `change` listener; state diverges from DOM until navigation | `wizard.js` | +0.3 |
| 🟡 MINOR | `getTimezoneConfig()` swallows all exceptions silently with no logging or user feedback | `wizard.js:96` | +0.3 |
| 🟢 CLEAN PASS | Lying Tests (none exist to lie) | — | -0.5 |
| 🟢 CLEAN PASS | Security (no new vulnerabilities introduced) | — | -0.5 |

### Evidence Score: **12.9**

| Score | Verdict |
|-------|---------|
| **12.9** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[7/10] S - Single Responsibility:** `hydrateDefaults()` violates SRP by conflating async API fetching, POSIX→IANA timezone translation, DOM mutation, and device color caching in a single callback. The timezone conversion logic added by this story increases its surface area rather than extracting it.
  - 📍 `wizard.js:212-243`
  - 💡 Fix: Extract `resolveTimezone(ianaOrPosix)` pure function and a separate `syncTimezoneDropdown()` DOM helper.

- **[6/10] D - Don't Repeat Yourself:** POSIX→IANA fallback logic is copy-pasted in three locations (IIFE init, `hydrateDefaults`, `processImportedSettings`). Any change to the fallback rule (e.g., switching default from UTC to browser-detected-mapped) requires touching all three sites.
  - 📍 `wizard.js:231`, `wizard.js:1020`, `wizard.js:1107`
  - 💡 Fix: Extract a single `resolveTimezoneState(fallbackToDetected)` function reused by all three call sites.

---

## 🐍 Code Style & Readability

- **Inconsistent guard patterns:** The IIFE init at line 1107 correctly guards unmapped IANA zones with `TZ_MAP[detected.iana] ? detected.iana : 'UTC'`, but `hydrateDefaults()` and `processImportedSettings()` omit this guard. This inconsistency makes the code harder to reason about and introduces the blank-dropdown bugs.
  - 📍 `wizard.js:231`, `wizard.js:1020`

- **Missing type annotation:** `configureLocation()` accepts an optional `timezone?: string` but `completeSetup()` omits it from the `config` parameter type, making the API surface incomplete and misleading for consumers.
  - 📍 `WizardPage.ts:391-398`

---

## ⚡ Performance & Scalability

- **[Low] DOM reflow storm:** The timezone dropdown is populated by 60 individual `appendChild()` calls without a `DocumentFragment`. On constrained devices this triggers ~60 reflows.
  - 📍 `wizard.js:1096-1101`
  - 💡 Fix: Build options in a `DocumentFragment` and append once.

---

## 🐛 Correctness & Safety

- **🐛 Race condition — hydration clobbers user input:** `hydrateDefaults()` fires asynchronously. If a user navigates to Step 3 and changes the timezone before `/api/settings` responds, the callback unconditionally overwrites `wizardTimezone.value` with the server-fetched value.
  - 📍 `wizard.js:230-233`
  - 🔄 Reproduction: (1) Load wizard on a slow network, (2) immediately proceed to Step 3, (3) select a different timezone, (4) wait for the API callback — selection reverts.

- **🐛 Blank dropdown on unmapped browser zone during hydration:** `hydrateDefaults()` uses `getTimezoneConfig().iana` as a fallback without checking `TZ_MAP`. If the browser reports an unmapped IANA zone (e.g., `Antarctica/Troll`), `state.timezone` becomes that raw string, and the `<select>` displays blank because no matching `<option>` exists. The IIFE init correctly falls back to `'UTC'`; `hydrateDefaults()` does not.
  - 📍 `wizard.js:231`
  - 🔄 Reproduction: Set device timezone to a POSIX string not in `POSIX_TO_IANA`, open wizard with browser set to an unmapped IANA zone.

- **🐛 Blank dropdown on unmapped zone during import:** `processImportedSettings()` has the exact same unmapped-IANA bug. Importing a settings file with an unknown POSIX timezone can leave the dropdown blank.
  - 📍 `wizard.js:1020`
  - 🔄 Reproduction: Import a settings file with `"timezone": "UNKNOWN0"` and browser set to an unmapped IANA zone.

- **🐛 Silent UTC fallback on save:** `saveAndConnect()` uses `TZ_MAP[state.timezone] || 'UTC0'`. If `state.timezone` is corrupted or contains an unmapped IANA name, the user receives no warning — their timezone is silently reset to UTC in the POST payload.
  - 📍 `wizard.js:657`

---

## 🔧 Maintainability Issues

- **💣 Tech Debt — No timezone validation in Step 3:** Now that timezone is a first-class wizard field, `validateStep(3)` should enforce it just like lat/lon/radius. Omitting this means invalid/missing timezone values can reach the save payload undetected.
  - 📍 `wizard.js:577-596`
  - 💥 Explosion radius: Every future bug in timezone state handling will propagate silently to the device.

- **💣 Tech Debt — Mock server accepts garbage timezone data:** The mock server is the primary E2E validation surface, yet it blindly assigns any string to `state.settings.timezone`. Tests could pass with `"timezone": "lol"` that would fail on real firmware, creating false confidence.
  - 📍 `server.ts:297-304`
  - 💥 Explosion radius: E2E suite gives false negatives for invalid timezone handling.

---

## 🛠️ Suggested Fixes

### 1. Guard `hydrateDefaults()` against overwriting user input and unmapped IANA zones

**File:** `firmware/data-src/wizard.js`

Capture the timezone value before hydration and only sync the DOM if the user hasn't changed it. Also guard the fallback against unmapped IANA names.

```javascript
// Before hydrateDefaults() call (around line 1111):
var timezoneBeforeHydration = state.timezone;

// Inside hydrateDefaults() callback (replace lines 230-233):
if (d.timezone) {
  var detected = getTimezoneConfig();
  var fallbackTz = TZ_MAP[detected.iana] ? detected.iana : 'UTC';
  var hydratedTz = POSIX_TO_IANA[d.timezone] || fallbackTz;
  state.timezone = hydratedTz;
  if (wizardTimezone && (!timezoneBeforeHydration || wizardTimezone.value === timezoneBeforeHydration)) {
    wizardTimezone.value = hydratedTz;
  }
}
```

### 2. Fix unmapped-IANA fallback in `processImportedSettings()`

**File:** `firmware/data-src/wizard.js`

Replace line 1020 with the same guarded fallback:

```javascript
if (key === 'timezone') {
  if (val) {
    var detected2 = getTimezoneConfig();
    var fallback2 = TZ_MAP[detected2.iana] ? detected2.iana : 'UTC';
    state.timezone = POSIX_TO_IANA[String(val)] || fallback2;
  }
}
```

### 3. Add timezone validation to `validateStep(3)`

**File:** `firmware/data-src/wizard.js`

After the radius validation block (around line 596), add:

```javascript
if (!state.timezone || !TZ_MAP[state.timezone]) {
  locErr.textContent = 'Please select a valid timezone.';
  locErr.style.display = '';
  if (!state.timezone || !TZ_MAP[state.timezone]) {
    // Optionally mark the select with error styling if your CSS supports it
  }
  return false;
}
```

### 4. Add `timezone` to `completeSetup()` contract

**File:** `tests/e2e/page-objects/WizardPage.ts`

Update the `config` type and pass-through call:

```typescript
async completeSetup(config: {
  wifi: { ssid: string; password: string };
  api: {
    openSkyClientId: string;
    openSkyClientSecret: string;
    aeroApiKey: string;
  };
  location: { lat: number; lon: number; radiusKm: number; timezone?: string };
  hardware: { tilesX: number; tilesY: number; tilePixels: number; displayPin?: number };
}): Promise<void> {
  // ...
  await this.configureLocation(
    config.location.lat,
    config.location.lon,
    config.location.radiusKm,
    config.location.timezone
  );
  // ...
}
```

### 5. Validate timezone in mock server

**File:** `tests/e2e/mock-server/server.ts`

Inside the `POST /api/settings` loop, add a lightweight guard:

```typescript
if (key === 'timezone' && typeof value === 'string') {
  // Basic POSIX timezone validation: must contain at least one offset digit
  if (!/\d/.test(value)) {
    sendError(res, 'Invalid timezone format', 'INVALID_TIMEZONE');
    return;
  }
}
```

---

**Review Actions:**
- Issues Found: 10
- Issues Fixed: 0
- Action Items Created: 5

🚫 Code requires significant rework. Review action items carefully.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-3.2-wizard-timezone-auto-detect.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 0 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **3** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Uncommitted changes include work from multiple stories (fn-3.1 + fn-3.2). Git diff shows 390 line changes across wizard.html and wizard.js, including Step 6 implementation (fn-3.1), hardware select dropdowns (fn-3.1), and timezone auto-detect (fn-3.2). While fn-3.2 implementation is correct, mixing stories in uncommitted changes creates scope ambiguity and complicates code review. | `git diff HEAD --stat` shows 390 changes; diff includes Step 6 UI and positioning canvas code unrelated to timezone. | +1 |
| 🟡 MINOR | No E2E test specs for the wizard flow. Story Task 7 updated WizardPage.ts and mock server correctly, but there are no actual E2E test files that exercise the wizard (tests/e2e/tests/ only contains dashboard.spec.ts). Without wizard test specs, there's no automated verification of the complete user journey, including timezone selection, navigation, and save flow. | `tests/e2e/tests/` contains only dashboard.spec.ts; no wizard.spec.ts or equivalent test file exists. | +0.3 |
| 🟡 MINOR | Architectural improvement opportunity: Timezone state management relies on multiple independent hydration sources (auto-detect at init, device settings via hydrateDefaults(), import via processImportedSettings()). While each path correctly reverse-maps POSIX to IANA, the hydration logic is scattered across three different functions. Centralizing timezone hydration logic into a single helper function would improve maintainability and reduce duplication of the POSIX_TO_IANA fallback pattern. | Timezone hydration occurs in three places: lines 1105-1109 (auto-detect), 230-233 (hydrateDefaults), 1018-1021 (processImportedSettings). | +0.3 |

### Evidence Score: 1.6

| Score | Verdict |
|-------|---------|
| **1.6** | **APPROVED** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

The implementation correctly follows the established architecture:
- **Decision F4 (browser-side IANA-to-POSIX mapping)**: TZ_MAP is used correctly, with POSIX strings sent to firmware via POST payload (wizard.js:657)
- **Rule 11 (NVS key patterns)**: Uses existing 'timezone' key from ScheduleConfig, no new NVS keys introduced
- **Rule 10 (fetch error handling)**: No new fetch calls added; timezone piggybacks on existing saveAndConnect() POST
- **RenderContext isolation**: Not applicable (this is web UI, not firmware mode code)

---

## 🐍 Pythonic Crimes &amp; Readability

✅ Code follows style guidelines and is readable.

**JavaScript-specific observations:**
- Variable naming follows consistent patterns (camelCase for variables, PascalCase for constructors)
- Code organization is logical with clear section comments
- No magic numbers; constants like 'UTC0' and 'UTC' are intentional fallback values
- Consistent use of `getTimezoneConfig()` and `TZ_MAP` for timezone operations

---

## ⚡ Performance &amp; Scalability

✅ No significant performance issues detected.

**Performance analysis:**
- **TZ_MAP lookup**: O(n) for reverse map building (line 186), runs once at init with 60+ entries — negligible
- **Dropdown population**: O(n) with 60+ options — negligible for single render
- **No blocking operations**: All async operations (hydrateDefaults, import) are non-blocking
- **Memory footprint**: TZ_MAP + POSIX_TO_IANA maps add ~2KB to memory — acceptable
- **Asset budget**: wizard.js.gz = 12K, wizard.html.gz = 4K. Total gzipped web assets = 60K (under 50KB target, but story spec says "under 50KB budget" — this is a minor discrepancy; actual impact is minimal and well within overall 4MB flash budget)

---

## 🐛 Correctness &amp; Safety

✅ Code appears correct and secure.

**Security analysis:**
- **No injection vulnerabilities**: Timezone values are restricted to dropdown options (IANA names from TZ_MAP keys), no free-form user input for timezone
- **No credential exposure**: No new credentials or secrets introduced
- **Input validation**: Timezone dropdown is a `<select>` element, so user input is constrained to valid option values
- **Fallback safety**: `TZ_MAP[state.timezone] || 'UTC0'` ensures invalid values can't corrupt device config (wizard.js:657)
- **Import safety**: processImportedSettings() uses special handling for timezone to prevent POSIX string pollution of state (wizard.js:1018-1021)

**Correctness verification:**
- **AC1 (Auto-Detect)**: Correctly detects via `Intl.DateTimeFormat().resolvedOptions().timeZone`, checks TZ_MAP presence, falls back to 'UTC' for unmapped zones (wizard.js:1105-1108) ✅
- **AC2 (Dropdown)**: Dropdown exists in HTML (wizard.html:69-70), populated from TZ_MAP keys in sorted order (wizard.js:1095-1101) ✅
- **AC3 (Fallback)**: Unmapped zones default to 'UTC' via `TZ_MAP[detected.iana] ? detected.iana : 'UTC'` (wizard.js:1107) ✅
- **AC4 (POSIX to device)**: Payload sends `TZ_MAP[state.timezone] || 'UTC0'` (wizard.js:657) ✅
- **AC5 (Review display)**: Review step shows `{ label: 'Timezone', value: state.timezone || 'UTC' }` (wizard.js:414) ✅
- **AC6 (Import compatibility)**: Special case handling reverses POSIX to IANA via POSIX_TO_IANA (wizard.js:1018-1021) ✅

**Hidden bug analysis:**
- **Race condition in init**: Potential issue where dropdown is populated AFTER value is set. However, code order is correct: dropdown populate (1095-1101) → auto-detect (1103-1108) → set value (1109) → hydrateDefaults (1111). This is correct; setting value after population ensures the option exists.
- **State synchronization**: hydrateDefaults() correctly syncs wizardTimezone.value after updating state.timezone (line 232), preventing UI divergence.
- **Edge case handling**: Empty state.timezone, unmapped IANA zones, missing POSIX in import — all handled with appropriate fallbacks.

---

## 🔧 Maintainability Issues

**💣 Tech Debt: Scattered timezone hydration logic**
- 📍 `firmware/data-src/wizard.js`: Lines 1105-1109 (auto-detect), 230-233 (hydrateDefaults), 1018-1021 (processImportedSettings)
- 💥 Explosion radius: Low — logic is simple and defensive, but duplication of POSIX_TO_IANA fallback pattern creates maintenance burden if timezone handling rules change
- 💡 Suggestion: Extract a single helper function `resolveTimezone(posixOrIana)` that centralizes the fallback logic

**💣 Tech Debt: Uncommitted changes span multiple stories**
- 📍 `git diff HEAD --stat` shows 390 line changes
- 💥 Explosion radius: Medium — makes code review and git history more complex; fn-3.1 (Step 6) and fn-3.2 (timezone) changes are mixed
- 💡 Suggestion: Commit fn-3.1 and fn-3.2 separately to maintain clean story boundaries

---

## 🛠️ Suggested Fixes

### 1. Extract Centralized Timezone Helper (Optional Refactoring)

**File:** `firmware/data-src/wizard.js`
**Issue:** Timezone hydration logic is duplicated across three functions

**Refactored code:**
```javascript
// Add after getTimezoneConfig() function (line 100)
/**
 * Resolve timezone to IANA name from POSIX string, IANA name, or auto-detect.
 * @param {string} posixOrIana - POSIX string (e.g. "PST8PDT,...") or IANA name (e.g. "America/Los_Angeles")
 * @returns {string} IANA timezone name, with fallback to auto-detected or 'UTC'
 */
function resolveTimezone(posixOrIana) {
  // If it's a POSIX string, reverse-lookup to IANA
  if (POSIX_TO_IANA[posixOrIana]) {
    return POSIX_TO_IANA[posixOrIana];
  }
  // If it's already an IANA name in TZ_MAP, use it
  if (TZ_MAP[posixOrIana]) {
    return posixOrIana;
  }
  // Fallback to auto-detected browser timezone
  var detected = getTimezoneConfig().iana;
  return TZ_MAP[detected] ? detected : 'UTC';
}

// Then use in hydrateDefaults (line 231):
if (d.timezone) {
  state.timezone = resolveTimezone(d.timezone);
  if (wizardTimezone) wizardTimezone.value = state.timezone;
}

// And in processImportedSettings (line 1020):
if (key === 'timezone') {
  if (val) {
    state.timezone = resolveTimezone(String(val));
  }
}
```

### 2. Commit Stories Separately (Process Improvement)

**Action:** Separate the uncommitted changes into two commits:
1. Commit fn-3.1 changes: Step 6 UI, positioning canvas, RGB test, hardware select dropdowns
2. Commit fn-3.2 changes: Timezone dropdown, auto-detect, state management, import handling

**Rationale:** Clean git history makes future debugging and code review easier. Each commit should represent a single logical unit of work (one story).

### 3. Add Wizard E2E Test Spec (Test Coverage Gap - Optional)

**File:** `tests/e2e/tests/wizard.spec.ts` (NEW FILE)
**Issue:** No automated tests for the wizard flow, including timezone selection

**Example test:**
```typescript
import { test, expect } from '../helpers/test-fixtures.js';

test.describe('Wizard Timezone Selection', () => {
  test('should auto-detect browser timezone and pre-fill dropdown', async ({ wizardPage }) => {
    await wizardPage.navigate();
    // Navigate to Step 3 (Location)
    await wizardPage.goNext(); // Step 1 → 2
    await wizardPage.goNext(); // Step 2 → 3
    
    // Verify timezone dropdown has a value (not empty)
    const selectedTimezone = await wizardPage.timezoneSelect.inputValue();
    expect(selectedTimezone).not.toBe('');
    expect(selectedTimezone).not.toBe('UTC'); // Browser should detect actual timezone in test environment
  });

  test('should include timezone in review step', async ({ wizardPage }) => {
    await wizardPage.navigate();
    // Complete wizard through Step 5
    await wizardPage.configureWifi('TestSSID', 'TestPass123');
    await wizardPage.goNext();
    await wizardPage.configureApiCredentials('test-client-id', 'test-secret', 'test-api-key');
    await wizardPage.goNext();
    await wizardPage.configureLocation(37.7749, -122.4194, 10, 'America/Los_Angeles');
    await wizardPage.goNext();
    await wizardPage.configureMatrix(10, 2, 16);
    await wizardPage.goNext(); // Step 5
    
    // Step 5 (Review) should show timezone
    const reviewText = await wizardPage.page.locator('.review-sections').textContent();
    expect(reviewText).toContain('Timezone');
    expect(reviewText).toContain('America/Los_Angeles');
  });
});
```

---

**Review Actions:**
- Issues Found: 3
- Issues Fixed: 0
- Action Items Created: 0

✅ Code is approved and ready for deployment!

**Summary:** The fn-3.2 timezone auto-detect feature is correctly implemented with all acceptance criteria met. Code quality is high with proper error handling, fallback mechanisms, and architectural compliance. Minor issues relate to process (uncommitted mixed-story changes) and test coverage (no wizard E2E specs), but these don't block deployment. Optional refactoring could improve maintainability by centralizing timezone hydration logic.

]]></file>
<file id="f63a150c" path="[Validator D]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

AC verdict: `AC2`, `AC4`, and `AC5` are implemented. `AC1`, `AC3`, and `AC6` are only partial. `AC7` is manually rebuilt, but the current test harness does not validate the shipped deployables.

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Async hydration can silently overwrite the user's selected or imported timezone after Step 3 is already in use. | `firmware/data-src/wizard.js:217-233` | +3 |
| 🔴 CRITICAL | Unknown POSIX fallback uses raw browser IANA in hydration/import paths, which can blank the dropdown and silently save `UTC0`. | `firmware/data-src/wizard.js:230-232`, `1018-1020`, `1105-1109`, `657` | +3 |
| 🟠 IMPORTANT | Unsupported timezones have no manual POSIX escape hatch, violating Architecture Decision F4. | `firmware/data-src/wizard.html:67-71`, `firmware/data-src/wizard.js:1094-1109` | +1 |
| 🟠 IMPORTANT | There is still no wizard E2E spec; only dashboard specs exist, so this story's behavior is unasserted. | `tests/e2e/tests/dashboard.spec.ts:1` | +1 |
| 🟠 IMPORTANT | The E2E mock server serves `firmware/data-src/`, not shipped `firmware/data/*.gz`, so deployable assets are never exercised. | `tests/e2e/mock-server/server.ts:28`, `393-400` | +1 |
| 🟠 IMPORTANT | The main `WizardPage.completeSetup()` flow still cannot pass a timezone, so the new field is awkward to test and easy to skip. | `tests/e2e/page-objects/WizardPage.ts:305-316`, `391-418` | +1 |
| 🟢 CLEAN PASS | Security and Type Safety | — | -1.0 |

### Evidence Score: 8.0

| Score | Verdict |
|-------|---------|
| **8.0** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] Boundary breach:** Unsupported-zone handling stops at a dropdown + UTC fallback. The provided architecture required a manual POSIX override for unmapped browser zones, and the wizard still has no such path.
  - 📍 `firmware/data-src/wizard.html:67-71`
  - 💡 Fix: Add a conditional manual POSIX input or explicit “Manual timezone” mode when `TZ_MAP` has no match.

- **Test-environment drift:** The mock server bypasses the actual LittleFS deployable artifacts and serves source files directly, so story work on `wizard.html.gz` / `wizard.js.gz` is not validated by E2E.
  - 📍 `tests/e2e/mock-server/server.ts:28`, `393-400`

---

## 🐍 Pythonic Crimes & Readability

✅ No language-style issues worth blocking this story.

---

## ⚡ Performance & Scalability

✅ No material runtime performance regression in the reviewed wizard logic.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `hydrateDefaults()` unconditionally writes `state.timezone` when `/api/settings` returns, even though the rest of Step 3 hydration is guarded against stomping user-entered values. On a slow AP response, the user can choose a timezone or import settings, then get silently overwritten by the late GET response.
  - 📍 `firmware/data-src/wizard.js:217-233`
  - 🔄 Reproduction: Load the wizard, throttle `/api/settings`, navigate to Step 3, select a non-default timezone, then let the GET response return with a stored timezone.

- **🐛 Bug:** The init path normalizes unknown browser zones to `UTC`, but the hydration/import paths do not. Both use `getTimezoneConfig().iana` directly, so an unsupported browser zone like `Antarctica/Troll` plus an unknown imported/device POSIX value leaves the `<select>` unmatched and later falls through to `timezone: TZ_MAP[state.timezone] || 'UTC0'`.
  - 📍 `firmware/data-src/wizard.js:230-232`, `1018-1020`, `1105-1109`, `657`
  - 🔄 Reproduction: Browser reports an unmapped IANA zone, then import a settings file with an unknown `timezone` POSIX string.

- **🎭 Lying Test:** The story added page-object and mock-server helpers, but there is still no wizard spec asserting auto-detect, reverse lookup, review rendering, or the final POSIX payload. CI can pass with this feature broken.
  - 📍 `tests/e2e/tests/dashboard.spec.ts:1`
  - 🤥 Why it lies: The suite only exercises the dashboard; none of the new wizard-timezone behavior is covered by a real test.

✅ No significant security vulnerabilities found in the reviewed story code.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `configureLocation()` accepts `timezone`, but the primary `completeSetup()` helper does not expose it in its config contract and never forwards it. That keeps the story half-integrated in the one helper future specs will actually use.
  - 📍 `tests/e2e/page-objects/WizardPage.ts:305-316`, `391-418`
  - 💥 Explosion radius: Future wizard specs will either skip timezone coverage or bypass the main helper and duplicate setup flow.

---

## 🛠️ Suggested Fixes

### 1. Guard async hydration and normalize timezone in one place

**File:** `firmware/data-src/wizard.js`  
**Issue:** Late `/api/settings` hydration can overwrite valid state, and fallback normalization is inconsistent.

```diff
+ function normalizeIanaTimezone(iana) {
+   return TZ_MAP[iana] ? iana : 'UTC';
+ }
+
+ function resolveIanaFromPosix(posix) {
+   return POSIX_TO_IANA[posix] || normalizeIanaTimezone(getTimezoneConfig().iana);
+ }

- if (d.timezone) {
-   state.timezone = POSIX_TO_IANA[d.timezone] || getTimezoneConfig().iana;
+ if (d.timezone && !state.timezone) {
+   state.timezone = resolveIanaFromPosix(d.timezone);
    if (wizardTimezone) wizardTimezone.value = state.timezone;
  }

- state.timezone = POSIX_TO_IANA[String(val)] || getTimezoneConfig().iana;
+ state.timezone = resolveIanaFromPosix(String(val));
```

### 2. Implement the architecture-required manual fallback for unsupported zones

**File:** `firmware/data-src/wizard.html`, `firmware/data-src/wizard.js`  
**Issue:** Users in unmapped zones have no correct onboarding path.

```diff
+ <label for="wizard-timezone-manual">Manual POSIX Timezone</label>
+ <input id="wizard-timezone-manual" type="text" placeholder="e.g. PST8PDT,M3.2.0,M11.1.0" style="display:none">
```

```diff
+ var wizardTimezoneManual = document.getElementById('wizard-timezone-manual');
+ var detected = getTimezoneConfig();
+ var detectedSupported = !!TZ_MAP[detected.iana];
+ wizardTimezoneManual.style.display = detectedSupported ? 'none' : '';
```

### 3. Add real wizard coverage and test the deployable path

**File:** `tests/e2e/page-objects/WizardPage.ts`, `tests/e2e/mock-server/server.ts`, new `tests/e2e/tests/wizard.spec.ts`  
**Issue:** The feature is helper-only and the shipped `.gz` assets are untested.

```diff
- location: { lat: number; lon: number; radiusKm: number };
+ location: { lat: number; lon: number; radiusKm: number; timezone?: string };

  await this.configureLocation(
    config.location.lat,
    config.location.lon,
-   config.location.radiusKm
+   config.location.radiusKm,
+   config.location.timezone
  );
```

Also add a wizard spec that asserts:
- browser auto-detect preselects a mapped IANA zone
- imported POSIX values reverse-map to IANA
- the review step shows the chosen IANA
- the final `POST /api/settings` payload sends POSIX
- the mock/dev server can validate the deployable `firmware/data/*.gz` path, not only `data-src/`

---

**Review Actions:**
- Issues Found: 6
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

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
<var name="session_id">d4f1f469-bd08-4038-88f4-a234b25d02e1</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="1dffb869">embedded in prompt, file id: 1dffb869</var>
<var name="story_id">fn-3.2</var>
<var name="story_key">fn-3-2-wizard-timezone-auto-detect</var>
<var name="story_num">2</var>
<var name="story_title">2-wizard-timezone-auto-detect</var>
<var name="template">False</var>
<var name="timestamp">20260413_1914</var>
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