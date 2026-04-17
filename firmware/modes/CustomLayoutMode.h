#pragma once
/*
Purpose: Production CustomLayoutMode — a 5th peer DisplayMode that renders a
         user-authored layout loaded from LittleFS via LayoutStore.

Story le-1.3 promoted this mode from V0 spike code to a production-quality
peer mode. Parsing happens exactly once at init() time (scoped JsonDocument
is freed before init() returns). render() dispatches each parsed WidgetSpec
via WidgetRegistry::dispatch(), performing zero heap allocations per frame.

References:
  - firmware/core/LayoutStore.h — JSON storage + active-id NVS key
  - firmware/core/WidgetRegistry.h — WidgetSpec / WidgetType / dispatch()
  - _bmad-output/implementation-artifacts/stories/le-1-3-custom-layout-mode-production.md
*/

#include "interfaces/DisplayMode.h"
#include "core/WidgetRegistry.h"

class CustomLayoutMode : public DisplayMode {
public:
    // Layout cap (see Dev Notes / le-1.3 story §Memory budget).
    static constexpr size_t MAX_WIDGETS = 24;

    // Heap budget:
    //   WidgetSpec array on mode object : 24 × 80 = 1920 B
    //   Transient JsonDocument in init(): ~512–2048 B (freed before render)
    //   Mode object scalars + vtable    : ~128 B
    //   Safety margin                   : ~512 B
    // Rounded up to 4 KiB; ModeRegistry applies a further 30 KB free-heap
    // guard on top of MEMORY_REQUIREMENT when deciding whether to allow a
    // switch — the combined budget covers the JsonDocument transient peak.
    static constexpr uint32_t MEMORY_REQUIREMENT = 4096;

    bool init(const RenderContext& ctx) override;
    void render(const RenderContext& ctx,
                const std::vector<FlightInfo>& flights) override;
    void teardown() override;

    const char* getName() const override;
    const ModeZoneDescriptor& getZoneDescriptor() const override;
    const ModeSettingsSchema* getSettingsSchema() const override;

    static const ModeZoneDescriptor _descriptor;

#ifdef PIO_UNIT_TESTING
    // Test-only introspection hooks. Gated so production binaries never ship
    // widget-inspection helpers. Mirrors ClockWidgetTest gating pattern.
    size_t testWidgetCount() const { return _widgetCount; }
    bool   testInvalid()    const { return _invalid; }
    void   testForceInvalid(bool v) { _invalid = v; }
    const WidgetSpec& testWidgetAt(size_t i) const { return _widgets[i]; }
#endif

private:
    // Production widget specs — populated in init() from the JSON that
    // LayoutStore returns. Capped at MAX_WIDGETS regardless of what the
    // layout JSON declares (surplus entries are skipped and logged).
    WidgetSpec _widgets[MAX_WIDGETS];
    size_t     _widgetCount = 0;

    // Parse-error sentinel — set to true when init() receives malformed JSON.
    // render() draws a visible error indicator while this flag is set.
    bool _invalid = false;

    // Parse the JSON string into _widgets[]. Allocates a scoped JsonDocument
    // on the stack (freed before the function returns). Returns false on
    // DeserializationError.
    bool parseFromJson(const String& json);
};
