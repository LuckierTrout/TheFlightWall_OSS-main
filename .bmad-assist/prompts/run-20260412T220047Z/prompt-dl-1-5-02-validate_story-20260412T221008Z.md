<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-1 -->
<!-- Story: 5 -->
<!-- Phase: validate-story -->
<!-- Timestamp: 20260412T221008Z -->
<compiled-workflow>
<mission><![CDATA[

Adversarial Story Validation

Target: Story dl-1.5 - 5-story-5

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
<file id="af701597" path="_bmad-output/implementation-artifacts/stories/dl-1-5-story-5.md" label="DOCUMENTATION"><![CDATA[

# Story dl-1.5: Orchestrator State Feedback in Dashboard

Status: ready-for-dev

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want the dashboard Mode Picker to display the current orchestrator state and reason (e.g., "Active: Clock (idle fallback — no flights)"),
So that I can always understand whether the wall is following my manual choice or responding automatically to flight availability.

## Context & Rationale

Stories dl-1.1 through dl-1.4 build the orchestrator firmware (ClockMode, ModeOrchestrator, idle fallback, watchdog recovery). But without dashboard feedback, the owner has no way to know **why** the wall is showing a particular mode. The UX spec's defining experience principle states: "Delegation requires transparency — the status line is the emotional contract between automation and control." This story closes the loop by surfacing orchestrator state through the API and Mode Picker UI.

**Dependencies:** This story MUST be implemented after dl-1.1 (ClockMode + DisplayMode interface), dl-1.2 (ModeOrchestrator), and dl-1.3 (manual switching via orchestrator). It assumes ds-3-1 (Display mode API endpoints) and ds-3-3 (Mode Picker UI) are already shipped.

**What this story does NOT cover:**
- Scheduled mode state (`SCHEDULED` state, `scheduled_mode`, `next_change`) — that's dl-4
- Per-mode settings panels — that's dl-5
- "Resume Schedule" button — that's dl-4
- Transition animations — that's dl-3

## Acceptance Criteria

1. **Given** the Mode Picker section is visible on the dashboard
   **When** the dashboard loads or a mode switch completes
   **Then** a status line at the top of the Mode Picker shows: "Active: {ModeName} ({reason})" where reason reflects the current orchestrator state

2. **Given** the owner manually selected Live Flight Card via Mode Picker
   **When** the status line renders
   **Then** it shows: "Active: Live Flight Card (manual)"

3. **Given** zero flights are in range and idle fallback activated Clock Mode
   **When** the status line renders
   **Then** it shows: "Active: Clock (idle fallback — no flights)"

4. **Given** the owner manually selects a mode while in IDLE_FALLBACK state
   **When** the API responds and the dashboard updates
   **Then** the status line changes to "Active: {SelectedMode} (manual)" reflecting the orchestrator's transition from IDLE_FALLBACK to MANUAL state

5. **Given** the existing `GET /api/display/modes` endpoint
   **When** this story is implemented
   **Then** the response JSON includes two new fields: `orchestrator_state` (string: "manual" | "idle_fallback") and `state_reason` (string: human-readable reason for the current mode)

6. **Given** the Mode Picker UI exists from ds-3-3
   **When** this story is implemented
   **Then** a persistent status line element is added at the top of the Mode Picker card, styled with mode name in `font-weight: 600` and reason in `font-weight: 400` within parentheses, using existing CSS custom properties

7. **Given** the dashboard fetches mode state on load
   **When** the API returns orchestrator state
   **Then** the Mode Picker also updates the active mode card's subtitle text to show the reason (e.g., "Manually selected" or "Idle fallback — no flights"), providing a secondary visual confirmation

8. **Given** a mode switch is triggered (via Mode Picker tap or automatic idle fallback)
   **When** the dashboard next fetches mode state (after the POST response or on next page load)
   **Then** both the status line and the active mode card subtitle reflect the new orchestrator state within the same render cycle (no partial updates)

## Tasks / Subtasks

- [ ] Task 1: Extend `GET /api/display/modes` response with orchestrator state (AC: #5)
  - [ ] 1.1 Add `getState()` and `getStateReason()` public methods to `ModeOrchestrator` if not already present
  - [ ] 1.2 In `WebPortal::_handleGetModes()` (or equivalent handler), read `ModeOrchestrator::getState()` and serialize as `orchestrator_state` string
  - [ ] 1.3 Generate `state_reason` string from orchestrator state: `MANUAL` → "manual", `IDLE_FALLBACK` → "idle fallback — no flights"
  - [ ] 1.4 Add both fields to the JSON response object alongside existing `active` and `modes[]` fields

- [ ] Task 2: Add status line to Mode Picker dashboard UI (AC: #1, #2, #3, #6)
  - [ ] 2.1 Add `<div class="mode-status-line">` element at the top of the Mode Picker card in `dashboard.html`
  - [ ] 2.2 Style with existing CSS tokens: use `--card-bg` for container, existing text color tokens for content
  - [ ] 2.3 Add CSS class `.mode-status-line` with `font-size: 0.9rem`, `padding: 8px 0`, `border-bottom: 1px solid var(--settings-border)`
  - [ ] 2.4 Mode name span: `font-weight: 600`. Reason span: `font-weight: 400`, in parentheses

- [ ] Task 3: Update dashboard JavaScript to render orchestrator state (AC: #1, #7, #8)
  - [ ] 3.1 In the mode fetch handler (after `GET /api/display/modes`), extract `orchestrator_state` and `state_reason` from response
  - [ ] 3.2 Update status line text: `Active: ${activeMode.displayName} (${stateReason})`
  - [ ] 3.3 Update active mode card subtitle with reason text
  - [ ] 3.4 Ensure both UI elements update in the same DOM operation (batch update) to prevent partial render

- [ ] Task 4: Update Mode Picker after POST /api/display/mode (AC: #4, #8)
  - [ ] 4.1 After successful POST response, re-fetch `GET /api/display/modes` to get updated orchestrator state
  - [ ] 4.2 Update status line and active card in the success callback
  - [ ] 4.3 Ensure the "Switching..." intermediate state clears before showing new orchestrator state

- [ ] Task 5: Gzip updated web assets (AC: all)
  - [ ] 5.1 Regenerate `data/dashboard.html.gz` from `data-src/dashboard.html`
  - [ ] 5.2 Regenerate `data/dashboard.js.gz` from `data-src/dashboard.js` (or equivalent JS file)
  - [ ] 5.3 Regenerate `data/style.css.gz` from `data-src/style.css`
  - [ ] 5.4 Verify total gzipped web assets remain well under 50KB budget

## Dev Notes

### Architecture Constraints

- **Response envelope pattern:** All API responses use `{ "ok": bool, "data": {...} }` or `{ "ok": false, "error": "message", "code": "..." }`. The new fields go inside the `data` object. [Source: architecture.md#D4]
- **ModeOrchestrator is static class:** All methods are static. Call `ModeOrchestrator::getState()` directly — no instance needed. [Source: architecture.md#DL2]
- **Rule 24 — requestSwitch() call sites:** `ModeRegistry::requestSwitch()` is called from exactly two methods: `ModeOrchestrator::tick()` and `ModeOrchestrator::onManualSwitch()`. The WebPortal handler must NOT call requestSwitch() directly — always go through the orchestrator. [Source: architecture.md#Enforcement Rules]
- **Rule 30 — cross-core atomics:** Only in `main.cpp`. ModeOrchestrator state is read/written from Core 1 only (orchestrator tick + web handler), so no atomic needed for orchestrator state getters. [Source: architecture.md#Enforcement Rules]
- **Web asset pattern:** Every `fetch()` in dashboard JS must check `json.ok` and call `showToast()` on failure. [Source: architecture.md#Enforcement Rules]
- **No background polling:** Dashboard fetches state on page load and after user actions. No `setInterval` polling for orchestrator state. [Source: UX spec#State Management Patterns]

### Orchestrator State Enum & Reason Mapping

From architecture.md#DL2, `ModeOrchestrator` has three states but only two are relevant for this story:

| OrchestratorState | API String | Reason String | When |
|---|---|---|---|
| `MANUAL` | `"manual"` | `"manual"` | Owner selected this mode |
| `IDLE_FALLBACK` | `"idle_fallback"` | `"idle fallback — no flights"` | Zero flights triggered Clock Mode |
| `SCHEDULED` | `"scheduled"` | (deferred to dl-4) | Schedule rule activated this mode |

The `SCHEDULED` state and its reason strings are NOT implemented in this story. If `getState()` returns `SCHEDULED` (shouldn't happen before dl-4), serialize as `"scheduled"` with reason `"scheduled"` as a safe fallback.

### API Response Shape

Extend the existing `GET /api/display/modes` response:

```json
{
  "ok": true,
  "data": {
    "modes": [
      { "id": "classic_card", "name": "Classic Card", "active": false },
      { "id": "live_flight", "name": "Live Flight Card", "active": false },
      { "id": "clock", "name": "Clock", "active": true }
    ],
    "active": "clock",
    "switch_state": "idle",
    "orchestrator_state": "idle_fallback",
    "state_reason": "idle fallback — no flights"
  }
}
```

### Dashboard HTML Structure

```html
<!-- Inside Mode Picker card, at the top before mode cards list -->
<div class="mode-status-line" id="modeStatusLine">
  Active: <span class="mode-status-name" id="modeStatusName">—</span>
  (<span class="mode-status-reason" id="modeStatusReason">loading</span>)
</div>
```

### CSS Additions (~15 lines)

```css
.mode-status-line {
  font-size: 0.9rem;
  padding: 8px 0;
  margin-bottom: 12px;
  border-bottom: 1px solid var(--settings-border, rgba(255,255,255,0.1));
  color: var(--text-secondary, #aaa);
}
.mode-status-name {
  font-weight: 600;
  color: var(--text-primary, #fff);
}
.mode-status-reason {
  font-weight: 400;
}
```

### JavaScript Update Pattern

```javascript
// After fetching GET /api/display/modes
function updateModeStatus(data) {
  const activeMode = data.modes.find(m => m.id === data.active);
  const statusName = document.getElementById('modeStatusName');
  const statusReason = document.getElementById('modeStatusReason');
  if (statusName && activeMode) {
    statusName.textContent = activeMode.name;
  }
  if (statusReason) {
    statusReason.textContent = data.state_reason || 'unknown';
  }
}
```

### Source Files to Touch

| File | Change |
|---|---|
| `firmware/core/ModeOrchestrator.h` | Add `getStateString()` and `getStateReason()` if not present |
| `firmware/adapters/WebPortal.cpp` | Add orchestrator state fields to mode list API response |
| `firmware/adapters/WebPortal.h` | Possibly add `#include "core/ModeOrchestrator.h"` |
| `firmware/data-src/dashboard.html` | Add status line `<div>` to Mode Picker section |
| `firmware/data-src/style.css` (or equivalent) | Add `.mode-status-line` styles (~15 lines) |
| `firmware/data-src/dashboard.js` (or equivalent) | Add `updateModeStatus()` function, call after mode fetch |
| `firmware/data/*.gz` | Regenerate gzipped assets |

### Testing Approach

**Manual verification (no automated CI):**
1. Boot device with flights in range → status shows "Active: Classic Card (manual)"
2. Wait for zero flights → status shows "Active: Clock (idle fallback — no flights)"
3. Manually switch mode during idle fallback → status shows "Active: {mode} (manual)"
4. Verify API response shape via `curl http://flightwall.local/api/display/modes | jq`
5. Verify gzipped assets load correctly (no 404s, no corruption)
6. Check total gzipped web asset size stays under 50KB

### Project Structure Notes

- Alignment with hexagonal architecture: WebPortal (adapter) reads from ModeOrchestrator (core) — dependency direction is correct (adapter depends on core, not reverse)
- New CSS follows existing custom property naming: `--settings-border` already exists from Foundation release
- Status line pattern will be reused by dl-4 (scheduling) which adds "scheduled" state and "Resume Schedule" button — design the HTML structure to accommodate future extension
- The status line is the foundation for the UX spec's "Status line as universal confirmation" pattern across all journeys

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#DL2] — ModeOrchestrator state machine (MANUAL, SCHEDULED, IDLE_FALLBACK)
- [Source: _bmad-output/planning-artifacts/architecture.md#DL6] — API endpoint additions for Delight
- [Source: _bmad-output/planning-artifacts/architecture.md#DS5] — Existing mode switch API shape
- [Source: _bmad-output/planning-artifacts/architecture.md#Enforcement Rules] — Rules 24, 30
- [Source: _bmad-output/planning-artifacts/ux-design-specification-delight.md#Key Interaction Patterns] — Status line: "Active: [Mode] ([reason])"
- [Source: _bmad-output/planning-artifacts/ux-design-specification-delight.md#Component Strategy] — Mode Picker anatomy: "Status line at top of container, always visible"
- [Source: _bmad-output/planning-artifacts/ux-design-specification-delight.md#UX Consistency Patterns] — Inline feedback: "always visible while relevant"
- [Source: _bmad-output/planning-artifacts/ux-design-specification-delight.md#Experience Principles] — "Delegation requires transparency"
- [Source: _bmad-output/planning-artifacts/epics/epic-dl-1.md] — Epic dl-1 stories dl-1.2 (ModeOrchestrator) and dl-1.3 (manual switching)
- [Source: _bmad-output/planning-artifacts/prd.md#FR9] — User can view and edit all device settings from web dashboard
- [Source: firmware/adapters/WebPortal.h] — Current WebPortal API surface
- [Source: firmware/src/main.cpp#displayTask] — Display task loop structure (lines 272-418)

## Dev Agent Record

### Agent Model Used

_To be filled by implementing agent_

### Debug Log References

_To be filled during implementation_

### Completion Notes List

_To be filled during implementation_

### File List

_To be filled during implementation_


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="Architecture for technical requirements verification" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="description">Quality competition validator - systematically review and improve story context created by create-story workflow</var>
<var name="document_output_language">English</var>
<var name="epic_num">dl-1</var>
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
<var name="story_file" file_id="af701597">embedded in prompt, file id: af701597</var>
<var name="story_id">dl-1.5</var>
<var name="story_key">dl-1-5-story-5</var>
<var name="story_num">5</var>
<var name="story_title">5-story-5</var>
<var name="timestamp">20260412_181008</var>
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