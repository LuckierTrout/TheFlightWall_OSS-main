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
  var dirtySections = { display: false, timing: false, network: false, hardware: false, nightmode: false };
  // nmTimezoneDirty tracks explicit user changes to the timezone dropdown separately from
  // the nightmode dirty section, so posixToIana()'s 'UTC' fallback cannot silently overwrite
  // a custom POSIX string stored on the device when other Night Mode fields are changed.
  var nmTimezoneDirty = false;

  function markSectionDirty(section) {
    dirtySections[section] = true;
    applyBar.style.display = '';
  }

  function clearDirtyState() {
    dirtySections.display = false;
    dirtySections.timing = false;
    dirtySections.network = false;
    dirtySections.hardware = false;
    dirtySections.nightmode = false;
    nmTimezoneDirty = false;
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
    if (dirtySections.nightmode) {
      // Use cached DOM references (nmEnabled, nmDimStart, etc.) — consistent with all
      // other sections and avoids repeated getElementById lookups on the Apply hot path.
      payload.sched_enabled = nmEnabled.checked ? 1 : 0;
      payload.sched_dim_start = timeToMinutes(nmDimStart.value);
      payload.sched_dim_end = timeToMinutes(nmDimEnd.value);
      payload.sched_dim_brt = parseInt(nmDimBrt.value, 10);
      // Only include timezone when the user (or auto-suggest) explicitly changed the dropdown.
      // This prevents posixToIana()'s "UTC" fallback from silently overwriting a custom
      // POSIX string stored on the device when the user changes an unrelated Night Mode field.
      if (nmTimezoneDirty && nmTimezone && nmTimezone.value) {
        payload.timezone = TZ_MAP[nmTimezone.value] || 'UTC0';
      }
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

  // --- Night Mode time helpers (Story fn-2.3) ---
  function timeToMinutes(timeStr) {
    if (!timeStr || typeof timeStr !== 'string') return 0;
    var parts = timeStr.split(':');
    // Use < 2 (not !== 2) so that HH:MM:SS emitted by some browsers (e.g. when
    // step < 60 is inferred) still parses correctly — we only need hours and minutes.
    if (parts.length < 2) return 0;
    var h = parseInt(parts[0], 10);
    var m = parseInt(parts[1], 10);
    if (isNaN(h) || isNaN(m)) return 0;
    var total = h * 60 + m;
    if (total < 0 || total > 1439) return 0;
    return total;
  }

  function minutesToTime(mins) {
    var h = Math.floor(mins / 60);
    var m = mins % 60;
    return (h < 10 ? '0' : '') + h + ':' + (m < 10 ? '0' : '') + m;
  }

  function posixToIana(posix) {
    var keys = Object.keys(TZ_MAP);
    for (var i = 0; i < keys.length; i++) {
      if (TZ_MAP[keys[i]] === posix) return keys[i];
    }
    return 'UTC';
  }

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

      // Night Mode (Story fn-2.3)
      // Use typeof guards (not !==undefined) so that a null value from a malformed API
      // response does not reach parseInt() / minutesToTime() and produce NaN display.
      if (d.sched_enabled !== undefined && d.sched_enabled !== null) {
        nmEnabled.checked = (d.sched_enabled === 1 || d.sched_enabled === '1' || d.sched_enabled === true);
      }
      if (typeof d.sched_dim_start === 'number') {
        nmDimStart.value = minutesToTime(d.sched_dim_start);
      }
      if (typeof d.sched_dim_end === 'number') {
        nmDimEnd.value = minutesToTime(d.sched_dim_end);
      }
      if (typeof d.sched_dim_brt === 'number') {
        nmDimBrt.value = d.sched_dim_brt;
        nmDimBrtVal.textContent = d.sched_dim_brt;
      }
      if (d.timezone !== undefined) {
        var iana = posixToIana(d.timezone);
        nmTimezone.value = iana;
        // If device is still on default UTC0 and browser has a different zone, auto-suggest.
        // Also mark nmTimezoneDirty so the auto-suggested value is included in the payload
        // when the user confirms by clicking Apply.
        if (d.timezone === 'UTC0') {
          var detected = getTimezoneConfig();
          if (detected.iana !== 'UTC' && TZ_MAP[detected.iana]) {
            nmTimezone.value = detected.iana;
            nmTimezoneDirty = true;
            markSectionDirty('nightmode');
          }
        }
      }
      updateNmFieldState();

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

  // --- Night Mode card (Story fn-2.3) ---
  var nmEnabled = document.getElementById('nm-enabled');
  var nmDimStart = document.getElementById('nm-dim-start');
  var nmDimEnd = document.getElementById('nm-dim-end');
  var nmDimBrt = document.getElementById('nm-dim-brt');
  var nmDimBrtVal = document.getElementById('nm-dim-brt-val');
  var nmTimezone = document.getElementById('nm-timezone');
  var nmFields = document.getElementById('nm-fields');

  // Populate timezone dropdown from TZ_MAP
  (function() {
    var keys = Object.keys(TZ_MAP).sort();
    for (var i = 0; i < keys.length; i++) {
      var opt = document.createElement('option');
      opt.value = keys[i];
      opt.textContent = keys[i].replace(/_/g, ' ');
      nmTimezone.appendChild(opt);
    }
  })();

  function updateNmFieldState() {
    var on = nmEnabled.checked;
    nmDimStart.disabled = !on;
    nmDimEnd.disabled = !on;
    nmDimBrt.disabled = !on;
    nmTimezone.disabled = !on;
    nmFields.classList.toggle('nm-fields-disabled', !on);
  }

  nmEnabled.addEventListener('change', function() {
    updateNmFieldState();
    markSectionDirty('nightmode');
  });

  nmDimStart.addEventListener('change', function() { markSectionDirty('nightmode'); });
  nmDimEnd.addEventListener('change', function() { markSectionDirty('nightmode'); });
  nmDimBrt.addEventListener('input', function() {
    nmDimBrtVal.textContent = nmDimBrt.value;
  });
  nmDimBrt.addEventListener('change', function() { markSectionDirty('nightmode'); });
  nmTimezone.addEventListener('change', function() { nmTimezoneDirty = true; markSectionDirty('nightmode'); });

  updateNmFieldState();

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
    if (!calibrationActive) return Promise.resolve();
    calibrationActive = false;
    return FW.post('/api/calibration/stop', {}).catch(function() {});
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
    if (!positioningActive) return Promise.resolve();
    positioningActive = false;
    return FW.post('/api/positioning/stop', {}).catch(function() {});
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
    return Promise.all([
      stopCalibrationMode(),
      stopPositioningMode()
    ]);
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

  // --- Mode Picker (Story dl-1.5, ds-3.4) ---
  var modeStatusName = document.getElementById('modeStatusName');
  var modeStatusReason = document.getElementById('modeStatusReason');
  var modeCardsList = document.getElementById('modeCardsList');
  var modePicker = document.getElementById('mode-picker');
  var modeSwitchInFlight = false;
  var SWITCH_POLL_INTERVAL = 200;  // ms between GET polls during switch
  var SWITCH_POLL_TIMEOUT = 2000;  // max ms to poll before treating as failure
  var ERROR_DISMISS_MS = 5000;     // auto-dismiss scoped error after 5s
  // Keyboard support: Enter/Space on mode cards triggers switchMode (ds-3.5 AC #2)
  if (modeCardsList) {
    modeCardsList.addEventListener('keydown', function(e) {
      var card = e.target.closest('.mode-card');
      if (!card) return;
      if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault(); // Prevent Space page scroll
        var modeId = card.getAttribute('data-mode-id');
        if (modeId) switchMode(modeId);
      }
    });
  }

  // Exposed for ds-3.6 banner dismiss — moves focus to the mode picker section heading (AC #4)
  window.focusModePickerHeading = function() {
    var heading = document.getElementById('mode-picker-heading');
    if (heading && typeof heading.focus === 'function') {
      heading.focus({ preventScroll: true });
    }
  };

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
    if (modeStatusName && activeMode) {
      modeStatusName.textContent = activeMode.name;
    }
    if (modeStatusReason) {
      // Wrap reason in parens only when non-empty; avoids orphaned "()" on screen
      var reason = data.state_reason || '';
      modeStatusReason.textContent = reason ? '(' + reason + ')' : '';
    }
    // Update mode cards (active state + subtitle + aria-current + tabindex)
    if (modeCardsList) {
      var cards = modeCardsList.querySelectorAll('.mode-card');
      for (var j = 0; j < cards.length; j++) {
        var cardId = cards[j].getAttribute('data-mode-id');
        var subtitle = cards[j].querySelector('.mode-card-subtitle');
        if (cardId === data.active) {
          cards[j].classList.add('active');
          cards[j].setAttribute('aria-current', 'true');
          cards[j].setAttribute('tabindex', '-1');
          cards[j].removeAttribute('role'); // active cards are not actionable button targets
          if (subtitle) subtitle.textContent = data.state_reason || '';
        } else {
          cards[j].classList.remove('active');
          cards[j].removeAttribute('aria-current');
          cards[j].setAttribute('tabindex', '0');
          cards[j].setAttribute('role', 'button'); // idle cards are keyboard-activatable (AC #1)
          if (subtitle) subtitle.textContent = '';
        }
        cards[j].classList.remove('switching');
        cards[j].classList.remove('disabled');
        cards[j].removeAttribute('aria-busy');
        cards[j].removeAttribute('aria-disabled');
      }
    }
  }

  /** Find a card DOM element by mode id */
  function getModeCard(id) {
    if (!modeCardsList) return null;
    return modeCardsList.querySelector('.mode-card[data-mode-id="' + id + '"]');
  }

  /** Clear any existing scoped error alert from a card */
  function clearCardError(card) {
    if (!card) return;
    var existing = card.querySelector('.mode-card-alert');
    if (existing) existing.parentNode.removeChild(existing);
  }

  /** Show a scoped inline error on a card with auto-dismiss (AC #4) */
  function showCardError(card, message) {
    if (!card) return;
    clearCardError(card);
    var alert = document.createElement('div');
    alert.className = 'mode-card-alert';
    alert.setAttribute('role', 'alert');
    alert.setAttribute('aria-live', 'polite');
    alert.textContent = message;
    card.appendChild(alert);

    // Auto-dismiss after ERROR_DISMISS_MS or on next click/keydown in mode-picker
    var dismissed = false;
    function dismiss() {
      if (dismissed) return;
      dismissed = true;
      clearTimeout(timer);
      if (modePicker) modePicker.removeEventListener('click', dismiss);
      if (modePicker) modePicker.removeEventListener('keydown', dismiss);
      // Remove only this specific alert element — avoids clearing a newer error
      // injected by a subsequent showCardError call on the same card.
      if (alert.parentNode) alert.parentNode.removeChild(alert);
    }
    // Stop click propagation on the alert itself so that clicking the error message
    // does not bubble to the parent .mode-card and accidentally re-trigger switchMode.
    alert.addEventListener('click', function(e) {
      e.stopPropagation();
      dismiss();
    });
    var timer = setTimeout(dismiss, ERROR_DISMISS_MS);
    if (modePicker) {
      modePicker.addEventListener('click', dismiss, { once: true });
      modePicker.addEventListener('keydown', dismiss, { once: true });
    }
  }

  /** Clear switching/disabled states on all cards and restore interactivity */
  function clearSwitchingStates() {
    if (!modeCardsList) return;
    var cards = modeCardsList.querySelectorAll('.mode-card');
    for (var i = 0; i < cards.length; i++) {
      cards[i].classList.remove('switching');
      cards[i].classList.remove('disabled');
      cards[i].removeAttribute('aria-busy');
      cards[i].removeAttribute('aria-disabled');
      // Restore tabindex and role: active card gets tabindex=-1 (no role); idle gets tabindex=0 + role=button
      var isActive = cards[i].classList.contains('active');
      cards[i].setAttribute('tabindex', isActive ? '-1' : '0');
      if (isActive) {
        cards[i].removeAttribute('role');
      } else {
        cards[i].setAttribute('role', 'button');
      }
      var sub = cards[i].querySelector('.mode-card-subtitle');
      if (sub && sub.textContent === 'Switching...') {
        // Restore the subtitle that was shown before the switch was initiated;
        // if the device goes offline mid-failure the original state_reason is preserved.
        sub.textContent = sub.getAttribute('data-prev-text') || '';
        sub.removeAttribute('data-prev-text');
      }
    }
  }

  function buildSchematic(zoneLayout, modeName) {
    var wrap = document.createElement('div');
    wrap.className = 'mode-schematic';
    wrap.setAttribute('role', 'img');
    wrap.setAttribute('aria-label', 'Schematic preview of zone layout for ' + modeName);
    if (!zoneLayout || !zoneLayout.length) return wrap;
    for (var i = 0; i < zoneLayout.length; i++) {
      var z = zoneLayout[i];
      if (!z.relW || !z.relH) continue;
      var cell = document.createElement('div');
      cell.className = 'mode-schematic-zone';
      cell.setAttribute('aria-hidden', 'true');
      // Guard relX/relY: undefined (e.g. JSON omitting 0-value fields) → 0
      var rx = (z.relX !== undefined && z.relX !== null) ? z.relX : 0;
      var ry = (z.relY !== undefined && z.relY !== null) ? z.relY : 0;
      var cs = Math.max(1, rx + 1);
      var ce = Math.min(101, cs + z.relW);
      var rs = Math.max(1, ry + 1);
      var re = Math.min(101, rs + z.relH);
      cell.style.gridColumn = cs + ' / ' + ce;
      cell.style.gridRow = rs + ' / ' + re;
      var label = z.label || '';
      if (label.length > 8) label = label.substring(0, 7) + '\u2026';
      cell.textContent = label;
      wrap.appendChild(cell);
    }
    return wrap;
  }

  /**
   * Build a settings panel for a mode card (Story dl-5.1, AC #3)
   * @param {object} mode - Mode object from GET /api/display/modes
   * @returns {HTMLElement|null} - Settings panel element or null if no settings
   *
   * Control types:
   * - enum → <select> (options from enumOptions split on comma, trimmed)
   * - uint8/uint16 → number input with min/max
   * - bool → checkbox (value 0/1 for JSON)
   */
  function buildModeSettingsPanel(mode) {
    // AC #3: Empty settings / null → no settings block
    if (!mode.settings || !mode.settings.length) return null;

    var panel = document.createElement('div');
    panel.className = 'mode-settings-panel';

    // Stop click propagation so clicking settings doesn't trigger mode switch
    panel.addEventListener('click', function(e) {
      e.stopPropagation();
    });

    // Track setting values for Apply button
    var settingValues = {};

    for (var i = 0; i < mode.settings.length; i++) {
      var setting = mode.settings[i];
      var row = document.createElement('div');
      row.className = 'mode-setting-row';

      var label = document.createElement('label');
      label.className = 'mode-setting-label';
      label.textContent = setting.label;

      var control;

      if (setting.type === 'enum') {
        // AC #3: enum → <select> with options from enumOptions split on comma
        control = document.createElement('select');
        control.className = 'mode-setting-input mode-setting-select';
        if (setting.enumOptions) {
          var options = setting.enumOptions.split(',');
          for (var j = 0; j < options.length; j++) {
            var opt = document.createElement('option');
            opt.value = j;  // Numeric value for JSON
            opt.textContent = options[j].trim();
            if (j === setting.value) opt.selected = true;
            control.appendChild(opt);
          }
        }
        settingValues[setting.key] = setting.value;
        control.addEventListener('change', (function(key) {
          return function(e) {
            settingValues[key] = parseInt(e.target.value, 10);
          };
        })(setting.key));

      } else if (setting.type === 'bool') {
        // AC #3: bool → checkbox (values 0/1)
        control = document.createElement('input');
        control.type = 'checkbox';
        control.className = 'mode-setting-checkbox';
        control.checked = setting.value === 1;
        settingValues[setting.key] = setting.value;
        control.addEventListener('change', (function(key) {
          return function(e) {
            settingValues[key] = e.target.checked ? 1 : 0;
          };
        })(setting.key));

      } else {
        // AC #3: uint8/uint16 → number input with min/max
        control = document.createElement('input');
        control.type = 'number';
        control.className = 'mode-setting-input mode-setting-number';
        control.min = setting.min;
        control.max = setting.max;
        control.value = setting.value;
        settingValues[setting.key] = setting.value;
        control.addEventListener('change', (function(key, min, max) {
          return function(e) {
            var val = parseInt(e.target.value, 10);
            if (isNaN(val)) val = min;
            if (val < min) val = min;
            if (val > max) val = max;
            e.target.value = val;
            settingValues[key] = val;
          };
        })(setting.key, setting.min, setting.max));
      }

      // Associate label with control
      var controlId = 'mode-setting-' + mode.id + '-' + setting.key;
      control.id = controlId;
      label.setAttribute('for', controlId);

      row.appendChild(label);
      row.appendChild(control);
      panel.appendChild(row);
    }

    // Apply button (AC #4: POST { mode, settings })
    var actionsRow = document.createElement('div');
    actionsRow.className = 'mode-setting-actions';

    var applyBtn = document.createElement('button');
    applyBtn.type = 'button';
    applyBtn.className = 'mode-setting-apply';
    applyBtn.textContent = 'Apply';

    applyBtn.addEventListener('click', function() {
      applyBtn.disabled = true;
      applyBtn.textContent = 'Applying...';

      // AC #4: POST /api/display/mode with { mode, settings }
      FW.post('/api/display/mode', {
        mode: mode.id,
        settings: settingValues
      }).then(function(res) {
        applyBtn.disabled = false;
        applyBtn.textContent = 'Apply';
        if (res.body.ok) {
          FW.showToast('Settings applied', 'success');
          // Reload modes to refresh current values
          loadDisplayModes();
        } else {
          FW.showToast(res.body.error || 'Failed to apply settings', 'error');
        }
      }).catch(function() {
        applyBtn.disabled = false;
        applyBtn.textContent = 'Apply';
        FW.showToast('Network error', 'error');
      });
    });

    actionsRow.appendChild(applyBtn);
    panel.appendChild(actionsRow);

    return panel;
  }

  function renderModeCards(data) {
    if (!modeCardsList || !data.modes) return;
    modeCardsList.innerHTML = '';
    for (var i = 0; i < data.modes.length; i++) {
      var mode = data.modes[i];
      var isActive = mode.id === data.active;
      var card = document.createElement('div');
      card.className = 'mode-card' + (isActive ? ' active' : '');
      card.setAttribute('data-mode-id', mode.id);
      // Active card: tabindex="-1" (not in tab order as actionable); idle: tabindex="0"
      card.setAttribute('tabindex', isActive ? '-1' : '0');
      if (isActive) {
        card.setAttribute('aria-current', 'true');
      } else {
        // Idle cards expose role="button" so screen readers announce them as interactive (AC #1)
        card.setAttribute('role', 'button');
      }
      var headerRow = document.createElement('div');
      headerRow.className = 'mode-card-header';
      var nameEl = document.createElement('span');
      nameEl.className = 'mode-card-name';
      nameEl.textContent = mode.name;
      headerRow.appendChild(nameEl);
      if (isActive) {
        var dotWrap = document.createElement('span');
        dotWrap.className = 'mode-active-indicator';
        var dot = document.createElement('span');
        dot.className = 'mode-active-dot';
        dotWrap.appendChild(dot);
        dotWrap.appendChild(document.createTextNode(' Active'));
        headerRow.appendChild(dotWrap);
      }
      card.appendChild(headerRow);
      var subtitleEl = document.createElement('div');
      subtitleEl.className = 'mode-card-subtitle';
      subtitleEl.textContent = isActive ? (data.state_reason || '') : '';
      card.appendChild(subtitleEl);
      card.appendChild(buildSchematic(mode.zone_layout, mode.name));

      // Story dl-5.1, AC #3: Per-mode settings panel
      var settingsPanel = buildModeSettingsPanel(mode);
      if (settingsPanel) {
        card.appendChild(settingsPanel);
      }

      card.addEventListener('click', (function(modeId) {
        return function() {
          switchMode(modeId);
        };
      })(mode.id));
      modeCardsList.appendChild(card);
    }
  }

  function syncPreemptedTestPattern(data, modeId, showToast) {
    if (!data || (data.preempted !== 'calibration' && data.preempted !== 'positioning')) {
      return false;
    }

    // Only one test pattern can be active at a time; after a preempting mode switch,
    // both client-side mirrors should be considered inactive.
    calibrationActive = false;
    positioningActive = false;

    if (!showToast) return true;

    var preemptedLabel = modeId;
    if (data.modes && data.modes.length) {
      for (var i = 0; i < data.modes.length; i++) {
        if (data.modes[i].id === modeId) {
          preemptedLabel = data.modes[i].name || modeId;
          break;
        }
      }
    }
    var srcLabel = (data.preempted === 'calibration') ? 'Calibration' : 'Positioning';
    FW.showToast(srcLabel + ' stopped. Switched to ' + preemptedLabel, 'info');
    return true;
  }

  /**
   * switchMode — full orchestration (ds-3.4)
   * 1. Track previousActiveId, apply switching + disabled states
   * 2. POST /api/display/mode { mode: id }
   * 3. Poll GET /api/display/modes until settled or timeout
   * 4. Finalize UI: active/idle cards, focus management, error handling
   */
  function switchMode(modeId) {
    if (modeSwitchInFlight) return;
    modeSwitchInFlight = true;

    // Track previous active mode before switch
    var previousActiveId = null;
    var cards = modeCardsList ? modeCardsList.querySelectorAll('.mode-card') : [];
    for (var i = 0; i < cards.length; i++) {
      if (cards[i].classList.contains('active')) {
        previousActiveId = cards[i].getAttribute('data-mode-id');
      }
      // NOTE: Do NOT short-circuit for the already-active mode. The orchestrator
      // may be in IDLE_FALLBACK or SCHEDULED state; the user clicking the active
      // card must trigger a POST so the orchestrator transitions to MANUAL.
      // (See ANTIPATTERNS ds-3.1: "Orchestrator bypassed when user re-selects
      // the currently active mode".)
    }

    // Apply switching state to target, disabled to all others (AC #1, #2)
    for (var j = 0; j < cards.length; j++) {
      var cid = cards[j].getAttribute('data-mode-id');
      if (cid === modeId) {
        cards[j].classList.add('switching');
        cards[j].classList.remove('active');
        cards[j].setAttribute('aria-busy', 'true');
        cards[j].setAttribute('tabindex', '-1');
        var sub = cards[j].querySelector('.mode-card-subtitle');
        if (sub) {
          // Store original subtitle so clearSwitchingStates can restore it on failure
          sub.setAttribute('data-prev-text', sub.textContent);
          sub.textContent = 'Switching...';
        }
      } else {
        cards[j].classList.add('disabled');
        // Do NOT remove .active here — per AC #3 the previously active card retains
        // its active chrome until the firmware confirms success (UX-DR1).
        cards[j].setAttribute('aria-disabled', 'true');
        cards[j].setAttribute('tabindex', '-1');
      }
    }

    // BF-1: POST mode switch request directly. Firmware now auto-yields any active
    // calibration/positioning test pattern (main.cpp displayTask), so the client
    // no longer needs to stop test patterns before issuing the switch.
    FW.post('/api/display/mode', { mode: modeId }).then(function(res) {
      // HTTP error from firmware (AC #8)
      if (!res.body || !res.body.ok) {
        clearSwitchingStates();
        var errMsg = (res.body && res.body.error) ? res.body.error : 'Mode switch failed';
        // Keep modeSwitchInFlight=true until reload completes so a concurrent switch
        // cannot race with the DOM rebuild (renderModeCards wipes switching/disabled classes).
        loadDisplayModes().then(function() {
          modeSwitchInFlight = false;
          var errCard = getModeCard(modeId);
          showCardError(errCard, errMsg);
          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
        }, function() {
          modeSwitchInFlight = false;
          var errCard = getModeCard(modeId);
          showCardError(errCard, errMsg);
          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
        });
        return;
      }

      // Check response for immediate registry error
      var respData = res.body.data || res.body;
      if (respData.registry_error) {
        clearSwitchingStates();
        var heapMsg = (respData.registry_error.indexOf('HEAP') !== -1 || respData.registry_error.indexOf('heap') !== -1)
          ? 'Not enough memory to activate this mode. Current mode restored.'
          : respData.registry_error;
        // Keep modeSwitchInFlight=true until reload completes to prevent concurrent switch races.
        loadDisplayModes().then(function() {
          modeSwitchInFlight = false;
          var errCard = getModeCard(modeId);
          showCardError(errCard, heapMsg);
          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
        }, function() {
          modeSwitchInFlight = false;
          var errCard = getModeCard(modeId);
          showCardError(errCard, heapMsg);
          if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
        });
        return;
      }

      // Even when POST already reports the target mode as active, poll the terminal
      // status once so the dashboard consumes the normalized envelope (`preempted`,
      // failed state, current mode list) from GET /api/display/modes.
      var switchState = respData.switch_state || 'idle';
      var immediateTerminalIdle = (switchState === 'idle' && respData.active === modeId);

      // Start polling GET /api/display/modes until settled (AC #7)
      // Use attempt counter instead of Date.now() diff so background-tab
      // setTimeout throttling (≥1000ms) does not falsely trigger timeout.
      var pollAttempts = 0;
      var maxPollAttempts = Math.ceil(SWITCH_POLL_TIMEOUT / SWITCH_POLL_INTERVAL);
      function pollSwitch() {
        if (pollAttempts++ >= maxPollAttempts) {
          // Timeout — treat as failure
          clearSwitchingStates();
          // Keep modeSwitchInFlight=true until reload completes to prevent concurrent switch races.
          loadDisplayModes().then(function() {
            modeSwitchInFlight = false;
            var errCard = getModeCard(modeId);
            showCardError(errCard, 'Mode switch timed out. Current mode restored.');
            if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
          }, function() {
            modeSwitchInFlight = false;
            var errCard = getModeCard(modeId);
            showCardError(errCard, 'Mode switch timed out. Current mode restored.');
            if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
          });
          return;
        }
        FW.get('/api/display/modes').then(function(pollRes) {
          if (!pollRes.body || !pollRes.body.ok || !pollRes.body.data) {
            // Poll failed — retry on next interval
            setTimeout(pollSwitch, SWITCH_POLL_INTERVAL);
            return;
          }
          var d = pollRes.body.data;
          var ss = d.switch_state || 'idle';

          // Check for registry error during switch
          if (d.registry_error) {
            modeSwitchInFlight = false;
            clearSwitchingStates();
            var msg = (d.registry_error.indexOf('HEAP') !== -1 || d.registry_error.indexOf('heap') !== -1)
              ? 'Not enough memory to activate this mode. Current mode restored.'
              : d.registry_error;
            // Render cards first (rebuilds DOM), then attach error to freshly rendered card
            renderModeCards(d);
            updateModeStatus(d);
            var errCard = getModeCard(modeId);
            showCardError(errCard, msg);
            // Restore keyboard focus to the affected card so users can read the inline error (ds-3.5)
            if (errCard && typeof errCard.focus === 'function') errCard.focus({ preventScroll: true });
            return;
          }

          // BF-1 AC #5: stall watchdog fired — surface as a clear failure rather than
          // continuing to poll until the SWITCH_POLL_TIMEOUT.
          if (ss === 'failed') {
            modeSwitchInFlight = false;
            clearSwitchingStates();
            syncPreemptedTestPattern(d, modeId, false);
            renderModeCards(d);
            updateModeStatus(d);
            var stallCard = getModeCard(modeId);
            var stallMsg = (d.switch_error_code === 'REQUESTED_STALL')
              ? 'Mode switch stalled — display task did not respond. Current mode kept.'
              : 'Mode switch failed. Current mode restored.';
            showCardError(stallCard, stallMsg);
            if (stallCard && typeof stallCard.focus === 'function') stallCard.focus({ preventScroll: true });
            return;
          }

          // Success: switch_state is idle AND active matches target
          if (ss === 'idle' && d.active === modeId) {
            renderModeCards(d);
            updateModeStatus(d);
            syncPreemptedTestPattern(d, modeId, true);
            finalizeModeSwitch(modeId, previousActiveId, true); // cards already rendered
            return;
          }

          // switch_state idle but active doesn't match — failure
          if (ss === 'idle' && d.active !== modeId) {
            modeSwitchInFlight = false;
            clearSwitchingStates();
            syncPreemptedTestPattern(d, modeId, false);
            // Render cards first (rebuilds DOM), then attach error to freshly rendered card
            renderModeCards(d);
            updateModeStatus(d);
            var errCard2 = getModeCard(modeId);
            showCardError(errCard2, 'Mode switch failed. Current mode restored.');
            // Restore keyboard focus to the affected card so users can read the inline error (ds-3.5)
            if (errCard2 && typeof errCard2.focus === 'function') errCard2.focus({ preventScroll: true });
            return;
          }

          // Still switching — poll again
          setTimeout(pollSwitch, SWITCH_POLL_INTERVAL);
        }).catch(function() {
          // Network error during poll — retry
          setTimeout(pollSwitch, SWITCH_POLL_INTERVAL);
        });
      }
      if (immediateTerminalIdle) {
        pollSwitch();
      } else {
        setTimeout(pollSwitch, SWITCH_POLL_INTERVAL);
      }

    }).catch(function() {
      // Network failure (AC #6) — clear states, reload.
      // loadDisplayModes() shows its own connectivity toast on failure, so we do
      // not emit a separate toast here to avoid double-toasting when the device
      // is unreachable.
      modeSwitchInFlight = false;
      clearSwitchingStates();
      loadDisplayModes();
    });
  }

  /** Finalize a successful mode switch — update UI, move focus (AC #3, UX-DR6).
   *  @param {boolean} [skipReload] Pass true when cards were already re-rendered
   *    by the caller (poll-success path) to avoid a redundant GET /api/display/modes. */
  function finalizeModeSwitch(newActiveId, previousActiveId, skipReload) {
    function doFocus() {
      // Focus the newly activated card so keyboard users land on the confirmed
      // active selection. (Focusing previousActiveId was a logic error — the
      // previous card is no longer the relevant target after a successful switch.)
      if (newActiveId) {
        var newCard = getModeCard(newActiveId);
        if (newCard && typeof newCard.focus === 'function') {
          newCard.focus({ preventScroll: true });
        }
      }
    }
    if (skipReload) {
      // DOM already rebuilt by the poll success path — safe to unlock immediately.
      modeSwitchInFlight = false;
      doFocus();
    } else {
      // Keep modeSwitchInFlight=true until the DOM rebuild completes so a
      // concurrent switch cannot race with the loadDisplayModes() GET response
      // and overwrite the new switching/disabled visual state mid-render.
      clearSwitchingStates();
      loadDisplayModes().then(function() {
        modeSwitchInFlight = false;
        doFocus();
      }, function() {
        modeSwitchInFlight = false;
      });
    }
  }

  // --- Upgrade Notification Banner (Story ds-3.6) ---

  function dismissAndClearUpgrade() {
    localStorage.setItem('mode_notif_seen', 'true');
    FW.post('/api/display/notification/dismiss', {}).catch(function() {});
    var host = document.getElementById('mode-upgrade-banner-host');
    if (host) {
      var banner = host.querySelector('.banner-info');
      if (banner) banner.parentNode.removeChild(banner);
    }
  }

  function maybeShowModeUpgradeBanner(data) {
    if (!data || !data.upgrade_notification) return;
    // Check both current key and legacy key (D7 documented 'flightwall_mode_notif_seen')
    if (localStorage.getItem('mode_notif_seen') === 'true') return;
    if (localStorage.getItem('flightwall_mode_notif_seen') === 'true') return;
    var host = document.getElementById('mode-upgrade-banner-host');
    if (!host) return;
    // Clear any stale banner before inserting a fresh one
    host.innerHTML = '';

    var banner = document.createElement('div');
    banner.className = 'banner-info';
    banner.setAttribute('role', 'region');
    banner.setAttribute('aria-labelledby', 'mode-upgrade-banner-title');

    var title = document.createElement('span');
    title.id = 'mode-upgrade-banner-title';
    title.textContent = 'New display modes available';

    var actions = document.createElement('div');
    actions.className = 'banner-actions';

    // Primary action — Try Live Flight Card (AC #6, #7)
    // Delegates to switchMode() to reuse ds-3.4 polling, in-flight locking,
    // error handling, and focus management (Synthesis fix: avoid duplication)
    var tryBtn = document.createElement('button');
    tryBtn.type = 'button';
    tryBtn.className = 'btn-primary-inline';
    tryBtn.textContent = 'Try Live Flight Card';
    tryBtn.addEventListener('click', function() {
      var hasLiveFlight = data.modes && data.modes.some(function(m) { return m.id === 'live_flight'; });
      if (!hasLiveFlight) {
        // Mode not registered in last GET — dismiss + toast (AC #7)
        FW.showToast('Mode not available', 'error');
        dismissAndClearUpgrade();
        return;
      }
      // Dismiss banner immediately before initiating switch (AC #4, #7)
      // The switchMode() orchestrator handles all error feedback via mode picker
      dismissAndClearUpgrade();
      // Delegate to switchMode() — reuses ds-3.4 polling, timeout handling,
      // in-flight guard, registry_error detection, and focus choreography
      switchMode('live_flight');
    });

    // Secondary action — Browse Modes (AC #8)
    var browseBtn = document.createElement('button');
    browseBtn.type = 'button';
    browseBtn.className = 'link-btn';
    browseBtn.textContent = 'Browse Modes';
    browseBtn.addEventListener('click', function() {
      var picker = document.getElementById('mode-picker');
      if (picker) picker.scrollIntoView({ behavior: 'smooth', block: 'start' });
      dismissAndClearUpgrade();
    });

    // Dismiss control — × (AC #4, #5)
    var dismissBtn = document.createElement('button');
    dismissBtn.type = 'button';
    dismissBtn.className = 'dismiss-btn';
    dismissBtn.setAttribute('aria-label', 'Dismiss new display modes notification');
    dismissBtn.innerHTML = '&times;';
    dismissBtn.addEventListener('click', function() {
      dismissAndClearUpgrade();
      // Move focus to #mode-picker-heading per AC #5 / UX-DR6
      var heading = document.getElementById('mode-picker-heading');
      if (heading) heading.focus();
    });

    actions.appendChild(tryBtn);
    actions.appendChild(browseBtn);
    banner.appendChild(title);
    banner.appendChild(actions);
    banner.appendChild(dismissBtn);
    host.appendChild(banner);
  }

  function loadDisplayModes() {
    return FW.get('/api/display/modes').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) {
        FW.showToast('Failed to load display modes', 'error');
        return null;
      }
      var data = res.body.data;
      renderModeCards(data);
      updateModeStatus(data);
      maybeShowModeUpgradeBanner(data); // AC #1 — show banner on first eligible load
      return data; // expose data so callers can reuse modes list (e.g. loadScheduleRules)
    }).catch(function() {
      FW.showToast('Cannot reach device to load display modes. Check connection.', 'error');
      return null;
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

  // --- OTA Update Check (Story dl-6.2) ---
  var btnCheckUpdate = document.getElementById('btn-check-update');
  var otaUpdateBanner = document.getElementById('ota-update-banner');
  var otaUpdateText = document.getElementById('ota-update-text');
  var btnToggleNotes = document.getElementById('btn-toggle-notes');
  var otaReleaseNotes = document.getElementById('ota-release-notes');
  var otaNotesContent = document.getElementById('ota-notes-content');
  var btnUpdateNow = document.getElementById('btn-update-now');
  var otaUptodate = document.getElementById('ota-uptodate');
  var otaUptodateText = document.getElementById('ota-uptodate-text');

  function hideOtaResults() {
    otaUpdateBanner.style.display = 'none';
    otaUptodate.style.display = 'none';
    otaReleaseNotes.style.display = 'none';
    if (btnToggleNotes) {
      btnToggleNotes.setAttribute('aria-expanded', 'false');
      btnToggleNotes.textContent = 'View Release Notes';
    }
  }

  if (btnCheckUpdate) {
    btnCheckUpdate.addEventListener('click', function() {
      btnCheckUpdate.disabled = true;
      btnCheckUpdate.textContent = 'Checking\u2026';
      hideOtaResults();

      FW.get('/api/ota/check').then(function(res) {
        btnCheckUpdate.disabled = false;
        btnCheckUpdate.textContent = 'Check for Updates';

        if (!res.body || !res.body.ok || !res.body.data) {
          FW.showToast('Update check failed', 'error');
          return;
        }

        var d = res.body.data;

        if (d.error) {
          // AC #7: check failed — show error toast, no banner
          FW.showToast(d.error, 'error');
          return;
        }

        if (d.available) {
          // AC #5: update available — show banner
          otaUpdateText.textContent = 'Firmware ' + d.version + ' available \u2014 you\u2019re running ' + d.current_version;
          if (d.release_notes) {
            otaNotesContent.textContent = d.release_notes;
            btnToggleNotes.style.display = '';
          } else {
            btnToggleNotes.style.display = 'none';
          }
          otaUpdateBanner.style.display = '';
          otaUptodate.style.display = 'none';
        } else {
          // AC #6: up to date
          otaUptodateText.textContent = 'Firmware is up to date (v' + d.current_version + ')';
          otaUptodate.style.display = '';
          otaUpdateBanner.style.display = 'none';
        }
      }).catch(function() {
        btnCheckUpdate.disabled = false;
        btnCheckUpdate.textContent = 'Check for Updates';
        FW.showToast('Cannot reach device', 'error');
      });
    });
  }

  // Release notes expand/collapse (AC #8)
  if (btnToggleNotes) {
    btnToggleNotes.addEventListener('click', function() {
      var expanded = btnToggleNotes.getAttribute('aria-expanded') === 'true';
      btnToggleNotes.setAttribute('aria-expanded', String(!expanded));
      otaReleaseNotes.style.display = expanded ? 'none' : '';
      btnToggleNotes.textContent = expanded ? 'View Release Notes' : 'Hide Release Notes';
    });
    btnToggleNotes.addEventListener('keydown', function(e) {
      if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault();
        btnToggleNotes.click();
      }
    });
  }

  // --- OTA Pull Download (Story dl-7.3, AC #4–#9) ---
  var otaPullProgress = document.getElementById('ota-pull-progress');
  var otaPullStatus = document.getElementById('ota-pull-status');
  var otaPullBarWrap = document.getElementById('ota-pull-bar-wrap');
  var otaPullBar = document.getElementById('ota-pull-bar');
  var otaPullPct = document.getElementById('ota-pull-pct');
  var otaPullError = document.getElementById('ota-pull-error');
  var btnOtaRetry = document.getElementById('btn-ota-retry');
  var OTA_POLL_INTERVAL = 500; // AC #4: 500ms poll interval
  var otaPollTimer = null;

  function resetOtaPullState() {
    if (otaPollTimer) { clearInterval(otaPollTimer); otaPollTimer = null; }
    otaPullProgress.style.display = 'none';
    otaPullBar.style.width = '0%';
    otaPullBarWrap.setAttribute('aria-valuenow', '0');
    otaPullBarWrap.classList.remove('indeterminate');
    otaPullPct.textContent = '0%';
    otaPullStatus.textContent = '';
    otaPullError.style.display = 'none';
    otaPullError.textContent = '';
    btnOtaRetry.style.display = 'none';
  }

  function startOtaPull() {
    resetOtaPullState();
    otaPullProgress.style.display = '';
    otaPullStatus.textContent = 'Starting download\u2026';
    otaUpdateBanner.style.display = 'none';
    if (btnUpdateNow) btnUpdateNow.disabled = true;

    FW.post('/api/ota/pull', {}).then(function(res) {
      if (!res.body || !res.body.ok) {
        var errMsg = (res.body && res.body.error) ? res.body.error : 'Could not start download';
        otaPullStatus.textContent = '';
        otaPullError.textContent = errMsg;
        otaPullError.style.display = '';
        if (btnUpdateNow) btnUpdateNow.disabled = false;
        return;
      }
      // Download started — begin polling (AC #4)
      startOtaPollLoop();
    }).catch(function() {
      otaPullStatus.textContent = '';
      otaPullError.textContent = 'Cannot reach device \u2014 check connection';
      otaPullError.style.display = '';
      if (btnUpdateNow) btnUpdateNow.disabled = false;
    });
  }

  function startOtaPollLoop() {
    if (otaPollTimer) clearTimeout(otaPollTimer);
    otaPollTimer = null;
    pollOtaStatus(); // Start first poll immediately
  }

  function pollOtaStatus() {
    FW.get('/api/ota/status').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) return;
      var d = res.body.data;

      if (d.state === 'downloading') {
        // AC #5: progress bar 0–100
        var pct = (d.progress != null) ? d.progress : 0;
        otaPullBar.style.width = pct + '%';
        otaPullBarWrap.setAttribute('aria-valuenow', String(pct));
        otaPullBarWrap.classList.remove('indeterminate');
        otaPullPct.textContent = pct + '%';
        otaPullStatus.textContent = 'Downloading firmware\u2026';
        otaPullError.style.display = 'none';
        btnOtaRetry.style.display = 'none';
        // Schedule next poll (recursive setTimeout pattern)
        otaPollTimer = setTimeout(pollOtaStatus, OTA_POLL_INTERVAL);

      } else if (d.state === 'verifying') {
        // AC #6: indeterminate verification phase
        otaPullBarWrap.classList.add('indeterminate');
        otaPullPct.textContent = 'Verifying\u2026';
        otaPullStatus.textContent = 'Verifying firmware integrity\u2026';
        otaPullError.style.display = 'none';
        btnOtaRetry.style.display = 'none';
        // Continue polling during verification
        otaPollTimer = setTimeout(pollOtaStatus, OTA_POLL_INTERVAL);

      } else if (d.state === 'rebooting') {
        // AC #7: stop polling, show rebooting message
        if (otaPollTimer) { clearTimeout(otaPollTimer); otaPollTimer = null; }
        otaPullBarWrap.classList.remove('indeterminate');
        otaPullBar.style.width = '100%';
        otaPullPct.textContent = '100%';
        otaPullStatus.textContent = 'Rebooting\u2026';
        // Grace period then poll for device return (reuse existing reboot polling)
        setTimeout(function() {
          otaPullStatus.textContent = 'Waiting for device\u2026';
          startOtaPullRebootPolling();
        }, 3000);

      } else if (d.state === 'error') {
        // AC #8: error with optional retry
        if (otaPollTimer) { clearTimeout(otaPollTimer); otaPollTimer = null; }
        otaPullBarWrap.classList.remove('indeterminate');
        otaPullStatus.textContent = 'Update failed';
        otaPullError.textContent = d.error || 'Download failed';
        otaPullError.style.display = '';
        // Show retry if retriable (AC #8)
        if (d.retriable) {
          btnOtaRetry.style.display = '';
        }
        if (btnUpdateNow) btnUpdateNow.disabled = false;

      } else if (d.state === 'idle' || d.state === 'available') {
        // Terminal idle after success or no-op — stop polling
        if (otaPollTimer) { clearTimeout(otaPollTimer); otaPollTimer = null; }
        if (btnUpdateNow) btnUpdateNow.disabled = false;
      }
    }).catch(function() {
      // Fetch failure during poll — may be device rebooting (AC #7)
      if (otaPollTimer) { clearTimeout(otaPollTimer); otaPollTimer = null; }
      otaPullStatus.textContent = 'Device disconnected \u2014 may be rebooting\u2026';
      setTimeout(function() {
        startOtaPullRebootPolling();
      }, 2000);
    });
  }

  function startOtaPullRebootPolling() {
    var attempts = 0;
    var maxAttempts = 20;
    var done = false;
    function poll() {
      if (done) return;
      if (attempts >= maxAttempts) {
        done = true;
        otaPullStatus.textContent = 'Device unreachable \u2014 try refreshing.';
        otaPullStatus.style.color = getComputedStyle(document.documentElement).getPropertyValue('--warning').trim() || '#d29922';
        if (btnUpdateNow) btnUpdateNow.disabled = false;
        return;
      }
      attempts++;
      FW.get('/api/status?_=' + Date.now()).then(function(res) {
        if (done) return;
        if (res.body && res.body.ok && res.body.data) {
          done = true;
          // AC #9: new firmware version, hide update banner
          var newVersion = res.body.data.firmware_version || '';
          FW.showToast('Updated to v' + newVersion, 'success');
          fwVersion.textContent = 'Version: v' + newVersion;
          resetOtaPullState();
          hideOtaResults();
          if (btnUpdateNow) btnUpdateNow.disabled = false;
        } else {
          setTimeout(poll, 3000);
        }
      }).catch(function() {
        if (!done) setTimeout(poll, 3000);
      });
    }
    poll();
  }

  // "Update Now" triggers OTA pull download (AC #4)
  if (btnUpdateNow) {
    btnUpdateNow.addEventListener('click', function() {
      startOtaPull();
    });
  }

  // Retry button (AC #8)
  if (btnOtaRetry) {
    btnOtaRetry.addEventListener('click', function() {
      startOtaPull();
    });
  }

  // --- Settings Export (Story fn-1.6) ---
  var btnExportSettings = document.getElementById('btn-export-settings');
  if (btnExportSettings) {
    btnExportSettings.addEventListener('click', function() {
      // Direct navigation triggers browser download via Content-Disposition header
      window.location.href = '/api/settings/export';
    });
  }

  // --- Night Mode status polling (Story fn-2.3) ---
  var nmNtpBadge = document.getElementById('nm-ntp-badge');
  var nmSchedStatus = document.getElementById('nm-sched-status');

  function updateNmStatus() {
    // Guard: abort silently if the Night Mode card is not present in this HTML build.
    if (!nmNtpBadge || !nmSchedStatus) return;
    // Use recursive setTimeout (not setInterval) so the next poll only fires
    // AFTER the current request resolves — prevents concurrent in-flight
    // requests piling up against the resource-constrained ESP32 web stack.
    FW.get('/api/status').then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) {
        // Silently reset indicators; background poll should not toast on transient failures.
        nmNtpBadge.textContent = 'NTP: --';
        nmNtpBadge.className = 'nm-ntp-badge';
        nmSchedStatus.textContent = '';
        setTimeout(updateNmStatus, 10000);
        return;
      }
      var s = res.body.data;

      // NTP badge
      if (s.ntp_synced) {
        nmNtpBadge.textContent = 'NTP Synced';
        nmNtpBadge.className = 'nm-ntp-badge nm-ntp-ok';
      } else {
        nmNtpBadge.textContent = 'NTP Not Synced';
        nmNtpBadge.className = 'nm-ntp-badge nm-ntp-warn';
      }

      // Schedule status
      if (!s.ntp_synced) {
        nmSchedStatus.textContent = 'Schedule paused \u2014 waiting for NTP sync';
        nmSchedStatus.className = 'nm-sched-status nm-inactive';
      } else if (s.schedule_dimming) {
        nmSchedStatus.textContent = 'Currently dimming';
        nmSchedStatus.className = 'nm-sched-status nm-dimming';
      } else if (s.schedule_active) {
        nmSchedStatus.textContent = 'Schedule active (not in dim window)';
        nmSchedStatus.className = 'nm-sched-status nm-active';
      } else {
        nmSchedStatus.textContent = 'Schedule inactive';
        nmSchedStatus.className = 'nm-sched-status nm-inactive';
      }

      setTimeout(updateNmStatus, 10000);
    }).catch(function() {
      nmNtpBadge.textContent = 'NTP: --';
      nmNtpBadge.className = 'nm-ntp-badge';
      nmSchedStatus.textContent = '';
      setTimeout(updateNmStatus, 10000);
    });
  }

  // Kick off initial status poll; subsequent polls are scheduled recursively.
  updateNmStatus();

  // --- Mode Schedule card (Story dl-4.2) ---
  var schedRulesList = document.getElementById('sched-rules-list');
  var schedEmptyState = document.getElementById('sched-empty-state');
  var schedOrchBadge = document.getElementById('sched-orch-badge');
  var btnAddRule = document.getElementById('btn-add-rule');
  var schedModes = [];      // populated from /api/display/modes
  var schedRulesData = [];  // current schedule rules from API
  var schedActiveIdx = -1;
  var schedOrchState = 'manual';

  /** Load schedule rules. Pass pre-loaded modesData to skip duplicate GET /api/display/modes. */
  function loadScheduleRules(modesData) {
    // Reuse caller-supplied modes list if available; otherwise fetch independently
    var modesStep;
    if (modesData && modesData.modes) {
      schedModes = modesData.modes;
      modesStep = Promise.resolve();
    } else {
      modesStep = FW.get('/api/display/modes').then(function(modesRes) {
        if (modesRes.body && modesRes.body.ok && modesRes.body.data && modesRes.body.data.modes) {
          schedModes = modesRes.body.data.modes;
        }
      });
    }
    return modesStep.then(function() {
      return FW.get('/api/schedule');
    }).then(function(res) {
      if (!res.body || !res.body.ok || !res.body.data) {
        FW.showToast('Failed to load schedule', 'error');
        return;
      }
      var d = res.body.data;
      schedRulesData = d.rules || [];
      schedActiveIdx = typeof d.active_rule_index === 'number' ? d.active_rule_index : -1;
      schedOrchState = d.orchestrator_state || 'manual';
      renderScheduleUI();
    }).catch(function() {
      FW.showToast('Cannot load schedule', 'error');
    });
  }

  function renderScheduleUI() {
    // Update orchestrator badge
    if (schedOrchBadge) {
      if (schedOrchState === 'scheduled') {
        schedOrchBadge.textContent = 'Scheduled';
        schedOrchBadge.className = 'sched-orch-badge sched-orch-scheduled';
      } else {
        schedOrchBadge.textContent = 'State: ' + schedOrchState.replace(/_/g, ' ');
        schedOrchBadge.className = 'sched-orch-badge sched-orch-manual';
      }
    }

    // Clear existing rows (keep empty state element)
    var rows = schedRulesList.querySelectorAll('.sched-rule-row');
    for (var i = 0; i < rows.length; i++) rows[i].remove();

    if (schedRulesData.length === 0) {
      if (schedEmptyState) schedEmptyState.style.display = '';
      return;
    }
    if (schedEmptyState) schedEmptyState.style.display = 'none';

    for (var r = 0; r < schedRulesData.length; r++) {
      schedRulesList.appendChild(buildRuleRow(r, schedRulesData[r]));
    }
  }

  function buildRuleRow(idx, rule) {
    var row = document.createElement('div');
    row.className = 'sched-rule-row';
    if (schedOrchState === 'scheduled' && idx === schedActiveIdx) {
      row.className += ' sched-rule-active';
    }

    // Start time
    var times = document.createElement('div');
    times.className = 'sched-rule-times';
    var startInput = document.createElement('input');
    startInput.type = 'time';
    startInput.value = minutesToTime(rule.start_min);
    startInput.setAttribute('data-idx', idx);
    startInput.setAttribute('data-field', 'start_min');
    startInput.addEventListener('change', onScheduleRuleChange);
    var sep = document.createElement('span');
    sep.textContent = '\u2013';
    var endInput = document.createElement('input');
    endInput.type = 'time';
    endInput.value = minutesToTime(rule.end_min);
    endInput.setAttribute('data-idx', idx);
    endInput.setAttribute('data-field', 'end_min');
    endInput.addEventListener('change', onScheduleRuleChange);
    times.appendChild(startInput);
    times.appendChild(sep);
    times.appendChild(endInput);

    // Mode dropdown
    var modeWrap = document.createElement('div');
    modeWrap.className = 'sched-rule-mode';
    var modeSelect = document.createElement('select');
    for (var m = 0; m < schedModes.length; m++) {
      var opt = document.createElement('option');
      opt.value = schedModes[m].id;
      opt.textContent = schedModes[m].name;
      if (schedModes[m].id === rule.mode_id) opt.selected = true;
      modeSelect.appendChild(opt);
    }
    modeSelect.setAttribute('data-idx', idx);
    modeSelect.setAttribute('data-field', 'mode_id');
    modeSelect.addEventListener('change', onScheduleRuleChange);
    modeWrap.appendChild(modeSelect);

    // Enabled toggle
    var toggleWrap = document.createElement('div');
    toggleWrap.className = 'sched-rule-toggle';
    var enabledCb = document.createElement('input');
    enabledCb.type = 'checkbox';
    enabledCb.checked = rule.enabled === 1;
    enabledCb.setAttribute('data-idx', idx);
    enabledCb.setAttribute('data-field', 'enabled');
    enabledCb.addEventListener('change', onScheduleRuleChange);
    var enabledLabel = document.createElement('span');
    enabledLabel.textContent = 'On';
    enabledLabel.style.fontSize = '0.8125rem';
    toggleWrap.appendChild(enabledCb);
    toggleWrap.appendChild(enabledLabel);

    // Delete button
    var delBtn = document.createElement('button');
    delBtn.className = 'sched-rule-del';
    delBtn.textContent = '\u00d7';
    delBtn.title = 'Delete rule';
    delBtn.setAttribute('data-idx', idx);
    delBtn.addEventListener('click', onScheduleRuleDelete);

    row.appendChild(times);
    row.appendChild(modeWrap);
    row.appendChild(toggleWrap);
    row.appendChild(delBtn);
    return row;
  }

  function onScheduleRuleChange(e) {
    var idx = parseInt(e.target.getAttribute('data-idx'), 10);
    var field = e.target.getAttribute('data-field');
    if (idx < 0 || idx >= schedRulesData.length) return;

    if (field === 'start_min') {
      schedRulesData[idx].start_min = timeToMinutes(e.target.value);
    } else if (field === 'end_min') {
      schedRulesData[idx].end_min = timeToMinutes(e.target.value);
    } else if (field === 'mode_id') {
      schedRulesData[idx].mode_id = e.target.value;
    } else if (field === 'enabled') {
      schedRulesData[idx].enabled = e.target.checked ? 1 : 0;
    }
    saveScheduleRules();
  }

  function onScheduleRuleDelete(e) {
    var idx = parseInt(e.target.getAttribute('data-idx'), 10);
    if (idx < 0 || idx >= schedRulesData.length) return;
    schedRulesData.splice(idx, 1);
    saveScheduleRules();
  }

  function saveScheduleRules() {
    // Build compacted rules payload (AC #5: no gaps, full replacement)
    var payload = { rules: [] };
    for (var i = 0; i < schedRulesData.length; i++) {
      var r = schedRulesData[i];
      payload.rules.push({
        start_min: r.start_min,
        end_min: r.end_min,
        mode_id: r.mode_id,
        enabled: r.enabled
      });
    }
    FW.post('/api/schedule', payload).then(function(res) {
      if (res.body && res.body.ok) {
        FW.showToast('Schedule saved', 'success');
        // Re-fetch to get updated active_rule_index / orchestrator_state
        loadScheduleRules();
      } else {
        FW.showToast(res.body.error || 'Failed to save schedule', 'error');
      }
    }).catch(function() {
      FW.showToast('Network error saving schedule', 'error');
    });
  }

  if (btnAddRule) {
    btnAddRule.addEventListener('click', function() {
      if (schedRulesData.length >= 8) {
        FW.showToast('Maximum 8 rules allowed', 'error');
        return;
      }
      // Default new rule: 08:00–17:00, first available mode, enabled
      var defaultModeId = schedModes.length > 0 ? schedModes[0].id : 'classic_card';
      schedRulesData.push({
        start_min: 480,
        end_min: 1020,
        mode_id: defaultModeId,
        enabled: 1
      });
      saveScheduleRules();
    });
  }

  // --- Init ---
  var settingsPromise = loadSettings();

  // Load firmware status (version, rollback) on page load (Story fn-1.6)
  loadFirmwareStatus();

  // Load display modes on page load (Story dl-1.5)
  loadDisplayModes();

  // Load schedule rules on page load (Story dl-4.2)
  loadScheduleRules();

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
