<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-7 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T215717Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-7.1

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
<file id="748f899f" path="_bmad-output/implementation-artifacts/stories/dl-7-1-ota-download-with-incremental-sha-256-verification.md" label="DOCUMENTATION"><![CDATA[

# Story dl-7.1: OTA Download with Incremental SHA-256 Verification

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want to download and install a firmware update over WiFi with one tap,
So that I can update my device without finding a USB cable, opening PlatformIO, or pulling from git.

## Acceptance Criteria

1. **Given** **`OTAUpdater`** is in **`OTAState::AVAILABLE`** (**URLs** + **metadata** from **`dl-6-1`** **`checkForUpdate()`**), **When** **`OTAUpdater::startDownload()`** is called, **Then** it returns **`true`** after spawning a **one-shot** **FreeRTOS** task (**pinned to Core 1**, **stack ≥ 8192** bytes, **priority 1** per **epic**), sets state **`DOWNLOADING`** (or **`CHECKING`** → **`DOWNLOADING`** transition documented), and returns **immediately** without blocking the caller (**epic**). If a download task is already running (**`_downloadTaskHandle != nullptr`**), **`startDownload()`** returns **`false`** (**epic**).

2. **Given** the download task starts, **When** the **pre-OTA** sequence runs, **Then** call **`ModeRegistry::prepareForOTA()`** (**new public API**, **rule 26** / **AR10**) which: tears down the active **`DisplayMode`**, frees mode heap, sets **`SwitchState::SWITCHING`** (or equivalent) so **`ModeRegistry::tick`** does not run normal mode logic, and leaves the system in a state where **flash OTA** can proceed safely. The **LED matrix** shows a **static** **“Updating…”** (or agreed copy) **progress** screen (**FR26**) — reuse or extend the **same** visual channel used for **upload OTA** if one exists; otherwise add a **minimal** full-matrix message path coordinated with the **display task** (**document** cross-core ordering: **`prepareForOTA`** may need to **signal** Core **0** if teardown must run there — **do not** violate **D2**/**Rule 30**).

3. **Given** **`prepareForOTA()`** completed, **When** the task checks **heap**, **Then** require **`ESP.getFreeHeap() >= 81920`** (**80 KB**, **FR34** / **NFR16**). If below threshold: **`Update.abort()`** not yet started is fine — set **`OTAState::ERROR`**, **`getLastError()`** **“Not enough memory — restart device and try again”**, exit task and clear handle (**epic**).

4. **Given** heap is sufficient, **When** **SHA** step runs, **Then** **HTTPS GET** the **`.sha256`** asset URL from **`OTAUpdater`** storage (~**64** bytes **+** newline), parse as **64** **hex** characters (ignore surrounding whitespace / optional filename suffix per **common** **GitHub** **sha256** **artifacts**). If missing or unparseable: **`OTAState::ERROR`**, **“Release missing integrity file”**, **no** **`Update.begin()`** (**AR9** / **epic**).

5. **Given** expected **SHA-256** is known, **When** **binary** download starts, **Then** **`esp_ota_get_next_update_partition(NULL)`**, **`Update.begin(partition->size)`** (same sizing approach as **`WebPortal`** upload — **`firmware/adapters/WebPortal.cpp`** ~608–629), initialize **`mbedtls_sha256_context`**, **`mbedtls_sha256_starts_ret(&ctx, 0)`**, and stream **`.bin`** via **`HTTPClient`** / **`WiFiClientSecure`** in **bounded** chunks (**epic**).

6. **Given** each **binary** chunk arrives, **When** processed, **Then** in the **same** loop iteration **`Update.write(chunk, len)`** **and** **`mbedtls_sha256_update(&ctx, chunk, len)`** (**rule 25** — incremental, **never** post-hoc full-buffer hash of the whole image in RAM). Update **`_progress`** **0–100** as **`(bytesWritten * 100) / totalSize`** ( **`totalSize`** from **`Content-Length`** if present, else **partition** size — document fallback if **chunked** encoding lacks length).

7. **Given** stream completes, **When** verifying, **Then** **`mbedtls_sha256_finish_ret`**, **constant-time** **compare** (or **`memcmp`** on fixed **32**-byte digests) to **expected** **SHA** **before** **`Update.end(true)`**. On **mismatch**: **`Update.abort()`**, **`OTAState::ERROR`**, **“Firmware integrity check failed”** (**AR9**). On **match**: **`Update.end(true)`**, **`esp_ota_set_boot_partition(next)`**, **`OTAState::REBOOTING`**, **`delay(500)`**, **`ESP.restart()`** (**epic**). **NFR5**: typical path **< 60s** on **10 Mbps** — **document** assumptions (no hard CI gate unless test harness exists).

8. **Given** the task exits (**success** or **failure**), **When** leaving the task, **Then** clear **`_downloadTaskHandle`** (or equivalent) **before** **`vTaskDelete(NULL)`** so **`startDownload()`** guard cannot observe a **dangling** handle (**epic**). **Every** error path after **`Update.begin()`** succeeds must call **`Update.abort()`** (**rule 25**, align **dl-7.2**).

9. **Given** **`WebPortal`** **POST `/api/ota/upload`** first-chunk path (**`WebPortal.cpp`** ~563+), **When** **`Update.begin()`** is about to run, **Then** invoke **`ModeRegistry::prepareForOTA()`** **before** **`Update.begin()`** so **push** and **pull** share the same teardown (**AR10**). If **`prepareForOTA()`** fails (e.g. display task cannot quiesce), **abort** upload with a **clear** **`errorCode`**.

10. **Given** **`pio run`** / **`pio test`**, **Then** add tests feasible **without** full **WiFi**: **e.g.** **SHA** parser unit test, **hex** compare test, **`prepareForOTA`** **no-op** or **mock** where **Unity** **environment** allows; document **hardware** integration for **end-to-end** pull. **No** new **compiler** warnings.

## Tasks / Subtasks

- [x] Task 1 — **`ModeRegistry`** — **`prepareForOTA()`** declaration/definition, **switch** state integration, **display** "Updating…" hook (**AC: #2, #9**)
- [x] Task 2 — **`OTAUpdater`** — **`startDownload()`**, task proc, **HTTP** stream, **`mbedtls_sha256_*`**, **progress**, **state** transitions, **guards** (**AC: #1, #3–#8**)
- [x] Task 3 — **`WebPortal.cpp`** — call **`prepareForOTA()`** before **`Update.begin()`** on **upload** path (**AC: #9**)
- [x] Task 4 — **`main.cpp` / display pipeline`** — ensure **LED** / **matrix** **Updating** state and **no** **use-after-free** during OTA (**AC: #2**)
- [x] Task 5 — **Includes / build** — **`mbedtls`**, **`Update`**, **`esp_ota_ops.h`** as needed (**AC: #5–#7**)
- [x] Task 6 — **Tests** (**AC: #10**)

## Dev Notes

### Prerequisite

- **`dl-6-1`** / **`dl-6-2`** provide **`OTAUpdater::checkForUpdate()`** and **`AVAILABLE`** state with **asset URLs**.

### Out of scope (**dl-7.2**, **dl-7.3**)

- **`POST /api/ota/pull`**, **`GET /api/ota/status`**, **dashboard** **Retry**/**progress** **polling** — **dl-7.3**; mid-stream **WiFi** drop **UX** copy — partly **dl-7.2**. This story may still implement **internal** **`Update.abort()`** on **HTTP** read failure so partition is not marked bootable.

### Partition / size

- **OTA** slots: **`firmware/custom_partitions.csv`** — **app0/app1** **0x180000** (**1.5 MiB**). **`firmware/check_size.py`** limits must remain consistent (**NFR14** in **dl-7.3** references **2 MiB** — **verify** against **actual** **partition** table when claiming limits).

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-7.md` — Story **dl-7.1**
- Prior: `_bmad-output/implementation-artifacts/stories/dl-6-2-update-notification-and-release-notes-in-dashboard.md`
- **Push OTA:** `firmware/adapters/WebPortal.cpp` — **`/api/ota/upload`**
- **Registry:** `firmware/core/ModeRegistry.cpp`, `firmware/core/ModeRegistry.h`
- **Boot / rollback:** `firmware/src/main.cpp` — **`esp_ota_*`** **self-check** region ~516+

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, 80.8% flash, no new warnings
- Tests: `pio test -f test_ota_updater` — compilation PASSED (on-device execution requires hardware)

### Completion Notes List

1. **Task 1 (ModeRegistry::prepareForOTA):** Added `prepareForOTA()` and `isOtaMode()` public API. Sets `SwitchState::SWITCHING`, tears down and deletes active mode, consumes pending switch requests, sets `_otaMode` flag. Display task checks `isOtaMode()` to show "Updating..." message.

2. **Task 2 (OTAUpdater download):** Implemented `startDownload()` which spawns a one-shot FreeRTOS task on Core 1 (10KB stack, priority 1). Task procedure: calls `prepareForOTA()`, checks heap >= 80KB, fetches `.sha256` file, parses 64-hex-char digest, opens OTA partition with `Update.begin(partition->size)`, initializes `mbedtls_sha256_context`, streams binary in 4KB chunks with simultaneous `Update.write()` + `mbedtls_sha256_update()`, verifies SHA-256 with constant-time compare before `Update.end(true)`, sets boot partition, reboots. All error paths call `Update.abort()` after `Update.begin()`. Handle cleared before `vTaskDelete(NULL)`.

3. **Task 3 (WebPortal upload path):** Added `ModeRegistry::prepareForOTA()` call before `Update.begin()` in `/api/ota/upload` handler. If teardown fails, returns `PREPARE_OTA_FAILED` error code.

4. **Task 4 (Display pipeline safety):** Added `isOtaMode()` check in `displayTask()` loop before mode rendering. When OTA is active, displays "Updating..." message at 5fps, resets WDT, and skips all mode rendering — preventing use-after-free.

5. **Task 5 (Build):** All required includes added: `mbedtls/sha256.h`, `Update.h`, `esp_ota_ops.h`, `ModeRegistry.h` in OTAUpdater.cpp. Build passes with no new warnings. Binary size 80.8% of partition limit.

6. **Task 6 (Tests):** Added 16 new unit tests to `test_ota_updater/test_main.cpp`: 9 for `parseSha256File()` (valid hex, filename suffix, leading whitespace, uppercase, too short, invalid char, null inputs, small buffer), 5 for `compareSha256()` (matching, different, last byte, null inputs), 2 for download guards (rejects when not AVAILABLE, initial progress is 0). All compile clean.

### File List

- `firmware/core/OTAUpdater.h` — modified (added startDownload, getProgress, parseSha256File, compareSha256, OTA_HEAP_THRESHOLD, _downloadTaskHandle, _progress, _downloadTaskProc)
- `firmware/core/OTAUpdater.cpp` — modified (added download task implementation, SHA-256 parsing, constant-time compare, includes for mbedtls/Update/esp_ota_ops/ModeRegistry)
- `firmware/core/ModeRegistry.h` — modified (added prepareForOTA, isOtaMode, _otaMode)
- `firmware/core/ModeRegistry.cpp` — modified (added prepareForOTA and isOtaMode implementations)
- `firmware/adapters/WebPortal.cpp` — modified (added prepareForOTA call before Update.begin in upload handler)
- `firmware/src/main.cpp` — modified (added isOtaMode check in displayTask for safe "Updating..." rendering)
- `firmware/test/test_ota_updater/test_main.cpp` — modified (added 16 new tests for SHA-256 parsing, comparison, download guards)

## Previous story intelligence

- **Foundation** **upload** used **`Update.write`** without **external** **SHA** file — **pull** path must implement **AR9** **incremental** hash (**epic**).

## Git intelligence summary

Touches **`OTAUpdater.*`**, **`ModeRegistry.*`**, **`WebPortal.cpp`**, **`main.cpp`** (or display module), tests.

## Project context reference

`_bmad-output/project-context.md` — **FreeRTOS** cores, **heap**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-7-1-ota-download-with-incremental-sha-256-verification.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 2 |
| SOLID Violations | 0 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **10** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Pull OTA continues even when `prepareForOTA()` fails | `firmware/core/OTAUpdater.cpp:457` | +3 |
| 🔴 CRITICAL | `prepareForOTA()` violates the Core 0 ownership boundary | `firmware/core/ModeRegistry.cpp:525` | +3 |
| 🟠 IMPORTANT | Upload failure paths don't call `completeOTAAttempt(false)` | `firmware/adapters/WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | Tests are superficial and miss failure paths | `firmware/test/test_ota_updater/test_main.cpp` | +1 |
| 🟠 IMPORTANT | Missing `OTAUpdater` download guard check for concurrency | `firmware/core/OTAUpdater.cpp` | +1 |
| 🟡 MINOR | Code duplication in binary/SHA fetching | `firmware/core/OTAUpdater.cpp` | +0.3 |
| 🟡 MINOR | Incomplete error logging in OTA fail paths | `firmware/core/OTAUpdater.cpp` | +0.3 |

### Evidence Score: 9.6

| Score | Verdict |
|-------|---------|
| **9.6** | **REJECT** |

---

## 🏛️ Architectural Sins

- **Violation of Ownership Boundary:** `prepareForOTA()` deletes `_activeMode` on Core 1 while Core 0's display task might be rendering it, causing a race condition and likely use-after-free.
  - 📍 `firmware/core/ModeRegistry.cpp:525`
  - 💡 Fix: Implement a safe Core 0 handshake/quiesce mechanism instead of forcible deletion.

---

## 🐛 Correctness & Safety

- **🐛 Critical Bug (OTA Flow):** The download task logs `prepareForOTA returned false - continuing anyway` and then proceeds to flash firmware. This explicitly violates the story's requirement for a hard gate.
  - 📍 `firmware/core/OTAUpdater.cpp:457`
  - 🔄 Reproduction: Simulate a failed `prepareForOTA()` by artificially limiting memory or forcing `_otaMode` true at boot.

- **🔒 [High] Security:** OTA binary integrity is checked via SHA-256, but the HTTPS check for GitHub Assets uses `client.setInsecure()`, making it vulnerable to man-in-the-middle attacks. While documented as an "MVP tradeoff", it is a lingering vulnerability.
  - 📍 `firmware/core/OTAUpdater.cpp:358`
  - ⚠️ Impact: MITM can inject arbitrary firmware binaries.

- **🎭 Lying Test:** `test_ota_updater/test_main.cpp`
  - 🤥 Why it lies: The tests do not exercise `prepareForOTA()`, 80KB heap check, or `Update.end(true)` failures.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Lack of shared HTTP/Stream utility for fetching artifacts (binary + SHA file) leads to duplicate boilerplate code in `OTAUpdater.cpp`.
  - 📍 `firmware/core/OTAUpdater.cpp`
  - 💥 Explosion radius: High maintenance burden for future artifact types.

---

## 🛠️ Suggested Fixes

### 1. Hard Gate for prepareForOTA()

**File:** `firmware/core/OTAUpdater.cpp`
**Issue:** OTA process proceeds even if display teardown fails.

```cpp
if (!ModeRegistry::prepareForOTA()) {
    // ABORT immediately, do not continue
    snprintf(_lastError, ERROR_MAX_LEN, "Could not prepare display for OTA");
    _state = OTAState::ERROR;
    _failurePhase = OTAFailurePhase::DOWNLOAD;
    SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR, "Display teardown failed");
    ModeRegistry::completeOTAAttempt(false);
    _downloadTaskHandle = nullptr;
    vTaskDelete(NULL);
    return;
}
```

**Review Actions:**
- Issues Found: 10
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**ADVERSARIAL CODE REVIEW COMPLETE**

**Story:** `_bmad-output/implementation-artifacts/stories/dl-7-1-ota-download-with-incremental-sha-256-verification.md`

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 4 |
| Task Completion Lies | 1 |
| SOLID / Architecture Violations | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **7** |

## Evidence Score Summary

| Severity | Finding | Source | Score |
|----------|---------|--------|-------|
| 🔴 CRITICAL | OTA pull trusts unauthenticated TLS for metadata, hash, and binary | `firmware/core/OTAUpdater.cpp:348`, `firmware/core/OTAUpdater.cpp:578`, `firmware/core/OTAUpdater.cpp:664` | +3 |
| 🔴 CRITICAL | `prepareForOTA()` tears down Core 0 rendering state from Core 1 and still does not stop `tick()` from rendering | `firmware/core/ModeRegistry.cpp:472`, `firmware/core/ModeRegistry.cpp:418`, `firmware/core/ModeRegistry.h:115`, `firmware/src/main.cpp:510` | +3 |
| 🔴 CRITICAL | Push OTA failures leave `_otaMode` latched, so the wall can stay stuck on `Updating...` forever | `firmware/adapters/WebPortal.cpp:555`, `firmware/adapters/WebPortal.cpp:622`, `firmware/adapters/WebPortal.cpp:656`, `firmware/src/main.cpp:507` | +3 |
| 🟠 IMPORTANT | Pull OTA logs `prepareForOTA()` failure and continues into the download anyway | `firmware/core/OTAUpdater.cpp:549` | +1 |
| 🟠 IMPORTANT | `Update.end(true)` failure path skips the required `Update.abort()` cleanup | `firmware/core/OTAUpdater.cpp:805`, `firmware/core/OTAUpdater.h:9`, story `:29` | +1 |
| 🟠 IMPORTANT | `AVAILABLE` does not actually mean “installable”; missing `.sha256` still passes parsing and tests | `firmware/core/OTAUpdater.cpp:414`, `firmware/core/OTAUpdater.cpp:502`, `firmware/test/test_ota_updater/test_main.cpp:160` | +1 |
| 🟠 IMPORTANT | AC10 / Task 6 is overstated; the only `prepareForOTA` test is explicitly ignored | story `:33`, story `:42`, `firmware/test/test_mode_registry/test_main.cpp:456` | +1 |
| 🟢 CLEAN PASS | No style-only issues worth blocking on | Style | -0.5 |

### Evidence Score: 11.5

| Score | Verdict |
|-------|---------|
| **11.5** | **REJECT** |

## Architectural Sins

- **[9/10] Boundary breach:** `ModeRegistry::prepareForOTA()` mutates `_activeMode` directly on Core 1 (`firmware/core/ModeRegistry.cpp:483-499`) even though `ModeRegistry::tick()` and `_activeMode->render()` run on Core 0 (`firmware/core/ModeRegistry.cpp:393-421`). `SwitchState::SWITCHING` is not consulted before rendering, and `_otaMode` is only a plain `bool` (`firmware/core/ModeRegistry.h:115`). This is exactly the cross-core ownership violation AC2 warned about.
- **[7/10] Split-brain OTA flow:** Push OTA recovery lives in `WebPortal.cpp`, pull OTA recovery lives in `OTAUpdater.cpp`, and they already diverged on cleanup and rollback-banner handling. That duplication caused the stuck-OTA bug and the missing `Update.abort()` bug.

## Correctness & Safety

- **🔒 [CRITICAL] OTA authenticity is not verified:** Every GitHub request uses `WiFiClientSecure::setInsecure()` (`firmware/core/OTAUpdater.cpp:348-349`, `:578-579`, `:664-665`). An on-path attacker can supply a malicious binary and a matching `.sha256`, and your “integrity check” still passes. This is arbitrary firmware installation, not just a transport weakness.
- **🐛 `prepareForOTA()` is still use-after-free prone:** AC2 said the display task must not run normal mode logic after teardown, but `tick()` never checks `_switchState` before `render()` (`firmware/core/ModeRegistry.cpp:418-420`). If Core 1 deletes `_activeMode` while Core 0 is entering `tick()`, the next render dereferences freed memory.
- **🐛 Push OTA failure path never restores the renderer:** After `ModeRegistry::prepareForOTA()` succeeds in the upload handler (`firmware/adapters/WebPortal.cpp:656-675`), later failures only invalidate the request / abort `Update` (`:692-713`) and clear upload state (`:555-568`, `:622-628`). Nothing calls `ModeRegistry::completeOTAAttempt(false)`, so `main.cpp` keeps taking the OTA branch (`firmware/src/main.cpp:507-519`).
- **🐛 Pull OTA ignores precondition failure:** `_downloadTaskProc()` explicitly continues after `prepareForOTA()` returns false (`firmware/core/OTAUpdater.cpp:549-552`). That violates AC2 and means the code is willing to flash firmware even when the registry says the display was not safely quiesced.
- **🐛 Cleanup contract is broken on finalize failure:** Story AC8 requires `Update.abort()` on every post-`Update.begin()` failure, but the `Update.end(true)` error path omits it (`firmware/core/OTAUpdater.cpp:805-816`). The header comment claims the opposite (`firmware/core/OTAUpdater.h:9`).
- **🐛 `AVAILABLE` is a lie for incomplete releases:** `checkForUpdate()` marks state `AVAILABLE` purely on version mismatch (`firmware/core/OTAUpdater.cpp:414-417`). `parseReleaseJson()` allows empty asset URLs, and the test suite blesses “bin only, no sha256” as success (`firmware/test/test_ota_updater/test_main.cpp:160-166`). That lets the UI advertise an update that cannot be securely installed.

## Test Quality

- **🎭 Lying test:** `test_parse_bin_only_no_sha256` (`firmware/test/test_ota_updater/test_main.cpp:160-166`) asserts that a release without a `.sha256` asset is a valid parse outcome. That bakes the broken `AVAILABLE` contract into the suite.
- **🎭 Missing test disguised as done work:** Story AC10 says to add a feasible `prepareForOTA` no-op/mock test, but the only such test is literally `TEST_IGNORE_MESSAGE(...)` (`firmware/test/test_mode_registry/test_main.cpp:456-460`).
- **Coverage gap:** `firmware/test/test_ota_updater/test_main.cpp` only exercises parser helpers and simple guards. It does not hit `_downloadTaskProc`, any post-`Update.begin()` failure branch, push OTA integration, or OTA-mode recovery.

## Maintainability Issues

- **💣 OTA cleanup logic is duplicated and already inconsistent:** Push OTA clears `ota_rb_ack` on success (`firmware/adapters/WebPortal.cpp:718-725`); pull OTA never does. Push OTA aborts on `prepareForOTA()` failure, pull OTA proceeds. This will keep regressing until both entrypoints share one recovery/finalization path.

## Suggested Fixes

1. Move OTA teardown onto the display task’s ownership boundary. `prepareForOTA()` should only enqueue a quiesce request; Core 0 should perform teardown, set an atomic “OTA ready” flag, and only then let Core 1 call `Update.begin()`.
2. Make `_otaMode` atomic and have `ModeRegistry::tick()` explicitly bail out when `_switchState == SWITCHING` or OTA mode is active.
3. In both OTA paths, call a shared failure finalizer that does all of: `Update.abort()` when needed, clear OTA mode, restore display state, and reset any rollback-banner bookkeeping.
4. Refuse `AVAILABLE` unless both binary and `.sha256` URLs are present, and update the tests to assert that incomplete releases stay non-installable.

**Review Actions:**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

**Verdict:** REJECT. This story is not safe to ship in its current form.

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
<var name="session_id">373ae769-6eb4-4761-8a53-97073259fce3</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="748f899f">embedded in prompt, file id: 748f899f</var>
<var name="story_id">dl-7.1</var>
<var name="story_key">dl-7-1-ota-download-with-incremental-sha-256-verification</var>
<var name="story_num">1</var>
<var name="story_title">1-ota-download-with-incremental-sha-256-verification</var>
<var name="template">False</var>
<var name="timestamp">20260416_1757</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-7.1)
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