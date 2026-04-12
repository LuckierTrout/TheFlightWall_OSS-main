---
stepsCompleted: ['step-01-init', 'step-02-discovery', 'step-02b-vision', 'step-02c-executive-summary', 'step-03-success', 'step-04-journeys', 'step-05-domain-skipped', 'step-06-innovation-skipped', 'step-07-project-type', 'step-08-scoping', 'step-09-functional', 'step-10-nonfunctional', 'step-11-polish', 'step-12-complete']
inputDocuments: ['_bmad-output/planning-artifacts/prd.md', '_bmad-output/planning-artifacts/prd-foundation.md', '_bmad-output/planning-artifacts/prd-display-system.md', '_bmad-output/planning-artifacts/architecture.md', 'docs/index.md', 'docs/architecture.md', 'docs/component-inventory.md', 'docs/project-overview.md', 'docs/source-tree-analysis.md', 'docs/development-guide.md', 'docs/api-contracts.md', 'docs/setup-guide.md', 'docs/device-setup-and-flash-guide.md', 'docs/device-setup-and-flash-printable.md']
documentCounts:
  briefs: 0
  research: 0
  brainstorming: 0
  projectDocs: 14
workflowType: 'prd'
projectType: 'brownfield'
classification:
  projectType: iot_embedded
  domain: general
  complexity: medium
  projectContext: brownfield
date: '2026-04-11'
---

# Product Requirements Document - TheFlightWall Delight Release

**Author:** Christian
**Date:** 2026-04-11

## Executive Summary

TheFlightWall Delight Release transforms the display wall from a single-purpose flight tracker into an ambient intelligence device — the wall reads the room. Building on the Display System Release's pluggable mode architecture, this release ships the first alternative display modes, teaches the wall to transition between them gracefully, and adds user-initiated firmware updates from GitHub Releases.

### Capabilities

**Display Delight** (5 capabilities)

| # | Capability | Description |
|---|-----------|-------------|
| 1 | **Clock Mode** | Large-format time display that serves as the wall's idle state. When the flight data pipeline returns zero aircraft in range, the wall falls back to Clock Mode automatically — a data-driven idle fallback, distinct from time-based mode scheduling. Never shows a blank or loading screen. |
| 2 | **Departures Board** | Airport-style scrolling list showing multiple flights simultaneously. Rows update in real-time as aircraft enter and leave the tracking radius. |
| 3 | **Animated Transitions** | Smooth visual transitions (fade, wipe, scroll) when switching between display modes, replacing hard cuts with polished handoffs. |
| 4 | **Mode Scheduling** | Time-driven automatic mode switching — e.g., "show Departures Board 6am–10pm, Clock Mode overnight." Coexists with Foundation Release's night mode brightness scheduler: scheduling controls *which mode* runs, night mode controls *how bright* it runs. |
| 5 | **Mode-Specific Settings** | Per-mode configuration: Live Flight Card chooses which telemetry fields to show, Clock Mode offers 12h/24h format, Departures Board configures row count. All configured through the Mode Picker. |

**Update Infrastructure** (1 capability)

| # | Capability | Description |
|---|-----------|-------------|
| 6 | **OTA Pull** | User-initiated firmware updates from GitHub Releases. The dashboard shows when a new version is available; one tap to install. No silent auto-updates — the user is always in control. |

### What Makes This Special

The Foundation Release built the configuration backbone. The Display System Release built the rendering engine and mode abstraction. The Delight Release is where those investments pay compound interest — each new mode is a plugin, each transition is a single animation function, each setting is a config key. The marginal cost of delight is low because the architecture was designed for it.

### Project Classification

| Dimension | Value |
|-----------|-------|
| Type | IoT / Embedded |
| Domain | General |
| Complexity | Medium |
| Context | Brownfield (extending 3 prior releases) |

## Success Criteria

### User Success

| Criteria | Measure |
|----------|---------|
| **The wall never looks dead** | When zero flights are in range, Clock Mode activates automatically — no blank screens, no loading spinners, no "no data" messages. A guest sees a functioning display 100% of the time. |
| **Multi-flight awareness** | Departures Board shows all tracked flights at a glance. Users can see relative positions, callsigns, and key telemetry without cycling through one-at-a-time cards. |
| **Ambient mode switching** | Mode scheduling activates the right mode at the right time without user intervention. The wall transitions from Departures Board to Clock Mode at night and back in the morning — the user configured it once and forgot. |
| **Polished transitions** | Mode switches feel intentional, not broken. Fade/wipe/scroll animations replace hard cuts. A non-technical observer perceives the wall as a finished product, not a prototype. |
| **Painless firmware updates** | Dashboard shows "Update available" when a new GitHub Release exists. One tap starts the update. The device reboots into the new version within 60 seconds. Zero bricked devices. |
| **Quick schedule setup** | User configures a mode schedule in under 2 minutes through the Mode Picker. If configuration is painful, nobody uses it. |

### Business Success

| Criteria | Measure |
|----------|---------|
| **Self-service onboarding** | A competent hobbyist completes USB flash of Delight firmware, first boot, WiFi join (setup AP or saved credentials), and LAN dashboard access using only the repository README and linked setup / device-flash guides — without direct support from the author (**Journey 0**). |
| **Contributor-ready architecture** | A new contributor can add a DisplayMode without modifying core code — the plugin interface proves its value. |
| **OTA reduces support burden** | Users update firmware independently. No "re-flash via USB" instructions needed for patch releases. |

### Technical Success

| Criteria | Measure |
|----------|---------|
| **RAM budget respected** | Peak heap usage of any single active DisplayMode plus shared overhead (WiFi stack, HTTP client, flight data pipeline) stays under the ESP32's ~320KB usable heap ceiling. Only one mode is active at a time per the DisplayMode architecture. |
| **Smooth animation** | Transitions render at minimum 15fps on the 160x32 LED matrix. No visible tearing or stutter during fade/wipe/scroll. |
| **Crash recovery** | Watchdog timer detects an unresponsive main loop and reboots the device cleanly. After reboot, the system starts in a known-good default mode. A DisplayMode bug results in a reboot, not a brick. |
| **OTA reliability** | Firmware update leverages ESP32's built-in dual-partition A/B OTA mechanism. New image writes to the inactive partition; bootloader swaps on reboot. Failed updates boot back to the previous partition — never a half-written image. |
| **Scheduling + night mode coexistence** | Mode scheduler and brightness scheduler operate independently without conflicts. Time-driven mode switches do not reset brightness. Brightness changes do not trigger mode switches. |

### Measurable Outcomes

1. Clock Mode activates within one flight-pipeline poll interval (~30s) of the last flight leaving range
2. Departures Board renders up to 4 flight rows simultaneously on 160x32 matrix
3. Mode transitions complete in under 1 second at minimum 15fps
4. OTA Pull checks GitHub Releases API, downloads binary, and reboots in under 60 seconds on a 10Mbps connection
5. Total firmware binary size stays within the OTA application slot size defined in `firmware/custom_partitions.csv` (currently **2,097,152 bytes / 2 MiB** for `ota_0`), preserving the project's OTA update layout
6. Mode schedule triggers fire within 5 seconds of the configured time

## User Journeys

### Journey 0: Self-Service Setup — First Flash From Published Guides

**Persona:** Casey, competent hobbyist. New to TheFlightWall repo, has flashed other ESP32 projects before. No prior contact with the author.

**Opening Scene:** Casey lands on GitHub, reads the README, follows links to the device setup and flash guide. They install PlatformIO (or use the documented alternative), connect USB, build the Delight Release environment, and flash firmware and filesystem per the guide. No Discord, no email — just docs.

**Rising Action:** First boot: device presents the documented setup path (captive portal or saved WiFi from a prior flash). Casey joins the home network. Opens the dashboard at the documented URL (`flightwall.local` or LAN IP). Sees the Mode Picker and live or idle display behavior matching the guide's "expected result" section.

**Resolution:** Casey has a working Delight-capable device and knows where to change modes and settings. The business success criterion *self-service onboarding* is satisfied without author intervention.

**Capabilities revealed:** Documentation completeness (README → setup → flash → first dashboard), parity between documented steps and actual device behavior, Delight firmware + LittleFS delivered together as described in guides.

**Inherited scope:** WiFi setup AP behavior, NVS credential storage, and base portal routing are established in the Foundation / Display System releases; Delight must not regress those flows. New Delight UI (Mode Picker tabs, scheduling) must appear as documented after upgrade or first install.

### Journey 1: The Wall Comes Alive — Owner Sets Up Delight Features

**Persona:** Alex, hobbyist maker. Built the FlightWall six months ago. Loves it when planes are overhead, but the wall sits dark or shows a loading spinner when skies are quiet. Guests ask "is it broken?" and Alex has to explain. Embarrassing.

**Opening Scene:** Alex flashes the Delight Release over USB — an upgrade from the Display System Release. The device reboots. The dashboard loads and Alex's existing settings are intact — WiFi credentials, API keys, location, night mode schedule — all carried forward. But the Mode Picker now shows new options: Clock Mode, Departures Board, and a Scheduling tab. The wall itself is already showing the time instead of a blank screen. Zero-flight idle fallback kicked in automatically, no configuration needed. First impression: *it just knows.*

**Rising Action:** Alex opens the Mode Picker. Enables Departures Board as the primary mode when flights are in range. Sets Clock Mode to 12h format. Taps into the Scheduling tab — configures "Departures Board 6am–11pm, Clock Mode overnight." Then realizes the times are backwards — meant 11pm, typed 11am. Edits the entry inline, fixes it in seconds. The schedule timeline updates immediately. Total setup time: under two minutes, including the mistake.

That evening, Alex is on the couch. A plane enters range. The wall fades from the clock to the Departures Board — three flights listed, callsigns and altitudes updating live. No tap, no command. The transition was a smooth fade, not a hard cut. Alex's partner glances up: "Oh, there's a few planes out tonight."

**Climax:** 11pm. The schedule fires. But there's still one flight in range — the Departures Board is showing it. The scheduler wins: the wall fades from Departures Board to Clock Mode as configured. The lone flight's data isn't lost — if Alex switches back manually via the dashboard, it's still there. The time-driven schedule and data-driven fallback don't fight; the schedule takes priority when explicitly configured.

The brightness dims (night mode from the Foundation Release, still running independently). The wall is a soft glowing clock in a dark room. It feels like furniture now, not a gadget.

**Resolution:** Two weeks later, Alex hasn't opened the dashboard once. The wall runs itself — planes during the day, clock at night, dim after dark. A friend visits: "That's really cool, is it showing live flights?" No one asks if it's broken anymore.

**Capabilities revealed:** Clock Mode idle fallback, Departures Board rendering, mode scheduling, animated transitions, mode-specific settings (12h/24h), night mode coexistence, config persistence across upgrades, schedule editing, schedule-vs-data priority resolution.

### Journey 2: Painless Update — Owner Performs OTA Pull

**Persona:** Alex again, three months after initial Delight setup. A new firmware version has been released on GitHub with a bug fix for Departures Board row alignment.

**Opening Scene:** Alex opens the dashboard to tweak a setting. A banner at the top reads: "Firmware v2.4.1 available — you're running v2.4.0." A "View Release Notes" link and an "Update Now" button sit below.

**Rising Action:** Alex taps "View Release Notes" — a brief changelog appears (pulled from the GitHub Release description). The fix is relevant. Alex taps "Update Now." A progress bar appears. The dashboard shows: "Downloading... Writing to partition B... Rebooting..."

**Climax:** The wall goes dark for 3 seconds. It reboots. The dashboard reconnects. Status bar reads: "Running v2.4.1." The wall resumes in whatever mode it was showing — no configuration lost, no schedule reset. Total elapsed time: about 40 seconds.

**Resolution:** Alex didn't need to find a USB cable, open PlatformIO, or pull from git. The A/B partition scheme means the new image wrote to the inactive partition while the old one stayed safe. Seamless.

#### Edge Case: OTA Failure & Rollback

A month later, v2.4.2 drops. Alex taps "Update Now" from the couch over WiFi. Halfway through the download, the router hiccups — connection drops. The dashboard shows: "Download failed — firmware unchanged. Tap to retry." The device never left its current partition. Alex reconnects, taps "Retry," and it completes cleanly.

Alternatively: the download completes but the checksum doesn't match. The device attempts to boot the new partition, detects corruption, and the bootloader rolls back automatically. The dashboard shows: "Update failed — rolled back to v2.4.1. Your device is unchanged." Alex reports the issue on GitHub. The wall never stopped working.

**Capabilities revealed:** OTA Pull (check, download, install), A/B partition safety, download failure handling, checksum verification, automatic rollback, configuration persistence.

### Journey 3: The Ambient Experience — Guest Encounters the Wall

**Persona:** Sam, Alex's friend. Not technical. Has never heard of ADS-B or ESP32. Visiting for dinner.

**Opening Scene:** Sam walks into the living room. A long LED display on the wall shows three rows of text — airline names, flight numbers, altitudes — scrolling gently. It looks like a departures board at an airport. Sam: "Wait, are those real flights?"

**Rising Action:** Alex explains it tracks planes overhead in real time. As they talk, a fourth row appears — a new flight just entered range. One of the existing rows disappears as that plane flies out of range. The board updated itself without anyone touching anything.

**Climax:** After dinner, the room is quieter. The last plane leaves range. The display fades — a smooth animation — into a large, clean clock. Sam doesn't notice the transition consciously. It just feels right. The wall went from informational to decorative without a jarring cut.

**Resolution:** Sam leaves thinking "that was cool" — not "that was a cool ESP32 project." The wall felt like a product, not a prototype. Sam texts Alex the next day: "Where do I get one of those?"

**Capabilities revealed:** Departures Board live updates, animated transitions, Clock Mode idle fallback, polish/product feel.

### Journey 4: Plugin Architecture — Contributor Adds a New Mode

**Persona:** Jordan, a C++ hobbyist who found TheFlightWall on GitHub. Wants to build a "Radar Mode" that shows aircraft positions as dots on a polar plot.

**Opening Scene:** Jordan reads the README and architecture docs. The DisplayMode interface is documented: `setup()`, `loop()`, `teardown()`, and a metadata struct for the mode name and settings schema. Jordan clones the repo, installs PlatformIO, and opens the `adapters/` folder — the existing modes (LiveFlightCard, ClockMode, DeparturesBoard) are all self-contained files implementing the same interface. The toolchain setup takes time, but the *interface* is simple — Jordan's effort goes into the radar rendering, not into understanding the framework.

**Rising Action:** Jordan creates `RadarMode.cpp`, implements the interface methods, and adds the mode to the compile-time registry (a single line in a header). The mode draws a polar grid and plots flight positions from the shared flight data pipeline — no need to touch the fetcher code. Jordan adds a settings key for radar radius. First compile works. Upload to ESP32 via USB.

**Climax:** The Mode Picker on the dashboard shows "Radar Mode" as a new option. Jordan selects it. The wall fades from Clock Mode to a radar display. Jordan's custom mode is running alongside the official modes with transitions, scheduling compatibility, and settings — all for free from the architecture.

**Resolution:** Jordan opens a PR. The radar mode is a single `.cpp` and `.h` plus the compile-time registry entry required by **FR41** — no changes to core flight-fetch or layout engine. The contributor-ready architecture promise delivered.

**Capabilities revealed:** DisplayMode plugin interface, compile-time registry, mode-specific settings schema, transition compatibility, shared flight data pipeline, PlatformIO toolchain requirement.

### Journey Requirements Summary

| Journey | Primary Capabilities Exercised |
|---------|-------------------------------|
| 0 — Self-Service Setup | Published README + setup/flash guides, first boot and dashboard access without author support (**FR43**); inherits Foundation/Display portal and WiFi flows |
| 1 — Ambient Delight | Clock Mode, Departures Board, mode scheduling, transitions, mode-specific settings, night mode coexistence, config persistence, schedule editing, schedule-vs-data priority |
| 2 — OTA Update | OTA Pull (check, download, install), A/B partition safety, failure handling, rollback, config persistence |
| 3 — Guest Experience | Departures Board live rendering, transitions, Clock Mode idle fallback, product polish |
| 4 — Contributor | **FR41–FR42** (extensibility contract + Mode Picker discovery); DisplayMode interface, compile-time registry, settings schema, transition compatibility, PlatformIO toolchain |

## IoT/Embedded Specific Requirements

### Hardware Requirements

| Spec | Value |
|------|-------|
| **MCU** | ESP32 (Espressif32), dual-core 240MHz, WiFi built-in |
| **Display** | 20x WS2812B 16x16 LED panels in 10x2 grid = 160x32 pixels |
| **RAM** | ~320KB usable heap; 8KB stack per task |
| **Flash** | 4MB with dual OTA partitions (A/B scheme) |
| **Power** | Wall-powered via USB/barrel jack — no battery constraints |
| **Existing peripherals** | NeoPixel data pin (configurable), WiFi antenna (onboard) |

**Delight Release hardware impact:** No new hardware. Clock Mode, Departures Board, and transitions all render to the existing 160x32 LED matrix. OTA Pull uses existing WiFi and flash partitions. Zero BOM change.

### Connectivity Protocol

| Protocol | Purpose | Delight Release Impact |
|----------|---------|----------------------|
| **WiFi (802.11 b/g/n)** | All network communication | No change — existing |
| **HTTPS → OpenSky API** | ADS-B flight data | No change — existing pipeline |
| **HTTPS → AeroAPI** | Flight metadata enrichment | No change — existing pipeline |
| **HTTPS → FlightWall CDN** | Airline display names | No change — existing |
| **HTTPS → GitHub Releases API** | OTA version check + binary download | **New** — polls `api.github.com` for latest release, downloads `.bin` asset |

**New endpoint:** GitHub Releases API (`api.github.com/repos/{owner}/{repo}/releases/latest`). Unauthenticated. Rate limit: 60 requests/hour. If rate limit is exceeded (HTTP 429), the dashboard displays "Unable to check for updates — try again later" and does not retry automatically. Binary download from GitHub's CDN (`github.com/.../*.bin`), with a companion `.sha256` asset for integrity verification.

### Power Profile

Wall-powered. No sleep modes, no battery optimization. The ESP32 runs continuously in a cooperative main loop. Power constraints are irrelevant for this project — the LED panels draw far more than the MCU.

### Security Model

| Concern | Approach |
|---------|----------|
| **API keys at rest** | Stored in ESP32 NVS (non-volatile storage) via ConfigManager. Not encrypted — physical access to the device means access to keys. Acceptable for a personal IoT device on a home network. |
| **API keys in transit** | All API calls use HTTPS. Certificate validation via ESP32's mbedTLS stack. |
| **OTA binary integrity** | SHA-256 checksum verified against a `.sha256` file published as a companion release asset. ESP32's bootloader performs additional image header validation (CRC + magic bytes) on boot. |
| **OTA source trust** | Binary downloaded from a known GitHub repository URL. No code signing in MVP — the user explicitly initiates the update and trusts the repo. |
| **Dashboard access** | HTTP on local network, no authentication. Any device on the same WiFi network can access the dashboard, change settings, switch modes, and trigger OTA updates. This is a conscious trade-off: for a single-user personal device on a home network, the complexity of authentication outweighs the risk. |

**Not in scope:** Code signing for OTA binaries (Growth feature if community demand emerges), HTTPS for the local dashboard, multi-user access control.

### Update Mechanism

| Aspect | Design |
|--------|--------|
| **Trigger** | User-initiated "Check for Updates" in dashboard |
| **Discovery** | GET to GitHub Releases API → compare `tag_name` with compiled `FIRMWARE_VERSION` |
| **Pre-download** | Verify minimum 80KB free heap. If insufficient, warn user "Not enough memory — restart device and try again." Tear down the active DisplayMode to free its heap. Switch LED matrix to a static "Updating..." progress screen. |
| **Download** | Stream `.bin` asset directly to inactive OTA partition via `esp_https_ota` or equivalent. Display shows download progress on the LED matrix. Main rendering loop is suspended — OTA owns the main loop until complete. |
| **Verification** | Download companion `.sha256` asset. Compare SHA-256 of written partition against expected hash. Reject and abort if mismatch. |
| **Activation** | `esp_ota_set_boot_partition()` + reboot |
| **Rollback** | If new partition fails boot validation, bootloader reverts to previous partition automatically |
| **Config persistence** | NVS (config data) is on a separate partition from firmware — survives OTA. Mode schedules, settings, and API keys are retained across updates. |

## Project Scoping & Phased Development

*This section is the authoritative source for scope decisions. The Product Scope summary in the Success Criteria section provides the short-form list; this section provides strategic framing, phasing, sequencing, and risk analysis.*

### MVP Strategy & Philosophy

**MVP Approach:** Experience MVP — the wall must feel like a finished ambient device, not a feature demo. Every capability ships polished or doesn't ship. One transition type done well beats three done poorly.

**Resource Model:** Solo developer. All capabilities implemented sequentially. No parallel workstreams.

### Implementation Sequence

Capabilities have real dependencies. For a solo developer, this sequence ensures each step validates the previous one:

| Order | Capability | Rationale |
|-------|-----------|-----------|
| 1 | **Clock Mode** | Simplest mode. Proves the DisplayMode lifecycle (setup/loop/teardown) works end-to-end. Immediately useful as idle fallback. |
| 2 | **Departures Board** | Proves multi-element rendering on the LED matrix. Uses existing flight data pipeline. After this step, the wall is already useful with two modes. |
| 3 | **Animated Transitions** | Requires at least two modes to exist. Implements fade between Clock and Departures Board. Validates dual-frame-buffer approach. |
| 4 | **Mode Scheduling** | Requires modes + transitions working. Adds time-driven switching layer. Tests coexistence with night mode brightness scheduler. |
| 5 | **Mode-Specific Settings** | Requires modes registered in the system. Adds per-mode config keys to Mode Picker UI. |
| 6 | **OTA Pull** | Architecturally independent — could be built any time. Placed last because testing benefits from having all modes stable, and it's the highest-risk feature (network + flash writes). |

**Shippable checkpoint after step 2:** Clock Mode + Departures Board alone delivers ambient intelligence (idle fallback + multi-flight display) even without transitions or scheduling.

### MVP Feature Set (Phase 1)

**Core Journeys Supported:**

| Journey | MVP Coverage |
|---------|-------------|
| 0 — Self-Service Setup | Full: README-linked guides suffice for flash, first boot, and dashboard access (**FR43**). Regression-tested against Foundation/Display setup flows. |
| 1 — Ambient Delight | Full: Clock Mode, Departures Board, scheduling, transitions, mode-specific settings |
| 2 — OTA Update | Full: check, download, install, rollback. Edge cases (download failure, checksum mismatch) included. |
| 3 — Guest Experience | Full: this journey has zero configuration — it validates that Journeys 1 and 2 produced a polished result |
| 4 — Contributor | Full for **interface contract**: **FR41–FR42** formalize extensibility on top of the Display System DisplayMode architecture. Step-by-step contributor guide remains **Growth** (post-MVP documentation). |

**Must-Have Capabilities:**

1. Clock Mode — 12h/24h, automatic idle fallback
2. Departures Board — multi-row flight list (up to 4 rows)
3. Animated Transitions — fade (minimum one type)
4. Mode Scheduling — time-based rules, coexists with night mode
5. Mode-Specific Settings — per-mode config via Mode Picker
6. OTA Pull — GitHub Releases check, one-tap install, A/B safety

**Explicitly deferred from MVP:**
- Multiple transition types (wipe, scroll) — fade only in MVP
- Departures Board layout variants — single layout in MVP
- Schedule templates — manual schedule entry only
- Contributor documentation / "how to add a mode" guide
- Clock Mode face styles

### Post-MVP Features

**Phase 2 — Growth:**
- Additional transition types (wipe, scroll) selectable per-mode pair
- Departures Board layout variants (compact vs expanded row)
- Schedule templates ("Home office," "Weekend") for one-tap switching
- Contributor guide: "How to build a DisplayMode"

**Phase 3 — Vision:**
- Clock Mode face styles (minimal, detailed with date/weather)
- Community-contributed DisplayMode plugins
- Context-aware mode selection (time + flight density + patterns)
- Transition choreography per mode pair
- Remote schedule management from companion app

### Risk Mitigation Strategy

**Technical Risks:**

| Risk | Severity | Mitigation |
|------|----------|------------|
| OTA bricks device | Critical | ESP32 A/B partitions provide hardware-level rollback. SHA-256 verification before boot swap. Heap check before download. Mode teardown frees resources. |
| Config migration between releases | Medium | Delight Release adds new config keys (schedule rules, mode preferences). Missing keys resolve to safe built-in defaults on first read (**FR33**). No migration script needed. Existing NVS values (WiFi, API keys, night mode) are never overwritten. |
| Transitions stutter on LED matrix | Medium | MVP ships only fade (simplest). Fade requires dual frame buffers (~30KB for 160x32 at 3 bytes/pixel). Measure fps during development. If <15fps, simplify animation. Wipe/scroll deferred to Growth. |
| Scheduling conflicts with night mode | Medium | Clear separation: scheduler = which mode, night mode = brightness. Independent state machines. Test both running simultaneously. |
| Departures Board exceeds RAM | Medium | Limit to 4 rows in MVP. Each row is lightweight (callsign + 2 telemetry fields). Profile heap before and after mode activation. |
| GitHub API unavailable | Low | OTA check is manual and non-blocking. If API unreachable, dashboard shows "Unable to check" and the device continues normal operation. |

**Market Risks:** N/A — personal open-source project. No market validation needed. The author is the primary user.

**Resource Risks:** Solo developer. If any single capability exceeds 2x its initial time estimate, evaluate the cut order before continuing:

1. First to cut: Schedule templates (already deferred)
2. Second: Multiple transition types (already deferred)
3. Third: Departures Board layout variants (already deferred)
4. If further cuts needed: simplify Mode Scheduling to fixed rules (no UI editor), keep programmatic config only

Core 6 MVP capabilities are non-negotiable.

## Functional Requirements

*Scope note:* **FR1–FR40** cover Delight end-user features, OTA, and runtime resilience. **FR41–FR42** formalize contributor extensibility on top of the Display System Release DisplayMode architecture (new requirements in this PRD, not a duplicate of Display System scope). **FR43** ties the business onboarding criterion to testable documentation expectations.

### Clock Mode

- **FR1:** The system can display the current time on the LED matrix in a large-format layout
- **FR2:** The owner can select between 12-hour and 24-hour time format for Clock Mode
- **FR3:** The system can automatically activate Clock Mode when the flight data pipeline returns zero aircraft in range (idle fallback)
- **FR4:** The system can exit Clock Mode idle fallback and restore the previously active mode when flight data becomes available

### Departures Board

- **FR5:** The system can display multiple flights simultaneously in a list format on the LED matrix
- **FR6:** The system can show, for each Departures Board row, the callsign and at least two telemetry fields chosen from the same owner-configurable field set as Live Flight Card / Mode Picker (e.g. altitude, ground speed, heading/track); when the owner has not customized the field list, the row shows callsign, altitude, and ground speed by default
- **FR7:** The system can add a row when a new flight enters tracking range
- **FR8:** The system can remove a row when a flight leaves tracking range
- **FR9:** The owner can configure the maximum number of visible rows on the Departures Board

### Mode Transitions

- **FR10:** The system can perform animated visual transitions when switching between display modes
- **FR11:** The system can execute a fade transition between any two display modes
- **FR12:** The system can complete mode transitions without visible tearing or artifacts on the LED matrix

### Mode Scheduling

- **FR13:** The owner can define time-based rules that automatically switch between display modes
- **FR14:** The system can execute scheduled mode switches at the configured times
- **FR15:** The system can operate the mode scheduler independently from the night mode brightness scheduler
- **FR16:** The owner can edit and delete existing schedule rules through the dashboard
- **FR17:** The system can resolve conflicts between scheduled mode switches and data-driven idle fallback, with the explicit schedule taking priority
- **FR36:** The system can persist schedule rules across power cycles and reboots
- **FR39:** The owner can view all configured schedule rules and their current status

### Mode Configuration

- **FR18:** The owner can view and modify settings specific to each display mode through the Mode Picker
- **FR19:** The system can apply mode-specific setting changes without requiring a device reboot
- **FR20:** The system can persist mode-specific settings across power cycles and reboots
- **FR40:** The owner can manually switch between available display modes from the dashboard

### OTA Firmware Updates

- **FR21:** The owner can check for available firmware updates from the dashboard
- **FR22:** The system can compare the installed firmware version against the latest version available on GitHub Releases
- **FR23:** The owner can view release notes for an available update before choosing to install
- **FR24:** The owner can initiate a firmware update with a single action from the dashboard
- **FR25:** The system can download and install firmware updates over WiFi without physical access to the device
- **FR26:** The system can display update progress on the LED matrix during the download and install process
- **FR27:** The system can verify firmware integrity before activating a new version
- **FR28:** The system can automatically revert to the previous firmware version if a new version fails to boot
- **FR29:** The system can preserve all user settings and configuration across firmware updates
- **FR30:** When a firmware update fails, the dashboard shows a status message that states (a) the failure phase: download, verify, or boot; (b) the outcome: retriable (e.g. download aborted, firmware unchanged) or rolled back to the previous version; (c) the recommended next action: retry, reboot, or none. If the failure is also indicated on the LED matrix, the text is distinct from the in-progress OTA progress state
- **FR31:** The system can handle interrupted downloads without affecting the currently running firmware
- **FR37:** The owner can retry a failed firmware update without restarting the device
- **FR38:** The system can display a notification in the dashboard when a newer firmware version is available

### System Resilience

- **FR32:** The system can recover from an unresponsive display mode by rebooting into a known-good default mode
- **FR33:** When upgrading from a prior firmware version, the system can supply safe default values for any configuration keys introduced in the Delight Release on first read if those keys are absent, without overwriting existing stored keys (WiFi, API keys, night mode, prior user schedules and mode settings)
- **FR34:** Before starting an OTA firmware download, the system verifies at least **80KB** free heap (aligned with NFR16) and blocks or warns per the documented OTA pre-check behavior. Before starting a mode transition that requires dual frame buffers, the system verifies buffer allocation can succeed or applies the MVP fallback (e.g. simplified transition) documented in risk mitigation
- **FR35:** When OpenSky or AeroAPI is unavailable, the flight pipeline stops receiving updates from the affected source, the device remains responsive, and the active display mode continues with the last successful flight snapshot or empty-flight rules (including Clock idle fallback when appropriate). When GitHub Releases is unavailable, OTA version check shows an unavailable state and flight display is unaffected. The device does not enter a tight reboot loop and does not show a blank LED matrix for more than **1 second** except during an intentional full-screen OTA progress state

### Contributor Extensibility

- **FR41:** A developer can add a new display mode by implementing the documented DisplayMode lifecycle (setup, loop, teardown, metadata/settings schema) and registering the mode in the project's compile-time mode registry, without modifying core flight-fetch, HTTP client, or layout-engine source files beyond the registry hook
- **FR42:** The dashboard Mode Picker lists every compile-time registered display mode (including modes added per **FR41**) for owner selection, with the same manual switching and scheduling integration as built-in Delight modes

### Documentation & Onboarding

- **FR43:** The repository README links to the current setup and device-flash guides such that a competent hobbyist can flash Delight firmware and filesystem, complete first boot and network join, and reach the dashboard using only those published documents — consistent with **Journey 0** and the self-service onboarding business success criterion

## Non-Functional Requirements

### Performance

- **NFR1:** Mode transitions render at minimum 15fps on the 160x32 LED matrix
- **NFR2:** Mode transitions complete in under 1 second
- **NFR3:** Clock Mode idle fallback activates within one flight-pipeline poll interval (~30s) of zero flights in range
- **NFR4:** Departures Board row additions and removals appear on the LED matrix within **50ms** of the flight list change (one frame at ≥20fps refresh), or within one full matrix redraw interval if the measured refresh rate is lower — whichever bound is tighter for the build under test
- **NFR5:** OTA firmware download, verification, and reboot complete in under 60 seconds on a 10Mbps WiFi connection
- **NFR6:** Mode schedule triggers fire within 5 seconds of the configured time
- **NFR7:** Dashboard pages load within 1 second on a local network connection
- **NFR8:** Mode-specific setting changes apply to the display within 2 seconds of submission

### Reliability

- **NFR9:** The device operates continuously for minimum 30 days without manual intervention under normal conditions
- **NFR10:** Watchdog timer reboots the device within 10 seconds of detecting an unresponsive main loop
- **NFR11:** After a watchdog reboot, the system restarts in a known-good default mode without user intervention
- **NFR12:** OTA update failure never renders the device unbootable — the previous firmware partition remains available
- **NFR13:** Peak heap usage of any single active DisplayMode plus shared overhead stays under the ESP32's usable heap ceiling (~320KB)
- **NFR14:** The firmware image fits within **2,097,152 bytes (2 MiB)**, the OTA application slot size for `ota_0` in `firmware/custom_partitions.csv` (authoritative for this project). If the partition table changes, this NFR tracks the **smaller** OTA `app` slot size in that file
- **NFR15:** Configuration data (NVS) survives firmware updates, power cycles, and watchdog reboots without corruption
- **NFR16:** The system maintains minimum 80KB free heap during normal operation to ensure OTA readiness
- **NFR17:** The system exhibits no progressive heap degradation — free heap after 24 hours of operation remains within 5% of free heap after boot

### Integration

- **NFR18:** When **OpenSky or AeroAPI** is unavailable: the device keeps running the active display mode using last-known flight data or zero-aircraft rules (Clock idle fallback per **FR3**/**FR35**); watchdog does not trigger solely due to that API outage. When **GitHub Releases** is unavailable: OTA check/reporting degrades (dashboard shows update check unavailable); flight rendering and mode scheduling are unchanged
- **NFR19:** For each integration failure (flight data source vs GitHub update check), the dashboard shows a **non-empty** status line naming the affected integration and whether **flight display** or **update check** is impacted. The LED matrix does not remain blank for more than **1 second** except during intentional OTA progress display; the device does not crash-loop
- **NFR20:** GitHub Releases API rate limit exhaustion (HTTP 429) results in a user-facing "try again later" message with no automatic retry loop
- **NFR21:** The system tolerates network latency up to 10 seconds on API calls without triggering watchdog recovery
