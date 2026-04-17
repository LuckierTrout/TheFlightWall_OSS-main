<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-2 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T135838Z -->
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

### File List

- `firmware/modes/DeparturesBoardMode.h` (MODIFIED — added diff state members, updated renderRow signature, MAX_SUPPORTED_ROWS constant)
- `firmware/modes/DeparturesBoardMode.cpp` (MODIFIED — diff-based render, zero-alloc renderRow, identHash with telemetry hashing, improved teardown, buffer overflow protections)
- `firmware/test/test_mode_registry/test_main.cpp` (MODIFIED — 6 new dl-2.2 tests)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (MODIFIED — status update)
- `_bmad-output/implementation-artifacts/stories/dl-2-2-dynamic-row-add-and-remove-on-flight-changes.md` (MODIFIED — completion, code review synthesis)

## Change Log

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

### Review: 2026-04-16
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
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **4** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Dependency on ConfigManager in `render()` hot path (Rule 18 violation). | `firmware/modes/DeparturesBoardMode.cpp:46` | +1.0 |
| 🔴 CRITICAL | Potential buffer overflow in `renderRow()` despite clamp logic (snprintf return check is insufficient if pos already at capacity). | `firmware/modes/DeparturesBoardMode.cpp:217` | +3.0 |
| 🟠 IMPORTANT | Use of static const array in `_zones` inside class header/source - missing explicit size or PROGMEM attribute for flash optimization. | `firmware/modes/DeparturesBoardMode.cpp:16` | +1.0 |
| 🟡 MINOR | Inconsistent test naming - `test_departures_board_empty_to_one_row` tests empty state handling but AC #1 specifically refers to row transitions. | `firmware/test/test_mode_registry/test_main.cpp:525` | +0.3 |

### Evidence Score: 5.3

| Score | Verdict |
|-------|---------|
| **5.3** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[6/10] Dependency Inversion:** `DeparturesBoardMode::render` directly accesses `ConfigManager` via `getModeSetting`, violating Rule 18 and creating tight coupling between the mode and global config singleton, preventing pure unit testing of rendering logic without config state.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:46`
  - 💡 Fix: Inject configuration dependencies via `RenderContext` at `init()` time.

---

## ⚡ Performance & Scalability

- **[High] Unnecessary configuration access in render path:** While the developer noted this as "deferred technical debt" in the antipattern log, reading config in `render()` adds unnecessary overhead to the hot path (Core 0), even if cached by ConfigManager.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:46`
  - 💡 Fix: Cache all configuration requirements in member variables during `init()` and use those cached values during `render()`.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `renderRow()` snprintf safety logic relies on `snprintf` return value; while corrected for overflow, it does not account for `snprintf` potentially returning a negative value on encoding error, which could be cast to `size_t` (large positive) leading to undefined behavior or further buffer corruption.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:217`
  - 🔄 Reproduction: Provide an invalid input that causes `snprintf` to return an encoding error (negative).
- **🐛 Bug:** `curIds` hash array initialization: `uint32_t curIds[MAX_SUPPORTED_ROWS] = {};` is a C++ feature, but depending on the compiler version/flag, it might not zero-init all elements correctly on all ESP32 toolchains; `memset` is safer.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:146`
  - 🔄 Reproduction: Inspect memory after init on a clean boot.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `_zones` array in `DeparturesBoardMode.cpp` is missing `PROGMEM` attribute. Given the constrained 4MB flash (2MB application + 2MB LittleFS), all static data of this type should be explicitly moved to flash to save RAM.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:16`
  - 💥 Explosion radius: Flash pressure will increase unnecessarily as mode count grows.

---

## 🛠️ Suggested Fixes

### 1. Robust snprintf usage

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** `snprintf` return value check is unsafe against encoding errors (negative values).

**Corrected code:**
```cpp
int written = snprintf(buf + pos, ROW_BUF_SIZE - pos, "%.1fkft", f.altitude_kft);
if (written > 0 && written < (ROW_BUF_SIZE - pos)) {
    pos += written;
} else {
    // Truncation or error occurred - clamp safely
    pos = ROW_BUF_SIZE - 1;
}
```

---

## Review Actions Summary

- Issues Found: 4
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
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 1 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 2 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Missing _lastMaxRows member in header - story completion notes claim it exists but it's not in the code | Story line 72 vs DeparturesBoardMode.h | +3 |
| 🔴 CRITICAL | Task 1 completion claim false - story claims _lastMaxRows was added but header shows no such member | Story line 31 vs DeparturesBoardMode.h | +3 |
| 🟠 IMPORTANT | Missing rowHeight > 0 check - integer division could produce zero height | DeparturesBoardMode.cpp:142 | +1 |
| 🟠 IMPORTANT | Tests don't verify dirty state logic - only check no crash, not actual diff behavior | test_main.cpp:750-925 | +1 |
| 🟠 IMPORTANT | Type-punning via union is undefined behavior per C++ standard | DeparturesBoardMode.cpp:87-91 | +1 |
| 🟡 MINOR | Inefficient memcpy - copies full 16 bytes instead of rowCount * 4 | DeparturesBoardMode.cpp:168 | +0.3 |
| 🟡 MINOR | Unnecessary full-clear on rowCount change - AC #4 only requires full clear on maxRows change | DeparturesBoardMode.cpp:135 | +0.3 |
| 🟡 MINOR | Potential buffer truncation edge case - exactly ROW_BUF_SIZE-2 length callsign | DeparturesBoardMode.cpp:207,210 | +0.3 |
| 🟢 CLEAN PASS | SOLID Principles | 0 | -0.5 |
| 🟢 CLEAN PASS | Security Vulnerabilities | 0 | -0.5 |
| 🟢 CLEAN PASS | Style Violations | 0 | -0.5 |
| 🟢 CLEAN PASS | Type Safety Issues | 0 | -0.5 |
| 🟢 CLEAN PASS | Abstraction Level Issues | 0 | -0.5 |

### Evidence Score: 6.9

| Score | Verdict |
|-------|---------|
| **6.9** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes &amp; Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance &amp; Scalability

- **[Medium] Unnecessary memory copy:** Full memcpy of _lastIds array (16 bytes) instead of copying only rowCount * 4 bytes, then manually zeroing unused slots.
  - 📍 `DeparturesBoardMode.cpp:168-172`
  - 💡 Fix: Use `memcpy(_lastIds, curIds, rowCount * sizeof(uint32_t))` directly

- **[Medium] Unnecessary full-screen clear:** AC #4 only requires full clear when maxRows changes (config reload), but implementation also clears on rowCount change (line 135), wasting GPU cycles.
  - 📍 `DeparturesBoardMode.cpp:135`
  - 💡 Fix: Remove `(rowCount != _lastCount)` condition, add separate `_lastMaxRows` tracking

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug: Missing _lastMaxRows prevents config change detection** - Story completion notes claim "_lastMaxRows (detect config changes)" was added, but this variable doesn't exist in header. AC #4 requires full clear on maxRows config reload, but code only checks rowCount changes.
  - 📍 `DeparturesBoardMode.h:48` vs Story line 72
  - 🔄 Reproduction: Change maxRows config while mode is active - previous rendering artifacts won't be cleared, causing visual corruption.

- **🐛 Bug: Zero rowHeight can cause rendering issues** - After computing `rowHeight = mh / rowCount`, there's no check if result is 0. If matrix is 10px tall with rowCount=4, rowHeight=2 (fine), but if matrix is extremely small or rowCount is large, could be 0.
  - 📍 `DeparturesBoardMode.cpp:142-143`
  - 🔄 Reproduction: Very small matrix dimensions (e.g., 10x4) with multiple flights causes zero-height rows, potential division by zero or no rendering.

- **🔒 [Medium] Undefined behavior: Type-punning via union** - Lines 87-91 use union for float-to-uint32_t conversion to include telemetry in hash. This violates strict aliasing rules (UB per C++ standard), though commonly accepted in embedded systems.
  - 📍 `DeparturesBoardMode.cpp:87-91`
  - ⚠️ Impact: Compiler optimization could break hash computation. Use `memcpy(&altBits.u, &f.altitude_kft, sizeof(float))` for standard-compliant type-punning.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Tests are smoke tests, not behavioral tests** - All 6 dl-2.2 tests only verify render() doesn't crash, not that the diff state logic works correctly. No verification that dirty rows are repainted and non-dirty rows are left untouched.
  - 📍 `test_main.cpp:750-925`
  - 💥 Explosion radius: High - core feature (row diff) is untested, could silently fail.

- **💣 Tech Debt: Potential buffer truncation at boundary** - Line 207: if callsign is exactly ROW_BUF_SIZE-2 characters, line 210 writes space to buf[ROW_BUF_SIZE-1], leaving only 1 byte for null terminator. Subsequent snprintf calls could truncate altitude/speed.
  - 📍 `DeparturesBoardMode.cpp:207,210`
  - 💥 Explosion radius: Medium - could cause truncated altitude/speed display in edge cases.

---

## 🛠️ Suggested Fixes

### 1. Add Missing _lastMaxRows Member

**File:** `firmware/modes/DeparturesBoardMode.h`
**Issue:** Story completion notes claim _lastMaxRows exists but it's missing from header, breaking AC #4 config change detection.

**Diff:**
```diff
 private:
     uint8_t _maxRows = 4;

     // --- Diff state for selective row repaint (dl-2.2) ---
     bool     _firstFrame = true;         // Full clear on first frame after init/mode switch
     uint8_t  _lastCount  = 0;            // Row count drawn on previous frame
+    uint8_t  _lastMaxRows = 4;          // Previous maxRows config value
     uint32_t _lastIds[MAX_SUPPORTED_ROWS] = {0, 0, 0, 0};  // Compact ident hash per row slot
```

```diff
     // Determine if full clear needed (dl-2.2 AC #4 exceptions)
-    bool fullClear = _firstFrame || (rowCount != _lastCount);
+    bool fullClear = _firstFrame || (rowCount != _lastCount) || (_maxRows != _lastMaxRows);
```

```diff
     // Update diff state for next frame
     _firstFrame = false;
     _lastCount = rowCount;
+    _lastMaxRows = _maxRows;
     memcpy(_lastIds, curIds, sizeof(_lastIds));
```

```diff
     // D2 teardown discipline: release all non-static resources (dl-2.2 AC #6)
     _maxRows = 4;
+    _lastMaxRows = 4;
     _firstFrame = true;
     _lastCount = 0;
     memset(_lastIds, 0, sizeof(_lastIds));
```

### 2. Add rowHeight > 0 Check

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Missing check for zero rowHeight after integer division.

**Diff:**
```diff
     // Compute row band height (integer division) - guarded above
     uint16_t rowHeight = mh / rowCount;
+    if (rowHeight == 0) {
+        // Cannot render anything - rows have no height
+        return;
+    }
```

### 3. Use Standard-Compliant Type-Punning

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Union type-punning is undefined behavior.

**Diff:**
```diff
     // Include telemetry in hash to detect altitude/speed changes
     // Use type-punning via union to get bit representation of floats
-    union { float f; uint32_t u; } altBits, spdBits;
-    altBits.f = f.altitude_kft;
-    spdBits.f = f.speed_mph;
-    h = ((h << 5) + h) ^ altBits.u;
-    h = ((h << 5) + h) ^ spdBits.u;
+    uint32_t altBits, spdBits;
+    memcpy(&altBits, &f.altitude_kft, sizeof(float));
+    memcpy(&spdBits, &f.speed_mph, sizeof(float));
+    h = ((h << 5) + h) ^ altBits;
+    h = ((h << 5) + h) ^ spdBits;
```

### 4. Optimize memcpy Size

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Unnecessary full-array copy when rowCount < 4.

**Diff:**
```diff
     // Update diff state for next frame
     _firstFrame = false;
     _lastCount = rowCount;
-    memcpy(_lastIds, curIds, sizeof(_lastIds));
-    // Zero out slots beyond current rowCount
-    for (uint8_t i = rowCount; i < MAX_SUPPORTED_ROWS; i++) {
-        _lastIds[i] = 0;
-    }
+    // Copy only the active range, then clear unused slots
+    memcpy(_lastIds, curIds, rowCount * sizeof(uint32_t));
+    for (uint8_t i = rowCount; i < MAX_SUPPORTED_ROWS; i++) {
+        _lastIds[i] = 0;
+    }
```

---

## Review Actions Summary

- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding. The missing _lastMaxRows member is a **CRITICAL** gap between story claims and actual implementation that violates AC #4 requirements.

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
<var name="session_id">0cbcd255-70ac-4519-b116-e6b0bfa5873d</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="2f7a53f7">embedded in prompt, file id: 2f7a53f7</var>
<var name="story_id">dl-2.2</var>
<var name="story_key">dl-2-2-dynamic-row-add-and-remove-on-flight-changes</var>
<var name="story_num">2</var>
<var name="story_title">2-dynamic-row-add-and-remove-on-flight-changes</var>
<var name="template">False</var>
<var name="timestamp">20260416_0958</var>
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