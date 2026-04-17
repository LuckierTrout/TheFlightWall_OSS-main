<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: ds-3 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260414T230827Z -->
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

Status: review

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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

I'm ready to help. What would you like me to do?

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/ds-3-1-display-mode-api-endpoints.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 0 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Chunked JSON payloads are silently corrupted/ignored | firmware/adapters/WebPortal.cpp:351 | +3 |
| 🔴 CRITICAL | Orchestrator bypassed for manual mode selection | firmware/adapters/WebPortal.cpp:391 | +3 |
| 🟠 IMPORTANT | Async POST decision breaks UI polling state | firmware/adapters/WebPortal.cpp:413 | +1 |
| 🟠 IMPORTANT | Heap guard failures buried in 200 OK | firmware/adapters/WebPortal.cpp:425 | +1 |
| 🟠 IMPORTANT | N+1 NVS reads violate <10ms enumeration budget | firmware/adapters/WebPortal.cpp:1228 | +1 |
| 🟡 MINOR | Duplicated SwitchState string mapping | firmware/adapters/WebPortal.cpp:414 | +0.3 |
| 🟢 CLEAN PASS | 5 |

### Evidence Score: 6.8

| Score | Verdict |
|-------|---------|
| **6.8** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

- **[High] Performance Footgun:** N+1 NVS reads violate <10ms enumeration budget (NFR P4)
  - 📍 `firmware/adapters/WebPortal.cpp:1228`
  - 💡 Fix: `_handleGetDisplayModes` iterates over every mode and its settings, calling `ConfigManager::getModeSetting()` for each. This triggers `Preferences::begin()` and `end()` repeatedly in a tight loop. With 4 modes and 10 settings, this incurs 20-50ms of blocking flash I/O latency, completely blowing the `<10ms` enumeration budget (AC 4) for an endpoint polled at 500ms intervals. Cache active settings in RAM or implement a batch-read method.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** Chunked JSON payloads are silently corrupted/ignored
  - 📍 `firmware/adapters/WebPortal.cpp:351`
  - 🔄 Reproduction: Send a `POST /api/display/mode` request with a JSON body larger than the TCP MTU (~1460 bytes) containing a large settings object. The `ESPAsyncWebServer` body handler ignores all initial chunks because `index + len == total` is false. When the final chunk arrives, it parses *only* the final chunk's `data` pointer. `deserializeJson()` fails with a ParseError and returns 400 Invalid JSON. Reassemble chunks using the `g_pendingBodies` pattern.

- **🐛 Bug:** Orchestrator bypassed for manual mode selection
  - 📍 `firmware/adapters/WebPortal.cpp:391`
  - 🔄 Reproduction: Wait for the system to enter `SCHEDULED` or `IDLE_FALLBACK` state. In the dashboard, explicitly click the currently active mode to lock it in as a `MANUAL` choice. The `!isSameMode` check evaluates to false, skipping `ModeOrchestrator::onManualSwitch()`. The orchestrator never transitions to `MANUAL` state, discarding the user's explicit intent (Violates AC 6 and Rule 24).

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Async POST decision breaks AC 7 and AC 9 API contracts
  - 📍 `firmware/adapters/WebPortal.cpp:413`
  - 💥 Explosion radius: High. The developer explicitly chose an async response strategy to avoid blocking. However: 1) They map `SwitchState::IDLE` to `"idle"` immediately because `tick()` hasn't run yet, which breaks the UI's polling transition state (AC 7). 2) Because the HTTP response is sent before the heap guard executes on Core 0, a heap failure cannot trigger the required `{"ok": false, "code": "HEAP_INSUFFICIENT"}` payload (AC 9). The error is instead buried in `data.registry_error` of a `200 OK` response.

- **💣 Tech Debt:** Duplicated SwitchState string mapping
  - 📍 `firmware/adapters/WebPortal.cpp:414`
  - 💥 Explosion radius: Low. The mapping from `SwitchState` to "idle"/"requested"/"switching" is copy-pasted in `_handleGetDisplayModes` and `_handlePostDisplayMode`. Extract this to `ModeRegistry::getSwitchStateString()`.

---

## 🛠️ Suggested Fixes

### 1. Fix Chunked Payload Corruption & Orchestrator Bypass

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** `POST /api/display/mode` ignores initial chunks of large payloads and incorrectly prevents the orchestrator from processing user intent.

**Corrected code:**
```cpp
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (total == 0 || data == nullptr) {
                _sendJsonError(request, 400, "Empty request body", "EMPTY_PAYLOAD");
                return;
            }
            if (total > MAX_SETTINGS_BODY_BYTES) {
                clearPendingBody(request);
                _sendJsonError(request, 413, "Request body too large", "BODY_TOO_LARGE");
                return;
            }

            if (index == 0) {
                clearPendingBody(request);
                PendingRequestBody pending{request, String()};
                pending.body.reserve(total);
                g_pendingBodies.push_back(pending);
                request->onDisconnect([request]() {
                    clearPendingBody(request);
                });
            }

            PendingRequestBody* pending = findPendingBody(request);
            if (pending == nullptr) {
                _sendJsonError(request, 400, "Incomplete request body", "INCOMPLETE_BODY");
                return;
            }

            for (size_t i = 0; i < len; ++i) {
                pending->body += static_cast<char>(data[i]);
            }

            if (index + len == total) {
                JsonDocument reqDoc;
                DeserializationError err = deserializeJson(reqDoc, pending->body);
                clearPendingBody(request);

                if (err) {
                    _sendJsonError(request, 400, "Invalid JSON", "INVALID_JSON");
                    return;
                }
                
                // ... fallback parsing logic & settings logic remains the same ...

                // AC #6 / Rule 24: switch MUST ALWAYS go via orchestrator
                // Remove the `!isSameMode` check to ensure the orchestrator exits fallback/scheduled states.
                ModeOrchestrator::onManualSwitch(modeId, matchedEntry->displayName);

                // ... continue to response ...
```

### 2. Implement Bounded Wait for AC 7 / AC 9 Compliance

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** Returning asynchronously breaks the UI transition polling and hides heap guard errors inside HTTP 200 responses.

**Corrected code:**
```cpp
                // Bounded wait for switch completion (up to 2000ms) to satisfy AC 7 and AC 9
                unsigned long startWait = millis();
                while (ModeRegistry::getSwitchState() != SwitchState::IDLE && (millis() - startWait) < 2000) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                // Check for registry errors (AC 9)
                const char* lastErr = ModeRegistry::getLastError();
                if (lastErr != nullptr && lastErr[0] != '\0') {
                    _sendJsonError(request, 400, lastErr, "REGISTRY_ERROR");
                    return;
                }

                // Build truthful response
                JsonDocument doc;
                JsonObject root = doc.to<JsonObject>();
                root["ok"] = true;
                JsonObject respData = root["data"].to<JsonObject>();
                respData["switching_to"] = modeId;
                respData["active"] = ModeRegistry::getActiveModeId();
                respData["settings_applied"] = settingsApplied;
                respData["switch_state"] = "idle";
                respData["orchestrator_state"] = ModeOrchestrator::getStateString();
                respData["state_reason"] = ModeOrchestrator::getStateReason();
                
                String output;
                serializeJson(doc, output);
                request->send(200, "application/json", output);
```

**Review Actions:**
- Issues Found: 6
- Issues Fixed: 0
- Action Items Created: 0

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
<var name="session_id">9dd2d0b2-35f0-4fca-b3c0-9de0df45af3d</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="99900e09">embedded in prompt, file id: 99900e09</var>
<var name="story_id">ds-3.1</var>
<var name="story_key">ds-3-1-display-mode-api-endpoints</var>
<var name="story_num">1</var>
<var name="story_title">1-display-mode-api-endpoints</var>
<var name="template">False</var>
<var name="timestamp">20260414_1908</var>
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