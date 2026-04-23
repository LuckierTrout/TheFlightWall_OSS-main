## Epic hw-1: HUB75 Display Migration (uniform 12-panel, 256x192)

Migrate the firmware from WS2812B addressable strips (FastLED + FastLED_NeoMatrix, single data pin, tile-based) to a uniform HUB75 composite display: **12× 64×64 panels arranged 4 wide × 3 tall** (256×192 canvas), driven by a single ESP32-S3 (N16R8 Gold Edition) through one WatangTech (E) HUB75 adapter and one DMA chain at 1/32 scan. The firmware presents a single **256×192 Adafruit_GFX canvas** to modes, widgets, and the Layout Editor via `BaseDisplay`; the mrfaptastic `VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>` template handles the 4×3 grid serpentine routing into one logical frame.

**Why a single chain (revised 2026-04-23):** the earlier dual-MCU plan existed solely to work around a mixed-scan panel choice (1/16 top strip + 1/32 bottom grid) which the ESP32-S3's singleton `LCD_CAM` peripheral cannot drive at two scan rates simultaneously. Returning the 3× 64×32 panels and buying 3 more 64×64s makes the entire chain 1/32-scan uniform, which the library supports natively in a single `MatrixPanel_I2S_DMA` instance. The canvas grows from the dual-MCU plan's 192×160 (30,720 px) to the uniform plan's **256×192 (49,152 px)** — ~60% more display area — while eliminating the slave firmware, SPI protocol, flip-sync, composite canvas wrapper, and slave OTA stories entirely. ~2-3 weeks of firmware work avoided; 5 cross-MCU risks (seam tearing, per-chain color matching, slave maintenance, debug surface, protocol reliability) deleted.

**Scope boundary:** modes, widgets, LayoutEngine, LogoManager, ConfigManager (non-hardware sections), OTA, scheduler, and the Cloudflare aggregator pipeline are **out of scope** — they render through `BaseDisplay` and only need to learn the new canvas dimensions. This epic replaces the adapter, the hardware configuration surface, the wizard, and the PlatformIO toolchain.

**Out-of-scope explicitly:**
- Redesigning the layouts, cards, or widget visuals for the new aspect ratio (hw-1.5 only audits and applies minimal fixes; a broader redesign belongs in a follow-up epic).
- New widget types or layout editor features (deferred; LE-1 ships independently first).
- Chain topologies beyond the uniform 4×3 grid (single-panel dev boards, non-64×64 mixes, alternate aspect ratios).

**Dependencies:**
- **LE-1 (Layout Editor V1 MVP) — closed 2026-04-22.** Canvas-size-agnostic (editor reads `matrix.width/height` from `/api/layout`), so HW-1 only needs to bump the reported canvas size.
- **td-7 (ESP32-S3 N16R8 build envs + 16MB partitions) — landed 2026-04-22.** The board target, `[env:esp32s3_n16r8]` build env, `partitions_16mb.csv` (3MB × 2 OTA + ~9.87MB LittleFS), env-aware `check_size.py`, and conditional `DISPLAY_PIN` are in place. HW-1 builds on this rather than re-doing it.
- **hw-1.1 (toolchain swap) — landed 2026-04-22.** FastLED/FastLED_NeoMatrix removed; `mrcodetastic/ESP32 HUB75 LED MATRIX PANEL DMA Display` in `lib_deps`.
- **hw-1.2 (master adapter) — landed 2026-04-22, revised 2026-04-23.** Initial implementation targeted 192×128 (3×2); bumped to 256×192 (4×3) when the uniform-panel decision was made.
- **hw-1.3 (NVS schema rewrite) — landed 2026-04-22, revised 2026-04-23.** `slave_enabled` flag added by the dual-MCU plan was retired in the revision — single-chain has no composite mode to gate.

**PSRAM caveat (from td-7 investigation):** on the Lonely Binary N16R8 module, enabling OPI PSRAM (`memory_type = qio_opi` + `BOARD_HAS_PSRAM`) causes a Wi-Fi driver panic on first station connect ("Cache disabled but cached memory region accessed"). PSRAM is therefore **disabled** in `[env:esp32s3_n16r8]`. A 256×192 double-buffered 16 bpc framebuffer is ~196 KB — this **exceeds ESP32-S3 internal SRAM headroom** (~160 KB usable with Wi-Fi + FreeRTOS overhead). Mitigation: the mrfaptastic library will auto-reduce color depth (likely to 5-6 bpc) to keep the framebuffer in SRAM and maintain ≥60 Hz refresh. This is expected and acceptable for the FlightWall use case (flight cards, tickers, logos) where color gradient smoothness is not critical. `FORCE_COLOR_DEPTH` is **not** set so the library picks the right trade-off automatically.

**12-panel chain — additional caveats:**
- **I2S clock speed:** `mxconfig.i2sspeed = HZ_10M` is a reasonable default; `HZ_15M` or `HZ_20M` available if refresh rate is visibly low but may introduce ghosting on some panel batches.
- **LED driver chip:** if the sourced panels use FM6126A or ICN2038S driver ICs (common on cheaper batches), `mxconfig.driver = HUB75_I2S_CFG::FM6126A` / `ICN2038S` may be required for the first column of pixels to render correctly. Discover empirically at bring-up.

**Hardware BOM:**

| Item | Quantity | Notes |
|------|----------|-------|
| ESP32-S3 N16R8 Gold Edition | 1 | user owns 2; second is spare |
| WatangTech RGB Matrix Adapter Board (E) | 1 | user owns 2; second is spare/return |
| 64×64 HUB75 panels (1/32 scan) | 12 | 4 wide × 3 tall serpentine chain |
| HUB75 ribbons | 12 | 1 adapter→panel + 11 panel→panel |
| Power supply 5V / 50A | 1 | peak ≈240 W at full white across 12 panels; pick 50A for margin and heat |
| VH-4P power pigtails | 12 | one per panel, injected at multiple points along the chain |
| M3 magnetic mounting screws | 48+ | 4 per panel; plus spares |
| Steel/iron backer plate | 1 | for the magnetic mounts; sized to the full 256×192-pixel physical extent (≈24"×18" at P3, ≈32"×24" at P4) |

---

### Story hw-1.1: Toolchain and library swap (HUB75 in, FastLED out) — DONE

✅ Landed 2026-04-22. FastLED and FastLED_NeoMatrix removed from `lib_deps`; `mrcodetastic/ESP32 HUB75 LED MATRIX PANEL DMA Display` present and compiling; `HUB75MatrixDisplay` stub renders to a `GFXcanvas16` with display-dead `show()`. Both `[env:esp32dev]` and `[env:esp32s3_n16r8]` pass. See `stories/hw-1-1-toolchain-and-library-swap.md`.

---

### Story hw-1.2: Master HUB75MatrixDisplay — single chain, 256×192 (12× 64×64) — DONE + REVISED

✅ Initial implementation landed 2026-04-22 targeting 192×128 (3×2 of 64×64). Revised 2026-04-23 to 256×192 (4×3 of 64×64) after the uniform-panel hardware decision. Replaces the hw-1.1 GFXcanvas stub with a real `MatrixPanel_I2S_DMA` + `VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>(3, 4, 64, 64)` driver. `show()` calls `flipDMABuffer()` for tear-free updates. Pin map in `adapters/HUB75PinMap.h` with TODO placeholders pending WatangTech (E) silkscreen photo. Builds: esp32s3_n16r8 green. On-device smoke test remains the final gate. See `stories/hw-1-2-single-chain-hub75-192x128.md` (filename retained; content reflects the revised 256×192 target).

---

### Story hw-1.3: Hardware configuration & NVS schema rewrite — DONE + REVISED

✅ Initial implementation landed 2026-04-22 with a `slave_enabled` flag reserved for the dual-MCU composite. Revised 2026-04-23: `slave_enabled` retired (single-chain has no composite mode), canvas constants updated to 256×192, and `LayoutEngine::compute(HardwareConfig)` simplified to target the fixed 256×192 directly. Legacy tile/pin keys (`display_pin`/`tiles_x`/`tiles_y`/`tile_pixels`) are removed from `HardwareConfig`, NVS persistence, `/api/settings` GET, and the reboot-key list. `applyJson` silently accepts retired keys for backward-compat with unfixed clients until hw-1.4 cleans up the UI. `/api/layout` reports `matrix.width=256`, `matrix.height=192`, `tile_pixels=64` (nominal for the LE-1 snap-grid). See `stories/hw-1-3-nvs-schema-rewrite.md`.

---

### Story hw-1.4: Wizard & dashboard UI for fixed 256×192 composite

As an **owner**,
I want **the setup wizard and dashboard to reflect the fixed 256×192 uniform HUB75 wall instead of tile/pin configuration**,
So that **first-boot setup is a summary screen rather than a pin picker and smoke tests pass end-to-end**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the rewritten wizard **When** a user completes first-boot setup **Then** the display-hardware step is a read-only summary of the 256×192 layout (no tile counts, no pin picker, no data-pin allowlist, no slave toggle) and submission does not attempt to POST any of the retired NVS keys; the dashboard drops the tile/pin UI; `firmware/data/` gzipped assets are regenerated (wizard.js, dashboard.js, style.css as needed, plus `.gz` copies); `tests/smoke/test_web_portal_smoke.py` passes against a live device (or a documented stub) end-to-end with references to removed hardware fields cleaned up.

---

### Story hw-1.5: Mode & widget visual audit at 256×192

As an **owner**,
I want **each display mode visually validated on the new canvas**,
So that **no mode ships broken at 256×192 even if a full redesign is deferred**.

**Status:** backlog

**Acceptance Criteria:**

**Given** hardware running hw-1.2 + hw-1.3 **When** each of `ClassicCardMode`, `LiveFlightCardMode`, `ClockMode`, `DeparturesBoardMode`, `CustomLayoutMode` is selected **Then** the mode renders without overflow, truncation, or crash; any required font/padding/logo-placement fixes are scoped to pull-values-from-`BaseDisplay::width()/height()` adjustments (no hard-coded 16×16 or 128×64 dimensions remain); a screenshot or bench photo of each mode is attached to the story as evidence; deeper aspect-ratio redesigns are filed as follow-up tickets, not attempted here.

---

### Story hw-1.6: Test suite cleanup

As a **maintainer**,
I want **FastLED-specific tests removed and HUB75-layer host-compile tests added where feasible**,
So that **the test suite stays green and meaningful after the adapter swap**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the migrated tree **When** `pio test --without-uploading --without-testing` runs all suites **Then** it completes with zero compile errors; `test_neo_matrix_display` is removed (or ported to the HUB75 equivalent with the same `BaseDisplay` contract); `test_config_manager`, `test_layout_engine`, `test_mode_registry`, and other `BaseDisplay`-dependent suites pass against 256×192; `tests/smoke/test_web_portal_smoke.py` has any references to the removed hardware fields cleaned up.

---

### Story hw-1.7: Documentation & hardware build guide

As a **new builder**,
I want **`CLAUDE.md`, `README.md`, `firmware/README.md`, and a new hardware build guide updated to reflect the uniform 12-panel 256×192 HUB75 build**,
So that **documentation does not contradict the hardware and I can source parts and assemble the wall correctly**.

**Status:** backlog

**Acceptance Criteria:**

**Given** the migrated tree **When** a new builder follows the documentation **Then** project docs no longer reference WS2812B, FastLED, data-pin setup, tile configuration, or the `display_pin`/`tiles_x`/`tiles_y`/`tile_pixels` NVS keys; the BOM table (1× adapter, 1× MCU, 12× panels, 50A PSU, ribbons, power distribution, E-pin requirement, magnetic mounts) is accurate; a wiring diagram for the serpentine 4×3 chain is present; `AGENTS.md` and MEMORY-style notes reflecting hardware are updated; hardware constraints section in `CLAUDE.md` (heap, binary size, PSRAM usage, expected color-depth auto-reduction at 12 panels) reflects the S3-N16R8 reality.
