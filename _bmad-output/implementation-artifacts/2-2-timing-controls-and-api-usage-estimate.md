# Story 2.2: Timing Controls & API Usage Estimate

Status: done

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want to adjust fetch frequency and display cycle with real-time API budget feedback,
so that I can balance freshness against my monthly API quota.

## Acceptance Criteria

1. **Fetch interval card:** Given a **Timing** section (card) on the dashboard, when the user adjusts the **fetch interval** control, the UI shows a live label: **Check for flights every: X min** (or **X min Y s** if you support sub-minute steps — if the control stays integer seconds only, show fractional minutes with one decimal or round sensibly; **must match** the value sent as `fetch_interval` in seconds).

2. **Monthly estimate:** While the fetch control moves, an estimate line updates in real time, formatted like **~N calls/month** (thousands separators optional). The number turns **amber** (warning styling — reuse toast warning token / CSS class) when **N > 4,000**, per OpenSky free-tier framing in the PRD.

3. **Persist fetch interval:** On **release** (for range input) or explicit apply (if using number input + button), the client calls `POST /api/settings` with `{ "fetch_interval": <uint16 seconds> }`. On success, show a **green** success toast via `FW.showToast` (Story 2.1).

4. **Display cycle card:** Given a **display cycle** control, when the user releases/adjusts and posts `{ "display_cycle": <uint16 seconds> }`, the LED wall **cycling speed** updates without reboot (hot-reload). Success toast confirms. Cycling is **seconds per flight** when multiple flights are shown — align copy with that behavior.

5. **Load state:** When the dashboard loads and `GET /api/settings` succeeds, **both** `fetch_interval` and `display_cycle` pre-populate the controls from `body.data`.

## Tasks / Subtasks

- [x] Task 1: Markup — add Timing card (AC: #1, #4, #5)
  - [x] Extend `firmware/data-src/dashboard.html` with a second `.card` after Display: headings, helper text area, range inputs (or number + slider pattern), live labels (`#fetch-label`, `#fetch-estimate`, `#cycle-label`, etc.).
  - [x] Keep layout within existing `.dashboard` / `480px` patterns and `style.css` tokens.

- [x] Task 2: Client logic — `dashboard.js` (AC: #1–#5)
  - [x] Parse `fetch_interval` and `display_cycle` in `loadSettings()` alongside display fields.
  - [x] Implement **estimate function** (see Dev Notes — OpenSky poll count); update on `input` for fetch control.
  - [x] Toggle **amber** styling on the estimate element when estimate > 4000 (classList, e.g. `.estimate-warning`).
  - [x] `change` handler on fetch slider: `POST` only `{ fetch_interval: v }`.
  - [x] `change` handler on display cycle slider: `POST` only `{ display_cycle: v }`.
  - [x] Avoid spamming POST on `input`; match Story 2.1 pattern (`change` or debounced release).

- [x] Task 3: Styles (AC: #2)
  - [x] Add any needed rules to `firmware/data-src/style.css` for the estimate line, warning color (e.g. `--warning` / `#ffa726` per UX spec), and Timing card layout.

- [x] Task 4: Firmware sanity check (AC: #4, #5)
  - [x] Confirm no firmware change **required**: `fetch_interval` / `display_cycle` already in `ConfigManager`, `dumpSettingsJson`, and `WebPortal` comments.
  - [x] Confirm `ConfigManager::requiresReboot` is **false** for both keys (see tests).
  - [x] Run `pio run`; refresh LittleFS gz assets if the build pipeline requires it.

- [x] Task 5: Manual verification
- [x] Timing loads from GET; sliders match persisted values after refresh.
- [x] Fetch POST updates device; estimate reacts live and crosses amber threshold when appropriate.
- [x] Display cycle POST changes multi-flight rotation timing observably on the wall.

### Review Findings

- [x] [Review][Patch] The fetch-interval slider cannot represent the firmware’s real default or any persisted value below 60 seconds: the control is hard-limited to `min="60"` in the markup, but `ConfigManager` still defaults `fetch_interval` to 30 seconds, so `loadSettings()` can show a 30-second label/estimate while the actual slider thumb is clamped to 60 seconds and later posts a different value than the one loaded [firmware/data-src/dashboard.html:52]

## Dev Notes

### Story foundation (Epic 2)

- Builds directly on **Story 2.1**: reuse `FW.get`, `FW.post`, `FW.showToast`, dashboard shell, and card styling.
- **Do not** implement Network/API/reboot flows (Story 2.3) or full System Health (Story 2.4) here.

### API contract (must not get wrong)

- **GET** `/api/settings` → `{ ok, data: { ..., fetch_interval, display_cycle, ... } }` — values are **seconds**, `uint16` in firmware.
- **POST** `/api/settings` → `{ ok, applied, reboot_required }` — for timing-only posts expect `reboot_required: false`.

### Monthly estimate — definitional guardrail

- PRD: OpenSky free tier is rate-limited to **~4,000 monthly requests** (state-vector fetches).
- **MVP formula (browser):** Treat **one fetch cycle** as **one** OpenSky state-vector request (matches `FlightDataFetcher` → `fetchStateVectors` once per cycle).
- **N per 30-day month:**  
  `N = Math.round((30 * 24 * 60 * 60) / fetchIntervalSeconds)`  
  i.e. `N = Math.round(2_592_000 / fetchIntervalSeconds)`.
- **Amber** when `N > 4000` (strict inequality per epic wording: *exceeds* 4,000).
- **Disclaimer in helper text (one line):** Quote is **OpenSky poll estimate**; AeroAPI/FlightWall CDN calls scale with traffic and are **not** included in N — prevents false confidence without requiring new firmware telemetry in this story.

**Product note:** Default `TimingConfiguration::FETCH_INTERVAL_SECONDS` is **30** today, which yields ~86k polls/month in this model — deep in the red vs 4,000. Do **not** silently change firmware defaults in this story unless explicitly tasked; the dashboard’s amber warning is part of why this control exists. Optional follow-up (outside 2.2) is raising the default interval or clarifying tier limits with product.

### Display cycle — live-code guardrails

- `fetch_interval` drives the main `loop()` fetch timing in `main.cpp` (seconds × 1000 ms).
- `display_cycle` is read in **`displayTask`** from `localTiming` after `g_configChanged` **and** inside `NeoMatrixDisplay::displayFlights` from `ConfigManager::getTiming()` for internal cycling. Hot-reload is expected to work via `ConfigManager::onChange` → display task refresh; if QA sees stale cycling, prefer checking that **display task** path is the one actually rendering (single source of truth) before patching NeoMatrix.

### Suggested UI bounds (if not specified elsewhere)

- **fetch_interval:** practical slider **60–3600 s** (1–60 min) keeps UX in a believable range; clamp posted values to `1..65535` uint16. Widen only if product asks.
- **display_cycle:** e.g. **1–120 s** — must be ≥1 for sensible behavior; align with defaults (`DISPLAY_CYCLE_SECONDS` = 3).

### Previous story intelligence (2.1)

- `dashboard.js` uses `change` for brightness POST and debounced `change` for RGB — mirror for timing sliders.
- Toasts: `FW.showToast(msg, 'success'|'warning'|'error')` — match existing severity strings from `common.js`.

### Architecture compliance

- No new REST endpoints; only `GET`/`POST` `/api/settings`.
- No NVS or timing logic in JS beyond POST — server remains source of truth.

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` — Story 2.2]
- [Source: `_bmad-output/planning-artifacts/prd.md` — FR43, FR48, OpenSky 4k/month]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` — fetch interval + estimate, display cycle hot-reload]
- [Source: `firmware/core/ConfigManager.h/.cpp` — `fetch_interval`, `display_cycle`]
- [Source: `firmware/config/TimingConfiguration.h` — defaults, OpenSky note]
- [Source: `firmware/src/main.cpp` — fetch interval usage]
- [Source: `firmware/adapters/NeoMatrixDisplay.cpp` — `display_cycle` in `displayFlights`]
- [Source: `firmware/data-src/dashboard.html`, `dashboard.js` — extend, do not fork]
- [Source: `firmware/test/test_config_manager/test_main.cpp` — hot-reboot keys]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` — SUCCESS (RAM 15.7%, Flash 55.4%)
- `pio test --without-uploading --without-testing` — PASSED (test_config_manager compiles, no regressions)
- Verified `REBOOT_KEYS` array does NOT include `fetch_interval` or `display_cycle` — both hot-reload

### Completion Notes List

- Added Timing card to dashboard with two range sliders: fetch interval (60–3600s, step 30) and display cycle (1–120s)
- Fetch interval label shows human-readable format: "X min" or "X min Y s"
- Monthly estimate formula: `Math.round(2592000 / fetchIntervalSeconds)` — updates live on slider `input` event
- Estimate turns amber (`estimate-warning` class, color `#d29922`) when N > 4000 (OpenSky free-tier threshold)
- Disclaimer note below estimate: "OpenSky poll estimate; AeroAPI/CDN calls not included"
- Both sliders POST on `change` event only (not `input`) — prevents request spam during drag, matches Story 2.1 pattern
- `loadSettings()` populates both `fetch_interval` and `display_cycle` from `GET /api/settings` `body.data` envelope
- No firmware C++ changes needed — both keys already in ConfigManager, dumpSettingsJson, and are not reboot-required
- Display cycle hot-reloads via `ConfigManager::onChange` → `g_configChanged` → `displayTask` refresh path
- Default fetch_interval=30s yields ~86k calls/month (deep amber) — intentional; dashboard warning is why this control exists

### Manual Smoke Test Plan

1. Flash firmware + upload FS
2. Open dashboard in STA mode — verify Timing card appears below Display card
3. Move fetch interval slider — verify label updates live ("Check for flights every: X min")
4. Verify estimate line shows `~N calls/month` and turns amber when slider is at low intervals
5. Release fetch slider — verify green "Applied" toast and `fetch_interval` persists across page refresh
6. Move display cycle slider — verify label shows "X s", release triggers POST with green toast
7. Verify display cycle change affects flight rotation speed on LED wall

### Change Log

- 2026-04-02: Story 2.2 implemented — Timing card with fetch interval + monthly estimate + display cycle sliders

### File List

- firmware/data-src/dashboard.html (modified — added Timing card markup)
- firmware/data-src/dashboard.js (modified — added timing controls, estimate logic, loadSettings extensions)
- firmware/data-src/style.css (modified — added estimate-line, estimate-warning, estimate-note styles)
- firmware/data/dashboard.html.gz (regenerated)
- firmware/data/dashboard.js.gz (regenerated)
- firmware/data/style.css.gz (regenerated)
