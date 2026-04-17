<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-3 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260415T000700Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story ds-3.2

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
<file id="0ae0ec16" path="_bmad-output/implementation-artifacts/stories/ds-3-2-nvs-mode-persistence-and-boot-restore.md" label="DOCUMENTATION"><![CDATA[

# Story ds-3.2: NVS Mode Persistence & Boot Restore

Status: complete

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user,
I want my display mode selection saved across power cycles,
So that the device boots into my preferred mode without requiring manual reselection.

## Acceptance Criteria

1. **Given** a successful **display** mode switch (registry active mode changes and remains stable), **When** persistence runs, **Then** the active mode id is stored in NVS namespace **`flightwall`** under key **`disp_mode`** (9 characters — **within** the 15-character NVS key limit, **NFR S6**). **Do not** add a second key named **`display_mode`** (epic text uses that name; **this product** standardizes on **`disp_mode`** already used by **`ConfigManager::getDisplayMode` / `setDisplayMode`**). If a one-time migration from an older **`display_mode`** key is ever found in the wild, read it once, write **`disp_mode`**, delete the legacy key — **document** if implemented.

2. **Given** **`ConfigManager::setDisplayMode(const char*)`** is the **single** writer for **`disp_mode`** from firmware (aside from tests), **When** **`ModeRegistry::tickNvsPersist()`** debounces a post-switch write, **Then** it still calls **`ConfigManager::setDisplayMode`** — **no** raw **`Preferences`** writes for **`disp_mode`** outside **`ConfigManager`** (keeps AR7 collision audit centralized).

3. **Given** boot sequence in **`main.cpp`** after **`g_display.initialize()`**, **When** **`ConfigManager::getDisplayMode()`** returns a mode id **present** in **`MODE_TABLE`**, **Then** **`ModeRegistry::requestSwitch(savedMode.c_str())`** succeeds; **When** the id is **unknown** or **`requestSwitch`** returns **false**, **Then** **`ModeRegistry::requestSwitch("classic_card")`** runs **and** **`ConfigManager::setDisplayMode("classic_card")`** is invoked **immediately** so NVS no longer stores an invalid id (**FR28** / epic invalid-mode case).

4. **Given** **Foundation Release** upgrade path (no **`disp_mode`** key has ever been written), **When** the device boots the **first** time after firmware that includes mode persistence, **Then** runtime behavior defaults to **`classic_card`** (already **`getDisplayMode()`** default) **and** **`GET /api/display/modes`** reports **`upgrade_notification: true`** per **FR29** / epic — implement by ensuring NVS flag **`upg_notif`** is **1** when **`disp_mode`** key is **absent** on first boot **or** by explicit **`setUpgNotif(true)`** in **`setup()`** once; **coordinate** with **`POST /api/display/notification/dismiss`** (already clears **`upg_notif`**) and **ds-3.6** banner UX.

5. **Given** a **valid** saved mode that **fails** activation on first **`ModeRegistry::tick()`** (heap guard or **`init()`** false — **rare** at boot), **Then** registry restore logic leaves **`classic_card`** (or prior safe mode) active **and** NVS is **corrected** to the **actually active** registry mode id **within** the same boot window (may require a small hook after first successful **`tick`** or explicit **`setDisplayMode`** from **`ModeRegistry`** error path — **document** chosen approach in Dev Agent Record; **no** silent drift between NVS and **`getActiveModeId()`**).

6. **Given** **`ModeOrchestrator`** / dashboard **manual** mode (Delight), **When** the user’s choice is committed to **`ModeRegistry`**, **Then** **`disp_mode`** eventually matches (**debounced** write is acceptable); **orchestrator** state itself **need not** duplicate NVS if **`ModeRegistry`** remains source of truth for **`disp_mode`** — document interaction if orchestrator adds its own NVS later.

7. **Given** **`test_mode_registry`** / **`test_config_manager`** tests, **When** **`pio test`** runs, **Then** NVS read/write tests for **`disp_mode`** continue to pass; add tests for **invalid boot id → NVS rewritten** if harness supports it.

8. **Given** **`pio run`**, **Then** no new warnings.

## Tasks / Subtasks

- [x] Task 1: Boot NVS correction (AC: #3)
  - [x] 1.1: After fallback **`requestSwitch("classic_card")`** when saved mode invalid, call **`ConfigManager::setDisplayMode("classic_card")`**
  - [x] 1.2: Log once at **`LOG_W`** when correction occurs

- [x] Task 2: Upgrade notification coherence (AC: #4)
  - [x] 2.1: Ensure **`upg_notif`** or equivalent reflects "first run after upgrade" per epic; align **`GET`** with **`setup()`** semantics
  - [x] 2.2: Verify **`POST /api/display/notification/dismiss`** clears flag and **`GET`** returns **`false`**

- [x] Task 3: Post-switch / failed-switch NVS (AC: #5)
  - [x] 3.1: Trace **`ModeRegistry::executeSwitch`** failure paths; add **`setDisplayMode`** where NVS would otherwise stay stale
  - [x] 3.2: Avoid double-write storms (respect debounce where appropriate)

- [x] Task 4: Documentation + tests (AC: #1, #2, #7, #8)
  - [x] 4.1: Comment in **`ConfigManager.h`** / epic cross-ref: **`disp_mode`** canonical name
  - [x] 4.2: Extend tests if missing cases for invalid NVS boot

#### Review Follow-ups (AI)
- [x] [AI-Review] HIGH: AC3 violated — normal-boot fallback tried "clock" before "classic_card"; fixed to go directly to "classic_card" (`firmware/src/main.cpp`)
- [x] [AI-Review] HIGH: AC5 state drift — tickNvsPersist wrote "classic_card" to NVS without activating it; fixed to queue `_requestedIndex=0` for next tick (`firmware/core/ModeRegistry.cpp`)
- [x] [AI-Review] MEDIUM: OCP violation — tickNvsPersist hardcoded "classic_card" string; fixed to use `_table[0].id` (`firmware/core/ModeRegistry.cpp`)
- [x] [AI-Review] MEDIUM: Missing test for cold-boot first-switch-fail edge case; added `test_first_switch_fail_queues_recovery_activation` (`firmware/test/test_mode_registry/test_main.cpp`)

#### Review Follow-ups (AI) — Pass 2
- [x] [AI-Review] HIGH: Infinite recovery loop — if `_table[0]` also fails init, `tickNvsPersist()` queues activation indefinitely every 2s, burning NVS flash; fixed with `_recoveryQueued` flag (`firmware/core/ModeRegistry.h`, `firmware/core/ModeRegistry.cpp`)
- [x] [AI-Review] HIGH: Layout hot-reload desync — zone_logo_pct, zone_split_pct, zone_layout changes not detected by `hardwareConfigChanged`/`hardwareMappingChanged`, so `_layout` in NeoMatrixDisplay is never updated without reboot despite API returning `reboot_required: false` (`firmware/src/main.cpp`)
- [x] [AI-Review] MEDIUM: Type safety — `kv.value().as<int32_t>()` silently returns 0 for non-numeric JSON strings, which can pass range validation and overwrite NVS settings with 0; fixed with `is<int32_t>()` pre-check (`firmware/adapters/WebPortal.cpp`)

## Dev Notes

### Already implemented (verify, do not duplicate)

- **`ConfigManager::getDisplayMode` / `setDisplayMode`** — **`firmware/core/ConfigManager.cpp`** ~688–713, key **`disp_mode`**
- **Boot `requestSwitch`** after **`initialize()`** — **`firmware/src/main.cpp`** ~688–697
- **Registry debounced NVS** — **`ModeRegistry::tickNvsPersist`** → **`setDisplayMode`**
- **`GET` `upgrade_notification`** — **`upg_notif`** in **`WebPortal::_handleGetDisplayModes`**
- **Dismiss route** — **`POST /api/display/notification/dismiss`**

### Epic vs product naming

| Epic text | Product |
|-----------|---------|
| NVS key `display_mode` | **`disp_mode`** |

[Source: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` Story ds-3.2, `ConfigManager.h`]

### Dependencies

- **ds-1.5** boot ordering (**`initialize`** before **`requestSwitch`**) — assumed present.
- **ds-3.1** GET/POST display routes — assumed present for **`upgrade_notification`** surface.

### Out of scope

- **ds-3.3** — Mode Picker UI
- **ds-3.6** — full banner (may consume **`upgrade_notification`**)

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.2)
- Architecture: `_bmad-output/planning-artifacts/architecture.md` — **D6** NVS debounce
- Prior: `_bmad-output/implementation-artifacts/stories/ds-3-1-display-mode-api-endpoints.md`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

No debug issues encountered.

### Completion Notes List

- **Task 1 (AC #3):** Added `ConfigManager::setDisplayMode("classic_card")` call in `main.cpp` boot mode restore when `requestSwitch()` rejects an invalid saved mode. LOG_W message updated to indicate NVS correction.
- **Task 2 (AC #4):** Added `ConfigManager::hasDisplayMode()`, `setUpgNotif()`, `getUpgNotif()` to centralize `upg_notif` NVS access (AR7). In `setup()`, detects Foundation Release upgrade (no `disp_mode` key) and sets `upg_notif=1`. Refactored `WebPortal.cpp` to use these centralized methods instead of raw `Preferences` calls.
- **Task 3 (AC #5):** In all three `ModeRegistry::executeSwitch()` failure paths (heap guard, factory null, init fail), set `_nvsWritePending = true` so `tickNvsPersist()` corrects NVS to the actually-active mode after debounce. Added fallback in `tickNvsPersist()` to write `classic_card` when no mode is active (all failed). Respects existing 2-second debounce — no double-write storms.
- **Task 4 (AC #1, #2, #7, #8):** Added canonical name documentation to `ConfigManager.h`. Added 4 new tests: `test_failed_init_corrects_nvs_after_debounce`, `test_has_display_mode_false_when_absent`, `test_has_display_mode_true_after_set`, `test_upg_notif_roundtrip`. No new build warnings.

### Implementation Approach (AC #5 Documentation)

AC #5 NVS correction uses the existing debounced `tickNvsPersist()` mechanism rather than immediate writes. In each `executeSwitch()` failure path, `_nvsWritePending` and `_lastSwitchMs` are set so the next `tickNvsPersist()` call (after 2-second debounce) persists the actually-active mode ID. When `_activeModeIndex == MODE_INDEX_NONE` (no mode active), NVS is corrected to `_table[0].id` (the safe default) **and** `_requestedIndex` is queued to 0 so that the next `tick()` activates that mode — preventing silent drift between NVS and `getActiveModeId()`.

### File List

- firmware/src/main.cpp (modified — boot NVS correction + upgrade notification detection; **synthesis fix: removed intermediate clock fallback in normal-boot invalid-mode path per AC #3**)
- firmware/core/ConfigManager.h (modified — added hasDisplayMode, setUpgNotif, getUpgNotif; canonical name docs)
- firmware/core/ConfigManager.cpp (modified — implemented hasDisplayMode, setUpgNotif, getUpgNotif)
- firmware/core/ModeRegistry.h (modified — **synthesis fix 2: added `_recoveryQueued` static member to break infinite recovery loop when table[0] also fails**)
- firmware/core/ModeRegistry.cpp (modified — NVS correction in all failure paths + fallback in tickNvsPersist; **synthesis fix: AC #5 state-drift fix + replaced hardcoded "classic_card" with _table[0].id**; **synthesis fix 2: `_recoveryQueued` guards tickNvsPersist else-branch; reset on successful switch**)
- firmware/adapters/WebPortal.cpp (modified — refactored upg_notif to use ConfigManager methods; **synthesis fix 3: added `is<int32_t>()` type check in settings pre-validation to reject non-numeric values with HTTP 400**)
- firmware/src/main.cpp (modified — boot NVS correction + upgrade notification detection; **synthesis fix: removed intermediate clock fallback in normal-boot invalid-mode path per AC #3**; **synthesis fix 4: zone_logo_pct, zone_split_pct, zone_layout added to hardwareConfigChanged + hardwareMappingChanged so zone hot-reloads update _layout via reconfigureFromConfig()**)
- firmware/test/test_mode_registry/test_main.cpp (modified — added 4 new tests; **synthesis fix: added test_first_switch_fail_queues_recovery_activation covering cold-boot first-switch-fail edge case**)

## Previous story intelligence

- **ds-3.1** added **`ModeRegistry`**-backed GET and **`upg_notif`**; **ds-3.2** tightens **NVS ↔ boot ↔ active mode** invariants.

## Git intelligence summary

Touches **`main.cpp`**, **`ConfigManager`**, possibly **`ModeRegistry.cpp`** for failure-path NVS sync.

## Project context reference

`_bmad-output/project-context.md` — NVS namespace **`flightwall`**, debounced writes.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-14 (Pass 1)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 6.3 → REJECT (MAJOR REWORK)
- **Issues Found:** 4 verified (2 High, 2 Medium) + 1 Low deferred
- **Issues Fixed:** 4
- **Action Items Created:** 0 (all verified issues fixed in this synthesis pass)

### Review: 2026-04-14 (Pass 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 5.3 → REJECT (MAJOR REWORK)
- **Issues Found:** 3 verified (1 High+, 1 High, 1 Medium) + 3 dismissed
- **Issues Fixed:** 3
- **Action Items Created:** 0 (all verified issues fixed in this synthesis pass)


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic ds-3 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story ds-3-1 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Orchestrator bypassed when user re-selects the currently active mode | Removed the `if (!isSameMode)` guard — `onManualSwitch` is now always called. This correctly transitions the orchestrator from `IDLE_FALLBACK`/`SCHEDULED` back to `MANUAL` when a user explicitly selects a mode, even if it's already the active one. `ModeRegistry::tick()` is idempotent for same-mode requests (line 367 guard `if (requested != _activeModeIndex)`), so no spurious mode restarts occur. |
| medium | `POST /api/display/mode` body handler skips chunk accumulation — only the final TCP chunk is parsed | Replaced the direct `deserializeJson(data, len)` path with the established `g_pendingBodies` accumulation pattern (identical to `POST /api/schedule` and `POST /api/settings`). Extracted all business logic into new `_handlePostDisplayMode(request, data, len)` helper for separation of concerns. |
| low | `SwitchState` → string mapping duplicated in `_handleGetDisplayModes` and the POST handler | Extracted to `static const char* switchStateToString(SwitchState)` in the anonymous namespace. Both call sites updated. |
| dismissed | Async POST decision breaks AC 7 and AC 9 API contracts — heap failures buried in HTTP 200 | FALSE POSITIVE: The Dev Agent Record (Implementation Plan, AC #7) explicitly documents this as an **accepted deviation from Architecture D5**. The rationale is sound: `ModeRegistry::tick()` runs on Core 0 while AsyncWebServer body handlers run on the AsyncTCP task (Core 1); a bounded wait here risks WDT. The story explicitly chose "async response with `switch_state: requested` + client re-fetch (ds-3.4 UX)" as the strategy. Heap errors are surfaced in `data.registry_error` on a 200 — a limitation of the async approach, not an omission. This is a design trade-off, not a bug. |
| dismissed | N+1 NVS reads violate `<10ms` enumeration budget (NFR P4) — 20–50ms per GET | FALSE POSITIVE: The reviewer assumed "4 modes and 10 settings." Reality: `ClassicCardMode::getSettingsSchema()` returns `nullptr`, `LiveFlightCardMode::getSettingsSchema()` returns `nullptr`. Only `ClockMode` (1 setting) and `DeparturesBoardMode` (1 setting) have schemas. Total NVS reads per GET: **2** (one per modes-with-settings) plus `getUpgNotif()` = **3 total**. The "20–50ms" claim is unsupported. Noted as future technical debt for when more settings are added (see Suggested Future Improvements). --- |

## Story ds-3-1 (2026-04-14)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `ModeRegistry::requestSwitch()` did not set `_switchState = REQUESTED` — the POST handler on Core 1 read the state immediately after queuing a switch and always saw `"idle"` instead of `"requested"`, completely breaking the AC #7 async strategy where the client is expected to poll on `switch_state: "requested"`. | Added `_switchState.store(SwitchState::REQUESTED)` in `requestSwitch()` before `return true`. Also added an `else` branch in `tick()` to reset state to IDLE when a same-mode request is consumed (prevents REQUESTED state from lingering when no actual switch occurs). |
| high | `_handlePostDisplayMode` applied mode settings sequentially with no pre-validation of values — if the first key-value pair succeeded (writing to NVS) but a subsequent pair failed value validation, NVS was left in a partially-applied state, violating the explicit "no partial apply" rule in the code comment. | Added a pre-validation pass that checks every value against the schema's `minValue`/`maxValue` fields before writing any setting. The apply loop only runs after all values pass. |
| medium | O(N) character-by-character string concatenation (`pending->body += char`) in all three POST body handlers (settings, display/mode, schedule) — causes repeated bounds-check overhead and blocks the AsyncTCP task for multi-chunk payloads. | Replaced all three loops with `pending->body.concat(reinterpret_cast<const char*>(data), len)` — single-call bulk append. |
| dismissed | AC 9 / registry_error buried inside HTTP 200 OK (`ok: true`) violates the API envelope contract for heap-guard failures. | FALSE POSITIVE: Already adjudicated as a FALSE POSITIVE in the prior synthesis pass (ANTIPATTERNS file, row "dismissed"). The Dev Agent Record (AC #7 Implementation Plan) explicitly documents the async response strategy as an accepted deviation from Architecture D5. Heap failures that occur asynchronously on Core 0 cannot surface in the already-sent POST response. `registry_error` in a 200 is the documented trade-off, not an omission. |
| dismissed | State desync — `ModeOrchestrator::onManualSwitch()` persists `_manualModeId` to NVS before `ModeRegistry` confirms the switch, leaving the orchestrator referencing a rejected mode. | FALSE POSITIVE: Design trade-off inherent in the cooperative-tick architecture. The orchestrator will discover the registry error on the next `tick()` cycle and revert. No data corruption — NVS holds the orchestrator's intent, not the confirmed active mode (the registry separately debounce-writes the confirmed active mode). Not actionable without restructuring the orchestrator API (separate story scope). |
| dismissed | Large `String` JSON allocation in `_handleGetDisplayModes` risks heap fragmentation — should use `AsyncResponseStream`. | FALSE POSITIVE: The GET response is approximately 900 bytes for 3 modes. While `AsyncResponseStream` is a best practice, the current mode count makes this a marginal improvement, not a correctness issue. This pattern is universal across all WebPortal handlers and would require a broader refactoring pass. Deferred as future improvement. |
| dismissed | SOLID violation — `_handlePostDisplayMode` is a god method combining HTTP parsing, schema lookup, value validation, NVS writes, and orchestrator dispatch. | FALSE POSITIVE: Valid architectural observation, but out of scope for a synthesis fix. Refactoring to extract `ConfigManager::applyModeSettings()` is a meaningful improvement but would expand change surface beyond modified files. Deferred to a dedicated refactoring story. - **Validator B output**: No findings produced. \| **Raised by**: Validator B \| **Dismissal Reason**: Reviewer produced only a placeholder greeting — no code analysis performed. --- |

## Story ds-3-2 (2026-04-15)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Infinite recovery loop when `_table[0]` (default mode) also fails to initialize — `tickNvsPersist()` writes NVS safe-default and queues `_requestedIndex=0` unconditionally, but after `executeSwitch(0)` fails it re-sets `_nvsWritePending=true`, causing the loop to repeat every 2 seconds indefinitely, burning NVS flash write cycles | Added `static bool _recoveryQueued` member to `ModeRegistry`; `tickNvsPersist()` now only queues activation once per failure episode (`!_recoveryQueued` guard); flag is reset to `false` in `executeSwitch()` on a successful switch so future failures start fresh. Reviewer's proposed fix (`_requestedIndex.load() != 0`) was ineffective (the request is consumed by `tick()` before `tickNvsPersist()` runs again); a proper one-shot flag was implemented instead. |
| high | Display layout hot-reload desync — `zone_logo_pct`, `zone_split_pct`, `zone_layout` are not in REBOOT_KEYS (treated as hot-reload), but `hardwareConfigChanged()` and `hardwareMappingChanged()` in `main.cpp` don't include them. When they change, `g_display.buildRenderContext()` is called but returns the OLD `_layout` (since `NeoMatrixDisplay::_layout` is only updated in `rebuildMatrix()`). `reconfigureFromConfig()` → `rebuildMatrix()` is never triggered for zone-only changes, so the physical LED layout doesn't update until reboot despite the API returning `reboot_required: false`. | Added `zone_logo_pct`, `zone_split_pct`, `zone_layout` to both `hardwareConfigChanged()` and `hardwareMappingChanged()`. Zone changes now route through `reconfigureFromConfig()` → `rebuildMatrix()` which updates `_layout = LayoutEngine::compute(hw)`. |
| medium | Type safety — in `_handlePostDisplayMode()`, the pre-validation loop calls `kv.value().as<int32_t>()` without first checking `is<int32_t>()`. A non-numeric JSON string (e.g. `{"format": "bad"}`) silently returns 0 from `as<>()`. If 0 is within the schema's valid range (e.g. `clock/format` has `minValue=0, maxValue=1`), the request passes validation and persists 0 to NVS instead of returning a 400 error. | Added `if (!kv.value().is<int32_t>())` guard at the start of the pre-validation loop, returning HTTP 400 with `"INVALID_SETTING_TYPE"` for non-numeric values. |
| dismissed | `ConfigManager::setModeSetting` hardcodes range validation per-mode (OCP violation) | FALSE POSITIVE: Pre-adjudicated as a false positive in the ANTIPATTERNS file from ds-3.1 synthesis: "Deferred to a dedicated refactoring story." The API-layer's dynamic schema validation in `_handlePostDisplayMode` already guards all incoming values. The ConfigManager layer validation is defense-in-depth for the 2 existing modes, not a scalability problem at the current 2-mode schema scope. |
| dismissed | `_handlePostDisplayMode` is a God Method (SRP violation, 100 lines) | FALSE POSITIVE: Pre-adjudicated as a false positive in the ANTIPATTERNS file from ds-3.1 synthesis: "Deferred to a dedicated refactoring story." The pattern is universal across all WebPortal handlers; extracting to `ConfigManager::applyModeSettings()` is a meaningful future improvement but expands change surface beyond this story's scope. |
| dismissed | `ModeOrchestrator` caches `_manualModeId` before `ModeRegistry` confirms the switch; if the registry rejects the switch, orchestrator will blindly retry the rejected mode | FALSE POSITIVE: Pre-adjudicated as a false positive in the ANTIPATTERNS file from ds-3.1 synthesis: "Design trade-off inherent in the cooperative-tick architecture. The orchestrator will discover the registry error on the next `tick()` cycle and revert. Not actionable without restructuring the orchestrator API (separate story scope)." |
| dismissed | Validator B raised no issues | FALSE POSITIVE: No analysis was produced. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-3-2-nvs-mode-persistence-and-boot-restore.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 1 |
| Lying Tests | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Race Condition returning static pointer across cores | `firmware/adapters/WebPortal.cpp` | +3.0 |
| 🔴 CRITICAL | Memory leak on mode initialization failure | `firmware/core/ModeRegistry.cpp` | +3.0 |
| 🟠 IMPORTANT | Lying Test: Tautological assertion on NVS recovery | `firmware/test/test_mode_registry/test_main.cpp` | +1.0 |
| 🟠 IMPORTANT | Performance Footgun: `String` capacity lost during copy | `firmware/adapters/WebPortal.cpp` | +1.0 |
| 🟠 IMPORTANT | Type Safety / SOLID: Rigid JSON validation rejects booleans | `firmware/adapters/WebPortal.cpp` | +1.0 |
| 🟠 IMPORTANT | Hidden Bug: Partial apply risk due to double validation | `firmware/adapters/WebPortal.cpp` | +1.0 |
| 🟢 CLEAN PASS | 3 | | -1.5 |

### Evidence Score: 8.5

| Score | Verdict |
|-------|---------|
| **8.5** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] Open/Closed Principle (OCP):** Rigid JSON validation rejects valid schema types.
  - 📍 `firmware/adapters/WebPortal.cpp:468`
  - 💡 Fix: The pre-validation loop enforces `!kv.value().is<int32_t>()` for all incoming mode settings. This strict check rejects `true` or `false` boolean values, even though the mode configuration schema officially supports `"bool"` as a setting type. As modes add boolean settings, the API will inexplicably reject valid payloads. Validation should dynamically consult `schema->settings[s].type` instead of blindly assuming all settings are 32-bit integers.

---

## ⚡ Performance & Scalability

- **[High] String Buffer Reallocation / Capacity Loss:**
  - 📍 `firmware/adapters/WebPortal.cpp:138` (and lines 334, 381)
  - 💡 Fix: In the chunk upload handlers for `POST /api/settings`, `POST /api/display/mode`, and `POST /api/schedule`, the code attempts to pre-allocate string capacity: `PendingRequestBody pending{request, String()}; pending.body.reserve(total); g_pendingBodies.push_back(pending);`. However, `std::vector::push_back` makes a copy of the `pending` struct. The `String` copy constructor allocates exactly the used length (0 bytes), completely discarding the `.reserve(total)` capacity. As chunks arrive, `.concat()` causes repeated heap reallocations and memory fragmentation on the ESP32. Add to the vector first, then reserve on the actual stored element: `g_pendingBodies.push_back({request, String()}); g_pendingBodies.back().body.reserve(total);`

---

## 🐛 Correctness & Safety

- **🐛 Bug: Race Condition returning static pointer across cores**
  - 📍 `firmware/adapters/WebPortal.cpp:527`
  - 🔄 Reproduction: `ModeRegistry::getLastError()` returns a raw pointer to a shared static char buffer (`_lastError`). The `WebPortal` (running on Core 1 async task) reads this pointer and passes it directly to ArduinoJson to serialize the HTTP response. Meanwhile, `ModeRegistry::requestSwitch()` (any core) or `ModeRegistry::tick()` (Core 0) can simultaneously `snprintf` into this exact same buffer. The `_errorUpdating` atomic only protects the *moment* of the pointer read, not the string traversal during JSON serialization. This causes corrupted JSON responses or crashes. Replace `getLastError()` calls with the thread-safe `copyLastError()` provided in the architecture.

- **🐛 Bug: Memory leak on mode initialization failure**
  - 📍 `firmware/core/ModeRegistry.cpp:249`
  - 🔄 Reproduction: In `ModeRegistry::executeSwitch()`, if a new mode fails initialization (`!newMode->init(ctx)`), the code executes `delete newMode;` and re-creates the previous mode. However, the architectural rules explicitly state: "all deallocation in teardown()". If `init()` allocates partial resources before returning false, `teardown()` is never called on the failed `newMode` prior to deletion, permanently leaking any heap memory it managed to allocate. Call `newMode->teardown()` immediately before `delete newMode;`.

- **🐛 Bug: Partial apply risk due to double validation**
  - 📍 `firmware/adapters/WebPortal.cpp:495`
  - 🔄 Reproduction: The code comments claim `// Apply all settings (all values pre-validated — no partial apply risk)`. However, the loop calls `ConfigManager::setModeSetting(...)` which contains its own secondary validation (e.g., NVS key length checks, hardcoded range checks). If `setModeSetting` returns `false` halfway through processing a multi-key JSON payload, `_handlePostDisplayMode` immediately returns HTTP 400, leaving NVS in a partially applied state. This directly contradicts the transactional guarantee.

- **🎭 Lying Test:** `test_first_switch_fail_queues_recovery_activation`
  - 📍 `firmware/test/test_mode_registry/test_main.cpp:519`
  - 🤥 Why it lies: This test asserts that NVS recovery logic properly overwrites an invalid mode by writing `mock_mode_a` to NVS, failing the switch, and then asserting that NVS is corrected to `mock_mode_a`. This is a tautology. If the fallback correction logic completely failed and NVS was untouched, the test would still pass because the starting value matches the expected end value. The test must write a *different* invalid mode (e.g., `mock_mode_b` or a non-existent mode) to genuinely verify that the NVS correction logic overwrites it.

✅ Code appears secure.

---

## 🔧 Maintainability Issues

✅ Code is maintainable and well-documented.

---

## 🛠️ Suggested Fixes

### 1. Fix Memory Leak on Mode Init Failure

**File:** `firmware/core/ModeRegistry.cpp`
**Issue:** `teardown()` is skipped when `init()` fails, leaking any partially allocated resources.

**Diff:**
```diff
     // Step 5: Init new mode
     if (!newMode->init(ctx)) {
         // Init failed — delete new, re-create previous; if re-init also fails, leave idle
         _lastErrorCode.store("MODE_INIT_FAILED");
         _errorUpdating.store(true);
         snprintf(_lastError, sizeof(_lastError),
                  "Init failed for %.40s", target.displayName);
         _errorUpdating.store(false);
         LOG_W("ModeRegistry", "Mode init failed");
 
+        newMode->teardown();
         delete newMode;
 
         if (prevEntry != nullptr) {
```

### 2. Fix Tautological NVS Recovery Test

**File:** `firmware/test/test_mode_registry/test_main.cpp`
**Issue:** Test writes the expected output as the starting condition, hiding total failures of the system under test.

**Diff:**
```diff
-    // Write a stale/boot mode to NVS (as if NVS already held this from a prior flash)
-    nvsWriteString("disp_mode", "mock_mode_a");
+    // Write a DIFFERENT stale/boot mode to NVS to prove correction actually overwrites it
+    nvsWriteString("disp_mode", "mock_mode_b");
 
     // Make table[0] (mock_mode_a) fail init so the first switch collapses
     g_mockModeAStats.initShouldFail = true;
```

### 3. Fix String Capacity Loss in Chunked Uploads

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** `String` capacity is lost during `std::vector::push_back` copy, leading to heap fragmentation. Applies to all three POST payload handlers.

**Diff:**
```diff
             if (index == 0) {
                 clearPendingBody(request);
-                PendingRequestBody pending{request, String()};
-                pending.body.reserve(total);
-                g_pendingBodies.push_back(pending);
+                g_pendingBodies.push_back({request, String()});
+                g_pendingBodies.back().body.reserve(total);
                 request->onDisconnect([request]() {
                     clearPendingBody(request);
                 });
```

**Review Actions:**
- Issues Found: 6
- Issues Fixed: 0
- Action Items Created: 0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

Hello! How can I help you with TheFlightWall project today?

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-14</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">ds-3</var>
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
<var name="session_id">0f868ba9-1c20-4b9c-9767-d37660f5626f</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="0ae0ec16">embedded in prompt, file id: 0ae0ec16</var>
<var name="story_id">ds-3.2</var>
<var name="story_key">ds-3-2-nvs-mode-persistence-and-boot-restore</var>
<var name="story_num">2</var>
<var name="story_title">2-nvs-mode-persistence-and-boot-restore</var>
<var name="template">False</var>
<var name="timestamp">20260414_2007</var>
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
      - Commit message format: fix(component): brief description (synthesis-ds-3.2)
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