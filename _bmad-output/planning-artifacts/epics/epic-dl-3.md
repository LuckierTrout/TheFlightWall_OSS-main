## Epic dl-3: Polished Handoffs — Animated Transitions

Mode switches feel intentional, not broken. Smooth fade animations replace hard cuts. A non-technical observer perceives the wall as a finished product, not a prototype.

### Story dl-3.1: Fade Transition Between Display Modes

As an owner,
I want mode switches to use a smooth fade animation instead of a hard cut,
So that the wall feels like a polished product and transitions look intentional to guests.

**Acceptance Criteria:**

**Given** any mode switch is triggered (manual, idle fallback, or scheduled)
**When** ModeRegistry's `tick()` executes the switch lifecycle
**Then** `_executeFadeTransition()` is called after the new mode's `init()` succeeds, producing a smooth crossfade from the outgoing frame to the incoming frame

**Given** a fade transition is executing
**When** the blend loop runs
**Then** the transition renders at minimum 15fps (NFR1) on the 160x32 LED matrix with no visible tearing or artifacts (FR12)

**Given** a fade transition is executing
**When** the blend loop completes all frames
**Then** the total transition duration is under 1 second (NFR2) — approximately 15 frames at ~66ms per frame

**Given** a mode switch is triggered
**When** `_executeFadeTransition()` allocates dual RGB888 buffers
**Then** two buffers of `160 * 32 * 3 = 15,360 bytes` each (~30KB total) are `malloc()`'d at transition start (AR1)

**Given** the fade transition has rendered its last blend frame
**When** the transition completes
**Then** both RGB888 buffers are `free()`'d immediately — buffers never persist beyond the fade call (rule 27) and no progressive heap degradation occurs (NFR17)

**Given** a mode switch is triggered
**When** `malloc()` returns `nullptr` for either buffer (heap too low)
**Then** both buffers are freed (`free(nullptr)` is a no-op), the transition falls back to an instant cut (current behavior), the mode still switches successfully, and a log message is emitted — this is graceful degradation, not an error (FR34, rule 27)

**Given** the fade transition is running on Core 0 (display task)
**When** `delay()` is called between blend frames
**Then** the Core 1 main loop (web server, flight pipeline, orchestrator) is unaffected — `SWITCHING` state prevents the display task's `tick()` from interfering

**Given** the updated switch flow in `ModeRegistry::tick()`
**When** a mode switch completes
**Then** the sequence is: `_switchState = SWITCHING` → `teardown()` → `delete old mode` → heap check → `factory()` → `init()` → `_executeFadeTransition()` → `_switchState = IDLE` → set NVS pending — matching the architecture decision DL1

---

