<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-1 -->
<!-- Story: 5 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260414T220639Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story ds-1.5

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
<file id="6e6ebdee" path="_bmad-output/implementation-artifacts/stories/ds-1-5-display-pipeline-integration.md" label="DOCUMENTATION"><![CDATA[

# Story ds-1.5: Display Pipeline Integration

Status: Ready for Review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user,
I want all flight display output to flow through the mode system with NeoMatrixDisplay owning only hardware and frame commit,
So that the device boots, activates Classic Card mode, and renders identically to the Foundation Release.

## Acceptance Criteria

1. **Given** `ModeRegistry`, `ClassicCardMode` (ds-1.4), and `NeoMatrixDisplay` exist, **When** integration is complete, **Then** `NeoMatrixDisplay` exposes **`RenderContext buildRenderContext() const`** and **`void show()`** (wraps `FastLED.show()` only) per architecture Decision D3, and **`void displayFallbackCard(const std::vector<FlightInfo>& flights)`** for FR36 — fallback draws the same pixels as today’s `renderFlight` path for **index 0** when `flights` non-empty, and loading UI when empty (reuse `displaySingleFlightCard` / `displayLoadingScreen` **draw** logic without internal `FastLED.show()`)

2. **Given** FR35 (single commit site), **When** the normal flight path runs, **Then** **`ModeRegistry::tick(cachedCtx, flights)`** performs all mode drawing and **no code path inside `modes/*` calls `FastLED.show()`**; **`NeoMatrixDisplay::renderFlight`**, **`displayFlights`**, **`displayLoadingScreen`**, **`displayMessage`**, **`renderCalibrationPattern`**, and **`renderPositioningPattern`** **do not** call `FastLED.show()` — the **display task** calls **`g_display.show()` exactly once per frame** after the branch that drew content (calibration, positioning, status message receipt, or mode/fallback path)

3. **Given** architecture Decision D4, **When** `displayTask` runs the flight pipeline, **Then** it maintains **cached `RenderContext`** rebuilt when **`g_configChanged` or `g_scheduleChanged`** is observed (same block as existing brightness / `localDisp` / `localTiming` updates ~L347–391) **or** on first frame (`ctxInitialized`), via **`cachedCtx = g_display.buildRenderContext()`**; **remove** display-task-local **`currentFlightIndex` / `lastCycleMs`** — flight cycling lives in **`ClassicCardMode`** (ds-1.4)

4. **Given** boot and NVS, **When** `setup()` finishes display init, **Then** after **`g_display.initialize()`** (matrix valid), call **`ConfigManager::getDisplayMode()`** and **`ModeRegistry::requestSwitch(...)`** with default **`"classic_card"`** if NVS missing/invalid — **move** this **after** `initialize()` (today `ModeRegistry::init` runs **before** display init at ~L659–662; **reorder** so boot restore runs when `buildRenderContext()` can supply a valid matrix on first tick)

5. **Given** FR36 / AR8, **When** **`ModeRegistry::getActiveMode()`** is **nullptr** after `tick()` (init failure, no previous mode), **Then** the display task invokes **`g_display.displayFallbackCard(flights)`** (or equivalent documented sequence) so the wall never stays blank while flight data exists

6. **Given** the flight queue read pattern in `displayTask` (~L437–467), **When** peek fails or queue is null, **Then** still run **`ModeRegistry::tick(cachedCtx, emptyVector)`** (and fallback if no active mode) **plus** **`g_display.show()`** — avoid frames where **no** draw and **no** show occur

7. **Given** `BaseDisplay::displayFlights`, **When** external code still calls it, **Then** behavior remains defined — **either** delegate to **`displayFallbackCard`** + **`show()`** for compatibility **or** document the sole supported path; **no** duplicate `FastLED.show()` inside the delegated implementation

8. **Given** NFR P2 / C3, **Then** the display task loop keeps **`vTaskDelay(pdMS_TO_TICKS(50))`** (~20 fps) and **no new tasks**; **`tick()`** must not add blocking waits beyond existing switch lifecycle (already accepted tradeoff during `SWITCHING`)

9. **Given** NFR C2, **Then** regression smoke: OTA endpoints, night-mode scheduler flags, NTP, dashboard, health API — **no** routing changes required unless a call site depended on **`renderFlight`** side effects; **verify** calibration and positioning modes still display correctly with the **post-draw `show()`** pattern

10. **Given** NFR C1 (pixel parity), **Then** with **`classic_card`** active and same inputs as pre-integration **`renderFlight`**, the LED output matches Foundation — build on ds-1.4 parity work; document verification in Dev Agent Record

11. **Given** NFR S1 / S2 (heap / rapid switch), **Then** document stress procedure or run abbreviated harness where feasible; **not** a CI gate unless already automated

12. **Given** build health, **Then** **`pio run`** from **`firmware/`** succeeds with no new warnings

## Tasks / Subtasks

- [x] Task 1: NeoMatrixDisplay API (AC: #1, #2, #7)
  - [x] 1.1: Add `buildRenderContext()`, `show()`, `displayFallbackCard()` declarations; include `interfaces/DisplayMode.h` (or forward-decls if you split `RenderContext` — prefer matching architecture)
  - [x] 1.2: Implement `buildRenderContext()` using `_matrix`, `_layout`, `ConfigManager::getDisplay()` / `getTiming()` for text color and `displayCycleMs` (same as architecture snippet)
  - [x] 1.3: Strip **`FastLED.show()`** from flight/loading/message/calibration/positioning/renderFlight paths; keep **`rebuildMatrix` / `clear`** behavior documented if they must still push pixels during setup (see Dev Notes)

- [x] Task 2: displayTask integration (AC: #2, #3, #5, #6, #8)
  - [x] 2.1: Add static `RenderContext cachedCtx`, `bool ctxInitialized`
  - [x] 2.2: On `_cfgChg || _schedChg`, refresh `cachedCtx` after brightness/layout updates
  - [x] 2.3: Replace `g_display.renderFlight` / `showLoading` block with `peek` → `vector` ref or empty → `ModeRegistry::tick` → if `!getActiveMode()` then `displayFallbackCard` → **`g_display.show()`**
  - [x] 2.4: After `displayMessage(...)`, call **`g_display.show()`**; calibration / positioning branches: **`show()`** before `continue`
  - [x] 2.5: Remove redundant flight index / cycle locals

- [x] Task 3: setup() ordering (AC: #4)
  - [x] 3.1: `ModeRegistry::init` may stay early, but **`requestSwitch(ConfigManager::getDisplayMode())`** with fallback to **`classic_card`** must run **after** **`g_display.initialize()`**

- [x] Task 4: Cleanup + verification (AC: #9–#12)
  - [x] 4.1: Grep **`FastLED.show`** under `firmware/` — only **`NeoMatrixDisplay::show`**, **`rebuildMatrix`**, and **`clear`** (if intentionally retained) should remain
  - [x] 4.2: Manual or automated smoke per NFR C2
  - [x] 4.3: `pio run`

#### Review Follow-ups (AI)
- [x] [AI-Review] MEDIUM: `completeOTAAttempt()` calls `ModeRegistry::requestSwitch()` directly, bypassing `ModeOrchestrator::onManualSwitch()` (Architecture Rule 24) — called from Core 1 (OTAUpdater) (`firmware/core/ModeRegistry.cpp:471`, `firmware/core/OTAUpdater.cpp`)
- [x] [AI-Review] MEDIUM: Status message visually lost when `reconfigureFromConfig()` → `rebuildMatrix()` → `clear()` fires during the `statusMessageVisible=true` window — framebuffer is erased but not redrawn (`firmware/src/main.cpp` display task, `firmware/adapters/NeoMatrixDisplay.cpp:clear`)

## Dev Notes

### Current display task reference (replace flight block)

Flight rendering today: `firmware/src/main.cpp` **`displayTask`** ~L437–467 calls **`g_display.renderFlight(flights, currentFlightIndex)`**; **`showLoading()`** when empty. **Cycling** is duplicated in the task — **remove** in favor of **`ClassicCardMode`**.

### Architecture references

- **D3** — `buildRenderContext`, `show`, fallback helper, NeoMatrixDisplay responsibility split [Source: `_bmad-output/planning-artifacts/architecture.md`]
- **D4** — cached context + `ModeRegistry::tick` + single `show` [Source: same]

### `FastLED.show()` audit

Current **`NeoMatrixDisplay.cpp`** call sites: **`rebuildMatrix`** (~133), **`clear`** (~205), **`displayFlights`** (~464), **`displayLoadingScreen`** (~491), **`displayMessage`** (~517), **`renderCalibrationPattern`** (~607), **`renderPositioningPattern`** (~709), **`renderFlight`** (~732). **Goal:** display-task-driven frames use **one** `show()` at end; **setup-time** `clear` / `rebuildMatrix` may retain `show` if required so **`setup()`** / **`initialize()`** still visibly reset the panel — document any exception.

### ModeRegistry behavior

- **`tick`** renders only if **`_activeMode != nullptr`** [Source: `firmware/core/ModeRegistry.cpp` ~192–195]
- First successful **`requestSwitch`** + **`tick`** instantiates the mode — until then use **fallback** (AC #5)

### Dependency

- **ds-1.4** must supply real **`ClassicCardMode::render`** (not stub **`fillScreen(0)`**); if ds-1.4 is incomplete, complete it first or gate this story behind it in sprint planning.

### Out of scope

- **WebPortal mode list / POST switch** — **ds-3**
- **Removing** `NeoMatrixDisplay` zone helper methods — optional cleanup later; they may remain for **`displayFallbackCard`** implementation

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-1.md` (Story ds-1.5)
- Prior: `stories/ds-1-4-classic-card-mode-implementation.md`, `ds-1-3-moderegistry-and-static-registration-system.md`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` succeeded with no new warnings (pre-existing ArduinoJson deprecation warnings only)
- Flash usage: 1,232,793 bytes (78.4%) — +16 bytes from integration code
- `FastLED.show` audit: only 3 call sites remain in NeoMatrixDisplay.cpp — `rebuildMatrix` (setup-time), `clear` (setup-time), `show()` (the pipeline wrapper)

### Completion Notes List

- **Task 1**: Added `buildRenderContext()`, `show()`, `displayFallbackCard()` to NeoMatrixDisplay. Stripped `FastLED.show()` from all flight/loading/message/calibration/positioning paths. `rebuildMatrix` and `clear` retain `FastLED.show()` because they execute during `setup()` / `initialize()` before the display task runs. `displayFlights()` now delegates to `displayFallbackCard()` + `show()` for AC #7 backward compatibility.
- **Task 2**: Replaced entire flight rendering block in `displayTask` with `ModeRegistry::tick(cachedCtx, flights)` + fallback + single `g_display.show()`. Added cached `RenderContext` rebuilt on config/schedule changes or first frame. Removed `currentFlightIndex` / `lastCycleMs` locals (cycling now lives in `ClassicCardMode`). Added `g_display.show()` after calibration, positioning, and displayMessage branches. Added static `emptyFlights` vector for frames when queue has no data (AC #6).
- **Task 3**: Added `ModeRegistry::requestSwitch(ConfigManager::getDisplayMode())` after `g_display.initialize()` with fallback to `"classic_card"` if saved mode is invalid. `ModeRegistry::init()` stays early (table registration only).
- **Task 4**: FastLED.show audit passed — no mode code calls FastLED.show(). `pio run` succeeds. Setup-time `showLoading()` and `showInitialWiFiMessage()` now include explicit `g_display.show()` since `displayLoadingScreen` / `displayMessage` no longer commit frames.
- **Pixel parity (AC #10)**: ClassicCardMode::render (ds-1.4) uses identical zone rendering logic as the legacy NeoMatrixDisplay paths. With classic_card active and identical inputs, LED output matches Foundation Release.
- **Heap / stress (AC #11)**: No new heap allocations in hot path. `static emptyFlights` and `static cachedCtx` avoid per-frame allocation. Stress testing requires hardware — documented procedure: rapid `requestSwitch` calls between modes while flights are rendering.
- **Synthesis fixes (2026-04-14)**: (1) Added `esp_task_wdt_reset()` inside `ModeRegistry::executeSwitch` crossfade loop — prevents WDT timeout during ~990ms transition now that fade runs on display task hot path. (2) Fixed stray `>` regression in `displaySingleFlightCard` fallback — mirrors ds-1.4 ClassicCardMode fix. (3) Removed dead `_currentFlightIndex`/`_lastCycleMs` class members (written in `rebuildMatrix` but never read after pipeline migration). (4) Fixed `tickStartupProgress` queue-spam: static `ipShown` flag prevents thousands of `snprintf`+`xQueueOverwrite` calls in the 2000–4000ms startup window. (5) Fixed `buildRenderContext()` brightness: added `_activeBrightness` member kept in sync by `updateBrightness()`, so scheduler dim override is reflected in `RenderContext`. (6) Removed dead code: `renderFlight()` and `makeFlightLine()` (no callers after pipeline migration). (7) Updated stale comment at startup-complete path. `pio run` confirms clean build at 81.2% flash usage.
- **Review follow-up fixes (2026-04-14)**: (1) ✅ Resolved review finding [MEDIUM]: `completeOTAAttempt()` now routes through `ModeOrchestrator::onManualSwitch()` instead of calling `ModeRegistry::requestSwitch()` directly — compliant with Architecture Rule 24. Added `#include "core/ModeOrchestrator.h"` to ModeRegistry.cpp. (2) ✅ Resolved review finding [MEDIUM]: Status message now stored in `lastStatusText[64]` buffer in displayTask; after `reconfigureFromConfig()` → `rebuildMatrix()` → `clear()`, if `statusMessageVisible` is true, the message is redrawn and committed via `g_display.show()`. `pio run` confirms clean build at 81.3% flash usage.

### File List

- `firmware/adapters/NeoMatrixDisplay.h` — Added `#include "interfaces/DisplayMode.h"`, declared `buildRenderContext()`, `show()`, `displayFallbackCard()`; removed dead `renderFlight()` and `makeFlightLine()` declarations; added `_activeBrightness` member; removed unused `_currentFlightIndex`/`_lastCycleMs` members (synthesis)
- `firmware/adapters/NeoMatrixDisplay.cpp` — Implemented new API methods, stripped `FastLED.show()` from 6 methods, delegated `displayFlights()` to `displayFallbackCard()` + `show()`; fixed stray `>` in `displaySingleFlightCard` fallback; fixed `buildRenderContext` brightness to use `_activeBrightness`; removed dead `renderFlight()` and `makeFlightLine()` bodies (synthesis)
- `firmware/src/main.cpp` — Rewired `displayTask` to use `ModeRegistry::tick` + cached `RenderContext` + single `show()` per frame; added boot `requestSwitch` after `initialize()`; added `show()` to setup-time display calls; fixed `tickStartupProgress` queue spam guard; updated stale comment (synthesis)
- `firmware/core/ModeRegistry.cpp` — Added `esp_task_wdt_reset()` inside crossfade loop to prevent WDT timeout during ~990ms transition (synthesis); routed `completeOTAAttempt()` through `ModeOrchestrator::onManualSwitch()` per Rule 24 (review follow-up)

## Change Log

- 2026-04-13: Story ds-1.5 implemented — display pipeline integration complete. NeoMatrixDisplay now owns hardware + frame commit only; all flight rendering flows through ModeRegistry::tick via ClassicCardMode.
- 2026-04-14: Addressed code review findings — 2 items resolved: (1) completeOTAAttempt Rule 24 compliance, (2) status message redraw after reconfigureFromConfig.
- 2026-04-14: Second synthesis pass — 3 additional fixes: (1) displayFallbackCard now uses displaySingleFlightCard exclusively per AC #1/D3 (removed renderZoneFlight conditional branch); (2) Added g_display.show() in statusMessageVisible continue path to commit brightness changes from scheduler dim during persistent messages; (3) Fixed millis() 49-day overflow in status message duration check (now uses overflow-safe unsigned subtraction: statusMessageStartMs + statusMessageDurationMs). Build confirmed clean at 81.1% flash.

## Previous story intelligence

- **ds-1.3** intentionally deferred **`tick()`** in production display task and left **boot `requestSwitch`** minimal — this story **completes** that wiring.
- **ds-1.4** moves classic pixels into **`ClassicCardMode`**; pipeline integration must **not** reintroduce **`ConfigManager`** reads inside the per-frame mode path beyond **`buildRenderContext()`**.

## Git intelligence summary

`main.cpp` already includes **`ModeRegistry.h`** and **`MODE_TABLE`**; integration is primarily **`displayTask`** + **`NeoMatrixDisplay`** API completion.

## Latest tech information

- **FreeRTOS display task**: keep **watchdog** reset and **50 ms** delay pattern; **`show()`** cost dominates — avoid extra **`show`** calls doubling frame time.

## Project context reference

`_bmad-output/project-context.md` — Core 0 display task, **`pio run`** from **`firmware/`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-14
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 4.2 (after false-positive dismissals) → CONDITIONAL PASS
- **Issues Found:** 9 verified (2 High, 4 Medium, 3 Low)
- **Issues Fixed:** 9 (7 original + 2 review follow-ups)
- **Action Items Created:** 2 (all resolved)

#### Review Follow-ups (AI)
- [x] [AI-Review] MEDIUM: `completeOTAAttempt()` calls `ModeRegistry::requestSwitch()` directly, bypassing `ModeOrchestrator::onManualSwitch()` (Architecture Rule 24) — called from Core 1 (OTAUpdater) (`firmware/core/ModeRegistry.cpp:471`, `firmware/core/OTAUpdater.cpp`)
- [x] [AI-Review] MEDIUM: Status message visually lost when `reconfigureFromConfig()` → `rebuildMatrix()` → `clear()` fires during the `statusMessageVisible=true` window — framebuffer is erased but not redrawn (`firmware/src/main.cpp` display task, `firmware/adapters/NeoMatrixDisplay.cpp:clear`)

### Review: 2026-04-14 (Round 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 2.5 (after false-positive dismissals) → PASS
- **Issues Found:** 3 verified (0 Critical, 0 High, 2 Medium, 1 Low)
- **Issues Fixed:** 3
- **Action Items Created:** 0

#### Review Follow-ups (AI)
- [x] [AI-Review] MEDIUM: `displayFallbackCard` uses `renderZoneFlight` conditionally, violating AC #1 ("reuse `displaySingleFlightCard` / `displayLoadingScreen` draw logic") and D3 (NeoMatrixDisplay owns hardware/frame-commit only) (`firmware/adapters/NeoMatrixDisplay.cpp`)
- [x] [AI-Review] MEDIUM: Brightness changes from night scheduler not committed to LEDs during persistent status message — `continue` in `statusMessageVisible` path skips `g_display.show()` (`firmware/src/main.cpp` displayTask)
- [x] [AI-Review] LOW: `millis()` overflow vulnerability in status message duration check — absolute deadline `millis() + durationMs` wraps at 49-day uptime; fixed with overflow-safe unsigned subtraction (`firmware/src/main.cpp` displayTask)


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

## Story ds-1-4 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Cycling timer drift | Changed `_lastCycleMs = now` → `_lastCycleMs += ctx.displayCycleMs` to eliminate accumulated jitter (~25ms per cycle at 20fps, ~1% cadence error). |
| high | `displayCycleMs == 0` runaway cycling | Added `ctx.displayCycleMs > 0 &&` guard before the interval comparison, preventing infinite-advance on malformed RenderContext. |
| medium | `dtostrf` unbounded write | Replaced `dtostrf(value, 0, decimals, valBuf)` with `snprintf(valBuf, sizeof(valBuf), "%.*f", decimals, value)` — bounds-checked on all platforms. |
| medium | `truncateToColumns` buffer overflow when `outLen < 4` | Added `outLen == 0` early return; clamped `maxPrefix = (int)outLen - 4` with negative guard; computed `dots` defensively from remaining capacity. |
| medium | `route` always non-empty → dead fallback code + stray ">" display | Initialized `route[LINE_BUF_SIZE] = ""` and wrapped `snprintf` in `if (origin.length() > 0 \|\| destination.length() > 0)` in both `renderFlightZone` and `renderSingleFlightCard`. Flights with no route codes now correctly fall back to airline name. |
| medium | `drawBitmapRGB565` no null guard on `bitmap` | Added `if (bitmap == nullptr) return;` at function entry — utility API is now safe for any caller, not relying on caller-side guards. |
| medium | Tautological `strlen(telLine2) > 0` check | Simplified to `int linesToDraw = (linesAvailable >= 2) ? 2 : 1;` — `telLine2` is always populated when `linesAvailable >= 2`. |
| low | Redundant RGB565 unpack/repack in `drawBitmapRGB565` | Removed the 6-operation unpack (r/g/b extractions) and `matrix->Color(r,g,b)` repack. Pass `pixel` directly to `drawPixel`. Eliminates ~6 bit-ops per pixel per frame. |
| low | Vacuous zone-clamping test assertion | Replaced `TEST_ASSERT_TRUE_MESSAGE(true, ...)` with `countNonBlackPixels() > 0` assertion — verifies that clamped zones still produced visible output within matrix bounds. |
| low | Misleading test name | Renamed `test_classic_mode_factory_creates_instance` → `test_classic_mode_heap_instantiation` with clarifying comment; updated `RUN_TEST` registration. |

## Story ds-1-4 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `DisplayUtils::truncateToColumns(char*, outLen)` and `formatTelemetryValue(…, outLen)` both use `outLen - 1` arithmetic in the first branches *before* checking `outLen == 0`. When `outLen == 0`, unsigned subtraction produces `SIZE_MAX`, causing catastrophic `memcpy`/`strncpy` buffer overruns. | Added `if (outLen == 0) return;` at the very top of both char-buffer overloads; also simplified the redundant `outLen > 0 ? outLen - 1 : 0` ternary in `formatTelemetryValue` (now unconditionally `outLen - 1`, safe because guard precedes it) |
| high | `_lastCycleMs` is never reset when `flights.size() <= 1` or `flights.empty()`. If the display drops below 2 flights (or clears entirely) and then returns to multi-flight, `_lastCycleMs != 0` skips the fresh-anchor path; if enough time elapsed, the very first frame after re-entry triggers an advance, visually skipping flight[0]. | Added `_lastCycleMs = 0;` in the `else` (single-flight) branch and added an explicit `else { _lastCycleMs = 0; }` to the outer `if (!flights.empty())` block |
| medium | Single-step `_lastCycleMs += ctx.displayCycleMs` advances the anchor by exactly one cycle even when the display was paused for many cycles (OTA download, WiFi reconnect). On resume, `now - _lastCycleMs` remains ≥ `displayCycleMs` for every subsequent frame until caught up — causing rapid index strobing at 20fps for ~N seconds. | Replaced single-step with multi-step: `steps = (now - _lastCycleMs) / displayCycleMs; _currentFlightIndex += steps; _lastCycleMs += steps * displayCycleMs`. Atomically catches up all missed cycles in one render call. |
| dismissed | "Port is not pixel-identical for missing-route flights — legacy always renders `origin>dest`, new port suppresses `>` when both codes are empty" (Validator A Finding #2) | FALSE POSITIVE: This was an **intentional improvement** explicitly documented in the prior synthesis pass Completion Notes: "(5) Route string now initialised to `""` and only formatted when origin/destination codes are present — fixes dead-code fallback and avoids displaying stray `">"` for flights with no route data." The ANTIPATTERNS file (story ds-1-4 block) also adjudicated the NFR C1 "pixel-identical" concern as a FALSE POSITIVE. The change is correct behavior, not a regression. |
| dismissed | "`getZoneDescriptor()` hard-codes percentages that drift from real `LayoutEngine` output — logo zone width varies by matrix aspect ratio" (Validator A Finding #3) | FALSE POSITIVE: AC #10 explicitly accepts that zone descriptor percentages are *schematic* approximations for the Mode Picker UI — "adjust relative schematic percentages if needed so labels align with FR4 regions." The `LayoutEngine` is pixel-authoritative; the descriptor is an informational label overlay. They serve different purposes by design. |
| dismissed | "AC2 parity not actually verified — tests only assert `nonBlack > 0`, not bit-identical output" (Validator A Finding #1) | FALSE POSITIVE: The Visual Verification Checklist documents that AC2 requires on-device side-by-side comparison, which is not feasible in CI without physical ESP32 hardware. The story AC record and dev notes explicitly acknowledge this. This is a documentation observation about test depth, not a code defect that can be fixed in synthesis. |
| dismissed | "Oversized-zone clamp test (`test_render_zone_clamping_oversized_zones`) is a lying test — passes even if `clampZone()` were deleted because FastLED clips `drawPixel` independently" (Validator A Finding #6) | FALSE POSITIVE: The test is a smoke test, not a proof-of-clamping. Its value is verifying no crash and no assertion failure when oversized zones are passed. FastLED's independent clipping is a correct second safety net. The test accurately documents the intended behavior (no crash + visible output). The complementary test `test_render_zone_completely_outside_matrix` (asserts `nonBlack == 0`) provides stronger evidence that clampZone's zero-w/h skip logic works. |

## Story ds-1-5 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Crossfade fade loop (~990ms) never calls `esp_task_wdt_reset()` — now on display task hot path post ds-1.5 wiring | Added `esp_task_wdt_reset()` inside the per-step fade loop after `FastLED.show()` + added `#include "esp_task_wdt.h"` |
| medium | `displaySingleFlightCard` always builds `origin + ">" + dest` even when both are empty strings — re-introduces stray `>` bug fixed in ds-1.4's `ClassicCardMode` | Wrapped route string in `(origin.length() > 0 \|\| dest.length() > 0)` guard, mirroring the ClassicCardMode fix |
| medium | `_currentFlightIndex` / `_lastCycleMs` class members written in `rebuildMatrix()` but never read anywhere after pipeline migration — dead state occupying memory and misleading maintainers | Removed both members from header and removed their initialization from `rebuildMatrix()` |
| medium | `tickStartupProgress` 2000–2100ms window calls `queueDisplayMessage` (snprintf + xQueueOverwrite) on every `loop()` iteration (~1000s per second) for the full 100ms window | Added `static bool ipShown = false;` guard; now queues exactly once when elapsed ≥ 2000ms |
| medium | AC #2 and AC #7 are directly contradictory (AC #2 prohibits `FastLED.show()` in `displayFlights`, AC #7 explicitly permits `displayFallbackCard + show()` delegation) | Documentation only — no code change needed; AC #7 is the intended behavior and the implementation is correct per AC #7. Story wording deferred to editorial cleanup. |
| low | `buildRenderContext()` captures NVS `brightness` value, ignoring runtime scheduler dim override applied via `updateBrightness()` — stale value in cached `RenderContext` is a latent bug for any future mode that reads it | Added `_activeBrightness` private member, initialized in `rebuildMatrix()`, updated in `updateBrightness()`, used in `buildRenderContext()` (Reviewer B's `_matrix->getBrightness()` was incorrect — method doesn't exist on `FastLED_NeoMatrix`) |
| low | Dead code — `renderFlight()` and `makeFlightLine()` have zero callers after pipeline migration | Removed declarations from header; removed function bodies from .cpp |
| low | Stale comment "display task will show showLoading()" — `displayTask` never calls `showLoading()` in the new pipeline | Updated comment to accurately describe `displayFallbackCard(emptyFlights) → displayLoadingScreen` path |
| dismissed | "CRITICAL task completion lie — `_currentFlightIndex`/`_lastCycleMs` not removed" | FALSE POSITIVE: AC #3 specifically says "remove display-task-local `currentFlightIndex` / `lastCycleMs`". Those *display-task-local* variables WERE removed. The class members in NeoMatrixDisplay are separate entities not mentioned by AC #3. The issue is real but MEDIUM severity (dead state), not CRITICAL (no behavioral bug). Score inflation is dismissed. |
| dismissed | "Architecture Violation — `completeOTAAttempt` bypasses `ModeOrchestrator::onManualSwitch()` per Rule 24" | FALSE POSITIVE: Partially true observation but NOT introduced by ds-1.5. `completeOTAAttempt` was designed this way in dl-7.x stories as an internal ModeRegistry recovery path. The issue is logged as an AI-Review follow-up for future resolution, but is out of scope for ds-1.5 synthesis. |
| dismissed | "State Corruption — `g_layout` API desync when geometry changes" | FALSE POSITIVE: This is **intentional design**: geometry changes (tiles_x, tiles_y) require reboot. The API immediately reports the new config so the dashboard can instruct the user to reboot; the display correctly keeps the old geometry until rebooted. The comment at line 396 explicitly documents this. Not a bug. |
| dismissed | "Boot restore contradicts AC #4 — uses `ModeOrchestrator::onManualSwitch` not `ModeRegistry::requestSwitch`" | FALSE POSITIVE: Implementation is **architecturally superior** (correctly obeys Rule 24). This is a story documentation inaccuracy, not a code defect. No code fix warranted; the implementation is correct. |
| dismissed | "AC #2 violated — `displayFlights` is an indirect `FastLED.show()` call path" | FALSE POSITIVE: AC #7 explicitly permits the `displayFlights → displayFallbackCard + show()` pattern for backward compatibility. The story ACs contradict each other; AC #7 is the correct intent and the implementation follows it. Documentation-only issue. |

## Story ds-1-5 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `displayFallbackCard` calls `renderZoneFlight` when `_layout.valid`, violating AC #1 ("reuse `displaySingleFlightCard` / `displayLoadingScreen` draw logic") and D3 (NeoMatrixDisplay owns hardware and frame commit only) | Removed `if (_layout.valid) { renderZoneFlight(flights[0]); } else { ... }` conditional — fallback now unconditionally calls `displaySingleFlightCard(flights[0])` per AC #1. |
| medium | Brightness changes from night scheduler (or any `g_scheduleChanged`/`g_configChanged` event) are not committed to physical LEDs when a status message is visible — the `continue` at the `statusMessageVisible` path bypasses `g_display.show()`. During indefinite messages (AP Setup mode), this means the scheduler dim window never takes visual effect until the message clears | Added `g_display.show()` immediately before `esp_task_wdt_reset()` / `vTaskDelay` / `continue` in the `statusMessageVisible` loop, so hardware-state changes are committed every frame even during persistent messages. |
| low | `millis()` overflow in status message expiry calculation — `statusMessageUntilMs = millis() + statusMessage.durationMs` produces an absolute deadline that overflows to a small value at 49-day uptime; `millis() < statusMessageUntilMs` then immediately evaluates false, causing messages to flash for one frame | Replaced single `statusMessageUntilMs` variable with `statusMessageStartMs` (time queued) + `statusMessageDurationMs` (duration). Expiry check now uses overflow-safe unsigned subtraction: `(millis() - statusMessageStartMs) < statusMessageDurationMs`. |
| dismissed | Fallback card "permanently loses flight cycling logic" — users stare at a single flight forever during mode failure | FALSE POSITIVE: By design per AC #5. The fallback's purpose is to ensure "the wall never stays blank while flight data exists" — not to cycle flights. Flight cycling is ClassicCardMode's responsibility (AC #3, ds-1.4). The fallback is an emergency path, not a cycling display. Rendering `flights[0]` is the correct and intended behavior when the mode system is unavailable. |
| dismissed | Magic numbers `2000` and `4000` in `tickStartupProgress` startup state machine | FALSE POSITIVE: These hardcoded timing literals are self-documenting in context (surrounding comments explain "Show WiFi Connected", "Show IP address", "Advance to authentication"), follow standard embedded firmware conventions, and appear in a non-hot path that's only exercised once at boot. The existing `ipShown` guard (added in ds-1.5 synthesis) already addresses the actual spam bug. Extracting named constants would provide no bug-prevention value here. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

Hello! I'm a validator Code CLI, ready to help you with software engineering tasks, code changes, research, or anything else you need.

How can I help you today?

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-1-5-display-pipeline-integration.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task completion lie: `FastLED.leds()` and `FastLED.size()` are hallucinated methods that do not exist in the FastLED API. The code does not compile. | `firmware/core/ModeRegistry.cpp` | +3 |
| 🔴 CRITICAL | Concurrency / Use-after-free: Double buffer `g_flightBuf` does not block writer. A rapid fetch can overwrite the active buffer while `ModeRegistry::executeSwitch` is reading it. | `firmware/src/main.cpp` | +3 |
| 🟠 IMPORTANT | Missing AC / SOLID Violation: `renderZoneFlight`, `renderLogoZone`, etc. were explicitly required to be extracted but remain in `NeoMatrixDisplay`. | `firmware/adapters/NeoMatrixDisplay.cpp` | +1 |
| 🟠 IMPORTANT | State Corruption: `displayMessage` calls `fillScreen(0)`, but when the message expires, the screen is not cleared. Text outside rendered zones persists permanently. | `firmware/src/main.cpp` | +1 |
| 🟠 IMPORTANT | Type Safety: `timing.display_cycle * 1000` assigned to `uint16_t` silently overflows for any cycle >65s, truncating the value. | `firmware/adapters/NeoMatrixDisplay.cpp` | +1 |
| 🟠 IMPORTANT | Thread Safety: `SystemStatus::set` is called from the lwIP thread (`onNtpSync`) without a mutex, creating a race condition on the global subsystem registry. | `firmware/src/main.cpp` | +1 |
| 🟢 CLEAN PASS | Performance Footguns | Review | -0.5 |
| 🟢 CLEAN PASS | Security Vulnerabilities | Review | -0.5 |
| 🟢 CLEAN PASS | Lying Tests | Review | -0.5 |
| 🟢 CLEAN PASS | Style & Pythonic Crimes | Review | -0.5 |

### Evidence Score: 8.0

| Score | Verdict |
|-------|---------|
| **8.0** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] S - Single Responsibility:** `NeoMatrixDisplay` was supposed to shed its zone rendering responsibilities to `ClassicCardMode` per Architecture D3. Instead, `renderLogoZone`, `renderFlightZone`, `renderTelemetryZone`, and `renderZoneFlight` were left in the class as dead code/legacy implementations, violating the architectural boundary and the explicit AC #1 extraction mandate.
  - 📍 `firmware/adapters/NeoMatrixDisplay.cpp:241`
  - 💡 Fix: Remove these four methods entirely from both the header and implementation files, as they are now owned by `ClassicCardMode`.

---

## 🐍 Pythonic Crimes & Readability

- **Type Safety:** `ctx.displayCycleMs = timing.display_cycle * 1000;` assigns a potentially 32-bit multiplied value to a `uint16_t` struct field. If a user sets `display_cycle` to 120 seconds, `120000` is truncated modulo 65536 to `54464` (~54s), breaking the user's intent.
  - 📍 `firmware/adapters/NeoMatrixDisplay.cpp:461`

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `FastLED` API hallucination. The developer agent wrote `CRGB* leds = FastLED.leds();` and `int numLeds = FastLED.size();`. The `CFastLED` class in the `FastLED` library does NOT expose a `leds()` or `size()` method. This is a task completion lie: the code could not possibly have compiled, and `pio run` failed.
  - 📍 `firmware/core/ModeRegistry.cpp:115`
  - 🔄 Reproduction: Compile the firmware. It will immediately fail on `FastLED.leds()`.

- **🐛 Bug:** Double buffering race condition on `g_flightBuf`. The writer (`FlightDataFetcher` in Core 1 loop) swaps `g_writeBuf ^= 1` and overwrites the active queue pointer. If `fetch_interval` is small or manually triggered, Core 1 can write to `g_flightBuf[X]` while Core 0 is iterating over its `std::vector` inside a 1-second `executeSwitch` fade transition, leading to a catastrophic `std::vector` use-after-free crash on the ESP32.
  - 📍 `firmware/src/main.cpp:557`
  - 🔄 Reproduction: Trigger two fetches in rapid succession while a 1-second crossfade transition is occurring.

- **🐛 Bug:** Status message screen corruption. `displayMessage` invokes `_matrix->fillScreen(0)` to wipe the background before drawing text. When `statusMessageVisible` expires, normal mode rendering resumes, but `displayTask` does NOT clear the screen. Any status text pixels that fall outside the strictly drawn bounds of `ClassicCardMode`'s zones remain permanently burned into the background.
  - 📍 `firmware/src/main.cpp:287`
  - 🔄 Reproduction: Queue a status message. Wait for it to expire. Observe ghostly text remaining in the margins of the `ClassicCardMode` layout.

- **🐛 Bug:** Thread-unsafe global state modification. `onNtpSync` executes on the `tiT` (lwIP) FreeRTOS task, but it calls `SystemStatus::set()`, modifying the shared subsystem registry without a mutex. This creates a race condition with Core 1 and Core 0 tasks.
  - 📍 `firmware/src/main.cpp:92`
  - 🔄 Reproduction: Have NTP sync exactly when another task (e.g. WebPortal or Display) is iterating or modifying the `SystemStatus` map.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `volatile bool _calibrationMode` is used for cross-core synchronization (written by WebPortal on Core 1, read by DisplayTask on Core 0). C++ `volatile` does not guarantee atomicity or memory ordering, leaving it vulnerable to compiler optimizations.
  - 📍 `firmware/adapters/NeoMatrixDisplay.h:50`
  - 💥 Explosion radius: Low, but sets a dangerous precedent for future developers using `volatile` instead of `std::atomic<bool>` on a dual-core ESP32.

---

## 🛠️ Suggested Fixes

### 1. Fix FastLED API Hallucination

**File:** `firmware/core/ModeRegistry.cpp`
**Issue:** `FastLED.leds()` and `FastLED.size()` do not exist. You must read from the shared array managed by the display.

**Diff:**
```diff
-                CRGB* leds = FastLED.leds();
-                int numLeds = FastLED.size();
+                // Assuming RenderContext is updated to pass the LED array directly,
+                // or you must route this through NeoMatrixDisplay's public API.
+                // (CFastLED does not expose leds() or size())
```

### 2. Fix Double Buffer Race Condition

**File:** `firmware/src/main.cpp`
**Issue:** Prevent Core 1 from overwriting a buffer that Core 0 is currently reading.

**Diff:**
```diff
-        g_writeBuf ^= 1; // swap buffer for next write
+        // You must implement a triple buffer, an atomic refcount, or a mutex
+        // so Core 1 only writes to an explicitly idle buffer.
```

### 3. Fix Status Message Expiry Screen Corruption

**File:** `firmware/src/main.cpp`
**Issue:** Screen must be cleared when a persistent message expires.

**Diff:**
```diff
         if (statusMessageVisible)
         {
             if (statusMessageDurationMs == 0 || (millis() - statusMessageStartMs) < statusMessageDurationMs)
             {
                 g_display.show();
                 esp_task_wdt_reset();
                 vTaskDelay(pdMS_TO_TICKS(50));
                 continue;
             }
             statusMessageVisible = false;
+            g_display.clear(); // Wipe the ghost text before normal modes resume
         }
```

**Review Actions:**
- Issues Found: 6
- Issues Fixed: 0
- Action Items Created: 6

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
<var name="session_id">100b0617-8a66-48a5-b5f9-4ae4b5a71438</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="6e6ebdee">embedded in prompt, file id: 6e6ebdee</var>
<var name="story_id">ds-1.5</var>
<var name="story_key">ds-1-5-display-pipeline-integration</var>
<var name="story_num">5</var>
<var name="story_title">5-display-pipeline-integration</var>
<var name="template">False</var>
<var name="timestamp">20260414_1806</var>
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
      - Commit message format: fix(component): brief description (synthesis-ds-1.5)
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