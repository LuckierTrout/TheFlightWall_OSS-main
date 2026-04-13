# Epic fn-1 - Story Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during validation of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent story-writing mistakes (unclear AC, missing Notes, unrealistic scope).

## Story fn-1-1 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Incorrect Partition Size Specifications in AC2**: Acceptance Criterion 2 listed offset values (0x10000, 0x190000, 0x310000) as if they were sizes, conflicting with the correct size values in the Dev Notes table (0x180000, 0x180000, 0xF0000). This would have caused immediate build failure or incorrect partition table creation. | Updated AC2 to use correct format: "nvs 0x5000/20KB (offset 0x9000), otadata 0x2000/8KB (offset 0xE000), app0 0x180000/1.5MB (offset 0x10000), app1 0x180000/1.5MB (offset 0x190000), spiffs 0xF0000/960KB (offset 0x310000)" |
| high | Ambiguous Documentation Location in AC1**: The phrase "a warning is documented" in AC1 did not specify where or how the warning should be recorded, creating ambiguity for an LLM agent. | Updated AC1 to specify: "a warning is logged to stdout and noted in the 'Completion Notes List' section of this story with optimization recommendations" |
| high | Overly Broad Functionality Verification in AC3**: The criterion "all existing functionality works: flight data pipeline, web dashboard, logo management, calibration tools" lacked specific verification steps. | Updated AC3 to enumerate specific checks: "the following existing functionality is verified to work correctly: device boots successfully; web dashboard accessible at device IP; flight data fetches and displays on LED matrix; logo management functions (upload, list, delete); calibration tools function" |
| high | Missing Runtime Verification in AC4**: AC4 only verified compile-time availability of FW_VERSION but not whether the runtime value matched the build flag. | Updated AC4 to include: "And the version string accurately reflects the value defined in the build flag when accessed at runtime" |

## Story fn-1-2 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | Ambiguous instruction in Task 5 regarding ConfigSnapshot usage for getSchedule() method | Split Task 5 into two clear sub-tasks: (1) implement getter following direct return pattern with ConfigLockGuard, (2) add ScheduleConfig field to ConfigSnapshot struct for loadFromNvs() operations |

## Story fn-1-3 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing Firmware Integrity Check (SHA256/MD5) | Added AC7 for firmware integrity verification with SHA256 hash, added Task 11 for implementation, added error code INTEGRITY_FAILED, updated Dev Notes with integrity verification section |
| critical | Lack of Version Check/Downgrade Protection | Added AC8 for firmware version validation, added Task 12 for version comparison implementation, added error codes VERSION_CHECK_FAILED and DOWNGRADE_BLOCKED, added Dev Notes section on version validation |
| critical | Missing RAM/Heap Impact Analysis for OTA State | Added subtask to Task 10 to measure and log heap usage during OTA upload, strengthened existing Dev Notes reference to heap constraints |
| high | Explicit Documentation for esp_ota_get_next_update_partition(NULL) | Enhanced AC1 to explicitly reference esp_ota_get_next_update_partition(NULL) |
| high | Specify UI Feedback for Reboot | Added note to AC5 referencing Story fn-1.6 requirement for UI reboot message, added cross-reference in Dependencies section |
| high | Guidance for OTAUploadState Helper Functions | Added comprehensive helper function implementation examples in Dev Notes with findOTAUpload() and clearOTAUpload() code snippets |
| high | Cross-reference UI handling of error messages | Added Dev Notes section on UI Error Handling Requirements with explicit fn-1.6 cross-reference |

## Story fn-1-4 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Rollback flag persistence API behavior was left as an open "consider whether" question rather than a definitive spec | Updated Critical Gotcha #2 to establish that `rollback_detected` remains `true` on every boot until a new successful OTA clears the invalid partition slot. This is declared intentional API behavior, and fn-1.6 is noted as the consumer that should display the persistent state. |
| medium | Log message time unit in AC1 shows `"8s"` (seconds) but Task 2 and the implementation pattern use `%lums` (milliseconds) — two different outputs for the same event | Updated AC1 log message example from `"Marked valid, WiFi connected in 8s"` to `"Firmware marked valid — WiFi connected in 8432ms"` to match the implementation pattern. Added explicit parenthetical `(X = elapsed seconds)` to the SystemStatus message format to clarify the unit distinction between the developer log and the user-facing status. |
| low | AC6 "no WARNING or ERROR status is set" was ambiguous about whether a StatusLevel::OK call is also omitted for normal boots | Updated AC6 to explicitly state "no `SystemStatus::set` call is made for the OTA subsystem" — eliminating the gap between "no WARNING or ERROR" and the implementation which makes no OTA status call at all on normal boots. |
| low | FW_VERSION note in the SystemStatus extension section was self-contradictory — stated the macro is "already available project-wide" then immediately suggested a local conditional define | Rewrote the note to explain the fallback guard is for non-PlatformIO build contexts (e.g., unit test harnesses), resolving the apparent contradiction. |
| dismissed | `"firmware_version": "1.0.0"` in AC5 hardcodes a specific version, which is misleading | FALSE POSITIVE: The value is immediately qualified with `(from FW_VERSION build flag)` — this is a concrete example in an AC, not a hardcoded literal. This is standard practice for illustrative acceptance criteria. No change warranted. |
| dismissed | `String(...).c_str()` usage in `performOtaSelfCheck()` should be replaced with direct String construction | FALSE POSITIVE: The pattern is valid, idiomatic Arduino/ESP32 embedded C++. `SystemStatus::set` accepting `const char*` or `const String&` is implementation-dependent, and `.c_str()` is universally compatible. The validator's alternative produces the same compiled output. The story's implementation pattern should not be changed for a style preference that has no correctness implication. --- |

## Story fn-1-6 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | Task 5 listed `localStorage` as an "Alternative simpler approach" for rollback acknowledgment persistence, directly contradicting AC #9's explicit "NVS-backed" requirement. A developer following the alternative path would violate the acceptance criteria. | Removed the localStorage alternative entirely from Task 5; replaced with a clean implementation note that frames direct `Preferences` access as the chosen pattern (not ConfigManager, not localStorage), and clarifies why. |
| medium | `ota_rb_ack` reset timing was inconsistent across the story — Task 5 said "when upload **succeeds**" but the Dev Notes code snippet said "at the `index == 0` **first-chunk block**" (upload start). If cleared at start: a failed upload would have already erased the ack, so a rollback banner that was previously dismissed would never reappear if the next attempt also fails. The correct behavior is clear-on-success only. | Harmonized Task 5, Dev Notes "Reset ota_rb_ack" section, Project Structure Notes, and Risk Mitigation #4 — all now consistently say "after `Update.end(true)` succeeds, before `ESP.restart()`". Added explicit warning: "Do NOT clear at upload start." |
| dismissed | Direct `Preferences` access for `ota_rb_ack` is a Critical architectural violation that must be routed through `ConfigManager`. | FALSE POSITIVE: The project context explicitly says "Prefer existing patterns in firmware/ over new abstractions." The AC says "NVS-backed" — it doesn't say "ConfigManager-backed." Adding a new ConfigManager struct for a simple transient flag would require modifying `ConfigManager.h`, `.cpp`, and structs across the board, which IS overkill. The story correctly justifies this deviation in Dev Notes. Direct `Preferences` access is standard ESP32 Arduino practice. |
| dismissed | Rollback ack reset logic is a Critical gap — ACs don't state that a new successful OTA resets acknowledgment. | FALSE POSITIVE: Task 5 and Dev Notes both document this behavior. The real problem was the *timing inconsistency* between Task 5 ("succeeds") and the Dev Notes code comment ("upload starts"), which IS addressed above — but it was a Medium inconsistency, not a Critical gap. |
| dismissed | "1.5MB" should be "1.5 MiB (1,572,864 bytes)" for technical precision. | FALSE POSITIVE: The entire fn-1 epic consistently uses "1.5MB" (from fn-1.1 partition work). Changing just this story's error message text would create inconsistency. The validation code already uses the exact byte count (`1572864`). User-facing error messages correctly mirror how OS file managers display sizes. |
| dismissed | AC #7 polling termination criteria are incomplete — must specify version-change detection. | FALSE POSITIVE: Since the server validates the OTA before responding `{ ok: true }` and rebooting, any successful `/api/status` poll after the reboot sequence proves the update installed. Requiring a version-change check would actually be incorrect in edge cases (e.g., reflashing the same version). Task 7 already specifies both stop conditions. |
| dismissed | CSS verification methods and FW_VERSION macro source are not explicitly stated. | FALSE POSITIVE: Testing Strategy already includes `ls -la` gzip verification. FW_VERSION source is documented in "Previous Story Intelligence" (from fn-1.1 and fn-1.4). These are already covered. --- |

## Story fn-1-7 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| dismissed | Improved User Control over Import UI Visibility — add an explicit "Clear/Dismiss" button to reset the import zone after import | FALSE POSITIVE: Already addressed by existing story design. Dev Notes line 183 explicitly states: *"the import zone can optionally collapse or show a success state, but it should NOT be removed — the user might want to re-import a different file."* Task 3's "Guard against re-import" subtask (line 102) already specifies that selecting a new file resets `importedExtras = {}` and re-processes — this is the natural "clear" mechanism. AC 7 further confirms the no-import path leaves the wizard unaffected. A dedicated dismiss button would add implementation surface with no functional gap it fills; the re-import affordance is the correct interaction model for this use case. |
| dismissed | Dynamically Generate `KNOWN_EXTRA_KEYS` from Server Schema — derive the extra keys list at runtime from `GET /api/settings/export` or a new `/api/schema` endpoint | FALSE POSITIVE: Directly contradicts two explicit project constraints. (1) The story's Dev Notes and "What NOT to Do" section both prohibit creating new server-side endpoints. (2) The story's architecture is intentionally CLIENT-SIDE ONLY with zero firmware modifications. `GET /api/settings/export` downloads a full populated settings file (not a schema), making runtime key derivation unreliable and requiring an authenticated device to be available at wizard startup. Risk Mitigation 1 (lines 359–361) already documents the `KNOWN_EXTRA_KEYS` approach as intentional: the whitelist filter is actually *more* robust than dynamic fetching because it is deterministic, offline-capable, and ensures future firmware keys from an unknown export file cannot accidentally be sent to an older firmware's `POST /api/settings` (which would cause a 400 error from `applyJson()`). |
| dismissed | Story is too prescriptive — reduces "Negotiable" aspect of INVEST | FALSE POSITIVE: Working as intended. This project uses BMAD LLM-agent workflows (`bmad-dev-story`) where the executing agent is a code-generation LLM, not a human developer making architectural decisions. Prescriptive stories that specify variable names, function signatures, and merge logic are a feature of this workflow, not a defect. The BMAD create-story agent deliberately includes this level of detail to prevent LLM hallucination and ensure architectural alignment. --- |
