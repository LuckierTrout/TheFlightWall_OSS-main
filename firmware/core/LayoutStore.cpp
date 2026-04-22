/*
Purpose: Implements LayoutStore — durable, size-capped, schema-validated CRUD
for layout JSON documents on LittleFS.

Design notes (Story le-1.1):
- Mirrors LogoManager's static-class shape.
- Schema validation is done with a scoped ArduinoJson v7 JsonDocument that is
  destroyed before save() returns (no retained docs; "parse once at mode init"
  rule from the epic is honored by keeping parsing transient here too).
- FS_FULL is enforced by iterating /layouts/ and summing sizes before the
  write, excluding the target id when overwriting. This keeps the cap honest
  even when an existing file is being replaced with a larger one.
- getActiveId()/setActiveId() delegate to ConfigManager so that Preferences
  (NVS) traffic stays in one place and the "flightwall" namespace is owned by
  a single module.
- When load(id) misses, we still fill `out` with kDefaultLayoutJson so callers
  always see a valid layout JSON string, even before any file has been saved.
*/

#include "core/LayoutStore.h"

#include "core/ConfigManager.h"
#include "utils/Log.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ctype.h>
#include <string.h>

namespace {

// Directory that holds per-layout JSON files. Kept without trailing slash to
// match LittleFS listing conventions (see LogoManager).
static const char LAYOUTS_DIR[] = "/layouts";

// Widget type allowlist (AC #3f). Must stay in sync with the WidgetRegistry
// introduced in LE-1.2. Any new widget type is a schema change and a version
// bump on "v".
static const char* const kAllowedWidgetTypes[] = {
    "text", "clock", "logo", "flight_field", "metric",
};

// Baked-in fallback layout returned by load() when the requested id is
// missing (AC #6). This is NEVER written to disk; the "_default" id is
// reserved and rejected by save() indirectly through isSafeLayoutId (it
// starts with "_" which is allowed, but tests should not rely on being able
// to save id="_default" — the epic architecture reserves it).
// Baked-in default layout. Designed for an 80x48 "expanded" panel — the
// common FlightWall form factor (5x3 tile grid of 16x16 panels). Falls back
// to this when the user hasn't saved any custom layout yet. Six rows of
// information stacked top-to-bottom: callsign (default 6x8), airline
// (TomThumb 4x6 so "United Airlines" fits), origin/destination codes,
// altitude + speed, aircraft type + clock.
static const char kDefaultLayoutJson[] = R"JSON({
  "id":"_default","name":"Default","v":1,
  "canvas":{"w":80,"h":48},
  "bg":"#000000",
  "widgets":[
    {"id":"cs","type":"flight_field","x":1,"y":1,"w":78,"h":8,"content":"callsign","align":"center","font":"default","text_size":1,"color":"#FFFFFF"},
    {"id":"al","type":"flight_field","x":1,"y":10,"w":78,"h":6,"content":"airline","align":"center","font":"tomthumb","text_size":1,"color":"#6699FF"},
    {"id":"org","type":"flight_field","x":1,"y":17,"w":36,"h":8,"content":"origin_iata","align":"left","font":"default","text_size":1,"color":"#00FFAA"},
    {"id":"dst","type":"flight_field","x":43,"y":17,"w":36,"h":8,"content":"destination_iata","align":"right","font":"default","text_size":1,"color":"#00FFAA"},
    {"id":"alt","type":"metric","x":1,"y":27,"w":39,"h":8,"content":"altitude_ft","align":"left","font":"default","text_size":1,"color":"#FFCC00"},
    {"id":"spd","type":"metric","x":40,"y":27,"w":39,"h":8,"content":"speed_kts","align":"right","font":"default","text_size":1,"color":"#FFCC00"},
    {"id":"ac","type":"flight_field","x":1,"y":37,"w":39,"h":6,"content":"aircraft_short","align":"left","font":"tomthumb","text_size":1,"color":"#AAAAAA"},
    {"id":"clk","type":"clock","x":40,"y":37,"w":39,"h":6,"content":"24h","align":"right","font":"tomthumb","text_size":1,"color":"#888888"}
  ]
})JSON";

// Extract the basename (after the final '/') without allocating a copy for
// the full path. LittleFS returns entry names with or without a leading slash
// depending on platform version.
String layoutBasename(const String& path) {
    int lastSlash = path.lastIndexOf('/');
    return lastSlash >= 0 ? path.substring(lastSlash + 1) : path;
}

// Return true iff `name` ends with ".json".
bool hasJsonExtension(const String& name) {
    return name.length() > 5 && name.endsWith(".json");
}

// Strip the ".json" suffix from a filename. Caller must ensure the suffix is
// present.
String stripJsonExtension(const String& name) {
    return name.substring(0, name.length() - 5);
}

// Build the canonical on-disk path for a given id.
String pathFor(const char* id) {
    String p;
    p.reserve(strlen(LAYOUTS_DIR) + 1 + strlen(id) + 5);
    p = LAYOUTS_DIR;
    p += '/';
    p += id;
    p += ".json";
    return p;
}

bool isAllowedWidgetType(const char* t) {
    if (t == nullptr) return false;
    for (size_t i = 0; i < sizeof(kAllowedWidgetTypes) / sizeof(kAllowedWidgetTypes[0]); ++i) {
        if (strcmp(t, kAllowedWidgetTypes[i]) == 0) return true;
    }
    return false;
}

// Required top-level fields per AC #3c.
bool hasRequiredTopLevelFields(JsonDocument& doc) {
    return !doc["id"].isNull()
        && !doc["name"].isNull()
        && !doc["v"].isNull()
        && !doc["canvas"].isNull()
        && !doc["widgets"].isNull();
}

// AC #3 full schema validation. Returns OK or a specific error code.
// `pathId` is the id used to construct the on-disk path (i.e. the first
// argument to save()). We require doc["id"] to match so that the filesystem
// truth (filename) and the JSON self-description are always consistent.
LayoutStoreError validateSchema(const char* json, const char* pathId) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    if (!hasRequiredTopLevelFields(doc)) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    // Require doc["id"] to equal the caller-supplied path id so the filename
    // and JSON self-description are always in sync (prevents silent desync
    // where save("foo", {"id":"bar",...}) stores a file whose name lies about
    // its own content).
    const char* docId = doc["id"] | (const char*)nullptr;
    if (docId == nullptr || strcmp(docId, pathId) != 0) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    // AC #3d
    if (doc["v"].as<int>() != 1) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    // AC #3e
    JsonVariantConst canvas = doc["canvas"];
    if (!canvas.is<JsonObjectConst>()) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    int cw = canvas["w"] | 0;
    int ch = canvas["h"] | 0;
    if (cw < 1 || ch < 1) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    // AC #3f
    JsonVariantConst widgets = doc["widgets"];
    if (!widgets.is<JsonArrayConst>()) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    for (JsonVariantConst wv : widgets.as<JsonArrayConst>()) {
        const char* t = wv["type"] | (const char*)nullptr;
        if (!isAllowedWidgetType(t)) {
            return LayoutStoreError::INVALID_SCHEMA;
        }
    }
    return LayoutStoreError::OK;
    // doc destructs here — no retained JSON state (per epic parse-once rule).
}

// Walk /layouts/ and accumulate file stats. Any file whose basename stem
// matches `excludeId` (i.e. the id being overwritten by save) is skipped
// from the totals so the overwrite path doesn't double-count.
struct DirStats {
    size_t totalBytes;
    uint8_t fileCount;
};

bool collectDirStats(const char* excludeId, DirStats& out) {
    out.totalBytes = 0;
    out.fileCount = 0;
    File dir = LittleFS.open(LAYOUTS_DIR);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return false;
    }
    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String name = layoutBasename(entry.name());
            if (hasJsonExtension(name)) {
                String stem = stripJsonExtension(name);
                bool excluded = (excludeId != nullptr && stem == excludeId);
                if (!excluded) {
                    out.totalBytes += entry.size();
                    out.fileCount++;
                }
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
    return true;
}

} // namespace

// ------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------

bool LayoutStore::init() {
    if (!LittleFS.exists(LAYOUTS_DIR)) {
        if (!LittleFS.mkdir(LAYOUTS_DIR)) {
            LOG_E("LayoutStore", "Failed to create /layouts/ directory");
            return false;
        }
        LOG_I("LayoutStore", "Created /layouts/ directory");
        return true;
    }
    File dir = LittleFS.open(LAYOUTS_DIR);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        LOG_E("LayoutStore", "/layouts exists but is not a directory");
        return false;
    }
    dir.close();
    return true;
}

bool LayoutStore::isSafeLayoutId(const char* id) {
    if (id == nullptr) return false;
    size_t len = strlen(id);
    if (len == 0 || len > LAYOUT_STORE_MAX_ID_LEN) return false;
    if (strstr(id, "..") != nullptr) return false;
    // "_default" is a reserved id (AC #6 / epic architecture). It is never
    // written to disk; rejecting it here prevents a saved file from silently
    // shadowing the baked-in fallback and breaking future LE-1.3 wiring.
    if (strcmp(id, "_default") == 0) return false;
    for (size_t i = 0; i < len; ++i) {
        char c = id[i];
        // Allowed: [A-Za-z0-9_-]. Explicitly rejects '/', '\\', '.' and any
        // control/unicode byte.
        bool ok = (c >= 'a' && c <= 'z')
               || (c >= 'A' && c <= 'Z')
               || (c >= '0' && c <= '9')
               || c == '_' || c == '-';
        if (!ok) return false;
    }
    return true;
}

LayoutStoreError LayoutStore::save(const char* id, const char* json) {
    // AC #3: id safety check first — path-traversal ids never reach the FS.
    if (!isSafeLayoutId(id)) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    if (json == nullptr) {
        return LayoutStoreError::INVALID_SCHEMA;
    }
    size_t jsonLen = strlen(json);
    // AC #3a: per-file cap.
    if (jsonLen > LAYOUT_STORE_MAX_BYTES) {
        return LayoutStoreError::TOO_LARGE;
    }
    // AC #3b–f: schema validation (also verifies doc["id"] == id).
    LayoutStoreError schemaResult = validateSchema(json, id);
    if (schemaResult != LayoutStoreError::OK) {
        return schemaResult;
    }
    // AC #4: directory-wide byte + file-count caps. Ensure /layouts/ exists
    // (fresh devices reach save() before init() only via tests that explicitly
    // skip init()).
    if (!LittleFS.exists(LAYOUTS_DIR)) {
        if (!LittleFS.mkdir(LAYOUTS_DIR)) {
            return LayoutStoreError::IO_ERROR;
        }
    }
    DirStats stats;
    if (!collectDirStats(id, stats)) {
        return LayoutStoreError::IO_ERROR;
    }
    if (stats.fileCount >= LAYOUT_STORE_MAX_FILES) {
        return LayoutStoreError::FS_FULL;
    }
    if (stats.totalBytes + jsonLen > LAYOUT_STORE_TOTAL_BYTES) {
        return LayoutStoreError::FS_FULL;
    }
    // Write atomically-ish: open for write truncates, then stream bytes.
    String path = pathFor(id);
    File f = LittleFS.open(path, "w");
    if (!f) {
        return LayoutStoreError::IO_ERROR;
    }
    size_t written = f.write(reinterpret_cast<const uint8_t*>(json), jsonLen);
    f.close();
    if (written != jsonLen) {
        // Partial write — clean up so the dir cap math stays honest.
        LittleFS.remove(path);
        return LayoutStoreError::IO_ERROR;
    }
    return LayoutStoreError::OK;
}

bool LayoutStore::load(const char* id, String& out) {
    if (!isSafeLayoutId(id)) {
        // AC #6: include the offending id in the warning for diagnosability.
        Serial.printf("[LayoutStore] WARNING: layout_active '%s' id invalid — using default\n", id);
        out = kDefaultLayoutJson;
        return false;
    }
    String path = pathFor(id);
    if (!LittleFS.exists(path)) {
        // AC #6: log the missing id so operators can diagnose stale NVS values.
        Serial.printf("[LayoutStore] WARNING: layout_active '%s' missing — using default\n", id);
        out = kDefaultLayoutJson;
        return false;
    }
    File f = LittleFS.open(path, "r");
    if (!f) {
        Serial.printf("[LayoutStore] WARNING: layout_active '%s' open failed — using default\n", id);
        out = kDefaultLayoutJson;
        return false;
    }
    // Guard against files that exceed the per-file cap (e.g. manual upload
    // or LittleFS corruption). readString() on the ESP32 allocates the full
    // file into a heap String; an oversized file would OOM the device.
    if (f.size() > LAYOUT_STORE_MAX_BYTES) {
        Serial.printf("[LayoutStore] WARNING: layout_active '%s' file too large (%u B) — using default\n",
                      id, (unsigned)f.size());
        f.close();
        out = kDefaultLayoutJson;
        return false;
    }
    out = f.readString();
    f.close();
    return true;
}

bool LayoutStore::remove(const char* id) {
    if (!isSafeLayoutId(id)) {
        return false;
    }
    String path = pathFor(id);
    if (!LittleFS.exists(path)) {
        return false;
    }
    return LittleFS.remove(path);
}

bool LayoutStore::list(std::vector<LayoutEntry>& result) {
    result.clear();
    File dir = LittleFS.open(LAYOUTS_DIR);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return false;
    }
    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String name = layoutBasename(entry.name());
            if (hasJsonExtension(name)) {
                LayoutEntry e;
                e.id = stripJsonExtension(name);
                e.sizeBytes = entry.size();

                // Pull "name" out of the JSON without retaining the doc. If
                // the file is malformed, leave e.name empty — list() still
                // advertises the file exists so the UI can offer a delete.
                // Guard against oversized files (corruption/manual upload)
                // before readString() to avoid heap exhaustion.
                if (entry.size() <= LAYOUT_STORE_MAX_BYTES) {
                    String body = entry.readString();
                    JsonDocument doc;
                    DeserializationError err = deserializeJson(doc, body);
                    if (!err) {
                        const char* nm = doc["name"] | "";
                        e.name = nm;
                    }
                }
                result.push_back(e);
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
    return true;
}

String LayoutStore::getActiveId() {
    return ConfigManager::getLayoutActiveId();
}

bool LayoutStore::setActiveId(const char* id) {
    if (!isSafeLayoutId(id)) return false;
    return ConfigManager::setLayoutActiveId(id);
}
