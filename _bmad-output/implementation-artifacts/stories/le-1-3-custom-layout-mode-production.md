
# Story LE-1.3: CustomLayoutMode Production

branch: le-1-3-custom-layout-mode-production
zone:
  - firmware/modes/CustomLayoutMode.*
  - firmware/src/main.cpp
  - firmware/core/ConfigManager.*
  - firmware/test/test_custom_layout_mode/**

Status: draft

## Story

As a **device owner**,
I want **CustomLayoutMode to be a real mode in the mode table**,
So that **I can activate my user-authored layouts alongside the built-in modes**.

## Acceptance Criteria

1. **Given** an active layout id pointing to a valid JSON file **When** `CustomLayoutMode::init(ctx)` is called **Then** the layout is loaded via `LayoutStore::load(activeId())`, parsed via ArduinoJson v7, populated into a fixed `WidgetInstance[]` array (cap 24) **And** the `JsonDocument` is freed before init returns.
2. **Given** an initialized CustomLayoutMode **When** `render(ctx, flights)` runs every frame **Then** it iterates the widget array, dispatches each via `WidgetRegistry::dispatch`, and performs **zero heap allocations per frame**.
3. **Given** a malformed or missing layout **When** `init()` fails to parse **Then** the mode renders a visible error pattern (red 1 px border + "INVALID LAYOUT" text) **And** does not crash or block the display task.
4. **Given** the mode table **When** firmware boots **Then** `CustomLayoutMode` is registered unconditionally as the 5th `ModeEntry` **And** no `#ifdef LE_V0_SPIKE` guard remains in `main.cpp`.
5. **Given** a mode switch to or from `CustomLayoutMode` **When** the orchestrator evaluates heap **Then** the 30 KB free-heap margin invariant is respected; the switch is rejected with a logged WARNING if free heap would drop below 30 KB.
6. **Given** `ConfigManager` **When** queried for the active layout id **Then** it exposes a getter that reads NVS key `layout_active` (or delegates to `LayoutStore::activeId()`), returning a 12-char string plus null terminator.
7. **Given** unit tests **When** `pio test -f test_custom_layout_mode` runs **Then** cases pass for: init with valid 8-widget layout, init with missing layout (fallback to default), render with max-density 24-widget layout stays within 30 KB heap margin.

## Tasks / Subtasks

- [ ] **Task 1: Remove LE_V0_SPIKE guard and register mode** (AC: #4)
  - [ ] `firmware/src/main.cpp` — delete `#ifdef LE_V0_SPIKE` block; add `CustomLayoutMode` as the 5th `ModeEntry` unconditionally
  - [ ] Provide factory `createCustomLayoutMode()` + memory requirement fn returning conservative 16 KB
  - [ ] Add `ModeZoneDescriptor` entry for canvas 192×48

- [ ] **Task 2: Promote CustomLayoutMode to LayoutStore-backed init** (AC: #1, #2)
  - [ ] `firmware/modes/CustomLayoutMode.cpp` — replace V0 `v0_spike_layout.h` include with `LayoutStore::load(LayoutStore::activeId(), json)`
  - [ ] Parse JSON into a local `JsonDocument` (size ≤ 8 KB), populate fixed `WidgetInstance instances_[24]` + `uint8_t count_`
  - [ ] Free `JsonDocument` before `init()` returns
  - [ ] `render()` iterates `instances_[0..count_]`, calls `WidgetRegistry::dispatch`; no allocation

- [ ] **Task 3: Error-pattern fallback** (AC: #3)
  - [ ] On `LayoutStore::load` failure or parse failure, set `state_ = INVALID` and store a static error message
  - [ ] `render()` draws red border + "INVALID LAYOUT" text via `DisplayUtils::drawTextLine`
  - [ ] Log `LOG_E("CustomLayoutMode", "Invalid layout '%s' — rendering error pattern", id)`

- [ ] **Task 4: Heap margin invariant** (AC: #5)
  - [ ] Ensure `memoryRequirement()` includes `WidgetInstance[24]` + small local scratch
  - [ ] `ModeRegistry` / `ModeOrchestrator` already enforce the 30 KB margin; verify CustomLayoutMode does not regress it
  - [ ] Add a runtime log at switch-in: `LOG_I("CustomLayoutMode", "Free heap after init: %u", ESP.getFreeHeap())`

- [ ] **Task 5: ConfigManager active layout id accessor** (AC: #6)
  - [ ] Add `String getActiveLayoutId()` / `bool setActiveLayoutId(const char*)` — delegate to `LayoutStore` or wrap NVS `layout_active`
  - [ ] Ensure key name length ≤ 15 chars (`layout_active` = 16 — use `active_layout` or `layout_active` = 13)

- [ ] **Task 6: Unit tests** (AC: #7)
  - [ ] `firmware/test/test_custom_layout_mode/test_main.cpp` — init with valid 8-widget fixture
  - [ ] test: init with missing file triggers default-layout fallback
  - [ ] test: 24-widget max-density layout init + one render pass, assert `ESP.getFreeHeap() >= 30*1024`
  - [ ] test: malformed JSON triggers error pattern (render does not crash)

- [ ] **Task 7: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] Compile-check tests: `pio test -f test_custom_layout_mode --without-uploading --without-testing`
  - [ ] Grep confirms no `LE_V0_SPIKE` / `v0_spike_layout` references remain

## Dev Notes

**Depends on:** LE-1.1 (LayoutStore), LE-1.2 (WidgetRegistry).

**Validated patterns from V0 spike** (`_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`):

- Parse JSON once at `init()`, free the `JsonDocument`, store into fixed `WidgetInstance[]` array.
- Fixed widget array, cap 24. No dynamic allocation on render hot path.
- Static dispatch via WidgetRegistry switch/table (see LE-1.2).

**NVS key naming.** The target key must fit in 15 chars. Use `layout_active` (13) or `cur_layout_id` (13) — update LE-1.1 accordingly to match. Coordinate with LE-1.1 to keep the key name consistent.

**Mode table reference.** Existing mode table in `main.cpp` registers `classic_card`, `live_flight_card`, `clock`, `departures_board`. Add `custom_layout` as 5th with factory + memory requirement + zone descriptor.

**Existing scaffold:** `firmware/modes/CustomLayoutMode.{h,cpp}` already exists from the V0 spike — this story promotes it. Remove `#include "v0_spike_layout.h"` and replace with LayoutStore-backed loading.

## File List

- `firmware/modes/CustomLayoutMode.h` (modified)
- `firmware/modes/CustomLayoutMode.cpp` (modified)
- `firmware/src/main.cpp` (modified — register mode, remove LE_V0_SPIKE)
- `firmware/core/ConfigManager.h` (modified — add active layout accessors)
- `firmware/core/ConfigManager.cpp` (modified)
- `firmware/test/test_custom_layout_mode/test_main.cpp` (new)
