/*
Purpose: Centralized subsystem health registry.
Responsibilities:
- Track health status (OK/WARNING/ERROR) for each subsystem with message and timestamp.
- Provide JSON serialization for the /api/status endpoint.
- Build extended diagnostics JSON for the health page (Story 2.4).
*/
#include "core/SystemStatus.h"
#include "core/ConfigManager.h"
#include "core/LogoManager.h"
#include "utils/Log.h"
#include <WiFi.h>
#include <LittleFS.h>

SubsystemStatus SystemStatus::_statuses[SUBSYSTEM_COUNT] = {};
SemaphoreHandle_t SystemStatus::_mutex = nullptr;

void SystemStatus::init() {
    _mutex = xSemaphoreCreateMutex();
    if (!_mutex) {
        LOG_E("SystemStatus", "Mutex creation failed — running without synchronization");
    }
    for (uint8_t i = 0; i < SUBSYSTEM_COUNT; i++) {
        _statuses[i].level = StatusLevel::OK;
        _statuses[i].message = "Not initialized";
        _statuses[i].timestamp = millis();
    }
    LOG_I("SystemStatus", "Initialized");
}

void SystemStatus::set(Subsystem sys, StatusLevel level, const String& message) {
    uint8_t idx = static_cast<uint8_t>(sys);
    if (idx >= SUBSYSTEM_COUNT) return;

    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        _statuses[idx].level = level;
        _statuses[idx].message = message;
        _statuses[idx].timestamp = millis();
        xSemaphoreGive(_mutex);
    } else {
        _statuses[idx].level = level;
        _statuses[idx].message = message;
        _statuses[idx].timestamp = millis();
    }
}

SubsystemStatus SystemStatus::get(Subsystem sys) {
    uint8_t idx = static_cast<uint8_t>(sys);
    if (idx >= SUBSYSTEM_COUNT) {
        return {StatusLevel::ERROR, "Invalid subsystem", millis()};
    }
    SubsystemStatus snapshot;
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        snapshot = _statuses[idx];
        xSemaphoreGive(_mutex);
    } else {
        snapshot = _statuses[idx]; // best-effort if mutex unavailable
    }
    return snapshot;
}

void SystemStatus::toJson(JsonObject& obj) {
    // Snapshot all statuses under mutex, then serialize outside
    SubsystemStatus snap[SUBSYSTEM_COUNT];
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (uint8_t i = 0; i < SUBSYSTEM_COUNT; i++) {
            snap[i] = _statuses[i];
        }
        xSemaphoreGive(_mutex);
    } else {
        for (uint8_t i = 0; i < SUBSYSTEM_COUNT; i++) {
            snap[i] = _statuses[i];
        }
    }
    for (uint8_t i = 0; i < SUBSYSTEM_COUNT; i++) {
        JsonObject sub = obj[subsystemName(static_cast<Subsystem>(i))].to<JsonObject>();
        sub["level"] = levelName(snap[i].level);
        sub["message"] = snap[i].message;
        sub["timestamp"] = snap[i].timestamp;
    }
}

void SystemStatus::toExtendedJson(JsonObject& obj, const FlightStatsSnapshot& stats) {
    // --- subsystems (existing six) ---
    JsonObject subsystems = obj["subsystems"].to<JsonObject>();
    toJson(subsystems);

    // --- wifi_detail ---
    JsonObject wifiDetail = obj["wifi_detail"].to<JsonObject>();
    wifi_mode_t mode = WiFi.getMode();
    if (mode == WIFI_STA) {
        wifiDetail["mode"] = "STA";
        wifiDetail["ssid"] = WiFi.SSID();
        wifiDetail["rssi"] = WiFi.RSSI();
        wifiDetail["ip"] = WiFi.localIP().toString();
    } else if (mode == WIFI_AP || mode == WIFI_AP_STA) {
        wifiDetail["mode"] = "AP";
        wifiDetail["ssid"] = WiFi.softAPSSID();
        wifiDetail["ip"] = WiFi.softAPIP().toString();
        wifiDetail["clients"] = WiFi.softAPgetStationNum();
    } else {
        wifiDetail["mode"] = "OFF";
    }

    // --- device ---
    JsonObject device = obj["device"].to<JsonObject>();
    device["uptime_ms"] = millis();
    device["free_heap"] = ESP.getFreeHeap();
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    device["fs_total"] = totalBytes;
    device["fs_used"] = usedBytes;
    device["logo_count"] = LogoManager::getLogoCount();

    // --- flight ---
    JsonObject flight = obj["flight"].to<JsonObject>();
    flight["last_fetch_ms"] = stats.last_fetch_ms;
    flight["state_vectors"] = stats.state_vectors;
    flight["enriched_flights"] = stats.enriched_flights;
    flight["logos_matched"] = stats.logos_matched;

    // --- quota ---
    JsonObject quota = obj["quota"].to<JsonObject>();
    uint16_t fetchInterval = ConfigManager::getTiming().fetch_interval;
    uint32_t estimatedMonthly = (fetchInterval > 0) ? (2592000UL / fetchInterval) : 0;
    bool overPace = estimatedMonthly > 4000;
    quota["fetches_since_boot"] = stats.fetches_since_boot;
    quota["limit"] = 4000;
    quota["fetch_interval_s"] = fetchInterval;
    quota["estimated_monthly_polls"] = estimatedMonthly;
    quota["over_pace"] = overPace;
}

const char* SystemStatus::subsystemName(Subsystem sys) {
    switch (sys) {
        case Subsystem::WIFI:     return "wifi";
        case Subsystem::OPENSKY:  return "opensky";
        case Subsystem::AEROAPI:  return "aeroapi";
        case Subsystem::CDN:      return "cdn";
        case Subsystem::NVS:      return "nvs";
        case Subsystem::LITTLEFS: return "littlefs";
        default:                  return "unknown";
    }
}

const char* SystemStatus::levelName(StatusLevel level) {
    switch (level) {
        case StatusLevel::OK:      return "ok";
        case StatusLevel::WARNING: return "warning";
        case StatusLevel::ERROR:   return "error";
        default:                   return "unknown";
    }
}
