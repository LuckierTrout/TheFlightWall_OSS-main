## Epic hw-1: HUB75 Composite Display Migration (dual-MCU)

Migrate the firmware from WS2812B addressable strips (FastLED + FastLED_NeoMatrix, single data pin, tile-based) to a composite HUB75 display: **3× 64×32 panels (top strip) + 6× 64×64 panels (3×2 bottom grid)**, driven by **two ESP32-S3 (N16R8 Gold Edition) boards**. A master S3 runs the full FlightWall firmware and drives the bottom grid (192×128) directly via LCD_CAM; a slave S3 runs a minimal SPI-framebuffer receiver and drives the top strip (192×32) via its own LCD_CAM. The master composes a single **192×160 virtual canvas** and routes rows with `y < 32` over SPI to the slave and `y >= 32` to the local LCD_CAM chain.

**Why two MCUs (changed from the original plan):** the ESP32-S3's LCD_CAM peripheral is a singleton — its register bank (`LCD_CAM.lcd_user.*`, `LCD_CAM.lcd_clock.*`) is global state and the mrfaptastic HUB75 library directly pokes it during `begin()`. You cannot instantiate two `MatrixPanel_I2S_DMA` objects with different scan rates on one S3; the second would overwrite the first's configuration. The mixed scan rates here (64×32 at 1/16, 64×64 at 1/32) therefore require either (a) a library fork that widens the 16-bit LCD_CAM bus to carry both chains' RGB and duplicates 64×32 row data across the 1/32 address space (~2-4 weeks of DMA-composer work plus ~50% brightness loss on the top strip) or (b) a second MCU dedicated to the top strip. Option (b) was chosen: smaller per-story risk, no library fork, preserves full brightness on both chains, and an incremental path where the master wall is usable end-to-end at 192×128 before any slave work lands.

**Scope boundary:** modes, widgets, LayoutEngine, LogoManager, ConfigManager (non-hardware sections), OTA, scheduler, and the Cloudflare aggregator pipeline are **out of scope** — they render through `BaseDisplay` and only need to learn the new canvas dimensions. This epic replaces the adapter, adds a slave firmware target, the inter-MCU protocol, the composite canvas wrapper, the hardware configuration surface, the wizard, and the PlatformIO toolchain.

**Incremental delivery:** stories hw-1.2–hw-1.4 deliver a **fully working 192×128 wall** with no slave hardware required. Slave-side work (hw-1.5–hw-1.7) is additive and lifts the canvas to 192×160. If slave work is delayed or blocked, the master-only build is a usable shipped product.

**Out-of-scope explicitly:**
- Redesigning the layouts, cards, or widget visuals for the new aspect ratio (hw-1.9 only audits and applies minimal fixes; a broader redesign belongs in a follow-up epic).
- New widget types or layout editor features (deferred; LE-1 ships independently first).
- Chain topologies beyond the 3×2 bottom grid + 1×3 top strip (single-panel dev boards, serpentine arrangements beyond what `VirtualMatrixPanel_T` presets cover).
- Slave OTA (story hw-1.8 captures the design; MVP ships slave as USB-reflash-only).

**Dependencies:**
- **LE-1 (Layout Editor V1 MVP) — closed 2026-04-22.** Canvas-size-agnostic (editor reads `matrix.width/height` from `/api/layout`), so HW-1 only needs to bump the reported canvas size.
- **td-7 (ESP32-S3 N16R8 build envs + 16MB partitions) — landed 2026-04-22.** The board target, `[env:esp32s3_n16r8]` build env, `partitions_16mb.csv` (3MB × 2 OTA + ~9.87MB LittleFS), env-aware `check_size.py`, and conditional `DISPLAY_PIN` are in place. HW-1 builds on this rather than re-doing it.
- **hw-1.1 (toolchain swap) — landed 2026-04-22.** FastLED/FastLED_NeoMatrix removed; `mrcodetastic/ESP32 HUB75 LED MATRIX PANEL DMA Display` in `lib_deps`; `HUB75MatrixDisplay` stub renders to a 192×96 `GFXcanvas16` (display-dead).

**PSRAM caveat (from td-7 investigation):** on the Lonely Binary N16R8 module, enabling OPI PSRAM (`memory_type = qio_opi` + `BOARD_HAS_PSRAM`) causes a Wi-Fi driver panic on first station connect ("Cache disabled but cached memory region accessed"). PSRAM is therefore **disabled** in `[env:esp32s3_n16r8]`. Master framebuffer for 192×128 double-buffered at 16bpp is ~98 KB; fits in internal SRAM (~160 KB usable) with headroom. Slave's 192×32 double-buffered framebuffer is ~24 KB; trivially fits.

**Hardware BOM (revised):**

| Item | Quantity | Notes |
|------|----------|-------|
| ESP32-S3 N16R8 Gold Edition | 2 | master + slave; user already owns both |
| WatangTech RGB Matrix Adapter Board (E) | 2 | one per S3; E-pin required on master's adapter for 1/32 scan |
| 64×64 HUB75 panels (1/32 scan) | 6 | master chain, 3×2 arrangement |
| 64×32 HUB75 panels (1/16 scan) | 3 | slave chain, 1×3 arrangement |
| HUB75 ribbons | 8 | 5 for master chain (adapter→p1, p1→p2, p2→p3, jumper to second row, p4→p5→p6), 3 for slave |
| Power supply 5V / 25-30A | 1 | combined peak ~120 W across 9 panels + 2 MCUs |
| VH-4P power pigtails | 9 | one per panel |
| Inter-MCU wiring | — | 4-5 wires between master and slave: MOSI, SCLK, CS, flip-sync GPIO, GND |

---

### Story hw-1.1: Toolchain and library swap (HUB75 in, FastLED out) — DONE

✅ Landed 2026-04-22. FastLED and FastLED_NeoMatrix removed from `lib_deps`; `mrcodetastic/ESP32 HUB75 LED MATRIX PANEL DMA Display` present and compiling; `HUB75MatrixDisplay` stub renders to a 192×96 `GFXcanvas16` with display-dead `show()`. Both `[env:esp32dev]` and `[env:esp32s3_n16r8]` pass. See `stories/hw-1-1-toolchain-and-library-swap.md`.

---

### Story hw-1.2: Master HUB75MatrixDisplay — single chain, 192×128 (6× 64×64)

As a **firmware engineer**,
I want **the `HUB75MatrixDisplay` stub replaced with a real driver that pushes pixels to 6× 64×64 panels in a 3×2 arrangement through `MatrixPanel_I2S_DMA` + `VirtualMatrixPanel_T`**,
So that **the master S3 renders a live 192×128 wall end-to-end on real hardware — usable and shippable even before slave work lands**.

**Status:** in-progress

**Acceptance Criteria:**

**Given** the master S3 wired to the WatangTech (E) adapter driving a 3×2 grid of 64×64 HUB75 panels **When** `HUB75MatrixDisplay::initialize()` is called **Then** a single `MatrixPanel_I2S_DMA` instance is configured for one 6-panel chain at 1/32 scan with `double_buff = true`, a `VirtualMatrixPanel_T<CHAIN_TYPE>` wraps it to expose a 192×128 `Adafruit_GFX` canvas, `buildRenderContext()` hands that virtual canvas to modes/widgets as `ctx.matrix` so existing rendering flows unchanged, `show()` calls `flipDMABuffer()` for tear-free updates, `updateBrightness()` calls `setBrightness8()`, the existing calibration and positioning patterns are updated to paint across the full 192×128 canvas with corner/edge markers validating panel order + orientation, pin-map constants live in one `HUB75_PinMap` struct with TODO markers until wiring is photographed, `pio run -e esp32dev` and `pio run -e esp32s3_n16r8` both compile and pass `check_size.py`, and a physical smoke test shows all 6 panels lit with correct color and no ghosting when calibration mode is enabled.

---

### Story hw-1.3: Hardware configuration & NVS schema rewrite

As a **firmware engineer**,
I want **the hardware configuration surface replaced with HUB75 constants (single-chain today, slave-link fields reserved for hw-1.5+), and stale NVS keys retired**,
So that **ConfigManager, WebPortal, and the setup flow stop advertising tile/pin concepts that no longer apply**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the rewritten `HardwareConfiguration.h` **When** `ConfigManager` initializes **Then** `DISPLAY_PIN`, `DISPLAY_TILE_PIXEL_W/H`, and `DISPLAY_TILES_X/Y` are replaced with a `HUB75_PinMap` constant for the master chain, per-chain panel size and chain length constants, and `MASTER_CANVAS_WIDTH = 192`/`MASTER_CANVAS_HEIGHT = 128` / `COMPOSITE_WIDTH = 192` / `COMPOSITE_HEIGHT = 160` constants; the reboot-required NVS key list drops `display_pin`, `tiles_x`, `tiles_y`, `tile_pixels` and adds `slave_enabled` (bool, default false so master-only builds work out of the box); a device that boots with legacy NVS values containing the removed keys discards them cleanly without crashing or corrupting other config; `/api/layout` returns `matrix.width = 192`, `matrix.height = 128` when `slave_enabled = false` and `matrix.height = 160` when true, plus a nominal `tile_pixels` value (64 for the 64×64 grid) so the LE-1 editor's snap grid keeps working.

---

### Story hw-1.4: Wizard & dashboard UI for master-only 192×128

As an **owner**,
I want **the setup wizard and dashboard to reflect the fixed 192×128 master build and surface a toggle for enabling the slave link when hardware is ready**,
So that **first-boot setup is a summary screen rather than a pin picker, the dashboard exposes the slave-link toggle (inert until hw-1.5 lands), and smoke tests keep passing**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the rewritten wizard **When** a user completes first-boot setup **Then** the display-hardware step is a read-only summary of the 192×128 composite layout (no tile counts, no pin picker, no data-pin allowlist) and submission does not attempt to POST any of the removed NVS keys; the dashboard exposes a `slave_enabled` toggle with help text explaining the hw-1 slave link; `firmware/data/` gzipped assets are regenerated; `tests/smoke/test_web_portal_smoke.py` passes against a live device (or a documented stub) end-to-end.

---

### Story hw-1.5: Slave firmware scaffold — SPI framebuffer receiver for 3× 64×32

As a **firmware engineer**,
I want **a minimal slave sketch that runs on the second ESP32-S3, receives framebuffer updates over SPI-slave DMA, and drives a single 3-panel 64×32 chain at 1/16 scan**,
So that **the top-strip hardware has a target that can be flashed, tested, and smoke-verified independently of the master**.

**Status:** backlog

**Acceptance Criteria:**

**Given** a dedicated `firmware/slave/` directory with its own `platformio.ini` (env `flightwall_slave` targeting the same Lonely Binary N16R8 board) **When** the slave is flashed and powered **Then** it initializes its `MatrixPanel_I2S_DMA` for a 3×64×32 chain at 1/16 scan with `double_buff = true`, allocates a 192×32 RGB565 back-framebuffer (~12 KB), configures SPI-slave DMA to receive whole-frame writes into the back-framebuffer, on receipt of a "flip" command (see hw-1.6 for protocol) pushes the back-framebuffer to `drawPixelRGB565` across the chain and calls `flipDMABuffer()`, and — with the master disconnected — a USB-flashable test build paints a hard-coded calibration pattern so the slave can be validated standalone.

---

### Story hw-1.6: Inter-MCU protocol — framing, flip-sync, boot handshake

As a **firmware engineer**,
I want **a well-defined SPI protocol between master and slave with framing, flip-sync, boot handshake, and link-loss handling**,
So that **the master can reliably push rendered top-strip pixels to the slave at 60 Hz with no tearing, detect when the slave is offline, and degrade gracefully**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the master running on the primary S3 with SPI-master configured on four pins (MOSI, SCLK, CS, flip-sync as GPIO) and the slave running hw-1.5 **When** the master completes a frame render **Then** the master ships the top 192×32 region as a single SPI DMA transaction (~12 KB payload + a small frame header containing frame number and CRC16), asserts the flip-sync GPIO to tell the slave to flip its back-framebuffer to the DMA buffer, and on boot the master polls for a slave "ready" byte before starting frame shipment; link-loss is detected (no ACK within 100 ms) and surfaced as a `SystemStatus::slaveLinkOk` flag; the dashboard shows slave-link state (connected / disconnected / last-frame-age); master-only builds (`slave_enabled = false`) skip all SPI work so they stay usable on a single S3.

---

### Story hw-1.7: CompositeHUB75Display — 192×160 virtual canvas wrapper

As a **firmware engineer**,
I want **a `CompositeHUB75Display` class that wraps `HUB75MatrixDisplay` (local 192×128) plus the slave link (remote 192×32) and exposes a single 192×160 `BaseDisplay`**,
So that **modes, widgets, and the Layout Engine draw into one virtual canvas and never know the y=32 row boundary exists**.

**Status:** backlog

**Acceptance Criteria:**

**Given** `slave_enabled = true` and the slave responding **When** a mode draws into the composite canvas **Then** rows with `y < 32` route to an internal 192×32 `GFXcanvas16` which is shipped to the slave via the hw-1.6 protocol on `show()`, rows with `y >= 32` route to `HUB75MatrixDisplay` (master LCD_CAM chain) with `y` re-based (`y - 32` mapped to the 192×128 canvas — or the Layout Engine is taught to produce 192×160 zones directly; one approach chosen with trade-offs documented); the calibration pattern validates the y=32 seam with a horizontal line spanning the full composite and corner markers on both halves; the master renders without the slave (`slave_enabled = false`) by simply returning the local 192×128 as the full canvas.

---

### Story hw-1.8: Slave OTA via master (optional)

As an **owner**,
I want **the dashboard to push slave firmware updates over SPI when the master is updated**,
So that **I don't have to plug a USB cable into the slave S3 to update it once the wall is assembled**.

**Status:** backlog (optional — MVP ships slave as USB-reflash-only)

**Acceptance Criteria:**

**Given** the dashboard OTA card and a slave firmware binary in the master's LittleFS **When** the user triggers a slave OTA **Then** the master streams the binary to the slave over the hw-1.6 SPI protocol using a distinct "firmware block" message type, the slave writes to its inactive OTA partition and validates a trailing SHA-256, the slave reboots into the new firmware and signals "ready" again, and a failed update rolls back cleanly; the dashboard UI reports master firmware version AND slave firmware version and the outcome of the last slave update.

---

### Story hw-1.9: Mode & widget visual audit at 192×160 (or 192×128 master-only)

As an **owner**,
I want **each display mode visually validated on the new canvas**,
So that **no mode ships broken at 192×128 or 192×160 even if a full redesign is deferred**.

**Status:** backlog

**Acceptance Criteria:**

**Given** hardware running hw-1.2 (master-only) or hw-1.7 (composite) **When** each of `ClassicCardMode`, `LiveFlightCardMode`, `ClockMode`, `DeparturesBoardMode`, `CustomLayoutMode` is selected **Then** the mode renders without overflow, truncation, or crash; any required font/padding/logo-placement fixes are scoped to pull-values-from-`BaseDisplay::width()/height()` adjustments (no hard-coded 16×16 or 128×64 dimensions remain); a screenshot or bench photo of each mode is attached to the story as evidence; deeper aspect-ratio redesigns are filed as follow-up tickets, not attempted here.

---

### Story hw-1.10: Test suite cleanup

As a **maintainer**,
I want **FastLED-specific tests removed and HUB75-layer host-compile tests added where feasible**,
So that **the test suite stays green and meaningful after the adapter swap**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the migrated tree **When** `pio test --without-uploading --without-testing` runs all suites **Then** it completes with zero compile errors; `test_neo_matrix_display` is removed (or ported to the HUB75 equivalent with the same `BaseDisplay` contract); a new host-compilable test exercises composite-canvas row routing (y<32 → slave-bound 192×32 buffer, y>=32 → master 192×128, out-of-bounds clipping); `test_config_manager`, `test_layout_engine`, `test_mode_registry`, and other `BaseDisplay`-dependent suites still pass; `tests/smoke/test_web_portal_smoke.py` has any references to the removed hardware fields cleaned up.

---

### Story hw-1.11: Documentation & hardware build guide

As a **new builder**,
I want **`CLAUDE.md`, `README.md`, `firmware/README.md`, and a new hardware build guide updated to reflect the dual-MCU HUB75 composite build**,
So that **documentation does not contradict the hardware and I can source parts and assemble the wall correctly**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the migrated tree **When** a new builder follows the documentation **Then** project docs no longer reference WS2812B, FastLED, data-pin setup, tile configuration, or the `display_pin`/`tiles_x`/`tiles_y`/`tile_pixels` NVS keys; the BOM table (adapter count, PSU sizing, HUB75 ribbons, power distribution, E-pin requirement on the master's adapter, two MCUs, inter-MCU wiring) is accurate; a wiring diagram for the two chains + inter-MCU SPI link is present; `AGENTS.md` and MEMORY-style notes reflecting hardware are updated; hardware constraints section in `CLAUDE.md` (heap, binary size, PSRAM usage) reflects the S3-N16R8 reality.
