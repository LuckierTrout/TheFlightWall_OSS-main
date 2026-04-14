#pragma once
/*
Purpose: LiveFlightCardMode — enriched telemetry flight card display via DisplayMode.
Responsibilities:
- Render logo, airline/route, and enriched telemetry (alt, spd, hdg, vrate) in LayoutEngine zones.
- Adaptive field dropping when zones are too small for all fields (ds-2.2).
- Vertical trend indicator (climb/descend/level) via accent-colored glyph (ds-2.2).
- Cycle through multiple flights using ctx.displayCycleMs.
- Show idle/loading screen when no flights available.
- Never call FastLED.show() — frame commit is the pipeline's responsibility.
Architecture: Story ds-2.1 base; ds-2.2 adaptive layout + trend indicator. Mode ID: "live_flight".
*/

#include "interfaces/DisplayMode.h"

class LiveFlightCardMode : public DisplayMode {
public:
    // Heap budget for mode instance — vtable + member fields, no heap allocations.
    // sizeof: vtable ptr (4) + _currentFlightIndex (4) + _lastCycleMs (4) = ~12 bytes
    // ds-2.2: No new instance members — adaptive logic + trend indicator use constexpr
    // and anonymous namespace free functions (zero .bss impact).
    // Padded to 96 to cover alignment and match prior test expectations.
    static constexpr uint32_t MEMORY_REQUIREMENT = 96;

    bool init(const RenderContext& ctx) override;
    void render(const RenderContext& ctx,
                const std::vector<FlightInfo>& flights) override;
    void teardown() override;

    const char* getName() const override;
    const ModeZoneDescriptor& getZoneDescriptor() const override;
    const ModeSettingsSchema* getSettingsSchema() const override;

private:
    // Flight cycling state (mirror ClassicCardMode)
    size_t _currentFlightIndex = 0;
    unsigned long _lastCycleMs = 0;

    // Zone rendering helpers
    void renderLogoZone(const RenderContext& ctx, const FlightInfo& f, const Rect& zone);
    void renderFlightZone(const RenderContext& ctx, const FlightInfo& f, const Rect& zone);
    void renderTelemetryZone(const RenderContext& ctx, const FlightInfo& f, const Rect& zone);
    void renderZoneFlight(const RenderContext& ctx, const FlightInfo& f);

    // Fallback: full-screen card when layout is invalid
    void renderSingleFlightCard(const RenderContext& ctx, const FlightInfo& f);

    // Idle display when no flights available
    void renderLoadingScreen(const RenderContext& ctx);

    static const ZoneRegion _zones[];

public:
    static const ModeZoneDescriptor _descriptor;
};
