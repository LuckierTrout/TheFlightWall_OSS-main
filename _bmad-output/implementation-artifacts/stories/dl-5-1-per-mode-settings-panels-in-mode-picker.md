# Story dl-5.1: Per-Mode Settings Panels in Mode Picker

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want to configure each display mode's settings through the Mode Picker on the dashboard,
So that I can tailor Clock Mode's time format, Departures Board's row count, and other mode-specific options without touching firmware.

## Acceptance Criteria

1. **Given** **`GET /api/display/modes`** (**`WebPortal::_handleGetDisplayModes`**), **When** the handler builds each **`modes[]`** entry, **Then** include **`settings`**: either **`null`** or a JSON **array** of objects **`{ key, label, type, default, min, max, enumOptions, value }`** where:
   - **Schema fields** (**`key`** … **`enumOptions`**) are copied from **`ModeEntry.settingsSchema`** (**`ModeSettingDef`** in **`interfaces/DisplayMode.h`**).
   - **`value`** is the current persisted value from **`ConfigManager::getModeSetting(schema.modeAbbrev, def.key, def.defaultValue)`** — use **`schema.modeAbbrev`** for **NVS** (**`m_{abbrev}_{key}`**), **not** the public **`id`** string when they differ (**e.g.** **`id`** **`"departures_board"`** vs abbrev **`"depbd"`** per **`DEPBD_SCHEMA`** / **`CLOCK_SCHEMA`**).
   - If **`entry.settingsSchema == nullptr`**, set **`settings`** to **`null`** or **`[]`** (consistent choice — document in code; **epic** allows empty).

2. **Given** the handler enumerates settings, **When** it runs, **Then** it **loops `ModeEntry.settingsSchema` only** (**rule 28** / **epic**) — **no** hardcoded **`"format"`** / **`"rows"`** lists in **`WebPortal.cpp`**. Adding a **`ModeSettingDef`** to **`CLOCK_SCHEMA`**, **`DEPBD_SCHEMA`**, or a future schema **automatically** appears in **GET** without portal changes.

3. **Given** the Mode Picker **`renderModeCards`** (**`dashboard.js`**), **When** a mode has a non-empty **`settings`** array, **Then** render a **per-card settings panel**: dynamic controls — **`enum`** → **`<select>`** (options from **`enumOptions`** split on **`,`**; labels trim whitespace), **`uint8`/`uint16`** → number input with **`min`/`max`**, **`bool`** → checkbox. Empty **`settings`** / **`null`**: **no** settings block — card keeps **name**, **description**, schematic, **activation** behavior (**epic**).

4. **Given** the owner changes a setting and saves, **When** the client calls **`POST /api/display/mode`** with **`{ "mode": "<modeId>", "settings": { "<key>": <number> } }`** (values numeric for JSON simplicity; **bools** as **0/1**), **Then** the firmware:
   - Resolves **`mode`** to a **`ModeEntry`** (same validation as today — unknown **`mode_id`** → **400**).
   - For each key in **`settings`**, confirms the key exists in that entry's **`settingsSchema`**; unknown keys → **400** with clear **`code`** (**e.g.** **`UNKNOWN_SETTING`**).
   - Calls **`ConfigManager::setModeSetting(abbrev, key, value)`** for each (**abbrev** from **`settingsSchema->modeAbbrev`**). On **`setModeSetting`** **false** (validation failure), return **400** without partial apply, or document atomic batch behavior.
   - Still performs **orchestrator** path for **mode switch** when the requested **`mode`** differs from current manual/active behavior **as today** — **or**, if **`mode`** matches **current active** and only **`settings`** are sent, **skip** **`ModeOrchestrator::onManualSwitch`** but **do** persist settings and signal **`g_configChanged`** so the display pipeline picks up changes within **~2s** (**NFR8** / **FR19**). **Clarify** in implementation: **`ClockMode`** / **`DeparturesBoardMode`** already re-read **`getModeSetting`** on relevant paths; if not, add **config-change** refresh (**no reboot**).

5. **Given** **`ClockMode`** (**`"clock"`**), **When** **`format`** is set to **1**, **Then** **`setModeSetting("clock", "format", 1)`** persists (**epic**); **12h** appears on next render cycle after **`g_configChanged`** (or equivalent).

6. **Given** **`DeparturesBoardMode`**, **When** **`rows`** is set to **2**, **Then** **`setModeSetting("depbd", "rows", 2)`** persists and **max rows** respects **2** on subsequent frames (**epic**).

7. **Given** **`firmware/data-src/dashboard.js`** (and **CSS** if needed), **When** shipped, **Then** use existing **`FW.get` / `FW.post`**, **`showToast`**, and Mode Picker patterns (**`switchMode`**, **`loadDisplayModes`**, scoped **`showCardError`**) — **NFR7**. After editing **`dashboard.js`**, regenerate **`firmware/data/dashboard.js.gz`** (**`gzip -9 -c ...`** from **`firmware/`**).

8. **Given** **`pio test`** / **`pio run`**, **Then** extend **`test_mode_registry`** or add **`WebPortal`** / **JSON** tests so **`GET /api/display/modes`** includes **`settings`** for **`clock`** / **`departures_board`** with expected **shape**; add **`POST`** with **`settings`** coverage where the test harness can call the handler or **`setModeSetting`** integration — **no** new warnings.

## Tasks / Subtasks

- [x] Task 1 (**`WebPortal.cpp`** — **`_handleGetDisplayModes`**) — build **`settings`** arrays from **`settingsSchema`** + **`getModeSetting`** (**AC: #1–#2**)
- [x] Task 2 (**`WebPortal.cpp`** — **`POST /api/display/mode`** body handler) — parse optional **`settings`** object; validate against schema; call **`setModeSetting`**; **`g_configChanged`** when settings applied (**AC: #4–#6**)
- [x] Task 3 (**`dashboard.js`**) — render settings UI inside **`.mode-card`**; wire save (**Apply** per card or debounced auto-save — pick one pattern and document); POST payload (**AC: #3–#4, #7**)
- [x] Task 4 (**`dashboard.css`** if needed) — spacing/accessibility for settings block (**AC: #7**)
- [x] Task 5 — **`dashboard.js.gz`** (**AC: #7**)
- [x] Task 6 — Native tests (**AC: #8**)

## Dev Notes

### Schema reference

```35:50:firmware/interfaces/DisplayMode.h
// --- Per-Mode Settings Schema (Delight Release forward-compat) ---
struct ModeSettingDef {
    const char* key;          // NVS key suffix (<=7 chars)
    const char* label;        // UI display name
    const char* type;         // "uint8", "uint16", "bool", "enum"
    int32_t defaultValue;
    int32_t minValue;
    int32_t maxValue;
    const char* enumOptions;  // comma-separated for enum type, NULL otherwise
};

struct ModeSettingsSchema {
    const char* modeAbbrev;           // <=5 chars, used for NVS key prefix
    const ModeSettingDef* settings;
    uint8_t settingCount;
};
```

### Existing NVS API

- **`ConfigManager::getModeSetting` / `setModeSetting`** — **`firmware/core/ConfigManager.cpp`** ~746–802; composed key **`m_{abbrev}_{key}`** ≤ **15** chars.
- **Validation** for **`clock`/`format`** and **`depbd`/`rows`** is already in **`setModeSetting`** — extend only if new setting types need bounds checks.

### Mode Picker baseline

- Cards: **`renderModeCards`**, **`switchMode`**, **`FW.post('/api/display/mode', { mode: modeId })`** — **`dashboard.js`** ~2860+.
- HTML shell: **`dashboard.html`** **`#mode-picker`**, **`#modeCardsList`**.

### Out of scope

- **New** mode-specific settings beyond what **`CLOCK_SCHEMA`** / **`DEPBD_SCHEMA`** already define (unless trivially required to test **rule 28**).
- **Wizard** integration.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-5.md`
- Prior DL UI: `_bmad-output/implementation-artifacts/stories/dl-4-2-schedule-management-dashboard-ui.md` (patterns: gzip, **`WebPortal`** JSON)
- **`WebPortal`**: `firmware/adapters/WebPortal.cpp`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

### Completion Notes List

- **Task 1**: Extended `_handleGetDisplayModes` to emit `settings` array per mode from `ModeEntry.settingsSchema`. Schema fields (key, label, type, default, min, max, enumOptions) copied from static `ModeSettingDef`. `value` read via `ConfigManager::getModeSetting(modeAbbrev, key, default)`. Modes without schema emit `settings: null`. No hardcoded key lists — fully data-driven from schema (rule 28).
- **Task 2**: Extended `POST /api/display/mode` body handler to accept optional `settings` object. Pre-validates all keys against `settingsSchema` (unknown key → 400 UNKNOWN_SETTING). Calls `setModeSetting` per key with `modeAbbrev`. If mode matches current active, skips `onManualSwitch` but settings persist and `g_configChanged` fires via `fireCallbacks()` added to `setModeSetting`. If mode differs, both settings and mode switch execute.
- **Task 3**: Added `buildModeSettingsPanel()` to `dashboard.js`. Renders per-card settings panel with dynamic controls: `enum` → `<select>` (options from `enumOptions` split on comma), `uint8`/`uint16` → number input with `min`/`max`, `bool` → checkbox. Apply button POSTs `{ mode, settings }` via `FW.post`. Click propagation stopped on settings panel to prevent accidental mode switch.
- **Task 4**: Added CSS classes `.mode-settings-panel`, `.mode-setting-row`, `.mode-setting-label`, `.mode-setting-input`, `.mode-setting-checkbox`, `.mode-setting-actions`, `.mode-setting-apply` to `style.css`.
- **Task 5**: Regenerated `dashboard.js.gz` and `style.css.gz` from `firmware/data-src/`.
- **Task 6**: Added 6 tests to `test_mode_registry`: schema shape validation for clock and departures_board, null schema for modes without settings, `setModeSetting` fires callbacks, settings get/set roundtrip, reject out-of-range values.

### Implementation Decisions
- **Apply button pattern** (not auto-save): Chose explicit Apply button per card for clear user intent and to avoid accidental saves during exploration. Pattern document: `buildModeSettingsPanel()` in `dashboard.js`.
- **`settings: null`** for modes without schema (not empty array) — consistent with JSON convention of "absent vs empty".
- **`fireCallbacks()` in `setModeSetting`**: Ensures `g_configChanged` fires on any mode setting change, supporting AC #4's "display pipeline picks up changes within ~2s" requirement without needing direct access to the atomic flag from WebPortal.

### Change Log

- 2026-04-14: Story dl-5.1 implemented — per-mode settings panels in Mode Picker

### File List

- `firmware/adapters/WebPortal.cpp` — modified (settings in GET + POST handlers)
- `firmware/core/ConfigManager.cpp` — modified (added `fireCallbacks()` to `setModeSetting`)
- `firmware/data-src/dashboard.js` — modified (added `buildModeSettingsPanel`, extended `renderModeCards`)
- `firmware/data-src/style.css` — modified (added mode settings panel CSS)
- `firmware/data/dashboard.js.gz` — regenerated
- `firmware/data/style.css.gz` — regenerated
- `firmware/test/test_mode_registry/test_main.cpp` — modified (added 6 dl-5.1 tests)

## Previous story intelligence

- **`ds-3.x`** delivered Mode Picker layout and **`POST /api/display/mode`** for switching — this story **extends** the same endpoint and **GET** payload; keep **orchestrator** (**Rule 24**) behavior intact.

## Git intelligence summary

Touches **`WebPortal.cpp`**, **`dashboard.js`**, **`dashboard.css`** (optional), **`firmware/data/dashboard.js.gz`**, possibly **`main.cpp`** / display task if **config** refresh for settings-on-inactive-mode needs a hook; tests under **`firmware/test/`**.

## Project context reference

`_bmad-output/project-context.md` — **REST**, **gzip** portal assets.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
