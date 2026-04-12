# Device Setup and Flash Guide

This guide walks you through the full first-time flow:

1. Install tooling
2. Configure firmware
3. Build and flash ESP32
4. Upload web portal assets (LittleFS)
5. Open the app in your browser (AP setup and STA dashboard)
6. Run a smoke test

## 1) Prerequisites

- ESP32 board connected by USB
- LED matrix hardware connected to the ESP32
- macOS terminal (zsh)
- Optional but recommended:
  - OpenSky API client credentials
  - FlightAware AeroAPI key

## 2) Install PlatformIO CLI

From the project root:

```bash
brew install platformio
pio --version
```

If `pio` is not found in your shell, use:

```bash
~/.platformio/penv/bin/pio --version
```

## 3) Configure the firmware

Edit these files before first flash:

- `firmware/config/WiFiConfiguration.h`
  - `WIFI_SSID`
  - `WIFI_PASSWORD`
- `firmware/config/APIConfiguration.h`
  - `OPENSKY_CLIENT_ID`
  - `OPENSKY_CLIENT_SECRET`
  - `AEROAPI_KEY`
- `firmware/config/UserConfiguration.h`
  - `CENTER_LAT`
  - `CENTER_LON`
  - `RADIUS_KM`
- `firmware/config/HardwareConfiguration.h`
  - Update pin/tile settings if your panel layout differs

Notes:

- If Wi-Fi credentials are left empty, the device starts in setup AP mode (`FlightWall-Setup`).
- If valid Wi-Fi credentials are present, the device attempts STA mode on boot.

## 4) Build firmware

```bash
cd firmware
pio run -e esp32dev
```

## 5) Flash firmware to device

Auto-detect serial port:

```bash
pio run -e esp32dev -t upload
```

If auto-detect fails, pass the USB port explicitly:

```bash
pio run -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX
```

## 6) Upload web portal assets (required)

The web app (wizard, dashboard, health page) is served from LittleFS and must be uploaded:

```bash
pio run -e esp32dev -t uploadfs
```

If you changed any source assets in `firmware/data-src/`, regenerate matching gzip files in `firmware/data/` before `uploadfs`, for example:

```bash
gzip -9 -c data-src/common.js > data/common.js.gz
```

Then run `uploadfs` again.

## 7) Watch serial logs (recommended)

```bash
pio device monitor -b 115200
```

You should see Wi-Fi state logs, including AP mode and/or assigned STA IP.

## 8) Open the app in your browser

### AP setup mode (first boot or no credentials)

1. Connect your laptop/phone to Wi-Fi SSID `FlightWall-Setup`.
2. Open:
   - `http://192.168.4.1/`
3. Complete the setup wizard.
4. Device reboots and attempts STA mode.

### STA mode (connected to your network)

After successful network join, open one of:

- `http://flightwall.local/` (mDNS)
- `http://<device-ip>/` (IP from serial logs/router)

Expected pages:

- `/` -> setup wizard in AP mode, dashboard in STA mode
- `/health.html` -> system health page

## 9) Quick validation after flashing

From repo root:

```bash
python3 tests/smoke/test_web_portal_smoke.py --base-url http://flightwall.local
```

If mDNS does not resolve, use the device IP:

```bash
python3 tests/smoke/test_web_portal_smoke.py --base-url http://192.168.x.x
```

## 10) Typical reflash workflow during development

From `firmware/`:

```bash
pio run -e esp32dev
pio run -e esp32dev -t upload
pio run -e esp32dev -t uploadfs
```

Then reload the browser page.

## Troubleshooting

- `pio: command not found`
  - Use `~/.platformio/penv/bin/pio` or restart shell after install.
- Upload fails / no serial port
  - Check USB cable is data-capable and select explicit `--upload-port`.
- Browser cannot reach `flightwall.local`
  - Use the direct device IP from serial logs.
- Root page is blank or 404
  - Re-run `pio run -e esp32dev -t uploadfs` (LittleFS assets likely missing).
