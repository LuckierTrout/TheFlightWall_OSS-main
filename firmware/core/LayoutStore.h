#pragma once
/*
Purpose: Persistent CRUD store for user-defined layouts on LittleFS.

Story le-1.1 (layout-store-littlefs-crud) — layer owned exclusively by this
story. Integration with CustomLayoutMode is deferred to LE-1.3.

Responsibilities:
  - Maintain `/layouts/<id>.json` files under LittleFS with strict size and
    count caps (per-file + directory-wide).
  - Validate the V1 layout JSON schema on every save() (required top-level
    fields, version, canvas bounds, widget type allowlist).
  - Track the currently active layout id in NVS (`layout_active` key,
    delegated to ConfigManager) with a baked-in default fallback.
  - Provide a pure, hardware-free API surface (only LittleFS + NVS IO); no
    heavy state — all methods are static.
*/

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>

// ------------------------------------------------------------------
// Compile-time limits (enforced by LayoutStore::save() before any IO).
// ------------------------------------------------------------------

// Per-file cap (AC #3a). Strings exceeding this are rejected before JSON
// parsing. 8 KiB comfortably fits the documented V1 schema examples while
// leaving headroom in the 960 KB LittleFS partition.
static constexpr size_t LAYOUT_STORE_MAX_BYTES = 8192;

// Directory-wide cap (AC #4). A save() that would push the /layouts/ tree
// past this total (net of any overwrite) returns FS_FULL. 64 KiB matches the
// budget documented in the epic Architecture section.
static constexpr size_t LAYOUT_STORE_TOTAL_BYTES = 65536;

// Maximum number of layouts retained on-device. New-id save() attempts beyond
// this return FS_FULL regardless of byte usage.
static constexpr uint8_t LAYOUT_STORE_MAX_FILES = 16;

// Maximum length of a validated layout id (bytes, excluding null).
static constexpr size_t LAYOUT_STORE_MAX_ID_LEN = 32;

// ------------------------------------------------------------------
// Public types
// ------------------------------------------------------------------

enum class LayoutStoreError : uint8_t {
    OK = 0,
    NOT_FOUND,
    TOO_LARGE,
    INVALID_SCHEMA,
    FS_FULL,
    IO_ERROR,
};

struct LayoutEntry {
    String id;          // filename stem (e.g. "home-wall" from "home-wall.json")
    String name;        // value of top-level "name" field inside the JSON
    size_t sizeBytes;   // raw file size on disk
};

class LayoutStore {
public:
    // Ensure /layouts/ exists as a directory. Returns false if it exists as a
    // non-directory entry or if creation fails.
    static bool init();

    // Persist `json` as /layouts/<id>.json. Validates id + payload; overwrites
    // in place if the id already exists. See LayoutStoreError for failure
    // codes and the story AC for the exact rejection contract.
    static LayoutStoreError save(const char* id, const char* json);

    // Load /layouts/<id>.json into `out`. On success returns true and `out`
    // contains the raw JSON text. On miss, fills `out` with the baked-in
    // default layout, emits a warning log, and returns false (AC #6).
    static bool load(const char* id, String& out);

    // Remove /layouts/<id>.json. Returns true if the delete succeeded.
    static bool remove(const char* id);

    // Enumerate every valid /layouts/*.json entry into `result`. Returns
    // false only if the directory cannot be opened.
    static bool list(std::vector<LayoutEntry>& result);

    // Validate a candidate layout id against the on-disk path-safety rules
    // (non-null, non-empty, length ≤ LAYOUT_STORE_MAX_ID_LEN, only
    // `[A-Za-z0-9_-]`, no "..", "/", or "\\" substrings).
    static bool isSafeLayoutId(const char* id);

    // NVS-backed active-id accessors (AC #5). Delegate to ConfigManager to
    // keep all Preferences traffic in one place.
    static String getActiveId();
    static bool setActiveId(const char* id);
};
