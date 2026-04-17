
# Story LE-1.1: LayoutStore LittleFS CRUD

branch: le-1-1-layout-store-littlefs-crud
zone:
  - firmware/core/LayoutStore.*
  - firmware/modes/v0_spike_layout.h
  - firmware/src/main.cpp
  - firmware/modes/CustomLayoutMode.*
  - firmware/core/ModeOrchestrator.cpp
  - firmware/test/test_layout_store/**

Status: draft

## Story

As a **firmware engineer**,
I want **CRUD operations over layout JSON files on LittleFS**,
So that **user-authored layouts persist across reboots and can be listed/loaded by the mode system**.

## Acceptance Criteria

1. **Given** a request to persist a layout **When** `LayoutStore::save(id, json)` is called **Then** the JSON is written to `/layouts/<id>.json` on LittleFS **And** overwrites any existing file with the same id **And** returns a success result.
2. **Given** layouts stored on LittleFS **When** `LayoutStore::list()` is called **Then** it returns an array of `{id, name, updated, bytes}` records for every `/layouts/*.json` entry **And** `LayoutStore::load(id)` returns the raw JSON text for a given id.
3. **Given** a `save()` request with invalid content **When** schema validation runs **Then** `save()` rejects payloads that are >8 KB, exceed 24 widgets, declare canvas dimensions != 192×48, contain unknown widget types, or miss required top-level fields (`id`, `name`, `canvas`, `widgets`) **And** returns a typed error code.
4. **Given** 16 layouts already stored (global 64 KB cap) **When** an additional `save()` is attempted **Then** the call fails with `FS_FULL` **And** the existing layouts are untouched.
5. **Given** an active layout id **When** `LayoutStore::setActiveId(id)` is called **Then** the id is persisted to NVS under key `layout_active` (12 chars, within 15-char NVS limit) **And** `LayoutStore::activeId()` returns that id on subsequent calls.
6. **Given** `layout_active` refers to a nonexistent file at boot **When** the system attempts to resolve it **Then** `LayoutStore::load(activeId())` returns the baked-in default layout string constant **And** a WARNING is logged (`[LayoutStore] layout_active '<id>' missing — using default`).
7. **Given** the V0 spike instrumentation **When** this story lands **Then** `#ifdef LE_V0_METRICS` blocks in `main.cpp` (ring buffer, `le_v0_record`, `le_v0_maybe_log`, mode-selection boot override, `tick` wrapper) are removed **And** `ModeOrchestrator::onIdleFallback()` `LE_V0_METRICS` early return is removed **And** `displayTask` `LE_V0_METRICS` `statusMessageVisible` bypass is removed **And** `firmware/modes/v0_spike_layout.h` is deleted **And** `[env:esp32dev_le_baseline]` / `[env:esp32dev_le_spike]` remain in `platformio.ini` as dev-only tools documented in `AGENTS.md`.
8. **Given** the V0 spike cleanup is complete **When** `pio run -e esp32dev` builds **Then** the main-branch binary size is within ±1 KB of the pre-spike baseline **And** a compile-only test check (`pio test -f test_layout_store --without-uploading --without-testing`) passes.

## Tasks / Subtasks

- [ ] **Task 1: Remove V0 spike code** (AC: #7, #8)
  - [ ] `firmware/src/main.cpp` — remove `#ifdef LE_V0_METRICS` ring buffer + `le_v0_record` + `le_v0_maybe_log` + `ModeRegistry::tick` wrapper
  - [ ] `firmware/src/main.cpp` — remove `#ifdef LE_V0_METRICS` boot-override in `setup()` mode-selection
  - [ ] `firmware/core/ModeOrchestrator.cpp` — remove `#ifdef LE_V0_METRICS` early return in `onIdleFallback()`
  - [ ] `firmware/src/main.cpp` displayTask — remove `statusMessageVisible = false;` bypass under `LE_V0_METRICS`
  - [ ] Delete `firmware/modes/v0_spike_layout.h`
  - [ ] `AGENTS.md` — document `[env:esp32dev_le_baseline]` / `[env:esp32dev_le_spike]` as dev-only measurement tools
  - [ ] Verify `pio run -e esp32dev` clean and binary size within ±1 KB of pre-spike baseline

- [ ] **Task 2: Create LayoutStore header** (AC: #1, #2, #5)
  - [ ] `firmware/core/LayoutStore.h` — class with `bool list(JsonArray&)`, `bool load(const char* id, String& out)`, `Result save(const char* id, const char* json)`, `bool remove(const char* id)`, `String activeId()`, `bool setActiveId(const char* id)`
  - [ ] Declare `enum class LayoutStoreError { OK, NOT_FOUND, TOO_LARGE, INVALID, FS_FULL, IO_ERROR }`
  - [ ] Declare constants: `MAX_LAYOUT_BYTES = 8192`, `MAX_TOTAL_BYTES = 65536`, `MAX_WIDGETS = 24`

- [ ] **Task 3: Implement LayoutStore CRUD** (AC: #1, #2, #3, #4)
  - [ ] `firmware/core/LayoutStore.cpp` — implement list/load/save/remove against LittleFS `/layouts/` dir
  - [ ] Schema validation in `save()`: parse JSON via `JsonDocument`, check top-level fields, canvas 192×48, widget count ≤ 24, widget types are in a static allowlist
  - [ ] Enforce per-file 8 KB cap and global 64 KB cap (iterate dir, sum sizes before write)
  - [ ] Free JsonDocument before returning — never retain parsed document in the store

- [ ] **Task 4: NVS active id + fallback** (AC: #5, #6)
  - [ ] Add NVS key `layout_active` (12 chars) via ConfigManager raw NVS or direct `Preferences` within LayoutStore
  - [ ] Declare baked-in default layout as `static const char kDefaultLayoutJson[]` constant inside `LayoutStore.cpp`
  - [ ] On `load(activeId())`, if file missing, return default JSON + log `LOG_W`
  - [ ] `CustomLayoutMode::init()` callers receive the default path transparently

- [ ] **Task 5: Unit tests** (AC: #1, #2, #3, #4, #6)
  - [ ] `firmware/test/test_layout_store/test_main.cpp` — round-trip save/load
  - [ ] test: schema rejection (oversized, missing field, bad canvas, unknown widget type, >24 widgets)
  - [ ] test: global cap enforcement (fill to 64 KB, 17th save rejected with FS_FULL)
  - [ ] test: remove() erases file and makes subsequent load() return NOT_FOUND
  - [ ] test: fallback-on-missing returns default JSON and logs WARNING
  - [ ] Use LittleFS mock or real LittleFS on ESP32 as existing test suites do

- [ ] **Task 6: Build and verify** (AC: all)
  - [ ] `~/.platformio/penv/bin/pio run` from `firmware/` — clean build
  - [ ] Binary under 1.5MB OTA partition
  - [ ] `pio test -f test_layout_store --without-uploading --without-testing` passes
  - [ ] Confirm spike artifacts removed: no references to `LE_V0_METRICS`, `le_v0_record`, or `v0_spike_layout.h` remain

## Dev Notes

**Validated context from V0 spike** (`_bmad-output/planning-artifacts/layout-editor-v0-spike-report.md`):

- Parse JSON once at `init()`, free the `JsonDocument`, store into fixed `WidgetInstance[]` array — **never parse per-frame**.
- Cache time-derived state at minute resolution (30 s threshold); eliminates per-frame `getLocalTime()` syscalls.
- Fixed widget array, cap 24, no dynamic allocation on the render hot path.
- LittleFS for layouts; NVS only for the 12-char `layout_active` key (within 15-char NVS limit).

**Existing code to reference:**
- `firmware/modes/CustomLayoutMode.{h,cpp}` — V0 spike scaffold; this story merely removes the spike include, LE-1.3 promotes to production.
- `firmware/core/ConfigManager.cpp` — follow its `Preferences` / NVS wrapping conventions when registering `layout_active`.

**Baked-in default layout (Task 4)** — a minimal safe fallback used when the active id is missing:

```cpp
static const char kDefaultLayoutJson[] = R"JSON({
  "id":"default",
  "name":"Default",
  "canvas":{"w":192,"h":48},
  "widgets":[
    {"type":"clock","x":0,"y":0,"w":48,"h":16,"color":"#FFFFFF"},
    {"type":"text","x":0,"y":20,"w":192,"h":10,"value":"FLIGHTWALL","align":"center"}
  ]
})JSON";
```

**Widget type allowlist (Task 3 schema validation)** — must match types registered in `WidgetRegistry` (LE-1.2): `text`, `clock`, `logo`, `flight_field`, `metric`.

## File List

- `firmware/core/LayoutStore.h` (new)
- `firmware/core/LayoutStore.cpp` (new)
- `firmware/test/test_layout_store/test_main.cpp` (new)
- `firmware/src/main.cpp` (modified — remove LE_V0_METRICS blocks)
- `firmware/core/ModeOrchestrator.cpp` (modified — remove LE_V0_METRICS early return)
- `firmware/modes/v0_spike_layout.h` (removed)
- `AGENTS.md` (modified — document dev-only envs)
