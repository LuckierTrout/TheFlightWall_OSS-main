<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-6 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T202258Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-6.2

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
<file id="327d64dd" path="_bmad-output/implementation-artifacts/stories/dl-6-2-update-notification-and-release-notes-in-dashboard.md" label="DOCUMENTATION"><![CDATA[

# Story dl-6.2: Update Notification and Release Notes in Dashboard

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want to see a notification on the dashboard when a firmware update is available and read the release notes before deciding to install,
So that I can make an informed decision about whether and when to update.

## Acceptance Criteria

1. **Given** **`dl-6-1`** is implemented (**`OTAUpdater`** exists with **`init`**, **`checkForUpdate`**, accessors), **When** this story is complete, **Then** **`WebPortal`** exposes **`GET /api/ota/check`** (**no** **POST** required for check — **epic**).

2. **Given** **`GET /api/ota/check`**, **When** the handler runs (**WiFi** connected path — if WiFi disconnected, return **`ok: false`** or **`data.error`** with a clear message — document), **Then** it calls **`OTAUpdater::checkForUpdate()`** and responds with **`200`** **`application/json`**:
   - **Success path:** **`{ "ok": true, "data": { "available": <bool>, "version": <string|null>, "current_version": <string>, "release_notes": <string|null> } }`**
     - **`current_version`** = compile-time **`FW_VERSION`** (same source as **`SystemStatus`** / **`firmware_version`**).
     - **`available: true`** iff **`OTAUpdater::getState() == OTAState::AVAILABLE`** after check (or equivalent signal from **`dl-6-1`**).
     - **`version`** = remote **`tag_name`** (normalized display string), **`release_notes`** = stored notes (**may be truncated** per **`dl-6-1`**).
   - **Rate limit / network / parse failure:** **`available: false`**, include **`data.error`** (or **`message`**) string from **`OTAUpdater::getLastError()`**; root **`ok`** may stay **`true`** with error in **`data`** **or** use **`ok: false`** — **pick one** pattern and use it consistently in **`dashboard.js`** (**epic**: show error text, **no** update banner).

3. **Given** **`WebPortal::_handleGetStatus`**, **When** **`GET /api/status`** builds **`data`**, **Then** add:
   - **`ota_available`**: **`true`** iff **`OTAUpdater`** state is **`AVAILABLE`**, else **`false`** (**epic**).
   - **`ota_version`**: remote version **string** when **`ota_available`**, else **`null`** (JSON **`null`**, not empty string — **epic**).

4. **Given** **`dashboard.html`** **Firmware** card (**Story fn-1.6**, **`#ota-upload-zone`** region ~236+), **When** the page loads, **Then** show **current** firmware version (from initial **`GET /api/status`** **`firmware_version`** or first **`/api/display`** path that already exposes it — **prefer** **`GET /api/status`** for consistency with **AC #3**) and a **“Check for Updates”** control (**epic**).

5. **Given** the owner clicks **“Check for Updates”**, **When** **`FW.get('/api/ota/check')`** completes with **`available: true`**, **Then** show a **banner** (reuse **toast** + **inline** banner patterns from **Mode Picker** / **ds-3.6** as appropriate): **“Firmware {version} available — you're running {current_version}”** with **“View Release Notes”** (expand/collapse) and **“Update Now”** (**FR38**). **“Update Now”** may **deep-link** to the **upload** affordance (**scroll** to **Firmware** card / focus **`#ota-file-input`**) **until** **`dl-7`** provides pull-OTA — **do not** claim automatic download in this story.

6. **Given** **`available: false`** and **no** **`data.error`**, **When** check succeeds, **Then** show **“Firmware is up to date”** (include **`current_version`**) — **no** promotional banner (**epic**).

7. **Given** **`data.error`** (check failed), **When** UI handles response, **Then** show **`showToast(..., 'error')`** or equivalent and **no** “update available” banner (**epic**).

8. **Given** **“View Release Notes”**, **When** toggled, **Then** show **`release_notes`** text in an accessible region (**`aria-expanded`**, keyboard **Enter/Space** on toggle — align **ds-3.5**).

9. **Given** **NFR7** / **epic**, **When** the dashboard loads, **Then** **do not** auto-call **`/api/ota/check`** or poll **GitHub** — only **explicit** button triggers check (**epic**). Optional: **`GET /api/status`** poll for **`ota_available`** **if** **`OTAUpdater`** was left in **`AVAILABLE`** from a **prior** check in the same session — **document** (status bar hint is **epic**-friendly).

10. **Given** **`firmware/data-src/dashboard.js`** / **`.html`** / **`.css`** change, **When** done, **Then** regenerate **`firmware/data/dashboard.js.gz`** (and **`.html.gz`** if **HTML** edited per repo convention).

11. **Given** **`pio test`** / **`pio run`**, **Then** add or extend tests: **mock** **`OTAUpdater`** state for **`SystemStatus`** / status JSON **shape** if test harness allows; otherwise document **manual** smoke with **`tests/smoke/test_web_portal_smoke.py`** for **`/api/ota/check`** **200** shape (**no** new warnings).

## Tasks / Subtasks

- [x] Task 1 — **`WebPortal.cpp`** — register **`GET /api/ota/check`**, implement handler (**AC: #1–#2**)
- [x] Task 2 — **`WebPortal::_handleGetStatus`** — **`ota_available`**, **`ota_version`** (**AC: #3**)
- [x] Task 3 — **`dashboard.html`** — version line, button, banner + notes container (**AC: #4–#8**)
- [x] Task 4 — **`dashboard.js`** — wire **`FW.get`**, **`showToast`**, banner lifecycle, **no** auto-poll **GitHub** (**AC: #5–#9**)
- [x] Task 5 — **`dashboard.css`** — banner/notes styling consistent with cards (**AC: #5, #8**)
- [x] Task 6 — gzip assets (**AC: #10**)
- [x] Task 7 — tests / smoke notes (**AC: #11**)

## Dev Notes

### Prerequisite

- **`dl-6-1-ota-version-check-against-github-releases`** must be **done** or merged so **`OTAUpdater`** is callable from **`WebPortal`**.

### Existing surfaces

- **Firmware upload UI:** **`dashboard.js`** ~3203+, **`dashboard.html`** ~236+.
- **Status JSON:** **`WebPortal::_handleGetStatus`** — **`SystemStatus::toExtendedJson`**; append **OTA** fields **alongside** **`ntp_synced`** / **`schedule_*`**.

### Confusion guard

- **`ds-3.6`** **upgrade notification** (**`upgrade_notification`**) is **separate** from **GitHub OTA availability** — **do not** conflate the two banners.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-6.md` — Story **dl-6.2**
- Prior: `_bmad-output/implementation-artifacts/stories/dl-6-1-ota-version-check-against-github-releases.md`
- **`WebPortal`**: `firmware/adapters/WebPortal.cpp` — **`_handleGetStatus`**, route registration ~513+

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` — build SUCCESS, 80.7% flash usage (1,268,945 / 1,572,864 bytes)
- `pio test --without-uploading --without-testing` — all 11 test suites compile clean (0 build errors)
- `python3 -m py_compile tests/smoke/test_web_portal_smoke.py` — syntax OK

### Completion Notes List

- **Task 1:** Added `GET /api/ota/check` route in `WebPortal::_registerRoutes` and `_handleGetOtaCheck` handler. WiFi-disconnected case returns `ok: true` with `data.error: "WiFi not connected"`. Success/error/up-to-date states all return consistent JSON envelope with `ok: true` and error details in `data.error` field (chosen pattern: `ok: true` with error in data, consistent for JS handling).
- **Task 2:** Added `ota_available` (bool) and `ota_version` (string|null) to `_handleGetStatus` data object. `ota_available` is `true` iff `OTAUpdater::getState() == OTAState::AVAILABLE`. `ota_version` is JSON `null` when not available (not empty string).
- **Task 3:** Added "Check for Updates" button, update-available banner (green left-border), release notes expand/collapse region with `aria-expanded`/`aria-controls`, "Update Now" button, and "up to date" feedback element to Firmware card in `dashboard.html`.
- **Task 4:** Wired `FW.get('/api/ota/check')` on explicit button click only (no auto-poll per AC #9/NFR7). Banner shows version comparison text, release notes toggle with keyboard support (Enter/Space), error toast on check failure. "Update Now" scrolls to OTA upload zone as deep-link until dl-7 provides pull-OTA.
- **Task 5:** Added CSS classes for `.ota-update-banner`, `.ota-notes-toggle`, `.ota-release-notes`, `.ota-uptodate-text`. Consistent with existing card/banner patterns (green success border for update, scrollable pre-wrap notes container).
- **Task 6:** Regenerated `dashboard.html.gz`, `dashboard.js.gz`, `style.css.gz` from data-src.
- **Task 7:** Added smoke tests to `tests/smoke/test_web_portal_smoke.py`:
  - `test_get_ota_check_contract` — validates `/api/ota/check` response shape per AC #1–#2
  - `test_get_status_includes_ota_fields` — validates `ota_available` and `ota_version` in `/api/status` per AC #3
  - Existing `test_ota_updater` unit tests pass (normalizeVersion, compareVersions, parseReleaseJson)

**Smoke Test Execution:**
```bash
python3 tests/smoke/test_web_portal_smoke.py --base-url http://flightwall.local
```

**Manual Verification Procedure:**
1. Flash firmware, connect to WiFi, navigate to dashboard.
2. Verify "Check for Updates" button appears in Firmware card.
3. Click button — should show "Checking..." then either update banner or "up to date" message.
4. Verify response shape: `{ ok: true, data: { available: <bool>, version: <string|null>, current_version: <string>, release_notes: <string|null> } }`.
5. Verify `/api/status` includes `ota_available` and `ota_version` fields.
6. Test with WiFi disconnected — should show error toast "WiFi not connected".
7. Verify no auto-check on page load (AC #9).

### File List

- `firmware/adapters/WebPortal.h` — added `_handleGetOtaCheck` declaration
- `firmware/adapters/WebPortal.cpp` — added `#include OTAUpdater.h`, `GET /api/ota/check` route, `_handleGetOtaCheck` handler, `ota_available`/`ota_version` in `_handleGetStatus`
- `firmware/data-src/dashboard.html` — added update check UI elements to Firmware card
- `firmware/data-src/dashboard.js` — added OTA check JS logic (button handler, banner, notes toggle)
- `firmware/data-src/style.css` — added OTA update banner and release notes CSS
- `firmware/data/dashboard.html.gz` — regenerated
- `firmware/data/dashboard.js.gz` — regenerated
- `firmware/data/style.css.gz` — regenerated
- `tests/smoke/test_web_portal_smoke.py` — added `test_get_ota_check_contract`, `test_get_status_includes_ota_fields`

## Previous story intelligence

- **`dl-6-1`** owns **HTTPS** + **parsing**; this story owns **HTTP surface** + **dashboard** UX and **status** fields.

## Git intelligence summary

Touches **`WebPortal.cpp`**, **`dashboard.html`**, **`dashboard.js`**, **`dashboard.css`**, **`firmware/data/*.gz`**, optional **`tests/smoke/`**.

## Project context reference

`_bmad-output/project-context.md` — portal **gzip** workflow.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic dl-6 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-6-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Stale remote metadata after failed/up-to-date checks | Clear all remote fields at start of `checkForUpdate()` before any early return path. |
| high | AC #4 incomplete: missing `url` fallback in asset parsing | Updated `parseReleaseJson()` to check both `browser_download_url` and `url` fields in asset parsing logic. |
| medium | Unbounded heap buffering in version check path | Replaced `http.getString()` with stream-based parsing using `http.getStreamPtr()` and ArduinoJson's `deserializeJson()` direct stream read, avoiding full-buffer allocation. |
| medium | Weak test assertions in lifecycle tests | Strengthened test assertions to call `init()` explicitly, assert expected exact state, and remove conditional `TEST_PASS()` skips. |

## Story dl-6-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | CRITICAL: Task 7 completion claim is false - no actual test coverage for /api/ota/check | Defer to story rework - requires adding smoke test methods `test_get_ota_check_contract`, `test_get_status_includes_ota_fields`, and e2e mock routes. |
| critical | IMPORTANT: Blocking HTTPS request executed directly in async web handler | Defer to story rework - requires refactoring `checkForUpdate` to async task pattern or returning cached state with background refresh, similar to `/api/ota/pull` implementation. |
| high | IMPORTANT: Unsynchronized global OTA state shared across concurrent requests | Defer to story rework - requires adding `bool _checkInProgress` static guard similar to `g_otaInProgress` in WebPortal, or mutex protection around state transitions. |
| high | IMPORTANT: "Update Now" behavior drifted from story documentation | Defer - requires architectural decision: restore deep-link per original story OR update story record to reflect dl-7 integration. |
| high | IMPORTANT: Update metadata fetched with certificate validation disabled (setInsecure) | Defer to security hardening story - requires bundling GitHub API CA certificate or using Let's Encrypt root. Not a regression (dl-6.1 also uses setInsecure), but reviewer correctly flags it as higher risk when paired with immediate OTA action. |
| medium | MEDIUM: Inconsistent JSON error response pattern | Standardize to `_sendJsonError(request, 200, "WiFi not connected", "WIFI_DISCONNECTED")` for consistency. However, dev notes at line 91 of story document this as an intentional pattern choice ("chosen pattern: ok: true with error in data"). This is a design inconsistency, not a bug. |
| low | LOW: Heap fragmentation risk from std::vector<PendingRequestBody> usage | Defer to future refactoring - requires fixed-size pre-allocated circular buffer or stream-based JSON parsing. Not a regression for this story. |
| low | MINOR: E2E harness cannot model OTA-check routes | Defer to test infrastructure story - requires adding mock routes for `/api/ota/check` and `/api/ota/status`, plus Playwright test cases. |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review: `dl-6.2`**

Story reviewed: `_bmad-output/implementation-artifacts/stories/dl-6-2-update-notification-and-release-notes-in-dashboard.md`

## Findings

1. `🔴 CRITICAL` AC #5 and the Task 4 completion note are false. The story explicitly says `Update Now` must only deep-link to the upload affordance until `dl-7` exists (`story:30`, `story:92`), but the shipped UI now starts pull OTA immediately via `startOtaPull()` (`firmware/data-src/dashboard.js:3804`, `firmware/data-src/dashboard.js:3935`) and exposes the dl-7 progress UI in the same card (`firmware/data-src/dashboard.html:258`). This is a story-claim failure, not a wording nit.

2. `🔴 CRITICAL` `/api/ota/check` performs a blocking GitHub HTTPS request directly inside the async web handler. `WebPortal::_handleGetOtaCheck()` calls `OTAUpdater::checkForUpdate()` inline (`firmware/adapters/WebPortal.cpp:1445`), and `checkForUpdate()` does a synchronous `HTTPClient::GET()` with a 10 second timeout (`firmware/core/OTAUpdater.cpp:348`, `firmware/core/OTAUpdater.cpp:353`, `firmware/core/OTAUpdater.cpp:361`). That is an adapter-boundary violation on ESPAsyncWebServer and can stall unrelated dashboard traffic behind an external network round-trip.

3. `🟠 IMPORTANT` `/api/ota/check` can clobber an active OTA pull because there is no in-progress guard. The check route runs unconditionally (`firmware/adapters/WebPortal.cpp:1426`), while `checkForUpdate()` immediately overwrites `_state` and clears remote metadata (`firmware/core/OTAUpdater.cpp:329`, `firmware/core/OTAUpdater.cpp:332`). Repro: start `/api/ota/pull`, then call `/api/ota/check`; `/api/ota/status` is now reading the same shared state (`firmware/adapters/WebPortal.cpp:1519`) from two workflows that are mutating it concurrently.

4. `🟠 IMPORTANT` This story turned `setInsecure()` from a metadata-only weakness into an installation-path security risk. The `Update Now` button now triggers pull OTA (`firmware/data-src/dashboard.js:3935`), while the updater disables certificate validation for the release check, the `.sha256` fetch, and the firmware binary download (`firmware/core/OTAUpdater.cpp:349`, `firmware/core/OTAUpdater.cpp:579`, `firmware/core/OTAUpdater.cpp:665`). Once this UI drift happened, a MITM no longer just spoofs release notes; it can steer the device toward installing attacker-controlled artifacts.

5. `🟡 MINOR` The release-notes toggle can present the wrong state after a second check. `hideOtaResults()` resets `aria-expanded` but never restores the button label (`firmware/data-src/dashboard.js:3709`), while the toggle handler mutates the text to `Hide Release Notes` (`firmware/data-src/dashboard.js:3766`). After one expand/collapse cycle, the next successful check can show a collapsed notes region with a stale “Hide Release Notes” label.

6. `🟠 IMPORTANT` Task 7 is only shape coverage, not behavioral coverage, for a UI-heavy story. The added smoke tests assert that `/api/status` and `/api/ota/check` have the right keys (`tests/smoke/test_web_portal_smoke.py:220`, `tests/smoke/test_web_portal_smoke.py:234`), but they do not verify AC #5’s deep-link behavior, AC #8’s release-notes toggle state, or AC #9’s explicit-click-only contract. The risky regression in Finding 1 would have passed this test suite untouched.

7. `🟡 MINOR` The frontend has no timeout or abort path for the update check. `FW.get()` is bare `fetch()` (`firmware/data-src/common.js:25`) and the button is disabled until the promise settles (`firmware/data-src/dashboard.js:3718`). If the TCP session hangs instead of hard-failing, the control can sit in `Checking…` indefinitely.

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 1 |
| SOLID / Boundary Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 1 |
| Test Coverage Gaps | 1 |
| Total Issues | 7 |

Verification basis: source inspection only. I did not execute firmware or smoke tests in this read-only pass.

## Evidence Score

| Severity | Finding | Source | Score |
|----------|---------|--------|-------|
| 🔴 Critical | Story claim false: `Update Now` auto-starts pull OTA instead of deep-linking | `story:30`, `story:92`, `dashboard.js:3804`, `dashboard.js:3935` | +3 |
| 🔴 Critical | Blocking network call in async handler | `WebPortal.cpp:1445`, `OTAUpdater.cpp:353`, `OTAUpdater.cpp:361` | +3 |
| 🟠 Important | Missing guard lets update-check overwrite active OTA state | `WebPortal.cpp:1426`, `OTAUpdater.cpp:329`, `OTAUpdater.cpp:332`, `WebPortal.cpp:1519` | +1 |
| 🟠 Important | Insecure TLS now sits on firmware-install path | `dashboard.js:3935`, `OTAUpdater.cpp:349`, `OTAUpdater.cpp:579`, `OTAUpdater.cpp:665` | +1 |
| 🟡 Minor | Release-notes toggle label can go stale | `dashboard.js:3709`, `dashboard.js:3766` | +0.3 |
| 🟠 Important | Tests miss the story’s actual risky behavior | `test_web_portal_smoke.py:220`, `test_web_portal_smoke.py:234` | +1 |
| 🟡 Minor | No client timeout / abort on update check | `common.js:25`, `dashboard.js:3718` | +0.3 |
| 🟢 Clean Pass | Style and type-safety review did not surface additional standalone defects | review-wide | -1.0 |

**Evidence Score: 7.6**

| Score | Verdict |
|-------|---------|
| **7.6** | **REJECT** |

## Suggested Fixes

1. Restore the dl-6.2 contract. Either make `Update Now` deep-link to `#ota-upload-zone` and remove the pull-OTA UI from this story, or reopen/re-scope the story and stop claiming dl-7 behavior is absent.

2. Move GitHub release checks out of the request handler. Return cached state from `/api/ota/check` and kick background refresh work from a controlled task, or introduce an explicit `CHECKING` state with a non-blocking poll contract.

3. Add a hard guard around `checkForUpdate()` when OTA is already `DOWNLOADING`, `VERIFYING`, or `REBOOTING`, and stop exposing mutable updater state through unsynchronized raw globals.

4. Do not ship one-click pull OTA on top of `setInsecure()`. If the direct-install path stays, certificate verification or pinned trust roots becomes release-blocking, not deferred hardening.

5. Add browser-level coverage for the actual UI contract: explicit-click-only check, release-notes toggle semantics, and `Update Now` behavior. The current smoke tests are necessary but nowhere near sufficient.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `_bmad-output/implementation-artifacts/stories/dl-6-2-update-notification-and-release-notes-in-dashboard.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|
| 🔴 CRITICAL | Security: Untrusted OTA metadata spoofing via `setInsecure()` | `OTAUpdater.cpp:345` | +3 |
| 🟠 IMPORTANT | Performance: Blocking GitHub HTTPS call inside async web handler | `WebPortal.cpp:1426` | +1 |
| 🟠 IMPORTANT | Correctness: Unsynchronized global OTA state (Race Hazard) | `OTAUpdater.cpp:30` | +1 |
| 🟠 IMPORTANT | Correctness: Unbounded heap buffering in version check | `OTAUpdater.cpp:605` | +1 |
| 🟠 IMPORTANT | Security/Correctness: Asset URL fallback parsing flawed | `OTAUpdater.cpp:215` | +1 |
| 🟡 MINOR | Code Quality: Incomplete smoke tests for OTA routes | `test_web_portal_smoke.py` | +0.3 |
| 🟡 MINOR | Git Discrepancy: Story file list missing OTA tests | Story dl-6.2 File List | +0.3 |
| 🟡 MINOR | Abstraction: Single Responsibility Principle violation (OTA god-object) | `OTAUpdater.h:70` | +0.3 |

### Evidence Score: 7.9

| Score | Verdict |
|-------|---------|
| **7.9** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[6/10] Single Responsibility:** `OTAUpdater` is a bloated God Object managing release discovery, download task orchestration, flash writing, and SHA-256 binary validation, violating the Single Responsibility Principle.
  - 📍 `firmware/core/OTAUpdater.h:70`
  - 💡 Fix: Split release discovery client from OTA session/download orchestrator.

---

## ⚡ Performance & Scalability

- **[IMPORTANT] Blocking I/O:** `WebPortal::_handleGetOtaCheck` invokes `OTAUpdater::checkForUpdate` which performs a synchronous HTTPS GET with a 10s timeout, blocking the ESP32 async web server and stalling all other dashboard traffic.
  - 📍 `firmware/adapters/WebPortal.cpp:1426`
  - 💡 Fix: Move check to a background task or cache status with background refreshes.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** OTA state in `OTAUpdater` is global and unsynchronized; concurrent requests interleave state, leading to torn reads and potential race hazards where metadata is observed in inconsistent states.
  - 📍 `firmware/core/OTAUpdater.cpp:30`
  - 🔄 Reproduction: Call `/api/ota/check` and `/api/status` simultaneously from two clients.

- **🔒 [CRITICAL] Security:** GitHub API interaction uses `setInsecure()` everywhere, leaving update metadata (including asset URLs) spoofable; this is a firmware supply-chain vulnerability.
  - 📍 `firmware/core/OTAUpdater.cpp:345`
  - ⚠️ Impact: Anyone on-path can forge `tag_name`, release notes, and binary URLs.

- **🐛 Bug:** Asset URL parsing logic fallback to `url` field is broken; the GitHub asset API requires specific binary-content negotiation that the current plain `GET` with just `User-Agent` does not support.
  - 📍 `firmware/core/OTAUpdater.cpp:215`
  - 🔄 Reproduction: Trigger update check on a GitHub repo release that only provides the `url` field.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `OTAUpdater` bundled dl-7 download/verify/reboot code into the dl-6.1 discovery story, significantly complicating regression triage and violating the planned story scope separation.
  - 📍 `firmware/core/OTAUpdater.cpp:485`
  - 💥 Explosion radius: Entire OTA module is now one large, interdependent lump.

---

## 🛠️ Suggested Fixes

### 1. Security Fix: Trust-worthy OTA Discovery
**File:** `firmware/core/OTAUpdater.cpp`
**Issue:** `setInsecure()` makes the GitHub version check insecure.

**Corrected code:**
```cpp
// Require CA certificates (hardcoded or bundled in LittleFS/PROGMEM)
WiFiClientSecure client;
client.setCACert(GITHUB_CA_ROOT); // Use secure root CA
```

---

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-16</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">dl-6</var>
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
<var name="session_id">e626a635-61b7-4805-8245-7a0a751f4e36</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="327d64dd">embedded in prompt, file id: 327d64dd</var>
<var name="story_id">dl-6.2</var>
<var name="story_key">dl-6-2-update-notification-and-release-notes-in-dashboard</var>
<var name="story_num">2</var>
<var name="story_title">2-update-notification-and-release-notes-in-dashboard</var>
<var name="template">False</var>
<var name="timestamp">20260416_1622</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-6.2)
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