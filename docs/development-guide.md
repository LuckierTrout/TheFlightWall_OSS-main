# Development Guide — TheFlightWall

## Prerequisites

- [VS Code](https://code.visualstudio.com/) with [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)
- ESP32 dev board (e.g., R32 D1) connected via USB
- OpenSky Network account with OAuth client credentials
- FlightAware AeroAPI account with API key

## Environment Setup

### 1. Clone and Open

Open the `firmware/` folder in VS Code with PlatformIO.

### 2. Configure API Keys

Edit `firmware/config/APIConfiguration.h`:
```cpp
static const char *OPENSKY_CLIENT_ID = "your-client-id";
static const char *OPENSKY_CLIENT_SECRET = "your-client-secret";
static const char *AEROAPI_KEY = "your-aeroapi-key";
```

### 3. Configure WiFi

Edit `firmware/config/WiFiConfiguration.h`:
```cpp
static const char *WIFI_SSID = "your-wifi-ssid";
static const char *WIFI_PASSWORD = "your-wifi-password";
```

### 4. Set Location

Edit `firmware/config/UserConfiguration.h`:
```cpp
static const double CENTER_LAT = 37.7749;   // Your latitude
static const double CENTER_LON = -122.4194; // Your longitude
static const double RADIUS_KM = 10.0;       // Search radius
```

### 5. Hardware Settings (Optional)

Edit `firmware/config/HardwareConfiguration.h` if your setup differs from the default 10x2 tile layout:
```cpp
static const uint8_t DISPLAY_PIN = 25;       // GPIO data pin
static const uint8_t DISPLAY_TILES_X = 10;   // Tiles wide
static const uint8_t DISPLAY_TILES_Y = 2;    // Tiles high
```

## Build and Flash

Using PlatformIO toolbar in VS Code:
- **Build:** Click the checkmark icon (or `pio run`)
- **Upload:** Click the arrow icon (or `pio run -t upload`)
- **Monitor:** Click the plug icon (or `pio device monitor -b 115200`)

### CLI Equivalents

```bash
cd firmware
pio run                          # Build
pio run -t upload                # Flash to ESP32
pio device monitor -b 115200     # Serial monitor
```

## Project Structure

```
firmware/
├── platformio.ini      # Build config (platform, libs, flags)
├── src/main.cpp        # Entry point
├── core/               # Business logic
├── adapters/           # External API + hardware drivers
├── interfaces/         # Abstract base classes
├── models/             # Data structures
├── config/             # Compile-time configuration
└── utils/              # Math helpers
```

## Key Build Configuration

From `platformio.ini`:
- **Platform:** espressif32
- **Board:** esp32dev
- **Framework:** Arduino
- **Monitor speed:** 115200 baud
- **Library dependency mode:** `deep+` (recursive header scanning)

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| FastLED | ^3.6.0 | WS2812B LED strip control |
| Adafruit GFX Library | ^1.11.9 | Graphics primitives (text, shapes) |
| FastLED NeoMatrix | ^1.2 | Tiled matrix abstraction over FastLED |
| ArduinoJson | ^7.4.2 | JSON parsing for API responses |

## Display Customization

In `firmware/config/UserConfiguration.h`:
- **Brightness:** `DISPLAY_BRIGHTNESS` (0–255, default: 5)
- **Text color:** `TEXT_COLOR_R/G/B` (default: 255/255/255 = white)

## Timing Configuration

In `firmware/config/TimingConfiguration.h`:
- **Fetch interval:** `FETCH_INTERVAL_SECONDS` (default: 30s) — limited by OpenSky's 4000 monthly request cap
- **Display cycle:** `DISPLAY_CYCLE_SECONDS` (default: 3s) — time per flight card when multiple flights are visible

## Testing

Unity test framework is configured (`test_framework = unity` in platformio.ini) but no tests exist yet.

To run tests when added:
```bash
cd firmware
pio test
```

## Debugging Tips

- Serial output at 115200 baud provides detailed logging for all API calls
- WiFi connection attempts are limited to 50 retries (10 seconds)
- OAuth token refresh happens automatically 60 seconds before expiry
- If a flight ident returns no results from AeroAPI, it's silently skipped
- Empty callsigns in OpenSky responses are filtered out
