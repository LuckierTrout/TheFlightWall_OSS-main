---
inputDocuments: ['_bmad-output/planning-artifacts/prd.md', '_bmad-output/planning-artifacts/prd-foundation.md', '_bmad-output/planning-artifacts/implementation-readiness-report-display-system-2026-04-11.md', 'docs/index.md', 'docs/project-overview.md', 'docs/architecture.md', 'docs/source-tree-analysis.md', 'docs/component-inventory.md', 'docs/development-guide.md', 'docs/api-contracts.md', 'docs/setup-guide.md', 'docs/device-setup-and-flash-guide.md', 'docs/device-setup-and-flash-printable.md']
documentCounts:
  briefs: 0
  research: 0
  brainstorming: 0
  projectDocs: 12
workflowType: 'prd'
projectType: 'brownfield'
classification:
  projectType: iot_embedded
  domain: general
  complexity: medium-high
  projectContext: brownfield
lastEdited: '2026-04-11'
editHistory:
  - date: '2026-04-11'
    changes: 'Edit after validation: reduced FR/NFR implementation leakage; tightened FR12 and NFR C3/C4; added IoT power profile and companion-web scope note; aligned MVP bullets and journey summary with capability-oriented wording.'
  - date: '2026-04-11'
    changes: 'Post re-validation (Fix simpler items): FR11/FR13 baseline wording; FR30/FR31 and MVP #6 reference documented onboard display-mode API; FR35 pipeline-only refresh; NFR C1/C4 de-HOWed; IoT connectivity and NFR P4 wording aligned.'
  - date: '2026-04-11'
    changes: 'PRD edit from implementation-readiness report: FR7 serialized switch with at-most-one pending (latest wins); FR32 synchronous activate + client in-flight state; normative upgrade notification contract (localStorage flightwall_mode_notif_seen, POST /api/display/ack-notification, upgrade_notification flag).'
stepsCompleted:
  - step-01-init
  - step-02-discovery
  - step-02b-vision
  - step-02c-executive-summary
  - step-03-success
  - step-04-journeys
  - step-05-domain-skipped
  - step-06-innovation-skipped
  - step-07-project-type
  - step-08-scoping
  - step-09-functional
  - step-10-nonfunctional
  - step-11-polish
  - step-12-complete
  - step-e-01-discovery
  - step-e-02-review
  - step-e-03-edit
---

# Product Requirements Document - TheFlightWall Display System Release

**Author:** Christian
**Date:** 2026-04-11

## Executive Summary

TheFlightWall is an ESP32-powered LED wall that displays live flight information for aircraft near a configurable location. The existing firmware — covered by the MVP PRD and Foundation Release PRD — delivers a complete web configuration portal, zone-based display with airline logos, OTA firmware updates, night mode scheduling, and onboarding calibration. That system is built (MVP) and planned (Foundation).

Today, the display is a single hardcoded rendering path: fetch flights, draw three-line cards (airline, route, aircraft), cycle through them. The LED matrix and zone-based layout engine are capable of far more, but every visual change requires a firmware code edit. This Display System Release closes that gap with three capabilities:

1. **DisplayMode Abstraction** — Extract the rendering pipeline from `NeoMatrixDisplay` into a pluggable display-mode model. Each mode receives flight data, owns its rendering logic and memory within allocated display zones, and releases resources when torn down. A mode registry manages activation order: the outgoing mode is fully torn down before the incoming mode is initialized. The set of modes shipped in firmware is fixed at build time; the active mode is switchable at runtime without reboot. This is the architectural foundation that makes every future visual feature a mode drop-in, not a firmware rewrite.

2. **Live Flight Card Mode** — The first mode built on the new abstraction. Renders enriched flight data using telemetry fields already present in `StateVector` but currently unused: altitude, ground speed, heading, and vertical rate. Replaces the static three-line card with a denser, more informative layout that exploits the full zone canvas.

3. **Mode Picker UI** — A new section in the web dashboard for selecting and configuring display modes at runtime. Users browse available modes, view a schematic representation of each mode's layout, activate one, and adjust mode-specific settings — all from their phone. This is closer to a remote control than a settings panel: designed for frequent, casual interaction rather than one-time configuration. The device boots into the last-active mode persisted in NVS, defaulting to Classic Card on first run after upgrade to preserve pre-upgrade behavior, with a one-time dashboard notification surfacing the new modes.

### What Makes This Special

The hardware is already capable. Twenty 16x16 LED panels driven by an ESP32 with a zone-based layout engine can render far more than flight tickers. The missing piece is software architecture — a clean abstraction boundary between "what data do we have" and "how do we visualize it." This release draws that line. Once the mode interface exists, adding a clock mode, a radar sweep, an arrivals board, or animated transitions becomes a contained exercise with no touch to the data pipeline or display infrastructure. The wall stops being a flight ticker and becomes a display platform.

## Project Classification

| Attribute | Value |
|-----------|-------|
| **Project Type** | IoT / Embedded (ESP32, WS2812B LED matrix) |
| **Domain** | General (no regulatory compliance) |
| **Complexity** | Medium-High (plugin architecture on memory-constrained MCU) |
| **Project Context** | Brownfield (extends MVP + Foundation Release) |
| **Companion surface** | On-device web dashboard (Mode Picker; same ESP32-hosted portal model as MVP) |

## Success Criteria

### User Success

- User can switch between display modes from their phone and see the wall respond within 2 seconds (including brief blank transition during teardown/init)
- User can view enriched flight data — altitude, ground speed, heading, vertical rate — on the Live Flight Card without losing the information density of the classic card
- User does not need to understand the mode system to use it — the Mode Picker presents available modes with clear labels and schematic previews showing a labeled wireframe of each mode's zone layout with content regions identified
- The wall "just works" after a mode switch — no reboot, no garbled output, no stale data from the previous mode
- After upgrading from Foundation Release, the wall continues displaying Classic Card (preserving current behavior) and the dashboard surfaces a one-time notification: "New display modes available"

### Business Success

- The mode abstraction is proven extensible: a third-party or future developer can add a new display mode by implementing a single interface and registering it in the mode table — no changes to the data pipeline, display infrastructure, or web portal required
- The Foundation Release OTA pipeline can deliver new modes to deployed devices — mode additions are firmware updates, not architectural changes
- Total time from "idea for a new mode" to "mode running on the wall" is measured in hours of development, not days of refactoring

### Technical Success

- Mode switching completes a full lifecycle (teardown old → init new → first render) without heap fragmentation or memory leaks across 100+ consecutive switches
- Mode registry serializes switch requests — if a switch is in progress, at most one pending request is held and a newer request replaces an older pending one (latest wins); switches are never double-executed
- Mode registry validates that sufficient heap exists using each mode's reported worst-case allocation after teardown, before activating the incoming mode; returns an error to the Mode Picker UI if insufficient
- The mode registry does not allocate its own control structures from the heap; the shipped mode list is fixed at firmware build time
- Live Flight Card renders all four telemetry fields (altitude, speed, heading, vertical rate) on a standard 160x32 matrix; gracefully drops fields in priority order (vertical rate first, then speed) on smaller configurations
- Mode Picker UI loads and renders the mode list within 1 second on the ESP32's web server
- Active mode selection persists in NVS and survives power cycles

### Measurable Outcomes

| Outcome | Target | Measurement |
|---------|--------|-------------|
| Mode switch latency | < 2 seconds (user-perceived, including blank transition) | Timestamp from API call to first render of new mode |
| Heap stability | No net heap loss after 100 mode switches | Free heap before first switch vs. after 100th switch |
| Rapid-switch safety | Zero undefined states after 10 rapid toggles in 3 seconds | Automated switch stress test |
| Mode Picker load time | < 1 second | Time from dashboard navigation to mode list rendered |
| Telemetry field coverage | 4/4 fields on 160x32, 2/4 minimum on smaller panels | Visual inspection per panel configuration |
| NVS persistence | Last-active mode restored on boot | Power cycle test |
| Heap guard | Mode activation refused with error when memory insufficient | Simulated low-heap test |

## Product Scope

### MVP Strategy & Philosophy

**MVP Approach:** Platform MVP — prove the abstraction layer works by shipping two modes (Classic Card migration + Live Flight Card) with a Mode Picker UI. If a user can switch modes from their phone and a developer can add a new mode in hours, the architecture is validated.

**Resource Requirements:** Solo developer (project owner). No external dependencies beyond the existing Foundation Release codebase.

### MVP — This Release

**Core User Journeys Supported:** All six (J1–J6)

**Must-Have Capabilities:**
1. Pluggable display-mode contract (lifecycle and memory reporting without dictating class layout in the PRD)
2. Mode registry listing all modes shipped in firmware, with serialized activation and heap guard
3. Classic Card mode (migration from existing rendering — J6)
4. Live Flight Card mode (telemetry-enriched rendering)
5. Mode Picker UI with schematic previews and transition feedback
6. Mode switch API (onboard read/list and activate operations for display mode, documented in release API documentation)
7. NVS persistence of active mode
8. Heap guard with graceful failure and previous mode restoration
9. Switch serialization for rapid-switch safety
10. Rendering parity validation for Classic Card migration

**Explicitly Deferred:**
- Mode-specific user settings
- Animated transitions (blank transition only)
- Mode scheduling
- Server-rendered preview thumbnails

### Growth Features (Post-MVP)

- Mode-specific settings (e.g., Live Flight Card could let user choose which telemetry fields to show)
- Animated transitions between modes (fade, wipe, scroll) instead of blank
- Mode scheduling — automatically switch modes at configured times (e.g., clock mode at night)
- Mode preview thumbnails rendered server-side as bitmap snapshots

### Vision (Future)

- Community mode library — downloadable modes from a shared repository delivered via OTA
- User-configurable mode: drag-and-drop zone assignment of data fields
- Multi-mode split screen — different modes in different zones simultaneously

### Risk Mitigation Strategy

**Technical Risks:**
- **Highest risk: Classic Card migration (J6)** — Extracting rendering from `NeoMatrixDisplay` without visual regression. Mitigation: photograph pre-migration output, compare pixel-by-pixel post-migration. Do this first — every other feature depends on it.
- **Heap fragmentation** — Repeated mode switches could fragment heap on ESP32's non-compacting allocator. Mitigation: 100-switch stress test in measurable outcomes; modes must use fixed-size heap allocations.
- **Stack overflow in per-frame drawing** — Silent corruption from large stack-local buffers. Mitigation: stack discipline requirement in IoT technical section; code review gate.
- **NVS key collision** — Foundation Release already uses NVS for config keys (wifi_ssid, brightness, timezone, etc.). The new `display_mode` key must use a distinct NVS namespace or verified key prefix to avoid colliding with existing Foundation keys.
- **Critical path:** J6 migration → DisplayMode interface → Mode registry → Classic Card mode → Live Flight Card mode → Mode Picker UI → NVS persistence. Heap guard can be developed in parallel; GET endpoint must precede Mode Picker UI development.

**Market Risks:** None — personal project, single user. The "market" is validated by the fact the wall is already mounted and running.

## User Journeys

### Journey 1: Mode Discovery (Post-Upgrade)

Christian has been running the Foundation Release for a week. OTA works, night mode dims the wall at 11pm, the onboarding wizard confirmed his panel layout on first boot. The wall shows the same three-line flight cards it always has — airline, route, aircraft — cycling every few seconds.

He opens the dashboard on his phone to check the health page and notices a banner he hasn't seen before: "New display modes available." He taps it and lands on a new Mode Picker section. Two cards are displayed — Classic Card (marked "Active") and Live Flight Card. Each shows a schematic wireframe: Classic has three labeled rows (Airline, Route, Aircraft); Live Flight Card has a denser layout with six labeled regions (Airline, Route, Altitude, Speed, Heading, VRate).

He taps "Activate" on Live Flight Card. The Mode Picker shows "Switching..." while the wall goes briefly blank — maybe half a second — then lights up with a richer card. The Mode Picker updates to show Live Flight Card as "Active." The flight overhead now shows "UAL 1829" with altitude at 34,200ft, ground speed 487kt, heading 275, and a climb arrow. He watches a few more flights cycle through. The telemetry data makes each one feel distinct — descending arrivals versus cruising overflights versus climbing departures. The wall just got more interesting.

The banner doesn't appear again on subsequent visits. The Mode Picker remains accessible from the dashboard navigation at all times — whether the notification was tapped, dismissed, or never seen.

**Capabilities revealed:** Mode Picker UI, schematic wireframe previews, mode activation API, transition state feedback, one-time upgrade notification, Mode Picker discoverability, Live Flight Card rendering, brief blank transition

### Journey 2: Casual Mode Switching

Friends are over on a Saturday. Someone asks about the wall. Christian pulls out his phone, opens the Mode Picker, and says "watch this." He taps Classic Card — the Mode Picker shows "Switching..." for a beat, then updates to "Active: Classic Card" as the wall transitions to the familiar three-line layout. He taps Live Flight Card — "Switching...", brief blank, then the enriched telemetry view appears and the UI confirms "Active: Live Flight Card." Back and forth a couple times.

The wall responds every time within about a second. No crashes, no garbled frames, no "still loading" states. The Mode Picker always reflects the true state — "Switching..." during the transition, "Active" only after the firmware confirms the new mode is rendering. The mode switching feels like changing channels — immediate and reliable. One friend taps back and forth rapidly on Christian's phone. The wall doesn't glitch — each switch completes cleanly before the next one starts.

**Capabilities revealed:** Switch serialization, rapid-switch safety, consistent teardown/init lifecycle, sub-2-second perceived latency, transition state feedback in UI, confirmed-active state synchronization

### Journey 3: Persistence & Power Cycle

Christian activates Live Flight Card and leaves it running. He prefers the telemetry view for daily use. Three days later, a thunderstorm knocks out power for twenty minutes.

Power returns. The ESP32 boots, connects to WiFi, syncs NTP, and begins fetching flights. The wall comes up displaying Live Flight Card — exactly where he left it. No dashboard visit needed. The NVS-stored mode preference survived the power cycle.

He opens the dashboard later and confirms: Mode Picker shows Live Flight Card as "Active." Everything consistent.

**Capabilities revealed:** NVS persistence of active mode, boot sequence mode restoration, consistency between hardware state and dashboard state

### Journey 4: Error Recovery (Heap Guard)

Six months later, Christian has added a third display mode — a radar sweep visualization that renders a polar coordinate grid with flight positions plotted by bearing and distance. It's memory-hungry: the polar-to-cartesian lookup table alone needs 8KB.

He opens the Mode Picker and taps "Activate" on Radar Sweep. The Mode Picker shows "Switching..." The mode registry tears down Live Flight Card, checks available heap, and finds it's below the threshold reported by Radar Sweep's `getMemoryRequirement()`. The registry does not call `init()`. Instead, it re-initializes Live Flight Card (the previous mode) and returns an error to the Mode Picker.

The Mode Picker shows: "Unable to activate Radar Sweep — insufficient memory. Current mode restored." The wall never went dark, never crashed, never showed garbage. The failure was clean and communicated.

Christian checks the serial log later and sees: `[ModeRegistry] Heap guard: need 8192, available 6104. Rejecting mode switch. Restoring previous mode.`

**Capabilities revealed:** Heap validation via `getMemoryRequirement()`, graceful failure with previous mode restoration, error propagation to Mode Picker UI, clean recovery without reboot

### Journey 5: Developer Extensibility (Adding a New Mode)

A contributor wants to add a "Departures Board" mode that lists the last N flights in an airport-style scrolling format. They clone the repo and look at the mode system.

They create a new class `DeparturesBoardMode` that extends the `DisplayMode` interface — implementing `init()`, `render(flights)`, `teardown()`, and `getMemoryRequirement()`. The init allocates a scrolling text buffer; render formats each flight as a single line and scrolls them vertically; teardown frees the buffer. `getMemoryRequirement()` returns the buffer size.

They add one line to the static mode table in the registry — a name string, a factory reference, and a priority/order value. No changes to `NeoMatrixDisplay`, `FlightDataFetcher`, `WebPortal`, or any other component. They build, flash via OTA, and the new mode appears in the Mode Picker automatically because the Mode Picker reads the registry's mode list dynamically.

Total touch points: one new `.h/.cpp` file pair, one line in the mode table. Time from idea to working mode on the wall: a few hours.

**Capabilities revealed:** Single-interface extensibility, static registration simplicity, zero coupling to data pipeline or web portal, automatic Mode Picker discovery from registry

### Journey 6: Migration — Classic Card Extraction

A developer begins the Display System Release by extracting the existing rendering logic from `NeoMatrixDisplay::displayFlights()` into a new `ClassicCardMode` class. This is the highest-risk task in the release — every other journey depends on it.

They create `ClassicCardMode` implementing the `DisplayMode` interface. The `render(flights)` method contains the exact rendering logic currently in `displayFlights()` — bordered three-line card with airline name, route (origin > destination), and aircraft type, cycling through flights on a configurable interval. `init()` sets up the same text color and cursor state that `displayFlights()` uses today. `teardown()` clears the matrix. `getMemoryRequirement()` returns a minimal footprint — Classic Card uses no dynamic buffers beyond what GFX provides.

They remove the rendering logic from `NeoMatrixDisplay`, replacing `displayFlights()` with a delegation to the mode registry's active mode. The mode registry is initialized with Classic Card as the default.

The critical validation: the wall must render **pixel-identical output** to the pre-migration firmware. Same card layout, same font, same colors, same border, same cycling timing. The developer captures a photograph or serial log of the pre-migration output and compares it to the post-migration output across multiple flight cards. Any visual discrepancy is a regression — the migration is not complete until parity is confirmed.

**Capabilities revealed:** Rendering parity validation, safe extraction of existing logic into mode interface, NeoMatrixDisplay delegation to mode registry, zero visual regression guarantee

### Journey Requirements Summary

| Capability Area | Journeys | Key Requirements |
|----------------|----------|-----------------|
| **DisplayMode Interface** | J1, J2, J3, J4, J5, J6 | Pluggable mode contract (activation, drawing, teardown, reported memory need); self-contained rendering within allocated zones |
| **Mode Registry** | J1, J2, J4, J5, J6 | Registry of shipped modes; lifecycle management; switch serialization; heap validation; automatic enumeration for UI |
| **Classic Card Mode** | J1, J2, J6 | Existing three-line card migrated to mode interface; pixel-identical rendering to pre-upgrade behavior |
| **Live Flight Card Mode** | J1, J2, J3 | Telemetry fields (altitude, speed, heading, vertical rate); adaptive field dropping; denser layout |
| **Mode Picker UI** | J1, J2, J4 | Mode list with schematic wireframe previews; activate button; active indicator; "Switching..." transition state; error display; one-time upgrade notification; always accessible from dashboard navigation |
| **NVS Persistence** | J3 | Store/restore active mode across power cycles; default to Classic Card on first run after upgrade |
| **Mode Switch API** | J1, J2, J4 | GET current/available modes; POST to activate; transition state (initiated vs. confirmed); error responses for heap guard failures |
| **Heap Guard** | J4 | Validate memory before init; restore previous mode on failure; propagate error to UI |
| **Switch Serialization** | J2 | Serialize switches; at most one pending during transition (latest wins); prevent double-execution |
| **Rendering Parity Validation** | J6 | Pixel-identical output verification between pre-migration and post-migration Classic Card rendering |

## IoT/Embedded Technical Requirements

### Hardware Platform

ESP32 with 20x WS2812B 16x16 LED panels (160x32 pixel matrix). No new hardware dependencies in this release — all capabilities are software-only, running on the existing platform defined in the MVP PRD.

### Power Profile

Unchanged from the MVP PRD: wall-powered operation; typical brightness-limited use stays within the existing supply assumptions. This release does not introduce new power states or battery operation.

### Companion web UI (in scope)

The Mode Picker is part of the same on-device dashboard served from the ESP32 (LittleFS), consistent with the MVP web portal. Companion UI for mode selection is intentional product scope alongside LED output, not an accidental add-on.

### Memory & Resource Constraints

The mode system operates within the ESP32's available heap after WiFi stack, HTTP server, JSON parser, and data pipeline are resident. Key constraints:

- **Mode registry** — Static allocation only. Mode table, active mode pointer, and serialized pending-switch state (at most one pending request) stored in BSS/data segments, not heap
- **Mode instances** — Each mode owns its heap allocation. The mode reports worst-case allocation before activation; the registry validates available heap exceeds this threshold before activating a mode
- **Concurrent allocation ceiling** — Only one mode is active at any time. During a switch, the outgoing mode must finish teardown and free all heap before the incoming mode allocates. No overlapping mode allocations
- **Heap headroom** — Mode activation must leave sufficient heap for concurrent WiFi (~30KB), HTTP serving, and flight data parsing (ArduinoJson deserialization spikes of 8-12KB for large state vector responses). Target floor of 30KB free heap after mode activation, validated empirically during integration testing
- **Stack discipline** — Per-frame mode drawing runs on the main loop stack (8KB default on ESP32). Large buffers (text formatting, flight sorting, lookup tables) must be heap-allocated at mode activation and reused across drawing calls. Stack-local work per frame must stay minimal to prevent silent stack corruption

### Connectivity

No new connectivity requirements. The Mode Picker UI and onboard display-mode HTTP API are served by the existing ESP32 HTTP server on the local WiFi network. All mode switching is local — no cloud dependencies.

### Security Model

Mode switch API endpoints are accessible to any device on the local network, consistent with the existing dashboard security model. No authentication layer required for this release — same trust model as all other `/api/*` endpoints.

### Update Mechanism

New display modes are delivered as part of firmware updates through the Foundation Release OTA pipeline. No mode-specific OTA or hot-loading mechanism. Adding a mode requires a firmware build and OTA push.

### Implementation Considerations

- **Cooperative scheduling** — Mode rendering stays on the existing cooperative main-loop model: no new FreeRTOS tasks dedicated to mode rendering, no interrupt-driven frame push, and no blocking waits in the per-frame display path. Stricter concurrency choices are implementation details; the compatibility bar is stated in NFR C3.
- **NeoMatrixDisplay refactoring** — The existing `displayFlights()` method is replaced with delegation to the mode registry's active mode. `NeoMatrixDisplay` retains ownership of the FastLED/GFX hardware interface and the `FastLED.show()` call — modes write to the pixel buffer through GFX primitives but do not push frames. The main loop calls `show()` after `render()` returns, preserving cooperative multitasking timing
- **Rendering context** — Modes receive a rendering context containing: (a) reference to the `Adafruit_NeoMatrix` object for GFX drawing primitives, (b) zone origin and dimensions, and (c) current brightness level as read-only — modes must not override brightness, which is managed by the Foundation Release night mode scheduler
- **Flight data pipeline unchanged** — `FlightDataFetcher` continues to produce the same `vector<FlightInfo>` output. The mode registry passes this data to the active mode's `render()` method. No changes to fetch, enrich, or data model layers

## Functional Requirements

### DisplayMode Interface

- **FR1:** Device can render flight data through interchangeable display modes that support activation, per-frame drawing, teardown, and a reported worst-case heap requirement before activation
- **FR2:** Device can report the memory requirement of a display mode before activation
- **FR3:** Display mode can receive a rendering context containing the matrix object, zone bounds, and current brightness level
- **FR4:** Display mode can render flight data within its allocated zone bounds without affecting pixels outside its zone

### Mode Registry & Lifecycle

- **FR5:** Device can maintain a registry of all display modes shipped in the firmware build
- **FR6:** Device can activate a display mode by executing its full lifecycle: teardown current mode, validate heap, initialize new mode, begin rendering
- **FR7:** Device can serialize concurrent mode switch requests: if a switch is already in progress, at most one additional request may be pending, and a newer request replaces any previous pending request (latest wins); mode switches never execute in parallel
- **FR8:** Device can validate that sufficient free heap exists before activating a requested mode
- **FR9:** Device can restore the previous mode when a requested mode activation fails due to insufficient heap
- **FR10:** Device can enumerate all registered modes with their names for external consumers (API, UI)

### Classic Card Mode

- **FR11:** Device can render a three-line flight card (airline name, route, aircraft type) pixel-identical to the Classic Card baseline used for migration acceptance (pre–Display System Release behavior recorded for parity testing)
- **FR12:** When two or more flights are available, device can cycle through them at a configurable interval in Classic Card mode
- **FR13:** Classic Card mode satisfies the same pixel-parity acceptance criteria as FR11

### Live Flight Card Mode

- **FR14:** Device can render an enriched flight card displaying airline, route, altitude, ground speed, heading, and vertical rate
- **FR15:** Device can adaptively drop telemetry fields in priority order (vertical rate first, then speed) when zone dimensions are insufficient for the full layout
- **FR16:** Device can indicate flight vertical direction (climbing, descending, level) as a distinct visual element in the Live Flight Card

### Mode Picker UI

- **FR17:** User can view a list of all available display modes in the web dashboard
- **FR18:** User can view a schematic wireframe preview of each mode's zone layout with labeled content regions
- **FR19:** User can activate a display mode by selecting it in the Mode Picker
- **FR20:** User can see the currently active mode indicated in the Mode Picker
- **FR21:** User can see a "Switching..." transition state in the Mode Picker during a mode change
- **FR22:** User can see the Mode Picker update to "Active" only after the firmware confirms the new mode is rendering
- **FR23:** User can see an error message in the Mode Picker when a mode activation fails (e.g., insufficient memory)
- **FR24:** User can access the Mode Picker from the dashboard navigation at all times
- **FR25:** User can see a one-time "New display modes available" notification in the dashboard after upgrading from Foundation Release
- **FR26:** User can dismiss the upgrade notification, and it does not reappear on subsequent visits

### Upgrade notification contract (normative)

Dual-source dismissal for FR25–FR26; UX, architecture, epics, and release API documentation must match:

- Firmware includes an `upgrade_notification` boolean in the display-modes read/list response when the user should be offered the one-time banner (first boot after upgrade from Foundation Release until acknowledged).
- The dashboard suppresses the banner when browser `localStorage` key **`flightwall_mode_notif_seen`** is set; the banner is shown only when the firmware flag is true **and** that key is unset.
- Clearing the firmware-side notification uses **`POST /api/display/ack-notification`**; dismissal actions on the client set `flightwall_mode_notif_seen` and call this endpoint so either source independently prevents the banner from returning inappropriately.

### Mode Persistence

- **FR27:** Device can store the active display mode selection in NVS
- **FR28:** Device can restore the last-active display mode from NVS on boot
- **FR29:** Device can default to Classic Card mode on first boot after upgrading from Foundation Release (preserving pre-upgrade behavior)

### Mode Switch API

- **FR30:** External client can retrieve the current active mode and list of available modes via the onboard display-mode HTTP API (read/list operation; request and response shape documented in release API documentation)
- **FR31:** External client can request a mode change via the onboard display-mode HTTP API (activate operation; request and response shape documented in release API documentation)
- **FR32:** Mode change uses a synchronous HTTP activate operation: the API response reflects a terminal outcome (success after the new mode is rendering, or failure with reason). The client may show an in-flight state while awaiting the response; no polling loop or separate API "initiated" state is required
- **FR33:** API can return an error response with reason when a mode switch fails

### Display Infrastructure

- **FR34:** Device can render all flight output through the currently active display mode
- **FR35:** Device can ensure only the shared display pipeline commits the matrix each frame: modes draw into the shared buffer and do not perform their own full-matrix refresh or re-entrant refresh from within mode rendering
- **FR36:** Device can always retain at least one valid active display mode (no empty or undefined "no mode" state after a completed switch or failed activation recovery)

## Non-Functional Requirements

### Performance

- **NFR P1:** Mode switch must complete the full lifecycle (teardown → heap check → init → first render) in under 2 seconds as perceived by the user
- **NFR P2:** Per-frame display work for the active mode must complete within 16ms (60fps budget) and must be non-blocking: no deliberate busy-waits, no network I/O, and no filesystem I/O on the hot path
- **NFR P3:** Mode Picker UI page must load and render the complete mode list within 1 second on the ESP32's HTTP server
- **NFR P4:** Mode registry enumeration (for the read/list API response) must complete in under 10ms with no heap allocation

### Stability & Memory Safety

- **NFR S1:** Mode switching must produce no net heap loss after 100 consecutive switches between any two registered modes
- **NFR S2:** Rapid mode switching (10 toggles in 3 seconds) must not produce undefined display states, heap corruption, or watchdog resets
- **NFR S3:** Stack-local allocations during per-frame mode drawing must not exceed 512 bytes total per call frame — larger buffers must be allocated at mode activation and reused across frames
- **NFR S4:** Mode activation must leave a minimum of 30KB free heap after initialization, validated empirically during integration testing
- **NFR S5:** Modes must not override the current brightness level — brightness is read-only in the rendering context, managed exclusively by the Foundation Release night mode scheduler
- **NFR S6:** The `display_mode` NVS key must use a namespace or prefix that does not collide with existing Foundation Release NVS keys

### Compatibility

- **NFR C1:** Classic Card mode must produce pixel-identical output to the Classic Card baseline — verified by visual comparison across at least 5 distinct flight cards against baseline captures or the parity checklist
- **NFR C2:** The mode system must not interfere with Foundation Release features. Verified by confirming all of the following while a display mode is active:
  - OTA firmware upload completes successfully
  - Night mode brightness changes on schedule
  - NTP time sync continues to function
  - Dashboard settings can be read and written
  - Health monitoring endpoint responds normally
- **NFR C3:** The mode system must preserve the product's existing cooperative scheduling behavior: no new concurrent rendering tasks beyond the current main-loop pattern, and no blocking delays on the display hot path (as measured by meeting NFR P2 during mode switches and steady-state operation)
- **NFR C4:** Mode Picker UI must meet all of the following, verifiable in the shipped dashboard: (a) reachable from the same primary navigation pattern as other dashboard sections, (b) visual styling and card layout conventions match adjacent settings sections without introducing a separate design system, (c) no third-party script runtime or module loader beyond what the MVP dashboard already ships — same client-side technology pattern as existing dashboard pages
