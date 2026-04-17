
# Story LE-1.9: Golden-Frame Tests and Budget Gate

branch: le-1-9-golden-frame-tests-and-budget-gate
zone:
  - firmware/test/test_golden_frames/**
  - firmware/check_size.py
  - _bmad-output/planning-artifacts/le-1-verification-report.md

Status: draft

## Story

As a **release gatekeeper**,
I want **golden-frame regression tests and a firm binary/heap budget check on every LE-1 build**,
So that **the editor cannot regress render correctness or blow past the partition**.

## Acceptance Criteria

1. **Given** the test suite **When** `pio test -f test_golden_frames` runs **Then** at least 3 canonical layout fixtures render through `CustomLayoutMode::render()` and their `CRGB[]` output matches the recorded golden buffer byte-for-byte.
2. **Given** `check_size.py` **When** the binary exceeds 92% of the 1.5 MB OTA partition **Then** the build fails with a clear error.
3. **Given** a PR branch **When** `check_size.py` runs **Then** it also fails if the binary delta versus `main` HEAD exceeds 180 KB.
4. **Given** CustomLayoutMode active with a max-density 24-widget layout **When** free heap is sampled **Then** the reading is ≥ 30 KB.
5. **Given** the spike's `[env:esp32dev_le_spike]` instrumentation **When** re-measured against ClassicCardMode on a realistic-density scene **Then** CustomLayoutMode render p95 ratio is ≤ 1.2× baseline.
6. **Given** `_bmad-output/planning-artifacts/le-1-verification-report.md` **When** opened **Then** it documents the 3 golden fixtures, binary size measurement, heap measurement, and p95 render ratio with timestamps and commit SHAs.
7. **Given** every LE-1 story has merged **When** a final compile check runs **Then** `pio run -e esp32dev` succeeds **And** every previous LE-1 story's test suite compiles.

## Tasks / Subtasks

- [ ] **Task 1: Author golden-frame fixtures** (AC: #1)
  - [ ] `firmware/test/test_golden_frames/fixtures/` — 3 layouts: (a) text + clock, (b) flight-field-heavy 8-widget, (c) max-density 24-widget
  - [ ] For each fixture: layout JSON + expected `CRGB[192*48]` buffer serialized as a `.bin` or C-array header

- [ ] **Task 2: Golden-frame test harness** (AC: #1)
  - [ ] `firmware/test/test_golden_frames/test_main.cpp` — load fixture JSON, init CustomLayoutMode, call `render()`, compare framebuffer bytes vs. golden
  - [ ] On mismatch, print first divergent pixel coordinate + expected/actual RGB565

- [ ] **Task 3: Extend check_size.py** (AC: #2, #3)
  - [ ] `firmware/check_size.py` — add 92%-of-partition hard cap
  - [ ] Add main-branch delta check: resolve `git merge-base HEAD main`, compare prior .bin size via stored artifact or fresh build; fail if delta > 180 KB
  - [ ] Provide an env flag to skip delta check locally when `main` baseline is unavailable (default: enforce in CI-like envs)

- [ ] **Task 4: Heap regression check** (AC: #4)
  - [ ] Add to `test_golden_frames` or a new `test_le_budget/` suite: boot CustomLayoutMode with fixture (c), read `ESP.getFreeHeap()`, assert ≥ 30 KB
  - [ ] Compile-only acceptable if no hardware; document as device test otherwise

- [ ] **Task 5: p95 render ratio measurement** (AC: #5)
  - [ ] Revive spike instrumentation under `[env:esp32dev_le_spike]` (non-destructive — ifdef-gated)
  - [ ] Capture 500 frames each for ClassicCardMode and CustomLayoutMode on fixture (b)
  - [ ] Record p95 timings and compute ratio; document in verification report

- [ ] **Task 6: Verification report** (AC: #6)
  - [ ] `_bmad-output/planning-artifacts/le-1-verification-report.md` — sections: golden fixtures, binary size + partition %, heap at max density, p95 ratio, commit SHAs, device/build timestamps

- [ ] **Task 7: Cross-story compile sweep** (AC: #7)
  - [ ] Run `pio test -f test_layout_store --without-uploading --without-testing`
  - [ ] Run the same for test_widget_registry, test_custom_layout_mode, test_web_portal, test_logo_widget, test_flight_field_widget, test_metric_widget, test_golden_frames
  - [ ] Record pass/fail in the verification report

- [ ] **Task 8: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition AND under the new 92% cap
  - [ ] All golden-frame tests pass
  - [ ] Verification report committed

## Dev Notes

**Validated spike instrumentation** (`_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`) — the `[env:esp32dev_le_baseline]` / `[env:esp32dev_le_spike]` envs are retained (per LE-1.1 Task 1) as dev-only measurement tools. This story re-uses them for the p95 re-measurement gate.

**Depends on:** All prior LE-1 stories (1.1–1.8). This is the final gate.

**Golden-frame design.** Store expected buffers as raw `uint16_t[192*48]` RGB565 arrays (18 KB each) in compressed form or as a base64/hex-encoded header under `fixtures/`. Regenerate only when widget rendering intentionally changes; the test compares byte-for-byte.

**Binary-size CI gate.** Per project CLAUDE.md the binary is currently ~81% used. 92% cap = ~1.38 MB. The 180 KB branch-delta gate is designed to catch sudden regressions without blocking legitimate feature growth across the whole epic.

**p95 render baseline.** ClassicCardMode on a realistic (not worst-case) scene is the baseline per the spike report. CustomLayoutMode ratio ≤ 1.2× confirms the widget registry + bitmap-blit + caching patterns hold.

## File List

- `firmware/test/test_golden_frames/test_main.cpp` (new)
- `firmware/test/test_golden_frames/fixtures/layout_a.json` (new)
- `firmware/test/test_golden_frames/fixtures/layout_b.json` (new)
- `firmware/test/test_golden_frames/fixtures/layout_c.json` (new)
- `firmware/test/test_golden_frames/fixtures/golden_a.bin` (new)
- `firmware/test/test_golden_frames/fixtures/golden_b.bin` (new)
- `firmware/test/test_golden_frames/fixtures/golden_c.bin` (new)
- `firmware/check_size.py` (modified — add 92% cap + delta gate)
- `_bmad-output/planning-artifacts/le-1-verification-report.md` (new)
