You are an Acceptance Auditor.

Review this diff against the spec below.

Check for:
- violations of acceptance criteria
- deviations from spec intent
- missing implementation of specified behavior
- contradictions between spec constraints and actual code

Output findings as a Markdown list.

Rules:
- Do not praise the code.
- If you find nothing, say `No findings.`
- Each finding should include:
  - a short severity label (`high`, `medium`, or `low`)
  - a one-line title
  - which acceptance criterion or constraint is violated
  - evidence from the diff
  - the impact

Spec:

```md
# Story 1.7: Setup Wizard - Location, Hardware & Review (Steps 3-5)

Status: review

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want to set location, confirm hardware, and review before connecting,
so that the device is configured correctly on the first attempt.

## Acceptance Criteria

1. **Step 3 location flow:** When the wizard advances to Step 3, the page offers a prominent `Use my location` action plus manual latitude, longitude, and radius inputs. If geolocation succeeds, latitude/longitude auto-fill. If permission is denied, unsupported, or unavailable, manual entry remains usable without blocking the flow.

2. **Step 3 validation and continuity:** Tapping `Next ->` on Step 3 only advances when latitude, longitude, and radius are valid. Going back and forward preserves all previously entered WiFi, API, and location values.

3. **Step 4 hardware defaults:** When Step 4 loads, the wizard shows hardware fields prefilled with the current/default config values (`tiles_x=10`, `tiles_y=2`, `tile_pixels=16`, `display_pin=25` on a fresh device) and shows the calculated resolution text based on the current inputs.

4. **Step 5 review UX:** When the wizard reaches Step 5, it renders a summary of WiFi, API keys, location, and hardware. Each section is tappable so the user can jump back to edit it, and the `Save & Connect` action is the single prominent primary button.

5. **Save and handoff flow:** When `Save & Connect` is tapped, the wizard sends the full configuration payload to `POST /api/settings`, confirms the save, then triggers the reboot/apply flow so the device can boot using the new network settings. The phone UI shows a clear handoff message telling the user to look at the LED wall for progress while the device leaves AP mode and reconnects in STA mode.

## Tasks / Subtasks

- [x] Task 1: Extend the existing wizard state and bootstrap data for Steps 3-5 (AC: #2, #3, #4, #5)
  - [x] Extend the current in-memory wizard state in `firmware/data/wizard.js` with exact firmware-facing keys: `center_lat`, `center_lon`, `radius_km`, `tiles_x`, `tiles_y`, `tile_pixels`, `display_pin`.
  - [x] On wizard startup, hydrate missing Step 3-4 defaults from `GET /api/settings` so the browser uses the same current/default values as `ConfigManager`, instead of duplicating magic numbers.
  - [x] Preserve all wizard values in memory when navigating between Steps 1-5; do not use `localStorage` or `sessionStorage`.
  - [x] Replace the current placeholder-pass behavior for Steps 3-5 with real validation and summary flow.

- [x] Task 2: Implement Step 3 location UI with best-effort geolocation and robust manual fallback (AC: #1, #2)
  - [x] Replace the Step 3 placeholder in `firmware/data/wizard.html` with a real location step containing a `Use my location` button, latitude input, longitude input, and radius input.
  - [x] Keep manual fields usable even if geolocation is denied, unavailable, or blocked by browser policy.
  - [x] Use browser geolocation as a progressive enhancement only: request current position on button tap, populate `center_lat` and `center_lon` on success, and show specific inline guidance on failure.
  - [x] Validate location only on `Next ->`: latitude within `-90..90`, longitude within `-180..180`, radius positive and non-zero.
  - [x] Add lightweight helper copy that the interactive map comes later in the dashboard, not in the captive-portal wizard.

- [x] Task 3: Implement Step 4 hardware entry with live resolution feedback (AC: #3)
  - [x] Replace the Step 4 placeholder in `firmware/data/wizard.html` with fields for `tiles_x`, `tiles_y`, `tile_pixels`, and `display_pin`.
  - [x] Prefill those fields from the hydrated wizard state so a first-boot device shows the known defaults from `ConfigManager`.
  - [x] Show derived resolution text such as `Your display: 160 x 32 pixels`, updating immediately as the hardware fields change.
  - [x] Validate only on `Next ->`: all values must be positive integers, and `display_pin` must remain compatible with the current `ConfigManager` validation rules.

- [x] Task 4: Implement Step 5 review and final save/apply flow (AC: #4, #5)
  - [x] Replace the Step 5 placeholder with a review screen that summarizes WiFi, API keys, location, and hardware from the in-memory wizard state.
  - [x] Make each review section tappable to jump back to its corresponding step for editing.
  - [x] Change the final primary action label from the placeholder flow to `Save & Connect`.
  - [x] On final submit, send one consolidated payload through the existing `POST /api/settings` route using the exact current public keys:
    `wifi_ssid`, `wifi_password`, `os_client_id`, `os_client_sec`, `aeroapi_key`, `center_lat`, `center_lon`, `radius_km`, `tiles_x`, `tiles_y`, `tile_pixels`, `display_pin`.
  - [x] Treat `reboot_required: true` as the expected success path, then call the existing `POST /api/reboot` route to reboot into the newly saved configuration.
  - [x] Replace the wizard UI with a browser-side handoff message such as `Configuration saved. Look at the LED wall for progress.` Do not try to keep the captive portal active while the device reboots and changes WiFi mode.

- [x] Task 5: Tighten shared web helpers and routing only where the final flow truly needs it (AC: #5)
  - [x] Extend `firmware/data/common.js` only if needed for reusable JSON error handling, settings POST, or reboot POST helpers.
  - [x] Keep `WebPortal` as the only browser-facing firmware adapter. Reuse the existing `/`, `/api/settings`, and `/api/reboot` routes; do not invent a parallel wizard save endpoint unless a hard blocker appears.
  - [x] Only touch `firmware/adapters/WebPortal.{h,cpp}` if the current static-asset serving or response shape blocks the Step 5 flow.

- [x] Task 6: Verification (AC: #1-#5)
  - [x] Build firmware with `pio run`.
  - [ ] Upload web assets with `pio run -t uploadfs` before manual device testing.
  - [ ] Verify Step 3 geolocation success path where the browser allows it.
  - [ ] Verify Step 3 denial/unavailable path falls back cleanly to manual entry without trapping the user.
  - [ ] Verify Step 4 shows the expected default hardware values and updates the resolution text live.
  - [ ] Verify Step 5 review sections jump back to the correct step and preserve previously entered values.
  - [ ] Verify `Save & Connect` sends the full payload, receives success, triggers reboot/apply flow, and shows the browser-side handoff message before the AP disappears.

## Dev Notes

### Story Foundation

- Story 1.7 completes the wizard flow started in Story 1.6 by replacing the current Step 3-5 placeholders with real location, hardware, and review behavior.
- This story is responsible for the browser-side save/apply handoff.
- Story 1.8 still owns the rich LED progress-state sequence after reboot. Story 1.7 should hand control to the existing firmware reboot/connect path, not recreate Story 1.8 inside the wizard.

### Current Implementation Surface

- The current wizard already exists as plain web assets, not placeholders in docs only:
  - `firmware/data/wizard.html`
  - `firmware/data/wizard.js`
  - `firmware/data/common.js`
  - `firmware/data/style.css`
- `wizard.html` already has real Step 1-2 markup and explicit placeholders for Steps 3-5.
- `wizard.js` already keeps Step 1-2 data in a single in-memory state object and currently treats Steps 3-5 as placeholder passes.
- `WebPortal::_handleRoot()` already serves `/wizard.html.gz` in docs, but the live asset surface in the repo is plain `wizard.html` plus `/style.css`, `/common.js`, and `/wizard.js`.

### Previous Story Intelligence (1.6)

- Build directly on the existing Step 1-2 shell. Do not rewrite the wizard from scratch or replace its navigation/state pattern with a different architecture.
- The current wizard already uses the exact browser-side network keys needed later: `wifi_ssid`, `wifi_password`, `os_client_id`, `os_client_sec`, `aeroapi_key`.
- The current asset pattern is plain HTML/CSS/JS under `firmware/data/`, loaded from:
  - `<link rel="stylesheet" href="/style.css">`
  - `<script src="/common.js"></script>`
  - `<script src="/wizard.js"></script>`
- The final story should preserve this asset-loading model unless you intentionally migrate the entire serving path consistently in the same implementation.

### Previous Story Intelligence (1.5)

- `WebPortal` already owns:
  - `GET /`
  - `GET /api/settings`
  - `POST /api/settings`
  - `GET /api/status`
  - `POST /api/reboot`
  - `GET /api/wifi/scan`
- `GET /api/settings` already returns the full current config snapshot via `ConfigManager::dumpSettingsJson()`. Use it to seed Step 3-4 defaults and avoid duplicating default values in browser code.
- `POST /api/settings` already accepts the exact config keys this story needs and returns `{ ok, applied, reboot_required }`.

### Previous Story Intelligence (1.4)

- `WiFiManager` owns AP/STA transitions. The wizard must not try to manipulate WiFi modes directly in browser code.
- The correct browser-side behavior after a successful save is to trigger the existing reboot/apply path and show a handoff message; after reboot, the firmware will re-enter with saved config and let `WiFiManager` move from AP to STA.

### Critical Technical Requirements

- Use the existing config/public API key names exactly:
  - `center_lat`, `center_lon`, `radius_km`
  - `tiles_x`, `tiles_y`, `tile_pixels`, `display_pin`
  - `wifi_ssid`, `wifi_password`, `os_client_id`, `os_client_sec`, `aeroapi_key`
- `display_pin`, WiFi credentials, and API credentials are reboot-required keys in the current config system. A successful final wizard submit should therefore expect `reboot_required: true`.
- The clean implementation path is:
  1. gather all wizard state in browser memory
  2. `POST /api/settings` once with the full payload
  3. on success, call `POST /api/reboot`
  4. replace the page with the handoff message
- `tiles_x`, `tiles_y`, and `tile_pixels` are currently hot-reload keys elsewhere, but in this wizard they should still be part of the same final consolidated payload.

### Latest Technical Information

- Modern browser geolocation APIs typically require a secure context (HTTPS). An ESP32 captive portal served over plain HTTP may not grant geolocation access in every browser even when `navigator.geolocation` exists.
- Treat geolocation as a best-effort enhancement, not a guaranteed path:
  - request it only on explicit user action (`Use my location`)
  - if it fails, show or keep manual fields immediately
  - never block progress on the geolocation API succeeding
- This caveat is especially important in captive-portal/browser-sheet contexts on iOS Safari and Android Chrome.

### UX Guardrails

- The wizard must stay linear: one visible step at a time, fixed progress text, single primary action.
- Step 3 should feel fast and forgiving:
  - `Use my location` as the optimistic primary helper
  - manual fields always available as the reliable fallback
- Step 4 should reduce uncertainty:
  - prefilled values from current/default config
  - live resolution text while editing
- Step 5 should maximize confidence:
  - summary sections clearly grouped
  - easy jump-back editing
  - one obvious `Save & Connect` button
- The browser-side completion message must explicitly tell the user to look at the LED wall for progress because the captive portal session will be disrupted by reboot/WiFi-mode change.

### Architecture Compliance

- Keep browser traffic inside `firmware/adapters/WebPortal.cpp`; do not move wizard-serving logic into `main.cpp`.
- Keep JSON names `snake_case` and aligned with `ConfigManager`.
- Keep web assets in `firmware/data/`.
- Preserve the current pattern of small shared helpers in `common.js` and step-specific logic in `wizard.js`.
- Reuse existing routes before adding new ones.

### File Structure Requirements

| File | Action | Notes |
|------|--------|-------|
| `firmware/data/wizard.html` | MODIFY | Replace Step 3-5 placeholders with real Location, Hardware, and Review markup |
| `firmware/data/wizard.js` | MODIFY | Extend wizard state, validation, review navigation, final save/reboot handoff |
| `firmware/data/common.js` | MODIFY if needed | Keep helpers small; add reusable settings/reboot helpers only if they reduce duplication |
| `firmware/data/style.css` | MODIFY | Add only the styles needed for the new steps and review screen |
| `firmware/adapters/WebPortal.cpp` | MODIFY only if required | Use only if current asset serving or API response handling blocks the final flow |
| `firmware/adapters/WebPortal.h` | MODIFY only if required | Only if new helper declarations are necessary |

### Testing Requirements

- `pio run` is required.
- `pio run -t uploadfs` is required before manual testing because this story changes web assets.
- Manual verification should cover:
  - Step 3 geolocation success on a browser that permits it
  - Step 3 manual fallback when permission is denied or geolocation is unavailable
  - Step 3 value preservation when navigating back to Steps 1-2 and forward again
  - Step 4 default values and live resolution text
  - Step 5 review summaries and tap-to-edit navigation
  - final `Save & Connect` consolidated payload
  - reboot/apply handoff message shown before AP drops

### What Not To Do

- Do not re-implement Steps 1-2.
- Do not add Leaflet, map tiles, or dashboard-only location UI here. The wizard uses text fields plus best-effort browser geolocation; the interactive map belongs later in the dashboard flow.
- Do not implement the full LED progress-state sequence from Story 1.8 in browser code.
- Do not invent alternate key names like `opensky_client_id` or `opensky_client_secret` in the payload.
- Do not use `localStorage`/`sessionStorage` as the source of truth for required wizard state.
- Do not create a second wizard save endpoint if `/api/settings` + `/api/reboot` can do the job.

### Project Structure Notes

- Planning docs describe gzipped asset serving, but the current repo surface for the wizard is plain `wizard.html`, `style.css`, `common.js`, and `wizard.js` under `firmware/data/`.
- For this story, extend the current working asset pattern unless you intentionally migrate the full wizard-serving path in one coherent change. Do not leave mixed plain/gzipped references behind.
- There is still no repo commit history to mine; rely on the planning artifacts, current code, and previous story files as the source of truth.
- No `project-context.md` was found.

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` - Story 1.7 acceptance criteria, UX-DR4, UX-DR6]
- [Source: `_bmad-output/planning-artifacts/architecture.md` - `/api/settings`, `/api/reboot`, key naming, WebPortal boundary, wizard asset placement]
- [Source: `_bmad-output/planning-artifacts/prd.md` - FR1, FR5, FR6, FR7, FR8, NFR5]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` - captive portal constraints, wizard flow, location UX, handoff UX]
- [Source: `_bmad-output/implementation-artifacts/1-6-setup-wizard-wifi-and-api-keys.md` - existing wizard shell, in-memory state expectations, shared-asset direction]
- [Source: `firmware/data/wizard.html` - current Step 3-5 placeholders and asset references]
- [Source: `firmware/data/wizard.js` - existing wizard state shape and navigation behavior]
- [Source: `firmware/data/common.js` - current shared fetch helper surface]
- [Source: `firmware/adapters/WebPortal.cpp` - existing `/`, `/api/settings`, `/api/reboot`, and `/api/wifi/scan` behavior]
- [Source: `firmware/core/ConfigManager.h` - current typed config structures and `reboot_required` contract]
- [Source: `firmware/core/ConfigManager.cpp` - public JSON keys, reboot-required behavior, default values]
- [Source: `firmware/adapters/WiFiManager.cpp` - WiFiManager owns AP/STA transitions]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- No git commit history is available yet in this repository.
- Browser geolocation in HTTP captive-portal contexts may fail due to secure-context requirements, so the story explicitly requires graceful manual fallback.
- Discovered source files live in `firmware/data-src/` (not `firmware/data/`) — gzipped copies in `firmware/data/` are what WebPortal serves.
- `pio run` build succeeded with no new errors (only pre-existing deprecation warnings in AeroAPIFetcher and FlightWallFetcher).

### Completion Notes List

- Story context created from live code plus BMAD planning artifacts.
- Guardrails added for the consolidated save/reboot handoff so the developer reuses existing firmware routes instead of inventing a parallel wizard apply path.
- Repo-variance note captured: current wizard assets are plain files in `firmware/data-src/`, gzipped to `firmware/data/`.
- Implementation complete: Steps 3-5 replaced from placeholders to full location/hardware/review flow.
- Wizard state extended with 7 new keys (`center_lat`, `center_lon`, `radius_km`, `tiles_x`, `tiles_y`, `tile_pixels`, `display_pin`).
- `hydrateDefaults()` fetches current config from `GET /api/settings` on wizard startup to prefill Step 3-4 fields.
- Step 3 uses `navigator.geolocation.getCurrentPosition()` as progressive enhancement with graceful fallback on denial/unavailable/unsupported.
- Step 4 provides live resolution text (`Your display: W x H pixels`) updating on input.
- Step 5 builds review cards for all 4 sections, each tappable to jump back to the corresponding step.
- Final `Save & Connect` sends consolidated 12-key payload to `POST /api/settings`, then `POST /api/reboot`, then shows handoff message.
- `common.js` and `WebPortal.cpp` required no changes — existing `FW.post()` and API routes were sufficient.
- CSS extended with styles for geolocation button, resolution text, review cards, Save & Connect button, and handoff screen.
- GPIO pin validation in browser mirrors `ConfigManager` supported pins list.

### Implementation Plan

1. Extended `wizard.js` state object with Step 3-4 firmware config keys
2. Added `hydrateDefaults()` to seed state from `GET /api/settings` on startup
3. Replaced `wizard.html` Step 3-5 placeholders with real form markup
4. Added Step 3 geolocation with `requestGeolocation()` (button-triggered, best-effort)
5. Added Step 4 hardware fields with live `updateResolution()` on input events
6. Added Step 5 `buildReview()` with tappable review cards that navigate back to steps
7. Implemented `saveAndConnect()`: POST settings → POST reboot → handoff message
8. Extended `style.css` with geo-btn, resolution-text, review-card, handoff styles
9. Gzipped all `data-src/` files to `data/` for WebPortal serving
10. Build verified with `pio run` — no new errors

### File List

- `firmware/data-src/wizard.html` — MODIFIED: replaced Step 3-5 placeholders with Location, Hardware, Review markup
- `firmware/data-src/wizard.js` — MODIFIED: extended state, hydration, validation, geolocation, review, save/reboot flow
- `firmware/data-src/style.css` — MODIFIED: added geo-btn, resolution-text, review-card, handoff, save-btn styles
- `firmware/data-src/common.js` — NOT MODIFIED (existing FW.get/FW.post sufficient)
- `firmware/data/wizard.html.gz` — REGENERATED from data-src
- `firmware/data/wizard.js.gz` — REGENERATED from data-src
- `firmware/data/style.css.gz` — REGENERATED from data-src
- `firmware/data/common.js.gz` — REGENERATED from data-src
- `firmware/adapters/WebPortal.cpp` — NOT MODIFIED (existing routes sufficient)
- `firmware/adapters/WebPortal.h` — NOT MODIFIED

## Change Log

- 2026-04-02: Implemented Steps 3-5 of setup wizard (location, hardware, review & save/connect handoff). Extended wizard state with 7 config keys, added hydration from GET /api/settings, geolocation progressive enhancement, live resolution display, review cards with tap-to-edit, and consolidated save/reboot/handoff flow. Build verified with `pio run`.

```

Diff:

```diff
See `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/code-review-1-7.diff`
```
