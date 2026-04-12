# FlightWall QA Plan

## Objective

Establish a repeatable QA process for the FlightWall firmware, web portal, and hardware-assisted user flows.

This plan is designed to answer two practical questions:

1. How do we know core user journeys still work?
2. How do we catch regressions before a release goes onto a real device?

## Scope

### In Scope

- Firmware configuration behavior and persistence
- Web portal API contracts exposed by `WebPortal.cpp`
- Setup wizard onboarding flow
- Dashboard configuration flow
- System health page rendering and refresh behavior
- Logo upload, listing, and deletion flows
- Recovery flows such as reboot and factory reset
- Hardware-assisted validation for Wi-Fi mode, LED behavior, and calibration

### Out of Scope for the First QA Pass

- Full browser automation for every dashboard control
- Performance benchmarking
- Long-duration soak testing
- Fault-injection frameworks
- Full external API simulation for OpenSky, AeroAPI, and CDN failures

## Quality Risks

### Highest Risk

- Device boots into the wrong mode or serves the wrong root page
- Settings save successfully in the UI but do not persist after reboot
- Reboot-required settings are applied incorrectly
- Factory reset does not return the device to onboarding
- Health page renders malformed or unsafe backend strings
- Logo storage operations fail or leave inconsistent state

### Medium Risk

- Dashboard and health page drift from backend JSON contracts
- Wi-Fi scan behavior is inconsistent between AP and STA modes
- Layout preview and layout API fall out of sync
- Calibration controls behave correctly in the UI but not on the LED wall

### Lower Risk

- Missing docstrings or style-level anti-pattern findings
- Non-critical UI copy or cosmetic rendering issues

## Test Strategy

QA should run in layers rather than relying on one giant test suite.

| Layer | Goal | Tooling | Frequency |
| --- | --- | --- | --- |
| Firmware unit tests | Protect pure logic and persistence behavior | PlatformIO + Unity | Every implementation cycle |
| API smoke tests | Verify web portal contract against a live device | `tests/smoke/test_web_portal_smoke.py` | Before merge and before release |
| Manual release smoke | Validate the primary user journeys on hardware | `release-smoke-checklist.md` | Every release candidate |
| Hardware-in-the-loop validation | Confirm LED, reboot, Wi-Fi, and calibration behavior | Bench device + human observer | Every release candidate |

## Existing Coverage Baseline

The repository already contains useful firmware-level tests in `firmware/test/`:

- `test_config_manager`
- `test_layout_engine`
- `test_logo_manager`
- `test_telemetry_conversion`

These should remain the foundation for logic coverage, but they do not replace end-to-end QA on a live device.

## Environments

### Required Bench Setup

- One known-good ESP32 flashed with the current firmware
- One known-good LED wall or matrix target
- One stable Wi-Fi network for STA mode validation
- One AP-mode validation path for first boot and reset scenarios
- Known-safe test API credentials or a clearly documented placeholder device profile
- At least one valid `.rgb565` logo file for upload testing

### Recommended Device Targets

- `bench-ap`: clean device expected to serve onboarding
- `bench-sta`: configured device expected to serve dashboard

## Entry Criteria

QA should begin when:

- Firmware builds successfully
- Required web assets are regenerated after any `firmware/data-src/` edits
- Unit tests are runnable in the intended environment
- The target device is flashed and reachable on the test network

## Exit Criteria

A release candidate is considered QA-passed when:

- Relevant firmware unit tests pass
- The automated smoke suite passes against the release candidate device
- The manual release smoke checklist is completed with no unresolved critical failures
- Any destructive checks such as reboot and factory reset pass
- No open defects remain in the boot, onboarding, dashboard, health, or recovery flows

## Initial QA Rollout

### Phase 1: Establish Baseline

- Keep the existing PlatformIO unit tests as the logic safety net
- Use the new smoke suite to validate core HTTP routes on a live device
- Run the manual smoke checklist on one bench device

### Phase 2: Expand High-Value Coverage

- Add write-roundtrip smoke checks for safe hot-reload settings
- Add negative API payload tests for validation paths
- Add a scripted logo upload and delete smoke scenario

### Phase 3: Add Richer UI Automation

- Introduce browser automation once the live-device access pattern is stable
- Focus browser automation on the onboarding happy path and dashboard happy path
- Keep destructive flows as explicit opt-in tests

## Test Cadence

### During Development

- Run unit tests whenever firmware logic changes
- Run the smoke suite when web portal routes or JSON contracts change

### Before a Release

- Flash a clean bench device
- Run the automated smoke suite
- Run the manual release smoke checklist
- Run destructive checks last

## Defect Triage

### Severity 1

- Device cannot onboard
- Device cannot serve dashboard or health page
- Settings persistence is broken
- Factory reset or reboot bricks the test path

### Severity 2

- Feature works but key API/UI behavior is partially broken
- Logo workflow or calibration workflow is unreliable
- Health information is incomplete or misleading

### Severity 3

- Cosmetic UI issues
- Low-value style problems
- Documentation gaps without runtime impact

## First Artifacts in This QA Process

- QA plan: `_bmad-output/test-artifacts/qa-plan.md`
- Release smoke checklist: `_bmad-output/test-artifacts/release-smoke-checklist.md`
- Automated smoke suite: `tests/smoke/test_web_portal_smoke.py`

## How to Start

1. Flash one bench device and record its reachable base URL.
2. Run the smoke suite against that device.
3. Execute the release smoke checklist on the same build.
4. Log failures by feature area, not by scanner warning category.

## Recommended Next Expansion

- Add a logo upload roundtrip check to automation
- Add an opt-in reboot roundtrip check
- Add an opt-in factory reset validation path
- Add browser automation only after the bench-device flow is stable
