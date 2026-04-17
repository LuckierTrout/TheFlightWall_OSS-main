---
epicId: LE-1
title: Layout Editor V1 MVP
status: planned
owner: Winston (Architect)
createdDate: '2026-04-17'
inputDocuments:
  - _bmad-output/planning-artifacts/layout-editor-v0-spike-report.md
  - _bmad-output/planning-artifacts/layout-editor-evidence-hunt.md
  - _bmad-output/planning-artifacts/architecture.md
  - _bmad-output/planning-artifacts/epics-display-system.md
  - _bmad-output/planning-artifacts/epics-delight.md
---

# Epic LE-1: Layout Editor V1 MVP

## Summary

Epic LE-1 delivers the **Layout Editor V1 MVP** — user-authored display layouts composed via a drag-and-drop editor on the captive-portal dashboard, rendered on the device by a new generic `CustomLayoutMode`. The V0 spike (2026-04-17) validated technical feasibility: a JSON-driven renderer coexists with the four fixed modes at **+4.3 KB flash, no measurable heap pressure, and p95 render time 1.23× ClassicCardMode baseline** after two widget-level optimizations. Architecture is proven; this epic promotes the spike scaffold to production, expands the widget set from three to five, adds the editor UX, and ships a persistence + REST layer. The LayoutEngine shared-zone algorithm used by the four fixed modes is **untouched** — CustomLayoutMode is a peer mode that bypasses it by design.

---

## Goals

- Ship a production-quality `CustomLayoutMode` as the 5th entry in the `ModeRegistry` mode table.
- Provide a dashboard editor that an owner can use to create, edit, save, activate, and delete a custom layout in under 30 seconds on a phone.
- Offer five closed widget types at launch: `text`, `clock`, `logo`, `flight_field`, `metric`.
- Persist layouts as LittleFS JSON files; a single NVS key tracks the active layout id.
- Maintain strict parity with existing quality gates: binary under 92% partition, ≥ 30 KB heap margin, render p95 ≤ 1.2× ClassicCardMode baseline.
- Reuse the existing RGB565 logo pipeline (Story 4-3) for the `logo` widget.

## Non-Goals (V1)

- Multi-layout scheduling. V1 supports authoring multiple layouts but only one is active at a time. Schedule-driven layout switching is V2.
- Animations, transitions, or time-based widget visibility.
- Conditional visibility rules (e.g., "show metric X only when ground speed > 100 kts").
- Pixel-precise default drag. Snap-to-8px is the default; 1px is only available behind an Advanced disclosure.
- Custom hex color picker. V1 exposes an ~8-color LED-friendly palette.
- Web-based preview of **real device state**. The editor previews the layout with placeholder/sample data only — the device is not the source of truth during editing.
- Community marketplace, import/export of third-party layouts, or remote sharing.

## User Value

The Layout Editor serves **two distinct audiences**, and the epic is deliberately designed to deliver to both without branching the codebase:

- **OSS builder / authorship audience** — Contributors and hobbyists who want creative control over what their wall displays. Today, adding a visual variant requires a C++ mode, a registry entry, a recompile, and a flash cycle. LE-1 collapses that loop to "open editor → drag widgets → save" with no toolchain required.
- **Commercial buyer / personalization audience** — Non-technical owners who want their FlightWall to say their name, show their route pairs, or match a theme without touching firmware. The dashboard editor is the only surface they ever see.

The audience split matters for scope decisions: the authorship audience tolerates rough edges to gain flexibility, while the personalization audience demands the 30-second-to-save UX path. V1 optimizes for the shared baseline; both audiences get the same tool.

---

## Architecture

### Mode model

`CustomLayoutMode` is the 5th `ModeEntry` in the `main.cpp` mode table. It coexists with `ClassicCardMode`, `LiveFlightCardMode`, `ClockMode`, and `DeparturesBoardMode`. The four fixed modes are **untouched**.

- The existing `LayoutEngine` shared-zone algorithm continues to serve the four fixed modes. CustomLayoutMode **does not** call `LayoutEngine`; it renders directly from its parsed widget list into the display buffer.
- Lifecycle is standard `DisplayMode`: `init()` loads and parses the active layout JSON once, populates a fixed `WidgetInstance[]` array, and releases the `JsonDocument`. `render()` iterates the array and dispatches via a static table. `teardown()` is a no-op on persistence; only the in-memory array is freed.
- Worst-case memory requirement is computed from the fixed array cap (~24 widgets) and the largest widget's per-instance footprint.

### Persistence

- **LittleFS** stores layout documents at `/layouts/<id>.json`. Each file is 2–8 KB; the `/layouts/` directory is capped at **64 KB total** and **16 layouts max**. LayoutStore enforces both on write.
- **NVS** stores only `layout_active` (12-character key, matching the 15-char NVS limit). This is the one key that's read at boot to resolve which layout to load into CustomLayoutMode.
- Logo bitmaps continue to live in the existing LogoManager store (RGB565, LittleFS). The `logo` widget references them by id — no bitmap bytes are embedded in layout JSON.

### Widget set (V1)

A **closed registry** of five widget types:

| Type           | Purpose                                                                      | V0 status        |
| -------------- | ---------------------------------------------------------------------------- | ---------------- |
| `text`         | Static or templated text with font preset and alignment.                     | Proven in spike. |
| `clock`        | Current time, 12/24h. Cached at minute resolution.                           | Proven in spike. |
| `logo`         | Reference to a stored RGB565 logo by id.                                     | Proven in spike (stub bitmap). |
| `flight_field` | Live flight telemetry field (callsign, altitude, speed, heading, vert rate). | **New in V1.**   |
| `metric`       | Derived numeric readouts (e.g., active flight count, uptime, heap).          | **New in V1.**   |

The three spike widgets form the proven subset. `flight_field` and `metric` are net-new in LE-1.8 and follow the same dispatch pattern.

### Layout JSON schema

Annotated example of the V1 schema. Same shape as the spike with V1 widget types added:

```json
{
  "id": "my-home-wall",
  "name": "Home Wall",
  "v": 1,
  "canvas": { "w": 160, "h": 32 },
  "bg": "#000000",
  "widgets": [
    {
      "id": "w1",
      "type": "text",
      "x": 0, "y": 0, "w": 96, "h": 10,
      "content": "WELCOME HOME",
      "font": "M",
      "align": "left",
      "color": "#FFAA00"
    },
    {
      "id": "w2",
      "type": "clock",
      "x": 100, "y": 0, "w": 60, "h": 10,
      "format": "24h",
      "font": "M",
      "color": "#FFFFFF"
    },
    {
      "id": "w3",
      "type": "logo",
      "x": 0, "y": 12, "w": 32, "h": 20,
      "logo_id": "united_1"
    },
    {
      "id": "w4",
      "type": "flight_field",
      "x": 34, "y": 12, "w": 70, "h": 10,
      "field": "callsign",
      "font": "M",
      "color": "#00FF88"
    },
    {
      "id": "w5",
      "type": "metric",
      "x": 106, "y": 12, "w": 54, "h": 10,
      "metric": "flight_count",
      "format": "{} flights",
      "font": "S",
      "color": "#AAAAAA"
    }
  ]
}
```

Fields common to every widget: `id`, `type`, `x`, `y`, `w`, `h`. Type-specific fields are documented per widget in LE-1.2 / LE-1.5 / LE-1.8. `v: 1` is the schema version; LayoutStore rejects unknown versions.

### Render path

Validated dispatch pattern carried forward from the V0 spike:

```cpp
// Inside CustomLayoutMode::render()
for (uint8_t i = 0; i < widgetCount_; ++i) {
    const WidgetInstance& w = widgets_[i];
    switch (w.type) {
        case WidgetType::Text:        renderText(w, ctx); break;
        case WidgetType::Clock:       renderClock(w, ctx); break;
        case WidgetType::Logo:        renderLogo(w, ctx); break;
        case WidgetType::FlightField: renderFlightField(w, ctx); break;
        case WidgetType::Metric:      renderMetric(w, ctx); break;
    }
}
```

- Render functions are stateless singletons. No per-frame heap allocation.
- No virtual dispatch; a `switch` on a packed enum is materially cheaper on ESP32 than a v-table indirection for a loop this hot.
- `WidgetInstance` is a fixed-size POD (cap ~24 widgets). Total mode memory is predictable at compile time.

### Editor architecture

- **Hand-rolled Pointer Events**, not a DnD library. ES5-only constraint plus iOS Safari quirks made a library a liability in the spike prototype.
- **Single `<canvas>`** rendered at **4× CSS scale** with `imageSmoothingEnabled = false`. Nearest-neighbor upscale. No anti-aliasing anywhere in the editor preview — the device itself has no sub-pixel smoothing, so the editor must not lie.
- **Full re-render on any change.** No dirty-rect optimizations; the canvas is 160×32 × 4 = small enough to redraw at 60fps on a phone.
- **Browser-only state during editing.** The ESP32 is **not** the source of truth while editing. The editor holds the full layout document in JavaScript; only `POST /api/layouts/:id` on save hits the device. This keeps the editor responsive even on a slow WiFi link and removes the device from the edit-debug loop.
- `touch-action: none` on the canvas element to prevent iOS Safari from consuming pointer gestures.

### REST API

All endpoints use the standard envelope `{ ok, data, error, code }`.

| Method | Path                            | Purpose                                                         |
| ------ | ------------------------------- | --------------------------------------------------------------- |
| GET    | `/api/layouts`                  | List layouts: `[{ id, name, size_bytes, modified }]`.           |
| POST   | `/api/layouts`                  | Create a new layout from a JSON body. Returns the assigned id.  |
| GET    | `/api/layouts/:id`              | Fetch full layout document.                                     |
| PUT    | `/api/layouts/:id`              | Replace layout document. Schema-validated before commit.        |
| DELETE | `/api/layouts/:id`              | Delete layout. Rejects if currently active.                     |
| POST   | `/api/layouts/:id/activate`     | Set `layout_active` in NVS. Triggers `CustomLayoutMode` reload on next mode switch. |
| GET    | `/api/widgets/types`            | Return widget type descriptors (min dims, fields, enums) for editor introspection. |

`GET /api/widgets/types` is the introspection surface that lets the editor render widget palettes and property panels without hardcoding widget schemas in JS — the firmware is the source of truth for what each widget accepts.

---

## Implementation Pattern Requirements (non-negotiable)

These five patterns are carried forward verbatim from the V0 spike's "V1 must carry forward" section. Each is enforced in code review for every LE-1 story.

1. **Bitmap-blit path, not fillRect.** Use `DisplayUtils::drawBitmapRGB565` (or equivalent) for any large-area paint. `fillRect` is materially more expensive on this hardware. The spike demonstrated this after the p95 regression was traced to logo fillRect calls.
2. **Cache time-derived state at minute resolution.** Clock strings, date strings, any formatted time output — compute once per minute boundary, not per frame. The spike's initial p95 was visibly hurt by per-frame `snprintf` of the clock string.
3. **Parse JSON at mode init only.** `CustomLayoutMode::init()` parses the layout, populates `WidgetInstance[]`, then frees the `JsonDocument`. `render()` must not see JSON.
4. **Fixed `WidgetInstance[]` array, no dynamic containers.** Cap raised from 8 (spike) to ~24 (V1). No `std::vector`, no heap growth at runtime.
5. **Hand-rolled Pointer Events in the editor, no DnD library.** Single `<canvas>` at 4× scale, `imageSmoothingEnabled = false`, full re-render on every change.

Any story PR that violates one of these without an explicit architecture-sign-off note is rejected.

---

## Story Breakdown

Nine stories. Size T-shirts: S = ~0.5 day, M = ~1 day, L = ~2 days.

| Story   | Title                                                        | Size | Description                                                                                              |
| ------- | ------------------------------------------------------------ | ---- | -------------------------------------------------------------------------------------------------------- |
| LE-1.1  | LayoutStore: LittleFS CRUD + schema validation              | M    | File I/O, id allocation, 64KB/16-file cap enforcement, JSON schema validator. Includes V0 spike cleanup. |
| LE-1.2  | WidgetRegistry + core 3 widget types (text, clock, logo)    | M    | Dispatch table, `WidgetInstance` POD, render functions for the three proven spike widgets.               |
| LE-1.3  | CustomLayoutMode promoted from spike to production          | M    | Remove `#ifdef LE_V0_SPIKE` gating, wire into permanent mode table entry, worst-case memory reporting.   |
| LE-1.4  | REST endpoints `/api/layouts/*` + activate flow             | S    | Seven endpoints, standard envelope, integrate with `ModeRegistry::switchMode`.                           |
| LE-1.5  | LogoStore reuse + `logo` widget (real bitmap)                | M    | Replace spike's stub with real RGB565 blit via LogoManager. Reuses Story 4-3 pipeline.                   |
| LE-1.6  | Editor page: canvas, pointer DnD, selection, resize          | L    | Single canvas at 4× scale, Pointer Events, snap-to-8px, selection rect, resize handles.                  |
| LE-1.7  | Editor property panel + widget palette + save/load UX        | M    | Right-side property panel, left-side palette, `/api/widgets/types` integration, save toast.              |
| LE-1.8  | `flight_field` and `metric` widgets                          | M    | Two new widget types; telemetry field bindings; `metric` format-string support.                          |
| LE-1.9  | Golden-frame regression tests + binary/heap budget gate     | S    | Byte-equal frame comparison for canonical layouts; CI gate on flash % and heap margin.                   |

**BMAD / bmad-assist:** Story IDs (`le-1.1` … `le-1.9`) are declared in the sharded epic [`epics/epic-le-1.md`](epics/epic-le-1.md) (`### Story le-1.x:` headings). This file keeps the roadmap table; full acceptance criteria live in per-story files under `_bmad-output/implementation-artifacts/stories/`.

**LE-1.1 includes the V0 spike cleanup as a subtask** — see "V0 Spike Cleanup" below for the exact revert list.

---

## V0 Spike Cleanup (prerequisites folded into LE-1.1)

The V0 spike left instrumentation and override paths behind development flags. LE-1.1 is the first story that touches the mode table and filesystem layout, so the cleanup lands there.

Items to revert or formalize:

1. **Remove `#ifdef LE_V0_METRICS` instrumentation block** in `firmware/src/main.cpp`:
   - The ring buffer
   - `le_v0_record`
   - `le_v0_maybe_log`
   - The wrap around `ModeRegistry::tick()`
2. **Remove `#ifdef LE_V0_METRICS` boot-override block** in `main.cpp`'s `setup()` mode-selection area (the code that forces the spike mode at boot for measurement).
3. **Remove `#ifdef LE_V0_METRICS` early-return guard** in `ModeOrchestrator::onIdleFallback()` (the block that prevented idle fallback from stealing the measurement mode).
4. **Remove `statusMessageVisible = false;` bypass** under `LE_V0_METRICS` in `displayTask` (overlay suppression for clean frame timing).
5. **Delete `firmware/modes/v0_spike_layout.h`** — the hardcoded spike layout is replaced by LittleFS-loaded JSON in LE-1.1.
6. **Resolve the `#ifdef LE_V0_SPIKE` guard around the mode-table entry**. Two acceptable landings:
   - **(a) Unconditional** once LE-1.3 lands and `CustomLayoutMode` is a real mode. This is the recommended path.
   - **(b) Compile-time feature flag** controlled by LE-1 config, if we want a kill-switch for commercial SKUs. Decide at LE-1.3 review.
7. **Fate of `[env:esp32dev_le_baseline]` and `[env:esp32dev_le_spike]` in `platformio.ini`.** Recommendation: **keep both as dev-only measurement environments** for future spike work (baseline vs. experimental comparisons are a repeatable pattern). Document their purpose in `AGENTS.md` so future contributors know they're scaffolding for performance measurement, not production builds.

Everything above is bounded, mechanical, and test-covered. No design judgment required at cleanup time.

---

## Quality Gates (carried forward from spike)

| Gate                         | Threshold                                                                                            | Enforced in |
| ---------------------------- | ---------------------------------------------------------------------------------------------------- | ----------- |
| **Binary size**              | ≤ 92% of 1.5 MB OTA partition. Current baseline ~81%. LE-1 total budget: ~170 KB.                    | LE-1.9      |
| **Heap margin**              | Active mode + loaded layout retains ≥ **30 KB free heap**. ModeSwitch invariant, unchanged from Delight release. | LE-1.3, LE-1.9 |
| **Render p95**               | `CustomLayoutMode::render()` with full V1 widget set at realistic density (6–8 widgets) ≤ **1.2× `ClassicCardMode::render()`** p95 on an equivalent-complexity scene. | LE-1.9 |
| **Golden frames**            | Byte-equal rendered output for a canonical set of test layouts across builds.                        | LE-1.9      |
| **No fixed-mode regression** | ClassicCardMode, LiveFlightCardMode, ClockMode, DeparturesBoardMode p95 and heap unchanged (±2%).    | LE-1.9      |

Render p95 must be **re-measured in LE-1.9** using the same instrumentation pattern the spike introduced (ring buffer + minute-wall clock). The spike's 1.23× number is a signal of feasibility, not a V1 acceptance number — V1 has more widgets and a realistic layout density.

---

## Risks & Mitigations

| Risk                                                                                      | Mitigation                                                                                                                                                     |
| ----------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Demand signal not validated.** The evidence hunt (Mary) was inconclusive; Christian chose to proceed on technical feasibility alone. | If V1 adoption is poor, the **Mode SDK path** (Mary's alternative) reuses the same JSON schema + dispatch pattern. Sunk cost is recoverable — LayoutStore, WidgetRegistry, and the render dispatch survive a pivot to "SDK for contributors" framing. Documented as graceful fallback. |
| **Binary bloat.** Five widget types + editor JS bundle + seven new API endpoints could push past 92% partition. | Gate enforced in LE-1.9 before merge. **Fallback:** compile-flag disable unused widget types to produce a lean SKU build.                                      |
| **ES5 DnD on mobile.** iOS Safari touch event quirks broke early DnD-library prototypes. | Hand-rolled Pointer Events. `touch-action: none` on canvas. Tested on iOS Safari and Android Chrome in LE-1.6.                                                 |
| **Pre-existing TWDT crash on `loopTask` (~24s during fetcher operation).** | Observed during spike; **not caused by LE-1, not blocking**. Flag as a separate tech-debt item if it persists through LE-1.9 testing. Surface in retrospective. |
| **Interference with fixed modes.** New mode could regress ClassicCardMode, etc.          | Mitigated by design — `CustomLayoutMode` is a 5th mode and does not touch `LayoutEngine` or any fixed-mode code path. LE-1.9 golden frames cover the four fixed modes too. |
| **Schema drift across versions.**                                                         | `v: 1` field in layout JSON; LayoutStore rejects unknown versions on load and logs a clear error. V2 schema changes require a migrator.                        |

---

## Dependencies on Other Stories

- **TD-4 (commit-discipline gates)** — should land **before LE-1 work starts**. TD-4 introduces `branch:` and `zone:` frontmatter discipline; every LE-1 story file should include those fields from day one. This is the one hard ordering dependency.
- **TD-1 (atomic calibration flag)** and **TD-2 (hardware-config combined-change bug)** — P0 correctness fixes, **orthogonal** to LE-1. No hard dependency; can run in parallel.
- **TD-3 (OTA self-check native tests)** — orthogonal. Can land before or after LE-1.
- **Story 3-4 (Canvas Preview, done)** — LE-1.6 Editor page **supersedes** the read-only preview for custom layouts. The existing preview paths for Classic / LiveFlight / Clock / Departures are **unchanged**.
- **Story 4-3 (Logo Upload, done)** — LE-1.5 reuses the RGB565 logo pipeline and `LogoManager` unchanged. No forking, no duplication.

---

## Success Criteria (demand-neutral, behaviour-level)

Shipped V1 means all of the following are observably true:

1. At least one user-authored layout renders correctly on the device via `CustomLayoutMode`.
2. The dashboard editor supports **add / drag / resize / style / save** of a widget, end-to-end, in under **30 seconds on mobile**.
3. **Golden-frame tests pass** (LE-1.9) for the canonical layout set across builds.
4. **Binary stays under 92%** of the 1.5 MB OTA partition.
5. **No regression** in the four fixed modes' render budget (p95 within ±2%) or heap margin.
6. All nine LE-1 stories are complete, reviewed, and merged, with the V0 spike cleanup list fully reverted or formalized.

These are behaviour-level outcomes. None of them require a demand-signal threshold; they describe a working product independent of how many people adopt it.

---

## Future Work (V2+, out of scope)

- **Multiple layouts + time-based schedule selection.** Wire custom layouts into the existing `Schedule` system so an owner can run Layout A in the morning and Layout B at night. Schema and persistence already support multiple layouts; this is a scheduler integration, not a storage change.
- **Animation and transition widgets.** Scrolling text, fade-in panels, marquee tickers. Non-trivial because it forces a re-think of the "stateless render fn" pattern; likely its own epic.
- **Conditional widget visibility.** Predicates like "show only when ground speed > 100 kts" or "show only between 0600 and 2200". Requires a small expression language.
- **Community layout marketplace.** Import/export layouts; browse shared layouts from a registry. Depends on V1 adoption signal.
- **Mode SDK (Mary's alternate path).** Not necessarily built, but documented as a strategic possibility. If the personalization audience turns out to be small and the authorship audience is what we actually have, the same JSON schema + dispatch pattern becomes the foundation for a contributor-facing SDK. The V1 investment is not stranded in that pivot.

---
