<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-3 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260414T232247Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story ds-3.1

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
<file id="99900e09" path="_bmad-output/implementation-artifacts/stories/ds-3-1-display-mode-api-endpoints.md" label="DOCUMENTATION"><![CDATA[

# Story ds-3.1: Display Mode API Endpoints

Status: complete

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an external client (web dashboard or third-party tool),
I want HTTP API endpoints to list available modes, retrieve the active mode, and request a mode change,
So that the mode system can be controlled programmatically from any client.

## Acceptance Criteria

1. **Given** `ModeRegistry` is initialized with `MODE_TABLE` (**ds-1.3+**), **When** **`GET /api/display/modes`** runs, **Then** the JSON envelope matches the existing product pattern **`{ "ok": true, "data": { ... } }`** [Source: `architecture.md` Decision D5, existing `WebPortal` handlers] and **`data`** includes at minimum:
   - **`modes`**: array built from **`ModeRegistry::getModeTable()`** / **`getModeCount()`** (or **`enumerate()`**), **not** a hardcoded list — each element has **`id`**, **`name`** (from `ModeEntry.displayName`), and **`zone_layout`**: array of objects **`{ "label", "relX", "relY", "relW", "relH" }`** mirroring **`ZoneRegion`** from **`DisplayMode::getZoneDescriptor()`** for that mode (see AC #2)
   - **`active`**: mode id string from **`ModeRegistry::getActiveModeId()`** (or **`nullptr`** → **`null`** / **`"classic_card"`** fallback — document choice)
   - **`switch_state`**: string derived from **`ModeRegistry::getSwitchState()`** — **`"idle"`** | **`"requested"`** | **`"switching"`** (map **`SwitchState`** enum explicitly)
   - **`upgrade_notification`**: boolean read from NVS (or **`false`** until NVS key exists — implement minimal read alongside **ds-3.2** if key not yet present; default **`false`**)

2. **Given** zone metadata for schematics, **When** serializing a mode entry, **Then** populate **`zone_layout`** from that mode’s **`ModeZoneDescriptor`** (**`regions`**, **`regionCount`**, **`description`** optional as **`description`** on the mode object) — **no** `new DisplayMode` per request: add **`const ModeZoneDescriptor* zoneDescriptor`** (or pointer to static descriptor) to **`ModeEntry`** in **`ModeRegistry.h`** and wire **`MODE_TABLE`** in **`main.cpp`** so **`GET`** walks **BSS/static** data only (**NFR P4**)

3. **Given** **dl-1.5** orchestrator transparency, **When** **`GET /api/display/modes`** responds, **Then** retain **`orchestrator_state`** and **`state_reason`** (or equivalent) **if** **`ModeOrchestrator`** remains in use — do not regress dashboard behavior; ordering inside **`data`** is flexible

4. **Given** **NFR P4** (**<10ms**, **zero heap allocation** for enumeration path), **When** implementing **`GET`**, **Then** avoid **`String`** concatenation in the hot path, prefer **`StaticJsonDocument`** with **documented capacity**, or pre-sized buffer; **do not** allocate **`DisplayMode`** instances per request

5. **Given** **`POST /api/display/mode`**, **When** the body is valid JSON, **Then** accept **`{ "mode": "<id>" }`** per **architecture Decision D5**; **also accept** legacy **`mode_id`** for one release if **`mode`** is absent (backward compatibility with **`dl-1.5`**)

6. **Given** **Rule 24** (orchestrator owns user intent), **When** **`POST`** handles a **manual** switch from the dashboard, **Then** **`ModeOrchestrator::onManualSwitch()`** (or current approved entry point) is invoked — **do not** call **`ModeRegistry::requestSwitch()`** directly from **`WebPortal`** for manual user actions

7. **Given** **architecture D5** “synchronous” semantics vs **cooperative** **`ModeRegistry`**, **When** **`POST`** completes, **Then** the response **must** expose **truthful** registry state: at minimum **`data.switching_to`**, **`data.active`** (or **`active_mode`** alias in **`data`**), **`data.switch_state`**, and **`data.registry_error`** / **`ModeRegistry::getLastError()`** when **`requestSwitch`** fails inside orchestrator path. **If** a **bounded wait** until **`SwitchState::IDLE`** is implemented, cap **≤2000ms**, **`yield`**, and document **watchdog** risk; **if** full synchronous completion is **not** feasible in **`AsyncWebServer`** body handler, document **explicit deviation** and return **`switch_state: "requested"`** with **client re-fetch** (**ds-3.4** UX) — **pick one strategy** in Dev Agent Record and match **`architecture.md`** intent as closely as practicable

8. **Given** unknown mode id, **When** **`POST`** resolves before registry, **Then** **`400`**, **`{ "ok": false, "error": "...", "code": "UNKNOWN_MODE" }`** (or existing **`_sendJsonError`** pattern)

9. **Given** heap guard failure (**`ModeRegistry`** sets error), **When** surfaced through API, **Then** **`{ "ok": false, ... }`** includes a **stable** **`code`** (e.g. **`HEAP_INSUFFICIENT`**) and human-readable **error** string (**FR33**)

10. **Given** **`POST /api/display/notification/dismiss`** (or **`/api/display/ack-notification`** per **architecture**), **When** called with valid session, **Then** **`upgrade_notification`** NVS flag is cleared and **`GET`** returns **`upgrade_notification: false`** thereafter — **namespace** **`flightwall`**, key name **≤15 chars** (coordinate with **ds-3.2** if the key is introduced there; stub no-op is **not** acceptable if **GET** promises the flag)

11. **Given** **`pio run`** on **`esp32dev`**, **Then** firmware builds with **no new warnings**

## Tasks / Subtasks

- [x] Task 1: `ModeEntry` + registry (AC: #2, #4)
  - [x] 1.1: Extend **`ModeEntry`** with **`const ModeZoneDescriptor* zoneDescriptor`** (nullable)
  - [x] 1.2: Point each **`MODE_TABLE`** row at **`&ClassicCardMode::_descriptor`**, **`&LiveFlightCardMode::_descriptor`**, and **clock** mode descriptor when **`ClockMode`** exists (**dl-1.1**) — or **`nullptr`** + **empty** **`zone_layout`** until clock ships
  - [x] 1.3: Verify **`extern` linkage** / **getter** if private statics are inaccessible from **`main.cpp`**

- [x] Task 2: `GET /api/display/modes` (AC: #1, #3, #4, #10)
  - [x] 2.1: Replace hardcoded **`JsonObject mClassic`** blocks in **`_handleGetDisplayModes`** with loop over **`ModeRegistry`**
  - [x] 2.2: Emit **`zone_layout`** from **`ModeZoneDescriptor`**
  - [x] 2.3: Set **`switch_state`** from **`ModeRegistry::getSwitchState()`**
  - [x] 2.4: Implement **`upgrade_notification`** NVS read (or shared helper with **ds-3.2**)

- [x] Task 3: `POST /api/display/mode` (AC: #5–#9)
  - [x] 3.1: Parse **`mode`** with fallback to **`mode_id`**
  - [x] 3.2: Orchestrator-only path; propagate **registry** errors into JSON if orchestrator exposes them
  - [x] 3.3: Resolve **sync vs async** response per AC #7 and document

- [x] Task 4: Notification dismiss route (AC: #10)
  - [x] 4.1: Register **`POST /api/display/notification/dismiss`** or **`/api/display/ack-notification`** (match **architecture** / **epic-ds-3** — one canonical path)
  - [x] 4.2: Clear NVS flag; return **`{ ok: true }`**

- [x] Task 5: Assets / consumers (AC: #11)
  - [x] 5.1: If JSON field names change (**`active`** vs **`active_mode`**), update **`data-src/dashboard.js`** in same story **or** log follow-up for **ds-3.3** — avoid breaking **`dl-1.5`** dashboard

#### Review Follow-ups (AI)
<!-- All verified issues fixed during synthesis — no open items. -->

## Dev Notes

### Existing code

- **`WebPortal::_handleGetDisplayModes`** — **`firmware/adapters/WebPortal.cpp`** ~915+ (**hardcoded** modes, **`switch_state`** stub **`"idle"`**)
- **`POST /api/display/mode`** — **`mode_id`**, **`ModeOrchestrator::onManualSwitch`**
- **`ConfigManager::getDisplayMode` / `setDisplayMode`** — **`disp_mode`** key (may differ from **architecture** **`display_mode`** — **align** with **ds-3.2** naming)

### Epic ↔ architecture naming

- Epic snippet uses top-level **`modes`** — product uses **`data.modes`** — follow **product** [Source: **dl-1.5** implementation]
- **`getZoneLayout()`** in epic → **`getZoneDescriptor()`** / **`zone_layout`** JSON [Source: **`interfaces/DisplayMode.h`**]

### Out of scope (later ds-3 stories)

- **ds-3.2** — full NVS boot restore policy if not already complete
- **ds-3.3** — Mode Picker cards / CSS grid
- **ds-3.4** — switching animation UX
- **ds-3.6** — banner UI (may depend on **`upgrade_notification`** from this story)

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.1)
- Architecture: `_bmad-output/planning-artifacts/architecture.md` — **D5**, **D6**, **Rule 24**
- Prior: **`dl-1-5`** story file (`orchestrator-state-feedback`)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

### Implementation Plan

**Sync vs Async Strategy (AC #7):** Chose **async response** with `switch_state: "requested"` and client re-fetch (ds-3.4 UX pattern). ESPAsyncWebServer body handlers run on the AsyncTCP task while `ModeRegistry::tick()` runs on Core 0 display task — a bounded wait would block the web server and risk watchdog timeout. This is an explicit deviation from architecture D5 "synchronous" intent, documented per AC #7.

**NVS key for upgrade_notification (AC #10):** `upg_notif` in namespace `flightwall` (8 chars, within 15-char limit). Coordinated for ds-3.2 reuse.

**Active mode fallback:** When `ModeRegistry::getActiveModeId()` returns `nullptr` (before first tick), falls back to `"classic_card"` string literal.

### Completion Notes List

- Task 1: Added `const ModeZoneDescriptor* zoneDescriptor` to `ModeEntry` struct. Made `_descriptor` public in both ClassicCardMode and LiveFlightCardMode so main.cpp can reference `&ClassicCardMode::_descriptor` etc. Updated MODE_TABLE and all test mode tables.
- Task 2: Rewrote `_handleGetDisplayModes` to loop over `ModeRegistry::getModeTable()` instead of hardcoded mode blocks. Emits `zone_layout` array from static `ModeZoneDescriptor`, maps `SwitchState` enum to string, reads `upgrade_notification` from NVS, retains `orchestrator_state`/`state_reason` for dl-1.5 compatibility, surfaces `registry_error` when present.
- Task 3: Updated POST handler to accept `mode` field with fallback to `mode_id` (AC #5). Mode name resolution now uses ModeRegistry lookup instead of hardcoded if-chain. Response includes truthful `active`, `switch_state`, `orchestrator_state`, `state_reason`, and `registry_error` fields.
- Task 4: Registered `POST /api/display/notification/dismiss` route that clears NVS `upg_notif` flag and returns `{ ok: true }`.
- Task 5: No dashboard.js changes needed — all existing field names (`active`, `modes`, `state_reason`, `orchestrator_state`) preserved. `mode_id` POST field still accepted via fallback. New fields (`zone_layout`, `switch_state`, `upgrade_notification`, `registry_error`) are additive-only.

### File List

- `firmware/core/ModeRegistry.h` — Added `zoneDescriptor` field to `ModeEntry` struct
- `firmware/modes/ClassicCardMode.h` — Made `_descriptor` public for MODE_TABLE access
- `firmware/modes/LiveFlightCardMode.h` — Made `_descriptor` public for MODE_TABLE access
- `firmware/src/main.cpp` — Updated MODE_TABLE entries with descriptor pointers
- `firmware/adapters/WebPortal.cpp` — Rewrote GET /api/display/modes, updated POST /api/display/mode, added POST /api/display/notification/dismiss, added ModeRegistry.h include
- `firmware/test/test_mode_registry/test_main.cpp` — Updated test mode tables with zoneDescriptor field

## Change Log

- **2026-04-14:** Replaced hardcoded display mode catalog with ModeRegistry-driven API. Added zone_layout metadata, real switch_state, upgrade_notification NVS flag, and notification dismiss endpoint. POST now accepts both `mode` and `mode_id` fields. All existing test suites build cleanly, no new warnings.

## Previous story intelligence

- **dl-1.5** shipped **GET/POST** scaffolding; **ds-3.1** replaces **stub** catalog and **`switch_state`** with **`ModeRegistry`** truth.
- **ModeOrchestrator** integration must stay **Rule-24**-compliant.

## Git intelligence summary

Primary edits: **`firmware/core/ModeRegistry.h`**, **`firmware/src/main.cpp`**, **`firmware/adapters/WebPortal.cpp`**.

## Latest tech information

- **ArduinoJson** on ESP32: prefer **`StaticJsonDocument<N>`** with **capacity** sized for worst-case mode list.

## Project context reference

After **`data-src/`** edits, regenerate **`.gz`** per **`AGENTS.md`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-14
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 6.8 → REJECT
- **Issues Found:** 3 verified (2 dismissed as false positives)
- **Issues Fixed:** 3
- **Action Items Created:** 0

#### Review Follow-ups (AI)
<!-- All verified issues were fixed during synthesis. No open action items. -->


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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-3-1-display-mode-api-endpoints.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 2 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task 3.3 / AC 9 Violation - API Contract Failure for Errors | `WebPortal.cpp` | +3 |
| 🔴 CRITICAL | Hidden Bug / Race Condition - `switch_state` never reports "requested" | `ModeRegistry.cpp` | +3 |
| 🔴 CRITICAL | Hidden Bug - Partial Apply of Mode Settings Corrupts NVS | `WebPortal.cpp` | +3 |
| 🟠 IMPORTANT | State Desync between Orchestrator and Registry | `WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | Performance Footgun - O(N) String Concatenation in AsyncTCP Task | `WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | Performance Footgun - Large `String` JSON Allocation | `WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | SOLID Violation - God Method in WebPortal | `WebPortal.cpp` | +1 |
| 🟢 CLEAN PASS | 4 (Security, Style, Lying Tests, Type Safety) | | -2.0 |

### Evidence Score: 11.0

| Score | Verdict |
|-------|---------|
| **11.0** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] Single Responsibility:** `WebPortal::_handlePostDisplayMode` is doing far too much. It parses HTTP JSON, looks up modes in the Registry, parses nested settings, iterates over mode schemas, validates settings types/ranges, writes directly to ConfigManager, invokes the Orchestrator, and builds a complex JSON response.
  - 📍 `firmware/adapters/WebPortal.cpp:580`
  - 💡 Fix: Extract the validation and application of mode-specific settings into a dedicated `ConfigManager::applyModeSettings(abbrev, json)` method to keep the HTTP adapter focused purely on HTTP concerns.

- **State Desync:** When `POST /api/display/mode` is called, `ModeOrchestrator::onManualSwitch()` records the requested mode in NVS as `_manualModeId`. If `ModeRegistry` subsequently rejects the switch (e.g., due to the heap guard), the Orchestrator still considers the rejected mode as the user's manual preference.
  - 📍 `firmware/adapters/WebPortal.cpp:640`
  - 💡 Fix: The Orchestrator's state will automatically attempt to switch back to the rejected mode when returning from `IDLE_FALLBACK`, causing repeated silent failures. The Orchestrator should only update NVS *after* the registry confirms the switch, or the UI must handle the desync gracefully.

✅ No significant Pythonic/Style violations detected.

---

## ⚡ Performance & Scalability

- **[High] O(N) String Concatenation in AsyncTCP Task:** In all `POST` body handlers (`_handlePostSettings`, `_handlePostDisplayMode`, `_handlePostSchedule`), the chunk accumulation loops character by character. Appending a single char to an Arduino `String` in a tight loop causes repeated bounds checks and function overhead on the single-threaded AsyncTCP task, blocking network processing.
  - 📍 `firmware/adapters/WebPortal.cpp:160`
  - 💡 Fix: Use `pending->body.concat(reinterpret_cast<const char*>(data), len);` to append the entire chunk at once.

- **[Medium] Large String Allocation for JSON Responses:** In `_handleGetDisplayModes`, the code serializes the entire mode catalog (which includes metadata and settings schemas, easily exceeding 1-2KB) into a single contiguous `String output;` before calling `request->send(200, "application/json", output);`. This risks severe heap fragmentation on the ESP32.
  - 📍 `firmware/adapters/WebPortal.cpp:570`
  - 💡 Fix: Use `AsyncResponseStream *response = request->beginResponseStream("application/json"); serializeJson(doc, *response); request->send(response);` to stream the payload chunk-by-chunk.

---

## 🐛 Correctness & Safety

- **🐛 Bug (Breaks AC 7 UX):** The dev agent chose an async strategy (return `switch_state: "requested"` and have the client poll) to satisfy AC 7. However, `ModeRegistry::requestSwitch()` does NOT set `_switchState` to `REQUESTED`; it only updates `_requestedIndex`. The state is only briefly updated in `tick()` on Core 0. Thus, the POST response almost always reads `IDLE` and returns `switch_state: "idle"` instead of `"requested"`.
  - 📍 `firmware/core/ModeRegistry.cpp:67`
  - 🔄 Reproduction: Call `POST /api/display/mode`. Observe the response contains `"switch_state": "idle"`. The dashboard UI will instantly terminate its polling loop, thinking the switch is already complete.

- **🐛 Bug (Data Corruption):** In `_handlePostDisplayMode`, the code pre-validates that setting *keys* exist in the schema, but fails to pre-validate their *values*. It applies values sequentially using `ConfigManager::setModeSetting`. If a setting value fails validation (e.g., out-of-range integer) and returns `false`, the loop aborts and returns a 400 error. However, any settings processed *before* the failed one have already been written to NVS!
  - 📍 `firmware/adapters/WebPortal.cpp:630`
  - 🔄 Reproduction: Send a POST request with two settings, where the first is valid and the second is invalid. Observe that the API returns 400, but the first setting is permanently saved to NVS, violating the explicit `(no partial apply)` requirement.

- **🎭 Lying Task / AC 9 Gap:** AC 9 requires heap guard failures to surface with `{ "ok": false, ... }` and a stable code. Because the dev agent made the POST endpoint asynchronous, it surfaces the error during the `GET /api/display/modes` polling loop. However, it returns `{ "ok": true, "data": { "registry_error": "..." } }`. This violates the standard API envelope (`ok: false` for errors) mandated by the architecture and AC 9, completely breaking client error handling.
  - 📍 `firmware/adapters/WebPortal.cpp:560`
  - 🤥 Why it lies: Task 3.3 claims to resolve the async response strategy correctly, but silently buries critical system errors inside successful 200 OK responses.

✅ Code appears secure with no obvious injection or memory overflow vulnerabilities.

---

## 🔧 Maintainability Issues

✅ Code is maintainable and well-documented.

---

## 🛠️ Suggested Fixes

### 1. Fix Race Condition in Switch State

**File:** `firmware/core/ModeRegistry.cpp`
**Issue:** `requestSwitch` fails to update `_switchState` to `REQUESTED`, breaking the API response.

**Corrected code:**
```cpp
bool ModeRegistry::requestSwitch(const char* modeId) {
    uint8_t idx = findIndex(modeId);
    if (idx == MODE_INDEX_NONE) {
        _errorUpdating.store(true);
        snprintf(_lastError, sizeof(_lastError), "Unknown mode: %.40s",
                 modeId ? modeId : "(null)");
        _errorUpdating.store(false);
        LOG_W("ModeRegistry", "Switch request rejected");
#if LOG_LEVEL >= 1
        Serial.printf("[ModeRegistry] %s\n", _lastError);
#endif
        return false;
    }

    // Latest-wins: atomic store overwrites any pending request
    _requestedIndex.store(idx);
    
    // FIX: Set state to REQUESTED immediately so the async API returns truthful state
    _switchState.store(SwitchState::REQUESTED);
    
    LOG_I("ModeRegistry", "Switch requested");
#if LOG_LEVEL >= 2
    Serial.printf("[ModeRegistry] Target: %s (index %d)\n", modeId, (int)idx);
#endif
    return true;
}
```

### 2. Fix O(N) String Concatenation in Body Handlers

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** Character-by-character string concatenation causes significant CPU overhead and blocks the AsyncTCP thread.

**Diff:**
```diff
-            for (size_t i = 0; i < len; ++i) {
-                pending->body += static_cast<char>(data[i]);
-            }
+            pending->body.concat(reinterpret_cast<const char*>(data), len);
```
*(Apply to `_handlePostSettings`, `_handlePostDisplayMode`, and `_handlePostSchedule`)*

**Review Actions:**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

I'm ready to help. What would you like me to do?

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
<var name="session_id">782e492a-50be-4086-bbf5-d9afb690c969</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="99900e09">embedded in prompt, file id: 99900e09</var>
<var name="story_id">ds-3.1</var>
<var name="story_key">ds-3-1-display-mode-api-endpoints</var>
<var name="story_num">1</var>
<var name="story_title">1-display-mode-api-endpoints</var>
<var name="template">False</var>
<var name="timestamp">20260414_1922</var>
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
      - Commit message format: fix(component): brief description (synthesis-ds-3.1)
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