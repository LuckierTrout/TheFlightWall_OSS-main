# TheFlightWall Hardware Build Guide

A single ESP32-S3 driving a 256×192 HUB75 LED wall (12× 64×64 panels, 4 wide × 3 tall). Single data chain, 1/32 scan. This guide covers the bill of materials, wiring, power distribution, assembly, and first-boot flashing.

> **Heads up:** earlier versions of this project (pre-April 2026) used WS2812B addressable strips driven via a single data pin with FastLED. That build is no longer the canonical hardware. If you're looking at old photos, old BOM links, or older forks, they'll show 20× 16×16 WS2812B panels in a 10×2 arrangement (160×32 canvas). The current hardware documented below is entirely different. Epic `hw-1` (April 2026) migrated the firmware to HUB75.

---

## Bill of materials

| Item | Qty | Notes |
|---|---|---|
| **Lonely Binary ESP32-S3 N16R8 "Gold Edition"** | 1 | 16MB flash, 8MB PSRAM (firmware disables PSRAM per td-7 Wi-Fi compatibility). Other S3-DevKitC-1-compatible boards work too if they expose the right GPIOs for the adapter. |
| **WatangTech RGB Matrix Adapter Board (E)** | 1 | The "(E)" variant has the E-pin broken out — **required** for 1/32 scan panels (i.e., 64×64). Has an S3-DevKitC-1 socket and a classic-ESP32 socket; use the S3 side. Single HUB75 output + 2× VH-4P power-output connectors. |
| **64×64 HUB75 LED panels (1/32 scan)** | 12 | Uniform set. P3 (3mm pitch) gives ~24"×18" total; P4 (~32"×24"). Check panel spec — some cheaper batches use FM6126A or ICN2038S driver ICs and need a one-line firmware config flip (see Troubleshooting). |
| **HUB75 ribbon cables** | 12 | Short 16-pin IDC ribbons. 1× adapter→panel + 11× panel→panel serpentine. Length depends on panel-to-panel spacing in your frame. |
| **5V / 50A power supply** | 1 | Peak draw for 12 panels at full white is ~240W (20A @ 5V). Pick 50A minimum for thermal headroom and to supply the ESP32 + adapter. Mean Well LRS-250-5 or similar. |
| **VH-4P power pigtails** | 12 | One per panel, injected at multiple points along the panel back. Do **not** try to chain power through one pigtail — voltage droop is real at these currents. |
| **Fused DC distribution** | — | Terminal block or bus bar plus per-branch fuses (5A-10A each). Ground the PSU chassis and tie ESP32 GND to the panel GND bus. |
| **M3 magnetic mounting screws** | 48+ | 4 per panel minimum. Neodymium disc base + M3 × 10mm or 12mm threaded post. Sold as "HUB75 magnetic mounting screws" on Amazon / AliExpress. |
| **Steel/iron backer plate** | 1 | Must cover the full 256×192 panel extent. Thin sheet steel or perforated hardware-store "plumber's steel" works. The M3 magnetic screws grab this plate. |
| **USB-C data cable** | 1 | For flashing the ESP32-S3. The adapter's built-in USB port is **power-only** — flashing happens via one of the S3 module's own USB ports. |

**Optional but recommended:**
- IR remote receiver + remote (if you plan to use the mode-switch hotkeys — not yet implemented but wired in the design).
- Small 40mm fan blowing across the back of the panels (passive dissipation is fine at moderate brightness but long spans at full white run warm).

---

## Hardware topology

```
                         5V / 50A PSU
                              │
         ┌────────────────────┴────────────────────┐
         │       fused distribution (5-10A/branch)  │
         └────┬───────┬───────┬───────┬───────┬────┘
              │       │       │       │       │   (×12 VH-4P pigtails,
              │       │       │       │       │    one per panel)
              ▼       ▼       ▼       ▼       ▼
          ┌──────┐┌──────┐┌──────┐┌──────┐  …
          │ P0,0 ││ P0,1 ││ P0,2 ││ P0,3 │   ← top row
          └──┬───┘└──┬───┘└──┬───┘└──┬───┘
             │       │       │       │
         HUB75 ribbons (serpentine daisy-chain)
             │       │       │       │
          ┌──▼───┐┌──▼───┐┌──▼───┐┌──▼───┐
          │ P1,3 ││ P1,2 ││ P1,1 ││ P1,0 │   ← middle row (right→left)
          └──┬───┘└──┬───┘└──┬───┘└──┬───┘
             │       │       │       │
          ┌──▼───┐┌──▼───┐┌──▼───┐┌──▼───┐
          │ P2,0 ││ P2,1 ││ P2,2 ││ P2,3 │   ← bottom row (left→right)
          └──────┘└──────┘└──────┘└──────┘
              ▲
              │  HUB75 from adapter
              │  (only panel P0,0 connects to the adapter)
       ┌──────┴───────┐
       │ WatangTech E │
       └──────┬───────┘
              │  S3-DevKitC-1 socket
              ▼
       ┌──────────────┐
       │ ESP32-S3     │
       │ Lonely B N16R8│
       └──────────────┘
```

**Data flow (HUB75 serpentine, `CHAIN_TOP_LEFT_DOWN`):**
The library's `VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>(3, 4, 64, 64)` preset expects the ribbon to go:
- Adapter → top-left (P0,0) → top row left-to-right → jumper down to bottom-right of middle row → middle row right-to-left → jumper down to bottom-left of bottom row → bottom row left-to-right.

Physically: the ribbon "out" of panel N goes into the ribbon "in" of panel N+1. Panels have silkscreened IN and OUT sides — pay attention or you'll see a mirrored / garbled image.

If your physical routing naturally goes a different direction (e.g., top-right start, or Z-pattern without the serpentine back-and-forth), change the template argument in `firmware/adapters/HUB75MatrixDisplay.h`:

```cpp
using VirtualPanel = VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>;
```

Library presets available:
- `CHAIN_TOP_LEFT_DOWN` / `CHAIN_TOP_RIGHT_DOWN` / `CHAIN_BOTTOM_LEFT_UP` / `CHAIN_BOTTOM_RIGHT_UP` (serpentine)
- `*_ZZ` variants for zigzag / Z-pattern topologies

---

## Pin map (WatangTech (E) adapter → ESP32-S3)

Copied from the WatangTech datasheet §4.1.1 and verified on 2026-04-24. These are the GPIOs the adapter physically routes to when an ESP32-S3-DevKitC-1 is seated in its S3 socket. **This matches what `firmware/adapters/HUB75PinMap.h` sets.**

| HUB75 | ESP32-S3 GPIO | Notes |
|---|---|---|
| R1 | IO37 | (Octal-PSRAM pin; free because PSRAM disabled) |
| G1 | IO06 | |
| B1 | IO36 | (Octal-PSRAM pin; free) |
| R2 | IO35 | (Octal-PSRAM pin; free) |
| G2 | IO05 | |
| B2 | **IO00** | Strapping pin (BOOT). Safe here — panel driver IC is high-Z at reset. |
| A | **IO45** | Strapping pin (VDD_SPI). Safe here — adapter leaves floating at reset; N16R8 flash is 3.3V. |
| B | IO01 | |
| C | IO48 | |
| D | IO02 | |
| E | IO04 | **Required** for 1/32 scan (64×64) panels. Missing or miswired = bottom half of panel dark. |
| LAT | **IO38** | Shares the Lonely Binary onboard RGB_LED pin — if your board has a WS2812 on it, it'll flicker during LAT pulses (cosmetic only). |
| OE | IO21 | |
| CLK | IO47 | |

If you're using a different adapter, photograph or consult its silkscreen and update `firmware/adapters/HUB75PinMap.h` — one file, 14 lines.

---

## Power distribution

Don't skip this. HUB75 panels are **current hogs**.

**Per-panel draw (64×64, typical):**
- All-white full-brightness: ~20W (4A @ 5V)
- Typical mixed content: 5-10W
- Firmware brightness at 128/255 halves this

**12 panels at full brightness white: ~240W / 48A @ 5V.** A 50A PSU gives you some headroom. A 30A PSU will sag under all-white content and trip undervoltage browning (panels will flicker or go dim briefly).

**Distribution rules:**
1. **One pigtail per panel.** Don't daisy-chain power through a single 4-pin cable — at 4A per leg you'll get noticeable voltage drop across 3 panels.
2. **Fuse every branch.** A single short in one pigtail shouldn't take down the whole wall. 5A-10A automotive blade fuses or inline glass fuses are fine.
3. **Thick gauge from PSU to distribution block.** 12 AWG minimum for the PSU→bus-bar run. 16-18 AWG per-panel pigtails are fine at 4A.
4. **Ground the PSU chassis, and tie ESP32 GND to the panel GND bus.** Otherwise you'll see data-line glitches from ground-bounce.
5. **Separate 5V rails OK but not required.** The adapter board feeds its own logic power from the PSU via the VH-4P outputs.

**Power-up sequence:** Apply 5V to panels **before** the ESP32 boots (or at the same time — they share the 5V rail anyway). If you hot-plug HUB75 ribbons on a powered system you can damage panel driver ICs — best practice is power-off → wire → power-on.

---

## Mounting

**Magnetic mounts (recommended):**
- Screw 4× M3 magnetic screws into each panel's back (every panel has 4 M3-threaded holes in standard positions).
- Mount a steel/iron backer plate larger than the 256×192 panel extent (roughly 24"×18" at P3 pitch).
- Snap panels to the plate. Magnets hold firmly but let you slide panels around during alignment.

**Alternative (rigid frame):**
- 3D-printed brackets bolted to each panel's M3 holes, then bolted to a wooden or aluminium frame. More permanent but no alignment forgiveness after assembly.

**Key alignment consideration:** the 4 cardinal gaps between panels in your 4×3 grid are very visible — align by eye against a test pattern (see "First boot smoke test" below) rather than with a ruler. 1mm mismatch at the panel seams is visible; 2mm is ugly.

---

## Firmware flash

**Prerequisites:**
- PlatformIO installed (`pip install platformio` or via the PlatformIO VS Code extension).
- USB-C cable from your computer to one of the S3 module's USB ports. Most S3 DevKitC-1 boards have two USB-C ports:
  - **UART port** (usually with a CH340 bridge) → appears as `/dev/cu.wchusbserial*` on macOS
  - **Native USB port** (USB-OTG) → appears as `/dev/cu.usbmodem*`
  Either works for flashing. The adapter's USB-C is power-only — you **cannot** flash through it.

**First flash:**

```bash
cd firmware/
pio run -e esp32s3_n16r8 -t upload --upload-port /dev/cu.wchusbserial10
pio run -e esp32s3_n16r8 -t uploadfs --upload-port /dev/cu.wchusbserial10
```

(Replace `/dev/cu.wchusbserial10` with your actual port — `ls /dev/cu.*` to find it.)

The first flash wipes any leftover NVS from a previous firmware. If you're upgrading an in-use board with stale state, also do:

```bash
pio run -e esp32s3_n16r8 -t erase --upload-port /dev/cu.wchusbserial10
```

before the `upload` step.

**Serial monitor:**

```bash
pio device monitor -b 115200 -p /dev/cu.wchusbserial10
```

Exit the monitor (`Ctrl+]` in some terminals, `Ctrl+T X` in minicom) before any upload — only one process can hold the serial device.

---

## First boot

1. Power the wall (5V PSU on; panels light briefly then go blank).
2. The ESP32 boots into **AP mode** as `FlightWall-Setup`. Join this Wi-Fi network from your phone/laptop.
3. Navigate to `http://192.168.4.1/` — the **setup wizard** should appear.
4. Walk through the 6 steps:
   - **Wi-Fi** — your home SSID + password.
   - **Aggregator** — URL and bearer token for the Cloudflare Worker (see `workers/flightwall-aggregator/README.md` for deployment).
   - **Location** — lat/lon/radius for the flights you want to see.
   - **Display** — this step is now a read-only summary ("256 × 192 pixels, 12 HUB75 panels"); just confirm.
   - **Review** — double-check all fields.
   - **Test Your Wall** — the firmware paints a calibration pattern you can verify by eye.
5. Click **Save & Connect**. The device reboots, joins your Wi-Fi, and the dashboard becomes available at `http://flightwall.local/`.

---

## First-boot smoke test (recommended)

From the dashboard, toggle **Calibration Mode** under the Display card. You should see:

- Each of the 12 panels showing a distinct primary or secondary color (R/G/B in the top row, Y/C/M in the middle, darker variants in the bottom).
- Each panel's top-left corner showing its grid coordinate ("0,0" through "2,3") in white.
- Four corner markers on the full 256×192 canvas: white top-left, red top-right, green bottom-left, blue bottom-right.

**What this verifies:**
- **All panels lit** → power distribution is working across all 12 pigtails.
- **Colors match the expected grid** → the ribbon chain is routed correctly (serpentine matches `CHAIN_TOP_LEFT_DOWN`). If panels are in the wrong positions or colors are jumbled, adjust ribbon routing or the template parameter in `HUB75MatrixDisplay.h`.
- **Sharp corner markers** → framebuffer is flipping cleanly with no DMA glitches.
- **No ghosting or color bleed** → panel driver IC is happy with the `mxconfig` defaults.

Toggle **Positioning Mode** to see the inter-panel seams and alignment guides — useful during the mechanical-mount step.

---

## Troubleshooting

**First column of every panel shows wrong colors / first-pixel-per-row glitches.**
Some panels use FM6126A or ICN2038S driver ICs and need an explicit register write to configure their latch timing. In `firmware/adapters/HUB75MatrixDisplay.cpp`, find the `HUB75_I2S_CFG mxconfig(...)` block in `initialize()` and add one of:

```cpp
mxconfig.driver = HUB75_I2S_CFG::FM6126A;   // or ICN2038S / FM6124 / MBI5124
```

Rebuild + reflash.

**Panels too dim, or flickering under moving content.**
Library is auto-reducing color depth to fit the 196KB framebuffer into SRAM (expected). If it's visually unacceptable, options:
- Lower pixel clock: `mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M` is default; try `HZ_15M` (faster refresh, more ghosting risk) or drop to `HZ_8M` (slower refresh, cleaner image).
- Reduce panel count (canvas size drops, framebuffer shrinks, more bits available for color depth).

**Bottom half of 64×64 panels dark.**
The E line isn't getting through. Check:
- `MasterChainPins::e` in `HUB75PinMap.h` matches the adapter's actual E pin.
- Ribbons are seated and not kinked; pin 1 (red stripe) is consistent end-to-end.
- The adapter's E-pin jumper (if any) is set — some WatangTech variants have a jumper to enable the E line.

**Top-right panel shows what the top-left should show.**
Ribbon routing doesn't match the template preset. Either physically rewire the ribbons, or change the template parameter in `HUB75MatrixDisplay.h` to a different `CHAIN_*` variant.

**Onboard LED flickering.**
Expected. GPIO38 serves double duty as both HUB75 LAT and the Lonely Binary onboard RGB_LED. Purely cosmetic — no display impact. Desolder the LED if it bothers you.

**Wi-Fi panics on first connect ("Cache disabled but cached memory region accessed").**
PSRAM got enabled in `platformio.ini`. Verify the `[env:esp32s3_n16r8]` section does **not** have `-D BOARD_HAS_PSRAM` or `board_build.arduino.memory_type = qio_opi`. The Lonely Binary N16R8 has an OPI-PSRAM/Wi-Fi-driver incompatibility documented in td-7; firmware expects PSRAM disabled.

**Boot loop / nothing on serial.**
- Unplug all HUB75 ribbons, power-cycle, flash again. If the ESP32 boots fine with no panels attached, the issue is likely the panels browning the 5V rail — upsize your PSU.
- If it still won't boot, try a full `pio run -t erase` followed by `upload` + `uploadfs`.

---

## Reference: what's different from the legacy WS2812B build

| Concern | Old (WS2812B) | New (HUB75) |
|---|---|---|
| Panel type | 16×16 addressable strip squares | 64×64 HUB75 matrix panels |
| Panel count | 20 | 12 |
| Physical arrangement | 10 wide × 2 tall | 4 wide × 3 tall |
| Canvas size | 160×32 | **256×192** |
| Data wiring | 1 wire (GPIO25 / GPIO21 on S3) | 14-pin HUB75 ribbon via adapter |
| Library | FastLED + FastLED_NeoMatrix | mrcodetastic/esp32-hub75-matrixpanel-dma |
| ESP32 target | classic ESP32 | ESP32-S3 required (LCD_CAM peripheral) |
| Power | 5V / 20A | 5V / 50A |
| Config UI | Tile count + data-pin picker | Read-only summary (wall is fixed) |
| Color depth | 8 bits/channel | 5-6 bits/channel (auto, library-managed) |
| Refresh rate | ~40-60 Hz | ~60 Hz (depends on color depth chosen) |

The firmware was migrated in epic `hw-1` (April 2026). See the epic and story files under `_bmad-output/planning-artifacts/epics/epic-hw-1.md` for the detailed migration history.
