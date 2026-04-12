## Epic ds-1: Display Mode Architecture & Classic Card Migration

The device renders flight data through a pluggable display mode system, with the existing Classic Card output migrated into the first mode — pixel-identical to the current Foundation Release output. The emergency fallback renderer ensures the display always works even if a mode fails to initialize.

### Story ds-1.1: Heap Baseline Measurement & Core Abstractions

As a firmware developer,
I want to measure the available heap after Foundation boot and define the DisplayMode interface and RenderContext struct,
So that all subsequent mode implementations have a documented memory budget and a stable contract to code against.

**Acceptance Criteria:**

**Given** the device has completed Foundation Release boot (WiFi connected, NTP synced, all existing tasks running)
**When** the heap baseline measurement executes
**Then** ESP.getFreeHeap() is logged to Serial and the value is documented as a constant in a header file (e.g., HEAP_BASELINE_AFTER_BOOT)
**And** the DisplayMode abstract class is defined in interfaces/DisplayMode.h with pure virtual methods: init(), render(const RenderContext&), teardown(), getMemoryRequirement(), getName(), getZoneLayout()
**And** the RenderContext struct is defined with: matrix reference, zone bounds (x, y, width, height), current brightness level — all members are const, and the render() method receives RenderContext as `const&` to enforce read-only access at compile time (NFR S5)
**And** the ZoneDescriptor struct is defined for getZoneLayout() return values with: label, gridRow, gridColumn fields
**And** the existing device behavior is completely unchanged — this story adds only new files and a Serial.println

**Requirements:** FR1, FR2, FR3, AR1, AR6

### Story ds-1.2: DisplayUtils Extraction

As a firmware developer,
I want shared rendering helper functions extracted from NeoMatrixDisplay into a standalone utility header,
So that both NeoMatrixDisplay and future display mode classes can use them without circular dependencies.

**Acceptance Criteria:**

**Given** NeoMatrixDisplay contains rendering helper functions (drawTextLine, truncateToColumns, formatTelemetryValue, drawBitmapRGB565)
**When** the extraction is complete
**Then** utils/DisplayUtils.h contains these functions as free functions (not class methods)
**And** NeoMatrixDisplay.cpp includes DisplayUtils.h and delegates to the extracted functions
**And** all existing rendering output is pixel-identical to pre-extraction behavior
**And** the project compiles cleanly with no new warnings
**And** no existing #include dependencies are broken

**Requirements:** AR4

### Story ds-1.3: ModeRegistry & Static Registration System

As a firmware developer,
I want a ModeRegistry that manages a static table of display modes with activation lifecycle, heap validation, and fallback,
So that modes can be registered at compile time and switched safely at runtime.

**Acceptance Criteria:**

**Given** the DisplayMode interface exists from Story 1.1
**When** ModeRegistry is implemented
**Then** a MODE_TABLE[] static array in main.cpp holds pointers to all registered DisplayMode instances
**And** ModeRegistry::enumerate() returns mode count and names with zero heap allocation and completes under 10ms (NFR P4)
**And** ModeRegistry::activate(id) executes the full lifecycle: teardown current → validate heap → init new → begin rendering (FR6)
**And** if ESP.getFreeHeap() after teardown is below the requested mode's getMemoryRequirement() + 30KB safety margin, activation is rejected and the previous mode is restored (FR8, FR9, NFR S4)
**And** concurrent activate() calls are serialized with queue-of-one semantics — a switch initiated during an in-progress transition replaces any pending request (latest wins, previous pending discarded), not an unbounded queue (FR7)
**And** mode switch requests from WebPortal (Core 1) are communicated to the display task (Core 0) via an atomic flag pattern matching existing g_configChanged (AR2)
**And** adding a new mode requires exactly two touch points: one .h/.cpp pair in modes/ and one line in MODE_TABLE[] (AR5)

**Requirements:** FR5, FR6, FR7, FR8, FR9, FR10, AR2, AR5, NFR P4, NFR S1, NFR S2, NFR S4

### Story ds-1.4: Classic Card Mode Implementation

As a FlightWall user,
I want the existing three-line flight card display migrated into a ClassicCardMode that renders through the new mode system,
So that the familiar display continues working identically while enabling the mode architecture.

**Acceptance Criteria:**

**Given** the DisplayMode interface and DisplayUtils exist from previous stories
**When** ClassicCardMode is implemented in modes/ClassicCardMode.h/.cpp
**Then** render() draws the three-line flight card (airline name, route, aircraft type) using DisplayUtils functions
**And** the output is pixel-identical to the current Foundation Release rendering when compared across 5+ different flight cards (NFR C1)
**And** when two or more flights are available, the mode cycles through them at the configurable interval (FR12)
**And** all rendering stays within the zone bounds provided in RenderContext — no pixels are written outside the zone (FR4)
**And** the mode reports its memory requirement via getMemoryRequirement()
**And** getName() returns "Classic Card" and getZoneLayout() returns zone metadata describing the layout regions
**And** stack-local allocations during render() are under 512 bytes (NFR S3)
**And** brightness is read from RenderContext but never modified (NFR S5)

**Requirements:** FR4, FR11, FR12, FR13, NFR C1, NFR S3, NFR S5

### Story ds-1.5: Display Pipeline Integration

As a FlightWall user,
I want all flight display output to flow through the mode system with NeoMatrixDisplay owning only hardware and frame commit,
So that the device boots, activates Classic Card mode, and renders identically to the Foundation Release.

**Acceptance Criteria:**

**Given** ModeRegistry, ClassicCardMode, and DisplayUtils are complete from previous stories
**When** the display pipeline is refactored
**Then** NeoMatrixDisplay owns only: hardware init (FastLED.addLeds), frame commit (FastLED.show()), shared resources (logo buffer), and emergency fallback renderer (AR3)
**And** the display task in main.cpp calls ModeRegistry to get the active mode and invokes render() each frame
**And** only the shared display pipeline calls FastLED.show() — modes never commit the matrix directly (FR35)
**And** ClassicCardMode is registered in MODE_TABLE[] and activated as the default mode on boot
**And** if no mode can initialize, the emergency fallback renderer (displaySingleFlightCard) activates to ensure the display always works (FR36, AR8)
**And** per-frame display work completes within 16ms with no blocking delays (NFR P2, NFR C3)
**And** no new concurrent rendering tasks are created — cooperative scheduling is preserved (NFR C3)
**And** all Foundation Release features continue working: OTA, night mode, NTP, dashboard, health endpoint (NFR C2)
**And** after 100 consecutive mode switches (Classic Card → fallback → Classic Card), free heap returns to within 1KB of starting value (NFR S1)
**And** 10 rapid switch toggles in 3 seconds produce no undefined states, corruption, or watchdog resets (NFR S2)
**And** `pio run` builds cleanly and the device renders pixel-identically to Foundation Release

**Requirements:** FR34, FR35, FR36, AR3, AR8, NFR P1, NFR P2, NFR C2, NFR C3, NFR S1, NFR S2

---

