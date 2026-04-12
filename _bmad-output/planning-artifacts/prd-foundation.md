---
stepsCompleted: ['step-01-init', 'step-02-discovery', 'step-02b-vision', 'step-02c-executive-summary', 'step-03-success', 'step-04-journeys', 'step-05-domain-skipped', 'step-06-innovation-skipped', 'step-07-project-type', 'step-08-scoping', 'step-09-functional', 'step-10-nonfunctional', 'step-11-polish', 'step-12-complete', 'step-e-01-discovery', 'step-e-02-review', 'step-e-03-edit']
inputDocuments: ['_bmad-output/planning-artifacts/prd.md', 'docs/project-overview.md', 'docs/architecture.md', 'docs/component-inventory.md', 'docs/api-contracts.md', '_bmad-output/planning-artifacts/implementation-readiness-report-foundation-2026-04-11.md']
documentCounts:
  briefs: 0
  research: 0
  brainstorming: 0
  projectDocs: 5
workflowType: 'prd'
projectType: 'brownfield'
classification:
  projectType: iot_embedded
  domain: general
  complexity: medium
  projectContext: brownfield
lastEdited: '2026-04-11'
workflow: 'edit'
editHistory:
  - date: '2026-04-11'
    changes: 'Validation fixes: added FR18 (IANA-to-POSIX timezone mapping), tightened FR10 measurability, replaced subjective NFR C5 with measurable threshold'
  - date: '2026-04-11'
    changes: 'Aligned OTA post-boot validation with Architecture Decision F3 (WiFi-or-60s-timeout; no self-HTTP check): Technical Success, MVP must-haves, FR5–FR6, NFR reliability, Journey 4, risk row, trade-off note'
---

# Product Requirements Document - TheFlightWall Foundation Release

**Author:** Christian
**Date:** 2026-04-11

## Executive Summary

TheFlightWall is an ESP32-powered LED wall that displays live flight information for aircraft near a configurable location. The existing firmware — covered by the original MVP PRD — delivers a complete web configuration portal, zone-based display with airline logos, and a multi-API data pipeline (OpenSky, AeroAPI, FlightWall CDN). That system is built and working.

This Foundation Release addresses the gap between "working firmware" and "self-sustaining product." Three capabilities close that gap:

1. **OTA Firmware Updates (Push via Dashboard)** — Upload new firmware binaries through the web UI. Requires a one-time partition table migration (USB flash) to create dual OTA app slots, after which the device never needs a USB cable again. Every future feature — display modes, animations, themes — ships over the air.

2. **Night Mode / Brightness Schedule** — Time-based brightness control using NTP clock sync. Users define a dim period (e.g., 11pm-7am at 10% brightness) so the wall is livable 24/7 without manual adjustment. Leverages the existing hot-reload brightness path — minimal new code, high quality-of-life impact.

3. **Onboarding Polish (Test Your Wall)** — A new Step 6 in the setup wizard that auto-runs calibration patterns and asks the user to confirm their panel layout matches before proceeding to flight fetching. Catches the two most common hardware issues — wrong wiring flags and bad GPIO pin — before the user ever sees garbled output.

### What Makes This Special

This release ships no user-visible "features" in the traditional sense. Its value is compounding infrastructure: OTA makes every future update frictionless, night mode makes the wall livable around the clock, and onboarding polish eliminates the most likely first-run failure. Together they transform a maker project that requires a laptop and PlatformIO into an appliance you configure from your phone and forget about. The wall just works — day, night, and through every firmware update that follows.

## Project Classification

- **Project Type:** IoT/Embedded — ESP32 firmware with WiFi connectivity, web UI companion interface, LED matrix display, and external API integrations
- **Domain:** General (aviation data consumer) — no regulatory, safety, or compliance requirements
- **Complexity:** Medium — OTA introduces a partition-level architectural change (dual app slots, ~56% LittleFS reduction) with downstream flash budget implications for all future releases. Night mode and onboarding polish are low complexity individually but ship alongside the partition migration.
- **Project Context:** Brownfield — enhancing existing working firmware with established hexagonal (Ports & Adapters) architecture

## Success Criteria

### User Success

- Flash the OTA-enabled firmware via USB one final time. All subsequent firmware updates happen through the dashboard — no cable, no PlatformIO, no laptop required
- Upload a new firmware binary from the dashboard and have the device update, reboot, and resume normal operation without manual intervention
- Set a night mode schedule in local time and have the wall automatically dim at the configured time and restore brightness in the morning — no daily manual adjustment, correct across midnight boundaries and DST transitions
- Complete the setup wizard on a fresh device and have the "Test Your Wall" step let you visually confirm the panel layout matches before ever seeing flight data

### Technical Success

- Dual OTA partition table (app0 ~1.5MB + app1 ~1.5MB + LittleFS ~896KB) fits within 4MB flash with existing web assets and 99 bundled logos, leaving at least 500KB free in LittleFS
- OTA upload endpoint validates binary size against partition capacity before writing — rejects oversized binaries with a clear error
- Device boots into new firmware after OTA and runs a **boot-time mark-valid gate** aligned with **Architecture Decision F3:** call `esp_ota_mark_app_valid_cancel_rollback()` when **WiFi STA connects** or when **60 seconds have elapsed since boot**, whichever comes first (no self-HTTP request to `/api/status` or `/api/ota/upload`). If the firmware **crashes before** that call, the active partition is never marked valid and the bootloader rolls back on next reboot
- If WiFi drops mid-OTA upload, `Update.abort()` is called and the inactive partition is not marked bootable — device continues running current firmware
- NTP time sync uses `configTzTime()` with a configurable POSIX timezone string, completing within 10 seconds of WiFi connection
- Night mode brightness scheduler correctly handles midnight-crossing schedules (e.g., 23:00-06:00) and DST transitions
- Night mode brightness changes use the existing hot-reload config path — no new inter-core communication needed
- Onboarding "Test Your Wall" step reuses the existing calibration pattern and panel positioning endpoints — no new LED rendering logic
- All existing functionality (fetch → enrich → display pipeline, web dashboard, logo management) works identically after the partition table migration

### Measurable Outcomes

- **OTA smoke test:** Build a firmware binary on a laptop → open dashboard → upload .bin file → device flashes, reboots, **F3 mark-valid gate** completes → new version string visible in `/api/status` → flights resume displaying within 60 seconds
- **OTA rejection test:** Upload a file exceeding 1.5MB → endpoint returns error before writing → device continues running normally
- **OTA interruption test:** Begin firmware upload → disconnect WiFi mid-transfer → device remains on current firmware, no boot loop, no corruption
- **Night mode smoke test (same-day):** Set dim start 1 minute from now → wall dims to configured brightness → set restore 1 minute later → wall restores. Confirm survives power cycle.
- **Night mode smoke test (midnight crossing):** Set dim start to 23:00, dim end to 06:00 → verify wall dims at 23:00 and restores at 06:00 the next day
- **Onboarding smoke test:** Factory reset → reconnect to AP → complete wizard steps 1-5 → Step 6 displays panel position pattern on LEDs and shows matching canvas preview → user asked "Does your wall match this layout?" → select No → returns to hardware settings → fix settings → Step 6 again → select Yes → RGB color sequence runs → proceed to flight fetching
- **Regression baseline:** All original PRD smoke tests (Epic 1 and Epic 2) pass unchanged after partition migration

## Product Scope & Phased Development

### MVP Strategy

**Approach:** Problem-solving MVP — deliver the three foundational capabilities (OTA, night mode, onboarding polish) that transform the FlightWall from a developer-maintained project into a self-updating appliance.

**Resource Requirements:** Solo developer (Christian) with AI-assisted development. No team dependencies. One-time USB flash for partition migration; all subsequent development benefits from OTA.

### MVP Feature Set (Phase 1 — Foundation Release)

**Core User Journeys Supported:**
- Journey 1 (The Last USB Flash) — partition migration + first OTA update
- Journey 2 (Living Room at Midnight) — night mode setup and operation
- Journey 3 (Fresh Start Done Right) — onboarding with Test Your Wall
- Journey 4 (The Bad Flash) — OTA failure recovery

**Must-Have Capabilities:**

**OTA Firmware Updates:**
- Custom dual-OTA partition table (`app0` ~1.5MB, `app1` ~1.5MB, `spiffs` ~896KB)
- `/api/ota/upload` endpoint using `Update.h`
- Binary size validation against partition capacity
- ESP32 image magic byte (`0xE9`) validation — reject non-firmware files
- `Update.abort()` on stream failure or WiFi interruption
- OTA **mark-valid gate** before the new firmware is considered permanent ( **Architecture Decision F3** — canonical detail in `architecture.md` ):
  1. After boot, wait until **WiFi STA is connected** *or* **60 seconds have elapsed** since boot (covers temporary outages without false rollback).
  2. Then call `esp_ota_mark_app_valid_cancel_rollback()` once. If WiFi was not verified (timeout path), the device may record a **warning** in status (implementation per architecture).
  3. **No** self-HTTP request to verify `/api/status` or `/api/ota/upload` — deliberate simplification to avoid async server races.
  4. If the firmware **crashes or resets** before step 2, the partition is not marked valid → bootloader rollback on next reboot.
- Rollback detection and user notification
- Firmware version string in `/api/status` response
- Dashboard Firmware card with upload UI, progress feedback, current version display
- Settings export (`GET /api/settings/export`) for pre-migration backup
- Settings import in wizard — client-side JSON file upload that pre-fills wizard form fields for user review and confirmation (~40 lines JS, no new endpoint)

**Night Mode / Brightness Schedule:**
- NTP time sync using `configTzTime()` with POSIX timezone string
- New config keys: `timezone`, `schedule_enabled`, `schedule_dim_start`, `schedule_dim_end`, `schedule_dim_brightness`
- Time-based brightness scheduler with midnight-crossing support
- Dashboard Night Mode card with time pickers, brightness slider
- NTP sync status indicator ("Clock not set — schedule inactive" / "Clock synced")
- Dashboard timezone selector

**Onboarding Polish:**
- Setup wizard Step 6: auto-run panel position pattern with canvas preview
- Visual confirmation prompt ("Does your wall match this layout?")
- Back-navigation to hardware settings on user "No"
- RGB color test sequence on user "Yes"
- Browser timezone auto-detection (`Intl.DateTimeFormat`) in Location step (Step 3)
- IANA-to-POSIX timezone mapping

**Pre-Implementation Gate:**
- Measure current firmware binary size — if exceeding 1.3MB, optimize before adding new code

### Phase 2 — Growth (Post-MVP)

- OTA pull from GitHub releases ("Check for Updates" button)
- Sunrise/sunset-aware brightness scheduling
- OTA update history log (last 5 updates with timestamps)

### Phase 3 — Expansion (Future)

- Fully automatic OTA with GitHub release monitoring
- Ambient light sensor integration for adaptive brightness
- Remote fleet management for multiple FlightWalls
- OTA firmware signing for public distribution

### Risk Mitigation Strategy

| Risk | Impact | Mitigation |
|------|--------|------------|
| Firmware binary exceeds 1.5MB partition | Cannot OTA update | Pre-implementation size gate at 1.3MB; optimize libraries if needed |
| Partition migration erases all user data | User frustration, reconfiguration time | Settings export + client-side wizard import for fast reconfiguration |
| OTA upload of non-firmware file | Potential brick | Magic byte validation + size check before write |
| OTA update crashes on boot | Device appears dead | Watchdog + rollback to previous partition; mark-valid gate never reached |
| NTP unreachable after WiFi connect | Night mode doesn't activate | Graceful degradation; dashboard shows sync status; no crash |
| Dashboard/web stack broken by bad firmware but WiFi stack still runs | User cannot fix via OTA from LAN; may need USB | F3 does **not** HTTP-verify the web server before mark-valid; recovery is USB or physical access. Team accepts this trade-off for simpler, race-free boot logic (see Architecture F3 scenario table) |
| LittleFS too small for future web assets | Can't add dashboard features | 618KB headroom sufficient; monitor usage; optimize gzip compression |

## User Journeys

### Journey 1: The Last USB Flash — "Cutting the Cord"

Christian has been running the FlightWall for a few weeks and loves it, but every tweak to the firmware means pulling the ESP32 off the wall, finding a USB-C cable, opening PlatformIO, building, flashing, and remounting. He just built the Foundation Release firmware with OTA support — time to make this the last time.

Before he flashes, he opens the dashboard and taps "Download Settings" in the System card. The browser saves a `settings.json` file with all his configuration — WiFi, API keys, location, hardware config, timing. He knows the repartition will erase NVS and LittleFS, so this backup is his safety net. Any custom logos he uploaded will need to be re-uploaded, but at least he won't be re-typing API keys from scratch.

He connects the ESP32 to his laptop, opens PlatformIO, and flashes the new firmware with the dual-OTA partition table. The device reboots into AP mode — NVS and LittleFS are clean. He connects to `FlightWall-Setup`, and on the WiFi step he enters his credentials. On Step 3 (Location), the wizard auto-detects his timezone from the browser (`America/Los_Angeles`) and pre-fills it alongside his lat/lon. He breezes through the rest, referencing `settings.json` on his phone for the API keys and hardware config. After Step 6 (Test Your Wall), flights appear. Total reconfiguration: about 3 minutes, mostly pasting keys.

A week later, he's made a display tweak in the code. He builds the `.bin` file on his laptop, opens the dashboard from his phone, taps the new Firmware card, and drags the binary onto the upload area. A progress bar fills. The dashboard shows "Uploading firmware... Rebooting..." The wall goes dark for a few seconds, then the loading screen appears, WiFi reconnects, and flights start cycling. He checks `/api/status` — the version string matches the new build. The whole process took 30 seconds from his couch.

**The moment:** The USB cable goes back in the drawer. Every future improvement — display modes, animations, themes — ships from the couch.

**Reveals requirements for:** Settings export (`/api/settings/export`), pre-migration guidance in dashboard, dual-OTA partition table, `/api/ota/upload` endpoint, firmware binary validation, upload progress feedback, **F3** mark-valid gate with `esp_ota_mark_app_valid_cancel_rollback()`, version string in `/api/status`, dashboard Firmware card with upload UI

### Journey 2: Living Room at Midnight — "The Wall That Sleeps"

It's 11:30pm and Christian is heading to bed. The FlightWall is mounted in the living room and the bright LED glow bleeds down the hallway. He's been manually opening the dashboard and dropping brightness to 5 every night, then bumping it back to 40 in the morning. It's annoying.

He opens the dashboard, scrolls to the new Night Mode card, and sets: dim start 23:00, dim end 07:00, dim brightness 10. The timezone was already configured during setup — the wizard auto-detected `America/Los_Angeles` from his browser. Hits Apply. The settings save instantly (no reboot needed — these are display-path configs).

That night at 11pm, the wall quietly fades to brightness 10. He barely notices — which is the point. The next morning at 7am, the wall is back to full brightness when he walks past with coffee. It just works.

A few weeks later, daylight saving time kicks in. The schedule doesn't drift — `configTzTime()` with the POSIX timezone string handles the DST transition automatically. Christian never touches the setting again.

**The moment:** The wall becomes invisible at night and present during the day — no daily ritual, no manual adjustment, no DST surprises.

**Reveals requirements for:** NTP time sync with `configTzTime()`, POSIX timezone config key, `schedule_dim_start` / `schedule_dim_end` / `schedule_dim_brightness` config keys, midnight-crossing time comparison logic, dashboard Night Mode card (time pickers, brightness slider), hot-reload brightness path

### Journey 3: Fresh Start Done Right — "The Setup That Checks Itself"

Christian's friend Jake saw the FlightWall and wants to build one. Christian ships him an ESP32 pre-flashed with the Foundation firmware and a box of LED panels. Jake has never used PlatformIO and never will.

Jake wires up the panels, powers on the ESP32, connects to `FlightWall-Setup` from his phone, and breezes through the wizard: WiFi credentials, API keys (Christian texted him the credentials), and then Location. On the location step, Jake hits "Use my location" and the browser fills in lat/lon. The wizard also auto-detects his timezone from the browser — `America/New_York` — and pre-fills it. He confirms and moves on. Hardware config: 8x2 tiles, 16x16 pixels, GPIO 25.

Then Step 6 appears: **"Test Your Wall."** The LED panels light up with numbered squares — "1" in the top-left, "2" next to it, and so on. The wizard shows a matching canvas preview and asks: *"Does your wall match this layout?"*

Jake looks at the wall. Panel 1 is in the bottom-right. The numbers are backwards. He taps **"No — take me back to fix it."** The wizard returns to hardware settings. He flips the origin corner from 0 to 2 and the scan direction from 0 to 1. Back to Step 6 — this time the numbered panels match the canvas. He taps **"Yes."**

The wall flashes red, then green, then blue — a quick color confirmation that all LEDs are working and the color order is correct. Then: *"Your FlightWall is ready! Fetching your first flights..."* Thirty seconds later, a Delta 737 appears on the display.

Jake never saw garbled output. He never had to debug wiring flags by trial and error. The wizard caught the problem and guided him to fix it before it mattered. And because timezone was auto-detected during setup, night mode will work correctly whenever he enables it — no UTC surprise.

**The moment:** A non-technical user sets up custom LED hardware from a phone, and the device *helps them get it right* instead of dumping them into a garbled display with no explanation.

**Reveals requirements for:** Setup wizard Step 6, auto-run panel position pattern, canvas preview in wizard, visual confirmation prompt ("Does your wall match?"), back-navigation to hardware settings on "No", RGB color test sequence on "Yes", transition to flight fetching after confirmation, browser timezone auto-detection (`Intl.DateTimeFormat`) in Location step, IANA-to-POSIX timezone mapping

### Journey 4: The Bad Flash — "It Didn't Take"

Christian is experimenting with a new display rendering approach. He builds a firmware binary, uploads it via the dashboard. The progress bar completes, the wall reboots... and nothing happens. The LEDs stay dark. Five seconds pass. Ten seconds. Something's wrong — the new firmware has a bug that crashes during display initialization.

But the ESP32 is not bricked. The new firmware booted, tried to initialize, and crashed before it could call `esp_ota_mark_app_valid_cancel_rollback()`. The watchdog fires, the device reboots, and the bootloader notices the active partition was never marked valid. It rolls back to the previous partition automatically.

The wall comes back to life — the old firmware is running, flights are displaying, the dashboard is accessible. Christian opens `/api/status` and sees the version string is the *previous* version, confirming the rollback happened. He opens the Firmware card — it shows "Firmware rolled back to previous version" in a warning toast.

He fixes the bug in his code, builds a new binary, uploads again. This time the wall reboots, WiFi connects, and the **mark-valid gate** (F3) runs: `esp_ota_mark_app_valid_cancel_rollback()` is called within the WiFi-or-60s window and the new partition is marked valid. Clean update.

A month later, Christian pushes firmware that boots, connects WiFi, and passes the **F3** gate — but the dashboard or display has a subtle bug. **Rollback does not trigger** because the device did not crash before mark-valid. He still fixes it by upload if the **OTA endpoint remains reachable**; if a future build broke the web server entirely, he would need USB. He notes: **F3** optimizes for crash detection and simple boot logic, not full HTTP health proofs.

**The moment:** A failed firmware update doesn't mean a trip to the workbench with a USB cable. The device recovers on its own or stays reachable for a quick fix.

**Reveals requirements for:** OTA mark-valid gate (**Decision F3**: WiFi connect or 60s timeout, then `esp_ota_mark_app_valid_cancel_rollback()`), watchdog-triggered rollback if crash before mark-valid, rollback detection and user notification (`rollback_detected` in `/api/status`), version string in `/api/status`, graceful recovery from failed OTA when the bootloader invalidates the bad partition

### Journey Requirements Summary

| Capability | J1 OTA Update | J2 Night Mode | J3 Onboarding | J4 Bad Flash |
|-----------|:-------------:|:-------------:|:--------------:|:------------:|
| Settings export (`/api/settings/export`) | x | | | |
| Pre-migration guidance | x | | | |
| Dual-OTA partition table | x | | | x |
| `/api/ota/upload` endpoint | x | | | x |
| Binary size validation | x | | | x |
| Upload progress feedback | x | | | x |
| OTA mark-valid gate (F3) | x | | | x |
| Watchdog rollback on crash | | | | x |
| Rollback detection + notification | | | | x |
| Firmware version in `/api/status` | x | | | x |
| Dashboard Firmware card | x | | | x |
| NTP time sync (`configTzTime()`) | | x | | |
| Timezone config key + selector | | x | x | |
| Browser timezone auto-detection | | | x | |
| IANA-to-POSIX timezone mapping | | | x | |
| Schedule dim start/end/brightness keys | | x | | |
| Midnight-crossing time logic | | x | | |
| Dashboard Night Mode card | | x | | |
| Wizard Step 6 panel position test | | | x | |
| Canvas preview in wizard | | | x | |
| Visual confirmation prompt | | | x | |
| Back-navigation on mismatch | | | x | |
| RGB color test sequence | | | x | |
| Boot-time mark-valid (F3) + recovery paths | | | | x |

## IoT/Embedded Technical Requirements

### Hardware Reference

No hardware changes from the original PRD. Same ESP-WROOM-32, WS2812B panels, 5V PSU, level shifter. The Foundation Release is firmware-only.

### Memory Budget (Post-Migration)

| Partition | Size | Contents |
|-----------|------|----------|
| `app0` | ~1.5MB | Active firmware |
| `app1` | ~1.5MB | OTA staging partition |
| `spiffs` (LittleFS) | ~896KB | Web assets (~80KB gz), 99 logos (~198KB), free ~618KB |

- **Breaking change from MVP:** LittleFS shrinks from ~2MB to ~896KB (56% reduction)
- Firmware binary must stay under ~1.5MB
- **Pre-implementation gate:** Measure current firmware binary size before starting. If current size exceeds 1.3MB, evaluate library optimizations (disable Bluetooth with `CONFIG_BT_ENABLED 0`, strip unused ArduinoJson features, review FastLED compile flags) before adding new code
- RAM budget unchanged — OTA upload streams directly to flash, no RAM buffering
- NTP and timezone add negligible RAM overhead (~200 bytes for time structs)

### Connectivity

- **NTP:** Required for night mode. `configTzTime()` called after WiFi STA connection. NTP servers: `pool.ntp.org`, `time.nist.gov` (configurable). Fails silently if unreachable — night mode simply won't activate until time syncs. Dashboard Night Mode card must show sync status: "Clock not set — schedule inactive" when NTP hasn't synced, "Clock synced" when active
- **OTA:** No new external connectivity. Firmware upload is local (browser → ESP32 over LAN). No phone-home, no GitHub checks (that's post-MVP)
- **WiFi:** No changes. AP mode, STA mode, and fallback behavior identical to MVP

### Power Profile

No changes. OTA flash writes draw slightly more current during the ~30s update window but well within PSU budget.

### Security Model

- **OTA input validation:** The `/api/ota/upload` endpoint must validate uploads before flashing:
  1. Check `Content-Length` against partition capacity — reject oversized binaries
  2. Read the first byte and verify ESP32 image magic byte (`0xE9`) — reject non-firmware files with clear error: "Not a valid ESP32 firmware image"
  3. `Update.abort()` on stream failure or WiFi interruption
- **OTA attack surface:** No signature verification (acceptable for single-user trusted network). No authentication on the endpoint (consistent with existing dashboard — local network only)
- **NVS expansion:** Five new config keys (`timezone`, `schedule_enabled`, `schedule_dim_start`, `schedule_dim_end`, `schedule_dim_brightness`) stored in NVS. No secrets — all user preferences
- **No new external API calls.** NTP uses UDP (port 123), not HTTPS

### Update Mechanism

- **This release enables OTA.** The partition table changes from single-app to dual-OTA layout
- One-time USB reflash required to apply the new partition table. After that, all updates go through `/api/ota/upload`
- **Settings export for migration:** `GET /api/settings/export` returns full config as JSON file download (`Content-Disposition: attachment; filename=settings.json`). Includes all keys (WiFi password included — single-user local device). User downloads before repartitioning to reference during reconfiguration
- `Update.h` (ESP32 Arduino core) handles the flash write. `esp_ota_mark_app_valid_cancel_rollback()` handles safe rollback
- No automatic updates — user-initiated only via dashboard upload

### Known Trade-offs

*These are architectural design decisions. For operational risks and mitigations, see the Risk Mitigation Strategy table in Product Scope.*

| Trade-off | Decision | Rationale |
|-----------|----------|-----------|
| LittleFS reduced 56% | Accept — 618KB free is sufficient | OTA requires dual app partitions; logo count and web assets fit comfortably |
| No OTA signature verification | Accept for now | Single-user, local network only. Add signing if the project goes public |
| NTP required for night mode | Accept — graceful degradation | Night mode doesn't activate until time syncs. Dashboard shows sync status. No crash, no error |
| One-time migration erases NVS + LittleFS | Mitigate with settings export | `/api/settings/export` lets user back up config before migration |
| No OTA firmware authentication | Mitigate with magic byte check | `0xE9` validation prevents accidental non-firmware uploads. Full signing is post-MVP |
| OTA mark-valid does not probe HTTP | Accept — matches F3 | WiFi-or-timeout proves enough for rollback prevention; HTTP checks deferred to avoid complexity; see risk row above |

## Functional Requirements

### OTA Firmware Updates

- **FR1:** User can upload a firmware binary file to the device through the web dashboard
- **FR2:** Device can validate an uploaded firmware binary for correct size and ESP32 image format before writing to flash
- **FR3:** Device can write a validated firmware binary to the inactive OTA partition without disrupting current operation
- **FR4:** Device can reboot into newly flashed firmware after a successful OTA write
- **FR5:** After booting new firmware from OTA, device can complete a **boot-time health gate** and mark the partition valid: when **WiFi STA connects** or **60 seconds** have elapsed since boot (whichever comes first), call `esp_ota_mark_app_valid_cancel_rollback()` — **Architecture Decision F3** (no self-HTTP check to the web server or OTA endpoint)
- **FR6:** Device can automatically roll back to the previous firmware partition if the new firmware **crashes or fails to reach mark-valid** (e.g. watchdog before `esp_ota_mark_app_valid_cancel_rollback()`)
- **FR7:** Device can detect that a rollback occurred and notify the user through the dashboard
- **FR8:** Device can abort an in-progress firmware upload if the connection is interrupted, leaving the current firmware unaffected
- **FR9:** User can view the currently running firmware version on the dashboard and through the status API
- **FR10:** User can view a progress indicator showing percentage of bytes transferred, updated at least every 2 seconds, during a firmware upload
- **FR11:** Device can return a specific error message to the user when a firmware upload is rejected (oversized binary, invalid format, or transfer failure)

### Settings Migration

- **FR12:** User can export all current device settings as a downloadable JSON file from the dashboard
- **FR13:** User can import a previously exported settings JSON file during the setup wizard to pre-fill configuration fields
- **FR14:** Setup wizard can validate an imported settings file and ignore unrecognized keys without failing

### Night Mode & Brightness Schedule

- **FR15:** Device can synchronize its clock with an NTP time server after connecting to WiFi
- **FR16:** User can configure a timezone for the device through the dashboard or setup wizard
- **FR17:** Device can auto-detect the user's timezone from the browser during setup and suggest it as the default
- **FR18:** Device can convert an IANA timezone identifier (e.g., `America/Los_Angeles`) to its POSIX equivalent for use with `configTzTime()`
- **FR19:** User can define a brightness schedule with a start time, end time, and dim brightness level
- **FR20:** Device can automatically adjust LED brightness according to the configured schedule, including schedules that cross midnight
- **FR21:** Device can maintain correct schedule behavior across daylight saving time transitions
- **FR22:** User can view the current NTP sync status on the dashboard (synced vs. not synced, schedule active vs. inactive)
- **FR23:** User can enable or disable the brightness schedule without deleting the configured times
- **FR24:** Device must not activate the brightness schedule until NTP time has been successfully synchronized

### Onboarding & Setup Polish

- **FR25:** Setup wizard can display a panel position test pattern on the LED matrix and a matching visual preview in the browser simultaneously
- **FR26:** User can confirm or deny that the physical LED layout matches the expected preview during setup
- **FR27:** Setup wizard can return the user to hardware configuration settings if the user reports a layout mismatch
- **FR28:** Setup wizard can run an RGB color sequence test on the LED matrix after the user confirms layout correctness
- **FR29:** Setup wizard can transition to flight fetching after the user confirms both layout and color tests pass

### Dashboard UI

- **FR30:** User can access a Firmware card on the dashboard to upload firmware and view the current version
- **FR31:** User can access a Night Mode card on the dashboard to configure brightness schedule and timezone
- **FR32:** User can access a settings export function from the dashboard System card
- **FR33:** Dashboard can display a warning toast when a firmware rollback has been detected
- **FR34:** Dashboard can display NTP sync status and schedule state (Active / Inactive) within the Night Mode card

### System & Resilience

- **FR35:** Device can operate with a dual-OTA partition table while maintaining all existing functionality (flight data pipeline, web dashboard, logo management, calibration tools)
- **FR36:** Device can continue running current firmware if an OTA upload fails at any stage (validation, transfer, or boot)
- **FR37:** Device can persist all new configuration keys (timezone, schedule settings, schedule enabled flag) to non-volatile storage that survives power cycles and OTA updates

## Non-Functional Requirements

### Performance

- OTA firmware upload must complete within 30 seconds for a 1.5MB binary over local WiFi. Uploads exceeding 45 seconds should be treated as potential failures and logged
- OTA upload progress reported to the dashboard must update at least every 2 seconds during transfer
- Display settings changes triggered by the night mode scheduler must be reflected on the LED matrix within 1 second (consistent with existing hot-reload behavior)
- NTP time synchronization must complete within 10 seconds of WiFi STA connection
- Setup wizard Step 6 (Test Your Wall) pattern must render on the LED matrix within 500ms of being triggered
- Dashboard pages (including new Firmware and Night Mode cards) must load within 3 seconds on the local network
- OTA upload validation (size check + magic byte) must complete and return an error response within 1 second if the file is invalid

### Reliability

- Device must continue operating uninterrupted for 30+ days, including correct night mode schedule execution across midnight boundaries and DST transitions
- Device must re-synchronize with NTP at least once every 24 hours while WiFi is connected. If NTP re-sync fails, the device continues using the last known time without disrupting schedule operation
- OTA **mark-valid gate** (F3): device must call `esp_ota_mark_app_valid_cancel_rollback()` after WiFi STA connects or no later than **60 seconds** after boot; firmware that crashes before that call must be rolled back by the bootloader/watchdog path. (No separate 30s HTTP self-check — superseded by F3.)
- Night mode schedule must survive power cycles — schedule configuration persists in NVS and resumes correct behavior after reboot once NTP re-syncs
- NTP sync failure must not crash the device or degrade any functionality other than schedule activation
- Settings export must produce a valid, complete JSON file that can be re-imported without data loss or corruption
- A failed or interrupted OTA upload must never leave the device in a state where the current firmware stops running or the dashboard becomes inaccessible

### Resource Constraints

- Total firmware binary must not exceed 1.5MB (OTA partition size)
- LittleFS must retain at least 500KB free after all web assets and bundled logos are stored
- New configuration keys (timezone, schedule settings, schedule enabled) must add no more than 256 bytes to NVS usage
- OTA upload must stream directly to flash — no full-binary RAM buffering
- Night mode scheduler must not increase main loop cycle time by more than 1% — limited to a non-blocking time comparison per iteration with no I/O or blocking operations
