# TheFlightWall Firmware

High-level overview of the firmware that powers TheFlightWall on ESP32-S3 driving a 256×192 HUB75 LED wall.

### What it does
- **Fetch the fleet snapshot** from the FlightWall Cloudflare aggregator Worker (`workers/flightwall-aggregator/`). The Worker polls adsb.lol, joins route data, caches the result in Workers KV, and serves it behind a bearer token.
- **Enrich flights** with airline/aircraft display names from the FlightWall CDN (`https://cdn.theflightwall.com`).
- **Render** flight cards / custom layouts / clock / departures board on a 256×192 HUB75 canvas (12× 64×64 panels, 4 wide × 3 tall, 1/32 scan).

### Key components
- **src/main.cpp**: Entry point. Initializes serial, Wi-Fi, the aggregator fetcher, and display. Periodically fetches/enriches and renders.
- **core/FlightDataFetcher**: Orchestrates state-vector fetch → per-callsign enrichment projection → CDN display-name lookup.
- **adapters/AggregatorFetcher**: Pulls `GET /fleet` from the Cloudflare Worker and projects the response into both `StateVector[]` (telemetry) and an in-memory enrichment map (callsign → route/aircraft/airline codes).
- **adapters/FlightWallFetcher**: Resolves ICAO airline/aircraft codes to human-readable names via the CDN.
- **adapters/HUB75MatrixDisplay**: Drives the 12-panel HUB75 chain via the `mrcodetastic/esp32-hub75-matrixpanel-dma` library. Owns one `MatrixPanel_I2S_DMA` plus a `VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>` that presents a single `Adafruit_GFX` 256×192 canvas to modes and widgets.
- **adapters/HUB75PinMap**: `constexpr` pin assignments matching the WatangTech (E) adapter silkscreen (§4.1.1). One-file edit if you use a different adapter.
- **adapters/WebPortal**: ESPAsyncWebServer — serves the setup wizard, dashboard, REST API, and OTA endpoints.
- **config/**: Compile-time defaults (only `ConfigManager.cpp` includes these directly).
- **models/**: Lightweight structs for `StateVector`, `FlightInfo`, `AirportInfo`.
- **utils/GeoUtils.h**: Haversine distance and bounding-box helpers.

### Configuration quickstart
Wi-Fi, aggregator URL/token, and location are entered in the on-device setup wizard after first boot — no code changes required. Everything else has a compile-time default:
- Compile-time display preferences in `config/UserConfiguration.h`.
- Fetch cadence in `config/TimingConfiguration.h`.
- Canvas constants (`MASTER_CANVAS_WIDTH`/`_HEIGHT`, `NOMINAL_TILE_PIXELS`) in `config/HardwareConfiguration.h`.
- HUB75 adapter pin map in `adapters/HUB75PinMap.h`.
- FlightWall CDN base URL in `config/APIConfiguration.h` (only change if you're self-hosting the CDN).

### Build
PlatformIO project: see `platformio.ini`. Envs:
- `esp32dev` — classic ESP32 (4 MB flash, 1.5 MB OTA partition). Compiles for CI sanity; not a valid runtime target post-HW-1 since the S3 is required for LCD_CAM HUB75 driving.
- `esp32s3_n16r8` — Lonely Binary ESP32-S3 N16R8 (16 MB flash; PSRAM disabled per td-7; 3 MB OTA partition) — **the primary hardware target**.
- `esp32dev_le_baseline` / `esp32dev_le_spike` — Layout Editor instrumentation builds (legacy, retained for reference).

Project builds with `gnu++17` (required by the HUB75 library's `VirtualMatrixPanel_T` `if constexpr`).

### First boot
On first flash the device starts an AP named `FlightWall-Setup`. Connect and visit `http://192.168.4.1/` to complete the setup wizard (Wi-Fi + aggregator URL/token + location). Once joined to your LAN the dashboard is at `http://flightwall.local/`.

### OTA updates
The dashboard **Firmware** card can check GitHub Releases for newer versions and download them over the air. You can also upload a `.bin` file manually via the same card.

### Notes
- Aggregator calls use `Authorization: Bearer <agg_token>`. The 15-char NVS limit abbreviates the keys to `agg_url` and `agg_token`.
- HUB75 framebuffer at 256×192 double-buffered 16bpp is ~196 KB, which exceeds internal SRAM headroom. The library auto-reduces color depth (typically to 5-6 bpc) to fit and maintain ≥60Hz refresh. Expected and documented.
- See [`docs/hardware-build-guide.md`](../docs/hardware-build-guide.md) for BOM, wiring, power, and assembly.
