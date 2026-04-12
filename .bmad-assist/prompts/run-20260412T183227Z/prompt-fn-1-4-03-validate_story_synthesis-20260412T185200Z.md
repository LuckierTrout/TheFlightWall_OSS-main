<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 4 -->
<!-- Phase: validate-story-synthesis -->
<!-- Timestamp: 20260412T185200Z -->
<compiled-workflow>
<mission><![CDATA[

Master Synthesis: Story fn-1.4

You are synthesizing 2 independent validator reviews.

Your mission:
1. VERIFY each issue raised by validators
   - Cross-reference with story content
   - Identify false positives (issues that aren't real problems)
   - Confirm valid issues with evidence

2. PRIORITIZE real issues by severity
   - Critical: Blocks implementation or causes major problems
   - High: Significant gaps or ambiguities
   - Medium: Improvements that would help
   - Low: Nice-to-have suggestions

3. SYNTHESIZE findings
   - Merge duplicate issues from different validators
   - Note validator consensus (if 3+ agree, high confidence)
   - Highlight unique insights from individual validators

4. APPLY changes to story file
   - You have WRITE PERMISSION to modify the story
   - CRITICAL: Before using Edit tool, ALWAYS Read the target file first
   - Use EXACT content from Read tool output as old_string, NOT content from this prompt
   - If Read output is truncated, use offset/limit parameters to locate the target section
   - Apply fixes for verified issues
   - Document what you changed and why

Output format:
## Synthesis Summary
## Issues Verified (by severity)
## Issues Dismissed (false positives with reasoning)
## Changes Applied

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

1. **Given** the device boots after an OTA update **When** WiFi connects successfully **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called **And** the partition is marked valid **And** a log message records the time to mark valid (e.g., `"Marked valid, WiFi connected in 8s"`) **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Firmware verified — WiFi connected in Xs")` is recorded

2. **Given** the device boots after an OTA update and WiFi does not connect **When** 60 seconds elapse since boot **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called on timeout **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Marked valid on timeout — WiFi not verified")` is recorded **And** a WARNING log message is emitted

3. **Given** the device boots after an OTA update and crashes before the self-check completes **When** the watchdog fires and the device reboots **Then** the bootloader detects the active partition was never marked valid **And** the bootloader rolls back to the previous partition automatically (this is ESP32 bootloader behavior, not application code)

4. **Given** a rollback has occurred **When** `esp_ota_get_last_invalid_partition()` returns non-NULL during boot **Then** a `rollbackDetected` flag is set to `true` **And** `SystemStatus::set(Subsystem::OTA, StatusLevel::WARNING, "Firmware rolled back to previous version")` is recorded **And** the invalid partition label is logged (e.g., `"Rollback detected: partition 'app1' was invalid"`)

5. **Given** `GET /api/status` is called **When** the response is built **Then** it includes `"firmware_version": "1.0.0"` (from `FW_VERSION` build flag) **And** `"rollback_detected": true/false` in the response data object

6. **Given** normal boot (not after OTA) **When** the self-check runs **Then** `esp_ota_mark_app_valid_cancel_rollback()` is called (idempotent — safe on non-OTA boots, returns ESP_OK with no effect when image is already valid) **And** no WARNING or ERROR status is set for OTA subsystem

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

Note: `FW_VERSION` is a compile-time macro already available project-wide. If SystemStatus.cpp doesn't have it, add:
```cpp
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"
#endif
```

### Critical Gotchas

1. **No factory partition** — The partition table has only app0 and app1 (no factory). If BOTH partitions become invalid, the device is bricked. However, this can only happen if the user OTA-updates with bad firmware AND the rollback target is also bad — extremely unlikely since the rollback target was previously running.

2. **Rollback detection is per-boot** — `esp_ota_get_last_invalid_partition()` returns the last invalid partition persistently (survives reboots). The flag will remain true until a successful OTA clears the invalid partition state. Consider whether the dashboard should show this persistently or just on the first boot after rollback.

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
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

# 🎯 Story Context Validation Report

<!-- report_header -->

**Story:** fn-1-4-ota-self-check-and-rollback-detection - OTA Self-Check & Rollback Detection
**Story File:** _bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md
**Validated:** 2026-04-12
**Validator:** Quality Competition Engine

---

<!-- executive_summary -->

## Executive Summary

### Issues Overview

| Category | Found | Applied |
|----------|-------|---------|
| 🚨 Critical Issues | 1 | 0 |
| ⚡ Enhancements | 5 | 0 |
| ✨ Optimizations | 0 | 0 |
| 🤖 LLM Optimizations | 1 | 0 |

**Overall Assessment:** The story is well-structured and provides significant detail for implementation, especially given the critical nature of OTA updates. However, there is one critical gap in defining user-facing behavior for rollback detection, alongside several opportunities for enhanced clarity and consistency in the acceptance criteria and technical notes.

---

<!-- evidence_score_summary -->

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | AC: Rollback detected flag persistence UI behavior undefined | Acceptance Criteria | +3 |
| 🟠 IMPORTANT | AC: Inconsistent time unit in SystemStatus message (AC1 & AC2) | Acceptance Criteria | +1 |
| 🟠 IMPORTANT | AC: Ambiguous `firmware_version` value (AC5) | Acceptance Criteria | +1 |
| 🟠 IMPORTANT | AC: Clarify normal boot SystemStatus (AC6) | Acceptance Criteria | +1 |
| 🟠 IMPORTANT | Technical Specification: Consistent `FW_VERSION` accessibility in `SystemStatus.cpp` | Technical Specification | +1 |
| 🟡 MINOR | Technical Alignment: Idiomatic `String` usage in `SystemStatus::set` (performOtaSelfCheck) | Technical Alignment | +0.3 |
| 🟢 CLEAN PASS | 8 | | -4 |

### Evidence Score: 3.3

| Score | Verdict |
|-------|---------|
| **3.3** | **MAJOR REWORK** |

---

<!-- story_quality_gate -->

## 🎯 Ruthless Story Validation fn-1.4

### INVEST Criteria Assessment

| Criterion | Status | Severity | Details |
|-----------|--------|----------|---------|
| **I**ndependent | OK | 0/10 | Story clearly lists its dependencies and dependents, indicating it's part of a sequence but can be developed independently once dependencies are met. |
| **N**egotiable | Minor Concern | 2/10 | The story is highly prescriptive in implementation details, which is partly justified by the critical nature of OTA and existing architecture decisions, but could limit developer flexibility in non-critical aspects. |
| **V**aluable | OK | 0/10 | Directly addresses a critical user need for device resilience and prevention of bricking due to bad firmware updates. |
| **E**stimable | OK | 0/10 | Detailed tasks, code patterns, API references, and placement guidance make the story highly estimable. "Tricky" rollback test noted but does not block estimation. |
| **S**mall | OK | 0/10 | With 5 main tasks and clear implementation patterns, the story is appropriately sized for a single sprint. |
| **T**estable | OK | 0/10 | Acceptance criteria are explicit and measurable, and the testing strategy covers various scenarios. |

### INVEST Violations

- **[2/10] Negotiable:** The story is very prescriptive about *how* to implement (e.g., exact code snippets, variable names, placement within `main.cpp`), reducing negotiation room for technical decisions that might achieve the same outcome.

### Acceptance Criteria Issues

- **Minor inconsistency:** Inconsistent unit of time (seconds vs milliseconds) between SystemStatus message in AC1/AC2 (`"Xs"`) and the logging message in Task 2 (`"%lums"`). The implementation pattern uses seconds in `SystemStatus::set` message.
  - *Quote:* "Firmware verified — WiFi connected in Xs" (AC1) vs. "Firmware marked valid — WiFi connected in %lums" (Task 2)
  - *Recommendation:* Standardize the unit of time for user-facing messages (e.g., seconds) across all ACs and ensure consistency in the implementation details.
- **Ambiguity:** AC5 states `"firmware_version": "1.0.0"`. While `FW_VERSION` is a build flag, hardcoding a specific version in the AC can be misleading if the actual version changes.
  - *Quote:* `"firmware_version": "1.0.0"`
  - *Recommendation:* Replace "1.0.0" with a reference to the `FW_VERSION` build flag (e.g., `"firmware_version": "value of FW_VERSION build flag"`) to reflect its dynamic nature.
- **Minor clarity:** AC6 states "no WARNING or ERROR status is set for OTA subsystem" for normal boot. It could be clearer that `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, ...)` should explicitly NOT be called for normal boots to avoid unnecessary status updates/logs.
  - *Quote:* "no WARNING or ERROR status is set for OTA subsystem"
  - *Recommendation:* Clarify that no `SystemStatus::set` call related to OTA status should occur for normal, already-valid boots.

### Hidden Risks and Dependencies

✅ No hidden dependencies or blockers identified.

### Estimation Reality-Check

**Assessment:** Realistic

The story is well-defined, providing clear tasks, detailed implementation patterns, and specific API calls. Critical potential issues are identified as "Critical Gotchas," which reduces the risk of "unknown unknowns." The testing strategy is comprehensive, with the only noted "tricky" aspect (rollback test) being explicitly acknowledged as not blocking implementation. This level of detail supports a solid and realistic estimation.

### Technical Alignment

**Status:** Aligned

- **Minor Inconsistency:** The implementation pattern in `performOtaSelfCheck()` uses `("Firmware verified — WiFi connected in " + String(elapsed / 1000) + "s").c_str()` to set the SystemStatus message. The `SystemStatus::set` function expects a `const String&`, which means this creates a temporary `String` from `const char*`.
  - *Architecture Reference:* `SystemStatus.cpp` (Function signature: `SystemStatus::set(Subsystem sys, StatusLevel level, const String& message)`)
  - *Recommendation:* For idiomatic C++ `String` usage, construct the `String` directly before passing it, e.g., `String("Firmware verified — WiFi connected in ") + String(elapsed / 1000) + "s"`.

### Evidence Score: 3.3 → MAJOR REWORK

---

<!-- critical_issues_section -->

<h2>🚨 Critical Issues (Must Fix)</h2>

These are essential requirements, security concerns, or blocking issues that could cause implementation disasters.

<h3>1. AC: Rollback detected flag persistence UI behavior undefined</h3>

**Impact:** User confusion, poor user experience (UX regression), and potential misinterpretation of device state. A user might not be aware a rollback occurred if the notification is transient, or conversely, be constantly alerted if it's overly persistent.
**Source:** Acceptance Criteria

**Problem:**
Critical Gotcha #2 states: "Rollback detection is per-boot — `esp_ota_get_last_invalid_partition()` returns the last invalid partition persistently (survives reboots). The flag will remain true until a successful OTA clears the invalid partition state. Consider whether the dashboard should show this persistently or just on the first boot after rollback." However, the story does not make a decision or provide an acceptance criterion for this critical user-facing behavior. This leaves a significant gap in the UX specification for how rollback detection is communicated to the user, leading to potential implementation ambiguity and a suboptimal user experience.

**Recommended Fix:**
Add a new Acceptance Criterion (or clarify an existing one) to explicitly define the desired UI behavior for the `rollback_detected` flag. For example:
"**Given** a rollback has occurred **When** the dashboard is accessed **Then** `rollback_detected` is displayed persistently in the Firmware Card until a successful OTA update is applied, to clearly inform the user of the device's state." OR "...**Then** a temporary toast notification is shown on the first boot after rollback, and the flag is cleared from persistent display on subsequent boots (if technically feasible and desired)." The key is to make a definitive decision and capture it in an AC.

---

<!-- enhancements_section -->

<h2>⚡ Enhancement Opportunities (Should Add)</h2>

Additional guidance that would significantly help the developer avoid mistakes.

<h3>1. Technical Specification: Consistent FW_VERSION accessibility in SystemStatus.cpp</h3>

**Benefit:** Improved code consistency, reduced potential for version reporting bugs, and clearer development instructions.
**Source:** Technical Specification

**Current Gap:**
Task 4 suggests adding `obj["firmware_version"] = FW_VERSION;` in `SystemStatus::toExtendedJson()` and includes a note: "If SystemStatus.cpp doesn't have it, add: `#ifndef FW_VERSION #define FW_VERSION "0.0.0-dev" #endif`." This implies `FW_VERSION` might not be reliably available "project-wide" as stated elsewhere. Relying on a conditional local define can lead to inconsistencies if the build flag changes or if `SystemStatus.cpp` is compiled in an environment where the global define isn't passed.

**Suggested Addition:**
Clarify how `FW_VERSION` is intended to be accessed across the project. If it's a global macro, `SystemStatus.cpp` should simply use it without a local fallback. If a local fallback is genuinely needed for specific compile environments (e.g., IDE parsing), this should be explicitly stated as the architectural pattern for such macros. Consider adding a `version.h` header that centralizes the `FW_VERSION` definition and fallbacks, and include it where needed. For now, explicitly state that `SystemStatus.cpp` should `#include "main.cpp"` or a common header that provides `FW_VERSION`.

---

<!-- optimizations_section -->

<h2>✨ Optimizations (Nice to Have)</h2>

Performance hints, development tips, and additional context for complex scenarios.

✅ No additional optimizations identified.

---

<!-- llm_optimizations_section -->

<h2>🤖 LLM Optimization Improvements</h2>

Token efficiency and clarity improvements for better dev agent processing.

<h3>1. Redundant FW_VERSION conditional define note in Task 4.</h3>

**Issue:** Context Overload
**Token Impact:** Minor

**Current:**
```
Note: `FW_VERSION` is a compile-time macro already available project-wide. If SystemStatus.cpp doesn't have it, add: `#ifndef FW_VERSION #define FW_VERSION "0.0.0-dev" #endif`
```

**Optimized:**
```
Note: Ensure FW_VERSION is accessible in SystemStatus.cpp (e.g., via #include or conditional define if necessary for specific build environments).
```

**Rationale:** The original note is slightly contradictory; it states `FW_VERSION` is "project-wide" then immediately suggests a local conditional define. The optimized version is more concise, avoids redundancy, and directly states the requirement for `FW_VERSION` accessibility in `SystemStatus.cpp` without over-explaining the fallback mechanism unless explicitly required by a broader architectural decision.

---

<!-- competition_results -->

<h2>🏆 Competition Results</h2>

<h3>Quality Metrics</h3>

| Metric | Score |
|--------|-------|
| Requirements Coverage | 80% |
| Architecture Alignment | 95% |
| Previous Story Integration | 100% |
| LLM Optimization Score | 85% |
| **Overall Quality Score** | **89%** |

<h3>Disaster Prevention Assessment</h3>

- **Reinvention Prevention:** ✅ Status: Clean. No risks of reinventing the wheel identified; existing components and patterns are well-utilized.
- **Technical Specification:** 🟠 Status: Minor Risk. Potential for inconsistent `FW_VERSION` accessibility and handling in `SystemStatus.cpp` if not explicitly managed.
- **File Structure:** ✅ Status: Clean. No issues identified; file modifications are clearly specified and align with project structure.
- **Regression:** 🔴 Status: Critical Risk. Undefined UI behavior for persistent rollback detection could lead to a poor or confusing user experience.
- **Implementation:** ✅ Status: Clean. No specific implementation disasters identified; patterns are clear and detailed.

<h3>Competition Outcome</h3>

🏆 **Validator identified 6 improvements** that enhance the story context.

---

**Report Generated:** 2026-04-12
**Validation Engine:** BMAD Method Quality Competition v1.0

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-12</var>
<var name="description">Master synthesizes validator findings and applies changes to story file</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-1</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/validate-story-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/validate-story-synthesis/instructions.xml</var>
<var name="name">validate-story-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="session_id">be3a264b-4541-4c4a-a207-bea3f3c01e44</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="e97afb18">embedded in prompt, file id: e97afb18</var>
<var name="story_id">fn-1.4</var>
<var name="story_key">fn-1-4-ota-self-check-and-rollback-detection</var>
<var name="story_num">4</var>
<var name="story_title">4-ota-self-check-and-rollback-detection</var>
<var name="template">False</var>
<var name="timestamp">20260412_1452</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="validator_count">2</var>
</variables>
<instructions><workflow>
  <critical>Communicate all responses in English and generate all documents in English</critical>

  <critical>You are the MASTER SYNTHESIS agent. Your role is to evaluate validator findings
    and produce a definitive synthesis with applied fixes.</critical>
  <critical>You have WRITE PERMISSION to modify the story file being validated.</critical>
  <critical>All context (project_context.md, story file, anonymized validations) is EMBEDDED below - do NOT attempt to read files.</critical>
  <critical>Apply changes to story file directly using atomic write pattern (temp file + rename).</critical>

  <step n="1" goal="Analyze validator findings">
    <action>Read all anonymized validator outputs (Validator A, B, C, D, etc.)</action>
    <action>For each issue raised:
      - Cross-reference with story content and project_context.md
      - Determine if issue is valid or false positive
      - Note validator consensus (if 3+ validators agree, high confidence issue)
    </action>
    <action>Issues with low validator agreement (1-2 validators) require extra scrutiny</action>
  </step>

  <step n="1.5" goal="Review Deep Verify technical findings" conditional="[Deep Verify Findings] section present">
    <critical>Deep Verify provides automated technical analysis that complements validator reviews.
      DV findings focus on: patterns, boundary cases, assumptions, temporal issues, security, and worst-case scenarios.</critical>

    <action>Review each DV finding:
      - CRITICAL findings: Must be addressed - these indicate serious technical issues
      - ERROR findings: Should be addressed unless clearly false positive
      - WARNING findings: Consider addressing, document if dismissed
    </action>

    <action>Cross-reference DV findings with validator findings:
      - If validators AND DV flag same issue: High confidence, prioritize fix
      - If only DV flags issue: Verify technically valid, may be edge case validators missed
      - If only validators flag issue: Normal processing per step 1
    </action>

    <action>For each DV finding, determine:
      - Is this a genuine issue in the story specification?
      - Does the story need to address this edge case/scenario?
      - Is this already covered but DV missed it? (false positive)
    </action>

    <action>DV findings with patterns (CC-*, SEC-*, DB-*, DT-*, GEN-*) reference known antipatterns.
      Treat pattern-matched findings as higher confidence.</action>
  </step>

  <step n="2" goal="Verify and prioritize issues">
    <action>For verified issues, assign severity:
      - Critical: Blocks implementation or causes major problems
      - High: Significant gaps or ambiguities that need attention
      - Medium: Improvements that would help quality
      - Low: Nice-to-have suggestions
    </action>
    <action>Document false positives with clear reasoning for dismissal:
      - Why the validator was wrong
      - What evidence contradicts the finding
      - Reference specific story content or project_context.md
    </action>
  </step>

  <step n="3" goal="Apply changes to story file">
    <action>For each verified issue (starting with Critical, then High), apply fix directly to story file</action>
    <action>Changes should be natural improvements:
      - DO NOT add review metadata or synthesis comments to story
      - DO NOT reference the synthesis or validation process
      - Preserve story structure, formatting, and style
      - Make changes look like they were always there
    </action>
    <action>For each change, log in synthesis output:
      - File path modified
      - Section/line reference (e.g., "AC4", "Task 2.3")
      - Brief description of change
      - Before snippet (2-3 lines context)
      - After snippet (2-3 lines context)
    </action>
    <action>Use atomic write pattern for story modifications to prevent corruption</action>
  </step>

  <step n="4" goal="Generate synthesis report">
    <critical>Your synthesis report MUST be wrapped in HTML comment markers for extraction:</critical>
    <action>Produce structured output in this exact format (including the markers):</action>
    <output-format>
&lt;!-- VALIDATION_SYNTHESIS_START --&gt;
## Synthesis Summary
[Brief overview: X issues verified, Y false positives dismissed, Z changes applied to story file]

## Validations Quality
[For each validator: name, score, comments]
[Summary of validation quality - 1-10 scale]

## Issues Verified (by severity)

### Critical
[Issues that block implementation - list with evidence and fixes applied]
[Format: "- **Issue**: Description | **Source**: Validator(s) | **Fix**: What was changed"]

### High
[Significant gaps requiring attention]

### Medium
[Quality improvements]

### Low
[Nice-to-have suggestions - may be deferred]

## Issues Dismissed
[False positives with reasoning for each dismissal]
[Format: "- **Claimed Issue**: Description | **Raised by**: Validator(s) | **Dismissal Reason**: Why this is incorrect"]

## Deep Verify Integration
[If DV findings were present, document how they were handled]

### DV Findings Addressed
[List DV findings that resulted in story changes]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Action**: {What was changed}"]

### DV Findings Dismissed
[List DV findings determined to be false positives or not applicable]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Reason**: {Why dismissed}"]

### DV-Validator Overlap
[Note any findings flagged by both DV and validators - these are high confidence]
[If no DV findings: "Deep Verify did not produce findings for this story."]

## Changes Applied
[Complete list of modifications made to story file]
[Format for each change:
  **Location**: [File path] - [Section/line]
  **Change**: [Brief description]
  **Before**:
  ```
  [2-3 lines of original content]
  ```
  **After**:
  ```
  [2-3 lines of updated content]
  ```
]
&lt;!-- VALIDATION_SYNTHESIS_END --&gt;
    </output-format>

  </step>

  <step n="5" goal="Final verification">
    <action>Verify all Critical and High issues have been addressed</action>
    <action>Confirm story file changes are coherent and preserve structure</action>
    <action>Ensure synthesis report is complete with all sections populated</action>
  </step>
</workflow></instructions>
<output-template></output-template>
</compiled-workflow>