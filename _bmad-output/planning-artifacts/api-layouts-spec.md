# Layouts REST API — Specification

> Story: **LE-1.4 (layouts-rest-api)**
> Consumer: Dashboard Layout Editor (LE-1.7)
> Owner: WebPortal adapter (`firmware/adapters/WebPortal.cpp`)
> Transport: HTTP 1.1 · JSON envelope
> Constraints: ESP32 · ESPAsyncWebServer (mathieucarbou fork) · ArduinoJson v7

## Envelope Convention

All responses use the canonical envelope already used by the rest of the
FlightWall REST surface:

```json
// Success
{ "ok": true,  "data": { ... } }

// Error
{ "ok": false, "error": "Human-readable message", "code": "MACHINE_CODE" }
```

The `error`/`code` split mirrors `POST /api/settings`, `POST /api/display/mode`,
and the OTA endpoints.

## HTTP Status Code Map

| Situation                         | HTTP | Code                  |
| --------------------------------- | ---- | --------------------- |
| Success                           | 200  | —                     |
| Invalid JSON / schema / id        | 400  | `LAYOUT_INVALID` / `INVALID_ID` / `MISSING_ID` |
| Size limit exceeded (>8 KiB)      | 413  | `LAYOUT_TOO_LARGE`    |
| Active layout deletion attempted  | 409  | `LAYOUT_ACTIVE`       |
| Layout id not found on LittleFS   | 404  | `LAYOUT_NOT_FOUND`    |
| Filesystem full (byte/file cap)   | 507  | `FS_FULL`             |
| NVS / IO failure                  | 500  | `NVS_ERROR` / `IO_ERROR` / `STORAGE_UNAVAILABLE` |

## Shared Validation

* **Layout id safety** — enforced by `LayoutStore::isSafeLayoutId()`.
  `[A-Za-z0-9_-]{1,32}`; rejects `..`, `/`, `\\`, control chars, and the
  reserved id `_default`.
* **Per-file cap** — `LAYOUT_STORE_MAX_BYTES = 8192`. Any body larger than
  this is rejected *before* it is buffered.
* **Directory cap** — `LAYOUT_STORE_TOTAL_BYTES = 65536` and
  `LAYOUT_STORE_MAX_FILES = 16`, enforced transparently by
  `LayoutStore::save()`.
* **JSON self-consistency** — every stored file must satisfy
  `doc["id"] == filename-stem`; mismatching ids are rejected with
  `LAYOUT_INVALID`.

---

## `GET /api/layouts`

List every layout on LittleFS plus the currently-active id.

### Response 200

```json
{
  "ok": true,
  "data": {
    "active_id": "home-wall",
    "layouts": [
      { "id": "home-wall", "name": "Home Wall", "bytes": 1834 },
      { "id": "office",    "name": "Office",    "bytes": 1211 }
    ]
  }
}
```

Empty array is returned when no layouts are saved.

### Errors
* 500 `STORAGE_UNAVAILABLE` — `/layouts/` directory cannot be opened.

### Example
```bash
curl -sf http://flightwall.local/api/layouts | jq .
```

---

## `GET /api/layouts/:id`

Return the full layout JSON for a single id.

### Response 200

```json
{
  "ok": true,
  "data": {
    "id": "home-wall",
    "name": "Home Wall",
    "v": 1,
    "canvas": { "w": 160, "h": 32 },
    "bg": "#000000",
    "widgets": [
      { "id": "w1", "type": "clock", "x": 0, "y": 0, "w": 80, "h": 16, "color": "#FFFFFF" }
    ]
  }
}
```

The layout is embedded as a parsed JSON object under `data`, **not** a
double-encoded string.

### Errors
* 400 `MISSING_ID` / `INVALID_ID`
* 404 `LAYOUT_NOT_FOUND`
* 500 `LAYOUT_CORRUPT` — file exists but failed to parse

### Example
```bash
curl -sf http://flightwall.local/api/layouts/home-wall | jq .data
```

---

## `POST /api/layouts`

Create (or overwrite) a layout. The body **must** include a top-level `id`
field that matches the caller's intended layout id.

### Request
```json
{
  "id":"home-wall","name":"Home Wall","v":1,
  "canvas":{"w":160,"h":32},"bg":"#000000",
  "widgets":[
    {"id":"w1","type":"clock","x":0,"y":0,"w":80,"h":16,"color":"#FFFFFF"}
  ]
}
```

### Response 200
```json
{ "ok": true, "data": { "id": "home-wall", "bytes": 186 } }
```

### Errors
* 400 `LAYOUT_INVALID` — parse failure, missing `id`, unsafe id, or
  schema violation (missing required fields, `v != 1`, bad canvas,
  unknown widget `type`)
* 413 `LAYOUT_TOO_LARGE` — body exceeds 8 KiB
* 507 `FS_FULL` — directory byte or file-count cap exceeded
* 500 `IO_ERROR` — LittleFS write failed

### Example
```bash
curl -sf -X POST -H 'content-type: application/json' \
  --data @home-wall.json http://flightwall.local/api/layouts | jq .
```

---

## `PUT /api/layouts/:id`

Overwrite an **existing** layout. To create a new layout, use `POST`.

The URL path id is authoritative: the body's `id` must match, otherwise the
request is rejected with `LAYOUT_INVALID`.

### Response 200
```json
{ "ok": true, "data": { "id": "home-wall", "bytes": 212 } }
```

### Errors
* 400 `MISSING_ID` / `INVALID_ID` / `LAYOUT_INVALID`
* 404 `LAYOUT_NOT_FOUND` — nothing to overwrite
* 413 `LAYOUT_TOO_LARGE`
* 507 `FS_FULL`
* 500 `IO_ERROR`

### Example
```bash
curl -sf -X PUT -H 'content-type: application/json' \
  --data @home-wall.json \
  http://flightwall.local/api/layouts/home-wall | jq .
```

---

## `DELETE /api/layouts/:id`

Remove a layout. The currently-active layout cannot be deleted — switch away
first via `POST /api/layouts/:other/activate`.

### Response 200
```json
{ "ok": true }
```

### Errors
* 400 `MISSING_ID` / `INVALID_ID`
* 404 `LAYOUT_NOT_FOUND`
* 409 `LAYOUT_ACTIVE` — attempting to delete the active layout

### Example
```bash
curl -sf -X DELETE http://flightwall.local/api/layouts/old-draft
```

---

## `POST /api/layouts/:id/activate`

Atomically make `:id` the active layout. Two side effects follow:

1. `LayoutStore::setActiveId(id)` persists the id to NVS via `ConfigManager`.
2. `ModeOrchestrator::onManualSwitch("custom_layout", "Custom Layout")` is
   called so the orchestrator enters `MANUAL` state on the next tick
   (Rule #24 compliant — never call `ModeRegistry::requestSwitch()` directly
   from the web layer).

If `custom_layout` is already the active mode, the orchestrator is still
nudged through `MANUAL` (same-mode is a no-op inside `ModeRegistry::tick()`);
the `configChanged` callback fired by `setActiveId()` drives the existing
`requestForceReload("custom_layout")` path so the new layout actually loads.

### Response 200
```json
{ "ok": true, "data": { "active_id": "home-wall" } }
```

### Errors
* 400 `INVALID_PATH` / `INVALID_ID`
* 404 `LAYOUT_NOT_FOUND`
* 500 `NVS_ERROR` — NVS write failed

### Example
```bash
curl -sf -X POST http://flightwall.local/api/layouts/home-wall/activate
```

---

## `GET /api/widgets/types`

Static schema endpoint used by the editor to drive the widget field forms.
Describes every widget type the firmware's `WidgetRegistry` currently
accepts.

### Response 200
```json
{
  "ok": true,
  "data": [
    {
      "type": "text",
      "label": "Text",
      "fields": [
        { "key": "content", "kind": "string", "default": "" },
        { "key": "align",   "kind": "select", "default": "left",
          "options": ["left","center","right"] },
        { "key": "color",   "kind": "color",  "default": "#FFFFFF" },
        { "key": "x", "kind": "int", "default": 0 },
        { "key": "y", "kind": "int", "default": 0 },
        { "key": "w", "kind": "int", "default": 32 },
        { "key": "h", "kind": "int", "default": 8 }
      ]
    },
    {
      "type": "clock",
      "label": "Clock",
      "fields": [
        { "key": "content", "kind": "select", "default": "24h",
          "options": ["12h","24h"] },
        { "key": "color",   "kind": "color",  "default": "#FFFFFF" },
        { "key": "x", "kind": "int", "default": 0 },
        { "key": "y", "kind": "int", "default": 0 },
        { "key": "w", "kind": "int", "default": 48 },
        { "key": "h", "kind": "int", "default": 8 }
      ]
    },
    {
      "type": "logo",
      "label": "Logo",
      "note": "LE-1.5 stub — renders solid color block until real bitmap pipeline lands",
      "fields": [
        { "key": "color", "kind": "color", "default": "#0000FF" },
        { "key": "x", "kind": "int", "default": 0 },
        { "key": "y", "kind": "int", "default": 0 },
        { "key": "w", "kind": "int", "default": 16 },
        { "key": "h", "kind": "int", "default": 16 }
      ]
    },
    {
      "type": "flight_field",
      "label": "Flight Field",
      "note": "LE-1.8 — not yet implemented, renders nothing",
      "fields": [
        { "key": "content", "kind": "string", "default": "" },
        { "key": "color",   "kind": "color",  "default": "#FFFFFF" },
        { "key": "x", "kind": "int", "default": 0 },
        { "key": "y", "kind": "int", "default": 0 },
        { "key": "w", "kind": "int", "default": 48 },
        { "key": "h", "kind": "int", "default": 8 }
      ]
    },
    {
      "type": "metric",
      "label": "Metric",
      "note": "LE-1.8 — not yet implemented, renders nothing",
      "fields": [
        { "key": "content", "kind": "string", "default": "" },
        { "key": "color",   "kind": "color",  "default": "#FFFFFF" },
        { "key": "x", "kind": "int", "default": 0 },
        { "key": "y", "kind": "int", "default": 0 },
        { "key": "w", "kind": "int", "default": 48 },
        { "key": "h", "kind": "int", "default": 8 }
      ]
    }
  ]
}
```

### Example
```bash
curl -sf http://flightwall.local/api/widgets/types | jq '.data[].type'
```
