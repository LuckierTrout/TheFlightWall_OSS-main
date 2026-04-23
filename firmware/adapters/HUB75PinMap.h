#pragma once
/*
  HUB75 pin map for the master chain (hw-1.2).

  The mrfaptastic library's HUB75_I2S_CFG::i2s_pins takes 14 GPIOs in
  this order: r1, g1, b1, r2, g2, b2, a, b, c, d, e, lat, oe, clk.
  This header centralises those assignments so swapping in real values
  from the WatangTech (E) adapter silkscreen later is a single-file edit.

  NOTE on naming: field names are lowercase deliberately. Arduino's
  `binary.h` #defines B0..B7 as binary-literal macros, so any uppercase
  B1/B2 identifier gets macro-expanded mid-struct and the compiler
  explodes with cryptic errors. The library itself uses lowercase for
  the same reason.

  Values below are **placeholders** chosen from ESP32-S3 GPIOs known to
  be safe for general I/O: avoids strapping pins (0, 3, 45, 46), flash
  pins (26-32 on N16R8), native USB D+/D- (19, 20), and default UART0
  pins (43, 44). They compile and will appear to work in isolation but
  do NOT match any specific adapter board - update this file with the
  WatangTech (E) board's actual GPIO routing before first bring-up.
*/

#include <stdint.h>

// Plain struct (not used by the library directly - that takes its own
// i2s_pins struct - but keeps the shape obvious for readers).
struct HUB75_PinMap {
    int8_t r1, g1, b1;
    int8_t r2, g2, b2;
    int8_t a, b, c, d, e;
    int8_t lat, oe, clk;
};

// Master chain: 6x 64x64 panels in a 3x2 arrangement (192x128), 1/32 scan.
// TODO(hw-1.2): replace with WatangTech (E) silkscreen values once photographed.
struct MasterChainPins {
    static constexpr int8_t r1  =  1;
    static constexpr int8_t g1  =  2;
    static constexpr int8_t b1  =  4;
    static constexpr int8_t r2  =  5;
    static constexpr int8_t g2  =  6;
    static constexpr int8_t b2  =  7;
    static constexpr int8_t a   =  8;
    static constexpr int8_t b   =  9;
    static constexpr int8_t c   = 10;
    static constexpr int8_t d   = 11;
    static constexpr int8_t e   = 12;  // required for 1/32 scan (64x64 panels)
    static constexpr int8_t lat = 13;
    static constexpr int8_t oe  = 14;
    static constexpr int8_t clk = 15;
};
