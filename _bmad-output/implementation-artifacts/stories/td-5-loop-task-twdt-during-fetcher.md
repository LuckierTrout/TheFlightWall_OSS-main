# Story TD-5: loopTask Task-Watchdog Abort During Fetcher Operation

branch: td-5-loop-task-twdt-during-fetcher
zone:
  - firmware/src/main.cpp
  - firmware/adapters/OpenSkyFetcher.*
  - firmware/adapters/AeroAPIFetcher.*
  - firmware/core/FlightDataFetcher.*
  - firmware/test/test_loop_task_twdt/**

Status: draft

## Story

As a **firmware engineer**,
I want **the `loopTask` Task-Watchdog abort that fires during startup fetcher operation investigated and resolved**,
so that **the device does not reboot-loop during normal WiFi-connected boot, on-device testing completes reliably, and LE-1 measurement runs produce clean sustained data instead of being fragmented across ~24-second cycles**.

## Acceptance Criteria

1. **Given** a freshly flashed device on a WiFi network with valid OpenSky + AeroAPI credentials **When** the device boots and enters the startup fetcher sequence **Then** no TWDT abort fires on `loopTask` for at least 10 minutes of continuous uptime.

2. **Given** the investigation completes **When** the root cause is identified **Then** the specific blocking call (file, function, line) is documented in Dev Notes, along with why it exceeds the 5-second TWDT window.

3. **Given** the fix is applied **When** normal fetcher operation runs **Then** OpenSky token fetch + state-vector retrieval + AeroAPI enrichment all complete without any call blocking `loopTask` for more than 3 seconds (conservative margin below the 5 s TWDT).

4. **Given** the fix is applied **When** external APIs are slow or unreachable **Then** the fetcher degrades gracefully (timeout, retry budget, or circuit-breaker backoff) rather than blocking `loopTask` indefinitely.

5. **Given** a regression test or smoke script exists **When** the crash recurs in a future change **Then** the test detects it automatically (e.g., a 10-minute uptime check in CI smoke or a serial-log grep for `task_wdt: Task watchdog got triggered`).

6. **Given** the fix lands **When** `~/.platformio/penv/bin/pio run -e esp32dev` builds **Then** binary remains under 92% of OTA partition and no unrelated functionality regresses.

## Tasks / Subtasks

- [ ] **Task 1: Reproduce and archive the crash** (AC: #2)
  - [ ] Flash `esp32dev_le_baseline` (or `esp32dev` after LE-1.1 spike cleanup) on a hardware-powered device with network
  - [ ] Capture ≥ 60 s of serial output via `~/.platformio/penv/bin/python tests/smoke/... > /tmp/td5-repro.log`
  - [ ] Confirm the `task_wdt: Task watchdog got triggered` / `loopTask (CPU 1)` pattern
  - [ ] Archive a representative capture to `_bmad-output/planning-artifacts/td-5-repro.log` (or attach to the PR description)

- [ ] **Task 2: Locate the blocking call** (AC: #2)
  - [ ] Decode the backtrace PC `0x4010a8e8` against the baseline ELF — `xtensa-esp32-elf-addr2line -e .pio/build/esp32dev_le_baseline/firmware.elf 0x4010a8e8`
  - [ ] If the backtrace is corrupted (as observed during the V0 spike: `|<-CORRUPTED`), add temporary `Serial.printf` breadcrumbs around each fetcher call in `loop()` / `FlightDataFetcher::tick()` / `OpenSkyFetcher::*` / `AeroAPIFetcher::*` to bracket the blocking region
  - [ ] Also inspect `esp_task_wdt_reset()` call density in `loop()` — the watchdog may simply not be reset often enough during a long fetch even if no single call is individually > 5 s

- [ ] **Task 3: Categorize the root cause** (AC: #2)
  - [ ] One of: (a) a single HTTPClient call blocks too long due to slow remote or large payload, (b) TLS handshake + body read exceeds 5 s under weak WiFi, (c) loop is missing a watchdog reset between phases, (d) unbounded loop / retry in an error path, (e) other
  - [ ] Document verdict in Dev Notes

- [ ] **Task 4: Apply the minimum-viable fix** (AC: #1, #3, #4)
  - [ ] Possible fixes (in rough order of preference):
    - [ ] Add `esp_task_wdt_reset()` between the fetcher phases in `loop()`
    - [ ] Split long blocking fetch into smaller non-blocking phases with explicit yields
    - [ ] Tighten HTTPClient timeouts (`setConnectTimeout`, `setTimeout`) with a sane retry budget
    - [ ] Increase TWDT timeout for `loopTask` from 5 s to ~15 s via `esp_task_wdt_init()` / `esp_task_wdt_add()` (last resort — treats the symptom)
  - [ ] Prefer fixes that preserve watchdog protection rather than weakening it

- [ ] **Task 5: Verify sustained uptime** (AC: #1, #3)
  - [ ] Run the device for ≥ 10 minutes with the fix applied; capture serial log
  - [ ] Confirm `grep -c "task_wdt: Task watchdog got triggered"` returns 0
  - [ ] Confirm normal fetcher operation completes (state vectors + enrichment, flights rendered on LED wall)

- [ ] **Task 6: Regression guard** (AC: #5)
  - [ ] Add a simple smoke script at `tests/smoke/test_loop_task_twdt.py` that captures N seconds of serial and fails if it sees `task_wdt` or `loopTask (CPU 1)` patterns
  - [ ] Document invocation in AGENTS.md or the existing smoke-test section

- [ ] **Task 7: Build and verify** (AC: #6)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5 MB OTA partition, under 92% gate
  - [ ] Run existing test suites to confirm no regression: `pio test -e esp32dev --without-uploading --without-testing` compile-check

## Dev Notes

### Observation context (2026-04-17 V0 spike)

During the LE-0.1 spike measurement runs, the device exhibited a repeating ~24 s reboot loop even on the unmodified baseline build (ClassicCardMode). Pattern:

```
[Main] Startup progress active: connecting to WiFi
...
[WiFi] State: CONNECTING -> STA_CONNECTED
[NTP] Time synchronized
[ModeRegistry] Fade transition complete
[ModeRegistry] Active mode: classic_card   (or clock)
[Main] Startup: authenticating APIs
[Main] Startup: triggering first fetch immediately
[Main] Startup: fetching flights
OpenSkyFetcher: Fetching new token
OpenSkyFetcher: POST body length: 128
OpenSkyFetcher: Obtained access token, length: 1526
OpenSkyFetcher: Token expires in (s): 1800
OpenSkyFetcher: Token cached. Expires at ms: 1807584
...
E (23999) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
E (23999) task_wdt:  - loopTask (CPU 1)
E (23999) task_wdt: Tasks currently running:
E (23999) task_wdt: CPU 0: IDLE0
E (23999) task_wdt: CPU 1: IDLE1
E (23999) task_wdt: Aborting.
abort() was called at PC 0x4010a8e8 on core 0
Backtrace: 0x40083f61:0x3ffbed5c |<-CORRUPTED
rst:0xc (SW_CPU_RESET)
```

Key facts:
- **Not caused by the V0 spike.** Observed on both `esp32dev_le_baseline` (no `LE_V0_SPIKE`) and `esp32dev_le_spike` builds, with identical timing. Same PC.
- **Timing is surprisingly consistent** — abort fires at ~24 s uptime every cycle, immediately after the first OpenSky token fetch completes and the state-vector request should be starting.
- **Backtrace is corrupted** (`|<-CORRUPTED` marker) — suggests either stack damage from the blocking call, or unusual call depth that the stack unwinder can't follow. Temporary `Serial.printf` breadcrumbs may be more reliable than `addr2line` for locating the blocking call.
- **CPU 0 and CPU 1 both IDLE at abort time** — the tasks scheduled on them are not actively running, which is consistent with a blocking I/O call (HTTPClient / WiFiClientSecure waiting on the network).
- Reference: see `_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md` under "Known limitations & caveats" for original observation.

### Why this is worth fixing (not just working around)

- **Testing:** every on-device test run fragments into ~15-second windows before reboot, complicating sustained measurements (LE-1.9 p95 render, stability soak, heap-leak detection).
- **User-visible:** a reboot loop during normal WiFi-connected startup is a functional regression. Even if the device eventually settles into a working state after retries, the first-run experience is broken.
- **Root-cause hygiene:** the fact that `loopTask` is getting watchdogged despite calling `esp_task_wdt_reset()` somewhere in `loop()` (confirmed by the `[Main] Loop task enrolled in TWDT (5s timeout)` log) indicates a specific blocking call the team should understand, not a broad problem.

### Scope boundary

This story is investigation + fix, NOT a rewrite of the fetcher architecture. If the investigation reveals a deeper architectural issue (e.g., all fetch paths should migrate to async/non-blocking), surface that as a separate follow-up (TD-6+) rather than expanding TD-5's scope.

### Priority and sprint placement

- **Priority: P2 (non-blocking for LE-1, but high-value for testing quality).**
- **Recommended sprint:** N+1 alongside TD-3 — both are orthogonal to LE-1, both unblock later measurement quality. If N+1 capacity is tight, acceptable to defer to N+2.
- **Size: S–M** (hard to estimate without reproduction; hopefully ≤ 1 dev-day once the blocking call is located).

## File List

- `firmware/src/main.cpp` (modified — possibly add `esp_task_wdt_reset()` between fetcher phases, or adjust TWDT timeout in `setup()`)
- `firmware/adapters/OpenSkyFetcher.{h,cpp}` (possibly modified — HTTPClient timeout tightening)
- `firmware/adapters/AeroAPIFetcher.{h,cpp}` (possibly modified — same)
- `firmware/core/FlightDataFetcher.{h,cpp}` (possibly modified — tick/phase structure)
- `tests/smoke/test_loop_task_twdt.py` (new — regression smoke)
- `AGENTS.md` (modified — document smoke-test invocation)
- `_bmad-output/planning-artifacts/td-5-repro.log` (new — archived reproduction trace, optional)
