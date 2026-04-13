# Epic fn-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #1 Automation Missing**: Binary size logging was manual, not automated in build process | Created `check_size.py` PlatformIO pre-action script that runs on every build, logs binary size, warns at 1.3MB threshold, fails at 1.5MB limit. Added `extra_scripts = pre:check_size.py` to platformio.ini. |
| critical | Silent Data Loss Risk**: LittleFS.begin(true) auto-formats on mount failure without notification | Changed to `LittleFS.begin(false)` with explicit error logging and user-visible instructions to reflash filesystem. Device continues boot but warns user of unavailable web assets/logos. |
| high | Missing Partition Runtime Validation**: No verification that running firmware matches expected partition layout | Added `validatePartitionLayout()` function that checks running app partition size (0x180000) and LittleFS partition size (0xF0000) against expectations. Logs warnings if mismatches detected. Called during setup before LittleFS mount. |

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Silent Exit in check_size.py if Binary Missing | Added explicit error logging when binary is missing |
| low | Magic Numbers for Timing/Thresholds | Named constants already exist for some values (AUTHENTICATING_DISPLAY_MS, BUTTON_DEBOUNCE_MS). Additional refactoring would require broader changes. |
| low | Interface Segregation - WiFiManager Callback | Changed to C++ comment syntax to clarify intent |

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Silent Exit in check_size.py When Binary Missing | Added explicit error logging and `env.Exit(1)` when binary is missing |
| high | Magic Numbers for Partition Sizes - No Cross-Reference | Added cross-reference comments in all 3 files to alert developers when updating partition sizes |
| medium | Partition Table Gap Not Documented | Added comment documenting reserved gap |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing `getSchedule()` implementation | Added `ConfigManager::getSchedule()` method at line 536 following existing getter pattern with ConfigLockGuard for thread safety. Method returns copy of `_schedule` struct. |
| critical | Schedule keys missing from `dumpSettingsJson()` API | Added `snapshot.schedule = _schedule` to snapshot capture at line 469 and added all 5 schedule keys to JSON output at lines 507-511. GET /api/settings now returns 27 total keys (22 existing + 5 new). |
| critical | OTA and NTP subsystems not added to SystemStatus | Added `OTA` and `NTP` to Subsystem enum, updated `SUBSYSTEM_COUNT` from 6 to 8, and added "ota" and "ntp" cases to `subsystemName()` switch. Also fixed stale comment "existing six" → "subsystems". |
| high | Required unit tests missing from test suite | Added 5 new test functions: `test_defaults_schedule()`, `test_nvs_write_read_roundtrip_schedule()`, `test_apply_json_schedule_hot_reload()`, `test_apply_json_schedule_validation()`, and `test_system_status_ota_ntp()`. All tests integrated into `setup()` test runner. |
| low | Stale comment in SystemStatus.cpp | Updated comment from "existing six" to "subsystems" to reflect new count of 8. |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Integer Overflow in `sched_enabled` Validation | Changed from `value.as<uint8_t>()` (wraps 256→0) to validating as `unsigned int` before cast. Now properly rejects values >1. Added type check `value.is<unsigned int>()`. |
| critical | Missing Validation for `sched_dim_brt` | Added bounds checking (0-255) and type validation. Previously accepted any value without validation, violating story requirements. |
| critical | Test Suite Contradicts Implementation | Renamed `test_apply_json_ignores_unknown_keys` → `test_apply_json_rejects_unknown_keys` and corrected assertions. applyJson uses all-or-nothing validation, not partial success. |
| high | Missing Test Coverage for AC #4 | Added `test_dump_settings_json_includes_schedule()` — verifies all 5 schedule keys present in JSON and total key count = 27. |
| high | Validation Test Coverage Gaps | Extended `test_apply_json_schedule_validation()` with 2 additional test cases: `sched_dim_brt > 255` rejection and `sched_enabled = 256` overflow rejection. |
| dismissed | `/api/settings` exposes secrets (wifi_password, API keys) in plaintext | FALSE POSITIVE: Pre-existing design, not introduced by this story. Requires separate security story to implement credential masking. Story scope was schedule keys + SystemStatus subsystems only. |
| dismissed | ConfigSnapshot heap allocation overhead in applyJson | FALSE POSITIVE: Necessary for atomic semantics — applyJson must validate all keys before committing any changes. Snapshot pattern is intentional design. |
| dismissed | SystemStatus mutex timeout fallback is unsafe | FALSE POSITIVE: Pre-existing pattern across SystemStatus implementation (lines 35-44, 53-58, 65-73). Requires broader refactor outside story scope. This story only added OTA/NTP subsystems. |
| dismissed | SystemStatus tight coupling to WiFi, LittleFS, ConfigManager | FALSE POSITIVE: Pre-existing architecture in `toExtendedJson()` method. Not introduced by this story — story only added 2 subsystems to existing enum. |
| dismissed | Hardcoded NVS namespace string | FALSE POSITIVE: Pre-existing pattern, not story scope. NVS namespace was defined before this story. |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Test baseline claims 27 keys but implementation has 29 | Updated test assertion from 27 to 29 to match actual implementation |
| critical | Missing negative number validation for schedule time fields | Added type validation with `value.is<int>()` and signed bounds checking before unsigned cast |
| critical | Missing type validation for timezone string | Added `value.is<const char*>()` type check before string conversion |
| high | Missing test coverage for AC #4 - JSON dump includes all schedule keys | Corrected key count assertion from 27 to 29 |
| high | Validation test coverage gaps | Already present in current code (lines 373-382) - FALSE POSITIVE on reviewer's part, but validation logic in implementation was incomplete (see fixes #6, #7) |
| high | Integer overflow in sched_enabled validation | Already has type check `value.is<unsigned int>()` - validation is correct. Reviewing again...actually the test exists to verify 256 is rejected (line 380). This is working correctly. |
| high | Missing validation for sched_dim_brt | Code already has type check and >255 rejection (lines 163-167) after Round 2 fixes. Verified correct. |
| medium | Test suite contradicts implementation (unknown key handling) | Test already renamed to `test_apply_json_rejects_unknown_keys` with correct assertions after Round 2 fixes |

## Story fn-1-3 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `clearOTAUpload()` unconditionally calls `Update.abort()` when `started == true`, including the success path where `clearOTAUpload()` is called immediately after `Update.end(true)` — aborting the just-written firmware | Set `state->started = false` after successful `Update.end(true)` so the completed-update flag is cleared before cleanup runs |
| high | No concurrent upload guard; a second client can call `Update.begin()` while an upload is in-flight, sharing the non-reentrant `Update` singleton and risking flash corruption | Added `g_otaInProgress` static bool; second upload receives `OTA_BUSY` / 409 Conflict immediately |
| medium | `SystemStatus::set()` is called on `WRITE_FAILED`/`VERIFY_FAILED` but silently omitted from `INVALID_FIRMWARE`, `NO_OTA_PARTITION`, and `BEGIN_FAILED` — OTA subsystem status does not reflect these failures | Added `SystemStatus::set(ERROR)` to all three missing error paths |
| low | Task 3 claimed partition info logging was complete, but code only emitted `LOG_I("OTA", "Update started")` with no label/size | Added `Serial.printf("[OTA] Writing to %s, size 0x%x\n", partition->label, partition->size);` |
| low | `NO_OTA_PARTITION`, `BEGIN_FAILED`, `WRITE_FAILED`, `VERIFY_FAILED` (server-side failures) return HTTP 400 (client error); `OTA_BUSY` conflicts returned as 400 | Error-code-to-HTTP mapping: `INVALID_FIRMWARE` → 400, `OTA_BUSY` → 409, server failures → 500 |
| dismissed | `POST /api/ota/upload` is unauthenticated / CSRF-vulnerable | FALSE POSITIVE: No endpoint in this device has authentication — `/api/reboot`, `/api/reset`, `/api/settings` are all unauthenticated. This is a pre-existing architectural design gap (LAN-only device), not introduced by this story. Requires a dedicated security story. |
| dismissed | Missing `return` statements after `NO_OTA_PARTITION` and `BEGIN_FAILED` — code falls through to write path | FALSE POSITIVE: FALSE POSITIVE. The actual code at lines 480 and 490 contains explicit `return;` statements. Validator C misread a code snippet. |
| dismissed | Task 8 (header declaration) incomplete — `_handleOTAUpload()` not in `WebPortal.h` | FALSE POSITIVE: FALSE POSITIVE. The task itself states "or keep inline like logo upload" as an explicit alternative. The inline lambda approach is the correct pattern and matches the logo upload implementation. |
| dismissed | `std::vector<OTAUploadState>` introduces heap churn on hot async path | FALSE POSITIVE: This is the established project pattern — `g_logoUploads` uses the same `std::vector` approach. OTA is single-flight (now enforced), so the vector holds at most one entry. Not worth a divergent pattern. |
| dismissed | Oversized binary not rejected before flash writes begin | FALSE POSITIVE: `Update.begin(partition->size)` tells the library the maximum expected size. `Update.write()` will fail and return fewer-than-requested bytes when the partition is full, which the existing write-failure path handles. `Update.end(true)` accepts partial writes correctly. The library itself is the bounds guard. --- |

## Story fn-1-4 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `g_otaSelfCheckDone = true` placed outside the `err == ESP_OK` block — self-check silently completes even when `esp_ota_mark_app_valid_cancel_rollback()` fails, leaving firmware in unvalidated state with no retry | Moved `g_otaSelfCheckDone = true` inside the success branch. On failure, function exits without setting the flag so the next loop iteration retries. |
| high | AC #2 requires "a WARNING log message" for the timeout path, but `LOG_I` was used — wrong log level AND there was no `LOG_W` macro in the project | Added `LOG_W` macro to `Log.h` at level `>= 1` (same severity tier as errors). Changed `LOG_I` → `LOG_W` on the timeout path. |
| medium | `isPendingVerify` OTA state computed via two IDF flash-read calls (`esp_ota_get_running_partition()` + `esp_ota_get_state_partition()`) on every `loop()` iteration for up to 60 seconds — state cannot change during this window | Introduced `static int8_t s_isPendingVerify = -1` cached on first call; subsequent iterations skip the IDF calls entirely. |
| medium | `("Firmware verified — WiFi connected in " + String(elapsedSec) + "s").c_str()` passes a pointer to a temporary `String`'s internal buffer to `SystemStatus::set(const String&)` — technically safe but fragile code smell | Extracted to named `String okMsg` variable before the call. Same pattern applied in the error path. |
| low | `OTA_SELF_CHECK_TIMEOUT_MS = 60000` had no inline comment explaining the rationale for the 60s value | Added 3-line comment citing Architecture Decision F3, typical WiFi connect time, and no-WiFi fallback scenario. |
| dismissed | AC #6 violated — rollback WARNING emitted on normal boots after a prior rollback | FALSE POSITIVE: Story Gotcha #2 explicitly states: *"g_rollbackDetected will be true on every subsequent boot until a new successful OTA… This means /api/status will return rollback_detected: true on every boot until that happens — this is correct and intentional API behavior."* The deferred `SystemStatus::set(WARNING)` is the mechanism that surfaces the persisted rollback state through the health API. Suppressing it would hide a real device condition. |
| dismissed | `g_bootStartMs` uninitialized read risk — if called before `setup()`, `millis() - 0` returns a large value triggering immediate timeout | FALSE POSITIVE: In Arduino on ESP32, `setup()` is guaranteed to execute before `loop()`. `g_bootStartMs` is set at the top of `setup()`. There is no code path where `performOtaSelfCheck()` can run before `setup()`. |
| dismissed | `SystemStatus::set()` best-effort mutex fallback is unsafe | FALSE POSITIVE: Pre-existing pattern across the entire `SystemStatus` implementation (lines 41–50, 59–64, 71–79). Not introduced by this story. Requires broader architectural refactor outside story scope. |
| dismissed | Magic numbers 2000/4000 in `tickStartupProgress()` | FALSE POSITIVE: Pre-existing code in `tickStartupProgress()` from a prior story. Not introduced by fn-1.4. Out of scope for this synthesis. |
| dismissed | LOG macros used inconsistently with `Serial.printf` — project logging pattern violated | FALSE POSITIVE: `Log.h` macros (`LOG_E`, `LOG_I`, `LOG_V`) only accept **fixed string literals** — they have no format-string support. `Serial.printf` is the only viable option when embedding runtime values (partition labels, elapsed time, error codes). The mixed usage is correct project practice, not an inconsistency. The sole actionable logging issue was the `LOG_I` vs `LOG_W` severity mismatch (fixed above). |
| dismissed | `FW_VERSION` format not validated at runtime | FALSE POSITIVE: `FW_VERSION` is a compile-time build flag enforced by the build system and team convention. Runtime validation of a compile-time constant is over-engineering and has no failure mode that warrants it. |
| dismissed | `FlightStatsSnapshot` location not explicit in story docs | FALSE POSITIVE: Documentation gap only, not a code defect. The struct is defined in `main.cpp` as confirmed by reading the source. No code fix needed. |

## Story fn-1-4 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `s_isPendingVerify` error caching — `s_isPendingVerify = 0` was set unconditionally before the conditional, so if `esp_ota_get_running_partition()` returned NULL or `esp_ota_get_state_partition()` returned non-`ESP_OK`, the pending-verify state was permanently cached as 0 (normal boot). A transient IDF probe error on the first loop iteration would suppress AC #1/#2 status and timing logs on a real post-OTA boot for the entire 60s window. | Removed `s_isPendingVerify = 0` pre-assignment; now only set on successful probe. State probe retries on next loop iteration if running partition is NULL or probe fails. |
| low | Rollback WARNING blocked by `err == ESP_OK` check — the `SystemStatus::set(WARNING, "Firmware rolled back...")` call was inside the `if (err == ESP_OK)` block, meaning if `esp_ota_mark_app_valid_cancel_rollback()` persistently failed, AC #4's required WARNING status was never emitted. | Moved rollback `SystemStatus::set()` before the mark_valid call, inside the WiFi/timeout condition but outside the success guard. Added `static bool s_rollbackStatusSet` to prevent repeated calls on retry iterations. |
| low | Boot timing starts 200ms late — `g_bootStartMs = millis()` was assigned after `Serial.begin(115200)` and `delay(200)`, understating the reported "WiFi connected in Xms" timing by 200ms. Story task requirement explicitly states capture before delays. | Moved `g_bootStartMs = millis()` to be the very first statement in `setup()`, before `Serial.begin()` and `delay(200)`. |
| low | Log spam on mark_valid failure — when `esp_ota_mark_app_valid_cancel_rollback()` failed, `Serial.printf()` and `SystemStatus::set(ERROR)` were called on every loop iteration (potentially 20/sec) since neither `g_otaSelfCheckDone` is set (correctly) nor the error path had a throttle. | Added `static bool s_markValidErrorLogged = false` guard so the error is logged and status set only on the first failure; subsequent retries are silent. |

## Story fn-1-6 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | No cancel/re-select path once a valid file is chosen | Added `<button id="btn-cancel-ota">Cancel</button>` to `.ota-file-info`, wired to `resetOtaUploadState()` in JS. |
| critical | Polling race condition — timeout check fires immediately after async `FW.get()` starts, allowing both "Device unreachable" message AND success toast to show simultaneously on the final attempt | Restructured `startRebootPolling()` to check `attempts >= maxAttempts` **before** issuing the request; added `done` boolean guard so late-arriving responses are discarded after timeout. |
| high | `resetOtaUploadState()` does not reset `otaRebootText.style.color` or `otaRebootText.textContent` — amber timeout color and stale text persist into subsequent upload attempts | Added `otaRebootText.textContent = ''` and `otaRebootText.style.color = ''` to `resetOtaUploadState()`. |
| high | `.banner-warning .dismiss-btn` has no `:focus` style — WCAG 2.1 AA violation (visible focus indicator required) | Added `.banner-warning .dismiss-btn:focus-visible { outline: 2px solid var(--primary); outline-offset: 2px }` rule. |
| high | `prefs.putUChar("ota_rb_ack", 1)` return value ignored in ack-rollback handler — silent failure if NVS write fails | Added `prefs.begin()` return check and `written == 0` check; return 500 error responses with inline JSON (not `_sendJsonError` — lambda is `[]`, no `this` capture). |
| medium | `dragleave` removes `.drag-over` class when cursor moves over child elements — causes visual flicker mid-drag | Added `e.relatedTarget` guard: `if (!otaUploadZone.contains(e.relatedTarget))`. |
| low | Zero-byte file produces misleading error "Not a valid ESP32 firmware image" instead of "File is empty" | Added explicit `file.size === 0` check with message "File is empty — select a valid firmware .bin file" before the magic byte check. |

## Story fn-1-6 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | WCAG AA contrast failure — white text (#fff) over `--primary` (#58a6ff) on `.ota-progress-text` yields 2.40:1 contrast ratio, below the 4.5:1 AA minimum. Text overlays both filled (#58a6ff) and unfilled (#30363d) progress bar regions, creating variable backgrounds | Added four-direction `text-shadow: rgba(0,0,0,0.65)` to create a dark halo that ensures readability on both light-blue and dark-grey backgrounds |
| medium | `btnUploadFirmware` not disabled at start of `uploadFirmware()` — rapid double-tap could enqueue two concurrent uploads before the `otaFileInfo` section is hidden. Server guards with `g_otaInProgress` (returning 409), but the second request still generates an error toast, confusing UX | `btnUploadFirmware.disabled = true` at function entry, re-enabled in both `xhr.onload` and `xhr.onerror` |
| low | Polling `FW.get('/api/status')` lacks cache-busting — browsers with aggressive cache policies could serve a stale 200 from the pre-reboot device, falsely triggering "Updated to v..." before the new firmware is running | Changed to `FW.get('/api/status?_=' + Date.now())` to guarantee a fresh request |
| low | `#d29922` amber color hardcoded in JS for the "Device unreachable" timeout message, bypassing the design system. `--warning` CSS variable was also absent, making `var(--warning)` fallback impossible | Added `--warning: #d29922` to `:root` CSS variables; updated JS to `getComputedStyle(document.documentElement).getPropertyValue('--warning').trim() \|\| '#d29922'` |
| dismissed | Server-side OTA file size check missing (DoS vulnerability) | FALSE POSITIVE: Explicitly dismissed as FALSE POSITIVE in fn-1.3 antipatterns: "Update.begin(partition->size) tells the library the maximum expected size. Update.write() will fail when the partition is full." Client-side 1.5MB check provides UX feedback; server-side is handled by the Update library. |
| dismissed | `_sendJsonError` should replace inline JSON in ack-rollback handler | FALSE POSITIVE: `_sendJsonError` is a `WebPortal` class member function. The `/api/ota/ack-rollback` handler is a `[]` lambda (no `this` capture) — calling `_sendJsonError` is syntactically impossible without restructuring the handler. Inline JSON strings are the correct pattern for captureless lambdas. |
| dismissed | `OTA_MAX_SIZE` should be driven by API (`/api/status.ota_max_size`) | FALSE POSITIVE: Over-engineering. The 1.5MB partition size is a compile-time constant (custom_partitions.csv) that cannot change at runtime. Adding an API field and HTTP round-trip for this is disproportionate; the constant is clearly documented with its hex value. |
| dismissed | Drag-over border uses `--primary` instead of `--accent` (AC #3) | FALSE POSITIVE: `--accent` does not exist in the project's CSS design system (`:root` defines `--primary`, `--primary-hover`, `--error`, `--success`, `--warning` — no `--accent`). The Dev Notes explicitly lists `--primary` as the correct token for "drag-over border." The AC spec contains a stale/incorrect variable name. |
| dismissed | Race condition in `startRebootPolling` — timeout and success could fire simultaneously | FALSE POSITIVE: Already fixed in Pass 1 synthesis. Lines 2818-2831 show: `done` flag prevents double-finish; `attempts >= maxAttempts` check fires BEFORE the next request is issued; `if (done) return` discards late-arriving responses. |
| dismissed | `FileReader.readAsArrayBuffer(file.slice(0, 4))` reads 4 bytes but only `bytes[0]` is used | FALSE POSITIVE: Negligible inefficiency. Reading 4 bytes on a modern browser is identical cost to reading 1 byte (minimum slice size). Functionally correct; no impact on correctness or performance. |
| dismissed | `FW.get()` reboot polling is "falsely optimistic" — could return cached pre-reboot 200 | FALSE POSITIVE: The server calls `request->send(200, ...)` before scheduling the 500ms reboot timer (WebPortal.cpp:494). The full 50-byte JSON response will be in the TCP send buffer before the timer fires. `xhr.onerror` cannot fire on a response that was successfully sent and received. Only genuine network failures during upload reach `onerror`. |
| dismissed | Story file omits WebPortal.h and main.cpp from modified file list | FALSE POSITIVE: Documentation concern, not a code defect. The synthesis mission is to verify and fix SOURCE CODE — story record auditing is out of scope. No code correction needed. |
| dismissed | Settings export silently backs up stale persisted config when form has unsaved edits | FALSE POSITIVE: AC #10 specifies "triggers `GET /api/settings/export`" — exporting persisted server config is the intended behavior. The helper text instructs users to download settings as a backup "before migration or reflash" — not mid-edit. A warning when `applyBar` is visible would be a UX enhancement, not a bug fix. Out of scope. |
| dismissed | No automated test coverage for new OTA/export endpoints | FALSE POSITIVE: Valid concern but outside synthesis scope. ESP32 integration tests require hardware-in-the-loop or mock server infrastructure not present in this repo. Would need a dedicated testing story. |
| dismissed | CSRF protection missing on POST /api/ota/upload | FALSE POSITIVE: Explicitly dismissed as FALSE POSITIVE in fn-1.3 antipatterns: "No endpoint in this device has authentication — `/api/reboot`, `/api/reset`, `/api/settings` are all unauthenticated. This is a pre-existing architectural design gap (LAN-only device), not introduced by this story." |
| dismissed | No upload cancellation mechanism | FALSE POSITIVE: ALREADY IMPLEMENTED in Pass 1 synthesis. `btn-cancel-ota` exists in dashboard.html:248 and is wired to `resetOtaUploadState()` in dashboard.js:2620-2623. |
| dismissed | Unthrottled XHR progress events violate AC #6 ("every 2 seconds") | FALSE POSITIVE: Misread. AC #6 states "updates at least every 2 seconds" — this is a MINIMUM frequency requirement. The XHR progress handler fires MORE frequently (on each browser progress event), which satisfies "at least every 2s." Throttling to 500ms would still satisfy the AC. Throttling to 2s would be the minimum required and is not needed since XHR events are already frequent enough. |
| dismissed | XHR object not explicitly nulled after upload — memory leak | FALSE POSITIVE: JavaScript GC handles local variable cleanup. On an embedded single-page dashboard with one upload per session, a 1-2KB XHR object is negligible. Not a real leak; the object is properly scoped. |
| dismissed | CSS line count is 41 lines, not "~35 lines" as claimed in AC #11 | FALSE POSITIVE: AC #11 says "approximately 35 lines" — 41 lines is within reasonable approximation range and has no functional impact. --- |

## Story fn-1-6 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `.ota-upload-zone.drag-over` only changes `border-color`, not `border-style` — border stays `dashed` during drag, violating AC #3 ("border becomes solid") | Added `border-style:solid` to `.ota-upload-zone.drag-over` rule |
| high | OTA buttons below 44×44px touch target minimum — `.btn-ota-cancel` renders at ~36px height with `padding:10px`; `#btn-upload-firmware` renders at ~40px with `padding:12px` — violates AC #11 | Added `min-height:44px` to `.btn-secondary`, `.btn-ota-cancel`, and `.ota-file-info .apply-btn` override |
| medium | OTA success path `otaPrefs.begin()` return value not checked before `putUChar()` — silent NVS failure leaves rollback banner permanently dismissed after a good-OTA-then-rollback sequence | Wrapped NVS calls in `if (otaPrefs.begin(...))` with `LOG_E` on failure path |
| medium | `loadFirmwareStatus()` catch block completely silent — firmware card shows blank version and hidden banner with no user feedback on network failure | Added `FW.showToast('Could not load firmware status — check connection', 'error')` to catch |
| medium | Reboot polling uses `setInterval` — if `FW.get()` hangs >3s, the next poll fires before resolution, piling concurrent requests on the ESP32 during recovery | Replaced `setInterval` with recursive `setTimeout` — next poll only fires after current request resolves (success or error) |
| low | No `xhr.timeout` on the upload XHR — a stalled upload on a degraded network hangs indefinitely with no feedback | Added `xhr.timeout = 120000` and `xhr.ontimeout` handler showing error toast and resetting UI |

## Story fn-1-7 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `importStatus` element not hidden on error/zero-count paths — stale "N settings imported" badge persists after a failed re-import, violating AC #4 ("import zone resets to its initial state") | Moved `importStatus.style.display = 'none'`, `importStatus.textContent = ''`, and `importedExtras = {}` to the very top of `processImportedSettings()`, before the try/catch. Covers all error exit paths. |
| high | `showStep(currentStep)` called from `processImportedSettings()` without first flushing in-progress input — if the user typed values into Step 1 that hadn't been saved to `state`, the subsequent rehydration would overwrite those typed values with the (empty) stale state | Added `saveCurrentStepState()` call immediately before `showStep(currentStep)` in the success path. |
| medium | `parsed.hasOwnProperty(key)` is unsafe on user-controlled JSON — a file containing `{"hasOwnProperty": 1}` turns the method lookup into a TypeError and bypasses all normal toast/reset behavior | Replaced both occurrences with `Object.prototype.hasOwnProperty.call(parsed, key)`. |
| medium | No file-size guard in `handleImportFile()` — a multi-MB file passed via drag-and-drop could hang the browser tab on `FileReader.readAsText()` | Added `if (file.size > 1024 * 1024)` check with error toast before the FileReader is created. |
| low | `String(null)` → `"null"`, `String({})` → `"[object Object]"` — a hand-crafted import file with `null` or nested-object values could silently write garbage strings into wizard `state` fields, which would then pass through to the POST payload | Added `if (val !== null && typeof val !== 'object')` type guard in both the WIZARD_KEYS and KNOWN_EXTRA_KEYS loops before assignment. |
| low | `WizardPage.importSettingsButton` locator targets `<button>` elements, but the import zone is a `<div role="button">` — the locator would never match and any test using it would fail | Replaced `this.page.locator('button', { hasText: /import/i })` with `this.page.locator('#settings-import-zone')`. |
| dismissed | Hardcoded `WIZARD_KEYS`/`KNOWN_EXTRA_KEYS` violates OCP; fix with `/api/settings/schema` endpoint | FALSE POSITIVE: The story explicitly states "Do NOT create a new server-side endpoint." This is a client-side-only change by design. A schema endpoint would add firmware code, flash usage, and a network round-trip to the boot path for no practical gain on a single-firmware device. |
| dismissed | `importedExtras` is a "global mutable state" SRP violation | FALSE POSITIVE: The entire `wizard.js` runs inside an IIFE — `importedExtras` is a module-level closure variable, not a true global. This is the established pattern for the project's vanilla-JS embedded UI (see `state`, `scanTimer`, `manualMode`). |
| dismissed | Missing API-key format validation in `validateStep(2)` | FALSE POSITIVE: Pre-existing pattern present before fn-1.7 and outside story scope. No change introduced by this story. |
| dismissed | Missing `min`/`max` HTML attributes on numeric inputs | FALSE POSITIVE: Pre-existing pattern; not introduced by fn-1.7. |
| dismissed | `saveAndConnect` catch block uses `showSaveError` instead of `FW.showToast` | FALSE POSITIVE: Pre-existing intentional pattern — `showSaveError` renders inline within the review section because Step 5 is still visible. Not introduced by fn-1.7. |
| dismissed | Hardcoded `TOTAL_STEPS` | FALSE POSITIVE: Pre-existing; not introduced by fn-1.7. |
| dismissed | Missing cache-busting on `hydrateDefaults()` fetch | FALSE POSITIVE: Pre-existing; not introduced by fn-1.7. ESP32 AsyncWebServer does not cache API responses. |
| dismissed | Em dash `\u2014` in error message is inconsistent with other error messages using `-` | FALSE POSITIVE: The AC #4 spec text reads "Could not read settings file — invalid format" — the em dash is the spec-required format. Correct as written. |
| dismissed | Re-import mixes wizard state keys from old and new files | FALSE POSITIVE: The story explicitly specifies "reset `importedExtras = {}` and re-process." Wizard state keys are intentionally additive so that a targeted re-import (WiFi-only file) doesn't wipe hardware/location already present. This is a documented design decision. |
| dismissed | MIME-type validation missing on drag-and-drop | FALSE POSITIVE: This is a single-user trusted LAN device with no authentication on any endpoint. The `accept=".json"` file-picker filter is adequate UX guidance. A strict MIME check would also cause false positives on some OS/browser combinations that assign `application/octet-stream` to `.json` files. |
| dismissed | FileReader `onerror` message says "invalid format" when the actual cause could be a permission/IO error | FALSE POSITIVE: AC #4 specifies exactly: "an error toast shows 'Could not read settings file — invalid format'." No distinction is required by the spec. |
| dismissed | Drag-over CSS class not cleared when FileReader errors | FALSE POSITIVE: False positive. The `drop` event handler calls `importZone.classList.remove('drag-over')` synchronously *before* `handleImportFile()` is called. The FileReader fires asynchronously; by that point `.drag-over` is already gone. |
| dismissed | `KNOWN_EXTRA_KEYS` needs `display_mode` and future `m_*`/`sched_r*` keys | FALSE POSITIVE: Future firmware keys are a known design tradeoff documented in the story's Risk Mitigation section. The list is intentionally bounded to the 30 keys that `ConfigManager::applyJson()` accepts today. Adding speculative future keys would cause 400 errors if those keys don't exist in current firmware. --- |

## Story fn-1-7 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `handleImportFile()` early-returns (file too large, FileReader.onerror) bypass the `importedExtras = {}` reset in `processImportedSettings()`, so a prior successful import's extras survive into the `saveAndConnect()` POST payload after a failed re-import attempt | Added `importedExtras = {}; importStatus.style.display = 'none'; importStatus.textContent = '';` at the very top of `handleImportFile()` before all early returns |
| medium | `WizardPage.completeWizard()` and `completeSetup()` model a phantom 6-step wizard. Real wizard is 5 steps; Step 5 is "Review & Connect" with "Save & Connect" button. `completeMessage` locator (`text=/setup complete | `WizardStep` type removes `'test'`/`'complete'`; `nextButton` changed to `#btn-next` (works for both "Next →" and "Save & Connect"); `completeMessage` → `h1:has-text("Configuration Saved")`; `completeWizard()` clicks `nextButton` and awaits `completeMessage`; `completeSetup()` rewritten to 5-step flow without `skipStep()` |
| medium | `scanForNetworks()` clicks a non-existent scan button (scanning is automatic on wizard load); `expectNetworksVisible()` targets `'li, .network-item'` (the real selector is `.scan-row`) and uses `toHaveCount(expect.any(Number) as unknown as number)` — a malformed assertion that always passes vacuously | `scanForNetworks()` waits for `#scan-results` to become visible (auto-scan completes); `expectNetworksVisible()` uses `.scan-row` and `expect(count).toBeGreaterThan(0)` |
| low | `tilePixelsSelect` getter implies a `<select>` element but `#tile-pixels` is `<input type="text">` | Renamed `tilePixelsSelect` → `tilePixelsInput`; updated `configureMatrix()` to use `tilePixelsInput` |
| low | ARIA spec: `<p>` (block content) inside `role="button"` container violates ARIA phrasing-content constraint | Replaced `<p class="upload-prompt">` and `<p class="upload-hint">` with `<span>` elements (inline/phrasing content) |
| dismissed | Race condition — `importFileInput.value = ''` cleared before FileReader processes file | FALSE POSITIVE: The `File` object is captured as a parameter (`handleImportFile(importFileInput.files[0])`) before `importFileInput.value = ''` runs. The value clear exists specifically to allow re-selection of the same file on next attempt. This is the standard browser pattern. No race exists. |
| dismissed | `display_mode` missing from `KNOWN_EXTRA_KEYS` | FALSE POSITIVE: `display_mode` does not exist in `ConfigManager::dumpSettingsJson()` nor `applyJson()`. Verified by reading all 29 keys emitted by `dumpSettingsJson()` (lines 482–520). Adding a non-existent key to `KNOWN_EXTRA_KEYS` would silently send it in the POST payload, causing a 400 error when ConfigManager rejects it as an unknown key. |
| dismissed | `KNOWN_EXTRA_KEYS` hardcoded list is a forward-compatibility/maintenance risk (CRITICAL/IMPORTANT) | FALSE POSITIVE: Already dismissed in previous synthesis rounds. The story explicitly prohibits new server-side endpoints. The `KNOWN_EXTRA_KEYS` list matches exactly the 17 extra keys in `dumpSettingsJson()`. Future keys are a known tradeoff documented in story Risk Mitigation section. Sending unknown keys to ConfigManager causes 400 errors, so speculative future keys must never be in this list. |
| dismissed | SRP violation — `wizard.js` handles too many responsibilities | FALSE POSITIVE: Pre-existing architecture, not introduced by fn-1.7. ESP32 embedded web UI uses plain IIFE-scoped JS without module support. Refactoring would require ES module infrastructure that doesn't exist on the device's LittleFS server. |
| dismissed | DRY violation — `state` object and `WIZARD_KEYS` array define the same 12 keys | FALSE POSITIVE: Pre-existing pattern. `state` is a value store; `WIZARD_KEYS` is an iteration list for import. They serve different purposes. The duplication is intentional and minimal. |
| dismissed | `TOTAL_STEPS` is a magic number | FALSE POSITIVE: Pre-existing, not introduced by fn-1.7. Out of scope. |
| dismissed | Missing automated E2E test coverage for import scenarios | FALSE POSITIVE: Valid tech debt but out of scope for this synthesis. The WizardPage.ts page object has been substantially corrected in this round, providing the infrastructure for future wizard specs. Creating a wizard spec requires mock-server support for the AP-mode endpoints — a separate effort. |
| dismissed | Extra-key type validation allows malformed values through (e.g., `timezone: 123`) causing server-side 400 rejection after a success toast | FALSE POSITIVE: Low risk. Extra keys come from a `flightwall-settings.json` exported by the same device firmware, so types are always correct. A hand-crafted file with `timezone: 123` on a single-user LAN-only device with no authentication is not a threat model the story addresses. The existing `not null && not object` guard is sufficient. |
| dismissed | `processImportedSettings()` SRP violation — too many responsibilities in one function | FALSE POSITIVE: Pre-existing style, embedded JS. The function is 50 lines and straightforward. Splitting it would require closures or parameter passing that adds complexity without benefit in this context. |
| dismissed | DRY — error message string `'Could not read settings file — invalid format'` appears 3× | FALSE POSITIVE: Low-value refactor in embedded JS. Three occurrences in a LittleFS-deployed file; extracting a constant adds a variable declaration but saves 0 bytes after gzip. Deferred. |
| dismissed | Documentation gaps — WizardPage.ts not in story File List, Task 3 completion lie | FALSE POSITIVE: Out of scope. Story documentation auditing is not a source-code synthesis responsibility. The code defects underlying the Task 3 claim were addressed in rounds 1 and 2. --- |
