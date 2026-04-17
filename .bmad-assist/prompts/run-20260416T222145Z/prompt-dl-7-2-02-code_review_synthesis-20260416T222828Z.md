<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-7 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T222828Z -->
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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**ADVERSARIAL CODE REVIEW COMPLETE, Christian**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-7-2-ota-failure-handling-rollback-and-retry.md`

---

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 (cannot fully reconcile without live `git diff`; working tree shows many unrelated edits — story File List is a narrow slice) |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 0 (tasks appear substantively implemented from provided sources) |
| SOLID / Boundary Violations | 2 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 (TLS `setInsecure()` is pre-existing tradeoff; not introduced by dl-7.2) |
| Test Honesty / Coverage Gaps | 2 |
| **Total Issues** | **10** |

---

## Acceptance Criteria Matrix (evidence-based)

| AC | Verdict | Notes |
|----|---------|-------|
| **#1** (`Update.abort()` on all post-`Update.begin()` failures) | **PARTIAL** | Most branches call `Update.abort()`, but the `Update.end(true)` failure path does not call `Update.abort()` after a failed finalize (`firmware/core/OTAUpdater.cpp` success tail). Architecture rule 25 explicitly expects abort discipline after `begin()`. |
| **#2** (WiFi drop → `OTAState::ERROR`, `getLastError()` string, `SystemStatus::OTA_PULL` w/ download phase) | **PARTIAL** | `_lastError` often matches “Download failed — tap to retry”, but `SystemStatus::set` messages are sometimes generic (“HTTP error”, “no stream”) and not consistently the same human string as `_lastError` / epic wording. |
| **#3** (verify failures → verify phase + retriable semantics) | **IMPLEMENTED** | `_failurePhase = VERIFY` + distinct `SystemStatus` strings + `Update.abort()` on SHA mismatch. |
| **#4** (bootloader rollback documentation) | **IMPLEMENTED** | Comment block in `firmware/core/OTAUpdater.h` / task proc header. |
| **#5** (`completeOTAAttempt` restores pipeline without power cycle) | **PARTIAL / RISK** | Unfreezes rendering, but recovery **always** routes `ModeOrchestrator::onManualSwitch(_table[0].id, …)` — not “previous manual mode” / orchestrator truth. This can override user intent and diverges from the story’s “per architecture” wording. |
| **#6** (`OTA_PULL` enum + mappings) | **IMPLEMENTED** | `SystemStatus.h/.cpp` as provided. |
| **#7** (`subsystems.ota_pull` in extended status) | **IMPLEMENTED** | `toExtendedJson` composes `subsystems` via `toJson`. |
| **#8** (distinct LED failure visual; no long blank) | **PARTIAL** | Different text (“Update failed” vs “Updating…”), but **no** distinct color/pattern requirement beyond text; still arguably meets “e.g.” minimally. |
| **#9** (do not block flight fetch path) | **IMPLEMENTED** | Failure delay is inside the OTA download task, not `loop()`’s fetch cadence. |
| **#10** (health UI + gzip) | **IMPLEMENTED** (from story notes) | `health.js` change present; gzip regen claimed — **not independently verified** here. |
| **#11** (structured accessors for dl-7-3) | **PARTIAL** | `getFailurePhase()` / `isRetriable()` exist in `OTAUpdater`, but **this review did not receive** `WebPortal.cpp` proving `GET /api/ota/status` emits `failure_phase` / `retriable` JSON — AC11 has a delivery risk if API lags accessors. |
| **#12** (tests: abort coverage via mocks) | **PARTIAL** | Tests added are mostly **pure helpers + enums**; no meaningful `Update.abort()` / task orchestration coverage. |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| IMPORTANT | Missing `Update.abort()` after failed `Update.end(true)` (post-`begin()` finalize failure) | `firmware/core/OTAUpdater.cpp` (`_downloadTaskProc`, finalize block) | +1 |
| IMPORTANT | OTA failure recovery forces `MODE_TABLE[0]` instead of restoring orchestrator “manual” selection / NVS truth | `firmware/core/ModeRegistry.cpp` (`completeOTAAttempt`) | +1 |
| IMPORTANT | `prepareForOTA()` failure is logged but download continues anyway (“continuing anyway”) — undermines AC2/AC9 safety story | `firmware/core/OTAUpdater.cpp` (`_downloadTaskProc` preface) | +1 |
| IMPORTANT | `ModeRegistry` now depends on `ModeOrchestrator` (core↔core coupling / cycle risk) | `firmware/core/ModeRegistry.cpp` includes + call | +1 |
| IMPORTANT | “Lying” / low-signal tests: `getStateString` / `getFailurePhaseString` “known values” tests accept almost anything | `firmware/test/test_ota_updater/test_main.cpp` | +1 |
| MINOR | AC8 asks for visually distinct failure treatment; implementation is mostly string swap | `firmware/src/main.cpp` + `ModeRegistry::completeOTAAttempt` timing | +0.3 |
| MINOR | `.sha256` fetch uses `HTTPClient::getString()` (bounded, but still allocates a `String`) — minor heap pressure vs streaming ethos | `firmware/core/OTAUpdater.cpp` | +0.3 |
| MINOR | `OTAUpdater::getState()` / `_state` not `std::atomic` — cross-task visibility is “probably OK” on ESP32 for byte-sized enum, but it is not a hard guarantee | `firmware/core/OTAUpdater.h/.cpp` | +0.3 |
| CLEAN PASS | No new obvious injection/auth surface in reviewed snippets | — | -0.5 |

### Evidence Score: **5.4**

| Score | Verdict |
|-------|---------|
| **5.4** | **MAJOR REWORK** |

---

## Architectural / Boundary Issues

- **[7/10] Dependency Inversion / layering:** `ModeRegistry` (mode lifecycle + OTA prep) now calls `ModeOrchestrator::onManualSwitch` directly (`firmware/core/ModeRegistry.cpp`). That tightens a dependency cycle risk (`ModeOrchestrator` ↔ `ModeRegistry`) and moves orchestration policy into the registry’s OTA unwind path.
  - Fix: Prefer an explicit “resume rendering” API on `ModeOrchestrator` (or a tiny `OtaDisplayGate` façade) that restores **manual** mode from orchestrator-owned state, not `table[0]`.

- **[6/10] Single Responsibility:** `completeOTAAttempt` encodes product policy (3s UX delay + default mode selection) inside `ModeRegistry`, mixing **transport failure recovery** with **mode policy**.
  - Fix: Keep `ModeRegistry` mechanical (`_otaMode`, `_switchState`, maybe `requestSwitch` scheduling), push policy to `ModeOrchestrator` / `OTAUpdater` coordination.

---

## Pythonic Crimes & Readability

- N/A (C++/JS). Minor: duplicated “Download failed — tap to retry” literals across `_lastError` vs `SystemStatus` messages increases drift risk (`firmware/core/OTAUpdater.cpp`).

---

## Performance & Scalability

- **[Low/Medium]** `completeOTAAttempt(false)` blocks the **OTA download task** for **3000ms** (`vTaskDelay`) (`firmware/core/ModeRegistry.cpp`). This is intentional for UX, but it pins a Core-1 task during failure recovery; acceptable for a one-shot OTA task, still a footgun if you later reuse the task for anything else.

---

## Correctness & Safety

- **Bug (Rule 25):** If `Update.end(true)` fails, code returns without `Update.abort()` (`firmware/core/OTAUpdater.cpp`). Depending on ESP32 `Update` implementation, this can leave the update subsystem in a bad state and violates the story’s explicit “every path calls `Update.abort()` before leaving the download task” requirement for post-`begin()` failures.

- **Bug (Recovery semantics):** After failure, `ModeOrchestrator::onManualSwitch(_table[0].id, …)` can **clobber** the user’s persisted manual selection / orchestrator state vs “resume where we were” (`firmware/core/ModeRegistry.cpp`).

- **Bug (Safety footgun):** If `ModeRegistry::prepareForOTA()` returns `false`, `_downloadTaskProc` still proceeds (`firmware/core/OTAUpdater.cpp`). That means you may flash OTA while the display pipeline is not in the intended “SWITCHING/OTA prep” contract — higher risk of concurrent assumptions even if it “usually works”.

---

## Lying / Weak Tests

- **`test_get_state_string_known_values` / `test_get_failure_phase_string_known_values`:** These accept a wide disjunction of strings and will pass even if string mapping regressions occur (`firmware/test/test_ota_updater/test_main.cpp`).
- **No test** asserts the presence/order of `Update.abort()` calls across branches (AC12 claimed mock-based abort coverage; not delivered in the provided test file).

---

## Maintainability / Tech Debt

- **Magic timing:** `3000ms` hold for error visual is undocumented vs frame rate assumptions (`firmware/core/ModeRegistry.cpp`).
- **Policy duplication:** OTA failure strings split across `_lastError`, `SystemStatus::set`, and UI (`main.cpp` display string) — future inconsistency likely.

---

## Suggested Fixes (prioritized)

1. **Add `Update.abort()`** on the `!Update.end(true)` failure path (and audit any other post-`begin()` exits).
2. **Change recovery** to restore **orchestrator manual mode** (read from `ModeOrchestrator` / NVS), not `MODE_TABLE[0]` unconditionally — or explicitly document + test that `table[0]` is the canonical safe mode and update the story AC to match reality.
3. **If `prepareForOTA()` fails**, fail fast: set `ERROR`, set `SystemStatus`, call `completeOTAAttempt(false)` (if appropriate), and **do not** call `Update.begin()`.
4. **Strengthen AC8** with a real visual delta (color/pattern) without violating FR35 “intentional full-screen OTA states” — e.g., red border wipe for ~300–500ms + shorter hold than 3s if desired.
5. **Replace weak string tests** with deterministic unit tests of `getStateString()`/`getFailurePhaseString()` by temporarily forcing `_state` / `_failurePhase` via a test-only seam (friend/test hook), or split mapping into a pure `constexpr` table tested directly.
6. **Verify `WebPortal`** actually serializes `failure_phase` + `retriable` on `GET /api/ota/status` (AC11 end-to-end), or track as a follow-up for dl-7-3 with explicit contract tests.

---

## Review Summary

**Issues Fixed:** 0  
**Action Items Created:** 0  

**Verdict:** **MAJOR REWORK** — the work is directionally right (status subsystem, failure phases, recovery hook, health UI), but there are **real correctness gaps** (`Update.end` abort discipline), **policy bugs** (default mode restore vs manual restore), and **test honesty** problems relative to AC12 and the story’s own bar.

If you want auto-fixes, approve a patch series in this order: (1) `Update.abort()` finalize failure, (2) orchestrator-correct restore path, (3) hard fail when `prepareForOTA()` fails, (4) tighten tests + API JSON contract.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-7-2-ota-failure-handling-rollback-and-retry.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 1 |
| SOLID Violations | 2 |
| Hidden Bugs | 3 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **11** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Critical cross-core race condition in `ModeRegistry::prepareForOTA` teardown | `firmware/core/ModeRegistry.cpp` | +3 |
| 🔴 CRITICAL | OTA failure paths do not reliably restore flight rendering pipeline | `firmware/core/OTAUpdater.cpp` | +3 |
| 🔴 CRITICAL | Pull OTA continues download even if `prepareForOTA()` fails | `firmware/core/OTAUpdater.cpp` | +3 |
| 🟠 IMPORTANT | Incomplete AC #12 testing: `prepareForOTA` test is ignored | `firmware/test/test_mode_registry/test_main.cpp` | +1 |
| 🟡 MINOR | Duplicate/Inconsistent error handling between Push and Pull OTA | `firmware/core/OTAUpdater.cpp` / `WebPortal.cpp` | +0.3 |

### Evidence Score: 10.3

| Score | Verdict |
|-------|---------|
| **10.3** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] Dependency Inversion:** `WebPortal.cpp` tightly couples OTA upload logic to `ModeRegistry::prepareForOTA()`, hardcoding a core orchestration dependency into a web adapter.
  - 📍 `firmware/adapters/WebPortal.cpp`
  - 💡 Fix: Define an `OTAProvider` interface and inject it into `WebPortal`.
- **[9/10] Boundary Breach:** `ModeRegistry::prepareForOTA()` mutates `_activeMode` directly on Core 1 while `ModeRegistry::tick()` and `_activeMode->render()` run on Core 0.
  - 📍 `firmware/core/ModeRegistry.cpp`
  - 💡 Fix: `ModeRegistry` should only enqueue a quiesce request; Core 0 should perform the actual teardown.

---

## ⚡ Performance & Scalability

- **[Moderate] Performance:** OTA download task uses `vTaskDelay(pdMS_TO_TICKS(1))` for yields during chunk streaming. Given 4KB chunks and 10Mbps link, this causes excessive context switching.
  - 📍 `firmware/core/OTAUpdater.cpp`
  - 💡 Fix: Yield only every N chunks or based on time, not every chunk.

---

## 🐛 Correctness & Safety

- **🐛 Bug (Race Condition):** `_otaMode` is accessed cross-core as a `bool` without synchronization, leading to potential torn reads and inconsistent state transitions between Core 0 and Core 1.
  - 📍 `firmware/core/ModeRegistry.h`
  - 🔄 Reproduction: Call `isOtaMode()` from Core 0 during a rapid-fire OTA state change initiated by Core 1.
- **🐛 Bug (UAF Risk):** `ModeRegistry::prepareForOTA()` deletes `_activeMode` on Core 1 while Core 0 might be actively rendering, leading to a classic use-after-free scenario.
  - 📍 `firmware/core/ModeRegistry.cpp`
  - 🔄 Reproduction: Initiate OTA while rendering a complex flight card with large bitmaps.
- **🐛 Bug (OTA Flow):** The push OTA failure path fails to restore the renderer, leaving the system in a permanent "Updating..." state.
  - 📍 `firmware/adapters/WebPortal.cpp`
  - 🔄 Reproduction: Initiate a push OTA, then fail the `Update.begin()` call by providing an oversized binary.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** OTA cleanup logic is duplicated between `WebPortal.cpp` (Push) and `OTAUpdater.cpp` (Pull), and already inconsistent. This is a maintenance nightmare waiting to regress.
  - 📍 `firmware/adapters/WebPortal.cpp` / `firmware/core/OTAUpdater.cpp`
  - 💥 Explosion radius: High - requires simultaneous updates to two disparate files for every change.

---

## 🛠️ Suggested Fixes

### 1. Unified OTA Teardown

**File:** `firmware/core/ModeRegistry.cpp`
**Issue:** Core 0/1 ownership boundary violation and UAF risk.

**Diff:**
```diff
- // ModeRegistry::prepareForOTA() as current:
- bool ModeRegistry::prepareForOTA() {
-     _switchState.store(SwitchState::SWITCHING);
-     if (_activeMode) { _activeMode->teardown(); delete _activeMode; _activeMode = nullptr; }
-     _otaMode = true;
-     return true;
- }
+ // Proposed: Enqueue request, Core 0 handles teardown
+ static std::atomic<bool> g_otaQuiesceRequested(false);
+ bool ModeRegistry::requestOTAQuiesce() { g_otaQuiesceRequested = true; return true; }
+ // Inside ModeRegistry::tick() on Core 0:
+ if (g_otaQuiesceRequested) { ... teardown here ... }
```

---

## Review Summary

**Issues Fixed:** 0
**Action Items Created:** 0

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
<var name="session_id">e935e4e7-02ff-4a2a-8caf-1f4042d9f9bd</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="38fadf94">embedded in prompt, file id: 38fadf94</var>
<var name="story_id">dl-7.2</var>
<var name="story_key">dl-7-2-ota-failure-handling-rollback-and-retry</var>
<var name="story_num">2</var>
<var name="story_title">2-ota-failure-handling-rollback-and-retry</var>
<var name="template">False</var>
<var name="timestamp">20260416_1828</var>
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