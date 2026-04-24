#pragma once
/*
  HUB75 pin map for the master chain (hw-1.2, real values 2026-04-24).

  Values below are the WatangTech RGB Matrix Adapter Board (E) wiring for
  ESP32-S3-DevKitC-1 (§4.1.1 of the product datasheet, photographed by the
  owner 2026-04-24):

       HUB75        ESP32-S3 GPIO
       ---------    -------------
       R1           IO37
       G1           IO06
       B1           IO36
       R2           IO35
       G2           IO05
       B2           IO00   <-- strapping pin (BOOT select), see note
       A            IO45   <-- strapping pin (VDD_SPI select), see note
       B            IO01
       C            IO48
       D            IO02
       E            IO04   (required for 1/32 scan 64x64 panels)
       LAT          IO38   <-- onboard RGB_LED pin on Lonely Binary, see note
       OE           IO21
       CLK          IO47

  NOTES on caveats the adapter vendor silently accepts:

    - GPIO0 (B2): strapping pin. At reset it selects boot mode (HIGH =
      normal boot, LOW = download). The adapter's R2C connection to the
      panel is high-impedance at reset (panel driver ICs don't drive the
      line back into the MCU), so boot strap reads correctly. Once the
      app is running, the DMA engine drives it as pixel data with no
      issue. If you ever flash via the RX/TX pins instead of USB and see
      boot failures, this is the suspect.

    - GPIO45 (A): strapping pin that selects VDD_SPI voltage (HIGH = 1.8V
      flash, LOW/floating = 3.3V). The Lonely Binary N16R8 uses 3.3V
      flash, so the pin must NOT read HIGH at reset. The adapter leaves
      it high-impedance until the app drives it, matching the expected
      default. Working as intended.

    - GPIO38 (LAT): on the Lonely Binary ESP32-S3 N16R8 pinout card this
      pin is labelled `RGB_LED`. If a WS2812 is physically wired on the
      board, it will flicker briefly during HUB75 LAT pulses - cosmetic
      only (no interference with panel timing because the WS2812 treats
      LAT activity as garbage data and times out). If the LED starts
      showing random colors after init, this is why; it's harmless.

  NAMING NOTE: field names are lowercase because Arduino's binary.h
  #defines B0..B7 as binary-literal macros, which would collide with
  any uppercase B1/B2 identifier.
*/

#include <stdint.h>

struct HUB75_PinMap {
    int8_t r1, g1, b1;
    int8_t r2, g2, b2;
    int8_t a, b, c, d, e;
    int8_t lat, oe, clk;
};

// Master chain: 12x 64x64 panels arranged 4 wide x 3 tall (256x192), 1/32 scan.
// WatangTech (E) adapter §4.1.1 wiring for ESP32-S3-DevKitC-1.
struct MasterChainPins {
    static constexpr int8_t r1  = 37;
    static constexpr int8_t g1  =  6;
    static constexpr int8_t b1  = 36;
    static constexpr int8_t r2  = 35;
    static constexpr int8_t g2  =  5;
    static constexpr int8_t b2  =  0;
    static constexpr int8_t a   = 45;
    static constexpr int8_t b   =  1;
    static constexpr int8_t c   = 48;
    static constexpr int8_t d   =  2;
    static constexpr int8_t e   =  4;
    static constexpr int8_t lat = 38;
    static constexpr int8_t oe  = 21;
    static constexpr int8_t clk = 47;
};
