# Epic fn-2 - Story Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during validation of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent story-writing mistakes (unclear AC, missing Notes, unrealistic scope).

## Story fn-2-1 (2026-04-13)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Task 4.3 lacked a concrete location for the `isNtpSynced()` extern declaration, leaving the developer to guess between a new header (violates "no new files" rule), an existing header (wrong ownership), or inline in `WebPortal.cpp` (correct). | Task 4.3 now specifies `extern bool isNtpSynced();` goes at the top of `firmware/adapters/WebPortal.cpp`, with rationale tied to the no-new-files project rule. |
| medium | AC #1 timing criterion ("within 10 seconds") could mislead a dev agent into writing a flaky unit test, since Task 7 only defines unit tests and NTP timing depends on external network conditions. | Added inline clarification to AC #1 marking this as an integration test / device observation criterion, not a unit test requirement. |
| low | Task 5.1 description was slightly verbose ("with ~60 common IANA-to-POSIX mappings"). | Minor wording tightened to "~60 IANA-to-POSIX mappings" in parentheses. |
| dismissed | `TZ_MAP` static content creates a long-term maintenance burden if IANA DST rules change. | FALSE POSITIVE: The static map is the correct architectural choice for a resource-constrained ESP32 device. Dynamic IANA data fetching is impractical on this platform. The story scope is intentionally limited; updating the map for DST rule changes is a routine maintenance activity no different from any other firmware constant. The "What This Story Does NOT Include" section already scopes this correctly. No story change warranted. --- |
