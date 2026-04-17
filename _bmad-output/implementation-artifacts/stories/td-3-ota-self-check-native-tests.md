# Story TD-3: OTA Self-Check Native Test Harness

branch: td-3-ota-self-check-native
zone:
  - firmware/core/OTASelfCheck.*
  - firmware/core/OTAUpdater.*
  - firmware/src/main.cpp
  - firmware/platformio.ini
  - firmware/test/test_ota_self_check/**
  - firmware/test/test_ota_self_check_native/**

Status: draft

## Story

As a **firmware engineer / CI system**,
I want **OTA self-check decision logic runnable on host (no ESP32 attached)**,
So that **self-check regressions are caught in PR review, not at hardware-test time**.

## Acceptance Criteria

1. **Given** the new header `firmware/core/OTASelfCheck.h` exists **When** inspected **Then** it declares `enum class SelfCheckVerdict { Pending, CommitOk, CommitTimeoutWarning, AlreadyValid }`, struct `SelfCheckInputs { bool pending_verify; bool wifi_connected; bool rollback_detected; uint32_t uptime_ms; uint32_t grace_window_ms; }`, and free function `SelfCheckVerdict evaluateSelfCheck(const SelfCheckInputs&)` — with no Arduino, ESP-IDF, or FreeRTOS includes.
2. **Given** `firmware/core/OTASelfCheck.cpp` exists **When** inspected **Then** `evaluateSelfCheck()` is implemented as a pure function (no I/O, no globals, no side effects, no dynamic allocation).
3. **Given** `firmware/src/main.cpp::performOtaSelfCheck()` (lines ~584-651) is refactored **When** the device boots **Then** verdict computation delegates to `evaluateSelfCheck()` and on-device behavior is unchanged — the existing hardware test `test_ota_self_check` still passes.
4. **Given** `firmware/platformio.ini` **When** inspected **Then** `[env:native]` is present with `platform = native`, `test_framework = unity`, `build_flags = -std=gnu++17 -DNATIVE_BUILD -I firmware/core`, and `test_filter = test_ota_self_check_native`.
5. **Given** `firmware/test/test_ota_self_check_native/test_main.cpp` **When** inspected **Then** it covers at least 7 cases: (a) pending_verify=true + wifi_connected=true → CommitOk, (b) pending_verify=true + wifi_connected=false + uptime ≥ grace → CommitTimeoutWarning, (c) pending_verify=false → AlreadyValid, (d) pending_verify=true + wifi_connected=false + uptime < grace → Pending, (e) boundary `uptime_ms == grace_window_ms` behavior, (f) pending_verify=false short-circuit (no commit/timeout path taken), (g) rollback_detected surfaces in verdict structure or via a separate tested helper.
6. **Given** `~/.platformio/penv/bin/pio test -e native -f test_ota_self_check_native` is executed **When** no ESP32 is attached **Then** the test suite runs on host and passes.
7. **Given** the existing hardware suite `firmware/test/test_ota_self_check/test_main.cpp` **When** inspected **Then** its scope is reduced to partition-API integration only (real `esp_ota_get_last_invalid_partition`, `esp_ota_mark_app_valid_cancel_rollback`) — pure decision-logic cases are removed — AND it still passes `pio test -f test_ota_self_check --without-uploading --without-testing` compile-only.
8. **Given** `pio run -e esp32dev` is executed from `firmware/` **When** the build completes **Then** binary size delta is ≤ 500 bytes vs. pre-refactor (pure refactor — no new on-device logic).
9. **Given** `OTASelfCheck.cpp` is inspected **When** scanned for heap allocation **Then** no `new`, `malloc`, `String`, `std::string`, or other dynamic allocation is present.

## Tasks / Subtasks

- [ ] **Task 1: Create OTASelfCheck header and implementation** (AC: #1, #2, #9)
  - [ ] Create `firmware/core/OTASelfCheck.h` with the `SelfCheckVerdict` enum, `SelfCheckInputs` struct, and `evaluateSelfCheck()` declaration. No Arduino/ESP-IDF includes — only `<cstdint>`.
  - [ ] Create `firmware/core/OTASelfCheck.cpp` implementing `evaluateSelfCheck()` as a pure function:
    - If `!inputs.pending_verify` → `AlreadyValid`
    - Else if `inputs.wifi_connected` → `CommitOk`
    - Else if `inputs.uptime_ms >= inputs.grace_window_ms` → `CommitTimeoutWarning`
    - Else → `Pending`
  - [ ] Decide how `rollback_detected` surfaces (either as a field on a richer verdict struct, or via a separate pure helper `bool shouldFlagRollback(const SelfCheckInputs&)` — document choice in Dev Notes)

- [ ] **Task 2: Refactor performOtaSelfCheck() to delegate** (AC: #3)
  - [ ] In `firmware/src/main.cpp::performOtaSelfCheck()` (lines ~584-651), construct `SelfCheckInputs` from existing locals: `pending_verify = s_isPendingVerify`, `wifi_connected = (g_wifiManager.getState() == WiFiState::STA_CONNECTED)`, `rollback_detected = g_rollbackDetected`, `uptime_ms = millis() - g_bootStartMs`, `grace_window_ms = OTA_SELF_CHECK_TIMEOUT_MS` (60000, main.cpp:206)
  - [ ] Switch on the returned `SelfCheckVerdict`: dispatch `esp_ota_mark_app_valid_cancel_rollback()` + `SystemStatus::set()` calls for each arm (preserve all existing log messages, OK/WARNING levels, throttling guards)
  - [ ] Preserve existing behavior exactly — no functional change on device

- [ ] **Task 3: Compile-check esp32dev build** (AC: #3, #8)
  - [ ] `~/.platformio/penv/bin/pio run -e esp32dev` from `firmware/` — clean build
  - [ ] Record binary size pre/post; delta ≤ 500 bytes

- [ ] **Task 4: Add [env:native] to platformio.ini** (AC: #4)
  - [ ] Append the `[env:native]` block to `firmware/platformio.ini` per Dev Notes

- [ ] **Task 5: Write native test suite** (AC: #5, #6)
  - [ ] Create `firmware/test/test_ota_self_check_native/test_main.cpp` with Unity `setUp()` / `tearDown()` / `main()` entry that invokes `UNITY_BEGIN()` / `UNITY_END()`
  - [ ] Write ≥ 7 test cases exercising the matrix above
  - [ ] Include only `<unity.h>` and `"OTASelfCheck.h"` — no Arduino headers

- [ ] **Task 6: Run native suite green** (AC: #6)
  - [ ] `~/.platformio/penv/bin/pio test -e native -f test_ota_self_check_native` — all cases pass on host

- [ ] **Task 7: Trim hardware suite to partition-API only** (AC: #7)
  - [ ] Audit `firmware/test/test_ota_self_check/test_main.cpp` — identify tests that exercise pure decision logic (duplicates of native suite) vs. partition-API integration
  - [ ] Remove pure-logic duplicates; retain tests that exercise real `esp_ota_*` calls
  - [ ] Document the trim rationale in a comment at the top of the test file

- [ ] **Task 8: Compile-check hardware suite** (AC: #7)
  - [ ] `pio test -f test_ota_self_check --without-uploading --without-testing` — compiles clean

- [ ] **Task 9: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run -e esp32dev` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] Native suite passes; hardware suite compiles clean

## Dev Notes

### Architecture Constraint
Per `CLAUDE.md`: ESP32 hardware tests require a physical device attached — slow feedback loop, blocks PR review. Pure decision logic should be host-testable. `evaluateSelfCheck()` is a classic extract-seam refactor: the function is side-effect-free, inputs are plain data, output is an enum. No Arduino or ESP-IDF dependency is needed to test it.

### Seam Extraction
`firmware/src/main.cpp:584-651` `performOtaSelfCheck()` currently mixes three concerns:
1. Read ESP-IDF state (`esp_ota_get_state_partition`, `g_wifiManager.getState()`, `millis()`)
2. Decide what to do (verdict)
3. Execute side effects (`esp_ota_mark_app_valid_cancel_rollback()`, `SystemStatus::set()`, `LOG_I`, `LOG_W`, `Serial.printf`)

Extract #2 into `evaluateSelfCheck()`. #1 and #3 stay in `main.cpp`.

### Constants Reference
- `OTA_SELF_CHECK_TIMEOUT_MS = 60000` at `firmware/src/main.cpp:206`
- `g_rollbackDetected` declared file-scope in `firmware/src/main.cpp`
- `g_bootStartMs` declared file-scope in `firmware/src/main.cpp`
- `s_isPendingVerify` static cache inside `performOtaSelfCheck()`

### Header Skeleton
```cpp
// firmware/core/OTASelfCheck.h
#pragma once
#include <cstdint>

enum class SelfCheckVerdict {
    Pending,
    CommitOk,
    CommitTimeoutWarning,
    AlreadyValid
};

struct SelfCheckInputs {
    bool pending_verify;
    bool wifi_connected;
    bool rollback_detected;
    uint32_t uptime_ms;
    uint32_t grace_window_ms;
};

SelfCheckVerdict evaluateSelfCheck(const SelfCheckInputs& inputs);
```

### platformio.ini Addition
```ini
[env:native]
platform = native
test_framework = unity
build_flags = -std=gnu++17 -DNATIVE_BUILD -I firmware/core
test_filter = test_ota_self_check_native
```

### rollback_detected Surfacing Decision
`rollback_detected` is orthogonal to the primary commit/pending/timeout verdict — the device may detect a rollback on a non-pending-verify boot. Two options:
- **Option A (preferred):** add a parallel pure helper `bool shouldFlagRollbackWarning(const SelfCheckInputs&)` returning `inputs.rollback_detected` (trivial, but testable in isolation and documents the contract).
- **Option B:** extend verdict to a struct `{ SelfCheckVerdict verdict; bool rollback_warning; }`.
Choose Option A unless verdict-struct return is already idiomatic here — Option A keeps the primary enum clean.

### Known Out of Scope
- GitHub Actions native workflow — deferred to follow-up story
- `web_portal_responsive` signal — confirmed does NOT exist in current codebase; not introduced here
- On-device self-check behavior changes — this story is a pure refactor; all observable behavior must remain identical

### References
- `firmware/src/main.cpp:584-651` — current `performOtaSelfCheck()` implementation
- `firmware/src/main.cpp:206` — `OTA_SELF_CHECK_TIMEOUT_MS` constant
- `firmware/test/test_ota_self_check/test_main.cpp` — existing hardware suite
- `_bmad-output/implementation-artifacts/stories/fn-1-4-ota-self-check-and-rollback-detection.md` — origin story for the self-check pattern
- `CLAUDE.md` — Testing section (Unity framework, `--without-uploading --without-testing` compile-only pattern)
- Winston's memory-model note (if applicable to the extracted pure function — trivially true: no shared state)

## File List

- `firmware/core/OTASelfCheck.h` (NEW) — pure header: `SelfCheckVerdict`, `SelfCheckInputs`, `evaluateSelfCheck()`
- `firmware/core/OTASelfCheck.cpp` (NEW) — pure implementation, no side effects, no dynamic allocation
- `firmware/src/main.cpp` (MODIFIED) — `performOtaSelfCheck()` delegates verdict to `evaluateSelfCheck()`
- `firmware/platformio.ini` (MODIFIED) — add `[env:native]` block
- `firmware/test/test_ota_self_check_native/test_main.cpp` (NEW) — ≥ 7 host-run Unity cases
- `firmware/test/test_ota_self_check/test_main.cpp` (MODIFIED) — trimmed to partition-API integration only
