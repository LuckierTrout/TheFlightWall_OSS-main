<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: fn-2 -->
<!-- Story: 3 -->
<!-- Phase: validate-story-synthesis -->
<!-- Timestamp: 20260413T190113Z -->
<compiled-workflow>
<mission><![CDATA[

Master Synthesis: Story fn-2.3

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
<file id="4edee726" path="_bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md" label="DOCUMENTATION"><![CDATA[

# Story fn-2.3: Dashboard Night Mode Card

Status: ready-for-dev

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

- [ ] **Task 1: Add Night Mode card HTML to dashboard.html** (AC: #1, #2, #3, #4, #5, #6, #7)
  - [ ] 1.1 Insert a new `<section class="card" id="nightmode-card">` block after the Firmware card (`</section>` at ~line 260) and before the System card (`<section class="card card-danger" id="system-card">` at line 261):
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
  - [ ] 1.2 Verify the card appears between Firmware and System cards in the rendered page.

- [ ] **Task 2: Add Night Mode section to dirty tracking and payload collection in dashboard.js** (AC: #8)
  - [ ] 2.1 Add `nightmode: false` to the `dirtySections` object (line 211):
    ```js
    var dirtySections = { display: false, timing: false, network: false, hardware: false, nightmode: false };
    ```
  - [ ] 2.2 Add `dirtySections.nightmode = false;` to `clearDirtyState()` (after line 223, before the `applyBar.style.display = 'none'`):
    ```js
    dirtySections.nightmode = false;
    ```
  - [ ] 2.3 Add a `nightmode` block to `collectPayload()` (after the `hardware` block ending at line 274, before `return payload;` at line 275):
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

- [ ] **Task 3: Add time conversion helpers in dashboard.js** (AC: #3, #9)
  - [ ] 3.1 Add helper functions near the top of the DOMContentLoaded handler (after the `getTimezoneConfig()` usage area, before `loadSettings()`):
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

- [ ] **Task 4: Populate timezone dropdown and wire up Night Mode event handlers in dashboard.js** (AC: #2, #5, #10)
  - [ ] 4.1 Add Night Mode initialization code after the existing event handler section (after the hardware input handlers, before `loadSettings()` call):
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
      nmFields.style.opacity = on ? '1' : '0.5';
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

- [ ] **Task 5: Load Night Mode settings from API response in dashboard.js** (AC: #9, #10)
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

- [ ] **Task 6: Add NTP and schedule status polling in dashboard.js** (AC: #6, #7)
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

        // Schedule status
        if (s.schedule_dimming) {
          nmSchedStatus.textContent = 'Currently dimming';
          nmSchedStatus.className = 'nm-sched-status nm-dimming';
        } else if (s.schedule_active) {
          nmSchedStatus.textContent = 'Schedule active';
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

- [ ] **Task 7: Add Night Mode card styles to style.css** (AC: #1, #2, #6, #7, #10)
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

- [ ] **Task 8: Rebuild gzipped web assets** (Post-implementation)
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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

I'll conduct a thorough validation of story fn-2.3 (dashboard-night-mode-card) following the workflow.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

# 🎯 Story Context Validation Report

**Story:** fn-2.3 - dashboard-night-mode-card
**Story File:** _bmad-output/implementation-artifacts/stories/fn-2-3-dashboard-night-mode-card.md
**Validated:** 2026-04-13
**Validator:** Quality Competition Engine

---

## Executive Summary

### Issues Overview

| Category | Found | Applied |
|----------|-------|---------|
| 🚨 Critical Issues | 0 | 0 |
| ⚡ Enhancements | 2 | 0 |
| ✨ Optimizations | 1 | 0 |
| 🤖 LLM Optimizations | 1 | 0 |

**Overall Assessment:** The story is well-written, highly detailed, and aligns strongly with architectural decisions and existing patterns. There are no critical issues that would block implementation or cause major regressions. Minor enhancements and LLM optimizations can further improve clarity and maintainability.

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | Duplicate `TZ_MAP` object in `dashboard.js` and `wizard.js`. | `dashboard.js`, `wizard.js` | +1 |
| 🟡 MINOR | Task 4.1 "Night Mode initialization code after the existing event handler section" could be more specific. | Task 4.1 | +0.3 |
| 🟡 MINOR | The `nmFields.style.opacity` directly manipulates DOM style rather than toggling a CSS class. | Task 4.1 | +0.3 |
| 🟡 MINOR | "Update Firmware" button still present in `dashboard.html` despite previous OTA Pull story. | `firmware/data-src/dashboard.html` | +0.3 |

### Evidence Score: 1.9

| Score | Verdict |
|-------|---------|
| **1.9** | **PASS** |

---

## 🎯 Ruthless Story Validation fn-2.3

### INVEST Criteria Assessment

| Criterion | Status | Severity | Details |
|-----------|--------|----------|---------|
| **I**ndependent | ✅ PASS | 0/10 | Explicitly lists `fn-2.1` and `fn-2.2` as `DONE` dependencies, ensuring no blocking external work. |
| **N**egotiable | ✅ PASS | 0/10 | The story is highly prescriptive regarding UI elements, which is appropriate for a UI implementation story with strong UX requirements. It defines "what" should be seen and "how" it should behave. |
| **V**aluable | ✅ PASS | 0/10 | Delivers clear value by providing a user interface to configure automatic nighttime dimming, eliminating the need for manual API calls or serial access. |
| **E**stimable | ✅ PASS | 0/10 | The tasks are detailed, providing HTML, JS, and CSS snippets, making the scope and complexity clearly defined and estimable. |
| **S**mall | ✅ PASS | 0/10 | Involves modifying existing UI files (`dashboard.html`, `dashboard.js`, `style.css`) and interacting with existing APIs. The scope is well-contained for a single sprint. |
| **T**estable | ✅ PASS | 0/10 | Acceptance criteria are precise, measurable, and cover UI behavior, API interaction, and conditional states. |

### INVEST Violations

✅ No significant INVEST violations detected.

### Acceptance Criteria Issues

✅ Acceptance criteria are well-defined and testable.

### Hidden Risks and Dependencies

✅ No hidden dependencies or blockers identified.

### Estimation Reality-Check

**Assessment:** Realistic

The story details the required HTML, JavaScript logic, and CSS, making the implementation straightforward for a developer agent. The dependencies on `fn-2.1` and `fn-2.2` are explicitly marked as completed, ensuring no external blockers.

### Technical Alignment

**Status:** ✅ ALIGNED

✅ Story aligns with architecture.md patterns.

---

## 🚨 Critical Issues (Must Fix)

✅ No critical issues found - the original story covered essential requirements.

---

## ⚡ Enhancement Opportunities (Should Add)

### 1. Unified Time Conversion Helpers in `common.js`

**Benefit:** Centralizes utility functions, improves maintainability, and prevents duplication across different JS files.

**Current Gap:** The `timeToMinutes`, `minutesToTime`, and `posixToIana` helper functions are duplicated or intended to be duplicated within `dashboard.js` and `wizard.js`. While this story only adds them to `dashboard.js`, the `wizard.js` task implies a future duplication. The `TZ_MAP` is also duplicated.

**Suggested Addition:** Extract `timeToMinutes`, `minutesToTime`, `posixToIana`, and the `TZ_MAP` object into `firmware/data-src/common.js`. Then, both `dashboard.js` and `wizard.js` can import/use them from this central location. This aligns with the `fn-2.1` review follow-up.

### 2. Standardize UI for OTA Pull/Push

**Benefit:** Improves user experience by consolidating firmware update options into a single, intuitive section.

**Current Gap:** The `Firmware Card` HTML (`dashboard.html`) still contains the OTA push elements (drag and drop zone, upload button) from `fn-1.6`, even though the architecture for Delight Release includes an "OTA Pull from GitHub Releases" feature, and UI for OTA pull might live in a separate card or be integrated into this one. Having both the push and pull functionality in separate, non-integrated sections could be confusing.

**Suggested Addition:** Revisit the `Firmware Card` in `dashboard.html`. If the OTA Pull feature (from Delight Release planning artifacts) is intended to replace or supersede the push functionality, the UI should reflect that. At minimum, the existing push elements should be visually distinguished or integrated into a more comprehensive firmware update section if both are retained. For now, it's a minor UI incongruity rather than a functional bug.

---

## ✨ Optimizations (Nice to Have)

### 1. Prefer CSS Class for Visual State

**Value:** Improves maintainability, separates concerns (structure/behavior/style), and allows for more complex styling transitions.

**Suggestion:** Instead of directly setting `nmFields.style.opacity` in JavaScript (`updateNmFieldState` function), consider toggling a CSS class (e.g., `nm-fields-disabled`) on the `nm-fields` div. The CSS could then define the opacity (and any other styles like pointer-events) for this class.

```css
/* In style.css */
.nm-fields.nm-fields-disabled {
  opacity: 0.5;
  pointer-events: none; /* Disable interactions when inactive */
}
```
```js
// In dashboard.js (updateNmFieldState function)
if (!on) {
  nmFields.classList.add('nm-fields-disabled');
} else {
  nmFields.classList.remove('nm-fields-disabled');
}
```

---

## 🤖 LLM Optimization Improvements

### 1. Refine Task 4.1 Heading for Specificity

**Issue:** Ambiguity issues
**Token Impact:** Minor

**Current:**
```
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
  nmFields.style.opacity = on ? '1' : '0.5';
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

**Optimized:**
```
// --- Night Mode card: DOM elements, timezone dropdown, and event handlers (Story fn-2.3) ---

// Get DOM elements for Night Mode card
var nmEnabled = document.getElementById('nm-enabled');
var nmDimStart = document.getElementById('nm-dim-start');
var nmDimEnd = document.getElementById('nm-dim-end');
var nmDimBrt = document.getElementById('nm-dim-brt');
var nmDimBrtVal = document.getElementById('nm-dim-brt-val');
var nmTimezone = document.getElementById('nm-timezone');
var nmFields = document.getElementById('nm-fields');

// Populate timezone dropdown from TZ_MAP on load
(function() {
  var keys = Object.keys(TZ_MAP).sort();
  for (var i = 0; i < keys.length; i++) {
    var opt = document.createElement('option');
    opt.value = keys[i];
    opt.textContent = keys[i].replace(/_/g, ' ');
    nmTimezone.appendChild(opt);
  }
})();

// Function to update the disabled/enabled state of Night Mode fields
function updateNmFieldState() {
  var on = nmEnabled.checked;
  nmDimStart.disabled = !on;
  nmDimEnd.disabled = !on;
  nmDimBrt.disabled = !on;
  nmTimezone.disabled = !on;
  nmFields.style.opacity = on ? '1' : '0.5'; // Consider using a CSS class instead for better styling control
}

// Add event listeners for Night Mode controls
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

// Initial update of field states when script runs
updateNmFieldState();
```

**Rationale:** The current heading "Add Night Mode initialization code after the existing event handler section" (Task 4) is vague. The proposed optimized heading clarifies that this section covers DOM element retrieval, timezone dropdown population, and event listener setup. This helps an LLM agent quickly understand the distinct parts of the task. Additionally, comments within the code block can further guide the LLM.

---

## 🏆 Competition Results

### Quality Metrics

| Metric | Score |
|--------|-------|
| Requirements Coverage | 100% |
| Architecture Alignment | 100% |
| Previous Story Integration | 100% |
| LLM Optimization Score | 90% |
| **Overall Quality Score** | **97.5%** |

### Disaster Prevention Assessment

- **Reinvention Prevention:** ✅ Status: All opportunities for reuse (helpers, patterns) are identified and integrated.
- **Technical Specification:** ✅ Status: API contracts are correctly referenced, data types and ranges are specified.
- **File Structure:** ✅ Status: Changes are in the correct files, adhering to project structure.
- **Regression:** ✅ Status: Changes are localized with minimal risk of breaking existing functionality due to specific insertion points and CSS class naming.
- **Implementation:** ✅ Status: Instructions are clear and detailed, minimizing ambiguity.

### Competition Outcome

🏆 **Validator identified 4 improvements** that enhance the story context.

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
<var name="epic_num">fn-2</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/validate-story-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/validate-story-synthesis/instructions.xml</var>
<var name="name">validate-story-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="568">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="session_id">01514ddf-37f4-4c8e-a0c5-337f1b894235</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="4edee726">embedded in prompt, file id: 4edee726</var>
<var name="story_id">fn-2.3</var>
<var name="story_key">fn-2-3-dashboard-night-mode-card</var>
<var name="story_num">3</var>
<var name="story_title">3-dashboard-night-mode-card</var>
<var name="template">False</var>
<var name="timestamp">20260413_1501</var>
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