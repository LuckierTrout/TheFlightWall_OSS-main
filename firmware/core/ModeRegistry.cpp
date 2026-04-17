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
#include <FastLED.h>
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
    LOG_I("ModeRegistry", "Switch requested");
#if LOG_LEVEL >= 2
    Serial.printf("[ModeRegistry] Target: %s (index %d)\n", modeId, (int)idx);
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

    {
        uint16_t w = ctx.layout.matrixWidth;
        uint16_t h = ctx.layout.matrixHeight;

        if (w == 0 || h == 0) {
            // AC #2: zero dimensions — skip fade, instant path
            LOG_W("ModeRegistry", "Fade skipped: matrix dimensions zero");
        } else {
            fadePixelCount = (size_t)w * (size_t)h;
            fadeBufBytes = fadePixelCount * 3;  // RGB888 per pixel

            // Allocate buffer A (outgoing snapshot)
            fadeSnapshotBuf = (uint8_t*)malloc(fadeBufBytes);
            if (fadeSnapshotBuf != nullptr) {
                // Copy current CRGB leds into RGB888 buffer A
                CRGB* leds = FastLED.leds();
                int numLeds = FastLED.size();
                size_t copyCount = (fadePixelCount <= (size_t)numLeds) ? fadePixelCount : (size_t)numLeds;
                for (size_t i = 0; i < copyCount; i++) {
                    fadeSnapshotBuf[i * 3 + 0] = leds[i].r;
                    fadeSnapshotBuf[i * 3 + 1] = leds[i].g;
                    fadeSnapshotBuf[i * 3 + 2] = leds[i].b;
                }
                // Zero-fill any remaining pixels if pixelCount > numLeds
                if (copyCount < fadePixelCount) {
                    memset(fadeSnapshotBuf + copyCount * 3, 0,
                           (fadePixelCount - copyCount) * 3);
                }
                LOG_I("ModeRegistry", "Fade snapshot captured");
#if LOG_LEVEL >= 2
                Serial.printf("[ModeRegistry] Snapshot: %ux%u = %u pixels, %u bytes\n",
                              (unsigned)w, (unsigned)h, (unsigned)fadePixelCount, (unsigned)fadeBufBytes);
#endif
            } else {
                // AC #6: malloc failed — will use instant handoff
                LOG_W("ModeRegistry", "Fade snapshot malloc failed — instant handoff");
                fadePixelCount = 0;
                fadeBufBytes = 0;
            }
        }
    }

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

    // Step 7 (Story dl-3.1, AC #1): Execute fade transition AFTER init succeeds,
    // BEFORE returning to IDLE. _switchState remains SWITCHING during fade so
    // orchestrator/HTTP paths remain coherent (AC #9).
    // New requestSwitch calls during fade write to _requestedIndex atomically
    // but are NOT consumed until tick() runs after IDLE (AC #8 — drop during fade).
    if (fadeSnapshotBuf != nullptr && fadePixelCount > 0) {
        // Render first frame of new mode to capture buffer B
        _activeMode->render(ctx, flights);

        // Allocate buffer B (incoming first frame)
        uint8_t* fadeIncomingBuf = (uint8_t*)malloc(fadeBufBytes);
        if (fadeIncomingBuf != nullptr) {
            CRGB* leds = FastLED.leds();
            int numLeds = FastLED.size();
            size_t copyCount = (fadePixelCount <= (size_t)numLeds) ? fadePixelCount : (size_t)numLeds;
            for (size_t i = 0; i < copyCount; i++) {
                fadeIncomingBuf[i * 3 + 0] = leds[i].r;
                fadeIncomingBuf[i * 3 + 1] = leds[i].g;
                fadeIncomingBuf[i * 3 + 2] = leds[i].b;
            }
            if (copyCount < fadePixelCount) {
                memset(fadeIncomingBuf + copyCount * 3, 0,
                       (fadePixelCount - copyCount) * 3);
            }

            // Execute crossfade: ~15 steps over ≤1s (AC #3)
            // FR35 exception: multiple show() calls during transition only (AC #9)
            static constexpr int FADE_STEPS = 15;
            static constexpr int FADE_STEP_DELAY_MS = 66;  // 15 steps × 66ms ≈ 990ms ≤ 1s

            LOG_I("ModeRegistry", "Fade transition starting");

            for (int step = 1; step <= FADE_STEPS; step++) {
                // AC #4: integer lerp with fixed-point 0–255 alpha
                uint8_t alpha = (uint8_t)((step * 255) / FADE_STEPS);
                uint8_t invAlpha = 255 - alpha;

                CRGB* leds = FastLED.leds();
                size_t blendCount = (fadePixelCount <= (size_t)FastLED.size())
                                     ? fadePixelCount : (size_t)FastLED.size();

                for (size_t i = 0; i < blendCount; i++) {
                    size_t off = i * 3;
                    leds[i].r = (uint8_t)(((uint16_t)fadeSnapshotBuf[off + 0] * invAlpha +
                                           (uint16_t)fadeIncomingBuf[off + 0] * alpha) >> 8);
                    leds[i].g = (uint8_t)(((uint16_t)fadeSnapshotBuf[off + 1] * invAlpha +
                                           (uint16_t)fadeIncomingBuf[off + 1] * alpha) >> 8);
                    leds[i].b = (uint8_t)(((uint16_t)fadeSnapshotBuf[off + 2] * invAlpha +
                                           (uint16_t)fadeIncomingBuf[off + 2] * alpha) >> 8);
                }

                FastLED.show();
                esp_task_wdt_reset();  // Prevent WDT timeout during ~990ms crossfade
                vTaskDelay(pdMS_TO_TICKS(FADE_STEP_DELAY_MS));
            }

            LOG_I("ModeRegistry", "Fade transition complete");
            free(fadeIncomingBuf);  // AC #7: free immediately
        } else {
            // AC #6: buffer B malloc failed — graceful degradation, instant handoff
            LOG_W("ModeRegistry", "Fade incoming buffer malloc failed — instant handoff");
        }
    }

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

void ModeRegistry::enumerate(void (*cb)(const char* id, const char* displayName,
                                        uint8_t index, void* user), void* user) {
    if (cb == nullptr || _table == nullptr) return;
    for (uint8_t i = 0; i < _count; i++) {
        cb(_table[i].id, _table[i].displayName, i, user);
    }
}
