# TheFlightWall Firmware

This is a high-level overview of the firmware that powers TheFlightWall on ESP32 / ESP32-S3.

### What it does
- **Fetch the fleet snapshot** from the FlightWall Cloudflare aggregator Worker (`workers/flightwall-aggregator/`). The Worker polls adsb.lol, joins route data, caches the result in Workers KV, and serves it behind a bearer token.
- **Enrich flights** with airline/aircraft display names from the FlightWall CDN (`https://cdn.theflightwall.com`).
- **Render** flight cards / custom layouts / clock / departures board on a tiled WS2812B LED matrix.

### Key components
- **src/main.cpp**: Entry point. Initializes serial, Wi-Fi, the aggregator fetcher, and display. Periodically fetches/enriches and renders.
- **core/FlightDataFetcher**: Orchestrates state-vector fetch → per-callsign enrichment projection → CDN display-name lookup.
- **adapters/AggregatorFetcher**: Pulls `GET /fleet` from the Cloudflare Worker and projects the response into both `StateVector[]` (telemetry) and an in-memory enrichment map (callsign → route/aircraft/airline codes).
- **adapters/FlightWallFetcher**: Resolves ICAO airline/aircraft codes to human-readable names via the CDN.
- **adapters/NeoMatrixDisplay**: Renders to the tiled LED matrix.
- **adapters/WebPortal**: ESPAsyncWebServer — serves the setup wizard, dashboard, REST API, and OTA endpoints.
- **config/**: Compile-time defaults (only `ConfigManager.cpp` includes these directly).
- **models/**: Lightweight structs for `StateVector`, `FlightInfo`, `AirportInfo`.
- **utils/GeoUtils.h**: Haversine distance and bounding-box helpers.

### Configuration quickstart
Wi-Fi, aggregator URL/token, and location are entered in the on-device setup wizard after first boot — no code changes required. Everything else has a compile-time default:
- Compile-time display preferences in `config/UserConfiguration.h`.
- Fetch cadence in `config/TimingConfiguration.h`.
- Display pin / tile layout defaults in `config/HardwareConfiguration.h`.
- FlightWall CDN base URL in `config/APIConfiguration.h` (only change if you're self-hosting the CDN).

### Build
PlatformIO project: see `platformio.ini`. Envs:
- `esp32dev` — classic ESP32 (4 MB flash, 1.5 MB OTA partition)
- `esp32s3_n16r8` — Lonely Binary ESP32-S3 N16R8 (16 MB flash + 8 MB OPI PSRAM, 3 MB OTA partition) — the current primary target
- `esp32dev_le_baseline` / `esp32dev_le_spike` — Layout Editor instrumentation builds

### First boot
On first flash the device starts an AP named `FlightWall-Setup`. Connect and visit `http://192.168.4.1/` to complete the setup wizard (Wi-Fi + aggregator URL/token + location). Once joined to your LAN the dashboard is at `http://flightwall.local/`.

### OTA updates
The dashboard **Firmware** card can check GitHub Releases for newer versions and download them over the air. You can also upload a `.bin` file manually via the same card.

### Notes
- Aggregator calls use `Authorization: Bearer <agg_token>`. The 15-char NVS limit abbreviates the keys to `agg_url` and `agg_token`.
- Display uses `FastLED_NeoMatrix` with WS2812B strips; adjust tiling/orientation in hardware config.
