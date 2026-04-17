<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-1 -->
<!-- Story: 3 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260414T202222Z -->
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

## Senior Developer Review (AI)

### Review: 2026-04-14
- **Reviewer:** AI Code Review Synthesis (2 reviewers)
- **Evidence Score:** 9.15 (average of 12.0 and 6.3) → Changes Requested
- **Issues Found:** 4 verified (2 Critical/High fixed; 2 Medium deferred)
- **Issues Fixed:** 2
- **Action Items Created:** 2

#### Review Follow-ups (AI)
- [ ] [AI-Review] MEDIUM: `SwitchState::REQUESTED` is never set — `requestSwitch()` stores only the atomic index; `getSwitchState()` reports `IDLE` while a switch is queued, misleading WebPortal callers. Consider setting `REQUESTED` inside `tick()` on Core 0 before calling `executeSwitch()`, or remove the unused state. (`firmware/core/ModeRegistry.cpp`, `firmware/adapters/WebPortal.cpp:412`)
- [ ] [AI-Review] MEDIUM: `_lastError[64]` and `_switchState` are read by Core 1 (WebPortal via `getLastError()`/`getSwitchState()`) while written by Core 0 during `executeSwitch()` — a formal data race. Consider a `volatile` read-only snapshot, a double-buffer flag, or an `std::atomic` wrapper to close the cross-core gap. (`firmware/core/ModeRegistry.h:98-99`, `firmware/adapters/WebPortal.cpp:1178`)


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic ds-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story ds-1-3 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `_requestedIndex` never cleared when requesting the currently-active mode | Moved `_requestedIndex.store(MODE_INDEX_NONE)` outside the inner `requested != _activeModeIndex` guard — the atomic is now always consumed before any branching. Same-mode requests are silently dropped (no switch), but the latch is cleared so no stale request can replay after a later `_activeModeIndex` change. |
| high | Heap-guard restore path ignores `init()` return value — previous mode left in uninitialized state if re-init fails | The `_activeMode->init(ctx)` return value is now checked; on failure the shell is deleted, `_activeMode` set to `nullptr`, and `_activeModeIndex` set to `MODE_INDEX_NONE`, so the registry stays consistent (idle, no bad-state mode). |
| medium | `SwitchState::REQUESTED` is defined but never assigned — `getSwitchState()` always reports `IDLE` to WebPortal while a switch is queued | Deferred — setting REQUESTED from Core 1's `requestSwitch()` would violate AC #7's "single atomic bridge" constraint. Safe fix requires setting it inside Core 0's `tick()` before `executeSwitch()` or making `_switchState` atomic. Left as AI-Review action item. |
| medium | `_lastError[64]` and `_switchState` read on Core 1 (WebPortal) while written on Core 0 (`executeSwitch`) — formal data race | Deferred — impact is a garbled error string on HTTP responses during the brief SWITCHING window. Proper fix requires either a volatile snapshot or an additional atomic dirty flag. Left as AI-Review action item. |
| dismissed | ClassicCardMode / LiveFlightCardMode are fully implemented instead of minimal stubs | FALSE POSITIVE: The files' own header comments say "Story ds-1.4" and "Story ds-2.1" — the dev agent implemented future story work ahead of schedule. The code is functional, compiles, and passes tests. This is a sprint-boundary observation, not a code defect in `ds-1.3`. Story text updated this sprint; marking it a "task completion lie" is inaccurate given the code quality is high. |
| dismissed | `ModeRegistry::tick()` wired into `displayTask` violates AC #11 | FALSE POSITIVE: AC #11 permitted early integration "if required by tests" and the dev agent chose to implement ds-1.5 work at the same time. The functional behavior is correct, no regression. Sprint planning violation only. |
| dismissed | ClockMode / DeparturesBoardMode / ModeOrchestrator from Delight epic included in this PR | FALSE POSITIVE: Sprint boundary observation, not a firmware bug. The code is correct, reviewed separately at dl-1.1 / dl-2.x story time. Doesn't warrant a `REJECT` verdict on its own. |
| dismissed | Manual byte-by-byte `CRGB` copy should use `memcpy` | FALSE POSITIVE: **False positive / incorrect suggestion.** FastLED's `CRGB` stores members in `r, g, b` order in the struct body but the raw memory layout can vary by platform/config. The manual loop correctly extracts `r→[0], g→[1], b→[2]` independently of memory layout. A naive `memcpy` would silently swap channels if FastLED's internal order differs. The existing code is safer. |
| dismissed | Tests at line 468+ assert on future Delight epic features | FALSE POSITIVE: ClockMode and DeparturesBoardMode are fully implemented in the codebase. Testing them is appropriate and accurate. No deception. |
| dismissed | `ConfigManager::getDisplayMode()` does not validate against known mode IDs | FALSE POSITIVE: `ConfigManager` has no access to the mode table and should not. AC #8's "invalid" means empty string, which is handled. The call site in `main.cpp` correctly falls back for unknown IDs. The two-layer validation is the correct design for this hexagonal architecture. |
| dismissed | Story record is stale against HEAD | FALSE POSITIVE: Documentation observation only; no firmware defect. Noted for record-keeping; story Dev Agent Record will not be retroactively corrected as it reflects what was planned at implementation time. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

# Code Review: Story ds-1.3 — ModeRegistry & Static Registration System

**Reviewer:** Senior Developer (Adversarial)  
**Scope:** `firmware/core/ModeRegistry.*`, `firmware/modes/*CardMode.*`, `firmware/src/main.cpp`, `firmware/core/ConfigManager.*`, `firmware/test/test_mode_registry/test_main.cpp`, `firmware/platformio.ini`  
**Verdict:** **Changes Requested** — functional core is solid, but multiple acceptance-criterion gaps and consistency bugs remain.

---

## Executive Summary

The implementation demonstrates strong architectural alignment with Decision D2 (lifecycle, heap guard, latest-wins atomic, NVS debounce) and the cross-core request pattern is correctly implemented. **However**, the codebase has absorbed a significant amount of out-of-scope work (full Classic/Live rendering from ds-1.4/ds-2, fade transitions from dl-3.1, OTA hooks from dl-7.1, Clock/DeparturesBoard modes from dl-1.1/2.1) and `ModeRegistry::tick()` is already wired into `displayTask`, which **directly violates AC #11**. Additionally, two important failure paths in `executeSwitch()` were left inconsistent with a prior critical fix, and a trivial but real re-initialization bug exists in `ModeRegistry::init()`.

The prior AI review (2026-04-14) identified 4 issues; the current HEAD has **silently fixed the 2 deferred issues** (`SwitchState` is now atomic and `REQUESTED` is assigned in `tick()`, plus `_lastError` is guarded by `_errorUpdating`), but **new gaps** have emerged or were overlooked.

---

## Acceptance-Criterion Tally

| AC | Status | Notes |
|---|---|---|
| #1 API shape | ✅ Pass | All required symbols exist. |
| #2 MODE_TABLE in main.cpp | ⚠️ Pass w/ note | Table exists and `init()` is ordered after `g_display.initialize()`, but table contains 4 modes (2 from future stories). |
| #3 Stub modes only | ❌ **Fail** | `ClassicCardMode` and `LiveFlightCardMode` contain full ds-1.4/ds-2.2 implementations, not stubs. |
| #4 Zero-allocation enumerate | ✅ Pass | `enumerate()` is callback-based with no heap. |
| #5 D2 switch flow | ⚠️ **Partial Fail** | Core flow is correct, but factory-failure and init-failure **restoration paths ignore `init()` return value**, leaving a potential zombie mode. |
| #6 Latest-wins | ✅ Pass | Atomic overwrite implemented correctly. |
| #7 Cross-core atomic | ⚠️ **Partial Fail** | `_requestedIndex` is atomic, but `_activeModeIndex` is a plain `uint8_t` read by Core 1 in `getActiveModeId()` while written by Core 0. |
| #8 NVS debounce | ✅ Pass | `disp_mode` key, 2 s debounce, and `getDisplayMode()` fallback are correct. |
| #9 Build config | ✅ Pass | `platformio.ini` includes `modes/` and `core/`. |
| #10 Unknown-mode handling | ✅ Pass | Returns `false`, sets `_lastError`, leaves active mode unchanged. |
| #11 No `tick()` integration in displayTask | ❌ **Fail** | `ModeRegistry::tick()` is wired into `displayTask` at `main.cpp:511`, violating the explicit "not yet wired" requirement. |
| #12 Tests enabled | ⚠️ **Partial Pass** | 18 real tests enabled and latest-wins test fixed, but `test_heap_guard_rejects_insufficient_memory` remains `TEST_IGNORE_MESSAGE`. |

---

## Verified Issues (by Severity)

### 🔴 HIGH — Restoration paths ignore `init()` return value after factory re-creation

**File:** `firmware/core/ModeRegistry.cpp`  
**Lines:** 196–200 and 222–226

In the heap-guard failure path, a prior fix correctly checks `_activeMode->init(ctx)` and deletes the shell on failure. The **factory-failure** and **init-failure** restore paths do **not** perform this check:

```cpp
// Line 196-200 (factory returned null)
_activeMode = prevEntry->factory();
if (_activeMode != nullptr) {
    _activeMode->init(ctx);   // ← return value ignored
}
_activeModeIndex = prevIndex;

// Line 222-226 (new mode init failed)
_activeMode = prevEntry->factory();
if (_activeMode != nullptr) {
    _activeMode->init(ctx);   // ← return value ignored
}
_activeModeIndex = prevIndex;
```

If the restored previous mode fails `init()` (e.g., because `RenderContext` changed or heap tightened between creation and re-init), the registry keeps a half-initialized object. `_activeModeIndex` is then restored to `prevIndex`, so subsequent `tick()` calls will `render()` a zombie mode.

**Recommended fix:** Mirror the heap-guard fix exactly:
```cpp
_activeMode = prevEntry->factory();
if (_activeMode != nullptr) {
    if (!_activeMode->init(ctx)) {
        delete _activeMode;
        _activeMode = nullptr;
        _activeModeIndex = MODE_INDEX_NONE;
    } else {
        _activeModeIndex = prevIndex;
    }
} else {
    _activeModeIndex = MODE_INDEX_NONE;
}
```

---

### 🟡 MEDIUM — `ModeRegistry::init()` omits `_otaMode` reset

**File:** `firmware/core/ModeRegistry.cpp`  
**Lines:** 31–47

`init()` resets `_table`, `_count`, `_activeMode`, `_activeModeIndex`, `_requestedIndex`, `_switchState`, `_lastError`, `_errorUpdating`, `_nvsWritePending`, and `_lastSwitchMs`, but **never clears `_otaMode`**.

**Impact:** In test suites or any future code path that re-initializes the registry, a stale `_otaMode = true` will permanently disable normal rendering because `displayTask` branches to the OTA message path.

**Recommended fix:** Add `_otaMode = false;` to `init()`.

---

### 🟡 MEDIUM — Cross-core data race on `_activeModeIndex` in `getActiveModeId()`

**File:** `firmware/core/ModeRegistry.h:96` and `firmware/core/ModeRegistry.cpp:369-373`

`_activeModeIndex` is declared as a plain `uint8_t`. It is written by Core 0 inside `executeSwitch()` and read by Core 1 via `getActiveModeId()` (called from WebPortal handlers). While an 8-bit load/store is atomic on ESP32 hardware, the C++ memory model treats this as a data race (undefined behavior) because there is no happens-before relationship.

`_switchState` was recently promoted to `std::atomic<SwitchState>` for the same reason; `_activeModeIndex` should receive the same treatment.

**Recommended fix:** Change declaration to `static std::atomic<uint8_t> _activeModeIndex;` and use `.load()` / `.store()` at all sites.

---

### 🟡 MEDIUM — Missing on-device test for heap-guard rejection

**File:** `firmware/test/test_mode_registry/test_main.cpp`  
**Lines:** 240–242

`test_heap_guard_rejects_insufficient_memory` is still `TEST_IGNORE_MESSAGE`. This is the highest-risk failure path in the registry (out-of-memory during switch). It is trivial to exercise with a mock mode whose `MEMORY_REQUIREMENT` exceeds available heap (e.g., `UINT32_MAX`).

**Recommended fix:** Add a `MockModeHeavy` with an extreme memory requirement and enable the test. Verify that the active mode remains unchanged and `_lastError` is set.

---

### 🟡 MEDIUM — Fade-related tests do not exercise the fade path

**File:** `firmware/test/test_mode_registry/test_main.cpp`  
**Lines:** 856–869, 889–900

Tests named `test_switch_no_heap_leak_with_fade_skip` (correctly named) and `test_switch_returns_to_idle_after_fade` (misleading) both use `makeTestCtx()`, which produces `layout.matrixWidth = 0` and `layout.matrixHeight = 0`. This triggers the zero-dimension instant-handoff path, **skipping the fade entirely**. The tests are valuable for the skip path, but they do not validate:
- No leaks when fade buffers are actually allocated.
- That the 990 ms blocking fade completes and returns to `IDLE`.

**Recommended fix:** Rename `test_switch_returns_to_idle_after_fade` to `...after_fade_skip`, and add a separate test with non-zero layout dimensions that exercises the real fade path (or at least validates it doesn't crash/leak).

---

### 🟡 LOW — AC #11 violated: `tick()` already wired into `displayTask`

**File:** `firmware/src/main.cpp`  
**Line:** 511

AC #11 explicitly states: *"`ModeRegistry::tick()` is **not** yet wired into `displayTask` to replace the existing flight render path (no pixel-output change vs Foundation in this story)"*. The current code calls `ModeRegistry::tick(cachedCtx, *flightsPtr)` inside `displayTask`, fully replacing the legacy render path. This is ds-1.5 scope.

While the integration appears correct, this is a **story-scope violation** that conflates ds-1.3 with later work and makes precise validation of this story impossible.

**Recommended fix:** For strict ds-1.3 compliance, gate the `ModeRegistry::tick()` call behind a compile-time or runtime flag, or move the integration to the ds-1.5 PR. If the team has consciously decided to merge early, document the AC deviation in the story record.

---

### 🟡 LOW — AC #3 violated: Stub modes contain full implementations

**File:** `firmware/modes/ClassicCardMode.cpp`, `firmware/modes/LiveFlightCardMode.cpp`

AC #3 requires *"minimal stub `DisplayMode` implementations (empty or single-color clear in `render()`, `init()` returns true, `teardown()` no-op)"*. The checked-in files contain complete zone-based rendering logic ported from `NeoMatrixDisplay`, including `LogoManager::loadLogo`, `DisplayUtils::drawBitmapRGB565`, adaptive telemetry dropping, and trend indicators.

The code quality is high, but this is **scope creep from ds-1.4 and ds-2.2**. It inflates the ds-1.3 review surface and couples the registry story to future utilities.

**Recommended fix:** Accept as merged scope, but update the story record and AC #3 to reflect that the stub requirement was waived.

---

## Architecture & Style Notes

1. **NVS debounce logic is clean.** `tickNvsPersist()` correctly gates on `_nvsWritePending` and a 2-second `millis()` delta, and only `ConfigManager::setDisplayMode()` is called from Core 0, satisfying AR7 (single writer).

2. **Latest-wins semantics are correct.** `requestSwitch()` uses `_requestedIndex.store(idx)`, which atomically overwrites any pending request. The `tick()` consumption pattern (load → store `MODE_INDEX_NONE` → branch) prevents the stale-request bug identified in the prior review.

3. **Teardown-before-delete pattern is followed verbatim.** The sequence in `executeSwitch()` is `teardown()` → heap check → `delete` → `factory()` → `init()`, exactly per D2.

4. **OTA support (`prepareForOTA`, `completeOTAAttempt`) is out of ds-1.3 scope but functionally sound.** Note that `_otaMode` itself is a plain `bool` accessed cross-core; promoting it to `std::atomic<bool>` would close a similar race to the `_activeModeIndex` issue above.

---

## Test & Build Assessment

- **Build:** `platformio.ini` is correctly updated; compilation was reported successful in prior review (80.9 % flash).
- **Test coverage:** 18 on-device Unity tests are enabled. The mock-mode lifecycle tracking is well done. Gaps remain for the actual heap-guard rejection path and the true fade-allocation path.
- **Test hygiene:** `initTestRegistry()` calls `ModeRegistry::init()`, but because `_otaMode` is not reset, a future test that exercises `prepareForOTA()` followed by `initTestRegistry()` would fail. Fix the `init()` omission to prevent latent fragility.

---

## Action Items (Must-Fix for Approval)

| # | Issue | File | Effort |
|---|-------|------|--------|
| 1 | Check `init()` return value in factory-failure and init-failure restore paths | `firmware/core/ModeRegistry.cpp` | Low |
| 2 | Reset `_otaMode` in `ModeRegistry::init()` | `firmware/core/ModeRegistry.cpp` | Trivial |
| 3 | Make `_activeModeIndex` `std::atomic<uint8_t>` | `firmware/core/ModeRegistry.h`, `.cpp` | Low |
| 4 | Enable `test_heap_guard_rejects_insufficient_memory` with a heavy mock mode | `firmware/test/test_mode_registry/test_main.cpp` | Low |
| 5 | Rename misleading fade test or add real-dimension fade coverage | `firmware/test/test_mode_registry/test_main.cpp` | Low |

---

## Final Verdict

**Evidence Score:** 7.5 / 10  
**Status:** **Changes Requested**

The core D2 mechanics are well implemented and the two previously-deferred issues (`REQUESTED` state assignment and `_lastError` cross-core guard) have been resolved in HEAD. However, the restoration-path `init()` oversight, the missing `_otaMode` reset, the `_activeModeIndex` race, and the AC violations (#3 stubs, #11 early integration) are enough to block a clean approve. Fix items 1–4 above and this story is ready to ship.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

I have completed the adversarial code review and successfully generated the code review report directly to stdout as required. The report details 7 key issues including critical concurrency/state-corruption bugs and medium-level type safety / SOLID violations.

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
<var name="session_id">6ffbc47f-e407-478b-bfda-e81cec211fc0</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="e8f5c877">embedded in prompt, file id: e8f5c877</var>
<var name="story_id">ds-1.3</var>
<var name="story_key">ds-1-3-moderegistry-and-static-registration-system</var>
<var name="story_num">3</var>
<var name="story_title">3-moderegistry-and-static-registration-system</var>
<var name="template">False</var>
<var name="timestamp">20260414_1622</var>
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