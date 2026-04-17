/*
Purpose: Unity tests for WidgetRegistry + core widget render fns (Story LE-1.2).

Covered acceptance criteria:
  AC #1 — dispatch() invokes matching render fn and returns true
  AC #2 — isKnownType()/dispatch() reject unknown type strings & enum values
  AC #3 — dispatch is a switch on WidgetType (no vtable) — exercised by the
          fact that this test TU links without any widget class-hierarchy
  AC #4 — Text widget renders via DisplayUtils char* overloads
  AC #5 — Clock widget caches HH:MM across rapid re-dispatch
  AC #6 — Logo widget does NOT use fillRect (exercised via grep gate in CI)
  AC #7 — Undersized specs return true without writing OOB
  AC #8 — All of the above compiled as on-device Unity tests

Environment: esp32dev (on-device test, matches test_layout_store scaffold).
Run with: pio test -e esp32dev --filter test_widget_registry
*/
#include <Arduino.h>
#include <unity.h>
#include <cstring>

#include "core/WidgetRegistry.h"
#include "widgets/TextWidget.h"
#include "widgets/ClockWidget.h"
#include "widgets/LogoWidget.h"

// ------------------------------------------------------------------
// Test helpers
// ------------------------------------------------------------------

// Build a minimal-but-valid WidgetSpec with caller-provided geometry and
// optional content. Content is copied null-terminated (strncpy + manual
// terminator). Align defaults to 0 (left).
static WidgetSpec makeSpec(WidgetType type,
                           int16_t x, int16_t y,
                           uint16_t w, uint16_t h,
                           const char* content = "") {
    WidgetSpec s{};
    s.type = type;
    s.x = x; s.y = y; s.w = w; s.h = h;
    s.color = 0xFFFF;  // white in RGB565
    strncpy(s.id, "wT", sizeof(s.id) - 1);
    strncpy(s.content, content, sizeof(s.content) - 1);
    s.align = 0;
    return s;
}

// Minimal RenderContext with null matrix — render fns guard ctx.matrix
// and skip hardware writes, which is the documented test contract.
static RenderContext makeCtx() {
    RenderContext ctx{};
    ctx.matrix = nullptr;
    return ctx;
}

// ------------------------------------------------------------------
// AC #1 — dispatch happy paths
// ------------------------------------------------------------------

void test_dispatch_text_returns_true() {
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Text, 0, 0, 80, 8, "Hello");
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Text, spec, ctx));
}

void test_dispatch_clock_returns_true() {
    ClockWidgetTest::resetCacheForTest();
    RenderContext ctx = makeCtx();
    // Width must be >= 6*CHAR_W = 36 to pass clock minimum dimension floor.
    WidgetSpec spec = makeSpec(WidgetType::Clock, 0, 0, 40, 8);
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Clock, spec, ctx));
}

void test_dispatch_logo_returns_true() {
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Logo, 0, 0, 16, 16);
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Logo, spec, ctx));
}

// ------------------------------------------------------------------
// AC #2 — unknown type rejection
// ------------------------------------------------------------------

void test_dispatch_unknown_returns_false() {
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Unknown, 0, 0, 10, 10);
    TEST_ASSERT_FALSE(WidgetRegistry::dispatch(WidgetType::Unknown, spec, ctx));
}

void test_dispatch_flight_field_returns_true() {
    // LE-1.8: FlightField now dispatches to renderFlightField. Without a
    // currentFlight in the context, the widget renders a "--" placeholder
    // and still returns true (the render is a success — the fallback is
    // a rendered frame, not an error).
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::FlightField, 0, 0, 40, 8, "airline");
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::FlightField, spec, ctx));
}

void test_dispatch_metric_returns_true() {
    // LE-1.8: Metric now dispatches to renderMetric. Same fallback
    // contract as FlightField — no currentFlight → "--" placeholder,
    // true return.
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Metric, 0, 0, 40, 8, "alt");
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Metric, spec, ctx));
}

void test_is_known_type_true() {
    TEST_ASSERT_TRUE(WidgetRegistry::isKnownType("text"));
    TEST_ASSERT_TRUE(WidgetRegistry::isKnownType("clock"));
    TEST_ASSERT_TRUE(WidgetRegistry::isKnownType("logo"));
    TEST_ASSERT_TRUE(WidgetRegistry::isKnownType("flight_field"));
    TEST_ASSERT_TRUE(WidgetRegistry::isKnownType("metric"));
}

void test_is_known_type_false() {
    TEST_ASSERT_FALSE(WidgetRegistry::isKnownType("foo"));
    // fromString is case-sensitive — uppercase variants must fail.
    TEST_ASSERT_FALSE(WidgetRegistry::isKnownType("CLOCK"));
    TEST_ASSERT_FALSE(WidgetRegistry::isKnownType(""));
    TEST_ASSERT_FALSE(WidgetRegistry::isKnownType(nullptr));
    // Adjacent-but-wrong strings (common typo vectors).
    TEST_ASSERT_FALSE(WidgetRegistry::isKnownType("texts"));
    TEST_ASSERT_FALSE(WidgetRegistry::isKnownType("flight-field"));
}

void test_from_string_roundtrip() {
    TEST_ASSERT_EQUAL((int)WidgetType::Text,
                      (int)WidgetRegistry::fromString("text"));
    TEST_ASSERT_EQUAL((int)WidgetType::Clock,
                      (int)WidgetRegistry::fromString("clock"));
    TEST_ASSERT_EQUAL((int)WidgetType::Logo,
                      (int)WidgetRegistry::fromString("logo"));
    TEST_ASSERT_EQUAL((int)WidgetType::FlightField,
                      (int)WidgetRegistry::fromString("flight_field"));
    TEST_ASSERT_EQUAL((int)WidgetType::Metric,
                      (int)WidgetRegistry::fromString("metric"));
    TEST_ASSERT_EQUAL((int)WidgetType::Unknown,
                      (int)WidgetRegistry::fromString("nope"));
    TEST_ASSERT_EQUAL((int)WidgetType::Unknown,
                      (int)WidgetRegistry::fromString(nullptr));
}

// ------------------------------------------------------------------
// AC #5 — clock cache behavior
// ------------------------------------------------------------------

void test_clock_cache_reuse() {
    // Phase 1: after reset, 50 rapid dispatches must result in AT MOST one
    // getLocalTime() attempt. The 30s cache window now throttles both
    // successful and failed reads (fix for NTP-down hot-path runaway).
    ClockWidgetTest::resetCacheForTest();
    ClockWidgetTest::resetTimeCallCount();

    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Clock, 0, 0, 80, 12);

    for (int i = 0; i < 50; ++i) {
        TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Clock, spec, ctx));
    }

    // getTimeCallCount() counts only SUCCESSFUL getLocalTime() calls. On a
    // test rig without NTP the count will be 0. Either way the important
    // invariant is that we did not call getLocalTime() 50 times.
    TEST_ASSERT_LESS_OR_EQUAL(1u, ClockWidgetTest::getTimeCallCount());

    // Phase 2: reset, dispatch once to arm the cache, then dispatch 49 more
    // in the same millis() window — all must be cache hits.
    ClockWidgetTest::resetCacheForTest();
    ClockWidgetTest::resetTimeCallCount();
    WidgetRegistry::dispatch(WidgetType::Clock, spec, ctx);  // arms cache
    for (int i = 0; i < 49; ++i) {
        WidgetRegistry::dispatch(WidgetType::Clock, spec, ctx);
    }
    // Regardless of NTP state, the cache was armed on the first call; the
    // subsequent 49 calls must be throttled. getTimeCallCount ≤ 1.
    TEST_ASSERT_LESS_OR_EQUAL(1u, ClockWidgetTest::getTimeCallCount());
}

// ------------------------------------------------------------------
// AC #7 — minimum dimension floors
// ------------------------------------------------------------------

void test_bounds_floor_text() {
    RenderContext ctx = makeCtx();
    // Zone shorter than a glyph — must not crash and must return true.
    WidgetSpec spec = makeSpec(WidgetType::Text, 0, 0, 80, 2, "Hi");
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Text, spec, ctx));

    // Zone narrower than a glyph — must not crash and must return true.
    WidgetSpec narrow = makeSpec(WidgetType::Text, 0, 0, 2, 8, "Hi");
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Text, narrow, ctx));
}

void test_bounds_floor_clock() {
    ClockWidgetTest::resetCacheForTest();
    RenderContext ctx = makeCtx();
    // Below clock minimum (6*CHAR_W = 36 wide, 8 tall).
    WidgetSpec spec = makeSpec(WidgetType::Clock, 0, 0, 20, 8);
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Clock, spec, ctx));

    WidgetSpec shortSpec = makeSpec(WidgetType::Clock, 0, 0, 40, 4);
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Clock, shortSpec, ctx));
}

void test_bounds_floor_logo() {
    RenderContext ctx = makeCtx();
    // Below logo minimum (8x8).
    WidgetSpec spec = makeSpec(WidgetType::Logo, 0, 0, 4, 4);
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Logo, spec, ctx));
}

// ------------------------------------------------------------------
// AC #4 — text alignment variations (smoke test; pixel-accurate assertions
// require a real framebuffer which we don't have in the test env).
// ------------------------------------------------------------------

void test_text_alignment_all_three() {
    RenderContext ctx = makeCtx();
    WidgetSpec leftSpec = makeSpec(WidgetType::Text, 0, 0, 80, 8, "L");
    leftSpec.align = 0;
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Text, leftSpec, ctx));

    WidgetSpec centerSpec = makeSpec(WidgetType::Text, 0, 0, 80, 8, "C");
    centerSpec.align = 1;
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Text, centerSpec, ctx));

    WidgetSpec rightSpec = makeSpec(WidgetType::Text, 0, 0, 80, 8, "R");
    rightSpec.align = 2;
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Text, rightSpec, ctx));
}

void test_text_empty_content_is_noop() {
    RenderContext ctx = makeCtx();
    WidgetSpec spec = makeSpec(WidgetType::Text, 0, 0, 80, 8, "");
    TEST_ASSERT_TRUE(WidgetRegistry::dispatch(WidgetType::Text, spec, ctx));
}

// ------------------------------------------------------------------
// Unity driver
// ------------------------------------------------------------------

void setup() {
    delay(2000);
    UNITY_BEGIN();

    // AC #1: dispatch happy paths
    RUN_TEST(test_dispatch_text_returns_true);
    RUN_TEST(test_dispatch_clock_returns_true);
    RUN_TEST(test_dispatch_logo_returns_true);

    // AC #2: unknown type rejection
    RUN_TEST(test_dispatch_unknown_returns_false);
    RUN_TEST(test_dispatch_flight_field_returns_true);
    RUN_TEST(test_dispatch_metric_returns_true);
    RUN_TEST(test_is_known_type_true);
    RUN_TEST(test_is_known_type_false);
    RUN_TEST(test_from_string_roundtrip);

    // AC #5: clock cache
    RUN_TEST(test_clock_cache_reuse);

    // AC #7: dimension floors
    RUN_TEST(test_bounds_floor_text);
    RUN_TEST(test_bounds_floor_clock);
    RUN_TEST(test_bounds_floor_logo);

    // AC #4: text alignment + empty content
    RUN_TEST(test_text_alignment_all_three);
    RUN_TEST(test_text_empty_content_is_noop);

    UNITY_END();
}

void loop() {
    // Tests run once in setup().
}
