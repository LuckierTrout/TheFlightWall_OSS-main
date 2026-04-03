# Component Inventory — TheFlightWall

## Interfaces (Abstract Ports)

| Component | File | Methods | Purpose |
|-----------|------|---------|---------|
| `BaseDisplay` | `interfaces/BaseDisplay.h` | `initialize()`, `clear()`, `displayFlights(flights)` | Abstract display output port |
| `BaseFlightFetcher` | `interfaces/BaseFlightFetcher.h` | `fetchFlightInfo(ident, &out)` | Abstract flight info fetcher port |
| `BaseStateVectorFetcher` | `interfaces/BaseStateVectorFetcher.h` | `fetchStateVectors(lat, lon, radius, &out)` | Abstract state vector fetcher port |

## Adapters (Concrete Implementations)

| Component | File(s) | Implements | Lines | Purpose |
|-----------|---------|-----------|-------|---------|
| `OpenSkyFetcher` | `adapters/OpenSkyFetcher.h/.cpp` | `BaseStateVectorFetcher` | ~360 | Fetches ADS-B state vectors from OpenSky with OAuth2 |
| `AeroAPIFetcher` | `adapters/AeroAPIFetcher.h/.cpp` | `BaseFlightFetcher` | ~90 | Retrieves flight metadata from FlightAware AeroAPI |
| `FlightWallFetcher` | `adapters/FlightWallFetcher.h/.cpp` | — (standalone) | ~90 | Resolves airline/aircraft display names from CDN |
| `NeoMatrixDisplay` | `adapters/NeoMatrixDisplay.h/.cpp` | `BaseDisplay` | ~270 | Renders flight cards on WS2812B LED matrix |

## Core

| Component | File(s) | Lines | Purpose |
|-----------|---------|-------|---------|
| `FlightDataFetcher` | `core/FlightDataFetcher.h/.cpp` | ~70 | Orchestrates the fetch → enrich → output pipeline |

## Models (Data Structures)

| Model | File | Fields | Purpose |
|-------|------|--------|---------|
| `StateVector` | `models/StateVector.h` | 17 fields (icao24, callsign, lat/lon, altitude, velocity, heading, distance, bearing, etc.) | Raw ADS-B position data |
| `FlightInfo` | `models/FlightInfo.h` | 11 fields (ident, operator codes, origin/destination, aircraft code, display names) | Enriched flight metadata |
| `AirportInfo` | `models/AirportInfo.h` | 1 field (code_icao) | Airport ICAO code container |

## Configuration (Compile-Time Constants)

| Namespace | File | Key Constants |
|-----------|------|--------------|
| `APIConfiguration` | `config/APIConfiguration.h` | `OPENSKY_CLIENT_ID`, `OPENSKY_CLIENT_SECRET`, `AEROAPI_KEY`, base URLs, TLS flags |
| `WiFiConfiguration` | `config/WiFiConfiguration.h` | `WIFI_SSID`, `WIFI_PASSWORD` |
| `UserConfiguration` | `config/UserConfiguration.h` | `CENTER_LAT`, `CENTER_LON`, `RADIUS_KM`, `DISPLAY_BRIGHTNESS`, `TEXT_COLOR_R/G/B` |
| `HardwareConfiguration` | `config/HardwareConfiguration.h` | `DISPLAY_PIN`, tile pixel/count dimensions, derived matrix width/height |
| `TimingConfiguration` | `config/TimingConfiguration.h` | `FETCH_INTERVAL_SECONDS`, `DISPLAY_CYCLE_SECONDS` |

## Utilities

| Utility | File | Functions |
|---------|------|-----------|
| `GeoUtils` | `utils/GeoUtils.h` | `haversineKm()`, `computeBearingDeg()`, `centeredBoundingBox()`, `degreesToRadians()`, `radiansToDegrees()` |

## Component Dependencies

```
main.cpp
  ├── OpenSkyFetcher ──► BaseStateVectorFetcher
  │     ├── GeoUtils
  │     ├── APIConfiguration
  │     ├── UserConfiguration
  │     └── ArduinoJson, HTTPClient
  ├── AeroAPIFetcher ──► BaseFlightFetcher
  │     ├── APIConfiguration
  │     └── ArduinoJson, HTTPClient, WiFiClientSecure
  ├── FlightDataFetcher (core)
  │     ├── BaseStateVectorFetcher*
  │     ├── BaseFlightFetcher*
  │     ├── FlightWallFetcher
  │     └── UserConfiguration
  └── NeoMatrixDisplay ──► BaseDisplay
        ├── UserConfiguration
        ├── HardwareConfiguration
        ├── TimingConfiguration
        └── FastLED, Adafruit_GFX, FastLED_NeoMatrix
```
