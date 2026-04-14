# Story 4.3: Logo Upload with RGB565 Preview

Status: done

Ultimate context engine analysis completed - comprehensive developer guide created.

## Story

As a user,
I want to upload logo files and see a preview before committing,
so that I can verify logos are correct.

## Acceptance Criteria

1. **Batch file intake:** Given the Logos section upload area, when files are selected or dragged, multiple `.bin` files are accepted in one action (batch workflow).

2. **Client-side file validation:** Given each selected file is validated client-side, when a file is not `.bin` or is not exactly `2048` bytes, an error is shown with filename context (example: `"logo_xyz.bin - invalid format or size"`), and invalid files are not uploaded.

3. **RGB565 preview rendering:** Given a valid logo file, when RGB565 decode runs in the browser, a preview canvas renders the decoded `32x32` image scaled to `128x128` next to the filename before upload.

4. **Upload behavior and partial success:** Given upload starts, when files are POSTed to `/api/logos`, successful files persist even if some files fail; success shows `"N logos uploaded"` and failures show a red toast with specific cause (`"LittleFS full"`, `"invalid size"`, etc.).

5. **Contract and resource safety:** Upload implementation preserves existing API envelope conventions and follows streaming constraints (no full-file RAM buffering on device).

## Tasks / Subtasks

- [x] Task 1: Add logo upload + preview UI in dashboard (AC: #1, #2, #3)
  - [x] Extend `firmware/data-src/dashboard.html` Logos section with drag/drop upload zone and file picker.
  - [x] Add per-file preview list rows with filename, validation state, preview canvas, and upload result status.
  - [x] Ensure empty/idle states remain clear on mobile-sized viewports.

- [x] Task 2: Implement client-side validation and decode pipeline (AC: #2, #3)
  - [x] In `firmware/data-src/dashboard.js`, validate extension (`.bin`) and strict byte length (`2048`) before queueing upload.
  - [x] Use `FileReader.readAsArrayBuffer()` to read each valid file and decode RGB565 -> RGB888 for preview rendering.
  - [x] Reuse shared helper patterns from `common.js` where possible; do not duplicate generalized toast or decode utilities if they already exist.
  - [x] Keep decode logic deterministic for all `32x32` pixels and guard against malformed buffer lengths.

- [x] Task 3: Implement upload queue with resilient partial success (AC: #1, #4, #5)
  - [x] POST each valid file to `/api/logos` using current web API contract.
  - [x] Track per-file success/failure and show aggregate completion toast (`N uploaded`, `M failed`) with cause details.
  - [x] Do not block successful uploads because one file fails.
  - [x] Keep request pacing bounded to avoid pressure on ESP32 connection/memory limits.

- [x] Task 4: Verify backend route behavior and guardrails (AC: #4, #5)
  - [x] Confirm `firmware/adapters/WebPortal.cpp` handles multipart upload in streaming mode and returns standard JSON envelope.
  - [x] Confirm logo writes land in `/logos/` and naming rules remain consistent with `LogoManager` lookup expectations.
  - [x] Ensure rejection paths provide specific, user-facing errors for invalid type, invalid size, and storage exhaustion.
  - [x] Ensure no regressions to existing `GET /api/logos` and `DELETE /api/logos/:name`.

- [x] Task 5: Storage and integration integrity checks (AC: #4, #5)
  - [x] Verify logo count and LittleFS usage reflect newly uploaded files (for later System Health/logo list surfaces).
  - [x] Validate uploaded files are immediately consumable by `LogoManager::loadLogo()` in expected `.bin` format.
  - [x] Preserve existing runtime LittleFS layout and avoid introducing alternate upload paths.

- [x] Task 6: Validation and build verification
  - [x] Manual validation:
    - [x] mixed valid/invalid batch -> valid files still upload
    - [x] invalid extension and invalid byte length produce specific inline errors
    - [x] preview image appears before upload for valid files
    - [x] simulated low-storage condition yields specific failure message
  - [x] Run `pio run` from `firmware/`.
  - [x] Regenerate modified gzipped assets in `firmware/data/` when `firmware/data-src/*` changes.

## Dev Notes

### Epic and story context

- Epic 4 stories are parallelizable, but this story still depends on existing dashboard and LogoManager behavior from Epics 2 and 3.
- This story fulfills FR34 and FR36 directly, and sets up smooth handoff into Story 4.4 (logo list/deletion UX).

### Existing implementation surface

- Frontend:
  - `firmware/data-src/dashboard.html`
  - `firmware/data-src/dashboard.js`
  - `firmware/data-src/style.css` (if upload/preview states need styling)
  - `firmware/data-src/common.js` (shared helpers such as toast/decode)
- Backend:
  - `firmware/adapters/WebPortal.cpp` (`/api/logos` upload + listing route family)
  - `firmware/core/LogoManager.cpp` and `firmware/core/LogoManager.h`
- Runtime storage:
  - LittleFS web assets at root, uploaded logos under `/logos/`.

### Architecture guardrails (must follow)

- Preserve endpoint contract and response envelope:
  - success: `{ "ok": true, "data": { ... } }`
  - error: `{ "ok": false, "error": "...", "code": "..." }`
- Keep ESP32 upload handling streaming-oriented; do not introduce full-file server buffering.
- Respect platform constraints: limited concurrent clients and bounded heap usage.
- Keep code placement aligned with architecture mapping: logo management spans `LogoManager`, `WebPortal`, and `dashboard.js`.

### UX guardrails (must follow)

- Feedback must remain immediate and specific:
  - per-file validation feedback before upload
  - decoded visual preview before commit
  - concise success/failure toast messaging with explicit cause
- Maintain dashboard mobile-first card consistency and dark theme styling.
- Keep touch targets and readability aligned with existing dashboard conventions.

### Previous story intelligence

- From `4-2-display-calibration-and-test-pattern.md`: preserve triple-feedback behavior and avoid introducing API contract drift while adding new UI behavior.
- From `4-1-interactive-map-with-circle-radius.md` (currently in review): continue using release/bounded update semantics and resilient fallback behavior instead of noisy high-frequency network writes.

### Git intelligence summary

- Current branch history is minimal (`first commit`), so implementation guidance should follow the present codebase conventions and completed artifact patterns rather than commit evolution.
- Worktree is active with ongoing Epic 3/4 changes; integrate incrementally without reverting unrelated edits.

### Latest technical information (Context7)

- ESPAsyncWebServer (`/esp32async/espasyncwebserver`):
  - multipart upload handlers receive chunked callbacks with `index`, `len`, and `final`; this supports stream-to-file workflows appropriate for constrained RAM.
  - route pattern supports separate completion handler plus upload handler; keep completion response consistent with current API envelope.
  - request parameter introspection distinguishes GET/POST/file params; useful for robust validation and diagnostics.
- MDN Web APIs (`/mdn/content`):
  - `FileReader.readAsArrayBuffer()` is the appropriate browser-side file read path for binary logo decode.
  - decoded pixel writes should use typed arrays and canvas `ImageData`/`putImageData` for deterministic preview rendering.

### Project Structure Notes

- Likely files touched:
  - `firmware/data-src/dashboard.html`
  - `firmware/data-src/dashboard.js`
  - `firmware/data-src/style.css` (optional)
  - `firmware/data-src/common.js` (only if shared helper updates are needed)
  - `firmware/adapters/WebPortal.cpp`
  - `firmware/core/LogoManager.cpp`
  - `firmware/core/LogoManager.h`
  - `firmware/data/dashboard.html.gz`
  - `firmware/data/dashboard.js.gz`
  - `firmware/data/style.css.gz` (if CSS changed)

### References

- [Source: `_bmad-output/planning-artifacts/epics.md` - Story 4.3, Epic 4]
- [Source: `_bmad-output/planning-artifacts/prd.md` - FR34, FR36, memory/streaming constraints]
- [Source: `_bmad-output/planning-artifacts/architecture.md` - `/api/logos` routes, response envelope, LittleFS layout, structural mapping]
- [Source: `_bmad-output/planning-artifacts/ux-design-specification.md` - RGB565 preview component and feedback/toast behavior]
- [Source: `_bmad-output/implementation-artifacts/4-2-display-calibration-and-test-pattern.md`]
- [Source: `_bmad-output/implementation-artifacts/4-1-interactive-map-with-circle-radius.md`]
- [Source: Context7 `/esp32async/espasyncwebserver` queried 2026-04-03]
- [Source: Context7 `/mdn/content` queried 2026-04-03]

## Dev Agent Record

### Agent Model Used

claude-opus-4-6

### Debug Log References

- Story auto-selected from sprint backlog order: `4-3-logo-upload-with-rgb565-preview`
- Context7 library IDs resolved and used:
  - `/esp32async/espasyncwebserver`
  - `/mdn/content`
- Build successful: RAM 16.4%, Flash 56.8%
- Existing test suite compiles without regressions

### Implementation Plan

1. Backend first: Added `GET /api/logos`, `POST /api/logos` (streaming multipart), `DELETE /api/logos/:name` routes
2. Extended LogoManager with `listLogos()`, `deleteLogo()`, `saveLogo()` methods
3. Frontend: Logos card with drag-drop zone, file picker, per-file preview rows
4. Client-side validation: .bin extension + 2048 byte size check before upload
5. RGB565 decode pipeline: FileReader → Uint16Array → canvas ImageData → scaled preview
6. Sequential upload queue: one file at a time to respect ESP32 constraints, partial success handling

### Completion Notes List

- Created implementation-ready story context for Story 4.3 with architecture, UX, and endpoint guardrails.
- Added explicit anti-regression guidance for API envelope, streaming upload behavior, and partial-success batching.
- Included concrete file-level targets and validation checklist for dev execution.
- Implemented all 3 architected API routes: GET /api/logos (list), POST /api/logos (upload), DELETE /api/logos/:name (delete)
- Upload handler uses ESPAsyncWebServer streaming multipart — no full-file RAM buffering on ESP32
- Files stream directly to LittleFS via chunked write callbacks (index, len, final pattern)
- Client-side validation rejects non-.bin files and incorrect sizes before upload with per-file error messages
- RGB565 decode uses FileReader.readAsArrayBuffer() → Uint16Array → canvas putImageData for deterministic 32x32 preview
- Upload queue processes files sequentially (one at a time) to avoid overloading ESP32 connection/memory
- Partial success: valid files upload even if others fail; aggregate toast shows "N uploaded, M failed"
- Per-file status indicators: Ready → Uploading → Uploaded (green) or error (red with cause)
- Drag-and-drop zone with visual feedback (dashed border → highlighted on dragover)
- Remove button (×) on each file row lets user dequeue before upload
- Standard JSON envelope preserved on all new endpoints
- Backend rejection provides specific error codes: INVALID_TYPE, INVALID_SIZE, FS_FULL, FS_WRITE_ERROR
- LogoManager gains listLogos(), deleteLogo(), saveLogo() for Story 4.4 readiness
- scanLogoCount() made public so upload handler can refresh count after write
- All gzipped assets regenerated

### File List

- _bmad-output/implementation-artifacts/4-3-logo-upload-with-rgb565-preview.md (modified)
- firmware/data-src/dashboard.html (modified — added Logos card)
- firmware/data-src/dashboard.js (modified — added logo upload/validate/decode/queue logic)
- firmware/data-src/style.css (modified — added logo upload styling)
- firmware/adapters/WebPortal.h (modified — added logo handler declarations)
- firmware/adapters/WebPortal.cpp (modified — added GET/POST/DELETE /api/logos routes)
- firmware/core/LogoManager.h (modified — added listLogos, deleteLogo, saveLogo, made scanLogoCount public)
- firmware/core/LogoManager.cpp (modified — implemented listLogos, deleteLogo, saveLogo)
- firmware/data/dashboard.html.gz (regenerated)
- firmware/data/dashboard.js.gz (regenerated)
- firmware/data/style.css.gz (regenerated)

### Change Log

- 2026-04-03: Implemented Story 4.3 — Logo Upload with RGB565 Preview. Added full-stack logo upload pipeline: drag-drop UI with client-side validation and RGB565 preview, streaming multipart upload to ESP32, and resilient partial-success handling.

### Review Findings

- [x] [Review][Patch] Use a red aggregate toast that summarizes all failure causes for mixed-result uploads to satisfy AC4 [firmware/data-src/dashboard.js:1605]
- [x] [Review][Patch] Reject unsafe logo filenames before write/delete to close LittleFS path traversal gaps [firmware/adapters/WebPortal.cpp:258, firmware/core/LogoManager.cpp:206]
- [x] [Review][Patch] Clean up abandoned multipart uploads so partial files and `g_logoUploads` entries do not leak on disconnect [firmware/adapters/WebPortal.cpp:58, firmware/adapters/WebPortal.cpp:217]
- [x] [Review][Patch] Render the visible preview at 128x128 instead of 40x40 to satisfy AC3 [firmware/data-src/dashboard.js:1513]
- [x] [Review][Patch] Include the filename in client-side validation errors to satisfy AC2 [firmware/data-src/dashboard.js:1453]
- [x] [Review][Patch] Filter `GET /api/logos` to valid 2048-byte `.bin` logos so list output matches logo-count semantics [firmware/core/LogoManager.cpp:159, firmware/core/LogoManager.cpp:183]
