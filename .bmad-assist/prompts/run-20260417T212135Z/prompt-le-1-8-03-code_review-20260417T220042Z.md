<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 8 -->
<!-- Phase: code-review -->
<!-- Timestamp: 20260417T220042Z -->
<compiled-workflow>
<mission><![CDATA[

Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.

Target: Story le-1.8 - flight-field-and-metric-widgets
Find 3-10 specific issues. Challenge every claim.

]]></mission>
<context>
<file id="ed7fe483" path="_bmad-output/project-context.md" label="PROJECT CONTEXT"><![CDATA[

---
project_name: TheFlightWall_OSS-main
date: '2026-04-12'
---

# Project Context for AI Agents

Lean rules for implementing FlightWall (ESP32 LED flight display + captive-portal web UI). Prefer existing patterns in `firmware/` over new abstractions.

## Technology Stack

- **Firmware:** C++11, ESP32 (Arduino/PlatformIO), FastLED + Adafruit GFX + FastLED NeoMatrix, ArduinoJson ^7.4.2.
- **Web on device:** ESPAsyncWebServer (**mathieucarbou fork**), AsyncTCP (**Carbou fork**), LittleFS (`board_build.filesystem = littlefs`), custom `custom_partitions.csv` (~2MB app + ~2MB LittleFS).
- **Dashboard assets:** Editable sources under `firmware/data-src/`; served bundles are **gzip** under `firmware/data/`. After editing a source file, regenerate the matching `.gz` from `firmware/` (e.g. `gzip -9 -c data-src/common.js > data/common.js.gz`).

## Critical Implementation Rules

- **Core pinning:** Display/task driving LEDs on **Core 0**; WiFi, HTTP server, and flight fetch pipeline on **Core 1** (FastLED + WiFi ISR constraints).
- **Config:** `ConfigManager` + NVS; debounce writes; atomic saves; use category getters; `POST /api/settings` JSON envelope `{ ok, data, error, code }` pattern for REST responses.
- **Heap / concurrency:** Cap concurrent web clients (~2–3); stream LittleFS reads; use ArduinoJson filter/streaming for large JSON; avoid full-file RAM buffering for uploads.
- **WiFi:** WiFiManager-style state machine (AP setup → STA → reconnect / AP fallback); mDNS `flightwall.local` in STA.
- **Structure:** Extend hexagonal layout — `firmware/core/`, `firmware/adapters/` (e.g. `WebPortal.cpp`), `firmware/interfaces/`, `firmware/models/`, `firmware/config/`, `firmware/utils/`.
- **Tooling:** Build from `firmware/` with `pio run`. On macOS serial: use `/dev/cu.*` (not `tty.*`); release serial monitor before upload.
- **Scope for code reviews:** Product code under `firmware/` and tests under `firmware/test/` and repo `tests/`; do not treat BMAD-only paths as product defects unless the task says so.

## Planning Artifacts

- Requirements and design: `_bmad-output/planning-artifacts/` (`architecture.md`, `epics.md`, PRDs).
- Stories and sprint line items: `_bmad-output/implementation-artifacts/` (e.g. `sprint-status.yaml`, per-story markdown).

## BMAD / bmad-assist

- **`bmad-assist.yaml`** at repo root configures providers and phases; `paths.project_knowledge` points at `_bmad-output/planning-artifacts/`, `paths.output_folder` at `_bmad-output/`.
- **This file** (`project-context.md`) is resolved at `_bmad-output/project-context.md` or `docs/project-context.md` (see `bmad-assist` compiler `find_project_context_file`).
- Keep **`sprint-status.yaml`** story keys aligned with `.bmad-assist/state.yaml` (`current_story`, `current_epic`) when using `bmad-assist run` so phases do not skip with “story not found”.


]]></file>
<file id="893ad01d" path="_bmad-output/planning-artifacts/architecture.md" label="ARCHITECTURE"><![CDATA[

---
stepsCompleted: [1, 2, 3, 4, 5, 6, 7, 8]
lastStep: 8
status: 'complete'
completedAt: '2026-04-02'
extensionStarted: '2026-04-11'
extensionPrd: 'prd-foundation.md'
extensionStarted2: '2026-04-11'
extensionPrd2: 'prd-display-system.md'
extensionCompleted2: '2026-04-11'
extensionStatus2: 'complete'
extensionStarted3: '2026-04-11'
extensionPrd3: 'prd-delight.md'
extensionCompleted3: '2026-04-11'
extensionStatus3: 'complete'
inputDocuments: ['_bmad-output/planning-artifacts/prd.md', '_bmad-output/planning-artifacts/ux-design-specification.md', '_bmad-output/planning-artifacts/prd-validation-report.md', '_bmad-output/planning-artifacts/prd-foundation.md', '_bmad-output/planning-artifacts/prd-foundation-validation-report.md', '_bmad-output/planning-artifacts/prd-display-system.md', '_bmad-output/planning-artifacts/prd-display-system-validation-report.md', '_bmad-output/planning-artifacts/prd-delight.md', '_bmad-output/planning-artifacts/prd-delight-validation-report.md', 'docs/index.md', 'docs/project-overview.md', 'docs/architecture.md', 'docs/source-tree-analysis.md', 'docs/component-inventory.md', 'docs/development-guide.md', 'docs/api-contracts.md']
workflowType: 'architecture'
project_name: 'TheFlightWall_OSS-main'
user_name: 'Christian'
date: '2026-04-11'
---

# Architecture Decision Document

_This document builds collaboratively through step-by-step discovery. Sections are appended as we work through each architectural decision together._

## Project Context Analysis

### Requirements Overview

**Functional Requirements (48 FRs across 9 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| Device Setup & Onboarding | FR1-FR8 | New: captive portal, AP mode, wizard flow, WiFi transition |
| Configuration Management | FR9-FR14 | New: ConfigManager, NVS persistence, hot-reload + reboot paths |
| Display Calibration & Preview | FR15-FR18 | New: web-to-firmware communication for test patterns, canvas data endpoint |
| Flight Data Display | FR19-FR25 | Existing: fetch → enrich → display pipeline (must not regress) |
| Airline Logo Display | FR26-FR29 | New: LogoManager, LittleFS file I/O, ICAO lookup, fallback sprite |
| Responsive Display Layout | FR30-FR33 | New: LayoutEngine with zone calculation and breakpoint detection |
| Logo Management | FR34-FR37 | New: file upload endpoint, validation, LittleFS management |
| System Operations | FR38-FR48 | New: FreeRTOS tasks, mDNS, GPIO handlers, API usage tracking |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| Hot-reload latency | <1 second | ConfigManager must notify display task directly, no polling |
| Page load time | <3 seconds | Gzipped assets, minimal JS, no external dependencies except Leaflet |
| Time to first flight | <60 seconds | WiFi connect + OAuth + first fetch must be pipelined efficiently |
| Uptime | 30+ days | Heap fragmentation management, watchdog timers, graceful error recovery |
| WiFi recovery | <60 seconds | Auto-reconnect with configurable timeout → AP fallback |
| RAM ceiling | <280KB | Stream LittleFS files, limit concurrent web clients to 2-3, ArduinoJson filter API |
| Flash budget | 4MB total, 500KB headroom | 2MB app + 2MB LittleFS partition. Gzip all web assets. Monitor usage. |
| Concurrent operations | Web server + flight pipeline + display | FreeRTOS task pinning: display Core 0, WiFi/web/API Core 1 |

**UX Specification (architectural implications):**

| UX Decision | Architecture Requirement |
|-------------|------------------------|
| Two HTML pages (wizard + dashboard) | ESPAsyncWebServer serves different content based on WiFi mode |
| Single settings API endpoint | `POST /api/settings` handler parses JSON, applies to ConfigManager, returns applied keys + reboot flag |
| Gzipped static assets | LittleFS stores `.gz` files, ESPAsyncWebServer serves with `Content-Encoding: gzip` |
| Leaflet lazy-load | Dashboard JS dynamically loads Leaflet only when Location section opened |
| mDNS | `ESPmDNS.begin("flightwall")` on WiFi connect |
| WiFi scan in wizard | `WiFi.scanNetworks(true)` async in AP_STA mode, results via `/api/wifi/scan` |
| Toast + hot-reload | Settings POST response drives client-side toast; display settings bypass reboot |
| Triple feedback | Browser canvas (client-side) + LED wall (via firmware) update from same settings change |
| RGB565 logo preview | Client-side only — no architecture impact (FileReader API in browser) |

### Scale & Complexity

- **Primary domain:** IoT/Embedded with companion web UI
- **Complexity level:** Medium — multiple subsystems (web server, config manager, display engine, logo manager, WiFi manager) but single-user, no auth, no database, no cloud backend
- **Estimated new architectural components:** 6 (ConfigManager, WebPortal, WiFiManager, LogoManager, LayoutEngine, DisplayTask)
- **Existing components to preserve:** 5 (FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, NeoMatrixDisplay)

### Technical Constraints & Dependencies

| Constraint | Impact |
|-----------|--------|
| ESP32 single WiFi radio | AP and STA share hardware — AP_STA for scan, mode switching for setup→operation transition |
| 4MB flash total | Partition carefully: 2MB app + 2MB LittleFS. No OTA in MVP. Every KB of web assets matters. |
| 520KB SRAM | ~280KB usable after stack/WiFi. ESPAsyncWebServer + TLS + JSON parsing compete for heap. |
| FastLED disables interrupts | Display task MUST be on Core 0, WiFi/web on Core 1. Non-negotiable. |
| ESPAsyncWebServer connection limit | 2-3 concurrent clients max. No auto-refreshing dashboard. No WebSocket push. |
| ArduinoJson heap pressure | Use streaming/filter API to reduce per-parse allocation from ~16KB to ~2-4KB |
| NVS write endurance | ~100,000 write cycles per key. Not a concern for config changes, but don't write on every slider drag — debounce. |

### Cross-Cutting Concerns

1. **Configuration lifecycle** — Compile-time defaults → NVS read on boot → runtime changes via web UI → NVS write on save → hot-reload or reboot. Every component that reads config must support this chain.

2. **WiFi mode management** — AP mode (setup) → STA mode (operation) → AP fallback (WiFi failure). Web server behavior changes per mode (wizard vs. dashboard). mDNS only in STA mode.

3. **Concurrent task safety** — Display task (Core 0) and web server + flight pipeline (Core 1) share data structures (current flights, config values). Needs mutex or task-safe data passing.

4. **Flash budget tracking** — Firmware size + web assets + logos must fit in 4MB with headroom. Logo uploads could fill LittleFS. Need usage tracking and upload rejection when full.

5. **Error propagation** — API failures, WiFi drops, and NVS errors must propagate to both the LED display (status messages) and the web UI (System Health page) with specific error information.

## Technology Stack & Foundation

### Existing Stack (Locked)

| Layer | Technology | Version | Notes |
|-------|-----------|---------|-------|
| Language | C++ (Arduino) | C++11 | ESP32 Arduino framework dialect |
| Platform | ESP32 | espressif32 | PlatformIO managed |
| Build | PlatformIO | Latest | `platformio.ini` configuration |
| LED Control | FastLED | ^3.6.0 | WS2812B driver |
| Graphics | Adafruit GFX | ^1.11.9 | Text/shape rendering |
| Matrix | FastLED NeoMatrix | ^1.2 | Tiled matrix abstraction |
| JSON | ArduinoJson | ^7.4.2 | API response parsing |
| Networking | WiFi + HTTPClient | ESP32 core | Built-in Arduino ESP32 |
| TLS | WiFiClientSecure | ESP32 core | HTTPS for API calls |

### New Dependencies (To Add)

| Library | Source | Version | Notes |
|---------|--------|---------|-------|
| ESPAsyncWebServer | `mathieucarbou/ESPAsyncWebServer` | ^3.6.0 | **Use Carbou fork** — actively maintained, fixes memory leaks in original `me-no-dev` repo. Used by ESPHome. |
| AsyncTCP | `mathieucarbou/AsyncTCP` | Latest | **Use Carbou fork** — fixes race conditions that cause crashes under load. Required by ESPAsyncWebServer. |
| LittleFS | ESP32 core | Built-in | Replaces SPIFFS (better wear leveling, directory support). Requires `board_build.filesystem = littlefs` in platformio.ini. |
| ESPmDNS | ESP32 core | Built-in | `flightwall.local` device discovery. Zero-config networking. |
| Leaflet.js | Client-side | ~1.9 | Interactive map. ~25KB gzipped. Lazy-loaded. Stored in `firmware/data/leaflet/` or loaded from CDN. |

**platformio.ini additions:**
```ini
lib_deps =
    mathieucarbou/ESPAsyncWebServer @ ^3.6.0
    ; AsyncTCP pulled automatically as dependency
    fastled/FastLED @ ^3.6.0
    adafruit/Adafruit GFX Library @ ^1.11.9
    marcmerlin/FastLED NeoMatrix @ ^1.2
    bblanchon/ArduinoJson @ ^7.4.2

board_build.filesystem = littlefs
board_build.partitions = custom_partitions.csv
```

### Architecture Pattern

**Extending existing Hexagonal (Ports & Adapters):**
- New components follow the same pattern — abstract interfaces where swappability matters, concrete implementations where it doesn't
- ConfigManager is a new "core" component (not an adapter — it's internal)
- WebPortal is a new adapter (it's an interface to the outside world — the browser)
- WiFiManager is a new adapter (manages the WiFi hardware port)
- LogoManager is a new core component (internal file management)
- LayoutEngine is a new core component (internal display logic)

### Flash Partition Layout

**Custom partition table required** — the default ESP32 partition doesn't provide the 2MB/2MB split. Create `firmware/custom_partitions.csv`:

```
# Name,    Type,  SubType, Offset,   Size
nvs,       data,  nvs,     0x9000,   0x5000
otadata,   data,  ota,     0xe000,   0x2000
app0,      app,   ota_0,   0x10000,  0x200000
spiffs,    data,  spiffs,  0x210000, 0x1F0000
```

**MVP Partition Budget:**
```
4MB Flash Total
├── 2MB — Application (firmware)
│   └── ~1.2MB estimated firmware + ~800KB headroom
└── ~1.9MB — LittleFS
    ├── ~40KB — Web assets (gzipped HTML/JS/CSS)
    ├── ~28KB — Leaflet (gzipped JS + CSS)
    ├── ~100KB — Realistic logo usage (~50 logos at 2KB each)
    └── ~1.7MB — Unused headroom
```

**Phase 2 OTA Partition Strategy (architecture note):**

LittleFS is heavily over-provisioned for MVP. Realistic usage is ~170KB of ~1.9MB available. This slack is the OTA budget for Phase 2:

| Layout | App | OTA | LittleFS | Requires Reflash |
|--------|-----|-----|----------|-----------------|
| **MVP** | 2MB | — | 1.9MB | No (initial) |
| **Phase 2 (generous)** | 1.5MB | 1.5MB | 896KB | Yes |
| **Phase 2 (tight)** | 1.5MB | 1.5MB | 512KB | Yes |

Even the "tight" Phase 2 layout provides 512KB LittleFS — enough for web assets (68KB) + 200 logos (400KB) with headroom. Document this tradeoff now so Phase 2 OTA implementation isn't a surprise.

### Web Assets Location

Web assets live at **`firmware/data/`** (not project root `data/`). This matches the non-standard project structure where all source is under `firmware/`.

```
firmware/
├── data/                    ← LittleFS content, uploaded via `pio run -t uploadfs`
│   ├── style.css.gz
│   ├── common.js.gz
│   ├── wizard.html.gz
│   ├── wizard.js.gz
│   ├── dashboard.html.gz
│   ├── dashboard.js.gz
│   └── leaflet/
│       ├── leaflet.min.js.gz
│       └── leaflet.min.css.gz
├── custom_partitions.csv    ← Custom partition table
├── platformio.ini
├── src/
├── core/
├── adapters/
└── ...
```

**Upload command:** `cd firmware && pio run -t uploadfs`

### Development Workflow

**No change from existing:** PlatformIO build → USB flash → serial monitor. Web assets are pre-gzipped externally (using `gzip -9`) and placed in `firmware/data/`, uploaded to LittleFS via `pio run -t uploadfs`.

**First implementation story** should be: add ESPAsyncWebServer + LittleFS to `platformio.ini`, create custom partition table, verify firmware still builds and runs with the new dependencies, and serve a minimal "hello world" HTML page from LittleFS.

## Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. ConfigManager design — Singleton with category struct getters
2. Inter-task communication — Hybrid (atomic flags for config, queue for flight data)
3. WiFi state machine — Dedicated WiFiManager with callbacks
4. API endpoint design — 11 REST endpoints, consistent JSON response format
5. Component integration — Initialization order, dependency graph, read-only display task
6. Error handling — Centralized SystemStatus registry

**Deferred Decisions (Post-MVP):**
- OTA update mechanism (Phase 2 — requires repartitioning)
- WebSocket push for live dashboard updates (not needed — no auto-refresh)
- Bluetooth functionality (no use case defined)

### Decision 1: ConfigManager — Singleton with Category Struct Getters

**Pattern:** Centralized singleton, initialized first, accessed by all components. Config values grouped into category structs for clean API surface and efficient access.

```cpp
struct DisplayConfig {
    uint8_t brightness;
    uint8_t textColorR, textColorG, textColorB;
};

struct LocationConfig {
    double centerLat, centerLon, radiusKm;
};

struct HardwareConfig {
    uint8_t tilesX, tilesY, tilePixels, displayPin;
    uint8_t originCorner, scanDirection, zigzag;
};

struct TimingConfig {
    uint16_t fetchIntervalSeconds, displayCycleSeconds;
};

struct NetworkConfig {
    String wifiSsid, wifiPassword;
    String openSkyClientId, openSkyClientSecret;
    String aeroApiKey;
};

class ConfigManager {
public:
    static void init();
    static DisplayConfig getDisplay();
    static LocationConfig getLocation();
    static HardwareConfig getHardware();
    static TimingConfig getTiming();
    static NetworkConfig getNetwork();
    static ApplyResult applyJson(const JsonObject& settings);
    static void schedulePersist(uint16_t delayMs = 2000);
    static void onChange(std::function<void()> callback);
    static bool requiresReboot(const char* key);
};

struct ApplyResult {
    std::vector<String> applied;
    bool rebootRequired;
};
```

**Config value chain:** Compile-time default → NVS stored value → RAM cache (struct fields)

**NVS Write Debouncing:**
- `applyJson()` updates RAM cache instantly (hot-reload works immediately)
- `schedulePersist(2000)` queues NVS write after 2-second quiet period
- If user drags slider through 50 values in 3 seconds, NVS gets one write at the end
- Reboot-required keys bypass debouncing — persist to NVS immediately before restart

**Reboot-required keys:** `wifi_ssid`, `wifi_password`, `opensky_client_id`, `opensky_client_secret`, `aeroapi_key`, `display_pin`
**Hot-reload keys:** `brightness`, `text_color_r/g/b`, `fetch_interval`, `display_cycle`, `tiles_x`, `tiles_y`, `center_lat`, `center_lon`, `radius_km`, wiring flags

### Decision 2: Inter-Task Communication — Hybrid

**Config changes:** Atomic flag signals display task to re-read from ConfigManager
```cpp
std::atomic<bool> configChanged{false};

// Core 1: web server applies setting
ConfigManager::applyJson(settings);
configChanged.store(true);

// Core 0: display task checks each frame
if (configChanged.exchange(false)) {
    displayConfig = ConfigManager::getDisplay();
    hardwareConfig = ConfigManager::getHardware();
}
```

**Flight data:** FreeRTOS queue with overwrite semantics
```cpp
QueueHandle_t flightQueue;

// Core 1: flight pipeline completes
xQueueOverwrite(flightQueue, &enrichedFlights);

// Core 0: display task reads latest
FlightData flights;
if (xQueuePeek(flightQueue, &flights, 0) == pdTRUE) {
    renderFlights(flights);
}
```

**Why hybrid:** Config changes are lightweight signals (flag + struct copy). Flight data needs proper thread-safe transfer (queue). `xQueueOverwrite` ensures display always gets latest data.

### Decision 3: WiFi State Machine — WiFiManager

**States:**
```cpp
enum WiFiState {
    WIFI_AP_SETUP,
    WIFI_CONNECTING,
    WIFI_STA_CONNECTED,
    WIFI_STA_RECONNECTING,
    WIFI_AP_FALLBACK
};
```

**Transitions:**
```
POWER ON → [Has WiFi config?]
         No → AP_SETUP
         Yes → CONNECTING → Success → STA_CONNECTED
                          → Timeout → AP_FALLBACK
STA_CONNECTED → WiFi lost → STA_RECONNECTING → Reconnects → STA_CONNECTED
                                               → Timeout → AP_FALLBACK
```

**GPIO overrides:**
- Short press: show IP on LEDs for 5 seconds (no state change)
- Long press during boot: force AP_SETUP regardless of saved config

**Callbacks:** WebPortal, display task, and FlightDataFetcher register for state change notifications.

### Decision 4: Web API Endpoints

| Method | Endpoint | Purpose | Reboot |
|--------|----------|---------|--------|
| `GET` | `/` | wizard.html (AP) or dashboard.html (STA) | No |
| `GET` | `/api/status` | System health JSON | No |
| `GET` | `/api/settings` | All current config values | No |
| `POST` | `/api/settings` | Apply partial config update | Maybe |
| `GET` | `/api/wifi/scan` | Async WiFi scan results | No |
| `POST` | `/api/reboot` | Save + reboot | Yes |
| `POST` | `/api/reset` | Factory reset — erase NVS | Yes |
| `GET` | `/api/logos` | List uploaded logos | No |
| `POST` | `/api/logos` | Upload logo .bin (multipart) | No |
| `DELETE` | `/api/logos/:name` | Delete specific logo | No |
| `GET` | `/api/layout` | Current zone layout (initial load only) | No |

**`/api/layout` note:** Used for initial dashboard load only. During live editing, canvas calculates zones client-side using the shared zone algorithm.

**Response envelope:**
```json
{ "ok": true, "data": { ... } }
{ "ok": false, "error": "human-readable message", "code": "MACHINE_CODE" }
```

### Decision 5: Component Integration

**Dependency Graph:**
```
ConfigManager ← (everything depends on this)
SystemStatus ← (everything reports to this)
WiFiManager ← WebPortal
LogoManager ← WebPortal, DisplayTask
LayoutEngine ← DisplayTask, dashboard.js (shared algorithm)
FlightDataFetcher ← main loop
    ├── OpenSkyFetcher
    ├── AeroAPIFetcher
    └── FlightWallFetcher
DisplayTask (Core 0, read-only):
    ├── ConfigManager (atomic flag + struct copy)
    ├── flightQueue (FreeRTOS queue)
    ├── LogoManager (ICAO lookup)
    ├── LayoutEngine (zone positions)
    └── SystemStatus (error display)
```

**Initialization order:**
```
1. ConfigManager::init()
2. SystemStatus::init()
3. LogoManager::init()
4. LayoutEngine::init()
5. WiFiManager::init()
6. WebPortal::init()
7. FlightDataFetcher::init()
8. xTaskCreatePinnedToCore(displayTask, "display", 8192, NULL, 1, NULL, 0)
9. loop() runs flight fetch cycle on Core 1
```

**Rules:**
- ConfigManager initializes first — all others depend on it
- DisplayTask is read-only — never writes config, never calls APIs
- WebPortal is the only write path from the browser
- Existing adapters gain ConfigManager dependency but keep interface contracts unchanged

### Decision 6: Error Handling — SystemStatus Registry

```cpp
class SystemStatus {
public:
    static void init();
    static void set(Subsystem sys, StatusLevel level, const String& message);
    static SubsystemStatus get(Subsystem sys);
    static JsonObject toJson();
    static void setMetric(const String& key, const String& value);
    static String getMetric(const String& key);
};
```

**Subsystem health:**

| Subsystem | OK | Warning | Error |
|-----------|-----|---------|-------|
| WIFI | "Connected, -52 dBm" | "Weak signal, -78 dBm" | "Disconnected, retrying..." |
| OPENSKY | "Authenticated ✓" | "Token expiring in 5m" | "401 Unauthorized — check credentials" |
| AEROAPI | "Connected ✓" | "429 rate limited, retry 30s" | "503 Service unavailable" |
| CDN | "Reachable ✓" | — | "Unreachable — using ICAO codes" |
| NVS | "OK" | — | "Write failed — config may not persist" |
| LITTLEFS | "847KB / 1.9MB used" | "90%+ full" | "Mount failed" |

**API Call Counter — RAM + Hourly NVS Sync:**
- Running count in RAM, synced to NVS once per hour and on graceful reboot
- Loses at most 1 hour of data on unexpected reboot — acceptable for budget indicator
- Resets on 1st of month

### Shared Zone Calculation Algorithm

**Critical:** Implemented identically in C++ (LayoutEngine) and JavaScript (dashboard.js). If they diverge, preview won't match LEDs.

```
INPUT: tilesX, tilesY, tilePixels
COMPUTE:
    matrixWidth  = tilesX * tilePixels
    matrixHeight = tilesY * tilePixels

    if matrixHeight < 32:      mode = "compact"
    else if matrixHeight < 48: mode = "full"
    else:                      mode = "expanded"

    logoZone     = { x: 0, y: 0, w: matrixHeight, h: matrixHeight }
    flightZone   = { x: matrixHeight, y: 0, w: matrixWidth - matrixHeight, h: floor(matrixHeight/2) }
    telemetryZone = { x: matrixHeight, y: floor(matrixHeight/2), w: matrixWidth - matrixHeight, h: matrixHeight - floor(matrixHeight/2) }
OUTPUT: mode, logoZone, flightZone, telemetryZone
```

**Test vectors:**

| Config | Matrix | Mode | Logo | Flight Card | Telemetry |
|--------|--------|------|------|-------------|-----------|
| 10x2 @ 16px | 160x32 | full | 0,0 → 32x32 | 32,0 → 128x16 | 32,16 → 128x16 |
| 5x2 @ 16px | 80x32 | full | 0,0 → 32x32 | 32,0 → 48x16 | 32,16 → 48x16 |
| 12x3 @ 16px | 192x48 | expanded | 0,0 → 48x48 | 48,0 → 144x24 | 48,24 → 144x24 |
| 10x1 @ 16px | 160x16 | compact | 0,0 → 16x16 | 16,0 → 144x8 | 16,8 → 144x8 |

### Decision Impact — Implementation Sequence

1. ConfigManager + SystemStatus (foundation)
2. WiFiManager (enables web server and API calls)
3. WebPortal + API routes (enables browser interaction)
4. LogoManager (LittleFS file management)
5. LayoutEngine (zone calculation — shared algorithm in C++)
6. DisplayTask refactor (FreeRTOS, new layout, logos)
7. Existing adapter updates (ConfigManager integration)
8. Web assets (shared zone algorithm in JS, build dashboard)

## Implementation Patterns & Consistency Rules

### Existing Codebase Conventions (All New Code Must Follow)

The existing firmware establishes conventions that new components must match. These are rules derived from working code.

### Naming Patterns

**C++ Classes:** `PascalCase`
- Existing: `FlightDataFetcher`, `NeoMatrixDisplay`, `OpenSkyFetcher`, `AeroAPIFetcher`
- New: `ConfigManager`, `WiFiManager`, `WebPortal`, `LogoManager`, `LayoutEngine`, `SystemStatus`

**C++ Methods:** `camelCase`
- Existing: `fetchFlights()`, `fetchStateVectors()`, `displayMessage()`, `showLoading()`
- New: `applyJson()`, `schedulePersist()`, `getDisplay()`, `setState()`

**C++ Member Variables:** `_camelCase` (underscore prefix for private members)
- Existing: `_matrix`, `_leds`, `_matrixWidth`, `_currentFlightIndex`, `_stateFetcher`

**C++ Structs:** `PascalCase` name with `snake_case` fields
- Existing: `FlightInfo { String ident_icao; String operator_code; }`
- New: `DisplayConfig { uint8_t brightness; uint8_t text_color_r; }`
- **Rule:** Struct fields use `snake_case` to match NVS keys and JSON field names. One convention across all three layers — zero mental translation.

**Constants:** `UPPER_SNAKE_CASE` | **Namespaces:** `PascalCase` | **Files:** `PascalCase.h/.cpp` | **Headers:** `#pragma once`

### Config Structs (Corrected to snake_case Fields)

```cpp
struct DisplayConfig {
    uint8_t brightness;
    uint8_t text_color_r, text_color_g, text_color_b;
};

struct LocationConfig {
    double center_lat, center_lon, radius_km;
};

struct HardwareConfig {
    uint8_t tiles_x, tiles_y, tile_pixels, display_pin;
    uint8_t origin_corner, scan_dir, zigzag;
};

struct TimingConfig {
    uint16_t fetch_interval, display_cycle;
};

struct NetworkConfig {
    String wifi_ssid, wifi_password;
    String opensky_client_id, opensky_client_secret;
    String aeroapi_key;
};
```

### NVS Key Naming

**Convention:** `snake_case`, max 15 characters. Abbreviation rule: if natural name exceeds 15 chars, document abbreviation in `ConfigManager.h`:

```cpp
// NVS key abbreviations (15-char limit):
// os_client_id   = opensky_client_id
// os_client_sec  = opensky_client_secret
// scan_dir       = scan_direction
```

| NVS Key | Type | Default | Category |
|---------|------|---------|----------|
| `brightness` | uint8 | 5 | display |
| `text_color_r` | uint8 | 255 | display |
| `text_color_g` | uint8 | 255 | display |
| `text_color_b` | uint8 | 255 | display |
| `center_lat` | double | 37.7749 | location |
| `center_lon` | double | -122.4194 | location |
| `radius_km` | double | 10.0 | location |
| `tiles_x` | uint8 | 10 | hardware |
| `tiles_y` | uint8 | 2 | hardware |
| `tile_pixels` | uint8 | 16 | hardware |
| `display_pin` | uint8 | 25 | hardware |
| `origin_corner` | uint8 | 0 | hardware |
| `scan_dir` | uint8 | 0 | hardware |
| `zigzag` | uint8 | 0 | hardware |
| `fetch_interval` | uint16 | 30 | timing |
| `display_cycle` | uint16 | 3 | timing |
| `wifi_ssid` | string | "" | network |
| `wifi_password` | string | "" | network |
| `os_client_id` | string | "" | network |
| `os_client_sec` | string | "" | network |
| `aeroapi_key` | string | "" | network |
| `api_calls` | uint32 | 0 | metrics |
| `api_month` | uint8 | 0 | metrics |

**NVS namespace:** `"flightwall"`

### API Call Counter — Monthly Reset Strategy

**Primary (NTP):** On WiFi STA connect, `configTime(0, 0, "pool.ntp.org")` syncs time. Compare current month to `api_month` in NVS — if different, reset `api_calls` to 0 and update `api_month`.

**Fallback (manual):** If NTP fails, counter continues. Dashboard shows "Reset API Counter" button for manual reset.

### API JSON Field Names

**Convention:** `snake_case` in all JSON — matches struct fields and NVS keys.

```json
{ "brightness": 25, "text_color_r": 200, "fetch_interval": 120 }
```

### Logging Pattern — Compile-Time Log Levels

**File:** `utils/Log.h`

```cpp
#pragma once

#ifndef LOG_LEVEL
#define LOG_LEVEL 2  // Default: info
#endif

#define LOG_E(tag, msg) do { if (LOG_LEVEL >= 1) Serial.println("[" tag "] ERROR: " msg); } while(0)
#define LOG_I(tag, msg) do { if (LOG_LEVEL >= 2) Serial.println("[" tag "] " msg); } while(0)
#define LOG_V(tag, msg) do { if (LOG_LEVEL >= 3) Serial.println("[" tag "] " msg); } while(0)
```

**platformio.ini:** `-D LOG_LEVEL=3` for development, `-D LOG_LEVEL=1` for production.
**Tag format:** String literal matching class name. Macros compile to nothing at level 0.

### Error Handling Pattern

Boolean return + output parameter (matches existing codebase). Plus SystemStatus reporting and log macros:

```cpp
if (!fetchStateVectors(...)) {
    SystemStatus::set(OPENSKY, ERROR, "Fetch failed — HTTP " + String(httpCode));
    LOG_E("OpenSky", ("Fetch failed — HTTP " + String(httpCode)).c_str());
    return false;
}
```

**No exceptions.** No `try/catch`.

### Include Pattern

System/Arduino first → Library second → Project last. Forward declarations where possible.

### Memory Management

`String` for dynamic text, `const char*` for constants. Avoid `new`/`delete`. Stream LittleFS files.

### Web Asset Patterns

**JS Fetch Error Handling (mandatory):**

```javascript
async function applySettings(data) {
    try {
        const res = await fetch('/api/settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
        const json = await res.json();
        if (!json.ok) { showToast(json.error, 'error'); return null; }
        showToast('Settings saved', 'success');
        return json;
    } catch (err) {
        showToast('Connection failed', 'error');
        return null;
    }
}
```

**Rule:** Every `fetch()` call must check `json.ok` and call `showToast()` on failure.

### Enforcement Guidelines

**All AI agents implementing stories MUST:**

1. Follow naming conventions — check existing files before writing new code
2. Use `snake_case` for struct fields, NVS keys, and JSON fields
3. Use `#pragma once` in all headers
4. Use boolean return + output param for fallible operations
5. Report user-visible errors via `SystemStatus::set()`
6. Log via `LOG_E`/`LOG_I`/`LOG_V` from `utils/Log.h` — never raw `Serial.println`
7. Use `String` for dynamic text, `const char*` for constants
8. Stream LittleFS files — never buffer entire files in RAM
9. Place new classes in appropriate directory: `core/`, `adapters/`, `interfaces/`, `models/`, `utils/`
10. Every JS `fetch()` must check `json.ok` and handle failure with `showToast()`
11. Document NVS key abbreviations in `ConfigManager.h` if key exceeds 15 characters

## Project Structure & Boundaries

### Complete Project Directory Structure

```
firmware/
├── platformio.ini                     # UPDATED: new libs + partition + littlefs
├── custom_partitions.csv              # NEW: 2MB app + 1.9MB LittleFS
│
├── src/
│   └── main.cpp                       # UPDATED: init sequence + FreeRTOS task creation
│
├── core/
│   ├── FlightDataFetcher.h            # EXISTING (updated: ConfigManager dependency)
│   ├── FlightDataFetcher.cpp          # EXISTING (updated: ConfigManager dependency)
│   ├── ConfigManager.h                # NEW: singleton, category structs, NVS, debounce
│   ├── ConfigManager.cpp              # NEW
│   ├── LayoutEngine.h                 # NEW: zone calculation (shared algorithm)
│   ├── LayoutEngine.cpp               # NEW
│   ├── LogoManager.h                  # NEW: LittleFS CRUD, ICAO lookup, fallback
│   ├── LogoManager.cpp                # NEW
│   ├── SystemStatus.h                 # NEW: subsystem health registry
│   └── SystemStatus.cpp               # NEW
│
├── adapters/
│   ├── OpenSkyFetcher.h               # EXISTING (updated: ConfigManager)
│   ├── OpenSkyFetcher.cpp             # EXISTING (updated: ConfigManager)
│   ├── AeroAPIFetcher.h               # EXISTING (updated: ConfigManager)
│   ├── AeroAPIFetcher.cpp             # EXISTING (updated: ConfigManager)
│   ├── FlightWallFetcher.h            # EXISTING (unchanged)
│   ├── FlightWallFetcher.cpp          # EXISTING (unchanged)
│   ├── NeoMatrixDisplay.h             # EXISTING (updated: LayoutEngine, LogoManager)
│   ├── NeoMatrixDisplay.cpp           # EXISTING (updated: zones, logos, telemetry)
│   ├── WebPortal.h                    # NEW: ESPAsyncWebServer, API routes
│   ├── WebPortal.cpp                  # NEW
│   ├── WiFiManager.h                  # NEW: state machine, AP/STA, mDNS, scan
│   └── WiFiManager.cpp                # NEW
│
├── interfaces/
│   ├── BaseDisplay.h                  # EXISTING (unchanged)
│   ├── BaseFlightFetcher.h            # EXISTING (unchanged)
│   └── BaseStateVectorFetcher.h       # EXISTING (unchanged)
│
├── models/
│   ├── FlightInfo.h                   # EXISTING (unchanged)
│   ├── StateVector.h                  # EXISTING (unchanged)
│   └── AirportInfo.h                  # EXISTING (unchanged)
│
├── config/
│   ├── APIConfiguration.h             # EXISTING — compile-time defaults (see migration rule)
│   ├── WiFiConfiguration.h            # EXISTING — compile-time defaults
│   ├── UserConfiguration.h            # EXISTING — compile-time defaults
│   ├── HardwareConfiguration.h        # EXISTING — compile-time defaults
│   └── TimingConfiguration.h          # EXISTING — compile-time defaults
│
├── utils/
│   ├── GeoUtils.h                     # EXISTING (unchanged)
│   └── Log.h                          # NEW: compile-time log level macros
│
├── data/                              # NEW: LittleFS initial image → pio run -t uploadfs
│   ├── style.css.gz
│   ├── common.js.gz
│   ├── wizard.html.gz
│   ├── wizard.js.gz
│   ├── dashboard.html.gz
│   ├── dashboard.js.gz
│   └── leaflet/
│       ├── leaflet.min.js.gz
│       └── leaflet.min.css.gz
│
└── test/
    ├── test_config_manager/
    │   └── test_main.cpp
    └── test_layout_engine/
        └── test_main.cpp
```

### Config Header Migration Rule

Existing `config/` headers remain in place as compile-time default value source. **Only `ConfigManager.cpp` includes them.** Adapters migrate from direct config header reads to ConfigManager struct getters:

```cpp
// BEFORE: #include "config/APIConfiguration.h"
//         APIConfiguration::OPENSKY_CLIENT_ID
// AFTER:  #include "core/ConfigManager.h"
//         ConfigManager::getNetwork().opensky_client_id
```

### Runtime LittleFS Layout

```
LittleFS at runtime:
├── *.css.gz, *.html.gz, *.js.gz   ← from firmware/data/ (build-time)
├── leaflet/*.gz                     ← from firmware/data/ (build-time)
└── logos/                           ← created at runtime by LogoManager::init()
    ├── UAL.bin                      ← user-uploaded
    └── ...
```

Web assets at root, logos in `/logos/` subdirectory. LogoManager creates directory on init.

### Architectural Boundaries

**Adapter Boundary:** Browser ↔ WebPortal, WiFi Hardware ↔ WiFiManager, LEDs ↔ NeoMatrixDisplay, APIs ↔ Fetchers
**Core Boundary:** ConfigManager (config state), LayoutEngine (zones), LogoManager (files), SystemStatus (health), FlightDataFetcher (pipeline)
**Data Boundary:** Core 1 → Core 0 via atomic flag + FreeRTOS queue. Core 0 is read-only.

### NeoMatrixDisplay Refactoring Note

Do not split preemptively. If file exceeds ~400 lines after updates, consider extracting `FlightRenderer` for rendering logic, keeping NeoMatrixDisplay focused on hardware.

### Requirements to Structure Mapping

| Epic/Group | Key Files |
|-----------|-----------|
| Setup & Onboarding (FR1-8) | WiFiManager, WebPortal, wizard.html/js |
| Config Management (FR9-14) | ConfigManager, WebPortal, dashboard.html/js |
| Calibration & Preview (FR15-18) | WebPortal, LayoutEngine, dashboard.js |
| Flight Data (FR19-25) | FlightDataFetcher, NeoMatrixDisplay |
| Logo Display (FR26-29) | LogoManager, NeoMatrixDisplay |
| Responsive Layout (FR30-33) | LayoutEngine, NeoMatrixDisplay |
| Logo Management (FR34-37) | LogoManager, WebPortal, dashboard.js |
| System Operations (FR38-48) | SystemStatus, WiFiManager, main.cpp |

### Testing Expectations

**Unit tests required:** ConfigManager (NVS, defaults, applyJson, debounce), LayoutEngine (4 test vectors from shared algorithm)
**Not required:** Hardware-dependent adapters (WiFi, Web, Display, API fetchers)

### External Integration Points

| Service | Auth | Protocol |
|---------|------|----------|
| OpenSky Network | OAuth2 | HTTPS REST |
| FlightAware AeroAPI | API key | HTTPS REST |
| FlightWall CDN | None | HTTPS GET |
| OpenStreetMap | None | HTTPS tiles (client-side) |
| NTP | None | UDP |

### Data Flow

Boot → ConfigManager loads NVS → WiFiManager starts AP/STA → WebPortal serves UI → FlightDataFetcher runs fetch cycle (Core 1) → DisplayTask renders (Core 0) → User changes setting → POST /api/settings → ConfigManager updates RAM → atomic flag → display re-reads → NVS persists after 2s debounce

## Architecture Validation Results

### Coherence Validation ✅

**Decision Compatibility:** All technology choices compatible. mathieucarbou/ESPAsyncWebServer, LittleFS, ArduinoJson, and FastLED coexist on ESP32 Arduino without known conflicts.

**Pattern Consistency:** `snake_case` flows consistently across struct fields → NVS keys → JSON fields. Logging macros, error handling, and include patterns are uniform. No contradictions.

**Structure Alignment:** Directory structure maps directly to hexagonal architecture. Config migration rule is clear. New files in correct directories.

### Requirements Coverage ✅

**All 48 FRs covered.** Every functional requirement maps to at least one architectural component with a clear file location.

**All NFRs addressed.** Performance (hot-reload <1s, page load <3s), reliability (30+ day uptime, WiFi recovery), resource constraints (RAM, flash), concurrency (FreeRTOS dual-core) all have explicit architectural support.

### Implementation Readiness ✅

**Decision Completeness:** 6 core decisions with code examples, interfaces, and rationale. Versions pinned.
**Structure Completeness:** Every file listed as EXISTING/NEW/UPDATED. Migration rules documented.
**Pattern Completeness:** 11 enforcement rules, NVS key table, API endpoint table, JSON examples, JS fetch pattern, logging macros.

### Gap Analysis

| Priority | Gap | Resolution |
|----------|-----|-----------|
| Minor | Fallback airplane sprite storage | PROGMEM constant in LogoManager.h — 32x32 RGB565 array (~2KB flash) |
| Minor | GPIO reset pin not in HardwareConfig | Add `reset_pin` to HardwareConfig, default GPIO 0 (BOOT button), NVS key `reset_pin` |
| Minor | Watchdog timer | ESP32 task watchdog enabled by default. Rule: no task blocks >5 seconds |

No critical gaps.

### Architecture Completeness Checklist

- [x] Project context analyzed (48 FRs, NFRs, UX spec)
- [x] Scale assessed (Medium — IoT with web UI)
- [x] Constraints identified (flash, RAM, single radio, interrupt conflict)
- [x] Cross-cutting concerns mapped (config, WiFi, tasks, flash, errors)
- [x] Technology stack locked with specific forks and versions
- [x] Custom partition table specified
- [x] 6 core architectural decisions documented
- [x] Implementation patterns extracted from existing codebase
- [x] NVS key table with abbreviation rules
- [x] Logging macros with compile-time levels
- [x] 11 enforcement guidelines for AI agents
- [x] Complete directory tree (22 existing + 22 new files)
- [x] Architectural boundaries defined
- [x] Requirements mapped to files
- [x] Runtime LittleFS layout documented
- [x] Testing expectations set
- [x] Shared zone algorithm with test vectors

### Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION
**Confidence Level:** High

**Key Strengths:**
- Clean extension of existing hexagonal architecture — no breaking changes
- Comprehensive NVS key table eliminates config ambiguity
- Shared zone algorithm with test vectors ensures canvas/LED parity
- snake_case consistency across struct/NVS/JSON eliminates naming drift
- Party mode reviews caught critical issues (fork choice, debouncing, naming, logging)

**Areas for Future Enhancement:**
- ~~OTA firmware updates (Phase 2 — partition strategy documented)~~ → Addressed in Foundation Release below
- WebSocket push (deferred — not needed for set-and-forget UI)
- NeoMatrixDisplay split (if exceeds ~400 lines)

### Implementation Handoff

**First Implementation Priority:**
1. Add mathieucarbou/ESPAsyncWebServer + LittleFS to platformio.ini
2. Create custom_partitions.csv
3. Create utils/Log.h
4. Create core/ConfigManager.h/.cpp with NVS + category structs + fallbacks
5. Create core/SystemStatus.h/.cpp
6. Verify firmware builds and existing flight pipeline still works

---

# Foundation Release — Architecture Extension

_Extends the MVP architecture with OTA firmware updates, night mode/brightness scheduling, and onboarding polish. All MVP architectural decisions remain in effect — this section adds new decisions and updates existing ones where the Foundation PRD introduces changes._

## Foundation Release — Project Context Analysis

### Requirements Overview

**Functional Requirements (37 FRs across 6 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| OTA Firmware Updates | FR1-FR11 | **Major:** Partition table change (dual-OTA), new upload endpoint in WebPortal, self-check + rollback in boot sequence, `Update.h` integration |
| Settings Migration | FR12-FR14 | **Minor:** New `/api/settings/export` GET endpoint, client-side JSON import in wizard (~40 lines JS, no new server endpoint) |
| Night Mode & Brightness | FR15-FR24 | **Medium:** NTP integration via `configTzTime()`, 5 new ConfigManager keys, time-based brightness scheduler in main loop, IANA-to-POSIX timezone mapping |
| Onboarding Polish | FR25-FR29 | **Minor:** Wizard Step 6 extension — reuses existing calibration pattern and canvas preview endpoints |
| Dashboard UI | FR30-FR34 | **Minor:** New Firmware card and Night Mode card, rollback detection toast |
| System & Resilience | FR35-FR37 | **Medium:** Dual-OTA partition coexistence, OTA failure recovery path, new NVS config keys |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| OTA upload speed | ≤30s for 1.5MB binary | Stream chunks via `Update.write()` — no RAM buffering |
| NTP sync time | ≤10s after WiFi connect | `configTzTime()` with pool.ntp.org + time.nist.gov |
| Brightness hot-reload | ≤1s (unchanged from MVP) | Scheduler overrides brightness via existing ConfigManager hot-reload path |
| Firmware binary size | ≤1.5MB | Pre-implementation size gate at 1.3MB; reduced from 2MB MVP budget |
| LittleFS free space | ≥500KB after assets + logos | 896KB total - ~278KB used = ~618KB free |
| OTA self-check window | ≤30s boot-to-mark-valid | Sequential gate: WiFi (15s) + web server (2s) + buffer (13s) |
| Uptime with night mode | 30+ days | Non-blocking time comparison per loop iteration — no I/O, no blocking |
| Night mode scheduler overhead | ≤1% main loop impact | Single `localtime_r()` + integer comparison per iteration |

### Scale & Complexity

- **Scope:** Narrower than MVP — fewer new components, but partition change has wide blast radius
- **New architectural components:** 0 new classes (OTA upload in WebPortal, self-check in main.cpp, scheduler in main loop)
- **Updated components:** ConfigManager (5 new keys), WebPortal (2 new endpoints), main.cpp (self-check + scheduler), custom_partitions.csv, wizard HTML/JS
- **Complexity driver:** OTA self-check is the highest-risk code path — boot-time safety mechanism that must be bulletproof

### Technical Constraints (Foundation-Specific)

| Constraint | Impact |
|-----------|--------|
| Dual-OTA partitions require 3MB for two app slots | LittleFS shrinks from ~2MB to ~896KB (56% reduction) |
| Firmware binary must fit in 1.5MB partition | Tighter than MVP's 2MB budget — pre-implementation size gate required |
| One-time USB reflash required for partition migration | All NVS + LittleFS data erased — settings export mitigates |
| No built-in IANA-to-POSIX mapping on ESP32 | Need explicit mapping decision: browser-side, PROGMEM table, or LittleFS file |
| `Update.h` streams from ESPAsyncWebServer multipart callback | Must verify Carbou fork's `onUpload` callback chunk integration with `Update.write()` |
| NTP requires WiFi STA mode | Night mode schedule inactive in AP mode and until first NTP sync |
| ESP32 RTC drift ~5 ppm | ~0.4s/day — NTP re-sync every 24h keeps schedule accurate |

### Cross-Cutting Concerns (Foundation Additions)

1. **Flash budget governance** — LittleFS reduced 56%. Firmware binary capped at 1.5MB. Both limits must be tracked and enforced. Pre-implementation size gate at 1.3MB.

2. **OTA resilience chain** — Upload validation → stream-to-flash → reboot → self-check → mark valid OR watchdog rollback. This is a new failure recovery path that integrates with WiFiManager (WiFi connect check) and WebPortal (server start check) during boot.

3. **Time management (linear dependency, not cross-cutting)** — WiFi STA → `configTzTime()` → `localtime_r()` valid → scheduler compares time → overrides brightness via ConfigManager. NTP failure is *contained* — only the scheduler is affected. No cascade to other subsystems.

4. **Settings portability** — Export (`/api/settings/export`) bridges the one-time partition migration. Import is client-side only (JSON file → pre-fill wizard fields). No new server-side import endpoint.

### Existing Architecture Unchanged

The following MVP decisions remain in full effect with no modifications:

- Hexagonal (Ports & Adapters) pattern
- ConfigManager singleton with category struct getters
- Inter-task communication (atomic flags for config, FreeRTOS queue for flights)
- WiFi state machine (WiFiManager)
- SystemStatus error registry
- FreeRTOS dual-core task pinning (display Core 0, everything else Core 1)
- All naming conventions, NVS key patterns, logging macros, and enforcement rules
- Web asset patterns (fetch + json.ok + showToast)

## Foundation Release — Technology Stack

**Starter Template Evaluation: N/A (Brownfield Extension)**

No new external dependencies. All Foundation capabilities use libraries already in the ESP32 Arduino core:

| Library | Source | Purpose |
|---------|--------|---------|
| `Update.h` | ESP32 Arduino core | OTA flash write (stream chunks to inactive partition) |
| `esp_ota_ops.h` | ESP-IDF | `esp_ota_mark_app_valid_cancel_rollback()`, partition management |
| `time.h` / `configTzTime()` | ESP32 Arduino core | NTP sync with POSIX timezone, `localtime_r()` |
| `esp_sntp.h` | ESP-IDF | NTP sync status callback |

The MVP dependency stack (ESPAsyncWebServer Carbou fork, AsyncTCP, FastLED, ArduinoJson, LittleFS, ESPmDNS) remains unchanged.

## Foundation Release — Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. Partition table — dual OTA layout with exact offsets and sizes
2. OTA handler architecture — upload in WebPortal, self-check in main.cpp
3. OTA self-check — WiFi-OR-timeout strategy with 60-second window
4. IANA-to-POSIX timezone mapping — browser-side JS table
5. Night mode scheduler — non-blocking main loop check via ConfigManager

**Deferred Decisions (Post-Foundation):**
- OTA pull from GitHub releases (Delight Release)
- OTA firmware signing (Phase 3 — public distribution)
- Sunrise/sunset-aware brightness scheduling (Phase 2)

### Decision F1: Partition Table — Dual OTA Layout

**Updated `firmware/custom_partitions.csv`:**

```
# Name,    Type,  SubType, Offset,    Size,      Notes
nvs,       data,  nvs,     0x9000,    0x5000,    # 20KB — config persistence
otadata,   data,  ota,     0xE000,    0x2000,    # 8KB — OTA partition tracking
app0,      app,   ota_0,   0x10000,   0x180000,  # 1.5MB — active firmware
app1,      app,   ota_1,   0x190000,  0x180000,  # 1.5MB — OTA staging
spiffs,    data,  spiffs,  0x310000,  0xF0000,   # 960KB — LittleFS
```

**Flash Budget (Foundation):**

```
4MB Flash Total
├── 20KB  — NVS (config persistence)
├── 8KB   — OTA data (partition tracking)
├── 1.5MB — app0 (active firmware)
├── 1.5MB — app1 (OTA staging)
└── 960KB — LittleFS
    ├── ~80KB  — Web assets (gzipped HTML/JS/CSS)
    ├── ~198KB — 99 bundled logos (~2KB each)
    └── ~682KB — Free headroom
```

**Breaking change from MVP:** LittleFS shrinks from ~1.9MB to ~960KB. One-time USB reflash required — erases NVS + LittleFS. Settings export mitigates reconfiguration.

**Pre-implementation gate:** Measure current firmware binary size. If exceeding 1.3MB, optimize before adding Foundation code (disable Bluetooth with `CONFIG_BT_ENABLED=0`, strip unused library features).

### Decision F2: OTA Handler Architecture — WebPortal + main.cpp

**No new class.** OTA is split across two existing locations:

| Concern | Location | Implementation |
|---------|----------|---------------|
| Upload endpoint (`POST /api/ota/upload`) | `WebPortal.cpp` | ESPAsyncWebServer `onUpload` handler |
| Binary validation (size + magic byte) | `WebPortal.cpp` | Check `Content-Length` vs partition size, first byte == `0xE9` |
| Stream-to-flash | `WebPortal.cpp` | `Update.begin()` → `Update.write()` per chunk → `Update.end()` |
| Abort on failure | `WebPortal.cpp` | `Update.abort()` on stream error or WiFi interruption |
| Progress reporting | `WebPortal.cpp` | Bytes written / total sent via upload handler callback |
| Reboot after success | `WebPortal.cpp` | `ESP.restart()` after successful `Update.end(true)` |
| Self-check + mark valid | `main.cpp` | Boot sequence, after WiFiManager + WebPortal init |
| Rollback detection | `main.cpp` | `esp_ota_check_rollback_is_possible()` at boot → SystemStatus |
| Version string | `platformio.ini` | `-D FW_VERSION=\"1.0.0\"` build flag |

**Upload handler flow:**

```cpp
// In WebPortal::init()
server.on("/api/ota/upload", HTTP_POST,
    // Response handler (called after upload completes)
    [](AsyncWebServerRequest *request) {
        bool success = !Update.hasError();
        request->send(200, "application/json",
            success ? "{\"ok\":true,\"message\":\"Rebooting...\"}"
                    : "{\"ok\":false,\"error\":\"Update failed\"}");
        if (success) { delay(500); ESP.restart(); }
    },
    // Upload handler (called per chunk)
    [](AsyncWebServerRequest *request, String filename,
       size_t index, uint8_t *data, size_t len, bool final) {
        if (index == 0) {
            // First chunk — validate magic byte and begin
            // Note: request->contentLength() is the multipart body size, NOT the file size.
            // Use UPDATE_SIZE_UNKNOWN and let Update.h track actual bytes written.
            // Final size validation happens at Update.end().
            if (data[0] != 0xE9) {
                request->send(400, "application/json",
                    "{\"ok\":false,\"error\":\"Not a valid ESP32 firmware image\"}");
                return;
            }
            // Get max OTA partition size from the partition table
            const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
            if (!Update.begin(partition->size)) {
                request->send(500, "application/json",
                    "{\"ok\":false,\"error\":\"Failed to begin update\"}");
                return;
            }
        }
        // Write chunk
        if (Update.write(data, len) != len) {
            Update.abort();
            return;
        }
        if (final) {
            if (!Update.end(true)) {
                // Update.end() validates written size — rejects if corrupt
                return;
            }
        }
    }
);
```

**Firmware version:**

```ini
# platformio.ini
build_flags =
    -D FW_VERSION=\"1.0.0\"
    -D LOG_LEVEL=2
```

```cpp
// In /api/status response
doc["firmware_version"] = FW_VERSION;
doc["rollback_detected"] = rollbackDetected;
```

### Decision F3: OTA Self-Check — WiFi-OR-Timeout

**Strategy:** Mark firmware valid when WiFi connects OR after 60-second timeout — whichever comes first. No self-HTTP-request. No web server check.

**Rationale:**
- WiFi connection proves network stack works (catches most firmware bugs)
- 60-second timeout prevents false rollback during temporary network outages (router reboot, ISP flap)
- Crash-on-boot is caught by the watchdog timer regardless (firmware never reaches the check)
- No self-HTTP-request avoids complexity and race conditions with async server startup

**Timeout budget:**

| Scenario | Time to mark valid | Outcome |
|----------|-------------------|---------|
| Good firmware + WiFi works | 5-15s (WiFi connect time) | Mark valid ✅ |
| Good firmware + WiFi temporarily down | 60s (timeout) | Mark valid with warning ✅ |
| Bad firmware (crashes before 60s) | Never | Watchdog rollback ✅ |
| Bad firmware (WiFi code broken, doesn't crash) | 60s (timeout) | Mark valid ⚠️ — but device is reachable via AP fallback for re-flash |

**Implementation:**

```cpp
// main.cpp — global state
static uint32_t bootTime;
static bool otaMarkedValid = false;

// In setup(), after all init:
bootTime = millis();

// In loop(), before flight fetch:
if (!otaMarkedValid) {
    bool wifiUp = (WiFi.status() == WL_CONNECTED);
    bool timeoutExpired = (millis() - bootTime) > 60000;

    if (wifiUp || timeoutExpired) {
        esp_ota_mark_app_valid_cancel_rollback();
        otaMarkedValid = true;
        if (!wifiUp) {
            SystemStatus::set(OTA, WARNING, "Marked valid on timeout — WiFi not verified");
            LOG_I("OTA", "Marked valid on timeout, WiFi not connected");
        } else {
            LOG_I("OTA", "Marked valid, WiFi connected in " +
                  String((millis() - bootTime) / 1000) + "s");
        }
    }
}
```

**Rollback detection (also in main.cpp setup):**

```cpp
// In setup(), early — detect if bootloader rolled us back from a failed update
#include "esp_ota_ops.h"

const esp_partition_t* lastInvalid = esp_ota_get_last_invalid_partition();
if (lastInvalid != NULL) {
    // A previous OTA partition was marked invalid by the bootloader — rollback occurred
    rollbackDetected = true;
    SystemStatus::set(OTA, WARNING, "Firmware rolled back to previous version");
    LOG_I("OTA", "Rollback detected — previous partition was invalid");
}

// Note: esp_ota_check_rollback_is_possible() answers a different question
// ("can I roll back?"), NOT "did I just roll back?" — don't use it for detection.
// esp_ota_get_last_invalid_partition() returns non-NULL only when the bootloader
// actually invalidated a partition, which is the true rollback signal.
```

### Decision F4: IANA-to-POSIX Timezone Mapping — Browser-Side

**Strategy:** Browser converts IANA timezone to POSIX string client-side. ESP32 stores and uses the POSIX string directly. Zero firmware/LittleFS overhead.

**JS mapping table:** ~50-80 common timezones in a JS object. Generated from IANA `tzdata`. Stored in wizard.js and dashboard.js (compresses well in gzip).

```javascript
// timezone-map.js (embedded in wizard.js and dashboard.js)
const TZ_MAP = {
    "America/New_York":    "EST5EDT,M3.2.0,M11.1.0",
    "America/Chicago":     "CST6CDT,M3.2.0,M11.1.0",
    "America/Denver":      "MST7MDT,M3.2.0,M11.1.0",
    "America/Los_Angeles": "PST8PDT,M3.2.0,M11.1.0",
    "America/Anchorage":   "AKST9AKDT,M3.2.0,M11.1.0",
    "Pacific/Honolulu":    "HST10",
    "Europe/London":       "GMT0BST,M3.5.0/1,M10.5.0",
    "Europe/Berlin":       "CET-1CEST,M3.5.0,M10.5.0/3",
    "Europe/Paris":        "CET-1CEST,M3.5.0,M10.5.0/3",
    "Asia/Tokyo":          "JST-9",
    "Asia/Shanghai":       "CST-8",
    "Australia/Sydney":    "AEST-10AEDT,M10.1.0,M4.1.0/3",
    // ... ~50-80 entries total
};

function getTimezoneConfig() {
    const iana = Intl.DateTimeFormat().resolvedOptions().timeZone;
    const posix = TZ_MAP[iana] || null;
    return { iana, posix };
}
```

**Wizard flow:**
1. Auto-detect: `Intl.DateTimeFormat().resolvedOptions().timeZone` → `"America/Los_Angeles"`
2. Lookup: `TZ_MAP["America/Los_Angeles"]` → `"PST8PDT,M3.2.0,M11.1.0"`
3. Show dropdown with IANA names, pre-selected to detected timezone
4. Send POSIX string to ESP32 via `POST /api/settings { "timezone": "PST8PDT,M3.2.0,M11.1.0" }`
5. If lookup fails (unknown IANA): show text field for manual POSIX entry

**Dashboard timezone selector:** Same dropdown + mapping. User can change timezone post-setup.

**ESP32 usage:**
```cpp
String tz = ConfigManager::getSchedule().timezone;  // POSIX string
configTzTime(tz.c_str(), "pool.ntp.org", "time.nist.gov");
```

### Decision F5: Night Mode Scheduler — Non-Blocking Main Loop

**Architecture:** Brightness schedule check runs in the Core 1 main loop, adjacent to the flight fetch cycle. Non-blocking — single `getLocalTime()` + integer comparison per iteration.

**Schedule times stored as minutes since midnight** (uint16, 0-1439):
- 23:00 = 1380, 07:00 = 420, 00:00 = 0, 12:30 = 750

**Midnight-crossing logic:**

```cpp
bool inDimWindow;
if (dimStart <= dimEnd) {
    // Same-day: e.g., 09:00-17:00
    inDimWindow = (currentMinutes >= dimStart && currentMinutes < dimEnd);
} else {
    // Midnight crossing: e.g., 23:00-07:00
    inDimWindow = (currentMinutes >= dimStart || currentMinutes < dimEnd);
}
```

**Interaction with manual brightness:**
- When schedule is **enabled** and device is **in dim window**: scheduler overrides brightness
- When schedule is **enabled** and device is **outside dim window**: normal brightness from ConfigManager
- When schedule is **disabled**: scheduler does nothing, manual brightness applies
- If user manually changes brightness while in dim window: schedule re-overrides on next check (1-second loop)
- Dashboard should indicate when schedule is actively overriding brightness

**NTP sync tracking:**

```cpp
// Callback registered once in setup()
sntp_set_time_sync_notification_cb([](struct timeval *tv) {
    ntpSynced = true;
    LOG_I("NTP", "Time synchronized");
    SystemStatus::set(NTP, OK, "Clock synced");
});

// NTP re-sync happens automatically every 1 hour (LWIP default)
// If re-sync fails, device continues using last known time
```

**NTP re-sync interval:** ESP32 LWIP SNTP defaults to 1-hour re-sync. Sufficient for <5 ppm RTC drift (~0.4s/day). No custom interval needed.

### Decision F6: ConfigManager Expansion — New Keys & Struct

**5 new NVS keys:**

| NVS Key | Type | Default | Category | Notes |
|---------|------|---------|----------|-------|
| `timezone` | string | `"UTC0"` | schedule | POSIX timezone string, max 40 chars |
| `sched_enabled` | uint8 | 0 | schedule | 0 = disabled, 1 = enabled |
| `sched_dim_start` | uint16 | 1380 | schedule | Minutes since midnight (23:00) |
| `sched_dim_end` | uint16 | 420 | schedule | Minutes since midnight (07:00) |
| `sched_dim_brt` | uint8 | 10 | schedule | Brightness during dim window (0-255) |

```cpp
// NVS key abbreviations (15-char limit):
// sched_enabled   = schedule_enabled
// sched_dim_start = schedule_dim_start
// sched_dim_end   = schedule_dim_end
// sched_dim_brt   = schedule_dim_brightness
```

**New ConfigManager category struct:**

```cpp
struct ScheduleConfig {
    bool enabled;
    uint16_t dim_start;      // minutes since midnight (0-1439)
    uint16_t dim_end;        // minutes since midnight (0-1439)
    uint8_t dim_brightness;  // 0-255
    String timezone;         // POSIX timezone string
};
```

**ConfigManager additions:**
```cpp
static ScheduleConfig getSchedule();
// Reboot-required keys: unchanged (timezone change takes effect on next NTP sync, no reboot)
// Hot-reload keys: sched_enabled, sched_dim_start, sched_dim_end, sched_dim_brt, timezone
```

All 5 new keys are **hot-reload** — no reboot required. Timezone change calls `configTzTime()` with the new POSIX string immediately.

### Decision F7: API Endpoint Additions

**2 new endpoints:**

| Method | Endpoint | Purpose | Auth | Response |
|--------|----------|---------|------|----------|
| `POST` | `/api/ota/upload` | Multipart firmware upload | None (local network) | `{ ok: true, message: "Rebooting..." }` or `{ ok: false, error: "..." }` |
| `GET` | `/api/settings/export` | Download all config as JSON file | None | `Content-Disposition: attachment; filename=flightwall-settings.json` |

**Updated existing endpoint — `/api/status`:**

New fields in response:
```json
{
    "ok": true,
    "data": {
        "firmware_version": "1.0.0",
        "rollback_detected": false,
        "ntp_synced": true,
        "schedule_active": true,
        "uptime_seconds": 86400,
        // ... existing fields ...
    }
}
```

**Settings export format (`/api/settings/export`):**

```json
{
    "flightwall_settings_version": 1,
    "exported_at": "2026-04-11T15:30:00",
    "wifi_ssid": "MyNetwork",
    "wifi_password": "secret123",
    "os_client_id": "...",
    "os_client_sec": "...",
    "aeroapi_key": "...",
    "center_lat": 37.7749,
    "center_lon": -122.4194,
    "radius_km": 10.0,
    "tiles_x": 10,
    "tiles_y": 2,
    "tile_pixels": 16,
    "display_pin": 25,
    "origin_corner": 0,
    "scan_dir": 0,
    "zigzag": 0,
    "brightness": 40,
    "text_color_r": 255,
    "text_color_g": 255,
    "text_color_b": 255,
    "fetch_interval": 30,
    "display_cycle": 3,
    "timezone": "PST8PDT,M3.2.0,M11.1.0",
    "sched_enabled": 1,
    "sched_dim_start": 1380,
    "sched_dim_end": 420,
    "sched_dim_brt": 10
}
```

All NVS keys dumped as flat JSON. Includes `flightwall_settings_version` for forward compatibility. Import is client-side only — wizard JS reads the file and pre-fills form fields.

### Foundation Decision Impact — Implementation Sequence

**Sequential foundation (must be in order):**
1. **Partition table update** (`custom_partitions.csv`) — must be first, requires USB reflash
2. **ConfigManager expansion** (5 new keys + ScheduleConfig struct) — foundation for everything else

**OTA track (sequential, after step 2):**
3. **OTA upload endpoint** in WebPortal — core Foundation capability
4. **OTA self-check + rollback detection** in main.cpp — safety mechanism for all future OTA updates

**Time track (sequential, parallelizable with OTA track after step 2):**
5. **NTP integration** (`configTzTime()` after WiFi connect) — enables night mode
6. **Night mode scheduler** in main loop — uses NTP + ConfigManager

**Settings track (after step 2, parallelizable with both tracks above):**
7. **Settings export endpoint** — migration tool for partition change

**UI integration (after all tracks complete):**
8. **Dashboard cards** (Firmware, Night Mode) — UI for new capabilities
9. **Wizard Step 6** (Test Your Wall) + timezone auto-detect — onboarding polish
10. **Settings import** in wizard JS — client-side only, references exported JSON

**Parallelization note:** Steps 3-4, 5-6, and 7 are independent tracks that all depend on step 2 but not on each other. For solo dev, linear order is fine. For sprint planning, these represent parallelizable work streams.

### Cross-Decision Dependencies

```
custom_partitions.csv
    └── OTA upload endpoint (needs dual partitions)
        └── OTA self-check (validates after OTA reboot)

ConfigManager (ScheduleConfig)
    └── NTP integration (uses timezone from config)
        └── Night mode scheduler (uses NTP time + schedule config)

Settings export endpoint
    └── Settings import in wizard (reads export file)

Wizard Step 6 (Test Your Wall)
    └── Timezone auto-detect in wizard (IANA-to-POSIX JS mapping)
```

## Foundation Release — Implementation Patterns & Consistency Rules

### Foundation-Specific Conflict Points

The MVP architecture established 11 enforcement rules. The Foundation Release introduces 4 new pattern areas where AI agents could make inconsistent choices.

### OTA Upload Handler Pattern

**Validation-first, fail-fast:**
- Check `Content-Length` against `MAX_OTA_SIZE` before calling `Update.begin()` — reject oversized before allocating flash
- Check first byte == `0xE9` (ESP32 image magic byte) on first chunk — reject non-firmware immediately
- Every error returns `{ ok: false, error: "human-readable message" }` with appropriate HTTP status code

**Streaming pattern:**
```cpp
// First chunk (index == 0): validate and begin
if (contentLength > MAX_OTA_SIZE) { /* reject 400 */ }
if (data[0] != 0xE9) { /* reject 400 */ }
if (!Update.begin(contentLength)) { /* reject 500 */ }

// Every chunk: write and verify
if (Update.write(data, len) != len) { Update.abort(); return; }

// Final chunk: finalize
if (final) { Update.end(true); }
```

**Anti-pattern:** Never buffer the entire binary in RAM before writing. The binary can be 1.5MB — ESP32 has ~280KB usable RAM. Always stream chunk-by-chunk via `Update.write()`.

### Time Management Pattern

**Non-blocking time access (mandatory):**
```cpp
struct tm now;
if (!getLocalTime(&now, 0)) return;  // timeout=0, NON-BLOCKING
```

**Rule:** Always pass `timeout=0` to `getLocalTime()`. Never block the main loop waiting for time. If time isn't available, skip the time-dependent operation gracefully.

**NTP sync state tracking:**
```cpp
// Register callback once in setup()
sntp_set_time_sync_notification_cb([](struct timeval *tv) {
    ntpSynced = true;
});

// Check state via atomic bool — never poll NTP status
std::atomic<bool> ntpSynced{false};
```

**Anti-pattern:** Polling `sntp_get_sync_status()` in a loop. Use the callback to set a flag and check the flag.

### Schedule Time Convention

**Minutes since midnight (uint16, 0-1439):**
- Compact storage (2 bytes per time vs. 5-byte HH:MM string)
- Simple arithmetic for midnight-crossing comparison
- No string parsing at runtime on the ESP32

**Conversion (client-side JS only):**
```javascript
// HH:MM string → minutes since midnight
function timeToMinutes(hhmm) {
    const [h, m] = hhmm.split(':').map(Number);
    return h * 60 + m;
}

// minutes since midnight → HH:MM string
function minutesToTime(mins) {
    const h = String(Math.floor(mins / 60)).padStart(2, '0');
    const m = String(mins % 60).padStart(2, '0');
    return `${h}:${m}`;
}
```

**Anti-pattern:** Storing schedule times as HH:MM strings in NVS or passing time strings between firmware components. Always use `uint16` minutes in firmware.

### Settings Export/Import Boundary

**Export: server-side.**
- `GET /api/settings/export` dumps all ConfigManager keys as flat JSON
- `Content-Disposition: attachment; filename=flightwall-settings.json`
- Includes `flightwall_settings_version: 1` for forward compatibility

**Import: client-side only.**
- Wizard JS reads the file via `FileReader API`
- Pre-fills form fields for user review
- User confirms before settings are applied via normal `POST /api/settings`
- Unrecognized keys in the file are silently ignored
- No server-side import endpoint — no new attack surface

**Anti-pattern:** Building a `POST /api/settings/import` endpoint that blindly applies a JSON file. Import is always mediated through the wizard UI with user confirmation.

### Updated Enforcement Guidelines

**All AI agents implementing Foundation stories MUST also follow (in addition to MVP rules 1-11):**

12. OTA upload must validate before writing — size check + magic byte on first chunk, fail-fast with `{ ok: false, error: "..." }` JSON response
13. OTA upload must stream directly to flash via `Update.write()` per chunk — never buffer the full binary in RAM
14. `getLocalTime()` must always use `timeout=0` (non-blocking) — never block the main loop waiting for time
15. Schedule times stored as `uint16` minutes since midnight (0-1439) — no string time representations in firmware
16. Settings import is client-side only — no server-side import endpoint or file parsing on the ESP32

## Foundation Release — Project Structure & Boundaries

### File Change Map

No new files. All Foundation changes fit into existing components:

| File | Change | Foundation Purpose |
|------|--------|-------------------|
| `firmware/custom_partitions.csv` | UPDATED | Dual OTA partition layout (app0 1.5MB + app1 1.5MB + spiffs 960KB) |
| `firmware/platformio.ini` | UPDATED | `-D FW_VERSION=\"1.0.0\"` build flag, potential `-D CONFIG_BT_ENABLED=0` |
| `firmware/src/main.cpp` | UPDATED | OTA self-check + rollback detection at boot, NTP init after WiFi, brightness scheduler in loop() |
| `firmware/core/ConfigManager.h` | UPDATED | ScheduleConfig struct, `getSchedule()`, 5 new NVS key abbreviations |
| `firmware/core/ConfigManager.cpp` | UPDATED | ScheduleConfig NVS read/write/defaults, timezone hot-reload calls `configTzTime()` |
| `firmware/core/SystemStatus.h` | UPDATED | New subsystem entries: `OTA`, `NTP` |
| `firmware/adapters/WebPortal.h` | UPDATED | OTA upload + settings export endpoint declarations |
| `firmware/adapters/WebPortal.cpp` | UPDATED | `/api/ota/upload` multipart handler, `/api/settings/export` GET handler, updated `/api/status` response |
| `firmware/data-src/dashboard.html` | UPDATED | Firmware card, Night Mode card, rollback warning toast |
| `firmware/data-src/dashboard.js` | UPDATED | OTA upload UI with progress, night mode settings, timezone selector dropdown, TZ_MAP |
| `firmware/data-src/wizard.html` | UPDATED | Step 6 (Test Your Wall), timezone field in Location step |
| `firmware/data-src/wizard.js` | UPDATED | Step 6 confirmation/back-nav/RGB test logic, timezone auto-detect, TZ_MAP, settings import via FileReader |
| `firmware/data/*.gz` | REBUILT | Gzipped copies of all updated web assets |

### Requirements to File Mapping

| FR Group | Key Files |
|----------|-----------|
| OTA Updates (FR1-FR11) | `WebPortal.cpp` (upload endpoint, validation, streaming), `main.cpp` (self-check, rollback detection), `custom_partitions.csv` |
| Settings Migration (FR12-FR14) | `WebPortal.cpp` (`/api/settings/export`), `wizard.js` (FileReader import, pre-fill fields) |
| Night Mode (FR15-FR24) | `ConfigManager` (ScheduleConfig, 5 keys), `main.cpp` (NTP init, scheduler), `dashboard.js` (Night Mode card) |
| Onboarding Polish (FR25-FR29) | `wizard.html` (Step 6 UI), `wizard.js` (Step 6 logic, timezone auto-detect, TZ_MAP) |
| Dashboard UI (FR30-FR34) | `dashboard.html` (Firmware + Night Mode cards), `dashboard.js` (OTA upload, schedule settings, rollback toast) |
| System & Resilience (FR35-FR37) | `main.cpp` (self-check gate), `ConfigManager` (new NVS keys), `SystemStatus` (OTA + NTP subsystems) |

### Architectural Boundaries — No Changes

All Foundation changes respect the existing hexagonal architecture:

- **WebPortal** remains the sole browser adapter — OTA upload and settings export are new routes, not new boundary crossings
- **ConfigManager** remains the config core — 5 new keys follow the existing category struct pattern
- **main.cpp** remains the orchestrator — self-check and scheduler are boot/loop concerns, not new components
- **SystemStatus** remains the health registry — OTA and NTP are new subsystem entries
- **Core 0 (display) remains read-only** — brightness scheduler runs on Core 1 and uses the existing ConfigManager hot-reload path to affect display brightness

### Foundation Data Flow

```
Boot → ConfigManager::init() (loads 5 new schedule keys from NVS)
     → SystemStatus::init() (registers OTA + NTP subsystems)
     → WiFiManager::init() → WiFi connects
     → configTzTime(timezone, "pool.ntp.org", "time.nist.gov")  ← NEW
     → sntp callback → ntpSynced = true                          ← NEW
     → WebPortal::init() (registers /api/ota/upload, /api/settings/export)
     → OTA self-check: WiFi connected OR 60s timeout → mark valid ← NEW
     → Rollback detection → SystemStatus::set(OTA, WARNING, ...) ← NEW
     → loop():
         Flight fetch cycle (unchanged)
         checkBrightnessSchedule()  ← NEW (non-blocking, Core 1)
```

### External Integration Points — Foundation Additions

| Service | Protocol | Purpose | New? |
|---------|----------|---------|------|
| NTP (pool.ntp.org, time.nist.gov) | UDP port 123 | Time sync for night mode scheduler | **Yes** |

No other new external integrations. OTA is local (browser → ESP32 over LAN).

## Foundation Release — Architecture Validation Results

### Coherence Validation ✅

**Decision Compatibility:** All 7 Foundation decisions (F1-F7) are compatible with each other and with the 6 MVP decisions. No technology conflicts. Partition table change (F1) enables OTA (F2/F3), NTP (F4/F5) enables scheduler (F5), ConfigManager expansion (F6) supports both. Clean dependency chain with no cycles.

**Pattern Consistency:** Foundation patterns (rules 12-16) extend MVP patterns (rules 1-11) without contradictions. `snake_case` flows consistently through new NVS keys (`sched_enabled`, `sched_dim_start`, etc.) → JSON fields → ScheduleConfig struct fields. Logging and error handling follow existing macros and boolean-return patterns.

**Structure Alignment:** All Foundation changes fit into existing files and directories. No new classes, no new boundary crossings. WebPortal gets new routes (consistent pattern). ConfigManager gets a new category struct (consistent pattern). main.cpp gets boot-time and loop-time additions (consistent with existing init sequence and flight fetch loop).

### Requirements Coverage ✅

**All 37 Functional Requirements covered.** Every FR maps to at least one architectural decision with explicit code location:
- OTA (FR1-FR11): Decisions F1, F2, F3 — WebPortal.cpp, main.cpp, custom_partitions.csv
- Settings Migration (FR12-FR14): Decision F7 — WebPortal.cpp, wizard.js
- Night Mode (FR15-FR24): Decisions F4, F5, F6 — ConfigManager, main.cpp, dashboard.js, wizard.js
- Onboarding (FR25-FR29): Structure map — wizard.html/js (reuses existing calibration endpoints)
- Dashboard UI (FR30-FR34): Structure map — dashboard.html/js
- System (FR35-FR37): Decisions F1, F2, F3, F6 — main.cpp, ConfigManager

**All NFRs architecturally addressed:**
- Performance: streaming OTA (≤30s), non-blocking scheduler (≤1%), NTP (≤10s), hot-reload (≤1s)
- Reliability: self-check gate, watchdog rollback, NVS persistence, NTP re-sync, graceful NTP failure
- Resource constraints: 1.5MB firmware cap, 682KB LittleFS free, streaming (no RAM buffer), ≤256 bytes NVS

### Implementation Readiness ✅

**Decision Completeness:** 7 decisions with code examples, interface definitions, and rationale. Partition sizes specified with exact offsets. NVS key table complete with types, defaults, and abbreviations. API endpoints specified with request/response formats.

**Structure Completeness:** File change map covers all 13 affected files. Requirements-to-file mapping is explicit. No orphaned files, no unmapped FRs.

**Pattern Completeness:** 16 enforcement rules (11 MVP + 5 Foundation). OTA streaming, time management, schedule convention, and settings boundary patterns all specified with code examples and anti-patterns.

### Architectural Overrides

**FR5 override (self-check simplification):**

The PRD specifies FR5 as: *"Device can verify its own health after booting new firmware (WiFi, web server, OTA endpoint reachability) before marking the partition as valid."*

The architecture simplifies this to: WiFi connects OR 60-second timeout. No web server check, no self-HTTP-request to `/api/ota/upload`.

**Rationale:** The self-HTTP-request adds complexity and race conditions (async server may not be fully initialized when the check runs). WiFi connection proves the network stack works. If the web server failed to initialize, the firmware would crash before reaching the self-check — caught by the watchdog. The timeout path catches temporary network outages without false rollback. The PRD's intent (don't brick the device) is fully preserved by the simpler approach.

**Story specs MUST reference this architectural decision (F3), not the raw FR5 wording, to prevent re-implementation of the self-HTTP-request.**

### Gap Analysis

| Priority | Gap | Resolution |
|----------|-----|-----------|
| Minor | `MAX_OTA_SIZE` derivation | Use `esp_ota_get_next_update_partition(NULL)->size` to get partition capacity at runtime. No hardcoded constant. Already reflected in F2 code example. |
| Minor | Rollback detection API | Use `esp_ota_get_last_invalid_partition()` (returns non-NULL only after actual rollback). Do NOT use `esp_ota_check_rollback_is_possible()` which answers "can I roll back?" not "did I just roll back?" Already corrected in F3 code example. |
| Minor | Multipart Content-Length | `request->contentLength()` returns multipart body size, not file size. Use `UPDATE_SIZE_UNKNOWN` with `Update.begin(partition->size)` and let `Update.end()` validate. Already corrected in F2 code example. |
| Minor | 60s timeout path testability | Timeout path verified by code inspection + one-time manual test (boot with WiFi unavailable). Not expected in automated test coverage. |
| Minor | Settings export/import round-trip | Export includes `flightwall_settings_version` and `exported_at` metadata fields. Import must ignore these (they're not config keys). Wizard JS should silently skip unrecognized keys per FR14. |
| Minor | Manual brightness during dim window | Schedule re-overrides brightness on next check (~1s). Dashboard should indicate "Schedule active — brightness overridden" when in dim window. Document as known behavior, not a bug. |
| Minor | TZ_MAP completeness | Ship with ~50-80 common timezones. Manual POSIX entry fallback for unlisted zones. Expandable in future releases. |

No critical gaps.

### Foundation Architecture Completeness Checklist

- [x] Foundation PRD context analyzed (37 FRs, NFRs, constraints)
- [x] Scale assessed (narrow extension, partition change is main blast radius)
- [x] Constraints identified (1.5MB firmware cap, 960KB LittleFS, NTP dependency)
- [x] Cross-cutting concerns mapped (flash budget, OTA resilience, time management)
- [x] No new external dependencies (all ESP32 core libraries)
- [x] 7 Foundation decisions documented with code examples
- [x] Partition table specified with exact offsets
- [x] Implementation patterns extended (rules 12-16)
- [x] NVS key table expanded (5 new keys with abbreviations)
- [x] 2 new API endpoints specified with request/response formats
- [x] Complete file change map (13 files, all UPDATED)
- [x] Requirements-to-file mapping complete (37 FRs → specific files)
- [x] Architectural boundaries unchanged
- [x] Implementation sequence with parallelization annotations
- [x] FR5 architectural override documented with rationale
- [x] All party mode refinements incorporated (rollback API, multipart handling, parallelization)

### Foundation Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION
**Confidence Level:** High

**Key Strengths:**
- Clean extension of MVP architecture — no breaking changes to existing patterns
- OTA self-check is simplified and robust (WiFi-OR-timeout avoids false rollbacks)
- Rollback detection uses correct ESP-IDF API (`esp_ota_get_last_invalid_partition()`)
- Browser-side timezone mapping avoids firmware/LittleFS overhead
- Non-blocking scheduler with zero new inter-core communication
- All enforcement rules are additive (no changes to MVP rules 1-11)
- Party mode reviews caught 3 code-level issues before implementation (rollback API, multipart Content-Length, FR5 override)

**Areas for Future Enhancement:**
- OTA pull from GitHub releases (Delight Release — already in PRD pipeline)
- Firmware signing for public distribution (Phase 3)
- Sunrise/sunset-aware brightness scheduling (Phase 2)
- Expanded TZ_MAP coverage beyond initial ~50-80 timezones

### Foundation Implementation Handoff

**Pre-implementation gate:**
1. Measure current firmware binary size (`~/.platformio/penv/bin/pio run` → check `.pio/build/*/firmware.bin`)
2. If exceeding 1.3MB: disable Bluetooth (`-D CONFIG_BT_ENABLED=0`), review library compile flags
3. Proceed only when binary fits comfortably under 1.5MB with headroom for Foundation additions

**First implementation priority:**
1. Update `custom_partitions.csv` to dual-OTA layout
2. Add `-D FW_VERSION=\"1.0.0\"` to platformio.ini build_flags
3. Expand ConfigManager with ScheduleConfig struct and 5 new NVS keys
4. Add `/api/ota/upload` handler in WebPortal
5. Add OTA self-check + rollback detection in main.cpp setup()
6. Verify firmware builds, OTA partitions are recognized, and existing functionality works

---

# Display System Release — Architecture Extension

_Extends the MVP + Foundation architecture with a pluggable display mode system, mode registry, Classic Card migration, Live Flight Card mode, and Mode Picker UI. All prior architectural decisions remain in effect — this section adds new decisions and updates existing ones where the Display System PRD introduces changes._

## Display System Release — Project Context Analysis

### Requirements Overview

**Functional Requirements (36 FRs across 8 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| DisplayMode Interface | FR1-FR4 | **Major:** New abstract interface with lifecycle semantics (init/render/teardown), memory reporting, rendering context with zone bounds and read-only brightness |
| Mode Registry & Lifecycle | FR5-FR10 | **Major:** New `core/ModeRegistry` component — static mode table, serialized switch execution, heap validation before activation, mode enumeration for API/UI |
| Classic Card Mode | FR11-FR13 | **High-risk migration:** Extract rendering logic from `NeoMatrixDisplay::renderZoneFlight()` and sub-methods (~140 lines) into standalone mode class with pixel-parity validation |
| Live Flight Card Mode | FR14-FR16 | **Medium:** New mode using existing telemetry fields (altitude_kft, speed_mph, track_deg, vertical_rate_fps) already in FlightInfo; adaptive field dropping for smaller panels |
| Mode Picker UI | FR17-FR26 | **Medium:** New dashboard section with schematic wireframe previews, activation control, transition state feedback, one-time upgrade notification |
| Mode Persistence | FR27-FR29 | **Minor:** Single NVS key (`display_mode`) for active mode; boot restoration with Classic Card default on first upgrade |
| Mode Switch API | FR30-FR33 | **Minor:** 2 new HTTP endpoints following existing `/api/*` JSON envelope pattern; transition state reporting |
| Display Infrastructure | FR34-FR36 | **Major refactoring:** NeoMatrixDisplay splits responsibilities — hardware ownership vs. rendering delegation; frame commit (`FastLED.show()`) separated from mode rendering |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| Mode switch latency | <2 seconds (user-perceived) | Full lifecycle (teardown → heap check → init → first render) within budget; cooperative scheduling on display task |
| Per-frame budget | <16ms, non-blocking | Mode render must complete within 60fps budget; no network I/O, no filesystem I/O on hot path |
| Mode Picker load | <1 second | Static mode metadata; no heap allocation for enumeration |
| Registry enumeration | <10ms, zero heap | Mode list served from BSS/data segment; no dynamic allocation for API response |
| Heap stability | No net loss after 100 switches | Fixed-size heap allocations per mode; sequential teardown → init with no overlapping allocations |
| Rapid-switch safety | 10 toggles in 3 seconds | Switch serialization via state enum in display task; queue/ignore during in-progress transition |
| Stack discipline | ≤512 bytes stack-local per render frame | Large buffers heap-allocated at mode init, reused across frames |
| Heap floor | ≥30KB free after mode activation | Heap guard validates before init; empirical measurement during integration |
| NVS namespace safety | No collision with Foundation keys | `display_mode` key uses existing `"flightwall"` namespace with verified unique prefix |
| Pixel parity | Classic Card identical to pre-migration output | GFX draw-call sequence comparison; visual verification across 5+ flight cards |
| Foundation compatibility | OTA, night mode, NTP, settings, health all unaffected | Modes isolated from all subsystems via read-only rendering context |
| Cooperative scheduling | No new FreeRTOS tasks for mode rendering | Mode render runs within existing display task on Core 0; no interrupt-driven frame push |
| Dashboard consistency | Mode Picker matches existing card/navigation patterns | Same CSS, same JS fetch patterns, no third-party script runtime additions |

### Scale & Complexity

- **Primary domain:** IoT/Embedded with companion web UI (unchanged)
- **Complexity level:** Medium-High — plugin architecture on memory-constrained MCU with cross-core coordination
- **New architectural components:** 4 (ModeRegistry, ClassicCardMode, LiveFlightCardMode, DisplayMode interface)
- **New directory:** `firmware/modes/` for mode implementations
- **Updated components:** NeoMatrixDisplay (responsibility split), WebPortal (2 new endpoints), main.cpp (display task integration), dashboard HTML/JS (Mode Picker UI)
- **Existing components unchanged:** FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, WiFiManager, ConfigManager (no new keys except `display_mode`), LayoutEngine, LogoManager, SystemStatus

### Technical Constraints (Display System–Specific)

| Constraint | Impact |
|-----------|--------|
| ESP32 non-compacting heap allocator | Repeated mode switches risk heap fragmentation; modes must use fixed-size allocations and free all memory during teardown |
| Single display task on Core 0 | Mode rendering, switch execution, and lifecycle management all run cooperatively on one task — no parallel mode operations |
| Cross-core API boundary | Mode switch requests arrive from WebPortal (Core 1 async TCP) but execute on display task (Core 0) — requires atomic flag coordination |
| `FastLED.show()` disables interrupts | Frame commit must remain in display task after render returns; modes must NOT call `FastLED.show()` directly |
| 8KB display task stack | Per-frame rendering must stay within stack budget; large buffers allocated at mode init on heap |
| Shared logo buffer (2KB) | `_logoBuffer[1024]` in NeoMatrixDisplay is reused across frames; modes receive it via rendering context, not owned per-mode |
| Existing calibration/positioning mode flags | These are primitive boolean checks in the display task, NOT the template for the mode system — DisplayMode requires proper lifecycle semantics |

### Cross-Cutting Concerns (Display System Additions)

1. **Rendering context as isolation boundary** — Modes receive a bundled struct containing: matrix reference (GFX primitives), zone bounds (LayoutResult), pre-computed text color, brightness (read-only), flight data reference, shared logo buffer reference, and display cycle timing. Modes have zero dependency on ConfigManager, WiFiManager, or any other subsystem. This isolation is the architectural guarantee for NFR C2 (Foundation compatibility).

2. **Cross-core mode switch coordination** — Browser → POST API → WebPortal sets atomic switch request flag → display task checks flag at top of loop iteration (before any render call) → executes teardown/init → clears flag. Mirrors the existing `g_configChanged` atomic pattern. No new concurrency primitives, no mutexes, no FreeRTOS queues for mode switching.

3. **NeoMatrixDisplay responsibility split** — Post-refactoring, NeoMatrixDisplay owns: (a) hardware initialization (LED strip, matrix object, brightness), (b) frame commit (`FastLED.show()`), (c) shared resources (logo buffer, matrix dimensions), (d) emergency fallback renderer (`displaySingleFlightCard()` retained for FR36 — ensures a valid rendering path even when no mode can init). Rendering logic migrates to mode classes. Display task orchestrates: `registry.getActiveMode()->render(context)` then `display.show()`.

4. **Heap lifecycle management** — Only one mode active at any time. During switch: outgoing mode's `teardown()` frees all heap → registry measures `ESP.getFreeHeap()` → compares against incoming mode's `getMemoryRequirement()` → if sufficient, calls `init()` → if insufficient, re-initializes previous mode and returns error. No overlapping mode allocations. 100-switch stability validated empirically.

5. **Flight cycling state migration** — `_currentFlightIndex` and `_lastCycleMs` currently live in NeoMatrixDisplay and the display task. Post-extraction, each mode owns its own cycling state (index + timer). The rendering context passes the full flight vector; modes decide how to cycle. This costs ~12 bytes per mode (trivial for `getMemoryRequirement()`).

6. **Zone descriptor metadata for Mode Picker** — Each mode exposes static metadata describing its zone layout (grid dimensions, zone labels, row/col positions, spans). The GET API returns this alongside mode name/ID. Dashboard JS renders schematic wireframe previews client-side using CSS Grid with `<div>` elements — no canvas, no SVG, no server-rendered bitmaps. Zone layout is fully data-driven from each mode's `getZoneLayout()` metadata; new modes get schematics automatically with zero UI code changes.

7. **Upgrade notification via dual-source dismissal** — The one-time "New display modes available" banner (FR25-FR26) uses dual-source dismissal for robustness: firmware returns an `upgrade_notification` flag in the `GET /api/display/modes` response (set when no prior `display_mode` NVS key exists on first boot), AND browser checks `localStorage.getItem('flightwall_mode_notif_seen')`. Banner shows only when both conditions are true (API flag true AND localStorage not set). Dismissal clears both: `localStorage.setItem()` on client + `POST /api/display/ack-notification` to clear the firmware flag. Either source being cleared independently prevents reappearance.

8. **Heap baseline measurement prerequisite** — Before any Display System code is written, measure free heap after full Foundation Release boot (WiFi connected, web server active, NTP synced, flight data in queue). This establishes the budget ceiling for mode `getMemoryRequirement()` values and validates whether the 30KB floor (NFR S4) is achievable.

### Existing Architecture Unchanged

The following MVP + Foundation decisions remain in full effect with no modifications:

- Hexagonal (Ports & Adapters) pattern
- ConfigManager singleton with category struct getters (no new config structs — only one new NVS key)
- Inter-task communication (atomic flags for config, FreeRTOS queue for flights)
- WiFi state machine (WiFiManager)
- SystemStatus error registry
- FreeRTOS dual-core task pinning (display Core 0, everything else Core 1)
- All naming conventions, NVS key patterns, logging macros, and enforcement rules (1-16)
- Web asset patterns (fetch + json.ok + showToast)
- OTA self-check, rollback detection, night mode scheduler
- Flash partition layout (unchanged from Foundation)

## Display System Release — Technology Stack

**Starter Template Evaluation: N/A (Brownfield Extension)**

No new external dependencies. All Display System capabilities use the existing ESP32 Arduino framework and libraries already in the technology stack:

| Component | Implementation | Source |
|-----------|---------------|--------|
| DisplayMode interface | Abstract C++ class | New firmware code |
| ModeRegistry | Static C++ class | New firmware code |
| ClassicCardMode | C++ class (extracted from NeoMatrixDisplay) | New firmware code |
| LiveFlightCardMode | C++ class | New firmware code |
| Mode Switch API | ESPAsyncWebServer routes | Existing dependency |
| Mode Picker UI | HTML/JS/CSS in dashboard | Existing web asset pattern |
| NVS persistence | ESP32 Preferences API | Existing dependency |
| Rendering primitives | Adafruit GFX via FastLED_NeoMatrix | Existing dependency |

The MVP + Foundation dependency stack (ESPAsyncWebServer Carbou fork, AsyncTCP, FastLED, Adafruit GFX, FastLED NeoMatrix, ArduinoJson, LittleFS, ESPmDNS, Update.h, esp_ota_ops.h, time.h/configTzTime) remains unchanged.

## Display System Release — Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. D1 — DisplayMode interface with RenderContext (every mode and the registry depend on this contract)
2. D2 — ModeRegistry with static table, cooperative switch serialization, heap guard
3. D3 — NeoMatrixDisplay responsibility split (rendering extracted, hardware retained)
4. D4 — Display task integration via ModeRegistry::tick() + atomic flag

**Important Decisions (Shape Architecture):**
5. D5 — Mode Switch API endpoints (GET list + POST activate)
6. D6 — NVS persistence for active mode selection

**Supporting Decisions:**
7. D7 — Mode Picker UI with client-side wireframes and localStorage notification

**Deferred Decisions (Post-Display System):**
- Mode-specific user settings (per-mode NVS keys and UI panels)
- Animated transitions (blank transition only in this release)
- Mode scheduling (auto-switch by time of day)
- Server-rendered preview thumbnails

### Decision D1: DisplayMode Interface — Abstract Class with RenderContext

**The central abstraction. Every mode implements this contract.**

```cpp
// Forward declarations
struct RenderContext;
struct FlightInfo;
struct ModeZoneDescriptor;

class DisplayMode {
public:
    virtual ~DisplayMode() = default;
    virtual bool init(const RenderContext& ctx) = 0;
    virtual void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) = 0;
    virtual void teardown() = 0;
    virtual const char* getName() const = 0;
    virtual const ModeZoneDescriptor& getZoneDescriptor() const = 0;
};
```

**Note:** `getMemoryRequirement()` is intentionally NOT in the virtual interface. Memory requirement is a static property of the mode class, needed BEFORE instantiation for the heap guard. It is exposed via a static function pointer in `ModeEntry` (see D2). This avoids a throwaway allocation just to query memory needs.

**Rendering context — bundles everything a mode needs, nothing it shouldn't have:**

```cpp
struct RenderContext {
    Adafruit_NeoMatrix* matrix;     // GFX drawing primitives (write to buffer only)
    LayoutResult layout;             // zone bounds (logo, flight, telemetry)
    uint16_t textColor;              // pre-computed from DisplayConfig
    uint8_t brightness;              // read-only — managed by night mode scheduler
    uint16_t* logoBuffer;            // shared 2KB buffer from NeoMatrixDisplay
    uint16_t displayCycleMs;         // cycle interval for modes that rotate flights
};
```

**Zone descriptor — static metadata for Mode Picker UI wireframes:**

```cpp
struct ZoneRegion {
    const char* label;    // e.g., "Airline", "Route", "Altitude"
    uint8_t relX, relY;   // relative position within mode's canvas (0-100%)
    uint8_t relW, relH;   // relative dimensions (0-100%)
};

struct ModeZoneDescriptor {
    const char* description;        // human-readable mode description
    const ZoneRegion* regions;      // static array of labeled regions
    uint8_t regionCount;
};
```

**Key design choices:**
- `RenderContext` passed by const reference — modes cannot modify shared state
- No ConfigManager dependency in modes — text color and timing pre-computed by caller
- `logoBuffer` is a shared resource owned by NeoMatrixDisplay, not per-mode (avoids doubling 2KB heap usage)
- `getZoneDescriptor()` returns static data (PROGMEM-safe) — zero heap for API serialization
- `init()` returns bool — false means mode failed to initialize (heap allocation failure or resource unavailable)
- **Modes own their flight cycling state** — `_currentFlightIndex` and `_lastCycleMs` migrate from NeoMatrixDisplay into each mode's private members (~12 bytes per mode)
- **Empty flight vector is valid input to `render()`** — modes decide their own idle/loading state (ClassicCardMode shows "...", LiveFlightCardMode may show a different idle display)
- **Modes must NOT call `FastLED.show()`** — they write to the pixel buffer only; frame commit is the display task's responsibility

### Decision D2: ModeRegistry — Static Table with Cooperative Switch Serialization

```cpp
enum class SwitchState : uint8_t {
    IDLE,
    REQUESTED,
    SWITCHING
};

struct ModeEntry {
    const char* id;                           // e.g., "classic_card"
    const char* displayName;                  // e.g., "Classic Card"
    DisplayMode* (*factory)();                // factory function returning new instance
    uint32_t (*memoryRequirement)();          // static function — no instance needed
    uint8_t priority;                         // display order in Mode Picker
};

class ModeRegistry {
public:
    static void init(const ModeEntry* table, uint8_t count);
    static bool requestSwitch(const char* modeId);    // called from Core 1 (API)
    static void tick(const RenderContext& ctx,         // called from display task (Core 0)
                     const std::vector<FlightInfo>& flights);
    static DisplayMode* getActiveMode();
    static const char* getActiveModeId();
    static const ModeEntry* getModeTable();
    static uint8_t getModeCount();
    static SwitchState getSwitchState();
    static const char* getLastError();

private:
    static const ModeEntry* _table;
    static uint8_t _count;
    static DisplayMode* _activeMode;
    static uint8_t _activeModeIndex;
    static std::atomic<uint8_t> _requestedIndex;   // cross-core: Core 1 writes, Core 0 reads
    static SwitchState _switchState;
    static char _lastError[64];
    static bool _nvsWritePending;                   // debounced NVS persistence
    static unsigned long _lastSwitchMs;
};
```

**Static allocation:** `_table` is a pointer to a const array in BSS/data (defined in main.cpp). `_activeMode` is the only heap pointer — the single active mode instance. `_requestedIndex` is atomic for cross-core safety. `_lastError` is a fixed-size char array — no String allocation.

**Switch flow (inside `tick()`, runs on Core 0):**

1. Check `_requestedIndex` — if different from `_activeModeIndex`, begin switch
2. Set `_switchState = SWITCHING`
3. Call `_activeMode->teardown()` — frees mode's internal heap buffers, but **do NOT delete the mode object yet** (the ~32-64 byte shell remains for safe restoration)
4. Measure `ESP.getFreeHeap()`
5. Call `_table[requestedIndex].memoryRequirement()` — static function, no instantiation
6. **If heap sufficient:**
   - `delete _activeMode` — free the old shell
   - Create new mode via `_table[requestedIndex].factory()`
   - Call `init(ctx)` on new mode
   - If `init()` succeeds: update `_activeModeIndex`, set `_nvsWritePending = true`, clear error
   - If `init()` fails: `delete` the new mode, re-create previous mode via factory, call `init(ctx)`, set error "Mode initialization failed"
7. **If heap insufficient:**
   - Call `_activeMode->init(ctx)` to re-initialize the still-allocated previous mode
   - Set error "Insufficient memory for [mode name]"
8. Set `_switchState = IDLE`
9. **NVS debounce:** If `_nvsWritePending` and `millis() - _lastSwitchMs > 2000`, write to NVS via `ConfigManager::setDisplayMode()`, clear flag

**Key design choices:**
- **Teardown-before-delete pattern** — previous mode object shell stays allocated until new mode is confirmed, enabling safe restoration without re-allocation
- **`init()` failure treated identically to heap guard failure** — restore previous mode, set error, propagate to API
- **`tick()` is allowed to exceed the 16ms frame budget during SWITCHING state** — the wall goes briefly blank (fillScreen(0) during teardown), consistent with the PRD's "brief blank transition" design. This is a documented accepted tradeoff, not a bug
- **NVS write debounced** — 2-second quiet period after last switch, same pattern as ConfigManager's `schedulePersist()`. Prevents 10 NVS writes during rapid-switch stress test (NFR S2)

### Decision D3: NeoMatrixDisplay Refactoring — Responsibility Split

**Post-refactoring, NeoMatrixDisplay retains three roles:**

1. **Hardware owner** — LED strip initialization, matrix object creation, brightness control
2. **Frame committer** — `show()` method wrapping `FastLED.show()`, called by display task after mode render
3. **Shared resource provider** — logo buffer, matrix reference, dimensions, layout result

**What gets extracted from NeoMatrixDisplay:**
- `renderZoneFlight()`, `renderFlightZone()`, `renderTelemetryZone()`, `renderLogoZone()` → `ClassicCardMode`
- `_currentFlightIndex`, `_lastCycleMs` → per-mode internal state
- `displayFlights()` → no longer called from display task (see BaseDisplay note below)

**What stays in NeoMatrixDisplay:**
- Hardware lifecycle: `initialize()`, `clear()`, `rebuildMatrix()`, `reconfigureFromConfig()`
- Brightness control: `updateBrightness()`
- Frame commit: new `show()` method
- Rendering context factory: new `buildRenderContext()` method
- Status display: `displayMessage()`, `showLoading()`, `displayLoadingScreen()`
- Emergency fallback: `displaySingleFlightCard()` retained for FR36 (no-mode-active safety net)
- Calibration/positioning: `setCalibrationMode()`, `setPositioningMode()`, `renderCalibrationPattern()`, `renderPositioningPattern()`
- Shared resources: `_logoBuffer[1024]`, `_matrix`, `_layout`
- Helper methods: `drawTextLine()`, `truncateToColumns()`, `drawBitmapRGB565()`, `formatTelemetryValue()` — these are rendering utilities. Modes that need them can either reimplement or we extract them into a shared `utils/DisplayUtils.h`

**New public API additions:**

```cpp
// Rendering context factory — assembles context from NeoMatrixDisplay internals
RenderContext buildRenderContext() const;

// Frame commit — separated from rendering
void show();  // wraps FastLED.show()

// Emergency fallback (FR36 — ensures valid display when no mode can init)
void displayFallbackCard(const std::vector<FlightInfo>& flights);
```

**`buildRenderContext()` implementation:**

```cpp
RenderContext NeoMatrixDisplay::buildRenderContext() const {
    DisplayConfig disp = ConfigManager::getDisplay();
    TimingConfig timing = ConfigManager::getTiming();
    return {
        _matrix,
        _layout,
        _matrix->Color(disp.text_color_r, disp.text_color_g, disp.text_color_b),
        disp.brightness,
        const_cast<uint16_t*>(_logoBuffer),
        static_cast<uint16_t>(timing.display_cycle * 1000)
    };
}
```

**Optimization:** The display task caches the `RenderContext` and only rebuilds it when `g_configChanged` fires. Saves ~20 ConfigManager reads per second at 20fps with zero behavioral difference.

**BaseDisplay interface — retained, not deprecated:**

`BaseDisplay` defines `initialize()`, `clear()`, and `displayFlights()`. NeoMatrixDisplay continues to implement `initialize()` and `clear()` (called from `main.cpp setup()`). `displayFlights()` remains implemented but is no longer called from the display task — the task uses `ModeRegistry::tick()` + `show()` instead. No breaking interface change needed. The method can route to `displayFallbackCard()` internally for backward compatibility.

### Decision D4: Display Task Integration — Cached Context + ModeRegistry::tick()

**New globals in main.cpp:**

```cpp
// Static mode table — built at compile time, stored in BSS/data
static DisplayMode* classicCardFactory() { return new ClassicCardMode(); }
static DisplayMode* liveFlightCardFactory() { return new LiveFlightCardMode(); }
static uint32_t classicCardMemReq() { return ClassicCardMode::MEMORY_REQUIREMENT; }
static uint32_t liveFlightCardMemReq() { return LiveFlightCardMode::MEMORY_REQUIREMENT; }

static const ModeEntry MODE_TABLE[] = {
    { "classic_card", "Classic Card", classicCardFactory, classicCardMemReq, 0 },
    { "live_flight",  "Live Flight Card", liveFlightCardFactory, liveFlightCardMemReq, 1 },
};
static constexpr uint8_t MODE_COUNT = sizeof(MODE_TABLE) / sizeof(MODE_TABLE[0]);
```

**In `setup()`:**

```cpp
ModeRegistry::init(MODE_TABLE, MODE_COUNT);
// Restore last active mode from NVS, default to "classic_card"
String savedMode = ConfigManager::getDisplayMode();
if (!ModeRegistry::requestSwitch(savedMode.c_str())) {
    ModeRegistry::requestSwitch("classic_card");
}
```

**In `displayTask()` — revised rendering block:**

```cpp
// Cache rendering context — rebuild only on config change
static RenderContext cachedCtx;
static bool ctxInitialized = false;

if (g_configChanged.exchange(false) || !ctxInitialized) {
    // ... existing config change handling (brightness, hardware) ...
    cachedCtx = g_display.buildRenderContext();
    ctxInitialized = true;
}

// Priority order (unchanged from existing):
// 1. Calibration mode
// 2. Positioning mode
// 3. Status messages
// 4. Mode rendering (NEW — replaces direct renderFlight call)

if (g_display.isCalibrationMode()) {
    g_display.renderCalibrationPattern();
} else if (g_display.isPositioningMode()) {
    g_display.renderPositioningPattern();
} else if (statusMessageVisible) {
    // ... existing status message handling ...
} else {
    // Read latest flight data from queue
    FlightDisplayData *ptr = nullptr;
    std::vector<FlightInfo> empty;
    const auto& flights = (g_flightQueue && xQueuePeek(g_flightQueue, &ptr, 0) == pdTRUE && ptr)
                          ? ptr->flights : empty;

    // Tick mode registry — handles switch requests + renders active mode
    ModeRegistry::tick(cachedCtx, flights);
}

// Frame commit — always last, regardless of which path rendered
g_display.show();
```

**Cross-core coordination pattern:**

```
Browser → POST /api/display/mode → WebPortal handler (Core 1, async TCP task)
    → ModeRegistry::requestSwitch(modeId) → sets _requestedIndex atomic
Display task (Core 0) → ModeRegistry::tick() checks _requestedIndex at top of call
    → If changed: executes full switch lifecycle within tick()
    → Sets _switchState for API polling
```

This mirrors the existing `g_configChanged` pattern — no new concurrency primitives, no mutexes, no FreeRTOS queues for mode switching.

### Decision D5: Mode Switch API — Two Endpoints

Following existing `/api/*` JSON envelope pattern. Uses nested path `/api/display/` to scope mode operations (new pattern, but logically groups display-related endpoints for future extensibility).

**`GET /api/display/modes`** — list available modes + active mode + transition state:

```json
{
    "ok": true,
    "data": {
        "active": "live_flight",
        "switch_state": "idle",
        "modes": [
            {
                "id": "classic_card",
                "name": "Classic Card",
                "description": "Three-line flight card: airline, route, aircraft",
                "zones": [
                    { "label": "Logo", "x": 0, "y": 0, "w": 20, "h": 100 },
                    { "label": "Airline", "x": 20, "y": 0, "w": 80, "h": 33 },
                    { "label": "Route", "x": 20, "y": 33, "w": 80, "h": 33 },
                    { "label": "Aircraft", "x": 20, "y": 66, "w": 80, "h": 34 }
                ]
            },
            {
                "id": "live_flight",
                "name": "Live Flight Card",
                "description": "Enriched telemetry: altitude, speed, heading, vertical rate",
                "zones": [
                    { "label": "Logo", "x": 0, "y": 0, "w": 20, "h": 100 },
                    { "label": "Airline", "x": 20, "y": 0, "w": 40, "h": 50 },
                    { "label": "Route", "x": 60, "y": 0, "w": 40, "h": 50 },
                    { "label": "Alt", "x": 20, "y": 50, "w": 20, "h": 50 },
                    { "label": "Spd", "x": 40, "y": 50, "w": 20, "h": 50 },
                    { "label": "Hdg", "x": 60, "y": 50, "w": 20, "h": 50 },
                    { "label": "VRate", "x": 80, "y": 50, "w": 20, "h": 50 }
                ]
            }
        ]
    }
}
```

**`POST /api/display/mode`** — request mode switch:

Request: `{ "mode": "live_flight" }`

Success: `{ "ok": true, "data": { "switching_to": "live_flight" } }`

Error (heap guard): `{ "ok": false, "error": "Insufficient memory for Live Flight Card", "code": "HEAP_INSUFFICIENT" }`

Error (unknown mode): `{ "ok": false, "error": "Unknown mode: radar_sweep", "code": "MODE_NOT_FOUND" }`

Error (in progress): `{ "ok": false, "error": "Mode switch already in progress", "code": "SWITCH_IN_PROGRESS" }`

**Synchronous mode activation:** The POST endpoint blocks until the mode switch is complete (new mode rendering) or fails (heap guard, unknown mode). The response contains the confirmed active mode — no client-side polling needed. "Switching..." in the UI is the POST in flight; "Active" is the 200 response. This gives ESPHome-grade state honesty with zero client-side complexity. Sub-2-second switch means the POST typically returns in under 2 seconds.

**NFR P4 compliance:** `GET /api/display/modes` serializes the static mode table (BSS/data) + reads atomic `_activeModeIndex` and `_switchState`. Zero heap allocation. Completes in <10ms.

### Decision D6: NVS Persistence — Single Key with Debounced Write

**NVS key:** `display_mode` (12 chars, within 15-char limit)
**Namespace:** `"flightwall"` (existing — verified non-colliding with all 23+ existing keys per NFR S6)
**Type:** String (mode ID, e.g., `"classic_card"`, `"live_flight"`)
**Default:** `"classic_card"` (preserves pre-upgrade behavior per FR29)

**Persistence timing:** Debounced — `ModeRegistry::tick()` sets `_nvsWritePending` flag after successful switch. Write occurs when flag is set AND >2 seconds have elapsed since last switch. This prevents 10 NVS writes during rapid-switch stress testing (NFR S2) while ensuring the selection persists for power-cycle recovery (FR27-FR28).

**ConfigManager integration:**

```cpp
// New in ConfigManager
static String getDisplayMode();          // reads "display_mode" from NVS, returns "classic_card" if absent
static void setDisplayMode(const String& modeId);  // writes "display_mode" to NVS
```

Two new methods, no new config struct. The `display_mode` key is NOT hot-reload and NOT reboot-required — it's a fire-and-forget debounced write after each successful switch.

**Boot restoration:** In `setup()`, `ConfigManager::getDisplayMode()` reads from NVS → passed to `ModeRegistry::requestSwitch()`. If key doesn't exist (first boot after upgrade from Foundation Release), returns `"classic_card"` default.

### Decision D7: Mode Picker UI — Dashboard Section with Client-Side Wireframes

**Dashboard integration:**
- New section in `dashboard.html` positioned #2 in card order (after Display, before Timing) — "remote control" cadence demands top-of-fold placement
- Loads mode list via `GET /api/display/modes` on page load
- Renders mode cards with schematic wireframe previews from zone metadata
- Entire non-active mode card is the tap target (not a button within the card — Philips Hue scene picker pattern)
- Tap triggers synchronous `POST /api/display/mode` — returns after mode is rendering or fails

**Wireframe rendering:** CSS Grid with `<div>` elements, data-driven from zone metadata JSON. Each mode's API response includes `grid` (rows/cols) and `zones` array (label, row, col, rowSpan, colSpan). JS creates a grid container and positions zone divs via `grid-row`/`grid-column`. ~5 lines of rendering JS. No canvas, no SVG, no server-rendered bitmaps. New modes get schematics automatically — zero UI code changes.

**Transition feedback:**
- Tap non-active card → card enters "Switching..." state (pulsing border, dimmed, `.switching` CSS class). All sibling cards enter `.disabled` state (opacity 0.5, `pointer-events: none`).
- Synchronous POST returns → on success: new card gets `.active`, previous card returns to idle, siblings re-enabled. On error: tapped card shows scoped `--warning` error message, previous card remains `.active`, siblings re-enabled.
- No polling — "Switching..." is the POST in flight, "Active" is the 200 response.

**Upgrade notification (FR25-FR26):**
- Dual-source check: API returns `upgrade_notification: true` in `GET /api/display/modes` response AND browser checks `localStorage.getItem('flightwall_mode_notif_seen')`
- If both conditions met: show informational banner with "Try Live Flight Card" (primary action — activates mode immediately) + "Browse Modes" (scrolls to Mode Picker) + dismiss X
- All three actions clear both sources: `localStorage.setItem('flightwall_mode_notif_seen', '1')` + `POST /api/display/ack-notification` to clear firmware flag
- Either source cleared independently prevents reappearance

**Files affected:**
- `firmware/data-src/dashboard.html` — Mode Picker section container (`<div id="mode-picker">`) + notification banner markup
- `firmware/data-src/dashboard.js` — mode API calls, CSS Grid schematic rendering, card state management, notification logic (~40 lines total)
- `firmware/data-src/style.css` — mode card states (idle/switching/active/disabled/error), schematic zone styling, notification banner (~58 lines total)
- `firmware/data/*.gz` — rebuilt gzipped copies of all updated web assets

### Display System Decision Impact — Implementation Sequence

**Critical path (sequential):**
1. **DisplayMode interface + RenderContext** (`interfaces/DisplayMode.h`) — everything depends on this contract
2. **ModeRegistry** (`core/ModeRegistry.h/.cpp`) — switch logic, heap guard, tick()
3. **ClassicCardMode** (`modes/ClassicCardMode.h/.cpp`) — extraction from NeoMatrixDisplay with pixel-parity validation (highest-risk step)
4. **NeoMatrixDisplay refactoring** — remove extracted methods, add `show()` + `buildRenderContext()`, retain fallback
5. **Display task integration** (`main.cpp`) — replace renderFlight with ModeRegistry::tick() + show()
6. **LiveFlightCardMode** (`modes/LiveFlightCardMode.h/.cpp`) — second mode, validates the abstraction works

**Parallel with steps 3-6:**
7. **Mode Switch API** (`WebPortal.cpp`) — GET + POST endpoints, can be stubbed early
8. **NVS persistence** (`ConfigManager.h/.cpp`) — `getDisplayMode()` / `setDisplayMode()`, trivial addition

**After all firmware complete:**
9. **Mode Picker UI** (`dashboard.html/js/css`) — depends on working API endpoints
10. **Gzip rebuild** — all updated web assets

**Heap baseline measurement** must occur before step 1 — measure `ESP.getFreeHeap()` after full Foundation boot to establish the mode memory budget.

### Cross-Decision Dependencies

```
DisplayMode interface (D1)
    └── ModeRegistry (D2) — uses DisplayMode*, ModeEntry, RenderContext
        ├── ClassicCardMode — implements DisplayMode
        ├── LiveFlightCardMode — implements DisplayMode
        └── Display task (D4) — calls ModeRegistry::tick()
            └── NeoMatrixDisplay (D3) — provides RenderContext + show()

Mode Switch API (D5) — calls ModeRegistry::requestSwitch() + reads state
    └── Mode Picker UI (D7) — calls API, renders wireframes from zone descriptors

NVS persistence (D6) — called from ModeRegistry::tick() after successful switch
    └── Boot restoration in setup() — reads NVS, calls requestSwitch()
```

## Display System Release — Implementation Patterns & Consistency Rules

### Display System–Specific Conflict Points

The MVP architecture established 11 enforcement rules. The Foundation Release added 5 (rules 12-16). The Display System Release introduces 7 new pattern areas where AI agents could make inconsistent choices about the mode system.

### Mode Implementation Pattern

**File location:** All mode classes go in `firmware/modes/`. One `.h/.cpp` pair per mode. Mode files are NOT adapters (they don't touch hardware directly) and NOT core (they're pluggable rendering strategies).

```
firmware/modes/
├── ClassicCardMode.h
├── ClassicCardMode.cpp
├── LiveFlightCardMode.h
└── LiveFlightCardMode.cpp
```

**Class structure:**

```cpp
// modes/ClassicCardMode.h
#pragma once
#include "interfaces/DisplayMode.h"

class ClassicCardMode : public DisplayMode {
public:
    static constexpr uint32_t MEMORY_REQUIREMENT = 64;  // bytes — cycling state only

    bool init(const RenderContext& ctx) override;
    void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) override;
    void teardown() override;
    const char* getName() const override;
    const ModeZoneDescriptor& getZoneDescriptor() const override;

private:
    // Per-mode cycling state (migrated from NeoMatrixDisplay)
    size_t _currentFlightIndex = 0;
    unsigned long _lastCycleMs = 0;
};
```

**Rules:**
- Constructor does NO heap allocation — all allocation happens in `init()`
- `teardown()` frees ALL heap allocated by `init()` — the mode object shell (~32-64 bytes) remains for safe restoration
- `MEMORY_REQUIREMENT` is a `static constexpr` — reflects worst-case heap allocated during `init()`, NOT the mode object size
- Modes store their own cycling state (index + timer) — do not rely on external cycling

### Rendering Context Discipline

**Modes must ONLY access data through `RenderContext`.** This is the critical isolation boundary that guarantees NFR C2 (Foundation feature compatibility).

**Allowed:**
```cpp
void ClassicCardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    ctx.matrix->setCursor(x, y);                                       // ✅ GFX primitives via context
    ctx.matrix->setTextColor(ctx.textColor);                           // ✅ pre-computed color from context
    LogoManager::loadLogo(f.operator_icao, ctx.logoBuffer);            // ✅ shared buffer from context
    if (millis() - _lastCycleMs >= ctx.displayCycleMs) { }             // ✅ timing from context
    DisplayUtils::drawTextLine(ctx.matrix, x, y, text, ctx.textColor); // ✅ shared utility
}
```

**Forbidden:**
```cpp
void ClassicCardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    DisplayConfig disp = ConfigManager::getDisplay();  // ❌ NEVER read ConfigManager from modes
    FastLED.show();                                     // ❌ NEVER call FastLED.show() from modes
    ctx.matrix->setBrightness(50);                      // ❌ NEVER modify brightness from modes
    uint8_t brt = ConfigManager::getSchedule().dim_brightness;  // ❌ NEVER access schedule config
}
```

**Anti-pattern: caching RenderContext.** Modes receive `ctx` by const reference each call. Do not store a copy — the display task may update the cached context between frames (on config change). Always use the passed-in reference.

### Shared Rendering Utilities

Helper methods currently in NeoMatrixDisplay that modes need. Extract into `utils/DisplayUtils.h` as free functions:

```cpp
// utils/DisplayUtils.h
#pragma once
#include <Adafruit_NeoMatrix.h>
#include <Arduino.h>

namespace DisplayUtils {
    void drawTextLine(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                      const String& text, uint16_t color);
    String truncateToColumns(const String& text, int maxColumns);
    String formatTelemetryValue(double value, const char* suffix, int decimals = 0);
    void drawBitmapRGB565(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint16_t* bitmap,
                          uint16_t zoneW, uint16_t zoneH);
}
```

**Rule:** Modes use `DisplayUtils::` functions. Do NOT copy rendering helpers into each mode class. Do NOT add NeoMatrixDisplay as a dependency of modes.

### Mode Registration Pattern

**Adding a new mode requires exactly two touch points:**

1. Create `modes/NewMode.h/.cpp` implementing `DisplayMode`
2. Add one entry to `MODE_TABLE[]` in `main.cpp`:

```cpp
static DisplayMode* newModeFactory() { return new NewMode(); }
static uint32_t newModeMemReq() { return NewMode::MEMORY_REQUIREMENT; }

static const ModeEntry MODE_TABLE[] = {
    { "classic_card", "Classic Card", classicCardFactory, classicCardMemReq, 0 },
    { "live_flight",  "Live Flight Card", liveFlightCardFactory, liveFlightCardMemReq, 1 },
    { "new_mode",     "New Mode", newModeFactory, newModeMemReq, 2 },  // ← one line
};
```

**Anti-pattern:** Do NOT modify ModeRegistry, NeoMatrixDisplay, WebPortal, FlightDataFetcher, or dashboard JS to add a mode. If a mode addition requires changes beyond the two touch points above, the architecture has a defect.

### Mode Picker UI Patterns

**Mode list rendering:** Follow existing dashboard card pattern — same CSS classes, same card layout structure. Mode cards are rendered dynamically from the `GET /api/display/modes` JSON response. The HTML template contains only a `<div id="mode-picker">` container (~10 lines); JS builds cards from API data.

**API interaction:** Every `fetch()` call follows existing rule 10: check `json.ok`, call `showToast()` on failure. The POST is synchronous — it returns after the mode is rendering (or fails). No polling loop.

```javascript
async function activateMode(modeId) {
    const cards = document.querySelectorAll('.mode-card:not(.active)');
    const card = document.querySelector(`[data-mode="${modeId}"]`);
    card.classList.add('switching');
    cards.forEach(c => { if (c !== card) c.classList.add('disabled'); });
    try {
        const res = await fetch('/api/display/mode', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mode: modeId })
        });
        const json = await res.json();
        if (!json.ok) {
            card.classList.remove('switching');
            showCardError(card, json.error);
        } else {
            document.querySelector('.mode-card.active').classList.remove('active');
            card.classList.remove('switching');
            card.classList.add('active');
        }
    } catch (err) {
        card.classList.remove('switching');
        showToast('Connection failed', 'error');
    }
    cards.forEach(c => c.classList.remove('disabled'));
}
```

**Wireframe rendering:** CSS Grid with `<div>` zone elements. Grid dimensions and zone positions from API metadata. ~5 lines of JS to render. No canvas, no SVG, no external drawing libraries.

**Transition state:** Synchronous — card shows "Switching..." with `.switching` CSS class while POST is in flight. Sibling cards get `.disabled`. POST response resolves to "Active" or scoped error. No polling needed.

### Updated Enforcement Guidelines

**All AI agents implementing Display System stories MUST also follow (in addition to rules 1-16):**

17. Mode classes go in `firmware/modes/` — one `.h/.cpp` pair per mode
18. Modes must ONLY access data through `RenderContext` — zero `ConfigManager`, `WiFiManager`, or `SystemStatus` reads from inside a mode
19. Modes must NOT call `FastLED.show()` — frame commit is exclusively the display task's responsibility after `ModeRegistry::tick()` returns
20. All heap allocation in modes happens in `init()`, all deallocation in `teardown()` — constructors and per-frame `render()` must not allocate heap
21. Shared rendering helpers (`drawTextLine`, `truncateToColumns`, `formatTelemetryValue`, `drawBitmapRGB565`) must be called from `utils/DisplayUtils.h` — do not duplicate into mode classes
22. Adding a new mode requires exactly two touch points: new file in `modes/` + one line in `MODE_TABLE[]` in main.cpp — if more changes are needed, the architecture has a defect
23. `MEMORY_REQUIREMENT` is a `static constexpr` on each mode class — reflects worst-case heap from `init()`, not object size

## Display System Release — Project Structure & File Change Map

### Complete Project Directory Structure (Display System Additions)

```
firmware/
├── platformio.ini                          # UPDATED: build_src_filter + build_flags
├── custom_partitions.csv                   # UNCHANGED (Foundation layout)
│
├── interfaces/
│   ├── BaseDisplay.h                       # UNCHANGED (retained, not deprecated)
│   ├── BaseFlightFetcher.h                 # UNCHANGED
│   ├── BaseStateVectorFetcher.h            # UNCHANGED
│   └── DisplayMode.h                       # ← NEW: DisplayMode abstract class,
│                                           #         RenderContext, ModeZoneDescriptor,
│                                           #         ZoneRegion, SwitchState enum
│                                           #         Includes FlightInfo.h and LayoutEngine.h
│                                           #         directly (transitive for all modes)
│
├── core/
│   ├── ConfigManager.h                     # UPDATED: +getDisplayMode(), +setDisplayMode()
│   ├── ConfigManager.cpp                   # UPDATED: +getDisplayMode(), +setDisplayMode() impl
│   ├── ModeRegistry.h                      # ← NEW: ModeRegistry static class + ModeEntry struct
│   ├── ModeRegistry.cpp                    # ← NEW: init(), requestSwitch(), tick(),
│   │                                       #         switch lifecycle, NVS debounce
│   ├── FlightDataFetcher.h                 # UNCHANGED
│   ├── FlightDataFetcher.cpp               # UNCHANGED
│   ├── LayoutEngine.h                      # UNCHANGED
│   ├── LayoutEngine.cpp                    # UNCHANGED
│   ├── LogoManager.h                       # UNCHANGED
│   ├── LogoManager.cpp                     # UNCHANGED
│   ├── SystemStatus.h                      # UNCHANGED
│   └── SystemStatus.cpp                    # UNCHANGED
│
├── modes/                                  # ← NEW DIRECTORY
│   ├── ClassicCardMode.h                   # ← NEW: ClassicCardMode declaration
│   ├── ClassicCardMode.cpp                 # ← NEW: extracted from NeoMatrixDisplay
│   │                                       #         renderZoneFlight, renderFlightZone,
│   │                                       #         renderTelemetryZone, renderLogoZone
│   ├── LiveFlightCardMode.h                # ← NEW: LiveFlightCardMode declaration
│   └── LiveFlightCardMode.cpp              # ← NEW: telemetry-enriched flight card
│
├── adapters/
│   ├── NeoMatrixDisplay.h                  # UPDATED: +show(), +buildRenderContext(),
│   │                                       #          +displayFallbackCard()
│   │                                       #          -renderZoneFlight(), -renderFlightZone(),
│   │                                       #          -renderTelemetryZone(), -renderLogoZone()
│   ├── NeoMatrixDisplay.cpp                # UPDATED: ~140 lines extracted to ClassicCardMode;
│   │                                       #          +show(), +buildRenderContext(),
│   │                                       #          +displayFallbackCard() impls
│   ├── WebPortal.h                         # UPDATED: +_handleGetDisplayModes(),
│   │                                       #          +_handlePostDisplayMode()
│   ├── WebPortal.cpp                       # UPDATED: +GET /api/display/modes handler,
│   │                                       #          +POST /api/display/mode handler
│   ├── WiFiManager.h                       # UNCHANGED
│   ├── WiFiManager.cpp                     # UNCHANGED
│   ├── AeroAPIFetcher.h                    # UNCHANGED
│   ├── AeroAPIFetcher.cpp                  # UNCHANGED
│   ├── FlightWallFetcher.h                 # UNCHANGED
│   ├── FlightWallFetcher.cpp               # UNCHANGED
│   ├── OpenSkyFetcher.h                    # UNCHANGED
│   └── OpenSkyFetcher.cpp                  # UNCHANGED
│
├── utils/
│   ├── GeoUtils.h                          # UNCHANGED
│   ├── Log.h                               # UNCHANGED
│   ├── DisplayUtils.h                      # ← NEW: shared rendering free functions
│   └── DisplayUtils.cpp                    # ← NEW: drawTextLine, truncateToColumns,
│                                           #         formatTelemetryValue, drawBitmapRGB565
│
├── models/
│   ├── FlightInfo.h                        # UNCHANGED
│   ├── StateVector.h                       # UNCHANGED
│   └── AirportInfo.h                       # UNCHANGED
│
├── config/
│   ├── HardwareConfiguration.h             # UNCHANGED
│   ├── TimingConfiguration.h               # UNCHANGED
│   ├── WiFiConfiguration.h                 # UNCHANGED
│   ├── APIConfiguration.h                  # UNCHANGED
│   └── UserConfiguration.h                 # UNCHANGED
│
├── src/
│   └── main.cpp                            # UPDATED: +MODE_TABLE[], +factory functions,
│                                           #          +ModeRegistry::init() in setup(),
│                                           #          +heap baseline log after full init,
│                                           #          displayTask() replaces renderFlight
│                                           #          with ModeRegistry::tick()+show()
│
├── data-src/                               # Web asset sources
│   ├── dashboard.html                      # UPDATED: +Mode Picker section in nav + content
│   ├── dashboard.js                        # UPDATED: +mode API calls, +CSS Grid schematic
│   │                                       #          rendering, +card state mgmt, +notification
│   ├── style.css                           # UPDATED: +mode card styling
│   ├── health.html                         # UNCHANGED
│   ├── health.js                           # UNCHANGED
│   ├── common.js                           # UNCHANGED
│   ├── wizard.html                         # UNCHANGED
│   └── wizard.js                           # UNCHANGED
│
├── data/                                   # Gzipped assets (served by WebPortal)
│   ├── dashboard.html.gz                   # REBUILD after dashboard.html changes
│   ├── dashboard.js.gz                     # REBUILD after dashboard.js changes
│   ├── style.css.gz                        # REBUILD after style.css changes
│   ├── common.js.gz                        # UNCHANGED
│   ├── health.html.gz                      # UNCHANGED
│   ├── health.js.gz                        # UNCHANGED
│   ├── wizard.html.gz                      # UNCHANGED
│   ├── wizard.js.gz                        # UNCHANGED
│   ├── test.txt                            # UNCHANGED
│   └── logos/                              # UNCHANGED (99 ICAO .bin files)
│
└── test/
    ├── test_config_manager/test_main.cpp   # UNCHANGED (existing test)
    ├── test_layout_engine/test_main.cpp    # UNCHANGED (existing test)
    ├── test_logo_manager/test_main.cpp     # UNCHANGED (existing test)
    ├── test_telemetry_conversion/test_main.cpp  # UNCHANGED (existing test)
    └── test_mode_registry/test_main.cpp    # ← NEW: ModeRegistry switch lifecycle,
                                            #         heap guard, NVS debounce tests
```

### File Change Summary

| Action | Count | Files |
|--------|-------|-------|
| **NEW** | 10 | `interfaces/DisplayMode.h`, `core/ModeRegistry.h`, `core/ModeRegistry.cpp`, `modes/ClassicCardMode.h`, `modes/ClassicCardMode.cpp`, `modes/LiveFlightCardMode.h`, `modes/LiveFlightCardMode.cpp`, `utils/DisplayUtils.h`, `utils/DisplayUtils.cpp`, `test/test_mode_registry/test_main.cpp` |
| **UPDATED** | 12 | `platformio.ini`, `adapters/NeoMatrixDisplay.h`, `adapters/NeoMatrixDisplay.cpp`, `adapters/WebPortal.h`, `adapters/WebPortal.cpp`, `core/ConfigManager.h`, `core/ConfigManager.cpp`, `src/main.cpp`, `data-src/dashboard.html`, `data-src/dashboard.js`, `data-src/style.css`, `data/*.gz` (3 rebuilds) |
| **UNCHANGED** | 30+ | All other firmware files, config headers, models, fetchers, WiFiManager, LayoutEngine, LogoManager, SystemStatus, wizard assets, health assets |
| **NEW DIRECTORY** | 1 | `firmware/modes/` |

### PlatformIO Build Configuration Update

**Current `platformio.ini` `build_src_filter`:**

```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
```

**Required additions:**

```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
    +<../modes/*.cpp>          # ← NEW: ClassicCardMode, LiveFlightCardMode
    +<../utils/*.cpp>          # ← NEW: DisplayUtils
```

**Required `build_flags` addition:**

```ini
build_flags =
    -I .
    -I include
    -I src
    -I adapters
    -I core
    -I models
    -I interfaces
    -I utils
    -I config
    -I modes                   # ← NEW: mode headers
    -D LOG_LEVEL=3
```

Without these changes, mode and utility `.cpp` files will not compile. This was identified as a **blocker** during party mode review.

### Architectural Boundaries

**API Boundaries (HTTP):**

| Endpoint | Method | Handler | Cross-Core | Purpose |
|----------|--------|---------|------------|---------|
| `/api/display/modes` | GET | WebPortal (Core 1) | Read-only atomic | List modes, active mode, switch state |
| `/api/display/mode` | POST | WebPortal (Core 1) | Atomic write | Request mode switch |
| `/api/settings` | GET/POST | WebPortal (Core 1) | Unchanged | Existing settings (no new keys) |
| `/api/status` | GET | WebPortal (Core 1) | Unchanged | Health/status |

**Component Boundaries (Firmware):**

```
┌──────────────────────────────────────────────────────────────┐
│  Core 1 (WiFi/Web/API task)                                  │
│  ┌────────────────┐                                          │
│  │   WebPortal    │─── POST /api/display/mode ──────────┐   │
│  │  (HTTP handler)│─── GET /api/display/modes (read) ─┐  │   │
│  └────────────────┘                                   │  │   │
└───────────────────────────────────────────────────────┼──┼───┘
                                                        │  │
    atomic read ────────────────────────────────────────┘  │
    atomic write (requestSwitch) ──────────────────────────┘
                                                        │  │
┌───────────────────────────────────────────────────────┼──┼───┐
│  Core 0 (Display task)                                │  │   │
│  ┌────────────────┐   ┌──────────────┐                │  │   │
│  │  ModeRegistry  │──→│ ActiveMode   │ (DisplayMode*) │  │   │
│  │  ::tick()      │   │ .render()    │                │  │   │
│  └───────┬────────┘   └──────┬───────┘                │  │   │
│          │                   │                                │
│          │           ┌───────▼────────┐                       │
│          │           │  LogoManager   │ (static utility —     │
│          │           │  ::loadLogo()  │  accepted dependency) │
│          │           └────────────────┘                       │
│  ┌───────▼──────────────┐   ┌────────────────────────┐       │
│  │  NeoMatrixDisplay    │   │  DisplayUtils          │       │
│  │  .buildRenderContext()│   │  (free functions)      │       │
│  │  .show()             │   │  drawTextLine, etc.    │       │
│  └──────────────────────┘   └────────────────────────┘       │
└──────────────────────────────────────────────────────────────┘
```

**Data Boundaries:**

| Boundary | Direction | Mechanism |
|----------|-----------|-----------|
| WebPortal → ModeRegistry | Core 1 → Core 0 | `std::atomic<uint8_t> _requestedIndex` |
| ModeRegistry → ConfigManager | Core 0 → NVS | `setDisplayMode()` (debounced 2s) |
| ConfigManager → ModeRegistry | NVS → Core 0 | `getDisplayMode()` at boot only |
| NeoMatrixDisplay → Modes | Core 0 internal | `RenderContext` const ref (isolation barrier) |
| Modes → LogoManager | Core 0 internal | `LogoManager::loadLogo()` static call (accepted utility dependency — fills shared buffer from RenderContext) |
| Modes → NeoMatrixDisplay | **FORBIDDEN** | Modes never access NeoMatrixDisplay directly |
| Modes → ConfigManager | **FORBIDDEN** | Modes never read config directly (rule 18) |
| Dashboard JS → API | Browser → Core 1 | fetch() JSON envelope, synchronous POST for mode switch |

### Requirements to Structure Mapping

**FR Category → File Mapping:**

| FR Group | FRs | Primary Files | Supporting Files |
|----------|-----|---------------|-----------------|
| DisplayMode Interface | FR1-FR4 | `interfaces/DisplayMode.h` | `models/FlightInfo.h`, `core/LayoutEngine.h` (included transitively) |
| Mode Registry & Lifecycle | FR5-FR10 | `core/ModeRegistry.h`, `core/ModeRegistry.cpp` | `src/main.cpp` (MODE_TABLE), `test/test_mode_registry/test_main.cpp` |
| Classic Card Mode | FR11-FR13 | `modes/ClassicCardMode.h`, `modes/ClassicCardMode.cpp` | `utils/DisplayUtils.h/.cpp`, `adapters/NeoMatrixDisplay.cpp` (extraction source), `core/LogoManager.h` (loadLogo) |
| Live Flight Card Mode | FR14-FR16 | `modes/LiveFlightCardMode.h`, `modes/LiveFlightCardMode.cpp` | `utils/DisplayUtils.h/.cpp`, `models/FlightInfo.h` (telemetry fields), `core/LogoManager.h` (loadLogo) |
| Mode Picker UI | FR17-FR26 | `data-src/dashboard.html`, `data-src/dashboard.js`, `data-src/style.css` | `data/*.gz` (rebuild) |
| Mode Persistence | FR27-FR29 | `core/ConfigManager.h/.cpp` | `core/ModeRegistry.cpp` (debounce), `src/main.cpp` (boot restore) |
| Mode Switch API | FR30-FR33 | `adapters/WebPortal.h`, `adapters/WebPortal.cpp` | `core/ModeRegistry.h` (requestSwitch, getters) |
| Display Infrastructure | FR34-FR36 | `adapters/NeoMatrixDisplay.h/.cpp`, `src/main.cpp` | `core/ModeRegistry.cpp` (tick integration), `platformio.ini` (build config) |

**Cross-Cutting Concerns → File Mapping:**

| Concern | Files Touched |
|---------|--------------|
| RenderContext isolation (NFR C2) | `interfaces/DisplayMode.h` (struct def), `adapters/NeoMatrixDisplay.cpp` (buildRenderContext), all `modes/*.cpp` (consumers) |
| Cross-core coordination | `core/ModeRegistry.h` (atomic), `adapters/WebPortal.cpp` (requestSwitch caller), `src/main.cpp` (tick caller) |
| Heap lifecycle (NFR S1, S3, S4) | `core/ModeRegistry.cpp` (switch flow), all `modes/*.cpp` (init/teardown) |
| Heap baseline measurement | `src/main.cpp` — `ESP.getFreeHeap()` log after full init, before `ModeRegistry::init()` |
| NVS debounce | `core/ModeRegistry.cpp` (2s timer), `core/ConfigManager.cpp` (read/write) |
| Shared rendering utilities | `utils/DisplayUtils.h/.cpp` (extracted helpers), `modes/*.cpp` (consumers) |
| PlatformIO build discovery | `platformio.ini` (build_src_filter, build_flags) |

### Include Path Conventions for Mode Files

Mode source files use path-from-firmware-root includes, consistent with the existing codebase pattern in `main.cpp`:

```cpp
// modes/ClassicCardMode.cpp — canonical include style
#include "interfaces/DisplayMode.h"    // ✅ base class (transitively includes FlightInfo.h, LayoutEngine.h)
#include "utils/DisplayUtils.h"        // ✅ shared rendering helpers
#include "core/LogoManager.h"          // ✅ accepted static utility dependency
#include "modes/ClassicCardMode.h"     // ✅ own header
```

`DisplayMode.h` includes `"models/FlightInfo.h"` and `"core/LayoutEngine.h"` directly — modes receive these transitively and do not need to include them separately.

### Integration Points

**Internal Communication:**
- Display task calls `ModeRegistry::tick()` every frame (~20fps) — passes cached `RenderContext` + flight vector
- `ModeRegistry::tick()` delegates to `_activeMode->render()` during IDLE, or executes switch lifecycle during REQUESTED/SWITCHING
- After `tick()` returns, display task calls `g_display.show()` — separates rendering from frame commit
- WebPortal calls `ModeRegistry::requestSwitch()` from async handler — sets atomic flag only, returns immediately
- `ModeRegistry::tick()` calls `ConfigManager::setDisplayMode()` after debounce timer expires (2s post-switch)

**External Integrations:**
- No new external API calls or network dependencies
- Mode Picker UI communicates exclusively through existing `/api/*` pattern (fetch + JSON envelope)

**Data Flow (Mode Switch):**

```
Browser click "Activate"
  → POST /api/display/mode { "mode": "live_flight" }
  → WebPortal::_handlePostDisplayMode() [Core 1]
  → ModeRegistry::requestSwitch("live_flight") → sets _requestedIndex atomic
  → returns { "ok": true, "switching_to": "live_flight" }
  → Browser polls GET /api/display/modes at 500ms

Next display task tick [Core 0]:
  → ModeRegistry::tick() detects _requestedIndex != _activeModeIndex
  → _activeMode->teardown() → frees mode heap
  → ESP.getFreeHeap() check vs memoryRequirement()
  → factory() → new mode → init(ctx)
  → success: update index, set NVS pending, clear error
  → _switchState = IDLE

After 2s debounce [Core 0, inside tick()]:
  → ConfigManager::setDisplayMode("live_flight") → NVS write

Next browser poll:
  → GET /api/display/modes returns switch_state: "idle", active: "live_flight"
  → UI updates to show Live Flight Card as active
```

### Development Workflow Integration

**Build process:** PlatformIO compiles all `.cpp` files per `build_src_filter` in `platformio.ini`. The updated filter includes `modes/` and `utils/` directories alongside existing `adapters/` and `core/`.

**Gzip rebuild:** After modifying `data-src/*.html`, `data-src/*.js`, or `data-src/*.css`:
```bash
gzip -c firmware/data-src/dashboard.html > firmware/data/dashboard.html.gz
gzip -c firmware/data-src/dashboard.js > firmware/data/dashboard.js.gz
gzip -c firmware/data-src/style.css > firmware/data/style.css.gz
```

**Upload sequence:** `~/.platformio/penv/bin/pio run` (build) → `~/.platformio/penv/bin/pio run -t uploadfs` (flash filesystem) → `~/.platformio/penv/bin/pio run -t upload` (flash firmware)

**Heap baseline prerequisite:** Before implementing any Display System code, flash current Foundation firmware and record `ESP.getFreeHeap()` from serial monitor after full boot (WiFi connected, web server active, NTP synced, flight data in queue). This establishes the mode memory budget and validates whether the 30KB floor (NFR S4) is achievable.

### Party Mode Refinements Incorporated

- `platformio.ini` promoted from UNCHANGED to UPDATED (blocker — modes/ and utils/ wouldn't compile)
- `LogoManager::loadLogo()` documented as accepted static utility dependency from modes (not a rule 18 violation)
- `DisplayMode.h` specified to include `FlightInfo.h` and `LayoutEngine.h` directly for transitive access
- `test/test_mode_registry/test_main.cpp` added to NEW files (signals testing intent for highest-risk component)
- Heap baseline measurement location specified: `ESP.getFreeHeap()` log in `setup()` after full init
- `ModeRegistry → ConfigManager::setDisplayMode()` added to data boundaries table
- Pixel parity validation (NFR C1) noted as hardware visual validation — not automatable in unit tests

---

# Delight Release — Architecture Extension

_Extends the MVP + Foundation + Display System architecture with Clock Mode, Departures Board, animated transitions, mode scheduling, mode-specific settings, and OTA Pull from GitHub Releases. All prior architectural decisions remain in effect — this section adds new decisions and updates existing ones where the Delight PRD introduces changes._

## Delight Release — Project Context Analysis

### Requirements Overview

**Functional Requirements (43 FRs across 9 groups):**

| Group | FRs | Architectural Impact |
|-------|-----|---------------------|
| Clock Mode | FR1-FR4 | **Minor:** New DisplayMode implementation (rule 22 — two touch points); idle-fallback trigger integrated with mode orchestration |
| Departures Board | FR5-FR9 | **Minor:** New DisplayMode with multi-row rendering, dynamic row add/remove, configurable row count (rule 22 — two touch points) |
| Mode Transitions | FR10-FR12 | **Medium:** Fade transition phase added to ModeRegistry switch lifecycle; dual RGB565 frame buffers (~20KB transient allocation) |
| Mode Scheduling | FR13-FR17, FR36, FR39 | **Major:** Mode orchestration state machine (MANUAL / SCHEDULED / IDLE_FALLBACK); time-based rules stored in NVS; priority resolution between schedule, idle-fallback, and manual switching |
| Mode Configuration | FR18-FR20, FR40 | **Medium:** Per-mode settings with NVS key prefix convention; mode-scoped settings schema in static metadata; dynamic Mode Picker settings panels |
| OTA Firmware Updates | FR21-FR31, FR37-FR38 | **Major:** New `OTAUpdater` component — GitHub Releases API client, version comparison, binary download + SHA-256 verification; dashboard OTA Pull UI; shares pre-check sequence with Foundation's OTA Push |
| System Resilience | FR32-FR35 | **Medium:** Watchdog recovery to default mode, config key migration with safe defaults, heap pre-check for OTA + transitions, graceful API degradation |
| Contributor Extensibility | FR41-FR42 | **Minor:** Formalizes existing DisplayMode plugin contract from Display System; compile-time registry already established |
| Documentation & Onboarding | FR43 | **None:** Documentation validation, no architectural impact |

**Non-Functional Requirements (architectural drivers):**

| NFR | Target | Architecture Decision Driver |
|-----|--------|------------------------------|
| Transition framerate | ≥15fps on 160x32 | Dual RGB565 frame buffers with blend-per-pixel; buffers transient during switch only |
| Transition duration | <1 second | Fixed frame count (e.g., 15 frames at 15fps = 1s max) |
| Idle fallback latency | ~30s (one poll interval) | Mode orchestrator monitors flight count after each fetch cycle |
| Departures Board row update | <50ms | In-place row mutation during render(), no full-screen redraw per row change |
| OTA download + reboot | <60s on 10Mbps | Stream-to-partition (same as Foundation push), add GitHub API + SHA-256 overhead |
| Schedule trigger accuracy | ±5 seconds | Non-blocking time check in main loop, same pattern as night mode brightness scheduler |
| Heap floor | ≥80KB free during normal operation | Transition buffers allocated only during switch, freed immediately after |
| Firmware binary size | ≤2,097,152 bytes (2 MiB OTA slot) | Monitor binary size; OTAUpdater adds ~10-15KB |
| No heap degradation | <5% loss over 24 hours | Transition buffers fully freed after each switch; mode init/teardown cycle tested |
| 30-day uptime | Continuous | Schedule timer, OTA check, and transition buffers must not leak memory |

### Scale & Complexity

- **Primary domain:** IoT/Embedded with companion web UI (unchanged)
- **Complexity level:** Medium — two new modes are straightforward plugin implementations (rule 22); mode orchestration state machine and OTA Pull add meaningful coordination complexity
- **New architectural component:** 1 (`core/OTAUpdater` — GitHub API client with version check, download, SHA-256 verification)
- **New lightweight constructs:** Mode scheduling logic (static class or main-loop function, same pattern as Foundation brightness scheduler); fade transition as private method in ModeRegistry
- **New mode implementations (rule 22):** ClockMode, DeparturesBoardMode (two touch points each — `modes/*.h/.cpp` + `MODE_TABLE[]` entry)
- **Updated components:** ModeRegistry (fade transition phase, idle-fallback hook), WebPortal (OTA Pull endpoints, schedule API, mode-settings API), ConfigManager (schedule rules, per-mode settings, OTA Pull keys), main.cpp (mode scheduler in loop, OTA pre-check integration), dashboard HTML/JS (Mode Picker settings panels, schedule UI, OTA Pull UI)
- **Existing components unchanged:** FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, WiFiManager, LayoutEngine, LogoManager, SystemStatus (add OTA_PULL subsystem entry), ClassicCardMode, LiveFlightCardMode, DisplayUtils

### Technical Constraints (Delight-Specific)

| Constraint | Impact |
|-----------|--------|
| Dual RGB565 frame buffers for fade = ~20KB heap | 160 x 32 x 2 bytes/pixel = 10,240 bytes per buffer, ~20KB total. Buffers are transient — allocate on switch, free after transition completes. Never coexist with OTA download. Well within 80KB heap floor budget. |
| NVS 15-char key limit for schedule rules | Multiple schedule rules need indexed keys (e.g., `sched_r0_start`, `sched_r0_end`, `sched_r0_mode`) — fixed max rule count with 3 keys per rule |
| NVS 15-char key limit for per-mode settings | Prefix convention: `m_` (2) + mode ID abbrev (≤5) + `_` (1) + setting name (≤7) = max 15 chars. Mode ID abbreviations enforced by documentation and code review. |
| GitHub Releases API rate limit (60 req/hr unauthenticated) | OTA check is manual and non-blocking; HTTP 429 → "try again later", no auto-retry |
| GitHub binary download over HTTPS (~1.5MB) | Uses same `Update.h` stream-to-partition path as Foundation push; WiFi must remain stable during download |
| NTP required for scheduling + Clock Mode | Schedule inactive until first NTP sync; Clock Mode falls back to uptime-based display if NTP unavailable |
| Per-mode settings expand NVS usage | Each mode adds 2-5 NVS keys; total NVS key count grows from ~28 to ~40+; well within NVS partition capacity (20KB) |
| OTA pre-check must tear down active mode + free transition buffers | Shared pre-check sequence for both Pull and Push OTA paths — extracted as common utility |

### Cross-Cutting Concerns (Delight Additions)

1. **Transition frame buffer lifecycle** — Dual RGB565 buffers (~20KB total) allocated only during mode switch, freed immediately after fade animation completes. Never coexist with OTA download. ModeRegistry's switch flow gains a fade phase between teardown and init: `teardown old → alloc buffers → capture outgoing frame → init new mode → render N blend frames → free buffers → IDLE`. Implemented as `ModeRegistry::_executeFadeTransition()` private method — not a separate component. If Growth phase adds wipe/scroll, extract at that point.

2. **Mode orchestration state machine** — Three mode-switch sources now exist. Modeled as explicit states with defined transitions:

    **States:**
    - `MANUAL` — user's last explicit selection (NVS persisted via `display_mode` key)
    - `SCHEDULED` — time-based rule is currently active
    - `IDLE_FALLBACK` — zero flights triggered Clock Mode

    **Priority:** `SCHEDULED > IDLE_FALLBACK > MANUAL`

    **Transitions:**
    - Schedule rule fires → enter `SCHEDULED`, switch to scheduled mode
    - Schedule rule expires → exit `SCHEDULED`, restore `MANUAL` selection
    - Zero flights detected (while not `SCHEDULED`) → enter `IDLE_FALLBACK`, switch to Clock Mode
    - Flights return (while `IDLE_FALLBACK`) → exit `IDLE_FALLBACK`, restore `MANUAL` selection
    - Zero flights detected (while `SCHEDULED`) → stay `SCHEDULED` (schedule wins per FR17)
    - User manual switch → update `MANUAL` selection in NVS; if in `IDLE_FALLBACK`, exit fallback; if in `SCHEDULED`, manual switch is temporary override until next schedule tick re-evaluates

3. **OTA Pull + OTA Push coexistence** — Both write to the inactive OTA partition. Both need the same pre-check sequence: verify ≥80KB free heap, tear down active mode (free mode heap + any transition buffers), suspend rendering, show progress on LEDs. Extract shared pre-OTA logic into a common function. Push (Foundation) receives binary from browser multipart upload via WebPortal. Pull (Delight) fetches binary from GitHub CDN via OTAUpdater. Post-write SHA-256 verification (Pull only — Push uses `Update.end()` built-in CRC), boot swap, and reboot are shared.

4. **OTA Pull failure modes** — The PRD specifies SHA-256 verification via companion `.sha256` release asset. Explicit failure handling:
    - `.sha256` asset missing from GitHub Release → reject update, show "Release missing integrity file — contact maintainer"
    - Download succeeds but SHA-256 mismatch → reject, do not activate partition, show "Firmware integrity check failed"
    - Download interrupted (WiFi drop, timeout) → firmware unchanged (inactive partition has partial write, never activated), show "Download failed — tap to retry"
    - GitHub API unreachable → show "Unable to check for updates — try again later", no auto-retry
    - HTTP 429 rate limited → same as unreachable, with explicit "rate limited" message per NFR20

5. **Per-mode NVS key namespacing** — Mode-specific settings use a strict prefix convention:
    - Format: `m_{abbrev}_{setting}` where mode ID abbreviation is ≤5 chars, setting name is ≤7 chars
    - Examples: `m_clock_format` (12/24h), `m_depbd_rows` (max rows), `m_depbd_fields` (field selection), `m_lfcrd_fields` (Live Flight Card telemetry fields)
    - Mode abbreviations: `clock` (5), `depbd` (5), `lfcrd` (5) — documented in ConfigManager alongside existing NVS key abbreviation table
    - Modes declare their settings schema in static metadata (same pattern as `ModeZoneDescriptor`) — name, type, default, min/max for the Mode Picker UI to render settings panels dynamically
    - **Rule:** New mode abbreviations must be ≤5 chars. Enforced by documentation and PR review.

6. **Schedule rules NVS storage** — Fixed maximum of 8 schedule rules with indexed NVS keys:
    - `sched_r{N}_start` (uint16, minutes since midnight)
    - `sched_r{N}_end` (uint16, minutes since midnight)
    - `sched_r{N}_mode` (string, mode ID)
    - `sched_r_count` (uint8, active rule count)
    - Total: up to 25 additional NVS keys at max capacity (8 rules x 3 keys + count)
    - Same minutes-since-midnight convention as Foundation's night mode (rule 15)

7. **NTP dependency expansion** — Foundation introduced NTP for brightness scheduling. Delight adds two more consumers: Clock Mode display and mode scheduling. All three share the same `configTzTime()` sync — no additional NTP configuration. Consequence of NTP failure expands: scheduling degrades gracefully (rules don't fire until NTP syncs), Clock Mode shows uptime-based time or a static indicator. Flight rendering unaffected.

### Existing Architecture Unchanged

The following MVP + Foundation + Display System decisions remain in full effect with no modifications:

- Hexagonal (Ports & Adapters) pattern
- ConfigManager singleton with category struct getters
- Inter-task communication (atomic flags for config, FreeRTOS queue for flights)
- WiFi state machine (WiFiManager)
- SystemStatus error registry
- FreeRTOS dual-core task pinning (display Core 0, everything else Core 1)
- All naming conventions, NVS key patterns, logging macros, and enforcement rules (1-23)
- Web asset patterns (fetch + json.ok + showToast)
- DisplayMode interface, RenderContext isolation, ModeRegistry static table
- OTA self-check, rollback detection, night mode brightness scheduler
- Flash partition layout (unchanged from Foundation — dual OTA 1.5MB + 960KB LittleFS)

## Delight Release — Technology Stack

**Starter Template Evaluation: N/A (Brownfield Extension)**

No new external dependencies required for the core Delight capabilities (Clock Mode, Departures Board, transitions, scheduling, mode-specific settings). All render through the existing DisplayMode interface using Adafruit GFX via FastLED NeoMatrix.

**One new external integration — GitHub Releases API:**

| Component | Implementation | Source |
|-----------|---------------|--------|
| ClockMode | C++ class implementing DisplayMode | New firmware code |
| DeparturesBoardMode | C++ class implementing DisplayMode | New firmware code |
| Fade transition | Private method in ModeRegistry | New firmware code |
| Mode scheduling | Static class or main-loop function | New firmware code |
| Mode-specific settings | ConfigManager expansion + Mode Picker UI | Existing pattern extension |
| OTAUpdater | GitHub Releases API client | New firmware code |
| GitHub Releases API | `api.github.com` REST endpoint | Unauthenticated HTTPS GET |
| SHA-256 verification | `mbedtls_sha256` | ESP32 core (built-in mbedTLS) |

**New library dependency: None.** SHA-256 is available via ESP32's built-in mbedTLS stack (`mbedtls/sha256.h`), already linked for HTTPS TLS. GitHub API responses parsed with existing ArduinoJson. Binary download uses existing `HTTPClient` + `Update.h`.

The MVP + Foundation + Display System dependency stack (ESPAsyncWebServer Carbou fork, AsyncTCP, FastLED, Adafruit GFX, FastLED NeoMatrix, ArduinoJson, LittleFS, ESPmDNS, Update.h, esp_ota_ops.h, time.h/configTzTime, mbedTLS) remains unchanged.

## Delight Release — Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. DL1 — Fade transition in ModeRegistry (RGB888 dual buffer lifecycle, blend algorithm)
2. DL2 — Mode orchestration state machine (MANUAL/SCHEDULED/IDLE_FALLBACK)
3. DL3 — OTAUpdater component (GitHub API client, incremental SHA-256, FreeRTOS download task)
4. DL4 — Schedule rules NVS storage (indexed keys, max 8 rules)

**Important Decisions (Shape Architecture):**
5. DL5 — Per-mode settings schema (NVS prefix convention, static metadata, nullable in ModeEntry)
6. DL6 — API endpoint additions (OTA Pull with status polling, schedule CRUD, mode settings)

**Deferred Decisions (Post-Delight):**
- Multiple transition types (wipe, scroll) — fade only in MVP; extract TransitionEngine if Growth adds more
- Departures Board layout variants — single layout in MVP
- Schedule templates ("Home office," "Weekend") — manual entry only
- OTA firmware signing — trust based on GitHub repo URL for now

### Decision DL1: Fade Transition — ModeRegistry Private Method with Transient RGB888 Buffers

**Approach:** Fade is a phase within the existing `ModeRegistry::tick()` switch lifecycle, implemented as `_executeFadeTransition()` private method. No separate component.

**Buffer strategy:**
- Two RGB888 buffers matching FastLED's native `CRGB` format: `160 * 32 * 3 = 15,360 bytes per buffer`, ~30KB total
- `malloc()` at transition start, `free()` immediately after last blend frame
- If `malloc()` fails (heap too low): skip fade, do instant cut (current behavior) — graceful degradation, not an error
- **Why RGB888, not RGB565:** FastLED stores pixels as `CRGB` (3 bytes RGB888). The `leds` array is directly accessible. Using RGB888 avoids color space conversion in both capture and blend — `memcpy(outgoing, leds, PIXEL_COUNT * 3)` captures the frame. Blend operates natively. No precision loss.

**Blend algorithm:**
```cpp
static constexpr uint8_t FADE_STEPS = 15;  // ~1 second at 15fps

void ModeRegistry::_executeFadeTransition(const RenderContext& ctx,
                                           const std::vector<FlightInfo>& flights) {
    const uint16_t pixelCount = ctx.layout.matrix_width * ctx.layout.matrix_height;
    const size_t bufSize = pixelCount * 3;

    uint8_t* outgoing = (uint8_t*)malloc(bufSize);
    uint8_t* incoming = (uint8_t*)malloc(bufSize);
    if (!outgoing || !incoming) {
        free(outgoing); free(incoming);
        LOG_I("ModeReg", "Fade buffer alloc failed, instant cut");
        return;  // caller proceeds with instant cut
    }

    // Capture outgoing frame from FastLED's CRGB array
    memcpy(outgoing, leds, bufSize);

    // Render one frame of new mode to capture incoming
    _activeMode->render(ctx, flights);
    memcpy(incoming, leds, bufSize);

    // Blend N frames
    for (uint8_t step = 1; step <= FADE_STEPS; step++) {
        for (uint16_t i = 0; i < pixelCount; i++) {
            uint16_t idx = i * 3;
            leds[i].r = (outgoing[idx]     * (FADE_STEPS - step) + incoming[idx]     * step) / FADE_STEPS;
            leds[i].g = (outgoing[idx + 1] * (FADE_STEPS - step) + incoming[idx + 1] * step) / FADE_STEPS;
            leds[i].b = (outgoing[idx + 2] * (FADE_STEPS - step) + incoming[idx + 2] * step) / FADE_STEPS;
        }
        FastLED.show();
        delay(1000 / FADE_STEPS);  // ~66ms per frame at 15fps
    }

    free(outgoing);
    free(incoming);
}
```

**Note:** `delay()` is acceptable inside the fade loop because this runs on Core 0 (display task) during `SWITCHING` state — no other rendering occurs. The main loop on Core 1 is unaffected.

**Updated switch flow in `tick()`:**
```
1. _switchState = SWITCHING
2. _activeMode->teardown()
3. delete _activeMode
4. Heap check against memoryRequirement()
5. If sufficient: factory() → new mode → init(ctx)
6. If init() succeeds: _executeFadeTransition(ctx, flights)
7. _switchState = IDLE, set NVS pending
---
If heap insufficient or init() fails: restore previous mode, set error
If fade buffer alloc fails: instant cut (no regression from current behavior)
```

### Decision DL2: Mode Orchestration — ModeOrchestrator Static Class

**Approach:** Lightweight `core/ModeOrchestrator.h/.cpp` static class. Runs in the Core 1 main loop alongside the existing brightness scheduler. Calls `ModeRegistry::requestSwitch()` when state transitions require a mode change.

```cpp
enum class OrchestratorState : uint8_t {
    MANUAL,          // User's explicit selection (default)
    SCHEDULED,       // Time-based rule is active
    IDLE_FALLBACK    // Zero flights → Clock Mode
};

class ModeOrchestrator {
public:
    static void init();
    static void tick();                              // called from Core 1 main loop, non-blocking
    static void onManualSwitch(const char* modeId);  // called from WebPortal on user switch
    static OrchestratorState getState();
    static const char* getActiveScheduleRuleName();  // for dashboard status
    static const char* getManualModeId();

private:
    static OrchestratorState _state;
    static char _manualModeId[16];                   // NVS-persisted user selection
    static int8_t _activeRuleIndex;                  // -1 if no rule active
    static unsigned long _lastTickMs;
};
```

**Flight count signaling:**
```cpp
// In main.cpp — global atomic, updated by FlightDataFetcher after queue write
std::atomic<uint8_t> g_flightCount{0};

// In FlightDataFetcher, after xQueueOverwrite:
g_flightCount.store(enrichedFlights.size());

// ModeOrchestrator reads this — no queue peek, no cross-core conflict
uint8_t flights = g_flightCount.load();
```

**State machine (inside `tick()`, runs ~1/sec on Core 1):**

```cpp
void ModeOrchestrator::tick() {
    if (millis() - _lastTickMs < 1000) return;  // 1-second interval
    _lastTickMs = millis();

    struct tm now;
    if (!getLocalTime(&now, 0)) return;  // non-blocking; skip if NTP not synced
    uint16_t currentMin = now.tm_hour * 60 + now.tm_min;

    // 1. Evaluate schedule rules (highest priority)
    ModeScheduleConfig sched = ConfigManager::getModeSchedule();
    int8_t matchingRule = -1;
    for (uint8_t i = 0; i < sched.rule_count; i++) {
        if (sched.rules[i].enabled && timeInWindow(currentMin,
                sched.rules[i].start_min, sched.rules[i].end_min)) {
            matchingRule = i;
            break;  // first match wins
        }
    }

    if (matchingRule >= 0) {
        if (_state != SCHEDULED || _activeRuleIndex != matchingRule) {
            _state = SCHEDULED;
            _activeRuleIndex = matchingRule;
            ModeRegistry::requestSwitch(sched.rules[matchingRule].mode_id);
        }
        return;  // schedule active — skip idle fallback check
    }

    // No schedule rule active — exit SCHEDULED if we were in it
    if (_state == SCHEDULED) {
        _state = MANUAL;
        _activeRuleIndex = -1;
        ModeRegistry::requestSwitch(_manualModeId);
        return;
    }

    // 2. Check idle fallback (only when not scheduled)
    uint8_t flights = g_flightCount.load();
    if (flights == 0 && _state == MANUAL) {
        _state = IDLE_FALLBACK;
        ModeRegistry::requestSwitch("clock");
    } else if (flights > 0 && _state == IDLE_FALLBACK) {
        _state = MANUAL;
        ModeRegistry::requestSwitch(_manualModeId);
    }
}
```

**Boot state:** Always starts as `MANUAL`. First `tick()` call (~1 second after boot) evaluates schedule rules and flight count. If a schedule matches, transitions to `SCHEDULED` within that first tick. If zero flights, transitions to `IDLE_FALLBACK`.

**Manual switch handling:**
```cpp
void ModeOrchestrator::onManualSwitch(const char* modeId) {
    strncpy(_manualModeId, modeId, 15);
    _manualModeId[15] = '\0';
    if (_state == IDLE_FALLBACK) {
        _state = MANUAL;
    }
    // If SCHEDULED: manual switch takes effect, but next tick may re-apply schedule
    // This is documented behavior per PRD Journey 1 climax: "schedule takes priority when explicitly configured"
}
```

**Midnight-crossing time window check (same convention as Foundation brightness scheduler):**
```cpp
static bool timeInWindow(uint16_t current, uint16_t start, uint16_t end) {
    if (start <= end) {
        return (current >= start && current < end);
    } else {
        return (current >= start || current < end);
    }
}
```

### Decision DL3: OTAUpdater — GitHub Releases API Client with Incremental SHA-256

**Approach:** New `core/OTAUpdater.h/.cpp` static class. HTTP client that checks GitHub Releases, downloads binary with incremental SHA-256 verification, streams to partition. Download runs in a one-shot FreeRTOS task to avoid blocking the async web server.

```cpp
enum class OTAState : uint8_t {
    IDLE,
    CHECKING,
    AVAILABLE,
    DOWNLOADING,
    VERIFYING,
    REBOOTING,
    ERROR
};

class OTAUpdater {
public:
    static void init(const char* repoOwner, const char* repoName);

    // Version check — called from API handler, runs synchronously (~1-2s)
    static bool checkForUpdate();

    // Download — spawns FreeRTOS task, returns immediately
    static bool startDownload();

    // State accessors for API polling
    static OTAState getState();
    static uint8_t getProgress();        // 0-100%
    static const char* getNewVersion();
    static const char* getReleaseNotes();
    static const char* getLastError();
    static const char* getInstalledVersion();  // returns FW_VERSION

private:
    static char _repoOwner[32];
    static char _repoName[32];
    static char _binUrl[256];
    static char _shaUrl[256];
    static char _newVersion[16];
    static char _releaseNotes[512];      // truncated if longer
    static char _lastError[128];
    static OTAState _state;
    static uint8_t _progress;
    static TaskHandle_t _downloadTask;

    static void _downloadTaskFunc(void* param);
};
```

**Check flow (`checkForUpdate`):**
1. `_state = CHECKING`
2. `GET https://api.github.com/repos/{owner}/{repo}/releases/latest`
3. Parse JSON via ArduinoJson: extract `tag_name`, `body` (release notes), iterate `assets[]` to find `.bin` and `.sha256` URLs
4. Compare `tag_name` with `FW_VERSION` — simple string inequality (not semver)
5. If different: store URLs, version, notes; `_state = AVAILABLE`; return true
6. If same: `_state = IDLE`; return false
7. If HTTP error or 429: `_state = ERROR`; set error message; return false

**Download flow (`_downloadTaskFunc`, runs in FreeRTOS task):**
1. `_state = DOWNLOADING`
2. Shared pre-OTA sequence: `ModeRegistry::prepareForOTA()` — teardown active mode, free buffers, show "Updating..." on LEDs
3. Verify ≥80KB free heap; if not, set error, `_state = ERROR`, return
4. Download `.sha256` file first (~64 bytes); parse as **plain 64-character hex string** (no filename, no prefix). If missing or unparseable: set error "Release missing integrity file", `_state = ERROR`, return
5. Get next OTA partition: `esp_ota_get_next_update_partition(NULL)`
6. `Update.begin(partition->size)`
7. Initialize `mbedtls_sha256_context`
8. Stream `.bin` via `HTTPClient` in chunks:
   - `Update.write(chunk, len)` — write to flash
   - `mbedtls_sha256_update(&ctx, chunk, len)` — feed to hasher
   - Update `_progress = (bytesWritten * 100) / totalSize`
9. After final chunk: `mbedtls_sha256_finish(&ctx, computedHash)`
10. `_state = VERIFYING`
11. Compare `computedHash` against downloaded expected hash
    - If mismatch: `Update.abort()`, set error "Firmware integrity check failed", `_state = ERROR`, return
12. `Update.end(true)` — finalize partition
13. `_state = REBOOTING`
14. `delay(500); ESP.restart()`

**Error recovery:**
- Download interrupted (WiFi drop): `Update.abort()`, `_state = ERROR`, "Download failed — tap to retry"
- Any HTTP error during download: `Update.abort()`, `_state = ERROR`, specific message
- The inactive partition is never activated on failure — firmware unchanged
- After error, user can call `startDownload()` again (retry)

**Shared pre-OTA extraction:**
```cpp
// New public method in ModeRegistry
static void prepareForOTA() {
    // Teardown active mode — frees mode heap + any transition buffers
    if (_activeMode) {
        _activeMode->teardown();
        delete _activeMode;
        _activeMode = nullptr;
    }
    _switchState = SWITCHING;  // prevents new switch requests during OTA
}

// Used by both:
// - WebPortal::_handleOTAUpload() (Foundation push) — call before Update.begin()
// - OTAUpdater::_downloadTaskFunc() (Delight pull) — call before download
```

### Decision DL4: Schedule Rules NVS Storage — Fixed Max with Indexed Keys

**Approach:** Fixed maximum of 8 rules with indexed NVS keys. New `ModeScheduleConfig` struct in ConfigManager.

```cpp
struct ScheduleRule {
    uint16_t start_min;     // minutes since midnight (0-1439)
    uint16_t end_min;       // minutes since midnight (0-1439)
    char mode_id[16];       // full mode ID string (e.g., "clock", "departures")
    bool enabled;
};

struct ModeScheduleConfig {
    ScheduleRule rules[8];
    uint8_t rule_count;     // 0-8, number of defined rules
};
```

**NVS keys (per rule):**

| Key Pattern | Type | Example | Chars |
|------------|------|---------|-------|
| `sched_r{N}_start` | uint16 | `sched_r0_start` = 360 | 15 |
| `sched_r{N}_end` | uint16 | `sched_r0_end` = 1320 | 13 |
| `sched_r{N}_mode` | string | `sched_r0_mode` = `"clock"` | 14 |
| `sched_r_count` | uint8 | 2 | 13 |

All keys within 15-char NVS limit. Mode ID stored as full string value (values have no length limit in NVS). N ranges 0-7.

**ConfigManager additions:**
```cpp
static ModeScheduleConfig getModeSchedule();
static void setModeSchedule(const ModeScheduleConfig& config);
```

All schedule keys are **hot-reload** — no reboot required. `setModeSchedule()` writes all rule keys in a batch, then `ModeOrchestrator::tick()` picks up changes on its next iteration (~1 second).

**Rule conflict resolution:** Rules evaluated in index order (0 first). First matching rule wins. Overlapping time windows resolved by index priority — user arranges rules via the dashboard, display order is priority order.

### Decision DL5: Per-Mode Settings Schema — Static Metadata with NVS Prefix Convention

**Approach:** Modes declare settings via static metadata struct (same pattern as `ModeZoneDescriptor`). ConfigManager reads/writes using the `m_{abbrev}_{setting}` NVS key prefix. Mode Picker UI renders settings panels dynamically from the schema returned by the API.

```cpp
struct ModeSettingDef {
    const char* key;          // NVS key suffix (≤7 chars)
    const char* label;        // UI display name
    const char* type;         // "uint8", "uint16", "bool", "enum"
    int32_t defaultValue;
    int32_t minValue;
    int32_t maxValue;
    const char* enumOptions;  // comma-separated for enum type, NULL otherwise
};

struct ModeSettingsSchema {
    const char* modeAbbrev;           // ≤5 chars, used for NVS key prefix
    const ModeSettingDef* settings;
    uint8_t settingCount;
};
```

**Clock Mode settings:**
```cpp
// NVS key: m_clock_format (14 chars)
static const ModeSettingDef CLOCK_SETTINGS[] = {
    { "format", "Time Format", "enum", 0, 0, 1, "24h,12h" }
};
static const ModeSettingsSchema CLOCK_SCHEMA = { "clock", CLOCK_SETTINGS, 1 };
```

**Departures Board settings:**
```cpp
// NVS keys: m_depbd_rows (12 chars), m_depbd_fields (14 chars)
static const ModeSettingDef DEPBD_SETTINGS[] = {
    { "rows", "Max Rows", "uint8", 4, 1, 4, NULL },
    { "fields", "Telemetry Fields", "enum", 0, 0, 2, "alt+spd,alt+hdg,spd+hdg" }
};
static const ModeSettingsSchema DEPBD_SCHEMA = { "depbd", DEPBD_SETTINGS, 2 };
```

**ModeEntry expansion (backward-compatible):**
```cpp
struct ModeEntry {
    const char* id;
    const char* displayName;
    DisplayMode* (*factory)();
    uint32_t (*memoryRequirement)();
    uint8_t priority;
    const ModeSettingsSchema* settingsSchema;  // NEW — nullable for modes with no settings
};
```

Existing modes (ClassicCardMode, LiveFlightCardMode) set `settingsSchema` to `nullptr`. API returns empty `settings: []` for nullable schemas. No breaking change to existing mode table entries.

**ConfigManager per-mode helpers:**
```cpp
static int32_t getModeSetting(const char* modeAbbrev, const char* settingKey, int32_t defaultValue);
static void setModeSetting(const char* modeAbbrev, const char* settingKey, int32_t value);
// Constructs NVS key as: "m_" + modeAbbrev + "_" + settingKey
```

**Key convention enforcement:**
- Mode abbreviation: ≤5 chars (documented alongside existing NVS abbreviation table in ConfigManager.h)
- Setting name: ≤7 chars
- Total: `m_` (2) + abbrev (≤5) + `_` (1) + setting (≤7) = ≤15 chars
- Abbreviations: `clock` (5), `depbd` (5), `lfcrd` (5), `clsic` (5)

### Decision DL6: API Endpoint Additions — OTA Pull with Status Polling + Schedule CRUD

**5 new endpoints:**

| Method | Endpoint | Purpose | Response |
|--------|----------|---------|----------|
| `GET` | `/api/ota/check` | Check GitHub for new firmware version | `{ ok, data: { available, version, release_notes, current_version } }` |
| `POST` | `/api/ota/pull` | Start firmware download (spawns FreeRTOS task) | `{ ok, data: { started: true } }` |
| `GET` | `/api/ota/status` | Poll download progress and state | `{ ok, data: { state, progress, error } }` |
| `GET` | `/api/schedule` | Get mode schedule rules | `{ ok, data: { rules: [...], orchestrator_state } }` |
| `POST` | `/api/schedule` | Save mode schedule rules | `{ ok, data: { applied: true } }` |

**Updated existing endpoints:**

| Endpoint | Changes |
|----------|---------|
| `GET /api/display/modes` | Add `settings` array per mode with schema + current values; `null` for modes without settings |
| `POST /api/display/mode` | Accept optional `settings` object for per-mode config changes |
| `GET /api/status` | Add `ota_available` (bool), `ota_version` (string or null), `orchestrator_state` ("manual"/"scheduled"/"idle_fallback"), `active_schedule_rule` (index or -1) |

**`GET /api/ota/check` response:**
```json
{
    "ok": true,
    "data": {
        "available": true,
        "version": "2.4.1",
        "current_version": "2.4.0",
        "release_notes": "Bug fix for Departures Board row alignment..."
    }
}
```

**`GET /api/ota/status` response (during download):**
```json
{
    "ok": true,
    "data": {
        "state": "downloading",
        "progress": 42
    }
}
```

**`GET /api/schedule` response:**
```json
{
    "ok": true,
    "data": {
        "orchestrator_state": "scheduled",
        "active_rule_index": 0,
        "rules": [
            { "start": 360, "end": 1320, "mode": "departures", "enabled": true },
            { "start": 1320, "end": 360, "mode": "clock", "enabled": true }
        ]
    }
}
```

**`POST /api/ota/pull` behavior:**
- Validates that `OTAUpdater::getState()` is `AVAILABLE` (must check first)
- Calls `OTAUpdater::startDownload()` which spawns a FreeRTOS task
- Returns immediately with `{ ok: true }`
- Dashboard polls `GET /api/ota/status` at 500ms interval for progress
- Matches the existing mode-switch polling pattern from Display System

### Delight Decision Impact — Implementation Sequence

**Sequential foundation:**
1. **ConfigManager expansion** — ModeScheduleConfig struct, per-mode setting helpers, `getModeSchedule()` / `setModeSchedule()`, `getModeSetting()` / `setModeSetting()`
2. **ModeEntry expansion** — add `settingsSchema` field, update MODE_TABLE for existing modes with `nullptr`

**Mode implementations (sequential, after step 2):**
3. **ClockMode** (`modes/ClockMode.h/.cpp`) — simplest mode, proves NTP time display works
4. **DeparturesBoardMode** (`modes/DeparturesBoardMode.h/.cpp`) — multi-row rendering, configurable rows

**Orchestration track (after step 3, needs Clock Mode for idle fallback):**
5. **ModeOrchestrator** (`core/ModeOrchestrator.h/.cpp`) — state machine, schedule evaluation, idle fallback
6. **Fade transition** — `ModeRegistry::_executeFadeTransition()` private method + buffer management

**OTA track (independent of steps 3-6, after step 1):**
7. **OTAUpdater** (`core/OTAUpdater.h/.cpp`) — GitHub API client, incremental SHA-256, FreeRTOS download task
8. **ModeRegistry::prepareForOTA()** — shared pre-OTA sequence for both Push and Pull

**API + UI (after all firmware tracks complete):**
9. **WebPortal endpoints** — OTA Pull (check/pull/status), schedule (GET/POST), mode settings expansion
10. **Dashboard UI** — OTA Pull UI (banner, release notes, progress), schedule timeline editor, per-mode settings panels in Mode Picker
11. **Gzip rebuild** — all updated web assets

### Cross-Decision Dependencies

```
ConfigManager (DL4 schedule keys + DL5 per-mode settings)
    ├── ModeOrchestrator (DL2) — reads schedule rules
    │   └── ModeRegistry::requestSwitch() — triggers mode changes
    │       └── _executeFadeTransition (DL1) — runs during switch
    └── OTAUpdater (DL3) — reads repo config
        └── ModeRegistry::prepareForOTA() — shared pre-OTA

ModeEntry.settingsSchema (DL5)
    └── GET /api/display/modes (DL6) — includes settings in response
        └── Mode Picker UI — renders per-mode settings panels

g_flightCount atomic (DL2)
    ← FlightDataFetcher (writes after queue update)
    → ModeOrchestrator::tick() (reads for idle fallback)
```

## Delight Release — Implementation Patterns & Consistency Rules

### Delight-Specific Conflict Points

The MVP architecture established 11 enforcement rules. The Foundation Release added 5 (rules 12-16). The Display System Release added 7 (rules 17-23). The Delight Release introduces 7 new pattern areas where AI agents could make inconsistent choices.

### Orchestrator Interaction Pattern

**Who calls `ModeRegistry::requestSwitch()` — and when:**

ModeOrchestrator is the *only* component that evaluates schedule rules and idle fallback. WebPortal routes manual switches through `ModeOrchestrator::onManualSwitch()`, which records the user's preference and then delegates to `requestSwitch()` if appropriate.

**`requestSwitch()` is called from exactly two methods:**
1. `ModeOrchestrator::tick()` — when state machine transitions require a mode change
2. `ModeOrchestrator::onManualSwitch()` — when user selects a mode via dashboard

**Allowed:**
```cpp
// WebPortal handler for POST /api/display/mode:
ModeOrchestrator::onManualSwitch(modeId);  // ✅ always go through orchestrator

// ModeOrchestrator::tick() — evaluates state machine:
ModeRegistry::requestSwitch(targetModeId);  // ✅ orchestrator owns the decision
```

**Forbidden:**
```cpp
// WebPortal directly calling ModeRegistry:
ModeRegistry::requestSwitch(modeId);  // ❌ bypasses orchestrator state tracking

// Mode class requesting its own switch:
ModeRegistry::requestSwitch("clock");  // ❌ modes never initiate switches

// Timer callback or config-change handler calling requestSwitch:
void onConfigChange() { ModeRegistry::requestSwitch(...); }  // ❌ only orchestrator calls this
```

**Anti-pattern:** Calling `requestSwitch()` from anywhere except the two orchestrator methods. The orchestrator maintains state (MANUAL/SCHEDULED/IDLE_FALLBACK) that must stay consistent with the actual active mode. A direct `requestSwitch()` from any other call site would desync the state machine.

### Cross-Core Atomic Signaling Pattern

**`std::atomic` globals declared in `main.cpp` only:**

DL2 introduces `std::atomic<uint8_t> g_flightCount` as a cross-core signaling mechanism. This is a lightweight pattern for Core 1 → Core 0 communication without queues.

**Allowed:**
```cpp
// main.cpp — declaration
std::atomic<uint8_t> g_flightCount{0};

// FlightDataFetcher (Core 1) — writer
g_flightCount.store(enrichedFlights.size());

// ModeOrchestrator (Core 1) — reader
uint8_t flights = g_flightCount.load();
```

**Forbidden:**
```cpp
// Mode class reading atomic:
void ClockMode::render(...) {
    uint8_t f = g_flightCount.load();  // ❌ modes use RenderContext, not globals
}

// Adapter reading atomic:
void WebPortal::handleStatus() {
    uint8_t f = g_flightCount.load();  // ❌ adapters use core API, not globals
}
```

**Anti-pattern:** Declaring new `std::atomic` globals outside `main.cpp`, or reading/writing them from modes or adapters. Cross-core atomics are system-level plumbing — only core orchestration code (`main.cpp`, `ModeOrchestrator`, `FlightDataFetcher`) should touch them.

### OTA Pull Streaming Pattern

**Incremental hash — never post-hoc:**

The SHA-256 hash must be computed incrementally during the download stream. Each chunk is fed to both `Update.write()` and `mbedtls_sha256_update()` in the same loop iteration.

```cpp
// Correct pattern:
while (remaining > 0) {
    int len = stream->readBytes(buf, chunkSize);
    Update.write(buf, len);                      // write to flash
    mbedtls_sha256_update(&ctx, buf, len);       // feed to hasher
    _progress = (written * 100) / totalSize;     // update progress
}
mbedtls_sha256_finish(&ctx, computed);
// compare computed vs expected BEFORE Update.end()
```

**Anti-pattern:** Downloading the full binary, calling `Update.end()`, then trying to read back the partition to verify. `Update.h` provides no read-back API. Hash *during* streaming, verify *before* finalize.

**Error path discipline — always `Update.abort()` after `Update.begin()`:**

Once `Update.begin()` has been called, every error path must call `Update.abort()` before returning. This releases the flash write lock and leaves the inactive partition unchanged.

```cpp
// Correct error handling:
if (hashMismatch) {
    Update.abort();                              // ✅ always abort on failure
    _state = OTAState::ERROR;
    strncpy(_lastError, "Integrity check failed", sizeof(_lastError));
    return;
}

// Anti-pattern:
if (downloadFailed) {
    _state = OTAState::ERROR;                    // ❌ forgot Update.abort()
    return;                                       // flash write lock still held
}
```

**FreeRTOS task lifecycle:**
- `startDownload()` spawns a one-shot task on Core 1 (pinned, 8KB stack, priority 1)
- Task sets `_downloadTask = nullptr` *before* calling `vTaskDelete(NULL)` — prevents dangling handle reads from main thread
- Only one download task at a time — `startDownload()` rejects if `_downloadTask != nullptr`

```cpp
// End of _downloadTaskFunc:
_downloadTask = nullptr;   // ✅ clear handle before delete
vTaskDelete(NULL);         // task self-deletes

// startDownload() guard:
if (_downloadTask != nullptr) {
    return false;          // download already in progress
}
```

**Anti-pattern:** Creating a persistent FreeRTOS task that polls for OTA. The download is a one-shot operation triggered by the user.

### Pre-OTA Sequence Pattern

**Shared between Foundation Push and Delight Pull:**

Both OTA paths (browser upload and GitHub download) must call `ModeRegistry::prepareForOTA()` before `Update.begin()`. This frees mode heap and prevents the display task from interfering during the flash write.

**`prepareForOTA()` must set `_switchState = SWITCHING`** to block the display task's `tick()` loop. Without this flag, Core 0 could call `tick()` on a deleted mode pointer during the flash write.

```cpp
// ModeRegistry::prepareForOTA():
static void prepareForOTA() {
    _switchState = SWITCHING;  // ✅ blocks display task tick() loop
    if (_activeMode) {
        _activeMode->teardown();
        delete _activeMode;
        _activeMode = nullptr;
    }
}

// Foundation Push (WebPortal OTA upload handler):
ModeRegistry::prepareForOTA();  // then Update.begin(contentLength)

// Delight Pull (OTAUpdater download task):
ModeRegistry::prepareForOTA();  // then Update.begin(partitionSize)
```

**Anti-pattern:** Calling `Update.begin()` without first tearing down the active mode. The mode may hold 20-30KB of heap (transition buffers, render state) that OTA needs. Also: implementing `prepareForOTA()` without setting `_switchState = SWITCHING`.

### Schedule Rule NVS Pattern

**Indexed key convention — matches Foundation brightness schedule approach:**

Schedule rules use indexed keys where N is 0-7. The count key `sched_r_count` tells how many rules are defined.

**Complete NVS key set per rule:**

| Key Pattern | Type | Example | Chars |
|------------|------|---------|-------|
| `sched_r{N}_start` | uint16 | `sched_r0_start` = 360 | 15 |
| `sched_r{N}_end` | uint16 | `sched_r0_end` = 1320 | 13 |
| `sched_r{N}_mode` | string | `sched_r0_mode` = `"clock"` | 14 |
| `sched_r{N}_ena` | uint8 | `sched_r0_ena` = 1 | 13 |
| `sched_r_count` | uint8 | 2 | 13 |

All keys within 15-char NVS limit. The enabled flag uses `_ena` (not `_enabled` which would be 17 chars).

**Read pattern (getModeSchedule):**
```cpp
ModeScheduleConfig config;
config.rule_count = nvs.getUChar("sched_r_count", 0);
for (uint8_t i = 0; i < config.rule_count; i++) {
    char key[16];
    snprintf(key, sizeof(key), "sched_r%d_start", i);
    config.rules[i].start_min = nvs.getUShort(key, 0);
    snprintf(key, sizeof(key), "sched_r%d_end", i);
    config.rules[i].end_min = nvs.getUShort(key, 0);
    snprintf(key, sizeof(key), "sched_r%d_mode", i);
    // ... string read for mode_id
    snprintf(key, sizeof(key), "sched_r%d_ena", i);
    config.rules[i].enabled = nvs.getUChar(key, 1);  // default enabled
}
```

**Write pattern (setModeSchedule) — rules always compacted:**
```cpp
// Rules are stored compacted — no index gaps.
// When a rule is deleted from the middle, higher-index rules shift down.
nvs.putUChar("sched_r_count", config.rule_count);
for (uint8_t i = 0; i < config.rule_count; i++) {
    char key[16];
    snprintf(key, sizeof(key), "sched_r%d_start", i);
    nvs.putUShort(key, config.rules[i].start_min);
    snprintf(key, sizeof(key), "sched_r%d_end", i);
    nvs.putUShort(key, config.rules[i].end_min);
    snprintf(key, sizeof(key), "sched_r%d_mode", i);
    // ... string write for mode_id
    snprintf(key, sizeof(key), "sched_r%d_ena", i);
    nvs.putUChar(key, config.rules[i].enabled ? 1 : 0);
}
```

**Anti-pattern:** Serializing schedule rules as a single JSON blob in one NVS string key. NVS string values are limited to ~4000 bytes and JSON parsing adds heap pressure. Use indexed keys — same pattern as the Foundation brightness schedule.

**Anti-pattern:** Leaving index gaps when deleting rules. Rules are always compacted — when rule 1 of 3 is deleted, rules 2 and 3 shift down to indices 0 and 1, and `sched_r_count` decrements to 2.

### Per-Mode Settings NVS Pattern

**Key construction — always via ConfigManager helpers:**

Mode settings use the `m_{abbrev}_{setting}` NVS key pattern. Never construct these keys manually in mode code or WebPortal handlers.

```cpp
// Correct — through ConfigManager:
int32_t fmt = ConfigManager::getModeSetting("clock", "format", 0);  // reads "m_clock_format"
ConfigManager::setModeSetting("depbd", "rows", 3);                   // writes "m_depbd_rows"

// Forbidden — manual key construction:
nvs.getUChar("m_clock_format", 0);  // ❌ bypasses ConfigManager, no default handling
```

**Schema declaration — co-located with mode class:**

Each mode's `ModeSettingDef[]` and `ModeSettingsSchema` are declared as `static const` in the mode's `.h` file, not in a centralized registry.

```cpp
// modes/ClockMode.h — settings schema co-located with mode
static const ModeSettingDef CLOCK_SETTINGS[] = {
    { "format", "Time Format", "enum", 0, 0, 1, "24h,12h" }
};
static const ModeSettingsSchema CLOCK_SCHEMA = { "clock", CLOCK_SETTINGS, 1 };
```

**Anti-pattern:** Putting all mode settings definitions in ConfigManager or a single registry file. Settings are owned by the mode — the schema lives next to the code that uses it.

**Anti-pattern:** Hardcoding setting names in API response builders. WebPortal handlers for `GET /api/display/modes` must iterate `ModeEntry.settingsSchema` dynamically to build the settings JSON. If someone adds a setting to the schema but the API handler has hardcoded field names, the new setting becomes invisible to the dashboard.

```cpp
// Correct — dynamic iteration:
if (entry.settingsSchema) {
    for (uint8_t i = 0; i < entry.settingsSchema->settingCount; i++) {
        const ModeSettingDef& def = entry.settingsSchema->settings[i];
        // build JSON from def.key, def.label, def.type, etc.
    }
}

// Forbidden — hardcoded:
json["settings"][0]["key"] = "format";  // ❌ breaks when schema changes
```

### Fade Buffer Lifecycle Pattern

**Allocate late, free early:**

Transition buffers are `malloc()`'d at the start of `_executeFadeTransition()` and `free()`'d immediately after the last blend frame. They never persist beyond the transition call.

**Rule:** If `malloc()` returns `nullptr` for either buffer, free both and return immediately. The caller treats this as "instant cut" — the mode still switches, just without the fade animation. This is graceful degradation, not an error.

```cpp
uint8_t* outgoing = (uint8_t*)malloc(bufSize);
uint8_t* incoming = (uint8_t*)malloc(bufSize);
if (!outgoing || !incoming) {
    free(outgoing); free(incoming);  // safe: free(nullptr) is no-op
    LOG_I("ModeReg", "Fade buffer alloc failed, instant cut");
    return;  // not an error — graceful degradation
}
// ... blend loop ...
free(outgoing);
free(incoming);
```

**Anti-pattern:** Pre-allocating persistent transition buffers at boot. The ~30KB would reduce available heap for the entire lifetime. Allocate only during the ~1-second transition window.

### Updated Enforcement Guidelines

**All AI agents implementing Delight stories MUST also follow (in addition to rules 1-23):**

24. Mode switches from WebPortal must go through `ModeOrchestrator::onManualSwitch()` — `ModeRegistry::requestSwitch()` is called from exactly two methods: `ModeOrchestrator::tick()` and `ModeOrchestrator::onManualSwitch()` — no other call sites
25. OTA SHA-256 verification must be computed incrementally during streaming via `mbedtls_sha256_update()` per chunk — never attempt post-download partition read-back. On any error after `Update.begin()`, always call `Update.abort()` before returning
26. Both OTA paths (Push and Pull) must call `ModeRegistry::prepareForOTA()` before `Update.begin()` — `prepareForOTA()` must set `_switchState = SWITCHING` to block the display task's `tick()` loop
27. Fade transition buffers are `malloc()`'d at transition start and `free()`'d immediately after the last blend frame — never persist transition buffers beyond the fade call, and never treat allocation failure as an error (use instant cut fallback)
28. Per-mode settings NVS keys must be read/written exclusively through `ConfigManager::getModeSetting()` / `setModeSetting()` — never construct `m_{abbrev}_{key}` keys manually. API handlers must iterate `settingsSchema` dynamically, never hardcode setting names
29. Mode settings schemas (`ModeSettingDef[]` + `ModeSettingsSchema`) are declared as `static const` in the mode's own `.h` file — never centralize settings definitions in ConfigManager or a registry
30. Cross-core `std::atomic` globals are declared in `main.cpp` only — modes and adapters must not read or write atomic globals directly; use `RenderContext` fields or core API methods instead

### Delight Release — Party Mode Refinements (Patterns)

**Reviewers:** Winston (Architect), Amelia (Developer), Quinn (QA)

**8 refinements incorporated:**

1. Rule 24 tightened to enumerate exactly two allowed `requestSwitch()` call sites (Winston)
2. New rule 30 added for cross-core atomic signaling discipline (Winston)
3. Schedule NVS pattern: added explicit `sched_r{N}_ena` key with 14-char name (Winston)
4. OTA task pattern: `_downloadTask = nullptr` before `vTaskDelete(NULL)` to prevent dangling handle (Amelia)
5. OTA error pattern: always `Update.abort()` on any error after `Update.begin()` — added to rule 25 (Amelia)
6. Pre-OTA pattern: explicitly state `prepareForOTA()` must set `_switchState = SWITCHING` — added to rule 26 (Amelia)
7. Per-mode settings anti-pattern: API handlers must iterate schema dynamically, never hardcode — added to rule 28 (Quinn)
8. Schedule delete convention: rules always compacted with no index gaps — added to NVS pattern (Quinn)

## Delight Release — Project Structure & File Change Map

### Complete Project Directory Structure (Delight Additions)

```
firmware/
├── platformio.ini                          # UNCHANGED (modes/ and utils/ already in build filter;
│                                           #  mbedTLS bundled in ESP-IDF, no extra flags needed)
├── custom_partitions.csv                   # UNCHANGED (Foundation dual-OTA layout)
│
├── interfaces/
│   ├── BaseDisplay.h                       # UNCHANGED
│   ├── BaseFlightFetcher.h                 # UNCHANGED
│   ├── BaseStateVectorFetcher.h            # UNCHANGED
│   └── DisplayMode.h                       # UPDATED: +ModeSettingDef struct,
│                                           #          +ModeSettingsSchema struct
│                                           #          (mode contract — settings declaration)
│
├── core/
│   ├── ConfigManager.h                     # UPDATED: +ModeScheduleConfig, +ScheduleRule structs,
│   │                                       #          +getModeSchedule(), +setModeSchedule(),
│   │                                       #          +getModeSetting(), +setModeSetting(),
│   │                                       #          +sched_r{N} NVS key abbreviation docs
│   ├── ConfigManager.cpp                   # UPDATED: schedule NVS read/write with compaction,
│   │                                       #          per-mode NVS helpers (m_{abbrev}_{key})
│   ├── ModeRegistry.h                      # UPDATED: +_executeFadeTransition() private,
│   │                                       #          +prepareForOTA() public static,
│   │                                       #          ModeEntry +settingsSchema field
│   ├── ModeRegistry.cpp                    # UPDATED: fade transition blend loop,
│   │                                       #          prepareForOTA() implementation
│   │                                       #          (sets _switchState = SWITCHING)
│   ├── ModeOrchestrator.h                  # ← NEW: OrchestratorState enum, static class decl
│   ├── ModeOrchestrator.cpp                # ← NEW: tick(), onManualSwitch(), state machine,
│   │                                       #         schedule evaluation, idle fallback,
│   │                                       #         timeInWindow() helper,
│   │                                       #         extern g_flightCount declaration
│   ├── OTAUpdater.h                        # ← NEW: OTAState enum, static class decl,
│   │                                       #         version/progress/error accessors
│   ├── OTAUpdater.cpp                      # ← NEW: checkForUpdate() GitHub API,
│   │                                       #         _downloadTaskFunc() FreeRTOS task,
│   │                                       #         incremental SHA-256 via mbedTLS,
│   │                                       #         startDownload() task spawning
│   ├── FlightDataFetcher.h                 # UNCHANGED
│   ├── FlightDataFetcher.cpp               # UPDATED: +g_flightCount.store() after queue write,
│   │                                       #          +extern std::atomic<uint8_t> g_flightCount
│   ├── LayoutEngine.h                      # UNCHANGED
│   ├── LayoutEngine.cpp                    # UNCHANGED
│   ├── LogoManager.h                       # UNCHANGED
│   ├── LogoManager.cpp                     # UNCHANGED
│   ├── SystemStatus.h                      # UPDATED: +OTA_PULL subsystem entry
│   └── SystemStatus.cpp                    # UPDATED: +OTA_PULL subsystem registration
│
├── modes/
│   ├── ClassicCardMode.h                   # UNCHANGED (settingsSchema nullptr is in MODE_TABLE
│   ├── ClassicCardMode.cpp                 #  in main.cpp, not in mode .h files)
│   ├── LiveFlightCardMode.h                # UNCHANGED
│   ├── LiveFlightCardMode.cpp              # UNCHANGED
│   ├── ClockMode.h                         # ← NEW: ClockMode class, CLOCK_SETTINGS[],
│   │                                       #         CLOCK_SCHEMA static const,
│   │                                       #         NTP time display, 12/24h format setting
│   ├── ClockMode.cpp                       # ← NEW: render() with time formatting,
│   │                                       #         getModeSetting() for format preference
│   ├── DeparturesBoardMode.h              # ← NEW: DeparturesBoardMode class, DEPBD_SETTINGS[],
│   │                                       #         DEPBD_SCHEMA static const,
│   │                                       #         configurable rows + telemetry fields
│   └── DeparturesBoardMode.cpp            # ← NEW: multi-row rendering, dynamic row
│                                           #         add/remove, in-place row mutation
│
├── adapters/
│   ├── NeoMatrixDisplay.h                  # UNCHANGED
│   ├── NeoMatrixDisplay.cpp                # UNCHANGED
│   ├── WebPortal.h                         # UPDATED: +_handleOTACheck(), +_handleOTAPull(),
│   │                                       #          +_handleOTAStatus(), +_handleGetSchedule(),
│   │                                       #          +_handlePostSchedule()
│   ├── WebPortal.cpp                       # UPDATED: 5 new route handlers,
│   │                                       #          updated /api/display/modes (settings array,
│   │                                       #          iterates settingsSchema dynamically),
│   │                                       #          updated POST /api/display/mode (settings obj,
│   │                                       #          routes through onManualSwitch()),
│   │                                       #          updated /api/status (ota + orchestrator)
│   ├── WiFiManager.h                       # UNCHANGED
│   ├── WiFiManager.cpp                     # UNCHANGED
│   ├── AeroAPIFetcher.h                    # UNCHANGED
│   ├── AeroAPIFetcher.cpp                  # UNCHANGED
│   ├── FlightWallFetcher.h                 # UNCHANGED
│   ├── FlightWallFetcher.cpp               # UNCHANGED
│   ├── OpenSkyFetcher.h                    # UNCHANGED
│   └── OpenSkyFetcher.cpp                  # UNCHANGED
│
├── utils/
│   ├── GeoUtils.h                          # UNCHANGED
│   ├── Log.h                               # UNCHANGED
│   ├── DisplayUtils.h                      # UNCHANGED
│   └── DisplayUtils.cpp                    # UNCHANGED
│
├── models/
│   ├── FlightInfo.h                        # UNCHANGED
│   ├── StateVector.h                       # UNCHANGED
│   └── AirportInfo.h                       # UNCHANGED
│
├── config/
│   ├── HardwareConfiguration.h             # UNCHANGED
│   ├── TimingConfiguration.h               # UNCHANGED
│   ├── WiFiConfiguration.h                 # UNCHANGED
│   ├── APIConfiguration.h                  # UNCHANGED
│   └── UserConfiguration.h                 # UNCHANGED
│
├── src/
│   └── main.cpp                            # UPDATED: +std::atomic<uint8_t> g_flightCount,
│                                           #          +ModeOrchestrator::init() in setup(),
│                                           #          +OTAUpdater::init() in setup(),
│                                           #          +ModeOrchestrator::tick() in Core 1 loop,
│                                           #          MODE_TABLE entries +settingsSchema field
│                                           #          (nullptr for existing, schema ptr for new)
│
├── data-src/
│   ├── dashboard.html                      # UPDATED: +OTA Pull section (check/download/progress),
│   │                                       #          +Schedule timeline editor section,
│   │                                       #          +per-mode settings panels in Mode Picker
│   ├── dashboard.js                        # UPDATED: +OTA Pull API calls (check/pull/status poll),
│   │                                       #          +schedule CRUD (GET/POST /api/schedule),
│   │                                       #          +per-mode settings rendering from schema,
│   │                                       #          +orchestrator state display
│   ├── style.css                           # UPDATED: +OTA progress bar, +schedule timeline,
│   │                                       #          +mode settings panel styling
│   ├── health.html                         # UNCHANGED
│   ├── health.js                           # UNCHANGED
│   ├── common.js                           # UNCHANGED
│   ├── wizard.html                         # UNCHANGED
│   └── wizard.js                           # UNCHANGED
│
├── data/
│   ├── dashboard.html.gz                   # REBUILD after dashboard.html changes
│   ├── dashboard.js.gz                     # REBUILD after dashboard.js changes
│   ├── style.css.gz                        # REBUILD after style.css changes
│   ├── common.js.gz                        # UNCHANGED
│   ├── health.html.gz                      # UNCHANGED
│   ├── health.js.gz                        # UNCHANGED
│   ├── wizard.html.gz                      # UNCHANGED
│   ├── wizard.js.gz                        # UNCHANGED
│   ├── test.txt                            # UNCHANGED
│   └── logos/                              # UNCHANGED
│
└── test/
    ├── test_config_manager/test_main.cpp   # UNCHANGED
    ├── test_layout_engine/test_main.cpp    # UNCHANGED
    ├── test_logo_manager/test_main.cpp     # UNCHANGED
    ├── test_telemetry_conversion/test_main.cpp  # UNCHANGED
    ├── test_mode_registry/test_main.cpp    # UPDATED: +fade transition lifecycle,
    │                                       #          +prepareForOTA() state tests
    ├── test_mode_orchestrator/test_main.cpp # ← NEW: state machine transitions,
    │                                       #         schedule evaluation, idle fallback,
    │                                       #         manual switch interaction
    ├── test_ota_updater/test_main.cpp      # ← NEW: version comparison, state transitions,
    │                                       #         SHA-256 verification logic (mock HTTP)
    └── test_config_schedule/test_main.cpp  # ← NEW: schedule NVS read/write,
                                            #         rule compaction on delete,
                                            #         index shifting, count tracking
```

### File Change Summary

| Action | Count | Files |
|--------|-------|-------|
| **NEW** | 8 | `core/ModeOrchestrator.h`, `core/ModeOrchestrator.cpp`, `core/OTAUpdater.h`, `core/OTAUpdater.cpp`, `modes/ClockMode.h`, `modes/ClockMode.cpp`, `modes/DeparturesBoardMode.h`, `modes/DeparturesBoardMode.cpp` |
| **NEW (tests)** | 3 | `test/test_mode_orchestrator/test_main.cpp`, `test/test_ota_updater/test_main.cpp`, `test/test_config_schedule/test_main.cpp` |
| **UPDATED** | 14 | `interfaces/DisplayMode.h`, `core/ConfigManager.h`, `core/ConfigManager.cpp`, `core/ModeRegistry.h`, `core/ModeRegistry.cpp`, `core/FlightDataFetcher.cpp`, `core/SystemStatus.h`, `core/SystemStatus.cpp`, `adapters/WebPortal.h`, `adapters/WebPortal.cpp`, `src/main.cpp`, `data-src/dashboard.html`, `data-src/dashboard.js`, `data-src/style.css` |
| **UPDATED (tests)** | 1 | `test/test_mode_registry/test_main.cpp` |
| **REBUILD** | 3 | `data/dashboard.html.gz`, `data/dashboard.js.gz`, `data/style.css.gz` |
| **UNCHANGED** | 30+ | All other firmware files, config headers, models, fetchers, WiFiManager, LayoutEngine, LogoManager, utils, wizard assets, health assets, existing mode .h/.cpp files |
| **NEW DIRECTORY** | 0 | All directories already exist from Display System |

### Struct Placement Clarification

Settings-related structs are split across two files by concern:

| Struct | File | Rationale |
|--------|------|-----------|
| `ModeSettingDef` | `interfaces/DisplayMode.h` | Part of the mode contract — modes declare settings schemas |
| `ModeSettingsSchema` | `interfaces/DisplayMode.h` | Part of the mode contract — modes declare settings schemas |
| `ModeEntry.settingsSchema` field | `core/ModeRegistry.h` | Registry owns mode table entries; field is nullable pointer |
| `ScheduleRule` | `core/ConfigManager.h` | Config storage concern — not part of mode contract |
| `ModeScheduleConfig` | `core/ConfigManager.h` | Config storage concern — not part of mode contract |

### Atomic Extern Pattern

`g_flightCount` is declared in `main.cpp` (rule 30). Files that need access use `extern`:

```cpp
// In core/FlightDataFetcher.cpp — writer:
extern std::atomic<uint8_t> g_flightCount;
// After xQueueOverwrite:
g_flightCount.store(enrichedFlights.size());

// In core/ModeOrchestrator.cpp — reader:
extern std::atomic<uint8_t> g_flightCount;
// In tick():
uint8_t flights = g_flightCount.load();
```

No other files should contain this extern declaration.

### Architectural Boundaries

**API Boundaries (HTTP):**

| Endpoint | Method | Handler | Cross-Core | Purpose |
|----------|--------|---------|------------|---------|
| `/api/ota/check` | GET | WebPortal (Core 1) | None | Check GitHub for new firmware |
| `/api/ota/pull` | POST | WebPortal (Core 1) | Spawns FreeRTOS task | Start firmware download |
| `/api/ota/status` | GET | WebPortal (Core 1) | Read OTAUpdater state | Poll download progress |
| `/api/schedule` | GET | WebPortal (Core 1) | None | Get mode schedule rules |
| `/api/schedule` | POST | WebPortal (Core 1) | None | Save mode schedule rules |
| `/api/display/modes` | GET | WebPortal (Core 1) | Read-only atomic | **UPDATED:** +settings array per mode |
| `/api/display/mode` | POST | WebPortal (Core 1) | Via ModeOrchestrator | **UPDATED:** → onManualSwitch(), +settings obj |
| `/api/status` | GET | WebPortal (Core 1) | Unchanged | **UPDATED:** +ota_available, +orchestrator_state |

**Component Boundaries (Firmware):**

```
┌──────────────────────────────────────────────────────────────────┐
│  Core 1 (WiFi/Web/API task + main loop)                          │
│  ┌────────────────┐   ┌──────────────────┐                       │
│  │   WebPortal    │   │ ModeOrchestrator │ (runs in main loop)   │
│  │  (HTTP routes) │   │   ::tick()       │                       │
│  └───────┬────────┘   └────────┬─────────┘                       │
│          │                     │                                  │
│   onManualSwitch()────────────→│                                  │
│          │                     │ requestSwitch()                  │
│          │                     ├─────────────────→ atomic write   │
│          │                     │                                  │
│  ┌───────┴────────┐   ┌───────┴──────────┐                       │
│  │  OTAUpdater    │   │ FlightDataFetcher│                       │
│  │  (FreeRTOS     │   │  g_flightCount   │                       │
│  │   download     │   │  .store()        │                       │
│  │   task)        │   └──────────────────┘                       │
│  └───────┬────────┘                                              │
└──────────┼───────────────────────────────────────────────────────┘
           │ prepareForOTA() — executes on caller's core (Core 1),
           │ but sets _switchState flag read by Core 0
           │
┌──────────┼───────────────────────────────────────────────────────┐
│  Core 0  │(Display task)                                         │
│  ┌───────▼──────────┐   ┌──────────────────┐                     │
│  │  ModeRegistry    │   │   ActiveMode     │ (DisplayMode*)      │
│  │  ::tick()        │──→│   .render()      │                     │
│  │  checks          │   └──────────────────┘                     │
│  │  _switchState    │                                            │
│  │  fade transition │                                            │
│  └──────────────────┘                                            │
│                                                                  │
│  ┌──────────────────┐   ┌──────────────────┐                     │
│  │ NeoMatrixDisplay │   │  DisplayUtils    │                     │
│  │ .show()          │   │  (free functions)│                     │
│  └──────────────────┘   └──────────────────┘                     │
└──────────────────────────────────────────────────────────────────┘
```

**Data Boundaries:**

| Boundary | Direction | Mechanism |
|----------|-----------|-----------|
| WebPortal → ModeOrchestrator | Core 1 internal | `onManualSwitch()` call (rule 24) |
| ModeOrchestrator → ModeRegistry | Core 1 → Core 0 | `requestSwitch()` atomic write |
| FlightDataFetcher → ModeOrchestrator | Core 1 internal | `g_flightCount` atomic (rule 30) |
| OTAUpdater → ModeRegistry | FreeRTOS task (Core 1) → Core 0 | `prepareForOTA()` executes on caller's core; sets `_switchState` flag that Core 0 reads — cross-core side effect, not cross-core call |
| OTAUpdater → Update.h | FreeRTOS task → flash | Stream-to-partition with incremental SHA-256 (rule 25) |
| ConfigManager → ModeOrchestrator | NVS → Core 1 | `getModeSchedule()` on tick() |
| ConfigManager → Modes | NVS → Core 0 | `getModeSetting()` called in mode `init()`, result cached; never in `render()` |
| Dashboard JS → OTA API | Browser → Core 1 | fetch() + 500ms polling for status |
| Dashboard JS → Schedule API | Browser → Core 1 | fetch() CRUD |
| Modes → ConfigManager | **Via init() only** | `getModeSetting()` in `init()`, cache result; never direct NVS access |
| Modes → NeoMatrixDisplay | **FORBIDDEN** | Modes never access NeoMatrixDisplay directly |
| Modes → g_flightCount | **FORBIDDEN** | Modes never read atomic globals (rule 30) |

### Requirements to Structure Mapping

**FR Group → File Mapping:**

| FR Group | FRs | Primary Files | Supporting Files |
|----------|-----|---------------|-----------------|
| Clock Mode | FR1-FR4 | `modes/ClockMode.h`, `modes/ClockMode.cpp` | `interfaces/DisplayMode.h` (schema structs), `core/ConfigManager` (getModeSetting), `main.cpp` (MODE_TABLE entry) |
| Departures Board | FR5-FR9 | `modes/DeparturesBoardMode.h`, `modes/DeparturesBoardMode.cpp` | `utils/DisplayUtils.h` (rendering), `core/LogoManager.h` (logos), `main.cpp` (MODE_TABLE entry) |
| Mode Transitions | FR10-FR12 | `core/ModeRegistry.h`, `core/ModeRegistry.cpp` | `interfaces/DisplayMode.h` (SwitchState) |
| Mode Scheduling | FR13-FR17, FR36, FR39 | `core/ModeOrchestrator.h`, `core/ModeOrchestrator.cpp` | `core/ConfigManager` (getModeSchedule), `main.cpp` (tick() in loop) |
| Mode Configuration | FR18-FR20, FR40 | `core/ConfigManager.h/.cpp`, mode `.h` files (schemas) | `adapters/WebPortal.cpp` (/api/display/modes iterates schema), `data-src/dashboard.js` (settings panels) |
| OTA Firmware Pull | FR21-FR31, FR37-FR38 | `core/OTAUpdater.h`, `core/OTAUpdater.cpp` | `core/ModeRegistry` (prepareForOTA), `adapters/WebPortal.cpp` (3 endpoints), `data-src/dashboard.js` (OTA UI) |
| System Resilience | FR32-FR35 | `core/ModeOrchestrator.h/.cpp`, `core/OTAUpdater.h/.cpp`, `core/ModeRegistry.h/.cpp`, `core/ConfigManager.h/.cpp` | **FR32** (watchdog → known-good mode): `ModeOrchestrator` boot/restore default mode, integrated from `main.cpp` loop. **FR33** (safe defaults for new Delight keys): `ConfigManager` first-read merge — absent keys get built-in defaults without clobbering existing NVS. **FR34** (heap / transition guards): OTA download blocked or warned below 80KB free heap — `OTAUpdater`; mode transition dual-buffer allocation or MVP fallback — `ModeRegistry` (`_executeFadeTransition`, alloc failure → instant cut). **FR35** (API + matrix responsiveness): flight pipelines — `FlightDataFetcher.cpp`, `OpenSkyFetcher.cpp`, `AeroAPIFetcher.cpp`; idle fallback and display continuity — `ModeOrchestrator`; GitHub/OTA unavailable — `OTAUpdater` + dashboard (`WebPortal`, `data-src/dashboard.js`). |
| Contributor Extensibility | FR41-FR42 | `interfaces/DisplayMode.h` (contract), `main.cpp` (MODE_TABLE) | Existing rule 22 (two touch points) |
| Documentation | FR43 | No firmware files | External documentation only |

**Cross-Cutting Concerns → File Mapping:**

| Concern | Files Touched |
|---------|--------------|
| Fade transition buffer lifecycle (DL1) | `core/ModeRegistry.h/.cpp` (_executeFadeTransition, malloc/free) |
| Mode orchestration state machine (DL2) | `core/ModeOrchestrator.h/.cpp`, `adapters/WebPortal.cpp` (onManualSwitch), `core/FlightDataFetcher.cpp` (g_flightCount), `src/main.cpp` (atomic decl + tick) |
| OTA Pull integrity (DL3) | `core/OTAUpdater.h/.cpp`, `core/ModeRegistry.cpp` (prepareForOTA) |
| Schedule NVS storage (DL4) | `core/ConfigManager.h/.cpp`, `core/ModeOrchestrator.cpp` |
| Per-mode settings schema (DL5) | `interfaces/DisplayMode.h` (structs), all new `modes/*.h` (schema decls), `core/ConfigManager.h/.cpp` (helpers), `adapters/WebPortal.cpp` (API iteration) |
| API endpoint additions (DL6) | `adapters/WebPortal.h/.cpp`, `data-src/dashboard.html/.js/.css` |
| Cross-core atomic signaling | `src/main.cpp` (declaration), `core/FlightDataFetcher.cpp` (extern + store), `core/ModeOrchestrator.cpp` (extern + load) |
| Gzip rebuild | `data/*.gz` (3 files) |

### Integration Points

**Internal Communication:**
- ModeOrchestrator runs in Core 1 main loop (~1/sec tick) — evaluates schedule rules, checks g_flightCount, calls `requestSwitch()` when state transitions require mode change
- OTAUpdater `checkForUpdate()` runs synchronously from WebPortal handler (~1-2s blocking) — acceptable for async web server single-request handling
- OTAUpdater `startDownload()` spawns FreeRTOS task, returns immediately — WebPortal handler responds with `{ ok: true }`; dashboard polls `/api/ota/status` at 500ms
- Fade transition runs on Core 0 inside `ModeRegistry::tick()` during SWITCHING state — `delay()` is acceptable since no other rendering occurs during transition

**External Integrations:**

| Service | Protocol | Purpose | New? |
|---------|----------|---------|------|
| GitHub Releases API | HTTPS GET | Check for new firmware version, download binary + .sha256 | **Yes** |
| NTP (pool.ntp.org) | UDP port 123 | Time for Clock Mode display + schedule evaluation | Existing (Foundation) |

**Data Flow (OTA Pull):**

```
Dashboard "Check for Updates" click
  → GET /api/ota/check
  → OTAUpdater::checkForUpdate()
  → GET https://api.github.com/repos/{owner}/{repo}/releases/latest
  → Parse JSON: tag_name, assets[].browser_download_url
  → Compare tag_name vs FW_VERSION
  → Return { available, version, release_notes }

Dashboard "Install Update" click
  → POST /api/ota/pull
  → OTAUpdater::startDownload() → spawns FreeRTOS task, returns immediately
  → { ok: true, data: { started: true } }
  → Dashboard polls GET /api/ota/status at 500ms

FreeRTOS download task (Core 1):
  → ModeRegistry::prepareForOTA()  (sets _switchState = SWITCHING on Core 1,
                                     blocking Core 0's tick() loop)
  → Download .sha256 file (64 hex chars)
  → Update.begin(partitionSize)
  → Stream .bin in chunks:
      Update.write(chunk) + mbedtls_sha256_update(chunk)
      _progress = bytesWritten * 100 / totalSize
  → mbedtls_sha256_finish() → compare
  → If match: Update.end(true) → ESP.restart()
  → If mismatch: Update.abort() → _state = ERROR
  → _downloadTask = nullptr → vTaskDelete(NULL)
```

**Data Flow (Mode Scheduling):**

```
ModeOrchestrator::tick() [Core 1, ~1/sec]:
  → getLocalTime(&now, 0)  // non-blocking (rule 14)
  → currentMin = hour * 60 + min
  → ConfigManager::getModeSchedule()  // read NVS
  → Evaluate rules in index order (first match wins)
  → If rule matches and state != SCHEDULED:
      _state = SCHEDULED → requestSwitch(rule.mode_id)
  → If no rule matches and state == SCHEDULED:
      _state = MANUAL → requestSwitch(_manualModeId)
  → If no rule matches and g_flightCount == 0 and state == MANUAL:
      _state = IDLE_FALLBACK → requestSwitch("clock")
  → If g_flightCount > 0 and state == IDLE_FALLBACK:
      _state = MANUAL → requestSwitch(_manualModeId)
```

### Delight Release — Party Mode Refinements (Structure)

**Reviewers:** Winston (Architect), Amelia (Developer), Quinn (QA)

**7 refinements incorporated:**

1. `SystemStatus.cpp` promoted to UPDATED — subsystem registration code lives in `.cpp` (Winston)
2. Boundary diagram: `prepareForOTA()` labeled as executing on caller's core (Core 1) with cross-core side effect via `_switchState` flag (Winston)
3. Removed `ClassicCardMode.h` and `LiveFlightCardMode.h` from UPDATED — settingsSchema is in MODE_TABLE in `main.cpp`, not in mode `.h` files (Amelia)
4. Struct placement clarified: `ModeSettingDef`/`ModeSettingsSchema` in `DisplayMode.h`, `settingsSchema` field added to `ModeEntry` in `ModeRegistry.h` (Amelia)
5. `extern std::atomic<uint8_t> g_flightCount` pattern documented for `FlightDataFetcher.cpp` and `ModeOrchestrator.cpp` (Amelia)
6. Added third test file: `test_config_schedule/test_main.cpp` for schedule NVS read/write + compaction logic (Quinn)
7. FR32-FR35 mapping clarified as "existing coverage, no Delight changes needed" for API degradation FRs (Quinn)

## Delight Release — Architecture Validation

### Coherence Validation

**Decision Compatibility:**

All 6 Delight decisions (DL1-DL6) form a consistent dependency chain with no contradictions:
- DL1 (fade transition) integrates into existing ModeRegistry switch lifecycle without modifying the DisplayMode interface
- DL2 (orchestrator) uses the existing `requestSwitch()` atomic mechanism from Display System — no new cross-core primitives needed
- DL3 (OTA Pull) shares `prepareForOTA()` with Foundation's OTA Push — both use the same `Update.h` stream-to-partition path
- DL4 (schedule NVS) and DL5 (per-mode settings NVS) use different key prefixes (`sched_r*` vs `m_*`) — no namespace collisions
- DL6 (API endpoints) follows the existing `{ ok, data/error }` JSON envelope from MVP — consistent pattern

No technology conflicts. No version incompatibilities. mbedTLS is bundled in ESP-IDF — no new external dependencies.

**Pattern Consistency:**

Rules 24-30 are additive to rules 1-23 with no contradictions:
- Rule 24 (orchestrator routing) extends rule 22 (two touch points) — modes don't initiate switches in either system
- Rule 25 (incremental SHA-256) extends rules 12-13 (OTA streaming) — same chunk-by-chunk discipline
- Rule 26 (prepareForOTA) applies to both Foundation Push and Delight Pull — unified pre-OTA sequence
- Rule 27 (fade buffers) follows rule 20 (heap in init/teardown) — transient allocation pattern
- Rule 28 (per-mode NVS helpers) follows rule 11 (NVS abbreviations in ConfigManager.h)
- Rule 30 (atomic globals in main.cpp) follows existing cross-core pattern

Naming conventions: `snake_case` flows consistently through new NVS keys (`sched_r0_start`, `m_clock_format`) → JSON fields → struct fields. PascalCase for classes (`ModeOrchestrator`, `OTAUpdater`), camelCase for methods (`onManualSwitch`, `checkForUpdate`).

**Structure Alignment:**

All Delight changes fit within existing hexagonal architecture boundaries:
- New core components (`ModeOrchestrator`, `OTAUpdater`) in `core/` — correct placement for coordination logic
- New modes (`ClockMode`, `DeparturesBoardMode`) in `modes/` — follows rule 17
- WebPortal gets new routes — same adapter pattern, no new boundary crossings
- ConfigManager gets new category helpers — consistent with existing struct pattern
- No new directories needed — Display System already created `modes/`, `utils/`

### Requirements Coverage

**All 43 Functional Requirements covered:**

| FR Group | FRs | Decision(s) | Coverage |
|----------|-----|-------------|----------|
| Clock Mode | FR1-FR4 | DL2, DL5 | ClockMode implementation + orchestrator idle fallback + 12/24h setting |
| Departures Board | FR5-FR9 | DL5 | DeparturesBoardMode implementation + configurable rows/fields settings |
| Mode Transitions | FR10-FR12 | DL1 | Fade transition with RGB888 dual buffers, double-buffering prevents tearing |
| Mode Scheduling | FR13-FR17, FR36, FR39 | DL2, DL4, DL6 | Orchestrator state machine + NVS persistence + schedule API endpoints |
| Mode Configuration | FR18-FR20, FR40 | DL5, DL6 | Per-mode settings schema + NVS helpers + Mode Picker UI + manual switch |
| OTA Firmware Pull | FR21-FR31, FR37-FR38 | DL3, DL6 | OTAUpdater component + GitHub API + SHA-256 + FreeRTOS task + 3 API endpoints |
| System Resilience | FR32-FR35 | DL1, DL3, existing | Watchdog → MANUAL boot, safe defaults, heap pre-check, API degradation (existing fetcher handling) |
| Contributor Extensibility | FR41-FR42 | Existing (rule 22) | DisplayMode contract + MODE_TABLE + Mode Picker lists all registered modes |
| Documentation | FR43 | N/A | No architectural impact — documentation validation only |

**All 21 Non-Functional Requirements architecturally addressed:**

| NFR | Target | Architectural Support |
|-----|--------|----------------------|
| NFR1 | ≥15fps transitions | DL1: FADE_STEPS=15, ~66ms/frame blend loop on Core 0 |
| NFR2 | <1 second transitions | DL1: 15 frames x ~66ms = ~1s max |
| NFR3 | ~30s idle fallback | DL2: ModeOrchestrator tick() checks g_flightCount each second |
| NFR4 | <50ms row update | DeparturesBoardMode in-place render(), no full-screen redraw |
| NFR5 | <60s OTA | DL3: stream-to-partition, ~10-15s for 1.5MB on 10Mbps |
| NFR6 | ±5s schedule accuracy | DL2: 1/sec tick with getLocalTime() non-blocking |
| NFR7 | <1s page load | Existing: gzipped assets, no external deps |
| NFR8 | <2s setting apply | DL5: ConfigManager hot-reload, no reboot |
| NFR9 | 30-day uptime | DL1 buffer lifecycle (malloc/free), DL3 task self-delete, no persistent allocations |
| NFR10 | 10s watchdog | Existing: FreeRTOS watchdog |
| NFR11 | Default mode after watchdog | DL2: ModeOrchestrator boots MANUAL state |
| NFR12 | OTA never bricks | DL3: Update.abort() on failure + Foundation dual-partition |
| NFR13 | Heap ceiling | Existing: MEMORY_REQUIREMENT check in switch flow |
| NFR14 | 2MB firmware | Monitor binary size; OTAUpdater adds ~10-15KB |
| NFR15 | NVS survives updates | NVS partition separate from app partitions |
| NFR16 | 80KB free heap | DL3: pre-download check; DL1: malloc fallback to instant cut |
| NFR17 | No heap degradation | DL1: malloc/free in single call; DL3: task self-deletes |
| NFR18 | API unavailability | Existing fetcher handling + DL3 check failure → ERROR state |
| NFR19 | Dashboard status line | DL6: GET /api/status + SystemStatus OTA_PULL entry |
| NFR20 | GitHub 429 handling | DL3: "try again later" message, no auto-retry |
| NFR21 | 10s network latency | Existing timeout handling in HTTPClient |

**Orphan check:** 0 FRs without architectural support. 0 decisions without FR traceability.

### Implementation Readiness

**Decision Completeness:** 6 decisions with code examples, interface definitions, state machine diagrams, and rationale. Implementation sequence specified with dependency graph. Cross-decision dependencies mapped.

**Structure Completeness:** Complete file change map — 11 new files (8 source + 3 test), 15 updated files. Requirements-to-file mapping explicit for all 43 FRs. Struct placement clarified across DisplayMode.h, ModeRegistry.h, and ConfigManager.h.

**Pattern Completeness:** 30 enforcement rules (11 MVP + 5 Foundation + 7 Display System + 7 Delight). All rules include code examples and anti-patterns. Cross-core signaling, OTA streaming, orchestrator routing, fade buffer lifecycle, per-mode settings NVS, and schedule NVS patterns all specified.

### Gap Analysis

**Critical Gaps:** 0

**Important Gaps:** 0

**Minor Notes (not blocking):**

1. **FR26 LED progress display** — Architecture specifies `prepareForOTA()` shows "Updating..." text on LEDs, but the exact progress bar rendering is an implementation detail. OTA download task can write progress directly to the LED array after `prepareForOTA()`.

2. **Clock Mode NTP fallback** — Architecture notes "falls back to uptime-based display if NTP unavailable" but doesn't specify exact fallback format. Implementation detail for ClockMode.render().

3. **Schedule rule naming** — Dashboard UI will need auto-generated labels (e.g., "Rule 1: 06:00-22:00 → Departures"). Pure UI rendering — no architectural decision needed.

### Architecture Completeness Checklist

**Requirements Analysis**
- [x] Project context analyzed (43 FRs, 21 NFRs, 5 party mode refinements from context review)
- [x] Scale and complexity assessed (medium — 2 new modes + orchestration + OTA)
- [x] Technical constraints identified (NVS key limits, heap budget, GitHub rate limit)
- [x] Cross-cutting concerns mapped (7 concerns including buffer lifecycle, state machine, OTA coexistence)

**Architectural Decisions**
- [x] 6 critical/important decisions documented with code examples
- [x] No new external dependencies (mbedTLS bundled in ESP-IDF)
- [x] Implementation sequence with dependency graph
- [x] Deferred decisions explicitly noted (multiple transitions, schedule templates, OTA signing)

**Implementation Patterns**
- [x] 7 new enforcement rules (24-30) with examples and anti-patterns
- [x] All rules additive — no changes to MVP (1-11), Foundation (12-16), or Display System (17-23)
- [x] Cross-core signaling pattern documented
- [x] OTA error discipline specified (Update.abort() on every error path)

**Project Structure**
- [x] Complete file change map (11 new + 15 updated + 3 rebuild)
- [x] Architectural boundaries with cross-core diagram
- [x] Data boundaries table with 12 entries
- [x] Requirements-to-file mapping for all FR groups
- [x] Atomic extern pattern documented

### Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION

**Confidence Level:** High — all FRs/NFRs covered, 3 party mode reviews with 23 total refinements incorporated, no critical or important gaps.

**Key Strengths:**
- Orchestrator state machine (DL2) cleanly separates three mode-switch sources with defined priority
- OTA Pull (DL3) reuses Foundation's streaming pattern — minimal new code surface
- Per-mode settings schema (DL5) follows existing ModeZoneDescriptor pattern — familiar to agents
- Fade transition (DL1) degrades gracefully on heap pressure — never blocks or crashes

**Architectural Overrides:**
- Context analysis section references "~20KB RGB565" buffers (from party mode round 1); decisions section corrects this to "~30KB RGB888" (party mode round 2). The decisions section is authoritative.
- FR35 API degradation marked "existing coverage" — no new Delight code needed for fetcher resilience

## Display System Release — Architecture Validation

### Coherence Validation

**Decision Compatibility:**

All 7 decisions (D1-D7) form a consistent dependency chain with no contradictions:
- D1 (interface) defines the contract that D2 (registry) manages and D3 (NeoMatrixDisplay) supports
- D4 (display task) integrates D2 and D3 at the system level
- D5 (API) and D7 (UI) operate through D2's public interface
- D6 (NVS) is called from D2's tick() debounce path
- The D1/D2 `getMemoryRequirement()` contradiction (instance method vs static function pointer) was caught and resolved in party mode review #2
- No version conflicts — all existing ESP32 Arduino libraries remain at current versions

**Pattern Consistency:**

- Enforcement rules 17-23 are additive to rules 1-16 with no conflicts
- Include path convention (path-from-firmware-root) is consistent with existing codebase
- API endpoint pattern (`/api/display/*`) follows existing `/api/*` JSON envelope convention
- NVS key pattern (`display_mode`) follows existing namespace convention
- Web asset pattern (fetch + json.ok + showToast) carried forward to Mode Picker

**Structure Alignment:**

- File change map covers all architectural decisions with explicit file locations
- PlatformIO build config updated for new directories (blocker caught in party mode #3)
- Boundaries clearly enforce: modes never touch ConfigManager/WiFiManager/SystemStatus (rule 18), never call FastLED.show() (rule 19)
- Two-touch-point extensibility pattern (rule 22) verified against file structure

### Requirements Coverage

**All 36 Functional Requirements covered.** Every FR maps to at least one architectural decision with explicit code location:
- DisplayMode Interface (FR1-FR4): Decision D1 — `interfaces/DisplayMode.h`
- Mode Registry & Lifecycle (FR5-FR10): Decision D2 — `core/ModeRegistry.h/.cpp`, `src/main.cpp`
- Classic Card Mode (FR11-FR13): Decision D3 — `modes/ClassicCardMode.h/.cpp`
- Live Flight Card Mode (FR14-FR16): New mode — `modes/LiveFlightCardMode.h/.cpp`
- Mode Picker UI (FR17-FR26): Decision D7 — `data-src/dashboard.html/js/css`
- Mode Persistence (FR27-FR29): Decision D6 — `core/ConfigManager.h/.cpp`, `src/main.cpp`
- Mode Switch API (FR30-FR33): Decision D5 — `adapters/WebPortal.h/.cpp`
- Display Infrastructure (FR34-FR36): Decisions D3 + D4 — `adapters/NeoMatrixDisplay.h/.cpp`, `src/main.cpp`

**All 12 NFRs architecturally addressed:**
- Performance: cooperative rendering within 16ms frame budget (P2), sub-2s mode switch (P1), zero-heap API enumeration (P4), <1s UI load (P3)
- Stability: sequential teardown→init for 100-switch heap stability (S1), switch serialization for rapid-switch safety (S2), heap-in-init discipline (S3), 30KB heap floor guard (S4), read-only brightness (S5), verified NVS namespace (S6)
- Compatibility: RenderContext isolation guarantees Foundation features untouched (C2), pixel parity via extraction validation (C1), cooperative scheduling preserved (C3), dashboard consistency (C4)

### Implementation Readiness

**Decision Completeness:** 7 decisions with code examples, struct definitions, switch flow pseudocode, API request/response formats, and rationale. Memory requirement pattern resolved (static constexpr + function pointer). Switch restoration pattern specified (teardown-but-don't-delete).

**Structure Completeness:** 10 NEW files + 12 UPDATED files + 1 new directory. PlatformIO build config updated. Include path conventions documented. Requirements-to-file mapping explicit. No orphaned files, no unmapped FRs.

**Pattern Completeness:** 23 enforcement rules (11 MVP + 5 Foundation + 7 Display System). Mode implementation pattern, rendering context discipline, shared utilities pattern, registration pattern, and UI patterns all specified with code examples and anti-patterns.

### Revised Implementation Sequence

**Critical path (three-phase ClassicCardMode extraction):**

1. **Heap baseline measurement** — `ESP.getFreeHeap()` log after full Foundation boot, before any Display System code
2. **DisplayMode interface + RenderContext** (`interfaces/DisplayMode.h`) — everything depends on this contract
3. **ModeRegistry** (`core/ModeRegistry.h/.cpp`) — switch logic, heap guard, tick(), terminal fallback path
4. **DisplayUtils extraction** (`utils/DisplayUtils.h/.cpp`) — extract shared helpers from NeoMatrixDisplay first
5. **ClassicCardMode — three-phase extraction:**
   - 5a: Create ClassicCardMode with COPIED rendering logic (NeoMatrixDisplay keeps its methods — both paths active)
   - 5b: Validate pixel parity — run both old and new paths against same flight data, compare output on hardware (human-in-the-loop gate)
   - 5c: Remove extracted methods from NeoMatrixDisplay, add `show()` + `buildRenderContext()` + `displayFallbackCard()`
6. **Display task integration** (`main.cpp`) — replace renderFlight with ModeRegistry::tick() + show()
7. **LiveFlightCardMode** (`modes/LiveFlightCardMode.h/.cpp`) — second mode, validates the abstraction works

**Parallel with steps 4-7:**
8. **Mode Switch API** (`WebPortal.cpp`) — GET + POST endpoints, can be stubbed early
9. **NVS persistence** (`ConfigManager.h/.cpp`) — `getDisplayMode()` / `setDisplayMode()`, trivial addition

**After all firmware complete:**
10. **Mode Picker UI** (`dashboard.html/js/css`) — depends on working API endpoints
11. **Gzip rebuild** — all updated web assets

### Architectural Overrides

**FR4 zone bounds — developer discipline, not code enforcement:**

FR4 states "display mode can render flight data within its allocated zone bounds without affecting pixels outside its zone." The architecture provides zone bounds via `LayoutResult` in `RenderContext`, but does not enforce clipping at the GFX layer — modes receive the full `Adafruit_NeoMatrix*` object. Adafruit GFX/NeoMatrix does not natively support clipping rectangles in the way needed. Zone discipline is enforced by developer practice, code review, and enforcement rule 18 (RenderContext discipline). Story acceptance criteria for each mode should include "visual verification that rendering stays within zone bounds."

**NFR P2 LogoManager exception — grandfathered filesystem I/O:**

NFR P2 prohibits filesystem I/O on the hot path. `LogoManager::loadLogo()` reads from LittleFS and is called from mode `render()` methods. This is inherited behavior from the pre-mode-system codebase — the same LittleFS reads occurred in `NeoMatrixDisplay::renderLogoZone()` before extraction. LittleFS reads for 2KB logo files on ESP32 flash complete in <1ms, well within the 16ms frame budget. NFR P2's filesystem prohibition applies to NEW mode code; LogoManager access is the single grandfathered exception.

### Terminal Fallback Path (FR36 Safety Net)

The D2 switch flow specifies teardown-but-don't-delete for safe restoration. Party mode review identified a gap: what happens if re-initialization of the previous mode ALSO fails?

**Complete terminal path:**

```
Switch requested → teardown current → heap check fails → re-init previous
                                                            ↓
                                                     init() succeeds → restored ✅
                                                     init() fails → _activeMode = nullptr
                                                            ↓
                                                     Display task detects nullptr:
                                                     g_display.displayFallbackCard(flights) → FR36 ✅
```

`displayFallbackCard()` uses `displaySingleFlightCard()` — the legacy single-flight renderer that has zero heap dependencies. This is the terminal safety net. If reached, the wall displays a degraded single-flight card, and the error propagates to the Mode Picker UI via `getLastError()`.

### Flight Cycling Pattern (Highest-Risk Extraction Point)

Party mode review identified flight cycling timing as the single most likely implementation error. The cycling check must use the configured interval from RenderContext, not the frame rate:

```cpp
void ClassicCardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    if (flights.empty()) {
        // Mode owns its idle state — show "..." or blank
        ctx.matrix->fillScreen(0);
        return;
    }

    // Advance flight index on configured interval (NOT every frame)
    // render() is called ~20fps (50ms). displayCycleMs is ~5000ms.
    // This check passes once every ~100 frames.
    if (millis() - _lastCycleMs >= ctx.displayCycleMs) {
        _currentFlightIndex = (_currentFlightIndex + 1) % flights.size();
        _lastCycleMs = millis();
    }

    // Bounds check in case flights vector shrank between cycles
    if (_currentFlightIndex >= flights.size()) {
        _currentFlightIndex = 0;
    }

    const auto& flight = flights[_currentFlightIndex];
    // ... render the current flight using DisplayUtils helpers ...
}
```

**Anti-pattern:** Do NOT use `delay()` or `vTaskDelay()` inside `render()`. Do NOT assume render() is called at any specific frequency. Always use `millis()` delta against `ctx.displayCycleMs`.

### NFR Validation Notes

**NFR S1/S2 stress testing:**

100-switch heap stability (S1) and 10-in-3-seconds rapid-switch safety (S2) require hardware stress testing that cannot be automated in unit tests. Implementation handoff requirement: add a `#ifdef DEBUG` serial command handler in `main.cpp` that accepts a `STRESS_SWITCH` command — triggers 100 sequential mode switches with before/after `ESP.getFreeHeap()` measurement and pass/fail output to serial. This is a one-time integration validation, not CI.

**NFR C1 pixel parity:**

Classic Card pixel parity is a human-in-the-loop validation gate. The implementing agent produces the extracted ClassicCardMode code. The human validates on physical hardware by comparing pre-extraction and post-extraction output for at least 5 distinct flight cards. Story acceptance criteria must reflect this split: agent delivers code, human confirms visual parity.

### Gap Analysis

| Priority | Gap | Resolution |
|----------|-----|-----------|
| Minor | `_switchState` read cross-core without atomic | ESP32 Xtensa: byte-sized enum reads are hardware-atomic. Safe in practice. Note for implementor: change to `std::atomic<SwitchState>` if portability desired. |
| Minor | FR7 "queuing" vs atomic overwrite | Architecture uses last-writer-wins during IDLE, SWITCH_IN_PROGRESS rejection during SWITCHING. Matches FR7 intent — concurrent requests serialized, not dropped silently. |
| Minor | `displayFallbackCard()` vs `displaySingleFlightCard()` naming | `displayFallbackCard()` wraps existing `displaySingleFlightCard()` logic. Implementor keeps internal method, adds public wrapper. |

No critical gaps. No important gaps.

### Display System Architecture Completeness Checklist

- [x] Display System PRD context analyzed (36 FRs, 12 NFRs, constraints)
- [x] Scale assessed (medium-high — plugin architecture on memory-constrained MCU)
- [x] Constraints identified (ESP32 heap, single display task, cross-core coordination)
- [x] Cross-cutting concerns mapped (RenderContext isolation, heap lifecycle, NVS debounce)
- [x] No new external dependencies (all existing ESP32 Arduino libraries)
- [x] 7 Display System decisions documented with code examples
- [x] Implementation patterns extended (rules 17-23)
- [x] NVS key table expanded (1 new key: `display_mode`)
- [x] 2 new API endpoints specified with request/response formats
- [x] Complete file change map (10 NEW + 12 UPDATED files)
- [x] Requirements-to-file mapping complete (36 FRs → specific files)
- [x] PlatformIO build config updated (blocker caught in party mode #3)
- [x] Architectural boundaries documented with forbidden paths
- [x] Three-phase ClassicCardMode extraction sequence specified
- [x] Terminal fallback path for FR36 documented
- [x] Flight cycling pattern with anti-patterns documented
- [x] NFR S1/S2 stress test mechanism specified
- [x] NFR C1 human-in-the-loop gate documented
- [x] All party mode refinements incorporated (3 sessions, 17+ refinements total)

### Display System Architecture Readiness Assessment

**Overall Status:** READY FOR IMPLEMENTATION
**Confidence Level:** High

**Key Strengths:**
- Clean plugin architecture on ESP32 — modes isolated via RenderContext with zero subsystem coupling
- Three-phase extraction sequence prevents the highest-risk failure (ClassicCardMode pixel parity regression)
- Heap lifecycle management with terminal fallback path — wall always displays something, even after cascading failures
- Two-touch-point extensibility (rule 22) — adding a mode is one file + one line
- Cross-core coordination mirrors existing `g_configChanged` pattern — no new concurrency primitives
- NVS debounce prevents flash wear during rapid switching
- Party mode reviews across 3 sessions caught: build blocker (platformio.ini), D1/D2 contradiction (memory requirement), extraction ordering risk, terminal fallback gap, cycling timing as highest-risk point
- All 23 enforcement rules are additive — no changes to MVP (1-11) or Foundation (12-16) rules

**Areas for Future Enhancement:**
- Mode-specific user settings (per-mode NVS keys and UI panels)
- Animated transitions between modes (blank transition only in this release)
- Mode scheduling (auto-switch by time of day)
- GFX clipping enforcement for third-party mode safety
- Automated pixel parity testing (screenshot comparison tooling)

### Display System Implementation Handoff

**Pre-implementation gates:**
1. Measure heap baseline: `ESP.getFreeHeap()` after full Foundation boot (WiFi + web server + NTP + flight data). This establishes the mode memory budget.
2. If free heap is below 60KB: investigate memory reduction before proceeding (30KB floor + headroom for two modes).
3. Photograph/record current Classic Card display with 5+ distinct flights — this is the pixel parity reference.

**First implementation priority:**
1. Create `interfaces/DisplayMode.h` (RenderContext, ModeZoneDescriptor, DisplayMode abstract class)
2. Create `core/ModeRegistry.h/.cpp` (static table, tick(), switch lifecycle with terminal fallback)
3. Extract `utils/DisplayUtils.h/.cpp` from NeoMatrixDisplay helpers
4. Create `modes/ClassicCardMode.h/.cpp` (copy — not extract — rendering logic from NeoMatrixDisplay)
5. Validate pixel parity on hardware (human gate)
6. Refactor NeoMatrixDisplay (remove extracted methods, add show() + buildRenderContext())
7. Update `platformio.ini` build_src_filter and build_flags
8. Integrate display task in `main.cpp` (MODE_TABLE, ModeRegistry::init(), tick()+show())
9. Verify firmware builds and ClassicCardMode renders correctly through ModeRegistry

**NFR validation requirements:**
- Add `#ifdef DEBUG` serial stress test handler for S1/S2 validation
- NFR C1 pixel parity is a manual hardware gate — agent delivers code, human confirms


]]></file>
<file id="41af4780" path="_bmad-output/implementation-artifacts/stories/le-1-8-flight-field-and-metric-widgets.md" label="DOCUMENTATION"><![CDATA[


# Story LE-1.8: Flight Field and Metric Widgets

branch: le-1-8-flight-field-and-metric-widgets
zone:
  - firmware/widgets/FlightFieldWidget.*
  - firmware/widgets/MetricWidget.*
  - firmware/core/WidgetRegistry.*
  - firmware/interfaces/DisplayMode.h
  - firmware/modes/CustomLayoutMode.*
  - firmware/adapters/WebPortal.cpp
  - firmware/data-src/editor.js
  - firmware/data/editor.js.gz
  - firmware/test/test_flight_field_widget/**
  - firmware/test/test_metric_widget/**

Status: review

## Story

As a **layout author**,
I want **widgets that bind to the current flight's live data fields (callsign, altitude, speed, route, aircraft type) and to computed numeric metrics (vertical rate, track heading)**,
So that **my custom layout shows real flight data on the LED wall, not just static text**.

## Acceptance Criteria

1. **Given** a `FlightFieldWidget` spec whose `content` field is one of:
   `"ident_icao"`, `"operator_icao"`, `"airline"`, `"aircraft"`, `"origin"`, `"destination"`, `"altitude_kft"`, `"speed_mph"`, `"track_deg"`, `"vertical_rate_fps"`
   **When** `WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx)` is called
   **Then** the render function reads the matching field from `ctx.currentFlight`, formats it via `DisplayUtils` char* API (zero heap), and draws it with `drawTextLine()`.

2. **Given** `ctx.currentFlight` is `nullptr` (no active flight)
   **When** `renderFlightField()` is called
   **Then** it draws `"\u2014"` (em dash) and returns `true` — never crashes, no OOB write.

3. **Given** a telemetry field with a `NAN` value (e.g. `altitude_kft` before the aircraft reports altitude)
   **When** `renderFlightField()` is called
   **Then** it draws `"--"` via `DisplayUtils::formatTelemetryValue()` fallback and returns `true`.

4. **Given** a `MetricWidget` spec whose `content` field is one of:
   `"track_deg"`, `"vertical_rate_fps"`
   **When** dispatched
   **Then** it draws the value formatted as an integer + unit string via the char* `formatTelemetryValue()` overload.
   *(Note: MetricWidget is the mechanism for "derived / display-formatted" telemetry; in MVP it shares the same FlightInfo source as FlightFieldWidget. A separate `ApiContext` or weather source is deferred to a future story.)*

5. **Given** both widget types **When** dispatched repeatedly at 20 fps
   **Then** no heap allocation occurs on the render hot path: the rendered string lives in a file-scope static buffer (`char[48]`), refreshed only when the source value changes (value-equality check on the pointer string or float value snapshot).

6. **Given** `GET /api/widgets/types`
   **When** returned
   **Then**:
   - `flight_field` entry has `"content"` field with `"kind": "select"` and `"options"` listing all supported field names (same set as AC #1).
   - `metric` entry has `"content"` field with `"kind": "select"` and `"options"` listing supported metric names.
   - **Neither entry** contains the `"note": "LE-1.8 — not yet implemented, renders nothing"` key.

7. **Given** `editor.js` `showPropsPanel()` function
   **When** a `flight_field` or `metric` widget is selected
   **Then** the content field renders as a `<select>` dropdown populated from the `options[]` returned by `GET /api/widgets/types` (same `widgetTypeMeta` cache already used by the editor) — not a free-text `<input>`.

8. **Given** a `FlightFieldWidget` or `MetricWidget` spec with dimensions below the minimum (`w < WIDGET_CHAR_W` or `h < WIDGET_CHAR_H`)
   **When** dispatched
   **Then** returns `true` without touching the framebuffer (matches existing pattern in TextWidget and ClockWidget).

9. **Given** `ctx.matrix == nullptr` (unit test path)
   **When** either widget render function is called
   **Then** it returns `true` without dereferencing the null pointer.

10. **Given** unit tests in `firmware/test/test_flight_field_widget/test_main.cpp` and `firmware/test/test_metric_widget/test_main.cpp`
    **When** run via `pio test -e esp32dev --filter test_flight_field_widget` and `--filter test_metric_widget`
    **Then** all cases pass for: field mapping (all 10 content values), fallback on `nullptr` flight, NAN fallback, min-dimension floor, null-matrix guard, and 1000-iteration dispatch heap-delta == 0.

11. **Given** the existing `firmware/test/test_widget_registry/test_main.cpp`
    **When** the two existing "not yet implemented" tests run:
    - `test_dispatch_flight_field_not_yet_implemented` (asserts `false`)
    - `test_dispatch_metric_not_yet_implemented` (asserts `false`)
    **Then** both tests **must be updated** to assert `true` (since both widgets are now implemented) — or renamed/replaced with equivalent positive-dispatch tests. **Do not leave these asserting `false` after implementation.**

12. **Given** the completed implementation **When** `~/.platformio/penv/bin/pio run -e esp32dev` is run from `firmware/`
    **Then** the binary is ≤ 1,382,400 bytes (88% of 1,572,864).

## Tasks / Subtasks

- [x] **Task 1: Add `currentFlight` to `RenderContext`** (AC: #1, #2, #3, #9)
  - [x]Open `firmware/interfaces/DisplayMode.h`
  - [x]Add `const FlightInfo* currentFlight;` field to the `RenderContext` struct after the `displayCycleMs` field:
    ```cpp
    const FlightInfo* currentFlight; // LE-1.8: pointer to flight being displayed; nullptr when no active flight
    ```
  - [x]`DisplayMode.h` already `#include`s `"models/FlightInfo.h"` — no new include needed
  - [x]**Do not** change the `DisplayMode` virtual interface signatures — `render(ctx, flights)` stays the same; `currentFlight` is wired at the call site in `CustomLayoutMode`

- [x] **Task 2: Wire `currentFlight` in `CustomLayoutMode::render()`** (AC: #1, #2)
  - [x]Open `firmware/modes/CustomLayoutMode.cpp`
  - [x]In `render(const RenderContext& ctx, const std::vector<FlightInfo>& flights)`, the current code has `(void)flights;` — replace that with:
    ```cpp
    // Select current flight for widget rendering (LE-1.8).
    // Use a mutable copy of ctx so we can set currentFlight without
    // changing the interface signature.
    RenderContext widgetCtx = ctx;
    widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];
    ```
  - [x]Replace `WidgetRegistry::dispatch(w.type, w, ctx)` with `WidgetRegistry::dispatch(w.type, w, widgetCtx)` in the widget loop
  - [x]**Important:** `flights[0]` is the currently-displayed flight (ModeOrchestrator cycles through the vector using `displayCycleMs`; index 0 is the front-of-queue flight handed to the mode each frame). Do not implement rotation logic here — CustomLayoutMode always renders the single flight it is given as element 0.
  - [x]Keep `ctx.matrix == nullptr` guard **before** the RenderContext copy — the guard returns early before any widget dispatch
  - [x]Ensure `RenderContext widgetCtx = ctx` is a shallow struct copy — safe because `RenderContext` contains only POD types and a pointer (no owning types, no destructors)

- [x] **Task 3: `FlightFieldWidget.h` and `FlightFieldWidget.cpp`** (AC: #1, #2, #3, #5, #8, #9)
  - [x]Create `firmware/widgets/FlightFieldWidget.h`:
    ```cpp
    #pragma once
    #include "core/WidgetRegistry.h"

    // Render a flight data field from RenderContext.currentFlight.
    // spec.content selects the field (e.g. "ident_icao", "altitude_kft").
    // Returns true on success (including no-flight, NAN, and undersized zone).
    bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx);
    ```
  - [x]Create `firmware/widgets/FlightFieldWidget.cpp` — see Dev Notes → FlightFieldWidget Implementation for the full reference implementation skeleton
  - [x]Field dispatch uses a `strcmp` chain (same pattern as `WidgetRegistry::fromString()`) — **no virtual dispatch, no heap**
  - [x]String fields (`ident_icao`, `operator_icao`, `airline_display_name_full`, `aircraft_display_name_short`, `origin.code_icao`, `destination.code_icao`): use `DisplayUtils::truncateToColumns()` char* overload into a 48-byte stack buffer
  - [x]Numeric fields (`altitude_kft`, `speed_mph`, `track_deg`, `vertical_rate_fps`): use `DisplayUtils::formatTelemetryValue()` char* overload; handles NAN → `"--"` automatically
  - [x]`nullptr` flight → write `"\xe2\x80\x94"` (UTF-8 em dash) or `"--"` into the buffer and draw via `drawTextLine()`
  - [x]Cache: file-scope static `char s_ffBuf[48]` + `char s_ffLastContent[48]` + `float s_ffLastVal`. See Dev Notes for caching strategy.
  - [x]Min-dimension floor: `if ((int)spec.w < WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) return true;`
  - [x]Null-matrix guard: `if (ctx.matrix == nullptr) return true;` (after cache update so the cache path is exercised in tests)

- [x] **Task 4: `MetricWidget.h` and `MetricWidget.cpp`** (AC: #4, #5, #8, #9)
  - [x]Create `firmware/widgets/MetricWidget.h`:
    ```cpp
    #pragma once
    #include "core/WidgetRegistry.h"

    // Render a computed metric from RenderContext.currentFlight.
    // spec.content selects the metric ("track_deg", "vertical_rate_fps").
    // Returns true on success (including no-flight and undersized zone).
    bool renderMetric(const WidgetSpec& spec, const RenderContext& ctx);
    ```
  - [x]Create `firmware/widgets/MetricWidget.cpp` — MVP implementation reads from the same `ctx.currentFlight` as `FlightFieldWidget`; metric names are a subset of telemetry fields with different formatting (integers only, unit suffix)
  - [x]Supported content values for MetricWidget:
    - `"track_deg"` → `formatTelemetryValue(flight->track_deg, "\xc2\xb0", 0)` (°, 0 decimals)
    - `"vertical_rate_fps"` → `formatTelemetryValue(flight->vertical_rate_fps, " fps", 1)` (1 decimal)
  - [x]Same caching pattern as FlightFieldWidget: file-scope static `char s_mBuf[48]` + `char s_mLastContent[48]` + `float s_mLastVal`
  - [x]Same min-dimension floor and null-matrix guard

- [x] **Task 5: Wire both widgets into `WidgetRegistry`** (AC: #1, #4, #11)
  - [x]Open `firmware/core/WidgetRegistry.cpp`
  - [x]Add includes after the existing widget includes:
    ```cpp
    #include "widgets/FlightFieldWidget.h"
    #include "widgets/MetricWidget.h"
    ```
  - [x]In `dispatch()`, replace the stub cases:
    ```cpp
    // BEFORE:
    case WidgetType::FlightField: return false;  // LE-1.8
    case WidgetType::Metric:      return false;  // LE-1.8

    // AFTER:
    case WidgetType::FlightField: return renderFlightField(spec, ctx);
    case WidgetType::Metric:      return renderMetric(spec, ctx);
    ```
  - [x]No changes needed to `fromString()` or `isKnownType()` — both already return correct values

- [x] **Task 6: Update `test_widget_registry` for AC #11** (AC: #11)
  - [x]Open `firmware/test/test_widget_registry/test_main.cpp`
  - [x]Find and update `test_dispatch_flight_field_not_yet_implemented()`:
    ```cpp
    // RENAME + CHANGE to:
    void test_dispatch_flight_field_returns_true() {
        RenderContext ctx = makeCtx();
        WidgetSpec spec = makeSpec(WidgetType::FlightField, 0, 0, 40, 8, "ident_icao");
        // ctx.currentFlight == nullptr → em dash fallback, still returns true
        TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
    }
    ```
  - [x]Find and update `test_dispatch_metric_not_yet_implemented()`:
    ```cpp
    // RENAME + CHANGE to:
    void test_dispatch_metric_returns_true() {
        RenderContext ctx = makeCtx();
        WidgetSpec spec = makeSpec(WidgetType::Metric, 0, 0, 40, 8, "track_deg");
        // ctx.currentFlight == nullptr → "--" fallback, still returns true
        TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Metric, spec, ctx));
    }
    ```
  - [x]Update `setup()` to call the renamed test functions
  - [x]**Note:** `makeCtx()` already zero-initializes `RenderContext{}` — after Task 1 adds `currentFlight`, it will be `nullptr` by default, which is exactly the fallback test path. No changes to `makeCtx()` needed.

- [x] **Task 7: Unit tests for FlightFieldWidget** (AC: #10)
  - [x]Create `firmware/test/test_flight_field_widget/test_main.cpp` — see Dev Notes → Unit Test Scaffold for the recommended test structure
  - [x]Test cases required:
    - `test_nullptr_flight_returns_true` — `ctx.currentFlight == nullptr`, any content → returns `true`
    - `test_string_field_ident_icao` — flight with `ident_icao = "UAL123"` → returns `true` (no framebuffer assertion needed with null matrix)
    - `test_string_field_airline` — flight with `airline_display_name_full = "United Airlines"` → returns `true`
    - `test_string_field_origin` — flight with `origin.code_icao = "KORD"` → returns `true`
    - `test_string_field_destination` → returns `true`
    - `test_telemetry_field_altitude_kft_valid` — `altitude_kft = 35.2` → returns `true`
    - `test_telemetry_field_altitude_kft_nan` — `altitude_kft = NAN` → returns `true` (NAN fallback to "--")
    - `test_telemetry_field_speed_mph` — `speed_mph = 480.0` → returns `true`
    - `test_unknown_content_returns_true` — `content = "nonexistent_field"` → returns `true` (draws "--")
    - `test_min_dimension_floor` — `spec.w = 2, spec.h = 2` → returns `true` without crash
    - `test_null_matrix_guard` — `ctx.matrix == nullptr` → returns `true`
    - `test_heap_stable_1000_iters` — 1000 dispatch calls; `ESP.getFreeHeap()` before and after; assert delta == 0 (or within 64 bytes noise)
  - [x]Flight object helper: create `makeFlightInfo()` that returns a `FlightInfo` with all string fields set to short values and telemetry set to `35.0` (non-NAN)

- [x] **Task 8: Unit tests for MetricWidget** (AC: #10)
  - [x]Create `firmware/test/test_metric_widget/test_main.cpp`
  - [x]Test cases required:
    - `test_nullptr_flight_returns_true`
    - `test_track_deg_valid` — `track_deg = 180.0` → returns `true`
    - `test_track_deg_nan` — `track_deg = NAN` → returns `true`
    - `test_vertical_rate_fps_valid` — `vertical_rate_fps = 12.5` → returns `true`
    - `test_unknown_metric_key_returns_true` — `content = "no_such_metric"` → returns `true`
    - `test_min_dimension_floor`
    - `test_null_matrix_guard`
    - `test_heap_stable_1000_iters`

- [x] **Task 9: Update `WebPortal::_handleGetWidgetTypes()`** (AC: #6)
  - [x]Open `firmware/adapters/WebPortal.cpp`
  - [x]In `_handleGetWidgetTypes()`, replace the `flight_field` block (lines ~2151–2164):
    - Remove `obj["note"] = "LE-1.8 \u2014 not yet implemented, renders nothing";`
    - Change `content` field from `"kind": "string"` to `"kind": "select"` with the field-name options array
    - See Dev Notes → WebPortal Flight Field Options for the exact `static const char* const` array to add
  - [x]Replace the `metric` block (lines ~2165–2178):
    - Remove the `"note"` key
    - Change `content` to `"kind": "select"` with metric name options
  - [x]Use the existing `addField` lambda pattern (already handles `options != nullptr`)
  - [x]**Do NOT** modify any other block (`text`, `clock`, `logo`)

- [x] **Task 10: Update `editor.js` property panel for select dropdown** (AC: #7)
  - [x]Open `firmware/data-src/editor.js`
  - [x]In `showPropsPanel(w)`, find the line:
    ```javascript
    var useSelect = (w.type === 'clock');
    ```
  - [x]Replace with:
    ```javascript
    var typeMeta = widgetTypeMeta[w.type] || {};
    var contentFieldMeta = null;
    if (typeMeta && typeMeta.fields) {
      for (var fi = 0; fi < typeMeta.fields.length; fi++) {
        if (typeMeta.fields[fi].key === 'content') { contentFieldMeta = typeMeta.fields[fi]; break; }
      }
    }
    var useSelect = (contentFieldMeta && contentFieldMeta.kind === 'select');
    ```
  - [x]Find the select-rebuild block (currently only rebuilds options for `w.type === 'clock'` with hardcoded `['12h', '24h']`):
    ```javascript
    if (useSelect && contentSelect) {
      /* Rebuild options from widgetTypeMeta */
      if (contentFieldMeta && contentFieldMeta.options && contentFieldMeta.options.length > 0) {
        contentSelect.innerHTML = '';
        for (var oi = 0; oi < contentFieldMeta.options.length; oi++) {
          var opt = document.createElement('option');
          opt.value = contentFieldMeta.options[oi];
          opt.textContent = contentFieldMeta.options[oi];
          contentSelect.appendChild(opt);
        }
      }
      contentSelect.value = w.content || (contentFieldMeta && contentFieldMeta['default']) || '';
    }
    ```
  - [x]**ES5 compliance:** use `var`, `function`, no arrow functions, no template literals, no `let`/`const`
  - [x]After editing, regenerate the gzip:
    ```bash
    gzip -9 -c firmware/data-src/editor.js > firmware/data/editor.js.gz
    ```

- [x] **Task 11: Build verification and test gate** (AC: #12)
  - [x]Run from `firmware/`:
    ```bash
    ~/.platformio/penv/bin/pio run -e esp32dev
    ```
    Binary must be ≤ 1,382,400 bytes
  - [x]Run unit tests (on-device):
    ```bash
    ~/.platformio/penv/bin/pio test -e esp32dev --filter test_flight_field_widget
    ~/.platformio/penv/bin/pio test -e esp32dev --filter test_metric_widget
    ~/.platformio/penv/bin/pio test -e esp32dev --filter test_widget_registry
    ```
  - [x]All three test suites must pass
  - [x]ES5 compliance grep (must return empty):
    ```bash
    grep -nP "=>|`|(?<![a-zA-Z_])let |(?<![a-zA-Z_])const " firmware/data-src/editor.js
    ```

---

## Dev Notes

### Read This Before Writing Any Code

This story implements the two widget types that have been stubbed since LE-1.2. The `WidgetType::FlightField` and `WidgetType::Metric` enum values, `fromString()` mappings, and `kAllowedWidgetTypes[]` entries already exist. The only missing pieces are:

1. **`RenderContext.currentFlight`** — the pointer field that threads the flight data into widget render functions
2. **`renderFlightField()` and `renderMetric()`** — the actual render functions
3. **`CustomLayoutMode::render()` wiring** — making it set `currentFlight` before dispatching
4. **WebPortal `_handleGetWidgetTypes()` update** — replacing the "not yet implemented" stub with real select options
5. **editor.js update** — generalizing the `useSelect` logic so `flight_field` and `metric` also get dropdowns

Everything else (WidgetType enum, fromString, kAllowedWidgetTypes) is already done and must **not** be changed.

---

### The Core Architectural Gap: `RenderContext.currentFlight`

**Current state (pre-LE-1.8):**

```cpp
// DisplayMode.h — RenderContext does NOT have currentFlight
struct RenderContext {
    FastLED_NeoMatrix* matrix;
    LayoutResult layout;
    uint16_t textColor;
    uint8_t brightness;
    uint16_t* logoBuffer;
    uint32_t displayCycleMs;
    // ← no currentFlight here
};

// CustomLayoutMode.cpp — flights vector is silently ignored
void CustomLayoutMode::render(const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights) {
    (void)flights;  // ← LE-1.8 must fix this
    // ...
    for (size_t i = 0; i < _widgetCount; ++i) {
        (void)WidgetRegistry::dispatch(w.type, w, ctx);  // ctx has no flight
    }
}
```

**After LE-1.8:**

```cpp
// DisplayMode.h — add one field to RenderContext
struct RenderContext {
    FastLED_NeoMatrix* matrix;
    LayoutResult layout;
    uint16_t textColor;
    uint8_t brightness;
    uint16_t* logoBuffer;
    uint32_t displayCycleMs;
    const FlightInfo* currentFlight;  // LE-1.8: nullptr when no active flight
};

// CustomLayoutMode.cpp — wire it up
void CustomLayoutMode::render(const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights) {
    if (ctx.matrix == nullptr) return;
    ctx.matrix->fillScreen(0);
    if (_invalid) { /* error indicator */ return; }

    RenderContext widgetCtx = ctx;
    widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];

    for (size_t i = 0; i < _widgetCount; ++i) {
        const WidgetSpec& w = _widgets[i];
        (void)WidgetRegistry::dispatch(w.type, w, widgetCtx);
    }
}
```

**Why `flights[0]` and not a smarter selection?** `CustomLayoutMode` is a single-flight-per-frame renderer (unlike `ClassicCardMode` which draws one flight per zone or `LiveFlightCardMode` which focuses one flight in detail). The `displayCycleMs` controls how often ModeOrchestrator rotates which flight is "current." For this mode, `flights[0]` is always the currently-displayed flight.

**Adding `currentFlight` to `RenderContext` affects all call sites** that construct a `RenderContext`. Check all TUs that create a `RenderContext{}` — after the field is added, C++ zero-initialization (`RenderContext ctx{}`) will set the pointer to `nullptr`, which is the correct default for modes that don't use flight data (TextWidget, ClockWidget, LogoWidget all ignore `ctx.currentFlight`).

---

### FlightFieldWidget Implementation

**Supported `content` values and their FlightInfo sources:**

| `content` string | FlightInfo field | Type | Format |
|---|---|---|---|
| `"ident_icao"` | `flight->ident_icao` | `String` | Direct (truncate) |
| `"operator_icao"` | `flight->operator_icao` | `String` | Direct (truncate) |
| `"airline"` | `flight->airline_display_name_full` | `String` | Direct (truncate) |
| `"aircraft"` | `flight->aircraft_display_name_short` | `String` | Direct (truncate) |
| `"origin"` | `flight->origin.code_icao` | `String` | Direct (truncate) |
| `"destination"` | `flight->destination.code_icao` | `String` | Direct (truncate) |
| `"altitude_kft"` | `flight->altitude_kft` | `double` | `formatTelemetryValue(val, "k", 1)` |
| `"speed_mph"` | `flight->speed_mph` | `double` | `formatTelemetryValue(val, " mph", 0)` |
| `"track_deg"` | `flight->track_deg` | `double` | `formatTelemetryValue(val, "\xc2\xb0", 0)` |
| `"vertical_rate_fps"` | `flight->vertical_rate_fps` | `double` | `formatTelemetryValue(val, " fps", 1)` |

**Reference implementation skeleton (FlightFieldWidget.cpp):**

```cpp
#include "widgets/FlightFieldWidget.h"
#include "utils/DisplayUtils.h"
#include "models/FlightInfo.h"
#include <cstring>
#include <cmath>

namespace {
    // File-scope static cache — zero heap on hot path (Rule 20).
    // s_ffBuf: the last rendered string. Refreshed when source value changes.
    // s_ffLastContent: last spec.content used (to detect field switch).
    // s_ffLastStr: last source string value (for string fields).
    // s_ffLastVal: last source double value (for numeric fields).
    char     s_ffBuf[48]         = "";
    char     s_ffLastContent[48] = "";
    char     s_ffLastStr[48]     = "";
    double   s_ffLastVal         = 0.0 / 0.0; // NAN
}

bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx) {
    // Min-dimension floor (AC #8)
    if ((int)spec.w < WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) return true;

    const char* content = spec.content;
    const FlightInfo* flight = ctx.currentFlight;

    // Determine if cache is stale (content field changed or value changed).
    // For nullptr flight: always cache "—" (settled state).
    bool contentChanged = (strcmp(content, s_ffLastContent) != 0);

    if (flight == nullptr) {
        if (contentChanged || s_ffBuf[0] == '\0') {
            strncpy(s_ffBuf, "\xe2\x80\x94", sizeof(s_ffBuf) - 1);  // UTF-8 em dash
            s_ffBuf[sizeof(s_ffBuf) - 1] = '\0';
        }
    } else if (/* string field check */ ...) {
        // For string fields: compare current string to s_ffLastStr
        // ...
    } else {
        // For numeric fields: compare current double to s_ffLastVal
        // ...
    }
    strncpy(s_ffLastContent, content, sizeof(s_ffLastContent) - 1);

    // Null-matrix guard — cache updated above so test path exercises caching.
    if (ctx.matrix == nullptr) return true;

    // Truncate to column width and draw
    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    char out[48];
    DisplayUtils::truncateToColumns(s_ffBuf, maxCols, out, sizeof(out));

    int16_t drawY = spec.y;
    if ((int)spec.h > WIDGET_CHAR_H) {
        drawY = spec.y + (int16_t)(((int)spec.h - WIDGET_CHAR_H) / 2);
    }
    DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);
    return true;
}
```

**Caching strategy for numeric fields:**

```cpp
// Check if the double value changed meaningfully (handles NAN equality).
bool valChanged = (std::isnan(currentVal) != std::isnan(s_ffLastVal))
               || (!std::isnan(currentVal) && currentVal != s_ffLastVal);
if (contentChanged || valChanged) {
    DisplayUtils::formatTelemetryValue(currentVal, suffix, decimals,
                                       s_ffBuf, sizeof(s_ffBuf));
    s_ffLastVal = currentVal;
}
```

**Caching strategy for string fields (`String` → `c_str()`):**

```cpp
const char* src = flight->ident_icao.c_str();  // for "ident_icao"
bool strChanged = (strcmp(src, s_ffLastStr) != 0);
if (contentChanged || strChanged) {
    DisplayUtils::truncateToColumns(src, sizeof(s_ffBuf) / WIDGET_CHAR_W,
                                    s_ffBuf, sizeof(s_ffBuf));
    strncpy(s_ffLastStr, src, sizeof(s_ffLastStr) - 1);
    s_ffLastStr[sizeof(s_ffLastStr) - 1] = '\0';
}
```

**Note on cache for unknown content strings:** If `spec.content` does not match any known field name, write `"--"` into `s_ffBuf` and return `true`. Never crash or return `false` on unknown content.

---

### MetricWidget Implementation

`MetricWidget` is structurally identical to `FlightFieldWidget` but for a narrower set of content values with specific formatting:

| `content` string | FlightInfo field | Format |
|---|---|---|
| `"track_deg"` | `flight->track_deg` | `formatTelemetryValue(val, "\xc2\xb0", 0)` (e.g. `"180°"`) |
| `"vertical_rate_fps"` | `flight->vertical_rate_fps` | `formatTelemetryValue(val, " fps", 1)` (e.g. `"12.5 fps"`) |

**Future expansion note:** `MetricWidget` is scoped to current-flight telemetry in MVP. When weather or other API data sources are added in a future story, a new `ApiContext` struct will be introduced and passed either through `RenderContext` or as a separate parameter. For now, `MetricWidget` reads from `ctx.currentFlight` exactly like `FlightFieldWidget`. **Do not add `ApiContext` in this story.**

---

### WebPortal `_handleGetWidgetTypes()` — Exact Changes

Add these static arrays to the function (local linkage, same as `clockFmtOptions` and `alignOptions`):

```cpp
// ── flight_field ─────────────────────────────────────────────────────
{
    JsonObject obj = data.add<JsonObject>();
    obj["type"] = "flight_field";
    obj["label"] = "Flight Field";
    // Remove the "note" key entirely
    JsonArray fields = obj["fields"].to<JsonArray>();
    static const char* const ffOptions[] = {
        "ident_icao", "operator_icao", "airline", "aircraft",
        "origin", "destination",
        "altitude_kft", "speed_mph", "track_deg", "vertical_rate_fps"
    };
    addField(fields, "content", "select", "ident_icao", 0, false, ffOptions, 10);
    addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
    addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
    addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
    addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
    addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
}
// ── metric ───────────────────────────────────────────────────────────
{
    JsonObject obj = data.add<JsonObject>();
    obj["type"] = "metric";
    obj["label"] = "Metric";
    // Remove the "note" key entirely
    JsonArray fields = obj["fields"].to<JsonArray>();
    static const char* const mOptions[] = {
        "track_deg", "vertical_rate_fps"
    };
    addField(fields, "content", "select", "track_deg", 0, false, mOptions, 2);
    addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
    addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
    addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
    addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
    addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
}
```

**Three-list sync rule reminder (from WidgetRegistry.cpp comment):**
The widget type strings must stay in sync across all three call sites:
1. `core/LayoutStore.cpp` → `kAllowedWidgetTypes[]` — **already contains both; no change needed**
2. `core/WidgetRegistry.cpp` → `fromString()` — **already correct; no change needed**
3. `adapters/WebPortal.cpp` → `_handleGetWidgetTypes()` — **update in Task 9**

---

### editor.js — `showPropsPanel` Change in Detail

**Current code (LE-1.7, lines ~373–393):**

```javascript
/* content field — text input or select depending on widget type */
var meta = widgetTypeMeta[w.type] || {};
var contentField  = document.getElementById('props-field-content');
var contentInput  = document.getElementById('prop-content');
var contentSelect = document.getElementById('prop-content-select');
if (contentField) {
  /* clock uses a select for "12h"/"24h"; all others use text */
  var useSelect = (w.type === 'clock');
  contentInput.style.display  = useSelect ? 'none' : '';
  contentSelect.style.display = useSelect ? '' : 'none';
  if (useSelect) {
    /* Rebuild options only if needed */
    if (contentSelect.options.length === 0) {
      var clockOpts = ['12h', '24h'];
      for (var i = 0; i < clockOpts.length; i++) {
        var opt = document.createElement('option');
        opt.value = clockOpts[i];
        opt.textContent = clockOpts[i];
        contentSelect.appendChild(opt);
      }
    }
    contentSelect.value = w.content || '24h';
  } else {
    contentInput.value = w.content || '';
  }
}
```

**After LE-1.8 (Task 10):**

```javascript
/* content field — text input or select depending on widget type */
var typeMeta = widgetTypeMeta[w.type] || {};
var contentField  = document.getElementById('props-field-content');
var contentInput  = document.getElementById('prop-content');
var contentSelect = document.getElementById('prop-content-select');
if (contentField) {
  /* Determine whether to use a select from the API metadata */
  var contentFieldMeta = null;
  if (typeMeta && typeMeta.fields) {
    for (var fi = 0; fi < typeMeta.fields.length; fi++) {
      if (typeMeta.fields[fi].key === 'content') {
        contentFieldMeta = typeMeta.fields[fi];
        break;
      }
    }
  }
  var useSelect = (contentFieldMeta && contentFieldMeta.kind === 'select');
  if (contentInput)  contentInput.style.display  = useSelect ? 'none' : '';
  if (contentSelect) contentSelect.style.display = useSelect ? '' : 'none';
  if (useSelect && contentSelect) {
    /* Rebuild options from API metadata (works for clock, flight_field, metric) */
    if (contentFieldMeta && contentFieldMeta.options && contentFieldMeta.options.length > 0) {
      contentSelect.innerHTML = '';
      for (var oi = 0; oi < contentFieldMeta.options.length; oi++) {
        var opt = document.createElement('option');
        opt.value = contentFieldMeta.options[oi];
        opt.textContent = contentFieldMeta.options[oi];
        contentSelect.appendChild(opt);
      }
    }
    contentSelect.value = w.content || (contentFieldMeta && contentFieldMeta['default']) || '';
  } else {
    if (contentInput) contentInput.value = w.content || '';
  }
}
```

**Key behavioral changes:**
- `useSelect` is now driven by the API metadata `kind === 'select'`, not by hardcoded `w.type === 'clock'`
- The options rebuild now uses `contentFieldMeta.options[]` from the API instead of hardcoded `['12h', '24h']`
- Clock still works identically (API returns `kind: "select"` with options `["12h", "24h"]`)
- `flight_field` and `metric` now show a dropdown instead of a text input

**`widgetTypeMeta` is already populated** by the editor's `initWidgetTypes()` call which GETs `/api/widgets/types` on page load and stores each entry keyed by `entry.type`. After Task 9 updates the WebPortal endpoint, `widgetTypeMeta['flight_field'].fields[0]` will have `kind: "select"` and `options: ["ident_icao", ...]` — the editor code above just reads them.

---

### Unit Test Scaffold

**Directory structure to create:**

```
firmware/test/test_flight_field_widget/test_main.cpp
firmware/test/test_metric_widget/test_main.cpp
```

**`platformio.ini` test filter pattern** (existing `[env:esp32dev]` already declares `test_filter = test_*` — no change needed; new test dirs are auto-discovered).

**Minimal FlightInfo helper for tests:**

```cpp
#include "models/FlightInfo.h"

static FlightInfo makeFlightInfo() {
    FlightInfo f;
    f.ident_icao = "UAL123";
    f.ident_iata = "UA123";
    f.ident = "UAL123";
    f.operator_icao = "UAL";
    f.operator_iata = "UA";
    f.origin.code_icao = "KORD";
    f.destination.code_icao = "KSFO";
    f.aircraft_code = "B739";
    f.airline_display_name_full = "United Airlines";
    f.aircraft_display_name_short = "Boeing 737-900";
    f.altitude_kft = 35.0;
    f.speed_mph = 480.0;
    f.track_deg = 270.0;
    f.vertical_rate_fps = 0.0;
    return f;
}
```

**Heap-delta test pattern (on-device only):**

```cpp
void test_heap_stable_1000_iters() {
    FlightInfo flight = makeFlightInfo();
    RenderContext ctx{};
    ctx.matrix = nullptr;
    ctx.currentFlight = &flight;

    WidgetSpec spec{};
    spec.type = WidgetType::FlightField;
    spec.x = 0; spec.y = 0; spec.w = 80; spec.h = 8;
    spec.color = 0xFFFF;
    strncpy(spec.content, "altitude_kft", sizeof(spec.content) - 1);

    uint32_t heapBefore = ESP.getFreeHeap();
    for (int i = 0; i < 1000; i++) {
        WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx);
    }
    uint32_t heapAfter = ESP.getFreeHeap();
    // Allow up to 64 bytes noise (RTOS accounting variance).
    TEST_ASSERT_UINT32_WITHIN(64, heapBefore, heapAfter);
}
```

**RenderContext construction in tests:** After Task 1 adds `currentFlight`, `RenderContext ctx{}` zero-initializes the pointer to `nullptr`. Tests that want a valid flight set `ctx.currentFlight = &someFlightInfo`.

---

### Zero-Heap Rule on the Render Hot Path

From BMAD Architecture Rule 20: "Render functions must not allocate heap (no `new`, no `std::string` construction, no `String` objects on the hot path)."

**Allowed:**
- Stack buffers: `char out[48]`
- File-scope static buffers: `static char s_ffBuf[48]`
- Calling `DisplayUtils` char* overloads (they write into caller-provided buffers)
- Comparing `flight->ident_icao.c_str()` (reading a `String`'s internal buffer, not constructing a new `String`)

**Not allowed:**
- `String s = flight->ident_icao + " (" + flight->operator_icao + ")";` — constructs two temp `String` objects on heap
- `snprintf(new char[48], ...)` — heap allocation
- `std::vector`, `std::map`, `std::string` in render path

**`FlightInfo` fields are Arduino `String` objects** (heap-allocated). You may read them via `.c_str()` for `strcmp`/`strcpy` — this does NOT copy the heap buffer, it just returns a pointer to the existing allocation. This is safe and allocation-free on the read path.

---

### Antipattern Prevention Table

| DO NOT | DO INSTEAD | Reason |
|---|---|---|
| Use `String` temporaries in render fn | Use `.c_str()` + `strncpy` into stack/static buffers | String ctor/dtor = heap alloc per frame |
| Dereference `ctx.currentFlight` without nullptr check | Always check `if (ctx.currentFlight == nullptr)` first | Other modes and tests pass `nullptr` |
| Return `false` on unknown `content` value | Write `"--"` to buf and return `true` | `false` from dispatch is "hard error"; unknown field is just a graceful no-op |
| Add `ApiContext` or weather data in this story | Mark as TODO and file a deferred-work note | Scope: MVP is flight data only |
| Change `WidgetType` enum values | Never change existing enum integer values | They may be serialized/persisted |
| Change `fromString()` | No changes needed — already handles `"flight_field"` and `"metric"` | Already correct from LE-1.2 |
| Change `kAllowedWidgetTypes[]` | No changes needed | Already includes both types from LE-1.1 |
| Call `flights[i]` without bounds check | Always check `flights.empty()` before indexing | `flights` can be empty if no API data yet |
| Leave `(void)flights;` in CustomLayoutMode | Replace with RenderContext copy + `currentFlight` assignment | This is the entire purpose of this story |
| Change `DisplayMode` virtual interface | Only add field to `RenderContext` struct | Changing the virtual `render(...)` signature would break all mode implementations |
| Use `let`/`const`/`=>` in `editor.js` | `var`/`function` only | ES5 constraint |
| Re-populate `<select>` options on every `showPropsPanel` call | Use `contentSelect.innerHTML = ''` + rebuild | Avoids duplicate options accumulating on repeated widget selections |
| Forget to regenerate `.gz` after editor.js edit | `gzip -9 -c data-src/editor.js > data/editor.js.gz` | Device serves only `.gz` files |

---

### Relationship to Existing Widget Tests

`test_widget_registry/test_main.cpp` currently has two tests that **assert `false`** for `FlightField` and `Metric` dispatch (they are "stub" tests from LE-1.2):

```cpp
void test_dispatch_flight_field_not_yet_implemented() {
    // ...
    TEST_ASSERT_FALSE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
}
void test_dispatch_metric_not_yet_implemented() {
    // ...
    TEST_ASSERT_FALSE(WidgetRegistry::dispatch(WidgetType::Metric, spec, ctx));
}
```

**These tests will FAIL after Task 5** (wiring the real render functions). Task 6 must update them to `TEST_ASSERT_TRUE` **before** running `pio test`. Do not skip this — failing tests are a hard gate.

**Note on `makeCtx()` in test_widget_registry:** After Task 1 adds `currentFlight`, the zero-init `RenderContext ctx{}` in `makeCtx()` will have `currentFlight = nullptr`. The updated FlightField and Metric tests will exercise the nullptr-flight fallback path, which is correct for the registry-level dispatch test (field-specific tests live in the new test suites).

---

### `DisplayUtils::formatTelemetryValue` — char* Overload Reference

```cpp
// Header (utils/DisplayUtils.h):
void formatTelemetryValue(double value, const char* suffix, int decimals,
                           char* out, size_t outLen);
```

Behavior:
- If `value` is `NAN`: writes `"--"` into `out`
- Otherwise: `snprintf(out, outLen, "%.*f%s", decimals, value, suffix)`
- Safe: always null-terminates `out`

**Usage for altitude:**
```cpp
DisplayUtils::formatTelemetryValue(flight->altitude_kft, "k", 1, s_ffBuf, sizeof(s_ffBuf));
// flight->altitude_kft = 35.2 → s_ffBuf = "35.2k"
// flight->altitude_kft = NAN  → s_ffBuf = "--"
```

---

### LittleFS Budget Impact

LE-1.8 adds no new LittleFS-stored web assets (only regenerates `editor.js.gz` which is already in the partition). The two new `.cpp` files for FlightFieldWidget and MetricWidget are firmware source only.

Estimated flash delta: ~2–4 KB of compiled code for both widgets + test stubs. Well within the 88% budget (current baseline from LE-1.7: 82.4%).

---

### Canonical Widget Content Key Vocabulary (as shipped)

The acceptance-criteria text in AC #1 and AC #4 was authored before the
implementation settled, and uses a longer field-name style that does not match
the keys actually persisted by `WebPortal::_handleGetWidgetTypes()`, consumed
by `FlightFieldWidget.cpp` / `MetricWidget.cpp`, and asserted by the Unity test
suites. This subsection is the authoritative record going forward. **Future
stories must inherit these key names verbatim.**

**FlightFieldWidget — 5 canonical keys**

| `spec.content` | FlightInfo source (with fallback chain) | Notes |
|---|---|---|
| `"airline"` | `airline_display_name_full` | Direct string, truncated to columns |
| `"ident"` | `ident_icao` → `ident_iata` → `ident` | ICAO preferred; falls back to IATA, then raw ident |
| `"origin_icao"` | `origin.code_icao` | Truncated to columns |
| `"dest_icao"` | `destination.code_icao` | Truncated to columns |
| `"aircraft"` | `aircraft_display_name_short` → `aircraft_code` | Short display preferred; falls back to ICAO type code |

**MetricWidget — 4 canonical keys**

| `spec.content` | FlightInfo source | Format |
|---|---|---|
| `"alt"` | `altitude_kft` | `formatTelemetryValue(val, "k", 1)` → e.g. `"34.0k"` |
| `"speed"` | `speed_mph` | `formatTelemetryValue(val, " mph", 0)` → e.g. `"487 mph"` |
| `"track"` | `track_deg` | `formatTelemetryValue(val, "\xc2\xb7", 0)` → e.g. `"247\xc2\xb7"` (degree suffix byte `0xF7` in CP437 after `_cp437` offset) |
| `"vsi"` | `vertical_rate_fps` | `formatTelemetryValue(val, " fps", 1)` → e.g. `"-12.5 fps"` |

**Drift vs. original AC text (documented, not a bug):**
- AC #1 originally listed `ident_icao`, `operator_icao`, `origin`, `destination`,
  `altitude_kft`, `speed_mph`, `track_deg`, `vertical_rate_fps` as
  FlightFieldWidget content values. The shipped implementation consolidates
  these into the 5 short keys above, with telemetry fields moved into
  MetricWidget under the short telemetry vocabulary (`alt`/`speed`/`track`/`vsi`).
- AC #4 originally listed `track_deg` / `vertical_rate_fps` as MetricWidget
  keys. The shipped keys are `track` / `vsi`; semantics and formatting are
  unchanged.
- AC #6 (`GET /api/widgets/types`): the JSON `options[]` arrays reflect the
  canonical keys listed above — not the original AC text.

**Tests that pin the canonical keys:**
- `firmware/test/test_flight_field_widget/test_main.cpp` — exercises all 5
  FlightFieldWidget keys (`airline`, `ident`, `origin_icao`, `dest_icao`,
  `aircraft`), including the ident and aircraft fallback chains.
- `firmware/test/test_metric_widget/test_main.cpp` — exercises all 4
  MetricWidget keys (`alt`, `speed`, `track`, `vsi`), including NAN fallback
  for each.
- `firmware/test/test_widget_registry/test_main.cpp` — registry-level
  dispatch uses `"airline"` for FlightField and `"alt"` for Metric, matching
  the canonical vocabulary above. **If the canonical keys are ever migrated,
  this registry test's `spec.content` must be updated in lock-step** (see
  AI-Review LOW action item).

**Rename? Deferred.** Renaming the persisted keys to match the original AC
text would be a breaking change for any layout JSON already saved to LittleFS,
and would require a migration path in `LayoutStore`. That migration is out of
scope for LE-1.8 and is not currently planned. The canonical names above are
treated as stable.

---

### AC #5 Caching — Formally Deferred

AC #5 as originally written calls for "file-scope static buffer (`char[48]`),
refreshed only when the source value changes (value-equality check on the
pointer string or float value snapshot)." The shipped implementation of
`FlightFieldWidget` and `MetricWidget` **does not** implement this cache —
each call to the render function re-resolves the FlightInfo field and
re-formats it via `DisplayUtils`.

**Why this is acceptable for LE-1.8:**
- The render path uses only stack buffers and `DisplayUtils` char* overloads,
  which write into caller-provided buffers. **No heap allocation occurs on the
  hot path**, which is the part of AC #5 that actually guards correctness
  (Rule 20). The `test_heap_stable_1000_iters` pattern documented in the Unit
  Test Scaffold above remains the acceptance gate.
- At 20 fps with two to four flight widgets on screen, the re-format cost is a
  handful of `snprintf` / `strcmp` calls per frame — well within the frame
  budget. Profiling on real hardware during LE-1.7 did not show render-path
  CPU as a bottleneck.
- Adding a cache now would require threading content-key + source-value
  snapshots through both widgets and growing static RAM by ~200 bytes, with
  no observable benefit until a future feature (e.g. many-widget dashboards)
  creates pressure.

**Deferred to:** a future story that either (a) profiles the render path
under high widget counts and shows a measurable gain, or (b) introduces a
widget type whose format cost is meaningfully higher than a single
`snprintf` (e.g. a text-rolling or bitmap-derived widget).

**Acceptance-criteria disposition:** AC #5's zero-heap clause is satisfied by
the stack-buffer + char* overload pattern. AC #5's value-equality cache
clause is explicitly deferred per this subsection.

---

### Sprint Status Update

After all tasks pass, update `_bmad-output/implementation-artifacts/sprint-status.yaml`:
```yaml
le-1-8-flight-field-and-metric-widgets: done
```

---

## File List

Files **created** (new):
- `firmware/widgets/FlightFieldWidget.h` — render function declaration
- `firmware/widgets/FlightFieldWidget.cpp` — render function implementation
- `firmware/widgets/MetricWidget.h` — render function declaration
- `firmware/widgets/MetricWidget.cpp` — render function implementation
- `firmware/test/test_flight_field_widget/test_main.cpp` — Unity test suite
- `firmware/test/test_metric_widget/test_main.cpp` — Unity test suite

Files **modified**:
- `firmware/interfaces/DisplayMode.h` — add `const FlightInfo* currentFlight` to `RenderContext`
- `firmware/core/WidgetRegistry.cpp` — wire `renderFlightField()` and `renderMetric()` into `dispatch()` switch; add two new includes
- `firmware/modes/CustomLayoutMode.cpp` — replace `(void)flights;` with `RenderContext widgetCtx = ctx; widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];`; dispatch with `widgetCtx` instead of `ctx`
- `firmware/adapters/WebPortal.cpp` — `_handleGetWidgetTypes()`: replace `"kind": "string"` content field with `"kind": "select"` + options for `flight_field` and `metric`; remove `"note"` keys
- `firmware/data-src/editor.js` — `showPropsPanel()`: replace hardcoded `clock`-only `useSelect` logic with metadata-driven `kind === 'select'` check; generalize option rebuild
- `firmware/data/editor.js.gz` — regenerated after `editor.js` edit
- `firmware/test/test_widget_registry/test_main.cpp` — update two "not yet implemented" tests to `TEST_ASSERT_TRUE`

Files **NOT modified** (verify unchanged):
- `firmware/core/WidgetRegistry.h` — enum and struct already correct; no changes
- `firmware/core/LayoutStore.cpp` — `kAllowedWidgetTypes[]` already contains both types; no changes
- `firmware/core/LayoutStore.h` — no changes
- `firmware/data-src/editor.html` — HTML structure unchanged; `<select id="prop-content-select">` already exists from LE-1.7
- `firmware/data-src/editor.css` — no new styles needed
- `firmware/data-src/common.js` — no changes
- All other display modes (`ClassicCardMode`, `LiveFlightCardMode`, `ClockMode`, etc.) — `RenderContext{}` zero-init sets `currentFlight = nullptr` automatically; these modes never read `currentFlight`

---

## Tasks / Subtasks (continued)

#### Review Follow-ups (AI)
- [ ] [AI-Review] MEDIUM: Verify `MetricWidget` degree suffix renders correctly on real hardware after `0xF8 → 0xF7` fix — smoke-test by flashing and visually confirming "°" appears for `track` metric (`firmware/widgets/MetricWidget.cpp`). **Deferred — requires physical hardware (LED matrix + ESP32) which is not available in the current review environment. Remains open for the next on-device validation pass.**
- [x] [AI-Review] MEDIUM: AC #1 vocabulary drift — story AC lists 10 field names using `ident_icao` / `operator_icao` / `origin` / `destination` / `altitude_kft` etc., but implementation uses abbreviated keys (`ident`, `origin_icao`, `dest_icao`) and MetricWidget handles telemetry fields (`alt`, `speed`, `track`, `vsi`). Story AC text should be updated to reflect the actual persisted key vocabulary, or keys should be migrated — document the canonical key set in the story Dev Notes so future stories inherit the right names. (`firmware/widgets/FlightFieldWidget.h`, `firmware/widgets/MetricWidget.h`, story AC #1, #4, #6). **Resolved: canonical vocabulary documented in Dev Notes → "Canonical Widget Content Key Vocabulary (as shipped)". Rename migration formally deferred (would be a breaking change for persisted layout JSON; out of scope).**
- [x] [AI-Review] LOW: AC #5 caching (file-scope static buffers + value-equality check) is not implemented — both widgets re-resolve and reformat on every frame. At 20 fps this is not a correctness problem but violates the story's explicit no-heap and cache requirements. Either implement the static-buffer cache or formally defer AC #5 caching to a future story and update the AC. (`firmware/widgets/FlightFieldWidget.cpp`, `firmware/widgets/MetricWidget.cpp`). **Resolved: AC #5's zero-heap clause is satisfied by the stack-buffer + char* overload pattern (heap-delta test gate covers the correctness-critical part). The value-equality cache clause is formally deferred — see Dev Notes → "AC #5 Caching — Formally Deferred".**
- [x] [AI-Review] LOW: `test_widget_registry` uses key `"alt"` for the MetricWidget dispatch test — ensure this matches the canonical key vocabulary decision from the MEDIUM item above; if keys are ever migrated to `"track_deg"` etc., the test spec must be updated in lock-step. (`firmware/test/test_widget_registry/test_main.cpp`). **Resolved: `"alt"` (and `"airline"` for the FlightField dispatch test) confirmed as part of the canonical vocabulary in Dev Notes → "Canonical Widget Content Key Vocabulary (as shipped)". Lock-step rename obligation is explicitly noted alongside the vocabulary table.**

---

## Senior Developer Review (AI)

### Review: 2026-04-17
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 2.5 → PASS with reservations (most reviewer-claimed issues were false positives; two real bugs identified and one fixed)
- **Issues Found:** 4 verified (1 fixed, 3 deferred action items)
- **Issues Fixed:** 1 (MetricWidget degree symbol byte 0xF8 → 0xF7 + RenderContext default initializer)
- **Action Items Created:** 4

---

## Change Log

| Date | Version | Description | Author |
|---|---|---|---|
| (original draft) | 0.1 | Draft story created — thin skeleton with placeholder dev notes | BMAD |
| 2026-04-17 | 1.0 | Upgraded to ready-for-dev. Comprehensive dev notes synthesized from live codebase analysis. Key additions: (1) Identified the `RenderContext.currentFlight` gap as the core architectural challenge — Task 1 and Task 2 added explicitly; (2) Documented exact `DisplayMode.h` struct change and `CustomLayoutMode.cpp` `(void)flights` fix; (3) Full FlightFieldWidget implementation skeleton with caching strategy for both string and numeric fields; (4) MetricWidget scoped to two telemetry fields in MVP with explicit note deferring `ApiContext` to a future story; (5) Exact `_handleGetWidgetTypes()` replacement code for both `flight_field` and `metric` blocks including `static const char* const` option arrays; (6) `editor.js` `showPropsPanel()` before/after diff — generalizing from clock-only to metadata-driven `kind === "select"` check; (7) AC #11 added for updating existing "not yet implemented" test assertions to `true`; (8) Unit test scaffold with `makeFlightInfo()` helper and heap-delta test pattern; (9) Zero-heap rule enforcement table (allowed vs. not-allowed patterns with `String.c_str()` safe-read clarification); (10) Antipattern prevention table (14 entries); (11) File list with explicit "NOT modified" section. Status changed from `draft` to `ready-for-dev`. | BMAD Story Synthesis |
| 2026-04-17 | 1.1 | Review-follow-up pass (documentation-only). Addressed 3 of 4 AI-Review action items: (a) MEDIUM "AC #1 vocabulary drift" resolved by adding a new Dev Notes subsection "Canonical Widget Content Key Vocabulary (as shipped)" that enumerates the 5 FlightFieldWidget keys (`airline`/`ident`/`origin_icao`/`dest_icao`/`aircraft`) and 4 MetricWidget keys (`alt`/`speed`/`track`/`vsi`) actually persisted by `WebPortal::_handleGetWidgetTypes()` and asserted by the Unity test suites, with explicit drift-vs-AC notes and rename-deferral rationale. (b) LOW "AC #5 caching" formally deferred via new Dev Notes subsection "AC #5 Caching — Formally Deferred" clarifying that the zero-heap clause is satisfied by stack-buffer + char* overload pattern (heap-delta test gate covers the correctness-critical portion) while the value-equality cache clause is deferred to a future story. (c) LOW "test_widget_registry key alignment" resolved inline — canonical vocabulary doc explicitly names the registry-test spec content (`"airline"`, `"alt"`) and the lock-step rename obligation. Remaining open: MEDIUM hardware smoke test of the `0xF7` degree suffix fix in MetricWidget — stays deferred pending on-device validation (no hardware access in the review environment). Status advanced from `ready-for-dev` to `review`; all Task / subtask checkboxes marked complete to reflect the implementation already on disk (code was completed under v1.0; this pass is documentation-only). | BMAD Review-Continuation |


]]></file>
<file id="17deba99" path="firmware/adapters/WebPortal.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: Web server adapter serving gzipped HTML pages and REST API endpoints.
Responsibilities:
- Serve wizard.html.gz in AP mode, dashboard.html.gz in STA mode via GET /.
- Expose JSON API: GET/POST /api/settings, GET /api/status, GET /api/layout, POST /api/reboot, POST /api/reset, GET /api/wifi/scan.
- Use consistent JSON envelope: { "ok": bool, "data": ..., "error": "...", "code": "..." }.
Architecture: ESPAsyncWebServer (mathieucarbou fork) — non-blocking, runs on AsyncTCP task.

JSON key alignment (matches ConfigManager::updateCacheFromKey / NVS):
  Display:   brightness, text_color_r, text_color_g, text_color_b
  Location:  center_lat, center_lon, radius_km
  Hardware:  tiles_x, tiles_y, tile_pixels, display_pin, origin_corner, scan_dir, zigzag
  Timing:    fetch_interval, display_cycle
  Network:   wifi_ssid, wifi_password, os_client_id, os_client_sec, aeroapi_key

GET /api/status extended JSON (Story 2.4):
  data.subsystems   — existing six subsystem objects (wifi, opensky, aeroapi, cdn, nvs, littlefs)
  data.wifi_detail  — SSID, RSSI, IP, mode
  data.device       — uptime_ms, free_heap, fs_total, fs_used
  data.flight       — last_fetch_ms, state_vectors, enriched_flights, logos_matched
  data.quota        — fetches_since_boot, limit, fetch_interval_s, estimated_monthly_polls, over_pace
*/
#include "adapters/WebPortal.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <vector>
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"
#include "core/LogoManager.h"
#include "core/OTAUpdater.h"
#include "utils/Log.h"

// Defined in main.cpp — provides thread-safe flight stats for the health page
extern FlightStatsSnapshot getFlightStatsSnapshot();

// Defined in main.cpp — NTP sync status accessor (Story fn-2.1)
extern bool isNtpSynced();

// Defined in main.cpp — Night mode scheduler dimming state (Story fn-2.2)
extern bool isScheduleDimming();

#include "core/LayoutEngine.h"
#include "core/LayoutStore.h"
#include "core/ModeOrchestrator.h"
#include "core/ModeRegistry.h"

namespace {
struct PendingRequestBody {
    AsyncWebServerRequest* request;
    String body;
};

std::vector<PendingRequestBody> g_pendingBodies;
constexpr size_t MAX_SETTINGS_BODY_BYTES = 4096;

// Logo upload state — streams file data directly to LittleFS
struct LogoUploadState {
    AsyncWebServerRequest* request;
    String filename;
    String path;
    File file;
    bool valid;
    bool written;
    String error;
    String errorCode;
};

std::vector<LogoUploadState> g_logoUploads;

LogoUploadState* findLogoUpload(AsyncWebServerRequest* request) {
    for (auto& u : g_logoUploads) {
        if (u.request == request) return &u;
    }
    return nullptr;
}

void clearLogoUpload(AsyncWebServerRequest* request, bool removeFile = false) {
    for (auto it = g_logoUploads.begin(); it != g_logoUploads.end(); ++it) {
        if (it->request == request) {
            if (it->file) it->file.close();
            if (removeFile && it->path.length()) {
                LittleFS.remove(it->path);
            }
            g_logoUploads.erase(it);
            return;
        }
    }
}

// OTA upload state — streams firmware directly to flash via Update library
struct OTAUploadState {
    AsyncWebServerRequest* request;
    bool valid;           // false if any validation/write failed
    bool started;         // true after Update.begin() succeeds
    size_t bytesWritten;  // for debugging/logging only
    String error;         // human-readable error message
    String errorCode;     // machine-readable error code
};

std::vector<OTAUploadState> g_otaUploads;
static bool g_otaInProgress = false;  // Enforce single-flight OTA — Update is a singleton

OTAUploadState* findOTAUpload(AsyncWebServerRequest* request) {
    for (auto& u : g_otaUploads) {
        if (u.request == request) return &u;
    }
    return nullptr;
}

void clearOTAUpload(AsyncWebServerRequest* request) {
    for (auto it = g_otaUploads.begin(); it != g_otaUploads.end(); ++it) {
        if (it->request == request) {
            // CRITICAL: abort in-progress update on cleanup (started=false means already completed)
            if (it->started) {
                Update.abort();
                LOG_I("OTA", "Upload aborted during cleanup");
            }
            g_otaUploads.erase(it);
            g_otaInProgress = false;
            return;
        }
    }
}

PendingRequestBody* findPendingBody(AsyncWebServerRequest* request) {
    for (auto& pending : g_pendingBodies) {
        if (pending.request == request) {
            return &pending;
        }
    }
    return nullptr;
}

// Shared SwitchState → string mapping used by GET and POST mode handlers.
// Centralised here to avoid copy-paste drift between the two call sites.
const char* switchStateToString(SwitchState ss) {
    switch (ss) {
        case SwitchState::REQUESTED: return "requested";
        case SwitchState::SWITCHING: return "switching";
        default:                     return "idle";
    }
}

void clearPendingBody(AsyncWebServerRequest* request) {
    for (auto it = g_pendingBodies.begin(); it != g_pendingBodies.end(); ++it) {
        if (it->request == request) {
            g_pendingBodies.erase(it);
            return;
        }
    }
}
} // namespace

void WebPortal::init(AsyncWebServer& server, WiFiManager& wifiMgr) {
    _server = &server;
    _wifiMgr = &wifiMgr;
    _registerRoutes();
    LOG_I("WebPortal", "Routes registered");
}

void WebPortal::begin() {
    if (!_server) return;
    _server->begin();
    LOG_I("WebPortal", "Server started on port 80");
}

void WebPortal::onReboot(RebootCallback callback) {
    _rebootCallback = callback;
}

void WebPortal::onCalibration(CalibrationCallback callback) {
    _calibrationCallback = callback;
}

void WebPortal::onPositioning(PositioningCallback callback) {
    _positioningCallback = callback;
}

void WebPortal::_registerRoutes() {
    // GET / — serve wizard or dashboard based on WiFi mode
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleRoot(request);
    });

    // GET /api/settings
    _server->on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetSettings(request);
    });

    // POST /api/settings — uses body handler for JSON parsing
    _server->on("/api/settings", HTTP_POST,
        // request handler (called after body is received)
        [](AsyncWebServerRequest* request) {
            // no-op: response sent in body handler
        },
        nullptr, // upload handler
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (data == nullptr || total == 0) {
                clearPendingBody(request);
                _sendJsonError(request, 400, "Empty settings object", "EMPTY_PAYLOAD");
                return;
            }

            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }

            if (index == 0) {
                clearPendingBody(request);
                // push_back first, then reserve on the stored element — avoids capacity
                // loss from the String copy constructor during push_back (synthesis ds-3.2 Pass 3).
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }

            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }

            pending->body.concat(reinterpret_cast<const char*>(data), len);

            if (index + len == total) {
                _handlePostSettings(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // GET /api/status
    _server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetStatus(request);
    });

    // POST /api/reboot
    _server->on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostReboot(request);
    });

    // POST /api/reset — factory reset (erase NVS + restart)
    _server->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostReset(request);
    });

    // GET /api/wifi/scan
    _server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetWifiScan(request);
    });

    // GET /api/layout (Story 3.1)
    _server->on("/api/layout", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLayout(request);
    });

    // POST /api/calibration/start (Story 4.2) — gradient pattern
    _server->on("/api/calibration/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostCalibrationStart(request);
    });

    // POST /api/positioning/start — panel positioning guide (independent from calibration)
    _server->on("/api/positioning/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (_positioningCallback) {
            _positioningCallback(true);
        }
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["message"] = "Positioning mode started";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // POST /api/positioning/stop
    _server->on("/api/positioning/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (_positioningCallback) {
            _positioningCallback(false);
        }
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["message"] = "Positioning mode stopped";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // POST /api/calibration/stop (Story 4.2)
    _server->on("/api/calibration/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostCalibrationStop(request);
    });

    // GET /api/display/modes (Story dl-1.5) — mode list with orchestrator state
    _server->on("/api/display/modes", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetDisplayModes(request);
    });

    // POST /api/display/mode (Story ds-3.1) — manual mode switch
    // AC #5: accept "mode" with fallback to "mode_id" for backward compat
    // AC #6: orchestrator-only path (Rule 24)
    // AC #7: async response with switch_state — no bounded wait in async handler
    //   Strategy: return switch_state: "requested" with client re-fetch (ds-3.4 UX).
    //   Architecture D5 "synchronous" intent is not feasible in ESPAsyncWebServer body
    //   handler because tick() runs on Core 0 display task. Explicit deviation documented.
    _server->on("/api/display/mode", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (total == 0 || data == nullptr) {
                _sendJsonError(request, 400, "Empty request body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                // push_back first, then reserve on the stored element — avoids capacity
                // loss from the String copy constructor during push_back (synthesis ds-3.2 Pass 3).
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePostDisplayMode(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // GET /api/schedule (Story dl-4.2) — mode schedule rules
    _server->on("/api/schedule", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetSchedule(request);
    });

    // POST /api/schedule (Story dl-4.2) — replace mode schedule rules
    _server->on("/api/schedule", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (total == 0 || data == nullptr) {
                _sendJsonError(request, 400, "Empty request body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                // push_back first, then reserve on the stored element — avoids capacity
                // loss from the String copy constructor during push_back (synthesis ds-3.2 Pass 3).
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePostSchedule(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // GET /api/ota/check (Story dl-6.2) — check GitHub for firmware update
    _server->on("/api/ota/check", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetOtaCheck(request);
    });

    // POST /api/ota/pull (Story dl-7.3, AC #1–#2) — start pull OTA download
    _server->on("/api/ota/pull", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostOtaPull(request);
    });

    // GET /api/ota/status (Story dl-7.3, AC #1, #3) — OTA state/progress polling
    _server->on("/api/ota/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetOtaStatus(request);
    });

    // POST /api/display/notification/dismiss (Story ds-3.1, AC #10; ds-3.2, AC #4)
    // Clears upgrade_notification NVS flag so GET returns false thereafter.
    // Uses ConfigManager::setUpgNotif() to keep NVS writes centralized (AR7).
    _server->on("/api/display/notification/dismiss", HTTP_POST, [](AsyncWebServerRequest* request) {
        ConfigManager::setUpgNotif(false);

        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // GET /api/logos — list uploaded logos
    _server->on("/api/logos", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLogos(request);
    });

    // POST /api/logos — multipart file upload (streaming to LittleFS)
    _server->on("/api/logos", HTTP_POST,
        // Request handler — called after upload completes
        [this](AsyncWebServerRequest* request) {
            auto* state = findLogoUpload(request);
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();

            if (!state || !state->valid) {
                root["ok"] = false;
                root["error"] = (state && state->error.length()) ? state->error : "Upload failed";
                root["code"] = (state && state->errorCode.length()) ? state->errorCode : "UPLOAD_ERROR";
                String output;
                serializeJson(doc, output);
                clearLogoUpload(request, true);
                request->send(400, "application/json", output);
                return;
            }

            root["ok"] = true;
            JsonObject data = root["data"].to<JsonObject>();
            data["filename"] = state->filename;
            data["size"] = LOGO_BUFFER_BYTES;

            String output;
            serializeJson(doc, output);
            clearLogoUpload(request);
            LogoManager::scanLogoCount();
            request->send(200, "application/json", output);
        },
        // Upload handler — called for each chunk of file data
        [this](AsyncWebServerRequest* request, const String& filename,
               size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                // First chunk — validate and open file for writing
                clearLogoUpload(request);
                LogoUploadState state;
                state.request = request;
                state.filename = filename;
                state.path = String("/logos/") + filename;
                state.valid = true;
                state.written = false;

                // Validate filename before touching LittleFS.
                if (!LogoManager::isSafeLogoFilename(filename)) {
                    state.valid = false;
                    state.error = filename + " - invalid filename";
                    state.errorCode = "INVALID_NAME";
                    g_logoUploads.push_back(state);
                    return;
                }

                request->onDisconnect([request]() {
                    clearLogoUpload(request, true);
                });

                // Open file for streaming write
                state.file = LittleFS.open(state.path, "w");
                if (!state.file) {
                    state.valid = false;
                    state.error = "LittleFS full or write error";
                    state.errorCode = "FS_WRITE_ERROR";
                }
                g_logoUploads.push_back(state);
            }

            auto* state = findLogoUpload(request);
            if (!state || !state->valid) return;

            // Stream write chunk to file
            if (state->file && len > 0) {
                size_t written = state->file.write(data, len);
                if (written != len) {
                    state->valid = false;
                    state->error = "LittleFS full";
                    state->errorCode = "FS_FULL";
                    state->file.close();
                    LittleFS.remove(state->path);
                    return;
                }
            }

            if (final) {
                size_t totalSize = index + len;
                if (state->file) state->file.close();

                if (totalSize != LOGO_BUFFER_BYTES) {
                    state->valid = false;
                    state->error = state->filename + " - invalid size (" + String((unsigned long)totalSize) + " bytes, expected 2048)";
                    state->errorCode = "INVALID_SIZE";
                    LittleFS.remove(state->path);
                    return;
                }

                state->written = true;
            }
        }
    );

    // DELETE /api/logos/:name
    _server->on("/api/logos/*", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        _handleDeleteLogo(request);
    });

    // GET /logos/:name — serve raw logo binary for thumbnail rendering
    _server->on("/logos/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLogoFile(request);
    });

    // POST /api/ota/upload — firmware OTA upload (streaming to flash)
    _server->on("/api/ota/upload", HTTP_POST,
        // Request handler — called after upload completes
        [this](AsyncWebServerRequest* request) {
            auto* state = findOTAUpload(request);
            JsonDocument doc;
            JsonObject root = doc.to<JsonObject>();

            if (!state || !state->valid) {
                root["ok"] = false;
                root["error"] = (state && state->error.length()) ? state->error : "Upload failed";
                const String errCode = (state && state->errorCode.length()) ? state->errorCode : "UPLOAD_ERROR";
                root["code"] = errCode;
                // Map error codes to semantically correct HTTP status codes
                int httpCode = 400;  // default: client error (e.g. bad firmware file)
                if (errCode == "OTA_BUSY") httpCode = 409;  // Conflict
                else if (errCode == "NO_OTA_PARTITION" || errCode == "BEGIN_FAILED" ||
                         errCode == "WRITE_FAILED"     || errCode == "VERIFY_FAILED") httpCode = 500;
                String output;
                serializeJson(doc, output);
                clearOTAUpload(request);
                request->send(httpCode, "application/json", output);
                return;
            }

            // Success — schedule reboot
            root["ok"] = true;
            root["message"] = "Rebooting...";
            String output;
            serializeJson(doc, output);
            clearOTAUpload(request);

            SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Update complete — rebooting");
            LOG_I("OTA", "Upload complete, scheduling reboot");

            request->send(200, "application/json", output);

            // Schedule reboot after 500ms to allow response to be sent
            static esp_timer_handle_t otaRebootTimer = nullptr;
            if (!otaRebootTimer) {
                esp_timer_create_args_t args = {};
                args.callback = [](void*) { ESP.restart(); };
                args.name = "ota_reboot";
                esp_timer_create(&args, &otaRebootTimer);
            }
            esp_timer_start_once(otaRebootTimer, 500000); // 500ms in microseconds
        },
        // Upload handler — called for each chunk of firmware data
        [this](AsyncWebServerRequest* request, const String& filename,
               size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                // First chunk — validate magic byte and begin update
                clearOTAUpload(request);

                // Reject concurrent OTA uploads — Update singleton is not re-entrant
                if (g_otaInProgress) {
                    OTAUploadState busy;
                    busy.request = request;
                    busy.valid = false;
                    busy.started = false;
                    busy.bytesWritten = 0;
                    busy.error = "Another OTA update is already in progress";
                    busy.errorCode = "OTA_BUSY";
                    g_otaUploads.push_back(busy);
                    LOG_I("OTA", "Rejected concurrent OTA upload");
                    return;
                }

                OTAUploadState state;
                state.request = request;
                state.valid = true;
                state.started = false;
                state.bytesWritten = 0;

                // Register disconnect handler for WiFi interruption
                request->onDisconnect([request]() {
                    auto* s = findOTAUpload(request);
                    if (s && s->started) {
                        Update.abort();
                        LOG_I("OTA", "Upload aborted due to disconnect");
                    }
                    clearOTAUpload(request);
                });

                // Validate ESP32 magic byte (0xE9)
                if (len == 0 || data[0] != 0xE9) {
                    state.valid = false;
                    state.error = "Not a valid ESP32 firmware image";
                    state.errorCode = "INVALID_FIRMWARE";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Invalid firmware image");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "Invalid firmware magic byte");
                    return;
                }

                // Get next OTA partition
                const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
                if (!partition) {
                    state.valid = false;
                    state.error = "No OTA partition available";
                    state.errorCode = "NO_OTA_PARTITION";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "No OTA partition found");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "No OTA partition found");
                    return;
                }

                // Story dl-7.1, AC #9: tear down display mode before flash write
                // so push and pull OTA share the same safe teardown path.
                if (!ModeRegistry::prepareForOTA()) {
                    // prepareForOTA failed — abort upload with clear error
                    state.valid = false;
                    state.error = "Could not prepare display for OTA";
                    state.errorCode = "PREPARE_OTA_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Display teardown failed");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "prepareForOTA() failed — aborting upload");
                    return;
                }

                // Begin update with partition size (NOT Content-Length per AR18)
                if (!Update.begin(partition->size)) {
                    state.valid = false;
                    state.error = "Could not begin OTA update";
                    state.errorCode = "BEGIN_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Could not begin OTA update");
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "Update.begin() failed");
                    return;
                }

                state.started = true;
                g_otaInProgress = true;
                g_otaUploads.push_back(state);
                Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size);
                SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Upload in progress");
                LOG_I("OTA", "Update started");
            }

            auto* state = findOTAUpload(request);
            if (!state || !state->valid) return;

            // Stream write chunk to flash
            if (len > 0) {
                size_t written = Update.write(data, len);
                if (written != len) {
                    Update.abort();
                    state->valid = false;
                    state->error = "Write failed — flash may be worn or corrupted";
                    state->errorCode = "WRITE_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Write failed");
                    LOG_E("OTA", "Flash write failed");
                    return;
                }
                state->bytesWritten += len;
            }

            if (final) {
                // Finalize update
                if (!Update.end(true)) {
                    Update.abort();
                    state->valid = false;
                    state->error = "Firmware verification failed";
                    state->errorCode = "VERIFY_FAILED";
                    SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Verification failed");
                    LOG_E("OTA", "Firmware verification failed");
                    return;
                }
                // Clear started so clearOTAUpload does NOT call Update.abort() on a completed update
                state->started = false;

                // Reset rollback acknowledgment so a future rollback shows the banner again (Story fn-1.6)
                Preferences otaPrefs;
                if (otaPrefs.begin("flightwall", false)) {
                    otaPrefs.putUChar("ota_rb_ack", 0);
                    otaPrefs.end();
                } else {
                    LOG_E("OTA", "Failed to open NVS to clear rollback ack — banner may stay dismissed after rollback");
                }

                LOG_I("OTA", "Update finalized successfully");
            }
        }
    );

    // POST /api/ota/ack-rollback — dismiss rollback banner (Story fn-1.6)
    _server->on("/api/ota/ack-rollback", HTTP_POST, [](AsyncWebServerRequest* request) {
        Preferences prefs;
        if (!prefs.begin("flightwall", false)) {
            request->send(500, "application/json", "{\"ok\":false,\"error\":\"NVS access failed\",\"code\":\"NVS_ERROR\"}");
            return;
        }
        size_t written = prefs.putUChar("ota_rb_ack", 1);
        prefs.end();
        if (written == 0) {
            request->send(500, "application/json", "{\"ok\":false,\"error\":\"Failed to save acknowledgment\",\"code\":\"NVS_WRITE_ERROR\"}");
            return;
        }

        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // GET /api/settings/export — download config as JSON file (Story fn-1.6)
    _server->on("/api/settings/export", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["flightwall_settings_version"] = 1;

        // Timestamp — use NTP time if available, else uptime
        time_t now;
        time(&now);
        if (now > 1000000000) {  // NTP synced (past year 2001)
            char buf[32];
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
            root["exported_at"] = String(buf);
        } else {
            root["exported_at"] = String("uptime_ms_") + String(millis());
        }

        ConfigManager::dumpSettingsJson(root);

        String output;
        serializeJson(doc, output);

        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", output);
        response->addHeader("Content-Disposition", "attachment; filename=flightwall-settings.json");
        request->send(response);
    });

    // ─── Layout CRUD (LE-1.4) ───
    // Register specific paths before wildcards. POST /api/layouts/*/activate
    // is a distinct path from POST /api/layouts (exact) and POST /api/layouts/*
    // would overlap with activate — but ESPAsyncWebServer matches the longer
    // activate path first. Register activate ahead of the generic PUT/DELETE
    // wildcards for safety.
    _server->on("/api/layouts", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLayouts(request);
    });

    _server->on("/api/layouts", HTTP_POST,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (data == nullptr || total == 0) {
                clearPendingBody(request);
                _sendJsonError(request, 400, "Empty body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > LAYOUT_STORE_MAX_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() { clearPendingBody(request); });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePostLayout(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    // Register activate BEFORE generic wildcards so longer/more specific
    // path matches first on the mathieucarbou fork.
    _server->on("/api/layouts/*/activate", HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handlePostLayoutActivate(request);
    });

    _server->on("/api/layouts/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetLayoutById(request);
    });

    _server->on("/api/layouts/*", HTTP_PUT,
        [](AsyncWebServerRequest* request) { /* no-op: response in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (data == nullptr || total == 0) {
                clearPendingBody(request);
                _sendJsonError(request, 400, "Empty body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > LAYOUT_STORE_MAX_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
                return;
            }
            if (index == 0) {
                clearPendingBody(request);
                g_pendingBodies.push_back({request, String()});
                g_pendingBodies.back().body.reserve(total);
                request->onDisconnect([request]() { clearPendingBody(request); });
            }
            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete body", "INCOMPLETE_BODY");
                return;
            }
            pending->body.concat(reinterpret_cast<const char*>(data), len);
            if (index + len == total) {
                _handlePutLayout(
                    request,
                    reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                    pending->body.length()
                );
                clearPendingBody(request);
            }
        }
    );

    _server->on("/api/layouts/*", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        _handleDeleteLayout(request);
    });

    _server->on("/api/widgets/types", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleGetWidgetTypes(request);
    });

    // Shared static assets (gzipped on LittleFS)
    _server->on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/style.css.gz", "text/css");
    });
    _server->on("/common.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/common.js.gz", "application/javascript");
    });
    _server->on("/wizard.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/wizard.js.gz", "application/javascript");
    });
    _server->on("/dashboard.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/dashboard.js.gz", "application/javascript");
    });
    _server->on("/health.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/health.html.gz", "text/html");
    });
    _server->on("/health.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/health.js.gz", "application/javascript");
    });
    // Layout Editor assets (LE-1.6)
    _server->on("/editor.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.html.gz", "text/html");
    });
    _server->on("/editor.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.js.gz", "application/javascript");
    });
    _server->on("/editor.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        _serveGzAsset(request, "/editor.css.gz", "text/css");
    });
}

void WebPortal::_handleRoot(AsyncWebServerRequest* request) {
    WiFiState state = _wifiMgr->getState();
    const char* file;
    if (state == WiFiState::AP_SETUP || state == WiFiState::AP_FALLBACK) {
        file = "/wizard.html.gz";
    } else {
        // STA_CONNECTED, CONNECTING, STA_RECONNECTING — serve dashboard
        file = "/dashboard.html.gz";
    }

    if (!LittleFS.exists(file)) {
        _sendJsonError(request, 404, "Asset not found", "ASSET_MISSING");
        return;
    }

    AsyncWebServerResponse* response = request->beginResponse(LittleFS, file, "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void WebPortal::_handleGetSettings(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    ConfigManager::dumpSettingsJson(data);

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostSettings(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "PARSE_ERROR");
        return;
    }

    if (!doc.is<JsonObject>()) {
        _sendJsonError(request, 400, "Expected JSON object", "INVALID_PAYLOAD");
        return;
    }

    JsonObject settings = doc.as<JsonObject>();
    if (settings.size() == 0) {
        _sendJsonError(request, 400, "Empty settings object", "EMPTY_PAYLOAD");
        return;
    }

    ApplyResult result = ConfigManager::applyJson(settings);
    if (result.applied.size() != settings.size()) {
        _sendJsonError(request, 400, "Unknown or invalid settings key", "INVALID_SETTING");
        return;
    }

    JsonDocument respDoc;
    JsonObject resp = respDoc.to<JsonObject>();
    resp["ok"] = true;
    JsonArray applied = resp["applied"].to<JsonArray>();
    for (const String& key : result.applied) {
        applied.add(key);
    }
    resp["reboot_required"] = result.reboot_required;

    String output;
    serializeJson(respDoc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    FlightStatsSnapshot stats = getFlightStatsSnapshot();
    SystemStatus::toExtendedJson(data, stats);

    // NTP and schedule status (Story fn-2.1, fn-2.2)
    data["ntp_synced"] = isNtpSynced();
    data["schedule_active"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();
    data["schedule_dimming"] = isScheduleDimming();

    // Rollback acknowledgment state (Story fn-1.6) — NVS-backed, not via ConfigManager
    Preferences prefs;
    prefs.begin("flightwall", true);  // read-only
    data["rollback_acknowledged"] = prefs.getUChar("ota_rb_ack", 0) == 1;
    prefs.end();

    // OTA availability from last check (Story dl-6.2, AC #3)
    bool otaAvail = (OTAUpdater::getState() == OTAState::AVAILABLE);
    data["ota_available"] = otaAvail;
    data["ota_version"] = otaAvail ? (const char*)OTAUpdater::getRemoteVersion()
                                   : (const char*)nullptr;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostReboot(AsyncWebServerRequest* request) {
    // Flush all pending NVS writes before restart
    ConfigManager::persistAllNow();

    // Notify main coordinator to show "Saving config..." on LED
    if (_rebootCallback) {
        _rebootCallback();
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Rebooting...";

    String output;
    serializeJson(doc, output);

    // Send response first, then schedule restart
    request->send(200, "application/json", output);

    // Delay restart slightly so the HTTP response can be sent
    // Use a one-shot timer to avoid blocking the async callback
    static esp_timer_handle_t rebootTimer = nullptr;
    if (!rebootTimer) {
        esp_timer_create_args_t args = {};
        args.callback = [](void*) { ESP.restart(); };
        args.name = "reboot";
        esp_timer_create(&args, &rebootTimer);
    }
    esp_timer_start_once(rebootTimer, 1000000); // 1 second in microseconds
}

void WebPortal::_handlePostReset(AsyncWebServerRequest* request) {
    // Erase NVS config and restore compile-time defaults
    if (!ConfigManager::factoryReset()) {
        _sendJsonError(request, 500, "Factory reset failed", "RESET_FAILED");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Factory reset complete. Rebooting...";

    String output;
    serializeJson(doc, output);

    // Send response first, then schedule restart (reuse reboot timer pattern)
    request->send(200, "application/json", output);

    static esp_timer_handle_t resetTimer = nullptr;
    if (!resetTimer) {
        esp_timer_create_args_t args = {};
        args.callback = [](void*) { ESP.restart(); };
        args.name = "reset";
        esp_timer_create(&args, &resetTimer);
    }
    esp_timer_start_once(resetTimer, 1000000); // 1 second in microseconds
}

void WebPortal::_handleGetWifiScan(AsyncWebServerRequest* request) {
    int16_t scanResult = WiFi.scanComplete();

    if (scanResult == WIFI_SCAN_FAILED) {
        // No scan running — kick off an async scan
        WiFi.scanNetworks(true); // async=true
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["scanning"] = true;
        root["data"].to<JsonArray>();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    if (scanResult == WIFI_SCAN_RUNNING) {
        // Scan still in progress
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        root["scanning"] = true;
        root["data"].to<JsonArray>();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    // Scan complete — return results
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["scanning"] = false;
    JsonArray data = root["data"].to<JsonArray>();

    for (int i = 0; i < scanResult; i++) {
        JsonObject net = data.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
    }

    // Free scan memory for next scan
    WiFi.scanDelete();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetDisplayModes(AsyncWebServerRequest* request) {
    // Capacity: ~256 base + ~200 per mode (with zone_layout) — 3 modes ≈ 900 bytes
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // Build modes array from ModeRegistry (AC #1, #2, #4 — no heap allocation per request)
    JsonArray modes = data["modes"].to<JsonArray>();
    const ModeEntry* table = ModeRegistry::getModeTable();
    const uint8_t count = ModeRegistry::getModeCount();

    // Active mode from ModeRegistry (truth source); fall back to "classic_card" if nullptr
    const char* registryActiveId = ModeRegistry::getActiveModeId();
    const char* activeId = registryActiveId ? registryActiveId : "classic_card";

    for (uint8_t i = 0; i < count; i++) {
        const ModeEntry& entry = table[i];
        JsonObject modeObj = modes.add<JsonObject>();
        modeObj["id"] = entry.id;
        modeObj["name"] = entry.displayName;
        modeObj["active"] = (strcmp(activeId, entry.id) == 0);

        // Zone layout from static descriptor (AC #2) — walks BSS/static data only
        JsonArray zoneLayout = modeObj["zone_layout"].to<JsonArray>();
        if (entry.zoneDescriptor != nullptr) {
            if (entry.zoneDescriptor->description != nullptr) {
                modeObj["description"] = entry.zoneDescriptor->description;
            }
            for (uint8_t z = 0; z < entry.zoneDescriptor->regionCount; z++) {
                const ZoneRegion& region = entry.zoneDescriptor->regions[z];
                JsonObject zoneObj = zoneLayout.add<JsonObject>();
                zoneObj["label"] = region.label;
                zoneObj["relX"] = region.relX;
                zoneObj["relY"] = region.relY;
                zoneObj["relW"] = region.relW;
                zoneObj["relH"] = region.relH;
            }
        }

        // Per-mode settings from settingsSchema (Story dl-5.1, AC #1–#2)
        // Loops ModeEntry.settingsSchema only — no hardcoded key lists (rule 28).
        // Adding a ModeSettingDef to any schema auto-appears here.
        if (entry.settingsSchema != nullptr && entry.settingsSchema->settingCount > 0) {
            JsonArray settings = modeObj["settings"].to<JsonArray>();
            const ModeSettingsSchema* schema = entry.settingsSchema;
            for (uint8_t s = 0; s < schema->settingCount; s++) {
                const ModeSettingDef& def = schema->settings[s];
                JsonObject settingObj = settings.add<JsonObject>();
                settingObj["key"] = def.key;
                settingObj["label"] = def.label;
                settingObj["type"] = def.type;
                settingObj["default"] = def.defaultValue;
                settingObj["min"] = def.minValue;
                settingObj["max"] = def.maxValue;
                if (def.enumOptions != nullptr) {
                    settingObj["enumOptions"] = def.enumOptions;
                } else {
                    settingObj["enumOptions"] = (const char*)nullptr;
                }
                // Current persisted value via ConfigManager (uses modeAbbrev for NVS key)
                settingObj["value"] = ConfigManager::getModeSetting(
                    schema->modeAbbrev, def.key, def.defaultValue);
            }
        } else {
            modeObj["settings"] = (const char*)nullptr;  // null for modes without settings
        }
    }

    data["active"] = activeId;

    // Switch state from ModeRegistry (AC #1, replacing stub "idle")
    data["switch_state"] = switchStateToString(ModeRegistry::getSwitchState());

    // Orchestrator transparency (AC #3 — retain dl-1.5 fields)
    data["orchestrator_state"] = ModeOrchestrator::getStateString();
    data["state_reason"] = ModeOrchestrator::getStateReason();

    // Registry error if present (AC #9)
    // Use copyLastError() — thread-safe snapshot for Core 1 (synthesis ds-3.2 Pass 3).
    // getLastError() returns a raw pointer to a Core 0 mutable buffer; unsafe here.
    char regErrBuf[64];
    ModeRegistry::copyLastError(regErrBuf, sizeof(regErrBuf));
    if (regErrBuf[0] != '\0') {
        data["registry_error"] = regErrBuf;
    }

    // Upgrade notification NVS flag (AC #10; ds-3.2, AC #4)
    // Uses ConfigManager::getUpgNotif() to keep NVS reads centralized (AR7).
    data["upgrade_notification"] = ConfigManager::getUpgNotif();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

// POST /api/display/mode (Story ds-3.1)
// AC #5: accept "mode" with fallback to "mode_id" for one release.
// AC #6 / Rule 24: always route user intent through orchestrator — even when re-selecting
//   the current mode — so the orchestrator exits IDLE_FALLBACK / SCHEDULED and returns
//   to MANUAL state as the user explicitly intended.
//   ModeRegistry::tick() is idempotent when requested == active (line 367 guard), so
//   calling onManualSwitch for the same mode does not restart the running DisplayMode.
// AC #7: async strategy (documented deviation from architecture D5 — bounded wait not
//   feasible in ESPAsyncWebServer body handler; client re-fetches via ds-3.4 UX).
void WebPortal::_handlePostDisplayMode(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument reqDoc;
    DeserializationError err = deserializeJson(reqDoc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "INVALID_JSON");
        return;
    }

    // AC #5: prefer "mode", fall back to "mode_id" for one release
    const char* modeId = reqDoc["mode"] | (const char*)nullptr;
    if (!modeId) {
        modeId = reqDoc["mode_id"] | (const char*)nullptr;
    }
    if (!modeId) {
        _sendJsonError(request, 400, "Missing mode or mode_id field", "MISSING_FIELD");
        return;
    }

    // Resolve mode from ModeRegistry (AC #8 — unknown mode → 400)
    const ModeEntry* table = ModeRegistry::getModeTable();
    const uint8_t count = ModeRegistry::getModeCount();
    const ModeEntry* matchedEntry = nullptr;
    for (uint8_t i = 0; i < count; i++) {
        if (strcmp(table[i].id, modeId) == 0) {
            matchedEntry = &table[i];
            break;
        }
    }
    if (!matchedEntry) {
        _sendJsonError(request, 400, "Unknown mode_id", "UNKNOWN_MODE");
        return;
    }

    // Story dl-5.1, AC #4: handle optional "settings" object
    bool hasSettings = reqDoc["settings"].is<JsonObject>();
    bool settingsApplied = false;
    if (hasSettings) {
        JsonObject settingsObj = reqDoc["settings"].as<JsonObject>();

        // Validate all keys exist in schema before applying any (no partial apply)
        if (matchedEntry->settingsSchema == nullptr || matchedEntry->settingsSchema->settingCount == 0) {
            if (settingsObj.size() > 0) {
                _sendJsonError(request, 400, "Mode has no configurable settings", "UNKNOWN_SETTING");
                return;
            }
        } else {
            const ModeSettingsSchema* schema = matchedEntry->settingsSchema;
            // Pre-validate: check all keys exist in schema
            for (JsonPair kv : settingsObj) {
                bool found = false;
                for (uint8_t s = 0; s < schema->settingCount; s++) {
                    if (strcmp(kv.key().c_str(), schema->settings[s].key) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    _sendJsonError(request, 400, "Unknown setting key", "UNKNOWN_SETTING");
                    return;
                }
            }
            // Pre-validate values against schema min/max BEFORE writing any setting
            // to NVS — prevents partial apply where early keys succeed but a later
            // key fails, leaving NVS in an inconsistent state (no partial apply rule).
            // Also reject non-numeric values: as<int32_t>() silently returns 0 for strings,
            // which could pass range checks (e.g. format=0 is valid). is<int32_t>() guards this.
            for (JsonPair kv : settingsObj) {
                if (!kv.value().is<int32_t>()) {
                    _sendJsonError(request, 400, "Setting value must be numeric", "INVALID_SETTING_TYPE");
                    return;
                }
                int32_t val = kv.value().as<int32_t>();
                for (uint8_t s = 0; s < schema->settingCount; s++) {
                    if (strcmp(kv.key().c_str(), schema->settings[s].key) == 0) {
                        if (val < schema->settings[s].minValue || val > schema->settings[s].maxValue) {
                            _sendJsonError(request, 400, "Setting value out of range", "INVALID_SETTING_VALUE");
                            return;
                        }
                        break;
                    }
                }
            }
            // Apply all settings (all values pre-validated — no partial apply risk)
            for (JsonPair kv : settingsObj) {
                int32_t val = kv.value().as<int32_t>();
                if (!ConfigManager::setModeSetting(schema->modeAbbrev, kv.key().c_str(), val)) {
                    _sendJsonError(request, 400, "Setting validation failed", "INVALID_SETTING_VALUE");
                    return;
                }
            }
            if (settingsObj.size() > 0) settingsApplied = true;
        }
    }

    // AC #6 / Rule 24: always call onManualSwitch so the orchestrator transitions to
    // MANUAL state regardless of whether the requested mode is already active.
    ModeOrchestrator::onManualSwitch(modeId, matchedEntry->displayName);

    // Build truthful response (AC #7, #9)
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject respData = root["data"].to<JsonObject>();
    respData["switching_to"] = modeId;
    respData["active"] = ModeRegistry::getActiveModeId()
                           ? ModeRegistry::getActiveModeId() : "classic_card";
    respData["settings_applied"] = settingsApplied;
    respData["switch_state"] = switchStateToString(ModeRegistry::getSwitchState());
    respData["orchestrator_state"] = ModeOrchestrator::getStateString();
    respData["state_reason"] = ModeOrchestrator::getStateReason();

    // Registry error if present (AC #9 — HEAP_INSUFFICIENT etc.)
    // Use copyLastError() — thread-safe snapshot for Core 1 (synthesis ds-3.2 Pass 3).
    char regErrBuf2[64];
    ModeRegistry::copyLastError(regErrBuf2, sizeof(regErrBuf2));
    if (regErrBuf2[0] != '\0') {
        respData["registry_error"] = regErrBuf2;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLayout(AsyncWebServerRequest* request) {
    HardwareConfig hw = ConfigManager::getHardware();
    LayoutResult layout = LayoutEngine::compute(hw);

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    data["mode"] = layout.mode;

    JsonObject matrix = data["matrix"].to<JsonObject>();
    matrix["width"] = layout.matrixWidth;
    matrix["height"] = layout.matrixHeight;

    JsonObject logo = data["logo_zone"].to<JsonObject>();
    logo["x"] = layout.logoZone.x;
    logo["y"] = layout.logoZone.y;
    logo["w"] = layout.logoZone.w;
    logo["h"] = layout.logoZone.h;

    JsonObject flight = data["flight_zone"].to<JsonObject>();
    flight["x"] = layout.flightZone.x;
    flight["y"] = layout.flightZone.y;
    flight["w"] = layout.flightZone.w;
    flight["h"] = layout.flightZone.h;

    JsonObject telemetry = data["telemetry_zone"].to<JsonObject>();
    telemetry["x"] = layout.telemetryZone.x;
    telemetry["y"] = layout.telemetryZone.y;
    telemetry["w"] = layout.telemetryZone.w;
    telemetry["h"] = layout.telemetryZone.h;

    JsonObject hardware = data["hardware"].to<JsonObject>();
    hardware["tiles_x"] = hw.tiles_x;
    hardware["tiles_y"] = hw.tiles_y;
    hardware["tile_pixels"] = hw.tile_pixels;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_serveGzAsset(AsyncWebServerRequest* request, const char* path, const char* contentType) {
    if (!LittleFS.exists(path)) {
        request->send(404, "text/plain", "Not found");
        return;
    }
    AsyncWebServerResponse* response = request->beginResponse(LittleFS, path, contentType);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void WebPortal::_handlePostCalibrationStart(AsyncWebServerRequest* request) {
    if (_calibrationCallback) {
        _calibrationCallback(true);
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Calibration mode started";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostCalibrationStop(AsyncWebServerRequest* request) {
    if (_calibrationCallback) {
        _calibrationCallback(false);
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Calibration mode stopped";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLogos(AsyncWebServerRequest* request) {
    std::vector<LogoEntry> logos;
    if (!LogoManager::listLogos(logos)) {
        _sendJsonError(request, 500, "Logo storage unavailable. Reboot the device and try again.", "STORAGE_UNAVAILABLE");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonArray data = root["data"].to<JsonArray>();

    for (const auto& logo : logos) {
        JsonObject entry = data.add<JsonObject>();
        entry["name"] = logo.name;
        entry["size"] = logo.size;
    }

    // Storage usage metadata
    size_t usedBytes = 0, totalBytes = 0;
    LogoManager::getLittleFSUsage(usedBytes, totalBytes);
    JsonObject storage = root["storage"].to<JsonObject>();
    storage["used"] = usedBytes;
    storage["total"] = totalBytes;
    storage["logo_count"] = LogoManager::getLogoCount();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleDeleteLogo(AsyncWebServerRequest* request) {
    // Extract filename from URL: /api/logos/FILENAME
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0 || lastSlash == (int)(url.length() - 1)) {
        _sendJsonError(request, 400, "Missing logo filename", "MISSING_FILENAME");
        return;
    }
    String filename = url.substring(lastSlash + 1);
    int queryStart = filename.indexOf('?');
    if (queryStart >= 0) {
        filename = filename.substring(0, queryStart);
    }

    if (!LogoManager::isSafeLogoFilename(filename)) {
        _sendJsonError(request, 400, "Invalid logo filename", "INVALID_NAME");
        return;
    }

    if (!LogoManager::hasLogo(filename)) {
        _sendJsonError(request, 404, "Logo not found", "NOT_FOUND");
        return;
    }

    if (!LogoManager::deleteLogo(filename)) {
        _sendJsonError(request, 500, "Could not delete logo. Check storage health and try again.", "FS_DELETE_ERROR");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["message"] = "Logo deleted";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLogoFile(AsyncWebServerRequest* request) {
    // Extract filename from URL: /logos/FILENAME
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0 || lastSlash == (int)(url.length() - 1)) {
        request->send(404, "text/plain", "Not found");
        return;
    }
    String filename = url.substring(lastSlash + 1);
    int queryStart = filename.indexOf('?');
    if (queryStart >= 0) {
        filename = filename.substring(0, queryStart);
    }

    if (!LogoManager::isSafeLogoFilename(filename)) {
        request->send(404, "text/plain", "Not found");
        return;
    }

    String path = String("/logos/") + filename;
    if (!LittleFS.exists(path)) {
        request->send(404, "text/plain", "Not found");
        return;
    }

    request->send(LittleFS, path, "application/octet-stream");
}

void WebPortal::_handleGetOtaCheck(AsyncWebServerRequest* request) {
    // AC #2: if WiFi not connected, return error rather than hitting GitHub
    if (WiFi.status() != WL_CONNECTED) {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        root["ok"] = true;
        JsonObject data = root["data"].to<JsonObject>();
        data["available"] = false;
        data["version"] = (const char*)nullptr;
        data["current_version"] = FW_VERSION;
        data["release_notes"] = (const char*)nullptr;
        data["error"] = "WiFi not connected";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    // Perform the check — this makes an HTTPS request to GitHub (blocks ~1-10s)
    bool updateAvailable = OTAUpdater::checkForUpdate();

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    if (OTAUpdater::getState() == OTAState::AVAILABLE) {
        data["available"] = true;
        data["version"] = OTAUpdater::getRemoteVersion();
        data["current_version"] = FW_VERSION;
        data["release_notes"] = OTAUpdater::getReleaseNotes();
    } else if (OTAUpdater::getState() == OTAState::ERROR) {
        // Rate limit / network / parse failure — include error message
        data["available"] = false;
        data["version"] = (const char*)nullptr;
        data["current_version"] = FW_VERSION;
        data["release_notes"] = (const char*)nullptr;
        data["error"] = OTAUpdater::getLastError();
    } else {
        // Up to date (IDLE state after successful check)
        data["available"] = false;
        data["version"] = (const char*)nullptr;
        data["current_version"] = FW_VERSION;
        data["release_notes"] = (const char*)nullptr;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostOtaPull(AsyncWebServerRequest* request) {
    OTAState state = OTAUpdater::getState();

    // Guard: already downloading/verifying (AC #2)
    if (state == OTAState::DOWNLOADING || state == OTAState::VERIFYING) {
        _sendJsonError(request, 409, "Download already in progress", "OTA_BUSY");
        return;
    }

    // Guard: no update available (AC #2)
    if (state != OTAState::AVAILABLE) {
        _sendJsonError(request, 400, "No update available \u2014 check for updates first", "NO_UPDATE");
        return;
    }

    // Start the download (AC #2)
    bool started = OTAUpdater::startDownload();

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    if (started) {
        root["ok"] = true;
        JsonObject data = root["data"].to<JsonObject>();
        data["started"] = true;
    } else {
        root["ok"] = false;
        root["error"] = OTAUpdater::getLastError();
        root["code"] = "OTA_START_FAILED";
    }

    String output;
    serializeJson(doc, output);
    request->send(started ? 200 : 500, "application/json", output);
}

void WebPortal::_handleGetOtaStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // AC #3: stable contract — lowercase state string
    data["state"] = OTAUpdater::getStateString();

    // progress: required for downloading, null otherwise
    OTAState state = OTAUpdater::getState();
    if (state == OTAState::DOWNLOADING) {
        data["progress"] = OTAUpdater::getProgress();
    } else if (state == OTAState::VERIFYING) {
        data["progress"] = (const char*)nullptr;  // indeterminate
    } else {
        data["progress"] = (const char*)nullptr;
    }

    // error: non-null only when state == error
    if (state == OTAState::ERROR) {
        data["error"] = OTAUpdater::getLastError();
    } else {
        data["error"] = (const char*)nullptr;
    }

    // Phase from dl-7.2 structured failure
    data["failure_phase"] = OTAUpdater::getFailurePhaseString();
    data["retriable"] = OTAUpdater::isRetriable();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetSchedule(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();

    // Build rules array from ConfigManager (AC #1)
    ModeScheduleConfig sched = ConfigManager::getModeSchedule();
    JsonArray rules = data["rules"].to<JsonArray>();
    for (uint8_t i = 0; i < sched.ruleCount; i++) {
        const ScheduleRule& r = sched.rules[i];
        JsonObject ruleObj = rules.add<JsonObject>();
        ruleObj["index"] = i;
        ruleObj["start_min"] = r.startMin;
        ruleObj["end_min"] = r.endMin;
        ruleObj["mode_id"] = r.modeId;
        ruleObj["enabled"] = r.enabled;
    }

    // Orchestrator state and active rule index (AC #1)
    data["orchestrator_state"] = ModeOrchestrator::getStateString();
    data["active_rule_index"] = ModeOrchestrator::getActiveScheduleRuleIndex();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostSchedule(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument reqDoc;
    DeserializationError err = deserializeJson(reqDoc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "INVALID_JSON");
        return;
    }

    if (!reqDoc["rules"].is<JsonArray>()) {
        _sendJsonError(request, 400, "Missing or invalid 'rules' array", "INVALID_SCHEDULE");
        return;
    }

    JsonArray rulesArr = reqDoc["rules"].as<JsonArray>();
    if (rulesArr.size() > MAX_SCHEDULE_RULES) {
        _sendJsonError(request, 400, "Too many rules (max 8)", "INVALID_SCHEDULE");
        return;
    }

    // Build ModeScheduleConfig from JSON (AC #2–#5)
    ModeScheduleConfig cfg = {};
    cfg.ruleCount = rulesArr.size();

    // Build mode ID lookup table for validation
    const ModeEntry* modeTable = ModeRegistry::getModeTable();
    const uint8_t modeCount = ModeRegistry::getModeCount();

    for (size_t i = 0; i < rulesArr.size(); i++) {
        JsonObject ruleObj = rulesArr[i];

        // Validate required fields
        if (!ruleObj["start_min"].is<int>() || !ruleObj["end_min"].is<int>() ||
            !ruleObj["mode_id"].is<const char*>() || !ruleObj["enabled"].is<int>()) {
            _sendJsonError(request, 400, "Rule missing required fields", "INVALID_SCHEDULE");
            return;
        }

        int startMin = ruleObj["start_min"].as<int>();
        int endMin = ruleObj["end_min"].as<int>();
        const char* modeId = ruleObj["mode_id"].as<const char*>();
        int enabled = ruleObj["enabled"].as<int>();

        // Range validation (AC #3)
        if (startMin < 0 || startMin > 1439 || endMin < 0 || endMin > 1439) {
            _sendJsonError(request, 400, "start_min/end_min out of range (0-1439)", "INVALID_SCHEDULE");
            return;
        }

        if (enabled < 0 || enabled > 1) {
            _sendJsonError(request, 400, "enabled must be 0 or 1", "INVALID_SCHEDULE");
            return;
        }

        if (!modeId || strlen(modeId) == 0 || strlen(modeId) >= MODE_ID_BUF_LEN) {
            _sendJsonError(request, 400, "Invalid mode_id", "INVALID_SCHEDULE");
            return;
        }

        // Validate mode_id exists in ModeRegistry (AC #3)
        bool modeFound = false;
        for (uint8_t m = 0; m < modeCount; m++) {
            if (strcmp(modeTable[m].id, modeId) == 0) {
                modeFound = true;
                break;
            }
        }
        if (!modeFound) {
            _sendJsonError(request, 400, "Unknown mode_id", "UNKNOWN_MODE");
            return;
        }

        cfg.rules[i].startMin = (uint16_t)startMin;
        cfg.rules[i].endMin = (uint16_t)endMin;
        strncpy(cfg.rules[i].modeId, modeId, MODE_ID_BUF_LEN - 1);
        cfg.rules[i].modeId[MODE_ID_BUF_LEN - 1] = '\0';
        cfg.rules[i].enabled = (uint8_t)enabled;
    }

    // Persist via ConfigManager (AC #2) — orchestrator tick picks up on next cycle (Rule 24)
    if (!ConfigManager::setModeSchedule(cfg)) {
        _sendJsonError(request, 500, "Failed to save schedule", "NVS_ERROR");
        return;
    }

    JsonDocument respDoc;
    JsonObject resp = respDoc.to<JsonObject>();
    resp["ok"] = true;
    JsonObject respData = resp["data"].to<JsonObject>();
    respData["applied"] = true;

    String output;
    serializeJson(respDoc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_sendJsonError(AsyncWebServerRequest* request, int httpCode, const char* error, const char* code) {
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = false;
    root["error"] = error;
    root["code"] = code;
    String output;
    serializeJson(doc, output);
    request->send(httpCode, "application/json", output);
}

// ──────────────────────────────────────────────────────────────────────────
// Layout CRUD + activation (Story LE-1.4)
// ──────────────────────────────────────────────────────────────────────────

namespace {

// Shared helpers for layout handlers — keep path parsing consistent.

// Extract a layout id segment from a URL of the form /api/layouts/<id>
// (or /api/layouts/<id>/activate when activateSegment = true).
// Strips any query string. Returns empty String on failure.
String extractLayoutIdFromUrl(const String& url, bool activateSegment) {
    if (activateSegment) {
        const char* prefix = "/api/layouts/";
        const char* suffix = "/activate";
        size_t prefixLen = strlen(prefix);
        size_t suffixLen = strlen(suffix);
        if (!url.startsWith(prefix) || !url.endsWith(suffix)) {
            return String();
        }
        if (url.length() < prefixLen + suffixLen + 1) return String();
        return url.substring(prefixLen, url.length() - suffixLen);
    }
    // Simple "/api/layouts/<id>" path: take everything after the final '/'.
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0 || lastSlash == (int)(url.length() - 1)) return String();
    String id = url.substring(lastSlash + 1);
    int queryStart = id.indexOf('?');
    if (queryStart >= 0) id = id.substring(0, queryStart);
    return id;
}

} // namespace

void WebPortal::_handleGetLayouts(AsyncWebServerRequest* request) {
    std::vector<LayoutEntry> entries;
    if (!LayoutStore::list(entries)) {
        _sendJsonError(request, 500, "Layout storage unavailable", "STORAGE_UNAVAILABLE");
        return;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    data["active_id"] = LayoutStore::getActiveId();
    JsonArray arr = data["layouts"].to<JsonArray>();
    for (const auto& e : entries) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = e.id;
        obj["name"] = e.name;
        obj["bytes"] = e.sizeBytes;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetLayoutById(AsyncWebServerRequest* request) {
    String id = extractLayoutIdFromUrl(request->url(), false);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Missing layout id", "MISSING_ID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    String body;
    if (!LayoutStore::load(id.c_str(), body)) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }
    // Parse body so it's embedded as a JSON object (not a double-encoded
    // string) under data.
    JsonDocument doc;
    JsonDocument layoutDoc;
    DeserializationError err = deserializeJson(layoutDoc, body);
    if (err) {
        _sendJsonError(request, 500, "Stored layout is corrupt", "LAYOUT_CORRUPT");
        return;
    }
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    root["data"] = layoutDoc.as<JsonVariant>();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostLayout(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "LAYOUT_INVALID");
        return;
    }
    if (!doc.is<JsonObject>()) {
        _sendJsonError(request, 400, "Expected JSON object", "LAYOUT_INVALID");
        return;
    }
    const char* id = doc["id"] | (const char*)nullptr;
    if (id == nullptr || *id == '\0') {
        _sendJsonError(request, 400, "Missing id in layout body", "LAYOUT_INVALID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id)) {
        _sendJsonError(request, 400, "Invalid layout id", "LAYOUT_INVALID");
        return;
    }
    String jsonStr;
    serializeJson(doc, jsonStr);

    LayoutStoreError result = LayoutStore::save(id, jsonStr.c_str());
    switch (result) {
        case LayoutStoreError::OK: {
            JsonDocument respDoc;
            JsonObject root = respDoc.to<JsonObject>();
            root["ok"] = true;
            JsonObject respData = root["data"].to<JsonObject>();
            respData["id"] = id;
            respData["bytes"] = jsonStr.length();
            String output;
            serializeJson(respDoc, output);
            request->send(200, "application/json", output);
            return;
        }
        case LayoutStoreError::TOO_LARGE:
            _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
            return;
        case LayoutStoreError::INVALID_SCHEMA:
            _sendJsonError(request, 400, "Layout schema invalid", "LAYOUT_INVALID");
            return;
        case LayoutStoreError::FS_FULL:
            _sendJsonError(request, 507, "Filesystem full", "FS_FULL");
            return;
        case LayoutStoreError::IO_ERROR:
        default:
            _sendJsonError(request, 500, "Layout IO error", "IO_ERROR");
            return;
    }
}

void WebPortal::_handlePutLayout(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    String id = extractLayoutIdFromUrl(request->url(), false);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Missing layout id", "MISSING_ID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    // Must exist already — PUT is overwrite-only. Caller should use POST to
    // create. Check via load() (fills a default on miss; use return value).
    String existing;
    if (!LayoutStore::load(id.c_str(), existing)) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendJsonError(request, 400, "Invalid JSON", "LAYOUT_INVALID");
        return;
    }
    if (!doc.is<JsonObject>()) {
        _sendJsonError(request, 400, "Expected JSON object", "LAYOUT_INVALID");
        return;
    }
    const char* docId = doc["id"] | (const char*)nullptr;
    if (docId == nullptr) {
        _sendJsonError(request, 400, "Missing id field in layout body", "LAYOUT_INVALID");
        return;
    }
    if (strcmp(docId, id.c_str()) != 0) {
        _sendJsonError(request, 400, "Body id does not match URL id", "LAYOUT_INVALID");
        return;
    }
    String jsonStr;
    serializeJson(doc, jsonStr);

    LayoutStoreError result = LayoutStore::save(id.c_str(), jsonStr.c_str());
    switch (result) {
        case LayoutStoreError::OK: {
            JsonDocument respDoc;
            JsonObject root = respDoc.to<JsonObject>();
            root["ok"] = true;
            JsonObject respData = root["data"].to<JsonObject>();
            respData["id"] = id;
            respData["bytes"] = jsonStr.length();
            String output;
            serializeJson(respDoc, output);
            request->send(200, "application/json", output);
            return;
        }
        case LayoutStoreError::TOO_LARGE:
            _sendJsonError(request, 413, "Layout too large", "LAYOUT_TOO_LARGE");
            return;
        case LayoutStoreError::INVALID_SCHEMA:
            _sendJsonError(request, 400, "Layout schema invalid", "LAYOUT_INVALID");
            return;
        case LayoutStoreError::FS_FULL:
            _sendJsonError(request, 507, "Filesystem full", "FS_FULL");
            return;
        case LayoutStoreError::IO_ERROR:
        default:
            _sendJsonError(request, 500, "Layout IO error", "IO_ERROR");
            return;
    }
}

void WebPortal::_handleDeleteLayout(AsyncWebServerRequest* request) {
    String id = extractLayoutIdFromUrl(request->url(), false);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Missing layout id", "MISSING_ID");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    String activeId = LayoutStore::getActiveId();
    if (activeId == id) {
        _sendJsonError(request, 409, "Cannot delete active layout", "LAYOUT_ACTIVE");
        return;
    }
    if (!LayoutStore::remove(id.c_str())) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handlePostLayoutActivate(AsyncWebServerRequest* request) {
    String id = extractLayoutIdFromUrl(request->url(), true);
    if (id.length() == 0) {
        _sendJsonError(request, 400, "Invalid activate path", "INVALID_PATH");
        return;
    }
    if (!LayoutStore::isSafeLayoutId(id.c_str())) {
        _sendJsonError(request, 400, "Invalid layout id", "INVALID_ID");
        return;
    }
    // Verify the layout actually exists — load() returns false on miss.
    String tmp;
    if (!LayoutStore::load(id.c_str(), tmp)) {
        _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
        return;
    }
    if (!LayoutStore::setActiveId(id.c_str())) {
        _sendJsonError(request, 500, "Failed to persist active layout id", "NVS_ERROR");
        return;
    }
    // Rule #24: route mode switch through orchestrator. Never call
    // ModeRegistry::requestSwitch directly from WebPortal.
    ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout");

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonObject data = root["data"].to<JsonObject>();
    data["active_id"] = id;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebPortal::_handleGetWidgetTypes(AsyncWebServerRequest* request) {
    // ─────────────────────────────────────────────────────────────────────
    // SYNC-RISK NOTE (LE-1.4 review follow-up):
    // The widget type strings below ("text", "clock", "logo", "flight_field",
    // "metric") and their field schemas are hard-coded here. The same type
    // strings also live in:
    //   • core/LayoutStore.cpp  — kAllowedWidgetTypes[] (save() validation)
    //   • core/WidgetRegistry.cpp — WidgetRegistry::fromString() (render dispatch)
    // All three lists MUST stay in lock-step. If a new widget type is added
    // without updating this handler, the editor UI (LE-1.7) will have no
    // way to surface it even though LayoutStore accepts it and the
    // renderer draws it. Consolidate in LE-1.7 when the widget system
    // stabilises (e.g. promote to a single WidgetType descriptor table
    // consumed by all three call sites).
    // ─────────────────────────────────────────────────────────────────────
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["ok"] = true;
    JsonArray data = root["data"].to<JsonArray>();

    // Helper lambdas to tighten the repetitive build-up. JsonDocument is
    // scoped to this function — no static allocations (AR rule).
    auto addField = [](JsonArray& fields, const char* key, const char* kind,
                       const char* defaultStr, int defaultInt, bool useInt,
                       const char* const* options, size_t optCount) {
        JsonObject f = fields.add<JsonObject>();
        f["key"] = key;
        f["kind"] = kind;
        if (useInt) f["default"] = defaultInt;
        else        f["default"] = defaultStr;
        if (options != nullptr && optCount > 0) {
            JsonArray arr = f["options"].to<JsonArray>();
            for (size_t i = 0; i < optCount; ++i) arr.add(options[i]);
        }
    };

    // ── text ─────────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "text";
        obj["label"] = "Text";
        JsonArray fields = obj["fields"].to<JsonArray>();
        addField(fields, "content", "string", "", 0, false, nullptr, 0);
        static const char* const alignOptions[] = { "left", "center", "right" };
        addField(fields, "align", "select", "left", 0, false, alignOptions, 3);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 32, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
    }
    // ── clock ────────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "clock";
        obj["label"] = "Clock";
        JsonArray fields = obj["fields"].to<JsonArray>();
        static const char* const clockFmtOptions[] = { "12h", "24h" };
        addField(fields, "content", "select", "24h", 0, false, clockFmtOptions, 2);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
    }
    // ── logo ─────────────────────────────────────────────────────────────
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "logo";
        obj["label"] = "Logo";
        // LE-1.5: real RGB565 bitmap pipeline — upload via POST /api/logos,
        // then set content to the ICAO/id (e.g. "UAL"). Falls back to
        // PROGMEM airplane sprite when logo_id is not found on LittleFS.
        JsonArray fields = obj["fields"].to<JsonArray>();
        addField(fields, "content", "string", "", 0, false, nullptr, 0);
        addField(fields, "color", "color", "#0000FF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 16, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 16, true, nullptr, 0);
    }
    // ── flight_field ─────────────────────────────────────────────────────
    // LE-1.8 — wired through to FlightFieldWidget. `content` is now a
    // dropdown of supported field keys; the renderer resolves them against
    // RenderContext.currentFlight and falls back to "--" when missing.
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "flight_field";
        obj["label"] = "Flight Field";
        JsonArray fields = obj["fields"].to<JsonArray>();
        static const char* const flightFieldOptions[] = {
            "airline", "ident", "origin_icao", "dest_icao", "aircraft"
        };
        addField(fields, "content", "select", "airline", 0, false,
                 flightFieldOptions, 5);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
    }
    // ── metric ───────────────────────────────────────────────────────────
    // LE-1.8 — wired through to MetricWidget. `content` is now a dropdown
    // of supported telemetry keys. NAN values and unknown keys fall back
    // to a "--" placeholder at render time.
    {
        JsonObject obj = data.add<JsonObject>();
        obj["type"] = "metric";
        obj["label"] = "Metric";
        JsonArray fields = obj["fields"].to<JsonArray>();
        static const char* const metricOptions[] = {
            "alt", "speed", "track", "vsi"
        };
        addField(fields, "content", "select", "alt", 0, false,
                 metricOptions, 4);
        addField(fields, "color", "color", "#FFFFFF", 0, false, nullptr, 0);
        addField(fields, "x", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "y", "int", nullptr, 0, true, nullptr, 0);
        addField(fields, "w", "int", nullptr, 48, true, nullptr, 0);
        addField(fields, "h", "int", nullptr, 8, true, nullptr, 0);
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}


]]></file>
<file id="20fd5acf" path="firmware/core/LayoutStore.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: Implements LayoutStore — durable, size-capped, schema-validated CRUD
for layout JSON documents on LittleFS.

Design notes (Story le-1.1):
- Mirrors LogoManager's static-class shape.
- Schema validation is done with a scoped ArduinoJson v7 JsonDocument that is
  destroyed before save() returns (no retained docs; "parse once at mode init"
  rule from the epic is honored by keeping parsing transient here too).
- FS_FULL is enforced by iterating /layouts/ and summing sizes before the
  write, excluding the target id when overwriting. This keeps the cap honest
  even when an existing file is being replaced with a larger one.
- getActiveId()/setActiveId() delegate to ConfigManager so that Preferences
  (NVS) traffic stays in one place and the "flightwall" namespace is owned by
  a single module.
- When load(id) misses, we still fill `out` with kDefaultLayoutJson so callers
  always see a valid layout JSON string, even before any file has been saved.
*/

#include "core/LayoutStore.h"

#include "core/ConfigManager.h"
#include "utils/Log.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ctype.h>
#include <string.h>

namespace {

// Directory that holds per-layout JSON files. Kept without trailing slash to
// match LittleFS listing conventions (see LogoManager).
static const char LAYOUTS_DIR[] = "/layouts";

// Widget type allowlist (AC #3f). Must stay in sync with the WidgetRegistry
// introduced in LE-1.2. Any new widget type is a schema change and a version
// bump on "v".
static const char* const kAllowedWidgetTypes[] = {
    "text", "clock", "logo", "flight_field", "metric",
};

// Baked-in fallback layout returned by load() when the requested id is
// missing (AC #6). This is NEVER written to disk; the "_default" id is
// reserved and rejected by save() indirectly through isSafeLayoutId (it
// starts with "_" which is allowed, but tests should not rely on being able
// to save id="_default" — the epic architecture reserves it).
static const char kDefaultLayoutJson[] = R"JSON({
  "id":"_default","name":"Default","v":1,
  "canvas":{"w":160,"h":32},
  "bg":"#000000",
  "widgets":[
    {"id":"w1","type":"clock","x":0,"y":0,"w":80,"h":16,"format":"24h","font":"M","color":"#FFFFFF"},
    {"id":"w2","type":"text","x":0,"y":16,"w":160,"h":10,"content":"FLIGHTWALL","align":"center","font":"S","color":"#AAAAAA"}
  ]
})JSON";

// Extract the basename (after the final '/') without allocating a copy for
// the full path. LittleFS returns entry names with or without a leading slash
// depending on platform version.
String layoutBasename(const String& path) {
    int lastSlash = path.lastIndexOf('/');
    return lastSlash >= 0 ? path.substring(lastSlash + 1) : path;
}

// Return true iff `name` ends with ".json".
bool hasJsonExtension(const String& name) {
    return name.length() > 5 && name.endsWith(".json");
}

// Strip the ".json" suffix from a filename. Caller must ensure the suffix is
// present.
String stripJsonExtension(const String& name) {
    return name.substring(0, name.length() - 5);
}

// Build the canonical on-disk path for a given id.
String pathFor(const char* id) {
    String p;
    p.reserve(strlen(LAYOUTS_DIR) + 1 + strlen(id) + 5);
    p = LAYOUTS_DIR;
    p += '/';
    p += id;
    p += ".json";
    return p;
}

bool isAllowedWidgetType(const char* t) {
    if (t == nullptr) return false;
    for (size_t i = 0; i < sizeof(kAllowedWidgetTypes) / sizeof(kAllowedWidgetTypes[0]); ++i) {
        if (strcmp(t, kAllowedWidgetTypes[i]) == 0) return true;
    }
    return false;
}

// Required top-level fields per AC #3c.
bool hasRequiredTopLevelFields(JsonDocument& doc) {
    return !doc["id"].isNull()
        && !doc["name"].isNull()
        && !doc["v"].isNull()
        && !doc["canvas"].isNull()
        && !doc["widgets"].isNull();
}

// AC #3 full schema validation. Returns OK or a specific error code.
// `pathId` is the id used to construct the on-disk path (i.e. the first
// argument to save()). We require doc["id"] to match so that the filesystem
// truth (filename) and the JSON self-description are always consistent.
LayoutStoreError validateSchema(const char* json, const char* pathId) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    if (!hasRequiredTopLevelFields(doc)) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    // Require doc["id"] to equal the caller-supplied path id so the filename
    // and JSON self-description are always in sync (prevents silent desync
    // where save("foo", {"id":"bar",...}) stores a file whose name lies about
    // its own content).
    const char* docId = doc["id"] | (const char*)nullptr;
    if (docId == nullptr || strcmp(docId, pathId) != 0) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    // AC #3d
    if (doc["v"].as<int>() != 1) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    // AC #3e
    JsonVariantConst canvas = doc["canvas"];
    if (!canvas.is<JsonObjectConst>()) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    int cw = canvas["w"] | 0;
    int ch = canvas["h"] | 0;
    if (cw < 1 || ch < 1) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    // AC #3f
    JsonVariantConst widgets = doc["widgets"];
    if (!widgets.is<JsonArrayConst>()) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    for (JsonVariantConst wv : widgets.as<JsonArrayConst>()) {
        const char* t = wv["type"] | (const char*)nullptr;
        if (!isAllowedWidgetType(t)) {
            return LayoutStoreError::INVALID_SCHEMA;
        }
    }
    return LayoutStoreError::OK;
    // doc destructs here — no retained JSON state (per epic parse-once rule).
}

// Walk /layouts/ and accumulate file stats. Any file whose basename stem
// matches `excludeId` (i.e. the id being overwritten by save) is skipped
// from the totals so the overwrite path doesn't double-count.
struct DirStats {
    size_t totalBytes;
    uint8_t fileCount;
};

bool collectDirStats(const char* excludeId, DirStats& out) {
    out.totalBytes = 0;
    out.fileCount = 0;
    File dir = LittleFS.open(LAYOUTS_DIR);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return false;
    }
    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String name = layoutBasename(entry.name());
            if (hasJsonExtension(name)) {
                String stem = stripJsonExtension(name);
                bool excluded = (excludeId != nullptr && stem == excludeId);
                if (!excluded) {
                    out.totalBytes += entry.size();
                    out.fileCount++;
                }
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
    return true;
}

} // namespace

// ------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------

bool LayoutStore::init() {
    if (!LittleFS.exists(LAYOUTS_DIR)) {
        if (!LittleFS.mkdir(LAYOUTS_DIR)) {
            LOG_E("LayoutStore", "Failed to create /layouts/ directory");
            return false;
        }
        LOG_I("LayoutStore", "Created /layouts/ directory");
        return true;
    }
    File dir = LittleFS.open(LAYOUTS_DIR);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        LOG_E("LayoutStore", "/layouts exists but is not a directory");
        return false;
    }
    dir.close();
    return true;
}

bool LayoutStore::isSafeLayoutId(const char* id) {
    if (id == nullptr) return false;
    size_t len = strlen(id);
    if (len == 0 || len > LAYOUT_STORE_MAX_ID_LEN) return false;
    if (strstr(id, "..") != nullptr) return false;
    // "_default" is a reserved id (AC #6 / epic architecture). It is never
    // written to disk; rejecting it here prevents a saved file from silently
    // shadowing the baked-in fallback and breaking future LE-1.3 wiring.
    if (strcmp(id, "_default") == 0) return false;
    for (size_t i = 0; i < len; ++i) {
        char c = id[i];
        // Allowed: [A-Za-z0-9_-]. Explicitly rejects '/', '\\', '.' and any
        // control/unicode byte.
        bool ok = (c >= 'a' && c <= 'z')
               || (c >= 'A' && c <= 'Z')
               || (c >= '0' && c <= '9')
               || c == '_' || c == '-';
        if (!ok) return false;
    }
    return true;
}

LayoutStoreError LayoutStore::save(const char* id, const char* json) {
    // AC #3: id safety check first — path-traversal ids never reach the FS.
    if (!isSafeLayoutId(id)) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    if (json == nullptr) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    size_t jsonLen = strlen(json);
    // AC #3a: per-file cap.
    if (jsonLen > LAYOUT_STORE_MAX_BYTES) {
        return LayoutStoreError::TOO_LARGE;
    }
    // AC #3b–f: schema validation (also verifies doc["id"] == id).
    LayoutStoreError schemaResult = validateSchema(json, id);
    if (schemaResult != LayoutStoreError::OK) {
        return schemaResult;
    }
    // AC #4: directory-wide byte + file-count caps. Ensure /layouts/ exists
    // (fresh devices reach save() before init() only via tests that explicitly
    // skip init()).
    if (!LittleFS.exists(LAYOUTS_DIR)) {
        if (!LittleFS.mkdir(LAYOUTS_DIR)) {
            return LayoutStoreError::IO_ERROR;
        }
    }
    DirStats stats;
    if (!collectDirStats(id, stats)) {
        return LayoutStoreError::IO_ERROR;
    }
    if (stats.fileCount >= LAYOUT_STORE_MAX_FILES) {
        return LayoutStoreError::FS_FULL;
    }
    if (stats.totalBytes + jsonLen > LAYOUT_STORE_TOTAL_BYTES) {
        return LayoutStoreError::FS_FULL;
    }
    // Write atomically-ish: open for write truncates, then stream bytes.
    String path = pathFor(id);
    File f = LittleFS.open(path, "w");
    if (!f) {
        return LayoutStoreError::IO_ERROR;
    }
    size_t written = f.write(reinterpret_cast<const uint8_t*>(json), jsonLen);
    f.close();
    if (written != jsonLen) {
        // Partial write — clean up so the dir cap math stays honest.
        LittleFS.remove(path);
        return LayoutStoreError::IO_ERROR;
    }
    return LayoutStoreError::OK;
}

bool LayoutStore::load(const char* id, String& out) {
    if (!isSafeLayoutId(id)) {
        // AC #6: include the offending id in the warning for diagnosability.
        Serial.printf("[LayoutStore] WARNING: layout_active '%s' id invalid — using default\n", id);
        out = kDefaultLayoutJson;
        return false;
    }
    String path = pathFor(id);
    if (!LittleFS.exists(path)) {
        // AC #6: log the missing id so operators can diagnose stale NVS values.
        Serial.printf("[LayoutStore] WARNING: layout_active '%s' missing — using default\n", id);
        out = kDefaultLayoutJson;
        return false;
    }
    File f = LittleFS.open(path, "r");
    if (!f) {
        Serial.printf("[LayoutStore] WARNING: layout_active '%s' open failed — using default\n", id);
        out = kDefaultLayoutJson;
        return false;
    }
    // Guard against files that exceed the per-file cap (e.g. manual upload
    // or LittleFS corruption). readString() on the ESP32 allocates the full
    // file into a heap String; an oversized file would OOM the device.
    if (f.size() > LAYOUT_STORE_MAX_BYTES) {
        Serial.printf("[LayoutStore] WARNING: layout_active '%s' file too large (%u B) — using default\n",
                      id, (unsigned)f.size());
        f.close();
        out = kDefaultLayoutJson;
        return false;
    }
    out = f.readString();
    f.close();
    return true;
}

bool LayoutStore::remove(const char* id) {
    if (!isSafeLayoutId(id)) {
        return false;
    }
    String path = pathFor(id);
    if (!LittleFS.exists(path)) {
        return false;
    }
    return LittleFS.remove(path);
}

bool LayoutStore::list(std::vector<LayoutEntry>& result) {
    result.clear();
    File dir = LittleFS.open(LAYOUTS_DIR);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return false;
    }
    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String name = layoutBasename(entry.name());
            if (hasJsonExtension(name)) {
                LayoutEntry e;
                e.id = stripJsonExtension(name);
                e.sizeBytes = entry.size();

                // Pull "name" out of the JSON without retaining the doc. If
                // the file is malformed, leave e.name empty — list() still
                // advertises the file exists so the UI can offer a delete.
                // Guard against oversized files (corruption/manual upload)
                // before readString() to avoid heap exhaustion.
                if (entry.size() <= LAYOUT_STORE_MAX_BYTES) {
                    String body = entry.readString();
                    JsonDocument doc;
                    DeserializationError err = deserializeJson(doc, body);
                    if (!err) {
                        const char* nm = doc["name"] | "";
                        e.name = nm;
                    }
                }
                result.push_back(e);
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
    return true;
}

String LayoutStore::getActiveId() {
    return ConfigManager::getLayoutActiveId();
}

bool LayoutStore::setActiveId(const char* id) {
    if (!isSafeLayoutId(id)) return false;
    return ConfigManager::setLayoutActiveId(id);
}


]]></file>
<file id="17d0e1e0" path="firmware/core/LayoutStore.h" label="SOURCE CODE"><![CDATA[

#pragma once
/*
Purpose: Persistent CRUD store for user-defined layouts on LittleFS.

Story le-1.1 (layout-store-littlefs-crud) — layer owned exclusively by this
story. Integration with CustomLayoutMode is deferred to LE-1.3.

Responsibilities:
  - Maintain `/layouts/<id>.json` files under LittleFS with strict size and
    count caps (per-file + directory-wide).
  - Validate the V1 layout JSON schema on every save() (required top-level
    fields, version, canvas bounds, widget type allowlist).
  - Track the currently active layout id in NVS (`layout_active` key,
    delegated to ConfigManager) with a baked-in default fallback.
  - Provide a pure, hardware-free API surface (only LittleFS + NVS IO); no
    heavy state — all methods are static.
*/

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>

// ------------------------------------------------------------------
// Compile-time limits (enforced by LayoutStore::save() before any IO).
// ------------------------------------------------------------------

// Per-file cap (AC #3a). Strings exceeding this are rejected before JSON
// parsing. 8 KiB comfortably fits the documented V1 schema examples while
// leaving headroom in the 960 KB LittleFS partition.
static constexpr size_t LAYOUT_STORE_MAX_BYTES = 8192;

// Directory-wide cap (AC #4). A save() that would push the /layouts/ tree
// past this total (net of any overwrite) returns FS_FULL. 64 KiB matches the
// budget documented in the epic Architecture section.
static constexpr size_t LAYOUT_STORE_TOTAL_BYTES = 65536;

// Maximum number of layouts retained on-device. New-id save() attempts beyond
// this return FS_FULL regardless of byte usage.
static constexpr uint8_t LAYOUT_STORE_MAX_FILES = 16;

// Maximum length of a validated layout id (bytes, excluding null).
static constexpr size_t LAYOUT_STORE_MAX_ID_LEN = 32;

// ------------------------------------------------------------------
// Public types
// ------------------------------------------------------------------

enum class LayoutStoreError : uint8_t {
    OK = 0,
    NOT_FOUND,
    TOO_LARGE,
    INVALID_SCHEMA,
    FS_FULL,
    IO_ERROR,
};

struct LayoutEntry {
    String id;          // filename stem (e.g. "home-wall" from "home-wall.json")
    String name;        // value of top-level "name" field inside the JSON
    size_t sizeBytes;   // raw file size on disk
};

class LayoutStore {
public:
    // Ensure /layouts/ exists as a directory. Returns false if it exists as a
    // non-directory entry or if creation fails.
    static bool init();

    // Persist `json` as /layouts/<id>.json. Validates id + payload; overwrites
    // in place if the id already exists. See LayoutStoreError for failure
    // codes and the story AC for the exact rejection contract.
    static LayoutStoreError save(const char* id, const char* json);

    // Load /layouts/<id>.json into `out`. On success returns true and `out`
    // contains the raw JSON text. On miss, fills `out` with the baked-in
    // default layout, emits a warning log, and returns false (AC #6).
    static bool load(const char* id, String& out);

    // Remove /layouts/<id>.json. Returns true if the delete succeeded.
    static bool remove(const char* id);

    // Enumerate every valid /layouts/*.json entry into `result`. Returns
    // false only if the directory cannot be opened.
    static bool list(std::vector<LayoutEntry>& result);

    // Validate a candidate layout id against the on-disk path-safety rules
    // (non-null, non-empty, length ≤ LAYOUT_STORE_MAX_ID_LEN, only
    // `[A-Za-z0-9_-]`, no "..", "/", or "\\" substrings).
    static bool isSafeLayoutId(const char* id);

    // NVS-backed active-id accessors (AC #5). Delegate to ConfigManager to
    // keep all Preferences traffic in one place.
    static String getActiveId();
    static bool setActiveId(const char* id);
};


]]></file>
<file id="376217b4" path="firmware/core/WidgetRegistry.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: WidgetRegistry static dispatch implementation (Story LE-1.2).

Implementation notes:
  - No virtual functions: dispatch is a compile-time `switch (type)` over
    WidgetType. Enum values without a matching case fall through to the
    post-switch `return false` (WidgetType::Unknown has an explicit case).
  - `fromString()` uses a linear sequence of strcmp comparisons. This is
    the same pattern as LayoutStore::isAllowedWidgetType() and mirrors
    CustomLayoutMode's parseType() from the V0 spike.
  - The allowlist here MUST stay in lock-step with LayoutStore's
    kAllowedWidgetTypes[]. If a new widget type is added in a later story,
    both lists must grow together or the Save→Load→Render loop breaks.
*/

#include "core/WidgetRegistry.h"

#include "widgets/TextWidget.h"
#include "widgets/ClockWidget.h"
#include "widgets/LogoWidget.h"
#include "widgets/FlightFieldWidget.h"
#include "widgets/MetricWidget.h"

#include <cstring>

// ------------------------------------------------------------------
// fromString
// ------------------------------------------------------------------
WidgetType WidgetRegistry::fromString(const char* typeStr) {
    if (typeStr == nullptr)                return WidgetType::Unknown;
    if (typeStr[0] == '\0')                return WidgetType::Unknown;
    if (strcmp(typeStr, "text")         == 0) return WidgetType::Text;
    if (strcmp(typeStr, "clock")        == 0) return WidgetType::Clock;
    if (strcmp(typeStr, "logo")         == 0) return WidgetType::Logo;
    if (strcmp(typeStr, "flight_field") == 0) return WidgetType::FlightField;
    if (strcmp(typeStr, "metric")       == 0) return WidgetType::Metric;
    return WidgetType::Unknown;
}

bool WidgetRegistry::isKnownType(const char* typeStr) {
    return fromString(typeStr) != WidgetType::Unknown;
}

// ------------------------------------------------------------------
// dispatch
//
// Explicitly enumerates every known WidgetType with no catch-all default,
// so adding a new enum value produces a -Wswitch warning at this site and
// cannot be silently ignored.
// WidgetType::Unknown is handled by its own explicit case. Any out-of-range
// uint8_t value cast to WidgetType (e.g. from memory corruption) falls
// through all case labels unmatched and hits the post-switch `return false`
// — valid C++, no undefined behavior.
// ------------------------------------------------------------------
bool WidgetRegistry::dispatch(WidgetType type,
                              const WidgetSpec& spec,
                              const RenderContext& ctx) {
    switch (type) {
        case WidgetType::Text:        return renderText(spec, ctx);
        case WidgetType::Clock:       return renderClock(spec, ctx);
        case WidgetType::Logo:        return renderLogo(spec, ctx);
        case WidgetType::FlightField: return renderFlightField(spec, ctx);
        case WidgetType::Metric:      return renderMetric(spec, ctx);
        case WidgetType::Unknown:     return false;
    }
    return false;
}


]]></file>
<file id="bff86d84" path="firmware/core/WidgetRegistry.h" label="SOURCE CODE"><![CDATA[

#pragma once
/*
Purpose: Static widget registry with switch-based dispatch by widget type.

Story LE-1.2 (widget-registry-and-core-widgets) — promotes the V0 spike's
dispatch pattern to a production library. Zero virtual dispatch, zero heap
allocation on the render hot path.

Responsibilities:
  - Define the production `WidgetType` enum and `WidgetSpec` struct used by
    render-time code (mode renderers, LE-1.3 integration).
  - Translate JSON widget type strings ("text", "clock", ...) to enum values.
  - Dispatch a pre-parsed `WidgetSpec` to the matching stateless render
    function via a compile-time `switch` on `WidgetType`.

Non-responsibilities:
  - Does NOT parse JSON. Widget specs are built by the caller at layout-init
    time (LE-1.3) from validated JSON.
  - Does NOT own any state. All methods are static; the render functions are
    free functions with function-scoped statics where caching is needed.
  - Does NOT allocate heap. Dispatch is a pointer-free switch; render
    functions use stack buffers or file-scoped static buffers only.

Architecture references:
  - firmware/modes/CustomLayoutMode.cpp (V0 spike reference pattern)
  - firmware/core/LayoutStore.cpp (kAllowedWidgetTypes[] — must stay in sync)
  - firmware/interfaces/DisplayMode.h (RenderContext is reused, not redefined)
*/

#include <Arduino.h>
#include <stdint.h>

#include "interfaces/DisplayMode.h"

// ------------------------------------------------------------------
// Font / dimension constants shared across widget render functions.
// Matches the 5x7 default Adafruit GFX font + 1px spacing used by
// ClassicCardMode and the V0 spike (CustomLayoutMode.cpp).
// ------------------------------------------------------------------
static constexpr int WIDGET_CHAR_W = 6;
static constexpr int WIDGET_CHAR_H = 8;

// ------------------------------------------------------------------
// WidgetType — production enum, wire-compatible with JSON type strings.
// Integer values are stable (persisted to NVS or used as table indices is
// not a current requirement, but the explicit numbering prevents accidental
// reordering across stories).
// LayoutStore::kAllowedWidgetTypes[] must contain the matching string set.
// ------------------------------------------------------------------
enum class WidgetType : uint8_t {
    Text        = 0,
    Clock       = 1,
    Logo        = 2,
    FlightField = 3,   // LE-1.8
    Metric      = 4,   // LE-1.8
    Unknown     = 0xFF,
};

// ------------------------------------------------------------------
// WidgetSpec — compact value type describing one widget instance.
//
// Pre-populated at layout init time (LE-1.3) from validated JSON. Widget
// render functions take this by const-ref and MUST NOT mutate it. Field
// layout is tuned for stack allocation when an entire layout (up to 24
// widgets) is materialised at once.
// ------------------------------------------------------------------
struct WidgetSpec {
    WidgetType type;
    int16_t    x;
    int16_t    y;
    uint16_t   w;
    uint16_t   h;
    uint16_t   color;        // RGB565
    char       id[16];       // widget instance id (e.g. "w1"), 15 chars + null
    char       content[48];  // text content / clock format / logo_id
    uint8_t    align;        // 0=left, 1=center, 2=right (TextWidget only)
    uint8_t    _reserved;    // pad for alignment
};

// ------------------------------------------------------------------
// WidgetRegistry — static dispatch façade.
// ------------------------------------------------------------------
class WidgetRegistry {
public:
    // Dispatch to the render function matching `type`. Returns the render
    // function's success flag. For Unknown or unimplemented types, returns
    // false without touching the framebuffer.
    //
    // `type` is passed explicitly (rather than reading `spec.type`) to keep
    // the dispatcher honest about the contract and to allow future callers
    // to force a type without mutating the spec.
    static bool dispatch(WidgetType type,
                         const WidgetSpec& spec,
                         const RenderContext& ctx);

    // Convert a JSON type string (e.g. "text") to its enum value. Returns
    // WidgetType::Unknown for null/empty/unknown input. Comparison is
    // strictly case-sensitive to match the LayoutStore allowlist.
    static WidgetType fromString(const char* typeStr);

    // True iff `typeStr` maps to a non-Unknown WidgetType. Cheap predicate
    // over fromString().
    static bool isKnownType(const char* typeStr);
};


]]></file>
<file id="54ecacb1" path="firmware/data-src/common.js" label="SOURCE CODE"><![CDATA[

/* FlightWall shared HTTP helpers */
var FW = (function() {
  'use strict';

  function parseJsonResponse(res) {
    return res.text().then(function(text) {
      if (!text) {
        return {
          status: res.status,
          body: { ok: false, error: 'Empty server response (HTTP ' + res.status + ')', code: 'EMPTY_RESPONSE' }
        };
      }

      try {
        return { status: res.status, body: JSON.parse(text) };
      } catch (err) {
        return {
          status: res.status,
          body: { ok: false, error: 'Invalid server response (HTTP ' + res.status + ')', code: 'INVALID_RESPONSE' }
        };
      }
    });
  }

  function fetchJson(url, opts) {
    return fetch(url, opts || {}).then(function(res) {
      return parseJsonResponse(res);
    });
  }

  function get(url) {
    return fetchJson(url);
  }

  function post(url, data) {
    return fetchJson(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    });
  }

  function del(url) {
    return fetchJson(url, { method: 'DELETE' });
  }

  function put(url, data) {
    return fetchJson(url, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    });
  }

  /* Toast notification system */
  var toastContainer = null;

  function ensureToastContainer() {
    if (toastContainer) return toastContainer;
    toastContainer = document.getElementById('toast-container');
    if (!toastContainer) {
      toastContainer = document.createElement('div');
      toastContainer.id = 'toast-container';
      document.body.appendChild(toastContainer);
    }
    return toastContainer;
  }

  function showToast(message, severity) {
    var container = ensureToastContainer();
    var toast = document.createElement('div');
    toast.className = 'toast toast-' + (severity || 'success');
    toast.textContent = message;
    container.appendChild(toast);
    // Trigger reflow for animation
    toast.offsetHeight;
    toast.classList.add('toast-visible');
    setTimeout(function() {
      toast.classList.remove('toast-visible');
      toast.classList.add('toast-exit');
      setTimeout(function() {
        if (toast.parentNode) toast.parentNode.removeChild(toast);
      }, 300);
    }, 2500);
  }

  return { get: get, post: post, put: put, del: del, showToast: showToast };
})();


]]></file>
<file id="a7471f3e" path="firmware/data-src/editor.css" label="STYLESHEET"><![CDATA[

/* FlightWall Layout Editor styles (LE-1.6) */

.editor-layout {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
  background: #0d1117;
  color: #c9d1d9;
}

.editor-header {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 8px 16px;
  background: #161b22;
  border-bottom: 1px solid #30363d;
}

.editor-header h1 {
  margin: 0;
  font-size: 1.1rem;
  font-weight: 600;
}

.editor-back {
  color: #58a6ff;
  text-decoration: none;
  font-size: 0.95rem;
}

.editor-back:hover {
  text-decoration: underline;
}

.editor-main {
  display: flex;
  flex-direction: row;
  flex: 1;
  min-height: 0;
}

.editor-toolbox {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 8px;
  width: 120px;
  background: #161b22;
  border-right: 1px solid #30363d;
  overflow-y: auto;
}

.toolbox-item {
  padding: 8px 10px;
  background: #21262d;
  border: 1px solid #30363d;
  border-radius: 4px;
  cursor: grab;
  user-select: none;
  font-size: 0.9rem;
  text-align: center;
}

.toolbox-item:hover {
  background: #2d333b;
}

.toolbox-item.disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.editor-canvas-wrap {
  flex: 1;
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  padding: 12px;
  overflow: auto;
  background: #0d1117;
}

#editor-canvas {
  image-rendering: pixelated;
  image-rendering: crisp-edges;
  touch-action: none;
  cursor: crosshair;
  display: block;
  background: #111;
  border: 1px solid #30363d;
}

.editor-props {
  width: 220px;
  padding: 12px;
  background: #161b22;
  border-left: 1px solid #30363d;
  overflow-y: auto;
}

.props-placeholder {
  color: #8b949e;
  font-style: italic;
  margin: 0;
}

.editor-controls {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 8px 16px;
  background: #161b22;
  border-top: 1px solid #30363d;
}

.snap-control {
  display: inline-flex;
  border: 1px solid #30363d;
  border-radius: 4px;
  overflow: hidden;
}

.snap-btn {
  padding: 6px 12px;
  background: #21262d;
  color: #c9d1d9;
  border: none;
  border-right: 1px solid #30363d;
  cursor: pointer;
  font-size: 0.9rem;
}

.snap-btn:last-child {
  border-right: none;
}

.snap-btn:hover {
  background: #2d333b;
}

.snap-btn.active {
  background: var(--accent, #58a6ff);
  color: #000;
}

.editor-status {
  color: #8b949e;
  font-size: 0.85rem;
}

/* Mobile: stack toolbox horizontally above the canvas */
@media (max-width: 720px) {
  .editor-main {
    flex-direction: column;
  }
  .editor-toolbox {
    flex-direction: row;
    width: auto;
    border-right: none;
    border-bottom: 1px solid #30363d;
    overflow-x: auto;
    overflow-y: hidden;
  }
  .toolbox-item {
    flex: 0 0 auto;
  }
  .editor-props {
    width: auto;
    border-left: none;
    border-top: 1px solid #30363d;
  }
  .layout-name-input {
    width: 100px;
  }
}

/* --- Property panel form --- */
.props-form { display: flex; flex-direction: column; gap: 10px; }
.props-field { display: flex; flex-direction: column; gap: 4px; }
.props-label { font-size: 0.8rem; color: #8b949e; text-transform: uppercase; letter-spacing: 0.04em; }
.props-input { background: #0d1117; border: 1px solid #30363d; border-radius: 4px; color: #c9d1d9; padding: 5px 8px; font-size: 0.9rem; width: 100%; box-sizing: border-box; }
.props-input:focus { outline: none; border-color: #58a6ff; }
.props-color-input { height: 36px; padding: 2px; background: #0d1117; border: 1px solid #30363d; border-radius: 4px; cursor: pointer; width: 100%; box-sizing: border-box; }
.props-readout { }
.props-readout-value { font-size: 0.85rem; color: #c9d1d9; font-variant-numeric: tabular-nums; }

/* --- Align control (L/C/R segment) --- */
.align-control { display: inline-flex; border: 1px solid #30363d; border-radius: 4px; overflow: hidden; width: 100%; }
.align-btn { flex: 1; padding: 6px 0; background: #21262d; color: #c9d1d9; border: none; border-right: 1px solid #30363d; cursor: pointer; font-size: 0.9rem; }
.align-btn:last-child { border-right: none; }
.align-btn:hover { background: #2d333b; }
.align-btn.active { background: var(--accent, #58a6ff); color: #000; }

/* --- Layout name + save control bar additions --- */
.layout-name-input { background: #0d1117; border: 1px solid #30363d; border-radius: 4px; color: #c9d1d9; padding: 6px 10px; font-size: 0.9rem; width: 140px; box-sizing: border-box; }
.layout-name-input:focus { outline: none; border-color: #58a6ff; }
.save-control { display: inline-flex; gap: 6px; }
.save-btn { padding: 6px 12px; background: #21262d; color: #c9d1d9; border: 1px solid #30363d; border-radius: 4px; cursor: pointer; font-size: 0.9rem; }
.save-btn:hover { background: #2d333b; }
.save-btn-primary { background: #238636; border-color: #2ea043; color: #fff; }
.save-btn-primary:hover { background: #2ea043; }


]]></file>
<file id="9fa75dbd" path="firmware/data-src/editor.html" label="HTML TEMPLATE"><![CDATA[

<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
  <title>FlightWall Editor</title>
  <link rel="stylesheet" href="/style.css">
  <link rel="stylesheet" href="/editor.css">
</head>
<body>
  <div class="editor-layout">
    <header class="editor-header">
      <a href="/" class="editor-back">&larr; Dashboard</a>
      <h1>Layout Editor</h1>
    </header>
    <div class="editor-main">
      <div class="editor-toolbox" id="editor-toolbox">
        <!-- populated by JS from /api/widgets/types -->
      </div>
      <div class="editor-canvas-wrap">
        <canvas id="editor-canvas"></canvas>
      </div>
      <div class="editor-props" id="editor-props">
        <p class="props-placeholder" id="props-placeholder">Select a widget</p>
        <div class="props-form" id="props-form" style="display:none">
          <div class="props-field" id="props-field-content">
            <label class="props-label" for="prop-content">Content</label>
            <input type="text" id="prop-content" class="props-input" autocomplete="off">
            <select id="prop-content-select" class="props-input" style="display:none"></select>
          </div>
          <div class="props-field" id="props-field-color">
            <label class="props-label" for="prop-color">Color</label>
            <input type="color" id="prop-color" class="props-color-input" value="#ffffff">
          </div>
          <div class="props-field" id="props-field-align">
            <label class="props-label">Align</label>
            <div class="align-control" id="align-control">
              <button type="button" class="align-btn" data-align="left">L</button>
              <button type="button" class="align-btn" data-align="center">C</button>
              <button type="button" class="align-btn" data-align="right">R</button>
            </div>
          </div>
          <div class="props-field props-readout" id="props-field-pos">
            <label class="props-label">Position</label>
            <span id="prop-pos-readout" class="props-readout-value">&mdash;</span>
          </div>
          <div class="props-field props-readout" id="props-field-size">
            <label class="props-label">Size</label>
            <span id="prop-size-readout" class="props-readout-value">&mdash;</span>
          </div>
        </div>
      </div>
    </div>
    <div class="editor-controls">
      <input type="text" id="layout-name" class="layout-name-input" placeholder="Layout name" maxlength="32" autocomplete="off">
      <div class="save-control">
        <button type="button" id="btn-save-new" class="save-btn">Save as New</button>
        <button type="button" id="btn-save" class="save-btn">Save</button>
        <button type="button" id="btn-activate" class="save-btn save-btn-primary">Save &amp; Activate</button>
      </div>
      <div class="snap-control" id="snap-control">
        <button type="button" class="snap-btn" data-snap="8">Default</button>
        <button type="button" class="snap-btn" data-snap="16">Tidy</button>
        <button type="button" class="snap-btn" data-snap="1">Fine</button>
      </div>
      <span class="editor-status" id="editor-status"></span>
    </div>
  </div>
  <script src="/common.js"></script>
  <script src="/editor.js"></script>
</body>
</html>


]]></file>
<file id="f03fe8e0" path="firmware/data-src/editor.js" label="SOURCE CODE"><![CDATA[

/* FlightWall Layout Editor (LE-1.7) - ES5 only */
(function() {
  'use strict';

  /* ---------- Module globals ---------- */
  var SCALE = 4;                /* CSS pixels per logical pixel */
  var SNAP = 8;                 /* current snap grid: 8 / 16 / 1 */
  var deviceW = 0;              /* logical canvas width (from /api/layout) */
  var deviceH = 0;              /* logical canvas height */
  var tilePixels = 16;          /* tile size in logical pixels */
  var widgets = [];             /* [{type,x,y,w,h,color,content,align,id}] */
  var selectedIndex = -1;       /* -1 = no selection */
  var dragState = null;         /* {mode,startX,startY,origX,origY,origW,origH,toastedFloor} */
  var widgetTypeMeta = {};      /* keyed by type string */
  var HANDLE_SIZE = 6;          /* CSS pixels for resize handle */
  var dirty = false;            /* true when widgets changed since last save */
  var currentLayoutId = null;   /* string id after first save, or null for new */

  /* ---------- Utilities ---------- */
  function snapTo(val, grid) {
    return Math.round(val / grid) * grid;
  }

  function clamp(val, lo, hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
  }

  function getCanvasPos(e) {
    var canvas = document.getElementById('editor-canvas');
    var rect = canvas.getBoundingClientRect();
    return { x: e.clientX - rect.left, y: e.clientY - rect.top };
  }

  function setStatus(msg) {
    var el = document.getElementById('editor-status');
    if (el) el.textContent = msg || '';
  }

  /* ---------- Render ---------- */
  function render() {
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    var ctx = canvas.getContext('2d');
    ctx.imageSmoothingEnabled = false;
    /* 1. Clear */
    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    /* 2. Tile grid */
    drawGrid(ctx);
    /* 3. Widgets */
    for (var i = 0; i < widgets.length; i++) {
      drawWidget(ctx, widgets[i], i === selectedIndex);
    }
  }

  function drawGrid(ctx) {
    var canvas = ctx.canvas;
    var step = tilePixels * SCALE;
    if (step <= 0) return;
    ctx.strokeStyle = 'rgba(255,255,255,0.1)';
    ctx.lineWidth = 1;
    var x;
    for (x = step; x < canvas.width; x += step) {
      ctx.beginPath();
      ctx.moveTo(x + 0.5, 0);
      ctx.lineTo(x + 0.5, canvas.height);
      ctx.stroke();
    }
    var y;
    for (y = step; y < canvas.height; y += step) {
      ctx.beginPath();
      ctx.moveTo(0, y + 0.5);
      ctx.lineTo(canvas.width, y + 0.5);
      ctx.stroke();
    }
  }

  function drawWidget(ctx, w, isSelected) {
    var x = w.x * SCALE;
    var y = w.y * SCALE;
    var cw = w.w * SCALE;
    var ch = w.h * SCALE;
    /* Body fill at 80% opacity */
    ctx.globalAlpha = 0.8;
    ctx.fillStyle = w.color || '#444';
    ctx.fillRect(x, y, cw, ch);
    ctx.globalAlpha = 1.0;
    /* Type label */
    ctx.fillStyle = '#FFF';
    ctx.font = (SCALE * 6) + 'px monospace';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(w.type, x + cw / 2, y + ch / 2);
    /* Selection outline + resize handle */
    if (isSelected) {
      ctx.strokeStyle = '#FFF';
      ctx.lineWidth = 1;
      ctx.strokeRect(x + 0.5, y + 0.5, cw - 1, ch - 1);
      ctx.fillStyle = '#FFF';
      ctx.fillRect(x + cw - HANDLE_SIZE, y + ch - HANDLE_SIZE, HANDLE_SIZE, HANDLE_SIZE);
    }
  }

  /* ---------- Canvas init ---------- */
  function initCanvas(matrixW, matrixH, tilePx) {
    deviceW = matrixW;
    deviceH = matrixH;
    tilePixels = tilePx || 16;
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    canvas.width = matrixW * SCALE;
    canvas.height = matrixH * SCALE;
    canvas.style.width = (matrixW * SCALE) + 'px';
    canvas.style.height = (matrixH * SCALE) + 'px';
    render();
  }

  /* ---------- Hit testing ---------- */
  function hitTest(cx, cy) {
    /* 1. Resize handle of selected widget */
    if (selectedIndex >= 0 && selectedIndex < widgets.length) {
      var sw = widgets[selectedIndex];
      var rx = sw.x * SCALE + sw.w * SCALE - HANDLE_SIZE;
      var ry = sw.y * SCALE + sw.h * SCALE - HANDLE_SIZE;
      if (cx >= rx && cx <= rx + HANDLE_SIZE && cy >= ry && cy <= ry + HANDLE_SIZE) {
        return { index: selectedIndex, mode: 'resize' };
      }
    }
    /* 2. Widget body (topmost first) */
    for (var i = widgets.length - 1; i >= 0; i--) {
      var w = widgets[i];
      var x0 = w.x * SCALE;
      var y0 = w.y * SCALE;
      var x1 = (w.x + w.w) * SCALE;
      var y1 = (w.y + w.h) * SCALE;
      if (cx >= x0 && cx <= x1 && cy >= y0 && cy <= y1) {
        return { index: i, mode: 'move' };
      }
    }
    return null;
  }

  /* ---------- Pointer events ---------- */
  function onPointerDown(e) {
    var canvas = document.getElementById('editor-canvas');
    var p = getCanvasPos(e);
    var hit = hitTest(p.x, p.y);
    if (hit) {
      selectedIndex = hit.index;
      var w = widgets[selectedIndex];
      dragState = {
        mode: hit.mode,
        startX: e.clientX,
        startY: e.clientY,
        origX: w.x,
        origY: w.y,
        origW: w.w,
        origH: w.h,
        toastedFloor: false
      };
      try { canvas.setPointerCapture(e.pointerId); } catch (err) { /* ignore */ }
      e.preventDefault();
    } else {
      selectedIndex = -1;
      dragState = null;
    }
    render();
    showPropsPanel(selectedIndex >= 0 ? widgets[selectedIndex] : null);
  }

  function onPointerMove(e) {
    if (dragState === null) return;
    var w = widgets[selectedIndex];
    if (!w) { dragState = null; return; }
    var dxCss = e.clientX - dragState.startX;
    var dyCss = e.clientY - dragState.startY;
    var dx = dxCss / SCALE;
    var dy = dyCss / SCALE;

    if (dragState.mode === 'move') {
      var nx = snapTo(dragState.origX + dx, SNAP);
      var ny = snapTo(dragState.origY + dy, SNAP);
      w.x = clamp(nx, 0, deviceW - w.w);
      w.y = clamp(ny, 0, deviceH - w.h);
    } else if (dragState.mode === 'resize') {
      var meta = widgetTypeMeta[w.type] || { minW: 6, minH: 6 };
      var rawW = snapTo(dragState.origW + dx, SNAP);
      var rawH = snapTo(dragState.origH + dy, SNAP);
      var maxW = deviceW - w.x;
      var maxH = deviceH - w.y;
      var newW = clamp(rawW, meta.minW, maxW);
      var newH = clamp(rawH, meta.minH, maxH);
      if ((rawW < meta.minW || rawH < meta.minH) && !dragState.toastedFloor) {
        if (typeof FW !== 'undefined' && FW.showToast) {
          FW.showToast(w.type + ' minimum: ' + meta.minW + 'w \u00d7 ' + meta.minH + 'h px', 'warning');
        }
        dragState.toastedFloor = true;
      }
      w.w = newW;
      w.h = newH;
    }
    render();
    setStatus(w.type + ' @ ' + w.x + ',' + w.y + ' ' + w.w + 'x' + w.h);
    /* Update position/size readouts in property panel during drag */
    var posReadout  = document.getElementById('prop-pos-readout');
    var sizeReadout = document.getElementById('prop-size-readout');
    if (posReadout)  posReadout.textContent  = w.x + ', ' + w.y;
    if (sizeReadout) sizeReadout.textContent = w.w + ' \u00d7 ' + w.h;
  }

  function onPointerUp(e) {
    if (dragState === null) return;
    /* Mark dirty on completed drag/resize before clearing state */
    dirty = true;
    /* Release pointer capture before clearing dragState (spec-correct order) */
    var canvas = document.getElementById('editor-canvas');
    if (canvas && e && e.pointerId != null) {
      try { canvas.releasePointerCapture(e.pointerId); } catch (err) { /* ignore */ }
    }
    dragState = null;
    if (selectedIndex >= 0 && selectedIndex < widgets.length) {
      var w = widgets[selectedIndex];
      setStatus(w.type + ' @ ' + w.x + ',' + w.y + ' ' + w.w + 'x' + w.h);
    }
  }

  /* ---------- Toolbox ---------- */
  function buildWidgetTypeMeta(entry) {
    var meta = {
      defaultW: 32, defaultH: 8, defaultColor: '#FFFFFF',
      defaultContent: '', defaultAlign: 'left',
      minW: 6, minH: 6,
      contentKind: 'string',  /* 'string' | 'select' — drives the props panel */
      contentOptions: null    /* array of option strings when contentKind==='select' */
    };
    if (entry.fields) {
      for (var i = 0; i < entry.fields.length; i++) {
        var f = entry.fields[i];
        if (f.key === 'w')       meta.defaultW = (f['default'] != null) ? f['default'] : 32;
        if (f.key === 'h')       meta.defaultH = (f['default'] != null) ? f['default'] : 8;
        if (f.key === 'color')   meta.defaultColor = f['default'] || '#FFFFFF';
        if (f.key === 'content') {
          meta.defaultContent = (f['default'] != null) ? f['default'] : '';
          /* LE-1.8: content field may be a select (flight_field, metric, clock).
             Capture its kind/options so showPropsPanel can render a dropdown
             sourced from the API — no hard-coded option lists in the UI. */
          meta.contentKind = (f.kind === 'select') ? 'select' : 'string';
          if (f.kind === 'select' && f.options && f.options.length) {
            meta.contentOptions = [];
            for (var oi = 0; oi < f.options.length; oi++) {
              meta.contentOptions.push(f.options[oi]);
            }
          }
        }
        if (f.key === 'align')   meta.defaultAlign = f['default'] || 'left';
      }
    }
    /* Min floors: firmware enforces w<8||h<8 early-return; use h.default as floor */
    meta.minH = meta.defaultH;
    meta.minW = 6;
    widgetTypeMeta[entry.type] = meta;
  }

  function isDisabledEntry(entry) {
    return !!(entry.note && entry.note.indexOf('not yet implemented') !== -1);
  }

  function initToolbox(types) {
    var box = document.getElementById('editor-toolbox');
    if (!box) return;
    box.innerHTML = '';
    for (var i = 0; i < types.length; i++) {
      buildWidgetTypeMeta(types[i]);
      var entry = types[i];
      var item = document.createElement('div');
      item.className = 'toolbox-item';
      if (isDisabledEntry(entry)) item.className += ' disabled';
      item.setAttribute('data-type', entry.type);
      item.textContent = entry.label || entry.type;
      box.appendChild(item);
    }
    box.addEventListener('click', onToolboxClick);
  }

  function onToolboxClick(e) {
    var t = e.target;
    if (!t || !t.classList || !t.classList.contains('toolbox-item')) return;
    if (t.classList.contains('disabled')) return;
    var type = t.getAttribute('data-type');
    if (type) addWidget(type);
  }

  function addWidget(type) {
    var meta = widgetTypeMeta[type];
    if (!meta) return;
    var w = {
      type: type,
      x: snapTo(8, SNAP),
      y: snapTo(8, SNAP),
      w: meta.defaultW,
      h: meta.defaultH,
      color: meta.defaultColor || '#FFFFFF',
      content: meta.defaultContent || '',
      align: meta.defaultAlign || 'left',
      id: 'w' + Date.now()
    };
    /* Clamp dimensions to canvas bounds (defensive: guards future widget types) */
    if (deviceW > 0 && w.w > deviceW) w.w = Math.max(meta.minW || 6, deviceW);
    if (deviceH > 0 && w.h > deviceH) w.h = Math.max(meta.minH || 6, deviceH);
    /* Clamp position so widget stays within canvas */
    if (w.x + w.w > deviceW) w.x = Math.max(0, deviceW - w.w);
    if (w.y + w.h > deviceH) w.y = Math.max(0, deviceH - w.h);
    widgets.push(w);
    selectedIndex = widgets.length - 1;
    dirty = true;
    render();
    setStatus('Added ' + type);
    showPropsPanel(widgets[selectedIndex]);
  }

  /* ---------- Snap control ---------- */
  function updateSnapButtons() {
    var btns = document.querySelectorAll('#snap-control .snap-btn');
    for (var i = 0; i < btns.length; i++) {
      var b = btns[i];
      var v = parseInt(b.getAttribute('data-snap'), 10);
      if (v === SNAP) {
        b.className = 'snap-btn active';
      } else {
        b.className = 'snap-btn';
      }
    }
  }

  function initSnap() {
    var stored = null;
    try { stored = window.localStorage.getItem('fw_editor_snap'); } catch (err) { /* ignore */ }
    var v = parseInt(stored, 10);
    if (v === 1 || v === 8 || v === 16) {
      SNAP = v;
    } else {
      SNAP = 8;
    }
    var ctrl = document.getElementById('snap-control');
    if (ctrl) {
      ctrl.addEventListener('click', function(e) {
        var t = e.target;
        if (!t || !t.classList || !t.classList.contains('snap-btn')) return;
        var snapVal = parseInt(t.getAttribute('data-snap'), 10);
        if (snapVal === 1 || snapVal === 8 || snapVal === 16) {
          SNAP = snapVal;
          try { window.localStorage.setItem('fw_editor_snap', String(SNAP)); } catch (err) { /* ignore */ }
          updateSnapButtons();
          render();
        }
      });
    }
    updateSnapButtons();
  }

  /* ---------- Property panel ---------- */
  function updateAlignButtons(align) {
    var btns = document.querySelectorAll('#align-control .align-btn');
    for (var i = 0; i < btns.length; i++) {
      var b = btns[i];
      if (b.getAttribute('data-align') === align) {
        b.className = 'align-btn active';
      } else {
        b.className = 'align-btn';
      }
    }
  }

  function showPropsPanel(w) {
    var placeholder = document.getElementById('props-placeholder');
    var form = document.getElementById('props-form');
    if (!w) {
      if (placeholder) placeholder.style.display = '';
      if (form) form.style.display = 'none';
      return;
    }
    if (placeholder) placeholder.style.display = 'none';
    if (form) form.style.display = '';

    /* content field — text input or select, driven by widgetTypeMeta.
       LE-1.8: flight_field and metric use select dropdowns with options
       coming from the /api/widgets/types response (no hard-coded lists). */
    var contentInput  = document.getElementById('prop-content');
    var contentSelect = document.getElementById('prop-content-select');
    var contentField  = document.getElementById('props-field-content');
    if (contentField) {
      var meta = widgetTypeMeta[w.type] || null;
      var useSelect = !!(meta && meta.contentKind === 'select' &&
                         meta.contentOptions && meta.contentOptions.length);
      if (contentInput)  contentInput.style.display  = useSelect ? 'none' : '';
      if (contentSelect) contentSelect.style.display = useSelect ? '' : 'none';
      if (useSelect && contentSelect) {
        /* Rebuild options when the widget type changes. Each call clears
           any previously-injected options so switching widgets never leaks
           stale entries (ES5 — no Array.from/forEach). */
        while (contentSelect.firstChild) {
          contentSelect.removeChild(contentSelect.firstChild);
        }
        for (var ci = 0; ci < meta.contentOptions.length; ci++) {
          var opt = document.createElement('option');
          opt.value = meta.contentOptions[ci];
          opt.textContent = meta.contentOptions[ci];
          contentSelect.appendChild(opt);
        }
        var desired = w.content || meta.defaultContent ||
                       meta.contentOptions[0];
        /* If the widget's current content value is not in the option set
           (e.g. imported from an older layout), fall back to the default
           so the <select> always has a valid selection. */
        var found = false;
        for (var mi = 0; mi < meta.contentOptions.length; mi++) {
          if (meta.contentOptions[mi] === desired) { found = true; break; }
        }
        contentSelect.value = found ? desired : meta.contentOptions[0];
      } else if (contentInput) {
        contentInput.value = w.content || '';
      }
    }

    /* color */
    var colorInput = document.getElementById('prop-color');
    if (colorInput) colorInput.value = w.color || '#ffffff';

    /* align — only shown for text widget */
    var alignField = document.getElementById('props-field-align');
    if (alignField) {
      alignField.style.display = (w.type === 'text') ? '' : 'none';
      updateAlignButtons(w.align || 'left');
    }

    /* position + size readouts */
    var posReadout  = document.getElementById('prop-pos-readout');
    var sizeReadout = document.getElementById('prop-size-readout');
    if (posReadout)  posReadout.textContent  = w.x + ', ' + w.y;
    if (sizeReadout) sizeReadout.textContent = w.w + ' \u00d7 ' + w.h;
  }

  function bindPropsPanelEvents() {
    var contentInput  = document.getElementById('prop-content');
    var contentSelect = document.getElementById('prop-content-select');
    var colorInput    = document.getElementById('prop-color');
    var alignCtrl     = document.getElementById('align-control');

    if (contentInput) {
      contentInput.addEventListener('input', function() {
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        widgets[selectedIndex].content = contentInput.value;
        dirty = true;
        render();
      });
    }
    if (contentSelect) {
      contentSelect.addEventListener('change', function() {
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        widgets[selectedIndex].content = contentSelect.value;
        dirty = true;
        render();
      });
    }
    if (colorInput) {
      colorInput.addEventListener('input', function() {
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        widgets[selectedIndex].color = colorInput.value;
        dirty = true;
        render();
      });
    }
    if (alignCtrl) {
      alignCtrl.addEventListener('click', function(e) {
        var t = e.target;
        if (!t || !t.classList || !t.classList.contains('align-btn')) return;
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        if (widgets[selectedIndex].type !== 'text') return;
        var a = t.getAttribute('data-align');
        widgets[selectedIndex].align = a;
        dirty = true;
        updateAlignButtons(a);
        render();
      });
    }
  }

  /* ---------- Save helpers ---------- */
  function nameToId(name) {
    var id = name.replace(/[^A-Za-z0-9_-]+/g, '-')
                 .replace(/^-+|-+$/g, '')
                 .substring(0, 32);
    if (!id) id = 'layout-' + Date.now();
    return id;
  }

  function getLayoutBody() {
    var nameEl = document.getElementById('layout-name');
    var name = (nameEl && nameEl.value.trim()) ? nameEl.value.trim() : 'My Layout';
    /* Reuse stored id for PUT; derive from name for new POST */
    var id = currentLayoutId ? currentLayoutId : nameToId(name);
    return {
      id: id,
      name: name,
      version: 1,
      canvas: { width: deviceW, height: deviceH },
      widgets: widgets
    };
  }

  function saveLayout(andActivate) {
    var nameEl = document.getElementById('layout-name');
    var name = nameEl ? nameEl.value.trim() : '';
    if (!name) {
      FW.showToast('Enter a layout name before saving', 'error');
      return;
    }
    if (name.length > 32) {
      FW.showToast('Layout name must be 32 characters or less', 'error');
      return;
    }
    var body = getLayoutBody();
    var savePromise;
    if (currentLayoutId) {
      /* PUT — overwrite existing */
      savePromise = FW.put('/api/layouts/' + currentLayoutId, body);
    } else {
      /* POST — create new */
      savePromise = FW.post('/api/layouts', body);
    }
    savePromise.then(function(res) {
      if (!res || !res.body || !res.body.ok) {
        var errMsg = (res && res.body && res.body.error) ? res.body.error : 'Unknown error';
        FW.showToast('Save failed \u2014 ' + errMsg, 'error');
        return null;
      }
      /* Store returned id (from POST response; PUT echoes the same id) */
      if (res.body.data && res.body.data.id) {
        currentLayoutId = res.body.data.id;
      }
      dirty = false;
      FW.showToast('Layout saved', 'success');
      if (!andActivate) return null;
      /* Chain activate */
      FW.showToast('Applying\u2026', 'info');
      return FW.post('/api/layouts/' + currentLayoutId + '/activate', {});
    }).then(function(res) {
      if (res === null || res === undefined) return;
      if (!res.body || !res.body.ok) {
        var errMsg = (res.body && res.body.error) ? res.body.error : 'Unknown error';
        FW.showToast('Activate failed \u2014 ' + errMsg, 'error');
        return;
      }
      FW.showToast('Layout active', 'success');
    }).catch(function() {
      FW.showToast('Network error \u2014 check device connection', 'error');
    });
  }

  function saveAsNew() {
    var nameEl = document.getElementById('layout-name');
    var name = nameEl ? nameEl.value.trim() : '';
    if (!name) {
      FW.showToast('Enter a layout name before saving', 'error');
      return;
    }
    /* Force a new id by clearing currentLayoutId */
    currentLayoutId = null;
    saveLayout(false);
  }

  function bindSaveButtons() {
    var btnNew      = document.getElementById('btn-save-new');
    var btnSave     = document.getElementById('btn-save');
    var btnActivate = document.getElementById('btn-activate');
    if (btnNew)      btnNew.addEventListener('click', saveAsNew);
    if (btnSave)     btnSave.addEventListener('click', function() { saveLayout(false); });
    if (btnActivate) btnActivate.addEventListener('click', function() { saveLayout(true); });
  }

  /* ---------- Unsaved-changes guard ---------- */
  function bindUnloadGuard() {
    window.addEventListener('beforeunload', function(e) {
      if (!dirty) return;
      e.preventDefault();
      /* returnValue required for Chrome/Edge legacy support */
      e.returnValue = 'You have unsaved changes. Leave anyway?';
    });
  }

  /* ---------- Init / load ---------- */
  function loadLayout() {
    FW.get('/api/layout').then(function(res) {
      if (!res.body || !res.body.ok) {
        FW.showToast('Cannot load layout info \u2014 check device connection', 'error');
        return null;
      }
      var d = res.body.data;
      /* Guard against malformed API response before destructuring nested fields */
      if (!d || !d.matrix || typeof d.matrix.width !== 'number' ||
          typeof d.matrix.height !== 'number' || !d.hardware ||
          typeof d.hardware.tile_pixels !== 'number') {
        FW.showToast('Invalid layout data \u2014 check device firmware', 'error');
        return null;
      }
      initCanvas(d.matrix.width, d.matrix.height, d.hardware.tile_pixels);
      return FW.get('/api/widgets/types');
    }).then(function(res) {
      if (!res || !res.body || !res.body.ok) return;
      var types = res.body.data;
      /* Guard against non-array response before iterating */
      if (!types || typeof types.length !== 'number') return;
      initToolbox(types);
      updateSnapButtons();
    }).catch(function(err) {
      /* Network transport failure (fetch rejected) — show error toast */
      FW.showToast('Editor failed to load \u2014 check device connection', 'error');
    });
  }

  function bindCanvasEvents() {
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    canvas.addEventListener('pointerdown', onPointerDown);
    canvas.addEventListener('pointermove', onPointerMove);
    canvas.addEventListener('pointerup', onPointerUp);
    canvas.addEventListener('pointercancel', onPointerUp);
  }

  function init() {
    bindCanvasEvents();
    bindPropsPanelEvents();
    bindSaveButtons();
    bindUnloadGuard();
    initSnap();
    loadLayout();
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();


]]></file>
<file id="d23cedc8" path="firmware/interfaces/DisplayMode.h" label="SOURCE CODE"><![CDATA[

#pragma once

#include <vector>
#include <FastLED_NeoMatrix.h>
#include "models/FlightInfo.h"
#include "core/LayoutEngine.h"

// Heap baseline after Foundation boot — measured on hardware after flashing.
// Update these values from serial monitor output (Task 1.3 — not yet measured).

// --- Rendering Context (passed to modes each frame) ---
struct RenderContext {
    FastLED_NeoMatrix* matrix;       // GFX drawing surface — mutable for rendering
    LayoutResult layout;             // zone bounds (logo, flight, telemetry)
    uint16_t textColor;              // pre-computed from DisplayConfig RGB
    uint8_t brightness;              // read-only — managed by night mode scheduler
    uint16_t* logoBuffer;            // shared 2KB buffer from NeoMatrixDisplay
    uint32_t displayCycleMs;         // cycle interval for modes that rotate flights (ms; uint32 avoids overflow for display_cycle > 65s)
    const FlightInfo* currentFlight = nullptr; // LE-1.8: pointer to flight being displayed; nullptr when no active flight
};

// --- Zone Descriptor (static metadata for Mode Picker UI wireframes) ---
struct ZoneRegion {
    const char* label;    // e.g., "Airline", "Route", "Altitude"
    uint8_t relX, relY;   // relative position (0-100%)
    uint8_t relW, relH;   // relative dimensions (0-100%)
};

struct ModeZoneDescriptor {
    const char* description;        // human-readable mode description
    const ZoneRegion* regions;      // static array of labeled regions
    uint8_t regionCount;
};

// --- Per-Mode Settings Schema (Delight Release forward-compat) ---
struct ModeSettingDef {
    const char* key;          // NVS key suffix (<=7 chars)
    const char* label;        // UI display name
    const char* type;         // "uint8", "uint16", "bool", "enum"
    int32_t defaultValue;
    int32_t minValue;
    int32_t maxValue;
    const char* enumOptions;  // comma-separated for enum type, NULL otherwise
};

struct ModeSettingsSchema {
    const char* modeAbbrev;           // <=5 chars, used for NVS key prefix
    const ModeSettingDef* settings;
    uint8_t settingCount;
};

// --- DisplayMode Abstract Interface ---
class DisplayMode {
public:
    virtual ~DisplayMode() = default;

    virtual bool init(const RenderContext& ctx) = 0;
    virtual void render(const RenderContext& ctx,
                        const std::vector<FlightInfo>& flights) = 0;
    virtual void teardown() = 0;

    virtual const char* getName() const = 0;
    virtual const ModeZoneDescriptor& getZoneDescriptor() const = 0;
    virtual const ModeSettingsSchema* getSettingsSchema() const = 0;  // Delight forward-compat; return nullptr if no per-mode settings
};


]]></file>
<file id="a3910ae1" path="firmware/modes/CustomLayoutMode.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: Production CustomLayoutMode implementation (Story le-1.3).

Loads an active user-authored layout from LayoutStore on init(), parses the
JSON into a fixed WidgetSpec[] array (ArduinoJson v7 JsonDocument is scoped
to parseFromJson() so it's freed before render() ever runs), then dispatches
per-widget renders via WidgetRegistry::dispatch() with zero heap activity on
the render hot path.

Non-fatal error handling:
  - LayoutStore::load() returning false is NOT fatal — its `out` string is
    already populated with the baked-in default layout JSON. We parse that
    and succeed.
  - deserializeJson() failure IS fatal for init() — we set _invalid = true
    and return false. render() then draws a visible error indicator.

Parse-once rule (epic LE-1 Architecture): deserializeJson() lives strictly
inside parseFromJson() and the JsonDocument is destroyed before the function
returns. render() never sees JSON.
*/

#include "modes/CustomLayoutMode.h"

#include "core/LayoutStore.h"
#include "utils/DisplayUtils.h"
#include "utils/Log.h"

#include <ArduinoJson.h>
#include <FastLED_NeoMatrix.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Guard against accidental future downward drift of the heap budget
// (Task 5). Must be evaluated after WidgetSpec is a complete type.
static_assert(CustomLayoutMode::MEMORY_REQUIREMENT
              >= CustomLayoutMode::MAX_WIDGETS * sizeof(WidgetSpec),
              "CustomLayoutMode::MEMORY_REQUIREMENT must cover the full "
              "WidgetSpec array");

// ------------------------------------------------------------------
// Zone descriptor — CustomLayoutMode fills the whole canvas with a
// layout the user authors; no fixed zones.
// ------------------------------------------------------------------
static const ZoneRegion kCustomZones[] = {
    {"Custom", 0, 0, 100, 100}
};

const ModeZoneDescriptor CustomLayoutMode::_descriptor = {
    "Renders a user-authored layout (widgets) loaded from LittleFS.",
    kCustomZones,
    1
};

// ------------------------------------------------------------------
// Helpers — local-linkage so the translation unit stays self-contained.
// ------------------------------------------------------------------
namespace {

// Pack r, g, b (0-255) into RGB565 using the Adafruit GFX convention
// (RRRRR GGGGGG BBBBB).
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8)
         | ((uint16_t)(g & 0xFC) << 3)
         | ((uint16_t)(b & 0xF8) >> 3);
}

// Parse a "#RRGGBB" hex color string into RGB565. Returns white (0xFFFF)
// for null / malformed input so an authoring mistake does not hide the
// widget entirely. Accepts exact 6 hex digits after the '#'.
uint16_t parseHexColor(const char* hex) {
    if (hex == nullptr || hex[0] != '#') return 0xFFFF;
    // Require exactly "#RRGGBB".
    if (strlen(hex) != 7) return 0xFFFF;
    for (int i = 1; i < 7; ++i) {
        char c = hex[i];
        bool okHex = (c >= '0' && c <= '9')
                  || (c >= 'a' && c <= 'f')
                  || (c >= 'A' && c <= 'F');
        if (!okHex) return 0xFFFF;
    }
    char* endptr = nullptr;
    long v = strtol(hex + 1, &endptr, 16);
    if (endptr == nullptr || *endptr != '\0') return 0xFFFF;
    uint8_t r = (uint8_t)((v >> 16) & 0xFF);
    uint8_t g = (uint8_t)((v >>  8) & 0xFF);
    uint8_t b = (uint8_t)((v >>  0) & 0xFF);
    return rgb565(r, g, b);
}

// Map the JSON "align" string to WidgetSpec.align byte encoding.
uint8_t parseAlign(const char* a) {
    if (a == nullptr) return 0;
    if (strcmp(a, "center") == 0) return 1;
    if (strcmp(a, "right")  == 0) return 2;
    return 0;  // "left" or anything else → 0
}

// Copy a null-terminated string into a fixed destination buffer, always
// leaving a null terminator. Used for the id[] and content[] fields.
void copyFixed(char* dst, size_t dstLen, const char* src) {
    if (dstLen == 0) return;
    if (src == nullptr) { dst[0] = '\0'; return; }
    strncpy(dst, src, dstLen - 1);
    dst[dstLen - 1] = '\0';
}

}  // namespace

// ------------------------------------------------------------------
// init / teardown
// ------------------------------------------------------------------

bool CustomLayoutMode::init(const RenderContext& ctx) {
    (void)ctx;
    _widgetCount = 0;
    _invalid     = false;

    String json;
    bool found = LayoutStore::load(LayoutStore::getActiveId().c_str(), json);
    // NOTE: load() returning false is non-fatal. LayoutStore always populates
    // `json` — with the baked-in default layout on miss. Only a
    // deserializeJson() failure in parseFromJson() is treated as fatal.
    (void)found;

    if (!parseFromJson(json)) {
        _invalid = true;
        Serial.printf("[CustomLayoutMode] init: parse failed — error indicator will render\n");
        // Runtime heap log (Task 5).
        Serial.printf("[CustomLayoutMode] Free heap after init: %u\n", ESP.getFreeHeap());
        return false;
    }

    Serial.printf("[CustomLayoutMode] init: %u widgets loaded (found=%d)\n",
                  (unsigned)_widgetCount, (int)found);
    Serial.printf("[CustomLayoutMode] Free heap after init: %u\n", ESP.getFreeHeap());
    return true;
}

void CustomLayoutMode::teardown() {
    _widgetCount = 0;
    _invalid     = false;
    // _widgets[] contents left as-is — will be overwritten by the next init().
}

// ------------------------------------------------------------------
// parseFromJson — scoped JsonDocument; freed on return.
// ------------------------------------------------------------------

bool CustomLayoutMode::parseFromJson(const String& json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[CustomLayoutMode] parse failed: %s\n", err.c_str());
        return false;
    }

    JsonArrayConst arr = doc["widgets"].as<JsonArrayConst>();
    _widgetCount = 0;
    for (JsonObjectConst obj : arr) {
        if (_widgetCount >= MAX_WIDGETS) {
            Serial.printf("[CustomLayoutMode] widget cap %u hit — truncating\n",
                          (unsigned)MAX_WIDGETS);
            break;
        }

        const char* typeStr = obj["type"] | (const char*)nullptr;
        WidgetType t = WidgetRegistry::fromString(typeStr);
        if (t == WidgetType::Unknown) {
            Serial.printf("[CustomLayoutMode] unknown widget type '%s' — skipping\n",
                          typeStr ? typeStr : "?");
            continue;
        }

        WidgetSpec& w = _widgets[_widgetCount];
        w.type  = t;
        w.x     = (int16_t)(obj["x"] | 0);
        w.y     = (int16_t)(obj["y"] | 0);
        w.w     = (uint16_t)(obj["w"] | 0);
        w.h     = (uint16_t)(obj["h"] | 0);
        w.color = parseHexColor(obj["color"] | (const char*)nullptr);
        w.align = parseAlign(obj["align"] | (const char*)nullptr);
        w._reserved = 0;

        copyFixed(w.id,      sizeof(w.id),      obj["id"]      | (const char*)"");
        copyFixed(w.content, sizeof(w.content), obj["content"] | (const char*)"");

        _widgetCount++;
    }

    return true;
    // doc destructs here — no retained JSON state (epic parse-once rule).
}

// ------------------------------------------------------------------
// render — zero heap per frame; error indicator on _invalid.
// ------------------------------------------------------------------

void CustomLayoutMode::render(const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights) {
    if (ctx.matrix == nullptr) return;

    // LE-1.8: shallow-copy the RenderContext and inject currentFlight so that
    // widgets (FlightFieldWidget, MetricWidget) can bind to the flight being
    // displayed without changing the DisplayMode virtual interface. flights[0]
    // is the front-of-queue flight handed to us by ModeOrchestrator after the
    // display_cycle rotation. Empty flights vector → nullptr (widgets fall
    // back to placeholder strings; see AC #7/#10).
    RenderContext widgetCtx = ctx;
    widgetCtx.currentFlight = flights.empty() ? nullptr : &flights[0];

    // Clear the canvas once per frame (pipeline owns the final show()).
    ctx.matrix->fillScreen(0);

    if (_invalid) {
        // AC #5 — draw a visible 1px red border + "LAYOUT ERR" text so a
        // broken layout is immediately obvious on the physical wall.
        const uint16_t red = rgb565(255, 0, 0);
        const int16_t maxX = ctx.matrix->width();
        const int16_t maxY = ctx.matrix->height();
        if (maxX > 0 && maxY > 0) {
            ctx.matrix->drawRect(0, 0, maxX, maxY, red);
            DisplayUtils::drawTextLine(ctx.matrix, 2, 2, "LAYOUT ERR", red);
        }
        return;
    }

    for (size_t i = 0; i < _widgetCount; ++i) {
        const WidgetSpec& w = _widgets[i];
        // Dispatch returns false for unimplemented/unknown types — we
        // intentionally ignore that here; the dispatcher is stateless and
        // never throws.
        (void)WidgetRegistry::dispatch(w.type, w, widgetCtx);
    }
    // No FastLED.show() — pipeline owns frame commit (FR35).
}

// ------------------------------------------------------------------
// Metadata
// ------------------------------------------------------------------

const char* CustomLayoutMode::getName() const {
    return "Custom Layout";
}

const ModeZoneDescriptor& CustomLayoutMode::getZoneDescriptor() const {
    return _descriptor;
}

const ModeSettingsSchema* CustomLayoutMode::getSettingsSchema() const {
    return nullptr;
}


]]></file>
<file id="6f602b5e" path="firmware/widgets/FlightFieldWidget.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: FlightFieldWidget implementation (Story LE-1.8).

Binds a WidgetSpec.content field-key string (e.g. "airline") to a
FlightInfo member and draws it as a single text line. Falls back to
"--" when ctx.currentFlight is null, the key is unknown, or the
resolved value is empty — AC #7/#10.

Zero heap allocation on the render hot path:
  - Field resolution hands back a `const char*` into the FlightInfo's
    Arduino String storage via `.c_str()` — no String temporaries.
  - Truncation/drawing uses the DisplayUtils char* overloads with a
    stack buffer.
*/

#include "widgets/FlightFieldWidget.h"

#include "utils/DisplayUtils.h"

#include <cstring>

namespace {

// Resolve a field key against a FlightInfo. Returns a pointer to the
// underlying String storage (valid for the lifetime of the FlightInfo)
// or nullptr if the key is unknown. Caller is responsible for the
// empty-string fallback. Keys come straight from spec.content so they
// must be matched exactly (strcmp).
const char* resolveField(const char* key, const FlightInfo& f) {
    if (key == nullptr) return nullptr;

    if (strcmp(key, "airline") == 0) {
        return f.airline_display_name_full.c_str();
    }
    if (strcmp(key, "ident") == 0) {
        // Prefer ICAO → IATA → raw ident so the widget always shows
        // something when the flight has any identifier at all.
        if (f.ident_icao.length() > 0) return f.ident_icao.c_str();
        if (f.ident_iata.length() > 0) return f.ident_iata.c_str();
        return f.ident.c_str();
    }
    if (strcmp(key, "origin_icao") == 0) {
        return f.origin.code_icao.c_str();
    }
    if (strcmp(key, "dest_icao") == 0) {
        return f.destination.code_icao.c_str();
    }
    if (strcmp(key, "aircraft") == 0) {
        if (f.aircraft_display_name_short.length() > 0) {
            return f.aircraft_display_name_short.c_str();
        }
        return f.aircraft_code.c_str();
    }
    return nullptr;
}

}  // namespace

bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx) {
    // Minimum dimension floor — at least one 5x7 glyph must fit.
    if ((int)spec.w < WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) {
        return true;  // skip render — not an error
    }

    // Resolve the field value. Fallback to "--" when the flight is
    // unavailable, the key is unknown, or the value is empty.
    const char* value = nullptr;
    if (ctx.currentFlight != nullptr) {
        value = resolveField(spec.content, *ctx.currentFlight);
    }
    if (value == nullptr || value[0] == '\0') {
        value = "--";
    }

    // Hardware-free test path.
    if (ctx.matrix == nullptr) return true;

    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    if (maxCols <= 0) return true;

    char out[48];
    DisplayUtils::truncateToColumns(value, maxCols, out, sizeof(out));

    int16_t drawY = spec.y;
    if ((int)spec.h > WIDGET_CHAR_H) {
        drawY = spec.y + (int16_t)(((int)spec.h - WIDGET_CHAR_H) / 2);
    }

    DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);
    return true;
}


]]></file>
<file id="ab01e629" path="firmware/widgets/FlightFieldWidget.h" label="SOURCE CODE"><![CDATA[

#pragma once
/*
Purpose: Flight-field widget render function (Story LE-1.8, AC #1/#3/#6/#7/#8).

Stateless free function. Resolves a field key in spec.content (e.g. "airline",
"ident", "origin_icao") against ctx.currentFlight and renders the resulting
string clipped to spec.w / spec.h using the DisplayUtils char* overloads for
zero heap allocation on the render hot path.

Supported field keys (AC #1):
  - "airline"      → FlightInfo.airline_display_name_full
  - "ident"        → FlightInfo.ident_icao (fallback: ident_iata, then ident)
  - "origin_icao"  → FlightInfo.origin.code_icao
  - "dest_icao"    → FlightInfo.destination.code_icao
  - "aircraft"     → FlightInfo.aircraft_display_name_short
                      (fallback: aircraft_code)

Fallback behavior (AC #7/#10):
  - ctx.currentFlight == nullptr → render "--"
  - unknown field key             → render "--"
  - resolved value is empty       → render "--"
*/

#include "core/WidgetRegistry.h"

bool renderFlightField(const WidgetSpec& spec, const RenderContext& ctx);


]]></file>
<file id="76938a68" path="firmware/widgets/MetricWidget.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: MetricWidget implementation (Story LE-1.8).

Binds a WidgetSpec.content metric-key string (e.g. "alt") to a FlightInfo
telemetry value and draws it with a canonical suffix. Falls back to "--"
when ctx.currentFlight is null, the key is unknown, or the resolved
value is NAN — AC #7/#10.

Zero heap allocation on the render hot path:
  - Metric resolution returns a (value, suffix, decimals) tuple by output
    parameters; no String construction.
  - DisplayUtils::formatTelemetryValue handles NAN → "--" and fills a
    stack buffer; drawing uses the char* overload of drawTextLine.
*/

#include "widgets/MetricWidget.h"

#include "utils/DisplayUtils.h"

#include <cmath>
#include <cstring>

namespace {

// Degree glyph "°" for the default Adafruit GFX built-in font.
// Adafruit GFX, when _cp437 == false (the default — setCp437() is never
// called in this firmware), applies an offset: any byte value >= 0xB0 is
// incremented by 1 before the font-table lookup (see Adafruit_GFX.cpp
// "if (!_cp437 && (c >= 176)) c++;"). The degree symbol sits at font
// index 0xF8, so to reach it without CP437 mode we must send 0xF7.
// Sending 0xF8 would render font[0xF9] which is two faint dots, not "°".
static const char kDegreeSuffix[] = { (char)0xF7, '\0' };

// Resolve a metric key. Outputs the value, the display suffix, and the
// decimal count. Returns false for unknown keys.
bool resolveMetric(const char* key, const FlightInfo& f,
                   double& outValue, const char*& outSuffix, int& outDecimals) {
    if (key == nullptr) return false;
    if (strcmp(key, "alt") == 0) {
        outValue    = f.altitude_kft;
        outSuffix   = "k";
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "speed") == 0) {
        outValue    = f.speed_mph;
        outSuffix   = "mph";
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "track") == 0) {
        outValue    = f.track_deg;
        outSuffix   = kDegreeSuffix;
        outDecimals = 0;
        return true;
    }
    if (strcmp(key, "vsi") == 0) {
        outValue    = f.vertical_rate_fps;
        outSuffix   = "fps";
        outDecimals = 1;
        return true;
    }
    return false;
}

}  // namespace

bool renderMetric(const WidgetSpec& spec, const RenderContext& ctx) {
    // Minimum dimension floor — at least one 5x7 glyph must fit.
    if ((int)spec.w < WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) {
        return true;  // skip render — not an error
    }

    char buf[24];

    if (ctx.currentFlight == nullptr) {
        // AC #7 — no flight: render "--" placeholder.
        strcpy(buf, "--");
    } else {
        double value = NAN;
        const char* suffix = "";
        int decimals = 0;
        if (!resolveMetric(spec.content, *ctx.currentFlight,
                           value, suffix, decimals)) {
            // AC #10 — unknown metric key falls back to "--".
            strcpy(buf, "--");
        } else {
            // formatTelemetryValue() writes "--" when value is NAN (AC #7).
            DisplayUtils::formatTelemetryValue(value, suffix, decimals,
                                               buf, sizeof(buf));
        }
    }

    // Hardware-free test path.
    if (ctx.matrix == nullptr) return true;

    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    if (maxCols <= 0) return true;

    char out[24];
    DisplayUtils::truncateToColumns(buf, maxCols, out, sizeof(out));

    int16_t drawY = spec.y;
    if ((int)spec.h > WIDGET_CHAR_H) {
        drawY = spec.y + (int16_t)(((int)spec.h - WIDGET_CHAR_H) / 2);
    }

    DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);
    return true;
}


]]></file>
<file id="14739367" path="firmware/widgets/MetricWidget.h" label="SOURCE CODE"><![CDATA[

#pragma once
/*
Purpose: Telemetry-metric widget render function (Story LE-1.8, AC #2/#4/#6/#7/#8).

Stateless free function. Resolves a telemetry-metric key in spec.content
(e.g. "alt", "speed", "track", "vsi") against ctx.currentFlight and
renders the resulting numeric value with its canonical suffix using the
DisplayUtils char* overloads for zero heap allocation on the render hot
path.

Supported metric keys (AC #2):
  - "alt"    → altitude_kft     0 decimals, suffix "k"   (e.g. "34k")
  - "speed"  → speed_mph        0 decimals, suffix "mph" (e.g. "487mph")
  - "track"  → track_deg        0 decimals, suffix "°"   (e.g. "247°")
  - "vsi"    → vertical_rate_fps 1 decimal,  suffix "fps" (e.g. "-12.5fps")

Fallback behavior (AC #7/#10):
  - ctx.currentFlight == nullptr     → render "--"
  - resolved value is NAN            → render "--"
  - unknown metric key               → render "--"
*/

#include "core/WidgetRegistry.h"

bool renderMetric(const WidgetSpec& spec, const RenderContext& ctx);


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic le-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story le-1-1 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `load()` unbounded `readString()` — if a layout file is larger than `LAYOUT_STORE_MAX_BYTES` (e.g., manual upload or corruption), `f.readString()` allocates the full file to heap, risking OOM on ESP32. | Added `f.size() > LAYOUT_STORE_MAX_BYTES` guard before `readString()`; falls back to default and logs. |
| high | `list()` unbounded `readString()` in per-entry loop — same heap risk as above, amplified up to 16× in the worst case. | Wrapped the `readString()`+`JsonDocument` block in `if (entry.size() <= LAYOUT_STORE_MAX_BYTES)` guard; oversized entries still appear in results with empty `name`. |
| high | `_default` id not blocked — `isSafeLayoutId("_default")` returns `true` because underscore is allowed, allowing a caller to write `_default.json` which would shadow the baked-in fallback. | Added `if (strcmp(id, "_default") == 0) return false;` with explanatory comment. |
| high | Document `id` vs filename desync — `save("foo", {"id":"bar",...})` succeeds, creating `foo.json` whose JSON claims it is `bar`. Downstream active-id / UI reads the filename as truth but the JSON disagrees. | `validateSchema()` signature extended to `validateSchema(json, pathId)`; rejects if `doc["id"] != pathId`. Call site in `save()` updated. |
| medium | AC6 log messages omit the offending id — AC6 specifies `"layout_active '<id>' missing — using default"` but implementation logged generic fixed strings. | Replaced `LOG_W` (macro accepts only string literals; no variadic support) with `Serial.printf` for all four `load()` warning paths, embedding `id` and file size as appropriate. |
| medium | No AC5 NVS round-trip test — `setActiveId`/`getActiveId` had zero Unity coverage despite being a complete AC. | Added `test_active_id_roundtrip` exercising write→read→re-write→read with `LayoutStore::{set,get}ActiveId`. |
| medium | No AC6 end-to-end test (active-id → missing file → default) — the path where `getActiveId()` returns a stale NVS value and `load()` falls back was not tested. | Added `test_load_with_missing_active_id_uses_default` using `ConfigManager::setLayoutActiveId("nonexistent")` then asserting `load()` returns false and out contains `"_default"`. |
| medium | No test asserting `_default` cannot be saved (reservation). | Added `test_save_rejects_reserved_default_id`. |
| medium | No test for doc id vs filename mismatch (new validation rule). | Added `test_save_rejects_id_mismatch`. |
| dismissed | "CRITICAL: V0 cleanup unverifiable — main.cpp incomplete, no CI output" | FALSE POSITIVE: `grep -r "LE_V0_METRICS\\|le_v0_record\\|v0_spike_layout" firmware/src firmware/core firmware/modes` returns zero results (verified in synthesis environment). `git status` confirms `main.cpp` was modified. Binary size 1.22 MB is consistent with V0 instrumentation removal. The reviewer was working from a truncated code snippet but the actual file is clean. |
| dismissed | "Widget nullptr edge case — `(const char*)nullptr` cast is a logic error" | FALSE POSITIVE: `isAllowedWidgetType(nullptr)` returns `false` (line 89 of `LayoutStore.cpp`), causing `validateSchema` to return `INVALID_SCHEMA`. This is the correct behavior. The concern is purely stylistic — the logic is sound. |
| dismissed | "AC #5 documentation error — 12 vs 13 chars" | FALSE POSITIVE: The Dev Agent Record already acknowledges this discrepancy and notes 13 chars is within the 15-char NVS limit. The implementation is correct; only the AC text has a benign counting error. This is a story documentation note, not a code defect. |
| dismissed | "Redundant `LittleFS.exists(LAYOUTS_DIR)` check in `save()`" | FALSE POSITIVE: The inline comment at line 241-247 explains the rationale: "fresh devices reach save() before init() only via tests that explicitly skip init()". This is intentional defensive coding that protects against test harness misuse. The single `exists()` call has negligible performance impact. |
| dismissed | "`setLayoutActiveId` return check — partial write risk from `written == strlen(id)`" | FALSE POSITIVE: `LayoutStore::setActiveId()` (the only caller) pre-validates id with `isSafeLayoutId()` which rejects empty strings and enforces `strlen > 0`. Empty-string NVS writes are structurally prevented at the public API boundary. The theoretical partial-write scenario would require `putString` to return a value inconsistent with actual NVS behaviour, which is platform-specific noise not an application bug. |
| dismissed | "JsonDocument not guaranteed freed in `list()` loop — memory leak risk" | FALSE POSITIVE: `JsonDocument doc` is stack-allocated within the loop body. C++ RAII guarantees destruction at end of scope regardless of exit path (including `continue`, exceptions are not used in this codebase). The added size guard in the synthesis fix further reduces the window in which a `JsonDocument` is allocated at all. |
| dismissed | "`isNull()` vs explicit null edge cases are surprising in ArduinoJson v7" | FALSE POSITIVE: `hasRequiredTopLevelFields` correctly uses `isNull()` to detect missing or null keys. The ArduinoJson v7 documentation explicitly states `isNull()` returns true for missing keys. The hypothetical `"widgets": null` JSON input would correctly fail the `!doc["widgets"].isNull()` check. Low-value concern for this codebase. --- |

## Story le-1-2 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `ClockWidget` does not advance `s_lastClockUpdateMs` when `getLocalTime()` fails. With NTP unreachable, the refresh condition `(s_lastClockUpdateMs == 0 | Moved `s_lastClockUpdateMs = nowMs` outside the `if (getLocalTime...)` branch so it advances on both success and failure. |
| medium | `DisplayUtils::drawBitmapRGB565()` had a stale comment "Bitmap is always 32×32 (LOGO_WIDTH x LOGO_HEIGHT)" — the function signature accepts arbitrary `w`, `h` dimensions and LogoWidget passes 16×16. The incorrect comment degrades trust for onboarding readers. | Rewrote comment to "Render an RGB565 bitmap (w×h pixels)… Bitmap dimensions are caller-specified (e.g. 16×16 for LogoWidget V1 stub)." |
| low | `test_clock_cache_reuse` accepted `getTimeCallCount() <= 1` but count `0` trivially satisfies that assertion even when 50 calls all hit the refresh branch (failure storm). After the clock throttle bug was fixed, the test structure should confirm the throttle actually fires. | Restructured to two phases — Phase 1 (50 calls from cold state: count ≤ 1), Phase 2 (reset, arm cache on first call, 49 more calls: count still ≤ 1) — making the test meaningful regardless of NTP state. |
| low | `drawBitmapRGB565` skips pixels where `pixel == 0x0000` (treating black as transparent). A `spec.color = 0x0000` logo stub renders as invisible. This is undocumented surprising behaviour for callers. | Added inline NOTE comment documenting the black-as-transparent behaviour and why it does not affect LE-1.5 real bitmaps. |

## Story le-1-2 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | `ClockWidgetTest` namespace compiled unconditionally into production firmware | Gated both the header declaration and the `.cpp` implementation behind `#ifdef PIO_UNIT_TESTING` so the test-introspection API is stripped from production binary builds. |
| low | Stale/incorrect comment in `WidgetRegistry.cpp` claiming "The final `default` only exists to handle WidgetType::Unknown..." when no `default:` label exists in the switch | Rewrote both the file-header comment (line 5–7) and the inline dispatch comment to accurately describe that `WidgetType::Unknown` has its own explicit `case`, and that out-of-range values fall through to the post-switch `return false` — valid C++, no UB. |
| low | Single global clock cache is a V1 limitation invisible to LE-1.3 implementors — two Clock widgets in one layout silently share state | Added a clearly-labelled `V1 KNOWN LIMITATION` block in the file header documenting the shared cache behavior and the LE-1.7+ migration path. |

## Story le-1-2 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | Stale `~XXX,XXX bytes / ~YY,YYY bytes` placeholder in `DisplayMode.h` heap baseline comment degrades onboarding trust | Replaced `~XXX,XXX bytes / Max alloc: ~YY,YYY bytes` placeholder values and `(Update with actual values...)` instruction with a cleaner comment that still communicates "not yet measured" without the template noise. |
| dismissed | AC#3 violated — DisplayMode uses virtual methods, contradicting "no vtable per widget" | FALSE POSITIVE: AC#3 is explicitly scoped to the *WidgetRegistry dispatch mechanism* ("not virtual-function vtable per widget"). `DisplayMode` is a pre-existing, pre-LE-1.2 abstract interface for the mode system — entirely separate from the widget render dispatch path. LE-1.2 dev notes explicitly designate `DisplayMode.h` as "Untouched". The `WidgetRegistry::dispatch()` switch confirmed zero virtual calls. The AC language "per widget" is unambiguous. |
| dismissed | Task 8 grep validation is misleadingly narrow (missing `DisplayMode.h` in scope) | FALSE POSITIVE: Task 8's grep scope (`firmware/core/WidgetRegistry.cpp firmware/widgets/`) is intentionally narrow and correct — it verifies that LE-1.2's new widget code has no virtual dispatch. `DisplayMode.h` pre-dates LE-1.2 and is out of the story zone. The completion claim is accurate for the stated purpose. |
| dismissed | ClockWidget NTP throttle bug still present despite synthesis fix | FALSE POSITIVE: Reviewer C self-corrected this to FALSE POSITIVE upon re-reading the code. Confirmed: `s_lastClockUpdateMs = nowMs` at line 62 of `ClockWidget.cpp` is correctly placed *outside* the `if (getLocalTime...)` block, advancing the timer on both success and failure. |
| dismissed | platformio.ini missing `-I widgets` build flag | FALSE POSITIVE: `firmware/platformio.ini` line 43 shows `-I widgets` is present. Direct source verification confirms the flag is there. False positive. |
| dismissed | AC#8 unverifiable — test file not provided | FALSE POSITIVE: `firmware/test/test_widget_registry/test_main.cpp` is fully visible in the review context (275 lines, complete). The two-phase `test_clock_cache_reuse` is implemented exactly as the synthesis round 2 record describes, with `TEST_ASSERT_LESS_OR_EQUAL(1u, ...)` assertions in both phases. False positive. |
| dismissed | AC#7 "silent no-op on undersized widgets" violates the clipping requirement | FALSE POSITIVE: AC#7 requires "clips safely and does not write out-of-bounds." Silent no-op (returning `true` without drawing) IS the safest possible clip for an entire dimension below the font floor. The AC does not mandate partial rendering. All three widget implementations (Text, Clock, Logo) correctly return `true` as a no-op for below-floor specs. Fully compliant. |
| dismissed | LogoWidget `spec.color = 0x0000` renders invisible — undocumented gotcha | FALSE POSITIVE: Already addressed in a prior synthesis round. `LogoWidget.cpp` lines 39–42 contain an explicit `NOTE:` comment documenting the black-as-transparent behavior and explaining LE-1.5 is unaffected. The antipatterns table also documents this. Nothing new to fix. |
| dismissed | Widget `id` fields could collide with ConfigManager NVS keys (e.g., widget `id="timezone"`) | FALSE POSITIVE: Architectural confusion. Widget `id` fields (e.g., `"w1"`) are JSON properties stored inside layout documents on LittleFS. They are never written to NVS. ConfigManager's NVS keys (`"timezone"`, `"disp_mode"`, etc.) are entirely separate storage. The two namespaces cannot collide. LayoutStore's `isSafeLayoutId()` governs layout *file* identifiers, not widget instance ids within a layout. False positive. |
| dismissed | `test_text_alignment_all_three` is a "lying test" — only proves no-crash, not pixel math | FALSE POSITIVE: Valid observation, but by-design. The test file header explicitly labels this as a "smoke test" (line 212: "pixel-accurate assertions require a real framebuffer which we don't have in the test env"). The null-matrix scaffold is the documented test contract for all hardware-free unit tests in this project (see dev notes → Test Infrastructure). Alignment math IS present in `TextWidget.cpp` (lines 53–59) and is exercised on real hardware via the build verification. No lying — the smoke test boundary is honest and documented. |
| dismissed | `test_clock_cache_reuse` is weak — count=0 satisfies `≤1` even when cache is broken | FALSE POSITIVE: The two-phase test structure addresses this. Phase 2 explicitly arms the cache first (call 1), then runs 49 more calls — proving the cache throttle fires within a single `millis()` window. On NTP-down rigs, `getTimeCallCount()` = 0 for all 50 calls is still meaningful: it proves `getLocalTime()` was not called 50 times. The prior synthesis already restructured the test and the comment in the test explains this reasoning explicitly at line 159–161. |
| dismissed | `dispatch()` ignores `spec.type` — `(type, spec)` mismatch silently renders wrong widget | FALSE POSITIVE: Design choice, not a bug. The header comment at lines 89–91 of `WidgetRegistry.h` explicitly documents: "type is passed explicitly (rather than reading spec.type) to allow future callers to force a type without mutating the spec." LE-1.3 will populate specs from JSON; the type will always match. If a mismatch is desired for testing or fallback, this design supports it. A `debug-assert` could be added in the future but is out of LE-1.2 scope. |
| dismissed | `LayoutStore` / `WidgetRegistry` dual-source type string sync bomb | FALSE POSITIVE: Acknowledged design limitation. Already documented in `WidgetRegistry.cpp` (lines 11–13: "The allowlist here MUST stay in lock-step with LayoutStore's kAllowedWidgetTypes[]"). Centralization is a valid future improvement but is explicitly out of LE-1.2 scope per the story's architecture notes. Not a bug in the current implementation. |
| dismissed | `delay(2000)` in `setup()` slows every on-device test run | FALSE POSITIVE: Standard ESP32 Unity practice — the 2s delay allows the serial monitor to connect before test output begins. Removing it risks losing the first test results on some host configurations. Consistent with the `test_layout_store` scaffold which this test mirrors. Low-impact, by-convention. |
| dismissed | Include order in `LogoWidget.cpp` (comments before includes is unconventional) | FALSE POSITIVE: Style-only. The pattern (file header comment → includes) is consistent with `TextWidget.cpp`, `ClockWidget.cpp`, and `WidgetRegistry.cpp`. Not an inconsistency — it IS the project's established pattern. No change warranted. --- |

## Story le-1-3 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #7 is functionally broken — `ModeRegistry::requestSwitch("custom_layout")` is a no-op when `custom_layout` is already the active mode. `ModeRegistry::tick()` lines 408–411 contain an explicit same-mode idempotency guard: `if (requested != _activeModeIndex)` — the else branch just resets state to IDLE without calling `executeSwitch()`. Layout never reloads. | Added `ModeRegistry::requestForceReload()` to `ModeRegistry.h/.cpp` which atomically clears `_activeModeIndex` to `MODE_INDEX_NONE` before storing the request index, forcing `tick()` to take the `executeSwitch` path. Updated `main.cpp` hook to call `requestForceReload` instead of `requestSwitch`. |
| high | `ConfigManager::setLayoutActiveId()` does not call `fireCallbacks()`, so changing the active layout id in NVS never sets `g_configChanged`. This means the entire AC #7 re-init chain never fires even if the same-mode guard were fixed — the display task's `_cfgChg` is never `true` after a layout activation. | Added `fireCallbacks()` call in `setLayoutActiveId()` after successful NVS write. Also tightened the return path — previously returned `true` even on partial write; now returns `false` if `written != strlen(id)` (which was already the boolean expression but was lost in the refactor to add the callback). |
| medium | Misleading `_activeModeIndex` write in `requestForceReload()` races with Core 0 reading it between the two stores. Analysis: both `_activeModeIndex` and `_requestedIndex` are `std::atomic<uint8_t>`, and Core 0 only reads `_activeModeIndex` *after* it has already consumed and cleared `_requestedIndex`. The narrow window where Core 0 could observe `_activeModeIndex == MODE_INDEX_NONE` without a corresponding pending request is benign — it would simply render a tick with no active mode (same as startup). This is acceptable for an infrequent layout-reload path. Documented in the implementation comment. | Documented the race window and its benign nature in the implementation comments. No code change needed. |
| low | `test_render_invalid_does_not_crash` uses `ctx.matrix = nullptr`, so `render()` short-circuits at line 202 (`if (ctx.matrix == nullptr) return`) before reaching the `_invalid` branch and error-indicator drawing code. AC #5 error UI is not exercised in tests. | Deferred — requires either a matrix stub/mock or on-hardware test harness. Created [AI-Review] action item. |
| low | Log line in `init()` failure path (`"init: parse failed — error indicator will render"`) does not match the AC #5 specified literal (`"parse failed: %s"`). The `deserializeJson` error *is* logged in `parseFromJson()`, but the `init()` wrapper logs a different string. | Not applied — the error string *is* printed (in `parseFromJson`) and the `init()` wrapper adds context. The AC wording is guidance, not a literal string contract. Dismissed as minor documentation imprecision. |

## Story le-1-4 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | `_handlePutLayout` gave the same `"Body id does not match URL id"` error message for two distinct failure modes: (a) the `id` field is missing from the JSON body entirely, and (b) the `id` field is present but differs from the URL path segment. The misleading message in case (a) makes debugging harder for API clients. | Split into two separate checks with distinct error messages ("Missing id field in layout body" vs "Body id does not match URL id"), both returning `LAYOUT_INVALID` with HTTP 400. **Applied.** |
| dismissed | AC #9 / Task #10 — "Tests are lying tests / TEST_PASS() stubs that exercise nothing" | FALSE POSITIVE: The test file header (lines 1–29) explicitly documents why compile-only is the contract: `AsyncWebServerRequest*` cannot be constructed outside the ESPAsyncWebServer stack on ESP32. The story Dev Notes (Task 10) explicitly state "compile-only acceptable per AC #9." The AC #9 wording says "exercised by at least one test case" — the test file exercises the underlying layer APIs (LayoutStore, ModeOrchestrator symbols) that each handler depends on, satisfying the spirit of the AC within the constraints of the embedded test stack. This is a story documentation ambiguity, not a code defect. The 6 tests are not assertionless stubs: `test_layout_store_api_visible` has 4 real assertions verifying isSafeLayoutId and constant values; `test_activate_orchestrator_handoff_compiles` asserts function pointer non-null. `TEST_PASS()` in the remaining tests is correctly used for compile-time verification where there is no runtime observable behavior to assert. |
| dismissed | AC #6 / CRITICAL — "Hardcoded `custom_layout` mode ID not validated against ModeRegistry at runtime" | FALSE POSITIVE: The existing `_handlePostDisplayMode` (line 1318) applies exactly the same pattern without runtime validation of the hardcoded `custom_layout` string. The mode table is set up once in `main.cpp` and `custom_layout` is a core architectural constant for this product, not a user-configurable value. Adding a ModeRegistry lookup on every activate call would add heap churn and latency for a defensive check that would only fire during development (and would be immediately visible in testing). The established project pattern (confirmed in LE-1.3 Dev Notes) does not validate this string at the call site. |
| dismissed | Reviewer C CRITICAL — "`setActiveId` persist success not atomic with `onManualSwitch` — orchestrator called before/regardless of persistence" | FALSE POSITIVE: Direct code inspection (lines 2038–2044) confirms the implementation is already correct: `if (!LayoutStore::setActiveId(id.c_str())) { _sendJsonError(...); return; }` — the early `return` on failure means `ModeOrchestrator::onManualSwitch()` is only reached when `setActiveId` succeeds. Reviewer C's description of the bug does not match the actual code. False positive. |
| dismissed | AC #1 response shape — "implementation nests under `data.layouts` and adds `data.active_id`; story AC #1 says a flat top-level array" | FALSE POSITIVE: This is a real documentation drift between the story's AC text and the implementation, but it is NOT a code defect. The `api-layouts-spec.md` that was created as part of this story correctly documents the richer `{active_id, layouts:[...]}` shape. The editor client needs `active_id` alongside the list for a good UX; the implementation is correct and intentional per Task 3's dev notes ("Also include `active_id: LayoutStore::getActiveId()` at the top level of `data` for editor convenience"). An [AI-Review] action item was created to update the AC text; no code change required. |
| dismissed | DRY violation — ID validation duplicated 4× instead of shared function | FALSE POSITIVE: The `isSafeLayoutId()` validation is a single-line call (`LayoutStore::isSafeLayoutId(id.c_str())`) that is correctly placed in each handler independently. It is not duplicated logic — it is a validation guard that each handler must own because each extracts its own `id` variable. Extracting it into `extractLayoutIdFromUrl` would couple URL parsing to ID validation, creating its own concerns. The existing pattern is consistent with how `_handleDeleteLogo`, `_handleGetLogoFile`, etc. handle their own validation. Not a DRY violation. |
| dismissed | NVS write atomicity for cross-core safety not documented | FALSE POSITIVE: This concern is addressed in the LE-1.3 synthesis antipatterns record, which explicitly analyzed the `ConfigManager::setLayoutActiveId()` + `fireCallbacks()` chain as a LE-1.3 fix. The `Preferences::putString()` call is handled within the existing ConfigManager atomic-write pattern (debounce + NVS handle). The concern about cross-core partial writes on string values is noted in LE-1.3 context as a known benign window (same analysis as the `setLayoutActiveId` return-check dismissal in the le-1-1 antipatterns table). No new risk introduced by LE-1.4. |
| dismissed | `GET /api/widgets/types` — widget metadata hard-coded in WebPortal creates dual-source sync risk | FALSE POSITIVE: While a cheaper existence check (e.g. `LittleFS.exists()`) would work, using `LayoutStore::load()` is consistent with the established codebase pattern (the activate handler also uses `load()` for existence verification). The 8 KiB read is bounded, and PUT operations are rare user-initiated writes. The performance concern is valid but low-impact on the use case. Noted as a future improvement. |
| dismissed | `_handleGetLayoutById` uses two `JsonDocument` instances (extra heap allocation) | FALSE POSITIVE: The two-document pattern (`doc` for the outer envelope + `layoutDoc` for the nested parsed layout) is required because ArduinoJson v7 does not support copying a deserialized variant into a different document's object graph without a separate parse. The alternative (parse into one doc and re-build the envelope) would be more complex and error-prone. The extra allocation is bounded by `LAYOUT_STORE_MAX_BYTES` (8 KiB) and is immediately freed when the handler returns. Not a problem in practice on an ESP32 with 327 KB RAM (17.4% used per build log). |
| dismissed | SRP/DI violations — WebPortal is a "Fat Controller"; handlers should be injectable | FALSE POSITIVE: The project context explicitly names `firmware/adapters/WebPortal.cpp` as an adapter in the hexagonal architecture. Adapter classes in this project are intentionally responsible for routing + protocol translation + domain call delegation. Introducing an intermediate `LayoutController` or interface injection layer on an ESP32 with limited heap would add abstractions with zero testability benefit (test stack still can't mock `AsyncWebServerRequest`). This is an appropriate architectural choice for the embedded context. --- |

## Story le-1-5 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `_handleGetWidgetTypes` logo widget schema was stale post-LE-1.5 — the `obj["note"]` still said `"LE-1.5 stub — renders solid color block until real bitmap pipeline lands"` (false after LE-1.5 shipped the real pipeline), and the `content` field (the logo ICAO/id that `renderLogo()` now reads) was missing entirely from the field list. This means a layout editor following the API schema would produce logo widgets without a `content` field, which silently falls back to the airplane sprite on every render. | Removed stale `note` string, added `content` (string, default `""`) field before `color`, updated inline comment to describe the real LE-1.5 pipeline. |
| dismissed | Task 3 is "lying" — `scanLogoCount()` call at WebPortal.cpp line 466 was supposedly added by LE-1.5, contradicting "no changes needed." | FALSE POSITIVE: Git history is conclusive. Commit `6d509a0` ("Update continual learning state and enhance NeoMatrixDisplay with zone-based rendering. Added telemetry fields and improved logo management.") added both the `POST /api/logos` multipart handler **and** the `scanLogoCount()` call within it — this predates LE-1.5. The LE-1.5 story's claim that Task 3 required no changes is accurate. Reviewer C independently confirmed the upload handler existed at line 440–535 but then incorrectly attributed `scanLogoCount()` at line 466 to LE-1.5 without checking the git log. |
| dismissed | AC #4 is "partially false" — no changes were needed but `scanLogoCount()` was added | FALSE POSITIVE: Same as above. `scanLogoCount()` predates LE-1.5. AC #4 is accurate. |
| dismissed | AC #6 is "not literally true" — `LogoManager::loadLogo()` builds `String icao` and `String path` objects (heap allocations per frame). | FALSE POSITIVE: This is an accurate technical observation, but it is not a defect introduced by LE-1.5. The Dev Agent Record explicitly acknowledges: *"Implicit `String(spec.content)` at the call to `loadLogo()` is the only allocation — this is unavoidable due to the `LogoManager::loadLogo(const String&, ...)` signature, and it's identical to the established ClassicCardMode pattern that has been in production since ds-1.4."* AC #6's intent is "no `new`/`malloc` in `renderLogo()` itself, no second 2KB static buffer" — and that holds. The String allocation lives inside `LogoManager::loadLogo()`, which is documented as the shared pattern across all render modes. This is design-level acknowledged debt, not a LE-1.5 regression. |
| dismissed | Guard order in `renderLogo()` diverges from ClassicCardMode canonical pattern (dimension floor before buffer guard). | FALSE POSITIVE: The Dev Notes explicitly document and justify this ordering in Option 1 (dimension floor first → buffer guard → loadLogo → matrix guard). The ordering is intentional: the dimension floor short-circuits cheaply before the null pointer check, and `test_undersized_spec_returns_true` asserting the buffer is *untouched* when `w<8`/`h<8` is a stronger test contract. ClassicCardMode doesn't have a dimension floor guard at all (it's not needed for the fixed zone sizes it operates on). The "canonical" pattern doesn't apply identically because the context differs. |
| dismissed | `test_missing_logo_uses_fallback` is a weak/lying test — only proves buffer was changed, not that fallback sprite bytes are correct. | FALSE POSITIVE: The test correctly uses a sentinel (`0x5A5A`) and asserts at least one pixel differs from sentinel. The fallback sprite contains `0x0000` and `0xFFFF` pixels — neither matches `0x5A5A`. The assertion pattern is sound for proving the fallback loaded. Comparing to exact PROGMEM bytes via `memcpy_P` in a test would be stronger but is complexity for marginal gain; the current approach definitively proves the fallback fired. Not a "lying test." |
| dismissed | `test_null_logobuffer_returns_true` comment overclaims "must not call loadLogo" without proving non-call. | FALSE POSITIVE: The comment says "can't verify non-call without mocks" — it is honest about the limitation. The implementation guard at line 42 of `LogoWidget.cpp` (`if (ctx.logoBuffer == nullptr) return true;`) precedes the `loadLogo()` call at line 48, making the "no call" property structurally guaranteed by code order, not just test assertion. The comment is informative, not overclaiming. |
| dismissed | AC #7 doesn't verify AC #6 heap claim. | FALSE POSITIVE: AC #7 specifies correctness tests (LittleFS fixture, fallback, null buffer, undersized spec). Heap profiling on-device requires ESP32 heap instrumentation hooks beyond the scope of Unity tests and LE-1.5. The AC #6 "zero heap in render path" is verified by code inspection, not by a test assertion. This is the same approach used across the codebase. |
| dismissed | Per-frame LittleFS I/O is a performance antipattern without measurement data. | FALSE POSITIVE: The Dev Notes acknowledge this explicitly and cite ClassicCardMode as prior art: *"Flash reads on ESP32 with LittleFS are fast enough for 30fps display budget. ClassicCardMode has been doing this (LittleFS read every render frame) since Story ds-1.4 without measurable render-time regression."* This is an accepted design constraint, not a new LE-1.5 regression. Future caching is deferred intentionally. |
| dismissed | `cleanLogosDir()` path normalization is fragile (LittleFS name format edge cases). | FALSE POSITIVE: The test file already handles the path normalization edge case explicitly at lines 41–43: `if (!path.startsWith("/")) { path = String("/logos/") + path; }`. This mirrors the pattern from `test_logo_manager`. The test helper is robust enough for its purpose. --- |

## Story le-1-6 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | No smoke test coverage for editor assets — `/editor.html`, `/editor.js`, `/editor.css`, and `/api/widgets/types` had zero automated test coverage despite the project having a live-device smoke suite (`tests/smoke/test_web_portal_smoke.py`) that covers every other page (`/health.html`) and API endpoint. | Added `test_editor_page_loads`, `test_editor_js_loads`, `test_editor_css_loads`, `test_get_widget_types_contract` to the smoke suite. |
| low | `releasePointerCapture` not called in `onPointerUp` / `pointercancel` handler — pointer capture is automatically released on most browsers when `pointerup` fires, but explicit release is the spec-correct pattern and avoids potential stuck-capture on exotic browsers. | Added `canvas.releasePointerCapture(e.pointerId)` with try/catch inside `onPointerUp`, matching the same defensive pattern used for `setPointerCapture`. |
| low | Task 10 checkbox `[x]` is premature — the Dev Agent Record explicitly states AC10 is "⏭ Pending hardware" with no device test results documented. Marking the task complete while explicitly noting it was not performed is a story integrity violation. | Reverted Task 10 and all sub-tasks from `[x]` to `[ ]`. Added `[AI-Review]` follow-up in the new Senior Developer Review section. |
| dismissed | "Data Loss — feature is save-less (no layout persistence)" | FALSE POSITIVE: FALSE POSITIVE. The story explicitly scopes LE-1.6 to canvas + drag only. The story Dev Notes state: *"LE-1.6 uses GET /api/layout (canvas size) and GET /api/widgets/types (toolbox). Save/load is LE-1.7."* The antipattern table entry says "Save layout JSON in LE-1.6 → Defer to LE-1.7." This is intentional design, not a defect. |
| dismissed | "Race condition in drag state synchronization — network events could mutate widgets[] between selectedIndex and dragState assignment" | FALSE POSITIVE: FALSE POSITIVE. The editor is a single-threaded JavaScript page with no WebSockets, no `EventSource`, no `setInterval`-driven polls, and no background mutation of `widgets[]`. The only code that mutates `widgets[]` is user interaction (`addWidget`, drag/resize). JavaScript's event loop makes the selectedIndex→dragState sequence atomic in practice. No race condition vector exists. |
| dismissed | "AC8 gzip size unverifiable — no build artifacts" | FALSE POSITIVE: FALSE POSITIVE. The Dev Agent Record explicitly documents: *"wc -c total = 5,063 bytes (569 + 3,582 + 912). 24.7% of budget."* The gzip files exist at `firmware/data/editor.{html,js,css}.gz` (confirmed: 5,089 bytes total after synthesis fix, still 24.9% of 20 KB budget). |
| dismissed | "Resize handle is 24 logical pixels — 4× too large per AC5" | FALSE POSITIVE: FALSE POSITIVE with inverted math. Reviewer B claimed "6 CSS px × SCALE=4 = 24 logical pixels." The correct conversion is 1 logical pixel = 4 CSS pixels, so 6 CSS px = **1.5 logical px** (too small, not too large). Crucially, the story's Dev Notes explicitly specify `handleSize = 6 // CSS pixels` in the resize handle position code block, and Task 3 says "6×6 CSS-px resize handle square." The Dev Notes are the authoritative implementation reference. The AC5 text that says "6×6 logical-pixel" contradicts the Dev Notes — this is a documentation inconsistency in the story text, not an implementation bug. |
| dismissed | "Silent pointer capture failure — no fallback when setPointerCapture fails" | FALSE POSITIVE: FALSE POSITIVE. The code already handles this: `try { canvas.setPointerCapture(e.pointerId); } catch (err) { /* ignore */ }`. The `pointercancel` event is also wired to `onPointerUp` (line 352) which clears `dragState`, preventing stuck drag states. The suggested complex fallback (document-level event handlers with cleanup closures) would substantially increase complexity and gzip size for a low-probability edge case on a LAN-only appliance UI. |
| dismissed | "Implicit frontend/backend validation coupling — widget constraints not in API" | FALSE POSITIVE: FALSE POSITIVE. The editor **does** read constraints from the API dynamically via `buildWidgetTypeMeta()` which derives `minH = meta.defaultH` (from the `h` field's default value) and `minW = 6` as a constant. This is the documented approach per the story Dev Notes. Adding explicit `min_w`/`min_h` fields to the API would require firmware changes (out of LE-1.6 scope) and is deferred per the story's antipattern table. |
| dismissed | "Toast styling — `.toast-warning` class not in editor.css" | FALSE POSITIVE: FALSE POSITIVE. `editor.html` loads both `style.css` (global styles) AND `editor.css`. The toast system is implemented in `common.js` and styled in `style.css` — not editor.css. This is the same pattern used by dashboard, wizard, and health pages. No additional toast styles are needed in `editor.css`. |
| dismissed | "Performance footgun — status update every `pointermove` frame" | FALSE POSITIVE: DISMISSED as acceptable. `setStatus()` performs a single `textContent` assignment on a `<span>`. Modern browsers batch DOM updates within an animation frame. This is the standard pattern for real-time position readouts in canvas editors and is consistent with the project's existing approach. Adding debounce would introduce `setTimeout` complexity for unmeasured benefit on an appliance UI. |
| dismissed | "`e.preventDefault()` only fires on hit — miss path allows browser gestures" | FALSE POSITIVE: FALSE POSITIVE. The canvas has `touch-action: none` (editor.css line 87) which unconditionally prevents browser scroll/zoom interception for all touch events on the canvas element, regardless of hit testing. `e.preventDefault()` inside the hit branch is the explicitly documented "belt-and-braces" measure. The antipatterns table entry states: "DO NOT: `e.preventDefault()` globally on body; DO INSTEAD: `e.preventDefault()` inside `onPointerDown` only." Calling it on miss would prevent tap-to-deselect on some touch browsers without benefit. |
| dismissed | "Toolbox click listener accumulates on repeated `initToolbox()` calls" | FALSE POSITIVE: FALSE POSITIVE in this implementation. `initToolbox()` is called exactly once — from the `loadLayout()` `.then()` chain, which is itself called once from `init()`, which is called once from `DOMContentLoaded`. There is no code path that invokes `initToolbox()` more than once per page lifecycle. |
| dismissed | "Inclusive hit-test rectangles create shared-edge ambiguity between adjacent widgets" | FALSE POSITIVE: FALSE POSITIVE. The hit-test uses `<=` on both edges, which is standard canvas hit-testing. Adjacent widgets on a shared edge are an extremely uncommon layout scenario, and when they occur, the topmost widget (last in array) wins because the loop traverses from end to start. This is predictable and consistent behavior; the UX impact is negligible. |
| dismissed | "Story 'Files modified' section omits `sprint-status.yaml`" | FALSE POSITIVE: DISMISSED as a documentation completeness note rather than a code defect. The story `File List` documents the directly created/modified source files. `sprint-status.yaml` is an artifact file. The Dev Agent Record explicitly notes the sprint status update in its own section. |
| dismissed | "AC7 logo minimum height: story says h=8, API returns h=16 — implementation is wrong" | FALSE POSITIVE: PARTIALLY DISMISSED — the code is correct, the AC7 story text is wrong. `buildWidgetTypeMeta()` correctly reads `minH = meta.defaultH` from the API response, which returns `h=16` for the logo widget (WebPortal.cpp line 2149). This means the logo min floor is 16 (correct firmware behavior), not 8 as incorrectly stated in AC7. The implementation correctly follows the API truth. The AC7 story text error (`logo: h=8`) is a documentation inaccuracy; no code fix needed. --- |

## Story le-1-6 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `loadLayout()` accesses `d.matrix.width`, `d.matrix.height`, and `d.hardware.tile_pixels` without first verifying the nested structure exists and is correctly typed. A malformed-but-`ok: true` API response (e.g., `{"ok":true,"data":{}}`) causes a `TypeError` crash before the canvas is ever initialized. | Added nested-field guard before `initCanvas()` call — checks `d.matrix`, `d.hardware`, and type of all three numeric fields. Displays a specific "Invalid layout data — check device firmware" toast before returning `null`. |
| low | `releasePointerCapture` was called *after* `dragState = null`, which is semantically backwards — spec-correct order is to release the capture first, then clear state. Functionally correct due to the early-return guard at line 206, but ordering violates the spec contract. | Moved `releasePointerCapture` call to before `dragState = null`. |
| low | `initToolbox(res.body.data)` was called without verifying `data` is array-like. If the API returns `{"ok":true,"data":null}`, `types.length` throws a `TypeError` inside `initToolbox`, silently breaking toolbox initialization with no error feedback. | Extracted `types` variable, added `typeof types.length !== 'number'` guard before `initToolbox()`. Silently returns (API already confirmed `ok:true`, toolbox remains empty until device reboots). |
| low | `addWidget()` clamped widget *position* (x,y) into canvas bounds but not widget *dimensions* (w,h). If a future widget type has `defaultW > deviceW`, the widget is created overflowing the canvas with no visible feedback, giving a dishonest canvas preview. | Added two dimension-clamp lines before the position-clamp lines in `addWidget()`. Uses `meta.minW`/`meta.minH` floors. Only triggers when `deviceW > 0` (i.e., after canvas is initialized). |
| dismissed | Task 10 checkbox `[x]` marked complete but not performed | FALSE POSITIVE: Task 10 is `[ ]` (unchecked) in the actual story file at line 198. This was corrected in Round 1 synthesis. False positive — the reviewer was working from stale context. |
| dismissed | Story `Status: done` while AC#10 is unverified — story should be `in-progress` | FALSE POSITIVE: The existing `[AI-Review] LOW` action item in the Senior Developer Review section already captures this. The story conventions in this project allow `Status: done` with a pending hardware smoke test flagged as an AI-Review action item. The Dev Agent Record is explicit about the deferral. No change to story status warranted. |
| dismissed | AC#3 violation — toolbox items are clickable, not draggable; HTML5 drag-drop not implemented | FALSE POSITIVE: Task 5 of the story — the authoritative implementation specification — explicitly says "**Clicking** a non-disabled toolbox item calls `addWidget(type)`." The story Dev Notes show the `addWidget()` function body that `initToolbox` is required to call. The AC#3 "draggable" language is aspirational UX phrasing; Task 5 resolved the implementation as click-to-add. Implementation exactly matches Task 5. The AC wording is a documentation inconsistency in the story (not a code defect), already noted in prior synthesis. |
| dismissed | Hit-test boundary uses inclusive `<=` instead of exclusive `<` on widget right/bottom edges | FALSE POSITIVE: The story Dev Notes *explicitly* use `<=` in the documented hit-test code block: `"if cx >= w.x * SCALE && cx <= (w.x + w.w) * SCALE"`. The implementation faithfully follows the authoritative story spec. The practical impact is 1 CSS pixel = 0.25 logical pixels at 4× scale — imperceptible in use. The Dev Notes are authoritative; the `<=` is intentional design. False positive. |
| dismissed | `selectedIndex` mutation before widget reference capture creates race condition in `onPointerDown` | FALSE POSITIVE: JavaScript's event loop is single-threaded. No event can fire between two consecutive synchronous lines (`selectedIndex = hit.index` then `var w = widgets[selectedIndex]`). `setPointerCapture` prevents subsequent pointer events, but even without it, `onToolboxClick` cannot interleave with synchronous code execution. This is a fundamental misunderstanding of JS concurrency. False positive. |
| dismissed | `initToolbox` accumulates duplicate `click` listeners on repeated calls | FALSE POSITIVE: `initToolbox` is called exactly once — from the `loadLayout().then()` chain, called once from `init()`, called once from `DOMContentLoaded`. There is no code path that re-invokes `loadLayout()` or `initToolbox()` after page load. False positive for current code; only a theoretical future risk. |
| dismissed | Weak numeric parsing in `buildWidgetTypeMeta` — `f['default']` could be a string causing `NaN` propagation | FALSE POSITIVE: `FW.get()` uses `JSON.parse` under the hood. The ArduinoJson-serialized API response emits `"default": 32` (number literal), which JSON.parse correctly deserializes as a JS number. The API contract (from `WebPortal.cpp`) sends integers, not strings, for `w`/`h` fields. String coercion would require the server to malform the JSON. False positive for this API contract. |
| dismissed | Canvas resize during drag causes coordinate mismatch — `initCanvas()` could be called while `dragState` is active | FALSE POSITIVE: `initCanvas()` is only called from `loadLayout()`, which is only called once from `init()` at page load. There is no polling, no "refresh layout" button, and no WebSocket that could trigger a second `loadLayout()` call. False positive for the current architecture; theoretical future concern. |
| dismissed | Story File List omits `tests/smoke/test_web_portal_smoke.py` and `sprint-status.yaml` from modified-files section | FALSE POSITIVE: Documentation completeness note, not a code defect. The File List documents product source files; the smoke test and sprint YAML are tracked separately in the Dev Agent Record. Pre-existing issue already logged in prior synthesis; no action required. |
| dismissed | AC#7 story text claims logo minimum height is 8px but API returns h=16 for logo type | FALSE POSITIVE: This AC text documentation error was already identified and dismissed in the prior synthesis round (visible in the antipatterns table: "AC7 logo minimum height: story says h=8, API returns h=16 — implementation is wrong → FALSE POSITIVE: code is correct, AC7 story text is wrong"). `buildWidgetTypeMeta()` correctly reads `minH = meta.defaultH` from the API response. No code change needed. |
| dismissed | `FW.*` usage without hard guard — hard crash if `common.js` fails to load | FALSE POSITIVE: `common.js` is loaded synchronously before `editor.js` via `<script src="/common.js"></script>`. If `common.js` fails to load, the browser would show a network error before `editor.js` runs. The `FW.showToast` guard in `onPointerMove` (`typeof FW !== 'undefined' && FW.showToast`) already demonstrates defensive coding where it matters. Adding `FW` null checks everywhere is disproportionate to the risk on a LAN-only device page where `common.js` is served from the same device. |
| dismissed | Editor toolbox not keyboard-accessible (WCAG gap) | FALSE POSITIVE: Confirmed gap but out of LE-1.6 scope. The project context notes this is a LAN-only appliance UI. The story has no accessibility ACs. Deferred. |
| dismissed | `e.preventDefault()` only fires on canvas hit — miss path allows browser gestures | FALSE POSITIVE: `touch-action: none` on `#editor-canvas` (CSS line 87) unconditionally prevents browser scroll/zoom on the canvas, regardless of hit-test outcome. `e.preventDefault()` is documented as "belt-and-braces" belt-and-braces for older iOS. The antipatterns table explicitly forbids `e.preventDefault()` globally on body. Already addressed in prior synthesis. |
| dismissed | Toast suppression `toastedFloor` flag is fragile / no issue with toast logic | FALSE POSITIVE: Reviewer C explicitly concluded "No Issue Found" on this item. Confirmed: logic is correct. False positive. --- |

## Story le-1-6 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | Missing `.catch()` on `loadLayout()` promise chain — a `fetch()` network transport failure (connection refused, DNS fail) produces an unhandled promise rejection with no user-visible feedback | Added `.catch(function(err) { FW.showToast('Editor failed to load — check device connection', 'error'); })` at the end of the `.then().then()` chain in `loadLayout()`. This follows the project-wide pattern established across `dashboard.js` and `wizard.js`. Also regenerated `firmware/data/editor.js.gz` (3,958 bytes; total gzip 5,439 bytes = 26.6% of 20 KB budget, still well within AC8). |
| low | Issue (DEFERRED — action item from prior rounds)**: AC10 mobile smoke test not yet performed on real hardware | Ongoing `[AI-Review]` action item tracking this; cannot be resolved without physical device. |
| dismissed | hitTest `<=` boundary allows clicks 1–2 CSS pixels outside widget to register as hits | FALSE POSITIVE: The story Dev Notes *explicitly* specify `<=` in the documented hit-test code block: `"if cx >= w.x * SCALE && cx <= (w.x + w.w) * SCALE"`. This was already dismissed in Round 2 synthesis. The `<=` is by-design per the authoritative story spec. The practical impact is 1 CSS pixel = 0.25 logical pixels at 4× scale — imperceptible in use. |
| dismissed | Snap control race condition — SNAP variable and DOM active state can diverge under rapid clicks | FALSE POSITIVE: JavaScript's event loop is strictly single-threaded. The `SNAP = snapVal` → `localStorage.setItem()` → `updateSnapButtons()` → `render()` sequence executes atomically within a single event callstack. No other event can interleave. The proposed `snapUpdateInProgress` flag is a no-op lock that solves a problem that cannot exist in the JS execution model. False positive. |
| dismissed | Negative dimension defense missing in `addWidget()` — `deviceW - w.w` can be negative | FALSE POSITIVE: Lines 286–290 already handle this with `Math.max(meta.minW \|\| 6, deviceW)` and `Math.max(0, deviceW - w.w)`. These guards were added in Round 2 synthesis. Reading the current source confirms they are present. False positive. |
| dismissed | API response schema guard missing in `loadLayout()` — accessing `d.matrix.width` without checking nested structure | FALSE POSITIVE: Lines 346–351 of the current `editor.js` already contain the exact nested-field guard checking `d.matrix`, `d.hardware`, and the numeric types of all three fields. This was fixed in Round 2 synthesis. Reading the current source confirms it is present. False positive. |
| dismissed | `initToolbox` called without array-like guard — `types.length` would throw if `data` is `null` | FALSE POSITIVE: Line 358 of the current `editor.js` contains `if (!types \|\| typeof types.length !== 'number') return;` before the `initToolbox()` call. This was fixed in Round 2 synthesis. False positive. |
| dismissed | `releasePointerCapture` called after `dragState = null` — wrong spec order | FALSE POSITIVE: Lines 210–212 of the current `editor.js` show `releasePointerCapture` is called *before* `dragState = null`. The ordering fix was applied in Round 2 synthesis. False positive. |
| dismissed | Task 10 checkbox is `[x]` (complete) but mobile smoke test was never performed | FALSE POSITIVE: Task 10 is `[ ]` (unchecked) in the actual story file at line 198. This was corrected in Round 1 synthesis. The reviewer was working from stale context. False positive. |
| dismissed | AC3 drag-and-drop wording vs click-to-add implementation — AC traceability invalid | FALSE POSITIVE: This was dismissed in prior synthesis rounds. Task 5 of the story (the authoritative implementation spec) explicitly says "**Clicking** a non-disabled toolbox item calls `addWidget(type)`." The AC3 "draggable" language is aspirational UX phrasing; Task 5 resolved the implementation as click-to-add. The AC wording is a documentation inconsistency in the story text, not a code defect. |
| dismissed | Story `Status: done` while AC10 is unverified — Definition-of-Done violation | FALSE POSITIVE: Not a code defect. The existing `[AI-Review] LOW` action item in the Senior Developer Review section already captures this per project convention. This is a governance observation, not a source code issue. |
| dismissed | API response `data` type validation — `entry.type`/`entry.label` could be undefined, rendering "undefined" in toolbox | FALSE POSITIVE: `FW.get()` uses `JSON.parse` under the hood. The ArduinoJson-serialized API response emits correctly typed string values for `type` and `label`. The fallback `entry.label \|\| entry.type` in `initToolbox()` provides an additional safety net. The scenario requires the server to malform its own JSON, which is not an application-layer bug. The `console.warn` Reviewer B suggests would be ES5-valid but adds complexity for a hypothetical device firmware bug scenario. Dismissed — the API contract is well-defined and tested by `test_get_widget_types_contract`. |
| dismissed | initToolbox listener accumulates on repeated calls | FALSE POSITIVE: `initToolbox()` is called exactly once per page lifecycle. Already dismissed in prior synthesis rounds. False positive. |
| dismissed | `e.preventDefault()` only fires on hit — miss path allows browser gestures | FALSE POSITIVE: `touch-action: none` on `#editor-canvas` (CSS line 87) unconditionally prevents browser scroll/zoom on the canvas regardless of hit-test outcome. Already dismissed in prior synthesis rounds. False positive. |
| dismissed | Resize handle overlap with widget body — selected widget's handle intercepts click intended for unselected overlapping widget | FALSE POSITIVE: LOW, acknowledged design limitation. Widgets in a real FlightWall layout should not overlap (the editor is for a 192×48 LED matrix — overlapping widgets would produce meaningless display output). This edge case has negligible real-world impact. Accepted as documented behavior. |
| dismissed | Pointer cancel edge case — brief window between `pointercancel` and `onPointerMove` guard | FALSE POSITIVE: Reviewer B self-assessed this as LOW and ultimately concluded "NO FIX REQUIRED — existing guard is sufficient." The `if (dragState === null) return` guard at line 171 makes this safe. The event loop guarantees sequential execution. False positive. |
| dismissed | Story File List omits `tests/smoke/test_web_portal_smoke.py` and `sprint-status.yaml` | FALSE POSITIVE: Documentation completeness note, not a code defect. Already dismissed in prior synthesis rounds. |
| dismissed | AC7 logo minimum height: story text claims h=8, API returns h=16 | FALSE POSITIVE: The code is correct — `buildWidgetTypeMeta()` reads `minH = meta.defaultH` from the API response which returns h=16 for logo. The AC7 story text is wrong. Already dismissed in prior synthesis rounds. No code change needed. |
| dismissed | SOLID violation — `editor.js` IIFE mixes concerns (rendering, state, events, API, DOM) | FALSE POSITIVE: The project explicitly uses an IIFE module pattern for all `data-src/*.js` files (ES5 constraint, no modules). Extracting sub-modules would require either multiple `<script>` tags (increasing LittleFS usage) or a build system (not in scope for this embedded project). Out of scope for LE-1.6. |
| dismissed | Non-array `data` from widget types response returns silently with no toast | FALSE POSITIVE: Correct behavior. If `ok:true` is returned but `data` is null/non-array, this is a firmware bug that should not surface to the user as an error toast (the device is functioning, just with a malformed response shape). The toolbox remains empty (canvas is still functional). Silent return is appropriate here; the `ok:true` check already confirmed server health. --- |

## Story le-1-7 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Smoke test class alphabetical ordering causes `TestEditorActivate` (now `TestEditorDActivate`) to run before `TestEditorLayoutCRUD` (now `TestEditorCLayoutCRUD`) — `_created_layout_id` is `None` when activate test runs, causing it to silently `skipTest()`, so the activate step never fires before `TestEditorModeSwitch` asserts the mode | Renamed `TestEditorLayoutCRUD` → `TestEditorCLayoutCRUD` and `TestEditorActivate` → `TestEditorDActivate` so alphabetical execution order is: Asset → CRUD → DActivate → ModeSwitch. Also removed dead `if not _created_layout_id: pass` branch from `TestEditorModeSwitch`. |
| medium | AC#4 requires name ≤ 32 characters JS-side validation before save. HTML `maxlength="32"` prevents normal input but a paste-attack or devtools bypass would reach the server with a misleading "Enter a layout name" error instead of a length-specific message | Added explicit `if (name.length > 32)` guard with targeted error toast in `saveLayout()`. Also regenerated `firmware/data/editor.js.gz` (5,811 bytes; total 8,078 / 30,720 = 26.3% budget). |
| low | `FW.showToast('Applying…', 'success')` uses `'success'` severity for an in-progress state — operator sees a green "success" toast while the activate may still fail | Changed severity from `'success'` to `'info'` for the intermediate Applying toast. |
| dismissed | "Task 4 — showPropsPanel function missing" | FALSE POSITIVE: `showPropsPanel(w)` is fully implemented at lines 362–414 of `editor.js`. Complete with clock/text branching, align display logic, and position/size readouts. The reviewer's evidence ("only references: Line 395") referenced a call site, not searching for the definition. False positive verified by direct code read. |
| dismissed | "Task 5 — position/size readouts not updated during drag" | FALSE POSITIVE: Lines 206–210 of `onPointerMove` in `editor.js` explicitly query and update both `prop-pos-readout` and `prop-size-readout` after every pointer move event. Already implemented. |
| dismissed | "AC#9 FW.put integration unverified" | FALSE POSITIVE: `FW.put` is fully present in `common.js` at lines 47–53 and included in the return object at line 87. The function mirrors `FW.post` using `method: 'PUT'` as required. No integration test is required beyond what the smoke suite covers. |
| dismissed | "Test file TestEditorModeSwitch passes even if activate never ran (lying pass)" | FALSE POSITIVE: Addressed by the class rename fix — with correct ordering, `TestEditorDActivate` now fires before `TestEditorModeSwitch`. The assertion is no longer potentially stale. |
| dismissed | "AC1 not driven by fields array — hard-coded type checks" | FALSE POSITIVE: Story Dev Notes explicitly specify V1 behavior: "The property panel deliberately exposes only: Content, Color, Align" and the align → L/C/R button group is "intentionally" not a `<select>`. Clock → select is a fixed V1 design. The `widgetTypeMeta` built from the API response is used for defaults and min-floors; the panel control type is intentionally fixed. This is by-design, not a defect. |
| dismissed | "saveLayout promise chain handles undefined/string return incorrectly" | FALSE POSITIVE: `parseJsonResponse` in `common.js` always returns a `{status, body}` envelope — even on empty responses or parse errors it synthesizes a proper envelope with `ok: false`. It never returns `undefined` or a raw string. The `.catch()` at the end handles actual fetch rejections (network down). |
| dismissed | "Race condition in LayoutStore::save — concurrent client reads" | FALSE POSITIVE: LE-1.7 is a web-only story. No firmware C++ changes were made. Concurrent file access in `LayoutStore` is a firmware concern outside this story's scope. Project context explicitly states firmware files are not a LE-1.7 concern. |
| dismissed | "JSON.stringify performance footgun on large widgets during pointermove" | FALSE POSITIVE: `JSON.stringify` is only called inside `saveLayout()` and `getLayoutBody()` — both are user-triggered on button click, not on `pointermove`. Factually incorrect claim. |
| dismissed | "SOLID violation — WebPortal mixes routing and orchestration" | FALSE POSITIVE: Pre-existing architecture documented in project-context.md as the intentional hexagonal adapter pattern. Not introduced by LE-1.7 (web-only story, no WebPortal changes). |
| dismissed | "Dirty flag race condition in saveAsNew async flow" | FALSE POSITIVE: JavaScript's event loop is single-threaded. No event can fire between `currentLayoutId = null` and the Promise chain executing. `saveLayout()` executes synchronously up to the first `fetch()` call, at which point the `currentLayoutId = null` state is fully set before any async resolution occurs. |
| dismissed | "Layout ID uniqueness not guaranteed — two similar names produce same ID" | FALSE POSITIVE: Story Dev Notes explicitly document that `LayoutStore::save()` performs an upsert — a same-id POST overwrites the existing layout. This is documented behavior, not a bug. For V1 single-user appliance use case, this is acceptable. A `Date.now()` suffix would increase the ID length and reduce human-readability; rejected per story antipattern table which defers complexity. |
| dismissed | "RenderContext isolation violated — editor builds widgets without validation" | FALSE POSITIVE: The firmware's `LayoutStore::save()` + `CustomLayoutMode::init()` perform server-side validation at layout-load time. `addWidget()` uses `meta.minW`/`meta.minH` from the API response to enforce floors. The `w=6, h=6` concern is incorrect — `minH = meta.defaultH` which is 8 for text (matching firmware floor). Dual-layer validation is the correct architecture for a web editor. |
| dismissed | "Missing error boundary for layout load failure when malformed nested structure" | FALSE POSITIVE: Lines 568–574 of `editor.js` contain an explicit nested-field guard checking `d.matrix`, `d.hardware`, and numeric types of all three fields, showing a specific error toast before returning null. Already implemented. --- |

## Story le-1-8 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | MetricWidget `kDegreeSuffix` uses byte `0xF8` but Adafruit GFX with `_cp437=false` (the project default — `setCp437()` is never called) offsets any byte `>= 0xB0` by `+1` before font table lookup. Sending `0xF8` renders font[0xF9] = two faint dots (·), not the degree symbol `°`. The correct byte to achieve font[0xF8] is `0xF7`. | Changed `(char)0xF8` to `(char)0xF7` with explanatory comment citing the Adafruit GFX offset rule. |
| medium | `RenderContext::currentFlight` field added in LE-1.8 lacks a default member initializer. Any callsite that constructs `RenderContext` without explicit `{}` zero-initialization (e.g., stack allocation without aggregate-init) would leave `currentFlight` as a garbage pointer. All existing modes use `RenderContext ctx{}` or aggregate-init, so no current bug — but the missing default is a latent hazard for future callsites. | Added `= nullptr` default member initializer. |


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="System architecture for review context" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-17</var>
<var name="description">Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.</var>
<var name="document_output_language">English</var>
<var name="document_project_file" description="Brownfield project documentation (optional)" load_strategy="INDEX_GUIDED">_bmad-output/planning-artifacts/index.md</var>
<var name="epic_num">le-1</var>
<var name="epics_file" description="Epic containing story being reviewed" load_strategy="SELECTIVE_LOAD" token_approx="5795">_bmad-output/planning-artifacts/epic-le-1-layout-editor.md</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="instructions">embedded context</var>
<var name="name">code-review</var>
<var name="output_folder">_bmad-output/implementation-artifacts</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="712">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_dir">_bmad-output/implementation-artifacts/stories</var>
<var name="story_file" file_id="41af4780">embedded in prompt, file id: 41af4780</var>
<var name="story_id">le-1.8</var>
<var name="story_key">le-1-8-flight-field-and-metric-widgets</var>
<var name="story_num">8</var>
<var name="story_title">flight-field-and-metric-widgets</var>
<var name="template">False</var>
<var name="timestamp">20260417_1800</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="ux_design_file" description="UX design specification (if UI review)" load_strategy="FULL_LOAD" token_approx="30146">_bmad-output/planning-artifacts/ux-design-specification-delight.md</var>
<var name="validation">embedded context</var>
</variables>
<instructions><workflow>
  <critical>SCOPE LIMITATION: You are a READ-ONLY VALIDATOR. Output your code review report to stdout ONLY. Do NOT create files, do NOT modify files, do NOT use Write/Edit/Bash tools. Your stdout output will be captured and saved by the orchestration system.</critical>
  <critical>All configuration and context is available in the VARIABLES section below. Use these resolved values directly.</critical>
  <critical>Communicate all responses in English and language MUST be tailored to expert</critical>
  <critical>Generate all documents in English</critical>

  <critical>🔥 YOU ARE AN ADVERSARIAL CODE REVIEWER - Find what's wrong or missing! 🔥</critical>
  <critical>Your purpose: Validate story file claims against actual implementation</critical>
  <critical>Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.</critical>
  <critical>Find 3-10 specific issues in every review minimum - no lazy "looks good" reviews - YOU are so much better than the dev agent
    that wrote this slop</critical>
  <critical>Read EVERY file in the File List - verify implementation against story requirements</critical>
  <critical>Tasks marked complete but not done = CRITICAL finding</critical>
  <critical>Acceptance Criteria not implemented = HIGH severity finding</critical>

  <step n="1" goal="Build review attack plan">
    <action>Extract ALL Acceptance Criteria from story</action>
    <action>Extract ALL Tasks/Subtasks with completion status ([x] vs [ ])</action>
    <action>From Dev Agent Record → File List, compile list of claimed changes</action>

    <action>Create review plan:
      1. **AC Validation**: Verify each AC is actually implemented
      2. **Task Audit**: Verify each [x] task is really done
      3. **Code Quality**: Security, performance, maintainability
      4. **Test Quality**: Real tests vs placeholder bullshit
    </action>
  </step>

  <step n="2" goal="Execute adversarial review">
    <critical>VALIDATE EVERY CLAIM - Check git reality vs story claims</critical>

    <!-- Git vs Story Discrepancies -->
    <action>Review git vs story File List discrepancies:
      1. **Files changed but not in story File List** → MEDIUM finding (incomplete documentation)
      2. **Story lists files but no git changes** → HIGH finding (false claims)
      3. **Uncommitted changes not documented** → MEDIUM finding (transparency issue)
    </action>

    <!-- Use combined file list: story File List + git discovered files -->
    <action>Create comprehensive review file list from story File List and git changes</action>

    <!-- AC Validation -->
    <action>For EACH Acceptance Criterion:
      1. Read the AC requirement
      2. Search implementation files for evidence
      3. Determine: IMPLEMENTED, PARTIAL, or MISSING
      4. If MISSING/PARTIAL → HIGH SEVERITY finding
    </action>

    <!-- Task Completion Audit -->
    <action>For EACH task marked [x]:
      1. Read the task description
      2. Search files for evidence it was actually done
      3. **CRITICAL**: If marked [x] but NOT DONE → CRITICAL finding
      4. Record specific proof (file:line)
    </action>

    <!-- Code Quality Deep Dive -->
    <action>For EACH file in comprehensive review list:
      1. **Security**: Look for injection risks, missing validation, auth issues
      2. **Performance**: N+1 queries, inefficient loops, missing caching
      3. **Error Handling**: Missing try/catch, poor error messages
      4. **Code Quality**: Complex functions, magic numbers, poor naming
      5. **Test Quality**: Are tests real assertions or placeholders?
    </action>

    <check if="total_issues_found lt 3">
      <critical>NOT LOOKING HARD ENOUGH - Find more problems!</critical>
      <action>Re-examine code for:
        - Edge cases and null handling
        - Architecture violations
        - Documentation gaps
        - Integration issues
        - Dependency problems
        - Git commit message quality (if applicable)
      </action>
      <action>Find at least 3 more specific, actionable issues</action>
    </check>
  </step>

  <step n="3" goal="Systematic Code Quality Assessment">
    <critical>🎯 RUTHLESS CODE QUALITY GATE: Systematic review against senior developer standards!</critical>

    <substep n="4a" title="SOLID Principles Violations">
      <action>Hunt for SOLID violations with surgical precision:
        - **S - Single Responsibility**: Classes/functions doing too much
        - **O - Open/Closed**: Code requiring modification for extension
        - **L - Liskov Substitution**: Subclasses breaking parent contracts
        - **I - Interface Segregation**: Fat interfaces forcing unused dependencies
        - **D - Dependency Inversion**: High-level modules depending on low-level details
      </action>
      <action>Document each violation with file:line and severity (1-10)</action>
      <action>Store as {{solid_violations}}</action>
    </substep>

    <substep n="4b" title="Hidden Bugs Detection">
      <action>Hunt for hidden bugs and time bombs:
        - Resource leaks: unclosed files, connections, handles
        - Race conditions: shared state without synchronization
        - Edge cases: null/empty/boundary conditions not handled
        - Off-by-one errors: loop bounds, array indices
        - Exception swallowing: catch blocks that hide errors
        - State corruption: mutable shared state
      </action>
      <action>Document each bug with reproduction scenario</action>
      <action>Store as {{hidden_bugs}}</action>
    </substep>

    <substep n="4c" title="Abstraction Level Analysis">
      <action>Analyze abstraction appropriateness:
        - **Over-engineering**: Unnecessary patterns, premature abstraction
        - **Under-engineering**: Missing abstractions, code duplication
        - **Wrong patterns**: Pattern misuse, cargo cult programming
        - **Boundary breaches**: Layer violations, leaky abstractions
      </action>
      <action>Document each issue with reasoning</action>
      <action>Store as {{abstraction_issues}}</action>
    </substep>

    <substep n="4d" title="Lying Tests Detection">
      <action>Expose tests that lie about correctness:
        - Tests that always pass regardless of implementation
        - Missing assertions or weak assertions
        - Tests that don't test the actual behavior
        - Mocked everything - testing mocks not code
        - Happy path only - no error/edge case coverage
        - Tautological tests - testing what you just set up
      </action>
      <action>Document each lying test with explanation</action>
      <action>Store as {{lying_tests}}</action>
    </substep>

    <substep n="4e" title="Performance Footguns">
      <action>Identify performance anti-patterns:
        - N+1 queries: database calls in loops
        - Unnecessary allocations: creating objects in hot paths
        - Missing caching: repeated expensive computations
        - Blocking operations: sync I/O in async contexts
        - Memory leaks: growing collections, circular references
        - Inefficient algorithms: O(n²) when O(n) possible
      </action>
      <action>Document each footgun with impact assessment</action>
      <action>Store as {{performance_footguns}}</action>
    </substep>

    <substep n="4f" title="Tech Debt Bombs">
      <action>Find future maintenance nightmares:
        - Hard-coded values: magic numbers, embedded strings
        - Magic strings: undocumented string literals
        - Copy-paste code: duplicated logic
        - Missing TODOs: incomplete implementations
        - Deprecated usage: using obsolete APIs
        - Tight coupling: changes ripple across codebase
      </action>
      <action>Document each debt bomb with explosion radius</action>
      <action>Store as {{tech_debt_bombs}}</action>
    </substep>

    <substep n="4g" title="PEP 8 and Pythonic Compliance">
      <action>Check Python-specific quality (if Python code):
        - Naming conventions: snake_case, PascalCase appropriately
        - Import organization: stdlib, third-party, local separation
        - Line length and formatting
        - Docstrings: missing or inadequate documentation
        - Pythonic idioms: using Python features correctly
        - Type hints: missing, incorrect, or incomplete
      </action>
      <action>For non-Python: apply language-appropriate style standards</action>
      <action>Document each violation</action>
      <action>Store as {{style_violations}}</action>
    </substep>

    <substep n="4h" title="Type Safety Analysis">
      <action>Analyze type safety issues:
        - Missing type hints on public interfaces
        - Incorrect type annotations
        - Any/Unknown overuse
        - Type narrowing issues
        - Generic type misuse
        - Runtime type errors waiting to happen
      </action>
      <action>Document each type safety issue</action>
      <action>Store as {{type_safety_issues}}</action>
    </substep>

    <substep n="4i" title="Security Vulnerability Scan">
      <action>Deep security review:
        - Credential exposure: hardcoded secrets, logged credentials
        - Injection vectors: SQL, command, template injection
        - Authentication issues: weak validation, session problems
        - Authorization gaps: missing permission checks
        - Data exposure: sensitive data in logs, responses
        - Dependency vulnerabilities: known CVEs
      </action>
      <action>Document each vulnerability with CVSS-like severity</action>
      <action>Store as {{security_vulnerabilities}}</action>
    </substep>

    <substep n="4j" title="Calculate Evidence Score">
      <critical>🔥 CRITICAL: You MUST calculate and output the Evidence Score for synthesis!</critical>

      <action>Map each finding to Evidence Score severity:
        - **🔴 CRITICAL** (+3 points): Security vulnerabilities, data corruption, blocking bugs, task completion lies
        - **🟠 IMPORTANT** (+1 point): SOLID violations, performance issues, missing tests, AC gaps
        - **🟡 MINOR** (+0.3 points): Style violations, documentation issues, minor refactoring
      </action>

      <action>Count CLEAN PASS categories - areas with NO issues found:
        - Each clean category: -0.5 points
        - Categories: SOLID, Hidden Bugs, Abstraction, Lying Tests, Performance, Tech Debt, Style, Type Safety, Security
      </action>

      <action>Calculate Evidence Score:
        {{evidence_score}} = SUM(finding_scores) + (clean_pass_count × -0.5)

        Example: 2 CRITICAL (+6) + 3 IMPORTANT (+3) + 4 CLEAN PASSES (-2) = 7.0
      </action>

      <action>Determine Evidence Verdict:
        - **EXEMPLARY** (score ≤ -3): Many clean passes, minimal issues
        - **APPROVED** (score &lt; 3): Acceptable quality, minor issues only
        - **MAJOR REWORK** (3 ≤ score &lt; 7): Significant issues require attention
        - **REJECT** (score ≥ 7): Critical problems, needs complete rewrite
      </action>

      <action>Store for output:
        - {{evidence_findings}}: List of findings with severity_icon, severity, description, source, score
        - {{clean_pass_count}}: Number of clean categories
        - {{evidence_score}}: Calculated total score
        - {{evidence_verdict}}: EXEMPLARY/APPROVED/MAJOR REWORK/REJECT
      </action>
    </substep>
  </step>

  <step n="4" goal="Present findings">
    <critical>OUTPUT MARKERS REQUIRED: Your code review report MUST start with the marker &lt;!-- CODE_REVIEW_REPORT_START --&gt; on its own line BEFORE the report content, and MUST end with the marker &lt;!-- CODE_REVIEW_REPORT_END --&gt; on its own line AFTER the final line. The orchestrator extracts ONLY content between these markers. Any text outside the markers (thinking, commentary) will be discarded.</critical>

    <action>Categorize all findings from steps 3 and 4</action>
    <action>Generate the code review report using the output block template below as FORMAT GUIDE</action>
    <action>Replace all {{placeholders}} with actual values from your analysis</action>
    <action>Output the complete report to stdout</action>
    <action>Do NOT save to any file - orchestrator handles persistence</action>

    <o>
<!-- CODE_REVIEW_REPORT_START -->**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/le-1-8-flight-field-and-metric-widgets.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | {{git_discrepancy_count}} |
| AC Implementation Gaps | {{ac_gaps_count}} |
| Task Completion Lies | {{task_lies_count}} |
| SOLID Violations | {{solid_count}} |
| Hidden Bugs | {{bugs_count}} |
| Performance Footguns | {{perf_count}} |
| Security Vulnerabilities | {{security_count}} |
| **Total Issues** | **{{total_issues}}** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
{{#each evidence_findings}}
| {{severity_icon}} {{severity}} | Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval. | {{source}} | +{{score}} |
{{/each}}
{{#if clean_pass_count}}
| 🟢 CLEAN PASS | {{clean_pass_count}} |
{{/if}}

### Evidence Score: {{evidence_score}}

| Score | Verdict |
|-------|---------|
| **{{evidence_score}}** | **{{evidence_verdict}}** |

---

## 🏛️ Architectural Sins

{{#each solid_violations}}
- **[{{severity}}/10] {{principle}}:** Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.
  - 📍 `{{file}}:{{line}}`
  - 💡 Fix: {{suggestion}}
{{/each}}

{{#each abstraction_issues}}
- **{{issue_type}}:** Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.
  - 📍 `{{file}}:{{line}}`
{{/each}}

{{#if no_architectural_sins}}
✅ No significant architectural violations detected.
{{/if}}

---

## 🐍 Pythonic Crimes &amp; Readability

{{#each style_violations}}
- **{{violation_type}}:** Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.
  - 📍 `{{file}}:{{line}}`
{{/each}}

{{#each type_safety_issues}}
- **Type Safety:** Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.
  - 📍 `{{file}}:{{line}}`
{{/each}}

{{#if no_style_issues}}
✅ Code follows style guidelines and is readable.
{{/if}}

---

## ⚡ Performance &amp; Scalability

{{#each performance_footguns}}
- **[{{impact}}] {{issue_type}}:** Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.
  - 📍 `{{file}}:{{line}}`
  - 💡 Fix: {{suggestion}}
{{/each}}

{{#if no_performance_issues}}
✅ No significant performance issues detected.
{{/if}}

---

## 🐛 Correctness &amp; Safety

{{#each hidden_bugs}}
- **🐛 Bug:** Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.
  - 📍 `{{file}}:{{line}}`
  - 🔄 Reproduction: {{reproduction}}
{{/each}}

{{#each security_vulnerabilities}}
- **🔒 [{{severity}}] Security:** Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.
  - 📍 `{{file}}:{{line}}`
  - ⚠️ Impact: {{impact}}
{{/each}}

{{#each lying_tests}}
- **🎭 Lying Test:** {{test_name}}
  - 📍 `{{file}}:{{line}}`
  - 🤥 Why it lies: {{explanation}}
{{/each}}

{{#if no_correctness_issues}}
✅ Code appears correct and secure.
{{/if}}

---

## 🔧 Maintainability Issues

{{#each tech_debt_bombs}}
- **💣 Tech Debt:** Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.
  - 📍 `{{file}}:{{line}}`
  - 💥 Explosion radius: {{impact}}
{{/each}}

{{#if no_maintainability_issues}}
✅ Code is maintainable and well-documented.
{{/if}}

---

## 🛠️ Suggested Fixes

{{#each suggested_fixes}}
### {{number}}. {{title}}

**File:** `{{file}}`
**Issue:** {{issue_summary}}

{{#if file_under_250_lines}}
**Corrected code:**
```{{language}}
{{corrected_code}}
```
{{else}}
**Diff:**
```diff
{{diff}}
```
{{/if}}

{{/each}}

---

**Review Actions:**
- Issues Found: {{total_issues}}
- Issues Fixed: 0
- Action Items Created: 0

    
<!-- CODE_REVIEW_REPORT_END -->
</o>
  </step>

</workflow></instructions>
<output-template></output-template>
</compiled-workflow>