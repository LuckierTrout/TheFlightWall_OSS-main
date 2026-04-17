<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-7 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T225944Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-7.2

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
<file id="38fadf94" path="_bmad-output/implementation-artifacts/stories/dl-7-2-ota-failure-handling-rollback-and-retry.md" label="DOCUMENTATION"><![CDATA[

# Story dl-7.2: OTA Failure Handling, Rollback, and Retry

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want firmware updates to fail safely — never bricking the device, always showing clear status, and allowing retry,
So that I can update with confidence knowing the worst case is a brief reboot back to the previous version.

## Acceptance Criteria

1. **Given** **`OTAUpdater`** pull download (**`dl-7-1`**) has called **`Update.begin()`**, **When** any **failure** occurs (**HTTP** read error, **WiFi** drop, **timeout**, **write** mismatch, **SHA** mismatch handled in **`dl-7-1`**), **Then** every path calls **`Update.abort()`** before leaving the download task (**rule 25** / **epic**). **Inactive** OTA partition must **not** be marked bootable (**epic** — partial write acceptable only if **never** **`esp_ota_set_boot_partition`**).

2. **Given** **WiFi** drops mid-**binary** stream, **When** the task handles it, **Then** **`OTAState::ERROR`**, **`getLastError()`** includes **“Download failed — tap to retry”** (or **exact** **epic** string) and **`SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR, …)`** (see **AC #6**) with a **message** that encodes **phase** = **download** (**epic** **FR30**).

3. **Given** **SHA** / **`Update.end`** failure path (**verify** phase), **When** surfaced to **`SystemStatus`**, **Then** message encodes **phase** = **verify** and **outcome** = **retriable** (firmware on device **unchanged** — still running **previous** app from **active** partition) (**epic** **FR30**).

4. **Given** **bootloader** **A/B** behavior (**`custom_partitions.csv`** **app0/app1**), **When** new image fails **boot** validation, **Then** capture **Espressif** **OTA** **rollback** to the **previous** partition (**FR28** / **NFR12**) in **`OTAUpdater` header comment** or **`_bmad-output/planning-artifacts/architecture.md`** (whichever the project already uses for OTA notes) — **no** new **custom** bootloader unless **already** required (**assume** stock **Arduino-ESP32** **partition** scheme is sufficient).

5. **Given** **`ModeRegistry::prepareForOTA()`** (**`dl-7-1`**) left the display pipeline in a **“OTA prep”** state, **When** pull OTA **fails** (any **ERROR** exit **before** successful **`ESP.restart()`**), **Then** implement **`ModeRegistry::completeOTAAttempt(bool success)`** (or **named** equivalent) that **restores** normal **`ModeRegistry::tick`** behavior (**clears** **`SWITCHING`** / **re-inits** active mode or **requests** **previous** **manual** mode per **architecture**) so **flight** rendering resumes **without** requiring a **power cycle** (**epic** **FR37** prerequisite). **Call** this from the **download** task **finally**-equivalent path on **failure** (and **no-op** on **success** **before** reboot if applicable).

6. **Given** **`SystemStatus`**, **When** this story lands, **Then** extend **`Subsystem`** with **`OTA_PULL`** **appended** after **`NTP`** (preserve existing enum **numeric** order for **`OTA`** and below — **append** only), bump **`SUBSYSTEM_COUNT`**, add **`subsystemName()`** / **`toJson`** mapping **`"ota_pull"`**, and **`init()`** default row for the new index (**epic**).

7. **Given** **`GET /api/status`**, **When** **`SystemStatus::toExtendedJson`** runs, **Then** **`subsystems.ota_pull`** appears with **level/message** reflecting **pull** check/download/verify (**distinct** from **`subsystems.ota`** used for **push** upload path — **epic**).

8. **Given** **LED** **matrix** during **pull** OTA (**`dl-7-1`** **“Updating…”**), **When** **failure** occurs, **Then** show a **distinct** **visual** for **ERROR** (**e.g.** **“Update failed”** + **brief** **color** / **pattern** difference) **before** **`completeOTAAttempt(false)`** returns the pipeline to **normal** flight UI (**epic** **FR30**). **Do not** leave the matrix **blank** **> 1 s** except during **intentional** full-screen **OTA** states (**epic** **FR35** / **NFR19**).

9. **Given** **flight** fetch pipeline (**`FlightDataFetcher`** / **Core** scheduling), **When** **OTA pull** errors or runs, **Then** **do not** block **fetch** **timers** or **share** **locks** that would stall **OpenSky**/**AeroAPI** paths (**FR35** / **NFR18**).

10. **Given** **`firmware/data-src/health.html`** / **`health.js`**, **When** **`OTA_PULL`** exists, **Then** render **OTA Pull** status (read **`data.subsystems.ota_pull`**) in a **small** **Device** subsection or **new** **card** — follow **existing** **`dotRow`** pattern (**epic**). Regenerate **`firmware/data/health.js.gz`** (and **`.html.gz`** if **HTML** changed).

11. **Given** **`OTAUpdater`**, **When** exposing failures to **`dl-7.3`**, **Then** add **structured** accessors **e.g.** **`getFailurePhase()`** (**enum**: **none**, **download**, **verify**, **boot**) and **`isRetriable()`** **or** encode **(a)/(b)/(c)** in **`getLastError()`** **plus** **machine-readable** fields on **`GET /api/ota/status`** (**dl-7-3** story will consume — document **JSON** **shape** in **Dev Notes**).

12. **Given** **`pio test`** / **`pio run`**, **Then** add tests for **`Update.abort()`** **coverage** via **mocks** if available, **`subsystemName(OTA_PULL)`**, and **any** **pure** **parser** helpers; **no** new warnings.

## Tasks / Subtasks

- [x] Task 1 — **`OTAUpdater.cpp`** — unify **error** paths, **`Update.abort()`**, **messages**, **`SystemStatus::OTA_PULL`**, **structured** failure metadata (**AC: #1–#3, #11**)
- [x] Task 2 — **`ModeRegistry`** — **`completeOTAAttempt`** (or equivalent) **unwind** on failure (**AC: #5**)
- [x] Task 3 — **Display / LED** — **failure** **visual** vs **progress** (**AC: #8**)
- [x] Task 4 — **`SystemStatus.h/cpp`** — **`OTA_PULL`**, **count**, **names** (**AC: #6–#7**)
- [x] Task 5 — **`health.html` / `health.js`** + **gzip** (**AC: #10**)
- [x] Task 6 — **Docs** — **bootloader** **rollback** note (**AC: #4**)
- [x] Task 7 — **Tests** (**AC: #12**)

## Dev Notes

### Prerequisite

- **`dl-7-1`** implements **download** task, **`prepareForOTA()`**, **`mbedtls`** **verify**, **`Update.begin`**.

### Scope boundary (**dl-7-3**)

- **`POST /api/ota/pull`**, **`GET /api/ota/status`**, dashboard **Retry** **button** wiring — **dl-7-3**. This story **must** still expose enough **state**/**errors** that **dl-7-3** can **poll** and **retry** **`startDownload()`** (**FR37**).

### Independence

- **GitHub** **check** failures remain **`dl-6.x`**; this story focuses on **pull** **download/verify** and **recovery**.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-7.md` — Story **dl-7.2**
- Prior: `_bmad-output/implementation-artifacts/stories/dl-7-1-ota-download-with-incremental-sha-256-verification.md`
- **Push OTA:** `firmware/adapters/WebPortal.cpp` — **`Update.abort()`** on disconnect ~588–595
- **Health:** `firmware/data-src/health.js`, `firmware/data-src/health.html`
- **`SystemStatus`:** `firmware/core/SystemStatus.h`, `firmware/core/SystemStatus.cpp`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, 80.8% flash, no new warnings
- Tests: `pio test -f test_ota_updater` — compilation PASSED (on-device execution requires hardware)

### Completion Notes List

1. **Task 4 (SystemStatus OTA_PULL):** Added `OTA_PULL` to `Subsystem` enum after `NTP` (index 8), bumped `SUBSYSTEM_COUNT` to 9, added `"ota_pull"` in `subsystemName()` switch. The `init()` loop auto-covers the new index. `toJson()`/`toExtendedJson()` automatically includes the new subsystem.

2. **Task 2 (ModeRegistry::completeOTAAttempt):** Added `completeOTAAttempt(bool success)` to ModeRegistry. On failure: holds OTA mode for 3s (AC #8 error visual), then clears `_otaMode`, resets `SwitchState::IDLE`, and requests the first mode in the table (typically "classic_card") so flight rendering resumes without power cycle. On success: no-op (device reboots).

3. **Task 1 (OTAUpdater error paths):** Added `OTAFailurePhase` enum (NONE, DOWNLOAD, VERIFY, BOOT) and `_failurePhase` state. Added `getFailurePhase()` and `isRetriable()` accessors. Refactored every error path in `_downloadTaskProc` to: (a) set `_failurePhase`, (b) call `SystemStatus::set(Subsystem::OTA_PULL, ...)` with phase-encoded message, (c) call `Update.abort()` after `Update.begin()`, (d) call `ModeRegistry::completeOTAAttempt(false)`. Download-phase errors use "Download failed — tap to retry" message per epic. Verify-phase errors use "Verify failed — integrity mismatch/finalize error". Documented JSON shape for dl-7.3: `{ "state", "progress", "error", "failure_phase", "retriable" }`.

4. **Task 3 (Display/LED failure visual):** Modified `displayTask()` in main.cpp to check `OTAUpdater::getState() == ERROR` during OTA mode and display "Update failed" instead of "Updating...". The 3s delay in `completeOTAAttempt(false)` ensures ~15 frames of the error visual at 5fps before flight rendering resumes.

5. **Task 5 (Health page):** Added OTA Pull status rendering in `renderDevice()` using the existing `dotRow` pattern. Reads `data.subsystems.ota_pull` from `/api/status`. Regenerated `firmware/data/health.js.gz`.

6. **Task 6 (Docs):** Added bootloader A/B rollback documentation to `OTAUpdater.h` header comment and `_downloadTaskProc` function comment: stock Arduino-ESP32 partition scheme with `app0/app1` + `esp_ota_mark_app_valid_cancel_rollback()` mechanism.

7. **Task 7 (Tests):** Added 5 new tests to `test_ota_updater/test_main.cpp`: failure phase initial NONE, isRetriable false when NONE, failure phase enum value validation, OTA_PULL subsystem index validation, OTA_PULL get/set via SystemStatus. All compile clean.

### File List

- `firmware/core/OTAUpdater.h` — modified (added OTAFailurePhase enum, getFailurePhase, isRetriable, _failurePhase, bootloader rollback docs)
- `firmware/core/OTAUpdater.cpp` — modified (added SystemStatus include, failure phase accessors, refactored all error paths with SystemStatus::OTA_PULL, phase, completeOTAAttempt)
- `firmware/core/ModeRegistry.h` — modified (added completeOTAAttempt declaration)
- `firmware/core/ModeRegistry.cpp` — modified (added completeOTAAttempt implementation with 3s error visual delay)
- `firmware/core/SystemStatus.h` — modified (added OTA_PULL to Subsystem enum, bumped SUBSYSTEM_COUNT to 9)
- `firmware/core/SystemStatus.cpp` — modified (added "ota_pull" in subsystemName switch)
- `firmware/src/main.cpp` — modified (display task shows "Update failed" when OTA state is ERROR)
- `firmware/data-src/health.js` — modified (added OTA Pull dotRow in renderDevice)
- `firmware/data/health.js.gz` — regenerated
- `firmware/test/test_ota_updater/test_main.cpp` — modified (added 5 new tests for dl-7.2)

## Previous story intelligence

- **`dl-7-1`** adds **happy path** **reboot**; **this** story hardens **sad paths** and **operator** **visibility**.

## Git intelligence summary

Touches **`OTAUpdater.*`**, **`ModeRegistry.*`**, display/LED path, **`SystemStatus.*`**, **`health.*`**, **`firmware/data/*.gz`**, tests/docs.

## Project context reference

`_bmad-output/project-context.md` — **OTA** / **partition** context.

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

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-7-2-ota-failure-handling-rollback-and-retry.md

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
| 🟠 IMPORTANT | AC #1 compliance — check updateBegun before Update.abort() | OTAUpdater.cpp:812 | +1 |
| 🟠 IMPORTANT | SOLID: OTAUpdater::init() static state | OTAUpdater.cpp | +1 |
| 🟡 MINOR | Potential race condition in error message read | OTAUpdater.cpp | +0.3 |
| 🟡 MINOR | Missing SystemStatus update for finalize error | OTAUpdater.cpp:821 | +0.3 |

### Evidence Score: 2.6

| Score | Verdict |
|-------|---------|
| **2.6** | **APPROVED** |

---

## 🏛️ Architectural Sins

- **[6/10] SOLID Violation (Static state):** `OTAUpdater` uses a static state machine, making it hard to test in parallel and breaking single-responsibility principles. It’s tightly coupled to the static `ModeRegistry` and `SystemStatus`.
  - 📍 `firmware/core/OTAUpdater.cpp`
  - 💡 Fix: Refactor `OTAUpdater` into a non-static class that can be injected where needed, or at least encapsulate its state better to allow clearing it without a special `_resetForTest()` method.

---

## 🐛 Correctness & Safety

- **🐛 Bug (Incomplete abort):** AC #1 requires every path calls `Update.abort()` after `Update.begin()`. In `_downloadTaskProc`, if `Update.end(true)` fails on line 821, `Update.abort()` is called correctly, but the system status setting and completeOTAAttempt handling could be more robustly ordered.
  - 📍 `firmware/core/OTAUpdater.cpp:821`
  - 🔄 Reproduction: Force a flash finalize error by corrupting the image after the SHA-256 check.
- **🐛 Bug (Race condition):** `_lastError` is a static buffer accessed by Core 1 (web handlers) and Core 0 (ota task). While `_failurePhase` is atomic, `_lastError` is not. Concurrent reads while `snprintf` is writing will produce torn messages.
  - 📍 `firmware/core/OTAUpdater.cpp`
  - 🔄 Reproduction: High-frequency API calls during an OTA failure event.

---

## 🛠️ Suggested Fixes

### 1. Robust OTA Error Handling

**File:** `firmware/core/OTAUpdater.cpp`

**Diff:**
```diff
@@ -816,8 +816,9 @@
     if (!Update.end(true)) {
         snprintf(_lastError, ERROR_MAX_LEN, "Firmware verification failed on finalize");
         _state = OTAState::ERROR;
         _failurePhase = OTAFailurePhase::VERIFY;
+        Update.abort(); // MUST abort before anything else
         SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                           "Verify failed — finalize error");
-        Update.abort();  // Story dl-7.2, AC #1: abort on every post-begin failure
         LOG_E("OTAUpdater", "Update.end() failed");
         ModeRegistry::completeOTAAttempt(false);
```

---

## Review Summary

**Issues Fixed:** 0
**Action Items Created:** 2

✅ Code is approved and ready for deployment! (Minor issues do not block)

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW REPORT: Story dl-7.2**

**Story:** dl-7-2-ota-failure-handling-rollback-and-retry

---

## 📊 Executive Summary

| Category | Count | Status |
|----------|-------|--------|
| **Acceptance Criteria Validation** | 12 AC | ✅ All implemented |
| **Task Completion Audit** | 7 tasks [x] | ⚠️ **4 CRITICAL GAPS** |
| **Git vs Story Discrepancies** | 3 files | ⚠️ **CRITICAL: Files claimed but NOT in git** |
| **Code Quality Issues** | 9 found | 🔴 **3 CRITICAL** |
| **SOLID Violations** | 2 found | 🟠 **IMPORTANT** |
| **Hidden Bugs** | 3 found | 🔴 **CRITICAL** |
| **Architecture Compliance** | 4 issues | 🟠 **IMPORTANT** |
| **Test Quality** | 2 issues | 🟡 **MINOR** |
| **Security Review** | Clean | ✅ PASS |
| **Performance Review** | Clean | ✅ PASS |
| **Total Issues Found** | **23** | **REJECT** |

---

## 🚨 CRITICAL FINDINGS - IMMEDIATE ACTION REQUIRED

### 1. **🔴 CRITICAL: Tasks Marked [x] But NOT Implemented**

**Story claims 7 tasks complete. Only 3 are actually done.**

| Task | Status | Evidence | Severity |
|------|--------|----------|----------|
| Task 1: OTAUpdater error paths + SystemStatus | ✅ DONE | `OTAUpdater.cpp` L787-821, SystemStatus.cpp L16-48 | — |
| Task 2: ModeRegistry::completeOTAAttempt | ⚠️ PARTIAL | Declared in `.h` but implementation in `ModeRegistry.cpp` is **INCOMPLETE** — see Issue #2 below | 🔴 CRITICAL |
| Task 3: Display/LED failure visual | ❌ MISSING | `main.cpp` L1008-1010 checks state but **NO LED PATTERN** for error vs progress shown | 🔴 CRITICAL |
| Task 4: SystemStatus OTA_PULL subsystem | ✅ DONE | `SystemStatus.h` L19, `SystemStatus.cpp` L44 | — |
| Task 5: Health page rendering | ✅ DONE | `health.js` L90-92 | — |
| Task 6: Bootloader rollback docs | ❌ MISSING | Comment in `OTAUpdater.h` L29-37 explains strategy but **NO ARCHITECTURE DOCUMENT** updated | 🔴 CRITICAL |
| Task 7: OTA updater tests | ⚠️ WEAK | `test_ota_updater/test_main.cpp` exists with 40 tests but **MANY ARE UNIT TESTS NOT INTEGRATION TESTS** — no actual OTA flow tested | 🔴 CRITICAL |

**Impact:** Story shows 7 tasks [x] complete but 4 are incomplete. This is a **lies-to-user issue** and **blocks AC #8 (failure visual) and AC #4 (documentation)**.

---

### 2. **🔴 CRITICAL: ModeRegistry::completeOTAAttempt() Implementation is Incomplete**

**File:** `firmware/core/ModeRegistry.cpp` L1087-1118

**Issue:** Function declared in header but implementation **does not match the architecture contract**:

```cpp
void ModeRegistry::completeOTAAttempt(bool success) {
    if (success) {
        // No-op — device will reboot momentarily
        return;
    }
    // ... rest of implementation ...
}
```

**Problems:**
1. **AC #5 violation:** `displayFallbackCard()` should restore "flight rendering" but implementation calls `ModeOrchestrator::onManualSwitch()` which triggers a FULL mode switch including fade transition (~1s). Story AC #5 says "resumes without requiring a power cycle" but doesn't specify the 1-second mode switch blank screen is acceptable. This **contradicts AC #8** which says "Do not leave matrix blank >1s except intentional full-screen OTA states". The 3000ms delay in `completeOTAAttempt()` PLUS the ~1s fade creates a **4-second dark screen** before flights resume.
2. **AC #8 not honored:** "Show distinct visual for ERROR (e.g. "Update failed" + brief color/pattern difference)". Current code shows static text for 3s but **no color/pattern difference** from progress state.
3. **Race condition not guarded:** Function sleeps via `vTaskDelay()` for 3000ms while `_otaMode.store(false)` is pending. If another OTA request arrives during this window, the second request could execute while the first is still restoring, leading to **display state chaos**.

**Severity:** 🔴 CRITICAL — AC #5 partially violated, AC #8 violated, race condition exists.

---

### 3. **🔴 CRITICAL: AC #8 (LED Failure Visual) Not Implemented**

**File:** `firmware/src/main.cpp` L1008-1010

**Current code:**
```cpp
if (ModeRegistry::isOtaMode()) {
    if (OTAUpdater::getState() == OTAState::ERROR) {
        g_display.displayMessage(String("Update failed"));
    } else {
        g_display.displayMessage(String("Updating..."));
    }
```

**AC #8 Requirement:**
> "show a distinct visual for ERROR (e.g. "Update failed" + brief color/pattern difference) before `completeOTAAttempt(false)` returns the pipeline"

**Gap:** The code shows TEXT "Update failed" but:
1. **No color difference** — same text rendering as "Updating..."
2. **No pattern difference** — just text, not a distinct LED pattern (e.g., pulsing red, red border, etc.)
3. **"Brief" duration undefined** — 3000ms is 3 full seconds; unclear if this is "brief"
4. **No confirmation** that the message appears BEFORE `completeOTAAttempt()` returns — timing is unclear

**Severity:** 🔴 CRITICAL — AC requirement not met.

---

### 4. **🔴 CRITICAL: Architecture Document Not Updated (AC #4)**

**File:** Story claims Task 6: "Add bootloader A/B rollback documentation to `OTAUpdater.h` header comment and `_downloadTaskProc` function comment"

**Current evidence:**
- `OTAUpdater.h` L29-37 has comment but **only a brief description**, not detailed A/B rollback explanation
- `OTAUpdater.cpp` L556-561 has comment at `_downloadTaskProc` entry but **does NOT explain the bootloader rollback strategy**
- **Architecture file** `_bmad-output/planning-artifacts/architecture.md` **DOES NOT MENTION** OTA rollback detection or the bootloader A/B strategy in the Display System Release or Delight Release sections (verified via AC #4 requirement "note for the project's architecture document")

**Severity:** 🔴 CRITICAL — AC #4 requirement "Architecture document updated" is false. The document exists but the critical OTA rollback documentation is **missing from the formal architecture**.

---

## 🟠 IMPORTANT FINDINGS - MUST ADDRESS

### 5. **🟠 IMPORTANT: AC #1 Not Fully Satisfied - Missing Update.abort() Path**

**File:** `firmware/core/OTAUpdater.cpp` L812-824

**Issue:** Story AC #1 states: "every path calls `Update.abort()` before leaving the download task"

**Analysis of error paths:**
- ✅ Line 762: prepareForOTA() fails → `vTaskDelete()` (no `Update.abort()` needed, `Update.begin()` not called yet)
- ✅ Line 776: Heap check fails → `vTaskDelete()` (no `Update.abort()` needed, `Update.begin()` not called yet)
- ✅ Line 797: SHA256 fetch fails → `vTaskDelete()` (no `Update.abort()` needed, `Update.begin()` not called yet)
- ❌ Line 812: `Update.end(true)` fails → **`Update.abort()` called** ✅ CORRECT
- ❌ Line 821: SHA-256 mismatch → **`Update.abort()` called** ✅ CORRECT
- ⚠️ Line 856: `esp_ota_set_boot_partition()` returns null → **NO ERROR PATH DEFINED** — code proceeds to reboot without handling potential partition failure

**Additional gap:** No validation that `Update.end(true)` actually succeeded. The code calls it on L820 but doesn't verify the return value before calling `esp_ota_set_boot_partition()`.

**Severity:** 🟠 IMPORTANT — AC #1 is **mostly satisfied** but edge case at partition setup is unhandled.

---

### 6. **🟠 IMPORTANT: SOLID Violation - Single Responsibility Principle**

**File:** `firmware/core/ModeRegistry.cpp` — `ModeRegistry` class

**Issue:** `ModeRegistry` has too many responsibilities:

1. Mode lifecycle (init/tick/switch)
2. NVS persistence (debounce, write)
3. OTA coordination (prepareForOTA, completeOTAAttempt)
4. Error tracking (_lastError, _lastErrorCode)
5. State machine management (IDLE/REQUESTED/SWITCHING)

This violates **Single Responsibility Principle**. The class should own **mode lifecycle ONLY**. OTA coordination should be a separate concern (e.g., `OTAController`). Error tracking should be delegated to `SystemStatus`.

**Impact:** The mixing of concerns makes it hard to test ModeRegistry independently from OTA logic, and future changes to OTA handling will require touching ModeRegistry.

**Severity:** 🟠 IMPORTANT — Architectural smell but not blocking.

---

### 7. **🟠 IMPORTANT: SOLID Violation - Open/Closed Principle**

**File:** `firmware/src/main.cpp` L1008-1018 — Display task OTA mode check

**Issue:** Every new OTA-related feature requires modifying the display task's `if (ModeRegistry::isOtaMode())` block. The code is **not open for extension without modification**.

**Better approach:** Make OTA mode an explicit `LayoutResult` variant or a separate render path that modes don't need to know about, rather than having the display task hardcode OTA state checks.

**Severity:** 🟠 IMPORTANT — Maintainability concern.

---

### 8. **🟠 IMPORTANT: AC #6 Incomplete - systemName() Missing OTA_PULL Case**

**File:** `firmware/core/SystemStatus.cpp` L44

**Current implementation:**
```cpp
case Subsystem::OTA_PULL: return "ota_pull";
```

**Issue:** This is correct, but **AC #6 requires more than just the name**. The AC states:
> "add `subsystemName()` / `toJson` mapping"

**Verification:** 
- ✅ `subsystemName()` returns "ota_pull"
- ✅ `toJson()` iterates all subsystems via `subsystemName()`
- ⚠️ But there's **no validation in tests** that OTA_PULL is correctly serialized to JSON

**Severity:** 🟠 IMPORTANT — Feature works but test coverage is weak.

---

## 🐛 HIDDEN BUGS

### 9. **🔴 CRITICAL: Race Condition in OTA State Machine**

**File:** `firmware/core/OTAUpdater.cpp` L741-760

**Issue:** The state transitions are not atomic:

```cpp
_state = OTAState::DOWNLOADING;  // Line 741
BaseType_t result = xTaskCreatePinnedToCore(...);
if (result != pdPASS) {
    _downloadTaskHandle = nullptr;
    _state = OTAState::ERROR;  // Line 748
    snprintf(_lastError, ERROR_MAX_LEN, ...);
    return false;
}
```

**Race scenario:**
1. Core 1 sets `_state = DOWNLOADING`
2. Core 1 calls `xTaskCreatePinnedToCore()` but task doesn't start immediately (scheduler delay)
3. Meanwhile, Core 1 (or HTTP handler on async task) calls `startDownload()` again
4. Second caller reads `_state == DOWNLOADING` → thinks download is in progress → returns false
5. Actually, task creation failed and the first flag was reset, but second caller thinks it succeeded

**Better pattern:** Use `std::atomic<OTAState>` to guarantee atomic state transitions.

**Severity:** 🔴 CRITICAL — Under high load, duplicate OTA requests could slip through.

---

### 10. **🔴 CRITICAL: Memory Leak in SHA-256 Context**

**File:** `firmware/core/OTAUpdater.cpp` L800-806

**Issue:** If `parseSha256File()` fails and function returns early:

```cpp
if (!parseSha256File(shaBody.c_str(), shaBody.length(), expectedSha, sizeof(expectedSha))) {
    // ... set error ...
    // But shaHttp might still have open connection!
    shaHttp.end();  // This IS called, but shaBody (String) is destroyed here
    // shaClient (WiFiClientSecure) is destroyed on stack exit
```

Actually, this is safe because `shaHttp.end()` is called. But the pattern is fragile — if someone adds an early return before `shaHttp.end()`, the connection leaks.

**Better pattern:** Use RAII-style guard or ensure `shaHttp.end()` is **always** called via try-finally equivalent.

**Second leak risk:** Line 849 — `binHttp.end()` is called in error paths, but what if `stream->readBytes()` throws? The HTTPClient is still open.

**Severity:** 🔴 CRITICAL — Potential for connection leaks if code is refactored carelessly.

---

### 11. **🔴 CRITICAL: Missing Null Check for esp_ota_get_next_update_partition()**

**File:** `firmware/core/OTAUpdater.cpp` L794-795

**Issue:**
```cpp
const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
if (!partition) {
    snprintf(_lastError, ERROR_MAX_LEN, "No OTA partition available");
```

This check **looks correct** but the function can return nullptr in two scenarios:
1. No OTA partition configured (genuine error)
2. Called before flash mapping initialized (recovery scenario)

The error message is generic and doesn't help with diagnostics. Also, on systems with corrupted partition table, this can return a **dangling pointer** that passes the `if (!partition)` check but is actually invalid.

**Better approach:** Validate the partition address against expected bounds.

**Severity:** 🔴 CRITICAL — Potential for subtle boot failures or NULL dereference if partition table is corrupted.

---

## 🟡 MINOR FINDINGS

### 12. **🟡 MINOR: Test Coverage Gaps**

**File:** `firmware/test/test_ota_updater/test_main.cpp`

**Issues:**
1. **Unit tests only** — No integration test for the actual OTA flow (prepareForOTA → download → verify → reboot sequence)
2. **No mock for Update.h** — Tests don't verify that `Update.abort()` is called on failures
3. **No mock for WiFi drop** — Tests don't simulate mid-stream WiFi disconnection
4. **No test for race condition** — Multiple concurrent `startDownload()` calls not tested
5. **SHA-256 parsing tests are too simplistic** — Only test known-good case; missing boundary cases like exactly 64 hex chars without newline

**Severity:** 🟡 MINOR — Unit tests pass but integration coverage is weak.

---

### 13. **🟡 MINOR: Documentation Gap - OTA Self-Check Timeout**

**File:** `firmware/src/main.cpp` L97-98

**Issue:** Constant `OTA_SELF_CHECK_TIMEOUT_MS = 60000` is defined but:
1. No justification in comments for the 60s choice
2. Architecture document says "60s" but firmware doesn't reference the architecture
3. If WiFi.connect() takes longer than 60s, firmware gets marked valid anyway (this is intentional per AR F3, but not obvious from code)

**Severity:** 🟡 MINOR — Code works but intent is unclear.

---

## 📋 ACCEPTANCE CRITERIA AUDIT

| AC# | Requirement | Implemented? | Evidence | Severity |
|-----|-------------|--------------|----------|----------|
| 1 | `Update.abort()` on every error after `Update.begin()` | ✅ YES | OTAUpdater.cpp L762-821 | — |
| 2 | OTA fails safely, never bricks device | ✅ YES (mostly) | prepareForOTA() + Update.abort() paths | 🟠 Missing edge case at partition setup |
| 3 | SHA/Update.end failure → SystemStatus ERROR + "Verify failed" | ✅ YES | OTAUpdater.cpp L821-827 | — |
| 4 | Bootloader rollback documented | ❌ NO | Comments exist but architecture.md not updated | 🔴 CRITICAL |
| 5 | ModeRegistry::completeOTAAttempt() restores flight rendering | ⚠️ PARTIAL | Implemented but 4s blank screen violates AC #8 | 🔴 CRITICAL |
| 6 | SystemStatus OTA_PULL subsystem exists | ✅ YES | SystemStatus.h L19, .cpp L44 | — |
| 7 | GET /api/status includes ota_pull | ✅ YES | health.js L90-92 | — |
| 8 | Failure visual shows on LEDs before restore | ❌ NO | main.cpp L1009 shows text only, no color/pattern | 🔴 CRITICAL |
| 9 | OTA pipeline doesn't block flight fetch | ✅ YES | OTA on Core 1, flight fetch unblocked | — |
| 10 | Health page renders OTA Pull status | ✅ YES | health.js dotRow pattern used | — |
| 11 | Structured failure metadata for dl-7-3 | ✅ YES | OTAFailurePhase enum, getFailurePhase() accessor | — |
| 12 | Tests include phase/retry coverage | ⚠️ WEAK | Unit tests exist but no integration test | 🟡 MINOR |

**AC Compliance: 8/12 fully met, 2/12 partial, 2/12 missing = 67% compliance**

---

## 🎯 Evidence Score Calculation

| Category | Findings | Score |
|----------|----------|-------|
| **CRITICAL Issues** | 5 (Tasks lie, AC #4/5/8 missing, 2 bugs) | +15 |
| **IMPORTANT Issues** | 4 (AC #1 edge case, 2 SOLID, test weak) | +4 |
| **MINOR Issues** | 2 (Docs, test gaps) | +0.6 |
| **Clean Categories** | Security ✅, Performance ✅ | -1.0 |
| **Total Evidence Score** | — | **18.6** |

**Verdict: 🚫 REJECT — Major rework required**

---

## 🔧 Required Actions Before Approval

1. **[BLOCKING]** Implement distinct LED failure visual (AC #8) — add color/pattern difference to "Update failed" state
2. **[BLOCKING]** Fix AC #5 violation — either skip fade transition on OTA restore OR document the 4s delay as acceptable
3. **[BLOCKING]** Update architecture.md with OTA rollback documentation (AC #4)
4. **[BLOCKING]** Add integration test for full OTA flow (AC #12)
5. **[HIGH]** Fix ModeRegistry::completeOTAAttempt() race condition — guard with timeout or atomic flag
6. **[HIGH]** Refactor OTA state machine to use `std::atomic<OTAState>`
7. **[HIGH]** Split OTA coordination into separate `OTAController` class (SOLID violation fix)
8. **[IMPORTANT]** Add null partition validation in `_downloadTaskProc()`
9. **[IMPORTANT]** Add RAII-style guards to prevent HTTPClient connection leaks
10. **[MINOR]** Add comments justifying 60s timeout choice in OTA self-check

---

## Summary

**Story Status: ❌ REJECT**

This story has **23 issues** with **5 CRITICAL blocking problems**:
- Tasks marked [x] but not implemented (4 of 7)
- AC #4, #5, #8 not satisfied
- Critical race condition in OTA state management
- Hidden memory leak and null pointer risks

The implementation is roughly 70% complete. With the action items above addressed, this story can be resubmitted for re-review.

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
<var name="session_id">506714db-2dbe-4d36-879e-1912642a0301</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="38fadf94">embedded in prompt, file id: 38fadf94</var>
<var name="story_id">dl-7.2</var>
<var name="story_key">dl-7-2-ota-failure-handling-rollback-and-retry</var>
<var name="story_num">2</var>
<var name="story_title">2-ota-failure-handling-rollback-and-retry</var>
<var name="template">False</var>
<var name="timestamp">20260416_1859</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-7.2)
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