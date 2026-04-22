#pragma once

#include <Arduino.h>

namespace APIConfiguration
{
    // FlightWall CDN lookup — airline/aircraft display-name enrichment layered
    // on top of the aggregator's raw codes. See firmware/adapters/FlightWallFetcher.
    static constexpr const char *FLIGHTWALL_CDN_BASE_URL = "https://cdn.theflightwall.com";

    // Legacy TLS-insecure flag kept for the FlightWall CDN HTTPS client — the
    // CDN cert pin isn't baked into the firmware, so we fall back to TLS
    // without strict cert validation. Revisit when cert pinning lands.
    static const bool FLIGHTWALL_INSECURE_TLS = true;
}
