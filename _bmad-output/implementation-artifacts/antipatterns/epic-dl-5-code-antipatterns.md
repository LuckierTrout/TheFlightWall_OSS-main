# Epic dl-5 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-5-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing `clock/format` validation causes test failure | Added validation for `clock/format` range (0-1) matching existing pattern for `depbd/rows`. Test `test_settings_reject_unknown_mode_key` (line 1123) expects `setModeSetting("clock", "format", 2)` to return `false`; without validation, it returns `true` and writes invalid value to NVS, causing test failure on device. |
| high | Triple iteration of settingsObj in POST handler | Deferred — requires refactor to single-pass validation+apply loop. Functional but inefficient. Not blocking story completion. |
| high | SOLID Open/Closed Principle violation | Deferred — requires mode-specific validation callbacks or generic schema-based validation in setModeSetting. Current approach works but not extensible. |
| medium | Modes don't re-read settings on config change | Deferred — story Dev Notes say "settings changes take effect on next mode switch" (line 114 in DeparturesBoardMode). This is a design choice (Rule 18 caching), not a defect. AC #4's "~2s" clause is ambiguous. |
| medium | Multiple `fireCallbacks()` invocations for batch settings | Deferred — would require batch API `setModeSettingBatch()`. Current approach is inefficient but functional. Not a critical issue given typical 1-2 settings per mode. |
| medium | ArduinoJson `is<int32_t>()` rejects float-typed JSON values | Deferred — could use `.is<JsonVariant>()` and cast, or accept `.is<float>()` then round. Low risk given primary client (dashboard.js) serializes integers correctly. |
| dismissed | POST handler always calls `onManualSwitch` even for settings-only changes to active mode, contradicting AC #4's "skip onManualSwitch" clause | FALSE POSITIVE: AC #4 contains conflicting guidance. It says "skip onManualSwitch" for settings-only changes, but AC #6 and Rule 24 mandate "always route through orchestrator". Implementation comment at line 1215 cites Rule 24 as justification for calling `onManualSwitch` regardless. This is a **design decision** favoring orchestrator consistency over the AC #4 skip clause. Not a defect. |
| dismissed | `loadDisplayModes()` in Apply button handler rebuilds entire Mode Picker DOM, causing UX jank | FALSE POSITIVE: **Out of scope** — this is JavaScript client behavior (`dashboard.js:3010`), not firmware. Story tasks were completed. Frontend UX polish is a future enhancement. |
| dismissed | Partial apply risk — sequential `setModeSetting` calls write to NVS without transaction semantics | FALSE POSITIVE: **False positive**. The code pre-validates ALL keys and values in two loops (lines 1169-1201) before calling `setModeSetting`. The comment at line 1203 explicitly states "all values pre-validated — no partial apply risk". While technically each `setModeSetting` is an independent NVS write, validation ensures none fail, so in practice there is no partial apply. |
| dismissed | AC #4 claims firmware returns 400 without partial apply, but logic applies settings sequentially | FALSE POSITIVE: Duplicate/overlapping with previous. Pre-validation prevents this. |
