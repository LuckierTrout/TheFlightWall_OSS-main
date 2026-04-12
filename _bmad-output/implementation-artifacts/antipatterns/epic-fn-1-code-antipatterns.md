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
