#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>

enum class Subsystem : uint8_t {
    WIFI,
    AGGREGATOR,  // Cloudflare aggregator worker — flight-data fetch status
    CDN,         // FlightWall CDN — airline/aircraft display-name enrichment status
    NVS,
    LITTLEFS,
    OTA,
    NTP,
    OTA_PULL   // Story dl-7.2, AC #6: pull-OTA download/verify status (distinct from push OTA)
};

enum class StatusLevel : uint8_t {
    OK,
    WARNING,
    ERROR
};

struct SubsystemStatus {
    StatusLevel level;
    String message;
    unsigned long timestamp;
};

// Thread-safe flight statistics snapshot (written from loop(), read from HTTP handler).
// All fields are plain scalars — safe for atomic-style copy on 32-bit ESP32.
struct FlightStatsSnapshot {
    unsigned long last_fetch_ms;    // millis() of last completed fetch
    uint16_t state_vectors;         // state vector count from last fetch
    uint16_t enriched_flights;      // enriched flight count from last fetch
    uint16_t logos_matched;         // placeholder 0 until Epic 3
    uint32_t fetches_since_boot;    // total fetch attempts since boot
    bool rollback_detected;         // true if OTA rollback occurred (Story fn-1.4)
};

class SystemStatus {
public:
    static void init();
    static void set(Subsystem sys, StatusLevel level, const String& message);
    static SubsystemStatus get(Subsystem sys);
    static void toJson(JsonObject& obj);

    // Build extended status JSON for /api/status health page (Story 2.4).
    // Includes subsystems + wifi_detail + device + flight + quota.
    static void toExtendedJson(JsonObject& obj, const FlightStatsSnapshot& stats);

    static const char* levelName(StatusLevel level);

private:
    static const uint8_t SUBSYSTEM_COUNT = 8;
    static SubsystemStatus _statuses[SUBSYSTEM_COUNT];
    static SemaphoreHandle_t _mutex;
    static const char* subsystemName(Subsystem sys);
};
