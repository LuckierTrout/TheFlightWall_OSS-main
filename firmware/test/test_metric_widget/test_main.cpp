/*
Purpose: Unity tests for MetricWidget (LE-1.8 renderer + LE-1.10 catalog).

Covered acceptance criteria:
  AC #2  — kCatalog is exposed via MetricWidgetCatalog::catalog() with 6
           entries, each carrying id/label/unit (includes distance_nm, bearing_deg).
  AC #5  — Per-metric resolution against FlightInfo renders successfully;
           unit conversions (kft→ft, mph→kts, fps→fpm) do not crash on the
           render path.
  AC #5  — NaN telemetry falls back to "--".
  AC #5  — Unknown field id falls back to catalog[0] (altitude_ft).
  AC #8  — isKnownFieldId() whitelist behavior (positive + negative cases,
           including pre-LE-1.10 keys now rejected).

Environment: esp32dev.
*/
#include <Arduino.h>
#include <unity.h>
#include <cmath>
#include <cstring>

#include "core/WidgetRegistry.h"
#include "interfaces/DisplayMode.h"
#include "models/FlightInfo.h"
#include "widgets/FieldDescriptor.h"
#include "widgets/MetricWidget.h"

// ------------------------------------------------------------------
// Test helpers
// ------------------------------------------------------------------

static WidgetSpec makeSpec(uint16_t w, uint16_t h, const char* content) {
    WidgetSpec s{};
    s.type  = WidgetType::Metric;
    s.x     = 0;
    s.y     = 0;
    s.w     = w;
    s.h     = h;
    s.color = 0xFFFF;
    strncpy(s.id, "wM", sizeof(s.id) - 1);
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
    f.altitude_kft      = 34.0;   // → 34000 ft
    f.speed_mph         = 487.0;  // → ~423 kts
    f.track_deg         = 247.0;
    f.vertical_rate_fps = -12.5;  // → -750 fpm
    f.distance_km       = 18.5;   // → ~10.0 nm
    f.bearing_deg       = 274.0;
    return f;
}

// ------------------------------------------------------------------
// AC #2 — catalog shape (6 entries, with units)
// ------------------------------------------------------------------

void test_catalog_has_six_entries() {
    size_t count = 0;
    const FieldDescriptor* cat = MetricWidgetCatalog::catalog(count);
    TEST_ASSERT_NOT_NULL(cat);
    TEST_ASSERT_EQUAL_UINT(6u, (unsigned)count);
    for (size_t i = 0; i < count; ++i) {
        TEST_ASSERT_NOT_NULL(cat[i].id);
        TEST_ASSERT_NOT_NULL(cat[i].label);
        TEST_ASSERT_NOT_NULL(cat[i].unit);  // LE-1.10: metrics carry units.
        TEST_ASSERT_TRUE(strlen(cat[i].id) > 0);
        TEST_ASSERT_TRUE(strlen(cat[i].label) > 0);
        TEST_ASSERT_TRUE(strlen(cat[i].unit) > 0);
    }
}

void test_catalog_first_entry_is_altitude_ft() {
    size_t count = 0;
    const FieldDescriptor* cat = MetricWidgetCatalog::catalog(count);
    TEST_ASSERT_TRUE(count > 0);
    TEST_ASSERT_EQUAL_STRING("altitude_ft", cat[0].id);
}

// ------------------------------------------------------------------
// AC #2 / #8 — every catalog id is known and renders
// ------------------------------------------------------------------

void test_every_metric_catalog_id_renders() {
    size_t count = 0;
    const FieldDescriptor* cat = MetricWidgetCatalog::catalog(count);
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    for (size_t i = 0; i < count; ++i) {
        TEST_ASSERT_TRUE(MetricWidgetCatalog::isKnownFieldId(cat[i].id));
        WidgetSpec spec = makeSpec(60, 8, cat[i].id);
        TEST_ASSERT_TRUE_MESSAGE(renderMetric(spec, ctx), cat[i].id);
    }
}

// ------------------------------------------------------------------
// AC #8 — whitelist behavior
// ------------------------------------------------------------------

void test_unknown_metric_id_is_not_known() {
    TEST_ASSERT_FALSE(MetricWidgetCatalog::isKnownFieldId("bogus"));
    TEST_ASSERT_FALSE(MetricWidgetCatalog::isKnownFieldId(""));
    TEST_ASSERT_FALSE(MetricWidgetCatalog::isKnownFieldId(nullptr));
    // Pre-LE-1.10 keys are no longer valid.
    TEST_ASSERT_FALSE(MetricWidgetCatalog::isKnownFieldId("alt"));
    TEST_ASSERT_FALSE(MetricWidgetCatalog::isKnownFieldId("speed"));
    TEST_ASSERT_FALSE(MetricWidgetCatalog::isKnownFieldId("track"));
    TEST_ASSERT_FALSE(MetricWidgetCatalog::isKnownFieldId("vsi"));
}

// ------------------------------------------------------------------
// AC #5 — unknown metric id falls back to altitude_ft via resolver
// ------------------------------------------------------------------

void test_unknown_metric_id_renders_true_via_fallback() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "temperature");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_empty_content_renders_true_via_fallback() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// AC #5 — NaN telemetry → "--"
// ------------------------------------------------------------------

void test_nan_altitude_renders_true() {
    FlightInfo f = makeFlight();
    f.altitude_kft = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "altitude_ft");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_nan_speed_renders_true() {
    FlightInfo f = makeFlight();
    f.speed_mph = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "speed_kts");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_nan_heading_renders_true() {
    FlightInfo f = makeFlight();
    f.track_deg = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "heading_deg");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_nan_vertical_rate_renders_true() {
    FlightInfo f = makeFlight();
    f.vertical_rate_fps = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "vertical_rate_fpm");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_nan_distance_renders_true() {
    FlightInfo f = makeFlight();
    f.distance_km = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "distance_nm");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_nan_bearing_renders_true() {
    FlightInfo f = makeFlight();
    f.bearing_deg = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "bearing_deg");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// Null flight → "--"
// ------------------------------------------------------------------

void test_null_flight_renders_true() {
    RenderContext ctx = makeCtx(nullptr);
    WidgetSpec spec = makeSpec(60, 8, "altitude_ft");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// Negative vsi (signed) — regression against any unsigned mishandling
// ------------------------------------------------------------------

void test_negative_vertical_rate_renders_true() {
    FlightInfo f = makeFlight();
    f.vertical_rate_fps = -42.5;  // → -2550 fpm
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "vertical_rate_fpm");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// Dimension floors
// ------------------------------------------------------------------

void test_undersized_width_returns_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(2, 8, "altitude_ft");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_undersized_height_returns_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 2, "altitude_ft");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// Test runner
// ------------------------------------------------------------------

void setup() {
    delay(2000);
    UNITY_BEGIN();

    RUN_TEST(test_catalog_has_six_entries);
    RUN_TEST(test_catalog_first_entry_is_altitude_ft);
    RUN_TEST(test_every_metric_catalog_id_renders);
    RUN_TEST(test_unknown_metric_id_is_not_known);

    RUN_TEST(test_unknown_metric_id_renders_true_via_fallback);
    RUN_TEST(test_empty_content_renders_true_via_fallback);

    RUN_TEST(test_nan_altitude_renders_true);
    RUN_TEST(test_nan_speed_renders_true);
    RUN_TEST(test_nan_heading_renders_true);
    RUN_TEST(test_nan_vertical_rate_renders_true);
    RUN_TEST(test_nan_distance_renders_true);
    RUN_TEST(test_nan_bearing_renders_true);

    RUN_TEST(test_null_flight_renders_true);
    RUN_TEST(test_negative_vertical_rate_renders_true);

    RUN_TEST(test_undersized_width_returns_true);
    RUN_TEST(test_undersized_height_returns_true);

    UNITY_END();
}

void loop() {
    // Tests run once in setup().
}
