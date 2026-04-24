# TheFlightWall

TheFlightWall is an LED wall which shows live information of flights going by your window.

This is the open source version with some basic guides to the panels, mounting them together, data services, and code. Check out our viral build video: [https://www.instagram.com/p/DLIbAtbJxPl](https://www.instagram.com/p/DLIbAtbJxPl)

**Don't feel like building one? Check out the offical product: [theflightwall.com](https://theflightwall.com)**

![Main Image](images/main-image.png)
*Airline logo lookup will be added soon!*

# Component List

See [`docs/hardware-build-guide.md`](docs/hardware-build-guide.md) for the full bill of materials, wiring reference, and assembly steps.

**Quick summary:**
- **MCU:** ESP32-S3 N16R8 (Lonely Binary Gold Edition recommended — 16MB flash, PSRAM disabled for Wi-Fi compatibility)
- **Adapter:** 1× WatangTech RGB Matrix Adapter Board (E) — HUB75 level shifter + power distribution, E-pin broken out for 1/32 scan panels
- **Display:** 12× 64×64 HUB75 LED panels (1/32 scan), arranged 4 wide × 3 tall → **256×192 canvas**
- **Power:** 5V / 50A PSU (≈240W peak at full white across 12 panels)
- **Wiring:** 12× HUB75 ribbons (1 adapter→panel + 11 panel→panel serpentine) + 12× VH-4P power pigtails with multi-point injection
- **Mounting:** 48+ M3 magnetic screws + steel/iron backer plate
- **Data:**
  - FlightWall **aggregator Worker** (Cloudflare) that pulls ADS-B state vectors from [adsb.lol](https://adsb.lol/), joins route metadata, and exposes a bearer-protected endpoint to the ESP32. See [`workers/flightwall-aggregator/`](workers/flightwall-aggregator/).
  - FlightWall **CDN** for airline / aircraft display-name enrichment (resolves ICAO codes like `UAL` / `B738` to `United Airlines` / `Boeing 737-800`).

> **NOTE:** Earlier versions of this project used WS2812B addressable strips (`20× 16×16` panels, 160×32 canvas) driven via a single data pin with FastLED. Epic `hw-1` migrated to HUB75 in April 2026. If you're looking at old photos, old BOM links, or older forks, they'll show that older configuration — the current canonical hardware is the 12-panel HUB75 build above.

# Hardware

See [`docs/hardware-build-guide.md`](docs/hardware-build-guide.md) for wiring diagrams, power distribution, panel mounting, and first-boot steps.

## Dimensions

At P3 pitch (3mm LED spacing): ~24" wide × ~18" tall. At P4: ~32" × ~24". Measure the specific panels you buy — pitch varies.

# Data and Software

## Data Pipeline

The firmware no longer talks to OpenSky or FlightAware directly. All flight data is served by a Cloudflare Worker (the "aggregator") that sits in front of the upstream feeds and returns a single normalized fleet snapshot.

1. **Aggregator Worker** — [`workers/flightwall-aggregator/`](workers/flightwall-aggregator/). Polls [adsb.lol](https://adsb.lol/) on a cron, joins route metadata, caches the result in Cloudflare KV, and serves `GET /fleet` behind `Authorization: Bearer <ESP32_AUTH_TOKEN>`.
2. **FlightWall CDN** — `https://cdn.theflightwall.com`. Resolves ICAO airline/aircraft codes to human-readable names. Called from the ESP32 for display enrichment.

### Setting up the aggregator

Deploy the Worker once (see [`workers/flightwall-aggregator/README.md`](workers/flightwall-aggregator/README.md) for `wrangler` setup and the `ESP32_AUTH_TOKEN` secret). After deploy you'll have a URL like `https://flightwall-aggregator.<subdomain>.workers.dev`.

### Configuring the ESP32

Credentials are **not** compiled into the firmware — they're entered in the setup wizard on first boot and persisted to NVS. The wizard asks for:

- Wi-Fi SSID + password
- `agg_url` — the deployed Worker URL
- `agg_token` — the bearer token matching the Worker's `ESP32_AUTH_TOKEN`

## Software Setup

### Set your WiFi

Enter your WiFi credentials into `WIFI_SSID` and `WIFI_PASSWORD` in [WiFiConfiguration.h](firmware/config/WiFiConfiguration.h)

### Set your location

Set your location to track flights by updating the following values in [UserConfiguration.h](firmware/config/UserConfiguration.h):

- `CENTER_LAT`: Latitude of the center point to track (e.g., your home or city)
- `CENTER_LON`: Longitude of the center point
- `RADIUS_KM`: Search radius in kilometers for flights to include

### Build and flash with PlatformIO

The firmware can be built and uploaded to the ESP32 using [PlatformIO](https://platformio.org/)

1. **Install PlatformIO**: 
   - Install [VS Code](https://code.visualstudio.com/)
   - Add the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)

2. **Configure your settings**:
   - Wi-Fi, aggregator URL/token, and location are entered in the on-device setup wizard — no code changes needed for those.
   - Compile-time defaults for display preferences live in [UserConfiguration.h](firmware/config/UserConfiguration.h).
   - Display hardware constants (canvas width/height, nominal tile size) in [HardwareConfiguration.h](firmware/config/HardwareConfiguration.h). The HUB75 pin map is fixed in [HUB75PinMap.h](firmware/adapters/HUB75PinMap.h) to match the WatangTech (E) adapter's silkscreen routing — change only if you're using a different adapter.
   - The FlightWall CDN base URL lives in [APIConfiguration.h](firmware/config/APIConfiguration.h) (only change this if you're self-hosting the CDN).

3. **Build and upload**:
   - Open the `firmware` folder in PlatformIO
   - Connect your ESP32 via USB
   - Click the "Upload" button (→) in the PlatformIO toolbar

### Customization

- **Brightness**: Controls overall display brightness (0–255)
  - Edit `DISPLAY_BRIGHTNESS` in [UserConfiguration.h](firmware/config/UserConfiguration.h)
- **Text color**: RGB values used for all text/borders
  - Edit `TEXT_COLOR_R`, `TEXT_COLOR_G`, `TEXT_COLOR_B` in [UserConfiguration.h](firmware/config/UserConfiguration.h)

We may add more customization options in the future, but of course this being open source the whole thing is customizable to your liking.

## Firmware & OTA

See [`firmware/README.md`](firmware/README.md) for build/flash instructions and [`firmware/platformio.ini`](firmware/platformio.ini) for the PlatformIO configuration.

**First boot:** The ESP32 starts in AP mode as `FlightWall-Setup`. Connect to this network and navigate to `http://flightwall.local/` (or `192.168.4.1`) to run the setup wizard. Once Wi-Fi is configured the device connects to your network and the dashboard is available at `http://flightwall.local/`.

**OTA updates:** The dashboard Firmware card can check GitHub Releases for new versions and download them directly to the device, or you can upload a `.bin` file manually.

# Thanks
We really appreciate all the support on this project!

If you don't want to build one but still find it cool, check out our offical displays: **[https://theflightwall.com](https://theflightwall.com)**

Excited to see your builds :) Tag @theflightwall on IG