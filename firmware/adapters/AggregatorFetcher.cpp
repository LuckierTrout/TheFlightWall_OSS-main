#include "adapters/AggregatorFetcher.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_task_wdt.h>

#include "core/ConfigManager.h"
#include "utils/GeoUtils.h"

namespace {

// Aggregator serves feet/knots; StateVector carries meters/m-per-s so the
// downstream conversion in FlightDataFetcher (kft, mph) stays correct.
constexpr double FT_TO_M = 0.3048;
constexpr double KT_TO_MS = 0.514444;

}  // namespace

bool AggregatorFetcher::fetchStateVectors(double centerLat,
                                          double centerLon,
                                          double radiusKm,
                                          std::vector<StateVector> &outStateVectors)
{
    m_enrichmentByCallsign.clear();

    NetworkConfig net = ConfigManager::getNetwork();
    if (net.agg_url.length() == 0 || net.agg_token.length() == 0) {
        Serial.println("[Aggregator] agg_url or agg_token not configured");
        return false;
    }

    String url = net.agg_url;
    // Tolerate trailing slash in the configured base URL.
    if (url.endsWith("/")) url.remove(url.length() - 1);
    url += "/fleet";

    WiFiClientSecure client;
    // Cloudflare Workers present a valid cert, but shipping the root CA bundle
    // to flash is overkill for a LAN-only client. Match FlightWallFetcher's
    // approach and skip verification; the bearer token is the actual auth.
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, url)) {
        Serial.println("[Aggregator] HTTPClient.begin failed");
        return false;
    }
    // TWDT is 10s (main.cpp); keep HTTP timeout comfortably below that so a
    // slow Worker response fails cleanly instead of tripping the watchdog.
    http.setTimeout(8000);
    http.addHeader("Authorization", String("Bearer ") + net.agg_token);
    http.addHeader("Accept", "application/json");

    esp_task_wdt_reset();
    int code = http.GET();
    esp_task_wdt_reset();
    if (code <= 0) {
        Serial.printf("[Aggregator] Connection failed (error %d)\n", code);
        http.end();
        return false;
    }
    if (code != 200) {
        Serial.printf("[Aggregator] HTTP %d from /fleet\n", code);
        http.end();
        return false;
    }

    // Pull the body fully before parsing — stream parsing can block for
    // seconds on a slow connection and trip the loop-task WDT. Buffering to
    // a String bounds the blocking window to http.setTimeout() above.
    String payload = http.getString();
    http.end();
    esp_task_wdt_reset();

    // Only parse the aircraft array and its needed fields — ignore metadata
    // envelope (updated_at, provider, count) to bound heap.
    JsonDocument filter;
    JsonObject acFilter = filter["aircraft"][0].to<JsonObject>();
    acFilter["icao24"]       = true;
    acFilter["callsign"]     = true;
    acFilter["lat"]          = true;
    acFilter["lon"]          = true;
    acFilter["alt_ft"]       = true;
    acFilter["heading"]      = true;
    acFilter["speed_kt"]     = true;
    acFilter["squawk"]       = true;
    acFilter["origin_iata"]  = true;
    acFilter["origin_icao"]  = true;
    acFilter["dest_iata"]    = true;
    acFilter["dest_icao"]    = true;
    acFilter["airline_code"] = true;
    acFilter["aircraft_type"]= true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(
        doc, payload, DeserializationOption::Filter(filter));
    esp_task_wdt_reset();
    if (err) {
        Serial.printf("[Aggregator] JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray aircraft = doc["aircraft"].as<JsonArray>();
    if (aircraft.isNull()) {
        // Valid snapshot with empty fleet is not an error.
        return true;
    }

    for (JsonVariant v : aircraft) {
        if (!v.is<JsonObject>()) continue;
        JsonObject a = v.as<JsonObject>();

        StateVector s;
        s.icao24 = a["icao24"].is<const char*>() ? String(a["icao24"].as<const char*>()) : String("");
        s.callsign = a["callsign"].is<const char*>() ? String(a["callsign"].as<const char*>()) : String("");
        s.callsign.trim();

        s.lat = a["lat"].is<double>() ? a["lat"].as<double>() : NAN;
        s.lon = a["lon"].is<double>() ? a["lon"].as<double>() : NAN;
        if (isnan(s.lat) || isnan(s.lon)) continue;

        s.distance_km = haversineKm(centerLat, centerLon, s.lat, s.lon);
        if (s.distance_km > radiusKm) continue;
        s.bearing_deg = computeBearingDeg(centerLat, centerLon, s.lat, s.lon);

        if (a["alt_ft"].is<double>())   s.baro_altitude = a["alt_ft"].as<double>() * FT_TO_M;
        if (a["speed_kt"].is<double>()) s.velocity      = a["speed_kt"].as<double>() * KT_TO_MS;
        if (a["heading"].is<double>())  s.heading       = a["heading"].as<double>();

        s.squawk = a["squawk"].is<const char*>() ? String(a["squawk"].as<const char*>()) : String("");

        outStateVectors.push_back(s);

        // Enrichment map keyed by callsign — the only ID FlightDataFetcher
        // passes to fetchFlightInfo. icao24-only aircraft (no broadcast
        // callsign) have no enrichment entry and render as anonymous.
        if (s.callsign.length() == 0) continue;

        Enrichment e;
        if (a["airline_code"].is<const char*>())  e.operator_icao      = String(a["airline_code"].as<const char*>());
        if (a["aircraft_type"].is<const char*>()) e.aircraft_code      = String(a["aircraft_type"].as<const char*>());
        if (a["origin_icao"].is<const char*>())   e.origin_icao        = String(a["origin_icao"].as<const char*>());
        if (a["dest_icao"].is<const char*>())     e.destination_icao   = String(a["dest_icao"].as<const char*>());
        if (a["origin_iata"].is<const char*>())   e.origin_iata        = String(a["origin_iata"].as<const char*>());
        if (a["dest_iata"].is<const char*>())     e.destination_iata   = String(a["dest_iata"].as<const char*>());

        // Skip map entry if the aggregator returned no enrichment at all
        // for this callsign (keeps fetchFlightInfo's "miss" path clean).
        if (e.operator_icao.length() || e.aircraft_code.length() ||
            e.origin_icao.length() || e.destination_icao.length()) {
            m_enrichmentByCallsign[s.callsign] = e;
        }
    }

    return true;
}

bool AggregatorFetcher::fetchFlightInfo(const String &flightIdent, FlightInfo &outInfo)
{
    if (flightIdent.length() == 0) return false;

    auto it = m_enrichmentByCallsign.find(flightIdent);
    if (it == m_enrichmentByCallsign.end()) return false;

    const Enrichment &e = it->second;

    // FlightDataFetcher overlays only non-empty fields, so we only have to
    // populate what we actually know. ident is set to the callsign so that
    // the overlay in FlightDataFetcher doesn't wipe the caller's ident.
    outInfo.ident = flightIdent;
    outInfo.ident_icao = flightIdent;
    outInfo.operator_icao = e.operator_icao;
    outInfo.operator_code = e.operator_icao;  // FlightWall CDN airline lookup uses ICAO
    outInfo.aircraft_code = e.aircraft_code;
    outInfo.origin.code_icao = e.origin_icao;
    outInfo.origin.code_iata = e.origin_iata;
    outInfo.destination.code_icao = e.destination_icao;
    outInfo.destination.code_iata = e.destination_iata;

    return true;
}
