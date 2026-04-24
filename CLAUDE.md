# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TheFlightWall is an ESP32-S3 firmware for a 256×192 HUB75 LED wall (12× 64×64 panels arranged 4 wide × 3 tall, single chain at 1/32 scan) that displays live flight information. It pulls a normalized fleet snapshot from a Cloudflare Worker (the "aggregator" at `workers/flightwall-aggregator/`, which in turn pulls adsb.lol and joins route data), resolves airline/aircraft display names via the FlightWall CDN, and renders flight cards on the HUB75 canvas. A captive-portal web UI handles configuration, display mode selection, OTA updates, and diagnostics.

**NOTE — legacy:**
- Earlier versions of the firmware used WS2812B addressable strips via FastLED + FastLED_NeoMatrix (epic hw-1 migrated to HUB75; the old `display_pin`/`tiles_x`/`tiles_y`/`tile_pixels` NVS keys were retired on 2026-04-22/23). Any reference to FastLED, WS2812, NeoMatrix, data-pin setup, or tile configuration in old docs or comments is stale.
- Earlier versions called OpenSky and FlightAware AeroAPI directly. That path was fully removed when the Cloudflare aggregator shipped; `os_client_id` / `aeroapi_key` references are stale.

## Build & Flash Commands

All commands run from `firmware/`. PlatformIO may not be in PATH on macOS — use `~/.platformio/penv/bin/pio` if needed.

```bash
pio run                          # Build firmware
pio run -t upload                # Flash firmware to ESP32
pio run -t uploadfs              # Upload LittleFS filesystem (web assets)
pio device monitor -b 115200     # Serial monitor (close before uploading)
pio test                         # Run all Unity tests on device
pio test -f test_ota_updater     # Run a single test suite
pio test -f test_ota_updater --without-uploading --without-testing  # Compile-only check
```

On macOS: use `/dev/cu.*` (not `tty.*`) for serial port. Only one process can hold the serial device — exit the monitor before upload.

## Web Asset Pipeline

Editable sources live in `firmware/data-src/` (HTML, JS, CSS). The ESP32 serves gzip-compressed copies from `firmware/data/`. There is no automated build script — after editing any source file, regenerate manually:

```bash
cd firmware
gzip -c data-src/dashboard.js > data/dashboard.js.gz
gzip -c data-src/dashboard.html > data/dashboard.html.gz
gzip -c data-src/style.css > data/style.css.gz
# Also: common.js, wizard.html, wizard.js, health.html, health.js
```

Then `pio run -t uploadfs` to flash the updated assets.

## Architecture

**Hexagonal (Ports & Adapters)** with **dual-core FreeRTOS**:

- **Core 0** — Display rendering. Runs `MatrixPanel_I2S_DMA::flipDMABuffer()` (HUB75 LCD_CAM double-buffer flip), mode render loops, layout engine. Must never block on network I/O.
- **Core 1** — WiFi, HTTP server (ESPAsyncWebServer), API fetches, OTA downloads, config writes.
- **Synchronization** — `std::atomic` flags for cross-core signals (config changed, NTP synced, flight count). FreeRTOS queues for flight data producer-consumer. No mutexes between cores.

### Key Directories (`firmware/`)

| Directory | Role |
|-----------|------|
| `src/main.cpp` | Entry point, FreeRTOS task setup, mode table registration |
| `core/` | Business logic: ConfigManager, ModeOrchestrator, ModeRegistry, OTAUpdater, SystemStatus |
| `adapters/` | External integrations: WebPortal, WiFiManager, HUB75MatrixDisplay, HUB75PinMap, API fetchers |
| `interfaces/` | Abstract base classes: DisplayMode, BaseDisplay, BaseFlightFetcher |
| `modes/` | DisplayMode implementations: ClassicCardMode, LiveFlightCardMode, ClockMode, DeparturesBoardMode |
| `models/` | Data structs: StateVector, FlightInfo, AirportInfo |
| `config/` | Compile-time defaults (only ConfigManager.cpp includes these) |
| `utils/` | Log, GeoUtils, TimeUtils, DisplayUtils |
| `test/` | Unity test suites, one directory per component |
| `data-src/` | Web portal source files (HTML/JS/CSS) |
| `data/` | Gzipped web assets served from LittleFS |

### Display Mode System

Modes are registered in a static table in `main.cpp` with factory functions. `ModeRegistry` manages lifecycle (create/teardown/switch) on Core 0. `ModeOrchestrator` handles state transitions (manual selection, idle fallback to clock, time-based scheduling).

Adding a new mode requires: a `DisplayMode` subclass in `modes/`, a factory function, a memory requirement function, a `ModeZoneDescriptor`, and one `ModeEntry` line in the mode table in `main.cpp`.

### Configuration System

`ConfigManager` wraps ESP32 NVS (namespace `"flightwall"`). Only ConfigManager.cpp includes compile-time config headers. Other components use category struct getters (`getDisplay()`, `getHardware()`, `getNetwork()`, etc.) which return thread-safe copies.

Reboot-required NVS keys: `wifi_ssid`, `wifi_password`, `agg_url`, `agg_token`. (The legacy `display_pin`/`tiles_x`/`tiles_y`/`tile_pixels` keys were retired in hw-1.3 — the HUB75 wall is fixed at 256×192 and its pin map lives in `adapters/HUB75PinMap.h`.)

### REST API Pattern

All endpoints use a consistent JSON envelope:
```json
{ "ok": true, "data": { ... }, "error": "message", "code": "ERROR_CODE" }
```

Key routes: `GET/POST /api/settings`, `GET /api/status`, `GET /api/display/modes`, `POST /api/display/mode`, `GET/POST /api/schedule`, `GET /api/ota/check`, `POST /api/ota/pull`, `GET /api/ota/status`, `POST /api/ota/upload`.

### OTA Update Flow

Two paths: **pull** (check GitHub Releases, download binary with SHA-256 verification) and **push** (manual .bin upload via dashboard). Both use ESP32's stock A/B partition scheme with automatic bootloader rollback if the new image fails boot validation.

## Partition Layout

Dual-OTA: `app0` 1.5MB + `app1` 1.5MB + `spiffs` (LittleFS) 960KB. Binary must stay under 1.5MB (`check_size.py` enforces this at build time). If partitions change, update both `check_size.py` and `main.cpp` `validatePartitionLayout()`.

## Testing

Unity framework on `esp32dev`. Tests require a physical ESP32 for execution but can be compile-checked without one:

```bash
pio test -f test_config_manager --without-uploading --without-testing  # compile only
```

Test files are in `firmware/test/test_<component>/test_main.cpp`. Shared fixtures in `test/fixtures/`. Tests use `#ifndef PIO_UNIT_TESTING` guards in `main.cpp` to exclude dual-core setup during test builds.

Smoke test the web portal against a running device (accepts `--base-url` or the `FLIGHTWALL_BASE_URL` env var):
```bash
python3 tests/smoke/test_web_portal_smoke.py --base-url http://flightwall.local
```

**First boot:** The device starts in AP mode as `FlightWall-Setup` at `http://192.168.4.1/`. Once Wi-Fi is configured it joins your LAN and the dashboard is reachable at `http://flightwall.local/`.

## Critical Constraints

- **Heap**: ESP32-S3 has ~160KB usable internal SRAM (PSRAM disabled per the td-7 Wi-Fi-driver incompatibility with OPI PSRAM on the Lonely Binary N16R8). Modes require a 30KB heap margin before switching. OTA download requires 80KB free. Prefer static allocation over dynamic.
- **HUB75 framebuffer**: a 256×192 double-buffered 16bpp frame is ~196KB, which exceeds internal SRAM. The mrfaptastic `MatrixPanel_I2S_DMA` library auto-reduces color depth (typically to 5-6 bpc) to fit the framebuffer and maintain ≥60Hz refresh. Expected behaviour — do not set `FORCE_COLOR_DEPTH`.
- **Binary size**: esp32s3_n16r8 targets a 3MB OTA partition (uses ~1.16MB, 38.7%). The classic esp32dev env targets 1.5MB (uses ~1.23MB, 81%). `check_size.py` enforces both at build time.
- **NVS keys**: 15-character limit. Many are abbreviated (e.g., `agg_url`/`agg_token` for the Cloudflare aggregator Worker, `sched_dim_start` for the night-mode schedule).
- **C++ standard**: project builds with `gnu++17` (required by the HUB75 library's `VirtualMatrixPanel_T` `if constexpr` usage). Set via `build_unflags = -std=gnu++11` + `build_flags = -std=gnu++17` in `platformio.ini`.
- **ArduinoJson v7**: Use `JsonDocument` (not deprecated `StaticJsonDocument`/`DynamicJsonDocument`). Use filter documents for large payloads.
- **Web JS**: ES5 syntax only (no arrow functions, no `let`/`const`, no template literals). `FW.get()`, `FW.post()`, `FW.del()` return promises with `{ status, body }`. `FW.showToast(msg, severity)` for notifications.

## BMAD Workflow

Sprint tracking: `_bmad-output/implementation-artifacts/sprint-status.yaml`. Story files: `_bmad-output/implementation-artifacts/stories/`. Planning artifacts (architecture, epics, PRDs): `_bmad-output/planning-artifacts/`. Config: `_bmad/bmm/config.yaml`.

## Code-Only Review Scope

For static analysis, code review, or coverage work, scope primarily to `firmware/` (`src/`, `core/`, `adapters/`, `interfaces/`, `models/`, `config/`, `utils/`, `data-src/`, `platformio.ini`, `custom_partitions.csv`). Include `firmware/test/` and repo-root `tests/` when test coverage matters. Omit `_bmad/`, `_bmad-output/`, `.bmad-assist/`, editor/agent dirs (`.claude/`, `.cursor/`, `.agents/`, etc.), `docs/`, generated `firmware/data/`, and metadata-only root files (`README.md`, `AGENTS.md`, `LICENSE`).
