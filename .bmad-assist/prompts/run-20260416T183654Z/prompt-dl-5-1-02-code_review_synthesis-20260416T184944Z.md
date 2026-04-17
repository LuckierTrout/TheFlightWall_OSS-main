<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: dl-5 -->
<!-- Story: 1 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260416T184944Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story dl-5.1

You are synthesizing 2 independent code review findings.

Your mission:
1. VERIFY each issue raised by reviewers
   - Cross-reference with project_context.md (ground truth)
   - Cross-reference with git diff and source files
   - Identify false positives (issues that aren't real problems)
   - Confirm valid issues with evidence

2. PRIORITIZE real issues by severity
   - Critical: Security vulnerabilities, data corruption risks
   - High: Bugs, logic errors, missing error handling
   - Medium: Code quality issues, performance concerns
   - Low: Style issues, minor improvements

3. SYNTHESIZE findings
   - Merge duplicate issues from different reviewers
   - Note reviewer consensus (if 3+ agree, high confidence)
   - Highlight unique insights from individual reviewers

4. APPLY source code fixes
   - You have WRITE PERMISSION to modify SOURCE CODE files
   - CRITICAL: Before using Edit tool, ALWAYS Read the target file first
   - Use EXACT content from Read tool output as old_string, NOT content from this prompt
   - If Read output is truncated, use offset/limit parameters to locate the target section
   - Apply fixes for verified issues
   - Do NOT modify the story file (only Dev Agent Record if needed)
   - Document what you changed and why

Output format:
## Synthesis Summary
## Issues Verified (by severity)
## Issues Dismissed (false positives with reasoning)
## Source Code Fixes Applied

]]></mission>
<context>
<file id="ed7fe483" path="_bmad-output/project-context.md" label="PROJECT CONTEXT"><![CDATA[

---
project_name: TheFlightWall_OSS-main
date: '2026-04-12'
---

# Project Context for AI Agents

Lean rules for implementing FlightWall (ESP32 LED flight display + captive-portal web UI). Prefer existing patterns in `firmware/` over new abstractions.

## Technology Stack

- **Firmware:** C++11, ESP32 (Arduino/PlatformIO), FastLED + Adafruit GFX + FastLED NeoMatrix, ArduinoJson ^7.4.2.
- **Web on device:** ESPAsyncWebServer (**mathieucarbou fork**), AsyncTCP (**Carbou fork**), LittleFS (`board_build.filesystem = littlefs`), custom `custom_partitions.csv` (~2MB app + ~2MB LittleFS).
- **Dashboard assets:** Editable sources under `firmware/data-src/`; served bundles are **gzip** under `firmware/data/`. After editing a source file, regenerate the matching `.gz` from `firmware/` (e.g. `gzip -9 -c data-src/common.js > data/common.js.gz`).

## Critical Implementation Rules

- **Core pinning:** Display/task driving LEDs on **Core 0**; WiFi, HTTP server, and flight fetch pipeline on **Core 1** (FastLED + WiFi ISR constraints).
- **Config:** `ConfigManager` + NVS; debounce writes; atomic saves; use category getters; `POST /api/settings` JSON envelope `{ ok, data, error, code }` pattern for REST responses.
- **Heap / concurrency:** Cap concurrent web clients (~2–3); stream LittleFS reads; use ArduinoJson filter/streaming for large JSON; avoid full-file RAM buffering for uploads.
- **WiFi:** WiFiManager-style state machine (AP setup → STA → reconnect / AP fallback); mDNS `flightwall.local` in STA.
- **Structure:** Extend hexagonal layout — `firmware/core/`, `firmware/adapters/` (e.g. `WebPortal.cpp`), `firmware/interfaces/`, `firmware/models/`, `firmware/config/`, `firmware/utils/`.
- **Tooling:** Build from `firmware/` with `pio run`. On macOS serial: use `/dev/cu.*` (not `tty.*`); release serial monitor before upload.
- **Scope for code reviews:** Product code under `firmware/` and tests under `firmware/test/` and repo `tests/`; do not treat BMAD-only paths as product defects unless the task says so.

## Planning Artifacts

- Requirements and design: `_bmad-output/planning-artifacts/` (`architecture.md`, `epics.md`, PRDs).
- Stories and sprint line items: `_bmad-output/implementation-artifacts/` (e.g. `sprint-status.yaml`, per-story markdown).

## BMAD / bmad-assist

- **`bmad-assist.yaml`** at repo root configures providers and phases; `paths.project_knowledge` points at `_bmad-output/planning-artifacts/`, `paths.output_folder` at `_bmad-output/`.
- **This file** (`project-context.md`) is resolved at `_bmad-output/project-context.md` or `docs/project-context.md` (see `bmad-assist` compiler `find_project_context_file`).
- Keep **`sprint-status.yaml`** story keys aligned with `.bmad-assist/state.yaml` (`current_story`, `current_epic`) when using `bmad-assist run` so phases do not skip with “story not found”.


]]></file>
<file id="5b122526" path="_bmad-output/implementation-artifacts/stories/dl-5-1-per-mode-settings-panels-in-mode-picker.md" label="DOCUMENTATION"><![CDATA[

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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** /Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/dl-5-1-per-mode-settings-panels-in-mode-picker.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **3** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | Task completion lie: AC #4 claims firmware returns 400 without partial apply, but logic applies settings sequentially. | `WebPortal.cpp` | +3 |
| 🟠 IMPORTANT | SOLID violation: Open/Closed Principle violated by hardcoding mode settings validation logic inside `ConfigManager`. | `ConfigManager.cpp` | +1 |
| 🟡 MINOR | Incomplete implementation of AC #4: Clock mode doesn't refresh settings on `g_configChanged` for active mode. | `ClockMode.cpp` | +0.3 |

### Evidence Score: 4.3

| Score | Verdict |
|-------|---------|
| **4.3** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[6/10] Open/Closed Principle (SOLID):** `ConfigManager::setModeSetting()` hardcodes range validation for specific modes (`depbd/rows`). Adding a new mode with settings requires modifying this central configuration file, violating OCP. The dynamic `settingsSchema` approach in the API layer is undermined by this manual configuration coupling.
  - 📍 `firmware/core/ConfigManager.cpp:793`
  - 💡 Fix: Remove hardcoded mode validation from `ConfigManager` and rely exclusively on the `ModeSettingsSchema` validation in `WebPortal::_handlePostDisplayMode` or a mode-specific `validateSetting(key, value)` callback in the `DisplayMode` interface.

---

## 🐍 Pythonic Crimes & Readability

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

✅ No significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** `WebPortal::_handlePostDisplayMode` claims "Apply all settings (all values pre-validated — no partial apply risk)", but sequential calls to `ConfigManager::setModeSetting()` do not provide atomicity. If a payload contains three valid keys followed by one invalid key, the first three are already committed to NVS before the loop aborts and returns 400.
  - 📍 `firmware/adapters/WebPortal.cpp:1206`
  - 🔄 Reproduction: POST JSON with `{"key1": 1, "invalid_key": 999}`. `key1` persists in NVS.

---

## 🔧 Maintainability Issues

✅ Code is maintainable and well-documented.

---

## 🛠️ Suggested Fixes

### 1. Atomic Settings Application
**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** Sequential NVS writes lead to partial application on validation failure.

**Diff:**
```diff
--- a/firmware/adapters/WebPortal.cpp
+++ b/firmware/adapters/WebPortal.cpp
@@ -1202,13 +1202,13 @@
-            // Apply all settings (all values pre-validated — no partial apply risk)
+            // Atomic apply: stage all changes, then persist in a batch
+            // (Requires ConfigManager batch API enhancement)
             for (JsonPair kv : settingsObj) {
-                int32_t val = kv.value().as<int32_t>();
-                if (!ConfigManager::setModeSetting(schema->modeAbbrev, kv.key().c_str(), val)) {
-                    _sendJsonError(request, 400, "Setting validation failed", "INVALID_SETTING_VALUE");
-                    return;
-                }
+                ConfigManager::stageModeSetting(schema->modeAbbrev, kv.key().c_str(), kv.value().as<int32_t>());
             }
+            ConfigManager::commitStagedSettings();
             if (settingsObj.size() > 0) settingsApplied = true;
```

---

**Review Actions:**
- Issues Found: 3
- Issues Fixed: 0
- Action Items Created: 3

⚠️ Address the identified issues before proceeding.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** _bmad-output/implementation-artifacts/stories/dl-5-1-per-mode-settings-panels-in-mode-picker.md

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 2 |
| Task Completion Lies | 0 |
| SOLID Violations | 1 |
| Hidden Bugs | 2 |
| Performance Footguns | 2 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **7** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | `clock/format` range validation missing from `setModeSetting` — test `test_settings_reject_unknown_mode_key` will **fail on device** | `ConfigManager.cpp:793` | +3 |
| 🟠 IMPORTANT | AC #4 violation: modes don't re-read settings on config change — settings only take effect after mode switch (teardown/init cycle) | `ClockMode.cpp:53`, `DeparturesBoardMode.cpp:45` | +1 |
| 🟠 IMPORTANT | POST handler always calls `onManualSwitch` even for settings-only changes to the active mode, contradicting AC #4's "skip onManualSwitch" clause | `WebPortal.cpp:1217` | +1 |
| 🟠 IMPORTANT | Triple iteration of `settingsObj` in POST handler — O(n×m) per pass where n=keys, m=schema entries — unnecessary on constrained device | `WebPortal.cpp:1169-1210` | +1 |
| 🟡 MINOR | `loadDisplayModes()` call in Apply button handler rebuilds entire Mode Picker DOM, destroying the settings panel the user just interacted with (UX jank) | `dashboard.js:3010` | +0.3 |
| 🟡 MINOR | `fireCallbacks()` called N times for N settings in a single POST — one per `setModeSetting()` call — triggers N config-change notifications instead of 1 | `ConfigManager.cpp:809` | +0.3 |
| 🟡 MINOR | `is<int32_t>()` will reject `1.0` sent by some JSON clients — ArduinoJson 7.x distinguishes integer from float types | `WebPortal.cpp:1188` | +0.3 |
| 🟢 CLEAN PASS | 5 categories clean: Security, Abstraction, Type Safety, Style, Lying Tests | — | -2.5 |

### Evidence Score: 4.4

| Score | Verdict |
|-------|---------|
| **4.4** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[6/10] Open/Closed Principle — Hardcoded validation in ConfigManager violates rule 28 intent:** `setModeSetting` hardcodes validation only for `depbd/rows` (line 793) but not `clock/format`. The comment on line 787 says "caller should validate" but the WebPortal pre-validates using schema, making the ConfigManager validation redundant for callers that use the schema AND incomplete for callers that don't. The test at line 1123 expects `setModeSetting("clock", "format", 2)` to return `false` — this test will **fail on device** because ConfigManager has no clock/format validation.
  - 📍 `firmware/core/ConfigManager.cpp:793`
  - 💡 Fix: Either add generic schema-based validation in `setModeSetting` (loop through ModeEntry table), or remove the inline `depbd/rows` exception and rely entirely on WebPortal's pre-validation. The test expectation must match the chosen approach.

- **[4/10] Dependency Inversion — Modes read ConfigManager in `init()` but can't pick up hot-reload changes without a full teardown/init cycle:** AC #4 says "display pipeline picks up changes within ~2s" and mentions `g_configChanged` or equivalent. The `fireCallbacks()` in `setModeSetting` fires `g_configChanged`, but modes only read `getModeSetting` in `init()` (ClockMode:53, DeparturesBoardMode:45). The display task would need to teardown+init the active mode on config change for settings to take effect — this path doesn't exist.
  - 📍 `firmware/modes/ClockMode.cpp:53`, `firmware/modes/DeparturesBoardMode.cpp:45`

---

## ⚡ Performance & Scalability

- **[MEDIUM] Triple-pass iteration of settingsObj:** The POST handler iterates `settingsObj` three times — once for key existence check (line 1169), once for range validation (line 1187), once for apply (line 1204). Each inner loop is O(m) against schema entries. On an ESP32 with 2-3 concurrent web clients, this triples CPU work per request. Could be collapsed into a single pass that validates and applies.
  - 📍 `firmware/adapters/WebPortal.cpp:1169-1210`
  - 💡 Fix: Single-pass: validate key existence, type, range, and collect into a temp array, then write all in one batch.

- **[LOW] N `fireCallbacks()` invocations for N settings applied in one POST:** If a future mode has 5 settings and the user changes all 5, `setModeSetting` fires callbacks 5 times. Each callback fires `g_configChanged` on the atomic flag, which triggers `buildRenderContext()` rebuild. On ESP32, this is 5 unnecessary NVS opens (each `setModeSetting` opens/closes Preferences independently).
  - 📍 `firmware/core/ConfigManager.cpp:809`
  - 💡 Fix: Add a `setModeSettingBatch()` that opens Preferences once and fires callbacks once, or have the WebPortal call `fireCallbacks()` once after the loop.

---

## 🐛 Correctness & Safety

- **🐛 Bug: Test `test_settings_reject_unknown_mode_key` will fail on device.** The test at line 1123 asserts `ConfigManager::setModeSetting("clock", "format", 2)` returns `false`. But `ConfigManager::setModeSetting` only has an explicit range check for `depbd/rows` (lines 793-798). For `clock/format`, the value `2` will pass through — NVS write succeeds, returns `true`. The test assertion is wrong given the current implementation.
  - 📍 `firmware/test/test_mode_registry/test_main.cpp:1123`
  - 🔄 Reproduction: Flash and run `pio test -e esp32dev -f test_mode_registry`. Test `test_settings_reject_unknown_mode_key` will fail with `Expected FALSE Was TRUE`.

- **🐛 Bug: `is<int32_t>()` rejects float-typed JSON values.** ArduinoJson 7.x `is<int32_t>()` returns `false` for `1.0` (a float with integer value). If any JSON client serializes `{"format": 1.0}` instead of `{"format": 1}`, the firmware rejects it with `INVALID_SETTING_TYPE`. The JS client uses `parseInt()` so it sends integers, but this is fragile against other clients (e.g., curl, Postman with default number serialization).
  - 📍 `firmware/adapters/WebPortal.cpp:1188`
  - 🔄 Reproduction: `curl -X POST http://flightwall.local/api/display/mode -H 'Content-Type: application/json' -d '{"mode":"clock","settings":{"format":1.0}}'` → 400 INVALID_SETTING_TYPE.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: `loadDisplayModes()` in Apply button handler destroys and rebuilds the entire Mode Picker DOM.** After the user clicks "Apply" on a settings panel, `loadDisplayModes()` is called (line 3010), which calls `renderModeCards()` which sets `modeCardsList.innerHTML = ''` (line 3029). This destroys the settings panel the user just interacted with and rebuilds it from scratch. While functionally correct (values refresh from API), it's visually jarring — the panel flickers and scroll position may reset.
  - 📍 `firmware/data-src/dashboard.js:3010`
  - 💥 Explosion radius: UX — any future mode with complex settings (sliders, color pickers) will suffer from DOM destruction after every Apply.

- **💣 Tech Debt: AC #4's "skip onManualSwitch" clause is not implemented.** The story AC #4 explicitly says: *"if mode matches current active and only settings are sent, skip ModeOrchestrator::onManualSwitch"*. The implementation always calls `onManualSwitch` (line 1217) regardless. The comment at line 1215 cites Rule 24 as justification, but Rule 24 is about manual mode *switches*, not settings-only changes to the already-active mode. This means every "Apply" on the active mode's settings triggers an unnecessary orchestrator state transition.
  - 📍 `firmware/adapters/WebPortal.cpp:1215-1217`
  - 💥 Explosion radius: If the mode scheduler is active, clicking Apply on settings could bump the orchestrator to MANUAL, disrupting the schedule.

---

## 🛠️ Suggested Fixes

### 1. Add `clock/format` validation to ConfigManager::setModeSetting

**File:** `firmware/core/ConfigManager.cpp`
**Issue:** Missing range validation for `clock/format` causes test failure on device.

**Diff:**
```diff
@@ -793,6 +793,11 @@
     if (strcmp(abbrev, "depbd") == 0 && strcmp(key, "rows") == 0) {
         if (value < 1 || value > 4) {
             LOG_E("ConfigManager", "depbd/rows out of range, must be 1-4");
+            return false;
+        }
+    }
+    if (strcmp(abbrev, "clock") == 0 && strcmp(key, "format") == 0) {
+        if (value < 0 || value > 1) {
+            LOG_E("ConfigManager", "clock/format out of range, must be 0-1");
             return false;
         }
     }
```

### 2. Skip onManualSwitch when mode matches active and only settings changed

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** Always triggers orchestrator transition even for settings-only changes to active mode.

**Diff:**
```diff
@@ -1214,8 +1214,13 @@
     }

-    // AC #6 / Rule 24: always call onManualSwitch so the orchestrator transitions to
-    // MANUAL state regardless of whether the requested mode is already active.
-    ModeOrchestrator::onManualSwitch(modeId, matchedEntry->displayName);
+    // Story dl-5.1 AC #4: if mode matches current active and only settings changed,
+    // skip onManualSwitch to avoid disrupting orchestrator state (schedule/idle-fallback).
+    const char* currentActive = ModeRegistry::getActiveModeId();
+    bool isSameMode = currentActive && strcmp(currentActive, modeId) == 0;
+    if (!isSameMode || !settingsApplied) {
+        // AC #6 / Rule 24: route mode switches through orchestrator
+        ModeOrchestrator::onManualSwitch(modeId, matchedEntry->displayName);
+    }
```

### 3. Collapse triple-pass settings validation into single pass

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** Three separate iterations of settingsObj each with inner O(m) loop.

**Diff:**
```diff
@@ -1167,36 +1167,23 @@
         } else {
             const ModeSettingsSchema* schema = matchedEntry->settingsSchema;
-            // Pre-validate: check all keys exist in schema
+            // Single pass: validate key existence, type, range
             for (JsonPair kv : settingsObj) {
                 bool found = false;
                 for (uint8_t s = 0; s < schema->settingCount; s++) {
                     if (strcmp(kv.key().c_str(), schema->settings[s].key) == 0) {
                         found = true;
+                        if (!kv.value().is<int32_t>()) {
+                            _sendJsonError(request, 400, "Setting value must be numeric", "INVALID_SETTING_TYPE");
+                            return;
+                        }
+                        int32_t val = kv.value().as<int32_t>();
+                        if (val < schema->settings[s].minValue || val > schema->settings[s].maxValue) {
+                            _sendJsonError(request, 400, "Setting value out of range", "INVALID_SETTING_VALUE");
+                            return;
+                        }
                         break;
                     }
                 }
                 if (!found) {
                     _sendJsonError(request, 400, "Unknown setting key", "UNKNOWN_SETTING");
                     return;
                 }
             }
-            // Pre-validate values (second pass)
-            for (JsonPair kv : settingsObj) { ... }
             // Apply all settings (all values pre-validated)
             for (JsonPair kv : settingsObj) {
```

---

**Review Actions:**
- Issues Found: 7
- Issues Fixed: 0
- Action Items Created: 0

⚠️ Address the identified issues before proceeding. The CRITICAL test failure (clock/format validation) will cause `pio test` to fail on device, directly violating AC #8. The AC #4 "skip onManualSwitch" gap risks disrupting the mode scheduler on every settings Apply.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-16</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">dl-5</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="712">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">2</var>
<var name="session_id">2204a991-6946-4620-a34e-b0ed04df9202</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="5b122526">embedded in prompt, file id: 5b122526</var>
<var name="story_id">dl-5.1</var>
<var name="story_key">dl-5-1-per-mode-settings-panels-in-mode-picker</var>
<var name="story_num">1</var>
<var name="story_title">1-per-mode-settings-panels-in-mode-picker</var>
<var name="template">False</var>
<var name="timestamp">20260416_1449</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="validator_count"></var>
</variables>
<instructions><workflow>
  <critical>Communicate all responses in English and generate all documents in English</critical>
  <critical>You are the MASTER SYNTHESIS agent for CODE REVIEW findings.</critical>
  <critical>You have WRITE PERMISSION to modify SOURCE CODE files and story Dev Agent Record section.</critical>
  <critical>DO NOT modify story context (AC, Dev Notes content) - only Dev Agent Record (task checkboxes, completion notes, file list).</critical>
  <critical>All context (project_context.md, story file, anonymized reviews) is EMBEDDED below - do NOT attempt to read files.</critical>

  <step n="1" goal="Analyze reviewer findings">
    <action>Read all anonymized reviewer outputs (Reviewer A, B, C, D, etc.)</action>
    <action>For each issue raised:
      - Cross-reference with embedded project_context.md and story file
      - Cross-reference with source code snippets provided in reviews
      - Determine if issue is valid or false positive
      - Note reviewer consensus (if 3+ reviewers agree, high confidence issue)
    </action>
    <action>Issues with low reviewer agreement (1-2 reviewers) require extra scrutiny</action>
    <action>Group related findings that address the same underlying problem</action>
  </step>

  <step n="1.5" goal="Review Deep Verify code analysis" conditional="[Deep Verify Findings] section present">
    <critical>Deep Verify analyzed the actual source code files for this story.
      DV findings are based on static analysis patterns and may identify issues reviewers missed.</critical>

    <action>Review each DV finding:
      - CRITICAL findings: Security vulnerabilities, race conditions, resource leaks - must address
      - ERROR findings: Bugs, missing error handling, boundary issues - should address
      - WARNING findings: Code quality concerns - consider addressing
    </action>

    <action>Cross-reference DV findings with reviewer findings:
      - DV + Reviewers agree: High confidence issue, prioritize in fix order
      - Only DV flags: Verify in source code - DV has precise line numbers
      - Only reviewers flag: May be design/logic issues DV can't detect
    </action>

    <action>DV findings may include evidence with:
      - Code quotes (exact text from source)
      - Line numbers (precise location, when available)
      - Pattern IDs (known antipattern reference)
      Use this evidence when applying fixes.</action>

    <action>DV patterns reference:
      - CC-*: Concurrency issues (race conditions, deadlocks)
      - SEC-*: Security vulnerabilities
      - DB-*: Database/storage issues
      - DT-*: Data transformation issues
      - GEN-*: General code quality (null handling, resource cleanup)
    </action>
  </step>

  <step n="2" goal="Verify issues and identify false positives">
    <action>For each issue, verify against embedded code context:
      - Does the issue actually exist in the current code?
      - Is the suggested fix appropriate for the codebase patterns?
      - Would the fix introduce new issues or regressions?
    </action>
    <action>Document false positives with clear reasoning:
      - Why the reviewer was wrong
      - What evidence contradicts the finding
      - Reference specific code or project_context.md patterns
    </action>
  </step>

  <step n="3" goal="Prioritize by severity">
    <action>For verified issues, assign severity:
      - Critical: Security vulnerabilities, data corruption, crashes
      - High: Bugs that break functionality, performance issues
      - Medium: Code quality issues, missing error handling
      - Low: Style issues, minor improvements, documentation
    </action>
    <action>Order fixes by severity - Critical first, then High, Medium, Low</action>
    <action>For disputed issues (reviewers disagree), note for manual resolution</action>
  </step>

  <step n="4" goal="Apply fixes to source code">
    <critical>This is SOURCE CODE modification, not story file modification</critical>
    <critical>Use Edit tool for all code changes - preserve surrounding code</critical>
    <critical>After applying each fix group, run: pytest -q --tb=line --no-header</critical>
    <critical>NEVER proceed to next fix if tests are broken - either revert or adjust</critical>

    <action>For each verified issue (starting with Critical):
      1. Identify the source file(s) from reviewer findings
      2. Apply fix using Edit tool - change ONLY the identified issue
      3. Preserve code style, indentation, and surrounding context
      4. Log the change for synthesis report
    </action>

    <action>After each logical fix group (related changes):
      - Run: pytest -q --tb=line --no-header
      - If tests pass, continue to next fix
      - If tests fail:
        a. Analyze which fix caused the failure
        b. Either revert the problematic fix OR adjust implementation
        c. Run tests again to confirm green state
        d. Log partial fix failure in synthesis report
    </action>

    <action>Atomic commit guidance (for user reference):
      - Commit message format: fix(component): brief description (synthesis-dl-5.1)
      - Group fixes by severity and affected component
      - Never commit unrelated changes together
      - User may batch or split commits as preferred
    </action>
  </step>

  <step n="5" goal="Refactor if needed">
    <critical>Only refactor code directly related to applied fixes</critical>
    <critical>Maximum scope: files already modified in Step 4</critical>

    <action>Review applied fixes for duplication patterns:
      - Same fix applied 2+ times across files = candidate for refactor
      - Only if duplication is in files already modified
    </action>

    <action>If refactoring:
      - Extract common logic to shared function/module
      - Update all call sites in modified files
      - Run tests after refactoring: pytest -q --tb=line --no-header
      - Log refactoring in synthesis report
    </action>

    <action>Do NOT refactor:
      - Unrelated code that "could be improved"
      - Files not touched in Step 4
      - Patterns that work but are just "not ideal"
    </action>

    <action>If broader refactoring needed:
      - Note it in synthesis report as "Suggested future improvement"
      - Do not apply - leave for dedicated refactoring story
    </action>
  </step>

  <step n="6" goal="Generate synthesis report">
    <critical>When updating story file, use atomic write pattern (temp file + rename).</critical>
    <action>Update story file Dev Agent Record section ONLY:
      - Mark completed tasks with [x] if fixes address them
      - Append to "Completion Notes List" subsection summarizing changes applied
      - Update file list with all modified files
    </action>

    <critical>Your synthesis report MUST be wrapped in HTML comment markers for extraction:</critical>
    <action>Produce structured output in this exact format (including the markers):</action>
    <output-format>
&lt;!-- CODE_REVIEW_SYNTHESIS_START --&gt;
## Synthesis Summary
[Brief overview: X issues verified, Y false positives dismissed, Z fixes applied to source files]

## Validations Quality
[For each reviewer: ID (A, B, C...), score (1-10), brief assessment]
[Note: Reviewers are anonymized - do not attempt to identify providers]

## Issues Verified (by severity)

### Critical
[Issues that required immediate fixes - list with evidence and fixes applied]
[Format: "- **Issue**: Description | **Source**: Reviewer(s) | **File**: path | **Fix**: What was changed"]
[If none: "No critical issues identified."]

### High
[Bugs and significant problems - same format]

### Medium
[Code quality issues - same format]

### Low
[Minor improvements - same format, note any deferred items]

## Issues Dismissed
[False positives with reasoning for each dismissal]
[Format: "- **Claimed Issue**: Description | **Raised by**: Reviewer(s) | **Dismissal Reason**: Why this is incorrect"]
[If none: "No false positives identified."]

## Changes Applied
[Complete list of modifications made to source files]
[Format for each change:
  **File**: [path/to/file.py]
  **Change**: [Brief description]
  **Before**:
  ```
  [2-3 lines of original code]
  ```
  **After**:
  ```
  [2-3 lines of updated code]
  ```
]
[If no changes: "No source code changes required."]

## Deep Verify Integration
[If DV findings were present, document how they were handled]

### DV Findings Fixed
[List DV findings that resulted in code changes]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **File**: {path} | **Fix**: {What was changed}"]

### DV Findings Dismissed
[List DV findings determined to be false positives]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Reason**: {Why this is not an issue}"]

### DV-Reviewer Overlap
[Note findings flagged by both DV and reviewers - highest confidence fixes]
[If no DV findings: "Deep Verify did not produce findings for this story."]

## Files Modified
[Simple list of all files that were modified]
- path/to/file1.py
- path/to/file2.py
[If none: "No files modified."]

## Suggested Future Improvements
[Broader refactorings or improvements identified in Step 5 but not applied]
[Format: "- **Scope**: Description | **Rationale**: Why deferred | **Effort**: Estimated complexity"]
[If none: "No future improvements identified."]

## Test Results
[Final test run output summary]
- Tests passed: X
- Tests failed: 0 (required for completion)
&lt;!-- CODE_REVIEW_SYNTHESIS_END --&gt;
    </output-format>

  </step>

  <step n="6.5" goal="Write Senior Developer Review section to story file for dev_story rework detection">
    <critical>This section enables dev_story to detect that a code review has occurred and extract action items.</critical>
    <critical>APPEND this section to the story file - do NOT replace existing content.</critical>

    <action>Determine the evidence verdict from the [Evidence Score] section:
      - REJECT: Evidence score exceeds reject threshold
      - PASS: Evidence score is below accept threshold
      - UNCERTAIN: Evidence score is between thresholds
    </action>

    <action>Map evidence verdict to review outcome:
      - PASS → "Approved"
      - REJECT → "Changes Requested"
      - UNCERTAIN → "Approved with Reservations"
    </action>

    <action>Append to story file "## Senior Developer Review (AI)" section:
      ```
      ## Senior Developer Review (AI)

      ### Review: {current_date}
      - **Reviewer:** AI Code Review Synthesis
      - **Evidence Score:** {evidence_score} → {evidence_verdict}
      - **Issues Found:** {total_verified_issues}
      - **Issues Fixed:** {fixes_applied_count}
      - **Action Items Created:** {remaining_unfixed_count}
      ```
    </action>

    <critical>When evidence verdict is REJECT, you MUST create Review Follow-ups tasks.
      If "Action Items Created" count is &gt; 0, there MUST be exactly that many [ ] [AI-Review] tasks.
      Do NOT skip this step. Do NOT claim all issues are fixed if you reported deferred items above.</critical>

    <action>Find the "## Tasks / Subtasks" section in the story file</action>
    <action>Append a "#### Review Follow-ups (AI)" subsection with checkbox tasks:
      ```
      #### Review Follow-ups (AI)
      - [ ] [AI-Review] {severity}: {brief description of unfixed issue} ({file path})
      ```
      One line per unfixed/deferred issue, prefixed with [AI-Review] tag.
      Order by severity: Critical first, then High, Medium, Low.
    </action>

    <critical>ATDD DEFECT CHECK: Search test directories (tests/**) for test.fixme() calls in test files related to this story.
      If ANY test.fixme() calls remain in story-related test files, this is a DEFECT — the dev_story agent failed to activate ATDD RED-phase tests.
      Create an additional [AI-Review] task:
      - [ ] [AI-Review] HIGH: Activate ATDD tests — convert all test.fixme() to test() and ensure they pass ({test file paths})
      Do NOT dismiss test.fixme() as "intentional TDD methodology". After dev_story completes, ALL test.fixme() tests for the story MUST be converted to test().</critical>
  </step>

  </workflow></instructions>
<output-template></output-template>
</compiled-workflow>