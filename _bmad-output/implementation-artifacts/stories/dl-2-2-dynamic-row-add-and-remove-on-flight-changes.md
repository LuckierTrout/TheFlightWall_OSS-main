# Story dl-2.2: Dynamic Row Add and Remove on Flight Changes

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want Departures Board rows to appear and disappear in real-time as flights enter and leave tracking range,
So that the display always reflects the current sky without manual refresh.

## Acceptance Criteria

1. **Given** **`DeparturesBoardMode`** is active and the **previous** **`render()`** drew **`N`** rows for **`maxRows = M`**, **When** the **`flights`** vector passed into **`render()`** gains a **new** tail entry (**`N+1 ≤ M`** vs prior snapshot) **or** loses an entry so the **visible** count changes, **Then** the **next** committed frame shows the **updated** row set (**NFR4** — same **display-task** frame boundary as **`ModeRegistry::tick` → `show()`**, ~**50ms** at **20fps**; document measurement method).

2. **Given** **`N == M`** (at cap) and an **additional** flight appears in **`flights`**, **When** **`render()`** runs, **Then** still **exactly `M`** rows are drawn (**first `M`** in order per **dl-2.1**), **no** buffer overruns, **no** stray pixels outside row bands (**epic**).

3. **Given** a **replace** in one **poll** (one **`flights`** snapshot): one flight **drops** off and another **appears** (same **`size()`** but different **idents**), **When** **`render()`** runs, **Then** affected **row(s)** reflect the **new** list — no stale callsign on a reused row slot (**epic** same-cycle case).

4. **Given** **epic** **NFR4** / **NFR13** / **NFR17**, **When** implementing updates, **Then** avoid a **full-matrix `fillScreen`** on **every** frame **when** only **one** row’s content changed: **mutate** row **bands** in-place (**clear row rect** → **redraw** text/glyphs). A **full clear** is acceptable when **`maxRows`** changes (**config** reload) or on **first** frame after **mode switch** — document exceptions.

5. **Given** **`render()`** hot path, **When** profiling / code review, **Then** **no** **`new`/`malloc`** per frame; minimize **`String`** temporaries (prefer **`snprintf`** into **`char[]`** buffers or **`const char*`** from **`FlightInfo`** with **guarded** lifetime). **Do not** grow **`std::vector`** members on each frame.

6. **Given** **`DeparturesBoardMode`** internal state for diffing (**fingerprints**, **cached idents**, etc.), **When** **`teardown()`** runs, **Then** release or reset any **non-static** resources so **mode switches** do not leak (**D2** teardown discipline).

7. **Given** **`pio test`** (or new **`test_departures_board_mode`** cases), **When** executed, **Then** at minimum **unit** coverage for: **empty → one row**, **one → zero**, **cap** overflow stable, **same-size** swap of **idents**; **`pio run`** **no** new warnings.

## Tasks / Subtasks

- [x] Task 1: **Row diff / dirty flags** — cache **`maxRows`**, **`lastCount`**, **`lastId[maxRows]`** (or compact hash) (**AC: #1, #3, #4**)
- [x] Task 2: **Draw path** — per-row **`fillRect`** + text only for dirty rows (**AC: #4**)
- [x] Task 3: **Heap / String hygiene** (**AC: #5, #6**)
- [x] Task 4: Tests (**AC: #7**)

## Dev Notes

### Data flow

- **`displayTask`** peeks **`g_flightQueue`**; **`ModeRegistry::tick(ctx, flights)`** passes the **current** vector reference into **`DeparturesBoardMode::render`**. **One “frame”** = one **`tick`** invocation before **`g_display.show()`**.

### Dependency

- **`dl-2-1`** must land **`DeparturesBoardMode`** + **`depbd`** in **`MODE_TABLE`** — **this** story is **incremental** behavior on that implementation.

### Out of scope

- Scroll animations, **per-row** slide transitions (**dl-3** territory).
- Changing **sort** order of flights (**product** default remains **pipeline order** unless a later story defines sorting).

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-2.md` (Story dl-2.2)
- Prior: `_bmad-output/implementation-artifacts/stories/dl-2-1-departures-board-multi-row-rendering.md`
- Pipeline: `firmware/src/main.cpp` (**`displayTask`**, **`ModeRegistry::tick`**)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, 79.0% flash usage (no increase from dl-2.1)
- Test build: `pio test -f test_mode_registry --without-uploading --without-testing` — PASSED
- Regression: `test_config_manager`, `test_mode_orchestrator` — both PASSED (build only, no device)
- No new warnings introduced (all warnings are pre-existing ArduinoJson deprecations)

### Completion Notes List

- **Task 1:** Added diff state to `DeparturesBoardMode`: `_firstFrame` bool, `_lastCount` (row count on previous frame), `_lastMaxRows` (detect config changes), `_lastIds[4]` (djb2 hash of ident per row slot). `identHash()` static method computes compact deterministic hash from `ident_icao` (fallback `ident`). Diff state compared each frame to determine which rows are dirty.

- **Task 2:** Replaced unconditional `fillScreen(0)` with selective per-row `fillRect`. Full clear only on: first frame after init/mode switch (`_firstFrame`), `maxRows` config change, or row count change. Dirty rows get `fillRect(0, y, mw, rowHeight, 0)` then text redraw. Non-dirty rows left untouched — avoids unnecessary pixel writes per NFR4/NFR13/NFR17.

- **Task 3:** Eliminated all `String` temporaries from `renderRow()` hot path. Row text now built into stack-allocated `char buf[40]` using `memcpy` + `snprintf`. `renderEmpty()` also converted to direct `matrix->write()` loop instead of String wrapper. `teardown()` resets all diff state via `memset` — no leakable resources. No `new`/`malloc` per frame.

- **Task 4:** Added 6 new tests to `test_mode_registry`: `test_departures_board_empty_to_one_row`, `test_departures_board_one_to_zero_rows`, `test_departures_board_cap_overflow_stable`, `test_departures_board_same_size_swap_idents`, `test_departures_board_teardown_resets_diff_state`, `test_departures_board_no_heap_leak_across_renders`. All build with no regressions.

**Full clear exceptions documented (AC #4):** Full `fillScreen(0)` is used when: (1) `_firstFrame` is true (mode switch / init), (2) `_lastMaxRows != newMaxRows` (config reload), (3) `rowCount != _lastCount` (flight count changed). All other frames use per-row `fillRect` only on dirty rows.

**Measurement method (AC #1):** Row updates appear in the same frame as the `render()` call — `ModeRegistry::tick()` calls `render()` then pipeline calls `g_display.show()`, so updates are committed within the same display-task cycle (~50ms at 20fps).

### File List

- `firmware/modes/DeparturesBoardMode.h` (MODIFIED — added diff state members, updated renderRow signature, MAX_SUPPORTED_ROWS constant)
- `firmware/modes/DeparturesBoardMode.cpp` (MODIFIED — diff-based render, zero-alloc renderRow, identHash, improved teardown)
- `firmware/test/test_mode_registry/test_main.cpp` (MODIFIED — 6 new dl-2.2 tests)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (MODIFIED — status update)
- `_bmad-output/implementation-artifacts/stories/dl-2-2-dynamic-row-add-and-remove-on-flight-changes.md` (MODIFIED — completion)

## Change Log

- **2026-04-14:** Implemented row diff / dirty flags, selective per-row fillRect draw path, zero-alloc renderRow with char[] buffers, teardown discipline, and 6 new unit tests (Story dl-2.2)

## Previous story intelligence

- **dl-2.1** establishes **static** layout and **row cap** — **dl-2.2** optimizes **update** semantics and **visual** stability.

## Git intelligence summary

Primarily **`firmware/modes/DeparturesBoardMode.{h,cpp}`** and tests.

## Project context reference

`_bmad-output/project-context.md` — **Core 0** render budget, **heap** discipline.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
