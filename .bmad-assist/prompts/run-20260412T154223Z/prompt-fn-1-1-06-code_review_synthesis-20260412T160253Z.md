<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-1 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260412T160253Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-1.1

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
<file id="f72f6282" path="_bmad-output/implementation-artifacts/stories/fn-1-1-partition-table-and-build-configuration.md" label="DOCUMENTATION"><![CDATA[

# Story fn-1.1: Partition Table & Build Configuration

Status: blocked-hardware

## Story

As a **developer**,
I want **a dual-OTA partition table and firmware version build flag**,
So that **the device supports over-the-air updates with a known firmware identity**.

## Acceptance Criteria

1. **Given** the current firmware binary **When** built via `pio run` **Then** the binary size is measured and logged to stdout **And** if the binary exceeds 1.3MB, a warning is logged to stdout and noted in the 'Completion Notes List' section of this story with optimization recommendations (disable Bluetooth via `-D CONFIG_BT_ENABLED=0`, strip unused library features) before proceeding

2. **Given** `firmware/custom_partitions.csv` updated with dual-OTA layout: nvs 0x5000/20KB (offset 0x9000), otadata 0x2000/8KB (offset 0xE000), app0 0x180000/1.5MB (offset 0x10000), app1 0x180000/1.5MB (offset 0x190000), spiffs 0xF0000/960KB (offset 0x310000) **When** `platformio.ini` references the custom partition table and includes `-D FW_VERSION=\"1.0.0\"` in build_flags **Then** `pio run` builds successfully with no errors **And** the compiled binary is under 1.5MB

3. **Given** the new partition layout is flashed via USB **When** the device boots **Then** LittleFS mounts successfully on the 960KB spiffs partition **And** the following existing functionality is verified to work correctly: device boots successfully; web dashboard accessible at device IP; flight data fetches and displays on LED matrix; logo management functions (upload, list, delete); calibration tools function **And** `pio run -t uploadfs` uploads web assets and logos to LittleFS with at least 500KB free remaining

4. **Given** `FW_VERSION` is defined as a build flag **When** any source file references `FW_VERSION` **Then** the version string is available at compile time **And** the version string accurately reflects the value defined in the build flag when accessed at runtime

## Tasks / Subtasks

- [x] **Task 1: Measure current binary size baseline** (AC: #1)
  - [x] Run `pio run` and record binary size from build output
  - [x] Document current size as baseline for comparison
  - [x] If >1.3MB, document optimization recommendations

- [x] **Task 2: Update custom_partitions.csv with dual-OTA layout** (AC: #2)
  - [x] Modify `firmware/custom_partitions.csv` with exact offsets and sizes:
    - nvs: 0x9000, 20KB (0x5000)
    - otadata: 0xE000, 8KB (0x2000)
    - app0: 0x10000, 1.5MB (0x180000)
    - app1: 0x190000, 1.5MB (0x180000)
    - spiffs: 0x310000, 960KB (0xF0000)
  - [x] Verify total does not exceed 4MB flash

- [x] **Task 3: Add FW_VERSION build flag to platformio.ini** (AC: #2, #4)
  - [x] Add `-D FW_VERSION=\"1.0.0\"` to build_flags section
  - [x] Ensure proper escaping for string macro
  - [x] Add `-D CONFIG_BT_ENABLED=0` if binary is large (optimization) — NOT NEEDED, binary is 1.15MB

- [x] **Task 4: Build and verify firmware compiles** (AC: #2)
  - [x] Run `pio run` and verify no errors
  - [x] Confirm binary size under 1.5MB
  - [x] Document final binary size

- [ ] **Task 5: Flash and test on hardware** (AC: #3) — REQUIRES HARDWARE
  - [ ] Flash via USB: `pio run -t upload`
  - [ ] Verify device boots successfully
  - [ ] Verify LittleFS mounts on 960KB partition
  - [ ] Test all existing functionality:
    - Flight data pipeline fetches and displays
    - Web dashboard accessible at device IP
    - Logo management works
    - Calibration tools function

- [ ] **Task 6: Upload filesystem and verify space** (AC: #3) — REQUIRES HARDWARE
  - [ ] Run `pio run -t uploadfs`
  - [ ] Verify web assets upload successfully
  - [ ] Verify at least 500KB free remaining in LittleFS
  - [ ] Test served assets work correctly in browser

- [x] **Task 7: Verify FW_VERSION accessible in code** (AC: #4)
  - [x] Add test reference to FW_VERSION in code (e.g., log on boot)
  - [x] Verify version string compiles and logs correctly

## Dev Notes

### Critical Architecture Constraints

**Partition Layout Math (MUST be exact):**
```
Total Flash: 4MB (0x400000)
Bootloader:  0x1000 - 0x8FFF (reserved, not in CSV)
Partitions start at 0x9000

| Name    | Type | SubType | Offset   | Size     | End      |
|---------|------|---------|----------|----------|----------|
| nvs     | data | nvs     | 0x9000   | 0x5000   | 0xDFFF   | 20KB
| otadata | data | ota     | 0xE000   | 0x2000   | 0xFFFF   | 8KB
| app0    | app  | ota_0   | 0x10000  | 0x180000 | 0x18FFFF | 1.5MB
| app1    | app  | ota_1   | 0x190000 | 0x180000 | 0x30FFFF | 1.5MB
| spiffs  | data | spiffs  | 0x310000 | 0xF0000  | 0x3FFFFF | 960KB
```

**BREAKING CHANGE WARNING:**
- This partition change requires a full USB reflash
- NVS will be erased - all saved settings will be lost
- LittleFS will be erased - all uploaded logos will be lost
- Users should export settings first (but that endpoint doesn't exist yet - this is the first Foundation story)

**Binary Size Targets:**
- Warning threshold: 1.3MB (1,363,148 bytes)
- Hard limit: 1.5MB (1,572,864 bytes) - must fit in app0/app1 partition
- Current MVP partition has 2MB app space - we're cutting it to 1.5MB

**Optimization Flags (if needed):**
```ini
build_flags =
    -D CONFIG_BT_ENABLED=0        ; Saves ~60KB - Bluetooth disabled
    -D CONFIG_BTDM_CTRL_MODE_BLE_ONLY=1  ; If BLE needed, saves some
    -Os                            ; Optimize for size (already default)
```

### Current vs Target Partition Layout

**Current (MVP - from exploration):**
```csv
# Name,    Type,  SubType, Offset,   Size
nvs,       data,  nvs,     0x9000,   0x5000      # 20KB
otadata,   data,  ota,     0xe000,   0x2000      # 8KB
app0,      app,   ota_0,   0x10000,  0x200000    # 2MB (single app)
spiffs,    data,  spiffs,  0x210000, 0x1F0000   # ~1.9MB
```

**Target (Foundation dual-OTA):**
```csv
# Name,    Type,  SubType, Offset,   Size
nvs,       data,  nvs,     0x9000,   0x5000      # 20KB
otadata,   data,  ota,     0xe000,   0x2000      # 8KB
app0,      app,   ota_0,   0x10000,  0x180000    # 1.5MB
app1,      app,   ota_1,   0x190000, 0x180000    # 1.5MB (NEW)
spiffs,    data,  spiffs,  0x310000, 0xF0000    # 960KB (reduced)
```

**Space Trade-off:**
- App space: 2MB → 1.5MB per slot (but now have 2 slots for OTA)
- LittleFS: ~1.9MB → 960KB (56% reduction)
- LittleFS budget: ~80KB web assets + ~198KB logos (~2KB each) + ~682KB free

### FW_VERSION Macro Usage

**In platformio.ini:**
```ini
build_flags =
    -D FW_VERSION=\"1.0.0\"
```

**In C++ code (will be used in later stories):**
```cpp
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"  // Fallback for IDE/testing
#endif

Serial.printf("Firmware version: %s\n", FW_VERSION);
```

**Note:** The escaped quotes `\"` in platformio.ini produce a string literal. The preprocessor will replace `FW_VERSION` with `"1.0.0"`.

### Testing Standards

**Build Verification:**
- `pio run` must complete with 0 errors
- Check `.pio/build/esp32dev/firmware.bin` size
- Size logged in build output as "Building .pio/build/esp32dev/firmware.bin"

**Hardware Verification:**
- Boot log shows LittleFS mount success
- Web server responds at device IP
- Flight data displays on LED matrix
- All dashboard sections load

**Filesystem Verification:**
```bash
# Check LittleFS space after upload
# Will be visible in serial output or via /api/status endpoint
```

### Project Structure Notes

**Files to modify:**
1. `firmware/custom_partitions.csv` - Partition table definition
2. `firmware/platformio.ini` - Build flags and partition reference

**platformio.ini already has:**
- `board_build.filesystem = littlefs` ✓
- `board_build.partitions = custom_partitions.csv` ✓

**Only changes needed in platformio.ini:**
- Add `-D FW_VERSION=\"1.0.0\"` to build_flags

### References

- [Source: architecture.md#Decision F1: Partition Table — Dual OTA Layout]
- [Source: architecture.md#Decision F2: OTA Handler Architecture — WebPortal + main.cpp] (for FW_VERSION usage context)
- [Source: prd.md#Update Mechanism] - Partition table explanation
- [Source: prd.md#Flash Partition Layout] - 4MB total with custom CSV
- [Source: epic-fn-1.md#Story fn-1.1] - Full acceptance criteria
- [Source: project-context.md] - Build from `firmware/` with `pio run`

### Dependencies

**This story has NO code dependencies** - it's the foundation for all Foundation release stories.

**Stories that depend on this:**
- fn-1.2: ConfigManager Expansion (needs partition table for NVS)
- fn-1.3: OTA Upload Endpoint (needs app1 partition for OTA)
- fn-1.4: OTA Self-Check (needs dual-OTA partitions)
- fn-1.5: Settings Export (needs working firmware)
- fn-1.6: Dashboard Firmware Card (needs FW_VERSION)
- fn-1.7: Settings Import (needs working wizard)

### Risk Mitigation

1. **Binary too large:** If >1.5MB, add `-D CONFIG_BT_ENABLED=0`. BT is not used in MVP.
2. **LittleFS mount fails:** Verify partition offset/size math. Use `pio run -t erase` before flash if needed.
3. **Existing functionality broken:** This is a regression risk. Run full smoke test after flash.

## Dev Agent Record

### Agent Model Used

Claude Opus 4.5

### Debug Log References

- Build output verified via `pio run` - SUCCESS
- Binary size: 1,206,432 bytes (1.15MB) - under 1.5MB limit
- Flash usage reported: 76.3% of 1.5MB partition (confirms new partition active)
- RAM usage: 16.4% (53,684 bytes of 327,680 bytes)

### Completion Notes List

**Implementation Summary:**

1. **Binary Size Baseline (Task 1):**
   - Current binary: 1,199,857 bytes (~1.14 MB) before changes
   - This is BELOW the 1.3MB warning threshold
   - No optimization flags needed (Bluetooth disable not required)
   - **Code Review Fix Applied**: Implemented automated binary size check via `check_size.py` script to satisfy AC #1 requirement for automated logging

2. **Partition Table Update (Task 2):**
   - Updated `custom_partitions.csv` with exact dual-OTA layout
   - Added app1 partition at 0x190000 (1.5MB) for OTA
   - Reduced spiffs from ~1.9MB to 960KB
   - Total: 4MB (0x400000) - verified correct

3. **FW_VERSION Build Flag (Task 3):**
   - Added `-D FW_VERSION=\"1.0.0\"` to platformio.ini build_flags
   - Added fallback `#define FW_VERSION "0.0.0-dev"` in main.cpp for IDE compatibility
   - Added boot log: `Serial.printf("FlightWall Firmware v%s\n", FW_VERSION);`

4. **Build Verification (Task 4):**
   - Final binary: 1,207,440 bytes (1.15MB) after code review fixes
   - 76.8% of new 1.5MB partition
   - Build completed with 0 errors
   - Automated size check runs on every build

5. **Hardware Testing (Tasks 5-6):**
   - REQUIRES PHYSICAL DEVICE ACCESS
   - Current LittleFS data: 456KB (99 logos + web assets)
   - With 960KB partition: ~504KB free (exceeds 500KB requirement)

6. **FW_VERSION Code Integration (Task 7):**
   - Version logged at boot via Serial.printf
   - Compiles and links correctly

**Code Review Fixes Applied (2026-04-12):**
1. **CRITICAL**: Fixed silent data loss risk by changing `LittleFS.begin(true)` to `LittleFS.begin(false)` with explicit error messaging
2. **HIGH**: Implemented automated binary size check script (`check_size.py`) to satisfy AC #1 requirement
3. **MEDIUM**: Added partition layout runtime validation to detect flash/firmware mismatches early

**Breaking Change Warning:**
- This partition change requires full USB reflash
- NVS and LittleFS will be erased on first flash with new partitions

**Final Validation (2026-04-12):**
- Clean build succeeds with 0 errors
- Binary size: 1,207,440 bytes (1.15MB) - 76.8% of 1.5MB partition
- check_size.py output confirmed: "OK: Binary size within limits"
- RAM usage: 16.4% (53,684 bytes of 327,680 bytes)
- Partition table math verified: nvs+otadata+app0+app1+spiffs = 0x3F7000 < 0x400000 (4MB)

### Change Log

| Date | Change | Author |
|------|--------|--------|
| 2026-04-12 | Story created with comprehensive context | BMad |
| 2026-04-12 | Implemented Tasks 1-4, 7. Tasks 5-6 require hardware. | Claude Opus 4.5 |
| 2026-04-12 | Code review synthesis: Applied critical fixes for AC #1 automation, LittleFS safety, partition validation | Claude Sonnet 4.5 |
| 2026-04-12 | Validated implementation: All software tasks complete. Build verified 1.15MB binary. Tasks 5-6 blocked on hardware. | Claude Opus 4 |

### File List

- `firmware/custom_partitions.csv` (MODIFIED) - Dual-OTA partition layout
- `firmware/platformio.ini` (MODIFIED) - Added FW_VERSION build flag, extra_scripts for size check
- `firmware/src/main.cpp` (MODIFIED) - Added FW_VERSION logging, partition validation, safe LittleFS mount
- `firmware/check_size.py` (NEW) - Automated binary size verification script


## Senior Developer Review (AI)

### Review: 2026-04-12
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 9.3 (Validator A), 5.6 (Validator B) → REJECT/MAJOR REWORK
- **Issues Found:** 6 verified issues (2 Critical, 3 High/Important, 1 Minor)
- **Issues Fixed:** 3 critical/high priority issues
- **Action Items Created:** 1 deferred enhancement

### Review Summary

Both validators identified legitimate issues with the initial implementation. Validator A scored 9.3 (REJECT) and Validator B scored 5.6 (MAJOR REWORK), indicating significant gaps in AC fulfillment.

**Critical Issues Fixed:**
1. AC #1 automation claim - Added `check_size.py` script for automated binary size logging
2. Silent data loss risk - Changed LittleFS mount to fail-safe mode with user notification
3. Partition runtime validation - Added boot-time validation to detect flash/firmware mismatches

**Issues Dismissed as False Positives:**
1. "spiffs" partition type for LittleFS - This is correct per ESP32 Arduino framework conventions
2. AC #3 hardware testing incomplete - Story explicitly marks Tasks 5-6 as requiring hardware, correctly deferred
3. FW_VERSION test is debug print - For this foundational story, boot logging is acceptable verification

**Deferred for Future Story:**
1. Git-based auto-versioning - Valid improvement but beyond scope of fn-1.1; recommend for fn-1.6 or dedicated refactoring story



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


]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-1-1-partition-table-and-build-configuration.md

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
| **Total Issues** | **7** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | AC #3 - Hardware verification incomplete. The Acceptance Criterion requires physical hardware verification of existing functionality and LittleFS space, which is explicitly stated as not completed. | Story fn-1.1 Acceptance Criteria #3, Tasks 5 and 6 | +3 |
| 🟠 IMPORTANT | Inconsistent FreeRTOS Queue Null Checks. `xQueueCreate` failures are logged, but subsequent `xQueueOverwrite` and `xQueuePeek` calls in `displayTask` and `loop()` do not consistently check for `nullptr` queues, leading to potential silent failures or confusing behavior. | firmware/src/main.cpp | +3 |
| 🟠 IMPORTANT | Heap Fragmentation Risk from `String` Concatenation. Frequent dynamic allocations and deallocations due to `String` concatenations in message formatting can lead to heap fragmentation in an embedded system, impacting long-term stability. | firmware/src/main.cpp | +3 |
| 🟡 MINOR | Hardcoded Build Flags and Include Paths in `platformio.ini`. The manual listing of include paths is brittle and increases maintenance overhead. A more dynamic approach is preferable. | firmware/platformio.ini | +0.3 |
| 🟡 MINOR | Magic Numbers for Timing/Thresholds. Numerous magic numbers are used for timing and button thresholds, reducing readability and maintainability. These should be defined as named constants. | firmware/src/main.cpp | +0.3 |
| 🟡 MINOR | Silent Exit in `check_size.py` if Binary Not Found. The build script `check_size.py` exits silently if `firmware.bin` is missing, which could obscure prior build failures. It should log an explicit warning or error. | firmware/check_size.py | +0.3 |
| 🟡 MINOR | Interface Segregation Principle Violation in `WiFiManager` Callback. The `onStateChange` callback forces consumers to accept `oldState` even when not needed, as evidenced by `(void)oldState;` in `queueWiFiStateMessage`. | firmware/src/main.cpp | +0.3 |
| 🟢 CLEAN PASS | Abstraction Level Analysis | | -0.5 |
| 🟢 CLEAN PASS | Lying Tests Detection | | -0.5 |
| 🟢 CLEAN PASS | Security Vulnerability Scan | | -0.5 |

### Evidence Score: 8.7

| Score | Verdict |
|-------|---------|
| **8.7** | **REJECT** |

---

## 🏛️ Architectural Sins

- **[6/10] Interface Segregation:** The `WiFiManager::onStateChange` callback provides both `oldState` and `newState`. However, the `queueWiFiStateMessage` lambda explicitly casts `oldState` to `void`, indicating it doesn't utilize this parameter. This forces consumers of the callback to accept an interface that includes unnecessary information, which is a minor violation of the Interface Segregation Principle.
  - 📍 `firmware/src/main.cpp:386`
  - 💡 Fix: Consider if `WiFiManager` can offer a more granular event (e.g., `onStateChangedTo(newState)`) or if `oldState` is truly optional for all callback implementers.

---

## 🐍 Pythonic Crimes & Readability

- **Silent Exit:** The `check_size.py` script will silently return if the `binary_path` does not exist. While it prevents errors, in a build pipeline context, a script designed to verify a binary's size should report if the binary itself is missing, indicating a preceding build failure.
  - 📍 `firmware/check_size.py:9`
- **Magic Numbers:** The `limit` (1.5MB) and `warning_threshold` (1.3MB) values are defined as hexadecimal literals directly within the script. While commented, using named constants at the top of the script or sourcing these from a configuration file would improve readability and maintainability.
  - 📍 `firmware/check_size.py:12`

---

## ⚡ Performance & Scalability

- **[HIGH] Heap Fragmentation:** The use of `String` concatenation (e.g., `String("IP: ") + g_wifiManager.getLocalIP()`) to construct messages for `queueDisplayMessage` involves dynamic memory allocations. Frequent, small, and varying-sized allocations and deallocations can lead to heap fragmentation on an ESP32 over extended uptime, potentially causing out-of-memory errors.
  - 📍 `firmware/src/main.cpp:187`
  - 💡 Fix: Prioritize `snprintf` with fixed-size buffers or `char` arrays for message construction to minimize dynamic memory usage and prevent heap fragmentation.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** Inconsistent FreeRTOS Queue Null Checks. After checking for `nullptr` post-`xQueueCreate` (e.g., for `g_flightQueue` and `g_displayMessageQueue`), several subsequent calls to `xQueueOverwrite` and `xQueuePeek` (e.g., in `displayTask` and `loop()`) do not re-verify if the queue handles are valid. Although these FreeRTOS functions return `pdFAIL` for null queues, the lack of explicit checks and handling of `pdFAIL` can lead to confusing behavior or silent operational failures if a queue creation previously failed.
  - 📍 `firmware/src/main.cpp:291`
  - 🔄 Reproduction: Simulate `xQueueCreate` returning `nullptr` (e.g., by exhausting heap or configuring a tiny queue pool) and observe subsequent queue operations.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** Hardcoded Include Paths in `platformio.ini`. The `build_flags` section explicitly lists numerous include directories (`-I . -I include -I src -I adapters -I core -I models -I interfaces -I utils -I config`). This manual approach is prone to errors and increases maintenance overhead; any changes to the project's directory structure or module additions require manual updates to this list. PlatformIO typically offers more dynamic ways to manage source and include paths.
  - 📍 `firmware/platformio.ini:20`
  - 💥 Explosion radius: High, impacts all future development and refactoring of the project structure.
- **💣 Tech Debt:** Magic Numbers for Timing and Thresholds. The code frequently uses un-named literal values for timings (e.g., `delay(200)`, `BUTTON_DEBOUNCE_MS = 50`, `AUTHENTICATING_DISPLAY_MS = 1000UL`, and various values within `tickStartupProgress`). This reduces code clarity and makes future modifications or tuning difficult, requiring developers to infer the meaning of each number.
  - 📍 `firmware/src/main.cpp:109`
  - 💥 Explosion radius: Medium, affects readability and ease of modification for timing-related logic.

---

## 🛠️ Suggested Fixes

### 1. Address Incomplete AC #3

**File:** `_bmad-output/implementation-artifacts/stories/fn-1-1-partition-table-and-build-configuration.md`
**Issue:** Acceptance Criterion #3 explicitly requires hardware verification for LittleFS mount, web dashboard accessibility, flight data display, logo management, and calibration tools functionality, as well as `pio run -t uploadfs` verification. The story's "Completion Notes List" and task tracking indicate that Tasks 5 and 6, which cover this AC, "REQUIRE HARDWARE" and are not yet completed. This leaves a significant portion of the AC unverified.

**Diff:**
```diff
--- a/_bmad-output/implementation-artifacts/stories/fn-1-1-partition-table-and-build-configuration.md
+++ b/_bmad-output/implementation-artifacts/stories/fn-1-1-partition-table-and-build-configuration.md
@@ -20,7 +20,7 @@
 - [ ] **Task 5: Flash and test on hardware** (AC: #3) — REQUIRES HARDWARE
   - [ ] Flash via USB: `pio run -t upload`
   - [ ] Verify device boots successfully
-  - [ ] Verify LittleFS mounts on 960KB partition
+  - [ ] **CRITICAL**: Verify LittleFS mounts successfully on the 960KB spiffs partition
   - [ ] Test all existing functionality:
     - Flight data pipeline fetches and displays
     - Web dashboard accessible at device IP
@@ -30,7 +30,7 @@
 
 - [ ] **Task 6: Upload filesystem and verify space** (AC: #3) — REQUIRES HARDWARE
   - [ ] Run `pio run -t uploadfs`
-  - [ ] Verify web assets upload successfully
-  - [ ] Verify at least 500KB free remaining in LittleFS
+  - [ ] **CRITICAL**: Verify web assets and logos upload successfully to LittleFS
+  - [ ] **CRITICAL**: Verify at least 500KB free remaining in LittleFS
   - [ ] Test served assets work correctly in browser
 
 - [x] **Task 7: Verify FW_VERSION accessible in code** (AC: #4)

```

### 2. Standardize FreeRTOS Queue Null Checks

**File:** `firmware/src/main.cpp`
**Issue:** There is inconsistent checking for `nullptr` FreeRTOS queue handles after creation. While `xQueueCreate` failures are logged, subsequent calls to `xQueueOverwrite` and `xQueuePeek` often proceed without checking if the queue is valid, leading to potential silent failures or confusing behavior if queue creation failed.

**Diff:**
```diff
--- a/firmware/src/main.cpp
+++ b/firmware/src/main.cpp
@@ -296,8 +296,10 @@
     DisplayStatusMessage statusMessage = {};
     snprintf(statusMessage.text, sizeof(statusMessage.text), "%s", message.c_str());
     statusMessage.durationMs = durationMs;
-    xQueueOverwrite(g_displayMessageQueue, &statusMessage);
-}
+    if (g_displayMessageQueue != nullptr) {
+        xQueueOverwrite(g_displayMessageQueue, &statusMessage);
+    }
+}
 
 static void showInitialWiFiMessage()
 {
@@ -512,7 +514,9 @@
         // Write flight data to double buffer and push pointer to queue
         g_flightBuf[g_writeBuf].flights = flights;
         FlightDisplayData *ptr = &g_flightBuf[g_writeBuf];
-        if (g_flightQueue != nullptr)
+        // Ensure queue is valid before attempting to write
+        // xQueueOverwrite returns pdFAIL if queue is null, but explicit check avoids ambiguity.
+        if (g_flightQueue != nullptr) 
         {
             xQueueOverwrite(g_flightQueue, &ptr);
         }

```

### 3. Replace String Concatenation with `snprintf` for Heap Safety

**File:** `firmware/src/main.cpp`
**Issue:** The use of `String` concatenation for constructing messages passed to `queueDisplayMessage` can lead to dynamic memory allocations and heap fragmentation, which is undesirable in long-running embedded systems.

**Diff:**
```diff
--- a/firmware/src/main.cpp
+++ b/firmware/src/main.cpp
@@ -184,20 +184,28 @@
     switch (g_wifiManager.getState())
     {
         case WiFiState::AP_SETUP:
-            g_display.displayMessage(String("Setup Mode"));
+            queueDisplayMessage("Setup Mode");
             break;
         case WiFiState::CONNECTING:
-            g_display.displayMessage(String("Connecting to WiFi..."));
+            queueDisplayMessage("Connecting to WiFi...");
             break;
         case WiFiState::STA_CONNECTED:
-            g_display.displayMessage(String("IP: ") + g_wifiManager.getLocalIP());
+        {
+            char ipMsg[32];
+            snprintf(ipMsg, sizeof(ipMsg), "IP: %s", g_wifiManager.getLocalIP().c_str());
+            queueDisplayMessage(ipMsg);
+            break;
+        }
             break;
         case WiFiState::STA_RECONNECTING:
-            g_display.displayMessage(String("WiFi Lost..."));
+            queueDisplayMessage("WiFi Lost...");
             break;
         case WiFiState::AP_FALLBACK:
-            g_display.displayMessage(String("WiFi Failed"));
+            queueDisplayMessage("WiFi Failed");
             break;
+        default:
+            // Handle other states or do nothing
+            break;
     }
 }
 
@@ -216,7 +224,9 @@
                 return;
             case WiFiState::STA_CONNECTED:
                 enterPhase(StartupPhase::WIFI_CONNECTED);
-                queueDisplayMessage(String("WiFi Connected ✓"), 2000);
+                // Reusing the char array from DisplayStatusMessage to avoid heap allocation
+                char wifiConnectedMsg[64];
+                snprintf(wifiConnectedMsg, sizeof(wifiConnectedMsg), "WiFi Connected ✓"); // Use snprintf directly for consistency
+                queueDisplayMessage(wifiConnectedMsg, 2000);
                 LOG_I("Main", "Startup: WiFi connected");
                 return;
             case WiFiState::AP_FALLBACK:
@@ -234,16 +244,21 @@
     switch (state)
     {
         case WiFiState::AP_SETUP:
-            queueDisplayMessage(String("Setup Mode"));
+            queueDisplayMessage("Setup Mode");
             break;
         case WiFiState::CONNECTING:
-            queueDisplayMessage(String("Connecting to WiFi..."));
+            queueDisplayMessage("Connecting to WiFi...");
             break;
         case WiFiState::STA_CONNECTED:
-            queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 3000);
+        {
+            char ipMsg[32];
+            snprintf(ipMsg, sizeof(ipMsg), "IP: %s", g_wifiManager.getLocalIP().c_str());
+            queueDisplayMessage(ipMsg, 3000);
+            break;
+        }
             break;
         case WiFiState::STA_RECONNECTING:
-            queueDisplayMessage(String("WiFi Lost..."));
+            queueDisplayMessage("WiFi Lost...");
             break;
         case WiFiState::AP_FALLBACK:
-            queueDisplayMessage(String("WiFi Failed"));
+            queueDisplayMessage("WiFi Failed");
             break;
     }
 }
@@ -499,7 +514,9 @@
                 if (pressDuration > 0 && pressDuration <= BUTTON_SHORT_PRESS_MAX_MS) {
                     // Short press detected — show IP or status message
                     if (g_wifiManager.getState() == WiFiState::STA_CONNECTED) {
-                        queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 5000);
+                        char ipMsg[32];
+                        snprintf(ipMsg, sizeof(ipMsg), "IP: %s", g_wifiManager.getLocalIP().c_str());
+                        queueDisplayMessage(ipMsg, 5000);
                     } else {
                         queueDisplayMessage(String("No IP — Setup mode"), 5000);
                     }
```

### 4. Refactor Hardcoded Include Paths in `platformio.ini`

**File:** `firmware/platformio.ini`
**Issue:** The `build_flags` section contains a long, manually maintained list of include directories. This is brittle and error-prone.

**Diff:**
```diff
--- a/firmware/platformio.ini
+++ b/firmware/platformio.ini
@@ -16,14 +16,7 @@
 
 build_src_filter =
     +<*.cpp>
-    +<**/*.cpp>
-    +<../adapters/*.cpp>
-    +<../core/*.cpp>
-
-build_flags =
-    -I .
-    -I include
-    -I src
-    -I adapters
-    -I core
-    -I models
-    -I interfaces
-    -I utils
-    -I config
+    +<src/**.cpp>
+    +<adapters/**.cpp>
+    +<core/**.cpp>
+    +<models/**.cpp>
+    +<interfaces/**.cpp>
+    +<utils/**.cpp>
+    +<config/**.cpp>
+
+build_flags =
     -D LOG_LEVEL=3
     -D FW_VERSION=\"1.0.0\"

```

### 5. Introduce Named Constants for Magic Numbers

**File:** `firmware/src/main.cpp`
**Issue:** Various magic numbers are used for timing and thresholds, decreasing readability and maintainability.

**Diff:**
```diff
--- a/firmware/src/main.cpp
+++ b/firmware/src/main.cpp
@@ -106,8 +106,14 @@
 static StartupPhase g_startupPhase = StartupPhase::IDLE;
 static unsigned long g_phaseEnteredMs = 0;
 static bool g_firstFetchDone = false;  // One-shot flag: first fetch after startup
-static constexpr unsigned long AUTHENTICATING_DISPLAY_MS = 1000UL;
-
+
+// Named constants for timing and thresholds
+static constexpr unsigned long AUTHENTICATING_DISPLAY_DURATION_MS = 1000UL;
+static constexpr unsigned long WIFI_CONNECTED_MESSAGE_DURATION_MS = 2000UL;
+static constexpr unsigned long WIFI_IP_MESSAGE_DURATION_MS = 2000UL;
+static constexpr unsigned long WIFI_FAILED_MESSAGE_DURATION_MS = 5000UL;
+static constexpr unsigned long BOOT_BUTTON_INITIAL_HOLD_DETECT_MS = 500UL;
+
 static void enterPhase(StartupPhase phase)
 {
     g_startupPhase = phase;
@@ -432,15 +438,15 @@
 
         case StartupPhase::WIFI_CONNECTED:
             // Show "WiFi Connected ✓" briefly, then IP, then move to auth
-            if (elapsed < 2000)
+            if (elapsed < WIFI_CONNECTED_MESSAGE_DURATION_MS)
             {
                 // Still showing "WiFi Connected"
             }
-            else if (elapsed < 4000)
+            else if (elapsed < WIFI_CONNECTED_MESSAGE_DURATION_MS + WIFI_IP_MESSAGE_DURATION_MS)
             {
                 // Show IP address for discovery (use phase time to gate, not a flag)
-                if (elapsed >= 2000 && elapsed < 2100)
+                if (elapsed >= WIFI_CONNECTED_MESSAGE_DURATION_MS && elapsed < WIFI_CONNECTED_MESSAGE_DURATION_MS + 100) // Brief flash for IP
                 {
-                    queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), 2000);
+                    queueDisplayMessage(String("IP: ") + g_wifiManager.getLocalIP(), WIFI_IP_MESSAGE_DURATION_MS);
                 }
             }
             else
@@ -454,7 +460,7 @@
         case StartupPhase::WIFI_FAILED:
             // Terminal state — device returns to AP/setup mode
             // WiFiManager handles the AP fallback; message stays visible
-            if (elapsed >= 5000)
+            if (elapsed >= WIFI_FAILED_MESSAGE_DURATION_MS)
             {
                 // After showing failure message, return to idle
                 // (device is now in AP mode, user must re-run wizard)
@@ -464,7 +470,7 @@
 
         case StartupPhase::AUTHENTICATING:
             // Hold the authentication message briefly before starting the first fetch.
-            if (elapsed >= AUTHENTICATING_DISPLAY_MS)
+            if (elapsed >= AUTHENTICATING_DISPLAY_DURATION_MS)
             {
                 return true;
             }
@@ -490,7 +496,7 @@
     pinMode(BOARD_BOOT_BUTTON_GPIO, INPUT_PULLUP);
     delay(50); // Allow pull-up to settle
     if (digitalRead(BOARD_BOOT_BUTTON_GPIO) == LOW) {
-        // Button is held — sample for ~500ms to distinguish from noise
+        // Button is held — sample for duration to distinguish from noise
         unsigned long holdStart = millis();
-        bool held = true;
-        while (millis() - holdStart < 500) {
+        bool held = true; 
+        while (millis() - holdStart < BOOT_BUTTON_INITIAL_HOLD_DETECT_MS) {
             if (digitalRead(BOARD_BOOT_BUTTON_GPIO) == HIGH) {
                 held = false;
                 break;

```

### 6. Add Error Logging to `check_size.py` for Missing Binary

**File:** `firmware/check_size.py`
**Issue:** The script currently exits silently if `firmware.bin` is not found, which can obscure build failures.

**Diff:**
```diff
--- a/firmware/check_size.py
+++ b/firmware/check_size.py
@@ -7,7 +7,8 @@
     """Check firmware binary size against partition limit"""
     binary_path = str(target[0])
 
-    if not os.path.exists(binary_path):
+    if not os.path.exists(binary_path): 
+        print(f"ERROR: Firmware binary not found at {binary_path}")
         return
 
     size = os.path.getsize(binary_path)

```

### 7. Refine `WiFiManager` Callback Signature or Usage

**File:** `firmware/src/main.cpp`
**Issue:** The `WiFiManager::onStateChange` callback passes `oldState`, but `queueWiFiStateMessage` explicitly ignores it, indicating a potential Interface Segregation Principle violation where the interface provides more than is needed.

**Diff:**
```diff
--- a/firmware/src/main.cpp
+++ b/firmware/src/main.cpp
@@ -383,7 +383,7 @@
     g_wifiManager.init(g_forcedApSetup);
 
     // Activate startup progress coordinator when WiFi credentials exist
-    // (i.e. this is a post-setup boot, not first-time AP mode)
+    // (i.e. this is a post-setup boot, not first-time AP mode) 
     if (g_forcedApSetup)
     {
         // Forced AP via boot button — show distinct message
@@ -399,8 +399,8 @@
         showInitialWiFiMessage();
     }
 
-    g_wifiManager.onStateChange([](WiFiState oldState, WiFiState newState) {
-        (void)oldState;
+    // The callback only uses newState, indicating a potential ISP violation.
+    g_wifiManager.onStateChange([](WiFiState /* oldState */, WiFiState newState) {
         queueWiFiStateMessage(newState);
     });
 

```

---

## Review Actions Summary

- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

🚫 Code requires significant rework. Review action items carefully.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

I'll perform an adversarial code review of Story fn-1.1, examining the implementation files against the acceptance criteria and looking for code quality issues.

<tool>ReadFile</tool>
<parameter>{"path": "firmware/check_size.py"}</parameter>
<tool>ReadFile</tool>
<parameter>{"path": "firmware/custom_partitions.csv"}</parameter>
<tool>ReadFile</tool>
<parameter>{"path": "firmware/platformio.ini"}</parameter>
<tool>ReadFile</tool>
<parameter>{"path": "firmware/src/main.cpp", "n_lines": 150}</parameter>

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
<var name="reviewer_count">3</var>
<var name="session_id">dcc7cd32-ce48-4ff9-be25-191f79efe4cc</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="f72f6282">embedded in prompt, file id: f72f6282</var>
<var name="story_id">fn-1.1</var>
<var name="story_key">fn-1-1-partition-table-and-build-configuration</var>
<var name="story_num">1</var>
<var name="story_title">1-partition-table-and-build-configuration</var>
<var name="template">False</var>
<var name="timestamp">20260412_1202</var>
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
      - Commit message format: fix(component): brief description (synthesis-fn-1.1)
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