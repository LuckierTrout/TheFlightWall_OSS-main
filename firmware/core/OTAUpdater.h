#pragma once
/*
Purpose: Check GitHub Releases API for newer firmware versions and download OTA updates.
Responsibilities:
- HTTPS GET the latest release from the configured GitHub repo.
- Parse tag_name, release notes, and binary/sha256 asset URLs.
- Compare remote version against compile-time FW_VERSION.
- Download firmware binary with incremental SHA-256 verification (Story dl-7.1).
- Handle failures safely: Update.abort() on every post-begin error, restore display (dl-7.2).
- Surface state, error messages, progress, failure phase, and parsed fields for consumers.
Inputs: GitHub owner/repo strings (set once via init()).
Outputs: OTAState, remote version, release notes, asset URLs, error messages, download progress.

Bootloader A/B Rollback (FR28/NFR12, Story dl-7.2 AC #4):
  ESP32 Arduino uses stock esp-idf OTA partition scheme (app0/app1 in custom_partitions.csv).
  After esp_ota_set_boot_partition(), the bootloader boots the new slot. If the new image
  fails boot validation (doesn't call esp_ota_mark_app_valid_cancel_rollback() within the
  timeout), the bootloader automatically rolls back to the previous partition on next boot.
  No custom bootloader is required — the stock Arduino-ESP32 partition scheme is sufficient.
  Rollback detection is handled by main.cpp detectRollback() / performOtaSelfCheck().
*/

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// OTA state machine — all states defined per epic scope.
// DOWNLOADING, VERIFYING, REBOOTING are reserved for dl-7; unused in dl-6.1.
enum class OTAState : uint8_t {
    IDLE,
    CHECKING,
    AVAILABLE,
    DOWNLOADING,
    VERIFYING,
    REBOOTING,
    ERROR
};

// Story dl-7.2, AC #11: Failure phase for structured error reporting.
// Consumed by dl-7.3 GET /api/ota/status for machine-readable failure context.
enum class OTAFailurePhase : uint8_t {
    NONE,       // No failure
    DOWNLOAD,   // HTTP read error, WiFi drop, timeout, write mismatch
    VERIFY,     // SHA-256 mismatch, Update.end() failure
    BOOT        // Post-reboot rollback detection (reserved for future use)
};

class OTAUpdater {
public:
    // Initialise with GitHub owner/repo (bounded copies).
    // Max 59 chars each per GitHub username/repo limits with safe margin.
    // Idempotent — asserts single init in debug builds.
    static void init(const char* repoOwner, const char* repoName);

    // Perform HTTPS GET to GitHub releases/latest, parse response,
    // and compare tag_name against FW_VERSION.
    // Returns true if a newer version is available (state → AVAILABLE).
    // Returns false on up-to-date, error, or network failure (state → IDLE or ERROR).
    static bool checkForUpdate();

    // --- Accessors ---
    static OTAState    getState();
    static const char* getStateString();     // Lowercase state name for JSON (dl-7.3 AC #3)
    static const char* getLastError();       // null-terminated, human-readable
    static const char* getRemoteVersion();   // normalised tag (no leading 'v')
    static const char* getReleaseNotes();    // truncated to NOTES_MAX_LEN
    static const char* getBinaryAssetUrl();  // first .bin asset URL (or "")
    static const char* getSha256AssetUrl();  // first .sha256 asset URL (or "")

    // --- OTA Download (Story dl-7.1) ---

    // Start downloading firmware binary in a background FreeRTOS task.
    // Requires state == AVAILABLE (URLs populated by checkForUpdate()).
    // Returns true if download task spawned; false if already downloading or preconditions unmet.
    static bool startDownload();

    // Download progress 0–100 (updated incrementally during download).
    static uint8_t getProgress();

    // --- Structured failure accessors (Story dl-7.2, AC #11) ---
    // Consumed by dl-7.3 GET /api/ota/status JSON shape:
    //   { "state": "error", "progress": 75, "error": "...",
    //     "failure_phase": "download"|"verify"|"boot"|"none",
    //     "retriable": true|false }

    // Phase where the last failure occurred (NONE if no failure).
    static OTAFailurePhase getFailurePhase();
    static const char* getFailurePhaseString();  // Lowercase phase name for JSON (dl-7.3 AC #3)

    // Whether the last failure is retriable (device unchanged, safe to call startDownload again).
    // Download and verify failures are retriable; boot failures are not.
    static bool isRetriable();

    // --- Testable helpers (public for unit testing, AC #9, #10) ---

    // Normalise a version tag: strip leading 'v'/'V', trim whitespace.
    // Result written to outBuf (outBufSize must be >= strlen(tag)+1).
    // Returns false if tag is null/empty.
    static bool normalizeVersion(const char* tag, char* outBuf, size_t outBufSize);

    // Compare two normalised version strings.
    // Returns true if remoteVersion differs from localVersion (i.e. update available).
    static bool compareVersions(const char* remoteNormalized, const char* localNormalized);

    // Parse a GitHub releases/latest JSON payload.
    // Extracts tag_name, body, first .bin asset URL, first .sha256 asset URL.
    // Returns true on success (fields populated), false on parse error.
    static bool parseReleaseJson(const char* json, size_t jsonLen);

    // Parse a .sha256 file body: extract 64 hex chars, decode to 32-byte binary digest.
    // Handles leading/trailing whitespace and optional "  filename" suffix.
    // Returns true on success (outDigest filled with 32 bytes).
    static bool parseSha256File(const char* body, size_t bodyLen,
                                uint8_t* outDigest, size_t outDigestSize);

    // Constant-time compare of two 32-byte SHA-256 digests.
    // Returns true if they match.
    static bool compareSha256(const uint8_t* a, const uint8_t* b);

    // Reset internal state for testing. Clears _initialized flag to allow re-init.
    // WARNING: Only for unit tests — do not use in production code.
    static void _resetForTest();

    // Minimum free heap required before starting OTA download (80 KB per FR34/NFR16).
    static constexpr uint32_t OTA_HEAP_THRESHOLD = 81920;

    // Buffer sizes
    static constexpr size_t OWNER_MAX_LEN   = 60;
    static constexpr size_t REPO_MAX_LEN    = 60;
    static constexpr size_t VERSION_MAX_LEN = 32;
    static constexpr size_t ERROR_MAX_LEN   = 128;
    static constexpr size_t NOTES_MAX_LEN   = 512;
    static constexpr size_t URL_MAX_LEN     = 256;

private:
    static bool        _initialized;
    static OTAState    _state;
    static char        _repoOwner[OWNER_MAX_LEN];
    static char        _repoName[REPO_MAX_LEN];
    static char        _remoteVersion[VERSION_MAX_LEN];
    static char        _lastError[ERROR_MAX_LEN];
    static char        _releaseNotes[NOTES_MAX_LEN];
    static char        _binaryAssetUrl[URL_MAX_LEN];
    static char        _sha256AssetUrl[URL_MAX_LEN];

    // Download task state (Story dl-7.1)
    static TaskHandle_t _downloadTaskHandle;
    static uint8_t      _progress;  // 0–100

    // Failure metadata (Story dl-7.2, AC #11)
    static OTAFailurePhase _failurePhase;

    // FreeRTOS task procedure for OTA download (pinned to Core 1)
    static void _downloadTaskProc(void* param);

    // Stream-based JSON parsing (internal helper for bounded heap usage)
    static bool parseReleaseJsonStream(Stream& stream);
};
