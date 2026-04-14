<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-3 -->
<!-- Story: 2 -->
<!-- Phase: validate-story-synthesis -->
<!-- Timestamp: 20260413T222624Z -->
<compiled-workflow>
<mission><![CDATA[

Master Synthesis: Story fn-3.2

You are synthesizing 2 independent validator reviews.

Your mission:
1. VERIFY each issue raised by validators
   - Cross-reference with story content
   - Identify false positives (issues that aren't real problems)
   - Confirm valid issues with evidence

2. PRIORITIZE real issues by severity
   - Critical: Blocks implementation or causes major problems
   - High: Significant gaps or ambiguities
   - Medium: Improvements that would help
   - Low: Nice-to-have suggestions

3. SYNTHESIZE findings
   - Merge duplicate issues from different validators
   - Note validator consensus (if 3+ agree, high confidence)
   - Highlight unique insights from individual validators

4. APPLY changes to story file
   - You have WRITE PERMISSION to modify the story
   - CRITICAL: Before using Edit tool, ALWAYS Read the target file first
   - Use EXACT content from Read tool output as old_string, NOT content from this prompt
   - If Read output is truncated, use offset/limit parameters to locate the target section
   - Apply fixes for verified issues
   - Document what you changed and why

Output format:
## Synthesis Summary
## Issues Verified (by severity)
## Issues Dismissed (false positives with reasoning)
## Changes Applied

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


]]></file>
<file id="1dffb869" path="_bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md" label="DOCUMENTATION"><![CDATA[

# Story fn-3.2: Wizard Timezone Auto-Detect

Status: ready-for-dev

## Story

As a new user completing the setup wizard,
I want the wizard to auto-detect my timezone from my browser and let me confirm or change it,
So that my FlightWall displays correct local times for flights without manual POSIX string entry.

## Acceptance Criteria

1. **Auto-Detect on Step 3 Load:**
   - Given the user navigates to Step 3 (Location)
   - When the step loads (initial or rehydration via `showStep(3)`)
   - Then the browser's timezone is detected via `Intl.DateTimeFormat().resolvedOptions().timeZone`
   - And if the detected IANA timezone exists in `TZ_MAP`, the corresponding dropdown option is pre-selected
   - And if no prior user selection exists in `state.timezone`, the auto-detected value is applied
   - And if a prior user selection exists in `state.timezone`, the user's selection is preserved (not overwritten by auto-detect)

2. **Timezone Dropdown:**
   - Given Step 3 is displayed
   - When rendered
   - Then a `<select>` dropdown is shown below the Radius field, labeled "Timezone"
   - And the dropdown is populated with all IANA timezone names from `TZ_MAP` keys (sorted alphabetically)
   - And each option displays the IANA name (e.g., "America/Los_Angeles")
   - And the dropdown uses the existing `.cal-select` CSS class for consistent styling

3. **Fallback for Unknown Timezones:**
   - Given the browser reports an IANA timezone not present in `TZ_MAP`
   - When auto-detection runs
   - Then the dropdown defaults to "UTC" as a safe fallback
   - And the auto-detected (unmapped) IANA name is NOT shown in the dropdown (no phantom entries)

4. **POSIX String Sent to Device:**
   - Given the user completes the wizard and settings are saved
   - When `saveAndConnect()` builds the payload
   - Then `payload.timezone` is set to the POSIX string from `TZ_MAP[selectedIANA]` (e.g., "PST8PDT,M3.2.0,M11.1.0")
   - And the POSIX string (not the IANA name) is what gets stored in NVS via `POST /api/settings`

5. **Review Step Shows Timezone:**
   - Given the user reaches Step 5 (Review)
   - When `buildReview()` renders the Location section
   - Then a "Timezone" row is displayed showing the selected IANA timezone name
   - And clicking the Location review card navigates back to Step 3 where the timezone dropdown is editable

6. **Settings Import Compatibility:**
   - Given the user imports settings that include a `timezone` POSIX value
   - When the import is processed
   - Then the imported timezone is preserved in `importedExtras` (existing behavior via `KNOWN_EXTRA_KEYS`)
   - And if the wizard form timezone differs from the imported value, the wizard form value takes precedence in the final payload
   - And the `timezone` key is no longer treated as an "extra" — it is a first-class wizard field

7. **Asset Budget:**
   - Given updated wizard web assets (wizard.html, wizard.js)
   - When gzipped and placed in `firmware/data/`
   - Then total gzipped web asset size remains under 50KB budget

## Tasks / Subtasks

- [ ] Task 1: Add timezone dropdown HTML to Step 3 in wizard.html (AC: #2)
  - [ ] 1.1: Add `<label for="wizard-timezone">Timezone</label>` after the radius field and before `<p id="loc-err">` in the Step 3 `<div class="form-fields">`
  - [ ] 1.2: Add `<select id="wizard-timezone" class="cal-select"></select>` — empty options, populated by JS at init

- [ ] Task 2: Wire timezone into wizard.js state and WIZARD_KEYS (AC: #1, #4, #6)
  - [ ] 2.1: Add `timezone: ''` to the `state` object (line ~108, after `radius_km`)
  - [ ] 2.2: Add `'timezone'` to the `WIZARD_KEYS` array (line ~130, after `'radius_km'`)
  - [ ] 2.3: Remove `'timezone'` from `KNOWN_EXTRA_KEYS` array (line ~143) — it is now a wizard key, not an extra. **Critical:** If timezone remains in both `WIZARD_KEYS` and `KNOWN_EXTRA_KEYS`, an imported timezone could override the user's wizard selection via `importedExtras` merge in `saveAndConnect()`

- [ ] Task 3: Populate timezone dropdown and auto-detect on init (AC: #1, #2, #3)
  - [ ] 3.1: In the IIFE init section, grab the DOM element: `var wizardTimezone = document.getElementById('wizard-timezone');`
  - [ ] 3.2: Populate the dropdown from `TZ_MAP` keys sorted alphabetically:
    ```js
    var tzKeys = Object.keys(TZ_MAP).sort();
    tzKeys.forEach(function(iana) {
      var opt = document.createElement('option');
      opt.value = iana;
      opt.textContent = iana;
      wizardTimezone.appendChild(opt);
    });
    ```
  - [ ] 3.3: Auto-detect browser timezone and set initial state:
    ```js
    var detected = getTimezoneConfig();
    // Only auto-fill if state.timezone is empty (no prior selection or import)
    if (!state.timezone) {
      state.timezone = detected.iana;
    }
    wizardTimezone.value = state.timezone;
    ```
    Note: `getTimezoneConfig()` already exists at wizard.js lines 92-99 and handles the TZ_MAP lookup + UTC fallback.

- [ ] Task 4: Wire timezone into step navigation lifecycle (AC: #1, #5)
  - [ ] 4.1: Update `saveCurrentStepState()` — in the `currentStep === 3` block (line ~511), add:
    ```js
    state.timezone = wizardTimezone.value;
    ```
  - [ ] 4.2: Update `showStep(3)` rehydration block (line ~476) — add after `radiusKm.value`:
    ```js
    wizardTimezone.value = state.timezone || getTimezoneConfig().iana;
    ```
  - [ ] 4.3: Update `buildReview()` — add a Timezone item to the Location section (line ~395):
    ```js
    { label: 'Timezone', value: state.timezone || 'UTC' }
    ```

- [ ] Task 5: Include timezone POSIX value in save payload (AC: #4)
  - [ ] 5.1: In `saveAndConnect()` payload construction (line ~630), add after `radius_km`:
    ```js
    timezone: TZ_MAP[state.timezone] || 'UTC0',
    ```
    This converts the IANA name stored in `state.timezone` to the POSIX string the firmware expects.

- [ ] Task 6: Handle settings import timezone hydration (AC: #6)
  - [ ] 6.1: In the existing settings import logic (where `KNOWN_EXTRA_KEYS` values are read from imported JSON), if the imported data contains `timezone` as a POSIX string, reverse-lookup the IANA name from `TZ_MAP` to populate `state.timezone`. If no reverse match, keep `state.timezone` at the auto-detected value.
    **Implementation hint:** Build a one-time reverse map:
    ```js
    var POSIX_TO_IANA = {};
    Object.keys(TZ_MAP).forEach(function(iana) {
      if (!POSIX_TO_IANA[TZ_MAP[iana]]) POSIX_TO_IANA[TZ_MAP[iana]] = iana;
    });
    ```
    Note: Multiple IANA zones map to the same POSIX string (e.g., America/New_York and America/Toronto both map to EST5EDT). The reverse lookup picks the first alphabetically, which is acceptable for display purposes.

- [ ] Task 7: Update E2E test page objects and mock server (AC: #1, #2)
  - [ ] 7.1: In `tests/e2e/page-objects/WizardPage.ts`, add locator: `timezoneSelect = this.page.locator('#wizard-timezone');`
  - [ ] 7.2: Add helper method to `WizardPage`:
    ```ts
    async selectTimezone(iana: string) {
      await this.timezoneSelect.selectOption(iana);
    }
    ```
  - [ ] 7.3: Update `configureLocation()` method to optionally accept and set timezone
  - [ ] 7.4: In `tests/e2e/mock-server/server.ts`, add `timezone: 'UTC0'` to the default settings state if not already present

- [ ] Task 8: Rebuild gzipped assets (AC: #7)
  - [ ] 8.1: `gzip -9 -c firmware/data-src/wizard.html > firmware/data/wizard.html.gz`
  - [ ] 8.2: `gzip -9 -c firmware/data-src/wizard.js > firmware/data/wizard.js.gz`
  - [ ] 8.3: Verify total gzipped asset size remains under 50KB

## Dev Notes

### Critical Architecture Constraints

- **Architecture Decision F4:** Browser-side IANA-to-POSIX mapping via JS table. Zero firmware overhead — the `TZ_MAP` object in wizard.js (lines 7-86) already contains 60+ entries. The firmware only sees the resulting POSIX string via `POST /api/settings`. [Source: architecture.md#Decision-F4]
- **NVS key `timezone`:** String type, default "UTC0", max 40 chars, stored in ScheduleConfig struct. Hot-reload — no reboot required. The firmware calls `configTzTime(tz.c_str(), "pool.ntp.org", "time.nist.gov")` immediately on change. [Source: architecture.md#NVS-Key-Table]
- **Enforcement Rule 10:** Every JS `fetch()` call must check `json.ok` and handle failure with `FW.showToast()`. This story adds no new API calls — timezone is included in the existing `saveAndConnect()` payload. [Source: architecture.md#Enforcement-Guidelines]
- **Web asset budget:** Total gzipped assets are at ~49.22KB of 50KB budget. This story adds ~5 lines of HTML and ~20 lines of JS. The TZ_MAP already exists and is already gzipped. Expected impact: <0.1KB additional gzipped. [Source: fn-3.1 dev notes]
- **Gzip build process:** No automated script. After editing `data-src/` files, manually run `gzip -9 -c data-src/X > data/X.gz` from `firmware/` directory. [Source: MEMORY.md]

### Existing Code to Reuse (DO NOT Reinvent)

**`TZ_MAP` (wizard.js lines 7-86):** Already fully implemented with 60+ IANA-to-POSIX entries. DO NOT create a new mapping table. Reference this object directly.

**`getTimezoneConfig()` (wizard.js lines 92-99):** Already implemented — detects browser IANA timezone via `Intl.DateTimeFormat().resolvedOptions().timeZone`, looks up POSIX string in `TZ_MAP`, returns `{ iana, posix }`. Currently dead code relative to the wizard UI. This story wires it in. DO NOT rewrite this function.

**Dashboard timezone dropdown (dashboard.js lines 1042-1076):** Reference implementation for how to populate a `<select>` from `TZ_MAP` keys. Uses `Object.keys(TZ_MAP).sort()`, creates `<option>` elements. The wizard implementation should follow this same pattern. The dashboard uses `#nm-timezone` with `.cal-select` class — the wizard uses `#wizard-timezone` with the same class.

**`KNOWN_EXTRA_KEYS` (wizard.js line 139-144):** Currently includes `'timezone'`. This story MOVES timezone from extras to a first-class wizard key. Failure to remove it from `KNOWN_EXTRA_KEYS` would cause the imported POSIX value to overwrite the user's IANA-derived POSIX value in `saveAndConnect()` payload merge.

### State Flow

```
Browser auto-detect → getTimezoneConfig() → { iana: "America/Los_Angeles", posix: "PST8PDT,..." }
                                                    ↓
state.timezone = "America/Los_Angeles"  ← IANA name stored in state
                                                    ↓
Dropdown shows IANA name ← user can change selection
                                                    ↓
saveAndConnect() → payload.timezone = TZ_MAP[state.timezone] → "PST8PDT,M3.2.0,M11.1.0"
                                                    ↓
POST /api/settings → firmware stores POSIX string in NVS → configTzTime() applies it
```

### Key Difference from Dashboard

The **dashboard** stores the IANA name in the dropdown value and converts to POSIX on save. The **wizard** must do the same: `state.timezone` holds the IANA name (for dropdown display and review), and `TZ_MAP[state.timezone]` produces the POSIX string only at save time. Do NOT store the POSIX string in `state.timezone` — that would make the dropdown and review display unreadable.

### Files Modified

| File | Change |
|------|--------|
| `firmware/data-src/wizard.html` | Add timezone `<select>` to Step 3 |
| `firmware/data-src/wizard.js` | Add `timezone` to state/WIZARD_KEYS, remove from KNOWN_EXTRA_KEYS, populate dropdown, wire auto-detect, add to review, add to payload |
| `firmware/data/wizard.html.gz` | Rebuild gzip |
| `firmware/data/wizard.js.gz` | Rebuild gzip |
| `tests/e2e/page-objects/WizardPage.ts` | Add timezone locator and helper |
| `tests/e2e/mock-server/server.ts` | Add timezone to default state |

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-fn-3.md` → Story fn-3.2
- PRD Requirements: FR17 (auto-detect timezone), FR18 (timezone config)
- Architecture: Decision F4 (browser-side IANA-to-POSIX mapping)
- UX Design Rule: UX-DR11 (timezone pre-fill from browser)
- Previous Story: fn-3.1 (wizard Step 6 — Test Your Wall) — established Step 6 as final step, TOTAL_STEPS=6


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

# 🎯 Story Context Validation Report

<!-- report_header -->

**Story:** fn-3.2 - wizard-timezone-auto-detect
**Story File:** _bmad-output/implementation-artifacts/stories/fn-3-2-wizard-timezone-auto-detect.md
**Validated:** 2026-04-13
**Validator:** Quality Competition Engine

---

<!-- executive_summary -->

## Executive Summary

### Issues Overview

| Category | Found | Applied |
|----------|-------|---------|
| 🚨 Critical Issues | 2 | 0 |
| ⚡ Enhancements | 1 | 0 |
| ✨ Optimizations | 1 | 0 |
| 🤖 LLM Optimizations | 0 | 0 |

**Overall Assessment:** The story is well-structured and technically aligned with the browser-side mapping architecture (Decision F4). However, it contains a significant implementation "footgun" regarding how POSIX strings from imported settings/device hydration will interact with the IANA-based dropdown state, potentially leading to data corruption (reverting to UTC) upon wizard navigation.

---

<!-- evidence_score_summary -->

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | POSIX-to-IANA clobbering in Import loop | Implementation | +3 |
| 🔴 CRITICAL | Missing hydration reverse-lookup in hydrateDefaults | Implementation | +3 |
| 🟠 IMPORTANT | Omission of reusable reverse-lookup helper | Maintainability | +1 |
| 🟡 MINOR | Redundant TZ_MAP description in Dev Notes | LLM Opt | +0.3 |
| 🟢 CLEAN PASS | INVEST Criteria (6/6) | INVEST | -3.0 |
| 🟢 CLEAN PASS | Acceptance Criteria | AC | -0.5 |
| 🟢 CLEAN PASS | Hidden Dependencies | Deps | -0.5 |
| 🟢 CLEAN PASS | Technical Alignment | Architecture | -0.5 |

### Evidence Score: 2.8

| Score | Verdict |
|-------|---------|
| **2.8** | **PASS** |

---

<!-- story_quality_gate -->

## 🎯 Ruthless Story Validation fn-3.2

### INVEST Criteria Assessment

| Criterion | Status | Severity | Details |
|-----------|--------|----------|---------|
| **I**ndependent | PASS | 0/10 | No blocking dependencies on unfinished work. |
| **N**egotiable | PASS | 0/10 | Requirements focus on outcomes; technical hints are provided as guidance. |
| **V**aluable | PASS | 0/10 | High value: eliminates manual POSIX string entry for end users. |
| **E**stimable | PASS | 0/10 | Tasks are granular and mapped to specific file locations. |
| **S**mall | PASS | 0/10 | Well-scoped for a single implementation turn. |
| **T**estable | PASS | 0/10 | Clear browser-level and API-level verification steps. |

### INVEST Violations

✅ No significant INVEST violations detected.

### Acceptance Criteria Issues

✅ Acceptance criteria are well-defined and testable.

### Hidden Risks and Dependencies

- **DATA INTEGRITY RISK:** The mismatch between the IANA names used in the UI (`state.timezone`) and the POSIX strings stored in the firmware/NVS (`payload.timezone`) creates a risk where raw POSIX strings from imported files could be mismanaged by the generic hydration loops.
  - *Impact:* User settings could be silently reverted to UTC or empty selections upon entering Step 3.
  - *Mitigation:* Explicitly require reverse-lookup logic during all state hydration phases (Import and Bootstrap).

### Estimation Reality-Check

**Assessment:** realistic

The effort correctly identifies the need for E2E page object updates and gzipped asset management. The logic additions are minimal but require precision.

### Technical Alignment

**Status:** ALIGNED

✅ Story aligns with architecture.md patterns (Decision F4 - Browser-side mapping).

---

<!-- critical_issues_section -->

## 🚨 Critical Issues (Must Fix)

### 1. POSIX-to-IANA Clobbering in Import Loop

**Impact:** Data Corruption / UX Regression
**Source:** `wizard.js` lines 992-1002 (`processImportedSettings`)

**Problem:**
Task 2.2 adds `timezone` to `WIZARD_KEYS`. The existing `processImportedSettings` loop iterates `WIZARD_KEYS` and assigns `state[key] = String(parsed[key])`. Because exported settings files from the device contain the **POSIX string** (e.g., `"PST8PDT,..."`), the loop will populate `state.timezone` with the POSIX string. When the user navigates to Step 3, the dropdown (which expects IANA names like `"America/Los_Angeles"`) will fail to match the value, appearing empty. Subsequent navigation or saving will then send `TZ_MAP["PST8PDT,..."]` (undefined), defaulting the device back to `"UTC0"`.

**Recommended Fix:**
Explicitly instruct the developer to handle `timezone` specially within the `WIZARD_KEYS` loop in `processImportedSettings`. If the key is `timezone`, it must undergo the reverse-lookup to IANA name *before* being assigned to `state.timezone`.

### 2. Missing Hydration Reverse-Lookup in hydrateDefaults

**Impact:** UI Inconsistency / User Confusion
**Source:** `wizard.js` lines 204-228 (`hydrateDefaults`)

**Problem:**
The story tasks focus on Step 3 load and Settings Import but omit updating the `hydrateDefaults()` function which bootstraps the wizard from existing device settings. If a user has already configured a timezone and reloads the wizard, `hydrateDefaults` will (if updated following existing patterns) fetch the POSIX string from `/api/settings`. Without a reverse-lookup to IANA, Step 3 will show an incorrect/empty selection instead of the current device setting.

**Recommended Fix:**
Add a task to update `hydrateDefaults()` to include `timezone` hydration, utilizing the same reverse-lookup logic used for imports.

---

<!-- enhancements_section -->

## ⚡ Enhancement Opportunities (Should Add)

### 1. Centralized POSIX-to-IANA Reverse Lookup Helper

**Benefit:** Maintainability / Dry Code
**Source:** Task 6.1

**Current Gap:**
Task 6.1 suggests building a one-time reverse map within the import logic. However, this logic is needed in at least two places (Import and `hydrateDefaults`).

**Suggested Addition:**
Declare a reusable `getIanaFromPosix(posix)` helper function next to `getTimezoneConfig()` that encapsulates the `POSIX_TO_IANA` mapping and lookup logic. This ensures consistency across all hydration paths.

---

<!-- optimizations_section -->

## ✨ Optimizations (Nice to Have)

### 1. Dropdown "Please select" placeholder

**Value:** UX Clarity

**Suggestion:**
While auto-detect is the primary path, if detection fails or is disabled, the dropdown should ideally have a disabled "Select your timezone..." top option to prevent accidental "America/Abidjan" (first alphabetical) selections if the user hasn't interacted with the field.

---

<!-- competition_results -->

## 🏆 Competition Results

### Quality Metrics

| Metric | Score |
|--------|-------|
| Requirements Coverage | 90% |
| Architecture Alignment | 100% |
| Previous Story Integration | 100% |
| LLM Optimization Score | 95% |
| **Overall Quality Score** | **96%** |

### Disaster Prevention Assessment

- **Reinvention Prevention:** PASS (Reuses existing `TZ_MAP` and `getTimezoneConfig`).
- **Technical Specification:** FAIL (Missing critical guard against POSIX raw-string hydration in loops).
- **File Structure:** PASS (Correct paths and gzip instructions).
- **Regression Disasters:** PASS (Maintains `TOTAL_STEPS=6` and existing wizard keys).
- **Implementation Disasters:** FAIL (Identified data corruption risk on re-entry/import).

### Competition Outcome

🏆 **Validator identified 2 critical improvements** regarding state hydration and data integrity that prevent the user's timezone from being reset to UTC during common configuration scenarios.

---

**Report Generated:** 2026-04-13
**Validation Engine:** BMAD Method Quality Competition v1.0

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

# 🎯 Story Context Validation Report

**Story:** fn-3.2-wizard-timezone-auto-detect
**Story Title:** wizard-timezone-auto-detect
**Story File:** _bmad-output/implementation-artifacts/stories/fn-3.2-wizard-timezone-auto-detect.md
**Validated:** 2026-04-13
**Validator:** Quality Competition Engine

---

## Executive Summary

### Issues Overview

| Category | Found | Applied |
|----------|-------|---------|
| 🚨 Critical Issues | 3 | 0 |
| ⚡ Enhancements | 4 | 0 |
| ✨ Optimizations | 3 | 0 |
| 🤖 LLM Optimizations | 2 | 0 |

**Overall Assessment:** Story is well-structured but has critical gaps in settings import handling and device state restoration. The original story correctly identifies the main architecture pattern but misses important integration points with existing code.

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🔴 CRITICAL | AC #1 timing ambiguity - auto-detect timing unclear vs hydrateDefaults() | Story AC #1 vs code lines 204-228 | +3 |
| 🔴 CRITICAL | Missing hydrateDefaults() update for timezone restoration | Task 2.1 vs code lines 204-228 | +3 |
| 🔴 CRITICAL | Settings import reverse-lookup implementation unclear | Task 6.1 vs code lines 1003-1012 | +3 |
| 🟠 IMPORTANT | Task 4.2 rehydration logic has empty string bug | Task 4.2 code snippet | +1 |
| 🟠 IMPORTANT | Missing DOM reference for wizardTimezone | Task 3.1 vs code lines 174-180 | +1 |
| 🟠 IMPORTANT | Task 6.1 doesn't specify WHERE to add reverse-lookup code | Task 6.1 description | +1 |
| 🟠 IMPORTANT | No AC for timezone rehydration on Step 3 re-entry | Acceptance Criteria section | +1 |
| 🟡 MINOR | Verbose task descriptions with exact line numbers | All tasks | +0.3 |
| 🟡 MINOR | Duplicate "DO NOT reinvent" warnings | Dev Notes sections | +0.3 |
| 🟡 MINOR | Missing UX detail about UTC dropdown position | AC #2 | +0.3 |
| 🟢 CLEAN PASS | INVEST criteria (Independence, Valuable, Estimable, Small, Testable) | INVEST validation | -0.5 |
| 🟢 CLEAN PASS | Technical alignment with architecture.md | Architecture references | -0.5 |
| 🟢 CLEAN PASS | Acceptance criteria testability | AC section | -0.5 |
| 🟢 CLEAN PASS | No hidden dependencies | Dependencies analysis | -0.5 |
| 🟢 CLEAN PASS | Estimation realistic | Scope analysis | -0.5 |

### Evidence Score: 5.2

| Score | Verdict |
|-------|---------|
| **5.2** | **MAJOR REWORK** |

---

## 🎯 Ruthless Story Validation 3.2

### INVEST Criteria Assessment

| Criterion | Status | Severity | Details |
|-----------|--------|----------|---------|
| **I**ndependent | ✅ PASS | 0/10 | Story can be developed independently. Reuses existing TZ_MAP and getTimezoneConfig(). No external dependencies. |
| **N**egotiable | ⚠️ WARN | 2/10 | Somewhat prescriptive with exact line numbers and code snippets, but acceptable for this scope. Leaves room for implementation decisions. |
| **V**aluable | ✅ PASS | 0/10 | Clear value: auto-detect timezone reduces user friction during onboarding. Contributes to epic fn-3 goal. |
| **E**stimable | ✅ PASS | 0/10 | Well-defined scope. 8 focused tasks. No unknown technical risks. However, missing implementation details for settings import. |
| **S**mall | ✅ PASS | 0/10 | 8 tasks, all focused on one feature. Can be completed in a single sprint. Appropriately sized. |
| **T**estable | ✅ PASS | 0/10 | All 7 ACs are specific and measurable. Covers edge cases (unknown timezone, settings import). However, missing AC for rehydration scenarios. |

### INVEST Violations

- **[2/10] N - Negotiable:** Task descriptions include exact line numbers and detailed code snippets. This is appropriate for this small scope but creates maintenance burden if code changes.

✅ No other significant INVEST violations detected.

### Acceptance Criteria Issues

- **[IMPORTANT] AC #1 timing ambiguity:** 
  - *Quote:* "When the step loads (initial or rehydration via `showStep(3)`)"
  - *Recommendation:* Clarify that auto-detect runs ONCE at IIFE init (not on every showStep call), and that rehydration preserves user selection. The current wording conflicts with Task 3.3 which says "Only auto-fill if state.timezone is empty".

- **[IMPORTANT] Missing rehydration AC:**
  - *Recommendation:* Add AC covering: "Given the user selected a timezone and navigated away, when returning to Step 3, Then the user's previous selection is preserved (not re-auto-detected)."

- **[MINOR] AC #2 ambiguous about UTC position:**
  - *Quote:* "dropdown is populated with all IANA timezone names from `TZ_MAP` keys (sorted alphabetically)"
  - *Recommendation:* Specify whether "UTC" should be the first option or if detected timezone should be auto-selected. Current implementation will auto-select detected timezone in init but AC doesn't state this expectation clearly.

### Hidden Risks and Dependencies

- **[IMPORTANT] Dependency on existing import logic:** Task 6.1 requires modifying processImportedSettings() but doesn't specify WHERE. The reverse-lookup for POSIX→IANA needs to happen between lines 1002-1012, but this isn't explicitly stated.

- **[IMPORTANT] Dependency on DOM references section:** Task 3.1 adds `wizardTimezone` DOM element but doesn't mention adding it to the DOM references section (around line 180). This will cause the element to not be cached properly.

- **[MINOR] Dependency on hydrateDefaults() pattern:** Task 2.1 adds `timezone` to state but doesn't specify updating hydrateDefaults() to restore it from `/api/settings` on page reload. This is implied but not explicit.

### Estimation Reality-Check

**Assessment:** Realistic

The story's scope is appropriate for a single sprint. 8 focused tasks covering HTML, JS state management, dropdown population, auto-detection, settings payload, import handling, E2E tests, and gzip rebuild. The only complexity is the settings import reverse-lookup which is not fully specified.

### Technical Alignment

**Status:** Good alignment with minor gaps

- **[IMPORTANT] Missing hydrateDefaults() integration:** Story correctly references architecture.md Decision F4 but doesn't account for the existing hydrateDefaults() pattern (lines 204-228) which needs to be updated to restore timezone from device settings.

- **[IMPORTANT] Incomplete settings import handling:** Story references existing KNOWN_EXTRA_KEYS pattern but doesn't fully explain the POSIX→IANA reverse-lookup implementation location.

✅ Story correctly follows TZ_MAP pattern from architecture.md Decision F4, uses existing .cal-select CSS class, and correctly stores IANA in state / converts to POSIX on save.

### Evidence Score: 5.2 → MAJOR REWORK

---

## 🚨 Critical Issues (Must Fix)

These are essential requirements, security concerns, or blocking issues that could cause implementation disasters.

### 1. Missing hydrateDefaults() Update for Timezone Restoration

**Impact:** HIGH - User's timezone won't persist if page is refreshed during setup
**Source:** Task 2.1 vs code lines 204-228

**Problem:**
Task 2.1 adds `timezone: ''` to the state object but doesn't specify updating the `hydrateDefaults()` function to restore timezone from `/api/settings`. The existing pattern (lines 209-218) shows that hardware config keys (`origin_corner`, `scan_dir`, `zigzag`) are restored from device settings. Without updating hydrateDefaults(), a user who refreshes the page will lose their timezone selection.

**Recommended Fix:**
Add to Task 2.1 or create new Task 2.4:
```js
// In hydrateDefaults(), add after line 218:
if (d.timezone !== undefined) state.timezone = d.timezone;
```

### 2. Settings Import Reverse-Lookup Implementation Location Unclear

**Impact:** HIGH - Developer may place reverse-lookup code in wrong location, causing import to fail
**Source:** Task 6.1 vs code lines 1003-1012

**Problem:**
Task 6.1 says "In the existing settings import logic (where `KNOWN_EXTRA_KEYS` values are read from imported JSON)" but doesn't specify the EXACT location. The reverse-lookup needs to happen when processing `timezone` from imported extras, converting POSIX → IANA for display. If placed incorrectly, the IANA value won't be in `state.timezone` for the dropdown.

**Recommended Fix:**
Replace Task 6.1 with more specific implementation:
```
Task 6.1: Handle settings import timezone hydration (AC: #6)
  - 6.1: In processImportedSettings(), after the KNOWN_EXTRA_KEYS loop (line 1012), add timezone reverse-lookup:
    ```js
    // Reverse-lookup imported POSIX to IANA for display
    if (importedExtras.timezone) {
      var POSIX_TO_IANA = {};
      Object.keys(TZ_MAP).forEach(function(iana) {
        if (!POSIX_TO_IANA[TZ_MAP[iana]]) POSIX_TO_IANA[TZ_MAP[iana]] = iana;
      });
      state.timezone = POSIX_TO_IANA[importedExtras.timezone] || getTimezoneConfig().iana;
      // Remove from extras so wizard form value takes precedence in saveAndConnect()
      delete importedExtras.timezone;
    }
    ```
```

### 3. AC #1 Timing Ambiguity - Auto-Detect vs. Rehydration

**Impact:** MEDIUM - Confusion about when auto-detect runs could cause implementation bugs
**Source:** AC #1 vs Task 3.3

**Problem:**
AC #1 says "When the step loads (initial or rehydration via `showStep(3)`)" which suggests auto-detect runs on every Step 3 entry. However, Task 3.3 says "Only auto-fill if state.timezone is empty" which suggests it runs once at init. The actual correct behavior per architecture Decision F4 is: auto-detect ONCE at IIFE init, then preserve user selection through navigation. The current wording is contradictory.

**Recommended Fix:**
Rewrite AC #1 to clarify:
```
1. **Auto-Detect on Wizard Init:**
   - Given the wizard loads for the first time
   - When the IIFE init section executes
   - Then the browser's timezone is detected via `Intl.DateTimeFormat().resolvedOptions().timeZone`
   - And if the detected IANA timezone exists in `TZ_MAP`, it is stored in `state.timezone`
   - And if no prior user selection exists (empty state), the auto-detected value is applied
   - And if a prior user selection or imported value exists in `state.timezone`, it is preserved (not overwritten by auto-detect)

   - Given the user navigates to Step 3 (Location) after initialization
   - When `showStep(3)` is called
   - Then the timezone dropdown displays the value from `state.timezone` (whether auto-detected, user-selected, or imported)
   - And no new auto-detection occurs (preserve user choice)
```

---

## ⚡ Enhancement Opportunities (Should Add)

Additional guidance that would significantly help the developer avoid mistakes.

### 1. Add DOM Reference for Timezone Dropdown

**Benefit:** Prevents DOM element from not being cached, following existing pattern
**Source:** Task 3.1 vs code lines 174-180

**Current Gap:**
Task 3.1 adds `wizardTimezone = document.getElementById('wizard-timezone')` in the IIFE init section but doesn't mention adding it to the "DOM references — Step 3" section (lines 174-180). The existing pattern shows all Step 3 DOM elements are declared at the top level for consistency.

**Suggested Addition:**
Add to Task 3.1:
```
- 3.1: In the IIFE init section, after `var radiusKm` (line ~179), add:
  ```js
  var wizardTimezone = document.getElementById('wizard-timezone');
  ```

- 3.2: Update comment on line 174 from "// DOM references — Step 3" to "// DOM references — Step 3 (including timezone dropdown)" for clarity
```

### 2. Clarify Rehydration Logic in Task 4.2

**Benefit:** Prevents empty string bug in dropdown value
**Source:** Task 4.2 code snippet

**Current Gap:**
Task 4.2 shows: `wizardTimezone.value = state.timezone || getTimezoneConfig().iana;`

This is correct but the explanation is missing a critical edge case: if `state.timezone` is an empty string (explicitly set by user clearing selection), this will fall back to auto-detected. The story should explain that `state.timezone` should be either a valid IANA string OR empty/null/undefined, and that empty string is treated as "not set".

**Suggested Addition:**
Add note to Task 4.2:
```
Note: `state.timezone` can be empty string, null, or undefined (all meaning "not set"). The OR operator correctly handles all these cases. If user explicitly cleared the field to empty string, auto-detected value is used as fallback. This matches the UX intent that the dropdown should never show a blank value.
```

### 3. Add Validation for Timezone Selection

**Benefit:** Ensures timezone is always set before save
**Source:** AC #4

**Current Gap:**
None of the tasks mention adding timezone to Step 3 validation. If the dropdown somehow becomes empty or has an invalid value, the save could fail or send undefined to the firmware.

**Suggested Addition:**
Add new Task 4.4:
```
- [ ] 4.4: Update validateStep(3) to ensure timezone is selected (AC: #4)
  - [ ] 4.4.1: In the `n === 3` block (line ~559), add validation:
    ```js
    if (!state.timezone) {
      errs.push(wizardTimezone);  // Need to add wizardTimezone to DOM references first (see Enhancement #1)
      // Show error: "Timezone is required"
    }
    ```
```

### 4. Document the UTC Default Fallback

**Benefit:** Clarifies behavior when auto-detect fails or user has no selection
**Source:** AC #3

**Current Gap:**
AC #3 says "dropdown defaults to 'UTC'" but doesn't explain WHEN this happens. Is it at init time? When user clears selection? When import has invalid timezone?

**Suggested Addition:**
Add to Dev Notes:
```
### UTC Fallback Behavior

The timezone dropdown uses UTC as safe fallback in these scenarios:
1. Browser's Intl API throws error or returns null (getTimezoneConfig() returns { iana: "UTC", posix: "UTC0" })
2. Browser reports IANA timezone not in TZ_MAP (getTimezoneConfig() returns POSIX "UTC0" but state.timezone remains undefined, so rehydration shows UTC)
3. User clears the dropdown selection (empty string → getTimezoneConfig().iana fallback in rehydration)
4. Imported settings have unrecognized POSIX string (reverse-lookup fails → falls back to getTimezoneConfig().iana)

All fallback paths ultimately select the "UTC" IANA entry from TZ_MAP (line 84), which maps to "UTC0" POSIX string.
```

---

## ✨ Optimizations (Nice to Have)

Performance hints, development tips, and additional context for complex scenarios.

### 1. Suggest Using Object.hasOwn for Clean UTC Fallback

**Value:** More robust handling of edge cases in reverse-lookup

**Suggestion:**
In Task 6.1, the POSIX_TO_IANA build could use `Object.hasOwn(TZ_MAP, iana)` for safety, though unlikely to matter in practice since TZ_MAP is controlled code.

### 2. Document the Duplicate TZ_MAP Maintenance

**Value:** Prevents future drift between wizard.js and dashboard.js

**Suggestion:**
Add to Dev Notes under "Existing Code to Reuse":
```
### TZ_MAP Synchronization

The TZ_MAP object exists in BOTH wizard.js (lines 8-87) and dashboard.js (lines similar location). When adding new timezone entries, update BOTH files to maintain consistency. This is necessary because they are separate gzipped files served from LittleFS.

Future optimization: Consider extracting TZ_MAP to a common.js file, but this requires gzip budget analysis (adds ~2KB uncompressed, ~500B gzipped).
```

### 3. Add Test Case for Settings Import with Timezone

**Value:** Ensures the POSIX→IANA reverse-lookup is tested

**Suggestion:**
Add to Task 7 (E2E test page objects):
```
- [ ] 7.5: Add E2E test for settings import with timezone
  - [ ] 7.5.1: Import settings file with `"timezone": "EST5EDT,M3.2.0,M11.1.0"`
  - [ ] 7.5.2: Verify Step 3 timezone dropdown shows "America/New_York" (first alphabetical match)
  - [ ] 7.5.3: Navigate to Review step and verify Timezone row shows "America/New_York"
```

---

## 🤖 LLM Optimization Improvements

Token efficiency and clarity improvements for better dev agent processing.

### 1. Remove Exact Line Numbers from Task Descriptions

**Issue:** Excessive detail that wastes tokens
**Token Impact:** -15%

**Current:**
```
- [ ] Task 2: Wire timezone into wizard.js state and WIZARD_KEYS (AC: #1, #4, #6)
  - [ ] 2.1: Add `timezone: ''` to the `state` object (line ~108, after `radius_km`)
  - [ ] 2.2: Add `'timezone'` to the `WIZARD_KEYS` array (line ~130, after `'radius_km'`)
```

**Optimized:**
```
- [ ] Task 2: Wire timezone into wizard.js state and WIZARD_KEYS
  - [ ] 2.1: Add `timezone: ''` to state object after radius_km
  - [ ] 2.2: Add `'timezone'` to WIZARD_KEYS array after 'radius_km'
  - [ ] 2.3: Remove `'timezone'` from KNOWN_EXTRA_KEYS (critical: prevents import override bug)
```

**Rationale:** Line numbers go stale quickly and the context makes it clear where to add. Removed "after `radius_km`" for 2.2 since it's sequential from 2.1. Made 2.3 warning more concise.

### 2. Consolidate Redundant "DO NOT Reinvent" Warnings

**Issue:** Verbose warnings repeated multiple times
**Token Impact:** -8%

**Current:**
```
### Existing Code to Reuse (DO NOT Reinvent)

**`TZ_MAP` (wizard.js lines 7-86):** Already fully implemented with 60+ IANA-to-POSIX entries. DO NOT create a new mapping table. Reference this object directly.

**`getTimezoneConfig()` (wizard.js lines 92-99):** Already implemented — detects browser IANA timezone via `Intl.DateTimeFormat().resolvedOptions().timeZone`, looks up POSIX string in `TZ_MAP`, returns `{ iana, posix }`. Currently dead code relative to the wizard UI. This story wires it in. DO NOT rewrite this function.
```

**Optimized:**
```
### Reuse Existing Code (Do Not Reinvent)

- **`TZ_MAP`** (wizard.js lines 7-86): 60+ IANA-to-POSIX entries already implemented. Reference directly.
- **`getTimezoneConfig()`** (wizard.js lines 92-99): Detects browser IANA via Intl API, returns `{ iana, posix }`. Currently unused — this story wires it in.
- **Dashboard timezone dropdown** (dashboard.js lines 1042-1076): Reference implementation for populating `<select>` from TZ_MAP using `Object.keys(TZ_MAP).sort()`.
```

**Rationale:** Bulleted format is more scannable. Removed repetitive "DO NOT create a new..." warnings - the heading makes this clear. Consolidated dashboard reference for conciseness.

---

## 🏆 Competition Results

### Quality Metrics

| Metric | Score |
|--------|-------|
| Requirements Coverage | 90% |
| Architecture Alignment | 85% |
| Previous Story Integration | 95% |
| LLM Optimization Score | 75% |
| **Overall Quality Score** | **86%** |

### Disaster Prevention Assessment

- **Code reinvention risks:** LOW - Story correctly identifies existing TZ_MAP and getTimezoneConfig() to reuse
- **Technical specification disasters:** MEDIUM - Missing hydrateDefaults() update and unclear settings import implementation location
- **File structure disasters:** LOW - Correct file locations specified, following existing patterns
- **Regression disasters:** LOW - No breaking changes identified, correctly moves timezone from extras to wizard key
- **Implementation disasters:** MEDIUM - AC #1 timing ambiguity could cause bugs; missing validation could allow empty timezone saves

### Competition Outcome

🏆 **Validator identified 10 improvements** (3 critical, 4 enhancements, 3 optimizations, 2 LLM optimizations) that enhance the story context.

The original story is well-structured and correctly identifies the main architecture pattern, but has critical gaps in:
1. Device state restoration (hydrateDefaults integration)
2. Settings import reverse-lookup implementation details
3. Auto-detect timing clarity
4. DOM reference pattern compliance

These gaps would likely cause the developer to make mistakes or miss important integration points, leading to bugs in production (timezone not persisting on page refresh, import not working correctly).

---

**Report Generated:** 2026-04-13
**Validation Engine:** BMAD Method Quality Competition v1.0

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-13</var>
<var name="description">Master synthesizes validator findings and applies changes to story file</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-3</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/validate-story-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/validate-story-synthesis/instructions.xml</var>
<var name="name">validate-story-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="session_id">a9859635-c246-4b46-86e9-c0ecd64d8280</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="1dffb869">embedded in prompt, file id: 1dffb869</var>
<var name="story_id">fn-3.2</var>
<var name="story_key">fn-3-2-wizard-timezone-auto-detect</var>
<var name="story_num">2</var>
<var name="story_title">2-wizard-timezone-auto-detect</var>
<var name="template">False</var>
<var name="timestamp">20260413_1826</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="validator_count">2</var>
</variables>
<instructions><workflow>
  <critical>Communicate all responses in English and generate all documents in English</critical>

  <critical>You are the MASTER SYNTHESIS agent. Your role is to evaluate validator findings
    and produce a definitive synthesis with applied fixes.</critical>
  <critical>You have WRITE PERMISSION to modify the story file being validated.</critical>
  <critical>All context (project_context.md, story file, anonymized validations) is EMBEDDED below - do NOT attempt to read files.</critical>
  <critical>Apply changes to story file directly using atomic write pattern (temp file + rename).</critical>

  <step n="1" goal="Analyze validator findings">
    <action>Read all anonymized validator outputs (Validator A, B, C, D, etc.)</action>
    <action>For each issue raised:
      - Cross-reference with story content and project_context.md
      - Determine if issue is valid or false positive
      - Note validator consensus (if 3+ validators agree, high confidence issue)
    </action>
    <action>Issues with low validator agreement (1-2 validators) require extra scrutiny</action>
  </step>

  <step n="1.5" goal="Review Deep Verify technical findings" conditional="[Deep Verify Findings] section present">
    <critical>Deep Verify provides automated technical analysis that complements validator reviews.
      DV findings focus on: patterns, boundary cases, assumptions, temporal issues, security, and worst-case scenarios.</critical>

    <action>Review each DV finding:
      - CRITICAL findings: Must be addressed - these indicate serious technical issues
      - ERROR findings: Should be addressed unless clearly false positive
      - WARNING findings: Consider addressing, document if dismissed
    </action>

    <action>Cross-reference DV findings with validator findings:
      - If validators AND DV flag same issue: High confidence, prioritize fix
      - If only DV flags issue: Verify technically valid, may be edge case validators missed
      - If only validators flag issue: Normal processing per step 1
    </action>

    <action>For each DV finding, determine:
      - Is this a genuine issue in the story specification?
      - Does the story need to address this edge case/scenario?
      - Is this already covered but DV missed it? (false positive)
    </action>

    <action>DV findings with patterns (CC-*, SEC-*, DB-*, DT-*, GEN-*) reference known antipatterns.
      Treat pattern-matched findings as higher confidence.</action>
  </step>

  <step n="2" goal="Verify and prioritize issues">
    <action>For verified issues, assign severity:
      - Critical: Blocks implementation or causes major problems
      - High: Significant gaps or ambiguities that need attention
      - Medium: Improvements that would help quality
      - Low: Nice-to-have suggestions
    </action>
    <action>Document false positives with clear reasoning for dismissal:
      - Why the validator was wrong
      - What evidence contradicts the finding
      - Reference specific story content or project_context.md
    </action>
  </step>

  <step n="3" goal="Apply changes to story file">
    <action>For each verified issue (starting with Critical, then High), apply fix directly to story file</action>
    <action>Changes should be natural improvements:
      - DO NOT add review metadata or synthesis comments to story
      - DO NOT reference the synthesis or validation process
      - Preserve story structure, formatting, and style
      - Make changes look like they were always there
    </action>
    <action>For each change, log in synthesis output:
      - File path modified
      - Section/line reference (e.g., "AC4", "Task 2.3")
      - Brief description of change
      - Before snippet (2-3 lines context)
      - After snippet (2-3 lines context)
    </action>
    <action>Use atomic write pattern for story modifications to prevent corruption</action>
  </step>

  <step n="4" goal="Generate synthesis report">
    <critical>Your synthesis report MUST be wrapped in HTML comment markers for extraction:</critical>
    <action>Produce structured output in this exact format (including the markers):</action>
    <output-format>
&lt;!-- VALIDATION_SYNTHESIS_START --&gt;
## Synthesis Summary
[Brief overview: X issues verified, Y false positives dismissed, Z changes applied to story file]

## Validations Quality
[For each validator: name, score, comments]
[Summary of validation quality - 1-10 scale]

## Issues Verified (by severity)

### Critical
[Issues that block implementation - list with evidence and fixes applied]
[Format: "- **Issue**: Description | **Source**: Validator(s) | **Fix**: What was changed"]

### High
[Significant gaps requiring attention]

### Medium
[Quality improvements]

### Low
[Nice-to-have suggestions - may be deferred]

## Issues Dismissed
[False positives with reasoning for each dismissal]
[Format: "- **Claimed Issue**: Description | **Raised by**: Validator(s) | **Dismissal Reason**: Why this is incorrect"]

## Deep Verify Integration
[If DV findings were present, document how they were handled]

### DV Findings Addressed
[List DV findings that resulted in story changes]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Action**: {What was changed}"]

### DV Findings Dismissed
[List DV findings determined to be false positives or not applicable]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Reason**: {Why dismissed}"]

### DV-Validator Overlap
[Note any findings flagged by both DV and validators - these are high confidence]
[If no DV findings: "Deep Verify did not produce findings for this story."]

## Changes Applied
[Complete list of modifications made to story file]
[Format for each change:
  **Location**: [File path] - [Section/line]
  **Change**: [Brief description]
  **Before**:
  ```
  [2-3 lines of original content]
  ```
  **After**:
  ```
  [2-3 lines of updated content]
  ```
]
&lt;!-- VALIDATION_SYNTHESIS_END --&gt;
    </output-format>

  </step>

  <step n="5" goal="Final verification">
    <action>Verify all Critical and High issues have been addressed</action>
    <action>Confirm story file changes are coherent and preserve structure</action>
    <action>Ensure synthesis report is complete with all sections populated</action>
  </step>
</workflow></instructions>
<output-template></output-template>
</compiled-workflow>