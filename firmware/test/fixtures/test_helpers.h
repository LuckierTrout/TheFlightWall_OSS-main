/*
 * Test Helpers for FlightWall Unity Tests
 *
 * Purpose: Common test utilities, assertion helpers, and mock utilities.
 * Environment: esp32dev (PlatformIO Unity test framework)
 *
 * Usage: Include this header for extended assertions, NVS cleanup,
 *        LittleFS helpers, and cross-test coordination utilities.
 */
#pragma once

#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include <cmath>

// ============================================================================
// Extended Assertions
// ============================================================================

/**
 * Assert a string equals expected value.
 */
#define TEST_ASSERT_STRING_EQUAL(expected, actual) \
    TEST_ASSERT_TRUE_MESSAGE(String(expected) == String(actual), \
        (String("Expected: '") + String(expected) + "' but got: '" + String(actual) + "'").c_str())

/**
 * Assert a string contains a substring.
 */
#define TEST_ASSERT_STRING_CONTAINS(substring, actual) \
    TEST_ASSERT_TRUE_MESSAGE(String(actual).indexOf(substring) >= 0, \
        (String("Expected '") + String(actual) + "' to contain '" + String(substring) + "'").c_str())

/**
 * Assert a value is NAN.
 */
#define TEST_ASSERT_NAN(value) \
    TEST_ASSERT_TRUE_MESSAGE(isnan(value), "Expected NAN")

/**
 * Assert a value is NOT NAN.
 */
#define TEST_ASSERT_NOT_NAN(value) \
    TEST_ASSERT_FALSE_MESSAGE(isnan(value), "Expected non-NAN value")

/**
 * Assert pointer is not null.
 */
#define TEST_ASSERT_NOT_NULL_PTR(ptr) \
    TEST_ASSERT_NOT_NULL_MESSAGE(ptr, "Expected non-null pointer")

/**
 * Assert pointer is null.
 */
#define TEST_ASSERT_NULL_PTR(ptr) \
    TEST_ASSERT_NULL_MESSAGE(ptr, "Expected null pointer")

// ============================================================================
// Heap Monitoring
// ============================================================================

namespace TestHelpers {

/**
 * Captures current free heap for later comparison.
 */
class HeapMonitor {
public:
    HeapMonitor() : _startHeap(ESP.getFreeHeap()) {}

    /**
     * Returns the difference in heap from start (positive = leaked, negative = freed).
     */
    int32_t heapDelta() const {
        return static_cast<int32_t>(_startHeap) - static_cast<int32_t>(ESP.getFreeHeap());
    }

    /**
     * Asserts no significant heap leak (within tolerance).
     */
    void assertNoLeak(int32_t toleranceBytes = 128) const {
        int32_t delta = heapDelta();
        char msg[64];
        snprintf(msg, sizeof(msg), "Heap leak: %d bytes", delta);
        TEST_ASSERT_TRUE_MESSAGE(delta <= toleranceBytes, msg);
    }

    /**
     * Returns starting heap value.
     */
    uint32_t startHeap() const { return _startHeap; }

    /**
     * Returns current free heap.
     */
    static uint32_t currentHeap() { return ESP.getFreeHeap(); }

private:
    uint32_t _startHeap;
};

// ============================================================================
// NVS Utilities
// ============================================================================

/**
 * Clears the "flightwall" NVS namespace.
 */
inline void clearNvs(const char* ns = "flightwall") {
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.clear();
    prefs.end();
}

/**
 * Writes a value to NVS for test setup.
 */
inline void nvsWriteUInt8(const char* key, uint8_t value, const char* ns = "flightwall") {
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.putUChar(key, value);
    prefs.end();
}

inline void nvsWriteUInt16(const char* key, uint16_t value, const char* ns = "flightwall") {
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.putUShort(key, value);
    prefs.end();
}

inline void nvsWriteString(const char* key, const char* value, const char* ns = "flightwall") {
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.putString(key, value);
    prefs.end();
}

inline void nvsWriteDouble(const char* key, double value, const char* ns = "flightwall") {
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.putDouble(key, value);
    prefs.end();
}

/**
 * Reads a value from NVS for test verification.
 */
inline uint8_t nvsReadUInt8(const char* key, uint8_t defaultVal = 0, const char* ns = "flightwall") {
    Preferences prefs;
    prefs.begin(ns, true);
    uint8_t val = prefs.getUChar(key, defaultVal);
    prefs.end();
    return val;
}

inline uint16_t nvsReadUInt16(const char* key, uint16_t defaultVal = 0, const char* ns = "flightwall") {
    Preferences prefs;
    prefs.begin(ns, true);
    uint16_t val = prefs.getUShort(key, defaultVal);
    prefs.end();
    return val;
}

inline String nvsReadString(const char* key, const char* defaultVal = "", const char* ns = "flightwall") {
    Preferences prefs;
    prefs.begin(ns, true);
    String val = prefs.getString(key, defaultVal);
    prefs.end();
    return val;
}

inline bool nvsKeyExists(const char* key, const char* ns = "flightwall") {
    Preferences prefs;
    prefs.begin(ns, true);
    bool exists = prefs.isKey(key);
    prefs.end();
    return exists;
}

// ============================================================================
// Timing Utilities
// ============================================================================

/**
 * Measures execution time of a block.
 */
class TimingMeasure {
public:
    TimingMeasure() : _startMs(millis()) {}

    /**
     * Returns elapsed time in milliseconds.
     */
    uint32_t elapsedMs() const {
        return millis() - _startMs;
    }

    /**
     * Asserts execution completed within time limit.
     */
    void assertWithinMs(uint32_t maxMs) const {
        uint32_t elapsed = elapsedMs();
        char msg[64];
        snprintf(msg, sizeof(msg), "Took %lu ms, expected <= %lu ms", elapsed, maxMs);
        TEST_ASSERT_TRUE_MESSAGE(elapsed <= maxMs, msg);
    }

private:
    uint32_t _startMs;
};

/**
 * Busy-wait for condition with timeout.
 * Returns true if condition was met, false if timeout.
 */
template<typename Predicate>
bool waitFor(Predicate condition, uint32_t timeoutMs, uint32_t pollIntervalMs = 10) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (condition()) {
            return true;
        }
        delay(pollIntervalMs);
    }
    return false;
}

// ============================================================================
// Test State Machine Helpers (for ModeRegistry tests)
// ============================================================================

/**
 * Enum matching ModeRegistry::SwitchState for test verification.
 * Import actual enum in test files that need to verify state.
 */
enum class MockSwitchState : uint8_t {
    IDLE = 0,
    REQUESTED = 1,
    SWITCHING = 2
};

/**
 * Simulates a state machine transition sequence for testing.
 */
struct StateMachineTracker {
    std::vector<uint8_t> transitions;

    void recordState(uint8_t state) {
        transitions.push_back(state);
    }

    void clear() {
        transitions.clear();
    }

    size_t transitionCount() const {
        return transitions.size();
    }

    bool lastStateIs(uint8_t expected) const {
        return !transitions.empty() && transitions.back() == expected;
    }
};

// ============================================================================
// Test Callback Helpers
// ============================================================================

/**
 * Simple callback counter for testing event handlers.
 */
class CallbackCounter {
public:
    CallbackCounter() : _count(0) {}

    void increment() { _count++; }
    void reset() { _count = 0; }
    int count() const { return _count; }

    /**
     * Returns a lambda suitable for std::function callbacks.
     */
    std::function<void()> callback() {
        return [this]() { this->increment(); };
    }

private:
    int _count;
};

/**
 * Callback that captures a single value.
 */
template<typename T>
class ValueCapture {
public:
    ValueCapture() : _captured(false) {}

    void capture(const T& value) {
        _value = value;
        _captured = true;
    }

    bool hasCaptured() const { return _captured; }
    const T& value() const { return _value; }
    void reset() { _captured = false; }

private:
    T _value;
    bool _captured;
};

} // namespace TestHelpers
