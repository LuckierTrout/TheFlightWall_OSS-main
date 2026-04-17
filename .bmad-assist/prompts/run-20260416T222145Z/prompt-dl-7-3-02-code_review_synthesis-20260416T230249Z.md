<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-7 -->
<!-- Story: 3 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T230249Z -->
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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `dl-7.3` — `ota-pull-dashboard-ui-and-status-polling`

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
| 🟠 IMPORTANT | Incomplete error handling / UI feedback synchronization | `dashboard.js` poll loop | +1 |
| 🟠 IMPORTANT | Potential for inconsistent state if `Update.end` fails after stream success | `OTAUpdater.cpp` | +1 |
| 🟢 CLEAN PASS | SOLID Principles | N/A | -0.5 |
| 🟢 CLEAN PASS | Performance | N/A | -0.5 |
| 🟢 CLEAN PASS | Style / Readability | N/A | -0.5 |
| 🟢 CLEAN PASS | Type Safety | N/A | -0.5 |
| 🟢 CLEAN PASS | Security | N/A | -0.5 |

### Evidence Score: -0.5

| Score | Verdict |
|-------|---------|
| **-0.5** | **EXEMPLARY** |

---

## 🏛️ Architectural Sins

None. The architecture established in Foundation and Display System is robustly followed here.

---

## 🐍 Pythonic Crimes & Readability

None.

---

## ⚡ Performance & Scalability

None.

---

## 🐛 Correctness & Safety

- **🐛 Bug: OTA Polling Lifecycle Risk:**
  - `firmware/data-src/dashboard.js` uses `setInterval` for OTA polling. While the implementation handles `error` and terminal states, the `setInterval` is not consistently cleared if the browser tab is backgrounded/suspended, which can lead to multiple concurrent poll timers if the user navigates away and back.
  - 📍 `dashboard.js:460`
  - 🔄 Reproduction: Navigate to dashboard, start OTA pull, navigate away to another tab, return. The timer may still be running or have been duplicated.
  - 💡 Fix: Use a recurring `setTimeout` pattern as used in `pollSwitch()` instead of `setInterval`.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: OTA Finalization Verification:**
  - `OTAUpdater.cpp` performs SHA-256 verification *before* calling `Update.end(true)`. While technically correct, `Update.end(true)` is the *final* authority on partition integrity. If `Update.end()` fails, there is no separate logical step for "verification failure" distinct from "finalize failure." The current logic splits this into two phases, creating a slightly confusing error reporting path.
  - 📍 `OTAUpdater.cpp:805`
  - 💥 Explosion radius: Minor — just a logic abstraction mismatch.

---

## 🛠️ Suggested Fixes

### 1. Robust OTA Polling

**File:** `dashboard.js`
**Issue:** `setInterval` in polling loop is fragile for browser background/suspend cycles.

**Corrected code:**
```javascript
  function startOtaPollLoop() {
    if (otaPollTimer) clearTimeout(otaPollTimer);
    function poll() {
       pollOtaStatus().then(() => {
          if (otaStateIsTerminal) return;
          otaPollTimer = setTimeout(poll, OTA_POLL_INTERVAL);
       });
    }
    otaPollTimer = setTimeout(poll, OTA_POLL_INTERVAL);
  }
```

---

## Review Summary

**Issues Fixed:** 0
**Action Items Created:** 1

🎉 Exemplary code quality! The implementation adheres strictly to the architectural patterns established in the Foundation and Display System releases. The identified issues are minor maintenance/UX refinements.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** dl-7.3 - ota-pull-dashboard-ui-and-status-polling

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 4 |
| Task Completion Lies | 2 |
| SOLID Violations | 3 |
| Hidden Bugs | 2 |
| Performance Footguns | 2 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **15** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task 3 marked [x] but `getStateString()` not in actual code | WebPortal.cpp, OTAUpdater.h | +3 |
| 🔴 CRITICAL | Task 8 marked [x] but `getFailurePhaseString()` not implemented | OTAUpdater.h absence | +3 |
| 🟠 IMPORTANT | AC #3 requires `getStateString()` + `getFailurePhaseString()` - completely missing | dl-7-3.md vs implementation | +1 |
| 🟠 IMPORTANT | AC #5 requires scoped error display on cards - missing from dashboard.js | dl-7-3.md AC#5 vs dashboard.js line 1200+ | +1 |
| 🟠 IMPORTANT | AC #6 requires retry button logic when `retriable=true` - stubbed/incomplete | dl-7-3.md AC#8 vs dashboard.js | +1 |
| 🟠 IMPORTANT | Poll interval hard-coded as 500ms - no AC spec, should verify AC#4 requirement | dashboard.js line 1285 | +1 |
| 🟠 IMPORTANT | No race condition protection on `otaPollTimer` global | dashboard.js global state, concurrent callbacks | +1 |
| 🟡 MINOR | Missing null check on `otaPullProgress` DOM elements | dashboard.js lines 1230-1245 | +0.3 |
| 🟡 MINOR | HTTP error messages not including status codes per dl-7-2 ANTIPATTERNS | WebPortal.cpp, OTAUpdater.cpp | +0.3 |
| 🟡 MINOR | No documentation for OTA_POLL_INTERVAL constant | dashboard.js line 1280 | +0.3 |
| 🟢 CLEAN PASS | Architectural compliance with existing patterns | x1 | -0.5 |

### Evidence Score: 10.3

| Score | Verdict |
|-------|---------|
| **10.3** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[7/10] Single Responsibility Violation:** The `startOtaPull()` function in dashboard.js mixes UI state management, HTTP request handling, and polling lifecycle. It should delegate to smaller functions.
  - 📍 `firmware/data-src/dashboard.js:1243-1256`
  - 💡 Fix: Extract `FW.post()` call and state management into separate `requestOtaDownload()` and `setOtaUiState()` functions

- **[6/10] Violated Dependency Inversion:** `dashboard.js` hard-codes API endpoint names (`/api/ota/pull`, `/api/ota/status`). Should use API contract from WebPortal as source of truth.
  - 📍 `firmware/data-src/dashboard.js:1245, 1260`
  - 💡 Fix: Load endpoint definitions from `window.API_ENDPOINTS` set by WebPortal on page init

- **[5/10] Insufficient Abstraction - Toast System:** Toast display called directly 8+ times in dashboard.js without abstraction. Each has slightly different error formatting.
  - 📍 `firmware/data-src/dashboard.js:1249, 1260, 1272, 1275, 1289, 1305, 1330`
  - 💡 Fix: Create `OtaUi.showMessage(level, message, options)` wrapper that normalizes all toast calls

---

## 🐍 Pythonic Crimes & Readability

- **Variable Naming:** `btnOtaRetry` uses abbreviations inconsistently. Other buttons use `btn` prefix, but DOM elements switch between `btnOta*`, `otaPull*`, and `otaProgress*` conventions.
  - 📍 `firmware/data-src/dashboard.js:lines 1220-1245`

- **Magic String Duplication:** The string "OTA" appears hardcoded in 12 locations without a single constant. The string `"Rebooting in "` appears twice.
  - 📍 `firmware/data-src/dashboard.js:1256, 1262, 1306, etc.`

- **Missing JSDoc Comments:** Functions `startOtaPull()`, `startOtaPullRebootPolling()`, `pollOtaStatus()` have zero documentation. Line count and complexity justify documentation.
  - 📍 `firmware/data-src/dashboard.js:1243, 1290, 1310`

- **Type Safety:** `d.progress` can be `null` but code does integer division without checking. `d.retriable` is assumed boolean but never validated.
  - 📍 `firmware/data-src/dashboard.js:1323`

---

## ⚡ Performance & Scalability

- **[HIGH] Race Condition on Poll Timer:** `otaPollTimer` is a global that can be set/cleared concurrently by `pollOtaStatus()` (creates interval) and error handlers (clears interval). No guard against double-clear causing exception.
  - 📍 `firmware/data-src/dashboard.js:1280, 1287, 1333, 1340`
  - 💡 Fix: Wrap timer management in `OtaPolling` class with state guards

- **[MEDIUM] Unbounded Reboot Poll:** `startOtaPullRebootPolling()` polls forever if device never returns. No maximum attempt counter documented (code shows `maxAttempts = 20` but this is 1 minute hard-coded).
  - 📍 `firmware/data-src/dashboard.js:1335`
  - 💡 Document `MAX_REBOOT_POLL_SECONDS = 60` as configurable const or verify AC requirement

---

## 🐛 Correctness & Safety

- **🐛 Bug: Missing Guard on DOM Elements:** If dashboard.html is modified and `otaPullProgress` div is removed, `resetOtaPullState()` will throw when accessing `.style.display`. No element existence checks.
  - 📍 `firmware/data-src/dashboard.js:1225-1232`
  - 🔄 Reproduction: Delete `<div id="ota-pull-progress">` from HTML, click "Update Now" → JavaScript exception in console

- **🐛 Bug: Silent Failure on Missing API Response Field:** `d.retriable` is optional per AC#8, but code uses it as truthy without checking if the field exists. Browser JS will treat `undefined` as falsy (works), but violates defensive programming.
  - 📍 `firmware/data-src/dashboard.js:1330`
  - 🔄 Reproduction: Modify `/api/ota/status` mock to omit `retriable` field → retry button won't show even if failure is retriable

---

## 🔒 Security Vulnerabilities

- **[MEDIUM] HTML Injection Risk in Error Messages:** When `d.error` is displayed directly in `otaPullError.textContent`, malicious server response could inject HTML if assigned to `.innerHTML` instead. Code is safe (uses `.textContent`), but comment should note why.
  - 📍 `firmware/data-src/dashboard.js:1289`
  - 💡 Add comment: `// Safe: textContent prevents HTML injection. Do NOT change to innerHTML.`

---

## 🎭 Lying Tests

- **🎭 Lying Test:** `test_get_state_string_returns_non_null()` in test_ota_updater/test_main.cpp
  - 📍 `firmware/test/test_ota_updater/test_main.cpp:402-406`
  - 🤥 Why it lies: Test only checks that string is non-null and has length > 0. Does NOT validate that string matches one of the 7 valid state names. A function returning "invalid_state" would pass this test and break runtime.

- **🎭 Lying Test:** `test_get_failure_phase_string_known_values()` 
  - 📍 `firmware/test/test_ota_updater/test_main.cpp:416-423`
  - 🤥 Why it lies: Uses local `bool valid = (strcmp(...) == 0 || ...)` variable but never asserts it's true. If the function returns "wrong_value", test passes silently.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Hard-Coded Poll Constants** - `OTA_POLL_INTERVAL = 500`, `MAX_REBOOT_POLL_ATTEMPTS = 20` embedded in function without constants file.
  - 📍 `firmware/data-src/dashboard.js:1280-1285`
  - 💥 Explosion radius: Changing polling behavior requires editing minified production file. No way to adjust behavior without code edit.

- **💣 Tech Debt: No Error Recovery State** - When OTA fails and user clicks "Retry", code doesn't validate that `/api/ota/status` has returned to IDLE/AVAILABLE state. Could retry in DOWNLOADING state mid-upload.
  - 📍 `firmware/data-src/dashboard.js:1375-1382`
  - 💥 Explosion radius: User could trigger double-upload, corrupted state, or heap overflow on ESP32.

---

## 📋 Detailed AC Gap Analysis

**AC #3 - MISSING IMPLEMENTATION:**
```
Requirement: "return { ok, data: { state: "<string>", progress: <0-100|null>, error: <string|null>, phase: <optional same as dl-7-2> } }"
Expected: OTAUpdater::getStateString() returns lowercase state name ("idle", "checking", "available", "downloading", "verifying", "rebooting", "error")
Actual: Function does NOT exist in OTAUpdater.h or OTAUpdater.cpp
Evidence: Grep of entire codebase finds zero definition of getStateString()
Severity: 🔴 CRITICAL - API contract incomplete
```

**AC #5 - PARTIAL IMPLEMENTATION:**
```
Requirement: "display message and a "Retry" button that calls POST /api/ota/pull again (only if dl-7-2 marks failure retriable)"
Expected: Retry button appears only when d.retriable === true
Actual: Code shows `if (d.retriable) { btnOtaRetry.style.display = ''; }` but...
  1. No null-check on btnOtaRetry (throws if removed from HTML)
  2. No validation that d.retriable exists before truthy check
  3. Retry button click resets UI but doesn't validate AVAILABLE state first
Severity: 🟠 IMPORTANT - Partial implementation, missing defensive checks
```

**AC #6 - UNDOCUMENTED REQUIREMENT:**
```
Requirement: "Keep modeSwitchInFlight=true until reload completes to prevent concurrent switch races"
Status: Code comment says "modeSwitchInFlight=true until reload completes" exists in mode switching
Actual: OTA code does NOT use equivalent pattern. `startOtaPull()` doesn't set an in-flight guard.
Potential: If user clicks "Update Now" twice rapidly, both requests hit the same `/api/ota/pull` endpoint simultaneously.
Severity: 🟠 IMPORTANT - Race condition unprotected
```

**AC #4 - HARD-CODED VALUE:**
```
Requirement: "per AC #4: 500 ms"
Status: Code has `const OTA_POLL_INTERVAL = 500;`
Issue: This constant is embedded in JavaScript and not referenced from any AC specification document
  - No validation that 500ms is appropriate for bandwidth/latency
  - No comment explaining why 500ms vs 1000ms
  - No mechanism to adjust without editing code
Severity: 🟡 MINOR - Works as intended but lacks justification
```

---

## 🛠️ Suggested Auto-Fixes

### 1. Add Missing `getStateString()` and `getFailurePhaseString()` to OTAUpdater

**File:** `firmware/core/OTAUpdater.h`
**Issue:** AC #3 requires these methods but they're completely missing

**Add to header:**
```cpp
static const char* getStateString();     // Returns lowercase state name
static const char* getFailurePhaseString();  // Returns lowercase phase name
```

**File:** `firmware/core/OTAUpdater.cpp`
**Add implementations:**
```cpp
const char* OTAUpdater::getStateString() {
    switch (_state) {
        case OTAState::IDLE:        return "idle";
        case OTAState::CHECKING:    return "checking";
        case OTAState::AVAILABLE:   return "available";
        case OTAState::DOWNLOADING: return "downloading";
        case OTAState::VERIFYING:   return "verifying";
        case OTAState::REBOOTING:   return "rebooting";
        case OTAState::ERROR:       return "error";
        default:                    return "unknown";
    }
}

const char* OTAUpdater::getFailurePhaseString() {
    switch (_failurePhase) {
        case OTAFailurePhase::NONE:     return "none";
        case OTAFailurePhase::DOWNLOAD: return "download";
        case OTAFailurePhase::VERIFY:   return "verify";
        case OTAFailurePhase::BOOT:     return "boot";
        default:                        return "unknown";
    }
}
```

### 2. Fix DOM Element Null Safety in dashboard.js

**File:** `firmware/data-src/dashboard.js`
**Issue:** Functions access DOM elements without existence checks

**Corrected code:**
```javascript
function resetOtaPullState() {
    if (otaPollTimer) { clearInterval(otaPollTimer); otaPollTimer = null; }
    
    // Guard against missing DOM elements
    if (!otaPullProgress) return console.warn('otaPullProgress element not found');
    
    otaPullProgress.style.display = 'none';
    if (otaPullBar) otaPullBar.style.width = '0%';
    if (otaPullBarWrap) otaPullBarWrap.setAttribute('aria-valuenow', '0');
    if (otaPullBarWrap) otaPullBarWrap.classList.remove('indeterminate');
    if (otaPullPct) otaPullPct.textContent = '0%';
    if (otaPullStatus) otaPullStatus.textContent = '';
    if (otaPullError) otaPullError.style.display = 'none';
    if (otaPullError) otaPullError.textContent = '';
    if (btnOtaRetry) btnOtaRetry.style.display = 'none';
}
```

### 3. Add Guard Against Double-Update and In-Flight Race

**File:** `firmware/data-src/dashboard.js`
**Issue:** No guard prevents rapid "Update Now" clicks or concurrent requests

**Corrected code:**
```javascript
var otaUpdateInFlight = false;

function startOtaPull() {
    // Guard against double-click
    if (otaUpdateInFlight) {
        FW.showToast('Update already in progress', 'warning');
        return;
    }
    
    otaUpdateInFlight = true;
    if (btnUpdateNow) btnUpdateNow.disabled = true;
    
    resetOtaPullState();
    otaPullProgress.style.display = '';
    otaPullStatus.textContent = 'Starting download…';
    
    // ... rest of POST call ...
    FW.post('/api/ota/pull', {}).then(function(res) {
        if (!res.body || !res.body.ok) {
            otaUpdateInFlight = false;
            if (btnUpdateNow) btnUpdateNow.disabled = false;
            // ... error handling ...
            return;
        }
        startOtaPollLoop();
    }).catch(function() {
        otaUpdateInFlight = false;
        if (btnUpdateNow) btnUpdateNow.disabled = false;
        // ... error handling ...
    });
}

function resetOtaPullState() {
    // ... existing code ...
    otaUpdateInFlight = false;
    if (btnUpdateNow) btnUpdateNow.disabled = false;
}
```

### 4. Fix Lying Tests - Add Proper Assertions

**File:** `firmware/test/test_ota_updater/test_main.cpp`
**Issue:** Tests don't actually assert the valid values, just check string exists

**Corrected code:**
```cpp
void test_get_state_string_returns_valid_values() {
    // Create a set of known valid values
    const std::set<std::string> validStates = {
        "idle", "checking", "available", "downloading", "verifying", "rebooting", "error"
    };
    
    const char* s = OTAUpdater::getStateString();
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_TRUE(strlen(s) > 0);
    
    // CRITICAL: Actually assert the value is valid
    TEST_ASSERT_TRUE(validStates.count(std::string(s)) > 0);
}

void test_get_failure_phase_string_known_values() {
    const std::set<std::string> validPhases = {
        "none", "download", "verify", "boot"
    };
    
    const char* s = OTAUpdater::getFailurePhaseString();
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_TRUE(strlen(s) > 0);
    
    // CRITICAL: Actually assert the value is valid
    TEST_ASSERT_TRUE(validPhases.count(std::string(s)) > 0);
}
```

### 5. Add Missing `retriable` Field Validation

**File:** `firmware/data-src/dashboard.js`
**Issue:** Code assumes `d.retriable` exists without checking

**Corrected code:**
```javascript
function pollOtaStatus() {
    FW.get('/api/ota/status').then(function(res) {
        if (!res.body || !res.body.ok || !res.body.data) return;
        var d = res.body.data;

        // ... existing state checks ...

        } else if (d.state === 'error') {
            if (otaPollTimer) { clearInterval(otaPollTimer); otaPollTimer = null; }
            otaPullBarWrap.classList.remove('indeterminate');
            otaPullStatus.textContent = 'Update failed';
            if (d.error) otaPullError.textContent = d.error;
            if (otaPullError) otaPullError.style.display = '';
            
            // FIX: Validate retriable field exists and is boolean
            if (typeof d.retriable === 'boolean' && d.retriable) {
                if (btnOtaRetry) btnOtaRetry.style.display = '';
            } else {
                if (btnOtaRetry) btnOtaRetry.style.display = 'none';
            }
            
            if (btnUpdateNow) btnUpdateNow.disabled = false;
```

---

## 🚨 Critical Task Completion Audit

| Task # | Task Description | Status | Evidence |
|--------|------------------|--------|----------|
| 1 | Start OTA Pull download in background FreeRTOS task | ✅ DONE | `OTAUpdater::startDownload()` in cpp spawns task |
| 2 | Implement `/api/ota/pull` WebPortal handler | ✅ DONE | WebPortal.cpp lines 1250-1275 |
| 3 | **Add `getStateString()` method for AC#3** | ❌ **MISSING** | Zero references in entire codebase |
| 4 | **Add `getFailurePhaseString()` method for AC#3** | ❌ **MISSING** | Zero references in entire codebase |
| 5 | Create 500ms polling interval loop in dashboard.js | ✅ DONE | dashboard.js line 1280 |
| 6 | Display progress bar 0-100% during download | ✅ DONE | dashboard.js lines 1318-1324 |
| 7 | Show "Verifying..." during verification phase | ⚠️ **PARTIAL** | Code exists but doesn't validate `d.state` enum |
| 8 | Display error message with retry button | ✅ DONE | But missing null-checks per findings |

**CRITICAL FINDINGS:** Tasks 3 and 4 are marked [x] in the story but the methods do NOT exist in the actual OTAUpdater code. This is a lie.

---

## Review Summary

**Issues Fixed:** 0 (Report only; fixes await user approval)
**Action Items Created:** 8 critical/important items

### Evidence Verdict Assessment

**Evidence Score: 10.3** = **REJECT**

**Score Breakdown:**
- 🔴 CRITICAL: Tasks 3-4 marked done but missing (×2 = +6 points)
- 🔴 CRITICAL: AC #3 completely unimplemented (×1 = +3 points)
- 🟠 IMPORTANT: AC #5 missing defensive checks (×1 = +1 point)
- 🟠 IMPORTANT: Race condition on in-flight guard (×1 = +1 point)
- 🟠 IMPORTANT: Poll interval hard-coded without rationale (×1 = +1 point)
- 🟡 MINOR: Missing null checks on DOM (×3 = +0.9 points)
- 🟡 MINOR: Lying tests without real assertions (×2 = +0.6 points)
- 🟢 CLEAN PASS: Architecture extends properly (×1 = -0.5 points)

**Result:** 10.3 exceeds APPROVED threshold of 3.0 significantly.

---

## ⚠️ Recommendations

1. **MUST FIX BEFORE MERGE:** Implement `getStateString()` and `getFailurePhaseString()` — these are AC requirements
2. **MUST FIX BEFORE MERGE:** Add in-flight guard to prevent double-update race condition
3. **SHOULD FIX:** Add null-checks to all DOM element accesses
4. **SHOULD FIX:** Proper test assertions instead of lying tests
5. **NICE-TO-HAVE:** Document hard-coded constants and add configuration mechanism

🚫 **This story should NOT be approved for merge in its current state.** Two AC requirements are completely missing, tests are lying, and there's a race condition vulnerability.

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
<var name="session_id">06dfbf22-e9a9-49ca-9f64-a097844e7f90</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="671ae19b">embedded in prompt, file id: 671ae19b</var>
<var name="story_id">dl-7.3</var>
<var name="story_key">dl-7-3-ota-pull-dashboard-ui-and-status-polling</var>
<var name="story_num">3</var>
<var name="story_title">3-ota-pull-dashboard-ui-and-status-polling</var>
<var name="template">False</var>
<var name="timestamp">20260416_1902</var>
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