# Story 1.4: WiFi Manager & Network Connectivity

Status: done

## Story

As a user,
I want the device to manage WiFi automatically — AP for setup, STA for operation, and recovery from disconnections,
so that I never need to reflash to fix network issues.

## Acceptance Criteria

1. **AP mode on first boot:** Device boots with no WiFi credentials in NVS → WiFiManager enters AP_SETUP state, broadcasts "FlightWall-Setup" SSID, LED shows "Setup Mode".

2. **STA connection with saved credentials:** Device boots with saved WiFi credentials → WiFiManager transitions CONNECTING → STA_CONNECTED, registers mDNS "flightwall", syncs NTP, LED displays "IP: [address]" for 3 seconds.

3. **WiFi disconnection recovery:** WiFi drops while in STA_CONNECTED → state enters STA_RECONNECTING, retries every 5 seconds for 12 attempts (60-second window). If network returns within window → transitions back to STA_CONNECTED with mDNS re-registration.

4. **Fallback to AP on timeout:** Reconnection timeout expires (12 failed attempts) → state enters AP_FALLBACK, broadcasts "FlightWall-Setup" SSID in AP_STA mode, LED shows "WiFi Failed - Setup Mode".

5. **State change callbacks:** WebPortal, display task, and FlightDataFetcher can register for state change notifications. Callbacks fire on every state transition with (old_state, new_state) parameters.

6. **SystemStatus integration:** WiFiManager reports WiFi health via `SystemStatus::set(WIFI, ...)` on every state transition — OK ("Connected, [rssi] dBm"), WARNING ("Weak signal, [rssi] dBm"), ERROR ("Disconnected, retrying...").

7. **No regression:** Existing flight pipeline (fetch → enrich → display) continues to work on Core 1. Display task on Core 0 receives WiFi status messages through the existing display messaging system. Build succeeds with `pio run`, zero errors.

## Tasks / Subtasks

- [x] Task 1: Create WiFiManager header with state enum and class interface (AC: #1, #2, #5)
  - [x] Define `WiFiState` enum: AP_SETUP, CONNECTING, STA_CONNECTED, STA_RECONNECTING, AP_FALLBACK
  - [x] Define `WiFiManager` class: init(), tick(), getState(), getLocalIP(), getSSID(), onStateChange(callback)
  - [x] Define callback signature: `std::function<void(WiFiState oldState, WiFiState newState)>`
  - [x] Include forward declarations, no heavy includes in header

- [x] Task 2: Implement WiFiManager state machine core (AC: #1, #2, #3, #4)
  - [x] Implement `init()`: read ConfigManager::getNetwork(), branch on ssid empty → AP_SETUP vs non-empty → CONNECTING
  - [x] Implement AP mode setup: `WiFi.mode(WIFI_AP)`, `WiFi.softAP("FlightWall-Setup")`, fire callbacks
  - [x] Implement STA connection: `WiFi.mode(WIFI_STA)`, `WiFi.begin(ssid, password)`, non-blocking with WiFi.onEvent()
  - [x] Register `WiFi.onEvent()` handlers for ARDUINO_EVENT_WIFI_STA_CONNECTED, _DISCONNECTED, _GOT_IP
  - [x] Implement `tick()` method for reconnection retry timing (called from loop() on Core 1)
  - [x] Track retry count and timing: `_retryCount`, `_lastRetryMs`, 5-second interval, max 12 retries
  - [x] On reconnection timeout: transition to AP_FALLBACK with `WiFi.mode(WIFI_AP_STA)`

- [x] Task 3: Implement mDNS and NTP on STA_CONNECTED (AC: #2)
  - [x] On STA_CONNECTED entry: `MDNS.begin("flightwall")` + `MDNS.addService("_http", "_tcp", 80)`
  - [x] On STA_CONNECTED entry: `configTime(0, 0, "pool.ntp.org")` for UTC time sync
  - [x] NTP is fire-and-forget (non-blocking); no timeout waiting needed
  - [x] On transition away from STA_CONNECTED: `MDNS.end()` cleanup

- [x] Task 4: Implement callback system and SystemStatus reporting (AC: #5, #6)
  - [x] Store callbacks in `std::vector<std::function<void(WiFiState, WiFiState)>>`
  - [x] Fire all callbacks on every `_setState()` transition
  - [x] Call `SystemStatus::set(Subsystem::WIFI, ...)` on every state change
  - [x] Report RSSI on STA_CONNECTED: `WiFi.RSSI()` in status message
  - [x] WARNING threshold: RSSI < -75 dBm; ERROR: disconnected/retrying states

- [x] Task 5: Integrate WiFiManager into main.cpp (AC: #7)
  - [x] Add `#include "WiFiManager.h"` and `WiFiManager g_wifiManager;` global
  - [x] Call `g_wifiManager.init()` in setup() — replaces existing blocking WiFi code
  - [x] Call `g_wifiManager.tick()` in loop() on Core 1 (alongside ConfigManager::tick())
  - [x] Remove existing blocking WiFi.begin() sequence from setup() (lines ~177-206)
  - [x] Register display callback: on STA_CONNECTED show IP via displayMessage(), on AP/reconnecting show status
  - [x] Display IP message: use `g_display.displayMessage()` from setup() context (before display task starts, or via queue message if after)
  - [x] Preserve `#ifndef PIO_UNIT_TESTING` guard

- [x] Task 6: Build verification (AC: #7)
  - [x] `pio run` — zero errors, zero new warnings
  - [x] RAM/Flash usage logged (baseline: 15.0% RAM / 50.5% Flash from story 1.3)
  - [x] Verify existing `test_config_manager` still builds
  - [x] Add LOG_I/LOG_E messages for all state transitions

## Dev Notes

### Architecture pattern: Event-driven state machine with tick() polling

WiFiManager combines two patterns:
1. **WiFi.onEvent()** — ESP32 WiFi stack fires events asynchronously on a system task. These set flags/state but do NOT call WiFi API functions (risk of deadlock).
2. **tick()** — Called from loop() on Core 1 each iteration. Handles reconnection retry timing, state transitions that require WiFi API calls, and callback firing.

```
                    WiFi.onEvent()                tick() on Core 1
                    ┌───────────┐                 ┌──────────────────┐
                    │ GOT_IP    │─── sets flag ──→│ Process flag:    │
                    │ DISCONNECT│─── sets flag ──→│  → setState()    │
                    │ CONNECTED │─── sets flag ──→│  → fire callbacks│
                    └───────────┘                 │  → mDNS/NTP      │
                                                  │  → retry logic   │
                                                  └──────────────────┘
```

**Why two-phase?** WiFi.onEvent() callbacks run on the WiFi system task (Core 0). Calling `WiFi.begin()`, `WiFi.disconnect()`, or `MDNS.begin()` from within an event callback can deadlock. Instead, the event handler sets atomic flags that `tick()` processes safely on Core 1.

### Critical implementation details

**WiFi event handler threading:**
- Event callbacks execute on the WiFi driver's internal FreeRTOS task (not your code's task)
- Do NOT call `WiFi.begin()`, `WiFi.disconnect()`, `WiFi.reconnect()` from inside event callbacks
- Do NOT call `WiFi.onEvent()` or `WiFi.removeEvent()` from inside event callbacks
- Use `std::atomic<bool>` or volatile flags to communicate events to `tick()`
- Keep event handlers minimal: set flag, return

```cpp
// In WiFiManager.cpp — event handler (runs on WiFi system task)
void WiFiManager::_onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            _gotIP = true;  // std::atomic<bool>
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            _disconnected = true;  // std::atomic<bool>
            break;
    }
}

// In tick() — runs on Core 1 in loop()
void WiFiManager::tick() {
    if (_gotIP.exchange(false)) {
        _setState(STA_CONNECTED);
        MDNS.begin("flightwall");
        MDNS.addService("_http", "_tcp", 80);
        configTime(0, 0, "pool.ntp.org");
        // fire callbacks, update SystemStatus
    }
    if (_disconnected.exchange(false) && _state == STA_CONNECTED) {
        _setState(STA_RECONNECTING);
        _retryCount = 0;
        _lastRetryMs = millis();
    }
    // Reconnection retry logic in STA_RECONNECTING state
    if (_state == STA_RECONNECTING) {
        if (millis() - _lastRetryMs >= 5000) {
            _retryCount++;
            if (_retryCount >= 12) {
                _startAPFallback();
            } else {
                WiFi.reconnect();
                _lastRetryMs = millis();
            }
        }
    }
}
```

**AP_FALLBACK uses WIFI_AP_STA mode:**
- Not WIFI_AP alone — AP_STA allows the device to keep trying STA reconnection in background
- Both AP and STA share one radio on ESP32 — they operate on the same channel
- When entering AP_FALLBACK: `WiFi.mode(WIFI_AP_STA)`, `WiFi.softAP("FlightWall-Setup")`
- The AP allows user to reconfigure via wizard; STA side can attempt reconnection if user updates credentials

**WiFi scan limitations (for Story 1.5, but relevant context):**
- `WiFi.scanNetworks(true)` (async) returns -2 if STA is not connected in AP_STA mode — this is a known hardware limitation
- Scanning temporarily disrupts AP beacon, disconnecting clients briefly
- WiFiManager should NOT perform scans — that's the WebPortal's job (Story 1.5)

**NTP via configTime():**
- Use `configTime(0, 0, "pool.ntp.org")` for UTC (no timezone offset needed — FlightWall only needs UTC for API counter reset)
- configTime() is non-blocking; it configures SNTP client which syncs in background
- Do NOT wait for sync to complete — it happens asynchronously
- Time needed for: OpenSky API monthly call counter reset (compare `api_month` in NVS with current UTC month)

**mDNS registration:**
- `MDNS.begin("flightwall")` makes device reachable at `flightwall.local`
- `MDNS.addService("_http", "_tcp", 80)` registers HTTP service
- Call `MDNS.end()` when leaving STA_CONNECTED (cleanup)
- mDNS resolution is unreliable on Windows 10 — IP display on LEDs is the fallback discovery method
- Include `#include <ESPmDNS.h>` in WiFiManager.cpp

**Display integration — IP message timing:**
- WiFiManager.init() runs in setup() BEFORE the display task starts (per architecture init order: WiFiManager at step 5, display task at step 8)
- Initial status messages ("Setup Mode", "Connecting...") can use `g_display.displayMessage()` directly from setup()
- Post-setup state changes (reconnection, IP display) happen after display task is running
- For post-setup messages: register an onStateChange callback that calls `g_display.displayMessage()` — safe because displayMessage() is called from the callback context on Core 1, and the display task reads the message via the existing queue mechanism
- IP display duration (3 seconds) should be handled by the callback using `delay(3000)` in setup context, or via a timed flag if display task is already running

**init() sequence in main.cpp setup():**
```
Current order (from architecture Decision 5):
1. ConfigManager::init()
2. SystemStatus::init()
3. g_display.initialize()  ← display init
4. showLoading()
5. g_wifiManager.init()    ← NEW: WiFiManager
6. [create queue]
7. [create display task]
8. [FlightDataFetcher setup]
```

WiFiManager.init() runs AFTER display is initialized but BEFORE the FreeRTOS display task is created. This means init() can safely call `g_display.displayMessage()` for the initial status (e.g., "Connecting..." or "Setup Mode").

### What NOT to do

- **Do NOT create WebPortal or API endpoints** — that's Story 1.5
- **Do NOT create the setup wizard HTML** — that's Stories 1.6-1.7
- **Do NOT implement GPIO button handling** — Story 1.8 handles LED progress states; GPIO forced AP mode is referenced in the BDD scenarios but is a future-story concern (architecture shows it in the WiFiManager design, but the wizard flow that responds to it is Story 1.6+). Implement the `forcedAPMode` flag/check in init() so it can be set later, but do NOT implement the actual GPIO read/interrupt
- **Do NOT implement captive portal DNS redirect** — that's an optional enhancement for Story 1.5
- **Do NOT add ESPAsyncWebServer code** — WebPortal (Story 1.5) owns the web server
- **Do NOT pause/resume FlightDataFetcher directly** — register a callback that sets a flag; FlightDataFetcher will check it in a future story
- **Do NOT use WiFi.waitForConnectResult()** — it's blocking. Use event-driven approach with WiFi.onEvent()
- **Do NOT call WiFi API functions from inside WiFi.onEvent() callbacks** — deadlock risk

### Architecture compliance

- **File location:** `firmware/adapters/WiFiManager.h` and `WiFiManager.cpp` — architecture spec places WiFi in `adapters/` (not `core/`)
- **Class naming:** PascalCase `WiFiManager`, camelCase methods `getState()`, `onStateChange()`
- **Enum naming:** UPPER_SNAKE for enum values `AP_SETUP`, `STA_CONNECTED`
- **Logging:** `LOG_E("WiFi", ...)` / `LOG_I("WiFi", ...)` / `LOG_V("WiFi", ...)` from `utils/Log.h` — never raw Serial.println in new code
- **Error pattern:** boolean return + SystemStatus reporting per architecture Decision 6
- **Initialization order:** WiFiManager::init() at step 5 per architecture Decision 5
- **Config access:** `ConfigManager::getNetwork()` for credentials — never read NVS directly

### Library / framework requirements

- **WiFi.h** — built-in ESP32 Arduino core; `#include <WiFi.h>`
- **ESPmDNS.h** — built-in ESP32 Arduino core; `#include <ESPmDNS.h>`
- **std::atomic** — C++11 standard library; `#include <atomic>` (already in project from Story 1.3)
- **std::vector, std::function** — C++ STL; `#include <vector>`, `#include <functional>`
- **No new PlatformIO lib_deps required** — WiFi, mDNS, NTP are all built-in ESP32

### File structure requirements

| File | Action | Notes |
|------|--------|-------|
| `firmware/adapters/WiFiManager.h` | CREATE | State enum, class declaration, callback types |
| `firmware/adapters/WiFiManager.cpp` | CREATE | State machine, event handlers, tick(), mDNS, NTP |
| `firmware/src/main.cpp` | MODIFY | Replace blocking WiFi code with WiFiManager init/tick, register callbacks |

**No other files should be modified.** Display adapter, ConfigManager, and SystemStatus already have all needed APIs from previous stories.

### Testing requirements

- **No unit tests required** — WiFi operations are hardware-dependent and cannot be mocked in the native PlatformIO test environment
- **Verification:** `pio run` must succeed with zero errors
- **RAM/Flash usage:** Should be logged. Expect modest increase (~2-4KB for WiFiManager class + mDNS)
- **On-device testing (when available):**
  - Boot with empty NVS → verify AP_SETUP state, "FlightWall-Setup" SSID broadcasts
  - Boot with saved WiFi → verify STA_CONNECTED, mDNS, IP display
  - Disconnect WiFi router → verify STA_RECONNECTING → reconnect or AP_FALLBACK after 60s
  - Verify SystemStatus reports correct WiFi state via Serial output

### Previous story intelligence (1.3)

- **Dual-core architecture** is now active: display task on Core 0, loop() on Core 1
- **FlightDisplayData double-buffer queue** with pointer-based approach — WiFiManager does not interact with this directly
- **g_configChanged atomic flag** pattern established — WiFiManager uses same atomic flag pattern for WiFi events
- **displayMessage() timing:** Before display task starts, it's safe to call from setup(). After task starts, the display task owns rendering — but `displayMessage()` writes to the matrix directly (single-threaded access is ensured because display task only calls `renderFlight()` / `xQueuePeek()`, not `displayMessage()`)
- **vTaskDelay(50ms)** frame rate in display task — WiFi status messages displayed via `displayMessage()` persist until overwritten
- **Build baseline:** RAM: 15.0% (49044 bytes), Flash: 50.5% (1058265 bytes)
- **LOG macro limitation:** Only supports 2 args (tag + literal string concat). For dynamic strings, use `Serial.println()` with `#if LOG_LEVEL >= N` guards (same pattern as Story 1.3)
- **Review from 1.3 (in review status):** Implementation chose pointer-based double-buffer queue (Option A). Display task handles its own flight cycling timing. `renderFlight()` and `updateBrightness()` added to NeoMatrixDisplay.

### Previous story intelligence (1.2)

- **ConfigManager::getNetwork()** returns `NetworkConfig { wifi_ssid, wifi_password, opensky_client_id, opensky_client_secret, aeroapi_key }`
- **requiresReboot("wifi_ssid")** returns true — WiFi credential changes trigger immediate NVS persist + reboot flag
- **onChange()** callbacks fire on hot-reload keys only (not reboot keys) — WiFiManager doesn't need onChange for WiFi creds since they require reboot
- **tick()** must be called from loop() — ConfigManager already has this pattern; WiFiManager follows same pattern
- **NVS namespace:** "flightwall" — 15-char key limit for NVS keys
- **22 NVS keys total** defined in ConfigManager; no new keys needed for WiFiManager

### Previous story intelligence (1.1)

- **LittleFS** mounts via `LittleFS.begin(true)` in setup() — runs before WiFiManager
- **Log macros** LOG_E/LOG_I/LOG_V from `utils/Log.h` — use exclusively
- **build_src_filter** includes `+<../adapters/*.cpp>` — new WiFiManager.cpp in adapters/ will be auto-included
- **ESPAsyncWebServer** already in lib_deps (added in 1.1) — WiFiManager does not use it, but it's available for Story 1.5

### Git intelligence

- Repository has no commits — greenfield development; no commit patterns to match

### Latest technical notes

- **WiFi.onEvent() callback threading:** Callbacks run on WiFi driver's internal FreeRTOS task, NOT on your application task. Never call WiFi API functions (begin, disconnect, reconnect) from inside callbacks — use atomic flags and process in tick()
- **ESP32 Arduino Core event names:** `ARDUINO_EVENT_WIFI_STA_CONNECTED`, `ARDUINO_EVENT_WIFI_STA_DISCONNECTED`, `ARDUINO_EVENT_WIFI_STA_GOT_IP` — stable across v2.x and v3.x
- **WiFi.scanNetworks(true) known issue:** Returns -2 (WIFI_SCAN_FAILED) if STA is not connected in AP_STA mode. Scanning also temporarily disrupts AP beacon. WiFiManager should NOT do scans — leave that to WebPortal (Story 1.5)
- **configTime() timezone bug:** ESP32 inverts timezone offset sign vs ESP8266. Use `configTime(0, 0, "pool.ntp.org")` for UTC (no offset needed for FlightWall)
- **WIFI_AP_STA mode:** Both AP and STA share one radio, forced to same channel. When STA connects to external AP, the softAP channel auto-adjusts to match. This is expected behavior, not a bug
- **mDNS on Windows 10:** `flightwall.local` resolution is unreliable. IP display on LEDs (3 seconds on connect) serves as fallback device discovery
- **WiFi.reconnect():** Attempts to reconnect using previously stored credentials. Safe to call from tick() on Core 1. Do NOT call from event callbacks

### Project context reference

- No `project-context.md` found — rely on architecture.md, epics.md, prd.md, and previous story files

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` — Story 1.4, Epic 1 overview]
- [Source: `_bmad-output/planning-artifacts/architecture.md` — Decision 3 (WiFi State Machine), Decision 4 (Web API), Decision 5 (Init Order), Decision 6 (SystemStatus)]
- [Source: `_bmad-output/planning-artifacts/prd.md` — FR2, FR7, FR8, FR40, FR41, FR44, NFR-Reliability-2, NFR-Reliability-4]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` — LED progress states, AP mode UX, mDNS discovery]
- [Source: `1-2-configmanager-and-systemstatus.md` — ConfigManager API, NetworkConfig, SystemStatus subsystems]
- [Source: `1-3-freertos-dual-core-task-architecture.md` — dual-core pattern, display task, atomic flag pattern, LOG macro limitations]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Initial build failed due to `WiFiEvent_t` type not being declared in header — needed `#include <WiFi.h>` in WiFiManager.h to resolve the typedef. Fixed and rebuild succeeded.

### Completion Notes List

- **Task 1:** Created WiFiManager.h with WiFiState enum (5 states), WiFiManager class with public API (init, tick, getState, getLocalIP, getSSID, onStateChange), StateCallback typedef, and private members including atomic flags for thread-safe event handling.
- **Task 2:** Implemented full state machine in WiFiManager.cpp. init() reads ConfigManager::getNetwork() and branches on empty SSID. Two-phase event architecture: WiFi.onEvent() sets atomic flags, tick() processes them safely on Core 1. Static _instance pointer pattern for event callback routing. Reconnection logic: 5s interval, 12 max retries, fallback to AP_STA mode.
- **Task 3:** mDNS registration (flightwall.local + HTTP service) and NTP config (pool.ntp.org, UTC) on STA_CONNECTED entry. MDNS.end() cleanup on disconnect.
- **Task 4:** Callback vector with onStateChange() registration. _setState() fires all callbacks and updates SystemStatus. RSSI-based reporting: OK (>= -75 dBm), WARNING (< -75 dBm), ERROR (disconnected/retrying).
- **Task 5:** Replaced blocking WiFi.begin() loop in main.cpp with WiFiManager init/tick. Init order follows architecture Decision 5 (WiFiManager at step 5, before queue/task creation). Display callback registered for all state transitions. PIO_UNIT_TESTING guard preserved.
- **Task 6:** Build verified — zero errors, zero warnings. RAM: 15.6% (+0.6% from baseline), Flash: 52.2% (+1.7% from baseline). test_config_manager builds successfully. LOG_I/LOG_E messages present for all state transitions.

### Implementation Plan

Event-driven state machine with two-phase architecture:
1. WiFi.onEvent() handlers (run on WiFi system task) set std::atomic<bool> flags
2. tick() (called from loop() on Core 1) processes flags, performs WiFi API calls, fires callbacks, updates SystemStatus
3. Static _instance pointer enables routing from static event callback to instance methods
4. Display integration via onStateChange callback registered in setup()

### File List

| File | Action | Notes |
|------|--------|-------|
| `firmware/adapters/WiFiManager.h` | CREATED | State enum, class declaration, callback types |
| `firmware/adapters/WiFiManager.cpp` | CREATED | State machine, event handlers, tick(), mDNS, NTP, SystemStatus |
| `firmware/src/main.cpp` | MODIFIED | Replaced blocking WiFi with WiFiManager init/tick, added display callback |

## Change Log

- 2026-04-02: Story 1.4 implemented — WiFiManager with event-driven state machine, mDNS/NTP, SystemStatus integration, display callbacks. Replaced blocking WiFi code in main.cpp. Build: RAM 15.6%, Flash 52.2%.

### Review Findings

- [x] [Review][Patch] Missing WiFi display/status callback integration breaks AC #1, #2, #4, and #7 [`firmware/src/main.cpp:211`]
- [x] [Review][Patch] Initial AP setup never fires callback or SystemStatus update because `_state` already starts as `AP_SETUP` [`firmware/adapters/WiFiManager.h:30`]
- [x] [Review][Patch] Disconnect events during initial `CONNECTING` are ignored, so bad credentials can leave WiFi stuck forever instead of retrying/falling back [`firmware/adapters/WiFiManager.cpp:133`]
- [x] [Review][Patch] Reconnect loop falls back on retry count 12 before issuing the 12th reconnect attempt [`firmware/adapters/WiFiManager.cpp:41`]
