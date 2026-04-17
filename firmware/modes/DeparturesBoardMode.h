#pragma once
/*
Purpose: DeparturesBoardMode — multi-row departures list on LED matrix via DisplayMode.
Responsibilities:
- Render up to maxRows flights simultaneously in stacked horizontal row bands.
- Each row shows callsign (ICAO ident) + altitude (kft) + speed (mph).
- maxRows from ConfigManager::getModeSetting("depbd", "rows", 4), clamped 1-4.
- Show "NO FLIGHTS" centered when flight vector is empty (AC #7).
- Never call FastLED.show() — frame commit is the pipeline's responsibility.
- Row diff: only repaint dirty rows to avoid full fillScreen per frame (dl-2.2 AC #4).
- No heap alloc per frame: char[] buffers, no String temporaries in hot path (dl-2.2 AC #5).
Architecture: Story dl-2.1/dl-2.2; Mode ID: "departures_board", modeAbbrev: "depbd".
NVS key: m_depbd_rows (12 chars, within 15-char limit).
References: interfaces/DisplayMode.h, core/ConfigManager.h (getModeSetting/setModeSetting)
*/

#include "interfaces/DisplayMode.h"

// Static schema declaration (rule 29): settings defined in header
static const ModeSettingDef DEPBD_SETTINGS[] = {
    { "rows", "Max Rows", "uint8", 4, 1, 4, nullptr }
};

static const ModeSettingsSchema DEPBD_SCHEMA = {
    "depbd",         // modeAbbrev (<=5 chars, NVS key prefix)
    DEPBD_SETTINGS,
    1
};

class DeparturesBoardMode : public DisplayMode {
public:
    // Heap budget — vtable ptr (4) + _maxRows (1) + diff state (~24) = ~29 bytes
    // Padded to 64 to cover alignment.
    static constexpr uint32_t MEMORY_REQUIREMENT = 64;

    static constexpr uint8_t MAX_SUPPORTED_ROWS = 4;

    bool init(const RenderContext& ctx) override;
    void render(const RenderContext& ctx,
                const std::vector<FlightInfo>& flights) override;
    void teardown() override;

    const char* getName() const override;
    const ModeZoneDescriptor& getZoneDescriptor() const override;
    const ModeSettingsSchema* getSettingsSchema() const override;

private:
    uint8_t _maxRows = 4;

    // --- Diff state for selective row repaint (dl-2.2) ---
    bool     _firstFrame = true;         // Full clear on first frame after init/mode switch
    uint8_t  _lastCount  = 0;            // Row count drawn on previous frame
    uint32_t _lastIds[MAX_SUPPORTED_ROWS] = {0, 0, 0, 0};  // Compact ident hash per row slot (explicit zero init)

    // Compute a compact hash for a flight's identity (ident_icao / ident)
    static uint32_t identHash(const FlightInfo& f);

    // Render a single row band for one flight (zero-alloc: uses char[] buffers)
    void renderRow(FastLED_NeoMatrix* matrix, const FlightInfo& f,
                   int16_t y, uint16_t rowHeight, uint16_t matrixWidth,
                   uint16_t textColor);

    // Render empty state when no flights
    void renderEmpty(const RenderContext& ctx);

    static const ZoneRegion _zones[];

public:
    static const ModeZoneDescriptor _descriptor;
};
