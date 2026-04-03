# Story 4.4: Logo List Management and Deletion

Status: done

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want to see uploaded logos with previews and delete ones I do not need,
so that I can manage my collection and free storage.

## Acceptance Criteria

1. **Logo list rendering:** Given the Logos section loads, when `GET /api/logos` returns entries, each row shows preview thumbnail canvas, ICAO filename, file size, and a delete action.

2. **Empty state copy:** Given no logos are uploaded, when the section loads, show: `"No logos uploaded yet. Drag .bin files here to add airline logos."`

3. **Inline delete confirmation:** Given user taps delete on a row, inline confirmation appears in-row (`Delete [filename]?` + Confirm/Cancel) with no modal dialog.

4. **Delete execution and refresh:** Given user confirms, when `DELETE /api/logos/[filename]` succeeds, file is removed from LittleFS, list refreshes, and toast shows `"Logo deleted"`.

5. **Storage visibility:** Given list is populated, storage usage summary appears (used bytes/limit and count), e.g. `"Storage: 847 KB / 1.9 MB used (28 logos)"`.

6. **Error specificity and contract safety:** Failed delete/list actions use specific messages (named cause + recovery path where possible) and preserve existing API envelope conventions.

## Tasks / Subtasks

- [x] Task 1: Build logo list UI states (AC: #1, #2, #5)
  - [x] Extend `firmware/data-src/dashboard.html` Logos section with list container, empty state, and storage summary area.
  - [x] Add per-row fields: thumbnail canvas, filename, file size, delete button.
  - [x] Keep single-column mobile readability and touch target sizing.

- [x] Task 2: Implement list fetch + render logic (AC: #1, #2, #5, #6)
  - [x] In `firmware/data-src/dashboard.js`, load logos via `GET /api/logos`.
  - [x] Render deterministic row order (stable sort) to avoid list flicker.
  - [x] Render empty state when list is empty.
  - [x] Compute/display storage usage text from returned metadata (or follow-up status fields if required by route contract).

- [x] Task 3: Add inline delete confirmation flow (AC: #3, #6)
  - [x] Replace row delete button with inline Confirm/Cancel controls on first tap.
  - [x] Ensure only one row is in confirm state at a time.
  - [x] Cancel returns row to normal state with no side effects.
  - [x] Avoid modal dialogs and browser `confirm()`.

- [x] Task 4: Implement delete API interaction and resilient refresh (AC: #4, #6)
  - [x] Execute `DELETE /api/logos/[filename]` for confirmed row.
  - [x] On success: show `"Logo deleted"` toast and refresh list from source of truth.
  - [x] On failure: show specific red toast (for example, `file not found`, `filesystem error`) and keep row visible.
  - [x] Keep concurrent interaction safe (disable duplicate confirm submits while request is in flight).

- [x] Task 5: Ensure backend route and LogoManager alignment (AC: #1, #4, #5, #6)
  - [x] Verify `firmware/adapters/WebPortal.cpp` list/delete handlers follow standard JSON envelope.
  - [x] Verify filename normalization/sanitization is consistent with upload and LogoManager lookup rules.
  - [x] Verify `LogoManager` counters and LittleFS usage reflect deletions immediately.
  - [x] Preserve existing `/api/logos` upload behavior from Story 4.3.

- [x] Task 6: Verification and build
  - [x] Manual checks:
    - [x] list shows thumbnails + filename + size + delete
    - [x] empty state copy displays when no files exist
    - [x] inline confirm works and no modal appears
    - [x] successful delete refreshes list and storage summary
    - [x] API failure path shows specific error
  - [x] Run `pio run` from `firmware/`.
  - [x] Regenerate modified gzipped assets under `firmware/data/` for any changed web sources.

### Review Findings

- [x] [Review][Patch] Distinguish filesystem delete failures from missing files in `DELETE /api/logos/:name` [firmware/adapters/WebPortal.cpp:674]
- [x] [Review][Patch] Return a real error when logo storage cannot be opened instead of silently treating it as an empty list [firmware/adapters/WebPortal.cpp:628]
- [x] [Review][Patch] Handle non-JSON `/api/logos` responses so list/delete failures keep specific error messages [firmware/data-src/common.js:5]

## Dev Notes

### Epic and story context

- Epic 4 stories are parallelizable, but this story builds directly on logo upload and logo storage behavior.
- This story delivers FR35 and FR37 and closes the end-user logo management loop opened by Story 4.3.

### Existing implementation surface

- Frontend:
  - `firmware/data-src/dashboard.html`
  - `firmware/data-src/dashboard.js`
  - `firmware/data-src/style.css` (row/confirm state styling)
  - `firmware/data-src/common.js` (shared toast/network helpers)
- Backend:
  - `firmware/adapters/WebPortal.cpp` (`GET /api/logos`, `DELETE /api/logos/:name`)
  - `firmware/core/LogoManager.cpp`
  - `firmware/core/LogoManager.h`
- Runtime storage:
  - logos persisted under `/logos/` in LittleFS.

### Architecture guardrails (must follow)

- Keep API response envelope consistent:
  - success: `{ "ok": true, "data": { ... } }`
  - error: `{ "ok": false, "error": "...", "code": "..." }`
- Preserve route contracts already defined in architecture (`GET /api/logos`, `DELETE /api/logos/:name`).
- Maintain streaming/file-system safety and avoid new memory-heavy behaviors.
- Keep all browser write paths through WebPortal APIs; do not add alternate direct mutation paths.

### UX guardrails (must follow)

- Use inline confirmation for destructive delete action; no modal.
- Preserve specific error text and concise success toasts.
- Keep empty state copy exactly aligned with UX spec language.
- Keep logo list interaction clear and low-friction on mobile-sized screens.

### Previous story intelligence

- From `4-3-logo-upload-with-rgb565-preview.md`:
  - preserve filename and format expectations from upload path
  - preserve API envelope and explicit error reporting patterns
  - preserve partial-success resilience mindset (do not hide successful operations because another request failed)
- From `4-1-interactive-map-with-circle-radius.md` and `4-2-display-calibration-and-test-pattern.md`:
  - keep bounded request behavior and immediate, clear UI feedback conventions.

### Git intelligence summary

- Branch history remains minimal (`first commit`), so follow current codebase patterns and artifact conventions over commit-derived style shifts.
- Repository contains active parallel Epic 4 work; integrate without reverting unrelated in-flight changes.

### Latest technical information (Context7)

- MDN (`/mdn/content`) guidance for browser list/delete interactions:
  - use `fetch()` with explicit non-OK handling and JSON response parsing for actionable errors.
  - use `FormData` for upload paths where needed, while delete/list operations remain straightforward JSON-fetch workflows.
- Existing Context7 guidance already used in this epic remains applicable:
  - ESPAsyncWebServer multipart and route handler patterns should remain unchanged for compatibility.

### Project Structure Notes

- Likely files touched:
  - `firmware/data-src/dashboard.html`
  - `firmware/data-src/dashboard.js`
  - `firmware/data-src/style.css`
  - `firmware/adapters/WebPortal.cpp`
  - `firmware/core/LogoManager.cpp`
  - `firmware/core/LogoManager.h`
  - `firmware/data/dashboard.html.gz`
  - `firmware/data/dashboard.js.gz`
  - `firmware/data/style.css.gz`

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` - Story 4.4, Epic 4]
- [Source: `_bmad-output/planning-artifacts/prd.md` - FR35, FR37]
- [Source: `_bmad-output/planning-artifacts/architecture.md` - `/api/logos` contracts, response envelope, LittleFS layout]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` - UX-DR15, UX-DR20, empty/error/confirmation patterns]
- [Source: `_bmad-output/implementation-artifacts/4-3-logo-upload-with-rgb565-preview.md`]
- [Source: `_bmad-output/implementation-artifacts/4-2-display-calibration-and-test-pattern.md`]
- [Source: `_bmad-output/implementation-artifacts/4-1-interactive-map-with-circle-radius.md`]
- [Source: Context7 `/mdn/content` queried 2026-04-03]

## Dev Agent Record

### Agent Model Used

claude-opus-4-6

### Debug Log References

- Story auto-selected from sprint backlog order: `4-4-logo-list-management-and-deletion`
- Context7 docs queried for fetch/FormData error-handling guidance: `/mdn/content`
- PlatformIO build succeeded — 56.9% flash, 16.4% RAM
- Gzipped web assets regenerated for dashboard.html, dashboard.js, style.css

### Completion Notes List

- Created implementation-ready story context for logo list rendering, storage visibility, and inline deletion flow.
- Added concrete anti-regression guardrails for API contract consistency and UX confirmation patterns.
- Included file-level implementation map and verification checklist.
- **Implementation complete (2026-04-03):**
  - Added logo list container, empty state, and storage summary HTML to Logos card in dashboard.html
  - Implemented `loadLogoList()` in dashboard.js: fetches `GET /api/logos`, renders sorted rows with thumbnail preview, filename, size, and delete action
  - RGB565 thumbnail decoding fetches raw binary from new `GET /logos/:name` route and renders 48x48 preview canvas
  - Inline delete confirmation: single-row confirm state with Confirm/Cancel, no modal, cancel restores normal state
  - Delete via `DELETE /api/logos/[filename]` with concurrency guard (`logoDeleteInFlight`), specific error toasts, and full list refresh on success
  - Extended `GET /api/logos` backend response with `storage` object: `{ used, total, logo_count }`
  - Added `GET /logos/*` route to WebPortal for serving raw logo binaries for client-side thumbnail rendering
  - All CSS for list rows, thumbnails, inline confirm, storage summary, mobile-friendly sizing
  - Logo list auto-refreshes after successful upload
  - Empty state shows exact UX spec copy

### Implementation Plan

1. HTML: Extended Logos card with `logo-storage-summary`, `logo-empty-state`, and `logo-list` container
2. CSS: Added `.logo-list-*` classes for row layout, thumbnails, inline confirm, cancel/confirm buttons
3. JS: Added `loadLogoList()`, `renderLogoListRow()`, `loadLogoThumbnail()`, `showInlineConfirm()`, `resetRowActions()`, `executeDelete()`, `formatBytes()`
4. Backend: Extended `_handleGetLogos` with storage metadata, added `_handleGetLogoFile` route and handler
5. Wired `loadLogoList()` into page init and post-upload refresh

### File List

- _bmad-output/implementation-artifacts/4-4-logo-list-management-and-deletion.md (modified)
- firmware/data-src/dashboard.html (modified)
- firmware/data-src/dashboard.js (modified)
- firmware/data-src/style.css (modified)
- firmware/adapters/WebPortal.cpp (modified)
- firmware/adapters/WebPortal.h (modified)
- firmware/data/dashboard.html.gz (regenerated)
- firmware/data/dashboard.js.gz (regenerated)
- firmware/data/style.css.gz (regenerated)

## Change Log

- 2026-04-03: Implemented Story 4.4 — Logo list management with thumbnail previews, storage summary, inline delete confirmation, and resilient delete API interaction. Extended GET /api/logos with storage metadata. Added GET /logos/* route for thumbnail serving.
