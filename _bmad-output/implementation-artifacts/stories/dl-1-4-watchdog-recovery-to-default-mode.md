# Story dl-1.4: Watchdog Recovery to Default Mode

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want the device to recover automatically from a crash by rebooting into Clock Mode,
So that a display mode bug causes a brief reboot, not a bricked or stuck device.

## Acceptance Criteria

1. **Given** **`"clock"`** exists in **`MODE_TABLE`** (**dl-1.1**), **When** the SoC reboots because of a **task watchdog** event (**`esp_reset_reason()`** in **`{ ESP_RST_TASK_WDT, ESP_RST_INT_WDT, ESP_RST_WDT }`** — verify against your **ESP-IDF** version), **Then** the **first** boot path **does not** blindly restore a potentially bad **`disp_mode`** as the **only** option: apply **known-good** **`clock`** (**epic** **NFR11**) **or** attempt saved mode **once** and **fall back** to **`clock`** on **`init()`** failure (**epic** second AC). **Document** the exact policy in Dev Agent Record (prefer **always `clock`** after WDT for simplicity).

2. **Given** a **normal** power-on / **software** reset (**reason** not watchdog), **When** **`disp_mode`** is **valid** and **`ModeRegistry::requestSwitch(saved)`** succeeds, **Then** behavior matches **today’s** restore (**epic** third AC — **no** forced **clock**).

3. **Given** **any** boot (watchdog or not), **When** **`disp_mode`** references an **unknown** id **or** **`requestSwitch`** fails **before** the first successful **`ModeRegistry::tick`**, **Then** fall back to **`"clock"`** if registered, else **`"classic_card"`** (safe until **clock** ships — **document** transition). **Update** **`ConfigManager::setDisplayMode`** to the **mode actually activated** so NVS does not retain a **dead** id (**same** invariant as **ds-3.2**).

4. **Given** **`ModeRegistry::executeSwitch`** reports **heap** / **`init()`** failure for the **requested** mode, **When** on a **post-watchdog** boot path, **Then** retry **once** with **`clock`** and persist **`disp_mode="clock"`**; **log** **`LOG_W`** with prior mode id.

5. **Given** **watchdog coverage** today (**`displayTask`** only: **`esp_task_wdt_add(NULL)`** + **`esp_task_wdt_reset()`** in **`main.cpp`**), **When** this story is implemented, **Then** evaluate **`loop()`** (Core **1**) for **stall** risk: if **epic** **NFR10** (“reboot within **10s**”) requires **main** supervision, add **`esp_task_wdt_init(..., true)`** / **`esp_task_wdt_add`** for **`loop`** **or** enable **Interrupt Watchdog** via **`sdkconfig`** / **`platformio.ini`** build flags — **document** measured **timeout** and **which** task(s) are enrolled.

6. **Given** **architecture rule 24** (**dl-1.3**), **When** applying **boot** / recovery **`requestSwitch`**, **Then** route through **`ModeOrchestrator`** (**`onManualSwitch`** or **`tick`**-only policy) **so** **`main.cpp`** does **not** call **`ModeRegistry::requestSwitch`** directly after **dl-1.3** lands (**coordinate** ordering if both stories split across PRs).

7. **Given** **`pio test`** / **`pio run`**, **Then** add a **unit** test for **`esp_reset_reason()`** policy using **stubbed** reason if feasible, **or** document **hardware-only** verification; **no** new warnings.

## Tasks / Subtasks

- [x] Task 1: **Reset reason + boot branch** (**AC: #1–#3**)
  - [x] 1.1: Early **`setup()`**, read **`esp_reset_reason()`**; set **`static bool s_wdtRecoveryBoot`**
  - [x] 1.2: Branch **`disp_mode`** restore vs **forced clock**

- [x] Task 2: **Watchdog configuration** (**AC: #5**)
  - [x] 2.1: Confirm default **TWDT** timeout; adjust to **≤10s** if needed
  - [x] 2.2: Optional **`loop()`** task enrollment

- [x] Task 3: **Orchestrator + NVS** (**AC: #3, #4, #6**)
  - [x] 3.1: On forced **clock**, **`ModeOrchestrator::init`** or **`onManualSwitch("clock", …)`** + **`setDisplayMode`**

- [x] Task 4: Tests / docs (**AC: #7**)

## Dev Notes

### Product vs epic

| Epic text | Product |
|-----------|---------|
| NVS **`display_mode`** | **`disp_mode`** (**`ConfigManager`**) |
| Default **clock** on invalid | Today boot uses **`classic_card`** for invalid id — **this** story moves default to **`clock`** when **`clock`** is registered |

### Current baseline

- **`displayTask`** already calls **`esp_task_wdt_reset()`** each frame (~**20fps**).
- Boot restore: **`main.cpp`** ~701–711 **`requestSwitch(saved)`** → **`classic_card`** on failure.

### Dependencies

- **dl-1.1** — **`ClockMode`** + **`"clock"`** in **`MODE_TABLE`**.
- **dl-1.3** — **rule 24** boot path through **`ModeOrchestrator`**.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-1.md` (Story dl-1.4)
- Prior: `_bmad-output/implementation-artifacts/stories/dl-1-3-manual-mode-switching-via-orchestrator.md`
- ESP-IDF: [`esp_reset_reason`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html#_CPPv418esp_reset_reasonv), task WDT APIs **`esp_task_wdt_*`**

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

None required — build clean, no new warnings.

### Implementation Plan

**WDT Recovery Policy (AC #1, preferred "always clock" approach):**
After any watchdog reset (`ESP_RST_TASK_WDT`, `ESP_RST_INT_WDT`, `ESP_RST_WDT`), the boot path unconditionally forces clock mode. The rationale: if the previously-active mode caused a crash leading to WDT reset, blindly restoring that mode would immediately crash again. Forcing the lightest-weight mode (clock) ensures the device remains operational and accessible via the dashboard for the owner to manually switch back.

**Watchdog Configuration (AC #5):**
- Default TWDT timeout = 5s (CONFIG_ESP_TASK_WDT_TIMEOUT_S=5), well within NFR10's 10s requirement.
- Core 0 (displayTask): already enrolled via `esp_task_wdt_add(NULL)` + `esp_task_wdt_reset()` each frame.
- Core 1 (loopTask/loop()): NOW enrolled via `esp_task_wdt_add(NULL)` in setup(), with `esp_task_wdt_reset()` at top of each loop() iteration and around blocking HTTP fetch operations to prevent false positives.

**Fallback Chain (AC #3):** Unknown mode ID → try "clock" (if registered) → else "classic_card". NVS corrected immediately to the actually-activated mode.

**Rule 24 Compliance (AC #6):** All boot/recovery mode switches routed through `ModeOrchestrator::onManualSwitch()`. No direct `ModeRegistry::requestSwitch()` calls from main.cpp.

### Completion Notes List

- Task 1: Added `esp_system.h` include, `g_wdtRecoveryBoot` flag, early `esp_reset_reason()` detection in setup(). Replaced boot restore block with WDT-aware branching: WDT → force clock; normal → restore NVS with fallback chain (clock → classic_card).
- Task 2: Confirmed TWDT timeout = 5s (meets NFR10). Enrolled loop() in TWDT via `esp_task_wdt_add(NULL)` in setup(). Added `esp_task_wdt_reset()` at top of loop() and around blocking HTTP fetch calls.
- Task 3: WDT recovery path routes through `ModeOrchestrator::onManualSwitch("clock", "Clock")` + `ConfigManager::setDisplayMode("clock")` — fully compliant with Rule 24. Normal boot fallback chain also routes through orchestrator.
- Task 4: Added 6 unit tests to test_mode_orchestrator covering WDT recovery policy, NVS persistence, normal boot restore, unknown mode fallback, orchestrator routing, and manual mode ID preservation. Tests use `simulateBootRestore()` helper that mirrors the exact main.cpp policy logic. Hardware-only verification noted for `esp_reset_reason()` itself (cannot stub IDF function in on-device tests).
- 2026-04-16 (Code Review Synthesis): Fixed test isolation bug in `test_wdt_boot_uses_orchestrator_not_direct_registry` — test now properly calls `initOrchestratorWithRegistry()` at the start to ensure clean state, preventing dependency on previous test execution order.

### File List

- `firmware/src/main.cpp` — Modified: WDT detection, boot branch, loop() WDT enrollment
- `firmware/test/test_mode_orchestrator/test_main.cpp` — Modified: 6 new dl-1.4 tests

### Change Log

- 2026-04-14: Story dl-1.4 implemented — watchdog recovery boot detection, forced clock mode on WDT reset, loop() TWDT enrollment, 6 unit tests added.

## Previous story intelligence

- **dl-1.3** centralizes **`requestSwitch`** entry points — **dl-1.4** boot/recovery must **reuse** that design.

## Git intelligence summary

Touches **`main.cpp`**, possibly **`platformio.ini` / `sdkconfig`**, **`ModeOrchestrator`**, **`ConfigManager`**.

## Project context reference

`_bmad-output/project-context.md` — **Core 0** display task, **`disp_mode`**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-16
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 2.0 (Validator A), 2.0 (Validator B) → APPROVED
- **Issues Found:** 1
- **Issues Fixed:** 1
- **Action Items Created:** 0
