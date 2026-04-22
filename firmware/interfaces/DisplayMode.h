#pragma once

#include <vector>
#include <Adafruit_GFX.h>
#include "models/FlightInfo.h"
#include "core/LayoutEngine.h"

// Heap baseline after Foundation boot — measured on hardware after flashing.
// Update these values from serial monitor output (Task 1.3 — not yet measured).

// --- Rendering Context (passed to modes each frame) ---
struct RenderContext {
    // GFX drawing surface — mutable for rendering. Typed as the Adafruit_GFX
    // base class so the adapter layer can plug in either a FastLED_NeoMatrix
    // (legacy WS2812B path, retired), a GFXcanvas16 (HW-1.1 stub), or a
    // MatrixPanel_I2S_DMA (HW-1.2 real HUB75) without modes/widgets caring.
    // Base-class methods (drawPixel, fillRect, setCursor, print, setFont,
    // setTextColor, setTextSize, setTextWrap, width, height) cover all
    // existing draw paths. For RGB565 color packing, call the static
    // DisplayUtils::rgb565(r,g,b) helper instead of FastLED_NeoMatrix::Color.
    Adafruit_GFX* matrix;
    LayoutResult layout;             // zone bounds (logo, flight, telemetry)
    uint16_t textColor;              // pre-computed from DisplayConfig RGB
    uint8_t brightness;              // read-only — managed by night mode scheduler
    uint16_t* logoBuffer;            // shared 2KB buffer from NeoMatrixDisplay
    uint32_t displayCycleMs;         // cycle interval for modes that rotate flights (ms; uint32 avoids overflow for display_cycle > 65s)
    const FlightInfo* currentFlight = nullptr; // LE-1.8: pointer to flight being displayed; nullptr when no active flight
};

// --- Zone Descriptor (static metadata for Mode Picker UI wireframes) ---
struct ZoneRegion {
    const char* label;    // e.g., "Airline", "Route", "Altitude"
    uint8_t relX, relY;   // relative position (0-100%)
    uint8_t relW, relH;   // relative dimensions (0-100%)
};

struct ModeZoneDescriptor {
    const char* description;        // human-readable mode description
    const ZoneRegion* regions;      // static array of labeled regions
    uint8_t regionCount;
};

// --- Per-Mode Settings Schema (Delight Release forward-compat) ---
struct ModeSettingDef {
    const char* key;          // NVS key suffix (<=7 chars)
    const char* label;        // UI display name
    const char* type;         // "uint8", "uint16", "bool", "enum"
    int32_t defaultValue;
    int32_t minValue;
    int32_t maxValue;
    const char* enumOptions;  // comma-separated for enum type, NULL otherwise
};

struct ModeSettingsSchema {
    const char* modeAbbrev;           // <=5 chars, used for NVS key prefix
    const ModeSettingDef* settings;
    uint8_t settingCount;
};

// --- DisplayMode Abstract Interface ---
class DisplayMode {
public:
    virtual ~DisplayMode() = default;

    virtual bool init(const RenderContext& ctx) = 0;
    virtual void render(const RenderContext& ctx,
                        const std::vector<FlightInfo>& flights) = 0;
    virtual void teardown() = 0;

    virtual const char* getName() const = 0;
    virtual const ModeZoneDescriptor& getZoneDescriptor() const = 0;
    virtual const ModeSettingsSchema* getSettingsSchema() const = 0;  // Delight forward-compat; return nullptr if no per-mode settings
};
