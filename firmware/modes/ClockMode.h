#pragma once
/*
Purpose: ClockMode — large-format time display on LED matrix via DisplayMode.
Responsibilities:
- Render current local time in large digits using full matrix area.
- Support 24-hour (default) and 12-hour format via per-mode NVS setting.
- Show "--:--" fallback when NTP is not synced.
- Update display only when second changes (no flicker).
- Never call FastLED.show() — frame commit is the pipeline's responsibility.
Architecture: Story dl-1.1; Mode ID: "clock". NVS key: m_clock_format (13 chars).
References: interfaces/DisplayMode.h, core/ConfigManager.h (getModeSetting/setModeSetting)
*/

#include "interfaces/DisplayMode.h"

// Static schema declaration (AC #9, rule 29): settings defined in header
static const ModeSettingDef CLOCK_SETTINGS[] = {
    { "format", "Time Format", "enum", 0, 0, 1, "24-hour,12-hour" }
};

static const ModeSettingsSchema CLOCK_SCHEMA = {
    "clock",         // modeAbbrev (<=5 chars, NVS key prefix)
    CLOCK_SETTINGS,
    1
};

class ClockMode : public DisplayMode {
public:
    // Heap budget — vtable ptr (4) + _lastRenderedSecond (1) + _format (1) = ~6 bytes
    // Padded to 64 to cover alignment.
    static constexpr uint32_t MEMORY_REQUIREMENT = 64;

    bool init(const RenderContext& ctx) override;
    void render(const RenderContext& ctx,
                const std::vector<FlightInfo>& flights) override;
    void teardown() override;

    const char* getName() const override;
    const ModeZoneDescriptor& getZoneDescriptor() const override;
    const ModeSettingsSchema* getSettingsSchema() const override;

private:
    int8_t _lastRenderedSecond = -1;  // -1 = never rendered yet
    uint8_t _format = 0;              // 0 = 24h, 1 = 12h

    // Render the time string centered on the matrix using large text
    void renderTime(const RenderContext& ctx, const char* timeStr);
    // Render fallback when NTP not synced
    void renderFallback(const RenderContext& ctx);

    static const ZoneRegion _zones[];

public:
    static const ModeZoneDescriptor _descriptor;
};
