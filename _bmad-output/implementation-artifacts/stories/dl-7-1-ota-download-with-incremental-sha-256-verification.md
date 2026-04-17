# Story dl-7.1: OTA Download with Incremental SHA-256 Verification

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want to download and install a firmware update over WiFi with one tap,
So that I can update my device without finding a USB cable, opening PlatformIO, or pulling from git.

## Acceptance Criteria

1. **Given** **`OTAUpdater`** is in **`OTAState::AVAILABLE`** (**URLs** + **metadata** from **`dl-6-1`** **`checkForUpdate()`**), **When** **`OTAUpdater::startDownload()`** is called, **Then** it returns **`true`** after spawning a **one-shot** **FreeRTOS** task (**pinned to Core 1**, **stack ≥ 8192** bytes, **priority 1** per **epic**), sets state **`DOWNLOADING`** (or **`CHECKING`** → **`DOWNLOADING`** transition documented), and returns **immediately** without blocking the caller (**epic**). If a download task is already running (**`_downloadTaskHandle != nullptr`**), **`startDownload()`** returns **`false`** (**epic**).

2. **Given** the download task starts, **When** the **pre-OTA** sequence runs, **Then** call **`ModeRegistry::prepareForOTA()`** (**new public API**, **rule 26** / **AR10**) which: tears down the active **`DisplayMode`**, frees mode heap, sets **`SwitchState::SWITCHING`** (or equivalent) so **`ModeRegistry::tick`** does not run normal mode logic, and leaves the system in a state where **flash OTA** can proceed safely. The **LED matrix** shows a **static** **“Updating…”** (or agreed copy) **progress** screen (**FR26**) — reuse or extend the **same** visual channel used for **upload OTA** if one exists; otherwise add a **minimal** full-matrix message path coordinated with the **display task** (**document** cross-core ordering: **`prepareForOTA`** may need to **signal** Core **0** if teardown must run there — **do not** violate **D2**/**Rule 30**).

3. **Given** **`prepareForOTA()`** completed, **When** the task checks **heap**, **Then** require **`ESP.getFreeHeap() >= 81920`** (**80 KB**, **FR34** / **NFR16**). If below threshold: **`Update.abort()`** not yet started is fine — set **`OTAState::ERROR`**, **`getLastError()`** **“Not enough memory — restart device and try again”**, exit task and clear handle (**epic**).

4. **Given** heap is sufficient, **When** **SHA** step runs, **Then** **HTTPS GET** the **`.sha256`** asset URL from **`OTAUpdater`** storage (~**64** bytes **+** newline), parse as **64** **hex** characters (ignore surrounding whitespace / optional filename suffix per **common** **GitHub** **sha256** **artifacts**). If missing or unparseable: **`OTAState::ERROR`**, **“Release missing integrity file”**, **no** **`Update.begin()`** (**AR9** / **epic**).

5. **Given** expected **SHA-256** is known, **When** **binary** download starts, **Then** **`esp_ota_get_next_update_partition(NULL)`**, **`Update.begin(partition->size)`** (same sizing approach as **`WebPortal`** upload — **`firmware/adapters/WebPortal.cpp`** ~608–629), initialize **`mbedtls_sha256_context`**, **`mbedtls_sha256_starts_ret(&ctx, 0)`**, and stream **`.bin`** via **`HTTPClient`** / **`WiFiClientSecure`** in **bounded** chunks (**epic**).

6. **Given** each **binary** chunk arrives, **When** processed, **Then** in the **same** loop iteration **`Update.write(chunk, len)`** **and** **`mbedtls_sha256_update(&ctx, chunk, len)`** (**rule 25** — incremental, **never** post-hoc full-buffer hash of the whole image in RAM). Update **`_progress`** **0–100** as **`(bytesWritten * 100) / totalSize`** ( **`totalSize`** from **`Content-Length`** if present, else **partition** size — document fallback if **chunked** encoding lacks length).

7. **Given** stream completes, **When** verifying, **Then** **`mbedtls_sha256_finish_ret`**, **constant-time** **compare** (or **`memcmp`** on fixed **32**-byte digests) to **expected** **SHA** **before** **`Update.end(true)`**. On **mismatch**: **`Update.abort()`**, **`OTAState::ERROR`**, **“Firmware integrity check failed”** (**AR9**). On **match**: **`Update.end(true)`**, **`esp_ota_set_boot_partition(next)`**, **`OTAState::REBOOTING`**, **`delay(500)`**, **`ESP.restart()`** (**epic**). **NFR5**: typical path **< 60s** on **10 Mbps** — **document** assumptions (no hard CI gate unless test harness exists).

8. **Given** the task exits (**success** or **failure**), **When** leaving the task, **Then** clear **`_downloadTaskHandle`** (or equivalent) **before** **`vTaskDelete(NULL)`** so **`startDownload()`** guard cannot observe a **dangling** handle (**epic**). **Every** error path after **`Update.begin()`** succeeds must call **`Update.abort()`** (**rule 25**, align **dl-7.2**).

9. **Given** **`WebPortal`** **POST `/api/ota/upload`** first-chunk path (**`WebPortal.cpp`** ~563+), **When** **`Update.begin()`** is about to run, **Then** invoke **`ModeRegistry::prepareForOTA()`** **before** **`Update.begin()`** so **push** and **pull** share the same teardown (**AR10**). If **`prepareForOTA()`** fails (e.g. display task cannot quiesce), **abort** upload with a **clear** **`errorCode`**.

10. **Given** **`pio run`** / **`pio test`**, **Then** add tests feasible **without** full **WiFi**: **e.g.** **SHA** parser unit test, **hex** compare test, **`prepareForOTA`** **no-op** or **mock** where **Unity** **environment** allows; document **hardware** integration for **end-to-end** pull. **No** new **compiler** warnings.

## Tasks / Subtasks

- [x] Task 1 — **`ModeRegistry`** — **`prepareForOTA()`** declaration/definition, **switch** state integration, **display** "Updating…" hook (**AC: #2, #9**)
- [x] Task 2 — **`OTAUpdater`** — **`startDownload()`**, task proc, **HTTP** stream, **`mbedtls_sha256_*`**, **progress**, **state** transitions, **guards** (**AC: #1, #3–#8**)
- [x] Task 3 — **`WebPortal.cpp`** — call **`prepareForOTA()`** before **`Update.begin()`** on **upload** path (**AC: #9**)
- [x] Task 4 — **`main.cpp` / display pipeline`** — ensure **LED** / **matrix** **Updating** state and **no** **use-after-free** during OTA (**AC: #2**)
- [x] Task 5 — **Includes / build** — **`mbedtls`**, **`Update`**, **`esp_ota_ops.h`** as needed (**AC: #5–#7**)
- [x] Task 6 — **Tests** (**AC: #10**)

## Dev Notes

### Prerequisite

- **`dl-6-1`** / **`dl-6-2`** provide **`OTAUpdater::checkForUpdate()`** and **`AVAILABLE`** state with **asset URLs**.

### Out of scope (**dl-7.2**, **dl-7.3**)

- **`POST /api/ota/pull`**, **`GET /api/ota/status`**, **dashboard** **Retry**/**progress** **polling** — **dl-7.3**; mid-stream **WiFi** drop **UX** copy — partly **dl-7.2**. This story may still implement **internal** **`Update.abort()`** on **HTTP** read failure so partition is not marked bootable.

### Partition / size

- **OTA** slots: **`firmware/custom_partitions.csv`** — **app0/app1** **0x180000** (**1.5 MiB**). **`firmware/check_size.py`** limits must remain consistent (**NFR14** in **dl-7.3** references **2 MiB** — **verify** against **actual** **partition** table when claiming limits).

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-7.md` — Story **dl-7.1**
- Prior: `_bmad-output/implementation-artifacts/stories/dl-6-2-update-notification-and-release-notes-in-dashboard.md`
- **Push OTA:** `firmware/adapters/WebPortal.cpp` — **`/api/ota/upload`**
- **Registry:** `firmware/core/ModeRegistry.cpp`, `firmware/core/ModeRegistry.h`
- **Boot / rollback:** `firmware/src/main.cpp` — **`esp_ota_*`** **self-check** region ~516+

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, 80.8% flash, no new warnings
- Tests: `pio test -f test_ota_updater` — compilation PASSED (on-device execution requires hardware)

### Completion Notes List

1. **Task 1 (ModeRegistry::prepareForOTA):** Added `prepareForOTA()` and `isOtaMode()` public API. Sets `SwitchState::SWITCHING`, tears down and deletes active mode, consumes pending switch requests, sets `_otaMode` flag. Display task checks `isOtaMode()` to show "Updating..." message.

2. **Task 2 (OTAUpdater download):** Implemented `startDownload()` which spawns a one-shot FreeRTOS task on Core 1 (10KB stack, priority 1). Task procedure: calls `prepareForOTA()`, checks heap >= 80KB, fetches `.sha256` file, parses 64-hex-char digest, opens OTA partition with `Update.begin(partition->size)`, initializes `mbedtls_sha256_context`, streams binary in 4KB chunks with simultaneous `Update.write()` + `mbedtls_sha256_update()`, verifies SHA-256 with constant-time compare before `Update.end(true)`, sets boot partition, reboots. All error paths call `Update.abort()` after `Update.begin()`. Handle cleared before `vTaskDelete(NULL)`.

3. **Task 3 (WebPortal upload path):** Added `ModeRegistry::prepareForOTA()` call before `Update.begin()` in `/api/ota/upload` handler. If teardown fails, returns `PREPARE_OTA_FAILED` error code.

4. **Task 4 (Display pipeline safety):** Added `isOtaMode()` check in `displayTask()` loop before mode rendering. When OTA is active, displays "Updating..." message at 5fps, resets WDT, and skips all mode rendering — preventing use-after-free.

5. **Task 5 (Build):** All required includes added: `mbedtls/sha256.h`, `Update.h`, `esp_ota_ops.h`, `ModeRegistry.h` in OTAUpdater.cpp. Build passes with no new warnings. Binary size 80.8% of partition limit.

6. **Task 6 (Tests):** Added 16 new unit tests to `test_ota_updater/test_main.cpp`: 9 for `parseSha256File()` (valid hex, filename suffix, leading whitespace, uppercase, too short, invalid char, null inputs, small buffer), 5 for `compareSha256()` (matching, different, last byte, null inputs), 2 for download guards (rejects when not AVAILABLE, initial progress is 0). All compile clean.

### File List

- `firmware/core/OTAUpdater.h` — modified (added startDownload, getProgress, parseSha256File, compareSha256, OTA_HEAP_THRESHOLD, _downloadTaskHandle, _progress, _downloadTaskProc)
- `firmware/core/OTAUpdater.cpp` — modified (added download task implementation, SHA-256 parsing, constant-time compare, includes for mbedtls/Update/esp_ota_ops/ModeRegistry)
- `firmware/core/ModeRegistry.h` — modified (added prepareForOTA, isOtaMode, _otaMode)
- `firmware/core/ModeRegistry.cpp` — modified (added prepareForOTA and isOtaMode implementations)
- `firmware/adapters/WebPortal.cpp` — modified (added prepareForOTA call before Update.begin in upload handler)
- `firmware/src/main.cpp` — modified (added isOtaMode check in displayTask for safe "Updating..." rendering)
- `firmware/test/test_ota_updater/test_main.cpp` — modified (added 16 new tests for SHA-256 parsing, comparison, download guards)

## Previous story intelligence

- **Foundation** **upload** used **`Update.write`** without **external** **SHA** file — **pull** path must implement **AR9** **incremental** hash (**epic**).

## Git intelligence summary

Touches **`OTAUpdater.*`**, **`ModeRegistry.*`**, **`WebPortal.cpp`**, **`main.cpp`** (or display module), tests.

## Project context reference

`_bmad-output/project-context.md` — **FreeRTOS** cores, **heap**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
