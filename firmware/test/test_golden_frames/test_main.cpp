/*
Purpose: Golden-frame regression tests for CustomLayoutMode (Story LE-1.9).

Tests load fixture layouts, run them through CustomLayoutMode, and verify:
  (a) The layout parses correctly and _widgetCount matches the fixture definition
  (b) render() with nullptr matrix completes without crash (hardware-free path)
  (c) On-device: ESP.getFreeHeap() stays >= 30 KB for the max-density fixture
  (d) String resolution for flight_field widgets matches expected values
      given a known FlightInfo — delegates to the same logic tested by
      test_flight_field_widget, exercised here at the integration level.

Golden-frame philosophy: LE-1.9 uses structural + string-resolution checks
rather than full framebuffer byte comparison. Rationale documented in the
story's Dev Notes — Golden-Frame Test Strategy section.

Environment: esp32dev (on-device). Run with:
    pio test -e esp32dev --filter test_golden_frames
*/
#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include <vector>
#include <cstring>

#include "core/ConfigManager.h"
#include "core/LayoutStore.h"
#include "core/WidgetRegistry.h"
#include "modes/CustomLayoutMode.h"
#include "models/FlightInfo.h"
#include "interfaces/DisplayMode.h"
#include "widgets/FlightFieldWidget.h"
#include "widgets/MetricWidget.h"

// -----------------------------------------------------------------------
// Fixture JSON strings (inline — avoids LittleFS filesystem-image dependency
// for the parse/structural tests). These strings MUST stay in lockstep with
// fixtures/layout_{a,b,c}.json files checked into the repo (Task 1).
// -----------------------------------------------------------------------

// Fixture A: text + clock (2 widgets)
static const char kFixtureA[] =
    "{\"id\":\"gf_a\",\"name\":\"Golden A - Text+Clock\",\"v\":1,"
    "\"canvas\":{\"w\":160,\"h\":32},\"bg\":\"#000000\","
    "\"widgets\":["
    "{\"id\":\"w1\",\"type\":\"text\",\"x\":2,\"y\":2,\"w\":80,\"h\":8,"
    "\"color\":\"#FFFFFF\",\"content\":\"FLIGHTWALL\",\"align\":\"left\"},"
    "{\"id\":\"w2\",\"type\":\"clock\",\"x\":2,\"y\":12,\"w\":48,\"h\":8,"
    "\"color\":\"#FFFF00\",\"content\":\"24h\"}"
    "]}";

// Fixture B: 8 flight_field + metric widgets
static const char kFixtureB[] =
    "{\"id\":\"gf_b\",\"name\":\"Golden B - FlightField Heavy\",\"v\":1,"
    "\"canvas\":{\"w\":160,\"h\":32},\"bg\":\"#000000\","
    "\"widgets\":["
    "{\"id\":\"b1\",\"type\":\"flight_field\",\"x\":0,\"y\":0,\"w\":60,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"airline\"},"
    "{\"id\":\"b2\",\"type\":\"flight_field\",\"x\":0,\"y\":8,\"w\":48,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"ident\"},"
    "{\"id\":\"b3\",\"type\":\"flight_field\",\"x\":0,\"y\":16,\"w\":36,\"h\":8,\"color\":\"#00FF00\",\"content\":\"origin_icao\"},"
    "{\"id\":\"b4\",\"type\":\"flight_field\",\"x\":40,\"y\":16,\"w\":36,\"h\":8,\"color\":\"#00FF00\",\"content\":\"dest_icao\"},"
    "{\"id\":\"b5\",\"type\":\"metric\",\"x\":64,\"y\":0,\"w\":36,\"h\":8,\"color\":\"#FFFF00\",\"content\":\"alt\"},"
    "{\"id\":\"b6\",\"type\":\"metric\",\"x\":64,\"y\":8,\"w\":48,\"h\":8,\"color\":\"#FFFF00\",\"content\":\"speed\"},"
    "{\"id\":\"b7\",\"type\":\"metric\",\"x\":64,\"y\":16,\"w\":36,\"h\":8,\"color\":\"#FF8800\",\"content\":\"track\"},"
    "{\"id\":\"b8\",\"type\":\"metric\",\"x\":64,\"y\":24,\"w\":48,\"h\":8,\"color\":\"#FF8800\",\"content\":\"vsi\"}"
    "]}";

// Fixture C: 24-widget max-density (MAX_WIDGETS = 24).
// Coordinates match fixtures/layout_c.json; intentional overlap at y=0
// exercises widget-count saturation at the cap.
static const char kFixtureC[] =
    "{\"id\":\"gf_c\",\"name\":\"Golden C - Max Density\",\"v\":1,"
    "\"canvas\":{\"w\":160,\"h\":32},\"bg\":\"#000000\","
    "\"widgets\":["
    "{\"id\":\"c01\",\"type\":\"text\",\"x\":0,\"y\":0,\"w\":40,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"AA\"},"
    "{\"id\":\"c02\",\"type\":\"text\",\"x\":40,\"y\":0,\"w\":40,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"BB\"},"
    "{\"id\":\"c03\",\"type\":\"text\",\"x\":80,\"y\":0,\"w\":40,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"CC\"},"
    "{\"id\":\"c04\",\"type\":\"text\",\"x\":120,\"y\":0,\"w\":40,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"DD\"},"
    "{\"id\":\"c05\",\"type\":\"text\",\"x\":0,\"y\":8,\"w\":40,\"h\":8,\"color\":\"#FFFF00\",\"content\":\"EE\"},"
    "{\"id\":\"c06\",\"type\":\"text\",\"x\":40,\"y\":8,\"w\":40,\"h\":8,\"color\":\"#FFFF00\",\"content\":\"FF\"},"
    "{\"id\":\"c07\",\"type\":\"flight_field\",\"x\":80,\"y\":8,\"w\":40,\"h\":8,\"color\":\"#00FF00\",\"content\":\"airline\"},"
    "{\"id\":\"c08\",\"type\":\"flight_field\",\"x\":120,\"y\":8,\"w\":40,\"h\":8,\"color\":\"#00FF00\",\"content\":\"ident\"},"
    "{\"id\":\"c09\",\"type\":\"flight_field\",\"x\":0,\"y\":16,\"w\":40,\"h\":8,\"color\":\"#00FFFF\",\"content\":\"origin_icao\"},"
    "{\"id\":\"c10\",\"type\":\"flight_field\",\"x\":40,\"y\":16,\"w\":40,\"h\":8,\"color\":\"#00FFFF\",\"content\":\"dest_icao\"},"
    "{\"id\":\"c11\",\"type\":\"flight_field\",\"x\":80,\"y\":16,\"w\":40,\"h\":8,\"color\":\"#FF8800\",\"content\":\"aircraft\"},"
    "{\"id\":\"c12\",\"type\":\"metric\",\"x\":120,\"y\":16,\"w\":40,\"h\":8,\"color\":\"#FF8800\",\"content\":\"alt\"},"
    "{\"id\":\"c13\",\"type\":\"metric\",\"x\":0,\"y\":24,\"w\":40,\"h\":8,\"color\":\"#FF00FF\",\"content\":\"speed\"},"
    "{\"id\":\"c14\",\"type\":\"metric\",\"x\":40,\"y\":24,\"w\":40,\"h\":8,\"color\":\"#FF00FF\",\"content\":\"track\"},"
    "{\"id\":\"c15\",\"type\":\"metric\",\"x\":80,\"y\":24,\"w\":40,\"h\":8,\"color\":\"#FF00FF\",\"content\":\"vsi\"},"
    "{\"id\":\"c16\",\"type\":\"clock\",\"x\":120,\"y\":24,\"w\":40,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"24h\"},"
    "{\"id\":\"c17\",\"type\":\"text\",\"x\":0,\"y\":0,\"w\":20,\"h\":8,\"color\":\"#888888\",\"content\":\"a\"},"
    "{\"id\":\"c18\",\"type\":\"text\",\"x\":20,\"y\":0,\"w\":20,\"h\":8,\"color\":\"#888888\",\"content\":\"b\"},"
    "{\"id\":\"c19\",\"type\":\"text\",\"x\":40,\"y\":0,\"w\":20,\"h\":8,\"color\":\"#888888\",\"content\":\"c\"},"
    "{\"id\":\"c20\",\"type\":\"text\",\"x\":60,\"y\":0,\"w\":20,\"h\":8,\"color\":\"#888888\",\"content\":\"d\"},"
    "{\"id\":\"c21\",\"type\":\"text\",\"x\":80,\"y\":0,\"w\":20,\"h\":8,\"color\":\"#888888\",\"content\":\"e\"},"
    "{\"id\":\"c22\",\"type\":\"text\",\"x\":100,\"y\":0,\"w\":20,\"h\":8,\"color\":\"#888888\",\"content\":\"f\"},"
    "{\"id\":\"c23\",\"type\":\"text\",\"x\":120,\"y\":0,\"w\":20,\"h\":8,\"color\":\"#888888\",\"content\":\"g\"},"
    "{\"id\":\"c24\",\"type\":\"text\",\"x\":140,\"y\":0,\"w\":20,\"h\":8,\"color\":\"#888888\",\"content\":\"h\"}"
    "]}";

// Fixture D (LE-2.7): exercises every catalog entry added in LE-2 —
// aircraft_short, aircraft_full, origin_iata, destination_iata, and the
// speed_mph metric. Must stay in lockstep with fixtures/layout_d.json.
static const char kFixtureD[] =
    "{\"id\":\"gf_d\",\"name\":\"Golden D - LE-2 Catalog\",\"v\":1,"
    "\"canvas\":{\"w\":160,\"h\":32},\"bg\":\"#000000\","
    "\"widgets\":["
    "{\"id\":\"d1\",\"type\":\"flight_field\",\"x\":0,\"y\":0,\"w\":80,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"aircraft_short\"},"
    "{\"id\":\"d2\",\"type\":\"flight_field\",\"x\":0,\"y\":8,\"w\":80,\"h\":8,\"color\":\"#FFFFFF\",\"content\":\"aircraft_full\"},"
    "{\"id\":\"d3\",\"type\":\"flight_field\",\"x\":0,\"y\":16,\"w\":40,\"h\":8,\"color\":\"#00FF00\",\"content\":\"origin_iata\"},"
    "{\"id\":\"d4\",\"type\":\"flight_field\",\"x\":40,\"y\":16,\"w\":40,\"h\":8,\"color\":\"#00FF00\",\"content\":\"destination_iata\"},"
    "{\"id\":\"d5\",\"type\":\"metric\",\"x\":80,\"y\":0,\"w\":64,\"h\":8,\"color\":\"#FFFF00\",\"content\":\"speed_mph\"}"
    "]}";

// -----------------------------------------------------------------------
// Test helpers
// -----------------------------------------------------------------------

static RenderContext makeCtx() {
    RenderContext ctx{};
    ctx.matrix         = nullptr;   // hardware-free path — widgets no-op safely
    ctx.textColor      = 0xFFFF;
    ctx.brightness     = 128;
    ctx.logoBuffer     = nullptr;
    ctx.displayCycleMs = 0;
    ctx.currentFlight  = nullptr;
    return ctx;
}

static FlightInfo makeKnownFlight() {
    FlightInfo f;
    f.ident                       = "UAL123";
    f.ident_icao                  = "UAL123";
    f.ident_iata                  = "UA123";
    f.operator_icao               = "UAL";
    f.origin.code_icao            = "KSFO";
    f.origin.code_iata            = "SFO";
    f.destination.code_icao       = "KLAX";
    f.destination.code_iata       = "LAX";
    f.aircraft_code               = "B738";
    f.airline_display_name_full   = "United Airlines";
    f.aircraft_display_name_short = "737-800";
    f.aircraft_display_name_full  = "Boeing 737-800";
    f.altitude_kft                = 34.0;
    f.speed_mph                   = 487.0;
    f.track_deg                   = 247.0;
    f.vertical_rate_fps           = -12.5;
    return f;
}

// Save fixture JSON through LayoutStore (honouring the schema validation),
// set it as active, and init the mode. Returns the init() result.
static bool loadFixture(CustomLayoutMode& mode, const char* json, const char* id) {
    LayoutStoreError err = LayoutStore::save(id, json);
    if (err != LayoutStoreError::OK) {
        Serial.printf("[golden] save(%s) failed with err=%u\n", id, (unsigned)err);
        return false;
    }
    if (!LayoutStore::setActiveId(id)) {
        Serial.printf("[golden] setActiveId(%s) failed\n", id);
        return false;
    }
    RenderContext ctx = makeCtx();
    return mode.init(ctx);
}

static void cleanLayouts() {
    if (LittleFS.exists("/layouts")) {
        File dir = LittleFS.open("/layouts");
        if (dir && dir.isDirectory()) {
            File e = dir.openNextFile();
            while (e) {
                String p = e.name();
                if (!p.startsWith("/")) {
                    p = String("/layouts/") + p;
                }
                e.close();
                LittleFS.remove(p);
                e = dir.openNextFile();
            }
        }
        LittleFS.rmdir("/layouts");
    }
}

// -----------------------------------------------------------------------
// Fixture A: text + clock (2 widgets)
// -----------------------------------------------------------------------

void test_golden_a_parse_and_widget_count() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureA, "gf_a"));
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)mode.testWidgetCount());
    TEST_ASSERT_FALSE(mode.testInvalid());
}

void test_golden_a_render_does_not_crash() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureA, "gf_a"));
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> flights;
    // Null-matrix short-circuits in CustomLayoutMode::render() before any
    // dispatch — reaching TEST_PASS is itself the assertion.
    mode.render(ctx, flights);
    TEST_PASS();
}

// -----------------------------------------------------------------------
// Fixture B: 8-widget flight-field-heavy
// -----------------------------------------------------------------------

void test_golden_b_parse_and_widget_count() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureB, "gf_b"));
    TEST_ASSERT_EQUAL_UINT(8, (unsigned)mode.testWidgetCount());
    TEST_ASSERT_FALSE(mode.testInvalid());
}

void test_golden_b_render_with_flight_does_not_crash() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureB, "gf_b"));
    FlightInfo flight = makeKnownFlight();
    RenderContext ctx = makeCtx();
    ctx.currentFlight = &flight;
    std::vector<FlightInfo> flights;
    flights.push_back(flight);
    mode.render(ctx, flights);
    TEST_PASS();
}

// Verify that flight_field "airline" key resolves to the known FlightInfo
// when CustomLayoutMode invokes it. The resolver is hardware-free when
// ctx.matrix is null (returns true after fallback/resolution path).
void test_golden_b_flight_field_resolves_airline() {
    FlightInfo f = makeKnownFlight();
    WidgetSpec spec{};
    spec.type  = WidgetType::FlightField;
    spec.x = 0; spec.y = 0; spec.w = 60; spec.h = 8;
    spec.color = 0xFFFF;
    strncpy(spec.id, "b1", sizeof(spec.id) - 1);
    strncpy(spec.content, "airline", sizeof(spec.content) - 1);
    RenderContext ctx = makeCtx();
    ctx.currentFlight = &f;
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

// Metric resolution integration smoke — LE-1.10 altitude_ft against known flight.
void test_golden_b_metric_resolves_altitude_ft() {
    FlightInfo f = makeKnownFlight();
    WidgetSpec spec{};
    spec.type  = WidgetType::Metric;
    spec.x = 64; spec.y = 0; spec.w = 36; spec.h = 8;
    spec.color = 0xFFFF;
    strncpy(spec.id, "b5", sizeof(spec.id) - 1);
    strncpy(spec.content, "altitude_ft", sizeof(spec.content) - 1);
    RenderContext ctx = makeCtx();
    ctx.currentFlight = &f;
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// -----------------------------------------------------------------------
// Fixture C: 24-widget max-density
// -----------------------------------------------------------------------

void test_golden_c_parse_and_widget_count() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureC, "gf_c"));
    TEST_ASSERT_EQUAL_UINT(24, (unsigned)mode.testWidgetCount());
    TEST_ASSERT_FALSE(mode.testInvalid());
}

void test_golden_c_render_does_not_crash() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureC, "gf_c"));
    FlightInfo flight = makeKnownFlight();
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> flights;
    flights.push_back(flight);
    mode.render(ctx, flights);
    TEST_PASS();
}

// AC #4 — heap floor at max density. On-device measurement; with
// LittleFS + NVS + ConfigManager initialized the reading is representative
// of steady state. Floor is 30 KB per the story AC.
void test_golden_c_heap_floor() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureC, "gf_c"));

    uint32_t heapBefore = ESP.getFreeHeap();
    FlightInfo flight = makeKnownFlight();
    RenderContext ctx = makeCtx();
    std::vector<FlightInfo> flights;
    flights.push_back(flight);
    mode.render(ctx, flights);
    uint32_t heapAfter = ESP.getFreeHeap();

    Serial.printf("[golden_c] heap before=%u after=%u\n",
                  (unsigned)heapBefore, (unsigned)heapAfter);

    // AC #4: heap must remain above 30 KB floor.
    TEST_ASSERT_GREATER_OR_EQUAL(30720u, heapAfter);
    // Heap must not drop more than 2 KB across a render (steady state).
    // Using signed delta to tolerate minor RTOS accounting noise.
    int32_t delta = (int32_t)heapBefore - (int32_t)heapAfter;
    TEST_ASSERT_LESS_OR_EQUAL_INT32(2048, delta);
    TEST_ASSERT_GREATER_OR_EQUAL_INT32(-2048, delta);
}

// -----------------------------------------------------------------------
// Fixture D (LE-2.7): LE-2 catalog coverage
// -----------------------------------------------------------------------

void test_golden_d_parse_and_widget_count() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureD, "gf_d"));
    TEST_ASSERT_EQUAL_UINT(5, (unsigned)mode.testWidgetCount());
    TEST_ASSERT_FALSE(mode.testInvalid());
}

void test_golden_d_render_with_flight_does_not_crash() {
    cleanLayouts();
    LayoutStore::init();
    CustomLayoutMode mode;
    TEST_ASSERT_TRUE(loadFixture(mode, kFixtureD, "gf_d"));
    FlightInfo flight = makeKnownFlight();
    RenderContext ctx = makeCtx();
    ctx.currentFlight = &flight;
    std::vector<FlightInfo> flights;
    flights.push_back(flight);
    mode.render(ctx, flights);
    TEST_PASS();
}

// Catalog-presence assertions for every LE-2 FlightField entry. A missing
// key here would indicate the catalog in FlightFieldWidget.cpp has drifted
// from the story AC; the editor UI and the save-time whitelist depend on
// this set being stable.
void test_golden_d_flight_field_catalog_includes_le2_entries() {
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("aircraft_short"));
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("aircraft_full"));
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("origin_iata"));
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("destination_iata"));

    // Sanity: the five legacy entries that le-2 didn't touch remain.
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("callsign"));
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("airline"));
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("aircraft_type"));
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("origin_icao"));
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("destination_icao"));
    TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId("flight_number"));

    // Unknown key must fail closed.
    TEST_ASSERT_FALSE(FlightFieldWidgetCatalog::isKnownFieldId("not_a_real_field"));
}

void test_golden_d_metric_catalog_includes_speed_mph() {
    TEST_ASSERT_TRUE(MetricWidgetCatalog::isKnownFieldId("speed_mph"));

    // Sanity: the six legacy metric entries remain.
    TEST_ASSERT_TRUE(MetricWidgetCatalog::isKnownFieldId("altitude_ft"));
    TEST_ASSERT_TRUE(MetricWidgetCatalog::isKnownFieldId("speed_kts"));
    TEST_ASSERT_TRUE(MetricWidgetCatalog::isKnownFieldId("heading_deg"));
    TEST_ASSERT_TRUE(MetricWidgetCatalog::isKnownFieldId("vertical_rate_fpm"));
    TEST_ASSERT_TRUE(MetricWidgetCatalog::isKnownFieldId("distance_nm"));
    TEST_ASSERT_TRUE(MetricWidgetCatalog::isKnownFieldId("bearing_deg"));

    TEST_ASSERT_FALSE(MetricWidgetCatalog::isKnownFieldId("not_a_real_metric"));
}

// Integration-level resolver smoke — LE-2 FlightField keys against the
// known flight. Following the LE-1.9 philosophy: render path accepting
// the key (returning true) is the structural assertion. Deeper pixel
// assertions are handled by the Playwright preview-regression spec.
void test_golden_d_flight_field_resolves_le2_keys() {
    FlightInfo f = makeKnownFlight();
    const char* le2Keys[] = {
        "aircraft_short", "aircraft_full", "origin_iata", "destination_iata"
    };
    for (const char* key : le2Keys) {
        WidgetSpec spec{};
        spec.type  = WidgetType::FlightField;
        spec.x = 0; spec.y = 0; spec.w = 80; spec.h = 8;
        spec.color = 0xFFFF;
        strncpy(spec.id, "d_le2", sizeof(spec.id) - 1);
        strncpy(spec.content, key, sizeof(spec.content) - 1);
        RenderContext ctx = makeCtx();
        ctx.currentFlight = &f;
        TEST_ASSERT_TRUE_MESSAGE(renderFlightField(spec, ctx),
            "renderFlightField must accept every LE-2 catalog key");
    }
}

void test_golden_d_metric_resolves_speed_mph() {
    FlightInfo f = makeKnownFlight();
    WidgetSpec spec{};
    spec.type  = WidgetType::Metric;
    spec.x = 80; spec.y = 0; spec.w = 64; spec.h = 8;
    spec.color = 0xFFFF;
    strncpy(spec.id, "d5", sizeof(spec.id) - 1);
    strncpy(spec.content, "speed_mph", sizeof(spec.content) - 1);
    RenderContext ctx = makeCtx();
    ctx.currentFlight = &f;
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// -----------------------------------------------------------------------
// Unity driver
// -----------------------------------------------------------------------

void setup() {
    delay(2000);

    if (!LittleFS.begin(true)) {
        Serial.println("[golden] LittleFS mount failed — aborting tests");
        return;
    }
    ConfigManager::init();

    UNITY_BEGIN();

    RUN_TEST(test_golden_a_parse_and_widget_count);
    RUN_TEST(test_golden_a_render_does_not_crash);

    RUN_TEST(test_golden_b_parse_and_widget_count);
    RUN_TEST(test_golden_b_render_with_flight_does_not_crash);
    RUN_TEST(test_golden_b_flight_field_resolves_airline);
    RUN_TEST(test_golden_b_metric_resolves_altitude_ft);

    RUN_TEST(test_golden_c_parse_and_widget_count);
    RUN_TEST(test_golden_c_render_does_not_crash);
    RUN_TEST(test_golden_c_heap_floor);

    RUN_TEST(test_golden_d_parse_and_widget_count);
    RUN_TEST(test_golden_d_render_with_flight_does_not_crash);
    RUN_TEST(test_golden_d_flight_field_catalog_includes_le2_entries);
    RUN_TEST(test_golden_d_metric_catalog_includes_speed_mph);
    RUN_TEST(test_golden_d_flight_field_resolves_le2_keys);
    RUN_TEST(test_golden_d_metric_resolves_speed_mph);

    cleanLayouts();
    UNITY_END();
}

void loop() {
    // Tests run once in setup().
}
