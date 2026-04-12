## Epic dl-1: The Wall Never Sleeps — Clock Mode & Orchestration Foundation

The owner sees a functioning display 100% of the time. When no flights are in range, Clock Mode activates automatically. The owner can manually switch modes from the dashboard. Watchdog recovery reboots to Clock Mode as a known-good default. New config keys introduced by the Delight Release resolve to safe defaults on first read.

### Story dl-1.1: Clock Mode Time Display

As an owner,
I want the wall to display the current time in a large-format layout on the LED matrix,
So that the wall always shows something useful even when I'm not tracking flights.

**Acceptance Criteria:**

**Given** the device has booted and NTP time is synced
**When** Clock Mode is the active display mode
**Then** the LED matrix displays the current time in large-format digits readable from across the room
**And** the time updates every second without flickering or artifacts

**Given** Clock Mode is active
**When** the `m_clock_format` NVS key is absent or set to default (0)
**Then** the clock displays time in 24-hour format (e.g., "14:30")

**Given** Clock Mode is active
**When** the `m_clock_format` NVS key is set to 1
**Then** the clock displays time in 12-hour format (e.g., "2:30")

**Given** the device is upgrading from a prior firmware version that lacks Delight config keys
**When** `ConfigManager::getModeSetting("clock", "format", 0)` is called and the NVS key `m_clock_format` does not exist
**Then** the method returns the default value (0 = 24h) without error and without overwriting any existing stored NVS keys (FR33)

**Given** NTP has not synced yet after boot
**When** Clock Mode is active
**Then** the display shows a static time indicator or uptime-based time rather than a blank screen

**Given** ClockMode is implemented
**When** the firmware is compiled
**Then** `ClockMode` implements the `DisplayMode` interface (`init()`, `render()`, `teardown()`, `getName()`, `getZoneDescriptor()`), declares `CLOCK_SETTINGS[]` and `CLOCK_SCHEMA` as `static const` in `ClockMode.h` (rule 29), is registered in `MODE_TABLE[]` in `main.cpp` with a non-null `settingsSchema` pointer, and `ModeEntry` struct includes the new `settingsSchema` field (AR8)

**Given** `ConfigManager` does not yet have per-mode setting helpers
**When** this story is implemented
**Then** `ConfigManager::getModeSetting(abbrev, key, default)` and `setModeSetting(abbrev, key, value)` are added, constructing NVS keys as `m_{abbrev}_{key}` (rule 28), and `ModeSettingDef` + `ModeSettingsSchema` structs are added to `interfaces/DisplayMode.h`

### Story dl-1.2: Idle Fallback to Clock Mode

As an owner,
I want the wall to automatically switch to Clock Mode when no flights are in range and switch back when flights return,
So that the wall never shows a blank screen or loading spinner — guests always see a functioning display.

**Acceptance Criteria:**

**Given** a display mode other than Clock Mode is active and showing flight data
**When** the flight data pipeline returns zero aircraft in range
**Then** within one poll interval (~30s, NFR3) the wall switches to Clock Mode automatically

**Given** Clock Mode is active due to idle fallback (zero flights)
**When** a new flight enters tracking range
**Then** the wall switches back to the owner's previously active mode (the MANUAL selection)

**Given** the system has just booted
**When** the first `ModeOrchestrator::tick()` executes (~1 second after boot) and zero flights are in range
**Then** the orchestrator transitions from MANUAL to IDLE_FALLBACK state and requests a switch to Clock Mode

**Given** `ModeOrchestrator` is implemented
**When** the firmware is compiled
**Then** `core/ModeOrchestrator.h/.cpp` exists as a static class with `OrchestratorState` enum (MANUAL, SCHEDULED, IDLE_FALLBACK), `init()`, `tick()`, `onManualSwitch()`, `getState()`, and `getManualModeId()` methods

**Given** `g_flightCount` does not yet exist
**When** this story is implemented
**Then** `std::atomic<uint8_t> g_flightCount` is declared in `main.cpp` (rule 30), `FlightDataFetcher.cpp` stores the flight count after queue write via `g_flightCount.store()`, and `ModeOrchestrator::tick()` reads it via `g_flightCount.load()`

**Given** `ModeOrchestrator::tick()` is implemented
**When** it is wired into the Core 1 main loop
**Then** it executes at ~1-second intervals (non-blocking) and does not interfere with the existing brightness scheduler or flight data pipeline

### Story dl-1.3: Manual Mode Switching via Orchestrator

As an owner,
I want to manually switch between available display modes from the dashboard,
So that I can choose what the wall shows right now without waiting for automatic triggers.

**Acceptance Criteria:**

**Given** the owner is on the dashboard and the Mode Picker is visible
**When** the owner selects a different display mode
**Then** `POST /api/display/mode` routes through `ModeOrchestrator::onManualSwitch()` (rule 24) and the wall switches to the selected mode

**Given** the wall is in IDLE_FALLBACK state (showing Clock Mode because zero flights)
**When** the owner manually selects a different mode via the dashboard
**Then** the orchestrator exits IDLE_FALLBACK, enters MANUAL state, and switches to the selected mode

**Given** the wall is in MANUAL state
**When** `ModeOrchestrator::onManualSwitch()` is called
**Then** the manual mode selection is stored in `_manualModeId` (persisted via NVS `display_mode` key) so it survives power cycles

**Given** `WebPortal::POST /api/display/mode` currently calls `ModeRegistry::requestSwitch()` directly
**When** this story is implemented
**Then** the handler is updated to call `ModeOrchestrator::onManualSwitch(modeId)` instead, and `ModeRegistry::requestSwitch()` is called from exactly two methods: `ModeOrchestrator::tick()` and `ModeOrchestrator::onManualSwitch()` — no other call sites (rule 24)

### Story dl-1.4: Watchdog Recovery to Default Mode

As an owner,
I want the device to recover automatically from a crash by rebooting into Clock Mode,
So that a display mode bug causes a brief reboot, not a bricked or stuck device.

**Acceptance Criteria:**

**Given** the watchdog timer detects an unresponsive main loop
**When** the watchdog triggers a reboot
**Then** the device reboots within 10 seconds (NFR10) and restarts in a known-good default mode without user intervention (NFR11)

**Given** the device is booting after a watchdog reboot
**When** the NVS-persisted `display_mode` key references a mode whose `init()` fails
**Then** the system falls back to Clock Mode ("clock") as the default and logs the failure

**Given** the device is booting after a watchdog reboot
**When** the NVS-persisted `display_mode` key references a mode ID not found in `MODE_TABLE`
**Then** the system falls back to Clock Mode ("clock") as the default

**Given** Clock Mode is the known-good default
**When** the device boots normally (not a watchdog reboot)
**Then** the previously active mode is restored from NVS as before — Clock Mode default only applies when the stored mode is invalid or fails init

---

