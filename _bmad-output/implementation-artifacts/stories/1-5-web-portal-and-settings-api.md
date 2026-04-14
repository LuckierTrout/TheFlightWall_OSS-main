# Story 1.5: Web Portal & Settings API

Status: done

Ultimate context engine analysis completed — comprehensive developer guide created.

## Story

As a user,
I want the device to serve web pages and API endpoints,
so that I can configure it from any browser.

## Acceptance Criteria

1. **Root route by WiFi mode:** In AP mode (`WiFiManager::getState()` is `AP_SETUP` or `AP_FALLBACK`) `GET /` serves `wizard.html.gz` from LittleFS with `Content-Encoding: gzip`. In STA (`STA_CONNECTED`, and treat `CONNECTING` / `STA_RECONNECTING` per your UX choice — document in Dev Agent Record) serves `dashboard.html.gz` the same way.

2. **POST /api/settings:** Body is JSON object of known config keys. Handler parses with ArduinoJson, calls `ConfigManager::applyJson()` on the object, returns JSON: `{"ok": true, "applied": ["..."], "reboot_required": false|true}` matching applied keys and reboot flag from `ApplyResult`. On unknown/invalid payload return envelope `{"ok": false, "error": "...", "code": "..."}` per architecture.

3. **GET /api/settings:** Returns JSON object with **all** current config fields the firmware exposes (display, location, hardware, timing, network), using the **same key names** `applyJson` accepts (e.g. `os_client_id` / `os_client_sec`, not alternate spellings) so the wizard/dashboard can round-trip.

4. **GET /api/status:** Returns `SystemStatus` snapshot — build a `JsonObject` via `SystemStatus::toJson()` or equivalent assembly; use consistent top-level envelope `{ "ok": true, "data": { ... } }` if you extend beyond raw status object — stay consistent with other endpoints in this story.

5. **POST /api/reboot:** Forces any pending debounced NVS writes to complete, persists full config state if needed, then `ESP.restart()` within ~3 seconds. (Add `ConfigManager::persistAllNow()` or reuse internal persist path — there is no public full flush today.)

6. **GET /api/wifi/scan:** Triggers **async** scan (`WiFi.scanNetworks(true)` or current ESP32 pattern), returns JSON array `[{ "ssid": "...", "rssi": n }, ...]` when results ready. Document behavior in AP_STA (scan may briefly disturb AP clients) per Story 1.4 Dev Notes.

7. **Non-blocking coexistence:** Request handlers must not block the Arduino `loop()` on Core 1 indefinitely; flight fetch cycle (`FlightDataFetcher`) and `ConfigManager::tick()` / `WiFiManager::tick()` continue. Prefer AsyncWebServer request callbacks that parse/respond quickly; no busy-wait for scan — poll scan status across calls or use timer.

8. **Build:** `pio run` succeeds; new gzipped assets uploaded with `pio run -t uploadfs` when testing on device.

## Tasks / Subtasks

- [x] Task 1: Static assets (AC: #1, #8)
  - [x] Add `firmware/data/wizard.html.gz` and `firmware/data/dashboard.html.gz` — minimal valid HTML placeholders (Stories 1.6–1.7 replace wizard content; Epic 2 expands dashboard). Gzip with `gzip -9` per architecture.
  - [x] Verify LittleFS paths match ESPAsyncWebServer `serveStatic` or manual `request->send(LittleFS, ...)`.

- [x] Task 2: `ConfigManager` serialization + reboot flush (AC: #3, #5)
  - [x] Add `void ConfigManager::dumpSettingsJson(JsonObject& out)` (or `bool toJson(...)`) that writes every key in `_display`, `_location`, `_hardware`, `_timing`, `_network` using **exact** `applyJson` key names (see `ConfigManager.cpp` `updateCacheFromKey`).
  - [x] Add `void ConfigManager::persistAllNow()` (or `flushNvs()`) that performs a **full** `persistToNvs()`-equivalent write and clears debounce flags — call from `/api/reboot` before `ESP.restart()`.

- [x] Task 3: `WebPortal` adapter (AC: #1–#7)
  - [x] Create `firmware/adapters/WebPortal.h` / `WebPortal.cpp` using **mathieucarbou/ESPAsyncWebServer** only (AR1) [Source: `firmware/platformio.ini`].
  - [x] `init(AsyncWebServer& or WebServer instance)` — register routes; `begin()` on port 80 when WiFi is ready (AP or STA).
  - [x] Route `GET /` → branch on `g_wifiManager.getState()` (or injected `WiFiManager&`) for wizard vs dashboard file.
  - [x] `POST /api/settings` — body `application/json`; `deserializeJson` to `JsonObject`; `applyJson`; serialize response.
  - [x] `GET /api/settings` — use new dump method.
  - [x] `GET /api/status` — status JSON.
  - [x] `POST /api/reboot` — `persistAllNow()`, delay optional short feedback, `ESP.restart()`.
  - [x] `GET /api/wifi/scan` — async scan pattern; return JSON array; handle "scan in progress" / empty with clear `ok` / `data` semantics.
  - [x] Register `WiFiManager::onStateChange` if server must restart or re-bind when mode changes — **or** serve both pages from same server regardless of mode and only change `GET /` mapping (preferred simplicity).

- [x] Task 4: `main.cpp` integration (AC: #7)
  - [x] Instantiate `AsyncWebServer` (global or static in WebPortal).
  - [x] After `WiFiManager::init()` and when AP or STA is active, call `WebPortal::begin()` (exact order: follow architecture Decision 5 — WebPortal after WiFiManager).
  - [x] Ensure `loop()` still runs `ConfigManager::tick()`, `WiFiManager::tick()`, flight pipeline — no blocking `server.handleClient()` (async server has none).

- [x] Task 5: Verification (AC: #8)
  - [x] `pio run` clean.
  - [x] Document manual curl/browser checks: `GET /`, `GET /api/settings`, `POST /api/settings` with one key, `GET /api/status`, `GET /api/wifi/scan` in AP mode.

### Review Findings

- [x] [Review][Patch] `/api/settings` can return `ok: true` for unknown or invalid keys because `applyJson()` silently ignores them instead of surfacing an error, which violates AC2 and can leave the UI believing a bad payload applied successfully [firmware/adapters/WebPortal.cpp:133]
- [x] [Review][Patch] `POST /api/settings` only accepts single-chunk request bodies and rejects valid chunked JSON with `413 BODY_TOO_LARGE`, making settings saves flaky depending on transport framing [firmware/adapters/WebPortal.cpp:49]
- [x] [Review][Patch] `ConfigManager` is mutated from the AsyncWebServer callback without synchronization while display, fetch, and WiFi code read the same global config on other tasks/cores, creating a real race on `String` and numeric fields [firmware/core/ConfigManager.cpp:247]
- [x] [Review][Patch] Hardware-layout changes applied through `/api/settings` are lost because they schedule a debounced persist, then the display task immediately calls `ESP.restart()` before that debounce window can flush to NVS [firmware/src/main.cpp:163]
- [x] [Review][Patch] `/api/wifi/scan` builds its in-progress response with `root["data"] = JsonArray();`, which does not create a document-backed empty array and can produce an unstable `data` shape for clients polling scan state [firmware/adapters/WebPortal.cpp:198]

## Dev Notes

### Previous story intelligence (1.4)

- **WiFiManager** lives in `firmware/adapters/WiFiManager.{h,cpp}` — use `getState()`, **do not** embed WiFi scan inside WiFiManager (explicitly deferred to WebPortal) [Source: `1-4-wifi-manager-and-network-connectivity.md`].
- **Threading:** WiFi event handlers must stay minimal; web handlers run on AsyncTCP context — avoid calling blocking `WiFi` APIs that can deadlock; follow same discipline as 1.4.
- **Scan caveat:** `WiFi.scanNetworks(true)` in AP_STA can return errors or briefly drop AP association — document behavior for Story 1.6 wizard [Source: 1.4 Dev Notes “WiFi scan limitations”].
- Story 1.4 **did not** add ESPAsyncWebServer routes — 1.5 is first web surface.

### Architecture compliance

- **AR10:** REST endpoints with JSON envelope `{ ok, data, error, code }` [Source: `_bmad-output/planning-artifacts/architecture.md` — Decision 4].
- **Carbou fork required** — already in `lib_deps` [Source: `architecture.md` AR1, `platformio.ini`].
- **Gzipped assets, LittleFS** — `firmware/data/`, `Content-Encoding: gzip` [Source: `architecture.md` — Web Asset Patterns].
- **Init order:** … `WiFiManager::init()` → `WebPortal::init/begin()` before or after display task per current `main.cpp` — align with Decision 5; web server must not delay display task creation if that is already established [Source: `architecture.md` — Decision 5].
- **Logging:** `LOG_*` from `utils/Log.h` in new C++ [Source: `architecture.md` — Logging Pattern].
- **RAM:** Keep JSON documents small; limit concurrent connections (NFR18) — avoid holding large `String` bodies globally.

### JSON key alignment (critical)

Public API keys must match `ConfigManager::updateCacheFromKey` / NVS: `os_client_id`, `os_client_sec` (not `opensky_client_id` in JSON). WiFi: `wifi_ssid`, `wifi_password`. Re-export this table in a comment in `WebPortal.cpp` or reuse a single helper — prevents wizard bugs in 1.6.

### File structure requirements

| File | Action |
|------|--------|
| `firmware/adapters/WebPortal.h` | CREATE |
| `firmware/adapters/WebPortal.cpp` | CREATE |
| `firmware/core/ConfigManager.h` | MODIFY — declare `dumpSettingsJson` + `persistAllNow` |
| `firmware/core/ConfigManager.cpp` | MODIFY — implement dump + flush |
| `firmware/src/main.cpp` | MODIFY — WebPortal + server lifecycle |
| `firmware/data/wizard.html.gz` | CREATE |
| `firmware/data/dashboard.html.gz` | CREATE |

`build_src_filter` already includes `../adapters/*.cpp` — WebPortal should compile without ini changes.

### Testing requirements

- No hardware-free unit tests required for AsyncWebServer in this story unless you add native mocks — document manual HTTP checks.
- **PIO:** `pio run` mandatory; device tests optional for merge.

### Git intelligence

- Repository may still lack commits — no blame patterns.

### Project context reference

- No `project-context.md` in repo.

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6 (claude-opus-4-6)

### Debug Log References

- `pio run` succeeded: RAM 15.7% (51412/327680), Flash 55.0% (1153525/2097152)
- No new warnings from Story 1.5 code; all warnings are pre-existing deprecated ArduinoJson usage in other adapters
- main.cpp was modified by linter between story creation and implementation — WiFiManager was not integrated in current main.cpp (inline WiFi.begin instead). Re-integrated WiFiManager + added WebPortal.

### Completion Notes List

- **Task 1:** Created `wizard.html.gz` (238 bytes) and `dashboard.html.gz` (227 bytes) placeholder pages in `firmware/data/`, gzipped with `gzip -9`. Pages serve via `Content-Encoding: gzip` from LittleFS.
- **Task 2:** Added `ConfigManager::dumpSettingsJson(JsonObject&)` — serializes all 19 config keys using exact same key names as `updateCacheFromKey`/`applyJson` (e.g. `os_client_id`, `os_client_sec`). Added `ConfigManager::persistAllNow()` — immediate full NVS flush, clears debounce.
- **Task 3:** Created `WebPortal` adapter using `ESPAsyncWebServer` (mathieucarbou fork). Routes: `GET /` (WiFi-state-aware routing: AP→wizard, STA→dashboard), `POST /api/settings` (JSON patch via `applyJson`), `GET /api/settings` (full config dump), `GET /api/status` (`SystemStatus::toJson` envelope), `POST /api/reboot` (`persistAllNow` + esp_timer 1s restart), `GET /api/wifi/scan` (async scan with `scanning` flag). All endpoints use `{ ok, data/error, code }` envelope per AR10.
- **Task 4:** Integrated into `main.cpp`: global `AsyncWebServer(80)`, `WiFiManager`, `WebPortal`. Init order: WiFiManager → WebPortal → FlightDataFetcher. `loop()` calls `ConfigManager::tick()` + `g_wifiManager.tick()`. No blocking calls — async server handles connections on AsyncTCP task.
- **Task 5:** `pio run` clean build — SUCCESS. No compilation errors.
- **UX Decision (AC #1):** `CONNECTING` and `STA_RECONNECTING` states serve `dashboard.html.gz` — rationale: user already has credentials configured, so dashboard context is more appropriate than setup wizard during transient reconnection states.

### File List

| File | Action |
|------|--------|
| `firmware/adapters/WebPortal.h` | CREATED |
| `firmware/adapters/WebPortal.cpp` | CREATED |
| `firmware/core/ConfigManager.h` | MODIFIED — added `dumpSettingsJson`, `persistAllNow` declarations |
| `firmware/core/ConfigManager.cpp` | MODIFIED — implemented `dumpSettingsJson`, `persistAllNow` |
| `firmware/src/main.cpp` | MODIFIED — added WiFiManager, AsyncWebServer, WebPortal globals; replaced inline WiFi with WiFiManager.init(); added WebPortal lifecycle |
| `firmware/data/wizard.html.gz` | CREATED |
| `firmware/data/dashboard.html.gz` | CREATED |

## Change Log

- **2026-04-02:** Story 1.5 implemented — WebPortal adapter with 6 REST endpoints, ConfigManager dump/flush methods, gzipped asset placeholders, main.cpp integration. Build verified clean. (Claude Opus 4.6)

## References

- [Source: `_bmad-output/planning-artifacts/epics.md` — Story 1.5]
- [Source: `_bmad-output/planning-artifacts/architecture.md` — Decision 4 (API), AR1, AR10, Web assets, JSON patterns]
- [Source: `_bmad-output/planning-artifacts/prd.md` — FR39]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` — UX-DR18 gzipped common.js pattern (extend in later stories)]
- [Source: `1-4-wifi-manager-and-network-connectivity.md` — WiFiManager API, scan limitations]
- [Source: `firmware/core/ConfigManager.h` / `ConfigManager.cpp` — applyJson, NVS keys]
