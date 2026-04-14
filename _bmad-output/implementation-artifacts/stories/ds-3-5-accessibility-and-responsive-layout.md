# Story ds-3.5: Accessibility & Responsive Layout

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user with accessibility needs or using a phone,
I want the Mode Picker to be fully keyboard-navigable, screen-reader friendly, and responsive across devices,
So that I can control display modes regardless of how I access the dashboard.

## Acceptance Criteria

1. **Given** mode cards are rendered in **idle** state (not active, not switching, not disabled), **When** the user navigates by **keyboard**, **Then** each idle card exposes **`role="button"`** and **`tabindex="0"`** (**UX-DR7**). The **active** card has **`aria-current="true"`** (already from **ds-3.3** if implemented) and **must not** be a tab stop for activation: use **`tabindex="-1"`** on the active card **or** omit **`tabindex`** and rely on **`aria-current`** only — **pick one** pattern and document; epic requires active card **not** in tab order as an **actionable** element (no Enter-to-“switch” on already-active mode).

2. **Given** focus is on an **idle** mode card, **When** the user presses **Enter** or **Space**, **Then** **`switchMode(modeId)`** runs (same path as click) (**UX-DR7**). **`preventDefault`** on **Space** to avoid page scroll.

3. **Given** a **mode switch completes** (success path from **ds-3.4**), **When** focus management runs, **Then** focus moves from the **newly active** card to the **previously active** card (now idle, **`tabindex="0"`**) (**UX-DR6**). If **ds-3.4** already moves focus, **reconcile** so there is **one** authoritative focus step (avoid double **`focus()`** calls).

4. **Given** the **upgrade notification banner** (**ds-3.6**) is dismissed or removed, **When** focus management runs, **Then** focus moves to the **Mode Picker section heading** (**UX-DR6**). **Prerequisite:** add **`id="mode-picker-heading"`** (or equivalent) on the **`#mode-picker`** **`h2`** in **`dashboard.html`** so **ds-3.6** and this story share a stable target; heading should have **`tabindex="-1"`** for programmatic focus only.

5. **Given** a card is in **switching** state (**ds-3.4**), **When** assistive tech queries it, **Then** that card has **`aria-busy="true"`** until the switch finalizes (**UX-DR8**). Remove **`aria-busy`** when returning to idle/active.

6. **Given** a card is in **disabled** state (siblings during switch, **ds-3.4**), **When** assistive tech queries it, **Then** that card has **`aria-disabled="true"`** (**UX-DR8**). **Note:** **`aria-disabled="true"`** does not remove the element from the tab order in all browsers — prefer **`tabindex="-1"`** on disabled cards **in addition** to **`aria-disabled`** so keyboard users cannot land on unusable targets during the switch.

7. **Given** viewport **width &lt; 600px**, **When** the Mode Picker renders, **Then** **`.mode-cards-list`** is a **single column**, full width, with comfortable touch spacing (min **44px** vertical tap height per card row is desirable) (**UX-DR9** phone-first).

8. **Given** viewport **width ≥ 1024px**, **When** the Mode Picker renders, **Then** **`.mode-cards-list`** uses a **two-column CSS Grid** (e.g. **`grid-template-columns: repeat(2, minmax(0, 1fr))`**) with gap consistent with **`var(--gap)`** / **8px** (**UX-DR9** desktop).

9. **Given** the dashboard already uses responsive rules elsewhere, **When** adding **1024px** rules, **Then** they are **additive**: do **not** remove or break any existing **`@media (max-width: 600px)`** (or equivalent) patterns used by **`.dashboard`** / **`.dash-cards`** if present; if **no** **600px** breakpoint exists today, introduce **`max-width: 599px`** for the single-column mode list **and** document that **600px** behavior is preserved for future shared breakpoints.

10. **Given** **`dashboard.html`**, **`dashboard.js`**, **`style.css`** change, **When** shipping, **Then** regenerate **`firmware/data/*.gz`** per **`AGENTS.md`**.

11. **Given** manual verification, **When** testing with **VoiceOver** (macOS) or **NVDA** (Windows) on **`dashboard.html`**, **Then** mode names and active state are announced sensibly; switching/disabled states do not trap focus.

## Tasks / Subtasks

- [x] Task 1: HTML — heading id (**AC: #4**)
  - [x] 1.1: **`#mode-picker` `h2`**: add **`id="mode-picker-heading"`**, **`tabindex="-1"`**

- [x] Task 2: **`renderModeCards`** — ARIA + tab order (**AC: #1, #2, #5, #6**)
  - [x] 2.1: Set **`role`**, **`tabindex`**, **`aria-current`**, **`aria-busy`**, **`aria-disabled`** on card root
  - [x] 2.2: **`keydown`** listener on **`#modeCardsList`** (delegate) for **Enter**/**Space**

- [x] Task 3: Focus integration (**AC: #3, #4**)
  - [x] 3.1: Export or call a shared **`focusModePickerHeading()`** for **ds-3.6**
  - [x] 3.2: Align **`switchMode`** completion focus with **ds-3.4** (single owner)

- [x] Task 4: CSS — responsive grid (**AC: #7–#9**)
  - [x] 4.1: **`@media (max-width: 599px)`** — single column **`.mode-cards-list`**
  - [x] 4.2: **`@media (min-width: 1024px)`** — two-column grid

- [x] Task 5: Gzip assets (**AC: #10**)

## Dev Notes

### Current baseline (verify in tree)

- **`dashboard.html`**: **`#mode-picker`**, nav link **`#mode-picker`**, **`h2`** text **"Display Mode"** — may lack **`id`** / **`tabindex`** on heading.
- **`style.css`**: **`.mode-cards-list`** is **`display:flex; flex-direction:column`** — no **1024px** two-column rule yet; no dedicated **599px** block for mode list.
- **`dashboard.js`**: **`renderModeCards`** builds **`div.mode-card`** with **click** handler; likely **no** **`role="button"`** / **`keydown`** yet.

### Coordination

- **ds-3.4** — switching / disabled / focus after switch; this story **must not** regress polling or state classes.
- **ds-3.6** — banner dismiss → focus heading; implement **heading hook** here first.

### Out of scope

- New **design system** beyond epic tokens.
- **health.html** accessibility pass.

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.5)
- Prior: `_bmad-output/implementation-artifacts/stories/ds-3-4-mode-switching-flow-and-transition-states.md`, `ds-3-3-mode-picker-section-cards-and-schematic-previews.md`
- Sources: `firmware/data-src/dashboard.html`, `dashboard.js`, `style.css`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Firmware build: SUCCESS (79.2% flash usage)
- Unit tests: ERRORED (expected — ESP32-only tests require hardware upload, not available in dev environment)

### Completion Notes List

- **Task 1:** Added `id="mode-picker-heading"` and `tabindex="-1"` to the `#mode-picker` `h2` in `dashboard.html` for programmatic focus target (AC #4).
- **Task 2:** Updated `renderModeCards` to set `role="button"` and `tabindex="0"` on idle cards, `tabindex="-1"` on active card. Added delegated `keydown` listener on `#modeCardsList` for Enter/Space with `preventDefault` on Space. Updated `switchMode` to set `aria-busy="true"` on switching card and `aria-disabled="true"` + `tabindex="-1"` on disabled cards. Updated `clearSwitchingStates` and `updateModeStatus` to properly remove ARIA attributes and restore correct tabindex values (AC #1, #2, #5, #6).
- **Task 3:** Created `focusModePickerHeading()` function exposed on `window` for ds-3.6 banner dismiss focus. Verified `finalizeModeSwitch` already correctly handles post-switch focus by moving to previously-active (now idle) card — single authoritative focus step (AC #3, #4).
- **Task 4:** Added `@media (max-width: 599px)` for single-column `.mode-cards-list` with `min-height: 44px` on cards for touch targets. Added `@media (min-width: 1024px)` for two-column CSS Grid. No existing 600px breakpoints were modified — changes are purely additive (AC #7, #8, #9).
- **Task 5:** Regenerated `firmware/data/dashboard.html.gz`, `dashboard.js.gz`, and `style.css.gz` (AC #10).
- **Pattern choice documented (AC #1):** Active card uses `tabindex="-1"` to exclude from tab order while remaining programmatically focusable for post-switch focus management.

### Change Log

- 2026-04-14: Implemented ds-3.5 accessibility and responsive layout — ARIA attributes, keyboard navigation, focus management, responsive CSS grid, gzipped assets.

### File List

- `firmware/data-src/dashboard.html` (modified)
- `firmware/data-src/dashboard.js` (modified)
- `firmware/data-src/style.css` (modified)
- `firmware/data/dashboard.html.gz` (regenerated)
- `firmware/data/dashboard.js.gz` (regenerated)
- `firmware/data/style.css.gz` (regenerated)

## Previous story intelligence

- **ds-3.4** owns **switching** / **disabled** visuals and **POST**/**GET** polling; **ds-3.5** layers **ARIA**, **tab order**, and **responsive** grid on the same DOM.

## Git intelligence summary

Expect changes under **`firmware/data-src/`** and **`firmware/data/*.gz`**.

## Project context reference

`_bmad-output/project-context.md` — gzip bundled assets after edits.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
