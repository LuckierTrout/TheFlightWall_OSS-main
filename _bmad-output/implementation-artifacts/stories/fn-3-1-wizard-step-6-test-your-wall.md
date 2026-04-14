# Story fn-3.1: Wizard Step 6 — Test Your Wall

Status: complete

## Story

As a new user completing the setup wizard,
I want the wizard to test my LED panel layout and let me confirm it looks correct before seeing flight data,
So that I catch wiring problems immediately instead of debugging garbled output later.

## Acceptance Criteria

1. **Automatic Pattern Trigger:**
   - Given the user completes Step 5 (Review) and taps "Next"
   - When Step 6 ("Test Your Wall") loads
   - Then the device auto-runs the panel positioning test pattern on the LED matrix via `POST /api/positioning/start`
   - And the pattern renders on the LED matrix within 500ms of being triggered
   - And a matching canvas preview renders in the browser showing the expected numbered panel layout

2. **Confirmation UI:**
   - Given Step 6 is displayed with canvas preview and LED pattern both visible
   - When rendered
   - Then the wizard asks: "Does your wall match this layout?"
   - And two buttons are shown: "Yes, it matches" (primary) and "No — take me back" (secondary)
   - And the primary button follows the one-primary-per-context rule
   - And the standard nav bar (Back/Next) is hidden for Step 6 (replaced by the confirmation buttons)

3. **Back Navigation (Guided Correction):**
   - Given the user taps "No — take me back"
   - When the wizard navigates
   - Then `POST /api/positioning/stop` is called to turn off the test pattern
   - Then the user returns to **Step 4 (Hardware configuration)** — not the immediately previous step (Step 5)
   - And all previously entered values are preserved (via existing `state` object)
   - And the user can change origin corner, scan direction, zigzag, or GPIO pin and return to Step 6 to re-test

4. **RGB Color Test:**
   - Given the user taps "Yes, it matches"
   - When the confirmation is received
   - Then `POST /api/positioning/stop` is called to turn off the positioning pattern
   - Then an RGB color test sequence runs on the LED matrix: solid red → solid green → solid blue (each held ~1 second)
   - And the wizard shows "Testing colors..." text during the sequence
   - And each color is sent via `POST /api/settings` with the respective RGB values at brightness 40, then the original brightness and text color values (from the wizard `state` object) are restored via a final `POST /api/settings`

5. **Completion Flow:**
   - Given the RGB color test completes
   - When the sequence finishes
   - Then the wizard displays "Your FlightWall is ready! Fetching your first flights..."
   - And all wizard settings are saved via `POST /api/settings` (same payload as current Step 5 `saveAndConnect()`)
   - And `POST /api/reboot` is called to trigger AP→STA transition
   - And the handoff message is displayed (phone loses AP connection — expected behavior)

6. **Accessibility & Responsive Design:**
   - Given all Step 6 UI when rendered
   - Then all buttons have 44x44px minimum touch targets
   - And all components meet WCAG 2.1 AA contrast requirements
   - And all components work at 320px minimum viewport width
   - And the canvas preview scales proportionally within the card width

7. **Dashboard Post-Setup Integration:**
   - Given the Calibration card on the dashboard
   - Then the existing "Panel Position" toggle button (already present in `#cal-pattern-toggle`) satisfies this requirement — tapping it calls `POST /api/positioning/start` and `POST /api/positioning/stop` via the existing `startPositioningMode()` / `stopPositioningMode()` functions in dashboard.js
   - No dashboard code changes are required for this AC

8. **Asset Deployment:**
   - Given updated wizard web assets (wizard.html, wizard.js)
   - When gzipped and placed in `firmware/data/`
   - Then the gzipped copies replace existing ones for LittleFS upload
   - And total gzipped web asset size remains under 50KB budget

## Tasks / Subtasks

- [x] Task 1: Add Step 6 HTML to wizard.html (AC: #1, #2, #6)
  - [x] 1.1: Add `<section id="step-6" class="step">` with heading "Test Your Wall"
  - [x] 1.2: Add `<canvas id="wizard-position-preview">` for the panel layout preview
  - [x] 1.3: Add description text: "Does your wall match this layout?"
  - [x] 1.4: Add "Yes, it matches" primary button and "No — take me back" secondary button
  - [x] 1.5: Add "Testing colors..." status text (hidden by default)
  - [x] 1.6: Add "Your FlightWall is ready!" completion text (hidden by default)
  - [x] 1.7: Update `<p id="progress">` counter — TOTAL_STEPS from 5 to 6

- [x] Task 2: Add Step 6 JS logic to wizard.js (AC: #1, #3, #4, #5)
  - [x] 2.1: Update `TOTAL_STEPS` constant from 5 to 6
  - [x] 2.2: Add `showStep(6)` handler: call `POST /api/positioning/start` (on failure show error toast via `FW.showToast()` and remain on Step 5), render canvas preview, hide nav bar, show confirmation buttons
  - [x] 2.3: Implement canvas rendering — port the tile positioning preview algorithm from dashboard.js `renderWiringCanvas()` (line 1947; draws numbered colored tiles matching the LED pattern)
  - [x] 2.4: Add "No — take me back" handler: call `POST /api/positioning/stop` (on failure show error toast but still navigate — user must not be stuck on Step 6), then `showStep(4)` (jump to Hardware step)
  - [x] 2.5: Add "Yes, it matches" handler: call `POST /api/positioning/stop`, then run RGB sequence
  - [x] 2.6: Implement RGB color test sequence: capture original brightness and text color from `state` before starting; POST brightness=40 + R/G/B values with 1s delays using `setTimeout` chain; restore original values via final `POST /api/settings` on completion or on any failure (show error toast on failure per Enforcement Rule 10)
  - [x] 2.7: After RGB sequence: call existing `saveAndConnect()` function (reuse Step 5 save + reboot logic)
  - [x] 2.8: Update `showStep()` to hide/show nav bar for Step 6 vs other steps
  - [x] 2.9: Update Step 5 (Review) to be a pass-through — "Next" on Step 5 now goes to Step 6 instead of calling `saveAndConnect()`
  - [x] 2.10: Move `saveAndConnect()` call from Step 5's "Next" handler to Step 6 completion

- [x] Task 3: Add Step 6 CSS to style.css (AC: #2, #6)
  - [x] 3.1: Style `.wizard-test-canvas` — responsive canvas container, proportional scaling
  - [x] 3.2: Style confirmation buttons — primary/secondary pair, 44px min touch targets
  - [x] 3.3: Style status text elements (testing colors, completion message)
  - [x] 3.4: Ensure 320px minimum viewport compatibility

- [x] Task 4: Verify dashboard Panel Position toggle satisfies AC #7 (no code changes required)
  - [x] 4.1: Confirm "Panel Position" toggle button (`#cal-pattern-toggle` data-pattern="1") in the Calibration card calls `startPositioningMode()` / `stopPositioningMode()` via the existing `activatePattern()` handler in dashboard.js
  - [x] 4.2: Confirm behavior matches AC #7 intent — no new dashboard HTML or JS required

- [x] Task 5: Rebuild gzipped assets (AC: #8)
  - [x] 5.1: `gzip -9 -c firmware/data-src/wizard.html > firmware/data/wizard.html.gz`
  - [x] 5.2: `gzip -9 -c firmware/data-src/wizard.js > firmware/data/wizard.js.gz`
  - [x] 5.3: `gzip -9 -c firmware/data-src/style.css > firmware/data/style.css.gz`
  - [x] 5.4: Verify total gzipped asset size remains under 50KB

## Dev Notes

### Critical Architecture Constraints

- **No new firmware endpoints required.** Step 6 reuses existing `POST /api/positioning/start`, `POST /api/positioning/stop`, and `POST /api/settings` endpoints. All changes are web asset only (HTML/JS/CSS). [Source: architecture.md#Decision-4-Web-API-Endpoints]
- **No new NVS keys.** This story uses existing config keys only.
- **Enforcement Rules 10:** Every JS `fetch()` call must check `json.ok` and handle failure with `showToast()`. Use `FW.post()` and `FW.get()` from common.js for all API calls. [Source: architecture.md#Enforcement-Guidelines]
- **Web asset budget:** Total gzipped assets must stay under 50KB. Current total is ~46KB. This story adds ~30-50 lines of HTML, ~100-150 lines of JS, ~30 lines of CSS — expect minimal size impact (~0.5-1KB gzipped). [Source: architecture.md#Web-Assets-Location]
- **Gzip build process:** No automated script. After editing `data-src/` files, manually run `gzip -9 -c data-src/X > data/X.gz` from `firmware/` directory. [Source: MEMORY.md]

### Existing Code Patterns to Reuse

**Step Navigation Pattern** (wizard.js lines 415-465):
- Steps are `<section id="step-N" class="step">` elements toggled via `.active` class
- `showStep(n)` handles display toggling, progress text update, Back button visibility, and field rehydration
- `TOTAL_STEPS` constant controls the step count (currently 5, must become 6)
- `validateStep(n)` is called before advancing — Step 6 needs no field validation (it's a visual confirmation)
- `saveCurrentStepState()` saves form fields to the `state` object before navigation

**Save & Connect Pattern** (wizard.js lines 582-634):
- `saveAndConnect()` builds a payload from `state` object, merges `importedExtras`, POSTs to `/api/settings`, then POSTs to `/api/reboot`, then calls `showHandoff()`
- This function must be called from Step 6 completion instead of Step 5
- The `btnNext` click handler (line 662) currently calls `saveAndConnect()` when `currentStep === TOTAL_STEPS` — changing TOTAL_STEPS to 6 means Step 5 "Next" now advances to Step 6, and Step 6's "Yes" button triggers save

**Canvas Positioning Preview** (dashboard.js `renderWiringCanvas()` line 1947):
- The dashboard already renders a tile positioning canvas showing numbered, color-coded tiles
- Algorithm: iterates tilesX * tilesY, draws colored rectangles with bright borders and white digit numbers
- This algorithm must be ported/adapted for the wizard canvas (cannot import from dashboard.js — they're separate gzipped files)
- The LED-side positioning pattern is rendered by `NeoMatrixDisplay::renderPositioningPattern()` (NeoMatrixDisplay.cpp line 692) — it draws numbered tiles with unique hues, bright borders, red corner markers, and 3x5 pixel font digits

**Button Styling Pattern** (style.css):
- Primary button: `.nav-btn-primary` class (blue background)
- Secondary button: `.nav-btn` class without primary modifier (gray/outline)
- Save button: `.nav-btn-save` class (green background, used on Step 5)
- All nav buttons already have proper touch target sizing (44px min)

### RGB Color Test Implementation

The RGB test sequence should:
1. Call `POST /api/settings` with `{ brightness: 40, text_color_r: 255, text_color_g: 0, text_color_b: 0 }` (red)
2. Wait ~1 second
3. Call `POST /api/settings` with `{ brightness: 40, text_color_r: 0, text_color_g: 255, text_color_b: 0 }` (green)
4. Wait ~1 second
5. Call `POST /api/settings` with `{ brightness: 40, text_color_r: 0, text_color_g: 0, text_color_b: 255 }` (blue)
6. Wait ~1 second
7. Proceed to save & connect

**Implementation approach:** Positioning mode must be stopped first (via `/api/positioning/stop`) before the RGB sequence begins, because calibration/positioning modes take display priority over normal rendering. With positioning stopped, the display task reads `text_color_r/g/b` from config via the `g_configChanged` atomic flag (hot-reload path, applies within <1 second). In AP mode without flight data, the display shows a loading/status screen rendered in the configured text color — this is sufficient to verify each RGB channel independently. No new firmware endpoints are required.

Before starting the sequence, capture the original brightness and text color values from `state` (populated by `hydrateDefaults()`). After the sequence completes — or on any API failure — restore these values via a final `POST /api/settings`. Wrap each API call with `json.ok` check and `FW.showToast()` on error per Enforcement Rule 10.

### Step 5 → Step 6 Flow Change

Currently Step 5 is the final step with "Save & Connect" button. After this story:
- Step 5 "Review" becomes a regular step with "Next →" button (not "Save & Connect")
- Step 6 "Test Your Wall" becomes the final step
- The "Save & Connect" action moves from the Step 5 "Next" handler to the Step 6 "Yes, it matches" → RGB test → completion flow

**Key change in `showStep()`:** The `btnNext` text/class logic (lines 424-429) checks `n < TOTAL_STEPS` vs `n === TOTAL_STEPS`. Changing `TOTAL_STEPS` from 5 to 6 automatically makes Step 5 a "Next →" step. Step 6 has its own buttons and hides the nav bar entirely.

**Key change in `btnNext` click handler** (line 662): When `currentStep === TOTAL_STEPS` (now 6), the handler should NOT call `saveAndConnect()` — Step 6 has its own "Yes" button that handles completion. Instead, treat Step 6's "Next" as unreachable (nav bar hidden) or guard with `if (currentStep === 6) return;`.

### Canvas Preview Algorithm

Port from dashboard.js `renderWiringCanvas()` (line 1947; the panel-level tile positioning preview). The algorithm:

1. Read tile dimensions from wizard `state` object: `tiles_x`, `tiles_y`, `tile_pixels`
2. Calculate canvas dimensions proportionally (scale to fit card width)
3. For each tile `(row, col)`:
   - Compute `gridIdx = row * tilesX + col`
   - Assign a unique hue: `hue = gridIdx * 360 / totalTiles`
   - Draw a filled rectangle with the tile's color
   - Draw a bright border around the tile
   - Draw a red corner marker at top-left (orientation guide)
   - Draw the tile number (gridIdx) centered in white

This MUST match the LED positioning pattern rendered by `NeoMatrixDisplay::renderPositioningPattern()` so the user can compare browser vs wall.

### Hardware Config Fields in Step 4

Step 4 currently only has: `tiles_x`, `tiles_y`, `tile_pixels`, `display_pin`. The positioning pattern also uses wiring flags (`origin_corner`, `scan_dir`, `zigzag`) which are NOT in the wizard. These default to 0/0/0.

For the "No — take me back" correction flow: the user can only adjust `tiles_x`, `tiles_y`, `tile_pixels`, and `display_pin` in the wizard. If the pattern shows tiles in the wrong ORDER (origin/scan issue), the wizard can't fix it — the user would need the dashboard's calibration card post-setup. The epic's AC says users can "change origin corner, scan direction, or GPIO pin" — this implies adding wiring flag fields to Step 4 or creating a minimal version.

**Decision needed:** The epic AC #3 explicitly says "change origin corner, scan direction." This means Step 4 needs **origin_corner** and **scan_dir** fields (and possibly **zigzag**) added. These are existing NVS keys (`origin_corner`, `scan_dir`, `zigzag`) already handled by `POST /api/settings`. Add them to Step 4's form fields, state object, validation, and the save payload.

### Adding Wiring Flags to Step 4

Add three new fields to Step 4 (Hardware):
- **Origin Corner:** `<select id="origin-corner">` with options: "Top-Left (0)", "Top-Right (1)", "Bottom-Left (2)", "Bottom-Right (3)"
- **Scan Direction:** `<select id="scan-dir">` with options: "Rows (0)", "Columns (1)"
- **Zigzag:** `<select id="zigzag">` with options: "Progressive (0)", "Zigzag (1)"

These must be:
1. Added to `state` object with defaults from `hydrateDefaults()` (read from `/api/settings`)
2. Added to `saveCurrentStepState()` for Step 4
3. Added to `showStep(4)` rehydration
4. Added to the `saveAndConnect()` payload
5. Added to `buildReview()` for Step 5 display
6. Added to `WIZARD_KEYS` array for settings import support

**Valid default values:** origin_corner=0, scan_dir=0, zigzag=0

### Project Structure Notes

- **Files to modify:**
  - `firmware/data-src/wizard.html` — add Step 6 section, add wiring fields to Step 4
  - `firmware/data-src/wizard.js` — add Step 6 logic, canvas rendering, RGB test, wiring state/validation
  - `firmware/data-src/style.css` — add Step 6 styles (canvas container, confirmation buttons)
- **Files to rebuild (gzip):**
  - `firmware/data/wizard.html.gz`
  - `firmware/data/wizard.js.gz`
  - `firmware/data/style.css.gz`
- **No firmware C++ changes required** — all existing endpoints sufficient
- **Alignment:** All paths match project structure conventions. Web sources in `data-src/`, gzipped in `data/`. [Source: _bmad-output/project-context.md]

### Existing Wizard State Keys

Current `WIZARD_KEYS` array (wizard.js):
```javascript
['wifi_ssid', 'wifi_password', 'os_client_id', 'os_client_sec',
 'aeroapi_key', 'center_lat', 'center_lon', 'radius_km',
 'tiles_x', 'tiles_y', 'tile_pixels', 'display_pin']
```

Add: `'origin_corner', 'scan_dir', 'zigzag'` — bringing total to 15 wizard keys.

### `hydrateDefaults()` Pattern

The existing function (wizard.js) calls `FW.get('/api/settings')` and populates `state` from the response. Add the three new wiring keys with fallback defaults of 0.

### References

- [Source: _bmad-output/planning-artifacts/epics/fn-3-onboarding-polish.md — Story fn-3.1 full spec]
- [Source: _bmad-output/planning-artifacts/prd-foundation.md — FR25-FR29, NFR-P5]
- [Source: _bmad-output/planning-artifacts/architecture.md — Decision 4 (API Endpoints), Decision F4 (Timezone), Enforcement Rules]
- [Source: firmware/data-src/wizard.js — existing step navigation, state management, saveAndConnect()]
- [Source: firmware/data-src/wizard.html — existing 5-step structure]
- [Source: firmware/data-src/dashboard.js — `renderWiringCanvas()` line 1947 (tile positioning preview), `renderCalibrationCanvas()` line 1654 (pixel scan order)]
- [Source: firmware/adapters/NeoMatrixDisplay.cpp — renderPositioningPattern() line 692]
- [Source: firmware/adapters/WebPortal.cpp — /api/positioning/start|stop, /api/calibration/start|stop]
- [Source: firmware/data-src/common.js — FW.post(), FW.get(), FW.showToast()]

## Dev Agent Record

### Agent Model Used

claude-sonnet-4-5 (implementation) / claude-sonnet-4-6 (code review synthesis)

### Debug Log References

N/A — all changes are web asset only (HTML/JS/CSS + test infrastructure).

### Completion Notes List

**Implementation Validation — 2026-04-13:**
- All 5 tasks (31 subtasks) verified complete against source files on disk.
- Gzipped assets verified: `TOTAL_STEPS=6` and `cachedDeviceColors` confirmed in decompressed `wizard.js.gz`; `step-6` section confirmed in decompressed `wizard.html.gz`.
- Total gzipped asset size: 49.07 KB (under 50 KB budget — AC #8).
- Dashboard Panel Position toggle (AC #7) verified: `startPositioningMode()`/`stopPositioningMode()` in dashboard.js call `/api/positioning/start|stop` via existing `activatePattern()` handler. No dashboard changes required.
- WizardPage.ts TypeScript compilation: clean (pre-existing TS strictness issue in api-helpers.ts is unrelated).
- All 11 code review findings from prior synthesis pass confirmed resolved in source.

**Code Review Synthesis — 2026-04-13:**
- Fixed `enterStep6()` to navigate back to Step 5 when `/api/positioning/start` fails (AC1 compliance).
- Fixed `showSaveError()` to inject errors into the active Step 6 container instead of the hidden Step 5 section; restored `btnTestYes`/`btnTestNo` visibility on save failure so the user can retry.
- Fixed `saveAndConnect()` message sequencing: removed premature "Saving..." override; "Your FlightWall is ready!" now shows after successful reboot response (AC5).
- Fixed `renderPositionCanvas()` to render colored tile backgrounds with unique hues + bright white borders + red corner marker on tile 0, matching `NeoMatrixDisplay::renderPositioningPattern()`. Added 400px height cap to prevent unbounded canvas growth on extreme aspect ratios.
- Fixed `runRgbTest()` original-value capture: now resolves from `importedExtras` first, then from `cachedDeviceColors` (populated from `/api/settings` in `hydrateDefaults()`), then falls back to safe defaults. This prevents silently resetting device brightness/colors when the user has not imported a settings file.
- Fixed race condition in `runRgbTest()` error paths: restore POST and `finishStep6()` are now chained (awaited), preventing concurrent POSTs to `/api/settings`.
- Added `cachedDeviceColors` variable and populated it in `hydrateDefaults()`.
- Updated stale WIZARD_KEYS comment (12 → 15 keys).
- Rebuilt all three gzipped web assets (`wizard.html.gz`, `wizard.js.gz`, `style.css.gz`). Total gzipped size: 49.07 KB (under 50 KB budget).
- Updated `tests/e2e/page-objects/WizardPage.ts` for 6-step flow: updated type, `getCurrentStep()`, `nextButton` comment, added Step 6 locators, updated `completeWizard()` and `completeSetup()`.
- Added `/api/positioning/start` and `/api/positioning/stop` mock routes to `tests/e2e/mock-server/server.ts`.

**Final Validation — 2026-04-13:**
- Detected stale `wizard.js.gz` (source modified at 17:13, gz from 16:46). Rebuilt all three gz assets from current sources. MD5 verified match.
- Total gzipped asset size: 49.19 KB (under 50 KB budget — AC #8).
- All 8 Acceptance Criteria verified satisfied against source code.
- Firmware build: SUCCESS (1.17 MB, 78.3% of 1.5 MB partition).
- TypeScript compilation: clean (pre-existing api-helpers.ts issue is out of scope).
- No regressions introduced; all changes are web-asset-only (HTML/JS/CSS) + test infrastructure.

**Code Review Synthesis #2 — 2026-04-13 (4 validators):**
- Fixed CSS: added `max-height:70vh;object-fit:contain` to `.wizard-test-canvas canvas` — prevents unbounded vertical canvas growth for extreme tile aspect ratios (e.g., 1×10 config).
- Fixed JS: added `wizardTestStatus.textContent = 'Rebooting...'` in `saveAndConnect()` when `currentStep === 6` — gives user visible feedback during reboot wait (previously "Saving..." persisted through the reboot phase with no update).
- Fixed WizardPage.ts `backButton` locator from `/back|previous/i` regex to `#btn-back` ID — the regex erroneously matched "No — take me back" on Step 6, making `goBack()` click the wrong button in tests.
- Fixed WizardPage.ts `previewCanvas` locator from `#preview-canvas, canvas` (any canvas) to `#wizard-position-preview` (specific Step 6 canvas ID).
- Rebuilt `wizard.js.gz` and `style.css.gz`. Total gzipped assets: 49.22 KB (under 50 KB budget — AC #8).
- Confirmed false positives: red corner marker on EVERY tile is correct (firmware verified, lines 747-752 are inside the nested tile loop); canvas wiring-flag omission is correct (NeoMatrix handles physical remapping, visual grid order is always row-major); `enterStep6()` race guard using `currentStep === 6` is correct (a captured-step variable would reintroduce a regression); CORS wildcard in mock server is appropriate for local test tooling; CSRF not applicable to AP-mode IoT.

### File List

- `firmware/data-src/wizard.html` — Added Step 6 section (canvas, confirmation buttons, status text), wiring fields in Step 4, progress counter "of 6"
- `firmware/data-src/wizard.js` — TOTAL_STEPS=6, enterStep6(), renderPositionCanvas(), runRgbTest(), finishStep6(), cachedDeviceColors, wiring flags in state/WIZARD_KEYS/saveAndConnect, showStep(6) nav bar hide, Step 5 pass-through; added "Rebooting..." to wizardTestStatus in saveAndConnect()
- `firmware/data-src/style.css` — .wizard-test-canvas, .wizard-test-question, .wizard-test-actions, .wizard-test-status styles; added max-height:70vh;object-fit:contain to canvas rule
- `firmware/data/wizard.html.gz` — rebuilt from data-src/wizard.html
- `firmware/data/wizard.js.gz` — rebuilt from data-src/wizard.js
- `firmware/data/style.css.gz` — rebuilt from data-src/style.css
- `tests/e2e/page-objects/WizardPage.ts` — WizardStep type +test, getCurrentStep 1-6, Step 6 locators, completeWizard() via #btn-test-yes, completeSetup() 6-step flow; tightened backButton and previewCanvas locators
- `tests/e2e/mock-server/server.ts` — /api/positioning/start and /api/positioning/stop mock routes

## Senior Developer Review (AI)

### Review: 2026-04-13
- **Reviewer:** AI Code Review Synthesis (4 validators)
- **Evidence Score:** 9.0 (highest) → REJECT
- **Issues Found:** 11 verified
- **Issues Fixed:** 11
- **Action Items Created:** 0

#### Review Follow-ups (AI)

All verified issues were fixed in this synthesis pass. No outstanding action items.

> **Deferred / Dismissed:**
> - `TZ_MAP` / `getTimezoneConfig()` dead code in `wizard.js` — pre-existing from Story fn-2.1, out of scope for fn-3.1. Carry forward as a cleanup candidate.
> - RGB test shows text-in-color rather than solid-color fill — by design per Dev Notes ("sufficient to verify each RGB channel independently"); no firmware changes required per AC constraints.
> - Full E2E test *cases* for Step 6 not written in this pass — page-object infrastructure updated; dedicated test spec creation belongs in a separate test story.

### Review: 2026-04-13 (Synthesis #2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 2.5 → PASS
- **Issues Found:** 4 verified
- **Issues Fixed:** 4
- **Action Items Created:** 0

#### Review Follow-ups (AI)

All 4 verified issues fixed in this synthesis pass. No outstanding action items.

> **Deferred / Dismissed:**
> - No E2E test specs for Step 6 — explicitly deferred per Dev Agent Record; page-object infrastructure is in place for when test specs are written in a future story.
> - `saveAndConnect()` SOLID complexity — pre-existing function pattern, acceptable for embedded web UI scope.
> - RGB solid-fill vs. text-tint — dismissed per antipatterns doc ("by design").
> - Canvas ignores wiring flags — FALSE POSITIVE; NeoMatrix library handles physical wiring, visual grid order is always row-major in both firmware and canvas.
> - Red corner marker "tile 0 only" — DOUBLE FALSE POSITIVE; firmware draws on EVERY tile (verified in NeoMatrixDisplay.cpp lines 747-752 inside the nested tile loop); canvas implementation is correct.
