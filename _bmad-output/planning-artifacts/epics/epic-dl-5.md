## Epic dl-5: Tailored Experience — Mode-Specific Settings

Each display mode is configurable through the Mode Picker. Clock Mode offers 12h/24h format, Departures Board configures row count and telemetry fields. Settings apply without reboot and persist across power cycles.

### Story dl-5.1: Per-Mode Settings Panels in Mode Picker

As an owner,
I want to configure each display mode's settings through the Mode Picker on the dashboard,
So that I can tailor Clock Mode's time format, Departures Board's row count, and other mode-specific options without touching firmware.

**Acceptance Criteria:**

**Given** the owner opens the Mode Picker on the dashboard
**When** `GET /api/display/modes` responds
**Then** each mode in the response includes a `settings` array containing the schema (key, label, type, default, min, max, enumOptions) and current values for each setting — or `null`/empty array for modes with no settings (ClassicCardMode, LiveFlightCardMode)

**Given** the API handler builds the settings array for a mode
**When** iterating settings
**Then** the handler iterates `ModeEntry.settingsSchema` dynamically (rule 28) — never hardcodes setting names — so that adding a setting to a mode's schema automatically surfaces it in the API response

**Given** the Mode Picker displays a mode with settings (e.g., Clock Mode)
**When** the settings panel renders
**Then** the UI dynamically generates form controls from the schema: dropdowns for `enum` type, number inputs for `uint8`/`uint16` with min/max constraints, toggles for `bool` type

**Given** the owner changes Clock Mode's time format from 24h to 12h in the Mode Picker
**When** the setting is submitted via `POST /api/display/mode` with a `settings` object
**Then** the setting is persisted to NVS via `ConfigManager::setModeSetting("clock", "format", 1)` and applied to the display within 2 seconds (NFR8) without requiring a device reboot (FR19)

**Given** the owner changes Departures Board's max rows from 4 to 2
**When** the setting is submitted
**Then** `ConfigManager::setModeSetting("depbd", "rows", 2)` persists the value and Departures Board renders at most 2 rows on the next frame

**Given** mode-specific settings have been saved
**When** the device reboots or loses power
**Then** all per-mode settings are restored from NVS (FR20) — `ConfigManager::getModeSetting()` reads the stored values using the `m_{abbrev}_{key}` NVS key convention

**Given** a mode with no settings schema (e.g., ClassicCardMode with `settingsSchema = nullptr`)
**When** the Mode Picker renders that mode's panel
**Then** no settings controls are shown — the panel displays only the mode name, description, and activation button

**Given** the Mode Picker settings panels
**When** the dashboard loads
**Then** the UI follows existing dashboard CSS and JS fetch patterns, loads within 1 second on a local network (NFR7), and uses the same `fetch + json.ok + showToast` pattern as other dashboard sections

---

