# Epic dl-3 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story dl-3-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | Missing API Implementation: `getLastErrorCode()` not implemented | ✅ Implemented method returning `_lastErrorCode.load()` at line 460 in ModeRegistry.cpp |
| high | Null Pointer Dereference Risk: `tickNvsPersist()` accesses `_table[0]` without guard | ✅ Added guard check at line 363 returning early if `_table == nullptr \|\| _count == 0` |
| medium | Uninitialized RenderContext in tests (minor) | ✅ Added explicit layout field initialization to `makeTestCtx()` at line 169 |

## Story dl-3-1 (2026-04-16)

| Severity | Issue | Fix |
|----------|-------|-----|
| medium | Documentation Gap: AC #1 mentions `_executeFadeTransition()` helper that doesn't exist | Added clarifying comment to ModeRegistry.h documenting the decision to integrate fade logic rather than extract to separate function (avoids function call overhead and simplifies buffer lifetime management). |
| medium | Missing Test Coverage: Incoming buffer allocation failure path not tested | Added test stub with TEST_IGNORE_MESSAGE documenting manual test requirement (forcing second malloc failure requires custom allocator mocking beyond standard test infrastructure). |
