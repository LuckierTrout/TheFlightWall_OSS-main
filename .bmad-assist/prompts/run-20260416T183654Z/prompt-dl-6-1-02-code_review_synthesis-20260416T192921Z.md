<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-6 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T192921Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-6.1

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
<file id="40ee7354" path="_bmad-output/implementation-artifacts/stories/dl-6-1-ota-version-check-against-github-releases.md" label="DOCUMENTATION"><![CDATA[

# Story dl-6.1: OTA Version Check Against GitHub Releases

Status: done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want to check whether a newer firmware version is available on GitHub,
So that I know when updates exist without manually browsing the repository.

## Acceptance Criteria

1. **Given** **`OTAUpdater`** is new in this story, **When** implemented, **Then** add **`firmware/core/OTAUpdater.h`** and **`firmware/core/OTAUpdater.cpp`** with:
   - **`OTAState`** enum: **`IDLE`**, **`CHECKING`**, **`AVAILABLE`**, **`DOWNLOADING`**, **`VERIFYING`**, **`REBOOTING`**, **`ERROR`** (**epic** **AR3** — **`DOWNLOADING`/`VERIFYING`/`REBOOTING`** reserved for **dl-7**; may remain unused here but **must** exist in the enum).
   - **`static void init(const char* repoOwner, const char* repoName)`** — copies owner/repo into **bounded** static storage (document max lengths, **e.g.** **59** each per GitHub username/repo limits with safe margin), idempotent or assert single init; after init, state **`IDLE`**.
   - **`static bool checkForUpdate()`** — performs the GitHub **latest release** fetch and parse path below (**AC #2–#7**).
   - Accessors: **`getState()`**, **`getLastError()`** (null-terminated, human-readable), **`getRemoteVersion()`**, **`getReleaseNotes()`** (truncated buffer), **`getBinaryAssetUrl()`**, **`getSha256AssetUrl()`** (or empty if missing) — whatever **dl-6.2** needs for **`GET /api/ota/check`** plus **dl-7** download (**epic**).

2. **Given** **`checkForUpdate()`** runs while **WiFi** is connected, **When** it issues **HTTPS GET** **`https://api.github.com/repos/{owner}/{repo}/releases/latest`**, **Then** use **`WiFiClientSecure` + `HTTPClient`** consistent with **`FlightWallFetcher::httpGetJson`** (**`firmware/adapters/FlightWallFetcher.cpp`**) — **`setInsecure()`** acceptable for **this** story **if** the project does not yet bundle **`api.github.com`** roots (document tradeoff); **timeout** **≤ 10000 ms** (**NFR21** — no blocking **>10s** on Core paths used by **`checkForUpdate`**).

3. **Given** GitHub REST requirements, **When** building the request, **Then** set headers **`Accept: application/json`** and a valid **`User-Agent`** (GitHub rejects or rate-limits anonymous requests without **User-Agent** — use a product string **e.g.** **`FlightWall-OTA/1.0`** plus optional **`FW_VERSION`**).

4. **Given** HTTP **200** and a JSON body, **When** parsed with **ArduinoJson** (**v7** per **`platformio.ini`**), **Then** extract **`tag_name`**, **`body`** (release notes), and scan **`assets[]`** for the first **`.bin`** and first **`.sha256`** **asset** **`browser_download_url`** (or **`url`** if that is what the API returns — match **GitHub** **releases** **API** shape). Store **notes** truncated to **512** chars (**epic**).

5. **Given** **`FW_VERSION`** (**`-D FW_VERSION=...`** in **`platformio.ini`**, also surfaced in **`SystemStatus`** — **`firmware/core/SystemStatus.cpp`**), **When** comparing to **`tag_name`**, **Then** implement **documented** normalization (**strip** leading **`v`**, trim whitespace) and determine **“newer available”** vs **“up to date”**. If **`tag_name`** equals **current** after normalization, set state **`IDLE`**, return **`false`**. If different, set state **`AVAILABLE`**, persist parsed fields, return **`true`**. If **`tag_name`** is **missing** or parse fails, **`ERROR`** + message, return **`false`**.

6. **Given** HTTP **429**, **When** handled, **Then** state **`ERROR`**, **`_lastError`** **“Rate limited — try again later”** (**epic**), return **`false`**, **no** automatic retry loop (**NFR20**).

7. **Given** network failure (**HTTP code ≤ 0**, DNS, **timeout**, non-**200** **4xx/5xx** not otherwise special-cased), **When** handled, **Then** state **`ERROR`**, descriptive **`_lastError`**, return **`false`**; device behavior otherwise unchanged (**NFR21** tolerance).

8. **Given** **`main.cpp` `setup()`**, **When** system starts, **Then** call **`OTAUpdater::init(owner, name)`** with **compile-time** or **central config** constants for this OSS product’s GitHub **`owner`/`repo`** (prefer **`-D`** **`build_flags`** in **`platformio.ini`** so forks can override without editing **`.cpp`** — document exact macro names).

9. **Given** **`pio test`** / **`pio run`**, **Then** add **native** tests where feasible: **e.g.** a **testable** **`compareVersion(normalizedTag, fwVersion)`** or **JSON parse helper** fed **fixture** strings; **avoid** live **GitHub** calls in **CI**. **`pio run`** must stay **green** with **no** new warnings.

## Tasks / Subtasks

- [x] Task 1 — **`OTAUpdater.h/.cpp`** — state machine, storage, **`init`**, **`checkForUpdate`**, accessors (**AC: #1–#7**)
- [x] Task 2 — **`main.cpp`** — **`OTAUpdater::init`** in **`setup()`** (**AC: #8**)
- [x] Task 3 — **`platformio.ini`** — **`GITHUB_REPO_OWNER`** / **`GITHUB_REPO_NAME`** (or agreed macro names) defaults for **`TheFlightWall_OSS-main`** upstream (**AC: #8**)
- [x] Task 4 — **`firmware/test/`** — unit tests for parsing / compare / **429** branch if injectable (**AC: #9**)
- [x] Task 5 — **`CMakeLists.txt` / test** registration if new test folder (**AC: #9**)

### Review Follow-ups (AI)

- [x] [AI-Review] Critical: Clear all remote metadata fields at start of `checkForUpdate()` before any early return path (stale data fix)
- [x] [AI-Review] High: Add `url` fallback in asset parsing per AC #4 (was missing)
- [x] [AI-Review] Medium: Replace `http.getString()` with stream-based parsing via `http.getStreamPtr()` and `parseReleaseJsonStream()` (bounded heap)
- [x] [AI-Review] Medium: Strengthen test assertions — call `_resetForTest()` explicitly, assert exact expected state, remove conditional `TEST_PASS()` skips

## Dev Notes

### Scope boundary (**dl-6.2**)

- **No** **`GET /api/ota/check`**, **no** **`GET /api/status`** **`ota_*`** fields, **no** dashboard UI — those are **`dl-6.2`**. This story delivers **only** the **`OTAUpdater`** **core** + **`setup()`** **init**.

### HTTPS stack

- Reuse patterns from **`FlightWallFetcher`** / **`AeroAPIFetcher`**; keep **heap** use bounded — prefer **`http.getString()`** into a **`String`** with **reasonable** cap or **stream** parse if payload size is a concern (**releases/latest** JSON is typically small).

### Version semantics

- Document whether **pre-release** tags are ignored (**`releases/latest`** is **latest stable** per GitHub — good default).

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-6.md` — Story **dl-6.1**
- Prior: `_bmad-output/implementation-artifacts/stories/dl-5-1-per-mode-settings-panels-in-mode-picker.md` (sprint order)
- Existing OTA upload: `firmware/adapters/WebPortal.cpp` — **`/api/ota/upload`** (separate concern)
- **`FW_VERSION`**: `firmware/platformio.ini`, `firmware/core/SystemStatus.cpp`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- `pio run` build: SUCCESS (0 new warnings from OTAUpdater, 80.7% flash)
- `pio test -f test_ota_updater --without-uploading --without-testing`: BUILD PASSED
- Tests are on-device (ESP32); compilation verified but execution requires physical device

### Completion Notes List

- **Task 1**: Created `OTAUpdater.h` and `OTAUpdater.cpp` with full OTAState enum (7 states including dl-7 reserved), static `init()` with bounded 60-char buffers, `checkForUpdate()` using WiFiClientSecure+HTTPClient consistent with FlightWallFetcher pattern, and 6 accessors. Public testable helpers: `normalizeVersion()`, `compareVersions()`, `parseReleaseJson()`, `parseReleaseJsonStream()`. ArduinoJson v7 filter used to reduce heap. `setInsecure()` used with documented tradeoff (no bundled CA roots). 10s timeout per NFR21. User-Agent header `FlightWall-OTA/1.0 FW/{version}`. HTTP 429 returns "Rate limited" error, no retry loop per NFR20. Network failures set descriptive `_lastError`.
- **Task 2**: Added `#include "core/OTAUpdater.h"` and `OTAUpdater::init(GITHUB_REPO_OWNER, GITHUB_REPO_NAME)` call in `setup()` after `SystemStatus::init()`. Macro fallbacks defined in main.cpp for IDE/testing.
- **Task 3**: Added `-D GITHUB_REPO_OWNER` and `-D GITHUB_REPO_NAME` build flags in `platformio.ini`. Forks override by editing these flags without touching `.cpp`.
- **Task 4**: Created `test_ota_updater/test_main.cpp` with 27 tests covering: normalizeVersion (10 tests: v-stripping, whitespace trim, edge cases), compareVersions (5 tests: same/different/null/prerelease), parseReleaseJson (10 tests: valid release, no assets, bin-only, missing tag, null/empty/invalid JSON, notes extraction, url fallback, stale value clearing), state lifecycle (2 tests). No live GitHub calls — all fixture-based.
- **Task 5**: No registration needed — PlatformIO auto-discovers `test_*` directories under `firmware/test/`.
- ✅ Resolved review finding [Critical]: Stale remote metadata after failed/up-to-date checks — now cleared at start of `checkForUpdate()`.
- ✅ Resolved review finding [High]: AC #4 `url` fallback — now checks both `browser_download_url` and `url` fields in asset parsing.
- ✅ Resolved review finding [Medium]: Unbounded heap buffering — replaced `http.getString()` with stream-based `parseReleaseJsonStream()`.
- ✅ Resolved review finding [Medium]: Weak test assertions — added `_resetForTest()`, removed conditional `TEST_PASS()` skips, assertions now deterministic.

### Implementation Plan

- Version semantics: `releases/latest` endpoint returns latest stable release only (pre-releases excluded by GitHub API). Version comparison is string equality after normalization — any difference triggers "update available".
- HTTPS tradeoff: `setInsecure()` used because ESP32 Arduino does not bundle api.github.com CA roots. Documented for future hardening.
- Release notes truncated to 512 chars per epic spec.
- Asset scanning: first `.bin` and first `.sha256` by filename suffix from `assets[]` array.

### File List

- `firmware/core/OTAUpdater.h` (new)
- `firmware/core/OTAUpdater.cpp` (new)
- `firmware/src/main.cpp` (modified — include, macros, init call)
- `firmware/platformio.ini` (modified — 2 new build flags)
- `firmware/test/test_ota_updater/test_main.cpp` (new)

### Change Log

- 2026-04-14: Story dl-6.1 implemented — OTAUpdater core module with GitHub releases version check, main.cpp integration, build flags, and 25 unit tests.
- 2026-04-16: Addressed code review findings — 4 items resolved (1 critical, 1 high, 2 medium). Added `parseReleaseJsonStream()` for bounded heap, `url` fallback per AC #4, stale metadata clearing, `_resetForTest()` for deterministic tests. Test count now 27.

## Previous story intelligence

- **Delight** stories use **`WebPortal`** + **`dashboard.js`**; this story is **device-side HTTP client** only — align with **`HTTPClient`** usage elsewhere.

## Git intelligence summary

New **`core/OTAUpdater.*`**, **`main.cpp`**, **`platformio.ini`**, tests.

## Project context reference

`_bmad-output/project-context.md` — **WiFi** / **HTTPS** constraints.

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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-6-1-ota-version-check-against-github-releases.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **7** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | GitHub release discovery is unauthenticated because every GitHub request uses `setInsecure()`, so a MITM can spoof release metadata and future OTA asset URLs. | `firmware/core/OTAUpdater.cpp:345`, `:579`, `:665` | +3 |
| 🟠 IMPORTANT | The new `url` fallback is not usable end-to-end; download code never negotiates asset binary content for GitHub asset API URLs. | `firmware/core/OTAUpdater.cpp:215`, `:290`, `:667` | +1 |
| 🟠 IMPORTANT | Rate-limit handling is incomplete; only `429` is treated as rate-limited even though GitHub documents `403` or `429`. | `firmware/core/OTAUpdater.cpp:363`, `:381` | +1 |
| 🟠 IMPORTANT | `OTAUpdater` shared state is read and written from different tasks with no synchronization, so `/api/ota/*` can observe torn or stale values. | `firmware/core/OTAUpdater.cpp:31`, `:77`, `:767`; `firmware/adapters/WebPortal.cpp:1452`, `:1519` | +1 |
| 🟠 IMPORTANT | `compareVersions()` is not a version comparison; it is raw inequality, so older remote tags and dev builds are misreported as “update available.” | `firmware/core/OTAUpdater.cpp:160`; `firmware/test/test_ota_updater/test_main.cpp:74` | +1 |
| 🟠 IMPORTANT | The test suite never exercises `checkForUpdate()` or its HTTP/state transitions; it mostly tests helpers and later-story wiring, so AC #2-#7 remain unverified. | `firmware/test/test_ota_updater/test_main.cpp:516` | +1 |
| 🟠 IMPORTANT | This story is no longer scoped to dl-6.1: `OTAUpdater` already owns dl-7 download/verify/reboot behavior and `main.cpp` carries unrelated runtime changes, so the story record is not trustworthy. | `firmware/core/OTAUpdater.h:70`; `firmware/core/OTAUpdater.cpp:485`; `firmware/src/main.cpp:325`, `:507`, `:1072` | +1 |
| 🟢 CLEAN PASS | Style and type-safety are not the blocking problems here. | — | -1 |

### Evidence Score: 8.0

| Score | Verdict |
|-------|---------|
| **8.0** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[8/10] Single Responsibility:** `OTAUpdater` is supposed to deliver dl-6.1 release discovery, but it already bundles dl-7 download orchestration, SHA-256 verification, reboot control, display teardown, and status reporting. `firmware/core/OTAUpdater.h:70`, `firmware/core/OTAUpdater.cpp:485`. Fix: split release-check metadata from OTA session/download logic and keep dl-6.1 scoped to the check path plus init.

- **Boundary breach:** `main.cpp` changes are not limited to `OTAUpdater::init(...)`; the current delta also alters display-task behavior, status-message timing, and scheduler/orchestrator flow. `firmware/src/main.cpp:325`, `:373`, `:507`, `:1072`, `:1146`. That makes the story’s file list and completion notes materially incomplete.

---

## 🐍 Pythonic Crimes & Readability

✅ No blocking style/readability issue. The defects are behavioral and architectural.

---

## ⚡ Performance & Scalability

✅ No standalone perf-only finding outranks the correctness/security problems above.

---

## 🐛 Correctness & Safety

- **🔒 [critical] Security:** Release discovery trusts any certificate. `firmware/core/OTAUpdater.cpp:345`, `:579`, `:665`. Impact: anyone on-path can forge `tag_name`, release notes, and asset URLs; with the later dl-7 code already present, this becomes a firmware supply-chain problem, not just a cosmetic version-check issue.

- **🐛 Bug:** The `url` fallback added for asset parsing is incomplete. `parseReleaseJson*()` stores the asset API `url` field (`firmware/core/OTAUpdater.cpp:215`, `:290`), but the downloader later performs a plain GET with only `User-Agent` (`:667-674`). GitHub’s asset API requires binary-content negotiation or using `browser_download_url`, so this “fallback” still breaks when exercised.

- **🐛 Bug:** Rate-limit detection is wrong in practice. `checkForUpdate()` special-cases `429` only (`firmware/core/OTAUpdater.cpp:363-369`) and collapses `403` into a generic HTTP failure (`:381-387`). GitHub’s REST docs say primary and secondary limits can surface as `403` or `429`, so users lose the promised “try again later” handling.

- **🐛 Bug:** OTA state is not thread-safe. `_state`, `_progress`, `_failurePhase`, `_lastError`, `_remoteVersion`, and the asset URL buffers are plain statics (`firmware/core/OTAUpdater.cpp:31-42`) exposed through raw getters (`:77-96`), while async HTTP handlers read them concurrently (`firmware/adapters/WebPortal.cpp:1452-1463`, `:1519-1540`) and the download task mutates them (`firmware/core/OTAUpdater.cpp:766-824`). That is a classic torn-read/stale-state hazard.

- **🐛 Bug:** `compareVersions()` is logically wrong for “newer available.” `firmware/core/OTAUpdater.cpp:160-162` returns true on any difference, and the tests lock that behavior in (`firmware/test/test_ota_updater/test_main.cpp:74-92`). Reproduction: flash a locally newer dev build, then query an older stable release; the code still reports “update available.”

- **🎭 Lying Test:** The suite claims AC coverage but never tests `checkForUpdate()` itself. `firmware/test/test_ota_updater/test_main.cpp:516-592`. It validates helper routines and later-story enums, but there is no proof that HTTP 200/403/429/timeout paths actually drive `OTAState`, `_lastError`, and metadata correctly.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** The story record says dl-6.1 is a core-only version-check story, but the implementation now spans multiple future stories and unrelated `main.cpp` behavior. That makes story-level regression triage unreliable and increases blast radius for every future OTA change.

---

## 🛠️ Suggested Fixes

1. Split `OTAUpdater` into a release-check client and a later OTA-download/session component. dl-6.1 should expose checked metadata only; dl-7 should own download tasks, ModeRegistry coordination, and failure-phase state.

2. Replace `setInsecure()` with a real trust model before shipping OTA discovery. At minimum, pin or bundle GitHub roots for `api.github.com`; if you keep the asset-API `url` fallback, add `Accept: application/octet-stream` and handle `200/302` correctly.

3. Add an injectable transport seam around `checkForUpdate()` and write tests for the public behavior: 200 + newer, 200 + same version, missing `tag_name`, `403/429` rate limit, timeout/negative HTTP code, and stale-state reset after failures.

---

## Sources

- GitHub release asset download behavior: https://docs.github.com/en/rest/releases/assets?apiVersion=2026-03-10
- GitHub REST rate-limit behavior (`403` or `429`): https://docs.github.com/en/rest/using-the-rest-api/rate-limits-for-the-rest-api?apiVersion=latest

---

**Review Actions:**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-6-1-ota-version-check-against-github-releases.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **4** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | AC #4: `.sha256` asset parsing uses `String::getString()` (unbounded heap) | OTAUpdater.cpp | +1.0 |
| 🟠 IMPORTANT | SOLID: `OTAUpdater` manages download task state (Story dl-7.1 concern) | OTAUpdater.h/.cpp | +1.0 |
| 🔴 CRITICAL | Hidden Bug: `_downloadTaskHandle` race condition | OTAUpdater.cpp | +3.0 |
| 🟠 IMPORTANT | Performance: `http.getString()` for SHA file | OTAUpdater.cpp | +1.0 |
| 🟢 CLEAN PASS | 5 | Categories: Style, Type Safety, Security, Git, Tests | -2.5 |

### Evidence Score: 3.5

| Score | Verdict |
|-------|---------|
| **3.5** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[IMPORTANT/10] SOLID Principle - Single Responsibility:** `OTAUpdater` currently manages version check, but also owns download task state management, SHA-256 binary verification logic, and FreeRTOS task orchestration. This violates the Single Responsibility Principle, as this component is becoming a "God Object" for OTA functionality across multiple stories.
  - 📍 `firmware/core/OTAUpdater.h:70`
  - 💡 Fix: Separate the HTTP version-check client from the OTA execution/verification engine.

---

## ⚡ Performance & Scalability

- **[IMPORTANT] Performance Footgun - Unbounded heap usage:** The `.sha256` file parsing path uses `http.getString()` into a `String` object before processing. While the SHA-256 file is small, the pattern encourages unbounded heap allocation for larger assets and contradicts the architectural rule for bounded heap usage in OTA components.
  - 📍 `firmware/core/OTAUpdater.cpp:605`
  - 💡 Fix: Replace `http.getString()` with stream-based parsing of the SHA-256 file, similar to how the main release JSON is handled.

---

## 🐛 Correctness & Safety

- **🐛 Hidden Bug - Race Condition:** `_downloadTaskHandle` is set to `nullptr` inside the download task *after* completion. If `startDownload()` is called between the time the task completes and the time `_downloadTaskHandle` is actually cleared (or if the task self-deletes and the handle is stale), it could lead to unexpected behavior.
  - 📍 `firmware/core/OTAUpdater.cpp:637`
  - 🔄 Reproduction: Call `startDownload()` repeatedly during task exit window.

---

## 🛠️ Suggested Fixes

### 1. Fix .sha256 parsing performance

**File:** `firmware/core/OTAUpdater.cpp`
**Issue:** `http.getString()` is used for small files; switch to stream reading.

**Diff:**
```diff
-        String shaBody = shaHttp.getString();
-        shaHttp.end();
-
-        if (!parseSha256File(shaBody.c_str(), shaBody.length(), expectedSha, sizeof(expectedSha))) {
+        Stream& shaStream = shaHttp.getStream();
+        // Use a small buffer to parse sha256 hex string incrementally if needed
+        // or simply limit the read to the expected size.
```

---

**Review Actions:**
- Issues Found: 4
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding.

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
<var name="session_id">e5fde0e2-8578-4b65-b7a3-4ef4aaa8aa0e</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="40ee7354">embedded in prompt, file id: 40ee7354</var>
<var name="story_id">dl-6.1</var>
<var name="story_key">dl-6-1-ota-version-check-against-github-releases</var>
<var name="story_num">1</var>
<var name="story_title">1-ota-version-check-against-github-releases</var>
<var name="template">False</var>
<var name="timestamp">20260416_1529</var>
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
      - Commit message format: fix(component): brief description (synthesis-dl-6.1)
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