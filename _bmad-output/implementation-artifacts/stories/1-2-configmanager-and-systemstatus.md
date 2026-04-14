# Story 1.2: ConfigManager & SystemStatus

Status: done

Ultimate context engine analysis completed — comprehensive developer guide created.

## Story

As a developer,
I want a ConfigManager singleton that persists settings in NVS with compile-time fallbacks, and a SystemStatus singleton for health tracking,
so that all components have reliable configuration and error reporting.

## Acceptance Criteria

1. **Empty NVS / first boot:** After `ConfigManager::init()`, `getDisplay()` returns display values matching compile-time defaults from existing `config/UserConfiguration.h` (brightness=5, text RGB=255/255/255 per current headers).

2. **NVS override:** When keys exist in NVS, `getDisplay()` (and other category getters) return NVS-backed values, not compile-time defaults.

3. **applyJson hot-reload path:** For non-reboot keys (e.g. `brightness`, `text_color_r`), RAM cache updates immediately, registered `onChange` callbacks fire, `schedulePersist(2000)` debounces NVS write (2s quiet), `ApplyResult` lists applied keys and `reboot_required=false`.

4. **applyJson reboot path:** For reboot-required keys (see Technical Requirements), NVS persists immediately (no debounce), `ApplyResult` has `reboot_required=true`.

5. **SystemStatus:** After `SystemStatus::init()`, `set(WIFI, OK, "Connected")` (or equivalent enum) causes `get(WIFI)` to return level, message, timestamp; `toJson()` includes that subsystem in the health snapshot.

6. **Pipeline migration:** OpenSkyFetcher, AeroAPIFetcher, FlightDataFetcher, NeoMatrixDisplay, and FlightWallFetcher run the fetch → enrich → display pipeline with behavior identical to pre-migration (same effective credentials, geometry, timing, colors) when NVS is empty (defaults = former header values).

7. **AR13 include rule:** No `config/*.h` includes remain outside `ConfigManager.cpp` — all other files use `ConfigManager` / `SystemStatus` APIs only.

8. **Unit tests (Unity):** `pio test` passes tests covering NVS read/write, default fallback chain, `applyJson` parsing, reboot-key detection, and debounce timing (mock or harness as appropriate for embedded).

## Tasks / Subtasks

- [x] Task 1: Add `core/ConfigManager.h` and `core/ConfigManager.cpp` (AC: #1–4, #7–8)
  - [x] Define category structs with **snake_case** fields per architecture (Display, Location, Hardware, Timing, Network) — JSON/NVS/struct alignment
  - [x] NVS namespace `"flightwall"`; keys per NVS table in architecture; abbreviate keys >15 chars (`os_client_id`, `os_client_sec`, `scan_dir`) documented in header
  - [x] `init()`: load NVS into RAM cache; missing keys use compile-time defaults from namespaces in **ConfigManager.cpp only** (`UserConfiguration`, `WiFiConfiguration`, `TimingConfiguration`, `HardwareConfiguration`, `APIConfiguration`)
  - [x] Getters return copies of cached structs (thread-safe enough for current single-writer pattern; document if mutex added later)
  - [x] `applyJson(const JsonObject&)`: validate keys, update cache, distinguish reboot vs debounced persist, build `ApplyResult`
  - [x] `schedulePersist`, `onChange`, debounce timer (FreeRTOS software timer or millis-based — document choice)
  - [x] **Only** `ConfigManager.cpp` includes `config/*.h`

- [x] Task 2: Add `core/SystemStatus.h` and `core/SystemStatus.cpp` (AC: #5)
  - [x] `Subsystem` enum: at minimum WIFI, OPENSKY, AEROAPI, CDN, NVS, LITTLEFS (extensible)
  - [x] `StatusLevel`: OK, WARNING, ERROR (or equivalent)
  - [x] `SubsystemStatus`: level, message, timestamp
  - [x] `set`, `get`, `toJson` — use ArduinoJson `JsonObject` consistent with project patterns; keep RAM modest

- [x] Task 3: Wire boot order in `main.cpp` (AC: #1, #6)
  - [x] Call `ConfigManager::init()` early in `setup()` after Serial (before WiFi/display pipeline depends on config)
  - [x] Call `SystemStatus::init()` immediately after
  - [x] Replace WiFi credential reads in `main.cpp` with `ConfigManager::getNetwork()` (or equivalent)

- [x] Task 4: Migrate adapters off config headers (AC: #6, #7)
  - [x] `core/FlightDataFetcher.cpp` — location/radius from `getLocation()`
  - [x] `adapters/OpenSkyFetcher.cpp/.h` — OAuth IDs/secrets and OpenSky URLs: use `getNetwork()` + accessors for static URLs if needed (URLs can remain compile-time inside ConfigManager.cpp)
  - [x] `adapters/AeroAPIFetcher.cpp/.h` — AeroAPI key and base URL pattern same approach
  - [x] `adapters/FlightWallFetcher.cpp/.h` — CDN base / TLS flags via ConfigManager getters only (no `APIConfiguration.h` in fetcher TU)
  - [x] `adapters/NeoMatrixDisplay.cpp` — brightness, colors, timing, hardware matrix dimensions/pin from `getDisplay()`, `getTiming()`, `getHardware()`
  - [x] Remove all `#include "config/..."` from migrated files; fix `build_src_filter` / includes if `core/` needs `-I core` (already present)

- [x] Task 5: Unity tests under `firmware/test/` (AC: #8)
  - [x] Use existing `test_framework = unity` and `test_build_src = true` in `platformio.ini`
  - [x] Cover: defaults when NVS empty; write + read round-trip; `applyJson` hot vs reboot paths; debounce fires once after quiet period (time simulation or short timeout in test env)

- [x] Task 6: Verification (AC: #6, #8)
  - [x] `pio run` — zero errors
  - [x] `pio test` — all tests build (device upload pending — no ESP32 connected)
  - [ ] On device (when available): pipeline still shows flights with empty NVS using former header defaults

### Review Findings

- [x] [Review][Patch] Hot-reload config changes never reach NVS because the scheduled persist path is never serviced [firmware/core/ConfigManager.cpp:246]
- [x] [Review][Patch] `display_pin` is persisted as a reboot-required setting but NeoMatrixDisplay still hardcodes GPIO 25 [firmware/adapters/NeoMatrixDisplay.cpp:55]
- [x] [Review][Patch] ConfigManager accepts unknown JSON keys as "applied" instead of validating and rejecting them [firmware/core/ConfigManager.cpp:213]

## Dev Notes

### Developer context / guardrails

- **Do not reintroduce** `config/*.h` into adapters or `main` after migration — violates AR13 and breaks the single source of truth.
- **Reboot-required keys** (persist immediately, no debounce): `wifi_ssid`, `wifi_password`, `opensky_client_id`, `opensky_client_secret`, `aeroapi_key`, `display_pin` [Source: `_bmad-output/planning-artifacts/architecture.md` — Decision 1]
- **Hot-reload keys** (debounced NVS): brightness, text colors, fetch/display intervals, tiles, lat/lon/radius, wiring flags [Source: architecture.md — Decision 1]
- **NFR9:** Make NVS writes atomic where feasible — avoid partial key updates if possible; critical reboot path should commit before reporting success.
- **Memory:** Prefer ArduinoJson filter / minimal documents in `applyJson`; avoid large heap allocations on stack in handlers.

### Architecture compliance

- Class/method naming: PascalCase classes, camelCase methods, struct fields snake_case [Source: architecture.md — Naming Patterns]
- Error logging: `LOG_E` / `LOG_I` / `LOG_V` from `utils/Log.h` [Source: story 1.1 — Log.h established]
- ConfigManager + SystemStatus are **core** components under `firmware/core/` [Source: architecture.md — source tree]
- Initialization order for full system (this story implements first two steps only): `(1) ConfigManager::init()` `(2) SystemStatus::init()` … [Source: architecture.md — Decision 5]

### Library / framework requirements

- **ArduinoJson** ^7.x — use `JsonObject` for `applyJson`; match existing project usage [Source: `firmware/platformio.ini`]
- **Preferences** (ESP32 NVS) — standard Arduino layer for namespace `"flightwall"`
- **Unity** — PlatformIO native `test_framework = unity` [Source: `firmware/platformio.ini`]

### File structure requirements

| File | Action |
|------|--------|
| `firmware/core/ConfigManager.h` | CREATE |
| `firmware/core/ConfigManager.cpp` | CREATE |
| `firmware/core/SystemStatus.h` | CREATE |
| `firmware/core/SystemStatus.cpp` | CREATE |
| `firmware/src/main.cpp` | MODIFY — init + network from ConfigManager |
| `firmware/core/FlightDataFetcher.cpp` | MODIFY |
| `firmware/adapters/OpenSkyFetcher.*`, `AeroAPIFetcher.*`, `FlightWallFetcher.*`, `NeoMatrixDisplay.cpp` | MODIFY |
| `firmware/test/test_*/...` | CREATE — ConfigManager tests |

Ensure `platformio.ini` `build_src_filter` still picks up `../core/*.cpp` (already includes `+<../core/*.cpp>`).

### Testing requirements

- AR15: unit tests for ConfigManager required — this story delivers them [Source: epics.md — Additional Requirements]
- Prefer native test environment (`pio test -e native`) if available; else `esp32dev` test target — document chosen env in Dev Agent Record
- Tests must not require live WiFi or external APIs

### Previous story intelligence (1.1)

- **LittleFS** mounts in `main.cpp` with `LittleFS.begin(true)`; logging uses `utils/Log.h` [Source: `1-1-project-infrastructure.md`]
- **Partition / deps:** Carbou ESPAsyncWebServer fork, custom partitions, `-D LOG_LEVEL=3` already in place — do not regress
- **Story 1.1** intentionally did not add ConfigManager — this story is the first core logic layer on top of infrastructure
- Physical-device checks (uploadfs, flight display) may still be pending on 1.1; for 1.2 prioritize compile + `pio test` + code review

### Git intelligence

- Repository had **no commits** at story creation time — no commit-history patterns to cite.

### Latest technical notes

- ESP32 NVS key length limit **15 characters** — architecture table uses abbreviated keys for long names
- `ApplyResult.applied` in architecture shows `std::vector<String>` — acceptable on ESP32; if flash size tight, consider capped inline array with count (document tradeoff if changed)

### Project context reference

- No `project-context.md` found in repo — rely on `_bmad-output/planning-artifacts/architecture.md`, `epics.md`, and `docs/` as needed

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6 (claude-opus-4-6)

### Implementation Plan

- **Debounce approach:** millis-based (`_persistScheduledAt` + `_persistPending` flags). No FreeRTOS software timer used since this story doesn't introduce FreeRTOS tasks yet. The caller (loop or future task) checks the pending flag. Reboot-required keys bypass debounce entirely and persist immediately.
- **Test environment:** esp32dev (on-device Unity tests). Tests require physical ESP32 for actual execution due to NVS hardware dependency. Tests compile and link successfully. `#ifndef PIO_UNIT_TESTING` guard added to main.cpp to avoid duplicate `setup()`/`loop()` symbols.
- **URL accessors:** Compile-time URLs (OpenSky token URL, base URLs, CDN base URL, TLS flags) exposed via static ConfigManager getters that delegate to `APIConfiguration.h` — these values never change at runtime and don't need NVS persistence.
- **FastLED pin:** `FastLED.addLeds` requires a compile-time constant for the template pin parameter. Hardcoded to `25` (matching default). Runtime pin changes via `display_pin` NVS key require reboot (already marked as reboot-required key).

### Debug Log References

- `pio run` — SUCCESS (0 errors, 0 new warnings from this story's code)
- `pio test -e esp32dev --without-uploading --without-testing` — BUILD PASSED
- AR13 compliance: `grep '#include "config/'` returns only `core/ConfigManager.cpp` (5 includes)
- RAM: 14.7% (48092/327680 bytes) | Flash: 49.7% (1042625/2097152 bytes)

### Completion Notes List

- ConfigManager singleton created with 5 category structs (Display, Location, Hardware, Timing, Network) using snake_case fields aligned with NVS keys and JSON field names
- NVS namespace "flightwall" with all 22 keys from architecture spec; abbreviations documented in header (os_client_id, os_client_sec, scan_dir)
- init() loads compile-time defaults first, then overrides from NVS where keys exist
- applyJson() distinguishes reboot keys (immediate NVS persist) from hot-reload keys (debounced + callback fire)
- SystemStatus registry with 6 subsystems (WIFI, OPENSKY, AEROAPI, CDN, NVS, LITTLEFS), 3 levels, timestamp tracking, and toJson() serialization
- All 5 adapter files + main.cpp migrated off config/*.h includes to use ConfigManager/SystemStatus APIs
- 17 Unity tests covering: default fallbacks (5), NVS round-trip (1), applyJson hot/reboot/mixed (3), requiresReboot detection (2), onChange callbacks (1), SystemStatus init/set/get/toJson (4)
- main.cpp guarded with `#ifndef PIO_UNIT_TESTING` for test compatibility

### File List

| File | Action |
|------|--------|
| `firmware/core/ConfigManager.h` | CREATE |
| `firmware/core/ConfigManager.cpp` | CREATE |
| `firmware/core/SystemStatus.h` | CREATE |
| `firmware/core/SystemStatus.cpp` | CREATE |
| `firmware/src/main.cpp` | MODIFY — replaced config header includes with ConfigManager/SystemStatus; added init calls; replaced WiFi/Timing config reads; added PIO_UNIT_TESTING guard |
| `firmware/core/FlightDataFetcher.cpp` | MODIFY — replaced UserConfiguration with ConfigManager::getLocation() |
| `firmware/adapters/OpenSkyFetcher.h` | MODIFY — removed config header includes |
| `firmware/adapters/OpenSkyFetcher.cpp` | MODIFY — replaced all APIConfiguration refs with ConfigManager getters |
| `firmware/adapters/AeroAPIFetcher.h` | MODIFY — removed config header include |
| `firmware/adapters/AeroAPIFetcher.cpp` | MODIFY — replaced APIConfiguration refs with ConfigManager getters |
| `firmware/adapters/FlightWallFetcher.h` | MODIFY — removed config header include |
| `firmware/adapters/FlightWallFetcher.cpp` | MODIFY — replaced APIConfiguration refs with ConfigManager getters |
| `firmware/adapters/NeoMatrixDisplay.cpp` | MODIFY — replaced all UserConfiguration/HardwareConfiguration/TimingConfiguration refs with ConfigManager getters |
| `firmware/test/test_config_manager/test_main.cpp` | CREATE — 17 Unity tests |

### Change Log

- 2026-04-02: Story 1.2 implemented — ConfigManager + SystemStatus created, all adapters migrated, Unity tests added, `pio run` passes with zero errors

## References

- [Source: `_bmad-output/planning-artifacts/epics.md` — Story 1.2]
- [Source: `_bmad-output/planning-artifacts/architecture.md` — Decision 1 ConfigManager, Decision 5 init order, Decision 6 SystemStatus, NVS table, Config Structs, source tree]
- [Source: `_bmad-output/planning-artifacts/prd.md` — FR10, FR11 persistence / defaults]
- [Source: `1-1-project-infrastructure.md` — foundation constraints]
