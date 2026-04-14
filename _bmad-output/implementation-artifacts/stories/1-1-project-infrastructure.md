# Story 1.1: Project Infrastructure

Status: done

## Story

As a developer,
I want the firmware to use a custom partition table, LittleFS filesystem, and compile-time log macros,
so that the project foundation supports all subsequent features.

## Acceptance Criteria

1. **AC1:** platformio.ini updated with new dependencies and build config — firmware builds successfully via `pio run`
2. **AC2:** Custom partition table allocates ~2MB app + ~1.9MB LittleFS
3. **AC3:** LittleFS mounts successfully on device boot
4. **AC4:** `pio run -t uploadfs` uploads firmware/data/ contents to LittleFS
5. **AC5:** LOG_E/LOG_I/LOG_V macros output to Serial at configured LOG_LEVEL
6. **AC6:** Macros compile to nothing when LOG_LEVEL is below their threshold
7. **AC7:** Existing flight pipeline (fetch → enrich → display) still works — no regression

## Tasks / Subtasks

- [x] Task 1: Update platformio.ini (AC: #1)
  - [x] Add `mathieucarbou/ESPAsyncWebServer @ ^3.6.0` to lib_deps (NOT me-no-dev — Carbou fork fixes memory leaks, used by ESPHome) [Source: architecture.md#New Dependencies]
  - [x] AsyncTCP is pulled automatically as a dependency of ESPAsyncWebServer
  - [x] Add `board_build.filesystem = littlefs` 
  - [x] Add `board_build.partitions = custom_partitions.csv`
  - [x] Add `-D LOG_LEVEL=3` to build_flags (verbose for development)
  - [x] Keep ALL existing lib_deps (FastLED, Adafruit GFX, FastLED NeoMatrix, ArduinoJson)
  - [x] Keep ALL existing build_src_filter and build_flags entries

- [x] Task 2: Create custom partition table (AC: #2)
  - [x] Create file: `firmware/custom_partitions.csv`
  - [x] Content:
    ```
    # Name,    Type,  SubType, Offset,   Size
    nvs,       data,  nvs,     0x9000,   0x5000
    otadata,   data,  ota,     0xe000,   0x2000
    app0,      app,   ota_0,   0x10000,  0x200000
    spiffs,    data,  spiffs,  0x210000, 0x1F0000
    ```
  - [x] Note: SubType is "spiffs" even for LittleFS — this is correct, ESP32 partition table uses "spiffs" as the SubType label for any data filesystem

- [x] Task 3: Create firmware/data/ directory with test file (AC: #3, #4)
  - [x] Create directory: `firmware/data/`
  - [x] Create a minimal test file: `firmware/data/test.txt` containing "FlightWall LittleFS OK"
  - [x] This verifies `pio run -t uploadfs` works and LittleFS mounts

- [x] Task 4: Create utils/Log.h (AC: #5, #6)
  - [x] Create file: `firmware/utils/Log.h`
  - [x] Content:
    ```cpp
    #pragma once
    
    #ifndef LOG_LEVEL
    #define LOG_LEVEL 2
    #endif
    
    #define LOG_E(tag, msg) do { if (LOG_LEVEL >= 1) Serial.println("[" tag "] ERROR: " msg); } while(0)
    #define LOG_I(tag, msg) do { if (LOG_LEVEL >= 2) Serial.println("[" tag "] " msg); } while(0)
    #define LOG_V(tag, msg) do { if (LOG_LEVEL >= 3) Serial.println("[" tag "] " msg); } while(0)
    ```
  - [x] Tag parameter must be a string literal (not a variable) for compile-time concatenation

- [x] Task 5: Add LittleFS mount verification to main.cpp (AC: #3)
  - [x] Add `#include <LittleFS.h>` to main.cpp
  - [x] Add `#include "utils/Log.h"` to main.cpp
  - [x] In setup(), after Serial.begin(), add:
    ```cpp
    if (!LittleFS.begin(true)) {
        LOG_E("Main", "LittleFS mount failed");
    } else {
        LOG_I("Main", "LittleFS mounted");
    }
    ```
  - [x] The `true` parameter formats LittleFS on first use if not yet formatted
  - [x] DO NOT modify any other existing code in main.cpp — the flight pipeline must remain unchanged

- [x] Task 6: Build and verify (AC: #1, #7)
  - [x] Run `pio run` — must compile with zero errors
  - [ ] Run `pio run -t uploadfs` — must upload data/ contents (requires physical device)
  - [ ] Flash firmware to ESP32 — device must boot, mount LittleFS, connect WiFi, and display flights exactly as before (requires physical device)
  - [ ] Verify Serial output shows LOG_I messages from LittleFS mount (requires physical device)
  - [ ] Verify the existing flight fetch → enrich → display cycle works identically (requires physical device)

### Review Findings

- [x] [Review][Patch] Log.h is not self-contained and depends on include order [firmware/utils/Log.h:7]

## Dev Notes

### Critical Warnings

- **DO NOT modify existing source files** except main.cpp (LittleFS mount + Log.h include only)
- **DO NOT change** the existing lib_deps — only ADD new ones
- **DO NOT change** the existing build_src_filter or build_flags — only ADD new entries
- **DO NOT create** ConfigManager, WiFiManager, WebPortal, or any other new component — those are future stories
- The partition table SubType "spiffs" is used even for LittleFS — this is NOT a bug, it's how ESP32 partition tables work
- `board_build.filesystem = littlefs` tells PlatformIO to use LittleFS driver regardless of SubType label

### Architecture Compliance

- **Library fork:** MUST use `mathieucarbou/ESPAsyncWebServer` — the original `me-no-dev/ESPAsyncWebServer` has known memory leaks that cause crashes on long-running devices [Source: architecture.md#New Dependencies]
- **Partition table:** Must match the exact layout in architecture.md — 2MB app + ~1.9MB LittleFS. This is a non-default partition that all subsequent stories depend on [Source: architecture.md#Flash Partition Layout]
- **Log macros:** Must use the `do { } while(0)` wrapper pattern for safe macro expansion in all control flow contexts [Source: architecture.md#Logging Pattern]
- **LOG_LEVEL default:** 2 (info) in the header, overridable via `-D LOG_LEVEL=3` in build_flags [Source: architecture.md#Logging Pattern]

### Existing Code Understanding

The current firmware structure (`firmware/src/main.cpp`):
- `setup()`: Serial init → display init → WiFi connect → create FlightDataFetcher
- `loop()`: Timer-based fetch cycle calling `g_fetcher->fetchFlights()` → `g_display.displayFlights()`
- All config is compile-time via `config/` namespace headers
- No FreeRTOS tasks yet — single-threaded loop()

**This story adds infrastructure alongside the existing code without changing behavior.**

### File Changes Summary

| File | Action | Notes |
|------|--------|-------|
| `firmware/platformio.ini` | MODIFY | Add lib_deps, filesystem, partitions, LOG_LEVEL flag |
| `firmware/custom_partitions.csv` | CREATE | Custom 2MB app + 1.9MB LittleFS partition |
| `firmware/data/test.txt` | CREATE | Minimal test file for LittleFS upload verification |
| `firmware/utils/Log.h` | CREATE | Compile-time log level macros |
| `firmware/src/main.cpp` | MODIFY | Add LittleFS mount + Log.h include ONLY |

### Project Structure Notes

- All new files go under `firmware/` — the project root is NOT the firmware root
- `firmware/utils/` already exists (contains GeoUtils.h) — Log.h goes alongside it
- `firmware/data/` does NOT exist yet — must be created
- `firmware/custom_partitions.csv` goes at the same level as platformio.ini

### References

- [Source: architecture.md#New Dependencies] — mathieucarbou fork requirement
- [Source: architecture.md#Flash Partition Layout] — custom_partitions.csv exact content
- [Source: architecture.md#Logging Pattern] — Log.h specification
- [Source: architecture.md#Web Assets Location] — firmware/data/ directory
- [Source: architecture.md#Development Workflow] — pio run -t uploadfs command
- [Source: epics.md#Story 1.1] — Acceptance criteria
- [Source: prd.md#Memory Budget] — 2MB app + 2MB LittleFS partition strategy

## Dev Agent Record

### Agent Model Used
Claude Opus 4.6 (1M context)

### Debug Log References
- `pio run` build succeeded: 0 errors, 0 warnings. RAM: 8.6% (28252/327680 bytes), Flash: 32.5% (681341/2097152 bytes)
- No existing test directory (`firmware/test/`) — no regression tests to run. Build success confirms no compilation regressions.

### Completion Notes List
- ✅ Task 1: Updated platformio.ini — added ESPAsyncWebServer (Carbou fork), littlefs filesystem, custom partitions, LOG_LEVEL=3 build flag. All existing deps/flags preserved.
- ✅ Task 2: Created custom_partitions.csv with 2MB app (0x200000) + ~1.9MB LittleFS (0x1F0000) partition layout. SubType "spiffs" used as required by ESP32 partition table spec.
- ✅ Task 3: Created firmware/data/test.txt with "FlightWall LittleFS OK" for uploadfs verification.
- ✅ Task 4: Created firmware/utils/Log.h with LOG_E/LOG_I/LOG_V macros using do-while(0) pattern. Default LOG_LEVEL=2, overridable via build_flags.
- ✅ Task 5: Added LittleFS.h and Log.h includes to main.cpp. Added LittleFS.begin(true) mount with LOG_E/LOG_I output in setup() after Serial.begin(). No other existing code modified.
- ✅ Task 6: `pio run` compiles with zero errors. Device-dependent subtasks (uploadfs, flash, serial verification) require physical ESP32.

### Change Log
- 2026-04-02: Story 1.1 implementation complete — project infrastructure (partition table, LittleFS, logging macros, ESPAsyncWebServer dependency)

### File List
- firmware/platformio.ini (MODIFIED) — added ESPAsyncWebServer lib_dep, littlefs filesystem, custom partitions, LOG_LEVEL build flag
- firmware/custom_partitions.csv (CREATED) — custom partition table: nvs + otadata + 2MB app + 1.9MB LittleFS
- firmware/data/test.txt (CREATED) — LittleFS upload test file
- firmware/utils/Log.h (CREATED) — compile-time LOG_E/LOG_I/LOG_V macros
- firmware/src/main.cpp (MODIFIED) — added LittleFS mount and Log.h include in setup()
