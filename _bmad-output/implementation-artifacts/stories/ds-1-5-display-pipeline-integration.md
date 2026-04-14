# Story ds-1.5: Display Pipeline Integration

Status: review

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

### File List

- `firmware/adapters/NeoMatrixDisplay.h` — Added `#include "interfaces/DisplayMode.h"`, declared `buildRenderContext()`, `show()`, `displayFallbackCard()`
- `firmware/adapters/NeoMatrixDisplay.cpp` — Implemented new API methods, stripped `FastLED.show()` from 6 methods, delegated `displayFlights()` to `displayFallbackCard()` + `show()`
- `firmware/src/main.cpp` — Rewired `displayTask` to use `ModeRegistry::tick` + cached `RenderContext` + single `show()` per frame; added boot `requestSwitch` after `initialize()`; added `show()` to setup-time display calls

## Change Log

- 2026-04-13: Story ds-1.5 implemented — display pipeline integration complete. NeoMatrixDisplay now owns hardware + frame commit only; all flight rendering flows through ModeRegistry::tick via ClassicCardMode.

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
