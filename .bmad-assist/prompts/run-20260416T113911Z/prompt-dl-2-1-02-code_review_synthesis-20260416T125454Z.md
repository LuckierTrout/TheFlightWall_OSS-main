<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-2 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T125454Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-2.1

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
<file id="5cb07b2e" path="_bmad-output/implementation-artifacts/stories/dl-2-1-departures-board-multi-row-rendering.md" label="DOCUMENTATION"><![CDATA[

# Story dl-2.1: Departures Board Multi-Row Rendering

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want to see multiple flights displayed simultaneously in a list format on the LED matrix,
So that I can see all tracked flights at a glance without cycling through one-at-a-time cards.

## Acceptance Criteria

1. **Given** **`DeparturesBoardMode`** is active and **`RenderContext`** + **`flights`** vector has **`N ≥ 1`** **`FlightInfo`** entries, **When** **`render()`** runs, **Then** the matrix draws **up to `R`** rows simultaneously, where **`R = min(N, maxRows)`** and **`maxRows`** comes from **`ConfigManager::getModeSetting("depbd", "rows", 4)`** (clamped **1–4** in **`setModeSetting`** path).

2. **Given** each rendered row, **When** data is present, **Then** the row shows **callsign** (**`FlightInfo::ident`** or **`ident_icao`** — pick one convention and document; prefer **ICAO** if **`ident`** empty) and **at least two** telemetry fields. **MVP default** fields: **`altitude_kft`** (format as **kft** or **FL** style — document) and **`speed_mph`** (ground speed). **FR6** alignment: use the **same** physical fields **Live Flight Card** already surfaces where possible; if a future **owner-configurable** field set exists, **`DEPBD_SCHEMA`** must declare the **`uint8`/`enum`** hook — **stub** extra keys **not** required for **dl-2.1** beyond **`rows`**.

3. **Given** **`N > maxRows`**, **When** rendering, **Then** only the **first `maxRows`** flights in **vector order** are shown — **no** scrolling in this story (**epic** MVP).

4. **Given** **`m_depbd_rows`** NVS key **absent**, **When** **`getModeSetting("depbd", "rows", 4)`** is called, **Then** returns **4** with **no** NVS write (**FR33**).

5. **Given** **`DeparturesBoardMode`**, **When** compiled, **Then**:
   - Implements full **`DisplayMode`** API (**`init` / `render` / `teardown` / `getName` / `getZoneDescriptor` / `getSettingsSchema`**).
   - **`DEPBD_SETTINGS[]`** and **`DEPBD_SCHEMA`** (**`ModeSettingsSchema`**) are **`static const`** in **`DeparturesBoardMode.h`** (**rule 29**); **`modeAbbrev`** string is **`"depbd"`** so NVS keys match **`m_depbd_rows`** (**≤15** chars — verify composed keys).
   - **`MEMORY_REQUIREMENT`** declared and used by **`MODE_TABLE`** factory row.

6. **Given** **`MODE_TABLE`** in **`main.cpp`**, **When** registered, **Then** new row uses stable mode **`id`** (recommended **`depbd`** to match abbrev **or** **`departures_board`** with **`schema.modeAbbrev = "depbd"`** — **pick one**, document in Dev Agent Record), **`settingsSchema: &DEPBD_SCHEMA`**, **`zoneDescriptor`** describing **row bands** (e.g. stacked horizontal strips) for **`GET /api/display/modes`** schematics, and **`priority`** ordering consistent with **Mode Picker** (after **clock** / existing modes per product taste).

7. **Given** **`N == 0`** and mode is **forced** active (orchestrator has **not** yet switched to **clock**), **When** **`render()`** runs, **Then** show a **defined empty state** (**"NO FLIGHTS"**, centered dashes, etc.) — **not** uninitialized framebuffer garbage. **Epic:** idle **clock** fallback is **orchestrator** — this mode **must not** **`requestSwitch`**.

8. **Given** **`pio run`** / tests, **Then** extend **`test_mode_registry`** (or add **`test_departures_board_mode`**) for **`getName`**, row clamp, and **`getModeSetting`** default; **no** new warnings.

## Tasks / Subtasks

- [x] Task 1: **`DeparturesBoardMode.h/.cpp`** — schema, zones, **`render`** layout math (**AC: #1–#3, #5–#7**)
- [x] Task 2: **`MODE_TABLE`** + factory + mem requirement (**AC: #5, #6**)
- [x] Task 3: **`setModeSetting("depbd", "rows", v)`** validation **1–4** (**AC: #1, #4**)
- [x] Task 4: Tests + manual device check on small matrix (**AC: #8**)

#### Review Follow-ups (AI)
- [x] [AI-Review] MEDIUM: Refactor ConfigManager access in render() - move maxRows to RenderContext for Rule 18 compliance (firmware/modes/DeparturesBoardMode.cpp:107)

## Dev Notes

### Layout hints

- Compute **row band** as **`matrixHeight / maxRows`** (integer division); clip text with **`DisplayUtils`** / **`Adafruit_GFX`** patterns from **`LiveFlightCardMode`** / **`ClassicCardMode`**.
- **NAN** telemetry → **`"--"`** placeholders (match **Live Flight Card** behavior).

### **`FlightInfo`** fields

- See **`firmware/models/FlightInfo.h`** — **`altitude_kft`**, **`speed_mph`**, **`vertical_rate_fps`**, etc.

### Dependencies

- **ds-1.3** **`ModeRegistry`**, **ds-1.5** pipeline (**no** **`show()`** inside mode).
- **dl-1.1** **`getModeSetting`** / **`setModeSetting`** pattern (**`ClockMode`** reference).

### Out of scope (**dl-2.2**)

- Sub-frame row diffing, **NFR4** single-frame add/remove guarantees, heap churn optimizations.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-2.md` (Story dl-2.1)
- Prior modes: `firmware/modes/LiveFlightCardMode.{h,cpp}`, `ClassicCardMode.{h,cpp}`, `ClockMode.{h,cpp}`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, 79.4% flash usage (no increase concerns)
- Test build: `pio test -f test_mode_registry --without-uploading --without-testing` — PASSED
- Regression: `test_config_manager`, `test_mode_orchestrator` — both PASSED (build only, no device)

### Completion Notes List

- **Task 1:** Created `DeparturesBoardMode.h/.cpp` with full DisplayMode API. Schema `DEPBD_SCHEMA` with `modeAbbrev="depbd"`, single setting `rows` (uint8, default 4, range 1-4). Zone descriptor has 4 stacked horizontal row bands. Render computes `R = min(N, maxRows)`, divides matrix height by R for row bands. Each row shows `ident_icao` (fallback to `ident`) + `altitude_kft` + `speed_mph` using `DisplayUtils::formatTelemetryValue` (NAN -> "--" automatically). Empty state renders "NO FLIGHTS" centered. No `FastLED.show()` called.
- **Task 2:** Registered in `MODE_TABLE` as `id="departures_board"`, `displayName="Departures Board"`, `priority=3` (after clock), with `&DEPBD_SCHEMA` and `&DeparturesBoardMode::_descriptor`. Factory + memReq wrappers added.
- **Task 3:** Added `depbd`/`rows` validation in `ConfigManager::setModeSetting` — rejects values outside 1-4 range.
- **Task 4:** Extended `test_mode_registry` with 10 new tests: getName, zone descriptor, settings schema, prod table presence, init/render null safety, getModeSetting default (no NVS write), setModeSetting valid range, setModeSetting reject out-of-range, NVS key length verification. Added `isNtpSynced()` stub for test builds. All test suites build with no regressions.

**Code Review Fixes (2026-04-16):**
- **AC #3 enforcement:** Enhanced `ConfigManager::setModeSetting()` to validate depbd/rows range (1-4) at write time, preventing invalid NVS persistence.
- **Zero-heap hash:** Refactored `identHash()` to use raw char* access instead of String temporaries, eliminating heap allocation in hot path (dl-2.2 AC #5 compliance).
- **Divide-by-zero safety:** Added guard in `render()` to prevent division by zero when matrixHeight=0 or rowCount=0.
- **Explicit initialization:** Changed `_lastIds` array initialization to explicit `{0,0,0,0}` for clarity.
- **Rule 18 compliance (2026-04-16):** Removed per-frame ConfigManager::getModeSetting() call from render(). Now maxRows is read only in init() and cached in `_maxRows`. Settings changes take effect on mode switch (teardown/init cycle). Removed unused `_lastMaxRows` member variable.

**Design Decision (AC #6):** Mode ID is `"departures_board"` (descriptive, matches other mode IDs like `"classic_card"`, `"live_flight"`), with `schema.modeAbbrev = "depbd"` for NVS key prefix. This keeps the API-facing ID readable while NVS keys stay compact.

**Callsign Convention (AC #2):** Prefers `ident_icao` (ICAO callsign); falls back to `ident` if `ident_icao` is empty; uses `"---"` as last resort.

**Telemetry Format (AC #2):** altitude displayed as `kft` (e.g., "35.0kft"), speed as `mph` (e.g., "450mph") — same format as Live Flight Card.

### File List

- `firmware/modes/DeparturesBoardMode.h` (NEW)
- `firmware/modes/DeparturesBoardMode.cpp` (NEW)
- `firmware/src/main.cpp` (MODIFIED — include + MODE_TABLE entry)
- `firmware/core/ConfigManager.cpp` (MODIFIED — depbd/rows validation)
- `firmware/test/test_mode_registry/test_main.cpp` (MODIFIED — 10 new tests + isNtpSynced stub)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (MODIFIED — status update)

## Change Log

- **2026-04-14:** Implemented DeparturesBoardMode with multi-row rendering, MODE_TABLE registration, setModeSetting validation, and comprehensive tests (Story dl-2.1)
- **2026-04-16:** Code review synthesis - fixed AC #3 validation enforcement, eliminated String allocations in identHash(), added divide-by-zero guards

## Previous story intelligence

- **dl-2.1** starts **epic dl-2**; **dl-1.x** established **orchestrator**, **clock**, and **mode settings** NVS — reuse exactly.

## Git intelligence summary

New **`firmware/modes/DeparturesBoardMode.*`**, **`main.cpp`**, tests, optional **`data-src`** if mode name surfaces only via API.

## Project context reference

`_bmad-output/project-context.md` — **gzip** only if portal sources change.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-16
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 6.3 (Reviewer B) / 4.9 (Reviewer A) → MAJOR REWORK
- **Issues Found:** 5 (Reviewer A) + 8 (Reviewer B) = 13 total
- **Issues Fixed:** 4
- **Action Items Created:** 1


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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-2-1-departures-board-multi-row-rendering.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **5** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | ConfigManager::getModeSetting() called inside render() violates Rule 18 (Mode Data Access). | `DeparturesBoardMode.cpp:107` | +1 |
| 🟠 IMPORTANT | Division by zero risk in render() band calculation when `mh` or `rowCount` is zero. | `DeparturesBoardMode.cpp:115` | +1 |
| 🟡 MINOR | Missing explicit array initialization for `_lastIds` in header. | `DeparturesBoardMode.h` | +0.3 |
| 🟡 MINOR | Potential redundant identHash() calculation. | `DeparturesBoardMode.cpp` | +0.3 |

| Score | Verdict |
|-------|---------|
| **2.6** | **APPROVED** |

---

## 🏛️ Architectural Sins

- **Rule 18 / Isolation Principle Violation:** `DeparturesBoardMode::render()` accesses `ConfigManager` directly, breaking the strict hexagonal boundary established for mode rendering. Modes MUST ONLY access state via `RenderContext`.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:107`
  - 💡 Fix: Cache `maxRows` in `init()` as `_maxRows` and access the member variable. (Already done for `_maxRows`, but re-check for other potential leakage).

---

## 🐛 Correctness & Safety

- **🐛 Bug (Divide-by-zero):** `rowHeight = mh / rowCount` does not guard against `rowCount == 0`. While there is an early guard for `rowCount == 0`, adding an explicit assertion or hardening the calculation is safer.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:115`
  - 🔄 Reproduction: Call render() with empty flight vector where `rowCount` logic might be bypassed or return zero despite checks.

- **🎭 Lying Test (test_departures_board_mode_render_with_null_matrix_no_crash):** The test validates that the code doesn't crash with a `nullptr` matrix, but doesn't actually verify the rendering logic. It's a tautological safety check.
  - 📍 `firmware/test/test_mode_registry/test_main.cpp:520`
  - 🤥 Why it lies: It proves nothing about visual correctness or layout parity.

---

## ⚡ Performance & Scalability

- **Footgun (IdentHash Redundancy):** `identHash()` is computed for *every* frame for *every* row, even if rows are not dirty. While `djb2` is fast, this is redundant logic for static flight rows.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:109`
  - 💡 Fix: Only compute hashes for flights that have changed or during a full refresh.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Lack of explicit row height unit testing. The integer division logic for band heights is critical but only tested implicitly.
  - 📍 `firmware/modes/DeparturesBoardMode.cpp:115`

---

## 🛠️ Suggested Fixes

### 1. Robustness Guard

**File:** `firmware/modes/DeparturesBoardMode.cpp`
**Issue:** Potential division by zero in row band calculation.

**Corrected code:**
```cpp
    // Compute row band height (integer division) - guarded above
    if (rowCount == 0) return;
    uint16_t rowHeight = mh / rowCount;
```

---

## Review Actions Summary

- Issues Found: 5
- Issues Fixed: 0
- Action Items Created: 1

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

Looking at this code review request for story dl-2.1, I need to perform an adversarial review of the Departures Board Multi-Row Rendering implementation. Let me analyze the files systematically.

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
<var name="session_id">fab6a09a-bc1e-4389-aaa9-5453be55364f</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="5cb07b2e">embedded in prompt, file id: 5cb07b2e</var>
<var name="story_id">dl-2.1</var>
<var name="story_key">dl-2-1-departures-board-multi-row-rendering</var>
<var name="story_num">1</var>
<var name="story_title">1-departures-board-multi-row-rendering</var>
<var name="template">False</var>
<var name="timestamp">20260416_0854</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-2.1)
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