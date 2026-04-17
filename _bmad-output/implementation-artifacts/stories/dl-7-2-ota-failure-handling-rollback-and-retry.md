# Story dl-7.2: OTA Failure Handling, Rollback, and Retry

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want firmware updates to fail safely — never bricking the device, always showing clear status, and allowing retry,
So that I can update with confidence knowing the worst case is a brief reboot back to the previous version.

## Acceptance Criteria

1. **Given** **`OTAUpdater`** pull download (**`dl-7-1`**) has called **`Update.begin()`**, **When** any **failure** occurs (**HTTP** read error, **WiFi** drop, **timeout**, **write** mismatch, **SHA** mismatch handled in **`dl-7-1`**), **Then** every path calls **`Update.abort()`** before leaving the download task (**rule 25** / **epic**). **Inactive** OTA partition must **not** be marked bootable (**epic** — partial write acceptable only if **never** **`esp_ota_set_boot_partition`**).

2. **Given** **WiFi** drops mid-**binary** stream, **When** the task handles it, **Then** **`OTAState::ERROR`**, **`getLastError()`** includes **“Download failed — tap to retry”** (or **exact** **epic** string) and **`SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR, …)`** (see **AC #6**) with a **message** that encodes **phase** = **download** (**epic** **FR30**).

3. **Given** **SHA** / **`Update.end`** failure path (**verify** phase), **When** surfaced to **`SystemStatus`**, **Then** message encodes **phase** = **verify** and **outcome** = **retriable** (firmware on device **unchanged** — still running **previous** app from **active** partition) (**epic** **FR30**).

4. **Given** **bootloader** **A/B** behavior (**`custom_partitions.csv`** **app0/app1**), **When** new image fails **boot** validation, **Then** capture **Espressif** **OTA** **rollback** to the **previous** partition (**FR28** / **NFR12**) in **`OTAUpdater` header comment** or **`_bmad-output/planning-artifacts/architecture.md`** (whichever the project already uses for OTA notes) — **no** new **custom** bootloader unless **already** required (**assume** stock **Arduino-ESP32** **partition** scheme is sufficient).

5. **Given** **`ModeRegistry::prepareForOTA()`** (**`dl-7-1`**) left the display pipeline in a **“OTA prep”** state, **When** pull OTA **fails** (any **ERROR** exit **before** successful **`ESP.restart()`**), **Then** implement **`ModeRegistry::completeOTAAttempt(bool success)`** (or **named** equivalent) that **restores** normal **`ModeRegistry::tick`** behavior (**clears** **`SWITCHING`** / **re-inits** active mode or **requests** **previous** **manual** mode per **architecture**) so **flight** rendering resumes **without** requiring a **power cycle** (**epic** **FR37** prerequisite). **Call** this from the **download** task **finally**-equivalent path on **failure** (and **no-op** on **success** **before** reboot if applicable).

6. **Given** **`SystemStatus`**, **When** this story lands, **Then** extend **`Subsystem`** with **`OTA_PULL`** **appended** after **`NTP`** (preserve existing enum **numeric** order for **`OTA`** and below — **append** only), bump **`SUBSYSTEM_COUNT`**, add **`subsystemName()`** / **`toJson`** mapping **`"ota_pull"`**, and **`init()`** default row for the new index (**epic**).

7. **Given** **`GET /api/status`**, **When** **`SystemStatus::toExtendedJson`** runs, **Then** **`subsystems.ota_pull`** appears with **level/message** reflecting **pull** check/download/verify (**distinct** from **`subsystems.ota`** used for **push** upload path — **epic**).

8. **Given** **LED** **matrix** during **pull** OTA (**`dl-7-1`** **“Updating…”**), **When** **failure** occurs, **Then** show a **distinct** **visual** for **ERROR** (**e.g.** **“Update failed”** + **brief** **color** / **pattern** difference) **before** **`completeOTAAttempt(false)`** returns the pipeline to **normal** flight UI (**epic** **FR30**). **Do not** leave the matrix **blank** **> 1 s** except during **intentional** full-screen **OTA** states (**epic** **FR35** / **NFR19**).

9. **Given** **flight** fetch pipeline (**`FlightDataFetcher`** / **Core** scheduling), **When** **OTA pull** errors or runs, **Then** **do not** block **fetch** **timers** or **share** **locks** that would stall **OpenSky**/**AeroAPI** paths (**FR35** / **NFR18**).

10. **Given** **`firmware/data-src/health.html`** / **`health.js`**, **When** **`OTA_PULL`** exists, **Then** render **OTA Pull** status (read **`data.subsystems.ota_pull`**) in a **small** **Device** subsection or **new** **card** — follow **existing** **`dotRow`** pattern (**epic**). Regenerate **`firmware/data/health.js.gz`** (and **`.html.gz`** if **HTML** changed).

11. **Given** **`OTAUpdater`**, **When** exposing failures to **`dl-7.3`**, **Then** add **structured** accessors **e.g.** **`getFailurePhase()`** (**enum**: **none**, **download**, **verify**, **boot**) and **`isRetriable()`** **or** encode **(a)/(b)/(c)** in **`getLastError()`** **plus** **machine-readable** fields on **`GET /api/ota/status`** (**dl-7-3** story will consume — document **JSON** **shape** in **Dev Notes**).

12. **Given** **`pio test`** / **`pio run`**, **Then** add tests for **`Update.abort()`** **coverage** via **mocks** if available, **`subsystemName(OTA_PULL)`**, and **any** **pure** **parser** helpers; **no** new warnings.

## Tasks / Subtasks

- [x] Task 1 — **`OTAUpdater.cpp`** — unify **error** paths, **`Update.abort()`**, **messages**, **`SystemStatus::OTA_PULL`**, **structured** failure metadata (**AC: #1–#3, #11**)
- [x] Task 2 — **`ModeRegistry`** — **`completeOTAAttempt`** (or equivalent) **unwind** on failure (**AC: #5**)
- [x] Task 3 — **Display / LED** — **failure** **visual** vs **progress** (**AC: #8**)
- [x] Task 4 — **`SystemStatus.h/cpp`** — **`OTA_PULL`**, **count**, **names** (**AC: #6–#7**)
- [x] Task 5 — **`health.html` / `health.js`** + **gzip** (**AC: #10**)
- [x] Task 6 — **Docs** — **bootloader** **rollback** note (**AC: #4**)
- [x] Task 7 — **Tests** (**AC: #12**)

## Dev Notes

### Prerequisite

- **`dl-7-1`** implements **download** task, **`prepareForOTA()`**, **`mbedtls`** **verify**, **`Update.begin`**.

### Scope boundary (**dl-7-3**)

- **`POST /api/ota/pull`**, **`GET /api/ota/status`**, dashboard **Retry** **button** wiring — **dl-7-3**. This story **must** still expose enough **state**/**errors** that **dl-7-3** can **poll** and **retry** **`startDownload()`** (**FR37**).

### Independence

- **GitHub** **check** failures remain **`dl-6.x`**; this story focuses on **pull** **download/verify** and **recovery**.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-7.md` — Story **dl-7.2**
- Prior: `_bmad-output/implementation-artifacts/stories/dl-7-1-ota-download-with-incremental-sha-256-verification.md`
- **Push OTA:** `firmware/adapters/WebPortal.cpp` — **`Update.abort()`** on disconnect ~588–595
- **Health:** `firmware/data-src/health.js`, `firmware/data-src/health.html`
- **`SystemStatus`:** `firmware/core/SystemStatus.h`, `firmware/core/SystemStatus.cpp`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, 80.8% flash, no new warnings
- Tests: `pio test -f test_ota_updater` — compilation PASSED (on-device execution requires hardware)

### Completion Notes List

1. **Task 4 (SystemStatus OTA_PULL):** Added `OTA_PULL` to `Subsystem` enum after `NTP` (index 8), bumped `SUBSYSTEM_COUNT` to 9, added `"ota_pull"` in `subsystemName()` switch. The `init()` loop auto-covers the new index. `toJson()`/`toExtendedJson()` automatically includes the new subsystem.

2. **Task 2 (ModeRegistry::completeOTAAttempt):** Added `completeOTAAttempt(bool success)` to ModeRegistry. On failure: holds OTA mode for 3s (AC #8 error visual), then clears `_otaMode`, resets `SwitchState::IDLE`, and requests the first mode in the table (typically "classic_card") so flight rendering resumes without power cycle. On success: no-op (device reboots).

3. **Task 1 (OTAUpdater error paths):** Added `OTAFailurePhase` enum (NONE, DOWNLOAD, VERIFY, BOOT) and `_failurePhase` state. Added `getFailurePhase()` and `isRetriable()` accessors. Refactored every error path in `_downloadTaskProc` to: (a) set `_failurePhase`, (b) call `SystemStatus::set(Subsystem::OTA_PULL, ...)` with phase-encoded message, (c) call `Update.abort()` after `Update.begin()`, (d) call `ModeRegistry::completeOTAAttempt(false)`. Download-phase errors use "Download failed — tap to retry" message per epic. Verify-phase errors use "Verify failed — integrity mismatch/finalize error". Documented JSON shape for dl-7.3: `{ "state", "progress", "error", "failure_phase", "retriable" }`.

4. **Task 3 (Display/LED failure visual):** Modified `displayTask()` in main.cpp to check `OTAUpdater::getState() == ERROR` during OTA mode and display "Update failed" instead of "Updating...". The 3s delay in `completeOTAAttempt(false)` ensures ~15 frames of the error visual at 5fps before flight rendering resumes.

5. **Task 5 (Health page):** Added OTA Pull status rendering in `renderDevice()` using the existing `dotRow` pattern. Reads `data.subsystems.ota_pull` from `/api/status`. Regenerated `firmware/data/health.js.gz`.

6. **Task 6 (Docs):** Added bootloader A/B rollback documentation to `OTAUpdater.h` header comment and `_downloadTaskProc` function comment: stock Arduino-ESP32 partition scheme with `app0/app1` + `esp_ota_mark_app_valid_cancel_rollback()` mechanism.

7. **Task 7 (Tests):** Added 5 new tests to `test_ota_updater/test_main.cpp`: failure phase initial NONE, isRetriable false when NONE, failure phase enum value validation, OTA_PULL subsystem index validation, OTA_PULL get/set via SystemStatus. All compile clean.

### File List

- `firmware/core/OTAUpdater.h` — modified (added OTAFailurePhase enum, getFailurePhase, isRetriable, _failurePhase, bootloader rollback docs)
- `firmware/core/OTAUpdater.cpp` — modified (added SystemStatus include, failure phase accessors, refactored all error paths with SystemStatus::OTA_PULL, phase, completeOTAAttempt)
- `firmware/core/ModeRegistry.h` — modified (added completeOTAAttempt declaration)
- `firmware/core/ModeRegistry.cpp` — modified (added completeOTAAttempt implementation with 3s error visual delay)
- `firmware/core/SystemStatus.h` — modified (added OTA_PULL to Subsystem enum, bumped SUBSYSTEM_COUNT to 9)
- `firmware/core/SystemStatus.cpp` — modified (added "ota_pull" in subsystemName switch)
- `firmware/src/main.cpp` — modified (display task shows "Update failed" when OTA state is ERROR)
- `firmware/data-src/health.js` — modified (added OTA Pull dotRow in renderDevice)
- `firmware/data/health.js.gz` — regenerated
- `firmware/test/test_ota_updater/test_main.cpp` — modified (added 5 new tests for dl-7.2)

## Previous story intelligence

- **`dl-7-1`** adds **happy path** **reboot**; **this** story hardens **sad paths** and **operator** **visibility**.

## Git intelligence summary

Touches **`OTAUpdater.*`**, **`ModeRegistry.*`**, display/LED path, **`SystemStatus.*`**, **`health.*`**, **`firmware/data/*.gz`**, tests/docs.

## Project context reference

`_bmad-output/project-context.md` — **OTA** / **partition** context.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
