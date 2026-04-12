       COMPREHENSIVE ANALYSIS: FlightWall Firmware Main Application Flow

       FILE: /Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp

       ---
       1. COMPLETE BOOT/SETUP SEQUENCE (setup() function - Lines 402-557)

       Initialization Order:

       1. Serial Communication (Line 404-405)
         - 115200 baud rate
         - 200ms delay for stabilization
       2. Filesystem (Lines 407-411)
         - LittleFS mounted with begin(true) (format if mount fails)
         - Used for logo storage and web assets
       3. Core Managers (Lines 413-414)
         - ConfigManager::init() - Loads NVS settings with compile-time fallbacks
         - SystemStatus::init() - Initializes thread-safe subsystem status tracking
       4. Logo Manager (Lines 417-419)
         - LogoManager::init() after LittleFS
         - Failure non-fatal - falls back to default sprites
       5. Layout Engine (Lines 422-424)
         - Computes display zones from hardware config
         - Stored in global g_layout variable
         - Layout modes: "compact", "full", or "expanded"
       6. Display Initialization (Lines 427-428)
         - g_display.initialize() - Creates NeoPixel matrix on Core 1 (setup runs on Core 1)
         - g_display.showLoading() - Shows initial loading animation
       7. FreeRTOS Queues (Lines 430-443)
         - g_flightQueue: Queue length 1, stores pointer to FlightDisplayData
         - g_displayMessageQueue: Queue length 1, stores DisplayStatusMessage struct
         - Both use xQueueOverwrite semantics (single-entry, latest wins)
       8. Config Change Callback (Lines 446-450)
         - Registers lambda that sets atomic g_configChanged flag
         - Recomputes layout when hardware config changes
         - Callback fires on hot-reload config changes
       9. Display Task Creation (Lines 453-466)
         - Task function: displayTask
         - Core: 0 (APP CPU)
         - Stack: 8192 bytes
         - Priority: 1
         - Runs independently of fetch loop
       10. Boot Button Detection (Lines 469-487)
         - GPIO 0 (BOOT button) configured as INPUT_PULLUP
         - Samples for 500ms to detect held-low state
         - Sets g_forcedApSetup = true if held
         - Forces AP setup mode regardless of stored credentials
       11. WiFi Manager (Lines 491)
         - g_wifiManager.init(g_forcedApSetup)
         - Non-blocking, event-driven WiFi state machine
       12. Startup Progress Coordinator (Lines 495-515)
         - Activates if WiFi credentials exist (not first boot)
         - Sets initial phase based on WiFi state:
             - Forced AP: "Setup Mode — Forced"
           - CONNECTING: enters CONNECTING_WIFI phase
           - Other: shows initial WiFi message
         - Registers WiFi state change callback
       13. WebPortal (Lines 519-549)
         - g_webPortal.init(g_webServer, g_wifiManager)
         - Registers callback handlers:
             - Calibration (Lines 522-529): Toggles gradient test pattern
           - Positioning (Lines 532-539): Toggles panel position guide
           - Reboot (Lines 542-546): Shows "Saving config..." before restart
         - g_webPortal.begin() starts AsyncWebServer on port 80
       14. Flight Data Fetcher (Lines 551-555)
         - Creates FlightDataFetcher with OpenSky and AeroAPI adapters
         - Runs on Core 1 (loop() context)

       ---
       2. FETCH TASK LOOP (Core 1 - loop() function)

       Main Loop Flow (Lines 639-772):

       A. Tick Operations (Lines 641-642)

       ConfigManager::tick();     // Process pending NVS persist operations
       g_wifiManager.tick();      // Process WiFi events and retry logic

       B. Boot Button Short-Press Detection (Lines 645-673)

       - Active only when NOT in forced AP setup mode
       - Debounce: 50ms (BUTTON_DEBOUNCE_MS)
       - Short press: ≤ 1000ms (BUTTON_SHORT_PRESS_MAX_MS)
       - Action on short press:
         - If STA_CONNECTED: Show IP for 5 seconds
         - Else: Show "No IP — Setup mode" for 5 seconds
       - Long presses ignored (>1000ms)

       C. Startup Progress State Machine (Line 682)

       - Calls tickStartupProgress() (Lines 564-635)
       - Returns true when ready to trigger first fetch
       - State transitions:
         a. SAVING_CONFIG → Persists until reboot
         b. CONNECTING_WIFI → Waits for WiFi callback
         c. WIFI_CONNECTED → Shows "WiFi Connected ✓" (2s) then IP (2s) then advances to auth
         d. WIFI_FAILED → Shows failure message (5s) then returns to IDLE
         e. AUTHENTICATING → Holds message (1s) then triggers first fetch
         f. FETCHING_FLIGHTS → Waits for fetch completion
         g. COMPLETE → Transitions to normal operation

       D. Fetch Interval Logic (Lines 685-695)

       - Normal fetch: Every fetch_interval seconds (from config)
       - First fetch: Triggered immediately when tickStartupProgress() returns true
       - g_firstFetchDone prevents repeat first-fetch

       E. Flight Data Fetch (Lines 698-748)

       1. Show "Fetching flights..." during startup phases
       2. Call g_fetcher->fetchFlights(states, flights)
          - Fetches state vectors from OpenSky (geo-filtered)
          - Enriches with AeroAPI flight info
          - Enriches display names from FlightWall CDN
          - Converts telemetry units (SI → display units)
       3. Update flight stats snapshot (atomic writes)
       4. Write flights to double buffer
       5. Push buffer pointer to queue (xQueueOverwrite)
       6. Swap buffers (g_writeBuf ^= 1)

       F. Startup Progress Completion (Lines 750-770)

       - When g_firstFetchDone = false and in fetch phase:
         - Mark g_firstFetchDone = true
         - Enter COMPLETE phase
         - Clear progress message (queue empty string for 1ms)
         - Allows display task to render flight data

       ---
       3. DISPLAY TASK (Core 0 - displayTask())

       Task Loop Flow (Lines 252-397):

       A. Config Change Detection (Lines 273-305)

       - Checks atomic g_configChanged flag
       - On hardware config change:
         - Geometry change (tiles, pixels, pin): Logs reboot required message
         - Mapping change (origin, scan, zigzag): Hot-reloads via reconfigureFromConfig()
       - Always updates: Brightness and timing from local copies

       B. Special Mode Overrides (Lines 309-326)

       - Calibration mode (priority 1): Renders gradient test pattern at 50ms intervals
       - Positioning mode (priority 2): Renders panel position guide at 50ms intervals
       - Both modes override status messages

       C. Status Message Queue (Lines 328-349)

       - Receives from g_displayMessageQueue (non-blocking peek)
       - Message priority: Overrides flight rendering
       - Duration logic:
         - durationMs = 0: Persistent (manual clear)
         - durationMs > 0: Timed (auto-expire)
       - Displays via g_display.displayMessage()

       D. Flight Rendering (Lines 352-381)

       - Peeks (doesn't remove) latest buffer pointer from g_flightQueue
       - Flight cycling:
         - Single flight: Always shows index 0
         - Multiple flights: Cycles every display_cycle seconds
         - Tracks currentFlightIndex and lastCycleMs
       - Renders via g_display.renderFlight(flights, index)
       - Empty flight list: Shows loading screen

       E. Watchdog & Yield (Lines 394-396)

       - Resets task watchdog timer
       - Yields for 50ms (~20fps frame rate)
       - Prevents task starvation

       ---
       4. GLOBAL VARIABLES AND PURPOSES

       Data Structures:
       ┌───────────────────────┬──────────────────────┬─────────────────────────────────────────────────
       ──┐
       │       Variable        │         Type         │                      Purpose
         │
       ├───────────────────────┼──────────────────────┼─────────────────────────────────────────────────
       ──┤
       │ g_flightBuf[2]        │ FlightDisplayData[2] │ Double buffer for cross-core flight data
       transfer │
       ├───────────────────────┼──────────────────────┼─────────────────────────────────────────────────
       ──┤
       │ g_writeBuf            │ uint8_t              │ Current write buffer index (0 or 1)
         │
       ├───────────────────────┼──────────────────────┼─────────────────────────────────────────────────
       ──┤
       │ g_flightQueue         │ QueueHandle_t        │ Queue holding pointer to current read buffer
         │
       ├───────────────────────┼──────────────────────┼─────────────────────────────────────────────────
       ──┤
       │ g_displayMessageQueue │ QueueHandle_t        │ Queue for status messages (overwrite semantics)
         │
       ├───────────────────────┼──────────────────────┼─────────────────────────────────────────────────
       ──┤
       │ g_configChanged       │ atomic<bool>         │ Signals config change from Core 1 to Core 0
         │
       └───────────────────────┴──────────────────────┴─────────────────────────────────────────────────
       ──┘





       Startup Progress State:
       ┌──────────────────┬───────────────────┬──────────────────────────────────────────┐
       │     Variable     │       Type        │                 Purpose                  │
       ├──────────────────┼───────────────────┼──────────────────────────────────────────┤
       │ g_startupPhase   │ StartupPhase enum │ Current startup progress phase           │
       ├──────────────────┼───────────────────┼──────────────────────────────────────────┤
       │ g_phaseEnteredMs │ unsigned long     │ Timestamp when current phase started     │
       ├──────────────────┼───────────────────┼──────────────────────────────────────────┤
       │ g_firstFetchDone │ bool              │ One-shot flag for first fetch completion │
       └──────────────────┴───────────────────┴──────────────────────────────────────────┘
       Component Instances:
       ┌───────────────┬────────────────────┬────────────────────────────────┐
       │   Variable    │        Type        │            Purpose             │
       ├───────────────┼────────────────────┼────────────────────────────────┤
       │ g_openSky     │ OpenSkyFetcher     │ State vector fetcher adapter   │
       ├───────────────┼────────────────────┼────────────────────────────────┤
       │ g_aeroApi     │ AeroAPIFetcher     │ Flight info enrichment adapter │
       ├───────────────┼────────────────────┼────────────────────────────────┤
       │ g_fetcher     │ FlightDataFetcher* │ Orchestrates fetch pipeline    │
       ├───────────────┼────────────────────┼────────────────────────────────┤
       │ g_display     │ NeoMatrixDisplay   │ LED matrix renderer            │
       ├───────────────┼────────────────────┼────────────────────────────────┤
       │ g_wifiManager │ WiFiManager        │ WiFi state machine             │
       ├───────────────┼────────────────────┼────────────────────────────────┤
       │ g_webServer   │ AsyncWebServer     │ HTTP server (port 80)          │
       ├───────────────┼────────────────────┼────────────────────────────────┤
       │ g_webPortal   │ WebPortal          │ Web dashboard route handler    │
       └───────────────┴────────────────────┴────────────────────────────────┘
       Feature Flags:
       ┌───────────────────┬───────────────┬───────────────────────────────────┐
       │     Variable      │     Type      │              Purpose              │
       ├───────────────────┼───────────────┼───────────────────────────────────┤
       │ g_forcedApSetup   │ bool          │ Boot button held - force AP mode  │
       ├───────────────────┼───────────────┼───────────────────────────────────┤
       │ g_buttonLastState │ bool          │ Button debounce state tracking    │
       ├───────────────────┼───────────────┼───────────────────────────────────┤
       │ g_lastFetchMs     │ unsigned long │ Timestamp of last fetch operation │
       └───────────────────┴───────────────┴───────────────────────────────────┘
       Flight Statistics (atomic):
       ┌─────────────────────────┬───────────────────────┬─────────────────────────────────────────┐
       │        Variable         │         Type          │                 Purpose                 │
       ├─────────────────────────┼───────────────────────┼─────────────────────────────────────────┤
       │ g_statsLastFetchMs      │ atomic<unsigned long> │ Timestamp of last fetch for /api/status │
       ├─────────────────────────┼───────────────────────┼─────────────────────────────────────────┤
       │ g_statsStateVectors     │ atomic<uint16_t>      │ State vector count from last fetch      │
       ├─────────────────────────┼───────────────────────┼─────────────────────────────────────────┤
       │ g_statsEnrichedFlights  │ atomic<uint16_t>      │ Enriched flight count from last fetch   │
       ├─────────────────────────┼───────────────────────┼─────────────────────────────────────────┤
       │ g_statsLogosMatched     │ atomic<uint16_t>      │ Logos matched (placeholder 0)           │
       ├─────────────────────────┼───────────────────────┼─────────────────────────────────────────┤
       │ g_statsFetchesSinceBoot │ atomic<uint32_t>      │ Total fetch attempts since boot         │
       └─────────────────────────┴───────────────────────┴─────────────────────────────────────────┘
       Layout Engine State:
       ┌──────────┬──────────────┬──────────────────────────────────────────────────┐
       │ Variable │     Type     │                     Purpose                      │
       ├──────────┼──────────────┼──────────────────────────────────────────────────┤
       │ g_layout │ LayoutResult │ Computed display zones (logo, flight, telemetry) │
       └──────────┴──────────────┴──────────────────────────────────────────────────┘
       ---
       5. CONFIGURATION CHANGE DETECTION & APPLICATION

       Detection Mechanism:

       1. WebPortal receives POST /api/settings with JSON patch
       2. ConfigManager::applyJson() (Lines 491-547):
         - Validates all keys before applying
         - Distinguishes reboot-required keys vs hot-reload keys
         - Updates in-memory cache under mutex
         - Reboot keys: Immediate NVS persist, return reboot_required: true
         - Hot-reload keys: Schedule debounced persist (2s), fire callbacks
       3. Reboot-Required Keys (Lines 164-169):
       wifi_ssid, wifi_password
       os_client_id, os_client_sec
       aeroapi_key, display_pin
       tiles_x, tiles_y, tile_pixels
       4. Config Change Callback (Line 446):
       ConfigManager::onChange([]() {
           g_configChanged.store(true);      // Signal Core 0
           g_layout = LayoutEngine::compute(...);  // Recompute zones
       });
       5. Display Task Handling (Lines 273-305):
         - Reads g_configChanged.exchange(false) (atomic clear)
         - Geometry change: Logs "reboot required" (no auto-restart)
         - Mapping change: Calls g_display.reconfigureFromConfig() (hot-reload)
         - Always: Updates brightness and timing

       NVS Persistence:

       - Immediate persist: Reboot-required keys (Line 534)
       - Debounced persist: Hot-reload keys via schedulePersist(2000) (Line 539)
       - Tick-driven flush: ConfigManager::tick() checks elapsed time and calls persistToNvs()

       ---
       6. DOUBLE-BUFFER FLIGHT DATA QUEUE

       Architecture:

       Core 1 (Producer)               Core 0 (Consumer)
       ─────────────────               ─────────────────
       g_flightBuf[0] ◄───────────────► Read buffer pointer
       g_flightBuf[1] ◄───────────────► (from g_flightQueue)
              │
              ▼
         g_writeBuf (0 or 1)

       Producer Side (Core 1 - Lines 741-747):

       // Write flight data to current write buffer
       g_flightBuf[g_writeBuf].flights = flights;

       // Push pointer to queue (overwrites previous)
       FlightDisplayData *ptr = &g_flightBuf[g_writeBuf];
       xQueueOverwrite(g_flightQueue, &ptr);

       // Swap buffers for next write
       g_writeBuf ^= 1;

       Consumer Side (Core 0 - Lines 352-375):

       // Peek (don't remove) latest buffer pointer
       FlightDisplayData *ptr = nullptr;
       xQueuePeek(g_flightQueue, &ptr, 0);

       // Read from buffer (safe - producer writes to other buffer)
       const auto &flights = ptr->flights;

       Safety Properties:

       1. No memcpy of String/vector - Only pointer swap
       2. Lock-free - No mutex contention
       3. Latest-wins semantics - xQueueOverwrite discards old data
       4. No torn reads - Consumer always reads from stable buffer

       ---
       7. CALLBACK REGISTRATIONS

       A. Config Change Callback (Lines 446-450)

       ConfigManager::onChange([]() {
           g_configChanged.store(true);
           g_layout = LayoutEngine::compute(ConfigManager::getHardware());
       });
       - When: Any hot-reload config key changes
       - Action: Signal Core 0 and recompute layout zones

       B. WiFi State Change Callback (Lines 512-515)

       g_wifiManager.onStateChange([](WiFiState oldState, WiFiState newState) {
           queueWiFiStateMessage(newState);
       });
       - When: WiFi state transitions (AP, CONNECTING, CONNECTED, RECONNECTING, FALLBACK)
       - Action: Queue appropriate display message and update startup phase

       C. Calibration Callback (Lines 522-529)

       g_webPortal.onCalibration([](bool enabled) {
           g_display.setCalibrationMode(enabled);
       });
       - When: POST /api/calibration/start or /api/calibration/stop
       - Action: Toggle gradient test pattern rendering

       D. Positioning Callback (Lines 532-539)

       g_webPortal.onPositioning([](bool enabled) {
           g_display.setPositioningMode(enabled);
       });
       - When: POST /api/positioning/start or /api/positioning/stop
       - Action: Toggle panel position guide rendering

       E. Reboot Callback (Lines 542-546)

       g_webPortal.onReboot([]() {
           enterPhase(StartupPhase::SAVING_CONFIG);
           queueDisplayMessage(String("Saving config..."));
       });
       - When: POST /api/reboot (before ESP.restart())
       - Action: Show "Saving config..." on LED display

       ---
       8. STATUS MESSAGE QUEUE & PRIORITY LOGIC

       Queue Structure:

       struct DisplayStatusMessage {
           char text[64];        // Fixed-size message (no heap alloc)
           uint32_t durationMs;  // 0 = persistent, >0 = timed
       };

       Priority Hierarchy (highest to lowest):

       1. Calibration Mode (Lines 309-316)
         - Renders gradient pattern
         - Overrides all messages
         - Checked before status message queue
       2. Positioning Mode (Lines 319-326)
         - Renders panel position guide
         - Overrides status messages
         - Checked after calibration
       3. Status Messages (Lines 328-349)
         - Read from g_displayMessageQueue
         - Persistent (duration=0) or timed (duration>0)
         - Overrides flight rendering
       4. Flight Rendering (Lines 352-381)
         - Default when no higher-priority mode active
         - Shows loading screen if flight list empty

       Message Queuing Function (Lines 140-151):

       queueDisplayMessage(const String &message, uint32_t durationMs = 0)
       - Uses xQueueOverwrite - latest message wins
       - No blocking - safe from any context
       - Duration 0 = persistent until cleared

       ---
       9. WiFi CONNECTION FLOW

       State Machine (WiFiManager):

       ┌─────────────┐
       │  AP_SETUP   │ ◄──── Boot (no credentials or forced)
       └─────────────┘
              │
              ▼
       ┌─────────────┐
       │ CONNECTING  │ ◄──── Credentials exist
       └─────────────┘
              │
              ├─────────────► STA_CONNECTED ──► (Normal operation)
              │                      │
              │                      ▼
              │               STA_RECONNECTING ◄─── Disconnect event
              │                      │
              │                      ├─────────► (Retry up to 12 times)
              │                      │
              ▼                      ▼
       ┌─────────────┐      ┌─────────────┐
       │ AP_FALLBACK │ ◄────┤ Max retries │
       └─────────────┘      └─────────────┘

       WiFiManager::tick() Logic (Lines 36-62):

       1. Process atomic flags from WiFi event handler:
         - _gotIP.exchange(false) → call _onConnected()
         - _disconnected.exchange(false) → call _onDisconnected()
       2. Reconnection retry logic:
         - If in STA_RECONNECTING state
         - Wait 5 seconds between retries (RETRY_INTERVAL_MS)
         - Max 12 retries (MAX_RETRIES)
         - On timeout: Enter AP_FALLBACK mode

       Event Handler (Lines 190-203):

       void WiFiManager::_onWiFiEvent(WiFiEvent_t event) {
           switch (event) {
               case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                   _instance->_gotIP.store(true);
                   break;
               case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                   _instance->_disconnected.store(true);
                   break;
           }
       }
       - Runs on WiFi system task (different core)
       - Sets atomic flags read by tick() on Core 1

       Connection Success Actions (Lines 120-139):

       1. Reset retry count
       2. Start mDNS: flightwall.local
       3. Configure NTP: pool.ntp.org (UTC)
       4. Update SystemStatus
       5. Fire state change callbacks

       ---
       10. SYSTEM STATE TRANSITIONS

       Startup Sequence Flow:

       BOOT
        │
        ├─ Boot button held? ──YES──► IDLE (Forced AP Setup)
        │                              └─► Show "Setup Mode — Forced"
        │
        ├─ No WiFi credentials? ──YES─► IDLE (AP Setup)
        │                              └─► Show "Setup Mode"
        │
        └─ Credentials exist ──► CONNECTING_WIFI
                                  └─► Show "Connecting to WiFi..."
                                        │
                                        ├─ WiFi fails ──► WIFI_FAILED
                                        │                 └─► Show "WiFi Failed - Reopen Setup" (5s)
                                        │                     └─► IDLE (AP Fallback)
                                        │
                                        └─ WiFi connected ──► WIFI_CONNECTED
                                                              └─► Show "WiFi Connected ✓" (2s)
                                                                  └─► Show "IP: x.x.x.x" (2s)
                                                                      └─► AUTHENTICATING
                                                                          └─► Show "Authenticating
       APIs..." (1s)
                                                                              └─► FETCHING_FLIGHTS
                                                                                  └─► Show "Fetching
       flights..."
                                                                                      └─► First fetch
       completes
                                                                                          └─► COMPLETE
                                                                                              └─► Clear
       message
                                                                                                  └─►
       Render flight data

       State Transition Drivers:

       ┌──────────────────┬──────────────────┬───────────────────────────┐
       │      Phase       │    Driven By     │   Transition Condition    │
       ├──────────────────┼──────────────────┼───────────────────────────┤
       │ CONNECTING_WIFI  │ WiFi callback    │ GOT_IP event or timeout   │
       ├──────────────────┼──────────────────┼───────────────────────────┤
       │ WIFI_CONNECTED   │ Timer            │ 4s elapsed (2s ✓ + 2s IP) │
       ├──────────────────┼──────────────────┼───────────────────────────┤
       │ AUTHENTICATING   │ Timer            │ 1s elapsed                │
       ├──────────────────┼──────────────────┼───────────────────────────┤
       │ FETCHING_FLIGHTS │ Fetch completion │ fetchFlights() returns    │
       ├──────────────────┼──────────────────┼───────────────────────────┤
       │ COMPLETE         │ Fetch flag       │ g_firstFetchDone set      │
       └──────────────────┴──────────────────┴───────────────────────────┘
       Normal Operation State:

       - Startup phase: COMPLETE or IDLE
       - WiFi state: STA_CONNECTED or AP_SETUP
       - Display mode:
         - Calibration → Gradient test pattern
         - Positioning → Panel position guide
         - Status message → Text banner
         - Default → Flight cycling

       ---
       KEY ARCHITECTURAL INSIGHTS

       Dual-Core Strategy:

       - Core 1 (Protocol CPU): Network I/O, config management, fetch orchestration
       - Core 0 (APP CPU): Display rendering, LED matrix updates
       - Cross-core communication: Lock-free queues and atomic flags

       No String/Vector Copies:

       - Double-buffer scheme avoids expensive std::vector<FlightInfo> copies
       - Only pointers exchanged between cores
       - Prevents heap fragmentation and latency spikes

       Event-Driven Architecture:

       - WiFi: Atomic flags from event handler → processed in tick()
       - Config: Callback chain (WebPortal → ConfigManager → main.cpp → DisplayTask)
       - Display: Queue-based message system with priority override

       Graceful Degradation:

       - LogoManager init failure → fallback sprites
       - AeroAPI failure → partial enrichment (logs warning, continues)
       - CDN failure → missing display names (shows operator codes)
       - Empty flight list → loading screen

       Memory Safety:

       - Fixed-size message buffers (64 chars, no heap)
       - Tile config validation prevents OOM (max 16,384 pixels)
       - GPIO pin whitelist prevents invalid hardware access

       This firmware demonstrates professional embedded systems architecture with proper separation of
       concerns, thread-safe cross-core communication, and robust error handling.
  ⎿  Done (24 tool uses · 55.3k tokens · 3m 2s)

⏺ TheFlightWall OSS — Comprehensive Overview                                                            

  What It Is                                                                                                
                                                                                                            
  TheFlightWall is an ESP32-powered LED matrix wall display that shows real-time flight information for     
  aircraft flying overhead. It fetches live ADS-B tracking data, enriches it with airline/route details, and
   renders everything — airline logos, flight identifiers, routes, and telemetry — on a WS2812B LED panel
  array. A built-in web dashboard provides full configuration and monitoring without touching code.

  ---
  Hardware
  ┌───────────────┬─────────────────────────────────────────────────────────────────────────────┐
  │   Component   │                                   Details                                   │
  ├───────────────┼─────────────────────────────────────────────────────────────────────────────┤
  │ MCU           │ ESP32 dev board (dual-core, WiFi)                                           │
  ├───────────────┼─────────────────────────────────────────────────────────────────────────────┤
  │ Display       │ WS2812B LED panels in tile grid (e.g., 10x2 tiles of 16x16 = 160x32 pixels) │
  ├───────────────┼─────────────────────────────────────────────────────────────────────────────┤
  │ Power         │ 5V 20A+ supply for LED panels                                               │
  ├───────────────┼─────────────────────────────────────────────────────────────────────────────┤
  │ Physical size │ ~63" x 12.6" for a 10x2 panel configuration                                 │
  ├───────────────┼─────────────────────────────────────────────────────────────────────────────┤
  │ Data pin      │ Configurable GPIO (20 valid pins)                                           │
  └───────────────┴─────────────────────────────────────────────────────────────────────────────┘
  ---
  Architecture

  Dual-Core Design (FreeRTOS)

  Core 1 (Protocol CPU)                Core 0 (App CPU)
  ─────────────────────                ─────────────────
  WiFi management                      LED matrix rendering
  HTTP API fetch pipeline              Flight data cycling
  Web server (port 80)                 Calibration/positioning patterns
  Config management                    Status message display
  NVS persistence                      Brightness/color updates

  Cross-core communication uses lock-free FreeRTOS queues and atomic flags — no mutexes between cores, no
  string/vector copies. A double-buffer scheme ensures the display always reads from a stable buffer while
  the fetch task writes to the other.

  Key Components
  Component: ConfigManager
  File(s): core/ConfigManager.cpp/h
  Purpose: NVS-backed settings with hot-reload vs reboot-required distinction
  ────────────────────────────────────────
  Component: WiFiManager
  File(s): adapters/WiFiManager.cpp/h
  Purpose: State machine: AP Setup → Connecting → Connected → Reconnecting → Fallback
  ────────────────────────────────────────
  Component: FlightDataFetcher
  File(s): core/FlightDataFetcher.cpp/h
  Purpose: Orchestrates the 3-API fetch pipeline
  ────────────────────────────────────────
  Component: OpenSkyFetcher
  File(s): adapters/OpenSkyFetcher.cpp/h
  Purpose: OAuth2 state vector fetching from OpenSky Network
  ────────────────────────────────────────
  Component: AeroAPIFetcher
  File(s): adapters/AeroAPIFetcher.cpp/h
  Purpose: Flight detail enrichment from FlightAware AeroAPI
  ────────────────────────────────────────
  Component: FlightWallFetcher
  File(s): adapters/FlightWallFetcher.cpp/h
  Purpose: Airline/aircraft display name lookup from FlightWall CDN
  ────────────────────────────────────────
  Component: NeoMatrixDisplay
  File(s): adapters/NeoMatrixDisplay.cpp/h
  Purpose: Zone-based LED rendering via FastLED_NeoMatrix
  ────────────────────────────────────────
  Component: LayoutEngine
  File(s): core/LayoutEngine.cpp/h
  Purpose: Zone geometry computation (shared algorithm with dashboard JS)
  ────────────────────────────────────────
  Component: LogoManager
  File(s): core/LogoManager.cpp/h
  Purpose: 32x32 RGB565 airline logo storage on LittleFS
  ────────────────────────────────────────
  Component: SystemStatus
  File(s): core/SystemStatus.cpp/h
  Purpose: Health tracking for 6 subsystems
  ────────────────────────────────────────
  Component: WebPortal
  File(s): adapters/WebPortal.cpp/h
  Purpose: REST API + gzipped web asset serving
  Interface Abstractions

  The project uses C++ interfaces for testability:
  - BaseDisplay — display abstraction
  - BaseStateVectorFetcher — state vector source abstraction
  - BaseFlightFetcher — flight enrichment abstraction

  ---
  Data Pipeline

  OpenSky Network API ──► State Vectors (ICAO24, callsign, position, velocity)
           │
           ▼ (per-flight enrichment)
  FlightAware AeroAPI ──► Flight Info (airline, aircraft type, origin/destination)
           │
           ▼ (display name lookup)
  FlightWall CDN ────────► Friendly Names ("United Airlines", "Boeing 737-800")
           │
           ▼ (unit conversion: m→kft, m/s→mph)
  Double Buffer ─────────► FreeRTOS Queue (pointer swap)
           │
           ▼
  LED Matrix Rendering ──► Zone-based display (logo + flight + telemetry)

  API Details
  ┌──────────────────┬─────────────────────┬────────────────┬──────────────────────────────────────────────┐
  │       API        │        Auth         │    Purpose     │                     Data                     │
  ├──────────────────┼─────────────────────┼────────────────┼──────────────────────────────────────────────┤
  │ OpenSky Network  │ OAuth2 client       │ ADS-B state    │ Position, altitude, speed, heading, callsign │
  │                  │ credentials         │ vectors        │                                              │
  ├──────────────────┼─────────────────────┼────────────────┼──────────────────────────────────────────────┤
  │ FlightAware      │ API key header      │ Flight         │ Airline, aircraft type, origin/destination   │
  │ AeroAPI          │                     │ metadata       │ airports                                     │
  ├──────────────────┼─────────────────────┼────────────────┼──────────────────────────────────────────────┤
  │ FlightWall CDN   │ None (public)       │ Display names  │ "United Airlines" instead of "UAL", "B737"   │
  │                  │                     │                │ instead of "B73J"                            │
  └──────────────────┴─────────────────────┴────────────────┴──────────────────────────────────────────────┘
  Geospatial Filtering

  Flights are filtered by a configurable center point and radius:
  - Bounding box query to OpenSky (lat/lon min/max)
  - Haversine distance filter for precise radius
  - Bearing calculation for directional awareness

  ---
  Display System

  Zone Layout

  The LED matrix is divided into three zones with two layout modes:

  Classic (default):
  ┌──────────┬──────────────────┐
  │          │   Flight Zone    │
  │   Logo   │  (airline/route) │
  │   Zone   ├──────────────────┤
  │  (32x32) │ Telemetry Zone   │
  │          │ (alt/spd/trk/vr) │
  └──────────┴──────────────────┘

  Full-width bottom:
  ┌──────────┬──────────────────┐
  │   Logo   │   Flight Zone    │
  │   Zone   │  (airline/route) │
  ├──────────┴──────────────────┤
  │      Telemetry Zone         │
  │   (alt/spd/trk/vrate)      │
  └─────────────────────────────┘

  Adaptive Content Density

  The system automatically adapts content based on matrix height:
  ┌──────────┬─────────┬─────────────────────────────────────┬──────────────────────────────┐
  │   Mode   │ Height  │             Flight Zone             │       Telemetry Format       │
  ├──────────┼─────────┼─────────────────────────────────────┼──────────────────────────────┤
  │ Compact  │ < 32px  │ 1 line: route only                  │ A28k S450 T045 V-12          │
  ├──────────┼─────────┼─────────────────────────────────────┼──────────────────────────────┤
  │ Full     │ 32-47px │ 2 lines: airline + route/aircraft   │ 28.3kft 450mph / 045d -12fps │
  ├──────────┼─────────┼─────────────────────────────────────┼──────────────────────────────┤
  │ Expanded │ ≥ 48px  │ 3 lines: airline + route + aircraft │ Same as full                 │
  └──────────┴─────────┴─────────────────────────────────────┴──────────────────────────────┘
  Zone Customization

  Users can drag zone dividers in the dashboard preview:
  - zone_logo_pct (1-99%): Logo zone width as % of matrix
  - zone_split_pct (1-99%): Flight/telemetry vertical split
  - zone_layout (0 or 1): Classic or full-width-bottom

  Logo System

  - 32x32 RGB565 bitmaps stored on LittleFS (/logos/ directory)
  - Loaded by airline ICAO code (e.g., UAL.bin, DAL.bin)
  - Fallback airplane sprite when logo not found
  - Upload/delete via web dashboard or API

  ---
  Web Dashboard

  Pages
  ┌───────────────┬──────────────┬───────────────────────────────┐
  │     Page      │     URL      │            Purpose            │
  ├───────────────┼──────────────┼───────────────────────────────┤
  │ Setup Wizard  │ / (AP mode)  │ First-time WiFi configuration │
  ├───────────────┼──────────────┼───────────────────────────────┤
  │ Dashboard     │ / (STA mode) │ Full configuration interface  │
  ├───────────────┼──────────────┼───────────────────────────────┤
  │ System Health │ /health.html │ Monitoring and diagnostics    │
  └───────────────┴──────────────┴───────────────────────────────┘
  Dashboard Cards

  1. Display — Brightness slider (0-255), RGB text color pickers with live preview
  2. Timing — Fetch interval (30s-60min), flight cycle duration (1-30s)
  3. Network & API — WiFi credentials, OpenSky OAuth, AeroAPI key
  4. Hardware — Tile dimensions, GPIO pin, wiring flags (origin corner, scan direction, zigzag), layout
  style selector, interactive zone preview canvas with drag handles, panel wiring path visualization
  5. Calibration — Toggle gradient test pattern or panel position guide for hardware setup
  6. Location — Center lat/lon with radius (km) for flight tracking area
  7. Logos — Drag-and-drop upload, manage 32x32 RGB565 airline logos
  8. System — Factory reset

  Unified Apply System

  All changes across cards are tracked as "dirty" and applied together via a single "Apply Changes" bar. The
   dashboard detects which keys require a device reboot and prompts accordingly.

  REST API
  Endpoint: /api/settings
  Method: GET
  Purpose: Read all 23 config keys
  ────────────────────────────────────────
  Endpoint: /api/settings
  Method: POST
  Purpose: Apply config patch (returns reboot_required)
  ────────────────────────────────────────
  Endpoint: /api/status
  Method: GET
  Purpose: Extended health JSON (subsystems, WiFi, device, flight stats, quota)
  ────────────────────────────────────────
  Endpoint: /api/reboot
  Method: POST
  Purpose: Trigger device restart
  ────────────────────────────────────────
  Endpoint: /api/reset
  Method: POST
  Purpose: Factory reset (erase NVS)
  ────────────────────────────────────────
  Endpoint: /api/wifi/scan
  Method: GET
  Purpose: Scan nearby WiFi networks
  ────────────────────────────────────────
  Endpoint: /api/layout
  Method: GET
  Purpose: Computed zone geometry
  ────────────────────────────────────────
  Endpoint: /api/calibration/start|stop
  Method: POST
  Purpose: Toggle calibration pattern
  ────────────────────────────────────────
  Endpoint: /api/positioning/start|stop
  Method: POST
  Purpose: Toggle positioning pattern
  ────────────────────────────────────────
  Endpoint: /api/logos
  Method: GET
  Purpose: List uploaded logos
  ────────────────────────────────────────
  Endpoint: /api/logos/upload
  Method: POST
  Purpose: Upload logo file
  ────────────────────────────────────────
  Endpoint: /api/logos/:filename
  Method: GET/DELETE
  Purpose: Download or delete logo
  ---
  Configuration System

  23 Configuration Keys
  Category: Display (4)
  Keys: brightness, text_color_r/g/b
  Hot-reload?: Yes
  ────────────────────────────────────────
  Category: Timing (2)
  Keys: fetch_interval, display_cycle
  Hot-reload?: Yes
  ────────────────────────────────────────
  Category: Location (3)
  Keys: center_lat, center_lon, radius_km
  Hot-reload?: Yes
  ────────────────────────────────────────
  Category: Hardware wiring (3)
  Keys: origin_corner, scan_dir, zigzag
  Hot-reload?: Yes (hot-reconfigure)
  ────────────────────────────────────────
  Category: Hardware geometry (4)
  Keys: tiles_x, tiles_y, tile_pixels, display_pin
  Hot-reload?: Reboot required
  ────────────────────────────────────────
  Category: Zone layout (3)
  Keys: zone_logo_pct, zone_split_pct, zone_layout
  Hot-reload?: Yes
  ────────────────────────────────────────
  Category: Network (5)
  Keys: wifi_ssid, wifi_password, os_client_id, os_client_sec, aeroapi_key
  Hot-reload?: Reboot required
  Persistence

  - NVS (Non-Volatile Storage): All settings survive reboots
  - Debounced writes: Hot-reload keys persist after 2s quiet period (reduces flash wear)
  - Immediate persist: Reboot-required keys write to NVS immediately
  - Compile-time fallbacks: Every key has a default in config headers

  ---
  Boot Sequence

  Power on
    │
    ├─ BOOT button held? → Force AP setup mode ("Setup Mode — Forced")
    │
    ├─ No WiFi credentials? → AP setup mode ("Setup Mode")
    │
    └─ Credentials exist → "Connecting to WiFi..."
                               │
                               ├─ Success → "WiFi Connected ✓" → IP display
                               │             → "Authenticating APIs..."
                               │             → "Fetching flights..."
                               │             → Render flight data
                               │
                               └─ Failure (12 retries) → AP fallback mode

  ---
  Health Monitoring

  Tracks 6 subsystems with 3 severity levels (OK / WARNING / ERROR):
  ┌───────────┬──────────────────────────────────────────┐
  │ Subsystem │                 Monitors                 │
  ├───────────┼──────────────────────────────────────────┤
  │ WiFi      │ Connection state, RSSI, IP, mode         │
  ├───────────┼──────────────────────────────────────────┤
  │ OpenSky   │ State vector fetch success, vector count │
  ├───────────┼──────────────────────────────────────────┤
  │ AeroAPI   │ Flight enrichment success/fail ratio     │
  ├───────────┼──────────────────────────────────────────┤
  │ CDN       │ Display name lookup success/fail ratio   │
  ├───────────┼──────────────────────────────────────────┤
  │ NVS       │ Config persistence health                │
  ├───────────┼──────────────────────────────────────────┤
  │ LittleFS  │ Filesystem mount, storage usage          │
  └───────────┴──────────────────────────────────────────┘
  The health page also tracks API quota: fetches since boot, monthly projection, and over-pace warnings
  (AeroAPI has usage limits).

  ---
  Build System

  - Platform: PlatformIO with espressif32 framework
  - Filesystem: LittleFS (custom partition table)
  - Dependencies: FastLED 3.6, Adafruit GFX 1.11, FastLED NeoMatrix 1.2, ArduinoJson 7.4, ESPAsyncWebServer
  3.6
  - Web assets: Source in data-src/, manually gzipped to data/ for serving
  - Build: ~/.platformio/penv/bin/pio run
  - Upload FS: ~/.platformio/penv/bin/pio run -t uploadfs

  ---
  Project File Structure

  firmware/
  ├── src/main.cpp                    # Entry point, dual-core orchestration
  ├── adapters/                       # External system adapters
  │   ├── WiFiManager.cpp/h           # WiFi state machine
  │   ├── NeoMatrixDisplay.cpp/h      # LED matrix rendering
  │   ├── WebPortal.cpp/h             # REST API + web serving
  │   ├── OpenSkyFetcher.cpp/h        # OpenSky API (OAuth2)
  │   ├── AeroAPIFetcher.cpp/h        # FlightAware API
  │   └── FlightWallFetcher.cpp/h     # CDN display names
  ├── core/                           # Business logic
  │   ├── ConfigManager.cpp/h         # NVS config with hot-reload
  │   ├── FlightDataFetcher.cpp/h     # Fetch pipeline orchestrator
  │   ├── LayoutEngine.cpp/h          # Zone geometry computation
  │   ├── LogoManager.cpp/h           # Airline logo filesystem
  │   └── SystemStatus.cpp/h          # Health monitoring
  ├── config/                         # Compile-time defaults
  │   ├── APIConfiguration.h          # API URLs and credentials
  │   ├── HardwareConfiguration.h     # Display pin and tile config
  │   ├── UserConfiguration.h         # Location and display defaults
  │   ├── TimingConfiguration.h       # Fetch/cycle intervals
  │   └── WiFiConfiguration.h         # WiFi defaults
  ├── interfaces/                     # Abstract base classes
  │   ├── BaseDisplay.h
  │   ├── BaseFlightFetcher.h
  │   └── BaseStateVectorFetcher.h
  ├── models/                         # Data structures
  │   ├── FlightInfo.h                # Enriched flight data
  │   ├── StateVector.h               # Raw ADS-B data
  │   └── AirportInfo.h               # Airport code
  ├── utils/                          # Helpers
  │   ├── GeoUtils.h                  # Haversine, bounding box, bearing
  │   └── Log.h                       # Log level macros
  ├── data-src/                       # Web asset sources
  │   ├── dashboard.html/js           # Configuration dashboard
  │   ├── health.html/js              # System health page
  │   ├── wizard.html/js              # First-time setup wizard
  │   ├── common.js                   # Shared HTTP/toast utilities
  │   └── style.css                   # Shared styles
  └── data/                           # Gzipped assets served by WebPortal
      ├── *.html.gz, *.js.gz, *.css.gz
      └── logos/                      # Uploaded airline logos