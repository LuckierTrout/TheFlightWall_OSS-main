<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-2 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T124152Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-2.1

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
4. **AC #4 (Timezone hot-reload):** `configTzTime()` called in `ConfigManager::onChange` lambda on every config change. Does NOT reset `g_ntpSynced`.
5. **AC #5 (TZ_MAP):** ~75 IANA-to-POSIX entries added to both `dashboard.js` and `wizard.js`. `getTimezoneConfig()` returns `{ iana, posix }` using `Intl.DateTimeFormat().resolvedOptions().timeZone`.
6. **AC #6 (API status):** `ntp_synced` and `schedule_active` fields added to `GET /api/status` response.

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
- `firmware/test/test_config_manager/test_main.cpp` (modified — added 4 NTP tests)


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 0 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Over-eager timezone hot-reload restarts SNTP on every config change | main.cpp | +1 |
| 🟠 IMPORTANT | SNTP callback does not verify sync status before flagging success | main.cpp | +1 |
| 🟠 IMPORTANT | No POSIX timezone string format validation in ConfigManager | ConfigManager.cpp (inferred) | +1 |
| 🟠 IMPORTANT | Potential race condition between tzset() and localtime_r() across cores | main.cpp | +1 |
| 🟡 MINOR | Hidden extern coupling violates hexagonal boundary | WebPortal.cpp | +0.3 |
| 🟡 MINOR | TZ_MAP duplicated verbatim in dashboard.js and wizard.js | dashboard.js / wizard.js | +0.3 |
| 🟡 MINOR | Unit tests bypass actual NTP callback and API endpoint integration | test_main.cpp | +0.3 |
| 🟢 CLEAN PASS | SOLID principles | — | -0.5 |
| 🟢 CLEAN PASS | Lying tests (none found) | — | -0.5 |
| 🟢 CLEAN PASS | Style violations | — | -0.5 |
| 🟢 CLEAN PASS | Type safety issues | — | -0.5 |
| 🟢 CLEAN PASS | Security vulnerabilities (none found) | — | -0.5 |

### Evidence Score: 2.7

| Score | Verdict |
|-------|---------|
| **2.7** | **APPROVED** |

---

## 🏛️ Architectural Sins

- **[4/10] Dependency Inversion:** WebPortal (an adapter) depends on a global function declared in `main.cpp` via `extern bool isNtpSynced();`, breaking the hexagonal boundary between adapters and the application core.
  - 📍 `WebPortal.cpp:30`
  - 💡 Fix: Move the `isNtpSynced()` declaration into an existing core/interface header (e.g., `core/SystemStatus.h` or a new `utils/TimeUtils.h`) and include it properly.

- **Duplication / Leaky Abstraction:** The ~75-entry `TZ_MAP` and `getTimezoneConfig()` helper are copy-pasted into both `dashboard.js` and `wizard.js`.
  - 📍 `dashboard.js:12-89` and `wizard.js:12-89`
  - 💡 Fix: Extract both into `common.js` so the two pages share a single source of truth.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

- **[MEDIUM] Unnecessary SNTP restart on every settings change:** The `ConfigManager::onChange` lambda in `main.cpp` calls `configTzTime()` unconditionally, even when the changed keys are unrelated to timezone (e.g., `brightness`, `text_color_r`, `fetch_interval`). This restarts the SNTP client, discarding any in-flight NTP request and delaying time sync.
  - 📍 `main.cpp:635-640`
  - 💡 Fix: Inspect the changed keys (or pass a change mask to the callback) and call `configTzTime()` only when `timezone` or related schedule keys actually changed.

---

## 🐛 Correctness & Safety

- **🐛 Bug: SNTP callback assumes success without status check:** `onNtpSync()` unconditionally sets `g_ntpSynced = true`. Per ESP-IDF documentation, `sntp_set_time_sync_notification_cb` can fire on any sync status change. The callback should verify `sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED` before declaring success.
  - 📍 `main.cpp:104-108`
  - 🔄 Reproduction: If ESP-IDF internally resets SNTP state (e.g., due to network change), the callback could fire with a reset event and falsely report "Clock synced".

- **🐛 Bug: Missing POSIX timezone format validation:** ConfigManager only validates that `timezone` is ≤40 characters. It does not validate POSIX TZ syntax. A malformed string (e.g., `"!!!"`) is passed directly to `configTzTime()` → `setenv()` → `tzset()`, which can silently fail or cause `localtime_r()` to return UTC incorrectly.
  - 📍 `ConfigManager.cpp` (inferred from `test_apply_json_schedule_validation`)
  - 🔄 Reproduction: POST `{"timezone":"bad_tz_string"}` to `/api/settings`. The value is accepted and later passed to `configTzTime()`, corrupting local time calculations.

- **🔒 [LOW] Race condition in global timezone state:** `configTzTime()` calls `tzset()`, which modifies newlib global variables. `localtime_r()` is called from both Core 1 (`ModeOrchestrator::tick()`) and Core 0 (`ClockMode::render()`). `tzset()` is not guaranteed to be atomic with respect to `localtime_r()` on ESP32/newlib, creating a cross-core data race.
  - 📍 `main.cpp: onChange lambda` / `core/ModeOrchestrator.cpp`
  - ⚠️ Impact: Corrupted `struct tm` values or garbled time display during/after a timezone hot-reload.

- **🎭 Weak Test Coverage:** `test_ntp_status_transitions` tests `SystemStatus` in isolation, not the actual `onNtpSync` callback mechanism. `test_ntp_status_in_json_output` tests `SystemStatus::toJson()`, not the actual `/api/status` response assembled by `WebPortal::_handleGetStatus()`. No test verifies that `configTzTime()` is actually invoked when the timezone key changes.
  - 📍 `test_main.cpp:400-450`
  - 🤥 Why it lies: The tests pass while providing no assurance that the firmware's NTP callback wiring or API endpoint extensions work correctly.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: `extern` function prototype hidden in implementation file**
  - 📍 `WebPortal.cpp:30`
  - 💥 Explosion radius: If `isNtpSynced()` signature changes or is renamed, WebPortal will fail to link with no compile-time warning during header-based compilation.

- **💣 Tech Debt: Duplicated TZ_MAP**
  - 📍 `dashboard.js:12-89` and `wizard.js:12-89`
  - 💥 Explosion radius: Any DST rule change or timezone correction requires editing two files; divergence is inevitable.

---

## 🛠️ Suggested Fixes

### 1. Guard timezone hot-reload to schedule-only changes

**File:** `firmware/src/main.cpp`
**Issue:** `configTzTime()` fires on every config change.

```cpp
// BEFORE:
ConfigManager::onChange([]() {
    g_configChanged.store(true);
    g_layout = LayoutEngine::compute(ConfigManager::getHardware());
    ScheduleConfig sched = ConfigManager::getSchedule();
    configTzTime(sched.timezone.c_str(), "pool.ntp.org", "time.nist.gov");
});

// AFTER:
ConfigManager::onChange([]() {
    g_configChanged.store(true);
    g_layout = LayoutEngine::compute(ConfigManager::getHardware());
    // Only restart SNTP if timezone actually changed
    if (ConfigManager::lastChangedKeyEquals("timezone")) {
        ScheduleConfig sched = ConfigManager::getSchedule();
        configTzTime(sched.timezone.c_str(), "pool.ntp.org", "time.nist.gov");
        LOG_I("NTP", "Timezone hot-reloaded: %s", sched.timezone.c_str());
    }
});
```

### 2. Verify SNTP sync status in callback

**File:** `firmware/src/main.cpp`
**Issue:** Callback assumes success.

```cpp
static void onNtpSync(struct timeval* tv) {
    if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        return; // Ignore reset or intermediate events
    }
    g_ntpSynced.store(true);
    SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Clock synced");
    LOG_I("NTP", "Time synchronized");
}
```

### 3. Add lightweight POSIX TZ validation

**File:** `firmware/core/ConfigManager.cpp` (in `applyJson` validation for `timezone`)
**Issue:** Any string ≤40 chars is accepted.

```cpp
// Reject strings that don't look like a POSIX TZ entry
if (value.indexOf(':') >= 0 || !isValidPosixTz(value.c_str())) {
    return false;
}
```

---

**Review Actions:**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

✅ Code is approved and ready for deployment, but address the SNTP status guard and over-eager hot-reload before the next story to avoid time-sync regressions.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review: `fn-2.1 - ntp-time-sync-and-timezone-configuration`**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-1-ntp-time-sync-and-timezone-configuration.md`

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 4 |
| Task Completion Lies | 1 |
| SOLID / Architecture Violations | 2 |
| Hidden Bugs | 3 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

## Evidence Score Summary

| Severity | Finding | Source | Score |
|----------|---------|--------|-------|
| 🟠 IMPORTANT | `g_ntpSynced` is monotonic and never reset on a new sync attempt, so `/api/status` can report stale success | `firmware/src/main.cpp` (`g_ntpSynced`, `onNtpSync`, `ConfigManager::onChange`), `firmware/adapters/WiFiManager.cpp` (`_onConnected`) | +1.0 |
| 🟠 IMPORTANT | Timezone hot-reload is wired to **every** config change, not schedule changes, so normal settings updates restart SNTP unnecessarily | `firmware/src/main.cpp` (`ConfigManager::onChange`) | +1.0 |
| 🟠 IMPORTANT | Unknown IANA zones silently fall back to `UTC0`, which misconfigures local time instead of surfacing an unmapped timezone | `firmware/data-src/dashboard.js` (`getTimezoneConfig`), `firmware/data-src/wizard.js` (`getTimezoneConfig`) | +1.0 |
| 🟠 IMPORTANT | `/api/status.schedule_active` is semantically wrong; it reports “enabled + synced”, not “currently active in dim window” | `firmware/adapters/WebPortal.cpp` (`_handleGetStatus`) | +1.0 |
| 🟠 IMPORTANT | Task 7.3 is overstated: `test_ntp_status_in_json_output()` never hits `/api/status` and never asserts `ntp_synced` / `schedule_active` | `firmware/test/test_config_manager/test_main.cpp` (`test_ntp_status_in_json_output`) | +1.0 |
| 🟡 MINOR | `TZ_MAP` is duplicated in two bundles, so DST/rule fixes now require twin edits and twin gzip rebuilds | `firmware/data-src/dashboard.js`, `firmware/data-src/wizard.js` | +0.3 |
| 🟡 MINOR | New NTP paths bypass the project logging contract with raw `Serial.println` in production code | `firmware/src/main.cpp`, `firmware/adapters/WiFiManager.cpp` | +0.3 |
| 🟢 CLEAN PASS | No direct auth/credential exposure bug introduced by this story | review-wide | -0.5 |
| 🟢 CLEAN PASS | No material type-safety issue found beyond existing C++/JS conventions | review-wide | -0.5 |

### Evidence Score: **4.6**

| Score | Verdict |
|-------|---------|
| **4.6** | **MAJOR REWORK** |

## Architectural Sins

- **[7/10] Single Responsibility breach:** the global `ConfigManager::onChange` hook now mixes display invalidation, layout recompute, and SNTP reconfiguration. That couples unrelated config domains and puts network/time side effects on the generic settings path.  
  Location: `firmware/src/main.cpp` (`ConfigManager::onChange` lambda)

- **[5/10] Wrong abstraction:** timezone mapping is hardcoded twice in two separate bundles instead of being shared once. That is already causing duplicated behavior bugs and will drift on future DST fixes.  
  Location: `firmware/data-src/dashboard.js`, `firmware/data-src/wizard.js`

## Correctness & Safety Findings

- **Stale NTP success state:** after the first successful sync, `g_ntpSynced` stays `true` forever for the lifetime of the process. WiFi reconnects and timezone changes start a fresh sync attempt but never clear the flag or restore `NTP=WARNING`. Reproduction: sync once, disconnect WAN or change timezone, then query `/api/status`; the API still reports `ntp_synced: true`.  
  Location: `firmware/src/main.cpp`, `firmware/adapters/WiFiManager.cpp`

- **Silent wrong-timezone fallback:** `getTimezoneConfig()` uses `TZ_MAP[iana] || "UTC0"`. For any browser zone not present in the table, the device silently schedules in UTC. Reproduction: run the UI in an unmapped zone such as `Asia/Manila`; `posix` becomes `UTC0` with no warning.  
  Location: `firmware/data-src/dashboard.js`, `firmware/data-src/wizard.js`

- **Misleading status contract:** `schedule_active` is not computed from the current clock window; it is just `(sched_enabled && ntp_synced)`. A 23:00-07:00 dim schedule will report active at 14:00, which will break any dashboard logic that trusts the field name.  
  Location: `firmware/adapters/WebPortal.cpp`

## Performance & Scalability

- **Unnecessary SNTP churn on normal settings writes:** the main config callback calls `configTzTime()` for every settings mutation, including display/hardware changes. That restarts SNTP on a hot path that should have been schedule-only, and it is especially bad because calibration/mapping edits already post rapidly.  
  Location: `firmware/src/main.cpp`

## Test Quality

- **Lying test:** `test_ntp_status_in_json_output()` does not test the new API fields added by this story. It only checks that `SystemStatus::toJson()` emits an `ntp` object. If `WebPortal::_handleGetStatus()` dropped `ntp_synced` and `schedule_active` tomorrow, this test would still pass.  
  Location: `firmware/test/test_config_manager/test_main.cpp`

- **Coverage gap:** there is no test seam around `WiFiManager::_onConnected()` or the `ConfigManager::onChange` hot-reload path, so the two highest-risk changes in the story are effectively unverified.  
  Location: `firmware/adapters/WiFiManager.cpp`, `firmware/src/main.cpp`

## Maintainability Issues

- **Duplicated timezone table:** the same 70+ entry `TZ_MAP` is maintained in two files. That is pure drift bait and directly amplifies timezone correctness bugs.  
  Location: `firmware/data-src/dashboard.js`, `firmware/data-src/wizard.js`

- **Logging-rule violation:** the story notes explicitly said to use `LOG_I/LOG_E/LOG_V`, but the implementation added raw `Serial.println` calls instead. That breaks tag consistency and makes filtering/reviewing logs harder.  
  Location: `firmware/src/main.cpp`, `firmware/adapters/WiFiManager.cpp`

## Suggested Fixes

1. Restrict `configTzTime()` to actual schedule/timezone changes; do not run it from the generic config callback for unrelated keys.
2. Introduce explicit NTP state transitions: set `g_ntpSynced=false` and `SystemStatus::WARNING` when a fresh sync attempt starts, then flip to OK only from the SNTP callback.
3. Change `getTimezoneConfig()` to return `{ iana, posix: null }` for unmapped zones, then force a manual fallback path instead of silently using UTC.
4. Rename `schedule_active` to `schedule_enabled` now, or compute the real in-window state before exposing it.
5. Replace the fake JSON test with a WebPortal-level status test that asserts `ntp_synced` and `schedule_active` directly.
6. Deduplicate `TZ_MAP` into one shared asset or generation step, then rebuild both gzipped bundles from the shared source.

**Review Actions**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

**Verdict:** address the findings before treating this story as done. The implementation is close enough to demo, but the status semantics, timezone fallback, and test claims are not at senior-review quality.

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
<var name="reviewer_count">2</var>
<var name="session_id">3e4f9d30-b2b6-4993-a1f9-3998cadb5aec</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="2c306e06">embedded in prompt, file id: 2c306e06</var>
<var name="story_id">fn-2.1</var>
<var name="story_key">fn-2-1-ntp-time-sync-and-timezone-configuration</var>
<var name="story_num">1</var>
<var name="story_title">1-ntp-time-sync-and-timezone-configuration</var>
<var name="template">False</var>
<var name="timestamp">20260413_0841</var>
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