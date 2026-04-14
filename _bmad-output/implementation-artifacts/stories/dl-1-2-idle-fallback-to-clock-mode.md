# Story dl-1.2: Idle Fallback to Clock Mode

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want the wall to automatically switch to Clock Mode when no flights are in range and switch back when flights return,
So that the wall never shows a blank screen or loading spinner — guests always see a functioning display.

## Acceptance Criteria

1. **Given** **`"clock"`** is registered in **`MODE_TABLE`** (**dl-1.1**), **When** the orchestrator determines **idle fallback** (see AC #3–#5), **Then** **`ModeRegistry::requestSwitch("clock")`** is invoked so the **display task** actually activates **Clock Mode** — **not** only **`ModeOrchestrator`** string fields.

2. **Given** **`IDLE_FALLBACK`** and flights return (**`flightCount > 0`**), **When** **`onFlightsRestored()`** runs, **Then** **`ModeRegistry::requestSwitch(ModeOrchestrator::getManualModeId())`** (or equivalent) restores the owner’s **saved manual** mode id (**`_manualModeId`**). If **`requestSwitch`** returns **false**, log **`LOG_W`**, keep orchestrator state consistent with registry truth (**document** reconciliation — e.g. leave **MANUAL** but surface **`registry_error`** on next **GET**).

3. **Given** **`std::atomic<uint8_t> g_flightCount`** (rule **30**), **When** the firmware publishes the latest enriched flight list to the display pipeline (**`xQueueOverwrite`** path in **`main.cpp`** today), **Then** **`g_flightCount.store()`** reflects **`min(flights.size(), 255)`**. **Epic** cites **`FlightDataFetcher.cpp`** — acceptable to update in **`main.cpp`** immediately after **`fetchFlights`** / queue publish **if** you document that the **count** is tied to the **same** snapshot the display consumes (avoid drift).

4. **Given** **Core 1** **`loop()`**, **When** time advances, **Then** **`ModeOrchestrator::tick(g_flightCount.load())`** runs at **approximately 1-second** intervals (**non-blocking**, no busy-wait longer than **`delay(1)`** style), **independent** of whether a **fetch** is due — so **boot** + **zero** flights transitions within **~1s** after the first tick, not only on the next **10+ minute** fetch (**epic** AC). **Keep** existing **`tick(...)`** call on the fetch path **or** replace with a single periodic site — **do not** double-tick the same wall-clock second unless idempotent (**`onIdleFallback`** already guards re-entry).

5. **Given** a **non-clock** mode is active (**registry** active id ≠ **`"clock"`**) and **`flightCount == 0`** and orchestrator state is **`MANUAL`**, **When** **`tick`** evaluates, **Then** transition to **`IDLE_FALLBACK`** and request **clock** (**epic**).

6. **Given** orchestrator **`MANUAL`**, **`flightCount == 0`**, but registry **already** shows **`"clock"`** (user manually picked clock), **When** **`tick`** runs, **Then** behavior is **safe** (no flip-flop): either **no-op** state transition or enter **`IDLE_FALLBACK`** without a second **`requestSwitch`** — document chosen behavior.

7. **Given** **`OrchestratorState::SCHEDULED`**, **When** **`tick`** runs, **Then** **do not** override schedule logic (**no** change unless **dl-1.4+** defines interaction — default **leave** **`tick`** branches as today for **`SCHEDULED`**).

8. **Given** **NFR** “within one poll interval” for zero-flight detection **after** data exists, **Then** worst-case latency to fallback is **≤ max(fetch_interval, ~1s)`** once **`g_flightCount`** reflects zero — document in Dev Agent Record.

9. **Given** **epic API surface**, **When** compiling, **Then** expose **`getManualModeId()`** on **`ModeOrchestrator`** if still missing (returns **`_manualModeId`**) — used by tests and **GET** transparency if needed.

10. **Given** **`test_mode_orchestrator`**, **When** extended, **Then** mock or assert **`ModeRegistry::requestSwitch`** call counts (**link test** or refactor orchestrator to accept a thin callback — **prefer** minimal **integration** test on device over heavy mocking if impractical).

11. **Given** **`pio run`**, **Then** no new warnings.

## Tasks / Subtasks

- [x] Task 1: **`g_flightCount`** (**AC: #3**)
  - [x] 1.1: Declare **`static std::atomic<uint8_t> g_flightCount(0)`** in **`main.cpp`** (Rule #30: cross-core atomics stay in main.cpp)
  - [x] 1.2: **`store`** immediately after flight queue publish + initialized to **0** at boot

- [x] Task 2: Periodic **`ModeOrchestrator::tick`** (**AC: #4**)
  - [x] 2.1: **`static unsigned long s_lastOrchMs`** in **`loop()`**; if **`millis() - s_lastOrchMs >= 1000`** → **`tick(g_flightCount.load())`**; removed old fetch-path tick to avoid double-tick

- [x] Task 3: **`ModeRegistry`** bridge (**AC: #1, #2**)
  - [x] 3.1: **`onIdleFallback`**: calls **`ModeRegistry::requestSwitch("clock")`** after state update; logs LOG_W on failure
  - [x] 3.2: **`onFlightsRestored`**: calls **`ModeRegistry::requestSwitch(_manualModeId)`**; logs LOG_W on failure, keeps orchestrator in MANUAL (reconciliation documented)
  - [x] 3.3: Verified **`onManualSwitch`** does NOT call **`requestSwitch`** — **`POST /api/display/mode`** in WebPortal only calls orchestrator. **Out of scope for dl-1.2; follow-up for dl-1.3** (manual mode switching via orchestrator story).

- [x] Task 4: **`getManualModeId()`** + tests (**AC: #9, #10**)

## Dev Notes

### Current implementation (gap analysis)

- **`ModeOrchestrator::tick(uint8_t)`** only runs from **`main.cpp`** **inside** the **fetch-due** block — **not** every second.
- **`onIdleFallback` / `onFlightsRestored`** update **orchestrator** mirrors only — **no** **`ModeRegistry::requestSwitch`** anywhere from orchestrator (**grep** verified).
- **`WebPortal::POST /api/display/mode`** calls **`ModeOrchestrator::onManualSwitch`** only — registry may lag (**dl-1.3** territory).

### Hard dependency

- **dl-1.1** — **`"clock"`** in **`MODE_TABLE`** and **`ClockMode`**; without it, **`requestSwitch("clock")`** fails.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-1.md` (Story dl-1.2)
- Prior: `_bmad-output/implementation-artifacts/stories/dl-1-1-clock-mode-time-display.md`
- Code: `firmware/core/ModeOrchestrator.{h,cpp}`, `firmware/src/main.cpp` (**fetch** + **`loop()`**)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, no new warnings (AC #11 satisfied)
- Pre-existing warning: `s_lastAppliedTz` capture — not introduced by this story

### Completion Notes List

- **AC #1**: `onIdleFallback()` now calls `ModeRegistry::requestSwitch("clock")` to drive the display task into Clock Mode, not just orchestrator string fields.
- **AC #2**: `onFlightsRestored()` calls `ModeRegistry::requestSwitch(_manualModeId)` to restore the owner's saved mode. On failure, logs `LOG_W` and keeps orchestrator in MANUAL state (reconciliation: registry_error will surface on next GET).
- **AC #3**: `std::atomic<uint8_t> g_flightCount` declared in `main.cpp` (Rule #30), stored immediately after `xQueueOverwrite` — tied to the same snapshot the display consumes.
- **AC #4**: Periodic 1-second tick via `s_lastOrchMs` in `loop()`, independent of fetch interval. Old fetch-path tick removed (single periodic site).
- **AC #5**: `tick()` logic: `MANUAL + flightCount==0` → `IDLE_FALLBACK` + `requestSwitch("clock")`.
- **AC #6**: Safe when clock already active — `onIdleFallback` re-entry guard prevents double transition. `requestSwitch("clock")` is called once; `ModeRegistry::tick` skips switch if target == active index. Documented behavior: enters `IDLE_FALLBACK` state, single `requestSwitch`, no flip-flop.
- **AC #7**: `SCHEDULED` state not overridden — tick only branches on `MANUAL` and `IDLE_FALLBACK` conditions. No change for `SCHEDULED` (deferred to dl-1.4+).
- **AC #8**: Worst-case fallback latency: ≤ max(fetch_interval, ~1s) once `g_flightCount` reflects zero. The 1-second periodic tick detects zero flights within ~1s of the count being stored.
- **AC #9**: `getManualModeId()` exposed as public static method on `ModeOrchestrator`.
- **AC #10**: Integration tests added to `test_mode_orchestrator`: `test_idle_fallback_requests_clock_in_registry`, `test_flights_restored_requests_manual_mode_in_registry`, `test_idle_fallback_safe_when_clock_already_active`. Used real ModeRegistry with production mode table rather than heavy mocking (per story preference).
- **AC #11**: `pio run` produces no new warnings.
- **dl-1.3 follow-up**: `onManualSwitch` does not call `requestSwitch` — WebPortal POST only updates orchestrator. This is the core scope of story dl-1.3 (manual mode switching via orchestrator).

### File List

- `firmware/core/ModeOrchestrator.h` — added `getManualModeId()` declaration
- `firmware/core/ModeOrchestrator.cpp` — added `ModeRegistry.h` include, `getManualModeId()` implementation, `requestSwitch` calls in `onIdleFallback` and `onFlightsRestored`
- `firmware/src/main.cpp` — added `g_flightCount` atomic, periodic 1s orchestrator tick, `g_flightCount.store()` after queue publish, removed old fetch-path orchestrator tick
- `firmware/test/test_mode_orchestrator/test_main.cpp` — added ModeRegistry integration tests, `getManualModeId` tests, updated to init both registry and orchestrator

### Change Log

- 2026-04-14: Story dl-1.2 implemented — idle fallback bridges ModeOrchestrator to ModeRegistry, periodic tick, g_flightCount atomic, getManualModeId()

## Previous story intelligence

- **dl-1.1** adds **Clock** as a real mode — **dl-1.2** must **drive** **`ModeRegistry`** into **`clock`** on idle.

## Git intelligence summary

Touches **`main.cpp`**, **`ModeOrchestrator.cpp`**, possibly **`ModeOrchestrator.h`**, **`test_mode_orchestrator`**.

## Project context reference

`_bmad-output/project-context.md` — **Core 1** **`loop()`**, **cross-core** **`requestSwitch`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
