<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-1 -->
<!-- Story: 4 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260414T210117Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story ds-1.4

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
<file id="6002f7fb" path="_bmad-output/implementation-artifacts/stories/ds-1-4-classic-card-mode-implementation.md" label="DOCUMENTATION"><![CDATA[

# Story ds-1.4: Classic Card Mode Implementation

Status: ready-for-review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user,
I want the existing three-line flight card display migrated into a ClassicCardMode that renders through the new mode system,
So that the familiar display continues working identically while enabling the mode architecture.

## Acceptance Criteria

1. **Given** `DisplayUtils`, `RenderContext`, and the ds-1.3 stub `ClassicCardMode` exist, **When** implementation is complete, **Then** `ClassicCardMode::render()` performs the same **zone-based** classic layout as `NeoMatrixDisplay::renderZoneFlight()` — logo zone (`LogoManager::loadLogo` + `DisplayUtils::drawBitmapRGB565`), flight zone (airline / route / aircraft lines per `renderFlightZone` branching), telemetry zone (`renderTelemetryZone`) — using **`ctx.layout`** (`logoZone`, `flightZone`, `telemetryZone`) and **`ctx.matrix`** instead of `NeoMatrixDisplay` private `_layout` / `_matrix`

2. **Given** NFR C1 (pixel-identical classic output), **When** `ctx.layout.valid == true` and the same `FlightInfo` + context are supplied, **Then** framebuffer content matches the Foundation path **before** `FastLED.show()` — verify across **at least 5** representative flights (short/long airline names, missing telemetry, single-line vs three-line flight zone heights) using a **test harness** (e.g. Unity test that renders to matrix and samples key pixels, or documented side-by-side capture procedure) and record the method in the Dev Agent Record

3. **Given** FR12 (flight cycling), **When** `flights.size() > 1`, **Then** ClassicCardMode advances the displayed index every **`ctx.displayCycleMs`** milliseconds (same cadence as `displayFlights`: `ConfigManager::getTiming().display_cycle * 1000` is already folded into `RenderContext` by the caller — **do not read `ConfigManager` inside the mode** for timing or colors)

4. **Given** FR4 (zone bounds), **When** rendering, **Then** no drawing calls write outside each active zone’s `Rect` (x, y, w, h) — same effective clipping as current helpers

5. **Given** empty `flights`, **When** `render()` runs, **Then** the idle UI matches `NeoMatrixDisplay::displayLoadingScreen()` (white border, centered `...`, text color from user theme) **without** calling `FastLED.show()` — frame commit remains the pipeline’s responsibility (FR35 prep; enforced in ds-1.5)

6. **Given** `ctx.layout.valid == false`, **When** `render()` runs with non-empty flights, **Then** behavior matches the fallback path **`NeoMatrixDisplay::displaySingleFlightCard`** (bordered inset card, three lines with `>` route separator) using `ctx.matrix` and dimensions from `ctx.layout.matrixWidth` / `matrixHeight` when set, otherwise from the matrix object’s pixel width/height (Adafruit GFX) — **not** unbounded text past the physical matrix

7. **Given** heap accounting (Decision D2), **Then** `ClassicCardMode::MEMORY_REQUIREMENT` (static `constexpr` in the class, consumed via `ModeEntry`) is **updated** to cover any **persistent** allocations the mode makes after `init`/`teardown` cycles (if none, document as instance + vtable + member fields only); **do not** add `getMemoryRequirement()` as a virtual on `DisplayMode`

8. **Given** NFR S3, **Then** stack allocations in `render()` (including temporaries from `String`) stay **under 512 bytes** — if compiler/stack usage is borderline, refactor hot paths to smaller buffers or `char[]` **without** changing pixels

9. **Given** NFR S5, **Then** `ctx.brightness` is **read-only** usage (if referenced for future logic); **do not** call `matrix->setBrightness` or otherwise change brightness inside the mode

10. **Given** metadata for Mode Picker, **Then** `getName()` returns **`"Classic Card"`** (stub already does) and `getZoneDescriptor()` / `_zones` **reflect** logo + flight + telemetry regions consistently with the real `LayoutEngine` zones (adjust relative schematic percentages if needed so labels align with FR4 regions)

11. **Given** regression safety for users still on the **legacy** `NeoMatrixDisplay::displayFlights` path until ds-1.5, **Then** this story **does not** change the production display task to call `ClassicCardMode` yet — **leave** `NeoMatrixDisplay::displayFlights`, `renderFlight`, and zone helpers **behavior-identical** for the same inputs (optional micro-refactor only if required for shared static helpers extracted to an anonymous namespace / file — prefer **copy-then-verify** in ClassicCardMode first to avoid drift)

12. **Given** build health, **Then** `pio run` from `firmware/` succeeds with no new warnings

## Tasks / Subtasks

- [x] Task 1: Port zone rendering into ClassicCardMode (AC: #1, #4, #6, #9)
  - [x] 1.1: Implement private helpers mirroring `renderLogoZone`, `renderFlightZone`, `renderTelemetryZone` signatures using `(FastLED_NeoMatrix* matrix, const RenderContext& ctx, const FlightInfo& f, const Rect& zone)` or equivalent — use `ctx.textColor`, `ctx.logoBuffer`, **never** `ConfigManager::getDisplay()`
  - [x] 1.2: Use `DisplayUtils::*` for text and bitmap drawing; pass `ctx.matrix` as first argument where required
  - [x] 1.3: Handle `!ctx.layout.valid` branch by porting `displaySingleFlightCard` logic with matrix width/height from `ctx.layout.matrixWidth` / `matrixHeight` (see `LayoutResult`)

- [x] Task 2: Cycling + idle (AC: #3, #5)
  - [x] 2.1: Add member fields `_currentFlightIndex`, `_lastCycleMs` (or equivalent) — reset in `init()`, clear in `teardown()` if needed
  - [x] 2.2: Port cycling logic from `displayFlights` lines ~435–449 using `millis()` and `ctx.displayCycleMs`
  - [x] 2.3: Port empty-vector behavior from `displayLoadingScreen` (no `show`)

- [x] Task 3: Memory + metadata (AC: #7, #8, #10)
  - [x] 3.1: Tune `MEMORY_REQUIREMENT` comment + value after final struct size / any heap use
  - [x] 3.2: Review `String` usage in `render`; shrink or split if stack risk
  - [x] 3.3: Align `_zones` / `_descriptor` with real zones (Logo / Flight / Telemetry labeling)

- [x] Task 4: Verification (AC: #2, #11, #12)
  - [x] 4.1: Add or extend `firmware/test/` coverage for parity (recommended: dedicated test target or reuse patterns from `test_layout_engine`)
  - [x] 4.2: Document manual visual checklist if automated pixel test is impractical on CI
  - [x] 4.3: `pio run` clean build

#### Review Follow-ups (AI)
- [x] [AI-Review] HIGH: All render-path tests use `ctx.matrix=nullptr` — render(), renderLogoZone, renderFlightZone, renderTelemetryZone, renderSingleFlightCard, renderLoadingScreen are never exercised in CI. Add at least one test with a non-null matrix (real or spy) to verify actual draw calls. (`firmware/test/test_classic_card_mode/test_main.cpp`) — **RESOLVED**: Added 4 real-matrix tests using CRGB buffer + FastLED_NeoMatrix (no hardware needed): `test_render_valid_layout_draws_pixels`, `test_render_loading_screen_draws_border`, `test_render_fallback_card_draws_border`, `test_render_with_matrix_no_heap_leak`.
- [x] [AI-Review] HIGH: Flight cycling logic (_currentFlightIndex advance, _lastCycleMs anchor) is unreachable in all tests because render() returns early on nullptr matrix before executing cycling state updates. Add cycling coverage via non-null matrix or expose read-only cycling state under a TEST_ONLY accessor. (`firmware/modes/ClassicCardMode.cpp:54-65`, `firmware/test/test_classic_card_mode/test_main.cpp`) — **RESOLVED**: Moved cycling state update before matrix null guard in render(). Added `#ifdef PIO_UNIT_TESTING` accessors (getCurrentFlightIndex, getLastCycleMs). Added 6 cycling tests: starts_at_zero, stays_zero_for_single, advances_after_display_cycle, wraps_around, does_not_advance_before_interval, teardown_resets.
- [x] [AI-Review] MEDIUM: renderFlightZone and renderTelemetryZone construct 6–10 Arduino String objects per call at ~20fps, causing per-frame heap fragmentation. Refactor hot paths to fixed `char[]` + `snprintf` without pixel change. (`firmware/modes/ClassicCardMode.cpp:109-207`) — **RESOLVED**: Refactored all render methods to use `char[]` buffers + `snprintf`. Added 3 new char*-based overloads to DisplayUtils (drawTextLine, truncateToColumns, formatTelemetryValue). Zero heap allocation on hot path.
- [x] [AI-Review] MEDIUM: renderZoneFlight passes ctx.layout zones directly to draw helpers with no clipping against matrix dimensions. Add defensive bounds assertions or clamp logic before draw calls to guard against malformed RenderContext. (`firmware/modes/ClassicCardMode.cpp:94-98`) — **RESOLVED**: Added static `clampZone()` helper that constrains zone rects to matrix dimensions. Zones with zero w/h after clamping are skipped. Added 2 tests: `test_render_zone_clamping_oversized_zones`, `test_render_zone_completely_outside_matrix`.

## Dev Notes

### Source of truth for “Foundation” pixels

Port from **`firmware/adapters/NeoMatrixDisplay.cpp`**:

```417:422:firmware/adapters/NeoMatrixDisplay.cpp
void NeoMatrixDisplay::renderZoneFlight(const FlightInfo &f)
{
    renderLogoZone(f, _layout.logoZone);
    renderFlightZone(f, _layout.flightZone);
    renderTelemetryZone(f, _layout.telemetryZone);
}
```

- **`renderLogoZone`** — `NeoMatrixDisplay.cpp` ~292–297  
- **`renderFlightZone`** — ~300–365  
- **`renderTelemetryZone`** — ~368–414  
- **Cycling + valid/invalid layout** — `displayFlights` ~426–457  
- **Fallback card** — `displaySingleFlightCard` ~243–287  
- **Idle** — `displayLoadingScreen` ~467–491  

`renderFlight` (~712–732) duplicates zone vs fallback vs loading without cycling — use **`displayFlights`** as the behavioral reference for multi-flight cadence.

### RenderContext usage (strict)

| Field | Use |
|--------|-----|
| `matrix` | Draw target; **no** `FastLED.show()` |
| `layout` | `logoZone`, `flightZone`, `telemetryZone`, `valid`, `matrixWidth`, `matrixHeight` |
| `textColor` | RGB565 text (replaces per-call `ConfigManager` color reads) |
| `brightness` | Read-only per NFR S5 |
| `logoBuffer` | Same role as `NeoMatrixDisplay::_logoBuffer` — pass pointer into `LogoManager` / bitmap draw |
| `displayCycleMs` | Flight rotation interval |

### Epic ↔ code naming

- Epic mentions `getMemoryRequirement()` and `getZoneLayout()` — the codebase uses **`static constexpr MEMORY_REQUIREMENT`** + **`getZoneDescriptor()`** [Source: `interfaces/DisplayMode.h`, ds-1.3 story Dev Notes]

### Dependencies

- **ds-1.2** — `utils/DisplayUtils.h` / `.cpp`  
- **ds-1.3** — `ClassicCardMode` file names and mode id `classic_card` unchanged  
- **ds-1.5** — wires `ModeRegistry::tick` + `show()` into `displayTask`; this story only ensures Classic mode is **ready**

### Out of scope

- Removing `NeoMatrixDisplay` zone methods or `_currentFlightIndex` — **ds-1.5** unless required for deduplication with zero behavior change  
- WebPortal mode APIs — **ds-3**  
- `LiveFlightCardMode` real implementation — **ds-2**

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-1.md` (Story ds-1.4)  
- Architecture: `_bmad-output/planning-artifacts/architecture.md` (D1 DisplayMode, D3 NeoMatrixDisplay split)  
- Prior: `_bmad-output/implementation-artifacts/stories/ds-1-3-moderegistry-and-static-registration-system.md`  
- Layout: `firmware/core/LayoutEngine.h` (`LayoutResult`, `Rect`)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` clean build: SUCCESS — no new warnings (78.2% flash, 16.7% RAM) [initial]
- `pio run` post-review-followup: SUCCESS — 80.8% flash, 17.1% RAM (added DisplayUtils overloads)
- `pio test --filter test_classic_card_mode` compile: SUCCESS — 27-test binary built cleanly
- `pio test --without-uploading --without-testing` full regression: SUCCESS — all test suites compile clean
- On-device test execution: requires ESP32 hardware (Unity on-device framework)

### Completion Notes List

- **Task 1**: Ported all three zone rendering methods (renderLogoZone, renderFlightZone, renderTelemetryZone) from NeoMatrixDisplay into ClassicCardMode as private helpers. All use RenderContext fields (ctx.matrix, ctx.textColor, ctx.logoBuffer, ctx.layout zones) — zero ConfigManager reads. Added renderSingleFlightCard fallback for invalid layout and renderLoadingScreen for empty flights. Rendering logic is line-for-line identical to NeoMatrixDisplay foundation.
- **Task 2**: Added `_currentFlightIndex` and `_lastCycleMs` member fields. Cycling uses `ctx.displayCycleMs` (not ConfigManager). Both fields reset in init() and teardown(). Empty flights path renders white-bordered "..." loading screen without FastLED.show().
- **Task 3**: MEMORY_REQUIREMENT kept at 64 bytes (sizeof instance ~12 bytes actual: vtable ptr + size_t + unsigned long). Zone descriptors updated from 4-zone stub (Logo/Airline/Route/Aircraft) to 3-zone real layout (Logo 25%/Flight 75%x60%/Telemetry 75%x40%) matching LayoutEngine output. String usage reviewed: all temporaries are local to render() scope, longest ~40 chars, well under 512 byte stack budget. ctx.brightness is never written.
- **Task 4**: Created `firmware/test/test_classic_card_mode/test_main.cpp` with 16 Unity tests covering: init/teardown lifecycle, metadata correctness (getName, getZoneDescriptor 3-zone alignment, getSettingsSchema null), MEMORY_REQUIREMENT sizing, null matrix guards, heap safety (10 init/teardown cycles), factory integration, render lifecycle with multiple flights, and single-flight cycling behavior.
- **Review Follow-up #1 (render coverage)**: Added 4 real-matrix tests using CRGB[2048] buffer + FastLED_NeoMatrix(64x32). Tests verify non-zero pixel output for zone layout, loading screen border, fallback card border, and heap stability across 20 render frames.
- **Review Follow-up #2 (cycling coverage)**: Moved cycling state update before matrix null guard. Added PIO_UNIT_TESTING accessors. Added 6 cycling tests covering start, single-flight, advance, wraparound, pre-interval hold, and teardown reset.
- **Review Follow-up #3 (String heap churn)**: Refactored renderFlightZone, renderTelemetryZone, renderSingleFlightCard to char[] + snprintf. Added 3 char*-based overloads to DisplayUtils (drawTextLine, truncateToColumns, formatTelemetryValue). Zero heap allocation on 20fps hot path.
- **Review Follow-up #4 (zone bounds)**: Added static clampZone() helper that constrains zone Rects to matrix dimensions. Added 2 tests for oversized and fully-outside zones.

### Visual Verification Checklist (AC #2)

Pixel-identical verification requires on-device side-by-side comparison. Procedure:
1. Flash firmware with both legacy `displayFlights` path and ClassicCardMode active
2. Test with 5+ representative flights:
   - Short airline name (e.g., "UA") — compact text
   - Long airline name (e.g., "American Airlines") — truncation with "..."
   - Missing telemetry (NAN values) — "--" placeholders
   - Single-line flight zone height — compact route-only display
   - Three-line expanded flight zone — all three fields visible
3. Compare framebuffer output visually or via serial pixel dump
4. Verify no drawing outside zone bounds (check border pixels remain black)

### File List

- `firmware/modes/ClassicCardMode.h` (modified — added cycling fields, private helpers, PIO_UNIT_TESTING accessors)
- `firmware/modes/ClassicCardMode.cpp` (modified — full rendering implementation, cycling before null guard, char[] refactor, clampZone)
- `firmware/utils/DisplayUtils.h` (modified — added 3 char*-based overload declarations)
- `firmware/utils/DisplayUtils.cpp` (modified — implemented 3 char*-based overloads)
- `firmware/test/test_classic_card_mode/test_main.cpp` (rewritten — 27 Unity tests including real-matrix, cycling, zone clamping)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (modified — status update)

## Change Log

- **2026-04-13**: Implemented ClassicCardMode with full zone rendering (logo, flight, telemetry), cycling logic, fallback card, and idle screen. Added 16 Unity tests. Build clean. Story moved to review.
- **2026-04-14**: Resolved all 4 AI-Review follow-ups: (1) Added 4 real-matrix tests with CRGB buffer; (2) Fixed cycling testability — moved state update before null guard, added 6 cycling tests with PIO_UNIT_TESTING accessors; (3) Eliminated String heap churn — refactored to char[]/snprintf, added DisplayUtils char* overloads; (4) Added zone bounds clamping with clampZone() + 2 tests. Total: 27 tests, all compile clean. Full regression suite (all test targets) compiles clean. Build: 80.8% flash, 17.1% RAM.

## Previous story intelligence

- **ds-1.3** shipped stub `ClassicCardMode` with `fillScreen(0)` — replace body; keep **mode id** and **file paths**.  
- Registry uses **`MEMORY_REQUIREMENT`** static, not a virtual.  
- **Modes must not call `FastLED.show()`** — already violated inside some `NeoMatrixDisplay` helpers today; ClassicCardMode must not add new `show()` calls.

## Git intelligence summary

Recent epic-ds-1 work added `modes/`, `ModeRegistry`, `DisplayUtils` — Classic migration should be a focused edit to `ClassicCardMode.cpp` plus optional tests.

## Latest tech information

- **Arduino `String`** on ESP32: short-lived locals in `render()` are common but count toward **stack** and fragmentation risk; keep lifetime bounded to `render()`.

## Project context reference

`_bmad-output/project-context.md` — build with `pio run` from `firmware/`, hexagonal layout, display on Core 0.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-14
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 8.0 → REJECT
- **Issues Found:** 9 verified (1 critical, 2 high-deferred, 2 medium, 2 low-fixed, 2 dismissed)
- **Issues Fixed:** 4 (critical logoBuffer guard, medium innerWidth guard, low test rename, low heap tolerance)
- **Action Items Created:** 4 deferred (render coverage, cycling coverage, String heap churn, zone bounds) — **ALL 4 RESOLVED** in review follow-up pass (2026-04-14)


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

## Story ds-1-3 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `executeSwitch()` factory-failure and init-failure restore paths called `_activeMode->init(ctx)` and silently discarded the return value. If the previous mode's re-init failed, a half-initialised zombie mode would become the active mode. | Both restore paths now check `!_activeMode->init(ctx)`; on re-init failure the shell is deleted, `_activeMode = nullptr`, `_activeModeIndex = MODE_INDEX_NONE` — matching the pattern already applied in the heap-guard path by the prior review. |
| medium | `ModeRegistry::init()` did not reset `_otaMode` to `false`. A test calling `prepareForOTA()` followed by `ModeRegistry::init()` would leave `_otaMode = true`, permanently disabling the normal render path for subsequent tests. | Added `_otaMode = false;` in `init()` after `_lastSwitchMs = 0;`. |
| medium | `_activeModeIndex` was declared as a plain `uint8_t`. It is written by Core 0 (`executeSwitch`, `tick`) and by Core 1 (`prepareForOTA`), and read by Core 1 (`getActiveModeId`). The C++ memory model classifies this as a data race (undefined behaviour) even though an 8-bit load/store is hardware-atomic on the ESP32 Xtensa LX6. `_switchState` was already promoted to `std::atomic` for the same reason. | Changed declaration to `static std::atomic<uint8_t> _activeModeIndex;` and the static-member definition to `std::atomic<uint8_t> ModeRegistry::_activeModeIndex(MODE_INDEX_NONE);`. All existing `= X` assignments invoke `atomic::operator=(T)` and all reads invoke `atomic::operator T()` at `memory_order_seq_cst`, so no additional call-site changes were required. |
| medium | `test_heap_guard_rejects_insufficient_memory` remained `TEST_IGNORE_MESSAGE`. The heap-guard rejection is the highest-risk path in the registry and was completely untested. | Added `MockModeHeavy` (MEMORY_REQUIREMENT = 200 KB, exceeds ESP32's ~160 KB usable heap), a dedicated `HEAP_GUARD_MODE_TABLE`, and a real test that verifies: active mode unchanged, `_lastError` contains "Insufficient memory", `SwitchState` returns to IDLE. |
| medium | `test_switch_returns_to_idle_after_fade` used `makeTestCtx()` (zero matrix dimensions) which triggered the instant-handoff skip path, never executing the fade code. The misleading name created false confidence that the fade path was covered. | Renamed to `test_switch_returns_to_idle_after_fade_skip` and added a clarifying comment. The `RUN_TEST` registration was updated accordingly. |
| dismissed | AC #11 — `tick()` wired into `displayTask` | FALSE POSITIVE: Prior ANTIPATTERNS record classifies as false positive. AC #11 explicitly allowed early integration if required by tests, and this is ds-1.5 scope work the agent chose to deliver early. Functionally correct, no regressions. |
| dismissed | AC #3 — stub modes contain full implementations | FALSE POSITIVE: Prior ANTIPATTERNS record classifies as false positive. High-quality implementations ahead of schedule (ds-1.4 / ds-2.2 scope), not a code defect. --- |

## Story ds-1-4 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `ctx.logoBuffer` null pointer dereference in `renderLogoZone` — `LogoManager::loadLogo` returns false on null but its return is ignored; `drawBitmapRGB565` unconditionally dereferences the bitmap pointer at line 66 of `DisplayUtils.cpp` | Added `if (ctx.logoBuffer == nullptr) return;` guard at top of `renderLogoZone` before any LogoManager or bitmap draw call. |
| high | All render-path tests use `ctx.matrix = nullptr`, triggering the early-return guard on line 46 of `ClassicCardMode.cpp`. Zero lines of `renderLogoZone`, `renderFlightZone`, `renderTelemetryZone`, `renderSingleFlightCard`, `renderLoadingScreen` are exercised in CI. AC #2, #4, #5, #6 have no automated verification. | Deferred — requires `FastLED_NeoMatrix` mock or spy infrastructure. Logged as `[AI-Review]` task. |
| high | Flight cycling logic (`_currentFlightIndex`, `_lastCycleMs`) runs after `fillScreen(0)` on line 48, which itself requires a non-null matrix. Since all tests use null matrix, the cycling state update code (lines 54-65) is completely unreachable. AC #3 has no automated coverage. | Deferred — same constraint as above. Logged as `[AI-Review]` task. |
| medium | `renderSingleFlightCard` computes `innerWidth = mw - 2 - (2 * padding)` without guarding against `mw < 6`. A matrix narrower than 6px produces `innerWidth <= 0`; passing that as `maxCols` to `truncateToColumns(text, int)` causes the negative int to implicitly convert to a large `unsigned int` in `String::substring`, drawing untruncated text outside display bounds. | Added `if (innerWidth <= 0 \|\| innerHeight <= 0) return;` and `if (maxCols <= 0) return;` guards immediately after the inner-dimension calculations. |
| medium | `renderFlightZone` and `renderTelemetryZone` construct 6–10 `String` objects per call on the 20fps display task hot path, generating per-frame heap allocations and fragmentation on the ESP32's non-compacting heap. | Deferred — refactor to `char[]`/`snprintf` without pixel change. This was also present in the original `NeoMatrixDisplay` code. Logged as `[AI-Review]` task. |
| medium | `renderZoneFlight` passes `ctx.layout.logoZone`, `flightZone`, `telemetryZone` directly to draw helpers without clipping against matrix dimensions. A malformed `RenderContext` can cause draws outside framebuffer bounds. | Deferred — same pattern as original `NeoMatrixDisplay`. `LayoutEngine` is the authoritative producer of zone Rects; adding second-layer validation is a defensive enhancement. Logged as `[AI-Review]` task. |
| low | `test_init_resets_cycling_state` promises state-reset verification but only calls `render()` with a null matrix (no cycling logic runs) and asserts `TEST_ASSERT_TRUE(true)`. Name is actively misleading. | Renamed to `test_init_teardown_cycle_no_crash` with an explanatory comment documenting why state verification is deferred. |
| low | `monitor.assertNoLeak(256)` allows 256 bytes of "leak" tolerance across 10 `init()`/`teardown()` cycles for a mode with zero persistent heap allocations. A 200-byte leak per cycle would accumulate to 2KB and still pass. | Tightened to `assertNoLeak(32)` with a comment explaining the intent (32 bytes accounts for FreeRTOS timer-task noise on ESP32). |
| dismissed | "Legacy `displayFlights` behavior destroyed — AC #11 violated" (Reviewer B) | FALSE POSITIVE: `displayFlights` was intentionally simplified to `displayFallbackCard + show()` as part of ds-1.3's ModeRegistry integration, which is confirmed by: (a) the `// AC #7: delegate to displayFallbackCard + show() for compatibility` comment in the production code, (b) the ds-1.3 ANTIPATTERNS record classifying "ModeRegistry::tick wired into displayTask" as a FALSE POSITIVE, (c) the entire purpose of ds-1.4 being to migrate cycling into the new mode system. The behavior was intentionally moved, not accidentally broken. |
| dismissed | "Prematurely integrated into main.cpp — scope creep violating AC #11" (Reviewer B) | FALSE POSITIVE: The ds-1.3 ANTIPATTERNS record (filed 2026-04-14) explicitly classified this as `FALSE POSITIVE`: "AC #11 permitted early integration 'if required by tests' and the dev agent chose to implement ds-1.5 work at the same time. The functional behavior is correct, no regression. Sprint planning violation only." This story's synthesis must respect prior adjudicated decisions. |
| dismissed | NFR C1 "pixel-identical" is impossible to satisfy for multi-flight case because Foundation reference always renders `flights[0]` (Reviewer A, Finding #4) | FALSE POSITIVE: The current `displayFlights` Foundation already had its cycling removed in ds-1.3. `ClassicCardMode` RESTORES the cycling behavior that predated ds-1.3, which is the correct semantic intent. NFR C1's "pixel-identical" applies to the per-frame rendering of a given flight (zone layout, text, colors), not to cycling cadence vs the degraded legacy path. The NFR wording needs clarification (see AI-Review documentation task), but it's not a code defect. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

=== ADVERSARIAL CODE REVIEW REPORT START ===

## Validator Review: Story ds-1.4 (Classic Card Mode Implementation)
**Role:** Read-only adversarial validator  
**Scope:** `firmware/modes/ClassicCardMode.{h,cpp}`, `firmware/utils/DisplayUtils.{h,cpp}`, `firmware/test/test_classic_card_mode/test_main.cpp`

---

### Issues Found (8)

#### 1. Cycling timer drift causes unstable flight cadence — HIGH (Performance / Timing)
**File:** `firmware/modes/ClassicCardMode.cpp:77`  
**Evidence:**  
```cpp
if (now - _lastCycleMs >= ctx.displayCycleMs) {
    _lastCycleMs = now;   // <-- resets anchor to "now" on every advance
    _currentFlightIndex = (_currentFlightIndex + 1) % flights.size();
}
```
Resetting `_lastCycleMs` to the current time on every advance accumulates drift equal to the frame jitter (e.g., at 20fps the average interval becomes `displayCycleMs + ~25ms`). Over long runtimes the displayed flight will rotate measurably slower than the configured `displayCycleMs`.  
**Fix:** Use `_lastCycleMs += ctx.displayCycleMs;` to maintain a stable average cadence.

---

#### 2. `ctx.displayCycleMs == 0` causes runaway index rotation — MEDIUM (Robustness)
**File:** `firmware/modes/ClassicCardMode.cpp:76`  
**Evidence:**  
There is no lower-bound guard on `displayCycleMs`. If the caller passes `0` (malformed `RenderContext` or bad NVS config), `now - _lastCycleMs >= 0` is always true and `_currentFlightIndex` advances on **every** `render()` call, making the card unreadable.  
**Fix:** Add `if (ctx.displayCycleMs == 0) return;` or `>= minimumCycleMs` (e.g., 1000ms) before the interval check.

---

#### 3. `drawBitmapRGB565` dereferences bitmap without null guard — MEDIUM (Security / Robustness)
**File:** `firmware/utils/DisplayUtils.cpp:117`  
**Evidence:**  
```cpp
uint16_t pixel = bitmap[(row + offsetY) * w + (col + offsetX)];
```
`DisplayUtils::drawBitmapRGB565` unconditionally indexes `bitmap`. While `ClassicCardMode::renderLogoZone` now guards `ctx.logoBuffer`, the utility is a public API; any future mode that forgets the guard will hard-fault.  
**Fix:** Add `if (bitmap == nullptr) return;` at the top of the function.

---

#### 4. Test coverage gaps for render branching logic — MEDIUM (Test Coverage)
**File:** `firmware/test/test_classic_card_mode/test_main.cpp`  
**Evidence:**  
The real-matrix tests verify only that *some* pixels are non-black. None of the 27 tests independently exercise:
- `renderFlightZone` with 1-line, 2-line, and 3-line `linesAvailable` branches.
- `renderTelemetryZone` with 1-line vs 2-line branches.
- NaN telemetry values producing `"--"` output.
This means a regression that swaps the 1-line/2-line logic or breaks NaN handling would not be caught.  
**Fix:** Add targeted matrix tests that render with zone heights forcing each branch and sample key pixels (or assert expected strings via a GFX spy).

---

#### 5. Zone-clamping test is vacuous — MEDIUM (Test Quality)
**File:** `firmware/test/test_classic_card_mode/test_main.cpp:503`  
**Evidence:**  
```cpp
TEST_ASSERT_TRUE_MESSAGE(true, "Render with oversized zones did not crash");
```
This assertion can never fail. A future refactor that removes `clampZone()` entirely would still pass because the test only verifies "no crash," not that drawing was actually constrained to matrix bounds.  
**Fix:** Assert on post-render pixel state (e.g., verify no pixels were written beyond `TEST_MATRIX_W` / `TEST_MATRIX_H`) or at least compare `countNonBlackPixels()` against an unclamped baseline.

---

#### 6. Zero logo-zone coverage with a real matrix — MEDIUM (Test Coverage)
**File:** `firmware/test/test_classic_card_mode/test_main.cpp:112-113`  
**Evidence:**  
```cpp
ctx.logoBuffer = nullptr;  // Skip logo zone (no LittleFS in test)
```
Every real-matrix test (`makeRealMatrixCtx`) passes `nullptr` for `logoBuffer`, so `renderLogoZone` returns early. The `LogoManager::loadLogo` → `drawBitmapRGB565` integration path is never exercised with an actual pixel buffer.  
**Fix:** Allocate a dummy 2048-byte RGB565 buffer in at least one real-matrix test and verify that logo-zone pixels change after render.

---

#### 7. Redundant `strlen()` calls on hot render path — LOW (Performance)
**File:** `firmware/modes/ClassicCardMode.cpp:173-196`  
**Evidence:**  
`renderFlightZone` calls `strlen(route)` at least 4 times and `strlen(aircraftSrc)` multiple times on the same immutable stack buffers. At 20fps this is unnecessary O(n) work inside a timing-critical display task.  
**Fix:** Cache lengths in local `int` variables once per buffer (e.g., `int routeLen = strlen(route);`).

---

#### 8. Misleading test name implies non-existent factory — LOW (Code Quality)
**File:** `firmware/test/test_classic_card_mode/test_main.cpp:260`  
**Evidence:**  
```cpp
void test_classic_mode_factory_creates_instance() {
    ClassicCardMode* mode = new ClassicCardMode();
    ...
}
```
The test name claims a factory is being tested, but the code simply calls `new` directly. There is no factory abstraction in the current codebase. This misleads future maintainers about the testing intent.  
**Fix:** Rename to `test_classic_mode_heap_instantiation`.

---

### Evidence Score
**8.5 / 10**

All findings are directly traceable to specific lines in the modified files. The two HIGH/MEDIUM timing and robustness issues are material; the remainder are coverage gaps and code-quality nits that do not block functionality but would improve maintainability and safety.

### Verdict
**CONDITIONAL PASS**

Story ds-1.4 meets its core acceptance criteria: the mode renders via `RenderContext` with zero `ConfigManager` reads, no `FastLED.show()` calls, cycling logic is present, and the build is clean. The 4 prior AI-Review follow-ups (real-matrix tests, cycling testability, String→char[] refactor, and zone clamping) have been resolved satisfactorily.  

**Recommended actions before closing the sprint:**
1. Fix the cycling drift (Issue #1) and add a zero-guard (Issue #2).
2. Harden `drawBitmapRGB565` with a null-guard (Issue #3).
3. Strengthen the zone-clamping test so it actually validates clipping behavior (Issue #5).

=== ADVERSARIAL CODE REVIEW REPORT END ===

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-1-4-classic-card-mode-implementation.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 3 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 1 |
| Lying Tests | 2 |
| Tech Debt Bombs | 1 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Stack buffer overflow risk via `dtostrf` without bounds checking in telemetry formatting | `firmware/utils/DisplayUtils.cpp:48` | +3.0 |
| 🔴 CRITICAL | Buffer overflow in `truncateToColumns` when `outLen` is extremely small | `firmware/utils/DisplayUtils.cpp:32` | +3.0 |
| 🟠 IMPORTANT | `route` format creates dead code and logic bug (fails to fallback to airline name) | `firmware/modes/ClassicCardMode.cpp:115` | +1.0 |
| 🟠 IMPORTANT | `clampZone` fails to clamp negative coordinates and incorrectly increases zone width | `firmware/modes/ClassicCardMode.cpp:33` | +1.0 |
| 🟠 IMPORTANT | Performance footgun: Redundant unpacking/packing of 16-bit RGB565 pixels | `firmware/utils/DisplayUtils.cpp:80` | +1.0 |
| 🟠 IMPORTANT | Lying Test: Weak assertions (`nonBlack > 0`) do not actually verify loading/fallback screens | `firmware/test/test_classic_card_mode/test_main.cpp:259` | +1.0 |
| 🟠 IMPORTANT | Lying Test: Out-of-bounds test only checks positive coordinates, hiding negative bug | `firmware/test/test_classic_card_mode/test_main.cpp:339` | +1.0 |
| 🟡 MINOR | Tautological check on `strlen(telLine2) > 0` is deceptive tech debt | `firmware/modes/ClassicCardMode.cpp:176` | +0.3 |
| 🟢 CLEAN PASS | 4 categories passed (SOLID, Abstraction, Style, Type Safety) | | -2.0 |

### Evidence Score: 9.3

| Score | Verdict |
|-------|---------|
| **9.3** | **REJECT** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

- **[High] Redundant RGB565 Unpack/Repack:** `DisplayUtils::drawBitmapRGB565` unpacks a 16-bit RGB565 pixel into 8-bit R, G, B components with bitshifts, and immediately calls `matrix->Color(r, g, b)`, which packs them right back into a 16-bit RGB565 value. This causes redundant bitwise operations in a tight double-loop rendering on the ESP32 hot path.
  - 📍 `firmware/utils/DisplayUtils.cpp:80`
  - 💡 Fix: Directly pass the 16-bit `pixel` to `matrix->drawPixel(destX + col, destY + row, pixel);`.

---

## 🐛 Correctness & Safety

- **🔒 [CRITICAL] Security (Stack Buffer Overflow):** `DisplayUtils::formatTelemetryValue` uses `dtostrf(value, 0, decimals, valBuf)` where `valBuf` is `char valBuf[16]`. `dtostrf` does not perform bounds checking. If a telemetry value (e.g. `speed_mph` or `altitude_kft`) is unexpectedly large, infinite, or malformed, `dtostrf` will write past the 16-byte boundary and smash the stack.
  - 📍 `firmware/utils/DisplayUtils.cpp:48`
  - ⚠️ Impact: Severe risk of hard crash or arbitrary code execution via malformed API response.

- **🐛 Bug (Buffer Overflow):** In `DisplayUtils::truncateToColumns`, when `maxColumns > 3` but `outLen < 4`, the `else` branch executes `memcpy(out + prefixLen, "...", 3); out[prefixLen + 3] = '\0';`. This forces 4 bytes to be written into the `out` buffer regardless of its actual capacity `outLen`, causing an unconditional buffer overflow for extremely small output buffers.
  - 📍 `firmware/utils/DisplayUtils.cpp:32`
  - 🔄 Reproduction: Call `truncateToColumns("Test", 5, buf, 2)`.

- **🐛 Bug (Logic Flaw):** `clampZone` relies on implicit unsigned promotion which fails for negative coordinates (`c.x >= matW` is false for negative `c.x`). Furthermore, if `c.x < 0` and `c.x + c.w > matW`, the calculation `c.w = matW - c.x` will *increase* the width of the zone rather than clamping it (e.g., `64 - (-5) = 69`), causing downstream truncation calculations to overflow the actual zone boundary.
  - 📍 `firmware/modes/ClassicCardMode.cpp:33`
  - 🔄 Reproduction: Pass a zone with `x = -5, y = 0, w = 100, h = 10`.

- **🎭 Lying Test:** `test_render_loading_screen_draws_border` and `test_render_fallback_card_draws_border`
  - 📍 `firmware/test/test_classic_card_mode/test_main.cpp:259` and `278`
  - 🤥 Why it lies: These tests attempt to verify that specific borders and text are drawn by asserting `TEST_ASSERT_TRUE(nonBlack > 0)`. This is an extremely weak assertion; simply counting if at least one pixel changed does not verify a border, a shape, or any specific text. A single stray pixel from memory corruption or an entirely filled screen would pass.

- **🎭 Lying Test:** `test_render_zone_completely_outside_matrix`
  - 📍 `firmware/test/test_classic_card_mode/test_main.cpp:339`
  - 🤥 Why it lies: Claims to verify that out-of-bounds zones are skipped but only tests large positive values (e.g., 200). Since `clampZone` handles positive out-of-bounds correctly but fails completely on negative out-of-bounds coordinates, this test provides a false sense of security and completely hides the negative boundary edge case.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `route` is unconditionally formatted as `snprintf(route, ..., "%s>%s", ...)`. Thus, `strlen(route)` is always at least 1 (the `>` character). This breaks the compact mode fallback `compactSrc = (strlen(route) > 0) ? route : airline;` (it displays `>` instead of falling back to the airline name). It also makes the branch `else if (strlen(aircraftSrc) > 0)` unreachable dead code because the first `if` condition will always be satisfied.
  - 📍 `firmware/modes/ClassicCardMode.cpp:115` and `125`
  - 💥 Explosion radius: Broken UI rendering for flights lacking origin/destination and completely untested dead code paths.

- **💣 Tech Debt:** Tautological check on `strlen(telLine2) > 0`.
  - 📍 `firmware/modes/ClassicCardMode.cpp:176`
  - 💥 Explosion radius: `telLine2` is populated via `snprintf(combined, ..., "%s %s", trk, vr);`, meaning it will always contain at least a space. `strlen(telLine2) > 0` is tautological and misleading to future maintainers.

---

## 🛠️ Suggested Fixes

### 1. Fix Telemetry Stack Buffer Overflow
**File:** `firmware/utils/DisplayUtils.cpp`
**Issue:** `dtostrf` writes without bounds checking, risking a stack overflow for large telemetry values.

**Corrected code:**
```cpp
void DisplayUtils::formatTelemetryValue(double value, const char* suffix, int decimals,
                                         char* out, size_t outLen)
{
    if (isnan(value)) {
        strncpy(out, "--", outLen - 1);
        out[outLen > 0 ? outLen - 1 : 0] = '\0';
        return;
    }
    if (decimals == 0) {
        snprintf(out, outLen, "%d%s", (int)value, suffix);
    } else {
        char valBuf[16];
        snprintf(valBuf, sizeof(valBuf), "%.*f", decimals, value);
        snprintf(out, outLen, "%s%s", valBuf, suffix);
    }
}
```

### 2. Fix truncateToColumns Buffer Overflow
**File:** `firmware/utils/DisplayUtils.cpp`
**Issue:** When `outLen` is very small, the function writes 4 bytes unconditionally, overflowing the buffer.

**Diff:**
```diff
     } else {
         int prefixLen = maxColumns - 3;
         if (prefixLen > (int)(outLen - 4)) prefixLen = (int)(outLen - 4);
         if (prefixLen < 0) prefixLen = 0;
-        memcpy(out, text, prefixLen);
-        memcpy(out + prefixLen, "...", 3);
-        out[prefixLen + 3] = '\0';
+        if (outLen > 0) {
+            memcpy(out, text, prefixLen);
+            int dots = outLen - 1 - prefixLen;
+            if (dots > 3) dots = 3;
+            if (dots > 0) memcpy(out + prefixLen, "...", dots);
+            out[prefixLen + dots] = '\0';
+        }
     }
```

### 3. Eliminate Redundant RGB565 Packing
**File:** `firmware/utils/DisplayUtils.cpp`
**Issue:** Unpacking a 16-bit RGB565 pixel to 8-bit components just to repack it is a severe performance footgun.

**Diff:**
```diff
         for (uint16_t col = 0; col < drawW; col++) {
             uint16_t pixel = bitmap[(row + offsetY) * w + (col + offsetX)];
             if (pixel != 0) {
-                // Convert RGB565 to R, G, B components
-                uint8_t r = ((pixel >> 11) & 0x1F) << 3;
-                uint8_t g = ((pixel >> 5) & 0x3F) << 2;
-                uint8_t b = (pixel & 0x1F) << 3;
-                matrix->drawPixel(destX + col, destY + row, matrix->Color(r, g, b));
+                // Pass RGB565 pixel directly; no need to unpack/repack
+                matrix->drawPixel(destX + col, destY + row, pixel);
             }
         }
```

### 4. Fix Logic Bug / Dead Code in Route Formatting
**File:** `firmware/modes/ClassicCardMode.cpp`
**Issue:** `route` always has a length > 0 (it contains `>`), causing `compactSrc` to never fallback to `airline` and creating dead code.

**Diff:**
```diff
-    char route[LINE_BUF_SIZE];
-    snprintf(route, sizeof(route), "%s>%s",
-             f.origin.code_icao.c_str(), f.destination.code_icao.c_str());
+    char route[LINE_BUF_SIZE] = "";
+    if (f.origin.code_icao.length() > 0 || f.destination.code_icao.length() > 0) {
+        snprintf(route, sizeof(route), "%s>%s",
+                 f.origin.code_icao.c_str(), f.destination.code_icao.c_str());
+    }
```

### 5. Fix clampZone Bounds Checking for Negative Coordinates
**File:** `firmware/modes/ClassicCardMode.cpp`
**Issue:** `clampZone` relies on implicit unsigned promotion which fails for negative coordinates, resulting in increased zone width instead of clipping.

**Diff:**
```diff
 static Rect clampZone(const Rect& zone, uint16_t matW, uint16_t matH) {
     Rect c = zone;
-    if (c.x >= matW || c.y >= matH) { c.x = 0; c.y = 0; c.w = 0; c.h = 0; return c; }
-    if (c.x + c.w > matW) c.w = matW - c.x;
-    if (c.y + c.h > matH) c.h = matH - c.y;
+    if (c.x >= matW || c.y >= matH || c.x + c.w <= 0 || c.y + c.h <= 0) {
+        c.x = 0; c.y = 0; c.w = 0; c.h = 0; return c;
+    }
+    if (c.x < 0) { c.w += c.x; c.x = 0; }
+    if (c.y < 0) { c.h += c.y; c.y = 0; }
+    if (c.x + c.w > matW) c.w = matW - c.x;
+    if (c.y + c.h > matH) c.h = matH - c.y;
     return c;
 }
```

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 0

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
<var name="session_id">f6d0f6a0-c24e-4328-ba87-861883f255a1</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="6002f7fb">embedded in prompt, file id: 6002f7fb</var>
<var name="story_id">ds-1.4</var>
<var name="story_key">ds-1-4-classic-card-mode-implementation</var>
<var name="story_num">4</var>
<var name="story_title">4-classic-card-mode-implementation</var>
<var name="template">False</var>
<var name="timestamp">20260414_1701</var>
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
      - Commit message format: fix(component): brief description (synthesis-ds-1.4)
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