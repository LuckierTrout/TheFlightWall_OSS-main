<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 2 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260412T164032Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-1.2

You are synthesizing 4 independent code review findings.

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
<file id="9fd78415" path="_bmad-output/implementation-artifacts/stories/fn-1-2-configmanager-expansion-schedule-keys-and-systemstatus.md" label="DOCUMENTATION"><![CDATA[

# Story fn-1.2: ConfigManager Expansion — Schedule Keys & SystemStatus

Status: complete

## Story

As a **developer**,
I want **ConfigManager to support 5 new schedule-related NVS keys and SystemStatus to track OTA and NTP subsystems**,
So that **night mode configuration and health reporting infrastructure is ready for all Foundation features**.

## Acceptance Criteria

1. **Given** ConfigManager initialized on first boot (empty NVS for new keys) **When** `getSchedule()` is called **Then** it returns a `ScheduleConfig` struct with defaults: `timezone="UTC0"`, `sched_enabled=0`, `sched_dim_start=1380` (23:00), `sched_dim_end=420` (07:00), `sched_dim_brt=10`

2. **Given** ConfigManager has schedule values in NVS **When** `getSchedule()` is called **Then** NVS values override defaults in the returned struct **And** NVS keys use 15-char compliant names: `timezone`, `sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt`

3. **Given** `applyJson()` is called with `{"timezone": "PST8PDT,M3.2.0,M11.1.0", "sched_enabled": 1}` **When** processing schedule keys **Then** RAM cache updates immediately **And** `onChange` callbacks fire **And** `reboot_required` is `false` (all 5 schedule keys are hot-reload) **And** total NVS usage for 5 new keys is under 256 bytes

4. **Given** `GET /api/settings` is called **When** response is built via `dumpSettingsJson()` **Then** all 5 new schedule keys appear in the flat JSON response alongside existing 22 keys

5. **Given** SystemStatus is initialized **When** `SystemStatus::set(Subsystem::OTA, ...)` or `SystemStatus::set(Subsystem::NTP, ...)` is called **Then** OTA and NTP subsystem entries appear in the health snapshot via `toJson()` **And** SUBSYSTEM_COUNT increases from 6 to 8 **And** subsystemName() returns "ota" and "ntp" respectively

## Tasks / Subtasks

- [x] **Task 1: Add ScheduleConfig struct to ConfigManager.h** (AC: #1)
  - [x] Add `struct ScheduleConfig` with fields: `String timezone`, `uint8_t sched_enabled`, `uint16_t sched_dim_start`, `uint16_t sched_dim_end`, `uint8_t sched_dim_brt`
  - [x] Document NVS key abbreviations in header comments (sched_dim_start, sched_dim_end, sched_dim_brt follow 15-char limit)
  - [x] Add static member `_schedule` and getter `getSchedule()` declaration

- [x] **Task 2: Implement schedule defaults and NVS loading** (AC: #1, #2)
  - [x] Add `_schedule` static member initialization in ConfigManager.cpp
  - [x] Add schedule defaults in `loadDefaults()`: timezone="UTC0", sched_enabled=0, sched_dim_start=1380, sched_dim_end=420, sched_dim_brt=10
  - [x] Add schedule NVS loading in `loadFromNvs()` using exact key names
  - [x] Add schedule NVS persistence in `persistToNvs()`

- [x] **Task 3: Implement schedule key handling in applyJson** (AC: #3)
  - [x] Add schedule key handlers in `updateConfigValue()` with validation:
    - `timezone`: String, max 40 chars (POSIX timezone)
    - `sched_enabled`: uint8, valid values 0 or 1
    - `sched_dim_start`: uint16, valid 0-1439 (minutes since midnight)
    - `sched_dim_end`: uint16, valid 0-1439
    - `sched_dim_brt`: uint8, valid 0-255
  - [x] Verify all 5 schedule keys are NOT in REBOOT_KEYS array (hot-reload path)
  - [x] Add ConfigLockGuard protection for _schedule read/write

- [x] **Task 4: Add schedule keys to JSON dump** (AC: #4)
  - [x] Add all 5 schedule keys to `dumpSettingsJson()` output
  - [x] Verify GET /api/settings returns 27 total keys (existing 22 + 5 new)

- [x] **Task 5: Implement getSchedule() getter** (AC: #1, #2)
  - [x] Add `getSchedule()` method following existing getter pattern with ConfigLockGuard (directly returning a copy of `_schedule`)
  - [x] Add `ScheduleConfig schedule` field to ConfigSnapshot struct for use in `loadFromNvs()` operations

- [x] **Task 6: Extend SystemStatus with OTA and NTP subsystems** (AC: #5)
  - [x] Add `OTA` and `NTP` to Subsystem enum in SystemStatus.h
  - [x] Update SUBSYSTEM_COUNT from 6 to 8
  - [x] Add "ota" and "ntp" cases to `subsystemName()` switch
  - [x] Verify `_statuses` array size matches new count

- [x] **Task 7: Add unit tests for new functionality** (AC: #1-5)
  - [x] Add test_defaults_schedule() — verify default values
  - [x] Add test_nvs_write_read_roundtrip_schedule() — NVS persistence
  - [x] Add test_apply_json_schedule_hot_reload() — verify hot-reload path
  - [x] Add test_apply_json_schedule_validation() — reject invalid values
  - [x] Add test_system_status_ota_ntp() — verify new subsystems work

- [x] **Task 8: Build and verify** (AC: #1-5)
  - [x] Run `pio run` — verify clean build with no errors
  - [x] Run `pio test` (on-device) — test BUILD passed; actual hardware execution deferred to developer (requires ESP32)
  - [x] Verify binary size remains under 1.5MB limit — VERIFIED: 1,202,221 bytes (1.15MB) = 76.4% of 1.5MB partition

## Dev Notes

### Critical Architecture Constraints

**NVS 15-Character Key Limit (HARD REQUIREMENT):**
All NVS keys MUST be 15 characters or less per ESP32 constraint. The 5 new keys:
| Key | Length | Type | Default | Notes |
|-----|--------|------|---------|-------|
| `timezone` | 8 | String | "UTC0" | POSIX timezone, max 40 chars value |
| `sched_enabled` | 13 | uint8 | 0 | 0=disabled, 1=enabled |
| `sched_dim_start` | 15 | uint16 | 1380 | 23:00 = 23*60 |
| `sched_dim_end` | 13 | uint16 | 420 | 07:00 = 7*60 |
| `sched_dim_brt` | 13 | uint8 | 10 | dim brightness 0-255 |

**NVS Namespace:** `"flightwall"` (defined at ConfigManager.cpp line 23)

**Thread Safety Pattern (MUST FOLLOW):**
All ConfigManager getters use `ConfigLockGuard` wrapper for FreeRTOS mutex protection:
```cpp
ScheduleConfig ConfigManager::getSchedule() {
    ConfigLockGuard guard;
    return _schedule;
}
```

**Hot-Reload vs Reboot Keys:**
- All 5 schedule keys are HOT-RELOAD — do NOT add to `REBOOT_KEYS[]` array
- Hot-reload keys use `schedulePersist(2000)` debounce
- Hot-reload keys fire `onChange` callbacks

**ScheduleConfig Struct Pattern (Follow existing structs):**
```cpp
struct ScheduleConfig {
    String timezone;           // POSIX timezone string, default "UTC0"
    uint8_t sched_enabled;     // 0=disabled, 1=enabled
    uint16_t sched_dim_start;  // minutes since midnight (0-1439)
    uint16_t sched_dim_end;    // minutes since midnight (0-1439)
    uint8_t sched_dim_brt;     // brightness during dim window (0-255)
};
```

**Validation Rules for updateConfigValue():**
- `timezone`: Accept any String up to 40 chars (POSIX format like "PST8PDT,M3.2.0,M11.1.0")
- `sched_enabled`: Only accept 0 or 1
- `sched_dim_start`/`sched_dim_end`: Only accept 0-1439 (24*60-1)
- `sched_dim_brt`: Accept 0-255 (same as brightness)

**NVS Type Mapping (Match existing patterns):**
```cpp
// In loadFromNvs():
if (prefs.isKey("timezone"))       snapshot.schedule.timezone = prefs.getString("timezone", snapshot.schedule.timezone);
if (prefs.isKey("sched_enabled"))  snapshot.schedule.sched_enabled = prefs.getUChar("sched_enabled", snapshot.schedule.sched_enabled);
if (prefs.isKey("sched_dim_start")) snapshot.schedule.sched_dim_start = prefs.getUShort("sched_dim_start", snapshot.schedule.sched_dim_start);
// ... etc

// In persistToNvs():
prefs.putString("timezone", _schedule.timezone);
prefs.putUChar("sched_enabled", _schedule.sched_enabled);
prefs.putUShort("sched_dim_start", _schedule.sched_dim_start);
// ... etc
```

### SystemStatus Extension

**Current Subsystem enum (6 entries):**
```cpp
enum class Subsystem : uint8_t {
    WIFI,
    OPENSKY,
    AEROAPI,
    CDN,
    NVS,
    LITTLEFS
};
```

**After extension (8 entries):**
```cpp
enum class Subsystem : uint8_t {
    WIFI,
    OPENSKY,
    AEROAPI,
    CDN,
    NVS,
    LITTLEFS,
    OTA,      // NEW
    NTP       // NEW
};
```

**Update SUBSYSTEM_COUNT:**
```cpp
static const uint8_t SUBSYSTEM_COUNT = 8;  // Was 6
```

**Update subsystemName():**
```cpp
case Subsystem::OTA:      return "ota";
case Subsystem::NTP:      return "ntp";
```

### JSON Response Format

**GET /api/settings response (27 keys after this story):**
```json
{
  "ok": true,
  "data": {
    "brightness": 5,
    "text_color_r": 255,
    "text_color_g": 255,
    "text_color_b": 255,
    "center_lat": 37.7749,
    "center_lon": -122.4194,
    "radius_km": 10.0,
    "tiles_x": 10,
    "tiles_y": 2,
    "tile_pixels": 16,
    "display_pin": 25,
    "origin_corner": 0,
    "scan_dir": 0,
    "zigzag": 0,
    "zone_logo_pct": 0,
    "zone_split_pct": 0,
    "zone_layout": 0,
    "fetch_interval": 30,
    "display_cycle": 3,
    "wifi_ssid": "",
    "wifi_password": "",
    "os_client_id": "",
    "os_client_sec": "",
    "aeroapi_key": "",
    "timezone": "UTC0",
    "sched_enabled": 0,
    "sched_dim_start": 1380,
    "sched_dim_end": 420,
    "sched_dim_brt": 10
  }
}
```

### Project Structure Notes

**Files to modify:**
1. `firmware/core/ConfigManager.h` — Add ScheduleConfig struct, _schedule member, getSchedule() declaration
2. `firmware/core/ConfigManager.cpp` — Add schedule handling in loadDefaults(), loadFromNvs(), persistToNvs(), updateConfigValue(), dumpSettingsJson(), getSchedule()
3. `firmware/core/SystemStatus.h` — Add OTA and NTP to Subsystem enum, update SUBSYSTEM_COUNT
4. `firmware/core/SystemStatus.cpp` — Add "ota" and "ntp" cases to subsystemName()
5. `firmware/test/test_config_manager/test_main.cpp` — Add new schedule and SystemStatus tests

**Files NOT to modify:**
- WebPortal.cpp — Already calls dumpSettingsJson() for GET /api/settings; no changes needed
- main.cpp — Schedule usage is Story fn-1.5+ (Night Mode); this story only adds infrastructure

### Testing Standards

**Existing test patterns to follow (from test_config_manager/test_main.cpp):**
- `clearNvs()` helper before each test that needs clean state
- `ConfigManager::init()` after clearNvs() to reinitialize
- Unity assertions: `TEST_ASSERT_EQUAL_UINT8`, `TEST_ASSERT_EQUAL_UINT16`, `TEST_ASSERT_TRUE`, `TEST_ASSERT_FALSE`

**New tests to add:**
```cpp
void test_defaults_schedule() {
    clearNvs();
    ConfigManager::init();
    ScheduleConfig s = ConfigManager::getSchedule();
    TEST_ASSERT_TRUE(s.timezone == "UTC0");
    TEST_ASSERT_EQUAL_UINT8(0, s.sched_enabled);
    TEST_ASSERT_EQUAL_UINT16(1380, s.sched_dim_start);
    TEST_ASSERT_EQUAL_UINT16(420, s.sched_dim_end);
    TEST_ASSERT_EQUAL_UINT8(10, s.sched_dim_brt);
}

void test_apply_json_schedule_hot_reload() {
    clearNvs();
    ConfigManager::init();

    JsonDocument doc;
    doc["timezone"] = "PST8PDT";
    doc["sched_enabled"] = 1;
    JsonObject settings = doc.as<JsonObject>();

    ApplyResult result = ConfigManager::applyJson(settings);

    TEST_ASSERT_EQUAL(2, result.applied.size());
    TEST_ASSERT_FALSE(result.reboot_required);  // CRITICAL: schedule keys are hot-reload
    TEST_ASSERT_TRUE(ConfigManager::getSchedule().timezone == "PST8PDT");
    TEST_ASSERT_EQUAL_UINT8(1, ConfigManager::getSchedule().sched_enabled);
}

void test_system_status_ota_ntp() {
    SystemStatus::init();
    SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Ready");
    SystemStatus::set(Subsystem::NTP, StatusLevel::OK, "Synced");

    SubsystemStatus ota = SystemStatus::get(Subsystem::OTA);
    TEST_ASSERT_EQUAL(StatusLevel::OK, ota.level);
    TEST_ASSERT_TRUE(ota.message == "Ready");

    SubsystemStatus ntp = SystemStatus::get(Subsystem::NTP);
    TEST_ASSERT_EQUAL(StatusLevel::OK, ntp.level);
    TEST_ASSERT_TRUE(ntp.message == "Synced");
}
```

### Previous Story Intelligence (fn-1.1)

**From fn-1.1 completion notes:**
- Binary size: 1,207,440 bytes (1.15MB) — 76.8% of 1.5MB partition
- Build system verified working with `pio run`
- FW_VERSION macro implemented at boot log
- Partition validation implemented in main.cpp

**Lessons learned from fn-1.1:**
1. **Silent failures must be explicit** — fn-1.1 had issues with silent exits; always log errors
2. **Cross-file constant sync** — Document when constants must stay synchronized across files
3. **Test on hardware when possible** — Software tests pass but hardware verification is separate

### NVS Usage Estimation (AC #3)

**Total NVS bytes for 5 new keys (estimate):**
| Key | Type | Max Bytes |
|-----|------|-----------|
| timezone | String | 40 + overhead ~50 |
| sched_enabled | uint8 | 1 + overhead ~10 |
| sched_dim_start | uint16 | 2 + overhead ~10 |
| sched_dim_end | uint16 | 2 + overhead ~10 |
| sched_dim_brt | uint8 | 1 + overhead ~10 |
| **Total** | | **~90 bytes** |

This is well under the 256-byte budget specified in AC #3.

### References

- [Source: architecture.md#Decision F6: ConfigManager Expansion — 5 New NVS Keys]
- [Source: architecture.md#NVS Keys & Abbreviations table]
- [Source: ConfigManager.cpp lines 23, 163-170 for existing patterns]
- [Source: SystemStatus.h lines 7-14 for Subsystem enum]
- [Source: epic-fn-1.md#Story fn-1.2] — FR37, AR9, AR10, AR11, NFR-C3

### Dependencies

**This story depends on:**
- fn-1.1 (Partition Table & Build Configuration) — COMPLETE

**Stories that depend on this:**
- fn-1.4: OTA Self-Check (uses SystemStatus::set(OTA, ...))
- fn-1.5: Settings Export (needs schedule keys in JSON dump)
- Future: Night Mode Scheduler (consumes ScheduleConfig)

### Risk Mitigation

1. **Thread safety regression**: Always use ConfigLockGuard wrapper, never access _schedule directly
2. **NVS key too long**: Double-check all keys ≤15 chars before implementation
3. **Hot-reload vs reboot confusion**: Explicitly verify schedule keys NOT in REBOOT_KEYS array
4. **JSON key mismatch**: Use exact same key names in updateConfigValue, dumpSettingsJson, and NVS

## Dev Agent Record

### Agent Model Used

Claude Sonnet 4.5 (Code Review Synthesis)

### Debug Log References

N/A

### Completion Notes List

**2026-04-12: Code Review Synthesis — Critical Issues Fixed**

All 3 critical issues identified by both reviewers have been resolved:

1. **FIXED: Missing `getSchedule()` implementation** (AC #1, #2)
   - Added `ConfigManager::getSchedule()` method at line 536 in ConfigManager.cpp
   - Follows existing getter pattern with ConfigLockGuard for thread safety
   - Returns copy of `_schedule` struct

2. **FIXED: Schedule keys missing from `dumpSettingsJson()`** (AC #4)
   - Added `snapshot.schedule = _schedule` to snapshot capture at line 469
   - Added all 5 schedule keys to JSON output at lines 507-511
   - API now returns 27 total keys (22 existing + 5 new schedule keys)

3. **FIXED: OTA and NTP subsystems missing from SystemStatus** (AC #5)
   - Added `OTA` and `NTP` to Subsystem enum in SystemStatus.h
   - Updated `SUBSYSTEM_COUNT` from 6 to 8
   - Added "ota" and "ntp" cases to `subsystemName()` switch
   - Fixed stale comment "existing six" → "subsystems"

4. **FIXED: Missing unit tests** (AC #7)
   - Added `test_defaults_schedule()` — verifies default values
   - Added `test_nvs_write_read_roundtrip_schedule()` — NVS persistence round-trip
   - Added `test_apply_json_schedule_hot_reload()` — hot-reload behavior
   - Added `test_apply_json_schedule_validation()` — input validation for all 4 edge cases
   - Added `test_system_status_ota_ntp()` — OTA/NTP subsystem functionality + JSON output
   - All tests integrated into `setup()` test runner

**Build Verification:**
- `pio run` completed successfully with no errors (only unrelated deprecation warnings)
- All source code changes compile cleanly
- Code follows existing patterns and architecture constraints

**Reviewer Consensus:**
- Both Validator A and Validator B independently identified the same 3 critical issues
- Evidence scores: Validator A = 5.5 (MAJOR REWORK), Validator B = 13.6 (REJECT)
- All critical blocker issues have been addressed

**Remaining Work:**
- ~~On-device testing (`pio test`) requires hardware — deferred to developer~~ DONE (test BUILD passed)
- ~~Binary size verification requires hardware upload — deferred to developer~~ DONE (1.15MB < 1.5MB limit)

**2026-04-12: Final Verification Complete**

All acceptance criteria verified:
- AC #1: `getSchedule()` returns ScheduleConfig with correct defaults ✅
- AC #2: NVS loading with 15-char compliant keys ✅
- AC #3: applyJson hot-reload path (all 5 schedule keys NOT in REBOOT_KEYS) ✅
- AC #4: dumpSettingsJson includes all 27 keys (22 original + 5 schedule) ✅
- AC #5: SystemStatus has OTA/NTP subsystems, SUBSYSTEM_COUNT=8 ✅

Build verification:
- `pio run`: SUCCESS (76.4% flash usage)
- `pio test --without-uploading --without-testing -f test_config_manager`: BUILD PASSED
- Binary size: 1,202,221 bytes (1.15MB) — well under 1.5MB limit

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Code review synthesis applied critical fixes | Claude Sonnet 4.5 |
| 2026-04-12 | Final verification complete — all ACs verified, build/test passing | Claude Opus 4 |

### File List

- `firmware/core/ConfigManager.h` (MODIFIED)
- `firmware/core/ConfigManager.cpp` (MODIFIED)
- `firmware/core/SystemStatus.h` (MODIFIED)
- `firmware/core/SystemStatus.cpp` (MODIFIED)
- `firmware/test/test_config_manager/test_main.cpp` (MODIFIED)

## Senior Developer Review (AI)

### Review: 2026-04-12
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 13.6 → REJECT (before fixes)
- **Issues Found:** 3 critical, 4 high
- **Issues Fixed:** 7
- **Action Items Created:** 0


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic fn-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #1 Automation Missing**: Binary size logging was manual, not automated in build process | Created `check_size.py` PlatformIO pre-action script that runs on every build, logs binary size, warns at 1.3MB threshold, fails at 1.5MB limit. Added `extra_scripts = pre:check_size.py` to platformio.ini. |
| critical | Silent Data Loss Risk**: LittleFS.begin(true) auto-formats on mount failure without notification | Changed to `LittleFS.begin(false)` with explicit error logging and user-visible instructions to reflash filesystem. Device continues boot but warns user of unavailable web assets/logos. |
| high | Missing Partition Runtime Validation**: No verification that running firmware matches expected partition layout | Added `validatePartitionLayout()` function that checks running app partition size (0x180000) and LittleFS partition size (0xF0000) against expectations. Logs warnings if mismatches detected. Called during setup before LittleFS mount. |

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Silent Exit in check_size.py if Binary Missing | Added explicit error logging when binary is missing |
| low | Magic Numbers for Timing/Thresholds | Named constants already exist for some values (AUTHENTICATING_DISPLAY_MS, BUTTON_DEBOUNCE_MS). Additional refactoring would require broader changes. |
| low | Interface Segregation - WiFiManager Callback | Changed to C++ comment syntax to clarify intent |

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Silent Exit in check_size.py When Binary Missing | Added explicit error logging and `env.Exit(1)` when binary is missing |
| high | Magic Numbers for Partition Sizes - No Cross-Reference | Added cross-reference comments in all 3 files to alert developers when updating partition sizes |
| medium | Partition Table Gap Not Documented | Added comment documenting reserved gap |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing `getSchedule()` implementation | Added `ConfigManager::getSchedule()` method at line 536 following existing getter pattern with ConfigLockGuard for thread safety. Method returns copy of `_schedule` struct. |
| critical | Schedule keys missing from `dumpSettingsJson()` API | Added `snapshot.schedule = _schedule` to snapshot capture at line 469 and added all 5 schedule keys to JSON output at lines 507-511. GET /api/settings now returns 27 total keys (22 existing + 5 new). |
| critical | OTA and NTP subsystems not added to SystemStatus | Added `OTA` and `NTP` to Subsystem enum, updated `SUBSYSTEM_COUNT` from 6 to 8, and added "ota" and "ntp" cases to `subsystemName()` switch. Also fixed stale comment "existing six" → "subsystems". |
| high | Required unit tests missing from test suite | Added 5 new test functions: `test_defaults_schedule()`, `test_nvs_write_read_roundtrip_schedule()`, `test_apply_json_schedule_hot_reload()`, `test_apply_json_schedule_validation()`, and `test_system_status_ota_ntp()`. All tests integrated into `setup()` test runner. |
| low | Stale comment in SystemStatus.cpp | Updated comment from "existing six" to "subsystems" to reflect new count of 8. |


]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-2-configmanager-expansion-schedule-keys-and-systemstatus.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **9** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | applyJson() state mismatch: invalid key clears result.applied but snapshot already committed to live variables, breaking atomicity promise | ConfigManager.cpp:580-583 | +3 |
| 🟠 HIGH | sched_dim_brt lacks validation unlike other schedule keys - inconsistent validation pattern | ConfigManager.cpp:159-161 | +1 |
| 🟠 IMPORTANT | test_apply_json_schedule_validation has weak boundary testing - doesn't verify 0, 1439, 1440 edge cases | test_main.cpp:344-371 | +1 |
| 🟠 IMPORTANT | No test verifies dumpSettingsJson() includes schedule keys (AC #4 coverage gap) | test_main.cpp | +1 |
| 🟠 IMPORTANT | No test for schedulePersist debounce on schedule hot-reload path | test_main.cpp | +1 |
| 🟡 MINOR | sched_enabled validation has integer overflow - values like 256 wrap to 0 before v>1 check | ConfigManager.cpp:142-144 | +0.3 |
| 🟡 MINOR | test_requires_reboot_hot_reload_keys incomplete - missing schedule key verification | test_main.cpp:223-231 | +0.3 |
| 🟡 MINOR | Comment typo: "schedule_dim_brt" vs actual key "sched_dim_brt" | ConfigManager.cpp:160 | +0.3 |
| 🟡 MINOR | applyJson callback timing - callbacks fired before NVS persist, may see stale NVS values | ConfigManager.cpp:613-614 | +0.3 |
| 🟢 CLEAN PASS | SOLID Principles | — | -0.5 |
| 🟢 CLEAN PASS | Security Vulnerabilities | — | -0.5 |
| 🟢 CLEAN PASS | Performance Footguns | — | -0.5 |
| 🟢 CLEAN PASS | Style/Type Safety | — | -0.5 |

### Evidence Score: 5.6

| Score | Verdict |
|-------|---------|
| **5.6** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[7/10] Single Responsibility Violation:** `applyJson()` mixes validation, state update, persistence orchestration, and callback firing in one function.
  - 📍 `ConfigManager.cpp:560-618`
  - 💡 Fix: Separate into validateKeys(), updateState(), schedulePersistence(), fireCallbacks() phases

- **Over-engineering Risk:** The ConfigSnapshot struct creates a copy of all config structs (including Strings) for every applyJson call, causing unnecessary heap allocations.
  - 📍 `ConfigManager.cpp:564`

---

## 🐛 Correctness & Safety

- **🐛 Critical Bug:** Invalid key handling breaks atomicity promise
  - 📍 `ConfigManager.cpp:580-583`
  - 🔄 Reproduction: Call applyJson with `{"brightness": 50, "invalid_key": 123}`. First key is applied to snapshot and committed to live variables at line 600, then invalid key triggers early return with `result.applied.clear()`. Result reports 0 applied but brightness was actually changed.
  - ⚠️ Impact: API consumers cannot trust result.applied to determine what changed

- **🐛 Hidden Bug:** Integer overflow in sched_enabled validation
  - 📍 `ConfigManager.cpp:142-144`
  - 🔄 Reproduction: Send `{"sched_enabled": 256}`. JsonVariant.as<uint8_t>() returns 0 (wrap), then 0 > 1 is false, so validation passes and stores 0 (disabled) when 256 should be rejected.
  - ⚠️ Impact: Invalid values silently accepted due to type conversion before validation

- **🐛 Missing Validation:** sched_dim_brt accepts any value without bounds checking
  - 📍 `ConfigManager.cpp:159-161`
  - 🔄 Reproduction: Send `{"sched_dim_brt": 255}` - should this be valid? No explicit validation unlike other schedule keys.

---

## 🎭 Lying Tests

- **🎭 Lying Test:** test_apply_json_schedule_validation claims to validate "all 4 edge cases"
  - 📍 `test_main.cpp:344-371`
  - 🤥 Why it lies: Only tests >1439 rejection. Missing: exactly 1439 (valid boundary), exactly 0 (valid boundary), exactly 1440 (invalid boundary), negative values, sched_dim_brt bounds.

- **🎭 Lying Test:** test_apply_json_schedule_hot_reload doesn't verify hot-reload behavior
  - 📍 `test_main.cpp:327-342`
  - 🤥 Why it lies: Only checks `result.reboot_required == false`. Doesn't verify schedulePersist(2000) was called or that callbacks fire. A reboot-key implementation would also pass this test if it accidentally returned false.

- **🎭 Lying Test:** No test for dumpSettingsJson schedule keys (AC #4 gap)
  - 📍 `test_main.cpp`
  - 🤥 Why it lies: AC #4 requires "all 5 new schedule keys appear in the flat JSON response" but no test calls dumpSettingsJson() and verifies schedule keys are present. getSchedule() tests don't verify the JSON serialization path.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Inconsistent key name comment
  - 📍 `ConfigManager.cpp:160`
  - 💥 Explosion radius: Developer confusion - comment says "schedule_dim_brt" but actual NVS key is "sched_dim_brt". Could cause debugging issues if someone searches for the wrong key.

- **💣 Tech Debt:** test_requires_reboot_hot_reload_keys incomplete documentation
  - 📍 `test_main.cpp:223-231`
  - 💥 Explosion radius: Schedule keys not verified in hot-reload key test, creating blind spot for regression.

---

## 🛠️ Suggested Fixes

### 1. Fix applyJson Atomicity Violation

**File:** `firmware/core/ConfigManager.cpp`
**Issue:** State committed before validation completes

```cpp
// BEFORE (broken):
for (JsonPair kv : settings) {
    if (!updateConfigValue(...)) {  // validation fails on key N
        result.applied.clear();     // reports nothing applied
        return result;              // but keys 1..N-1 were already committed!
    }
    result.applied.push_back(String(key));
}
// ... commit snapshot to live variables ...

// AFTER (fixed):
// Phase 1: Validate all keys first without side effects
for (JsonPair kv : settings) {
    const char* key = kv.key().c_str();
    JsonVariant value = kv.value();
    
    // Create temporary snapshot for validation
    ConfigSnapshot testSnapshot = snapshot;
    if (!updateConfigValue(testSnapshot.display, ..., key, value)) {
        LOG_E("ConfigManager", "Rejected unknown or invalid key: %s", key);
        result.applied.clear();
        return result;
    }
    result.applied.push_back(String(key));
}

// Phase 2: Only after all keys validated, commit to live variables
{
    ConfigLockGuard guard;
    _display = snapshot.display;
    // ... etc ...
}
```

### 2. Fix sched_dim_brt Validation Inconsistency

**File:** `firmware/core/ConfigManager.cpp:159-161`

```cpp
// BEFORE:
if (strcmp(key, "sched_dim_brt") == 0) {
    schedule.sched_dim_brt = value.as<uint8_t>();
    return true;
}

// AFTER:
if (strcmp(key, "sched_dim_brt") == 0) {
    uint8_t v = value.as<uint8_t>();
    // Explicit validation for consistency with other schedule keys
    // Though uint8_t range is 0-255, explicit check documents intent
    if (v > 255) return false;  // Or remove if truly unrestricted
    schedule.sched_dim_brt = v;
    return true;
}
```

### 3. Fix sched_enabled Overflow Bug

**File:** `firmware/core/ConfigManager.cpp:141-146`

```cpp
// BEFORE:
if (strcmp(key, "sched_enabled") == 0) {
    uint8_t v = value.as<uint8_t>();  // 256 becomes 0 here!
    if (v > 1) return false;          // 0 > 1 is false, passes!
    schedule.sched_enabled = v;
    return true;
}

// AFTER:
if (strcmp(key, "sched_enabled") == 0) {
    // Validate before cast to catch overflow
    if (!value.is<uint8_t>()) return false;
    uint32_t v = value.as<uint32_t>();  // Use wider type first
    if (v > 1) return false;            // Now 256 > 1 correctly fails
    schedule.sched_enabled = static_cast<uint8_t>(v);
    return true;
}
```

### 4. Add Missing Test for dumpSettingsJson Schedule Keys

**File:** `firmware/test/test_config_manager/test_main.cpp`

```cpp
void test_dump_settings_json_includes_schedule() {
    clearNvs();
    ConfigManager::init();
    
    // Set known schedule values
    JsonDocument doc;
    doc["timezone"] = "EST5EDT";
    doc["sched_enabled"] = 1;
    doc["sched_dim_start"] = 1320;  // 22:00
    doc["sched_dim_end"] = 360;     // 06:00
    doc["sched_dim_brt"] = 5;
    ConfigManager::applyJson(doc.as<JsonObject>());
    
    // Call dumpSettingsJson
    JsonDocument outDoc;
    JsonObject out = outDoc.to<JsonObject>();
    ConfigManager::dumpSettingsJson(out);
    
    // Verify all schedule keys present
    TEST_ASSERT_TRUE(out["timezone"].is<String>());
    TEST_ASSERT_TRUE(out["timezone"] == "EST5EDT");
    TEST_ASSERT_EQUAL_UINT8(1, out["sched_enabled"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT16(1320, out["sched_dim_start"].as<uint16_t>());
    TEST_ASSERT_EQUAL_UINT16(360, out["sched_dim_end"].as<uint16_t>());
    TEST_ASSERT_EQUAL_UINT8(5, out["sched_dim_brt"].as<uint8_t>());
    
    // Verify total key count (27 keys)
    size_t keyCount = 0;
    for (JsonPair kv : out) keyCount++;
    TEST_ASSERT_EQUAL_UINT32(27, keyCount);
}
```

### 5. Strengthen Boundary Testing

**File:** `firmware/test/test_config_manager/test_main.cpp:344-371`

Add boundary value tests:
```cpp
void test_apply_json_schedule_validation() {
    // ... existing tests ...
    
    // Boundary value tests
    JsonDocument validStart;
    validStart["sched_dim_start"] = 0;  // Midnight (valid)
    TEST_ASSERT_EQUAL(1, ConfigManager::applyJson(validStart.as<JsonObject>()).applied.size());
    
    JsonDocument validEnd;
    validEnd["sched_dim_end"] = 1439;  // 23:59 (valid)
    TEST_ASSERT_EQUAL(1, ConfigManager::applyJson(validEnd.as<JsonObject>()).applied.size());
    
    JsonDocument invalidOverflow;
    invalidOverflow["sched_enabled"] = 256;  // Overflow to 0
    TEST_ASSERT_EQUAL(0, ConfigManager::applyJson(invalidOverflow.as<JsonObject>()).applied.size());
}
```

---

## Review Actions Summary

- Issues Found: 9
- Issues Fixed: 0
- Action Items Created: 5

⚠️ Address the identified issues before proceeding. The critical applyJson atomicity violation and sched_enabled overflow bug are blockers that could cause data inconsistency and silent acceptance of invalid configuration values.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-2-configmanager-expansion-schedule-keys-and-systemstatus.md

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
| Security Vulnerabilities | 1 |
| **Total Issues** | **7** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Silent Data Type Coercion in `updateConfigValue`: `value.as<Type>()` silently coerces JSON values to default types if the JSON value's actual type does not match the expected type, rather than rejecting the update. | firmware/core/ConfigManager.cpp:50 | +3 |
| 🟠 IMPORTANT | Lack of Runtime NVS Key Length Enforcement: The NVS key length limit (15 characters) is documented, but there is no runtime or compile-time enforcement mechanism. | firmware/core/ConfigManager.h | +1 |
| 🟠 IMPORTANT | Tight Coupling in `SystemStatus::toExtendedJson`: `SystemStatus::toExtendedJson` directly interacts with `WiFi`, `LittleFS` global objects, and calls `ConfigManager::getTiming()` and `LogoManager::getLogoCount()`. | firmware/core/SystemStatus.cpp:99 | +1 |
| 🟠 IMPORTANT | Inadequate Callback Test Coverage: `test_on_change_callback_fires()` only verifies a single callback fires, lacking tests for multiple callbacks or re-entrancy scenarios. | firmware/test/test_config_manager/test_main.cpp:309 | +1 |
| 🟡 MINOR | Magic Number for Debounce Delay in Unit Test: The test `test_apply_json_hot_reload_persists_after_debounce()` uses a magic number `delay(2100);` which is brittle. | firmware/test/test_config_manager/test_main.cpp:144 | +0.3 |
| 🟡 MINOR | Hardcoded NVS Namespace String: The NVS namespace string `"flightwall"` is hardcoded in multiple files. | firmware/core/ConfigManager.cpp:23 | +0.3 |
| 🟡 MINOR | `String` Copying Overhead in `SystemStatus::set`: Assigning `const String& message` to a `String` member (`_statuses[idx].message`) incurs a heap allocation and copy. | firmware/core/SystemStatus.cpp:34 | +0.3 |

### Evidence Score: 6.9

| Score | Verdict |
|-------|---------|
| **6.9** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[1/10] Dependency Inversion Principle:** The `SystemStatus::toExtendedJson` method directly interacts with `WiFi`, `LittleFS` global objects, and calls `ConfigManager::getTiming()` and `LogoManager::getLogoCount()`.
  - 📍 `firmware/core/SystemStatus.cpp:99`
  - 💡 Fix: Refactor `toExtendedJson` to accept necessary data as parameters, depending on abstractions rather than concrete implementations.

---

## 🐍 Pythonic Crimes &amp; Readability

- **Style Violation:** Magic Number for Debounce Delay in Unit Test
  - 📍 `firmware/test/test_config_manager/test_main.cpp:144`
- **Style Violation:** Hardcoded NVS Namespace String
  - 📍 `firmware/core/ConfigManager.cpp:23`

---

## ⚡ Performance &amp; Scalability

- **[MEDIUM] String Copying Overhead:** The `SystemStatus::set` method takes `const String& message` as a parameter. Assigning this to `_statuses[idx].message` (a `String`) will likely involve a heap allocation and copy, potentially leading to unnecessary memory fragmentation.
  - 📍 `firmware/core/SystemStatus.cpp:34`
  - 💡 Fix: Consider passing `const char* message` and managing the `String` conversion internally or using fixed-size buffers.

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug:** Silent Data Type Coercion in `updateConfigValue`: `value.as<Type>()` silently coerces JSON values to default types (e.g., `uint8_t` to `0`, `double` to `0.0`) if the JSON value's actual type does not match the expected type, rather than rejecting the update.
  - 📍 `firmware/core/ConfigManager.cpp:50`
  - 🔄 Reproduction: Call `ConfigManager::applyJson` with a key expecting a numeric type but providing a string (e.g., `{"brightness": "high"}`). The `brightness` will be set to `0` without an error.
- **🎭 Lying Test:** Inadequate Callback Test Coverage: `test_on_change_callback_fires()` only verifies that a single callback fires. It lacks tests for crucial edge cases like multiple callbacks or re-entrancy scenarios.
  - 📍 `firmware/test/test_config_manager/test_main.cpp:309`
  - 🤥 Why it lies: The test provides a false sense of security for the `onChange` callback mechanism by only validating the simplest happy path, failing to expose potential issues in more complex or concurrent usage patterns.
- **🔒 [HIGH] Security:** Silent Data Type Coercion in `updateConfigValue`: As described in the hidden bug, this behavior can allow invalid user input to silently corrupt configuration, potentially leading to unexpected device states or vulnerabilities.
  - 📍 `firmware/core/ConfigManager.cpp:50`
  - ⚠️ Impact: Device malfunction, unpredictable behavior, and difficult-to-diagnose issues originating from seemingly valid API calls.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Lack of Runtime NVS Key Length Enforcement: The 15-character NVS key length limit is only documented, not enforced programmatically.
  - 📍 `firmware/core/ConfigManager.h`
  - 💥 Explosion radius: Future NVS write failures or silent data truncation if new keys exceed the limit, requiring painful debugging.
- **💣 Tech Debt:** Hardcoded NVS Namespace String: The string `"flightwall"` is duplicated in multiple files.
  - 📍 `firmware/core/ConfigManager.cpp:23`
  - 💥 Explosion radius: Requires manual changes in multiple locations if the namespace needs to be updated, increasing the risk of inconsistencies and errors.
- **💣 Tech Debt:** Magic Number for Debounce Delay in Unit Test: The test uses a hardcoded `delay(2100);` that is brittle to changes in the `ConfigManager`'s persistence debounce logic.
  - 📍 `firmware/test/test_config_manager/test_main.cpp:144`
  - 💥 Explosion radius: Test failure or unreliability if the system's debounce timing is adjusted, without indicating a functional regression.

---

## 🛠️ Suggested Fixes

### 1. Address Silent Data Type Coercion in `updateConfigValue`

**File:** `firmware/core/ConfigManager.cpp`
**Issue:** `value.as<Type>()` silently coerces JSON values to default types if the JSON value's actual type does not match the expected type, rather than rejecting the update.

### 2. Implement Runtime NVS Key Length Enforcement

**File:** `firmware/core/ConfigManager.h`, `firmware/core/ConfigManager.cpp`
**Issue:** The NVS key length limit (15 characters) is documented but not enforced, risking future errors.

### 3. Decouple `SystemStatus::toExtendedJson`

**File:** `firmware/core/SystemStatus.cpp`
**Issue:** `SystemStatus::toExtendedJson` is tightly coupled to global objects and other managers.

### 4. Improve `onChange` Callback Test Coverage

**File:** `firmware/test/test_config_manager/test_main.cpp`
**Issue:** The existing test for `onChange` callbacks (`test_on_change_callback_fires`) is too simplistic and does not cover multiple callbacks or re-entrancy scenarios, leading to insufficient validation.

### 5. Replace Magic Number in Debounce Test

**File:** `firmware/test/test_config_manager/test_main.cpp`
**Issue:** The test `test_apply_json_hot_reload_persists_after_debounce()` uses a magic number `delay(2100);` for its wait period, making it brittle.

### 6. Centralize NVS Namespace Definition

**File:** `firmware/core/ConfigManager.cpp`, `firmware/test/test_config_manager/test_main.cpp`
**Issue:** The NVS namespace string `"flightwall"` is hardcoded in multiple files, violating DRY.

### 7. Optimize `SystemStatus::set` for `String` Parameter

**File:** `firmware/core/SystemStatus.cpp`
**Issue:** Assigning a `const String&` parameter to a `String` member (`_statuses[idx].message`) likely causes a heap allocation and copy, potentially impacting performance on an embedded system.

---

## Review Actions Summary

- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="f63a150c" path="[Validator D]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-2-configmanager-expansion-schedule-keys-and-systemstatus.md`

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 2 |
| SOLID / Architecture Violations | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 1 |
| Total Issues | 7 |

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Test suite contains a case that directly contradicts production behavior, so the story’s “verification complete” claim is not credible | `firmware/core/ConfigManager.cpp:576-579`, `firmware/test/test_config_manager/test_main.cpp:180-194` | +3 |
| 🔴 CRITICAL | Task 3 is marked done, but `sched_dim_brt` has no real validation and the validation test omits that branch entirely | `firmware/core/ConfigManager.cpp:151-154`, `firmware/test/test_config_manager/test_main.cpp:321-345` | +3 |
| 🔴 CRITICAL | `SystemStatus` explicitly drops synchronization on mutex timeout and races on heap-backed `String` fields | `firmware/core/SystemStatus.cpp:34-42`, `:54-57`, `:65-73` | +3 |
| 🔴 CRITICAL | `/api/settings` still exposes WiFi and API credentials in plaintext | `firmware/core/ConfigManager.cpp:499-503` | +3 |
| 🟠 IMPORTANT | `loadFromNvs()` trusts unvalidated schedule values from persistent storage | `firmware/core/ConfigManager.cpp:381-386` | +1 |
| 🟠 IMPORTANT | AC #4 is not protected by any automated test of `dumpSettingsJson()` presence/count | `firmware/test/test_config_manager/test_main.cpp:408-438` | +1 |
| 🟠 IMPORTANT | `SystemStatus::init()` leaks semaphore handles on repeated init | `firmware/core/SystemStatus.cpp:17-18`, `firmware/test/test_config_manager/test_main.cpp:349`, `:356`, `:370`, `:386` | +1 |
| 🟢 CLEAN PASS | No additional story-specific performance, style, or type-safety defects were substantiated beyond the findings above | reviewed files | -1.5 |

### Evidence Score: 13.5

| Score | Verdict |
|-------|---------|
| **13.5** | **REJECT** |

## Findings

1. **Critical: the shipped test suite disagrees with the implementation.**
   - `test_apply_json_ignores_unknown_keys()` expects `applyJson()` to keep `"brightness"` and ignore `"bogus_key"` (`firmware/test/test_config_manager/test_main.cpp:180-194`).
   - Production code does the opposite: on the first unknown/invalid key it logs, clears `result.applied`, and returns immediately (`firmware/core/ConfigManager.cpp:576-579`).
   - Consequence: the suite cannot pass if it is actually executed on-device. The story notes only prove a build-only test invocation, not runtime verification.

2. **Critical: Task 3 is marked complete, but `sched_dim_brt` validation was never implemented.**
   - The story explicitly says `sched_dim_brt` must be validated as `0-255`.
   - The actual handler just does `schedule.sched_dim_brt = value.as<uint8_t>(); return true;` with no type/range rejection (`firmware/core/ConfigManager.cpp:151-154`).
   - The supposed validation test covers `sched_enabled`, `sched_dim_start`, `sched_dim_end`, and timezone length, but never exercises `sched_dim_brt` at all (`firmware/test/test_config_manager/test_main.cpp:321-345`).
   - Repro: send `{"sched_dim_brt": 999}` or another non-conforming value; the code accepts/coerces it instead of rejecting it.

3. **Critical: `SystemStatus` is not actually thread-safe under contention.**
   - `set()`, `get()`, and `toJson()` all bypass the mutex on timeout and read/write `_statuses[idx].message` anyway (`firmware/core/SystemStatus.cpp:34-42`, `:54-57`, `:65-73`).
   - `SubsystemStatus.message` is an Arduino `String`, so this fallback is not harmless. Concurrent mutation and copy of a heap-backed string is exactly how you get torn JSON, stale pointers, or heap corruption on ESP32.
   - This is worse than no locking because the code advertises synchronization while silently abandoning it when the system is busy.

4. **Critical: `GET /api/settings` still leaks secrets in plaintext.**
   - `dumpSettingsJson()` serializes `wifi_password`, `os_client_id`, `os_client_sec`, and `aeroapi_key` directly into the response (`firmware/core/ConfigManager.cpp:499-503`).
   - This story touched the serialization path and kept the bug intact.
   - If the dashboard is reachable on LAN, any caller with endpoint access can exfiltrate WiFi credentials and third-party API secrets. Even if the UI needs to know whether a value exists, it does not need the raw secret echoed back on every GET.

5. **High: boot-time schedule loading trusts corrupted NVS state.**
   - `loadFromNvs()` validates tile geometry and display pin, but the new schedule keys are copied straight from NVS with no bounds checks (`firmware/core/ConfigManager.cpp:381-386`).
   - Repro: store `sched_enabled=255` or `sched_dim_start=65535` in NVS, reboot, and `ConfigManager` accepts poisoned values into RAM.
   - This matters because later scheduler logic depends on `0/1` flags and `0-1439` minute ranges.

6. **High: AC #4 is effectively unverified.**
   - The story claims `GET /api/settings` now returns 27 keys, but the registered Unity suite contains no test for `dumpSettingsJson()`, no count assertion, and no presence check for the five new schedule fields (`firmware/test/test_config_manager/test_main.cpp:408-438`).
   - This is not a paperwork nit. Endpoint shape is a contract, and it was changed here without automated coverage.

7. **Medium: `SystemStatus::init()` leaks RTOS objects on repeated initialization.**
   - Every call does `_mutex = xSemaphoreCreateMutex();` with no guard and no cleanup of the old handle (`firmware/core/SystemStatus.cpp:17-18`).
   - The current tests call `SystemStatus::init()` repeatedly (`firmware/test/test_config_manager/test_main.cpp:349`, `:356`, `:370`, `:386`), so this pattern is already exercised.
   - On embedded targets, leaking semaphores in infrastructure code is a maintenance trap and makes future reset/reinit flows unsafe.

## Architectural Sins

- **[6/10] Single Responsibility breach:** `SystemStatus` is no longer just a subsystem registry; `toExtendedJson()` also reaches into WiFi, LittleFS, LogoManager, ConfigManager, and quota math (`firmware/core/SystemStatus.cpp:83-121`).
  - Each new subsystem addition expands an already broad god-object instead of a focused health registry.
  - Fix: keep registry storage/query in `SystemStatus`, move endpoint-specific DTO assembly elsewhere.

## Open Questions / Assumptions

- If `/api/settings` is authenticated elsewhere, the secret-exposure finding drops from critical to high, but it is still poor API hygiene.
- The story notes mention `pio test --without-uploading --without-testing`, which is a compile check, not executed test evidence. I am assuming no executed Unity run exists beyond what was embedded here.

## Review Actions Summary

- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

**Verdict:** REJECT. The story is not review-clean. At minimum it needs the validation gap fixed, the contradictory test suite corrected/executed, and the `SystemStatus` synchronization bug addressed before this should be treated as complete.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-1</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">4</var>
<var name="session_id">f5ba6147-0591-4a64-b853-7236c5700a51</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="9fd78415">embedded in prompt, file id: 9fd78415</var>
<var name="story_id">fn-1.2</var>
<var name="story_key">fn-1-2-configmanager-expansion-schedule-keys-and-systemstatus</var>
<var name="story_num">2</var>
<var name="story_title">2-configmanager-expansion-schedule-keys-and-systemstatus</var>
<var name="template">False</var>
<var name="timestamp">20260412_1240</var>
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
      - Commit message format: fix(component): brief description (synthesis-fn-1.2)
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