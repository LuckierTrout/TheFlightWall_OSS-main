#pragma once
/*
Purpose: Shared time utilities for schedule evaluation (Story dl-4.1, AC #8).
Responsibilities:
- minutesInWindow(): Determine if a minute-of-day is within a time window.
- Same inclusive-start / exclusive-end convention as tickNightScheduler.
- Handles cross-midnight windows (start > end).
Architecture: Header-only utility, no heap allocation.
*/

#include <cstdint>

/// Determine if `nowMin` (0–1439) falls within the window [start, end).
/// Convention: start is inclusive, end is exclusive.
/// Handles cross-midnight windows where start > end (e.g., 23:00–07:00).
/// Zero-length window (start == end) returns false (never in window).
inline bool minutesInWindow(uint16_t nowMin, uint16_t start, uint16_t end) {
    if (start <= end) {
        return (nowMin >= start && nowMin < end);
    } else {
        // Cross-midnight: in window if after start OR before end
        return (nowMin >= start || nowMin < end);
    }
}
