# Story HW-1.7: Documentation & hardware build guide

branch: epic/le-1-layout-editor
zone:
  - CLAUDE.md                              [edit]
  - README.md                              [edit]
  - AGENTS.md                              [edit]
  - firmware/README.md                     [edit]
  - docs/hardware-build-guide.md           [new]

Status: done

## Story

As a **new builder**,
I want **`CLAUDE.md`, `README.md`, `firmware/README.md`, `AGENTS.md`, and a new hardware build guide updated to reflect the 256×192 HUB75 build**,
So that **documentation does not contradict the hardware and I can source parts and assemble the wall correctly**.

## Acceptance Criteria

1. `CLAUDE.md` project overview reflects the ESP32-S3 + 256×192 HUB75 hardware; legacy WS2812B / FastLED / NeoMatrix references are gone or marked legacy. Critical Constraints section mentions the SRAM framebuffer auto-reduction, gnu++17 requirement, PSRAM disabled, and the new 3MB OTA partition. Reboot-required NVS key list drops `display_pin`/`tiles_x`/`tiles_y`/`tile_pixels`.
2. `README.md` BOM + Hardware section reflects the 12-panel HUB75 build; drops WS2812 component list; links to the new `docs/hardware-build-guide.md`. Legacy-build note preserved so old photos / forks don't confuse readers.
3. `AGENTS.md` Tech Stack table drops FastLED entries and adds the HUB75 DMA library. Project overview updates the pixel dimensions. Directory inventory swaps `NeoMatrixDisplay` → `HUB75MatrixDisplay` + `HUB75PinMap`. Data Flow step 5 names the new adapter class.
4. `firmware/README.md` component list, config quickstart, build env description, and notes all reflect HUB75 (canvas constants, `HUB75PinMap.h`, gnu++17, framebuffer auto-reduction).
5. `docs/hardware-build-guide.md` is new and contains: BOM table, hardware topology diagram, serpentine chain explanation, pin-map table (reflects WatangTech (E) §4.1.1 + `HUB75PinMap.h` values), power distribution rules, mounting instructions, firmware flash commands, first-boot walkthrough, smoke-test protocol, troubleshooting catalogue, and a "what's different from legacy" comparison table.
6. No firmware code changes; both envs still compile.

## Dev Notes

### Scope

Pure documentation. No code touched. The build guide is new; existing `docs/*.md` files (architecture, api-contracts, etc.) are out of scope — they describe firmware internals rather than hardware assembly and were already mostly hardware-agnostic.

### GEMINI.md

Left untouched — it's a Gemini agent persona file that doesn't contain hardware specifics.

### app-overview-and-info-flightwall.md

Left untouched — mostly product-marketing copy; if anyone wants a pass later it's trivial.

## Tasks

- [x] Draft this story.
- [x] Update `CLAUDE.md` (project overview, core-0 FastLED reference, directory table, NVS reboot keys, Critical Constraints).
- [x] Update `README.md` (BOM → summary + link to build guide; Hardware dimensions reworded).
- [x] Update `AGENTS.md` (overview, tech stack table, directory table, data flow).
- [x] Update `firmware/README.md` (what it does, key components, config quickstart, build envs, notes).
- [x] Create `docs/hardware-build-guide.md` (BOM, wiring, power, mounting, flash, first boot, smoke test, troubleshooting, legacy comparison).
- [x] Verify `pio run` on both envs still passes (no code changed; sanity only).

## Change Log

| Date       | Version | Description |
|------------|---------|-------------|
| 2026-04-24 | 0.1     | Draft + implementation landed in one pass. Five documentation files updated; one new build guide (~290 lines) created. |
