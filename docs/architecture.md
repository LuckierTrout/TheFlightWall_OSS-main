# Architecture — TheFlightWall Firmware

## Executive Summary

TheFlightWall firmware follows a **Hexagonal (Ports & Adapters)** architecture. Abstract interfaces define the system's boundaries, concrete adapters implement external integrations, and a core orchestrator ties the data pipeline together. This clean separation enables swapping data sources or display hardware without modifying business logic.

## Architecture Pattern

```
┌─────────────────────────────────────────────────────────┐
│                      main.cpp                           │
│              (setup + periodic loop)                    │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              FlightDataFetcher (core)                   │
│         Orchestrates: fetch → enrich → output           │
│                                                         │
│  Uses:                                                  │
│    BaseStateVectorFetcher* ──► OpenSkyFetcher           │
│    BaseFlightFetcher*      ──► AeroAPIFetcher           │
│    FlightWallFetcher       ──► CDN name resolution      │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│             NeoMatrixDisplay (adapter)                   │
│    Renders flight cards on WS2812B LED matrix           │
│    Implements BaseDisplay interface                      │
└─────────────────────────────────────────────────────────┘
```

## Technology Stack

| Category | Technology | Version | Notes |
|----------|-----------|---------|-------|
| Language | C++ | C++11 | Arduino framework dialect |
| Platform | ESP32 | espressif32 | Via PlatformIO |
| Build System | PlatformIO | — | `platformio.ini` |
| LED Control | FastLED | ^3.6.0 | WS2812B driver |
| Graphics | Adafruit GFX | ^1.11.9 | Text/shape rendering primitives |
| Matrix Abstraction | FastLED NeoMatrix | ^1.2 | Tiled matrix support |
| JSON Parsing | ArduinoJson | ^7.4.2 | API response parsing |
| Networking | WiFi + HTTPClient | ESP32 core | Built-in Arduino ESP32 |
| TLS | WiFiClientSecure | ESP32 core | HTTPS for API calls |
| Test Framework | Unity | — | Configured but unused |

## Component Architecture

### Interfaces (Ports)

| Interface | Purpose | Method |
|-----------|---------|--------|
| `BaseStateVectorFetcher` | Fetch ADS-B position data | `fetchStateVectors(lat, lon, radius, &out)` |
| `BaseFlightFetcher` | Fetch flight metadata by ident | `fetchFlightInfo(ident, &out)` |
| `BaseDisplay` | Render flight data visually | `initialize()`, `clear()`, `displayFlights(flights)` |

### Adapters (Implementations)

| Adapter | Implements | External Service | Auth Method |
|---------|-----------|-----------------|-------------|
| `OpenSkyFetcher` | `BaseStateVectorFetcher` | OpenSky Network `states/all` | OAuth2 client_credentials |
| `AeroAPIFetcher` | `BaseFlightFetcher` | FlightAware AeroAPI `/flights/{ident}` | API key (`x-apikey` header) |
| `FlightWallFetcher` | — (standalone) | FlightWall CDN `/oss/lookup/` | None (public) |
| `NeoMatrixDisplay` | `BaseDisplay` | WS2812B LED matrix (hardware) | N/A |

### Core

| Component | Purpose |
|-----------|---------|
| `FlightDataFetcher` | Pipeline orchestrator: fetches state vectors → enriches each with flight info → resolves display names |

### Models

| Model | Purpose | Key Fields |
|-------|---------|------------|
| `StateVector` | ADS-B position data | icao24, callsign, lat/lon, altitude, velocity, heading, distance_km, bearing_deg |
| `FlightInfo` | Enriched flight metadata | ident (ICAO/IATA), operator codes, origin/destination airports, aircraft code, display names |
| `AirportInfo` | Airport identifier | code_icao |

### Utilities

| Utility | Functions |
|---------|-----------|
| `GeoUtils` | `haversineKm()` — great-circle distance; `computeBearingDeg()` — initial bearing; `centeredBoundingBox()` — lat/lon bounds for API queries |

## Data Flow

```
1. Timer fires (every 30s default)
         │
         ▼
2. OpenSkyFetcher.fetchStateVectors()
   ├── Ensure OAuth token (auto-refresh with 60s skew)
   ├── Compute bounding box from center + radius
   ├── GET /api/states/all?lamin=...&lamax=...&lomin=...&lomax=...
   ├── Parse JSON array into StateVector structs
   └── Filter by haversine distance ≤ radius
         │
         ▼
3. For each StateVector with a callsign:
   ├── AeroAPIFetcher.fetchFlightInfo(callsign)
   │   ├── GET /flights/{ident} with x-apikey header
   │   └── Parse first flight from JSON response
   │
   ├── FlightWallFetcher.getAirlineName(operator_icao)
   │   └── GET /oss/lookup/airline/{icao}.json
   │
   └── FlightWallFetcher.getAircraftName(aircraft_code)
       └── GET /oss/lookup/aircraft/{icao}.json
         │
         ▼
4. NeoMatrixDisplay.displayFlights(enrichedFlights)
   ├── Render bordered 3-line flight card
   │   ├── Line 1: Airline name
   │   ├── Line 2: Origin > Destination (ICAO codes)
   │   └── Line 3: Aircraft type
   └── Cycle through flights every 3s (configurable)
```

## Configuration Architecture

All configuration is compile-time via C++ namespace constants in header files:

| File | Scope | Key Settings |
|------|-------|-------------|
| `APIConfiguration.h` | API credentials/URLs | OpenSky client ID/secret, AeroAPI key, CDN base URL, TLS settings |
| `WiFiConfiguration.h` | Network | SSID, password |
| `UserConfiguration.h` | User preferences | Center lat/lon, radius (km), brightness (0–255), text RGB color |
| `HardwareConfiguration.h` | Display hardware | Data pin (GPIO 25), tile dimensions (16x16), tile count (10x2) |
| `TimingConfiguration.h` | Timing | Fetch interval (30s), display cycle (3s) |

## Security Considerations

- API credentials are stored as compile-time constants (not environment variables)
- OpenSky uses OAuth2 client_credentials with automatic token refresh
- AeroAPI and FlightWall CDN connections use `WiFiClientSecure` but with `setInsecure()` (TLS certificate verification disabled) — configurable via `AEROAPI_INSECURE_TLS` and `FLIGHTWALL_INSECURE_TLS` flags
- No OTA update mechanism present

## Testing

- Unity test framework is configured in `platformio.ini` (`test_framework = unity`)
- No test files exist yet in the codebase
