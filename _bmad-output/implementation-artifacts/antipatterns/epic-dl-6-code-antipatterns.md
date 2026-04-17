# Epic dl-6 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-6-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Stale remote metadata after failed/up-to-date checks | Clear all remote fields at start of `checkForUpdate()` before any early return path. |
| high | AC #4 incomplete: missing `url` fallback in asset parsing | Updated `parseReleaseJson()` to check both `browser_download_url` and `url` fields in asset parsing logic. |
| medium | Unbounded heap buffering in version check path | Replaced `http.getString()` with stream-based parsing using `http.getStreamPtr()` and ArduinoJson's `deserializeJson()` direct stream read, avoiding full-buffer allocation. |
| medium | Weak test assertions in lifecycle tests | Strengthened test assertions to call `init()` explicitly, assert expected exact state, and remove conditional `TEST_PASS()` skips. |

## Story dl-6-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | CRITICAL: Task 7 completion claim is false - no actual test coverage for /api/ota/check | Defer to story rework - requires adding smoke test methods `test_get_ota_check_contract`, `test_get_status_includes_ota_fields`, and e2e mock routes. |
| critical | IMPORTANT: Blocking HTTPS request executed directly in async web handler | Defer to story rework - requires refactoring `checkForUpdate` to async task pattern or returning cached state with background refresh, similar to `/api/ota/pull` implementation. |
| high | IMPORTANT: Unsynchronized global OTA state shared across concurrent requests | Defer to story rework - requires adding `bool _checkInProgress` static guard similar to `g_otaInProgress` in WebPortal, or mutex protection around state transitions. |
| high | IMPORTANT: "Update Now" behavior drifted from story documentation | Defer - requires architectural decision: restore deep-link per original story OR update story record to reflect dl-7 integration. |
| high | IMPORTANT: Update metadata fetched with certificate validation disabled (setInsecure) | Defer to security hardening story - requires bundling GitHub API CA certificate or using Let's Encrypt root. Not a regression (dl-6.1 also uses setInsecure), but reviewer correctly flags it as higher risk when paired with immediate OTA action. |
| medium | MEDIUM: Inconsistent JSON error response pattern | Standardize to `_sendJsonError(request, 200, "WiFi not connected", "WIFI_DISCONNECTED")` for consistency. However, dev notes at line 91 of story document this as an intentional pattern choice ("chosen pattern: ok: true with error in data"). This is a design inconsistency, not a bug. |
| low | LOW: Heap fragmentation risk from std::vector<PendingRequestBody> usage | Defer to future refactoring - requires fixed-size pre-allocated circular buffer or stream-based JSON parsing. Not a regression for this story. |
| low | MINOR: E2E harness cannot model OTA-check routes | Defer to test infrastructure story - requires adding mock routes for `/api/ota/check` and `/api/ota/status`, plus Playwright test cases. |

## Story dl-6-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Story AC #5 implementation false - "Update Now" auto-starts pull OTA instead of deep-linking to upload zone | DEFERRED - Requires architectural decision: either restore dl-6.2 contract (remove pull UI, implement deep-link) OR update story record to acknowledge dl-7 integration |
| critical | Blocking HTTPS request executed directly in async web handler - 10s GitHub API call blocks all dashboard traffic | DEFERRED - Requires refactoring to background task pattern with cached state, similar to /api/ota/pull implementation |
| critical | Unsynchronized global OTA state shared across concurrent requests - race hazard between /api/ota/check and /api/ota/pull | DEFERRED - Requires adding mutex or `_checkInProgress` guard similar to `g_otaInProgress` pattern |
| critical | Security escalation - setInsecure() now on firmware installation path due to "Update Now" triggering pull OTA | DEFERRED - Requires bundling GitHub CA certificate or Let's Encrypt root for secure TLS validation (release-blocking if pull OTA stays) |
| medium | Release notes toggle label can show stale state after second check if hideOtaResults() called while expanded | DEFERRED - Low impact UI polish, safe to fix in cleanup story |
| medium | Test coverage only validates JSON shape, not behavioral contracts (deep-link, toggle state, explicit-click-only) | DEFERRED - Requires browser-level E2E tests with Playwright, beyond smoke test scope |
| low | No client timeout or abort path for update check - button can sit in "Checking…" indefinitely if TCP hangs | DEFERRED - Nice-to-have UX improvement, not a functional defect |
| dismissed | Asset URL fallback parsing flawed - `url` field requires binary-content negotiation | FALSE POSITIVE: False positive - this was fixed in dl-6-1 antipatterns (see line 5 of ANTIPATTERNS file). Current code at OTAUpdater.cpp checks both `browser_download_url` and `url` with proper headers. |
| dismissed | Unbounded heap buffering in version check at OTAUpdater.cpp:605 | FALSE POSITIVE: False positive - dl-6-1 antipatterns show this was replaced with stream-based parsing using `http.getStreamPtr()` and ArduinoJson's direct stream read. |
| dismissed | Single Responsibility Principle violation - OTAUpdater is a God Object | FALSE POSITIVE: While technically true, this is pre-existing architectural debt, not a dl-6.2 regression. Not in scope for this story. |

## Story dl-6-2 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #5 Contract Violation - "Update Now" behavior**: Story AC #5 explicitly states "Update Now" should deep-link to upload zone until dl-7, but implementation triggers `startOtaPull()` which initiates dl-7 pull-OTA workflow. This is a fundamental story contract violation. | DEFERRED — Requires architectural decision: either restore dl-6.2 contract (remove pull UI, implement deep-link scroll/focus to #ota-file-input) OR update story record to acknowledge dl-7 integration. This is a scope-creep/boundary issue requiring product owner input. |
| critical | Blocking HTTPS in Async Handler**: `_handleGetOtaCheck()` performs synchronous `OTAUpdater::checkForUpdate()` call which executes 10-second HTTPS request to GitHub API, blocking the async web server event loop and rendering dashboard unresponsive for all concurrent users. | DEFERRED — Requires refactoring to background task pattern (spawn FreeRTOS task) or cached-state refresh with async check, similar to `/api/ota/pull` implementation. This is an architectural change affecting Core 1 threading model. |
| critical | Unsynchronized Global OTA State**: `OTAUpdater::checkForUpdate()` clears global metadata buffers (`_remoteVersion`, `_releaseNotes`, `_binaryAssetUrl`, `_sha256AssetUrl`) at line 332-337 while concurrent requests from `/api/status` and `/api/ota/pull` read the same globals with no mutex or single-flight guard. Race condition: Browser A calling `/api/ota/check` can reset state to CHECKING and clear URLs while Browser B relies on AVAILABLE state with populated asset URLs. | DEFERRED — Requires adding `static bool _checkInProgress` guard similar to `g_otaInProgress` pattern in WebPortal, or mutex protection around state transitions, plus immutable "last check result" snapshot for consumers. |
| critical | Security Escalation - TLS Validation Disabled**: GitHub API and binary downloads use `client.setInsecure()` (line 349), disabling SSL/TLS certificate validation. While this was acceptable risk in dl-6.1 for metadata-only checks, dl-6.2 now places user-facing "Update Now" CTA in front of this path, turning spoofed metadata into a practical MitM attack vector for firmware installation. | DEFERRED — Requires bundling GitHub API CA certificate or Let's Encrypt root for secure TLS validation. This is a security hardening task beyond dl-6.2 scope, but reviewer correctly flags it as release-blocking if pull-OTA UI remains. |
| medium | Release Notes Toggle Label Desync**: `hideOtaResults()` collapses the notes region and resets `aria-expanded` to `false`, but never restores the button text to "View Release Notes". After one expanded check followed by a second check, the banner collapses but the control label can show stale "Hide Release Notes" text. | APPLIED — Added `btnToggleNotes.textContent = 'View Release Notes'` to `hideOtaResults()` function at line 3715, regenerated dashboard.js.gz. |
| medium | Test Coverage Gap - Behavioral Contracts**: Smoke tests validate only JSON response shape (`test_get_ota_check_contract`, `test_get_status_includes_ota_fields`) but never prove user-visible behavior that actually regressed: AC #5 deep-link, AC #9 explicit-click-only semantics, release notes toggle state correctness. The suite would still pass if "Update Now" auto-installs immediately or if the notes toggle label is wrong. | DEFERRED — Requires browser-level E2E tests with Playwright to validate UI behavior contracts. Beyond smoke test scope; documented in Review Follow-ups. |
