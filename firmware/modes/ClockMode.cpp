/*
Purpose: ClockMode implementation — large-format time display on LED matrix.
Story dl-1.1: Renders current local time using Adafruit GFX text APIs.
- 24h (default) or 12h format via ConfigManager::getModeSetting("clock", "format", 0).
- Updates only when second changes to avoid flicker (AC #2).
- Shows "--:--" fallback when NTP is not synced (AC #7).
- Never calls FastLED.show() — frame commit is pipeline responsibility.
*/

#include "modes/ClockMode.h"
#include "core/ConfigManager.h"
#include "utils/DisplayUtils.h"
#include <ctime>

// NTP sync accessor (defined in main.cpp, Story fn-2.1)
extern bool isNtpSynced();

// Settings schema definitions (moved from header to avoid ODR violations)
const ModeSettingDef CLOCK_SETTINGS[] = {
    { "format", "Time Format", "enum", 0, 0, 1, "24-hour,12-hour" }
};

const ModeSettingsSchema CLOCK_SCHEMA = {
    "clock",         // modeAbbrev (<=5 chars, NVS key prefix)
    CLOCK_SETTINGS,
    1
};

// Zone descriptor for Mode Picker UI — single full-matrix "Time" region
const ZoneRegion ClockMode::_zones[] = {
    {"Time", 0, 0, 100, 100}
};

const ModeZoneDescriptor ClockMode::_descriptor = {
    "Large-format clock showing current local time",
    _zones,
    1
};

// Default GFX font character dimensions (5x7 + 1px spacing)
static constexpr int CHAR_WIDTH = 6;
static constexpr int CHAR_HEIGHT = 8;

// Scale factor for large text — use setTextSize() for readability at distance
// Scale 2 = 12x16 per char; Scale 3 = 18x24 per char
static constexpr uint8_t TEXT_SIZE_LARGE = 2;
static constexpr int LARGE_CHAR_WIDTH = CHAR_WIDTH * TEXT_SIZE_LARGE;
static constexpr int LARGE_CHAR_HEIGHT = CHAR_HEIGHT * TEXT_SIZE_LARGE;

bool ClockMode::init(const RenderContext& ctx) {
    (void)ctx;
    _lastRenderedSecond = -1;
    _format = (uint8_t)ConfigManager::getModeSetting("clock", "format", 0);
    return true;
}

void ClockMode::teardown() {
    _lastRenderedSecond = -1;
}

void ClockMode::render(const RenderContext& ctx,
                        const std::vector<FlightInfo>& flights) {
    (void)flights;
    if (ctx.matrix == nullptr) return;

    // Check NTP sync (AC #7)
    if (!isNtpSynced()) {
        // Only redraw fallback once (or on first frame)
        if (_lastRenderedSecond != -2) {
            renderFallback(ctx);
            _lastRenderedSecond = -2;  // Sentinel: fallback displayed
        }
        return;
    }

    // Get current local time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        // Time not yet available — show fallback
        if (_lastRenderedSecond != -2) {
            renderFallback(ctx);
            _lastRenderedSecond = -2;
        }
        return;
    }

    // AC #2: Only update when second changes (avoid flicker)
    if (timeinfo.tm_sec == _lastRenderedSecond) {
        return;
    }
    _lastRenderedSecond = (int8_t)timeinfo.tm_sec;

    // Build time string
    char timeStr[16];
    if (_format == 1) {
        // 12-hour format with AM/PM (AC #5)
        int hour12 = timeinfo.tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* ampm = (timeinfo.tm_hour < 12) ? "AM" : "PM";
        snprintf(timeStr, sizeof(timeStr), "%2d:%02d%s", hour12, timeinfo.tm_min, ampm);
    } else {
        // 24-hour format (AC #4)
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    }

    // Render time. We still update only on second changes, but each fresh clock
    // frame must clear the whole matrix so stale pixels from the previous mode
    // do not survive underneath the time digits.
    renderTime(ctx, timeStr);
}

const char* ClockMode::getName() const {
    return "clock";
}

const ModeZoneDescriptor& ClockMode::getZoneDescriptor() const {
    return _descriptor;
}

const ModeSettingsSchema* ClockMode::getSettingsSchema() const {
    return &CLOCK_SCHEMA;
}

void ClockMode::renderTime(const RenderContext& ctx, const char* timeStr) {
    uint16_t mw = ctx.layout.matrixWidth;
    uint16_t mh = ctx.layout.matrixHeight;
    if (mw == 0) mw = ctx.matrix->width();
    if (mh == 0) mh = ctx.matrix->height();

    // Clock mode owns the full canvas. Clear once per refresh so switching in
    // from card-based modes does not leave prior flight pixels around the time.
    ctx.matrix->fillScreen(0);

    // Calculate optimal text size to fill the matrix
    int strLen = (int)strlen(timeStr);
    uint8_t textSize = 1;

    // Find largest text size that fits
    for (uint8_t s = 4; s >= 1; s--) {
        int tw = strLen * CHAR_WIDTH * s;
        int th = CHAR_HEIGHT * s;
        if (tw <= mw && th <= mh) {
            textSize = s;
            break;
        }
    }

    int charW = CHAR_WIDTH * textSize;
    int charH = CHAR_HEIGHT * textSize;
    int totalW = strLen * charW;

    // Center on matrix
    int16_t x = (mw - totalW) / 2;
    int16_t y = (mh - charH) / 2;

    ctx.matrix->setTextSize(textSize);
    // Draw text with black background to overwrite previous pixels (AC #2: no flicker)
    ctx.matrix->setTextColor(ctx.textColor, 0);
    ctx.matrix->setTextWrap(false);
    ctx.matrix->setCursor(x, y);
    ctx.matrix->print(timeStr);
    ctx.matrix->setTextSize(1);  // Reset to default
}

void ClockMode::renderFallback(const RenderContext& ctx) {
    // AC #7: Non-blank fallback when NTP not synced — show "--:--"
    renderTime(ctx, "--:--");
}
