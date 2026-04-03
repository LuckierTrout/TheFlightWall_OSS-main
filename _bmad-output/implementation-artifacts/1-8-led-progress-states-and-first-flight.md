# Story 1.8: LED Progress States & First Flight

Status: done

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want the LED wall to show clear progress during setup,
so that I know the device is working and can watch it come to life.

## Acceptance Criteria

1. **Post-save progress sequence:** After the wizard's `Save & Connect` flow is triggered, the LED wall shows an ordered startup/progress sequence instead of jumping straight from reboot to generic status text. The sequence must include: `Saving config...` -> `Connecting to WiFi...` -> `WiFi Connected ✓` -> `IP: [address]` -> `Authenticating APIs...` -> `Fetching flights...` -> first flight card.

2. **WiFi failure message:** If WiFi connection does not recover and the device falls back to setup mode, the LED wall shows a failure message aligned with the setup flow intent, then the device returns to AP/setup behavior.

3. **First fetch startup behavior:** On the first connected boot after setup, the device does not wait an entire normal fetch interval before beginning the first API/auth/fetch attempt. The first-flight path is prioritized so the post-wizard experience can complete promptly.

4. **Return to normal rendering:** Once the first successful flight data is ready, the display returns cleanly from progress text to normal flight rendering. If no flights are available yet, the existing loading/waiting behavior remains the fallback rather than leaving a stale progress message on screen forever.

5. **Time-to-value target:** When WiFi connects within 10 seconds and flights are available, total time from `Save & Connect` to first flight card remains under 60 seconds.

## Tasks / Subtasks

- [x] Task 1: Add a setup-progress coordinator that owns ordered LED status messaging (AC: #1, #4, #5)
  - [x] Extend the existing display-message orchestration in `firmware/src/main.cpp` instead of inventing a second display path.
  - [x] Introduce a small startup/progress state machine or equivalent one-owner coordinator that serializes the setup messages in order.
  - [x] Make the coordinator explicitly able to return control to normal display rendering after progress messaging completes.
  - [x] Do not blast multiple `queueDisplayMessage()` calls back-to-back and assume they will all render; the current queue is single-entry overwrite semantics.

- [x] Task 2: Show `Saving config...` during the reboot/apply handoff without breaking adapter boundaries (AC: #1)
  - [x] Reuse the existing `POST /api/reboot` path from `WebPortal`; do not create a second reboot/save endpoint.
  - [x] Add only the narrowest necessary hook so the firmware can request a display progress message before restart.
  - [x] Keep `WebPortal` as a browser adapter, not a direct display owner. If a callback or notifier is added, keep it minimal and one-directional.
  - [x] Preserve the current behavior where the reboot response is sent before the restart timer fires.

- [x] Task 3: Upgrade the WiFi-to-display transition flow for setup completion (AC: #1, #2)
  - [x] Reuse the existing `WiFiManager` state changes already wired into `main.cpp`.
  - [x] Replace generic startup wording where needed so the user sees setup-specific progress text:
    - `Connecting to WiFi...`
    - `WiFi Connected ✓`
    - `IP: [address]`
  - [x] Update the AP fallback/failure text so it matches the setup recovery intent instead of the current generic AP fallback wording.
  - [x] Keep the IP message visible long enough for discovery, but do not let it block the rest of the startup sequence indefinitely.

- [x] Task 4: Trigger the first auth/fetch cycle promptly after connection (AC: #1, #3, #4, #5)
  - [x] Add a one-shot startup path so the first connected boot does not wait the full configured `fetch_interval` before beginning the first fetch cycle.
  - [x] Show `Authenticating APIs...` immediately before the first startup fetch/auth work begins.
  - [x] Show `Fetching flights...` while the first live data attempt is in progress.
  - [x] When first flight data is ready, clear/complete the progress state so normal rendering takes over.
  - [x] If the first fetch returns no flights, fall back to the existing loading screen instead of leaving `Fetching flights...` stuck on the display forever.

- [x] Task 5: Keep progress orchestration in the right layer (AC: #1, #4)
  - [x] Prefer orchestration in `main.cpp` or a similarly central coordinator, not scattered direct LED calls inside every adapter.
  - [x] Do not push display-side progress text from `OpenSkyFetcher`, `AeroAPIFetcher`, or `FlightWallFetcher`; those currently expose bool-return behavior and should stay focused on network/data retrieval.
  - [x] If a helper is needed for first-fetch progress, keep it small and local to the setup/startup flow.

- [x] Task 6: Verification (AC: #1-#5)
  - [x] Build firmware with `pio run`.
  - [x] Upload filesystem assets with `pio run -t uploadfs` if any web assets changed during implementation.
- [ ] Verify the ordered LED sequence on a real device after the wizard's `Save & Connect`.
- [ ] Verify WiFi failure path shows the new failure message and returns to AP/setup mode.
- [ ] Verify first fetch starts promptly after connect rather than waiting a full steady-state fetch interval.
- [ ] Verify normal flight rendering takes over after progress text, with no stuck progress message.
- [ ] Measure the happy path with WiFi connecting within 10 seconds and at least one live flight available in range; confirm total time to first flight card is under 60 seconds.

### Review Findings

- [x] [Review][Patch] The `Authenticating APIs...` step is never actually shown in the startup sequence because `tickStartupProgress()` queues it and immediately returns `true`, then the same `loop()` iteration transitions to `FETCHING_FLIGHTS` and overwrites the single-entry display queue with `Fetching flights...`, skipping the required ordered message [firmware/src/main.cpp:447]
- [x] [Review][Patch] The startup sequence text does not match AC1 because the `STA_CONNECTED` path still queues `WiFi Connected` instead of the required `WiFi Connected ✓` message [firmware/src/main.cpp:139]

## Dev Notes

### Story Foundation

- Story 1.7 now completes the wizard browser flow and hands off to firmware with `POST /api/settings` followed by `POST /api/reboot`.
- Story 1.8 is the firmware-side payoff: make the LED wall narrate the reboot/connect/first-fetch path clearly.
- This story should not rework the browser wizard flow unless a tiny hook is required to support the display-side sequence.

### Current Implementation Surface

- `firmware/src/main.cpp` already contains the current display-message path:
  - `DisplayStatusMessage`
  - `g_displayMessageQueue`
  - `queueDisplayMessage()`
  - `queueWiFiStateMessage()`
- The display task already owns actual LED writes and can temporarily suppress normal flight rendering while a status message is visible.
- Current WiFi text already exists but is simpler than the story requires:
  - `Setup Mode`
  - `Connecting...`
  - `IP: ...`
  - `WiFi Lost...`
  - `WiFi Failed - Setup Mode`

### Previous Story Intelligence (1.7)

- The wizard source files now live in `firmware/data-src/`, with gzipped copies generated into `firmware/data/` for serving.
- `wizard.js` already calls:
  1. `POST /api/settings` with the full 12-key setup payload
  2. `POST /api/reboot`
  3. then replaces the page with a handoff message telling the user to look at the LED wall
- That means Story 1.8 should hook into the existing reboot/startup path rather than inventing a new save-or-connect sequence.

### Previous Story Intelligence (1.4)

- `WiFiManager` already owns WiFi state transitions and publishes callbacks for:
  - `AP_SETUP`
  - `CONNECTING`
  - `STA_CONNECTED`
  - `STA_RECONNECTING`
  - `AP_FALLBACK`
- `main.cpp` is already subscribed to those state changes and is the correct orchestration layer for user-facing display messages.

### Critical Live-Code Guardrails

- The current display message queue is **length 1** and uses `xQueueOverwrite()`.
- That means if you enqueue several progress strings in rapid succession, intermediate messages can be lost.
- Do not model the startup sequence as "fire six queue writes quickly and hope the display shows them all."
- Use a single coordinator that advances the progress sequence intentionally and knows when control should return to normal rendering.

### First-Fetch Timing Guardrail

- The current `loop()` waits for `now - g_lastFetchMs >= fetch_interval * 1000UL`.
- Because `g_lastFetchMs` starts at `0`, the first fetch happens only after a full interval elapses.
- That is risky for Story 1.8 because it can waste most of the 60-second budget before the first API/auth/fetch even begins.
- The implementation should add a one-shot startup fetch path so the initial post-setup fetch begins promptly after WiFi connection, without changing steady-state interval behavior afterward.

### Progress-Orchestration Guardrails

- `OpenSkyFetcher`, `AeroAPIFetcher`, and `FlightWallFetcher` currently use focused bool-return APIs and should remain data/network adapters, not display-message owners.
- Keep setup progress orchestration near `main.cpp` or a similarly central coordinator where WiFi state, fetch timing, and display message lifetimes are already visible.
- If you need a helper abstraction, keep it narrow and dedicated to startup progress rather than generalizing prematurely.

### Likely File Touches

- `firmware/src/main.cpp` is the primary file for this story.
- `firmware/adapters/WebPortal.h/.cpp` should only change if you need a minimal reboot-progress callback/notifier to support `Saving config...`.
- `firmware/adapters/WiFiManager.cpp` should only change if a tiny adjustment is needed for progress/failure wording or callback timing.
- Browser assets in `firmware/data-src/` should remain untouched unless you truly need a tiny wording update; Story 1.7 already provides the browser-side handoff.

### Architecture Compliance

- Keep display writes owned by the display task / existing display-message pipeline.
- Keep browser routing owned by `WebPortal`.
- Keep WiFi state ownership in `WiFiManager`.
- Keep JSON naming and config behavior unchanged; this story is primarily about orchestration and user-visible progress.

### UX Guardrails

- The LED sequence should feel deliberate and reassuring, not noisy.
- Avoid gratuitous delays added only for theater; the sequence must still meet the under-60-second target.
- The failure state must be specific and recovery-oriented.
- Do not let progress text get stuck if first-flight data is delayed or unavailable.

### Testing Requirements

- Real-device verification is essential; this story is fundamentally hardware/runtime behavior.
- For the `< 60s` first-flight validation, test with:
  - WiFi that connects within 10 seconds
  - at least one flight likely to be available in range
  - a fresh wizard `Save & Connect` handoff
- If no flights are naturally available, do not fake a flight card in production code just to satisfy the story. Use a test scenario with active flights or a temporarily widened radius for validation.

### What Not To Do

- Do not scatter direct `g_display.displayMessage()` calls across fetchers.
- Do not replace the current display-task ownership model with ad hoc cross-core writes.
- Do not leave the first fetch on the normal steady-state timer if it makes the post-wizard experience drag unnecessarily.
- Do not assume the browser remains connected after reboot; Story 1.7 already established that the LED wall is the authoritative progress surface after handoff.
- Do not break the existing loading/waiting behavior when there are no flights yet.

### Project Structure Notes

- There is still no repo commit history to mine; rely on story artifacts and live code as the source of truth.
- No `project-context.md` was found.
- Repo variance remains important: source web assets are edited under `firmware/data-src/`, while served gzipped copies live under `firmware/data/`.

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` - Story 1.8 acceptance criteria, UX-DR19, NFR5]
- [Source: `_bmad-output/planning-artifacts/architecture.md` - WiFiManager ownership, display task on Core 0, startup/fetch architecture]
- [Source: `_bmad-output/planning-artifacts/prd.md` - FR8, FR44, first-boot success criteria]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` - post-wizard transition, progress visibility, under-60s target]
- [Source: `_bmad-output/implementation-artifacts/1-7-setup-wizard-location-hardware-and-review.md` - wizard handoff, data-src/data split, current save/reboot flow]
- [Source: `firmware/src/main.cpp` - existing display message queue, WiFi message flow, current first-fetch timing]
- [Source: `firmware/adapters/WiFiManager.cpp` - WiFi states, callback timing, fallback behavior]
- [Source: `firmware/core/FlightDataFetcher.cpp` - current first-fetch orchestration surface]
- [Source: `firmware/data-src/wizard.js` - current `Save & Connect` implementation and reboot handoff]
- [Source: `firmware/adapters/WebPortal.cpp` - current `/api/reboot` behavior]

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- No git commit history is available yet in this repository.
- The current display-message path uses a single-entry overwrite queue, which is a key implementation risk for ordered progress states.
- The current main loop waits a full fetch interval before the first fetch, which is a key risk against the under-60-second first-flight goal.
- Build verified with `pio run` — SUCCESS (RAM 15.7%, Flash 55.3%).
- Test build verified with `pio test --without-uploading --without-testing` — no compilation regressions.
- No web assets changed, so `uploadfs` is not required for this story.

### Implementation Plan

**StartupPhase state machine** in `main.cpp`:
- `IDLE` -> `SAVING_CONFIG` -> (reboot) -> `CONNECTING_WIFI` -> `WIFI_CONNECTED` -> `AUTHENTICATING` -> `FETCHING_FLIGHTS` -> `COMPLETE`
- Failure branch: `CONNECTING_WIFI` -> `WIFI_FAILED` -> `IDLE`
- Each phase transition driven by real events (WiFi callback, timer elapsed, fetch complete)
- Single coordinator advances the sequence intentionally; no multi-write queue blasting
- `tickStartupProgress()` called from `loop()` on Core 1; returns `true` to trigger first fetch
- `g_firstFetchDone` one-shot flag ensures first fetch bypasses normal interval timer

**WebPortal reboot callback**: Minimal `onReboot(callback)` pattern — WebPortal calls the callback in `_handlePostReboot()` before scheduling restart. Callback registered in `setup()` to queue "Saving config..." message. WebPortal remains a browser adapter, not a display owner.

**WiFi message interception**: During startup progress, `queueWiFiStateMessage()` intercepts STA_CONNECTED and AP_FALLBACK events to advance the coordinator instead of showing generic messages.

### Completion Notes List

- Story context built from live implementation state after Story 1.7, not from planning docs alone.
- Added strong guardrails around ordered status sequencing, first-fetch timing, and keeping orchestration in `main.cpp`.
- Captured the repo's current `data-src` -> `data` asset workflow, even though this story is primarily firmware-side.
- Implemented `StartupPhase` enum with 8 states and `tickStartupProgress()` coordinator in `main.cpp`.
- Added `onReboot()` callback to `WebPortal` for "Saving config..." display before restart.
- Updated WiFi messages: "Connecting..." -> "Connecting to WiFi...", "WiFi Failed - Setup Mode" -> "WiFi Failed" (normal) / "WiFi Failed - Reopen Setup" (startup).
- First-fetch one-shot path triggers immediately after WiFi connect + IP display, bypassing normal `fetch_interval` wait.
- Progress completion sends empty message with 1ms duration to clear status overlay, letting display task resume flight rendering or loading screen.
- WiFiManager.cpp not modified — all changes handled through existing callback wiring.
- No web assets changed — wizard.js handoff from Story 1.7 works as-is.
- Real-device verification subtasks left unchecked — require physical hardware testing by user.

### File List

- `firmware/src/main.cpp` (modified)
- `firmware/adapters/WebPortal.cpp` (modified)
- `firmware/adapters/WebPortal.h` (modified)

## Change Log

- 2026-04-02: Story 1.8 implementation — added startup progress coordinator (StartupPhase state machine), reboot display callback, setup-specific WiFi messages, and first-fetch one-shot timing. Build verified, no regressions.
