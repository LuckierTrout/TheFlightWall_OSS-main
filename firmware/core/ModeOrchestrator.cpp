#include "core/ModeOrchestrator.h"
#include "core/ModeRegistry.h"
#include "utils/Log.h"
#include <cstring>

// Static member initialization
OrchestratorState ModeOrchestrator::_state = OrchestratorState::MANUAL;
char ModeOrchestrator::_activeModeId[32] = "classic_card";
char ModeOrchestrator::_activeModeName[32] = "Classic Card";
char ModeOrchestrator::_manualModeId[32] = "classic_card";
char ModeOrchestrator::_manualModeName[32] = "Classic Card";

void ModeOrchestrator::init() {
    _state = OrchestratorState::MANUAL;
    strncpy(_activeModeId, "classic_card", sizeof(_activeModeId) - 1);
    strncpy(_activeModeName, "Classic Card", sizeof(_activeModeName) - 1);
    strncpy(_manualModeId, "classic_card", sizeof(_manualModeId) - 1);
    strncpy(_manualModeName, "Classic Card", sizeof(_manualModeName) - 1);
    LOG_I("ModeOrch", "Initialized in MANUAL state");
}

OrchestratorState ModeOrchestrator::getState() {
    return _state;
}

const char* ModeOrchestrator::getStateReason() {
    switch (_state) {
        case OrchestratorState::MANUAL:
            return "manual";
        case OrchestratorState::IDLE_FALLBACK:
            return "idle fallback \xe2\x80\x94 no flights";
        case OrchestratorState::SCHEDULED:
            return "scheduled";
        default:
            return "unknown";
    }
}

const char* ModeOrchestrator::getStateString() {
    switch (_state) {
        case OrchestratorState::MANUAL:
            return "manual";
        case OrchestratorState::IDLE_FALLBACK:
            return "idle_fallback";
        case OrchestratorState::SCHEDULED:
            return "scheduled";
        default:
            return "unknown";
    }
}

const char* ModeOrchestrator::getActiveModeId() {
    return _activeModeId;
}

const char* ModeOrchestrator::getActiveModeName() {
    return _activeModeName;
}

const char* ModeOrchestrator::getManualModeId() {
    return _manualModeId;
}

void ModeOrchestrator::onManualSwitch(const char* modeId, const char* modeName) {
    _state = OrchestratorState::MANUAL;
    strncpy(_activeModeId, modeId, sizeof(_activeModeId) - 1);
    _activeModeId[sizeof(_activeModeId) - 1] = '\0';
    strncpy(_activeModeName, modeName, sizeof(_activeModeName) - 1);
    _activeModeName[sizeof(_activeModeName) - 1] = '\0';
    // Remember as manual selection for restore after fallback
    strncpy(_manualModeId, modeId, sizeof(_manualModeId) - 1);
    _manualModeId[sizeof(_manualModeId) - 1] = '\0';
    strncpy(_manualModeName, modeName, sizeof(_manualModeName) - 1);
    _manualModeName[sizeof(_manualModeName) - 1] = '\0';
    // Story dl-1.3, AC #1: drive ModeRegistry so LED mode actually changes.
    // AC #5: if requestSwitch fails (unknown mode / heap guard), leave
    // _activeModeId aligned with what the registry reports after tick.
    if (!ModeRegistry::requestSwitch(modeId)) {
        LOG_W("ModeOrch", "Manual switch: requestSwitch failed for mode");
    }
    LOG_I("ModeOrch", "Manual switch");
}

void ModeOrchestrator::onIdleFallback() {
    if (_state == OrchestratorState::IDLE_FALLBACK) return; // already in fallback
    _state = OrchestratorState::IDLE_FALLBACK;
    strncpy(_activeModeId, "clock", sizeof(_activeModeId) - 1);
    _activeModeId[sizeof(_activeModeId) - 1] = '\0';
    strncpy(_activeModeName, "Clock", sizeof(_activeModeName) - 1);
    _activeModeName[sizeof(_activeModeName) - 1] = '\0';
    // Story dl-1.2, AC #1: drive ModeRegistry into clock mode.
    // AC #6: safe if clock is already active — requestSwitch is idempotent
    // (ModeRegistry::tick skips switch when requested == active index).
    if (!ModeRegistry::requestSwitch("clock")) {
        LOG_W("ModeOrch", "Failed to request clock mode via registry");
    }
    LOG_I("ModeOrch", "Idle fallback activated (zero flights)");
}

void ModeOrchestrator::onFlightsRestored() {
    if (_state != OrchestratorState::IDLE_FALLBACK) return;
    _state = OrchestratorState::MANUAL;
    // Restore previous manual selection
    strncpy(_activeModeId, _manualModeId, sizeof(_activeModeId) - 1);
    _activeModeId[sizeof(_activeModeId) - 1] = '\0';
    strncpy(_activeModeName, _manualModeName, sizeof(_activeModeName) - 1);
    _activeModeName[sizeof(_activeModeName) - 1] = '\0';
    // Story dl-1.2, AC #2: restore owner's saved manual mode via registry.
    // If requestSwitch fails (e.g. mode unregistered), log warning and keep
    // orchestrator state consistent — leave in MANUAL but registry_error
    // will surface on next GET (reconciliation documented here).
    if (!ModeRegistry::requestSwitch(_manualModeId)) {
        LOG_W("ModeOrch", "Failed to restore manual mode via registry");
    }
    LOG_I("ModeOrch", "Flights restored, back to MANUAL");
}

void ModeOrchestrator::tick(uint8_t flightCount) {
    // State transition logic per architecture.md#DL2
    if (flightCount == 0 && _state == OrchestratorState::MANUAL) {
        onIdleFallback();
    } else if (flightCount > 0 && _state == OrchestratorState::IDLE_FALLBACK) {
        onFlightsRestored();
    }
}
