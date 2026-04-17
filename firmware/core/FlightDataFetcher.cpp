/*
Purpose: Orchestrate fetching and enrichment of flight data for display.
Flow:
1) Use BaseStateVectorFetcher to fetch nearby state vectors by geo filter.
2) For each callsign, use BaseFlightFetcher (e.g., AeroAPI) to retrieve FlightInfo.
3) Enrich names via FlightWallFetcher (airline/aircraft display names).
Output: Returns count of enriched flights and fills outStates/outFlights.
*/
#include "core/FlightDataFetcher.h"
#include "core/ConfigManager.h"
#include "core/SystemStatus.h"
#include "adapters/FlightWallFetcher.h"
#include <esp_task_wdt.h>

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
        SystemStatus::set(Subsystem::OPENSKY, StatusLevel::ERROR, "Fetch failed");
        return 0;
    }

    SystemStatus::set(Subsystem::OPENSKY, StatusLevel::OK,
        String(outStates.size()) + " vectors");

    size_t enriched = 0;
    size_t aeroOk = 0, aeroFail = 0;
    size_t cdnOk = 0, cdnFail = 0;

    // Hard cap on enrichment: with 60+ flights in busy airspace, fetching all of
    // them causes heap exhaustion (ArduinoJson NoMemory) and AeroAPI rate-limit
    // (HTTP 429) cascades. 20 is well within the ~160KB free heap and leaves
    // headroom for the display pipeline.
    constexpr size_t MAX_ENRICH = 20;

    for (const StateVector &s : outStates)
    {
        // Each iteration makes up to 3 sequential HTTPS calls (AeroAPI + two
        // FlightWall CDN lookups). Reset the loop-task watchdog per iteration
        // so a large state-vector batch doesn't trip the TWDT.
        esp_task_wdt_reset();
        if (enriched >= MAX_ENRICH) {
            break;
        }
        if (s.callsign.length() == 0)
        {
            continue;
        }
        FlightInfo info;
        if (_flightFetcher->fetchFlightInfo(s.callsign, info))
        {
            aeroOk++;
            FlightWallFetcher fw;
            if (info.operator_icao.length())
            {
                String airlineFull;
                if (fw.getAirlineName(info.operator_icao, airlineFull))
                {
                    info.airline_display_name_full = airlineFull;
                    cdnOk++;
                }
                else
                {
                    cdnFail++;
                }
            }
            if (info.aircraft_code.length())
            {
                String aircraftShort, aircraftFull;
                if (fw.getAircraftName(info.aircraft_code, aircraftShort, aircraftFull))
                {
                    if (aircraftShort.length())
                    {
                        info.aircraft_display_name_short = aircraftShort;
                    }
                    cdnOk++;
                }
                else
                {
                    cdnFail++;
                }
            }
            // Join state-vector telemetry with display units (Story 3.3)
            // Convert from SI (meters, m/s) to display units (kft, mph, deg, ft/s)
            if (!isnan(s.baro_altitude)) {
                info.altitude_kft = s.baro_altitude * 3.28084 / 1000.0;
            }
            if (!isnan(s.velocity)) {
                info.speed_mph = s.velocity * 2.23694;
            }
            if (!isnan(s.heading)) {
                info.track_deg = s.heading;
            }
            if (!isnan(s.vertical_rate)) {
                info.vertical_rate_fps = s.vertical_rate * 3.28084;
            }

            outFlights.push_back(info);
            enriched++;
        }
        else
        {
            aeroFail++;
        }
    }

    // Update AeroAPI subsystem status
    if (aeroOk + aeroFail == 0) {
        SystemStatus::set(Subsystem::AEROAPI, StatusLevel::OK, "No flights to enrich");
    } else if (aeroFail == 0) {
        SystemStatus::set(Subsystem::AEROAPI, StatusLevel::OK,
            String(aeroOk) + " enriched");
    } else if (aeroOk == 0) {
        SystemStatus::set(Subsystem::AEROAPI, StatusLevel::ERROR,
            "All " + String(aeroFail) + " lookups failed");
    } else {
        SystemStatus::set(Subsystem::AEROAPI, StatusLevel::WARNING,
            String(aeroOk) + "/" + String(aeroOk + aeroFail) + " enriched");
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
