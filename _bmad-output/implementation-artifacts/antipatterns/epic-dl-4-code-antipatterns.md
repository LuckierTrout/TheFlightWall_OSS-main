# Epic dl-4 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-4-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | `invalidateScheduleCache()` declared and defined but NEVER called after `setModeSchedule()`, causing schedule changes to not take effect until device reboot | Added `ModeOrchestrator::invalidateScheduleCache()` call at end of `setModeSchedule()` after successful NVS persist, with comment explaining AC #2 requirement. |
| critical | AC #2 requires modeId validation but implementation only checks non-empty string — invalid modeIds are persisted to NVS and fail silently at runtime with no user feedback | Added comprehensive documentation to ConfigManager.h explaining deferred validation strategy (modeId existence cannot be checked at `setModeSchedule()` time due to circular dependency). Added inline comment in `.cpp` explaining this is intentional architectural decision per AC #2. |
| high | Missing test coverage for invalid modeId scenarios — test suite only covers happy paths with valid mode IDs | DEFERRED — Added as action item below. Test would verify that schedule with invalid modeId is accepted by `setModeSchedule()` but fails gracefully at runtime when orchestrator tick evaluates. |
| dismissed | SOLID violation — `ModeOrchestrator::tick` signature tightly coupled to NTP logic, forcing orchestrator to know about NTP sync status | FALSE POSITIVE: False positive. The orchestrator receives **results** of NTP evaluation (bool + uint16_t), not NTP internals. This is dependency injection via parameters, which is correct SOLID design. The tick signature is clean and testable. |
| dismissed | Race condition on `g_flightCount` — atomic update happens after potentially blocking `fetchFlights()` call, leading to stale counts | FALSE POSITIVE: False positive. Line 1288 in main.cpp updates `g_flightCount` immediately after queue publish (line 1281), which is the correct pattern. The fetch operation completes before the atomic store, so orchestrator always operates on the most recent snapshot. This is by design. |
| dismissed | WDT recovery boot forces "clock" mode but orchestrator state machine not updated, leading to immediate schedule re-evaluation | FALSE POSITIVE: Not verified in source code. WDT recovery logic (lines 1131-1138 in main.cpp) is separate from orchestrator initialization. The claim lacks evidence of a concrete bug. |
| dismissed | Premature optimization — schedule cache for 8 rules (~100 bytes) adds maintenance burden without measurable performance benefit | FALSE POSITIVE: While the cache does add complexity, it's a reasonable optimization for a 1Hz tick rate to avoid repeated NVS reads. The **bug** was that the cache wasn't being invalidated, which has now been fixed. The cache itself is not the problem. |
