#pragma once
/*
Purpose: HH:MM clock widget render function (Story LE-1.2, AC #5/#7).

The render function caches the formatted string for ~30s so that a 20fps
display path does not call getLocalTime() ~600 times per minute. Minute
resolution is sufficient because the rendered value only changes on the
minute boundary.

V1 KNOWN LIMITATION: The cache (s_clockBuf / s_lastClockUpdateMs) is a
file-scope static shared by ALL ClockWidget instances in a layout. A layout
with two Clock widgets therefore shows the same string and refresh cadence —
two timezones or different formats are not representable in V1. LE-1.7+
should migrate the cache to a per-spec-id or per-format key if multi-clock
layouts are required.
*/

#include "core/WidgetRegistry.h"

bool renderClock(const WidgetSpec& spec, const RenderContext& ctx);

// Test introspection hooks (Story LE-1.2, AC #8).
// Compiled only in PlatformIO test builds (PIO_UNIT_TESTING is defined by
// the Unity test framework when `pio test` is run). Production firmware
// builds never see this namespace so test-only code stays out of the binary.
// Callers outside of the test harness must not depend on these.
#ifdef PIO_UNIT_TESTING
namespace ClockWidgetTest {
    // Resets the cache so the next renderClock() call will refresh.
    void resetCacheForTest();

    // Count of successful getLocalTime() calls observed by renderClock().
    // Incremented only when a refresh actually occurs (cache miss).
    uint32_t getTimeCallCount();

    // Reset the call counter to zero.
    void resetTimeCallCount();
}
#endif  // PIO_UNIT_TESTING
