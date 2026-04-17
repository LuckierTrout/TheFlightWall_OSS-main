# Story le-1.1: layout-store-littlefs-crud

branch: le-1-1-layout-store-littlefs-crud
zone:
  - firmware/core/LayoutStore.h
  - firmware/core/LayoutStore.cpp
  - firmware/core/ConfigManager.h
  - firmware/core/ConfigManager.cpp
  - firmware/src/main.cpp
  - firmware/core/ModeOrchestrator.cpp
  - firmware/modes/v0_spike_layout.h
  - firmware/test/test_layout_store/**
  - AGENTS.md

Status: ready-for-review

---

## Story

As an **owner**,
I want **layouts persisted on-device with enforced size and file caps**,
So that **custom layouts are durable and cannot exhaust flash**.

---

## Acceptance Criteria

1. **Given** a request to persist a layout **When** `LayoutStore::save(id, json)` is called **Then** the JSON is written to `/layouts/<id>.json` on LittleFS **And** overwrites any existing file with the same id **And** returns `LayoutStoreError::OK`.

2. **Given** layouts stored on LittleFS **When** `LayoutStore::list(result)` is called **Then** it fills a `std::vector<LayoutEntry>` with `{id, name, sizeBytes}` for every `/layouts/*.json` entry **And** `LayoutStore::load(id, out)` fills a `String&` with the raw JSON text and returns `true`.

3. **Given** a `save()` request with invalid content **When** schema validation runs **Then** `save()` rejects payloads that: (a) exceed `MAX_LAYOUT_BYTES` (65536 / 16 = 4096 per file or 8192 as individual cap; use 8192 per epic), (b) fail `ArduinoJson` deserialization, (c) are missing required top-level fields (`id`, `name`, `v`, `canvas`, `widgets`), (d) have `v` ≠ 1, (e) have `canvas.w` or `canvas.h` out of range (≥ 1), or (f) contain `widgets[]` with a type not in the allowlist (`text`, `clock`, `logo`, `flight_field`, `metric`) **And** returns the appropriate `LayoutStoreError` code.

4. **Given** 16 layouts already stored **When** an additional `save()` for a new id is attempted **Then** the call returns `LayoutStoreError::FS_FULL` **And** no new file is created **And** no existing files are modified. **And** given the total size of all `/layouts/` files already meets or exceeds `MAX_TOTAL_BYTES` (65536) **When** a new `save()` is attempted **Then** it also returns `LayoutStoreError::FS_FULL`.

5. **Given** a valid layout id **When** `LayoutStore::setActiveId(id)` is called **Then** the id is written to NVS key `layout_active` (12 chars, within the 15-char NVS limit) **And** `LayoutStore::getActiveId()` returns that same id on the next call (including after a simulated reboot / `Preferences` re-open).

6. **Given** `layout_active` NVS key is set to a non-existent layout file id **When** `LayoutStore::load(LayoutStore::getActiveId(), out)` is called **Then** it returns `false` **And** `out` contains the baked-in `kDefaultLayoutJson` constant **And** a warning is logged via `LOG_W("LayoutStore", "layout_active '<id>' missing — using default")`.

7. **Given** the V0 spike instrumentation in `main.cpp` **When** this story is merged **Then**:
   - All `#ifdef LE_V0_METRICS` blocks are removed from `firmware/src/main.cpp` (ring buffer declaration, `le_v0_record`, `le_v0_maybe_log` functions, `_leT0` timing wrapper around `ModeRegistry::tick()`, boot-mode-override block in `setup()`)
   - The `#ifdef LE_V0_METRICS` early-return guard in `ModeOrchestrator::onIdleFallback()` is removed
   - The `#ifdef LE_V0_METRICS` `statusMessageVisible = false;` bypass in `displayTask` is removed
   - `firmware/modes/v0_spike_layout.h` is deleted (no references remain)
   - `[env:esp32dev_le_baseline]` and `[env:esp32dev_le_spike]` in `platformio.ini` remain **untouched** (dev-only measurement environments, documented in `AGENTS.md`)
   - The `#ifdef LE_V0_SPIKE` guard around the `custom_layout` mode-table entry in `main.cpp` is **left in place** (resolved in LE-1.3)

8. **Given** the V0 spike cleanup is complete **When** `pio run -e esp32dev` builds from `firmware/` **Then** the binary compiles cleanly with zero `LE_V0_METRICS` or `le_v0_record` references, the binary size is within the 1.5 MB OTA partition (`check_size.py` gate passes), and `pio test -e esp32dev --filter test_layout_store` compiles and all tests pass.

---

## Tasks / Subtasks

#### Review Follow-ups (AI)
- [x] [AI-Review] HIGH: `load()` unbounded `readString()` — guard `f.size() > LAYOUT_STORE_MAX_BYTES` before read (`firmware/core/LayoutStore.cpp`) — **FIXED in synthesis**
- [x] [AI-Review] HIGH: `list()` unbounded `readString()` — same size guard applied before readString in loop (`firmware/core/LayoutStore.cpp`) — **FIXED in synthesis**
- [x] [AI-Review] HIGH: `_default` id not blocked by `isSafeLayoutId` — explicit `strcmp(id,"_default")==0` rejection added (`firmware/core/LayoutStore.cpp`) — **FIXED in synthesis**
- [x] [AI-Review] HIGH: doc `id` vs filename desync — `validateSchema` now accepts `pathId` and rejects if `doc["id"] != pathId` (`firmware/core/LayoutStore.cpp`) — **FIXED in synthesis**
- [x] [AI-Review] MEDIUM: AC6 log messages omit the offending id — replaced LOG_W with `Serial.printf` to embed id in warning strings (`firmware/core/LayoutStore.cpp`) — **FIXED in synthesis**
- [x] [AI-Review] MEDIUM: No AC5 NVS round-trip test — added `test_active_id_roundtrip` (`firmware/test/test_layout_store/test_main.cpp`) — **FIXED in synthesis**
- [x] [AI-Review] MEDIUM: No AC6 end-to-end test (active-id → missing file → default) — added `test_load_with_missing_active_id_uses_default` (`firmware/test/test_layout_store/test_main.cpp`) — **FIXED in synthesis**
- [x] [AI-Review] MEDIUM: No test asserting `_default` cannot be saved — added `test_save_rejects_reserved_default_id` (`firmware/test/test_layout_store/test_main.cpp`) — **FIXED in synthesis**
- [x] [AI-Review] MEDIUM: No test for doc id vs filename mismatch — added `test_save_rejects_id_mismatch` (`firmware/test/test_layout_store/test_main.cpp`) — **FIXED in synthesis**
- [ ] [AI-Review] LOW: `NOT_FOUND` enum value is declared but never returned — dead API surface creates confusion for future HTTP layer (`firmware/core/LayoutStore.h`) — defer to LE-1.3 when HTTP endpoints are wired
- [ ] [AI-Review] LOW: `CustomLayoutMode.cpp` uses `Serial.printf` for spike logging instead of `LOG_*` macros — minor discipline issue, acceptable while whole file is `#ifdef LE_V0_SPIKE` gated; defer to LE-1.3 cleanup

- [x] **Task 1 — V0 Spike Cleanup** (AC #7, #8)
  - [x] `firmware/src/main.cpp` lines ~369–424: remove entire `#ifdef LE_V0_METRICS … #endif // LE_V0_METRICS` block (ring buffer + `le_v0_record` + `le_v0_maybe_log`)
  - [x] `firmware/src/main.cpp` lines ~608–614: remove `#ifdef LE_V0_METRICS` `_leT0` / `le_v0_record` wrapper around `ModeRegistry::tick()`
  - [x] `firmware/src/main.cpp` lines ~543–550: remove `#ifdef LE_V0_METRICS` `statusMessageVisible = false;` bypass block in `displayTask`
  - [x] `firmware/src/main.cpp` lines ~879–902: remove `#ifdef LE_V0_METRICS` boot-override block in `setup()` (the block that force-calls `ModeRegistry::requestSwitch` + `ModeOrchestrator::onManualSwitch` + `ConfigManager::setDisplayMode`)
  - [x] `firmware/core/ModeOrchestrator.cpp`: remove the `#ifdef LE_V0_METRICS` early-return guard inside `onIdleFallback()`
  - [x] `git rm firmware/modes/v0_spike_layout.h` — delete the hardcoded spike layout header (also inlined the JSON into `CustomLayoutMode.cpp` and removed the `#include` so the spike env still compiles)
  - [x] `AGENTS.md`: add a section documenting that `[env:esp32dev_le_baseline]` and `[env:esp32dev_le_spike]` in `platformio.ini` are dev-only performance measurement scaffolds, not production builds
  - [x] Verify: `grep -r "LE_V0_METRICS\|le_v0_record\|v0_spike_layout" firmware/{src,core,modes}` returns zero results (matches remain only in `platformio.ini`, which is intentional per AC #7)

- [x] **Task 2 — Create `LayoutStore.h`** (AC #1, #2, #3, #4, #5, #6)
  - [x] Create `firmware/core/LayoutStore.h` using `#pragma once`
  - [x] Declare `enum class LayoutStoreError : uint8_t { OK = 0, NOT_FOUND, TOO_LARGE, INVALID_SCHEMA, FS_FULL, IO_ERROR }`
  - [x] Declare `struct LayoutEntry { String id; String name; size_t sizeBytes; }`
  - [x] Declare constants:
    - `static constexpr size_t LAYOUT_STORE_MAX_BYTES = 8192;`  — per-file cap
    - `static constexpr size_t LAYOUT_STORE_TOTAL_BYTES = 65536;` — directory-wide 64 KB cap
    - `static constexpr uint8_t LAYOUT_STORE_MAX_FILES = 16;` — layout count cap
  - [x] Declare static class `LayoutStore` with methods:
    - `static bool init();` — creates `/layouts/` dir if absent, returns false if dir creation fails
    - `static LayoutStoreError save(const char* id, const char* json);`
    - `static bool load(const char* id, String& out);` — fills `out` with raw JSON; on missing active-id uses default
    - `static bool remove(const char* id);`
    - `static bool list(std::vector<LayoutEntry>& result);`
    - `static bool isSafeLayoutId(const char* id);` — rejects path traversal; alphanumeric + `-_` only, max 32 chars
    - `static String getActiveId();`
    - `static bool setActiveId(const char* id);`

- [x] **Task 3 — Implement `LayoutStore.cpp`** (AC #1, #2, #3, #4)
  - [x] File: `firmware/core/LayoutStore.cpp`
  - [x] `init()`: call `LittleFS.mkdir("/layouts")` if `!LittleFS.exists("/layouts")`; return false if it exists as a file (not a dir)
  - [x] `isSafeLayoutId(id)`: reject null, empty, length > 32, any char not in `[a-zA-Z0-9_-]`, any `..` or `/` substring
  - [x] `save(id, json)`:
    1. Validate id via `isSafeLayoutId`; return `INVALID_SCHEMA` if invalid
    2. Check `strlen(json) > LAYOUT_STORE_MAX_BYTES`; return `TOO_LARGE` if exceeded
    3. Parse with `JsonDocument doc; DeserializationError err = deserializeJson(doc, json);` — return `INVALID_SCHEMA` on error
    4. Validate required top-level fields: `id`, `name`, `v`, `canvas`, `widgets` — return `INVALID_SCHEMA` on any missing
    5. Validate `doc["v"].as<int>() == 1` — return `INVALID_SCHEMA` if not version 1
    6. Validate `doc["canvas"]["w"].as<int>() >= 1 && doc["canvas"]["h"].as<int>() >= 1`
    7. Iterate `doc["widgets"]` array; reject any `type` not in allowlist `{"text","clock","logo","flight_field","metric"}`; return `INVALID_SCHEMA`
    8. Check total existing dir byte usage: iterate `/layouts/`, sum sizes of all files where id ≠ the id being saved (to allow overwrite). If total + `strlen(json)` > `LAYOUT_STORE_TOTAL_BYTES`, return `FS_FULL`
    9. Count existing files (excluding the id being saved). If count >= `LAYOUT_STORE_MAX_FILES`, return `FS_FULL`
    10. Write to `/layouts/<id>.json` using `LittleFS.open(path, "w")`, stream `json`, close. Return `IO_ERROR` on write failure; `OK` on success
    11. **Do not retain `doc`** after returning — let it destruct
  - [x] `load(id, out)`:
    - Construct path `/layouts/<id>.json`
    - If file does not exist, set `out = kDefaultLayoutJson` (from anonymous namespace), log `LOG_W`, return `false`
    - Open file, read into `out` via `File.readString()` or streaming read, close, return `true`
  - [x] `remove(id)`:
    - Validate id via `isSafeLayoutId`; return `false` if invalid
    - Return `LittleFS.remove("/layouts/<id>.json")`
  - [x] `list(result)`:
    - Open `/layouts/` directory. Return `false` if open fails
    - For each entry: open the file, parse with `JsonDocument` just long enough to read `id` and `name` fields, close, push `LayoutEntry` with `sizeBytes = file.size()`. Skip non-`.json` files
    - Free each `JsonDocument` before the next iteration
    - Return `true`
  - [x] Baked-in default layout constant (in anonymous namespace or as `static const`):
    ```cpp
    static const char kDefaultLayoutJson[] = R"JSON({
      "id":"_default","name":"Default","v":1,
      "canvas":{"w":160,"h":32},
      "bg":"#000000",
      "widgets":[
        {"id":"w1","type":"clock","x":0,"y":0,"w":80,"h":16,"format":"24h","font":"M","color":"#FFFFFF"},
        {"id":"w2","type":"text","x":0,"y":16,"w":160,"h":10,"content":"FLIGHTWALL","align":"center","font":"S","color":"#AAAAAA"}
      ]
    })JSON";
    ```

- [x] **Task 4 — Add `layout_active` NVS key to `ConfigManager`** (AC #5)
  - [x] `firmware/core/ConfigManager.h`: add to the NVS key abbreviation comment block at top of file: `// layout_active = layout_active (13 chars, within 15-char NVS limit)` (story text listed 12; actual count is 13 — still under the NVS limit)
  - [x] `firmware/core/ConfigManager.h`: add two new static method declarations:
    - `static String getLayoutActiveId();`   // Returns NVS value; empty string if unset
    - `static bool setLayoutActiveId(const char* id);` // Persist to NVS; return false if write fails
  - [x] `firmware/core/ConfigManager.cpp`: implement both methods using `Preferences` with namespace `"flightwall"`, key `"layout_active"`:
    ```cpp
    String ConfigManager::getLayoutActiveId() {
        Preferences prefs;
        prefs.begin("flightwall", true); // read-only
        String val = prefs.getString("layout_active", "");
        prefs.end();
        return val;
    }
    bool ConfigManager::setLayoutActiveId(const char* id) {
        Preferences prefs;
        if (!prefs.begin("flightwall", false)) return false;
        prefs.putString("layout_active", id);
        prefs.end();
        return true;
    }
    ```
  - [x] `LayoutStore::setActiveId` / `getActiveId` delegate to `ConfigManager::setLayoutActiveId` / `ConfigManager::getLayoutActiveId`

- [x] **Task 5 — Write Unity tests** (AC #1, #2, #3, #4, #6, #8)
  - [x] Create `firmware/test/test_layout_store/test_main.cpp`
  - [x] Header comment block (purpose, environment, test list) matching pattern of `test_logo_manager/test_main.cpp`
  - [x] `setup()`: `LittleFS.begin(true)` → `UNITY_BEGIN()` → `RUN_TEST(...)` → `UNITY_END()`
  - [x] `loop()`: empty
  - [x] Helper: `cleanLayoutsDir()` — mirrors `cleanLogosDir()` in logo manager tests
  - [x] Test cases (all 16 implemented; also added `test_save_rejects_bad_canvas` for AC #3e):
    - `test_init_creates_layouts_dir` — clean dir, call `init()`, assert dir exists
    - `test_init_existing_dir` — second `init()` on existing dir returns `true`
    - `test_save_and_load_roundtrip` — save valid JSON, load by id, assert content matches
    - `test_save_overwrites_existing` — save id "abc", save id "abc" again with different content, load returns new content
    - `test_save_rejects_oversized` — JSON string > 8192 bytes returns `TOO_LARGE`
    - `test_save_rejects_missing_fields` — JSON missing `v` field returns `INVALID_SCHEMA`
    - `test_save_rejects_wrong_version` — `"v":2` returns `INVALID_SCHEMA`
    - `test_save_rejects_unknown_widget_type` — widget with `"type":"unknown"` returns `INVALID_SCHEMA`
    - `test_save_rejects_malformed_json` — `deserializeJson` failure returns `INVALID_SCHEMA`
    - `test_save_rejects_bad_canvas` — canvas w/h < 1 returns `INVALID_SCHEMA`
    - `test_cap_16_files` — fill with 16 layouts, 17th new-id save returns `FS_FULL`, dir has 16 files
    - `test_cap_64kb_bytes` — fill dir to ≥ 65536 bytes total (with large-but-valid JSONs), next save of a new id returns `FS_FULL`
    - `test_remove_erases_file` — save layout, remove it, `LittleFS.exists(path)` is false
    - `test_load_missing_returns_default` — load non-existent id returns `false`, `out` contains `"_default"` substring
    - `test_list_returns_all_layouts` — save 3 layouts, `list()` returns vector of size 3 with matching ids
    - `test_unsafe_id_rejected` — ids with `../`, `/nested`, or characters outside `[a-zA-Z0-9_-]` return `INVALID_SCHEMA` from `save()`
  - [x] Clean up `cleanLayoutsDir()` at end of `setup()` before `UNITY_END()`

- [x] **Task 6 — Build verification** (AC #8)
  - [x] `cd firmware && pio run -e esp32dev` — clean compile, zero errors (18.55 s, binary 1,277,760 bytes = 1.22 MB)
  - [x] Confirm `check_size.py` gate passes (binary 1,277,760 bytes ≤ 1,572,864 bytes OTA limit — "OK: Binary size within limits")
  - [x] `pio test -e esp32dev --filter test_layout_store --without-testing --without-uploading` — test translation unit compiles cleanly (PASSED in 19.28 s). On-device execution requires a physically connected ESP32 and is deferred to pre-merge verification by the reviewer.
  - [x] Final scan: `grep -r "LE_V0_METRICS\|le_v0_record\|v0_spike_layout" firmware/src firmware/core firmware/modes` → zero results
  - [x] Cross-env verification: `pio run -e esp32dev_le_baseline` and `pio run -e esp32dev_le_spike` both still build cleanly (1.22 MB each) — confirms the auxiliary measurement envs were not broken by the cleanup.

---

## Dev Notes

### Architecture Context

**LayoutStore scope for this story:** LE-1.1 creates the storage layer only. `CustomLayoutMode` is NOT wired to `LayoutStore` in this story — that integration happens in LE-1.3. The `#ifdef LE_V0_SPIKE` guard around the `custom_layout` mode-table entry remains untouched until LE-1.3.

**Hexagonal architecture:** New files go in `firmware/core/` (pure logic, no hardware binding except LittleFS calls). `LayoutStore` is a static class in the same style as `LogoManager` and `ConfigManager`.

### LogoManager as the Reference Pattern

**`firmware/core/LogoManager.h` is the direct template for `LayoutStore`.**

Key patterns to mirror verbatim:
- `#pragma once` header guard
- Static-only class (no instances, no constructors)
- All methods return `bool` or typed result — no exceptions
- `isSafeLayoutId()` parallels `isSafeLogoFilename()` — reject `..`, `/`, `\`, and any non-alphanumeric-plus-safe-symbols
- `init()` creates the directory if absent; returns `false` if a *file* named `/layouts` exists (conflict)
- File operations: `LittleFS.open(path, "w")` to write, `LittleFS.open(path, "r")` to read, `LittleFS.remove(path)` to delete, `LittleFS.open("/layouts")` with `isDirectory()` check for listing
- Directory listing uses `dir.openNextFile()` loop (same pattern as LogoManager's `listLogos`)

### V0 Spike Cleanup — Precise Locations

All 7 items from `epic-le-1-layout-editor.md` "V0 Spike Cleanup" section. **Exact line references** (verify against current file before editing — line numbers may drift):

| # | File | What to remove |
|---|------|----------------|
| 1 | `firmware/src/main.cpp` ~L369–424 | Entire `#ifdef LE_V0_METRICS … #endif // LE_V0_METRICS` block: `LE_V0_RING_SIZE`, `s_leSamples[]`, `s_leCount`, `s_leIdx`, `s_leLastStatsMs`, `s_leRecordCalls`, `le_v0_record()`, `le_v0_maybe_log()` |
| 2 | `firmware/src/main.cpp` ~L608–614 | `#ifdef LE_V0_METRICS` `_leT0` timing + `le_v0_record()` + `le_v0_maybe_log()` wrapper around `ModeRegistry::tick()` |
| 3 | `firmware/src/main.cpp` ~L543–550 | `#ifdef LE_V0_METRICS statusMessageVisible = false; #endif` bypass block in `displayTask` |
| 4 | `firmware/src/main.cpp` ~L879–902 | `#ifdef LE_V0_METRICS` boot-override block that calls `ModeRegistry::requestSwitch` + `ModeOrchestrator::onManualSwitch` + `ConfigManager::setDisplayMode` and its inner `#ifdef LE_V0_SPIKE` |
| 5 | `firmware/core/ModeOrchestrator.cpp` | `#ifdef LE_V0_METRICS` early-return guard in `onIdleFallback()` |
| 6 | `firmware/modes/v0_spike_layout.h` | **Delete file entirely** (`git rm`) — no references remain in `CustomLayoutMode.cpp` after this (the spike header is only `#include`d from there; check and remove that include too if it exists) |
| 7 | `AGENTS.md` | **ADD** documentation (do not remove): document that `[env:esp32dev_le_baseline]` and `[env:esp32dev_le_spike]` in `platformio.ini` are dev-only performance measurement environments using `-DLE_V0_METRICS` / `-DLE_V0_SPIKE` build flags for future spike-vs-baseline comparisons |

**Do NOT remove:**
- `#ifdef LE_V0_SPIKE` around the `custom_layout` mode-table entry, factory functions, and `#include "modes/CustomLayoutMode.h"` — these stay until LE-1.3
- `[env:esp32dev_le_baseline]` and `[env:esp32dev_le_spike]` entries in `platformio.ini`

### NVS Key Budget

ConfigManager's NVS key comment block (top of `ConfigManager.h`) documents all keys and their char counts. The 15-char NVS limit is enforced by ESP32 `Preferences`. Add `layout_active` (12 chars) to that comment block.

Existing pattern for reference:
```cpp
// Display mode NVS persistence
// NVS key: "disp_mode" (9 chars, within 15-char limit)
static String getDisplayMode();
static void setDisplayMode(const char* modeId);
```

New pattern to add:
```cpp
// Active layout NVS persistence (Story le-1.1, AC #5)
// NVS key: "layout_active" (12 chars, within 15-char limit)
// Empty string = no layout persisted; LayoutStore::load() falls back to kDefaultLayoutJson.
static String getLayoutActiveId();
static bool setLayoutActiveId(const char* id);
```

Both accessors open and close `Preferences("flightwall", ...)` in a single call (same pattern as `getDisplayMode()` / `setDisplayMode()` in `ConfigManager.cpp`).

### Layout JSON Schema

From `epic-le-1-layout-editor.md` Architecture section — V1 schema:

```json
{
  "id": "my-home-wall",
  "name": "Home Wall",
  "v": 1,
  "canvas": { "w": 160, "h": 32 },
  "bg": "#000000",
  "widgets": [
    { "id": "w1", "type": "text",  "x": 0,   "y": 0,  "w": 96, "h": 10, "content": "WELCOME HOME", "font": "M", "align": "left",   "color": "#FFAA00" },
    { "id": "w2", "type": "clock", "x": 100, "y": 0,  "w": 60, "h": 10, "format": "24h",            "font": "M",                    "color": "#FFFFFF" },
    { "id": "w3", "type": "logo",  "x": 0,   "y": 12, "w": 32, "h": 20, "logo_id": "united_1" }
  ]
}
```

**Fields common to every widget:** `id`, `type`, `x`, `y`, `w`, `h`
**Top-level required fields for schema validation in `save()`:** `id`, `name`, `v`, `canvas`, `widgets`
**`v` must equal `1`** — future versions require a migrator; unknown versions are rejected
**Widget type allowlist** (must match `WidgetRegistry` in LE-1.2): `text`, `clock`, `logo`, `flight_field`, `metric`

Note: canvas dimensions are **not** hardcoded to 192×48 — they reflect the actual device tile configuration. Do NOT hard-reject based on specific canvas size; only reject canvas `w < 1` or `h < 1`.

### ArduinoJson v7 Usage

The project uses ArduinoJson v7 (`JsonDocument`, not `DynamicJsonDocument`). Pattern:

```cpp
JsonDocument doc;
DeserializationError err = deserializeJson(doc, json);
if (err) {
    return LayoutStoreError::INVALID_SCHEMA;
}
// Access fields:
const char* id = doc["id"] | "";
int version = doc["v"] | -1;
JsonArray widgets = doc["widgets"].as<JsonArray>();
// doc destructs automatically at end of scope — DO NOT store doc as a member
```

Never retain a `JsonDocument` past the function scope in `LayoutStore`. The rule from `epic-le-1-layout-editor.md`: *"Parse JSON at mode init only. render() must not see JSON."*

### LittleFS Budget

From `custom_partitions.csv`:
- LittleFS partition: `spiffs, data, spiffs, 0x310000, 0xF0000` → **960 KB total**
- Logos (`/logos/`): ~278 KB estimated in use (varies by logo count)
- New `/layouts/` directory cap: **64 KB** (16 files × ~4 KB average JSON, 8 KB max per file)
- Remaining headroom after logos + layouts: ~618 KB — within budget

The 64 KB cap is enforced by `LayoutStore::save()` before any write. There is no OS-level LittleFS quota; the cap is entirely in application logic (iterate directory, sum file sizes, reject if over limit).

### File and Path Conventions

- Directory: `/layouts/` (slash prefix, slash suffix) — created by `LayoutStore::init()`
- Individual layout path: `/layouts/<id>.json`
- `id` validation rules (matches LogoManager's filename safety pattern, adapted for JSON ids):
  - Non-null, non-empty
  - Length ≤ 32 characters
  - Characters restricted to `[a-zA-Z0-9_-]` only
  - No `..`, no `/`, no `\` substrings
- The `_default` id is reserved for the baked-in fallback (produced by `load()` when the active file is missing); it is never written to disk

### Test Infrastructure

Tests live in `firmware/test/test_layout_store/test_main.cpp`. This is an **on-device Unity test** (same as `test_logo_manager`):

```
Environment: esp32dev (on-device test) — requires LittleFS
Run with: pio test -e esp32dev --filter test_layout_store
```

Pattern from `test_logo_manager/test_main.cpp`:
- `setup()` mounts LittleFS with `LittleFS.begin(true)`, calls `UNITY_BEGIN()`, runs all `RUN_TEST()` calls, calls `UNITY_END()`
- `loop()` is empty
- Each test function follows `void test_<scenario>()` naming
- A `cleanLayoutsDir()` helper mirrors `cleanLogosDir()` — removes all files in `/layouts/` then removes the dir itself

### Project Structure Notes

```
firmware/
├── core/
│   ├── ConfigManager.h        ← ADD getLayoutActiveId() / setLayoutActiveId()
│   ├── ConfigManager.cpp      ← ADD implementation using Preferences("flightwall")
│   ├── LayoutStore.h          ← NEW
│   └── LayoutStore.cpp        ← NEW
├── modes/
│   ├── CustomLayoutMode.h     ← UNTOUCHED (LE-1.3 scope)
│   ├── CustomLayoutMode.cpp   ← UNTOUCHED (LE-1.3 scope; remove v0_spike_layout.h include if present)
│   └── v0_spike_layout.h      ← DELETE
├── src/
│   └── main.cpp               ← MODIFY: remove LE_V0_METRICS blocks (4 locations)
├── test/
│   └── test_layout_store/
│       └── test_main.cpp      ← NEW
AGENTS.md                      ← MODIFY: document le_baseline / le_spike envs
```

`firmware/core/ModeOrchestrator.cpp` is also modified (remove LE_V0_METRICS early return in `onIdleFallback()`).

### References

- `_bmad-output/planning-artifacts/epic-le-1-layout-editor.md` — Architecture, V0 Spike Cleanup list (items 1–7), Quality Gates, V1 schema, implementation pattern requirements
- `_bmad-output/planning-artifacts/epics/epic-le-1.md` — BMAD-parseable story shards; this story's user statement and AC one-liner
- `firmware/core/LogoManager.h` / `LogoManager.cpp` — Direct template for LayoutStore API shape and implementation style
- `firmware/core/ConfigManager.h` — NVS abbreviation comment block, `disp_mode` pattern to follow for `layout_active`
- `firmware/src/main.cpp` — Exact locations of all `LE_V0_METRICS` blocks to remove
- `firmware/test/test_logo_manager/test_main.cpp` — Test structure, `cleanDir` helper, Unity boilerplate
- `firmware/custom_partitions.csv` — LittleFS partition size (960 KB)
- `firmware/check_size.py` — Binary size gate (1.5 MB hard limit, 1.3 MB warning)

---

## Dev Agent Record

- **Agent:** Claude (Sonnet 4.5)
- **Completed:** 2026-04-17
- **Build result:** **pass** — `pio run -e esp32dev` produced `firmware.bin` at **1,277,760 bytes (1.22 MB)**, well under the 1.5 MB OTA gate enforced by `check_size.py` ("OK: Binary size within limits"). Auxiliary envs also build clean: `esp32dev_le_baseline` (1,277,824 bytes) and `esp32dev_le_spike` (1,281,904 bytes).
- **Test result:** **pass (compile)** — `pio test -e esp32dev --filter test_layout_store --without-testing --without-uploading` compiled the 16 Unity test cases in 19.28 s with zero errors. On-device execution (`UNITY_END()` pass/fail) deferred to reviewer with a physical ESP32; build-only verification is sufficient to prove the test translation unit links against the final `LayoutStore` / `ConfigManager` symbols.
- **Notes:**
  - AC #5 states the `layout_active` NVS key is 12 chars; actual count is **13 chars** (`l-a-y-o-u-t-_-a-c-t-i-v-e`). Still comfortably within the 15-char NVS limit. Comment block in `ConfigManager.h` and the new declarations use the accurate count.
  - `firmware/modes/v0_spike_layout.h` deletion left `CustomLayoutMode.cpp` with an unresolved include and symbol reference (`v0spike::kLayoutJson`). To keep `[env:esp32dev_le_spike]` building without expanding this story's scope into LE-1.3, the spike JSON was **inlined** into `CustomLayoutMode.cpp` as `kSpikeLayoutJson` in an anonymous namespace. Behavior unchanged; spike env still exercises the generic renderer.
  - Grep verification: `grep -r "LE_V0_METRICS\|le_v0_record\|v0_spike_layout" firmware/{src,core,modes}` returns zero results. Matches remaining in `firmware/platformio.ini` are intentional per AC #7 (dev-only measurement envs retained).
  - `CustomLayoutMode` ↔ `LayoutStore` wiring is out of scope for LE-1.1 per the Dev Notes — the `#ifdef LE_V0_SPIKE` guard around the `custom_layout` mode-table entry in `main.cpp` is left in place for LE-1.3.

---

## File List

**New files:**
- `firmware/core/LayoutStore.h`
- `firmware/core/LayoutStore.cpp`
- `firmware/test/test_layout_store/test_main.cpp`

**Modified files:**
- `firmware/core/ConfigManager.h` — added `layout_active` NVS key comment + `getLayoutActiveId()` / `setLayoutActiveId()` declarations
- `firmware/core/ConfigManager.cpp` — implemented `getLayoutActiveId()` / `setLayoutActiveId()` using `Preferences("flightwall")`
- `firmware/src/main.cpp` — removed 4 `#ifdef LE_V0_METRICS` blocks (ring buffer + helpers, `_leT0` tick wrapper, `statusMessageVisible` bypass, boot-override in `setup()`)
- `firmware/core/ModeOrchestrator.cpp` — removed `#ifdef LE_V0_METRICS` early-return guard in `onIdleFallback()`
- `firmware/modes/CustomLayoutMode.cpp` — removed `#include "modes/v0_spike_layout.h"`; inlined spike JSON as `kSpikeLayoutJson` to keep `LE_V0_SPIKE` env building
- `firmware/modes/CustomLayoutMode.h` — updated header comment to reference the inlined JSON
- `AGENTS.md` — added "Dev-only PlatformIO Environments (Layout Editor)" section documenting `esp32dev_le_baseline` / `esp32dev_le_spike`

**Deleted files:**
- `firmware/modes/v0_spike_layout.h`

---

## Change Log

| Date       | Version | Description                                                                                                                                                                     | Author             |
|------------|---------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------|
| 2026-04-17 | 1.0     | Initial implementation of `LayoutStore` (LittleFS CRUD + schema validation + 8 KB/64 KB/16-file caps), `layout_active` NVS accessors on `ConfigManager`, 16 Unity test cases, and full V0 spike instrumentation cleanup (AC #7). Spike JSON inlined into `CustomLayoutMode.cpp` to preserve `LE_V0_SPIKE` env. | Claude (Sonnet 4.5) |
| 2026-04-17 | 1.1     | Code review synthesis (3 reviewers): fixed unbounded `readString()` in `load()` and `list()`, enforced `_default` reserved id in `isSafeLayoutId()`, added doc-id vs path-id consistency check in `validateSchema()`, improved AC6 warning messages to include offending id, added 4 new Unity tests (id-mismatch, reserved-default, AC5 NVS round-trip, AC6 E2E). Build: 1,277,760 bytes (1.22 MB), check_size.py OK. Test TU compiles clean. | AI Code Review Synthesis |

---

## Senior Developer Review (AI)

### Review: 2026-04-17
- **Reviewer:** AI Code Review Synthesis
- **Evidence Score:** 6.7 (Reviewers A+B+C combined) → MAJOR REWORK
- **Issues Found:** 9 verified (4 High, 5 Medium), 2 deferred (Low)
- **Issues Fixed:** 9 (all High and Medium items resolved in synthesis)
- **Action Items Created:** 2 (Low severity, deferred to LE-1.3)
