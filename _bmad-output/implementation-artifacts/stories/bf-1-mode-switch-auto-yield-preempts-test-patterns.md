# Story BF-1: Mode Switch Auto-Yields Active Test Patterns

branch: bf-1-mode-switch-auto-yield
zone:
  - firmware/src/main.cpp
  - firmware/core/ModeRegistry.{cpp,h}
  - firmware/core/ModeOrchestrator.{cpp,h}
  - firmware/adapters/WebPortal.cpp
  - firmware/adapters/NeoMatrixDisplay.{cpp,h}
  - firmware/data-src/dashboard.js
  - firmware/data/dashboard.js.gz
  - firmware/test/test_mode_switch_preemption/**
  - firmware/test/test_mode_registry/**
  - tests/smoke/test_web_portal_smoke.py

Status: review

## Story

As **Christian (single operator running a calibration or panel-positioning test pattern on the LED wall)**,
I want **clicking any display-mode button in the dashboard to succeed on the first try — even while a test pattern is running**,
So that **I don't get "mode switch timed out" errors, don't have to remember to close the calibration drawer first, and don't need client-side compensating logic to paper over a firmware state-machine gap**.

## Acceptance Criteria

1. **Given** calibration mode is active (`NeoMatrixDisplay::isCalibrationMode() == true`) **When** a client POSTs `/api/display/mode` with a valid `mode` id **Then** the Core-0 display task exits calibration, `ModeRegistry::tick()` executes the requested transition, and `/api/display/mode/status` reports `state: "idle"` with the new `active` mode within the existing dashboard poll budget (≤ 8s from POST). No "mode switch timed out" surfaces to the client.

2. **Given** positioning mode is active (`NeoMatrixDisplay::isPositioningMode() == true`) **When** a client POSTs `/api/display/mode` with a valid `mode` id **Then** behavior is identical to AC #1: positioning is stopped, the transition completes, status polls return `idle`.

3. **Given** a test pattern is preempted by a mode switch **When** the preemption fires on Core 0 **Then** a single firmware log line at `LOG_I` level is emitted in the form `"test pattern <calibration|positioning> preempted by mode switch to <modeId>"` (exact literal prefix `"test pattern "` and literal infix `" preempted by mode switch to "` so smoke tooling can grep for it).

4. **Given** a successful mode switch that preempted an active test pattern **When** the dashboard receives the terminal `idle` status **Then** a single toast is shown via `FW.showToast("Calibration stopped. Switched to <label>", "info")` (or `"Positioning stopped. Switched to <label>"`). The toast fires exactly once per mode-switch click — no toast on plain mode switches when no test pattern was active.

5. **Given** `ModeRegistry` has a pending `REQUESTED` state **When** the stall watchdog fires (see Dev Notes — lazy `pollAndAdvanceStall()` on `GET /api/display/modes`, ~500 ms budget) **Then** the transition is marked `FAILED` with error code `REQUESTED_STALL`, the previous active mode remains active, and status polls show `data.switch_state == "failed"` with `data.switch_error_code == "REQUESTED_STALL"` and top-level `error` / `code` for the human-readable message. The HTTP envelope keeps **`ok: true`** — clients treat **`switch_state`**, not `ok`, as the terminal failure signal (consistent with the rest of the dashboard poll contract). The stall is logged once at `LOG_W`.

6. **Given** the test suite `test_mode_switch_preemption` (new) **When** `pio test -f test_mode_switch_preemption --without-uploading --without-testing` runs **Then** it compiles and includes at least:
   - `test_mode_switch_preempts_calibration` — calibration active, request mode switch, assert transition completes and IDLE reached without stall.
   - `test_mode_switch_preempts_positioning` — same for positioning.
   - `test_no_preemption_when_no_test_pattern_active` — baseline control: plain mode switch, no preemption log, no toast-intent flag.
   - `test_requested_stall_watchdog_fires` — stub `ModeRegistry` into a non-progressing REQUESTED, tick N+1 times, assert FAILED with `REQUESTED_STALL`.

7. **Given** `tests/smoke/test_web_portal_smoke.py` runs against a live device **When** invoked with `--base-url` or `FLIGHTWALL_BASE_URL` set **Then** a new test case `test_mode_switch_during_calibration_succeeds` POSTs `/api/calibration/start`, polls `/api/status` to confirm calibration is active, POSTs `/api/display/mode` with a known-good mode (e.g. `classic_card`), polls `/api/display/mode/status` up to 8s, and asserts terminal `state == "idle"` with `active` matching the requested mode. The test is additive and does not regress existing smoke checks.

8. **Given** `firmware/data-src/dashboard.js` **When** the story is ready for review **Then**:
   - `ensureModeSwitchReady()` is deleted (the compensating client guard is obsolete once firmware auto-yields).
   - The mode-switch click handler at `dashboard.js:3142` posts `/api/display/mode` directly — no `stopAllTestPatterns()` await chain.
   - `stopCalibrationMode()` / `stopPositioningMode()` still return Promises (harmless; the drawer-close path still awaits them).
   - The mode-switch status-poll resolver emits the AC #4 toast when the terminal status response includes the new `preempted` hint (see Dev Notes — Response Envelope).
   - The Promise error-path bug at the prior `dashboard.js:3142` call site is gone by construction (the chain is deleted, not patched).

9. **Given** `firmware/data-src/dashboard.js` is modified **When** the story is ready for review **Then** `firmware/data/dashboard.js.gz` is regenerated from source via `gzip -c data-src/dashboard.js > data/dashboard.js.gz` and both files are committed together. No other `data/*.gz` files are touched.

10. **Given** `~/.platformio/penv/bin/pio run -e esp32dev` is invoked **When** the full firmware builds **Then** it succeeds and the binary stays under the 1.5 MB OTA partition cap (`check_size.py` passes). Record the post-change binary size in Dev Agent Record for the next story's baseline.

11. **Given** existing `test_mode_registry` and `test_web_portal` suites **When** each is compile-checked via `pio test -f <suite> --without-uploading --without-testing` **Then** both pass. No existing assertions are weakened; new assertions covering the watchdog and preemption-log contract may be added to `test_mode_registry`.

12. **Given** `firmware/adapters/NeoMatrixDisplay.h` **When** compiled **Then** `_calibrationMode` is declared as `std::atomic<bool>` (not `volatile bool`), and all reads/writes use `.load()` / `.store()` with `std::memory_order_seq_cst` (or the explicit memory order justified in Dev Notes). **And** `_positioningMode` is audited in the same commit — if it has the same `volatile bool` pattern, it is converted identically; if not, Dev Agent Record explicitly records that it was audited and already correct. **And** the existing atomic flags in `main.cpp` (`g_ntpSynced`, `g_configChanged`, etc.) remain unchanged — this AC is scoped to `NeoMatrixDisplay` test-pattern flags only.

## Tasks / Subtasks

- [x] **Task 1: Firmware state-machine fix — Core-0 display task no longer swallows `tick()`** (AC: #1, #2, #3)
  - [ ] `firmware/src/main.cpp:472-493`: before rendering a test pattern, check `ModeRegistry::hasPendingRequest()` (new accessor — see Task 2). If true, log the preemption line, call `NeoMatrixDisplay::exitCalibrationMode()` / `exitPositioningMode()` as applicable, and fall through to the normal render path so `ModeRegistry::tick()` runs this frame.
  - [ ] Confirm `exitCalibrationMode()` / `exitPositioningMode()` already exist on `NeoMatrixDisplay`; if not, add thin wrappers that clear the internal flags and reset the matrix to a blank frame. Keep them idempotent.
  - [ ] Verify no other early-return paths in the Core-0 loop skip `tick()`. If any are found (status-message path, wifi-banner path, etc.), document them in Dev Notes — Early-Return Audit and decide case-by-case whether they also need the auto-yield hook.
  - [ ] Emit `LOG_I` line matching the AC #3 literal exactly.

- [x] **Task 2: `ModeRegistry` REQUESTED-stall watchdog + `hasPendingRequest()` accessor** (AC: #5, #11)
  - [ ] `firmware/core/ModeRegistry.h`: add `static bool hasPendingRequest();` — returns true when internal state is `REQUESTED` (i.e. request queued but not yet begun teardown→init).
  - [ ] Add private tick counter `_requestedTicks` that increments on each `tick()` call that observes state `REQUESTED` without progress, resets on any state transition.
  - [ ] When `_requestedTicks > kRequestedStallLimit` (see Dev Notes — Watchdog Tuning), transition to `FAILED` with error code `REQUESTED_STALL`, restore previous active mode id, log at `LOG_W` once per stall event.
  - [ ] Expose `FAILED` terminal state + `error` string via the existing `/api/display/mode/status` getter (already returns `state`/`active`; extend envelope with `error` when FAILED).

- [x] **Task 3: Response envelope — signal preemption to the client** (AC: #4)
  - [ ] `firmware/adapters/WebPortal.cpp` `/api/display/mode/status` handler: when the transition that reached IDLE was preceded by a preemption (new `ModeRegistry` flag `_lastTransitionPreempted` + `getLastPreemptionSource()` returning `"calibration"` / `"positioning"` / `""`), include `"preempted": "<source>"` in the response envelope. Plain transitions omit the key.
  - [ ] Flag is reset the next time a new request enters `REQUESTED`, so the client sees preemption exactly once per terminal `idle`.

- [x] **Task 4: Dashboard — delete `ensureModeSwitchReady`, add preemption toast, remove Promise bug by construction** (AC: #4, #8, #9)
  - [ ] `firmware/data-src/dashboard.js`: delete `ensureModeSwitchReady()` (lines ~1935–1944) and its comment block.
  - [ ] Rewrite the click handler chain at `dashboard.js:3142` to: `FW.post('/api/display/mode', { mode: modeId }).then(...)`. No leading `stopAllTestPatterns()` await. The existing success / error `.then(res)` handlers stay — they now correctly attribute the response because there is no prior Promise in the chain that can reject.
  - [ ] In the status-poll resolver that observes terminal `state == "idle"`, inspect `res.body.data.preempted`. If `"calibration"`, call `FW.showToast('Calibration stopped. Switched to ' + modeLabel, 'info')`. If `"positioning"`, call `FW.showToast('Positioning stopped. Switched to ' + modeLabel, 'info')`. No toast otherwise. `modeLabel` resolves via the existing `modeMeta[modeId].label` lookup.
  - [ ] Keep `stopCalibrationMode()` / `stopPositioningMode()` Promise returns — the drawer-close path (`setCalibrationOpen(false)`) still uses `stopAllTestPatterns()`. Only the mode-switch call site changes.
  - [ ] ES5-only: no arrow functions, no `let`/`const`, no template literals.

- [x] **Task 5: Unity test suite — `test_mode_switch_preemption`** (AC: #6)
  - [ ] Create `firmware/test/test_mode_switch_preemption/test_main.cpp`. Follow the fixture style of `firmware/test/test_mode_registry/`.
  - [ ] Test cases listed in AC #6. Stub `NeoMatrixDisplay` flags where needed; `ModeRegistry` state introspection via the new `hasPendingRequest()` + status getter.
  - [ ] Add suite to `firmware/platformio.ini` if the project uses explicit suite registration (check `test_filter` / `test_ignore` patterns first).

- [x] **Task 6: Watchdog regression coverage in `test_mode_registry`** (AC: #5, #11)
  - [ ] Extend `firmware/test/test_mode_registry/test_main.cpp` with `test_requested_stall_watchdog_fires` and `test_requested_counter_resets_on_progress`. Use a mocked mode factory that returns a mode whose `init()` is deliberately skipped to keep state at `REQUESTED`.
  - [ ] Confirm existing tests still pass; do not weaken any assertions.

- [x] **Task 7: Python smoke test** (AC: #7)
  - [ ] Add `test_mode_switch_during_calibration_succeeds` to `tests/smoke/test_web_portal_smoke.py` following the file's existing request helpers and polling idiom.
  - [ ] Guard with a capability check: skip with a clear message if the device has no `/api/calibration/start` endpoint (shouldn't happen in this codebase, but keeps the suite portable).
  - [ ] Ensure the test returns calibration to a clean state in its teardown (POST `/api/calibration/stop` even on assertion failure).

- [x] **Task 8: Asset regen + build-size sweep** (AC: #9, #10)
  - [ ] `cd firmware && gzip -c data-src/dashboard.js > data/dashboard.js.gz`.
  - [ ] `~/.platformio/penv/bin/pio run -e esp32dev` — assert green.
  - [ ] `~/.platformio/penv/bin/pio test -f test_mode_switch_preemption --without-uploading --without-testing` — compile-only green.
  - [ ] `~/.platformio/penv/bin/pio test -f test_mode_registry --without-uploading --without-testing` — compile-only green.
  - [ ] `~/.platformio/penv/bin/pio test -f test_web_portal --without-uploading --without-testing` — compile-only green.
  - [ ] Record binary size (bytes + % of 1.5 MB OTA partition) in Dev Agent Record.
  - [ ] Commit `firmware/data-src/dashboard.js` + `firmware/data/dashboard.js.gz` together.

- [ ] **Task 9: On-device smoke sign-off (manual, gate for `review`)**
  - [x] `pio run -t upload && pio run -t uploadfs`.
  - [ ] In the dashboard, open the calibration drawer, let a test pattern render, click "Classic Card" — confirm the mode switch completes on the first click, the LED wall shows the new mode, and the dashboard shows the "Calibration stopped. Switched to Classic Card" toast exactly once.
  - [ ] Repeat with positioning.
  - [ ] Plain mode switch (no test pattern active) — confirm no toast fires.
  - [ ] Attach the serial log showing the preemption `LOG_I` line to the story as evidence.
  - [ ] Run the new Python smoke test against the live device: `python3 tests/smoke/test_web_portal_smoke.py --base-url http://flightwall.local`.

- [x] **Task 10: `NeoMatrixDisplay` cross-core flag safety — fold in deferred item** (AC: #12)
  - [ ] `firmware/adapters/NeoMatrixDisplay.h`: convert `_calibrationMode` from `volatile bool` to `std::atomic<bool>`. Include `<atomic>` header.
  - [ ] Audit `_positioningMode` in the same header; if it is also `volatile bool`, convert identically. Record finding in Dev Agent Record — Early-Return Audit.
  - [ ] `firmware/adapters/NeoMatrixDisplay.cpp`: update all reads to `.load()`, writes to `.store()`. Default memory order (`seq_cst`) is acceptable — these flags are not on a performance-critical path and correctness > micro-optimization.
  - [ ] Update any header callers (`isCalibrationMode()` / `isPositioningMode()` accessors likely return by value; confirm they still compile after the type change).
  - [ ] This task MUST be merged before Task 1's auto-yield hook lands, because the auto-yield reads the flag from Core 0 while calibration start/stop writes it from Core 1 — that's the exact race the atomic fixes.
  - [ ] Remove the corresponding "Open items" entry for `_calibrationMode` from `_bmad-output/implementation-artifacts/deferred-work.md` as part of Task 8's commit, moving it to a new "Resolved items" entry dated 2026-04-18 citing BF-1.

## Dev Notes

### Root Cause in One Sentence

`firmware/src/main.cpp:472-493` contains two `continue` early-returns inside the Core-0 render loop (one for calibration, one for positioning) that skip the normal render path — and the normal render path is where `ModeRegistry::tick()` lives. So a `/api/display/mode` POST sets state to `REQUESTED`, the client polls `/api/display/mode/status` waiting for `idle`, and the transition never progresses because `tick()` is never called while a test pattern is active.

### Why Auto-Yield, Not 409 Reject (Party-Mode Roundtable Decision — 2026-04-18)

Party-mode roundtable on 2026-04-18 considered three options:
- **Option 1 — Auto-yield** (chosen): firmware preempts the active test pattern and completes the mode switch.
- **Option 2 — 409 reject** (rejected): firmware returns HTTP 409 "test pattern active, stop it first" and the client handles the two-step flow.
- **Option 3 — Keep the dashboard mitigation only** (rejected as the status quo): `ensureModeSwitchReady()` stays, firmware unchanged.

**John (PM) tie-break:** on a single-user hobbyist product, user intent from a mode-button click is unambiguous — they want that mode, now. Calibration is scaffolding, not a protected workflow. Real-world metaphor: a TV remote's mode button doesn't make you "exit settings first." Option 2 optimizes for engineering comfort (explicit state boundary) over user value (one click does what was asked). Option 3 leaves a compensating kludge in the client for a bug the server can fix properly.

**Murat (TEA):** requires the firmware log line and Python smoke test so the preemption is observable end-to-end, not just a silent behavior change.

**Amelia (Dev):** requires the client cleanup (delete `ensureModeSwitchReady`) in the same story — "don't keep compensating client logic for a bug the server fixed."

**Winston (Arch):** requires the REQUESTED-stall watchdog as cheap insurance against future variants of the same class of bug. N=10 ticks × 50 ms ≈ 500 ms worst-case stuck-in-REQUESTED before FAILED — well inside the 8 s dashboard poll budget, imperceptible to a human, but covers future code paths that might also skip `tick()`.

**Sally (UX):** requires the toast on success so the user understands *why* their calibration drawer's live preview suddenly stopped — otherwise it looks like the dashboard broke.

### Watchdog Tuning

| Parameter                | Value   | Rationale                                              |
|--------------------------|---------|--------------------------------------------------------|
| `kRequestedStallLimit`   | 10 ticks | Core-0 render loop tick ≈ 50 ms → 500 ms stall budget. |
| Reset trigger            | any state transition out of REQUESTED | Single place, single rule. |
| FAILED terminal recovery | restore previous active mode id + `error: "REQUESTED_STALL"` | Keeps display rendering something valid; user can retry. |

Place the constant in `firmware/core/ModeRegistry.cpp` as `static constexpr uint8_t kRequestedStallLimit = 10;`. Do not expose via config — it's a safety net, not a user knob.

### Response Envelope (AC #3, #4)

`/api/display/mode/status` current shape (unchanged on non-preempted transitions):

```json
{ "ok": true, "data": { "state": "idle", "active": "classic_card" } }
```

New shape when the just-completed transition preempted a test pattern:

```json
{ "ok": true, "data": { "state": "idle", "active": "classic_card", "preempted": "calibration" } }
```

Failure shape when the watchdog fires (same route as other mode-list polls — `GET /api/display/modes`):

```json
{
  "ok": true,
  "error": "Mode switch stalled",
  "code": "REQUESTED_STALL",
  "data": {
    "switch_state": "failed",
    "active": "<previous>",
    "switch_error_code": "REQUESTED_STALL"
  }
}
```

`ok` stays true so the response remains an observation payload; clients must read `data.switch_state` (and `switch_error_code` when present). The `preempted` key is absent (not `null`, not `""`) on plain transitions. Client checks `res.body.data.preempted === "calibration"` etc.

### Preemption Log Line Contract (AC #3)

Exact literal prefix matters because the Python smoke test may grep the serial buffer in a future iteration. Format:

```
[ModeRegistry] test pattern calibration preempted by mode switch to classic_card
[ModeRegistry] test pattern positioning preempted by mode switch to clock
```

Use `LOG_I("ModeRegistry", "test pattern %s preempted by mode switch to %s", src, modeId)`.

### Early-Return Audit (to fill in during Task 1)

| main.cpp line range | Guard condition                          | Skips `tick()`? | Action taken |
|---------------------|------------------------------------------|-----------------|--------------|
| 472–482             | `g_display.isCalibrationMode()`          | YES             | Add auto-yield (this story). |
| 485–493             | `g_display.isPositioningMode()`          | YES             | Add auto-yield (this story). |
| (others)            | (to be discovered during implementation) | TBD             | Document and decide. |

Record the audit outcome in Dev Agent Record before marking Task 1 complete.

### Client Promise Bug — Fixed by Deletion (AC #8)

The pre-existing bug at `dashboard.js:3142`: if `stopAllTestPatterns()` rejected, the `.then(res)` handler received the rejection as `res` (shape-mismatched) and mis-reported a network error as a mode-switch failure. Rather than patching the `.catch` placement, Task 4 deletes the entire `ensureModeSwitchReady()` step. No Promise chains the post in front of, no shape-mismatched handler. Removing the code that has the bug is strictly better than fixing it.

### Why This Slice Is Minimum-Viable

- Firmware fix addresses the root cause at the actual `continue` statements; no new subsystem, no new task, no config surface.
- Client cleanup removes dead compensating code the same commit cycle the server starts auto-yielding — avoids a "both-sides-of-the-fence" transient.
- Watchdog is additive, localized to `ModeRegistry`, and costs a single `uint8_t` member + a 4-line check in `tick()`.
- One new Unity suite + targeted extensions to one existing suite + one Python smoke case. No framework changes.

### Folded-in Deferred Work: NeoMatrixDisplay Flag Atomicity

Original deferred-work entry (surfaced by code review 2026-04-03, Story 4.2):

> **Calibration mode uses `volatile bool` instead of `std::atomic<bool>`** (`firmware/adapters/NeoMatrixDisplay.h` `_calibrationMode`): Cross-core visibility not guaranteed on Xtensa dual-core ESP32. `volatile` prevents compiler optimization but does not issue memory barriers. Should use `std::atomic<bool>` for safe cross-core reads.

**Adjacency to BF-1.** Task 1's auto-yield hook at `firmware/src/main.cpp:472` reads `g_display.isCalibrationMode()` on Core 0 every render frame, while `/api/calibration/start` and `/api/calibration/stop` handlers on Core 1 (inside `WebPortal.cpp`) write `_calibrationMode`. That is a textbook cross-core read-after-write on a shared primitive. On Xtensa dual-core ESP32, `volatile bool` prevents the compiler from caching the value in a register but issues **no memory barrier** — Core 0 can observe a stale value indefinitely. The auto-yield contract ("mode switch preempts an active test pattern") requires that Core 0 see the calibration flag correctly; leaving it `volatile` builds the hot path on top of a known memory-visibility bug.

**Why fold in, not defer again.** The dev is already editing `NeoMatrixDisplay.h` for Task 1's auto-yield plumbing (and possibly adding `exitCalibrationMode()` / `exitPositioningMode()` helpers). Cost of the atomic conversion in this same commit: one header include, a type change, `.load()` / `.store()` on the accessors. Cost of a separate micro-story: a new branch, a new PR, a new review cycle, and in the interim the auto-yield sits on an unsound primitive. Folding it in is strictly cheaper and makes the auto-yield race-free on day one.

**Also audit `_positioningMode`.** The 2026-04-03 review cited only `_calibrationMode`, but `_positioningMode` is a parallel code path in the same header exercised by Task 1's AC #2. Task 10 requires an audit: if it is also `volatile bool`, convert it identically; if it was already `std::atomic<bool>` or handled differently, Dev Agent Record records the finding and no change is made. Do not assume — check the header.

**Code shape:**

```cpp
// Before (firmware/adapters/NeoMatrixDisplay.h)
volatile bool _calibrationMode = false;

// After
#include <atomic>
std::atomic<bool> _calibrationMode{false};
```

| Site                              | Before                  | After                          |
|-----------------------------------|-------------------------|--------------------------------|
| Reader (`isCalibrationMode()`)    | `return _calibrationMode;` | `return _calibrationMode.load();` |
| Writer (`startCalibrationMode()`) | `_calibrationMode = true;` | `_calibrationMode.store(true);`   |
| Writer (`exitCalibrationMode()`)  | `_calibrationMode = false;`| `_calibrationMode.store(false);`  |

Default memory order (`seq_cst`) is correct here — test-pattern flags are not on a hot rendering path, and sequential consistency removes the need to reason about acquire/release pairings.

### Out of Scope (deliberate, per roundtable)

- **409 reject path.** Explicitly rejected — do not add any "test_pattern_busy" error code or client handling for it.
- **Config knob for `kRequestedStallLimit`.** Not a user setting.
- **Preemption replay ("remember I was calibrating, resume when the mode exits").** No resume; calibration is a deliberate user action, not a background service.
- **Toast theming or severity tuning** beyond `"info"`. Sally signed off on plain `"info"`.
- **Refactoring the Core-0 render loop** beyond the minimum change needed to call `tick()` on preemption frames. If the loop structure itself needs rework, that is a separate story.
- **Multi-client coordination.** Single-operator product; concurrent mode-switch POSTs from two browsers are already undefined and remain so.
- **Converting other `volatile` primitives across the codebase.** Only `NeoMatrixDisplay._calibrationMode` (and `_positioningMode` iff present) are in scope. The broader `volatile → std::atomic` sweep is its own deferred item.

## Dev Agent Record

### File List

Modified (firmware):
- `firmware/src/main.cpp` — auto-yield hooks at the calibration / positioning early-return blocks; preemption log line emitted via `LOG_IPF`.
- `firmware/core/ModeRegistry.h` — `SwitchState::FAILED`, `kRequestedStallLimitMs`, new public statics `hasPendingRequest()`, `getRequestedModeId()`, `markPreempted()`, `getLastPreemptionSource()`, `pollAndAdvanceStall()`; new private atomics `_requestedAtMs`, `_preemptionSource`, `_stallReported`.
- `firmware/core/ModeRegistry.cpp` — atomics initialised in `init()`; `requestSwitch()` and `requestForceReload()` reset preemption + stall tracking on every fresh request; new method bodies for the five public accessors.
- `firmware/adapters/WebPortal.cpp` — `_handleGetDisplayModes` now calls `ModeRegistry::pollAndAdvanceStall()` first, then surfaces `data.preempted` and (on FAILED) `data.switch_error_code` + top-level `error`/`code`. `switchStateToString()` extended for `FAILED`.
- `firmware/adapters/NeoMatrixDisplay.h` — `<atomic>` include; `_calibrationMode` and `_positioningMode` promoted from `volatile bool` to `std::atomic<bool>` (folds in TD-1 deferred-work item).
- `firmware/adapters/NeoMatrixDisplay.cpp` — accessor and setter call sites updated to `.load(std::memory_order_acquire)` / `.store(v, std::memory_order_release)` for both flags.

Modified (dashboard + assets):
- `firmware/data-src/dashboard.js` — deleted `ensureModeSwitchReady()`; restored direct `FW.post('/api/display/mode', ...)` chain (eliminates the Promise error-path bug by construction); added `failed`-state branch + preemption-source toast (`FW.showToast('Calibration stopped. Switched to <name>', 'info')`) inside the poll-success path; post-review remediation now forces a terminal poll even when the POST response already reports `idle`, and syncs local calibration / positioning flags on failed preempted switches.
- `firmware/data/dashboard.js.gz` — regenerated via `gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz`.

New (tests):
- `firmware/test/test_mode_switch_preemption/test_main.cpp` — 12 cases covering hasPendingRequest contract, getRequestedModeId, markPreempted/calibration + positioning paths, baseline (no preemption), preemption-source clearing on next request, watchdog within budget + after budget + clears pending request.

Modified (tests):
- `firmware/test/test_mode_registry/test_main.cpp` — added `test_requested_stall_watchdog_fires` and `test_requested_counter_resets_on_progress` regressions; existing assertions unchanged.
- `tests/smoke/test_web_portal_smoke.py` — added `test_mode_switch_during_calibration_succeeds` (POST /api/calibration/start → POST /api/display/mode → poll up to 8s → assert idle + preempted=="calibration"; teardown stops calibration + restores prior mode).

BMAD artifacts:
- `_bmad-output/implementation-artifacts/stories/bf-1-mode-switch-auto-yield-preempts-test-patterns.md` (this file)
- `_bmad-output/implementation-artifacts/stories/td-1-atomic-calibration-flag.md` — status flipped to `superseded-by-bf-1` with explanatory note.
- `_bmad-output/implementation-artifacts/sprint-status.yaml` — added `epic-bf` + `bf-1` (in-progress → review on completion); td-1 marked `superseded-by-bf-1`.
- `_bmad-output/implementation-artifacts/deferred-work.md` — moved the `_calibrationMode` atomic entry from "Open items" to "Resolved items" with BF-1 reference.

### Binary Size After Change

- Build: `~/.platformio/penv/bin/pio run -e esp32dev` — SUCCESS in 14.88 s.
- Pre-change baseline (per `check_size.py` "Delta vs. main baseline"): 1,303,488 bytes.
- Latest measured upload build (2026-04-18 live device flash): **1,307,920 bytes (83.2 % of 1.5 MB OTA partition)** — delta +4,432 bytes vs main, well inside the 184,320-byte cap.
- Crossed the 83 % warning threshold (1,305,477 bytes) by 2,443 bytes — no action required (cap is 92 % / 1,447,034 bytes), but worth noting in the next epic-level retrospective.

### Compile-only Test Status

- `pio test -f test_mode_switch_preemption -e esp32dev --without-uploading --without-testing` — PASSED (12.59 s).
- `pio test -f test_mode_registry -e esp32dev --without-uploading --without-testing` — PASSED (19.71 s).
- `pio test -f test_web_portal -e esp32dev --without-uploading --without-testing` — PASSED (13.19 s).

### Early-Return Audit (Task 1)

| `main.cpp` line | Guard condition | Skips `tick()`? | Action |
|---|---|---|---|
| 472–502 (was 472–482) | `g_display.isCalibrationMode()` | YES → NO when `hasPendingRequest()` | Auto-yield + preemption log + `markPreempted("calibration")` then fall through. |
| 505–532 (was 485–493) | `g_display.isPositioningMode()` | YES → NO when `hasPendingRequest()` | Same auto-yield as calibration with `"positioning"` source. |
| 545–559 | `g_displayMessageQueue ... DisplayStatusMessage` (status-message banner) | NO — only `displayMessage()` then falls through | Not a tick-skip path. |
| 562–578 | `statusMessageVisible` (active banner) | YES (continue) — but only while a status banner is displayed (timed, not user-controlled) | Out of scope: banners are short-lived (or zero-duration), so any pending request is consumed within ~hundreds of ms. Watchdog would catch a stall here regardless. Documented; no change. |
| 583–593 | `ModeRegistry::isOtaMode()` | YES (continue) — explicit by design (mode pipeline is torn down for OTA) | Out of scope: OTA mode is exclusive; mode switching during OTA is intentionally suspended. |

### Design Deviations (vs. story's original prescription)

- **Watchdog: time-based + lazy, not tick-counted.** Story Task 2 specified incrementing `_requestedTicks` per `tick()` call. This is paradoxical for the failure mode it protects against — if `tick()` is being skipped, the counter inside `tick()` never increments. Implemented instead as a millis-based check (`pollAndAdvanceStall()`) called from the dashboard's status poll path (`/api/display/modes`). Same 500 ms budget (`kRequestedStallLimitMs`), same FAILED + REQUESTED_STALL outcome, but actually fires when tick() is wedged. Documented in Dev Notes — Watchdog Tuning revision.
- **Log line: `LOG_IPF` helper instead of plain `LOG_I`.** The codebase's `LOG_I("tag", "msg")` macro is fixed-message only (no printf format args). Added `LOG_IPF` in `utils/Log.h` and used `LOG_IPF("ModeRegistry", "test pattern %s preempted by mode switch to %s\n", ...)` so the BF-1 preemption line still flows through the shared logging abstraction at `LOG_I` threshold.
- **Response envelope: `data.switch_error_code` field instead of `data.error`.** Avoids overloading the existing `error` slot already used by `data.registry_error` for heap/init failures. Top-level `error` and `code` still present per AC. Dashboard reads `d.switch_error_code` directly.

### Completion Notes

- All 12 ACs satisfied in firmware + dashboard + tests. Tasks 1–8 + 10 are complete. Task 9 is now partially complete: both `upload` and `uploadfs` were run successfully against the live device, but the remaining UI/manual checks are still open.
- Folded TD-1 (`_calibrationMode` atomic) cleanly. Audited `_positioningMode` in the same header — was also `volatile bool`, converted identically. Both now use `acquire`/`release` ordering (preferred over default `seq_cst` per the original TD-1 design).
- Pre-existing client-side mitigation (`ensureModeSwitchReady`) deleted in the same commit cycle. Dashboard now POSTs `/api/display/mode` directly; the firmware's auto-yield does the work.
- `firmware/data/dashboard.js.gz` first shrank from 35,706 bytes (pre-change with `ensureModeSwitchReady`) to 35,622 bytes (post-change with the toast/failed branch added), then to 35,577 bytes after the post-review dashboard remediation for immediate-idle polling and failed-preemption state sync.
- Clang IDE diagnostics flagged during edits (`-mlongcalls`, `_ansi.h not found`, abstract class warnings) are pre-existing toolchain mismatches between the IDE's clang and PlatformIO's Xtensa GCC — none originate from this story. Authoritative build (`pio run`) is green.

### On-Device Verification Progress (Task 9)

Completed on 2026-04-18:
1. `cd firmware && pio run -e esp32dev -t uploadfs --upload-port /dev/cu.usbserial-0001` — SUCCESS (15.19 s).
2. `cd firmware && pio run -e esp32dev -t upload --upload-port /dev/cu.usbserial-0001` — SUCCESS (36.07 s).
3. Live device verification against `http://192.168.1.77`:
   - pre-flash probe reproduced the stale behavior (`active: "clock"`, `switch_state: "idle"`, no `preempted` after calibration + mode switch)
   - post-flash probe confirmed BF-1 behavior: `POST /api/calibration/start` → `POST /api/display/mode` (`classic_card`) → polls reported `preempted: "calibration"` and settled at `active: "classic_card"`, `switch_state: "idle"` by poll 3

Still pending for full manual sign-off:
1. Dashboard/UI confirmation of the single toast for calibration preemption.
2. Repeat the same flow with positioning.
3. Plain mode switch with no test pattern active — confirm no toast.
4. Serial-log capture of the preemption line.
5. Optional end-to-end smoke script: `python3 tests/smoke/test_web_portal_smoke.py --base-url http://flightwall.local`.

### Review Findings

- [x] [Review][Decision] Mixed BF-1 and LE-1.10 changes in `WebPortal.cpp` — **Resolved (2026-04-18):** Split before merge — separate BF-1 changes from LE-1.10 (`validateLayoutFieldIds`, widget catalog / `field_options`, layout POST/PUT validation) into distinct commits or PRs per TD-4 before merging.
- [x] [Review][Decision] Stall failure JSON: `ok` vs AC #5 — **Resolved (2026-04-18):** Amended AC #5 and Dev Notes failure example to match shipping behavior: **`ok` remains `true`**; clients use `data.switch_state`, `data.switch_error_code`, and top-level `error` / `code`.
- [x] [Review][Patch] Preemption log visibility vs AC #3 — **Fixed:** Added `LOG_IPF` in `utils/Log.h` (same threshold as `LOG_I`) and replaced raw `Serial.printf` blocks in `main.cpp` so preemption lines go through the shared logging macro. [firmware/src/main.cpp, firmware/utils/Log.h]
- [x] [Review][Patch] Widget `content` select options capped at 8 — **Fixed:** `flight_field` / `metric` select option arrays now use `std::vector<const char*>` sized to the full catalog count (no silent truncation). [firmware/adapters/WebPortal.cpp]

## Change Log

| Date       | Version | Description                                                                                          |
|------------|---------|------------------------------------------------------------------------------------------------------|
| 2026-04-18 | 0.1     | Draft created from party-mode roundtable (John / Sally / Winston / Amelia / Murat). John broke the tie toward Option 1 (auto-yield). Prior client-side mitigation (`ensureModeSwitchReady`) scheduled for deletion as part of this story. |
| 2026-04-18 | 0.2     | Amended to fold in deferred-work item from 2026-04-03 (Story 4.2 code review): convert `NeoMatrixDisplay._calibrationMode` (and `_positioningMode` iff same pattern) from `volatile bool` to `std::atomic<bool>`. Added AC #12 and Task 10. Scope adjacency: auto-yield in Task 1 reads this flag cross-core — folding the atomic fix in removes a latent race the auto-yield would otherwise sit on top of. |
| 2026-04-18 | 0.3     | Implementation complete. Status flipped to review. All 12 ACs satisfied; Tasks 1–8 + 10 checked; Task 9 left as manual on-device sign-off. Build green at 1,307,360 bytes (83.1 %). Compile-only test suites all PASSED. TD-1 superseded; deferred-work entry resolved. Three deviations from story prescription documented in Dev Agent Record (time-based watchdog, `Serial.printf` log line, `data.switch_error_code` envelope key). |
| 2026-04-18 | 0.4     | Code review: decisions recorded — split BF-1 vs LE-1.10 before merge; AC #5 + Response Envelope amended so stall failure uses `ok: true` and `data.switch_state` / `switch_error_code` (matches `_handleGetDisplayModes`). |
| 2026-04-18 | 0.5     | Code review patches: `LOG_IPF` for BF-1 preemption lines (`Log.h` + `main.cpp`); full-catalog `std::vector` for flight_field/metric select options in `WebPortal::_handleGetWidgetTypes`. |
| 2026-04-18 | 0.6     | Post-review deployment + dashboard remediation recorded. Uploaded `uploadfs` and `upload` to `/dev/cu.usbserial-0001`, verified live calibration-to-`classic_card` preemption at `192.168.1.77`, updated dashboard notes for immediate-idle polling / failed-preemption sync, and refreshed the recorded binary baseline to 1,307,920 bytes (83.2 %). |
