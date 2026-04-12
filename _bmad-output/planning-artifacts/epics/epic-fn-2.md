## Epic fn-2: Night Mode & Brightness Scheduling — "The Wall That Sleeps"

User configures a timezone and brightness schedule on the dashboard. The wall automatically dims at night and restores in the morning — correct across midnight boundaries and DST transitions. NTP sync status is visible, and the schedule degrades gracefully if NTP is unavailable.

### Story fn-2.1: NTP Time Sync & Timezone Configuration

As a user,
I want the device to sync its clock after connecting to WiFi using my configured timezone,
So that time-dependent features like night mode have an accurate local clock.

**Acceptance Criteria:**

**Given** the device connects to WiFi in STA mode
**When** WiFiManager reports `STA_CONNECTED`
**Then** `configTzTime()` is called with the POSIX timezone string from `ConfigManager::getSchedule().timezone` and NTP servers `"pool.ntp.org"`, `"time.nist.gov"`
**And** NTP synchronization completes within 10 seconds of WiFi connection

**Given** NTP synchronization succeeds
**When** the `sntp_set_time_sync_notification_cb` callback fires
**Then** a `std::atomic<bool> ntpSynced` flag is set to `true`
**And** `SystemStatus::set(NTP, OK, "Clock synced")` is recorded
**And** subsequent calls to `getLocalTime(&now, 0)` return the correct local time for the configured timezone

**Given** NTP is unreachable after WiFi connects
**When** the sync attempt fails
**Then** `ntpSynced` remains `false`
**And** `SystemStatus::set(NTP, WARNING, "Clock not set")` is recorded
**And** the device does NOT crash or degrade any functionality other than schedule activation
**And** NTP re-sync is automatically retried by LWIP SNTP (default ~1 hour interval)

**Given** the timezone config key is changed via `POST /api/settings`
**When** the hot-reload callback fires
**Then** `configTzTime()` is called immediately with the new POSIX timezone string
**And** no reboot is required

**Given** `dashboard.js` includes a `TZ_MAP` object
**When** the timezone mapping is loaded
**Then** it contains ~50-80 common IANA-to-POSIX entries (e.g., `"America/Los_Angeles": "PST8PDT,M3.2.0,M11.1.0"`)
**And** `getTimezoneConfig()` returns `{ iana, posix }` using `Intl.DateTimeFormat().resolvedOptions().timeZone`

**Given** `GET /api/status` is called
**When** the response is built
**Then** it includes `"ntp_synced": true/false` and `"schedule_active": true/false`

**References:** FR15, FR18, FR22, AR6, AR15, NFR-P4, NFR-R2, NFR-R5

### Story fn-2.2: Night Mode Brightness Scheduler

As a user,
I want the device to automatically dim the LEDs during my configured night hours and restore brightness in the morning,
So that the wall is livable 24/7 without daily manual adjustment.

**Acceptance Criteria:**

**Given** schedule is enabled (`sched_enabled=1`) and NTP has synced (`ntpSynced=true`)
**When** the current local time enters the dim window (e.g., current time is 23:15 with dim_start=1380, dim_end=420)
**Then** LED brightness is overridden to `sched_dim_brt` via the existing ConfigManager hot-reload path
**And** the brightness change is reflected on the LED matrix within 1 second

**Given** a same-day schedule (dim_start < dim_end, e.g., 09:00-17:00)
**When** the scheduler checks `currentMinutes >= dimStart && currentMinutes < dimEnd`
**Then** the device is in the dim window during the specified hours and at normal brightness outside

**Given** a midnight-crossing schedule (dim_start > dim_end, e.g., 23:00-07:00)
**When** the scheduler checks `currentMinutes >= dimStart || currentMinutes < dimEnd`
**Then** the device is in the dim window from 23:00 through midnight to 07:00

**Given** a DST transition occurs (e.g., clocks spring forward at 2:00 AM)
**When** `configTzTime()` with the POSIX timezone string handles the transition
**Then** the schedule continues to operate correctly in local time without drift or double-triggering

**Given** schedule is enabled but NTP has NOT synced
**When** the scheduler runs
**Then** it skips the brightness check entirely (no override applied)
**And** manual brightness settings remain in effect

**Given** schedule is disabled (`sched_enabled=0`)
**When** the scheduler runs
**Then** it does nothing — manual brightness applies
**And** the configured times and brightness are preserved in NVS for re-enabling

**Given** the scheduler runs in the main loop on Core 1
**When** `getLocalTime(&now, 0)` is called with timeout=0
**Then** the call is non-blocking (returns immediately if time unavailable)
**And** the total scheduler overhead is a single `localtime_r()` + integer comparison per iteration
**And** main loop cycle time increases by no more than 1%

**Given** the device runs for 30+ days with schedule enabled
**When** operating continuously
**Then** the schedule executes correctly across every midnight boundary and DST transition
**And** NTP re-syncs automatically (LWIP default ~1 hour) to prevent RTC drift

**References:** FR19, FR20, FR21, FR23, FR24, AR7, AR8, AR-E14, AR-E15, NFR-P3, NFR-R1, NFR-R4, NFR-C5

### Story fn-2.3: Dashboard Night Mode Card

As a user,
I want a Night Mode card on the dashboard where I can set my timezone, configure a dim schedule, see a visual timeline, and monitor NTP sync status,
So that I can set up and manage night mode from my phone.

**Acceptance Criteria:**

**Given** the dashboard loads
**When** the Night Mode card renders
**Then** the card is always expanded (not collapsed by default)
**And** it fetches current settings via `GET /api/settings` and status via `GET /api/status`

**Given** the Night Mode card is visible
**When** rendered with current settings
**Then** it displays: timezone selector dropdown, schedule enable/disable toggle, dim start time picker, dim end time picker, dim brightness slider with value + "%" suffix, 24-hour timeline bar, and schedule status line

**Given** the timezone selector dropdown
**When** rendered
**Then** it lists IANA timezone names from `TZ_MAP`, pre-selected to the current configured timezone
**And** if the user's browser-detected timezone is in `TZ_MAP`, it is suggested as default
**And** if the current timezone is not in `TZ_MAP`, a manual POSIX entry field is shown

**Given** the Time Picker Row (`.time-picker-row`)
**When** rendered with schedule enabled
**Then** two side-by-side `<input type="time">` elements show dim start and dim end
**And** each has a proper `<label for>` association
**And** values are converted from minutes-since-midnight to HH:MM for display and back on change
**And** at 320px viewport width, each picker gets ~45% card width with gap

**Given** the schedule toggle is switched OFF
**When** the toggle state changes
**Then** time pickers and brightness slider become visually disabled (reduced opacity)
**And** the configured times and brightness are NOT deleted — toggling ON restores them

**Given** the dim brightness slider
**When** adjusted
**Then** the value displays inline as the user drags (existing `.range-val` pattern)
**And** the value shows with "%" suffix

**Given** the Timeline Bar (`.schedule-timeline`)
**When** rendered with time picker values
**Then** a canvas (24px height, full card width) shows the dim period as a shaded `--accent-dim` region on a `--bg-input` track
**And** hour markers (00, 06, 12, 18, 24) are rendered as HTML text below the canvas (not canvas text)
**And** a "now" marker (1px `--accent` vertical line) shows the current hour
**And** midnight-crossing schedules render as two `fillRect` calls (00:00-end and start-24:00)
**And** the canvas is `aria-hidden="true"` (time pickers are the accessible data)
**And** the timeline updates in real-time as time picker values change

**Given** the Schedule Status Line (`.schedule-status`)
**When** rendered based on current state
**Then** it shows the correct state with colored dot prefix:
- Active dimming: `--accent-dim` dot, "Dimmed (10%) until 07:00"
- Scheduled: `--success` dot, "Schedule active — next dim at 23:00"
- Waiting: `--warning` dot, "Schedule saved — waiting for clock sync"
- Clock not set: `--warning` dot, "Clock not set — schedule inactive"
- Disabled: `--text-secondary` dot, "Schedule disabled"
**And** the dot is `aria-hidden="true"`, text communicates the full state

**Given** any Night Mode setting is changed
**When** the setting differs from the last-saved state
**Then** a sticky apply bar (`.apply-bar`) appears at the bottom with "Unsaved changes" label and "Apply Changes" primary button
**And** tapping "Apply Changes" sends `POST /api/settings` with the changed schedule keys
**And** on success, a green toast confirms "Night mode schedule saved"
**And** the apply bar disappears
**And** no reboot is required (hot-reload keys)

**Given** all new CSS for Night Mode components
**When** added to `style.css`
**Then** total addition is approximately 25 lines
**And** all components meet WCAG 2.1 AA contrast requirements
**And** all interactive elements have 44x44px minimum touch targets
**And** all components work at 320px minimum viewport width with no new media queries

**Given** updated web assets (dashboard.html, dashboard.js, style.css)
**When** gzipped and placed in `firmware/data/`
**Then** the gzipped copies replace existing ones for LittleFS upload

**References:** FR16, FR19, FR22, FR23, FR31, FR34, AR6, UX-DR4, UX-DR5, UX-DR6, UX-DR7, UX-DR8, UX-DR10, UX-DR12, UX-DR13, NFR-P6

---

