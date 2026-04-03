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
#include "adapters/FlightWallFetcher.h"

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
        return 0;

    size_t enriched = 0;
    for (const StateVector &s : outStates)
    {
        if (s.callsign.length() == 0)
        {
            continue;
        }
        FlightInfo info;
        if (_flightFetcher->fetchFlightInfo(s.callsign, info))
        {
            FlightWallFetcher fw;
            if (info.operator_icao.length())
            {
                String airlineFull;
                if (fw.getAirlineName(info.operator_icao, airlineFull))
                {
                    info.airline_display_name_full = airlineFull;
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
    }
    return enriched;
}
