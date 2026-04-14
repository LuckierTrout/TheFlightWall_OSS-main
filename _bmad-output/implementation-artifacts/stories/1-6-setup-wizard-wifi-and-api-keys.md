# Story 1.6: Setup Wizard - WiFi & API Keys (Steps 1-2)

Status: done

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want to enter WiFi credentials and API keys through a mobile-friendly wizard,
so that I can configure the device from my phone.

## Acceptance Criteria

1. **Wizard entry point:** When a user connects to the `FlightWall-Setup` AP and opens the captive portal, `wizard.html` loads Step 1 with a visible progress label `Step 1 of 5`.

2. **WiFi scan behavior:** On Step 1 load, an async WiFi scan starts. If networks are found within 5 seconds, show a tappable SSID list. If the scan is empty or times out, show `No networks found - enter manually`. The `Enter manually instead` action must always be available.

3. **Step 1 validation and navigation:** Selecting a scanned SSID or entering SSID/password manually, then tapping `Next ->` with non-empty fields, advances to Step 2.

4. **Step 2 API key UX:** Step 2 shows three paste-friendly fields with `autocorrect="off"`, `autocapitalize="off"`, and `spellcheck="false"`. When all three are non-empty and the user taps `Next ->`, the wizard advances to Step 3.

5. **Back preserves values:** Tapping `<- Back` from any visited step preserves all values already entered.

## Tasks / Subtasks

- [x] Task 1: Replace the wizard placeholder with a real Step 1-2 shell (AC: #1, #3, #4, #5)
  - [x] Replace the placeholder `firmware/data/wizard.html.gz` with a mobile-first wizard page that includes a fixed progress header, a step content region, and bottom navigation.
  - [x] Scaffold step containers for all 5 steps so Step 2 can legally advance to a Step 3 placeholder without implementing Stories 1.7-1.8 inside this story.
  - [x] Keep the page single-column, `max-width: 480px`, with full-width controls on mobile except the Back/Next nav pair.
  - [x] Reference shared local assets for styling and behavior instead of inlining large CSS/JS blocks into the HTML.

- [x] Task 2: Add local static assets required by the wizard (AC: #1, #2, #4)
  - [x] Create `firmware/data/style.css.gz` for the shared design tokens and wizard layout.
  - [x] Create `firmware/data/common.js.gz` for small shared HTTP helpers used by the wizard now and the dashboard later.
  - [x] Create `firmware/data/wizard.js.gz` for step state, WiFi scan polling, validation, and navigation.
  - [x] Keep all assets plain vanilla HTML/CSS/JS with no bundler, no framework, and no CDN dependency.

- [x] Task 3: Implement Step 1 WiFi scanning and manual-entry flow (AC: #2, #3, #5)
  - [x] On initial load of Step 1, call the existing `GET /api/wifi/scan` endpoint and poll its current response shape until results arrive or 5 seconds elapse.
  - [x] Render scan states clearly: scanning, results list, and manual fallback.
  - [x] Make each returned SSID row tappable so it populates the SSID field without retyping.
  - [x] Keep manual SSID/password entry available at all times via `Enter manually instead`.
  - [x] Validate only on `Next ->`: both SSID and password must be non-empty before leaving Step 1.

- [x] Task 4: Implement Step 2 API key entry and cross-step state preservation (AC: #4, #5)
  - [x] Add fields for OpenSky Client ID, OpenSky Client Secret, and AeroAPI Key.
  - [x] Back these inputs with a single in-memory wizard state object using the exact config key names the firmware already accepts: `wifi_ssid`, `wifi_password`, `os_client_id`, `os_client_sec`, `aeroapi_key`.
  - [x] Preserve state when moving backward and forward by re-rendering from that in-memory object, not by relying on captive-portal browser storage.
  - [x] Advance from Step 2 to a Step 3 placeholder only after all three API fields are non-empty.

- [x] Task 5: Expose shared assets cleanly from the web server (AC: #1)
  - [x] Update `WebPortal` so the wizard page can load `style.css`, `common.js`, and `wizard.js` from LittleFS with gzip headers.
  - [x] Reuse the existing root-route mode switch from Story 1.5; do not create a second wizard entrypoint.
  - [x] Keep API routes unchanged unless a small response-shape fix is required for the documented Step 1 polling behavior.

- [x] Task 6: Verification (AC: #1-#5)
  - [x] Build firmware with `pio run`.
  - [ ] Upload filesystem assets with `pio run -t uploadfs` before device testing.
  - [ ] Verify on a phone-sized viewport that `GET /` in AP mode shows Step 1, progress text is correct, and navigation works without layout breakage.
  - [ ] Verify WiFi scan success path, empty/timeout fallback path, and Back preserving values from Step 2 to Step 1.

### Review Findings

- [x] [Review][Patch] Step 1 scan state never settles cleanly into manual fallback: an empty completed scan keeps polling until timeout and can re-trigger scans, and the in-flight poll path ignores `manualMode`, so clicking `Enter manually instead` can still be overridden by later scan callbacks; the fallback text also does not match the story’s required `No networks found - enter manually` copy [firmware/data-src/wizard.js:48]

## Dev Notes

### Previous story intelligence (1.5)

- `WebPortal` already owns `GET /`, `GET /api/settings`, `POST /api/settings`, `GET /api/status`, `POST /api/reboot`, and `GET /api/wifi/scan` in `firmware/adapters/WebPortal.{h,cpp}`. Story 1.6 should consume and extend that adapter rather than invent parallel web-serving code.
- The current WiFi scan API response shape is `{ ok, scanning, data }`, where `data` is an array of `{ ssid, rssi }`. Build Step 1 polling against that contract unless you make a tightly scoped fix in the same adapter.
- Story 1.5 already established the critical public config keys for later save flow: `wifi_ssid`, `wifi_password`, `os_client_id`, `os_client_sec`, `aeroapi_key`. Do not use `opensky_client_id` or `opensky_client_secret` in browser state or future JSON payloads.
- `firmware/data/wizard.html.gz` and `firmware/data/dashboard.html.gz` already exist as placeholders. Replace the wizard placeholder; leave the dashboard placeholder alone unless a shared asset reference needs a compatible update.

### Previous story intelligence (1.4)

- `WiFiManager` owns mode/state transitions (`AP_SETUP`, `CONNECTING`, `STA_CONNECTED`, `STA_RECONNECTING`, `AP_FALLBACK`). The wizard is only a UI on top of AP/AP fallback mode; it must not duplicate WiFi state-machine logic.
- WiFi scans are intentionally handled through `WebPortal`, not `WiFiManager`.
- AP/AP_STA scanning on ESP32 can be imperfect. The UX already accounts for this: 5-second timeout plus manual entry fallback. Do not block the page waiting indefinitely for scan results.

### Technical guardrails

- Use plain HTML/CSS/JS only. No React, no build tool, no npm dependency, no CDN assets. Captive portal setup must work offline except for the ESP32 itself.
- Keep Step 1 and Step 2 state entirely client-side until Story 1.7 implements the final save/connect flow. This story should not POST partial configuration to `/api/settings`.
- Preserve values with an in-memory JS state object and DOM rehydration. Do not rely on `localStorage` or `sessionStorage` for required wizard behavior in captive portal contexts.
- Prefer small helper functions in `common.js` and step-specific logic in `wizard.js`. Avoid duplicating fetch/error helpers inside each step handler.
- Maintain exact firmware-facing key names in JS state now so Story 1.7 can serialize directly without another mapping layer.

### UX guardrails

- Wizard flow stays linear and obvious: one visible step at a time, fixed progress text, and only one primary action per screen.
- The secondary action on Step 1 is `Enter manually instead`. It must remain reachable even if scan results are available.
- API credential fields must be paste-friendly: disable autocorrect, autocapitalize, and spellcheck.
- Validation should happen on `Next ->`, not on every keystroke. Use inline helper text and field styling for missing required values.
- Keep the experience optimized for a phone: single-column, touch-friendly tap targets, and no tiny side-by-side controls beyond the wizard nav row.

### What not to do

- Do not implement Step 3 location entry, geolocation, hardware fields, review, or `Save & Connect` in this story. Those belong to Story 1.7 and Story 1.8.
- Do not add new configuration keys or alternate JSON names.
- Do not move WiFi scan logic into `WiFiManager`.
- Do not embed all CSS/JS into `wizard.html`; the architecture already expects shared gzipped assets in LittleFS.
- Do not introduce dashboard-only features such as Leaflet, calibration, or the full toast system unless a tiny shared helper is genuinely needed.

### Architecture compliance

- Web assets live in `firmware/data/` and are uploaded to LittleFS as gzipped files.
- `WebPortal` remains the adapter boundary for browser traffic. Keep firmware web routing in `firmware/adapters/WebPortal.cpp`, not in `main.cpp`.
- Existing `build_src_filter` patterns already compile `adapters/*.cpp`; no PlatformIO source-filter change should be necessary.
- JSON naming stays `snake_case` and aligned with NVS/public API keys.

### File structure requirements

| File | Action | Notes |
|------|--------|-------|
| `firmware/data/wizard.html.gz` | MODIFY | Replace placeholder with the actual Step 1-2 wizard shell |
| `firmware/data/style.css.gz` | CREATE | Shared styles for wizard now, dashboard later |
| `firmware/data/common.js.gz` | CREATE | Shared HTTP/helper layer kept small and framework-free |
| `firmware/data/wizard.js.gz` | CREATE | Step navigation, scan polling, validation, state preservation |
| `firmware/adapters/WebPortal.h` | MODIFY if needed | Only if new static-serving helper declarations are required |
| `firmware/adapters/WebPortal.cpp` | MODIFY | Serve shared gzipped assets cleanly from LittleFS |

### Testing requirements

- `pio run` is required.
- `pio run -t uploadfs` is required before manual device verification because this story is asset-heavy.
- Manual verification should cover:
  - AP mode root route loads Step 1 with `Step 1 of 5`
  - scan results appear when available
  - 5-second timeout falls back to manual entry with the specified message
  - `Enter manually instead` is always available
  - Step 2 fields preserve values when navigating back
  - Step 2 advances to a Step 3 placeholder after valid input

### Project Structure Notes

- This story extends the existing Epic 1 split correctly: Story 1.5 provides the server and APIs, Story 1.6 provides the first two wizard steps, Story 1.7 finishes steps 3-5 and final save.
- There is no repo commit history yet, so there is no git pattern intelligence to follow beyond the current file layout and prior story artifacts.
- No `project-context.md` was found; architecture, epics, PRD, UX spec, and prior story files are the source of truth.

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` - Story 1.6, UX-DR4, UX-DR5]
- [Source: `_bmad-output/planning-artifacts/architecture.md` - Web asset layout, WebPortal boundary, JSON naming, LittleFS asset pattern]
- [Source: `_bmad-output/planning-artifacts/prd.md` - FR3, FR4, NFR5]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` - captive portal constraints, wizard flow, mobile layout, input behavior, WiFi scanner component]
- [Source: `_bmad-output/implementation-artifacts/1-5-web-portal-and-settings-api.md` - root-route behavior, scan endpoint, JSON key alignment]
- [Source: `_bmad-output/implementation-artifacts/1-4-wifi-manager-and-network-connectivity.md` - WiFiManager ownership, scan limitations, AP/AP fallback context]
- [Source: `firmware/adapters/WebPortal.cpp` - current `/api/wifi/scan` response contract and root-route implementation]
- [Source: `firmware/core/ConfigManager.h` - current public network field names and config structure]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Pre-existing build failure: `isSupportedDisplayPin` forward-reference in `ConfigManager.cpp` — fixed with forward declaration.

### Completion Notes List

- Replaced wizard.html.gz placeholder with full 5-step shell (Steps 1-2 functional, Steps 3-5 placeholders for Stories 1.7-1.8).
- Created shared style.css.gz with dark theme design tokens, mobile-first layout, form fields, scan result rows, wizard navigation.
- Created common.js.gz with FW.get/FW.post HTTP helpers for JSON API calls.
- Created wizard.js.gz with step navigation, WiFi scan polling (800ms interval, 5s timeout), SSID deduplication/sorting by signal strength, manual entry mode, in-memory state preservation using exact firmware key names (wifi_ssid, wifi_password, os_client_id, os_client_sec, aeroapi_key), validation on Next only, input error highlighting.
- Added WebPortal routes for /style.css, /common.js, /wizard.js serving gzipped LittleFS assets.
- Created firmware/data-src/ directory with uncompressed source files for future editing.
- Firmware builds successfully: 55.3% Flash, 15.7% RAM.
- LittleFS image builds with only .gz assets (no bloat from uncompressed sources).
- Existing unit tests compile and pass (no regressions).
- Manual device verification (uploadfs + phone viewport) deferred to user — requires physical ESP32 hardware.

### File List

- `firmware/data/wizard.html.gz` — MODIFIED (replaced placeholder with full wizard shell)
- `firmware/data/style.css.gz` — CREATED (shared design tokens and wizard layout)
- `firmware/data/common.js.gz` — CREATED (shared HTTP helpers)
- `firmware/data/wizard.js.gz` — CREATED (step navigation, scan polling, validation, state)
- `firmware/data-src/wizard.html` — CREATED (uncompressed source for future editing)
- `firmware/data-src/style.css` — CREATED (uncompressed source)
- `firmware/data-src/common.js` — CREATED (uncompressed source)
- `firmware/data-src/wizard.js` — CREATED (uncompressed source)
- `firmware/adapters/WebPortal.cpp` — MODIFIED (added static asset routes and _serveGzAsset helper)
- `firmware/adapters/WebPortal.h` — MODIFIED (added _serveGzAsset declaration)
- `firmware/core/ConfigManager.cpp` — MODIFIED (added forward declaration for isSupportedDisplayPin to fix pre-existing build failure)
