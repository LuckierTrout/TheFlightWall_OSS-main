
# Story LE-1.5: Logo Widget RGB565 Pipeline

branch: le-1-5-logo-widget-rgb565-pipeline
zone:
  - firmware/widgets/LogoWidget.*
  - firmware/core/LogoManager.*
  - firmware/adapters/WebPortal.cpp
  - firmware/test/test_logo_widget/**

Status: draft

## Story

As a **layout author**,
I want **`logo` widgets to render real RGB565 bitmaps uploaded via `/api/logos`**,
So that **I can place airline, airport, or custom logos on my layout**.

## Acceptance Criteria

1. **Given** a `logo` widget spec with non-empty `logo_id` **When** dispatched **Then** `LogoWidget` loads the RGB565 bitmap from `/logos/<logo_id>.bin` via `LogoManager` **And** blits it via `DisplayUtils::drawBitmapRGB565` — **not** `fillRect`.
2. **Given** a `logo_id` that does not resolve to a file **When** dispatched **Then** the widget renders a solid-color fallback block (color from `spec.color`) — no crash, no blank frame.
3. **Given** a multipart upload **When** `POST /api/logos` is called with a RGB565 `.bin` body + metadata (`id`, `w`, `h`) **Then** the file is stored at `/logos/<id>.bin` on LittleFS **And** returns `{ok: true, data: {id, bytes}}`.
4. **Given** a logo id **When** `DELETE /api/logos/:id` is called **Then** the bin file is removed **And** any layout still referencing it renders the solid-color fallback on next render without crashing.
5. **Given** a global logo quota of 256 KB **When** an upload would exceed the quota **Then** the request is rejected with `{ok: false, code: "LOGO_QUOTA"}`.
6. **Given** a widget render **When** inspected **Then** no per-frame heap allocation occurs (bitmap buffer is held by `LogoManager`'s cached map or re-read from flash with a stack-local staging buffer).
7. **Given** unit tests **When** `pio test -f test_logo_widget` runs **Then** cases pass for: blit correctness against golden RGB565 fixture, fallback on missing logo, quota rejection at upload boundary.

## Tasks / Subtasks

- [ ] **Task 1: Replace V1 stub LogoWidget with real loader** (AC: #1, #2, #6)
  - [ ] `firmware/widgets/LogoWidget.cpp` — remove fixed-color 16×16 stub path
  - [ ] Call `LogoManager::getBitmap(logoId, outPtr, outW, outH)`; on success use `DisplayUtils::drawBitmapRGB565`
  - [ ] On failure: `DisplayUtils::drawBitmapRGB565` of a stack-constructed single-color buffer **or** call existing solid-color primitive that internally uses the bitmap path (never `fillRect`)
  - [ ] Extend `WidgetSpec` to carry `char logo_id[16]`

- [ ] **Task 2: Extend LogoManager** (AC: #1, #3, #4, #5, #6)
  - [ ] `firmware/core/LogoManager.h` / `.cpp` — add `bool getBitmap(const char* id, const uint16_t** out, uint8_t* w, uint8_t* h)` returning a pointer to a cached or freshly-staged buffer
  - [ ] Add `bool save(const char* id, const uint8_t* data, size_t bytes, uint8_t w, uint8_t h)` honoring 256 KB global quota
  - [ ] Add `bool remove(const char* id)` and `size_t totalBytes()`
  - [ ] Reuse Story 4-3 logo upload plumbing if present; otherwise add the routes here

- [ ] **Task 3: WebPortal logo CRUD routes** (AC: #3, #4, #5)
  - [ ] `firmware/adapters/WebPortal.cpp` — `POST /api/logos` (multipart), `DELETE /api/logos/:id`, `GET /api/logos` (list metadata)
  - [ ] Enforce quota: compute `LogoManager::totalBytes() + incoming` before persisting; reject with `LOGO_QUOTA`
  - [ ] Validate `w * h * 2 == bytes` (RGB565 is 2 bytes/pixel) and `w, h <= 32`

- [ ] **Task 4: Safe reference chain on deletion** (AC: #4)
  - [ ] Confirm widgets that reference a deleted logo hit the fallback path (Task 1)
  - [ ] No layout rewrite required on delete; `LogoWidget` handles missing file gracefully

- [ ] **Task 5: Unit tests** (AC: #7)
  - [ ] `firmware/test/test_logo_widget/test_main.cpp` — golden RGB565 fixture (small 8×8) blit; compare framebuffer bytes
  - [ ] test: missing logo_id → fallback solid color, no crash
  - [ ] test: upload at 256 KB boundary rejected with `LOGO_QUOTA`
  - [ ] test: dimension/bytes mismatch rejected

- [ ] **Task 6: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] `pio test -f test_logo_widget --without-uploading --without-testing` passes
  - [ ] Grep: no `fillRect` in the logo render path

## Dev Notes

**Validated spike finding** (`_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`): `fillRect(32x32)` measured 2.67× baseline; `drawBitmapRGB565` of a similarly sized pattern dropped to 1.23×. **All logo render paths — including the missing-logo fallback — must use `drawBitmapRGB565`.**

**Depends on:** LE-1.2 (LogoWidget V1 stub registered in WidgetRegistry), LE-1.4 (`GET /api/widgets/types` should advertise the `logo_id` field).

**Reuse existing pipeline.** `firmware/core/LogoManager.{h,cpp}` and the Story 4-3 logo upload path already handle RGB565 files. Extend — do not fork.

**Quota math.** 256 KB / (16×16×2) ≈ 512 logos max at the smallest size; more realistically ~128 at typical mixed sizes. Enforce at upload.

**No per-frame alloc.** `LogoManager` should either keep a small LRU cache of recently-used logos in RAM, or read from LittleFS into a stack-local staging buffer on widget init only. Document the chosen approach in `LogoManager.h`.

## File List

- `firmware/widgets/LogoWidget.cpp` (modified — replace stub)
- `firmware/widgets/LogoWidget.h` (modified — spec extension)
- `firmware/core/LogoManager.h` (modified)
- `firmware/core/LogoManager.cpp` (modified)
- `firmware/adapters/WebPortal.cpp` (modified — logo CRUD routes)
- `firmware/test/test_logo_widget/test_main.cpp` (new)
