<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-2 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T134115Z -->
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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** dl-2.2 - dynamic-row-add-and-remove-on-flight-changes

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **4** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | **Incomplete cleanup in `teardown()`**: The `_lastIds` array is `memset` to 0, but the `_lastCount` and `_firstFrame` are also reset, which is correct, but there is no explicit check if `_lastIds` was previously allocated or dynamic resources were cleaned up properly. While `_lastIds` is static sized, it is good practice to explicitly reset. | `DeparturesBoardMode.cpp:52-57` | +1.0 |
| 🟠 IMPORTANT | **Potential `snprintf` buffer overflow**: The `ROW_BUF_SIZE` is 40, but the buffer arithmetic (`ROW_BUF_SIZE - pos`) is correct. However, if `pos` somehow exceeds 40 due to logic error, `snprintf` will still write to a negative offset or overflow. | `DeparturesBoardMode.cpp:188-197` | +1.0 |
| 🟡 MINOR | **Missing `const` on local `maxCols`**: `maxCols` is calculated but not used in a way that requires mutation. Should be `const`. | `DeparturesBoardMode.cpp:166` | +0.3 |
| 🟡 MINOR | **Hardcoded MAGIC number for rowHeight**: The `rowHeight` logic assumes simple division without considering remainder pixels for different screen heights. | `DeparturesBoardMode.cpp:125` | +0.3 |

### Evidence Score: 2.6

| Score | Verdict |
|-------|---------|
| **2.6** | **APPROVED** |

---

## 🏛️ Architectural Sins

- **[6/10] Dependency Inversion**: `DeparturesBoardMode` relies on `ConfigManager` directly in `init()` for settings retrieval. While `init()` is acceptable, the mode should ideally receive its configuration through the `RenderContext` or an injected dependency to fully decouple it from the global `ConfigManager` singleton.
  - 📍 `DeparturesBoardMode.cpp:46`
  - 💡 Fix: Pass mode-specific settings through the `RenderContext` during `init()` or a separate configuration method.

---

## ⚡ Performance &amp; Scalability

✅ No significant performance issues detected. The zero-alloc per-frame requirement is well-met.

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug:** Potential for invalid memory access if `maxRows` is manipulated incorrectly via `ConfigManager` outside of `init()`.
  - 📍 `DeparturesBoardMode.cpp:48`
  - 🔄 Reproduction: Call `ConfigManager::setModeSetting` to change rows while the mode is active. The mode will not react until a full teardown/init cycle occurs, leading to potential inconsistency between configuration and display.

- **🐛 Bug:** `snprintf` return value usage. `snprintf` returns the number of characters that *would* have been written. The code uses `pos += snprintf(...)`. If `snprintf` truncates, `pos` will be larger than the actual buffer size, leading to buffer overflow on subsequent `memcpy` or `pos` usage.
  - 📍 `DeparturesBoardMode.cpp:191`
  - 🔄 Reproduction: Force a string longer than 40 chars into the row text buffer.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Magic number `40` for `ROW_BUF_SIZE`.
  - 📍 `DeparturesBoardMode.h:26`
  - 💥 Explosion radius: If row format changes to include more info, this buffer size might be insufficient, leading to silent truncation or buffer overflow if logic isn't updated.

---

## 🛠️ Suggested Fixes

### 1. Fix `snprintf` return value handling

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** `snprintf` can return a value larger than the buffer size if truncation occurs. Using `pos += snprintf(...)` directly is dangerous.

**Corrected code:**
```cpp
    int written = snprintf(buf + pos, ROW_BUF_SIZE - pos, "%.1fkft", f.altitude_kft);
    if (written > 0 && written < (ROW_BUF_SIZE - pos)) {
        pos += written;
    } else {
        // Handle truncation or error
        pos = ROW_BUF_SIZE - 1; 
    }
```

---

## Review Actions Summary

- Issues Found: 4
- Issues Fixed: 0
- Action Items Created: 0

✅ Code is approved and ready for deployment!

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-2-2-dynamic-row-add-and-remove-on-flight-changes.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 4 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 2 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **15** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Hash collision bug - different flights with same callsign produce same hash, causing stale telemetry display | `DeparturesBoardMode.cpp:65-83` | +3 |
| 🔴 CRITICAL | Test validation failure - HeapMonitor::assertNoLeak() is a stub that always passes, giving false confidence | `test_mode_registry/test_main.cpp:922` | +3 |
| 🔴 CRITICAL | AC documentation false positive - Test comments reference wrong AC #7 (should be AC #1), confusing test coverage | `test_mode_registry/test_main.cpp:751, 775` | +3 |
| 🟠 IMPORTANT | Task completion lie - Task 1 claims "row diff / dirty flags" complete but only tracks identity, NOT telemetry changes (altitude/speed) | `DeparturesBoardMode.cpp:133-137` | +1 |
| 🟠 IMPORTANT | Full clear logic bug - Doesn't detect _maxRows config changes (comment claims stable but it's NOT enforced) | `DeparturesBoardMode.cpp:124, 122-123` | +1 |
| 🟠 IMPORTANT | Unsafe array access - curIds[4] but loop to rowCount without bounds check | `DeparturesBoardMode.cpp:134, 157-159` | +1 |
| 🟠 IMPORTANT | Hash collision race condition - Two different flights can have same djb2 hash, treated as identical | `DeparturesBoardMode.cpp:65-83` | +1 |
| 🟡 MINOR | Unsafe string fallback - Ternary defaults to "---" literal without null check on f.ident | `DeparturesBoardMode.cpp:69-74` | +0.3 |
| 🟡 MINOR | Buffer overflow risk - snprintf with ROW_BUF_SIZE(40) but multiple concatenations without per-call validation | `DeparturesBoardMode.cpp:182-216` | +0.3 |
| 🟡 MINOR | Incomplete reset in teardown - Resets _maxRows to 4 instead of cached config value, breaking Rule 18 | `DeparturesBoardMode.cpp:56-62` | +0.3 |
| 🟡 MINOR | Repeated dimension calculations - matrixWidth/Height computed in render() but not cached despite being frame-stable | `DeparturesBoardMode.cpp:106-109, 237-240, 247-248` | +0.3 |
| 🟡 MINOR | AC #1 measurement method not implemented - Story requires documented measurement, but only explanation provided | Story AC #1, Completion Notes | +0.3 |
| 🟡 MINOR | memcpy size mismatch - Uses sizeof(_lastIds) instead of rowCount * sizeof(uint32_t), brittle if MAX_SUPPORTED_ROWS changes | `DeparturesBoardMode.cpp:155` | +0.3 |
| 🟡 MINOR | Unused include - #include <cmath> only for isnan(), no comment explaining dependency | `DeparturesBoardMode.cpp:17` | +0.3 |
| 🟡 MINOR | Missing const correctness - FlightInfo& f should be explicitly const throughout | `DeparturesBoardMode.cpp:65` | +0.3 |
| 🟢 CLEAN PASS | Type Safety - No significant type safety violations found | | -0.5 |
| 🟢 CLEAN PASS | Security - No security vulnerabilities found | | -0.5 |
| 🟢 CLEAN PASS | Style - Code follows C++ style guidelines | | -0.5 |
| 🟢 CLEAN PASS | Tech Debt - No major technical debt bombs | | -0.5 |
| 🟢 CLEAN PASS | SOLID - Minimal SOLID violations | | -0.5 |
| 🟢 CLEAN PASS | Abstraction - Appropriate abstraction level | | -0.5 |

### Evidence Score: 14.9

| Score | Verdict |
|-------|---------|
| **14.9** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[9/10] Single Responsibility Violation:** `render()` method does too much - calculates hashes, determines dirty rows, renders text, AND manages diff state. Should extract dirty determination to separate method.
  - 📍 `DeparturesBoardMode.cpp:85-161`
  - 💡 Fix: Extract `bool determineDirtyRows(const std::vector<FlightInfo>& flights, uint32_t* curIds)` to separate concerns

- **[Design Inconsistency]:** Comment claims _maxRows is stable during mode lifetime but doesn't enforce it - config changes require mode switch but no validation prevents external modification.
  - 📍 `DeparturesBoardMode.cpp:122-123`
  - 💡 Fix: Make _maxRows const after init(), or validate in render() against cached value and trigger mode switch if changed

{{#if no_architectural_sins}}
✅ No significant architectural violations detected.
{{/if}}

---

## 🐍 Pythonic Crimes &amp; Readability

{{#each style_violations}}
- **{{violation_type}}:** {{description}}
  - 📍 `{{file}}:{{line}}`
{{/each}}

{{#each type_safety_issues}}
- **Type Safety:** {{description}}
  - 📍 `{{file}}:{{line}}`
{{/each}}

- **Unsafe Array Access:** `curIds` array size (4) vs `rowCount` loop variable without explicit bounds check.
  - 📍 `DeparturesBoardMode.cpp:134, 157-159`
- **Unsafe String Fallback:** Ternary defaults to "---" literal without validating f.ident is not null.
  - 📍 `DeparturesBoardMode.cpp:69-74`
- **Missing Const Correctness:** FlightInfo& f should be explicitly const throughout identHash and renderRow.
  - 📍 `DeparturesBoardMode.cpp:65, 175`
- **Unused Include:** `<cmath>` included only for isnan() with no explanatory comment.
  - 📍 `DeparturesBoardMode.cpp:17`

{{#if no_style_issues}}
✅ Code follows style guidelines and is readable.
{{/if}}

---

## ⚡ Performance &amp; Scalability

- **[High Impact] Unnecessary Hash Computation:** `identHash()` called for EVERY row on EVERY frame, even when flight data hasn't changed. Wastes CPU cycles on Core 0.
  - 📍 `DeparturesBoardMode.cpp:133-137`
  - 💡 Fix: Only recompute hashes when `rowCount != _lastCount`, otherwise trust cached _lastIds

- **[Medium Impact] Repeated Dimension Calculations:** `matrixWidth/Height` fetched from RenderContext 3 times per render call (lines 106-109, 237-240, 247-248), plus fallback to matrix->width()/height().
  - 📍 `DeparturesBoardMode.cpp:106-109, 237-240`
  - 💡 Fix: Cache matrix dimensions at top of render() after null check

- **[Low Impact] memcpy Overhead:** Full `memcpy(_lastIds, curIds, sizeof(_lastIds))` copies 16 bytes even when only 1 row changed.
  - 📍 `DeparturesBoardMode.cpp:155`
  - 💡 Fix: Use `rowCount * sizeof(uint32_t)` to only copy active rows

{{#if no_performance_issues}}
✅ No significant performance issues detected.
{{/if}}

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug:** Hash collision causes stale telemetry display - two different flights with same ident_icao produce identical hash, preventing repaint when altitude/speed changes.
  - 📍 `DeparturesBoardMode.cpp:65-83`
  - 🔄 Reproduction: Frame 1: UAL123 alt 35.0 spd 500 → hash=0xABC123; Frame 2: UAL123 alt 40.0 spd 520 → hash=0xABC123 (SAME); Row NOT repainted, stale alt/speed displayed!

- **🐛 Bug:** Config change not detected - Comment claims "_maxRows is stable during mode lifetime" but no enforcement prevents config changes requiring full clear.
  - 📍 `DeparturesBoardMode.cpp:122-123`
  - 🔄 Reproduction: User changes rows from 2 to 4 via dashboard, init() not called again, render() still uses old _maxRows, layout mismatch persists.

- **🎭 Lying Test:** `test_departures_board_no_heap_leak_across_renders`
  - 📍 `test_mode_registry/test_main.cpp:896-925`
  - 🤥 Why it lies: Uses `HeapMonitor::assertNoLeak(256)` which is a stub that always returns true (no actual heap tracking implementation). Test provides false confidence about memory safety.

- **🎭 Lying Test:** `test_departures_board_empty_to_one_row` and `test_departures_board_one_to_zero_rows`
  - 📍 `test_mode_registry/test_main.cpp:750-795`
  - 🤥 Why it lies: Only verify no crash (TEST_PASS()), no assertions about visual correctness, dirty state, or frame count changes. Tests don't verify the AC requirements were actually met.

- **🎭 Lying Test:** `test_departures_board_cap_overflow_stable`
  - 📍 `test_mode_registry/test_main.cpp:797-834`
  - 🤥 Why it lies: Comments claim "No crash, no buffer overrun" but there's no validation - only loop rendering 5 times and TEST_PASS(). No assertion that exactly M rows were rendered.

- **🎭 Lying Test:** `test_departures_board_same_size_swap_idents`
  - 📍 `test_mode_registry/test_main.cpp:836-869`
  - 🤥 Why it lies: Comment says "Both rows should be dirty and repainted" but no dirty flag verification, no assertion that repaint occurred, just TEST_PASS().

{{#if no_correctness_issues}}
✅ Code appears correct and secure.
{{/if}}

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Hardcoded `MAX_SUPPORTED_ROWS = 4` constant used in multiple places - changing requires edits in header, hash loop bounds, and zero-out loop.
  - 📍 `DeparturesBoardMode.h:36`, `DeparturesBoardMode.cpp:53, 134, 157`
  - 💥 Explosion radius: Medium - requires coordinated changes across 4 locations

- **💣 Tech Debt:** Magic number `ROW_BUF_SIZE = 40` without documented calculation or safety margin.
  - 📍 `DeparturesBoardMode.cpp:40`
  - 💥 Explosion radius: Low - single location but unclear why 40 vs actual max needs

- **💣 Tech Debt:** Incomplete teardown - resets _maxRows to 4 instead of preserving cached config value, violates Rule 18 "use cached _maxRows from init()".
  - 📍 `DeparturesBoardMode.cpp:58`
  - 💥 Explosion radius: Medium - breaks hot-reload consistency, requires init() to be called again

{{#if no_maintainability_issues}}
✅ Code is maintainable and well-documented.
{{/if}}

---

## 🛠️ Suggested Fixes

### 1. Fix Hash Collision Bug (CRITICAL)

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Hash collision causes stale telemetry display - only hashes ident, not altitude/speed

**Corrected code:**
```cpp
uint32_t DeparturesBoardMode::identHash(const FlightInfo& f) {
    uint32_t h = 5381;
    const char* s = nullptr;
    if (f.ident_icao.length() && f.ident_icao.c_str()[0] != '\0') {
        s = f.ident_icao.c_str();
    } else if (f.ident.length() && f.ident.c_str()[0] != '\0') {
        s = f.ident.c_str();
    } else {
        s = "---";
    }

    if (s != nullptr) {
        for (unsigned int i = 0; s[i] != '\0'; i++) {
            h = ((h << 5) + h) + (uint8_t)s[i];
        }
    }
    
    // FIX: Include altitude and speed in hash to detect telemetry changes
    uint32_t altBits = *(uint32_t*)&f.altitude_kft;
    uint32_t spdBits = *(uint32_t*)&f.speed_mph;
    h = ((h << 5) + h) ^ altBits;
    h = ((h << 5) + h) ^ spdBits;
    
    return h;
}
```

### 2. Implement Real Heap Monitoring (CRITICAL)

**File:** `firmware/test/test_mode_registry/test_main.cpp`
**Issue:** HeapMonitor::assertNoLeak() is a stub that always passes

**Corrected code:**
```cpp
// In test_helpers.h, implement actual heap tracking:
class HeapMonitor {
    size_t _startHeap;
public:
    HeapMonitor() : _startHeap(ESP.getFreeHeap()) {}
    void assertNoLeak(size_t tolerance) {
        size_t endHeap = ESP.getFreeHeap();
        size_t leaked = _startHeap - endHeap;
        if (leaked > tolerance) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Heap leak: %u bytes (tolerance: %u)", leaked, tolerance);
            TEST_FAIL_MESSAGE(msg);
        }
    }
};
```

### 3. Fix Full Clear Logic (IMPORTANT)

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Doesn't detect _maxRows config changes

**Diff:**
```diff
     // Determine if full clear needed (dl-2.2 AC #4 exceptions)
-    // Note: _maxRows is stable during mode lifetime (set in init), so no maxRows
-    // change check needed — changes require mode teardown/init cycle.
-    bool fullClear = _firstFrame || (rowCount != _lastCount);
+    // Full clear on: first frame, row count change, OR maxRows config change
+    bool fullClear = _firstFrame || (rowCount != _lastCount) || (rowCount != _maxRows);
```

### 4. Add Bounds Check for Array Access (IMPORTANT)

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Unsafe array access when rowCount > MAX_SUPPORTED_ROWS

**Diff:**
```diff
     // Compute current ident hashes
     uint32_t curIds[MAX_SUPPORTED_ROWS] = {};
+    uint8_t hashCount = (rowCount < MAX_SUPPORTED_ROWS) ? rowCount : MAX_SUPPORTED_ROWS;
-    for (uint8_t i = 0; i < rowCount; i++) {
+    for (uint8_t i = 0; i < hashCount; i++) {
         curIds[i] = identHash(flights[i]);
     }
```

### 5. Implement Real Test Assertions (HIGH)

**File:** `firmware/test/test_mode_registry/test_main.cpp`
**Issue:** Tests only check no crash, don't verify AC requirements

**Corrected code:**
```cpp
void test_departures_board_empty_to_one_row() {
    clearModeNvs();
    ConfigManager::init();
    DeparturesBoardMode mode;
    RenderContext ctx = makeTestCtx();
    mode.init(ctx);

    std::vector<FlightInfo> empty;
    mode.render(ctx, empty);

    FlightInfo f;
    f.ident_icao = "UAL123";
    f.altitude_kft = 35.0;
    f.speed_mph = 520.0;
    std::vector<FlightInfo> flights = {f};
    mode.render(ctx, flights);

    // FIX: Verify row count changed and dirty state updated
    TEST_ASSERT_FALSE(mode._firstFrame);
    TEST_ASSERT_EQUAL(1, mode._lastCount);

    mode.teardown();
}
```

### 6. Optimize Hash Computation (MEDIUM)

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Hashes recomputed every frame even when unchanged

**Diff:**
```diff
     // Compute current ident hashes
     uint32_t curIds[MAX_SUPPORTED_ROWS] = {};
-    for (uint8_t i = 0; i < rowCount; i++) {
-        curIds[i] = identHash(flights[i]);
+    // Only recompute hashes if row count changed, otherwise use cached
+    if (rowCount != _lastCount) {
+        for (uint8_t i = 0; i < rowCount; i++) {
+            curIds[i] = identHash(flights[i]);
+        }
+    } else {
+        // Hashes are same as _lastIds when count hasn't changed
+        memcpy(curIds, _lastIds, rowCount * sizeof(uint32_t));
     }
```

### 7. Add AC #1 Measurement Method (MINOR)

**File:** Story documentation
**Issue:** AC #1 requires documented measurement method, but only explanation provided

**Fix:** Add timing code to DeparturesBoardMode::render():
```cpp
void DeparturesBoardMode::render(const RenderContext& ctx, const std::vector<FlightInfo>& flights) {
    if (ctx.matrix == nullptr) return;

#ifdef DEBUG_PERF
    unsigned long startTime = micros();
#endif

    // ... existing render code ...

#ifdef DEBUG_PERF
    unsigned long elapsed = micros() - startTime;
    if (elapsed > 50000) {  // Log if >50ms
        LOG_W("DeparturesBoard", ("Render took " + String(elapsed) + "us").c_str());
    }
#endif
```

---

## Review Actions Summary

- Issues Found: 15
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully. The hash collision bug and test validation failures are blocking issues that MUST be fixed before this story can be approved.

---

## Key Blockers

1. **Hash Collision Bug (CRITICAL):** Different flights with same ident produce identical hash, causing stale telemetry display. Violates AC #3 and creates visible data corruption.

2. **Test Validation Failure (CRITICAL):** HeapMonitor::assertNoLeak() is a stub that always passes, providing false confidence about memory safety. AC #5 cannot be verified.

3. **AC Documentation False Positive (CRITICAL):** Test comments reference wrong AC numbers, making test coverage verification impossible.

4. **Task Completion Lie (IMPORTANT):** Task 1 claims "row diff / dirty flags" complete but doesn't track telemetry changes, only identity. Partial implementation marked as complete.

**Recommendation:** REJECT story - requires significant rework to address hash collision bug, implement real test validation, and complete Task 1 properly.

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
<var name="session_id">c4e70258-231f-4ff4-a115-f373c96644d9</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="2f7a53f7">embedded in prompt, file id: 2f7a53f7</var>
<var name="story_id">dl-2.2</var>
<var name="story_key">dl-2-2-dynamic-row-add-and-remove-on-flight-changes</var>
<var name="story_num">2</var>
<var name="story_title">2-dynamic-row-add-and-remove-on-flight-changes</var>
<var name="template">False</var>
<var name="timestamp">20260416_0941</var>
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