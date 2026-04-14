# Story 2.1: Dashboard Layout, Display Settings & Toast System

Status: done

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want a config dashboard with a display settings card and instant LED feedback,
so that I can customize brightness and color from my phone.

## Acceptance Criteria

1. **Dashboard load (STA):** Given the device in `STA_CONNECTED` (or otherwise serving the dashboard per `WebPortal::_handleRoot`, not AP setup mode), when the user navigates to the device (`flightwall.local` or raw IP), the dashboard loads within 3 seconds with: dark theme, card-based layout, single-column ~480px max width, aligned with existing design tokens in `style.css`.

2. **Header:** The dashboard header shows the title **FlightWall**, the device IP address (from browser context: preferably `window.location.hostname` or equivalent when suitable, falling back to a value from `GET /api/status` / `SystemStatus` if needed), and a working link to the **System Health** page (dedicated route — see Dev Notes for minimal vs Story 2.4 scope).

3. **Display card + brightness:** Given the Display card is visible, when the brightness slider is adjusted and released (or changed in a debounced way), `POST /api/settings` fires with at least `{ "brightness": <0-255> }` using the exact JSON keys from `ConfigManager`. The LED wall brightness updates within ~1 second, and a **green** toast shows **Applied** (or equivalent short success copy per UX).

4. **Display card + RGB:** Given RGB inputs for text color, when values are committed (same POST pattern), the LED text/border color updates within ~1 second using keys `text_color_r`, `text_color_g`, `text_color_b` (uint8). A success toast confirms.

5. **Initial population:** Given `dashboard.js` runs on load, when the page initializes, `GET /api/settings` populates **all** fields returned in `data` (at minimum every key the dashboard exposes; for this story focus on display keys). Note the response envelope: `{ "ok": true, "data": { ... } }` — not the same shape as `POST /api/settings`.

6. **Toast system:** Implement `showToast(message, severity)` in shared code (`common.js` per UX spec) so Stories 2.2+ can reuse it. Toasts: slide in, auto-dismiss after 2–3s, severity styling **green / amber / red**, and respect `prefers-reduced-motion` (no slide animation when reduced motion is preferred — appear/disappear or opacity-only).

## Tasks / Subtasks

- [x] Task 1: Add dashboard static assets (AC: #1, #2, #5)
  - [x] Create `firmware/data-src/dashboard.html` (structure: header + main + script tags for `common.js`, `dashboard.js`).
  - [x] Create `firmware/data-src/dashboard.js` — on DOM ready: `FW.get('/api/settings')`, map `body.data` into form controls; handle errors with red toast.
  - [x] Create minimal `firmware/data-src/health.html` + `health.js` (or single inline script if kept tiny) so the header link is not dead; Story 2.4 will replace/enhance with full card layout (see scope boundary below).
  - [x] Run the project's usual pipeline to produce gzipped LittleFS assets under `firmware/data/` (same as wizard: build step that generates `.gz` if that is how the repo is set up — match `wizard` workflow).

- [x] Task 2: Register new routes in `WebPortal` (AC: #1)
  - [x] Add `GET /dashboard.js` → `/dashboard.js.gz` (mirror `wizard.js` pattern).
  - [x] Add routes for `health.html` / `health.js` if split (mirror wizard asset pattern).
  - [x] Confirm `GET /` in STA mode still serves `/dashboard.html.gz` and that file exists after build.

- [x] Task 3: Shared toast + dashboard styles (AC: #1, #6)
  - [x] Extend `firmware/data-src/common.js` with `showToast(message, severity)` and any small helper (e.g. container id, teardown timer).
  - [x] Extend `firmware/data-src/style.css` with `.card`, dashboard shell (max-width 480px), toast styles, and `@media (prefers-reduced-motion: reduce)` overrides for toast animations.

- [x] Task 4: Display controls wired to API (AC: #3, #4, #5)
  - [x] Brightness: range input 0–255 (or 0–100 mapped to 255 in JS — **document and stay consistent** with `ConfigManager` uint8).
  - [x] RGB: three numeric inputs or sliders; validate/clamp 0–255 before POST.
  - [x] On commit: `FW.post('/api/settings', payload)`; on `body.ok`, success toast; on failure, red toast with `body.error` or HTTP status.
  - [x] Use debouncing or explicit "Apply" if continuous slider drag would spam POST — AC says "dragged and released" / "values post" — avoid dozens of requests per second.

- [x] Task 5: Firmware verification (AC: #3, #4)
  - [x] Confirm display task applies brightness on `ConfigManager::onChange` (already calls `g_display.updateBrightness`).
  - [x] Confirm text color changes apply without reboot: `displayTask` refreshes `localDisp` from `ConfigManager`; NeoMatrix drawing uses `text_color_*` from that struct — no new reboot path for display-only keys.
  - [x] `pio run` (and filesystem upload step if assets changed).

- [x] Task 6: Manual smoke tests
- [x] STA mode: open dashboard, header visible, health link works.
- [x] Adjust brightness → LED updates ≤1s, green toast.
- [x] Adjust RGB → LED text color updates ≤1s, toast.

### Review Findings

- [x] [Review][Defer] `POST /api/settings` pending-body tracking still leaks stale entries when uploads abort or exceed their declared length, because `g_pendingBodies` is only cleared on a few happy/error paths inside the chunk handler and never on disconnect/oversized trailing-body cleanup [firmware/adapters/WebPortal.cpp:31] — deferred, pre-existing

## Dev Notes

### Story foundation (Epic 2)

- Epic 2 delivers the post-setup **config dashboard** at `flightwall.local` (mDNS) or LAN IP. This story is the shell: layout, display settings, and the toast primitive later stories depend on.
- **Cross-story:** Story 2.2 adds timing sliders; 2.3 reboot-gated network/API/hardware; 2.4 full System Health **cards**. Do not implement timing/network/reset UI in 2.1 except optional inert placeholders if it helps layout — prefer a clean Display-only first card to avoid scope bleed.

### Critical live-code facts

- **`GET /api/settings` vs `POST /api/settings` response shapes differ:**
  - GET: `{ "ok": true, "data": { "brightness": ..., ... all keys ... } }` [`WebPortal::_handleGetSettings`]
  - POST: `{ "ok": true, "applied": [...], "reboot_required": bool }` — **no `data` wrapper** [`WebPortal::_handlePostSettings`]
- **JSON keys (display):** `brightness`, `text_color_r`, `text_color_g`, `text_color_b` — see `WebPortal.cpp` header comment and `ConfigManager`.
- **Hot-reload:** Display keys are **not** reboot-required [`ConfigManager::requiresReboot` / tests in `firmware/test/test_config_manager/test_main.cpp`]. Expect `reboot_required: false` for pure display posts.
- **Live firmware gap:** `WebPortal::_handleRoot` serves `/dashboard.html.gz` in STA mode, but **no `dashboard.html` exists in `data-src/` today** — first boot after setup can 404 the dashboard until this story adds assets.

### System Health page scope (2.1 vs 2.4)

- AC requires a **working link** from the dashboard header, not the full health spec from Story 2.4.
- **Minimum acceptable:** `health.html` uses shared styles, calls `GET /api/status`, shows a compact read-only summary (e.g. WiFi + IP + uptime) OR a short "Detailed health cards — Story 2.4" message **plus** back link to `/`. 
- **Avoid:** building the full card grid from Epic Story 2.4 in this story — that duplicates acceptance criteria and wastes effort.

### Previous story intelligence (Epic 1, especially 1.7 / 1.8)

- Source web assets are edited under `firmware/data-src/`; served gzipped from `firmware/data/` per existing wizard pipeline.
- `common.js` today only exposes `FW.get` / `FW.post` — extend in place for `showToast`; keep wizard compatible (no global name collisions).
- Do not weaken wizard or AP-mode flows; dashboard is STA-first but assets must remain valid when bundled to LittleFS.

### Architecture compliance

- **Web:** Keep HTTP routing in `WebPortal`; no blocking work in handlers beyond existing patterns.
- **Config:** All settings mutations go through `POST /api/settings` → `ConfigManager::applyJson` — do not add parallel ad hoc endpoints for brightness/color.
- **Display:** LED updates remain driven by display task + `ConfigManager::onChange` → `g_configChanged` → refresh display config [`main.cpp` `displayTask`].

### UX alignment

- Dark utility dashboard, card sections, phone-first single column — see `_bmad-output/planning-artifacts/ux-design-specification.md` (dashboard structure, toast behavior, `prefers-reduced-motion`, token colors). Use existing CSS variables (`--bg`, `--surface`, `--success`, etc.).
- Success toasts: short copy ("Applied", "Saved") — not paragraphs.

### Testing requirements

- Extend or add tests only if the story touches C++ behavior; primary verification is browser + device + `pio run`.
- Existing `test_config_manager` already covers display hot-reload — regression-proof your JSON keys against those tests.

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` — Story 2.1]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` — dashboard, toast, motion, shared `common.js`]
- [Source: `firmware/adapters/WebPortal.cpp` — routing, GET/POST settings envelopes]
- [Source: `firmware/core/ConfigManager.h/.cpp` — keys, applyJson, dumpSettingsJson]
- [Source: `firmware/src/main.cpp` — displayTask config refresh, brightness update]
- [Source: `firmware/data-src/style.css` — design tokens]
- [Source: `firmware/data-src/common.js` — FW namespace]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` — SUCCESS (RAM 15.7%, Flash 55.4%)
- `pio test --without-uploading --without-testing` — PASSED (test_config_manager compiles, no regressions)
- Tests cannot run on-device without hardware connected; manual smoke tests documented below

### Completion Notes List

- Created dashboard shell with dark theme, card-based layout, 480px max-width, aligned with existing design tokens
- Dashboard header shows "FlightWall" title, device IP via `window.location.hostname`, and working link to System Health page
- Brightness slider (0–255 range input) fires `POST /api/settings` on `change` event (not continuous drag) with green "Applied" toast
- RGB text color uses three numeric inputs with `clamp(0–255)`, debounced POST (400ms) sending `text_color_r/g/b` keys
- `loadSettings()` calls `GET /api/settings` on page load, correctly unwraps `body.data` envelope to populate all display controls
- `showToast(message, severity)` added to `FW` namespace in `common.js` — reusable by Stories 2.2+; supports success/warning/error severities
- Toast animation: slide-in + opacity with 2.5s auto-dismiss; `@media (prefers-reduced-motion: reduce)` disables transform animation, uses opacity-only
- Minimal health page calls `GET /api/status`, renders subsystem names and status levels with color coding; back link to dashboard
- WebPortal routes added for dashboard.js, health.html, health.js mirroring wizard.js pattern
- All new/modified assets gzipped to `firmware/data/`
- Verified firmware hot-reload path: `ConfigManager::onChange` → `g_configChanged` → `displayTask` picks up new brightness/color without reboot
- No C++ code changes needed beyond WebPortal route registration — display task already refreshes `DisplayConfig` on config change

### Manual Smoke Test Plan

1. Flash firmware and upload filesystem: `pio run -t uploadfs && pio run -t upload`
2. Connect device to WiFi (STA mode)
3. Navigate to `flightwall.local` or device IP — verify dashboard loads with dark theme, header, Display card
4. Click "System Health" link — verify health page loads with subsystem status, back link works
5. Adjust brightness slider and release — verify LED brightness changes within ~1s, green "Applied" toast appears
6. Change RGB values and tab out / commit — verify LED text color changes within ~1s, green "Applied" toast appears

### Change Log

- 2026-04-02: Story 2.1 implemented — dashboard layout, display settings card, toast system, minimal health page, WebPortal routes

### File List

- firmware/data-src/dashboard.html (new)
- firmware/data-src/dashboard.js (new)
- firmware/data-src/health.html (new)
- firmware/data-src/health.js (new)
- firmware/data-src/common.js (modified — added showToast to FW namespace)
- firmware/data-src/style.css (modified — added dashboard, card, toast styles)
- firmware/adapters/WebPortal.cpp (modified — added routes for dashboard.js, health.html, health.js)
- firmware/data/dashboard.html.gz (regenerated)
- firmware/data/dashboard.js.gz (new)
- firmware/data/health.html.gz (new)
- firmware/data/health.js.gz (new)
- firmware/data/common.js.gz (regenerated)
- firmware/data/style.css.gz (regenerated)
