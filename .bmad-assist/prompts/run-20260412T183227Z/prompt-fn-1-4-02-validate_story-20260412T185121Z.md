<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 4 -->
<!-- Phase: validate-story -->
<!-- Timestamp: 20260412T185121Z -->
<compiled-workflow>
<mission><![CDATA[

Adversarial Story Validation

Target: Story fn-1.4 - ota-self-check-and-rollback-detection

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

# Architecture Decision Document — The FlightWall OSS Project

## Project Context Analysis

### Requirements Overview

**Functional Requirements:** 48 FRs across 9 groups (Device Setup, Configuration Management, Display Calibration, Flight Data Display, Airline Logo Display, Responsive Layout, Logo Management, System Operations).

**Non-Functional Requirements:** Hot-reload latency <1s, page load <3s, 30+ days uptime, WiFi recovery <60s, RAM ceiling <280KB, flash budget 4MB total with 500KB headroom, concurrent operations across dual cores.

**UX Specification:** Two HTML pages (wizard + dashboard), single settings API endpoint, gzipped static assets, Leaflet lazy-load, mDNS, WiFi scan, toast notifications with hot-reload, triple feedback (browser/LEDs), RGB565 logo preview.

### Scale & Complexity

Medium IoT/Embedded with web UI. 6 new architectural components (ConfigManager, WebPortal, WiFiManager, LogoManager, LayoutEngine, DisplayTask). 5 existing components preserved (FlightDataFetcher, OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, NeoMatrixDisplay).

### Technical Constraints

ESP32 single WiFi radio (AP_STA mode required), 4MB flash total (2MB app + 2MB LittleFS), 520KB SRAM (~280KB usable), FastLED disables interrupts (display on Core 0, WiFi/web on Core 1), ESPAsyncWebServer 2-3 concurrent clients max, ArduinoJson streaming/filter API for heap pressure, NVS ~100K write cycles per key.

## Technology Stack

**Existing (Locked):** C++ Arduino, ESP32, PlatformIO, FastLED ^3.6.0, Adafruit GFX ^1.11.9, FastLED NeoMatrix ^1.2, ArduinoJson ^7.4.2, WiFi + HTTPClient (ESP32 core), TLS (WiFiClientSecure).

**New Dependencies:** ESPAsyncWebServer (mathieucarbou fork ^3.6.0 — actively maintained, fixes memory leaks), AsyncTCP (mathieucarbou fork — fixes race conditions), LittleFS (ESP32 core, replaces SPIFFS), ESPmDNS (ESP32 core), Leaflet.js ~1.9 (lazy-loaded client-side).

**PlatformIO additions:**
```ini
lib_deps =
    mathieucarbou/ESPAsyncWebServer @ ^3.6.0
    fastled/FastLED @ ^3.6.0
    adafruit/Adafruit GFX Library @ ^1.11.9
    marcmerlin/FastLED NeoMatrix @ ^1.2
    bblanchon/ArduinoJson @ ^7.4.2
board_build.filesystem = littlefs
board_build.partitions = custom_partitions.csv
```

**Flash Partition Layout:** Custom `firmware/custom_partitions.csv` — 20KB NVS + 8KB OTA + 2MB app0 + ~1.9MB spiffs (LittleFS).

## Core Architectural Decisions

### Decision 1: ConfigManager — Singleton with Category Struct Getters

Centralized singleton, initialized first. Config values grouped into category structs. Config value chain: compile-time default → NVS stored value → RAM cache (struct fields).

**5 category structs:** DisplayConfig (brightness, text colors), LocationConfig (center, radius), HardwareConfig (tiles, pins, wiring), TimingConfig (fetch/display intervals), NetworkConfig (WiFi, API keys).

**NVS Write Debouncing:** Hot-reload keys update RAM instantly; NVS write debounced to 2-second quiet period. Reboot-required keys persist immediately.

**Reboot-required keys:** `wifi_ssid`, `wifi_password`, `opensky_client_id`, `opensky_client_secret`, `aeroapi_key`, `display_pin`.

**Hot-reload keys:** All display, location, hardware timing settings.

### Decision 2: Inter-Task Communication — Hybrid

**Config changes:** Atomic flag signals display task to re-read from ConfigManager.

**Flight data:** FreeRTOS queue with overwrite semantics — `xQueueOverwrite()` ensures display always gets latest data.

Atomic flags for lightweight signals. Queues for proper thread-safe data transfer.

### Decision 3: WiFi State Machine — WiFiManager

**States:** WIFI_AP_SETUP → CONNECTING → STA_CONNECTED, with fallback to AP_FALLBACK and reconnect states.

**GPIO overrides:** Short press shows IP on LEDs; long press during boot forces AP setup.

**Callbacks:** WebPortal, display task, FlightDataFetcher register for state change notifications.

### Decision 4: Web API Endpoints

11 REST endpoints: `GET /` (wizard/dashboard), `GET /api/status` (health), `GET/POST /api/settings` (config), `GET /api/wifi/scan` (async scan), `POST /api/reboot`, `POST /api/reset`, `GET /api/logos`, `POST/DELETE /api/logos/:name`, `GET /api/layout` (zones).

**Response envelope:** `{ ok: true/false, data: {...}, error: "message", code: "CODE" }`.

### Decision 5: Component Integration

**Dependency graph:** ConfigManager ← all; SystemStatus ← all; FlightDataFetcher (Core 1) → display task (Core 0) via atomic + queue.

**Initialization order:** ConfigManager → SystemStatus → LogoManager → LayoutEngine → WiFiManager → WebPortal → FlightDataFetcher → displayTask on Core 0.

**Rules:** ConfigManager initialized first; DisplayTask read-only; WebPortal only write path from browser; existing adapters preserve interface contracts.

### Decision 6: Error Handling — SystemStatus Registry

Centralized subsystem health tracking (WiFi, OpenSky, AeroAPI, CDN, NVS, LittleFS) with human-readable status strings. API call counter (RAM + hourly NVS sync, resets monthly via NTP).

## Implementation Patterns

### Naming Conventions

**C++ Classes:** PascalCase. **Methods:** camelCase. **Member Variables:** _camelCase. **Structs:** PascalCase name, snake_case fields. **Constants:** UPPER_SNAKE_CASE. **Files:** PascalCase.h/.cpp with `#pragma once`.

### Config Structs — All Fields snake_case

Struct fields use `snake_case` to match NVS keys and JSON field names — single convention across all three layers.

### NVS Keys & Abbreviations

15-char limit. Table includes: `brightness`, `text_color_{r,g,b}`, `center_lat`, `center_lon`, `radius_km`, `tiles_{x,y}`, `tile_pixels`, `display_pin`, `origin_corner`, `scan_dir`, `zigzag`, `fetch_interval`, `display_cycle`, `wifi_ssid`, `wifi_password`, `os_client_id`, `os_client_sec`, `aeroapi_key`, `api_calls`, `api_month`. Abbreviations documented in ConfigManager.h.

### Logging Pattern

**File:** `utils/Log.h`. Compile-time log levels: `LOG_E`, `LOG_I`, `LOG_V`. Macros compile to nothing at level 0.

### Error Handling Pattern

Boolean return + output parameter (matches existing codebase). Plus SystemStatus reporting and log macros.

### Zone Calculation Algorithm (C++ + JavaScript Parity)

Shared identical implementation in LayoutEngine.h and dashboard.js. Test vectors validate: 160x32 full mode, 80x32 full mode, 192x48 expanded mode, 160x16 compact mode.

### Web Asset Patterns

All `fetch()` calls check `json.ok` and call `showToast()` on failure. Mandatory pattern for every API interaction.

## Project Structure

### Directory Layout

```
firmware/
├── platformio.ini
├── custom_partitions.csv
├── src/main.cpp
├── core/ (ConfigManager, FlightDataFetcher, LayoutEngine, LogoManager, SystemStatus)
├── adapters/ (OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher, NeoMatrixDisplay, WebPortal, WiFiManager)
├── interfaces/ (BaseDisplay, BaseFlightFetcher, BaseStateVectorFetcher)
├── models/ (FlightInfo, StateVector, AirportInfo)
├── config/ (APIConfiguration, WiFiConfiguration, UserConfiguration, HardwareConfiguration, TimingConfiguration)
├── utils/ (GeoUtils, Log)
├── data/ (LittleFS content — gzipped HTML/JS/CSS, Leaflet, logos)
└── test/ (unit tests for ConfigManager, LayoutEngine)
```

### Requirements to Structure Mapping

Device Setup/Onboarding → WiFiManager, WebPortal, wizard HTML/JS. Configuration Management → ConfigManager, WebPortal, dashboard HTML/JS. Display Calibration → WebPortal, LayoutEngine, dashboard.js. Flight Data → FlightDataFetcher, NeoMatrixDisplay. Airline Logo → LogoManager, NeoMatrixDisplay. Responsive Layout → LayoutEngine, NeoMatrixDisplay. Logo Management → LogoManager, WebPortal, dashboard.js. System Operations → SystemStatus, WiFiManager, main.cpp.

## Enforcement Guidelines

All new code must:

1. Follow naming conventions (PascalCase classes, camelCase methods, _camelCase private vars, snake_case struct fields)
2. Use `#pragma once` in all headers
3. Use boolean return + output parameter for fallible operations
4. Report user-visible errors via `SystemStatus::set()`
5. Log via `LOG_E/I/V` from `utils/Log.h`
6. Use `String` for dynamic text, `const char*` for constants
7. Stream LittleFS files — never buffer entire files in RAM
8. Place classes in: `core/` (internal), `adapters/` (hardware/network), `interfaces/`, `models/`, `utils/`
9. Every JS `fetch()` must check `json.ok` + call `showToast()` on error
10. Document NVS key abbreviations if key exceeds 15 characters
11. No `new`/`delete` — use automatic storage; stream LittleFS files

## Architecture Validation

### Coherence ✅

All technology choices compatible on ESP32 Arduino. `snake_case` flows consistently through struct fields → NVS keys → JSON fields. Logging, error handling, include patterns are uniform.

### Requirements Coverage ✅

All 48 FRs mapped to architectural components. All NFRs addressed (hot-reload <1s, page load <3s, uptime 30+ days, WiFi recovery <60s, RAM <280KB, flash 4MB with headroom).

### Implementation Readiness ✅

6 core decisions with code examples, interfaces, rationale. 11 enforcement guidelines. NVS key table complete with abbreviations. API endpoint table with response formats. Shared zone algorithm with test vectors. Complete directory tree (44 files total, all EXISTING/NEW/UPDATED specified).

---

# Foundation Release — Architecture Extension

_Extends MVP architecture with OTA firmware updates, night mode/brightness scheduling, and onboarding polish. All MVP decisions remain in effect._

## Foundation Requirements Overview

**37 FRs across 6 groups:** OTA Firmware Updates (FR1-FR11), Settings Migration (FR12-FR14), Night Mode & Brightness Scheduling (FR15-FR24), Onboarding Polish (FR25-FR29), Dashboard UI (FR30-FR34), System & Resilience (FR35-FR37).

**NFRs:** OTA upload ≤30s, NTP sync ≤10s, brightness hot-reload ≤1s, firmware ≤1.5MB, LittleFS ≥500KB free, self-check ≤30s, 30+ days uptime, night mode ≤1% loop impact.

## Foundation Decisions

### Decision F1: Partition Table — Dual OTA Layout

**Updated `custom_partitions.csv`:** 20KB NVS + 8KB OTA + 1.5MB app0 + 1.5MB app1 + 960KB spiffs (LittleFS).

LittleFS shrinks from ~1.9MB to ~960KB (56% reduction). Breaking change — one-time USB reflash required, erases NVS + LittleFS. Settings export mitigates reconfiguration.

**Flash budget (Foundation):** 1.5MB firmware + 1.5MB OTA staging. LittleFS: ~80KB web assets + ~198KB logos (~2KB each) + ~682KB free.

### Decision F2: OTA Handler Architecture — WebPortal + main.cpp

No new class. OTA split across two locations:

| Concern | Location | Implementation |
|---------|----------|---------------|
| Upload endpoint | WebPortal.cpp | POST handler with stream-to-partition |
| Binary validation | WebPortal.cpp | Magic byte + size check |
| Stream-to-flash | WebPortal.cpp | `Update.write()` per chunk |
| Abort on failure | WebPortal.cpp | `Update.abort()` |
| Progress reporting | WebPortal.cpp | Bytes written / total |
| Reboot | WebPortal.cpp | `ESP.restart()` after success |
| Self-check | main.cpp | Boot sequence after init |
| Rollback detection | main.cpp | `esp_ota_get_last_invalid_partition()` |
| Version string | platformio.ini | `-D FW_VERSION=\"1.0.0\"` |

**Upload handler flow:** First chunk validates magic byte 0xE9 and calls `Update.begin()`. Per chunk: `Update.write()` validates write success. Final chunk: `Update.end(true)` finalizes and validates partition. On error anywhere: `Update.abort()`, return error JSON.

### Decision F3: OTA Self-Check — WiFi-OR-Timeout Strategy

Boot after all init: if WiFi connects OR 60-second timeout expires — whichever comes first — mark firmware valid. No self-HTTP-request. No web server check.

**Rationale:** WiFi proves network stack works. Timeout prevents false rollback on temporary outages. Crash-on-boot caught by watchdog regardless.

**Timeout budget:** Good firmware + WiFi = 5-15s, Good firmware no WiFi = 60s, Bad firmware = watchdog rollback.

### Decision F4: IANA-to-POSIX Timezone Mapping — Browser-Side JS

Browser converts IANA timezone to POSIX string client-side via TZ_MAP object (~50-80 entries). ESP32 stores and uses POSIX string. Zero firmware/LittleFS overhead.

**Wizard flow:** Auto-detect via `Intl.DateTimeFormat().resolvedOptions().timeZone`, show dropdown with IANA names pre-selected, send POSIX string to ESP32, fallback to manual POSIX entry if unknown.

### Decision F5: Night Mode Scheduler — Non-Blocking Main Loop

Brightness schedule check runs in Core 1 main loop (1-second interval). Non-blocking time check via `getLocalTime()` with `timeout=0`. Schedule times stored as minutes since midnight (uint16, 0-1439). Midnight-crossing logic: if dimStart ≤ dimEnd, same-day window; else midnight crossing.

**Interaction with manual brightness:** When scheduled and in dim window, scheduler overrides brightness. Dashboard indicates when schedule is active.

**NTP re-sync:** ESP32 LWIP defaults to 1-hour re-sync. Sufficient for <5 ppm RTC drift (~0.4s/day).

### Decision F6: ConfigManager Expansion — 5 New NVS Keys

| NVS Key | Type | Default | Category |
|---------|------|---------|----------|
| `timezone` | string | "UTC0" | schedule |
| `sched_enabled` | uint8 | 0 | schedule |
| `sched_dim_start` | uint16 | 1380 | schedule |
| `sched_dim_end` | uint16 | 420 | schedule |
| `sched_dim_brt` | uint8 | 10 | schedule |

New `ScheduleConfig` struct with all fields. 5 new hot-reload keys — no reboot required. Timezone change calls `configTzTime()` immediately.

### Decision F7: API Endpoint Additions

2 new endpoints: `POST /api/ota/upload` (multipart firmware), `GET /api/settings/export` (JSON download).

Updated `/api/status` response: add `firmware_version`, `rollback_detected`, `ntp_synced`, `schedule_active`, `uptime_seconds`.

## Foundation Patterns & Enforcement

### Rules 12-16 (Foundation-Specific)

12. OTA upload validates before writing — size + magic byte on first chunk, fail-fast with JSON error
13. OTA upload streams directly via `Update.write()` per chunk — never buffer full binary in RAM
14. `getLocalTime()` always uses `timeout=0` (non-blocking) — never block main loop
15. Schedule times stored as `uint16` minutes since midnight (0-1439) — no string time representations
16. Settings import client-side only — no server-side import endpoint

## Foundation Implementation Sequence

**Sequential foundation:**
1. Partition table update (custom_partitions.csv) — USB reflash required
2. ConfigManager expansion (ScheduleConfig, 5 new keys)

**OTA track (sequential, after step 2):**
3. OTA upload endpoint (WebPortal)
4. OTA self-check + rollback detection (main.cpp)

**Time track (parallelizable with OTA after step 2):**
5. NTP integration (`configTzTime()` after WiFi connect)
6. Night mode scheduler (main loop)

**Settings track (after step 2):**
7. Settings export endpoint (WebPortal)

**UI integration (after all tracks):**
8. Dashboard cards (Firmware, Night Mode), rollback toast
9. Wizard Step 6 (Test Your Wall), timezone auto-detect
10. Settings import in wizard JS

## Foundation Validation

**Coherence ✅:** All 7 decisions compatible. Partition change enables OTA, NTP enables night mode, ConfigManager expansion supports both. No contradictions.

**Requirements ✅:** All 37 FRs covered. OTA, settings migration, night mode, onboarding, dashboard UI, resilience all architecturally addressed.

**Implementation Readiness ✅:** 7 decisions with code examples. Partition sizes exact. NVS key table with abbreviations. API endpoints specified. Implementation sequence with parallelization noted.

---

# Display System Release — Architecture Extension

_Extends MVP + Foundation with pluggable display modes, mode registry, Classic Card migration, Live Flight Card mode, and Mode Picker UI. All prior decisions remain in effect._

## Display System Requirements Overview

**36 FRs across 8 groups:** DisplayMode Interface (FR1-FR4), Mode Registry & Lifecycle (FR5-FR10), Classic Card Mode (FR11-FR13), Live Flight Card Mode (FR14-FR16), Mode Picker UI (FR17-FR26), Mode Persistence (FR27-FR29), Mode Switch API (FR30-FR33), Display Infrastructure (FR34-FR36).

**NFRs:** Mode switch <2s, 60fps hot path, <10ms enumeration, no heap degradation after 100 switches, 30KB free heap floor, pixel parity with pre-migration output.

## Display System Decisions

### Decision D1: DisplayMode Interface — Abstract Class with RenderContext

**Central abstraction** — every mode implements this contract.

```cpp
class DisplayMode {
    virtual bool init(const RenderContext& ctx) = 0;
    virtual void render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) = 0;
    virtual void teardown() = 0;
    virtual const char* getName() const = 0;
    virtual const ModeZoneDescriptor& getZoneDescriptor() const = 0;
};
```

**RenderContext** bundles everything modes need, nothing they shouldn't:

```cpp
struct RenderContext {
    Adafruit_NeoMatrix* matrix;
    LayoutResult layout;
    uint16_t textColor;
    uint8_t brightness;
    uint16_t* logoBuffer;
    uint16_t displayCycleMs;
};
```

**Zone descriptor** — static metadata for Mode Picker UI wireframes.

**Key design:** `getMemoryRequirement()` is static function pointer in `ModeEntry` (not virtual) — needed before instantiation for heap guard.

### Decision D2: ModeRegistry — Static Table with Cooperative Switch Serialization

**Lifecycle:**
1. Teardown old mode, free heap
2. Measure `ESP.getFreeHeap()`
3. Check against `memoryRequirement()`
4. If sufficient: factory() → init() on new mode
5. If success: update index, set NVS pending, clear error
6. If failure: restore previous mode, set error

**Serialization:** Atomic `_requestedIndex` for cross-core signaling. `_switchState = SWITCHING` blocks new requests.

**NVS debounce:** 2-second quiet period after switch. No reboot required — hot-reload.

### Decision D3: NeoMatrixDisplay Responsibility Split

**Retained:** Hardware initialization, matrix object, brightness control, frame commit (`show()`), shared logo buffer, layout result, status messages, fallback renderer.

**Extracted:** `renderZoneFlight()`, `renderFlightZone()`, `renderTelemetryZone()`, `renderLogoZone()` → ClassicCardMode.

**New methods:** `show()` (frame commit), `buildRenderContext()` (assembles context), `displayFallbackCard()` (FR36 safety net).

### Decision D4: Display Task Integration — Cached Context + Tick

**Cache RenderContext** — rebuild only on config change.

**Replace renderFlight with:**
```cpp
ModeRegistry::tick(cachedCtx, flights);
g_display.show();
```

Tick handles mode-switch checks + state transitions + rendering in one call.

### Decision D5: Mode Switch API — Two Endpoints

`GET /api/display/modes` — list modes + active + switch state + zone metadata for Mode Picker wireframes.

`POST /api/display/mode` — request switch, returns after mode renders or fails.

**Zone descriptors** used by dashboard to build CSS Grid wireframe previews dynamically.

### Decision D6: NVS Persistence — Single Key with Debounced Write

`display_mode` key (12 chars) stores active mode ID in `"flightwall"` namespace. Debounced 2-second write after switch. Boot reads NVS, defaults to "classic_card".

### Decision D7: Mode Picker UI — Client-Side Wireframes + localStorage Notification

Mode list loaded via API on page load. Wireframe rendering via CSS Grid, data-driven from zone metadata. Schematic `<div>` elements positioned per zone with labels.

**Upgrade notification:** Dual-source dismissal — API flag + localStorage key. Dismissed via `localStorage.setItem()` + POST to clear firmware flag.

## Display System Patterns & Enforcement

### Rules 17-23 (Display System–Specific)

17. Mode classes in `firmware/modes/` — one `.h/.cpp` pair per mode
18. Modes ONLY access data through RenderContext — zero ConfigManager/WiFiManager/SystemStatus reads
19. Modes NEVER call `FastLED.show()` — frame commit exclusive to display task
20. Heap allocation in init(), deallocation in teardown() — constructors and render() must not allocate
21. Shared rendering helpers from `utils/DisplayUtils.h` — no duplication in mode classes
22. Adding new mode: exactly two touch points — new file in `modes/` + one line in MODE_TABLE[] in main.cpp
23. `MEMORY_REQUIREMENT` is `static constexpr` on each mode — reflects worst-case heap from init()

## Display System Implementation Sequence

**Critical path (three-phase ClassicCardMode extraction):**
1. DisplayMode interface + RenderContext
2. ModeRegistry (switch logic, heap guard, tick())
3. ClassicCardMode (COPY rendering logic first, validate pixel parity, then extract)
4. NeoMatrixDisplay refactoring (remove methods, add show()+buildRenderContext())
5. Display task integration (MODE_TABLE, ModeRegistry::init(), tick()+show())
6. LiveFlightCardMode (validates abstraction)
7. Mode Switch API (WebPortal endpoints)
8. NVS persistence (ConfigManager methods)
9. Mode Picker UI (dashboard HTML/JS)

## Display System Validation

**Coherence ✅:** All 7 decisions compatible, form consistent dependency chain. No contradictions with MVP + Foundation.

**Requirements ✅:** All 36 FRs covered. Interface, registry, modes, API, UI, persistence all explicitly mapped.

**Implementation Readiness ✅:** 7 decisions with code examples, switch flow, struct definitions. Three-phase extraction prevents highest-risk failure (pixel parity regression). Terminal fallback documented.

---

# Delight Release — Architecture Extension

_Extends MVP + Foundation + Display System with Clock Mode, Departures Board, animated fade transitions, mode scheduling, per-mode settings, and OTA Pull from GitHub. All prior decisions remain in effect._

## Delight Requirements Overview

**43 FRs across 9 groups:** Clock Mode (FR1-FR4), Departures Board (FR5-FR9), Mode Transitions (FR10-FR12), Mode Scheduling (FR13-FR17, FR36, FR39), Mode Configuration (FR18-FR20, FR40), OTA Pull (FR21-FR31, FR37-FR38), System Resilience (FR32-FR35), Extensibility (FR41-FR42), Documentation (FR43).

**NFRs:** ≥15fps transitions, <1s fade, ~30s idle fallback, <50ms row update, <60s OTA, ±5s schedule accuracy, 30-day uptime, no heap degradation.

## Delight Decisions

### Decision DL1: Fade Transition — Dual RGB888 Buffers in ModeRegistry

Fade is private method `_executeFadeTransition()` in ModeRegistry. Dual RGB888 buffers (~30KB total) allocated on switch, freed after fade completes.

**Blend algorithm:** 15 frames at 15fps = ~1s max. Blend per-pixel: `(outgoing * (15-step) + incoming * step) / 15`.

**Graceful degradation:** If malloc() fails, skip fade, use instant cut — not an error.

### Decision DL2: Mode Orchestration — Static Class with State Machine

`MANUAL` (user selection) / `SCHEDULED` (time-based) / `IDLE_FALLBACK` (zero flights).

**Priority:** `SCHEDULED > IDLE_FALLBACK > MANUAL`.

**Tick runs Core 1 main loop, ~1/sec:** Evaluates schedule rules, checks `g_flightCount` atomic, calls `ModeRegistry::requestSwitch()`.

**`onManualSwitch()`** called from WebPortal — updates MANUAL selection, exits IDLE_FALLBACK if needed.

### Decision DL3: OTAUpdater — GitHub Releases API Client with Incremental SHA-256

Static class: `checkForUpdate()` (synchronous, ~1-2s), `startDownload()` (spawns FreeRTOS task, returns immediately).

**Download flow:** Check GitHub latest release, download .sha256 file, validate format, get OTA partition, `Update.begin()`, stream .bin in chunks with incremental `mbedtls_sha256_update()`, verify hash, `Update.end(true)`, reboot.

**Error handling:** On any error after `Update.begin()`, always `Update.abort()`. Never attempt partition read-back. Inactive partition unchanged on failure.

### Decision DL4: Schedule Rules NVS — Fixed Max 8 with Indexed Keys

```
sched_r{N}_start   = uint16 (minutes since midnight)
sched_r{N}_end     = uint16
sched_r{N}_mode    = string (mode ID)
sched_r{N}_ena     = uint8 (enabled flag)
sched_r_count      = uint8 (active rule count)
```

Max 8 rules, indexed 0-7. Rules compacted on delete — no gaps. All within 15-char NVS limit.

### Decision DL5: Per-Mode Settings — Static Metadata with NVS Prefix Convention

Modes declare settings via `ModeSettingDef[]` and `ModeSettingsSchema` static const in mode `.h` files.

**NVS key pattern:** `m_{abbrev}_{setting}` (≤15 chars total). Mode abbreviations: `clock` (5), `depbd` (5), `lfcrd` (5) — documented.

**Mode abbreviation rule:** ≤5 chars, enforced by documentation + PR review.

**ConfigManager helpers:** `getModeSetting()`, `setModeSetting()` — read/write via m_{abbrev}_{key} convention.

### Decision DL6: API Endpoints — OTA Pull + Schedule + Mode Settings

**New endpoints:**
- `GET /api/ota/check` — check GitHub for update
- `POST /api/ota/pull` — start download task
- `GET /api/ota/status` — poll progress + state
- `GET /api/schedule` — get mode schedule rules
- `POST /api/schedule` — save schedule rules

**Updated endpoints:**
- `GET /api/display/modes` — add settings array per mode + current values
- `POST /api/display/mode` — accept optional settings object
- `GET /api/status` — add ota_available, ota_version, orchestrator_state, active_schedule_rule

## Delight Patterns & Enforcement

### Rules 24-30 (Delight-Specific)

24. Mode switches from WebPortal via `ModeOrchestrator::onManualSwitch()` — only two call sites: `tick()` and `onManualSwitch()`
25. OTA SHA-256 via `mbedtls_sha256_update()` per chunk — never post-download read-back; always `Update.abort()` after `Update.begin()` on error
26. Both OTA paths (Push + Pull) call `ModeRegistry::prepareForOTA()` before `Update.begin()` — sets `_switchState = SWITCHING`
27. Fade buffers malloc/free in single call — never persist beyond transition; malloc failure uses instant cut (graceful degradation)
28. Per-mode NVS via `ConfigManager::getModeSetting()/setModeSetting()` only — never construct `m_{abbrev}_{key}` manually; API handlers iterate schema dynamically
29. Mode settings schemas declared `static const` in mode `.h` files — never centralize in ConfigManager
30. Cross-core `std::atomic` globals in `main.cpp` only — modes/adapters never read/write atomics directly

## Delight Implementation Sequence

**Sequential foundation:**
1. ConfigManager expansion (ModeScheduleConfig, per-mode helpers)
2. ModeEntry expansion (settingsSchema field, nullptr for existing modes)

**Mode implementations (after step 2):**
3. ClockMode
4. DeparturesBoardMode

**Orchestration track (after step 3):**
5. ModeOrchestrator
6. Fade transition in ModeRegistry

**OTA track (after step 2):**
7. OTAUpdater
8. ModeRegistry::prepareForOTA()

**API + UI (after all firmware):**
9. WebPortal endpoints (OTA Pull, schedule, mode settings)
10. Dashboard UI (OTA Pull, schedule timeline, per-mode settings panels)

## Delight Validation

**Coherence ✅:** All 6 decisions form consistent dependency chain. Orchestrator uses existing `requestSwitch()` atomic. OTA Pull reuses Foundation streaming pattern. Per-mode settings follow ModeZoneDescriptor pattern.

**Requirements ✅:** All 43 FRs covered. Clock Mode, Departures Board, transitions, scheduling, settings, OTA Pull, resilience all architecturally addressed.

**Implementation Readiness ✅:** 6 decisions with code examples, state machine diagrams, API request/response formats. Cross-core signaling pattern documented. OTA error discipline specified. Terminal fallback path from Display System retained.

]]></file>
<file id="f72f6282" path="_bmad-output/implementation-artifacts/stories/fn-1-1-partition-table-and-build-configuration.md" label="DOCUMENTATION"><![CDATA[

# Story fn-1.1: Partition Table & Build Configuration

Status: blocked-hardware

## Story

As a **developer**,
I want **a dual-OTA partition table and firmware version build flag**,
So that **the device supports over-the-air updates with a known firmware identity**.

## Acceptance Criteria

1. **Given** the current firmware binary **When** built via `pio run` **Then** the binary size is measured and logged to stdout **And** if the binary exceeds 1.3MB, a warning is logged to stdout and noted in the 'Completion Notes List' section of this story with optimization recommendations (disable Bluetooth via `-D CONFIG_BT_ENABLED=0`, strip unused library features) before proceeding

2. **Given** `firmware/custom_partitions.csv` updated with dual-OTA layout: nvs 0x5000/20KB (offset 0x9000), otadata 0x2000/8KB (offset 0xE000), app0 0x180000/1.5MB (offset 0x10000), app1 0x180000/1.5MB (offset 0x190000), spiffs 0xF0000/960KB (offset 0x310000) **When** `platformio.ini` references the custom partition table and includes `-D FW_VERSION=\"1.0.0\"` in build_flags **Then** `pio run` builds successfully with no errors **And** the compiled binary is under 1.5MB

3. **Given** the new partition layout is flashed via USB **When** the device boots **Then** LittleFS mounts successfully on the 960KB spiffs partition **And** the following existing functionality is verified to work correctly: device boots successfully; web dashboard accessible at device IP; flight data fetches and displays on LED matrix; logo management functions (upload, list, delete); calibration tools function **And** `pio run -t uploadfs` uploads web assets and logos to LittleFS with at least 500KB free remaining

4. **Given** `FW_VERSION` is defined as a build flag **When** any source file references `FW_VERSION` **Then** the version string is available at compile time **And** the version string accurately reflects the value defined in the build flag when accessed at runtime

## Tasks / Subtasks

- [x] **Task 1: Measure current binary size baseline** (AC: #1)
  - [x] Run `pio run` and record binary size from build output
  - [x] Document current size as baseline for comparison
  - [x] If >1.3MB, document optimization recommendations

- [x] **Task 2: Update custom_partitions.csv with dual-OTA layout** (AC: #2)
  - [x] Modify `firmware/custom_partitions.csv` with exact offsets and sizes:
    - nvs: 0x9000, 20KB (0x5000)
    - otadata: 0xE000, 8KB (0x2000)
    - app0: 0x10000, 1.5MB (0x180000)
    - app1: 0x190000, 1.5MB (0x180000)
    - spiffs: 0x310000, 960KB (0xF0000)
  - [x] Verify total does not exceed 4MB flash

- [x] **Task 3: Add FW_VERSION build flag to platformio.ini** (AC: #2, #4)
  - [x] Add `-D FW_VERSION=\"1.0.0\"` to build_flags section
  - [x] Ensure proper escaping for string macro
  - [x] Add `-D CONFIG_BT_ENABLED=0` if binary is large (optimization) — NOT NEEDED, binary is 1.15MB

- [x] **Task 4: Build and verify firmware compiles** (AC: #2)
  - [x] Run `pio run` and verify no errors
  - [x] Confirm binary size under 1.5MB
  - [x] Document final binary size

- [ ] **Task 5: Flash and test on hardware** (AC: #3) — REQUIRES HARDWARE
  - [ ] Flash via USB: `pio run -t upload`
  - [ ] Verify device boots successfully
  - [ ] Verify LittleFS mounts on 960KB partition
  - [ ] Test all existing functionality:
    - Flight data pipeline fetches and displays
    - Web dashboard accessible at device IP
    - Logo management works
    - Calibration tools function

- [ ] **Task 6: Upload filesystem and verify space** (AC: #3) — REQUIRES HARDWARE
  - [ ] Run `pio run -t uploadfs`
  - [ ] Verify web assets upload successfully
  - [ ] Verify at least 500KB free remaining in LittleFS
  - [ ] Test served assets work correctly in browser

- [x] **Task 7: Verify FW_VERSION accessible in code** (AC: #4)
  - [x] Add test reference to FW_VERSION in code (e.g., log on boot)
  - [x] Verify version string compiles and logs correctly

## Dev Notes

### Critical Architecture Constraints

**Partition Layout Math (MUST be exact):**
```
Total Flash: 4MB (0x400000)
Bootloader:  0x1000 - 0x8FFF (reserved, not in CSV)
Partitions start at 0x9000

| Name    | Type | SubType | Offset   | Size     | End      |
|---------|------|---------|----------|----------|----------|
| nvs     | data | nvs     | 0x9000   | 0x5000   | 0xDFFF   | 20KB
| otadata | data | ota     | 0xE000   | 0x2000   | 0xFFFF   | 8KB
| app0    | app  | ota_0   | 0x10000  | 0x180000 | 0x18FFFF | 1.5MB
| app1    | app  | ota_1   | 0x190000 | 0x180000 | 0x30FFFF | 1.5MB
| spiffs  | data | spiffs  | 0x310000 | 0xF0000  | 0x3FFFFF | 960KB
```

**BREAKING CHANGE WARNING:**
- This partition change requires a full USB reflash
- NVS will be erased - all saved settings will be lost
- LittleFS will be erased - all uploaded logos will be lost
- Users should export settings first (but that endpoint doesn't exist yet - this is the first Foundation story)

**Binary Size Targets:**
- Warning threshold: 1.3MB (1,363,148 bytes)
- Hard limit: 1.5MB (1,572,864 bytes) - must fit in app0/app1 partition
- Current MVP partition has 2MB app space - we're cutting it to 1.5MB

**Optimization Flags (if needed):**
```ini
build_flags =
    -D CONFIG_BT_ENABLED=0        ; Saves ~60KB - Bluetooth disabled
    -D CONFIG_BTDM_CTRL_MODE_BLE_ONLY=1  ; If BLE needed, saves some
    -Os                            ; Optimize for size (already default)
```

### Current vs Target Partition Layout

**Current (MVP - from exploration):**
```csv
# Name,    Type,  SubType, Offset,   Size
nvs,       data,  nvs,     0x9000,   0x5000      # 20KB
otadata,   data,  ota,     0xe000,   0x2000      # 8KB
app0,      app,   ota_0,   0x10000,  0x200000    # 2MB (single app)
spiffs,    data,  spiffs,  0x210000, 0x1F0000   # ~1.9MB
```

**Target (Foundation dual-OTA):**
```csv
# Name,    Type,  SubType, Offset,   Size
nvs,       data,  nvs,     0x9000,   0x5000      # 20KB
otadata,   data,  ota,     0xe000,   0x2000      # 8KB
app0,      app,   ota_0,   0x10000,  0x180000    # 1.5MB
app1,      app,   ota_1,   0x190000, 0x180000    # 1.5MB (NEW)
spiffs,    data,  spiffs,  0x310000, 0xF0000    # 960KB (reduced)
```

**Space Trade-off:**
- App space: 2MB → 1.5MB per slot (but now have 2 slots for OTA)
- LittleFS: ~1.9MB → 960KB (56% reduction)
- LittleFS budget: ~80KB web assets + ~198KB logos (~2KB each) + ~682KB free

### FW_VERSION Macro Usage

**In platformio.ini:**
```ini
build_flags =
    -D FW_VERSION=\"1.0.0\"
```

**In C++ code (will be used in later stories):**
```cpp
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"  // Fallback for IDE/testing
#endif

Serial.printf("Firmware version: %s\n", FW_VERSION);
```

**Note:** The escaped quotes `\"` in platformio.ini produce a string literal. The preprocessor will replace `FW_VERSION` with `"1.0.0"`.

### Testing Standards

**Build Verification:**
- `pio run` must complete with 0 errors
- Check `.pio/build/esp32dev/firmware.bin` size
- Size logged in build output as "Building .pio/build/esp32dev/firmware.bin"

**Hardware Verification:**
- Boot log shows LittleFS mount success
- Web server responds at device IP
- Flight data displays on LED matrix
- All dashboard sections load

**Filesystem Verification:**
```bash
# Check LittleFS space after upload
# Will be visible in serial output or via /api/status endpoint
```

### Project Structure Notes

**Files to modify:**
1. `firmware/custom_partitions.csv` - Partition table definition
2. `firmware/platformio.ini` - Build flags and partition reference

**platformio.ini already has:**
- `board_build.filesystem = littlefs` ✓
- `board_build.partitions = custom_partitions.csv` ✓

**Only changes needed in platformio.ini:**
- Add `-D FW_VERSION=\"1.0.0\"` to build_flags

### References

- [Source: architecture.md#Decision F1: Partition Table — Dual OTA Layout]
- [Source: architecture.md#Decision F2: OTA Handler Architecture — WebPortal + main.cpp] (for FW_VERSION usage context)
- [Source: prd.md#Update Mechanism] - Partition table explanation
- [Source: prd.md#Flash Partition Layout] - 4MB total with custom CSV
- [Source: epic-fn-1.md#Story fn-1.1] - Full acceptance criteria
- [Source: project-context.md] - Build from `firmware/` with `pio run`

### Dependencies

**This story has NO code dependencies** - it's the foundation for all Foundation release stories.

**Stories that depend on this:**
- fn-1.2: ConfigManager Expansion (needs partition table for NVS)
- fn-1.3: OTA Upload Endpoint (needs app1 partition for OTA)
- fn-1.4: OTA Self-Check (needs dual-OTA partitions)
- fn-1.5: Settings Export (needs working firmware)
- fn-1.6: Dashboard Firmware Card (needs FW_VERSION)
- fn-1.7: Settings Import (needs working wizard)

### Risk Mitigation

1. **Binary too large:** If >1.5MB, add `-D CONFIG_BT_ENABLED=0`. BT is not used in MVP.
2. **LittleFS mount fails:** Verify partition offset/size math. Use `pio run -t erase` before flash if needed.
3. **Existing functionality broken:** This is a regression risk. Run full smoke test after flash.

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5

### Debug Log References

- Build output verified via `pio run` - SUCCESS
- Binary size: 1,206,432 bytes (1.15MB) - under 1.5MB limit
- Flash usage reported: 76.3% of 1.5MB partition (confirms new partition active)
- RAM usage: 16.4% (53,684 bytes of 327,680 bytes)

### Completion Notes List

**Implementation Summary:**

1. **Binary Size Baseline (Task 1):**
   - Current binary: 1,199,857 bytes (~1.14 MB) before changes
   - This is BELOW the 1.3MB warning threshold
   - No optimization flags needed (Bluetooth disable not required)
   - **Code Review Fix Applied**: Implemented automated binary size check via `check_size.py` script to satisfy AC #1 requirement for automated logging

2. **Partition Table Update (Task 2):**
   - Updated `custom_partitions.csv` with exact dual-OTA layout
   - Added app1 partition at 0x190000 (1.5MB) for OTA
   - Reduced spiffs from ~1.9MB to 960KB
   - Total: 4MB (0x400000) - verified correct

3. **FW_VERSION Build Flag (Task 3):**
   - Added `-D FW_VERSION=\"1.0.0\"` to platformio.ini build_flags
   - Added fallback `#define FW_VERSION "0.0.0-dev"` in main.cpp for IDE compatibility
   - Added boot log: `Serial.printf("FlightWall Firmware v%s\n", FW_VERSION);`

4. **Build Verification (Task 4):**
   - Final binary: 1,207,440 bytes (1.15MB) after code review fixes
   - 76.8% of new 1.5MB partition
   - Build completed with 0 errors
   - Automated size check runs on every build

5. **Hardware Testing (Tasks 5-6):**
   - REQUIRES PHYSICAL DEVICE ACCESS
   - Current LittleFS data: 456KB (99 logos + web assets)
   - With 960KB partition: ~504KB free (exceeds 500KB requirement)

6. **FW_VERSION Code Integration (Task 7):**
   - Version logged at boot via Serial.printf
   - Compiles and links correctly

**Code Review Fixes Applied (2026-04-12):**

*Initial dev_story fixes:*
1. **CRITICAL**: Fixed silent data loss risk by changing `LittleFS.begin(true)` to `LittleFS.begin(false)` with explicit error messaging
2. **HIGH**: Implemented automated binary size check script (`check_size.py`) to satisfy AC #1 requirement
3. **MEDIUM**: Added partition layout runtime validation to detect flash/firmware mismatches early

*Code review synthesis fixes (2026-04-12):*
1. **CRITICAL**: Fixed silent exit in `check_size.py` when binary is missing - now logs error and exits with code 1
2. **HIGH**: Added maintainability comments linking partition size constants across files to prevent drift
3. **MEDIUM**: Documented 36KB gap at end of partition table (0x3F7000-0x3FFFFF) reserved for future use

**Breaking Change Warning:**
- This partition change requires full USB reflash
- NVS and LittleFS will be erased on first flash with new partitions

**Final Validation (2026-04-12):**
- Clean build succeeds with 0 errors
- Binary size: 1,207,440 bytes (1.15MB) - 76.8% of 1.5MB partition
- check_size.py output confirmed: "OK: Binary size within limits"
- RAM usage: 16.4% (53,684 bytes of 327,680 bytes)
- Partition table math verified: nvs+otadata+app0+app1+spiffs = 0x3F7000 < 0x400000 (4MB)

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive context | BMad |
| 2026-04-12 | Implemented Tasks 1-4, 7. Tasks 5-6 require hardware. | Claude Opus 4.5 |
| 2026-04-12 | Code review synthesis: Applied critical fixes for AC #1 automation, LittleFS safety, partition validation | Claude Sonnet 4.5 |
| 2026-04-12 | Validated implementation: All software tasks complete. Build verified 1.15MB binary. Tasks 5-6 blocked on hardware. | Claude Opus 4 |

### File List

- `firmware/custom_partitions.csv` (MODIFIED) - Dual-OTA partition layout
- `firmware/platformio.ini` (MODIFIED) - Added FW_VERSION build flag, extra_scripts for size check
- `firmware/src/main.cpp` (MODIFIED) - Added FW_VERSION logging, partition validation, safe LittleFS mount
- `firmware/check_size.py` (NEW) - Automated binary size verification script


## Senior Developer Review (AI)

### Review: 2026-04-12
- **Reviewer:** AI Code Review Synthesis (2 validators)
- **Evidence Score:** 10.2 (Validator A), 13.3 (Validator B) → PASS with fixes
- **Issues Found:** 10 total issues raised (3 Critical verified, 7 dismissed as false positives or deferred)
- **Issues Fixed:** 3 critical/high priority issues
- **Action Items Created:** 0 (all critical issues fixed, low-priority items deferred)

### Review Summary

Both validators identified legitimate critical issues plus several false positives. After verification against source code:

**Critical Issues Fixed (Synthesis):**
1. **Silent exit in check_size.py** - Script returned silently if binary missing, masking build failures. Fixed with explicit error logging and exit(1).
2. **Magic number maintainability** - Partition size constants duplicated across 3 files with no linkage. Added cross-reference comments to prevent drift.
3. **Undocumented partition gap** - 36KB gap at end of flash not explained. Added comment documenting reserved space.

**Issues Dismissed as False Positives:**
1. **"No new/delete" violation** (Validator B) - FlightDataFetcher allocation is intentional singleton pattern; automatic storage would require global or static lifetime management with more complexity. Architecture rule applies to general allocation patterns, not core component initialization.
2. **Logging pattern violation** (Validator B) - Direct Serial.printf in validatePartitionLayout() is intentional for partition validation that runs before LOG subsystem may be ready. Not a violation of architectural intent.
3. **AC #3 hardware testing incomplete** (Both validators) - Story explicitly marks Tasks 5-6 as hardware-dependent and correctly blocked. Not an implementation gap.
4. **"spiffs" partition name** (Validator A) - This is the correct ESP32 Arduino framework convention. The partition subtype MUST be "spiffs" even when using LittleFS filesystem. Changing this would break the platform.
5. **SRP/DIP violations in main.cpp** (Validator B) - These architectural concerns are out of scope for this foundational partition table story. Main.cpp refactoring is not part of fn-1.1 acceptance criteria.
6. **Bluetooth optimization** (Validator B) - Binary is 1.15MB, well under 1.3MB warning threshold. Adding -D CONFIG_BT_ENABLED=0 is premature optimization.

**Deferred for Future Work:**
1. **Unit tests for check_size.py** (Validator A) - Valid improvement but not required for AC fulfillment. Recommend for dedicated testing story.
2. **Git-based auto-versioning** (Validator A) - Valid improvement but beyond scope; recommend for fn-1.6 or dedicated refactoring story.

---

<!-- CODE_REVIEW_SYNTHESIS_START -->
## Code Review Synthesis Report

### Synthesis Summary
10 issues raised by 2 validators. **3 critical issues verified and fixed** in source code. 6 issues dismissed as false positives or out-of-scope. 1 issue deferred as future enhancement. All acceptance criteria remain satisfied after fixes.

### Validations Quality

**Validator A** (Adversarial Review): Score 10.2/10 → MAJOR REWORK verdict
- Identified 8 issues: 2 Critical, 3 Important, 3 Minor
- Strong focus on maintainability and testing gaps
- 3 issues verified and fixed, 5 dismissed or deferred
- Assessment: High-quality review with valid critical findings on silent failures and magic numbers

**Validator B** (Adversarial Review): Score 13.3/10 → REJECT verdict
- Identified 6 issues: 2 Critical, 4 Important
- Strong focus on architectural patterns and SOLID violations
- 1 issue verified and fixed, 5 dismissed as false positives
- Assessment: Overly strict on architectural concerns outside story scope, but caught same critical binary check issue as Validator A

**Reviewer Consensus:**
- Both validators identified the silent exit bug in check_size.py (HIGH CONFIDENCE)
- Both validators flagged AC #3 hardware testing as incomplete (FALSE POSITIVE - correctly marked as hardware-dependent)
- Validator agreement on partition documentation gaps

---

## Issues Verified (by severity)

### Critical

**1. Silent Exit in check_size.py When Binary Missing**
- **Source**: Validator A (Bug #2), Validator B (CRITICAL)
- **Evidence**: Both validators independently identified that check_size.py returns silently at line 12-13 if binary doesn't exist
- **File**: `firmware/check_size.py`
- **Fix Applied**: Added explicit error logging and `env.Exit(1)` when binary is missing
  ```python
  # Before:
  if not os.path.exists(binary_path):
      return

  # After:
  if not os.path.exists(binary_path):
      print(f"ERROR: Firmware binary not found at {binary_path}")
      print("Build may have failed - check build output above")
      env.Exit(1)
  ```
- **Impact**: Previously, build could appear successful even if firmware.bin wasn't created, masking compile failures

### High

**2. Magic Numbers for Partition Sizes - No Cross-Reference**
- **Source**: Validator A (Issue #3), Validator B (IMPORTANT - Under-engineering)
- **Evidence**: Partition sizes hardcoded in 3 locations with no linkage:
  - `custom_partitions.csv` (source of truth)
  - `check_size.py:16` (0x180000, 0x140000)
  - `main.cpp:413-414` (EXPECTED_APP_SIZE, EXPECTED_SPIFFS_SIZE)
- **Files**: `firmware/custom_partitions.csv`, `firmware/check_size.py`, `firmware/src/main.cpp`
- **Fix Applied**: Added cross-reference comments in all 3 files to alert developers when updating partition sizes
  ```
  # custom_partitions.csv header:
  # IMPORTANT: If you modify partition sizes, also update:
  #   - firmware/check_size.py (limit/warning_threshold)
  #   - firmware/src/main.cpp validatePartitionLayout()

  # check_size.py:
  # Partition sizes match custom_partitions.csv (Story fn-1.1)
  limit = 0x180000  # app0/app1 partition size: 1.5MB

  # main.cpp:
  // Expected partition sizes from custom_partitions.csv (Story fn-1.1)
  // IMPORTANT: If you modify custom_partitions.csv, update these constants
  const size_t EXPECTED_APP_SIZE = 0x180000;   // app0/app1: 1.5MB
  ```
- **Impact**: Reduces risk of inconsistent partition size checks after future updates

### Medium

**3. Partition Table Gap Not Documented**
- **Source**: Validator A (CRITICAL - Partition table gap)
- **Evidence**: CSV ends at 0x3FFFFF (spiffs end: 0x310000 + 0xF0000 = 0x400000), leaving 36KB gap (0x3F7000 to 0x3FFFFF) unexplained
- **File**: `firmware/custom_partitions.csv`
- **Fix Applied**: Added comment documenting reserved gap
  ```
  # End: 0x3FFFFF (36KB gap to 4MB boundary reserved for future use)
  ```
- **Impact**: Clarifies that gap is intentional, not a calculation error

### Low
No low-severity issues required fixes.

---

## Issues Dismissed

**1. "No new/delete" Architectural Violation (Validator B - CRITICAL)**
- **Claimed Issue**: `main.cpp:610` uses `new FlightDataFetcher(...)` violating architecture rule "No new/delete — use automatic storage"
- **Dismissal Reason**: This is intentional singleton initialization for a core component with program lifetime. The architecture rule targets general heap allocation patterns (e.g., dynamic arrays, temporary objects), not fundamental subsystem setup. Automatic storage would require global or static variables with initialization order issues. The existing pattern is correct for this use case.
- **Evidence**: Project_context.md architecture rules are guidelines for general patterns, not absolute prohibitions for all scenarios. Core component initialization is an exception.

**2. Logging Pattern Violation (Validator B - IMPORTANT)**
- **Claimed Issue**: `main.cpp:420-443` uses `Serial.printf()` directly instead of `LOG_E/I/V` macros
- **Dismissal Reason**: `validatePartitionLayout()` runs early in setup() before full subsystem initialization. Direct Serial output for partition validation is intentional - it needs to work even if logging subsystem fails. This is defensive programming, not a violation.
- **Evidence**: Function logs partition table mismatches that could indicate flash corruption or incorrect firmware - critical to see this output even if LOG subsystem has issues.

**3. AC #3 Incomplete Implementation (Both Validators - IMPORTANT)**
- **Claimed Issue**: AC #3 requires hardware verification (device boot, LittleFS mount, uploadfs), but Tasks 5-6 marked incomplete
- **Dismissal Reason**: Story Dev Notes and Completion Notes explicitly state "Tasks 5-6 REQUIRES HARDWARE" and "Tasks 5-6 blocked on hardware." This is not an implementation gap - it's correctly deferred pending hardware access. Software implementation is complete and verified via build success.
- **Evidence**: Story file line 81-93 (Tasks 5-6), line 227 in Completion Notes: "REQUIRES PHYSICAL DEVICE ACCESS"

**4. "spiffs" Partition Name for LittleFS (Validator A - IMPORTANT)**
- **Claimed Issue**: Partition named "spiffs" but filesystem is LittleFS - semantically confusing
- **Dismissal Reason**: This is the **required** ESP32 Arduino framework convention. The partition subtype MUST be `ESP_PARTITION_SUBTYPE_DATA_SPIFFS` for LittleFS to work. Renaming would break platform compatibility. The confusion is a platform limitation, not a code defect.
- **Evidence**: `main.cpp:432-433` uses `esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL)` - this is the only way to locate LittleFS partition in ESP32 framework.

**5. SRP/DIP Violations in main.cpp (Validator B - IMPORTANT)**
- **Claimed Issue**: Startup coordinator in main.cpp violates SRP; direct instantiation of FlightDataFetcher violates DIP
- **Dismissal Reason**: These architectural concerns are **out of scope** for Story fn-1.1 (Partition Table & Build Configuration). The acceptance criteria focus on partition layout, build flags, and firmware version - not main.cpp refactoring. Expanding scope to refactor core architecture would violate story boundaries. If these patterns are problems, they should be separate stories.
- **Evidence**: Story fn-1.1 AC #1-4 make no mention of code structure improvements.

**6. Bluetooth Optimization Not Applied (Validator B - IMPORTANT)**
- **Claimed Issue**: platformio.ini missing `-D CONFIG_BT_ENABLED=0` to save ~60KB
- **Dismissal Reason**: Current binary is 1.15MB, well below the 1.3MB warning threshold defined in AC #1. Story Dev Notes state "If >1.5MB, add CONFIG_BT_ENABLED=0" - this condition is not met. Applying unnecessary optimization flags is premature. If binary size grows in future stories, this optimization can be added then.
- **Evidence**: check_size.py output: "Binary size: 1,207,440 bytes (1.15 MB) ... Usage: 76.8% ... OK: Binary size within limits"

**7. FW_VERSION Hardcoded String (Validator A - IMPORTANT)**
- **Claimed Issue**: No validation that build flag actually overrides fallback "0.0.0-dev"
- **Dismissal Reason**: For this foundational story, the requirement is that FW_VERSION compiles and is accessible at runtime (AC #4). Build output shows version logging works. Adding runtime assertions for non-default version is beyond AC scope and would break developer builds without platformio.ini. The fallback is intentional for IDE compatibility.
- **Evidence**: AC #4: "version string is available at compile time AND accurately reflects value defined in build flag when accessed at runtime" - satisfied by build success and serial logging.

---

## Changes Applied

**File: firmware/check_size.py**
- **Change**: Added error handling for missing binary file
- **Before**:
  ```python
  if not os.path.exists(binary_path):
      return
  ```
- **After**:
  ```python
  if not os.path.exists(binary_path):
      print(f"ERROR: Firmware binary not found at {binary_path}")
      print("Build may have failed - check build output above")
      env.Exit(1)
  ```

**File: firmware/check_size.py**
- **Change**: Added cross-reference comments for partition size constants
- **Before**:
  ```python
  limit = 0x180000  # 1.5MB partition size (1,572,864 bytes)
  warning_threshold = 0x140000  # 1.3MB warning (1,310,720 bytes)
  ```
- **After**:
  ```python
  # Partition sizes match custom_partitions.csv (Story fn-1.1)
  limit = 0x180000  # app0/app1 partition size: 1.5MB (1,572,864 bytes)
  warning_threshold = 0x140000  # Warning threshold: 1.3MB (1,310,720 bytes)
  ```

**File: firmware/custom_partitions.csv**
- **Change**: Added cross-reference header and partition size documentation
- **Before**:
  ```csv
  # Name,    Type,  SubType, Offset,   Size
  # Dual-OTA partition layout for Foundation release
  # Total: 4MB (0x400000), Bootloader uses 0x1000-0x8FFF
  nvs,       data,  nvs,     0x9000,   0x5000
  ...
  ```
- **After**:
  ```csv
  # Name,    Type,  SubType, Offset,   Size
  # Dual-OTA partition layout for Foundation release (Story fn-1.1)
  # Total: 4MB (0x400000), Bootloader uses 0x1000-0x8FFF
  #
  # IMPORTANT: If you modify partition sizes, also update:
  #   - firmware/check_size.py (limit/warning_threshold)
  #   - firmware/src/main.cpp validatePartitionLayout()
  #
  # Partition sizes:
  #   nvs:     20KB  (0x5000)
  #   otadata: 8KB   (0x2000)
  #   app0:    1.5MB (0x180000)
  #   app1:    1.5MB (0x180000)
  #   spiffs:  960KB (0xF0000) - LittleFS filesystem
  #
  # End: 0x3FFFFF (36KB gap to 4MB boundary reserved for future use)
  nvs,       data,  nvs,     0x9000,   0x5000
  ...
  ```

**File: firmware/src/main.cpp**
- **Change**: Added cross-reference comment in validatePartitionLayout()
- **Before**:
  ```cpp
  // Expected partition sizes from custom_partitions.csv
  const size_t EXPECTED_APP_SIZE = 0x180000;   // 1.5MB
  const size_t EXPECTED_SPIFFS_SIZE = 0xF0000; // 960KB
  ```
- **After**:
  ```cpp
  // Expected partition sizes from custom_partitions.csv (Story fn-1.1)
  // IMPORTANT: If you modify custom_partitions.csv, update these constants
  const size_t EXPECTED_APP_SIZE = 0x180000;   // app0/app1: 1.5MB
  const size_t EXPECTED_SPIFFS_SIZE = 0xF0000; // spiffs: 960KB
  ```

---

## Deep Verify Integration
Deep Verify did not produce findings for this story.

---

## Files Modified
- firmware/check_size.py
- firmware/custom_partitions.csv
- firmware/src/main.cpp
- _bmad-output/implementation-artifacts/stories/fn-1-1-partition-table-and-build-configuration.md (Dev Agent Record section only)

---

## Suggested Future Improvements

**1. Unit tests for check_size.py**
- **Scope**: Add pytest tests for binary size validation logic
- **Rationale**: Build script is critical path; tests would catch edge cases (missing file, size boundary conditions, warning thresholds)
- **Effort**: Low (1-2 hours) - create tests/unit/test_check_size.py with mocked file system
- **Deferred**: Not required for AC fulfillment; recommend for dedicated testing story or Epic fn-2

**2. Git-based automatic versioning**
- **Scope**: Replace hardcoded FW_VERSION="1.0.0" with git tag/commit hash extraction
- **Rationale**: Auto-versioning from `git describe --tags` would eliminate manual version updates and provide build traceability
- **Effort**: Low (2-3 hours) - Python script to extract version, update platformio.ini build_flags dynamically
- **Deferred**: Beyond scope of fn-1.1 foundational story; recommend for fn-1.6 (Dashboard Firmware Card) which displays version to users

---

## Test Results

**Firmware Build:**
```
Building .pio/build/esp32dev/firmware.bin
check_binary_size([".pio/build/esp32dev/firmware.bin"], [".pio/build/esp32dev/firmware.elf"])
============================================================
Firmware Binary Size Check (Story fn-1.1)
Binary size: 1,207,440 bytes (1.15 MB)
Partition limit: 1,572,864 bytes (1.50 MB)
Usage: 76.8%
OK: Binary size within limits
============================================================
[SUCCESS] Took 12.35 seconds
```

**Python Unit Tests:**
- 79 passed
- 9 errors (pre-existing smoke test setup issues unrelated to partition table changes)
- No test failures introduced by synthesis fixes
- Error details: `RuntimeError: CLI arguments were not initialized before tests started` in smoke tests - not related to modified files

**Validation:**
- Clean build with 0 compilation errors
- Binary size unchanged from pre-synthesis baseline (1.15MB)
- Automated size check executes successfully
- All modified files compile without warnings

<!-- CODE_REVIEW_SYNTHESIS_END -->



]]></file>
<file id="9fd78415" path="_bmad-output/implementation-artifacts/stories/fn-1-2-configmanager-expansion-schedule-keys-and-systemstatus.md" label="DOCUMENTATION"><![CDATA[

# Story fn-1.2: ConfigManager Expansion — Schedule Keys & SystemStatus

Status: complete

## Story

As a **developer**,
I want **ConfigManager to support 5 new schedule-related NVS keys and SystemStatus to track OTA and NTP subsystems**,
So that **night mode configuration and health reporting infrastructure is ready for all Foundation features**.

## Acceptance Criteria

1. **Given** ConfigManager initialized on first boot (empty NVS for new keys) **When** `getSchedule()` is called **Then** it returns a `ScheduleConfig` struct with defaults: `timezone="UTC0"`, `sched_enabled=0`, `sched_dim_start=1380` (23:00), `sched_dim_end=420` (07:00), `sched_dim_brt=10`

2. **Given** ConfigManager has schedule values in NVS **When** `getSchedule()` is called **Then** NVS values override defaults in the returned struct **And** NVS keys use 15-char compliant names: `timezone`, `sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt`

3. **Given** `applyJson()` is called with `{"timezone": "PST8PDT,M3.2.0,M11.1.0", "sched_enabled": 1}` **When** processing schedule keys **Then** RAM cache updates immediately **And** `onChange` callbacks fire **And** `reboot_required` is `false` (all 5 schedule keys are hot-reload) **And** total NVS usage for 5 new keys is under 256 bytes

4. **Given** `GET /api/settings` is called **When** response is built via `dumpSettingsJson()` **Then** all 5 new schedule keys appear in the flat JSON response alongside existing 22 keys

5. **Given** SystemStatus is initialized **When** `SystemStatus::set(Subsystem::OTA, ...)` or `SystemStatus::set(Subsystem::NTP, ...)` is called **Then** OTA and NTP subsystem entries appear in the health snapshot via `toJson()` **And** SUBSYSTEM_COUNT increases from 6 to 8 **And** subsystemName() returns "ota" and "ntp" respectively

## Tasks / Subtasks

- [x] **Task 1: Add ScheduleConfig struct to ConfigManager.h** (AC: #1)
  - [x] Add `struct ScheduleConfig` with fields: `String timezone`, `uint8_t sched_enabled`, `uint16_t sched_dim_start`, `uint16_t sched_dim_end`, `uint8_t sched_dim_brt`
  - [x] Document NVS key abbreviations in header comments (sched_dim_start, sched_dim_end, sched_dim_brt follow 15-char limit)
  - [x] Add static member `_schedule` and getter `getSchedule()` declaration

- [x] **Task 2: Implement schedule defaults and NVS loading** (AC: #1, #2)
  - [x] Add `_schedule` static member initialization in ConfigManager.cpp
  - [x] Add schedule defaults in `loadDefaults()`: timezone="UTC0", sched_enabled=0, sched_dim_start=1380, sched_dim_end=420, sched_dim_brt=10
  - [x] Add schedule NVS loading in `loadFromNvs()` using exact key names
  - [x] Add schedule NVS persistence in `persistToNvs()`

- [x] **Task 3: Implement schedule key handling in applyJson** (AC: #3)
  - [x] Add schedule key handlers in `updateConfigValue()` with validation:
    - `timezone`: String, max 40 chars (POSIX timezone)
    - `sched_enabled`: uint8, valid values 0 or 1
    - `sched_dim_start`: uint16, valid 0-1439 (minutes since midnight)
    - `sched_dim_end`: uint16, valid 0-1439
    - `sched_dim_brt`: uint8, valid 0-255
  - [x] Verify all 5 schedule keys are NOT in REBOOT_KEYS array (hot-reload path)
  - [x] Add ConfigLockGuard protection for _schedule read/write

- [x] **Task 4: Add schedule keys to JSON dump** (AC: #4)
  - [x] Add all 5 schedule keys to `dumpSettingsJson()` output
  - [x] Verify GET /api/settings returns 27 total keys (existing 22 + 5 new)

- [x] **Task 5: Implement getSchedule() getter** (AC: #1, #2)
  - [x] Add `getSchedule()` method following existing getter pattern with ConfigLockGuard (directly returning a copy of `_schedule`)
  - [x] Add `ScheduleConfig schedule` field to ConfigSnapshot struct for use in `loadFromNvs()` operations

- [x] **Task 6: Extend SystemStatus with OTA and NTP subsystems** (AC: #5)
  - [x] Add `OTA` and `NTP` to Subsystem enum in SystemStatus.h
  - [x] Update SUBSYSTEM_COUNT from 6 to 8
  - [x] Add "ota" and "ntp" cases to `subsystemName()` switch
  - [x] Verify `_statuses` array size matches new count

- [x] **Task 7: Add unit tests for new functionality** (AC: #1-5)
  - [x] Add test_defaults_schedule() — verify default values
  - [x] Add test_nvs_write_read_roundtrip_schedule() — NVS persistence
  - [x] Add test_apply_json_schedule_hot_reload() — verify hot-reload path
  - [x] Add test_apply_json_schedule_validation() — reject invalid values
  - [x] Add test_system_status_ota_ntp() — verify new subsystems work

- [x] **Task 8: Build and verify** (AC: #1-5)
  - [x] Run `pio run` — verify clean build with no errors
  - [x] Run `pio test` (on-device) — test BUILD passed; actual hardware execution deferred to developer (requires ESP32)
  - [x] Verify binary size remains under 1.5MB limit — VERIFIED: 1,202,221 bytes (1.15MB) = 76.4% of 1.5MB partition

## Dev Notes

### Critical Architecture Constraints

**NVS 15-Character Key Limit (HARD REQUIREMENT):**
All NVS keys MUST be 15 characters or less per ESP32 constraint. The 5 new keys:
| Key | Length | Type | Default | Notes |
|-----|--------|------|---------|-------|
| `timezone` | 8 | String | "UTC0" | POSIX timezone, max 40 chars value |
| `sched_enabled` | 13 | uint8 | 0 | 0=disabled, 1=enabled |
| `sched_dim_start` | 15 | uint16 | 1380 | 23:00 = 23*60 |
| `sched_dim_end` | 13 | uint16 | 420 | 07:00 = 7*60 |
| `sched_dim_brt` | 13 | uint8 | 10 | dim brightness 0-255 |

**NVS Namespace:** `"flightwall"` (defined at ConfigManager.cpp line 23)

**Thread Safety Pattern (MUST FOLLOW):**
All ConfigManager getters use `ConfigLockGuard` wrapper for FreeRTOS mutex protection:
```cpp
ScheduleConfig ConfigManager::getSchedule() {
    ConfigLockGuard guard;
    return _schedule;
}
```

**Hot-Reload vs Reboot Keys:**
- All 5 schedule keys are HOT-RELOAD — do NOT add to `REBOOT_KEYS[]` array
- Hot-reload keys use `schedulePersist(2000)` debounce
- Hot-reload keys fire `onChange` callbacks

**ScheduleConfig Struct Pattern (Follow existing structs):**
```cpp
struct ScheduleConfig {
    String timezone;           // POSIX timezone string, default "UTC0"
    uint8_t sched_enabled;     // 0=disabled, 1=enabled
    uint16_t sched_dim_start;  // minutes since midnight (0-1439)
    uint16_t sched_dim_end;    // minutes since midnight (0-1439)
    uint8_t sched_dim_brt;     // brightness during dim window (0-255)
};
```

**Validation Rules for updateConfigValue():**
- `timezone`: Accept any String up to 40 chars (POSIX format like "PST8PDT,M3.2.0,M11.1.0")
- `sched_enabled`: Only accept 0 or 1
- `sched_dim_start`/`sched_dim_end`: Only accept 0-1439 (24*60-1)
- `sched_dim_brt`: Accept 0-255 (same as brightness)

**NVS Type Mapping (Match existing patterns):**
```cpp
// In loadFromNvs():
if (prefs.isKey("timezone"))       snapshot.schedule.timezone = prefs.getString("timezone", snapshot.schedule.timezone);
if (prefs.isKey("sched_enabled"))  snapshot.schedule.sched_enabled = prefs.getUChar("sched_enabled", snapshot.schedule.sched_enabled);
if (prefs.isKey("sched_dim_start")) snapshot.schedule.sched_dim_start = prefs.getUShort("sched_dim_start", snapshot.schedule.sched_dim_start);
// ... etc

// In persistToNvs():
prefs.putString("timezone", _schedule.timezone);
prefs.putUChar("sched_enabled", _schedule.sched_enabled);
prefs.putUShort("sched_dim_start", _schedule.sched_dim_start);
// ... etc
```

### SystemStatus Extension

**Current Subsystem enum (6 entries):**
```cpp
enum class Subsystem : uint8_t {
    WIFI,
    OPENSKY,
    AEROAPI,
    CDN,
    NVS,
    LITTLEFS
};
```

**After extension (8 entries):**
```cpp
enum class Subsystem : uint8_t {
    WIFI,
    OPENSKY,
    AEROAPI,
    CDN,
    NVS,
    LITTLEFS,
    OTA,      // NEW
    NTP       // NEW
};
```

**Update SUBSYSTEM_COUNT:**
```cpp
static const uint8_t SUBSYSTEM_COUNT = 8;  // Was 6
```

**Update subsystemName():**
```cpp
case Subsystem::OTA:      return "ota";
case Subsystem::NTP:      return "ntp";
```

### JSON Response Format

**GET /api/settings response (27 keys after this story):**
```json
{
  "ok": true,
  "data": {
    "brightness": 5,
    "text_color_r": 255,
    "text_color_g": 255,
    "text_color_b": 255,
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
    "zone_logo_pct": 0,
    "zone_split_pct": 0,
    "zone_layout": 0,
    "fetch_interval": 30,
    "display_cycle": 3,
    "wifi_ssid": "",
    "wifi_password": "",
    "os_client_id": "",
    "os_client_sec": "",
    "aeroapi_key": "",
    "timezone": "UTC0",
    "sched_enabled": 0,
    "sched_dim_start": 1380,
    "sched_dim_end": 420,
    "sched_dim_brt": 10
  }
}
```

### Project Structure Notes

**Files to modify:**
1. `firmware/core/ConfigManager.h` — Add ScheduleConfig struct, _schedule member, getSchedule() declaration
2. `firmware/core/ConfigManager.cpp` — Add schedule handling in loadDefaults(), loadFromNvs(), persistToNvs(), updateConfigValue(), dumpSettingsJson(), getSchedule()
3. `firmware/core/SystemStatus.h` — Add OTA and NTP to Subsystem enum, update SUBSYSTEM_COUNT
4. `firmware/core/SystemStatus.cpp` — Add "ota" and "ntp" cases to subsystemName()
5. `firmware/test/test_config_manager/test_main.cpp` — Add new schedule and SystemStatus tests

**Files NOT to modify:**
- WebPortal.cpp — Already calls dumpSettingsJson() for GET /api/settings; no changes needed
- main.cpp — Schedule usage is Story fn-1.5+ (Night Mode); this story only adds infrastructure

### Testing Standards

**Existing test patterns to follow (from test_config_manager/test_main.cpp):**
- `clearNvs()` helper before each test that needs clean state
- `ConfigManager::init()` after clearNvs() to reinitialize
- Unity assertions: `TEST_ASSERT_EQUAL_UINT8`, `TEST_ASSERT_EQUAL_UINT16`, `TEST_ASSERT_TRUE`, `TEST_ASSERT_FALSE`

**New tests to add:**
```cpp
void test_defaults_schedule() {
    clearNvs();
    ConfigManager::init();
    ScheduleConfig s = ConfigManager::getSchedule();
    TEST_ASSERT_TRUE(s.timezone == "UTC0");
    TEST_ASSERT_EQUAL_UINT8(0, s.sched_enabled);
    TEST_ASSERT_EQUAL_UINT16(1380, s.sched_dim_start);
    TEST_ASSERT_EQUAL_UINT16(420, s.sched_dim_end);
    TEST_ASSERT_EQUAL_UINT8(10, s.sched_dim_brt);
}

void test_apply_json_schedule_hot_reload() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["timezone"] = "PST8PDT";
    doc["sched_enabled"] = 1;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(2, result.applied.size());
    TEST_ASSERT_FALSE(result.reboot_required);  // CRITICAL: schedule keys are hot-reload
    TEST_ASSERT_TRUE(ConfigManager::getSchedule().timezone == "PST8PDT");
    TEST_ASSERT_EQUAL_UINT8(1, ConfigManager::getSchedule().sched_enabled);
}

void test_system_status_ota_ntp() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Ready");
    SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Synced");

    SubsystemStatus ota = SystemStatus::get(Subsystem::OTA);
    TEST_ASSERT_EQUAL(StatusLevel::OK, ota.level);
    TEST_ASSERT_TRUE(ota.message == "Ready");

    SubsystemStatus ntp = SystemStatus::get(Subsystem::NTP);
    TEST_ASSERT_EQUAL(StatusLevel::OK, ntp.level);
    TEST_ASSERT_TRUE(ntp.message == "Synced");
}
```

### Previous Story Intelligence (fn-1.1)

**From fn-1.1 completion notes:**
- Binary size: 1,207,440 bytes (1.15MB) — 76.8% of 1.5MB partition
- Build system verified working with `pio run`
- FW_VERSION macro implemented at boot log
- Partition validation implemented in main.cpp

**Lessons learned from fn-1.1:**
1. **Silent failures must be explicit** — fn-1.1 had issues with silent exits; always log errors
2. **Cross-file constant sync** — Document when constants must stay synchronized across files
3. **Test on hardware when possible** — Software tests pass but hardware verification is separate

### NVS Usage Estimation (AC #3)

**Total NVS bytes for 5 new keys (estimate):**
| Key | Type | Max Bytes |
|-----|------|-----------|
| timezone | String | 40 + overhead ~50 |
| sched_enabled | uint8 | 1 + overhead ~10 |
| sched_dim_start | uint16 | 2 + overhead ~10 |
| sched_dim_end | uint16 | 2 + overhead ~10 |
| sched_dim_brt | uint8 | 1 + overhead ~10 |
| **Total** | | **~90 bytes** |

This is well under the 256-byte budget specified in AC #3.

### References

- [Source: architecture.md#Decision F6: ConfigManager Expansion — 5 New NVS Keys]
- [Source: architecture.md#NVS Keys & Abbreviations table]
- [Source: ConfigManager.cpp lines 23, 163-170 for existing patterns]
- [Source: SystemStatus.h lines 7-14 for Subsystem enum]
- [Source: epic-fn-1.md#Story fn-1.2] — FR37, AR9, AR10, AR11, NFR-C3

### Dependencies

**This story depends on:**
- fn-1.1 (Partition Table & Build Configuration) — COMPLETE

**Stories that depend on this:**
- fn-1.4: OTA Self-Check (uses SystemStatus::set(OTA, ...))
- fn-1.5: Settings Export (needs schedule keys in JSON dump)
- Future: Night Mode Scheduler (consumes ScheduleConfig)

### Risk Mitigation

1. **Thread safety regression**: Always use ConfigLockGuard wrapper, never access _schedule directly
2. **NVS key too long**: Double-check all keys ≤15 chars before implementation
3. **Hot-reload vs reboot confusion**: Explicitly verify schedule keys NOT in REBOOT_KEYS array
4. **JSON key mismatch**: Use exact same key names in updateConfigValue, dumpSettingsJson, and NVS

## Dev Agent Record

### Agent Model Used

Claude Sonnet 4.5 (Code Review Synthesis)

### Debug Log References

N/A

### Completion Notes List

**2026-04-12: Code Review Synthesis — Critical Issues Fixed (Round 3)**

4 independent code reviewers (Validators A, B, C, D) identified critical validation gaps and test baseline errors. All issues within story scope have been resolved:

**Critical Issues Fixed (Round 3):**

1. **FIXED: Test baseline error - 27 vs 29 keys** (Validator B - Critical)
   - Test claimed 27 total keys but `dumpSettingsJson()` outputs 29 keys
   - Breakdown: 4 display + 3 location + 10 hardware + 2 timing + 5 network + 5 schedule = 29
   - Updated test assertion from `TEST_ASSERT_EQUAL_UINT32(27, keyCount)` to `29`
   - test_main.cpp line 414

2. **FIXED: Missing type validation for timezone** (Validators B, C)
   - `timezone` field accepted coerced non-string JSON (bool, number)
   - Added `value.is<const char*>()` check before string conversion
   - ConfigManager.cpp line 136

3. **FIXED: Missing negative number validation for sched_dim_start** (Validator C - Critical)
   - `value.as<uint16_t>()` could accept negative JSON values that wrap to positive
   - Added type check `value.is<int>()` and signed bounds validation before cast
   - ConfigManager.cpp lines 151-155

4. **FIXED: Missing negative number validation for sched_dim_end** (Validator C - Critical)
   - Same issue as sched_dim_start - could accept negative values
   - Added type check `value.is<int>()` and signed bounds validation before cast
   - ConfigManager.cpp lines 158-162

**Issues Dismissed (Round 3 - Pre-existing or Out of Scope):**
- SystemStatus mutex timeout fallback (Validators A, C) — Pre-existing pattern across all SystemStatus methods
- ConfigSnapshot heap overhead (Validator B) — Intentional for atomic semantics
- Secret exposure in /api/settings (Validator D) — Pre-existing design, needs separate security story
- SystemStatus::init() not idempotent (Validator B) — Test artifact, production calls once
- Missing schedule-specific callback test (Validator B) — Covered by combination of generic callback + hot-reload tests
- Blocking delay() in test (Validator A) — Test-only code, functions correctly
- String copying (Validators A, C) — Minor performance concern, not worth complexity
- NVS write result checking (Validator B) — ESP32 pattern, pre-existing

**Build Verification (Round 3):**
- `pio run`: SUCCESS (76.9% flash usage, 1,209,440 bytes)
- `pio test --without-uploading --without-testing -f test_config_manager`: BUILD PASSED
- All 4 critical validation fixes compile cleanly

**Final Acceptance Criteria Verification:**
- AC #1: `getSchedule()` returns ScheduleConfig with correct defaults ✅
- AC #2: NVS loading with 15-char compliant keys ✅
- AC #3: applyJson hot-reload path + robust validation (all schedule keys NOT in REBOOT_KEYS) ✅
- AC #4: dumpSettingsJson includes all 29 keys (corrected from 27) ✅
- AC #5: SystemStatus has OTA/NTP subsystems, SUBSYSTEM_COUNT=8 ✅

---

**2026-04-12: Code Review Synthesis — Critical Issues Fixed (Round 2)**

4 independent code reviewers (Validators B, C, D, and additional review) identified critical issues. All have been resolved:

**Critical Issues Fixed:**

1. **FIXED: Integer overflow in `sched_enabled` validation** (Validator B, D)
   - Changed from `value.as<uint8_t>()` (which wraps 256→0) to proper validation
   - Now validates as `unsigned int` before cast: rejects values >1 correctly
   - Also added type check `value.is<unsigned int>()` to catch non-numeric input
   - ConfigManager.cpp lines 141-148

2. **FIXED: Missing validation for `sched_dim_brt`** (Validators B, C, D)
   - Added proper bounds checking: rejects values >255
   - Added type validation to prevent silent coercion of invalid JSON types
   - ConfigManager.cpp lines 161-168

3. **FIXED: Test suite contradicts implementation** (Validator D - Critical)
   - Renamed `test_apply_json_ignores_unknown_keys` → `test_apply_json_rejects_unknown_keys`
   - Updated test expectations: `applyJson()` uses all-or-nothing validation (not partial success)
   - Test now correctly verifies that unknown keys reject the entire batch
   - test_main.cpp lines 190-204

**High Priority Issues Fixed:**

4. **FIXED: Missing test coverage for AC #4** (Validators B, D)
   - Added `test_dump_settings_json_includes_schedule()` test
   - Verifies all 5 schedule keys present in JSON output
   - Verifies total key count = 27 (22 original + 5 schedule)
   - test_main.cpp lines 385-415

5. **FIXED: Validation test coverage gaps** (Validator B)
   - Added `sched_dim_brt > 255` rejection test
   - Added `sched_enabled = 256` overflow rejection test
   - test_main.cpp lines 372-382

**Build Verification:**
- `pio run`: SUCCESS (76.9% flash usage, 1,209,280 bytes)
- `pio test --without-uploading --without-testing -f test_config_manager`: BUILD PASSED
- All source code changes compile cleanly with no errors

**Issues Dismissed (Pre-existing, Out of Story Scope):**
- Secret exposure in `/api/settings` (Validator D) — Pre-existing issue, requires separate security story
- SystemStatus mutex timeout fallback (Validators C, D) — Pre-existing design pattern, requires broader refactor
- ConfigSnapshot heap overhead (Validator B) — Necessary for atomic semantics
- SystemStatus tight coupling (Validator C) — Pre-existing architecture

**Reviewer Consensus:**
- All 4 reviewers identified overlapping critical validation issues
- Evidence scores: Validator B = 5.6, Validator C = 6.9, Validator D = 13.5
- All critical issues within story scope have been addressed

**Final Acceptance Criteria Verification:**
- AC #1: `getSchedule()` returns ScheduleConfig with correct defaults ✅
- AC #2: NVS loading with 15-char compliant keys ✅
- AC #3: applyJson hot-reload path + validation (all schedule keys NOT in REBOOT_KEYS) ✅
- AC #4: dumpSettingsJson includes all 27 keys (verified by new test) ✅
- AC #5: SystemStatus has OTA/NTP subsystems, SUBSYSTEM_COUNT=8 ✅

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Code review synthesis Round 1 (initial fixes) | Claude Sonnet 4.5 |
| 2026-04-12 | Code review synthesis Round 2 (validation fixes) | Claude Sonnet 4.5 |
| 2026-04-12 | Code review synthesis Round 3 (test baseline + negative validation) | Claude Sonnet 4.5 |
| 2026-04-12 | Final verification complete — all ACs verified, build/test passing | Claude Opus 4 |

### File List

- `firmware/core/ConfigManager.h` (MODIFIED)
- `firmware/core/ConfigManager.cpp` (MODIFIED)
- `firmware/core/SystemStatus.h` (MODIFIED)
- `firmware/core/SystemStatus.cpp` (MODIFIED)
- `firmware/test/test_config_manager/test_main.cpp` (MODIFIED)

## Senior Developer Review (AI)

### Review: 2026-04-12 (Round 3)
- **Reviewer:** AI Code Review Synthesis (4 validators)
- **Evidence Score:** 10.9 (highest) → REJECT (before Round 3 fixes)
- **Issues Found:** 4 critical, 0 high
- **Issues Fixed:** 4
- **Action Items Created:** 0

### Review: 2026-04-12 (Round 2)
- **Reviewer:** AI Code Review Synthesis (4 validators)
- **Evidence Score:** 13.5 → REJECT (before Round 2 fixes)
- **Issues Found:** 5 critical, 2 high
- **Issues Fixed:** 7
- **Action Items Created:** 0


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

Status: ready-for-dev

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a **user**,
I want **the device to verify new firmware works after an OTA update and automatically roll back if it doesn't**,
So that **a bad firmware update never bricks my device**.

## Acceptance Criteria

1. **Given** the device boots after an OTA update **When** WiFi connects successfully **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called **And** the partition is marked valid **And** a log message records the time to mark valid (e.g., `"Marked valid, WiFi connected in 8s"`) **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Firmware verified — WiFi connected in Xs")` is recorded

2. **Given** the device boots after an OTA update and WiFi does not connect **When** 60 seconds elapse since boot **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called on timeout **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Marked valid on timeout — WiFi not verified")` is recorded **And** a WARNING log message is emitted

3. **Given** the device boots after an OTA update and crashes before the self-check completes **When** the watchdog fires and the device reboots **Then** the bootloader detects the active partition was never marked valid **And** the bootloader rolls back to the previous partition automatically (this is ESP32 bootloader behavior, not application code)

4. **Given** a rollback has occurred **When** `esp_ota_get_last_invalid_partition()` returns non-NULL during boot **Then** a `rollbackDetected` flag is set to `true` **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Firmware rolled back to previous version")` is recorded **And** the invalid partition label is logged (e.g., `"Rollback detected: partition 'app1' was invalid"`)

5. **Given** `GET /api/status` is called **When** the response is built **Then** it includes `"firmware_version": "1.0.0"` (from `FW_VERSION` build flag) **And** `"rollback_detected": true/false` in the response data object

6. **Given** normal boot (not after OTA) **When** the self-check runs **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called (idempotent — safe on non-OTA boots, returns ESP_OK with no effect when image is already valid) **And** no WARNING or ERROR status is set for OTA subsystem

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

Note: `FW_VERSION` is a compile-time macro already available project-wide. If SystemStatus.cpp doesn't have it, add:
```cpp
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"
#endif
```

### Critical Gotchas

1. **No factory partition** — The partition table has only app0 and app1 (no factory). If BOTH partitions become invalid, the device is bricked. However, this can only happen if the user OTA-updates with bad firmware AND the rollback target is also bad — extremely unlikely since the rollback target was previously running.

2. **Rollback detection is per-boot** — `esp_ota_get_last_invalid_partition()` returns the last invalid partition persistently (survives reboots). The flag will remain true until a successful OTA clears the invalid partition state. Consider whether the dashboard should show this persistently or just on the first boot after rollback.

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

### File List

- `firmware/src/main.cpp` (MODIFIED) — Rollback detection, self-check function, loop integration, boot timestamp, FlightStatsSnapshot extension
- `firmware/core/SystemStatus.cpp` (MODIFIED) — Add firmware_version and rollback_detected to toExtendedJson()


]]></file>
<file id="e166c6e7" path="firmware/core/SystemStatus.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: Centralized subsystem health registry.
Responsibilities:
- Track health status (OK/WARNING/ERROR) for each subsystem with message and timestamp.
- Provide JSON serialization for the /api/status endpoint.
- Build extended diagnostics JSON for the health page (Story 2.4).
*/
#include "core/SystemStatus.h"
#include "core/ConfigManager.h"
#include "core/LogoManager.h"
#include "utils/Log.h"
#include <WiFi.h>
#include <LittleFS.h>

SubsystemStatus SystemStatus::_statuses[SUBSYSTEM_COUNT] = {};
SemaphoreHandle_t SystemStatus::_mutex = nullptr;

void SystemStatus::init() {
    _mutex = xSemaphoreCreateMutex();
    if (!_mutex) {
        LOG_E("SystemStatus", "Mutex creation failed — running without synchronization");
    }
    for (uint8_t i = 0; i < SUBSYSTEM_COUNT; i++) {
        _statuses[i].level = StatusLevel::OK;
        _statuses[i].message = "Not initialized";
        _statuses[i].timestamp = millis();
    }
    LOG_I("SystemStatus", "Initialized");
}

void SystemStatus::set(Subsystem sys, StatusLevel level, const String& message) {
    uint8_t idx = static_cast<uint8_t>(sys);
    if (idx >= SUBSYSTEM_COUNT) return;

    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        _statuses[idx].level = level;
        _statuses[idx].message = message;
        _statuses[idx].timestamp = millis();
        xSemaphoreGive(_mutex);
    } else {
        _statuses[idx].level = level;
        _statuses[idx].message = message;
        _statuses[idx].timestamp = millis();
    }
}

SubsystemStatus SystemStatus::get(Subsystem sys) {
    uint8_t idx = static_cast<uint8_t>(sys);
    if (idx >= SUBSYSTEM_COUNT) {
        return {StatusLevel::ERROR, "Invalid subsystem", millis()};
    }
    SubsystemStatus snapshot;
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        snapshot = _statuses[idx];
        xSemaphoreGive(_mutex);
    } else {
        snapshot = _statuses[idx]; // best-effort if mutex unavailable
    }
    return snapshot;
}

void SystemStatus::toJson(JsonObject& obj) {
    // Snapshot all statuses under mutex, then serialize outside
    SubsystemStatus snap[SUBSYSTEM_COUNT];
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (uint8_t i = 0; i < SUBSYSTEM_COUNT; i++) {
            snap[i] = _statuses[i];
        }
        xSemaphoreGive(_mutex);
    } else {
        for (uint8_t i = 0; i < SUBSYSTEM_COUNT; i++) {
            snap[i] = _statuses[i];
        }
    }
    for (uint8_t i = 0; i < SUBSYSTEM_COUNT; i++) {
        JsonObject sub = obj[subsystemName(static_cast<Subsystem>(i))].to<JsonObject>();
        sub["level"] = levelName(snap[i].level);
        sub["message"] = snap[i].message;
        sub["timestamp"] = snap[i].timestamp;
    }
}

void SystemStatus::toExtendedJson(JsonObject& obj, const FlightStatsSnapshot& stats) {
    // --- subsystems ---
    JsonObject subsystems = obj["subsystems"].to<JsonObject>();
    toJson(subsystems);

    // --- wifi_detail ---
    JsonObject wifiDetail = obj["wifi_detail"].to<JsonObject>();
    wifi_mode_t mode = WiFi.getMode();
    if (mode == WIFI_STA) {
        wifiDetail["mode"] = "STA";
        wifiDetail["ssid"] = WiFi.SSID();
        wifiDetail["rssi"] = WiFi.RSSI();
        wifiDetail["ip"] = WiFi.localIP().toString();
    } else if (mode == WIFI_AP || mode == WIFI_AP_STA) {
        wifiDetail["mode"] = "AP";
        wifiDetail["ssid"] = WiFi.softAPSSID();
        wifiDetail["ip"] = WiFi.softAPIP().toString();
        wifiDetail["clients"] = WiFi.softAPgetStationNum();
    } else {
        wifiDetail["mode"] = "OFF";
    }

    // --- device ---
    JsonObject device = obj["device"].to<JsonObject>();
    device["uptime_ms"] = millis();
    device["free_heap"] = ESP.getFreeHeap();
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    device["fs_total"] = totalBytes;
    device["fs_used"] = usedBytes;
    device["logo_count"] = LogoManager::getLogoCount();

    // --- flight ---
    JsonObject flight = obj["flight"].to<JsonObject>();
    flight["last_fetch_ms"] = stats.last_fetch_ms;
    flight["state_vectors"] = stats.state_vectors;
    flight["enriched_flights"] = stats.enriched_flights;
    flight["logos_matched"] = stats.logos_matched;

    // --- quota ---
    JsonObject quota = obj["quota"].to<JsonObject>();
    uint16_t fetchInterval = ConfigManager::getTiming().fetch_interval;
    uint32_t estimatedMonthly = (fetchInterval > 0) ? (2592000UL / fetchInterval) : 0;
    bool overPace = estimatedMonthly > 4000;
    quota["fetches_since_boot"] = stats.fetches_since_boot;
    quota["limit"] = 4000;
    quota["fetch_interval_s"] = fetchInterval;
    quota["estimated_monthly_polls"] = estimatedMonthly;
    quota["over_pace"] = overPace;
}

const char* SystemStatus::subsystemName(Subsystem sys) {
    switch (sys) {
        case Subsystem::WIFI:     return "wifi";
        case Subsystem::OPENSKY:  return "opensky";
        case Subsystem::AEROAPI:  return "aeroapi";
        case Subsystem::CDN:      return "cdn";
        case Subsystem::NVS:      return "nvs";
        case Subsystem::LITTLEFS: return "littlefs";
        case Subsystem::OTA:      return "ota";
        case Subsystem::NTP:      return "ntp";
        default:                  return "unknown";
    }
}

const char* SystemStatus::levelName(StatusLevel level) {
    switch (level) {
        case StatusLevel::OK:      return "ok";
        case StatusLevel::WARNING: return "warning";
        case StatusLevel::ERROR:   return "error";
        default:                   return "unknown";
    }
}


]]></file>
<file id="efdcd863" path="firmware/src/main.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: Firmware entry point for ESP32.
Responsibilities:
- Initialize serial, connect to Wi-Fi, and construct fetchers and display.
- Periodically fetch state vectors (OpenSky), enrich flights (AeroAPI), and push to queue.
- Display task on Core 0 reads queue and renders to LED matrix independently.
Configuration: ConfigManager (NVS-backed with compile-time fallbacks).
Architecture: Producer-Consumer dual-core (Core 1 = fetch/network, Core 0 = display).
*/
#include <vector>
#include <atomic>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_task_wdt.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "utils/Log.h"
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"
#include "adapters/OpenSkyFetcher.h"
#include "adapters/AeroAPIFetcher.h"
#include "core/FlightDataFetcher.h"
#include "adapters/NeoMatrixDisplay.h"
#include "adapters/WiFiManager.h"
#include "adapters/WebPortal.h"
#include "core/LayoutEngine.h"
#include "core/LogoManager.h"

// Firmware version defined in platformio.ini build_flags
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"  // Fallback for IDE/testing
#endif

#ifndef PIO_UNIT_TESTING

// --- Shared data structures (Task 1) ---

struct FlightDisplayData {
    std::vector<FlightInfo> flights;
};

struct DisplayStatusMessage {
    char text[64];
    uint32_t durationMs;
};

// Double buffer for safe cross-core data transfer (no memcpy of String/vector)
static FlightDisplayData g_flightBuf[2];
static uint8_t g_writeBuf = 0;

// FreeRTOS queue holding a pointer to the current read buffer
static QueueHandle_t g_flightQueue = nullptr;
static QueueHandle_t g_displayMessageQueue = nullptr;

// Atomic flag for config change notification (Core 1 sets, Core 0 reads)
static std::atomic<bool> g_configChanged(false);

// --- Startup progress coordinator (Story 1.8) ---
// Owns the ordered LED status sequence during post-wizard startup.
// Advances intentionally through phases; the single-entry overwrite queue
// means only one message is active at a time, so we drive transitions
// from loop() as each real event completes.

enum class StartupPhase : uint8_t {
    IDLE,               // Normal operation — coordinator inactive
    SAVING_CONFIG,      // "Saving config..." (set by reboot callback)
    CONNECTING_WIFI,    // "Connecting to WiFi..."
    WIFI_CONNECTED,     // "WiFi Connected ✓" then "IP: x.x.x.x"
    WIFI_FAILED,        // WiFi did not connect — fallback to AP/setup
    AUTHENTICATING,     // "Authenticating APIs..."
    FETCHING_FLIGHTS,   // "Fetching flights..."
    COMPLETE            // First flight data ready — hand off to normal rendering
};

static StartupPhase g_startupPhase = StartupPhase::IDLE;
static unsigned long g_phaseEnteredMs = 0;
static bool g_firstFetchDone = false;  // One-shot flag: first fetch after startup
static constexpr unsigned long AUTHENTICATING_DISPLAY_MS = 1000UL;

static void enterPhase(StartupPhase phase)
{
    g_startupPhase = phase;
    g_phaseEnteredMs = millis();
}

// --- Existing globals ---

static OpenSkyFetcher g_openSky;
static AeroAPIFetcher g_aeroApi;
static FlightDataFetcher *g_fetcher = nullptr;
static NeoMatrixDisplay g_display;
static WiFiManager g_wifiManager;
static AsyncWebServer g_webServer(80);
static WebPortal g_webPortal;

static bool g_forcedApSetup = false;  // Set if boot button held during startup

// GPIO 0 is the "BOOT" button on most ESP32 devkits.
// Holding it low during boot forces AP setup mode.
// NOTE: GPIO 0 is a strapping pin — holding it low at hardware reset can
// trigger UART download mode in ROM. This detection runs after ROM boot,
// during Arduino setup(), so it only triggers when firmware is already running.
static constexpr gpio_num_t BOARD_BOOT_BUTTON_GPIO = GPIO_NUM_0;

// --- Boot button short-press state (Task 5) ---
static bool g_buttonLastState = HIGH;       // Previous debounced state
static unsigned long g_buttonLastChangeMs = 0;
static unsigned long g_buttonPressStartMs = 0;
static constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;
static constexpr unsigned long BUTTON_SHORT_PRESS_MAX_MS = 1000;  // Presses longer than this are ignored (long press)

static unsigned long g_lastFetchMs = 0;

// --- Flight stats snapshot for /api/status health page (Story 2.4) ---
// Written from loop() on Core 1 after each fetch; read from HTTP handler on async TCP task.
// Use std::atomic for each field to avoid torn reads on 32-bit ESP32.
static std::atomic<unsigned long> g_statsLastFetchMs(0);
static std::atomic<uint16_t> g_statsStateVectors(0);
static std::atomic<uint16_t> g_statsEnrichedFlights(0);
static std::atomic<uint16_t> g_statsLogosMatched(0);    // Placeholder 0 until Epic 3
static std::atomic<uint32_t> g_statsFetchesSinceBoot(0);

FlightStatsSnapshot getFlightStatsSnapshot() {
    FlightStatsSnapshot s;
    s.last_fetch_ms = g_statsLastFetchMs.load();
    s.state_vectors = g_statsStateVectors.load();
    s.enriched_flights = g_statsEnrichedFlights.load();
    s.logos_matched = g_statsLogosMatched.load();
    s.fetches_since_boot = g_statsFetchesSinceBoot.load();
    return s;
}

// --- Layout engine state (Story 3.1) ---
// Computed once at boot from HardwareConfig; recomputed on config changes.
// Read by WebPortal for GET /api/layout.
static LayoutResult g_layout;

LayoutResult getCurrentLayout() {
    return g_layout;
}

static void queueDisplayMessage(const String &message, uint32_t durationMs = 0)
{
    if (g_displayMessageQueue == nullptr)
    {
        return;
    }

    DisplayStatusMessage statusMessage = {};
    snprintf(statusMessage.text, sizeof(statusMessage.text), "%s", message.c_str());
    statusMessage.durationMs = durationMs;
    xQueueOverwrite(g_displayMessageQueue, &statusMessage);
}

static void showInitialWiFiMessage()
{
    switch (g_wifiManager.getState())
    {
        case WiFiState::AP_SETUP:
            g_display.displayMessage(String("Setup Mode"));
            break;
        case WiFiState::CONNECTING:
            g_display.displayMessage(String("Connecting to WiFi..."));
            break;
        case WiFiState::STA_CONNECTED:
            g_display.displayMessage(String("IP: ") + g_wifiManager.getLocalIP());
            break;
        case WiFiState::STA_RECONNECTING:
            g_display.displayMessage(String("WiFi Lost..."));
            break;
        case WiFiState::AP_FALLBACK:
            g_display.displayMessage(String("WiFi Failed"));
            break;
    }
}

static void queueWiFiStateMessage(WiFiState state)
{
    // During startup progress, the coordinator drives display messages
    // so we intercept WiFi events to advance the progress sequence
    if (g_startupPhase != StartupPhase::IDLE &&
        g_startupPhase != StartupPhase::COMPLETE)
    {
        switch (state)
        {
            case WiFiState::STA_CONNECTED:
                enterPhase(StartupPhase::WIFI_CONNECTED);
                queueDisplayMessage(String("WiFi Connected ✓"), 2000);
                LOG_I("Main", "Startup: WiFi connected");
                return;
            case WiFiState::AP_FALLBACK:
                enterPhase(StartupPhase::WIFI_FAILED);
                queueDisplayMessage(String("WiFi Failed - Reopen Setup"));
                LOG_I("Main", "Startup: WiFi failed, returning to setup");
                return;
            case WiFiState::CONNECTING:
            case WiFiState::STA_RECONNECTING:
                // Already showing connecting message — don't overwrite
                return;
            default:
                break;
        }
    }

    // Normal (non-startup) WiFi state messages
    switch (state)
    {
        case WiFiState::AP_SETUP:
            queueDisplayMessage(String("Setup Mode"));
            break;
        case WiFiState::CONNECTING:
            queueDisplayMessage(String("Connecting to WiFi..."));
            break;
        case WiFiState::STA_CONNECTED:
            queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 3000);
            break;
        case WiFiState::STA_RECONNECTING:
            queueDisplayMessage(String("WiFi Lost..."));
            break;
        case WiFiState::AP_FALLBACK:
            queueDisplayMessage(String("WiFi Failed"));
            break;
    }
}

static bool hardwareConfigChanged(const HardwareConfig &lhs, const HardwareConfig &rhs)
{
    return lhs.tiles_x != rhs.tiles_x ||
           lhs.tiles_y != rhs.tiles_y ||
           lhs.tile_pixels != rhs.tile_pixels ||
           lhs.display_pin != rhs.display_pin ||
           lhs.origin_corner != rhs.origin_corner ||
           lhs.scan_dir != rhs.scan_dir ||
           lhs.zigzag != rhs.zigzag;
}

static bool hardwareGeometryChanged(const HardwareConfig &lhs, const HardwareConfig &rhs)
{
    return lhs.tiles_x != rhs.tiles_x ||
           lhs.tiles_y != rhs.tiles_y ||
           lhs.tile_pixels != rhs.tile_pixels ||
           lhs.display_pin != rhs.display_pin;
}

static bool hardwareMappingChanged(const HardwareConfig &lhs, const HardwareConfig &rhs)
{
    return lhs.origin_corner != rhs.origin_corner ||
           lhs.scan_dir != rhs.scan_dir ||
           lhs.zigzag != rhs.zigzag;
}

// --- Display task (Task 2) ---

void displayTask(void *pvParameters)
{
    LOG_I("DisplayTask", "Display task started");

    // Read initial config as local copies
    DisplayConfig localDisp = ConfigManager::getDisplay();
    HardwareConfig localHw = ConfigManager::getHardware();
    TimingConfig localTiming = ConfigManager::getTiming();

    // Flight cycling state (owned by display task)
    size_t currentFlightIndex = 0;
    unsigned long lastCycleMs = millis();
    bool statusMessageVisible = false;
    unsigned long statusMessageUntilMs = 0;

    // Subscribe to task watchdog
    esp_task_wdt_add(NULL);

    for (;;)
    {
        // Check for config changes (atomic flag from Core 1)
        if (g_configChanged.exchange(false))
        {
            DisplayConfig newDisp = ConfigManager::getDisplay();
            HardwareConfig newHw = ConfigManager::getHardware();
            TimingConfig newTiming = ConfigManager::getTiming();

            if (hardwareConfigChanged(localHw, newHw))
            {
                if (hardwareGeometryChanged(localHw, newHw))
                {
                    localHw = newHw;
                    LOG_I("DisplayTask", "Display geometry changed; reboot required to apply layout");
                    // No automatic restart — dashboard sends POST /api/reboot after apply
                }
                else if (hardwareMappingChanged(localHw, newHw))
                {
                    localHw = newHw;
                    if (g_display.reconfigureFromConfig())
                    {
                        LOG_I("DisplayTask", "Applied matrix mapping change without reboot");
                    }
                    else
                    {
                        LOG_E("DisplayTask", "Failed to reconfigure matrix mapping at runtime");
                    }
                }
            }

            localDisp = newDisp;
            localTiming = newTiming;
            g_display.updateBrightness(localDisp.brightness);
            LOG_I("DisplayTask", "Config change detected, display settings updated");
        }

        // Calibration mode (Story 4.2): render test pattern instead of flights
        // Checked before status messages so test patterns override persistent banners
        if (g_display.isCalibrationMode())
        {
            statusMessageVisible = false;
            g_display.renderCalibrationPattern();
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // Positioning mode: render panel position guide instead of flights
        if (g_display.isPositioningMode())
        {
            statusMessageVisible = false;
            g_display.renderPositioningPattern();
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        DisplayStatusMessage statusMessage = {};
        if (g_displayMessageQueue != nullptr &&
            xQueueReceive(g_displayMessageQueue, &statusMessage, 0) == pdTRUE)
        {
            statusMessageVisible = true;
            statusMessageUntilMs = statusMessage.durationMs == 0
                ? 0
                : millis() + statusMessage.durationMs;
            g_display.displayMessage(String(statusMessage.text));
        }

        if (statusMessageVisible)
        {
            if (statusMessageUntilMs == 0 || millis() < statusMessageUntilMs)
            {
                esp_task_wdt_reset();
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            statusMessageVisible = false;
        }

        // Read latest flight data from queue (peek, don't remove)
        FlightDisplayData *ptr = nullptr;
        if (g_flightQueue != nullptr && xQueuePeek(g_flightQueue, &ptr, 0) == pdTRUE && ptr != nullptr)
        {
            const auto &flights = ptr->flights;

            if (!flights.empty())
            {
                const unsigned long now = millis();
                const unsigned long cycleMs = localTiming.display_cycle * 1000UL;

                if (flights.size() > 1)
                {
                    if (now - lastCycleMs >= cycleMs)
                    {
                        lastCycleMs = now;
                        currentFlightIndex = (currentFlightIndex + 1) % flights.size();
                    }
                }
                else
                {
                    currentFlightIndex = 0;
                }

                g_display.renderFlight(flights, currentFlightIndex % flights.size());
            }
            else
            {
                g_display.showLoading();
            }
        }

        // Log stack high watermark at verbose level for tuning
#if LOG_LEVEL >= 3
        static unsigned long lastStackLogMs = 0;
        const unsigned long nowMs = millis();
        if (nowMs - lastStackLogMs >= 30000UL)
        {
            lastStackLogMs = nowMs;
            Serial.println("[DisplayTask] Stack HWM: " + String(uxTaskGetStackHighWaterMark(NULL)) + " bytes");
        }
#endif

        // Reset watchdog and yield (~20fps frame rate)
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --- Partition validation helper (Story fn-1.1) ---

void validatePartitionLayout() {
    Serial.println("[Main] Validating partition layout...");

    // Expected partition sizes from custom_partitions.csv (Story fn-1.1)
    // IMPORTANT: If you modify custom_partitions.csv, update these constants
    const size_t EXPECTED_APP_SIZE = 0x180000;   // app0/app1: 1.5MB
    const size_t EXPECTED_SPIFFS_SIZE = 0xF0000; // spiffs: 960KB

    // Validate running app partition
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running) {
        if (running->size == EXPECTED_APP_SIZE) {
            Serial.printf("[Main] App partition: %s, size 0x%x (correct)\n", running->label, running->size);
        } else {
            Serial.printf("[Main] WARNING: App partition size mismatch: expected 0x%x, got 0x%x\n",
                  EXPECTED_APP_SIZE, running->size);
            Serial.println("WARNING: Partition table may not match firmware expectations!");
            Serial.println("Reflash with new partition table via USB if OTA updates fail.");
        }
    } else {
        Serial.println("[Main] WARNING: Could not determine running partition");
    }

    // Validate LittleFS partition
    const esp_partition_t* littlefs = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
    if (littlefs) {
        if (littlefs->size == EXPECTED_SPIFFS_SIZE) {
            Serial.printf("[Main] LittleFS partition: size 0x%x (correct)\n", littlefs->size);
        } else {
            Serial.printf("[Main] WARNING: LittleFS partition size mismatch: expected 0x%x, got 0x%x\n",
                  EXPECTED_SPIFFS_SIZE, littlefs->size);
        }
    } else {
        Serial.println("[Main] WARNING: Could not find LittleFS partition");
    }
}

// --- setup() (Task 3) ---

void setup()
{
    Serial.begin(115200);
    delay(200);

    // Log firmware version at boot (Story fn-1.1)
    Serial.println();
    Serial.println("=================================");
    Serial.printf("FlightWall Firmware v%s\n", FW_VERSION);
    Serial.println("=================================");

    // Validate partition layout matches expectations (Story fn-1.1)
    validatePartitionLayout();

    // Mount LittleFS without auto-format to prevent silent data loss
    if (!LittleFS.begin(false)) {
        LOG_E("Main", "LittleFS mount failed - filesystem may be corrupted or unformatted");
        Serial.println("WARNING: LittleFS mount failed!");
        Serial.println("To format filesystem, reflash with: pio run -t uploadfs");
        // Continue boot - device will function but web assets/logos unavailable
    } else {
        LOG_I("Main", "LittleFS mounted successfully");
    }

    ConfigManager::init();
    SystemStatus::init();

    // Logo manager initialization (Story 3.2) — after LittleFS mount
    if (!LogoManager::init()) {
        LOG_E("Main", "LogoManager init failed — fallback sprites will be used");
    }

    // Compute initial layout from hardware config (Story 3.1)
    g_layout = LayoutEngine::compute(ConfigManager::getHardware());
    LOG_I("Main", "Layout computed");
    Serial.println("[Main] Layout: " + String(g_layout.matrixWidth) + "x" + String(g_layout.matrixHeight) + " mode=" + g_layout.mode);

    // Display initialization before task creation (setup runs on Core 1)
    g_display.initialize();
    g_display.showLoading();

    // Create flight data queue (length 1, stores a pointer)
    g_flightQueue = xQueueCreate(1, sizeof(FlightDisplayData *));
    if (g_flightQueue == nullptr) {
        LOG_E("Main", "Failed to create flight data queue");
    } else {
        LOG_I("Main", "Flight data queue created");
    }

    g_displayMessageQueue = xQueueCreate(1, sizeof(DisplayStatusMessage));
    if (g_displayMessageQueue == nullptr) {
        LOG_E("Main", "Failed to create display message queue");
    } else {
        LOG_I("Main", "Display message queue created");
    }

    // Register config change callback (sets atomic flag for display task + recompute layout)
    ConfigManager::onChange([]() {
        g_configChanged.store(true);
        g_layout = LayoutEngine::compute(ConfigManager::getHardware());
    });
    LOG_I("Main", "Config change callback registered");

    // Create display task pinned to Core 0 (priority 1, 8KB stack)
    BaseType_t taskResult = xTaskCreatePinnedToCore(
        displayTask,
        "display",
        8192,
        NULL,
        1,
        NULL,
        0
    );
    if (taskResult == pdPASS) {
        LOG_I("Main", "Display task created on Core 0 (8KB stack, priority 1)");
    } else {
        LOG_E("Main", "Failed to create display task");
    }

    // Boot button GPIO sampling — detect held-low for forced AP setup
    // Configure as input with pull-up (BOOT button is active-low)
    pinMode(BOARD_BOOT_BUTTON_GPIO, INPUT_PULLUP);
    delay(50); // Allow pull-up to settle
    if (digitalRead(BOARD_BOOT_BUTTON_GPIO) == LOW) {
        // Button is held — sample for ~500ms to distinguish from noise
        unsigned long holdStart = millis();
        bool held = true;
        while (millis() - holdStart < 500) {
            if (digitalRead(BOARD_BOOT_BUTTON_GPIO) == HIGH) {
                held = false;
                break;
            }
            delay(10);
        }
        if (held) {
            g_forcedApSetup = true;
            LOG_I("Main", "Boot button held — forcing AP setup mode");
        }
    }

    // WiFiManager initialization (non-blocking, event-driven)
    // Per architecture Decision 5: WiFiManager before WebPortal
    g_wifiManager.init(g_forcedApSetup);

    // Activate startup progress coordinator when WiFi credentials exist
    // (i.e. this is a post-setup boot, not first-time AP mode)
    if (g_forcedApSetup)
    {
        // Forced AP via boot button — show distinct message
        queueDisplayMessage(String("Setup Mode — Forced"));
        LOG_I("Main", "Displaying forced setup message");
    }
    else if (g_wifiManager.getState() == WiFiState::CONNECTING)
    {
        enterPhase(StartupPhase::CONNECTING_WIFI);
        queueDisplayMessage(String("Connecting to WiFi..."));
        LOG_I("Main", "Startup progress active: connecting to WiFi");
    }
    else
    {
        showInitialWiFiMessage();
    }

    g_wifiManager.onStateChange([](WiFiState oldState, WiFiState newState) {
        (void)oldState;
        queueWiFiStateMessage(newState);
    });

    // WebPortal initialization — register routes then start server
    // Server serves in both AP and STA modes; GET / routes dynamically based on WiFi state
    g_webPortal.init(g_webServer, g_wifiManager);

    // Register calibration callback (Story 4.2) — toggles gradient test pattern
    g_webPortal.onCalibration([](bool enabled) {
        g_display.setCalibrationMode(enabled);
        if (enabled) {
            LOG_I("Main", "Calibration mode started");
        } else {
            LOG_I("Main", "Calibration mode stopped");
        }
    });

    // Register positioning callback — toggles panel positioning guide
    g_webPortal.onPositioning([](bool enabled) {
        g_display.setPositioningMode(enabled);
        if (enabled) {
            LOG_I("Main", "Positioning mode started");
        } else {
            LOG_I("Main", "Positioning mode stopped");
        }
    });

    // Register reboot callback so "Saving config..." shows on LED before restart
    g_webPortal.onReboot([]() {
        enterPhase(StartupPhase::SAVING_CONFIG);
        queueDisplayMessage(String("Saving config..."));
        LOG_I("Main", "Startup: saving config before reboot");
    });

    g_webPortal.begin();
    LOG_I("Main", "WebPortal started");

    g_fetcher = new FlightDataFetcher(&g_openSky, &g_aeroApi);
    if (g_fetcher == nullptr)
    {
        LOG_E("Main", "Failed to create FlightDataFetcher");
    }
    LOG_I("Main", "Setup complete");
}

// --- Startup progress coordinator tick (Story 1.8) ---
// Drives the progress state machine from loop() on Core 1.
// Each phase transition waits for a real event (WiFi connect, fetch complete)
// rather than blasting the queue. Returns true if a first-fetch should run now.

static bool tickStartupProgress()
{
    if (g_startupPhase == StartupPhase::IDLE ||
        g_startupPhase == StartupPhase::COMPLETE)
    {
        return false;
    }

    const unsigned long elapsed = millis() - g_phaseEnteredMs;

    switch (g_startupPhase)
    {
        case StartupPhase::SAVING_CONFIG:
            // Display persists until reboot — no transition needed here
            break;

        case StartupPhase::CONNECTING_WIFI:
            // Waiting for WiFi callback to advance to WIFI_CONNECTED or WIFI_FAILED
            break;

        case StartupPhase::WIFI_CONNECTED:
            // Show "WiFi Connected ✓" briefly, then IP, then move to auth
            if (elapsed < 2000)
            {
                // Still showing "WiFi Connected"
            }
            else if (elapsed < 4000)
            {
                // Show IP address for discovery (use phase time to gate, not a flag)
                if (elapsed >= 2000 && elapsed < 2100)
                {
                    queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 2000);
                }
            }
            else
            {
                // Advance to authentication phase
                enterPhase(StartupPhase::AUTHENTICATING);
                queueDisplayMessage(String("Authenticating APIs..."));
                LOG_I("Main", "Startup: authenticating APIs");
            }
            break;

        case StartupPhase::WIFI_FAILED:
            // Terminal state — device returns to AP/setup mode
            // WiFiManager handles the AP fallback; message stays visible
            if (elapsed >= 5000)
            {
                // After showing failure message, return to idle
                // (device is now in AP mode, user must re-run wizard)
                enterPhase(StartupPhase::IDLE);
            }
            break;

        case StartupPhase::AUTHENTICATING:
            // Hold the authentication message briefly before starting the first fetch.
            if (elapsed >= AUTHENTICATING_DISPLAY_MS)
            {
                return true;
            }
            break;

        case StartupPhase::FETCHING_FLIGHTS:
            // Waiting for fetch to complete — transition driven by loop()
            break;

        default:
            break;
    }

    return false;
}

// --- loop() (Task 4) ---

void loop()
{
    ConfigManager::tick();
    g_wifiManager.tick();

    // Boot button short-press detection (millis debounce, no ISR)
    // Only active during normal operation — skip if forced AP setup
    if (!g_forcedApSetup) {
        bool raw = digitalRead(BOARD_BOOT_BUTTON_GPIO);
        unsigned long now_btn = millis();

        if (raw != g_buttonLastState && (now_btn - g_buttonLastChangeMs >= BUTTON_DEBOUNCE_MS)) {
            g_buttonLastChangeMs = now_btn;

            if (raw == LOW) {
                // Button pressed (active-low)
                g_buttonPressStartMs = now_btn;
            } else {
                // Button released — check for short press
                unsigned long pressDuration = now_btn - g_buttonPressStartMs;
                if (pressDuration > 0 && pressDuration <= BUTTON_SHORT_PRESS_MAX_MS) {
                    // Short press detected — show IP or status message
                    if (g_wifiManager.getState() == WiFiState::STA_CONNECTED) {
                        queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 5000);
                    } else {
                        queueDisplayMessage(String("No IP — Setup mode"), 5000);
                    }
                    LOG_I("Main", "Short press: showing IP/status");
                }
                // Long presses are ignored during normal operation
            }

            g_buttonLastState = raw;
        }
    }

    if (g_fetcher == nullptr)
    {
        delay(1000);
        return;
    }

    // Drive startup progress coordinator
    bool triggerFirstFetch = tickStartupProgress();

    // Determine if we should fetch now
    const unsigned long intervalMs = ConfigManager::getTiming().fetch_interval * 1000UL;
    const unsigned long now = millis();
    bool normalFetchDue = (now - g_lastFetchMs >= intervalMs);

    // First-fetch one-shot: when startup coordinator says go, fetch immediately
    // regardless of the normal interval timer
    if (triggerFirstFetch && !g_firstFetchDone)
    {
        normalFetchDue = true;
        LOG_I("Main", "Startup: triggering first fetch immediately");
    }

    if (normalFetchDue)
    {
        g_lastFetchMs = now;

        // Show "Fetching flights..." during startup progress
        if (g_startupPhase == StartupPhase::AUTHENTICATING ||
            g_startupPhase == StartupPhase::FETCHING_FLIGHTS)
        {
            if (g_startupPhase == StartupPhase::AUTHENTICATING)
            {
                enterPhase(StartupPhase::FETCHING_FLIGHTS);
                queueDisplayMessage(String("Fetching flights..."));
                LOG_I("Main", "Startup: fetching flights");
            }
        }

        std::vector<StateVector> states;
        std::vector<FlightInfo> flights;
        size_t enriched = g_fetcher->fetchFlights(states, flights);

        // Update flight stats snapshot for /api/status (Story 2.4)
        g_statsFetchesSinceBoot.fetch_add(1);
        g_statsLastFetchMs.store(millis());
        g_statsStateVectors.store(static_cast<uint16_t>(states.size()));
        g_statsEnrichedFlights.store(static_cast<uint16_t>(enriched));
        // g_statsLogosMatched stays 0 until Epic 3

#if LOG_LEVEL >= 2
        Serial.println("[Main] Fetch: " + String((int)states.size()) + " state vectors, " + String((int)enriched) + " enriched flights");
#endif

#if LOG_LEVEL >= 3
        for (const auto &s : states)
        {
            Serial.println("[Main] " + s.callsign + " @ " + String(s.distance_km, 1) + "km bearing " + String(s.bearing_deg, 1));
        }

        for (const auto &f : flights)
        {
            Serial.println("[Main] Flight: " + f.ident + " " + f.airline_display_name_full + " " + f.origin.code_icao + "->" + f.destination.code_icao);
        }
#endif

        // Write flight data to double buffer and push pointer to queue
        g_flightBuf[g_writeBuf].flights = flights;
        FlightDisplayData *ptr = &g_flightBuf[g_writeBuf];
        if (g_flightQueue != nullptr)
        {
            xQueueOverwrite(g_flightQueue, &ptr);
        }
        g_writeBuf ^= 1; // swap buffer for next write

        // Complete startup progress after first fetch
        if (!g_firstFetchDone &&
            (g_startupPhase == StartupPhase::FETCHING_FLIGHTS ||
             g_startupPhase == StartupPhase::AUTHENTICATING))
        {
            g_firstFetchDone = true;
            enterPhase(StartupPhase::COMPLETE);

            if (!flights.empty())
            {
                // Clear progress message so display task renders flight data
                queueDisplayMessage(String(""), 1);
                LOG_I("Main", "Startup complete: first flight data ready");
            }
            else
            {
                // No flights available — fall back to normal loading screen
                // (display task will show showLoading() when flight list is empty)
                queueDisplayMessage(String(""), 1);
                LOG_I("Main", "Startup complete: no flights yet, showing loading");
            }
        }
    }
}

#endif // PIO_UNIT_TESTING


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
<var name="instructions">embedded context</var>
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
<var name="story_file" file_id="e97afb18">embedded in prompt, file id: e97afb18</var>
<var name="story_id">fn-1.4</var>
<var name="story_key">fn-1-4-ota-self-check-and-rollback-detection</var>
<var name="story_num">4</var>
<var name="story_title">ota-self-check-and-rollback-detection</var>
<var name="template">embedded context</var>
<var name="timestamp">20260412_145121</var>
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
      <action>Verify alignment with /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/planning-artifacts/architecture.md patterns:
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