/*
Purpose: DeparturesBoardMode implementation — multi-row departures list on LED matrix.
Story dl-2.1 + dl-2.2: Renders up to maxRows flights simultaneously in stacked row bands.
Each row: callsign | altitude(kft) | speed(mph).
- maxRows read from ConfigManager::getModeSetting("depbd", "rows", 4).
- N > maxRows: only first maxRows flights shown (no scrolling — dl-2.1 AC #3).
- N == 0: render "NO FLIGHTS" centered (dl-2.1 AC #7).
- Row diff: only repaint dirty rows using fillRect (dl-2.2 AC #1, #3, #4).
- No new/malloc per frame: char[] buffers, no String growth (dl-2.2 AC #5).
- teardown() resets all diff state (dl-2.2 AC #6).
- Never calls FastLED.show() — frame commit is pipeline responsibility.
*/

#include "modes/DeparturesBoardMode.h"
#include "core/ConfigManager.h"
#include "utils/DisplayUtils.h"
#include <cmath>
#include <cstring>

// Zone descriptor for Mode Picker UI — stacked horizontal row bands
const ZoneRegion DeparturesBoardMode::_zones[] = {
    {"Row 1", 0,  0,  100, 25},
    {"Row 2", 0, 25,  100, 25},
    {"Row 3", 0, 50,  100, 25},
    {"Row 4", 0, 75,  100, 25}
};

const ModeZoneDescriptor DeparturesBoardMode::_descriptor = {
    "Multi-row departures board showing flights in stacked horizontal bands",
    _zones,
    4
};

// Character dimensions for default 5x7 Adafruit GFX font (+1px spacing)
static constexpr int CHAR_WIDTH = 6;
static constexpr int CHAR_HEIGHT = 8;

// Max chars for a single row text buffer
// Worst case: callsign(8) + ' ' + alt(10) + ' ' + spd(8) + '\0' = ~28
static constexpr int ROW_BUF_SIZE = 40;

bool DeparturesBoardMode::init(const RenderContext& ctx) {
    (void)ctx;
    _maxRows = (uint8_t)ConfigManager::getModeSetting("depbd", "rows", 4);
    if (_maxRows < 1) _maxRows = 1;
    if (_maxRows > 4) _maxRows = 4;

    // Reset diff state — force full clear on first render
    _firstFrame = true;
    _lastCount = 0;
    _lastMaxRows = 0;
    memset(_lastIds, 0, sizeof(_lastIds));
    return true;
}

void DeparturesBoardMode::teardown() {
    // D2 teardown discipline: release all non-static resources (dl-2.2 AC #6)
    _maxRows = 4;
    _firstFrame = true;
    _lastCount = 0;
    _lastMaxRows = 0;
    memset(_lastIds, 0, sizeof(_lastIds));
}

// djb2 variant — compact, deterministic, no heap
uint32_t DeparturesBoardMode::identHash(const FlightInfo& f) {
    uint32_t h = 5381;
    const String& s = f.ident_icao.length() ? f.ident_icao
                      : (f.ident.length() ? f.ident : String());
    for (unsigned int i = 0; i < s.length(); i++) {
        h = ((h << 5) + h) + (uint8_t)s[i];
    }
    return h;
}

void DeparturesBoardMode::render(const RenderContext& ctx,
                                  const std::vector<FlightInfo>& flights) {
    if (ctx.matrix == nullptr) return;

    // Handle empty → show "NO FLIGHTS" (full clear always for empty state)
    if (flights.empty()) {
        ctx.matrix->fillScreen(0);
        renderEmpty(ctx);
        // Update diff state
        if (_lastCount != 0) {
            _lastCount = 0;
            memset(_lastIds, 0, sizeof(_lastIds));
        }
        _firstFrame = false;
        return;
    }

    // Re-read maxRows each render in case it changed via API
    uint8_t newMaxRows = (uint8_t)ConfigManager::getModeSetting("depbd", "rows", 4);
    if (newMaxRows < 1) newMaxRows = 1;
    if (newMaxRows > 4) newMaxRows = 4;

    // Determine matrix dimensions
    uint16_t mw = ctx.layout.matrixWidth;
    uint16_t mh = ctx.layout.matrixHeight;
    if (mw == 0) mw = ctx.matrix->width();
    if (mh == 0) mh = ctx.matrix->height();

    // AC #1, #2: R = min(N, maxRows)
    uint8_t rowCount = (uint8_t)(flights.size() < newMaxRows
                                 ? flights.size() : newMaxRows);

    // Determine if full clear needed (dl-2.2 AC #4 exceptions)
    bool fullClear = _firstFrame
                     || (newMaxRows != _lastMaxRows)
                     || (rowCount != _lastCount);

    if (fullClear) {
        ctx.matrix->fillScreen(0);
    }

    _maxRows = newMaxRows;

    // Compute row band height (integer division)
    uint16_t rowHeight = mh / rowCount;

    // Compute current ident hashes
    uint32_t curIds[MAX_SUPPORTED_ROWS] = {};
    for (uint8_t i = 0; i < rowCount; i++) {
        curIds[i] = identHash(flights[i]);
    }

    // Render rows — only dirty ones unless fullClear
    for (uint8_t i = 0; i < rowCount; i++) {
        bool dirty = fullClear || (curIds[i] != _lastIds[i]);
        if (dirty) {
            int16_t y = i * rowHeight;
            // Clear just this row band (dl-2.2 AC #4)
            if (!fullClear) {
                ctx.matrix->fillRect(0, y, mw, rowHeight, 0);
            }
            renderRow(ctx.matrix, flights[i], y, rowHeight, mw, ctx.textColor);
        }
    }

    // Update diff state for next frame
    _firstFrame = false;
    _lastCount = rowCount;
    _lastMaxRows = _maxRows;
    memcpy(_lastIds, curIds, sizeof(_lastIds));
    // Zero out slots beyond current rowCount
    for (uint8_t i = rowCount; i < MAX_SUPPORTED_ROWS; i++) {
        _lastIds[i] = 0;
    }
    // No FastLED.show() — pipeline responsibility
}

const char* DeparturesBoardMode::getName() const {
    return "Departures Board";
}

const ModeZoneDescriptor& DeparturesBoardMode::getZoneDescriptor() const {
    return _descriptor;
}

const ModeSettingsSchema* DeparturesBoardMode::getSettingsSchema() const {
    return &DEPBD_SCHEMA;
}

void DeparturesBoardMode::renderRow(FastLED_NeoMatrix* matrix, const FlightInfo& f,
                                     int16_t y, uint16_t rowHeight, uint16_t matrixWidth,
                                     uint16_t textColor) {
    const int maxCols = matrixWidth / CHAR_WIDTH;
    if (maxCols <= 0) return;

    // Build row text into stack buffer — zero heap alloc (dl-2.2 AC #5)
    char buf[ROW_BUF_SIZE];
    int pos = 0;

    // Pick callsign: prefer ident_icao, fall back to ident (dl-2.1 AC #2)
    const char* cs = nullptr;
    if (f.ident_icao.length()) cs = f.ident_icao.c_str();
    else if (f.ident.length()) cs = f.ident.c_str();
    else cs = "---";

    // Append callsign
    int csLen = (int)strlen(cs);
    int copyLen = csLen < (ROW_BUF_SIZE - 2) ? csLen : (ROW_BUF_SIZE - 2);
    memcpy(buf, cs, copyLen);
    pos = copyLen;
    buf[pos++] = ' ';

    // Format altitude (kft)
    if (isnan(f.altitude_kft)) {
        memcpy(buf + pos, "--", 2);
        pos += 2;
    } else {
        pos += snprintf(buf + pos, ROW_BUF_SIZE - pos, "%.1fkft", f.altitude_kft);
    }

    buf[pos++] = ' ';

    // Format speed (mph)
    if (isnan(f.speed_mph)) {
        memcpy(buf + pos, "--", 2);
        pos += 2;
    } else {
        pos += snprintf(buf + pos, ROW_BUF_SIZE - pos, "%dmph", (int)f.speed_mph);
    }

    buf[pos] = '\0';

    // Truncate to column limit
    if (pos > maxCols) {
        pos = maxCols;
        buf[pos] = '\0';
    }

    // Vertically center text within row band
    int16_t textY = y + (rowHeight - CHAR_HEIGHT) / 2;

    // Draw directly using matrix API — no String allocation
    matrix->setCursor(0, textY);
    matrix->setTextColor(textColor);
    for (int i = 0; i < pos; i++) {
        matrix->write(buf[i]);
    }
}

void DeparturesBoardMode::renderEmpty(const RenderContext& ctx) {
    // AC #7: Show defined empty state, not uninitialized framebuffer garbage
    uint16_t mw = ctx.layout.matrixWidth;
    uint16_t mh = ctx.layout.matrixHeight;
    if (mw == 0) mw = ctx.matrix->width();
    if (mh == 0) mh = ctx.matrix->height();

    const char* emptyText = "NO FLIGHTS";
    int strLen = (int)strlen(emptyText);
    int totalW = strLen * CHAR_WIDTH;

    // Center on matrix
    int16_t x = (mw - totalW) / 2;
    int16_t y = (mh - CHAR_HEIGHT) / 2;

    // Draw directly without String wrapper
    ctx.matrix->setCursor(x, y);
    ctx.matrix->setTextColor(ctx.textColor);
    for (int i = 0; i < strLen; i++) {
        ctx.matrix->write(emptyText[i]);
    }
}
