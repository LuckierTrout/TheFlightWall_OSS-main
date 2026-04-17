#pragma once
/*
Purpose: Orchestrates display mode switching with state machine logic.
Responsibilities:
- Track orchestrator state (MANUAL, IDLE_FALLBACK, SCHEDULED)
- Provide state and reason strings for API/UI consumption
- Manage transitions between states based on flight availability
- Evaluate time-based mode schedule rules (Story dl-4.1)
Architecture: Static class, called from Core 1 only (no atomics needed).
References: architecture.md#DL2
*/

#include <Arduino.h>
#include "core/ConfigManager.h"

enum class OrchestratorState : uint8_t {
    MANUAL = 0,
    IDLE_FALLBACK = 1,
    SCHEDULED = 2
};

class ModeOrchestrator {
public:
    /// Initialize orchestrator (call once during setup)
    static void init();

    /// Get current orchestrator state enum
    static OrchestratorState getState();

    /// Get human-readable reason string for current state
    static const char* getStateReason();

    /// Get API-friendly state string ("manual", "idle_fallback", "scheduled")
    static const char* getStateString();

    /// Get the currently active mode ID (e.g., "classic_card", "clock")
    static const char* getActiveModeId();

    /// Get the display name of the currently active mode
    static const char* getActiveModeName();

    /// Get the saved manual mode ID (for restore after fallback)
    static const char* getManualModeId();

    /// Called when user manually selects a mode via API
    static void onManualSwitch(const char* modeId, const char* modeName);

    /// Called when idle fallback triggers (zero flights)
    static void onIdleFallback();

    /// Called when flights return after idle fallback
    static void onFlightsRestored();

    /// Get active schedule rule index (-1 = no active rule, 0–7 = active rule index)
    static int8_t getActiveScheduleRuleIndex();

    /// Invalidate schedule cache (call after mode schedule config changes)
    static void invalidateScheduleCache();

    /// Tick function — evaluates state transitions (called from Core 1)
    /// ntpSynced: whether NTP time is available
    /// nowMinutes: current minutes since midnight (0–1439), only valid when ntpSynced
    static void tick(uint8_t flightCount, bool ntpSynced = false, uint16_t nowMinutes = 0);

private:
    static OrchestratorState _state;
    static char _activeModeId[32];
    static char _activeModeName[32];
    static char _manualModeId[32];    // remembers manual selection during fallback
    static char _manualModeName[32];
    static int8_t _activeRuleIndex;   // -1 = no active rule, 0–7 = active rule index
    static char _stateReasonBuf[64];  // buffer for dynamic state reason string
};
