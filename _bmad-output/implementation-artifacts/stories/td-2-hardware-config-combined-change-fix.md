# Story TD-2: hardwareConfigChanged Combined Geometry+Mapping Fix

branch: td-2-hw-config-combined-fix
zone:
  - firmware/src/main.cpp
  - firmware/test/test_display_task_reconfigure/**

Status: draft

## Story

As a **device user**,
I want **when I change both display geometry and mapping in one settings save, both changes to apply (geometry via reboot, mapping via live reconfig)**,
So that **config saves are not silently partially-applied**.

## Acceptance Criteria

1. **Given** both `hardwareGeometryChanged(localHw, newHw)` and `hardwareMappingChanged(localHw, newHw)` return true **When** `displayTask` processes the config change **Then** `g_display.reconfigureFromConfig()` is invoked exactly once AND the reboot-required log is emitted.
2. **Given** only geometry changes **When** `displayTask` processes the config change **Then** the reboot-required log is emitted AND `reconfigureFromConfig()` is NOT called.
3. **Given** only mapping changes **When** `displayTask` processes the config change **Then** `reconfigureFromConfig()` is called AND no reboot-required log is emitted.
4. **Given** neither geometry nor mapping changes **When** `displayTask` processes the config change **Then** no log is emitted and `reconfigureFromConfig()` is NOT called.
5. **Given** a mapping change occurs AND a status message is currently visible **When** `reconfigureFromConfig()` succeeds **Then** the status message is redrawn via `g_display.displayMessage(...)` + `g_display.show()` (preserving current mapping-only-change behavior).
6. **Given** the test file `firmware/test/test_display_task_reconfigure/test_main.cpp` exists **When** tests are run **Then** all four combinations (none / geometry-only / mapping-only / both) are covered and pass.
7. **Given** `~/.platformio/penv/bin/pio run` is executed from `firmware/` **When** the build completes **Then** the build is clean and `pio test -f test_display_task_reconfigure --without-uploading --without-testing` compiles clean.

## Tasks / Subtasks

- [ ] **Task 1: Write failing regression test** (AC: #1, #2, #3, #4, #6)
  - [ ] Create `firmware/test/test_display_task_reconfigure/test_main.cpp` with 4 test cases: none / geometry-only / mapping-only / both
  - [ ] The `both` case must FAIL against current code (reproduces the bug — mapping branch short-circuited by `else if`)
  - [ ] `pio test -f test_display_task_reconfigure --without-uploading --without-testing` compiles clean

- [ ] **Task 2: Apply fix to displayTask** (AC: #1, #2, #3, #4, #5)
  - [ ] In `firmware/src/main.cpp` lines ~399-425, restructure the branching per the Dev Notes code block — evaluate `geometryChanged` and `mappingChanged` independently, apply both effects
  - [ ] Preserve the status-message redraw for the mapping branch

- [ ] **Task 3: Verify tests pass** (AC: #6)
  - [ ] Re-run the test suite — all four cases pass

- [ ] **Task 4: Audit related call sites** (AC: all)
  - [ ] Grep `hardwareConfigChanged`, `hardwareGeometryChanged`, `hardwareMappingChanged` across `firmware/` for any other branching that mirrors the same bug
  - [ ] Document findings in Dev Notes (even if none)

- [ ] **Task 5: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] All new/existing tests pass

## Dev Notes

### Bug Location
`firmware/src/main.cpp:399-425` inside `displayTask`:
```cpp
if (hardwareConfigChanged(localHw, newHw)) {
    if (hardwareGeometryChanged(localHw, newHw)) {
        localHw = newHw;
        LOG_I(...); // reboot-required path
    } else if (hardwareMappingChanged(localHw, newHw)) {  // BUG: else if
        localHw = newHw;
        g_display.reconfigureFromConfig();
    }
}
```
The `else if` means when both geometry and mapping changed in a single settings save, the mapping branch is skipped and `reconfigureFromConfig()` is never called. Geometry requires reboot to apply, so the mapping change silently never takes effect until the user reboots.

### Target Code
```cpp
if (hardwareConfigChanged(localHw, newHw)) {
    bool geometryChanged = hardwareGeometryChanged(localHw, newHw);
    bool mappingChanged  = hardwareMappingChanged(localHw, newHw);
    localHw = newHw;
    if (mappingChanged) {
        if (g_display.reconfigureFromConfig()) {
            LOG_I("DisplayTask", "Display mapping reconfigured live");
            if (statusMessageVisible && lastStatusText[0] != '\0') {
                g_display.displayMessage(String(lastStatusText));
                g_display.show();
            }
        } else {
            LOG_W("DisplayTask", "reconfigureFromConfig failed");
        }
    }
    if (geometryChanged) {
        LOG_I("DisplayTask", "Display geometry changed; reboot required to apply layout");
    }
}
```

### Architecture Constraint
Per `CLAUDE.md`: ConfigManager writes run on Core 1, `displayTask` runs on Core 0. Config changes propagate via the existing snapshot pattern (`newHw` retrieved via `ConfigManager::getHardware()`). This fix is a pure branching-logic correction — no new cross-core synchronization needed.

### Test Strategy
The test harness for `displayTask` decision logic is thin today. Task 1 creates a new test file. Since `displayTask` loops over FreeRTOS queues and is not directly unit-testable, extract the branching decision into a pure helper (or test via the existing `hardwareConfigChanged` / `hardwareGeometryChanged` / `hardwareMappingChanged` helpers) and assert the expected side-effect sequence. If a pure helper is needed, keep it in `firmware/src/main.cpp` behind an `#ifndef PIO_UNIT_TESTING` or extract to a small header.

### References
- `firmware/src/main.cpp:399-425` — buggy branching in displayTask
- `firmware/src/main.cpp` — `hardwareConfigChanged`, `hardwareGeometryChanged`, `hardwareMappingChanged` helpers
- `firmware/adapters/NeoMatrixDisplay.cpp::reconfigureFromConfig()` — live-reconfig implementation
- `CLAUDE.md` — Configuration System section (reboot-required NVS keys)

## File List

- `firmware/src/main.cpp` (MODIFIED) — fix `else if` → independent `if` branches in displayTask, evaluate geometry + mapping flags independently
- `firmware/test/test_display_task_reconfigure/test_main.cpp` (NEW) — 4-case regression test (none / geometry-only / mapping-only / both)
