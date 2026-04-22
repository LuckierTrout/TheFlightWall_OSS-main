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
    SWITCHING,
    FAILED  // BF-1 AC #5: REQUESTED-stall watchdog terminal state
};

// BF-1 AC #5: budget for REQUESTED → SWITCHING progress, in ms. If the dashboard
// polls /api/display/mode/status after this budget without observing progress, the
// status getter advances state to FAILED with code REQUESTED_STALL. Insurance
// against future variants of "tick() never runs" (the auto-yield in main.cpp
// already removes the only known cause).
//
// Bumped from 500ms → 5000ms (2026-04-20) after observing real switches take
// 3-5s in practice (widget-heavy custom_layout teardown + init + fade), which
// was causing spurious "stalled" reports on the dashboard even though the
// switch was still in-flight and completing successfully.
static constexpr uint32_t kRequestedStallLimitMs = 5000;

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

    /// Force a full teardown → init cycle for the currently active mode (e.g.,
    /// to reload a layout whose backing file changed without a mode change).
    /// Unlike requestSwitch(), this bypasses the same-mode idempotency guard
    /// in tick() by temporarily clearing _activeModeIndex so the request is
    /// treated as a genuine switch. Safe to call from any core (atomic write).
    /// Returns false if modeId is unknown or the registry is uninitialised.
    /// LE-1.3 AC #7: used by displayTask when layout_active NVS key changes.
    static bool requestForceReload(const char* modeId);

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

    /// BF-1 AC #1, #2: cross-core check used by the Core-0 display task
    /// auto-yield path. Returns true while a mode-switch request is queued but
    /// not yet processed (state == REQUESTED). Atomic load — safe from any core.
    static bool hasPendingRequest();

    /// BF-1 AC #3: id of the mode currently sitting in REQUESTED state, or
    /// nullptr if no pending request / index out of range. Used to name the
    /// preemption log line in main.cpp.
    static const char* getRequestedModeId();

    /// BF-1 AC #3: called from the Core-0 display task immediately before
    /// yielding a test pattern in favor of an incoming mode switch. Source must
    /// be a string literal ("calibration" or "positioning") — stored as an
    /// atomic pointer, no copy. Cleared on the next requestSwitch().
    static void markPreempted(const char* source);

    /// BF-1 AC #4: source of the most recent test-pattern preemption, or
    /// nullptr if the last transition was a plain mode switch. Cleared when a
    /// fresh requestSwitch() arrives.
    static const char* getLastPreemptionSource();

    /// BF-1 AC #5: lazy stall watchdog. Call from /api/display/mode/status
    /// before reading switch state. If state has been REQUESTED for longer
    /// than kRequestedStallLimitMs, advances state to FAILED with code
    /// REQUESTED_STALL, logs once at LOG_W, and clears the pending request so
    /// the dashboard stops polling. The previous active mode index is
    /// untouched (executeSwitch never ran), so getActiveModeId() continues
    /// to return the prior mode.
    static void pollAndAdvanceStall();

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

    // BF-1: stall watchdog + preemption tracking
    static std::atomic<uint32_t> _requestedAtMs;       // millis() when latest requestSwitch fired
    static std::atomic<const char*> _preemptionSource; // literal "calibration"|"positioning"|nullptr
    static std::atomic<bool> _stallReported;           // debounce: log REQUESTED_STALL once per stall

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
