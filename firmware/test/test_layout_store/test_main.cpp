/*
Purpose: Unity tests for LayoutStore (Story le-1.1 — layout-store-littlefs-crud).

Covered acceptance criteria:
  AC #1 — save()/overwrite happy path
  AC #2 — list() / load() round-trip
  AC #3 — schema validation (size cap, malformed JSON, missing fields, version,
          canvas bounds, widget allowlist, unsafe ids, id mismatch, _default reserved)
  AC #4 — 16-file and 64 KB directory caps
  AC #5 — setActiveId / getActiveId NVS round-trip
  AC #6 — load() falls back to kDefaultLayoutJson on miss (including active-id path)

Environment: esp32dev (on-device test) — requires LittleFS.
Run with: pio test -e esp32dev --filter test_layout_store

Test list (see RUN_TEST calls in setup()):
  test_init_creates_layouts_dir
  test_init_existing_dir
  test_save_and_load_roundtrip
  test_save_overwrites_existing
  test_save_rejects_oversized
  test_save_rejects_missing_fields
  test_save_rejects_wrong_version
  test_save_rejects_unknown_widget_type
  test_save_rejects_malformed_json
  test_save_rejects_bad_canvas
  test_save_rejects_id_mismatch
  test_save_rejects_reserved_default_id
  test_cap_16_files
  test_cap_64kb_bytes
  test_remove_erases_file
  test_load_missing_returns_default
  test_list_returns_all_layouts
  test_unsafe_id_rejected
  test_active_id_roundtrip
  test_load_with_missing_active_id_uses_default
*/
#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include "core/LayoutStore.h"
#include "core/ConfigManager.h"

// --- Test helpers ---------------------------------------------------------

// Remove every entry under /layouts/ and the directory itself so each test
// starts from a known-empty state.
static void cleanLayoutsDir() {
    if (LittleFS.exists("/layouts")) {
        File dir = LittleFS.open("/layouts");
        if (dir && dir.isDirectory()) {
            File entry = dir.openNextFile();
            while (entry) {
                String path = entry.name();
                if (!path.startsWith("/")) {
                    path = String("/layouts/") + path;
                }
                entry.close();
                LittleFS.remove(path);
                entry = dir.openNextFile();
            }
        }
        LittleFS.rmdir("/layouts");
    }
}

// Compose a valid V1 layout JSON with a caller-supplied id/name and a single
// clock widget. Used throughout the happy-path tests.
static String makeValidLayout(const char* id, const char* name) {
    String s;
    s.reserve(256);
    s += "{\"id\":\"";
    s += id;
    s += "\",\"name\":\"";
    s += name;
    s += "\",\"v\":1,\"canvas\":{\"w\":160,\"h\":32},\"bg\":\"#000000\",";
    s += "\"widgets\":[";
    s += "{\"id\":\"w1\",\"type\":\"clock\",\"x\":0,\"y\":0,\"w\":80,\"h\":16,\"format\":\"24h\",\"color\":\"#FFFFFF\"}";
    s += "]}";
    return s;
}

// Build a "padded" valid layout JSON whose serialized size is close to
// `targetBytes`. Achieved by repeating a benign filler field in the canvas
// object (harmless to validation — extra top-level fields are ignored).
// The produced JSON is guaranteed to stay ≤ LAYOUT_STORE_MAX_BYTES.
static String makeLayoutAroundSize(const char* id, size_t targetBytes) {
    String base = makeValidLayout(id, "PAD");
    if (base.length() >= targetBytes) return base;
    // Inject a harmless "pad":"..." field into the top-level object. Replace
    // the final "}" with `, "pad":"<filler>"}`.
    String filler;
    size_t need = targetBytes - base.length() - 12; // 12 chars of overhead for `,"pad":""`
    if (need > LAYOUT_STORE_MAX_BYTES) need = LAYOUT_STORE_MAX_BYTES;
    filler.reserve(need);
    for (size_t i = 0; i < need; ++i) filler += 'x';
    // Close before the final '}'.
    String out = base.substring(0, base.length() - 1);
    out += ",\"pad\":\"";
    out += filler;
    out += "\"}";
    return out;
}

// --- AC #7/#8 sanity: basic init lifecycle --------------------------------

void test_init_creates_layouts_dir() {
    cleanLayoutsDir();
    TEST_ASSERT_FALSE(LittleFS.exists("/layouts"));

    TEST_ASSERT_TRUE(LayoutStore::init());
    TEST_ASSERT_TRUE(LittleFS.exists("/layouts"));
}

void test_init_existing_dir() {
    // /layouts/ was created by the previous test
    TEST_ASSERT_TRUE(LayoutStore::init());
}

// --- AC #1 / AC #2: save + load + overwrite -------------------------------

void test_save_and_load_roundtrip() {
    cleanLayoutsDir();
    LayoutStore::init();

    String payload = makeValidLayout("home", "Home Wall");
    LayoutStoreError err = LayoutStore::save("home", payload.c_str());
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK, (int)err);
    TEST_ASSERT_TRUE(LittleFS.exists("/layouts/home.json"));

    String out;
    TEST_ASSERT_TRUE(LayoutStore::load("home", out));
    TEST_ASSERT_TRUE(out.indexOf("\"id\":\"home\"") >= 0);
    TEST_ASSERT_TRUE(out.indexOf("Home Wall") >= 0);
}

void test_save_overwrites_existing() {
    cleanLayoutsDir();
    LayoutStore::init();

    String first = makeValidLayout("abc", "First");
    String second = makeValidLayout("abc", "Second");

    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("abc", first.c_str()));
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("abc", second.c_str()));

    String out;
    TEST_ASSERT_TRUE(LayoutStore::load("abc", out));
    TEST_ASSERT_TRUE(out.indexOf("Second") >= 0);
    TEST_ASSERT_TRUE(out.indexOf("First") < 0);
}

// --- AC #3: validation rejections ----------------------------------------

void test_save_rejects_oversized() {
    cleanLayoutsDir();
    LayoutStore::init();

    // Build a raw buffer strictly larger than the per-file cap. We don't
    // need to emit valid JSON — the size check must fire before parsing.
    String huge;
    huge.reserve(LAYOUT_STORE_MAX_BYTES + 64);
    for (size_t i = 0; i < LAYOUT_STORE_MAX_BYTES + 64; ++i) huge += 'x';

    TEST_ASSERT_EQUAL((int)LayoutStoreError::TOO_LARGE,
                      (int)LayoutStore::save("big", huge.c_str()));
    TEST_ASSERT_FALSE(LittleFS.exists("/layouts/big.json"));
}

void test_save_rejects_missing_fields() {
    cleanLayoutsDir();
    LayoutStore::init();

    // Missing "v" field
    const char* missing = "{\"id\":\"x\",\"name\":\"X\",\"canvas\":{\"w\":16,\"h\":16},\"widgets\":[]}";
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("x", missing));
    TEST_ASSERT_FALSE(LittleFS.exists("/layouts/x.json"));
}

void test_save_rejects_wrong_version() {
    cleanLayoutsDir();
    LayoutStore::init();

    const char* v2 =
        "{\"id\":\"x\",\"name\":\"X\",\"v\":2,\"canvas\":{\"w\":16,\"h\":16},\"widgets\":[]}";
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("x", v2));
}

void test_save_rejects_unknown_widget_type() {
    cleanLayoutsDir();
    LayoutStore::init();

    const char* bad =
        "{\"id\":\"x\",\"name\":\"X\",\"v\":1,\"canvas\":{\"w\":16,\"h\":16},"
        "\"widgets\":[{\"id\":\"w1\",\"type\":\"unknown\",\"x\":0,\"y\":0,\"w\":8,\"h\":8}]}";
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("x", bad));
}

void test_save_rejects_malformed_json() {
    cleanLayoutsDir();
    LayoutStore::init();

    const char* junk = "{\"id\":\"x\",\"name\":"; // truncated
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("x", junk));
}

void test_save_rejects_bad_canvas() {
    cleanLayoutsDir();
    LayoutStore::init();

    const char* zeroW =
        "{\"id\":\"x\",\"name\":\"X\",\"v\":1,\"canvas\":{\"w\":0,\"h\":16},\"widgets\":[]}";
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("x", zeroW));

    const char* zeroH =
        "{\"id\":\"x\",\"name\":\"X\",\"v\":1,\"canvas\":{\"w\":16,\"h\":0},\"widgets\":[]}";
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("x", zeroH));
}

void test_save_rejects_id_mismatch() {
    cleanLayoutsDir();
    LayoutStore::init();

    // doc["id"] is "other" but path id is "myid" — must be rejected so that
    // the filename and JSON self-description always agree.
    const char* mismatch =
        "{\"id\":\"other\",\"name\":\"X\",\"v\":1,\"canvas\":{\"w\":16,\"h\":16},\"widgets\":[]}";
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("myid", mismatch));
    TEST_ASSERT_FALSE(LittleFS.exists("/layouts/myid.json"));
    TEST_ASSERT_FALSE(LittleFS.exists("/layouts/other.json"));
}

void test_save_rejects_reserved_default_id() {
    cleanLayoutsDir();
    LayoutStore::init();

    // "_default" is reserved for the baked-in fallback. Saving it would
    // shadow kDefaultLayoutJson and break future LE-1.3 wiring.
    const char* defJson =
        "{\"id\":\"_default\",\"name\":\"D\",\"v\":1,\"canvas\":{\"w\":16,\"h\":16},\"widgets\":[]}";
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("_default", defJson));
    TEST_ASSERT_FALSE(LittleFS.exists("/layouts/_default.json"));
}

// --- AC #4: directory caps ------------------------------------------------

void test_cap_16_files() {
    cleanLayoutsDir();
    LayoutStore::init();

    // Save exactly 16 layouts (ids L00..L15). Each fits well under the
    // per-file cap so we don't trigger the byte budget.
    for (uint8_t i = 0; i < LAYOUT_STORE_MAX_FILES; ++i) {
        char id[8];
        snprintf(id, sizeof(id), "L%02u", (unsigned)i);
        String payload = makeValidLayout(id, "x");
        TEST_ASSERT_EQUAL_MESSAGE((int)LayoutStoreError::OK,
                                  (int)LayoutStore::save(id, payload.c_str()),
                                  "expected first 16 saves to succeed");
    }

    // A 17th new-id save must be rejected with FS_FULL and must NOT create
    // the file on disk.
    String extra = makeValidLayout("L99", "x");
    TEST_ASSERT_EQUAL((int)LayoutStoreError::FS_FULL,
                      (int)LayoutStore::save("L99", extra.c_str()));
    TEST_ASSERT_FALSE(LittleFS.exists("/layouts/L99.json"));

    // Overwriting an existing id must still succeed even at the cap.
    String overwrite = makeValidLayout("L00", "updated");
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("L00", overwrite.c_str()));
}

void test_cap_64kb_bytes() {
    cleanLayoutsDir();
    LayoutStore::init();

    // Fill /layouts/ with large-but-valid files until we're close to the
    // 64 KB directory cap, then verify the next new-id save is rejected.
    // Target per file ≈ 7000 bytes → ~10 files reach ~70 KB total.
    const size_t bytesPerFile = 7000;
    uint8_t written = 0;
    for (uint8_t i = 0; i < LAYOUT_STORE_MAX_FILES; ++i) {
        char id[8];
        snprintf(id, sizeof(id), "B%02u", (unsigned)i);
        String payload = makeLayoutAroundSize(id, bytesPerFile);
        LayoutStoreError err = LayoutStore::save(id, payload.c_str());
        if (err == LayoutStoreError::FS_FULL) break;
        TEST_ASSERT_EQUAL((int)LayoutStoreError::OK, (int)err);
        written++;
    }
    // We must have been stopped by the byte budget before the file cap.
    TEST_ASSERT_LESS_THAN(LAYOUT_STORE_MAX_FILES, written);

    // Confirm the next new-id save explicitly reports FS_FULL.
    String extra = makeLayoutAroundSize("BNEW", bytesPerFile);
    TEST_ASSERT_EQUAL((int)LayoutStoreError::FS_FULL,
                      (int)LayoutStore::save("BNEW", extra.c_str()));
    TEST_ASSERT_FALSE(LittleFS.exists("/layouts/BNEW.json"));
}

// --- AC #1 tail: remove() -------------------------------------------------

void test_remove_erases_file() {
    cleanLayoutsDir();
    LayoutStore::init();

    String payload = makeValidLayout("gone", "Gone");
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("gone", payload.c_str()));
    TEST_ASSERT_TRUE(LittleFS.exists("/layouts/gone.json"));

    TEST_ASSERT_TRUE(LayoutStore::remove("gone"));
    TEST_ASSERT_FALSE(LittleFS.exists("/layouts/gone.json"));
}

// --- AC #6: missing-layout fallback --------------------------------------

void test_load_missing_returns_default() {
    cleanLayoutsDir();
    LayoutStore::init();

    String out;
    out = "pre-existing garbage";
    TEST_ASSERT_FALSE(LayoutStore::load("does-not-exist", out));
    TEST_ASSERT_TRUE(out.indexOf("_default") >= 0);
    TEST_ASSERT_TRUE(out.indexOf("FLIGHTWALL") >= 0);
}

// --- AC #2: list() --------------------------------------------------------

void test_list_returns_all_layouts() {
    cleanLayoutsDir();
    LayoutStore::init();

    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("a", makeValidLayout("a", "A").c_str()));
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("b", makeValidLayout("b", "B").c_str()));
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("c", makeValidLayout("c", "C").c_str()));

    std::vector<LayoutEntry> result;
    TEST_ASSERT_TRUE(LayoutStore::list(result));
    TEST_ASSERT_EQUAL(3, result.size());

    // listing order isn't specified; just assert the three ids appear.
    bool sawA = false, sawB = false, sawC = false;
    for (const auto& e : result) {
        if (e.id == "a") sawA = true;
        if (e.id == "b") sawB = true;
        if (e.id == "c") sawC = true;
        TEST_ASSERT_GREATER_THAN(0u, e.sizeBytes);
    }
    TEST_ASSERT_TRUE(sawA && sawB && sawC);
}

// --- AC #3: unsafe ids ----------------------------------------------------

void test_unsafe_id_rejected() {
    cleanLayoutsDir();
    LayoutStore::init();

    String payload = makeValidLayout("ok", "ok");
    // Path traversal / separators / empty id / overlong id must all reject
    // before any FS call.
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("../etc", payload.c_str()));
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("nested/id", payload.c_str()));
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("back\\id", payload.c_str()));
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("", payload.c_str()));
    // 33 chars → 1 over the cap
    TEST_ASSERT_EQUAL((int)LayoutStoreError::INVALID_SCHEMA,
                      (int)LayoutStore::save("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                                             payload.c_str()));

    // Confirm nothing leaked onto disk.
    std::vector<LayoutEntry> result;
    LayoutStore::list(result);
    TEST_ASSERT_EQUAL(0, result.size());
}

// --- AC #5: NVS active-id round-trip --------------------------------------

void test_active_id_roundtrip() {
    // Clear any prior NVS value for layout_active by writing a known string,
    // then overwrite and verify the new value survives a re-read.
    // (Full reboot persistence is verified on-device only; this confirms the
    // Preferences read/write path within a single session.)
    cleanLayoutsDir();
    LayoutStore::init();

    // Save a layout so we have a valid id to use.
    String payload = makeValidLayout("nvs-test", "NVS Test");
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("nvs-test", payload.c_str()));

    TEST_ASSERT_TRUE(LayoutStore::setActiveId("nvs-test"));
    String got = LayoutStore::getActiveId();
    TEST_ASSERT_EQUAL_STRING("nvs-test", got.c_str());

    // Write a second id and confirm the value updates.
    String payload2 = makeValidLayout("nvs-test2", "NVS Test 2");
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("nvs-test2", payload2.c_str()));

    TEST_ASSERT_TRUE(LayoutStore::setActiveId("nvs-test2"));
    got = LayoutStore::getActiveId();
    TEST_ASSERT_EQUAL_STRING("nvs-test2", got.c_str());
}

// --- AC #6 E2E: active-id that points to a missing file uses default ------

void test_load_with_missing_active_id_uses_default() {
    cleanLayoutsDir();
    LayoutStore::init();

    // Persist an id for a layout that does not exist on disk.
    ConfigManager::setLayoutActiveId("nonexistent");
    String activeId = LayoutStore::getActiveId();
    TEST_ASSERT_EQUAL_STRING("nonexistent", activeId.c_str());

    // load() with that id must return false and fill out with kDefaultLayoutJson.
    String out;
    bool found = LayoutStore::load(activeId.c_str(), out);
    TEST_ASSERT_FALSE(found);
    TEST_ASSERT_TRUE(out.indexOf("_default") >= 0);
    TEST_ASSERT_TRUE(out.indexOf("FLIGHTWALL") >= 0);
}

// --- Unity driver ---------------------------------------------------------

void setup() {
    delay(2000);

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed — aborting tests");
        return;
    }

    UNITY_BEGIN();

    // Init lifecycle
    RUN_TEST(test_init_creates_layouts_dir);
    RUN_TEST(test_init_existing_dir);

    // Save / load
    RUN_TEST(test_save_and_load_roundtrip);
    RUN_TEST(test_save_overwrites_existing);

    // Validation
    RUN_TEST(test_save_rejects_oversized);
    RUN_TEST(test_save_rejects_missing_fields);
    RUN_TEST(test_save_rejects_wrong_version);
    RUN_TEST(test_save_rejects_unknown_widget_type);
    RUN_TEST(test_save_rejects_malformed_json);
    RUN_TEST(test_save_rejects_bad_canvas);
    RUN_TEST(test_save_rejects_id_mismatch);
    RUN_TEST(test_save_rejects_reserved_default_id);

    // Caps
    RUN_TEST(test_cap_16_files);
    RUN_TEST(test_cap_64kb_bytes);

    // CRUD tail
    RUN_TEST(test_remove_erases_file);
    RUN_TEST(test_load_missing_returns_default);
    RUN_TEST(test_list_returns_all_layouts);
    RUN_TEST(test_unsafe_id_rejected);

    // NVS / active-id (AC #5, AC #6 E2E)
    RUN_TEST(test_active_id_roundtrip);
    RUN_TEST(test_load_with_missing_active_id_uses_default);

    cleanLayoutsDir();

    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
