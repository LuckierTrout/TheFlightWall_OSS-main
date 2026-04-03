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
# Story 1.5: Web Portal & Settings API

Status: review

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
```

Diff:

```diff
See `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/code-review-1-5.diff`
```
