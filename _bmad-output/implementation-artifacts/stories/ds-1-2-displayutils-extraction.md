# Story ds-1.2: DisplayUtils Extraction

Status: review

## Story

As a firmware developer,
I want shared rendering helper functions extracted from NeoMatrixDisplay into a standalone utility header,
So that both NeoMatrixDisplay and future display mode classes can use them without circular dependencies.

## Acceptance Criteria

1. **Given** NeoMatrixDisplay contains rendering helper functions (`drawTextLine`, `truncateToColumns`, `formatTelemetryValue`, `drawBitmapRGB565`), **When** the extraction is complete, **Then** `utils/DisplayUtils.h` declares these functions as free functions in the `DisplayUtils` namespace (not class methods)

2. **Given** `DisplayUtils.h` is a header and `DisplayUtils.cpp` is the implementation file, **When** the project builds, **Then** `platformio.ini` `build_src_filter` includes `+<../utils/*.cpp>` so the new `.cpp` file compiles

3. **Given** `drawTextLine` and `drawBitmapRGB565` currently access `_matrix` member variable, **When** extracted as free functions, **Then** each receives `Adafruit_NeoMatrix*` as its first parameter (base class pointer, not `FastLED_NeoMatrix*`)

4. **Given** `NeoMatrixDisplay.cpp` currently calls these four methods as `this->method()`, **When** extraction is complete, **Then** NeoMatrixDisplay.cpp includes `utils/DisplayUtils.h` and calls `DisplayUtils::functionName()` instead — all four private method declarations are removed from `NeoMatrixDisplay.h`

5. **Given** all rendering output must remain pixel-identical, **When** the extraction is complete, **Then** all existing rendering (flight cards, telemetry zones, logo zones, calibration, positioning) produces identical output to pre-extraction behavior

6. **Given** the project must compile cleanly, **When** `pio run` executes, **Then** the build succeeds with no new warnings and no existing `#include` dependencies are broken

## Tasks / Subtasks

- [x] Task 1: Create `utils/DisplayUtils.h` header (AC: #1, #3)
  - [x] 1.1: Create file with `#pragma once`, includes for `<FastLED_NeoMatrix.h>` and `<Arduino.h>`
  - [x] 1.2: Declare all four functions in `DisplayUtils` namespace with correct signatures

- [x] Task 2: Create `utils/DisplayUtils.cpp` implementation (AC: #1, #3)
  - [x] 2.1: Create file including `"utils/DisplayUtils.h"` and `<cmath>` (for `isnan` in `formatTelemetryValue`)
  - [x] 2.2: Implement `truncateToColumns` — copied existing logic verbatim
  - [x] 2.3: Implement `formatTelemetryValue` — copied existing logic verbatim
  - [x] 2.4: Implement `drawTextLine` — copied logic, replaced `_matrix->` with `matrix->` parameter
  - [x] 2.5: Implement `drawBitmapRGB565` — copied logic, replaced `_matrix->` with `matrix->` parameter

- [x] Task 3: Update `NeoMatrixDisplay.h` — remove extracted declarations (AC: #4)
  - [x] 3.1: Removed four private method declarations from the class

- [x] Task 4: Update `NeoMatrixDisplay.cpp` — delegate to DisplayUtils (AC: #4, #5)
  - [x] 4.1: Added `#include "utils/DisplayUtils.h"` to includes
  - [x] 4.2: Removed four method implementations (the function bodies)
  - [x] 4.3: Updated all call sites to use `DisplayUtils::` prefix with `_matrix` parameter where needed
  - [x] 4.4: Verified `displaySingleFlightCard()` correctly calls `DisplayUtils::drawTextLine(_matrix, ...)`

- [x] Task 5: Update `platformio.ini` build configuration (AC: #2)
  - [x] 5.1: Added `+<../utils/*.cpp>` to `build_src_filter` section

- [x] Task 6: Build verification (AC: #6)
  - [x] 6.1: `pio run` succeeds — RAM: 16.7%, Flash: 77.9% (1,231,728 / 1,572,864 bytes)
  - [x] 6.2: N/A — no web asset changes

## Dev Notes

### Exact Function Signatures for DisplayUtils

Per architecture Decision D3 and the Shared Rendering Utilities pattern:

```cpp
// utils/DisplayUtils.h
#pragma once
#include <Adafruit_NeoMatrix.h>
#include <Arduino.h>

namespace DisplayUtils {
    void drawTextLine(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                      const String& text, uint16_t color);
    String truncateToColumns(const String& text, int maxColumns);
    String formatTelemetryValue(double value, const char* suffix, int decimals = 0);
    void drawBitmapRGB565(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint16_t* bitmap,
                          uint16_t zoneW, uint16_t zoneH);
}
```

[Source: architecture.md, Shared Rendering Utilities pattern]

### Function Independence Analysis

| Function | Uses `_matrix`? | Extraction strategy |
|---|---|---|
| `truncateToColumns` | No | Copy verbatim — pure string utility |
| `formatTelemetryValue` | No | Copy verbatim — pure value formatter |
| `drawTextLine` | Yes | Add `Adafruit_NeoMatrix* matrix` first param |
| `drawBitmapRGB565` | Yes | Add `Adafruit_NeoMatrix* matrix` first param |

### Existing Implementations to Copy

**`truncateToColumns`** (`NeoMatrixDisplay.cpp:250-257`) — no changes needed:
```cpp
String DisplayUtils::truncateToColumns(const String& text, int maxColumns) {
    if ((int)text.length() <= maxColumns)
        return text;
    if (maxColumns <= 3)
        return text.substring(0, maxColumns);
    return text.substring(0, maxColumns - 3) + String("...");
}
```

**`formatTelemetryValue`** (`NeoMatrixDisplay.cpp:310-319`) — no changes needed:
```cpp
String DisplayUtils::formatTelemetryValue(double value, const char* suffix, int decimals) {
    if (isnan(value)) {
        return String("--");
    }
    if (decimals == 0) {
        return String((int)value) + suffix;
    }
    return String(value, decimals) + suffix;
}
```

**`drawTextLine`** (`NeoMatrixDisplay.cpp:240-248`) — replace `_matrix->` with `matrix->`:
```cpp
void DisplayUtils::drawTextLine(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                                const String& text, uint16_t color) {
    matrix->setCursor(x, y);
    matrix->setTextColor(color);
    for (size_t i = 0; i < (size_t)text.length(); ++i) {
        matrix->write(text[i]);
    }
}
```

**`drawBitmapRGB565`** (`NeoMatrixDisplay.cpp:321-360`) — replace `_matrix->` with `matrix->`:
```cpp
void DisplayUtils::drawBitmapRGB565(Adafruit_NeoMatrix* matrix, int16_t x, int16_t y,
                                     uint16_t w, uint16_t h, const uint16_t* bitmap,
                                     uint16_t zoneW, uint16_t zoneH) {
    int16_t offsetX = 0, offsetY = 0;
    uint16_t drawW = w, drawH = h;
    if (zoneW < w) { offsetX = (w - zoneW) / 2; drawW = zoneW; }
    if (zoneH < h) { offsetY = (h - zoneH) / 2; drawH = zoneH; }
    int16_t destX = x;
    int16_t destY = y;
    if (zoneW > w) { destX = x + (zoneW - w) / 2; }
    if (zoneH > h) { destY = y + (zoneH - h) / 2; }
    for (uint16_t row = 0; row < drawH; row++) {
        for (uint16_t col = 0; col < drawW; col++) {
            uint16_t pixel = bitmap[(row + offsetY) * w + (col + offsetX)];
            if (pixel != 0) {
                uint8_t r = ((pixel >> 11) & 0x1F) << 3;
                uint8_t g = ((pixel >> 5) & 0x3F) << 2;
                uint8_t b = (pixel & 0x1F) << 3;
                matrix->drawPixel(destX + col, destY + row, matrix->Color(r, g, b));
            }
        }
    }
}
```

### Call Site Replacement Map in NeoMatrixDisplay.cpp

Every internal call site needs the `_matrix` member variable passed explicitly. Search for these patterns:

| Original call | Replacement |
|---|---|
| `drawTextLine(x, y, text, color)` | `DisplayUtils::drawTextLine(_matrix, x, y, text, color)` |
| `drawBitmapRGB565(x, y, w, h, buf, zw, zh)` | `DisplayUtils::drawBitmapRGB565(_matrix, x, y, w, h, buf, zw, zh)` |
| `truncateToColumns(text, cols)` | `DisplayUtils::truncateToColumns(text, cols)` |
| `formatTelemetryValue(val, sfx, dec)` | `DisplayUtils::formatTelemetryValue(val, sfx, dec)` |

**Call sites to update** (found in NeoMatrixDisplay.cpp):
- `displaySingleFlightCard()` — calls `drawTextLine`, `truncateToColumns`
- `renderFlightZone()` — calls `drawTextLine`, `truncateToColumns`
- `renderTelemetryZone()` — calls `drawTextLine`, `truncateToColumns`, `formatTelemetryValue`
- `renderLogoZone()` — calls `drawBitmapRGB565`

### platformio.ini Change

Current `build_src_filter`:
```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
```

Add `+<../utils/*.cpp>`:
```ini
build_src_filter =
    +<*.cpp>
    +<**/*.cpp>
    +<../adapters/*.cpp>
    +<../core/*.cpp>
    +<../utils/*.cpp>
```

**Why this is needed:** `GeoUtils.h` is header-only (all `inline` functions), so no `.cpp` was ever needed in `utils/`. `DisplayUtils.cpp` is the first `.cpp` in that directory. Without this line, PlatformIO won't compile it. [Source: firmware/platformio.ini, current build_src_filter]

### What NOT To Do

- **Do NOT** move `makeFlightLine()` to DisplayUtils — it will migrate to `ClassicCardMode` in ds-1.4, not to a shared utility
- **Do NOT** move `displaySingleFlightCard()` — it stays in NeoMatrixDisplay as the emergency fallback renderer (AR8)
- **Do NOT** move `renderZoneFlight()`, `renderFlightZone()`, `renderTelemetryZone()`, `renderLogoZone()` — these migrate to ClassicCardMode in ds-1.4
- **Do NOT** change any function logic — this is a pure mechanical extraction, pixel-identical output required
- **Do NOT** change `#include <FastLED_NeoMatrix.h>` in NeoMatrixDisplay.cpp — it still needs it for the concrete type
- **Do NOT** use `FastLED_NeoMatrix*` in DisplayUtils signatures — use `Adafruit_NeoMatrix*` (the base class) so DisplayUtils has no dependency on FastLED NeoMatrix library specifically [Source: architecture.md, Shared Rendering Utilities]
- **Do NOT** add `#include "core/ConfigManager.h"` to DisplayUtils — utility functions must not access config (Rule 18)

### Existing Code to Reference

| File | Why Reference It | Key Details |
|---|---|---|
| `firmware/adapters/NeoMatrixDisplay.h:61-76` | Private method declarations to remove | Four functions + zone rendering methods that stay |
| `firmware/adapters/NeoMatrixDisplay.cpp:240-360` | Source implementations to extract | `drawTextLine` (240), `truncateToColumns` (250), `formatTelemetryValue` (310), `drawBitmapRGB565` (321) |
| `firmware/adapters/NeoMatrixDisplay.cpp:261-306` | `displaySingleFlightCard` — calls extracted functions | Emergency fallback renderer — update calls but don't move the function |
| `firmware/adapters/NeoMatrixDisplay.cpp:370-485` | `renderFlightZone`/`renderTelemetryZone` — call extracted functions | Zone renderers that will later move to ClassicCardMode — update calls now |
| `firmware/utils/GeoUtils.h` | Existing utility pattern | Header-only with `inline` functions — note DisplayUtils uses `.h`+`.cpp` pattern instead |
| `firmware/platformio.ini:23-28` | Build filter that needs `+<../utils/*.cpp>` | Currently only has adapters/ and core/ |
| `firmware/interfaces/DisplayMode.h` | Created in ds-1.1 | New interface, not affected by this story |

### Previous Story Intelligence (ds-1.1)

The ds-1.1 story discovered that `LOG_I` macro uses compile-time string literal concatenation (`"[" tag "] " msg`), so dynamic log strings must use `Serial.printf` with `#if LOG_LEVEL >= N` guards instead. This is not directly relevant to ds-1.2 (no logging needed) but is a pattern to remember.

ds-1.1 created `firmware/interfaces/DisplayMode.h` with `RenderContext`, `ZoneRegion`, `ModeZoneDescriptor`, `ModeSettingDef`, `ModeSettingsSchema`, and the `DisplayMode` abstract class. This file is not modified by ds-1.2 but modes will include it transitively.

### Enforcement Rules Applicable to This Story

- **Rule 1:** Follow naming conventions — PascalCase classes, camelCase methods, `_camelCase` private members
- **Rule 6:** Log via `LOG_I`/`LOG_E`/`LOG_V` from `utils/Log.h` — N/A for this story (no new logging)
- **Rule 21:** Shared rendering helpers must be called from `utils/DisplayUtils.h` — this story creates that contract
- **Rule 22:** Adding a new mode requires exactly two touch points — N/A yet, but this extraction enables it

### Project Structure Notes

- **New files:** `firmware/utils/DisplayUtils.h`, `firmware/utils/DisplayUtils.cpp`
- **Modified files:** `firmware/adapters/NeoMatrixDisplay.h` (remove 4 declarations), `firmware/adapters/NeoMatrixDisplay.cpp` (remove 4 implementations + update call sites), `firmware/platformio.ini` (add build filter)
- **No directory creation needed** — `firmware/utils/` already exists
- **Include path:** `-I utils` already in `build_flags`, so `#include "utils/DisplayUtils.h"` resolves

### References

- [Source: architecture.md#Decision-D3] — NeoMatrixDisplay Refactoring, Responsibility Split
- [Source: architecture.md#Shared-Rendering-Utilities] — DisplayUtils namespace, function signatures
- [Source: architecture.md#Display-System-Enforcement-Rules-17-23] — Rule 21: shared helpers in DisplayUtils
- [Source: architecture.md#Rendering-Context-Discipline] — DisplayUtils usage pattern from modes
- [Source: architecture.md#Include-Path-Conventions] — Mode files include `"utils/DisplayUtils.h"`
- [Source: architecture.md#File-Structure] — `utils/DisplayUtils.h` and `utils/DisplayUtils.cpp` placement
- [Source: architecture.md#Implementation-Priority] — DisplayUtils extraction is step 4 in critical path
- [Source: epics/epic-ds-1.md#Story-ds-1.2] — Story definition and acceptance criteria
- [Source: epics-display-system.md#AR4] — DisplayUtils extraction requirement
- [Source: firmware/adapters/NeoMatrixDisplay.cpp:240-360] — Source implementations
- [Source: firmware/adapters/NeoMatrixDisplay.h:61-76] — Private method declarations
- [Source: firmware/platformio.ini:23-28] — Current build_src_filter

## Dev Agent Record

### Agent Model Used

Claude Opus 4.6

### Debug Log References

- Build error: `Adafruit_NeoMatrix.h: No such file or directory` when compiling `utils/DisplayUtils.cpp` — PlatformIO LDF could not resolve transitive dependency from `utils/` directory. Fixed by using `FastLED_NeoMatrix*` type instead of `Adafruit_NeoMatrix*` in DisplayUtils signatures. `FastLED_NeoMatrix` inherits from `Adafruit_NeoMatrix` which inherits from `Adafruit_GFX`, so all base class methods (setCursor, setTextColor, write, drawPixel, Color) remain accessible.

### Completion Notes List

1. **AC #3 deviation**: Story specified `Adafruit_NeoMatrix*` as parameter type, but PlatformIO LDF cannot resolve the `Adafruit_NeoMatrix.h` header when compiling files in `utils/` directory (it's a transitive dependency through FastLED NeoMatrix library, not a direct dependency). Changed to `FastLED_NeoMatrix*` which is the concrete type actually used throughout the project. This is functionally equivalent since `FastLED_NeoMatrix` extends `Adafruit_NeoMatrix`.
2. All four functions extracted with pixel-identical behavior — pure mechanical extraction with no logic changes.
3. Two additional call sites found beyond what the story listed: `displayLoadingScreen()` and `displayMessage()` — both updated to use `DisplayUtils::` prefix.

### Change Log

| Date | Author | Changes |
|------|--------|---------|
| 2026-04-13 | BMad | Story created — ultimate context engine analysis completed |
| 2026-04-13 | Claude Opus 4.6 | Implementation complete — all tasks done, build passes |

### File List

**Files to CREATE:**
- `firmware/utils/DisplayUtils.h` — Namespace with 4 free function declarations
- `firmware/utils/DisplayUtils.cpp` — Implementations of all 4 functions

**Files to MODIFY:**
- `firmware/adapters/NeoMatrixDisplay.h` — Remove 4 private method declarations
- `firmware/adapters/NeoMatrixDisplay.cpp` — Remove 4 implementations, add `#include "utils/DisplayUtils.h"`, update all call sites to `DisplayUtils::` prefix
- `firmware/platformio.ini` — Add `+<../utils/*.cpp>` to `build_src_filter`

**Files UNCHANGED (reference only):**
- `firmware/utils/GeoUtils.h` — Existing header-only utility pattern
- `firmware/utils/Log.h` — Logging macros
- `firmware/interfaces/DisplayMode.h` — Created in ds-1.1
- `firmware/interfaces/BaseDisplay.h` — Existing interface
- `firmware/core/LayoutEngine.h` — LayoutResult struct
- `firmware/models/FlightInfo.h` — FlightInfo struct
