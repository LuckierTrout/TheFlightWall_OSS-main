/*
Purpose: ModeRegistry implementation — static table with cooperative switch serialization.
Architecture: Decision D2 — teardown-before-delete, heap guard, latest-wins atomic,
              NVS debounce via ConfigManager::setDisplayMode().
              Story dl-3.1: linear crossfade transition between display modes.
References: architecture.md#D2, core/ModeRegistry.h
*/

#include "core/ModeRegistry.h"
#include "core/ConfigManager.h"
#include "core/ModeOrchestrator.h"
#include "utils/Log.h"
#include <Arduino.h>
#include <cstring>
// HW-1.1: FastLED removed from lib_deps. The crossfade transition below
// depended on direct CRGB buffer access through FastLED.leds() + FastLED.show().
// The fade path is stubbed to instant-handoff until HW-1.2 re-implements
// transitions against the HUB75 double-buffered DMA canvas. Search for
// "HW-1.2 fade placeholder" below for the disabled blocks.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_task_wdt.h"

// Static member initialization
const ModeEntry* ModeRegistry::_table = nullptr;
uint8_t ModeRegistry::_count = 0;
DisplayMode* ModeRegistry::_activeMode = nullptr;
std::atomic<uint8_t> ModeRegistry::_activeModeIndex(MODE_INDEX_NONE);
std::atomic<uint8_t> ModeRegistry::_requestedIndex(MODE_INDEX_NONE);
std::atomic<SwitchState> ModeRegistry::_switchState(SwitchState::IDLE);
char ModeRegistry::_lastError[64] = {0};
std::atomic<const char*> ModeRegistry::_lastErrorCode(nullptr);
std::atomic<bool> ModeRegistry::_errorUpdating(false);
bool ModeRegistry::_nvsWritePending = false;
unsigned long ModeRegistry::_lastSwitchMs = 0;
std::atomic<bool> ModeRegistry::_otaMode(false);
bool ModeRegistry::_recoveryQueued = false;
std::atomic<uint32_t> ModeRegistry::_requestedAtMs(0);
std::atomic<const char*> ModeRegistry::_preemptionSource(nullptr);
std::atomic<bool> ModeRegistry::_stallReported(false);

void ModeRegistry::init(const ModeEntry* table, uint8_t count) {
    _table = table;
    _count = count;
    _activeMode = nullptr;
    _activeModeIndex = MODE_INDEX_NONE;
    _requestedIndex.store(MODE_INDEX_NONE);
    _switchState.store(SwitchState::IDLE);
    _lastError[0] = '\0';
    _lastErrorCode.store(nullptr);
    _errorUpdating.store(false);
    _nvsWritePending = false;
    _lastSwitchMs = 0;
    _otaMode.store(false);
    _requestedAtMs.store(0);
    _preemptionSource.store(nullptr);
    _stallReported.store(false);

    LOG_I("ModeRegistry", "Initialized");
#if LOG_LEVEL >= 2
    Serial.printf("[ModeRegistry] Mode count: %d\n", (int)count);
#endif
}

uint8_t ModeRegistry::findIndex(const char* modeId) {
    if (modeId == nullptr || _table == nullptr) return MODE_INDEX_NONE;
    for (uint8_t i = 0; i < _count; i++) {
        if (strcmp(_table[i].id, modeId) == 0) return i;
    }
    return MODE_INDEX_NONE;
}

bool ModeRegistry::requestSwitch(const char* modeId) {
    uint8_t idx = findIndex(modeId);
    if (idx == MODE_INDEX_NONE) {
        _lastErrorCode.store("UNKNOWN_MODE");
        _errorUpdating.store(true);
        snprintf(_lastError, sizeof(_lastError), "Unknown mode: %.40s",
                 modeId ? modeId : "(null)");
        _errorUpdating.store(false);
        LOG_W("ModeRegistry", "Switch request rejected");
#if LOG_LEVEL >= 1
        Serial.printf("[ModeRegistry] %s\n", _lastError);
#endif
        return false;
    }

    // Latest-wins: atomic store overwrites any pending request (AC #6, #7)
    _requestedIndex.store(idx);
    // Signal REQUESTED immediately so Core-1 HTTP callers read truthful state in
    // the POST response before tick() runs on Core 0 (ds-3.1 AC #7 async strategy).
    // _switchState is atomic — safe to write from Core 1 here.
    _switchState.store(SwitchState::REQUESTED);
    // BF-1: reset preemption + stall tracking for this fresh request.
    _requestedAtMs.store((uint32_t)millis());
    _preemptionSource.store(nullptr);
    _stallReported.store(false);
    LOG_I("ModeRegistry", "Switch requested");
#if LOG_LEVEL >= 2
    Serial.printf("[ModeRegistry] Target: %s (index %d)\n", modeId, (int)idx);
#endif
    return true;
}

bool ModeRegistry::requestForceReload(const char* modeId) {
    uint8_t idx = findIndex(modeId);
    if (idx == MODE_INDEX_NONE) {
        _lastErrorCode.store("UNKNOWN_MODE");
        _errorUpdating.store(true);
        snprintf(_lastError, sizeof(_lastError), "Unknown mode: %.40s",
                 modeId ? modeId : "(null)");
        _errorUpdating.store(false);
        LOG_W("ModeRegistry", "Force-reload request rejected — unknown mode");
        return false;
    }

    // Bypass the same-mode idempotency guard in tick() by clearing
    // _activeModeIndex BEFORE storing the request. tick() on Core 0 reads
    // _activeModeIndex AFTER consuming _requestedIndex, so a race where Core 0
    // observes MODE_INDEX_NONE → idx is benign: it triggers executeSwitch()
    // exactly as intended. The atomic stores provide the required visibility.
    _activeModeIndex.store(MODE_INDEX_NONE);
    _requestedIndex.store(idx);
    _switchState.store(SwitchState::REQUESTED);
    // BF-1: same fresh-request semantics as requestSwitch().
    _requestedAtMs.store((uint32_t)millis());
    _preemptionSource.store(nullptr);
    _stallReported.store(false);
    LOG_I("ModeRegistry", "Force-reload requested");
#if LOG_LEVEL >= 2
    Serial.printf("[ModeRegistry] Force-reload target: %s (index %d)\n", modeId, (int)idx);
#endif
    return true;
}

void ModeRegistry::executeSwitch(uint8_t targetIndex, const RenderContext& ctx,
                                 const std::vector<FlightInfo>& flights) {
    _switchState.store(SwitchState::SWITCHING);

    const ModeEntry& target = _table[targetIndex];
    const ModeEntry* prevEntry = (_activeModeIndex != MODE_INDEX_NONE)
                                 ? &_table[_activeModeIndex] : nullptr;
    uint8_t prevIndex = _activeModeIndex;

    // Story dl-3.1, AC #5: Snapshot outgoing LED frame BEFORE teardown.
    // Teardown may clear backing drawable state, so we capture the CRGB buffer
    // while the old mode's last rendered frame is still in the FastLED leds array.
    // Snapshot point: after last render, before teardown — documented per AC #5.
    uint8_t* fadeSnapshotBuf = nullptr;
    size_t fadePixelCount = 0;
    size_t fadeBufBytes = 0;

    // HW-1.2 fade placeholder: outgoing CRGB snapshot capture was here.
    // Depends on FastLED.leds() / FastLED.size(), which are not available
    // under the HUB75 stack. Leave fadeSnapshotBuf = nullptr so the fade
    // loop below (also stubbed) falls through to instant handoff.

    // Step 1: Teardown active mode (keep shell allocated for safe restoration)
    if (_activeMode != nullptr) {
        _activeMode->teardown();
        LOG_I("ModeRegistry", "Teardown completed");
    }

    // Step 2: Measure free heap
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t required = target.memoryRequirement() + MODE_SWITCH_HEAP_MARGIN;

    // Step 3: Heap guard
    if (freeHeap < required) {
        // Insufficient memory — restore previous mode
        _lastErrorCode.store("HEAP_INSUFFICIENT");
        _errorUpdating.store(true);
        snprintf(_lastError, sizeof(_lastError),
                 "Insufficient memory for %.30s", target.displayName);
        _errorUpdating.store(false);
        LOG_W("ModeRegistry", "Heap guard rejected switch");
#if LOG_LEVEL >= 1
        Serial.printf("[ModeRegistry] Need %u, have %u\n", (unsigned)required, (unsigned)freeHeap);
#endif

        if (_activeMode != nullptr) {
            // Re-init the still-allocated previous mode.
            // If re-init also fails, discard the shell to avoid a mode that renders
            // from an uninitialized state. Registry will be idle (no active mode)
            // until the next valid requestSwitch() + tick() cycle.
            if (!_activeMode->init(ctx)) {
                LOG_E("ModeRegistry", "Heap guard: previous mode re-init failed — clearing active mode");
                delete _activeMode;
                _activeMode = nullptr;
                _activeModeIndex = MODE_INDEX_NONE;
            }
        }
        // Story ds-3.2, AC #5: correct NVS to actually-active mode so it doesn't drift
        _nvsWritePending = true;
        _lastSwitchMs = millis();
        free(fadeSnapshotBuf);  // AC #7: free snapshot on early exit
        _switchState.store(SwitchState::IDLE);
        return;
    }

    // Step 4: Delete old mode shell and create new mode
    delete _activeMode;
    _activeMode = nullptr;

    DisplayMode* newMode = target.factory();
    if (newMode == nullptr) {
        // Factory returned null — restore previous mode
        _lastErrorCode.store("MODE_FACTORY_FAILED");
        _errorUpdating.store(true);
        snprintf(_lastError, sizeof(_lastError),
                 "Factory failed for %.40s", target.displayName);
        _errorUpdating.store(false);
        LOG_E("ModeRegistry", "Factory returned null");

        // Re-create previous mode; if re-init also fails, leave registry idle
        if (prevEntry != nullptr) {
            _activeMode = prevEntry->factory();
            if (_activeMode != nullptr) {
                if (!_activeMode->init(ctx)) {
                    LOG_E("ModeRegistry", "Factory-null: previous mode re-init failed — clearing active mode");
                    delete _activeMode;
                    _activeMode = nullptr;
                    _activeModeIndex = MODE_INDEX_NONE;
                } else {
                    _activeModeIndex = prevIndex;
                }
            } else {
                _activeModeIndex = MODE_INDEX_NONE;
            }
        }
        // Story ds-3.2, AC #5: correct NVS to actually-active mode
        _nvsWritePending = true;
        _lastSwitchMs = millis();
        free(fadeSnapshotBuf);  // AC #7: free snapshot on early exit
        _switchState.store(SwitchState::IDLE);
        return;
    }

    // Step 5: Init new mode
    if (!newMode->init(ctx)) {
        // Init failed — delete new, re-create previous; if re-init also fails, leave idle
        _lastErrorCode.store("MODE_INIT_FAILED");
        _errorUpdating.store(true);
        snprintf(_lastError, sizeof(_lastError),
                 "Init failed for %.40s", target.displayName);
        _errorUpdating.store(false);
        LOG_W("ModeRegistry", "Mode init failed");

        // Architecture D2: teardown-before-delete — init() may have partially
        // allocated resources; teardown() is required to release them safely
        // before deletion (synthesis ds-3.2 Pass 3).
        newMode->teardown();
        delete newMode;

        if (prevEntry != nullptr) {
            _activeMode = prevEntry->factory();
            if (_activeMode != nullptr) {
                if (!_activeMode->init(ctx)) {
                    LOG_E("ModeRegistry", "Init-fail: previous mode re-init failed — clearing active mode");
                    delete _activeMode;
                    _activeMode = nullptr;
                    _activeModeIndex = MODE_INDEX_NONE;
                } else {
                    _activeModeIndex = prevIndex;
                }
            } else {
                _activeModeIndex = MODE_INDEX_NONE;
            }
        } else {
            _activeMode = nullptr;
            _activeModeIndex = MODE_INDEX_NONE;
        }
        // Story ds-3.2, AC #5: correct NVS to actually-active mode
        _nvsWritePending = true;
        _lastSwitchMs = millis();
        free(fadeSnapshotBuf);  // AC #7: free snapshot on early exit
        _switchState.store(SwitchState::IDLE);
        return;
    }

    // Step 6: Switch successful — set new mode as active
    _activeMode = newMode;
    _activeModeIndex = targetIndex;
    _recoveryQueued = false;  // ds-3.2 AC #5: clear flag so a future failure starts a fresh recovery
    _errorUpdating.store(true);
    _lastError[0] = '\0';
    _lastErrorCode.store(nullptr);
    _errorUpdating.store(false);
    _nvsWritePending = true;
    _lastSwitchMs = millis();

    // HW-1.2 fade placeholder: Step 7 used to run a ~1-second crossfade by
    // grabbing FastLED.leds() into two RGB888 buffers and linear-blending
    // frame-by-frame. That entire path depends on FastLED's CRGB buffer and
    // show() primitive, neither of which exists under the HUB75 stack.
    // Until HW-1.2 re-implements transitions against the HUB75 DMA
    // double-buffer, mode switches are instant. Story dl-3.1's fade
    // contract (≤1s, 15-step linear lerp, FR35 exception for multi-show
    // during transition) is deferred, not lost.
    (void)flights;  // unused until the fade path is re-wired
    (void)fadeBufBytes;

    // AC #7: free snapshot buffer immediately after fade (or if fade was skipped)
    free(fadeSnapshotBuf);

    LOG_I("ModeRegistry", "Switch completed");
#if LOG_LEVEL >= 2
    Serial.printf("[ModeRegistry] Active mode: %s\n", target.id);
#endif
    _switchState.store(SwitchState::IDLE);
}

void ModeRegistry::tickNvsPersist() {
    if (!_nvsWritePending) return;
    if (millis() - _lastSwitchMs < 2000) return;

    // Guard: _table must be initialized before NVS persistence
    if (_table == nullptr || _count == 0) {
        _nvsWritePending = false;
        return;
    }

    // Debounce elapsed — persist via ConfigManager (AR7: single writer)
    if (_activeModeIndex < _count) {
        ConfigManager::setDisplayMode(_table[_activeModeIndex].id);
        LOG_I("ModeRegistry", "Mode persisted to NVS");
    } else {
        // Story ds-3.2, AC #5: no active mode (all failed) — correct NVS to safe default.
        // Use table[0] dynamically so ModeRegistry is not coupled to a specific mode ID.
        // Also queue a switch to table[0] so active mode catches up to NVS (no silent drift).
        // _recoveryQueued prevents an infinite retry loop if table[0] also fails to init:
        //   activation is queued at most once per "no active mode" episode; the flag is
        //   cleared in executeSwitch() when any mode succeeds so a future failure starts fresh.
        const char* safeId = (_count > 0) ? _table[0].id : "classic_card";
        ConfigManager::setDisplayMode(safeId);
        LOG_W("ModeRegistry", "No active mode — NVS corrected to safe default");
        if (_count > 0 && !_recoveryQueued) {
            _recoveryQueued = true;
            LOG_W("ModeRegistry", "Queuing default mode activation (one attempt)");
            _requestedIndex.store(0);  // AC #5: queue table[0] for next tick to close NVS drift
        } else if (_recoveryQueued) {
            LOG_E("ModeRegistry", "Default mode failed to activate — no further recovery attempts");
        }
    }
    _nvsWritePending = false;
}

void ModeRegistry::tick(const RenderContext& ctx,
                        const std::vector<FlightInfo>& flights) {
    // Check for pending switch request (atomic read on Core 0).
    // Always consume the request (store MODE_INDEX_NONE) even when the target
    // equals the currently active mode — otherwise a same-mode request leaves
    // _requestedIndex permanently set, which can fire unexpectedly after an
    // unrelated _activeModeIndex change (e.g. OTA restore, heap-guard fallback).
    uint8_t requested = _requestedIndex.load();
    if (requested != MODE_INDEX_NONE) {
        _requestedIndex.store(MODE_INDEX_NONE);  // consume before any branch
        if (requested != _activeModeIndex) {
            // Ensure REQUESTED is set (requestSwitch() already set it on Core 1,
            // but re-affirm on Core 0 as the authoritative write path).
            _switchState.store(SwitchState::REQUESTED);
            executeSwitch(requested, ctx, flights);
        } else {
            // Same-mode request consumed silently — no mode restart (idempotent).
            // Reset REQUESTED that requestSwitch() set on Core 1 so state returns to IDLE.
            _switchState.store(SwitchState::IDLE);
        }
    }

    // NVS debounced persistence
    tickNvsPersist();

    // Render active mode
    if (_activeMode != nullptr) {
        _activeMode->render(ctx, flights);
    }
}

DisplayMode* ModeRegistry::getActiveMode() {
    return _activeMode;
}

const char* ModeRegistry::getActiveModeId() {
    if (_activeModeIndex < _count && _table != nullptr) {
        return _table[_activeModeIndex].id;
    }
    return nullptr;
}

const ModeEntry* ModeRegistry::getModeTable() {
    return _table;
}

uint8_t ModeRegistry::getModeCount() {
    return _count;
}

SwitchState ModeRegistry::getSwitchState() {
    return _switchState.load();
}

const char* ModeRegistry::getLastError() {
    // Guard against cross-core read during write (review follow-up #2)
    if (_errorUpdating.load()) {
        return "";
    }
    return _lastError;
}

void ModeRegistry::copyLastError(char* outBuf, size_t maxLen) {
    // Thread-safe snapshot for Core 1 callers (synthesis ds-3.2 Pass 3).
    // Uses seqlock-style double-check: discard copy if a write started or was
    // in progress during the memcpy so callers never see a torn string.
    if (outBuf == nullptr || maxLen == 0) return;
    outBuf[0] = '\0';
    if (_errorUpdating.load()) return;  // write in progress — return empty
    strlcpy(outBuf, _lastError, maxLen);
    if (_errorUpdating.load()) outBuf[0] = '\0';  // write started during copy — discard
}

const char* ModeRegistry::getLastErrorCode() {
    // Returns stable string literal (nullptr when no error).
    // Atomic pointer read — safe across cores without locking.
    return _lastErrorCode.load();
}

bool ModeRegistry::prepareForOTA() {
    if (_otaMode.load()) {
        LOG_W("ModeRegistry", "prepareForOTA: already in OTA mode");
        return false;
    }
    if (_table == nullptr) {
        snprintf(_lastError, sizeof(_lastError), "prepareForOTA: no mode table");
        LOG_E("ModeRegistry", "prepareForOTA failed: no mode table");
        return false;
    }

    // Set SWITCHING so tick() does not run normal mode logic
    _switchState.store(SwitchState::SWITCHING);

    // Teardown and delete the active mode to free heap for OTA
    if (_activeMode != nullptr) {
        _activeMode->teardown();
        LOG_I("ModeRegistry", "prepareForOTA: mode torn down");
        delete _activeMode;
        _activeMode = nullptr;
    }
    _activeModeIndex = MODE_INDEX_NONE;

    // Consume any pending switch request so tick() doesn't try to execute it
    _requestedIndex.store(MODE_INDEX_NONE);

    _otaMode.store(true);
    LOG_I("ModeRegistry", "prepareForOTA: system ready for OTA flash");
    return true;
}

bool ModeRegistry::isOtaMode() {
    return _otaMode.load();
}

void ModeRegistry::completeOTAAttempt(bool success) {
    if (success) {
        // No-op — device will reboot momentarily
        return;
    }

    if (!_otaMode.load()) {
        LOG_W("ModeRegistry", "completeOTAAttempt: not in OTA mode");
        return;
    }

    // Story dl-7.2, AC #8: Hold OTA mode for 3s so display task shows "Update failed"
    // visual before we restore the flight rendering pipeline. Display task runs at ~5fps
    // during OTA mode, so this gives ~15 frames of the error message.
    LOG_I("ModeRegistry", "completeOTAAttempt: showing error visual for 3s");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Restore normal tick() behavior
    _otaMode.store(false);
    _switchState.store(SwitchState::IDLE);

    // Request default mode so display pipeline resumes rendering.
    // Route through ModeOrchestrator per Architecture Rule 24 — requestSwitch()
    // must only be called from ModeOrchestrator::tick() or onManualSwitch().
    if (_table != nullptr && _count > 0) {
        ModeOrchestrator::onManualSwitch(_table[0].id, _table[0].displayName);
        LOG_I("ModeRegistry", "completeOTAAttempt: restored default mode via orchestrator after OTA failure");
    } else {
        LOG_W("ModeRegistry", "completeOTAAttempt: no mode table — display will be idle");
    }
}

// BF-1 AC #1, #2: cross-core check used by Core-0 display task auto-yield.
bool ModeRegistry::hasPendingRequest() {
    return _switchState.load() == SwitchState::REQUESTED;
}

// BF-1 AC #3: name the requested mode for the preemption log line.
const char* ModeRegistry::getRequestedModeId() {
    uint8_t idx = _requestedIndex.load();
    if (idx == MODE_INDEX_NONE || _table == nullptr || idx >= _count) {
        return nullptr;
    }
    return _table[idx].id;
}

// BF-1 AC #3: record which test pattern yielded for the next mode switch.
// Source must be a string literal (stored as atomic pointer, no copy).
void ModeRegistry::markPreempted(const char* source) {
    _preemptionSource.store(source);
}

// BF-1 AC #4: source of the most recent preemption (sticky until next requestSwitch).
const char* ModeRegistry::getLastPreemptionSource() {
    return _preemptionSource.load();
}

// BF-1 AC #5: lazy stall watchdog — invoked by /api/display/mode/status
// before reading switch state. If REQUESTED has persisted past the budget,
// promote to FAILED + REQUESTED_STALL so the dashboard can render an error
// instead of polling forever.
void ModeRegistry::pollAndAdvanceStall() {
    if (_switchState.load() != SwitchState::REQUESTED) return;

    uint32_t now = (uint32_t)millis();
    uint32_t requestedAt = _requestedAtMs.load();
    // millis() is unsigned, so subtraction wraps correctly on the 49-day rollover.
    if ((now - requestedAt) <= kRequestedStallLimitMs) return;

    // Advance to FAILED. Active mode index is untouched — executeSwitch never
    // ran, so getActiveModeId() still returns the prior mode (consistent with
    // AC #5 "previous active mode is restored" — it was never replaced).
    _switchState.store(SwitchState::FAILED);
    _requestedIndex.store(MODE_INDEX_NONE);  // stop tick() from later picking up the stale request

    if (!_stallReported.exchange(true)) {
        _lastErrorCode.store("REQUESTED_STALL");
        _errorUpdating.store(true);
        snprintf(_lastError, sizeof(_lastError), "Mode switch stalled in REQUESTED");
        _errorUpdating.store(false);
        LOG_W("ModeRegistry", "REQUESTED_STALL — display task not consuming switch request");
    }
}

void ModeRegistry::enumerate(void (*cb)(const char* id, const char* displayName,
                                        uint8_t index, void* user), void* user) {
    if (cb == nullptr || _table == nullptr) return;
    for (uint8_t i = 0; i < _count; i++) {
        cb(_table[i].id, _table[i].displayName, i, user);
    }
}
