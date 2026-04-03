# Project Overview — TheFlightWall

## What It Is

TheFlightWall is an LED wall that displays live information about flights passing near your location. It runs on an ESP32 microcontroller driving a 160x32 pixel WS2812B LED matrix (20 panels in a 10x2 arrangement).

## How It Works

1. **Fetch** — Queries OpenSky Network for ADS-B state vectors within a configurable radius of a GPS coordinate
2. **Enrich** — Looks up flight metadata (airline, aircraft type, origin/destination) from FlightAware AeroAPI
3. **Resolve** — Fetches human-friendly display names (airline name, aircraft model) from FlightWall CDN
4. **Display** — Renders bordered three-line flight cards on the LED matrix, cycling through active flights

## Quick Reference

| Attribute | Value |
|-----------|-------|
| **Type** | Embedded firmware (monolith) |
| **Language** | C++ (Arduino framework) |
| **Platform** | ESP32 (espressif32) |
| **Build System** | PlatformIO |
| **LED Hardware** | WS2812B 16x16 panels (10x2 = 160x32px) |
| **Architecture** | Hexagonal (Ports & Adapters) |
| **External APIs** | OpenSky Network, FlightAware AeroAPI, FlightWall CDN |
| **Libraries** | FastLED 3.6+, Adafruit GFX 1.11+, FastLED NeoMatrix 1.2+, ArduinoJson 7.4+ |
| **Test Framework** | Unity (configured, no tests yet) |

## Repository Structure

Single-part monolith. All source code lives under `firmware/`.

```
firmware/
├── src/          → Entry point (main.cpp)
├── core/         → Business logic orchestration
├── adapters/     → API clients + LED display driver
├── interfaces/   → Abstract base classes (ports)
├── models/       → Data structures
├── config/       → Compile-time settings
└── utils/        → Math helpers
```

## External Dependencies

### Hardware
- ESP32 dev board (e.g., R32 D1)
- 20x 16x16 WS2812B LED panels
- 5V 20A+ power supply
- 3.3V–5V voltage level shifter

### APIs
- **OpenSky Network** — ADS-B flight position data (OAuth2 client credentials)
- **FlightAware AeroAPI** — Flight route, airline, aircraft lookup (API key)
- **FlightWall CDN** — Airline/aircraft display name resolution (public, no auth)

## Links

- Product website: [theflightwall.com](https://theflightwall.com)
- Viral build video: [Instagram](https://www.instagram.com/p/DLIbAtbJxPl)
