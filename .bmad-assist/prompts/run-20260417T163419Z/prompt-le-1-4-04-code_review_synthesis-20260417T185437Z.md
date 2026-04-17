<?xml version="1.0" encoding="UTF-8"?>
<!-- BMAD Prompt Run Metadata -->
<!-- Epic: le-1 -->
<!-- Story: 4 -->
<!-- Phase: code-review-synthesis -->
<!-- Timestamp: 20260417T185437Z -->
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

1. **Given** stored layouts **When** `GET /api/layouts` is called **Then** it returns `{ok: true, data: [{id, name, bytes}, ...]}` with one entry per layout on LittleFS; empty array when no layouts are saved.
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

### File List
Files modified:
- `firmware/adapters/WebPortal.h` — added 7 private handler declarations in a `// Layout CRUD + activation (LE-1.4)` block
- `firmware/adapters/WebPortal.cpp` — added `#include "core/LayoutStore.h"`, registered 7 routes in `_registerRoutes()`, implemented 7 handlers + `extractLayoutIdFromUrl` helper (~380 new lines)

Files created:
- `firmware/test/test_web_portal/test_main.cpp` — compile-only Unity test scaffolding (6 tests)
- `_bmad-output/planning-artifacts/api-layouts-spec.md` — REST API reference with envelope spec, HTTP status map, per-endpoint request/response/errors/curl examples


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


]]></file>
<file id="1d910d88" path="[Validator A]" label="VALIDATION OUTPUT"><![CDATA[

**Adversarial code review complete, Christian**

**Story:** `_bmad-output/implementation-artifacts/stories/le-1-4-layouts-rest-api.md`  
**Primary implementation:** `firmware/adapters/WebPortal.cpp`, `firmware/adapters/WebPortal.h`, `firmware/test/test_web_portal/test_main.cpp`, `_bmad-output/planning-artifacts/api-layouts-spec.md`

---

## Review summary

| Category | Issues found |
|----------|----------------|
| Git vs story discrepancies | Not verified (read-only; no `git diff` run) |
| AC implementation gaps | 2 |
| Task / AC honesty | 1 |
| SOLID violations | 1 |
| Hidden bugs | 0 concrete repro |
| Performance footguns | 2 |
| Security vulnerabilities | 0 (LAN trust model unchanged) |
| **Total issues** | **8** |

---

## Evidence score summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| CRITICAL | AC #9 claims every layout route handler is “exercised” by tests; `test_web_portal/test_main.cpp` never invokes `_handleGetLayouts`, `_handlePostLayout`, `_handlePutLayout`, `_handleDeleteLayout`, `_handlePostLayoutActivate`, `_handleGetLayoutById`, or `_handleGetWidgetTypes`. Tests only touch `WebPortal` construction, constants, and symbols. That is structural coverage theater, not route coverage. | `test_web_portal/test_main.cpp` vs story AC #9 | +3 |
| IMPORTANT | AC #1 wording: success body is a **top-level array** of `{id,name,bytes}`. Implementation nests under `data.layouts` and adds `data.active_id`. The planning spec (`api-layouts-spec.md`) matches the code, but the **story AC #1** does not—contract drift for any client written strictly from the story. | `WebPortal::_handleGetLayouts` vs AC #1 text | +1 |
| IMPORTANT | Story `zone` lists `firmware/core/WidgetRegistry.*`; story also framed AC #7 around `WidgetRegistry`. Implementation hard-codes widget metadata in `WebPortal::_handleGetWidgetTypes`, reintroducing the **dual-source sync** risk called out in le-1 antipatterns (`kAllowedWidgetTypes` / `WidgetRegistry::fromString` vs HTTP schema). | `WebPortal.cpp` `_handleGetWidgetTypes`, `WidgetRegistry.h` | +1 |
| IMPORTANT | `PUT /api/layouts/:id` existence check calls `LayoutStore::load`, which reads and parses up to **8 KiB** (and fills a `String`) only to learn “file exists.” Correctness OK; cost and heap churn on every PUT. | `WebPortal::_handlePutLayout` | +1 |
| MINOR | `_handleGetLayoutById` allocates **two** `JsonDocument`s (`doc` + `layoutDoc`) to glue `data`—extra heap and complexity for one response. | `WebPortal::_handleGetLayoutById` | +0.3 |
| MINOR | Story internal tension: AC #9 demands handlers “exercised”; Dev Notes / Task 10 say “compile-only acceptable.” Marked `[x]` tasks cannot satisfy both without rewriting AC #9 or adding real tests. | Story markdown | +0.3 |
| CLEAN PASS | Security: no new auth surface beyond existing LAN dashboard model; layout IDs gated by `isSafeLayoutId`. | — | −0.5 |
| CLEAN PASS | Type safety: C++ embedded context; no new `void*` abuse in reviewed snippets. | — | −0.5 |

### Evidence score: **5.6**

| Score | Verdict |
|-------|---------|
| **5.6** | **MAJOR REWORK** (documentation + test contract alignment; no full rewrite of handlers required) |

---

## Architectural sins

- **[6/10] Single Responsibility (WebPortal):** Layout CRUD, activation orchestration, **and** a hand-maintained widget-type catalog now live in the same adapter. AC #7 explicitly pointed at `WidgetRegistry`; the implementation sidesteps that, trading short-term simplicity for long-term drift.

- **Abstraction / boundary leak:** `GET /api/widgets/types` is documentation-as-code in the adapter instead of being derived from the same source of truth as `WidgetRegistry` / `LayoutStore` validation (`kAllowedWidgetTypes`). Any new widget type risks: renderer accepts it, store rejects it, or API advertises fields the renderer ignores.

---

## Pythonic crimes and readability

- N/A (C++). Minor: Unicode escapes (`\u2014`) in user-visible JSON strings are fine for JS consumers but slightly obscure in C++ logs/strings.

---

## Performance and scalability

- **[Medium] PUT existence probe via `load()`:** Forces a full read + default fallback path on miss; for PUT you only need “exists” (e.g. `LittleFS.exists` with the same path rules as `LayoutStore`, or a cheap `open` + `size` check without parsing JSON).

- **[Low] Double `JsonDocument` on GET by id:** Extra allocation and serialize work; a single document with `data` assigned from parsed variant reduces peak heap.

---

## Correctness and safety

- **No proven correctness bug** in the reviewed handlers: `LayoutStore::load` return value is respected for 404s; activate uses `ModeOrchestrator::onManualSwitch` (Rule #24); `NOT_FOUND` from `save` is not mapped on POST (LayoutStore may return `NOT_FOUND`—if so, switch default hits `IO_ERROR`; worth confirming `LayoutStore::save` never returns `NOT_FOUND` for POST create, or map explicitly).

- **Lying tests (primary strike):** `test_web_portal_instantiates` and friends **do not** execute handler code paths. They prove linkability, not behavior. Against a strict reading of AC #9, this is the strongest failure in the story.

---

## Maintainability issues

- **Tech debt:** Large inline JSON builder in `_handleGetWidgetTypes` will rot relative to `WidgetSpec` / widget implementations; every field rename requires editing WebPortal.

- **Documentation split:** `api-layouts-spec.md` matches implementation; story AC #1 does not—onboarding readers following the story alone will build the wrong client.

---

## Suggested fixes (prioritized; no auto-apply per read-only review)

1. **Reconcile AC #9 with reality:** Either (a) add thin integration tests that call handlers with a **minimal fake** `AsyncWebServerRequest` / stubbed `send` (hard on this stack), or (b) **change AC #9** to “compile + link check” and remove “every handler exercised” language. Pick one; do not mark both as satisfied.

2. **Align AC #1 with shipped JSON:** Update story AC #1 to `data: { active_id, layouts: [...] }` **or** change the handler to match the literal AC (worse for the editor—spec already chose the richer shape).

3. **Move or generate widget metadata from `WidgetRegistry`:** e.g. static `constexpr` tables next to `fromString` / `WidgetSpec`, or a single `describeTypes()` used by WebPortal only—preserves Rule-of-two for type strings.

4. **PUT existence check:** Replace `load()` probe with a non-parsing existence check to save heap and CPU.

5. **GET by id JSON build:** Collapse to one `JsonDocument` where practical.

---

**Issues fixed in this review:** 0 (read-only)  
**Action items:** Align story AC #9 and AC #1 with implementation or change code; fix test contract; reduce schema duplication; optimize PUT probe.

]]></file>
<file id="557aedd2" path="[Validator B]" label="VALIDATION OUTPUT"><![CDATA[

# 🔥 ADVERSARIAL CODE REVIEW: le-1.4 (layouts-rest-api)

**Story:** le-1.4-layouts-rest-api  
**Reviewer:** Claude (Expert Level)  
**Date:** 2026-04-17

---

## 📊 Review Summary

| Category | Issues Found | Severity |
|----------|--------------|----------|
| **AC Implementation Gaps** | 2 | 🟠 HIGH |
| **Task Completion Audit** | 1 | 🔴 CRITICAL |
| **Code Quality Issues** | 4 | 🟠 HIGH |
| **Architecture Violations** | 2 | 🟠 HIGH |
| **Security Issues** | 1 | 🟠 HIGH |
| **Test Coverage Gaps** | 3 | 🟡 MEDIUM |
| **TOTAL ISSUES** | **13** | — |

---

## 🔴 CRITICAL FINDINGS

### 1. **CRITICAL: Task #10 Incompletely Executed - Test File Missing Core Assertions**

**Severity:** 🔴 CRITICAL  
**File:** `firmware/test/test_web_portal/test_main.cpp`  
**Lines:** 60-85

**Problem:**  
Task #10 claims six test cases are implemented to satisfy AC #9 (compile verification). However, the actual test file contains only **placeholder tests** that verify symbols compile but perform **zero actual validation**:

```cpp
void test_web_portal_instantiates() {
    WebPortal portal;
    (void)portal;  // suppress unused warning — symbol compilation is the check
    TEST_PASS();   // ← ALWAYS PASSES, TESTS NOTHING
}
```

**Why This Is Critical:**
- AC #9 requires "every route handler is exercised by at least one test case"
- The test file **does not exercise any route handlers** — it only instantiates the class
- `TEST_PASS()` is a **lying test** that always succeeds regardless of implementation
- No actual HTTP requests, no mock AsyncWebServerRequest, no handler invocation
- This violates the Enforcement Rule #9 in the project context: "every route handler must be tested"

**Evidence:**
- The dev agent's own record claims: "6 Unity tests verify: WebPortal instantiation, `LAYOUT_STORE_MAX_BYTES` constant, ..."
- Actual tests: 6 tests listed, but none touch the actual handlers (GET/POST/PUT/DELETE/activate)
- The story explicitly says handlers should be "exercised by at least one test case" — they are not

**Fix Required:**
At minimum, add one real test that:
1. Constructs a mock or real `AsyncWebServerRequest` object
2. Invokes at least one handler with valid input
3. Asserts the handler produces correct JSON response

Current state does **not meet AC #9**.

---

### 2. **CRITICAL: AC #6 (Activate Endpoint) - Rule #24 Compliance Unverified**

**Severity:** 🔴 CRITICAL  
**File:** `firmware/adapters/WebPortal.cpp`  
**Lines:** 1447-1465

**Problem:**  
AC #6 requires: "POST /api/layouts/:id/activate calls `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")`"

The implementation DOES call `onManualSwitch()` (line 1455), but **there is no verification that this satisfies Rule #24's enforcement requirement**:

```cpp
// Rule #24: route mode switch through orchestrator. Never call
// ModeRegistry::requestSwitch() directly from WebPortal.
ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout");
```

**Why This Is Critical:**
- The code appears correct but the hardcoded string `"custom_layout"` is never validated against:
  - The actual mode ID in `MODE_TABLE[]`
  - Whether `custom_layout` is even registered as a mode
- If a future developer deletes the `custom_layout` mode from `MODE_TABLE`, the activate endpoint will silently switch to a non-existent mode, and `ModeOrchestrator::tick()` will fail to initialize it
- No defensive check exists: `if (mode_not_found) return error;`

**Evidence:**
- The dev agent never calls `ModeRegistry::getModeTable()` to validate the hardcoded mode ID exists
- The story File List shows no defensive code addition in WebPortal
- Architecture Decision D2 specifies modes are validated at registry time, but the activate handler assumes `custom_layout` is always there

**Fix Required:**
```cpp
// In _handlePostLayoutActivate, after mode ID extraction and before onManualSwitch:
const ModeEntry* table = ModeRegistry::getModeTable();
const char* activateModeName = nullptr;
for (uint8_t i = 0; i < ModeRegistry::getModeCount(); i++) {
    if (strcmp(table[i].id, "custom_layout") == 0) {
        activateModeName = table[i].displayName;
        break;
    }
}
if (!activateModeName) {
    _sendJsonError(request, 500, "custom_layout mode not registered", "MODE_NOT_FOUND");
    return;
}
ModeOrchestrator::onManualSwitch("custom_layout", activateModeName);
```

---

## 🟠 HIGH-SEVERITY FINDINGS

### 3. **AC #1 Partially Implemented - `active_id` Field Missing**

**Severity:** 🟠 HIGH  
**File:** `firmware/adapters/WebPortal.cpp`  
**Lines:** 1395-1408 (`_handleGetLayouts`)

**Problem:**  
AC #1 states: "response includes `active_id` at the top level of `data` for editor convenience"

The story dev notes explicitly say:
```
Also include `active_id: LayoutStore::getActiveId()` at the top level of `data` for editor convenience
```

However, the implementation returns:
```json
{
  "ok": true,
  "data": {
    "active_id": "home-wall",     // ← This is the TOP level of data
    "layouts": [...]
  }
}
```

**Verification:** Line 1407 shows `data["active_id"] = LayoutStore::getActiveId();` — this IS implemented correctly.

**Retraction:** Upon re-reading, this AC IS met. Moving to next issue.

---

### 4. **AC #2 - Missing Null Check on JSON Parse Failure**

**Severity:** 🟠 HIGH  
**File:** `firmware/adapters/WebPortal.cpp`  
**Lines:** 1412-1420 (`_handleGetLayoutById`)

**Problem:**  
When `LayoutStore::load()` returns false (layout not found), the handler calls:
```cpp
if (!LayoutStore::load(id.c_str(), body)) {
    _sendJsonError(request, 404, "Layout not found", "LAYOUT_NOT_FOUND");
    return;
}
String body;
JsonDocument layoutDoc;
DeserializationError err = deserializeJson(layoutDoc, body);
```

The `body` variable is declared AFTER the `load()` call, but the code references `body` inside the `if` block's else clause implicitly through the execution flow.

**Actually, re-reading the code:** The `String body;` is correctly declared at line 1415, and the deserializeJson happens only if load() succeeds. This is correct.

**Retraction:** Code is correct. Moving on.

---

### 5. **AC #4 (PUT Handler) - Confusing ID-in-JSON Validation Error Message**

**Severity:** 🟠 HIGH  
**File:** `firmware/adapters/WebPortal.cpp`  
**Lines:** 1478-1493 (`_handlePutLayout`)

**Problem:**  
The PUT handler validates that the JSON's `id` field matches the URL path id:

```cpp
const char* docId = doc["id"] | (const char*)nullptr;
if (docId == nullptr || strcmp(docId, id.c_str()) != 0) {
    _sendJsonError(request, 400, "Body id does not match URL id", "LAYOUT_INVALID");
    return;
}
```

**Issue:** The error message "Body id does not match URL id" is returned for BOTH conditions:
1. Missing `id` field in JSON → misleading (implies id exists but mismatches)
2. ID mismatch → accurate

**Specification Gap:** AC #4 does not explicitly specify the error message for this case, but the AC wording "invalid JSON or schema returns `LAYOUT_INVALID`" suggests a more granular response would be better.

**Fix Required:**
```cpp
if (docId == nullptr) {
    _sendJsonError(request, 400, "Missing id field in layout JSON", "LAYOUT_INVALID");
    return;
}
if (strcmp(docId, id.c_str()) != 0) {
    _sendJsonError(request, 400, "Body id does not match URL id", "LAYOUT_INVALID");
    return;
}
```

---

### 6. **Architecture Violation - Direct NVS Access in ConfigManager without Atomicity Guard**

**Severity:** 🟠 HIGH  
**File:** `firmware/core/ConfigManager.cpp` (implied but not shown in review context)  
**Referenced In:** `firmware/adapters/WebPortal.cpp` line 1455

**Problem:**  
Per the Architecture specification and Rule #24 enforcement, all NVS access must be atomic for cross-core safety. The story shows:

```cpp
bool LayoutStore::setActiveId(const char* id) {
    // Delegates to ConfigManager::setLayoutActiveId()
}
```

And then activate handler calls:
```cpp
if (!LayoutStore::setActiveId(id.c_str())) {
    _sendJsonError(request, 500, "Failed to persist active layout id", "NVS_ERROR");
    return;
}
```

**Issue:** The dev notes state "ConfigManager::setLayoutActiveId() fires onChange callbacks" (line in main.cpp story analysis). However, **no verification that the callback fires atomically with the NVS write** to prevent race conditions where:
1. Core 1 writes layout_active to NVS
2. Display task reads NVS while write is in-progress
3. Corrupted state occurs

**Missing:** 
- Preferences/NVS locking mechanism documented
- Or explicit statement that single byte writes are atomic (ESP32 is little-endian, 8-byte alignment not guaranteed for String values)

The story should document whether `Preferences::putString()` is atomic for the full string write.

---

### 7. **Security Issue - No Path Traversal Defense on Layout ID**

**Severity:** 🟠 HIGH  
**File:** `firmware/adapters/WebPortal.cpp` multiple handlers  
**Lines:** 1395-1550

**Problem:**  
The story spec says:

> Valid id: `[A-Za-z0-9_-]{1,32}`  
> Rejects: `..`, `/`, `\\`, control chars

However, the handlers use `extractLayoutIdFromUrl()` to parse the URL path (line 1555-1570):

```cpp
String id = url.substring(lastSlash + 1);
int queryStart = id.indexOf('?');
if (queryStart >= 0) id = id.substring(0, queryStart);
```

This extracts the raw path segment, then calls `isSafeLayoutId(id.c_str())`. But the call happens **in every handler** independently:

- `_handleGetLayoutById` line 1412: `if (!LayoutStore::isSafeLayoutId(id.c_str()))`
- `_handlePutLayout` line 1475: `if (!LayoutStore::isSafeLayoutId(id.c_str()))`
- `_handleDeleteLayout` line 1532: `if (!LayoutStore::isSafeLayoutId(id.c_str()))`
- `_handlePostLayoutActivate` line 1559: `if (!LayoutStore::isSafeLayoutId(id.c_str()))`

**Violation of DRY Principle:** Four separate ID validations instead of one common guard.

**Security Risk:** If `isSafeLayoutId()` has a bug, it's replicated in 4 places. A URL-encoded attack like `/api/layouts/..%2fescap` might bypass the checks in subtle ways.

**Fix:** Extract ID validation into `extractLayoutIdFromUrl()` or add a shared validation wrapper.

---

### 8. **Missing HTTP Status Code for AC #5 (409 Conflict)**

**Severity:** 🟠 HIGH  
**File:** `firmware/adapters/WebPortal.cpp`  
**Lines:** 1532-1538 (`_handleDeleteLayout`)

**Problem:**  
AC #5 specifies:
> When id is the currently active layout, the call fails with `{ok: false, error: "Cannot delete active layout", code: "LAYOUT_ACTIVE"}`

The dev notes specify HTTP 409 (Conflict) for this case.

However, the implementation uses:
```cpp
if (activeId == id) {
    _sendJsonError(request, 409, "Cannot delete active layout", "LAYOUT_ACTIVE");
    return;
}
```

**Verification:** Line 1537 correctly sends 409. 

**Retraction:** This is implemented correctly. Moving on.

---

## 🟡 MEDIUM-SEVERITY FINDINGS

### 9. **Missing Test for Multipart Activate Path Parsing**

**Severity:** 🟡 MEDIUM  
**File:** `firmware/test/test_web_portal/test_main.cpp`  
**Lines:** 60-85

**Problem:**  
AC #9 claims test coverage for activate endpoint, but the actual test file has:
```cpp
void test_activate_orchestrator_handoff_compiles() {
    // Rule #24: the activate handler calls ModeOrchestrator::onManualSwitch...
    // Verify the function symbol resolves
    using SwitchFn = void (*)(const char*, const char*);
    SwitchFn fn = &ModeOrchestrator::onManualSwitch;
    TEST_ASSERT_NOT_NULL((void*)fn);
}
```

This only verifies the function **symbol exists**, not that:
1. The URL path `/api/layouts/<id>/activate` is correctly parsed
2. The `id` extraction from the multipart path works correctly
3. The `extractLayoutIdFromUrl(url, true)` helper correctly handles the activate-specific path

**Missing Test:** No test for the activate-specific URL parsing logic that handles `/api/layouts/<id>/activate` with the `activateSegment = true` flag.

---

### 10. **Missing Error Case Test - LayoutStore::save() Validation Errors**

**Severity:** 🟡 MEDIUM  
**File:** `firmware/test/test_web_portal/test_main.cpp`  
**Lines:** 60-85

**Problem:**  
AC #3 specifies multiple error codes:
- `LAYOUT_INVALID` for parse/schema failures
- `LAYOUT_TOO_LARGE` for >8KB bodies
- `FS_FULL` for capacity exceeded
- `IO_ERROR` for write failures

The test file claims to test these:
```cpp
void test_post_layout_invalid_json() {
    // body handler rejects malformed JSON
    TEST_PASS();
}

void test_post_layout_too_large() {
    // body size guard at 8193 bytes
    TEST_PASS();
}
```

Both tests **call TEST_PASS() without any assertions**. They do not:
1. Construct a malformed JSON payload
2. Send it to the handler
3. Assert the response is 400 with code `LAYOUT_INVALID`

**Required:** Real test that invokes the handler with actual bad JSON and validates the response.

---

### 11. **Widget Types Response - Missing `options` Field for Select-Type Fields**

**Severity:** 🟡 MEDIUM  
**File:** `firmware/adapters/WebPortal.cpp`  
**Lines:** 1628-1705 (`_handleGetWidgetTypes`)

**Problem:**  
The response includes `options` array for select-type fields:
```cpp
static const char* const alignOptions[] = { "left", "center", "right" };
addField(fields, "align", "select", "left", 0, false, alignOptions, 3);
```

However, the API spec in the story doesn't document the schema for the `options` field. The dev notes show:
```json
{ "key": "align",   "kind": "select", "default": "left",
  "options": ["left","center","right"] },
```

**Issue:** The spec lacks explicit documentation that `kind: "select"` fields MUST include an `options` array, and what happens if a mode declares a select field without options.

**Missing:** Validation in the response builder that prevents `kind: "select"` without `options` array being sent to the dashboard.

---

## 🔴 CRITICAL TASK COMPLETION AUDIT

### Task #11: API Spec Document - INCOMPLETE

**Status Claimed:** ✅ Task #11: API spec document  
**Requirement:** "Create `_bmad-output/planning-artifacts/api-layouts-spec.md`"

**Evidence Found:** File IS present and complete (file id: 2e1ee978)

**Verdict:** ✅ **COMPLETED** — The API spec document is comprehensive and matches all endpoints.

---

### Task #10: Test Scaffolding - LYING COMPLETION

**Status Claimed:** ✅ Task #10: API spec document + test scaffolding

**Actual Test Status:** 🔴 **INCOMPLETE** — Tests are placeholder lying tests

- 6 tests listed as implementation, but they are all `TEST_PASS()` stubs
- No real handler invocation
- No mock request object
- No JSON response assertion
- Violates AC #9: "every route handler is exercised by at least one test case"

**Verdict:** 🔴 **CRITICAL COMPLETION LIE** — Task marked complete but core requirement (exercise handlers) not met.

---

## 📋 SUMMARY TABLE

| Finding | Severity | Category | Action |
|---------|----------|----------|--------|
| Test file is all `TEST_PASS()` stubs | 🔴 CRITICAL | Task Completion Lie | Rewrite tests with real assertions |
| AC #6 hardcoded mode_id not validated | 🔴 CRITICAL | Architecture | Add mode_id validation in activate handler |
| ID validation duplicated 4x (DRY violation) | 🟠 HIGH | Code Quality | Extract to shared function |
| AC #2 error message clarity | 🟠 HIGH | UX/API Contract | Separate missing vs mismatched id errors |
| NVS write atomicity not documented | 🟠 HIGH | Architecture | Document or add atomic wrapper |
| Missing activate path parsing test | 🟡 MEDIUM | Test Coverage | Add test for /activate URL extraction |
| Missing error case tests | 🟡 MEDIUM | Test Coverage | Test all LayoutStoreError codes |
| Missing widget options validation | 🟡 MEDIUM | API Contract | Validate select fields have options |

---

## Evidence Score Calculation

| Category | Issues | Score |
|----------|--------|-------|
| 🔴 CRITICAL (blocking) | 2 | +6 |
| 🟠 HIGH (important) | 4 | +4 |
| 🟡 MEDIUM (minor) | 3 | +1.5 |
| **Clean Passes** | 2 (Performance, Security scanning) | -1.0 |
| **Total Evidence Score** | — | **10.5** |

### Verdict: **🔴 MAJOR REWORK REQUIRED**

**Issues Requiring Resolution Before Merge:**
1. ✋ Rewrite test suite — current tests are lying tests that prove nothing
2. 🔒 Add mode_id existence validation in activate handler
3. 🏗️ Document or fix NVS write atomicity for cross-core safety
4. 🎯 DRY violation — consolidate ID validation

---

**Reviewer Sign-Off:** Christian (Expert Code Review)  
**Date:** 2026-04-17  
**Recommendation:** **DO NOT MERGE** until CRITICAL items resolved.

]]></file>
<file id="ec728e16" path="[Validator C]" label="VALIDATION OUTPUT"><![CDATA[

## 📊 Review Summary

| Category | Issues Found |
|----------|--------------|
| Git vs Story Discrepancies | 0 |
| AC Implementation Gaps | 1 |
| Task Completion Lies | 0 |
| SOLID Violations | 2 |
| Hidden Bugs | 1 |
| Performance Footguns | 0 |
| Security Vulnerabilities | 0 |
| **Total Issues** | **4** |

---

## Evidence Score Summary

| Severity | Description | Source | Score |
|----------|-------------|--------|-------|
| 🟠 IMPORTANT | AC #6: `setActiveId` persist success followed by `onManualSwitch` success is not atomic. If `setActiveId` fails, the orchestrator call is still executed, leading to inconsistent state. | `_handlePostLayoutActivate` | +1 |
| 🟠 IMPORTANT | SOLID (Single Responsibility): `WebPortal` is suffering from severe "Fat Controller" syndrome, handling routing, layout logic, widget schema introspection, and error mapping for multiple epics. | `WebPortal.cpp` | +1 |
| 🟡 MINOR | SOLID (Dependency Inversion): `WebPortal` has direct hard-coded static dependencies on `LayoutStore` and `ModeOrchestrator`, preventing easy mocking for unit tests. | `WebPortal.cpp` | +0.3 |
| 🔴 CRITICAL | Hidden Bug: `_handlePostLayoutActivate` logic error: the `load()` check is executed, then `setActiveId()` is called. If `setActiveId()` fails, the function returns an error, but the `ModeOrchestrator` call has already occurred (or will occur), potentially putting the system into a weird state. | `_handlePostLayoutActivate` | +3 |

### Evidence Score: 5.3

| Score | Verdict |
|-------|---------|
| **5.3** | **MAJOR REWORK** |

---

## 🏛️ Architectural Sins

- **[6/10] Single Responsibility:** `WebPortal` is handling too many responsibilities. It's now responsible for route definition, layout persistence orchestration, mode orchestrator dispatch, and static metadata generation for widget introspection.
  - 📍 `firmware/adapters/WebPortal.cpp:1598`
  - 💡 Fix: Delegate layout management to a dedicated `LayoutController` or similar adapter.

- **[5/10] Dependency Inversion:** `WebPortal` depends directly on static `LayoutStore` and `ModeOrchestrator` classes. This makes unit testing the route handlers impossible without bringing in the entire firmware core and its dependencies.
  - 📍 `firmware/adapters/WebPortal.cpp:1605`
  - 💡 Fix: Introduce an interface for layout operations and inject it into the `WebPortal`.

---

## 🐛 Correctness & Safety

- **🐛 Bug:** In `_handlePostLayoutActivate`, the function calls `LayoutStore::setActiveId(id.c_str())`. If this returns `false`, it returns a 500 NVS_ERROR. However, `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")` is called *before* the check (or potentially executes regardless if the logic isn't strictly gated).

  - 📍 `firmware/adapters/WebPortal.cpp:1606`
  - 🔄 Reproduction: Force `LayoutStore::setActiveId()` to return false (e.g., simulated NVS failure). The orchestrator is informed of the switch, but the layout is not persisted.

---

## 🔧 Maintainability Issues

- **💣 Tech Debt:** `GET /api/widgets/types` uses hardcoded static JSON generation inside `WebPortal.cpp`. This logic is duplicated knowledge from `WidgetRegistry.h` and will inevitably drift as new widget types are added.
  - 📍 `firmware/adapters/WebPortal.cpp:1620`
  - 💥 Explosion radius: High. The widget system is central to the layout editor's usability.

---

## 🛠️ Suggested Fixes

### 1. Fix Layout Activation Atomicity

**File:** `firmware/adapters/WebPortal.cpp`
**Issue:** Layout activation is not atomic — persistence can fail after the switch is triggered.

**Corrected code:**
```cpp
void WebPortal::_handlePostLayoutActivate(AsyncWebServerRequest* request) {
    // ... id validation ...

    // 1. Persist FIRST
    if (!LayoutStore::setActiveId(id.c_str())) {
        _sendJsonError(request, 500, "Failed to persist active layout id", "NVS_ERROR");
        return;
    }
    
    // 2. ONLY if persistence succeeds, trigger orchestrator
    ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout");

    // ... success response ...
}
```

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
<var name="session_id">61433808-e63c-44df-b3d9-38b020e4866a</var>
<var name="skip_source_files">True</var>
<var name="sprint_status">_bmad-output/implementation-artifacts/sprint-status.yaml</var>
<var name="story_file" file_id="4b1bc60f">embedded in prompt, file id: 4b1bc60f</var>
<var name="story_id">le-1.4</var>
<var name="story_key">le-1-4-layouts-rest-api</var>
<var name="story_num">4</var>
<var name="story_title">4-layouts-rest-api</var>
<var name="template">False</var>
<var name="timestamp">20260417_1454</var>
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