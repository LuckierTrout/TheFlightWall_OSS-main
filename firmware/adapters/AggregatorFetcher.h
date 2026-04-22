#pragma once

#include <Arduino.h>
#include <map>
#include "interfaces/BaseStateVectorFetcher.h"
#include "interfaces/BaseFlightFetcher.h"

// Single-shot fetcher for the FlightWall Cloudflare aggregator Worker.
//
// One HTTPS GET to /fleet returns both live telemetry and route/airline
// enrichment in a single payload. This adapter therefore implements BOTH
// fetcher interfaces:
//
//   - BaseStateVectorFetcher::fetchStateVectors() performs the network call,
//     populates StateVectors for FlightDataFetcher's live telemetry path,
//     and caches the per-aircraft enrichment fields indexed by callsign.
//
//   - BaseFlightFetcher::fetchFlightInfo() is a synchronous in-memory lookup
//     into that cache. It never makes a network call — the data arrived with
//     the state vectors.
//
// Pass the same instance to both FlightDataFetcher constructor slots:
//     new FlightDataFetcher(&g_agg, &g_agg)
//
// NVS keys consumed (NetworkConfig):
//     agg_url    — e.g. "https://flightwall-aggregator.<sub>.workers.dev"
//     agg_token  — bearer token matching the Worker's ESP32_AUTH_TOKEN
class AggregatorFetcher : public BaseStateVectorFetcher, public BaseFlightFetcher
{
public:
    AggregatorFetcher() = default;
    ~AggregatorFetcher() override = default;

    bool fetchStateVectors(double centerLat,
                           double centerLon,
                           double radiusKm,
                           std::vector<StateVector> &outStateVectors) override;

    bool fetchFlightInfo(const String &flightIdent, FlightInfo &outInfo) override;

private:
    // Enrichment fields extracted from a single /fleet aircraft entry.
    // Mapped into FlightInfo when fetchFlightInfo is called.
    struct Enrichment {
        String operator_icao;   // aggregator "airline_code" (ICAO 3-letter)
        String aircraft_code;   // aggregator "aircraft_type" (e.g. "B738")
        String origin_icao;
        String destination_icao;
        String origin_iata;
        String destination_iata;
    };

    // Callsign (trimmed, upper) → enrichment. Refreshed each fetchStateVectors
    // call; stale entries are naturally overwritten or cleared.
    std::map<String, Enrichment> m_enrichmentByCallsign;
};
