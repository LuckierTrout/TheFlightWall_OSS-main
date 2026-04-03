---
title: 'Story 4.2 deferred runtime calibration and mapping fixes'
type: 'bugfix'
created: '2026-04-03'
status: 'draft'
context:
  - '_bmad-output/planning-artifacts/architecture.md'
  - '_bmad-output/planning-artifacts/ux-design-specification.md'
  - '_bmad-output/planning-artifacts/epics.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Two Story 4.2 deferred defects remain in the display runtime. Calibration mode is toggled across cores through `volatile bool`, which does not provide safe cross-core synchronization on ESP32, and a single settings save that changes both matrix geometry and matrix mapping skips the live mapping refresh path, so calibration feedback can stay wrong until reboot.

**Approach:** Harden calibration mode signaling with an atomic flag, and restructure display-task hardware change handling so mapping updates are still applied immediately to the currently running matrix when geometry and mapping change together, while preserving the existing contract that geometry and pin changes still require reboot before new panel dimensions or pin wiring take effect.

## Boundaries & Constraints

**Always:** Preserve the existing settings API envelope and key names. Keep geometry and display-pin changes in the reboot-required path. Keep calibration feedback responsive on the currently running matrix when only mapping changes, or when mapping changes are saved together with pending geometry changes. Prefer small, testable helpers over broad task-loop rewrites.

**Ask First:** Any change that would alter the reboot contract for `tiles_x`, `tiles_y`, `tile_pixels`, or `display_pin`; any change that removes or renames existing calibration APIs or settings keys.

**Never:** Hot-apply new panel dimensions or display pin assignments without reboot. Add new dependencies. Rework unrelated display rendering, flight cycling, or dashboard behavior.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Calibration flag crosses cores | Core 1 enables or disables calibration while Core 0 display loop is polling mode state | Display task observes the latest mode safely and renders either calibration or flights without relying on undefined cross-core visibility | No crashes, torn reads, or stale `volatile`-only behavior |
| Mapping-only save | `origin_corner`, `scan_dir`, or `zigzag` changes while geometry is unchanged | Runtime matrix mapping is rebuilt immediately and calibration pattern reflects the new orientation without reboot | Failed runtime reconfigure is logged and current matrix stays usable |
| Geometry and mapping saved together | A single settings apply persists new geometry keys and new mapping keys | Display task keeps geometry in reboot-required state but still refreshes live mapping against the current runtime geometry so calibration feedback matches the new mapping before reboot | Failure is logged without corrupting runtime state; reboot path still applies persisted geometry later |
| Geometry-only save | `tiles_x`, `tiles_y`, `tile_pixels`, or `display_pin` changes with no mapping change | Existing reboot-required behavior remains unchanged | No accidental runtime matrix rebuild on unsupported geometry changes |

</frozen-after-approval>

## Code Map

- `firmware/adapters/NeoMatrixDisplay.h` -- calibration mode storage and public runtime-control surface
- `firmware/adapters/NeoMatrixDisplay.cpp` -- calibration flag accessors and matrix reconfiguration behavior
- `firmware/src/main.cpp` -- display-task config-change classification and runtime apply flow
- `firmware/test/` -- targeted unit coverage for change classification or combined-update decision logic
- `_bmad-output/implementation-artifacts/deferred-work.md` -- mark these deferred items resolved once implemented

## Tasks & Acceptance

**Execution:**
- [ ] `firmware/adapters/NeoMatrixDisplay.h` -- Replace `_calibrationMode` with `std::atomic<bool>` and keep the existing accessor API intact -- removes unsafe cross-core state sharing without widening the public surface
- [ ] `firmware/adapters/NeoMatrixDisplay.cpp` -- Update calibration accessors for atomic load/store semantics and add the minimal runtime reconfiguration support needed to apply mapping safely against the active geometry -- preserves current rendering behavior while fixing the live-update gap
- [ ] `firmware/src/main.cpp` -- Refactor hardware change handling so geometry and mapping are evaluated independently instead of `if/else if`, and ensure combined saves still hot-apply mapping while logging that geometry remains reboot-required -- fixes the missed combined-change path without changing reboot expectations
- [ ] `firmware/test/` -- Add focused automated coverage for the combined geometry-plus-mapping decision path and the geometry-only / mapping-only split -- protects the non-obvious regression boundary that caused this deferred work
- [ ] `_bmad-output/implementation-artifacts/deferred-work.md` -- Move both open Story 4.2 items into the resolved section with a concise note about the chosen fix -- keeps the implementation artifact current

**Acceptance Criteria:**
- Given calibration mode is toggled from the web task, when the display task reads that state on the other core, then the latest value is observed through a synchronization-safe mechanism rather than `volatile` alone
- Given only matrix mapping keys change, when settings are applied, then the live calibration pattern updates without reboot and normal reboot requirements stay unchanged for geometry keys
- Given geometry keys and mapping keys change in the same settings save, when the display task processes the config change, then mapping is refreshed for the currently running matrix and geometry is still reported as reboot-required
- Given only geometry or display pin changes, when settings are applied, then the system does not attempt an unsupported runtime geometry hot-swap
- Given firmware validation is run, when the targeted tests and PlatformIO build complete, then the modified paths pass without new errors

## Spec Change Log

## Design Notes

The tricky case is a mixed save where persisted geometry no longer matches the active matrix instance. The implementation should not treat that as permission to hot-swap panel dimensions. Instead, it should separate two concerns:

1. runtime geometry contract: keep using the current in-memory geometry until reboot
2. runtime mapping contract: allow orientation and zigzag changes to be rendered immediately on that current geometry

That means the display task should not rely on a single `if/else if` branch for hardware changes. It should classify geometry changes and mapping changes independently, then apply only the safe live subset.

## Verification

**Commands:**
- `cd firmware && ~/.platformio/penv/bin/pio run` -- expected: SUCCESS with no new compile errors in modified display-runtime files
- `cd firmware && ~/.platformio/penv/bin/pio test -e esp32dev --without-uploading` -- expected: targeted test sources for the new decision logic compile cleanly in the configured PlatformIO environment

**Manual checks (if no CLI):**
- Open calibration mode, change only `origin_corner` / `scan_dir` / `zigzag`, and confirm the LED pattern still updates immediately
- Save a geometry change together with a mapping change, and confirm the UI/runtime still indicates reboot is needed for geometry while the live calibration mapping reflects the new orientation on the currently running matrix
