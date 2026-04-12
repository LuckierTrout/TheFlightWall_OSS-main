# FlightWall OSS — Complete Setup Guide

A step-by-step guide from unboxing to displaying live flights on your wall.

---

## What You'll Need

### Hardware

- **ESP32 dev board** (esp32dev — e.g., R32 D1 or any ESP32 devkit)
- **WS2812B LED panels** — default config is 10 tiles wide x 2 tiles high (16x16 pixel panels = 160x32 total)
- **5V power supply** — minimum 20A for 20 panels (LEDs are power hungry)
- **3.3V to 5V level shifter** — ESP32 outputs 3.3V logic, WS2812B needs 5V data signal
- **USB data cable** — must be a data-capable cable, not charge-only

### Software

- macOS, Linux, or Windows terminal
- PlatformIO (CLI or VS Code extension)
- A web browser

### Accounts (Optional but recommended)

- **OpenSky Network** account — free, provides aircraft tracking data
- **FlightAware AeroAPI key** — enriches flights with airline/route info

---

## Phase 1: Install PlatformIO

**macOS (Homebrew):**

```bash
brew install platformio
```

**Verify it works:**

```bash
pio --version
```

If `pio` isn't found in your PATH (common on macOS):

```bash
~/.platformio/penv/bin/pio --version
```

> PlatformIO manages the entire ESP32 toolchain — compiler, uploader, library dependencies. You don't need to install the Arduino IDE or any ESP32 SDK separately.

---

## Phase 2: Wire the Hardware

1. **Connect LED data line** to ESP32 **GPIO 25** (default pin) through the level shifter
2. **Connect LED power** to your 5V supply — do NOT power 20 panels from the ESP32's USB port
3. **Connect ESP32** to your computer via USB data cable
4. **Common ground** — ESP32 GND, LED GND, and power supply GND must all be connected together

### Default Pin Configuration

| Signal      | GPIO | Notes                              |
| ----------- | ---- | ---------------------------------- |
| LED Data    | 25   | Configurable via dashboard later   |
| Boot Button | 0    | Built into most ESP32 devkits      |

Valid alternative data pins: `0, 2, 4, 5, 12-19, 21-23, 25-27, 32, 33`

---

## Phase 3: Configure Before First Flash

Edit these files in `firmware/config/`:

### WiFiConfiguration.h

```cpp
static const char *WIFI_SSID = "";        // Leave empty for AP setup mode
static const char *WIFI_PASSWORD = "";    // Leave empty for AP setup mode
```

You have two choices:

- **Leave blank** — the device boots into AP setup mode with a browser-based wizard (recommended for first boot)
- **Fill them in** — the device connects to your WiFi immediately on first boot

### APIConfiguration.h

```cpp
static const char *OPENSKY_CLIENT_ID = "your-opensky-username";
static const char *OPENSKY_CLIENT_SECRET = "your-opensky-password";
static const char *AEROAPI_KEY = "your-flightaware-key";  // Optional
```

### UserConfiguration.h

```cpp
static const double CENTER_LAT = 37.7749;     // Your latitude
static const double CENTER_LON = -122.4194;   // Your longitude
static const double RADIUS_KM = 10.0;         // Search radius in km
```

> These compile-time values are just defaults. Every one of them can be changed later through the web dashboard without reflashing.

---

## Phase 4: Build the Firmware

From your terminal:

```bash
cd firmware
pio run -e esp32dev
```

Or if `pio` is not in PATH:

```bash
cd firmware
~/.platformio/penv/bin/pio run -e esp32dev
```

Wait for `SUCCESS`. First build downloads all dependencies automatically (~2-3 minutes). Subsequent builds are faster.

---

## Phase 5: Flash the Firmware to the ESP32

```bash
pio run -e esp32dev -t upload
```

If auto-detect fails (no serial port found):

```bash
# List available ports
ls /dev/cu.usb*

# Flash to specific port
pio run -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX
```

> If upload hangs, hold the **BOOT button** (GPIO 0) on the ESP32 while upload starts. Some boards don't auto-reset into flash mode.

---

## Phase 6: Upload Web Assets to LittleFS

**This step is critical.** The firmware and the web UI are separate uploads. Skip this and you get a blank page in the browser.

```bash
pio run -e esp32dev -t uploadfs
```

> The web portal (dashboard, setup wizard, health page) lives on the ESP32's flash filesystem (LittleFS). The files in `firmware/data/` are pre-gzipped — the ESP32 serves them compressed directly, saving RAM and transfer time. This is a separate flash partition from the firmware itself, which is why it's a separate upload step.

---

## Phase 7: Open Serial Monitor

```bash
pio device monitor -b 115200
```

You should see boot output like:

```
[ConfigManager] Initialized - 23 keys loaded
[WiFiManager] No credentials - entering AP Setup mode
[WiFiManager] AP SSID: FlightWall-Setup
[WiFiManager] AP IP: 192.168.4.1
[WebPortal] Server started
```

Write down the IP address — you'll need it next.

---

## Phase 8: First-Boot Setup (AP Mode)

If you left WiFi credentials blank (recommended), the device creates its own WiFi network:

1. **On your phone or laptop**, connect to WiFi network: **FlightWall-Setup**
2. **Open a browser** and go to: **http://192.168.4.1/**
3. **The setup wizard loads** — walk through it:
   - Select your WiFi network (it scans automatically)
   - Enter WiFi password
   - Enter API credentials
   - Set your location (lat/lon/radius)
   - Configure display settings
4. **Hit Save** — device reboots and connects to your WiFi

> To get back to AP setup mode later, hold the **BOOT button** during power-on for >500ms. This forces AP mode regardless of stored credentials.

---

## Phase 9: Access the Dashboard (Normal Mode)

After reboot, the device connects to your WiFi. Access it at:

- **mDNS:** http://flightwall.local/ (may not work on all networks)
- **Direct IP:** http://\<device-ip\>/ (check serial monitor for the assigned IP)

### Available Pages

| URL              | What it does                                              |
| ---------------- | --------------------------------------------------------- |
| `/`              | Main dashboard — flight display, settings, logo management |
| `/health.html`   | System health — uptime, memory, WiFi signal, API status    |

---

## Phase 10: Upload Airline Logos (Optional)

From the dashboard:

1. Click the **Logos** section
2. **Drag and drop** `.bin` files (32x32 pixels, RGB565 format, exactly 2048 bytes each)
3. Preview renders before upload so you can verify the logo looks correct
4. Files are named by ICAO code — e.g., `UAL.bin` for United Airlines, `DAL.bin` for Delta

When a flight appears from a matching airline, the logo renders on the LED matrix instead of the default airplane silhouette.

---

## Quick Reference: Daily Commands

```bash
# Build only (check for errors)
pio run -e esp32dev

# Build + flash firmware
pio run -e esp32dev -t upload

# Upload web assets
pio run -e esp32dev -t uploadfs

# Serial monitor
pio device monitor -b 115200
```

### Change Settings Without Reflashing

```bash
# Get current settings
curl http://flightwall.local/api/settings

# Update a setting
curl -X POST http://flightwall.local/api/settings \
  -H "Content-Type: application/json" \
  -d '{"brightness": 10}'

# Reboot the device
curl -X POST http://flightwall.local/api/reboot
```

---

## REST API Reference

| Method | Endpoint                  | Description                        |
| ------ | ------------------------- | ---------------------------------- |
| GET    | `/api/settings`           | Get all configuration              |
| POST   | `/api/settings`           | Update settings (JSON body)        |
| GET    | `/api/status`             | System health and flight stats     |
| GET    | `/api/layout`             | Display layout zones               |
| GET    | `/api/wifi/scan`          | Scan available WiFi networks       |
| POST   | `/api/reboot`             | Reboot device                      |
| POST   | `/api/reset`              | Factory reset (erase NVS)          |
| POST   | `/api/calibration/start`  | Start LED test pattern             |
| POST   | `/api/calibration/stop`   | Stop LED test pattern              |
| GET    | `/api/logos`              | List uploaded logos with storage    |
| POST   | `/api/logos`              | Upload logo (multipart/form-data)  |
| DELETE  | `/api/logos/:name`       | Delete a logo by filename          |

---

## Troubleshooting

| Problem                                | Fix                                                                           |
| -------------------------------------- | ----------------------------------------------------------------------------- |
| `pio: command not found`               | Use `~/.platformio/penv/bin/pio`                                              |
| Upload fails / no serial port          | Check USB cable is data-capable. Try `ls /dev/cu.usb*`                        |
| `flightwall.local` doesn't resolve     | Use the direct IP from serial monitor                                         |
| Browser shows blank page / 404         | Run `pio run -e esp32dev -t uploadfs` — web assets are missing from LittleFS  |
| WiFi won't connect                     | Hold BOOT button during startup to force AP setup mode                        |
| LEDs show nothing                      | Verify data pin matches config, check power supply, check level shifter       |
| Need to factory reset                  | `curl -X POST http://flightwall.local/api/reset` — erases all stored config   |

---

## Modifying Web Assets

The web UI source files live in `firmware/data-src/`. The ESP32 serves gzipped copies from `firmware/data/`.

If you edit a source file, regenerate the gzipped version before uploading:

```bash
gzip -9 -c data-src/dashboard.html > data/dashboard.html.gz
gzip -9 -c data-src/dashboard.js > data/dashboard.js.gz
gzip -9 -c data-src/style.css > data/style.css.gz
# Repeat for any modified file
```

Then re-upload:

```bash
pio run -e esp32dev -t uploadfs
```

---

## Architecture Notes

- **Dual-core design**: Core 1 handles WiFi, HTTP, API fetching, and configuration. Core 0 handles LED rendering and flight cycling at 20 FPS.
- **Settings persistence**: ConfigManager stores settings in NVS (Non-Volatile Storage). Compile-time values in config headers are fallback defaults only.
- **Hot-reload settings**: Brightness, text color, location, fetch interval, and display cycle can be changed via the dashboard without rebooting.
- **Reboot-required settings**: WiFi credentials, API keys, display pin, and tile configuration require a reboot to take effect.
