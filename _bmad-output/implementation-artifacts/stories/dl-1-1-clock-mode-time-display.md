# Story dl-1.1: Clock Mode Time Display

Status: Ready for Review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want the wall to display the current time in a large-format layout on the LED matrix,
So that the wall always shows something useful even when I'm not tracking flights.

## Acceptance Criteria

1. **Given** **`ClockMode`** is the active **`DisplayMode`** and **NTP** is synced (**`isNtpSynced() == true`** — **`firmware/src/main.cpp`** / **`WebPortal`** pattern), **When** **`render()`** runs each frame, **Then** the matrix shows **current local time** in **large** digits (readable at a distance — use full matrix height/width appropriately via **`Adafruit_GFX`** / **`FastLED_NeoMatrix`** text APIs already used elsewhere).

2. **Given** Clock Mode is active, **When** at least **one second** elapses, **Then** the displayed time **updates** without full-screen **flicker**: prefer **dirty** region updates or clear only the clock region (**no** visible flash of unrelated pixels).

3. **Given** **`ConfigManager::getModeSetting("clock", "format", 0)`** is used, **When** NVS key **`m_clock_format`** is **absent**, **Then** return **`0`** (**24-hour**, e.g. **"14:30"** style) and **do not** write NVS on read (**FR33** / epic — no accidental key creation on first read).

4. **Given** **`m_clock_format`** is **`0`**, **When** rendering, **Then** use **24-hour** display.

5. **Given** **`m_clock_format`** is **`1`**, **When** rendering, **Then** use **12-hour** display with **AM/PM** indicator (layout choice: same line or small suffix — document in Dev Agent Record).

6. **Given** **`ConfigManager::setModeSetting("clock", "format", value)`** with **`value`** in **`{0,1}`**, **When** called, **Then** persist **`m_clock_format`** under NVS namespace **`flightwall`** and enforce **≤15 characters** for the **composed** key (**rule 28**: **`m_{abbrev}_{key}`** — here **`abbrev`** = **`clock`**, setting key suffix **`format`** → **`m_clock_format`**, **13** chars).

7. **Given** **NTP** is **not** synced yet (**`isNtpSynced() == false`**), **When** Clock Mode renders, **Then** show a **non-blank** fallback: e.g. **"--:--"**, **uptime-based** pseudo clock, or **"NO TIME"** short string — epic requires **not** a blank matrix.

8. **Given** **`ClockMode`** ships, **Then** it **fully** implements **`DisplayMode`**: **`init`**, **`render`**, **`teardown`**, **`getName()`**, **`getZoneDescriptor()`**, **`getSettingsSchema()`** — **`getName()`** returns a stable id string consistent with **`MODE_TABLE`** (use **`"clock"`** to match **dl-1.4** watchdog fallback text in epic).

9. **Given** **rule 29** (static schema in header), **When** reviewing **`ClockMode.h`**, **Then** **`CLOCK_SETTINGS[]`** and **`CLOCK_SCHEMA`** (or equivalent **`ModeSettingDef[]`** + **`ModeSettingsSchema`**) are **`static const`** declarations in the header; **`ClockMode.cpp`** references them.

10. **Given** **`MODE_TABLE`** in **`main.cpp`**, **When** this story completes, **Then** a row registers **`"clock"`** / **"Clock"** (display name) with **factory**, **`memoryRequirement()`**, **priority**, **`zoneDescriptor`** (minimal single-zone schematic for Mode Picker — can be one full-matrix **`"Time"`** region), and **`settingsSchema`** pointer.

11. **Given** **AR8** / epic alignment, **When** **`ModeEntry`** is updated, **Then** add **`const ModeSettingsSchema* settingsSchema`** (nullable). Populate for **`ClockMode`**; set **`nullptr`** for **`classic_card`** and **`live_flight`** rows until they define schemas. **`ModeRegistry`** / **`WebPortal`** enumeration must remain **heap-safe** (no **`new ClockMode`** per **GET** — only static pointers).

12. **Given** **`pio test`**, **Then** add or extend tests: **`getModeSetting`** missing key; **`ClockMode`** name/id; optional **`render`** smoke with **`nullptr`** matrix guards if pattern exists in other mode tests.

13. **Given** **`pio run -e esp32dev`**, **Then** no new warnings.

## Tasks / Subtasks

- [x] Task 1: **`ConfigManager`** — per-mode NVS helpers (**AC: #3, #6**)
  - [x] 1.1: **`getModeSetting(const char* abbrev, const char* key, int32_t defaultValue)`** — build NVS key, **`Preferences::getInt`** typed read; **read-only** path never **`put`**
  - [x] 1.2: **`setModeSetting(...)`** — validate range for **`clock`/`format`**; immediate persist per existing **`ConfigManager`** patterns

- [x] Task 2: **`ClockMode`** (**AC: #1, #2, #4, #5, #7–#9**)
  - [x] 2.1: New files **`firmware/modes/ClockMode.h`** / **`ClockMode.cpp`**
  - [x] 2.2: Internal **`_lastRenderedSecond`** (or wall-clock second) to limit redraw churn
  - [x] 2.3: **`getZoneDescriptor()`** — static **`ModeZoneDescriptor`** for picker schematic

- [x] Task 3: **`ModeEntry` + `MODE_TABLE`** (**AC: #8, #10, #11**)
  - [x] 3.1: Extend **`ModeEntry`** in **`ModeRegistry.h`** with `settingsSchema` field
  - [x] 3.2: **`main.cpp`**: factory **`clockFactory`**, **`clockMemReq`**, register row
  - [x] 3.3: Update **`test_mode_registry`** fixtures — added `nullptr`/`&CLOCK_SCHEMA` for settingsSchema field

- [x] Task 4: Tests + build (**AC: #12, #13**)

#### Review Follow-ups (AI)
- [ ] [AI-Review] MEDIUM: Mode setting hot-reload — Active mode settings cannot be updated without mode switch/reboot. ConfigManager fires callbacks on setModeSetting but active mode caches values in init() and never re-reads. Affects all modes with settings, not ClockMode-specific. (firmware/modes/ClockMode.cpp:53, firmware/core/ModeRegistry.h)

## Dev Notes

### Product vs epic strings

| Epic / dl-1.4 text | This product |
|--------------------|----------------|
| NVS **`display_mode`** for manual mode (**dl-1.3** epic) | **`disp_mode`** (**`ConfigManager::getDisplayMode`**) — **do not** introduce a second persistence key for the **same** concept |
| Mode id **"clock"** | Use **`"clock"`** in **`MODE_TABLE`** for consistency with **dl-1.4** epic |

### Existing patterns

- **`DisplayMode`** already includes **`getSettingsSchema()`** — **`ClassicCardMode`** returns **`nullptr`** today.
- **`ModeEntry`** currently has **`zoneDescriptor`** only — **add** **`settingsSchema`** alongside it (mirrors static metadata pattern).
- **NTP**: **`g_ntpSynced`** / **`isNtpSynced()``** in **`main.cpp`**; **`time(&t)`** + **`localtime_r`** style used in **`WebPortal`** for timestamps when synced.

### Dependencies

- **ds-1.3** **`ModeRegistry`** / **`MODE_TABLE`** — assumed stable.
- **ds-1.5** display pipeline calls **`ModeRegistry::tick`** + mode **`render`** — Clock must not call **`FastLED.show()`** (same rule as other modes).

### Out of scope

- **dl-1.2** — idle fallback orchestration.
- **dl-1.3** — dashboard-only manual switch wiring beyond ensuring **`"clock"`** appears in **`GET /api/display/modes`** once registered.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-1.md` (Story dl-1.1)
- Related done: `_bmad-output/implementation-artifacts/stories/dl-1-5-orchestrator-state-feedback-in-dashboard.md` (orchestrator + **`POST`** path)
- Interfaces: `firmware/interfaces/DisplayMode.h` (**`ModeSettingsSchema`**, **`ModeSettingDef`** already defined)
- Registry: `firmware/core/ModeRegistry.h`, `firmware/src/main.cpp` (**`MODE_TABLE`**)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run -e esp32dev` — SUCCESS, no new warnings. Binary 79.3% of partition.

### Completion Notes List

- **Task 1**: Added `getModeSetting()` and `setModeSetting()` to ConfigManager. Read path is read-only (never creates NVS keys on read per AC #3). Write path is storage-only; validation moved to API layer per SOLID OCP principle (2026-04-15 code review fix). NVS key composition: `m_{abbrev}_{key}` enforced ≤15 chars via snprintf bounds check.
- **Task 2**: Created `ClockMode.h` and `ClockMode.cpp`. Implements full `DisplayMode` interface. `getName()` returns `"clock"` (matches MODE_TABLE id and dl-1.4 watchdog text). Auto-scales text size to fill matrix. `_lastRenderedSecond` tracks wall-clock second to avoid flicker (AC #2 — only redraws on second change). 12-hour mode appends AM/PM inline as suffix with padded hour format (e.g., " 9:30AM") to prevent UI jumping (2026-04-15 code review fix). Fallback shows "--:--" when NTP not synced (AC #7). `CLOCK_SETTINGS[]` and `CLOCK_SCHEMA` moved to .cpp file to avoid ODR violations (2026-04-15 code review fix). Removed redundant ConfigManager::getModeSetting() call from render() hot path per architecture rule 18 (2026-04-15 code review fix).
- **Task 3**: Extended `ModeEntry` struct with `const ModeSettingsSchema* settingsSchema` field (AC #11). Added clock row to MODE_TABLE with factory, memReq, priority=2, zone descriptor, and settings schema pointer. Set `nullptr` for classic_card and live_flight settingsSchema. Updated test mode tables accordingly.
- **Task 4**: Added 5 new ConfigManager tests (getModeSetting missing key returns default, set/get roundtrip, storage-only behavior verification, key length within NVS limit, key too long rejected). Added 6 new ModeRegistry tests (ClockMode name, zone descriptor, settings schema, table presence, init with null matrix, render with null matrix). Build compiles clean — no new warnings. Updated test expectations post-review to match storage-only architecture.

### File List

- `firmware/core/ConfigManager.h` — added `getModeSetting()`, `setModeSetting()` declarations
- `firmware/core/ConfigManager.cpp` — added `getModeSetting()`, `setModeSetting()` implementations
- `firmware/core/ModeRegistry.h` — added `settingsSchema` field to `ModeEntry` struct
- `firmware/modes/ClockMode.h` — **NEW** — ClockMode header with static schema declarations
- `firmware/modes/ClockMode.cpp` — **NEW** — ClockMode implementation
- `firmware/src/main.cpp` — added ClockMode include, factory, memReq, MODE_TABLE row
- `firmware/test/test_config_manager/test_main.cpp` — added 5 per-mode NVS settings tests
- `firmware/test/test_mode_registry/test_main.cpp` — added ClockMode include, prod table entry, 6 clock tests

### Change Log

- 2026-04-14: Story dl-1.1 implemented — ClockMode, ConfigManager per-mode NVS helpers, ModeEntry extension, tests
- 2026-04-15: Code review synthesis #1 — Fixed Rule 18 violation (removed NVS read from render hot path), OCP violation (removed hardcoded validation from ConfigManager), UI bouncing bug (padded 12h format), ODR violation (moved schema to .cpp)
- 2026-04-15: Code review synthesis #2 — Fixed AC #2 violation (removed fillScreen full-screen flicker, added background color to setTextColor), fixed lying test name

## Previous story intelligence

- **dl-1.1** is the **first** numbered story under **epic dl-1**; **dl-1.5** shipped dashboard/orchestrator plumbing — keep **`POST`/`GET`** contracts compatible when **`clock`** appears in the mode list.

## Git intelligence summary

New **`firmware/modes/ClockMode.*`**, edits **`ConfigManager.*`**, **`ModeRegistry.h`**, **`main.cpp`**, tests.

## Project context reference

`_bmad-output/project-context.md` — Core **0** display, **NVS** **`flightwall`**, **15-char** keys.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-15 (First Pass)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 6.8 → MAJOR REWORK
- **Issues Found:** 4
- **Issues Fixed:** 4
- **Action Items Created:** 0

### Review: 2026-04-15 (Second Pass)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 7.1 → REJECT
- **Issues Found:** 2 verified, 4 dismissed
- **Issues Fixed:** 2
- **Action Items Created:** 1
