<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 3 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260412T170728Z -->
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

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **6** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟡 MINOR | Incomplete File List in Story Documentation | `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md`, `firmware/adapters/WebPortal.cpp` | +0.3 |
| 🟠 IMPORTANT | Delayed Error Response for Invalid Firmware | `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md` (AC 2), `firmware/adapters/WebPortal.cpp` (lines 405-412, 432-441) | +1 |
| 🟠 IMPORTANT | Unverified Performance NFR | `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md` (AC 6, Completion Notes List), `firmware/adapters/WebPortal.cpp` | +1 |
| 🟠 IMPORTANT | Single Responsibility Principle Violation - Overloaded WebPortal | `firmware/adapters/WebPortal.cpp` (lines 142-595, specifically the `POST` route lambdas) | +1 |
| 🟡 MINOR | Potential Race Condition with Global `g_otaUploads` Vector | `firmware/adapters/WebPortal.cpp` (lines 89, 91-95, 97-107, 467) | +0.3 |
| 🟠 IMPORTANT | Insufficient Unit Test Coverage for Critical Logic | `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md` (Testing Expectations), `firmware/adapters/WebPortal.cpp` (lines 399-531) | +1 |
| 🟡 MINOR | Magic Number for Reboot Delay | `firmware/adapters/WebPortal.cpp` (line 513) | +0.3 |
| 🟠 IMPORTANT | Duplicate Reboot Timer Initialization Logic | `firmware/adapters/WebPortal.cpp` (lines 491-498, 504-511, 561-568) | +1 |
| 🟠 IMPORTANT | Missing Log Output as per Task Specification | `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md` (Task 3), `firmware/adapters/WebPortal.cpp` (lines 415-468) | +1 |
| 🟢 CLEAN PASS | 4 |

### Evidence Score: 4.9

| Score | Verdict |
|-------|---------|
| **4.9** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[6/10] Single Responsibility:** The `WebPortal` class and especially its `_registerRoutes()` method (lines 142-595) exhibit a violation of the Single Responsibility Principle. The class is responsible for too many distinct concerns: serving static assets, handling settings, managing WiFi, handling calibration, listing/uploading/deleting logos, and now handling OTA firmware uploads. The anonymous lambdas used for `POST /api/settings`, `POST /api/logos`, and `POST /api/ota/upload` encapsulate complex logic (validation, state management, file I/O, flash updates, error handling) directly within the route registration. This makes the `WebPortal` a monolithic class with tightly coupled responsibilities, hindering maintainability, testability, and extensibility. Extracting this logic into separate, dedicated handler functions or classes would improve modularity.
  - 📍 `firmware/adapters/WebPortal.cpp:142`
  - 💡 Fix: Refactor large lambda functions into distinct private methods or dedicated handler classes to separate concerns.

---

## 🐍 Pythonic Crimes &amp; Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance &amp; Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness &amp; Safety

- **🐛 Bug:** Delayed Error Response for Invalid Firmware
  - 📍 `firmware/adapters/WebPortal.cpp:405`
  - 🔄 Reproduction: Upload a file with a first byte other than `0xE9` to `/api/ota/upload`. The client will continue uploading data until the entire file is sent or a timeout occurs, before receiving the HTTP 400 error response.
- **🐛 Bug:** Unverified Performance NFR
  - 📍 `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:AC 6`
  - 🔄 Reproduction: N/A (requires actual performance testing)
- **🐛 Bug:** Potential Race Condition with Global `g_otaUploads` Vector
  - 📍 `firmware/adapters/WebPortal.cpp:89`
  - 🔄 Reproduction: Attempt multiple simultaneous OTA uploads from different clients, or trigger `onDisconnect` callbacks while `g_otaUploads` is being modified by another request handler. This could lead to vector corruption or incorrect state management due to lack of synchronization.
- **🐛 Bug:** Insufficient Unit Test Coverage for Critical Logic
  - 📍 `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:Testing Expectations`
  - 🔄 Reproduction: N/A (absence of tests)

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Magic Number for Reboot Delay
  - 📍 `firmware/adapters/WebPortal.cpp:513`
  - 💥 Explosion radius: Makes the purpose of the number unclear and prone to inconsistent changes across the codebase.
- **💣 Tech Debt:** Duplicate Reboot Timer Initialization Logic
  - 📍 `firmware/adapters/WebPortal.cpp:491`
  - 💥 Explosion radius: Increases code size, makes future changes to the timer setup error-prone, and violates the DRY principle.

---

## 🛠️ Suggested Fixes

### 1. Missing Log Output as per Task Specification

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** Task 3 explicitly required logging of partition info using `Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size)`, but only a generic `LOG_I("OTA", "Update started");` was implemented.

**Corrected code:**
```cpp
                if (!Update.begin(partition->size)) {
                    state.valid = false;
                    state.error = "Could not begin OTA update";
                    state.errorCode = "BEGIN_FAILED";
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "Update.begin() failed");
                    return;
                }

                state.started = true;
                g_otaUploads.push_back(state);
                SystemStatus::set(Subsystem::OTA, StatusLevel::OK, "Upload in progress");
                LOG_I("OTA", "Update started");
                // ADDED: Detailed logging of partition info
                Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size);
            }
```

### 2. Delayed Error Response for Invalid Firmware

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** When the magic byte `0xE9` validation fails, the `upload handler` only sets a flag and returns early. The HTTP 400 error response is sent much later by the `request handler`, violating the "validation completes within 1 second" NFR.

**Corrected code:**
```cpp
                // Validate ESP32 magic byte (0xE9)
                if (len == 0 || data[0] != 0xE9) {
                    state.valid = false;
                    state.error = "Not a valid ESP32 firmware image";
                    state.errorCode = "INVALID_FIRMWARE";
                    g_otaUploads.push_back(state);
                    LOG_E("OTA", "Invalid firmware magic byte");
                    // ADDED: Immediately send error response and disconnect the request
                    clearOTAUpload(request); // Clean up any state
                    request->send(400, "application/json", "{\"ok\":false,\"error\":\"Not a valid ESP32 firmware image\",\"code\":\"INVALID_FIRMWARE\"}");
                    return;
                }
```

### 3. Duplicate Reboot Timer Initialization Logic

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** The `esp_timer_handle_t` initialization logic for scheduling reboots is duplicated three times.

**Suggested Fix:**
Refactor the timer creation into a private helper function or a static member function that ensures only one timer is created and reused. This would centralize the timer management and adhere to the DRY principle.

### 4. Magic Number for Reboot Delay

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** The hardcoded `500000` (microseconds) for reboot delay is a magic number.

**Suggested Fix:**
Define a `const uint32_t OTA_REBOOT_DELAY_US = 500000;` at the top of `WebPortal.cpp` or in a common header if it's used elsewhere, and use this named constant instead of the literal value.

---

## Review Actions Summary

- Issues Found: 9
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial Code Review: `fn-1.3` OTA Upload Endpoint**

Scope note: findings below are based on the provided story artifact and `firmware/adapters/WebPortal.cpp` only.

## Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 2 |
| SOLID Violations | 1 |
| Hidden Bugs | 3 |
| Performance Footguns | 2 |
| Security Vulnerabilities | 1 |
| **Total Issues** | **7** |

## Evidence Score Summary

| Severity | Finding | Source | Score |
|----------|---------|--------|-------|
| CRITICAL | Invalid firmware and mid-stream write failures are not failed fast; the error response is deferred until the framework finishes the multipart request, which breaks AC2/AC3 timing and abort semantics. | `firmware/adapters/WebPortal.cpp:399-414`, `450-529` | +3 |
| CRITICAL | Multiple OTA uploads can run concurrently against the global `Update` singleton with no exclusivity guard, so requests can corrupt or abort each other. | `firmware/adapters/WebPortal.cpp:93-101`, `434-529` | +3 |
| CRITICAL | Story Task 3 claims partition label/size logging was implemented, but the code never logs `partition->label` or `partition->size`. | `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:37-43`, `firmware/adapters/WebPortal.cpp:490-494` | +3 |
| IMPORTANT | Task 10 is marked complete, but there is no reviewable test coverage or committed artifact for invalid upload, valid upload, or disconnect scenarios. | `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:77-82` | +1 |
| IMPORTANT | Successful uploads are still cleaned up through `Update.abort()` because `started` is never cleared after `Update.end(true)`. | `firmware/adapters/WebPortal.cpp:102-111`, `421-424`, `523-529` | +1 |
| IMPORTANT | `/api/ota/upload` is completely unauthenticated and CSRF-able; any host on the LAN, or a malicious webpage, can submit firmware. | `firmware/adapters/WebPortal.cpp:399-531` | +1 |
| IMPORTANT | There is no server-side size guard during streaming, despite Foundation Rule 12 requiring validation before writing. Oversize images fail late after flash erase/write work has already started. | `firmware/adapters/WebPortal.cpp:459-490`, `507-521`; `architecture.md` Foundation Rule 12 | +1 |
| CLEAN PASS | Style and type-safety issues were not the problem here. | Review scope | -1 |

### Evidence Score: 12

| Score | Verdict |
|-------|---------|
| **12** | **REJECT** |

## Findings

1. **CRITICAL — fail-fast validation is not actually implemented.**  
   References: `firmware/adapters/WebPortal.cpp:399-414`, `450-529`  
   The upload callback only marks `state.valid = false` and returns. The actual `request->send(400, ...)` happens later in the POST handler, after AsyncWebServer has finished receiving the multipart body. That means an invalid first byte or a flash write failure can still consume the entire upload before the client sees a rejection. This directly undercuts AC2’s “validation completes within 1 second” requirement and AC3’s immediate abort/error expectation.

2. **CRITICAL — the endpoint allows concurrent OTA sessions against one global updater.**  
   References: `firmware/adapters/WebPortal.cpp:93-101`, `434-529`  
   `g_otaUploads` tracks state per request, but `Update.begin()`, `Update.write()`, `Update.end()`, and `Update.abort()` all operate on the single global `Update` object. A second client can start another upload while the first is in progress, interleave chunks, and corrupt the staged image or abort the first upload. There is no 409/lockout path for “OTA already in progress”.

3. **CRITICAL — Task 3 is marked done, but the claimed partition logging is absent.**  
   References: `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:37-43`, `firmware/adapters/WebPortal.cpp:490-494`  
   The story explicitly says it logs target partition label and size. The code only logs `"Update started"`. That is a straight mismatch between the implementation record and the code, and it removes useful forensic data when OTA failures happen in the field.

4. **IMPORTANT — Task 10 is not reviewable as complete.**  
   References: `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md:77-82`  
   This story is almost entirely failure-handling logic, yet there is no committed test coverage under `firmware/test/` or `tests/`, and no captured build/hardware-test artifact in the review scope. In a read-only validation pass, “manual test passed” with zero evidence is indistinguishable from “not done”.

5. **IMPORTANT — the success path still routes through abort-style cleanup.**  
   References: `firmware/adapters/WebPortal.cpp:102-111`, `421-424`, `523-529`  
   `clearOTAUpload()` always calls `Update.abort()` when `started` is true. After `Update.end(true)` succeeds, `started` is never reset, and the success handler immediately calls `clearOTAUpload(request)`. At minimum this produces false “aborted during cleanup” behavior; at worst it risks invalidating or dirtying the just-finalized OTA state depending on library internals. Success cleanup and failure cleanup are incorrectly conflated.

6. **IMPORTANT — firmware upload has no authentication boundary at all.**  
   References: `firmware/adapters/WebPortal.cpp:399-531`  
   This is the highest-impact endpoint in the device and it accepts unauthenticated multipart POSTs. On a LAN device, that means any host on the network can push firmware. It is also CSRF-able from a browser via a plain HTML form post, even if the attacker cannot read the JSON response.

7. **IMPORTANT — there is no server-side size guard while streaming.**  
   References: `firmware/adapters/WebPortal.cpp:459-490`, `507-521`; `architecture.md` Foundation Rule 12  
   The implementation checks only the magic byte before `Update.begin(partition->size)`. It never enforces `bytesWritten + len <= partition->size` while streaming. Oversize binaries therefore fail late, after erase/write work has already started, which wastes time, flash wear, and violates the architecture’s “validate before writing” intent.

## Suggested Fixes

1. Reject OTA re-entry with a single global “upload in progress” guard and return HTTP 409 for concurrent attempts.
2. Split cleanup into `abortOTAUpload()` and `clearOTAUploadState()` so the success path never calls `Update.abort()`.
3. Send or force-close the request immediately on fatal upload errors instead of waiting for the multipart parser to finish the whole body.
4. Add a streaming size check using `state.bytesWritten + len` against the selected OTA partition size.
5. Add at least one reproducible automated test harness for invalid magic, write failure, and disconnect cleanup, plus a committed hardware verification record.
6. Add actual partition logging at OTA start, matching the story’s claimed implementation.
7. Put a device-local auth token or anti-CSRF nonce in front of `/api/ota/upload`.

## Review Actions Summary

- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 7

**Verdict:** REJECT. This endpoint is not ready to trust for OTA updates until the correctness, exclusivity, and security gaps are closed.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `_bmad-output/implementation-artifacts/stories/fn-1-3-ota-upload-endpoint.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| AC Implementation Gaps | 2 |
| Task Completion Lies | 2 |
| Critical Bugs | 3 |
| Architecture Violations | 2 |
| Missing Tests | 1 |
| **Total Issues** | **10** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Race condition: unprotected `g_otaUploads` vector accessed from async task, disconnect callback, and timer contexts concurrently | WebPortal.cpp:93-114 | +3 |
| 🔴 CRITICAL | Missing unit tests for OTA upload - Task 10 falsely claims "Build and test" complete | Test directory gap | +3 |
| 🔴 CRITICAL | `clearOTAUpload()` called BEFORE `request->send()` - use-after-free risk if request destroyed during send | WebPortal.cpp:413,428 | +3 |
| 🟠 IMPORTANT | Task 8 lie: WebPortal.h missing OTA handler declarations despite being marked complete | WebPortal.h | +1 |
| 🟠 IMPORTANT | No file size validation against partition capacity before `Update.begin()` | AC #1 gap | +1 |
| 🟠 IMPORTANT | AC #6 not validated: No performance timing enforcement for <30s upload | Missing implementation | +1 |
| 🟠 IMPORTANT | OTA reboot timer never deleted - ESP-IDF timer leak on repeated uploads | WebPortal.cpp:431-438 | +1 |
| 🟡 MINOR | Inconsistent magic number: 500ms uses raw literal, other timers use named constants | WebPortal.cpp:438 | +0.3 |
| 🟡 MINOR | Missing `bytesWritten` logging in final success case | Dev note gap | +0.3 |
| 🟡 MINOR | No partition info logged per story Dev Notes requirement | WebPortal.cpp:496 | +0.3 |

### Evidence Score: 13.9

| Score | Verdict |
|-------|---------|
| **13.9** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[9/10] Single Responsibility Violation:** OTA upload logic (130+ lines) embedded directly in lambda instead of separate handler method like `_handlePostLogos()`. Violates established pattern.
  - 📍 `WebPortal.cpp:399-531`
  - 💡 Fix: Extract to `_handleOTAUpload()` and `_handleOTAUploadChunk()` methods as Task 8 specified

- **[7/10] Open/Closed Violation:** Hard-coded error codes as string literals throughout handler. Adding new error type requires modifying multiple locations.
  - 📍 `WebPortal.cpp:466,476,486,509,523`
  - 💡 Fix: Define error code constants in anonymous namespace

- **[6/10] Tight Coupling:** `Update` singleton accessed directly from web handler without abstraction layer. Makes testing impossible and violates hexagonal architecture.
  - 📍 `WebPortal.cpp:484,504,506,519,520`
  - 💡 Fix: Create OTAService adapter class in `core/`

---

## ⚡ Performance & Scalability

- **[MEDIUM] Missing Bounds Check:** No validation that uploaded file size ≤ partition size before calling `Update.begin()`. Could attempt to write beyond partition boundary.
  - 📍 `WebPortal.cpp:484`
  - 💡 Fix: Check `Content-Length` header against `partition->size` (per AR18, though Content-Length may be multipart wrapper)

---

## 🐛 Correctness & Safety

- **🐛 Bug: Use-After-Free Risk:**
  - 📍 `WebPortal.cpp:413,428`
  - `clearOTAUpload(request)` clears state from vector BEFORE `request->send(200)` completes. If request is destroyed during send (e.g., client disconnects), subsequent access to `state` in error handling is undefined behavior.
  - 🔄 Reproduction: Upload firmware, immediately disconnect after server receives final chunk but before response sent
  - 💡 Fix: Move `clearOTAUpload()` to after `send()` completes, or use shared_ptr pattern

- **🐛 Bug: Race Condition on Global Vector:**
  - 📍 `WebPortal.cpp:93-114`
  - `g_otaUploads` accessed from: (1) AsyncWebServer task, (2) `onDisconnect` callback (potentially interrupt context), (3) Timer callback. No mutex protection.
  - 🔄 Reproduction: WiFi disconnects during chunk processing while another request being handled
  - 💡 Fix: Add `std::mutex` or use ESP-IDF critical sections

- **🐛 Bug: Silent Failure on Send Error:**
  - 📍 `WebPortal.cpp:428`
  - If `request->send()` fails (returns false), device still reboots. No visibility to client that update succeeded.
  - 💡 Fix: Check return value of `send()` before scheduling reboot

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Never-Deleted Timer:**
  - 📍 `WebPortal.cpp:431-438`
  - `otaRebootTimer` created once as static but never deleted with `esp_timer_delete()`. On repeated OTA uploads, this is fine (static prevents reallocation), but violates ESP-IDF best practices.
  - 💥 Explosion radius: Memory leak if OTA endpoint moved/reloaded dynamically

- **💣 Tech Debt: Task Documentation Lie:**
  - Task 8 explicitly states: "Add `_handleOTAUpload()` declaration in WebPortal.h"
  - WebPortal.h has NO OTA-related declarations
  - This is a pattern violation - all other handlers have explicit private method declarations

---

## 📋 Acceptance Criteria Audit

| AC | Status | Issue |
|----|--------|-------|
| #1 | ✅ PASS | Streaming implementation correct |
| #2 | ✅ PASS | Magic byte validation correct |
| #3 | ⚠️ PARTIAL | `Update.abort()` called on write failure, but error logging could be more detailed |
| #4 | ⚠️ PARTIAL | Disconnect handler registered, but race condition with vector access |
| #5 | ⚠️ PARTIAL | 500ms delay present, but timer leak and ordering issue with `clearOTAUpload()` |
| #6 | ❌ FAIL | No performance validation or timing enforcement for <30s upload |

---

## 🛠️ Suggested Fixes

### 1. Fix Race Condition and Ordering Bug

**File:** `firmware/adapters/WebPortal.cpp`

```cpp
// Move clearOTAUpload to AFTER send completes
request->send(200, "application/json", output);
// THEN clear state
clearOTAUpload(request);
```

### 2. Add Mutex Protection for Global State

**File:** `firmware/adapters/WebPortal.cpp`

```cpp
namespace {
    std::mutex g_otaUploadsMutex;  // Add this
    
    // Wrap all g_otaUploads access:
    std::lock_guard<std::mutex> lock(g_otaUploadsMutex);
}
```

### 3. Add Missing Header Declarations

**File:** `firmware/adapters/WebPortal.h`

```cpp
private:
    void _handleOTAUpload(AsyncWebServerRequest* request);
    static void _handleOTAUploadChunk(AsyncWebServerRequest* request, 
        const String& filename, size_t index, uint8_t* data, size_t len, bool final);
```

### 4. Add Unit Tests

**New File:** `firmware/test/test_web_portal/test_main.cpp`

Create tests for:
- Magic byte validation
- Error code responses
- State management
- Mock Update class for unit testing

---

## Review Actions Summary

- Issues Found: **10**
- Evidence Score: **13.9 (REJECT)**
- Verdict: **Code requires significant rework before approval**

**Action Required:**
1. Fix use-after-free by reordering `clearOTAUpload()` call
2. Add mutex protection for `g_otaUploads` vector
3. Add missing WebPortal.h declarations per Task 8
4. Create unit tests for OTA upload functionality
5. Add partition size validation before `Update.begin()`
6. Fix timer leak with proper cleanup
7. Document or implement AC #6 performance validation

🚫 **DO NOT MERGE** - Critical safety issues must be resolved first.

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
<var name="session_id">bf84ecca-1101-44eb-b4af-628768bc727a</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="ab213a12">embedded in prompt, file id: ab213a12</var>
<var name="story_id">fn-1.3</var>
<var name="story_key">fn-1-3-ota-upload-endpoint</var>
<var name="story_num">3</var>
<var name="story_title">3-ota-upload-endpoint</var>
<var name="template">False</var>
<var name="timestamp">20260412_1307</var>
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