<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-2 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T141128Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-2.2

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
<file id="2f7a53f7" path="_bmad-output/implementation-artifacts/stories/dl-2-2-dynamic-row-add-and-remove-on-flight-changes.md" label="DOCUMENTATION"><![CDATA[

# Story dl-2.2: Dynamic Row Add and Remove on Flight Changes

Status: done

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

- Build: `pio run` — SUCCESS, 80.6% flash usage (1,268,085 bytes / 1,572,864 bytes)
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

### Code Review Synthesis (2026-04-16)

- **Critical Fix:** identHash() now includes altitude_kft and speed_mph in hash computation. Previous implementation only hashed callsign, causing stale telemetry display when flight data changed. Uses type-punning via union for float-to-bits conversion (zero-heap).

- **Safety Fix:** Added bounds check for curIds array access — clamp hashCount to MAX_SUPPORTED_ROWS before loop to prevent buffer overrun if rowCount exceeds array size.

- **Safety Fix:** snprintf buffer overflow protection — check return value and clamp pos to buffer end if truncation occurs. Added bounds checks for all memcpy operations in renderRow().

- **Code Quality:** Added const qualifier to maxCols in renderRow() for immutability.

- **Tests:** All existing tests pass. Build successful (81.0% flash usage, no increase). No new warnings introduced beyond pre-existing ArduinoJson deprecations.

### Code Review Synthesis Round 2 (2026-04-16)

- **Standards Compliance Fix (HIGH):** Replaced union type-punning with memcpy for float-to-uint32_t conversion in identHash(). Union type-punning violates C++ strict aliasing rules (undefined behavior). memcpy provides standards-compliant type conversion with identical zero-heap footprint.

- **Safety Fix (MEDIUM):** Added rowHeight == 0 guard after integer division (line 143-146). Prevents rendering with zero-height rows if matrix dimensions are extremely small (e.g., 10px tall with 4 rows → rowHeight = 2, but edge cases exist).

- **Validation Quality Assessment:** Validator A score: 5.3 (3 false positives, 1 valid issue). Validator B score: 6.9 (6 false positives, 2 valid issues). Most claimed issues were already fixed in prior review round or incorrect interpretations of intentional design decisions documented in antipatterns log.

- **Build/Test:** All tests pass (pio test -f test_mode_registry, test_config_manager). Firmware builds successfully. Flash usage: 81.0% (1,268,133 bytes / 1,572,864 bytes). No new warnings.

### File List

- `firmware/modes/DeparturesBoardMode.h` (MODIFIED — added diff state members, updated renderRow signature, MAX_SUPPORTED_ROWS constant)
- `firmware/modes/DeparturesBoardMode.cpp` (MODIFIED — diff-based render, zero-alloc renderRow, identHash with telemetry hashing, improved teardown, buffer overflow protections)
- `firmware/test/test_mode_registry/test_main.cpp` (MODIFIED — 6 new dl-2.2 tests)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (MODIFIED — status update)
- `_bmad-output/implementation-artifacts/stories/dl-2-2-dynamic-row-add-and-remove-on-flight-changes.md` (MODIFIED — completion, code review synthesis)

## Change Log

- **2026-04-16:** Completed code review synthesis round 2 — 2 additional issues fixed (union type-punning replaced with memcpy for C++ standards compliance, rowHeight zero guard). All critical/high issues from both review rounds now addressed. Story marked done.
- **2026-04-16:** Completed code review synthesis — all 4 critical/high issues addressed (identHash telemetry, bounds checks, snprintf safety, const correctness). Story marked done.
- **2026-04-14:** Implemented row diff / dirty flags, selective per-row fillRect draw path, zero-alloc renderRow with char[] buffers, teardown discipline, and 6 new unit tests (Story dl-2.2)

## Previous story intelligence

- **dl-2.1** establishes **static** layout and **row cap** — **dl-2.2** optimizes **update** semantics and **visual** stability.

## Git intelligence summary

Primarily **`firmware/modes/DeparturesBoardMode.{h,cpp}`** and tests.

## Project context reference

`_bmad-output/project-context.md` — **Core 0** render budget, **heap** discipline.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-16 (Round 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** Validator A: 5.3; Validator B: 6.9
- **Issues Found:** 2 verified issues (from 12 total claims across both validators)
- **Issues Fixed:** 2 (union type-punning → memcpy, rowHeight zero guard)
- **Action Items Created:** 0

### Review: 2026-04-16 (Round 1)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** Validator A: 2.6 → APPROVED; Validator B: 14.9 → REJECT
- **Issues Found:** 4 verified critical/high issues
- **Issues Fixed:** 4 (all critical and high severity issues addressed)
- **Action Items Created:** 0


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic dl-2 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-2-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #3 violation - Invalid NVS writes**: ConfigManager.setModeSetting() lacked validation for depbd/rows range (1-4). Story AC #1 requires "clamped 1-4 in setModeSetting path" but clamping only occurred at read time. | Added range validation (1-4) to setModeSetting(), rejecting invalid values before NVS write. |
| critical | Zero-heap allocation violation**: identHash() created String temporaries every frame, violating dl-2.2 AC #5 requirement. Lines 68-69 constructed String objects via move constructor. | Refactored to use raw char* access via c_str(), eliminating all String allocations in hot path. |
| high | Divide-by-zero risk**: render() calculated rowHeight = mh / rowCount without checking if mh or rowCount is zero. Invalid hardware config could cause device crash. | Added guard returning early when rowCount == 0 or mh == 0. |
| medium | Rule 18 isolation violation**: DeparturesBoardMode::render() calls ConfigManager::getModeSetting() directly (line 94), violating architecture rule that modes should only access data through RenderContext. | Deferred - requires RenderContext refactor with config change notification system. Added technical debt comment for dl-5.x orchestrator refactor. This is necessary for hot-reload support until proper notification exists. |
| low | Array initialization clarity**: _lastIds array used `{}` initializer which may be unclear. | Changed to explicit `{0, 0, 0, 0}` for clarity. |

## Story dl-2-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| dismissed | ConfigManager::getModeSetting() called inside render() violates Rule 18 (line 107) | FALSE POSITIVE: Code at lines 102-103 explicitly documents Rule 18 compliance. The `_maxRows` member variable is used, not ConfigManager. Line 107 is not in render() method context. This was fixed in the previous review cycle per Dev Agent Record entry "Rule 18 compliance (2026-04-16)". |
| dismissed | Division by zero risk in render() band calculation when mh or rowCount is zero (line 115) | FALSE POSITIVE: Explicit guard exists at lines 116-119: `if (rowCount == 0 \|\| mh == 0) { return; }`. This was added in the previous review cycle per Dev Agent Record entry "Divide-by-zero safety". |
| dismissed | Missing explicit array initialization for _lastIds in header | FALSE POSITIVE: DeparturesBoardMode.h line 53 shows explicit initialization: `uint32_t _lastIds[MAX_SUPPORTED_ROWS] = {0, 0, 0, 0};`. This was corrected in the previous review cycle per Dev Agent Record entry "Explicit initialization". |
| dismissed | Potential redundant identHash() calculation - computed every frame for every row even if not dirty | FALSE POSITIVE: This is the correct implementation for the diff algorithm. Lines 133-137 compute hashes for all visible rows, then lines 140-150 compare against `_lastIds` to determine which rows are dirty. Computing hashes only for dirty rows would create a circular dependency — you need the hash to determine if a row is dirty. The hash function itself is zero-heap (lines 64-83) and uses the fast djb2 algorithm, so per-frame computation is acceptable. |
| dismissed | Lying test (test_departures_board_mode_render_with_null_matrix_no_crash) doesn't verify rendering logic | FALSE POSITIVE: This is a null-safety test, not a visual correctness test. Its purpose is to verify the code doesn't crash with invalid input (nullptr matrix), which is a valid defensive programming test. Visual correctness is verified through other tests and manual device validation per Dev Agent Record "manual device check on small matrix". |

## Story dl-2-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Hash collision causes stale telemetry display**: identHash() only hashed flight identifier (callsign), NOT altitude/speed. When UAL123's altitude changed from 35.0kft to 40.0kft, hash remained identical, row wasn't marked dirty, and stale data persisted on display. This violates AC #3 (same-size swap should refresh rows) and AC #1 (dynamic updates). | Extended hash to include altitude_kft and speed_mph using type-punning via union (lines 85-91). Zero-heap implementation maintains AC #5 compliance. |
| high | Buffer overflow risk in snprintf usage**: Lines 203, 213 used `pos += snprintf(...)` without checking if snprintf truncated. snprintf returns the number of characters that *would* have been written (not actual bytes written), so pos could exceed ROW_BUF_SIZE on truncation, causing buffer overrun on subsequent operations. | Added return value validation - capture written count, check `written > 0 && written < (ROW_BUF_SIZE - pos)`, clamp to buffer end on truncation (lines 217-223, 235-241). |
| high | Unsafe array access for curIds**: Line 135-136 looped to `rowCount` without verifying `rowCount <= MAX_SUPPORTED_ROWS`. If config manipulation or corruption caused rowCount > 4, would write beyond curIds[4] array bounds. | Added hashCount clamp: `uint8_t hashCount = (rowCount <= MAX_SUPPORTED_ROWS) ? rowCount : MAX_SUPPORTED_ROWS;` before hash loop (lines 147-150). |
| medium | Missing const correctness**: maxCols variable (line 166 old numbering) calculated but never mutated, should be const for correctness. | Added const qualifier to maxCols declaration (line 192). |
| dismissed | HeapMonitor::assertNoLeak() is a stub that always passes, providing false confidence about memory safety | FALSE POSITIVE: FALSE POSITIVE. HeapMonitor has real implementation in firmware/test/fixtures/test_helpers.h lines 68-101. assertNoLeak() (lines 82-87) captures ESP.getFreeHeap() at construction, computes delta, and fails test if leak exceeds tolerance via TEST_ASSERT_TRUE_MESSAGE. This is production-quality heap validation, not a stub. |
| dismissed | Tests reference wrong AC numbers (AC #7 should be AC #1) | FALSE POSITIVE: DOCUMENTATION ISSUE, NOT CODE DEFECT. Test comments on lines 751, 775 say "AC #7 (dl-2.2)" when they should reference AC #1. However, the test *implementation* is correct - it verifies row count transitions (empty↔one, one↔zero) per AC #1 requirements. This is a comment accuracy issue, not a functional bug. Deferred as low-priority cleanup. |
| dismissed | Tests don't verify dirty state or frame count changes, only check no crash | FALSE POSITIVE: MISCHARACTERIZATION. These are smoke tests validating integration stability (mode transitions don't crash, buffer overruns don't occur). DeparturesBoardMode's diff state (_firstFrame, _lastCount, _lastIds) is private implementation detail, not public API contract. Tests correctly verify observable behavior (render completes without crash) rather than internal state machine mechanics. Visual correctness was validated via manual device testing per Dev Agent Record. |
| dismissed | Incomplete teardown - resets _maxRows to 4 instead of cached config value, breaking Rule 18 | FALSE POSITIVE: INCORRECT INTERPRETATION. teardown() resets to default (4) to ensure clean state between mode lifecycles. _maxRows is repopulated from ConfigManager in init() (lines 45-47), so next mode activation reads fresh config. This is correct teardown discipline per AC #6 - release non-static resources, not "preserve config cache across teardown/init boundary." |
| dismissed | Repeated dimension calculations waste cycles | FALSE POSITIVE: PREMATURE OPTIMIZATION. matrixWidth/Height fetched 3 times per render() call (lines 106-109, 237-240, 247-248) - this is ~3 reads of uint16_t values, negligible cost (<10 cycles). Code prioritizes readability (dimensions fetched where needed) over micro-optimization. No AC requires dimension caching. Story NFR4/NFR13 target frame budget (~50ms), which this implementation meets. |
| dismissed | Full clear logic doesn't detect _maxRows config changes | FALSE POSITIVE: BY DESIGN. Code comment (lines 122-123) explicitly states "_maxRows is stable during mode lifetime (set in init)" - config changes require mode teardown/init cycle, not hot-reload mid-render. This is documented architectural decision, not a bug. Validator suggests adding check `(rowCount != _maxRows)` but this conflates two concerns: rowCount is flights.size(), _maxRows is config cap - they're independent and comparison is meaningless. |
| dismissed | memcpy size mismatch uses sizeof(_lastIds) instead of rowCount * sizeof(uint32_t) | FALSE POSITIVE: INTENTIONAL DESIGN. Line 155 copies full array (16 bytes) regardless of rowCount. Lines 157-159 then zero out slots beyond rowCount. This is explicit two-phase update: copy all hashes, then clear unused slots. Alternative (partial memcpy) is not "more correct," just different implementation choice. Current approach is clear and safe. |
| dismissed | Dependency Inversion violation - DeparturesBoardMode calls ConfigManager directly in init() | FALSE POSITIVE: ARCHITECTURAL PREFERENCE, NOT DEFECT. Validator suggests passing config through RenderContext to avoid singleton dependency. However, project_context.md Rule 18 explicitly permits ConfigManager reads in init() - "modes should only access data through RenderContext" applies to render() hot path, not initialization. This pattern is consistent across all modes in the codebase. Refactoring to full DI would require RenderContext expansion and mode contract changes - appropriate for dedicated architecture story, not bug fix. |
| dismissed | Unused include <cmath> only for isnan() with no comment | FALSE POSITIVE: STANDARD PRACTICE. <cmath> provides isnan() used in lines 199, 209. Include is necessary and purpose is obvious from usage. No comment required - this is idiomatic C++ (include what you use). |
| dismissed | Missing const on FlightInfo& f parameter | FALSE POSITIVE: ALREADY CORRECT. Function signature (line 188) uses `const FlightInfo& f` - parameter IS const-qualified. This is a hallucination by Validator B. |
| dismissed | Magic number ROW_BUF_SIZE=40 without documented calculation | FALSE POSITIVE: DOCUMENTED IN CONTEXT. Lines 38-40 show calculation: "Worst case: callsign(8) + ' ' + alt(10) + ' ' + spd(8) + '\0' = ~28" → 40 provides safety margin. This is adequate documentation for buffer sizing. |

## Story dl-2-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Union type-punning violates C++ standard (undefined behavior)**: Validator A and B both flagged identHash() using union for float-to-uint32_t conversion (lines 87-91). While commonly accepted in embedded systems, this violates strict aliasing rules and is undefined behavior per C++ standard. | Replaced union with memcpy for standards-compliant type conversion (maintains zero-heap footprint) |
| medium | Missing rowHeight == 0 guard**: Integer division `rowHeight = mh / rowCount` could theoretically produce zero on extreme matrix dimensions. While existing code guards against `mh == 0` and `rowCount == 0`, the result could still be 0 for very small matrices (e.g., 3px tall with 4 rows). | Added explicit guard `if (rowHeight == 0) return;` after division |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-2-2-dynamic-row-add-and-remove-on-flight-changes.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **3** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Dependency Inversion: DeparturesBoardMode directly reads from ConfigManager in init(). While permitted by existing project rules, it is a violation of clean architectural abstraction and forces mode-specific logic into ConfigManager. | `DeparturesBoardMode.cpp:44` | +1 |
| 🔴 CRITICAL | Potential integer overflow in `pos += written` inside `renderRow()` if `written` is negative or extremely large, despite `snprintf` clamping, due to `snprintf` return type. | `DeparturesBoardMode.cpp:218, 237` | +3 |
| 🟠 IMPORTANT | Potential heap fragmentation risk due to frequent teardown/init cycles of modes if `getSettingsSchema()` logic or other heap-dependent mode mechanisms grow over time. | `DeparturesBoardMode.h:35` | +1 |

### Evidence Score: 5.0

| Score | Verdict |
|-------|---------|
| **5.0** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[6/10] Dependency Inversion:** The mode reads configuration directly from `ConfigManager` instead of receiving it through `RenderContext`.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:44`
  - 💡 Fix: Pass mode-specific configuration through `RenderContext` or a dedicated `InitContext` struct that is populated by the orchestrator.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `snprintf` return type handling. `snprintf` returns `int`, which can be negative on error. The current check `written > 0` doesn't account for potential negative return values, leading to undefined behavior if `snprintf` fails.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:217, 236`
  - 🔄 Reproduction: Simulate a failed `snprintf` call (e.g., extremely low buffer size).

- **🐛 Bug:** `MAX_SUPPORTED_ROWS` constant is 4, but `_lastIds` array and loop bounds rely on this. While guarded, the logic is brittle. If a future developer increases `MAX_SUPPORTED_ROWS` to 5 but fails to update `_lastIds` array size, it will cause silent stack/heap corruption.
  - 📍 `firmware/modes/DeparturesBoardMode.h:38`
  - 🔄 Reproduction: Update `MAX_SUPPORTED_ROWS` without matching changes.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Lack of formal contract for mode settings defaults. While `ModeSettingDef` provides `defaultValue`, the logic to populate these defaults when NVS keys are missing is split between `ConfigManager` and mode implementation, leading to potential drift.

---

## 🛠️ Suggested Fixes

### 1. Robust snprintf usage

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Unsafe return value handling from `snprintf`.

```cpp
int written = snprintf(buf + pos, ROW_BUF_SIZE - pos, "%.1fkft", f.altitude_kft);
if (written > 0 && written < (ROW_BUF_SIZE - pos)) {
    pos += written;
} else if (written >= (ROW_BUF_SIZE - pos)) {
    pos = ROW_BUF_SIZE - 1;
} else {
    // Handle error (e.g., log or skip)
}
```

---

## Review Actions Summary

- Issues Found: 3
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-2-2-dynamic-row-add-and-remove-on-flight-changes.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 0 |
| SOLID Violations | 2 |
| Hidden Bugs | 3 |
| Performance Footguns | 2 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **10** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | String::length() in render() hot path violates AC #5 - may allocate memory per frame | DeparturesBoardMode.cpp:71,73,205,206 | +3 |
| 🔴 CRITICAL | Hash collision risk - XOR-based hash can produce identical values for different flights, causing stale data display | DeparturesBoardMode.cpp:85-91 | +3 |
| 🟠 IMPORTANT | Lying tests - dl-2.2 tests only check for no crashes, don't verify correct dirty detection behavior | test_mode_registry/test_main.cpp:750-925 | +1 |
| 🟠 IMPORTANT | Missing field-by-field comparison - hash-based dirty detection can miss changes when collisions occur | DeparturesBoardMode.cpp:158 | +1 |
| 🟠 IMPORTANT | Single Responsibility Violation - render() does too much: empty handling, dimensions, hashing, rendering, diff state management | DeparturesBoardMode.cpp:96-178 | +1 |
| 🟠 IMPORTANT | Test doesn't verify actual row updates - test_departures_board_same_size_swap_idents only ensures no crash | test_mode_registry/test_main.cpp:836-869 | +1 |
| 🟡 MINOR | Buffer size calculation error in comment - actual max could exceed 40 chars | DeparturesBoardMode.cpp:39 | +0.3 |
| 🟡 MINOR | Magic number 7 in comment without definition | DeparturesBoardMode.cpp:35 | +0.3 |
| 🟡 MINOR | Redundant dimension calculations - mw/mh fetched 3 times per render() call | DeparturesBoardMode.cpp:117-120,270-273 | +0.3 |
| 🟡 MINOR | Silent return on null matrix - no logging of initialization failure | DeparturesBoardMode.cpp:98 | +0.3 |

| 🟢 CLEAN PASS | 0 |

### Evidence Score: 11.2

| Score | Verdict |
|-------|---------|
| **11.2** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[7/10] Single Responsibility:** render() method violates SRP by handling empty state, dimension calculations, hash computation, row rendering, and diff state management all in one 82-line function.
  - 📍 `DeparturesBoardMode.cpp:96-178`
  - 💡 Fix: Extract empty handling to separate method, extract dimension logic to helper, extract hash computation loop to separate function

- **[6/10] Open/Closed:** Hash-based dirty detection is hard-coded into render() - extending with new dirty detection strategies requires modifying render().
  - 📍 `DeparturesBoardMode.cpp:158`
  - 💡 Fix: Abstract dirty detection into virtual `isRowDirty()` method that can be overridden

---

## 🐍 Pythonic Crimes &amp; Readability

- **Code Organization:** Magic numbers scattered throughout (CHAR_WIDTH=6, CHAR_HEIGHT=8, ROW_BUF_SIZE=40) without clear justification.
  - 📍 `DeparturesBoardMode.cpp:35-40`

- **Documentation:** Comments claim "no String allocation" but String::length() is called on lines 71, 73, 205, 206 - misleading documentation.
  - 📍 `DeparturesBoardMode.cpp:9-11`

{{#if no_style_issues}}
✅ Code follows style guidelines and is readable.
{{/if}}

---

## ⚡ Performance &amp; Scalability

- **[HIGH] String::length() in hot path:** Calling String::length() on FlightInfo::String members in render() violates AC #5 ("no new/malloc per frame"). String::length() may trigger allocation if the string is not in small-string optimization.
  - 📍 `DeparturesBoardMode.cpp:71,73,205,206`
  - 💡 Fix: Use strlen() on c_str() directly, or cache lengths in FlightInfo if available. Replace `f.ident_icao.length()` with `strlen(f.ident_icao.c_str())`.

- **[MEDIUM] Redundant dimension calculations:** Matrix width/height fetched 3 times per render() call (lines 117-120, 270-273, 272-273 in renderEmpty). Minor but unnecessary overhead.
  - 📍 `DeparturesBoardMode.cpp:117-120,270-273`
  - 💡 Fix: Cache dimensions in local variables at start of render() and reuse.

{{#if no_performance_issues}}
✅ No significant performance issues detected.
{{/if}}

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug:** Hash collision risk in identHash() - XOR operations can cancel out information. Two different flights with different telemetry could produce identical hash values, causing stale data to persist on display.
  - 📍 `DeparturesBoardMode.cpp:85-91`
  - 🔄 Reproduction: Create two flights with same callsign but different altitude/speed. If altitude XOR speed produces same result as previous frame, row won't be marked dirty. Example: Flight1 (alt=35.0, spd=500.0) and Flight2 (alt=40.0, spd=495.0) could produce same hash if bit patterns align unfavorably.

- **🐛 Bug:** Missing field-by-field comparison - Hash-based dirty detection is insufficient. When hash collision occurs (non-zero probability), changes to altitude/speed won't be detected.
  - 📍 `DeparturesBoardMode.cpp:158`
  - 🔄 Reproduction: Same as above - hash collision causes missed updates.

- **🐛 Bug:** Silent failure on null matrix - render() returns immediately without logging when ctx.matrix is nullptr, making initialization failures hard to debug.
  - 📍 `DeparturesBoardMode.cpp:98`
  - 🔄 Reproduction: Initialize DeparturesBoardMode with RenderContext where matrix=nullptr. render() silently does nothing.

- **🔒 [HIGH] Memory Allocation Violation:** String::length() calls in hot path may trigger heap allocations, violating AC #5 and potentially causing memory fragmentation in long-running embedded system.
  - 📍 `DeparturesBoardMode.cpp:71,73,205,206`
  - ⚠️ Impact: Could cause heap fragmentation over time, eventually leading to allocation failures or crashes.

- **🎭 Lying Test:** test_departures_board_empty_to_one_row only ensures no crash, doesn't verify row actually appears or dirty detection works.
  - 📍 `test_mode_registry/test_main.cpp:750-772`
  - 🤥 Why it lies: Test calls render() twice and passes TEST_PASS() without asserting any observable behavior change. Cannot detect if row actually appeared or if dirty detection failed.

- **🎭 Lying Test:** test_departures_board_same_size_swap_idents only checks for no crash, doesn't verify rows were actually repainted.
  - 📍 `test_mode_registry/test_main.cpp:836-869`
  - 🤥 Why it lies: Test renders two different flight sets but never asserts that the display changed. Hash collision would cause test to pass even though rows weren't updated.

{{#if no_correctness_issues}}
✅ Code appears correct and secure.
{{/if}}

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Misleading documentation - comments claim "no String allocation" but code uses String::length() which can allocate. Future maintainers may not catch this violation.
  - 📍 `DeparturesBoardMode.cpp:9-11`
  - 💥 Explosion radius: Medium - leads to AC violations and memory issues in production.

- **💣 Tech Debt:** Magic numbers without clear derivation - ROW_BUF_SIZE=40 has calculation in comments but actual maximum could be larger for edge cases.
  - 📍 `DeparturesBoardMode.cpp:39-40`
  - 💥 Explosion radius: Low - buffer may be slightly undersized for extreme cases.

- **💣 Tech Debt:** Hash collision vulnerability not addressed - code review history shows this was a critical issue in dl-2.1 but "fix" only added more data to hash, didn't address fundamental collision problem.
  - 📍 `DeparturesBoardMode.cpp:85-91`
  - 💥 Explosion radius: High - stale data display in production, user-visible bugs.

{{#if no_maintainability_issues}}
✅ Code is maintainable and well-documented.
{{/if}}

---

## 🛠️ Suggested Fixes

### 1. Remove String::length() from hot path

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** String::length() may allocate memory, violating AC #5

**Diff:**
```diff
     const char* s = nullptr;
-    if (f.ident_icao.length() && f.ident_icao.c_str()[0] != '\0') {
+    if (f.ident_icao.c_str()[0] != '\0') {
         s = f.ident_icao.c_str();
-    } else if (f.ident.length() && f.ident.c_str()[0] != '\0') {
+    } else if (f.ident.c_str()[0] != '\0') {
         s = f.ident.c_str();
```

```diff
     // Pick callsign: prefer ident_icao, fall back to ident (dl-2.1 AC #2)
     const char* cs = nullptr;
-    if (f.ident_icao.length()) cs = f.ident_icao.c_str();
-    else if (f.ident.length()) cs = f.ident.c_str();
+    if (f.ident_icao.c_str()[0] != '\0') cs = f.ident_icao.c_str();
+    else if (f.ident.c_str()[0] != '\0') cs = f.ident.c_str();
     else cs = "---";
```

### 2. Add field-by-field comparison fallback

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Hash collisions can cause missed updates

**Diff:**
```diff
     // Render rows — only dirty ones unless fullClear
     for (uint8_t i = 0; i < rowCount; i++) {
-        bool dirty = fullClear || (curIds[i] != _lastIds[i]);
+        bool dirty = fullClear || (curIds[i] != _lastIds[i]);
+        
+        // Hash collision fallback: field-by-field comparison
+        if (!dirty && i < flights.size()) {
+            dirty = (flights[i].ident_icao != _lastIcaos[i] || 
+                    flights[i].altitude_kft != _lastAlts[i] ||
+                    flights[i].speed_mph != _lastSpds[i]);
+        }
+        
         if (dirty) {
```

### 3. Improve test assertions

**File:** `firmware/test/test_mode_registry/test_main.cpp`
**Issue:** Tests don't verify actual behavior

**Diff:**
```diff
     std::vector<FlightInfo> empty;
     mode.render(ctx, empty);  // should not crash — renders "NO FLIGHTS"
 
     // Now add one flight and render again
     FlightInfo f;
     f.ident_icao = "UAL123";
     f.altitude_kft = 35.0;
     f.speed_mph = 520.0;
     std::vector<FlightInfo> flights = {f};
     mode.render(ctx, flights);  // should transition cleanly
+    
+    // Verify diff state updated - row should be marked as changed
+    // (This requires making _lastIds accessible for testing or adding inspection method)
 
     mode.teardown();
     TEST_PASS();
```

---

## Review Actions Summary

- Issues Found: 10
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

🔥 **CRITICAL BLOCKER:** Story dl-2.2 CANNOT be marked complete. The hash collision vulnerability and String::length() memory allocation violation must be fixed before this can proceed to production. These are not minor issues - they directly violate acceptance criteria and will cause user-visible bugs.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-16</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">dl-2</var>
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
<var name="session_id">5521a444-f9ce-4573-849e-0963719d80b4</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="2f7a53f7">embedded in prompt, file id: 2f7a53f7</var>
<var name="story_id">dl-2.2</var>
<var name="story_key">dl-2-2-dynamic-row-add-and-remove-on-flight-changes</var>
<var name="story_num">2</var>
<var name="story_title">2-dynamic-row-add-and-remove-on-flight-changes</var>
<var name="template">False</var>
<var name="timestamp">20260416_1011</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-2.2)
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