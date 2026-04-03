#pragma once

#include <Arduino.h>
#include <vector>

// Logo dimensions and buffer size constants
static constexpr uint16_t LOGO_WIDTH = 32;
static constexpr uint16_t LOGO_HEIGHT = 32;
static constexpr size_t LOGO_PIXEL_COUNT = LOGO_WIDTH * LOGO_HEIGHT;  // 1024
static constexpr size_t LOGO_BUFFER_BYTES = LOGO_PIXEL_COUNT * 2;     // 2048 (RGB565)

struct LogoEntry {
    String name;      // filename without path (e.g. "UAL.bin")
    size_t size;      // file size in bytes
};

class LogoManager {
public:
    static bool init();
    static bool loadLogo(const String& operatorIcao, uint16_t* rgb565Buffer);
    static size_t getLogoCount();
    static void getLittleFSUsage(size_t& usedBytes, size_t& totalBytes);
    static bool isSafeLogoFilename(const String& filename);
    static bool listLogos(std::vector<LogoEntry>& result);
    static std::vector<LogoEntry> listLogos();
    static bool hasLogo(const String& filename);
    static bool deleteLogo(const String& filename);
    static bool saveLogo(const String& filename, const uint8_t* data, size_t len);

    static void scanLogoCount();

private:
    static size_t _logoCount;
};
