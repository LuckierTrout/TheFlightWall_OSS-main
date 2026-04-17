/*
Purpose: Unity tests for LogoWidget — real RGB565 pipeline (Story LE-1.5).

Covered acceptance criteria:
  AC #1 — renderLogo() calls LogoManager::loadLogo(spec.content, ctx.logoBuffer)
          and blits via DisplayUtils::drawBitmapRGB565 (never fillRect).
  AC #2 — Missing logo_id falls back to the PROGMEM airplane sprite; no crash,
          no heap allocation.
  AC #3 — Null ctx.logoBuffer returns true immediately without touching the
          framebuffer.
  AC #5 — Undersized spec (spec.w < 8 || spec.h < 8) returns true immediately.
  AC #7 — All of the above executed as on-device Unity tests.

Environment: esp32dev (on-device Unity — requires LittleFS for the fixture
test). Mirrors the test_logo_manager scaffold.

Run with:
    pio test -e esp32dev --filter test_logo_widget
*/
#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include <cstring>

#include "core/LogoManager.h"
#include "core/WidgetRegistry.h"
#include "interfaces/DisplayMode.h"
#include "widgets/LogoWidget.h"

// ------------------------------------------------------------------
// Test helpers — cleanup, fixture writers, spec/ctx builders
// ------------------------------------------------------------------

static void cleanLogosDir() {
    if (LittleFS.exists("/logos")) {
        File dir = LittleFS.open("/logos");
        if (dir && dir.isDirectory()) {
            File entry = dir.openNextFile();
            while (entry) {
                String path = entry.name();
                if (!path.startsWith("/")) {
                    path = String("/logos/") + path;
                }
                entry.close();
                LittleFS.remove(path);
                entry = dir.openNextFile();
            }
        }
        LittleFS.rmdir("/logos");
    }
}

// Write a raw 2048-byte logo fixture with an identifiable byte pattern
// (so test_logo_renders_from_littlefs can verify exact buffer contents).
static void writeTestLogoFixture(const char* icao) {
    String path = String("/logos/") + icao + ".bin";
    File f = LittleFS.open(path, "w");
    TEST_ASSERT_TRUE_MESSAGE(f, "Failed to open fixture file for write");

    uint8_t data[LOGO_BUFFER_BYTES];
    for (size_t i = 0; i < LOGO_BUFFER_BYTES; i++) {
        // Distinctive pattern: (i + 0x37) & 0xFF — not 0x00, not 0xAA,
        // so we can distinguish loaded data from sentinel-initialized
        // buffers and the fallback sprite (which is mostly 0x0000/0xFFFF).
        data[i] = (uint8_t)((i + 0x37) & 0xFF);
    }
    TEST_ASSERT_EQUAL(LOGO_BUFFER_BYTES, f.write(data, LOGO_BUFFER_BYTES));
    f.close();
}

// Build a minimal-but-valid WidgetSpec for logo rendering.
static WidgetSpec makeLogoSpec(uint16_t w, uint16_t h, const char* content) {
    WidgetSpec s{};
    s.type = WidgetType::Logo;
    s.x = 0;
    s.y = 0;
    s.w = w;
    s.h = h;
    s.color = 0xFFFF;
    strncpy(s.id, "wL", sizeof(s.id) - 1);
    strncpy(s.content, content, sizeof(s.content) - 1);
    s.align = 0;
    return s;
}

// Build a RenderContext with a caller-supplied logoBuffer and null matrix
// (hardware-free test path — drawBitmapRGB565 skipped, LogoManager::loadLogo
// still populates the buffer per renderLogo() call ordering).
static RenderContext makeCtx(uint16_t* logoBuffer) {
    RenderContext ctx{};
    ctx.matrix = nullptr;
    ctx.textColor = 0xFFFF;
    ctx.brightness = 128;
    ctx.logoBuffer = logoBuffer;
    ctx.displayCycleMs = 0;
    return ctx;
}

// ------------------------------------------------------------------
// AC #1 — logo renders from LittleFS; buffer contents match fixture
// ------------------------------------------------------------------
void test_logo_renders_from_littlefs() {
    cleanLogosDir();
    TEST_ASSERT_TRUE(LogoManager::init());

    writeTestLogoFixture("TST");

    // Reference fixture bytes — must match writeTestLogoFixture() pattern.
    uint8_t expected[LOGO_BUFFER_BYTES];
    for (size_t i = 0; i < LOGO_BUFFER_BYTES; i++) {
        expected[i] = (uint8_t)((i + 0x37) & 0xFF);
    }

    static uint16_t logoBuffer[LOGO_PIXEL_COUNT];
    memset(logoBuffer, 0, LOGO_BUFFER_BYTES);

    WidgetSpec spec = makeLogoSpec(16, 16, "TST");
    RenderContext ctx = makeCtx(logoBuffer);

    bool rc = renderLogo(spec, ctx);
    TEST_ASSERT_TRUE_MESSAGE(rc, "renderLogo must return true on success");

    // AC #1: ctx.logoBuffer should contain the fixture bytes after render.
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
        expected, logoBuffer, LOGO_BUFFER_BYTES,
        "ctx.logoBuffer does not match TST.bin fixture contents");

    // Cleanup fixture so it does not pollute subsequent tests.
    LittleFS.remove("/logos/TST.bin");
}

// ------------------------------------------------------------------
// AC #2 — missing logo_id renders fallback sprite; no crash
// ------------------------------------------------------------------
void test_missing_logo_uses_fallback() {
    cleanLogosDir();
    TEST_ASSERT_TRUE(LogoManager::init());

    static uint16_t logoBuffer[LOGO_PIXEL_COUNT];
    // Seed buffer with a sentinel so we can prove the fallback overwrote it.
    memset(logoBuffer, 0x5A, LOGO_BUFFER_BYTES);

    WidgetSpec spec = makeLogoSpec(16, 16, "MISSING");
    RenderContext ctx = makeCtx(logoBuffer);

    bool rc = renderLogo(spec, ctx);
    TEST_ASSERT_TRUE_MESSAGE(rc, "renderLogo must return true even on miss");

    // Fallback sprite must have overwritten at least part of the sentinel.
    // It is mostly 0x0000 with 0xFFFF silhouette pixels — neither 0x5A5A.
    bool allSentinel = true;
    for (size_t i = 0; i < LOGO_PIXEL_COUNT; i++) {
        if (logoBuffer[i] != 0x5A5A) {
            allSentinel = false;
            break;
        }
    }
    TEST_ASSERT_FALSE_MESSAGE(allSentinel,
        "Fallback sprite was not written to ctx.logoBuffer on missing logo");
}

// ------------------------------------------------------------------
// AC #3 — null ctx.logoBuffer returns true without fault
// ------------------------------------------------------------------
void test_null_logobuffer_returns_true() {
    WidgetSpec spec = makeLogoSpec(16, 16, "ANY");
    RenderContext ctx = makeCtx(nullptr);  // no buffer

    // Must not crash; must return true; must not call loadLogo (can't verify
    // non-call without mocks, but LogoManager::loadLogo guards against a null
    // buffer anyway — the guard here is belt-and-braces for the render path).
    bool rc = renderLogo(spec, ctx);
    TEST_ASSERT_TRUE_MESSAGE(rc, "renderLogo must return true with null buffer");
}

// ------------------------------------------------------------------
// AC #5 — undersized spec (w<8 || h<8) returns true without fault
// ------------------------------------------------------------------
void test_undersized_spec_returns_true() {
    static uint16_t logoBuffer[LOGO_PIXEL_COUNT];
    memset(logoBuffer, 0xC3, LOGO_BUFFER_BYTES);  // sentinel
    const uint16_t sentinelWord = 0xC3C3;

    // w below floor
    {
        WidgetSpec spec = makeLogoSpec(4, 16, "TST");
        RenderContext ctx = makeCtx(logoBuffer);
        bool rc = renderLogo(spec, ctx);
        TEST_ASSERT_TRUE_MESSAGE(rc, "renderLogo must return true for w<8");
        // Buffer must be untouched — dimension floor returns BEFORE loadLogo.
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(sentinelWord, logoBuffer[0],
            "Buffer should not be touched when spec.w < 8");
    }

    // h below floor
    {
        WidgetSpec spec = makeLogoSpec(16, 4, "TST");
        RenderContext ctx = makeCtx(logoBuffer);
        bool rc = renderLogo(spec, ctx);
        TEST_ASSERT_TRUE_MESSAGE(rc, "renderLogo must return true for h<8");
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(sentinelWord, logoBuffer[0],
            "Buffer should not be touched when spec.h < 8");
    }

    // Both below floor
    {
        WidgetSpec spec = makeLogoSpec(0, 0, "TST");
        RenderContext ctx = makeCtx(logoBuffer);
        bool rc = renderLogo(spec, ctx);
        TEST_ASSERT_TRUE_MESSAGE(rc, "renderLogo must return true for w=h=0");
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(sentinelWord, logoBuffer[0],
            "Buffer should not be touched when both dimensions are 0");
    }
}

// ------------------------------------------------------------------
// AC #3 (companion) — null matrix returns true (guard order proves buffer
// is populated BEFORE the matrix guard — test contract, Dev Notes).
// ------------------------------------------------------------------
void test_null_matrix_returns_true_and_loads_buffer() {
    cleanLogosDir();
    TEST_ASSERT_TRUE(LogoManager::init());

    writeTestLogoFixture("TST");

    static uint16_t logoBuffer[LOGO_PIXEL_COUNT];
    memset(logoBuffer, 0, LOGO_BUFFER_BYTES);

    WidgetSpec spec = makeLogoSpec(16, 16, "TST");
    RenderContext ctx = makeCtx(logoBuffer);   // matrix is already nullptr

    bool rc = renderLogo(spec, ctx);
    TEST_ASSERT_TRUE_MESSAGE(rc, "renderLogo must return true with null matrix");

    // Buffer should contain fixture data — loadLogo runs BEFORE matrix guard.
    uint8_t expected[LOGO_BUFFER_BYTES];
    for (size_t i = 0; i < LOGO_BUFFER_BYTES; i++) {
        expected[i] = (uint8_t)((i + 0x37) & 0xFF);
    }
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
        expected, logoBuffer, LOGO_BUFFER_BYTES,
        "ctx.logoBuffer must be populated before matrix guard returns");

    LittleFS.remove("/logos/TST.bin");
}

// ------------------------------------------------------------------
// Test runner
// ------------------------------------------------------------------
void setup() {
    delay(2000);

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed — aborting tests");
        return;
    }

    UNITY_BEGIN();

    RUN_TEST(test_logo_renders_from_littlefs);
    RUN_TEST(test_missing_logo_uses_fallback);
    RUN_TEST(test_null_logobuffer_returns_true);
    RUN_TEST(test_undersized_spec_returns_true);
    RUN_TEST(test_null_matrix_returns_true_and_loads_buffer);

    cleanLogosDir();

    UNITY_END();
}

void loop() {
    // Tests run once in setup().
}
