# Story ds-3.1: Display Mode API Endpoints

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an external client (web dashboard or third-party tool),
I want HTTP API endpoints to list available modes, retrieve the active mode, and request a mode change,
So that the mode system can be controlled programmatically from any client.

## Acceptance Criteria

1. **Given** `ModeRegistry` is initialized with `MODE_TABLE` (**ds-1.3+**), **When** **`GET /api/display/modes`** runs, **Then** the JSON envelope matches the existing product pattern **`{ "ok": true, "data": { ... } }`** [Source: `architecture.md` Decision D5, existing `WebPortal` handlers] and **`data`** includes at minimum:
   - **`modes`**: array built from **`ModeRegistry::getModeTable()`** / **`getModeCount()`** (or **`enumerate()`**), **not** a hardcoded list — each element has **`id`**, **`name`** (from `ModeEntry.displayName`), and **`zone_layout`**: array of objects **`{ "label", "relX", "relY", "relW", "relH" }`** mirroring **`ZoneRegion`** from **`DisplayMode::getZoneDescriptor()`** for that mode (see AC #2)
   - **`active`**: mode id string from **`ModeRegistry::getActiveModeId()`** (or **`nullptr`** → **`null`** / **`"classic_card"`** fallback — document choice)
   - **`switch_state`**: string derived from **`ModeRegistry::getSwitchState()`** — **`"idle"`** | **`"requested"`** | **`"switching"`** (map **`SwitchState`** enum explicitly)
   - **`upgrade_notification`**: boolean read from NVS (or **`false`** until NVS key exists — implement minimal read alongside **ds-3.2** if key not yet present; default **`false`**)

2. **Given** zone metadata for schematics, **When** serializing a mode entry, **Then** populate **`zone_layout`** from that mode’s **`ModeZoneDescriptor`** (**`regions`**, **`regionCount`**, **`description`** optional as **`description`** on the mode object) — **no** `new DisplayMode` per request: add **`const ModeZoneDescriptor* zoneDescriptor`** (or pointer to static descriptor) to **`ModeEntry`** in **`ModeRegistry.h`** and wire **`MODE_TABLE`** in **`main.cpp`** so **`GET`** walks **BSS/static** data only (**NFR P4**)

3. **Given** **dl-1.5** orchestrator transparency, **When** **`GET /api/display/modes`** responds, **Then** retain **`orchestrator_state`** and **`state_reason`** (or equivalent) **if** **`ModeOrchestrator`** remains in use — do not regress dashboard behavior; ordering inside **`data`** is flexible

4. **Given** **NFR P4** (**<10ms**, **zero heap allocation** for enumeration path), **When** implementing **`GET`**, **Then** avoid **`String`** concatenation in the hot path, prefer **`StaticJsonDocument`** with **documented capacity**, or pre-sized buffer; **do not** allocate **`DisplayMode`** instances per request

5. **Given** **`POST /api/display/mode`**, **When** the body is valid JSON, **Then** accept **`{ "mode": "<id>" }`** per **architecture Decision D5**; **also accept** legacy **`mode_id`** for one release if **`mode`** is absent (backward compatibility with **`dl-1.5`**)

6. **Given** **Rule 24** (orchestrator owns user intent), **When** **`POST`** handles a **manual** switch from the dashboard, **Then** **`ModeOrchestrator::onManualSwitch()`** (or current approved entry point) is invoked — **do not** call **`ModeRegistry::requestSwitch()`** directly from **`WebPortal`** for manual user actions

7. **Given** **architecture D5** “synchronous” semantics vs **cooperative** **`ModeRegistry`**, **When** **`POST`** completes, **Then** the response **must** expose **truthful** registry state: at minimum **`data.switching_to`**, **`data.active`** (or **`active_mode`** alias in **`data`**), **`data.switch_state`**, and **`data.registry_error`** / **`ModeRegistry::getLastError()`** when **`requestSwitch`** fails inside orchestrator path. **If** a **bounded wait** until **`SwitchState::IDLE`** is implemented, cap **≤2000ms**, **`yield`**, and document **watchdog** risk; **if** full synchronous completion is **not** feasible in **`AsyncWebServer`** body handler, document **explicit deviation** and return **`switch_state: "requested"`** with **client re-fetch** (**ds-3.4** UX) — **pick one strategy** in Dev Agent Record and match **`architecture.md`** intent as closely as practicable

8. **Given** unknown mode id, **When** **`POST`** resolves before registry, **Then** **`400`**, **`{ "ok": false, "error": "...", "code": "UNKNOWN_MODE" }`** (or existing **`_sendJsonError`** pattern)

9. **Given** heap guard failure (**`ModeRegistry`** sets error), **When** surfaced through API, **Then** **`{ "ok": false, ... }`** includes a **stable** **`code`** (e.g. **`HEAP_INSUFFICIENT`**) and human-readable **error** string (**FR33**)

10. **Given** **`POST /api/display/notification/dismiss`** (or **`/api/display/ack-notification`** per **architecture**), **When** called with valid session, **Then** **`upgrade_notification`** NVS flag is cleared and **`GET`** returns **`upgrade_notification: false`** thereafter — **namespace** **`flightwall`**, key name **≤15 chars** (coordinate with **ds-3.2** if the key is introduced there; stub no-op is **not** acceptable if **GET** promises the flag)

11. **Given** **`pio run`** on **`esp32dev`**, **Then** firmware builds with **no new warnings**

## Tasks / Subtasks

- [x] Task 1: `ModeEntry` + registry (AC: #2, #4)
  - [x] 1.1: Extend **`ModeEntry`** with **`const ModeZoneDescriptor* zoneDescriptor`** (nullable)
  - [x] 1.2: Point each **`MODE_TABLE`** row at **`&ClassicCardMode::_descriptor`**, **`&LiveFlightCardMode::_descriptor`**, and **clock** mode descriptor when **`ClockMode`** exists (**dl-1.1**) — or **`nullptr`** + **empty** **`zone_layout`** until clock ships
  - [x] 1.3: Verify **`extern` linkage** / **getter** if private statics are inaccessible from **`main.cpp`**

- [x] Task 2: `GET /api/display/modes` (AC: #1, #3, #4, #10)
  - [x] 2.1: Replace hardcoded **`JsonObject mClassic`** blocks in **`_handleGetDisplayModes`** with loop over **`ModeRegistry`**
  - [x] 2.2: Emit **`zone_layout`** from **`ModeZoneDescriptor`**
  - [x] 2.3: Set **`switch_state`** from **`ModeRegistry::getSwitchState()`**
  - [x] 2.4: Implement **`upgrade_notification`** NVS read (or shared helper with **ds-3.2**)

- [x] Task 3: `POST /api/display/mode` (AC: #5–#9)
  - [x] 3.1: Parse **`mode`** with fallback to **`mode_id`**
  - [x] 3.2: Orchestrator-only path; propagate **registry** errors into JSON if orchestrator exposes them
  - [x] 3.3: Resolve **sync vs async** response per AC #7 and document

- [x] Task 4: Notification dismiss route (AC: #10)
  - [x] 4.1: Register **`POST /api/display/notification/dismiss`** or **`/api/display/ack-notification`** (match **architecture** / **epic-ds-3** — one canonical path)
  - [x] 4.2: Clear NVS flag; return **`{ ok: true }`**

- [x] Task 5: Assets / consumers (AC: #11)
  - [x] 5.1: If JSON field names change (**`active`** vs **`active_mode`**), update **`data-src/dashboard.js`** in same story **or** log follow-up for **ds-3.3** — avoid breaking **`dl-1.5`** dashboard

## Dev Notes

### Existing code

- **`WebPortal::_handleGetDisplayModes`** — **`firmware/adapters/WebPortal.cpp`** ~915+ (**hardcoded** modes, **`switch_state`** stub **`"idle"`**)
- **`POST /api/display/mode`** — **`mode_id`**, **`ModeOrchestrator::onManualSwitch`**
- **`ConfigManager::getDisplayMode` / `setDisplayMode`** — **`disp_mode`** key (may differ from **architecture** **`display_mode`** — **align** with **ds-3.2** naming)

### Epic ↔ architecture naming

- Epic snippet uses top-level **`modes`** — product uses **`data.modes`** — follow **product** [Source: **dl-1.5** implementation]
- **`getZoneLayout()`** in epic → **`getZoneDescriptor()`** / **`zone_layout`** JSON [Source: **`interfaces/DisplayMode.h`**]

### Out of scope (later ds-3 stories)

- **ds-3.2** — full NVS boot restore policy if not already complete
- **ds-3.3** — Mode Picker cards / CSS grid
- **ds-3.4** — switching animation UX
- **ds-3.6** — banner UI (may depend on **`upgrade_notification`** from this story)

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.1)
- Architecture: `_bmad-output/planning-artifacts/architecture.md` — **D5**, **D6**, **Rule 24**
- Prior: **`dl-1-5`** story file (`orchestrator-state-feedback`)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

### Implementation Plan

**Sync vs Async Strategy (AC #7):** Chose **async response** with `switch_state: "requested"` and client re-fetch (ds-3.4 UX pattern). ESPAsyncWebServer body handlers run on the AsyncTCP task while `ModeRegistry::tick()` runs on Core 0 display task — a bounded wait would block the web server and risk watchdog timeout. This is an explicit deviation from architecture D5 "synchronous" intent, documented per AC #7.

**NVS key for upgrade_notification (AC #10):** `upg_notif` in namespace `flightwall` (8 chars, within 15-char limit). Coordinated for ds-3.2 reuse.

**Active mode fallback:** When `ModeRegistry::getActiveModeId()` returns `nullptr` (before first tick), falls back to `"classic_card"` string literal.

### Completion Notes List

- Task 1: Added `const ModeZoneDescriptor* zoneDescriptor` to `ModeEntry` struct. Made `_descriptor` public in both ClassicCardMode and LiveFlightCardMode so main.cpp can reference `&ClassicCardMode::_descriptor` etc. Updated MODE_TABLE and all test mode tables.
- Task 2: Rewrote `_handleGetDisplayModes` to loop over `ModeRegistry::getModeTable()` instead of hardcoded mode blocks. Emits `zone_layout` array from static `ModeZoneDescriptor`, maps `SwitchState` enum to string, reads `upgrade_notification` from NVS, retains `orchestrator_state`/`state_reason` for dl-1.5 compatibility, surfaces `registry_error` when present.
- Task 3: Updated POST handler to accept `mode` field with fallback to `mode_id` (AC #5). Mode name resolution now uses ModeRegistry lookup instead of hardcoded if-chain. Response includes truthful `active`, `switch_state`, `orchestrator_state`, `state_reason`, and `registry_error` fields.
- Task 4: Registered `POST /api/display/notification/dismiss` route that clears NVS `upg_notif` flag and returns `{ ok: true }`.
- Task 5: No dashboard.js changes needed — all existing field names (`active`, `modes`, `state_reason`, `orchestrator_state`) preserved. `mode_id` POST field still accepted via fallback. New fields (`zone_layout`, `switch_state`, `upgrade_notification`, `registry_error`) are additive-only.

### File List

- `firmware/core/ModeRegistry.h` — Added `zoneDescriptor` field to `ModeEntry` struct
- `firmware/modes/ClassicCardMode.h` — Made `_descriptor` public for MODE_TABLE access
- `firmware/modes/LiveFlightCardMode.h` — Made `_descriptor` public for MODE_TABLE access
- `firmware/src/main.cpp` — Updated MODE_TABLE entries with descriptor pointers
- `firmware/adapters/WebPortal.cpp` — Rewrote GET /api/display/modes, updated POST /api/display/mode, added POST /api/display/notification/dismiss, added ModeRegistry.h include
- `firmware/test/test_mode_registry/test_main.cpp` — Updated test mode tables with zoneDescriptor field

## Change Log

- **2026-04-14:** Replaced hardcoded display mode catalog with ModeRegistry-driven API. Added zone_layout metadata, real switch_state, upgrade_notification NVS flag, and notification dismiss endpoint. POST now accepts both `mode` and `mode_id` fields. All existing test suites build cleanly, no new warnings.

## Previous story intelligence

- **dl-1.5** shipped **GET/POST** scaffolding; **ds-3.1** replaces **stub** catalog and **`switch_state`** with **`ModeRegistry`** truth.
- **ModeOrchestrator** integration must stay **Rule-24**-compliant.

## Git intelligence summary

Primary edits: **`firmware/core/ModeRegistry.h`**, **`firmware/src/main.cpp`**, **`firmware/adapters/WebPortal.cpp`**.

## Latest tech information

- **ArduinoJson** on ESP32: prefer **`StaticJsonDocument<N>`** with **capacity** sized for worst-case mode list.

## Project context reference

After **`data-src/`** edits, regenerate **`.gz`** per **`AGENTS.md`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
