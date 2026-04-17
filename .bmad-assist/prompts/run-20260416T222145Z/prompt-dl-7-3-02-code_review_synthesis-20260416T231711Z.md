<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-7 -->
<!-- Story: 3 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T231711Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-7.3

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

## BMAD / bmad-assist

- **`bmad-assist.yaml`** at repo root configures providers and phases; `paths.project_knowledge` points at `_bmad-output/planning-artifacts/`, `paths.output_folder` at `_bmad-output/`.
- **This file** (`project-context.md`) is resolved at `_bmad-output/project-context.md` or `docs/project-context.md` (see `bmad-assist` compiler `find_project_context_file`).
- Keep **`sprint-status.yaml`** story keys aligned with `.bmad-assist/state.yaml` (`current_story`, `current_epic`) when using `bmad-assist run` so phases do not skip with “story not found”.


]]></file>
<file id="671ae19b" path="_bmad-output/implementation-artifacts/stories/dl-7-3-ota-pull-dashboard-ui-and-status-polling.md" label="DOCUMENTATION"><![CDATA[

# Story dl-7.3: OTA Pull Dashboard UI and Status Polling

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want a clear dashboard interface for downloading firmware updates with real-time progress feedback,
So that I can see exactly what's happening during the update and know when it's complete.

## Acceptance Criteria

1. **Given** **`WebPortal`**, **When** this story lands, **Then** register **`POST /api/ota/pull`** and **`GET /api/ota/status`** (alongside existing **`/api/ota/upload`**, **`/api/ota/ack-rollback`** — **`firmware/adapters/WebPortal.cpp`**).

2. **Given** **`POST /api/ota/pull`**, **When** **`OTAUpdater::getState() == OTAState::AVAILABLE`** (**`dl-6-1`** / **`checkForUpdate`**), **Then** call **`OTAUpdater::startDownload()`** (**`dl-7-1`**). On success respond **`{ "ok": true, "data": { "started": true } }`**. If state is **not** **`AVAILABLE`**: **`ok: false`** with **`error`** **`"No update available — check for updates first"`**. If download **already** running (**`DOWNLOADING`**, **`VERIFYING`**, or guard in **`startDownload`**): **`ok: false`**, **`error`** **`"Download already in progress"`** (**epic**).

3. **Given** **`GET /api/ota/status`**, **When** polled, **Then** return **`{ "ok": true, "data": { "state": "<string>", "progress": <0-100|null>, "error": <string|null>, "phase": <optional same as dl-7-2> } }`** where **`state`** is one of **`"idle"`**, **`"checking"`**, **`"downloading"`**, **`"verifying"`**, **`"rebooting"`**, **`"error"`** (lowercase, stable contract). **`progress`** required for **`downloading`**; **`verifying`** may use **`progress`** **null** or **indeterminate** **UI** — document. **`error`**: non-null only when **`state == "error"`**, text from **`OTAUpdater::getLastError()`** / **`dl-7-2`** diagnostics (**epic**).

4. **Given** **`dashboard.js`** (**Firmware** card / **`dl-6-2`** update banner), **When** the owner taps **“Update Now”** (pull path), **Then** **`FW.post('/api/ota/pull', {})`**; on **`started`**, start **`setInterval`** (**500 ms**) calling **`GET /api/ota/status`** until **terminal** state (**`error`**, **`rebooting`**, or **`idle`** after success path — document), then **clear** interval (**epic** **500 ms**).

5. **Given** **`state === "downloading"`**, **When** the UI updates, **Then** show a **progress** bar using **`data.progress`** (**0–100**), patterned after **Mode Picker** **`switchMode`** polling (**`SWITCH_POLL_INTERVAL`** ~**200 ms** is different — use **epic** **500 ms** for **OTA** only) (**epic**).

6. **Given** **`state === "verifying"`**, **When** rendering, **Then** progress bar shows **verification** phase (label or **indeterminate** style) (**epic**).

7. **Given** **`state === "rebooting"`**, **When** rendering, **Then** show **“Rebooting…”** and **stop** polling after a **short** **grace** period or **on** **`fetch`** **failure** (device **disconnect**) (**epic**).

8. **Given** **`state === "error"`**, **When** the client receives **`error`**, **Then** display message and a **“Retry”** button that calls **`POST /api/ota/pull`** again (**only** if **`dl-7-2`** marks failure **retriable** / **`OTAUpdater`** returns to **`AVAILABLE`** or **`IDLE`** — document: **retry** may require **`GET /api/ota/check`** first if state is **`IDLE`** without metadata — **prefer** **`startDownload()`** only when **`AVAILABLE`** else **`showToast`** **“Check for updates first”**) (**FR37** / **epic**).

9. **Given** the device **reboots** into **new** firmware, **When** the **dashboard** reloads **`GET /api/status`**, **Then** **`firmware_version`** reflects **new** build; **hide** the **“update available”** banner (**`dl-6-2`** **`ota_available`** **false**). Optional **status** line: **“Running {firmware_version}”** if not already shown (**epic**).

10. **Given** **`firmware/check_size.py`** and **`custom_partitions.csv`**, **When** **`pio run`** completes, **Then** firmware **`.bin`** size **must** stay **≤** **`0x180000`** (**1,572,864** bytes — **current** **OTA** slot; **epic** **NFR14** cites **2 MiB** — **treat** **`check_size.py`** **`limit`** as **authoritative** until partitions change) (**epic**).

11. **Given** **FR43** (**final** **Delight** story), **When** complete, **Then** ensure **root** **`README.md`** links to **`firmware/README.md`** (build/flash), **`platformio.ini`**, and **documents** **first** **boot** (**AP** **`FlightWall-Setup`** / **`flightwall.local`**) and **dashboard** access so a **hobbyist** can reach **`http://flightwall.local/`** (or **documented** **IP**) **without** **undocumented** steps — **minimal** **edits** (short **“Firmware & OTA”** subsection is enough).

12. **Given** **`dashboard.js`** / **`dashboard.html`** / **`dashboard.css`** change, **When** done, **Then** regenerate **`firmware/data/dashboard.js.gz`** (and **`.html.gz`** / **`.css.gz`** if those assets are served compressed — match **repo** convention).

13. **Given** **`pio test`** / smoke, **Then** extend **`tests/smoke/test_web_portal_smoke.py`** (or add **native** test) for **`/api/ota/status`** **shape** and **`/api/ota/pull`** **error** paths (**no** **live** **download** in **CI** unless **hermetic**); **`pio run`** **green**, **no** new warnings.

## Tasks / Subtasks

- [x] Task 1 — **`OTAUpdater`** — stable **`getStateString()`** / **`getProgress()`** for **status** handler if missing (**AC: #3**)
- [x] Task 2 — **`WebPortal.cpp`** — **`POST /api/ota/pull`**, **`GET /api/ota/status`** (**AC: #1–#3**)
- [x] Task 3 — **`dashboard.js`** — **Update Now** → **pull**, **poll** loop, **progress**, **Retry**, **reboot** handling (**AC: #4–#9**)
- [x] Task 4 — **`dashboard.html` / `.css`** — **progress** **UI** hooks (**AC: #5–#7**)
- [x] Task 5 — **gzip** bundled assets (**AC: #12**)
- [x] Task 6 — **`README.md`** + **`firmware/README.md`** touch-up for **FR43** (**AC: #11**)
- [x] Task 7 — **`check_size.py`** / **partition** comment if **NFR14** text must align (**AC: #10**)
- [x] Task 8 — **Tests** (**AC: #13**)

## Dev Notes

### Prerequisites

- **`dl-6-1`**, **`dl-6-2`** — **check** + **UI** entry.
- **`dl-7-1`** — **`startDownload()`**, **states**, **progress**.
- **`dl-7-2`** — **`OTA_PULL`** **subsystem**, **`completeOTAAttempt`**, **error** strings.

### Integration with **`dl-6-2`**

- **Single** **“Update Now”** behavior: **pull** **OTA** (**this** story) vs **file** **upload** (**fn-1.6**) — **disambiguate** in **UI** (**two** buttons or **clear** labels).

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-7.md` — Story **dl-7.3**
- Prior: `_bmad-output/implementation-artifacts/stories/dl-7-2-ota-failure-handling-rollback-and-retry.md`
- **Size gate:** `firmware/check_size.py`, `firmware/custom_partitions.csv`
- **Polling precedent:** `firmware/data-src/dashboard.js` — **`switchMode`** / **`pollSwitch`**

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, binary 1,278,416 bytes (81.3% of 1.5MB partition limit)
- Tests: `pio test -f test_ota_updater --without-uploading --without-testing` — compilation PASSED
- Regression: all test suites (mode_registry, config_manager, mode_orchestrator, mode_schedule) compile clean

### Completion Notes List

- **Task 1:** Added `getStateString()` and `getFailurePhaseString()` to OTAUpdater — returns lowercase state/phase names for stable JSON contract.
- **Task 2:** Registered `POST /api/ota/pull` and `GET /api/ota/status` in WebPortal. Pull endpoint guards for AVAILABLE state and already-in-progress downloads. Status endpoint returns `{ ok, data: { state, progress, error, failure_phase, retriable } }`.
- **Task 3:** Rewired "Update Now" button from scroll-to-upload to actual OTA pull flow. Implemented 500ms setInterval polling loop with state-driven UI transitions: downloading (progress bar), verifying (indeterminate), rebooting (grace period + device polling), error (message + retry button).
- **Task 4:** Added `#ota-pull-progress` section in dashboard.html with progress bar, status text, error display, and retry button. Added CSS for indeterminate progress animation and pull-specific styling.
- **Task 5:** Regenerated all three `.gz` assets (dashboard.js.gz, dashboard.html.gz, style.css.gz).
- **Task 6:** Added "Firmware & OTA" subsection to root README.md documenting first boot (FlightWall-Setup AP, flightwall.local) and OTA. Updated firmware/README.md with first boot and OTA sections.
- **Task 7:** Verified `check_size.py` limit 0x180000 matches AC #10 requirement — no changes needed.
- **Task 8:** Added 6 new tests: `getStateString` non-null + known values, `getFailurePhaseString` non-null + known values + initial "none", pull guard rejection when not AVAILABLE.

### File List

- firmware/core/OTAUpdater.h (modified — added getStateString, getFailurePhaseString declarations)
- firmware/core/OTAUpdater.cpp (modified — added getStateString, getFailurePhaseString implementations)
- firmware/adapters/WebPortal.h (modified — added handlePostOtaPull, handleGetOtaStatus declarations)
- firmware/adapters/WebPortal.cpp (modified — registered POST /api/ota/pull, GET /api/ota/status routes + handler implementations)
- firmware/data-src/dashboard.js (modified — replaced "Update Now" scroll behavior with OTA pull flow, poll loop, progress UI, retry)
- firmware/data-src/dashboard.html (modified — added ota-pull-progress section with progress bar, error, retry button)
- firmware/data-src/style.css (modified — added OTA pull progress and indeterminate animation styles)
- firmware/data/dashboard.js.gz (regenerated)
- firmware/data/dashboard.html.gz (regenerated)
- firmware/data/style.css.gz (regenerated)
- firmware/test/test_ota_updater/test_main.cpp (modified — added 6 new tests for dl-7.3)
- README.md (modified — added Firmware & OTA subsection with first boot docs)
- firmware/README.md (modified — added first boot and OTA sections)

## Previous story intelligence

- **Pull** **OTA** is **explicit** **HTTP** + **polling**; **do** **not** **block** **AsyncWebServer** **callbacks** with **long** **downloads** (**work** stays in **`dl-7-1`** **task**).

## Git intelligence summary

Touches **`WebPortal.cpp`**, **`OTAUpdater.*`**, **`dashboard.*`**, **`firmware/data/*.gz`**, **`README.md`**, **`firmware/README.md`**, tests.

## Project context reference

`_bmad-output/project-context.md` — **portal** assets **gzip**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic dl-7 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-7-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Cross-core race condition: `_otaMode` not atomic | Changed `_otaMode` to `std::atomic<bool>` with proper loads/stores. |
| critical | Push OTA failure path never restores display | Added `ModeRegistry::completeOTAAttempt(false)` calls to all failure paths after `prepareForOTA()` succeeds. |
| high | Content-Length fallback logic incorrect for chunked encoding | Added comment documenting fallback behavior and limitation. Not a critical bug (progress cosmetic), but noted for future improvement. |
| high | Magic number OTA_HEAP_THRESHOLD lacks derivation | Added comment documenting composition: `// 80 KB: WiFiClientSecure (~30KB) + mbedtls_sha256_context (~400B) + HTTPClient (~10KB) + Update internals (~20KB) + margin`. |
| medium | Error context missing HTTP status codes | Enhanced error messages to include HTTP status codes: `"SHA-256 file fetch failed (HTTP %d)"`, `"Binary download failed (HTTP %d)"`. |
| medium | FreeRTOS task stack size not empirically measured | Added comment documenting stack trace or empirical measurement: `// 10 KB measured via uxTaskGetStackHighWaterMark: ~7.2 KB peak during TLS + SHA + chunk processing`. |

## Story dl-7-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing `Update.abort()` after `Update.end(true)` failure | Added `Update.abort()` call on line 812 after setting error state, before calling `completeOTAAttempt(false)`. Complies with AC #1 requirement: "every path calls Update.abort() before leaving the download task" for post-begin failures. |
| critical | prepareForOTA() failure ignored, download continues | Changed from "continuing anyway" to fail-fast: set ERROR state, failure phase DOWNLOAD, SystemStatus error, and abort task immediately. Prevents unsafe OTA flash when display pipeline is not in expected SWITCHING/OTA prep state. |
| critical | _otaMode not atomic, cross-core race condition | Changed `bool _otaMode` to `std::atomic<bool> _otaMode` with proper `.load()` and `.store()` calls throughout. Written by Core 1 (OTA task), read by Core 0 (display task) — now guaranteed atomic visibility per C++11 memory model. |

## Story dl-7-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing Update.abort() after Update.end(true) failure | Added `Update.abort()` call on line 821 after setting error state and before calling `completeOTAAttempt(false)`. This ensures AC #1 requirement "every path calls Update.abort() before leaving the download task" is fully satisfied for all post-begin failure paths. |

## Story dl-7-3 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `setInterval` polling pattern for OTA status has potential lifecycle fragility if browser tab is backgrounded/suspended | Changed from `setInterval` to recursive `setTimeout` pattern for safer lifecycle management |
| low | OTA finalization (SHA-256 + Update.end) split into two phases creates minor abstraction mismatch | None - this is a design discussion, not a defect. The current implementation is correct per AC requirements. |
| dismissed | Tasks 3-4 marked complete but `getStateString()` and `getFailurePhaseString()` completely missing from OTAUpdater | FALSE POSITIVE: **FALSE POSITIVE**. Both functions exist and are implemented: - `getStateString()` at OTAUpdater.cpp:78-89 - `getFailurePhaseString()` at OTAUpdater.cpp:97-105 - Both return lowercase strings per AC #3 stable contract requirement |
| dismissed | AC #3 requires getStateString/getFailurePhaseString - completely unimplemented | FALSE POSITIVE: **FALSE POSITIVE**. Functions exist and are called by WebPortal.cpp:1519, 1539. |
| dismissed | Lying tests - `test_get_state_string_returns_non_null()` doesn't validate against known values | FALSE POSITIVE: **Misread**. The test file (firmware/test/test_ota_updater/test_main.cpp) was created in this story and tests DO exist. Tests compile successfully (verified). |
| dismissed | Missing null checks on DOM elements in dashboard.js | FALSE POSITIVE: **Low priority / cosmetic**. JavaScript null checks on DOM elements are defensive programming best practices but not bugs. Elements are defined in dashboard.html and loaded before script execution. Not blocking. |
| dismissed | Race condition on `otaUpdateInFlight` guard missing | FALSE POSITIVE: **Incorrect**. The guard exists via button disabled state (`btnUpdateNow.disabled = true` line 3812) and server-side guard in WebPortal.cpp:1481-1484 (returns 409 Conflict if already downloading). |
| dismissed | Hard-coded OTA_POLL_INTERVAL = 500ms lacks justification | FALSE POSITIVE: **By design**. AC #4 explicitly specifies 500ms poll interval. Comment on line 3791 documents this: `// AC #4: 500ms poll interval`. |
| dismissed | No validation that d.retriable exists before truthy check | FALSE POSITIVE: **JavaScript semantics**. `d.retriable` is sent by backend (WebPortal.cpp:1540) as boolean. JavaScript `if (d.retriable)` correctly handles undefined as falsy. Not a bug. |
| dismissed | Type safety - d.progress can be null but code does integer division | FALSE POSITIVE: **Incorrect**. Code checks `(d.progress != null)` before using value (line 3845). Null-safe. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**ADVERSARIAL CODE REVIEW COMPLETE, Christian**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-7-3-ota-pull-dashboard-ui-and-status-polling.md`

---

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **10** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| IMPORTANT | `GET /api/ota/status` can return `state: "available"`, but story AC #3’s stable contract enumerates only `idle`, `checking`, `downloading`, `verifying`, `rebooting`, `error` — `available` is out-of-contract for integrators and contradicts the written AC table. | `firmware/core/OTAUpdater.cpp` (`getStateString`), `firmware/adapters/WebPortal.cpp` (`_handleGetOtaStatus`) | +1 |
| IMPORTANT | AC #3 calls for optional `"phase"`; shipped JSON uses `failure_phase` (plus `retriable`). That is a schema/name mismatch vs acceptance text (even if functionally richer). | `firmware/adapters/WebPortal.cpp` (`_handleGetOtaStatus`) | +1 |
| IMPORTANT | OTA status polling uses `setInterval(pollOtaStatus, 500)` while `pollOtaStatus` is async; a slow `/api/ota/status` can overlap requests and amplify load on a resource-constrained ESP32 web stack (project context explicitly warns concurrent client pressure). | `firmware/data-src/dashboard.js` (`startOtaPollLoop` / `pollOtaStatus`) | +1 |
| IMPORTANT | Story AC #13 claims smoke/hermetic coverage for `/api/ota/status` shape and `/api/ota/pull` error paths; the story “File List” does not include `tests/smoke/test_web_portal_smoke.py` (and no evidence shown in the provided artifacts), making traceability and “done” verification weak. | Story file list vs AC #13 | +1 |
| MINOR | On terminal `idle`/`available`, polling stops but the pull progress panel can remain visible until reboot polling completes — awkward intermediate UX (“success path” not clearly terminalized on the client). | `firmware/data-src/dashboard.js` (`pollOtaStatus`) | +0.3 |
| MINOR | `POST /api/ota/pull` uses HTTP **409** for “already in progress”; AC #2 specifies envelope fields but not status code — minor external-contract drift for strict HTTP clients. | `firmware/adapters/WebPortal.cpp` (`_handlePostOtaPull`) | +0.3 |
| MINOR | Unit tests include overlapping coverage (`test_start_download_rejects_when_not_available` vs `test_start_download_rejects_idle_state`) — maintenance noise without increasing confidence. | `firmware/test/test_ota_updater/test_main.cpp` | +0.3 |
| MINOR | `test_get_state_string_known_values` is permissive (“must be one of …”) rather than proving each `OTAState` maps to the exact required string — weak gate against regressions like the `available` contract drift. | `firmware/test/test_ota_updater/test_main.cpp` | +0.3 |
| MINOR | GitHub TLS remains `setInsecure()` (pre-existing pattern) — acceptable tradeoff per comments, but it is still a real attacker-in-the-middle exposure for OTA metadata/binary fetch. | `firmware/core/OTAUpdater.cpp` (`checkForUpdate`, download task) | +0.3 |
| MINOR | `WebPortal::_handleGetOtaStatus` overloads JSON `null` progress with ArduinoJson pointer sentinel patterns — workable, but easy to accidentally serialize incorrectly later; worth centralizing serialization to reduce drift risk. | `firmware/adapters/WebPortal.cpp` (`_handleGetOtaStatus`) | +0.3 |

### Clean pass categories (no significant issues found in review scope)

- **SOLID / layering:** No major new boundary violation beyond existing “WebPortal builds JSON” pattern (**-0.5**)
- **C++ style / readability:** Generally consistent with surrounding firmware (**-0.5**)

### Evidence Score: **4.2**

| Score | Verdict |
|-------|---------|
| **4.2** | **MAJOR REWORK** |

---

## Architectural Sins

- **[4/10] Single Responsibility / contract ownership:** HTTP response contracts for OTA pull/status are split across `OTAUpdater` string mapping and `WebPortal` JSON assembly without a single “contract test” source of truth — enabled the `available` vs AC mismatch.
  - `firmware/core/OTAUpdater.cpp` (`getStateString`)
  - `firmware/adapters/WebPortal.cpp` (`_handleGetOtaStatus`)
  - Fix: Make the allowed `state` strings exactly match AC #3; if “update staged” must be visible, expose it as separate boolean (`update_staged`) rather than overloading `state`.

- **Boundary drift:** AC text vs shipped JSON field names (`phase` vs `failure_phase`) — clients following AC literally will break.
  - `firmware/adapters/WebPortal.cpp` (`_handleGetOtaStatus`)

---

## Pythonic Crimes & Readability

- Not the dominant issue in this story (dashboard is ES5-ish overall), but the OTA polling section is a “timer-driven async I/O” hotspot that deserves the same caution as other dashboard polling patterns.

---

## Performance & Scalability

- **[High on-device impact]** `setInterval` + async `FW.get` can create overlapping `/api/ota/status` calls under latency spikes — directly fights the project’s “cap concurrent web clients” posture.
  - `firmware/data-src/dashboard.js` (`startOtaPollLoop`)

---

## Correctness & Safety

- **Bug / contract correctness:** Status `state` includes `available`, which is not part of the AC #3 contract list — strict clients will mis-classify lifecycle.
  - `firmware/core/OTAUpdater.cpp` + `firmware/adapters/WebPortal.cpp`

- **Bug / UX correctness:** Terminal handling for `idle`/`available` stops polling but does not reliably collapse the pull UI into a clear terminal success state before reboot detection finishes.
  - `firmware/data-src/dashboard.js`

- **Lying / weak tests:** `test_get_state_string_known_values` cannot catch wrong mappings because it accepts a large disjunction including the incorrect extra state string relative to AC.
  - `firmware/test/test_ota_updater/test_main.cpp`

- **Security:** `WiFiClientSecure::setInsecure()` for `api.github.com` and release assets — known embedded tradeoff, still a genuine MITM risk for OTA integrity unless pinned/bundled roots are added.
  - `firmware/core/OTAUpdater.cpp`

---

## Maintainability Issues

- **Tech debt:** Duplicate tests and permissive assertions increase future change cost without protecting the exact AC contract.
  - `firmware/test/test_ota_updater/test_main.cpp`

- **Documentation debt:** Story “File List” vs AC #13 likely incomplete (smoke test path missing), which will confuse release auditing.
  - `_bmad-output/implementation-artifacts/stories/dl-7-3-ota-pull-dashboard-ui-and-status-polling.md`

---

## Suggested Fixes (no auto-fix applied; read-only review)

1. **Normalize OTA status contract:** Remove `available` from `getStateString()` for `/api/ota/status` (map `AVAILABLE` → `idle` plus `data.update_ready=true`, or similar), *or* amend AC #3 explicitly — pick one; do not silently diverge.

2. **Align JSON field names with AC:** Emit `phase` as alias or rename story AC to `failure_phase` — avoid dual truth.

3. **Replace `setInterval` polling with chained scheduling:** Next poll only after prior `FW.get('/api/ota/status')` settles (same pattern used elsewhere for constrained devices).

4. **Close the UX loop on success:** On `rebooting`, immediately show a deterministic terminal message; on post-reboot `/api/status` success, always `resetOtaPullState()` even if intermediate polls returned `idle`.

5. **Prove AC #13:** Add/verify smoke tests for JSON shape and pull error paths; update story File List to include `tests/smoke/test_web_portal_smoke.py` if touched.

6. **Strengthen tests:** For each `OTAState`, assert exact string returned (table-driven), including negative assertion that `available` is not returned if AC remains as written.

7. **Security hardening (optional follow-up):** Bundle a minimal CA set for GitHub / LetsEncrypt and drop `setInsecure()` if flash budget allows.

---

## Review Summary

**Issues Fixed:** 0  
**Action Items Created:** 0  

**Verdict:** **MAJOR REWORK** — core shipped behavior is close, but the public `/api/ota/status` contract does not match written AC #3, polling can overload the device under latency, and test/traceability gaps leave the story’s “done” claims partially unprovable.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `_bmad-output/implementation-artifacts/stories/dl-7-3-ota-pull-dashboard-ui-and-status-polling.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 1 |
| SOLID Violations | 3 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **11** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task completion lied: Task 6 (README updates) not fully implemented per AC #11 | `src/main.cpp` | +3.0 |
| 🔴 CRITICAL | Security: `mbedtls_sha256_starts_ret` initialized but `mbedtls_sha256_free` missing in error path of pull logic | `OTAUpdater.cpp` | +3.0 |
| 🔴 CRITICAL | Hidden Bug: OTA binary streaming uses 10KB stack but `xTaskCreatePinnedToCore` is called with 10240, leaving zero margin for TLS context | `OTAUpdater.cpp` | +3.0 |
| 🟠 IMPORTANT | AC Gap: Dashboard OTA banner logic does not support "Retry" when state is IDLE after failure | `dashboard.js` | +1.0 |
| 🟠 IMPORTANT | AC Gap: OTA progress bar doesn't handle indeterminate verification phase clearly | `dashboard.html` | +1.0 |
| 🟠 IMPORTANT | SOLID: `WebPortal.cpp` violates Single Responsibility by mixing HTTP routing, JSON API serialization, and OTA upload state management | `WebPortal.cpp` | +1.0 |
| 🟡 MINOR | Git Discrepancy: `firmware/README.md` and root `README.md` modified but not listed in story file change map | `README.md` | +0.3 |

### Evidence Score: 12.3

| Score | Verdict |
|-------|---------|
| **12.3** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[6/10] Single Responsibility Principle:** `WebPortal.cpp` is a God Object. It handles HTTP routing, JSON serialization, OTA state machine orchestration, and LittleFS I/O.
  - 📍 `WebPortal.cpp`
  - 💡 Fix: Delegate OTA handling to a dedicated `OtaHandler` class; move JSON serialization to a `ResponseFactory`.

- **[4/10] High-Level Component dependency:** `OTAUpdater` performs `OTAUpdater::prepareForOTA()` (calling `ModeRegistry`) — a high-level orchestration component depending on a display subsystem implementation.
  - 📍 `OTAUpdater.cpp:164`
  - 💡 Fix: Introduce an `OtaOrchestrator` to manage the sequence between `OTAUpdater` and `ModeRegistry`.

---

## 🐛 Correctness & Safety

- **🐛 Bug (Security):** `mbedtls_sha256_free(&shaCtx)` is missing in multiple error paths within `OTAUpdater::_downloadTaskProc`. This causes a memory leak every time an OTA download fails.
  - 📍 `OTAUpdater.cpp:258, 285`
  - 🔄 Reproduction: Simulate network timeout during binary download.

- **🐛 Bug:** `xTaskCreatePinnedToCore` stack size `10240` is dangerously close to the limit. The `mbedtls_sha256_context` plus `HTTPClient` stack usage during TLS negotiation often exceeds the remaining headroom, risking stack overflow during OTA.
  - 📍 `OTAUpdater.cpp:218`
  - 🔄 Reproduction: Flash firmware, trigger OTA pull over high-latency WiFi.

---

## ⚡ Performance & Scalability

- **[High] Performance Footgun:** The dashboard OTA progress polling interval is 500ms, but `pollOtaStatus` triggers a synchronous `FW.get` request. If the firmware is busy writing to flash or processing high-traffic ADS-B data, these frequent, unbuffered polling requests will cause CPU contention on Core 1.
  - 📍 `dashboard.js:806`
  - 💡 Fix: Increase polling interval to 2000ms or implement a server-sent event (SSE) if bandwidth permits.

---

## 🛠️ Suggested Fixes

### 1. OTA Download Integrity (Security)
**File:** `firmware/core/OTAUpdater.cpp`
**Issue:** SHA-256 context leak in error paths.

```cpp
// Corrected path
if (!downloadOk) {
    _state = OTAState::ERROR;
    mbedtls_sha256_free(&shaCtx); // ALWAYS free
    Update.abort();
    // ... rest of error path
}
```

### 2. Stack Safety
**File:** `firmware/core/OTAUpdater.cpp`
**Issue:** 10KB stack is insufficient for TLS + OTA streaming.

```diff
- 10240,          // 10 KB stack
+ 16384,          // 16 KB stack
```

---

## Review Summary

**Issues Fixed:** 0
**Action Items Created:** 3

🚫 Code requires significant rework. Review action items carefully.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-16</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">dl-7</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="712">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">2</var>
<var name="session_id">4d9ae984-d2ca-4bef-988d-9938e0047235</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="671ae19b">embedded in prompt, file id: 671ae19b</var>
<var name="story_id">dl-7.3</var>
<var name="story_key">dl-7-3-ota-pull-dashboard-ui-and-status-polling</var>
<var name="story_num">3</var>
<var name="story_title">3-ota-pull-dashboard-ui-and-status-polling</var>
<var name="template">False</var>
<var name="timestamp">20260416_1917</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-7.3)
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