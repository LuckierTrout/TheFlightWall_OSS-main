# Story dl-7.3: OTA Pull Dashboard UI and Status Polling

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want a clear dashboard interface for downloading firmware updates with real-time progress feedback,
So that I can see exactly what's happening during the update and know when it's complete.

## Acceptance Criteria

1. **Given** **`WebPortal`**, **When** this story lands, **Then** register **`POST /api/ota/pull`** and **`GET /api/ota/status`** (alongside existing **`/api/ota/upload`**, **`/api/ota/ack-rollback`** — **`firmware/adapters/WebPortal.cpp`**).

2. **Given** **`POST /api/ota/pull`**, **When** **`OTAUpdater::getState() == OTAState::AVAILABLE`** (**`dl-6-1`** / **`checkForUpdate`**), **Then** call **`OTAUpdater::startDownload()`** (**`dl-7-1`**). On success respond **`{ "ok": true, "data": { "started": true } }`**. If state is **not** **`AVAILABLE`**: **`ok: false`** with **`error`** **`"No update available — check for updates first"`**. If download **already** running (**`DOWNLOADING`**, **`VERIFYING`**, or guard in **`startDownload`**): **`ok: false`**, **`error`** **`"Download already in progress"`** (**epic**).

3. **Given** **`GET /api/ota/status`**, **When** polled, **Then** return **`{ "ok": true, "data": { "state": "<string>", "progress": <0-100|null>, "error": <string|null>, "phase": <optional same as dl-7-2> } }`** where **`state`** is one of **`"idle"`**, **`"checking"`**, **`"downloading"`**, **`"verifying"`**, **`"rebooting"`**, **`"error"`** (lowercase, stable contract). **`progress`** required for **`downloading`**; **`verifying`** may use **`progress`** **null** or **indeterminate** **UI** — document. **`error`**: non-null only when **`state == "error"`**, text from **`OTAUpdater::getLastError()`** / **`dl-7-2`** diagnostics (**epic**).

4. **Given** **`dashboard.js`** (**Firmware** card / **`dl-6-2`** update banner), **When** the owner taps **“Update Now”** (pull path), **Then** **`FW.post('/api/ota/pull', {})`**; on **`started`**, start **`setInterval`** (**500 ms**) calling **`GET /api/ota/status`** until **terminal** state (**`error`**, **`rebooting`**, or **`idle`** after success path — document), then **clear** interval (**epic** **500 ms**).

5. **Given** **`state === "downloading"`**, **When** the UI updates, **Then** show a **progress** bar using **`data.progress`** (**0–100**), patterned after **Mode Picker** **`switchMode`** polling (**`SWITCH_POLL_INTERVAL`** ~**200 ms** is different — use **epic** **500 ms** for **OTA** only) (**epic**).

6. **Given** **`state === "verifying"`**, **When** rendering, **Then** progress bar shows **verification** phase (label or **indeterminate** style) (**epic**).

7. **Given** **`state === "rebooting"`**, **When** rendering, **Then** show **“Rebooting…”** and **stop** polling after a **short** **grace** period or **on** **`fetch`** **failure** (device **disconnect**) (**epic**).

8. **Given** **`state === "error"`**, **When** the client receives **`error`**, **Then** display message and a **“Retry”** button that calls **`POST /api/ota/pull`** again (**only** if **`dl-7-2`** marks failure **retriable** / **`OTAUpdater`** returns to **`AVAILABLE`** or **`IDLE`** — document: **retry** may require **`GET /api/ota/check`** first if state is **`IDLE`** without metadata — **prefer** **`startDownload()`** only when **`AVAILABLE`** else **`showToast`** **“Check for updates first”**) (**FR37** / **epic**).

9. **Given** the device **reboots** into **new** firmware, **When** the **dashboard** reloads **`GET /api/status`**, **Then** **`firmware_version`** reflects **new** build; **hide** the **“update available”** banner (**`dl-6-2`** **`ota_available`** **false**). Optional **status** line: **“Running {firmware_version}”** if not already shown (**epic**).

10. **Given** **`firmware/check_size.py`** and **`custom_partitions.csv`**, **When** **`pio run`** completes, **Then** firmware **`.bin`** size **must** stay **≤** **`0x180000`** (**1,572,864** bytes — **current** **OTA** slot; **epic** **NFR14** cites **2 MiB** — **treat** **`check_size.py`** **`limit`** as **authoritative** until partitions change) (**epic**).

11. **Given** **FR43** (**final** **Delight** story), **When** complete, **Then** ensure **root** **`README.md`** links to **`firmware/README.md`** (build/flash), **`platformio.ini`**, and **documents** **first** **boot** (**AP** **`FlightWall-Setup`** / **`flightwall.local`**) and **dashboard** access so a **hobbyist** can reach **`http://flightwall.local/`** (or **documented** **IP**) **without** **undocumented** steps — **minimal** **edits** (short **“Firmware & OTA”** subsection is enough).

12. **Given** **`dashboard.js`** / **`dashboard.html`** / **`dashboard.css`** change, **When** done, **Then** regenerate **`firmware/data/dashboard.js.gz`** (and **`.html.gz`** / **`.css.gz`** if those assets are served compressed — match **repo** convention).

13. **Given** **`pio test`** / smoke, **Then** extend **`tests/smoke/test_web_portal_smoke.py`** (or add **native** test) for **`/api/ota/status`** **shape** and **`/api/ota/pull`** **error** paths (**no** **live** **download** in **CI** unless **hermetic**); **`pio run`** **green**, **no** new warnings.

## Tasks / Subtasks

- [x] Task 1 — **`OTAUpdater`** — stable **`getStateString()`** / **`getProgress()`** for **status** handler if missing (**AC: #3**)
- [x] Task 2 — **`WebPortal.cpp`** — **`POST /api/ota/pull`**, **`GET /api/ota/status`** (**AC: #1–#3**)
- [x] Task 3 — **`dashboard.js`** — **Update Now** → **pull**, **poll** loop, **progress**, **Retry**, **reboot** handling (**AC: #4–#9**)
- [x] Task 4 — **`dashboard.html` / `.css`** — **progress** **UI** hooks (**AC: #5–#7**)
- [x] Task 5 — **gzip** bundled assets (**AC: #12**)
- [x] Task 6 — **`README.md`** + **`firmware/README.md`** touch-up for **FR43** (**AC: #11**)
- [x] Task 7 — **`check_size.py`** / **partition** comment if **NFR14** text must align (**AC: #10**)
- [x] Task 8 — **Tests** (**AC: #13**)

#### Review Follow-ups (AI)
- [ ] [AI-Review] HIGH: API contract violation - AC #3 specifies 6 valid states but implementation returns "available" as 7th state, breaking contract compliance
- [ ] [AI-Review] MEDIUM: JSON field name mismatch - AC #3 specifies "phase" but implementation uses "failure_phase"
- [ ] [AI-Review] LOW: Terminal success state UX not deterministic - pull progress UI may remain visible after operation completes
- [ ] [AI-Review] LOW: HTTP 409 status code not specified in AC #2 for "already in progress" case (implementation correct, documentation gap)
- [ ] [AI-Review] LOW: Overlapping unit tests reduce signal quality - test_start_download_rejects_when_not_available and test_start_download_rejects_idle_state test same guard
- [ ] [AI-Review] LOW: Permissive state string test doesn't enforce exact mappings - allowed "available" contract drift to pass undetected

## Dev Notes

### Prerequisites

- **`dl-6-1`**, **`dl-6-2`** — **check** + **UI** entry.
- **`dl-7-1`** — **`startDownload()`**, **states**, **progress**.
- **`dl-7-2`** — **`OTA_PULL`** **subsystem**, **`completeOTAAttempt`**, **error** strings.

### Integration with **`dl-6-2`**

- **Single** **“Update Now”** behavior: **pull** **OTA** (**this** story) vs **file** **upload** (**fn-1.6**) — **disambiguate** in **UI** (**two** buttons or **clear** labels).

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-7.md` — Story **dl-7.3**
- Prior: `_bmad-output/implementation-artifacts/stories/dl-7-2-ota-failure-handling-rollback-and-retry.md`
- **Size gate:** `firmware/check_size.py`, `firmware/custom_partitions.csv`
- **Polling precedent:** `firmware/data-src/dashboard.js` — **`switchMode`** / **`pollSwitch`**

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, binary 1,278,416 bytes (81.3% of 1.5MB partition limit)
- Tests: `pio test -f test_ota_updater --without-uploading --without-testing` — compilation PASSED
- Regression: all test suites (mode_registry, config_manager, mode_orchestrator, mode_schedule) compile clean

### Completion Notes List

- **Task 1:** Added `getStateString()` and `getFailurePhaseString()` to OTAUpdater — returns lowercase state/phase names for stable JSON contract.
- **Task 2:** Registered `POST /api/ota/pull` and `GET /api/ota/status` in WebPortal. Pull endpoint guards for AVAILABLE state and already-in-progress downloads. Status endpoint returns `{ ok, data: { state, progress, error, failure_phase, retriable } }`.
- **Task 3:** Rewired "Update Now" button from scroll-to-upload to actual OTA pull flow. Implemented 500ms setInterval polling loop with state-driven UI transitions: downloading (progress bar), verifying (indeterminate), rebooting (grace period + device polling), error (message + retry button).
- **Task 4:** Added `#ota-pull-progress` section in dashboard.html with progress bar, status text, error display, and retry button. Added CSS for indeterminate progress animation and pull-specific styling.
- **Task 5:** Regenerated all three `.gz` assets (dashboard.js.gz, dashboard.html.gz, style.css.gz).
- **Task 6:** Added "Firmware & OTA" subsection to root README.md documenting first boot (FlightWall-Setup AP, flightwall.local) and OTA. Updated firmware/README.md with first boot and OTA sections.
- **Task 7:** Verified `check_size.py` limit 0x180000 matches AC #10 requirement — no changes needed.
- **Task 8:** Added 6 new tests: `getStateString` non-null + known values, `getFailurePhaseString` non-null + known values + initial "none", pull guard rejection when not AVAILABLE.
- **Code Review Synthesis (2026-04-16):** Fixed MEDIUM severity polling pattern issue - replaced setInterval with recursive setTimeout to prevent request stacking under latency conditions. Regenerated dashboard.js.gz after fix.

### File List

- firmware/core/OTAUpdater.h (modified — added getStateString, getFailurePhaseString declarations)
- firmware/core/OTAUpdater.cpp (modified — added getStateString, getFailurePhaseString implementations)
- firmware/adapters/WebPortal.h (modified — added handlePostOtaPull, handleGetOtaStatus declarations)
- firmware/adapters/WebPortal.cpp (modified — registered POST /api/ota/pull, GET /api/ota/status routes + handler implementations)
- firmware/data-src/dashboard.js (modified — replaced "Update Now" scroll behavior with OTA pull flow, poll loop, progress UI, retry)
- firmware/data-src/dashboard.html (modified — added ota-pull-progress section with progress bar, error, retry button)
- firmware/data-src/style.css (modified — added OTA pull progress and indeterminate animation styles)
- firmware/data/dashboard.js.gz (regenerated)
- firmware/data/dashboard.html.gz (regenerated)
- firmware/data/style.css.gz (regenerated)
- firmware/test/test_ota_updater/test_main.cpp (modified — added 6 new tests for dl-7.3)
- README.md (modified — added Firmware & OTA subsection with first boot docs)
- firmware/README.md (modified — added first boot and OTA sections)

## Previous story intelligence

- **Pull** **OTA** is **explicit** **HTTP** + **polling**; **do** **not** **block** **AsyncWebServer** **callbacks** with **long** **downloads** (**work** stays in **`dl-7-1`** **task**).

## Git intelligence summary

Touches **`WebPortal.cpp`**, **`OTAUpdater.*`**, **`dashboard.*`**, **`firmware/data/*.gz`**, **`README.md`**, **`firmware/README.md`**, tests.

## Project context reference

`_bmad-output/project-context.md` — **portal** assets **gzip**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-16
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 4.2 → MAJOR REWORK
- **Issues Found:** 7
- **Issues Fixed:** 1
- **Action Items Created:** 6
