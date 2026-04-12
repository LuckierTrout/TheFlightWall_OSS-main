<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 3 -->
<!-- Phase: code-review -->
<!-- Timestamp: 20260412T183358Z -->
<compiled-workflow>
<mission><![CDATA[

Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.

Target: Story fn-1.3 - ota-upload-endpoint
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

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Implementation complete - OTA upload endpoint with streaming, validation, and error handling | Claude |

### File List

- `firmware/adapters/WebPortal.cpp` (MODIFIED) - Added OTA upload route, state struct, helpers


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
#include <esp_ota_ops.h>
#include <vector>
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"
#include "core/LogoManager.h"
#include "utils/Log.h"

// Defined in main.cpp — provides thread-safe flight stats for the health page
extern FlightStatsSnapshot getFlightStatsSnapshot();

#include "core/LayoutEngine.h"

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

OTAUploadState* findOTAUpload(AsyncWebServerRequest* request) {
    for (auto& u : g_otaUploads) {
        if (u.request == request) return &u;
    }
    return nullptr;
}

void clearOTAUpload(AsyncWebServerRequest* request) {
    for (auto it = g_otaUploads.begin(); it != g_otaUploads.end(); ++it) {
        if (it->request == request) {
            // CRITICAL: abort in-progress update on cleanup
            if (it->started) {
                Update.abort();
                LOG_I("OTA", "Upload aborted during cleanup");
            }
            g_otaUploads.erase(it);
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
                PendingRequestBody pending{request, String()};
                pending.body.reserve(total);
                g_pendingBodies.push_back(pending);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }

            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }

            for (size_t i = 0; i < len; ++i) {
                pending->body += static_cast<char>(data[i]);
            }

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
                root["code"] = (state && state->errorCode.length()) ? state->errorCode : "UPLOAD_ERROR";
                String output;
                serializeJson(doc, output);
                clearOTAUpload(request);
                request->send(400, "application/json", output);
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
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "No OTA partition found");
                    return;
                }

                // Begin update with partition size (NOT Content-Length per AR18)
                if (!Update.begin(partition->size)) {
                    state.valid = false;
                    state.error = "Could not begin OTA update";
                    state.errorCode = "BEGIN_FAILED";
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "Update.begin() failed");
                    return;
                }

                state.started = true;
                g_otaUploads.push_back(state);
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
                LOG_I("OTA", "Update finalized successfully");
            }
        }
    );

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


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic fn-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #1 Automation Missing**: Binary size logging was manual, not automated in build process | Created `check_size.py` PlatformIO pre-action script that runs on every build, logs binary size, warns at 1.3MB threshold, fails at 1.5MB limit. Added `extra_scripts = pre:check_size.py` to platformio.ini. |
| critical | Silent Data Loss Risk**: LittleFS.begin(true) auto-formats on mount failure without notification | Changed to `LittleFS.begin(false)` with explicit error logging and user-visible instructions to reflash filesystem. Device continues boot but warns user of unavailable web assets/logos. |
| high | Missing Partition Runtime Validation**: No verification that running firmware matches expected partition layout | Added `validatePartitionLayout()` function that checks running app partition size (0x180000) and LittleFS partition size (0xF0000) against expectations. Logs warnings if mismatches detected. Called during setup before LittleFS mount. |

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Silent Exit in check_size.py if Binary Missing | Added explicit error logging when binary is missing |
| low | Magic Numbers for Timing/Thresholds | Named constants already exist for some values (AUTHENTICATING_DISPLAY_MS, BUTTON_DEBOUNCE_MS). Additional refactoring would require broader changes. |
| low | Interface Segregation - WiFiManager Callback | Changed to C++ comment syntax to clarify intent |

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Silent Exit in check_size.py When Binary Missing | Added explicit error logging and `env.Exit(1)` when binary is missing |
| high | Magic Numbers for Partition Sizes - No Cross-Reference | Added cross-reference comments in all 3 files to alert developers when updating partition sizes |
| medium | Partition Table Gap Not Documented | Added comment documenting reserved gap |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing `getSchedule()` implementation | Added `ConfigManager::getSchedule()` method at line 536 following existing getter pattern with ConfigLockGuard for thread safety. Method returns copy of `_schedule` struct. |
| critical | Schedule keys missing from `dumpSettingsJson()` API | Added `snapshot.schedule = _schedule` to snapshot capture at line 469 and added all 5 schedule keys to JSON output at lines 507-511. GET /api/settings now returns 27 total keys (22 existing + 5 new). |
| critical | OTA and NTP subsystems not added to SystemStatus | Added `OTA` and `NTP` to Subsystem enum, updated `SUBSYSTEM_COUNT` from 6 to 8, and added "ota" and "ntp" cases to `subsystemName()` switch. Also fixed stale comment "existing six" → "subsystems". |
| high | Required unit tests missing from test suite | Added 5 new test functions: `test_defaults_schedule()`, `test_nvs_write_read_roundtrip_schedule()`, `test_apply_json_schedule_hot_reload()`, `test_apply_json_schedule_validation()`, and `test_system_status_ota_ntp()`. All tests integrated into `setup()` test runner. |
| low | Stale comment in SystemStatus.cpp | Updated comment from "existing six" to "subsystems" to reflect new count of 8. |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Integer Overflow in `sched_enabled` Validation | Changed from `value.as<uint8_t>()` (wraps 256→0) to validating as `unsigned int` before cast. Now properly rejects values >1. Added type check `value.is<unsigned int>()`. |
| critical | Missing Validation for `sched_dim_brt` | Added bounds checking (0-255) and type validation. Previously accepted any value without validation, violating story requirements. |
| critical | Test Suite Contradicts Implementation | Renamed `test_apply_json_ignores_unknown_keys` → `test_apply_json_rejects_unknown_keys` and corrected assertions. applyJson uses all-or-nothing validation, not partial success. |
| high | Missing Test Coverage for AC #4 | Added `test_dump_settings_json_includes_schedule()` — verifies all 5 schedule keys present in JSON and total key count = 27. |
| high | Validation Test Coverage Gaps | Extended `test_apply_json_schedule_validation()` with 2 additional test cases: `sched_dim_brt > 255` rejection and `sched_enabled = 256` overflow rejection. |
| dismissed | `/api/settings` exposes secrets (wifi_password, API keys) in plaintext | FALSE POSITIVE: Pre-existing design, not introduced by this story. Requires separate security story to implement credential masking. Story scope was schedule keys + SystemStatus subsystems only. |
| dismissed | ConfigSnapshot heap allocation overhead in applyJson | FALSE POSITIVE: Necessary for atomic semantics — applyJson must validate all keys before committing any changes. Snapshot pattern is intentional design. |
| dismissed | SystemStatus mutex timeout fallback is unsafe | FALSE POSITIVE: Pre-existing pattern across SystemStatus implementation (lines 35-44, 53-58, 65-73). Requires broader refactor outside story scope. This story only added OTA/NTP subsystems. |
| dismissed | SystemStatus tight coupling to WiFi, LittleFS, ConfigManager | FALSE POSITIVE: Pre-existing architecture in `toExtendedJson()` method. Not introduced by this story — story only added 2 subsystems to existing enum. |
| dismissed | Hardcoded NVS namespace string | FALSE POSITIVE: Pre-existing pattern, not story scope. NVS namespace was defined before this story. |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Test baseline claims 27 keys but implementation has 29 | Updated test assertion from 27 to 29 to match actual implementation |
| critical | Missing negative number validation for schedule time fields | Added type validation with `value.is<int>()` and signed bounds checking before unsigned cast |
| critical | Missing type validation for timezone string | Added `value.is<const char*>()` type check before string conversion |
| high | Missing test coverage for AC #4 - JSON dump includes all schedule keys | Corrected key count assertion from 27 to 29 |
| high | Validation test coverage gaps | Already present in current code (lines 373-382) - FALSE POSITIVE on reviewer's part, but validation logic in implementation was incomplete (see fixes #6, #7) |
| high | Integer overflow in sched_enabled validation | Already has type check `value.is<unsigned int>()` - validation is correct. Reviewing again...actually the test exists to verify 256 is rejected (line 380). This is working correctly. |
| high | Missing validation for sched_dim_brt | Code already has type check and >255 rejection (lines 163-167) after Round 2 fixes. Verified correct. |
| medium | Test suite contradicts implementation (unknown key handling) | Test already renamed to `test_apply_json_rejects_unknown_keys` with correct assertions after Round 2 fixes |


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="System architecture for review context" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="description">Perform an ADVERSARIAL Senior Developer code review that finds 3-10 specific problems in every story. Challenges everything: code quality, test coverage, architecture compliance, security, performance. NEVER accepts `looks good` - must find minimum issues and can auto-fix with user approval.</var>
<var name="document_output_language">English</var>
<var name="document_project_file" description="Brownfield project documentation (optional)" load_strategy="INDEX_GUIDED">_bmad-output/planning-artifacts/index.md</var>
<var name="epic_num">fn-1</var>
<var name="epics_file" description="Epic containing story being reviewed" load_strategy="SELECTIVE_LOAD" token_approx="13622">_bmad-output/planning-artifacts/epics-delight.md</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="instructions">embedded context</var>
<var name="name">code-review</var>
<var name="output_folder">_bmad-output/implementation-artifacts</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context">{project-root}/docs/project-context.md</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_dir">_bmad-output/implementation-artifacts/stories</var>
<var name="story_file" file_id="ab213a12">embedded in prompt, file id: ab213a12</var>
<var name="story_id">fn-1.3</var>
<var name="story_key">fn-1-3-ota-upload-endpoint</var>
<var name="story_num">3</var>
<var name="story_title">ota-upload-endpoint</var>
<var name="template">False</var>
<var name="timestamp">20260412_1433</var>
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

  <!-- Step 1 removed: file discovery handled by compiler -->

  <step n="1" goal="Execute adversarial review">
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

  <step n="2" goal="Systematic Code Quality Assessment">
    <critical>🎯 RUTHLESS CODE QUALITY GATE: Systematic review against senior developer standards!</critical>

    <substep n="3a" title="SOLID Principles Violations">
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

    <substep n="3b" title="Hidden Bugs Detection">
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

    <substep n="3c" title="Abstraction Level Analysis">
      <action>Analyze abstraction appropriateness:
        - **Over-engineering**: Unnecessary patterns, premature abstraction
        - **Under-engineering**: Missing abstractions, code duplication
        - **Wrong patterns**: Pattern misuse, cargo cult programming
        - **Boundary breaches**: Layer violations, leaky abstractions
      </action>
      <action>Document each issue with reasoning</action>
      <action>Store as {{abstraction_issues}}</action>
    </substep>

    <substep n="3d" title="Lying Tests Detection">
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

    <substep n="3e" title="Performance Footguns">
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

    <substep n="3f" title="Tech Debt Bombs">
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

    <substep n="3g" title="PEP 8 and Pythonic Compliance">
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

    <substep n="3h" title="Type Safety Analysis">
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

    <substep n="3i" title="Security Vulnerability Scan">
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

    <substep n="3j" title="Calculate Evidence Score">
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

  <step n="3" goal="Present findings">
    <critical>OUTPUT MARKERS REQUIRED: Your code review report MUST start with the marker &lt;!-- CODE_REVIEW_REPORT_START --&gt; on its own line BEFORE the report content, and MUST end with the marker &lt;!-- CODE_REVIEW_REPORT_END --&gt; on its own line AFTER the final line. The orchestrator extracts ONLY content between these markers. Any text outside the markers (thinking, commentary) will be discarded.</critical>

    <action>Generate the code review report using the output block template below as FORMAT GUIDE</action>
    <action>Replace all {{placeholders}} with actual values from your analysis</action>
    <action>Output the complete report to stdout</action>
    <action>Do NOT save to any file - orchestrator handles persistence</action>

    <o>
<!-- CODE_REVIEW_REPORT_START -->**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md

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