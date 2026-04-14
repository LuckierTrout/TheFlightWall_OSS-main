# Story fn-2.3: Dashboard Night Mode Card

Status: done

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
  - [x] 5.1 Add a Night Mode section to `loadSettings()` after the Hardware block (after `updateHwResolution();` at line 360, before `syncCalibrationFromSettings();`):
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
  - [x] 6.1 Add a status polling function and call it on load and at interval:
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
  - [x] 7.1 Add Night Mode styles after the existing card styles (after the mode-card styles at ~line 527):
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
  - [x] 8.1 After all HTML/JS/CSS changes are complete, rebuild gzipped assets:
    ```bash
    cd firmware
    gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
    gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz
    gzip -9 -c data-src/style.css > data/style.css.gz
    ```
  - [x] 8.2 Upload filesystem to device:
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
| 2026-04-13 | Code review synthesis — fixed silent timezone corruption (nmTimezoneDirty flag), double CSS opacity bug, timeToMinutes() NaN guard, null type guards in loadSettings(); rebuilt dashboard.js.gz and style.css.gz |
| 2026-04-13 | Post-review verification — all 10 ACs confirmed, all 8 tasks + subtasks marked complete, build passes (77.9% flash), .gz assets valid. Story → done |
| 2026-04-13 | Code review synthesis (round 2) — fixed timeToMinutes() HH:MM:SS parse bug (< 2 guard), replaced setInterval with recursive setTimeout per established project pattern, added aria-live="polite" to nm-status; rebuilt dashboard.js.gz and dashboard.html.gz |
| 2026-04-13 | Code review synthesis (round 3) — moved nmTimezoneDirty declaration to top of IIFE, replaced getElementById calls in collectPayload() with cached refs, added null guard to updateNmStatus(), fixed #0d1117 → var(--bg) in CSS; rebuilt dashboard.js.gz and style.css.gz |

## Senior Developer Review (AI)

### Review: 2026-04-13
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 4.1 → REJECT
- **Issues Found:** 4
- **Issues Fixed:** 4
- **Action Items Created:** 0

#### Review Follow-ups (AI)
<!-- All verified issues were fixed in this synthesis pass. No remaining action items. -->

### Review: 2026-04-13 (Round 2 — 4 reviewers)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 1.2 → APPROVED (Reviewer A), 3.2 → REJECT (Reviewer D) → net synthesis APPROVED after false-positive dismissals
- **Issues Found (verified):** 3
- **Issues Fixed:** 3
- **Action Items Created:** 0

#### Review Follow-ups (AI)
<!-- All verified issues were fixed in this synthesis pass. No remaining action items. -->

### Review: 2026-04-13 (Round 3 — 4 reviewers)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** net ~2.5 → APPROVED after false-positive dismissals
- **Issues Found (verified):** 4
- **Issues Fixed:** 4
- **Action Items Created:** 0

#### Review Follow-ups (AI)
<!-- All verified issues were fixed in this synthesis pass. No remaining action items. -->

## Dev Agent Record

### Implementation Session — 2026-04-13
- **Agent:** Claude Code (Opus)
- **Scope:** Full implementation of story fn-2.3 + code review resolution

#### Implementation Summary
All 8 tasks implemented across 3 source files:
- `dashboard.html`: Night Mode card HTML (lines 260-290) with NTP badge, schedule toggle, time inputs, brightness slider, timezone dropdown
- `dashboard.js`: Dirty tracking integration, `collectPayload()` nightmode block, `timeToMinutes()`/`minutesToTime()`/`posixToIana()` helpers, timezone dropdown population from sorted TZ_MAP keys, event handlers, `loadSettings()` NM block with browser timezone auto-suggest, `updateNmStatus()` polling every 10s
- `style.css`: Night Mode card styles (lines 530-564) — NTP badge with synced/warning states, schedule status indicators, toggle label, disabled field styling, time input theming

#### Code Review Resolution (4 issues fixed)
1. **nmTimezoneDirty flag** — prevents silent timezone corruption when user changes unrelated NM fields without touching timezone dropdown
2. **Disabled CSS cursor-only rules** — avoids double-opacity bug (container 0.5 × disabled element 0.5 = 0.25)
3. **timeToMinutes NaN guard** — returns 0 for invalid/empty input instead of NaN propagation
4. **typeof guards in loadSettings** — uses `typeof d.sched_dim_brt === 'number'` guards for number fields to prevent falsy-zero bugs

#### Build Verification
- `pio run` → SUCCESS (RAM 16.7%, Flash 77.9%)
- All .gz assets rebuilt and validated via `gzip -t`
- No native test environment available (esp32dev only); pure frontend story with no C++ changes

#### Code Review Resolution — Round 2 (3 issues fixed)
1. **timeToMinutes() HH:MM:SS robustness** — changed `parts.length !== 2` guard to `parts.length < 2` so browsers that emit `HH:MM:SS` (e.g. with sub-minute step) still parse correctly; added range clamp (0–1439)
2. **setInterval → recursive setTimeout** — `updateNmStatus` polling now uses recursive `setTimeout` inside `.then()` and `.catch()`, matching the established project pattern (`startRebootPolling` at line 3069) to prevent concurrent in-flight requests against the resource-constrained ESP32
3. **ARIA live region** — added `aria-live="polite"` to `#nm-status` div so screen readers announce NTP sync and schedule state changes

#### Code Review Resolution — Round 3 (4 issues fixed)
1. **nmTimezoneDirty variable hoisting** — moved `var nmTimezoneDirty = false` declaration to top of IIFE near `dirtySections` (line 215) with explanatory comment; removed duplicate declaration at former line 1047. Eliminates reliance on hoisted `undefined` in `clearDirtyState()`.
2. **collectPayload() uses cached DOM refs** — replaced five `document.getElementById()` calls in the nightmode payload block with cached variables (`nmEnabled`, `nmDimStart`, `nmDimEnd`, `nmDimBrt`, `nmTimezone`); consistent with every other section in `collectPayload()`. Also added null guard on `nmTimezone` itself.
3. **updateNmStatus() null guard** — added `if (!nmNtpBadge || !nmSchedStatus) return;` at function entry to match the defensive pattern used by `updateModeStatus()` and prevent TypeError if Night Mode card is absent from the HTML.
4. **CSS hardcoded #0d1117** — replaced hardcoded dark color on `.nm-ntp-ok` and `.nm-ntp-warn` badge rules with `var(--bg)` to stay in the design system and support future theme changes.
5. **Build:** `pio run` → SUCCESS (RAM 16.7%, Flash 77.9%); `dashboard.js.gz` and `style.css.gz` rebuilt and validated via `gzip -t`.
