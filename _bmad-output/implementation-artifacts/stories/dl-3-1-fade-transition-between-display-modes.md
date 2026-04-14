# Story dl-3.1: Fade Transition Between Display Modes

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As an owner,
I want mode switches to use a smooth fade animation instead of a hard cut,
So that the wall feels like a polished product and transitions look intentional to guests.

## Acceptance Criteria

1. **Given** **`ModeRegistry::executeSwitch`** runs a **successful** switch (**heap OK**, **`factory`**, **`init()`** all succeed), **When** the new mode is ready to take over the matrix, **Then** **`ModeRegistry`** (or a dedicated **`ModeTransition`** helper in the same module) runs **`_executeFadeTransition(ctx, flights, …)`** **after** **`init()`** succeeds and **before** **`_switchState`** returns to **`IDLE`**, producing a **linear crossfade** from the **last outgoing** LED frame to the **first incoming** frame (**epic** / **FR12**).

2. **Given** **pixel dimensions** **`W = ctx.layout.matrixWidth`**, **`H = ctx.layout.matrixHeight`**, **When** allocating crossfade buffers, **Then** use **`size_t pixelCount = (size_t)W * (size_t)H`**, **`bytes = pixelCount * 3`** per **RGB888** buffer (**two** buffers). **Do not** hardcode **160×32** (**epic** **AR1** is an **example** — product matrix is **config-driven**). If **`W` or `H` is zero**, skip fade (**instant** path) + **`LOG_W`**.

3. **Given** **`malloc`** succeeds for **both** buffers, **When** the blend loop runs, **Then** use **~15** blend steps over **≤ 1s** total (**NFR2**), minimum effective **~15fps** average (**NFR1** — e.g. **15** steps × **`vTaskDelay(pdMS_TO_TICKS(66))`** on the **display task**). Use **`vTaskDelay`** (not **`delay()`**) so **FreeRTOS** scheduling semantics match **Core 0** task context.

4. **Given** each blend step, **When** writing LEDs, **Then** **lerp** each channel **A → B** with **integer** math (fixed-point **0–255** alpha) and push pixels through the **existing** **`FastLED`/`NeoMatrix`** path used for normal frames (**no** tearing beyond hardware limits — **FR12**).

5. **Given** **outgoing** pixels must exist **before** **`teardown()`** destroys drawable state, **When** **`executeSwitch`** begins, **Then** **snapshot** the **current** **`CRGB`** buffer (**`FastLED`** leds, length **`NUM_LEDS`**) into buffer **A** **while** the **old** mode is still the active instance **or** immediately after a **final** **`_activeMode->render(ctx, flights)`** if the on-device buffer could be stale — **document** the chosen snapshot point in Dev Agent Record (**must** be **before** **`teardown()`** if **`teardown`** clears backing state).

6. **Given** **`malloc`** fails for either buffer, **When** detected, **Then** **`free`** any partial allocation, **`LOG_W`** graceful degradation (**FR34** / **rule 27**), and perform **instant** handoff (**current** behavior: new mode already **`init`**, **`render`** will run in same **`tick`** — **no** crash).

7. **Given** **fade completes**, **When** returning, **Then** **`free`** both buffers **immediately** (**rule 27** — **no** retention on **`DisplayMode`** instances). **NFR17**: no repeated **malloc** without **free** on subsequent switches.

8. **Given** **`_switchState == Switching`**, **When** fade runs, **Then** **`executeSwitch`** must **not** re-enter **`executeSwitch`** on the same **`tick`** (consume **`_requestedIndex`** **before** fade as today; during fade **ignore** new **`requestSwitch`** until **IDLE** — **document** if **latest-wins** should queue — default **drop** requests during fade **or** document **overwrite** of **`_requestedIndex`** after fade).

9. **Given** **architecture** **FR35** (single **`show()`** per outer frame), **When** fade runs **multiple** **`show()`** calls, **Then** document **explicit** exception for **transition** frames only; keep **`SWITCHING`** semantics so **orchestrator**/**HTTP** paths remain coherent (**epic** “**SWITCHING** prevents interference”).

10. **Given** **`pio run`** / **`pio test`**, **Then** **no** new warnings; add **native** tests only if **malloc** / fade math can be unit-tested without hardware — otherwise **document** on-device verification steps.

## Tasks / Subtasks

- [x] Task 1: **Snapshot + buffer math** — **`LayoutResult`** / **`NUM_LEDS`** consistency (**AC: #2, #5**)
- [x] Task 2: **`_executeFadeTransition`** in **`ModeRegistry.cpp`** (or **`ModeTransition.cpp`**) (**AC: #1, #3, #4, #6, #7**)
- [x] Task 3: **`executeSwitch`** sequencing — insert fade after **`init`**, adjust **`_switchState`/`IDLE`** timing (**AC: #1, #8, #9**)
- [x] Task 4: **Logging + manual** matrix-size matrix test (**AC: #10**)

## Dev Notes

### Current **`executeSwitch`** (baseline)

- **`ModeRegistry.cpp`**: **`SWITCHING` → teardown → delete → factory → init → IDLE`**, then **`tick`** renders new mode — **no** fade, **no** snapshot today.

### Snapshot source

- **`FastLED`** global **`leds`** / **`NUM_LEDS`** — confirm project uses **RGB** order consistent with **RGB888** packing in fade (**`CRGB`** layout).

### Dependencies

- **ds-1.5** — **`ModeRegistry::tick`** / **`executeSwitch`** location and **`RenderContext`** shape.
- **ds-3.4** dashboard may show **`switch_state: switching`** longer — ensure **API** semantics still **truthful** (**≤ ~1s** extra **switching** window).

### Out of scope

- **Dashboard** CSS fades (**ds-3.4**).
- **Audio** / **haptic** feedback.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-dl-3.md` (Story dl-3.1)
- Code: `firmware/core/ModeRegistry.cpp`, `firmware/core/ModeRegistry.h`
- Prior epic context: `_bmad-output/implementation-artifacts/stories/dl-2-2-dynamic-row-add-and-remove-on-flight-changes.md` (polish layer after **dl-2**)

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Implementation Plan

**Snapshot point decision (AC #5):** Snapshot is taken BEFORE `teardown()` in `executeSwitch()`. The `FastLED.leds()` buffer contains the last rendered frame from the outgoing mode. This is the correct point because `teardown()` may clear backing drawable state. The CRGB buffer is copied into a heap-allocated RGB888 buffer A before teardown runs.

**Fade integration approach:** Rather than a separate `_executeFadeTransition()` helper, the fade logic is integrated directly into `executeSwitch()` because it needs access to the snapshot buffer allocated before teardown and the incoming frame captured after init. This avoids passing multiple buffer pointers across functions.

**Re-entrancy policy (AC #8):** During fade, `_switchState` remains `SWITCHING`. New `requestSwitch()` calls write to `_requestedIndex` atomically (latest-wins), but `tick()` does not consume them until `executeSwitch` returns and state becomes `IDLE`. Requests arriving during fade effectively get dropped (overwritten by next request or consumed on next tick). This is the safest approach — no queuing complexity.

**FR35 exception (AC #9):** The fade loop calls `FastLED.show()` 15 times during the transition. This is an explicit documented exception to the single-show-per-frame rule. The `SWITCHING` state prevents orchestrator/HTTP interference during this window (~1s).

### Debug Log References

- Fade snapshot: `[ModeRegistry] Fade snapshot captured` + dimensions
- Fade start/complete: `[ModeRegistry] Fade transition starting/complete`
- Graceful degradation: `[ModeRegistry] Fade snapshot malloc failed — instant handoff`
- Zero dimensions: `[ModeRegistry] Fade skipped: matrix dimensions zero`

### Completion Notes List

- ✅ Task 1: Snapshot captures FastLED.leds() into RGB888 buffer A before teardown. Uses `ctx.layout.matrixWidth * matrixHeight` for config-driven pixel count (not hardcoded). Zero-dimension check skips fade with LOG_W.
- ✅ Task 2: Linear crossfade implemented inline in executeSwitch — 15 blend steps × 66ms = ~990ms ≤ 1s. Integer lerp per RGB channel using fixed-point 0–255 alpha. Both buffers freed immediately after fade (or on any early exit path). Graceful malloc failure degrades to instant handoff.
- ✅ Task 3: Fade runs after init() succeeds, before _switchState returns to IDLE. `executeSwitch` signature updated to accept `flights` vector for new mode's first render. _requestedIndex consumed before fade starts; new requests during fade are latest-wins overwrites consumed on next tick.
- ✅ Task 4: `pio run` produces no new warnings. Added 5 on-device tests: zero-layout no-crash, heap leak check, rapid switches, idle state return, blend math correctness. Test compilation verified clean.

### File List

- firmware/core/ModeRegistry.h (modified — added fade documentation comment, updated executeSwitch signature with flights param)
- firmware/core/ModeRegistry.cpp (modified — added fade snapshot/transition logic, FastLED/FreeRTOS includes)
- firmware/test/test_mode_registry/test_main.cpp (modified — added 5 fade transition tests)

### Change Log

- 2026-04-14: Story dl-3.1 implemented — linear crossfade transition between display modes in ModeRegistry::executeSwitch(). Added fade snapshot before teardown, ~15-step blend over ≤1s, graceful malloc fallback, and 5 unit tests.

## Previous story intelligence

- **dl-2.x** completes **Departures Board** motion semantics — **dl-3.1** is **global** **ModeRegistry** polish affecting **all** modes.

## Git intelligence summary

Touches **`ModeRegistry.*`**, possibly **`NeoMatrixDisplay`** if snapshot helpers live there, **`platformio.ini`** only if stack sizing required.

## Project context reference

`_bmad-output/project-context.md` — **Core 0** display task, **heap** headroom for **~2× RGB888** frame.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
