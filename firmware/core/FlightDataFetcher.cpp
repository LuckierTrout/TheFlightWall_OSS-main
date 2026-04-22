/*
Purpose: Orchestrate fetching and enrichment of flight data for display.
Flow:
1) Use BaseStateVectorFetcher (AggregatorFetcher against the Cloudflare worker)
   to fetch nearby state vectors with route metadata already joined.
2) For each callsign, use BaseFlightFetcher (also AggregatorFetcher, reading
   from its in-memory map) to retrieve the FlightInfo projection.
3) Enrich names via FlightWallFetcher (airline/aircraft display names from CDN).
Output: Returns count of enriched flights and fills outStates/outFlights.
*/
#include "core/FlightDataFetcher.h"
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"
#include "adapters/FlightWallFetcher.h"
#include <esp_task_wdt.h>
#include <vector>

namespace {

// -----------------------------------------------------------------------------
// Enrichment cache
//
// Aggregator lookups are free (the worker caches upstream), but the FlightWall
// CDN name-lookup HTTPS calls are not — and the answer doesn't change for the
// lifetime of a flight. A scheduled UAL996 flies KIAD→DGAA for its whole
// duration with the same aircraft and airline. Without a cache, every 30s
// fetch cycle re-asks the CDN for the same callsigns, burning time and
// risking rate-limit responses.
//
// This cache keeps the *enriched* fields of a FlightInfo (airline, route,
// aircraft, display names, ICAO/IATA idents) keyed by callsign. State-vector
// telemetry (altitude, speed, position) intentionally does NOT live here —
// that data is live and is always reapplied from the latest StateVector, so
// the cache holding a stale copy would be worse than useless.
//
// Size + TTL are tuned for a busy 50km radius: ~50 callsigns at once, each
// flight typically visible ~10-30 minutes. 60 entries leaves headroom; 1 hour
// TTL bounds memory if the process stays up across many flights.
// -----------------------------------------------------------------------------

struct CachedEnrichment {
    String   callsign;
    uint32_t insertedAtMs;
    // Enriched fields only — telemetry never cached.
    String   ident;
    String   ident_icao;
    String   ident_iata;
    String   operator_code;
    String   operator_icao;
    String   operator_iata;
    String   aircraft_code;
    String   origin_icao;
    String   origin_iata;
    String   destination_icao;
    String   destination_iata;
    String   airline_display_name_full;
    String   aircraft_display_name_short;
    String   aircraft_display_name_full;
};

constexpr size_t   CACHE_MAX_ENTRIES = 60;
constexpr uint32_t CACHE_TTL_MS      = 3600UL * 1000UL;  // 1 hour

std::vector<CachedEnrichment> g_enrichmentCache;

const CachedEnrichment* cacheLookup(const String& callsign, uint32_t nowMs) {
    for (size_t i = 0; i < g_enrichmentCache.size(); ++i) {
        const CachedEnrichment& e = g_enrichmentCache[i];
        if (e.callsign == callsign && (nowMs - e.insertedAtMs) < CACHE_TTL_MS) {
            return &g_enrichmentCache[i];
        }
    }
    return nullptr;
}

void cacheInsert(const String& callsign, const FlightInfo& info, uint32_t nowMs) {
    // Replace existing entry with this callsign (refresh timestamp + any
    // newly-resolved CDN names) before considering eviction.
    for (size_t i = 0; i < g_enrichmentCache.size(); ++i) {
        if (g_enrichmentCache[i].callsign == callsign) {
            g_enrichmentCache.erase(g_enrichmentCache.begin() + i);
            break;
        }
    }
    // Evict expired entries opportunistically so the LRU step below has a
    // chance to see an already-trimmed set and avoid dropping fresh data.
    for (size_t i = 0; i < g_enrichmentCache.size(); ) {
        if ((nowMs - g_enrichmentCache[i].insertedAtMs) >= CACHE_TTL_MS) {
            g_enrichmentCache.erase(g_enrichmentCache.begin() + i);
        } else {
            ++i;
        }
    }
    // LRU eviction: vector is append-ordered so front() is the oldest.
    while (g_enrichmentCache.size() >= CACHE_MAX_ENTRIES) {
        g_enrichmentCache.erase(g_enrichmentCache.begin());
    }

    CachedEnrichment e;
    e.callsign                    = callsign;
    e.insertedAtMs                = nowMs;
    e.ident                       = info.ident;
    e.ident_icao                  = info.ident_icao;
    e.ident_iata                  = info.ident_iata;
    e.operator_code               = info.operator_code;
    e.operator_icao               = info.operator_icao;
    e.operator_iata               = info.operator_iata;
    e.aircraft_code               = info.aircraft_code;
    e.origin_icao                 = info.origin.code_icao;
    e.origin_iata                 = info.origin.code_iata;
    e.destination_icao            = info.destination.code_icao;
    e.destination_iata            = info.destination.code_iata;
    e.airline_display_name_full   = info.airline_display_name_full;
    e.aircraft_display_name_short = info.aircraft_display_name_short;
    e.aircraft_display_name_full  = info.aircraft_display_name_full;
    g_enrichmentCache.push_back(e);
}

// Merge a cached entry's enriched fields onto a FlightInfo that already has
// fresh state-vector telemetry populated. Only overwrites empty/nonzero
// fields — callers can choose whether to treat cache as authoritative.
void applyCachedEnrichment(FlightInfo& info, const CachedEnrichment& e) {
    if (e.ident.length())                       info.ident                       = e.ident;
    if (e.ident_icao.length())                  info.ident_icao                  = e.ident_icao;
    if (e.ident_iata.length())                  info.ident_iata                  = e.ident_iata;
    if (e.operator_code.length())               info.operator_code               = e.operator_code;
    if (e.operator_icao.length())               info.operator_icao               = e.operator_icao;
    if (e.operator_iata.length())               info.operator_iata               = e.operator_iata;
    if (e.aircraft_code.length())               info.aircraft_code               = e.aircraft_code;
    if (e.origin_icao.length())                 info.origin.code_icao            = e.origin_icao;
    if (e.origin_iata.length())                 info.origin.code_iata            = e.origin_iata;
    if (e.destination_icao.length())            info.destination.code_icao       = e.destination_icao;
    if (e.destination_iata.length())            info.destination.code_iata       = e.destination_iata;
    if (e.airline_display_name_full.length())   info.airline_display_name_full   = e.airline_display_name_full;
    if (e.aircraft_display_name_short.length()) info.aircraft_display_name_short = e.aircraft_display_name_short;
    if (e.aircraft_display_name_full.length())  info.aircraft_display_name_full  = e.aircraft_display_name_full;
}

}  // anonymous namespace

FlightDataFetcher::FlightDataFetcher(BaseStateVectorFetcher *stateFetcher,
                                     BaseFlightFetcher *flightFetcher)
    : _stateFetcher(stateFetcher), _flightFetcher(flightFetcher) {}

size_t FlightDataFetcher::fetchFlights(std::vector<StateVector> &outStates,
                                       std::vector<FlightInfo> &outFlights)
{
    outStates.clear();
    outFlights.clear();

    LocationConfig loc = ConfigManager::getLocation();
    bool ok = _stateFetcher->fetchStateVectors(
        loc.center_lat,
        loc.center_lon,
        loc.radius_km,
        outStates);
    if (!ok)
    {
        SystemStatus::set(Subsystem::AGGREGATOR, StatusLevel::ERROR, "Fetch failed");
        return 0;
    }

    SystemStatus::set(Subsystem::AGGREGATOR, StatusLevel::OK,
        String(outStates.size()) + " vectors");

    size_t enriched = 0;     // count of flights with enrichment data this cycle (cache hits + fresh aggregator hits)
    size_t cacheHits = 0;
    size_t aggOk = 0, aggFail = 0;
    size_t cdnOk = 0, cdnFail = 0;
    const uint32_t nowMs = millis();

    // Per-cycle CDN call budget. The FlightWall CDN resolves ICAO codes to
    // display names over HTTPS (~150-400ms per call incl. TLS handshake).
    // With 60+ flights per cycle × 2 CDN calls each, a fresh-boot cycle can
    // exceed the fetch interval and block the Core-1 loop. Cap fresh CDN
    // lookups per cycle so the cache warms up gradually; cached flights
    // already carry their display names and don't count against the budget.
    constexpr size_t CDN_CALLS_PER_CYCLE_MAX = 8;
    size_t cdnBudgetRemaining = CDN_CALLS_PER_CYCLE_MAX;

    for (const StateVector &s : outStates)
    {
        // Each iteration makes up to 2 sequential HTTPS calls (two FlightWall
        // CDN lookups for airline/aircraft display names). Reset the loop-task
        // watchdog per iteration so a large state-vector batch doesn't trip
        // the TWDT.
        esp_task_wdt_reset();

        // Always start with a FlightInfo populated from the state vector.
        // ident = callsign when available, else the icao24 transponder hex
        // so the preview can at least identify anonymous traffic.
        FlightInfo info;
        info.ident = s.callsign.length() ? s.callsign : s.icao24;

        // Merge state-vector telemetry in display units (kft, mph, deg, ft/s).
        if (!isnan(s.baro_altitude))  info.altitude_kft      = s.baro_altitude * 3.28084 / 1000.0;
        if (!isnan(s.velocity))       info.speed_mph         = s.velocity * 2.23694;
        if (!isnan(s.heading))        info.track_deg         = s.heading;
        if (!isnan(s.vertical_rate))  info.vertical_rate_fps = s.vertical_rate * 3.28084;
        if (!isnan(s.distance_km))    info.distance_km       = s.distance_km;
        if (!isnan(s.bearing_deg))    info.bearing_deg       = s.bearing_deg;

        // Enrichment path: prefer the cache (free + instant), fall back to
        // the aggregator projection for cache misses. Cache hits skip the
        // aggregator lookup entirely so every repeat visitor gets airline/
        // route restored immediately.
        if (s.callsign.length() > 0) {
            const CachedEnrichment* cached = cacheLookup(s.callsign, nowMs);
            if (cached != nullptr) {
                applyCachedEnrichment(info, *cached);
                cacheHits++;
                enriched++;
            }
        }

        // Attempt enrichment when we have a callsign and no cache hit.
        // Source is the AggregatorFetcher's in-memory map (free, no quota);
        // on success, overlay the returned airline/route/aircraft fields
        // (which may overwrite info.ident with a more canonical ICAO form —
        // deliberate).
        bool needsEnrichment = s.callsign.length() > 0
                               && info.origin.code_icao.length() == 0;
        if (needsEnrichment) {
            FlightInfo enriched_info;
            if (_flightFetcher->fetchFlightInfo(s.callsign, enriched_info)) {
                aggOk++;
                // Copy airline/route/aircraft fields from enrichment result.
                // Telemetry is already populated above from the state vector
                // and stays authoritative — state vectors are live data, the
                // aggregator's merged route metadata is not.
                if (enriched_info.ident.length())          info.ident                       = enriched_info.ident;
                if (enriched_info.ident_icao.length())     info.ident_icao                  = enriched_info.ident_icao;
                if (enriched_info.ident_iata.length())     info.ident_iata                  = enriched_info.ident_iata;
                if (enriched_info.operator_code.length())  info.operator_code               = enriched_info.operator_code;
                if (enriched_info.operator_icao.length())  info.operator_icao               = enriched_info.operator_icao;
                if (enriched_info.operator_iata.length())  info.operator_iata               = enriched_info.operator_iata;
                if (enriched_info.aircraft_code.length())  info.aircraft_code               = enriched_info.aircraft_code;
                if (enriched_info.origin.code_icao.length())      info.origin.code_icao      = enriched_info.origin.code_icao;
                if (enriched_info.origin.code_iata.length())      info.origin.code_iata      = enriched_info.origin.code_iata;
                if (enriched_info.destination.code_icao.length()) info.destination.code_icao = enriched_info.destination.code_icao;
                if (enriched_info.destination.code_iata.length()) info.destination.code_iata = enriched_info.destination.code_iata;

                // CDN display-name enrichment is bounded by the per-cycle
                // budget. Each lookup consumes 1 budget slot (airline and
                // aircraft share the pool so we can't starve either). When
                // the budget is gone we still cache what we have — ICAO
                // codes render as fallbacks and the display-name columns
                // fill in on a subsequent cycle.
                FlightWallFetcher fw;
                if (info.operator_icao.length() && cdnBudgetRemaining > 0) {
                    String airlineFull;
                    cdnBudgetRemaining--;
                    if (fw.getAirlineName(info.operator_icao, airlineFull)) {
                        info.airline_display_name_full = airlineFull;
                        cdnOk++;
                    } else {
                        cdnFail++;
                    }
                }
                if (info.aircraft_code.length() && cdnBudgetRemaining > 0) {
                    String aircraftShort, aircraftFull;
                    cdnBudgetRemaining--;
                    if (fw.getAircraftName(info.aircraft_code, aircraftShort, aircraftFull)) {
                        if (aircraftShort.length()) {
                            info.aircraft_display_name_short = aircraftShort;
                        }
                        if (aircraftFull.length()) {
                            info.aircraft_display_name_full = aircraftFull;
                        }
                        cdnOk++;
                    } else {
                        cdnFail++;
                    }
                }
                // Persist the enriched fields so the next fetch cycle can
                // skip the HTTPS round-trip for this callsign. Telemetry
                // fields are explicitly NOT cached — they're refreshed from
                // the state vector every cycle. We cache even when the CDN
                // budget ran out; the cache will get upgraded next cycle
                // when the display names fill in for this callsign.
                cacheInsert(s.callsign, info, nowMs);
                enriched++;
            } else {
                aggFail++;
            }
        }

        outFlights.push_back(info);
    }

    // Roll the enrichment counts into the AGGREGATOR status message so
    // operators see both fetch count and enrichment coverage in one row.
    // CDN (display-name) status is reported separately below.
    if (aggOk + aggFail + cacheHits > 0) {
        String msg = String(outStates.size()) + " vectors, " + String(enriched) + " enriched";
        if (cacheHits > 0) msg += " (" + String(cacheHits) + " cached)";
        StatusLevel level = StatusLevel::OK;
        if (aggFail > 0 && aggOk == 0 && cacheHits == 0) {
            level = StatusLevel::ERROR;
            msg = "All " + String(aggFail) + " lookups failed";
        } else if (aggFail > 0) {
            level = StatusLevel::WARNING;
        }
        SystemStatus::set(Subsystem::AGGREGATOR, level, msg);
    }

    // Update CDN subsystem status
    if (cdnOk + cdnFail == 0) {
        SystemStatus::set(Subsystem::CDN, StatusLevel::OK, "No lookups needed");
    } else if (cdnFail == 0) {
        SystemStatus::set(Subsystem::CDN, StatusLevel::OK,
            String(cdnOk) + " resolved");
    } else if (cdnOk == 0) {
        SystemStatus::set(Subsystem::CDN, StatusLevel::ERROR,
            "All " + String(cdnFail) + " lookups failed");
    } else {
        SystemStatus::set(Subsystem::CDN, StatusLevel::WARNING,
            String(cdnOk) + "/" + String(cdnOk + cdnFail) + " resolved");
    }

    return enriched;
}
