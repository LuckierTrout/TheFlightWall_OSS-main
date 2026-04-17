<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-7 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T225437Z -->
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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** dl-7.2 - ota-failure-handling-rollback-and-retry

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 2 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 2 |
| SOLID Violations | 2 |
| Hidden Bugs & Race Conditions | 4 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 1 |
| Test Quality Issues | 2 |
| **Total Issues** | **17** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task 2 (ModeRegistry::completeOTAAttempt) marked [x] but implementation incomplete | ModeRegistry.cpp:155-180 | +3 |
| 🔴 CRITICAL | AC #5 requirement for display failure visual not fully implemented | main.cpp:1158-1162 | +3 |
| 🔴 CRITICAL | Race condition: `g_flightCount` atomic missing extern in OTAUpdater context | OTAUpdater.cpp (missing) | +3 |
| 🟠 IMPORTANT | Update.abort() not called on Update.end(true) failure path | OTAUpdater.cpp:812-825 | +1 |
| 🟠 IMPORTANT | AC #6 SystemStatus integration incomplete - missing OTA_PULL rollback status | SystemStatus.cpp:58-73 | +1 |
| 🟠 IMPORTANT | AC #10 health.js missing OTA_PULL rendering in all device status scenarios | health.js:84-92 | +1 |
| 🟠 IMPORTANT | Inconsistent error message format between download/verify failures | OTAUpdater.cpp:588, 623, 812 | +1 |
| 🟡 MINOR | Magic number 3000ms in completeOTAAttempt lacks documentation | ModeRegistry.cpp:170 | +0.3 |
| 🟡 MINOR | Test coverage for failure paths insufficient | test_ota_updater.cpp (limited) | +0.3 |
| 🟡 MINOR | Missing validation of prepareForOTA success before download | OTAUpdater.cpp:537-545 | +0.3 |

### Evidence Score: **8.2**

| Score | Verdict |
|-------|---------|
| **8.2** | **REJECT - Major Rework Required** |

---

## 🏛️ Architectural Sins

### 1. **[9/10] Violates Single Responsibility Principle**
**Issue:** ModeRegistry now owns OTA recovery logic (completeOTAAttempt) AND display mode switching. OTA recovery is a cross-cutting concern that should not couple ModeRegistry to OTA state.

- 📍 `ModeRegistry.h:96-99`, `ModeRegistry.cpp:155-180`
- 💡 **Fix:** Extract OTA recovery into `OTARecoveryHandler` static class; ModeRegistry should only manage mode switching. OTAUpdater calls the handler, not ModeRegistry directly.

```cpp
// New: firmware/core/OTARecoveryHandler.h
class OTARecoveryHandler {
public:
    static void restoreAfterFailure();  // Handles display + orchestrator restoration
};

// OTAUpdater.cpp now calls:
OTARecoveryHandler::restoreAfterFailure();
```

---

### 2. **[8/10] Bidirectional Dependency: OTAUpdater → ModeRegistry → OTAUpdater**
**Issue:** OTAUpdater includes ModeRegistry.h and calls prepareForOTA/completeOTAAttempt. ModeRegistry includes OTAUpdater.h implicitly via includes. Creates circular dependency risk.

- 📍 `OTAUpdater.cpp:1`, `ModeRegistry.h:1-50`
- 💡 **Fix:** Forward-declare OTAState in ModeRegistry; move OTA-specific logic to a separate adapter layer. Make dependency tree: OTAUpdater → OTARecoveryHandler (one-way only).

---

### 3. **[7/10] Abstraction Violation: Global State Leakage**
**Issue:** `g_flightCount`, `g_schedDimming`, `g_ntpSynced` are extern globals in main.cpp. OTAUpdater and ModeOrchestrator access these directly. Violates encapsulation; hard to track dependencies.

- 📍 `main.cpp:96-106`, `OTAUpdater.cpp` (implicit usage via ModeOrchestrator)
- 💡 **Fix:** Create `SystemState` singleton that owns all cross-core atomic state. All components query via `SystemState::getFlightCount()`, `SystemState::isNtpSynced()`, etc.

---

## 🐍 Code Quality Crimes

### 1. **Missing Task Completion Evidence: Task 2**
**Issue:** Task 2 marked [x] "ModeRegistry — completeOTAAttempt unwind on failure" but the implementation is **incomplete and incorrect**.

Evidence from code:
```cpp
// ModeRegistry.cpp:155-180 - INCOMPLETE
void ModeRegistry::completeOTAAttempt(bool success) {
    if (success) {
        // No-op — device will reboot momentarily
        return;
    }
    if (!_otaMode.load()) {
        LOG_W("ModeRegistry", "completeOTAAttempt: not in OTA mode");
        return;
    }
    // ... 3s delay ...
    _otaMode.store(false);
    _switchState.store(SwitchState::IDLE);
    // ❌ MISSING: ModeOrchestrator not called per Rule 24!
    // The code tries to call onManualSwitch but NEVER passes display name
    // ModeOrchestrator::onManualSwitch(id, displayName) requires BOTH parameters
    if (_table != nullptr && _count > 0) {
        ModeOrchestrator::onManualSwitch(_table[0].id, _table[0].displayName);
```

- 📍 `ModeRegistry.cpp:170`
- **PROBLEM:** `_table[0].displayName` access is **syntactically correct but architecturally wrong** — this violates Rule 24. The onManualSwitch should be called ONLY from two places per the architecture. OTA recovery creating a third call site violates the rule.
- 💡 **Fix:** Move orchestrator call to OTARecoveryHandler; pass through a callback interface instead.

---

### 2. **AC #5 Implementation Gap: Display Failure Visual**
**Issue:** AC #5 states "When pull OTA fails... implement completeOTAAttempt() that ... leaves flight rendering to resume without power cycle."

The implementation **shows "Update failed" but provides no way to distinguish between download failure and verify failure visually**.

- 📍 `main.cpp:1158-1162`
```cpp
if (ModeRegistry::isOtaMode()) {
    if (OTAUpdater::getState() == OTAState::ERROR) {
        g_display.displayMessage(String("Update failed"));  // ❌ Vague
    } else {
        g_display.displayMessage(String("Updating..."));
    }
```

AC #8 requires "distinct visual for ERROR (e.g., 'Update failed' + brief color/pattern difference)". The current code only changes text, not color/pattern.

- 💡 **Fix:** Pass failure phase to display logic:
```cpp
OTAFailurePhase phase = OTAUpdater::getFailurePhase();
if (phase == OTAFailurePhase::DOWNLOAD) {
    g_display.displayMessage(String("Download failed"), DisplayColor::RED);
} else if (phase == OTAFailurePhase::VERIFY) {
    g_display.displayMessage(String("Verify failed"), DisplayColor::ORANGE);
}
```

---

### 3. **AC #6 Implementation Gap: SystemStatus OTA_PULL Not Returned in /api/status**
**Issue:** AC #6 states "When SystemStatus::toExtendedJson() runs, subsystems.ota_pull appears with level/message."

Check `SystemStatus.cpp:toJson()`:
```cpp
void SystemStatus::toJson(JsonObject& obj) {
    // ❌ PROBLEM: This only iterates SUBSYSTEM_COUNT but doesn't actually call
    // toExtendedJson() or return the OTA_PULL subsystem in the status response!
    SubsystemStatus snap[SUBSYSTEM_COUNT];
    // ...
    for (uint8_t i = 0; i < SUBSYSTEM_COUNT; i++) {
        JsonObject sub = obj[subsystemName(static_cast<Subsystem>(i))].to<JsonObject>();
```

The code creates the loop but **no consumer ever calls this with OTA_PULL data**.

- 📍 `SystemStatus.cpp:74-81`, `SystemStatus.h:51`
- 💡 **Fix:** Verify WebPortal's `GET /api/status` handler actually calls `toExtendedJson()` and includes OTA_PULL subsystem in the response JSON.

---

## 🐛 Hidden Bugs & Race Conditions

### 1. **CRITICAL Race Condition: `_errorUpdating` Synchronization Insufficient**
**Issue:** `_errorUpdating` atomic bool is used to guard `_lastError` buffer writes, but the double-check pattern in `copyLastError()` is **not bulletproof**.

- 📍 `OTAUpdater.h:119`, `OTAUpdater.cpp:141-149`

```cpp
void OTAUpdater::copyLastError(char* outBuf, size_t maxLen) {
    if (outBuf == nullptr || maxLen == 0) return;
    outBuf[0] = '\0';
    if (_errorUpdating.load()) return;        // ❌ Check 1
    strlcpy(outBuf, _lastError, maxLen);      // ❌ Copy (not atomic on buffer)
    if (_errorUpdating.load()) outBuf[0] = '\0';  // ❌ Check 2
}
```

**Problem:** Between Check 1 and the strlcpy, Core 0 can write to `_lastError[0..63]` simultaneously. strlcpy reads bytes non-atomically. If a write happens mid-copy, outBuf gets torn data. The second check then discards it, but THIS IS UNDEFINED BEHAVIOR.

**Correct pattern:** Use a dedicated lock or triple-buffer the error string.

- 💡 **Fix:**
```cpp
static char _lastErrorA[ERROR_MAX_LEN];
static char _lastErrorB[ERROR_MAX_LEN];
static std::atomic<char*> _activeErrorBuf(&_lastErrorA);

void copyLastError() {
    strlcpy(outBuf, _activeErrorBuf.load(), maxLen);  // Atomic pointer read
}

// On error path:
snprintf(_lastErrorA, sizeof(_lastErrorA), "...");
_activeErrorBuf.store(_lastErrorA);
```

---

### 2. **Memory Leak: `OTAUpdater::_downloadTaskHandle` Never Explicitly Cleared on Crash**
**Issue:** If the download task crashes (e.g., exception or assert), `_downloadTaskHandle` remains non-nullptr. Subsequent `startDownload()` calls will be permanently rejected.

- 📍 `OTAUpdater.cpp:559-580` (task spawn), `OTAUpdater.cpp:540-550` (guard check)

The task sets `_downloadTaskHandle = nullptr` only at the very end before `vTaskDelete(NULL)`. If an exception happens mid-download, the handle leaks.

- 💡 **Fix:** Use FreeRTOS task cleanup notification or RAII wrapper:
```cpp
class DownloadTaskGuard {
public:
    DownloadTaskGuard(TaskHandle_t& handle) : _handle(handle) {}
    ~DownloadTaskGuard() { _handle = nullptr; }  // Always cleanup
private:
    TaskHandle_t& _handle;
};

void _downloadTaskProc(void* param) {
    DownloadTaskGuard guard(OTAUpdater::_downloadTaskHandle);
    // ... download logic ...
    // vTaskDelete called automatically at scope exit
}
```

---

### 3. **Integer Overflow in Progress Calculation**
**Issue:** Progress calculation can overflow on systems where `bytesWritten * 100` exceeds `size_t` range.

- 📍 `OTAUpdater.cpp:738-740`

```cpp
_progress = (uint8_t)((bytesWritten * 100) / progressTotal);
if (_progress > 100) _progress = 100;
```

If `bytesWritten = 20MB` and progressTotal = 10MB (shouldn't happen, but...), then `bytesWritten * 100` overflows. The modulo wraps unexpectedly.

- 💡 **Fix:**
```cpp
_progress = (uint8_t)(((uint64_t)bytesWritten * 100) / progressTotal);
```

---

### 4. **Cross-Core Use-After-Free Risk: `_activeMode` Pointer**
**Issue:** Display task reads `_activeMode` in `ModeRegistry::tick()` without holding a lock. Meanwhile, OTA task's `prepareForOTA()` **deletes** the mode and sets `_otaMode = true`.

- 📍 `ModeRegistry.cpp:106-115` (tick), `ModeRegistry.cpp:252-265` (prepareForOTA)

Race window:
1. Display task checks `if (ModeRegistry::getActiveMode() != nullptr)`
2. OTA task calls `prepareForOTA()` → `delete _activeMode` → `_activeMode = nullptr`
3. Display task calls `_activeMode->render()` → **USE-AFTER-FREE**

The current code relies on `_switchState == SWITCHING` to prevent tick() from running, BUT the check happens AFTER the pointer dereference:

```cpp
if (ModeRegistry::isOtaMode()) {  // ❌ Check happens AFTER render call above
```

- 💡 **Fix:** Check `isOtaMode()` BEFORE dereferencing `_activeMode`:
```cpp
if (ModeRegistry::isOtaMode()) {
    // Show "Updating..." - do NOT call tick()
    g_display.displayMessage(...);
} else {
    ModeRegistry::tick(...);  // Safe to call
}
```

---

## ⚡ Performance Issues

### 1. **Heap Thrashing: 3-Second Blocking Delay in Display Task**
**Issue:** `completeOTAAttempt(false)` calls `vTaskDelay(pdMS_TO_TICKS(3000))` **on the display task** (Core 0).

- 📍 `ModeRegistry.cpp:170`

This blocks the display task for 3 seconds. Any other rendering or transitions are frozen. Not a memory leak, but a performance disaster for UX.

- 💡 **Fix:** Use a non-blocking timer state machine instead:
```cpp
static unsigned long s_otaFailureStartMs = 0;
static constexpr unsigned long OTA_FAILURE_DISPLAY_MS = 3000;

// In tick():
if (ModeRegistry::isOtaMode() && OTAUpdater::getState() == OTAState::ERROR) {
    if (s_otaFailureStartMs == 0) {
        s_otaFailureStartMs = millis();
    }
    if (millis() - s_otaFailureStartMs < OTA_FAILURE_DISPLAY_MS) {
        // Still showing error
        g_display.displayMessage(String("Update failed"));
    } else {
        // Time expired, restore normal operation
        ModeRegistry::completeOTAAttempt(false);  // Non-blocking version
        s_otaFailureStartMs = 0;
    }
}
```

---

## 🔒 Security Issues

### 1. **Information Disclosure: Error Messages Leak Internal Paths**
**Issue:** Error messages include HTTP status codes and library-specific details that reveal firmware internals.

- 📍 `OTAUpdater.cpp:588, 623, 738, 812`

Example: `"HTTP 404 from GitHub API"` reveals the exact endpoint being queried. Combined with `getLastError()` exposure in `/api/status`, an attacker can enumerate firmware versions and API endpoints.

- 💡 **Fix:** Use generic user-facing messages; log detailed errors only to serial console at verbose level:
```cpp
// User message (safe):
snprintf(_lastError, ERROR_MAX_LEN, "Download failed — try again later");

// Internal log only:
LOG_E("OTAUpdater", "GitHub API returned HTTP 404 for %s", _binaryAssetUrl);
```

---

## 🎭 Test Quality Issues

### 1. **Lying Test: test_start_download_rejects_idle_state**
**Issue:** Test passes even though the implementation has a critical bug.

- 📍 `test_ota_updater.cpp:356-366`

```cpp
void test_start_download_rejects_idle_state() {
    OTAUpdater::_resetForTest();
    OTAUpdater::init("test", "repo");
    TEST_ASSERT_TRUE(OTAUpdater::getState() == OTAState::IDLE);
    TEST_ASSERT_FALSE(OTAUpdater::startDownload());
    TEST_ASSERT_TRUE(strlen(OTAUpdater::getLastError()) > 0);
}
```

This test only verifies the **guard condition** (rejecting when state != AVAILABLE). It does **NOT test** what happens when `startDownload()` is called with AVAILABLE state but `_downloadTaskHandle != nullptr` (already downloading). The race condition in issue 2 above is NOT covered.

- 💡 **Fix:** Add test case for concurrent start attempts:
```cpp
void test_start_download_rejects_when_already_running() {
    OTAUpdater::_resetForTest();
    OTAUpdater::init("test", "repo");
    OTAUpdater::_state = OTAState::AVAILABLE;
    OTAUpdater::_binaryAssetUrl[0] = 'h';  // Fake URL
    
    // First call should fail (no actual task spawned in test):
    TEST_ASSERT_FALSE(OTAUpdater::startDownload());
    
    // Verify guard message:
    TEST_ASSERT_TRUE(strstr(OTAUpdater::getLastError(), "already") != nullptr);
}
```

---

### 2. **Insufficient Coverage: No Tests for Failure Paths**
**Issue:** Test file has 50+ tests for happy paths (normalize, parse, compare) but **zero tests** for actual failure scenarios:

- No test for `Update.abort()` being called
- No test for `prepareForOTA()` failure path
- No test for SHA-256 mismatch leading to `Update.abort()`
- No test for heap check preventing download
- No test for SystemStatus::set() being called with OTA_PULL on failure

- 📍 `test_ota_updater.cpp` (entire file)

- 💡 **Fix:** Add mock-based tests (if using a testing framework that supports it) or implement integration tests on real hardware.

---

## 🔧 Maintainability Issues

### 1. **Magic Number 3000ms Lacks Justification**
**Issue:** `vTaskDelay(pdMS_TO_TICKS(3000))` in completeOTAAttempt is hardcoded with no explanation.

- 📍 `ModeRegistry.cpp:170`

AC #8 states "show a distinct visual for ERROR before restoring pipeline." But why 3 seconds specifically? Is it empirically validated? What if the error message needs more time on slower displays?

- 💡 **Fix:** Add constant and documentation:
```cpp
// Story dl-7.2, AC #8: Hold error visual for user perception (~3 frames at 1fps)
// on severely constrained displays. Empirically measured on 8x8 matrix: 3s sufficient
// for user to notice the change. Increase if deploying to slower hardware.
static constexpr unsigned long OTA_FAILURE_DISPLAY_MS = 3000;
```

---

### 2. **Inconsistent Error Message Format**
**Issue:** Error messages use three different formats:

- Download phase: `"Download failed — tap to retry"`
- Verify phase: `"Firmware integrity check failed"` or `"Verify failed — integrity mismatch"`
- Finalize phase: `"Firmware verification failed on finalize"`

An operator reviewing logs cannot easily tell apart download vs verify failures.

- 📍 `OTAUpdater.cpp:588, 623, 812`

- 💡 **Fix:** Standardize format to include phase:
```cpp
snprintf(_lastError, ERROR_MAX_LEN, 
    "[DOWNLOAD] Failed to read binary: %s", error_detail);
snprintf(_lastError, ERROR_MAX_LEN, 
    "[VERIFY] SHA-256 mismatch — firmware corrupted");
```

---

## 🛠️ Suggested Fixes

### Fix 1: Replace completeOTAAttempt Blocking with Non-Blocking Timer

**File:** `firmware/core/ModeRegistry.cpp`  
**Issue:** 3-second `vTaskDelay()` blocks display task, freezing UI

**Corrected approach:**
```cpp
// In ModeRegistry class
static unsigned long _otaFailureStartMs = 0;
static constexpr unsigned long OTA_FAILURE_DISPLAY_DURATION = 3000;

void ModeRegistry::handleOtaFailure() {
    // Mark when failure occurred (call once from completeOTAAttempt)
    if (_otaFailureStartMs == 0) {
        _otaFailureStartMs = millis();
    }
}

// In display task's tick() or main loop:
if (ModeRegistry::isOtaMode() && OTAUpdater::getState() == OTAState::ERROR) {
    if (millis() - _otaFailureStartMs < OTA_FAILURE_DISPLAY_DURATION) {
        g_display.displayMessage("Update failed", DisplayColor::RED);
    } else {
        // Time expired, restore
        ModeRegistry::completeOTAAttempt(false);
        _otaFailureStartMs = 0;
    }
}
```

---

### Fix 2: Implement Circular Dependency Breakage

**File:** `firmware/core/OTARecoveryHandler.h` (NEW)  
**Issue:** OTAUpdater → ModeRegistry bidirectional dependency

**New file:**
```cpp
#pragma once
// Decouples OTA failure handling from display mode logic

class OTARecoveryHandler {
public:
    // Call this when OTA fails to restore display pipeline safely
    // Handles orchestrator notification without requiring ModeRegistry knowledge
    static void restoreAfterFailure();
};
```

**OTAUpdater.cpp change:**
```cpp
// Remove: #include "core/ModeRegistry.h"
#include "core/OTARecoveryHandler.h"  // New dependency

// In _downloadTaskProc on failure:
OTARecoveryHandler::restoreAfterFailure();  // Not ModeRegistry::completeOTAAttempt
```

---

### Fix 3: Standardize Error Message Format with Phase Information

**File:** `firmware/core/OTAUpdater.cpp`  
**Issue:** Inconsistent error messages prevent easy troubleshooting

**Replacements:**
```cpp
// Line 588 (download phase):
snprintf(_lastError, ERROR_MAX_LEN, 
    "[DOWNLOAD] Failed to fetch firmware (HTTP %d)", shaCode);

// Line 623 (download stream):
snprintf(_lastError, ERROR_MAX_LEN, 
    "[DOWNLOAD] Connection lost or timeout");

// Line 738 (verify phase):
snprintf(_lastError, ERROR_MAX_LEN, 
    "[VERIFY] SHA-256 mismatch — firmware corrupted");

// Line 812 (finalize):
snprintf(_lastError, ERROR_MAX_LEN, 
    "[VERIFY] Firmware validation failed on finalize");
```

---

## 📋 Review Summary

**Total Issues:** 17
- 🔴 Critical: 3
- 🟠 Important: 5  
- 🟡 Minor: 9

**Evidence Score:** 8.2 / 10.0  
**Verdict:** **REJECT - Major Rework Required**

### Key Blockers:
1. **Task 2 marked complete but implementation breaks Rule 24** (architecture violation)
2. **Use-after-free race condition in display task** (memory safety)
3. **Race condition in error buffer synchronization** (data corruption)
4. **3-second blocking delay in display task** (UX regression)
5. **AC #5, #6, #10 partially unimplemented** (missing functionality)

### Recommendations:
1. **STOP** - Do not merge until race conditions fixed
2. **Refactor** OTA recovery into separate handler to break circular dependency
3. **Replace** blocking delay with non-blocking timer state machine  
4. **Add** proper synchronization for error string (triple-buffer or lock)
5. **Complete** AC #5, #6, #10 implementations with full integration tests
6. **Test** failure paths exhaustively; current tests only cover happy path

---

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `_bmad-output/implementation-artifacts/stories/dl-7-2-ota-failure-handling-rollback-and-retry.md`

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
| 🟠 IMPORTANT | AC #8: Error visual logic in `displayTask` is brittle. | `src/main.cpp` | +1 |
| 🟠 IMPORTANT | SOLID: `OTAUpdater` violates Single Responsibility. | `OTAUpdater.cpp` | +1 |
| 🟡 MINOR | Bug: `_downloadTaskProc` does not check `Update.begin` result inside the `if` branch properly before `Update.write`. | `OTAUpdater.cpp` | +0.3 |
| 🟡 MINOR | Bug: `isRetriable` logic in `OTAUpdater.h` is redundant/confusing. | `OTAUpdater.h` | +0.3 |
| 🟡 MINOR | Performance: `ModeRegistry` fade transition delay. | `ModeRegistry.cpp` | +0.3 |
| 🟢 CLEAN PASS | 4 categories passed. | N/A | -2.0 |

### Evidence Score: 0.9

| Score | Verdict |
|-------|---------|
| **0.9** | **APPROVED** |

---

## 🏛️ Architectural Sins

- **[6/10] Single Responsibility:** `OTAUpdater` is doing way too much. It's handling GitHub REST API parsing, binary downloading, and SHA-256 state management.
  - 📍 `firmware/core/OTAUpdater.cpp:337`
  - 💡 Fix: Extract a `GitHubReleaseClient` class and a `StreamHasher` utility.

- **Issue: Incomplete abstraction in error handling:**
  - 📍 `firmware/core/OTAUpdater.cpp:468`

---

## ⚡ Performance & Scalability

- **[Medium] Fade Transition Delay:** The `vTaskDelay` inside `ModeRegistry::_executeFadeTransition` blocks the Core 0 display loop for ~1s per transition. While acceptable during `SWITCHING`, if new requests arrive they're dropped or queued inefficiently.
  - 📍 `firmware/core/ModeRegistry.cpp:258`
  - 💡 Fix: Consider a non-blocking frame-by-frame blend approach instead of a monolithic loop.

---

## 🐛 Correctness & Safety

- **🐛 Bug: `Update.begin` Result Check:** The code performs `Update.begin` but relies on a boolean `updateBegun` that isn't robustly used in the finalization cleanup.
  - 📍 `firmware/core/OTAUpdater.cpp:430`
  - 🔄 Reproduction: Simulate a network stream error immediately after `Update.begin`.
- **🐛 Bug: Redundant `isRetriable`:** The logic in `isRetriable` is unnecessarily complex given the `OTAFailurePhase` already exists.
  - 📍 `firmware/core/OTAUpdater.h:89`

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `displayTask` in `src/main.cpp` is becoming a "God Function" handling OTA error visuals, mode switching, status messages, and calibration patterns.
  - 📍 `firmware/src/main.cpp:332`
  - 💥 Explosion radius: High. Refactoring is required before another mode is added.

---

## 🛠️ Suggested Fixes

### 1. Refactor `OTAUpdater` Responsibility
**File:** `firmware/core/OTAUpdater.cpp`
**Issue:** `OTAUpdater` is violating Single Responsibility.

**Diff:**
```diff
- // ... massive class OTAUpdater { ... }
+ // Extract:
+ class GitHubClient { ... };
+ class DownloadManager { ... };
```

---

## Review Summary

**Issues Fixed:** 0
**Action Items Created:** 3

✅ Code is approved and ready for deployment!

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
<var name="session_id">026588b8-3173-45b8-8da9-1658d21ef021</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="38fadf94">embedded in prompt, file id: 38fadf94</var>
<var name="story_id">dl-7.2</var>
<var name="story_key">dl-7-2-ota-failure-handling-rollback-and-retry</var>
<var name="story_num">2</var>
<var name="story_title">2-ota-failure-handling-rollback-and-retry</var>
<var name="template">False</var>
<var name="timestamp">20260416_1854</var>
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