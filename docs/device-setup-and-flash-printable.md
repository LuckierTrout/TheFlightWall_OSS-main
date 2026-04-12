# FlightWall Device Setup and Flash (Printable)

Use this as a bench worksheet. Check boxes as you go and record values in blanks.

---

## Session Info

- Date: ____________________
- Tester: __________________
- Device / Board: __________________
- USB Port (if used): __________________
- Firmware version / commit: __________________

---

## 1) Prerequisites

- [ ] ESP32 connected by USB
- [ ] LED matrix connected to ESP32
- [ ] macOS terminal ready
- [ ] Repo available locally
- [ ] (Optional) OpenSky credentials
- [ ] (Optional) AeroAPI key

---

## 2) Install and Verify PlatformIO

From repo root terminal:

```bash
brew install platformio
pio --version
```

If `pio` is not found:

```bash
~/.platformio/penv/bin/pio --version
```

- [ ] PlatformIO installed
- [ ] `pio --version` works

---

## 3) Configure Firmware Values

Edit the following files before first flash:

- [ ] `firmware/config/WiFiConfiguration.h`
  - [ ] `WIFI_SSID`
  - [ ] `WIFI_PASSWORD`
- [ ] `firmware/config/APIConfiguration.h`
  - [ ] `OPENSKY_CLIENT_ID`
  - [ ] `OPENSKY_CLIENT_SECRET`
  - [ ] `AEROAPI_KEY`
- [ ] `firmware/config/UserConfiguration.h`
  - [ ] `CENTER_LAT`
  - [ ] `CENTER_LON`
  - [ ] `RADIUS_KM`
- [ ] `firmware/config/HardwareConfiguration.h` (if needed)

Notes:

- Empty Wi-Fi credentials -> AP setup mode (`FlightWall-Setup`)
- Valid Wi-Fi credentials -> STA mode attempt on boot

---

## 4) Build Firmware

```bash
cd firmware
pio run -e esp32dev
```

- [ ] Build succeeded

---

## 5) Flash Firmware

Try auto port first:

```bash
pio run -e esp32dev -t upload
```

If needed, use explicit port:

```bash
pio run -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX
```

- [ ] Upload succeeded

---

## 6) Upload Web App Assets to LittleFS (Required)

```bash
pio run -e esp32dev -t uploadfs
```

If you edited `firmware/data-src/*`, regenerate matching gzip assets in `firmware/data/*` first, for example:

```bash
gzip -9 -c data-src/common.js > data/common.js.gz
```

Then rerun:

```bash
pio run -e esp32dev -t uploadfs
```

- [ ] LittleFS upload succeeded

---

## 7) Open Serial Monitor

```bash
pio device monitor -b 115200
```

Record key output:

- AP SSID shown: __________________
- Device IP shown: __________________
- mDNS status (`flightwall.local`): __________________

- [ ] Serial monitor confirms normal boot

---

## 8) Browser Access

### AP Setup Mode

1. Connect computer/phone to Wi-Fi: `FlightWall-Setup`
2. Open: `http://192.168.4.1/`
3. Complete setup wizard
4. Wait for reboot

- [ ] Wizard loaded
- [ ] Setup saved
- [ ] Reboot completed

### STA Mode

Open one:

- `http://flightwall.local/`
- `http://<device-ip>/`

Expected:

- `/` serves dashboard in STA mode
- `/health.html` serves health page

- [ ] Dashboard loaded
- [ ] Health page loaded

---

## 9) Smoke Test

From repo root:

```bash
python3 tests/smoke/test_web_portal_smoke.py --base-url http://flightwall.local
```

If mDNS fails, use device IP:

```bash
python3 tests/smoke/test_web_portal_smoke.py --base-url http://192.168.x.x
```

- [ ] Smoke test passed
- [ ] Any failures logged

Notes:
____________________________________________________________
____________________________________________________________
____________________________________________________________

---

## 10) Fast Reflash Loop

From `firmware/`:

```bash
pio run -e esp32dev
pio run -e esp32dev -t upload
pio run -e esp32dev -t uploadfs
```

- [ ] Reflash loop completed

---

## Troubleshooting Quick Hits

- `pio: command not found`
  - Use `~/.platformio/penv/bin/pio`
- Upload fails / no serial device
  - Check USB data cable and explicit `--upload-port`
- `flightwall.local` does not resolve
  - Use direct device IP
- Blank/404 web portal page
  - Run `pio run -e esp32dev -t uploadfs` again

