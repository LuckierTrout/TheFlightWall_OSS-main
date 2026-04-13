# Epic dl-1 - Story Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during validation of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent story-writing mistakes (unclear AC, missing Notes, unrealistic scope).

## Story dl-1-5 (2026-04-12)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | Task 1.3 and the Orchestrator State table had no catch-all for truly unknown/future enum values beyond SCHEDULED. The SCHEDULED row's reason column also said "(deferred to dl-4)" instead of a concrete string, leaving the API output undefined for that case. | Task 1.3 now explicitly maps any unrecognized state → `"unknown"`. The state table now includes a catch-all row and SCHEDULED has the concrete reason string `"scheduled"`. Dev Notes paragraph reinforces: never leave these fields empty. |
| high | AC7 used "Manually selected" as the subtitle example, but the JavaScript pattern in Dev Notes uses `data.state_reason` directly (which is `"manual"`). This created a genuine ambiguity — was the subtitle supposed to display a different, more human-friendly string, or the raw API value? | AC7 now reads `"the state_reason string from the API (e.g., 'manual' or 'idle fallback — no flights')"`, aligned with the JS pattern. |
| medium | Task 1.1 phrasing "if not already present" applied equally to both `getState()` (existing per architecture) and `getStateReason()` (new requirement), causing ambiguity about which method needs implementation work. | Task 1.1 now explicitly says "Confirm `getState()` exists per architecture.md#DL2; implement `getStateReason()` as a new static public method." |
| medium | Issue (bonus — not raised by validator)**: The Source Files table listed `getStateString()` as the method name, inconsistent with every other reference in the story which uses `getState()`. A developer following the table would create the wrong method name. | Source Files table corrected to `getState()` / `getStateReason()`. |
| low | Testing Approach covered only this story's specific scenarios, with no check that shared `dashboard.js` changes didn't introduce regressions in unrelated dashboard sections. | Added step 7 to Testing Approach: smoke-test settings panel, mode picker tap, and toast notifications after implementation. |
| dismissed | "Streamline 'Dev Notes' verbosity — the architecture constraints section is too verbose and could be condensed to save tokens." | FALSE POSITIVE: For embedded C++ + JavaScript cross-core development on ESP32, the verbose constraints (especially Rule 24 prohibiting direct `requestSwitch()` calls, and Rule 30 on cross-core atomics) carry precise, high-stakes implementation guidance. An implementing agent or developer encountering these constraints inline is far less likely to make a dangerous mistake than if they must chase a cross-reference to `architecture.md`. The marginal token cost is worth the safety. Not applied. --- |
