# TheFlightWall — Project Documentation Index

## Project Overview

- **Type:** Monolith (single embedded firmware)
- **Primary Language:** C++ (Arduino framework)
- **Platform:** ESP32 (espressif32) via PlatformIO
- **Architecture:** Hexagonal (Ports & Adapters)

## Quick Reference

- **Tech Stack:** C++ / Arduino / ESP32 / PlatformIO
- **Entry Point:** `firmware/src/main.cpp`
- **Architecture Pattern:** Hexagonal — abstract interfaces in `interfaces/`, concrete implementations in `adapters/`, orchestration in `core/`
- **External APIs:** OpenSky Network (ADS-B data), FlightAware AeroAPI (flight metadata), FlightWall CDN (display names)
- **Hardware:** 20x WS2812B 16x16 LED panels (10x2 = 160x32px) on ESP32

## Generated Documentation

- [Project Overview](./project-overview.md)
- [Architecture](./architecture.md)
- [Source Tree Analysis](./source-tree-analysis.md)
- [Component Inventory](./component-inventory.md)
- [Development Guide](./development-guide.md)
- [API Contracts](./api-contracts.md)

## Existing Documentation

- [README.md](../README.md) — Project introduction, hardware guide, setup instructions
- [firmware/README.md](../firmware/README.md) — Firmware architecture overview and configuration quickstart

## Getting Started

1. Install [VS Code](https://code.visualstudio.com/) + [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)
2. Open the `firmware/` folder in PlatformIO
3. Configure API keys in `firmware/config/APIConfiguration.h`
4. Set WiFi credentials in `firmware/config/WiFiConfiguration.h`
5. Set your location in `firmware/config/UserConfiguration.h`
6. Connect ESP32 via USB and click Upload

See [Development Guide](./development-guide.md) for detailed instructions.
