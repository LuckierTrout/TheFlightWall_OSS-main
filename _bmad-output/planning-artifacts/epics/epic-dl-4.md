## Epic dl-4: Ambient Intelligence — Mode Scheduling

The wall runs itself. The owner configures time-based rules once and forgets. Schedule and night mode brightness operate independently. The schedule takes priority over data-driven idle fallback when explicitly configured.

### Story dl-4.1: Schedule Rules Storage and Orchestrator Integration

As an owner,
I want to define time-based rules that automatically switch display modes at configured times,
So that the wall shows the right content at the right time without my intervention — Departures Board during the day, Clock Mode at night.

**Acceptance Criteria:**

**Given** the owner has defined one or more schedule rules
**When** the current time matches a rule's time window
**Then** the ModeOrchestrator transitions to SCHEDULED state and requests a switch to the rule's configured mode within 5 seconds of the configured time (NFR6)

**Given** a schedule rule is active (SCHEDULED state)
**When** the current time exits that rule's time window and no other rule matches
**Then** the orchestrator exits SCHEDULED state, returns to MANUAL, and restores the owner's manual mode selection

**Given** a schedule rule is active (SCHEDULED state)
**When** the flight data pipeline returns zero flights
**Then** the orchestrator stays in SCHEDULED state — the schedule takes priority over idle fallback (FR17)

**Given** the night mode brightness scheduler is active
**When** a mode schedule rule fires
**Then** the mode switches without resetting brightness, and a brightness change does not trigger a mode switch — the two schedulers operate independently (FR15)

**Given** schedule rules have been saved
**When** the device reboots or loses power
**Then** all schedule rules are restored from NVS on boot (FR36)

**Given** multiple schedule rules have overlapping time windows
**When** `ModeOrchestrator::tick()` evaluates rules
**Then** the first matching rule (lowest index) wins — rule priority is determined by index order

**Given** a schedule rule spans midnight (e.g., start 1320, end 360)
**When** `timeInWindow()` evaluates the current time
**Then** the midnight-crossing is handled correctly — same convention as Foundation brightness scheduler

**Given** `ConfigManager` does not yet have schedule storage
**When** this story is implemented
**Then** `ModeScheduleConfig` and `ScheduleRule` structs are added to `ConfigManager.h`, `getModeSchedule()` and `setModeSchedule()` methods are implemented using indexed NVS keys (`sched_r{N}_start`, `sched_r{N}_end`, `sched_r{N}_mode`, `sched_r{N}_ena`, `sched_r_count`) with a fixed max of 8 rules (AR4), all keys within 15-char NVS limit

**Given** NTP has not synced yet after boot
**When** `ModeOrchestrator::tick()` calls `getLocalTime(&now, 0)`
**Then** the non-blocking call returns false, tick skips schedule evaluation, and the device continues in MANUAL state until NTP syncs

### Story dl-4.2: Schedule Management Dashboard UI

As an owner,
I want to create, edit, and delete mode schedule rules through the dashboard,
So that I can configure the wall's automated behavior quickly without touching firmware.

**Acceptance Criteria:**

**Given** the owner opens the dashboard
**When** the schedule section loads
**Then** `GET /api/schedule` returns all configured rules with start time, end time, mode ID, enabled status, the current orchestrator state ("manual"/"scheduled"/"idle_fallback"), and the active rule index (-1 if none)

**Given** the owner creates a new schedule rule via the dashboard
**When** they set a start time, end time, and target mode and submit
**Then** `POST /api/schedule` saves the updated rule set to NVS via `ConfigManager::setModeSchedule()`, the response confirms `{ ok: true, data: { applied: true } }`, and ModeOrchestrator picks up the change on its next tick (~1 second)

**Given** the owner edits an existing schedule rule
**When** they change the start time, end time, mode, or enabled status and submit
**Then** the rule is updated in-place at its current index and persisted to NVS

**Given** the owner deletes a schedule rule from the middle of the list
**When** the deletion is submitted
**Then** higher-index rules shift down to fill the gap (compaction — no index gaps), `sched_r_count` decrements, and the compacted list is persisted to NVS

**Given** the owner has configured schedule rules
**When** viewing the schedule section
**Then** all rules are displayed with their current status — active rules visually distinguished from inactive ones (FR39), and the current orchestrator state is shown

**Given** the owner has made an error in a schedule rule (e.g., wrong times)
**When** they edit the entry inline
**Then** the correction is saved and takes effect within one orchestrator tick (~1 second) — no reboot required

**Given** the schedule management dashboard section
**When** it loads
**Then** times are displayed in the owner's local format, mode names match the display names from `GET /api/display/modes`, and the UI follows existing dashboard CSS and JS fetch patterns (NFR7: page load <1 second)

---

