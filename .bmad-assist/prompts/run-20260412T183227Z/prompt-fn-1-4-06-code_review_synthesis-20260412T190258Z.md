<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 4 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260412T190258Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-1.4

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
<file id="e97afb18" path="_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md" label="DOCUMENTATION"><![CDATA[


# Story fn-1.4: OTA Self-Check & Rollback Detection

Status: ready-for-dev

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a **user**,
I want **the device to verify new firmware works after an OTA update and automatically roll back if it doesn't**,
So that **a bad firmware update never bricks my device**.

## Acceptance Criteria

1. **Given** the device boots after an OTA update **When** WiFi connects successfully **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called **And** the partition is marked valid **And** a log message records the time in milliseconds (e.g., `"Firmware marked valid — WiFi connected in 8432ms"`) **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Firmware verified — WiFi connected in Xs")` is recorded (X = elapsed seconds)

2. **Given** the device boots after an OTA update and WiFi does not connect **When** 60 seconds elapse since boot **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called on timeout **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Marked valid on timeout — WiFi not verified")` is recorded **And** a WARNING log message is emitted

3. **Given** the device boots after an OTA update and crashes before the self-check completes **When** the watchdog fires and the device reboots **Then** the bootloader detects the active partition was never marked valid **And** the bootloader rolls back to the previous partition automatically (this is ESP32 bootloader behavior, not application code)

4. **Given** a rollback has occurred **When** `esp_ota_get_last_invalid_partition()` returns non-NULL during boot **Then** a `rollbackDetected` flag is set to `true` **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Firmware rolled back to previous version")` is recorded **And** the invalid partition label is logged (e.g., `"Rollback detected: partition 'app1' was invalid"`)

5. **Given** `GET /api/status` is called **When** the response is built **Then** it includes `"firmware_version": "1.0.0"` (from `FW_VERSION` build flag) **And** `"rollback_detected": true/false` in the response data object

6. **Given** normal boot (not after OTA) **When** the self-check runs **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called (idempotent — safe on non-OTA boots, returns ESP_OK with no effect when image is already valid) **And** no `SystemStatus::set` call is made for the OTA subsystem (no OK, WARNING, or ERROR status is emitted)

## Tasks / Subtasks

- [ ] **Task 1: Add rollback detection at early boot** (AC: #4)
  - [ ] In `setup()`, immediately after `Serial.begin()` and firmware version log (line ~458), before `validatePartitionLayout()`:
  - [ ] Call `esp_ota_get_last_invalid_partition()` — if non-NULL, set a file-scope `static bool g_rollbackDetected = true`
  - [ ] Log: `Serial.printf("[OTA] Rollback detected: partition '%s' was invalid\n", invalid->label)`
  - [ ] Note: SystemStatus is not initialized yet at this point — defer the `SystemStatus::set()` call to Task 3

- [ ] **Task 2: Implement OTA self-check function** (AC: #1, #2, #6)
  - [ ] Create a file-scope function `void performOtaSelfCheck()` in main.cpp
  - [ ] Record `unsigned long bootStartMs = millis()` at top of setup() (before any delays) for accurate boot timing
  - [ ] Function checks WiFi state via `g_wifiManager.getState() == WiFiState::STA_CONNECTED`
  - [ ] **If WiFi connected:** call `esp_ota_mark_app_valid_cancel_rollback()`, log time: `"Firmware marked valid — WiFi connected in %lums"`, set SystemStatus OK
  - [ ] **If timeout (60s) reached:** call `esp_ota_mark_app_valid_cancel_rollback()`, log WARNING: `"Firmware marked valid on timeout — WiFi not verified"`, set SystemStatus WARNING
  - [ ] **If normal boot (not pending verify):** call `esp_ota_mark_app_valid_cancel_rollback()` anyway (idempotent, no status change needed)
  - [ ] Use `esp_ota_get_state_partition()` on running partition to check if `ESP_OTA_IMG_PENDING_VERIFY` — only log detailed timing when pending

- [ ] **Task 3: Integrate self-check into loop()** (AC: #1, #2, #4, #6)
  - [ ] Add file-scope state: `static bool g_otaSelfCheckDone = false` and `static unsigned long g_bootStartMs = 0`
  - [ ] Set `g_bootStartMs = millis()` at top of `setup()`
  - [ ] At the start of `loop()`, after `ConfigManager::tick()` and `g_wifiManager.tick()`:
    - If `!g_otaSelfCheckDone`: call `performOtaSelfCheck()` which checks WiFi or timeout
    - When check completes (WiFi connected OR 60s elapsed): set `g_otaSelfCheckDone = true`
  - [ ] Also in the self-check block: if `g_rollbackDetected` and SystemStatus is initialized, call `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Firmware rolled back to previous version")` (deferred from Task 1)

- [ ] **Task 4: Update GET /api/status response** (AC: #5)
  - [ ] In `SystemStatus::toExtendedJson()` (SystemStatus.cpp, line ~83), add to the root `obj`:
    - `obj["firmware_version"] = FW_VERSION;`
    - `obj["rollback_detected"] = <rollback flag>;`
  - [ ] Expose the rollback flag: either add `static bool getRollbackDetected()` to SystemStatus, or pass it from main.cpp via the existing `FlightStatsSnapshot` struct (preferred: add `bool rollback_detected` field to `FlightStatsSnapshot`)
  - [ ] Update `getFlightStatsSnapshot()` in main.cpp to include `g_rollbackDetected`

- [ ] **Task 5: Build and verify** (AC: #1-#6)
  - [ ] Run `~/.platformio/penv/bin/pio run` from `firmware/` — verify clean build with no errors
  - [ ] Verify binary size remains under 1.5MB limit
  - [ ] Log expected output for normal boot: `"[OTA] Firmware marked valid (normal boot)"` or similar minimal message
  - [ ] Log expected output for post-OTA boot: `"[OTA] Firmware marked valid — WiFi connected in XXXXms"`
  - [ ] Verify `GET /api/status` response includes `firmware_version` and `rollback_detected` fields

## Dev Notes

### Critical Architecture Constraints

**Architecture Decision F3: WiFi-OR-Timeout Strategy (HARD REQUIREMENT)**
The self-check marks firmware valid when EITHER WiFi connects OR 60s timeout expires — whichever comes first. No self-HTTP-request. No web server check. This is the simplest strategy that catches crash-on-boot failures.

**Timeout budget:**
| Scenario | Expected Duration |
|----------|------------------|
| Good firmware + WiFi available | 5-15s (WiFi connects) |
| Good firmware + no WiFi | 60s (timeout fallback) |
| Bad firmware (crash on boot) | Watchdog rollback (never reaches self-check) |

**ESP32 Rollback Mechanism (bootloader-level, not application code):**
1. After OTA write + `Update.end(true)`, new partition state = `ESP_OTA_IMG_NEW`
2. On reboot, bootloader transitions to `ESP_OTA_IMG_PENDING_VERIFY` and boots new partition
3. Application calls `esp_ota_mark_app_valid_cancel_rollback()` → state = `ESP_OTA_IMG_VALID`
4. If application crashes before marking valid → bootloader sets `ESP_OTA_IMG_ABORTED` on next boot and loads previous partition

**Arduino-ESP32 rollback is ENABLED by default** — `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` is baked into the prebuilt bootloader. No sdkconfig or build flag changes needed.

**Idempotent behavior:** `esp_ota_mark_app_valid_cancel_rollback()` returns `ESP_OK` with no effect when the running partition is already in `ESP_OTA_IMG_VALID` state. Safe to call on every boot.

### ESP32 OTA API Reference

All functions from `#include "esp_ota_ops.h"` (already included in main.cpp line 21):

```cpp
// Get last partition that failed validation (rollback detection)
const esp_partition_t* esp_ota_get_last_invalid_partition(void);
// Returns non-NULL if a rollback occurred; the returned partition was the failed one

// Mark current partition as valid, cancel pending rollback
esp_err_t esp_ota_mark_app_valid_cancel_rollback(void);
// Returns ESP_OK; idempotent if already valid

// Get OTA image state of a partition
esp_err_t esp_ota_get_state_partition(const esp_partition_t* partition, esp_ota_img_states_t* ota_state);
// States: ESP_OTA_IMG_NEW, ESP_OTA_IMG_PENDING_VERIFY, ESP_OTA_IMG_VALID, ESP_OTA_IMG_INVALID, ESP_OTA_IMG_ABORTED, ESP_OTA_IMG_UNDEFINED

// Get currently running partition (already used in validatePartitionLayout())
const esp_partition_t* esp_ota_get_running_partition(void);
```

### Implementation Pattern

```cpp
// File-scope globals in main.cpp
static bool g_rollbackDetected = false;
static bool g_otaSelfCheckDone = false;
static unsigned long g_bootStartMs = 0;

// Called once early in setup(), before SystemStatus::init()
void detectRollback() {
    const esp_partition_t* invalid = esp_ota_get_last_invalid_partition();
    if (invalid != NULL) {
        g_rollbackDetected = true;
        Serial.printf("[OTA] Rollback detected: partition '%s' was invalid\n", invalid->label);
    }
}

// Called from loop() until complete
void performOtaSelfCheck() {
    if (g_otaSelfCheckDone) return;

    // Check if we're in pending-verify state (post-OTA boot)
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    bool isPendingVerify = false;
    if (running && esp_ota_get_state_partition(running, &state) == ESP_OK) {
        isPendingVerify = (state == ESP_OTA_IMG_PENDING_VERIFY);
    }

    unsigned long elapsed = millis() - g_bootStartMs;
    bool wifiConnected = (g_wifiManager.getState() == WiFiState::STA_CONNECTED);

    if (wifiConnected || elapsed >= 60000) {
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        if (err == ESP_OK) {
            if (isPendingVerify) {
                if (wifiConnected) {
                    LOG_I("OTA", "Firmware marked valid — WiFi connected in %lums", elapsed);
                    SystemStatus::set(Subsystem::OTA, StatusLevel::OK,
                        ("Firmware verified — WiFi connected in " + String(elapsed / 1000) + "s").c_str());
                } else {
                    LOG_I("OTA", "Firmware marked valid on timeout — WiFi not verified");
                    SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
                        "Marked valid on timeout — WiFi not verified");
                }
            } else {
                LOG_V("OTA", "Self-check: already valid (normal boot)");
            }
        }

        // Deferred rollback status (SystemStatus wasn't ready in setup())
        if (g_rollbackDetected) {
            SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
                "Firmware rolled back to previous version");
        }

        g_otaSelfCheckDone = true;
    }
}
```

### Placement in main.cpp

**In `setup()` (line ~458 area, after Serial.begin and FW_VERSION log, BEFORE validatePartitionLayout):**
```cpp
g_bootStartMs = millis();  // Record boot time for self-check timing
detectRollback();          // Check for rollback before anything else
```

**In `loop()` (line ~701 area, after ConfigManager::tick() and g_wifiManager.tick()):**
```cpp
if (!g_otaSelfCheckDone) {
    performOtaSelfCheck();
}
```

### FlightStatsSnapshot Extension

Add to the existing struct in main.cpp (around line 100-110 area):
```cpp
// In the FlightStatsSnapshot struct definition (or wherever it's defined)
bool rollback_detected;  // Set from g_rollbackDetected
```

Update `getFlightStatsSnapshot()` to include:
```cpp
snapshot.rollback_detected = g_rollbackDetected;
```

### SystemStatus::toExtendedJson() Extension

Add to the function in SystemStatus.cpp (line ~107 area, in the `device` section or as top-level fields):
```cpp
obj["firmware_version"] = FW_VERSION;
obj["rollback_detected"] = stats.rollback_detected;
```

Note: `FW_VERSION` is a compile-time build flag macro set in `platformio.ini`. Add the conditional guard below at the top of `SystemStatus.cpp` — it resolves to nothing in normal builds where the flag is defined, and provides a safe fallback if the file is ever compiled outside PlatformIO (e.g., in a unit test harness):
```cpp
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"
#endif
```

### Critical Gotchas

1. **No factory partition** — The partition table has only app0 and app1 (no factory). If BOTH partitions become invalid, the device is bricked. However, this can only happen if the user OTA-updates with bad firmware AND the rollback target is also bad — extremely unlikely since the rollback target was previously running.

2. **Rollback detection persists until next successful OTA** — `esp_ota_get_last_invalid_partition()` returns the last invalid partition persistently (survives reboots). `g_rollbackDetected` will therefore be `true` on every subsequent boot until a new successful OTA upload overwrites the invalid partition slot. This means `/api/status` will return `"rollback_detected": true` on every boot until that happens — this is correct and intentional API behavior. The consumer (fn-1.6 dashboard) should display this state prominently until cleared by a successful OTA.

3. **Self-check runs on Core 1** — `loop()` runs on Core 1, which is correct. WiFiManager's WiFi stack also runs on Core 1. No cross-core issues.

4. **Order matters** — `detectRollback()` must run BEFORE `SystemStatus::init()` because SystemStatus isn't ready yet. The actual `SystemStatus::set()` call is deferred to `loop()` when both SystemStatus and the self-check are ready.

5. **Don't block setup()** — The 60-second timeout MUST be checked in `loop()`, not via `delay(60000)` in `setup()`. The architecture requires non-blocking operation.

### Response Format for GET /api/status

The existing response structure (from `toExtendedJson`) adds these new top-level fields:
```json
{
  "ok": true,
  "data": {
    "firmware_version": "1.0.0",
    "rollback_detected": false,
    "subsystems": { ... },
    "wifi_detail": { ... },
    "device": { ... },
    "flight": { ... },
    "quota": { ... }
  }
}
```

### Project Structure Notes

**Files to modify:**
1. `firmware/src/main.cpp` — Add rollback detection, self-check function, loop integration, boot timestamp
2. `firmware/core/SystemStatus.cpp` — Add `firmware_version` and `rollback_detected` to toExtendedJson()
3. `firmware/src/main.cpp` (FlightStatsSnapshot struct) — Add `rollback_detected` field

**Files NOT to modify:**
- `firmware/adapters/WebPortal.cpp` — No endpoint changes needed; `/api/status` handler already calls `toExtendedJson()`
- `firmware/core/ConfigManager.cpp` — No new NVS keys needed; rollback state comes from ESP32 bootloader, not NVS
- `firmware/core/SystemStatus.h` — OTA subsystem already exists (added in fn-1.2)
- `firmware/custom_partitions.csv` — Already has dual-OTA layout (fn-1.1)

**Code location guidance:**
- New globals (`g_rollbackDetected`, `g_otaSelfCheckDone`, `g_bootStartMs`) at file scope in main.cpp, near existing globals (line ~60-100)
- `detectRollback()` function near `validatePartitionLayout()` (line ~407 area)
- `performOtaSelfCheck()` function after `detectRollback()` 
- Loop integration after line 702 (`g_wifiManager.tick()`)

### References

- [Source: architecture.md#Decision F3: OTA Self-Check — WiFi-OR-Timeout Strategy]
- [Source: architecture.md#Decision F2: OTA Handler Architecture — WebPortal + main.cpp]
- [Source: epic-fn-1.md#Story fn-1.4: OTA Self-Check & Rollback Detection]
- [Source: prd.md#Update Mechanism — self-check, rollback]
- [Source: prd.md#Reliability NFR — automatic recovery from OTA failures]
- [Source: ESP-IDF OTA Documentation — esp_ota_ops.h API reference]
- [Source: main.cpp lines 409-445 — validatePartitionLayout() for existing esp_ota_ops usage pattern]
- [Source: main.cpp line 21 — esp_ota_ops.h already included]
- [Source: SystemStatus.cpp lines 83-132 — toExtendedJson() for /api/status response]

### Dependencies

**This story depends on:**
- fn-1.1 (Partition Table & Build Configuration) — COMPLETE (dual-OTA partition layout + FW_VERSION)
- fn-1.2 (ConfigManager Expansion) — COMPLETE (SystemStatus OTA subsystem ready)
- fn-1.3 (OTA Upload Endpoint) — COMPLETE (writes firmware to flash, triggers reboot)

**Stories that depend on this:**
- fn-1.6: Dashboard Firmware Card & OTA Upload UI — displays `firmware_version` and `rollback_detected` from `/api/status`

### Previous Story Intelligence

**From fn-1.1:** Binary size 1.15MB (76.8%). Partition table validated with `validatePartitionLayout()`. `esp_ota_ops.h` already included.

**From fn-1.2:** SystemStatus expanded to 8 subsystems. OTA subsystem (`Subsystem::OTA`) ready. Binary size 1.21MB (76.9%).

**From fn-1.3:** OTA upload endpoint complete. After successful upload, `Update.end(true)` marks new partition bootable and `ESP.restart()` is called after 500ms delay. Binary size 1.22MB (77.4%). Critical lesson: `state->started = false` after `Update.end(true)` to prevent cleanup from aborting a completed update.

**Antipatterns from previous stories to avoid:**
- Don't add fields to `/api/status` response without verifying the struct that carries the data (fn-1.3 had missing SystemStatus calls)
- Don't defer logging — log immediately when detecting rollback, even if SystemStatus isn't ready yet (use Serial.printf)
- Ensure all code paths set appropriate SystemStatus levels (fn-1.3 had missing ERROR paths)

### Testing Strategy

**Build verification:**
```bash
cd firmware && ~/.platformio/penv/bin/pio run
# Expect: SUCCESS, binary < 1.5MB
```

**Manual test cases:**

1. **Normal boot (no OTA):** Power cycle the device
   - Expect: `[OTA] Self-check: already valid (normal boot)` in Serial log (verbose level)
   - Expect: No OTA WARNING or ERROR in SystemStatus
   - Expect: `/api/status` returns `rollback_detected: false`

2. **Post-OTA boot with WiFi:** Upload firmware via `/api/ota/upload`, device reboots
   - Expect: `[OTA] Firmware marked valid — WiFi connected in XXXXms` in Serial log
   - Expect: SystemStatus OTA = OK with timing message
   - Expect: `/api/status` returns `firmware_version: "1.0.0"`, `rollback_detected: false`

3. **Post-OTA boot without WiFi:** Upload firmware, then disconnect WiFi router before reboot
   - Expect: After 60s, `[OTA] Firmware marked valid on timeout — WiFi not verified`
   - Expect: SystemStatus OTA = WARNING

4. **Rollback test (if possible):** Upload intentionally broken firmware
   - Expect: Device crashes, reboots, runs previous firmware
   - Expect: `[OTA] Rollback detected: partition 'appX' was invalid`
   - Expect: `/api/status` returns `rollback_detected: true`
   - Note: Creating intentionally broken firmware that boots far enough to trigger watchdog but not self-check is tricky — this is primarily validated by code review of the ESP32 bootloader mechanism

5. **API response verification:** Call `GET /api/status` after any boot
   - Expect: `firmware_version` field present with correct version string
   - Expect: `rollback_detected` field present as boolean

### Risk Mitigation

1. **esp_ota_get_last_invalid_partition persistence:** The invalid partition state persists across normal reboots. Verify that the flag doesn't create a permanent "rollback detected" state that never clears. The flag clears when a new successful OTA occurs (overwrites the invalid partition with good firmware).

2. **Timing edge case:** If WiFi connects at exactly 60s, both the WiFi-connected and timeout paths could trigger. Guard with `g_otaSelfCheckDone` flag — first check that completes wins.

3. **Heap impact:** This story adds ~200 bytes of stack for the self-check function and 3 file-scope variables (~10 bytes). Negligible impact on the ~280KB RAM budget.

## Dev Agent Record

### Agent Model Used

Claude Opus 4 (Story Creation)

### Debug Log References

N/A — Story creation phase

### Completion Notes List

**2026-04-12: Ultimate context engine analysis completed**
- Comprehensive analysis of epic-fn-1.md extracted all 6 acceptance criteria with BDD format
- Architecture Decision F3 (WiFi-OR-Timeout) mapped as hard requirement
- ESP32 OTA rollback API thoroughly researched: esp_ota_mark_app_valid_cancel_rollback(), esp_ota_get_last_invalid_partition(), esp_ota_get_state_partition()
- Arduino-ESP32 confirmed: CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y is default in prebuilt bootloader
- main.cpp analyzed: setup() initialization order, loop() structure, existing esp_ota_ops.h include, validatePartitionLayout() pattern
- SystemStatus.cpp analyzed: toExtendedJson() extension point identified at line ~107
- FlightStatsSnapshot struct identified as transport for rollback_detected flag to SystemStatus
- All 3 prerequisite stories (fn-1.1, fn-1.2, fn-1.3) completion notes reviewed for binary size, patterns, and lessons learned
- 5 tasks created with explicit code references, line numbers, and implementation patterns
- Antipatterns from fn-1.3 code review incorporated: consistent SystemStatus coverage, proper error path handling

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |

### File List

- `firmware/src/main.cpp` (MODIFIED) — Rollback detection, self-check function, loop integration, boot timestamp, FlightStatsSnapshot extension
- `firmware/core/SystemStatus.cpp` (MODIFIED) — Add firmware_version and rollback_detected to toExtendedJson()


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

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Integer Overflow in `sched_enabled` Validation | Changed from `value.as<uint8_t>()` (wraps 256→0) to validating as `unsigned int` before cast. Now properly rejects values >1. Added type check `value.is<unsigned int>()`. |
| critical | Missing Validation for `sched_dim_brt` | Added bounds checking (0-255) and type validation. Previously accepted any value without validation, violating story requirements. |
| critical | Test Suite Contradicts Implementation | Renamed `test_apply_json_ignores_unknown_keys` → `test_apply_json_rejects_unknown_keys` and corrected assertions. applyJson uses all-or-nothing validation, not partial success. |
| high | Missing Test Coverage for AC #4 | Added `test_dump_settings_json_includes_schedule()` — verifies all 5 schedule keys present in JSON and total key count = 27. |
| high | Validation Test Coverage Gaps | Extended `test_apply_json_schedule_validation()` with 2 additional test cases: `sched_dim_brt > 255` rejection and `sched_enabled = 256` overflow rejection. |
| dismissed | `/api/settings` exposes secrets (wifi_password, API keys) in plaintext | FALSE POSITIVE: Pre-existing design, not introduced by this story. Requires separate security story to implement credential masking. Story scope was schedule keys + SystemStatus subsystems only. |
| dismissed | ConfigSnapshot heap allocation overhead in applyJson | FALSE POSITIVE: Necessary for atomic semantics — applyJson must validate all keys before committing any changes. Snapshot pattern is intentional design. |
| dismissed | SystemStatus mutex timeout fallback is unsafe | FALSE POSITIVE: Pre-existing pattern across SystemStatus implementation (lines 35-44, 53-58, 65-73). Requires broader refactor outside story scope. This story only added OTA/NTP subsystems. |
| dismissed | SystemStatus tight coupling to WiFi, LittleFS, ConfigManager | FALSE POSITIVE: Pre-existing architecture in `toExtendedJson()` method. Not introduced by this story — story only added 2 subsystems to existing enum. |
| dismissed | Hardcoded NVS namespace string | FALSE POSITIVE: Pre-existing pattern, not story scope. NVS namespace was defined before this story. |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Test baseline claims 27 keys but implementation has 29 | Updated test assertion from 27 to 29 to match actual implementation |
| critical | Missing negative number validation for schedule time fields | Added type validation with `value.is<int>()` and signed bounds checking before unsigned cast |
| critical | Missing type validation for timezone string | Added `value.is<const char*>()` type check before string conversion |
| high | Missing test coverage for AC #4 - JSON dump includes all schedule keys | Corrected key count assertion from 27 to 29 |
| high | Validation test coverage gaps | Already present in current code (lines 373-382) - FALSE POSITIVE on reviewer's part, but validation logic in implementation was incomplete (see fixes #6, #7) |
| high | Integer overflow in sched_enabled validation | Already has type check `value.is<unsigned int>()` - validation is correct. Reviewing again...actually the test exists to verify 256 is rejected (line 380). This is working correctly. |
| high | Missing validation for sched_dim_brt | Code already has type check and >255 rejection (lines 163-167) after Round 2 fixes. Verified correct. |
| medium | Test suite contradicts implementation (unknown key handling) | Test already renamed to `test_apply_json_rejects_unknown_keys` with correct assertions after Round 2 fixes |

## Story fn-1-3 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `clearOTAUpload()` unconditionally calls `Update.abort()` when `started == true`, including the success path where `clearOTAUpload()` is called immediately after `Update.end(true)` — aborting the just-written firmware | Set `state->started = false` after successful `Update.end(true)` so the completed-update flag is cleared before cleanup runs |
| high | No concurrent upload guard; a second client can call `Update.begin()` while an upload is in-flight, sharing the non-reentrant `Update` singleton and risking flash corruption | Added `g_otaInProgress` static bool; second upload receives `OTA_BUSY` / 409 Conflict immediately |
| medium | `SystemStatus::set()` is called on `WRITE_FAILED`/`VERIFY_FAILED` but silently omitted from `INVALID_FIRMWARE`, `NO_OTA_PARTITION`, and `BEGIN_FAILED` — OTA subsystem status does not reflect these failures | Added `SystemStatus::set(ERROR)` to all three missing error paths |
| low | Task 3 claimed partition info logging was complete, but code only emitted `LOG_I("OTA", "Update started")` with no label/size | Added `Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size);` |
| low | `NO_OTA_PARTITION`, `BEGIN_FAILED`, `WRITE_FAILED`, `VERIFY_FAILED` (server-side failures) return HTTP 400 (client error); `OTA_BUSY` conflicts returned as 400 | Error-code-to-HTTP mapping: `INVALID_FIRMWARE` → 400, `OTA_BUSY` → 409, server failures → 500 |
| dismissed | `POST /api/ota/upload` is unauthenticated / CSRF-vulnerable | FALSE POSITIVE: No endpoint in this device has authentication — `/api/reboot`, `/api/reset`, `/api/settings` are all unauthenticated. This is a pre-existing architectural design gap (LAN-only device), not introduced by this story. Requires a dedicated security story. |
| dismissed | Missing `return` statements after `NO_OTA_PARTITION` and `BEGIN_FAILED` — code falls through to write path | FALSE POSITIVE: FALSE POSITIVE. The actual code at lines 480 and 490 contains explicit `return;` statements. Validator C misread a code snippet. |
| dismissed | Task 8 (header declaration) incomplete — `_handleOTAUpload()` not in `WebPortal.h` | FALSE POSITIVE: FALSE POSITIVE. The task itself states "or keep inline like logo upload" as an explicit alternative. The inline lambda approach is the correct pattern and matches the logo upload implementation. |
| dismissed | `std::vector<OTAUploadState>` introduces heap churn on hot async path | FALSE POSITIVE: This is the established project pattern — `g_logoUploads` uses the same `std::vector` approach. OTA is single-flight (now enforced), so the vector holds at most one entry. Not worth a divergent pattern. |
| dismissed | Oversized binary not rejected before flash writes begin | FALSE POSITIVE: `Update.begin(partition->size)` tells the library the maximum expected size. `Update.write()` will fail and return fewer-than-requested bytes when the partition is full, which the existing write-failure path handles. `Update.end(true)` accepts partial writes correctly. The library itself is the bounds guard. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md

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
| **Total Issues** | **9** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Missing automated tests for critical safety feature | main.cpp | +3 |
| 🔴 CRITICAL | Unsafe temporary String with .c_str() pattern | main.cpp:451-452 | +3 |
| 🟠 IMPORTANT | Inconsistent logging patterns between code paths | main.cpp:450-456 | +1 |
| 🟠 IMPORTANT | OTA subsystem shows misleading status during rollback | SystemStatus.cpp:29-32 | +1 |
| 🟠 IMPORTANT | SystemStatus::set uses best-effort mutex with silent fallback | SystemStatus.cpp:37-51 | +1 |
| 🟡 MINOR | Missing error handling for esp_ota_get_last_invalid_partition() | main.cpp:418-424 | +0.3 |
| 🟡 MINOR | g_bootStartMs could be 0 causing immediate timeout | main.cpp:122,430-441 | +0.3 |
| 🟡 MINOR | Missing inline documentation for 60s timeout rationale | main.cpp:123 | +0.3 |
| 🟡 MINOR | No validation of FW_VERSION format compliance | SystemStatus.cpp:91 | +0.3 |

### Clean Pass Count: 4 (Performance, Security, Type Safety, Style)

### Evidence Score: 10.2

| Score | Verdict |
|-------|---------|
| **10.2** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[7/10] Single Responsibility Violation:** The `performOtaSelfCheck()` function mixes three concerns: (1) WiFi state checking, (2) timeout calculation, (3) SystemStatus reporting, and (4) ESP OTA API calls. This violates SRP and makes unit testing impossible.
  - 📍 `firmware/src/main.cpp:430-476`
  - 💡 Fix: Extract `shouldMarkValid()`, `formatElapsedMessage()`, and `updateOtaStatus()` helper functions

- **Over-engineering:** The `FlightStatsSnapshot` struct exists solely to ferry the `rollback_detected` flag from main.cpp to SystemStatus.cpp. This is an unnecessary abstraction layer when a simple getter like `bool SystemStatus::setRollbackDetected(bool)` would suffice.
  - 📍 `firmware/core/SystemStatus.h:32-39`

---

## 🐍 Code Quality Issues

- **Type Safety Concern:** The code uses `("Firmware verified..." + String(elapsedSec) + "s").c_str()` pattern which relies on evaluation order guarantees. While safe in Arduino String's implementation, this is fragile code that could break with compiler optimizations or different String implementations.
  - 📍 `firmware/src/main.cpp:451-452`

- **Magic Number:** The `60000` timeout value is defined as a constant but lacks rationale documentation explaining why 60 seconds was chosen over other values.
  - 📍 `firmware/src/main.cpp:123`

- **Inconsistent Logging:** The WiFi-connected path uses `Serial.printf()` directly while the timeout path uses `LOG_I()` macro. This makes log filtering and level control inconsistent.
  - 📍 `firmware/src/main.cpp:450,454`

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug: Potential race condition in SystemStatus::set:**
  When mutex acquisition fails (line 47), the code still updates the status without synchronization. In a dual-core system where `performOtaSelfCheck()` runs on Core 1 and HTTP handlers (which read status) run on async TCP task, this could cause torn reads or data corruption.
  - 📍 `firmware/core/SystemStatus.cpp:46-50`
  - 🔄 Reproduction: Under high load with concurrent status updates and API requests, the OTA status might show inconsistent values.

- **🐛 Bug: g_bootStartMs uninitialized read risk:**
  If `performOtaSelfCheck()` were ever called before `setup()` (e.g., via early init or static initialization), `g_bootStartMs` would be 0, causing `millis() - 0` to return a large value due to unsigned wraparound, triggering immediate timeout.
  - 📍 `firmware/src/main.cpp:122,441`
  - 🔄 Reproduction: Add a call to `performOtaSelfCheck()` in a constructor or global init before `setup()` runs.

- **Missing Error Handling:** `detectRollback()` doesn't check if `esp_ota_get_last_invalid_partition()` returns an error. While unlikely, the ESP-IDF API could fail, and the code treats any non-NULL as a rollback.
  - 📍 `firmware/src/main.cpp:418-424`

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Misleading initial OTA status:**
  On rollback boot, the OTA subsystem initially shows "Not initialized" (from `SystemStatus::init()`) before later showing the WARNING status. This is confusing UX—the user sees OK/Not initialized when a rollback was actually detected.
  - 📍 `firmware/core/SystemStatus.cpp:29-32`
  - 💥 Explosion radius: User confusion when checking device health after a rollback event.

- **💣 Tech Debt: No automated test coverage:**
  This is a critical safety feature (prevents bricking) with zero unit tests. The story mentions manual testing only. The existing test framework (Unity) is available but unused for this feature.
  - 📍 Entire implementation
  - 💥 Explosion radius: Future regressions in safety-critical code go undetected.

- **💣 Tech Debt: FW_VERSION format unvalidated:**
  The story specifies response should include `"firmware_version": "1.0.0"` but there's no validation that `FW_VERSION` follows semantic versioning or any specific format. A malformed version string could break API consumers.
  - 📍 `firmware/core/SystemStatus.cpp:91`

---

## 🛠️ Suggested Fixes

### 1. Add Unit Tests for OTA Self-Check

**File:** `firmware/test/test_ota_self_check/test_main.cpp`

Create a test file with mocked ESP-IDF OTA functions to verify:
- Normal boot marks valid without status change
- Post-OTA boot with WiFi connected marks valid with OK status
- Post-OTA boot with timeout marks valid with WARNING status
- Rollback detection sets correct flag

### 2. Fix String Concatenation Pattern

**File:** `firmware/src/main.cpp`

**Current code (lines 450-452):**
```cpp
Serial.printf("[OTA] Firmware marked valid — WiFi connected in %lums\n", elapsed);
SystemStatus::set(Subsystem::OTA, StatusLevel::OK,
    ("Firmware verified — WiFi connected in " + String(elapsedSec) + "s").c_str());
```

**Fixed code:**
```cpp
String msg = "Firmware verified — WiFi connected in " + String(elapsedSec) + "s";
LOG_I("OTA", "Firmware marked valid — WiFi connected in %lums", elapsed);
SystemStatus::set(Subsystem::OTA, StatusLevel::OK, msg);
```

### 3. Fix Inconsistent Logging

**File:** `firmware/src/main.cpp:454`

**Current:**
```cpp
LOG_I("OTA", "Firmware marked valid on timeout — WiFi not verified");
```

**Fixed (for consistency with WiFi path):**
```cpp
Serial.printf("[OTA] Firmware marked valid on timeout — WiFi not verified\n");
```
OR convert the WiFi path to use `LOG_I`.

### 4. Document Timeout Rationale

**File:** `firmware/src/main.cpp:123`

**Current:**
```cpp
static constexpr unsigned long OTA_SELF_CHECK_TIMEOUT_MS = 60000;
```

**Fixed:**
```cpp
// 60s timeout per Architecture Decision F3: allows good firmware to connect
// even on slow networks, while preventing indefinite wait on bad firmware.
// Typical WiFi connect time: 5-15s. Max acceptable: 60s.
static constexpr unsigned long OTA_SELF_CHECK_TIMEOUT_MS = 60000;
```

### 5. Fix OTA Status Initialization on Rollback

**File:** `firmware/src/main.cpp`

After `detectRollback()` in setup(), immediately set OTA status if rollback detected:

```cpp
// In setup(), after SystemStatus::init():
if (g_rollbackDetected) {
    SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
        "Firmware rolled back to previous version");
}
```

This removes the need for the deferred status update in `performOtaSelfCheck()`.

### 6. Add Error Handling to detectRollback

**File:** `firmware/src/main.cpp:418-424`

**Current:**
```cpp
static void detectRollback() {
    const esp_partition_t* invalid = esp_ota_get_last_invalid_partition();
    if (invalid != NULL) {
        g_rollbackDetected = true;
        Serial.printf("[OTA] Rollback detected: partition '%s' was invalid\n", invalid->label);
    }
}
```

**Fixed:**
```cpp
static void detectRollback() {
    const esp_partition_t* invalid = esp_ota_get_last_invalid_partition();
    if (invalid != NULL) {
        g_rollbackDetected = true;
        LOG_E("OTA", "Rollback detected: partition '%s' was invalid", invalid->label);
    }
}
```
(Also fixes inconsistent logging to use LOG_E for error-level rollback detection)

---

## 📋 Acceptance Criteria Verification

| AC | Status | Evidence |
|----|--------|----------|
| AC #1 | ✅ IMPLEMENTED | `performOtaSelfCheck()` marks valid on WiFi connect, logs timing, sets OK status |
| AC #2 | ✅ IMPLEMENTED | Timeout at 60s marks valid, sets WARNING status |
| AC #3 | ⚠️ PARTIAL | Bootloader behavior documented but not tested; no watchdog integration test |
| AC #4 | ✅ IMPLEMENTED | `detectRollback()` sets flag, logs partition label; deferred status in loop() |
| AC #5 | ✅ IMPLEMENTED | `toExtendedJson()` includes `firmware_version` and `rollback_detected` |
| AC #6 | ⚠️ PARTIAL | No SystemStatus::set on normal boot, but no log either (verbose only) |

---

## 🎯 Critical Action Items

1. **Add automated tests** - This safety-critical feature needs test coverage
2. **Fix the String concatenation pattern** - Use explicit String variable, not temporaries with `.c_str()`
3. **Initialize OTA status early on rollback** - Remove confusing "Not initialized" intermediate state
4. **Document the timeout rationale** - Future maintainers need to know why 60s was chosen

**Review Actions:**
- Issues Found: 9
- Issues Fixed: 0
- Action Items Created: 4

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 0 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **9** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | The `performOtaSelfCheck()` function uses `LOG_I` for an OTA timeout message when Acceptance Criterion #2 explicitly requests a "WARNING log message". This violates the AC and is inconsistent with the expected severity of a timeout scenario. | `firmware/src/main.cpp:525` | +1 |
| 🔴 CRITICAL | If `esp_ota_mark_app_valid_cancel_rollback()` fails, the `performOtaSelfCheck()` function logs an ERROR but still sets `g_otaSelfCheckDone = true`. This could mask a critical problem where the application could not mark itself valid, leaving it vulnerable to rollback if it crashes later, and falsely indicates the self-check is complete. | `firmware/src/main.cpp:535` | +3 |
| 🟠 IMPORTANT | Acceptance Criterion #6 states that on a normal boot, no `SystemStatus::set` call should be made. However, if `esp_ota_mark_app_valid_cancel_rollback()` fails on a normal boot, `SystemStatus::set(ERROR)` is called. This contradicts the AC, as the idempotent call should ideally always succeed on a valid image. | `firmware/src/main.cpp:535` | +1 |
| 🟡 MINOR | The `tickStartupProgress()` function uses magic numbers `2000` and `4000` (milliseconds) for timing display messages without defining them as named constants, violating architectural guidelines against magic numbers. | `firmware/src/main.cpp:846, 850` | +0.3 |
| 🟡 MINOR | For a successful OTA self-check via WiFi, `Serial.printf` logs elapsed time in milliseconds, but `SystemStatus::set` converts it to seconds, losing precision. Harmonizing these messages and ensuring consistent precision would improve diagnostics. | `firmware/src/main.cpp:521, 522` | +0.3 |
| 🟡 MINOR | The initial `Serial.printf` for rollback detection in `detectRollback()` includes the invalid partition label, but the deferred `SystemStatus::set` uses a generic message, reducing diagnostic detail in the API status. | `firmware/src/main.cpp:495, 540` | +0.3 |
| 🟡 MINOR | The `detectRollback()` function uses `Serial.printf` directly instead of the established `LOG_E/I/V` macros, bypassing log level filtering. `LOG_W` would be more appropriate for this warning. | `firmware/src/main.cpp:495` | +0.3 |
| 🟠 IMPORTANT | Acceptance Criterion #3 describes critical bootloader-level automatic rollback upon crash. The story's testing strategy acknowledges this is "tricky" to test. Relying on manual, difficult, and potentially unreliable methods for such a critical safety feature represents a significant gap in demonstrable test coverage. | `_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md` | +1 |
| 🟡 MINOR | The Dev Notes section implicitly refers to the `FlightStatsSnapshot` struct "wherever it's defined" without explicitly stating its location (e.g., in a header file). This lack of explicit reference for a core data structure can hinder clarity for new developers. | `_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md` | +0.3 |
| 🟢 CLEAN PASS | 6 categories | | -3.0 |

### Evidence Score: 4.5

| Score | Verdict |
|-------|---------|
| **4.5** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected (apart from general code quality issues).

---

## 🐍 Pythonic Crimes & Readability

**Style Violation:** The `detectRollback()` function directly uses `Serial.printf` instead of the project's established `LOG_E/I/V` macros, bypassing controlled log levels. This violates architectural guidelines for consistent logging practices.
  - 📍 `firmware/src/main.cpp:495`

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** If `esp_ota_mark_app_valid_cancel_rollback()` fails to mark the current application as valid (e.g., due to validation issues), the `performOtaSelfCheck()` function logs an error but critically proceeds to set `g_otaSelfCheckDone = true`. This leads to a false positive, where the system believes the self-check completed successfully even though the firmware was not properly validated. This leaves the device in a vulnerable state where a subsequent crash could trigger an unintended rollback, as the `PENDING_VERIFY` state was not transitioned to `VALID`.
  - 📍 `firmware/src/main.cpp:535`
  - 🔄 Reproduction: Simulate a failure in `esp_ota_mark_app_valid_cancel_rollback()` (e.g., by returning an error code). Observe that `g_otaSelfCheckDone` is set to `true`, potentially masking the failure from higher-level logic.

- **🎭 Lying Test:** Acceptance Criterion #3 describes an ESP32 bootloader mechanism for automatic rollback in case of firmware crash. The story explicitly notes this is "tricky" to test, implying reliance on this external behavior rather than a direct, verifiable application-level test. This means a critical safety mechanism is difficult to confirm through testing, potentially leading to a false sense of security that the rollback will always function as expected under all failure conditions.
  - 📍 `_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md`
  - 🤥 Why it lies: The test strategy highlights the difficulty in verifying a critical system behavior, effectively creating a gap where this AC's implementation cannot be fully proven with robust tests.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** The `tickStartupProgress()` function contains magic numbers (`2000`, `4000`) for display message timing durations. These unnamed constants make the code less readable and harder to maintain, as their purpose is not immediately clear, violating architectural guidelines for constants.
  - 📍 `firmware/src/main.cpp:846, 850`
  - 💥 Explosion radius: High - impacts readability, future modifications, and consistency across timing-related logic.

- **💣 Tech Debt:** The successful OTA self-check log message for `Serial.printf` uses milliseconds for timing, while the corresponding `SystemStatus::set` message converts it to seconds. This inconsistency in reporting precision could cause confusion in diagnostics and does not adhere to a unified logging strategy.
  - 📍 `firmware/src/main.cpp:521, 522`
  - 💥 Explosion radius: Medium - impacts diagnostic clarity and log parsing consistency.

- **💣 Tech Debt:** The initial `Serial.printf` for rollback detection includes the invalid partition label, which is valuable diagnostic information. However, the deferred `SystemStatus::set` for rollback detection uses a generic message ("Firmware rolled back to previous version") without this detail. This inconsistency means that a critical piece of diagnostic information is lost when querying `/api/status`.
  - 📍 `firmware/src/main.cpp:495, 540`
  - 💥 Explosion radius: Medium - impacts API diagnostic completeness and troubleshooting.

- **💣 Tech Debt:** The story's Dev Notes provide implicit guidance for modifying `FlightStatsSnapshot` ("wherever it's defined") rather than explicitly stating its location. This lack of clear documentation for a core data structure can lead to developer confusion, increased onboarding time, and potential errors when maintaining or extending related functionality.
  - 📍 `_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md`
  - 💥 Explosion radius: Low - primarily impacts developer experience and code discoverability.

---

## 🛠️ Suggested Fixes

### 1. Correct Log Level for OTA Timeout Warning

**File:** `firmware/src/main.cpp`
**Issue:** Acceptance Criterion #2 requires a WARNING log message for OTA timeout, but the implementation uses `LOG_I`.

```diff
--- a/firmware/src/main.cpp
+++ b/firmware/src/main.cpp
@@ -522,7 +522,7 @@
                     SystemStatus::set(Subsystem::OTA, StatusLevel::OK,
                         ("Firmware verified — WiFi connected in " + String(elapsedSec) + "s").c_str());
                 } else {
-                    LOG_I("OTA", "Firmware marked valid on timeout — WiFi not verified");
+                    LOG_W("OTA", "Firmware marked valid on timeout — WiFi not verified"); // Changed to LOG_W per AC #2
                     SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
                         "Marked valid on timeout — WiFi not verified");
                 }

```

### 2. Robust Error Handling for `esp_ota_mark_app_valid_cancel_rollback()`

**File:** `firmware/src/main.cpp`
**Issue:** If `esp_ota_mark_app_valid_cancel_rollback()` fails, `g_otaSelfCheckDone` is still set to `true`, potentially masking a critical failure.

```diff
--- a/firmware/src/main.cpp
+++ b/firmware/src/main.cpp
@@ -533,14 +533,14 @@
                 LOG_V("OTA", "Self-check: already valid (normal boot)");
                 // AC #6: No SystemStatus::set call on normal boot
             }
+            g_otaSelfCheckDone = true; // Only mark as done if successful
         } else {
             Serial.printf("[OTA] ERROR: esp_ota_mark_app_valid_cancel_rollback() failed: %d\n", err);
             SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR,
                 ("Failed to mark firmware valid: error " + String(err)).c_str());
+            // Do NOT set g_otaSelfCheckDone = true here, as the self-check failed
         }
 
         // Deferred rollback status (SystemStatus wasn't ready in setup())
         if (g_rollbackDetected) {
             SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,
                 "Firmware rolled back to previous version");
         }
-
-        g_otaSelfCheckDone = true;
     }
 }
```

### 3. Harmonize WiFi Connected Time Logging Precision

**File:** `firmware/src/main.cpp`
**Issue:** `Serial.printf` logs WiFi connection time in milliseconds, while `SystemStatus::set` uses seconds, causing precision loss and inconsistency.

```diff
--- a/firmware/src/main.cpp
+++ b/firmware/src/main.cpp
@@ -520,8 +520,8 @@
             if (isPendingVerify) {
                 if (wifiConnected) {
                     unsigned long elapsedSec = elapsed / 1000;
-                    Serial.printf("[OTA] Firmware marked valid — WiFi connected in %lums\n", elapsed);
                     SystemStatus::set(Subsystem::OTA, StatusLevel::OK,
-                        ("Firmware verified — WiFi connected in " + String(elapsedSec) + "s").c_str());
+                        ("Firmware verified — WiFi connected in " + String(elapsed) + "ms").c_str()); // Use milliseconds for consistency
+                    Serial.printf("[OTA] Firmware marked valid — WiFi connected in %lums\n", elapsed); // Keep for detailed debug
                 } else {
                     LOG_I("OTA", "Firmware marked valid on timeout — WiFi not verified");
                     SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING,

```

---

**Review Actions:**
- Issues Found: 9
- Issues Fixed: 0
- Action Items Created: 0

]]></file>
<file id="f63a150c" path="[Validator D]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review: `fn-1.4 - ota-self-check-and-rollback-detection`**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md`

## Review Summary

| Category | Issues |
|---|---:|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 3 |
| Task Completion Lies | 0 |
| SOLID / Architecture Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| Total | 7 |

## Findings

1. **High: AC #2 is only partially implemented because the timeout path logs at INFO, not WARNING.**  
   `performOtaSelfCheck()` uses `LOG_I("OTA", "Firmware marked valid on timeout — WiFi not verified")` in the timeout branch, but the story explicitly requires a warning log for that case. This is a direct acceptance-criteria miss, not a style nit.  
   - Evidence: `firmware/src/main.cpp:~395-408`

2. **High: The self-check permanently gives up after the first `esp_ota_mark_app_valid_cancel_rollback()` failure.**  
   `g_otaSelfCheckDone` is set to `true` even when `esp_ota_mark_app_valid_cancel_rollback()` returns an error. That means one failure stops all future attempts, leaving the image unvalidated until the next reboot. For rollback-sensitive boot logic, “log ERROR and never retry” is the wrong failure mode.  
   - Evidence: `firmware/src/main.cpp:~399-420`

3. **High: AC #6 is violated on normal boots after a prior rollback.**  
   The story says normal boot must not emit any OTA `SystemStatus::set()` entry. This code unconditionally emits `Subsystem::OTA = WARNING` whenever `g_rollbackDetected` is true, even if the current boot is a normal non-OTA boot. That also overwrites the current boot’s OK/WARNING self-check result, so the OTA subsystem no longer tells you whether this firmware just passed verification or merely booted after an older rollback.  
   - Evidence: `firmware/src/main.cpp:~413-418`

4. **High: There is no automated coverage for the new boot-time OTA logic or the `/api/status` contract change.**  
   The story changes boot validation behavior and adds two API fields, but no test updates are present under `firmware/test/` or `tests/`. The missing cases are exactly the brittle ones: pending-verify boot, 60s timeout path, rollback-detected serialization, and the “no OTA status on normal boot” rule. In this codebase, previous story reviews already treated missing tests as a serious defect.  
   - Evidence: reviewed file set contains only `firmware/src/main.cpp` and `firmware/core/SystemStatus.cpp`

5. **Medium: OTA image state is polled in the hot loop on every iteration for up to 60 seconds.**  
   `performOtaSelfCheck()` calls `esp_ota_get_running_partition()` and `esp_ota_get_state_partition()` every time through `loop()` until completion. On an offline/AP boot that is thousands of unnecessary IDF calls on Core 1. The pending-verify state cannot change during that window except when this same function marks it valid, so it should be cached once.  
   - Evidence: `firmware/src/main.cpp:~382-392`

6. **Medium: The new OTA code violates the project logging pattern by bypassing `LOG_E/I/V`.**  
   The story adds raw `Serial.printf()` calls for rollback detection, WiFi-connected success, and OTA mark-valid failure. Architecture enforcement says logging should go through `utils/Log.h`. Mixing raw serial output with structured logs makes filtering and log-level control inconsistent exactly where you want deterministic boot diagnostics.  
   - Evidence: `firmware/src/main.cpp:~373-379`, `~401-403`, `~409-411`

7. **Medium: The story/file-list evidence is incomplete around `FlightStatsSnapshot`.**  
   `SystemStatus.cpp` now reads `stats.rollback_detected`, but the reviewed `main.cpp` snippet does not contain the struct definition the story claims was modified there. Either an unlisted header change exists, or the story documentation is inaccurate. That is a documentation/review-scope defect and makes the implementation evidence incomplete.  
   - Evidence: `firmware/core/SystemStatus.cpp:~79-82`; `firmware/src/main.cpp` shows `getFlightStatsSnapshot()` but not the struct definition the story claims to change

## Evidence Score

| Severity | Finding | Score |
|---|---|---:|
| 🟠 Important | AC #2 wrong log severity | +1.0 |
| 🟠 Important | No retry after mark-valid failure | +1.0 |
| 🟠 Important | AC #6 violated by rollback warning on normal boot | +1.0 |
| 🟠 Important | No automated tests for OTA boot logic/API contract | +1.0 |
| 🟠 Important | Repeated OTA state polling in hot loop | +1.0 |
| 🟡 Minor | Logging pattern violation | +0.3 |
| 🟡 Minor | Story/file-list evidence gap | +0.3 |
| 🟢 Clean Pass | No concrete security defect introduced in reviewed code | -0.5 |
| 🟢 Clean Pass | No type-safety regression surfaced in reviewed code | -0.5 |
| 🟢 Clean Pass | No obvious abstraction-layer breach beyond logging inconsistency | -0.5 |

**Evidence Score: 4.1**

| Score | Verdict |
|---|---|
| **4.1** | **MAJOR REWORK** |

## Suggested Fix Direction

1. Change the timeout branch to emit an actual warning-level diagnostic, even if that means extending the logging facade or using a clearly tagged warning serial line consistently.
2. Do not set `g_otaSelfCheckDone` when `esp_ota_mark_app_valid_cancel_rollback()` fails; keep retrying or move into an explicit failure state with retry/backoff.
3. Separate “current boot verification result” from “historical rollback detected” so AC #1/#2 status is not overwritten by AC #4.
4. Cache pending-verify state once per boot instead of recomputing it every loop iteration.
5. Add tests for:
   - pending-verify + WiFi connected
   - pending-verify + timeout
   - normal boot without OTA status emission
   - rollback-detected propagation into `/api/status`

**Review Actions**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

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
<var name="session_id">2da6bd01-26c5-4566-9461-6b8c38ba8a20</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="e97afb18">embedded in prompt, file id: e97afb18</var>
<var name="story_id">fn-1.4</var>
<var name="story_key">fn-1-4-ota-self-check-and-rollback-detection</var>
<var name="story_num">4</var>
<var name="story_title">4-ota-self-check-and-rollback-detection</var>
<var name="template">False</var>
<var name="timestamp">20260412_1502</var>
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
      - Commit message format: fix(component): brief description (synthesis-fn-1.4)
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