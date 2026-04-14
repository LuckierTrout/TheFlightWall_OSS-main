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
# Story 2.3: Network & API Settings with Reboot Flow

Status: review

Ultimate context engine analysis completed — comprehensive developer guide created.

## Story

As a user,
I want to update WiFi or API credentials and have the device reboot cleanly,
so that I can change credentials without reflashing.

## Acceptance Criteria

1. **Network + API card:** Given a **Network** (or **Network & API**) card on the dashboard with fields for `wifi_ssid`, `wifi_password`, `os_client_id`, `os_client_sec`, and `aeroapi_key`, when the user edits values and taps **Apply**, the client sends `POST /api/settings` with a JSON object containing **only the keys the user intended to change** (or a full network payload — **either is valid** as long as unknown keys are not invented). Empty password may need firmware-compat behavior (see Dev Notes).

2. **Reboot contract:** When the POST touches any **reboot-required** key, the JSON response includes **`reboot_required: true`**. The client shows an **amber** toast **exactly:** `Rebooting to apply changes...` (per epic), then invokes **`POST /api/reboot`** so NVS is flushed and the device restarts (same sequence as wizard handoff: settings, then reboot).

3. **Reboot timing:** After **`POST /api/reboot`**, the device sends the HTTP response first, then restarts **within ~1s** (existing `WebPortal` timer). Epic wording “within 3 seconds” is satisfied; do not regress the response-before-restart behavior.

4. **Happy path:** Given correct new credentials, after reboot the device reconnects (STA), LED progress behavior matches existing Story **1.8** coordinator + `WiFiManager`, and the dashboard is reachable again at the new or same IP / `flightwall.local` when mDNS works.

5. **Wrong WiFi credentials:** Given invalid STA credentials after apply + reboot, **`WiFiManager`** falls back to **AP / setup** per existing firmware; user can recover via wizard — **no new WiFi state machine** in this story.

6. **Hardware card:** Given a **Hardware** card with `tiles_x`, `tiles_y`, `tile_pixels`, `display_pin`, and (optional but recommended for parity with NVS) `origin_corner`, `scan_dir`, `zigzag`, when the user **Apply**s changes that include **`display_pin`** or **panel layout** keys, the response reports **`reboot_required: true`** and the same amber toast + **`POST /api/reboot`** flow runs.

## Tasks / Subtasks

- [x] Task 1: Dashboard markup — Network / API / Hardware cards (AC: #1, #6)
  - [x] Extend `firmware/data-src/dashboard.html` with card(s): use `.card`, labels consistent with wizard copy where possible.
  - [x] Inputs: `autocomplete="off"`, `autocapitalize="off"`, `spellcheck="false"` on secrets; password fields `type="password"`.
  - [x] **Apply** button(s): either one **Apply** per card or one grouped Apply — avoid accidental mixing of unrelated dirty state unless batching is intentional.

- [x] Task 2: `dashboard.js` — load, validate, apply (AC: #1–#3, #6)
  - [x] Hydrate network + hardware fields from `GET /api/settings` (`body.data`).
  - [x] On Apply: build payload from current fields; `FW.post('/api/settings', payload)`.
  - [x] If `res.body.ok && res.body.reboot_required`: `FW.showToast('Rebooting to apply changes...', 'warning')` then `FW.post('/api/reboot', {})` (empty body is fine).
  - [x] **Error handling:** non-ok POST → red toast with `error` message; do not call reboot.
  - [x] **Connection loss** after reboot request is expected — mirror `wizard.js` pattern (do not treat as hard failure if reboot already sent).

- [x] Task 3: Optional WiFi scan (nice UX, not strictly in epic AC)
  - [x] Reuse wizard pattern: `GET /api/wifi/scan` polling in **STA** mode to pick SSID — same API as AP wizard.

- [x] Task 4: Firmware alignment — `reboot_required` vs hardware keys (AC: **#6**, epic parity)
  - [x] **Today:** `REBOOT_KEYS` in `ConfigManager.cpp` are: `wifi_ssid`, `wifi_password`, `os_client_id`, `os_client_sec`, `aeroapi_key`, `display_pin`. **`tiles_x`, `tiles_y`, `tile_pixels`, `origin_corner`, `scan_dir`, `zigzag` are *not* reboot keys**, yet the **display task** may **`ESP.restart()`** on hardware dimension changes after hot apply.
  - [x] **Required alignment for this story:** Ensure **`POST /api/settings`** returns **`reboot_required: true`** when **any** applied key requires a reboot to safely apply, **including panel layout keys** referenced in the epic. Pragmatic approach approved by this story:
    1. Add **`tiles_x`, `tiles_y`, `tile_pixels`, `zigzag`, `scan_dir`, `origin_corner`** to **`REBOOT_KEYS`** (or centralize equivalent logic) so `applyJson` sets `reboot_required` and **`persistAllNow()`** runs immediately for those batches.
    2. **Remove** the **automatic `ESP.restart()`** from `displayTask` when hardware layout changes — the **nominal path** becomes: user Apply → settings persisted → client **`POST /api/reboot`** (same as WiFi/API). This avoids **double restart** and matches dashboard UX.
  - [x] Update **`firmware/test/test_config_manager`** expectations for which keys require reboot.
  - [x] **`pio run`** + native tests if available.

- [ ] Task 5: Manual verification (AC: #4, #5)
  - [ ] Change WiFi to a known-good AP → reboot → dashboard loads on STA.
  - [ ] Deliberate bad SSID/key → device enters setup/AP recovery (existing behavior).
  - [ ] Change `display_pin` or tiles → `reboot_required` in response, toast, LED progress after reboot.

## Dev Notes

### Story foundation (Epic 2)

- Depends on **2.1** (dashboard shell, toasts, `FW.*`) and **2.2** timing card (same dashboard file — **merge cleanly**; story 2.2 may already extend `dashboard.html` / `dashboard.js`).
- Out of scope: **System Health** polish (2.4), **factory reset** (2.5).

### Exact JSON keys (must match wizard + `ConfigManager`)

| Key | Type / notes |
|-----|----------------|
| `wifi_ssid` | string |
| `wifi_password` | string |
| `os_client_id` | string |
| `os_client_sec` | string |
| `aeroapi_key` | string |
| `tiles_x`, `tiles_y`, `tile_pixels` | uint |
| `display_pin` | uint — valid GPIO set matches wizard `VALID_PINS` in `wizard.js` and `ConfigManager` validation |
| `origin_corner`, `scan_dir`, `zigzag` | uint — **include** if exposed in Hardware card |

### `GET /api/settings` vs `POST`

- GET wraps values in `data`; POST returns `applied[]` + `reboot_required`. Password values come back from device — treat as sensitive; still populate form if firmware returns them (mask in UI).

### `POST /api/reboot` side effects

- `WebPortal` triggers optional **`onReboot`** callback (LED **“Saving config...”** via `main.cpp`). For dashboard reboot-after-network, that messaging is **acceptable**; do not bypass `POST /api/reboot` to avoid the callback unless product decides otherwise.

### Architecture compliance

- Single settings surface: **`POST /api/settings`** only — no duplicate network endpoints.
- **`WiFiManager`** and **LED progress** remain unchanged unless fixing the hardware **double-restart** alignment in Task 4.

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` — Story 2.3]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` — reboot toast, network section, router-style apply]
- [Source: `firmware/core/ConfigManager.cpp` — `REBOOT_KEYS`, `applyJson`, persist behavior]
- [Source: `firmware/adapters/WebPortal.cpp` — `/api/settings`, `/api/reboot` timing]
- [Source: `firmware/src/main.cpp` — `displayTask` hardware change / restart — **candidate for change per Task 4**]
- [Source: `firmware/data-src/wizard.js` — final `POST /api/settings` + `POST /api/reboot` sequence]
- [Source: `firmware/data-src/dashboard.html`, `dashboard.js` — extend]
- [Source: `firmware/adapters/WiFiManager.cpp` — STA failure / AP fallback]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6 (2026-04-02)

### Debug Log References

- `pio run` build: SUCCESS (21s, 55.4% flash, 15.7% RAM)
- No new compile warnings introduced by story changes

### Completion Notes List

- **Task 1:** Added "Network & API" card (wifi_ssid, wifi_password, os_client_id, os_client_sec, aeroapi_key) and "Hardware" card (tiles_x, tiles_y, tile_pixels, display_pin, origin_corner, scan_dir, zigzag) to `dashboard.html`. Separate Apply buttons per card to avoid mixing unrelated dirty state. All secret fields use `type="password"` with `autocomplete="off"`, `autocapitalize="off"`, `spellcheck="false"`.
- **Task 2:** Extended `dashboard.js` with `applyWithReboot()` function that handles the reboot contract: POST /api/settings → check `reboot_required` → amber toast "Rebooting to apply changes..." → POST /api/reboot. Connection loss after reboot is caught gracefully (mirrors wizard.js pattern). Non-ok responses show red toast. Hardware Apply validates GPIO pin against VALID_PINS before sending.
- **Task 3:** Added WiFi scan button ("Scan for networks") that polls `GET /api/wifi/scan` in STA mode, shows deduplicated SSID list sorted by signal strength, and populates wifi_ssid on selection. Reuses same API and polling pattern as wizard.
- **Task 4:** Added `tiles_x`, `tiles_y`, `tile_pixels`, `origin_corner`, `scan_dir`, `zigzag` to `REBOOT_KEYS` in `ConfigManager.cpp`. Removed automatic `ESP.restart()` from `displayTask` on hardware config change — the nominal path is now: user Apply → NVS persisted → client POST /api/reboot. Added `test_requires_reboot_hardware_layout_keys` test, updated `test_requires_reboot_hot_reload_keys` to remove `tiles_x` assertion. Build passes.
- **Task 5:** Manual verification — requires physical device. Marked as pending for user.
- CSS: Added `.apply-btn` and `.card .form-fields` styles. Added `.scan-row-inline` for dashboard scan area.

### File List

- firmware/data-src/dashboard.html (modified — added Network & API card, Hardware card)
- firmware/data-src/dashboard.js (modified — hydrate network/hardware, applyWithReboot, WiFi scan)
- firmware/data-src/style.css (modified — apply-btn, card form-fields, scan-row-inline styles)
- firmware/data/dashboard.html.gz (regenerated)
- firmware/data/dashboard.js.gz (regenerated)
- firmware/data/style.css.gz (regenerated)
- firmware/core/ConfigManager.cpp (modified — expanded REBOOT_KEYS with hardware layout keys)
- firmware/src/main.cpp (modified — removed automatic ESP.restart() from displayTask)
- firmware/test/test_config_manager/test_main.cpp (modified — added hardware reboot key test, updated hot-reload test)

### Change Log

- 2026-04-02: Story 2.3 implementation — Network & API settings card, Hardware settings card, reboot flow, WiFi scan, firmware REBOOT_KEYS alignment, removed displayTask double-restart

```

Diff:

```diff
See `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/code-review-2-3.diff`
```
