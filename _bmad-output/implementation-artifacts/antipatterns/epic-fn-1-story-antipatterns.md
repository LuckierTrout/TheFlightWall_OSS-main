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
