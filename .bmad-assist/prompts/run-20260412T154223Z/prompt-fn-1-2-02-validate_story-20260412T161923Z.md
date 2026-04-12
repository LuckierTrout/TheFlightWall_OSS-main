<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 2 -->
<!-- Phase: validate-story -->
<!-- Timestamp: 20260412T161923Z -->
<compiled-workflow>
<mission><![CDATA[

Adversarial Story Validation

Target: Story fn-1.2 - configmanager-expansion-schedule-keys-and-systemstatus

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

Status: ready-for-dev

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

- [ ] **Task 1: Add ScheduleConfig struct to ConfigManager.h** (AC: #1)
  - [ ] Add `struct ScheduleConfig` with fields: `String timezone`, `uint8_t sched_enabled`, `uint16_t sched_dim_start`, `uint16_t sched_dim_end`, `uint8_t sched_dim_brt`
  - [ ] Document NVS key abbreviations in header comments (sched_dim_start, sched_dim_end, sched_dim_brt follow 15-char limit)
  - [ ] Add static member `_schedule` and getter `getSchedule()` declaration

- [ ] **Task 2: Implement schedule defaults and NVS loading** (AC: #1, #2)
  - [ ] Add `_schedule` static member initialization in ConfigManager.cpp
  - [ ] Add schedule defaults in `loadDefaults()`: timezone="UTC0", sched_enabled=0, sched_dim_start=1380, sched_dim_end=420, sched_dim_brt=10
  - [ ] Add schedule NVS loading in `loadFromNvs()` using exact key names
  - [ ] Add schedule NVS persistence in `persistToNvs()`

- [ ] **Task 3: Implement schedule key handling in applyJson** (AC: #3)
  - [ ] Add schedule key handlers in `updateConfigValue()` with validation:
    - `timezone`: String, max 40 chars (POSIX timezone)
    - `sched_enabled`: uint8, valid values 0 or 1
    - `sched_dim_start`: uint16, valid 0-1439 (minutes since midnight)
    - `sched_dim_end`: uint16, valid 0-1439
    - `sched_dim_brt`: uint8, valid 0-255
  - [ ] Verify all 5 schedule keys are NOT in REBOOT_KEYS array (hot-reload path)
  - [ ] Add ConfigLockGuard protection for _schedule read/write

- [ ] **Task 4: Add schedule keys to JSON dump** (AC: #4)
  - [ ] Add all 5 schedule keys to `dumpSettingsJson()` output
  - [ ] Verify GET /api/settings returns 27 total keys (existing 22 + 5 new)

- [ ] **Task 5: Implement getSchedule() getter** (AC: #1, #2)
  - [ ] Add `getSchedule()` method following existing getter pattern with ConfigLockGuard
  - [ ] Add to ConfigSnapshot struct if using snapshot pattern

- [ ] **Task 6: Extend SystemStatus with OTA and NTP subsystems** (AC: #5)
  - [ ] Add `OTA` and `NTP` to Subsystem enum in SystemStatus.h
  - [ ] Update SUBSYSTEM_COUNT from 6 to 8
  - [ ] Add "ota" and "ntp" cases to `subsystemName()` switch
  - [ ] Verify `_statuses` array size matches new count

- [ ] **Task 7: Add unit tests for new functionality** (AC: #1-5)
  - [ ] Add test_defaults_schedule() — verify default values
  - [ ] Add test_nvs_write_read_roundtrip_schedule() — NVS persistence
  - [ ] Add test_apply_json_schedule_hot_reload() — verify hot-reload path
  - [ ] Add test_apply_json_schedule_validation() — reject invalid values
  - [ ] Add test_system_status_ota_ntp() — verify new subsystems work

- [ ] **Task 8: Build and verify** (AC: #1-5)
  - [ ] Run `pio run` — verify clean build with no errors
  - [ ] Run `pio test` (on-device) — verify all existing + new tests pass
  - [ ] Verify binary size remains under 1.5MB limit

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

{{agent_model_name_version}}

### Debug Log References

### Completion Notes List

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |

### File List

- `firmware/core/ConfigManager.h` (MODIFIED)
- `firmware/core/ConfigManager.cpp` (MODIFIED)
- `firmware/core/SystemStatus.h` (MODIFIED)
- `firmware/core/SystemStatus.cpp` (MODIFIED)
- `firmware/test/test_config_manager/test_main.cpp` (MODIFIED)


]]></file>
<file id="fae2bc78" path="firmware/core/ConfigManager.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: Centralized configuration manager with NVS persistence and compile-time fallbacks.
Responsibilities:
- Load settings from NVS on init, falling back to compile-time defaults from config headers.
- Provide category struct getters for thread-safe reads (copy semantics).
- Apply JSON patches via applyJson(), distinguishing reboot vs hot-reload keys.
- Debounce NVS writes for hot-reload keys via schedulePersist().
- Only file that includes config/*.h headers (AR13 compliance).
*/
#include "core/ConfigManager.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "utils/Log.h"

// AR13: Only ConfigManager.cpp includes config headers
#include "config/UserConfiguration.h"
#include "config/WiFiConfiguration.h"
#include "config/TimingConfiguration.h"
#include "config/HardwareConfiguration.h"
#include "config/APIConfiguration.h"

static const char* NVS_NAMESPACE = "flightwall";
static SemaphoreHandle_t g_configMutex = nullptr;

struct ConfigSnapshot {
    DisplayConfig display;
    LocationConfig location;
    HardwareConfig hardware;
    TimingConfig timing;
    NetworkConfig network;
};

class ConfigLockGuard {
public:
    ConfigLockGuard() {
        if (g_configMutex != nullptr) {
            xSemaphoreTake(g_configMutex, portMAX_DELAY);
        }
    }

    ~ConfigLockGuard() {
        if (g_configMutex != nullptr) {
            xSemaphoreGive(g_configMutex);
        }
    }
};

static bool isSupportedDisplayPin(uint8_t pin);
static bool isValidTileConfig(uint8_t tiles_x, uint8_t tiles_y, uint8_t tile_pixels);

// Reject tile configs that would exceed ESP32 RAM (~160KB usable).
// CRGB is 3 bytes per pixel; cap at 16,384 pixels (~49KB).
static bool isValidTileConfig(uint8_t tiles_x, uint8_t tiles_y, uint8_t tile_pixels) {
    if (tiles_x == 0 || tiles_y == 0 || tile_pixels == 0) return false;
    uint32_t total = (uint32_t)tiles_x * tiles_y * tile_pixels * tile_pixels;
    return total <= 16384;
}

static bool updateConfigValue(DisplayConfig& display,
                              LocationConfig& location,
                              HardwareConfig& hardware,
                              TimingConfig& timing,
                              NetworkConfig& network,
                              const char* key,
                              JsonVariantConst value) {
    // Display
    if (strcmp(key, "brightness") == 0)        { display.brightness = value.as<uint8_t>(); return true; }
    if (strcmp(key, "text_color_r") == 0)      { display.text_color_r = value.as<uint8_t>(); return true; }
    if (strcmp(key, "text_color_g") == 0)      { display.text_color_g = value.as<uint8_t>(); return true; }
    if (strcmp(key, "text_color_b") == 0)      { display.text_color_b = value.as<uint8_t>(); return true; }

    // Location
    if (strcmp(key, "center_lat") == 0)        { location.center_lat = value.as<double>(); return true; }
    if (strcmp(key, "center_lon") == 0)        { location.center_lon = value.as<double>(); return true; }
    if (strcmp(key, "radius_km") == 0)         { location.radius_km = value.as<double>(); return true; }

    // Hardware — validate tile values to prevent OOM
    if (strcmp(key, "tiles_x") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (!isValidTileConfig(v, hardware.tiles_y, hardware.tile_pixels)) return false;
        hardware.tiles_x = v; return true;
    }
    if (strcmp(key, "tiles_y") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (!isValidTileConfig(hardware.tiles_x, v, hardware.tile_pixels)) return false;
        hardware.tiles_y = v; return true;
    }
    if (strcmp(key, "tile_pixels") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (!isValidTileConfig(hardware.tiles_x, hardware.tiles_y, v)) return false;
        hardware.tile_pixels = v; return true;
    }
    if (strcmp(key, "display_pin") == 0) {
        const uint8_t pin = value.as<uint8_t>();
        if (!isSupportedDisplayPin(pin)) {
            return false;
        }
        hardware.display_pin = pin;
        return true;
    }
    if (strcmp(key, "origin_corner") == 0)     { hardware.origin_corner = value.as<uint8_t>(); return true; }
    if (strcmp(key, "scan_dir") == 0)          { hardware.scan_dir = value.as<uint8_t>(); return true; }
    if (strcmp(key, "zigzag") == 0)            { hardware.zigzag = value.as<uint8_t>(); return true; }
    if (strcmp(key, "zone_logo_pct") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (v > 99) return false;
        hardware.zone_logo_pct = v; return true;
    }
    if (strcmp(key, "zone_split_pct") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (v > 99) return false;
        hardware.zone_split_pct = v; return true;
    }
    if (strcmp(key, "zone_layout") == 0) {
        uint8_t v = value.as<uint8_t>();
        if (v > 1) return false;
        hardware.zone_layout = v; return true;
    }

    // Timing
    if (strcmp(key, "fetch_interval") == 0)    { timing.fetch_interval = value.as<uint16_t>(); return true; }
    if (strcmp(key, "display_cycle") == 0)     { timing.display_cycle = value.as<uint16_t>(); return true; }

    // Network
    if (strcmp(key, "wifi_ssid") == 0)         { network.wifi_ssid = value.as<String>(); return true; }
    if (strcmp(key, "wifi_password") == 0)     { network.wifi_password = value.as<String>(); return true; }
    if (strcmp(key, "os_client_id") == 0)      { network.opensky_client_id = value.as<String>(); return true; }
    if (strcmp(key, "os_client_sec") == 0)     { network.opensky_client_secret = value.as<String>(); return true; }
    if (strcmp(key, "aeroapi_key") == 0)       { network.aeroapi_key = value.as<String>(); return true; }

    return false;
}

static bool isSupportedDisplayPin(uint8_t pin) {
    switch (pin) {
        case 0:
        case 2:
        case 4:
        case 5:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 21:
        case 22:
        case 23:
        case 25:
        case 26:
        case 27:
        case 32:
        case 33:
            return true;
        default:
            return false;
    }
}

// Reboot-required keys
static const char* REBOOT_KEYS[] = {
    "wifi_ssid", "wifi_password",
    "os_client_id", "os_client_sec",
    "aeroapi_key", "display_pin",
    "tiles_x", "tiles_y", "tile_pixels"
};
static const size_t REBOOT_KEY_COUNT = sizeof(REBOOT_KEYS) / sizeof(REBOOT_KEYS[0]);

// Static member initialization
DisplayConfig ConfigManager::_display = {};
LocationConfig ConfigManager::_location = {};
HardwareConfig ConfigManager::_hardware = {};
TimingConfig ConfigManager::_timing = {};
NetworkConfig ConfigManager::_network = {};
std::vector<std::function<void()>> ConfigManager::_callbacks;
unsigned long ConfigManager::_persistScheduledAt = 0;
bool ConfigManager::_persistPending = false;

void ConfigManager::loadDefaults() {
    _display.brightness = UserConfiguration::DISPLAY_BRIGHTNESS;
    _display.text_color_r = UserConfiguration::TEXT_COLOR_R;
    _display.text_color_g = UserConfiguration::TEXT_COLOR_G;
    _display.text_color_b = UserConfiguration::TEXT_COLOR_B;

    _location.center_lat = UserConfiguration::CENTER_LAT;
    _location.center_lon = UserConfiguration::CENTER_LON;
    _location.radius_km = UserConfiguration::RADIUS_KM;

    _hardware.tiles_x = HardwareConfiguration::DISPLAY_TILES_X;
    _hardware.tiles_y = HardwareConfiguration::DISPLAY_TILES_Y;
    _hardware.tile_pixels = HardwareConfiguration::DISPLAY_TILE_PIXEL_W;
    _hardware.display_pin = HardwareConfiguration::DISPLAY_PIN;
    _hardware.origin_corner = 0;
    _hardware.scan_dir = 0;
    _hardware.zigzag = 0;
    _hardware.zone_logo_pct = 0;
    _hardware.zone_split_pct = 0;
    _hardware.zone_layout = 0;

    _timing.fetch_interval = TimingConfiguration::FETCH_INTERVAL_SECONDS;
    _timing.display_cycle = TimingConfiguration::DISPLAY_CYCLE_SECONDS;

    _network.wifi_ssid = WiFiConfiguration::WIFI_SSID;
    _network.wifi_password = WiFiConfiguration::WIFI_PASSWORD;
    _network.opensky_client_id = APIConfiguration::OPENSKY_CLIENT_ID;
    _network.opensky_client_secret = APIConfiguration::OPENSKY_CLIENT_SECRET;
    _network.aeroapi_key = APIConfiguration::AEROAPI_KEY;
}

void ConfigManager::init() {
    if (g_configMutex == nullptr) {
        g_configMutex = xSemaphoreCreateMutex();
        if (g_configMutex == nullptr) {
            LOG_E("ConfigManager", "Failed to create config mutex");
            return;
        }
    }

    {
        ConfigLockGuard guard;
        _persistPending = false;
        _persistScheduledAt = 0;
        loadDefaults();
    }

    // Override with NVS values where they exist
    loadFromNvs();

    LOG_I("ConfigManager", "Initialized");
}

bool ConfigManager::factoryReset() {
    if (g_configMutex == nullptr) {
        init();
        if (g_configMutex == nullptr) {
            return false;
        }
    }

    ConfigLockGuard guard;
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for factory reset");
        return false;
    }

    prefs.clear();
    prefs.end();

    _persistPending = false;
    _persistScheduledAt = 0;
    loadDefaults();
    LOG_I("ConfigManager", "Factory reset complete — defaults restored");
    return true;
}

void ConfigManager::tick() {
    unsigned long persistScheduledAt = 0;
    bool persistPending = false;
    {
        ConfigLockGuard guard;
        persistPending = _persistPending;
        persistScheduledAt = _persistScheduledAt;
    }

    if (!persistPending) {
        return;
    }

    if ((long)(millis() - persistScheduledAt) >= 0) {
        persistToNvs();
    }
}

void ConfigManager::loadFromNvs() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        LOG_I("ConfigManager", "NVS namespace empty or first boot — using compile-time defaults");
        return;
    }

    ConfigSnapshot snapshot;
    {
        ConfigLockGuard guard;
        snapshot.display = _display;
        snapshot.location = _location;
        snapshot.hardware = _hardware;
        snapshot.timing = _timing;
        snapshot.network = _network;
    }

    // Display
    if (prefs.isKey("brightness"))     snapshot.display.brightness = prefs.getUChar("brightness", snapshot.display.brightness);
    if (prefs.isKey("text_color_r"))   snapshot.display.text_color_r = prefs.getUChar("text_color_r", snapshot.display.text_color_r);
    if (prefs.isKey("text_color_g"))   snapshot.display.text_color_g = prefs.getUChar("text_color_g", snapshot.display.text_color_g);
    if (prefs.isKey("text_color_b"))   snapshot.display.text_color_b = prefs.getUChar("text_color_b", snapshot.display.text_color_b);

    // Location
    if (prefs.isKey("center_lat"))     snapshot.location.center_lat = prefs.getDouble("center_lat", snapshot.location.center_lat);
    if (prefs.isKey("center_lon"))     snapshot.location.center_lon = prefs.getDouble("center_lon", snapshot.location.center_lon);
    if (prefs.isKey("radius_km"))      snapshot.location.radius_km = prefs.getDouble("radius_km", snapshot.location.radius_km);

    // Hardware — validate tile config to prevent OOM crash
    if (prefs.isKey("tiles_x") || prefs.isKey("tiles_y") || prefs.isKey("tile_pixels")) {
        uint8_t tx = prefs.isKey("tiles_x") ? prefs.getUChar("tiles_x", snapshot.hardware.tiles_x) : snapshot.hardware.tiles_x;
        uint8_t ty = prefs.isKey("tiles_y") ? prefs.getUChar("tiles_y", snapshot.hardware.tiles_y) : snapshot.hardware.tiles_y;
        uint8_t tp = prefs.isKey("tile_pixels") ? prefs.getUChar("tile_pixels", snapshot.hardware.tile_pixels) : snapshot.hardware.tile_pixels;
        if (isValidTileConfig(tx, ty, tp)) {
            snapshot.hardware.tiles_x = tx;
            snapshot.hardware.tiles_y = ty;
            snapshot.hardware.tile_pixels = tp;
        } else {
            LOG_E("ConfigManager", "NVS tile config invalid — using defaults");
        }
    }
    if (prefs.isKey("display_pin")) {
        const uint8_t storedPin = prefs.getUChar("display_pin", snapshot.hardware.display_pin);
        if (isSupportedDisplayPin(storedPin)) {
            snapshot.hardware.display_pin = storedPin;
        }
    }
    if (prefs.isKey("origin_corner"))  snapshot.hardware.origin_corner = prefs.getUChar("origin_corner", snapshot.hardware.origin_corner);
    if (prefs.isKey("scan_dir"))       snapshot.hardware.scan_dir = prefs.getUChar("scan_dir", snapshot.hardware.scan_dir);
    if (prefs.isKey("zigzag"))         snapshot.hardware.zigzag = prefs.getUChar("zigzag", snapshot.hardware.zigzag);
    if (prefs.isKey("zone_logo_pct"))  snapshot.hardware.zone_logo_pct = prefs.getUChar("zone_logo_pct", snapshot.hardware.zone_logo_pct);
    if (prefs.isKey("zone_split_pct")) snapshot.hardware.zone_split_pct = prefs.getUChar("zone_split_pct", snapshot.hardware.zone_split_pct);
    if (prefs.isKey("zone_layout"))    snapshot.hardware.zone_layout = prefs.getUChar("zone_layout", snapshot.hardware.zone_layout);

    // Timing
    if (prefs.isKey("fetch_interval")) snapshot.timing.fetch_interval = prefs.getUShort("fetch_interval", snapshot.timing.fetch_interval);
    if (prefs.isKey("display_cycle"))  snapshot.timing.display_cycle = prefs.getUShort("display_cycle", snapshot.timing.display_cycle);

    // Network
    if (prefs.isKey("wifi_ssid"))      snapshot.network.wifi_ssid = prefs.getString("wifi_ssid", snapshot.network.wifi_ssid);
    if (prefs.isKey("wifi_password"))  snapshot.network.wifi_password = prefs.getString("wifi_password", snapshot.network.wifi_password);
    if (prefs.isKey("os_client_id"))   snapshot.network.opensky_client_id = prefs.getString("os_client_id", snapshot.network.opensky_client_id);
    if (prefs.isKey("os_client_sec"))  snapshot.network.opensky_client_secret = prefs.getString("os_client_sec", snapshot.network.opensky_client_secret);
    if (prefs.isKey("aeroapi_key"))    snapshot.network.aeroapi_key = prefs.getString("aeroapi_key", snapshot.network.aeroapi_key);

    prefs.end();
    {
        ConfigLockGuard guard;
        _display = snapshot.display;
        _location = snapshot.location;
        _hardware = snapshot.hardware;
        _timing = snapshot.timing;
        _network = snapshot.network;
    }
    LOG_I("ConfigManager", "NVS values loaded");
}

void ConfigManager::persistToNvs() {
    ConfigLockGuard guard;

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        LOG_E("ConfigManager", "Failed to open NVS for writing");
        return;
    }

    // Display
    prefs.putUChar("brightness", _display.brightness);
    prefs.putUChar("text_color_r", _display.text_color_r);
    prefs.putUChar("text_color_g", _display.text_color_g);
    prefs.putUChar("text_color_b", _display.text_color_b);

    // Location
    prefs.putDouble("center_lat", _location.center_lat);
    prefs.putDouble("center_lon", _location.center_lon);
    prefs.putDouble("radius_km", _location.radius_km);

    // Hardware
    prefs.putUChar("tiles_x", _hardware.tiles_x);
    prefs.putUChar("tiles_y", _hardware.tiles_y);
    prefs.putUChar("tile_pixels", _hardware.tile_pixels);
    prefs.putUChar("display_pin", _hardware.display_pin);
    prefs.putUChar("origin_corner", _hardware.origin_corner);
    prefs.putUChar("scan_dir", _hardware.scan_dir);
    prefs.putUChar("zigzag", _hardware.zigzag);
    prefs.putUChar("zone_logo_pct", _hardware.zone_logo_pct);
    prefs.putUChar("zone_split_pct", _hardware.zone_split_pct);
    prefs.putUChar("zone_layout", _hardware.zone_layout);

    // Timing
    prefs.putUShort("fetch_interval", _timing.fetch_interval);
    prefs.putUShort("display_cycle", _timing.display_cycle);

    // Network
    prefs.putString("wifi_ssid", _network.wifi_ssid);
    prefs.putString("wifi_password", _network.wifi_password);
    prefs.putString("os_client_id", _network.opensky_client_id);
    prefs.putString("os_client_sec", _network.opensky_client_secret);
    prefs.putString("aeroapi_key", _network.aeroapi_key);

    prefs.end();
    _persistScheduledAt = 0;
    _persistPending = false;
    LOG_I("ConfigManager", "NVS persisted");
}

void ConfigManager::dumpSettingsJson(JsonObject& out) {
    ConfigSnapshot snapshot;
    {
        ConfigLockGuard guard;
        snapshot.display = _display;
        snapshot.location = _location;
        snapshot.hardware = _hardware;
        snapshot.timing = _timing;
        snapshot.network = _network;
    }

    // Display — uses same key names as updateCacheFromKey / applyJson
    out["brightness"] = snapshot.display.brightness;
    out["text_color_r"] = snapshot.display.text_color_r;
    out["text_color_g"] = snapshot.display.text_color_g;
    out["text_color_b"] = snapshot.display.text_color_b;

    // Location
    out["center_lat"] = snapshot.location.center_lat;
    out["center_lon"] = snapshot.location.center_lon;
    out["radius_km"] = snapshot.location.radius_km;

    // Hardware
    out["tiles_x"] = snapshot.hardware.tiles_x;
    out["tiles_y"] = snapshot.hardware.tiles_y;
    out["tile_pixels"] = snapshot.hardware.tile_pixels;
    out["display_pin"] = snapshot.hardware.display_pin;
    out["origin_corner"] = snapshot.hardware.origin_corner;
    out["scan_dir"] = snapshot.hardware.scan_dir;
    out["zigzag"] = snapshot.hardware.zigzag;
    out["zone_logo_pct"] = snapshot.hardware.zone_logo_pct;
    out["zone_split_pct"] = snapshot.hardware.zone_split_pct;
    out["zone_layout"] = snapshot.hardware.zone_layout;

    // Timing
    out["fetch_interval"] = snapshot.timing.fetch_interval;
    out["display_cycle"] = snapshot.timing.display_cycle;

    // Network — NVS abbreviated keys for API round-trip consistency
    out["wifi_ssid"] = snapshot.network.wifi_ssid;
    out["wifi_password"] = snapshot.network.wifi_password;
    out["os_client_id"] = snapshot.network.opensky_client_id;
    out["os_client_sec"] = snapshot.network.opensky_client_secret;
    out["aeroapi_key"] = snapshot.network.aeroapi_key;
}

void ConfigManager::persistAllNow() {
    persistToNvs();
    LOG_I("ConfigManager", "Full NVS flush completed");
}

DisplayConfig ConfigManager::getDisplay() {
    ConfigLockGuard guard;
    return _display;
}

LocationConfig ConfigManager::getLocation() {
    ConfigLockGuard guard;
    return _location;
}

HardwareConfig ConfigManager::getHardware() {
    ConfigLockGuard guard;
    return _hardware;
}

TimingConfig ConfigManager::getTiming() {
    ConfigLockGuard guard;
    return _timing;
}

NetworkConfig ConfigManager::getNetwork() {
    ConfigLockGuard guard;
    return _network;
}

bool ConfigManager::requiresReboot(const char* key) {
    for (size_t i = 0; i < REBOOT_KEY_COUNT; i++) {
        if (strcmp(key, REBOOT_KEYS[i]) == 0) return true;
    }
    return false;
}

bool ConfigManager::updateCacheFromKey(const char* key, JsonVariant value) {
    return updateConfigValue(_display, _location, _hardware, _timing, _network, key, value);
}

ApplyResult ConfigManager::applyJson(const JsonObject& settings) {
    ApplyResult result;
    result.reboot_required = false;

    ConfigSnapshot snapshot;
    bool hasRebootKey = false;
    bool hasHotKey = false;
    {
        ConfigLockGuard guard;
        snapshot.display = _display;
        snapshot.location = _location;
        snapshot.hardware = _hardware;
        snapshot.timing = _timing;
        snapshot.network = _network;

        for (JsonPair kv : settings) {
            const char* key = kv.key().c_str();
            JsonVariant value = kv.value();

            if (!updateConfigValue(snapshot.display, snapshot.location, snapshot.hardware, snapshot.timing, snapshot.network, key, value)) {
                LOG_E("ConfigManager", "Rejected unknown or invalid key");
                result.applied.clear();
                return result;
            }

            result.applied.push_back(String(key));

            if (requiresReboot(key)) {
                hasRebootKey = true;
            } else {
                hasHotKey = true;
            }
        }

        _display = snapshot.display;
        _location = snapshot.location;
        _hardware = snapshot.hardware;
        _timing = snapshot.timing;
        _network = snapshot.network;
    }

    if (hasRebootKey) {
        // Reboot keys persist immediately — no debounce
        persistAllNow();
        result.reboot_required = true;
    }

    if (hasHotKey && !hasRebootKey) {
        schedulePersist(2000);
    }

    if (hasHotKey) {
        fireCallbacks();
    }

    return result;
}

void ConfigManager::schedulePersist(uint16_t delayMs) {
    ConfigLockGuard guard;
    _persistScheduledAt = millis() + delayMs;
    _persistPending = true;
}

void ConfigManager::onChange(std::function<void()> callback) {
    ConfigLockGuard guard;
    _callbacks.push_back(callback);
}

void ConfigManager::fireCallbacks() {
    std::vector<std::function<void()>> callbacks;
    {
        ConfigLockGuard guard;
        callbacks = _callbacks;
    }

    for (auto& cb : callbacks) {
        cb();
    }
}

// Compile-time URL accessors — these never change at runtime
const char* ConfigManager::getOpenSkyTokenUrl() { return APIConfiguration::OPENSKY_TOKEN_URL; }
const char* ConfigManager::getOpenSkyBaseUrl() { return APIConfiguration::OPENSKY_BASE_URL; }
const char* ConfigManager::getAeroApiBaseUrl() { return APIConfiguration::AEROAPI_BASE_URL; }
const char* ConfigManager::getFlightWallCdnBaseUrl() { return APIConfiguration::FLIGHTWALL_CDN_BASE_URL; }
bool ConfigManager::getAeroApiInsecureTls() { return APIConfiguration::AEROAPI_INSECURE_TLS; }
bool ConfigManager::getFlightWallInsecureTls() { return APIConfiguration::FLIGHTWALL_INSECURE_TLS; }


]]></file>
<file id="b22b0c06" path="firmware/core/ConfigManager.h" label="SOURCE CODE"><![CDATA[

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

// NVS key abbreviations (15-char limit):
// os_client_id   = opensky_client_id
// os_client_sec  = opensky_client_secret
// scan_dir       = scan_direction

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
    uint8_t zone_logo_pct;   // 0 = auto (square logo), 1-99 = % of matrix width
    uint8_t zone_split_pct;  // 0 = auto (50/50), 1-99 = % of matrix height for flight zone
    uint8_t zone_layout;     // 0 = classic (logo full-height left), 1 = full-width bottom
};

struct TimingConfig {
    uint16_t fetch_interval, display_cycle;
};

struct NetworkConfig {
    String wifi_ssid, wifi_password;
    String opensky_client_id, opensky_client_secret;
    String aeroapi_key;
};

struct ApplyResult {
    std::vector<String> applied;
    bool reboot_required;
};

class ConfigManager {
public:
    static void init();
    static bool factoryReset();
    static void tick();
    static DisplayConfig getDisplay();
    static LocationConfig getLocation();
    static HardwareConfig getHardware();
    static TimingConfig getTiming();
    static NetworkConfig getNetwork();
    static ApplyResult applyJson(const JsonObject& settings);
    static void dumpSettingsJson(JsonObject& out);
    static void persistAllNow();
    static void schedulePersist(uint16_t delayMs = 2000);
    static void onChange(std::function<void()> callback);
    static bool requiresReboot(const char* key);

    // Compile-time URL accessors (remain in ConfigManager, not NVS)
    static const char* getOpenSkyTokenUrl();
    static const char* getOpenSkyBaseUrl();
    static const char* getAeroApiBaseUrl();
    static const char* getFlightWallCdnBaseUrl();
    static bool getAeroApiInsecureTls();
    static bool getFlightWallInsecureTls();

private:
    static DisplayConfig _display;
    static LocationConfig _location;
    static HardwareConfig _hardware;
    static TimingConfig _timing;
    static NetworkConfig _network;

    static std::vector<std::function<void()>> _callbacks;
    static unsigned long _persistScheduledAt;
    static bool _persistPending;

    static void loadDefaults();
    static void loadFromNvs();
    static void persistToNvs();
    static void fireCallbacks();
    static bool updateCacheFromKey(const char* key, JsonVariant value);
};


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
    // --- subsystems (existing six) ---
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
<file id="30ae03ec" path="firmware/core/SystemStatus.h" label="SOURCE CODE"><![CDATA[

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>

enum class Subsystem : uint8_t {
    WIFI,
    OPENSKY,
    AEROAPI,
    CDN,
    NVS,
    LITTLEFS
};

enum class StatusLevel : uint8_t {
    OK,
    WARNING,
    ERROR
};

struct SubsystemStatus {
    StatusLevel level;
    String message;
    unsigned long timestamp;
};

// Thread-safe flight statistics snapshot (written from loop(), read from HTTP handler).
// All fields are plain scalars — safe for atomic-style copy on 32-bit ESP32.
struct FlightStatsSnapshot {
    unsigned long last_fetch_ms;    // millis() of last completed fetch
    uint16_t state_vectors;         // state vector count from last fetch
    uint16_t enriched_flights;      // enriched flight count from last fetch
    uint16_t logos_matched;         // placeholder 0 until Epic 3
    uint32_t fetches_since_boot;    // total fetch attempts since boot
};

class SystemStatus {
public:
    static void init();
    static void set(Subsystem sys, StatusLevel level, const String& message);
    static SubsystemStatus get(Subsystem sys);
    static void toJson(JsonObject& obj);

    // Build extended status JSON for /api/status health page (Story 2.4).
    // Includes subsystems + wifi_detail + device + flight + quota.
    static void toExtendedJson(JsonObject& obj, const FlightStatsSnapshot& stats);

    static const char* levelName(StatusLevel level);

private:
    static const uint8_t SUBSYSTEM_COUNT = 6;
    static SubsystemStatus _statuses[SUBSYSTEM_COUNT];
    static SemaphoreHandle_t _mutex;
    static const char* subsystemName(Subsystem sys);
};


]]></file>
<file id="4f3059cc" path="firmware/test/test_config_manager/test_main.cpp" label="SOURCE CODE"><![CDATA[

/*
Purpose: Unity tests for ConfigManager.
Tests: NVS defaults, write/read round-trip, applyJson hot vs reboot paths,
       debounce scheduling, requiresReboot key detection.
Environment: esp32dev (on-device test) — requires NVS flash.
*/
#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"

// Helper: clear NVS namespace before tests
static void clearNvs() {
    Preferences prefs;
    prefs.begin("flightwall", false);
    prefs.clear();
    prefs.end();
}

// --- Default Fallback Tests ---

void test_defaults_display_brightness() {
    clearNvs();
    ConfigManager::init();
    DisplayConfig d = ConfigManager::getDisplay();
    TEST_ASSERT_EQUAL_UINT8(5, d.brightness);
}

void test_defaults_display_text_color() {
    DisplayConfig d = ConfigManager::getDisplay();
    TEST_ASSERT_EQUAL_UINT8(255, d.text_color_r);
    TEST_ASSERT_EQUAL_UINT8(255, d.text_color_g);
    TEST_ASSERT_EQUAL_UINT8(255, d.text_color_b);
}

void test_defaults_location() {
    LocationConfig l = ConfigManager::getLocation();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 37.7749, l.center_lat);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -122.4194, l.center_lon);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 10.0, l.radius_km);
}

void test_defaults_hardware() {
    HardwareConfig h = ConfigManager::getHardware();
    TEST_ASSERT_EQUAL_UINT8(10, h.tiles_x);
    TEST_ASSERT_EQUAL_UINT8(2, h.tiles_y);
    TEST_ASSERT_EQUAL_UINT8(16, h.tile_pixels);
    TEST_ASSERT_EQUAL_UINT8(25, h.display_pin);
}

void test_defaults_timing() {
    TimingConfig t = ConfigManager::getTiming();
    TEST_ASSERT_EQUAL_UINT16(30, t.fetch_interval);
    TEST_ASSERT_EQUAL_UINT16(3, t.display_cycle);
}

// --- NVS Write + Read Round-trip ---

void test_nvs_write_read_roundtrip() {
    clearNvs();

    // Write a value to NVS directly
    Preferences prefs;
    prefs.begin("flightwall", false);
    prefs.putUChar("brightness", 42);
    prefs.putDouble("center_lat", 40.7128);
    prefs.putUShort("fetch_interval", 60);
    prefs.putString("wifi_ssid", "TestNet");
    prefs.end();

    // Re-init ConfigManager — should pick up NVS values
    ConfigManager::init();

    TEST_ASSERT_EQUAL_UINT8(42, ConfigManager::getDisplay().brightness);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 40.7128, ConfigManager::getLocation().center_lat);
    TEST_ASSERT_EQUAL_UINT16(60, ConfigManager::getTiming().fetch_interval);
    TEST_ASSERT_TRUE(ConfigManager::getNetwork().wifi_ssid == "TestNet");
}

// --- applyJson Hot-Reload Path ---

void test_apply_json_hot_reload() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["brightness"] = 100;
    doc["text_color_r"] = 200;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(2, result.applied.size());
    TEST_ASSERT_FALSE(result.reboot_required);
    TEST_ASSERT_EQUAL_UINT8(100, ConfigManager::getDisplay().brightness);
    TEST_ASSERT_EQUAL_UINT8(200, ConfigManager::getDisplay().text_color_r);
}

void test_apply_json_matrix_mapping_hot_reload() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["origin_corner"] = 3;
    doc["scan_dir"] = 1;
    doc["zigzag"] = 1;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(3, result.applied.size());
    TEST_ASSERT_FALSE(result.reboot_required);
    TEST_ASSERT_EQUAL_UINT8(3, ConfigManager::getHardware().origin_corner);
    TEST_ASSERT_EQUAL_UINT8(1, ConfigManager::getHardware().scan_dir);
    TEST_ASSERT_EQUAL_UINT8(1, ConfigManager::getHardware().zigzag);
}

void test_apply_json_hot_reload_persists_after_debounce() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["brightness"] = 77;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(1, result.applied.size());
    TEST_ASSERT_FALSE(result.reboot_required);

    Preferences prefs;
    prefs.begin("flightwall", true);
    TEST_ASSERT_EQUAL_UINT8(0, prefs.getUChar("brightness", 0));
    prefs.end();

    delay(2100);
    ConfigManager::tick();

    prefs.begin("flightwall", true);
    TEST_ASSERT_EQUAL_UINT8(77, prefs.getUChar("brightness", 0));
    prefs.end();
}

// --- applyJson Reboot Path ---

void test_apply_json_reboot_path() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["wifi_ssid"] = "NewNetwork";
    doc["wifi_password"] = "secret123";
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(2, result.applied.size());
    TEST_ASSERT_TRUE(result.reboot_required);
    TEST_ASSERT_TRUE(ConfigManager::getNetwork().wifi_ssid == "NewNetwork");

    // Verify NVS was persisted immediately for reboot keys
    Preferences prefs;
    prefs.begin("flightwall", true);
    String storedSsid = prefs.getString("wifi_ssid", "");
    prefs.end();
    TEST_ASSERT_TRUE(storedSsid == "NewNetwork");
}

// --- applyJson Mixed Keys ---

void test_apply_json_mixed_keys() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["brightness"] = 50;       // hot-reload
    doc["aeroapi_key"] = "abc";   // reboot
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(2, result.applied.size());
    TEST_ASSERT_TRUE(result.reboot_required);
    TEST_ASSERT_EQUAL_UINT8(50, ConfigManager::getDisplay().brightness);
    TEST_ASSERT_TRUE(ConfigManager::getNetwork().aeroapi_key == "abc");
}

void test_apply_json_ignores_unknown_keys() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["brightness"] = 88;
    doc["bogus_key"] = 123;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(1, result.applied.size());
    TEST_ASSERT_TRUE(result.applied[0] == "brightness");
    TEST_ASSERT_EQUAL_UINT8(88, ConfigManager::getDisplay().brightness);
}

// --- requiresReboot Key Detection ---

void test_requires_reboot_known_keys() {
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("wifi_ssid"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("wifi_password"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("os_client_id"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("os_client_sec"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("aeroapi_key"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("display_pin"));
}

void test_requires_reboot_hardware_layout_keys() {
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("tiles_x"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("tiles_y"));
    TEST_ASSERT_TRUE(ConfigManager::requiresReboot("tile_pixels"));
}

void test_requires_reboot_hot_reload_keys() {
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("brightness"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("text_color_r"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("center_lat"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("fetch_interval"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("origin_corner"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("scan_dir"));
    TEST_ASSERT_FALSE(ConfigManager::requiresReboot("zigzag"));
}

// --- Factory Reset Tests ---

void test_factory_reset_clears_nvs_and_restores_defaults() {
    clearNvs();
    ConfigManager::init();

    // Write custom values to NVS via applyJson
    JsonDocument doc;
    doc["wifi_ssid"] = "MyNetwork";
    doc["brightness"] = 200;
    doc["center_lat"] = 51.5074;
    JsonObject settings = doc.as<JsonObject>();
    ConfigManager::applyJson(settings);

    // Verify values were applied
    TEST_ASSERT_TRUE(ConfigManager::getNetwork().wifi_ssid == "MyNetwork");
    TEST_ASSERT_EQUAL_UINT8(200, ConfigManager::getDisplay().brightness);

    // Perform factory reset
    ConfigManager::factoryReset();

    // After reset, wifi_ssid should be empty (compile-time default)
    TEST_ASSERT_EQUAL(0, ConfigManager::getNetwork().wifi_ssid.length());

    // Brightness should be back to compile-time default
    TEST_ASSERT_EQUAL_UINT8(5, ConfigManager::getDisplay().brightness);

    // Location should be back to compile-time default
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 37.7749, ConfigManager::getLocation().center_lat);

    // Verify NVS is actually empty
    Preferences prefs;
    prefs.begin("flightwall", true);
    TEST_ASSERT_FALSE(prefs.isKey("wifi_ssid"));
    TEST_ASSERT_FALSE(prefs.isKey("brightness"));
    prefs.end();
}

// --- onChange Callback ---

void test_on_change_callback_fires() {
    clearNvs();
    ConfigManager::init();

    bool callbackFired = false;
    ConfigManager::onChange([&callbackFired]() {
        callbackFired = true;
    });

    JsonDocument doc;
    doc["brightness"] = 77;
    JsonObject settings = doc.as<JsonObject>();
    ConfigManager::applyJson(settings);

    TEST_ASSERT_TRUE(callbackFired);
}

// --- SystemStatus Tests ---

void test_system_status_init() {
    SystemStatus::init();
    SubsystemStatus s = SystemStatus::get(Subsystem::WIFI);
    TEST_ASSERT_EQUAL(StatusLevel::OK, s.level);
    TEST_ASSERT_TRUE(s.message == "Not initialized");
}

void test_system_status_set_get() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::WIFI, StatusLevel::OK, "Connected");
    SubsystemStatus s = SystemStatus::get(Subsystem::WIFI);
    TEST_ASSERT_EQUAL(StatusLevel::OK, s.level);
    TEST_ASSERT_TRUE(s.message == "Connected");
}

void test_system_status_error() {
    SystemStatus::set(Subsystem::OPENSKY, StatusLevel::ERROR, "401 Unauthorized");
    SubsystemStatus s = SystemStatus::get(Subsystem::OPENSKY);
    TEST_ASSERT_EQUAL(StatusLevel::ERROR, s.level);
    TEST_ASSERT_TRUE(s.message == "401 Unauthorized");
}

void test_system_status_to_json() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::WIFI, StatusLevel::OK, "Connected");
    SystemStatus::set(Subsystem::NVS, StatusLevel::WARNING, "High usage");

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    SystemStatus::toJson(obj);

    TEST_ASSERT_TRUE(obj["wifi"].is<JsonObject>());
    TEST_ASSERT_TRUE(obj["wifi"]["level"] == "ok");
    TEST_ASSERT_TRUE(obj["wifi"]["message"] == "Connected");

    TEST_ASSERT_TRUE(obj["nvs"].is<JsonObject>());
    TEST_ASSERT_TRUE(obj["nvs"]["level"] == "warning");
    TEST_ASSERT_TRUE(obj["nvs"]["message"] == "High usage");
}

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // ConfigManager defaults
    RUN_TEST(test_defaults_display_brightness);
    RUN_TEST(test_defaults_display_text_color);
    RUN_TEST(test_defaults_location);
    RUN_TEST(test_defaults_hardware);
    RUN_TEST(test_defaults_timing);

    // NVS round-trip
    RUN_TEST(test_nvs_write_read_roundtrip);

    // applyJson paths
    RUN_TEST(test_apply_json_hot_reload);
    RUN_TEST(test_apply_json_matrix_mapping_hot_reload);
    RUN_TEST(test_apply_json_hot_reload_persists_after_debounce);
    RUN_TEST(test_apply_json_reboot_path);
    RUN_TEST(test_apply_json_mixed_keys);
    RUN_TEST(test_apply_json_ignores_unknown_keys);

    // Reboot key detection
    RUN_TEST(test_requires_reboot_known_keys);
    RUN_TEST(test_requires_reboot_hardware_layout_keys);
    RUN_TEST(test_requires_reboot_hot_reload_keys);

    // Factory reset
    RUN_TEST(test_factory_reset_clears_nvs_and_restores_defaults);

    // onChange callback
    RUN_TEST(test_on_change_callback_fires);

    // SystemStatus
    RUN_TEST(test_system_status_init);
    RUN_TEST(test_system_status_set_get);
    RUN_TEST(test_system_status_error);
    RUN_TEST(test_system_status_to_json);

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}


]]></file>
</context>
<variables>
<var name="architecture_file" file_id="893ad01d" description="Architecture for technical requirements verification" load_strategy="EMBEDDED" token_approx="59787">embedded in prompt, file id: 893ad01d</var>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="default_output_file">_bmad-output/implementation-artifacts/story-validations/validation-report-fn-1-2-configmanager-expansion-schedule-keys-and-systemstatus-2026-04-12.md</var>
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
<var name="story_file" file_id="9fd78415">embedded in prompt, file id: 9fd78415</var>
<var name="story_id">fn-1.2</var>
<var name="story_key">fn-1-2-configmanager-expansion-schedule-keys-and-systemstatus</var>
<var name="story_num">2</var>
<var name="story_title">configmanager-expansion-schedule-keys-and-systemstatus</var>
<var name="timestamp">20260412_121923</var>
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
      <action>Verify alignment with architecture patterns from embedded context:
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

    <action>Use the output template below as a FORMAT GUIDE</action>
    <action>Replace all {{placeholders}} with your actual analysis results</action>
    <action>Output the complete report to stdout with all sections filled in</action>
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