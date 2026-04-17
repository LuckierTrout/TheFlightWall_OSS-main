#pragma once
/*
Purpose: LiveFlightCardMode — enriched telemetry flight card display via DisplayMode.
Responsibilities:
- Render logo, airline/route, and enriched telemetry (alt, spd, hdg, vrate) in LayoutEngine zones.
- Two-line telemetry layout: line 1 = alt + spd, line 2 = trk + vrate (baseline, ds-2.1).
- Compact single-line fallback for narrow telemetry zones.
- Adaptive field dropping (ds-2.2): omit lowest-priority fields when zone too small.
  Drop order: vrate numeric → heading → ground speed → altitude → route → airline (never drop).
- Vertical trend indicator (ds-2.2): 5×5 glyph at telemetry zone leading column.
  Green up-arrow (climbing), red down-arrow (descending), amber bar (level), none (unknown).
- Cycle through multiple flights using ctx.displayCycleMs.
- Show idle/loading screen when no flights available.
- Never call FastLED.show() — frame commit is the pipeline's responsibility.
Architecture: Story ds-2.1 baseline + ds-2.2 adaptive layout + trend glyph. Mode ID: "live_flight".
*/

#include "interfaces/DisplayMode.h"

// Stack usage estimate for render() call tree (static analysis, AC#10 / NFR S3):
//   render()              :  ~0  (no large locals; calls helpers sequentially)
//   renderZoneFlight()    :  ~0  (no locals beyond pointers)
//   renderFlightZone()    : ~216 bytes (route[72] + line1[72] + line2[72])
//   renderTelemetryZone() : ~280 bytes (telLine1[72] + telLine2[72] + combined[72]
//                                       + 4x value[16]=64 bytes; combined reused for both lines)
//   renderSingleFlightCard: ~360 bytes (route[72] + alt/spd/trk/vr[16x4=64] + telStr[72]
//                                       + line1/2/3[72x3=216])
//   renderLoadingScreen() :  ~0  (no large locals)
// Maximum stack depth: ~360 bytes — well under the 512-byte budget (NFR S3 / AC#10).

// ds-2.2: Vertical trend classification type (declared at file scope for testability).
enum class VerticalTrend { UNKNOWN, CLIMBING, LEVEL, DESCENDING };

// ds-2.2: Telemetry field selection result from priority-drop computation.
struct TelemetryFieldSet { bool alt; bool spd; bool hdg; bool vrate; };

class LiveFlightCardMode : public DisplayMode {
public:
    // Heap budget for mode instance — vtable + member fields, no heap allocations in render().
    // sizeof: vtable ptr (4) + _currentFlightIndex (4) + _lastCycleMs (4) = ~12 bytes
    // Padded to 96; test_memory_requirement_is_reasonable verifies sizeof <= MEMORY_REQUIREMENT.
    // ds-2.2 static constexpr members and glyph patterns add zero instance size (.rodata only).
    static constexpr uint32_t MEMORY_REQUIREMENT = 96;

    // ds-2.2: Epsilon threshold for level-flight classification.
    // 0.5 fps ≈ 30 fpm — below typical ADS-B vertical rate noise floor.
    // Documented choice: matches common ADS-B jitter tolerance in flight display software.
    static constexpr float VRATE_LEVEL_EPS = 0.5f;

    // ds-2.2: Pure helpers exposed as public static methods for unit testing.
    // No hardware dependencies — safe to call in any test environment.
    static VerticalTrend classifyVerticalTrend(float vrate_fps);
    static TelemetryFieldSet computeTelemetryFields(int linesAvailable, int textCols);

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

#ifdef PIO_UNIT_TESTING
    // Test-only accessors for cycling state verification (mirrors ClassicCardMode)
    size_t getCurrentFlightIndex() const { return _currentFlightIndex; }
    unsigned long getLastCycleMs() const { return _lastCycleMs; }
#endif
};
