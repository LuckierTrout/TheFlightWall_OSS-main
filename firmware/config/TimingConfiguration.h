#pragma once

#include <Arduino.h>

namespace TimingConfiguration
{
    // Fetch cadence (seconds). The aggregator worker caches upstream state
    // vectors on its own schedule, so this primarily controls how often the
    // firmware pulls the latest snapshot — 30s keeps the display fresh while
    // bounding per-device egress.
    static const uint32_t FETCH_INTERVAL_SECONDS = 30; // seconds

    // Display cycling configuration
    static const uint32_t DISPLAY_CYCLE_SECONDS = 3; // seconds per flight when multiple flights
}
