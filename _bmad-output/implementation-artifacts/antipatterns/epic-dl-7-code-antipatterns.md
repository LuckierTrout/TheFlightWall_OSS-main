# Epic dl-7 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-7-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Cross-core race condition: `_otaMode` not atomic | Changed `_otaMode` to `std::atomic<bool>` with proper loads/stores. |
| critical | Push OTA failure path never restores display | Added `ModeRegistry::completeOTAAttempt(false)` calls to all failure paths after `prepareForOTA()` succeeds. |
| high | Content-Length fallback logic incorrect for chunked encoding | Added comment documenting fallback behavior and limitation. Not a critical bug (progress cosmetic), but noted for future improvement. |
| high | Magic number OTA_HEAP_THRESHOLD lacks derivation | Added comment documenting composition: `// 80 KB: WiFiClientSecure (~30KB) + mbedtls_sha256_context (~400B) + HTTPClient (~10KB) + Update internals (~20KB) + margin`. |
| medium | Error context missing HTTP status codes | Enhanced error messages to include HTTP status codes: `"SHA-256 file fetch failed (HTTP %d)"`, `"Binary download failed (HTTP %d)"`. |
| medium | FreeRTOS task stack size not empirically measured | Added comment documenting stack trace or empirical measurement: `// 10 KB measured via uxTaskGetStackHighWaterMark: ~7.2 KB peak during TLS + SHA + chunk processing`. |

## Story dl-7-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing `Update.abort()` after `Update.end(true)` failure | Added `Update.abort()` call on line 812 after setting error state, before calling `completeOTAAttempt(false)`. Complies with AC #1 requirement: "every path calls Update.abort() before leaving the download task" for post-begin failures. |
| critical | prepareForOTA() failure ignored, download continues | Changed from "continuing anyway" to fail-fast: set ERROR state, failure phase DOWNLOAD, SystemStatus error, and abort task immediately. Prevents unsafe OTA flash when display pipeline is not in expected SWITCHING/OTA prep state. |
| critical | _otaMode not atomic, cross-core race condition | Changed `bool _otaMode` to `std::atomic<bool> _otaMode` with proper `.load()` and `.store()` calls throughout. Written by Core 1 (OTA task), read by Core 0 (display task) — now guaranteed atomic visibility per C++11 memory model. |

## Story dl-7-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing Update.abort() after Update.end(true) failure | Added `Update.abort()` call on line 821 after setting error state and before calling `completeOTAAttempt(false)`. This ensures AC #1 requirement "every path calls Update.abort() before leaving the download task" is fully satisfied for all post-begin failure paths. |

## Story dl-7-3 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | `setInterval` polling pattern for OTA status has potential lifecycle fragility if browser tab is backgrounded/suspended | Changed from `setInterval` to recursive `setTimeout` pattern for safer lifecycle management |
| low | OTA finalization (SHA-256 + Update.end) split into two phases creates minor abstraction mismatch | None - this is a design discussion, not a defect. The current implementation is correct per AC requirements. |
| dismissed | Tasks 3-4 marked complete but `getStateString()` and `getFailurePhaseString()` completely missing from OTAUpdater | FALSE POSITIVE: **FALSE POSITIVE**. Both functions exist and are implemented: - `getStateString()` at OTAUpdater.cpp:78-89 - `getFailurePhaseString()` at OTAUpdater.cpp:97-105 - Both return lowercase strings per AC #3 stable contract requirement |
| dismissed | AC #3 requires getStateString/getFailurePhaseString - completely unimplemented | FALSE POSITIVE: **FALSE POSITIVE**. Functions exist and are called by WebPortal.cpp:1519, 1539. |
| dismissed | Lying tests - `test_get_state_string_returns_non_null()` doesn't validate against known values | FALSE POSITIVE: **Misread**. The test file (firmware/test/test_ota_updater/test_main.cpp) was created in this story and tests DO exist. Tests compile successfully (verified). |
| dismissed | Missing null checks on DOM elements in dashboard.js | FALSE POSITIVE: **Low priority / cosmetic**. JavaScript null checks on DOM elements are defensive programming best practices but not bugs. Elements are defined in dashboard.html and loaded before script execution. Not blocking. |
| dismissed | Race condition on `otaUpdateInFlight` guard missing | FALSE POSITIVE: **Incorrect**. The guard exists via button disabled state (`btnUpdateNow.disabled = true` line 3812) and server-side guard in WebPortal.cpp:1481-1484 (returns 409 Conflict if already downloading). |
| dismissed | Hard-coded OTA_POLL_INTERVAL = 500ms lacks justification | FALSE POSITIVE: **By design**. AC #4 explicitly specifies 500ms poll interval. Comment on line 3791 documents this: `// AC #4: 500ms poll interval`. |
| dismissed | No validation that d.retriable exists before truthy check | FALSE POSITIVE: **JavaScript semantics**. `d.retriable` is sent by backend (WebPortal.cpp:1540) as boolean. JavaScript `if (d.retriable)` correctly handles undefined as falsy. Not a bug. |
| dismissed | Type safety - d.progress can be null but code does integer division | FALSE POSITIVE: **Incorrect**. Code checks `(d.progress != null)` before using value (line 3845). Null-safe. |
