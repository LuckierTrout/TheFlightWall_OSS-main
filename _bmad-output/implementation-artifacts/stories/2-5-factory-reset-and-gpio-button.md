# Story 2.5: Factory Reset & GPIO Button

Status: done

Ultimate context engine analysis completed — comprehensive developer guide created.

## Story

As a user,
I want to reset the device from the web UI or force setup mode via a hardware button,
so that I can recover from any configuration problem.

## Acceptance Criteria

1. **System card (dashboard):** At the **bottom** of the main dashboard (`dashboard.html`), add a **System** (or **Danger zone**) section containing a **Reset to Defaults** control styled as destructive (e.g. `.btn-danger` using existing or new CSS token).

2. **Inline confirmation (no modal):** When **Reset to Defaults** is tapped, replace or reveal **inline** copy: **“All settings will be erased…”** (full sentence per UX/product preference) with **Confirm** and **Cancel** actions. **Do not** use a blocking `alert()`, **`confirm()`**, or a full-screen modal — keep it **in-flow** in the card (epic).

3. **POST /api/reset:** When **Confirm** is tapped, call **`POST /api/reset`** (new route). The handler must **erase persisted config** for the FlightWall NVS namespace, restore in-RAM config to the same **compile-time defaults** as a fresh device (`WiFiConfiguration::WIFI_SSID` is already **`""`** — critical so `WiFiManager` chooses AP after reboot), then **restart** the ESP32 so the device comes up in **`AP_SETUP`** / wizard path.

4. **Reboot UX:** Mirror **`POST /api/reboot`** behavior where appropriate: send JSON success **before** restart so the browser can show a toast; expect connection loss after reset (same as wizard/reboot flows).

5. **Boot-held GPIO (default GPIO 0):** When the user **holds** the configured button **during boot** (detect in `setup()` **before** `WiFiManager::init()`), `WiFiManager` must enter **`AP_SETUP`** (**`FlightWall-Setup`**) **even if** valid WiFi credentials exist in NVS — i.e. **do not** start STA from stored credentials when this flag is set.

6. **LED copy for forced setup:** When forced from GPIO at boot, the LED must show **`Setup Mode — Forced`** (epic wording) via the existing display-message path (`queueDisplayMessage` / coordinator) once the display pipeline is live — same phrase family as other setup strings.

7. **Short-press while running:** When the device is in **normal operation** (STA connected or otherwise not in forced AP story path), a **short press** on the same GPIO shows the current **IP** on the LED for **5 seconds**, then normal flight display resumes. If STA is not connected / no IP, show a short **status message** (e.g. **No IP** or **Setup mode**) instead of a bogus address.

## Tasks / Subtasks

- [x] Task 1: `ConfigManager::factoryReset()` (AC: #3)
  - [x] `Preferences` open namespace **`flightwall`**, **`clear()`** all keys (or equivalent full erase), then reload RAM cache using the **same default-initialization path** as `init()` uses when NVS is empty (compile-time namespaces in `ConfigManager.cpp`).
  - [x] Clear debounce persist flags (`_persistPending`, `_persistScheduledAt`) so no stale NVS write runs after reset.
  - [x] **Scope:** **NVS / settings only** unless product explicitly expands to LittleFS logo wipe — epic text says **NVS erased**; do **not** delete LittleFS assets in this story unless the PRD is updated.

- [x] Task 2: `WebPortal` — `POST /api/reset` (AC: #3, #4)
  - [x] Register route; handler calls `ConfigManager::factoryReset()` then schedules restart (reuse **`esp_timer`** pattern from `_handlePostReboot` or shared helper to avoid duplication).
  - [x] JSON envelope: `{ ok: true, message: "..." }` consistent with other simple POSTs.
  - [x] Update **`WebPortal.cpp` header comment** route list.

- [x] Task 3: `WiFiManager` — forced setup at boot (AC: #5)
  - [x] Add an API consumed by `main.cpp`, e.g. **`void WiFiManager::init(bool forceApSetup)`** or **`static void setForceApSetup(bool)`** read inside `init()`.
  - [x] When forced: call existing **`_startAP("FlightWall-Setup")`** (or shared helper) and **skip** **`_startSTA`** regardless of `ConfigManager::getNetwork()`.

- [x] Task 4: `main.cpp` — early boot GPIO sample (AC: #5, #6)
  - [x] Before `g_wifiManager.init(...)`, configure **`GPIO_NUM_0`** (or a **named constant** e.g. `BOARD_BOOT_BUTTON_GPIO`) as **input pull-up**, sample **hold** for a documented window (e.g. **~500–1000 ms** while low) to distinguish boot-hold from noise.
  - [x] Pass **force AP** into `WiFiManager::init`.
  - [x] Set a flag / phase so that after display queue exists, **`Setup Mode — Forced`** is shown (one-shot, appropriate duration — coordinate with Story **1.8** queue semantics: **length-1 overwrite**, do not blast competing messages).

- [x] Task 5: `loop()` — short-press handler (AC: #7)
  - [x] Poll or attach interrupt with debouncing — **keep it simple** (millis debounce) to avoid ISR complexity.
  - [x] Detect **short press** vs long (long may no-op in STA to avoid confusing "held" behavior while already running).
  - [x] Call **`queueDisplayMessage("IP: " + ip, 5000)`** (or localized equivalent) **only** when STA has IP; duration **5000 ms** must suppress flight UI for that window per existing display-task behavior.

- [x] Task 6: Dashboard UI (AC: #1, #2, #3)
  - [x] Add markup to `firmware/data-src/dashboard.html` (bottom **System** card).
  - [x] `dashboard.js`: toggle confirmation row; on Confirm, **`FW.post('/api/reboot', {})`** → **wrong** — use **`FW.post('/api/reset', {})`**; handle errors with **`FW.showToast(..., 'error')`**; expect connection drop on success.

- [x] Task 7: Assets & build
  - [x] Extend **`style.css`** for danger button + inline confirm row if needed.
  - [x] Regenerate gzipped LittleFS assets per project convention; **`pio run`**.

- [x] Task 8: Tests (optional but valuable)
  - [x] Unit test: after `factoryReset()` + `init()`, NVS has no keys (or known empty state) and `getNetwork().wifi_ssid` length is 0.

### Review Findings

- [x] [Review][Patch] Factory reset can be undone by an in-flight persist because `factoryReset()` clears NVS outside the normal persistence path while `ConfigManager::tick()` may already be inside `persistToNvs()`. Fixed by serializing `factoryReset()` and `persistToNvs()` on the same config mutex and restoring compile-time defaults inside the locked reset path, so stale writes cannot repopulate `flightwall` after the clear. [firmware/core/ConfigManager.cpp:197]
- [x] [Review][Patch] `/api/reset` always reports success even when the reset did not actually erase NVS. Fixed by making `ConfigManager::factoryReset()` return a real success/failure result and having `_handlePostReset()` return `RESET_FAILED` instead of a false `{ ok: true }` response when NVS could not be opened. [firmware/adapters/WebPortal.cpp:296]

## Dev Notes

### Live-code gaps (do not assume features exist)

- **`POST /api/reset` is not implemented** today — routes are settings, status, reboot, wifi scan (`WebPortal.cpp`).
- **`WiFiManager`** has **no** forced-setup or GPIO integration.
- **Compile-time WiFi defaults** are **empty strings** — after NVS clear + RAM reload, **`WiFiManager::init()`** naturally enters AP when `wifi_ssid.length() == 0` — verify **`factoryReset()`** really restores that state in RAM **before** reboot (or reboot immediately after clear without relying on stale RAM).

### GPIO 0 / strapping cautions

- **GPIO 0** is an ESP32 **strapping** pin; holding it **low at reset** can enter **UART boot / flash** mode on some boards. The epic assumes **GPIO 0** as the default — document in code comments that **product hardware** must tolerate this (many devkits wire the “BOOT” button to GPIO0). If hold detection conflicts with flashing workflow, keep the **hold window** finite and only act when firmware is already running past ROM boot (runtime short-press uses normal digital read).

### Coordination with display task

- **Short IP flash** and **forced-setup message** must respect the **single-slot** display message queue: schedule **one** message at a time; avoid overlapping with startup progress unless intentionally superseding.

### Security posture (document only)

- Reset endpoint is **unauthenticated**, like the rest of the LAN-facing portal — acceptable for the product model today; do not add token auth unless a separate story requires it.

### Previous story intelligence

- **2.3:** `applyWithReboot()` pattern — reset flow is **settings erase + restart**, not `POST /api/settings`.
- **2.1 / 2.4:** Reuse toast + card patterns; health page stays diagnostic-only.

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` — Story 2.5, FR45, FR46]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` — inline confirm, danger actions]
- [Source: `firmware/core/ConfigManager.cpp` — NVS namespace `flightwall`, `persistAllNow`, init defaults]
- [Source: `firmware/config/WiFiConfiguration.h` — empty default SSID]
- [Source: `firmware/adapters/WiFiManager.cpp` — `init`, `_startAP`]
- [Source: `firmware/adapters/WebPortal.cpp` — reboot timer pattern]
- [Source: `firmware/src/main.cpp` — `queueDisplayMessage`, `setup` order]
- [Source: `firmware/data-src/dashboard.html`, `dashboard.js`]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

Build compiles cleanly (pre-existing deprecation warnings in AeroAPIFetcher/OpenSkyFetcher/FlightWallFetcher are unrelated). RAM: 15.7%, Flash: 55.7%.

### Completion Notes List

- **Task 1:** Added `ConfigManager::factoryReset()` — clears NVS `flightwall` namespace via `Preferences::clear()`, resets debounce flags, then calls `init()` to reload compile-time defaults. Since NVS is empty after clear, `loadFromNvs()` finds no keys and defaults remain.
- **Task 2:** Added `POST /api/reset` route in WebPortal. Handler calls `factoryReset()`, returns `{ ok: true, message: "..." }`, then schedules ESP restart via 1-second `esp_timer` (same pattern as reboot handler). Updated header comment route list.
- **Task 3:** Changed `WiFiManager::init()` signature to accept `bool forceApSetup = false`. When true, calls `_startAP("FlightWall-Setup")` immediately, skipping credential check and STA connection.
- **Task 4:** Added `BOARD_BOOT_BUTTON_GPIO` constant (`GPIO_NUM_0`). In `setup()`, before WiFi init, configures GPIO as input pull-up, samples for 500ms hold. If held, sets `g_forcedApSetup = true` and passes to `WiFiManager::init(true)`. After display queue is ready, queues "Setup Mode — Forced" message. Includes code comments about GPIO 0 strapping pin behavior.
- **Task 5:** Added millis-based button polling in `loop()` with 50ms debounce. Detects short press (≤1s) on release. Shows "IP: x.x.x.x" for 5000ms if STA connected, or "No IP — Setup mode" otherwise. Long presses are ignored during normal operation. Disabled during forced AP setup to avoid conflict.
- **Task 6:** Added System/Danger zone card at bottom of dashboard. "Reset to Defaults" button reveals inline confirmation with warning text, Confirm and Cancel buttons. Confirm calls `FW.post('/api/reset', {})`, handles connection loss gracefully with toast notifications.
- **Task 7:** Added `.btn-danger`, `.btn-cancel`, `.reset-row`, `.reset-warning`, `.reset-actions`, `.card-danger` CSS classes. Card has red border/heading. Regenerated gzipped assets.
- **Task 8:** Added `test_factory_reset_clears_nvs_and_restores_defaults` Unity test — verifies NVS is cleared and RAM returns to compile-time defaults after `factoryReset()`.

### File List

- firmware/core/ConfigManager.h (modified — added `factoryReset()` declaration)
- firmware/core/ConfigManager.cpp (modified — added `factoryReset()` implementation)
- firmware/adapters/WebPortal.h (modified — added `_handlePostReset` declaration)
- firmware/adapters/WebPortal.cpp (modified — added `POST /api/reset` route and handler, updated header comment)
- firmware/adapters/WiFiManager.h (modified — changed `init()` to `init(bool forceApSetup = false)`)
- firmware/adapters/WiFiManager.cpp (modified — added forced AP setup path in `init()`)
- firmware/src/main.cpp (modified — added GPIO boot button sampling, forced AP setup flag, short-press handler in loop)
- firmware/data-src/dashboard.html (modified — added System card with reset UI)
- firmware/data-src/dashboard.js (modified — added reset button JS logic)
- firmware/data-src/style.css (modified — added danger button and reset row styles)
- firmware/data/dashboard.html.gz (regenerated)
- firmware/data/dashboard.js.gz (regenerated)
- firmware/data/style.css.gz (regenerated)
- firmware/test/test_config_manager/test_main.cpp (modified — added factory reset test)

### Change Log

- 2026-04-02: Implemented Story 2.5 — Factory Reset & GPIO Button. All 8 tasks complete. Added `ConfigManager::factoryReset()`, `POST /api/reset` endpoint, GPIO boot-hold forced AP setup, short-press IP display, dashboard System card with inline reset confirmation, and unit test.
