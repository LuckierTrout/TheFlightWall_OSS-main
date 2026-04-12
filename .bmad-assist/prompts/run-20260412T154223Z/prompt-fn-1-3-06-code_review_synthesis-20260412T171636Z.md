<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 3 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260412T171636Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-1.3

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
<file id="ab213a12" path="_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md" label="DOCUMENTATION"><![CDATA[

# Story fn-1.3: OTA Upload Endpoint

Status: complete

## Story

As a **user**,
I want **to upload a firmware binary to the device over the local network**,
So that **I can update the firmware without connecting a USB cable**.

## Acceptance Criteria

1. **Given** `POST /api/ota/upload` receives a multipart file upload **When** the first chunk arrives with first byte `0xE9` (ESP32 magic byte) **Then** `Update.begin(partition->size)` is called using the next OTA partition's capacity (not `Content-Length`) **And** each subsequent chunk is written via `Update.write(data, len)` **And** the upload streams directly to flash with no full-binary RAM buffering

2. **Given** the first byte of the uploaded file is NOT `0xE9` **When** the first chunk is processed **Then** the endpoint returns HTTP 400 with `{ "ok": false, "error": "Not a valid ESP32 firmware image", "code": "INVALID_FIRMWARE" }` **And** no data is written to flash **And** validation completes within 1 second

3. **Given** `Update.write()` returns fewer bytes than expected **When** a write failure is detected **Then** `Update.abort()` is called **And** the current firmware continues running unaffected **And** an error response `{ "ok": false, "error": "Write failed — flash may be worn or corrupted", "code": "WRITE_FAILED" }` is returned

4. **Given** WiFi drops mid-upload **When** the connection is interrupted **Then** `Update.abort()` is called via `request->onDisconnect()` **And** the inactive partition is NOT marked bootable **And** the device continues running current firmware

5. **Given** the upload completes successfully **When** `Update.end(true)` returns true on the final chunk **Then** the endpoint returns `{ "ok": true, "message": "Rebooting..." }` **And** `ESP.restart()` is called after a 500ms delay using `esp_timer_start_once()` **And** the device reboots into the newly written firmware

6. **Given** a 1.5MB firmware binary uploaded over local WiFi **When** the transfer completes **Then** the total upload time is under 30 seconds

## Tasks / Subtasks

- [x] **Task 1: Add OTA upload state management struct** (AC: #1, #4)
  - [x] Create `OTAUploadState` struct similar to `LogoUploadState` with fields: `request`, `valid`, `started`, `bytesWritten`, `error`, `errorCode`
  - [x] Add global `std::vector<OTAUploadState> g_otaUploads` and helper functions `findOTAUpload()`, `clearOTAUpload()`
  - [x] Helper must call `Update.abort()` if `started` is true when cleaning up

- [x] **Task 2: Register POST /api/ota/upload route** (AC: #1, #2, #3, #4, #5)
  - [x] Add route using same pattern as `POST /api/logos` (lines 256-352 in WebPortal.cpp)
  - [x] Use upload handler signature: `(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final)`
  - [x] Register `request->onDisconnect()` to call `clearOTAUpload(request)` for WiFi interruption handling

- [x] **Task 3: Implement first-chunk validation and Update.begin()** (AC: #1, #2)
  - [x] On `index == 0`: check `data[0] == 0xE9` magic byte
  - [x] If invalid: set `state.valid = false`, `state.error = "Not a valid ESP32 firmware image"`, `state.errorCode = "INVALID_FIRMWARE"`, return early
  - [x] If valid: get next OTA partition via `esp_ota_get_next_update_partition(NULL)`
  - [x] Call `Update.begin(partition->size)` (NOT Content-Length, per AR18)
  - [x] Set `state.started = true` after successful `Update.begin()`
  - [x] Log partition info: `Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size)`

- [x] **Task 4: Implement streaming write per chunk** (AC: #1, #3)
  - [x] For each chunk (including first after validation): call `Update.write(data, len)`
  - [x] Check return value equals `len`; if not, set `state.valid = false`, `state.error = "Write failed — flash may be worn or corrupted"`, `state.errorCode = "WRITE_FAILED"`
  - [x] Call `Update.abort()` immediately on write failure
  - [x] Update `state.bytesWritten += len` for debugging/logging

- [x] **Task 5: Implement final chunk handling and reboot** (AC: #5)
  - [x] On `final == true`: call `Update.end(true)`
  - [x] If `Update.end()` returns false: set error `"Firmware verification failed"`, code `"VERIFY_FAILED"`, call `Update.abort()`
  - [x] If success: state remains valid for request handler

- [x] **Task 6: Implement request handler for response** (AC: #2, #3, #5)
  - [x] Check `state->valid` — if false, return `{ "ok": false, "error": state->error, "code": state->errorCode }` with HTTP 400
  - [x] If valid: return `{ "ok": true, "message": "Rebooting..." }` with HTTP 200
  - [x] Schedule reboot using existing `esp_timer_start_once()` pattern (see lines 491-498 in WebPortal.cpp)
  - [x] Use 500ms delay (500000 microseconds) to allow response to be sent
  - [x] Call `clearOTAUpload(request)` after sending response

- [x] **Task 7: Add OTA subsystem status updates** (AC: #3, #5)
  - [x] On successful upload start: `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Upload in progress")`
  - [x] On write failure: `SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Write failed")`
  - [x] On verification failure: `SystemStatus::set(Subsystem::OTA, StatusLevel::ERROR, "Verification failed")`
  - [x] On success before reboot: `SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Update complete — rebooting")`

- [x] **Task 8: Add WebPortal header declarations** (AC: #1-#5)
  - [x] Add `_handleOTAUpload()` declaration in WebPortal.h (or keep inline like logo upload)
  - [x] Add `static void` helper for `_handleOTAUploadChunk()` if refactoring for readability

- [x] **Task 9: Add #include <Update.h>** (AC: #1, #3, #5)
  - [x] Add `#include <Update.h>` to WebPortal.cpp (ESP32 Arduino core library)
  - [x] Verify it compiles — Update.h is part of ESP32 Arduino core, no lib_deps needed

- [x] **Task 10: Build and test** (AC: #1-#6)
  - [x] Run `pio run` — verify clean build with no errors
  - [x] Verify binary size remains under 1.5MB limit (current: ~1.21MB = 76.9%)
  - [x] Manual test: upload invalid file (wrong magic byte) — expect 400 error < 1 second
  - [x] Manual test: upload valid firmware — expect success, reboot
  - [x] Manual test: disconnect WiFi mid-upload — verify device continues running

## Dev Notes

### Critical Architecture Constraints

**ESP32 Magic Byte Validation (HARD REQUIREMENT per AR-E12):**
ESP32 firmware binaries start with byte `0xE9`. Validate on first chunk BEFORE calling `Update.begin()`:
```cpp
if (index == 0 && len > 0 && data[0] != 0xE9) {
    state.valid = false;
    state.error = "Not a valid ESP32 firmware image";
    state.errorCode = "INVALID_FIRMWARE";
    return; // Early exit — no flash write
}
```

**Update.begin() Size Parameter (CRITICAL per AR18):**
Multipart `Content-Length` is the BODY size, NOT the file size. Do NOT use `total` from the upload handler. Instead, use the OTA partition capacity:
```cpp
const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
if (!partition) {
    state.valid = false;
    state.error = "No OTA partition available";
    state.errorCode = "NO_OTA_PARTITION";
    return;
}
if (!Update.begin(partition->size)) {
    state.valid = false;
    state.error = "Could not begin OTA update";
    state.errorCode = "BEGIN_FAILED";
    return;
}
```

**Streaming Architecture (HARD REQUIREMENT per AR-E13, NFR-C4):**
- Stream directly to flash via `Update.write()` per chunk
- NEVER buffer the full binary in RAM
- Each chunk is typically 4KB-16KB from ESPAsyncWebServer
- Total RAM impact: ~50-100 bytes for state struct only

**Abort on Any Error (CRITICAL):**
After `Update.begin()` is called, ANY error MUST call `Update.abort()`:
```cpp
if (writtenBytes != len) {
    Update.abort();  // MUST call before returning error
    state.valid = false;
    // ... set error
}
```

**Disconnect Handling Pattern:**
Follow the existing pattern from logo upload (WebPortal.cpp line 307):
```cpp
request->onDisconnect([request]() {
    auto* state = findOTAUpload(request);
    if (state && state->started) {
        Update.abort();  // Critical: abort in-progress update
    }
    clearOTAUpload(request);
});
```

**Reboot Timer Pattern:**
Use the existing `esp_timer_start_once()` pattern (WebPortal.cpp lines 491-498):
```cpp
static esp_timer_handle_t otaRebootTimer = nullptr;
if (!otaRebootTimer) {
    esp_timer_create_args_t args = {};
    args.callback = [](void*) { ESP.restart(); };
    args.name = "ota_reboot";
    esp_timer_create(&args, &otaRebootTimer);
}
esp_timer_start_once(otaRebootTimer, 500000); // 500ms in microseconds
```

### Response Format (Follow JSON Envelope Pattern)

**Success Response:**
```json
{
  "ok": true,
  "message": "Rebooting..."
}
```

**Error Responses:**
```json
{
  "ok": false,
  "error": "Not a valid ESP32 firmware image",
  "code": "INVALID_FIRMWARE"
}
```

Error codes:
| Code | Trigger |
|------|---------|
| `INVALID_FIRMWARE` | First byte != 0xE9 |
| `NO_OTA_PARTITION` | `esp_ota_get_next_update_partition()` returns NULL |
| `BEGIN_FAILED` | `Update.begin()` returns false |
| `WRITE_FAILED` | `Update.write()` returns fewer bytes than expected |
| `VERIFY_FAILED` | `Update.end(true)` returns false |
| `UPLOAD_ERROR` | Generic fallback for unexpected failures |

### OTAUploadState Struct

```cpp
struct OTAUploadState {
    AsyncWebServerRequest* request;
    bool valid;           // false if any validation/write failed
    bool started;         // true after Update.begin() succeeds
    size_t bytesWritten;  // for debugging/logging only
    String error;         // human-readable error message
    String errorCode;     // machine-readable error code
};
```

### ESP32 Update.h Library Reference

The `Update` class is a singleton from ESP32 Arduino core (`#include <Update.h>`):

| Method | Purpose |
|--------|---------|
| `Update.begin(size_t size)` | Start update with expected size (use partition size) |
| `Update.write(uint8_t* data, size_t len)` | Write chunk to flash, returns bytes written |
| `Update.end(bool evenIfRemaining)` | Finalize update, `true` = accept even if not all bytes written |
| `Update.abort()` | Cancel in-progress update, mark partition invalid |
| `Update.hasError()` | Check if error occurred |
| `Update.getError()` | Get error code |
| `Update.errorString()` | Get human-readable error string |

**Update library behavior:**
- `Update.begin()` erases flash blocks as needed (first call may take 2-3 seconds)
- `Update.write()` writes in 4KB flash block sizes internally
- `Update.end(true)` sets the boot partition to the newly written firmware
- After `Update.end(true)`, the next reboot loads the new firmware
- If new firmware crashes before self-check, bootloader rolls back automatically (Story fn-1.4)

Sources:
- [ESP32 Forum - OTA and Update.h](https://www.esp32.com/viewtopic.php?t=16512)
- [GitHub - arduino-esp32 Update examples](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/HTTPS_OTA_Update/HTTPS_OTA_Update.ino)
- [Last Minute Engineers - ESP32 OTA Web Updater](https://lastminuteengineers.com/esp32-ota-web-updater-arduino-ide/)

### Performance Requirements (NFR-P1, NFR-P7)

| Metric | Target |
|--------|--------|
| 1.5MB upload time | < 30 seconds over local WiFi |
| Invalid file rejection | < 1 second (fail on first chunk) |

The ESPAsyncWebServer receives chunks of 4-16KB depending on network conditions. At typical 10KB/chunk, a 1.5MB file arrives in ~150 chunks. Flash write speed is ~100-400KB/s depending on ESP32 flash quality.

### Partition Layout Reference

From `firmware/custom_partitions.csv` (Story fn-1.1):
```
# Name,   Type, SubType, Offset,    Size,    Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xE000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x180000,
app1,     app,  ota_1,   0x190000, 0x180000,
spiffs,   data, spiffs,  0x310000, 0xF0000,
```

- **app0**: 0x10000 to 0x18FFFF (1.5MB) — first OTA partition
- **app1**: 0x190000 to 0x30FFFF (1.5MB) — second OTA partition
- Running partition determined by `esp_ota_get_running_partition()`
- Next partition determined by `esp_ota_get_next_update_partition(NULL)`

### Project Structure Notes

**Files to modify:**
1. `firmware/adapters/WebPortal.cpp` — Add POST /api/ota/upload route + upload handler
2. `firmware/adapters/WebPortal.h` — Optional: add handler declaration if not using inline lambdas

**Files NOT to modify (handled by other stories):**
- `main.cpp` — Self-check logic is Story fn-1.4
- `dashboard.js` — UI is Story fn-1.6
- `ConfigManager.cpp` — Already has OTA subsystem from Story fn-1.2

**Code location guidance:**
- Insert new route registration after line 352 (after `POST /api/logos`) in `_registerRoutes()`
- Follow the exact pattern of logo upload handler for consistency
- Place `OTAUploadState` struct and helpers in the anonymous namespace (lines 38-98)

### Testing Strategy

**Automated build verification:**
```bash
cd firmware && pio run
# Expect: SUCCESS, binary < 1.5MB
```

**Manual test cases:**
1. **Invalid file test**: Upload a `.txt` file or random binary without 0xE9 header
   - Expect: HTTP 400, error message, device continues running
   - Verify: Response time < 1 second

2. **Valid firmware test**: Upload a valid `.bin` from `pio run` output
   - Expect: HTTP 200, "Rebooting..." message, device reboots in ~500ms
   - Verify: New firmware runs (check FW_VERSION in Serial output)

3. **Disconnect test**: Start upload, disconnect WiFi mid-transfer
   - Expect: Device continues running current firmware
   - Verify: No corruption, no crash

4. **Oversized file test**: Upload a file > 1.5MB (if possible to construct)
   - Expect: Upload proceeds but `Update.end()` may fail or fill partition
   - Note: This is an edge case; client-side validation (Story fn-1.6) prevents this

### Previous Story Intelligence (fn-1.2)

**From fn-1.2 completion notes:**
- Binary size: 1,209,440 bytes (1.15MB) — 76.9% of 1.5MB partition
- SystemStatus now has OTA and NTP subsystems ready (`Subsystem::OTA`)
- ConfigManager patterns established for new structs

**Lessons learned from fn-1.1 and fn-1.2:**
1. **Explicit error handling** — Always log and surface errors, never silent failures
2. **Thread safety** — OTA upload runs on AsyncTCP task (same as web server), no mutex needed for Upload singleton
3. **Test on hardware** — Unit tests build-pass but hardware verification essential

### References

- [Source: architecture.md#Decision F2: OTA Handler Architecture — WebPortal + main.cpp]
- [Source: epic-fn-1.md#Story fn-1.3: OTA Upload Endpoint]
- [Source: WebPortal.cpp lines 256-352 for logo upload pattern]
- [Source: WebPortal.cpp lines 491-498 for reboot timer pattern]
- [Source: prd.md#Technical Success criteria for OTA]
- [Source: architecture.md#AR-E12, AR-E13 enforcement rules]

### Dependencies

**This story depends on:**
- fn-1.1 (Partition Table & Build Configuration) — COMPLETE
- fn-1.2 (ConfigManager Expansion) — COMPLETE (provides SystemStatus::OTA)

**Stories that depend on this:**
- fn-1.4: OTA Self-Check & Rollback Detection — uses the firmware written by this endpoint
- fn-1.6: Dashboard Firmware Card & OTA Upload UI — calls this endpoint

### Risk Mitigation

1. **Flash wear concern**: Flash has ~100K write cycles. OTA updates are infrequent. Log write failures with helpful message.
2. **Partial write corruption**: `Update.abort()` marks partition invalid; bootloader ignores it. Safe.
3. **Power loss during write**: ESP32 bootloader handles this; rolls back to previous partition.
4. **Large file RAM exhaustion**: Streaming architecture prevents this. ESPAsyncWebServer chunks are 4-16KB max.

## Dev Agent Record

### Agent Model Used

Claude Opus 4 (Story Creation)

### Debug Log References

N/A — Story creation phase

### Completion Notes List

**2026-04-12: Ultimate context engine analysis completed**
- Comprehensive analysis of epic-fn-1.md extracted all 6 acceptance criteria with BDD format
- WebPortal.cpp patterns analyzed: logo upload (lines 256-352), reboot timer (lines 491-498)
- Existing OTA infrastructure verified: partition table ready, esp_ota_ops.h included in main.cpp
- ESP32 Update.h library research completed via web search
- All architecture requirements mapped: AR3, AR12, AR18, AR-E12, AR-E13
- Performance requirements documented: NFR-P1 (30s upload), NFR-P7 (1s validation)
- 10 tasks created with explicit code references and line numbers

**2026-04-12: Implementation completed**
- Added `OTAUploadState` struct in anonymous namespace (lines 83-91)
- Added helper functions `findOTAUpload()` and `clearOTAUpload()` (lines 95-114)
- Implemented `POST /api/ota/upload` route following logo upload pattern (lines 399-531)
- Added `#include <Update.h>` and `#include <esp_ota_ops.h>` (lines 27-28)
- Magic byte validation (0xE9) on first chunk prevents invalid firmware writes
- Partition size used for `Update.begin()` instead of Content-Length per AR18
- WiFi disconnect handling via `request->onDisconnect()` calls `Update.abort()`
- SystemStatus updates for OTA subsystem on all state transitions
- 500ms reboot delay using `esp_timer_start_once()` pattern
- Build successful: 1,216,384 bytes (77.3% of partition)
- All 6 acceptance criteria satisfied

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive developer context | BMad |
| 2026-04-12 | Implementation complete - OTA upload endpoint with streaming, validation, and error handling | Claude |

### File List

- `firmware/adapters/WebPortal.cpp` (MODIFIED) - Added OTA upload route, state struct, helpers


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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review**
Story: [fn-1-3-ota-upload-endpoint.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:2)

**Verdict:** `REJECT`  
**Evidence Score:** `8.0`

## Findings

1. **Critical: the success path can abort the freshly completed OTA image.**  
   `clearOTAUpload()` always calls `Update.abort()` when `started == true`, and the success handler calls `clearOTAUpload(request)` immediately after `Update.end(true)` without ever clearing `started`. That means a successful upload can be invalidated during cleanup, directly violating AC5.  
   Evidence: [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:103), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:419), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:489), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:515)

2. **High: OTA uploads are not single-flight guarded, but `Update` is global mutable state.**  
   The route accepts every POST and tracks uploads per request in `g_otaUploads`, but there is no global “OTA already in progress” rejection. Two clients can race through `Update.begin()/write()/end()`, or one request can abort another’s session. On ESP32 OTA, that is not a safe concurrency model.  
   Evidence: [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:94), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:396)

3. **High: architecture-required size validation is missing.**  
   Foundation rule 12 requires fail-fast validation on size and magic byte before writing. The implementation checks only the magic byte. `bytesWritten` is tracked but never used to reject `bytesWritten + len > partition->size`, so oversized uploads can erase/write flash for most of the transfer before failing late.  
   Evidence: [architecture.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/planning-artifacts/architecture.md:215), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:458), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:510)

4. **High: the endpoint introduces unauthenticated firmware replacement.**  
   `POST /api/ota/upload` is registered with no auth, no CSRF protection, and no capability check. Any host that can reach the device on the LAN can attempt to replace firmware. For the highest-privilege endpoint in the system, “local network only” is not a security control.  
   Evidence: [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:395)

5. **High: Task 10 is marked complete, but there is no repo evidence of OTA test coverage.**  
   The story claims build verification and manual testing are complete, yet the story’s file list shows only `WebPortal.cpp` changed and there is no new coverage under `firmware/test/` or `tests/`. ACs 2-5 are therefore unprotected against regression.  
   Evidence: [fn-1-3-ota-upload-endpoint.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:78)

6. **Important: early OTA startup failures never update `SystemStatus`.**  
   Invalid magic byte, missing OTA partition, and `Update.begin()` failure only set request-local errors. `SystemStatus` is updated for write failure, verify failure, and success, but not for the earlier failure modes, so `/api/status` can report stale OTA health after a rejected upload.  
   Evidence: [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:459), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:470), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:480), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:491)

7. **Important: the code returns “Rebooting...” without checking whether reboot scheduling succeeded.**  
   Both `esp_timer_create()` and `esp_timer_start_once()` ignore their return codes. If either fails, the API still returns success, but the device may never reboot into the new slot until an external restart. That is an AC5 gap.  
   Evidence: [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:427)

## AC Audit

- **AC1:** `PARTIAL`  
  Streaming and `Update.begin(partition->size)` are present, but the required fail-fast size validation is missing.
- **AC2:** `IMPLEMENTED`
- **AC3:** `IMPLEMENTED`
- **AC4:** `PARTIAL`  
  `onDisconnect()` exists, but the cleanup/abort logic is unsafe enough to also jeopardize completed uploads.
- **AC5:** `PARTIAL`  
  Finalization is followed by cleanup that can abort the update, and reboot scheduling is unchecked.
- **AC6:** `UNVERIFIED`  
  No codebase evidence of performance measurement or regression coverage.

## Task Audit

- **Task 3:** marked complete, but the requested partition label/size logging is missing.  
  Evidence: [fn-1-3-ota-upload-endpoint.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:44), [WebPortal.cpp](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/adapters/WebPortal.cpp:468)
- **Task 10:** marked complete, but there is no product-test artifact in the repo to support the claim.  
  Evidence: [fn-1-3-ota-upload-endpoint.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:78)

## Evidence Score

| Finding | Severity | Score |
|---|---:|---:|
| Success cleanup aborts completed OTA | 🔴 Critical | +3.0 |
| No single-flight OTA guard | 🟠 Important | +1.0 |
| Missing size validation | 🟠 Important | +1.0 |
| Unauthenticated firmware upload | 🟠 Important | +1.0 |
| No test coverage despite completion claim | 🟠 Important | +1.0 |
| Missing `SystemStatus` on early failures | 🟠 Important | +1.0 |
| Reboot timer errors ignored | 🟠 Important | +1.0 |
| Clean passes: style, type safety | 🟢 Clean | -1.0 |

**Total:** `8.0`  
**Verdict:** `REJECT`

## Recommended Fix Order

1. Fix OTA state cleanup so success never calls `Update.abort()`.
2. Add a global OTA-in-progress guard and reject concurrent uploads.
3. Enforce partition-capacity bounds during streaming, not only at `Update.end()`.
4. Add at least one automated OTA-path test harness for invalid magic, write failure, and disconnect cleanup.
5. Gate the endpoint behind an explicit admin authorization mechanism.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 2 |
| Security Vulnerabilities | 2 |
| **Total Issues** | **12** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Missing Content-Length validation allows oversized uploads | WebPortal.cpp:399 | +3 |
| 🔴 CRITICAL | No protection against concurrent OTA updates | WebPortal.cpp:93 | +3 |
| 🟠 IMPORTANT | Missing Update.isRunning() check enables re-entrant updates | WebPortal.cpp:484 | +1 |
| 🟠 IMPORTANT | Missing partition info logging per Task 3 requirement | WebPortal.cpp:483-496 | +1 |
| 🟠 IMPORTANT | Generic HTTP 400 for all errors - no semantic distinction | WebPortal.cpp:407-415 | +1 |
| 🟠 IMPORTANT | No rate limiting on OTA endpoint | WebPortal.cpp:399 | +1 |
| 🟡 MINOR | Task 8 claim inaccurate - WebPortal.h unchanged | WebPortal.h:1-44 | +0.3 |
| 🟡 MINOR | Lambda captures `this` in timer callback | WebPortal.cpp:433-436 | +0.3 |
| 🟡 MINOR | OTAUploadState stored even on validation failure | WebPortal.cpp:467 | +0.3 |
| 🟡 MINOR | No protection against partial chunk replay | WebPortal.cpp:441 | +0.3 |
| 🟡 MINOR | Missing validation of partition->size | WebPortal.cpp:484 | +0.3 |
| 🟡 MINOR | Static timer handle never deleted | WebPortal.cpp:431 | +0.3 |

### Clean Pass Categories: 0

### Evidence Score: 11.9

| Score | Verdict |
|-------|---------|
| **11.9** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[7/10] Single Responsibility Violation:** WebPortal handles OTA state management, upload streaming, and reboot scheduling. The OTA logic should be extracted to a dedicated OTAHandler class.
  - 📍 `WebPortal.cpp:83-531`
  - 💡 Fix: Create `OTAHandler` class in `adapters/OTAHandler.h/cpp`

- **Over-Engineering:** Using `std::vector<OTAUploadState>` for what should be a single atomic upload. ESP32 can only have ONE active OTA at a time.
  - 📍 `WebPortal.cpp:93`

---

## 🐍 Code Quality Issues

- **Missing Size Validation:** No `request->contentLength()` check before accepting upload. A 100MB upload would be accepted and fail only when flash is full.
  - 📍 `WebPortal.cpp:399-531`
  
- **Missing Running Check:** No `Update.isRunning()` validation before `Update.begin()`. Two simultaneous uploads from different clients could corrupt the update partition.
  - 📍 `WebPortal.cpp:484`

- **Generic Error Handling:** All errors return HTTP 400, making monitoring impossible. Verification failures should be 500, invalid firmware should be 400.
  - 📍 `WebPortal.cpp:407-415`

---

## ⚡ Performance & Security

- **[HIGH] No Rate Limiting:** OTA endpoint can be spammed indefinitely, causing flash wear.
  - 📍 `WebPortal.cpp:399`
  - 💡 Fix: Add last-OTA-time check and minimum interval (e.g., 60 seconds)

- **[HIGH] Unbounded Vector Growth:** `g_otaUploads` has no size limit. In a DoS scenario with many concurrent connections, this exhausts heap.
  - 📍 `WebPortal.cpp:93`
  - 💡 Fix: `if (g_otaUploads.size() >= 1) reject;`

- **[MEDIUM] Missing Partition Size Validation:** `partition->size` is used without sanity checking. If partition table is corrupted, this could overflow.
  - 📍 `WebPortal.cpp:484`

---

## 🐛 Correctness & Safety

- **🐛 Task Completion Lie:** Task 3 claims "Log partition info" but no `Serial.printf` for partition info exists in the code.
  - 📍 Story line 43 vs Implementation line 483-496
  - 🔄 Reproduction: Search for "Writing to" or partition label logging - NOT FOUND

- **🐛 Task 8 False Claim:** Task 8 says "Add WebPortal header declarations" marked complete, but `WebPortal.h` has no OTA-related declarations.
  - 📍 `WebPortal.h:1-44`
  - 🔄 Reproduction: `grep -i ota firmware/adapters/WebPortal.h` returns nothing

- **🐛 State Stored on Validation Failure:** When magic byte fails, state is pushed to vector (line 467) even though upload never started. This wastes memory.
  - 📍 `WebPortal.cpp:467`

- **🎭 Lying Test:** Task 10 claims "Manual test: upload invalid file" completed but no automated tests exist for the OTA endpoint. Story mentions no unit tests for OTA logic.
  - Story lines 77-82

---

## 🔧 Maintainability Issues

- **💣 Magic Numbers:** `500000` (500ms delay) and `0xE9` magic byte are hardcoded without named constants.
  - 📍 `WebPortal.cpp:438, 463`
  - 💥 Explosion radius: Every developer must know these values; typos cause silent failures

- **💣 Static Timer Never Cleaned:** `otaRebootTimer` is created once but never deleted. On repeated OTA attempts, this is a minor leak.
  - 📍 `WebPortal.cpp:431-436`

- **💣 Lambda `this` Capture:** Timer callback captures `this` unnecessarily since it only calls `ESP.restart()`.
  - 📍 `WebPortal.cpp:434`

---

## AC Implementation Audit

| AC | Status | Notes |
|----|--------|-------|
| AC #1 | ✅ IMPLEMENTED | Streaming upload correctly uses partition size |
| AC #2 | ⚠️ PARTIAL | Returns correct error but missing <1s timing guarantee |
| AC #3 | ✅ IMPLEMENTED | Update.abort() called on write failure |
| AC #4 | ✅ IMPLEMENTED | onDisconnect handler present |
| AC #5 | ✅ IMPLEMENTED | 500ms delay and esp_timer used |
| AC #6 | ❌ NOT VERIFIED | No code ensures <30s upload time |

---

## Suggested Fixes

### 1. Add Content-Length Validation

**File:** `firmware/adapters/WebPortal.cpp`

Add after line 443:
```cpp
if (index == 0) {
    const size_t MAX_FIRMWARE_SIZE = 0x180000; // 1.5MB per partition table
    if (request->contentLength() > MAX_FIRMWARE_SIZE) {
        _sendJsonError(request, 413, "Firmware too large", "OVERSIZED_FIRMWARE");
        return;
    }
    // ... rest of initialization
}
```

### 2. Add Concurrent Upload Protection

**File:** `firmware/adapters/WebPortal.cpp`

Add after line 443:
```cpp
if (index == 0) {
    if (!g_otaUploads.empty() || Update.isRunning()) {
        _sendJsonError(request, 503, "OTA already in progress", "OTA_BUSY");
        return;
    }
    // ... rest of initialization
}
```

### 3. Fix Task Completion Documentation

**File:** Story file should be updated:
- Task 3: Remove "Log partition info" subtask OR add logging to code
- Task 8: Mark as "N/A - inline lambdas used" not completed

### 4. Add Rate Limiting

**File:** `firmware/adapters/WebPortal.cpp`

Add static variable:
```cpp
static unsigned long lastOTATime = 0;
if (index == 0 && millis() - lastOTATime < 60000) {
    _sendJsonError(request, 429, "OTA rate limited", "RATE_LIMITED");
    return;
}
```

---

## Review Actions Summary

- Issues Found: 12
- Issues Fixed: 0
- Action Items Created: 4

⚠️ Address the identified issues before proceeding. The implementation is functionally correct but lacks hardening against edge cases and abuse scenarios critical for an OTA endpoint.

]]></file>
<file id="f63a150c" path="[Validator D]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 2 |
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **9** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Lying Claim: AC6 (1.5MB upload < 30s) is marked complete without runtime verification. | _bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md | +3 |
| 🔴 CRITICAL | Lying Claim: Task10 (Build and test) marked complete without verifiable execution of manual tests. | _bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md | +3 |
| 🔴 CRITICAL | Hidden Bug: Dangling `request` pointer in `onDisconnect` callbacks due to incorrect lambda capture. | firmware/adapters/WebPortal.cpp:456 | +3 |
| 🟠 IMPORTANT | SOLID Violation: Large `_registerRoutes` method and inline handlers violate SRP/OCP. | firmware/adapters/WebPortal.cpp:191 | +1 |
| 🟠 IMPORTANT | Security Vulnerability: Lack of server-side total size validation for uploaded firmware against partition capacity. | firmware/adapters/WebPortal.cpp:451 | +1 |
| 🟡 MINOR | Abstraction Issue: Repeated `JsonDocument` allocation in `_sendJsonError` and other response handlers. | firmware/adapters/WebPortal.cpp:752 | +0.3 |
| 🟡 MINOR | Performance Footgun: String concatenations for error messages can cause heap fragmentation. | firmware/adapters/WebPortal.cpp:384 | +0.3 |
| 🟡 MINOR | Tech Debt Bomb: Magic number `500000` for reboot delay. | firmware/adapters/WebPortal.cpp:444 | +0.3 |

### Evidence Score: 11.6

| Score | Verdict |
|-------|---------|
| **11.6** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[6/10] Single Responsibility:** The `_registerRoutes` method is excessively long and contains the full implementation logic for multiple API endpoints, including the OTA upload handler.
  - 📍 `firmware/adapters/WebPortal.cpp:191`
  - 💡 Fix: Extract specific API handler logic into dedicated private methods (e.g., `_handleOtaUploadRequest`, `_handleOtaUploadChunk`, `_handlePostLogosRequest`, etc.) to improve modularity, readability, and adherence to SRP.

## 🐍 Pythonic Crimes &amp; Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance &amp; Scalability

- **[Minor] Heap Fragmentation:** String concatenations are used for constructing error messages within the `LogoUploadState` handler (and conceptually elsewhere). While error paths are not hot paths, dynamic string allocations and deallocations can lead to heap fragmentation in an embedded system with limited memory.
  - 📍 `firmware/adapters/WebPortal.cpp:384`
  - 💡 Fix: Use `snprintf` with a fixed-size buffer for error message construction, or pre-allocate `String` objects if heap usage patterns are predictable.

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug:** Dangling `AsyncWebServerRequest` pointer in `request->onDisconnect()` lambda. The `onDisconnect` callback for `ESPAsyncWebServer` expects a lambda that takes `AsyncWebServerRequest*` as an argument (`std::function<void(AsyncWebServerRequest *request)>`). The current implementation captures the `request` pointer from the `_server->on` context, which might be a different or invalidated pointer when the `onDisconnect` callback is actually triggered. This can lead to undefined behavior, including crashes, if the captured `request` is dereferenced after its lifetime has ended.
  - 📍 `firmware/adapters/WebPortal.cpp:456`
  - 🔄 Reproduction: Initiate an OTA upload, then abruptly disconnect the client (e.g., disable WiFi on the client device). The `onDisconnect` callback may attempt to access an invalid `AsyncWebServerRequest` object.
- **🔒 [Important] Security:** Lack of server-side file size validation for OTA uploads. Although the `Update.begin()` uses the available partition size, there's no explicit check *before* `Update.begin()` that the total incoming data size (e.g., from `Content-Length` header or the `total` parameter of the upload handler) does not exceed the target partition's capacity. A malicious client could send an extremely large `Content-Length` or `total` value, leading to prolonged flash erase operations and potential denial-of-service, even if `Update.write()` fails later.
  - 📍 `firmware/adapters/WebPortal.cpp:451`
  - ⚠️ Impact: Denial-of-service due to unnecessary flash wear or extended resource consumption.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Magic number used for reboot delay. The value `500000` (microseconds) represents a 500ms delay for the OTA reboot. This is a magic number that reduces readability and maintainability.
  - 📍 `firmware/adapters/WebPortal.cpp:444`
  - 💥 Explosion radius: Readability and potential for error if the value needs to be changed and isn't updated consistently.

---

## 🛠️ Suggested Fixes

### 1. Refactor `onDisconnect` Lambda Capture

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** The `request->onDisconnect` lambda captures the `request` pointer from the outer scope, which can lead to a dangling pointer issue if the original `AsyncWebServerRequest` object is destroyed before the `onDisconnect` callback is invoked.

**Diff:**
```diff
--- a/firmware/adapters/WebPortal.cpp
+++ b/firmware/adapters/WebPortal.cpp
@@ -453,13 +453,13 @@
                 state.bytesWritten = 0;
 
                 // Register disconnect handler for WiFi interruption
-                request->onDisconnect([request]() {
-                    auto* s = findOTAUpload(request);
+                request->onDisconnect([this](AsyncWebServerRequest *disconnectRequest) {
+                    auto* s = findOTAUpload(disconnectRequest);
                     if (s && s->started) {
                         Update.abort();
                         LOG_I("OTA", "Upload aborted due to disconnect");
                     }
-                    clearOTAUpload(request);
+                    clearOTAUpload(disconnectRequest);
                 });
 
                 // Validate ESP32 magic byte (0xE9)
```

### 2. Add Server-Side Total Size Validation for OTA Uploads

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** No explicit check for the total incoming data size against the OTA partition capacity before initiating the update. This can lead to inefficient flash operations or potential denial-of-service.

**Diff:**
```diff
--- a/firmware/adapters/WebPortal.cpp
+++ b/firmware/adapters/WebPortal.cpp
@@ -467,6 +467,16 @@
                     return;
                 });
 
+                // Check total size against partition size (basic server-side validation)
+                // Note: 'total' here is the Content-Length of the body, which should be the firmware size.
+                if (total > partition->size) {
+                    state.valid = false;
+                    state.error = "Firmware image too large for partition";
+                    state.errorCode = "IMAGE_TOO_LARGE";
+                    g_otaUploads.push_back(state);
+                    LOG_E("OTA", "Firmware too large: %u bytes, partition size: %u bytes", total, partition->size);
+                    return;
+                }
+
                 // Validate ESP32 magic byte (0xE9)
                 if (len == 0 || data[0] != 0xE9) {
                     state.valid = false;
```

### 3. Use Named Constant for OTA Reboot Delay

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** The reboot delay for OTA is a magic number, affecting readability and maintainability.

**Diff:**
```diff
--- a/firmware/adapters/WebPortal.cpp
+++ b/firmware/adapters/WebPortal.cpp
@@ -441,7 +441,9 @@
         args.name = "ota_reboot";
         esp_timer_create(&args, &otaRebootTimer);
     }
-    esp_timer_start_once(otaRebootTimer, 500000); // 500ms in microseconds
+
+    static constexpr unsigned long OTA_REBOOT_DELAY_US = 500000; // 500ms
+    esp_timer_start_once(otaRebootTimer, OTA_REBOOT_DELAY_US);
         },
         // Upload handler — called for each chunk of firmware data
         [this](AsyncWebServerRequest* request, const String& filename,
```

## Review Actions Summary

- Issues Found: 9
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

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
<var name="session_id">b20cdacc-8390-46fa-aa43-733b10877133</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="ab213a12">embedded in prompt, file id: ab213a12</var>
<var name="story_id">fn-1.3</var>
<var name="story_key">fn-1-3-ota-upload-endpoint</var>
<var name="story_num">3</var>
<var name="story_title">3-ota-upload-endpoint</var>
<var name="template">False</var>
<var name="timestamp">20260412_1316</var>
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
      - Commit message format: fix(component): brief description (synthesis-fn-1.3)
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