/*
Purpose: Unity tests for CustomLayoutMode production wiring (Story le-1.3).

Covered acceptance criteria:
  AC #2 — init() loads JSON via LayoutStore, parses into WidgetSpec[] (fixed)
  AC #3 — LayoutStore::load() returning false (missing/unset active id) is
          non-fatal because load() fills `out` with the baked-in default
  AC #4 — render() iterates widgets and dispatches; zero heap per frame
          (validated structurally — no heap measurement in unit scope)
  AC #5 — malformed JSON sets _invalid = true and returns false from init();
          render() draws the error indicator instead of crashing
  AC #6 — MEMORY_REQUIREMENT covers MAX_WIDGETS * sizeof(WidgetSpec) —
          enforced by static_assert in CustomLayoutMode.cpp and exercised
          implicitly by the 24-widget max-density test
  AC #9 — the five test cases mandated by the story

Environment: esp32dev (on-device test — requires LittleFS + NVS for the
LayoutStore round-trips). Run with:
  pio test -e esp32dev --filter test_custom_layout_mode
*/
#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include <vector>

#include "core/ConfigManager.h"
#include "core/LayoutStore.h"
#include "core/WidgetRegistry.h"
#include "modes/CustomLayoutMode.h"
#include "models/FlightInfo.h"
#include "interfaces/DisplayMode.h"

// --- Test helpers ---------------------------------------------------------

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

// Minimal RenderContext with null matrix — widget render fns guard on
// ctx.matrix and no-op, which is the documented test contract across the
// LE-1.2 test files.
static RenderContext makeCtx() {
    RenderContext ctx{};
    ctx.matrix = nullptr;
    return ctx;
}

// Build a valid V1 layout JSON with N clock widgets — the allowlist accepts
// "clock" and our WidgetRegistry supports it. Staying under LAYOUT_STORE
// caps with N <= 24 (widget record ≈ 80 bytes of text → 24 × ~100 = ~2.4KB).
static String makeLayoutWithNWidgets(const char* id, size_t n) {
    String s;
    s.reserve(512 + n * 128);
    s += "{\"id\":\"";
    s += id;
    s += "\",\"name\":\"Test\",\"v\":1,";
    s += "\"canvas\":{\"w\":160,\"h\":32},\"bg\":\"#000000\",";
    s += "\"widgets\":[";
    for (size_t i = 0; i < n; ++i) {
        if (i > 0) s += ',';
        s += "{\"id\":\"w";
        s += String((unsigned)i);
        s += "\",\"type\":\"clock\",\"x\":0,\"y\":";
        s += String((unsigned)(i % 8));
        s += ",\"w\":80,\"h\":16,\"color\":\"#FFFFFF\"}";
    }
    s += "]}";
    return s;
}

static String makeLayoutWith8Widgets(const char* id) {
    return makeLayoutWithNWidgets(id, 8);
}

// --- AC #9 (a): init() with a valid 8-widget layout ----------------------

void test_init_valid_layout() {
    cleanLayoutsDir();
    LayoutStore::init();

    String payload = makeLayoutWith8Widgets("le13a");
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("le13a", payload.c_str()));
    TEST_ASSERT_TRUE(LayoutStore::setActiveId("le13a"));

    CustomLayoutMode mode;
    RenderContext ctx = makeCtx();
    TEST_ASSERT_TRUE(mode.init(ctx));
    TEST_ASSERT_EQUAL_UINT(8, (unsigned)mode.testWidgetCount());
    TEST_ASSERT_FALSE(mode.testInvalid());
}

// --- AC #9 (b): init() with LayoutStore::load() returning false is
// non-fatal (LayoutStore fills `out` with kDefaultLayoutJson). ------------

void test_init_missing_layout_nonfatal() {
    cleanLayoutsDir();
    LayoutStore::init();

    // Point active-id to a non-existent layout — LayoutStore::load() returns
    // false BUT populates `out` with the baked-in default layout.
    // isSafeLayoutId rejects ids starting with '_' only for "_default";
    // a regular non-existent id exercises the "file missing" path.
    TEST_ASSERT_TRUE(LayoutStore::setActiveId("nothere"));

    CustomLayoutMode mode;
    RenderContext ctx = makeCtx();
    // Non-fatal: init() returns true even though LayoutStore::load()
    // returned false, because the default layout JSON parsed successfully.
    TEST_ASSERT_TRUE(mode.init(ctx));
    TEST_ASSERT_FALSE(mode.testInvalid());
    // The baked-in default has 2 widgets (clock + text).
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)mode.testWidgetCount());
}

// --- AC #9 (c): malformed JSON → _invalid = true, init returns false -----

void test_init_malformed_json() {
    cleanLayoutsDir();
    LayoutStore::init();

    // We cannot save malformed JSON via LayoutStore::save() (schema
    // validation rejects it). To exercise the parseFromJson failure path we
    // write the file directly to LittleFS, then set active id and call
    // init(). LayoutStore::load() only guards size + existence — content is
    // passed through verbatim to the mode.
    File f = LittleFS.open("/layouts/broken.json", "w");
    TEST_ASSERT_TRUE((bool)f);
    const char kBad[] = "{not valid json at all";
    f.write(reinterpret_cast<const uint8_t*>(kBad), sizeof(kBad) - 1);
    f.close();
    TEST_ASSERT_TRUE(LayoutStore::setActiveId("broken"));

    CustomLayoutMode mode;
    RenderContext ctx = makeCtx();
    TEST_ASSERT_FALSE(mode.init(ctx));
    TEST_ASSERT_TRUE(mode.testInvalid());
}

// --- AC #9 (d): render() on an _invalid mode with null matrix must not
// crash (the matrix guard short-circuits before any hardware call). -------

void test_render_invalid_does_not_crash() {
    CustomLayoutMode mode;
    mode.testForceInvalid(true);

    RenderContext ctx = makeCtx();  // ctx.matrix == nullptr
    std::vector<FlightInfo> flights;
    // If the null-matrix guard in render() ever regresses, this line would
    // dereference nullptr and abort the test runner. Reaching the TEST_PASS
    // is itself the assertion.
    mode.render(ctx, flights);
    TEST_PASS();
}

// --- AC #9 (e): max-density 24-widget layout parses and renders ----------

void test_max_density_24_widgets() {
    cleanLayoutsDir();
    LayoutStore::init();

    String payload = makeLayoutWithNWidgets("max24", 24);
    TEST_ASSERT_EQUAL((int)LayoutStoreError::OK,
                      (int)LayoutStore::save("max24", payload.c_str()));
    TEST_ASSERT_TRUE(LayoutStore::setActiveId("max24"));

    CustomLayoutMode mode;
    RenderContext ctx = makeCtx();
    TEST_ASSERT_TRUE(mode.init(ctx));
    TEST_ASSERT_EQUAL_UINT(24, (unsigned)mode.testWidgetCount());

    // render() with null matrix must short-circuit — no crash.
    std::vector<FlightInfo> flights;
    mode.render(ctx, flights);
    TEST_PASS();
}

// --- Unity driver ---------------------------------------------------------

void setup() {
    delay(2000);

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed — aborting tests");
        return;
    }
    ConfigManager::init();

    UNITY_BEGIN();
    RUN_TEST(test_init_valid_layout);
    RUN_TEST(test_init_missing_layout_nonfatal);
    RUN_TEST(test_init_malformed_json);
    RUN_TEST(test_render_invalid_does_not_crash);
    RUN_TEST(test_max_density_24_widgets);
    cleanLayoutsDir();
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
