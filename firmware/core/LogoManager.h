#pragma once

#include <Arduino.h>

// Logo dimensions and buffer size constants
static constexpr uint16_t LOGO_WIDTH = 32;
static constexpr uint16_t LOGO_HEIGHT = 32;
static constexpr size_t LOGO_PIXEL_COUNT = LOGO_WIDTH * LOGO_HEIGHT;  // 1024
static constexpr size_t LOGO_BUFFER_BYTES = LOGO_PIXEL_COUNT * 2;     // 2048 (RGB565)

class LogoManager {
public:
    static bool init();
    static bool loadLogo(const String& operatorIcao, uint16_t* rgb565Buffer);
    static size_t getLogoCount();
    static void getLittleFSUsage(size_t& usedBytes, size_t& totalBytes);

private:
    static size_t _logoCount;
    static void scanLogoCount();
};
