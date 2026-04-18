# Story TD-1: Atomic Calibration Flag

branch: td-1-atomic-calibration-flag
zone:
  - firmware/adapters/NeoMatrixDisplay.*
  - firmware/test/test_neomatrix_display/**

Status: superseded-by-bf-1

> **Superseded 2026-04-18.** Folded into BF-1 (`bf-1-mode-switch-auto-yield-preempts-test-patterns`) as Task 10 / AC #12. Both `_calibrationMode` and `_positioningMode` were converted to `std::atomic<bool>` with explicit `memory_order_acquire` / `memory_order_release` ordering as part of the auto-yield work — the auto-yield path reads `isCalibrationMode()` from Core 0 while Core 1 toggles it, so fixing the atomic was a hard prerequisite for that story rather than independent tech debt.

## Story

As a **firmware engineer**,
I want **the cross-core calibration flag to be safe under the Xtensa memory model**,
So that **no torn reads or lost updates occur between the Core 0 render loop and Core 1 config writer**.

## Acceptance Criteria

1. **Given** `NeoMatrixDisplay.h` declares the calibration flag **When** the code is inspected **Then** `_calibrationMode` is declared as `std::atomic<bool> _calibrationMode{false};` with explicit acquire/release memory ordering at all access sites.
2. **Given** the header is compiled **When** the translation unit is built **Then** `#include <atomic>` is present in `NeoMatrixDisplay.h`.
3. **Given** a grep across `firmware/` for `_calibrationMode` **When** results are reviewed **Then** every read uses `.load(std::memory_order_acquire)` (or `memory_order_relaxed` if in an IRAM_ATTR ISR context) and every write uses `.store(v, std::memory_order_release)` — no raw bool access remains.
4. **Given** an audit of `NeoMatrixDisplay.{h,cpp}` **When** scanning for other cross-core `volatile bool` flags **Then** findings are documented in Dev Notes (either none remain, or a follow-up note is written).
5. **Given** `~/.platformio/penv/bin/pio run` is executed from `firmware/` **When** the build completes **Then** the build is clean and binary size delta is ≤ 200 bytes versus pre-change.
6. **Given** `pio test -f test_neomatrix_display --without-uploading --without-testing` is executed **When** the compile completes **Then** the test suite compiles clean.
7. **Given** the firmware is flashed to a device **When** the calibration toggle is pressed in the dashboard **Then** the mode change takes effect within one render frame.

## Tasks / Subtasks

- [ ] **Task 1: Audit existing calibration flag usage** (AC: #3)
  - [ ] Grep `_calibrationMode` across `firmware/` — enumerate every read/write site
  - [ ] Identify any IRAM_ATTR / ISR contexts that read the flag (note for memory-order selection)

- [ ] **Task 2: Promote flag to std::atomic<bool>** (AC: #1, #2)
  - [ ] In `firmware/adapters/NeoMatrixDisplay.h`, add `#include <atomic>`
  - [ ] Change `volatile bool _calibrationMode = false;` (line ~59) to `std::atomic<bool> _calibrationMode{false};`
  - [ ] Verify `isCalibrationMode() const` still compiles — `atomic<bool>::load() const` is const-qualified

- [ ] **Task 3: Update all call sites with explicit memory ordering** (AC: #3)
  - [ ] In `NeoMatrixDisplay.h` / `NeoMatrixDisplay.cpp`, replace every read with `_calibrationMode.load(std::memory_order_acquire)`
  - [ ] Replace every write with `_calibrationMode.store(v, std::memory_order_release)`
  - [ ] If any reader is in IRAM_ATTR / ISR context, downgrade that site to `std::memory_order_relaxed` and document why

- [ ] **Task 4: Audit remaining cross-core volatile bool flags** (AC: #4)
  - [ ] Grep `volatile bool` in `NeoMatrixDisplay.{h,cpp}`
  - [ ] Document findings in Dev Notes section of this story on completion (even if none remain)

- [ ] **Task 5: Build and size check** (AC: #5)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Record binary size pre/post; delta ≤ 200 bytes

- [ ] **Task 6: Extend NeoMatrixDisplay test suite** (AC: #6)
  - [ ] In `firmware/test/test_neomatrix_display/test_main.cpp`, add a concurrent set/get smoke test where feasible (single-thread sequential set→get→set→get is acceptable if multithreading not wired in the harness)
  - [ ] `pio test -f test_neomatrix_display --without-uploading --without-testing` compiles clean

- [ ] **Task 7: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] All new/existing tests pass

## Dev Notes

### Concurrency Constraint
Per `CLAUDE.md` Architecture: Core 0 runs the display render loop (reads `_calibrationMode` every frame in `NeoMatrixDisplay::show()` / render path) and Core 1 runs WebPortal + ConfigManager (writes `_calibrationMode` when the user toggles calibration mode via the dashboard). Xtensa does not guarantee natural `bool` atomicity across cores under arbitrary compiler reordering. `volatile` in C++ is **not** a concurrency primitive — it only prevents certain compiler optimizations, not cross-core memory reordering. `std::atomic<bool>` with `memory_order_acquire` (reader) / `memory_order_release` (writer) provides the correct happens-before edge.

### Current code
`firmware/adapters/NeoMatrixDisplay.h:59`:
```cpp
volatile bool _calibrationMode = false;
```

### Target code
```cpp
#include <atomic>
...
std::atomic<bool> _calibrationMode{false};

// Reader
bool cal = _calibrationMode.load(std::memory_order_acquire);

// Writer
_calibrationMode.store(true, std::memory_order_release);
```

### Risks
- **ISR context:** if any reader is called from IRAM_ATTR / hardware ISR, `std::atomic<bool>::load` with default `seq_cst` may pull in library calls that aren't IRAM-safe. Use `memory_order_relaxed` for ISR reads (acceptable: the flag is advisory, a stale read just means one frame delay).
- **const-correctness:** `atomic<bool>::load() const` is already const-qualified, so `isCalibrationMode() const` keeps compiling unchanged.
- **Header include cost:** `<atomic>` adds trivial compile-time; negligible runtime.

### References
- `firmware/adapters/NeoMatrixDisplay.h` — flag declaration
- `firmware/adapters/NeoMatrixDisplay.cpp` — render loop reads, setter writes
- `CLAUDE.md` — Architecture section (dual-core FreeRTOS, `std::atomic` for cross-core signals)
- `AGENTS.md` — concurrency guardrails

## File List

- `firmware/adapters/NeoMatrixDisplay.h` (MODIFIED) — add `<atomic>` include, promote `_calibrationMode` to `std::atomic<bool>`, update reader call sites
- `firmware/adapters/NeoMatrixDisplay.cpp` (MODIFIED) — update reader/writer call sites with explicit memory ordering
- `firmware/test/test_neomatrix_display/test_main.cpp` (MODIFIED) — add concurrent set/get smoke test
