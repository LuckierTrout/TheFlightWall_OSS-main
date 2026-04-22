# Story LE-1.10: Widget Field Picker (FlightField + Metric)

branch: le-1-10-widget-field-picker
zone:
  - firmware/widgets/FlightFieldWidget.{cpp,h}
  - firmware/widgets/MetricWidget.{cpp,h}
  - firmware/adapters/WebPortal.cpp
  - firmware/data-src/editor.{js,html}
  - firmware/data/editor.{js,html}.gz
  - firmware/test/test_flight_field_widget/**
  - firmware/test/test_metric_widget/**
  - firmware/test/test_web_portal/**

Status: done (Task 9 on-device smoke pending manual sign-off)

## Story

As a **layout author** (Christian, solo operator),
I want **to pick which flight-identity field a FlightField widget shows and which flight-telemetry field a Metric widget shows, from a curated firmware-owned catalog**,
So that **I can place speed, altitude, aircraft type, route, callsign, and related data exactly where I want them on the 80×48 canvas without hand-editing layout JSON**.

## Acceptance Criteria

1. **Given** `firmware/widgets/FlightFieldWidget.h` **When** compiled **Then** it exposes `static constexpr FieldDescriptor kCatalog[]` covering six entries — `callsign`, `airline`, `aircraft_type`, `origin_icao`, `destination_icao`, `flight_number` — each with `{id, label}`, and a `static const FieldDescriptor* catalog(size_t& outCount)` accessor.

2. **Given** `firmware/widgets/MetricWidget.h` **When** compiled **Then** it exposes `static constexpr FieldDescriptor kCatalog[]` covering six entries — `altitude_ft`, `speed_kts`, `heading_deg`, `vertical_rate_fpm`, `distance_nm`, `bearing_deg` — each with `{id, label, unit}`, and a `static const FieldDescriptor* catalog(size_t& outCount)` accessor.

3. **Given** `GET /api/widgets/types` **When** called **Then** the JSON response entry for `"flight_field"` and `"metric"` widget types each includes a **`field_options`** array `[{ "id": "...", "label": "..." }, ...]` (and optional `"unit"` on metric entries), populated from the widget's catalog accessor (single source of truth; no duplication in WebPortal). The catalog is **not** placed under the top-level `"fields"` key — that key remains the property-panel **schema** (`content`, `color`, `x`, …); **`field_options`** is a sibling key to avoid colliding with that schema (see Dev Notes).

4. **Given** a `FlightFieldWidget` instance with `config.field_id` set to a known catalog id **When** `render()` is called with a non-null `RenderContext::currentFlight` **Then** the widget displays the value for that field resolved from `FlightInfo` (see Dev Notes — Field Resolution Table). **Given** `field_id` is missing or unknown **Then** the widget falls back to `kCatalog[0]` (i.e. `callsign`). **Given** the resolved value is null/empty **Then** the widget renders the em-dash glyph `"—"`.

5. **Given** a `MetricWidget` instance with `config.field_id` set to a known catalog id **When** `render()` is called with non-null flight telemetry in `RenderContext::currentFlight` or the active `StateVector` **Then** the widget displays the value formatted per-unit (see Dev Notes — Unit Formatting Table). **Given** `field_id` is missing or unknown **Then** the widget falls back to `kCatalog[0]` (i.e. `altitude_ft`). **Given** the resolved value is null/NaN **Then** the widget renders `"—"`.

6. **Given** `aircraft_type` is resolved **When** `FlightInfo.aircraft_display_name_short` is populated **Then** that is returned. **Otherwise, when** `FlightInfo.aircraft_code` is populated **Then** that is returned. **Otherwise** the resolver returns empty string (widget renders em-dash per AC #4).

7. **Given** the editor property panel at `/editor.html` **When** a `flight_field` or `metric` widget is selected on the canvas **Then** the panel renders a `<select>` control labeled "Field" whose options are populated from the matching **`field_options`** array in the `/api/widgets/types` response (falling back to legacy `content` `options` ids when absent). **And** the current `config.field_id` is reflected as the selected option. **And** changing the selection updates the widget's config and marks the layout dirty (existing save flow).

8. **Given** `POST /api/layouts` or `PUT /api/layouts/*` **When** the submitted layout contains a `flight_field` or `metric` widget whose `config.field_id` is not present in that widget's catalog **Then** the handler rejects the request with HTTP 400 and error code `UNKNOWN_FIELD_ID` (three-line whitelist check; no new schema).

9. **Given** the native test suites `test_flight_field_widget`, `test_metric_widget`, and `test_web_portal` **When** `pio test -f test_flight_field_widget --without-uploading --without-testing` (and the corresponding MetricWidget / WebPortal suites) compile **Then** they pass. New cases cover: catalog lookup by id, unknown-id fallback to catalog[0], null-value → em-dash, aircraft_type resolver precedence, per-unit Metric formatting, `/api/widgets/types` payload includes non-empty **`field_options`** for both widget types, and layout save rejects unknown field ids (AC #8).

10. **Given** `firmware/data-src/editor.{js,html}` are modified **When** the story is marked ready for review **Then** `firmware/data/editor.js.gz` and `firmware/data/editor.html.gz` are regenerated from source and all four files are committed together.

## Tasks / Subtasks

- [x] **Task 1: Field catalog infrastructure in each widget** (AC: #1, #2)
  - [x] Add `FieldDescriptor` struct in a shared header (`firmware/widgets/FieldDescriptor.h`) — new, no existing similar type in `WidgetRegistry.h`.
  - [x] `firmware/widgets/FlightFieldWidget.h`: declare `kCatalog[]` with the 6 entries and `FlightFieldWidgetCatalog::catalog(size_t& outCount)` + `isKnownFieldId()` accessors. Namespace wrapper (rather than class) so the existing `renderFlightField()` free-function signature is preserved.
  - [x] `firmware/widgets/MetricWidget.h`: same pattern, six entries (includes `distance_nm`, `bearing_deg`). Accessor namespace `MetricWidgetCatalog`.
  - [x] Implement `catalog()` + `isKnownFieldId()` in each widget's `.cpp` returning pointer to the static array and writing the count.

- [x] **Task 2: FlightField render dispatch + fallback** (AC: #4, #6)
  - [x] `firmware/widgets/FlightFieldWidget.cpp`: added file-local `resolveField(key, FlightInfo)` returning `const char*` (strict strcmp; zero heap on hot path), plus `resolveFieldOrFallback()` which retries with `kCatalog[0].id` on unknown input.
  - [x] Per-field resolution matches Dev Notes — Field Resolution Table.
  - [x] Unknown / missing / empty `content` (our alias of `config.field_id` per scope decision #3) → resolver treats as `kCatalog[0].id` ("callsign").
  - [x] Empty resolved value → renders `"--"` (scope decision #5 — em/en-dash glyphs not reliably in Adafruit GFX default font; preserves LE-1.8 behavior).

- [x] **Task 3: Metric render dispatch + unit formatting + fallback** (AC: #5)
  - [x] `firmware/widgets/MetricWidget.cpp`: added file-local `resolveMetric(key, FlightInfo, &value, &suffix, &decimals)` (no StateVector — scope decision #1) plus `resolveMetricOrFallback()` for unknown-id → `altitude_ft`.
  - [x] Applied per-unit formatting from Dev Notes — Unit Formatting Table for the 4 shipped metrics (kft→ft ×1000, mph→kts ×0.8689762, track_deg as-is, fps→fpm ×60). NaN propagates — `DisplayUtils::formatTelemetryValue()` writes `"--"`.
  - [x] Unknown id / NaN value → `"--"`.

- [x] **Task 4: /api/widgets/types payload extension** (AC: #3)
  - [x] `firmware/adapters/WebPortal.cpp::_handleGetWidgetTypes`: `flight_field` and `metric` entries now build their dropdown option lists from `FlightFieldWidgetCatalog::catalog()` / `MetricWidgetCatalog::catalog()` (zero duplication). A sibling `field_options: [{id, label, unit?}]` array is emitted alongside the existing `fields` property schema — different key to avoid colliding with the existing property-panel schema (scope decision #4).

- [x] **Task 5: Save-time field_id whitelist** (AC: #8)
  - [x] `firmware/adapters/WebPortal.cpp`: shared file-local `validateLayoutFieldIds(doc, request)` helper called from `_handlePostLayout` and `_handlePutLayout` after JSON parse / id checks and before `LayoutStore::save()`. Iterates `widgets[]`, and when a widget's `type` is `flight_field` or `metric` and its `content` is a non-empty string, checks `FlightFieldWidgetCatalog::isKnownFieldId()` or `MetricWidgetCatalog::isKnownFieldId()` respectively. On mismatch the helper sends HTTP 400 with `{"ok":false,"error":"Unknown field_id","code":"UNKNOWN_FIELD_ID"}` and returns false. Empty `content` is allowed (renderer falls back to catalog[0]) — rejecting it would break the "drop a fresh widget" flow.

- [x] **Task 6: Editor property panel** (AC: #7, #10)
  - [x] `firmware/data-src/editor.js`: `buildWidgetTypeMeta()` now stores `contentOptions` as `[{value, label}]` pairs (widened from raw strings), and prefers `entry.field_options` when present to populate them with human labels (and parenthesized `unit` when provided). Sets `meta.contentLabel = 'Field'` so the props-panel label switches from "Content" to "Field" for picker-backed widgets.
  - [x] `showPropsPanel()`: rebuilds the existing `#prop-content-select` options from `{value, label}` pairs, preselects the widget's `content`, and re-labels the surrounding `<label>` to `meta.contentLabel`. Stale/invalid keys from older layouts are coerced to `kCatalog[0]` and mark the layout dirty (existing save flow).
  - [x] Change handler was already wired (LE-1.8) — it writes `widgets[selectedIndex].content = contentSelect.value` and flips `dirty=true`. Catalog ids flow through unchanged.
  - [x] `firmware/data-src/editor.html`: unchanged — existing `props-field-content` / `prop-content-select` markup carries the new dropdown without modification.
  - [x] Regenerated `firmware/data/editor.js.gz`. `editor.html` untouched so `editor.html.gz` unchanged.

- [x] **Task 7: Tests** (AC: #9)
  - [x] `firmware/test/test_flight_field_widget/test_main.cpp` — rewritten for LE-1.10:
    - [x] `test_catalog_has_six_entries` / `test_catalog_first_entry_is_callsign` — catalog shape guard.
    - [x] `test_every_catalog_id_is_known` — every id renders + isKnownFieldId true.
    - [x] `test_unknown_id_is_not_known` — whitelist negatives, incl. pre-LE-1.10 keys.
    - [x] `test_unknown_field_id_renders_true_via_fallback` + `test_empty_content_renders_true_via_fallback` — fallback to callsign.
    - [x] `test_aircraft_type_prefers_display_name_short` + `..._falls_back_to_aircraft_code` + `..._empty_both_sources_renders_true` — AC #6 precedence.
    - [x] `test_flight_number_resolves_like_callsign` — alias behavior.
    - [x] `test_callsign_fallback_to_iata` / `..._to_raw_ident` — ident-chain regression.
    - [x] `test_null_flight_renders_true`, `test_empty_airline_renders_true`, dimension-floor tests preserved.
  - [x] `firmware/test/test_metric_widget/test_main.cpp` — rewritten for LE-1.10:
    - [x] `test_catalog_has_four_entries` (4 post-scope-decision-#1) + `test_catalog_first_entry_is_altitude_ft`.
    - [x] `test_every_metric_catalog_id_renders` — full catalog render smoke.
    - [x] `test_unknown_metric_id_is_not_known` — whitelist negatives, including pre-LE-1.10 keys and deferred distance_nm/bearing_deg.
    - [x] `test_unknown_metric_id_renders_true_via_fallback` + `test_empty_content_renders_true_via_fallback`.
    - [x] `test_nan_{altitude,speed,heading,vertical_rate}_renders_true` — NaN → "--" per metric.
    - [x] `test_null_flight_renders_true`, `test_negative_vertical_rate_renders_true`, dimension-floor tests preserved.
  - [x] `firmware/test/test_web_portal/test_main.cpp` — additions (compile-only suite; live HTTP covered by manual smoke):
    - [x] `test_flight_field_catalog_accessor_visible` — surfaces `FlightFieldWidgetCatalog::catalog()` + `isKnownFieldId()` from WebPortal's TU.
    - [x] `test_metric_catalog_accessor_visible` — same for `MetricWidgetCatalog`.
  - [x] Regression: `test_widget_registry` and `test_golden_frames` updated from `"alt"` → `"altitude_ft"` so their smoke paths exercise the intended metric instead of silently falling back.

- [x] **Task 8: Compile sweep + gz regen commit** (AC: #9, #10)
  - [x] `~/.platformio/penv/bin/pio run -e esp32dev` — green. Binary size: 1,305,632 bytes (83.0% of OTA partition, +2,144 vs main baseline).
  - [x] Compile-check per-suite (using `--filter`, matches project convention): `test_flight_field_widget`, `test_metric_widget`, `test_web_portal`, plus regressions `test_widget_registry` and `test_golden_frames` — all PASSED (compile-only; on-device run deferred to Task 9).
  - [x] Regenerated `data/editor.js.gz` (editor.html unchanged).
  - [ ] Single commit — deferred to sign-off (user to trigger when happy with the review).

- [ ] **Task 9: On-device smoke (manual, sign-off gate)**
  - [ ] Flash `esp32dev` firmware + uploadfs, load `http://flightwall.local/editor.html`.
  - [ ] Drop a FlightField widget → confirm dropdown lists 6 options → pick each → verify the LED wall renders the right value (or em-dash when data absent).
  - [ ] Drop a Metric widget → repeat with 6 telemetry options.
  - [ ] Save layout → reload page → confirm field_id persists across a dashboard refresh.
  - [ ] Sanity-check a layout pulled via `curl http://flightwall.local/api/layouts/<id>` contains the chosen `field_id` strings.

## Dev Notes

### FlightField Catalog Definition (AC #1)

```cpp
// firmware/widgets/FlightFieldWidget.h
static constexpr FieldDescriptor FlightFieldWidget::kCatalog[] = {
    { "callsign",         "Callsign"    },
    { "airline",          "Airline"     },
    { "aircraft_type",    "Aircraft"    },
    { "origin_icao",      "Origin"      },
    { "destination_icao", "Destination" },
    { "flight_number",    "Flight #"    },
};
```

### Metric Catalog Definition (AC #2)

```cpp
// firmware/widgets/MetricWidget.h
static constexpr FieldDescriptor MetricWidget::kCatalog[] = {
    { "altitude_ft",        "Altitude (ft)",     "ft"  },
    { "speed_kts",          "Speed (kts)",       "kts" },
    { "heading_deg",        "Heading (deg)",     "deg" },
    { "vertical_rate_fpm",  "Vert Rate (fpm)",   "fpm" },
    { "distance_nm",        "Distance (nm)",     "nm"  },
    { "bearing_deg",        "Bearing (deg)",     "deg" },
};
```

### Field Resolution Table — FlightField (AC #4)

| field_id            | Source on `FlightInfo`                                                             |
|---------------------|------------------------------------------------------------------------------------|
| `callsign`          | `ident` if non-empty, else empty                                                   |
| `airline`           | `airline_display_name_full` if non-empty, else empty                               |
| `aircraft_type`     | `aircraft_display_name_short` → `aircraft_code` → empty (see AC #6)                |
| `origin_icao`       | `origin.code_icao`                                                                 |
| `destination_icao`  | `destination.code_icao`                                                            |
| `flight_number`     | `ident` (same field, distinct label — acceptable alias for now)                    |

### Unit Formatting Table — Metric (AC #5)

Source data for flight telemetry is already present on `FlightInfo` (populated by `FlightDataFetcher` from `StateVector` conversions — see `core/FlightDataFetcher.cpp`).

| field_id            | Source                          | Format                                       |
|---------------------|---------------------------------|----------------------------------------------|
| `altitude_ft`       | `altitude_kft * 1000`           | integer feet, e.g. `"37200"`                 |
| `speed_kts`         | `speed_mph * 0.8689762`         | integer knots, e.g. `"412"`                  |
| `heading_deg`       | `track_deg`                     | integer degrees 0-359, e.g. `"274"`          |
| `vertical_rate_fpm` | `vertical_rate_fps * 60.0`      | signed integer fpm, e.g. `"-1800"` or `"0"`  |
| `distance_nm`       | `distance_km * 0.5399568`       | 1-decimal nm, e.g. `"18.3"`                  |
| `bearing_deg`       | `bearing_deg`                   | integer degrees 0-359                        |

NaN or unset source value → resolver returns empty string → widget renders em-dash.

### Em-Dash Rendering (AC #4, #5)

Use the en-dash glyph U+2013 (`–`) for best visibility on the 4×6 tiny font — em-dash (`—`) may not exist in the embedded font atlas. **Verify the glyph exists in the font table** (`firmware/utils/DisplayUtils.cpp`) before committing. If neither en-dash nor em-dash is present, fall back to ASCII `"--"` (two hyphens) rather than shipping a missing glyph. Document the final choice in the story's implementation log.

### /api/widgets/types Payload Shape (AC #3)

Each widget type entry includes a **`fields`** array: the **property-panel schema** (`key`, `kind`, `default`, `options` for select kinds). For `flight_field` and `metric`, a sibling **`field_options`** array carries the firmware catalog `{id, label}` (and optional `unit` for metrics). Do not overload top-level **`fields`** for the catalog — it collides with the schema. Other widgets (Clock, Logo, Text) omit `field_options`.

```json
{
  "ok": true,
  "data": [
    {
      "type": "flight_field",
      "label": "Flight Field",
      "fields": [
        { "key": "content", "kind": "select", "default": "callsign", "options": ["callsign", "airline"] },
        { "key": "color", "kind": "color", "default": "#FFFFFF" }
      ],
      "field_options": [
        { "id": "callsign", "label": "Callsign" },
        { "id": "airline", "label": "Airline" },
        { "id": "aircraft_type", "label": "Aircraft" },
        { "id": "origin_icao", "label": "Origin" },
        { "id": "destination_icao", "label": "Destination" },
        { "id": "flight_number", "label": "Flight #" }
      ]
    },
    {
      "type": "metric",
      "label": "Metric",
      "fields": [
        { "key": "content", "kind": "select", "default": "altitude_ft", "options": ["altitude_ft"] },
        { "key": "color", "kind": "color", "default": "#FFFFFF" }
      ],
      "field_options": [
        { "id": "altitude_ft", "label": "Altitude (ft)", "unit": "ft" },
        { "id": "speed_kts", "label": "Speed (kts)", "unit": "kts" },
        { "id": "heading_deg", "label": "Heading (deg)", "unit": "deg" },
        { "id": "vertical_rate_fpm", "label": "Vert Rate (fpm)", "unit": "fpm" },
        { "id": "distance_nm", "label": "Distance (nm)", "unit": "nm" },
        { "id": "bearing_deg", "label": "Bearing (deg)", "unit": "deg" }
      ]
    }
  ]
}
```

(Shape simplified — see `WebPortal::_handleGetWidgetTypes` for the full `fields` entries including geometry.)

### Save-Time Whitelist (AC #8)

In `WebPortal::_handlePostLayout` and `WebPortal::_handlePutLayout`, after the existing per-widget structural validation (widget type present, geometry in range), add:

```cpp
if ((type == "flight_field" || type == "metric") &&
    widgetJson["config"]["field_id"].is<const char*>()) {
    const char* fid = widgetJson["config"]["field_id"];
    if (!isKnownFieldId(type.c_str(), fid)) {
        _sendJsonError(request, 400, "Unknown field_id", "UNKNOWN_FIELD_ID");
        return;
    }
}
```

Helper lives in `WebPortal.cpp` (internal static), implemented as a lookup against `FlightFieldWidget::catalog()` or `MetricWidget::catalog()`. No new file.

### Why This Slice Is Minimum-Viable

Confirmed during LE-1.10 planning roundtable (party mode, 2026-04-17):

- **John (PM)**: "Just for me" scope → no presets, no grouping in catalog, no smoke test for `/api/widgets/types`. Confirmed MVP = dropdown + extension + editor wiring + unit tests.
- **Sally (UX)**: curated dropdown with human labels, no format-string templating, smart defaults (catalog[0]) — all honored above.
- **Winston (Arch)**: single source of truth in firmware, em-dash fallback rule in widget base, three-line save-time whitelist against stale-editor drift. All honored; catalog grouping and sibling endpoint conceded as unnecessary ceremony at this scale.
- **Amelia (Dev)**: scoped to one story under LE-1 (re-opened), not a new epic.

### Out of Scope (punt to later stories if desired)

- System-health fields on MetricWidget (free_heap, wifi_rssi, uptime) — separate "system metric" widget or an additional catalog, not in LE-1.10.
- Grouping metadata in the catalog — additive later, no migration debt.
- User-facing unit toggles (ft ↔ m, kts ↔ mph).
- i18n on labels.
- Sibling `GET /api/widgets/fields` endpoint — Winston conceded the extension of `/api/widgets/types` is simpler and one fewer fetch.

## Dev Agent Record

### Scope Adjustments (confirmed with Christian 2026-04-17 before implementation)

1. **Metric catalog — six entries.** Initial plan deferred `distance_nm` / `bearing_deg`; **resolved 2026-04-17 (post-review D1:2)** by copying `StateVector::distance_km` / `bearing_deg` onto `FlightInfo` in `FlightDataFetcher` so `MetricWidget` can resolve all six ids without changing `RenderContext`.
2. **FlightField catalog ids replace LE-1.8 key strings without aliases.** Old LE-1.8 keys (`ident` / `aircraft` / `dest_icao` / `alt` / `speed` / `track` / `vsi`) are renamed to catalog ids (`callsign` / `aircraft_type` / `destination_icao` / `altitude_ft` / `speed_kts` / `heading_deg` / `vertical_rate_fpm`). Any saved layouts with the old keys will resolve as unknown → fall back to catalog[0] on render, and fail the save-time whitelist on re-save. Acceptable for solo-operator scope.
3. **`config.field_id` maps to existing `WidgetSpec.content`.** No new struct field, no new JSON shape. The story's "field_id" is the existing `content` string key — consistent with how LE-1.8 wired `"airline" / "alt" / ...` as `content` values.
4. **`/api/widgets/types` uses `field_options` key at widget-type level for the catalog.** Using the story's proposed `fields` key would collide with the existing property-panel schema array (also called `fields`). `field_options: [{id, label}]` sits alongside `fields: [...]` and requires no editor rewrite.
5. **Em-dash glyph: keep existing `"--"` fallback.** Adafruit GFX default font has no reliable em/en-dash entry (verified in DisplayUtils font handling — the degree symbol needed explicit CP437 handling at 0xF7). Two-hyphen ASCII matches LE-1.8 and the existing tests.

### Implementation Plan

- `firmware/widgets/FieldDescriptor.h` — new tiny POD (`id`, `label`, optional `unit`).
- `FlightFieldWidget.{h,cpp}` and `MetricWidget.{h,cpp}` — namespace-scoped `kCatalog[]` + `catalog(size_t&)` + `isKnownFieldId(const char*)`. Resolver rewritten over new ids.
- `WebPortal.cpp` — `_handleGetWidgetTypes` emits `field_options` from the catalog accessors. `_handlePostLayout` / `_handlePutLayout` reject unknown content values for flight_field / metric widgets with HTTP 400 `UNKNOWN_FIELD_ID`.
- `editor.js` — extend `buildWidgetTypeMeta` + `showPropsPanel` to prefer `field_options` ({id, label}) over the old string-array `options` when populating the select. Regenerate `editor.js.gz`.
- Unity tests — refreshed fixtures + new test cases for catalog lookup, unknown-id fallback, aircraft_type precedence, empty/NaN → `"--"`, WebPortal layout reject path.

### Completion Notes

- All 8 dev tasks complete. All 10 ACs satisfied in firmware + editor. Binary fits in the 1.5 MB OTA partition at 83.0% (+2,144 bytes vs main baseline, well below the 180 KB delta cap).
- **AC #6 note** — verified aircraft_type resolver precedence in code and tests (`aircraft_display_name_short` → `aircraft_code` → empty → `"--"`).
- **Em-dash fallback** rendered as `"--"` (two ASCII hyphens). En-dash / em-dash glyphs are not reliably present in the Adafruit GFX default font; this matches the LE-1.8 behavior already in production (see MetricWidget LE-1.8 source). Documented in scope decision #5.
- **Distance / bearing** (`distance_nm`, `bearing_deg`) — initially deferred, then restored per review decision D1:2 (2026-04-17). `FlightInfo` gained `distance_km` + `bearing_deg` fields; `FlightDataFetcher` now copies them from the upstream `StateVector` during enrichment. Metric catalog ships all 6 entries. Tests added for NaN cases on each.
- **Key rename** — LE-1.8 keys (`ident`, `aircraft`, `dest_icao`, `alt`, `speed`, `track`, `vsi`) are now unknown. Saved layouts still render (resolver falls back to catalog[0]) but fail the save-time whitelist on re-save — acceptable per solo-operator scope.
- Task 9 (on-device smoke) is a manual sign-off gate; left unchecked until Christian validates on the physical device.
- Commit is deferred to user trigger (scope decision: Claude Code should not create commits unless explicitly asked).

### File List

New:
- `firmware/widgets/FieldDescriptor.h`

Modified (firmware):
- `firmware/widgets/FlightFieldWidget.h`
- `firmware/widgets/FlightFieldWidget.cpp`
- `firmware/widgets/MetricWidget.h`
- `firmware/widgets/MetricWidget.cpp`
- `firmware/adapters/WebPortal.cpp`
- `firmware/models/FlightInfo.h` (D1:2 — added `distance_km`, `bearing_deg`)
- `firmware/core/FlightDataFetcher.cpp` (D1:2 — copy `StateVector::distance_km` / `bearing_deg` onto `FlightInfo` during enrichment)

Modified (editor + assets):
- `firmware/data-src/editor.js`
- `firmware/data/editor.js.gz` (regenerated from source)

Modified (tests):
- `firmware/test/test_flight_field_widget/test_main.cpp` (rewrite for LE-1.10 catalog)
- `firmware/test/test_metric_widget/test_main.cpp` (rewrite for LE-1.10 catalog)
- `firmware/test/test_web_portal/test_main.cpp` (catalog-accessor compile guards)
- `firmware/test/test_widget_registry/test_main.cpp` (regression: `alt` → `altitude_ft`)
- `firmware/test/test_golden_frames/test_main.cpp` (regression: `alt` → `altitude_ft`, test name follows)

BMAD artifacts:
- `_bmad-output/implementation-artifacts/stories/le-1-10-widget-field-picker.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`

## Change Log

| Date       | Version | Description                                                                 |
|------------|---------|-----------------------------------------------------------------------------|
| 2026-04-17 | 0.1     | Draft created from party-mode roundtable (John / Sally / Winston / Amelia). |
| 2026-04-17 | 0.2     | In-progress. Scope adjustments confirmed (Metric 4 items, rename LE-1.8 keys, reuse `content`, `field_options` key, `"--"` em-dash fallback). Epic LE-1 reopened in sprint-status. |
| 2026-04-17 | 0.3     | Ready for review. All 8 dev tasks complete; full firmware build green (83.0% partition); FlightField/Metric/WebPortal + regression test suites compile-pass on esp32dev; editor.js.gz regenerated. Task 9 (on-device smoke) pending user sign-off. |
| 2026-04-17 | 0.4     | BMAD code review (step 2–4): findings recorded below; status set to in-progress until decision items and patches are resolved. |
| 2026-04-17 | 0.5     | D1:2 implemented (`distance_nm`, `bearing_deg`); FlightInfo + FlightDataFetcher join; WebPortal option-count clamp; editor `field_options` guard; tests updated. |
| 2026-04-17 | 0.6     | D2:1 — AC #3/#7/#9 and Dev Notes aligned with shipped `field_options` key; Dev Agent scope #1 updated for six-metric catalog. |
| 2026-04-22 | 0.7     | Closing the book on LE-1.10. Implementation code was already in HEAD (merged in commit `2b5ff49`). Status bumped to `done`; Task 9 (on-device smoke) remains a manual user gate. Remaining working-tree changes in `FlightField/Metric/FlightInfo/FlightDataFetcher/editor.js` are a **new unformalized scope** (extended field catalog + IATA codes + speed_mph + WidgetFont) and are explicitly out-of-scope for LE-1.10. |

### Review Findings

- [x] [Review][Decision] Align AC #2 with the shipped metric catalog — **Resolved 2026-04-17:** Chose option **(2)** — implemented `distance_nm` and `bearing_deg` in `MetricWidget` catalog; added `FlightInfo::distance_km` / `bearing_deg` and copy from `StateVector` in `FlightDataFetcher.cpp`. Tests and `/api/widgets/types` pick up counts via catalog accessors.

- [x] [Review][Decision] Align AC #3 with the `/api/widgets/types` JSON shape — **Resolved 2026-04-17 (D2:1):** AC #3, #7, #9, and Dev Notes JSON now document **`field_options`** as the catalog carrier; top-level **`fields`** remains the property-panel schema. No API breaking change.

- [x] [Review][Patch] Clamp legacy `content` select option count to the stack buffer — **Fixed:** `ffOptCount` / `mOptCount` capped to 8 when calling `addField`.

- [x] [Review][Patch] Keep non-story files out of the LE-1.10 commit — **Resolved 2026-04-22:** Closing commit includes only the story file + sprint-status.yaml. Unrelated working-tree changes (`.cursor/hooks/state/continual-learning.json`, legacy-fetcher deletions, logo syncs, hardware config drift, extended-catalog WIP, etc.) are left untouched for separate commits under their appropriate epic labels.

- [x] [Review][Patch] Harden `editor.js` `field_options` ingest — **Fixed:** skip entries missing `id` when building `contentOptions`; `editor.js.gz` regenerated.

- [x] [Review][Defer] Unity suites run compile-only in typical agent/CI flows — On-device `pio test` execution remains a manual gate; same class of deferral as other firmware stories. [`firmware/test/`](firmware/test/) — deferred, pre-existing
