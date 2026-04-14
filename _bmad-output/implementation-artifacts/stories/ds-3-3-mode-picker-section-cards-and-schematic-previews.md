# Story ds-3.3: Mode Picker Section — Cards & Schematic Previews

Status: review

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

As a FlightWall user,
I want to see all available display modes in the web dashboard with schematic previews of each mode's zone layout,
So that I can understand what each mode looks like before switching.

## Acceptance Criteria

1. **Given** the dashboard loads and the Mode Picker section exists, **When** the page initializes, **Then** JavaScript calls **`GET /api/display/modes`**, reads **`res.body.data`** (envelope **`{ ok, data }`** — not top-level **`modes`**), and builds mode cards from **`data.modes`** dynamically (**UX-DR12**).

2. **Given** the epic placeholder contract, **When** the Mode Picker markup is finalized, **Then** the section includes a root element **`id="mode-picker"`** (may be the existing **`#mode-picker-card`** renamed, or an inner wrapper) with **minimal static HTML** (~10 lines): heading, optional status host, and an **empty** container for injected cards (e.g. **`#modeCardsList`**). **Do not** hardcode mode names in HTML.

3. **Given** each mode object from the API includes **`zone_layout`** (array of **`{ label, relX, relY, relW, relH }`** — **`uint8` 0–100** semantics per **`ZoneRegion`** in **`DisplayMode.h`**), **When** a card is rendered, **Then** it shows **mode name** and a **CSS Grid** schematic:
   - Container **height 80px**, **`display: grid`**, **`grid-template-columns: repeat(100, minmax(0, 1fr))`**, **`grid-template-rows: repeat(100, minmax(0, 1fr))`** (or equivalent **100×100** logical grid so integers map 1:1).
   - Each zone is a **child `div`** with **`grid-column`** / **`grid-row`** derived from **`relX, relY, relW, relH`** using **1-based** grid lines: e.g. **`grid-column: (relX + 1) / span relW`** only if **`relW > 0`**; clamp spans so lines stay within **1..101**. Prefer **`grid-column-start` / `grid-column-end`** if clearer for your layout math.
   - Zone cells show **visible label text** (abbreviate long **`label`** strings for the 80px-tall mini preview if needed).
   - Zone cells: **`aria-hidden="true"`** (decorative preview; **UX-DR8** partial).
   - Schematic wrapper: **`aria-label`** describing the layout (e.g. **"Schematic preview of zone layout for {mode name}"**).

4. **Given** **`data.active`** matches a mode **`id`**, **When** cards render, **Then** the active card has:
   - **`aria-current="true"`** (**UX-DR7**),
   - **4px** solid **left** accent border using **`var(--primary)`** (distinct from idle),
   - a **status dot** (small circular indicator) and visible **"Active"** label (**UX-DR1** active).

5. **Given** a non-active mode, **When** its card renders, **Then** it uses **1px** border with a **dimmed** accent (**UX-DR1** idle): add **`--accent-dim`** (or reuse **`var(--border)`** with a documented token) in **`:root`** if no suitable token exists; card remains **`cursor: pointer`** / tappable.

6. **Given** **UX-DR4**, **When** the user selects a mode, **Then** the **entire card** is the interaction surface — **no** inner **`<button>`** for “Switch”. (Click handler may remain on the card **`div`**; keyboard **`role="tabindex"`** is **ds-3.5** unless you add it early without conflicting with **ds-3.4**.)

7. **Given** **FR24**, **When** the user views the dashboard header navigation, **Then** a link to the Mode Picker is **always** available (e.g. **`href="#mode-picker"`** next to System Health) so the section is discoverable without scrolling the full card stack.

8. **Given** **NFR C4**, **When** styling the picker, **Then** reuse **dark theme** and existing **CSS custom properties** from **`style.css`** (`--bg`, `--surface`, `--border`, `--text`, `--primary`, …).

9. **Given** **NFR P3** (Mode Picker “loads” within **~1s** of page load), **When** measuring in dev tools, **Then** first successful paint of mode cards (including schematic) occurs **≤ 1000ms** after **`DOMContentLoaded`** on a warm LAN connection to the device. If **`loadSettings()`** contends for the ESP32 HTTP stack, **start `loadDisplayModes()` in parallel** with settings (already parallel today — **preserve or improve**, do not serialize behind a slow path).

10. **Given** **`POST /api/display/mode`** (**ds-3.1**), **When** this story touches the switch request, **Then** prefer body **`{ "mode": "<id>" }`**; keep **`mode_id`** only if needed for backward compatibility during transition (**firmware accepts both**).

11. **Given** bundled portal workflow, **When** **`dashboard.html`**, **`dashboard.js`**, or **`style.css`** under **`firmware/data-src/`** change, **Then** regenerate the matching **`.gz`** under **`firmware/data/`** per **`AGENTS.md`**.

12. **Given** **`pio run`** (or repo CI equivalent), **Then** no new build warnings from touched artifacts.

## Tasks / Subtasks

- [x] Task 1: Markup — **`id="mode-picker"`** + nav (AC: #2, #7)
  - [x] 1.1: Add **`#mode-picker`** to **`dashboard.html`**; keep **`#modeCardsList`** as injection target
  - [x] 1.2: Extend **`<nav>`** in **`dash-header`** with **"Display Modes"** → **`#mode-picker`**

- [x] Task 2: Schematic renderer (AC: #1, #3, #9)
  - [x] 2.1: In **`renderModeCards`**, after name row, append schematic **`div`** built from **`mode.zone_layout`**
  - [x] 2.2: Handle **missing** or **empty** **`zone_layout`** gracefully (empty schematic + **`aria-label`** still valid)

- [x] Task 3: Card chrome — active / idle (AC: #4, #5, #6)
  - [x] 3.1: Update **`style.css`** — **`.mode-card.active`**, **`.mode-card`**, dot + **"Active"** row
  - [x] 3.2: Set **`aria-current`** only on active card; remove from others on re-render

- [x] Task 4: POST body + gzip (AC: #10, #11, #12)
  - [x] 4.1: **`switchMode`**: send **`mode`** key
  - [x] 4.2: Regenerate **`data/dashboard.js.gz`**, **`data/dashboard.html.gz`**, **`data/style.css.gz`** as applicable

## Dev Notes

### Current implementation (delta from epic)

- **`dashboard.html`**: section **`#mode-picker-card`** with **`#modeCardsList`** — **no** schematic, **no** **`#mode-picker`** id, **no** header nav link to modes.
- **`dashboard.js`**: **`renderModeCards`**, **`loadDisplayModes`**, **`switchMode`** — basic list; **`switchMode`** uses **`mode_id`** only; switching animation is **partial** (full **UX-DR1** switching/disabled/error is **ds-3.4**).
- **`style.css`**: **`.mode-card`** / **`.active`** — uniform border, **no** 4px left rail, **no** status dot, **no** schematic styles.

### API contract (do not guess)

- Success: **`res.body.ok === true`**, payload in **`res.body.data`**
- Fields: **`data.modes[]`**, **`data.active`**, optional **`data.description`** on mode objects from firmware if populated
- Zone keys: **`relX`, `relY`, `relW`, `relH`** are **percentage 0–100** of logical display [Source: `firmware/interfaces/DisplayMode.h`]

### Out of scope (later stories)

- **ds-3.4** — pulsing border, sibling **disabled** state, scoped card **error** copy, **`prefers-reduced-motion`**, focus choreography after switch
- **ds-3.5** — **`role="button"`**, **`tabindex`**, full keyboard model
- **ds-3.6** — upgrade **banner** (may scroll to **`#mode-picker`** — stable id matters)

### Dependencies

- **ds-3.1** — **`GET /api/display/modes`** with **`zone_layout`** from **`ModeRegistry`**
- **dl-1.5** — status line **`orchestrator_state` / `state_reason`**; keep **`updateModeStatus`** working after DOM changes

### References

- Epic: `_bmad-output/planning-artifacts/epics/epic-ds-3.md` (Story ds-3.3)
- Prior API story: `_bmad-output/implementation-artifacts/stories/ds-3-1-display-mode-api-endpoints.md`
- Prior persistence story: `_bmad-output/implementation-artifacts/stories/ds-3-2-nvs-mode-persistence-and-boot-restore.md`
- Sources: `firmware/data-src/dashboard.html`, `dashboard.js`, `style.css`

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build: `pio run` — SUCCESS, 0 warnings, Flash 78.8%, RAM 16.7%
- Test build: `pio test --without-uploading --without-testing` — all 9 test suites compile OK (no device for runtime)

### Completion Notes List

- **Task 1:** Renamed `#mode-picker-card` → `#mode-picker` section id. Added "Display Modes" nav link in `dash-header` alongside "System Health".
- **Task 2:** Added `buildSchematic()` function that creates a 100×100 CSS Grid from `zone_layout` array. Labels are truncated to 7 chars + ellipsis for the 80px preview. Missing/empty `zone_layout` returns empty wrapper with valid `aria-label`. Zone cells have `aria-hidden="true"`.
- **Task 3:** Active card now has 4px solid left border (`var(--primary)`), green status dot + "Active" text, `aria-current="true"`. Idle cards use `var(--border)` for left border (dimmed). `updateModeStatus` manages `aria-current` on status polling. Card header restructured with flex row for name + active indicator.
- **Task 4:** `switchMode` POST body changed from `{ mode_id }` to `{ mode }`. All 3 gzip files regenerated.

### Change Log

- 2026-04-14: Implemented story ds-3.3 — mode picker cards with schematic previews, active/idle chrome, nav link, POST body update

### File List

- `firmware/data-src/dashboard.html` (modified)
- `firmware/data-src/dashboard.js` (modified)
- `firmware/data-src/style.css` (modified)
- `firmware/data/dashboard.html.gz` (regenerated)
- `firmware/data/dashboard.js.gz` (regenerated)
- `firmware/data/style.css.gz` (regenerated)

## Previous story intelligence

- **ds-3.2** standardizes **`disp_mode`** NVS and boot restore; picker is **read-only** of **`data.active`** except where **ds-3.4** extends **`POST`** UX.
- **ds-3.1** documents **`data.*`** envelope and **`zone_layout`** shape — UI **must** consume that shape.

## Git intelligence summary

Expect diffs under **`firmware/data-src/`** and matching **`firmware/data/*.gz`**.

## Project context reference

`_bmad-output/project-context.md` — regenerate **gzip** after **`data-src`** edits.

## Story completion status

Ultimate context engine analysis completed — comprehensive developer guide created.
