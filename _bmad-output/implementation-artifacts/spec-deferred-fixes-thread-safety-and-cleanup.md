---
title: 'Deferred fixes: thread safety, resource cleanup, and LOG consistency'
type: 'refactor'
created: '2026-04-03'
status: 'done'
baseline_commit: 'c55ee67'
context: []
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Three deferred code review findings remain unresolved: (1) SystemStatus String fields are read/written across cores without synchronization, risking heap corruption on ESP32; (2) aborted POST /api/settings requests leak stale entries in `g_pendingBodies`; (3) one dynamic log line uses raw `Serial.println` instead of the project's LOG macros. A fourth item (ConfigManager cross-core safety) was investigated and found already resolved — `g_configMutex` guards all reads/writes.

**Approach:** Add a FreeRTOS mutex to SystemStatus protecting `set()` and `toJson()`/`toExtendedJson()`. Register an `onDisconnect` handler for POST /api/settings to clean up stale pending body entries. Replace the `Serial.println` fetch summary with `LOG_I`.

## Boundaries & Constraints

**Always:** Preserve existing API contracts and behavior. Use RAII or paired take/give for mutex operations. Keep critical sections as short as possible.

**Ask First:** Any changes to SystemStatus that would alter the public API signature.

**Never:** Add new dependencies. Change ConfigManager (already thread-safe). Modify any web-facing response formats.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| SystemStatus concurrent set + read | Core 0 calls `set()` while Core 1 calls `toExtendedJson()` | Both complete without corruption; reader sees consistent snapshot | Mutex timeout returns stale data gracefully |
| Settings POST client disconnect mid-body | Client drops connection after first chunk | `onDisconnect` fires, stale entry removed from `g_pendingBodies` | No dangling pointer; vector stays clean |
| Settings POST normal completion | Full body received | Existing behavior unchanged — body parsed and responded | No change |

</frozen-after-approval>

## Code Map

- `firmware/core/SystemStatus.h` -- Subsystem status struct and class declaration; add mutex member
- `firmware/core/SystemStatus.cpp` -- `set()`, `toJson()`, `toExtendedJson()` implementations; wrap with mutex
- `firmware/adapters/WebPortal.cpp` -- POST /api/settings body handler; add `onDisconnect` cleanup for `g_pendingBodies`
- `firmware/src/main.cpp` -- Line ~702; replace `Serial.println` with `LOG_I`
- `firmware/utils/Log.h` -- LOG macro definitions (reference only)

## Tasks & Acceptance

**Execution:**
- [x] `firmware/core/SystemStatus.h` -- Add `static SemaphoreHandle_t _mutex;` member and `static void init();` if not present
- [x] `firmware/core/SystemStatus.cpp` -- Create mutex in `init()`, wrap `set()` body and `toJson()`/`toExtendedJson()` reads with `xSemaphoreTake`/`xSemaphoreGive`
- [x] `firmware/adapters/WebPortal.cpp` -- In POST /api/settings body handler, register `request->onDisconnect(...)` that calls `clearPendingBody(request)` when `index == 0`
- [x] `firmware/src/main.cpp` -- Replace `Serial.println("[Main] Fetch: ...")` at ~line 702 with `LOG_I("Main", ...)` (wrapped with `#if LOG_LEVEL >= 2` guard instead, since LOG macros use string literal concatenation)
- [x] `_bmad-output/implementation-artifacts/deferred-work.md` -- Remove resolved items, note ConfigManager item as already addressed

**Acceptance Criteria:**
- Given SystemStatus is used across cores, when `set()` and `toJson()` run concurrently, then no heap corruption or undefined behavior occurs (mutex serializes access)
- Given a client disconnects mid-POST to /api/settings, when the connection drops, then the stale `g_pendingBodies` entry is cleaned up
- Given LOG_LEVEL >= 3, when a flight fetch completes, then the summary line appears through LOG_I, not raw Serial
- Given `pio run` is executed, when compilation completes, then build succeeds with no new warnings

## Verification

**Commands:**
- `cd firmware && ~/.platformio/penv/bin/pio run` -- expected: SUCCESS, no new warnings from modified files

## Suggested Review Order

**Thread safety — SystemStatus mutex**

- Entry point: mutex member declaration added to class
  [`SystemStatus.h:54`](../../firmware/core/SystemStatus.h#L54)

- Mutex creation with failure diagnostic in init()
  [`SystemStatus.cpp:19`](../../firmware/core/SystemStatus.cpp#L19)

- set() wraps writes under mutex with best-effort fallback on timeout
  [`SystemStatus.cpp:32`](../../firmware/core/SystemStatus.cpp#L32)

- get() snapshots single status under mutex, degrades gracefully
  [`SystemStatus.cpp:46`](../../firmware/core/SystemStatus.cpp#L46)

- toJson() snapshots all 6 statuses under lock, serializes outside
  [`SystemStatus.cpp:57`](../../firmware/core/SystemStatus.cpp#L57)

**Resource cleanup — pending body leak on disconnect**

- onDisconnect handler cleans stale g_pendingBodies on client abort
  [`WebPortal.cpp:157`](../../firmware/adapters/WebPortal.cpp#L157)

**LOG consistency**

- Fetch summary wrapped in LOG_LEVEL >= 2 guard (LOG_I incompatible with dynamic String)
  [`main.cpp:702`](../../firmware/src/main.cpp#L702)

**Peripherals**

- deferred-work.md updated with resolved items and 2 new deferrals
  [`deferred-work.md:1`](./deferred-work.md#L1)
