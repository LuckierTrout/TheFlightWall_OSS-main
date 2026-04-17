
# Story LE-1.4: Layouts REST API

branch: le-1-4-layouts-rest-api
zone:
  - firmware/adapters/WebPortal.*
  - firmware/test/test_web_portal/**
  - _bmad-output/planning-artifacts/api-layouts-spec.md

Status: draft

## Story

As **the dashboard editor**,
I want **REST endpoints for layout CRUD and activation**,
So that **the browser can list/save/activate layouts with the standard envelope response**.

## Acceptance Criteria

1. **Given** stored layouts **When** `GET /api/layouts` is called **Then** it returns `{ok: true, data: [{id, name, updated, bytes}, ...]}` with one entry per layout on LittleFS.
2. **Given** a valid id **When** `GET /api/layouts/:id` is called **Then** the full layout JSON is returned as `data`; missing id returns `{ok: false, error: "Layout not found", code: "LAYOUT_NOT_FOUND"}`.
3. **Given** a layout body in the request **When** `POST /api/layouts` is called **Then** a new layout is created via `LayoutStore::save`, returning `{ok: true, data: {id}}`; bodies >8 KB return `LAYOUT_TOO_LARGE`, invalid schema returns `LAYOUT_INVALID`.
4. **Given** an existing id **When** `PUT /api/layouts/:id` is called **Then** the layout is overwritten **And** returns `{ok: true, data: {bytes}}` reflecting the new byte count.
5. **Given** a non-active layout id **When** `DELETE /api/layouts/:id` is called **Then** the file is removed and `{ok: true}` is returned; **when** the id is the currently active layout **Then** the call fails with `{ok: false, error, code: "LAYOUT_ACTIVE"}`.
6. **Given** an id **When** `POST /api/layouts/:id/activate` is called **Then** `layout_active` is written to NVS, a mode switch to `custom_layout` is requested via ModeOrchestrator **And** `{ok: true, data: {}}` is returned.
7. **Given** the editor needs widget type introspection **When** `GET /api/widgets/types` is called **Then** it returns `{ok: true, data: [{type, fields: [{key, kind, range}]}]}` for every widget type registered in `WidgetRegistry`.
8. **Given** filesystem capacity is exhausted **When** any write endpoint (`POST`/`PUT`) is called **Then** the response is `{ok: false, error, code: "FS_FULL"}` **And** no partial file is left on LittleFS.
9. **Given** integration tests **When** `pio test -f test_web_portal` runs (compile-only acceptable if no hardware) **Then** every endpoint path is exercised against a running LittleFS harness.

## Tasks / Subtasks

- [ ] **Task 1: Register routes in WebPortal** (AC: #1–#7)
  - [ ] `firmware/adapters/WebPortal.cpp` — add 7 AsyncWebServer routes: GET /api/layouts, GET /api/layouts/:id, POST /api/layouts, PUT /api/layouts/:id, DELETE /api/layouts/:id, POST /api/layouts/:id/activate, GET /api/widgets/types
  - [ ] Route each handler through a new `LayoutsRoutes` section; use existing envelope helpers (`sendOkJson` / `sendErrorJson`)

- [ ] **Task 2: Implement GET list + GET id** (AC: #1, #2)
  - [ ] Call `LayoutStore::list()` → serialize `{id, name, updated, bytes}` array
  - [ ] `GET /api/layouts/:id` → `LayoutStore::load(id, buf)` → return raw JSON; 404 with `LAYOUT_NOT_FOUND`

- [ ] **Task 3: Implement POST + PUT** (AC: #3, #4, #8)
  - [ ] Stream body into bounded buffer (max 8 KB); reject >8 KB with `LAYOUT_TOO_LARGE`
  - [ ] Call `LayoutStore::save(id, json)`; map errors → envelope codes `LAYOUT_INVALID`, `FS_FULL`
  - [ ] PUT requires pre-existing id; 404 if missing

- [ ] **Task 4: Implement DELETE** (AC: #5)
  - [ ] Compare target id against `LayoutStore::activeId()`; if equal, reject with `LAYOUT_ACTIVE`
  - [ ] Otherwise `LayoutStore::remove(id)`; 404 if missing

- [ ] **Task 5: Implement activate** (AC: #6)
  - [ ] `LayoutStore::setActiveId(id)` → success
  - [ ] Signal ModeOrchestrator to switch to `custom_layout` (`ModeOrchestrator::requestSwitch("custom_layout")` or equivalent atomic flag)

- [ ] **Task 6: Implement widget types introspection** (AC: #7)
  - [ ] `WidgetRegistry::describeTypes(JsonArray&)` — static schema for each widget type including field name, kind (enum string, int, color, select), and range/allowed values
  - [ ] Serialize and return

- [ ] **Task 7: Error envelope codes** (AC: #8)
  - [ ] Define constants in a shared header or in WebPortal.cpp: `LAYOUT_NOT_FOUND`, `LAYOUT_TOO_LARGE`, `LAYOUT_INVALID`, `LAYOUT_ACTIVE`, `FS_FULL`

- [ ] **Task 8: API spec doc** (AC: all)
  - [ ] `_bmad-output/planning-artifacts/api-layouts-spec.md` — document each route, method, request body, response envelope, error codes

- [ ] **Task 9: Integration tests** (AC: #9)
  - [ ] Extend `firmware/test/test_web_portal/` — hit each route, assert envelope shape and status code
  - [ ] Compile-only check acceptable: `pio test -f test_web_portal --without-uploading --without-testing`

- [ ] **Task 10: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] All tests pass (compile-only acceptable)
  - [ ] Optional: run `tests/smoke/test_web_portal_smoke.py` extension against live device

## Dev Notes

**Envelope convention** (from project CLAUDE.md):
```json
{ "ok": true, "data": { ... }, "error": "message", "code": "ERROR_CODE" }
```

**Depends on:** LE-1.1 (LayoutStore), LE-1.2 (WidgetRegistry introspection), LE-1.3 (ModeOrchestrator switch path).

**Existing patterns to follow:** `firmware/adapters/WebPortal.cpp` already registers settings, status, OTA, logo, and schedule routes. Use the same `AsyncWebServerRequest*` patterns, `StreamingJsonResponse` helpers, and envelope functions.

**Mode-switch request pattern.** See how existing endpoints trigger ModeOrchestrator (e.g., `POST /api/display/mode`) — reuse that signal mechanism; do not mutate display state directly from WebPortal.

**V0 spike context** (`_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`) confirmed the LittleFS-for-layouts + NVS-for-active-id split. This story exposes that store over HTTP.

## File List

- `firmware/adapters/WebPortal.cpp` (modified — add 7 routes)
- `firmware/adapters/WebPortal.h` (modified — declare LayoutsRoutes helpers)
- `firmware/core/WidgetRegistry.cpp` (modified — add `describeTypes()`)
- `firmware/core/WidgetRegistry.h` (modified — declare `describeTypes()`)
- `firmware/test/test_web_portal/test_main.cpp` (modified — extend coverage)
- `_bmad-output/planning-artifacts/api-layouts-spec.md` (new)
