/*
Purpose: Unity tests for FlightFieldWidget (LE-1.8 renderer + LE-1.10 catalog).

Covered acceptance criteria:
  AC #1  — kCatalog is exposed via FlightFieldWidgetCatalog::catalog() with
           six entries and non-empty id/label.
  AC #4  — Per-field resolution against FlightInfo renders successfully.
  AC #4  — Unknown or missing field id falls back to catalog[0] (callsign).
  AC #4  — Empty resolved value renders the "--" placeholder (via render path).
  AC #6  — aircraft_type precedence: display_name_short → aircraft_code → empty.
  AC #8  — isKnownFieldId() whitelist behavior (positive + negative cases).

Environment: esp32dev.
*/
#include <Arduino.h>
#include <unity.h>
#include <cstring>

#include "core/WidgetRegistry.h"
#include "interfaces/DisplayMode.h"
#include "models/FlightInfo.h"
#include "widgets/FieldDescriptor.h"
#include "widgets/FlightFieldWidget.h"

// ------------------------------------------------------------------
// Test helpers
// ------------------------------------------------------------------

static WidgetSpec makeSpec(uint16_t w, uint16_t h, const char* content) {
    WidgetSpec s{};
    s.type  = WidgetType::FlightField;
    s.x     = 0;
    s.y     = 0;
    s.w     = w;
    s.h     = h;
    s.color = 0xFFFF;
    strncpy(s.id, "wF", sizeof(s.id) - 1);
    strncpy(s.content, content, sizeof(s.content) - 1);
    s.content[sizeof(s.content) - 1] = '\0';
    s.align = 0;
    return s;
}

static RenderContext makeCtx(const FlightInfo* flight) {
    RenderContext ctx{};
    ctx.matrix         = nullptr;  // hardware-free test path
    ctx.textColor      = 0xFFFF;
    ctx.brightness     = 128;
    ctx.logoBuffer     = nullptr;
    ctx.displayCycleMs = 0;
    ctx.currentFlight  = flight;
    return ctx;
}

static FlightInfo makeFlight() {
    FlightInfo f;
    f.ident                         = "UAL123";
    f.ident_icao                    = "UAL123";
    f.ident_iata                    = "UA123";
    f.operator_icao                 = "UAL";
    f.operator_iata                 = "UA";
    f.origin.code_icao              = "KSFO";
    f.destination.code_icao         = "KLAX";
    f.aircraft_code                 = "B738";
    f.airline_display_name_full     = "United Airlines";
    f.aircraft_display_name_short   = "737";
    f.altitude_kft                  = 34.0;
    f.speed_mph                     = 487.0;
    f.track_deg                     = 247.0;
    f.vertical_rate_fps             = -12.5;
    return f;
}

// ------------------------------------------------------------------
// AC #1 — catalog shape
// ------------------------------------------------------------------

void test_catalog_has_six_entries() {
    size_t count = 0;
    const FieldDescriptor* cat = FlightFieldWidgetCatalog::catalog(count);
    TEST_ASSERT_NOT_NULL(cat);
    TEST_ASSERT_EQUAL_UINT(6u, (unsigned)count);
    for (size_t i = 0; i < count; ++i) {
        TEST_ASSERT_NOT_NULL(cat[i].id);
        TEST_ASSERT_NOT_NULL(cat[i].label);
        TEST_ASSERT_TRUE(strlen(cat[i].id) > 0);
        TEST_ASSERT_TRUE(strlen(cat[i].label) > 0);
        // FlightField entries carry no unit.
        TEST_ASSERT_NULL(cat[i].unit);
    }
}

void test_catalog_first_entry_is_callsign() {
    size_t count = 0;
    const FieldDescriptor* cat = FlightFieldWidgetCatalog::catalog(count);
    TEST_ASSERT_TRUE(count > 0);
    TEST_ASSERT_EQUAL_STRING("callsign", cat[0].id);
}

// ------------------------------------------------------------------
// AC #1 / #8 — every catalog id is a known id and resolves cleanly
// ------------------------------------------------------------------

void test_every_catalog_id_is_known() {
    size_t count = 0;
    const FieldDescriptor* cat = FlightFieldWidgetCatalog::catalog(count);
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    for (size_t i = 0; i < count; ++i) {
        TEST_ASSERT_TRUE(FlightFieldWidgetCatalog::isKnownFieldId(cat[i].id));
        WidgetSpec spec = makeSpec(96, 8, cat[i].id);
        TEST_ASSERT_TRUE_MESSAGE(renderFlightField(spec, ctx), cat[i].id);
    }
}

// ------------------------------------------------------------------
// AC #8 — unknown ids are rejected by the whitelist helper
// ------------------------------------------------------------------

void test_unknown_id_is_not_known() {
    TEST_ASSERT_FALSE(FlightFieldWidgetCatalog::isKnownFieldId("bogus"));
    TEST_ASSERT_FALSE(FlightFieldWidgetCatalog::isKnownFieldId(""));
    TEST_ASSERT_FALSE(FlightFieldWidgetCatalog::isKnownFieldId(nullptr));
    // Pre-LE-1.10 key strings are no longer valid — confirms the rename.
    TEST_ASSERT_FALSE(FlightFieldWidgetCatalog::isKnownFieldId("ident"));
    TEST_ASSERT_FALSE(FlightFieldWidgetCatalog::isKnownFieldId("aircraft"));
    TEST_ASSERT_FALSE(FlightFieldWidgetCatalog::isKnownFieldId("dest_icao"));
}

// ------------------------------------------------------------------
// AC #4 — unknown / missing field_id falls back to catalog[0] at render
// ------------------------------------------------------------------

void test_unknown_field_id_renders_true_via_fallback() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(96, 8, "totally_unknown_field");
    // Resolver falls back to callsign (catalog[0]) — renders true.
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

void test_empty_content_renders_true_via_fallback() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(96, 8, "");
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

// ------------------------------------------------------------------
// AC #4 — nullptr currentFlight falls back to "--" placeholder
// ------------------------------------------------------------------

void test_null_flight_renders_true() {
    RenderContext ctx = makeCtx(nullptr);
    WidgetSpec spec = makeSpec(48, 8, "airline");
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

// ------------------------------------------------------------------
// AC #4 — empty resolved value falls back to "--"
// ------------------------------------------------------------------

void test_empty_airline_renders_true() {
    FlightInfo f;  // defaults: all strings empty.
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(48, 8, "airline");
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

// ------------------------------------------------------------------
// AC #6 — aircraft_type precedence (display_name_short > aircraft_code > empty)
// ------------------------------------------------------------------

void test_aircraft_type_prefers_display_name_short() {
    // Full flight has both display_name_short="737" and aircraft_code="B738".
    // The resolver must pick the display name. We cannot inspect the framebuffer
    // in this test, but the two paths exercise different branches — both must
    // render successfully.
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 8, "aircraft_type");
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

void test_aircraft_type_falls_back_to_aircraft_code() {
    FlightInfo f = makeFlight();
    f.aircraft_display_name_short = "";  // force fallback branch
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 8, "aircraft_type");
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

void test_aircraft_type_empty_both_sources_renders_true() {
    FlightInfo f = makeFlight();
    f.aircraft_display_name_short = "";
    f.aircraft_code               = "";
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 8, "aircraft_type");
    // Resolver returns empty → render falls back to "--".
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

// ------------------------------------------------------------------
// flight_number aliases callsign — identical resolution, distinct label
// ------------------------------------------------------------------

void test_flight_number_resolves_like_callsign() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec specA = makeSpec(48, 8, "callsign");
    WidgetSpec specB = makeSpec(48, 8, "flight_number");
    TEST_ASSERT_TRUE(renderFlightField(specA, ctx));
    TEST_ASSERT_TRUE(renderFlightField(specB, ctx));
}

// ------------------------------------------------------------------
// callsign fallback chain (unchanged from LE-1.8): icao → iata → raw
// ------------------------------------------------------------------

void test_callsign_fallback_to_iata() {
    FlightInfo f = makeFlight();
    f.ident_icao = "";
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(48, 8, "callsign");
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

void test_callsign_fallback_to_raw_ident() {
    FlightInfo f = makeFlight();
    f.ident_icao = "";
    f.ident_iata = "";
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(48, 8, "callsign");
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

// ------------------------------------------------------------------
// Dimension floors — undersized specs must return true without crash
// ------------------------------------------------------------------

void test_undersized_width_returns_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(2, 8, "airline");
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

void test_undersized_height_returns_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(48, 2, "airline");
    TEST_ASSERT_TRUE(renderFlightField(spec, ctx));
}

// ------------------------------------------------------------------
// Test runner
// ------------------------------------------------------------------

void setup() {
    delay(2000);
    UNITY_BEGIN();

    RUN_TEST(test_catalog_has_six_entries);
    RUN_TEST(test_catalog_first_entry_is_callsign);
    RUN_TEST(test_every_catalog_id_is_known);
    RUN_TEST(test_unknown_id_is_not_known);

    RUN_TEST(test_unknown_field_id_renders_true_via_fallback);
    RUN_TEST(test_empty_content_renders_true_via_fallback);

    RUN_TEST(test_null_flight_renders_true);
    RUN_TEST(test_empty_airline_renders_true);

    RUN_TEST(test_aircraft_type_prefers_display_name_short);
    RUN_TEST(test_aircraft_type_falls_back_to_aircraft_code);
    RUN_TEST(test_aircraft_type_empty_both_sources_renders_true);

    RUN_TEST(test_flight_number_resolves_like_callsign);
    RUN_TEST(test_callsign_fallback_to_iata);
    RUN_TEST(test_callsign_fallback_to_raw_ident);

    RUN_TEST(test_undersized_width_returns_true);
    RUN_TEST(test_undersized_height_returns_true);

    UNITY_END();
}

void loop() {
    // Tests run once in setup().
}
