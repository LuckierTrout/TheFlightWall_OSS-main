# Source Tree Analysis — TheFlightWall

## Repository Structure

**Type:** Monolith
**Root:** `firmware/`
**Platform:** ESP32 (espressif32) via PlatformIO

```
TheFlightWall_OSS-main/
├── README.md                          # Project overview, hardware guide, setup instructions
├── LICENSE                            # Project license
├── .gitignore
├── firmware/                          # ★ All firmware source lives here
│   ├── README.md                      # Firmware-specific architecture overview
│   ├── platformio.ini                 # PlatformIO build configuration
│   ├── src/
│   │   └── main.cpp                   # ★ Entry point: setup() + loop()
│   ├── core/
│   │   ├── FlightDataFetcher.h        # Orchestrator interface
│   │   └── FlightDataFetcher.cpp      # ★ Core pipeline: fetch → enrich → output
│   ├── adapters/
│   │   ├── OpenSkyFetcher.h           # OpenSky API adapter interface
│   │   ├── OpenSkyFetcher.cpp         # ★ ADS-B state vector fetcher (OAuth2)
│   │   ├── AeroAPIFetcher.h           # AeroAPI adapter interface
│   │   ├── AeroAPIFetcher.cpp         # Flight metadata enrichment (API key auth)
│   │   ├── FlightWallFetcher.h        # CDN lookup adapter interface
│   │   ├── FlightWallFetcher.cpp      # Airline/aircraft display name resolver
│   │   ├── NeoMatrixDisplay.h         # LED display adapter interface
│   │   └── NeoMatrixDisplay.cpp       # ★ WS2812B matrix rendering (flight cards)
│   ├── interfaces/
│   │   ├── BaseDisplay.h              # Abstract display port
│   │   ├── BaseFlightFetcher.h        # Abstract flight info fetcher port
│   │   └── BaseStateVectorFetcher.h   # Abstract state vector fetcher port
│   ├── models/
│   │   ├── FlightInfo.h               # Enriched flight data struct
│   │   ├── StateVector.h              # ADS-B state vector struct
│   │   └── AirportInfo.h              # Airport code container struct
│   ├── config/
│   │   ├── APIConfiguration.h         # API keys and endpoints (OpenSky, AeroAPI, CDN)
│   │   ├── WiFiConfiguration.h        # WiFi SSID and password
│   │   ├── UserConfiguration.h        # Location (lat/lon/radius), display brightness/color
│   │   ├── HardwareConfiguration.h    # LED matrix pin, tile size, tile arrangement
│   │   └── TimingConfiguration.h      # Fetch interval, display cycle timing
│   └── utils/
│       └── GeoUtils.h                 # Haversine distance, bearing, bounding box helpers
└── _bmad/                             # BMad framework (not part of project source)
```

## Critical Folders

| Folder | Purpose | Key Files |
|--------|---------|-----------|
| `firmware/src/` | Application entry point | `main.cpp` — setup WiFi, init display, run fetch loop |
| `firmware/core/` | Business logic orchestration | `FlightDataFetcher` — coordinates fetch → enrich pipeline |
| `firmware/adapters/` | External service integrations + hardware | OpenSky, AeroAPI, FlightWall CDN, NeoMatrix LED display |
| `firmware/interfaces/` | Abstract ports (dependency inversion) | `BaseDisplay`, `BaseFlightFetcher`, `BaseStateVectorFetcher` |
| `firmware/models/` | Domain data structures | `StateVector`, `FlightInfo`, `AirportInfo` |
| `firmware/config/` | Compile-time configuration | API keys, WiFi, location, hardware, timing |
| `firmware/utils/` | Math helpers | `GeoUtils` — haversine, bearing, bounding box |

## Entry Points

- **`firmware/src/main.cpp`** — `setup()` initializes hardware and WiFi; `loop()` runs the periodic fetch-and-display cycle

## File Statistics

- **Total source files:** 22
- **C++ implementation files (.cpp):** 5
- **Header files (.h):** 16
- **Configuration files:** 1 (platformio.ini)
- **Total lines of code:** ~1,200 (estimated across all source files)
