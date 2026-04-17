#pragma once
/*
Purpose: Static mode registry with cooperative switch serialization.
Responsibilities:
- Manage a static table of display modes registered at compile time.
- Coordinate mode switching on Core 0 (display task) via tick().
- Provide cross-core safe requestSwitch() via atomic index.
- Debounce NVS persistence of active mode after switch.
Architecture: Decision D2 — teardown-before-delete pattern, heap guard, latest-wins.
References: architecture.md#D2, interfaces/DisplayMode.h
*/

#include <atomic>
#include <cstdint>
#include <vector>
#include "interfaces/DisplayMode.h"

// Heap margin required above mode's memory requirement for safe switch
static constexpr uint32_t MODE_SWITCH_HEAP_MARGIN = 30 * 1024;

// Sentinel value: no pending switch request
static constexpr uint8_t MODE_INDEX_NONE = 0xFF;

enum class SwitchState : uint8_t {
    IDLE,
    REQUESTED,
    SWITCHING
};

struct ModeEntry {
    const char* id;                           // e.g., "classic_card"
    const char* displayName;                  // e.g., "Classic Card"
    DisplayMode* (*factory)();                // factory function returning new instance
    uint32_t (*memoryRequirement)();          // static function — no instance needed
    uint8_t priority;                         // display order in Mode Picker
    const ModeZoneDescriptor* zoneDescriptor; // static zone metadata for API/UI (nullable)
    const ModeSettingsSchema* settingsSchema; // per-mode settings schema (nullable, Story dl-1.1 AC #11)
};

class ModeRegistry {
public:
    /// Initialize with a static mode table. Call once during setup().
    static void init(const ModeEntry* table, uint8_t count);

    /// Request a mode switch by ID. Safe to call from any core (atomic write).
    /// Returns false if modeId is unknown; sets _lastError.
    static bool requestSwitch(const char* modeId);

    /// Cooperative tick — processes pending switch on Core 0. Call from displayTask.
    static void tick(const RenderContext& ctx,
                     const std::vector<FlightInfo>& flights);

    /// Get the currently active mode instance (nullptr before first tick with a switch).
    static DisplayMode* getActiveMode();

    /// Get the mode ID of the active mode (nullptr if none).
    static const char* getActiveModeId();

    /// Expose the static mode table for enumeration.
    static const ModeEntry* getModeTable();

    /// Number of entries in the mode table.
    static uint8_t getModeCount();

    /// Current switch state.
    static SwitchState getSwitchState();

    /// Last error message (fixed char buffer, no heap).
    /// NOTE: Returns raw pointer to mutable buffer — safe for single-threaded
    /// test code. Prefer copyLastError() for cross-core (Core 1) callers.
    static const char* getLastError();

    /// Thread-safer snapshot of the last error string into caller-supplied buffer.
    /// Double-checks _errorUpdating before and after copy; discards result if a
    /// write was in progress on Core 0. outBuf is always null-terminated.
    static void copyLastError(char* outBuf, size_t maxLen);

    /// Stable machine-readable code for the last error (nullptr when no error).
    /// Points to a string literal — safe to read across cores without locking.
    static const char* getLastErrorCode();

    /// Prepare system for OTA flash write (Story dl-7.1, AC #2, #9).
    /// Tears down the active DisplayMode, deletes it, sets SwitchState::SWITCHING
    /// so tick() skips normal mode logic, and sets _otaMode flag.
    /// Call from Core 1 OTA task or upload handler BEFORE Update.begin().
    /// Returns true on success; false if no mode table or already in OTA mode.
    static bool prepareForOTA();

    /// Whether OTA mode is active (display task should show "Updating…").
    static bool isOtaMode();

    /// Complete an OTA attempt and restore normal operation (Story dl-7.2, AC #5).
    /// On failure (success=false): clears OTA mode, resets switch state to IDLE,
    /// and requests the default mode so flight rendering resumes without power cycle.
    /// On success (success=true): no-op before reboot.
    static void completeOTAAttempt(bool success);

    /// Zero-allocation callback enumeration (AC #4).
    /// Iterates all entries calling cb with (id, displayName, index, user).
    static void enumerate(void (*cb)(const char* id, const char* displayName,
                                     uint8_t index, void* user), void* user);

private:
    static const ModeEntry* _table;
    static uint8_t _count;
    static DisplayMode* _activeMode;
    static std::atomic<uint8_t> _activeModeIndex;  // written Core 0 (executeSwitch/prepareForOTA), read Core 1 (getActiveModeId)
    static std::atomic<uint8_t> _requestedIndex;   // cross-core: Core 1 writes, Core 0 reads
    static std::atomic<SwitchState> _switchState;  // atomic: read by Core 1 (WebPortal); written by Core 1 (requestSwitch→REQUESTED) and Core 0 (tick/executeSwitch→IDLE/SWITCHING)
    static char _lastError[64];
    static std::atomic<const char*> _lastErrorCode; // points to string literal — atomic pointer, no buffer race
    static std::atomic<bool> _errorUpdating;        // guard: true while _lastError is being written on Core 0
    static bool _nvsWritePending;
    static unsigned long _lastSwitchMs;
    static std::atomic<bool> _otaMode;  // Story dl-7.1: true after prepareForOTA() succeeds (atomic: Core 1 writes, Core 0 reads)
    static bool _recoveryQueued;  // ds-3.2 AC #5: prevents infinite retry if table[0] also fails

    /// Find table index for a mode ID. Returns MODE_INDEX_NONE if not found.
    static uint8_t findIndex(const char* modeId);

    /// Execute the switch sequence per D2.
    /// Story dl-3.1: flights param added for fade transition (first frame of new mode).
    static void executeSwitch(uint8_t targetIndex, const RenderContext& ctx,
                              const std::vector<FlightInfo>& flights);

    /// Persist mode to NVS if debounce period elapsed.
    static void tickNvsPersist();

    // Story dl-3.1: Fade transition logic is integrated into executeSwitch().
    // Linear crossfade runs ~15 steps over ≤1s on Core 0. If malloc fails for
    // either RGB888 buffer, performs instant handoff (no crash). SWITCHING state
    // is held during fade so orchestrator/HTTP paths remain coherent.
    // NOTE: Multiple FastLED.show() calls during transition — explicit exception
    // to FR35 single-show-per-frame rule (AC #9).
};
