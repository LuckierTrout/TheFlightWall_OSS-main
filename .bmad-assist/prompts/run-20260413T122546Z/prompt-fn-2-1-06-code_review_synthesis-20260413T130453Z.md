<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-2 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T130453Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-2.1

You are synthesizing 3 independent code review findings.

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
<file id="2c306e06" path="_bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md" label="DOCUMENTATION"><![CDATA[

# Story fn-2.1: NTP Time Sync & Timezone Configuration

Status: Done

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As the **device owner**,
I want the ESP32 to synchronize its clock via NTP using my configured timezone,
so that time-dependent features (night mode scheduling, future mode scheduling) have accurate local time without manual intervention.

## Acceptance Criteria

1. **WiFi Connection Triggers NTP Sync**
   - Given: device connects to WiFi in STA mode
   - When: WiFiManager reports `STA_CONNECTED`
   - Then: `configTzTime()` called with POSIX timezone string from `ConfigManager::getSchedule().timezone` and NTP servers `"pool.ntp.org"`, `"time.nist.gov"`
   - And: NTP synchronization completes within 10 seconds of WiFi connection *(integration test / device observation; unit tests cover callback logic only)*

2. **NTP Success Sets Status & Flag**
   - Given: NTP synchronization succeeds
   - When: `sntp_set_time_sync_notification_cb` callback fires with `SNTP_SYNC_STATUS_COMPLETED`
   - Then: `std::atomic<bool> ntpSynced` flag set to `true`
   - And: `SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Clock synced")` recorded
   - And: `getLocalTime(&now, 0)` returns correct local time for configured timezone

3. **NTP Failure Handled Gracefully**
   - Given: NTP unreachable after WiFi connects
   - When: sync attempt fails
   - Then: `ntpSynced` remains `false`
   - And: `SystemStatus::set(Subsystem::NTP, StatusLevel::WARNING, "Clock not set")` recorded
   - And: device does NOT crash or degrade other functionality
   - And: NTP re-sync automatically retried by LWIP SNTP (~1 hour interval)

4. **Timezone Hot-Reload**
   - Given: timezone config key changed via `POST /api/settings`
   - When: ConfigManager hot-reload callback fires
   - Then: `configTzTime()` called immediately with new POSIX timezone string
   - And: no reboot required

5. **TZ_MAP in Dashboard**
   - Given: `dashboard.js` includes a `TZ_MAP` object
   - When: timezone mapping loaded
   - Then: contains ~50-80 common IANA-to-POSIX entries (e.g., `"America/Los_Angeles": "PST8PDT,M3.2.0,M11.1.0"`)
   - And: `getTimezoneConfig()` returns `{ iana, posix }` using `Intl.DateTimeFormat().resolvedOptions().timeZone`

6. **API Status Endpoints**
   - Given: `GET /api/status` called
   - When: response built
   - Then: includes `"ntp_synced": true/false` and `"schedule_active": true/false`

## Tasks / Subtasks

- [x] **Task 1: Add global `ntpSynced` atomic flag and SNTP callback in main.cpp** (AC: #2, #3)
  - [x] 1.1 Add `#include "esp_sntp.h"` at top of main.cpp
  - [x] 1.2 Add `static std::atomic<bool> g_ntpSynced(false);` alongside other global atomics (~line 62)
  - [x] 1.3 Add public accessor `bool isNtpSynced() { return g_ntpSynced.load(); }` (needed by WebPortal)
  - [x] 1.4 Implement SNTP notification callback function that sets `g_ntpSynced = true` and calls `SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Clock synced")`
  - [x] 1.5 Register callback via `sntp_set_time_sync_notification_cb()` in `setup()` BEFORE WiFiManager starts
  - [x] 1.6 Set initial NTP status: `SystemStatus::set(Subsystem::NTP, StatusLevel::WARNING, "Clock not set")`

- [x] **Task 2: Replace hardcoded UTC NTP call in WiFiManager with timezone-aware call** (AC: #1)
  - [x] 2.1 In `WiFiManager::_onConnected()` (WiFiManager.cpp line ~132-134), replace `configTime(0, 0, "pool.ntp.org")` with `configTzTime(tz, "pool.ntp.org", "time.nist.gov")`
  - [x] 2.2 Read timezone from `ConfigManager::getSchedule().timezone.c_str()`
  - [x] 2.3 Log the actual POSIX timezone string being used (via Serial.println due to LOG_I macro limitation)

- [x] **Task 3: Add timezone hot-reload in ConfigManager onChange callback** (AC: #4)
  - [x] 3.1 In the existing `ConfigManager::onChange` lambda in main.cpp (~line 624-627), add a call to `configTzTime()` with the updated timezone when schedule config changes
  - [x] 3.2 Ensure callback does NOT reset `g_ntpSynced` flag (only the SNTP callback controls that)
  - [x] 3.3 Log timezone change (via Serial.println due to LOG_I macro limitation)

- [x] **Task 4: Extend WebPortal /api/status response with NTP fields** (AC: #6)
  - [x] 4.1 In `WebPortal::_handleGetStatus()` (WebPortal.cpp ~line 780), add `data["ntp_synced"] = isNtpSynced();`
  - [x] 4.2 Add `data["schedule_active"]` = `ConfigManager::getSchedule().sched_enabled == 1 && isNtpSynced()`
  - [x] 4.3 At the top of `firmware/adapters/WebPortal.cpp` (before the class implementation), add `extern bool isNtpSynced();` — forward-declares the function defined in `main.cpp` without requiring a new header file (aligns with project rule: no new files)

- [x] **Task 5: Add TZ_MAP and getTimezoneConfig() to dashboard.js** (AC: #5)
  - [x] 5.1 Add `TZ_MAP` object (~75 IANA-to-POSIX mappings) near top of `dashboard.js`
  - [x] 5.2 Implement `getTimezoneConfig()` that returns `{ iana, posix }` using browser `Intl.DateTimeFormat().resolvedOptions().timeZone`
  - [x] 5.3 Regenerate gzipped asset: `gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz`

- [x] **Task 6: Add TZ_MAP to wizard.js for settings import/export compatibility** (AC: #5)
  - [x] 6.1 Add same `TZ_MAP` object to wizard.js (with getTimezoneConfig() helper)
  - [x] 6.2 Regenerate gzipped asset for modified JS file(s)

- [x] **Task 7: Unit tests** (AC: #2, #3, #6)
  - [x] 7.1 Test SystemStatus NTP subsystem state transitions (OK/WARNING)
  - [x] 7.2 Test `getSchedule().timezone` returns correct default ("UTC0")
  - [x] 7.3 Test NTP status in JSON output (test_ntp_status_in_json_output) + timezone hot-reload key verification

## Dev Notes

### Architecture Constraints (MUST FOLLOW)

- **Core pinning:** NTP callback fires on the LWIP/WiFi task (Core 1). The `g_ntpSynced` atomic flag is safe for cross-core reads. Display task on Core 0 may read this flag in future stories (fn-2.2).
- **Enforcement Rule #30:** Cross-core atomics live in `main.cpp` only. Do NOT put `g_ntpSynced` in WiFiManager or WebPortal — keep it in main.cpp with a public accessor function.
- **Enforcement Rule #1-11:** Naming conventions: `g_ntpSynced` (global atomic), `isNtpSynced()` (accessor), `Subsystem::NTP` (enum).
- **Hot-reload pattern:** Timezone is a hot-reload key (NOT in `REBOOT_KEYS[]` at ConfigManager.cpp ~line 205). The `onChange` callback fires after `applyJson()` processes it.
- **No blocking calls:** `getLocalTime(&now, 0)` with timeout=0 per Enforcement Rule #14. Never block on NTP sync.
- **Logging:** Use `LOG_I`/`LOG_E`/`LOG_V` macros from `utils/Log.h`. Never use raw `Serial.println` in production paths.

### ESP32 Arduino SNTP API Reference

The ESP32 Arduino core wraps ESP-IDF SNTP. Key functions:

```cpp
#include "esp_sntp.h"  // ESP-IDF header, available in Arduino framework

// Configure NTP with POSIX timezone string
// Signature: void configTzTime(const char* tz, const char* server1, const char* server2 = nullptr, const char* server3 = nullptr)
configTzTime("PST8PDT,M3.2.0,M11.1.0", "pool.ntp.org", "time.nist.gov");

// Register sync notification callback (call BEFORE configTzTime)
// Callback fires on Core 1 (LWIP task context)
sntp_set_time_sync_notification_cb([](struct timeval* tv) {
    // tv contains the synced time
    // Safe to set atomic flag here
});

// Check time validity
struct tm timeinfo;
if (getLocalTime(&timeinfo, 0)) {  // timeout=0, non-blocking
    // Time is valid and in local timezone
}

// LWIP auto-resync interval: default 1 hour (CONFIG_LWIP_SNTP_UPDATE_DELAY)
// No manual retry logic needed
```

**Important:** `configTzTime()` internally calls `setenv("TZ", tz, 1)` + `tzset()` + starts SNTP. Calling it again with a new timezone stops/restarts SNTP — this is safe for hot-reload.

### Source Files to Modify

| File | Change | Lines |
|------|--------|-------|
| `firmware/src/main.cpp` | Add `g_ntpSynced` atomic, SNTP callback, register callback in setup(), extend onChange lambda | ~62, ~624, ~670+ |
| `firmware/adapters/WiFiManager.cpp` | Replace `configTime(0,0,"pool.ntp.org")` with `configTzTime(tz, "pool.ntp.org", "time.nist.gov")` | 132-134 |
| `firmware/adapters/WebPortal.cpp` | Add `ntp_synced` and `schedule_active` to /api/status response | ~780 |
| `firmware/data-src/dashboard.js` | Add `TZ_MAP` object + `getTimezoneConfig()` function | Top of file (new) |
| `firmware/data-src/wizard.js` | Add `TZ_MAP` if needed for settings import timezone resolution | Top of file (new) |
| `firmware/data/dashboard.js.gz` | Regenerate after JS change | N/A |
| `firmware/test/test_config_manager/test_main.cpp` | Add NTP status + API field tests | End of file |

### Existing Code Patterns to Follow

**Global atomic pattern (main.cpp ~line 62):**
```cpp
static std::atomic<bool> g_configChanged(false);
// Add alongside:
static std::atomic<bool> g_ntpSynced(false);
```

**ConfigManager onChange pattern (main.cpp ~line 624-627):**
```cpp
ConfigManager::onChange([]() {
    g_configChanged.store(true);
    g_layout = LayoutEngine::compute(ConfigManager::getHardware());
    // ADD: timezone hot-reload
});
```

**SystemStatus usage pattern (already used for OTA):**
```cpp
SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Clock synced");
SystemStatus::set(Subsystem::NTP, StatusLevel::WARNING, "Clock not set");
```

**WebPortal status response pattern (WebPortal.cpp ~line 780):**
```cpp
SystemStatus::toExtendedJson(data, stats);
// ADD after this line:
data["ntp_synced"] = isNtpSynced();
data["schedule_active"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();
```

### Existing Infrastructure Already in Place (from fn-1.2)

- `ScheduleConfig` struct with `timezone` field (default `"UTC0"`) -- ConfigManager.h line 44
- `Subsystem::NTP` enum value -- SystemStatus.h line 15
- NVS key `timezone` (string, max 40 chars) with validation -- ConfigManager.cpp line ~160
- `getSchedule()` getter with mutex guard -- ConfigManager.cpp
- `onChange()` callback mechanism with copy-under-lock pattern -- ConfigManager.cpp line 635
- All 5 schedule keys are hot-reload (not in `REBOOT_KEYS[]`)

### TZ_MAP Content Guide

Include ~60 entries covering major world timezones. Priority entries:

```javascript
const TZ_MAP = {
  // North America
  "America/New_York":       "EST5EDT,M3.2.0,M11.1.0",
  "America/Chicago":        "CST6CDT,M3.2.0,M11.1.0",
  "America/Denver":         "MST7MDT,M3.2.0,M11.1.0",
  "America/Los_Angeles":    "PST8PDT,M3.2.0,M11.1.0",
  "America/Phoenix":        "MST7",
  "America/Anchorage":      "AKST9AKDT,M3.2.0,M11.1.0",
  "Pacific/Honolulu":       "HST10",
  "America/Toronto":        "EST5EDT,M3.2.0,M11.1.0",
  "America/Vancouver":      "PST8PDT,M3.2.0,M11.1.0",
  // Europe
  "Europe/London":          "GMT0BST,M3.5.0/1,M10.5.0",
  "Europe/Paris":           "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Berlin":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Madrid":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Rome":            "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Amsterdam":       "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Stockholm":       "CET-1CEST,M3.2.0/2,M11.1.0/3",
  "Europe/Helsinki":        "EET-2EEST,M3.5.0/3,M10.5.0/4",
  "Europe/Athens":          "EET-2EEST,M3.5.0/3,M10.5.0/4",
  "Europe/Moscow":          "MSK-3",
  // Asia
  "Asia/Tokyo":             "JST-9",
  "Asia/Shanghai":          "CST-8",
  "Asia/Hong_Kong":         "HKT-8",
  "Asia/Singapore":         "SGT-8",
  "Asia/Kolkata":           "IST-5:30",
  "Asia/Dubai":             "GST-4",
  "Asia/Seoul":             "KST-9",
  "Asia/Bangkok":           "ICT-7",
  "Asia/Jakarta":           "WIB-7",
  // Oceania
  "Australia/Sydney":       "AEST-10AEDT,M10.1.0,M4.1.0/3",
  "Australia/Melbourne":    "AEST-10AEDT,M10.1.0,M4.1.0/3",
  "Australia/Perth":        "AWST-8",
  "Australia/Brisbane":     "AEST-10",
  "Australia/Adelaide":     "ACST-9:30ACDT,M10.1.0,M4.1.0/3",
  "Pacific/Auckland":       "NZST-12NZDT,M9.5.0,M4.1.0/3",
  // South America
  "America/Sao_Paulo":      "BRT3",
  "America/Argentina/Buenos_Aires": "ART3",
  "America/Santiago":       "CLT4CLST,M9.1.6/24,M4.1.6/24",
  "America/Bogota":         "COT5",
  "America/Lima":           "PET5",
  // Africa / Middle East
  "Africa/Cairo":           "EET-2EEST,M4.5.5/0,M10.5.4/24",
  "Africa/Johannesburg":    "SAST-2",
  "Africa/Lagos":           "WAT-1",
  "Africa/Nairobi":         "EAT-3",
  "Asia/Jerusalem":         "IST-2IDT,M3.4.4/26,M10.5.0",
  // UTC
  "UTC":                    "UTC0",
  "Etc/UTC":                "UTC0",
  "Etc/GMT":                "GMT0"
};
```

**Note:** POSIX timezone sign convention is **inverted** from UTC offset (west of UTC = positive, east = negative). E.g., UTC+9 (Tokyo) = `JST-9`.

### What This Story Does NOT Include

- Night Mode scheduler logic (fn-2.2)
- Dashboard Night Mode UI card (fn-2.3)
- Timezone dropdown UI in dashboard (fn-2.3)
- Wizard timezone auto-detect integration (fn-3.2)
- TZ_MAP is added to JS but NO UI elements use it yet -- fn-2.3 will add the dropdown

### Dependencies

| Dependency | Status | Notes |
|------------|--------|-------|
| fn-1.2 (ConfigManager expansion) | DONE | ScheduleConfig struct, NVS keys, SystemStatus::NTP |
| WiFiManager (epic-1) | DONE | _onConnected() hook exists |
| WebPortal /api/status | DONE | Handler exists, needs extension |
| SystemStatus | DONE | NTP subsystem enum exists |

### Gzip Regeneration Reminder

After modifying any file in `firmware/data-src/`, regenerate the corresponding `.gz`:

```bash
cd firmware
gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz
# If wizard.js modified:
gzip -9 -c data-src/wizard.js > data/wizard.js.gz
```

### Project Structure Notes

- All firmware C++ code under `firmware/` with hexagonal layout: `core/`, `adapters/`, `interfaces/`, `models/`
- NTP atomic flag goes in `firmware/src/main.cpp` (Rule #30: cross-core atomics in main.cpp only)
- WiFiManager modification in `firmware/adapters/WiFiManager.cpp`
- WebPortal modification in `firmware/adapters/WebPortal.cpp`
- Web assets: source in `firmware/data-src/`, gzipped in `firmware/data/`
- Tests in `firmware/test/test_config_manager/test_main.cpp`
- No new files created -- all modifications to existing files

### References

- [Source: _bmad-output/planning-artifacts/epics/epic-fn-2.md] -- Epic definition, all ACs
- [Source: _bmad-output/planning-artifacts/architecture.md#F4] -- IANA-to-POSIX browser-side mapping
- [Source: _bmad-output/planning-artifacts/architecture.md#F5] -- Night Mode scheduler, NTP via configTzTime()
- [Source: _bmad-output/planning-artifacts/architecture.md#F6] -- ConfigManager expansion, ScheduleConfig struct
- [Source: _bmad-output/planning-artifacts/prd.md#FR40] -- Auto-maintain API auth
- [Source: _bmad-output/planning-artifacts/prd.md#NFR-Reliability] -- 30+ day continuous operation
- [Source: firmware/core/ConfigManager.h:44-53] -- ScheduleConfig struct definition
- [Source: firmware/core/SystemStatus.h:15] -- Subsystem::NTP enum
- [Source: firmware/adapters/WiFiManager.cpp:132-134] -- Current hardcoded NTP call to replace
- [Source: firmware/src/main.cpp:62] -- Existing atomic flag pattern
- [Source: firmware/src/main.cpp:624-627] -- Existing ConfigManager::onChange callback
- [Source: firmware/adapters/WebPortal.cpp:774-791] -- /api/status handler

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6 (claude-sonnet-4-20250514)

### Debug Log References

- Build: `pio run` — SUCCESS, 77.8% flash usage (1,223,245 / 1,572,864 bytes)
- LOG_I macro is 2-arg only (string literal concatenation); used `Serial.println()` with `#if LOG_LEVEL >= 2` guard for dynamic timezone log messages (consistent with existing pattern in WiFiManager.cpp)

### Completion Notes List

**Implementation Summary (2026-04-13):**

All 7 tasks and all subtasks completed. All 6 acceptance criteria satisfied:

1. **AC #1 (WiFi → NTP):** `configTzTime()` called in `WiFiManager::_onConnected()` with POSIX TZ from `ConfigManager::getSchedule().timezone` and servers `pool.ntp.org`, `time.nist.gov`.
2. **AC #2 (NTP success):** `onNtpSync` callback registered via `sntp_set_time_sync_notification_cb()`, sets `g_ntpSynced = true` and `SystemStatus::set(NTP, OK, "Clock synced")`.
3. **AC #3 (NTP failure):** `g_ntpSynced` defaults to `false`, initial status set to WARNING. LWIP auto-retries (~1hr). No crash path — callback is fire-and-forget.
4. **AC #4 (Timezone hot-reload):** `configTzTime()` called in `ConfigManager::onChange` lambda only when timezone value changes (guarded by `s_lastAppliedTz` cache). Does NOT reset `g_ntpSynced`.
5. **AC #5 (TZ_MAP):** ~75 IANA-to-POSIX entries added to both `dashboard.js` and `wizard.js`. `getTimezoneConfig()` returns `{ iana, posix }` using `Intl.DateTimeFormat().resolvedOptions().timeZone`.
6. **AC #6 (API status):** `ntp_synced` and `schedule_active` fields added to `GET /api/status` response.

**Code Review Synthesis fixes applied (2026-04-13):**
- `onNtpSync()`: Added `sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED` guard — callback can fire on reset/intermediate SNTP events; now only sets `g_ntpSynced=true` on COMPLETED status.
- `onChange` lambda: Guarded `configTzTime()` with `static String s_lastAppliedTz` cache — prevents SNTP from restarting on unrelated config writes (brightness, fetch_interval, etc.).

**Code Review Synthesis fixes applied (2026-04-13) — Round 2 (3 validators):**
- `main.cpp` onChange: Moved `s_lastAppliedTz` OUTSIDE the lambda and seeded it with `ConfigManager::getSchedule().timezone` before `onChange` registration. Previously it was a static inside the lambda initialised to `""`, causing `configTzTime()` to fire on the very first unrelated config write (e.g., brightness). Lambda now captures by reference `[&s_lastAppliedTz]`.
- `ConfigManager.cpp` timezone validation: Added `if (tz.length() == 0) return false;` guard — empty string was previously accepted and would reach `configTzTime("")` on WiFi connect / hot-reload.
- `test_config_manager/test_main.cpp`: Added empty-timezone rejection test case inside `test_apply_json_schedule_validation`.

**Tests added:** 4 new Unity tests in `test_config_manager/test_main.cpp`:
- `test_ntp_status_transitions` — WARNING→OK state transitions
- `test_schedule_timezone_default` — default "UTC0" verification
- `test_timezone_is_hot_reload_key` — all 5 schedule keys are not reboot-required
- `test_ntp_status_in_json_output` — NTP subsystem in JSON with both states

### Change Log

- 2026-04-13: Implemented NTP time sync and timezone configuration (fn-2.1)

### File List

- `firmware/src/main.cpp` (modified — added esp_sntp.h include, g_ntpSynced atomic, isNtpSynced() accessor, onNtpSync callback, SNTP callback registration in setup(), timezone hot-reload in onChange)
- `firmware/adapters/WiFiManager.cpp` (modified — replaced `configTime(0,0,"pool.ntp.org")` with `configTzTime()` using ConfigManager timezone)
- `firmware/adapters/WebPortal.cpp` (modified — added extern isNtpSynced(), ntp_synced and schedule_active fields in /api/status)
- `firmware/data-src/dashboard.js` (modified — added TZ_MAP object with ~75 entries and getTimezoneConfig() function)
- `firmware/data-src/wizard.js` (modified — added TZ_MAP object with ~75 entries and getTimezoneConfig() function)
- `firmware/data/dashboard.js.gz` (regenerated)
- `firmware/data/wizard.js.gz` (regenerated)
- `firmware/test/test_config_manager/test_main.cpp` (modified — added 4 NTP tests; empty-timezone rejection test added in Round 2)

## Senior Developer Review (AI)

### Review: 2026-04-13
- **Reviewer:** AI Code Review Synthesis (2 validators)
- **Evidence Score:** Validator A: 2.7 (APPROVED) / Validator B: 4.6 (MAJOR REWORK) → Synthesized: APPROVED WITH RESERVATIONS
- **Issues Found:** 7 verified (2 fixed, 5 deferred/dismissed)
- **Issues Fixed:** 2
- **Action Items Created:** 3

#### Review Follow-ups (AI)
- [ ] [AI-Review] MEDIUM: Deduplicate TZ_MAP — move shared map to `common.js` so dashboard.js and wizard.js share a single source of truth (`firmware/data-src/dashboard.js`, `firmware/data-src/wizard.js`) — defer to fn-2.3 when timezone dropdown UI is built
- [ ] [AI-Review] LOW: `schedule_active` field semantics — rename to `schedule_enabled` or compute real in-window state when Night Mode scheduler (fn-2.2) is implemented (`firmware/adapters/WebPortal.cpp:787`)
- [ ] [AI-Review] LOW: `getTimezoneConfig()` silent UTC fallback — return `{ iana, posix: null }` for unmapped zones so fn-2.3 UI can surface an "unsupported timezone" warning instead of silently using UTC (`firmware/data-src/dashboard.js:101`, `firmware/data-src/wizard.js`)

### Review: 2026-04-13 (Round 2 — 3 validators)
- **Reviewer:** AI Code Review Synthesis (3 validators)
- **Evidence Score:** Validator A: (no score — output truncated) / Validator B: 5.1 (MAJOR REWORK) / Validator C: 4.5 (MAJOR REWORK) → Synthesized: APPROVED WITH RESERVATIONS
- **Issues Found:** 2 new verified bugs fixed; 6 re-raised items confirmed dismissed/deferred
- **Issues Fixed:** 2
- **Action Items Created:** 0 new (existing 3 follow-ups remain)

#### Review Follow-ups (AI) — Round 2
*(No new action items — all remaining issues already tracked above or confirmed as accepted design risks.)*


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic fn-2 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `onNtpSync()` sets `g_ntpSynced=true` unconditionally — but `sntp_set_time_sync_notification_cb` can fire on non-COMPLETED events (e.g., `SNTP_SYNC_STATUS_RESET` during network changes). Without a status check, the flag could flip to `true` spuriously. | Added `sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED` early-return guard ✅ Applied |
| high | `configTzTime()` called on **every** config write in the `onChange` lambda — including unrelated writes (brightness, fetch_interval, text colors, etc.). This restarts SNTP and discards in-flight NTP requests on hot paths. | Added `static String s_lastAppliedTz` cache inside the lambda; `configTzTime()` only fires when the timezone value actually changes ✅ Applied |
| medium | `TZ_MAP` with ~75 entries is copy-pasted verbatim into both `dashboard.js` and `wizard.js`. Any DST rule correction requires dual edits and dual `gzip` regenerations. | Deferred to fn-2.3 (timezone dropdown UI) — moving to `common.js` must be validated against wizard page's script loading order; project "no new files" rule for this story blocks immediate action. Follow-up task created. |
| low | `schedule_active` in `/api/status` is computed as `sched_enabled && ntp_synced`, not "currently inside the dim window". The field name implies real-time window state but delivers config+sync state. | Deferred to fn-2.2 (Night Mode scheduler) where the actual in-window calculation will be implemented. Follow-up task created. |
| low | `getTimezoneConfig()` silently falls back to `"UTC0"` for IANA zones not in `TZ_MAP` (e.g., `Asia/Manila`). The caller gets no signal that the zone was unmapped. | Deferred to fn-2.3 — no UI element currently consumes `getTimezoneConfig()`. Follow-up task created. |
| dismissed | `extern bool isNtpSynced()` in `WebPortal.cpp` breaks hexagonal architecture | FALSE POSITIVE: FALSE POSITIVE. The file already declares `extern FlightStatsSnapshot getFlightStatsSnapshot()` at line 37 using the identical pattern. The story's Dev Notes explicitly prescribe this approach ("aligns with project rule: no new files"). This is the established project convention, not a new violation. |
| dismissed | Race condition between `tzset()` (from `configTzTime()`) and `localtime_r()` across Core 0/Core 1 | FALSE POSITIVE: Theoretical but low-impact. Hot-reload is a rare user-triggered action; the `tzset()` modification window is microseconds; the worst-case consequence is a single garbled time display frame during a settings write. Accepted design risk per story scope. A mutex solution would require significant infrastructure not justified by this failure mode. |
| dismissed | Missing POSIX TZ format validation in ConfigManager | FALSE POSITIVE: Partially valid concern, but implemented validation (non-empty string ≤40 chars) is sufficient for the story AC. A malformed TZ string passed to `configTzTime()` → `tzset()` causes silent UTC fallback — the device stays functional and the LWIP stack doesn't crash. Full POSIX TZ syntax validation is non-trivial on ESP32 newlib and is outside story scope. Accepted risk. |
| dismissed | `g_ntpSynced` never resets to `false` after initial sync (stale success on WiFi reconnect) | FALSE POSITIVE: Out-of-story-scope design concern. The story ACs define `ntpSynced=false` only as the pre-sync default (AC #3). LWIP auto-retries maintain clock accuracy. The flag is correctly "sticky" for the current story's contract. If cross-story NTP state invalidation is needed (e.g., for fn-2.2 scheduling), that story should define the semantics. |
| dismissed | Raw `Serial.println` violates logging contract (should use `LOG_I`) | FALSE POSITIVE: Acknowledged in the Dev Agent Record as a known technical constraint. `LOG_I` is a 2-argument macro (tag + string literal only); it cannot format dynamic runtime strings. The `#if LOG_LEVEL >= 2` guard follows the pre-existing pattern already in `WiFiManager.cpp:140`. Not a new violation. |
| dismissed | Unit tests are "lying" — `test_ntp_status_in_json_output` doesn't test the actual `/api/status` HTTP endpoint | FALSE POSITIVE: Valid quality observation but not a "lying test". The test name and inline comment accurately describe what it tests (`SystemStatus::toJson()` NTP fields). Full HTTP endpoint testing is not achievable in Unity/embedded unit tests without a host simulator. The story explicitly notes "unit tests cover callback logic only" (AC #1). Coverage gap is real but architectural, not deceptive. --- |

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| dismissed | `g_ntpSynced` never resets to `false` after WiFi reconnect where NTP fails (Validators B, C) | FALSE POSITIVE: Already adjudicated as out-of-scope in the Round 1 antipatterns doc. AC #3 defines `ntpSynced=false` only as the pre-first-sync default. The "sticky true" design is intentional for this story contract; WiFi-reconnect NTP invalidation is a concern for a future story defining that semantic. |
| dismissed | AC #2 is "only half-implemented" — `getLocalTime()` not called in callback (Validator B) | FALSE POSITIVE: The AC states `getLocalTime()` *returns correct time* — this is a functional postcondition, not a requirement for the callback to call it. `configTzTime()` + SNTP completion makes `getLocalTime()` correct by design. The callback's job is flag + status, which is implemented. |
| dismissed | `test_ntp_status_in_json_output` is a lying test for AC #6 (Validators B, C) | FALSE POSITIVE: Already adjudicated in Round 1 antipatterns doc. Full HTTP endpoint testing is not achievable in Unity/embedded unit tests without a host simulator. The test name accurately describes what it tests (`SystemStatus::toJson()` NTP fields); the story AC #1 explicitly scopes: "unit tests cover callback logic only." |
| dismissed | `schedule_active` field is semantically misleading (Validators B, C) | FALSE POSITIVE: Already tracked as a deferred follow-up from Round 1. Will be corrected in fn-2.2 (Night Mode scheduler) when real in-window logic lands. |
| dismissed | `extern bool isNtpSynced()` in `WebPortal.cpp` breaks hexagonal architecture (Validator C) | FALSE POSITIVE: Already dismissed as FALSE POSITIVE in Round 1 antipatterns doc. `WebPortal.cpp:37` already uses the identical `extern FlightStatsSnapshot getFlightStatsSnapshot()` pattern — this is the established project convention, not a new violation. |
| dismissed | `TZ_MAP` duplication and silent UTC fallback in `getTimezoneConfig()` (Validators B, C) | FALSE POSITIVE: Already tracked as deferred follow-ups from Round 1. Deferred to fn-2.3 when the timezone dropdown UI is built and `common.js` sharing can be validated against wizard page script-load order. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 0 |
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
| 🟠 IMPORTANT | TZ_MAP duplicated across dashboard.js and wizard.js | dashboard.js / wizard.js | +1.0 |
| 🟠 IMPORTANT | schedule_active field semantics mislead API consumers | WebPortal.cpp | +1.0 |
| 🟠 IMPORTANT | getTimezoneConfig() silently falls back to UTC0 for unmapped zones | dashboard.js / wizard.js | +1.0 |
| 🟠 IMPORTANT | test_ntp_status_in_json_output tests wrong method (toJson vs toExtendedJson) | test_main.cpp | +1.0 |
| 🟠 IMPORTANT | Cross-core race: configTzTime()->setenv()/tzset() on Core 1 unsafe against future localtime_r() on Core 0 | main.cpp / WiFiManager.cpp | +1.0 |
| 🟠 IMPORTANT | configTzTime() thrashing risk on rapid alternating timezone changes | main.cpp | +1.0 |
| 🟢 CLEAN PASS | 4 categories with no issues (SOLID, Style, Type Safety, Security) | — | -2.0 |

### Evidence Score: 4.0

| Score | Verdict |
|-------|---------|
| **4.0** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

✅ No significant SOLID violations detected.

**Abstraction Issues:**
- **Missing Shared Abstraction:** `TZ_MAP` (~75 entries) is copy-pasted verbatim into both `dashboard.js` and `wizard.js`. Any DST rule correction requires dual edits and dual gzip regenerations.
  - 📍 `firmware/data-src/dashboard.js:1`
  - 📍 `firmware/data-src/wizard.js:1`

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

- **[Medium] SNTP Thrashing:** The `ConfigManager::onChange` lambda in `main.cpp` restarts SNTP via `configTzTime()` every time the timezone value changes. If a client rapidly alternates between two timezones (e.g., via API script), SNTP is continuously reset and may never reach `COMPLETED` status.
  - 📍 `firmware/src/main.cpp:670-678`
  - 💡 Fix: Add a short debounce window (e.g., 5s) or a "pending" flag in the lambda so `configTzTime()` is not invoked more than once within an SNTP convergence window.

---

## 🐛 Correctness & Safety

- **🐛 Bug: Cross-core race condition:** `configTzTime()` internally calls `setenv("TZ", ...)` and `tzset()`, which mutate global process state in newlib. These functions are **not thread-safe**. The callback fires on Core 1 (WiFi/web context), but the Display System / Delight architecture places ClockMode and scheduling on Core 0, which will call `localtime_r()` — creating a classic read-modify-write race across cores that can corrupt timezone tables or crash in `localtime_r()`.
  - 📍 `firmware/src/main.cpp:675`
  - 📍 `firmware/adapters/WiFiManager.cpp:132`
  - 🔄 Reproduction: Trigger a timezone hot-reload via dashboard while a Core 0 display mode (e.g., future ClockMode) calls `localtime_r()` in its render loop.

- **🐛 Bug: Silent UTC fallback:** `getTimezoneConfig()` returns `"UTC0"` for any IANA zone not present in `TZ_MAP` (e.g., `Asia/Manila`, `America/Monterrey`). Callers receive no signal that the mapping failed, so users will silently see UTC time for their actual zone.
  - 📍 `firmware/data-src/dashboard.js:97`
  - 📍 `firmware/data-src/wizard.js:97`

- **🎭 Lying Test:** `test_ntp_status_in_json_output` claims to test NTP status in the JSON output for `/api/status` (AC #6), but it calls `SystemStatus::toJson()`. The actual API handler (`WebPortal::_handleGetStatus`) calls `SystemStatus::toExtendedJson()`. If those methods diverge, this test gives false confidence.
  - 📍 `firmware/test/test_config_manager/test_main.cpp:390`
  - 🤥 Why it lies: It tests a different public method than the one used in production, and it never exercises the HTTP endpoint serialization path.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Misleading API field name:** `schedule_active` in `/api/status` is computed as `sched_enabled && ntp_synced`. The field name implies "the device is currently inside the scheduled dim window," but it actually means "scheduling is enabled and NTP is available." This will confuse dashboard developers when fn-2.2 adds the real in-window calculation.
  - 📍 `firmware/adapters/WebPortal.cpp:787`
  - 💥 Explosion radius: Will require a breaking API change or silent semantic shift in fn-2.2, breaking any external consumers.

- **💣 Tech Debt: Missing file in change log:** `firmware/core/ConfigManager.cpp` was modified in Round 2 to add `if (tz.length() == 0) return false;` (empty-timezone validation), yet it does not appear in the story's File List. Incomplete documentation erodes auditability.
  - 📍 `firmware/core/ConfigManager.cpp` (implied by antipatterns doc)
  - 💥 Explosion radius: Future reviewers will not know this file was touched for NTP validation.

---

## 🛠️ Suggested Fixes

### 1. Deduplicate TZ_MAP

**File:** `firmware/data-src/dashboard.js`, `firmware/data-src/wizard.js`
**Issue:** ~75-entry timezone table is duplicated, creating a maintenance hazard.

**Diff:**
Move `TZ_MAP` and `getTimezoneConfig()` to `firmware/data-src/common.js` and remove them from both `dashboard.js` and `wizard.js`. Ensure `wizard.html` loads `common.js` before `wizard.js` (validate load order). Regenerate both gzipped assets.

### 2. Fix `schedule_active` Semantics

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** Field name promises in-window state but delivers config+sync state.

**Diff:**
Rename the field now to avoid a breaking change later, or add a clarifying field:

```cpp
// Option A: rename
data["schedule_ready"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();

// Option B: keep both (backward-compatible)
data["schedule_active"] = (ConfigManager::getSchedule().sched_enabled == 1) && isNtpSynced();
data["schedule_in_window"] = false;  // placeholder for fn-2.2
```

### 3. Surface Unmapped Timezones

**File:** `firmware/data-src/dashboard.js`, `firmware/data-src/wizard.js`
**Issue:** Silent UTC fallback hides unsupported zones.

**Diff:**
```javascript
function getTimezoneConfig() {
  var iana = "UTC";
  try {
    iana = Intl.DateTimeFormat().resolvedOptions().timeZone || "UTC";
  } catch (e) {}
  var posix = TZ_MAP[iana] || null;  // return null, not "UTC0"
  return { iana: iana, posix: posix };
}
```

### 4. Test the Right JSON Path

**File:** `firmware/test/test_config_manager/test_main.cpp`
**Issue:** Tests `toJson()` instead of `toExtendedJson()`.

**Diff:**
Change the test to call `SystemStatus::toExtendedJson()` with a dummy `FlightStatsSnapshot`, matching the production code path:

```cpp
void test_ntp_status_in_json_output() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::NTP, StatusLevel::WARNING, "Clock not set");

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    FlightStatsSnapshot dummy = {};
    SystemStatus::toExtendedJson(obj, dummy);  // <-- match production

    TEST_ASSERT_TRUE(obj["ntp"].is<JsonObject>());
    TEST_ASSERT_TRUE(obj["ntp"]["level"] == "warning");
    TEST_ASSERT_TRUE(obj["ntp"]["message"] == "Clock not set");
    // ...
}
```

### 5. Eliminate Cross-core `setenv()` Race

**File:** `firmware/src/main.cpp`, `firmware/adapters/WiFiManager.cpp`
**Issue:** `configTzTime()` calls non-thread-safe `setenv()/tzset()` from Core 1 while Core 0 may call `localtime_r()`.

**Fix:**
Do not call `configTzTime()` directly from the async `onChange` callback. Instead, set an atomic flag (e.g., `g_timezoneChanged`) and call `configTzTime()` from `loop()` (Core 1). Architecturally, **mandate** that `localtime_r()` is never called from Core 0; any display mode needing local time (e.g., ClockMode) must receive a pre-computed `struct tm` via `RenderContext` that is populated on Core 1.

### 6. Add SNTP Restart Debounce

**File:** `firmware/src/main.cpp`
**Issue:** Rapid timezone changes can thrash SNTP.

**Diff:**
Guard `configTzTime()` with a last-call timestamp:

```cpp
static unsigned long s_lastTzApplyMs = 0;
if (sched.timezone != s_lastAppliedTz && (millis() - s_lastTzApplyMs > 5000)) {
    s_lastAppliedTz = sched.timezone;
    s_lastTzApplyMs = millis();
    configTzTime(sched.timezone.c_str(), "pool.ntp.org", "time.nist.gov");
}
```

---

**Review Actions:**
- Issues Found: 6
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review**

**Story:** [_bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md:1)

## Review Summary

| Category | Issues Found |
|---|---:|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 3 |
| Task/Test Overclaims | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

## Evidence Score Summary

| Severity | Finding | Source | Score |
|---|---|---|---:|
| 🟠 Important | Persisted invalid `timezone` still bypasses validation on boot | `ConfigManager.cpp`, `WiFiManager.cpp` | +1.0 |
| 🟠 Important | `ntp_synced` is sticky once true and never reset on reconnect or timezone restart | `main.cpp`, `WiFiManager.cpp`, `WebPortal.cpp` | +1.0 |
| 🟠 Important | `/api/status.schedule_active` reports “enabled + synced”, not actual active schedule state | `WebPortal.cpp` | +1.0 |
| 🟠 Important | Story claims AC-level unit coverage that the tests do not provide | story md, `test_config_manager/test_main.cpp` | +1.0 |
| 🟡 Minor | Story file list/completion log does not match the current git diff on `main` | story md, `git diff --name-only` | +0.3 |
| 🟡 Minor | JS helper silently maps unsupported IANA zones to `UTC0`, and the map is duplicated in two bundles | `dashboard.js`, `wizard.js` | +0.3 |
| 🟢 Clean Pass | No new security or memory-safety defect stood out in this story slice | static review | -0.5 |

**Evidence Score:** `4.1`  
**Verdict:** `MAJOR REWORK`

## Findings

- **[Important] Persisted invalid timezone data still reaches `configTzTime()`.** [ConfigManager.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/core/ConfigManager.cpp:395) loads `timezone` from NVS with no validation, while [WiFiManager.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WiFiManager.cpp:132) immediately passes that value into `configTzTime()`. The empty-string fix only protects JSON writes; it does not protect devices already carrying bad NVS data from earlier builds or manual prefs writes.

- **[Important] `ntp_synced` never returns to `false` for a fresh sync attempt.** [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:79) only ever sets `g_ntpSynced` to `true`. Neither [WiFiManager::_onDisconnected()](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WiFiManager.cpp:144) nor the timezone hot-reload path in [main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/src/main.cpp:663) clears it before starting a new SNTP cycle. After one successful sync, `/api/status` can report `ntp_synced: true` forever even if later reconnects or timezone changes fail, which contradicts AC #3’s intended failure behavior.

- **[Important] `schedule_active` is an API contract lie.** [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:787) computes it as `sched_enabled && isNtpSynced()`. That is not “active”; it is “feature enabled and clock once synced.” As implemented, the field becomes true all day long once sync succeeds. That will mislead any UI or automation consuming `/api/status`.

- **[Important] Task 7 materially overclaims test coverage.** The story marks AC #2/#3/#6 test work complete at [_story md_](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md:87) and claims all ACs are satisfied at [_story md_](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md:319), but the added tests at [test_main.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/test/test_config_manager/test_main.cpp:494) only poke `SystemStatus` and `ConfigManager` directly. They never exercise `onNtpSync()`, never verify `isNtpSynced()`, never drive `WiFiManager::_onConnected()`, and never build the real `/api/status` payload.

- **[Minor] The story’s file-level audit trail does not match the current git reality.** The story says this work modified `WiFiManager.cpp`, `WebPortal.cpp`, both JS sources, and both `.gz` assets at [_story md_](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md:133), but the current uncommitted diff on `main` only contains `firmware/core/ConfigManager.cpp`, `firmware/src/main.cpp`, and `firmware/test/test_config_manager/test_main.cpp`. That makes the “Done” record harder to audit from git alone.

- **[Minor] Unsupported browser timezones silently become UTC, and the bug is duplicated.** [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:94) and [wizard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/wizard.js:93) both return `TZ_MAP[iana] || "UTC0"`. Common zones such as `Asia/Manila` and `Europe/Kyiv` are absent from the 71-entry map, so the helper will quietly misconfigure local time instead of surfacing “unsupported timezone.” Because the map is copy-pasted in two files, every correction now requires dual edits and dual gzip rebuilds.

## Suggested Fixes

- Validate all schedule keys on `loadFromNvs()`, especially `timezone`, and fall back to defaults before any caller can consume them.
- Clear `g_ntpSynced` and restore `SystemStatus::NTP` to `WARNING` whenever a new SNTP cycle starts or WiFi drops; only the completion callback should promote it back to `OK`.
- Either compute real in-window schedule state in `/api/status`, or rename the field now to something truthful like `schedule_enabled`.
- Split the NTP/status logic into testable helpers and add real tests for callback state transitions plus the actual `/api/status` JSON shape.
- Move `TZ_MAP` into one shared bundle and return `{ iana, posix: null }` for unmapped zones so the UI can warn instead of silently using UTC.

## Review Actions

- Issues Found: `6`
- Issues Fixed: `0`
- Action Items Created: `6`

Static review only. I did not run `pio run` or on-device tests in this read-only sandbox.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

I'll conduct a thorough adversarial code review of story fn-2.1. Let me examine all the implementation files.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-13</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-2</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">3</var>
<var name="session_id">047fc2f0-1d05-4b21-b03a-80a6a675d01e</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="2c306e06">embedded in prompt, file id: 2c306e06</var>
<var name="story_id">fn-2.1</var>
<var name="story_key">fn-2-1-ntp-time-sync-and-timezone-configuration</var>
<var name="story_num">1</var>
<var name="story_title">1-ntp-time-sync-and-timezone-configuration</var>
<var name="template">False</var>
<var name="timestamp">20260413_0904</var>
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
      - Commit message format: fix(component): brief description (synthesis-fn-2.1)
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