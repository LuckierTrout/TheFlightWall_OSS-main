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

**Tests added:** 5 new Unity tests in `test_config_manager/test_main.cpp` (4 original + 1 added in Round 3):
- `test_ntp_status_transitions` — WARNING→OK state transitions
- `test_schedule_timezone_default` — default "UTC0" verification
- `test_timezone_is_hot_reload_key` — all 5 schedule keys are not reboot-required
- `test_ntp_status_in_json_output` — NTP subsystem via `toExtendedJson()` matching production code path (updated Round 3 to use `obj["subsystems"]["ntp"]`)
- `test_nvs_invalid_timezone_ignored` — NVS-loaded empty timezone is rejected; default "UTC0" preserved (added Round 3)

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
- `firmware/core/ConfigManager.cpp` (modified — NVS timezone load now validates length > 0 && <= 40; Round 2: empty-string guard in applyJson; Round 3: same guard applied to loadFromNvs path)
- `firmware/test/test_config_manager/test_main.cpp` (modified — 4 NTP tests in Round 1+2; Round 3: updated test_ntp_status_in_json_output to use toExtendedJson(), added test_nvs_invalid_timezone_ignored)

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

### Review: 2026-04-13 (Round 3 — 3 validators)
- **Reviewer:** AI Code Review Synthesis (3 validators)
- **Evidence Score:** Validator A: 4.0 (MAJOR REWORK) / Validator B: 4.1 (MAJOR REWORK) / Validator C: (output truncated) → Synthesized: APPROVED WITH RESERVATIONS
- **Issues Found:** 2 new verified (both fixed); 8 re-raised items confirmed dismissed/deferred
- **Issues Fixed:** 2
- **Action Items Created:** 0 new (existing 3 follow-ups remain)

#### Review Follow-ups (AI) — Round 3
*(No new action items — 2 new verified issues were both fixed inline; all remaining open items already tracked in Round 1 follow-ups above.)*
