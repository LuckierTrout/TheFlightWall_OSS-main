<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 4 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T190211Z -->
<compiled-workflow>
<mission><![CDATA[

Master Code Review Synthesis: Story le-1.4

You are synthesizing 3 independent code review findings.

Your mission:
1. VERIFY each issue raised by reviewers
   - Cross-reference with project_context.md (ground truth)
   - Cross-reference with git diff and source files
   - Identify false positives (issues that aren't real problems)
   - Confirm valid issues with evidence

2. PRIORITIZE real issues by severity
   - Critical: Security vulnerabilities, data corruption risks
   - High: Bugs, logic errors, missing error handling
   - Medium: Code quality issues, performance concerns
   - Low: Style issues, minor improvements

3. SYNTHESIZE findings
   - Merge duplicate issues from different reviewers
   - Note reviewer consensus (if 3+ agree, high confidence)
   - Highlight unique insights from individual reviewers

4. APPLY source code fixes
   - You have WRITE PERMISSION to modify SOURCE CODE files
   - CRITICAL: Before using Edit tool, ALWAYS Read the target file first
   - Use EXACT content from Read tool output as old_string, NOT content from this prompt
   - If Read output is truncated, use offset/limit parameters to locate the target section
   - Apply fixes for verified issues
   - Do NOT modify the story file (only Dev Agent Record if needed)
   - Document what you changed and why

Output format:
## Synthesis Summary
## Issues Verified (by severity)
## Issues Dismissed (false positives with reasoning)
## Source Code Fixes Applied

]]></mission>
<context>
<file id="ed7fe483" path="_bmad-output/project-context.md" label="PROJECT CONTEXT"><![CDATA[

---
project_name: TheFlightWall_OSS-main
date: '2026-04-12'
---

# Project Context for AI Agents

Lean rules for implementing FlightWall (ESP32 LED flight display + captive-portal web UI). Prefer existing patterns in `firmware/` over new abstractions.

## Technology Stack

- **Firmware:** C++11, ESP32 (Arduino/PlatformIO), FastLED + Adafruit GFX + FastLED NeoMatrix, ArduinoJson ^7.4.2.
- **Web on device:** ESPAsyncWebServer (**mathieucarbou fork**), AsyncTCP (**Carbou fork**), LittleFS (`board_build.filesystem = littlefs`), custom `custom_partitions.csv` (~2MB app + ~2MB LittleFS).
- **Dashboard assets:** Editable sources under `firmware/data-src/`; served bundles are **gzip** under `firmware/data/`. After editing a source file, regenerate the matching `.gz` from `firmware/` (e.g. `gzip -9 -c data-src/common.js > data/common.js.gz`).

## Critical Implementation Rules

- **Core pinning:** Display/task driving LEDs on **Core 0**; WiFi, HTTP server, and flight fetch pipeline on **Core 1** (FastLED + WiFi ISR constraints).
- **Config:** `ConfigManager` + NVS; debounce writes; atomic saves; use category getters; `POST /api/settings` JSON envelope `{ ok, data, error, code }` pattern for REST responses.
- **Heap / concurrency:** Cap concurrent web clients (~2–3); stream LittleFS reads; use ArduinoJson filter/streaming for large JSON; avoid full-file RAM buffering for uploads.
- **WiFi:** WiFiManager-style state machine (AP setup → STA → reconnect / AP fallback); mDNS `flightwall.local` in STA.
- **Structure:** Extend hexagonal layout — `firmware/core/`, `firmware/adapters/` (e.g. `WebPortal.cpp`), `firmware/interfaces/`, `firmware/models/`, `firmware/config/`, `firmware/utils/`.
- **Tooling:** Build from `firmware/` with `pio run`. On macOS serial: use `/dev/cu.*` (not `tty.*`); release serial monitor before upload.
- **Scope for code reviews:** Product code under `firmware/` and tests under `firmware/test/` and repo `tests/`; do not treat BMAD-only paths as product defects unless the task says so.

## Planning Artifacts

- Requirements and design: `_bmad-output/planning-artifacts/` (`architecture.md`, `epics.md`, PRDs).
- Stories and sprint line items: `_bmad-output/implementation-artifacts/` (e.g. `sprint-status.yaml`, per-story markdown).

## BMAD / bmad-assist

- **`bmad-assist.yaml`** at repo root configures providers and phases; `paths.project_knowledge` points at `_bmad-output/planning-artifacts/`, `paths.output_folder` at `_bmad-output/`.
- **This file** (`project-context.md`) is resolved at `_bmad-output/project-context.md` or `docs/project-context.md` (see `bmad-assist` compiler `find_project_context_file`).
- Keep **`sprint-status.yaml`** story keys aligned with `.bmad-assist/state.yaml` (`current_story`, `current_epic`) when using `bmad-assist run` so phases do not skip with “story not found”.


]]></file>
<file id="4b1bc60f" path="_bmad-output/implementation-artifacts/stories/le-1-4-layouts-rest-api.md" label="DOCUMENTATION"><![CDATA[


# Story LE-1.4: Layouts REST API

branch: le-1-4-layouts-rest-api
zone:
  - firmware/adapters/WebPortal.*
  - firmware/core/WidgetRegistry.*
  - firmware/test/test_web_portal/**
  - _bmad-output/planning-artifacts/api-layouts-spec.md

Status: done

## Story

As **the dashboard editor**,
I want **REST endpoints for layout CRUD and activation**,
So that **the browser can list/save/activate layouts with the standard envelope response**.

## Acceptance Criteria

1. **Given** stored layouts **When** `GET /api/layouts` is called **Then** it returns `{ok: true, data: {active_id: "<id-or-empty>", layouts: [{id, name, bytes}, ...]}}` — `active_id` is the currently persisted layout id from NVS (empty string when none is set) and `layouts` is one entry per layout on LittleFS (empty array when no layouts are saved). See `_bmad-output/planning-artifacts/api-layouts-spec.md` for the canonical contract consumed by the dashboard editor.
2. **Given** a valid id **When** `GET /api/layouts/:id` is called **Then** the full layout JSON is returned as `data`; missing id returns `{ok: false, error: "Layout not found", code: "LAYOUT_NOT_FOUND"}`.
3. **Given** a layout body in the request **When** `POST /api/layouts` is called **Then** a new layout is created via `LayoutStore::save`, returning `{ok: true, data: {id}}`; bodies >8 KB return `LAYOUT_TOO_LARGE`; invalid JSON or schema returns `LAYOUT_INVALID`; exceeding file count or total byte cap returns `FS_FULL`.
4. **Given** an existing id **When** `PUT /api/layouts/:id` is called **Then** the layout is overwritten and returns `{ok: true, data: {id, bytes}}` reflecting the new byte count; missing id returns `LAYOUT_NOT_FOUND`.
5. **Given** a non-active layout id **When** `DELETE /api/layouts/:id` is called **Then** the file is removed and `{ok: true}` is returned; **when** the id is the currently active layout **Then** the call fails with `{ok: false, error: "Cannot delete active layout", code: "LAYOUT_ACTIVE"}`; missing id returns `LAYOUT_NOT_FOUND`.
6. **Given** an id that exists on LittleFS **When** `POST /api/layouts/:id/activate` is called **Then** `LayoutStore::setActiveId(id)` persists the id to NVS via ConfigManager, `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")` is called to switch the display, and `{ok: true, data: {active_id}}` is returned; non-existent id returns `LAYOUT_NOT_FOUND`.
7. **Given** the editor needs widget type introspection **When** `GET /api/widgets/types` is called **Then** it returns `{ok: true, data: [{type, label, fields: [{key, kind, default}]}]}` for every widget type known to `WidgetRegistry`.
8. **Given** filesystem capacity is exhausted **When** any write endpoint (`POST`/`PUT`) fails with `LayoutStoreError::FS_FULL` **Then** the response is `{ok: false, error: "Filesystem full", code: "FS_FULL"}` and no partial file is left on LittleFS (LayoutStore::save already guarantees this — WebPortal just maps the error code).
9. **Given** integration tests **When** `pio test -e esp32dev --filter test_web_portal --without-uploading --without-testing` compiles cleanly **Then** every route handler is exercised by at least one test case in the test file.

## Tasks / Subtasks

- [x] **Task 1: Add layout route declarations to WebPortal.h** (AC: all)
  - [x] Add private handler method declarations for all 7 new routes
  - [x] No new public methods required — routes are wired internally in `_registerRoutes()`
  - [x] Add `#include "core/LayoutStore.h"` to `WebPortal.cpp` (NOT `.h` — keep header lean)

- [x] **Task 2: Register routes in `_registerRoutes()`** (AC: #1–#8)
  - [x] Add a `// ─── Layout CRUD (LE-1.4) ───` section comment near end of `_registerRoutes()`, before the gzipped asset block
  - [x] Register 6 JSON routes:
    - `GET /api/layouts` → `_handleGetLayouts`
    - `GET /api/layouts/*` → `_handleGetLayoutById` (wildcard path; extract id from URL like `_handleDeleteLogo`)
    - `POST /api/layouts` → `_handlePostLayout` (body handler pattern — see Task 3)
    - `PUT /api/layouts/*` → `_handlePutLayout` (body handler pattern)
    - `DELETE /api/layouts/*` → `_handleDeleteLayout`
    - `POST /api/layouts/*/activate` → `_handlePostLayoutActivate`
  - [x] Register 1 widget introspection route:
    - `GET /api/widgets/types` → `_handleGetWidgetTypes`
  - [x] **CRITICAL route-registration order**: `POST /api/layouts/*/activate` MUST be registered BEFORE `POST /api/layouts` in `_server->on()` calls, or ESPAsyncWebServer may match `activate` as a body on the wrong handler. Actually: ESPAsyncWebServer matches exact paths before wildcards, so `POST /api/layouts/*/activate` (with wildcard `*`) needs careful attention. Use the path string `"/api/layouts/*/activate"` — the `*` matches a single path segment on the mathieucarbou fork.

- [x] **Task 3: Implement GET /api/layouts** (AC: #1)
  - [x] Call `LayoutStore::list(entries)` — `std::vector<LayoutEntry>` where `LayoutEntry` has `.id`, `.name`, `.sizeBytes`
  - [x] If `list()` returns false, respond with 500 `STORAGE_UNAVAILABLE` (mirrors `_handleGetLogos` pattern)
  - [x] Build response: `{ok: true, data: [{id, name, bytes}]}` — use `bytes` (not `sizeBytes`) in the JSON key to match AC #1 naming
  - [x] Also include `active_id: LayoutStore::getActiveId()` at the top level of `data` for editor convenience

- [x] **Task 4: Implement GET /api/layouts/:id** (AC: #2)
  - [x] Extract `id` from URL path using `request->url()` + `lastIndexOf('/')` — same pattern as `_handleDeleteLogo`
  - [x] If `!LayoutStore::isSafeLayoutId(id.c_str())` → 400 `INVALID_ID`
  - [x] Call `LayoutStore::load(id.c_str(), bodyStr)` — **NOTE**: `load()` always fills `bodyStr` even on failure (fills with `kDefaultLayoutJson`); check the **return value** (bool). If returns `false`, respond with 404 `LAYOUT_NOT_FOUND`.
  - [x] If returns `true`, respond 200 with `{ok: true, data: <parsed JSON object>}`
  - [x] Parse the returned string into a `JsonDocument` to embed as a nested object — use scoped `JsonDocument` and serialize to `String` output. Do NOT return raw string as `data` field (it would be double-serialized).

- [x] **Task 5: Implement POST /api/layouts** (AC: #3, #8)
  - [x] Use the **body handler pattern** (same as `POST /api/settings`, `POST /api/display/mode`):
    ```cpp
    _server->on("/api/layouts", HTTP_POST,
        [](AsyncWebServerRequest* req) { /* no-op */ },
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            // ... chunked accumulation via g_pendingBodies ...
        }
    );
    ```
  - [x] Reject `total > LAYOUT_STORE_MAX_BYTES (8192)` immediately with 413 `LAYOUT_TOO_LARGE` (check before accumulating — mirrors `MAX_SETTINGS_BODY_BYTES` guard)
  - [x] On complete body, call `_handlePostLayout(request, data, len)` — same split as `_handlePostSettings`
  - [x] In handler:
    - [x] Parse JSON with `deserializeJson(doc, data, len)` — on `DeserializationError` → 400 `LAYOUT_INVALID`
    - [x] Extract `id` from `doc["id"]` — if missing or not a string → 400 `LAYOUT_INVALID`
    - [x] If `!LayoutStore::isSafeLayoutId(id)` → 400 `LAYOUT_INVALID`
    - [x] Serialize doc back to a `String` (needed for `LayoutStore::save`) via `serializeJson(doc, jsonStr)`
    - [x] Call `LayoutStore::save(id, jsonStr.c_str())` and map errors:
      - `OK` → 200 `{ok: true, data: {id}}`
      - `TOO_LARGE` → 413 `LAYOUT_TOO_LARGE`
      - `INVALID_SCHEMA` → 400 `LAYOUT_INVALID`
      - `FS_FULL` → 507 `FS_FULL`
      - `IO_ERROR` → 500 `IO_ERROR`

- [x] **Task 6: Implement PUT /api/layouts/:id** (AC: #4, #8)
  - [x] Use body handler pattern (same as POST)
  - [x] Extract `id` from URL path (same as Task 4)
  - [x] Reject `total > LAYOUT_STORE_MAX_BYTES` with 413 `LAYOUT_TOO_LARGE`
  - [x] In handler:
    - [x] Check id from URL is safe: `isSafeLayoutId` — 400 `INVALID_ID` if not
    - [x] Check that a file exists for this id already: call `LayoutStore::load(id, tmp)` and check return; if false → 404 `LAYOUT_NOT_FOUND` (caller must use POST to create)
    - [x] Parse JSON body; verify `doc["id"]` matches the URL id — mismatching id-in-JSON vs URL-id → 400 `LAYOUT_INVALID` (LayoutStore::save will also reject this, but fail early with clear error)
    - [x] Call `LayoutStore::save(id, jsonStr.c_str())` and map errors as in Task 5
    - [x] On success: 200 `{ok: true, data: {id, bytes: jsonStr.length()}}`

- [x] **Task 7: Implement DELETE /api/layouts/:id** (AC: #5)
  - [x] Extract `id` from URL (same pattern as `_handleDeleteLogo`)
  - [x] If `!isSafeLayoutId(id)` → 400 `INVALID_ID`
  - [x] Compare against active id: if `LayoutStore::getActiveId() == id` → 409 `LAYOUT_ACTIVE` ("Cannot delete active layout")
  - [x] Call `LayoutStore::remove(id.c_str())`:
    - `false` (file not found) → 404 `LAYOUT_NOT_FOUND`
    - `true` → 200 `{ok: true}`
  - [x] **NOTE**: `LayoutStore::remove()` returns bool (not `LayoutStoreError`). Check source: `remove()` returns `false` for both invalid id AND missing file. Since we already validated the id is safe above, `false` means not found.

- [x] **Task 8: Implement POST /api/layouts/:id/activate** (AC: #6)
  - [x] Extract `id` from URL path — the activate endpoint path is `/api/layouts/*/activate`; extract segment between `/api/layouts/` and `/activate` using substring operations
  - [x] If `!isSafeLayoutId(id)` → 400 `INVALID_ID`
  - [x] Verify layout exists: call `LayoutStore::load(id.c_str(), tmp)` — if false → 404 `LAYOUT_NOT_FOUND`
  - [x] Call `LayoutStore::setActiveId(id.c_str())` — returns bool; if false → 500 `NVS_ERROR`
  - [x] Call `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")` — this is the **Rule #24-compliant path** for switching to custom_layout from Core 1
  - [x] Respond: `{ok: true, data: {active_id: id}}`
  - [x] **CRITICAL**: Do NOT call `ModeRegistry::requestSwitch()` directly — Rule #24 mandates all user-intent mode switches go through `ModeOrchestrator::onManualSwitch()`. The orchestrator then drives ModeRegistry on the next tick from Core 0.
  - [x] **NOTE**: `LayoutStore::setActiveId()` calls `ConfigManager::setLayoutActiveId()` which fires onChange callbacks. The displayTask's `g_configChanged` flag gets set, causing a `ModeRegistry::requestForceReload("custom_layout")` on the next display tick — this is the mechanism that actually reloads the layout content. The `onManualSwitch` call additionally ensures the orchestrator state machine is in MANUAL state (not IDLE_FALLBACK or SCHEDULED).

- [x] **Task 9: Implement GET /api/widgets/types** (AC: #7)
  - [x] This is a **static introspection endpoint** — hardcode the schema for the 5 known widget types (text, clock, logo, flight_field, metric) based on `WidgetSpec` fields
  - [x] Do NOT add a `describeTypes()` method to WidgetRegistry — the draft story suggested this, but it would add complexity to the core for marginal benefit. A static response in WebPortal is simpler and sufficient for LE-1.7's needs.
  - [x] Response structure (example):
    ```json
    {
      "ok": true,
      "data": [
        {
          "type": "text",
          "label": "Text",
          "fields": [
            {"key": "content", "kind": "string", "default": ""},
            {"key": "align",   "kind": "select", "options": ["left","center","right"], "default": "left"},
            {"key": "color",   "kind": "color",  "default": "#FFFFFF"},
            {"key": "x",      "kind": "int",    "default": 0},
            {"key": "y",      "kind": "int",    "default": 0},
            {"key": "w",      "kind": "int",    "default": 32},
            {"key": "h",      "kind": "int",    "default": 8}
          ]
        },
        {
          "type": "clock",
          "label": "Clock",
          "fields": [
            {"key": "content", "kind": "select", "options": ["12h","24h"], "default": "24h"},
            {"key": "color",   "kind": "color",  "default": "#FFFFFF"},
            {"key": "x",      "kind": "int",    "default": 0},
            {"key": "y",      "kind": "int",    "default": 0},
            {"key": "w",      "kind": "int",    "default": 48},
            {"key": "h",      "kind": "int",    "default": 8}
          ]
        },
        {
          "type": "logo",
          "label": "Logo",
          "note": "LE-1.5 stub — renders solid color block until real bitmap pipeline lands",
          "fields": [
            {"key": "color",  "kind": "color",  "default": "#0000FF"},
            {"key": "x",      "kind": "int",    "default": 0},
            {"key": "y",      "kind": "int",    "default": 0},
            {"key": "w",      "kind": "int",    "default": 16},
            {"key": "h",      "kind": "int",    "default": 16}
          ]
        },
        {
          "type": "flight_field",
          "label": "Flight Field",
          "note": "LE-1.8 — not yet implemented, renders nothing",
          "fields": [
            {"key": "content", "kind": "string", "default": ""},
            {"key": "color",   "kind": "color",  "default": "#FFFFFF"},
            {"key": "x",      "kind": "int",    "default": 0},
            {"key": "y",      "kind": "int",    "default": 0},
            {"key": "w",      "kind": "int",    "default": 48},
            {"key": "h",      "kind": "int",    "default": 8}
          ]
        },
        {
          "type": "metric",
          "label": "Metric",
          "note": "LE-1.8 — not yet implemented, renders nothing",
          "fields": [
            {"key": "content", "kind": "string", "default": ""},
            {"key": "color",   "kind": "color",  "default": "#FFFFFF"},
            {"key": "x",      "kind": "int",    "default": 0},
            {"key": "y",      "kind": "int",    "default": 0},
            {"key": "w",      "kind": "int",    "default": 48},
            {"key": "h",      "kind": "int",    "default": 8}
          ]
        }
      ]
    }
    ```
  - [x] Use a single `JsonDocument` sized for this static payload; avoid multiple allocations

- [x] **Task 10: Add include and create test scaffolding** (AC: #9)
  - [x] In `WebPortal.cpp`: add `#include "core/LayoutStore.h"` and `#include "core/ModeOrchestrator.h"` (ModeOrchestrator.h is already included — verify)
  - [x] Create `firmware/test/test_web_portal/` directory if it does not exist
  - [x] Create `firmware/test/test_web_portal/test_main.cpp` with Unity test scaffolding; use compile-only approach (same as `test_custom_layout_mode`)
  - [x] Minimum test cases (compile-only acceptable per AC #9):
    - `test_get_layouts_empty` — list endpoint compiles and can be invoked via mock
    - `test_post_layout_invalid_json` — body handler rejects malformed JSON
    - `test_post_layout_too_large` — body size guard at 8193 bytes
    - `test_delete_layout_active_rejected` — LAYOUT_ACTIVE guard
    - `test_activate_layout_missing` — LAYOUT_NOT_FOUND guard
    - `test_get_widget_types` — static schema endpoint compiles

- [x] **Task 11: API spec document** (AC: all)
  - [x] Create `_bmad-output/planning-artifacts/api-layouts-spec.md` — document each route, method, request body, response envelope, error codes, and curl examples

- [x] **Task 12: Build and verify** (AC: all)
  - [x] `cd firmware && ~/.platformio/penv/bin/pio run -e esp32dev` — clean build
  - [x] Binary must be under 1.5 MB OTA partition
  - [x] `~/.platformio/penv/bin/pio test -e esp32dev --filter test_web_portal --without-uploading --without-testing` — compile check passes
  - [x] `~/.platformio/penv/bin/pio test -e esp32dev --filter test_layout_store --without-uploading --without-testing` — no regression in LE-1.1 tests
  - [x] `~/.platformio/penv/bin/pio test -e esp32dev --filter test_custom_layout_mode --without-uploading --without-testing` — no regression in LE-1.3 tests

## Dev Notes

### Envelope Convention (MANDATORY)
All responses use:
```json
{ "ok": true,  "data": { ... } }
{ "ok": false, "error": "Human message", "code": "MACHINE_CODE" }
```
The `_sendJsonError(request, httpCode, error, code)` helper already exists in WebPortal.cpp. Always use it for error paths — do NOT inline `{"ok":false,...}` strings.

### HTTP Status Code Map
| Situation | HTTP Code |
|---|---|
| Success | 200 |
| Invalid request body / id | 400 |
| Size limit exceeded | 413 |
| Active layout conflict | 409 |
| Not found | 404 |
| Filesystem full | 507 |
| NVS/IO error | 500 |

### Body Handler Pattern (MUST follow exactly)
All POST/PUT endpoints with JSON bodies use the three-callback `_server->on()` pattern. See existing `POST /api/settings` and `POST /api/display/mode` in `WebPortal.cpp` lines 194–241 and 318–356. The pattern:

```cpp
_server->on("/api/layouts", HTTP_POST,
    [](AsyncWebServerRequest* req) { /* no-op: response sent in body handler */ },
    nullptr,  // upload handler — null for JSON, only set for multipart
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
        if (data == nullptr || total == 0) {
            _sendJsonError(req, 400, "Empty body", "EMPTY_PAYLOAD");
            return;
        }
        if (total > LAYOUT_STORE_MAX_BYTES) {
            clearPendingBody(req);
            _sendJsonError(req, 413, "Layout too large", "LAYOUT_TOO_LARGE");
            return;
        }
        if (index == 0) {
            clearPendingBody(req);
            g_pendingBodies.push_back({req, String()});
            g_pendingBodies.back().body.reserve(total);
            req->onDisconnect([req]() { clearPendingBody(req); });
        }
        PendingRequestBody* pending = findPendingBody(req);
        if (pending == nullptr) {
            _sendJsonError(req, 400, "Incomplete body", "INCOMPLETE_BODY");
            return;
        }
        pending->body.concat(reinterpret_cast<const char*>(data), len);
        if (index + len == total) {
            _handlePostLayout(req,
                reinterpret_cast<uint8_t*>(const_cast<char*>(pending->body.c_str())),
                pending->body.length());
            clearPendingBody(req);
        }
    }
);
```

The `push_back first, then reserve` ordering is CRITICAL (see comment in existing code): `g_pendingBodies.push_back({req, String()}); g_pendingBodies.back().body.reserve(total);`. Do NOT do `String buf; buf.reserve(total); push_back({req, buf})` — the copy constructor loses the reserved capacity.

### URL ID Extraction Pattern
For wildcard routes (`/api/layouts/*`), extract the id segment using:
```cpp
String url = request->url();
int lastSlash = url.lastIndexOf('/');
String id = url.substring(lastSlash + 1);
// Strip query string if present
int queryStart = id.indexOf('?');
if (queryStart >= 0) id = id.substring(0, queryStart);
```
This is identical to the pattern in `_handleDeleteLogo` (WebPortal.cpp line 1361–1370). Copy it exactly.

For the **activate endpoint** (`/api/layouts/*/activate`), the id sits between the two path components. Use:
```cpp
String url = request->url();  // e.g. "/api/layouts/my-layout/activate"
// Strip "/api/layouts/" prefix and "/activate" suffix
const char* prefix = "/api/layouts/";
const char* suffix = "/activate";
size_t prefixLen = strlen(prefix);
size_t suffixLen = strlen(suffix);
if (!url.startsWith(prefix) || !url.endsWith(suffix)) {
    _sendJsonError(request, 400, "Invalid path", "INVALID_PATH");
    return;
}
String id = url.substring(prefixLen, url.length() - suffixLen);
```

### LayoutStore API Quick Reference
```cpp
// From firmware/core/LayoutStore.h — static class, no instance needed
bool         LayoutStore::init();                                // already called in setup()
bool         LayoutStore::isSafeLayoutId(const char* id);       // id validation
LayoutStoreError LayoutStore::save(const char* id, const char* json); // create/overwrite
bool         LayoutStore::load(const char* id, String& out);    // returns false → missing (fills kDefaultLayoutJson)
bool         LayoutStore::remove(const char* id);               // returns false → not found or invalid
bool         LayoutStore::list(std::vector<LayoutEntry>& result); // false → dir open fail
String       LayoutStore::getActiveId();                        // delegates to ConfigManager::getLayoutActiveId()
bool         LayoutStore::setActiveId(const char* id);          // validates id, delegates to ConfigManager

// LayoutEntry struct:
//   String id;
//   String name;
//   size_t sizeBytes;

// LayoutStoreError enum:
//   OK, NOT_FOUND, TOO_LARGE, INVALID_SCHEMA, FS_FULL, IO_ERROR
```

### LayoutStore::load() Gotcha — Return Value vs Output
`LayoutStore::load(id, out)` **always fills `out`** with something — even on failure it fills `out` with `kDefaultLayoutJson`. Check the **return value** to determine success:
- `true` → file found and read → `out` has the real layout JSON
- `false` → file not found or invalid → `out` has the fallback default

In the REST handlers, `false` means 404. Never inspect `out` to detect not-found.

### LayoutStore::save() id-in-JSON consistency requirement
`LayoutStore::save(pathId, json)` validates that `doc["id"]` inside the JSON **equals** `pathId`. If a client sends `POST /api/layouts` with `{"id":"foo",...}` the pathId and doc id must match. For POST, extract `id` from `doc["id"]` and use that as both the URL-derived key and the save argument. For PUT, the URL segment is authoritative — if `doc["id"]` differs from the URL id, return `LAYOUT_INVALID`.

### Mode Switching: Rule #24
**NEVER call `ModeRegistry::requestSwitch("custom_layout")` directly from WebPortal.**

The correct path for the activate endpoint is:
1. `LayoutStore::setActiveId(id)` → NVS persistence via ConfigManager
2. `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")` → Rule #24 compliant

Step 2 sets the orchestrator to MANUAL state and calls `ModeRegistry::requestSwitch()` internally on the next tick from Core 0. Additionally, `ConfigManager::setLayoutActiveId()` fires onChange callbacks → sets `g_configChanged` → displayTask calls `ModeRegistry::requestForceReload("custom_layout")` which causes CustomLayoutMode to re-init with the new active id. The two mechanisms work together: the orchestrator switch handles the case where we're NOT currently in custom_layout mode; the forceReload handles the case where we ARE already in custom_layout mode and just changed which layout is active.

### ModeOrchestrator Method Signatures (from ModeOrchestrator.h)
```cpp
// These are the only methods LE-1.4 touches:
static void onManualSwitch(const char* modeId, const char* modeName);
static const char* getStateString();  // "manual" | "idle_fallback" | "scheduled"
```
The `modeId` must be `"custom_layout"` (exact string — matches ModeEntry.id in main.cpp mode table).
The `modeName` should be `"Custom Layout"` (matches `displayName` in ModeEntry).

### ArduinoJson v7 Scoping Rule
All `JsonDocument` instances must be scoped — declare inside the handler function, not at file scope or as a static. They are destructed when the handler returns. Do NOT `static JsonDocument doc` in any handler.

### JsonDocument Sizing
For the widget types response (Task 9), the static payload is ~1.2 KB serialized. Use `JsonDocument doc` (v7 auto-sizes) or pre-allocate sufficient capacity. The v7 API does not require explicit capacity — `JsonDocument doc;` is correct.

### String vs char* in LayoutStore calls
`LayoutStore::getActiveId()` returns `String` (not `const char*`). Store it in a local `String`:
```cpp
String activeId = LayoutStore::getActiveId();
if (activeId == id) { /* LAYOUT_ACTIVE */ }
```
Do NOT compare against `nullptr` — `getActiveId()` always returns a `String` (may be empty `""`).

### WebPortal.cpp Existing Includes Already Present
Looking at WebPortal.cpp top (lines 23–48), these are already included:
- `#include "core/ModeOrchestrator.h"` ✓ (line 47)
- `#include "core/ModeRegistry.h"` ✓ (line 48)
- `#include <ArduinoJson.h>` ✓ (line 25)
- `#include <LittleFS.h>` ✓ (line 24)

You must add:
- `#include "core/LayoutStore.h"` ← NOT YET PRESENT

### ESPAsyncWebServer Wildcard Route Ordering
The mathieucarbou/Carbou fork of ESPAsyncWebServer matches routes in registration order. Register more specific paths before less specific ones:
1. Register `GET /api/layouts` first (exact)
2. Then `GET /api/layouts/*` (wildcard for get-by-id)
3. For POST: `POST /api/layouts/*/activate` before `POST /api/layouts/*`... but wait: `POST /api/layouts` (exact, no wildcard) vs `POST /api/layouts/*/activate` (two segments). These don't conflict. ESPAsyncWebServer distinguishes exact from wildcard.

### PIO Test File Structure (compile-only pattern)
```cpp
// firmware/test/test_web_portal/test_main.cpp
#include <unity.h>
// Pull in enough stubs to satisfy the linker in test env

void test_compile_placeholder() {
    // AC #9: compile-only verification that route handler code compiles
    TEST_ASSERT_TRUE(true);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_compile_placeholder);
    // ... additional tests ...
    UNITY_END();
}

void loop() {}
```
The test env `esp32dev` in `platformio.ini` already exists; the `test_web_portal` filter maps to the directory name. Check if a `test_web_portal` env is needed in `platformio.ini` or if the default `test_build_src_filter` covers it.

### Antipattern Prevention Table

| DO NOT | DO INSTEAD |
|---|---|
| `ModeRegistry::requestSwitch("custom_layout")` from WebPortal | `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")` (Rule #24) |
| `static JsonDocument doc;` in handler | `JsonDocument doc;` scoped to function |
| Compare `LayoutStore::getActiveId()` to `nullptr` | Compare the returned `String` directly: `if (activeId == id)` |
| Send raw layout JSON string as `data` field | Parse into `JsonDocument`, embed as object |
| Accumulate body in static buffer | Use `g_pendingBodies` + `clearPendingBody` pattern |
| Ignore disconnect while accumulating body | Call `request->onDisconnect([req]() { clearPendingBody(req); });` |
| `g_pendingBodies.push_back({req, buf_with_reserve})` | `push_back({req,String()})` THEN `back().body.reserve(total)` |
| Check `LayoutStore::load()` output for emptiness to detect not-found | Check the bool return value; `false` = not found |
| Call `LittleFS.remove()` directly | Use `LayoutStore::remove()` — it handles path construction |
| Use `String` heap allocation in render path | N/A — this story is REST only, not render |

### Sprint Status Update Required
After all tasks complete, update `_bmad-output/implementation-artifacts/sprint-status.yaml`:
```yaml
le-1-4-layouts-rest-api: done
```

## File List

Files modified:
- `firmware/adapters/WebPortal.h` (add 7 private handler declarations)
- `firmware/adapters/WebPortal.cpp` (add LayoutStore include, register 7 routes, implement 7 handlers)

Files created:
- `firmware/test/test_web_portal/test_main.cpp` (new — compile-only test scaffolding)
- `_bmad-output/planning-artifacts/api-layouts-spec.md` (new — REST API reference document)

Files NOT modified:
- `firmware/core/LayoutStore.h` / `.cpp` — no changes; LE-1.4 is a consumer only
- `firmware/core/WidgetRegistry.h` / `.cpp` — no changes (draft story suggested `describeTypes()` but static endpoint in WebPortal is simpler)
- `firmware/core/ConfigManager.h` / `.cpp` — no changes; `setLayoutActiveId()` fires callbacks already (LE-1.3 synthesis fix)
- `firmware/modes/CustomLayoutMode.*` — no changes
- `firmware/src/main.cpp` — no changes; `requestForceReload` path already wired in LE-1.3
- `firmware/platformio.ini` — no changes (no new source files in `build_src_filter`)

## Change Log

| Date | Version | Description | Author |
|---|---|---|---|
| 2026-04-17 | 0.1 | Draft story created (10-task outline, draft status) | BMAD |
| 2026-04-17 | 1.0 | Upgraded to ready-for-dev: exhaustive Dev Notes covering body-handler pattern, URL extraction, LayoutStore API gotchas, Rule #24 mode-switch path, activate dual-mechanism (onManualSwitch + requestForceReload via configChanged), JsonDocument scoping, include audit, route-order caveats, antipattern table, removed WidgetRegistry::describeTypes() (unnecessary complexity), corrected LayoutStore::load() return-value semantics, corrected ModeOrchestrator method signatures, added Task 11 (api spec) and Task 12 (build) | BMAD Story Synthesis |
| 2026-04-17 | 1.1 | Implementation complete: 7 handlers wired with envelope error mapping, Rule #24-compliant activate, compile-only test harness in test_web_portal/, API spec doc created, firmware builds at 1.24 MB (82.8% of 1.5 MB OTA partition), test_web_portal + test_layout_store + test_custom_layout_mode compile clean. Status → done. | Dev Agent |
| 2026-04-17 | 1.2 | Addressed code review findings — 2 follow-up items resolved: (1) AC #1 rewritten to document the shipped `{active_id, layouts:[...]}` response shape with a pointer to the canonical api-layouts-spec.md contract; (2) SYNC-RISK block comment added above `_handleGetWidgetTypes` (`firmware/adapters/WebPortal.cpp`) enumerating the three lock-step call sites (WebPortal schema, LayoutStore::kAllowedWidgetTypes, WidgetRegistry::fromString) and flagging LE-1.7 as the consolidation milestone. Firmware rebuilds clean at 1.24 MB / 82.8%; test_web_portal still compiles. | Dev Agent |

## Dev Agent Record

### Agent Model Used
claude-sonnet-4-5 (BMAD dev-story workflow, context-continuation run)

### Debug Log References
- Build (esp32dev): `firmware.bin` = 1,302,048 bytes (1.24 MB), 82.8% of 1.5 MB OTA partition — within limit
- RAM: 17.4% used (56,968 / 327,680 bytes)
- Test compile: `test_web_portal` PASSED (18.33 s), `test_layout_store` PASSED (11.82 s, regression check), `test_custom_layout_mode` PASSED (11.74 s, regression check)
- Warnings: only pre-existing `DynamicJsonDocument` deprecations in `OpenSkyFetcher.cpp` (unrelated to LE-1.4)

### Completion Notes
- All 7 handlers implemented in `firmware/adapters/WebPortal.cpp` with the canonical `{ok, data | error, code}` envelope via `_sendJsonError()`
- Route registration order: `POST /api/layouts/*/activate` registered before the generic `/api/layouts/*` wildcards; `GET /api/widgets/types` registered before `GET /api/layouts/*` is a non-issue because the path prefixes differ
- URL id extraction centralized in an anonymous-namespace helper `extractLayoutIdFromUrl(url, activateSegment)` — handles both `/api/layouts/<id>` and `/api/layouts/<id>/activate`
- Body accumulation uses the existing `g_pendingBodies` vector and `clearPendingBody` helpers — `push_back({req, String()})` first, then `.back().body.reserve(total)` per the String-capacity comment in existing code
- `LAYOUT_STORE_MAX_BYTES` (8192) enforced before body buffer grows — 413 `LAYOUT_TOO_LARGE` returned immediately with `clearPendingBody()` called
- Activate endpoint follows Rule #24: `LayoutStore::setActiveId()` (NVS persistence via `ConfigManager::setLayoutActiveId`) → `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")`. `ModeRegistry::requestSwitch()` is never called directly from the web layer.
- `LayoutStore::load()` return-value semantics observed — `false` → 404 `LAYOUT_NOT_FOUND`; output `String` contents (fallback default) never inspected for emptiness
- `GET /api/layouts/:id` parses the stored JSON into a scoped `JsonDocument` and embeds under `data` as a nested object — no double-encoding
- `GET /api/widgets/types` builds the 5-widget schema with a single scoped `JsonDocument` and an `addField` lambda to minimize duplication
- Test harness is compile-only per AC #9 — live `AsyncWebServerRequest` cannot be mocked outside the ESPAsyncWebServer stack. 6 Unity tests verify: WebPortal instantiation, `LAYOUT_STORE_MAX_BYTES` constant, `isSafeLayoutId` guardrails, `getActiveId` return-type round-trip, `ModeOrchestrator::onManualSwitch` symbol resolution, widget-types static response
- API spec doc `_bmad-output/planning-artifacts/api-layouts-spec.md` documents all 7 endpoints with request/response schemas, HTTP status map, validation rules, curl examples
- Flash usage unchanged relative to pre-story baseline within the ~1-2 KB noise band attributable to the new handlers
- ✅ Resolved review finding [MEDIUM]: AC #1 rewritten to describe the shipped `{active_id, layouts:[{id,name,bytes}]}` shape and redirect implementors to `api-layouts-spec.md` as the canonical contract (eliminates drift between story text and implementation).
- ✅ Resolved review finding [LOW]: SYNC-RISK block comment added above `_handleGetWidgetTypes` in `firmware/adapters/WebPortal.cpp` documenting the three lock-step call sites (`LayoutStore::kAllowedWidgetTypes`, `WidgetRegistry::fromString`, this handler) that must stay aligned, with explicit LE-1.7 consolidation cue.
- Post-fix rebuild: `pio run -e esp32dev` SUCCESS — 1,302,096 bytes (1.24 MB, 82.8% of 1.5 MB OTA partition, +48 bytes vs v1.1 from comment debug strings). `pio test -e esp32dev --filter test_web_portal --without-uploading --without-testing` PASSED (18.72 s).

### File List
Files modified:
- `firmware/adapters/WebPortal.h` — added 7 private handler declarations in a `// Layout CRUD + activation (LE-1.4)` block
- `firmware/adapters/WebPortal.cpp` — added `#include "core/LayoutStore.h"`, registered 7 routes in `_registerRoutes()`, implemented 7 handlers + `extractLayoutIdFromUrl` helper (~380 new lines); v1.2 review fix adds a SYNC-RISK block comment above `_handleGetWidgetTypes`

Files created:
- `firmware/test/test_web_portal/test_main.cpp` — compile-only Unity test scaffolding (6 tests)
- `_bmad-output/planning-artifacts/api-layouts-spec.md` — REST API reference with envelope spec, HTTP status map, per-endpoint request/response/errors/curl examples

## Senior Developer Review (AI)

### Review: 2026-04-17
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 1.3 (after false-positive dismissals) → PASS
- **Issues Found:** 9 raised across 3 reviewers; 1 valid fix applied, 8 dismissed as false positives
- **Issues Fixed:** 1 (low severity — PUT handler: split missing-id vs mismatched-id error messages)
- **Action Items Created:** 2

#### Review Follow-ups (AI)
- [x] [AI-Review] MEDIUM: AC #1 story text says `data: [{id,name,bytes}]` but implementation ships `data: {active_id, layouts:[{id,name,bytes}]}` — update story AC #1 text to reflect the richer (correct) shape so client authors reading the story build the right contract (`_bmad-output/implementation-artifacts/stories/le-1-4-layouts-rest-api.md`)
- [x] [AI-Review] LOW: `GET /api/widgets/types` widget metadata is hard-coded in WebPortal.cpp, duplicating the type-string knowledge already in `LayoutStore::kAllowedWidgetTypes` and `WidgetRegistry::fromString` — document sync-risk in a code comment or consolidate in LE-1.7 when the widget system stabilises (`firmware/adapters/WebPortal.cpp` ~line 2056)


]]></file>
<file id="90dfc412" path="[ANTIPATTERNS - DO NOT REPEAT]" label="VIRTUAL CONTENT"><![CDATA[

# Epic le-1 - Code Antipatterns

> **WARNING: ANTI-PATTERNS**
> The issues below were MISTAKES found during code review of previous stories.
> DO NOT repeat these patterns. Learn from them and avoid similar errors.
> These represent implementation mistakes (race conditions, missing tests, weak assertions, etc.)

## Story le-1-1 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `load()` unbounded `readString()` — if a layout file is larger than `LAYOUT_STORE_MAX_BYTES` (e.g., manual upload or corruption), `f.readString()` allocates the full file to heap, risking OOM on ESP32. | Added `f.size() > LAYOUT_STORE_MAX_BYTES` guard before `readString()`; falls back to default and logs. |
| high | `list()` unbounded `readString()` in per-entry loop — same heap risk as above, amplified up to 16× in the worst case. | Wrapped the `readString()`+`JsonDocument` block in `if (entry.size() <= LAYOUT_STORE_MAX_BYTES)` guard; oversized entries still appear in results with empty `name`. |
| high | `_default` id not blocked — `isSafeLayoutId("_default")` returns `true` because underscore is allowed, allowing a caller to write `_default.json` which would shadow the baked-in fallback. | Added `if (strcmp(id, "_default") == 0) return false;` with explanatory comment. |
| high | Document `id` vs filename desync — `save("foo", {"id":"bar",...})` succeeds, creating `foo.json` whose JSON claims it is `bar`. Downstream active-id / UI reads the filename as truth but the JSON disagrees. | `validateSchema()` signature extended to `validateSchema(json, pathId)`; rejects if `doc["id"] != pathId`. Call site in `save()` updated. |
| medium | AC6 log messages omit the offending id — AC6 specifies `"layout_active '<id>' missing — using default"` but implementation logged generic fixed strings. | Replaced `LOG_W` (macro accepts only string literals; no variadic support) with `Serial.printf` for all four `load()` warning paths, embedding `id` and file size as appropriate. |
| medium | No AC5 NVS round-trip test — `setActiveId`/`getActiveId` had zero Unity coverage despite being a complete AC. | Added `test_active_id_roundtrip` exercising write→read→re-write→read with `LayoutStore::{set,get}ActiveId`. |
| medium | No AC6 end-to-end test (active-id → missing file → default) — the path where `getActiveId()` returns a stale NVS value and `load()` falls back was not tested. | Added `test_load_with_missing_active_id_uses_default` using `ConfigManager::setLayoutActiveId("nonexistent")` then asserting `load()` returns false and out contains `"_default"`. |
| medium | No test asserting `_default` cannot be saved (reservation). | Added `test_save_rejects_reserved_default_id`. |
| medium | No test for doc id vs filename mismatch (new validation rule). | Added `test_save_rejects_id_mismatch`. |
| dismissed | "CRITICAL: V0 cleanup unverifiable — main.cpp incomplete, no CI output" | FALSE POSITIVE: `grep -r "LE_V0_METRICS\\|le_v0_record\\|v0_spike_layout" firmware/src firmware/core firmware/modes` returns zero results (verified in synthesis environment). `git status` confirms `main.cpp` was modified. Binary size 1.22 MB is consistent with V0 instrumentation removal. The reviewer was working from a truncated code snippet but the actual file is clean. |
| dismissed | "Widget nullptr edge case — `(const char*)nullptr` cast is a logic error" | FALSE POSITIVE: `isAllowedWidgetType(nullptr)` returns `false` (line 89 of `LayoutStore.cpp`), causing `validateSchema` to return `INVALID_SCHEMA`. This is the correct behavior. The concern is purely stylistic — the logic is sound. |
| dismissed | "AC #5 documentation error — 12 vs 13 chars" | FALSE POSITIVE: The Dev Agent Record already acknowledges this discrepancy and notes 13 chars is within the 15-char NVS limit. The implementation is correct; only the AC text has a benign counting error. This is a story documentation note, not a code defect. |
| dismissed | "Redundant `LittleFS.exists(LAYOUTS_DIR)` check in `save()`" | FALSE POSITIVE: The inline comment at line 241-247 explains the rationale: "fresh devices reach save() before init() only via tests that explicitly skip init()". This is intentional defensive coding that protects against test harness misuse. The single `exists()` call has negligible performance impact. |
| dismissed | "`setLayoutActiveId` return check — partial write risk from `written == strlen(id)`" | FALSE POSITIVE: `LayoutStore::setActiveId()` (the only caller) pre-validates id with `isSafeLayoutId()` which rejects empty strings and enforces `strlen > 0`. Empty-string NVS writes are structurally prevented at the public API boundary. The theoretical partial-write scenario would require `putString` to return a value inconsistent with actual NVS behaviour, which is platform-specific noise not an application bug. |
| dismissed | "JsonDocument not guaranteed freed in `list()` loop — memory leak risk" | FALSE POSITIVE: `JsonDocument doc` is stack-allocated within the loop body. C++ RAII guarantees destruction at end of scope regardless of exit path (including `continue`, exceptions are not used in this codebase). The added size guard in the synthesis fix further reduces the window in which a `JsonDocument` is allocated at all. |
| dismissed | "`isNull()` vs explicit null edge cases are surprising in ArduinoJson v7" | FALSE POSITIVE: `hasRequiredTopLevelFields` correctly uses `isNull()` to detect missing or null keys. The ArduinoJson v7 documentation explicitly states `isNull()` returns true for missing keys. The hypothetical `"widgets": null` JSON input would correctly fail the `!doc["widgets"].isNull()` check. Low-value concern for this codebase. --- |

## Story le-1-2 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| high | `ClockWidget` does not advance `s_lastClockUpdateMs` when `getLocalTime()` fails. With NTP unreachable, the refresh condition `(s_lastClockUpdateMs == 0 | Moved `s_lastClockUpdateMs = nowMs` outside the `if (getLocalTime...)` branch so it advances on both success and failure. |
| medium | `DisplayUtils::drawBitmapRGB565()` had a stale comment "Bitmap is always 32×32 (LOGO_WIDTH x LOGO_HEIGHT)" — the function signature accepts arbitrary `w`, `h` dimensions and LogoWidget passes 16×16. The incorrect comment degrades trust for onboarding readers. | Rewrote comment to "Render an RGB565 bitmap (w×h pixels)… Bitmap dimensions are caller-specified (e.g. 16×16 for LogoWidget V1 stub)." |
| low | `test_clock_cache_reuse` accepted `getTimeCallCount() <= 1` but count `0` trivially satisfies that assertion even when 50 calls all hit the refresh branch (failure storm). After the clock throttle bug was fixed, the test structure should confirm the throttle actually fires. | Restructured to two phases — Phase 1 (50 calls from cold state: count ≤ 1), Phase 2 (reset, arm cache on first call, 49 more calls: count still ≤ 1) — making the test meaningful regardless of NTP state. |
| low | `drawBitmapRGB565` skips pixels where `pixel == 0x0000` (treating black as transparent). A `spec.color = 0x0000` logo stub renders as invisible. This is undocumented surprising behaviour for callers. | Added inline NOTE comment documenting the black-as-transparent behaviour and why it does not affect LE-1.5 real bitmaps. |

## Story le-1-2 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | `ClockWidgetTest` namespace compiled unconditionally into production firmware | Gated both the header declaration and the `.cpp` implementation behind `#ifdef PIO_UNIT_TESTING` so the test-introspection API is stripped from production binary builds. |
| low | Stale/incorrect comment in `WidgetRegistry.cpp` claiming "The final `default` only exists to handle WidgetType::Unknown..." when no `default:` label exists in the switch | Rewrote both the file-header comment (line 5–7) and the inline dispatch comment to accurately describe that `WidgetType::Unknown` has its own explicit `case`, and that out-of-range values fall through to the post-switch `return false` — valid C++, no UB. |
| low | Single global clock cache is a V1 limitation invisible to LE-1.3 implementors — two Clock widgets in one layout silently share state | Added a clearly-labelled `V1 KNOWN LIMITATION` block in the file header documenting the shared cache behavior and the LE-1.7+ migration path. |

## Story le-1-2 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | Stale `~XXX,XXX bytes / ~YY,YYY bytes` placeholder in `DisplayMode.h` heap baseline comment degrades onboarding trust | Replaced `~XXX,XXX bytes / Max alloc: ~YY,YYY bytes` placeholder values and `(Update with actual values...)` instruction with a cleaner comment that still communicates "not yet measured" without the template noise. |
| dismissed | AC#3 violated — DisplayMode uses virtual methods, contradicting "no vtable per widget" | FALSE POSITIVE: AC#3 is explicitly scoped to the *WidgetRegistry dispatch mechanism* ("not virtual-function vtable per widget"). `DisplayMode` is a pre-existing, pre-LE-1.2 abstract interface for the mode system — entirely separate from the widget render dispatch path. LE-1.2 dev notes explicitly designate `DisplayMode.h` as "Untouched". The `WidgetRegistry::dispatch()` switch confirmed zero virtual calls. The AC language "per widget" is unambiguous. |
| dismissed | Task 8 grep validation is misleadingly narrow (missing `DisplayMode.h` in scope) | FALSE POSITIVE: Task 8's grep scope (`firmware/core/WidgetRegistry.cpp firmware/widgets/`) is intentionally narrow and correct — it verifies that LE-1.2's new widget code has no virtual dispatch. `DisplayMode.h` pre-dates LE-1.2 and is out of the story zone. The completion claim is accurate for the stated purpose. |
| dismissed | ClockWidget NTP throttle bug still present despite synthesis fix | FALSE POSITIVE: Reviewer C self-corrected this to FALSE POSITIVE upon re-reading the code. Confirmed: `s_lastClockUpdateMs = nowMs` at line 62 of `ClockWidget.cpp` is correctly placed *outside* the `if (getLocalTime...)` block, advancing the timer on both success and failure. |
| dismissed | platformio.ini missing `-I widgets` build flag | FALSE POSITIVE: `firmware/platformio.ini` line 43 shows `-I widgets` is present. Direct source verification confirms the flag is there. False positive. |
| dismissed | AC#8 unverifiable — test file not provided | FALSE POSITIVE: `firmware/test/test_widget_registry/test_main.cpp` is fully visible in the review context (275 lines, complete). The two-phase `test_clock_cache_reuse` is implemented exactly as the synthesis round 2 record describes, with `TEST_ASSERT_LESS_OR_EQUAL(1u, ...)` assertions in both phases. False positive. |
| dismissed | AC#7 "silent no-op on undersized widgets" violates the clipping requirement | FALSE POSITIVE: AC#7 requires "clips safely and does not write out-of-bounds." Silent no-op (returning `true` without drawing) IS the safest possible clip for an entire dimension below the font floor. The AC does not mandate partial rendering. All three widget implementations (Text, Clock, Logo) correctly return `true` as a no-op for below-floor specs. Fully compliant. |
| dismissed | LogoWidget `spec.color = 0x0000` renders invisible — undocumented gotcha | FALSE POSITIVE: Already addressed in a prior synthesis round. `LogoWidget.cpp` lines 39–42 contain an explicit `NOTE:` comment documenting the black-as-transparent behavior and explaining LE-1.5 is unaffected. The antipatterns table also documents this. Nothing new to fix. |
| dismissed | Widget `id` fields could collide with ConfigManager NVS keys (e.g., widget `id="timezone"`) | FALSE POSITIVE: Architectural confusion. Widget `id` fields (e.g., `"w1"`) are JSON properties stored inside layout documents on LittleFS. They are never written to NVS. ConfigManager's NVS keys (`"timezone"`, `"disp_mode"`, etc.) are entirely separate storage. The two namespaces cannot collide. LayoutStore's `isSafeLayoutId()` governs layout *file* identifiers, not widget instance ids within a layout. False positive. |
| dismissed | `test_text_alignment_all_three` is a "lying test" — only proves no-crash, not pixel math | FALSE POSITIVE: Valid observation, but by-design. The test file header explicitly labels this as a "smoke test" (line 212: "pixel-accurate assertions require a real framebuffer which we don't have in the test env"). The null-matrix scaffold is the documented test contract for all hardware-free unit tests in this project (see dev notes → Test Infrastructure). Alignment math IS present in `TextWidget.cpp` (lines 53–59) and is exercised on real hardware via the build verification. No lying — the smoke test boundary is honest and documented. |
| dismissed | `test_clock_cache_reuse` is weak — count=0 satisfies `≤1` even when cache is broken | FALSE POSITIVE: The two-phase test structure addresses this. Phase 2 explicitly arms the cache first (call 1), then runs 49 more calls — proving the cache throttle fires within a single `millis()` window. On NTP-down rigs, `getTimeCallCount()` = 0 for all 50 calls is still meaningful: it proves `getLocalTime()` was not called 50 times. The prior synthesis already restructured the test and the comment in the test explains this reasoning explicitly at line 159–161. |
| dismissed | `dispatch()` ignores `spec.type` — `(type, spec)` mismatch silently renders wrong widget | FALSE POSITIVE: Design choice, not a bug. The header comment at lines 89–91 of `WidgetRegistry.h` explicitly documents: "type is passed explicitly (rather than reading spec.type) to allow future callers to force a type without mutating the spec." LE-1.3 will populate specs from JSON; the type will always match. If a mismatch is desired for testing or fallback, this design supports it. A `debug-assert` could be added in the future but is out of LE-1.2 scope. |
| dismissed | `LayoutStore` / `WidgetRegistry` dual-source type string sync bomb | FALSE POSITIVE: Acknowledged design limitation. Already documented in `WidgetRegistry.cpp` (lines 11–13: "The allowlist here MUST stay in lock-step with LayoutStore's kAllowedWidgetTypes[]"). Centralization is a valid future improvement but is explicitly out of LE-1.2 scope per the story's architecture notes. Not a bug in the current implementation. |
| dismissed | `delay(2000)` in `setup()` slows every on-device test run | FALSE POSITIVE: Standard ESP32 Unity practice — the 2s delay allows the serial monitor to connect before test output begins. Removing it risks losing the first test results on some host configurations. Consistent with the `test_layout_store` scaffold which this test mirrors. Low-impact, by-convention. |
| dismissed | Include order in `LogoWidget.cpp` (comments before includes is unconventional) | FALSE POSITIVE: Style-only. The pattern (file header comment → includes) is consistent with `TextWidget.cpp`, `ClockWidget.cpp`, and `WidgetRegistry.cpp`. Not an inconsistency — it IS the project's established pattern. No change warranted. --- |

## Story le-1-3 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| critical | AC #7 is functionally broken — `ModeRegistry::requestSwitch("custom_layout")` is a no-op when `custom_layout` is already the active mode. `ModeRegistry::tick()` lines 408–411 contain an explicit same-mode idempotency guard: `if (requested != _activeModeIndex)` — the else branch just resets state to IDLE without calling `executeSwitch()`. Layout never reloads. | Added `ModeRegistry::requestForceReload()` to `ModeRegistry.h/.cpp` which atomically clears `_activeModeIndex` to `MODE_INDEX_NONE` before storing the request index, forcing `tick()` to take the `executeSwitch` path. Updated `main.cpp` hook to call `requestForceReload` instead of `requestSwitch`. |
| high | `ConfigManager::setLayoutActiveId()` does not call `fireCallbacks()`, so changing the active layout id in NVS never sets `g_configChanged`. This means the entire AC #7 re-init chain never fires even if the same-mode guard were fixed — the display task's `_cfgChg` is never `true` after a layout activation. | Added `fireCallbacks()` call in `setLayoutActiveId()` after successful NVS write. Also tightened the return path — previously returned `true` even on partial write; now returns `false` if `written != strlen(id)` (which was already the boolean expression but was lost in the refactor to add the callback). |
| medium | Misleading `_activeModeIndex` write in `requestForceReload()` races with Core 0 reading it between the two stores. Analysis: both `_activeModeIndex` and `_requestedIndex` are `std::atomic<uint8_t>`, and Core 0 only reads `_activeModeIndex` *after* it has already consumed and cleared `_requestedIndex`. The narrow window where Core 0 could observe `_activeModeIndex == MODE_INDEX_NONE` without a corresponding pending request is benign — it would simply render a tick with no active mode (same as startup). This is acceptable for an infrequent layout-reload path. Documented in the implementation comment. | Documented the race window and its benign nature in the implementation comments. No code change needed. |
| low | `test_render_invalid_does_not_crash` uses `ctx.matrix = nullptr`, so `render()` short-circuits at line 202 (`if (ctx.matrix == nullptr) return`) before reaching the `_invalid` branch and error-indicator drawing code. AC #5 error UI is not exercised in tests. | Deferred — requires either a matrix stub/mock or on-hardware test harness. Created [AI-Review] action item. |
| low | Log line in `init()` failure path (`"init: parse failed — error indicator will render"`) does not match the AC #5 specified literal (`"parse failed: %s"`). The `deserializeJson` error *is* logged in `parseFromJson()`, but the `init()` wrapper logs a different string. | Not applied — the error string *is* printed (in `parseFromJson`) and the `init()` wrapper adds context. The AC wording is guidance, not a literal string contract. Dismissed as minor documentation imprecision. |

## Story le-1-4 (2026-04-17)

| Severity | Issue | Fix |
|----------|-------|-----|
| low | `_handlePutLayout` gave the same `"Body id does not match URL id"` error message for two distinct failure modes: (a) the `id` field is missing from the JSON body entirely, and (b) the `id` field is present but differs from the URL path segment. The misleading message in case (a) makes debugging harder for API clients. | Split into two separate checks with distinct error messages ("Missing id field in layout body" vs "Body id does not match URL id"), both returning `LAYOUT_INVALID` with HTTP 400. **Applied.** |
| dismissed | AC #9 / Task #10 — "Tests are lying tests / TEST_PASS() stubs that exercise nothing" | FALSE POSITIVE: The test file header (lines 1–29) explicitly documents why compile-only is the contract: `AsyncWebServerRequest*` cannot be constructed outside the ESPAsyncWebServer stack on ESP32. The story Dev Notes (Task 10) explicitly state "compile-only acceptable per AC #9." The AC #9 wording says "exercised by at least one test case" — the test file exercises the underlying layer APIs (LayoutStore, ModeOrchestrator symbols) that each handler depends on, satisfying the spirit of the AC within the constraints of the embedded test stack. This is a story documentation ambiguity, not a code defect. The 6 tests are not assertionless stubs: `test_layout_store_api_visible` has 4 real assertions verifying isSafeLayoutId and constant values; `test_activate_orchestrator_handoff_compiles` asserts function pointer non-null. `TEST_PASS()` in the remaining tests is correctly used for compile-time verification where there is no runtime observable behavior to assert. |
| dismissed | AC #6 / CRITICAL — "Hardcoded `custom_layout` mode ID not validated against ModeRegistry at runtime" | FALSE POSITIVE: The existing `_handlePostDisplayMode` (line 1318) applies exactly the same pattern without runtime validation of the hardcoded `custom_layout` string. The mode table is set up once in `main.cpp` and `custom_layout` is a core architectural constant for this product, not a user-configurable value. Adding a ModeRegistry lookup on every activate call would add heap churn and latency for a defensive check that would only fire during development (and would be immediately visible in testing). The established project pattern (confirmed in LE-1.3 Dev Notes) does not validate this string at the call site. |
| dismissed | Reviewer C CRITICAL — "`setActiveId` persist success not atomic with `onManualSwitch` — orchestrator called before/regardless of persistence" | FALSE POSITIVE: Direct code inspection (lines 2038–2044) confirms the implementation is already correct: `if (!LayoutStore::setActiveId(id.c_str())) { _sendJsonError(...); return; }` — the early `return` on failure means `ModeOrchestrator::onManualSwitch()` is only reached when `setActiveId` succeeds. Reviewer C's description of the bug does not match the actual code. False positive. |
| dismissed | AC #1 response shape — "implementation nests under `data.layouts` and adds `data.active_id`; story AC #1 says a flat top-level array" | FALSE POSITIVE: This is a real documentation drift between the story's AC text and the implementation, but it is NOT a code defect. The `api-layouts-spec.md` that was created as part of this story correctly documents the richer `{active_id, layouts:[...]}` shape. The editor client needs `active_id` alongside the list for a good UX; the implementation is correct and intentional per Task 3's dev notes ("Also include `active_id: LayoutStore::getActiveId()` at the top level of `data` for editor convenience"). An [AI-Review] action item was created to update the AC text; no code change required. |
| dismissed | DRY violation — ID validation duplicated 4× instead of shared function | FALSE POSITIVE: The `isSafeLayoutId()` validation is a single-line call (`LayoutStore::isSafeLayoutId(id.c_str())`) that is correctly placed in each handler independently. It is not duplicated logic — it is a validation guard that each handler must own because each extracts its own `id` variable. Extracting it into `extractLayoutIdFromUrl` would couple URL parsing to ID validation, creating its own concerns. The existing pattern is consistent with how `_handleDeleteLogo`, `_handleGetLogoFile`, etc. handle their own validation. Not a DRY violation. |
| dismissed | NVS write atomicity for cross-core safety not documented | FALSE POSITIVE: This concern is addressed in the LE-1.3 synthesis antipatterns record, which explicitly analyzed the `ConfigManager::setLayoutActiveId()` + `fireCallbacks()` chain as a LE-1.3 fix. The `Preferences::putString()` call is handled within the existing ConfigManager atomic-write pattern (debounce + NVS handle). The concern about cross-core partial writes on string values is noted in LE-1.3 context as a known benign window (same analysis as the `setLayoutActiveId` return-check dismissal in the le-1-1 antipatterns table). No new risk introduced by LE-1.4. |
| dismissed | `GET /api/widgets/types` — widget metadata hard-coded in WebPortal creates dual-source sync risk | FALSE POSITIVE: While a cheaper existence check (e.g. `LittleFS.exists()`) would work, using `LayoutStore::load()` is consistent with the established codebase pattern (the activate handler also uses `load()` for existence verification). The 8 KiB read is bounded, and PUT operations are rare user-initiated writes. The performance concern is valid but low-impact on the use case. Noted as a future improvement. |
| dismissed | `_handleGetLayoutById` uses two `JsonDocument` instances (extra heap allocation) | FALSE POSITIVE: The two-document pattern (`doc` for the outer envelope + `layoutDoc` for the nested parsed layout) is required because ArduinoJson v7 does not support copying a deserialized variant into a different document's object graph without a separate parse. The alternative (parse into one doc and re-build the envelope) would be more complex and error-prone. The extra allocation is bounded by `LAYOUT_STORE_MAX_BYTES` (8 KiB) and is immediately freed when the handler returns. Not a problem in practice on an ESP32 with 327 KB RAM (17.4% used per build log). |
| dismissed | SRP/DI violations — WebPortal is a "Fat Controller"; handlers should be injectable | FALSE POSITIVE: The project context explicitly names `firmware/adapters/WebPortal.cpp` as an adapter in the hexagonal architecture. Adapter classes in this project are intentionally responsible for routing + protocol translation + domain call delegation. Introducing an intermediate `LayoutController` or interface injection layer on an ESP32 with limited heap would add abstractions with zero testability benefit (test stack still can't mock `AsyncWebServerRequest`). This is an appropriate architectural choice for the embedded context. --- |


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial code review complete, Christian.**

**Story:** `_bmad-output/implementation-artifacts/stories/le-1-4-layouts-rest-api.md`  
**Primary evidence:** embedded `WebPortal.cpp` / `WebPortal.h` / `test_web_portal/test_main.cpp` / story + `api-layouts-spec.md` excerpts (read-only; no repo mutation).

---

## Review summary

| Category | Issues found |
|----------|----------------|
| Git vs story discrepancies | 1 (zone vs “files NOT modified”; not git-proven—**cannot** run `git diff` per read-only rule) |
| AC implementation gaps | 1 (AC #7 wording vs static WebPortal schema) |
| Task completion lies | 0 (no `[x]` item clearly contradicted by provided excerpts) |
| SOLID / boundary | 2 |
| Hidden bugs / correctness traps | 2 |
| Performance | 1 |
| Security | 1 (inherited LAN threat model) |
| Lying / weak tests | 2 |
| **Total issues (listed)** | **10** |

---

## Evidence score summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| IMPORTANT | AC #7 requires introspection “for every widget type known to `WidgetRegistry`”, but `_handleGetWidgetTypes` hard-codes schemas in `WebPortal.cpp` (sync bomb across `LayoutStore::kAllowedWidgetTypes`, `WidgetRegistry::fromString`, WebPortal). | AC #7 vs `_handleGetWidgetTypes` | +1.0 |
| IMPORTANT | AC #9 claims each **route handler** is “exercised” by tests; `test_web_portal/test_main.cpp` never calls `_handleGetLayouts`, `_handlePostLayout`, etc.—only touches symbols/constants/`WebPortal` default ctor. | AC #9 vs `test_main.cpp` | +1.0 |
| MINOR | Story `zone:` lists `firmware/core/WidgetRegistry.*` while “Files NOT modified” explicitly excludes `WidgetRegistry`; traceability/review scope mismatch. | Story frontmatter | +0.3 |
| MINOR | `_handlePutLayout` uses `LayoutStore::load()` solely as an existence probe, then parses the request body again—extra bounded read + `String` churn on every PUT. | `WebPortal::_handlePutLayout` | +0.3 |
| MINOR | `POST …/activate` returns `200 {ok:true,data:{active_id}}` after NVS persist + `onManualSwitch`; there is **no** coupling to `ModeRegistry::tick()` success, heap guard, or orchestrator priority—client can observe “success” while display remains non-`custom_layout` until polled elsewhere. | `_handlePostLayoutActivate` | +0.3 |
| MINOR | Widget field metadata uses `kind` / `options` shapes that differ from `GET /api/display/modes` per-mode `settings` (`type` / `enumOptions`)—editor must special-case two JSON dialects (integration tax). | `_handleGetWidgetTypes` vs `_handleGetDisplayModes` | +0.3 |
| MINOR | “SYNC-RISK” comment documents tri-source drift but does not enforce it (no `static_assert`, no shared table, no unit test comparing counts/strings)—debt remains operational. | `_handleGetWidgetTypes` comment block | +0.3 |
| MINOR | Security model unchanged: unauthenticated LAN client can CRUD layouts + activate (expected product choice, still a blast-radius item if LAN is shared). | Architecture / threat model | +0.3 |
| CLEAN PASS | No Python / PEP8 surface in story scope | N/A | −0.5 |
| CLEAN PASS | No SQL/command injection vectors in shown handlers (JSON + id safety via `LayoutStore::isSafeLayoutId`) | N/A | −0.5 |
| CLEAN PASS | No obvious heap-unbounded `readString()` in WebPortal handlers themselves (size gate + `LayoutStore` caps) | N/A | −0.5 |

### Evidence score: **2.2**

| Score | Verdict |
|-------|---------|
| **2.2** | **APPROVED** (below 3.0 threshold; not EXEMPLARY) |

---

## Architectural sins

- **[6/10] Single Responsibility / adapter bloat:** `WebPortal.cpp` now centralizes routing, chunked body accumulation, **and** a large static widget-schema DSL (`_handleGetWidgetTypes`). That is a second product API contract living in the HTTP adapter instead of `WidgetRegistry` / a shared descriptor table.  
  - **Fix:** Promote a single `const` descriptor table consumed by `LayoutStore`, `WidgetRegistry`, and serialization (LE-1.7 milestone already hinted in comments).

- **[5/10] Dependency direction:** AC #7 explicitly names `WidgetRegistry` as the authority, but the implementation inverts dependency (adapter owns truth). This is exactly the “wrong abstraction owner” failure mode for long-term editor work.

---

## Pythonic crimes & readability

- **N/A (C++ / Arduino):** No PEP8 scope. Minor C++ readability: `_handleGetWidgetTypes` is long and repetitive; acceptable for now but hostile to review diffs.

---

## Performance & scalability

- **[Low–medium] Redundant I/O on PUT:** existence check via `load()` pulls up to 8 KiB into a `String` then the handler parses the HTTP body separately—double work on a constrained MCU under editor “typematic” saves.  
  - **Fix:** `LayoutStore::exists(id)` or `open` + `size` check without materializing JSON twice.

---

## Correctness & safety

- **Orchestrator vs HTTP “success” semantics:** `activate` cannot truthfully report “display is now showing this layout” in one response; it only proves NVS + intent signaling. That is not a crash bug, but it **is** a contract honesty bug for automation/clients that treat HTTP 200 as end-state.

- **Shared `g_pendingBodies` vector:** all JSON POST endpoints reuse the same pending-body table keyed by `AsyncWebServerRequest*`. The pattern matches existing settings/mode/schedule routes, so it is probably fine, but it increases blast radius: any future bug in `clearPendingBody` / disconnect handling affects unrelated endpoints.

---

## Lying tests

- **`test_web_portal_instantiates`:** Instantiating `WebPortal` does not register routes or execute handlers—proves linkage, not behavior.  
- **`test_widget_types_compiles` / several `TEST_PASS()` cases:** Explicitly admit “compile-only”; they **do not** falsify the claim “every route handler exercised” from a strict reading of AC #9.

---

## Maintainability issues

- **Tri-list sync debt:** `kAllowedWidgetTypes` + `WidgetRegistry::fromString` + `_handleGetWidgetTypes` must stay aligned; comment mitigates nothing at compile time—this is a classic maintenance grenade for LE-1.7.

---

## Suggested fixes (no auto-apply; read-only review)

1. **Re-home widget type introspection** behind a single source of truth (even if it is `constexpr` arrays in `WidgetRegistry.cpp` referenced by WebPortal) to satisfy AC #7 literally and reduce drift.

2. **Replace compile-only “handler exercised” language** in AC #9 *or* add a thin test seam (free functions taking `String url` + `String body`) extractable from handlers for deterministic unit tests without `AsyncWebServerRequest`.

3. **Add `LayoutStore::exists` / cheap stat** and use it in PUT to avoid redundant `load()` + default JSON fill on the miss path noise.

4. **Extend activate response** (optional) with `switch_state` / `orchestrator_state` snapshot similar to `POST /api/display/mode`, so clients do not need a second poll to detect “scheduled mode blocked manual custom_layout”.

---

**Issues fixed:** 0 (read-only validator)  
**Action items created:** 4 (see Suggested fixes)

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

# 🔥 ADVERSARIAL CODE REVIEW: Story LE-1.4 (layouts-rest-api)

**Reviewer:** AI Code Review Agent  
**Story:** le-1-4-layouts-rest-api  
**Date:** 2026-04-17  
**Target:** Layouts REST API endpoints + widget introspection  

---

## 📊 Review Summary

| Category | Count | Status |
|----------|-------|--------|
| **AC Implementation Issues** | 3 | 🔴 CRITICAL |
| **Task Completion Fraud** | 1 | 🔴 CRITICAL |
| **SOLID Violations** | 2 | 🟠 HIGH |
| **Hidden Bugs** | 2 | 🟠 HIGH |
| **Missing Security Checks** | 2 | 🟠 HIGH |
| **Test Coverage Gaps** | 2 | 🟠 HIGH |
| **Performance Issues** | 1 | 🟡 MEDIUM |
| **Architecture Violations** | 1 | 🟠 HIGH |
| **Total Issues Found** | **14** | 🔥 **MAJOR REWORK** |

---

## 🔴 CRITICAL FINDINGS

### 1. **AC #1 Response Shape Completely Undocumented in Story Text**
- **Severity:** 🔴 CRITICAL  
- **File:** `_bmad-output/implementation-artifacts/stories/le-1-4-layouts-rest-api.md` line 56  
- **Issue:** AC #1 states: `{ok: true, data: {active_id: "<id-or-empty>", layouts: [{id, name, bytes}, ...]}}` but the actual story text in Acceptance Criteria section only documents the old shape. The implementation ships the RICHER response envelope, but the story AC reads like a plain array.
- **Evidence:** 
  - Story AC #1: "returns `{ok: true, data: {active_id: "<id-or-empty>", layouts: [{id, name, bytes}, ...]}}`"
  - WebPortal.cpp line 1202-1224: Implementation correctly builds `active_id` and `layouts` array
  - BUT: Developer Record (v1.2) explicitly states "AC #1 rewritten" — meaning AFTER the story was marked done, the AC text was fixed
- **Problem:** This is AC DRIFT between the story document and the actual shipped behavior. The story file itself contains the MISLEADING text that led to the confusion.
- **Fix Required:** Update the story AC text to match the shipped implementation, OR revert implementation to match the documented (incorrect) AC.

### 2. **Task 11 (API Spec Document) Marked [x] Complete BUT the document is in the File List as "created"**
- **Severity:** 🔴 CRITICAL  
- **File:** Story file line 156, Change Log v1.1 line 3  
- **Issue:** Task 11 says "Create `_bmad-output/planning-artifacts/api-layouts-spec.md`" and is marked [x] complete. The file IS present (file id: 2e1ee978). HOWEVER, this is a CLASSIC case of "task completion fraud" — the spec document was created AFTER the story was marked done (per the Change Log: "v1.1 — Upgraded to ready-for-dev" but then "v1.1 → v1.2 implementation complete"). The story was "done" BEFORE the spec doc existed, violating AC #9 which requires the spec before API handlers ship.
- **Evidence:**
  - Task 11 appears in "Tasks / Subtasks" section, marked [x]
  - Change Log shows progression: draft → ready-for-dev → implementation complete
  - The api-layouts-spec.md file exists NOW, but was created as a follow-up after story marked done
- **Architectural Violation:** AC #11 implicitly requires the spec to guide implementation. Creating it AFTER code is complete is exactly backwards.
- **Fix:** Mark Task 11 as a follow-up [AI-Review] item, not part of the original story completion.

### 3. **AC #9 (Integration Tests) Claims "compile-only acceptable" BUT Tests Are Actually Lying Tests**
- **Severity:** 🔴 CRITICAL  
- **File:** `firmware/test/test_web_portal/test_main.cpp` lines 55-72  
- **Issue:** The test file explicitly documents "compile-only verification" and AC #9 says "compile-only acceptable per the story Dev Notes". BUT this is DECEPTIVE:
  - `test_web_portal_instantiates()` just creates a WebPortal object and does nothing with it
  - `test_delete_active_guard_path()` calls `LayoutStore::getActiveId()` but never asserts anything
  - `test_activate_orchestrator_handoff_compiles()` gets a function pointer and calls `TEST_ASSERT_NOT_NULL` on a compile-time-constant address (will always pass)
  - These are LYING TESTS that prove "code compiles" but prove NOTHING about correctness
  - The story AC #9 is satisfied by the LETTER (tests exist and compile) but NOT the SPIRIT (no actual behavior verification)
- **Evidence:**
  - No test actually exercises the LayoutStore CRUD methods
  - No test exercises the WebPortal handlers with mock requests
  - Test coverage: 0% of actual API behavior
- **Problem:** The review Find (v1.2) explicitly states tests are "compile-only per AC #9" as if this is a valid defense. It's not. Compile-only is a LOWER bar, not a FREE PASS.
- **What Should Happen:** Either (a) implement real mocked request tests, or (b) acknowledge in the story that AC #9 is "compile-time verification only" with zero runtime correctness proof.

---

## 🔴 CRITICAL IMPLEMENTATION GAPS

### 4. **AC #3 Completely Missing: POST /api/layouts Rejects Size >8KB**
- **Severity:** 🟠 HIGH (but blocks AC #3 completion)  
- **File:** `firmware/adapters/WebPortal.cpp` lines 365-400  
- **Issue:** The story body handler for POST /api/layouts accumulates the full request body before validation. The guard `if (total > LAYOUT_STORE_MAX_BYTES)` appears at line 373 BUT:
  - The `clearPendingBody()` is called BEFORE the error is sent, which is correct
  - BUT there is NO EARLY SIZE REJECTION on the first chunk
  - An attacker (or misbehaving client) can send a 100MB request before the server rejects it
  - The guard only prevents it from being BUFFERED, not from being SENT
  - **Critical:** On a memory-constrained device (280KB heap), this could DoS the server by forcing the parser to spend CPU rejecting an oversized body
- **Evidence:** WebPortal.cpp line 373 checks `total > LAYOUT_STORE_MAX_BYTES` but does NOT reject the request immediately in the body accumulation loop
- **Expected:** HTTP 413 should be sent immediately when `total > 8KB` is detected, before the body handler is even invoked
- **Actual:** Body accumulation handler does the check, but it's called for every chunk, and the response is only sent at the end
- **Fix:** Add early rejection in the on-receive handler before body accumulation starts OR set Content-Length validation at the web server layer
- **Proof:** AC #3 explicitly says "Reject `total > LAYOUT_STORE_MAX_BYTES` immediately with 413"

### 5. **AC #8 Never Tested: FS_FULL Error Path Unreachable in Tests**
- **Severity:** 🟠 HIGH  
- **File:** `test_web_portal/test_main.cpp` — NO TEST for FS_FULL path  
- **Issue:** AC #8 states "Given filesystem capacity is exhausted When any write endpoint fails with LayoutStoreError::FS_FULL Then the response is {ok: false, error: "Filesystem full", code: "FS_FULL"}". However:
  - There is NO test case in test_web_portal that exercises the FS_FULL error path
  - The test file only has 6 tests, none of which mock LayoutStore to return FS_FULL
  - AC #8 acceptance is based on ASSUMPTION, not verification
- **Evidence:** Grep through test_main.cpp shows only generic "compile-only" tests; no FS_FULL scenario
- **Fix:** Add `test_post_layout_fs_full_returns_507` that mocks LayoutStore::save() to return LayoutStoreError::FS_FULL
- **Risk:** The actual WebPortal handler at line 379-382 will never be tested in the current test suite, making AC #8 unverified

### 6. **AC #6 (Activate Endpoint) Missing Rule #24 Validation in Tests**
- **Severity:** 🟠 HIGH  
- **File:** `firmware/adapters/WebPortal.cpp` line 2044 + Story AC #6  
- **Issue:** AC #6 requires "POST /api/layouts/:id/activate calls `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")`" per Rule #24. The implementation does this correctly (line 2044). BUT:
  - The story never documents WHAT HAPPENS if `ModeOrchestrator::onManualSwitch()` fails or if the mode doesn't exist
  - The handler assumes "custom_layout" mode is always registered
  - What if someone deletes CustomLayoutMode from MODE_TABLE but leaves an active layout pointing to it?
  - No error handling, no fallback, just calls onManualSwitch and continues
- **Evidence:** Line 2044: `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout");` — no null check, no error handling
- **Problem:** AC #6 is "satisfied" mechanically but not ROBUSTLY
- **Real Risk:** If CustomLayoutMode is ever removed or MODE_TABLE is modified, this endpoint silently switches to a non-existent mode
- **Fix:** Add validation that "custom_layout" is actually in the MODE_TABLE before calling onManualSwitch, or return 500 if it's not found

---

## 🟠 HIGH SEVERITY FINDINGS

### 7. **SOLID Violation: Single Responsibility — WebPortal Handler God Class**
- **Severity:** 🟠 HIGH  
- **File:** `firmware/adapters/WebPortal.cpp` lines 1-2500+  
- **Issue:** The WebPortal class has grown to over 2500 lines with 30+ handler methods, all doing:
  - URL parsing (extract ID from path)
  - Request body parsing (accumulation, size validation)
  - Domain logic (LayoutStore calls)
  - Response JSON building
  - Error mapping
  - Each handler is duplicating URL extraction logic (`extractLayoutIdFromUrl` helper is defined but not used consistently)
- **Evidence:**
  - `_handleGetLayouts` (lines 1200-1224) is 24 lines
  - `_handlePostLayout` (lines 1252-1280) is 28 lines
  - `_handlePutLayout` (lines 1296-1330) is 34 lines
  - `_handleDeleteLayout` (lines 1336-1355) is 19 lines
  - All follow identical pattern: extract ID → validate → call LayoutStore → build response
- **Architectural Flaw:** This is a classic adapter bloat — the HTTP request/response layer is intertwined with domain logic
- **Fix:** Extract a LayoutController class that owns all layout domain operations, and have WebPortal route to it
- **Impact:** Future changes (new layout fields, new validations) will require touching WebPortal instead of a focused domain class

### 8. **Missing Security Check: AC #6 Activate Doesn't Validate Layout ID Ownership**
- **Severity:** 🟠 HIGH  
- **File:** `firmware/adapters/WebPortal.cpp` line 2030  
- **Issue:** The activate endpoint checks `isSafeLayoutId()` but does NOT verify that the requesting user has permission to activate a layout. In a multi-user scenario (unlikely for ESP32, but architecturally wrong):
  - User A could activate User B's private layout
  - User A could activate a layout marked "internal-use-only"
  - No audit trail, no permission model
- **Current State:** The code assumes single-user (device owner). No access control.
- **Missing:** At minimum, should document that this is single-user only and layouts are world-activatable
- **Evidence:** Line 2030-2044 has no permission check, no ownership validation, just loads and activates
- **Fix:** Add comment documenting single-user assumption, or implement layout ownership/ACL if multi-user is ever needed

### 9. **SOLID Violation: Open/Closed — Widget Types Hardcoded in Three Places**
- **Severity:** 🟠 HIGH  
- **Files:** 
  - `firmware/core/LayoutStore.cpp` (kAllowedWidgetTypes array)
  - `firmware/core/WidgetRegistry.cpp` (fromString switch)
  - `firmware/adapters/WebPortal.cpp` lines 2065-2150 (_handleGetWidgetTypes static response)
- **Issue:** The 5 widget types ("text", "clock", "logo", "flight_field", "metric") are defined in three separate places. Adding a new widget type requires changes in all three files:
  - Failure to update one location silently breaks the system (e.g., LayoutStore accepts a type that WidgetRegistry can't render)
  - The story Dev Notes (line ~2060) explicitly flag this: "SYNC-RISK NOTE" and "Consolidate in LE-1.7"
  - This is not deferred-by-design; it's a KNOWN BUG that can only be fixed in LE-1.7
- **Evidence:** The code ITSELF documents this risk (see SYNC-RISK comment in WebPortal.cpp)
- **Problem:** The story acknowledges the problem but doesn't fix it. By shipping code with documented sync risks, we're shipping a known time bomb.
- **Fix:** Either (a) consolidate widget types into a single source of truth, or (b) add a compile-time assertion that all three match

### 10. **Hidden Bug: PUT /api/layouts/:id Body ID Mismatch Error Message Is Confusing**
- **Severity:** 🟠 HIGH  
- **File:** `firmware/adapters/WebPortal.cpp` line 1312-1318  
- **Issue:** The Dev Agent Record (v1.2) explicitly notes: "PUT handler: split missing-id vs mismatched-id error messages". The implementation does this (two separate error cases), BUT:
  - Case 1: Missing `doc["id"]` → 400 "Missing id field in layout body"
  - Case 2: `doc["id"]` doesn't match URL → 400 "Body id does not match URL id"
  - But the error handling is INCONSISTENT with POST /api/layouts
  - POST allows the ID to be in the body ONLY (line 1265: `const char* id = doc["id"]`)
  - PUT requires BOTH the URL id AND the body id AND they must match (line 1308-1318)
  - This is ASYMMETRIC and SURPRISING for API clients
- **Evidence:**
  - POST at line 1265: `const char* id = doc["id"]` — takes ID from JSON ONLY
  - PUT at line 1310: Extracts URL id, then validates `doc["id"] == URL id`
  - Not documented in the API spec or story AC why PUT requires this dual validation
- **Fix:** Document the asymmetry OR make both endpoints consistent (POST should also require URL/:id to match body)
- **Real Impact:** A developer creating a layout via POST with POST /api/layouts (no :id in URL), then trying to update via PUT /api/layouts/:id with a different ID in the body will get a confusing error

---

## 🟡 MEDIUM SEVERITY FINDINGS

### 11. **Performance Issue: GET /api/layouts Lists All Layouts Every Time (No Caching)**
- **Severity:** 🟡 MEDIUM  
- **File:** `firmware/adapters/WebPortal.cpp` line 1200-1224  
- **Issue:** The GET /api/layouts handler calls `LayoutStore::list(entries)` on every request. This:
  - Opens the `/layouts/` directory on LittleFS (blocking I/O, ~10-50ms per call)
  - Reads directory entries
  - Iterates and reads file metadata for each layout
  - Builds a new JSON response from scratch
  - No caching, no etag, no "only-if-modified" support
  - On a device with 16 layouts, this could be 50-100ms per request
- **Evidence:** Synchronous `LayoutStore::list()` call with no caching layer
- **Impact:** A dashboard polling for layout changes will hammer the filesystem
- **Fix:** Cache the result with a 1-second TTL, or add an etag/modification-time header so clients can do conditional requests

### 12. **Test Coverage Gap: AC #4 (PUT Overwrite) Not Tested**
- **Severity:** 🟡 MEDIUM  
- **File:** `test_web_portal/test_main.cpp` — no PUT test  
- **Issue:** AC #4 requires "Given an existing id When PUT /api/layouts/:id is called Then the layout is overwritten". However:
  - There is NO test case `test_put_layout_overwrites_existing`
  - The test file has 6 tests, none of which exercise PUT behavior
  - AC #4 is accepted based on code inspection only, not test verification
- **Fix:** Add `test_put_layout_existing_overwrites_success` that creates a layout, updates it, verifies new content
- **Risk:** Future refactoring could break PUT without test failure detection

---

## 🟠 HIGH SEVERITY ARCHITECTURE VIOLATION

### 13. **Architecture Violation: Dependencies Flow Upward (Inverse of Hexagonal Pattern)**
- **Severity:** 🟠 HIGH  
- **File:** `firmware/adapters/WebPortal.cpp` includes:
  - `#include "core/LayoutStore.h"` (line ~50)
  - `#include "core/ModeOrchestrator.h"` (line ~48)
  - `#include "core/ConfigManager.h"` (line ~49)
- **Issue:** WebPortal (an adapter) is directly calling core business logic:
  - `LayoutStore::list()` — domain object
  - `LayoutStore::load()` — domain object
  - `LayoutStore::save()` — domain object
  - `ModeOrchestrator::onManualSwitch()` — core orchestration
  - This violates the hexagonal pattern where adapters should route to ports, not directly to core
- **Proper Pattern:** WebPortal should call a port interface (LayoutService or LayoutUseCase), which then delegates to the core
- **Current State:** WebPortal is tightly coupled to LayoutStore implementation
- **Fix:** Create an ILayoutService interface and have WebPortal depend on it, not on LayoutStore directly
- **Impact:** Changing LayoutStore's API requires changing WebPortal, violating separation of concerns

---

## 📝 Additional Findings

### 14. **Documentation Gap: No Schema Validation for Layout JSON**
- **Severity:** 🟡 MEDIUM  
- **File:** `firmware/adapters/WebPortal.cpp` line 1254-1280  
- **Issue:** The POST /api/layouts handler calls `LayoutStore::save(id, jsonStr)` which presumably validates the JSON schema. However, the WebPortal handler doesn't document what that validation is OR what happens on invalid schema.
- **AC #3 says:** "invalid JSON or schema returns `LAYOUT_INVALID`"
- **Problem:** The AC doesn't specify WHAT "schema" validation means. What fields are required? What are the constraints?
- **Evidence:** The story AC #3 mentions schema validation but never defines it
- **Fix:** Document the layout JSON schema in the story's Dev Notes or in the api-layouts-spec.md

---

## 📊 Evidence Score Calculation

| Finding | Severity | Score |
|---------|----------|-------|
| AC #1 Response Drift (critical claim mismatch) | 🔴 CRITICAL | +3.0 |
| Task 11 Completion Fraud (task marked done before work) | 🔴 CRITICAL | +3.0 |
| AC #9 Lying Tests (compile-only ≠ correctness) | 🔴 CRITICAL | +3.0 |
| AC #3 Size Validation Gap | 🟠 HIGH | +1.0 |
| AC #8 FS_FULL Path Untested | 🟠 HIGH | +1.0 |
| AC #6 Missing Robustness | 🟠 HIGH | +1.0 |
| SOLID: God Class WebPortal | 🟠 HIGH | +1.0 |
| Security: No Ownership Validation | 🟠 HIGH | +1.0 |
| SOLID: Widget Type Sync Bomb | 🟠 HIGH | +1.0 |
| PUT ID Mismatch Asymmetry | 🟠 HIGH | +1.0 |
| Performance: No Caching | 🟡 MEDIUM | +0.3 |
| Test Gap: PUT Not Tested | 🟡 MEDIUM | +0.3 |
| Architecture: Inverted Dependencies | 🟠 HIGH | +1.0 |
| Documentation: Schema Undefined | 🟡 MEDIUM | +0.3 |
| **Total Evidence Score** | | **18.6** |
| Clean Pass Categories | 0 | -0.0 |
| **Final Evidence Score** | | **18.6** |

---

## 🏁 VERDICT

| Score | Verdict | Recommendation |
|-------|---------|-----------------|
| **18.6** | 🔴 **REJECT - MAJOR REWORK** | Story is INCOMPLETE. 3 critical issues block acceptance: AC drift, task completion fraud, and lying test suite. 11 additional high/medium issues require fixes. |

---

## ✅ Summary for Synthesis

**Issues Found:** 14 (3 CRITICAL, 10 HIGH, 1 MEDIUM)  
**Pass Rate:** 0% (critical blockers present)  
**Recommendation:** REJECT — Do not merge. Requires:
1. Fix AC #1 drift between story text and implementation
2. Validate Task #11 actually completed as claimed
3. Implement REAL tests for AC #9 or document "compile-only" as a limitation
4. Address security/robustness gaps in AC #6
5. Resolve SOLID violations (God Class, inverted dependencies)

**Story Status:** ❌ INCOMPLETE

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 1 |
| SOLID Violations | 1 |
| Hidden Bugs | 1 |
| Performance Footguns | 1 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **5** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|----------------|
| 🟠 IMPORTANT | AC #7/9: Widget metadata sync-risk and test coverage | `WebPortal.cpp`, `test_main.cpp` | +1.0 |
| 🟠 IMPORTANT | SOLID: Dispatch responsibility breach in WebPortal | `WebPortal.cpp` | +1.0 |
| 🟡 MINOR | Hidden Bug: Body handler race condition vulnerability | `WebPortal.cpp` | +0.3 |
| 🟡 MINOR | Performance: Static JSON builder heap usage | `WebPortal.cpp` | +0.3 |
| 🟡 MINOR | Task Completion: AC#9 test coverage theater | `test_main.cpp` | +0.3 |
| 🟢 CLEAN PASS | 5 | (Style, Type Safety, Security, Git vs Story, Architecture) | -2.5 |

### Evidence Score: 0.4

| Score | Verdict |
|-------|---------|
| **0.4** | **APPROVED** |

---

## 🏛️ Architectural Sins

- **[6/10] Single Responsibility Breach:** `WebPortal` has become a "Fat Controller" and a schema registry. Hardcoding widget field metadata inside a network-adapter class (`WebPortal`) violates Dependency Inversion — the adapter should ideally receive a widget-schema registry or descriptor table from the core, not own the metadata definition.
  - 📍 `firmware/adapters/WebPortal.cpp:2060`
  - 💡 Fix: Consolidate widget schema in `WidgetRegistry` as a static descriptor table and export that table to `WebPortal` for JSON serialization.

---

## ⚡ Performance & Scalability

- **[LOW] Static JSON builder heap usage:** The `_handleGetWidgetTypes` handler builds a large JSON response using `JsonDocument` in a single scope. While v7 is efficient, this is a large static payload that is re-serialized on every GET request.
  - 📍 `firmware/adapters/WebPortal.cpp:2060`
  - 💡 Fix: Pre-serialize the static schema to a `static const char*` buffer in `WidgetRegistry` and serve it directly, avoiding runtime JSON construction.

---

## 🐛 Correctness & Safety

- **🐛 Body Handler Race Condition:** The `_handlePostLayout` and `_handlePutLayout` body handlers use a `g_pendingBodies` vector. If an `onDisconnect` event is fired while the handler is mid-accumulation, it is possible for `clearPendingBody` to be called prematurely, leading to a state where `findPendingBody` returns null or an incorrect request context.
  - 📍 `firmware/adapters/WebPortal.cpp:220`
  - 🔄 Reproduction: High-latency network during multipart JSON body streaming, triggering client-side disconnects.

---

## 🔧 Maintainability Issues

- **💣 Test Coverage Theater:** The story claims AC #9 (integration tests) is complete, but `test_main.cpp` is purely compile-time verification that symbols exist. It performs zero actual functional testing of the layout API logic.
  - 📍 `firmware/test/test_web_portal/test_main.cpp`
  - 💥 Explosion radius: High — functional bugs in CRUD logic will bypass CI, surfacing only on hardware.

- **💣 Widget Metadata Sync-Risk:** Widget field metadata is hard-coded in three disparate places (`WebPortal.cpp`, `LayoutStore.cpp`, `WidgetRegistry.cpp`). Adding a new field to a widget type requires three-way synchronization; failing to do so breaks the editor UI.
  - 📍 `firmware/adapters/WebPortal.cpp:2060`
  - 💥 Explosion radius: High — UI drift and broken form validation for new features.

---

## 🛠️ Suggested Fixes

### 1. Consolidate Widget Schema

**File:** `firmware/core/WidgetRegistry.h`
**Issue:** Metadata sync-risk and SRP violation.

**Corrected code (Conceptual):**
```cpp
// core/WidgetRegistry.h
struct WidgetFieldSpec {
    const char* key;
    const char* kind;
    const char* defaultValue;
};
struct WidgetSpec {
    WidgetType type;
    const char* label;
    const WidgetFieldSpec* fields;
    uint8_t fieldCount;
};
// Export the registry table to WebPortal
extern const WidgetSpec WIDGET_REGISTRY_TABLE[];
```

---

**Issues Fixed:** 0
**Action Items Created:** 0

]]></file>
</context>
<variables>
<var name="author">BMad</var>
<var name="communication_language">English</var>
<var name="date">2026-04-17</var>
<var name="description">Master synthesizes code review findings and applies fixes to source code</var>
<var name="document_output_language">English</var>
<var name="epic_num">le-1</var>
<var name="implementation_artifacts">_bmad-output/implementation-artifacts</var>
<var name="installed_path">_bmad/bmm/workflows/4-implementation/code-review-synthesis</var>
<var name="instructions">/Users/christianlee/App-Development/bmad-assist/src/bmad_assist/workflows/code-review-synthesis/instructions.xml</var>
<var name="name">code-review-synthesis</var>
<var name="output_folder">_bmad-output</var>
<var name="planning_artifacts">_bmad-output/planning-artifacts</var>
<var name="project_context" file_id="ed7fe483" load_strategy="EMBEDDED" token_approx="712">embedded in prompt, file id: ed7fe483</var>
<var name="project_knowledge">docs</var>
<var name="project_name">TheFlightWall_OSS-main</var>
<var name="reviewer_count">3</var>
<var name="session_id">0833d9a0-5547-4ae0-a073-377dc15b6a7b</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="4b1bc60f">embedded in prompt, file id: 4b1bc60f</var>
<var name="story_id">le-1.4</var>
<var name="story_key">le-1-4-layouts-rest-api</var>
<var name="story_num">4</var>
<var name="story_title">4-layouts-rest-api</var>
<var name="template">False</var>
<var name="timestamp">20260417_1502</var>
<var name="user_name">Christian</var>
<var name="user_skill_level">expert</var>
<var name="validator_count"></var>
</variables>
<instructions><workflow>
  <critical>Communicate all responses in English and generate all documents in English</critical>
  <critical>You are the MASTER SYNTHESIS agent for CODE REVIEW findings.</critical>
  <critical>You have WRITE PERMISSION to modify SOURCE CODE files and story Dev Agent Record section.</critical>
  <critical>DO NOT modify story context (AC, Dev Notes content) - only Dev Agent Record (task checkboxes, completion notes, file list).</critical>
  <critical>All context (project_context.md, story file, anonymized reviews) is EMBEDDED below - do NOT attempt to read files.</critical>

  <step n="1" goal="Analyze reviewer findings">
    <action>Read all anonymized reviewer outputs (Reviewer A, B, C, D, etc.)</action>
    <action>For each issue raised:
      - Cross-reference with embedded project_context.md and story file
      - Cross-reference with source code snippets provided in reviews
      - Determine if issue is valid or false positive
      - Note reviewer consensus (if 3+ reviewers agree, high confidence issue)
    </action>
    <action>Issues with low reviewer agreement (1-2 reviewers) require extra scrutiny</action>
    <action>Group related findings that address the same underlying problem</action>
  </step>

  <step n="1.5" goal="Review Deep Verify code analysis" conditional="[Deep Verify Findings] section present">
    <critical>Deep Verify analyzed the actual source code files for this story.
      DV findings are based on static analysis patterns and may identify issues reviewers missed.</critical>

    <action>Review each DV finding:
      - CRITICAL findings: Security vulnerabilities, race conditions, resource leaks - must address
      - ERROR findings: Bugs, missing error handling, boundary issues - should address
      - WARNING findings: Code quality concerns - consider addressing
    </action>

    <action>Cross-reference DV findings with reviewer findings:
      - DV + Reviewers agree: High confidence issue, prioritize in fix order
      - Only DV flags: Verify in source code - DV has precise line numbers
      - Only reviewers flag: May be design/logic issues DV can't detect
    </action>

    <action>DV findings may include evidence with:
      - Code quotes (exact text from source)
      - Line numbers (precise location, when available)
      - Pattern IDs (known antipattern reference)
      Use this evidence when applying fixes.</action>

    <action>DV patterns reference:
      - CC-*: Concurrency issues (race conditions, deadlocks)
      - SEC-*: Security vulnerabilities
      - DB-*: Database/storage issues
      - DT-*: Data transformation issues
      - GEN-*: General code quality (null handling, resource cleanup)
    </action>
  </step>

  <step n="2" goal="Verify issues and identify false positives">
    <action>For each issue, verify against embedded code context:
      - Does the issue actually exist in the current code?
      - Is the suggested fix appropriate for the codebase patterns?
      - Would the fix introduce new issues or regressions?
    </action>
    <action>Document false positives with clear reasoning:
      - Why the reviewer was wrong
      - What evidence contradicts the finding
      - Reference specific code or project_context.md patterns
    </action>
  </step>

  <step n="3" goal="Prioritize by severity">
    <action>For verified issues, assign severity:
      - Critical: Security vulnerabilities, data corruption, crashes
      - High: Bugs that break functionality, performance issues
      - Medium: Code quality issues, missing error handling
      - Low: Style issues, minor improvements, documentation
    </action>
    <action>Order fixes by severity - Critical first, then High, Medium, Low</action>
    <action>For disputed issues (reviewers disagree), note for manual resolution</action>
  </step>

  <step n="4" goal="Apply fixes to source code">
    <critical>This is SOURCE CODE modification, not story file modification</critical>
    <critical>Use Edit tool for all code changes - preserve surrounding code</critical>
    <critical>After applying each fix group, run: pytest -q --tb=line --no-header</critical>
    <critical>NEVER proceed to next fix if tests are broken - either revert or adjust</critical>

    <action>For each verified issue (starting with Critical):
      1. Identify the source file(s) from reviewer findings
      2. Apply fix using Edit tool - change ONLY the identified issue
      3. Preserve code style, indentation, and surrounding context
      4. Log the change for synthesis report
    </action>

    <action>After each logical fix group (related changes):
      - Run: pytest -q --tb=line --no-header
      - If tests pass, continue to next fix
      - If tests fail:
        a. Analyze which fix caused the failure
        b. Either revert the problematic fix OR adjust implementation
        c. Run tests again to confirm green state
        d. Log partial fix failure in synthesis report
    </action>

    <action>Atomic commit guidance (for user reference):
      - Commit message format: fix(component): brief description (synthesis-le-1.4)
      - Group fixes by severity and affected component
      - Never commit unrelated changes together
      - User may batch or split commits as preferred
    </action>
  </step>

  <step n="5" goal="Refactor if needed">
    <critical>Only refactor code directly related to applied fixes</critical>
    <critical>Maximum scope: files already modified in Step 4</critical>

    <action>Review applied fixes for duplication patterns:
      - Same fix applied 2+ times across files = candidate for refactor
      - Only if duplication is in files already modified
    </action>

    <action>If refactoring:
      - Extract common logic to shared function/module
      - Update all call sites in modified files
      - Run tests after refactoring: pytest -q --tb=line --no-header
      - Log refactoring in synthesis report
    </action>

    <action>Do NOT refactor:
      - Unrelated code that "could be improved"
      - Files not touched in Step 4
      - Patterns that work but are just "not ideal"
    </action>

    <action>If broader refactoring needed:
      - Note it in synthesis report as "Suggested future improvement"
      - Do not apply - leave for dedicated refactoring story
    </action>
  </step>

  <step n="6" goal="Generate synthesis report">
    <critical>When updating story file, use atomic write pattern (temp file + rename).</critical>
    <action>Update story file Dev Agent Record section ONLY:
      - Mark completed tasks with [x] if fixes address them
      - Append to "Completion Notes List" subsection summarizing changes applied
      - Update file list with all modified files
    </action>

    <critical>Your synthesis report MUST be wrapped in HTML comment markers for extraction:</critical>
    <action>Produce structured output in this exact format (including the markers):</action>
    <output-format>
&lt;!-- CODE_REVIEW_SYNTHESIS_START --&gt;
## Synthesis Summary
[Brief overview: X issues verified, Y false positives dismissed, Z fixes applied to source files]

## Validations Quality
[For each reviewer: ID (A, B, C...), score (1-10), brief assessment]
[Note: Reviewers are anonymized - do not attempt to identify providers]

## Issues Verified (by severity)

### Critical
[Issues that required immediate fixes - list with evidence and fixes applied]
[Format: "- **Issue**: Description | **Source**: Reviewer(s) | **File**: path | **Fix**: What was changed"]
[If none: "No critical issues identified."]

### High
[Bugs and significant problems - same format]

### Medium
[Code quality issues - same format]

### Low
[Minor improvements - same format, note any deferred items]

## Issues Dismissed
[False positives with reasoning for each dismissal]
[Format: "- **Claimed Issue**: Description | **Raised by**: Reviewer(s) | **Dismissal Reason**: Why this is incorrect"]
[If none: "No false positives identified."]

## Changes Applied
[Complete list of modifications made to source files]
[Format for each change:
  **File**: [path/to/file.py]
  **Change**: [Brief description]
  **Before**:
  ```
  [2-3 lines of original code]
  ```
  **After**:
  ```
  [2-3 lines of updated code]
  ```
]
[If no changes: "No source code changes required."]

## Deep Verify Integration
[If DV findings were present, document how they were handled]

### DV Findings Fixed
[List DV findings that resulted in code changes]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **File**: {path} | **Fix**: {What was changed}"]

### DV Findings Dismissed
[List DV findings determined to be false positives]
[Format: "- **{ID}** [{SEVERITY}]: {Title} | **Reason**: {Why this is not an issue}"]

### DV-Reviewer Overlap
[Note findings flagged by both DV and reviewers - highest confidence fixes]
[If no DV findings: "Deep Verify did not produce findings for this story."]

## Files Modified
[Simple list of all files that were modified]
- path/to/file1.py
- path/to/file2.py
[If none: "No files modified."]

## Suggested Future Improvements
[Broader refactorings or improvements identified in Step 5 but not applied]
[Format: "- **Scope**: Description | **Rationale**: Why deferred | **Effort**: Estimated complexity"]
[If none: "No future improvements identified."]

## Test Results
[Final test run output summary]
- Tests passed: X
- Tests failed: 0 (required for completion)
&lt;!-- CODE_REVIEW_SYNTHESIS_END --&gt;
    </output-format>

  </step>

  <step n="6.5" goal="Write Senior Developer Review section to story file for dev_story rework detection">
    <critical>This section enables dev_story to detect that a code review has occurred and extract action items.</critical>
    <critical>APPEND this section to the story file - do NOT replace existing content.</critical>

    <action>Determine the evidence verdict from the [Evidence Score] section:
      - REJECT: Evidence score exceeds reject threshold
      - PASS: Evidence score is below accept threshold
      - UNCERTAIN: Evidence score is between thresholds
    </action>

    <action>Map evidence verdict to review outcome:
      - PASS → "Approved"
      - REJECT → "Changes Requested"
      - UNCERTAIN → "Approved with Reservations"
    </action>

    <action>Append to story file "## Senior Developer Review (AI)" section:
      ```
      ## Senior Developer Review (AI)

      ### Review: {current_date}
      - **Reviewer:** AI Code Review Synthesis
      - **Evidence Score:** {evidence_score} → {evidence_verdict}
      - **Issues Found:** {total_verified_issues}
      - **Issues Fixed:** {fixes_applied_count}
      - **Action Items Created:** {remaining_unfixed_count}
      ```
    </action>

    <critical>When evidence verdict is REJECT, you MUST create Review Follow-ups tasks.
      If "Action Items Created" count is &gt; 0, there MUST be exactly that many [ ] [AI-Review] tasks.
      Do NOT skip this step. Do NOT claim all issues are fixed if you reported deferred items above.</critical>

    <action>Find the "## Tasks / Subtasks" section in the story file</action>
    <action>Append a "#### Review Follow-ups (AI)" subsection with checkbox tasks:
      ```
      #### Review Follow-ups (AI)
      - [ ] [AI-Review] {severity}: {brief description of unfixed issue} ({file path})
      ```
      One line per unfixed/deferred issue, prefixed with [AI-Review] tag.
      Order by severity: Critical first, then High, Medium, Low.
    </action>

    <critical>ATDD DEFECT CHECK: Search test directories (tests/**) for test.fixme() calls in test files related to this story.
      If ANY test.fixme() calls remain in story-related test files, this is a DEFECT — the dev_story agent failed to activate ATDD RED-phase tests.
      Create an additional [AI-Review] task:
      - [ ] [AI-Review] HIGH: Activate ATDD tests — convert all test.fixme() to test() and ensure they pass ({test file paths})
      Do NOT dismiss test.fixme() as "intentional TDD methodology". After dev_story completes, ALL test.fixme() tests for the story MUST be converted to test().</critical>
  </step>

  </workflow></instructions>
<output-template></output-template>
</compiled-workflow>