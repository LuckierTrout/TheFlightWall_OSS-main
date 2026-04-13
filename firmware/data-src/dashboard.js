/* FlightWall Dashboard — Display, Timing, Network/API, and Hardware settings */

/**
 * IANA-to-POSIX timezone mapping (Story fn-2.1).
 * Browser auto-detects IANA via Intl API; ESP32 needs POSIX string for configTzTime().
 * POSIX sign convention is inverted from UTC offset (west=positive, east=negative).
 */
var TZ_MAP = {
  // North America
  "America/New_York":       "EST5EDT,M3.2.0,M11.1.0",
  "America/Chicago":        "CST6CDT,M3.2.0,M11.1.0",
  "America/Denver":         "MST7MDT,M3.2.0,M11.1.0",
  "America/Los_Angeles":    "PST8PDT,M3.2.0,M11.1.0",
  "America/Phoenix":        "MST7",
  "America/Anchorage":      "AKST9AKDT,M3.2.0,M11.1.0",
  "Pacific/Honolulu":       "HST10",
  "America/Toronto":        "EST5EDT,M3.2.0,M11.1.0",
  "America/Vancouver":      "PST8PDT,M3.2.0,M11.1.0",
  "America/Edmonton":       "MST7MDT,M3.2.0,M11.1.0",
  "America/Winnipeg":       "CST6CDT,M3.2.0,M11.1.0",
  "America/Halifax":        "AST4ADT,M3.2.0,M11.1.0",
  "America/St_Johns":       "NST3:30NDT,M3.2.0,M11.1.0",
  "America/Mexico_City":    "CST6",
  "America/Tijuana":        "PST8PDT,M3.2.0,M11.1.0",
  // Europe
  "Europe/London":          "GMT0BST,M3.5.0/1,M10.5.0",
  "Europe/Paris":           "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Berlin":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Madrid":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Rome":            "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Amsterdam":       "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Brussels":        "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Zurich":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Vienna":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Warsaw":          "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Stockholm":       "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Oslo":            "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Copenhagen":      "CET-1CEST,M3.5.0/2,M10.5.0/3",
  "Europe/Helsinki":        "EET-2EEST,M3.5.0/3,M10.5.0/4",
  "Europe/Athens":          "EET-2EEST,M3.5.0/3,M10.5.0/4",
  "Europe/Bucharest":       "EET-2EEST,M3.5.0/3,M10.5.0/4",
  "Europe/Istanbul":        "TRT-3",
  "Europe/Moscow":          "MSK-3",
  "Europe/Lisbon":          "WET0WEST,M3.5.0/1,M10.5.0",
  "Europe/Dublin":          "IST-1GMT0,M10.5.0,M3.5.0/1",
  // Asia
  "Asia/Tokyo":             "JST-9",
  "Asia/Shanghai":          "CST-8",
  "Asia/Hong_Kong":         "HKT-8",
  "Asia/Singapore":         "SGT-8",
  "Asia/Kolkata":           "IST-5:30",
  "Asia/Dubai":             "GST-4",
  "Asia/Seoul":             "KST-9",
  "Asia/Bangkok":           "ICT-7",
  "Asia/Jakarta":           "WIB-7",
  "Asia/Taipei":            "CST-8",
  "Asia/Karachi":           "PKT-5",
  "Asia/Dhaka":             "BST-6",
  "Asia/Riyadh":            "AST-3",
  "Asia/Tehran":            "IRST-3:30",
  // Oceania
  "Australia/Sydney":       "AEST-10AEDT,M10.1.0,M4.1.0/3",
  "Australia/Melbourne":    "AEST-10AEDT,M10.1.0,M4.1.0/3",
  "Australia/Perth":        "AWST-8",
  "Australia/Brisbane":     "AEST-10",
  "Australia/Adelaide":     "ACST-9:30ACDT,M10.1.0,M4.1.0/3",
  "Pacific/Auckland":       "NZST-12NZDT,M9.5.0,M4.1.0/3",
  "Pacific/Fiji":           "FJT-12",
  // South America
  "America/Sao_Paulo":      "BRT3",
  "America/Argentina/Buenos_Aires": "ART3",
  "America/Santiago":       "CLT4CLST,M9.1.6/24,M4.1.6/24",
  "America/Bogota":         "COT5",
  "America/Lima":           "PET5",
  "America/Caracas":        "VET4",
  // Africa / Middle East
  "Africa/Cairo":           "EET-2EEST,M4.5.5/0,M10.5.4/24",
  "Africa/Johannesburg":    "SAST-2",
  "Africa/Lagos":           "WAT-1",
  "Africa/Nairobi":         "EAT-3",
  "Africa/Casablanca":      "WET0WEST,M3.5.0/2,M10.5.0/3",
  "Asia/Jerusalem":         "IST-2IDT,M3.4.4/26,M10.5.0",
  // UTC
  "UTC":                    "UTC0",
  "Etc/UTC":                "UTC0",
  "Etc/GMT":                "GMT0"
};

/**
 * Detect browser timezone and return IANA + POSIX pair.
 * Falls back to UTC if browser timezone is not in TZ_MAP.
 * @returns {{ iana: string, posix: string }}
 */
function getTimezoneConfig() {
  var iana = "UTC";
  try {
    iana = Intl.DateTimeFormat().resolvedOptions().timeZone || "UTC";
  } catch (e) {
    // Intl API not available — fall back to UTC
  }
  var posix = TZ_MAP[iana] || "UTC0";
  return { iana: iana, posix: posix };
}

/**
 * Shared zone calculation algorithm (must match C++ LayoutEngine::compute exactly).
 * @param {number} tilesX - Number of horizontal tiles
 * @param {number} tilesY - Number of vertical tiles
 * @param {number} tilePixels - Pixels per tile edge
 * @returns {object} { matrixWidth, matrixHeight, mode, logoZone, flightZone, telemetryZone, valid }
 */
function computeLayout(tilesX, tilesY, tilePixels, logoWidthPct, flightHeightPct, layoutMode) {
  if (!tilesX || !tilesY || !tilePixels) {
    return { matrixWidth: 0, matrixHeight: 0, mode: 'compact',
      logoZone: {x:0,y:0,w:0,h:0}, flightZone: {x:0,y:0,w:0,h:0},
      telemetryZone: {x:0,y:0,w:0,h:0}, valid: false };
  }

  var mw = tilesX * tilePixels;
  var mh = tilesY * tilePixels;

  if (mw < mh) {
    return { matrixWidth: mw, matrixHeight: mh, mode: 'compact',
      logoZone: {x:0,y:0,w:0,h:0}, flightZone: {x:0,y:0,w:0,h:0},
      telemetryZone: {x:0,y:0,w:0,h:0}, valid: false };
  }

  var mode;
  if (mh < 32) { mode = 'compact'; }
  else if (mh < 48) { mode = 'full'; }
  else { mode = 'expanded'; }

  var logoW = mh; // default: square logo
  if (logoWidthPct > 0 && logoWidthPct <= 99) {
    logoW = Math.round(mw * logoWidthPct / 100);
    logoW = Math.max(1, Math.min(mw - 1, logoW));
  }

  var splitY = Math.floor(mh / 2); // default: 50/50
  if (flightHeightPct > 0 && flightHeightPct <= 99) {
    splitY = Math.round(mh * flightHeightPct / 100);
    splitY = Math.max(1, Math.min(mh - 1, splitY));
  }

  var logoZone, flightZone, telemetryZone;
  if (layoutMode === 1) {
    // Full-width bottom: logo top-left, flight top-right, telemetry spans full width
    logoZone      = { x: 0,     y: 0,      w: logoW,      h: splitY };
    flightZone    = { x: logoW, y: 0,      w: mw - logoW, h: splitY };
    telemetryZone = { x: 0,     y: splitY, w: mw,         h: mh - splitY };
  } else {
    // Classic: logo full-height left, flight/telemetry stacked right
    logoZone      = { x: 0,     y: 0,      w: logoW,      h: mh };
    flightZone    = { x: logoW, y: 0,      w: mw - logoW, h: splitY };
    telemetryZone = { x: logoW, y: splitY, w: mw - logoW, h: mh - splitY };
  }

  return {
    matrixWidth: mw,
    matrixHeight: mh,
    mode: mode,
    logoZone: logoZone,
    flightZone: flightZone,
    telemetryZone: telemetryZone,
    valid: true
  };
}

(function() {
  'use strict';

  // --- Display card DOM ---
  var brightness = document.getElementById('brightness');
  var brightnessVal = document.getElementById('brightness-val');
  var colorR = document.getElementById('color-r');
  var colorG = document.getElementById('color-g');
  var colorB = document.getElementById('color-b');
  var colorPreview = document.getElementById('color-preview');
  var deviceIp = document.getElementById('device-ip');

  // --- Timing card DOM ---
  var fetchInterval = document.getElementById('fetch-interval');
  var fetchLabel = document.getElementById('fetch-label');
  var fetchEstimate = document.getElementById('fetch-estimate');
  var displayCycle = document.getElementById('display-cycle');
  var cycleLabel = document.getElementById('cycle-label');

  // --- Network & API card DOM ---
  var dWifiSsid = document.getElementById('d-wifi-ssid');
  var dWifiPass = document.getElementById('d-wifi-pass');
  var dOsClientId = document.getElementById('d-os-client-id');
  var dOsClientSec = document.getElementById('d-os-client-sec');
  var dAeroKey = document.getElementById('d-aeroapi-key');
  var dBtnScan = document.getElementById('d-btn-scan');
  var dScanArea = document.getElementById('d-scan-area');
  var dScanResults = document.getElementById('d-scan-results');

  // --- Hardware card DOM ---
  var dTilesX = document.getElementById('d-tiles-x');
  var dTilesY = document.getElementById('d-tiles-y');
  var dTilePixels = document.getElementById('d-tile-pixels');
  var dDisplayPin = document.getElementById('d-display-pin');
  var dOriginCorner = document.getElementById('d-origin-corner');
  var dScanDir = document.getElementById('d-scan-dir');
  var dZigzag = document.getElementById('d-zigzag');
  var dResText = document.getElementById('d-resolution-text');

  // --- Unified apply bar ---
  var applyBar = document.getElementById('apply-bar');
  var btnApplyAll = document.getElementById('btn-apply-all');
  var dirtySections = { display: false, timing: false, network: false, hardware: false };

  function markSectionDirty(section) {
    dirtySections[section] = true;
    applyBar.style.display = '';
  }

  function clearDirtyState() {
    dirtySections.display = false;
    dirtySections.timing = false;
    dirtySections.network = false;
    dirtySections.hardware = false;
    applyBar.style.display = 'none';
  }

  function collectPayload() {
    var payload = {};
    if (dirtySections.display) {
      payload.brightness = parseInt(brightness.value, 10);
      payload.text_color_r = clamp(parseInt(colorR.value, 10) || 0);
      payload.text_color_g = clamp(parseInt(colorG.value, 10) || 0);
      payload.text_color_b = clamp(parseInt(colorB.value, 10) || 0);
    }
    if (dirtySections.timing) {
      payload.fetch_interval = parseInt(fetchInterval.value, 10);
      payload.display_cycle = parseInt(displayCycle.value, 10);
    }
    if (dirtySections.network) {
      payload.wifi_ssid = dWifiSsid.value;
      payload.wifi_password = dWifiPass.value;
      payload.os_client_id = dOsClientId.value.trim();
      payload.os_client_sec = dOsClientSec.value.trim();
      payload.aeroapi_key = dAeroKey.value.trim();
    }
    if (dirtySections.hardware) {
      var tilesX = parseUint8Field(dTilesX, 'Tiles X', false);
      if (tilesX === null) return null;
      var tilesY = parseUint8Field(dTilesY, 'Tiles Y', false);
      if (tilesY === null) return null;
      var tilePixels = parseUint8Field(dTilePixels, 'Pixels per tile', false);
      if (tilePixels === null) return null;
      var dp = parseUint8Field(dDisplayPin, 'Display data pin', true);
      if (dp === null || VALID_PINS.indexOf(dp) === -1) {
        FW.showToast('Invalid GPIO pin. Supported: ' + VALID_PINS.join(', '), 'error');
        return null;
      }
      var originCorner = parseUint8Field(dOriginCorner, 'Origin corner', true);
      if (originCorner === null) return null;
      var scanDir = parseUint8Field(dScanDir, 'Scan direction', true);
      if (scanDir === null) return null;
      var zigzag = parseUint8Field(dZigzag, 'Zigzag', true);
      if (zigzag === null) return null;
      payload.tiles_x = tilesX;
      payload.tiles_y = tilesY;
      payload.tile_pixels = tilePixels;
      payload.display_pin = dp;
      payload.origin_corner = originCorner;
      payload.scan_dir = scanDir;
      payload.zigzag = zigzag;
      payload.zone_logo_pct = customLogoPct;
      payload.zone_split_pct = customSplitPct;
      payload.zone_layout = zoneLayout;
    }
    return payload;
  }

  var VALID_PINS = [0,2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33];

  var debounceTimer = null;
  var DEBOUNCE_MS = 400;
  var SECONDS_PER_MONTH = 2592000;
  var OPENSKY_WARN_THRESHOLD = 4000;
  var previewLastLayout = null;
  var hardwareInputDirty = false;
  var suppressHardwareInputHandler = false;
  var customLogoPct = 0;   // 0 = auto
  var customSplitPct = 0;  // 0 = auto
  var zoneLayout = 0;      // 0 = classic, 1 = full-width bottom
  var dZoneLayout = document.getElementById('d-zone-layout');

  deviceIp.textContent = window.location.hostname || '';

  // --- Load current settings ---
  function loadSettings() {
    return FW.get('/api/settings').then(function(res) {
      if (!res.body.ok || !res.body.data) {
        FW.showToast('Failed to load settings', 'error');
        return;
      }
      var d = res.body.data;

      // Display
      if (d.brightness !== undefined) {
        brightness.value = d.brightness;
        brightnessVal.textContent = d.brightness;
      }
      if (d.text_color_r !== undefined) colorR.value = d.text_color_r;
      if (d.text_color_g !== undefined) colorG.value = d.text_color_g;
      if (d.text_color_b !== undefined) colorB.value = d.text_color_b;
      updatePreview();

      // Timing
      if (d.fetch_interval !== undefined) {
        fetchInterval.value = d.fetch_interval;
        fetchLabel.textContent = formatInterval(d.fetch_interval);
        updateFetchEstimate(d.fetch_interval);
      }
      if (d.display_cycle !== undefined) {
        displayCycle.value = d.display_cycle;
        updateCycleLabel(d.display_cycle);
      }

      // Network & API
      if (d.wifi_ssid !== undefined) dWifiSsid.value = d.wifi_ssid;
      if (d.wifi_password !== undefined) dWifiPass.value = d.wifi_password;
      if (d.os_client_id !== undefined) dOsClientId.value = d.os_client_id;
      if (d.os_client_sec !== undefined) dOsClientSec.value = d.os_client_sec;
      if (d.aeroapi_key !== undefined) dAeroKey.value = d.aeroapi_key;

      // Location
      var loadedLocation = normalizeLocationValues({
        center_lat: d.center_lat,
        center_lon: d.center_lon,
        radius_km: d.radius_km
      });
      if (loadedLocation) {
        writeLocationFields(loadedLocation);
        rememberValidLocation(loadedLocation);
        if (mapInstance) {
          updateMapFromValues(loadedLocation, { fit: false, pan: false });
        } else if (leafletLoaded && isLocationCardOpen() && !mapFailureHandled) {
          maybeInitLocationMap();
        }
      }

      // Hardware
      if (d.tiles_x !== undefined) dTilesX.value = d.tiles_x;
      if (d.tiles_y !== undefined) dTilesY.value = d.tiles_y;
      if (d.tile_pixels !== undefined) dTilePixels.value = d.tile_pixels;
      if (d.display_pin !== undefined) dDisplayPin.value = d.display_pin;
      if (d.origin_corner !== undefined) dOriginCorner.value = d.origin_corner;
      if (d.scan_dir !== undefined) dScanDir.value = d.scan_dir;
      if (d.zigzag !== undefined) dZigzag.value = d.zigzag;
      if (d.zone_logo_pct !== undefined) customLogoPct = d.zone_logo_pct;
      if (d.zone_split_pct !== undefined) customSplitPct = d.zone_split_pct;
      if (d.zone_layout !== undefined) {
        zoneLayout = d.zone_layout;
        if (dZoneLayout) dZoneLayout.value = zoneLayout;
      }
      updateHwResolution();

      // Sync calibration selectors from loaded hardware values
      syncCalibrationFromSettings();

      // Show scan button now that we're in STA mode
      dScanArea.style.display = '';
    }).catch(function() {
      FW.showToast('Cannot reach device', 'error');
    });
  }

  // --- Apply settings (hot-reload, no reboot) ---
  function applySettings(payload) {
    FW.post('/api/settings', payload).then(function(res) {
      if (res.body.ok) {
        FW.showToast('Applied', 'success');
      } else {
        FW.showToast(res.body.error || 'Save failed', 'error');
      }
    }).catch(function() {
      FW.showToast('Network error', 'error');
    });
  }

  // --- Apply settings with reboot awareness ---
  function applyWithReboot(payload, btn, originalText) {
    btn.disabled = true;
    btn.textContent = 'Applying...';
    var rebootRequested = false;

    FW.post('/api/settings', payload).then(function(res) {
      if (!res.body.ok) {
        throw new Error(res.body.error || 'Save failed');
      }
      if (res.body.reboot_required) {
        FW.showToast('Rebooting to apply changes...', 'warning');
        rebootRequested = true;
        return FW.post('/api/reboot', {});
      }
      FW.showToast('Applied', 'success');
      btn.disabled = false;
      btn.textContent = originalText;
      return null;
    }).then(function(res) {
      if (!rebootRequested) return;
      if (!res || !res.body || !res.body.ok) {
        throw new Error((res && res.body && res.body.error) || 'Reboot failed');
      }
      btn.textContent = 'Rebooting...';
    }).catch(function(err) {
      if (rebootRequested && err && err.name === 'TypeError') {
        // Connection loss after reboot request is expected
        btn.textContent = 'Rebooting...';
        return;
      }
      FW.showToast(err && err.message ? err.message : 'Network error', 'error');
      btn.disabled = false;
      btn.textContent = originalText;
    });
  }

  function debouncedApply(payload) {
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(function() {
      applySettings(payload);
    }, DEBOUNCE_MS);
  }

  // --- Color preview ---
  function updatePreview() {
    var r = clamp(parseInt(colorR.value, 10) || 0);
    var g = clamp(parseInt(colorG.value, 10) || 0);
    var b = clamp(parseInt(colorB.value, 10) || 0);
    colorPreview.style.background = 'rgb(' + r + ',' + g + ',' + b + ')';
  }

  function clamp(v) {
    return Math.max(0, Math.min(255, v));
  }

  function parseStrictInteger(raw) {
    var text = String(raw === undefined || raw === null ? '' : raw).trim();
    if (!/^\d+$/.test(text)) return null;
    return parseInt(text, 10);
  }

  function parseUint8Field(el, label, allowZero) {
    var value = parseStrictInteger(el.value);
    var min = allowZero ? 0 : 1;
    if (value === null || value < min || value > 255) {
      FW.showToast(label + ' must be a whole number from ' + min + ' to 255', 'error');
      return null;
    }
    return value;
  }

  // --- Brightness slider ---
  brightness.addEventListener('input', function() {
    brightnessVal.textContent = brightness.value;
  });
  brightness.addEventListener('change', function() {
    markSectionDirty('display');
  });

  // --- RGB inputs ---
  function onColorChange() {
    var r = clamp(parseInt(colorR.value, 10) || 0);
    var g = clamp(parseInt(colorG.value, 10) || 0);
    var b = clamp(parseInt(colorB.value, 10) || 0);
    colorR.value = r;
    colorG.value = g;
    colorB.value = b;
    updatePreview();
    markSectionDirty('display');
  }

  colorR.addEventListener('change', onColorChange);
  colorG.addEventListener('change', onColorChange);
  colorB.addEventListener('change', onColorChange);
  colorR.addEventListener('input', updatePreview);
  colorG.addEventListener('input', updatePreview);
  colorB.addEventListener('input', updatePreview);

  // --- Timing: fetch interval ---
  function formatInterval(seconds) {
    var s = parseInt(seconds, 10);
    var min = Math.floor(s / 60);
    var rem = s % 60;
    if (min === 0) return s + ' s';
    if (rem === 0) return min + ' min';
    return min + ' min ' + rem + ' s';
  }

  function updateFetchEstimate(seconds) {
    var s = parseInt(seconds, 10);
    if (s <= 0) s = 1;
    var n = Math.round(SECONDS_PER_MONTH / s);
    fetchEstimate.textContent = '~' + n.toLocaleString() + ' calls/month';
    if (n > OPENSKY_WARN_THRESHOLD) {
      fetchEstimate.classList.add('estimate-warning');
    } else {
      fetchEstimate.classList.remove('estimate-warning');
    }
  }

  fetchInterval.addEventListener('input', function() {
    fetchLabel.textContent = formatInterval(fetchInterval.value);
    updateFetchEstimate(fetchInterval.value);
  });
  fetchInterval.addEventListener('change', function() {
    markSectionDirty('timing');
  });

  // --- Timing: display cycle ---
  function updateCycleLabel(seconds) {
    cycleLabel.textContent = parseInt(seconds, 10) + ' s';
  }

  displayCycle.addEventListener('input', function() {
    updateCycleLabel(displayCycle.value);
  });
  displayCycle.addEventListener('change', function() {
    markSectionDirty('timing');
  });

  // --- Network & API: mark dirty on change ---
  function onNetworkInput() { markSectionDirty('network'); }
  dWifiSsid.addEventListener('input', onNetworkInput);
  dWifiPass.addEventListener('input', onNetworkInput);
  dOsClientId.addEventListener('input', onNetworkInput);
  dOsClientSec.addEventListener('input', onNetworkInput);
  dAeroKey.addEventListener('input', onNetworkInput);

  // --- Hardware: Resolution text ---
  function updateHwResolution() {
    var dims = parseHardwareDimensionsFromInputs();
    setResolutionText(dims);
  }

  function setResolutionText(dims) {
    if (!dims || dims.matrixWidth <= 0 || dims.matrixHeight <= 0) {
      dResText.textContent = '';
      return;
    }
    dResText.textContent = 'Display: ' + dims.matrixWidth + ' x ' + dims.matrixHeight + ' pixels';
  }

  function parseHardwareDimensionsFromInputs() {
    var tx = parseStrictInteger(dTilesX.value);
    var ty = parseStrictInteger(dTilesY.value);
    var tp = parseStrictInteger(dTilePixels.value);
    if (tx === null || ty === null || tp === null) return null;
    if (tx < 1 || ty < 1 || tp < 1 || tx > 255 || ty > 255 || tp > 255) return null;
    return {
      tilesX: tx,
      tilesY: ty,
      tilePixels: tp,
      matrixWidth: tx * tp,
      matrixHeight: ty * tp
    };
  }

  function normalizeZone(zone) {
    if (!zone || typeof zone !== 'object') return null;
    var x = Number(zone.x);
    var y = Number(zone.y);
    var w = Number(zone.w);
    var h = Number(zone.h);
    if (!Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(w) || !Number.isFinite(h)) return null;
    return { x: x, y: y, w: w, h: h };
  }

  function normalizeLayoutFromApi(data) {
    if (!data || typeof data !== 'object' || !data.matrix || !data.hardware) return null;
    var mw = Number(data.matrix.width);
    var mh = Number(data.matrix.height);
    var tx = Number(data.hardware.tiles_x);
    var ty = Number(data.hardware.tiles_y);
    var tp = Number(data.hardware.tile_pixels);
    var logo = normalizeZone(data.logo_zone);
    var flight = normalizeZone(data.flight_zone);
    var telemetry = normalizeZone(data.telemetry_zone);
    if (!Number.isFinite(mw) || !Number.isFinite(mh)) return null;
    if (!Number.isFinite(tx) || !Number.isFinite(ty) || !Number.isFinite(tp)) return null;
    if (!logo || !flight || !telemetry) return null;
    return {
      matrixWidth: mw,
      matrixHeight: mh,
      mode: String(data.mode || 'compact'),
      logoZone: logo,
      flightZone: flight,
      telemetryZone: telemetry,
      valid: mw > 0 && mh > 0 && mw >= mh,
      hardware: { tilesX: tx, tilesY: ty, tilePixels: tp }
    };
  }

  // --- Canvas layout preview ---
  var layoutCanvas = document.getElementById('layout-preview');
  var previewContainer = document.getElementById('preview-container');

  var ZONE_COLORS = {
    logo: '#58a6ff',
    flight: '#3fb950',
    telemetry: '#d29922'
  };

  function renderLayoutCanvas(layout) {
    if (!layoutCanvas || !layoutCanvas.getContext || !previewContainer) return;
    var ctx = layoutCanvas.getContext('2d');

    if (!layout || !layout.valid) {
      layoutCanvas.width = 0;
      layoutCanvas.height = 0;
      previewContainer.style.display = 'none';
      previewLastLayout = null;
      return;
    }

    previewLastLayout = layout;
    previewContainer.style.display = '';

    // Scale canvas to fit container width while preserving aspect ratio
    var containerWidth = previewContainer.clientWidth || 300;
    var aspect = layout.matrixWidth / layout.matrixHeight;
    var drawWidth = Math.min(containerWidth, 480);
    var drawHeight = Math.round(drawWidth / aspect);

    layoutCanvas.width = drawWidth;
    layoutCanvas.height = drawHeight;

    var sx = drawWidth / layout.matrixWidth;
    var sy = drawHeight / layout.matrixHeight;

    // Background (matrix bounds)
    ctx.fillStyle = '#0d1117';
    ctx.fillRect(0, 0, drawWidth, drawHeight);

    // Draw tile grid when hardware dimensions are known.
    if (layout.hardware && layout.hardware.tilePixels > 0) {
      var tileStepX = layout.hardware.tilePixels * sx;
      var tileStepY = layout.hardware.tilePixels * sy;
      if (tileStepX >= 2 && tileStepY >= 2) {
        ctx.strokeStyle = '#21262d';
        ctx.lineWidth = 1;
        for (var gx = tileStepX; gx < drawWidth; gx += tileStepX) {
          var x = Math.round(gx) + 0.5;
          ctx.beginPath();
          ctx.moveTo(x, 0);
          ctx.lineTo(x, drawHeight);
          ctx.stroke();
        }
        for (var gy = tileStepY; gy < drawHeight; gy += tileStepY) {
          var y = Math.round(gy) + 0.5;
          ctx.beginPath();
          ctx.moveTo(0, y);
          ctx.lineTo(drawWidth, y);
          ctx.stroke();
        }
      }
    }

    // Draw zones
    var zones = [
      { zone: layout.logoZone, color: ZONE_COLORS.logo, label: 'Logo' },
      { zone: layout.flightZone, color: ZONE_COLORS.flight, label: 'Flight' },
      { zone: layout.telemetryZone, color: ZONE_COLORS.telemetry, label: 'Telemetry' }
    ];

    // Helper: truncate text to fit pixel columns (mirrors firmware truncateToColumns)
    function truncPreview(text, maxCols) {
      if (text.length <= maxCols) return text;
      if (maxCols <= 3) return text.substring(0, maxCols);
      return text.substring(0, maxCols - 3) + '...';
    }

    zones.forEach(function(z) {
      if (!z.zone || z.zone.w <= 0 || z.zone.h <= 0) return;
      var rx = Math.round(z.zone.x * sx);
      var ry = Math.round(z.zone.y * sy);
      var rw = Math.round(z.zone.w * sx);
      var rh = Math.round(z.zone.h * sy);
      if (rw <= 0 || rh <= 0) return;

      ctx.fillStyle = z.color;
      ctx.globalAlpha = 0.3;
      ctx.fillRect(rx, ry, rw, rh);
      ctx.globalAlpha = 1.0;
      ctx.strokeStyle = z.color;
      ctx.lineWidth = 2;
      if (rw > 2 && rh > 2) {
        ctx.strokeRect(rx + 1, ry + 1, rw - 2, rh - 2);
      }

      // Firmware uses 6x8px characters. Compute how many lines/cols fit.
      var charW = 6, charH = 8;
      var maxCols = Math.floor(z.zone.w / charW);
      var linesAvail = Math.floor(z.zone.h / charH);
      var fontSize = Math.max(7, Math.min(13, Math.round(charH * sy)));
      var lineH = fontSize * 1.15;

      if (maxCols <= 0 || linesAvail <= 0 || rw < 20 || rh < 10) {
        // Too small for text — just show zone color label
        if (rw > 30 && rh > 12) {
          ctx.fillStyle = z.color;
          ctx.font = '9px sans-serif';
          ctx.textAlign = 'center';
          ctx.textBaseline = 'middle';
          ctx.fillText(z.label, rx + rw / 2, ry + rh / 2);
        }
        return;
      }

      ctx.save();
      ctx.beginPath();
      ctx.rect(rx, ry, rw, rh);
      ctx.clip();
      ctx.font = fontSize + 'px monospace';
      ctx.textAlign = 'left';
      ctx.textBaseline = 'top';
      ctx.fillStyle = '#e6edf3';

      var lines = [];

      if (z.label === 'Logo') {
        // Logo zone: show icon placeholder
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillStyle = z.color;
        ctx.font = Math.min(fontSize + 2, Math.floor(rh * 0.35)) + 'px sans-serif';
        ctx.fillText('Logo', rx + rw / 2, ry + rh / 2);
        ctx.restore();
        return;
      }

      if (z.label === 'Flight') {
        // Mirror firmware renderFlightZone logic with sample data
        var airline = 'United 1234';
        var route = 'SFO>LAX';
        var aircraft = 'B737';

        if (linesAvail === 1) {
          lines.push(truncPreview(route, maxCols));
        } else if (linesAvail === 2) {
          lines.push(truncPreview(airline, maxCols));
          var detail = route + ' ' + aircraft;
          lines.push(truncPreview(detail, maxCols));
        } else {
          lines.push(truncPreview(airline, maxCols));
          lines.push(truncPreview(route, maxCols));
          lines.push(truncPreview(aircraft, maxCols));
        }
      }

      if (z.label === 'Telemetry') {
        // Mirror firmware renderTelemetryZone logic with sample data
        if (linesAvail >= 2) {
          lines.push(truncPreview('28.3kft 450mph', maxCols));
          lines.push(truncPreview('045d -12fps', maxCols));
        } else {
          lines.push(truncPreview('A28k S450 T045 V-12', maxCols));
        }
      }

      // Draw lines vertically centered in zone (mirrors firmware centering)
      var totalH = lines.length * lineH;
      var startY = ry + (rh - totalH) / 2;
      var padX = Math.round(2 * sx);
      for (var i = 0; i < lines.length; i++) {
        ctx.fillText(lines[i], rx + padX, startY + i * lineH);
      }

      ctx.restore();
    });

    // Matrix outline
    ctx.strokeStyle = '#30363d';
    ctx.lineWidth = 1;
    ctx.strokeRect(0, 0, drawWidth, drawHeight);

    // Draw zone divider drag handles
    var logoX = Math.round(layout.logoZone.w * sx);
    var splitYPos = Math.round(layout.flightZone.h * sy);

    ctx.save();
    ctx.setLineDash([4, 4]);
    ctx.strokeStyle = '#fff';
    ctx.globalAlpha = 0.4;
    ctx.lineWidth = 2;

    // Logo divider (vertical) — in mode 1, only extends to splitY
    var logoVEnd = (layout.zoneLayout === 1) ? splitYPos : drawHeight;
    ctx.beginPath();
    ctx.moveTo(logoX, 0);
    ctx.lineTo(logoX, logoVEnd);
    ctx.stroke();

    // Split divider (horizontal) — in mode 1, spans full width
    var splitXStart = (layout.zoneLayout === 1) ? 0 : logoX;
    ctx.beginPath();
    ctx.moveTo(splitXStart, splitYPos);
    ctx.lineTo(drawWidth, splitYPos);
    ctx.stroke();

    ctx.setLineDash([]);
    ctx.globalAlpha = 0.7;

    // Grab handles
    var ghw = 4, ghh = 16;
    ctx.fillStyle = '#fff';
    ctx.fillRect(logoX - ghw / 2, logoVEnd / 2 - ghh / 2, ghw, ghh);
    var infoMidX = (layout.zoneLayout === 1) ? drawWidth / 2 : logoX + (drawWidth - logoX) / 2;
    ctx.fillRect(infoMidX - ghh / 2, splitYPos - ghw / 2, ghh, ghw);

    ctx.restore();
  }

  function updatePreviewFromInputs() {
    var dims = parseHardwareDimensionsFromInputs();
    var layout;
    if (!dims) {
      layout = { valid: false };
    } else {
      layout = computeLayout(dims.tilesX, dims.tilesY, dims.tilePixels, customLogoPct, customSplitPct, zoneLayout);
      layout.zoneLayout = zoneLayout;
      layout.hardware = {
        tilesX: dims.tilesX,
        tilesY: dims.tilesY,
        tilePixels: dims.tilePixels
      };
    }
    renderLayoutCanvas(layout);
    renderWiringCanvas();
  }

  function onHardwareInput() {
    if (!suppressHardwareInputHandler) {
      hardwareInputDirty = true;
      markSectionDirty('hardware');
    }
    updateHwResolution();
    updatePreviewFromInputs();
  }

  dTilesX.addEventListener('input', onHardwareInput);
  dTilesY.addEventListener('input', onHardwareInput);
  dTilePixels.addEventListener('input', onHardwareInput);
  window.addEventListener('resize', function() {
    if (previewLastLayout) {
      renderLayoutCanvas(previewLastLayout);
    }
    renderWiringCanvas();
  });

  // --- Zone drag interaction on layout preview canvas ---
  var zoneDragTarget = null; // 'logo' or 'split'

  function canvasPos(e) {
    var rect = layoutCanvas.getBoundingClientRect();
    var cx = e.touches ? e.touches[0].clientX : e.clientX;
    var cy = e.touches ? e.touches[0].clientY : e.clientY;
    return { x: cx - rect.left, y: cy - rect.top };
  }

  function hitTestDivider(pos) {
    if (!previewLastLayout || !previewLastLayout.valid) return null;
    var layout = previewLastLayout;
    var sx = layoutCanvas.width / layout.matrixWidth;
    var sy = layoutCanvas.height / layout.matrixHeight;
    var threshold = 10;

    var logoX = layout.logoZone.w * sx;
    var logoVEnd = (layout.zoneLayout === 1) ? (layout.flightZone.h * sy) : (layoutCanvas.height);
    if (Math.abs(pos.x - logoX) < threshold && pos.y <= logoVEnd + threshold) return 'logo';

    var splitY = layout.flightZone.h * sy;
    var splitXStart = (layout.zoneLayout === 1) ? 0 : logoX;
    if (pos.x >= splitXStart - threshold && Math.abs(pos.y - splitY) < threshold) return 'split';

    return null;
  }

  function onZoneDragStart(e) {
    var pos = canvasPos(e);
    var target = hitTestDivider(pos);
    if (!target) return;
    e.preventDefault();
    zoneDragTarget = target;
  }

  function onZoneDragMove(e) {
    if (!zoneDragTarget) {
      // Update cursor on hover
      if (layoutCanvas && previewLastLayout && previewLastLayout.valid) {
        var hoverPos = e.touches ? null : canvasPos(e);
        if (hoverPos) {
          var hover = hitTestDivider(hoverPos);
          layoutCanvas.style.cursor = hover ? (hover === 'logo' ? 'col-resize' : 'row-resize') : '';
        }
      }
      return;
    }
    e.preventDefault();

    var pos = canvasPos(e);
    var layout = previewLastLayout;
    if (!layout || !layout.valid) return;
    var sx = layoutCanvas.width / layout.matrixWidth;
    var sy = layoutCanvas.height / layout.matrixHeight;

    if (zoneDragTarget === 'logo') {
      var newLogoW = Math.round(pos.x / sx);
      newLogoW = Math.max(Math.round(layout.matrixWidth * 0.05), Math.min(Math.round(layout.matrixWidth * 0.95), newLogoW));
      customLogoPct = Math.round(newLogoW / layout.matrixWidth * 100);
    } else {
      var newSplitY = Math.round(pos.y / sy);
      newSplitY = Math.max(Math.round(layout.matrixHeight * 0.1), Math.min(Math.round(layout.matrixHeight * 0.9), newSplitY));
      customSplitPct = Math.round(newSplitY / layout.matrixHeight * 100);
    }

    updatePreviewFromInputs();
  }

  function onZoneDragEnd() {
    if (zoneDragTarget) {
      zoneDragTarget = null;
      markSectionDirty('hardware');
    }
  }

  layoutCanvas.addEventListener('mousedown', onZoneDragStart);
  layoutCanvas.addEventListener('touchstart', onZoneDragStart, { passive: false });
  document.addEventListener('mousemove', onZoneDragMove);
  document.addEventListener('touchmove', onZoneDragMove, { passive: false });
  document.addEventListener('mouseup', onZoneDragEnd);
  document.addEventListener('touchend', onZoneDragEnd);

  // --- Hardware: mark dirty on change ---
  function onHardwareDirty() { markSectionDirty('hardware'); }
  dDisplayPin.addEventListener('input', onHardwareDirty);
  dOriginCorner.addEventListener('input', onHardwareDirty);
  dScanDir.addEventListener('input', onHardwareDirty);
  dZigzag.addEventListener('input', onHardwareDirty);

  // --- Layout mode selector ---
  if (dZoneLayout) {
    dZoneLayout.addEventListener('change', function() {
      zoneLayout = parseInt(dZoneLayout.value, 10) || 0;
      markSectionDirty('hardware');
      updatePreviewFromInputs();
    });
  }

  // --- WiFi Scan (Task 3 — optional SSID picker) ---
  var scanTimer = null;
  var scanStartTime = 0;
  var SCAN_TIMEOUT_MS = 5000;
  var SCAN_POLL_MS = 800;

  function escHtml(s) {
    var el = document.createElement('span');
    el.textContent = s;
    return el.innerHTML;
  }

  function rssiLabel(rssi) {
    if (rssi >= -50) return 'Excellent';
    if (rssi >= -60) return 'Good';
    if (rssi >= -70) return 'Fair';
    return 'Weak';
  }

  function startWifiScan() {
    clearTimeout(scanTimer);
    dBtnScan.disabled = true;
    dBtnScan.textContent = 'Scanning...';
    dScanResults.style.display = 'none';
    dScanResults.innerHTML = '';
    scanStartTime = Date.now();
    pollWifiScan();
  }

  function pollWifiScan() {
    FW.get('/api/wifi/scan').then(function(res) {
      var d = res.body;
      if (d.ok && !d.scanning && d.data && d.data.length > 0) {
        showWifiResults(d.data);
        return;
      }
      if (d.ok && !d.scanning) {
        finishScan('No networks found');
        return;
      }
      if (Date.now() - scanStartTime >= SCAN_TIMEOUT_MS) {
        finishScan('Scan timed out');
        return;
      }
      scanTimer = setTimeout(pollWifiScan, SCAN_POLL_MS);
    }).catch(function() {
      if (Date.now() - scanStartTime >= SCAN_TIMEOUT_MS) {
        finishScan('Scan failed');
        return;
      }
      scanTimer = setTimeout(pollWifiScan, SCAN_POLL_MS);
    });
  }

  function showWifiResults(networks) {
    clearTimeout(scanTimer);
    dBtnScan.disabled = false;
    dBtnScan.textContent = 'Scan for networks';
    dScanResults.style.display = '';
    dScanResults.innerHTML = '';

    var seen = {};
    networks.forEach(function(n) {
      if (!n.ssid) return;
      if (!seen[n.ssid] || n.rssi > seen[n.ssid].rssi) {
        seen[n.ssid] = n;
      }
    });
    var unique = Object.keys(seen).map(function(k) { return seen[k]; });
    unique.sort(function(a, b) { return b.rssi - a.rssi; });

    unique.forEach(function(n) {
      var row = document.createElement('div');
      row.className = 'scan-row';
      row.innerHTML = '<span class="ssid">' + escHtml(n.ssid) + '</span><span class="rssi">' + rssiLabel(n.rssi) + '</span>';
      row.addEventListener('click', function() {
        dWifiSsid.value = n.ssid;
        dScanResults.style.display = 'none';
        markSectionDirty('network');
      });
      dScanResults.appendChild(row);
    });
  }

  function finishScan(msg) {
    clearTimeout(scanTimer);
    dBtnScan.disabled = false;
    dBtnScan.textContent = 'Scan for networks';
    if (msg) FW.showToast(msg, 'warning');
  }

  dBtnScan.addEventListener('click', startWifiScan);

  // --- System: Factory Reset ---
  var btnReset = document.getElementById('btn-reset');
  var btnResetConfirm = document.getElementById('btn-reset-confirm');
  var btnResetCancel = document.getElementById('btn-reset-cancel');
  var resetDefault = document.getElementById('reset-default');
  var resetConfirm = document.getElementById('reset-confirm');

  btnReset.addEventListener('click', function() {
    resetDefault.style.display = 'none';
    resetConfirm.style.display = '';
  });

  btnResetCancel.addEventListener('click', function() {
    resetConfirm.style.display = 'none';
    resetDefault.style.display = '';
  });

  btnResetConfirm.addEventListener('click', function() {
    btnResetConfirm.disabled = true;
    btnResetCancel.disabled = true;
    btnResetConfirm.textContent = 'Resetting...';

    FW.post('/api/reset', {}).then(function(res) {
      if (res.body.ok) {
        FW.showToast('Factory reset complete. Rebooting...', 'warning');
        btnResetConfirm.textContent = 'Rebooting...';
      } else {
        throw new Error(res.body.error || 'Reset failed');
      }
    }).catch(function(err) {
      if (err && err.name === 'TypeError') {
        // Connection loss after reset is expected
        FW.showToast('Device is restarting...', 'warning');
        btnResetConfirm.textContent = 'Rebooting...';
        return;
      }
      FW.showToast(err && err.message ? err.message : 'Reset failed', 'error');
      btnResetConfirm.disabled = false;
      btnResetCancel.disabled = false;
      btnResetConfirm.textContent = 'Confirm';
    });
  });

  // --- Location card ---
  var locationToggle = document.querySelector('.location-toggle');
  var locationBody = document.getElementById('location-body');
  var locationMap = document.getElementById('location-map');
  var mapLoading = document.getElementById('map-loading');
  var locationFallback = document.getElementById('location-fallback');
  var locationHelper = document.getElementById('location-helper');
  var dCenterLat = document.getElementById('d-center-lat');
  var dCenterLon = document.getElementById('d-center-lon');
  var dRadiusKm = document.getElementById('d-radius-km');

  var leafletLoaded = false;
  var leafletLoadAttempted = false;
  var mapInstance = null;
  var mapMarker = null;
  var mapCircle = null;
  var mapTileLayer = null;
  var mapRadiusHandle = null;
  var mapLoadTimeout = null;
  var mapFailureHandled = false;
  var mapTileLoadSucceeded = false;
  var mapTileErrorCount = 0;
  var lastValidLocation = null;
  var LOCATION_INITIAL_ZOOM = 10;
  var LOCATION_TILE_ERROR_THRESHOLD = 2;
  var locationDebounce = null;

  function isLocationCardOpen() {
    return locationBody.style.display !== 'none';
  }

  function showLocationLoading(message) {
    mapLoading.textContent = message || 'Loading map...';
    mapLoading.style.display = '';
  }

  function hideLocationLoading() {
    mapLoading.style.display = 'none';
  }

  function cloneLocationValues(values) {
    return {
      center_lat: values.center_lat,
      center_lon: values.center_lon,
      radius_km: values.radius_km
    };
  }

  function formatCoordinate(value) {
    return Number(value).toFixed(6);
  }

  function formatRadius(value) {
    return (Math.round(Number(value) * 100) / 100).toString();
  }

  function normalizeLocationValues(values) {
    if (!values) return null;
    var lat = Number(values.center_lat);
    var lon = Number(values.center_lon);
    var radius = Number(values.radius_km);
    if (!Number.isFinite(lat) || !Number.isFinite(lon) || !Number.isFinite(radius)) return null;
    return {
      center_lat: clampLat(lat),
      center_lon: clampLon(lon),
      radius_km: clampRadius(radius)
    };
  }

  function readLocationValuesFromFields() {
    return normalizeLocationValues({
      center_lat: dCenterLat.value,
      center_lon: dCenterLon.value,
      radius_km: dRadiusKm.value
    });
  }

  function writeLocationFields(values) {
    dCenterLat.value = formatCoordinate(values.center_lat);
    dCenterLon.value = formatCoordinate(values.center_lon);
    dRadiusKm.value = formatRadius(values.radius_km);
  }

  function rememberValidLocation(values) {
    if (!values) return;
    lastValidLocation = cloneLocationValues(values);
  }

  function getRadiusHandleLatLng(centerLatLng, radiusMeters) {
    var bounds = L.circle(centerLatLng, { radius: radiusMeters }).getBounds();
    return L.latLng(centerLatLng.lat, bounds.getEast());
  }

  function updateMapFromValues(values, options) {
    if (!mapInstance || !mapMarker || !mapCircle) return;
    options = options || {};

    var latlng = L.latLng(values.center_lat, values.center_lon);
    var radiusMeters = values.radius_km * 1000;

    mapMarker.setLatLng(latlng);
    mapCircle.setLatLng(latlng);
    mapCircle.setRadius(radiusMeters);

    if (mapRadiusHandle) {
      mapRadiusHandle.setLatLng(getRadiusHandleLatLng(latlng, radiusMeters));
    }

    if (options.fit) {
      try { mapInstance.fitBounds(mapCircle.getBounds().pad(0.1)); } catch (e) { /* ignore */ }
    } else if (options.pan) {
      mapInstance.panTo(latlng);
    }
  }

  function restoreLastValidLocation() {
    if (!lastValidLocation) return false;
    writeLocationFields(lastValidLocation);
    updateMapFromValues(lastValidLocation, { fit: false, pan: false });
    return true;
  }

  function syncLocationFieldsFromMap(centerLatLng, radiusKm) {
    var values = normalizeLocationValues({
      center_lat: centerLatLng.lat,
      center_lon: centerLatLng.lng,
      radius_km: radiusKm
    });
    if (!values) return null;
    writeLocationFields(values);
    rememberValidLocation(values);
    return values;
  }

  function persistLocation(values) {
    var nextValues = normalizeLocationValues(values || readLocationValuesFromFields());
    if (!nextValues) {
      if (restoreLastValidLocation()) {
        FW.showToast('Enter valid latitude, longitude, and radius values', 'error');
      } else {
        FW.showToast('Location settings are not ready yet', 'error');
      }
      return;
    }

    rememberValidLocation(nextValues);

    FW.post('/api/settings', nextValues).then(function(res) {
      if (res.body.ok) {
        FW.showToast('Location updated', 'success');
      } else {
        FW.showToast(res.body.error || 'Save failed', 'error');
      }
    }).catch(function() {
      FW.showToast('Network error', 'error');
    });
  }

  function queueLocationPersist(values) {
    clearTimeout(locationDebounce);
    locationDebounce = setTimeout(function() {
      persistLocation(values);
    }, DEBOUNCE_MS);
  }

  function destroyLocationMap() {
    if (mapInstance) {
      mapInstance.remove();
    }
    mapInstance = null;
    mapMarker = null;
    mapCircle = null;
    mapTileLayer = null;
    mapRadiusHandle = null;
  }

  function onLeafletFail(helperText) {
    if (mapFailureHandled) return;
    mapFailureHandled = true;
    clearTimeout(mapLoadTimeout);
    destroyLocationMap();
    hideLocationLoading();
    locationMap.style.display = 'none';
    locationHelper.textContent = helperText || 'Map unavailable. Enter coordinates manually.';
    FW.showToast('Map could not be loaded', 'warning');
  }

  function maybeInitLocationMap() {
    if (!leafletLoaded || mapFailureHandled || mapInstance || !isLocationCardOpen()) return;

    if (!lastValidLocation) {
      settingsPromise.then(function() {
        if (!lastValidLocation) {
          onLeafletFail('Map unavailable until location settings load. Enter coordinates manually.');
          return;
        }
        maybeInitLocationMap();
      });
      return;
    }

    initMap(lastValidLocation);
  }

  function toggleLocationCard() {
    var shouldOpen = !isLocationCardOpen();

    if (shouldOpen && isCalibrationOpen()) {
      setCalibrationOpen(false);
    }

    locationBody.style.display = shouldOpen ? '' : 'none';
    locationToggle.classList.toggle('open', shouldOpen);

    if (!shouldOpen) return;

    if (mapInstance) {
      setTimeout(function() {
        if (!mapInstance || !isLocationCardOpen()) return;
        mapInstance.invalidateSize();
        if (lastValidLocation) {
          updateMapFromValues(lastValidLocation, { fit: true, pan: false });
        }
      }, 200);
      return;
    }

    if (!leafletLoadAttempted) {
      leafletLoadAttempted = true;
      loadLeaflet();
      return;
    }

    maybeInitLocationMap();
  }

  locationToggle.addEventListener('click', toggleLocationCard);

  function loadLeaflet() {
    if (leafletLoaded) {
      maybeInitLocationMap();
      return;
    }

    mapFailureHandled = false;
    mapTileLoadSucceeded = false;
    mapTileErrorCount = 0;
    showLocationLoading('Loading map...');
    locationMap.style.display = 'none';

    var link = document.createElement('link');
    link.rel = 'stylesheet';
    link.href = 'https://unpkg.com/leaflet@1.9.4/dist/leaflet.css';

    var script = document.createElement('script');
    script.src = 'https://unpkg.com/leaflet@1.9.4/dist/leaflet.js';

    mapLoadTimeout = setTimeout(function() {
      onLeafletFail('Map timed out. Enter coordinates manually.');
    }, 10000);

    script.onload = function() {
      clearTimeout(mapLoadTimeout);
      if (mapFailureHandled) return;
      leafletLoaded = true;
      maybeInitLocationMap();
    };

    script.onerror = function() {
      clearTimeout(mapLoadTimeout);
      onLeafletFail('Map assets could not be loaded. Enter coordinates manually.');
    };

    link.onerror = function() {
      clearTimeout(mapLoadTimeout);
      onLeafletFail('Map assets could not be loaded. Enter coordinates manually.');
    };

    document.head.appendChild(link);
    document.head.appendChild(script);
  }

  function clampLat(v) { return Math.max(-90, Math.min(90, v)); }
  function clampLon(v) { return Math.max(-180, Math.min(180, v)); }
  function clampRadius(v) { return Math.max(0.1, Math.min(500, v)); }

  function initMap(initialValues) {
    var values = normalizeLocationValues(initialValues);
    if (!values) {
      onLeafletFail('Map unavailable until location settings load. Enter coordinates manually.');
      return;
    }

    locationMap.style.display = '';
    showLocationLoading('Loading map...');
    mapTileLoadSucceeded = false;
    mapTileErrorCount = 0;

    var lat = values.center_lat;
    var lon = values.center_lon;
    var radiusM = values.radius_km * 1000;

    mapInstance = L.map(locationMap, { zoomControl: true }).setView([lat, lon], LOCATION_INITIAL_ZOOM);

    mapTileLayer = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      maxZoom: 18,
      attribution: '&copy; OpenStreetMap'
    });

    mapTileLayer.on('tileload', function() {
      mapTileLoadSucceeded = true;
      hideLocationLoading();
    });
    mapTileLayer.on('load', function() {
      mapTileLoadSucceeded = true;
      hideLocationLoading();
    });
    mapTileLayer.on('tileerror', function() {
      if (mapFailureHandled || mapTileLoadSucceeded) return;
      mapTileErrorCount += 1;
      if (mapTileErrorCount >= LOCATION_TILE_ERROR_THRESHOLD) {
        onLeafletFail('Map tiles could not be loaded. Enter coordinates manually.');
      }
    });
    mapTileLayer.addTo(mapInstance);

    mapMarker = L.marker([lat, lon], { draggable: true, autoPan: true }).addTo(mapInstance);
    mapCircle = L.circle([lat, lon], { radius: radiusM, color: '#58a6ff', fillOpacity: 0.15 }).addTo(mapInstance);
    mapRadiusHandle = L.marker(getRadiusHandleLatLng(L.latLng(lat, lon), radiusM), {
      draggable: true,
      autoPan: true,
      icon: L.divIcon({
        className: 'location-radius-handle',
        iconSize: [16, 16],
        iconAnchor: [8, 8]
      })
    }).addTo(mapInstance);

    mapMarker.on('drag', function() {
      var pos = mapMarker.getLatLng();
      mapCircle.setLatLng(pos);
      var dragValues = syncLocationFieldsFromMap(pos, mapCircle.getRadius() / 1000);
      if (dragValues && mapRadiusHandle) {
        mapRadiusHandle.setLatLng(getRadiusHandleLatLng(pos, dragValues.radius_km * 1000));
      }
    });
    mapMarker.on('dragend', function() {
      var pos = mapMarker.getLatLng();
      var dragValues = syncLocationFieldsFromMap(pos, mapCircle.getRadius() / 1000);
      if (!dragValues) return;
      updateMapFromValues(dragValues, { fit: false, pan: false });
      persistLocation(dragValues);
    });

    mapRadiusHandle.on('drag', function() {
      var center = mapMarker.getLatLng();
      var radiusKm = clampRadius(center.distanceTo(mapRadiusHandle.getLatLng()) / 1000);
      mapCircle.setRadius(radiusKm * 1000);
      syncLocationFieldsFromMap(center, radiusKm);
    });
    mapRadiusHandle.on('dragend', function() {
      var center = mapMarker.getLatLng();
      var radiusKm = clampRadius(center.distanceTo(mapRadiusHandle.getLatLng()) / 1000);
      var radiusValues = syncLocationFieldsFromMap(center, radiusKm);
      if (!radiusValues) return;
      updateMapFromValues(radiusValues, { fit: false, pan: false });
      persistLocation(radiusValues);
    });

    rememberValidLocation(values);
    updateMapFromValues(values, { fit: true, pan: false });
    setTimeout(function() {
      if (!mapInstance || !isLocationCardOpen()) return;
      mapInstance.invalidateSize();
      updateMapFromValues(values, { fit: true, pan: false });
    }, 200);
  }

  function handleLocationFieldChange() {
    var values = readLocationValuesFromFields();
    if (!values) {
      if (restoreLastValidLocation()) {
        FW.showToast('Enter valid latitude, longitude, and radius values', 'error');
      } else {
        FW.showToast('Location settings are not ready yet', 'error');
      }
      return;
    }

    writeLocationFields(values);
    rememberValidLocation(values);
    updateMapFromValues(values, { fit: false, pan: true });
    queueLocationPersist(values);

    if (!mapInstance && leafletLoaded && !mapFailureHandled && isLocationCardOpen()) {
      maybeInitLocationMap();
    }
  }

  dCenterLat.addEventListener('change', handleLocationFieldChange);
  dCenterLon.addEventListener('change', handleLocationFieldChange);
  dRadiusKm.addEventListener('change', handleLocationFieldChange);

  // --- Calibration card (Story 4.2) ---
  var calToggle = document.querySelector('.calibration-toggle');
  var calBody = document.getElementById('calibration-body');
  var calOrigin = document.getElementById('cal-origin-corner');
  var calScanDir = document.getElementById('cal-scan-dir');
  var calZigzag = document.getElementById('cal-zigzag');
  var calCanvas = document.getElementById('cal-preview-canvas');
  var calPreviewContainer = document.getElementById('cal-preview-container');
  var wiringCanvas = document.getElementById('wiring-preview');
  var wiringLegend = document.getElementById('wiring-legend');
  var calibrationActive = false;
  var positioningActive = false;
  var calPattern = 0; // 0=scan order, 1=panel position
  var calPatternToggle = document.getElementById('cal-pattern-toggle');
  var calDebounce = null;
  var CAL_DEBOUNCE_MS = 50;

  function isCalibrationOpen() {
    return calBody.style.display !== 'none';
  }

  function readCalibrationValues() {
    return {
      origin_corner: parseInt(calOrigin.value, 10),
      scan_dir: parseInt(calScanDir.value, 10),
      zigzag: parseInt(calZigzag.value, 10)
    };
  }

  function syncCalibrationFromSettings() {
    calOrigin.value = dOriginCorner.value || '0';
    calScanDir.value = dScanDir.value || '0';
    calZigzag.value = dZigzag.value || '0';
  }

  function renderCalibrationCanvas() {
    if (!calCanvas || !calCanvas.getContext || !calPreviewContainer) return;
    var ctx = calCanvas.getContext('2d');
    var dims = parseHardwareDimensionsFromInputs();

    if (!dims || dims.matrixWidth <= 0 || dims.matrixHeight <= 0) {
      calCanvas.width = 0;
      calCanvas.height = 0;
      calPreviewContainer.style.display = 'none';
      return;
    }

    calPreviewContainer.style.display = '';

    var mw = dims.matrixWidth;
    var mh = dims.matrixHeight;
    var originCorner = parseInt(calOrigin.value, 10) || 0;
    var scanDir = parseInt(calScanDir.value, 10) || 0;
    var zigzag = parseInt(calZigzag.value, 10) || 0;

    var containerWidth = calPreviewContainer.clientWidth || 300;
    var aspect = mw / mh;
    var drawWidth = Math.min(containerWidth, 480);
    var drawHeight = Math.round(drawWidth / aspect);

    calCanvas.width = drawWidth;
    calCanvas.height = drawHeight;

    var sx = drawWidth / mw;
    var sy = drawHeight / mh;

    // Background
    ctx.fillStyle = '#0d1117';
    ctx.fillRect(0, 0, drawWidth, drawHeight);

    // Tile grid
    var tp = dims.tilePixels;
    var tileStepX = tp * sx;
    var tileStepY = tp * sy;
    if (tileStepX >= 2 && tileStepY >= 2) {
      ctx.strokeStyle = '#21262d';
      ctx.lineWidth = 1;
      for (var gx = tileStepX; gx < drawWidth; gx += tileStepX) {
        var x = Math.round(gx) + 0.5;
        ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, drawHeight); ctx.stroke();
      }
      for (var gy = tileStepY; gy < drawHeight; gy += tileStepY) {
        var y = Math.round(gy) + 0.5;
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(drawWidth, y); ctx.stroke();
      }
    }

    // Position pattern mode: colored tiles with numbers
    if (calPattern === 1) {
      var tilesX = dims.tilesX;
      var tilesY = dims.tilesY;
      var totalTiles = tilesX * tilesY;
      for (var tRow = 0; tRow < tilesY; tRow++) {
        for (var tCol = 0; tCol < tilesX; tCol++) {
          var tIdx = tRow * tilesX + tCol;
          var tileHue = tIdx / totalTiles;

          // Dim fill
          var dimR = Math.round(40 * Math.max(0, Math.cos(tileHue * Math.PI * 2)));
          var dimG = Math.round(40 * Math.max(0, Math.cos((tileHue - 0.333) * Math.PI * 2)));
          var dimB = Math.round(40 * Math.max(0, Math.cos((tileHue - 0.667) * Math.PI * 2)));
          // Bright border
          var brtR = Math.round(200 * Math.max(0, Math.cos(tileHue * Math.PI * 2)));
          var brtG = Math.round(200 * Math.max(0, Math.cos((tileHue - 0.333) * Math.PI * 2)));
          var brtB = Math.round(200 * Math.max(0, Math.cos((tileHue - 0.667) * Math.PI * 2)));

          var tpx = tCol * tp * sx;
          var tpy = tRow * tp * sy;
          var tw = tp * sx;
          var th = tp * sy;

          // Dim fill
          ctx.fillStyle = 'rgb(' + (dimR + 20) + ',' + (dimG + 20) + ',' + (dimB + 20) + ')';
          ctx.fillRect(tpx, tpy, tw, th);

          // Bright border
          ctx.strokeStyle = 'rgb(' + (brtR + 55) + ',' + (brtG + 55) + ',' + (brtB + 55) + ')';
          ctx.lineWidth = Math.max(1, Math.round(sx));
          ctx.strokeRect(tpx + 0.5, tpy + 0.5, tw - 1, th - 1);

          // Red corner marker
          var mSize = Math.max(3, Math.round(Math.min(tw, th) * 0.15));
          ctx.fillStyle = '#f00';
          ctx.fillRect(tpx, tpy, mSize, mSize);

          // Tile number centered
          var fontSize = Math.max(10, Math.round(Math.min(tw, th) * 0.35));
          ctx.font = 'bold ' + fontSize + 'px sans-serif';
          ctx.fillStyle = '#fff';
          ctx.textAlign = 'center';
          ctx.textBaseline = 'middle';
          ctx.fillText(String(tIdx), tpx + tw / 2, tpy + th / 2);
        }
      }

      // Matrix outline
      ctx.strokeStyle = '#30363d';
      ctx.lineWidth = 1;
      ctx.strokeRect(0, 0, drawWidth, drawHeight);
      return;
    }

    // Draw calibration test pattern: numbered pixel traversal showing scan order
    // Simulate the NeoMatrix pixel mapping based on origin_corner, scan_dir, zigzag
    var totalPixels = mw * mh;
    var tilesX = dims.tilesX;
    var tilesY = dims.tilesY;

    // Build pixel order map to visualize scan direction
    // For each logical pixel index (0..N-1), compute the (px, py) position
    // This simulates how NeoMatrix maps pixel indices to XY coordinates
    var pixelCount = Math.min(totalPixels, 4096); // Cap for performance
    var stepSize = totalPixels > 4096 ? Math.ceil(totalPixels / 4096) : 1;
    var positions = [];

    for (var py = 0; py < mh; py++) {
      for (var px = 0; px < mw; px++) {
        // Determine tile and pixel within tile
        var tileCol = Math.floor(px / tp);
        var tileRow = Math.floor(py / tp);
        var localX = px % tp;
        var localY = py % tp;

        // Apply origin corner transform
        var effTileCol = tileCol;
        var effTileRow = tileRow;
        var effLocalX = localX;
        var effLocalY = localY;

        // Origin corner: 0=TL, 1=TR, 2=BL, 3=BR
        if (originCorner === 1 || originCorner === 3) {
          effTileCol = tilesX - 1 - tileCol;
          effLocalX = tp - 1 - localX;
        }
        if (originCorner === 2 || originCorner === 3) {
          effTileRow = tilesY - 1 - tileRow;
          effLocalY = tp - 1 - localY;
        }

        // Scan direction: 0=rows, 1=columns
        var tileIndex, localIndex;
        if (scanDir === 0) {
          // Row-major tile order
          if (zigzag && (effTileRow % 2 === 1)) {
            effTileCol = tilesX - 1 - effTileCol;
          }
          tileIndex = effTileRow * tilesX + effTileCol;
          // Local pixel within tile: row-major
          if (zigzag && (effLocalY % 2 === 1)) {
            effLocalX = tp - 1 - effLocalX;
          }
          localIndex = effLocalY * tp + effLocalX;
        } else {
          // Column-major tile order
          if (zigzag && (effTileCol % 2 === 1)) {
            effTileRow = tilesY - 1 - effTileRow;
          }
          tileIndex = effTileCol * tilesY + effTileRow;
          // Local pixel within tile: column-major
          if (zigzag && (effLocalX % 2 === 1)) {
            effLocalY = tp - 1 - effLocalY;
          }
          localIndex = effLocalX * tp + effLocalY;
        }

        var pixelIndex = tileIndex * (tp * tp) + localIndex;
        positions.push({ px: px, py: py, idx: pixelIndex });
      }
    }

    // Draw pixels colored by their index in the scan order (gradient red -> blue)
    for (var i = 0; i < positions.length; i++) {
      var p = positions[i];
      var t = totalPixels > 1 ? p.idx / (totalPixels - 1) : 0;
      t = Math.max(0, Math.min(1, t));
      // Red (start) -> Blue (end) gradient
      var r = Math.round(248 * (1 - t));
      var g = Math.round(81 * (1 - t) + 166 * t);
      var b = Math.round(73 * (1 - t) + 255 * t);
      ctx.fillStyle = 'rgb(' + r + ',' + g + ',' + b + ')';
      var rx = Math.round(p.px * sx);
      var ry = Math.round(p.py * sy);
      var rw = Math.max(1, Math.round(sx));
      var rh = Math.max(1, Math.round(sy));
      ctx.fillRect(rx, ry, rw, rh);
    }

    // Draw pixel 0 marker (start) and last pixel marker
    if (positions.length > 0) {
      // Find pixel 0 position
      var startPixel = null;
      var endPixel = null;
      for (var j = 0; j < positions.length; j++) {
        if (positions[j].idx === 0) startPixel = positions[j];
        if (positions[j].idx === totalPixels - 1) endPixel = positions[j];
      }

      if (startPixel) {
        var markerSize = Math.max(4, Math.round(Math.min(sx, sy) * 2));
        ctx.fillStyle = '#f85149';
        ctx.fillRect(
          Math.round(startPixel.px * sx) - 1,
          Math.round(startPixel.py * sy) - 1,
          markerSize, markerSize
        );
        ctx.fillStyle = '#fff';
        ctx.font = Math.max(8, Math.round(Math.min(sx, sy) * 3)) + 'px sans-serif';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'top';
        ctx.fillText('0', Math.round(startPixel.px * sx) + markerSize + 2, Math.round(startPixel.py * sy));
      }
    }

    // Matrix outline
    ctx.strokeStyle = '#30363d';
    ctx.lineWidth = 1;
    ctx.strokeRect(0, 0, drawWidth, drawHeight);
  }

  function startCalibrationMode() {
    if (calibrationActive) return;
    calibrationActive = true;
    FW.post('/api/calibration/start', {}).then(function(res) {
      if (!res.body.ok) {
        FW.showToast(res.body.error || 'Calibration start failed', 'error');
        calibrationActive = false;
      }
    }).catch(function() {
      FW.showToast('Network error starting calibration', 'error');
      calibrationActive = false;
    });
  }

  function stopCalibrationMode() {
    if (!calibrationActive) return;
    calibrationActive = false;
    FW.post('/api/calibration/stop', {}).catch(function() {});
  }

  function startPositioningMode() {
    if (positioningActive) return;
    positioningActive = true;
    FW.post('/api/positioning/start', {}).then(function(res) {
      if (!res.body.ok) {
        FW.showToast(res.body.error || 'Positioning start failed', 'error');
        positioningActive = false;
      }
    }).catch(function() {
      FW.showToast('Network error starting positioning', 'error');
      positioningActive = false;
    });
  }

  function stopPositioningMode() {
    if (!positioningActive) return;
    positioningActive = false;
    FW.post('/api/positioning/stop', {}).catch(function() {});
  }

  function activatePattern() {
    if (calPattern === 1) {
      stopCalibrationMode();
      startPositioningMode();
    } else {
      stopPositioningMode();
      startCalibrationMode();
    }
  }

  function stopAllTestPatterns() {
    stopCalibrationMode();
    stopPositioningMode();
  }

  function setCalibrationOpen(shouldOpen) {
    calBody.style.display = shouldOpen ? '' : 'none';
    calToggle.classList.toggle('open', shouldOpen);

    if (shouldOpen) {
      syncCalibrationFromSettings();
      renderCalibrationCanvas();
      activatePattern();
    } else {
      stopAllTestPatterns();
    }
  }

  // --- Wiring diagram canvas ---
  function renderWiringCanvas() {
    if (!wiringCanvas || !wiringCanvas.getContext || !previewContainer) return;
    var ctx = wiringCanvas.getContext('2d');
    var dims = parseHardwareDimensionsFromInputs();

    if (!dims || dims.tilesX <= 0 || dims.tilesY <= 0) {
      wiringCanvas.width = 0;
      wiringCanvas.height = 0;
      wiringCanvas.style.display = 'none';
      if (wiringLegend) wiringLegend.style.display = 'none';
      return;
    }

    wiringCanvas.style.display = '';
    if (wiringLegend) wiringLegend.style.display = '';

    var tilesX = dims.tilesX;
    var tilesY = dims.tilesY;
    var originCorner = parseInt(calOrigin.value, 10) || 0;
    var scanDir = parseInt(calScanDir.value, 10) || 0;
    var zigzag = parseInt(calZigzag.value, 10) || 0;

    var containerWidth = previewContainer.clientWidth || 300;
    var aspect = tilesX / tilesY;
    var drawWidth = Math.min(containerWidth, 480);
    var drawHeight = Math.round(drawWidth / aspect);
    if (drawHeight < 60) drawHeight = 60;

    wiringCanvas.width = drawWidth;
    wiringCanvas.height = drawHeight;

    var cellW = drawWidth / tilesX;
    var cellH = drawHeight / tilesY;

    // Background
    ctx.fillStyle = '#0d1117';
    ctx.fillRect(0, 0, drawWidth, drawHeight);

    // Tile grid lines
    ctx.strokeStyle = '#30363d';
    ctx.lineWidth = 1;
    for (var gx = 0; gx <= tilesX; gx++) {
      var x = Math.round(gx * cellW) + 0.5;
      ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, drawHeight); ctx.stroke();
    }
    for (var gy = 0; gy <= tilesY; gy++) {
      var y = Math.round(gy * cellH) + 0.5;
      ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(drawWidth, y); ctx.stroke();
    }

    // Build tile traversal order (same algorithm as calibration but at tile level)
    var tilePositions = [];
    for (var tRow = 0; tRow < tilesY; tRow++) {
      for (var tCol = 0; tCol < tilesX; tCol++) {
        var effCol = tCol;
        var effRow = tRow;

        // Origin corner transform
        if (originCorner === 1 || originCorner === 3) {
          effCol = tilesX - 1 - tCol;
        }
        if (originCorner === 2 || originCorner === 3) {
          effRow = tilesY - 1 - tRow;
        }

        // Scan direction + zigzag
        var tileIndex;
        if (scanDir === 0) {
          // Row-major
          if (zigzag && (effRow % 2 === 1)) {
            effCol = tilesX - 1 - effCol;
          }
          tileIndex = effRow * tilesX + effCol;
        } else {
          // Column-major
          if (zigzag && (effCol % 2 === 1)) {
            effRow = tilesY - 1 - effRow;
          }
          tileIndex = effCol * tilesY + effRow;
        }

        tilePositions.push({
          col: tCol, row: tRow,
          cx: (tCol + 0.5) * cellW,
          cy: (tRow + 0.5) * cellH,
          idx: tileIndex
        });
      }
    }

    // Sort by tile index to get wiring order
    tilePositions.sort(function(a, b) { return a.idx - b.idx; });

    // Draw cable path (thick blue polyline)
    if (tilePositions.length > 1) {
      ctx.strokeStyle = '#58a6ff';
      ctx.lineWidth = 3;
      ctx.lineJoin = 'round';
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(tilePositions[0].cx, tilePositions[0].cy);
      for (var i = 1; i < tilePositions.length; i++) {
        ctx.lineTo(tilePositions[i].cx, tilePositions[i].cy);
      }
      ctx.stroke();

      // Draw arrowheads along path at each segment midpoint
      ctx.fillStyle = '#58a6ff';
      for (var j = 0; j < tilePositions.length - 1; j++) {
        var ax = tilePositions[j].cx;
        var ay = tilePositions[j].cy;
        var bx = tilePositions[j + 1].cx;
        var by = tilePositions[j + 1].cy;
        var mx = (ax + bx) / 2;
        var my = (ay + by) / 2;
        var angle = Math.atan2(by - ay, bx - ax);
        var arrowSize = Math.min(cellW, cellH) * 0.18;
        if (arrowSize < 4) arrowSize = 4;
        ctx.save();
        ctx.translate(mx, my);
        ctx.rotate(angle);
        ctx.beginPath();
        ctx.moveTo(arrowSize, 0);
        ctx.lineTo(-arrowSize * 0.6, -arrowSize * 0.6);
        ctx.lineTo(-arrowSize * 0.6, arrowSize * 0.6);
        ctx.closePath();
        ctx.fill();
        ctx.restore();
      }
    }

    // Draw tile index numbers
    var fontSize = Math.max(10, Math.min(cellW, cellH) * 0.35);
    ctx.font = 'bold ' + Math.round(fontSize) + 'px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#e6edf3';
    for (var k = 0; k < tilePositions.length; k++) {
      var tp = tilePositions[k];
      ctx.fillText(String(tp.idx), tp.cx, tp.cy);
    }

    // Green marker at tile 0 (data input)
    if (tilePositions.length > 0) {
      var t0 = tilePositions[0];
      var markerR = Math.max(5, Math.min(cellW, cellH) * 0.15);
      ctx.beginPath();
      ctx.arc(t0.cx, t0.cy - cellH * 0.35, markerR, 0, Math.PI * 2);
      ctx.fillStyle = '#3fb950';
      ctx.fill();
      ctx.fillStyle = '#fff';
      ctx.font = 'bold ' + Math.round(markerR * 1.1) + 'px sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText('IN', t0.cx, t0.cy - cellH * 0.35);
    }

    // Matrix outline
    ctx.strokeStyle = '#58a6ff';
    ctx.lineWidth = 2;
    ctx.strokeRect(1, 1, drawWidth - 2, drawHeight - 2);
  }

  function onCalibrationChange() {
    var values = readCalibrationValues();

    // Sync back to hardware fields so Apply picks them up
    dOriginCorner.value = values.origin_corner;
    dScanDir.value = values.scan_dir;
    dZigzag.value = values.zigzag;

    // Instant canvas update
    renderCalibrationCanvas();
    renderWiringCanvas();

    // Debounced server POST (persists + updates LED test pattern)
    clearTimeout(calDebounce);
    calDebounce = setTimeout(function() {
      FW.post('/api/settings', {
        origin_corner: values.origin_corner,
        scan_dir: values.scan_dir,
        zigzag: values.zigzag
      }).then(function(res) {
        if (!res.body.ok) {
          FW.showToast(res.body.error || 'Save failed', 'error');
        } else if (res.body.reboot_required) {
          FW.showToast('Calibration saved. Reboot required to apply live mapping.', 'warning');
        } else {
          FW.showToast('Calibration updated', 'success');
        }
      }).catch(function() {
        FW.showToast('Network error', 'error');
      });
    }, CAL_DEBOUNCE_MS);
  }

  calOrigin.addEventListener('change', onCalibrationChange);
  calScanDir.addEventListener('change', onCalibrationChange);
  calZigzag.addEventListener('change', onCalibrationChange);

  function toggleCalibrationCard() {
    var shouldOpen = !isCalibrationOpen();
    setCalibrationOpen(shouldOpen);
  }

  calToggle.addEventListener('click', toggleCalibrationCard);

  // Pattern toggle buttons
  if (calPatternToggle) {
    var patternBtns = calPatternToggle.querySelectorAll('.cal-pattern-btn');
    for (var pi = 0; pi < patternBtns.length; pi++) {
      patternBtns[pi].addEventListener('click', function() {
        calPattern = parseInt(this.getAttribute('data-pattern'), 10) || 0;
        var all = calPatternToggle.querySelectorAll('.cal-pattern-btn');
        for (var j = 0; j < all.length; j++) all[j].classList.remove('active');
        this.classList.add('active');
        renderCalibrationCanvas();
        activatePattern();
      });
    }
  }

  // Stop all test patterns on page unload
  window.addEventListener('beforeunload', function() {
    try {
      if (calibrationActive) {
        var xhr1 = new XMLHttpRequest();
        xhr1.open('POST', '/api/calibration/stop', false);
        xhr1.setRequestHeader('Content-Type', 'application/json');
        xhr1.send('{}');
      }
      if (positioningActive) {
        var xhr2 = new XMLHttpRequest();
        xhr2.open('POST', '/api/positioning/stop', false);
        xhr2.setRequestHeader('Content-Type', 'application/json');
        xhr2.send('{}');
      }
    } catch (e) { /* best effort */ }
  });

  // Resize handler for calibration canvas
  window.addEventListener('resize', function() {
    if (isCalibrationOpen()) {
      renderCalibrationCanvas();
    }
  });

  // --- Logo upload (Story 4.3) ---
  var logoUploadZone = document.getElementById('logo-upload-zone');
  var logoFileInput = document.getElementById('logo-file-input');
  var logoFileList = document.getElementById('logo-file-list');
  var btnUploadLogos = document.getElementById('btn-upload-logos');
  var LOGO_PREVIEW_SIZE = 128;
  var logoPendingFiles = []; // { file, valid, error, previewDataUrl, row }

  function resetLogoUploadState() {
    logoPendingFiles = [];
    logoFileList.innerHTML = '';
    btnUploadLogos.style.display = 'none';
  }

  // Drag and drop
  logoUploadZone.addEventListener('dragover', function(e) {
    e.preventDefault();
    logoUploadZone.classList.add('drag-over');
  });
  logoUploadZone.addEventListener('dragleave', function() {
    logoUploadZone.classList.remove('drag-over');
  });
  logoUploadZone.addEventListener('drop', function(e) {
    e.preventDefault();
    logoUploadZone.classList.remove('drag-over');
    if (e.dataTransfer && e.dataTransfer.files) {
      processLogoFiles(e.dataTransfer.files);
    }
  });

  // File picker
  logoFileInput.addEventListener('change', function() {
    if (logoFileInput.files && logoFileInput.files.length) {
      processLogoFiles(logoFileInput.files);
    }
    logoFileInput.value = '';
  });

  function processLogoFiles(fileList) {
    resetLogoUploadState();
    var files = Array.prototype.slice.call(fileList);

    files.forEach(function(file) {
      var entry = { file: file, valid: false, error: '', previewDataUrl: null, row: null };

      // Validate extension
      if (!file.name.toLowerCase().endsWith('.bin')) {
        entry.error = file.name + ' - invalid format or size (.bin only)';
        logoPendingFiles.push(entry);
        renderLogoRow(entry);
        return;
      }

      // Validate size
      if (file.size !== 2048) {
        entry.error = file.name + ' - invalid format or size (' + file.size + ' bytes, expected 2048)';
        logoPendingFiles.push(entry);
        renderLogoRow(entry);
        return;
      }

      // Valid — decode RGB565 for preview
      entry.valid = true;
      logoPendingFiles.push(entry);
      renderLogoRow(entry);
      decodeRgb565Preview(entry);
    });

    updateUploadButton();
  }

  function decodeRgb565Preview(entry) {
    var reader = new FileReader();
    reader.onload = function() {
      if (reader.result.byteLength !== 2048) {
        entry.valid = false;
        entry.error = 'Decode error: unexpected pixel count';
        updateRowState(entry);
        updateUploadButton();
        return;
      }

      // Decode RGB565 (big-endian) -> RGBA for canvas
      var view = new DataView(reader.result);
      var canvas = document.createElement('canvas');
      canvas.width = 32;
      canvas.height = 32;
      var ctx = canvas.getContext('2d');
      var imgData = ctx.createImageData(32, 32);
      var pixels = imgData.data;

      for (var i = 0; i < 1024; i++) {
        var px = view.getUint16(i * 2, false); // big-endian
        var r5 = (px >> 11) & 0x1F;
        var g6 = (px >> 5) & 0x3F;
        var b5 = px & 0x1F;
        pixels[i * 4]     = (r5 << 3) | (r5 >> 2);
        pixels[i * 4 + 1] = (g6 << 2) | (g6 >> 4);
        pixels[i * 4 + 2] = (b5 << 3) | (b5 >> 2);
        pixels[i * 4 + 3] = 255;
      }

      ctx.putImageData(imgData, 0, 0);

      // Render scaled preview into the row's canvas
      if (entry.row) {
        var previewCanvas = entry.row.querySelector('.logo-file-preview');
        if (previewCanvas && previewCanvas.getContext) {
          var pctx = previewCanvas.getContext('2d');
          previewCanvas.width = LOGO_PREVIEW_SIZE;
          previewCanvas.height = LOGO_PREVIEW_SIZE;
          pctx.imageSmoothingEnabled = false;
          pctx.drawImage(canvas, 0, 0, LOGO_PREVIEW_SIZE, LOGO_PREVIEW_SIZE);
        }
      }
    };
    reader.onerror = function() {
      entry.valid = false;
      entry.error = 'File read error';
      updateRowState(entry);
      updateUploadButton();
    };
    reader.readAsArrayBuffer(entry.file);
  }

  function renderLogoRow(entry) {
    var row = document.createElement('div');
    row.className = 'logo-file-row' + (entry.valid ? '' : ' file-error');

    var preview = document.createElement('canvas');
    preview.className = 'logo-file-preview';
    preview.width = LOGO_PREVIEW_SIZE;
    preview.height = LOGO_PREVIEW_SIZE;

    var info = document.createElement('div');
    info.className = 'logo-file-info';
    var nameEl = document.createElement('span');
    nameEl.className = 'logo-file-name';
    nameEl.textContent = entry.file.name;
    var statusEl = document.createElement('span');
    statusEl.className = 'logo-file-status' + (entry.error ? ' status-error' : '');
    statusEl.textContent = entry.error || (entry.valid ? 'Ready' : '');
    info.appendChild(nameEl);
    info.appendChild(statusEl);

    var removeBtn = document.createElement('button');
    removeBtn.className = 'logo-file-remove';
    removeBtn.textContent = '\u00D7';
    removeBtn.setAttribute('aria-label', 'Remove');
    removeBtn.addEventListener('click', function() {
      var idx = logoPendingFiles.indexOf(entry);
      if (idx >= 0) logoPendingFiles.splice(idx, 1);
      row.parentNode.removeChild(row);
      updateUploadButton();
    });

    row.appendChild(preview);
    row.appendChild(info);
    row.appendChild(removeBtn);

    entry.row = row;
    logoFileList.appendChild(row);
  }

  function updateRowState(entry) {
    if (!entry.row) return;
    entry.row.className = 'logo-file-row' + (entry.valid ? '' : ' file-error');
    var statusEl = entry.row.querySelector('.logo-file-status');
    if (statusEl) {
      statusEl.className = 'logo-file-status' + (entry.error ? ' status-error' : '');
      statusEl.textContent = entry.error || (entry.valid ? 'Ready' : '');
    }
  }

  function updateUploadButton() {
    var hasValid = logoPendingFiles.some(function(e) { return e.valid; });
    btnUploadLogos.style.display = hasValid ? '' : 'none';
  }

  // Upload queue: POST one file at a time to /api/logos
  btnUploadLogos.addEventListener('click', function() {
    var validFiles = logoPendingFiles.filter(function(e) { return e.valid; });
    if (validFiles.length === 0) return;

    btnUploadLogos.disabled = true;
    btnUploadLogos.textContent = 'Uploading...';

    var successes = 0;
    var failures = 0;
    var idx = 0;
    var failureMessages = [];

    function uploadNext() {
      if (idx >= validFiles.length) {
        // All done
        btnUploadLogos.disabled = false;
        btnUploadLogos.textContent = 'Upload';

        if (successes > 0 && failures === 0) {
          FW.showToast(successes + ' logo' + (successes > 1 ? 's' : '') + ' uploaded', 'success');
        } else if (successes > 0 && failures > 0) {
          FW.showToast(successes + ' uploaded, ' + failures + ' failed: ' + failureMessages.join('; '), 'error');
        } else {
          FW.showToast(failureMessages.join('; ') || 'All uploads failed', 'error');
        }

        // Remove successfully uploaded entries
        logoPendingFiles = logoPendingFiles.filter(function(e) {
          if (e._uploaded) {
            if (e.row && e.row.parentNode) e.row.parentNode.removeChild(e.row);
            return false;
          }
          return true;
        });
        updateUploadButton();
        // Refresh logo list after upload completes
        if (successes > 0) loadLogoList();
        return;
      }

      var entry = validFiles[idx];
      idx++;

      var statusEl = entry.row ? entry.row.querySelector('.logo-file-status') : null;
      if (statusEl) {
        statusEl.className = 'logo-file-status';
        statusEl.textContent = 'Uploading...';
      }

      var formData = new FormData();
      formData.append('file', entry.file, entry.file.name);

      fetch('/api/logos', {
        method: 'POST',
        body: formData
      }).then(function(res) {
        return res.json().then(function(body) {
          return { status: res.status, body: body };
        });
      }).then(function(res) {
        if (res.body.ok) {
          successes++;
          entry._uploaded = true;
          if (entry.row) {
            entry.row.className = 'logo-file-row file-ok';
          }
          if (statusEl) {
            statusEl.className = 'logo-file-status status-ok';
            statusEl.textContent = 'Uploaded';
          }
        } else {
          failures++;
          entry.valid = false;
          entry.error = res.body.error || 'Upload failed';
          failureMessages.push(entry.file.name + ' - ' + entry.error);
          if (entry.row) {
            entry.row.className = 'logo-file-row file-error';
          }
          if (statusEl) {
            statusEl.className = 'logo-file-status status-error';
            statusEl.textContent = entry.error;
          }
        }
        uploadNext();
      }).catch(function() {
        failures++;
        entry.valid = false;
        entry.error = 'Network error';
        failureMessages.push(entry.file.name + ' - Network error');
        if (entry.row) {
          entry.row.className = 'logo-file-row file-error';
        }
        if (statusEl) {
          statusEl.className = 'logo-file-status status-error';
          statusEl.textContent = 'Network error';
        }
        uploadNext();
      });
    }

    uploadNext();
  });

  // --- Logo list management (Story 4.4) ---
  var logoListEl = document.getElementById('logo-list');
  var logoEmptyState = document.getElementById('logo-empty-state');
  var logoStorageSummary = document.getElementById('logo-storage-summary');
  var logoListConfirmingRow = null; // only one row in confirm state at a time
  var logoDeleteInFlight = false;

  function formatBytes(bytes) {
    if (bytes < 1024) return bytes + ' B';
    var kb = bytes / 1024;
    if (kb < 1024) return Math.round(kb) + ' KB';
    var mb = kb / 1024;
    return (Math.round(mb * 10) / 10) + ' MB';
  }

  function loadLogoList() {
    FW.get('/api/logos').then(function(res) {
      if (!res.body.ok) {
        FW.showToast(res.body.error || 'Could not load logos. Check the device and try again.', 'error');
        return;
      }

      var logos = res.body.data || [];
      var storage = res.body.storage || {};

      // Sort deterministically by name for stable order
      logos.sort(function(a, b) {
        return a.name.localeCompare(b.name);
      });

      // Storage summary
      if (storage.used !== undefined && storage.total !== undefined) {
        logoStorageSummary.textContent = 'Storage: ' + formatBytes(storage.used) + ' / ' + formatBytes(storage.total) + ' used (' + (storage.logo_count || 0) + ' logos)';
        logoStorageSummary.style.display = '';
      } else {
        logoStorageSummary.style.display = 'none';
      }

      // Empty state vs list
      logoListEl.innerHTML = '';
      logoListConfirmingRow = null;
      if (logos.length === 0) {
        logoEmptyState.style.display = '';
        return;
      }
      logoEmptyState.style.display = 'none';

      logos.forEach(function(logo) {
        renderLogoListRow(logo);
      });
    }).catch(function() {
      FW.showToast('Cannot reach the device to load logos. Check connection and try again.', 'error');
    });
  }

  function renderLogoListRow(logo) {
    var row = document.createElement('div');
    row.className = 'logo-list-row';
    row.setAttribute('data-filename', logo.name);

    // Thumbnail canvas — load binary from device and decode RGB565
    var thumb = document.createElement('canvas');
    thumb.className = 'logo-list-thumb';
    thumb.width = 48;
    thumb.height = 48;

    var info = document.createElement('div');
    info.className = 'logo-list-info';
    var nameEl = document.createElement('span');
    nameEl.className = 'logo-list-name';
    nameEl.textContent = logo.name;
    var sizeEl = document.createElement('span');
    sizeEl.className = 'logo-list-size';
    sizeEl.textContent = formatBytes(logo.size);
    info.appendChild(nameEl);
    info.appendChild(sizeEl);

    var actions = document.createElement('div');
    actions.className = 'logo-list-actions';

    var deleteBtn = document.createElement('button');
    deleteBtn.className = 'logo-list-delete';
    deleteBtn.textContent = 'Delete';
    deleteBtn.type = 'button';
    deleteBtn.addEventListener('click', function() {
      showInlineConfirm(row, logo.name, actions);
    });
    actions.appendChild(deleteBtn);

    row.appendChild(thumb);
    row.appendChild(info);
    row.appendChild(actions);
    logoListEl.appendChild(row);

    // Load thumbnail preview from device
    loadLogoThumbnail(thumb, logo.name);
  }

  function loadLogoThumbnail(canvas, filename) {
    fetch('/logos/' + encodeURIComponent(filename))
      .then(function(res) {
        if (!res.ok) return null;
        return res.arrayBuffer();
      })
      .then(function(buf) {
        if (!buf || buf.byteLength !== 2048) return;
        var view = new DataView(buf);
        var ctx = canvas.getContext('2d');
        var imgData = ctx.createImageData(32, 32);
        var d = imgData.data;
        for (var i = 0; i < 1024; i++) {
          var px = view.getUint16(i * 2, false); // big-endian
          var r5 = (px >> 11) & 0x1F;
          var g6 = (px >> 5) & 0x3F;
          var b5 = px & 0x1F;
          d[i * 4]     = (r5 << 3) | (r5 >> 2);
          d[i * 4 + 1] = (g6 << 2) | (g6 >> 4);
          d[i * 4 + 2] = (b5 << 3) | (b5 >> 2);
          d[i * 4 + 3] = 255;
        }
        // Draw 32x32 to offscreen then scale to canvas
        var offscreen = document.createElement('canvas');
        offscreen.width = 32;
        offscreen.height = 32;
        offscreen.getContext('2d').putImageData(imgData, 0, 0);
        canvas.width = 48;
        canvas.height = 48;
        ctx = canvas.getContext('2d');
        ctx.imageSmoothingEnabled = false;
        ctx.drawImage(offscreen, 0, 0, 48, 48);
      })
      .catch(function() { /* thumbnail load failed — leave blank */ });
  }

  function showInlineConfirm(row, filename, actionsEl) {
    // Dismiss any other confirming row first
    if (logoListConfirmingRow && logoListConfirmingRow !== row) {
      resetRowActions(logoListConfirmingRow);
    }
    logoListConfirmingRow = row;

    actionsEl.innerHTML = '';

    var text = document.createElement('span');
    text.className = 'logo-list-confirm-text';
    text.textContent = 'Delete ' + filename + '?';

    var confirmBtn = document.createElement('button');
    confirmBtn.className = 'logo-list-confirm-btn';
    confirmBtn.textContent = 'Confirm';
    confirmBtn.type = 'button';
    confirmBtn.addEventListener('click', function() {
      executeDelete(row, filename, actionsEl, confirmBtn);
    });

    var cancelBtn = document.createElement('button');
    cancelBtn.className = 'logo-list-cancel-btn';
    cancelBtn.textContent = 'Cancel';
    cancelBtn.type = 'button';
    cancelBtn.addEventListener('click', function() {
      resetRowActions(row);
      logoListConfirmingRow = null;
    });

    actionsEl.appendChild(text);
    actionsEl.appendChild(confirmBtn);
    actionsEl.appendChild(cancelBtn);
  }

  function resetRowActions(row) {
    var actionsEl = row.querySelector('.logo-list-actions');
    if (!actionsEl) return;
    var filename = row.getAttribute('data-filename');
    actionsEl.innerHTML = '';
    var deleteBtn = document.createElement('button');
    deleteBtn.className = 'logo-list-delete';
    deleteBtn.textContent = 'Delete';
    deleteBtn.type = 'button';
    deleteBtn.addEventListener('click', function() {
      showInlineConfirm(row, filename, actionsEl);
    });
    actionsEl.appendChild(deleteBtn);
  }

  function executeDelete(row, filename, actionsEl, confirmBtn) {
    if (logoDeleteInFlight) return;
    logoDeleteInFlight = true;
    confirmBtn.disabled = true;
    confirmBtn.textContent = 'Deleting...';

    FW.del('/api/logos/' + encodeURIComponent(filename))
      .then(function(res) {
        logoDeleteInFlight = false;
        logoListConfirmingRow = null;
        if (res.body.ok) {
          FW.showToast('Logo deleted', 'success');
          loadLogoList();
        } else {
          var errMsg = res.body.error || 'Delete failed';
          if (res.body.code === 'NOT_FOUND') errMsg = filename + ' not found';
          FW.showToast(errMsg, 'error');
          resetRowActions(row);
        }
      })
      .catch(function() {
        logoDeleteInFlight = false;
        logoListConfirmingRow = null;
        FW.showToast('Cannot reach the device to delete ' + filename + '. Check connection and try again.', 'error');
        resetRowActions(row);
      });
  }

  // --- Unified Apply ---
  btnApplyAll.addEventListener('click', function() {
    var payload = collectPayload();
    if (payload === null) return; // validation failed

    if (Object.keys(payload).length === 0) {
      clearDirtyState();
      return;
    }

    applyWithReboot(payload, btnApplyAll, 'Apply Changes');

    // Clear dirty state after successful send (applyWithReboot handles UI)
    // We clear immediately since applyWithReboot manages the button state
    clearDirtyState();
  });

  // --- Mode Picker (Story dl-1.5) ---
  var modeStatusName = document.getElementById('modeStatusName');
  var modeStatusReason = document.getElementById('modeStatusReason');
  var modeCardsList = document.getElementById('modeCardsList');
  var modeSwitchInFlight = false;

  function updateModeStatus(data) {
    var activeMode = null;
    if (data.modes && data.active) {
      for (var i = 0; i < data.modes.length; i++) {
        if (data.modes[i].id === data.active) {
          activeMode = data.modes[i];
          break;
        }
      }
    }
    // Batch DOM update — status line + card subtitles in one operation
    if (modeStatusName && activeMode) {
      modeStatusName.textContent = activeMode.name;
    }
    if (modeStatusReason) {
      modeStatusReason.textContent = data.state_reason || 'unknown';
    }
    // Update mode cards (active state + subtitle)
    if (modeCardsList) {
      var cards = modeCardsList.querySelectorAll('.mode-card');
      for (var j = 0; j < cards.length; j++) {
        var cardId = cards[j].getAttribute('data-mode-id');
        var subtitle = cards[j].querySelector('.mode-card-subtitle');
        if (cardId === data.active) {
          cards[j].classList.add('active');
          if (subtitle) subtitle.textContent = data.state_reason || '';
        } else {
          cards[j].classList.remove('active');
          if (subtitle) subtitle.textContent = '';
        }
        cards[j].classList.remove('switching');
      }
    }
  }

  function renderModeCards(data) {
    if (!modeCardsList || !data.modes) return;
    modeCardsList.innerHTML = '';
    for (var i = 0; i < data.modes.length; i++) {
      var mode = data.modes[i];
      var card = document.createElement('div');
      card.className = 'mode-card' + (mode.id === data.active ? ' active' : '');
      card.setAttribute('data-mode-id', mode.id);
      var nameEl = document.createElement('div');
      nameEl.className = 'mode-card-name';
      nameEl.textContent = mode.name;
      card.appendChild(nameEl);
      var subtitleEl = document.createElement('div');
      subtitleEl.className = 'mode-card-subtitle';
      subtitleEl.textContent = (mode.id === data.active) ? (data.state_reason || '') : '';
      card.appendChild(subtitleEl);
      card.addEventListener('click', (function(modeId) {
        return function() { switchMode(modeId); };
      })(mode.id));
      modeCardsList.appendChild(card);
    }
  }

  function switchMode(modeId) {
    if (modeSwitchInFlight) return;
    modeSwitchInFlight = true;
    // Set switching state on the target card
    if (modeCardsList) {
      var cards = modeCardsList.querySelectorAll('.mode-card');
      for (var i = 0; i < cards.length; i++) {
        if (cards[i].getAttribute('data-mode-id') === modeId) {
          cards[i].classList.add('switching');
          var sub = cards[i].querySelector('.mode-card-subtitle');
          if (sub) sub.textContent = 'Switching...';
        }
      }
    }
    FW.post('/api/display/mode', { mode_id: modeId }).then(function(res) {
      modeSwitchInFlight = false;
      if (!res.body || !res.body.ok) {
        var errMsg = (res.body && res.body.error) ? res.body.error : 'Mode switch failed';
        FW.showToast(errMsg, 'error');
        loadDisplayModes(); // refresh to clear switching state
        return;
      }
      // Re-fetch full mode state for consistent update (AC #4, #8)
      loadDisplayModes();
    }).catch(function() {
      modeSwitchInFlight = false;
      FW.showToast('Cannot reach device. Check connection.', 'error');
      loadDisplayModes();
    });
  }

  function loadDisplayModes() {
    FW.get('/api/display/modes').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) {
        FW.showToast('Failed to load display modes', 'error');
        return;
      }
      var data = res.body.data;
      renderModeCards(data);
      updateModeStatus(data);
    }).catch(function() {
      FW.showToast('Cannot reach device to load display modes. Check connection.', 'error');
    });
  }

  // --- Firmware card / OTA Upload (Story fn-1.6) ---
  var otaUploadZone = document.getElementById('ota-upload-zone');
  var otaFileInput = document.getElementById('ota-file-input');
  var otaFileInfo = document.getElementById('ota-file-info');
  var otaFileName = document.getElementById('ota-file-name');
  var btnUploadFirmware = document.getElementById('btn-upload-firmware');
  var otaProgress = document.getElementById('ota-progress');
  var otaProgressBar = document.getElementById('ota-progress-bar');
  var otaProgressText = document.getElementById('ota-progress-text');
  var otaReboot = document.getElementById('ota-reboot');
  var otaRebootText = document.getElementById('ota-reboot-text');
  var fwVersion = document.getElementById('fw-version');
  var rollbackBanner = document.getElementById('rollback-banner');
  var btnDismissRollback = document.getElementById('btn-dismiss-rollback');
  var otaPendingFile = null;
  var OTA_MAX_SIZE = 1572864; // 1.5MB = 0x180000
  var btnCancelOta = document.getElementById('btn-cancel-ota');

  function resetOtaUploadState() {
    otaPendingFile = null;
    otaUploadZone.style.display = '';
    otaFileInfo.style.display = 'none';
    otaProgress.style.display = 'none';
    otaReboot.style.display = 'none';
    otaProgressBar.style.width = '0%';
    otaProgress.setAttribute('aria-valuenow', '0');
    otaProgressText.textContent = '0%';
    // Reset reboot text and color so a subsequent upload starts clean
    otaRebootText.textContent = '';
    otaRebootText.style.color = '';
  }

  // Cancel file selection — return to upload zone
  if (btnCancelOta) {
    btnCancelOta.addEventListener('click', function() {
      resetOtaUploadState();
    });
  }

  function loadFirmwareStatus() {
    FW.get('/api/status').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) return;
      var d = res.body.data;
      if (d.firmware_version) {
        fwVersion.textContent = 'Version: v' + d.firmware_version;
      }
      if (d.rollback_detected && !d.rollback_acknowledged) {
        rollbackBanner.style.display = '';
      } else {
        rollbackBanner.style.display = 'none';
      }
    }).catch(function() {
      FW.showToast('Could not load firmware status \u2014 check connection', 'error');
    });
  }

  // Rollback banner dismiss
  if (btnDismissRollback) {
    btnDismissRollback.addEventListener('click', function() {
      FW.post('/api/ota/ack-rollback', {}).then(function(res) {
        if (res.body.ok) {
          rollbackBanner.style.display = 'none';
        }
      }).catch(function() {
        FW.showToast('Could not dismiss rollback banner', 'error');
      });
    });
  }

  // Click/keyboard to open file picker
  otaUploadZone.addEventListener('click', function() {
    otaFileInput.click();
  });
  otaUploadZone.addEventListener('keydown', function(e) {
    if (e.key === 'Enter' || e.key === ' ') {
      e.preventDefault();
      otaFileInput.click();
    }
  });

  // Drag and drop
  otaUploadZone.addEventListener('dragenter', function(e) {
    e.preventDefault();
    otaUploadZone.classList.add('drag-over');
  });
  otaUploadZone.addEventListener('dragover', function(e) {
    e.preventDefault();
    otaUploadZone.classList.add('drag-over');
  });
  otaUploadZone.addEventListener('dragleave', function(e) {
    // Only remove drag-over when the cursor truly leaves the zone, not
    // when it moves over a child element (which fires dragleave on parent).
    if (!otaUploadZone.contains(e.relatedTarget)) {
      otaUploadZone.classList.remove('drag-over');
    }
  });
  otaUploadZone.addEventListener('drop', function(e) {
    e.preventDefault();
    otaUploadZone.classList.remove('drag-over');
    if (e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files.length > 0) {
      validateAndSelectFile(e.dataTransfer.files[0]);
    }
  });

  // File input change
  otaFileInput.addEventListener('change', function() {
    if (otaFileInput.files && otaFileInput.files.length > 0) {
      validateAndSelectFile(otaFileInput.files[0]);
    }
    otaFileInput.value = '';
  });

  function validateAndSelectFile(file) {
    // Size check — reject empty files and files exceeding the OTA partition limit
    if (file.size === 0) {
      FW.showToast('File is empty \u2014 select a valid firmware .bin file', 'error');
      resetOtaUploadState();
      return;
    }
    if (file.size > OTA_MAX_SIZE) {
      FW.showToast('File too large \u2014 maximum 1.5MB for OTA partition', 'error');
      resetOtaUploadState();
      return;
    }

    // Magic byte check
    var reader = new FileReader();
    reader.onload = function(e) {
      var bytes = new Uint8Array(e.target.result);
      if (bytes[0] !== 0xE9) {
        FW.showToast('Not a valid ESP32 firmware image', 'error');
        resetOtaUploadState();
        return;
      }
      // Valid — show file info
      otaPendingFile = file;
      otaUploadZone.style.display = 'none';
      otaFileInfo.style.display = '';
      otaFileName.textContent = file.name;
    };
    reader.onerror = function() {
      FW.showToast('Could not read file', 'error');
      resetOtaUploadState();
    };
    reader.readAsArrayBuffer(file.slice(0, 4));
  }

  // Upload firmware via XMLHttpRequest (not fetch — XHR required for upload progress events)
  if (btnUploadFirmware) {
    btnUploadFirmware.addEventListener('click', function() {
      if (!otaPendingFile) return;
      uploadFirmware(otaPendingFile);
    });
  }

  function uploadFirmware(file) {
    // Prevent double-submit (e.g. rapid double-tap on slow connections)
    if (btnUploadFirmware) btnUploadFirmware.disabled = true;

    var xhr = new XMLHttpRequest();
    var formData = new FormData();
    formData.append('firmware', file, file.name);

    // Show progress bar
    otaFileInfo.style.display = 'none';
    otaProgress.style.display = '';

    xhr.upload.onprogress = function(e) {
      if (e.lengthComputable) {
        var pct = Math.round((e.loaded / e.total) * 100);
        updateOtaProgress(pct);
      }
    };

    xhr.onload = function() {
      if (btnUploadFirmware) btnUploadFirmware.disabled = false;
      if (xhr.status === 200) {
        try {
          var resp = JSON.parse(xhr.responseText);
          if (resp.ok) {
            updateOtaProgress(100);
            startRebootCountdown();
          } else {
            FW.showToast(resp.error || 'Upload failed', 'error');
            resetOtaUploadState();
          }
        } catch (e) {
          FW.showToast('Upload failed \u2014 invalid response', 'error');
          resetOtaUploadState();
        }
      } else {
        try {
          var errResp = JSON.parse(xhr.responseText);
          FW.showToast(errResp.error || 'Upload failed', 'error');
        } catch (e) {
          FW.showToast('Upload failed \u2014 status ' + xhr.status, 'error');
        }
        resetOtaUploadState();
      }
    };

    xhr.onerror = function() {
      if (btnUploadFirmware) btnUploadFirmware.disabled = false;
      FW.showToast('Connection lost during upload', 'error');
      resetOtaUploadState();
    };

    // Timeout for stalled uploads (2 minutes)
    xhr.timeout = 120000;
    xhr.ontimeout = function() {
      if (btnUploadFirmware) btnUploadFirmware.disabled = false;
      FW.showToast('Upload timed out \u2014 try again', 'error');
      resetOtaUploadState();
    };

    xhr.open('POST', '/api/ota/upload');
    xhr.send(formData);
  }

  function updateOtaProgress(pct) {
    otaProgressBar.style.width = pct + '%';
    otaProgress.setAttribute('aria-valuenow', String(pct));
    otaProgressText.textContent = pct + '%';
  }

  function startRebootCountdown() {
    otaProgress.style.display = 'none';
    otaReboot.style.display = '';
    var count = 3;
    otaRebootText.textContent = 'Rebooting in ' + count + '...';
    var countdownInterval = setInterval(function() {
      count--;
      if (count > 0) {
        otaRebootText.textContent = 'Rebooting in ' + count + '...';
      } else {
        clearInterval(countdownInterval);
        otaRebootText.textContent = 'Waiting for device...';
        startRebootPolling();
      }
    }, 1000);
  }

  function startRebootPolling() {
    var attempts = 0;
    var maxAttempts = 20;
    var done = false;

    function poll() {
      if (done) return;
      // Check timeout BEFORE issuing the next request so a slow final request
      // cannot trigger both the success toast and the timeout message.
      if (attempts >= maxAttempts) {
        done = true;
        otaRebootText.textContent = 'Device unreachable \u2014 try refreshing. The device may have changed IP address after reboot.';
        otaRebootText.style.color = getComputedStyle(document.documentElement).getPropertyValue('--warning').trim() || '#d29922';
        return;
      }
      attempts++;
      // Use recursive setTimeout (not setInterval) so the next poll only fires
      // AFTER the current request resolves — prevents concurrent in-flight
      // requests piling up against the resource-constrained ESP32.
      FW.get('/api/status?_=' + Date.now()).then(function(res) {
        if (done) return;  // timeout already fired; discard late response
        if (res.body && res.body.ok && res.body.data) {
          done = true;
          var newVersion = res.body.data.firmware_version || '';
          FW.showToast('Updated to v' + newVersion, 'success');
          fwVersion.textContent = 'Version: v' + newVersion;
          resetOtaUploadState();
          // Check rollback state after update
          if (res.body.data.rollback_detected && !res.body.data.rollback_acknowledged) {
            rollbackBanner.style.display = '';
          } else {
            rollbackBanner.style.display = 'none';
          }
        } else {
          // Partial or not-yet-ready response — retry after delay
          setTimeout(poll, 3000);
        }
      }).catch(function() {
        // Device not yet reachable — retry after delay
        if (!done) setTimeout(poll, 3000);
      });
    }

    poll();
  }

  // --- Settings Export (Story fn-1.6) ---
  var btnExportSettings = document.getElementById('btn-export-settings');
  if (btnExportSettings) {
    btnExportSettings.addEventListener('click', function() {
      // Direct navigation triggers browser download via Content-Disposition header
      window.location.href = '/api/settings/export';
    });
  }

  // --- Init ---
  var settingsPromise = loadSettings();

  // Load firmware status (version, rollback) on page load (Story fn-1.6)
  loadFirmwareStatus();

  // Load display modes on page load (Story dl-1.5)
  loadDisplayModes();

  // Load the logo list on page load
  loadLogoList();

  // Fetch /api/layout for initial canvas (best-effort)
  FW.get('/api/layout').then(function(res) {
    if (!res.body || !res.body.ok || !res.body.data || hardwareInputDirty) return;
    var layout = normalizeLayoutFromApi(res.body.data);
    if (!layout) return;

    suppressHardwareInputHandler = true;
    dTilesX.value = layout.hardware.tilesX;
    dTilesY.value = layout.hardware.tilesY;
    dTilePixels.value = layout.hardware.tilePixels;
    suppressHardwareInputHandler = false;
    setResolutionText({
      matrixWidth: layout.matrixWidth,
      matrixHeight: layout.matrixHeight
    });
    renderLayoutCanvas(layout);
    renderWiringCanvas();
  }).catch(function() {
    // Fallback: render from settings-loaded form values.
    settingsPromise.then(function() {
      if (!hardwareInputDirty) {
        updateHwResolution();
        updatePreviewFromInputs();
      }
    });
  });
})();
