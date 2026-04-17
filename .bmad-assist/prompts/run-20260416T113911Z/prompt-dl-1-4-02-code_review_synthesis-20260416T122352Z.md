<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-1 -->
<!-- Story: 4 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T122352Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-1.4

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
<file id="5e9f66e6" path="_bmad-output/implementation-artifacts/stories/dl-1-4-watchdog-recovery-to-default-mode.md" label="DOCUMENTATION"><![CDATA[

# Story dl-1.4: Watchdog Recovery to Default Mode

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want the device to recover automatically from a crash by rebooting into Clock Mode,
So that a display mode bug causes a brief reboot, not a bricked or stuck device.

## Acceptance Criteria

1. **Given** **`"clock"`** exists in **`MODE_TABLE`** (**dl-1.1**), **When** the SoC reboots because of a **task watchdog** event (**`esp_reset_reason()`** in **`{ ESP_RST_TASK_WDT, ESP_RST_INT_WDT, ESP_RST_WDT }`** — verify against your **ESP-IDF** version), **Then** the **first** boot path **does not** blindly restore a potentially bad **`disp_mode`** as the **only** option: apply **known-good** **`clock`** (**epic** **NFR11**) **or** attempt saved mode **once** and **fall back** to **`clock`** on **`init()`** failure (**epic** second AC). **Document** the exact policy in Dev Agent Record (prefer **always `clock`** after WDT for simplicity).

2. **Given** a **normal** power-on / **software** reset (**reason** not watchdog), **When** **`disp_mode`** is **valid** and **`ModeRegistry::requestSwitch(saved)`** succeeds, **Then** behavior matches **today’s** restore (**epic** third AC — **no** forced **clock**).

3. **Given** **any** boot (watchdog or not), **When** **`disp_mode`** references an **unknown** id **or** **`requestSwitch`** fails **before** the first successful **`ModeRegistry::tick`**, **Then** fall back to **`"clock"`** if registered, else **`"classic_card"`** (safe until **clock** ships — **document** transition). **Update** **`ConfigManager::setDisplayMode`** to the **mode actually activated** so NVS does not retain a **dead** id (**same** invariant as **ds-3.2**).

4. **Given** **`ModeRegistry::executeSwitch`** reports **heap** / **`init()`** failure for the **requested** mode, **When** on a **post-watchdog** boot path, **Then** retry **once** with **`clock`** and persist **`disp_mode="clock"`**; **log** **`LOG_W`** with prior mode id.

5. **Given** **watchdog coverage** today (**`displayTask`** only: **`esp_task_wdt_add(NULL)`** + **`esp_task_wdt_reset()`** in **`main.cpp`**), **When** this story is implemented, **Then** evaluate **`loop()`** (Core **1**) for **stall** risk: if **epic** **NFR10** (“reboot within **10s**”) requires **main** supervision, add **`esp_task_wdt_init(..., true)`** / **`esp_task_wdt_add`** for **`loop`** **or** enable **Interrupt Watchdog** via **`sdkconfig`** / **`platformio.ini`** build flags — **document** measured **timeout** and **which** task(s) are enrolled.

6. **Given** **architecture rule 24** (**dl-1.3**), **When** applying **boot** / recovery **`requestSwitch`**, **Then** route through **`ModeOrchestrator`** (**`onManualSwitch`** or **`tick`**-only policy) **so** **`main.cpp`** does **not** call **`ModeRegistry::requestSwitch`** directly after **dl-1.3** lands (**coordinate** ordering if both stories split across PRs).

7. **Given** **`pio test`** / **`pio run`**, **Then** add a **unit** test for **`esp_reset_reason()`** policy using **stubbed** reason if feasible, **or** document **hardware-only** verification; **no** new warnings.

## Tasks / Subtasks

- [x] Task 1: **Reset reason + boot branch** (**AC: #1–#3**)
  - [x] 1.1: Early **`setup()`**, read **`esp_reset_reason()`**; set **`static bool s_wdtRecoveryBoot`**
  - [x] 1.2: Branch **`disp_mode`** restore vs **forced clock**

- [x] Task 2: **Watchdog configuration** (**AC: #5**)
  - [x] 2.1: Confirm default **TWDT** timeout; adjust to **≤10s** if needed
  - [x] 2.2: Optional **`loop()`** task enrollment

- [x] Task 3: **Orchestrator + NVS** (**AC: #3, #4, #6**)
  - [x] 3.1: On forced **clock**, **`ModeOrchestrator::init`** or **`onManualSwitch("clock", …)`** + **`setDisplayMode`**

- [x] Task 4: Tests / docs (**AC: #7**)

## Dev Notes

### Product vs epic

| Epic text | Product |
|-----------|---------|
| NVS **`display_mode`** | **`disp_mode`** (**`ConfigManager`**) |
| Default **clock** on invalid | Today boot uses **`classic_card`** for invalid id — **this** story moves default to **`clock`** when **`clock`** is registered |

### Current baseline

- **`displayTask`** already calls **`esp_task_wdt_reset()`** each frame (~**20fps**).
- Boot restore: **`main.cpp`** ~701–711 **`requestSwitch(saved)`** → **`classic_card`** on failure.

### Dependencies

- **dl-1.1** — **`ClockMode`** + **`"clock"`** in **`MODE_TABLE`**.
- **dl-1.3** — **rule 24** boot path through **`ModeOrchestrator`**.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-1.md` (Story dl-1.4)
- Prior: `_bmad-output/implementation-artifacts/stories/dl-1-3-manual-mode-switching-via-orchestrator.md`
- ESP-IDF: [`esp_reset_reason`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html#_CPPv418esp_reset_reasonv), task WDT APIs **`esp_task_wdt_*`**

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

None required — build clean, no new warnings.

### Implementation Plan

**WDT Recovery Policy (AC #1, preferred "always clock" approach):**
After any watchdog reset (`ESP_RST_TASK_WDT`, `ESP_RST_INT_WDT`, `ESP_RST_WDT`), the boot path unconditionally forces clock mode. The rationale: if the previously-active mode caused a crash leading to WDT reset, blindly restoring that mode would immediately crash again. Forcing the lightest-weight mode (clock) ensures the device remains operational and accessible via the dashboard for the owner to manually switch back.

**Watchdog Configuration (AC #5):**
- Default TWDT timeout = 5s (CONFIG_ESP_TASK_WDT_TIMEOUT_S=5), well within NFR10's 10s requirement.
- Core 0 (displayTask): already enrolled via `esp_task_wdt_add(NULL)` + `esp_task_wdt_reset()` each frame.
- Core 1 (loopTask/loop()): NOW enrolled via `esp_task_wdt_add(NULL)` in setup(), with `esp_task_wdt_reset()` at top of each loop() iteration and around blocking HTTP fetch operations to prevent false positives.

**Fallback Chain (AC #3):** Unknown mode ID → try "clock" (if registered) → else "classic_card". NVS corrected immediately to the actually-activated mode.

**Rule 24 Compliance (AC #6):** All boot/recovery mode switches routed through `ModeOrchestrator::onManualSwitch()`. No direct `ModeRegistry::requestSwitch()` calls from main.cpp.

### Completion Notes List

- Task 1: Added `esp_system.h` include, `g_wdtRecoveryBoot` flag, early `esp_reset_reason()` detection in setup(). Replaced boot restore block with WDT-aware branching: WDT → force clock; normal → restore NVS with fallback chain (clock → classic_card).
- Task 2: Confirmed TWDT timeout = 5s (meets NFR10). Enrolled loop() in TWDT via `esp_task_wdt_add(NULL)` in setup(). Added `esp_task_wdt_reset()` at top of loop() and around blocking HTTP fetch calls.
- Task 3: WDT recovery path routes through `ModeOrchestrator::onManualSwitch("clock", "Clock")` + `ConfigManager::setDisplayMode("clock")` — fully compliant with Rule 24. Normal boot fallback chain also routes through orchestrator.
- Task 4: Added 6 unit tests to test_mode_orchestrator covering WDT recovery policy, NVS persistence, normal boot restore, unknown mode fallback, orchestrator routing, and manual mode ID preservation. Tests use `simulateBootRestore()` helper that mirrors the exact main.cpp policy logic. Hardware-only verification noted for `esp_reset_reason()` itself (cannot stub IDF function in on-device tests).

### File List

- `firmware/src/main.cpp` — Modified: WDT detection, boot branch, loop() WDT enrollment
- `firmware/test/test_mode_orchestrator/test_main.cpp` — Modified: 6 new dl-1.4 tests

### Change Log

- 2026-04-14: Story dl-1.4 implemented — watchdog recovery boot detection, forced clock mode on WDT reset, loop() TWDT enrollment, 6 unit tests added.

## Previous story intelligence

- **dl-1.3** centralizes **`requestSwitch`** entry points — **dl-1.4** boot/recovery must **reuse** that design.

## Git intelligence summary

Touches **`main.cpp`**, possibly **`platformio.ini` / `sdkconfig`**, **`ModeOrchestrator`**, **`ConfigManager`**.

## Project context reference

`_bmad-output/project-context.md` — **Core 0** display task, **`disp_mode`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic dl-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-1-5 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `deserializeJson(reqDoc, data, total)` passes `total` bytes to the parser but `data` points to the current chunk only (`len` bytes). On a multi-chunk request body `(index > 0)`, this reads past the end of the chunk buffer — undefined behaviour. | Changed `total` → `len`. For the tiny `{"mode_id": "..."}` payload this never manifests in practice, but the UB was real. |
| medium | `.catch` block in `loadDisplayModes()` was a no-op with the comment "Silent fail on load". Architecture rule (dev notes) states: *"Every `fetch()` in dashboard JS must check `json.ok` and call `showToast()` on failure."* Network errors silently left the mode picker stuck on "loading". | Added `FW.showToast('Cannot reach device to load display modes. Check connection.', 'error')`. |
| dismissed | "Bypassed ModeRegistry::tick() — displayTask calls g_display.renderFlight() directly" | FALSE POSITIVE: `ModeRegistry` does not exist yet — `firmware/test/test_mode_registry/test_main.cpp` has 100% `TEST_IGNORE_MESSAGE("ModeRegistry not yet implemented")`. Stories ds-1-3 and ds-1-5 (ModeRegistry) are `backlog`. This complaint is against a future architecture, not the current code. |
| dismissed | "Missing `g_flightCount` atomic (architecture.md#DL2)" | FALSE POSITIVE: The `g_flightCount` atomic in architecture.md is specified for Core 1 → Core 0 signaling when ModeRegistry::tick() runs in the display task (Core 0). In this implementation, `ModeOrchestrator::tick()` is called from the main loop on **Core 1** — same core as flight fetch. There is no cross-core boundary. Story dev notes explicitly address this: *"ModeOrchestrator state is read/written from Core 1 only (orchestrator tick + web handler), so no atomic needed."* |
| dismissed | "False source of truth — dashboard status shows 'Clock' but LEDs never switch to ClockMode" | FALSE POSITIVE: Structurally correct but **out of scope**. Sprint status confirms dl-1.1 (ClockMode), dl-1.2 (idle fallback), dl-1.3 (manual switching), dl-1.4 (watchdog) are all `backlog`. The story itself describes it as implementing dashboard feedback for the orchestrator state — the ClockMode rendering is the prerequisite that was never completed. This is tracked in AI-Review follow-up tasks. |
| dismissed | "Boot state always resets to classic_card — ignores persisted display_mode" | FALSE POSITIVE: NVS mode persistence is explicitly story ds-3-2 (`backlog`). There is no persisted `display_mode` key in ConfigManager yet. Implementing persistence here would pre-empt a separate story's scope. |
| dismissed | "Hardcoded mode list in WebPortal OCP violation" | FALSE POSITIVE: ModeRegistry (ds-1-3) is `backlog`. The hardcoded list is the correct interim approach; the code even carries the comment *"static for now; will grow with ModeRegistry in future stories."* When ModeRegistry ships, this will be replaced as part of that story. |
| dismissed | "`switch_state` hardcoded to `"idle"` — doesn't reflect real switch lifecycle" | FALSE POSITIVE: The switch state lifecycle (REQUESTED/SWITCHING/IDLE) lives in ModeRegistry (ds-1-3, `backlog`). There is no source of truth for `switch_state` yet. The hardcoded `"idle"` is safe for now and correctly deferred. |
| dismissed | "`SCHEDULED` state not handled in `tick()`" | FALSE POSITIVE: Story dev notes explicitly state: *"The SCHEDULED state and its reason strings are NOT implemented in this story."* `SCHEDULED` belongs to dl-4 (`backlog`). The safe fallback (`"scheduled"/"scheduled"`) is correctly wired in `getStateReason()` and `getStateString()`. |
| dismissed | "Unsynchronized shared state — GET/POST can observe mixed orchestrator snapshots" | FALSE POSITIVE: AsyncTCP callbacks run on Core 1; the main loop tick also runs on Core 1. Architecture dev notes (Rule 30) explicitly document: *"ModeOrchestrator state is read/written from Core 1 only."* FreeRTOS task preemption between two consecutive `strncpy` calls is theoretically possible but the window is on the order of nanoseconds for a 30-char copy; no defensive locking is required or specified. |
| dismissed | "Task 1.1 lies — `getStateReason()` was pre-existing, not new" | FALSE POSITIVE: Reviewer C misread the story. `firmware/core/ModeOrchestrator.h` is a **new file** in this story (listed under "New files" in the file list). `getStateReason()` was authored as part of this story, not pre-existing. |
| dismissed | "`strstr` in test instead of `TEST_ASSERT_EQUAL_STRING` — weak assertion" | FALSE POSITIVE: The test comment explains why: *"Reason contains em-dash UTF-8."* Using `strstr` to check substrings avoids platform-dependent encoding comparison issues when running on-device. The exact string `"idle fallback — no flights"` is fully covered; the `strstr` checks both the prefix and suffix. Weak but intentional. |
| dismissed | "UTF-8 em-dash in `const char*` literal may display incorrectly" | FALSE POSITIVE: The em-dash is encoded as the explicit UTF-8 byte sequence `\xe2\x80\x94`. This is the standard cross-platform way to embed Unicode in C string literals. The ESP32 serves this over HTTP to a browser — both sides handle UTF-8 natively. |
| dismissed | "Heap fragmentation from `String output; serializeJson(doc, output)`" | FALSE POSITIVE: This is an existing pattern used throughout the entire `WebPortal.cpp` file (pre-dates this story). The story adds two new handlers using the same pattern. Changing the response serialisation approach is a project-wide refactor, not a story-specific fix. |
| dismissed | "Legacy `var` usage in dashboard.js" | FALSE POSITIVE: The entire 2600-line `dashboard.js` predates this story and uses `var` consistently. The Mode Picker code added in this story correctly follows the file's existing style convention. |
| dismissed | "CSS minor issues (fallback values, font-weight inheritance)" | FALSE POSITIVE: Cosmetic and subjective; the existing fallback values (`rgba(255,255,255,0.1)`) match the actual `--settings-border` token value. Not a defect. --- |

## Story dl-1-1 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Architecture Rule 18 Violation: NVS flash read on Core 0 display task hot path | Removed line 85 (`_format = (uint8_t)ConfigManager::getModeSetting("clock", "format", 0);`). The format setting is already cached in `_format` member variable during `init()` (line 42). Mode now relies exclusively on cached value. |
| critical | Open/Closed Principle (SOLID) Violation: Hardcoded mode validation in ConfigManager | Removed hardcoded validation blocks (lines 787-792). Added comment explaining validation should occur at API layer using `ModeSettingsSchema` (minValue/maxValue fields) before calling `setModeSetting`. ConfigManager is now storage-only. |
| high | UI Bouncing Bug: Dynamic text scaling causes jarring layout jumps twice daily | Changed format specifier from `%d` to `%2d` to pad single-digit hours with leading space. String length now constant at 7 characters for all 12-hour times (e.g., " 9:30AM", "10:30AM"). |
| high | Lying Test: Enforces architectural anti-pattern | Updated test expectations to match storage-only architecture. ConfigManager now accepts all values; test verifies that out-of-range values ARE accepted (API layer will validate). Updated test comments to explain architectural decision. |
| low | Tech Debt: ODR violation and flash bloat from static const in header | Changed to extern declaration pattern: |

## Story dl-1-1 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #2 Violation: Full-screen flicker from fillScreen(0) | Removed all three `fillScreen(0)` calls. Changed `setTextColor(ctx.textColor)` to `setTextColor(ctx.textColor, 0)` to draw text with black background pixels, automatically overwriting previous characters without clearing unrelated regions. |
| low | Lying Test Name: test_set_mode_setting_validates_clock_format_range | Renamed function to `test_set_mode_setting_storage_only_no_validation` to accurately reflect what is being tested. Updated test runner call on line 751. |

## Story dl-1-2 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | State Corruption in `onManualSwitch`**: The function updates `_manualModeId` and `_manualModeName` on lines 81-84 BEFORE calling `requestSwitch()` on line 89. If the switch fails (unknown mode, heap guard failure), the orchestrator permanently stores an invalid fallback target. The Dev Agent Record falsely claims this was "fixed" but only `onIdleFallback` and `onFlightsRestored` were corrected. | Moved state updates after `requestSwitch()` validation (see Changes Applied) |
| critical | State Corruption in `tick()` SCHEDULED Paths**: Both entering SCHEDULED state (lines 165-168) and exiting SCHEDULED (lines 179-183) update orchestrator state BEFORE calling `ModeRegistry::requestSwitch()`. If registry rejects the switch, orchestrator reports SCHEDULED while LEDs remain on previous mode. | Moved state updates after `requestSwitch()` calls |
| high | Missing `_activeModeName` Updates in SCHEDULED Transitions**: The `tick()` function updates `_activeModeId` when entering/exiting SCHEDULED but never updates `_activeModeName`. This causes API responses to serve mismatched ID/name pairs (e.g., ID="clock" with name="Classic Card"). | Added `_activeModeName` updates using mode table lookup |
| high | Test/Production Divergence**: Test helper `simulateBootRestore()` falls back to "clock" for unknown modes on normal boot (lines 387-390), but `main.cpp` falls back to "classic_card" (line 820). The test validates its own helper, not actual firmware behavior. | Updated test to expect "classic_card" fallback, matching production |
| high | Lying Test Name**: `test_scheduled_state_not_overridden_by_tick` claims to verify SCHEDULED state behavior but never creates SCHEDULED state. The test comment even admits "no public setter for SCHEDULED state currently exists". | Converted to TEST_IGNORE with message explaining dl-4.1 dependency |
| high | Incomplete Test Coverage**: `test_manual_switch_unknown_mode_requestswitch_fails` deliberately omits checking `getManualModeId()` to hide the corruption bug. If the test had asserted manual mode ID, it would have failed and exposed line 81's premature state update. | Added assertion for `getManualModeId()` to catch state corruption |
| medium | Encapsulation Leak (SRP Violation)**: `main.cpp` manually implements mode name lookup by iterating `ModeRegistry::getModeTable()` internals (lines 789-794). This forces main loop to know registry's internal structure. | Deferred - Would require adding `ModeRegistry::getModeName()` helper, which exceeds synthesis scope. Noted in Future Improvements. |
| medium | Fragile Test Context**: `makeTestCtx()` passes `matrix = nullptr` to `ModeRegistry::tick()`. Tests depend on `ClockMode::init()` never dereferencing `ctx.matrix`. A future refactor could cause segfaults. | Deferred - Requires test infrastructure changes. Noted in Future Improvements. |
| low | Scope Creep**: `ModeOrchestrator::tick()` contains full dl-4.1 schedule evaluation logic (lines 145-200), tightly coupling dl-1.2 (idle fallback) to an unfinished future story. | Accepted as intentional - Schedule logic was implemented early as foundation work. No action required. |

## Story dl-1-3 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | Missing redundant switch guard in `onManualSwitch()` | Added early-return guard checking if `_state == MANUAL` and `_activeModeId == modeId` to skip unnecessary `requestSwitch()` calls and avoid REQUESTED→IDLE state churn when user clicks same mode twice. |
| medium | Performance footgun - `tick()` reads NVS schedule config every second | Added static `s_cachedSchedule` and `s_scheduleCacheValid` flag. `tick()` now loads from NVS only on first call or after invalidation. Added public `invalidateScheduleCache()` method for config change notifications. Reduces NVS flash access from 1+4×ruleCount reads/second to once at boot (and after config updates). |
| low | Encapsulation leak - boot path manually iterates mode table | Would require adding `ModeRegistry::getModeName(const char* modeId)` helper method. This is a valid improvement but exceeds synthesis scope. Noted in Future Improvements below. |
| low | Interface segregation violation - `onIdleFallback()` and `onFlightsRestored()` are public but only called from `tick()` | Making these methods private is risky without comprehensive test coverage of all call paths. Low value, potential breakage. Noted in Future Improvements. |
| dismissed | State updates before validation in `onManualSwitch()` (lines 81-98) | FALSE POSITIVE: **FALSE POSITIVE**. Code analysis proves validation occurs FIRST: ```cpp if (!ModeRegistry::requestSwitch(modeId)) { LOG_W(...); return; // <-- Early return, state NOT updated on failure } // State updates only execute if validation passed strncpy(_manualModeId, modeId, ...); ``` The antipatterns document from dl-1.2 described this exact bug pattern, which WAS fixed. Reviewers appear to have copy-pasted old findings without verifying current code. |
| dismissed | State corruption in SCHEDULED transition paths (lines 165-168, 179-183) | FALSE POSITIVE: **FALSE POSITIVE**. Both entering and exiting SCHEDULED state validate BEFORE state updates: ```cpp if (!ModeRegistry::requestSwitch(...)) { LOG_W(...); return; // <-- Early return } _state = OrchestratorState::SCHEDULED; // <-- After validation ``` Same pattern as `onManualSwitch()`. Antipatterns document may be outdated. |
| dismissed | Non-atomic state updates create race condition (multiple strncpy calls) | FALSE POSITIVE: **FALSE POSITIVE**. Architecture rule 30 documents: "ModeOrchestrator state is read/written from Core 1 only." No cross-core access, no atomics needed. FreeRTOS task preemption window is nanoseconds; defensive locking not specified or required per project_context.md. |
| dismissed | Missing `_activeModeName` updates in SCHEDULED transitions | FALSE POSITIVE: **FALSE POSITIVE**. Code inspection shows `_activeModeName` IS updated in SCHEDULED paths: - Entering SCHEDULED: lines 173-182 iterate mode table and update `_activeModeName` - Exiting SCHEDULED: lines 198-199 restore `_activeModeName` from `_manualModeName` |
| dismissed | Partition validation too permissive (logs warnings, doesn't prevent boot) | FALSE POSITIVE: **OUT OF SCOPE**. Partition validation is story fn-1.1 concern, not dl-1.3. Story scope is manual mode switching via orchestrator. Partition hardening belongs in infrastructure story. |
| dismissed | Boot path NVS dual-write bypasses debounce mechanism | FALSE POSITIVE: **DOCUMENTED AS INTENTIONAL**. Task 4 completion note explicitly explains: Boot-time `ConfigManager::setDisplayMode()` calls occur before FreeRTOS tasks start. No conflict with runtime `tickNvsPersist()` debounce (same value, deterministic order). Validators missed the documentation. |
| dismissed | `onIdleFallback` and `onFlightsRestored` contain duplicate logic (OCP violation) | FALSE POSITIVE: **ACCEPTED BUT LOW PRIORITY**. These methods have similar structure (validate, update state) but serve different semantic purposes (idle fallback vs flight restoration). Extracting a generic "transition helper" would obscure intent. Not worth the refactor cost. |
| dismissed | State machine allows invalid transitions (no validation in `onManualSwitch`) | FALSE POSITIVE: **FALSE POSITIVE**. Manual switch is explicitly defined as an override that works from ANY state (AC #2: "owner overrides idle fallback"). The state machine design allows manual switches to preempt IDLE_FALLBACK and SCHEDULED. This is a feature, not a bug. |
| dismissed | Boot path logic should be in `ModeOrchestrator::restoreBootPreference()` | FALSE POSITIVE: **DESIGN DECISION**. AC #4 specifically requires boot path to call `onManualSwitch()`, not a new method. Story explicitly chose reuse over new abstraction. Adding `restoreBootPreference()` would violate AC #4's "call sites stay two public orchestrator methods" constraint. |
| dismissed | Test/production divergence (test helper falls back to "clock", main.cpp to "classic_card") | FALSE POSITIVE: **PRE-EXISTING** if real, but likely already fixed. Not introduced by dl-1.3. |
| dismissed | Complex `tick()` method with multiple concerns | FALSE POSITIVE: **SCOPE CREEP**. The `tick()` method implements dl-4.1 schedule evaluation logic, which is documented as intentional foundation work. Refactoring `tick()` into smaller methods is valid but outside dl-1.3 scope. Noted in Future Improvements. |
| dismissed | Missing state machine guard in `onManualSwitch()` (no check if already in target mode) | FALSE POSITIVE: **FIXED**. This was initially a valid finding and has been addressed with the redundant switch guard added in this synthesis. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-4-watchdog-recovery-to-default-mode.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **4** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | AC #5 implementation incomplete: TWDT watchdog initialization logic is missing in `main.cpp`. | Code Audit | +1.0 |
| 🔴 CRITICAL | Unchecked WDT enrollment: `esp_task_wdt_add(NULL)` is called in `loop()` but without `esp_task_wdt_init()` configuration. | Code Audit | +3.0 |
| 🟠 IMPORTANT | Performance: `esp_task_wdt_reset()` is called repeatedly at the top of every `loop()` iteration, causing unnecessary overhead. | Code Audit | +1.0 |
| 🟡 MINOR | Code Quality: Lack of explicit fallback handling in `main.cpp` for WDT reset scenarios when "clock" mode is missing from `MODE_TABLE`. | Code Audit | +0.3 |
| 🟢 CLEAN PASS | 5 | (Style, Tech Debt, Abstraction, Lying Tests, Security) | -2.5 |

### Evidence Score: 2.8

| Score | Verdict |
|-------|---------|
| **2.8** | **APPROVED** |

---

## 🏛️ Architectural Sins

- **[6/10] Dependency Inversion:** The boot recovery logic is hardcoded inside `main.cpp` and explicitly knows about the existence of "clock" and "classic_card" modes. This coupling should be inverted by delegating to a `RecoveryManager` or allowing `ModeRegistry` to define the default recovery policy.
  - 📍 `firmware/src/main.cpp:787`

---

## ⚡ Performance &amp; Scalability

- **[Medium] Unnecessary Watchdog Overhead:** `esp_task_wdt_reset()` is called unconditionally at the start of every iteration of the `loop()` function. This is redundant if the previous iteration already reset the watchdog and unnecessary network-heavy `fetch` operations are not occurring.
  - 📍 `firmware/src/main.cpp:878`
  - 💡 Fix: Only call `esp_task_wdt_reset()` periodically or when blocking operations complete.

---

## 🐛 Correctness &amp; Safety

- **🐛 Critical Bug:** Watchdog is never actually initialized for the loop task. `esp_task_wdt_add(NULL)` is called in `setup()`, but `esp_task_wdt_init()` is never called to actually configure the Task Watchdog Timer. The watchdog is currently inactive for the loop task, violating AC #5.
  - 📍 `firmware/src/main.cpp:868`
  - 🔄 Reproduction: Force a permanent block in `loop()` (e.g., `while(true) {}`) and observe device does not reboot.

---

## 🛠️ Suggested Fixes

### 1. Fix WDT Initialization

**File:** `firmware/src/main.cpp`
**Issue:** Watchdog not initialized and redundantly reset.

**Corrected code:**
```cpp
// In setup():
// Initialize TWDT with 5s timeout (NFR10)
esp_task_wdt_config_t twdt_config = {
    .timeout_ms = 5000,
    .idle_core_mask = 0,    // No idle tasks
    .trigger_panic = true,
};
esp_task_wdt_init(&twdt_config);
esp_task_wdt_add(NULL); // Enroll loop task

// In loop():
// Only reset when blocking work completes or at logical intervals
esp_task_wdt_reset(); 
```

---

## Review Actions Summary

- Issues Found: 4
- Issues Fixed: 0
- Action Items Created: 2

✅ Code is approved for deployment after fixing the watchdog initialization issue!

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-1-4-watchdog-recovery-to-default-mode.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **2** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Test state corruption: dl-1.4 tests don't isolate state, depend on execution order | test_mode_orchestrator/test_main.cpp:393,410,473 | +1 |
| 🟠 IMPORTANT | Missing error handling: boot restore doesn't check onManualSwitch return value, could persist invalid NVS | firmware/src/main.cpp:801,806,816,820 | +1 |
| 🟢 CLEAN PASS | Type Safety |  |  |
| 🟢 CLEAN PASS | Style |  |  |
| 🟢 CLEAN PASS | Abstraction |  |  |
| 🟢 CLEAN PASS | Performance |  |  |
| 🟢 CLEAN PASS | Security |  |  |
| 🟢 CLEAN PASS | Tech Debt |  |  |
| 🟢 CLEAN PASS | Lying Tests |  |

### Evidence Score: 2.0

| Score | Verdict |
|-------|---------|
| **2.0** | **APPROVED** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

### 🐛 Bug: Test State Corruption via Missing Isolation

**📍** `firmware/test/test_mode_orchestrator/test_main.cpp:393,410,473`

**🔄 Reproduction:** Run tests in different orders or insert tests that modify `_manualModeId` before the dl-1.4 tests. The WDT recovery tests (`test_wdt_recovery_forces_clock_mode`, `test_wdt_recovery_persists_clock_to_nvs`) call `initOrchestratorWithRegistry()` at the start, but `test_wdt_boot_uses_orchestrator_not_direct_registry` (line 473) does NOT. It depends on the final state from all previous tests including the WDT tests which modify `_manualModeId` via `simulateBootRestore()`. If a test is inserted that modifies orchestrator state, `test_wdt_boot_uses_orchestrator_not_direct_registry` will fail flakily.

**💡 Fix:** Add `initOrchestratorWithRegistry();` at the start of `test_wdt_boot_uses_orchestrator_not_direct_registry()` to ensure test isolation.

---

## 🔧 Maintainability Issues

### 💣 Tech Debt: Boot Restore Missing Error Handling

**📍** `firmware/src/main.cpp:801,806,816,820`

**💥 Explosion radius:** Medium - could leave NVS in inconsistent state if mode switching fails

**Issue:** The boot mode restore code calls `ModeOrchestrator::onManualSwitch()` but never checks if it succeeded before calling `ConfigManager::setDisplayMode()`. If `onManualSwitch` returns early due to `requestSwitch` failure (e.g., heap guard failure, unknown mode), the orchestrator state remains unchanged but NVS gets written anyway with the potentially invalid mode ID. This creates a mismatch between orchestrator state and NVS that will persist across reboots until manually corrected.

```cpp
// Line 801 - WDT recovery path
ModeOrchestrator::onManualSwitch("clock", clockName);
ConfigManager::setDisplayMode("clock");  // ❌ Never checked if onManualSwitch succeeded!
```

**Impact:** If a mode switch fails during boot (e.g., ClockMode fails to init due to heap exhaustion), NVS will be written with "clock" but the orchestrator will stay in its previous state (likely still uninitialized or in whatever state it was before). On next boot, the cycle repeats - stuck in a boot loop with corrupted NVS.

**Why it's not caught:** The tests use `simulateBootRestore()` which also doesn't check return values, and they only verify the "happy path" where everything succeeds. No test covers the failure case where `requestSwitch` returns false.

**💡 Fix:** Check the return value or verify the orchestrator state before writing to NVS:

```cpp
// WDT recovery path (lines 798-808)
if (g_wdtRecoveryBoot) {
    const char* clockName = findModeName("clock");
    if (clockName) {
        LOG_W("Main", "WDT recovery: forcing clock mode");
        ModeOrchestrator::onManualSwitch("clock", clockName);
        // ✅ Verify switch succeeded before persisting
        if (ModeRegistry::getSwitchState() == SwitchState::REQUESTED) {
            ConfigManager::setDisplayMode("clock");
        } else {
            LOG_E("Main", "WDT recovery: failed to switch to clock mode, NVS not updated");
        }
    } else {
        // Similar error handling needed here
    }
}
```

---

## Review Actions Summary

- Issues Found: 2
- Issues Fixed: 0
- Action Items Created: 0

✅ Code is approved and ready for deployment!

**Note:** The two issues found are moderate-severity testing and error handling gaps. The code functions correctly in the happy path, but would benefit from test isolation improvements and defensive NVS writes to handle edge cases (mode init failures, heap guard failures) more gracefully. Consider addressing in a follow-up cleanup story.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-16</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">dl-1</var>
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
<var name="session_id">1b016656-6f51-4f4e-a3a7-79ea300c1de1</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="5e9f66e6">embedded in prompt, file id: 5e9f66e6</var>
<var name="story_id">dl-1.4</var>
<var name="story_key">dl-1-4-watchdog-recovery-to-default-mode</var>
<var name="story_num">4</var>
<var name="story_title">4-watchdog-recovery-to-default-mode</var>
<var name="template">False</var>
<var name="timestamp">20260416_0823</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-1.4)
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