/*
Purpose: Check GitHub Releases API for newer firmware versions and download OTA updates.
Responsibilities:
- HTTPS GET the latest release from the configured GitHub repo.
- Parse tag_name, release notes, and binary/sha256 asset URLs with ArduinoJson v7.
- Compare remote version against compile-time FW_VERSION.
- Download firmware binary with incremental SHA-256 verification (Story dl-7.1).
- Surface state, error messages, progress, and parsed fields for downstream consumers.
Inputs: GitHub owner/repo strings (set once via init()).
Outputs: OTAState, remote version, release notes, asset URLs, error messages, download progress.
*/
#include "core/OTAUpdater.h"
#include "core/ModeRegistry.h"
#include "core/SystemStatus.h"
#include "utils/Log.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

// FW_VERSION is a compile-time build flag from platformio.ini.
// Fallback for unit test harness or non-PlatformIO compilation.
#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"
#endif

// --- Static storage ---
bool     OTAUpdater::_initialized = false;
OTAState OTAUpdater::_state = OTAState::IDLE;
char     OTAUpdater::_repoOwner[OWNER_MAX_LEN] = {};
char     OTAUpdater::_repoName[REPO_MAX_LEN] = {};
char     OTAUpdater::_remoteVersion[VERSION_MAX_LEN] = {};
char     OTAUpdater::_lastError[ERROR_MAX_LEN] = {};
char     OTAUpdater::_releaseNotes[NOTES_MAX_LEN] = {};
char     OTAUpdater::_binaryAssetUrl[URL_MAX_LEN] = {};
char     OTAUpdater::_sha256AssetUrl[URL_MAX_LEN] = {};
TaskHandle_t OTAUpdater::_downloadTaskHandle = nullptr;
uint8_t  OTAUpdater::_progress = 0;
OTAFailurePhase OTAUpdater::_failurePhase = OTAFailurePhase::NONE;

// --- init ---

void OTAUpdater::init(const char* repoOwner, const char* repoName) {
    if (_initialized) {
        LOG_W("OTAUpdater", "init() called more than once — ignoring");
        return;
    }
    if (!repoOwner || !repoName) {
        LOG_E("OTAUpdater", "init() called with null owner or repo");
        return;
    }

    strncpy(_repoOwner, repoOwner, OWNER_MAX_LEN - 1);
    _repoOwner[OWNER_MAX_LEN - 1] = '\0';

    strncpy(_repoName, repoName, REPO_MAX_LEN - 1);
    _repoName[REPO_MAX_LEN - 1] = '\0';

    _state = OTAState::IDLE;
    _lastError[0] = '\0';
    _remoteVersion[0] = '\0';
    _releaseNotes[0] = '\0';
    _binaryAssetUrl[0] = '\0';
    _sha256AssetUrl[0] = '\0';
    _initialized = true;

#if LOG_LEVEL >= 2
    Serial.printf("[OTAUpdater] Initialized for %s/%s\n", _repoOwner, _repoName);
#endif
}

// --- Accessors ---

OTAState    OTAUpdater::getState()          { return _state; }
const char* OTAUpdater::getStateString() {
    switch (_state) {
        case OTAState::IDLE:        return "idle";
        case OTAState::CHECKING:    return "checking";
        case OTAState::AVAILABLE:   return "available";
        case OTAState::DOWNLOADING: return "downloading";
        case OTAState::VERIFYING:   return "verifying";
        case OTAState::REBOOTING:   return "rebooting";
        case OTAState::ERROR:       return "error";
        default:                    return "idle";
    }
}
const char* OTAUpdater::getLastError()      { return _lastError; }
const char* OTAUpdater::getRemoteVersion()  { return _remoteVersion; }
const char* OTAUpdater::getReleaseNotes()   { return _releaseNotes; }
const char* OTAUpdater::getBinaryAssetUrl() { return _binaryAssetUrl; }
const char* OTAUpdater::getSha256AssetUrl() { return _sha256AssetUrl; }
uint8_t     OTAUpdater::getProgress()      { return _progress; }
OTAFailurePhase OTAUpdater::getFailurePhase() { return _failurePhase; }
const char* OTAUpdater::getFailurePhaseString() {
    switch (_failurePhase) {
        case OTAFailurePhase::NONE:     return "none";
        case OTAFailurePhase::DOWNLOAD: return "download";
        case OTAFailurePhase::VERIFY:   return "verify";
        case OTAFailurePhase::BOOT:     return "boot";
        default:                        return "none";
    }
}
bool        OTAUpdater::isRetriable() {
    return _failurePhase == OTAFailurePhase::DOWNLOAD ||
           _failurePhase == OTAFailurePhase::VERIFY;
}

// --- _resetForTest (for unit tests only) ---

void OTAUpdater::_resetForTest() {
    _initialized = false;
    _state = OTAState::IDLE;
    _repoOwner[0] = '\0';
    _repoName[0] = '\0';
    _remoteVersion[0] = '\0';
    _lastError[0] = '\0';
    _releaseNotes[0] = '\0';
    _binaryAssetUrl[0] = '\0';
    _sha256AssetUrl[0] = '\0';
    _downloadTaskHandle = nullptr;
    _progress = 0;
    _failurePhase = OTAFailurePhase::NONE;
}

// --- normalizeVersion (public, testable) ---

bool OTAUpdater::normalizeVersion(const char* tag, char* outBuf, size_t outBufSize) {
    if (!tag || !outBuf || outBufSize == 0) return false;

    // Skip leading whitespace
    while (*tag == ' ' || *tag == '\t' || *tag == '\r' || *tag == '\n') tag++;

    // Strip leading 'v' or 'V'
    if (*tag == 'v' || *tag == 'V') tag++;

    if (*tag == '\0') {
        outBuf[0] = '\0';
        return false;
    }

    // Copy until end, then trim trailing whitespace
    strncpy(outBuf, tag, outBufSize - 1);
    outBuf[outBufSize - 1] = '\0';

    // Trim trailing whitespace
    size_t len = strlen(outBuf);
    while (len > 0 && (outBuf[len - 1] == ' ' || outBuf[len - 1] == '\t' ||
                        outBuf[len - 1] == '\r' || outBuf[len - 1] == '\n')) {
        outBuf[--len] = '\0';
    }

    return len > 0;
}

// --- compareVersions (public, testable) ---

bool OTAUpdater::compareVersions(const char* remoteNormalized, const char* localNormalized) {
    if (!remoteNormalized || !localNormalized) return false;
    return strcmp(remoteNormalized, localNormalized) != 0;
}

// --- parseReleaseJson (public, testable) ---

bool OTAUpdater::parseReleaseJson(const char* json, size_t jsonLen) {
    if (!json || jsonLen == 0) return false;

    // Use ArduinoJson v7 filter to reduce memory — only extract fields we need.
    // AC #4: Include both browser_download_url and url fields for fallback.
    JsonDocument filter;
    filter["tag_name"] = true;
    filter["body"] = true;
    filter["assets"][0]["name"] = true;
    filter["assets"][0]["browser_download_url"] = true;
    filter["assets"][0]["url"] = true;  // Fallback per AC #4

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json, jsonLen,
        DeserializationOption::Filter(filter));
    if (err) {
        snprintf(_lastError, ERROR_MAX_LEN, "JSON parse error: %s", err.c_str());
        return false;
    }

    // Extract tag_name
    const char* tagName = doc["tag_name"] | (const char*)nullptr;
    if (!tagName) {
        snprintf(_lastError, ERROR_MAX_LEN, "Missing tag_name in release JSON");
        return false;
    }

    // Normalise remote version
    if (!normalizeVersion(tagName, _remoteVersion, VERSION_MAX_LEN)) {
        snprintf(_lastError, ERROR_MAX_LEN, "Failed to normalise tag_name: %s", tagName);
        return false;
    }

    // Extract release notes (body), truncated to NOTES_MAX_LEN
    const char* body = doc["body"] | "";
    strncpy(_releaseNotes, body, NOTES_MAX_LEN - 1);
    _releaseNotes[NOTES_MAX_LEN - 1] = '\0';

    // Scan assets for first .bin and first .sha256
    _binaryAssetUrl[0] = '\0';
    _sha256AssetUrl[0] = '\0';

    JsonArray assets = doc["assets"].as<JsonArray>();
    for (JsonObject asset : assets) {
        const char* name = asset["name"] | "";
        // AC #4 fix: Check browser_download_url first, fall back to url if empty.
        // GitHub releases API typically uses browser_download_url, but the url field
        // is also valid for asset retrieval via API authentication.
        const char* assetUrl = asset["browser_download_url"] | "";
        if (!assetUrl[0]) {
            assetUrl = asset["url"] | "";
        }

        if (!name[0] || !assetUrl[0]) continue;

        size_t nameLen = strlen(name);

        // Match first .bin asset
        if (_binaryAssetUrl[0] == '\0' && nameLen > 4 &&
            strcmp(name + nameLen - 4, ".bin") == 0) {
            strncpy(_binaryAssetUrl, assetUrl, URL_MAX_LEN - 1);
            _binaryAssetUrl[URL_MAX_LEN - 1] = '\0';
        }

        // Match first .sha256 asset
        if (_sha256AssetUrl[0] == '\0' && nameLen > 7 &&
            strcmp(name + nameLen - 7, ".sha256") == 0) {
            strncpy(_sha256AssetUrl, assetUrl, URL_MAX_LEN - 1);
            _sha256AssetUrl[URL_MAX_LEN - 1] = '\0';
        }

        // Stop early if both found
        if (_binaryAssetUrl[0] != '\0' && _sha256AssetUrl[0] != '\0') break;
    }

    return true;
}

// --- parseReleaseJsonStream (stream-based parsing for bounded heap) ---

bool OTAUpdater::parseReleaseJsonStream(Stream& stream) {
    // Use ArduinoJson v7 filter to reduce memory — only extract fields we need.
    // AC #4: Include both browser_download_url and url fields for fallback.
    JsonDocument filter;
    filter["tag_name"] = true;
    filter["body"] = true;
    filter["assets"][0]["name"] = true;
    filter["assets"][0]["browser_download_url"] = true;
    filter["assets"][0]["url"] = true;  // Fallback per AC #4

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, stream,
        DeserializationOption::Filter(filter));
    if (err) {
        snprintf(_lastError, ERROR_MAX_LEN, "JSON parse error: %s", err.c_str());
        return false;
    }

    // Extract tag_name
    const char* tagName = doc["tag_name"] | (const char*)nullptr;
    if (!tagName) {
        snprintf(_lastError, ERROR_MAX_LEN, "Missing tag_name in release JSON");
        return false;
    }

    // Normalise remote version
    if (!normalizeVersion(tagName, _remoteVersion, VERSION_MAX_LEN)) {
        snprintf(_lastError, ERROR_MAX_LEN, "Failed to normalise tag_name: %s", tagName);
        return false;
    }

    // Extract release notes (body), truncated to NOTES_MAX_LEN
    const char* body = doc["body"] | "";
    strncpy(_releaseNotes, body, NOTES_MAX_LEN - 1);
    _releaseNotes[NOTES_MAX_LEN - 1] = '\0';

    // Scan assets for first .bin and first .sha256
    // Note: _binaryAssetUrl and _sha256AssetUrl already cleared in checkForUpdate()

    JsonArray assets = doc["assets"].as<JsonArray>();
    for (JsonObject asset : assets) {
        const char* name = asset["name"] | "";
        // AC #4 fix: Check browser_download_url first, fall back to url if empty.
        const char* assetUrl = asset["browser_download_url"] | "";
        if (!assetUrl[0]) {
            assetUrl = asset["url"] | "";
        }

        if (!name[0] || !assetUrl[0]) continue;

        size_t nameLen = strlen(name);

        // Match first .bin asset
        if (_binaryAssetUrl[0] == '\0' && nameLen > 4 &&
            strcmp(name + nameLen - 4, ".bin") == 0) {
            strncpy(_binaryAssetUrl, assetUrl, URL_MAX_LEN - 1);
            _binaryAssetUrl[URL_MAX_LEN - 1] = '\0';
        }

        // Match first .sha256 asset
        if (_sha256AssetUrl[0] == '\0' && nameLen > 7 &&
            strcmp(name + nameLen - 7, ".sha256") == 0) {
            strncpy(_sha256AssetUrl, assetUrl, URL_MAX_LEN - 1);
            _sha256AssetUrl[URL_MAX_LEN - 1] = '\0';
        }

        // Stop early if both found
        if (_binaryAssetUrl[0] != '\0' && _sha256AssetUrl[0] != '\0') break;
    }

    return true;
}

// --- checkForUpdate ---

bool OTAUpdater::checkForUpdate() {
    if (!_initialized) {
        snprintf(_lastError, ERROR_MAX_LEN, "OTAUpdater not initialized");
        _state = OTAState::ERROR;
        return false;
    }

    _state = OTAState::CHECKING;
    _lastError[0] = '\0';

    // Clear all remote metadata fields upfront to prevent stale data on any early return.
    // Antipattern fix: previous implementation left stale fields after failed/up-to-date checks.
    _remoteVersion[0] = '\0';
    _releaseNotes[0] = '\0';
    _binaryAssetUrl[0] = '\0';
    _sha256AssetUrl[0] = '\0';

    // Build URL: https://api.github.com/repos/{owner}/{repo}/releases/latest
    char url[256];
    snprintf(url, sizeof(url),
        "https://api.github.com/repos/%s/%s/releases/latest",
        _repoOwner, _repoName);

    // HTTPS client — setInsecure() acceptable per AC #2 tradeoff:
    // ESP32 does not bundle api.github.com CA roots by default.
    // Future: bundle Let's Encrypt root CA for production hardening.
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, url);
    http.setTimeout(10000); // NFR21: ≤10s timeout

    // GitHub API requires User-Agent and Accept headers (AC #3)
    http.addHeader("Accept", "application/json");
    char userAgent[64];
    snprintf(userAgent, sizeof(userAgent), "FlightWall-OTA/1.0 FW/%s", FW_VERSION);
    http.addHeader("User-Agent", userAgent);

    int httpCode = http.GET();

    // Handle rate limiting (AC #6)
    if (httpCode == 429) {
        _state = OTAState::ERROR;
        snprintf(_lastError, ERROR_MAX_LEN, "Rate limited — try again later");
        LOG_W("OTAUpdater", "GitHub API rate limited (429)");
        http.end();
        return false;
    }

    // Handle network failure or non-200 (AC #7)
    if (httpCode <= 0) {
        _state = OTAState::ERROR;
        snprintf(_lastError, ERROR_MAX_LEN, "Connection failed (error %d)", httpCode);
        LOG_E("OTAUpdater", "Connection failed");
        http.end();
        return false;
    }

    if (httpCode != 200) {
        _state = OTAState::ERROR;
        snprintf(_lastError, ERROR_MAX_LEN, "HTTP %d from GitHub API", httpCode);
        LOG_W("OTAUpdater", "GitHub API returned non-200");
        http.end();
        return false;
    }

    // Parse response via stream to avoid unbounded heap allocation (AC #4).
    // Antipattern fix: replaced http.getString() with direct stream parsing.
    // GitHub releases/latest JSON is typically <10KB, but stream parsing is safer.
    WiFiClient* stream = http.getStreamPtr();
    if (!stream) {
        _state = OTAState::ERROR;
        snprintf(_lastError, ERROR_MAX_LEN, "Failed to get response stream");
        LOG_E("OTAUpdater", "No stream pointer");
        http.end();
        return false;
    }

    if (!parseReleaseJsonStream(*stream)) {
        _state = OTAState::ERROR;
        // _lastError already set by parseReleaseJsonStream
        LOG_E("OTAUpdater", "JSON parse failed");
        http.end();
        return false;
    }
    http.end();

    // Compare versions (AC #5)
    char localNormalized[VERSION_MAX_LEN];
    normalizeVersion(FW_VERSION, localNormalized, VERSION_MAX_LEN);

    if (compareVersions(_remoteVersion, localNormalized)) {
        _state = OTAState::AVAILABLE;
        LOG_I("OTAUpdater", "Update available");
        return true;
    } else {
        _state = OTAState::IDLE;
        LOG_I("OTAUpdater", "Firmware up to date");
        return false;
    }
}

// --- parseSha256File (public, testable) ---

bool OTAUpdater::parseSha256File(const char* body, size_t bodyLen,
                                  uint8_t* outDigest, size_t outDigestSize) {
    if (!body || bodyLen == 0 || !outDigest || outDigestSize < 32) return false;

    // Skip leading whitespace
    size_t i = 0;
    while (i < bodyLen && (body[i] == ' ' || body[i] == '\t' ||
                           body[i] == '\r' || body[i] == '\n')) {
        i++;
    }

    // Extract exactly 64 hex characters
    if (i + 64 > bodyLen) return false;

    for (size_t h = 0; h < 32; h++) {
        char hi = body[i + h * 2];
        char lo = body[i + h * 2 + 1];

        // Convert hex chars to nibbles
        uint8_t hiVal, loVal;

        if (hi >= '0' && hi <= '9')      hiVal = hi - '0';
        else if (hi >= 'a' && hi <= 'f') hiVal = hi - 'a' + 10;
        else if (hi >= 'A' && hi <= 'F') hiVal = hi - 'A' + 10;
        else return false;

        if (lo >= '0' && lo <= '9')      loVal = lo - '0';
        else if (lo >= 'a' && lo <= 'f') loVal = lo - 'a' + 10;
        else if (lo >= 'A' && lo <= 'F') loVal = lo - 'A' + 10;
        else return false;

        outDigest[h] = (hiVal << 4) | loVal;
    }

    // After 64 hex chars, must be EOF, whitespace, or space+filename (common .sha256 format)
    size_t afterHex = i + 64;
    if (afterHex < bodyLen) {
        char next = body[afterHex];
        if (next != ' ' && next != '\t' && next != '\r' && next != '\n' && next != '\0') {
            return false;  // Unexpected character after hex digest
        }
    }

    return true;
}

// --- compareSha256 (public, testable) ---

bool OTAUpdater::compareSha256(const uint8_t* a, const uint8_t* b) {
    if (!a || !b) return false;
    // Constant-time compare on fixed 32-byte digests
    uint8_t diff = 0;
    for (size_t i = 0; i < 32; i++) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

// --- startDownload (Story dl-7.1, AC #1) ---

bool OTAUpdater::startDownload() {
    // Guard: reject if already downloading (AC #1)
    if (_downloadTaskHandle != nullptr) {
        snprintf(_lastError, ERROR_MAX_LEN, "Download already in progress");
        LOG_W("OTAUpdater", "startDownload rejected: task already running");
        return false;
    }

    // Guard: must be in AVAILABLE state with URLs populated
    if (_state != OTAState::AVAILABLE) {
        snprintf(_lastError, ERROR_MAX_LEN, "No update available to download");
        LOG_W("OTAUpdater", "startDownload rejected: state not AVAILABLE");
        return false;
    }

    if (_binaryAssetUrl[0] == '\0') {
        snprintf(_lastError, ERROR_MAX_LEN, "Binary asset URL not set");
        _state = OTAState::ERROR;
        return false;
    }

    // Spawn one-shot FreeRTOS task pinned to Core 1 (AC #1: stack >= 8192, priority 1)
    _progress = 0;
    _failurePhase = OTAFailurePhase::NONE;
    _state = OTAState::DOWNLOADING;

    BaseType_t result = xTaskCreatePinnedToCore(
        _downloadTaskProc,
        "ota_dl",
        10240,          // 10 KB stack (HTTP + TLS + SHA context needs headroom)
        nullptr,
        1,              // Priority 1 per epic
        &_downloadTaskHandle,
        1               // Core 1 (network core)
    );

    if (result != pdPASS) {
        _downloadTaskHandle = nullptr;
        _state = OTAState::ERROR;
        snprintf(_lastError, ERROR_MAX_LEN, "Failed to create download task");
        LOG_E("OTAUpdater", "xTaskCreatePinnedToCore failed");
        return false;
    }

    LOG_I("OTAUpdater", "Download task started");
    return true;
}

// --- _downloadTaskProc (Story dl-7.1 AC #1–#8, dl-7.2 AC #1–#3, #5, #8, #11) ---
// Bootloader A/B rollback note (AC #4):
// ESP32 Arduino uses stock esp-idf OTA partition scheme (app0/app1 in custom_partitions.csv).
// After esp_ota_set_boot_partition(), the bootloader boots the new slot. If the new image
// fails validation (doesn't call esp_ota_mark_app_valid_cancel_rollback() in time), the
// bootloader automatically rolls back to the previous partition on next boot. No custom
// bootloader is required — stock Arduino-ESP32 partition scheme is sufficient (FR28/NFR12).

void OTAUpdater::_downloadTaskProc(void* param) {
    (void)param;

    // Track whether Update.begin() succeeded so we know if Update.abort() is needed
    bool updateBegun = false;

    // === Pre-OTA: Tear down display mode (AC #2) ===
    // Story dl-7.2: Fail fast if prepareForOTA() fails — do not proceed with download.
    if (!ModeRegistry::prepareForOTA()) {
        snprintf(_lastError, ERROR_MAX_LEN, "Failed to prepare display for OTA");
        _state = OTAState::ERROR;
        _failurePhase = OTAFailurePhase::DOWNLOAD;
        SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                          "Download failed — display prep error");
        LOG_E("OTAUpdater", "prepareForOTA failed — aborting OTA");
        _downloadTaskHandle = nullptr;
        vTaskDelete(NULL);
        return;
    }

    // Report OTA pull starting to SystemStatus
    SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::OK, "Downloading firmware");

    // === Heap check (AC #3) ===
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < OTA_HEAP_THRESHOLD) {
        snprintf(_lastError, ERROR_MAX_LEN,
                 "Not enough memory — restart device and try again");
        _state = OTAState::ERROR;
        _failurePhase = OTAFailurePhase::DOWNLOAD;
        SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                          "Download failed — not enough memory");
        LOG_E("OTAUpdater", "Heap insufficient for OTA");
        ModeRegistry::completeOTAAttempt(false);
        _downloadTaskHandle = nullptr;
        vTaskDelete(NULL);
        return;
    }

    LOG_I("OTAUpdater", "Heap check passed");

    // === Fetch .sha256 file (AC #4) ===
    uint8_t expectedSha[32] = {};
    {
        WiFiClientSecure shaClient;
        shaClient.setInsecure();

        HTTPClient shaHttp;
        shaHttp.begin(shaClient, _sha256AssetUrl);
        shaHttp.setTimeout(15000);
        shaHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

        char userAgent[64];
        snprintf(userAgent, sizeof(userAgent), "FlightWall-OTA/1.0 FW/%s", FW_VERSION);
        shaHttp.addHeader("User-Agent", userAgent);

        int shaCode = shaHttp.GET();
        if (shaCode != 200) {
            snprintf(_lastError, ERROR_MAX_LEN, "Release missing integrity file");
            _state = OTAState::ERROR;
            _failurePhase = OTAFailurePhase::DOWNLOAD;
            SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                              "Download failed — integrity file missing");
            LOG_E("OTAUpdater", "SHA256 file fetch failed");
            shaHttp.end();
            ModeRegistry::completeOTAAttempt(false);
            _downloadTaskHandle = nullptr;
            vTaskDelete(NULL);
            return;
        }

        String shaBody = shaHttp.getString();
        shaHttp.end();

        if (!parseSha256File(shaBody.c_str(), shaBody.length(), expectedSha, sizeof(expectedSha))) {
            snprintf(_lastError, ERROR_MAX_LEN, "Release missing integrity file");
            _state = OTAState::ERROR;
            _failurePhase = OTAFailurePhase::DOWNLOAD;
            SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                              "Download failed — integrity file invalid");
            LOG_E("OTAUpdater", "SHA256 file parse failed");
            ModeRegistry::completeOTAAttempt(false);
            _downloadTaskHandle = nullptr;
            vTaskDelete(NULL);
            return;
        }

        LOG_I("OTAUpdater", "SHA256 digest loaded");
    }

    // === Get OTA partition and begin Update (AC #5) ===
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    if (!partition) {
        snprintf(_lastError, ERROR_MAX_LEN, "No OTA partition available");
        _state = OTAState::ERROR;
        _failurePhase = OTAFailurePhase::DOWNLOAD;
        SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                          "Download failed — no OTA partition");
        LOG_E("OTAUpdater", "No OTA partition");
        ModeRegistry::completeOTAAttempt(false);
        _downloadTaskHandle = nullptr;
        vTaskDelete(NULL);
        return;
    }

    if (!Update.begin(partition->size)) {
        snprintf(_lastError, ERROR_MAX_LEN, "Could not begin OTA update");
        _state = OTAState::ERROR;
        _failurePhase = OTAFailurePhase::DOWNLOAD;
        SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                          "Download failed — could not begin update");
        LOG_E("OTAUpdater", "Update.begin() failed");
        ModeRegistry::completeOTAAttempt(false);
        _downloadTaskHandle = nullptr;
        vTaskDelete(NULL);
        return;
    }
    updateBegun = true;

#if LOG_LEVEL >= 2
    Serial.printf("[OTAUpdater] Writing to partition %s, size 0x%x\n",
                  partition->label, partition->size);
#endif

    // === Initialize SHA-256 context (AC #5) ===
    mbedtls_sha256_context shaCtx;
    mbedtls_sha256_init(&shaCtx);
    mbedtls_sha256_starts_ret(&shaCtx, 0);  // 0 = SHA-256 (not SHA-224)

    // === Stream binary download (AC #5, #6) ===
    WiFiClientSecure binClient;
    binClient.setInsecure();

    HTTPClient binHttp;
    binHttp.begin(binClient, _binaryAssetUrl);
    binHttp.setTimeout(30000);
    binHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    char userAgent[64];
    snprintf(userAgent, sizeof(userAgent), "FlightWall-OTA/1.0 FW/%s", FW_VERSION);
    binHttp.addHeader("User-Agent", userAgent);

    int binCode = binHttp.GET();
    if (binCode != 200) {
        snprintf(_lastError, ERROR_MAX_LEN,
                 "Download failed — tap to retry");
        _state = OTAState::ERROR;
        _failurePhase = OTAFailurePhase::DOWNLOAD;
        SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                          "Download failed — HTTP error");
        Update.abort();
        mbedtls_sha256_free(&shaCtx);
        binHttp.end();
        LOG_E("OTAUpdater", "Binary download HTTP error");
        ModeRegistry::completeOTAAttempt(false);
        _downloadTaskHandle = nullptr;
        vTaskDelete(NULL);
        return;
    }

    int totalSize = binHttp.getSize();  // -1 if chunked encoding
    WiFiClient* stream = binHttp.getStreamPtr();
    if (!stream) {
        snprintf(_lastError, ERROR_MAX_LEN,
                 "Download failed — tap to retry");
        _state = OTAState::ERROR;
        _failurePhase = OTAFailurePhase::DOWNLOAD;
        SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                          "Download failed — no stream");
        Update.abort();
        mbedtls_sha256_free(&shaCtx);
        binHttp.end();
        LOG_E("OTAUpdater", "No stream pointer");
        ModeRegistry::completeOTAAttempt(false);
        _downloadTaskHandle = nullptr;
        vTaskDelete(NULL);
        return;
    }

    // Use partition size as fallback if Content-Length unavailable (chunked encoding)
    size_t progressTotal = (totalSize > 0) ? (size_t)totalSize : partition->size;

    // Stream in bounded chunks (AC #6)
    static constexpr size_t CHUNK_SIZE = 4096;
    uint8_t chunkBuf[CHUNK_SIZE];
    size_t bytesWritten = 0;
    bool downloadOk = true;

    while (binHttp.connected() && (totalSize < 0 || bytesWritten < (size_t)totalSize)) {
        size_t avail = stream->available();
        if (avail == 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        size_t toRead = (avail < CHUNK_SIZE) ? avail : CHUNK_SIZE;
        if (totalSize > 0 && bytesWritten + toRead > (size_t)totalSize) {
            toRead = (size_t)totalSize - bytesWritten;
        }

        int readLen = stream->readBytes(chunkBuf, toRead);
        if (readLen <= 0) {
            // Read error or timeout — WiFi drop or stream failure (AC #2)
            snprintf(_lastError, ERROR_MAX_LEN,
                     "Download failed — tap to retry");
            downloadOk = false;
            break;
        }

        // Write to flash and update SHA in same loop iteration (AC #6, rule 25)
        size_t written = Update.write(chunkBuf, readLen);
        if (written != (size_t)readLen) {
            snprintf(_lastError, ERROR_MAX_LEN,
                     "Download failed — tap to retry");
            downloadOk = false;
            break;
        }

        mbedtls_sha256_update_ret(&shaCtx, chunkBuf, readLen);

        bytesWritten += readLen;

        // Update progress (AC #6)
        _progress = (uint8_t)((bytesWritten * 100) / progressTotal);
        if (_progress > 100) _progress = 100;

        // Yield to other tasks periodically
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    binHttp.end();

    if (!downloadOk) {
        _state = OTAState::ERROR;
        _failurePhase = OTAFailurePhase::DOWNLOAD;
        SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                          "Download failed — tap to retry");
        Update.abort();
        mbedtls_sha256_free(&shaCtx);
        LOG_E("OTAUpdater", "Download failed during stream");
        ModeRegistry::completeOTAAttempt(false);
        _downloadTaskHandle = nullptr;
        vTaskDelete(NULL);
        return;
    }

    // === Verify SHA-256 (AC #7, dl-7.2 AC #3) ===
    _state = OTAState::VERIFYING;
    _progress = 100;
    SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::OK, "Verifying firmware");

    uint8_t computedSha[32];
    mbedtls_sha256_finish_ret(&shaCtx, computedSha);
    mbedtls_sha256_free(&shaCtx);

    if (!compareSha256(computedSha, expectedSha)) {
        snprintf(_lastError, ERROR_MAX_LEN, "Firmware integrity check failed");
        _state = OTAState::ERROR;
        _failurePhase = OTAFailurePhase::VERIFY;
        SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                          "Verify failed — integrity mismatch");
        Update.abort();
        LOG_E("OTAUpdater", "SHA-256 mismatch — aborting");
        ModeRegistry::completeOTAAttempt(false);
        _downloadTaskHandle = nullptr;
        vTaskDelete(NULL);
        return;
    }

    LOG_I("OTAUpdater", "SHA-256 verified OK");

    // === Finalize and reboot (AC #7) ===
    if (!Update.end(true)) {
        snprintf(_lastError, ERROR_MAX_LEN, "Firmware verification failed on finalize");
        _state = OTAState::ERROR;
        _failurePhase = OTAFailurePhase::VERIFY;
        SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::ERROR,
                          "Verify failed — finalize error");
        Update.abort();  // Story dl-7.2, AC #1: abort on every post-begin failure
        LOG_E("OTAUpdater", "Update.end() failed");
        ModeRegistry::completeOTAAttempt(false);
        _downloadTaskHandle = nullptr;
        vTaskDelete(NULL);
        return;
    }

    // Set boot partition to the newly written OTA slot
    esp_ota_set_boot_partition(partition);

    _state = OTAState::REBOOTING;
    _failurePhase = OTAFailurePhase::NONE;
    SystemStatus::set(Subsystem::OTA_PULL, StatusLevel::OK, "Rebooting with new firmware");
    LOG_I("OTAUpdater", "OTA complete — rebooting in 500ms");

    // completeOTAAttempt(true) is a no-op — device reboots
    ModeRegistry::completeOTAAttempt(true);

    // Clear handle before delete (AC #8)
    _downloadTaskHandle = nullptr;

    delay(500);
    ESP.restart();

    // Unreachable
    vTaskDelete(NULL);
}
