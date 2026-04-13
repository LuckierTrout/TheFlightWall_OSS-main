<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 7 -->
<!-- Phase: validate-story -->
<!-- Timestamp: 20260413T011928Z -->
<compiled-workflow>
<mission><![CDATA[

Adversarial Story Validation

Target: Story fn-1.7 - settings-import-in-setup-wizard

Your mission is to FIND ISSUES in the story file:
- Identify missing requirements or acceptance criteria
- Find ambiguous or unclear specifications
- Detect gaps in technical context
- Suggest improvements for developer clarity

CRITICAL: You are a VALIDATOR, not a developer.
- Read-only: You cannot modify any files
- Adversarial: Assume the story has problems
- Thorough: Check all sections systematically

Focus on STORY QUALITY, not code implementation.

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


]]></file>
<file id="893ad01d" path="_bmad-output/planning-artifacts/architecture.md" label="ARCHITECTURE"><![CDATA[

# Architecture Decision Document — TheFlightWall OSS

## Project Context

**Functional Requirements:** 48 FRs across 9 groups: Device Setup & Onboarding (FR1-8), Configuration (FR9-14), Calibration (FR15-18), Flight Data (FR19-25), Logo Display (FR26-29), Responsive Layout (FR30-33), Logo Management (FR34-37), System Ops (FR38-48).

**Key Non-Functional Requirements:**
| NFR | Target | Driver |
|-----|--------|--------|
| Hot-reload latency | <1s | ConfigManager atomic flag, no polling |
| Page load | <3s | Gzipped assets, minimal JS, ~280KB RAM ceiling |
| WiFi recovery | <60s | Auto-reconnect + AP fallback |
| Flash budget | 4MB total: 2MB app + 2MB LittleFS | Monitor usage, gzip web assets |
| Concurrent ops | Web + flight + display | FreeRTOS task pinning: display Core 0, WiFi/web/API Core 1 |

**Technology Stack (Locked):**
- Language: C++11 (Arduino ESP32)
- Platform: ESP32 with PlatformIO
- Dependencies: FastLED ^3.6.0, Adafruit GFX ^1.11.9, FastLED NeoMatrix ^1.2, ArduinoJson ^7.4.2, **ESPAsyncWebServer (mathieucarbou fork) ^3.6.0**, **AsyncTCP (mathieucarbou fork)**, LittleFS (built-in), ESPmDNS (built-in)

## Core Architectural Decisions

### D1: ConfigManager — Singleton with Category Struct Getters
Central singleton initialized first. Config values grouped into structs (DisplayConfig, LocationConfig, HardwareConfig, TimingConfig, NetworkConfig) for clean API and efficient access.

**NVS Key Abbreviations (15-char limit):**
| Key | Type | Default | Category |
|-----|------|---------|----------|
| brightness | uint8 | 5 | display |
| text_color_r/g/b | uint8 | 255 | display |
| center_lat/lon | double | 37.7749/-122.4194 | location |
| radius_km | double | 10.0 | location |
| tiles_x, tiles_y, tile_pixels | uint8 | 10,2,16 | hardware |
| display_pin | uint8 | 25 | hardware |
| origin_corner, scan_dir, zigzag | uint8 | 0 | hardware |
| fetch_interval, display_cycle | uint16 | 30,3 | timing |
| wifi_ssid, wifi_password | string | "" | network |
| os_client_id, os_client_sec | string | "" | network |
| aeroapi_key | string | "" | network |

**Config flow:** Compile-time defaults → NVS read on boot → RAM cache (struct fields) → runtime changes via web UI → debounced NVS write (2s quiet period) → hot-reload via atomic flag (config) or FreeRTOS queue (flight data).

**Reboot-required keys:** wifi_ssid, wifi_password, opensky credentials, aeroapi_key, display_pin. **Hot-reload keys:** brightness, text color, fetch/display intervals, layout params.

### D2: Inter-Task Communication — Hybrid (Atomic Flags + FreeRTOS Queue)
**Config changes:** `std::atomic<bool> configChanged` signals display task to re-read from ConfigManager.
**Flight data:** `QueueHandle_t flightQueue` with `xQueueOverwrite()` — display task always gets latest data.

### D3: WiFi State Machine — WiFiManager
**States:** WIFI_AP_SETUP → WIFI_CONNECTING → WIFI_STA_CONNECTED ↔ WIFI_STA_RECONNECTING → WIFI_AP_FALLBACK

**Transitions:** No WiFi config = AP_SETUP. Connected successfully = STA_CONNECTED. WiFi lost = STA_RECONNECTING (configurable timeout) → AP_FALLBACK. Long press GPIO 0 during boot = force AP_SETUP.

### D4: Web API Endpoints (11 REST)
| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/` | wizard.html (AP) or dashboard.html (STA) |
| GET/POST | `/api/settings` | Get/apply config (reboot flag in response) |
| GET | `/api/status` | System health |
| GET | `/api/wifi/scan` | Async WiFi scan |
| POST | `/api/reboot` | Save + reboot |
| POST | `/api/reset` | Factory reset |
| GET/POST/DELETE | `/api/logos` | Logo management |
| GET | `/api/layout` | Zone layout (initial load) |

Response envelope: `{ "ok": bool, "data": {...} }` or `{ "ok": false, "error": "message", "code": "..." }`

### D5: Component Integration
**Init sequence:** ConfigManager → SystemStatus → LogoManager → LayoutEngine → WiFiManager → WebPortal → FlightDataFetcher → displayTask on Core 0.

**Dependency graph:** ConfigManager (all depend on). DisplayTask is read-only (atomic flag + queue peel). WebPortal is only write path.

### D6: SystemStatus Registry
Health tracking for subsystems (WIFI, OPENSKY, AEROAPI, CDN, NVS, LITTLEFS) with levels (OK/Warning/Error) and human-readable messages. API call counter with monthly NTP-based reset.

### D7: Shared Zone Calculation Algorithm
**Critical:** Implemented identically in C++ (LayoutEngine) and JavaScript (dashboard.js) to ensure canvas preview matches LEDs.

**Test vectors:**
| Config | Matrix | Mode | Logo Zone | Flight Card | Telemetry |
|--------|--------|------|-----------|-------------|-----------|
| 10x2 @ 16px | 160x32 | full | 0,0→32x32 | 32,0→128x16 | 32,16→128x16 |
| 5x2 @ 16px | 80x32 | full | 0,0→32x32 | 32,0→48x16 | 32,16→48x16 |

## Project Structure

**Complete directory tree (44 files, 13 existing → updated, 5 new):**

```
firmware/
├── platformio.ini (board_build.filesystem=littlefs, custom_partitions.csv)
├── custom_partitions.csv (nvs 20KB, otadata 8KB, app0 2MB, spiffs 2MB)
├── src/main.cpp (init sequence, loop, display task, FreeRTOS)
├── core/
│   ├── ConfigManager.h/.cpp
│   ├── FlightDataFetcher.h/.cpp (updated: ConfigManager)
│   ├── LayoutEngine.h/.cpp
│   ├── LogoManager.h/.cpp
│   └── SystemStatus.h/.cpp
├── adapters/
│   ├── NeoMatrixDisplay.h/.cpp
│   ├── WebPortal.h/.cpp (11 endpoints)
│   ├── WiFiManager.h/.cpp
│   ├── OpenSkyFetcher.h/.cpp (updated: ConfigManager)
│   ├── AeroAPIFetcher.h/.cpp (updated: ConfigManager)
│   └── FlightWallFetcher.h/.cpp
├── interfaces/ (unchanged: BaseDisplay, BaseFlightFetcher, BaseStateVectorFetcher)
├── models/ (unchanged: FlightInfo, StateVector, AirportInfo)
├── config/ (unchanged: compile-time defaults, migrated to ConfigManager)
├── utils/
│   └── Log.h (LOG_E/I/V compile-time macros, LOG_LEVEL build flag)
├── data/ (LittleFS: *.gz web assets + logos/)
└── test/ (test_config_manager, test_layout_engine)
```

## Foundation Release — OTA, Night Mode, Settings Export

**4 new NVS keys (schedule):**
| Key | Type | Default | Purpose |
|-----|------|---------|---------|
| timezone | string | "UTC0" | POSIX timezone (from browser-side IANA-to-POSIX mapping) |
| sched_enabled | uint8 | 0 | Schedule enable flag |
| sched_dim_start, sched_dim_end | uint16 | 1380, 420 | Minutes since midnight (23:00 - 07:00) |
| sched_dim_brt | uint8 | 10 | Brightness during dim window |

**F1: Dual-OTA Partition Table**
```
nvs (20KB) | otadata (8KB) | app0 (1.5MB) | app1 (1.5MB) | spiffs (960KB)
```
Flash budget: 4MB total. LittleFS reduces 56% (2MB→960KB). One-time USB reflash required.

**F2: OTA Handler (WebPortal + main.cpp)**
Upload via `POST /api/ota/upload` multipart. Stream-to-partition via `Update.write()` per chunk. No RAM buffering of binary. Validate magic byte 0xE9 on first chunk. Reboot after successful `Update.end(true)`.

**F3: OTA Self-Check — WiFi-OR-Timeout (60s)**
Mark firmware valid when WiFi connects OR after 60-second timeout (whichever first). No self-HTTP-request. Watchdog handles crash-on-boot. If timeout path taken (WiFi down), device remains reachable via AP fallback for re-flash.

**F4: IANA-to-POSIX Timezone Mapping — Browser-Side**
JS object in wizard.js/dashboard.js: `{ "America/Los_Angeles": "PST8PDT,M3.2.0,M11.1.0", ... }`. Auto-detect via `Intl.DateTimeFormat()`. Send POSIX string to ESP32 via POST /api/settings.

**F5: Night Mode Scheduler — Non-Blocking Main Loop**
Schedule times as `uint16` minutes since midnight (0-1439). Midnight-crossing logic: if (dimStart <= dimEnd) then (current >= start && current < end), else (current >= start || current < end). Brightness scheduler overrides ConfigManager brightness. NTP sync via `configTzTime()` after WiFi connect. LWIP auto-resync every 1 hour.

**F6: ConfigManager Expansion — 5 new keys + ScheduleConfig struct**
All hot-reload, no reboot required. Timezone change calls `configTzTime()` immediately.

**F7: API Endpoint Additions**
| Endpoint | Purpose |
|----------|---------|
| POST `/api/ota/upload` | Multipart firmware upload |
| GET `/api/settings/export` | Download JSON config file |
| Updated `/api/status` | +firmware_version, +rollback_detected |

**Settings export:** Flat JSON with `flightwall_settings_version: 1`, all NVS keys. Import is client-side only (wizard pre-fills form fields).

## Display System Release — Mode System, Classic Card, Live Flight Card

**4 new components: DisplayMode interface, ModeRegistry, ClassicCardMode, LiveFlightCardMode.**

### DS1: DisplayMode Interface — Abstract Class with RenderContext
```cpp
class DisplayMode {
    bool init(const RenderContext& ctx);
    void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights);
    void teardown();
    const char* getName() const;
    const ModeZoneDescriptor& getZoneDescriptor() const;  // static metadata
};

struct RenderContext {
    Adafruit_NeoMatrix* matrix;     // GFX primitives
    LayoutResult layout;             // zone bounds
    uint16_t textColor;              // pre-computed
    uint8_t brightness;              // read-only
    uint16_t* logoBuffer;            // shared 2KB
    uint16_t displayCycleMs;         // cycle timing
};
```

**Key rules:** Modes receive RenderContext const ref (cannot modify shared state). Modes own flight cycling state (_currentFlightIndex, _lastCycleMs). Empty flight vector is valid (modes decide idle state). Modes must NOT call FastLED.show() — frame commit is display task's responsibility.

### DS2: ModeRegistry — Static Table with Cooperative Switch Serialization
```cpp
struct ModeEntry {
    const char* id;           // e.g., "classic_card"
    const char* displayName;
    DisplayMode* (*factory)();  // factory function
    uint32_t (*memoryRequirement)();  // static function, no instance
    uint8_t priority;
};

class ModeRegistry {
    static void init(const ModeEntry* table, uint8_t count);
    static bool requestSwitch(const char* modeId);  // Core 1
    static void tick(const RenderContext& ctx, const std::vector<FlightInfo>& flights);  // Core 0
    static DisplayMode* getActiveMode();
    static SwitchState getSwitchState();
};
```

**Switch flow (in tick()):**
1. _switchState = SWITCHING
2. Teardown current mode
3. Heap check: `ESP.getFreeHeap()` vs `memoryRequirement()`
4. If sufficient: factory() → new mode → init(). If init() fails: restore previous.
5. If insufficient: re-init previous, set error
6. Debounced NVS write (2s quiet period)

**Switch restoration pattern:** Teardown-but-don't-delete previous mode shell, enabling safe re-init on failure without re-allocation.

### DS3: NeoMatrixDisplay Responsibility Split
**Retained:** Hardware init, brightness control, frame commit (show()), fallback card rendering, calibration/positioning modes, shared resources (logo buffer, matrix, layout).

**Extracted:** renderZoneFlight(), renderFlightZone(), renderTelemetryZone(), renderLogoZone() → ClassicCardMode.

**New methods:** `RenderContext buildRenderContext()` (assembles context from internals), `show()` (wraps FastLED.show()).

### DS4: Display Task Integration
**In setup():** ModeRegistry::init(MODE_TABLE, MODE_COUNT). Restore last mode from NVS, default to "classic_card".

**In displayTask():**
```cpp
if (g_configChanged.exchange(false)) {
    cachedCtx = g_display.buildRenderContext();
}
if (calibrationMode) { ... }
else if (positioningMode) { ... }
else if (statusMessage) { ... }
else {
    ModeRegistry::tick(cachedCtx, flights);  // mode rendering
}
g_display.show();  // frame commit after mode renders
```

### DS5: Mode Switch API
| Endpoint | Response |
|----------|----------|
| GET `/api/display/modes` | modes[], active, switch_state, upgrade_notification |
| POST `/api/display/mode` | { switching_to: "mode_id" } or error |

Response includes mode metadata (zones, description) for wireframe UI.

### DS6: NVS Persistence — Single Key
**Key:** `display_mode` (string, mode ID). **Default:** `"classic_card"`. **Debounce:** 2s after successful switch. Boot restoration reads from NVS, defaults to classic_card if absent.

### DS7: Mode Picker UI
Dashboard section with CSS Grid wireframe schematics. Each mode card is entire tap target. Tap triggers synchronous POST (returns after mode is rendering or fails). Transition feedback: card in "Switching..." state during POST, sibling cards disabled. All three dismiss actions (Try, Browse, X) clear notification via localStorage + POST /api/display/ack-notification.

**NVS key pattern:** `display_mode` (unique with all MVP + Foundation keys, within namespace "flightwall").

**Enforcement:** ClassicCardMode extraction — 3-phase (copy, validate parity, delete from source). Terminal fallback: if mode init fails twice, displayFallbackCard() renders single-flight legacy card.

## Delight Release — Clock Mode, Departures Board, Scheduling, OTA Pull

**New components: ModeOrchestrator (state machine), OTAUpdater (GitHub API client), ClockMode, DeparturesBoardMode.**

### DL1: Fade Transition — RGB888 Dual Buffers (Transient)
**Buffers:** ~30KB RGB888 (two CRGB arrays, 160x32 @ 3 bytes/pixel). Allocate at transition start, free immediately after fade completes. If malloc() fails: instant cut (graceful degradation, not error).

**Blend:** 15 frames @ 66ms/frame = ~1s. Per-pixel linear interpolation: `r = (out*15 + in*step)/15`.

### DL2: Mode Orchestration — State Machine
**States:** MANUAL (user selection), SCHEDULED (time-based rule active), IDLE_FALLBACK (zero flights).
**Priority:** SCHEDULED > IDLE_FALLBACK > MANUAL.

**ModeOrchestrator::tick() (Core 1, ~1/sec):**
1. Evaluate schedule rules (first match wins)
2. If rule matches: SCHEDULED state, switch to rule mode
3. If no rule && was SCHEDULED: MANUAL state, restore user selection
4. If zero flights && MANUAL state: IDLE_FALLBACK, switch to Clock Mode
5. If flights > 0 && IDLE_FALLBACK state: MANUAL, restore selection

**Flight count signaling:** `std::atomic<uint8_t> g_flightCount` updated by FlightDataFetcher after queue write.

### DL3: OTAUpdater — GitHub Releases API Client with SHA-256
**Check (synchronous, ~1-2s):** GET releases/latest, parse JSON, compare tag_name vs FW_VERSION.

**Download (FreeRTOS task, Core 1):**
1. ModeRegistry::prepareForOTA() (teardown mode, set _switchState = SWITCHING)
2. Download .sha256 (64-char hex string)
3. Update.begin(partitionSize)
4. Stream .bin: Update.write() + mbedtls_sha256_update() per chunk
5. mbedtls_sha256_finish(), compare vs downloaded hash
6. If match: Update.end(true) → ESP.restart(). If mismatch: Update.abort() → ERROR.

**Error paths:** All errors after Update.begin() must call Update.abort(). Task self-deletes via `_downloadTask = nullptr; vTaskDelete(NULL)`.

### DL4: Schedule Rules NVS Storage
**Max 8 rules, indexed keys:**
```
sched_r{N}_start (uint16)    minutes since midnight
sched_r{N}_end (uint16)      minutes since midnight
sched_r{N}_mode (string)     mode ID
sched_r{N}_ena (uint8)       enabled flag
sched_r_count (uint8)        active rule count
```
Rules always compacted (no index gaps on delete).

### DL5: Per-Mode Settings Schema
**Mode declares settings as static const:**
```cpp
struct ModeSettingDef {
    const char* key;      // ≤7 chars (NVS suffix)
    const char* label;    // UI display name
    const char* type;     // "uint8", "enum", etc.
    int32_t defaultValue, minValue, maxValue;
    const char* enumOptions;
};

struct ModeSettingsSchema {
    const char* modeAbbrev;   // ≤5 chars for NVS prefix
    const ModeSettingDef* settings;
    uint8_t settingCount;
};
```

**NVS key format:** `m_{abbrev}_{key}` (total ≤15 chars). ConfigManager helpers: `getModeSetting()` / `setModeSetting()`. API iterates settingsSchema dynamically (never hardcode field names).

### DL6: API Endpoint Additions
| Endpoint | Purpose |
|----------|---------|
| GET `/api/ota/check` | Check GitHub for update |
| POST `/api/ota/pull` | Start download (spawns FreeRTOS task) |
| GET `/api/ota/status` | Poll progress (state, %) |
| GET `/api/schedule` | Get rules |
| POST `/api/schedule` | Save rules |

Updated `/api/display/modes`: includes `settings` array per mode (schema + current values, null for modes without settings).

## Implementation Patterns & Consistency Rules

**MVP Naming Conventions:**
- Classes: PascalCase (ConfigManager, FlightDataFetcher)
- Methods: camelCase (applyJson, fetchFlights)
- Private members: _camelCase (_matrix, _currentFlightIndex)
- Struct fields: snake_case (brightness, text_color_r, center_lat)
- Constants: UPPER_SNAKE_CASE
- Namespaces: PascalCase
- Files: PascalCase.h/.cpp
- Headers: #pragma once

**NVS Key Convention:** snake_case, max 15 chars. Abbreviations documented in ConfigManager.h.

**Logging Pattern:** #include "utils/Log.h", use LOG_E/LOG_I/LOG_V macros (compile-time levels).

**Error Handling:** Boolean return + output parameter (no exceptions). SystemStatus reporting + log macros. JSON API responses: { "ok": bool, "data": {...} } or { "ok": false, "error": "message", "code": "..." }.

**Memory:** String for dynamic text, const char* for constants. Avoid new/delete. Stream LittleFS files.

**Web Assets:** Every fetch() must check json.ok and call showToast() on failure.

**Enforcement Rules (30 total):**

1-11 MVP: naming, NVS keys, includes, memory, logging, error handling, web patterns.

12-16 Foundation: OTA validation (header before write), streaming (no RAM buffer), getLocalTime non-blocking (timeout=0), schedule times as uint16 minutes, settings import client-side only.

17-23 Display System: modes in firmware/modes/, RenderContext isolation (no ConfigManager/WiFiManager from modes), modes never call FastLED.show(), heap allocation only in init/teardown, shared utils from DisplayUtils.h, adding mode = 2 touch points (file + MODE_TABLE line), MEMORY_REQUIREMENT static constexpr.

24-30 Delight: ModeRegistry::requestSwitch() called from exactly 2 methods (ModeOrchestrator::tick/onManualSwitch), OTA SHA-256 incremental + Update.abort() on errors, prepareForOTA() sets _switchState=SWITCHING, fade buffers malloc/free in single call + alloc failure uses instant cut, per-mode settings via ConfigManager helpers + dynamic schema iteration, settings schemas in mode .h files not centralized, cross-core atomics in main.cpp only (modes/adapters don't access directly).

## Validation Summary

**Coverage:** 48 MVP + 37 Foundation + 36 Display System + 43 Delight = 164 FRs. All mapped to specific files and decisions. 21 MVP NFRs + 8 Foundation + 12 Display System + 21 Delight NFRs all addressed architecturally.

**No critical gaps.** Party mode reviews across 3+ sessions incorporated 30+ refinements. Key catches: platformio.ini build filter blocker, DisplayMode interface vs instance methods contradiction, terminal fallback for mode init failure, flight cycling timing as highest-risk extraction point.

**Readiness:** All decisions have code examples, struct definitions, API formats, state diagrams. File change maps complete. Enforcement rules with anti-patterns. Boot/loop integration points specified. Implementation sequence with dependencies. Tests required for ConfigManager, LayoutEngine, ModeRegistry, ModeOrchestrator, OTAUpdater, schedule NVS.

**MVP implementation handoff:** Add ESPAsyncWebServer Carbou fork + LittleFS to platformio.ini, create custom_partitions.csv, build and verify existing flight pipeline still works, then implement ConfigManager + SystemStatus as foundation.

**Foundation handoff:** Pre-implementation size gate (binary ≤1.3MB), then update partitions, expand ConfigManager (schedule), add OTA upload + self-check, integrate NTP + scheduler.

**Display System handoff:** Measure heap baseline post-Foundation, create DisplayMode interface, ModeRegistry, extract ClassicCardMode with pixel-parity validation gate (human confirmation required), then LiveFlightCardMode, then Mode Picker UI.

**Delight handoff:** ModeOrchestrator state machine, ClockMode + DeparturesBoardMode, OTAUpdater with GitHub API + SHA-256, schedule CRUD, mode settings schemas, UI for OTA Pull + scheduling.

]]></file>
<file id="ab213a12" path="_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md" label="DOCUMENTATION"><![CDATA[

# Story fn-1.3: OTA Upload Endpoint

Status: complete

## Story

As a **user**,
I want **to upload a firmware binary to the device over the local network**,
So that **I can update the firmware without connecting a USB cable**.

## Acceptance Criteria

1. **Given** `POST /api/ota/upload` receives a multipart file upload **When** the first chunk arrives with first byte `0xE9` (ESP32 magic byte) **Then** `Update.begin(partition->size)` is called using the next OTA partition's capacity (not `Content-Length`) **And** each subsequent chunk is written via `Update.write(data, len)` **And** the upload streams directly to flash with no full-binary RAM buffering

2. **Given** the first byte of the uploaded file is NOT `0xE9` **When** the first chunk is processed **Then** the endpoint returns HTTP 400 with `{ "ok": false, "error": "Not a valid ESP32 firmware image", "code": "INVALID_FIRMWARE" }` **And** no data is written to flash **And** validation completes within 1 second

3. **Given** `Update.write()` returns fewer bytes than expected **When** a write failure is detected **Then** `Update.abort()` is called **And** the current firmware continues running unaffected **And** an error response `{ "ok": false, "error": "Write failed — flash may be worn or corrupted", "code": "WRITE_FAILED" }` is returned

4. **Given** WiFi drops mid-upload **When** the connection is interrupted **Then** `Update.abort()` is called via `request->onDisconnect()` **And** the inactive partition is NOT marked bootable **And** the device continues running current firmware

5. **Given** the upload completes successfully **When** `Update.end(true)` returns true on the final chunk **Then** the endpoint returns `{ "ok": true, "message": "Rebooting..." }` **And** `ESP.restart()` is called after a 500ms delay using `esp_timer_start_once()` **And** the device reboots into the newly written firmware

6. **Given** a 1.5MB firmware binary uploaded over local WiFi **When** the transfer completes **Then** the total upload time is under 30 seconds

## Tasks / Subtasks

- [x] **Task 1: Add OTA upload state management struct** (AC: #1, #4)
  - [x] Create `OTAUploadState` struct similar to `LogoUploadState` with fields: `request`, `valid`, `started`, `bytesWritten`, `error`, `errorCode`
  - [x] Add global `std::vector<OTAUploadState> g_otaUploads` and helper functions `findOTAUpload()`, `clearOTAUpload()`
  - [x] Helper must call `Update.abort()` if `started` is true when cleaning up

- [x] **Task 2: Register POST /api/ota/upload route** (AC: #1, #2, #3, #4, #5)
  - [x] Add route using same pattern as `POST /api/logos` (lines 256-352 in WebPortal.cpp)
  - [x] Use upload handler signature: `(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final)`
  - [x] Register `request->onDisconnect()` to call `clearOTAUpload(request)` for WiFi interruption handling

- [x] **Task 3: Implement first-chunk validation and Update.begin()** (AC: #1, #2)
  - [x] On `index == 0`: check `data[0] == 0xE9` magic byte
  - [x] If invalid: set `state.valid = false`, `state.error = "Not a valid ESP32 firmware image"`, `state.errorCode = "INVALID_FIRMWARE"`, return early
  - [x] If valid: get next OTA partition via `esp_ota_get_next_update_partition(NULL)`
  - [x] Call `Update.begin(partition->size)` (NOT Content-Length, per AR18)
  - [x] Set `state.started = true` after successful `Update.begin()`
  - [x] Log partition info: `Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size)`

- [x] **Task 4: Implement streaming write per chunk** (AC: #1, #3)
  - [x] For each chunk (including first after validation): call `Update.write(data, len)`
  - [x] Check return value equals `len`; if not, set `state.valid = false`, `state.error = "Write failed — flash may be worn or corrupted"`, `state.errorCode = "WRITE_FAILED"`
  - [x] Call `Update.abort()` immediately on write failure
  - [x] Update `state.bytesWritten += len` for debugging/logging

- [x] **Task 5: Implement final chunk handling and reboot** (AC: #5)
  - [x] On `final == true`: call `Update.end(true)`
  - [x] If `Update.end()` returns false: set error `"Firmware verification failed"`, code `"VERIFY_FAILED"`, call `Update.abort()`
  - [x] If success: state remains valid for request handler

- [x] **Task 6: Implement request handler for response** (AC: #2, #3, #5)
  - [x] Check `state->valid` — if false, return `{ "ok": false, "error": state->error, "code": state->errorCode }` with HTTP 400
  - [x] If valid: return `{ "ok": true, "message": "Rebooting..." }` with HTTP 200
  - [x] Schedule reboot using existing `esp_timer_start_once()` pattern (see lines 491-498 in WebPortal.cpp)
  - [x] Use 500ms delay (500000 microseconds) to allow response to be sent
  - [x] Call `clearOTAUpload(request)` after sending response

- [x] **Task 7: Add OTA subsystem status updates** (AC: #3, #5)
  - [x] On successful upload start: `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Upload in progress")`
  - [x] On write failure: `SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Write failed")`
  - [x] On verification failure: `SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Verification failed")`
  - [x] On success before reboot: `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Update complete — rebooting")`

- [x] **Task 8: Add WebPortal header declarations** (AC: #1-#5)
  - [x] Add `_handleOTAUpload()` declaration in WebPortal.h (or keep inline like logo upload)
  - [x] Add `static void` helper for `_handleOTAUploadChunk()` if refactoring for readability

- [x] **Task 9: Add #include <Update.h>** (AC: #1, #3, #5)
  - [x] Add `#include <Update.h>` to WebPortal.cpp (ESP32 Arduino core library)
  - [x] Verify it compiles — Update.h is part of ESP32 Arduino core, no lib_deps needed

- [x] **Task 10: Build and test** (AC: #1-#6)
  - [x] Run `pio run` — verify clean build with no errors
  - [x] Verify binary size remains under 1.5MB limit (current: ~1.21MB = 76.9%)
  - [x] Manual test: upload invalid file (wrong magic byte) — expect 400 error < 1 second
  - [x] Manual test: upload valid firmware — expect success, reboot
  - [x] Manual test: disconnect WiFi mid-upload — verify device continues running

## Dev Notes

### Critical Architecture Constraints

**ESP32 Magic Byte Validation (HARD REQUIREMENT per AR-E12):**
ESP32 firmware binaries start with byte `0xE9`. Validate on first chunk BEFORE calling `Update.begin()`:
```cpp
if (index == 0 && len > 0 && data[0] != 0xE9) {
    state.valid = false;
    state.error = "Not a valid ESP32 firmware image";
    state.errorCode = "INVALID_FIRMWARE";
    return; // Early exit — no flash write
}
```

**Update.begin() Size Parameter (CRITICAL per AR18):**
Multipart `Content-Length` is the BODY size, NOT the file size. Do NOT use `total` from the upload handler. Instead, use the OTA partition capacity:
```cpp
const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
if (!partition) {
    state.valid = false;
    state.error = "No OTA partition available";
    state.errorCode = "NO_OTA_PARTITION";
    return;
}
if (!Update.begin(partition->size)) {
    state.valid = false;
    state.error = "Could not begin OTA update";
    state.errorCode = "BEGIN_FAILED";
    return;
}
```

**Streaming Architecture (HARD REQUIREMENT per AR-E13, NFR-C4):**
- Stream directly to flash via `Update.write()` per chunk
- NEVER buffer the full binary in RAM
- Each chunk is typically 4KB-16KB from ESPAsyncWebServer
- Total RAM impact: ~50-100 bytes for state struct only

**Abort on Any Error (CRITICAL):**
After `Update.begin()` is called, ANY error MUST call `Update.abort()`:
```cpp
if (writtenBytes != len) {
    Update.abort();  // MUST call before returning error
    state.valid = false;
    // ... set error
}
```

**Disconnect Handling Pattern:**
Follow the existing pattern from logo upload (WebPortal.cpp line 307):
```cpp
request->onDisconnect([request]() {
    auto* state = findOTAUpload(request);
    if (state && state->started) {
        Update.abort();  // Critical: abort in-progress update
    }
    clearOTAUpload(request);
});
```

**Reboot Timer Pattern:**
Use the existing `esp_timer_start_once()` pattern (WebPortal.cpp lines 491-498):
```cpp
static esp_timer_handle_t otaRebootTimer = nullptr;
if (!otaRebootTimer) {
    esp_timer_create_args_t args = {};
    args.callback = [](void*) { ESP.restart(); };
    args.name = "ota_reboot";
    esp_timer_create(&args, &otaRebootTimer);
}
esp_timer_start_once(otaRebootTimer, 500000); // 500ms in microseconds
```

### Response Format (Follow JSON Envelope Pattern)

**Success Response:**
```json
{
  "ok": true,
  "message": "Rebooting..."
}
```

**Error Responses:**
```json
{
  "ok": false,
  "error": "Not a valid ESP32 firmware image",
  "code": "INVALID_FIRMWARE"
}
```

Error codes:
| Code | Trigger |
|------|---------|
| `INVALID_FIRMWARE` | First byte != 0xE9 |
| `NO_OTA_PARTITION` | `esp_ota_get_next_update_partition()` returns NULL |
| `BEGIN_FAILED` | `Update.begin()` returns false |
| `WRITE_FAILED` | `Update.write()` returns fewer bytes than expected |
| `VERIFY_FAILED` | `Update.end(true)` returns false |
| `UPLOAD_ERROR` | Generic fallback for unexpected failures |

### OTAUploadState Struct

```cpp
struct OTAUploadState {
    AsyncWebServerRequest* request;
    bool valid;           // false if any validation/write failed
    bool started;         // true after Update.begin() succeeds
    size_t bytesWritten;  // for debugging/logging only
    String error;         // human-readable error message
    String errorCode;     // machine-readable error code
};
```

### ESP32 Update.h Library Reference

The `Update` class is a singleton from ESP32 Arduino core (`#include <Update.h>`):

| Method | Purpose |
|--------|---------|
| `Update.begin(size_t size)` | Start update with expected size (use partition size) |
| `Update.write(uint8_t* data, size_t len)` | Write chunk to flash, returns bytes written |
| `Update.end(bool evenIfRemaining)` | Finalize update, `true` = accept even if not all bytes written |
| `Update.abort()` | Cancel in-progress update, mark partition invalid |
| `Update.hasError()` | Check if error occurred |
| `Update.getError()` | Get error code |
| `Update.errorString()` | Get human-readable error string |

**Update library behavior:**
- `Update.begin()` erases flash blocks as needed (first call may take 2-3 seconds)
- `Update.write()` writes in 4KB flash block sizes internally
- `Update.end(true)` sets the boot partition to the newly written firmware
- After `Update.end(true)`, the next reboot loads the new firmware
- If new firmware crashes before self-check, bootloader rolls back automatically (Story fn-1.4)

Sources:
- [ESP32 Forum - OTA and Update.h](https://www.esp32.com/viewtopic.php?t=16512)
- [GitHub - arduino-esp32 Update examples](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/HTTPS_OTA_Update/HTTPS_OTA_Update.ino)
- [Last Minute Engineers - ESP32 OTA Web Updater](https://lastminuteengineers.com/esp32-ota-web-updater-arduino-ide/)

### Performance Requirements (NFR-P1, NFR-P7)

| Metric | Target |
|--------|--------|
| 1.5MB upload time | < 30 seconds over local WiFi |
| Invalid file rejection | < 1 second (fail on first chunk) |

The ESPAsyncWebServer receives chunks of 4-16KB depending on network conditions. At typical 10KB/chunk, a 1.5MB file arrives in ~150 chunks. Flash write speed is ~100-400KB/s depending on ESP32 flash quality.

### Partition Layout Reference

From `firmware/custom_partitions.csv` (Story fn-1.1):
```
# Name,   Type, SubType, Offset,    Size,    Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xE000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x180000,
app1,     app,  ota_1,   0x190000, 0x180000,
spiffs,   data, spiffs,  0x310000, 0xF0000,
```

- **app0**: 0x10000 to 0x18FFFF (1.5MB) — first OTA partition
- **app1**: 0x190000 to 0x30FFFF (1.5MB) — second OTA partition
- Running partition determined by `esp_ota_get_running_partition()`
- Next partition determined by `esp_ota_get_next_update_partition(NULL)`

### Project Structure Notes

**Files to modify:**
1. `firmware/adapters/WebPortal.cpp` — Add POST /api/ota/upload route + upload handler
2. `firmware/adapters/WebPortal.h` — Optional: add handler declaration if not using inline lambdas

**Files NOT to modify (handled by other stories):**
- `main.cpp` — Self-check logic is Story fn-1.4
- `dashboard.js` — UI is Story fn-1.6
- `ConfigManager.cpp` — Already has OTA subsystem from Story fn-1.2

**Code location guidance:**
- Insert new route registration after line 352 (after `POST /api/logos`) in `_registerRoutes()`
- Follow the exact pattern of logo upload handler for consistency
- Place `OTAUploadState` struct and helpers in the anonymous namespace (lines 38-98)

### Testing Strategy

**Automated build verification:**
```bash
cd firmware && pio run
# Expect: SUCCESS, binary < 1.5MB
```

**Manual test cases:**
1. **Invalid file test**: Upload a `.txt` file or random binary without 0xE9 header
   - Expect: HTTP 400, error message, device continues running
   - Verify: Response time < 1 second

2. **Valid firmware test**: Upload a valid `.bin` from `pio run` output
   - Expect: HTTP 200, "Rebooting..." message, device reboots in ~500ms
   - Verify: New firmware runs (check FW_VERSION in Serial output)

3. **Disconnect test**: Start upload, disconnect WiFi mid-transfer
   - Expect: Device continues running current firmware
   - Verify: No corruption, no crash

4. **Oversized file test**: Upload a file > 1.5MB (if possible to construct)
   - Expect: Upload proceeds but `Update.end()` may fail or fill partition
   - Note: This is an edge case; client-side validation (Story fn-1.6) prevents this

### Previous Story Intelligence (fn-1.2)

**From fn-1.2 completion notes:**
- Binary size: 1,209,440 bytes (1.15MB) — 76.9% of 1.5MB partition
- SystemStatus now has OTA and NTP subsystems ready (`Subsystem::OTA`)
- ConfigManager patterns established for new structs

**Lessons learned from fn-1.1 and fn-1.2:**
1. **Explicit error handling** — Always log and surface errors, never silent failures
2. **Thread safety** — OTA upload runs on AsyncTCP task (same as web server), no mutex needed for Upload singleton
3. **Test on hardware** — Unit tests build-pass but hardware verification essential

### References

- [Source: architecture.md#Decision F2: OTA Handler Architecture — WebPortal + main.cpp]
- [Source: epic-fn-1.md#Story fn-1.3: OTA Upload Endpoint]
- [Source: WebPortal.cpp lines 256-352 for logo upload pattern]
- [Source: WebPortal.cpp lines 491-498 for reboot timer pattern]
- [Source: prd.md#Technical Success criteria for OTA]
- [Source: architecture.md#AR-E12, AR-E13 enforcement rules]

### Dependencies

**This story depends on:**
- fn-1.1 (Partition Table & Build Configuration) — COMPLETE
- fn-1.2 (ConfigManager Expansion) — COMPLETE (provides SystemStatus::OTA)

**Stories that depend on this:**
- fn-1.4: OTA Self-Check & Rollback Detection — uses the firmware written by this endpoint
- fn-1.6: Dashboard Firmware Card & OTA Upload UI — calls this endpoint

### Risk Mitigation

1. **Flash wear concern**: Flash has ~100K write cycles. OTA updates are infrequent. Log write failures with helpful message.
2. **Partial write corruption**: `Update.abort()` marks partition invalid; bootloader ignores it. Safe.
3. **Power loss during write**: ESP32 bootloader handles this; rolls back to previous partition.
4. **Large file RAM exhaustion**: Streaming architecture prevents this. ESPAsyncWebServer chunks are 4-16KB max.

## Dev Agent Record

### Agent Model Used

Claude Opus 4 (Story Creation)

### Debug Log References

N/A — Story creation phase

### Completion Notes List

**2026-04-12: Ultimate context engine analysis completed**
- Comprehensive analysis of epic-fn-1.md extracted all 6 acceptance criteria with BDD format
- WebPortal.cpp patterns analyzed: logo upload (lines 256-352), reboot timer (lines 491-498)
- Existing OTA infrastructure verified: partition table ready, esp_ota_ops.h included in main.cpp
- ESP32 Update.h library research completed via web search
- All architecture requirements mapped: AR3, AR12, AR18, AR-E12, AR-E13
- Performance requirements documented: NFR-P1 (30s upload), NFR-P7 (1s validation)
- 10 tasks created with explicit code references and line numbers

**2026-04-12: Implementation completed**
- Added `OTAUploadState` struct in anonymous namespace (lines 83-91)
- Added helper functions `findOTAUpload()` and `clearOTAUpload()` (lines 95-114)
- Implemented `POST /api/ota/upload` route following logo upload pattern (lines 399-531)
- Added `#include <Update.h>` and `#include <esp_ota_ops.h>` (lines 27-28)
- Magic byte validation (0xE9) on first chunk prevents invalid firmware writes
- Partition size used for `Update.begin()` instead of Content-Length per AR18
- WiFi disconnect handling via `request->onDisconnect()` calls `Update.abort()`
- SystemStatus updates for OTA subsystem on all state transitions
- 500ms reboot delay using `esp_timer_start_once()` pattern
- Build successful: 1,216,384 bytes (77.3% of partition)
- All 6 acceptance criteria satisfied

**2026-04-12: Code review synthesis — 7 fixes applied**
- CRITICAL: Added `state->started = false` after `Update.end(true)` so `clearOTAUpload()` does NOT call `Update.abort()` on a successfully completed update (was aborting the newly written firmware)
- HIGH: Added `g_otaInProgress` flag + concurrent-upload guard; second upload during active OTA now returns `OTA_BUSY` (409 Conflict) instead of corrupting the Update singleton
- MEDIUM: Added `SystemStatus::set(ERROR)` for all previously-missing error paths: `INVALID_FIRMWARE`, `NO_OTA_PARTITION`, `BEGIN_FAILED` — now consistent with `WRITE_FAILED`/`VERIFY_FAILED`
- LOW: Added `Serial.printf("[OTA] Writing to %s, size 0x%x\n", ...)` partition log completing Task 3 requirement
- LOW: Server-side failures (`NO_OTA_PARTITION`, `BEGIN_FAILED`, `WRITE_FAILED`, `VERIFY_FAILED`) now return HTTP 500; concurrent conflict returns 409; client errors (`INVALID_FIRMWARE`) keep 400
- Build successful post-fixes: 1,217,104 bytes (77.4% of 1.5MB partition)

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Implementation complete - OTA upload endpoint with streaming, validation, and error handling | Claude |
| 2026-04-12 | Code review synthesis: 7 fixes — critical abort bug, concurrent guard, SystemStatus consistency, HTTP codes | Claude |

### File List

- `firmware/adapters/WebPortal.cpp` (MODIFIED) - Added OTA upload route, state struct, helpers; patched by code review synthesis

## Senior Developer Review (AI)

### Review: 2026-04-12
- **Reviewer:** AI Code Review Synthesis (4 validators)
- **Evidence Score:** 12 → REJECT (pre-fix); all critical/high issues resolved
- **Issues Found:** 8 verified (5 false positives dismissed)
- **Issues Fixed:** 7
- **Action Items Created:** 1

#### Review Follow-ups (AI)
- [ ] [AI-Review] MEDIUM: Add automated unit/integration tests for OTA endpoint — invalid magic byte rejection, write failure path, disconnect cleanup, and success-path must not abort (`firmware/test/` or `tests/smoke/test_web_portal_smoke.py`)


]]></file>
<file id="e97afb18" path="_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md" label="DOCUMENTATION"><![CDATA[


# Story fn-1.4: OTA Self-Check & Rollback Detection

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a **user**,
I want **the device to verify new firmware works after an OTA update and automatically roll back if it doesn't**,
So that **a bad firmware update never bricks my device**.

## Acceptance Criteria

1. **Given** the device boots after an OTA update **When** WiFi connects successfully **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called **And** the partition is marked valid **And** a log message records the time in milliseconds (e.g., `"Firmware marked valid — WiFi connected in 8432ms"`) **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Firmware verified — WiFi connected in Xs")` is recorded (X = elapsed seconds)

2. **Given** the device boots after an OTA update and WiFi does not connect **When** 60 seconds elapse since boot **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called on timeout **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Marked valid on timeout — WiFi not verified")` is recorded **And** a WARNING log message is emitted

3. **Given** the device boots after an OTA update and crashes before the self-check completes **When** the watchdog fires and the device reboots **Then** the bootloader detects the active partition was never marked valid **And** the bootloader rolls back to the previous partition automatically (this is ESP32 bootloader behavior, not application code)

4. **Given** a rollback has occurred **When** `esp_ota_get_last_invalid_partition()` returns non-NULL during boot **Then** a `rollbackDetected` flag is set to `true` **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Firmware rolled back to previous version")` is recorded **And** the invalid partition label is logged (e.g., `"Rollback detected: partition 'app1' was invalid"`)

5. **Given** `GET /api/status` is called **When** the response is built **Then** it includes `"firmware_version": "1.0.0"` (from `FW_VERSION` build flag) **And** `"rollback_detected": true/false` in the response data object

6. **Given** normal boot (not after OTA) **When** the self-check runs **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called (idempotent — safe on non-OTA boots, returns ESP_OK with no effect when image is already valid) **And** no `SystemStatus::set` call is made for the OTA subsystem (no OK, WARNING, or ERROR status is emitted)

## Tasks / Subtasks

- [ ] **Task 1: Add rollback detection at early boot** (AC: #4)
  - [ ] In `setup()`, immediately after `Serial.begin()` and firmware version log (line ~458), before `validatePartitionLayout()`:
  - [ ] Call `esp_ota_get_last_invalid_partition()` — if non-NULL, set a file-scope `static bool g_rollbackDetected = true`
  - [ ] Log: `Serial.printf("[OTA] Rollback detected: partition '%s' was invalid\n", invalid->label)`
  - [ ] Note: SystemStatus is not initialized yet at this point — defer the `SystemStatus::set()` call to Task 3

- [ ] **Task 2: Implement OTA self-check function** (AC: #1, #2, #6)
  - [ ] Create a file-scope function `void performOtaSelfCheck()` in main.cpp
  - [ ] Record `unsigned long bootStartMs = millis()` at top of setup() (before any delays) for accurate boot timing
  - [ ] Function checks WiFi state via `g_wifiManager.getState() == WiFiState::STA_CONNECTED`
  - [ ] **If WiFi connected:** call `esp_ota_mark_app_valid_cancel_rollback()`, log time: `"Firmware marked valid — WiFi connected in %lums"`, set SystemStatus OK
  - [ ] **If timeout (60s) reached:** call `esp_ota_mark_app_valid_cancel_rollback()`, log WARNING: `"Firmware marked valid on timeout — WiFi not verified"`, set SystemStatus WARNING
  - [ ] **If normal boot (not pending verify):** call `esp_ota_mark_app_valid_cancel_rollback()` anyway (idempotent, no status change needed)
  - [ ] Use `esp_ota_get_state_partition()` on running partition to check if `ESP_OTA_IMG_PENDING_VERIFY` — only log detailed timing when pending

- [ ] **Task 3: Integrate self-check into loop()** (AC: #1, #2, #4, #6)
  - [ ] Add file-scope state: `static bool g_otaSelfCheckDone = false` and `static unsigned long g_bootStartMs = 0`
  - [ ] Set `g_bootStartMs = millis()` at top of `setup()`
  - [ ] At the start of `loop()`, after `ConfigManager::tick()` and `g_wifiManager.tick()`:
    - If `!g_otaSelfCheckDone`: call `performOtaSelfCheck()` which checks WiFi or timeout
    - When check completes (WiFi connected OR 60s elapsed): set `g_otaSelfCheckDone = true`
  - [ ] Also in the self-check block: if `g_rollbackDetected` and SystemStatus is initialized, call `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Firmware rolled back to previous version")` (deferred from Task 1)

- [ ] **Task 4: Update GET /api/status response** (AC: #5)
  - [ ] In `SystemStatus::toExtendedJson()` (SystemStatus.cpp, line ~83), add to the root `obj`:
    - `obj["firmware_version"] = FW_VERSION;`
    - `obj["rollback_detected"] = <rollback flag>;`
  - [ ] Expose the rollback flag: either add `static bool getRollbackDetected()` to SystemStatus, or pass it from main.cpp via the existing `FlightStatsSnapshot` struct (preferred: add `bool rollback_detected` field to `FlightStatsSnapshot`)
  - [ ] Update `getFlightStatsSnapshot()` in main.cpp to include `g_rollbackDetected`

- [ ] **Task 5: Build and verify** (AC: #1-#6)
  - [ ] Run `~/.platformio/penv/bin/pio run` from `firmware/` — verify clean build with no errors
  - [ ] Verify binary size remains under 1.5MB limit
  - [ ] Log expected output for normal boot: `"[OTA] Firmware marked valid (normal boot)"` or similar minimal message
  - [ ] Log expected output for post-OTA boot: `"[OTA] Firmware marked valid — WiFi connected in XXXXms"`
  - [ ] Verify `GET /api/status` response includes `firmware_version` and `rollback_detected` fields

## Dev Notes

### Critical Architecture Constraints

**Architecture Decision F3: WiFi-OR-Timeout Strategy (HARD REQUIREMENT)**
The self-check marks firmware valid when EITHER WiFi connects OR 60s timeout expires — whichever comes first. No self-HTTP-request. No web server check. This is the simplest strategy that catches crash-on-boot failures.

**Timeout budget:**
| Scenario | Expected Duration |
|----------|------------------|
| Good firmware + WiFi available | 5-15s (WiFi connects) |
| Good firmware + no WiFi | 60s (timeout fallback) |
| Bad firmware (crash on boot) | Watchdog rollback (never reaches self-check) |

**ESP32 Rollback Mechanism (bootloader-level, not application code):**
1. After OTA write + `Update.end(true)`, new partition state = `ESP_OTA_IMG_NEW`
2. On reboot, bootloader transitions to `ESP_OTA_IMG_PENDING_VERIFY` and boots new partition
3. Application calls `esp_ota_mark_app_valid_cancel_rollback()` → state = `ESP_OTA_IMG_VALID`
4. If application crashes before marking valid → bootloader sets `ESP_OTA_IMG_ABORTED` on next boot and loads previous partition

**Arduino-ESP32 rollback is ENABLED by default** — `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` is baked into the prebuilt bootloader. No sdkconfig or build flag changes needed.

**Idempotent behavior:** `esp_ota_mark_app_valid_cancel_rollback()` returns `ESP_OK` with no effect when the running partition is already in `ESP_OTA_IMG_VALID` state. Safe to call on every boot.

### ESP32 OTA API Reference

All functions from `#include "esp_ota_ops.h"` (already included in main.cpp line 21):

```cpp
// Get last partition that failed validation (rollback detection)
const esp_partition_t* esp_ota_get_last_invalid_partition(void);
// Returns non-NULL if a rollback occurred; the returned partition was the failed one

// Mark current partition as valid, cancel pending rollback
esp_err_t esp_ota_mark_app_valid_cancel_rollback(void);
// Returns ESP_OK; idempotent if already valid

// Get OTA image state of a partition
esp_err_t esp_ota_get_state_partition(const esp_partition_t* partition, esp_ota_img_states_t* ota_state);
// States: ESP_OTA_IMG_NEW, ESP_OTA_IMG_PENDING_VERIFY, ESP_OTA_IMG_VALID, ESP_OTA_IMG_INVALID, ESP_OTA_IMG_ABORTED, ESP_OTA_IMG_UNDEFINED

// Get currently running partition (already used in validatePartitionLayout())
const esp_partition_t* esp_ota_get_running_partition(void);
```

### Implementation Pattern

```cpp
// File-scope globals in main.cpp
static bool g_rollbackDetected = false;
static bool g_otaSelfCheckDone = false;
static unsigned long g_bootStartMs = 0;

// Called once early in setup(), before SystemStatus::init()
void detectRollback() {
    const esp_partition_t* invalid = esp_ota_get_last_invalid_partition();
    if (invalid != NULL) {
        g_rollbackDetected = true;
        Serial.printf("[OTA] Rollback detected: partition '%s' was invalid\n", invalid->label);
    }
}

// Called from loop() until complete
void performOtaSelfCheck() {
    if (g_otaSelfCheckDone) return;

    // Check if we're in pending-verify state (post-OTA boot)
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    bool isPendingVerify = false;
    if (running && esp_ota_get_state_partition(running, &state) == ESP_OK) {
        isPendingVerify = (state == ESP_OTA_IMG_PENDING_VERIFY);
    }

    unsigned long elapsed = millis() - g_bootStartMs;
    bool wifiConnected = (g_wifiManager.getState() == WiFiState::STA_CONNECTED);

    if (wifiConnected || elapsed >= 60000) {
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        if (err == ESP_OK) {
            if (isPendingVerify) {
                if (wifiConnected) {
                    LOG_I("OTA", "Firmware marked valid — WiFi connected in %lums", elapsed);
                    SystemStatus::set(Subsystem::OTA, StatusLevel::OK,
                        ("Firmware verified — WiFi connected in " + String(elapsed / 1000) + "s").c_str());
                } else {
                    LOG_I("OTA", "Firmware marked valid on timeout — WiFi not verified");
                    SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
                        "Marked valid on timeout — WiFi not verified");
                }
            } else {
                LOG_V("OTA", "Self-check: already valid (normal boot)");
            }
        }

        // Deferred rollback status (SystemStatus wasn't ready in setup())
        if (g_rollbackDetected) {
            SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
                "Firmware rolled back to previous version");
        }

        g_otaSelfCheckDone = true;
    }
}
```

### Placement in main.cpp

**In `setup()` (line ~458 area, after Serial.begin and FW_VERSION log, BEFORE validatePartitionLayout):**
```cpp
g_bootStartMs = millis();  // Record boot time for self-check timing
detectRollback();          // Check for rollback before anything else
```

**In `loop()` (line ~701 area, after ConfigManager::tick() and g_wifiManager.tick()):**
```cpp
if (!g_otaSelfCheckDone) {
    performOtaSelfCheck();
}
```

### FlightStatsSnapshot Extension

Add to the existing struct in main.cpp (around line 100-110 area):
```cpp
// In the FlightStatsSnapshot struct definition (or wherever it's defined)
bool rollback_detected;  // Set from g_rollbackDetected
```

Update `getFlightStatsSnapshot()` to include:
```cpp
snapshot.rollback_detected = g_rollbackDetected;
```

### SystemStatus::toExtendedJson() Extension

Add to the function in SystemStatus.cpp (line ~107 area, in the `device` section or as top-level fields):
```cpp
obj["firmware_version"] = FW_VERSION;
obj["rollback_detected"] = stats.rollback_detected;
```

Note: `FW_VERSION` is a compile-time build flag macro set in `platformio.ini`. Add the conditional guard below at the top of `SystemStatus.cpp` — it resolves to nothing in normal builds where the flag is defined, and provides a safe fallback if the file is ever compiled outside PlatformIO (e.g., in a unit test harness):
```cpp
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"
#endif
```

### Critical Gotchas

1. **No factory partition** — The partition table has only app0 and app1 (no factory). If BOTH partitions become invalid, the device is bricked. However, this can only happen if the user OTA-updates with bad firmware AND the rollback target is also bad — extremely unlikely since the rollback target was previously running.

2. **Rollback detection persists until next successful OTA** — `esp_ota_get_last_invalid_partition()` returns the last invalid partition persistently (survives reboots). `g_rollbackDetected` will therefore be `true` on every subsequent boot until a new successful OTA upload overwrites the invalid partition slot. This means `/api/status` will return `"rollback_detected": true` on every boot until that happens — this is correct and intentional API behavior. The consumer (fn-1.6 dashboard) should display this state prominently until cleared by a successful OTA.

3. **Self-check runs on Core 1** — `loop()` runs on Core 1, which is correct. WiFiManager's WiFi stack also runs on Core 1. No cross-core issues.

4. **Order matters** — `detectRollback()` must run BEFORE `SystemStatus::init()` because SystemStatus isn't ready yet. The actual `SystemStatus::set()` call is deferred to `loop()` when both SystemStatus and the self-check are ready.

5. **Don't block setup()** — The 60-second timeout MUST be checked in `loop()`, not via `delay(60000)` in `setup()`. The architecture requires non-blocking operation.

### Response Format for GET /api/status

The existing response structure (from `toExtendedJson`) adds these new top-level fields:
```json
{
  "ok": true,
  "data": {
    "firmware_version": "1.0.0",
    "rollback_detected": false,
    "subsystems": { ... },
    "wifi_detail": { ... },
    "device": { ... },
    "flight": { ... },
    "quota": { ... }
  }
}
```

### Project Structure Notes

**Files to modify:**
1. `firmware/src/main.cpp` — Add rollback detection, self-check function, loop integration, boot timestamp
2. `firmware/core/SystemStatus.cpp` — Add `firmware_version` and `rollback_detected` to toExtendedJson()
3. `firmware/src/main.cpp` (FlightStatsSnapshot struct) — Add `rollback_detected` field

**Files NOT to modify:**
- `firmware/adapters/WebPortal.cpp` — No endpoint changes needed; `/api/status` handler already calls `toExtendedJson()`
- `firmware/core/ConfigManager.cpp` — No new NVS keys needed; rollback state comes from ESP32 bootloader, not NVS
- `firmware/core/SystemStatus.h` — OTA subsystem already exists (added in fn-1.2)
- `firmware/custom_partitions.csv` — Already has dual-OTA layout (fn-1.1)

**Code location guidance:**
- New globals (`g_rollbackDetected`, `g_otaSelfCheckDone`, `g_bootStartMs`) at file scope in main.cpp, near existing globals (line ~60-100)
- `detectRollback()` function near `validatePartitionLayout()` (line ~407 area)
- `performOtaSelfCheck()` function after `detectRollback()` 
- Loop integration after line 702 (`g_wifiManager.tick()`)

### References

- [Source: architecture.md#Decision F3: OTA Self-Check — WiFi-OR-Timeout Strategy]
- [Source: architecture.md#Decision F2: OTA Handler Architecture — WebPortal + main.cpp]
- [Source: epic-fn-1.md#Story fn-1.4: OTA Self-Check & Rollback Detection]
- [Source: prd.md#Update Mechanism — self-check, rollback]
- [Source: prd.md#Reliability NFR — automatic recovery from OTA failures]
- [Source: ESP-IDF OTA Documentation — esp_ota_ops.h API reference]
- [Source: main.cpp lines 409-445 — validatePartitionLayout() for existing esp_ota_ops usage pattern]
- [Source: main.cpp line 21 — esp_ota_ops.h already included]
- [Source: SystemStatus.cpp lines 83-132 — toExtendedJson() for /api/status response]

### Dependencies

**This story depends on:**
- fn-1.1 (Partition Table & Build Configuration) — COMPLETE (dual-OTA partition layout + FW_VERSION)
- fn-1.2 (ConfigManager Expansion) — COMPLETE (SystemStatus OTA subsystem ready)
- fn-1.3 (OTA Upload Endpoint) — COMPLETE (writes firmware to flash, triggers reboot)

**Stories that depend on this:**
- fn-1.6: Dashboard Firmware Card & OTA Upload UI — displays `firmware_version` and `rollback_detected` from `/api/status`

### Previous Story Intelligence

**From fn-1.1:** Binary size 1.15MB (76.8%). Partition table validated with `validatePartitionLayout()`. `esp_ota_ops.h` already included.

**From fn-1.2:** SystemStatus expanded to 8 subsystems. OTA subsystem (`Subsystem::OTA`) ready. Binary size 1.21MB (76.9%).

**From fn-1.3:** OTA upload endpoint complete. After successful upload, `Update.end(true)` marks new partition bootable and `ESP.restart()` is called after 500ms delay. Binary size 1.22MB (77.4%). Critical lesson: `state->started = false` after `Update.end(true)` to prevent cleanup from aborting a completed update.

**Antipatterns from previous stories to avoid:**
- Don't add fields to `/api/status` response without verifying the struct that carries the data (fn-1.3 had missing SystemStatus calls)
- Don't defer logging — log immediately when detecting rollback, even if SystemStatus isn't ready yet (use Serial.printf)
- Ensure all code paths set appropriate SystemStatus levels (fn-1.3 had missing ERROR paths)

### Testing Strategy

**Build verification:**
```bash
cd firmware && ~/.platformio/penv/bin/pio run
# Expect: SUCCESS, binary < 1.5MB
```

**Manual test cases:**

1. **Normal boot (no OTA):** Power cycle the device
   - Expect: `[OTA] Self-check: already valid (normal boot)` in Serial log (verbose level)
   - Expect: No OTA WARNING or ERROR in SystemStatus
   - Expect: `/api/status` returns `rollback_detected: false`

2. **Post-OTA boot with WiFi:** Upload firmware via `/api/ota/upload`, device reboots
   - Expect: `[OTA] Firmware marked valid — WiFi connected in XXXXms` in Serial log
   - Expect: SystemStatus OTA = OK with timing message
   - Expect: `/api/status` returns `firmware_version: "1.0.0"`, `rollback_detected: false`

3. **Post-OTA boot without WiFi:** Upload firmware, then disconnect WiFi router before reboot
   - Expect: After 60s, `[OTA] Firmware marked valid on timeout — WiFi not verified`
   - Expect: SystemStatus OTA = WARNING

4. **Rollback test (if possible):** Upload intentionally broken firmware
   - Expect: Device crashes, reboots, runs previous firmware
   - Expect: `[OTA] Rollback detected: partition 'appX' was invalid`
   - Expect: `/api/status` returns `rollback_detected: true`
   - Note: Creating intentionally broken firmware that boots far enough to trigger watchdog but not self-check is tricky — this is primarily validated by code review of the ESP32 bootloader mechanism

5. **API response verification:** Call `GET /api/status` after any boot
   - Expect: `firmware_version` field present with correct version string
   - Expect: `rollback_detected` field present as boolean

### Risk Mitigation

1. **esp_ota_get_last_invalid_partition persistence:** The invalid partition state persists across normal reboots. Verify that the flag doesn't create a permanent "rollback detected" state that never clears. The flag clears when a new successful OTA occurs (overwrites the invalid partition with good firmware).

2. **Timing edge case:** If WiFi connects at exactly 60s, both the WiFi-connected and timeout paths could trigger. Guard with `g_otaSelfCheckDone` flag — first check that completes wins.

3. **Heap impact:** This story adds ~200 bytes of stack for the self-check function and 3 file-scope variables (~10 bytes). Negligible impact on the ~280KB RAM budget.

## Dev Agent Record

### Agent Model Used

Claude Opus 4 (Story Creation)

### Debug Log References

N/A — Story creation phase

### Completion Notes List

**2026-04-12: Ultimate context engine analysis completed**
- Comprehensive analysis of epic-fn-1.md extracted all 6 acceptance criteria with BDD format
- Architecture Decision F3 (WiFi-OR-Timeout) mapped as hard requirement
- ESP32 OTA rollback API thoroughly researched: esp_ota_mark_app_valid_cancel_rollback(), esp_ota_get_last_invalid_partition(), esp_ota_get_state_partition()
- Arduino-ESP32 confirmed: CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y is default in prebuilt bootloader
- main.cpp analyzed: setup() initialization order, loop() structure, existing esp_ota_ops.h include, validatePartitionLayout() pattern
- SystemStatus.cpp analyzed: toExtendedJson() extension point identified at line ~107
- FlightStatsSnapshot struct identified as transport for rollback_detected flag to SystemStatus
- All 3 prerequisite stories (fn-1.1, fn-1.2, fn-1.3) completion notes reviewed for binary size, patterns, and lessons learned
- 5 tasks created with explicit code references, line numbers, and implementation patterns
- Antipatterns from fn-1.3 code review incorporated: consistent SystemStatus coverage, proper error path handling

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Code review synthesis: 5 issues fixed across 2 files | AI Synthesis |
| 2026-04-12 | Code review synthesis (round 2): 4 issues fixed in main.cpp | AI Synthesis |

### Completion Notes List (Synthesis)

**2026-04-12: Code review synthesis applied**
- 3 independent reviewers (A, B, D) analyzed implementation
- 5 verified issues fixed; 7 false positives dismissed
- HIGH: Fixed g_otaSelfCheckDone set to true on mark_valid failure — now only set on ESP_OK
- HIGH: Fixed LOG_I → LOG_W for timeout path to comply with AC #2 (added LOG_W macro to Log.h)
- MEDIUM: Fixed String().c_str() temporary anti-pattern — named String variable now used
- MEDIUM: Added static cache for isPendingVerify — avoids repeated IDF flash reads per loop iteration
- LOW: Added rationale comment for OTA_SELF_CHECK_TIMEOUT_MS constant
- DISMISSED: AC #6 rollback-WARNING-on-normal-boot — intentional per story Gotcha #2
- DISMISSED: g_bootStartMs = 0 risk — false positive (setup() always precedes loop())
- DISMISSED: SystemStatus mutex best-effort fallback — pre-existing pattern, not story scope
- DISMISSED: Magic numbers in tickStartupProgress (2000/4000) — pre-existing code, not story scope
- DISMISSED: LOG macros "inconsistent" with Serial.printf — LOG macros don't support printf format strings; mixed use is correct project pattern
- ACTION ITEM: No automated Unity tests for OTA boot logic — deferred (requires new test file)

**2026-04-12: Code review synthesis round 2 applied**
- 3 independent reviewers (A, B, C) analyzed post-synthesis implementation
- 4 verified issues fixed; 15 false positives dismissed
- MEDIUM: Fixed `s_isPendingVerify` error caching — removed premature `= 0` assignment before conditional; state probe now retries on transient IDF errors instead of permanently misclassifying pending-verify boot
- LOW: Fixed rollback WARNING decoupled from mark_valid success — moved `SystemStatus::set(WARNING, "rolled back")` before mark_valid call with static guard; satisfies AC #4 even if mark_valid persistently fails
- LOW: Fixed boot timing accuracy — moved `g_bootStartMs = millis()` before `Serial.begin()/delay(200)` per story task requirement (captures 200ms of startup previously excluded)
- LOW: Added log spam throttle to mark_valid error path — `s_markValidErrorLogged` static guard prevents repeated Serial.printf + SystemStatus::set on every loop retry when mark_valid fails
- DISMISSED: Normal boot after rollback emits WARNING — already dismissed in round 1; intentional per Gotcha #2
- DISMISSED: String concat → snprintf — named String variables already present from round 1; snprintf is style preference only
- DISMISSED: Redundant `if (g_otaSelfCheckDone) return;` inside function — defense-in-depth guard acceptable alongside loop() guard
- DISMISSED: FW_VERSION fallback duplication — intentional per story Dev Notes guidance; minor DRY issue not worth a new header
- DISMISSED: SystemStatus mutex best-effort — pre-existing pattern already dismissed in round 1
- DISMISSED: Magic numbers in tickStartupProgress / validatePartitionLayout — pre-existing code from prior stories, out of scope
- DISMISSED: LOG macros vs Serial.printf — correct mixed use, already dismissed in round 1
- DISMISSED: Race condition on WiFi getState() — false positive; loop() and WiFi callbacks both on Core 1 per architecture constraints
- DISMISSED: millis() overflow — not a real bug at 60s timeout (wrap at 49.7 days)
- DISMISSED: SRP violation in performOtaSelfCheck() — acceptable for embedded firmware complexity
- DISMISSED: Unnecessary `if (!g_otaSelfCheckDone)` overhead in loop() — single bool check, negligible
- DISMISSED: Lying test `test_system_status_ota_no_change_on_normal_boot` — limitation is documented in the comment; test correctly verifies initial state contract
- DISMISSED: FW_VERSION format not validated at runtime — compile-time constant; runtime validation is over-engineering
- ACTION ITEM: No automated tests for performOtaSelfCheck() core logic — deferred; static function not directly testable; existing tests cover API contract

### File List

- `firmware/src/main.cpp` (MODIFIED) — Rollback detection, self-check function, loop integration, boot timestamp, FlightStatsSnapshot extension; round-1 synthesis fixes: isPendingVerify cache, String temp cleanup, g_otaSelfCheckDone moved to success path, timeout comment; round-2 synthesis fixes: isPendingVerify error-retry, rollback WARNING before mark_valid, boot timing accuracy, mark_valid error log throttle
- `firmware/core/SystemStatus.cpp` (MODIFIED) — Add firmware_version and rollback_detected to toExtendedJson()
- `firmware/utils/Log.h` (MODIFIED) — Added LOG_W macro for warning-level log messages

## Senior Developer Review (AI)

### Review: 2026-04-12
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 4.5–10.2 across reviewers → CHANGES REQUESTED
- **Issues Found:** 5 verified (after dismissing 7 false positives)
- **Issues Fixed:** 5
- **Action Items Created:** 1

### Review: 2026-04-12 (Round 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 4.0 / 6.8 / 2.6 across reviewers → CHANGES REQUESTED
- **Issues Found:** 4 verified (after dismissing 15 false positives)
- **Issues Fixed:** 4
- **Action Items Created:** 1

## Tasks / Subtasks (continued)

#### Review Follow-ups (AI)
- [ ] [AI-Review] HIGH: Add automated Unity tests for OTA boot logic and /api/status contract — pending-verify+WiFi, pending-verify+timeout, normal boot no-status, rollback serialization (`firmware/test/test_ota_self_check/`)
- [ ] [AI-Review] HIGH: Add automated tests that directly exercise performOtaSelfCheck() core logic — requires exposing function from static (e.g., conditional compilation seam or test-hook header) (`firmware/test/test_ota_self_check/`)

### Review Findings (bmad-code-review workflow, 2026-04-12)

- [x] [Review][Patch] If `esp_ota_get_state_partition` fails on the first loop iteration where WiFi is already connected, `s_isPendingVerify` can remain `-1`, so `isPendingVerify` is false, `g_otaSelfCheckDone` is set, and AC #1/#2 observability (Serial timing line, `SystemStatus::set` for pending-verify paths) can be skipped even though `esp_ota_mark_app_valid_cancel_rollback()` ran. Mitigation: bounded retry of the state probe in the same visit (e.g. small loop in the existing `s_isPendingVerify == -1` block) before treating completion messaging as “normal boot”. [`firmware/src/main.cpp` ~438–449] — **Fixed 2026-04-12:** `tryResolveOtaPendingVerifyCache()` with `OTA_PENDING_VERIFY_PROBE_ATTEMPTS`, called each loop and again when WiFi/timeout completion fires.
- [x] [Review][Defer] `pio test -e esp32dev -f test_ota_self_check` attempts device upload; with no board attached the Unity suite does not run in CI/local agent shells. Consider `test_build_src` + upload skip / host-only test env for contract tests. — deferred, tooling


]]></file>
<file id="60733e24" path="_bmad-output/implementation-artifacts/stories/fn-1-6-dashboard-firmware-card-and-ota-upload-ui.md" label="DOCUMENTATION"><![CDATA[


# Story fn-1.6: Dashboard Firmware Card & OTA Upload UI

Status: Ready for Review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a **user**,
I want **a Firmware card on the dashboard where I can upload firmware, see progress, view the current version, and be notified of rollbacks**,
So that **I can manage firmware updates from my phone**.

## Acceptance Criteria

1. **Given** the dashboard loads **When** `GET /api/status` returns firmware data **Then** the Firmware card displays the current firmware version (e.g., "v1.0.0") **And** the Firmware card is always expanded (not collapsed by default) **And** the dashboard loads within 3 seconds on the local network

2. **Given** the Firmware card is visible **When** no file is selected **Then** the OTA Upload Zone (`.ota-upload-zone`) shows: dashed border, "Drag .bin file here or tap to select" text **And** the zone has `role="button"`, `tabindex="0"`, `aria-label="Select firmware file for upload"` **And** Enter/Space keyboard triggers the file picker

3. **Given** a file is dragged over the upload zone **When** the drag enters the zone **Then** the border becomes solid `--accent` and background lightens slightly

4. **Given** a valid `.bin` file is selected (≤1.5MB, first byte `0xE9`) **When** client-side validation passes **Then** the filename is displayed and the "Upload Firmware" primary button is enabled

5. **Given** an invalid file is selected (wrong size or missing magic byte) **When** client-side validation fails **Then** an error toast fires with a specific message (e.g., "File too large — maximum 1.5MB for OTA partition" or "Not a valid ESP32 firmware image") **And** the upload zone resets to empty state

6. **Given** "Upload Firmware" is tapped **When** the upload begins via XMLHttpRequest **Then** the upload zone is replaced by the OTA Progress Bar (`.ota-progress`) **And** the progress bar shows percentage (0-100%) with `role="progressbar"`, `aria-valuenow`/`min`/`max` **And** the percentage updates at least every 2 seconds

7. **Given** the upload completes successfully **When** the server responds with `{ "ok": true }` **Then** a 3-second countdown displays ("Rebooting in 3... 2... 1...") **And** polling begins for device reachability (`GET /api/status`) **And** on successful poll, the new version string is displayed **And** a success toast shows "Updated to v[new version]"

8. **Given** the device is unreachable after 60 seconds of polling **When** the timeout expires **Then** an amber message shows "Device unreachable — try refreshing" with explanatory text

9. **Given** `rollback_detected` is `true` in `/api/status` **When** the dashboard loads **Then** a Persistent Banner (`.banner-warning`) appears in the Firmware card: "Firmware rolled back to previous version" **And** the banner has `role="alert"`, `aria-live="polite"`, hardcoded `#2d2738` background **And** a dismiss button sends `POST /api/ota/ack-rollback` and removes the banner **And** the banner persists across page refreshes until dismissed (NVS-backed)

10. **Given** the System card on the dashboard **When** rendered **Then** a "Download Settings" secondary button triggers `GET /api/settings/export` **And** helper text in the Firmware card links to the System card: "Before migrating, download your settings backup from System below"

11. **Given** all new CSS for OTA components **When** added to `style.css` **Then** total addition is approximately 35 lines (~150 bytes gzipped) **And** all components meet WCAG 2.1 AA contrast requirements **And** all interactive elements have 44x44px minimum touch targets **And** all components work at 320px minimum viewport width

12. **Given** updated web assets (dashboard.html, dashboard.js, style.css) **When** gzipped and placed in `firmware/data/` **Then** the gzipped copies replace existing ones for LittleFS upload

## Tasks / Subtasks

- [x] **Task 1: Add Firmware card HTML to dashboard.html** (AC: #1, #2, #3, #10)
  - [x] Add new `<section class="card" id="firmware-card">` between the Logos card and System card
  - [x] Card content: `<h2>Firmware</h2>`, version display `<p id="fw-version">`, rollback banner placeholder `<div id="rollback-banner">`, OTA upload zone, progress bar placeholder, reboot countdown placeholder
  - [x] OTA Upload Zone: `<div class="ota-upload-zone" id="ota-upload-zone" role="button" tabindex="0" aria-label="Select firmware file for upload">` with prompt text "Drag .bin file here or tap to select" and hidden file input `<input type="file" id="ota-file-input" accept=".bin" hidden>`
  - [x] File selection display: `<div class="ota-file-info" id="ota-file-info" style="display:none">` with filename span and "Upload Firmware" primary button
  - [x] Progress bar: `<div class="ota-progress" id="ota-progress" style="display:none">` with inner bar, percentage text, and `role="progressbar"` attributes
  - [x] Reboot countdown: `<div class="ota-reboot" id="ota-reboot" style="display:none">` with countdown text
  - [x] Add helper text: `<p class="helper-copy">Before migrating, download your settings backup from <a href="#system-card">System</a> below.</p>`
  - [x] Add `id="system-card"` to the existing System card `<section>` for the anchor link

- [x] **Task 2: Add "Download Settings" button to System card** (AC: #10)
  - [x] In the System card section (currently has only Reset to Defaults), add a "Download Settings" secondary button: `<button type="button" class="btn-secondary" id="btn-export-settings">Download Settings</button>`
  - [x] Add helper text below: `<p class="helper-copy">Download your configuration as a JSON file before partition migration or reflash.</p>`
  - [x] Place this ABOVE the danger zone reset button (less destructive action first)

- [x] **Task 3: Add new endpoint POST /api/ota/ack-rollback** (AC: #9)
  - [x] In `WebPortal.cpp`, in `_registerRoutes()`, add handler for `POST /api/ota/ack-rollback`
  - [x] Handler: set NVS key `ota_rb_ack` to `1` (uint8), respond `{ "ok": true }`
  - [x] In `WebPortal.h`, no new method needed — can be inline lambda in `_registerRoutes()` (same pattern as positioning handlers)

- [x] **Task 4: Add GET /api/settings/export endpoint** (AC: #10)
  - [x] In `WebPortal.cpp`, in `_registerRoutes()`, add handler for `GET /api/settings/export`
  - [x] Handler builds JSON using existing `ConfigManager::dumpSettingsJson()` method
  - [x] Add metadata fields: `"flightwall_settings_version": 1`, `"exported_at": "<ISO-8601 timestamp>"` (use `time()` with NTP or `millis()` fallback)
  - [x] Set response header: `Content-Disposition: attachment; filename=flightwall-settings.json`
  - [x] Response content-type: `application/json`

- [x] **Task 5: Expose rollback acknowledgment state in /api/status** (AC: #9)
  - [x] Add NVS key `ota_rb_ack` (uint8, default 0) — stored directly via `Preferences` in WebPortal (not via ConfigManager; this is a simple transient flag, not a user config key)
  - [x] In `SystemStatus::toExtendedJson()`, add `"rollback_acknowledged"` field: `true` if NVS key `ota_rb_ack` == 1
  - [x] When a NEW OTA upload completes successfully (fn-1.3 handler, after `Update.end(true)` returns true and before the reboot delay), clear the `ota_rb_ack` key by setting it to `0` — so a subsequent rollback will show the banner again. Do NOT clear at upload start; if the upload fails the ack must remain intact.

- [x] **Task 6: Add OTA CSS to style.css** (AC: #11)
  - [x] `.ota-upload-zone` — Reuse pattern from `.logo-upload-zone`: dashed border `2px dashed var(--border)`, padding, min-height 80px, cursor pointer, text-align center, flex column
  - [x] `.ota-upload-zone.drag-over` — Solid border `var(--primary)`, background rgba tint
  - [x] `.ota-file-info` — Filename + button row; flexbox with gap
  - [x] `.ota-progress` — Container for progress bar: height 28px, border-radius, background `var(--border)`
  - [x] `.ota-progress-bar` — Inner fill: height 100%, background `var(--primary)`, transition width 0.3s, border-radius
  - [x] `.ota-progress-text` — Centered percentage text over bar: position absolute, color white
  - [x] `.ota-reboot` — Countdown text: text-align center, padding, font-size larger
  - [x] `.banner-warning` — Persistent amber banner: background `#2d2738`, border-left 4px solid `var(--warning, #d29922)`, padding 12px 16px, border-radius, display flex, justify-content space-between, align-items center
  - [x] `.banner-warning .dismiss-btn` — Small X button: min 44x44px touch target
  - [x] `.btn-secondary` — If not already existing: outline style button, border `var(--border)`, background transparent, color `var(--text)`
  - [x] Target: ~35 lines, ~150 bytes gzipped
  - [x] All interactive elements ≥ 44x44px touch targets
  - [x] Verify contrast ratios meet WCAG 2.1 AA (4.5:1 for text, 3:1 for UI components)

- [x] **Task 7: Implement firmware card JavaScript in dashboard.js** (AC: #1, #2, #3, #4, #5, #6, #7, #8, #9)
  - [x] **Version display**: In `loadSettings()` or a new `loadFirmwareStatus()` function, call `GET /api/status`, extract `firmware_version` and `rollback_detected`, populate `#fw-version` text
  - [x] **Rollback banner**: If `rollback_detected === true` AND not acknowledged (check via `rollback_acknowledged` field in status response), show `.banner-warning` with dismiss button. Dismiss handler: `FW.post('/api/ota/ack-rollback')`, then hide banner.
  - [x] **File selection via click**: Click on `.ota-upload-zone` or Enter/Space key → trigger hidden `#ota-file-input` click
  - [x] **Drag and drop**: `dragenter`/`dragover` on zone → add `.drag-over` class, prevent default. `dragleave`/`drop` → remove class. On `drop`, extract `e.dataTransfer.files[0]`
  - [x] **Client-side validation**: Read first 4 bytes via `FileReader.readAsArrayBuffer()`. Check: (a) file size ≤ 1,572,864 bytes (1.5MB = 0x180000), (b) first byte === 0xE9 (ESP32 magic). On failure: `FW.showToast(message, 'error')` and reset zone.
  - [x] **File info display**: On valid file, hide upload zone, show `#ota-file-info` with filename and enabled "Upload Firmware" button
  - [x] **Upload via XMLHttpRequest**: Use `XMLHttpRequest` (not `fetch`) for upload progress events. `xhr.upload.onprogress` → update progress bar width and `aria-valuenow`. `FormData` with file appended. POST to `/api/ota/upload`.
  - [x] **Progress bar**: Hide file info, show `#ota-progress`. Update bar width as percentage. Ensure updates at least every 2 seconds (XHR progress events are browser-driven; add a minimum interval check if needed).
  - [x] **Success handler**: On `xhr.status === 200` and `response.ok === true`, show countdown "Rebooting in 3... 2... 1..." via `setInterval` decrementing. After countdown, begin polling.
  - [x] **Reboot polling**: `setInterval` every 3 seconds calling `GET /api/status`. On success: extract new `firmware_version`, show success toast "Updated to v[version]", update version display, reset OTA zone to initial state. Clear interval.
  - [x] **Polling timeout**: After 60 seconds (20 attempts at 3s each), stop polling, show amber message "Device unreachable — try refreshing. The device may have changed IP address after reboot."
  - [x] **Error handling**: On upload failure (non-200 response), parse error JSON, show error toast with specific message from server. Reset upload zone to initial state.
  - [x] **Cancel/reset**: Provide a way to cancel file selection (click zone again or select new file)

- [x] **Task 8: Add settings export JavaScript** (AC: #10)
  - [x] Click handler on `#btn-export-settings`: trigger `window.location.href = '/api/settings/export'` — browser downloads the JSON file via Content-Disposition header
  - [x] Alternative: `fetch('/api/settings/export')` then create blob URL and trigger download — but direct navigation is simpler and works universally

- [x] **Task 9: Gzip updated web assets** (AC: #12)
  - [x] From `firmware/` directory, regenerate gzipped assets:
    ```
    gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
    gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz
    gzip -9 -c data-src/style.css > data/style.css.gz
    ```
  - [x] Verify gzipped files replaced in `firmware/data/`

- [x] **Task 10: Build and verify** (AC: #1-#12)
  - [x] Run `~/.platformio/penv/bin/pio run` from `firmware/` — verify clean build
  - [x] Verify binary size remains under 1.5MB limit
  - [x] Measure total gzipped web asset size (should remain well under 50KB budget)

## Dev Notes

### Critical Architecture Constraints

**Web Asset Build Pipeline (MANUAL)**
Per project memory: No automated gzip build script. After editing any file in `firmware/data-src/`, manually run:
```bash
cd firmware
gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz  
gzip -9 -c data-src/style.css > data/style.css.gz
```

**Dashboard Card Ordering**
The dashboard currently has these cards in order:
1. Display
2. Display Mode (Mode Picker)
3. Timing
4. Network & API
5. Hardware
6. Calibration (collapsible)
7. Location (collapsible)
8. Logos
9. System (danger zone)

The Firmware card should be inserted between **Logos** and **System** — this matches the UX spec's dashboard ordering (Firmware before System) and keeps the danger zone at the bottom.

**Existing API Patterns**
- All API calls use `FW.get()` / `FW.post()` from `common.js`
- Error responses always: `{ "ok": false, "error": "message", "code": "CODE" }`
- Success responses: `{ "ok": true, "data": {...} }` or `{ "ok": true, "message": "..." }`
- Toast notifications: `FW.showToast(message, 'success'|'warning'|'error')`

**OTA Upload is ALREADY IMPLEMENTED server-side (fn-1.3)**
The `POST /api/ota/upload` endpoint exists in `WebPortal.cpp`. It:
- Accepts multipart form upload
- Validates magic byte `0xE9` on first chunk
- Streams directly to flash via `Update.write()`
- Returns `{ "ok": true, "message": "Rebooting..." }` on success
- Returns error with code (INVALID_FIRMWARE, OTA_BUSY, etc.) on failure
- Reboots after 500ms delay on success

**You do NOT need to modify the upload endpoint** — only create the client-side UI.

**OTA Self-Check (fn-1.4) provides version and rollback data**
`GET /api/status` returns:
```json
{
  "ok": true,
  "data": {
    "firmware_version": "1.0.0",
    "rollback_detected": false,
    ...
  }
}
```

**Rollback persistence behavior (fn-1.4 Gotcha #2):**
`rollback_detected` remains `true` on every boot until a new successful OTA clears the invalid partition. The banner must handle this — show persistently until user acknowledges via `POST /api/ota/ack-rollback`.

### XMLHttpRequest vs Fetch for Upload

Use `XMLHttpRequest` instead of `fetch()` for the firmware upload because:
1. `xhr.upload.onprogress` provides real upload progress events (bytes sent / total)
2. `fetch()` does not support upload progress in any browser
3. The existing logo upload in `dashboard.js` uses `fetch()` but without progress — OTA needs progress

**Pattern:**
```javascript
function uploadFirmware(file) {
  const xhr = new XMLHttpRequest();
  const formData = new FormData();
  formData.append('firmware', file, file.name);
  
  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      updateProgressBar(pct);
    }
  };
  
  xhr.onload = function() {
    if (xhr.status === 200) {
      const resp = JSON.parse(xhr.responseText);
      if (resp.ok) startRebootCountdown();
      else showUploadError(resp.error);
    } else {
      try {
        const resp = JSON.parse(xhr.responseText);
        showUploadError(resp.error || 'Upload failed');
      } catch(e) {
        showUploadError('Upload failed — status ' + xhr.status);
      }
    }
  };
  
  xhr.onerror = function() {
    showUploadError('Connection lost during upload');
  };
  
  xhr.open('POST', '/api/ota/upload');
  xhr.send(formData);
}
```

### Client-Side File Validation

The ESP32 magic byte validation MUST happen client-side before upload to provide immediate feedback (AC #5). Read the first byte using FileReader:

```javascript
function validateFirmwareFile(file) {
  return new Promise((resolve, reject) => {
    if (file.size > 1572864) {  // 1.5MB = 0x180000
      reject('File too large — maximum 1.5MB for OTA partition');
      return;
    }
    
    const reader = new FileReader();
    reader.onload = function(e) {
      const bytes = new Uint8Array(e.target.result);
      if (bytes[0] !== 0xE9) {
        reject('Not a valid ESP32 firmware image');
        return;
      }
      resolve();
    };
    reader.onerror = function() {
      reject('Could not read file');
    };
    reader.readAsArrayBuffer(file.slice(0, 4));  // Only read first 4 bytes
  });
}
```

### Reboot Polling Strategy

After successful upload, the device reboots after 500ms. The dashboard must:
1. Show 3-second countdown ("Rebooting in 3... 2... 1...")
2. After countdown, poll `GET /api/status` every 3 seconds
3. On success: extract `firmware_version`, show success toast
4. On timeout (60s / 20 polls): show amber warning

The device typically takes 5-15s to reboot and reconnect to WiFi. Total expected wait: countdown (3s) + reboot (5-15s) = 8-18s.

**Important:** The device's IP may change after reboot if DHCP reassigns. The polling should use the current window location's hostname. If the device got a new IP, the polling will time out and the user needs to find the new IP (via router or mDNS `flightwall.local`).

### NVS Key for Rollback Acknowledgment

**Key:** `ota_rb_ack` (uint8, default 0)
- Set to `1` when user dismisses the rollback banner (POST /api/ota/ack-rollback)
- Reset to `0` when a new OTA upload begins (in the OTA upload handler, fn-1.3 code)
- Read in `/api/status` response or as a separate query

**Implementation choice:** Rather than adding this to ConfigManager's formal config structs (overkill for a simple flag), use direct NVS access in WebPortal:
```cpp
#include <Preferences.h>
// In ack-rollback handler:
Preferences prefs;
prefs.begin("flightwall", false);
prefs.putUChar("ota_rb_ack", 1);
prefs.end();
```

And expose in the status response by adding to `FlightStatsSnapshot`:
```cpp
// In main.cpp getFlightStatsSnapshot():
Preferences prefs;
prefs.begin("flightwall", true);  // read-only
s.rollback_acked = prefs.getUChar("ota_rb_ack", 0) == 1;
prefs.end();
```

Then in `SystemStatus::toExtendedJson()`:
```cpp
obj["rollback_acknowledged"] = stats.rollback_acked;
```

**ALTERNATIVE (simpler):** Add the ack check directly in the ack-rollback POST handler and the status GET handler in WebPortal.cpp, avoiding any changes to FlightStatsSnapshot or SystemStatus. The handler reads/writes NVS directly. The status handler reads NVS when building the response. This keeps the scope minimal.

### Settings Export Endpoint

The data layer already exists: `ConfigManager::dumpSettingsJson(JsonObject& out)` writes all config keys to a JSON object. The endpoint just needs to:
1. Create a JsonDocument
2. Call `dumpSettingsJson()` on it
3. Add metadata (`flightwall_settings_version`, `exported_at`)
4. Set Content-Disposition header
5. Send response

```cpp
// In _registerRoutes():
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
```

### Reset ota_rb_ack on New OTA Upload

When a new OTA upload **completes successfully** (in the existing upload handler in WebPortal.cpp, after `Update.end(true)` returns `true` and before the 500ms reboot delay), clear the rollback ack:
```cpp
// In the OTA upload handler, after Update.end(true) succeeds, before ESP.restart():
Preferences prefs;
prefs.begin("flightwall", false);
prefs.putUChar("ota_rb_ack", 0);
prefs.end();
```

**Important:** Clear the flag only on successful completion — NOT at upload start (`index == 0`). If the upload fails mid-stream, the existing acknowledgment must remain so the rollback banner stays dismissed until the user explicitly re-acknowledges or a good OTA succeeds.

### Dashboard Load Sequence

The dashboard currently calls `loadSettings()` on DOMContentLoaded. For the Firmware card, add a new `loadFirmwareStatus()` call that:
1. Fetches `GET /api/status` (the health endpoint already exists)
2. Extracts `firmware_version` → displays in `#fw-version`
3. Extracts `rollback_detected` and `rollback_acknowledged` → shows/hides banner
4. This can run in parallel with `loadSettings()` since they hit different endpoints

### CSS Design System Tokens to Reuse

From the existing `style.css`:
- `--bg: #0d1117` — background
- `--surface: #161b22` — card background
- `--border: #30363d` — borders, dashed upload zone border
- `--text: #e6edf3` — primary text
- `--text-muted: #8b949e` — helper text, secondary info
- `--primary: #58a6ff` — buttons, progress bar fill, drag-over border
- `--error: #f85149` — error states
- `--success: #3fb950` — success toast
- `--radius: 8px` — border radius
- `--gap: 16px` — spacing

New token: rollback banner background `#2d2738` (hardcoded per AC #9).

### Accessibility Requirements Summary

- Upload zone: `role="button"`, `tabindex="0"`, `aria-label`
- Progress bar: `role="progressbar"`, `aria-valuenow`, `aria-valuemin="0"`, `aria-valuemax="100"`
- Rollback banner: `role="alert"`, `aria-live="polite"`
- All touch targets ≥ 44x44px
- Works at 320px viewport width
- WCAG 2.1 AA contrast ratios

### Project Structure Notes

**Files to modify:**
1. `firmware/data-src/dashboard.html` — Add Firmware card HTML, System card ID, Download Settings button
2. `firmware/data-src/dashboard.js` — Add firmware status loading, OTA upload UI logic, settings export trigger
3. `firmware/data-src/style.css` — Add OTA component styles (~35 lines)
4. `firmware/adapters/WebPortal.cpp` — Add `POST /api/ota/ack-rollback` and `GET /api/settings/export` endpoints, clear `ota_rb_ack` on successful OTA completion (before reboot)
5. `firmware/core/SystemStatus.cpp` — Add `rollback_acknowledged` to `toExtendedJson()` (or handle in WebPortal directly)
6. `firmware/data/dashboard.html.gz` — Regenerated
7. `firmware/data/dashboard.js.gz` — Regenerated
8. `firmware/data/style.css.gz` — Regenerated

**Files NOT to modify:**
- `firmware/adapters/WebPortal.h` — No new class methods needed; new endpoints use inline lambdas
- `firmware/core/ConfigManager.h/.cpp` — `ota_rb_ack` is a simple NVS flag, not a formal config key
- `firmware/src/main.cpp` — No changes needed; `FlightStatsSnapshot` could optionally be extended but the simpler approach is NVS-direct in WebPortal
- `firmware/data-src/common.js` — `FW.get/post/showToast` already sufficient
- `firmware/data-src/health.html/.js` — Health page already displays `firmware_version` from `/api/status`

### References

- [Source: epic-fn-1.md#Story fn-1.6 — All acceptance criteria]
- [Source: architecture.md#D4 — REST endpoint patterns, JSON envelope]
- [Source: architecture.md#F7 — POST /api/ota/upload, GET /api/settings/export endpoints]
- [Source: prd.md#Update Mechanism — OTA via web UI]
- [Source: prd.md#FR10 — Persist configuration to NVS]
- [Source: prd.md#NFR Performance — Dashboard loads within 3 seconds]
- [Source: ux-design-specification-delight.md#OTA Progress Display — States and patterns]
- [Source: fn-1-3 story — OTA upload endpoint implementation details]
- [Source: fn-1-4 story — Rollback detection, firmware_version in /api/status]
- [Source: WebPortal.cpp lines 458-620 — Existing OTA upload handler]
- [Source: SystemStatus.cpp lines 89-90 — firmware_version/rollback_detected in toExtendedJson()]
- [Source: ConfigManager.cpp line 469 — dumpSettingsJson() for settings export]
- [Source: dashboard.js — Logo upload zone pattern for reuse]

### Dependencies

**This story depends on:**
- fn-1.1 (Partition Table) — COMPLETE (partition size 0x180000 = 1.5MB used for file validation)
- fn-1.3 (OTA Upload Endpoint) — COMPLETE (POST /api/ota/upload server-side handler)
- fn-1.4 (OTA Self-Check) — COMPLETE (firmware_version and rollback_detected in /api/status)

**Stories that depend on this:**
- fn-1.7: Settings Import in Setup Wizard — uses the exported JSON format from `/api/settings/export`

**Cross-reference (same epic, not a dependency):**
- fn-1.5: Settings Export Endpoint — marked "done" in sprint status, but `GET /api/settings/export` is NOT yet implemented in WebPortal.cpp. This story (fn-1.6) implements that endpoint as Task 4. The `ConfigManager::dumpSettingsJson()` data layer method exists from fn-1.2.

### Previous Story Intelligence

**From fn-1.1:** Binary size 1.15MB (76.8%). Partition size 0x180000 (1,572,864 bytes) — this is the file size limit for client-side validation. `FW_VERSION` build flag established.

**From fn-1.2:** `ConfigManager::dumpSettingsJson()` method exists and dumps all 27+ config keys as flat JSON. NVS namespace is `"flightwall"`. SUBSYSTEM_COUNT = 8.

**From fn-1.3:** OTA upload endpoint complete. Key patterns:
- `g_otaInProgress` flag prevents concurrent uploads (returns 409)
- `clearOTAUpload()` resets `g_otaInProgress = false` on completion/error
- After `Update.end(true)`, `state->started = false` to prevent cleanup from aborting
- 500ms delay before `ESP.restart()` allows response transmission
- Error codes: INVALID_FIRMWARE, NO_OTA_PARTITION, BEGIN_FAILED, WRITE_FAILED, VERIFY_FAILED, OTA_BUSY

**From fn-1.4:** `rollback_detected` persists across reboots until next successful OTA. `firmware_version` comes from `FW_VERSION` build flag. Both are in `/api/status` response under `data` object.

**Antipatterns from previous stories to avoid:**
- Don't hardcode version strings — always use the API response value
- Don't assume the device IP stays the same after reboot (DHCP reassignment possible)
- Ensure all error paths reset the upload UI to initial state (learned from fn-1.3 cleanup patterns)
- Use named String variables for `SystemStatus::set()` calls — avoid `.c_str()` on temporaries

### Existing Logo Upload Zone Pattern (Reuse Reference)

The existing logo upload zone in `dashboard.html` and `dashboard.js` provides the exact pattern to follow:

**HTML (lines 218-224 of dashboard.html):**
```html
<div class="logo-upload-zone" id="logo-upload-zone">
  <p class="upload-prompt">Drag & drop .bin logo files here or</p>
  <label class="upload-label" for="logo-file-input">Choose files</label>
  <input type="file" id="logo-file-input" accept=".bin" multiple hidden>
  <p class="upload-hint">32×32 RGB565, 2048 bytes each</p>
</div>
```

**CSS (style.css) — `.logo-upload-zone` styles are already defined and can be reused/adapted for `.ota-upload-zone`.**

**JS patterns in dashboard.js:**
- Drag events: `dragenter`, `dragover` (prevent default), `dragleave`, `drop`
- File validation before upload
- Upload via fetch/FormData
- Error handling with `FW.showToast()`

The OTA upload zone differs from logos in:
1. Single file only (no `multiple` attribute)
2. Uses XMLHttpRequest instead of fetch (for progress events)
3. Has a progress bar phase
4. Has a reboot countdown phase
5. Binary validation (magic byte check via FileReader)

### Testing Strategy

**Build verification:**
```bash
cd firmware && ~/.platformio/penv/bin/pio run
# Expect: SUCCESS, binary < 1.5MB
```

**Gzip verification:**
```bash
cd firmware
gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz
gzip -9 -c data-src/style.css > data/style.css.gz
ls -la data/dashboard.html.gz data/dashboard.js.gz data/style.css.gz
# Verify files exist and are reasonable size
```

**Manual test cases:**

1. **Dashboard load — version display:**
   - Load dashboard in browser
   - Expect: Firmware card visible with version (e.g., "v1.0.0")
   - Expect: Card is expanded, not collapsed

2. **Upload zone — drag and drop:**
   - Drag a .bin file over the upload zone
   - Expect: Border becomes solid, background lightens
   - Drop file
   - Expect: If valid, filename shown with "Upload Firmware" button
   - Expect: If invalid, error toast and zone resets

3. **Upload zone — click to select:**
   - Click/tap the upload zone
   - Expect: File picker opens

4. **Upload zone — keyboard:**
   - Tab to upload zone, press Enter or Space
   - Expect: File picker opens

5. **Client-side validation — oversized file:**
   - Select a file > 1.5MB
   - Expect: Toast "File too large — maximum 1.5MB for OTA partition"

6. **Client-side validation — wrong magic byte:**
   - Select a non-firmware .bin file (e.g., a logo file)
   - Expect: Toast "Not a valid ESP32 firmware image"

7. **Successful upload flow:**
   - Select valid firmware file, tap "Upload Firmware"
   - Expect: Progress bar appears, fills 0-100%
   - Expect: "Rebooting in 3... 2... 1..." countdown
   - Expect: Polling starts, eventually shows "Updated to v[version]"

8. **Upload error handling:**
   - Upload while another upload is in progress (if possible)
   - Expect: Error toast with server error message

9. **Rollback banner:**
   - If `rollback_detected` is true, expect persistent amber banner
   - Click dismiss → banner disappears
   - Refresh page → banner stays dismissed

10. **Download Settings:**
    - Click "Download Settings" in System card
    - Expect: JSON file downloads with all config keys

11. **Responsive — 320px viewport:**
    - Resize browser to 320px width
    - Expect: All Firmware card elements visible and usable

### Risk Mitigation

1. **Dashboard.js file size:** At 87KB, this is already the largest web asset. Adding ~200 lines of OTA UI logic is proportional (~3% increase). Monitor gzip output stays reasonable.

2. **XHR vs Fetch consistency:** The rest of the dashboard uses `FW.get()`/`FW.post()` from common.js (which use `fetch`). The OTA upload is the only place using raw `XMLHttpRequest`. Document this inconsistency clearly in a comment: `// XHR required for upload progress events — fetch() does not support upload progress`

3. **IP change after reboot:** If DHCP assigns a new IP, polling will fail after 60s. The timeout message should suggest checking the router's device list or using mDNS (`flightwall.local`).

4. **NVS write during OTA:** The `ota_rb_ack` NVS write in the ack-rollback handler is small and fast. The NVS write clearing the flag on successful OTA completion happens after `Update.end(true)` but before `ESP.restart()`, when no streaming is active — no heap conflict.

5. **Settings export without NTP:** If NTP hasn't synced, `time()` returns near-zero. The export includes an `exported_at` field — use an uptime-based fallback string rather than a misleading timestamp.

## Dev Agent Record

### Agent Model Used

Claude Opus 4 (Story Creation), Claude Opus 4.6 (Implementation)

### Debug Log References

N/A

### Completion Notes List

**2026-04-12: Implementation completed**
- All 10 tasks and subtasks implemented and verified
- Firmware card HTML added between Logos and System cards with full accessibility attributes (role, tabindex, aria-label, aria-live)
- OTA upload zone with drag-and-drop, click, and keyboard (Enter/Space) triggers
- Client-side validation: file size ≤1.5MB check, ESP32 magic byte (0xE9) check via FileReader
- XHR-based upload with real-time progress bar (not fetch — XHR needed for upload.onprogress)
- 3-second reboot countdown + polling (3s intervals, 60s timeout with amber warning)
- Rollback banner with NVS-backed persistence (ota_rb_ack key), dismiss via POST /api/ota/ack-rollback
- ota_rb_ack cleared only on successful OTA completion (not at upload start)
- GET /api/settings/export endpoint with Content-Disposition header, metadata fields, NTP-aware timestamp
- Download Settings button in System card (above danger zone)
- ~35 lines CSS added for OTA components, banner-warning, btn-secondary
- Build succeeds: 1.17MB binary (78.1%), 38KB total gzipped web assets
- No regressions — all existing code paths preserved

**2026-04-12: Ultimate context engine analysis completed**
- Comprehensive analysis of epic-fn-1.md extracted all 12 acceptance criteria with BDD format
- Full dashboard.html structure analyzed (258 lines, 9 cards) — Firmware card insertion point identified between Logos and System
- Full dashboard.js analyzed (87KB) — existing patterns for upload zones, settings loading, toast notifications, and mode picker documented
- Full style.css analyzed (20KB) — CSS custom properties and component classes documented for reuse
- WebPortal.cpp analyzed (1065 lines) — existing OTA upload handler (fn-1.3) fully mapped, new endpoint insertion points identified
- SystemStatus.cpp analyzed — firmware_version and rollback_detected already in toExtendedJson() (fn-1.4)
- ConfigManager.cpp analyzed — dumpSettingsJson() method exists for settings export data layer
- common.js analyzed — FW.get/post/del/showToast API documented
- All 4 prerequisite stories (fn-1.1 through fn-1.4) completion notes reviewed for binary size, patterns, and lessons learned
- fn-1.5 discrepancy noted: marked "done" in sprint-status.yaml but GET /api/settings/export endpoint NOT implemented in WebPortal.cpp; this story includes that implementation
- 10 tasks created with explicit code references, patterns, and implementation guidance
- XMLHttpRequest usage justified for upload progress events (fetch limitation)
- Rollback banner NVS persistence pattern designed (ota_rb_ack key)
- Client-side validation patterns provided with exact byte values and FileReader approach
- Antipatterns from fn-1.3/fn-1.4 incorporated (cleanup patterns, temporary String avoidance)

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Validation synthesis: removed conflicting localStorage reference from Task 5; corrected ota_rb_ack reset timing from upload-start to successful-completion in Task 5, Dev Notes, Project Structure, and Risk Mitigation | BMad |
| 2026-04-12 | All 10 tasks implemented: Firmware card HTML/CSS/JS, OTA upload UI with progress, rollback banner, settings export endpoint, ack-rollback endpoint. Build verified at 1.17MB (78.1%). | Claude Opus 4.6 |
| 2026-04-12 | Code review synthesis fixes: cancel button for file-info state, polling race condition guard, resetOtaUploadState color/text reset, zero-byte file check, dragleave child-element fix, dismiss-btn focus style, NVS write error checking. Build re-verified at 1.17MB (78.1%). | AI Synthesis |
| 2026-04-12 | Second code review synthesis fixes: WCAG AA contrast fix for progress text (text-shadow), upload button disabled during upload, cache-busting for polling requests, --warning CSS variable added. Build re-verified at 1.22MB (77.7%). | AI Synthesis |
| 2026-04-12 | Third code review synthesis fixes: drag-over border-style solid (AC #3), touch target min-height for OTA buttons (AC #11), NVS begin() check in OTA success path, loadFirmwareStatus error toast, XHR timeout (120s), polling converted from setInterval to recursive setTimeout. Build re-verified at 1.17MB (78.2%). | AI Synthesis |

### File List

- `firmware/data-src/dashboard.html` (MODIFIED) — Added Firmware card HTML between Logos and System, System card id="system-card", Download Settings button, Cancel button in ota-file-info
- `firmware/data-src/dashboard.js` (MODIFIED) — Added loadFirmwareStatus(), OTA upload with XHR progress, client-side validation, reboot countdown/polling, rollback banner, settings export click handler; synthesis fixes: polling done-guard, zero-byte check, dragleave relatedTarget, resetOtaUploadState color/text reset, cancel button handler
- `firmware/data-src/style.css` (MODIFIED) — Added ~35 lines OTA component styles (.ota-upload-zone, .ota-progress, .banner-warning, .btn-secondary, etc.); synthesis fixes: dismiss-btn :focus-visible, .btn-ota-cancel override
- `firmware/adapters/WebPortal.cpp` (MODIFIED) — Added POST /api/ota/ack-rollback, GET /api/settings/export endpoints; added rollback_acknowledged to /api/status; clear ota_rb_ack on successful OTA completion; added #include <Preferences.h>; synthesis fix: NVS write error handling in ack-rollback handler
- `firmware/data/dashboard.html.gz` (REGENERATED) — 3,313 bytes
- `firmware/data/dashboard.js.gz` (REGENERATED) — 22,206 bytes (updated, Pass 3)
- `firmware/data/style.css.gz` (REGENERATED) — 4,722 bytes (updated, Pass 3)

## Senior Developer Review (AI)

### Review: 2026-04-12 (Pass 1)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** A=7.1 (REJECT) / B=2.6 (APPROVED) / C=13.1 (MAJOR REWORK) → weighted: ~7.6 → **Changes Requested**
- **Issues Found:** 7 verified (3 critical, 2 high, 2 medium/low)
- **Issues Fixed:** 7 (all verified issues addressed in this synthesis pass)
- **Action Items Created:** 0

#### Review Follow-ups (AI) — Pass 1
_All verified issues were fixed in this synthesis pass. No open action items remain._

### Review: 2026-04-12 (Pass 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** A=6.9 (MAJOR REWORK) / B=7.3 (REJECT) / C=7.9 (MAJOR REWORK) → weighted: ~7.4 → **Changes Requested**
- **Issues Found:** 4 verified (1 high, 1 medium, 2 low); 14 dismissed as false positives
- **Issues Fixed:** 4 (all verified issues addressed in this synthesis pass)
- **Action Items Created:** 0

#### Review Follow-ups (AI) — Pass 2
_All verified issues were fixed in this synthesis pass. No open action items remain._

### Review: 2026-04-12 (Pass 3)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** A=5.3 (MAJOR REWORK) / B=3.5 (MAJOR REWORK) / C=0.0 (APPROVED) → weighted: ~3.0 → **Changes Requested**
- **Issues Found:** 6 verified (2 high, 4 medium/low); 10 dismissed as false positives
- **Issues Fixed:** 6 (all verified issues addressed in this synthesis pass)
- **Action Items Created:** 0

#### Review Follow-ups (AI) — Pass 3
_All verified issues were fixed in this synthesis pass. No open action items remain._


]]></file>
<file id="ff10d1c0" path="_bmad-output/implementation-artifacts/stories/fn-1-7-settings-import-in-setup-wizard.md" label="DOCUMENTATION"><![CDATA[



# Story fn-1.7: Settings Import in Setup Wizard

Status: ready-for-dev

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a **user**,
I want **to import a previously exported settings file in the setup wizard**,
So that **I can quickly reconfigure my device after a partition migration without retyping API keys and coordinates**.

## Acceptance Criteria

1. **Given** the setup wizard loads **When** an import option is available **Then** the user can select a `.json` file via a file upload zone (reusing `.logo-upload-zone` CSS pattern) **And** the import zone appears at the top of Step 1 before the WiFi form fields **And** the import zone has `role="button"`, `tabindex="0"`, and descriptive `aria-label`

2. **Given** a valid `flightwall-settings.json` file is selected **When** the browser reads the file via FileReader API **Then** the JSON is parsed client-side **And** recognized config keys pre-fill their corresponding wizard form fields across all steps (WiFi, API keys, location, hardware) **And** the user can navigate forward through pre-filled steps, reviewing each before confirming **And** a success toast shows the count of imported keys (e.g., "Imported 12 settings")

3. **Given** the imported JSON contains unrecognized keys (e.g., `flightwall_settings_version`, `exported_at`, or future keys) **When** processing the import **Then** unrecognized keys are silently ignored without error **And** all recognized keys are still pre-filled correctly

4. **Given** the imported file is not valid JSON **When** parsing fails **Then** an error toast shows "Could not read settings file — invalid format" **And** the wizard continues without pre-fill (manual entry still available) **And** the import zone resets to its initial state

5. **Given** the imported file is valid JSON but contains no recognized config keys **When** processing completes **Then** a warning toast shows "No recognized settings found in file" **And** all form fields remain at their defaults

6. **Given** settings are imported and the user completes the wizard **When** "Save & Connect" is tapped **Then** settings are applied via the normal `POST /api/settings` path (no new server-side import endpoint) **And** any imported keys that are NOT part of the wizard's 12 core fields (e.g., brightness, timing, calibration, schedule settings) are included in the POST payload alongside the wizard fields

7. **Given** the import zone is visible **When** the user opts not to import **Then** the wizard functions identically to its current behavior — WiFi scan runs, steps proceed normally, no import-related UI interferes

8. **Given** a settings file is imported **When** the user navigates to a pre-filled step **Then** the pre-filled fields are visually indistinguishable from user-typed values (no special styling needed — they're just pre-populated form values)

9. **Given** updated web assets (wizard.html, wizard.js, style.css) **When** gzipped and placed in `firmware/data/` **Then** the gzipped copies replace existing ones for LittleFS upload

## Tasks / Subtasks

- [ ] **Task 1: Add import zone HTML to wizard.html** (AC: #1, #7)
  - [ ] Add a settings import section at the top of Step 1 (`#step-1`), BEFORE the `#scan-status` div
  - [ ] HTML structure:
    ```html
    <div class="settings-import-zone" id="settings-import-zone" role="button" tabindex="0" aria-label="Select settings backup file to import">
      <p class="upload-prompt">Have a backup? <strong>Import settings</strong></p>
      <p class="upload-hint">Select a flightwall-settings.json file</p>
      <input type="file" id="import-file-input" accept=".json" hidden>
    </div>
    ```
  - [ ] The import zone should be compact — 2 lines of text max, dashed border, same visual pattern as `.logo-upload-zone` but more minimal
  - [ ] Add a hidden success indicator `<p id="import-status" class="import-status" style="display:none"></p>` below the import zone to show "12 settings imported" after successful import

- [ ] **Task 2: Add import zone CSS to style.css** (AC: #1)
  - [ ] `.settings-import-zone` — Reuse pattern from `.logo-upload-zone`: dashed border `2px dashed var(--border)`, padding `12px`, border-radius `var(--radius)`, cursor pointer, text-align center, transition for border-color
  - [ ] `.settings-import-zone.drag-over` — Solid border `var(--primary)`, background `rgba(88,166,255,0.08)`
  - [ ] `.import-status` — font-size `0.8125rem`, color `var(--success)`, text-align center, margin-top `8px`
  - [ ] Target: ~15 lines of CSS, minimal footprint
  - [ ] All interactive elements meet 44x44px minimum touch targets
  - [ ] Works at 320px minimum viewport width

- [ ] **Task 3: Implement settings import logic in wizard.js** (AC: #2, #3, #4, #5, #6, #7, #8)
  - [ ] Add DOM references for import elements: `settings-import-zone`, `import-file-input`, `import-status`
  - [ ] Add `importedExtras` object (initially empty `{}`) to hold non-wizard keys from import
  - [ ] Define `WIZARD_KEYS` constant: the 12 keys that map to wizard form fields:
    ```javascript
    var WIZARD_KEYS = [
      'wifi_ssid', 'wifi_password',
      'os_client_id', 'os_client_sec', 'aeroapi_key',
      'center_lat', 'center_lon', 'radius_km',
      'tiles_x', 'tiles_y', 'tile_pixels', 'display_pin'
    ];
    ```
  - [ ] Define `KNOWN_EXTRA_KEYS` constant: non-wizard config keys that should be preserved:
    ```javascript
    var KNOWN_EXTRA_KEYS = [
      'brightness', 'text_color_r', 'text_color_g', 'text_color_b',
      'origin_corner', 'scan_dir', 'zigzag',
      'zone_logo_pct', 'zone_split_pct', 'zone_layout',
      'fetch_interval', 'display_cycle',
      'timezone', 'sched_enabled', 'sched_dim_start', 'sched_dim_end', 'sched_dim_brt'
    ];
    ```
  - [ ] **Click/keyboard handler**: click on `.settings-import-zone` or Enter/Space key triggers hidden `#import-file-input` click
  - [ ] **File input change handler**: when a file is selected, read via `FileReader.readAsText()`, then call `processImportedSettings(text)`
  - [ ] **`processImportedSettings(text)` function**:
    1. Try `JSON.parse(text)` — on failure: `FW.showToast('Could not read settings file — invalid format', 'error')`, return
    2. Check parsed object is a plain object (not array/null) — on failure: same error toast, return
    3. Iterate keys — for each key in `WIZARD_KEYS`, if present in parsed object, assign `state[key] = String(parsed[key])`
    4. Iterate keys — for each key in `KNOWN_EXTRA_KEYS`, if present in parsed object, assign `importedExtras[key] = parsed[key]`
    5. Count total recognized keys (wizard + extras)
    6. If count === 0: `FW.showToast('No recognized settings found in file', 'warning')`, return
    7. If count > 0: `FW.showToast('Imported ' + count + ' settings', 'success')`
    8. Show import status indicator with count
    9. Rehydrate current step's form fields from updated state (call `showStep(currentStep)`)
  - [ ] **Modify `saveAndConnect()` payload**: merge `importedExtras` into the payload object before POSTing:
    ```javascript
    // After building the existing 12-key payload:
    var key;
    for (key in importedExtras) {
      if (importedExtras.hasOwnProperty(key)) {
        payload[key] = importedExtras[key];
      }
    }
    ```
  - [ ] **Guard against re-import**: if the user selects a new file after already importing, reset `importedExtras = {}` and re-process

- [ ] **Task 4: Add drag-and-drop support for import zone** (AC: #1)
  - [ ] `dragenter`/`dragover` on zone — add `.drag-over` class, prevent default
  - [ ] `dragleave` — remove `.drag-over` class (use `contains(e.relatedTarget)` check to avoid child-element flicker)
  - [ ] `drop` — remove `.drag-over` class, extract `e.dataTransfer.files[0]`, read via FileReader
  - [ ] Pattern matches the existing logo upload and OTA upload drag-and-drop implementations

- [ ] **Task 5: Gzip updated web assets** (AC: #9)
  - [ ] From `firmware/` directory, regenerate gzipped assets:
    ```
    gzip -9 -c data-src/wizard.html > data/wizard.html.gz
    gzip -9 -c data-src/wizard.js > data/wizard.js.gz
    gzip -9 -c data-src/style.css > data/style.css.gz
    ```
  - [ ] Verify gzipped files replaced in `firmware/data/`

- [ ] **Task 6: Build and verify** (AC: #1-#9)
  - [ ] Run `~/.platformio/penv/bin/pio run` from `firmware/` — verify clean build
  - [ ] Verify binary size remains under 1.5MB limit
  - [ ] Measure total gzipped web asset size (should remain well under 50KB budget)

## Dev Notes

### Critical Architecture Constraints

**Web Asset Build Pipeline (MANUAL)**
Per project memory: No automated gzip build script. After editing any file in `firmware/data-src/`, manually run:
```bash
cd firmware
gzip -9 -c data-src/wizard.html > data/wizard.html.gz
gzip -9 -c data-src/wizard.js > data/wizard.js.gz
gzip -9 -c data-src/style.css > data/style.css.gz
```

**This is a CLIENT-SIDE ONLY change — NO new server-side endpoints or firmware modifications are needed.** The import reads a JSON file in the browser, pre-fills the wizard state object, and the existing `POST /api/settings` path handles persistence. The only firmware files that change are the gzipped web assets.

### Settings Export Format (from fn-1.6)

The `GET /api/settings/export` endpoint (implemented in fn-1.6, Task 4) returns a JSON file with this structure:
```json
{
  "flightwall_settings_version": 1,
  "exported_at": "2026-04-12T15:30:45",
  "brightness": 5,
  "text_color_r": 255, "text_color_g": 255, "text_color_b": 255,
  "center_lat": 37.7749, "center_lon": -122.4194, "radius_km": 10.0,
  "tiles_x": 10, "tiles_y": 2, "tile_pixels": 16, "display_pin": 25,
  "origin_corner": 0, "scan_dir": 0, "zigzag": 0,
  "zone_logo_pct": 0, "zone_split_pct": 0, "zone_layout": 0,
  "fetch_interval": 600, "display_cycle": 3,
  "wifi_ssid": "MyNetwork", "wifi_password": "secret",
  "os_client_id": "abc123", "os_client_sec": "def456",
  "aeroapi_key": "ghi789",
  "timezone": "PST8PDT,M3.2.0,M11.1.0",
  "sched_enabled": 0,
  "sched_dim_start": 1380, "sched_dim_end": 420, "sched_dim_brt": 10
}
```

**Key mapping:**
- 12 keys map directly to wizard `state` fields (wifi, API, location, hardware)
- 18 keys are "extras" that the wizard doesn't display but should preserve in the POST payload
- 2 keys are metadata (`flightwall_settings_version`, `exported_at`) — silently ignored

### Wizard State Architecture

The wizard maintains an in-memory `state` object with 12 keys (wizard.js lines 9-22). Each step rehydrates form fields from `state` when entered (lines 308-339) and saves fields back to `state` when leaving (lines 342-359).

**Import strategy:** Pre-fill `state` with wizard-relevant keys from the imported JSON. Store non-wizard keys in a separate `importedExtras` object. When `saveAndConnect()` builds the POST payload (lines 463-476), merge `importedExtras` into it.

**Hydration guard (wizard.js lines 62-77):** The existing `hydrateDefaults()` function checks `if (!state.center_lat && ...)` before populating from `/api/settings`. After import, these fields will already be non-empty, so `hydrateDefaults()` will correctly skip them. **No modification to `hydrateDefaults()` is needed.**

### Import Zone Placement

The import zone goes at the TOP of Step 1 (`#step-1`), before `#scan-status`. This placement:
1. Is the first thing the user sees on wizard load
2. Allows import before any WiFi scanning happens
3. Doesn't interfere with the scan flow (scan still runs in the background)
4. Pre-fills ALL steps, not just the current one

After a successful import, the import zone can optionally collapse or show a success state, but it should NOT be removed — the user might want to re-import a different file.

### Type Coercion for State Fields

The wizard `state` object stores all values as **strings** (see lines 9-22). The import JSON has numeric values for location/hardware keys. When importing:
- Wizard-state keys must be converted to strings: `state[key] = String(parsed[key])`
- Extra keys should preserve their original types (numbers stay numbers) since they go directly into the POST payload which already handles the numeric conversion

### Existing Patterns to Reuse

**File upload zone CSS:** Reuse `.logo-upload-zone` pattern from style.css (lines ~310-320) — dashed border, drag-over state, centered text.

**Drag-and-drop:** Reuse the pattern from the OTA upload zone in dashboard.js — `dragenter`/`dragover`/`dragleave`/`drop` with `contains(e.relatedTarget)` guard for dragleave.

**Toast notifications:** Use `FW.showToast(message, severity)` from common.js — already available in wizard context.

**FileReader API:** Already used in dashboard.js for OTA magic byte validation. Here we use `readAsText()` instead of `readAsArrayBuffer()`.

### POST /api/settings Behavior with Extra Keys

The `ConfigManager::applyJson()` method (called by `POST /api/settings`) processes each key individually and returns which keys were applied. Unknown keys cause the endpoint to return a 400 error (WebPortal.cpp line 207: "Unknown or invalid settings key"). 

**Important:** The validation checks `if (result.applied.size() != settings.size())` — if ANY key is unrecognized, the entire request fails. This means:
- `importedExtras` must ONLY contain keys that ConfigManager recognizes
- The `KNOWN_EXTRA_KEYS` list must exactly match ConfigManager's accepted keys
- Metadata keys like `flightwall_settings_version` and `exported_at` must NOT be included in `importedExtras`

### What NOT to Do

- **Do NOT create a new server-side endpoint** — import is purely client-side
- **Do NOT modify `hydrateDefaults()`** — the existing guard logic handles pre-filled state correctly
- **Do NOT modify wizard step count or flow** — the import is additive UI at the top of Step 1
- **Do NOT add the import zone to dashboard.html** — import is wizard-only (the dashboard has direct settings editing)
- **Do NOT include `flightwall_settings_version` or `exported_at` in `importedExtras`** — these are metadata keys, not config keys, and will cause the POST to fail with "Unknown or invalid settings key"

### Project Structure Notes

**Files to modify:**
1. `firmware/data-src/wizard.html` — Add import zone HTML to Step 1
2. `firmware/data-src/wizard.js` — Add import logic, modify `saveAndConnect()` payload
3. `firmware/data-src/style.css` — Add import zone styles (~15 lines)
4. `firmware/data/wizard.html.gz` — Regenerated
5. `firmware/data/wizard.js.gz` — Regenerated
6. `firmware/data/style.css.gz` — Regenerated

**Files NOT to modify:**
- `firmware/adapters/WebPortal.cpp` — No new endpoints needed
- `firmware/adapters/WebPortal.h` — No changes
- `firmware/core/ConfigManager.h/.cpp` — No changes
- `firmware/data-src/dashboard.html` — Import is wizard-only
- `firmware/data-src/dashboard.js` — Import is wizard-only
- `firmware/data-src/common.js` — `FW.showToast` already available
- `firmware/src/main.cpp` — No changes

### References

- [Source: epic-fn-1.md#Story fn-1.7 — All acceptance criteria, story definition]
- [Source: architecture.md#D4 — POST /api/settings endpoint, JSON envelope pattern]
- [Source: architecture.md#F7 — GET /api/settings/export format definition]
- [Source: prd.md#FR1 — User can complete initial configuration from mobile browser]
- [Source: prd.md#FR10 — Persist all configuration to NVS]
- [Source: wizard.js lines 9-22 — Wizard state object with 12 keys]
- [Source: wizard.js lines 62-77 — hydrateDefaults() guard logic]
- [Source: wizard.js lines 342-359 — saveCurrentStepState() state persistence]
- [Source: wizard.js lines 457-501 — saveAndConnect() POST payload construction]
- [Source: wizard.js lines 290-339 — showStep() rehydration from state]
- [Source: wizard.html lines 18-33 — Step 1 HTML structure, insertion point for import zone]
- [Source: WebPortal.cpp lines 653-680 — GET /api/settings/export endpoint implementation]
- [Source: WebPortal.cpp lines 200-208 — POST /api/settings validation (applied.size != settings.size = 400)]
- [Source: ConfigManager.cpp lines 469-521 — dumpSettingsJson() exports all 30 config keys]
- [Source: style.css lines ~310-320 — .logo-upload-zone pattern for reuse]
- [Source: dashboard.js OTA upload zone — drag-and-drop pattern with relatedTarget guard]
- [Source: fn-1-6 story — Settings export endpoint implementation details, CSS patterns]

### Dependencies

**This story depends on:**
- fn-1.5 (Settings Export Endpoint) — COMPLETE (defines the JSON export format this story imports)
- fn-1.6 (Dashboard Firmware Card & OTA Upload UI) — COMPLETE (implemented `GET /api/settings/export`, established file upload UI patterns)

**No stories depend on this story** — fn-1.7 is the final story in Epic fn-1.

### Previous Story Intelligence

**From fn-1.1:** Binary size 1.15MB (76.8%). Partition size 0x180000 (1,572,864 bytes). `FW_VERSION` build flag established.

**From fn-1.2:** `ConfigManager::dumpSettingsJson()` method exports all 30 config keys as flat JSON. NVS namespace is `"flightwall"`. ConfigManager accepts 30 known keys via `applyJson()`.

**From fn-1.6:** `GET /api/settings/export` implemented with `Content-Disposition: attachment` header. Exports `flightwall_settings_version: 1` and `exported_at` metadata alongside all config keys. OTA upload zone established drag-and-drop patterns with `contains(e.relatedTarget)` dragleave guard. Build verified at 1.17MB (78.2%), 38KB total gzipped web assets.

**Antipatterns from previous stories to avoid:**
- Don't include metadata keys (`flightwall_settings_version`, `exported_at`) in the POST payload — they'll cause a 400 error from ConfigManager
- Ensure all error paths reset UI to initial state (learned from fn-1.3/fn-1.6 cleanup patterns)
- Use `contains(e.relatedTarget)` for dragleave events (learned from fn-1.6 code review)
- Don't modify files outside the stated scope — this is a client-side-only change

### Testing Strategy

**Build verification:**
```bash
cd firmware && ~/.platformio/penv/bin/pio run
# Expect: SUCCESS, binary < 1.5MB
```

**Gzip verification:**
```bash
cd firmware
gzip -9 -c data-src/wizard.html > data/wizard.html.gz
gzip -9 -c data-src/wizard.js > data/wizard.js.gz
gzip -9 -c data-src/style.css > data/style.css.gz
ls -la data/wizard.html.gz data/wizard.js.gz data/style.css.gz
# Verify files exist and are reasonable size
```

**Manual test cases:**

1. **Wizard loads — import zone visible:**
   - Open wizard (connect to FlightWall AP, navigate to 192.168.4.1)
   - Expect: Import zone visible at top of Step 1 before WiFi scan
   - Expect: WiFi scan still runs normally below the import zone

2. **Import valid settings file:**
   - Export settings from dashboard (`Download Settings` in System card)
   - Reset device to AP mode
   - Open wizard, tap import zone, select the exported .json file
   - Expect: Success toast "Imported N settings"
   - Navigate through all 5 steps
   - Expect: WiFi SSID/password, API keys, location, hardware all pre-filled
   - Tap "Save & Connect"
   - Expect: Settings applied and device reboots

3. **Import file with extra keys only (no wizard keys):**
   - Create a JSON file with only `{"brightness": 50, "fetch_interval": 120}`
   - Import it
   - Expect: Success toast "Imported 2 settings"
   - Wizard fields remain at defaults (extras don't appear in wizard steps)
   - Complete wizard with manual entry
   - Expect: POST payload includes brightness=50 and fetch_interval=120 alongside wizard values

4. **Import invalid JSON:**
   - Select a non-JSON file (e.g., a .bin logo file, or a text file with invalid JSON)
   - Expect: Error toast "Could not read settings file — invalid format"
   - Wizard continues normally

5. **Import JSON with no recognized keys:**
   - Create `{"foo": "bar", "baz": 123}` file
   - Import it
   - Expect: Warning toast "No recognized settings found in file"

6. **Import file with metadata keys:**
   - Create `{"flightwall_settings_version": 1, "exported_at": "2026-01-01", "brightness": 50}`
   - Import it
   - Expect: Success toast "Imported 1 settings" (only brightness counted)
   - Expect: `flightwall_settings_version` and `exported_at` NOT in POST payload

7. **Skip import — wizard works normally:**
   - Don't tap the import zone at all
   - Navigate through all steps manually
   - Expect: Wizard behaves identically to pre-import behavior

8. **Import zone — keyboard access:**
   - Tab to import zone, press Enter or Space
   - Expect: File picker opens

9. **Import zone — drag and drop:**
   - Drag a .json file over the import zone
   - Expect: Border becomes solid, background lightens
   - Drop file
   - Expect: File is processed

10. **Responsive — 320px viewport:**
    - Resize browser to 320px width
    - Expect: Import zone visible and functional, doesn't overflow

### Risk Mitigation

1. **POST /api/settings key validation:** The server rejects the entire request if any key is unknown. The `KNOWN_EXTRA_KEYS` list must be kept in sync with ConfigManager's accepted keys. If a future firmware version adds new config keys but the user imports an old settings file, that's fine — missing keys just aren't sent. If a user imports a settings file from a NEWER firmware version with unknown keys, the extras filtering via `KNOWN_EXTRA_KEYS` ensures only recognized keys are sent.

2. **Wizard.js file size:** Currently ~14KB unminified (~3.5KB gzipped). Adding ~80-100 lines of import logic is proportional (~7% increase). Well within the gzip budget.

3. **WiFi password in settings export:** The exported JSON includes `wifi_password` in plaintext. This is by design (per architecture.md Security Model: "No authentication on web config UI, single-user trusted network"). The import simply reads it back. No additional security concern beyond what the export already accepts.

4. **Type safety:** The wizard state stores strings; the export has numbers for location/hardware. The `String()` conversion in import handles this safely. The POST payload in `saveAndConnect()` already converts state strings to `Number()` (wizard.js lines 469-476), so the round-trip is: export(number) → import(String) → state(string) → payload(Number) → POST.

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6 (Story Creation)

### Debug Log References

N/A

### Completion Notes List

**2026-04-12: Ultimate context engine analysis completed**
- Comprehensive analysis of epic-fn-1.md extracted all 6 acceptance criteria with BDD format, expanded to 9 ACs with additional edge cases
- Full wizard.html structure analyzed (103 lines, 5 steps) — import zone insertion point identified at top of Step 1 before scan-status
- Full wizard.js analyzed (558 lines) — state management, hydration, validation, saveAndConnect payload all documented
- Full style.css analyzed — `.logo-upload-zone` CSS pattern identified for reuse
- WebPortal.cpp POST /api/settings validation analyzed — critical finding: unknown keys cause entire request to fail (line 207)
- ConfigManager.cpp dumpSettingsJson() analyzed — all 30 exportable config keys mapped to wizard-keys (12) and extra-keys (18)
- Settings export format fully documented from fn-1.6 implementation
- 6 tasks created with explicit code patterns, insertion points, and key-mapping logic
- POST payload key-filtering design prevents metadata keys from causing server-side 400 errors
- hydrateDefaults() guard logic analyzed — no modification needed (import pre-fills state, guard skips hydration correctly)
- All dependency stories (fn-1.5, fn-1.6) verified complete with relevant implementation details extracted
- Antipatterns from fn-1.6 code reviews incorporated (dragleave relatedTarget, UI reset on error)

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |

### File List

_(To be populated during implementation)_


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="Architecture for technical requirements verification" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="description">Quality competition validator - systematically review and improve story context created by create-story workflow</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-1</var>
<var name="epics_file" description="Enhanced epics+stories file for story verification" load_strategy="SELECTIVE_LOAD" sharded="true" token_approx="55">_bmad-output/planning-artifacts/epics/index.md</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="model">validator</var>
<var name="name">validate-story</var>
<var name="output_folder">_bmad-output/implementation-artifacts</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="prd_file" description="PRD for requirements verification" load_strategy="SELECTIVE_LOAD" token_approx="3576">_bmad-output/planning-artifacts/prd-delight-validation-report.md</var>
<var name="project_context">{project-root}/docs/project-context.md</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_dir">_bmad-output/implementation-artifacts/stories</var>
<var name="story_file" file_id="ff10d1c0">embedded in prompt, file id: ff10d1c0</var>
<var name="story_id">fn-1.7</var>
<var name="story_key">fn-1-7-settings-import-in-setup-wizard</var>
<var name="story_num">7</var>
<var name="story_title">settings-import-in-setup-wizard</var>
<var name="timestamp">20260412_211928</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="ux_file" description="UX design for user experience verification" load_strategy="SELECTIVE_LOAD" token_approx="30146">_bmad-output/planning-artifacts/ux-design-specification-delight.md</var>
<var name="validation_focus">story_quality</var>
</variables>
<instructions><workflow>
  <critical>SCOPE LIMITATION: You are a READ-ONLY VALIDATOR. Output your validation report to stdout ONLY. Do NOT create files, do NOT modify files, do NOT use Write/Edit/Bash tools. Your stdout output will be captured and saved by the orchestration system.</critical>
  <critical>All configuration and context is available in the VARIABLES section below. Use these resolved values directly.</critical>
  <critical>Communicate all responses in English and generate all documents in English</critical>

  <critical>🔥 CRITICAL MISSION: You are an independent quality validator in a FRESH CONTEXT competing against the original create-story LLM!</critical>
  <critical>Your purpose is to thoroughly review a story file and systematically identify any mistakes, omissions, or disasters that the original LLM missed</critical>
  <critical>🚨 COMMON LLM MISTAKES TO PREVENT: reinventing wheels, wrong libraries, wrong file locations, breaking regressions, ignoring UX, vague implementations, lying about completion, not learning from past work</critical>
  <critical>🔬 UTILIZE SUBPROCESSES AND SUBAGENTS: Use research subagents or parallel processing if available to thoroughly analyze different artifacts simultaneously</critical>

  <step n="1" goal="Story Quality Gate - INVEST validation">
    <critical>🎯 RUTHLESS STORY VALIDATION: Check story quality with surgical precision!</critical>
    <critical>This assessment determines if the story is fundamentally sound before deeper analysis</critical>

    <substep n="1a" title="INVEST Criteria Validation">
      <action>Evaluate each INVEST criterion with severity score (1-10, where 10 is critical violation):</action>

      <action>**I - Independent:** Check if story can be developed independently
        - Does it have hidden dependencies on other stories?
        - Can it be implemented without waiting for other work?
        - Are there circular dependencies?
        Score severity of any violations found
      </action>

      <action>**N - Negotiable:** Check if story allows implementation flexibility
        - Is it overly prescriptive about HOW vs WHAT?
        - Does it leave room for technical decisions?
        - Are requirements stated as outcomes, not solutions?
        Score severity of any violations found
      </action>

      <action>**V - Valuable:** Check if story delivers clear business value
        - Is the benefit clearly stated and meaningful?
        - Does it contribute to epic/product goals?
        - Would stakeholder recognize the value?
        Score severity of any violations found
      </action>

      <action>**E - Estimable:** Check if story can be accurately estimated
        - Are requirements clear enough to estimate?
        - Is scope well-defined without ambiguity?
        - Are there unknown technical risks that prevent estimation?
        Score severity of any violations found
      </action>

      <action>**S - Small:** Check if story is appropriately sized
        - Can it be completed in a single sprint?
        - Is it too large and should be split?
        - Is it too small to be meaningful?
        Score severity of any violations found
      </action>

      <action>**T - Testable:** Check if story has testable acceptance criteria
        - Are acceptance criteria specific and measurable?
        - Can each criterion be verified objectively?
        - Are edge cases and error scenarios covered?
        Score severity of any violations found
      </action>

      <action>Store INVEST results: {{invest_results}} with individual scores</action>
    </substep>

    <substep n="1b" title="Acceptance Criteria Deep Analysis">
      <action>Hunt for acceptance criteria issues:
        - Ambiguous criteria: Vague language like "should work well", "fast", "user-friendly"
        - Untestable criteria: Cannot be objectively verified
        - Missing criteria: Expected behaviors not covered
        - Conflicting criteria: Criteria that contradict each other
        - Incomplete scenarios: Missing edge cases, error handling, boundary conditions
      </action>
      <action>Document each issue with specific quote and recommendation</action>
      <action>Store as {{acceptance_criteria_issues}}</action>
    </substep>

    <substep n="1c" title="Hidden Dependencies Discovery">
      <action>Uncover hidden dependencies and future sprint-killers:
        - Undocumented technical dependencies (libraries, services, APIs)
        - Cross-team dependencies not mentioned
        - Infrastructure dependencies (databases, queues, caches)
        - Data dependencies (migrations, seeds, external data)
        - Sequential dependencies on other stories
        - External blockers (third-party services, approvals)
      </action>
      <action>Document each hidden dependency with impact assessment</action>
      <action>Store as {{hidden_dependencies}}</action>
    </substep>

    <substep n="1d" title="Estimation Reality-Check">
      <action>Reality-check the story estimate against complexity:
        - Compare stated/implied effort vs actual scope
        - Check for underestimated technical complexity
        - Identify scope creep risks
        - Assess if unknown unknowns are accounted for
        - Compare with similar stories from previous work
      </action>
      <action>Provide estimation assessment: realistic / underestimated / overestimated / unestimable</action>
      <action>Store as {{estimation_assessment}}</action>
    </substep>

    <substep n="1e" title="Technical Alignment Verification">
      <action>Verify alignment with embedded context architecture patterns:
        - Does story follow established architectural patterns?
        - Are correct technologies/frameworks specified?
        - Does it respect defined boundaries and layers?
        - Are naming conventions and file structures aligned?
        - Does it integrate correctly with existing components?
      </action>
      <action>Document any misalignments or conflicts</action>
      <action>Store as {{technical_alignment_issues}}</action>
    </substep>

    <o>🎯 **Story Quality Gate Results:**
      - INVEST Violations: {{invest_violation_count}}
      - Acceptance Criteria Issues: {{ac_issues_count}}
      - Hidden Dependencies: {{hidden_deps_count}}
      - Estimation: {{estimation_assessment}}
      - Technical Alignment: {{alignment_status}}

      ℹ️ Continuing with full analysis...
    </o>
  </step>

  <step n="2" goal="Disaster prevention gap analysis">
    <critical>🚨 CRITICAL: Identify every mistake the original LLM missed that could cause DISASTERS!</critical>

    <substep n="2a" title="Reinvention Prevention Gaps">
      <action>Analyze for wheel reinvention risks:
        - Areas where developer might create duplicate functionality
        - Code reuse opportunities not identified
        - Existing solutions not mentioned that developer should extend
        - Patterns from previous stories not referenced
      </action>
      <action>Document each reinvention risk found</action>
    </substep>

    <substep n="2b" title="Technical Specification Disasters">
      <action>Analyze for technical specification gaps:
        - Wrong libraries/frameworks: Missing version requirements
        - API contract violations: Missing endpoint specifications
        - Database schema conflicts: Missing requirements that could corrupt data
        - Security vulnerabilities: Missing security requirements
        - Performance disasters: Missing requirements that could cause failures
      </action>
      <action>Document each technical specification gap</action>
    </substep>

    <substep n="2c" title="File Structure Disasters">
      <action>Analyze for file structure issues:
        - Wrong file locations: Missing organization requirements
        - Coding standard violations: Missing conventions
        - Integration pattern breaks: Missing data flow requirements
        - Deployment failures: Missing environment requirements
      </action>
      <action>Document each file structure issue</action>
    </substep>

    <substep n="2d" title="Regression Disasters">
      <action>Analyze for regression risks:
        - Breaking changes: Missing requirements that could break existing functionality
        - Test failures: Missing test requirements
        - UX violations: Missing user experience requirements
        - Learning failures: Missing previous story context
      </action>
      <action>Document each regression risk</action>
    </substep>

    <substep n="2e" title="Implementation Disasters">
      <action>Analyze for implementation issues:
        - Vague implementations: Missing details that could lead to incorrect work
        - Completion lies: Missing acceptance criteria that could allow fake implementations
        - Scope creep: Missing boundaries that could cause unnecessary work
        - Quality failures: Missing quality requirements
      </action>
      <action>Document each implementation issue</action>
    </substep>
  </step>

  <step n="3" goal="LLM-Dev-Agent optimization analysis">
    <critical>CRITICAL: Optimize story context for LLM developer agent consumption</critical>

    <action>Analyze current story for LLM optimization issues:
      - Verbosity problems: Excessive detail that wastes tokens without adding value
      - Ambiguity issues: Vague instructions that could lead to multiple interpretations
      - Context overload: Too much information not directly relevant to implementation
      - Missing critical signals: Key requirements buried in verbose text
      - Poor structure: Information not organized for efficient LLM processing
    </action>

    <action>Apply LLM Optimization Principles:
      - Clarity over verbosity: Be precise and direct, eliminate fluff
      - Actionable instructions: Every sentence should guide implementation
      - Scannable structure: Clear headings, bullet points, and emphasis
      - Token efficiency: Pack maximum information into minimum text
      - Unambiguous language: Clear requirements with no room for interpretation
    </action>

    <action>Document each LLM optimization opportunity</action>
  </step>

  <step n="4" goal="Categorize and prioritize improvements">
    <action>Categorize all identified issues into:
      - critical_issues: Must fix - essential requirements, security, blocking issues
      - enhancements: Should add - helpful guidance, better specifications
      - optimizations: Nice to have - performance hints, development tips
      - llm_optimizations: Token efficiency and clarity improvements
    </action>

    <action>Count issues in each category:
      - {{critical_count}} critical issues
      - {{enhancement_count}} enhancements
      - {{optimization_count}} optimizations
      - {{llm_opt_count}} LLM optimizations
    </action>

    <action>Assign numbers to each issue for user selection</action>

    <substep n="4b" title="Calculate Evidence Score">
      <critical>🔥 CRITICAL: You MUST calculate and output the Evidence Score for synthesis!</critical>

      <action>Map each finding to Evidence Score severity:
        - **🔴 CRITICAL** (+3 points): Security vulnerabilities, data corruption risks, blocking issues, missing essential requirements
        - **🟠 IMPORTANT** (+1 point): Missing guidance, unclear specifications, integration risks
        - **🟡 MINOR** (+0.3 points): Typos, style issues, minor clarifications
      </action>

      <action>Count CLEAN PASS categories - areas with NO issues found:
        - Each clean category: -0.5 points
        - Categories to check: INVEST criteria (6), Acceptance Criteria, Dependencies, Technical Alignment, Implementation
      </action>

      <action>Calculate Evidence Score:
        {{evidence_score}} = SUM(finding_scores) + (clean_pass_count × -0.5)

        Example: 2 CRITICAL (+6) + 1 IMPORTANT (+1) + 4 CLEAN PASSES (-2) = 5.0
      </action>

      <action>Determine Evidence Verdict:
        - **EXCELLENT** (score ≤ -3): Many clean passes, minimal issues
        - **PASS** (score &lt; 3): Acceptable quality, minor issues only
        - **MAJOR REWORK** (3 ≤ score &lt; 7): Significant issues require attention
        - **REJECT** (score ≥ 7): Critical problems, needs complete rewrite
      </action>

      <action>Store for template output:
        - {{evidence_findings}}: List of findings with severity_icon, severity, description, source, score
        - {{clean_pass_count}}: Number of clean categories
        - {{evidence_score}}: Calculated total score
        - {{evidence_verdict}}: EXCELLENT/PASS/MAJOR REWORK/REJECT
      </action>
    </substep>
  </step>

  <step n="5" goal="Generate validation report">
    <critical>OUTPUT MARKERS REQUIRED: Your validation report MUST start with the marker &lt;!-- VALIDATION_REPORT_START --&gt; on its own line BEFORE the report header, and MUST end with the marker &lt;!-- VALIDATION_REPORT_END --&gt; on its own line AFTER the final line. The orchestrator extracts ONLY content between these markers. Any text outside the markers (thinking, commentary) will be discarded.</critical>

    <action>Use the output template as a FORMAT GUIDE, replacing all {{placeholders}} with your actual analysis</action>
    <action>Output the complete report to stdout with all sections filled in</action>
    <action>Do NOT save to any file - the orchestrator handles persistence</action>
  </step>

</workflow></instructions>
<output-template><![CDATA[

<!-- VALIDATION_REPORT_START -->

# 🎯 Story Context Validation Report

<!-- report_header -->

**Story:** {{story_key}} - {{story_title}}
**Story File:** {{story_file}}
**Validated:** {{date}}
**Validator:** Quality Competition Engine

---

<!-- executive_summary -->

## Executive Summary

### Issues Overview

| Category | Found | Applied |
|----------|-------|---------|
| 🚨 Critical Issues | {{critical_count}} | {{critical_applied}} |
| ⚡ Enhancements | {{enhancement_count}} | {{enhancements_applied}} |
| ✨ Optimizations | {{optimization_count}} | {{optimizations_applied}} |
| 🤖 LLM Optimizations | {{llm_opt_count}} | {{llm_opts_applied}} |

**Overall Assessment:** {{overall_assessment}}

---

<!-- evidence_score_summary -->

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
{{#each evidence_findings}}
| {{severity_icon}} {{severity}} | {{description}} | {{source}} | +{{score}} |
{{/each}}
{{#if clean_pass_count}}
| 🟢 CLEAN PASS | {{clean_pass_count}} |
{{/if}}

### Evidence Score: {{evidence_score}}

| Score | Verdict |
|-------|---------|
| **{{evidence_score}}** | **{{evidence_verdict}}** |

---

<!-- story_quality_gate -->

## 🎯 Ruthless Story Validation {{epic_num}}.{{story_num}}

### INVEST Criteria Assessment

| Criterion | Status | Severity | Details |
|-----------|--------|----------|---------|
| **I**ndependent | {{invest_i_status}} | {{invest_i_severity}}/10 | {{invest_i_details}} |
| **N**egotiable | {{invest_n_status}} | {{invest_n_severity}}/10 | {{invest_n_details}} |
| **V**aluable | {{invest_v_status}} | {{invest_v_severity}}/10 | {{invest_v_details}} |
| **E**stimable | {{invest_e_status}} | {{invest_e_severity}}/10 | {{invest_e_details}} |
| **S**mall | {{invest_s_status}} | {{invest_s_severity}}/10 | {{invest_s_details}} |
| **T**estable | {{invest_t_status}} | {{invest_t_severity}}/10 | {{invest_t_details}} |

### INVEST Violations

{{#each invest_violations}}
- **[{{severity}}/10] {{criterion}}:** {{description}}
{{/each}}

{{#if no_invest_violations}}
✅ No significant INVEST violations detected.
{{/if}}

### Acceptance Criteria Issues

{{#each acceptance_criteria_issues}}
- **{{issue_type}}:** {{description}}
  - *Quote:* "{{quote}}"
  - *Recommendation:* {{recommendation}}
{{/each}}

{{#if no_acceptance_criteria_issues}}
✅ Acceptance criteria are well-defined and testable.
{{/if}}

### Hidden Risks and Dependencies

{{#each hidden_dependencies}}
- **{{dependency_type}}:** {{description}}
  - *Impact:* {{impact}}
  - *Mitigation:* {{mitigation}}
{{/each}}

{{#if no_hidden_dependencies}}
✅ No hidden dependencies or blockers identified.
{{/if}}

### Estimation Reality-Check

**Assessment:** {{estimation_assessment}}

{{estimation_details}}

### Technical Alignment

**Status:** {{technical_alignment_status}}

{{#each technical_alignment_issues}}
- **{{issue_type}}:** {{description}}
  - *Architecture Reference:* {{architecture_reference}}
  - *Recommendation:* {{recommendation}}
{{/each}}

{{#if no_technical_alignment_issues}}
✅ Story aligns with architecture.md patterns.
{{/if}}

### Evidence Score: {{evidence_score}} → {{evidence_verdict}}

---

<!-- critical_issues_section -->

## 🚨 Critical Issues (Must Fix)

These are essential requirements, security concerns, or blocking issues that could cause implementation disasters.

{{#each critical_issues}}
### {{number}}. {{title}}

**Impact:** {{impact}}
**Source:** {{source_reference}}

**Problem:**
{{problem_description}}

**Recommended Fix:**
{{recommended_fix}}

{{/each}}

{{#if no_critical_issues}}
✅ No critical issues found - the original story covered essential requirements.
{{/if}}

---

<!-- enhancements_section -->

## ⚡ Enhancement Opportunities (Should Add)

Additional guidance that would significantly help the developer avoid mistakes.

{{#each enhancements}}
### {{number}}. {{title}}

**Benefit:** {{benefit}}
**Source:** {{source_reference}}

**Current Gap:**
{{gap_description}}

**Suggested Addition:**
{{suggested_addition}}

{{/each}}

{{#if no_enhancements}}
✅ No significant enhancement opportunities identified.
{{/if}}

---

<!-- optimizations_section -->

## ✨ Optimizations (Nice to Have)

Performance hints, development tips, and additional context for complex scenarios.

{{#each optimizations}}
### {{number}}. {{title}}

**Value:** {{value}}

**Suggestion:**
{{suggestion}}

{{/each}}

{{#if no_optimizations}}
✅ No additional optimizations identified.
{{/if}}

---

<!-- llm_optimizations_section -->

## 🤖 LLM Optimization Improvements

Token efficiency and clarity improvements for better dev agent processing.

{{#each llm_optimizations}}
### {{number}}. {{title}}

**Issue:** {{issue_type}}
**Token Impact:** {{token_impact}}

**Current:**
```
{{current_text}}
```

**Optimized:**
```
{{optimized_text}}
```

**Rationale:** {{rationale}}

{{/each}}

{{#if no_llm_optimizations}}
✅ Story content is well-optimized for LLM processing.
{{/if}}

---

<!-- competition_results -->

## 🏆 Competition Results

### Quality Metrics

| Metric | Score |
|--------|-------|
| Requirements Coverage | {{requirements_coverage}}% |
| Architecture Alignment | {{architecture_alignment}}% |
| Previous Story Integration | {{previous_story_integration}}% |
| LLM Optimization Score | {{llm_optimization_score}}% |
| **Overall Quality Score** | **{{overall_quality_score}}%** |

### Disaster Prevention Assessment

{{#each disaster_categories}}
- **{{category}}:** {{status}} {{details}}
{{/each}}

### Competition Outcome

{{#if validator_won}}
🏆 **Validator identified {{total_issues}} improvements** that enhance the story context.
{{/if}}

{{#if original_won}}
✅ **Original create-story produced high-quality output** with minimal gaps identified.
{{/if}}

---

**Report Generated:** {{date}}
**Validation Engine:** BMAD Method Quality Competition v1.0

<!-- VALIDATION_REPORT_END -->

]]></output-template>
</compiled-workflow>