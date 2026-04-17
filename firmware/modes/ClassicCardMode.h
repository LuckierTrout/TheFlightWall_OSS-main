#pragma once
/*
Purpose: ClassicCardMode — zone-based classic flight card display via DisplayMode.
Responsibilities:
- Render logo, flight info, and telemetry in LayoutEngine zones via DisplayUtils.
- Cycle through multiple flights using ctx.displayCycleMs.
- Show idle/loading screen when no flights available.
- Never call FastLED.show() — frame commit is the pipeline's responsibility.
Architecture: Story ds-1.4; replaces ds-1.3 stub. Mode ID: "classic_card".
*/

#include "interfaces/DisplayMode.h"

class ClassicCardMode : public DisplayMode {
public:
    // Heap budget for mode instance — vtable + member fields, no heap allocations.
    // sizeof: vtable ptr (4) + _currentFlightIndex (4) + _lastCycleMs (4) = ~12 bytes
    // Padded to 64 to cover alignment and any future additions.
    static constexpr uint32_t MEMORY_REQUIREMENT = 64;

    bool init(const RenderContext& ctx) override;
    void render(const RenderContext& ctx,
                const std::vector<FlightInfo>& flights) override;
    void teardown() override;

    const char* getName() const override;
    const ModeZoneDescriptor& getZoneDescriptor() const override;
    const ModeSettingsSchema* getSettingsSchema() const override;

private:
    // Flight cycling state
    size_t _currentFlightIndex = 0;
    unsigned long _lastCycleMs = 0;

    // Zone rendering helpers (mirror NeoMatrixDisplay zone methods using RenderContext)
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

#ifdef PIO_UNIT_TESTING
    // Test-only accessors for cycling state verification (review item ds-1.4 #2)
    size_t getCurrentFlightIndex() const { return _currentFlightIndex; }
    unsigned long getLastCycleMs() const { return _lastCycleMs; }
#endif
};
