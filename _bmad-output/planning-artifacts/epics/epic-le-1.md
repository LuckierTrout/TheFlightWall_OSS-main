## Epic le-1: Layout Editor V1 MVP

User-authored display layouts on the dashboard, rendered by `CustomLayoutMode`. This shard holds **BMAD-parseable** `### Story` headings; narrative, schema, and deep design live in [`../epic-le-1-layout-editor.md`](../epic-le-1-layout-editor.md).

### Story le-1.1: LayoutStore — LittleFS CRUD and schema validation

As an **owner**,
I want **layouts persisted on-device with enforced size and file caps**,
So that **custom layouts are durable and cannot exhaust flash**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the LayoutStore API **When** layouts are created, read, updated, or deleted **Then** LittleFS paths, 64 KB / 16-file caps, and schema validation behave as specified in the LE-1.1 story file.

---

### Story le-1.2: WidgetRegistry and core widget types (text, clock, logo)

As an **OSS builder**,
I want **a registry-backed dispatch table for the three proven spike widget types**,
So that **rendering stays predictable and heap-free in the hot path**.

**Status:** backlog

**Acceptance Criteria:**

**Given** `CustomLayoutMode` loads a layout **When** it renders **Then** `text`, `clock`, and `logo` widgets dispatch through `WidgetRegistry` with fixed `WidgetInstance` POD storage.

---

### Story le-1.3: CustomLayoutMode promoted from spike to production

As a **firmware engineer**,
I want **CustomLayoutMode wired as a permanent mode-table entry without spike-only guards**,
So that **owners can ship layouts without rebuild flags**.

**Status:** backlog

**Acceptance Criteria:**

**Given** spike `#ifdef` gates **When** LE-1.3 is complete **Then** production mode registration and worst-case memory reporting meet the story acceptance criteria.

---

### Story le-1.4: REST endpoints `/api/layouts/*` and activate flow

As an **owner**,
I want **CRUD and activate endpoints with the standard JSON envelope**,
So that **the dashboard editor can save and select layouts**.

**Status:** backlog

**Acceptance Criteria:**

**Given** HTTP calls to `/api/layouts` and related routes **When** responses return **Then** `{ ok, data, error, code }` is honored and `activate` integrates with `ModeRegistry::switchMode` as specified.

---

### Story le-1.5: LogoStore reuse and `logo` widget (real RGB565 bitmap)

As an **owner**,
I want **the logo widget to render real RGB565 artwork via LogoManager**,
So that **custom layouts match existing logo quality**.

**Status:** backlog

**Acceptance Criteria:**

**Given** a layout referencing a logo id **When** the widget renders **Then** the real bitmap path from Story 4-3 is used (no permanent stub).

---

### Story le-1.6: Editor page — canvas, pointer DnD, selection, resize

As an **owner**,
I want **a phone-friendly editor with 4× canvas and pointer-driven drag/resize**,
So that **I can author a layout in under 30 seconds**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the editor page **When** the user drags and resizes widgets **Then** Pointer Events, snap-to-8px, and selection behavior match the LE-1.6 story.

---

### Story le-1.7: Editor property panel, widget palette, save UX

As an **owner**,
I want **palette + property panel + save feedback**,
So that **I can configure widgets without guessing field names**.

**Status:** backlog

**Acceptance Criteria:**

**Given** `/api/widgets/types` **When** the editor loads **Then** palettes and property panels stay in sync with firmware introspection.

---

### Story le-1.8: `flight_field` and `metric` widgets

As an **owner**,
I want **telemetry-bound fields and derived metrics in layouts**,
So that **my wall shows live and computed values**.

**Status:** backlog

**Acceptance Criteria:**

**Given** layout JSON **When** widgets of type `flight_field` or `metric` are present **Then** they render with the binding and format-string rules in the LE-1.8 story.

---

### Story le-1.9: Golden-frame tests and binary/heap budget gate

As a **maintainer**,
I want **byte-equal golden frames and CI gates on flash and heap**,
So that **releases cannot regress render output or blow the partition budget**.

**Status:** backlog

**Acceptance Criteria:**

**Given** canonical layouts **When** tests run in CI **Then** golden frames match and binary/heap thresholds from the story are enforced.
