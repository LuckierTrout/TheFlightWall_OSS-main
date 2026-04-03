# Story 1.3: FreeRTOS Dual-Core Task Architecture

Status: done

## Story

As a user,
I want the LED display to update smoothly without WiFi or web server interruptions,
so that the flight display never flickers or freezes during network operations.

## Acceptance Criteria

1. **Display task on dedicated core:** The firmware boots with a FreeRTOS display task pinned to Core 0 via `xTaskCreatePinnedToCore`. The main `loop()` continues on Core 1 handling WiFi, API calls, and `ConfigManager::tick()`.

2. **Flight data via queue:** The flight pipeline on Core 1 writes enriched flight data to a FreeRTOS queue via `xQueueOverwrite`. The display task on Core 0 reads the latest data via `xQueuePeek` and renders flight cards without removing the item from the queue.

3. **Config change via atomic flag:** When `ConfigManager::applyJson()` fires on Core 1, a `std::atomic<bool> configChanged` flag is set. The display task on Core 0 checks the flag each frame, and on detection re-reads `getDisplay()` / `getHardware()` / `getTiming()` and applies changes within 1 second (NFR1).

4. **No flicker or freezes:** WiFi operations, HTTP requests, and JSON parsing on Core 1 do not interrupt or delay LED matrix rendering on Core 0. The display maintains consistent frame output with no visible flicker during network operations.

5. **Stability under load:** After 30+ minutes of continuous operation under normal load, there are no crashes, watchdog resets, or heap exhaustion. `ESP.getFreeHeap()` remains above 100KB.

6. **No regression:** The existing fetch → enrich → display pipeline produces identical behavior to pre-refactor. All existing adapter code (OpenSkyFetcher, AeroAPIFetcher, FlightWallFetcher) continues to work via `loop()` on Core 1.

## Tasks / Subtasks

- [x] Task 1: Create shared data structures and globals (AC: #2, #3)
  - [x] Add `#include <atomic>` and `#include <freertos/queue.h>` to main.cpp
  - [x] Define `FlightDisplayData` struct holding `std::vector<FlightInfo>` for queue transfer
  - [x] Declare `QueueHandle_t g_flightQueue` global
  - [x] Declare `std::atomic<bool> g_configChanged(false)` global
  - [x] Register ConfigManager onChange callback that sets `g_configChanged = true`

- [x] Task 2: Implement the display task function (AC: #1, #2, #3, #4)
  - [x] Create `void displayTask(void* pvParameters)` function in main.cpp
  - [x] On entry: read initial config from ConfigManager, store as local copies
  - [x] Main loop: check `g_configChanged` atomic flag — if true, re-read config structs, update brightness/colors/timing
  - [x] Main loop: `xQueuePeek` flight data — if available, render flights via `g_display`
  - [x] Main loop: handle flight cycling timer (display_cycle interval) within the task
  - [x] Main loop: call `vTaskDelay(pdMS_TO_TICKS(50))` for ~20fps frame rate (also feeds watchdog)
  - [x] Subscribe to task watchdog via `esp_task_wdt_add(NULL)` and reset each loop iteration

- [x] Task 3: Refactor setup() to create FreeRTOS task (AC: #1)
  - [x] Initialize `g_flightQueue = xQueueCreate(1, sizeof(FlightDisplayData*))` in setup() after ConfigManager::init()
  - [x] Move display-related initialization (g_display.initialize, showLoading) to happen before task creation
  - [x] Call `xTaskCreatePinnedToCore(displayTask, "display", 8192, NULL, 1, NULL, 0)` — Core 0, priority 1, 8KB stack
  - [x] Remove display rendering calls from loop() — display is now owned by the task

- [x] Task 4: Refactor loop() to produce data for the queue (AC: #2, #6)
  - [x] After `g_fetcher->fetchFlights()` completes, pack results into `FlightDisplayData`
  - [x] Write to queue via `xQueueOverwrite(g_flightQueue, &displayData)`
  - [x] Keep `ConfigManager::tick()` in loop() (NVS debounce runs on Core 1)
  - [x] Keep existing WiFi connection logic in setup() (WiFiManager is a future story)
  - [x] Remove `g_display.displayFlights()` call from loop() — task handles rendering now
  - [x] Remove `delay(10)` — `loop()` naturally yields via the fetch interval timer

- [x] Task 5: Update NeoMatrixDisplay for task-safe usage (AC: #3, #4)
  - [x] NeoMatrixDisplay methods are now only called from the display task (single writer) — no mutex needed
  - [x] Add `updateBrightness(uint8_t)` method or re-read config in task and call `setBrightness()` directly
  - [x] Move flight cycling logic (currentFlightIndex, lastCycleMs) into the display task — display owns its own timing now
  - [x] Ensure `displayMessage()` can still be called from setup() before task starts (for WiFi status messages)

- [x] Task 6: Build verification and stability (AC: #5, #6)
  - [x] `pio run` — zero errors, zero new warnings
  - [x] Verify RAM usage stays reasonable (baseline: 14.7% from story 1.2; expect modest increase for task stack + queue)
  - [x] Add `LOG_I` for task creation, queue creation, config change detection
  - [x] Add `uxTaskGetStackHighWaterMark(NULL)` logging in display task (verbose level) for stack tuning

### Review Findings

- [x] [Review][Patch] Guard queue access when queue creation fails — both `displayTask()` (`xQueuePeek`) and `loop()` (`xQueueOverwrite`) can hit a null `g_flightQueue` if `xQueueCreate` returns null [firmware/src/main.cpp:160-166]
- [x] [Review][Patch] AC3: re-read `getHardware()` on config change — `displayTask` only reloads display/timing today; acceptance criteria require `getDisplay()` / `getHardware()` / `getTiming()` when `g_configChanged` fires. Apply hardware changes (e.g. call `g_display.initialize()` when tile/pin layout changes, or equivalent safe path) within the 1s NFR window [firmware/src/main.cpp:75-82]
- [x] [Review][Patch] Display ownership is broken after task startup — the WiFi state callback on Core 1 still calls `g_display.displayMessage(...)`, so display writes now happen from both cores even though the story requires NeoMatrixDisplay to be owned by the display task only [firmware/src/main.cpp:174-193]
- [x] [Review][Patch] `g_configChanged` uses `load()` then `store(false)` instead of an atomic consume-and-clear, so a config change raised between those two operations can be lost and never applied on the display core [firmware/src/main.cpp:77-80]
- [x] [Review][Patch] Verbose stack watermark logging runs every 50ms frame at the default `LOG_LEVEL=3`, adding continuous serial I/O on the display core and undermining the “no flicker or freezes” goal [firmware/src/main.cpp:118-121]
- [x] [Review][Defer] [firmware/src/main.cpp:228] — `Serial.println` for fetch summary vs story “LOG macros for new code”; Dev Agent Record documents dynamic logging exception — deferred, pre-existing pattern
- [x] [Review][Defer] Cross-core `ConfigManager::get*` while Core 1 mutates config — verify getters vs `applyJson` are safe or document ordering; deferred pending ConfigManager audit

## Dev Notes

### Architecture pattern: Producer-Consumer across cores

This story implements the hybrid inter-task communication pattern from Decision 2 in the architecture doc:

```
Core 1 (loop):                          Core 0 (displayTask):
┌─────────────────────┐                 ┌─────────────────────┐
│ ConfigManager::tick()│                 │ Check configChanged  │
│ fetchFlights()      │                 │   → re-read config  │
│ xQueueOverwrite()───┼──(queue)───────→│ xQueuePeek()        │
│ configChanged=true──┼──(atomic)──────→│ Render flights      │
│                     │                 │ FastLED.show()       │
│ (WiFi, HTTP, JSON)  │                 │ vTaskDelay(50ms)     │
└─────────────────────┘                 └─────────────────────┘
```

**Core 0** = display task (FastLED.show). WiFi system tasks also run on Core 0 by default on ESP32, but the display task uses `vTaskDelay()` which yields properly. FastLED uses the RMT peripheral on ESP32 and does not globally disable interrupts, so Core 0 WiFi system tasks coexist without issues as long as the display task yields regularly.

**Core 1** = Arduino loop() (HTTP, JSON parsing, ConfigManager). This is where all blocking network I/O happens.

### Critical implementation details

**FlightDisplayData struct:** Must be copyable and fit in the queue. Use `std::vector<FlightInfo>` inside. The queue stores a full copy via `memcpy` — but since `FlightInfo` contains `String` members (which use heap pointers), you have two options:
- **Option A (recommended):** Use a `QueueHandle_t` with size `sizeof(FlightDisplayData*)` and pass a pointer to a heap-allocated struct. Use a double-buffer pattern: Core 1 writes to buffer A, overwrite queue with pointer to A, next cycle writes to B, etc.
- **Option B:** Use a mutex-protected shared variable instead of a queue. Simpler but slightly less idiomatic.
- **Do NOT** use `xQueueOverwrite` with a struct containing `String` or `std::vector` — `memcpy` semantics will corrupt heap pointers.

**Recommended approach — pointer-based queue:**
```cpp
struct FlightDisplayData {
    std::vector<FlightInfo> flights;
};

// Double buffer (global)
FlightDisplayData g_flightBuf[2];
uint8_t g_writeBuf = 0;

// Core 1 writes:
g_flightBuf[g_writeBuf] = {flights};
FlightDisplayData* ptr = &g_flightBuf[g_writeBuf];
xQueueOverwrite(g_flightQueue, &ptr);
g_writeBuf ^= 1;  // swap buffer

// Core 0 reads:
FlightDisplayData* ptr;
if (xQueuePeek(g_flightQueue, &ptr, 0) == pdTRUE) {
    // read ptr->flights (safe — Core 1 is writing to the other buffer)
}
```

**Alternative — mutex approach (simpler, also acceptable):**
```cpp
SemaphoreHandle_t g_flightMutex = xSemaphoreCreateMutex();
std::vector<FlightInfo> g_sharedFlights;

// Core 1 writes:
if (xSemaphoreTake(g_flightMutex, pdMS_TO_TICKS(100))) {
    g_sharedFlights = flights;
    xSemaphoreGive(g_flightMutex);
}

// Core 0 reads:
if (xSemaphoreTake(g_flightMutex, pdMS_TO_TICKS(10))) {
    localFlights = g_sharedFlights;
    xSemaphoreGive(g_flightMutex);
}
```

Choose one approach and document the choice in Dev Agent Record.

**Task watchdog:** The ESP32 Arduino framework enables TWDT by default. The display task must either:
- Call `vTaskDelay()` regularly (which yields to idle task, resetting the watchdog), OR
- Explicitly subscribe via `esp_task_wdt_add(NULL)` and call `esp_task_wdt_reset()` each iteration

Using `vTaskDelay(pdMS_TO_TICKS(50))` each frame loop iteration (20fps) is sufficient to keep the watchdog happy. Include `#include "esp_task_wdt.h"` if using explicit watchdog management.

**Stack size 8192 bytes:** Architecture spec says 8192. This is adequate for NeoMatrixDisplay rendering (Adafruit GFX text + FastLED.show via RMT). Monitor with `uxTaskGetStackHighWaterMark()` and adjust if high watermark drops below 1KB.

**delay() vs vTaskDelay():** On ESP32 Arduino, `delay()` internally calls `vTaskDelay()`, so both work in FreeRTOS tasks. Use `vTaskDelay(pdMS_TO_TICKS(ms))` in the display task for clarity and portability.

### What NOT to do

- **Do NOT create a WiFiManager** — that's Story 1.4. Keep existing blocking WiFi.begin() in setup().
- **Do NOT create a WebPortal** — that's Story 1.5. No ESPAsyncWebServer handlers yet.
- **Do NOT add mDNS, NTP, or GPIO handling** — those are later stories.
- **Do NOT move the flight pipeline into a separate task** — it stays in loop() on Core 1.
- **Do NOT split NeoMatrixDisplay into multiple classes** — just ensure it's called from one core only.
- **Do NOT use `new` for the FlightDisplayData buffers** — use global static double-buffer to avoid heap fragmentation.

### Architecture compliance

- Class/method naming: PascalCase classes, camelCase methods, snake_case struct fields [Source: architecture.md — Naming Patterns]
- Logging: `LOG_E` / `LOG_I` / `LOG_V` from `utils/Log.h` — never raw `Serial.println` in new code [Source: architecture.md — Logging Pattern]
- Initialization order: ConfigManager → SystemStatus → (display init) → (create queue) → (create display task) → WiFi → FlightDataFetcher [Source: architecture.md — Decision 5]
- Error pattern: boolean return + SystemStatus reporting [Source: architecture.md — Error Handling]

### Library / framework requirements

- **FreeRTOS** — built into ESP32 Arduino core; no additional dependency needed. Use `<freertos/FreeRTOS.h>`, `<freertos/task.h>`, `<freertos/queue.h>`, `<freertos/semphr.h>`
- **std::atomic** — C++11 standard library, works on ESP32 Xtensa; `#include <atomic>`
- **esp_task_wdt.h** — ESP-IDF component, available in Arduino ESP32; `#include "esp_task_wdt.h"`
- **No new PlatformIO lib_deps required** — all FreeRTOS and atomic features are built-in

### File structure requirements

| File | Action |
|------|--------|
| `firmware/src/main.cpp` | MODIFY — add FreeRTOS includes, create display task, refactor loop() to produce queue data, register configChanged callback |
| `firmware/adapters/NeoMatrixDisplay.h` | MODIFY (minor) — if adding updateBrightness() or similar method |
| `firmware/adapters/NeoMatrixDisplay.cpp` | MODIFY (minor) — if adding new method; ensure no concurrent access issues |

No new files created in this story. This is a refactoring of main.cpp with minor display adapter changes.

### Testing requirements

- **No unit tests required** for this story — FreeRTOS task creation and queue operations are hardware-dependent and cannot be tested in a native test environment
- **Verification:** `pio run` must succeed. RAM/Flash usage should be logged.
- **On-device testing (when available):** Run for 30+ minutes, verify no watchdog resets, no heap exhaustion, display renders smoothly while fetch cycle runs
- **Stack monitoring:** Log `uxTaskGetStackHighWaterMark()` at LOG_V level in the display task

### Previous story intelligence (1.2)

- **ConfigManager** is fully functional: `init()`, `tick()`, `applyJson()`, `getDisplay()`, `getHardware()`, `getTiming()`, `onChange()` all working [Source: 1-2-configmanager-and-systemstatus.md]
- **Debounce approach:** millis-based (`_persistScheduledAt` + `_persistPending`). `ConfigManager::tick()` checks the pending flag — must continue being called from `loop()` on Core 1 [Source: 1.2 Dev Agent Record]
- **`#ifndef PIO_UNIT_TESTING` guard** already exists in main.cpp — preserve it
- **Build baseline:** RAM: 14.7% (48092/327680), Flash: 49.7% (1042625/2097152) — expect modest RAM increase (~8KB for task stack + queue overhead)
- **Review findings from 1.2 (all resolved):**
  - Hot-reload persist path is now serviced via `ConfigManager::tick()` — keep calling it in loop()
  - `display_pin` is NVS-backed but FastLED needs compile-time constant — solved via `configureStripForPin()` switch statement
  - Unknown JSON keys are now rejected (not applied) — `applyJson` returns only valid keys

### Previous story intelligence (1.1)

- **LittleFS** mounts via `LittleFS.begin(true)` in setup() — must remain before ConfigManager::init()
- **Log macros** LOG_E/LOG_I/LOG_V from `utils/Log.h` established — use exclusively
- **PlatformIO build config:** `build_src_filter` includes `+<../core/*.cpp>` and `+<../adapters/*.cpp>` — no changes needed
- **No FreeRTOS headers currently included** — this story adds them for the first time

### Git intelligence

- Repository has no commits — greenfield development; no commit patterns to match

### Latest technical notes

- **ESP32 FreeRTOS stack sizes are in BYTES** (not words) — `xTaskCreatePinnedToCore(..., 8192, ...)` = 8KB stack
- **FastLED on ESP32 uses RMT peripheral** — does not globally disable interrupts like on AVR. Core 0 is safe for display with proper yielding via `vTaskDelay()`
- **WiFi system tasks run on Core 0** by default in ESP32 Arduino — the display task on Core 0 must yield regularly (50ms delay is sufficient) to let WiFi tasks execute
- **`delay()` calls `vTaskDelay()` internally** on ESP32 Arduino — both are safe in FreeRTOS tasks, but prefer explicit `vTaskDelay(pdMS_TO_TICKS(ms))` for clarity in task functions
- **std::atomic<bool>** works correctly across ESP32 dual cores — uses Xtensa hardware atomic instructions
- **xQueueOverwrite requires queue length of 1** — it's designed for "mailbox" semantics (latest value wins)

### Project context reference

- No `project-context.md` found — rely on architecture.md, epics.md, and previous story files

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` — Story 1.3]
- [Source: `_bmad-output/planning-artifacts/architecture.md` — Decision 2 (Inter-Task Communication), Decision 5 (Init Order), AR7, AR16, AR21]
- [Source: `_bmad-output/planning-artifacts/prd.md` — FR38, FR39, NFR1, NFR4, NFR6, NFR16]
- [Source: `1-1-project-infrastructure.md` — foundation constraints]
- [Source: `1-2-configmanager-and-systemstatus.md` — ConfigManager API, tick(), onChange(), review findings]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6 (claude-opus-4-6)

### Implementation Plan

**Approach chosen:** Pointer-based double-buffer queue (Option A from Dev Notes). This avoids memcpy corruption of `String`/`std::vector` members in `FlightInfo`. Two static `FlightDisplayData` buffers are used: Core 1 writes to one while Core 0 reads from the other, with buffer swap on each write cycle.

**Key decisions:**
- Queue stores `FlightDisplayData*` (pointer), not the struct itself — avoids memcpy of non-POD types
- Double buffer (`g_flightBuf[2]`) with `g_writeBuf ^= 1` swap pattern — no heap allocation, no fragmentation
- Explicit `esp_task_wdt_add(NULL)` + `esp_task_wdt_reset()` in display task, plus `vTaskDelay(50ms)` for watchdog and ~20fps frame rate
- Flight cycling logic moved from `NeoMatrixDisplay::displayFlights()` into `displayTask()` — display task owns its own timing state
- Added `renderFlight()` method to NeoMatrixDisplay for single-flight rendering (called by display task with pre-computed index)
- Added `updateBrightness()` method for runtime brightness changes via config change detection
- LOG macros only support 2 args (tag + literal string concat), so dynamic log messages use `Serial.println()` with `#if LOG_LEVEL >= N` guards
- Replaced verbose `Serial.print` debug output in `loop()` with structured LOG_I/LOG_V-style messages
- `displayMessage()` works from `setup()` before task starts (display init happens before task creation)

### Debug Log References

- Build output: `pio run` SUCCESS, 0 errors, 0 new warnings
- RAM: 15.0% (49044 bytes) — delta +952 bytes from 1.2 baseline (14.7% / 48092 bytes)
- Flash: 50.5% (1058265 bytes) — delta +15640 bytes from 1.2 baseline (49.7% / 1042625 bytes)
- Test `test_config_manager`: builds successfully, upload fails (no device connected — expected for hardware tests)

### Completion Notes List

- Implemented dual-core producer-consumer architecture with FreeRTOS task pinned to Core 0
- Display task reads flight data via `xQueuePeek` (non-destructive read) from pointer-based queue
- Config changes detected via `std::atomic<bool>` flag, display task re-reads config within one frame (~50ms)
- All network/fetch operations remain on Core 1 in `loop()`, display rendering isolated on Core 0
- No new PlatformIO dependencies added — all FreeRTOS and atomic features are ESP32 built-ins
- Initialization order preserved: ConfigManager -> SystemStatus -> display init -> queue create -> task create -> WiFi -> FlightDataFetcher
- `#ifndef PIO_UNIT_TESTING` guard preserved
- Existing `displayFlights()` method retained for BaseDisplay interface compatibility
- No unit tests for this story per Dev Notes (FreeRTOS operations are hardware-dependent)

### Change Log

- 2026-04-02: Implemented FreeRTOS dual-core task architecture — refactored main.cpp for producer-consumer pattern, added display task on Core 0, pointer-based double-buffer queue, atomic config change flag, and NeoMatrixDisplay task-safe methods

### File List

- `firmware/src/main.cpp` — MODIFIED: Added FreeRTOS includes, FlightDisplayData struct, double buffer, queue handle, atomic flag, displayTask() function, refactored setup() and loop() for dual-core architecture
- `firmware/adapters/NeoMatrixDisplay.h` — MODIFIED: Added `updateBrightness()` and `renderFlight()` method declarations
- `firmware/adapters/NeoMatrixDisplay.cpp` — MODIFIED: Added `updateBrightness()` and `renderFlight()` method implementations
