/*
Purpose: Render a 24h HH:MM clock widget with 30s cache (Story LE-1.2, AC #5/#7).

Cache strategy:
  - Clock string lives in a file-scope static buffer, refreshed only when
    millis() rolls over a 30s interval. At 20fps this turns ~600 syscalls
    per minute into 2 — validated pattern from the V0 spike
    (CustomLayoutMode.cpp lines 164-186).
  - s_lastClockUpdateMs is advanced on EVERY refresh attempt (success OR
    failure). This throttles getLocalTime() to ~2 calls/min even when NTP
    has not yet synced, preventing per-frame syscalls (~600/min) on a
    device that has never received an NTP response. The "--:--" placeholder
    is retained in s_clockBuf until a successful read replaces it.
  - Cache state is reset-able via ClockWidgetTest::resetCacheForTest() so
    Unity tests can validate the cache without sleeping.

Rendering:
  - Uses char* overloads of DisplayUtils (zero heap).
  - Enforces min dimensions (w ≥ 6*CHAR_W for "HH:MM", h ≥ CHAR_H) — undersized
    zones are a no-op success, matching AC #7.
*/

#include "widgets/ClockWidget.h"

#include "utils/DisplayUtils.h"

#include <cstdio>
#include <cstring>
#include <time.h>

namespace {
    // Initial placeholder keeps the display readable before NTP has synced.
    char     s_clockBuf[8] = "--:--";
    uint32_t s_lastClockUpdateMs = 0;

    // Test introspection — only incremented on real successful getLocalTime
    // calls so the cache hit path is observable.
    uint32_t s_timeCallCount = 0;
}

bool renderClock(const WidgetSpec& spec, const RenderContext& ctx) {
    // Minimum dimension floor (AC #7): need room for "HH:MM" in 5x7 font.
    if ((int)spec.w < 6 * WIDGET_CHAR_W || (int)spec.h < WIDGET_CHAR_H) {
        return true;  // skip render — not an error
    }

    // Cache refresh window (AC #5).
    uint32_t nowMs = millis();
    if (s_lastClockUpdateMs == 0 || (nowMs - s_lastClockUpdateMs) > 30000) {
        struct tm tinfo;
        // Non-blocking: timeout=0 so the render path never stalls on SNTP.
        if (getLocalTime(&tinfo, 0)) {
            snprintf(s_clockBuf, sizeof(s_clockBuf), "%02d:%02d",
                     tinfo.tm_hour, tinfo.tm_min);
            s_timeCallCount++;
        }
        // Advance the timestamp regardless of success or failure so that
        // failed attempts are also throttled to ~30s. Without this, a
        // missing NTP sync would call getLocalTime() at full frame rate
        // (e.g. 20 Hz) instead of at most once per 30s. The "--:--"
        // placeholder is retained in s_clockBuf until a successful read.
        s_lastClockUpdateMs = nowMs;
    }

    // Hardware-free test path (matches CustomLayoutMode::render nullptr guard).
    if (ctx.matrix == nullptr) return true;

    // Truncate for safety — "HH:MM" is always 5 chars, but the spec might
    // shrink the column count below 5 in adversarial layouts.
    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    if (maxCols <= 0) return true;

    char out[8];
    DisplayUtils::truncateToColumns(s_clockBuf, maxCols, out, sizeof(out));

    int16_t drawY = spec.y;
    if ((int)spec.h > WIDGET_CHAR_H) {
        drawY = spec.y + (int16_t)(((int)spec.h - WIDGET_CHAR_H) / 2);
    }

    DisplayUtils::drawTextLine(ctx.matrix, spec.x, drawY, out, spec.color);
    return true;
}

// ------------------------------------------------------------------
// Test introspection hooks — compiled only in PlatformIO test builds.
// PIO_UNIT_TESTING is set by the Unity test runner (pio test).
// ------------------------------------------------------------------
#ifdef PIO_UNIT_TESTING
namespace ClockWidgetTest {
    void resetCacheForTest() {
        s_lastClockUpdateMs = 0;
        // Reset the buffer so stale values from a prior test cannot bleed
        // through into a cache-hit assertion.
        strcpy(s_clockBuf, "--:--");
    }

    uint32_t getTimeCallCount() {
        return s_timeCallCount;
    }

    void resetTimeCallCount() {
        s_timeCallCount = 0;
    }
}
#endif  // PIO_UNIT_TESTING
