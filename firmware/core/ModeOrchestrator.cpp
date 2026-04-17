#include "core/ModeOrchestrator.h"
#include "core/ModeRegistry.h"
#include "utils/Log.h"
#include "utils/TimeUtils.h"
#include <cstring>

// Static member initialization
OrchestratorState ModeOrchestrator::_state = OrchestratorState::MANUAL;
char ModeOrchestrator::_activeModeId[32] = "classic_card";
char ModeOrchestrator::_activeModeName[32] = "Classic Card";
char ModeOrchestrator::_manualModeId[32] = "classic_card";
char ModeOrchestrator::_manualModeName[32] = "Classic Card";
int8_t ModeOrchestrator::_activeRuleIndex = -1;
char ModeOrchestrator::_stateReasonBuf[64] = {0};

// Schedule config cache to avoid NVS reads in hot path (tick called every second)
static ModeScheduleConfig s_cachedSchedule = {};
static bool s_scheduleCacheValid = false;

void ModeOrchestrator::init() {
    _state = OrchestratorState::MANUAL;
    strncpy(_activeModeId, "classic_card", sizeof(_activeModeId) - 1);
    strncpy(_activeModeName, "Classic Card", sizeof(_activeModeName) - 1);
    strncpy(_manualModeId, "classic_card", sizeof(_manualModeId) - 1);
    strncpy(_manualModeName, "Classic Card", sizeof(_manualModeName) - 1);
    _activeRuleIndex = -1;
    _stateReasonBuf[0] = '\0';
    s_scheduleCacheValid = false;  // Invalidate schedule cache on init
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
            // AC #10: return useful state_reason with active rule index + mode id
            if (_activeRuleIndex >= 0) {
                snprintf(_stateReasonBuf, sizeof(_stateReasonBuf),
                         "scheduled rule %d: %s", _activeRuleIndex, _activeModeId);
                return _stateReasonBuf;
            }
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

int8_t ModeOrchestrator::getActiveScheduleRuleIndex() {
    return _activeRuleIndex;
}

void ModeOrchestrator::invalidateScheduleCache() {
    s_scheduleCacheValid = false;
}

void ModeOrchestrator::onManualSwitch(const char* modeId, const char* modeName) {
    // Guard: skip redundant switches to same mode to avoid unnecessary registry work
    if (_state == OrchestratorState::MANUAL && strcmp(_activeModeId, modeId) == 0) {
        LOG_V("ModeOrch", "Manual switch to same mode, skipping");
        return;
    }

    // Story dl-1.3, AC #1: drive ModeRegistry so LED mode actually changes.
    // AC #5: if requestSwitch fails (unknown mode / heap guard), do not update
    // orchestrator state at all — leave state unchanged.
    if (!ModeRegistry::requestSwitch(modeId)) {
        LOG_W("ModeOrch", "Manual switch: requestSwitch failed for mode");
        return;
    }
    // Remember as manual selection for restore after fallback (only after validation)
    strncpy(_manualModeId, modeId, sizeof(_manualModeId) - 1);
    _manualModeId[sizeof(_manualModeId) - 1] = '\0';
    strncpy(_manualModeName, modeName, sizeof(_manualModeName) - 1);
    _manualModeName[sizeof(_manualModeName) - 1] = '\0';
    _state = OrchestratorState::MANUAL;
    _activeRuleIndex = -1;
    strncpy(_activeModeId, modeId, sizeof(_activeModeId) - 1);
    _activeModeId[sizeof(_activeModeId) - 1] = '\0';
    strncpy(_activeModeName, modeName, sizeof(_activeModeName) - 1);
    _activeModeName[sizeof(_activeModeName) - 1] = '\0';
    LOG_I("ModeOrch", "Manual switch");
}

void ModeOrchestrator::onIdleFallback() {
    if (_state == OrchestratorState::IDLE_FALLBACK) return; // already in fallback
    // Story dl-1.2, AC #1: drive ModeRegistry into clock mode.
    // AC #6: safe if clock is already active — requestSwitch is idempotent
    // (ModeRegistry::tick skips switch when requested == active index).
    // AC #2: update orchestrator state only after registry confirms acceptance.
    if (!ModeRegistry::requestSwitch("clock")) {
        LOG_W("ModeOrch", "Failed to request clock mode via registry");
        return;
    }
    _state = OrchestratorState::IDLE_FALLBACK;
    _activeRuleIndex = -1;
    strncpy(_activeModeId, "clock", sizeof(_activeModeId) - 1);
    _activeModeId[sizeof(_activeModeId) - 1] = '\0';
    strncpy(_activeModeName, "Clock", sizeof(_activeModeName) - 1);
    _activeModeName[sizeof(_activeModeName) - 1] = '\0';
    LOG_I("ModeOrch", "Idle fallback activated (zero flights)");
}

void ModeOrchestrator::onFlightsRestored() {
    if (_state != OrchestratorState::IDLE_FALLBACK) return;
    // Story dl-1.2, AC #2: restore owner's saved manual mode via registry.
    // If requestSwitch fails (e.g. mode unregistered), log warning and do not
    // update orchestrator state — keep in IDLE_FALLBACK to maintain consistency
    // with registry truth. Registry error will surface on next GET.
    if (!ModeRegistry::requestSwitch(_manualModeId)) {
        LOG_W("ModeOrch", "Failed to restore manual mode via registry");
        return;
    }
    _state = OrchestratorState::MANUAL;
    _activeRuleIndex = -1;
    // Restore previous manual selection
    strncpy(_activeModeId, _manualModeId, sizeof(_activeModeId) - 1);
    _activeModeId[sizeof(_activeModeId) - 1] = '\0';
    strncpy(_activeModeName, _manualModeName, sizeof(_activeModeName) - 1);
    _activeModeName[sizeof(_activeModeName) - 1] = '\0';
    LOG_I("ModeOrch", "Flights restored, back to MANUAL");
}

void ModeOrchestrator::tick(uint8_t flightCount, bool ntpSynced, uint16_t nowMinutes) {
    // Story dl-4.1: Evaluate mode schedule rules when NTP is synced.
    // Schedule evaluation runs BEFORE idle fallback logic so SCHEDULED
    // takes priority over IDLE_FALLBACK (AC #6).

    if (ntpSynced) {
        // Use cached schedule if valid, reload from NVS on first call or after invalidation
        if (!s_scheduleCacheValid) {
            s_cachedSchedule = ConfigManager::getModeSchedule();
            s_scheduleCacheValid = true;
        }
        ModeScheduleConfig sched = s_cachedSchedule;

        // Find the first (lowest index) enabled rule whose window matches now (AC #5)
        int8_t matchIndex = -1;
        for (uint8_t i = 0; i < sched.ruleCount; i++) {
            const ScheduleRule& r = sched.rules[i];
            if (r.enabled == 1 && minutesInWindow(nowMinutes, r.startMin, r.endMin)) {
                matchIndex = (int8_t)i;
                break;
            }
        }

        if (matchIndex >= 0) {
            // A schedule rule matches — enter or stay in SCHEDULED state
            const ScheduleRule& matched = sched.rules[matchIndex];

            if (_state != OrchestratorState::SCHEDULED || _activeRuleIndex != matchIndex) {
                // Entering SCHEDULED or switching to a different rule
                // AC #9: only issue requestSwitch from tick (or onManualSwitch)
                if (!ModeRegistry::requestSwitch(matched.modeId)) {
                    LOG_W("ModeOrch", "Schedule: requestSwitch failed for rule mode");
                    return;
                }
                _state = OrchestratorState::SCHEDULED;
                _activeRuleIndex = matchIndex;
                strncpy(_activeModeId, matched.modeId, sizeof(_activeModeId) - 1);
                _activeModeId[sizeof(_activeModeId) - 1] = '\0';
                // Update active mode display name from mode table
                const ModeEntry* table = ModeRegistry::getModeTable();
                uint8_t count = ModeRegistry::getModeCount();
                for (uint8_t i = 0; i < count; i++) {
                    if (strcmp(table[i].id, matched.modeId) == 0) {
                        strncpy(_activeModeName, table[i].displayName, sizeof(_activeModeName) - 1);
                        _activeModeName[sizeof(_activeModeName) - 1] = '\0';
                        break;
                    }
                }
                LOG_I("ModeOrch", "Schedule rule activated");
            }
            // AC #6: SCHEDULED + flightCount == 0 → do NOT invoke idle fallback
            return;
        } else if (_state == OrchestratorState::SCHEDULED) {
            // AC #4: no rule matches and was SCHEDULED → transition to MANUAL
            if (!ModeRegistry::requestSwitch(_manualModeId)) {
                LOG_W("ModeOrch", "Schedule exit: requestSwitch failed for manual mode");
                return;
            }
            _state = OrchestratorState::MANUAL;
            _activeRuleIndex = -1;
            // Restore owner's manual selection (both ID and name)
            strncpy(_activeModeId, _manualModeId, sizeof(_activeModeId) - 1);
            _activeModeId[sizeof(_activeModeId) - 1] = '\0';
            strncpy(_activeModeName, _manualModeName, sizeof(_activeModeName) - 1);
            _activeModeName[sizeof(_activeModeName) - 1] = '\0';
            LOG_I("ModeOrch", "Schedule window ended, back to MANUAL");
            // Fall through to idle fallback logic below in case flightCount is 0
        }
    }

    // State transition logic per architecture.md#DL2
    // AC #6: SCHEDULED state is handled above and returns early — never reaches here
    if (flightCount == 0 && _state == OrchestratorState::MANUAL) {
        onIdleFallback();
    } else if (flightCount > 0 && _state == OrchestratorState::IDLE_FALLBACK) {
        onFlightsRestored();
    }
}
