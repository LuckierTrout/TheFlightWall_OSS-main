#pragma once
/*
Purpose: V0 feasibility spike — generic widget renderer that interprets an
         embedded JSON layout (v0_spike_layout.h) and dispatches to per-type
         render functions. Used to measure render time, binary delta, heap
         against fixed-mode baseline (ClassicCardMode).

This mode is ONLY registered when -DLE_V0_SPIKE is passed (see main.cpp
mode-table entry). Main-branch builds do not include it.

Reference: _bmad-output/implementation-artifacts/stories/le-0-1-layout-editor-v0-feasibility-spike.md
*/

#include "interfaces/DisplayMode.h"

class CustomLayoutMode : public DisplayMode {
public:
    // Heap budget: vtable ptr (4) + _widgets fixed array (8 * 56 = 448) + scalars.
    // Pad to 1024 — covers parse-time JsonDocument pressure during init().
    static constexpr uint32_t MEMORY_REQUIREMENT = 1024;

    bool init(const RenderContext& ctx) override;
    void render(const RenderContext& ctx,
                const std::vector<FlightInfo>& flights) override;
    void teardown() override;

    const char* getName() const override;
    const ModeZoneDescriptor& getZoneDescriptor() const override;
    const ModeSettingsSchema* getSettingsSchema() const override;

    static const ModeZoneDescriptor _descriptor;

    enum class WidgetType : uint8_t {
        TEXT = 0,
        CLOCK = 1,
        LOGO_STUB = 2,
        UNKNOWN = 0xFF
    };

private:
    // 56 bytes per instance (keep compact for heap budget):
    //   enum (1) + pad (1) + x(2) + y(2) + w(2) + h(2) + color(2) + text(32) + pad
    struct WidgetInstance {
        WidgetType type;
        int16_t x, y;
        uint16_t w, h;
        uint16_t color;        // RGB565
        char text[32];         // TEXT widget only (clock/logo_stub ignore)
    };

    // Cap widgets for the spike — layout author can't exceed this.
    static constexpr size_t MAX_WIDGETS = 8;

    WidgetInstance _widgets[MAX_WIDGETS];
    size_t _widgetCount = 0;
    bool _parsed = false;

    // Parse the embedded JSON into _widgets[] once at init() time.
    // Returns true on success (even if widget count is 0); false on parse error.
    bool parseLayout();

    // Per-type render fns. Dispatched via switch in render().
    void renderText(const RenderContext& ctx, const WidgetInstance& w);
    void renderClock(const RenderContext& ctx, const WidgetInstance& w);
    void renderLogoStub(const RenderContext& ctx, const WidgetInstance& w);
};
