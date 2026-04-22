## Epic hw-1: HUB75 Composite Display Migration

Migrate the firmware from WS2812B addressable strips (FastLED + FastLED_NeoMatrix, single data pin, tile-based) to a composite HUB75 display: **3× 64×64 panels (bottom) + 3× 64×32 panels (top)**, driven by an ESP32-S3 (N16R8 Gold Edition) through two independent HUB75 chains. The firmware presents a single **192×96 virtual canvas** to modes, widgets, and the Layout Editor; the adapter routes draws to the correct chain by y-coordinate.

**Why two chains:** 64×64 panels are 1/32 scan, 64×32 panels are 1/16 scan. They cannot share a HUB75 chain — the DMA timing is common across the chain. The migration therefore requires either a dual-output adapter or two single-output adapters and ~26 GPIOs split across two non-overlapping pin groups.

**Scope boundary:** modes, widgets, LayoutEngine, LogoManager, ConfigManager (non-hardware sections), OTA, scheduler, and the Cloudflare aggregator pipeline are **out of scope** — they render through `BaseDisplay` and only need to learn the new canvas dimensions. This epic replaces the adapter, the hardware configuration surface, the wizard, and the PlatformIO toolchain.

**Out-of-scope explicitly:**
- Redesigning the layouts, cards, or widget visuals for the new aspect ratio (HW-1.5 only audits and applies minimal fixes; a broader redesign belongs in a follow-up epic).
- New widget types or layout editor features (deferred; LE-1 ships independently first).
- Chain topologies beyond the 3+3 composite (single-panel dev boards, serpentine arrangements, etc.).

**Dependencies:**
- **LE-1 (Layout Editor V1 MVP) — closed 2026-04-22.** Canvas-size-agnostic (editor reads `matrix.width/height` from `/api/layout`), so HW-1 only needs to bump the reported canvas size and keep `hardware.tile_pixels` as a nominal snap-grid hint.
- **td-7 (ESP32-S3 N16R8 build envs + 16MB partitions) — landed 2026-04-22.** The board target, `[env:esp32s3_n16r8]` build env, `partitions_16mb.csv` (3MB × 2 OTA + ~9.87MB LittleFS), env-aware `check_size.py`, and conditional `DISPLAY_PIN` are already in place. HW-1.1 builds on this rather than re-doing it.

**PSRAM caveat (from td-7 investigation):** on the Lonely Binary N16R8 module, enabling OPI PSRAM (`memory_type = qio_opi` + `BOARD_HAS_PSRAM`) causes a Wi-Fi driver panic on first station connect ("Cache disabled but cached memory region accessed"). PSRAM is therefore **disabled** in `[env:esp32s3_n16r8]`. HW-1.2's framebuffer plan for the 192×96 composite (74 KB for a double-buffered 16bpp frame) **fits in SRAM without PSRAM** (~160KB usable), so this does not block the HUB75 migration — but the HW-1 epic originally assumed PSRAM availability and any plan that relied on it (large logo cache in PSRAM, multi-frame buffers for complex transitions) needs to be revisited before committing to that design.

**Hardware BOM deltas from build-spec doc (for reference — not firmware work):**

| Item | Doc spec | Actual need |
|------|----------|-------------|
| Adapter board | 1× WatangTech (E) | 2× adapter (or one dual-output) — single-output cannot drive two chains |
| Power supply | 5V / 8A (40W) | 5V / 25–30A — 3×64×64 + 3×64×32 peaks around 108W |
| HUB75 ribbons | 1× | 6× (3 per chain: adapter→panel1, panel1→panel2, panel2→panel3) |
| Power pigtails | 2× VH-4P | 6× VH-4P with fused distribution |
| E-pin | not called out | Required on the **bottom-chain** adapter for 1/32 scan 64×64 panels |

---

### Story hw-1.1: Toolchain and library swap (HUB75 in, FastLED out)

As a **firmware engineer**,
I want **FastLED and FastLED NeoMatrix removed and the HUB75 DMA library added as the display-side dependencies**,
So that **the project compiles against the new display stack on the already-landed `[env:esp32s3_n16r8]` env**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the td-7-landed S3 env (`[env:esp32s3_n16r8]`, `partitions_16mb.csv`, env-aware `check_size.py`) **When** `pio run -e esp32s3_n16r8` executes **Then** FastLED and FastLED NeoMatrix are removed from `lib_deps`, the HUB75 DMA library (`mrcodetastic/ESP32 HUB75 LED MATRIX PANEL DMA Display`) is present and compiles, a placeholder `BaseDisplay` stub (returning a 192×96 blank canvas) keeps the project linkable until HW-1.2 lands, and `check_size.py` gates pass against the 3MB OTA slot. **Note:** PSRAM remains disabled per the td-7 Wi-Fi-driver incompatibility finding — HW-1.2's 192×96 framebuffer fits in SRAM without it.

---

### Story hw-1.2: CompositeHUB75Display adapter (two chains, one virtual canvas)

As a **firmware engineer**,
I want **a `CompositeHUB75Display` adapter that owns two `MatrixPanel_I2S_DMA` instances and exposes a single 192×96 virtual canvas via `BaseDisplay`**,
So that **existing modes, widgets, and the Layout Engine render across both chains without knowing the physical split**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the composite adapter is initialized **When** a mode issues draw calls against the 192×96 canvas **Then** pixels with `y < 32` route to the top chain (3× 64×32, 1/16 scan) and pixels with `y ≥ 32` route to the bottom chain (3× 64×64, 1/32 scan) with `y` re-based; `show()` flips DMA buffers on both chains in the same render cycle; brightness, clear, and the existing `calibration`/`positioning` patterns work across both chains and visually validate the y=32 seam; per-chain brightness trim is configurable.

---

### Story hw-1.3: Hardware configuration & NVS schema rewrite

As a **firmware engineer**,
I want **the hardware configuration surface replaced with HUB75 pin groups and virtual-canvas constants, and stale NVS keys retired**,
So that **ConfigManager, WebPortal, and the setup flow stop advertising tile/pin concepts that no longer apply**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the rewritten `HardwareConfiguration.h` **When** `ConfigManager` initializes **Then** `DISPLAY_PIN`, `DISPLAY_TILE_PIXEL_W/H`, and `DISPLAY_TILES_X/Y` are replaced with two `HUB75_PinMap` constants (top chain and bottom chain), per-chain panel size, per-chain length, and `VIRTUAL_WIDTH=192`/`VIRTUAL_HEIGHT=96`; the reboot-required NVS key list drops `display_pin`, `tiles_x`, `tiles_y`, `tile_pixels` and (if exposed) adds per-chain pin-map keys; a device that boots with legacy NVS values containing the removed keys discards them cleanly without crashing or corrupting other config; `/api/layout` returns the new canvas size plus a nominal `tile_pixels` value (e.g., 16) so the LE-1 editor's snap grid keeps working unchanged.

---

### Story hw-1.4: Wizard & dashboard UI for fixed composite hardware

As an **owner**,
I want **the setup wizard and dashboard to reflect the fixed 192×96 composite panel instead of tile/pin configuration**,
So that **first-boot setup is a summary screen rather than a pin picker, and the dashboard exposes the new per-chain brightness trim**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the rewritten wizard **When** a user completes first-boot setup **Then** the display-hardware step is a read-only summary of the 192×96 composite layout (no tile counts, no pin picker, no data-pin allowlist) and submission does not attempt to POST any of the removed NVS keys; the dashboard exposes per-chain brightness trim (if HW-1.2 wired it); `firmware/data/` gzipped assets are regenerated; `tests/smoke/test_web_portal_smoke.py` passes against a live device (or a documented stub) end-to-end.

---

### Story hw-1.5: Mode & widget visual audit at 192×96

As an **owner**,
I want **each display mode visually validated on the new canvas**,
So that **no mode ships broken at 192×96 even if a full redesign is deferred**.

**Status:** backlog

**Acceptance Criteria:**

**Given** hardware running HW-1.2 + HW-1.3 **When** each of `ClassicCardMode`, `LiveFlightCardMode`, `ClockMode`, `DeparturesBoardMode`, `CustomLayoutMode` is selected **Then** the mode renders without overflow, truncation, or crash; any required font/padding/logo-placement fixes are scoped to pull-values-from-`BaseDisplay::width()/height()` adjustments (no hard-coded 16×16 or 128×64 dimensions remain); a screenshot or bench photo of each mode is attached to the story as evidence; deeper aspect-ratio redesigns are filed as follow-up tickets, not attempted here.

---

### Story hw-1.6: Test suite cleanup

As a **maintainer**,
I want **FastLED-specific tests removed and HUB75-layer coordinate routing covered where feasible without hardware**,
So that **the test suite stays green and meaningful after the adapter swap**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the migrated tree **When** `pio test --without-uploading --without-testing` runs all suites **Then** it completes with zero compile errors; `test_neo_matrix_display` is removed (or ported to the HUB75 equivalent with the same `BaseDisplay` contract); a new host-compilable test exercises `CompositeHUB75Display`'s coordinate routing (y<32 → top chain, y≥32 → bottom chain, out-of-bounds clipping); `test_config_manager`, `test_layout_engine`, `test_mode_registry`, and other `BaseDisplay`-dependent suites still pass; `tests/smoke/test_web_portal_smoke.py` has any references to the removed hardware fields cleaned up.

---

### Story hw-1.7: Documentation & hardware build guide

As a **new builder**,
I want **`CLAUDE.md`, `README.md`, `firmware/README.md`, and a new hardware build guide updated to reflect the HUB75 composite build**,
So that **documentation does not contradict the hardware and I can source parts and assemble the wall correctly**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the migrated tree **When** a new builder follows the documentation **Then** project docs no longer reference WS2812B, FastLED, data-pin setup, tile configuration, or the `display_pin`/`tiles_x`/`tiles_y`/`tile_pixels` NVS keys; the BOM table (adapter count, PSU sizing, HUB75 ribbons, power distribution, E-pin requirement on the bottom chain) is accurate; a wiring diagram or pin-group reference for the two chains is present; `AGENTS.md` and MEMORY-style notes reflecting hardware are updated; hardware constraints section in `CLAUDE.md` (heap, binary size, PSRAM usage) reflects the S3-N16R8 reality.
