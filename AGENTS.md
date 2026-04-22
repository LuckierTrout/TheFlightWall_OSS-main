# AGENTS.md — TheFlightWall

This file contains project-specific guidance for AI coding agents. The reader is expected to know nothing about the project beforehand.

---

## Project Overview

TheFlightWall is an ESP32-based embedded firmware project that drives a 160×32 pixel WS2812B LED matrix (20× 16×16 panels in a 10×2 arrangement) to display live flight information. It pulls a normalized fleet snapshot from a **Cloudflare Worker** (the "aggregator" at `workers/flightwall-aggregator/`, which polls adsb.lol and joins route metadata), enriches display names via the FlightWall CDN, and renders flight cards / custom layouts / clock / departures boards on the LED wall.

**Legacy note:** earlier firmware called OpenSky Network and FlightAware AeroAPI directly. That path was removed 2026-04-20; no rollback. Any reference to OpenSky, AeroAPI, `os_client_id`, or `aeroapi_key` in stale docs is wrong.

The project follows a **Hexagonal (Ports & Adapters)** architecture: abstract interfaces define system boundaries, concrete adapters implement external integrations, and core orchestrators tie the data pipeline together.

**Key characteristics:**
- Monolithic embedded C++ firmware (Arduino framework on ESP32)
- Dual-core FreeRTOS design: Core 1 = network/fetch, Core 0 = display render
- Onboard web portal served from LittleFS (setup wizard, dashboard, layout editor)
- OTA updates via GitHub Releases
- NVS-backed runtime configuration with compile-time defaults

---

## Technology Stack

| Category | Technology | Version / Notes |
|----------|-----------|-----------------|
| Language | C++ | C++11 (Arduino framework dialect) |
| Platform | ESP32 | espressif32 (PlatformIO) |
| Build System | PlatformIO | `platformio.ini` in `firmware/` |
| LED Control | FastLED | ^3.6.0 |
| Graphics | Adafruit GFX Library | ^1.11.9 |
| Matrix Abstraction | FastLED NeoMatrix | ^1.2 |
| JSON Parsing | ArduinoJson | ^7.4.2 |
| Web Server | ESPAsyncWebServer | ^3.6.0 |
| Networking | WiFi + HTTPClient | Built-in ESP32 Arduino core |
| TLS | WiFiClientSecure | Built-in ESP32 Arduino core |
| Filesystem | LittleFS | 960 KB partition |
| Test Framework | Unity | On-device tests via `pio test` |
| E2E Tests | Playwright | TypeScript, located in `tests/e2e/` |
| Smoke Tests | Python 3 | Standard library only, `tests/smoke/` |

---

## Repository Structure

```
TheFlightWall_OSS-main/
├── firmware/                 # All C++ source code
│   ├── src/main.cpp         # Entry point, FreeRTOS task setup
│   ├── core/                # Business logic (orchestrators, registries, managers)
│   ├── adapters/            # External API clients + hardware drivers + web portal
│   ├── interfaces/          # Abstract base classes (ports)
│   ├── models/              # Data structures (StateVector, FlightInfo, AirportInfo)
│   ├── modes/               # Display modes (ClassicCardMode, ClockMode, etc.)
│   ├── widgets/             # Reusable renderable widgets
│   ├── config/              # Compile-time configuration headers
│   ├── utils/               # Math helpers, logging macros
│   ├── data-src/            # Source web assets (HTML/JS/CSS)
│   ├── data/                # Gzip-compressed served assets + airline logos (.bin)
│   ├── test/                # Unity on-device tests
│   ├── platformio.ini       # Build configuration
│   ├── custom_partitions.csv # ESP32 flash partition table
│   └── check_size.py        # Post-build binary size gate script
├── tests/
│   ├── e2e/                 # Playwright TypeScript tests
│   └── smoke/               # Python smoke tests against live device
├── docs/                    # Architecture, API contracts, development guide
├── scripts/                 # install-hooks.sh
├── .github/workflows/       # CI pipeline (build, assets, lint, burn-in)
├── _bmad-output/            # BMAD planning & implementation artifacts
├── .bmad-assist/            # BMAD assist state and runs
├── bmad-assist.yaml         # BMAD configuration
└── package.json             # Root npm scripts (bmad:serve only)
```

---

## Build and Test Commands

### Firmware (PlatformIO)

All commands run from the `firmware/` directory.

```bash
cd firmware

# Build
pio run

# Flash to ESP32
pio run -t upload

# Upload filesystem (LittleFS web assets)
pio run -t uploadfs

# Serial monitor
pio device monitor -b 115200

# On-device Unity tests
pio test -e esp32dev

# Build with dev-only metrics baseline environment
pio run -e esp32dev_le_baseline

# Build with dev-only spike environment
pio run -e esp32dev_le_spike
```

**PlatformIO path on macOS:** If `pio` is missing from PATH, a typical local install exposes `~/.platformio/penv/bin/pio`.

**Serial port on macOS:** Use `/dev/cu.*` (not `tty.*`) for `--port` / `--upload-port`. Only one process may hold the serial device—exit `pio device monitor` (Ctrl+C) before upload or filesystem upload.

### Web Assets

After editing any file in `firmware/data-src/`, regenerate the matching `.gz` in `firmware/data/`:

```bash
cd firmware
gzip -9 -c data-src/common.js > data/common.js.gz
```

### E2E Tests (Playwright)

```bash
cd tests/e2e

# Install browsers (first time)
npm run install:browsers

# Run against mock server (default)
npm test

# Run against real device
FLIGHTWALL_BASE_URL=http://flightwall.local npm test

# Run with UI
npm run test:ui
```

### Smoke Tests (Python)

```bash
# Against a live device
python3 tests/smoke/test_web_portal_smoke.py --base-url http://flightwall.local

# Or set env var
export FLIGHTWALL_BASE_URL=http://flightwall.local
python3 tests/smoke/test_web_portal_smoke.py
```

---

## Code Organization and Module Divisions

### Architecture Pattern

Hexagonal (Ports & Adapters):
- **Interfaces** (`interfaces/`) define abstract contracts
- **Adapters** (`adapters/`) implement concrete external integrations
- **Core** (`core/`) contains business logic that depends only on interfaces
- **Modes** (`modes/`) implement `DisplayMode` interface for different visual presentations
- **Widgets** (`widgets/`) are reusable renderable components used by modes

### Key Directories

| Directory | Purpose |
|-----------|---------|
| `firmware/src/` | `main.cpp` — entry point, FreeRTOS task creation, startup sequence |
| `firmware/core/` | `FlightDataFetcher`, `ConfigManager`, `ModeRegistry`, `ModeOrchestrator`, `LayoutEngine`, `LayoutStore`, `LogoManager`, `OTAUpdater`, `SystemStatus`, `WidgetRegistry` |
| `firmware/adapters/` | `AggregatorFetcher`, `FlightWallFetcher`, `NeoMatrixDisplay`, `WiFiManager`, `WebPortal` |
| `firmware/interfaces/` | `BaseDisplay`, `BaseFlightFetcher`, `BaseStateVectorFetcher`, `DisplayMode` |
| `firmware/models/` | `StateVector`, `FlightInfo`, `AirportInfo` |
| `firmware/modes/` | `ClassicCardMode`, `LiveFlightCardMode`, `ClockMode`, `DeparturesBoardMode`, `CustomLayoutMode` |
| `firmware/widgets/` | `TextWidget`, `FlightFieldWidget`, `LogoWidget`, `MetricWidget`, `ClockWidget` |
| `firmware/config/` | Compile-time constants: `APIConfiguration`, `WiFiConfiguration`, `UserConfiguration`, `HardwareConfiguration`, `TimingConfiguration` |
| `firmware/utils/` | `GeoUtils`, `DisplayUtils`, `Log.h`, `TimeUtils` |

### Data Flow

1. Timer fires (every 30s default)
2. `AggregatorFetcher.fetchStateVectors()` → `GET https://<worker>.workers.dev/fleet` with `Authorization: Bearer <agg_token>` → parse + geo-filter + populate an in-memory enrichment map keyed by callsign
3. For each callsign: `AggregatorFetcher.fetchFlightInfo()` reads the in-memory map (no HTTPS) + `FlightWallFetcher` resolves airline/aircraft ICAO codes to display names via the CDN
4. Enriched flights pushed via FreeRTOS queue to display task on Core 0
5. `ModeRegistry::tick()` → active `DisplayMode` renders via `NeoMatrixDisplay`

---

## Code Style Guidelines

### C++ Conventions

- **Headers:** `#pragma once` guards (no include guards)
- **Includes:** Arduino headers first, then system headers, then project headers with quoted paths
- **Classes:** `PascalCase` for class names, `camelCase` for methods, `m_` prefix for private members
- **Interfaces:** Abstract base classes prefixed with `Base` (e.g., `BaseDisplay`)
- **Constants:** `SCREAMING_SNAKE_CASE` for macros and compile-time constants
- **Files:** One class per file, named after the class (e.g., `AggregatorFetcher.h` / `AggregatorFetcher.cpp`)
- **Namespaces:** Configuration uses namespace constants (e.g., `namespace APIConfiguration { ... }`)
- **Pointers:** Prefer references over pointers where possible; raw pointers acceptable for interface injection
- **Override:** Always mark overridden methods with `override`
- **Virtual destructors:** Always provide `virtual ~ClassName() = default;` in polymorphic base classes

### Logging

Use the macros from `utils/Log.h`:

```cpp
LOG_E("TAG", "error message");   // Error (LOG_LEVEL >= 1)
LOG_W("TAG", "warning message"); // Warning (LOG_LEVEL >= 1)
LOG_I("TAG", "info message");    // Info (LOG_LEVEL >= 2)
LOG_IPF("TAG", "fmt %d", val);   // Formatted info (LOG_LEVEL >= 2)
LOG_V("TAG", "verbose message"); // Verbose (LOG_LEVEL >= 3)
```

`LOG_LEVEL` is set via `-D LOG_LEVEL=3` in `platformio.ini` (3 = verbose, 2 = info, 1 = errors/warnings only).

### Comments

- Multi-line block comments at the top of files describing purpose, responsibilities, and configuration
- Inline comments for non-obvious logic, especially cross-core synchronization and timing
- Story IDs referenced in comments for traceability (e.g., `Story fn-1.4`, `AC #3`)

---

## Testing Instructions

### On-Device Unity Tests

Located in `firmware/test/`. Each test directory contains a `test_main.cpp` file. Tests run on the actual ESP32 hardware and require serial connection.

```bash
cd firmware
pio test -e esp32dev
```

Existing test suites include:
- `test_config_manager` — NVS defaults, write/read round-trip, JSON apply
- `test_layout_engine` — Layout computation from hardware config
- `test_layout_store` — Layout persistence and retrieval
- `test_mode_registry` — Mode registration, switching, preemption
- `test_mode_orchestrator` — Schedule and manual switch logic
- `test_ota_updater` — OTA state machine
- `test_logo_manager` — Logo lookup and caching
- `test_classic_card_mode` — Golden frame rendering
- `test_custom_layout_mode` — Custom layout rendering
- `test_golden_frames` — Pixel-exact render output validation

### E2E Tests

Playwright tests in `tests/e2e/tests/` targeting the web dashboard. Can run against a mock server or a real device.

```bash
cd tests/e2e
npm test                          # Mock server
FLIGHTWALL_BASE_URL=http://flightwall.local npm test  # Real device
```

### Smoke Tests

Python standard-library-only smoke suite against a live device:

```bash
python3 tests/smoke/test_web_portal_smoke.py --base-url http://flightwall.local
```

---

## CI/CD Pipeline

GitHub Actions workflow `.github/workflows/ci.yml` runs on push/PR to `main`, `develop`, and `release/**` branches when `firmware/**` changes.

### Stages

1. **Build & Size Gate** — Compiles firmware, enforces binary size limits
   - Max: 1,572,864 bytes (1.5 MiB OTA partition limit)
   - Warning: 1,363,148 bytes (1.3 MiB)
   - Hard cap (local build): 92% = 1,447,034 bytes (`check_size.py`)
2. **Web Assets Size Gate** — Validates gzipped assets fit 960 KB LittleFS budget
3. **Static Analysis** — `cppcheck` on `src/`, `core/`, `adapters/`
4. **Burn-In** (main/release only) — 3 clean builds to verify stability

### Manual Gates (Require ESP32 Hardware)

- ConfigManager unit tests
- LayoutEngine unit tests
- LogoManager unit tests
- ModeRegistry unit tests
- Telemetry conversion tests

---

## Deployment and Flashing

### First-Time Setup

1. Flash firmware: `pio run -t upload`
2. Flash filesystem: `pio run -t uploadfs`
3. Device starts AP `FlightWall-Setup`
4. Connect and open `http://192.168.4.1/` to run setup wizard
5. After WiFi config, device joins network; access at `http://flightwall.local/`

### OTA Updates

The dashboard **Firmware** card checks GitHub Releases for new versions and downloads them over the air. Manual `.bin` upload is also supported.

OTA partition layout (from `custom_partitions.csv`):
- `app0` / `app1`: 1.5 MB each (dual OTA slots)
- `spiffs`: 960 KB (LittleFS)

---

## Security Considerations

- Runtime credentials (`agg_url`, `agg_token`) are stored in **NVS** and entered via the setup wizard — never compiled in. Only the FlightWall CDN base URL lives in `config/APIConfiguration.h`.
- The aggregator Worker authenticates the device with `Authorization: Bearer <agg_token>`; the token matches the Worker's `ESP32_AUTH_TOKEN` secret.
- FlightWall CDN uses `WiFiClientSecure` with `setInsecure()` — TLS certificate verification is **disabled** by default (configurable via `FLIGHTWALL_INSECURE_TLS`). Revisit when cert pinning lands.
- OTA updates verify firmware via ESP-IDF `esp_ota_mark_app_valid_cancel_rollback()`; rollback triggers on watchdog/crash
- Web portal endpoints use basic input validation; no authentication required on local network
- LittleFS is mounted without auto-format to prevent silent data loss

---

## Commit Discipline (Story TD-4)

This repository enforces a per-story commit convention to keep diffs reviewable and prevent cross-story bleed.

**Install the local hooks once per clone:**

```bash
bash scripts/install-hooks.sh
```

This sets `core.hooksPath=.githooks` and installs `commit-msg`, which rejects any commit whose subject does not begin with a valid story-ID prefix.

**Valid subject prefixes** (case-insensitive; `.` or `-` accepted between version numbers):

| Prefix | Example | Scope |
| --- | --- | --- |
| `td-N:` | `td-1: atomic calibration flag` | Tech debt |
| `le-X.Y:` | `le-1.1: layout store CRUD` | Layout Editor epic |
| `fn-X.Y:` | `fn-1.4: OTA self-check` | fn epic |
| `dl-X.Y:` | `dl-7.3: OTA pull dashboard` | dl epic |
| `ds-X.Y:` | `ds-3.1: display mode API` | ds epic |

Merge and revert auto-messages are exempt. Empty messages are rejected.

**Story frontmatter is required.** Every new story file in `_bmad-output/implementation-artifacts/stories/` carries `branch:` and `zone:` fields (see `stories/_template.md`). The `zone:` field is the source of truth reviewers use to check that a PR stays within the story's declared scope — see `_bmad-output/implementation-artifacts/review-checklist.md` item #1.

**PR reviewers** should run through the full checklist at `_bmad-output/implementation-artifacts/review-checklist.md` before approving. The first four items (scope, correctness, safety) are required; remaining items are advisory.

**Known limitation:** git hooks are local-only and are bypassed on force-push or when a contributor has not run `install-hooks.sh`. PR review is the second layer; the review-checklist codifies that layer.

---

## Dev-only PlatformIO Environments (Layout Editor)

`firmware/platformio.ini` carries two auxiliary environments beyond the production `[env:esp32dev]`:

- **`[env:esp32dev_le_baseline]`** — extends `esp32dev` with `-D LE_V0_METRICS`. Compiled for render-budget measurement runs against the fixed `ClassicCardMode` baseline.
- **`[env:esp32dev_le_spike]`** — extends `esp32dev` with `-D LE_V0_METRICS -D LE_V0_SPIKE`. Adds the legacy `CustomLayoutMode` entry to the mode table (guarded by `#ifdef LE_V0_SPIKE` in `firmware/src/main.cpp`) so the generic renderer can be exercised for spike measurements.

These are **dev-only performance-measurement scaffolds**, not shipping builds. Production firmware is always built from `[env:esp32dev]` with neither flag. The `LE_V0_METRICS` instrumentation hooks were removed in Story le-1.1 (V0 Spike Cleanup); re-introducing them requires re-wiring timing around `ModeRegistry::tick()`. The `LE_V0_SPIKE` mode-table guard remains in place until Story LE-1.3 replaces the spike mode with the full LayoutStore-backed `CustomLayoutMode`.

---

## Learned User Preferences

- When triaging external analysis or assessment reports, the user may ask for **codebase-only** scope (product code and tests), excluding BMAD outputs and agent or LLM documentation collateral.

## Learned Workspace Facts

- Bundled web sources for the firmware portal live under `firmware/data-src/`; served assets are gzip-compressed under `firmware/data/`. After editing a bundled source file, regenerate the matching `.gz` from `firmware/` (for example `gzip -9 -c data-src/common.js > data/common.js.gz`).
- Firmware uses PlatformIO; run builds from `firmware/` with `pio run`. If `pio` is missing from the agent shell PATH, a typical local install on macOS exposes `~/.platformio/penv/bin/pio`.
- GitHub CLI pull-request flows (`gh pr …`) require a configured `git remote` pointing at GitHub; without a remote, PR-based `/code-review` style workflows cannot load or comment on a PR.
- BMAD planning and implementation work lean on `_bmad-output/planning-artifacts/` (for example `architecture.md`, `epics.md`) and story files under `_bmad-output/implementation-artifacts/`. For **`bmad-assist` `create_story`**, the bundled workflow still resolves `**/project-context.md` when `project-context` is listed under `compiler.strategic_context.create_story.include` in `bmad-assist.yaml`—add `_bmad-output/project-context.md` (for example via the generate-project-context workflow) or adjust that include list.
- The **bmad-assist dashboard** is started with `bmad-assist serve --project <repo-root>` (default URL `http://127.0.0.1:9600/`). In `serve`, `-p` / `--port` is the HTTP port, not the project path. If the default port is busy, `serve` may pick the next available port (for example 9602). Root `package.json` defines `bmad:serve` (for example `pnpm run bmad:serve`) to run `bmad-assist serve`, preferring `.venv/bin/bmad-assist` when that executable exists. When the dashboard **Start** loop runs `bmad-assist run` as a subprocess, optional verbosity/debug flags come from the **`serve` process environment** (`BMAD_DASHBOARD_RUN_VERBOSE`, `BMAD_DASHBOARD_RUN_DEBUG`, `BMAD_DASHBOARD_RUN_FULL_STREAM`) or from optional `dashboard.run_with_verbose` / `run_with_debug` / `run_with_full_stream` booleans in `bmad-assist.yaml` (requires a `bmad-assist` build that implements `build_dashboard_bmad_run_args` in `bmad_assist.dashboard.server`).
- For **`bmad-assist run`**, `.bmad-assist/state.yaml` values such as `current_epic` and `current_story` should match keys under `development_status` in `_bmad-output/implementation-artifacts/sprint-status.yaml`, or the runner may skip with "story not found in sprint-status." The **active epic list** comes from **non-done stories parsed from epic markdown** in planning artifacts (expected `### Story …` section headings); **`sprint-status.yaml` merges status onto those story IDs only** and does not create rows for backlog keys that never appear as parsed stories—so **No epics found in project** can happen when every loaded story is already `done` while remaining work exists only in sprint-status or in epic documents the parser does not treat as stories (for example tables without those section headers).
- When there is no git baseline (no commits or empty diff), BMAD `bmad-code-review` can still scope review from the story's declared file list and approximate diffs with `git diff --no-index` for new or untracked files.
- For code-only static analysis or reviews, scope primarily to `firmware/` (for example `src/`, `core/`, `adapters/`, `interfaces/`, `models/`, `config/`, `utils/`, `data-src/`, `platformio.ini`, `custom_partitions.csv`); include `firmware/test/` and repo-root `tests/` when test coverage matters; omit `_bmad/`, `_bmad-output/`, `.bmad-assist/`, typical agent or editor dirs (for example `.claude/`, `.cursor/`, `.agents/`), `docs/`, generated `firmware/data/`, and root metadata-only files such as `README.md`, `AGENTS.md`, and `LICENSE`.
- Smoke-test the onboard web portal against a running device with `python3 tests/smoke/test_web_portal_smoke.py --base-url http://<host>` (or set `FLIGHTWALL_BASE_URL`).
- On macOS, use `/dev/cu.*` (not `tty.*`) for PlatformIO `--port` / `--upload-port`. Only one process may hold the serial device—exit `pio device monitor` (Ctrl+C) before `upload` or filesystem upload.
- On **macOS**, Python **subprocess `fork`** from a **multi-threaded** process (common under **Cursor** with certain interpreters) can **SIGSEGV** in Apple's Network stack (`multi-threaded process forked` / `crashed on child side of fork pre-exec`). A frequent dev mitigation is `OBJC_DISABLE_INITIALIZE_FORK_SAFETY=YES` in the environment; using a **stable** interpreter (for example Python 3.12) for editor tooling instead of a very new Homebrew Python reduces how often this appears.
- The web portal is served by the ESP32 (flash both app and LittleFS from `firmware/`, e.g. `pio run -e esp32dev -t upload` and `-t uploadfs`). First-time setup: join AP `FlightWall-Setup`, open `http://192.168.4.1/`; on the home network use `http://flightwall.local/` or the device's LAN IP.
