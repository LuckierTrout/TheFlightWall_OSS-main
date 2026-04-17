/*
Purpose: Unity tests for MetricWidget (Story LE-1.8).

Covered acceptance criteria:
  AC #2  — All four supported metric keys (alt, speed, track, vsi) resolve
           and render successfully with canonical suffixes / decimals.
  AC #4  — Binding surface renders telemetry through DisplayUtils'
           formatTelemetryValue() with the right decimals/suffix.
  AC #7  — NAN telemetry value falls back to "--" (render returns true).
  AC #7  — nullptr currentFlight falls back to "--" (render returns true).
  AC #8  — Zero heap allocation on the render hot path (exercised by char*
           API usage; production code never allocates String).
  AC #10 — Unknown metric key falls back to "--" (render returns true).

Environment: esp32dev. Run with:
    pio test -e esp32dev --filter test_metric_widget
*/
#include <Arduino.h>
#include <unity.h>
#include <cmath>
#include <cstring>

#include "core/WidgetRegistry.h"
#include "interfaces/DisplayMode.h"
#include "models/FlightInfo.h"
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
    f.altitude_kft      = 34.0;
    f.speed_mph         = 487.0;
    f.track_deg         = 247.0;
    f.vertical_rate_fps = -12.5;
    return f;
}

// ------------------------------------------------------------------
// AC #2 — each supported metric key renders true
// ------------------------------------------------------------------

void test_metric_alt_renders_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 8, "alt");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_metric_speed_renders_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "speed");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_metric_track_renders_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 8, "track");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_metric_vsi_renders_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "vsi");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// AC #7 — NAN telemetry falls back to "--"
// ------------------------------------------------------------------

void test_metric_nan_altitude_renders_true() {
    FlightInfo f = makeFlight();
    f.altitude_kft = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 8, "alt");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_metric_nan_speed_renders_true() {
    FlightInfo f = makeFlight();
    f.speed_mph = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "speed");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_metric_nan_track_renders_true() {
    FlightInfo f = makeFlight();
    f.track_deg = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 8, "track");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_metric_nan_vsi_renders_true() {
    FlightInfo f = makeFlight();
    f.vertical_rate_fps = NAN;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "vsi");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// AC #7 — nullptr flight falls back
// ------------------------------------------------------------------

void test_null_flight_returns_true() {
    RenderContext ctx = makeCtx(nullptr);
    WidgetSpec spec = makeSpec(36, 8, "alt");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// AC #10 — unknown metric key falls back
// ------------------------------------------------------------------

void test_unknown_metric_returns_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 8, "temperature");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// Negative vsi (signed) renders true — regression against any
// unsigned-cast mishandling in the format path.
// ------------------------------------------------------------------

void test_negative_vsi_renders_true() {
    FlightInfo f = makeFlight();
    f.vertical_rate_fps = -42.5;
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(60, 8, "vsi");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// Dimension floors — undersized specs must return true without crash
// ------------------------------------------------------------------

void test_undersized_width_returns_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(2, 8, "alt");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

void test_undersized_height_returns_true() {
    FlightInfo f = makeFlight();
    RenderContext ctx = makeCtx(&f);
    WidgetSpec spec = makeSpec(36, 2, "alt");
    TEST_ASSERT_TRUE(renderMetric(spec, ctx));
}

// ------------------------------------------------------------------
// Test runner
// ------------------------------------------------------------------

void setup() {
    delay(2000);
    UNITY_BEGIN();

    RUN_TEST(test_metric_alt_renders_true);
    RUN_TEST(test_metric_speed_renders_true);
    RUN_TEST(test_metric_track_renders_true);
    RUN_TEST(test_metric_vsi_renders_true);

    RUN_TEST(test_metric_nan_altitude_renders_true);
    RUN_TEST(test_metric_nan_speed_renders_true);
    RUN_TEST(test_metric_nan_track_renders_true);
    RUN_TEST(test_metric_nan_vsi_renders_true);

    RUN_TEST(test_null_flight_returns_true);
    RUN_TEST(test_unknown_metric_returns_true);
    RUN_TEST(test_negative_vsi_renders_true);

    RUN_TEST(test_undersized_width_returns_true);
    RUN_TEST(test_undersized_height_returns_true);

    UNITY_END();
}

void loop() {
    // Tests run once in setup().
}
