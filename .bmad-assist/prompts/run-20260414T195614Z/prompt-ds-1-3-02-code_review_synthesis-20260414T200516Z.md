<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-1 -->
<!-- Story: 3 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260414T200516Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story ds-1.3

You are synthesizing 2 independent code review findings.

Your mission:
1. VERIFY each issue raised by reviewers
   - Cross-reference with project_context.md (ground truth)
   - Cross-reference with git diff and source files
   - Identify false positives (issues that aren't real problems)
   - Confirm valid issues with evidence

2. PRIORITIZE real issues by severity
   - Critical: Security vulnerabilities, data corruption risks
   - High: Bugs, logic errors, missing error handling
   - Medium: Code quality issues, performance concerns
   - Low: Style issues, minor improvements

3. SYNTHESIZE findings
   - Merge duplicate issues from different reviewers
   - Note reviewer consensus (if 3+ agree, high confidence)
   - Highlight unique insights from individual reviewers

4. APPLY source code fixes
   - You have WRITE PERMISSION to modify SOURCE CODE files
   - CRITICAL: Before using Edit tool, ALWAYS Read the target file first
   - Use EXACT content from Read tool output as old_string, NOT content from this prompt
   - If Read output is truncated, use offset/limit parameters to locate the target section
   - Apply fixes for verified issues
   - Do NOT modify the story file (only Dev Agent Record if needed)
   - Document what you changed and why

Output format:
## Synthesis Summary
## Issues Verified (by severity)
## Issues Dismissed (false positives with reasoning)
## Source Code Fixes Applied

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
<file id="e8f5c877" path="_bmad-output/implementation-artifacts/stories/ds-1-3-moderegistry-and-static-registration-system.md" label="DOCUMENTATION"><![CDATA[

# Story ds-1.3: ModeRegistry & Static Registration System

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a firmware developer,
I want a ModeRegistry that manages a static table of display modes with activation lifecycle, heap validation, and fallback,
So that modes can be registered at compile time and switched safely at runtime.

## Acceptance Criteria

1. **Given** `interfaces/DisplayMode.h` exists (Story ds-1.1), **When** ModeRegistry is implemented, **Then** `core/ModeRegistry.h` and `core/ModeRegistry.cpp` define `SwitchState`, `ModeEntry`, and `ModeRegistry` exactly per architecture Decision D2 (public API: `init`, `requestSwitch`, `tick`, `getActiveMode`, `getActiveModeId`, `getModeTable`, `getModeCount`, `getSwitchState`, `getLastError`)

2. **Given** the static registration pattern, **When** the firmware links, **Then** a `static const ModeEntry MODE_TABLE[]` (and `MODE_COUNT`) lives in `src/main.cpp` and is passed to `ModeRegistry::init(MODE_TABLE, MODE_COUNT)` during `setup()` after display hardware / NeoMatrixDisplay init is available (registry must not run before matrix exists — `RenderContext` needs a valid matrix pointer when modes eventually render)

3. **Given** Classic and Live modes are not fully implemented until ds-1.4 / ds-2, **When** this story ships, **Then** `modes/ClassicCardMode.h/.cpp` and `modes/LiveFlightCardMode.h/.cpp` exist as **minimal stub** `DisplayMode` implementations (empty or single-color clear in `render()`, `init()` returns true, `teardown()` no-op, static `MEMORY_REQUIREMENT` constexpr used by `ModeEntry::memoryRequirement`) so the table shape matches the architecture example — ds-1.4 replaces Classic body; ds-2 replaces Live body **without renaming files or mode IDs** (`classic_card`, `live_flight`)

4. **Given** NFR P4 requires enumeration with no heap allocation, **When** a caller needs mode metadata, **Then** `ModeRegistry::getModeTable()` and `getModeCount()` expose the const table; **and** `ModeRegistry::enumerate(void (*cb)(const char* id, const char* displayName, uint8_t index, void* user), void* user)` (or equivalent zero-allocation callback API) iterates all entries in O(n) with **no `String` / `new` / `malloc`** inside the registry — completes in under 10 ms for two modes on ESP32

5. **Given** architecture Decision D2 switch flow, **When** a mode switch is processed inside `tick()` on Core 0, **Then** the sequence is: observe `_requestedIndex` ≠ `_activeModeIndex` → `SWITCHING` → `teardown()` on active → measure `ESP.getFreeHeap()` → compare to `memoryRequirement() + MODE_SWITCH_HEAP_MARGIN` (30 KiB) → on success: delete old instance, `factory()` new mode, `init(ctx)`; on heap failure or `init()` false: restore previous mode per D2 (re-create previous via factory if old object was deleted, or re-`init` if architecture’s teardown-before-delete pattern is followed — **implement the documented teardown-before-delete / restore semantics verbatim**)

6. **Given** FR7 / NFR S2, **When** multiple `requestSwitch()` calls occur before `tick()` consumes them, **Then** semantics are **queue-of-one, latest wins**: `_requestedIndex` holds at most one pending target; a new valid request overwrites the previous pending index; **do not** implement an unbounded queue

7. **Given** AR2 (cross-core), **When** `requestSwitch()` is invoked from Core 1 (simulated in tests or future WebPortal), **Then** only `std::atomic<uint8_t> _requestedIndex` (or equivalent single atomic) bridges cores for the request — no mutex from WebPortal; mirror the `g_configChanged` pattern documented in `main.cpp` (Core 1 sets intent, Core 0 display path executes)

8. **Given** NVS persistence in Decision D2, **When** a switch completes successfully, **Then** `ModeRegistry` sets a debounced persistence flag; after **≥2000 ms** since the last switch, a subsequent `tick()` calls **`ConfigManager::setDisplayMode(const char* modeId)`** (new API — add to `ConfigManager`) using a **≤15-character** NVS key (e.g. `disp_mode`). **Also add `ConfigManager::getDisplayMode()`** returning default `"classic_card"` when missing or invalid

9. **Given** the project build layout, **When** new translation units compile, **Then** `platformio.ini` `build_src_filter` includes `+<../modes/*.cpp>` and `+<../core/*.cpp>` already covers `ModeRegistry.cpp` (verify `core/` is present — add if missing)

10. **Given** `requestSwitch(const char* modeId)` contract, **When** `modeId` is unknown, **Then** return `false`, set a stable `getLastError()` message (fixed char buffer, no `String`), and leave active mode unchanged

11. **Given** display pipeline story ds-1.5 defers full integration, **When** this story completes, **Then** `ModeRegistry::tick()` is **not** yet wired into `displayTask` to replace the existing flight render path (no pixel-output change vs Foundation in this story) — unit tests and optional manual test harness exercise `tick()`; `setup()` may call `init` + `getDisplayMode` + `requestSwitch` only if the first `tick()` will run before user-visible frames depend on it; **preferred minimal integration:** `ModeRegistry::init` in `setup()` **without** boot `requestSwitch` until ds-1.5, **or** init + requestSwitch with **one** guarded `tick()` from setup for bring-up only if required by tests — document chosen approach in Dev Agent Record

12. **Given** `firmware/test/test_mode_registry/test_main.cpp`, **When** implementation exists, **Then** replace `TEST_IGNORE_MESSAGE` with real tests **where feasible on-device**; **fix** the skeleton assumption in `test_switch_rejected_while_switching` — FR7 requires **latest-wins**, not “reject while switching”; align tests with epic + architecture

## Tasks / Subtasks

- [x] Task 1: ModeRegistry core (AC: #1, #5, #6, #7, #10)
  - [x] 1.1: Add `core/ModeRegistry.h` with `ModeEntry`, `SwitchState`, `ModeRegistry` per Decision D2
  - [x] 1.2: Implement `core/ModeRegistry.cpp` — static storage, `init`, `requestSwitch`, `tick`, getters, `_lastError[64]` buffer
  - [x] 1.3: `constexpr uint32_t MODE_SWITCH_HEAP_MARGIN = 30 * 1024;` (or in header) — use in heap guard
  - [x] 1.4: Implement latest-wins pending index semantics (atomic store of table index, validated against `MODE_COUNT`)

- [x] Task 2: ConfigManager display mode NVS (AC: #8)
  - [x] 2.1: Add `getDisplayMode()` / `setDisplayMode(const char*)` declarations to `ConfigManager.h`
  - [x] 2.2: Standalone NVS read/write using key `disp_mode` (9 chars, within 15-char limit) in `flightwall` namespace. Not integrated into the settings cache since mode is managed by ModeRegistry, not the settings API.

- [x] Task 3: Stub modes + MODE_TABLE (AC: #2, #3, #9)
  - [x] 3.1: Create `firmware/modes/ClassicCardMode.h`, `ClassicCardMode.cpp` — stub `DisplayMode`, `static constexpr uint32_t MEMORY_REQUIREMENT = 64` (vtable + cycling state fields for ds-1.4)
  - [x] 3.2: Create `firmware/modes/LiveFlightCardMode.h`, `LiveFlightCardMode.cpp` — same pattern, `MEMORY_REQUIREMENT = 96` (distinct for test variety)
  - [x] 3.3: In `main.cpp`, factory functions + `memoryRequirement` wrappers + `MODE_TABLE[]` + `MODE_COUNT`
  - [x] 3.4: Update `platformio.ini` `build_src_filter` for `modes/` and `-I modes` build flag

- [x] Task 4: Enumeration API (AC: #4)
  - [x] 4.1: Implement zero-heap `enumerate` (callback style) in addition to `getModeTable` / `getModeCount`

- [x] Task 5: Tests + build (AC: #11, #12)
  - [x] 5.1: Enabled real includes; implemented MockModeA/MockModeB with lifecycle tracking + production mode table tests
  - [x] 5.2: Corrected `test_switch_rejected_while_switching` → `test_latest_wins_overwrites_pending_request` per FR7/AC #6
  - [x] 5.3: `pio run` SUCCESS (78.4% flash), `pio test --filter test_mode_registry --without-uploading --without-testing` BUILD SUCCESS

## Dev Notes

### Architecture compliance (must follow)

- **Decision D2** — full `ModeRegistry` design, switch states, NVS debounce, cross-core atomic, teardown/heap/delete/init ordering [Source: `_bmad-output/planning-artifacts/architecture.md`, Decision D2]
- **Decision D1** — `DisplayMode` / `RenderContext` contracts: `init(const RenderContext&)` returns `bool`; `render` receives `const std::vector<FlightInfo>&`; modes must not call `FastLED.show()` [Source: `interfaces/DisplayMode.h`, architecture Decision D1]
- **AR5** — adding a future mode = new `modes/*.h/.cpp` + one `ModeEntry` line in `MODE_TABLE` [Source: epic ds-1, architecture anti-pattern table]
- **AR2** — Core 1 → Core 0 via atomics only for switch requests (same philosophy as `g_configChanged`) [Source: `firmware/src/main.cpp` ~L62–63, architecture]

### Epic ↔ implementation naming

- Epic text says `ModeRegistry::activate(id)` — the implemented API is **`requestSwitch` + `tick`** (cooperative serialization on the display core). Treat “activate” in requirements as this split API [Source: epic-ds-1.md Story ds-1.3, architecture D4]

### Memory requirement (not virtual)

- Per ds-1.1 story Dev Notes and D2: **`getMemoryRequirement()` is not a virtual on `DisplayMode`**. Expose heap need via `ModeEntry::memoryRequirement()` function pointer and `static constexpr uint32_t MEMORY_REQUIREMENT` on each mode class [Source: `ds-1-1-heap-baseline-measurement-and-core-abstractions.md` Dev Notes, architecture D2]

### Dependency on ds-1.2 (DisplayUtils)

- Stub modes should not duplicate NeoMatrix flight rendering; they may leave `render()` empty or `matrix->clear()` only. When ds-1.4 wires real Classic card rendering, it will use `DisplayUtils` [Source: `ds-1-2-displayutils-extraction.md`]

### Out of scope (defer)

- **`prepareForOTA()`** — referenced in ignored Unity tests; not required by epic ds-1.3. Leave test ignored or stub later when OTA + registry integration is scheduled [Source: `firmware/test/test_mode_registry/test_main.cpp`]
- **WebPortal `POST` mode switch** — epic ds-3; this story only ensures `requestSwitch` is safe to call from Core 1 when wired later
- **Replacing `displayTask` render path** — ds-1.5

### File structure

| Action | Path |
|--------|------|
| New | `firmware/core/ModeRegistry.h`, `firmware/core/ModeRegistry.cpp` |
| New | `firmware/modes/ClassicCardMode.h`, `ClassicCardMode.cpp`, `LiveFlightCardMode.h`, `LiveFlightCardMode.cpp` |
| Edit | `firmware/src/main.cpp` — `MODE_TABLE`, factories, `ModeRegistry::init` |
| Edit | `firmware/core/ConfigManager.h`, `ConfigManager.cpp` — display mode NVS |
| Edit | `firmware/platformio.ini` — `build_src_filter` |
| Edit | `firmware/test/test_mode_registry/test_main.cpp` — enable tests |

### Testing standards

- Unity on-device tests under `firmware/test/test_mode_registry/`; reuse `fixtures/test_helpers.h` / NVS clear patterns from sibling tests
- After any `data-src/` change (N/A here), regenerate `.gz` per `AGENTS.md` — not expected for this story

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-1.md` (Story ds-1.3)
- Architecture: `_bmad-output/planning-artifacts/architecture.md` (D1, D2, D4, file tree)
- Prior stories: `_bmad-output/implementation-artifacts/stories/ds-1-1-heap-baseline-measurement-and-core-abstractions.md`, `ds-1-2-displayutils-extraction.md`
- Project rules: `_bmad-output/project-context.md`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Implementation Plan

- **Minimal integration approach chosen (AC #11):** `ModeRegistry::init(MODE_TABLE, MODE_COUNT)` called in `setup()` after `g_display.initialize()`. No boot `requestSwitch` — deferred to ds-1.5 when `displayTask` will wire `ModeRegistry::tick()` into the render loop. This avoids any pixel-output changes vs Foundation.
- **ConfigManager display mode API:** Standalone `getDisplayMode()`/`setDisplayMode()` using dedicated NVS key `disp_mode` — not integrated into settings cache or `dumpSettingsJson` since the mode is owned by ModeRegistry, not the REST settings API.
- **LOG macros:** Project uses 2-arg LOG macros (tag, msg with string literal concatenation). Used `Serial.printf` in `#if LOG_LEVEL >= N` guards for parameterized messages.
- **Teardown-before-delete pattern:** Implemented verbatim per D2. On heap failure, previous mode shell is re-init'd. On init failure, previous mode is re-created via factory.

### Debug Log References

- Build: `pio run` — SUCCESS, 78.4% flash (1,232,976 / 1,572,864 bytes)
- Test build: `pio test --filter test_mode_registry --without-uploading --without-testing` — BUILD SUCCESS

### Completion Notes List

- Created ModeRegistry with full D2 switch lifecycle: teardown → heap guard → delete → factory → init, with fallback restoration
- Created ClassicCardMode and LiveFlightCardMode stubs with zone descriptors and memory requirements
- Added MODE_TABLE with 2 entries in main.cpp + ModeRegistry::init in setup()
- Added ConfigManager::getDisplayMode()/setDisplayMode() with NVS key "disp_mode"
- Implemented zero-allocation enumerate() callback API
- Enabled 18 real tests (+ 2 IGNORE for heap-extreme/OTA), fixed latest-wins test per FR7
- All builds pass, no regressions in existing test suites

### File List

- New: `firmware/core/ModeRegistry.h`
- New: `firmware/core/ModeRegistry.cpp`
- New: `firmware/modes/ClassicCardMode.h`
- New: `firmware/modes/ClassicCardMode.cpp`
- New: `firmware/modes/LiveFlightCardMode.h`
- New: `firmware/modes/LiveFlightCardMode.cpp`
- Modified: `firmware/core/ConfigManager.h` — added getDisplayMode/setDisplayMode declarations
- Modified: `firmware/core/ConfigManager.cpp` — added getDisplayMode/setDisplayMode implementations
- Modified: `firmware/src/main.cpp` — added MODE_TABLE, factories, ModeRegistry includes and init
- Modified: `firmware/platformio.ini` — added +<../modes/*.cpp> and -I modes
- Modified: `firmware/test/test_mode_registry/test_main.cpp` — enabled real tests with mock modes

### Change Log

- 2026-04-13: Implemented ModeRegistry & Static Registration System (Story ds-1.3)

## Previous story intelligence

- **ds-1.1** locked `DisplayMode` API (`init` → `bool`, `getZoneDescriptor`, `getSettingsSchema`, `RenderContext` fields). ModeRegistry must match these signatures when calling `init` / `teardown` / `render`.
- **ds-1.2** moved drawing helpers to `DisplayUtils`; stub modes should not pull large NeoMatrix logic into this story.
- **ds-1.1** clarified heap baseline is logged in `setup()`, not post-WiFi-connect; registry init ordering should remain consistent with “matrix exists before first real `tick(ctx, …)`”.

## Git intelligence summary

Recent commits are mostly tooling/docs (`bmad-assist.yaml`, `AGENTS.md`, sprint metadata). No conflicting ModeRegistry implementation on branch — greenfield `core/ModeRegistry.*`.

## Latest tech information

- **ESP32 Arduino / arduino-esp32**: `ESP.getFreeHeap()` for free heap; behavior is dynamic — heap guard must use live measurement at switch time, not compile-time constants.
- **`std::atomic<uint8_t>`** on ESP32: use `#include <atomic>`; ensure request side only writes index and display side reads — avoid holding locks across `tick()` rendering.

## Project context reference

See `_bmad-output/project-context.md` — core pinning (display Core 0, network Core 1), `pio run` from `firmware/`, hexagonal layout under `firmware/core`, `firmware/adapters`, `firmware/interfaces`, `firmware/modes`.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-1-3-moderegistry-and-static-registration-system.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 3 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **9** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task Completion Lie / AC Deviation: Classic/Live modes are fully implemented instead of minimal stubs | `firmware/modes/ClassicCardMode.cpp` | +3.0 |
| 🔴 CRITICAL | Scope Creep / Architecture Breach: ModeRegistry::tick() is fully wired into displayTask | `firmware/src/main.cpp:527` | +3.0 |
| 🔴 CRITICAL | Massive Future Epic Dump: Code from Delight Epic implemented in Display System story | `firmware/modes/ClockMode.cpp` | +3.0 |
| 🟠 IMPORTANT | Hidden Bug: Active mode request leaves dangling state in _requestedIndex | `firmware/core/ModeRegistry.cpp:322` | +1.0 |
| 🟠 IMPORTANT | SOLID Violation (SRP): executeSwitch is a monolith with inline fade logic | `firmware/core/ModeRegistry.cpp:78` | +1.0 |
| 🟠 IMPORTANT | Performance Footgun: Manual byte-by-byte CRGB buffer copy instead of memcpy | `firmware/core/ModeRegistry.cpp:111` | +1.0 |
| 🟠 IMPORTANT | Lying Tests: test_main.cpp asserts on future epic features (dl-1.1, dl-2.1) | `firmware/test/test_mode_registry/test_main.cpp:468` | +1.0 |
| 🟢 CLEAN PASS | Security Vulnerabilities | - | -0.5 |
| 🟢 CLEAN PASS | Type Safety Analysis | - | -0.5 |

### Evidence Score: 12.0

| Score | Verdict |
|-------|---------|
| **12.0** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] Task Completion Lie & Scope Creep:** Task 3.1 & 3.2 are marked complete as "stub DisplayMode". AC #3 requires `ClassicCardMode` and `LiveFlightCardMode` to be minimal stubs. Instead, they are FULLY implemented with complex text truncation, adaptive layout logic, and vertical trend rendering (features from stories ds-1.4 and ds-2.x). The Dev Agent lied on the checklist.
  - 📍 `firmware/modes/ClassicCardMode.cpp:1`
  - 💡 Fix: Revert both modes to minimal stubs (`fillScreen(0)` or empty body) as explicitly required by ds-1.3.

- **[9/10] Architecture Violation / Integration Leak:** AC #11 explicitly requires `ModeRegistry::tick()` to NOT be wired into `displayTask` to replace the flight render path. `main.cpp` fully wires `tick()` and removes `renderFlight`, violating the isolated integration constraint.
  - 📍 `firmware/src/main.cpp:527`
  - 💡 Fix: Remove the `tick()` call from `displayTask` and restore `g_display.renderFlight` until story ds-1.5.

- **[8/10] Single Responsibility Principle (SRP) Violation:** `ModeRegistry::executeSwitch` is a >150 line monolith handling heap checks, NVS debouncing, mode instantiations, error recovery, AND the fade transition blend loop. Architecture DL1 specifically required an `_executeFadeTransition` private method.
  - 📍 `firmware/core/ModeRegistry.cpp:78`
  - 💡 Fix: Extract the fade transition loop into `_executeFadeTransition(ctx, flights)` to decouple it from the instantiation lifecycle.

---

## ⚡ Performance & Scalability

- **[Medium] Manual CRGB frame buffer copy:** In `executeSwitch`, the outgoing snapshot buffer captures the `FastLED.leds()` array by manually looping over 5,120 pixels and assigning `[i * 3] = leds[i].r`. This burns unnecessary CPU cycles on a critical path.
  - 📍 `firmware/core/ModeRegistry.cpp:111`
  - 💡 Fix: Use `memcpy(fadeSnapshotBuf, leds, copyCount * 3)` as specified by the DL1 architecture documentation.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `requestSwitch` leaves stale state. If `requestSwitch()` is called with the *currently active mode*, it updates `_requestedIndex`. But `tick()` bypasses clearing it because `requested == _activeModeIndex`. This leaves `_requestedIndex` permanently set to the active mode. If `_activeModeIndex` changes externally later, this stale request may suddenly execute.
  - 📍 `firmware/core/ModeRegistry.cpp:322`
  - 🔄 Reproduction: Call `requestSwitch("classic_card")` when `classic_card` is active. Wait for `tick()`. Then trigger an external state change like OTA fallback or idle fallback. The stale request will unintentionally snap the system back.

- **🎭 Lying Test:** test_mode_registry/test_main.cpp
  - 📍 `firmware/test/test_mode_registry/test_main.cpp:468`
  - 🤥 Why it lies: The test file is heavily asserting on `ClockMode`, `DeparturesBoardMode`, Fade Transitions, and Per-mode Settings API (dl-1.x through dl-5.x). These are features for the *Delight* epic. By testing them here, the suite acts as a massive integration test for future out-of-scope code instead of validating the constrained boundaries of `ds-1.3`.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Massive Future Epic Code Dump. Code from the Delight Epic (`dl-1.x`, `dl-2.x`, `dl-3.x`, `dl-4.x`, `dl-7.x`), including `ClockMode`, `DeparturesBoardMode`, `OTAUpdater`, and `ModeOrchestrator`, was implemented inside this Display System `ds-1.3` PR.
  - 📍 `firmware/core/ModeOrchestrator.cpp:1`
  - 💥 Explosion radius: High. Destroys PR reviewability, breaks story isolation, and introduces untested future state machines into the current branch.

---

## 🛠️ Suggested Fixes

### 1. Fix Stale Request State in `tick()`

**File:** `firmware/core/ModeRegistry.cpp`
**Issue:** `_requestedIndex` is never cleared if the target matches the active mode, leaving a dangling cross-core request.

**Corrected code:**
```cpp
void ModeRegistry::tick(const RenderContext& ctx,
                        const std::vector<FlightInfo>& flights) {
    uint8_t requested = _requestedIndex.load();
    if (requested != MODE_INDEX_NONE) {
        _requestedIndex.store(MODE_INDEX_NONE);
        if (requested != _activeModeIndex) {
            executeSwitch(requested, ctx, flights);
        }
    }
    // ...
```

### 2. Optimize CRGB Snapshot Copy

**File:** `firmware/core/ModeRegistry.cpp`
**Issue:** Manual byte-by-byte assignment of the pixel buffer.

**Corrected code:**
```cpp
            if (fadeSnapshotBuf != nullptr) {
                CRGB* leds = FastLED.leds();
                int numLeds = FastLED.size();
                size_t copyCount = (fadePixelCount <= (size_t)numLeds) ? fadePixelCount : (size_t)numLeds;
                
                // Copy natively using memcpy for maximum performance
                memcpy(fadeSnapshotBuf, leds, copyCount * 3);
                
                if (copyCount < fadePixelCount) {
                    memset(fadeSnapshotBuf + copyCount * 3, 0,
                           (fadePixelCount - copyCount) * 3);
                }
                // ...
```

**Review Actions:**
- Issues Found: 9
- Issues Fixed: 0
- Action Items Created: 0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review: `ds-1.3`**

**Story:** [ds-1-3-moderegistry-and-static-registration-system.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-1-3-moderegistry-and-static-registration-system.md:1)

## Review Summary

| Category | Issues Found |
|---|---:|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 6 |
| Task Completion Lies | 0 |
| SOLID / Contract Violations | 2 |
| Hidden Bugs | 3 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

## Evidence Score

| Severity | Finding | Source | Score |
|---|---|---|---:|
| 🔴 CRITICAL | Same-mode latest-wins request is never consumed and can replay later | `firmware/core/ModeRegistry.cpp` | +3 |
| 🟠 IMPORTANT | `REQUESTED` state is dead; API reports `idle` while a switch is pending | `firmware/core/ModeRegistry.cpp`, `firmware/adapters/WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | Cross-core contract is broken by unsynchronized `_switchState` and `_lastError` | `firmware/core/ModeRegistry.h/.cpp` | +1 |
| 🟠 IMPORTANT | `ConfigManager::getDisplayMode()` does not honor “missing or invalid => classic_card” | `firmware/core/ConfigManager.cpp` | +1 |
| 🟠 IMPORTANT | Heap-guard restore path ignores failed re-init of the previous mode | `firmware/core/ModeRegistry.cpp` | +1 |
| 🟠 IMPORTANT | Heap-guard/state-transition coverage is still missing where the real bug risk lives | `firmware/test/test_mode_registry/test_main.cpp` | +1 |
| 🟡 MINOR | Story record and file list are stale against the product code now on disk | story markdown vs `firmware/src/main.cpp`, `firmware/modes/*` | +0.3 |
| 🟢 CLEAN PASS | No material security, performance, style, or type-safety findings in reviewed files | 4 clean categories | -2.0 |

**Evidence Score:** `6.3`  
**Verdict:** `MAJOR REWORK`

## Findings

1. **[CRITICAL] Queue-of-one semantics are wrong when the newest request equals the current mode.**  
`tick()` only clears `_requestedIndex` when `requested != _activeModeIndex`, so a sequence like active `A` -> request `B` -> request `A` leaves `A` stuck in `_requestedIndex` forever instead of being consumed as the winning request. The latent request can fire later after some unrelated mode change. Evidence: [ModeRegistry.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeRegistry.cpp:320), [test_main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/test/test_mode_registry/test_main.cpp:355).

2. **[IMPORTANT] `SwitchState::REQUESTED` is unreachable, so the public state API lies.**  
`requestSwitch()` only stores `_requestedIndex`; it never sets `_switchState = REQUESTED`. The only assignments are `IDLE` and `SWITCHING`, which means `/api/display/mode` and `/api/display/modes` report `idle` immediately after a valid request even though work is queued on Core 0. Evidence: [ModeRegistry.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeRegistry.cpp:55), [ModeRegistry.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeRegistry.cpp:78), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:412).

3. **[IMPORTANT] AC #7’s “single atomic bridge” contract is already violated.**  
The story explicitly says only `_requestedIndex` should bridge cores, but current code also shares `_switchState` and `_lastError` across Core 0/Core 1 with no atomic or mutex protection. `WebPortal` reads both while `requestSwitch()` / `executeSwitch()` mutate them, so the API can observe stale or torn state/error data. Evidence: [ModeRegistry.h](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeRegistry.h:97), [ModeRegistry.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeRegistry.cpp:148), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:1178).

4. **[IMPORTANT] `ConfigManager::getDisplayMode()` does not implement AC #8.**  
The AC requires defaulting to `"classic_card"` when the stored mode is missing **or invalid**. The implementation only handles missing/empty values and returns any other arbitrary string verbatim, forcing `main.cpp` to add a second validation layer later. That means every other caller can still consume poisoned mode IDs. Evidence: [ConfigManager.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ConfigManager.cpp:690), [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:790), [story AC #8](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-1-3-moderegistry-and-static-registration-system.md:29).

5. **[IMPORTANT] The teardown-before-delete restore path is incomplete on heap-guard failure.**  
If heap is insufficient, the code calls `_activeMode->init(ctx)` on the previous mode and ignores the return value. If that re-init fails, the registry still returns to `IDLE` with a non-null `_activeMode`, so the display path will keep calling `render()` on a mode that never successfully reinitialized. Evidence: [ModeRegistry.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ModeRegistry.cpp:155).

6. **[IMPORTANT] The tests still dodge the hard failure modes.**  
`test_heap_guard_rejects_insufficient_memory()` is still ignored even though the story required replacing ignores where feasible, and this is trivially feasible with a mock mode that advertises a huge `MEMORY_REQUIREMENT`. The “latest wins” test is also too weak: it checks the active ID after the tick, but not that the pending request was actually drained, so it completely misses Finding #1. Evidence: [test_main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/test/test_mode_registry/test_main.cpp:238), [test_main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/test/test_mode_registry/test_main.cpp:355).

7. **[MINOR] The story record is no longer truthful against HEAD.**  
The story still claims minimal integration, 2-entry `MODE_TABLE`, and stub `ClassicCardMode` / `LiveFlightCardMode`, while the actual code now has a 4-mode table, full display-task integration, boot restore logic, and real rendering bodies. That is a git-vs-story documentation defect, not a firmware bug, but it makes the story unreliable as an audit artifact. Evidence: [story AC #3/#11 and Dev Agent Record](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-1-3-moderegistry-and-static-registration-system.md:19), [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:73), [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:501), [ClassicCardMode.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/modes/ClassicCardMode.cpp:44), [LiveFlightCardMode.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/modes/LiveFlightCardMode.cpp:166).

## Clean Passes

No material security vulnerability, hot-path performance regression, style failure, or type-safety problem stood out in the reviewed `ds-1.3` product files. The problems here are contract accuracy, cross-core correctness, and test honesty.

## Review Actions

- Issues Found: `7`
- Issues Fixed: `0`
- Action Items Created: `0`

Assumption: I did not re-run `pio run` / `pio test`; build-success claims in the story were not independently re-verified in this review.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-14</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">ds-1</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">2</var>
<var name="session_id">33801b06-b726-4895-af5c-5cf8a3b87fff</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="e8f5c877">embedded in prompt, file id: e8f5c877</var>
<var name="story_id">ds-1.3</var>
<var name="story_key">ds-1-3-moderegistry-and-static-registration-system</var>
<var name="story_num">3</var>
<var name="story_title">3-moderegistry-and-static-registration-system</var>
<var name="template">False</var>
<var name="timestamp">20260414_1605</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="validator_count"></var>
</variables>
<instructions><workflow>
  <critical>Communicate all responses in English and generate all documents in English</critical>
  <critical>You are the MASTER SYNTHESIS agent for CODE REVIEW findings.</critical>
  <critical>You have WRITE PERMISSION to modify SOURCE CODE files and story Dev Agent Record section.</critical>
  <critical>DO NOT modify story context (AC, Dev Notes content) - only Dev Agent Record (task checkboxes, completion notes, file list).</critical>
  <critical>All context (project_context.md, story file, anonymized reviews) is EMBEDDED below - do NOT attempt to read files.</critical>

  <step n="1" goal="Analyze reviewer findings">
    <action>Read all anonymized reviewer outputs (Reviewer A, B, C, D, etc.)</action>
    <action>For each issue raised:
      - Cross-reference with embedded project_context.md and story file
      - Cross-reference with source code snippets provided in reviews
      - Determine if issue is valid or false positive
      - Note reviewer consensus (if 3+ reviewers agree, high confidence issue)
    </action>
    <action>Issues with low reviewer agreement (1-2 reviewers) require extra scrutiny</action>
    <action>Group related findings that address the same underlying problem</action>
  </step>

  <step n="1.5" goal="Review Deep Verify code analysis" conditional="[Deep Verify Findings] section present">
    <critical>Deep Verify analyzed the actual source code files for this story.
      DV findings are based on static analysis patterns and may identify issues reviewers missed.</critical>

    <action>Review each DV finding:
      - CRITICAL findings: Security vulnerabilities, race conditions, resource leaks - must address
      - ERROR findings: Bugs, missing error handling, boundary issues - should address
      - WARNING findings: Code quality concerns - consider addressing
    </action>

    <action>Cross-reference DV findings with reviewer findings:
      - DV + Reviewers agree: High confidence issue, prioritize in fix order
      - Only DV flags: Verify in source code - DV has precise line numbers
      - Only reviewers flag: May be design/logic issues DV can't detect
    </action>

    <action>DV findings may include evidence with:
      - Code quotes (exact text from source)
      - Line numbers (precise location, when available)
      - Pattern IDs (known antipattern reference)
      Use this evidence when applying fixes.</action>

    <action>DV patterns reference:
      - CC-*: Concurrency issues (race conditions, deadlocks)
      - SEC-*: Security vulnerabilities
      - DB-*: Database/storage issues
      - DT-*: Data transformation issues
      - GEN-*: General code quality (null handling, resource cleanup)
    </action>
  </step>

  <step n="2" goal="Verify issues and identify false positives">
    <action>For each issue, verify against embedded code context:
      - Does the issue actually exist in the current code?
      - Is the suggested fix appropriate for the codebase patterns?
      - Would the fix introduce new issues or regressions?
    </action>
    <action>Document false positives with clear reasoning:
      - Why the reviewer was wrong
      - What evidence contradicts the finding
      - Reference specific code or project_context.md patterns
    </action>
  </step>

  <step n="3" goal="Prioritize by severity">
    <action>For verified issues, assign severity:
      - Critical: Security vulnerabilities, data corruption, crashes
      - High: Bugs that break functionality, performance issues
      - Medium: Code quality issues, missing error handling
      - Low: Style issues, minor improvements, documentation
    </action>
    <action>Order fixes by severity - Critical first, then High, Medium, Low</action>
    <action>For disputed issues (reviewers disagree), note for manual resolution</action>
  </step>

  <step n="4" goal="Apply fixes to source code">
    <critical>This is SOURCE CODE modification, not story file modification</critical>
    <critical>Use Edit tool for all code changes - preserve surrounding code</critical>
    <critical>After applying each fix group, run: pytest -q --tb=line --no-header</critical>
    <critical>NEVER proceed to next fix if tests are broken - either revert or adjust</critical>

    <action>For each verified issue (starting with Critical):
      1. Identify the source file(s) from reviewer findings
      2. Apply fix using Edit tool - change ONLY the identified issue
      3. Preserve code style, indentation, and surrounding context
      4. Log the change for synthesis report
    </action>

    <action>After each logical fix group (related changes):
      - Run: pytest -q --tb=line --no-header
      - If tests pass, continue to next fix
      - If tests fail:
        a. Analyze which fix caused the failure
        b. Either revert the problematic fix OR adjust implementation
        c. Run tests again to confirm green state
        d. Log partial fix failure in synthesis report
    </action>

    <action>Atomic commit guidance (for user reference):
      - Commit message format: fix(component): brief description (synthesis-ds-1.3)
      - Group fixes by severity and affected component
      - Never commit unrelated changes together
      - User may batch or split commits as preferred
    </action>
  </step>

  <step n="5" goal="Refactor if needed">
    <critical>Only refactor code directly related to applied fixes</critical>
    <critical>Maximum scope: files already modified in Step 4</critical>

    <action>Review applied fixes for duplication patterns:
      - Same fix applied 2+ times across files = candidate for refactor
      - Only if duplication is in files already modified
    </action>

    <action>If refactoring:
      - Extract common logic to shared function/module
      - Update all call sites in modified files
      - Run tests after refactoring: pytest -q --tb=line --no-header
      - Log refactoring in synthesis report
    </action>

    <action>Do NOT refactor:
      - Unrelated code that "could be improved"
      - Files not touched in Step 4
      - Patterns that work but are just "not ideal"
    </action>

    <action>If broader refactoring needed:
      - Note it in synthesis report as "Suggested future improvement"
      - Do not apply - leave for dedicated refactoring story
    </action>
  </step>

  <step n="6" goal="Generate synthesis report">
    <critical>When updating story file, use atomic write pattern (temp file + rename).</critical>
    <action>Update story file Dev Agent Record section ONLY:
      - Mark completed tasks with [x] if fixes address them
      - Append to "Completion Notes List" subsection summarizing changes applied
      - Update file list with all modified files
    </action>

    <critical>Your synthesis report MUST be wrapped in HTML comment markers for extraction:</critical>
    <action>Produce structured output in this exact format (including the markers):</action>
    <output-format>
&lt;!-- CODE_REVIEW_SYNTHESIS_START --&gt;
## Synthesis Summary
[Brief overview: X issues verified, Y false positives dismissed, Z fixes applied to source files]

## Validations Quality
[For each reviewer: ID (A, B, C...), score (1-10), brief assessment]
[Note: Reviewers are anonymized - do not attempt to identify providers]

## Issues Verified (by severity)

### Critical
[Issues that required immediate fixes - list with evidence and fixes applied]
[Format: "- **Issue**: Description | **Source**: Reviewer(s) | **File**: path | **Fix**: What was changed"]
[If none: "No critical issues identified."]

### High
[Bugs and significant problems - same format]

### Medium
[Code quality issues - same format]

### Low
[Minor improvements - same format, note any deferred items]

## Issues Dismissed
[False positives with reasoning for each dismissal]
[Format: "- **Claimed Issue**: Description | **Raised by**: Reviewer(s) | **Dismissal Reason**: Why this is incorrect"]
[If none: "No false positives identified."]

## Changes Applied
[Complete list of modifications made to source files]
[Format for each change:
  **File**: [path/to/file.py]
  **Change**: [Brief description]
  **Before**:
  ```
  [2-3 lines of original code]
  ```
  **After**:
  ```
  [2-3 lines of updated code]
  ```
]
[If no changes: "No source code changes required."]

## Deep Verify Integration
[If DV findings were present, document how they were handled]

### DV Findings Fixed
[List DV findings that resulted in code changes]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **File**: {path} | **Fix**: {What was changed}"]

### DV Findings Dismissed
[List DV findings determined to be false positives]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Reason**: {Why this is not an issue}"]

### DV-Reviewer Overlap
[Note findings flagged by both DV and reviewers - highest confidence fixes]
[If no DV findings: "Deep Verify did not produce findings for this story."]

## Files Modified
[Simple list of all files that were modified]
- path/to/file1.py
- path/to/file2.py
[If none: "No files modified."]

## Suggested Future Improvements
[Broader refactorings or improvements identified in Step 5 but not applied]
[Format: "- **Scope**: Description | **Rationale**: Why deferred | **Effort**: Estimated complexity"]
[If none: "No future improvements identified."]

## Test Results
[Final test run output summary]
- Tests passed: X
- Tests failed: 0 (required for completion)
&lt;!-- CODE_REVIEW_SYNTHESIS_END --&gt;
    </output-format>

  </step>

  <step n="6.5" goal="Write Senior Developer Review section to story file for dev_story rework detection">
    <critical>This section enables dev_story to detect that a code review has occurred and extract action items.</critical>
    <critical>APPEND this section to the story file - do NOT replace existing content.</critical>

    <action>Determine the evidence verdict from the [Evidence Score] section:
      - REJECT: Evidence score exceeds reject threshold
      - PASS: Evidence score is below accept threshold
      - UNCERTAIN: Evidence score is between thresholds
    </action>

    <action>Map evidence verdict to review outcome:
      - PASS → "Approved"
      - REJECT → "Changes Requested"
      - UNCERTAIN → "Approved with Reservations"
    </action>

    <action>Append to story file "## Senior Developer Review (AI)" section:
      ```
      ## Senior Developer Review (AI)

      ### Review: {current_date}
      - **Reviewer:** AI Code Review Synthesis
      - **Evidence Score:** {evidence_score} → {evidence_verdict}
      - **Issues Found:** {total_verified_issues}
      - **Issues Fixed:** {fixes_applied_count}
      - **Action Items Created:** {remaining_unfixed_count}
      ```
    </action>

    <critical>When evidence verdict is REJECT, you MUST create Review Follow-ups tasks.
      If "Action Items Created" count is &gt; 0, there MUST be exactly that many [ ] [AI-Review] tasks.
      Do NOT skip this step. Do NOT claim all issues are fixed if you reported deferred items above.</critical>

    <action>Find the "## Tasks / Subtasks" section in the story file</action>
    <action>Append a "#### Review Follow-ups (AI)" subsection with checkbox tasks:
      ```
      #### Review Follow-ups (AI)
      - [ ] [AI-Review] {severity}: {brief description of unfixed issue} ({file path})
      ```
      One line per unfixed/deferred issue, prefixed with [AI-Review] tag.
      Order by severity: Critical first, then High, Medium, Low.
    </action>

    <critical>ATDD DEFECT CHECK: Search test directories (tests/**) for test.fixme() calls in test files related to this story.
      If ANY test.fixme() calls remain in story-related test files, this is a DEFECT — the dev_story agent failed to activate ATDD RED-phase tests.
      Create an additional [AI-Review] task:
      - [ ] [AI-Review] HIGH: Activate ATDD tests — convert all test.fixme() to test() and ensure they pass ({test file paths})
      Do NOT dismiss test.fixme() as "intentional TDD methodology". After dev_story completes, ALL test.fixme() tests for the story MUST be converted to test().</critical>
  </step>

  </workflow></instructions>
<output-template></output-template>
</compiled-workflow>