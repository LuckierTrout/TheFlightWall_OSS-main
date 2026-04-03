/*
Purpose: Manages airline logo loading from LittleFS with PROGMEM fallback sprite.
Responsibilities:
- Ensure /logos/ directory exists on init.
- Load RGB565 logo files by operator ICAO code (uppercase, trimmed).
- Provide a PROGMEM fallback airplane sprite when logo is missing or corrupt.
- Report logo count and filesystem usage for health metrics.
*/
#include "core/LogoManager.h"
#include "utils/Log.h"
#include <LittleFS.h>

static const char LOGO_DIR[] = "/logos";

size_t LogoManager::_logoCount = 0;

namespace {
String logoBasename(const String& path) {
    int lastSlash = path.lastIndexOf('/');
    return lastSlash >= 0 ? path.substring(lastSlash + 1) : path;
}
}

// --- Fallback airplane sprite (32x32 RGB565, stored in PROGMEM) ---
// A simple airplane silhouette: white (0xFFFF) on black (0x0000).
// Oriented nose-up, stylized front view with wings spread.
static const uint16_t FALLBACK_SPRITE[LOGO_PIXEL_COUNT] PROGMEM = {
    // Row 0-3: empty top margin
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 4: nose tip
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 5
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 6
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 7
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 8: fuselage widens
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 9
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 10: fuselage body
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 11
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 12: wings start
    0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,
    // Row 13: wings full span
    0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,
    // Row 14: wings full span
    0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,
    // Row 15: wings widest
    0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,
    // Row 16: wings taper
    0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,
    // Row 17
    0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,
    // Row 18: back to fuselage
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 19
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 20
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 21
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 22: tail fin starts
    0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,
    // Row 23: tail fin
    0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,
    // Row 24: tail fin widest
    0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,
    // Row 25: tail base
    0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,
    // Row 26: fuselage end
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 27
    0,0,0,0,0,0,0,0,0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // Row 28-31: empty bottom margin
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

bool LogoManager::init() {
    if (!LittleFS.exists(LOGO_DIR)) {
        if (!LittleFS.mkdir(LOGO_DIR)) {
            LOG_E("LogoManager", "Failed to create /logos/ directory");
            return false;
        }
        LOG_I("LogoManager", "Created /logos/ directory");
    } else {
        File dir = LittleFS.open(LOGO_DIR);
        if (!dir || !dir.isDirectory()) {
            if (dir) {
                dir.close();
            }
            LOG_E("LogoManager", "/logos exists but is not a directory");
            return false;
        }
        dir.close();
    }

    scanLogoCount();
    Serial.println("[LogoManager] Initialized — " + String((unsigned)_logoCount) + " logos found");
    return true;
}

bool LogoManager::loadLogo(const String& operatorIcao, uint16_t* rgb565Buffer) {
    if (rgb565Buffer == nullptr) {
        return false;
    }

    // Normalize ICAO: uppercase and trimmed
    String icao = operatorIcao;
    icao.trim();
    icao.toUpperCase();

    if (icao.length() == 0) {
        memcpy_P(rgb565Buffer, FALLBACK_SPRITE, LOGO_BUFFER_BYTES);
        return false;
    }

    String path = String(LOGO_DIR) + "/" + icao + ".bin";

    File f = LittleFS.open(path, "r");
    if (!f) {
        // No logo file — use fallback
        memcpy_P(rgb565Buffer, FALLBACK_SPRITE, LOGO_BUFFER_BYTES);
        return false;
    }

    // Validate exact file size
    size_t fileSize = f.size();
    if (fileSize != LOGO_BUFFER_BYTES) {
        Serial.println("[LogoManager] Logo " + icao + " bad size: " + String((unsigned)fileSize));
        f.close();
        memcpy_P(rgb565Buffer, FALLBACK_SPRITE, LOGO_BUFFER_BYTES);
        return false;
    }

    size_t bytesRead = f.read(reinterpret_cast<uint8_t*>(rgb565Buffer), LOGO_BUFFER_BYTES);
    f.close();

    if (bytesRead != LOGO_BUFFER_BYTES) {
        Serial.println("[LogoManager] Logo " + icao + " read error: got " + String((unsigned)bytesRead) + " bytes");
        memcpy_P(rgb565Buffer, FALLBACK_SPRITE, LOGO_BUFFER_BYTES);
        return false;
    }

    return true;
}

size_t LogoManager::getLogoCount() {
    return _logoCount;
}

void LogoManager::getLittleFSUsage(size_t& usedBytes, size_t& totalBytes) {
    usedBytes = LittleFS.usedBytes();
    totalBytes = LittleFS.totalBytes();
}

bool LogoManager::isSafeLogoFilename(const String& filename) {
    if (filename.length() <= 4) {
        return false;
    }
    if (!filename.endsWith(".bin")) {
        return false;
    }
    if (filename.indexOf('/') >= 0 || filename.indexOf('\\') >= 0) {
        return false;
    }
    if (filename.indexOf("..") >= 0) {
        return false;
    }
    return true;
}

void LogoManager::scanLogoCount() {
    _logoCount = 0;
    File dir = LittleFS.open(LOGO_DIR);
    if (!dir || !dir.isDirectory()) {
        if (dir) {
            dir.close();
        }
        return;
    }

    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String name = logoBasename(entry.name());
            if (isSafeLogoFilename(name) && entry.size() == LOGO_BUFFER_BYTES) {
                _logoCount++;
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
}

bool LogoManager::listLogos(std::vector<LogoEntry>& result) {
    result.clear();
    File dir = LittleFS.open(LOGO_DIR);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return false;
    }

    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String name = logoBasename(entry.name());
            if (!isSafeLogoFilename(name) || entry.size() != LOGO_BUFFER_BYTES) {
                entry.close();
                entry = dir.openNextFile();
                continue;
            }
            LogoEntry e;
            e.name = name;
            e.size = entry.size();
            result.push_back(e);
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
    return true;
}

std::vector<LogoEntry> LogoManager::listLogos() {
    std::vector<LogoEntry> result;
    listLogos(result);
    return result;
}

bool LogoManager::hasLogo(const String& filename) {
    if (!isSafeLogoFilename(filename)) {
        return false;
    }
    String path = String(LOGO_DIR) + "/" + filename;
    return LittleFS.exists(path);
}

bool LogoManager::deleteLogo(const String& filename) {
    if (!isSafeLogoFilename(filename)) {
        return false;
    }
    String path = String(LOGO_DIR) + "/" + filename;
    if (!LittleFS.exists(path)) {
        return false;
    }
    bool ok = LittleFS.remove(path);
    if (ok) {
        scanLogoCount();
    }
    return ok;
}

bool LogoManager::saveLogo(const String& filename, const uint8_t* data, size_t len) {
    if (len != LOGO_BUFFER_BYTES || !isSafeLogoFilename(filename)) {
        return false;
    }

    String path = String(LOGO_DIR) + "/" + filename;
    File f = LittleFS.open(path, "w");
    if (!f) {
        return false;
    }

    size_t written = f.write(data, len);
    f.close();

    if (written != len) {
        LittleFS.remove(path);
        return false;
    }

    scanLogoCount();
    return true;
}
