<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-2 -->
<!-- Story: 3 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260413T191615Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story fn-2.3

You are synthesizing 3 independent code review findings.

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


]]></file>
<file id="4edee726" path="_bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md" label="DOCUMENTATION"><![CDATA[

# Story fn-2.3: Dashboard Night Mode Card

Status: ready-for-review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As the **device owner**,
I want a Night Mode card on the dashboard that lets me enable/disable the brightness scheduler, set the dim window times, choose a dim brightness level, and select my timezone,
so that I can configure automatic nighttime dimming without needing serial access or manual API calls.

## Acceptance Criteria

1. **Night Mode Card Renders in Dashboard**
   - Given: the dashboard page loads
   - When: `dashboard.html` is rendered
   - Then: a "Night Mode" card appears between the Firmware and System cards (card position #10, before the danger-styled System card)
   - And: the card has `id="nightmode-card"` and uses the standard `<section class="card">` pattern

2. **Enable/Disable Toggle**
   - Given: the Night Mode card is visible
   - When: the user toggles the "Enable Schedule" checkbox
   - Then: the `sched_enabled` config key will be included in the next apply payload as `0` or `1`
   - And: when unchecked, the time inputs, brightness slider, and timezone dropdown are visually disabled (grayed out)
   - And: the `nightmode` dirty section is marked via `markSectionDirty('nightmode')`

3. **Dim Window Time Inputs**
   - Given: the schedule is enabled
   - When: the user sets "Dim Start" and "Dim End" time inputs
   - Then: the inputs display as `HH:MM` format (using `<input type="time">`)
   - And: values are converted to/from minutes-since-midnight (uint16 0-1439) for API transport
   - And: the `nightmode` dirty section is marked on change
   - And: default values display as 23:00 (start) and 07:00 (end) per ConfigManager defaults (`sched_dim_start=1380`, `sched_dim_end=420`)

4. **Dim Brightness Slider**
   - Given: the schedule is enabled
   - When: the user adjusts the "Dim Brightness" range slider
   - Then: the slider shows values 0-255 with a live numeric readout (matching the existing brightness slider pattern)
   - And: `sched_dim_brt` is included in the apply payload
   - And: default value is 10 per ConfigManager defaults

5. **Timezone Selector Dropdown**
   - Given: the Night Mode card is visible
   - When: the card initializes
   - Then: a `<select>` dropdown is populated from the existing `TZ_MAP` object (dashboard.js lines 8-87) with IANA timezone names as option text
   - And: the dropdown auto-selects the user's browser-detected timezone using `getTimezoneConfig()` (dashboard.js lines 94-103) if the current saved timezone is `UTC0` (default)
   - And: when the user selects a timezone, the corresponding POSIX string is sent as the `timezone` config key
   - And: the `nightmode` dirty section is marked on change

6. **NTP Sync Status Indicator**
   - Given: the Night Mode card is visible
   - When: status is polled from `GET /api/status`
   - Then: an NTP sync indicator shows "NTP Synced" (green) or "NTP Not Synced" (amber/warning) based on the `ntp_synced` field
   - And: if NTP is not synced, a brief note explains the schedule will not run until NTP syncs

7. **Schedule Dimming Status Indicator**
   - Given: the Night Mode card is visible and NTP is synced
   - When: status is polled from `GET /api/status`
   - Then: if `schedule_dimming` is `true`, a status line shows "Currently dimming" with visual emphasis
   - And: if `schedule_active` is `true` but `schedule_dimming` is `false`, status shows "Schedule active (not in dim window)"
   - And: if `schedule_active` is `false`, status shows "Schedule inactive"

8. **Integration with Unified Apply Bar**
   - Given: the user has modified any Night Mode setting
   - When: the user clicks the unified "Apply" button in the sticky apply bar
   - Then: all dirty Night Mode fields are collected by `collectPayload()` and sent via `POST /api/settings`
   - And: no reboot is required (all 5 schedule keys are hot-reload)
   - And: after successful apply, the `nightmode` dirty state is cleared

9. **Settings Load on Page Init**
   - Given: the dashboard page loads
   - When: `loadSettings()` fetches `GET /api/settings`
   - Then: all Night Mode fields are populated from the response: `sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt`, `timezone`
   - And: the timezone dropdown selects the matching IANA entry by reverse-looking up the POSIX string in `TZ_MAP`

10. **Conditional Field Disabling**
    - Given: `sched_enabled` is `0` (unchecked)
    - When: the card renders or the toggle changes
    - Then: time inputs, brightness slider, and timezone dropdown have the `disabled` attribute set
    - And: they appear with reduced opacity (CSS `opacity: 0.5` or similar)
    - And: the NTP/schedule status indicators remain visible regardless of toggle state

## Tasks / Subtasks

- [x] **Task 1: Add Night Mode card HTML to dashboard.html** (AC: #1, #2, #3, #4, #5, #6, #7)
  - [x] 1.1 Insert a new `<section class="card" id="nightmode-card">` block after the Firmware card (`</section>` at ~line 260) and before the System card (`<section class="card card-danger" id="system-card">` at line 261):
    ```html
    <!-- Night Mode Card (Story fn-2.3) -->
    <section class="card" id="nightmode-card">
      <h2>Night Mode</h2>

      <div class="nm-status" id="nm-status">
        <span class="nm-ntp-badge" id="nm-ntp-badge">NTP: --</span>
        <span class="nm-sched-status" id="nm-sched-status"></span>
      </div>

      <label class="nm-toggle-label">
        <input type="checkbox" id="nm-enabled">
        <span>Enable Schedule</span>
      </label>

      <div class="nm-fields" id="nm-fields">
        <label for="nm-dim-start">Dim Start</label>
        <input type="time" id="nm-dim-start" value="23:00">

        <label for="nm-dim-end">Dim End</label>
        <input type="time" id="nm-dim-end" value="07:00">

        <label for="nm-dim-brt">Dim Brightness</label>
        <div class="range-row">
          <input type="range" id="nm-dim-brt" min="0" max="255" value="10">
          <span class="range-val" id="nm-dim-brt-val">10</span>
        </div>

        <label for="nm-timezone">Timezone</label>
        <select id="nm-timezone" class="cal-select"></select>
      </div>
    </section>
    ```
  - [x] 1.2 Verify the card appears between Firmware and System cards in the rendered page.

- [x] **Task 2: Add Night Mode section to dirty tracking and payload collection in dashboard.js** (AC: #8)
  - [x] 2.1 Add `nightmode: false` to the `dirtySections` object (line 211):
    ```js
    var dirtySections = { display: false, timing: false, network: false, hardware: false, nightmode: false };
    ```
  - [x] 2.2 Add `dirtySections.nightmode = false;` to `clearDirtyState()` (after line 223, before the `applyBar.style.display = 'none'`):
    ```js
    dirtySections.nightmode = false;
    ```
  - [x] 2.3 Add a `nightmode` block to `collectPayload()` (after the `hardware` block ending at line 274, before `return payload;` at line 275):
    ```js
    if (dirtySections.nightmode) {
      payload.sched_enabled = document.getElementById('nm-enabled').checked ? 1 : 0;
      payload.sched_dim_start = timeToMinutes(document.getElementById('nm-dim-start').value);
      payload.sched_dim_end = timeToMinutes(document.getElementById('nm-dim-end').value);
      payload.sched_dim_brt = parseInt(document.getElementById('nm-dim-brt').value, 10);
      var sel = document.getElementById('nm-timezone');
      if (sel.value) {
        payload.timezone = TZ_MAP[sel.value] || 'UTC0';
      }
    }
    ```

- [x] **Task 3: Add time conversion helpers in dashboard.js** (AC: #3, #9)
  - [x] 3.1 Add helper functions near the top of the DOMContentLoaded handler (after the `getTimezoneConfig()` usage area, before `loadSettings()`):
    ```js
    // --- Night Mode time helpers (Story fn-2.3) ---
    // Convert "HH:MM" string to minutes-since-midnight (uint16)
    function timeToMinutes(timeStr) {
      var parts = timeStr.split(':');
      return parseInt(parts[0], 10) * 60 + parseInt(parts[1], 10);
    }

    // Convert minutes-since-midnight (uint16) to "HH:MM" string
    function minutesToTime(mins) {
      var h = Math.floor(mins / 60);
      var m = mins % 60;
      return (h < 10 ? '0' : '') + h + ':' + (m < 10 ? '0' : '') + m;
    }

    // Reverse lookup: find IANA key in TZ_MAP for a given POSIX string
    function posixToIana(posix) {
      var keys = Object.keys(TZ_MAP);
      for (var i = 0; i < keys.length; i++) {
        if (TZ_MAP[keys[i]] === posix) return keys[i];
      }
      return 'UTC';
    }
    ```

- [x] **Task 4: Populate timezone dropdown and wire up Night Mode event handlers in dashboard.js** (AC: #2, #5, #10)
  - [x] 4.1 Add Night Mode initialization code after the existing event handler section (after the hardware input handlers, before `loadSettings()` call):
    ```js
    // --- Night Mode card (Story fn-2.3) ---
    var nmEnabled = document.getElementById('nm-enabled');
    var nmDimStart = document.getElementById('nm-dim-start');
    var nmDimEnd = document.getElementById('nm-dim-end');
    var nmDimBrt = document.getElementById('nm-dim-brt');
    var nmDimBrtVal = document.getElementById('nm-dim-brt-val');
    var nmTimezone = document.getElementById('nm-timezone');
    var nmFields = document.getElementById('nm-fields');

    // Populate timezone dropdown from TZ_MAP
    (function() {
      var keys = Object.keys(TZ_MAP).sort();
      for (var i = 0; i < keys.length; i++) {
        var opt = document.createElement('option');
        opt.value = keys[i];
        opt.textContent = keys[i].replace(/_/g, ' ');
        nmTimezone.appendChild(opt);
      }
    })();

    // Toggle field enabled/disabled state
    function updateNmFieldState() {
      var on = nmEnabled.checked;
      nmDimStart.disabled = !on;
      nmDimEnd.disabled = !on;
      nmDimBrt.disabled = !on;
      nmTimezone.disabled = !on;
      nmFields.classList.toggle('nm-fields-disabled', !on);
    }

    nmEnabled.addEventListener('change', function() {
      updateNmFieldState();
      markSectionDirty('nightmode');
    });

    nmDimStart.addEventListener('change', function() { markSectionDirty('nightmode'); });
    nmDimEnd.addEventListener('change', function() { markSectionDirty('nightmode'); });
    nmDimBrt.addEventListener('input', function() {
      nmDimBrtVal.textContent = nmDimBrt.value;
    });
    nmDimBrt.addEventListener('change', function() { markSectionDirty('nightmode'); });
    nmTimezone.addEventListener('change', function() { markSectionDirty('nightmode'); });

    updateNmFieldState();
    ```

- [x] **Task 5: Load Night Mode settings from API response in dashboard.js** (AC: #9, #10)
  - [ ] 5.1 Add a Night Mode section to `loadSettings()` after the Hardware block (after `updateHwResolution();` at line 360, before `syncCalibrationFromSettings();`):
    ```js
    // Night Mode (Story fn-2.3)
    if (d.sched_enabled !== undefined) {
      nmEnabled.checked = (d.sched_enabled === 1 || d.sched_enabled === '1');
    }
    if (d.sched_dim_start !== undefined) {
      nmDimStart.value = minutesToTime(parseInt(d.sched_dim_start, 10));
    }
    if (d.sched_dim_end !== undefined) {
      nmDimEnd.value = minutesToTime(parseInt(d.sched_dim_end, 10));
    }
    if (d.sched_dim_brt !== undefined) {
      nmDimBrt.value = d.sched_dim_brt;
      nmDimBrtVal.textContent = d.sched_dim_brt;
    }
    if (d.timezone !== undefined) {
      var iana = posixToIana(d.timezone);
      nmTimezone.value = iana;
      // If device is still on default UTC0 and browser has a different zone, auto-suggest
      if (d.timezone === 'UTC0') {
        var detected = getTimezoneConfig();
        if (detected.iana !== 'UTC' && TZ_MAP[detected.iana]) {
          nmTimezone.value = detected.iana;
          markSectionDirty('nightmode');
        }
      }
    }
    updateNmFieldState();
    ```

- [x] **Task 6: Add NTP and schedule status polling in dashboard.js** (AC: #6, #7)
  - [ ] 6.1 Add a status polling function and call it on load and at interval:
    ```js
    // --- Night Mode status polling (Story fn-2.3) ---
    var nmNtpBadge = document.getElementById('nm-ntp-badge');
    var nmSchedStatus = document.getElementById('nm-sched-status');

    function updateNmStatus() {
      FW.get('/api/status').then(function(res) {
        if (!res.body) return;
        var s = res.body;

        // NTP badge
        if (s.ntp_synced) {
          nmNtpBadge.textContent = 'NTP Synced';
          nmNtpBadge.className = 'nm-ntp-badge nm-ntp-ok';
        } else {
          nmNtpBadge.textContent = 'NTP Not Synced';
          nmNtpBadge.className = 'nm-ntp-badge nm-ntp-warn';
        }

        // Schedule status (NTP must be synced for schedule to run)
        if (!s.ntp_synced) {
          nmSchedStatus.textContent = 'Schedule paused — waiting for NTP sync';
          nmSchedStatus.className = 'nm-sched-status nm-inactive';
        } else if (s.schedule_dimming) {
          nmSchedStatus.textContent = 'Currently dimming';
          nmSchedStatus.className = 'nm-sched-status nm-dimming';
        } else if (s.schedule_active) {
          nmSchedStatus.textContent = 'Schedule active (not in dim window)';
          nmSchedStatus.className = 'nm-sched-status nm-active';
        } else {
          nmSchedStatus.textContent = 'Schedule inactive';
          nmSchedStatus.className = 'nm-sched-status nm-inactive';
        }
      }).catch(function() {
        nmNtpBadge.textContent = 'NTP: --';
        nmNtpBadge.className = 'nm-ntp-badge';
        nmSchedStatus.textContent = '';
      });
    }

    // Poll status on load + every 10 seconds
    updateNmStatus();
    setInterval(updateNmStatus, 10000);
    ```

- [x] **Task 7: Add Night Mode card styles to style.css** (AC: #1, #2, #6, #7, #10)
  - [ ] 7.1 Add Night Mode styles after the existing card styles (after the mode-card styles at ~line 527):
    ```css
    /* Night Mode card (Story fn-2.3) */
    .nm-status{display:flex;gap:8px;align-items:center;margin-bottom:12px;flex-wrap:wrap}
    .nm-ntp-badge{
      font-size:0.75rem;font-weight:600;padding:2px 8px;
      border-radius:var(--radius);background:var(--border);color:var(--text-muted);
    }
    .nm-ntp-ok{background:var(--success);color:#0d1117}
    .nm-ntp-warn{background:#d29922;color:#0d1117}
    .nm-sched-status{font-size:0.8125rem;color:var(--text-muted)}
    .nm-dimming{color:var(--primary);font-weight:600}
    .nm-active{color:var(--success)}
    .nm-inactive{color:var(--text-muted)}

    .nm-toggle-label{
      display:flex;align-items:center;gap:8px;
      font-size:0.875rem;color:var(--text);
      cursor:pointer;margin-bottom:12px;
    }
    .nm-toggle-label input[type="checkbox"]{
      width:18px;height:18px;accent-color:var(--primary);cursor:pointer;
    }

    .nm-fields{transition:opacity 0.2s}
    .nm-fields-disabled{opacity:0.5;pointer-events:none}
    .nm-fields input[type="time"]{
      width:100%;padding:8px;font-size:1rem;
      background:var(--surface);color:var(--text);
      border:1px solid var(--border);border-radius:var(--radius);
      outline:none;
    }
    .nm-fields input[type="time"]:focus{border-color:var(--primary)}
    .nm-fields input[type="time"]:disabled{opacity:0.5;cursor:default}
    .nm-fields select:disabled{opacity:0.5;cursor:default}
    .nm-fields input[type="range"]:disabled{opacity:0.5;cursor:default}
    ```

- [x] **Task 8: Rebuild gzipped web assets** (Post-implementation)
  - [ ] 8.1 After all HTML/JS/CSS changes are complete, rebuild gzipped assets:
    ```bash
    cd firmware
    gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
    gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz
    gzip -9 -c data-src/style.css > data/style.css.gz
    ```
  - [ ] 8.2 Upload filesystem to device:
    ```bash
    ~/.platformio/penv/bin/pio run -t uploadfs
    ```

## Dev Notes

### Architecture Constraints (MUST FOLLOW)

- **Architecture Decision F4:** IANA-to-POSIX timezone mapping is done browser-side. The `TZ_MAP` object (84 entries) and `getTimezoneConfig()` already exist in dashboard.js lines 8-103. Reuse them — do NOT duplicate or fetch timezone data from the device.
- **Architecture Decision F6:** All 5 schedule keys (`timezone`, `sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt`) are hot-reload. No reboot required. Use `applySettings()` (line 373), NOT `applyWithReboot()`.
- **Enforcement Rule #15:** Schedule times stored as `uint16_t` minutes since midnight (0-1439). The UI must convert `HH:MM` ↔ minutes-since-midnight for display and API transport.
- **No new files:** All modifications to existing files only (`dashboard.html`, `dashboard.js`, `style.css`). No new JS libraries, no external dependencies.

### Minutes-Since-Midnight Conversion

```
HH:MM → minutes:  hours * 60 + minutes
minutes → HH:MM:  Math.floor(mins/60) + ':' + (mins%60)

Examples:
  23:00 → 1380    (default sched_dim_start)
  07:00 → 420     (default sched_dim_end)
  00:00 → 0
  23:59 → 1439
```

### Timezone Dropdown: Reverse Lookup Pattern

The API stores the POSIX string (e.g., `"EST5EDT,M3.2.0,M11.1.0"`). The dropdown displays IANA names (e.g., `"America/New_York"`). On load, reverse-lookup the stored POSIX string to find the matching IANA key:

```js
function posixToIana(posix) {
  var keys = Object.keys(TZ_MAP);
  for (var i = 0; i < keys.length; i++) {
    if (TZ_MAP[keys[i]] === posix) return keys[i];
  }
  return 'UTC';
}
```

Note: Multiple IANA zones may map to the same POSIX string (e.g., `America/New_York` and `America/Toronto` both map to `EST5EDT,...`). The first match is fine — the POSIX string sent to the device is what matters, not which IANA label is displayed.

### Browser Auto-Detect Behavior

When the saved timezone is `UTC0` (the factory default), auto-suggest the browser's detected timezone via `getTimezoneConfig()`. This triggers `markSectionDirty('nightmode')` so the user sees the apply bar and can confirm. This is a one-time convenience — once the user applies a timezone, subsequent loads will show the saved timezone without auto-suggesting.

### Existing Dashboard Patterns to Follow

**Dirty tracking (dashboard.js line 211-224):**
```js
var dirtySections = { display: false, timing: false, network: false, hardware: false };
// ADD: nightmode: false

function markSectionDirty(section) {
  dirtySections[section] = true;
  applyBar.style.display = '';
}
```

**Payload collection (dashboard.js line 226-275):**
Each dirty section adds its keys to the payload object. Night Mode adds: `sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt`, `timezone`.

**Range slider pattern (dashboard.js lines 458-463):**
- `input` event → update display label (live feedback)
- `change` event → mark section dirty (triggers apply bar)

**Card HTML pattern (dashboard.html):**
```html
<section class="card" id="card-id">
  <h2>Title</h2>
  <!-- fields -->
</section>
```

**Select dropdown styling:** Use `class="cal-select"` (style.css lines 380-390) for the timezone `<select>`.

### Status Polling

The `/api/status` endpoint returns:
```json
{
  "ntp_synced": true,
  "schedule_active": true,
  "schedule_dimming": false
}
```

Poll every 10 seconds to keep the NTP and schedule status indicators current. This reuses the existing `FW.get()` helper (common.js line 25).

### ConfigManager Defaults (for reference)

| Key | Default | Type |
|-----|---------|------|
| `timezone` | `"UTC0"` | POSIX string (max 40 chars) |
| `sched_enabled` | `0` | uint8 (0 or 1) |
| `sched_dim_start` | `1380` (23:00) | uint16 (0-1439) |
| `sched_dim_end` | `420` (07:00) | uint16 (0-1439) |
| `sched_dim_brt` | `10` | uint8 (0-255) |

### Card Positioning

Insert the Night Mode card between Firmware (`id="firmware-card"`, line 234) and System (`id="system-card"`, line 261). This places it near the bottom of the dashboard with other "system behavior" cards, keeping the primary display/timing/network cards at the top.

### Source Files to Modify

| File | Change | Lines |
|------|--------|-------|
| `firmware/data-src/dashboard.html` | Add Night Mode card HTML | Insert between ~line 260 and line 261 |
| `firmware/data-src/dashboard.js` | Add `nightmode` to dirty tracking, `collectPayload()`, time helpers, timezone dropdown init, event handlers, `loadSettings()` Night Mode block, status polling | Lines 211, 218-224, 274, 360, + new sections |
| `firmware/data-src/style.css` | Add Night Mode card styles | Append after ~line 527 |
| `firmware/data/dashboard.html.gz` | Rebuild gzip | `gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz` |
| `firmware/data/dashboard.js.gz` | Rebuild gzip | `gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz` |
| `firmware/data/style.css.gz` | Rebuild gzip | `gzip -9 -c data-src/style.css > data/style.css.gz` |

### What This Story Does NOT Include

- Backend scheduler logic (fn-2.2 — already complete)
- NTP sync infrastructure (fn-2.1 — already complete)
- ConfigManager schedule key validation (fn-1.2 — already complete)
- Gradual brightness fade transitions (not in scope)
- Sunrise/sunset-based scheduling (not in scope)
- Mobile-specific responsive breakpoints (the existing dashboard CSS handles mobile via percentage widths)

### Anti-Pattern Avoidance (from fn-2.1/fn-2.2 reviews)

- **DO** use the existing `TZ_MAP` and `getTimezoneConfig()` — do not create a separate timezone data source
- **DO** use `markSectionDirty('nightmode')` — do not create a separate apply button for Night Mode
- **DO** convert time inputs to minutes-since-midnight before sending to API — do not send `HH:MM` strings
- **DO NOT** add a separate "Save" button on the Night Mode card — use the unified apply bar pattern
- **DO NOT** poll `/api/status` more frequently than every 10 seconds — avoid unnecessary network load on the ESP32
- **DO NOT** add `nightmode` to the reboot-required flow — all schedule keys are hot-reload

### Dependencies

| Dependency | Status | Notes |
|------------|--------|-------|
| fn-2.1 (NTP Time Sync) | DONE | Provides `ntp_synced` in `/api/status`, timezone hot-reload in ConfigManager |
| fn-2.2 (Night Mode Scheduler) | DONE | Provides `schedule_active`, `schedule_dimming` in `/api/status`; all scheduler backend logic |
| fn-1.2 (ConfigManager Expansion) | DONE | All 5 schedule NVS keys with validation |
| TZ_MAP in dashboard.js | DONE | 84 IANA→POSIX timezone entries (lines 8-87) |
| `getTimezoneConfig()` | DONE | Browser auto-detect helper (lines 94-103) |
| Dirty tracking / apply bar | DONE | Existing pattern (lines 208-275) |

### References

- [Source: _bmad-output/planning-artifacts/architecture.md#F4] — IANA-to-POSIX timezone mapping, browser-side
- [Source: _bmad-output/planning-artifacts/architecture.md#F5] — Night Mode scheduler runs Core 1 main loop
- [Source: _bmad-output/planning-artifacts/architecture.md#F6] — All schedule keys hot-reload, no reboot
- [Source: firmware/data-src/dashboard.html:19-44] — Display card HTML pattern
- [Source: firmware/data-src/dashboard.html:234-260] — Firmware card (insert Night Mode after this)
- [Source: firmware/data-src/dashboard.html:261-262] — System card (insert Night Mode before this)
- [Source: firmware/data-src/dashboard.js:8-87] — TZ_MAP object
- [Source: firmware/data-src/dashboard.js:94-103] — getTimezoneConfig() function
- [Source: firmware/data-src/dashboard.js:208-275] — Dirty tracking, collectPayload()
- [Source: firmware/data-src/dashboard.js:294-370] — loadSettings() function
- [Source: firmware/data-src/dashboard.js:373-420] — applySettings() / applyWithReboot()
- [Source: firmware/data-src/dashboard.js:458-482] — Input event handler patterns
- [Source: firmware/data-src/style.css:162-188] — Card base styling
- [Source: firmware/data-src/style.css:202-221] — Apply bar styling
- [Source: firmware/data-src/style.css:380-390] — Select dropdown styling
- [Source: firmware/data-src/common.js:25-41] — FW.get(), FW.post() helpers
- [Source: firmware/data-src/common.js:48-77] — FW.showToast() implementation
- [Source: firmware/adapters/WebPortal.cpp:785-791] — /api/status handler with ntp_synced, schedule_active, schedule_dimming
- [Source: firmware/core/ConfigManager.cpp:134-172] — Schedule key validation
- [Source: firmware/core/ConfigManager.cpp:247-262] — Schedule defaults
- [Source: _bmad-output/implementation-artifacts/stories/fn-2-2-night-mode-brightness-scheduler.md] — Predecessor story

## File List

| File | Action |
|------|--------|
| `firmware/data-src/dashboard.html` | Modified — add Night Mode card HTML |
| `firmware/data-src/dashboard.js` | Modified — dirty tracking, payload collection, time helpers, timezone dropdown, event handlers, loadSettings block, status polling |
| `firmware/data-src/style.css` | Modified — Night Mode card styles |
| `firmware/data/dashboard.html.gz` | Rebuilt — gzip of modified HTML |
| `firmware/data/dashboard.js.gz` | Rebuilt — gzip of modified JS |
| `firmware/data/style.css.gz` | Rebuilt — gzip of modified CSS |

## Change Log

| Date | Change |
|------|--------|
| 2026-04-13 | Story created — ready-for-dev |
| 2026-04-13 | Synthesis pass — fixed Task 6.1 schedule status block: NTP-not-synced explanatory note (AC #6), "not in dim window" text (AC #7); switched `updateNmFieldState` to CSS class toggle; added `.nm-fields-disabled` CSS rule |


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic fn-2 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `onNtpSync()` sets `g_ntpSynced=true` unconditionally — but `sntp_set_time_sync_notification_cb` can fire on non-COMPLETED events (e.g., `SNTP_SYNC_STATUS_RESET` during network changes). Without a status check, the flag could flip to `true` spuriously. | Added `sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED` early-return guard ✅ Applied |
| high | `configTzTime()` called on **every** config write in the `onChange` lambda — including unrelated writes (brightness, fetch_interval, text colors, etc.). This restarts SNTP and discards in-flight NTP requests on hot paths. | Added `static String s_lastAppliedTz` cache inside the lambda; `configTzTime()` only fires when the timezone value actually changes ✅ Applied |
| medium | `TZ_MAP` with ~75 entries is copy-pasted verbatim into both `dashboard.js` and `wizard.js`. Any DST rule correction requires dual edits and dual `gzip` regenerations. | Deferred to fn-2.3 (timezone dropdown UI) — moving to `common.js` must be validated against wizard page's script loading order; project "no new files" rule for this story blocks immediate action. Follow-up task created. |
| low | `schedule_active` in `/api/status` is computed as `sched_enabled && ntp_synced`, not "currently inside the dim window". The field name implies real-time window state but delivers config+sync state. | Deferred to fn-2.2 (Night Mode scheduler) where the actual in-window calculation will be implemented. Follow-up task created. |
| low | `getTimezoneConfig()` silently falls back to `"UTC0"` for IANA zones not in `TZ_MAP` (e.g., `Asia/Manila`). The caller gets no signal that the zone was unmapped. | Deferred to fn-2.3 — no UI element currently consumes `getTimezoneConfig()`. Follow-up task created. |
| dismissed | `extern bool isNtpSynced()` in `WebPortal.cpp` breaks hexagonal architecture | FALSE POSITIVE: FALSE POSITIVE. The file already declares `extern FlightStatsSnapshot getFlightStatsSnapshot()` at line 37 using the identical pattern. The story's Dev Notes explicitly prescribe this approach ("aligns with project rule: no new files"). This is the established project convention, not a new violation. |
| dismissed | Race condition between `tzset()` (from `configTzTime()`) and `localtime_r()` across Core 0/Core 1 | FALSE POSITIVE: Theoretical but low-impact. Hot-reload is a rare user-triggered action; the `tzset()` modification window is microseconds; the worst-case consequence is a single garbled time display frame during a settings write. Accepted design risk per story scope. A mutex solution would require significant infrastructure not justified by this failure mode. |
| dismissed | Missing POSIX TZ format validation in ConfigManager | FALSE POSITIVE: Partially valid concern, but implemented validation (non-empty string ≤40 chars) is sufficient for the story AC. A malformed TZ string passed to `configTzTime()` → `tzset()` causes silent UTC fallback — the device stays functional and the LWIP stack doesn't crash. Full POSIX TZ syntax validation is non-trivial on ESP32 newlib and is outside story scope. Accepted risk. |
| dismissed | `g_ntpSynced` never resets to `false` after initial sync (stale success on WiFi reconnect) | FALSE POSITIVE: Out-of-story-scope design concern. The story ACs define `ntpSynced=false` only as the pre-sync default (AC #3). LWIP auto-retries maintain clock accuracy. The flag is correctly "sticky" for the current story's contract. If cross-story NTP state invalidation is needed (e.g., for fn-2.2 scheduling), that story should define the semantics. |
| dismissed | Raw `Serial.println` violates logging contract (should use `LOG_I`) | FALSE POSITIVE: Acknowledged in the Dev Agent Record as a known technical constraint. `LOG_I` is a 2-argument macro (tag + string literal only); it cannot format dynamic runtime strings. The `#if LOG_LEVEL >= 2` guard follows the pre-existing pattern already in `WiFiManager.cpp:140`. Not a new violation. |
| dismissed | Unit tests are "lying" — `test_ntp_status_in_json_output` doesn't test the actual `/api/status` HTTP endpoint | FALSE POSITIVE: Valid quality observation but not a "lying test". The test name and inline comment accurately describe what it tests (`SystemStatus::toJson()` NTP fields). Full HTTP endpoint testing is not achievable in Unity/embedded unit tests without a host simulator. The story explicitly notes "unit tests cover callback logic only" (AC #1). Coverage gap is real but architectural, not deceptive. --- |

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| dismissed | `g_ntpSynced` never resets to `false` after WiFi reconnect where NTP fails (Validators B, C) | FALSE POSITIVE: Already adjudicated as out-of-scope in the Round 1 antipatterns doc. AC #3 defines `ntpSynced=false` only as the pre-first-sync default. The "sticky true" design is intentional for this story contract; WiFi-reconnect NTP invalidation is a concern for a future story defining that semantic. |
| dismissed | AC #2 is "only half-implemented" — `getLocalTime()` not called in callback (Validator B) | FALSE POSITIVE: The AC states `getLocalTime()` *returns correct time* — this is a functional postcondition, not a requirement for the callback to call it. `configTzTime()` + SNTP completion makes `getLocalTime()` correct by design. The callback's job is flag + status, which is implemented. |
| dismissed | `test_ntp_status_in_json_output` is a lying test for AC #6 (Validators B, C) | FALSE POSITIVE: Already adjudicated in Round 1 antipatterns doc. Full HTTP endpoint testing is not achievable in Unity/embedded unit tests without a host simulator. The test name accurately describes what it tests (`SystemStatus::toJson()` NTP fields); the story AC #1 explicitly scopes: "unit tests cover callback logic only." |
| dismissed | `schedule_active` field is semantically misleading (Validators B, C) | FALSE POSITIVE: Already tracked as a deferred follow-up from Round 1. Will be corrected in fn-2.2 (Night Mode scheduler) when real in-window logic lands. |
| dismissed | `extern bool isNtpSynced()` in `WebPortal.cpp` breaks hexagonal architecture (Validator C) | FALSE POSITIVE: Already dismissed as FALSE POSITIVE in Round 1 antipatterns doc. `WebPortal.cpp:37` already uses the identical `extern FlightStatsSnapshot getFlightStatsSnapshot()` pattern — this is the established project convention, not a new violation. |
| dismissed | `TZ_MAP` duplication and silent UTC fallback in `getTimezoneConfig()` (Validators B, C) | FALSE POSITIVE: Already tracked as deferred follow-ups from Round 1. Deferred to fn-2.3 when the timezone dropdown UI is built and `common.js` sharing can be validated against wizard page script-load order. --- |

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `test_ntp_status_in_json_output` calls `SystemStatus::toJson()` but production `WebPortal::_handleGetStatus()` calls `SystemStatus::toExtendedJson()`. The NTP subsystem lives at `data["subsystems"]["ntp"]` in production output (due to `toExtendedJson`'s nesting), not `data["ntp"]` as the test asserted. Test gave false confidence for a different JSON path than what the API actually returns. | Updated test to call `toExtendedJson(obj, dummy)` and assert `obj["subsystems"]["ntp"]`. |
| medium | `ConfigManager::loadFromNvs()` loads the `timezone` key from NVS without any validation, while `applyJson()` already guards against empty-string timezone via `if (tz.length() == 0) return false`. A device with corrupted NVS (manual write, older firmware) carrying `timezone=""` would pass the empty string to `configTzTime("")`, silently falling back to UTC with no logging. | Added `if (tz.length() > 0 && tz.length() <= 40)` guard in `loadFromNvs()`, mirroring the `applyJson` validation. Also added `test_nvs_invalid_timezone_ignored` regression test. |
| dismissed | `g_ntpSynced` never resets to `false` after WiFi reconnect where NTP fails (Validators A, B, C) | FALSE POSITIVE: Adjudicated as out-of-scope in Round 1 and Round 2 antipatterns docs. AC #3 defines `ntpSynced=false` only as the pre-first-sync default; "sticky true" is the intentional contract for this story. WiFi-reconnect NTP invalidation is a concern for a future story. |
| dismissed | Cross-core race: `configTzTime()` → `setenv()`/`tzset()` on Core 1 unsafe against future `localtime_r()` on Core 0 | FALSE POSITIVE: Currently Core 0 display task makes **no** `localtime_r()` calls — there is no ClockMode or scheduler implemented yet. No actual race condition exists in the current code. Future stories (fn-2.2) that add Core 0 time access must address this; flagging it now as a bug is premature. Already dismissed in Round 1. |
| dismissed | SNTP thrashing on rapid timezone changes — missing debounce | FALSE POSITIVE: The `s_lastAppliedTz` cache (added in Round 2) already prevents SNTP restart on every unrelated config write. Rapid alternating between two different timezone values is a pathological case (API script abuse) with the only consequence being delayed NTP sync convergence — the device remains functional. A 5s debounce would require non-blocking timer infrastructure not warranted for this failure mode. |
| dismissed | `schedule_active` field semantics mislead API consumers | FALSE POSITIVE: Already tracked as Round 1 deferred action item. Will be corrected in fn-2.2 (Night Mode scheduler) when actual in-window logic lands. No breaking change needed now. |
| dismissed | `TZ_MAP` duplicated across `dashboard.js` and `wizard.js` | FALSE POSITIVE: Already tracked as Round 1 deferred action item. Moving to `common.js` must be validated against wizard page script-load order; project "no new files" rule blocks action in this story. Deferred to fn-2.3. |
| dismissed | `getTimezoneConfig()` silent UTC fallback for unmapped IANA zones | FALSE POSITIVE: Already tracked as Round 1 deferred action item. No UI element currently consumes `getTimezoneConfig()`. fn-2.3 will add the dropdown and can surface the unsupported-zone warning at that point. |
| dismissed | Task 7 overclaims test coverage — tests never exercise `onNtpSync()` callback or the actual HTTP endpoint | FALSE POSITIVE: Adjudicated as false positive in Round 1 and Round 2. Embedded Unity tests on ESP32 cannot instantiate the HTTP server or simulate LWIP callbacks without a full device simulator. AC #1 explicitly scopes: "unit tests cover callback logic only." The test names accurately describe what they test. |
| dismissed | Story file list does not match current git diff | FALSE POSITIVE: Minor audit trail issue — the story's File List was incomplete (missing `ConfigManager.cpp` which was added in Round 2). Fixed in this round by updating the File List to include `firmware/core/ConfigManager.cpp`. No code change required. --- |

## Story fn-2-2 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `extern bool isScheduleDimming()` missing from WebPortal.cpp + `schedule_dimming` field absent from `/api/status` — AC #8 violation (Task 4 never applied) | Added `extern bool isScheduleDimming();` declaration after `isNtpSynced()`, added `data["schedule_dimming"] = isScheduleDimming();` in `_handleGetStatus()` |
| medium | `ConfigManager::loadFromNvs()` reads `sched_enabled`, `sched_dim_start`, `sched_dim_end` from NVS without range validation — corrupted NVS (value `sched_dim_start=65000`) feeds impossible minutes values into `tickNightScheduler()` every second | Added conditional validation matching `applyJson` rules: `sched_enabled` ≤ 1 else reset to 0; `sched_dim_start`/`sched_dim_end` ≤ 1439 else clamp to defaults (1380/420); `sched_dim_brt` left as-is (uint8_t range 0–255 is always valid per `applyJson`) |
| low | `static_assert(SCHED_CHECK_INTERVAL_MS <= 2000, ...)` specified in story Task 1.1 was omitted from the implementation | Added `static_assert` immediately after the constant declaration |
| dismissed | `schedule_active` in `/api/status` should be changed to `isScheduleDimming()` | FALSE POSITIVE: Story Task 4.2 and the fn-2.1 Review Follow-ups section both explicitly define `schedule_active` as "enabled + NTP synced = scheduler can run" and `schedule_dimming` as the real-time override state. Changing `schedule_active` would be a breaking API change outside story scope. The new `schedule_dimming` field resolves the semantic gap as designed. |
| dismissed | `ScheduleConfig` copies `String timezone` in `tickNightScheduler()` every second — ~2.6M heap allocations over 30 days | FALSE POSITIVE: The fix (a new `getScheduleDimmingParams()` function) contradicts the story's "No new files / no new functions beyond what's specified" constraint. The 1/sec String copy is a known ESP32 heap-fragmentation risk but is a future-story optimization concern, not a blocking bug for this story's NFR. |
| dismissed | `g_lastNtpSynced` (plain bool) race with `g_ntpSynced` (atomic) can produce duplicate/missing transition log lines | FALSE POSITIVE: `g_lastNtpSynced` is read and written exclusively by `tickNightScheduler()` on Core 1's Arduino loop task. The SNTP callback also runs on Core 1 but on the LWIP task — preemption is possible but the consequence is at most a duplicate or missing log line. The story's own comment documents "Core 1 only — no atomic needed". Accepted design risk for a non-functional log side-effect. |
| dismissed | Redundant `ConfigManager::getSchedule()` at lines 327 and 363 | FALSE POSITIVE: FALSE POSITIVE. Reading the actual code at lines 326–365, line 327 is `DisplayConfig newDisp = ConfigManager::getDisplay()` — not a `getSchedule()` call. There is exactly one `getSchedule()` call at line 363, inside the `if (g_schedDimming.load())` guard, so it only executes when actually dimming. |
| dismissed | Race between config read (line 356) and `g_schedDimming.load()` check (line 362) — one frame of wrong brightness if schedule transitions mid-handler | FALSE POSITIVE: The race window is microseconds; consequence is at most one display frame (~50ms) of wrong brightness, self-correcting on the next scheduler tick (1s) or config change. This is an accepted design risk for a transient display artifact — no functional correctness or data integrity issue. |
| dismissed | `g_ntpSynced` never resets to `false` after WiFi disconnect — scheduler silently continues post-disconnect | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.1 antipatterns (Rounds 1, 2, and 3). AC #4 defines `ntpSynced=false` only as the pre-first-sync default. "Sticky true" post-sync is the intentional contract. WiFi-reconnect NTP invalidation semantics are a future-story concern. |
| dismissed | AC #6 behavioral mismatch — manual brightness is re-overridden immediately (in the same config-change handler), not "on the next scheduler tick" as the story describes | FALSE POSITIVE: Intentional documented deviation. The story Dev Notes ("Brightness Override Mechanism") explicitly say: "because they trigger `g_configChanged`, the display task's handler runs immediately and re-applies the dim override (not 'on the next scheduler tick' — it's immediate, which is better than AC#6 describes)." The code comment at line 360 also documents this. Not a bug. |
| dismissed | Unit tests only test a local `isInDimWindow()` helper, never calling `tickNightScheduler()` or asserting `g_schedDimming` transitions | FALSE POSITIVE: Architectural limitation already established across fn-2.1 reviews. Unity tests on ESP32 cannot instantiate the main loop, LWIP scheduler, or FastLED display. The local `isInDimWindow()` helper is the production arithmetic extracted as a testable unit — this is correct TDD discipline. The story explicitly scopes "unit tests validate the exact same arithmetic the scheduler uses." Full integration testing requires hardware. |
| dismissed | Story file list drift — `WebPortal.cpp` listed but untouched; `ConfigManager.cpp` unlisted but changed | FALSE POSITIVE: Audit trail concern, not a product defect. Fixes applied in this synthesis bring `WebPortal.cpp` in line with the story. `ConfigManager.cpp` change (NVS validation) is a synthesis-added defensive fix beyond the original story spec. |

## Story fn-2-2 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Short-circuit ` | Pre-evaluate both atomics into `bool _cfgChg` and `bool _schedChg` before the `if`, then test `_cfgChg \|\| _schedChg`. |
| low | Silent NVS clamp — when `sched_enabled`, `sched_dim_start`, or `sched_dim_end` are out-of-range in NVS, the code clamps them silently with no log output, making field NVS corruption invisible. | Added `LOG_W("ConfigManager", "Corrupted <key> in NVS — ...")` before each clamp operation. |
| low | `test_schedule_default_dim_brightness` tests 4 schedule defaults but its name implies only the `sched_dim_brt` field. Misleading for future maintainers. | Renamed to `test_schedule_defaults` with updated comment; updated `RUN_TEST` call. |
| dismissed | `String timezone` heap allocation every second in `tickNightScheduler()` | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. The fix (new `getScheduleDimmingParams()` function) contradicts the "no new functions beyond what's specified" story constraint. Accepted design risk for a future-story optimization. |
| dismissed | Unit tests only validate `isInDimWindow()` helper, not the full scheduler state machine | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. Architectural limitation — Unity/embedded tests cannot instantiate the main loop, LWIP scheduler, or FastLED display. Full integration testing requires hardware. |
| dismissed | Missing test coverage for AC #6 (manual brightness re-override during dim window) | FALSE POSITIVE: Same architectural limitation. Already adjudicated in antipatterns. |
| dismissed | `getLocalTime()` result used without bounds-checking `tm_hour`/`tm_min` | FALSE POSITIVE: After a successful `getLocalTime()` return, POSIX guarantees `tm_hour ∈ [0,23]` and `tm_min ∈ [0,59]` — these are invariants of `localtime_r()`. A defensive bounds check would guard against a condition that cannot occur under normal POSIX semantics. Dismissed as theoretical concern. |
| dismissed | Non-atomic `g_lastNtpSynced` race with LWIP task on Core 1 | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. Only consequence is a duplicate/missing log line; story code comment documents "Core 1 only — no atomic needed." |
| dismissed | AC #8 not satisfied — `schedule_active` should reflect actual dimming state | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. Story Task 4.2 and Dev Notes explicitly define `schedule_active` as "enabled + NTP synced" and `schedule_dimming` as the real-time override indicator. The design is intentional and documented. |
| dismissed | Task 5 is a "task completion lie" — tests don't cover scheduler AC #1, #2, #3, #4, #5 | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. `isInDimWindow()` helper is the production arithmetic extracted as a testable unit. Full scheduler integration tests require hardware. |
| dismissed | Dead NTP-lost branch — `g_ntpSynced` is only ever set to `true`, never to `false`, making the "NTP lost" log path unreachable | FALSE POSITIVE: Already adjudicated FALSE POSITIVE across fn-2.1 reviews (Rounds 1-3). "Sticky true" behavior is the intentional design contract for this story. NTP invalidation on WiFi reconnect is a future-story concern. The dead branch is low-risk forward-compatibility scaffolding. |
| dismissed | NVS clamp hardcodes `1380`/`420` rather than reusing struct defaults | FALSE POSITIVE: Partially valid but subsumed by the LOG_W fix applied. The literal values `1380`/`420` match the `ScheduleConfig` default values exactly. No functional issue. |
| dismissed | Redundant `ConfigManager::getSchedule()` calls in display task (lines 327 and 363) | FALSE POSITIVE: Already adjudicated FALSE POSITIVE — line 327 is `ConfigManager::getDisplay()`, not `getSchedule()`. Single `getSchedule()` call is inside the `if (g_schedDimming.load())` guard. |
| dismissed | `schedule_active` API field semantics mislead consumers | FALSE POSITIVE: Resolved by `schedule_dimming` field. Already adjudicated across fn-2.1 and fn-2.2 antipatterns. |

## Story fn-2-2 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `ConfigManager::init()` does not call `_callbacks.clear()`, leaving stale lambda callbacks from prior `init()` calls in the test suite. If a test registered a callback capturing a local variable by reference, subsequent `applyJson()` calls from later tests invoke a dangling closure — undefined behavior. | Added `_callbacks.clear()` inside the `ConfigLockGuard` block in `init()`, immediately before `loadDefaults()`. |
| low | NTP state-transition log message `"NTP synced — schedule active"` fires before the `sched_enabled` guard check, making it factually incorrect when `sched_enabled == 0`. | Changed to `"NTP synced — local time available"` with an explanatory comment. |
| low | Corrupted NVS schedule fields are clamped and logged with `LOG_W` but never written back to NVS, so the warning repeats on every subsequent boot until the user manually corrects the value. | Added `anyCorrupted` tracking flag in `loadFromNvs()`; when any value is clamped, `schedulePersist(0)` + `LOG_W("...Scheduling NVS self-heal persist...")` are called after the snapshot is applied, triggering a corrective NVS write on the first loop tick. |
| dismissed | `schedule_active` in `/api/status` should reflect real-time override state, not "enabled + NTP synced" | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in Rounds 1 and 2 antipatterns. Story Task 4.2 and Dev Notes explicitly define `schedule_active` as a capability indicator; `schedule_dimming` is the real-time override field. The design is intentional and documented. |
| dismissed | AC #6 behavioral mismatch — manual brightness is immediately re-overridden, not on the next scheduler tick | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. The Dev Notes explicitly document this as intentional: "it's immediate, which is better than AC#6 describes." The code comment at line 365 also documents the intentional deviation. |
| dismissed | Scheduler is "not actually non-blocking" because `ConfigManager::getSchedule()` takes a `portMAX_DELAY` mutex that could be held by `persistToNvs()` | FALSE POSITIVE: FALSE POSITIVE. Both `tick()` (which drives `persistToNvs()`) and `tickNightScheduler()` run sequentially on the same FreeRTOS task (Core 1 loop task) — there is no concurrent contention. The `portMAX_DELAY` pattern is the established project convention used by every ConfigManager getter call throughout the codebase. |
| dismissed | Tests are too weak — only validate `isInDimWindow()` helper arithmetic, not runtime scheduler state | FALSE POSITIVE: Already adjudicated FALSE POSITIVE across all fn-2.1 and fn-2.2 review rounds. Architectural limitation: Unity embedded tests cannot instantiate the FreeRTOS main loop, LWIP callbacks, or FastLED display. The `isInDimWindow()` helper is the production arithmetic extracted as a testable unit — correct TDD discipline for the platform. |
| dismissed | `g_ntpSynced` "NTP lost" branch is permanently dead code — flag is never reset to `false` | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.1 (Rounds 1, 2, 3) and fn-2.2 (Round 2) antipatterns. "Sticky true" is the intentional design contract for this story. NTP invalidation on WiFi reconnect is a future-story concern. |
| dismissed | Missing `getLocalTime()` result bounds check for `tm_hour`/`tm_min` | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. POSIX guarantees `tm_hour ∈ [0,23]` and `tm_min ∈ [0,59]` after a successful `getLocalTime()` return. No defensive guard is needed against conditions that cannot occur under POSIX semantics. |
| dismissed | `isInDimWindow()` test helper duplicates production logic without sharing | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. Cannot share code between embedded firmware (`main.cpp`) and the test target without violating the "no new files" constraint. The helper is an intentional test-local copy of the production arithmetic. |
| dismissed | Missing explicit `#include <time.h>` in `main.cpp` | FALSE POSITIVE: FALSE POSITIVE. `esp_sntp.h` already transitively includes the required POSIX time headers for the ESP-IDF/Arduino framework. The build succeeds without an explicit `#include <time.h>`. |
| dismissed | `String timezone` heap allocation in `tickNightScheduler()` hot path (1/sec, ~2.6M allocs over 30 days) | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. The proposed fix (`getScheduleDimmingParams()`) would add a new function in violation of the story constraint. Accepted design risk for a future-story optimization. |
| dismissed | Missing boundary-value tests for schedule validation (`sched_dim_start = 1439`) | FALSE POSITIVE: FALSE POSITIVE. `test_apply_json_schedule_validation` already tests `sched_dim_start = 1440` (boundary+1, line 356) and asserts it is rejected. The maximum valid value (1439) is implicitly accepted by the validation logic `v > 1439`; any value ≤ 1439 passes. The off-by-one regression scenario would require changing `> 1439` to `> 1440`, which `test_apply_json_schedule_validation` already guards against. |
| dismissed | Redundant `ConfigManager::getSchedule()` calls in display task (lines 327 and 363) | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 antipatterns. Line 327 is `ConfigManager::getDisplay()`, not `getSchedule()`. There is exactly one `getSchedule()` call at line 368, inside the `if (g_schedDimming.load())` guard — only executes when actually dimming. |
| dismissed | Single-frame wrong-brightness race between config read and `g_schedDimming.load()` check | FALSE POSITIVE: Already adjudicated FALSE POSITIVE. The race window is microseconds; consequence is at most one display frame (~50ms) of wrong brightness, self-correcting on next scheduler tick or config change. Accepted design risk. |
| dismissed | AC #8 not satisfied — `schedule_active` should be `isScheduleDimming()` | FALSE POSITIVE: See first dismissal above. Story intentionally bifurcates into `schedule_active` (capability) and `schedule_dimming` (runtime override). |
| dismissed | Task 5 is a "task completion lie" — tests don't cover scheduler ACs 1–5 | FALSE POSITIVE: Same architectural limitation dismissal as above. |
| dismissed | Story file list drift (`WebPortal.cpp` listed untouched; `ConfigManager.cpp` unlisted) | FALSE POSITIVE: Audit trail concern, not a product defect. File list was corrected in Round 2. |
| dismissed | NVS clamp hardcodes `1380`/`420` rather than struct defaults | FALSE POSITIVE: Already adjudicated FALSE POSITIVE in fn-2.2 Round 2 antipatterns. The literal values exactly match `ScheduleConfig` defaults. No functional issue. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**🔥 ADVERSARIAL CODE REVIEW COMPLETE, Christian!**

**Story:** `/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md`

---

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 1 |
| AC Implementation Gaps | 0 |
| Task Completion Lies / Doc Inconsistencies | 2 |
| SOLID Violations | 0 |
| Hidden Bugs | 3 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **8** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Task/subtask checkbox inconsistency — Tasks 5 & 6 marked complete but subtasks 5.1 & 6.1 remain unchecked despite being implemented | Story file | +1.0 |
| 🟠 IMPORTANT | `timeToMinutes()` has no input validation — empty/malformed strings produce `NaN` sent to API | `dashboard.js:305-308` | +1.0 |
| 🟠 IMPORTANT | Double opacity bug when Night Mode fields are disabled — container `opacity:0.5` × input `opacity:0.5` = 0.25 effective opacity | `style.css:551-552,560-562` | +1.0 |
| 🟠 IMPORTANT | Status polling uses `setInterval` — can pile up requests on slow ESP32 responses instead of safe recursive `setTimeout` | `dashboard.js:3130` | +1.0 |
| 🟠 IMPORTANT | Weak type guards in `loadSettings()` — boolean `true` missed for `sched_enabled`, `null` passes through to produce `"NaN:NaN"` time display | `dashboard.js:394-406` | +1.0 |
| 🟡 MINOR | Silent UTC fallback for unsupported browser timezones — no user-facing warning when `getTimezoneConfig()` falls back | `dashboard.js:411-416` | +0.3 |
| 🟡 MINOR | Magic number polling interval — `10000` hardcoded without named constant | `dashboard.js:3130` | +0.3 |
| 🟡 MINOR | `setInterval` never cleaned up on page unload — minor resource leak | `dashboard.js:3130` | +0.3 |

### Evidence Score: 3.9

| Score | Verdict |
|-------|---------|
| **3.9** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

✅ No significant architectural violations detected.

---

## 🐍 Pythonic Crimes & Readability

- **Magic number:** `10000` ms polling interval is a bare literal with no constant name or comment explaining why 10 seconds was chosen.
  - 📍 `dashboard.js:3130`

✅ Code follows style guidelines and is readable.

---

## ⚡ Performance & Scalability

- **[MEDIUM] `setInterval` request pile-up risk:** `setInterval(updateNmStatus, 10000)` fires unconditionally every 10 seconds. On a resource-constrained ESP32, if `/api/status` takes longer than 10 s to respond, multiple overlapping requests can stack up. The same file already uses the safer recursive-`setTimeout` pattern in `startRebootPolling()` — Night Mode polling should follow that convention.
  - 📍 `dashboard.js:3130`
  - 💡 Fix: Replace with recursive `setTimeout` that schedules the next poll only after the current request completes or fails.

✅ No other significant performance issues detected.

---

## 🐛 Correctness & Safety

- **🐛 Bug: `timeToMinutes()` returns `NaN` on empty input:** If the browser doesn't enforce `type="time"` or the input is cleared programmatically, `timeStr.split(':')` yields `['']` and `parseInt(undefined, 10)` → `NaN`. This gets sent to `/api/settings` where ConfigManager rejects it, but the user gets a generic "Save failed" instead of a specific validation message.
  - 📍 `dashboard.js:305-308`
  - 🔄 Reproduction: Clear the "Dim Start" input value in dev tools, then click Apply.

- **🐛 Bug: Disabled fields render at 25 % opacity due to multiplicative opacity:** `.nm-fields-disabled` sets `opacity:0.5` on the container, and each disabled input inside also sets `opacity:0.5`. CSS opacity multiplies down the compositing tree (0.5 × 0.5 = 0.25), making the disabled time/brightness/timezone fields appear much dimmer than the intended 50 %.
  - 📍 `style.css:551-552,560-562`
  - 🔄 Reproduction: Uncheck "Enable Schedule" and observe the fields are barely readable.

- **🐛 Bug: Weak type guards allow `null` to leak into `minutesToTime()`:** `d.sched_dim_start !== undefined` evaluates `true` for `null`, and `parseInt(null, 10)` returns `NaN`, causing `minutesToTime(NaN)` to display `"NaN:NaN"` in the time input. Similarly, `d.sched_enabled === 1 || d.sched_enabled === '1'` misses a boolean `true` from the API.
  - 📍 `dashboard.js:394-406`
  - 🔄 Reproduction: Temporarily modify `/api/settings` mock to return `{"sched_dim_start": null}`.

- **🔒 [LOW] Security:** No security vulnerabilities detected in this frontend-only change.

✅ No lying tests — this story contains no test files.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt: Story task/subtask status is inconsistent:** Task 5 and Task 6 are marked `[x]` complete, but their only subtasks (5.1 and 6.1) are marked `[ ]` incomplete. The code clearly implements both subtasks, so the story documentation is out of sync with reality. This erodes trust in the task audit trail.
  - 📍 Story file, Tasks 5 & 6
  - 💥 Explosion radius: Future reviewers waste time verifying whether work was actually done.

- **💣 Tech Debt: Silent UTC fallback for browser-detected timezones:** The fn-2.1 antipatterns explicitly called out that fn-2.3 should "surface the unsupported-zone warning" for `getTimezoneConfig()` falling back to UTC. The current implementation silently selects UTC with no toast, console warning, or UI indicator when the browser reports an IANA zone absent from `TZ_MAP`.
  - 📍 `dashboard.js:411-416`
  - 💥 Explosion radius: Users in unsupported zones (e.g. `Asia/Manila`) have no idea their timezone wasn't recognized.

- **💣 Tech Debt: Unverified gzip rebuilds:** The story File List claims `dashboard.html.gz`, `dashboard.js.gz`, and `style.css.gz` were rebuilt, but there is no build artifact checksum or CI verification that the `.gz` contents actually match the modified sources. This is a recurring blind spot in the project.
  - 💥 Explosion radius: A stale gzip can silently serve old assets to the ESP32, causing hours of confusing debug time.

✅ Other maintainability aspects are acceptable.

---

## 🛠️ Suggested Fixes

### 1. Harden `timeToMinutes()` against invalid input

**File:** `firmware/data-src/dashboard.js`
**Issue:** Empty or malformed time strings produce `NaN`.

```javascript
function timeToMinutes(timeStr) {
  if (!timeStr || typeof timeStr !== 'string') return 0;
  var parts = timeStr.split(':');
  if (parts.length !== 2) return 0;
  var h = parseInt(parts[0], 10);
  var m = parseInt(parts[1], 10);
  if (isNaN(h) || isNaN(m)) return 0;
  return h * 60 + m;
}
```

### 2. Fix double opacity on disabled Night Mode fields

**File:** `firmware/data-src/style.css`
**Issue:** Container and inputs both apply `opacity: 0.5`, compounding to 25 %.

```css
/* Remove opacity from individual disabled inputs since the container already dims them */
.nm-fields input[type="time"]:disabled{cursor:default}
.nm-fields select:disabled{cursor:default}
.nm-fields input[type="range"]:disabled{cursor:default}
```

### 3. Use safe recursive `setTimeout` for status polling

**File:** `firmware/data-src/dashboard.js`
**Issue:** `setInterval` can pile up requests on a slow ESP32.

```javascript
function scheduleNextNmStatusPoll() {
  setTimeout(function() {
    updateNmStatus().then(scheduleNextNmStatusPoll).catch(scheduleNextNmStatusPoll);
  }, 10000);
}
updateNmStatus().then(scheduleNextNmStatusPoll).catch(scheduleNextNmStatusPoll);
```
*(Also update `updateNmStatus` to return the promise chain.)*

### 4. Strengthen type guards in `loadSettings()`

**File:** `firmware/data-src/dashboard.js`
**Issue:** `null` and boolean values leak through guards.

```javascript
if (typeof d.sched_enabled === 'number' || typeof d.sched_enabled === 'boolean' || typeof d.sched_enabled === 'string') {
  nmEnabled.checked = !!(d.sched_enabled === 1 || d.sched_enabled === '1' || d.sched_enabled === true);
}
if (typeof d.sched_dim_start === 'number') {
  nmDimStart.value = minutesToTime(parseInt(d.sched_dim_start, 10));
}
if (typeof d.sched_dim_end === 'number') {
  nmDimEnd.value = minutesToTime(parseInt(d.sched_dim_end, 10));
}
if (typeof d.sched_dim_brt === 'number') {
  nmDimBrt.value = d.sched_dim_brt;
  nmDimBrtVal.textContent = d.sched_dim_brt;
}
```

### 5. Surface unsupported timezone fallback to the user

**File:** `firmware/data-src/dashboard.js`
**Issue:** Silent UTC fallback is invisible to users.

```javascript
var detected = getTimezoneConfig();
if (detected.iana !== 'UTC' && TZ_MAP[detected.iana]) {
  nmTimezone.value = detected.iana;
  markSectionDirty('nightmode');
} else if (detected.iana !== 'UTC') {
  FW.showToast('Timezone ' + detected.iana + ' not supported — falling back to UTC', 'warning');
}
```

---

**Review Actions:**
- Issues Found: 8
- Issues Fixed: 0
- Action Items Created: 5

⚠️ Address the identified issues before proceeding. The double-opacity CSS bug and the `setInterval` pile-up risk are the highest-impact items to fix.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

I'll perform an adversarial code review of story fn-2.3 (dashboard-night-mode-card). Let me read the implementation files to verify they match the story requirements.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

**Code Review: `fn-2.3` Dashboard Night Mode Card**

1. 🔴 **Silent timezone corruption for supported manual POSIX values.** [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:316), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:408), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:275)  
`posixToIana()` falls back to `"UTC"` whenever the device holds a POSIX timezone string that is not present in `TZ_MAP`. On the next Night Mode save, `collectPayload()` serializes that fallback back as `UTC0`, so a valid saved timezone gets overwritten just by changing dim hours or brightness. That is real config corruption, and it is especially bad because the broader architecture explicitly allows manual POSIX fallback for unmapped zones.

2. 🟠 **AC #8 is not actually implemented: dirty state is cleared before the save succeeds.** [fn-2-3-dashboard-night-mode-card.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md:64), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:2667)  
The story says Night Mode dirty state is cleared *after successful apply*. The code calls `applyWithReboot(...)` and immediately calls `clearDirtyState()`. If `/api/settings` rejects the payload or the request fails, the apply bar disappears and the user loses visibility into unsaved Night Mode edits.

3. 🟠 **The NTP/schedule badges can stay falsely green on server-side errors.** [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:3093), [common.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/common.js:25)  
`FW.get()` resolves even for HTTP error envelopes, but `updateNmStatus()` just `return`s when `!res.body.ok` or `!res.body.data`. Only the network `catch` path clears the badges. Result: after a 500, malformed JSON, or other non-throwing error, the card can keep showing the last “NTP Synced” / “Currently dimming” state instead of degrading to unknown.

4. 🟠 **There is no automated coverage for the newly added Night Mode UI logic.** [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:304), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:393), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:3093)  
This story adds conversion helpers, reverse timezone lookup, auto-suggestion, payload assembly, and live status mapping, but no file under `firmware/test/` or `tests/` was updated to exercise any of it. The two bugs above are exactly the sort of regressions this missing coverage should have caught.

5. 🟡 **AC #5 is only partially met: the dropdown does not display canonical IANA names.** [fn-2-3-dashboard-night-mode-card.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md:43), [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:1023)  
The acceptance criterion says the option text should be the IANA timezone names. The implementation rewrites underscores to spaces, so `America/New_York` becomes `America/New York`. That is a small UI bug, but it is still a spec miss.

6. 🟡 **The story record is internally inconsistent.** [fn-2-3-dashboard-night-mode-card.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md:222), [fn-2-3-dashboard-night-mode-card.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md:254), [fn-2-3-dashboard-night-mode-card.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md:301), [fn-2-3-dashboard-night-mode-card.md](/Users/christianlee/App-Development/TheFlightWall_OSS-main/_bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md:340)  
Tasks 5, 6, 7, and 8 are marked complete at the parent level while their only subtasks remain unchecked, even though the corresponding code and `.gz` bundles are present. That is not a product bug, but it does make the implementation audit trail unreliable.

**Summary**

| Category | Count |
|---|---:|
| Git/File List Discrepancies | 0 |
| Acceptance Criteria Gaps | 4 |
| Task Completion Lies | 0 |
| Hidden Bugs | 3 |
| Test Coverage Gaps | 1 |
| Security Findings | 0 |
| Total Issues | 6 |

**Evidence Score**

| Severity | Source | Score |
|---|---|---:|
| 🔴 Critical | Silent timezone overwrite on save | +3.0 |
| 🟠 Important | Dirty state cleared before successful apply | +1.0 |
| 🟠 Important | Stale status badges on non-throwing `/api/status` errors | +1.0 |
| 🟠 Important | No automated coverage for new Night Mode UI logic | +1.0 |
| 🟡 Minor | Non-canonical IANA option labels | +0.3 |
| 🟡 Minor | Story bookkeeping inconsistency | +0.3 |
| 🟢 Clean Passes | File list accurate, `.gz` assets rebuilt, card placement correct, backend key names align, no security issue found | -2.5 |

**Evidence Score: 4.1**  
**Verdict: MAJOR REWORK**

**Clean Passes**

- The Night Mode card is inserted in the correct place between Firmware and System. [dashboard.html](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.html:260)
- The shipped `.gz` assets are rebuilt and currently match the edited source files.
- The UI is using the correct backend key names: `sched_enabled`, `sched_dim_start`, `sched_dim_end`, `sched_dim_brt`, and `timezone`. [dashboard.js](/Users/christianlee/App-Development/TheFlightWall_OSS-main/firmware/data-src/dashboard.js:275)

**Review Action**

6 findings. 0 fixes applied. This story is not ready to accept as-is; the timezone overwrite and premature dirty-state clearing need to be corrected before sign-off.

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-13</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">fn-2</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">3</var>
<var name="session_id">57b4f028-49f7-45b4-83a7-942035e4fc10</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="4edee726">embedded in prompt, file id: 4edee726</var>
<var name="story_id">fn-2.3</var>
<var name="story_key">fn-2-3-dashboard-night-mode-card</var>
<var name="story_num">3</var>
<var name="story_title">3-dashboard-night-mode-card</var>
<var name="template">False</var>
<var name="timestamp">20260413_1516</var>
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
      - Commit message format: fix(component): brief description (synthesis-fn-2.3)
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