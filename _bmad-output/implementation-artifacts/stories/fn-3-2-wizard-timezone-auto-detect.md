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

### Review: 2026-04-13 (Synthesis Pass 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 5.2 → REJECT (resolved to APPROVED after fixes)
- **Issues Found:** 5 verified
- **Issues Fixed:** 5 (all applied in this synthesis pass)
- **Action Items Created:** 1 (deferred)

#### Review Follow-ups (AI)
- [ ] [AI-Review] LOW: No wizard E2E test specs exist — `tests/e2e/tests/` contains only `dashboard.spec.ts`; timezone auto-detect, import, selection, and save-payload paths are unasserted. Create `tests/e2e/tests/wizard.spec.ts` in a dedicated test story.
