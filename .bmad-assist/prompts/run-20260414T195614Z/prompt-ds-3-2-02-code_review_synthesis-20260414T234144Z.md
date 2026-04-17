<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-3 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260414T234144Z -->
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

Status: review

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

AC #5 NVS correction uses the existing debounced `tickNvsPersist()` mechanism rather than immediate writes. In each `executeSwitch()` failure path, `_nvsWritePending` and `_lastSwitchMs` are set so the next `tickNvsPersist()` call (after 2-second debounce) persists the actually-active mode ID. When `_activeModeIndex == MODE_INDEX_NONE` (no mode active), NVS is corrected to `"classic_card"` as the safe default.

### File List

- firmware/src/main.cpp (modified — boot NVS correction + upgrade notification detection)
- firmware/core/ConfigManager.h (modified — added hasDisplayMode, setUpgNotif, getUpgNotif; canonical name docs)
- firmware/core/ConfigManager.cpp (modified — implemented hasDisplayMode, setUpgNotif, getUpgNotif)
- firmware/core/ModeRegistry.cpp (modified — NVS correction in all failure paths + fallback in tickNvsPersist)
- firmware/adapters/WebPortal.cpp (modified — refactored upg_notif to use ConfigManager methods)
- firmware/test/test_mode_registry/test_main.cpp (modified — added 4 new tests)

## Previous story intelligence

- **ds-3.1** added **`ModeRegistry`**-backed GET and **`upg_notif`**; **ds-3.2** tightens **NVS ↔ boot ↔ active mode** invariants.

## Git intelligence summary

Touches **`main.cpp`**, **`ConfigManager`**, possibly **`ModeRegistry.cpp`** for failure-path NVS sync.

## Project context reference

`_bmad-output/project-context.md` — NVS namespace **`flightwall`**, debounced writes.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.


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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-3-2-nvs-mode-persistence-and-boot-restore.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **5** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | AC3 & Task 1.1 Lie: Boot fallback forcefully ignores "classic_card" mandate in favor of "clock" | `firmware/src/main.cpp` | +3.0 |
| 🔴 CRITICAL | AC5 Violation / State Drift: First-switch failure leaves registry idle but writes "classic_card" to NVS, causing permanent drift | `firmware/core/ModeRegistry.cpp` | +3.0 |
| 🟠 IMPORTANT | Lying Test: Failed switch test entirely omits the critical boot-time failure path | `firmware/test/test_mode_registry/test_main.cpp` | +1.0 |
| 🟠 IMPORTANT | SOLID Violation (Open/Closed): ModeRegistry hardcodes specific "classic_card" string for ultimate fallback | `firmware/core/ModeRegistry.cpp` | +1.0 |
| 🟡 MINOR | Tech Debt: Inconsistent concurrency semantics for NVS singleton methods | `firmware/core/ConfigManager.cpp` | +0.3 |
| 🟢 CLEAN PASS | 4 (Performance, Security, Style, Type Safety) | | -2.0 |

### Evidence Score: 6.3

| Score | Verdict |
|-------|---------|
| **6.3** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[8/10] Open/Closed Principle Violation:** The `ModeRegistry` explicitly hardcodes the string `"classic_card"` as its ultimate NVS fallback mechanism in `tickNvsPersist()`. As an orchestration component, the registry should not be coupled to specific plugin mode IDs. It should dynamically resolve a safe fallback (e.g., using `_table[0].id`), matching the pattern already correctly utilized in `completeOTAAttempt()`.
  - 📍 `firmware/core/ModeRegistry.cpp:348`
  - 💡 Fix: Replace `ConfigManager::setDisplayMode("classic_card");` with `ConfigManager::setDisplayMode(_count > 0 ? _table[0].id : "classic_card");`

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** AC5 demands that if an initialization fails, the device "leaves classic_card (or prior safe mode) active" ensuring "no silent drift between NVS and getActiveModeId()". However, if the *first* switch requested at boot fails (meaning `_activeModeIndex` is `MODE_INDEX_NONE`), `executeSwitch` falls back to leaving `_activeMode = nullptr`. Then, `tickNvsPersist()` triggers and blindly writes `"classic_card"` to NVS without ever activating it. This immediately introduces the exact silent state drift the AC forbade: `ConfigManager::getDisplayMode()` returns `"classic_card"`, but `getActiveModeId()` returns `nullptr`, trapping the user on the emergency fallback display.
  - 📍 `firmware/core/ModeRegistry.cpp:218`
  - 🔄 Reproduction: Simulate `init() == false` for the mode stored in NVS during a fresh boot. The display will show the emergency fallback screen, but NVS will silently transition to `"classic_card"` without actually starting the mode.

- **🎭 Lying Test:** `test_failed_init_corrects_nvs_after_debounce`
  - 📍 `firmware/test/test_mode_registry/test_main.cpp:295`
  - 🤥 Why it lies: Task 4.2 explicitly requires covering the "invalid NVS boot" cases. This test purports to validate AC5's NVS correction logic, but it *only* tests the scenario where a safe mode (`mock_mode_a`) was already active prior to the failed switch. It completely omits testing the critical "first-switch fails" edge case (where no prior mode exists), allowing the severe state drift bug above to go entirely undetected.

- **🐛 Bug (AC3 / Task 1.1 Lie):** AC3 states: `Then ModeRegistry::requestSwitch("classic_card") runs and ConfigManager::setDisplayMode("classic_card") is invoked immediately`. Furthermore, Task 1.1 explicitly claims: `After fallback requestSwitch("classic_card") when saved mode invalid...`. In direct contradiction, the implemented fallback chain in `main.cpp` prioritizes forcing `"clock"` mode over `"classic_card"`. The developer willfully ignored the strict criteria to implement their preferred fallback sequence.
  - 📍 `firmware/src/main.cpp:390`
  - 🔄 Reproduction: Flash the device with an unrecognized mode ID in NVS. The logs will output "WDT recovery: forcing clock mode" or "Saved display mode invalid, correcting NVS to clock", bypassing the explicitly mandated AC3 behavior.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `ConfigManager::setDisplayMode()` and `setUpgNotif()` directly open the `Preferences` namespace and mutate NVS without acquiring `g_configMutex` (via `ConfigLockGuard`). While ESP-IDF's underlying NVS implementation handles its own thread safety, skipping the mutex breaks the established concurrency semantics within `ConfigManager` (e.g., `persistToNvs()` and `factoryReset()` wrap NVS access).
  - 📍 `firmware/core/ConfigManager.cpp:707`
  - 💥 Explosion radius: Minor — Confusing locking strategies for future maintainers extending the configuration engine.

---

## 🛠️ Suggested Fixes

### 1. Resolve AC3 & Task 1.1 Boot Fallback Discrepancy

**File:** `firmware/src/main.cpp`
**Issue:** Boot recovery logic forces `"clock"` instead of the mandated `"classic_card"`.

**Diff:**
```diff
@@ -387,14 +387,7 @@
             if (bootModeName) {
                 ModeOrchestrator::onManualSwitch(savedMode.c_str(), bootModeName);
             } else {
-                const char* clockName = findModeName("clock");
-                if (clockName) {
-                    LOG_W("Main", "Saved display mode invalid, correcting NVS to clock");
-                    ModeOrchestrator::onManualSwitch("clock", clockName);
-                    ConfigManager::setDisplayMode("clock");
-                } else {
-                    LOG_W("Main", "Saved display mode invalid, correcting NVS to classic_card");
-                    ModeOrchestrator::onManualSwitch("classic_card", "Classic Card");
-                    ConfigManager::setDisplayMode("classic_card");
-                }
+                LOG_W("Main", "Saved display mode invalid, correcting NVS to classic_card");
+                ModeOrchestrator::onManualSwitch("classic_card", "Classic Card");
+                ConfigManager::setDisplayMode("classic_card");
             }
```

### 2. Fix AC5 NVS State Drift on First-Switch Failure

**File:** `firmware/core/ModeRegistry.cpp`
**Issue:** When the first switch fails, no mode is activated but NVS receives a hardcoded `"classic_card"` write, causing permanent state drift.

**Diff:**
```diff
@@ -214,6 +214,9 @@
                 } else {
                     _activeModeIndex = prevIndex;
                 }
             } else {
                 _activeModeIndex = MODE_INDEX_NONE;
+                // Enforce AC5: Activate the ultimate safe fallback dynamically
+                if (_count > 0) {
+                    _requestedIndex.store(0); // Queues table[0] (typically classic_card) for next tick
+                }
             }
         }
```

**Review Actions:**
- Issues Found: 5
- Issues Fixed: 0
- Action Items Created: 0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

I'm ready to help. What task would you like me to work on?

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
<var name="session_id">f5e4cd4c-34b1-4b3f-9c45-d919bd710fe3</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="0ae0ec16">embedded in prompt, file id: 0ae0ec16</var>
<var name="story_id">ds-3.2</var>
<var name="story_key">ds-3-2-nvs-mode-persistence-and-boot-restore</var>
<var name="story_num">2</var>
<var name="story_title">2-nvs-mode-persistence-and-boot-restore</var>
<var name="template">False</var>
<var name="timestamp">20260414_1941</var>
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