/*
Purpose: ModeRegistry implementation — static table with cooperative switch serialization.
Architecture: Decision D2 — teardown-before-delete, heap guard, latest-wins atomic,
              NVS debounce via ConfigManager::setDisplayMode().
              Story dl-3.1: linear crossfade transition between display modes.
References: architecture.md#D2, core/ModeRegistry.h
*/

#include "core/ModeRegistry.h"
#include "core/ConfigManager.h"
#include "utils/Log.h"
#include <Arduino.h>
#include <cstring>
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Static member initialization
const ModeEntry* ModeRegistry::_table = nullptr;
uint8_t ModeRegistry::_count = 0;
DisplayMode* ModeRegistry::_activeMode = nullptr;
uint8_t ModeRegistry::_activeModeIndex = MODE_INDEX_NONE;
std::atomic<uint8_t> ModeRegistry::_requestedIndex(MODE_INDEX_NONE);
SwitchState ModeRegistry::_switchState = SwitchState::IDLE;
char ModeRegistry::_lastError[64] = {0};
bool ModeRegistry::_nvsWritePending = false;
unsigned long ModeRegistry::_lastSwitchMs = 0;

void ModeRegistry::init(const ModeEntry* table, uint8_t count) {
    _table = table;
    _count = count;
    _activeMode = nullptr;
    _activeModeIndex = MODE_INDEX_NONE;
    _requestedIndex.store(MODE_INDEX_NONE);
    _switchState = SwitchState::IDLE;
    _lastError[0] = '\0';
    _nvsWritePending = false;
    _lastSwitchMs = 0;

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
        snprintf(_lastError, sizeof(_lastError), "Unknown mode: %.40s",
                 modeId ? modeId : "(null)");
        LOG_W("ModeRegistry", "Switch request rejected");
#if LOG_LEVEL >= 1
        Serial.printf("[ModeRegistry] %s\n", _lastError);
#endif
        return false;
    }

    // Latest-wins: atomic store overwrites any pending request (AC #6, #7)
    _requestedIndex.store(idx);
    LOG_I("ModeRegistry", "Switch requested");
#if LOG_LEVEL >= 2
    Serial.printf("[ModeRegistry] Target: %s (index %d)\n", modeId, (int)idx);
#endif
    return true;
}

void ModeRegistry::executeSwitch(uint8_t targetIndex, const RenderContext& ctx,
                                 const std::vector<FlightInfo>& flights) {
    _switchState = SwitchState::SWITCHING;

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
        snprintf(_lastError, sizeof(_lastError),
                 "Insufficient memory for %.30s", target.displayName);
        LOG_W("ModeRegistry", "Heap guard rejected switch");
#if LOG_LEVEL >= 1
        Serial.printf("[ModeRegistry] Need %u, have %u\n", (unsigned)required, (unsigned)freeHeap);
#endif

        if (_activeMode != nullptr) {
            // Re-init the still-allocated previous mode
            _activeMode->init(ctx);
        }
        // Story ds-3.2, AC #5: correct NVS to actually-active mode so it doesn't drift
        _nvsWritePending = true;
        _lastSwitchMs = millis();
        free(fadeSnapshotBuf);  // AC #7: free snapshot on early exit
        _switchState = SwitchState::IDLE;
        return;
    }

    // Step 4: Delete old mode shell and create new mode
    delete _activeMode;
    _activeMode = nullptr;

    DisplayMode* newMode = target.factory();
    if (newMode == nullptr) {
        // Factory returned null — restore previous mode
        snprintf(_lastError, sizeof(_lastError),
                 "Factory failed for %.40s", target.displayName);
        LOG_E("ModeRegistry", "Factory returned null");

        // Re-create previous mode
        if (prevEntry != nullptr) {
            _activeMode = prevEntry->factory();
            if (_activeMode != nullptr) {
                _activeMode->init(ctx);
            }
            _activeModeIndex = prevIndex;
        }
        // Story ds-3.2, AC #5: correct NVS to actually-active mode
        _nvsWritePending = true;
        _lastSwitchMs = millis();
        free(fadeSnapshotBuf);  // AC #7: free snapshot on early exit
        _switchState = SwitchState::IDLE;
        return;
    }

    // Step 5: Init new mode
    if (!newMode->init(ctx)) {
        // Init failed — delete new, re-create previous
        snprintf(_lastError, sizeof(_lastError),
                 "Init failed for %.40s", target.displayName);
        LOG_W("ModeRegistry", "Mode init failed");

        delete newMode;

        if (prevEntry != nullptr) {
            _activeMode = prevEntry->factory();
            if (_activeMode != nullptr) {
                _activeMode->init(ctx);
            }
            _activeModeIndex = prevIndex;
        } else {
            _activeMode = nullptr;
            _activeModeIndex = MODE_INDEX_NONE;
        }
        // Story ds-3.2, AC #5: correct NVS to actually-active mode
        _nvsWritePending = true;
        _lastSwitchMs = millis();
        free(fadeSnapshotBuf);  // AC #7: free snapshot on early exit
        _switchState = SwitchState::IDLE;
        return;
    }

    // Step 6: Switch successful — set new mode as active
    _activeMode = newMode;
    _activeModeIndex = targetIndex;
    _lastError[0] = '\0';
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
    _switchState = SwitchState::IDLE;
}

void ModeRegistry::tickNvsPersist() {
    if (!_nvsWritePending) return;
    if (millis() - _lastSwitchMs < 2000) return;

    // Debounce elapsed — persist via ConfigManager (AR7: single writer)
    if (_activeModeIndex < _count) {
        ConfigManager::setDisplayMode(_table[_activeModeIndex].id);
        LOG_I("ModeRegistry", "Mode persisted to NVS");
    } else {
        // Story ds-3.2, AC #5: no active mode (all failed) — correct NVS to default
        ConfigManager::setDisplayMode("classic_card");
        LOG_W("ModeRegistry", "No active mode — NVS corrected to classic_card");
    }
    _nvsWritePending = false;
}

void ModeRegistry::tick(const RenderContext& ctx,
                        const std::vector<FlightInfo>& flights) {
    // Check for pending switch request (atomic read from Core 0)
    uint8_t requested = _requestedIndex.load();
    if (requested != MODE_INDEX_NONE && requested != _activeModeIndex) {
        // Consume the request
        _requestedIndex.store(MODE_INDEX_NONE);
        executeSwitch(requested, ctx, flights);
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
    return _switchState;
}

const char* ModeRegistry::getLastError() {
    return _lastError;
}

void ModeRegistry::enumerate(void (*cb)(const char* id, const char* displayName,
                                        uint8_t index, void* user), void* user) {
    if (cb == nullptr || _table == nullptr) return;
    for (uint8_t i = 0; i < _count; i++) {
        cb(_table[i].id, _table[i].displayName, i, user);
    }
}
