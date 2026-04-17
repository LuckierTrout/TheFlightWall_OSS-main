# TheFlightWall OSS — Gemini Context

## Project Overview

**TheFlightWall** is an open-source ESP32-based firmware designed to control a large-scale LED wall (typically 20x 16x16 panels) that displays live flight information. It fetches real-time ADS-B data for nearby flights and enriches it with detailed metadata (airline name, route, aircraft type) for a "split-flap" style display experience.

### Core Technologies
- **Hardware:** ESP32 (e.g., R32 D1)
- **Framework:** Arduino (via PlatformIO)
- **Display:** WS2812B LED Matrix (FastLED, Adafruit GFX, FastLED NeoMatrix)
- **Data Sources:** OpenSky Network (ADS-B), FlightAware AeroAPI (Metadata), FlightWall CDN (Lookup)
- **Architecture:** Hexagonal (Ports & Adapters)

## Building and Running

The project uses **PlatformIO** for dependency management and build orchestration. All commands should be run from the `firmware/` directory.

### Prerequisites
- [VS Code](https://code.visualstudio.com/) with [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)
- ESP32 development board

### Configuration
Configuration is managed via compile-time constants in `firmware/config/`. You **must** update these files before building:
1. `APIConfiguration.h`: Add OpenSky and FlightAware AeroAPI keys.
2. `WiFiConfiguration.h`: Add your WiFi SSID and Password.
3. `UserConfiguration.h`: Set your `CENTER_LAT`, `CENTER_LON`, and `RADIUS_KM`.
4. `HardwareConfiguration.h`: Adjust `DISPLAY_PIN` and tile layout if necessary.

### Key Commands (CLI)
```bash
cd firmware
pio run                          # Build the project
pio run -t upload                # Build and flash to ESP32
pio device monitor -b 115200     # Open serial monitor (115200 baud)
```

## Testing

The project uses the **Unity** test framework via PlatformIO.

### Running Tests
```bash
cd firmware
pio test
```
Tests are located in the `firmware/test/` directory and cover core components like `ModeOrchestrator`, `ConfigManager`, and `LayoutEngine`.

## Development Conventions

### Architecture: Hexagonal (Ports & Adapters)
- **Interfaces (`firmware/interfaces/`):** Define abstract base classes (e.g., `BaseDisplay`, `BaseFlightFetcher`).
- **Adapters (`firmware/adapters/`):** Concrete implementations of interfaces (e.g., `NeoMatrixDisplay`, `AeroAPIFetcher`).
- **Core (`firmware/core/`):** Business logic and orchestrators (e.g., `FlightDataFetcher`, `ModeOrchestrator`).
- **Models (`firmware/models/`):** Plain data structures (e.g., `FlightInfo`, `StateVector`).

### Coding Standards
- **Memory Management:** Prefer static allocation or pre-allocated buffers over frequent heap allocation to avoid fragmentation on ESP32.
- **Async Operations:** Core 0 is typically used for display tasks (FastLED), while Core 1 handles network requests and data processing.
- **Logging:** Use serial logging at 115200 baud. Log levels are controlled via `LOG_LEVEL` build flag in `platformio.ini`.
- **Configuration:** Always use the `firmware/config/` headers for user-facing or hardware-specific settings.

### Directory Structure
- `firmware/src/main.cpp`: Main entry point (setup/loop).
- `firmware/core/`: Central logic and managers.
- `firmware/adapters/`: Hardware and API implementations.
- `firmware/interfaces/`: System boundaries.
- `firmware/config/`: Compile-time settings.
- `docs/`: Detailed architecture and setup guides.
