#pragma once

#include <Arduino.h>
#include <vector>
#include "AirportInfo.h"

struct FlightInfo
{
    // Flight identifiers
    String ident;
    String ident_icao;
    String ident_iata;

    // Operator
    String operator_code;
    String operator_icao;
    String operator_iata;

    // Route
    AirportInfo origin;
    AirportInfo destination;

    // Aircraft
    String aircraft_code;

    // Human-friendly display strings
    String airline_display_name_full;
    String aircraft_display_name_short;

    // Display-ready telemetry (converted from StateVector units in pipeline)
    // NAN indicates missing/unavailable data — renderer shows "--" placeholders.
    double altitude_kft = NAN;       // Barometric altitude in thousands of feet
    double speed_mph = NAN;          // Ground speed in miles per hour
    double track_deg = NAN;          // Track heading in degrees (0-359)
    double vertical_rate_fps = NAN;  // Vertical rate in feet per second
};
