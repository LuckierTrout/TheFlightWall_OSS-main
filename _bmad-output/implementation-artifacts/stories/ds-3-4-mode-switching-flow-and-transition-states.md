# Story ds-3.4: Mode Switching Flow & Transition States

Status: complete

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user,
I want to tap a mode card to activate that mode with clear visual feedback during the switch,
So that I know the switch is happening, when it completes, and if anything goes wrong.

## Acceptance Criteria

1. **Given** a **non-active** mode card is **tapped** (pointer), **When** **`switchMode(targetId)`** runs, **Then** the **target** card enters **switching** state:
   - **`opacity: 0.7`** (epic **UX-DR1** switching),
   - **pulsing** accent border **animation** on the target card (CSS **`@keyframes`**; respect **AC #8**),
   - visible **"Switching..."** label on that card (reuse or extend **`.mode-card-subtitle`** or a dedicated row),
   - **`POST /api/display/mode`** is sent with **`{ "mode": "<id>" }`** (fallback **`mode_id`** only if needed).

2. **Given** switching has started, **When** the request is in flight, **Then** **all sibling** cards that are **not** the target and **not** the previous active card (or: **all** non-target cards per epic **UX-DR10**) enter **disabled** state: **`opacity: 0.5`**, **`pointer-events: none`**, add class e.g. **`.mode-card.disabled`** — **including** the previously active card so the user cannot fire concurrent switches.

3. **Given** the **firmware** confirms success, **When** the client determines the switch **completed**, **Then**:
   - the **previously active** card loses **active** chrome (**4px** rail, dot, **"Active"**) and becomes **idle**,
   - the **target** card leaves **switching** and becomes **active** with epic **active** styling (**FR22**),
   - **all** **disabled** siblings return to **idle** (remove **`.disabled`**, restore **pointer-events**),
   - **focus** moves from the **new** active card to the **previously active** card (now idle) per **UX-DR6** — use **`element.focus({ preventScroll: true })`** if that card is **focusable**; if **ds-3.5** has not yet added **`tabindex`**, add **`tabindex="-1"`** on cards **only** for programmatic focus **or** document a minimal **`tabindex="0"`** on idle cards for this story (coordinate with **ds-3.5** so roles are not duplicated).

4. **Given** a **failure** path, **When** the UI must show **heap / registry** failure, **Then** the **target** card shows **scoped** inline copy: **"Not enough memory to activate this mode. Current mode restored."** (**UX-DR11**) when the failure is **heap-related**; for other errors, show **`res.body.error`** or **`data.registry_error`** truncated **inline on the card** (still **scoped**, not only a global toast).
   - Error host: **`role="alert"`**, **`aria-live="polite"`** (**UX-DR8**).
   - Auto-dismiss error **after 5s** **or** on **next** user **click** / **keydown** anywhere in **`#mode-picker`** (document listener with **`{ once: true }`** or explicit cleanup).

5. **Given** **`prefers-reduced-motion: reduce`**, **When** a card is **switching**, **Then** **no** pulsing border — use **static** **`var(--primary)`** or **`--accent-dim`** border at **full opacity** (**UX-DR5**); **"Switching..."** text remains the primary progress signal.

6. **Given** **network** failure (**`catch`** on **`FW.post`**), **When** the request fails, **Then** clear **switching** / **disabled** classes, restore cards to last known **`GET`** state (call **`loadDisplayModes()`**), keep **toast** for connectivity (existing pattern is fine); **do not** leave cards stuck **disabled**.

7. **Given** **async** registry (**`WebPortal`** comment: **no** bounded wait in POST handler — **`ds-3.1`**), **When** **`POST`** returns **`ok: true`** with **`data.switch_state`** **`"requested"`** or **`"switching"`**, **Then** the client **polls** **`GET /api/display/modes`** at a **small interval** (e.g. **150–300ms**) until **`data.switch_state === "idle"`** **or** **`data.active === targetId`** **or** **timeout** (e.g. **2s** aligned with **architecture** cap discussion), then **finalizes** UI. If **`data.registry_error`** is non-empty **or** **`data.active`** is still not **`targetId`** after idle, treat as **failure** for **AC #4** messaging (wording may reference **registry** vs **heap** based on **`code`** if present on future responses).

8. **Given** **`POST`** returns **`ok: false`** (**HTTP 400** etc.), **When** **`FW.post`** surfaces **`res.body`**, **Then** apply **AC #4** on the **target** card (unknown mode, missing field) with appropriate **short** message; clear **disabled** state on siblings.

9. **Given** **`style.css`** / **`dashboard.js`** change, **When** shipping, **Then** regenerate **`firmware/data/*.gz`** per **`AGENTS.md`**.

10. **Given** manual test on device, **When** switching **Classic** ↔ **Live Flight Card**, **Then** states match epic (**switching** → **active** / **idle**) with **no** stuck **pointer-events: none** on the list.

## Tasks / Subtasks

- [x] Task 1: CSS — states (**UX-DR1**, **UX-DR10**, **UX-DR5**)
  - [x] 1.1: **`.mode-card.switching`** — pulse keyframes + **0.7** opacity (media query for **reduced-motion**)
  - [x] 1.2: **`.mode-card.disabled`** — **0.5** opacity, **`pointer-events: none`**
  - [x] 1.3: **`.mode-card-error`** (or inner **`.mode-card-alert`**) for **scoped** error text

- [x] Task 2: JS — **`switchMode`** orchestration
  - [x] 2.1: Track **`previousActiveId`** before POST; apply **disabled** to siblings
  - [x] 2.2: Implement **GET poll** loop after POST **200** until settled or timeout; then **`updateModeStatus`** / re-render
  - [x] 2.3: **Focus** move **new active** → **previous** idle card (**UX-DR6**)
  - [x] 2.4: Error dismiss timer + **interaction** listener

- [x] Task 3: Wire **`mode`** in POST body (if not already from **ds-3.3**)

- [x] Task 4: Gzip + smoke check dashboard mode picker

## Dev Notes

### Current code (baseline)

- **`switchMode`**: sets **`.switching`** on target only; **opacity 0.6** in CSS today — adjust to **0.7** per epic; **no** sibling **disabled**; **no** poll — single **`loadDisplayModes()`** after POST.
- **`updateModeStatus`**: strips **`.switching`** on all cards after GET — may **fight** mid-poll unless poll owns UI state; refactor so **poll completion** drives final class cleanup.
- **`WebPortal`** **`POST /api/display/mode`**: documents **async** completion; response **`ok: true`** with **`switch_state`** / **`registry_error`** fields — **client must poll GET** for truth (**AC #7**).

### Coordination with ds-3.3

- **ds-3.3** adds **schematic**, **`#mode-picker`**, **active** chrome — **ds-3.4** must not regress those selectors; prefer classes **on the same** **`.mode-card`** nodes.

### Out of scope

- **ds-3.5** — full **keyboard** **`role="button"`** model (**Enter**/**Space**) — if **ds-3.4** adds **`tabindex`** for focus only, keep minimal.
- **ds-3.6** — upgrade banner.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.4)
- Prior: `_bmad-output/implementation-artifacts/stories/ds-3-3-mode-picker-section-cards-and-schematic-previews.md`
- API: `_bmad-output/implementation-artifacts/stories/ds-3-1-display-mode-api-endpoints.md`
- Firmware: `firmware/adapters/WebPortal.cpp` (**POST** **`/api/display/mode`**), `firmware/data-src/dashboard.js`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

### Completion Notes List

- Rewrote `switchMode()` with full state machine: switching → polling → finalize/error
- Added `previousActiveId` tracking to correctly restore previous active card to idle state
- Implemented GET poll loop (200ms interval, 2s timeout) for async mode registry response
- Added `clearSwitchingStates()` helper to prevent stuck disabled/switching states
- Added `showCardError()` with scoped inline error display, `role="alert"`, `aria-live="polite"`, auto-dismiss after 5s or user interaction
- Added `tabindex="-1"` on mode cards for programmatic focus per AC #3 / UX-DR6
- `loadDisplayModes()` now returns the Promise for chainable `.then()` in `finalizeModeSwitch`
- CSS: `.mode-card.switching` updated from 0.6 to 0.7 opacity, added `mode-pulse` keyframes for accent border animation
- CSS: Added `.mode-card.disabled` (0.5 opacity, pointer-events:none) and `.mode-card-alert` for scoped error
- CSS: Added `prefers-reduced-motion` override to disable pulse animation
- POST body confirmed sending `{ mode: id }` (already established in ds-3.3)
- Gzipped style.css and dashboard.js to firmware/data/

### Change Log

- 2026-04-14: Implemented ds-3.4 Mode Switching Flow & Transition States — full switchMode orchestration with polling, scoped errors, focus management, and CSS state classes
- 2026-04-14: Code review synthesis (5 issues fixed): premature active removal on siblings (AC #3), bricked UI in finalizeModeSwitch on reload failure, race condition unlocking modeSwitchInFlight before loadDisplayModes resolves (3 paths), showCardError dismiss closure removing wrong element, prefers-reduced-motion opacity:1 (AC #5)
- 2026-04-14: Verification pass — all 5 review fixes confirmed in source, gzip assets verified matching data-src originals, all 10 ACs validated against implementation. Status → complete.
- 2026-04-14: Code review synthesis pass 2 (2 issues fixed): event bubbling from .mode-card-alert triggering accidental switchMode (HIGH — added stopPropagation on alert click); double-toast on network failure in AC #6 catch block (MEDIUM — removed redundant FW.showToast, loadDisplayModes covers it). Regenerated dashboard.js.gz.
- 2026-04-14: Code review synthesis pass 3 (3 issues fixed): race condition in finalizeModeSwitch success path — modeSwitchInFlight=false now deferred inside loadDisplayModes().then() for the !skipReload branch (HIGH); missing CSS truncation on .mode-card-alert — added overflow:hidden;text-overflow:ellipsis;white-space:nowrap per AC #4 (LOW); visual jitter when active card transitions to switching — added .mode-card.switching[aria-current="true"] CSS rule to preserve 4px border-left and 9px padding (LOW). Regenerated dashboard.js.gz and style.css.gz.

### File List

- `firmware/data-src/style.css` (modified)
- `firmware/data-src/dashboard.js` (modified)
- `firmware/data/style.css.gz` (regenerated)
- `firmware/data/dashboard.js.gz` (regenerated)

## Previous story intelligence

- **ds-3.3** establishes **card DOM**, **schematic**, **`#mode-picker`**, and **POST** **`mode`** preference — **ds-3.4** layers **state machine** + **polling** on top.

## Git intelligence summary

Touches **`firmware/data-src/dashboard.js`**, **`style.css`**, possibly **`dashboard.html`** (error host markup optional — can be created in JS).

## Project context reference

`_bmad-output/project-context.md` — gzip **data-src** → **data/**.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.

## Senior Developer Review (AI)

### Review: 2026-04-14
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 5.5 → REJECT (MAJOR REWORK)
- **Issues Found:** 5
- **Issues Fixed:** 5
- **Action Items Created:** 0

### Review: 2026-04-14 (Pass 2)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 3.1 → PASS WITH FIXES
- **Issues Found:** 6 raised (2 verified, 4 dismissed)
- **Issues Fixed:** 2
- **Action Items Created:** 0

### Review: 2026-04-14 (Pass 3)
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 1.6 → PASS WITH FIXES
- **Issues Found:** 3 raised (3 verified, 0 dismissed) — Validator A non-functional
- **Issues Fixed:** 3
- **Action Items Created:** 0
