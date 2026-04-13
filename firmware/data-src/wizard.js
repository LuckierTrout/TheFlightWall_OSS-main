/* FlightWall Setup Wizard — Step navigation, WiFi scan polling, validation, state preservation */

/**
 * IANA-to-POSIX timezone mapping (Story fn-2.1).
 * Shared with dashboard.js — used for settings import timezone resolution.
 * Browser auto-detects IANA via Intl API; ESP32 needs POSIX string for configTzTime().
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
 * @returns {{ iana: string, posix: string }}
 */
function getTimezoneConfig() {
  var iana = "UTC";
  try {
    iana = Intl.DateTimeFormat().resolvedOptions().timeZone || "UTC";
  } catch (e) { /* Intl not available */ }
  var posix = TZ_MAP[iana] || "UTC0";
  return { iana: iana, posix: posix };
}

(function() {
  'use strict';

  var TOTAL_STEPS = 5;
  var currentStep = 1;

  // In-memory wizard state using exact firmware config key names
  var state = {
    wifi_ssid: '',
    wifi_password: '',
    os_client_id: '',
    os_client_sec: '',
    aeroapi_key: '',
    center_lat: '',
    center_lon: '',
    radius_km: '',
    tiles_x: '',
    tiles_y: '',
    tile_pixels: '',
    display_pin: ''
  };

  // Valid GPIO pins for display_pin (matches ConfigManager validation)
  var VALID_PINS = [0,2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33];

  // Keys that map to wizard form fields (12 keys)
  var WIZARD_KEYS = [
    'wifi_ssid', 'wifi_password',
    'os_client_id', 'os_client_sec', 'aeroapi_key',
    'center_lat', 'center_lon', 'radius_km',
    'tiles_x', 'tiles_y', 'tile_pixels', 'display_pin'
  ];

  // Non-wizard config keys preserved in POST payload
  var KNOWN_EXTRA_KEYS = [
    'brightness', 'text_color_r', 'text_color_g', 'text_color_b',
    'origin_corner', 'scan_dir', 'zigzag',
    'zone_logo_pct', 'zone_split_pct', 'zone_layout',
    'fetch_interval', 'display_cycle',
    'timezone', 'sched_enabled', 'sched_dim_start', 'sched_dim_end', 'sched_dim_brt'
  ];

  // Extra keys from import (not shown in wizard, but sent with POST)
  var importedExtras = {};

  // DOM references — Import zone
  var importZone = document.getElementById('settings-import-zone');
  var importFileInput = document.getElementById('import-file-input');
  var importStatus = document.getElementById('import-status');

  // DOM references — Steps 1-2
  var progress = document.getElementById('progress');
  var btnBack = document.getElementById('btn-back');
  var btnNext = document.getElementById('btn-next');
  var scanStatus = document.getElementById('scan-status');
  var scanResults = document.getElementById('scan-results');
  var btnManual = document.getElementById('btn-manual');
  var wifiSsid = document.getElementById('wifi-ssid');
  var wifiPass = document.getElementById('wifi-pass');
  var wifiErr = document.getElementById('wifi-err');
  var osClientId = document.getElementById('os-client-id');
  var osClientSec = document.getElementById('os-client-sec');
  var aeroKey = document.getElementById('aeroapi-key');
  var apiErr = document.getElementById('api-err');

  // DOM references — Step 3
  var btnGeolocate = document.getElementById('btn-geolocate');
  var geoStatus = document.getElementById('geo-status');
  var centerLat = document.getElementById('center-lat');
  var centerLon = document.getElementById('center-lon');
  var radiusKm = document.getElementById('radius-km');
  var locErr = document.getElementById('loc-err');

  // DOM references — Step 4
  var tilesX = document.getElementById('tiles-x');
  var tilesY = document.getElementById('tiles-y');
  var tilePixels = document.getElementById('tile-pixels');
  var displayPin = document.getElementById('display-pin');
  var hwErr = document.getElementById('hw-err');
  var resolutionText = document.getElementById('resolution-text');

  // DOM references — Step 5
  var reviewSections = document.getElementById('review-sections');

  // --- Bootstrap: hydrate defaults from firmware ---
  function hydrateDefaults() {
    FW.get('/api/settings').then(function(res) {
      if (!res.body.ok || !res.body.data) return;
      var d = res.body.data;
      // Only hydrate Step 3-4 keys if state is still empty (user hasn't typed yet)
      if (!state.center_lat && d.center_lat !== undefined) state.center_lat = String(d.center_lat);
      if (!state.center_lon && d.center_lon !== undefined) state.center_lon = String(d.center_lon);
      if (!state.radius_km && d.radius_km !== undefined) state.radius_km = String(d.radius_km);
      if (!state.tiles_x && d.tiles_x !== undefined) state.tiles_x = String(d.tiles_x);
      if (!state.tiles_y && d.tiles_y !== undefined) state.tiles_y = String(d.tiles_y);
      if (!state.tile_pixels && d.tile_pixels !== undefined) state.tile_pixels = String(d.tile_pixels);
      if (!state.display_pin && d.display_pin !== undefined) state.display_pin = String(d.display_pin);
    }).catch(function() {
      // Settings endpoint unavailable — user will fill manually
    });
  }

  // --- WiFi Scan ---
  var scanTimer = null;
  var scanStartTime = 0;
  var SCAN_TIMEOUT_MS = 5000;
  var SCAN_POLL_MS = 800;
  var manualMode = false;

  function startScan() {
    clearTimeout(scanTimer);
    manualMode = false;
    scanStatus.style.display = '';
    scanResults.style.display = 'none';
    scanStatus.querySelector('.scan-msg').textContent = 'Scanning for networks...';
    var spinner = scanStatus.querySelector('.spinner');
    if (spinner) spinner.style.display = '';
    scanStartTime = Date.now();
    pollScan();
  }

  function pollScan() {
    FW.get('/api/wifi/scan').then(function(res) {
      if (manualMode) return;
      var d = res.body;
      if (d.ok && !d.scanning && d.data && d.data.length > 0) {
        showScanResults(d.data);
        return;
      }
      if (d.ok && !d.scanning) {
        showScanEmpty();
        return;
      }
      if (Date.now() - scanStartTime >= SCAN_TIMEOUT_MS) {
        showScanEmpty();
        return;
      }
      scanTimer = setTimeout(pollScan, SCAN_POLL_MS);
    }).catch(function() {
      if (manualMode) return;
      if (Date.now() - scanStartTime >= SCAN_TIMEOUT_MS) {
        showScanEmpty();
        return;
      }
      scanTimer = setTimeout(pollScan, SCAN_POLL_MS);
    });
  }

  function showScanResults(networks) {
    clearTimeout(scanTimer);
    scanStatus.style.display = 'none';
    scanResults.style.display = '';
    scanResults.innerHTML = '';
    // Deduplicate by SSID, keep strongest RSSI
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
      row.addEventListener('click', function() { selectSsid(n.ssid); });
      scanResults.appendChild(row);
    });
  }

  function showScanEmpty() {
    clearTimeout(scanTimer);
    scanStatus.querySelector('.scan-msg').textContent = 'No networks found - enter manually';
    var spinner = scanStatus.querySelector('.spinner');
    if (spinner) spinner.style.display = 'none';
    scanResults.style.display = 'none';
  }

  function selectSsid(ssid) {
    wifiSsid.value = ssid;
    state.wifi_ssid = ssid;
    wifiSsid.focus();
  }

  function enterManual() {
    manualMode = true;
    clearTimeout(scanTimer);
    scanStatus.style.display = 'none';
    scanResults.style.display = 'none';
    wifiSsid.focus();
  }

  function rssiLabel(rssi) {
    if (rssi >= -50) return 'Excellent';
    if (rssi >= -60) return 'Good';
    if (rssi >= -70) return 'Fair';
    return 'Weak';
  }

  function escHtml(s) {
    var el = document.createElement('span');
    el.textContent = s;
    return el.innerHTML;
  }

  // --- Geolocation (Step 3) ---
  function requestGeolocation() {
    if (!navigator.geolocation) {
      showGeoStatus('Geolocation is not available in this browser. Please enter coordinates manually.', true);
      return;
    }
    btnGeolocate.disabled = true;
    showGeoStatus('Requesting location...', false);
    navigator.geolocation.getCurrentPosition(
      function(pos) {
        centerLat.value = pos.coords.latitude.toFixed(6);
        centerLon.value = pos.coords.longitude.toFixed(6);
        state.center_lat = centerLat.value;
        state.center_lon = centerLon.value;
        showGeoStatus('Location detected.', false);
        btnGeolocate.disabled = false;
      },
      function(err) {
        var msg = 'Could not get location. Please enter coordinates manually.';
        if (err.code === 1) msg = 'Location permission denied. Please enter coordinates manually.';
        if (err.code === 2) msg = 'Location unavailable. Please enter coordinates manually.';
        showGeoStatus(msg, true);
        btnGeolocate.disabled = false;
      },
      { enableHighAccuracy: false, timeout: 10000, maximumAge: 300000 }
    );
  }

  function showGeoStatus(msg, isError) {
    geoStatus.textContent = msg;
    geoStatus.style.display = '';
    geoStatus.className = 'geo-status' + (isError ? ' geo-error' : ' geo-ok');
  }

  // --- Hardware resolution text (Step 4) ---
  function updateResolution() {
    var tx = parseStrictPositiveInt(tilesX.value.trim());
    var ty = parseStrictPositiveInt(tilesY.value.trim());
    var tp = parseStrictPositiveInt(tilePixels.value.trim());
    if (tx !== null && ty !== null && tp !== null) {
      resolutionText.textContent = 'Your display: ' + (tx * tp) + ' x ' + (ty * tp) + ' pixels';
    } else {
      resolutionText.textContent = '';
    }
  }

  // --- Review (Step 5) ---
  function buildReview() {
    reviewSections.innerHTML = '';
    var sections = [
      { title: 'WiFi', step: 1, items: [
        { label: 'Network', value: state.wifi_ssid },
        { label: 'Password', value: state.wifi_password ? '\u2022'.repeat(Math.min(state.wifi_password.length, 12)) : '' }
      ]},
      { title: 'API Keys', step: 2, items: [
        { label: 'OpenSky Client ID', value: maskKey(state.os_client_id) },
        { label: 'OpenSky Secret', value: maskKey(state.os_client_sec) },
        { label: 'AeroAPI Key', value: maskKey(state.aeroapi_key) }
      ]},
      { title: 'Location', step: 3, items: [
        { label: 'Latitude', value: state.center_lat },
        { label: 'Longitude', value: state.center_lon },
        { label: 'Radius', value: state.radius_km + ' km' }
      ]},
      { title: 'Hardware', step: 4, items: [
        { label: 'Display', value: state.tiles_x + ' x ' + state.tiles_y + ' tiles (' + (parseInt(state.tiles_x,10)*parseInt(state.tile_pixels,10)) + ' x ' + (parseInt(state.tiles_y,10)*parseInt(state.tile_pixels,10)) + ' px)' },
        { label: 'Pixels per tile', value: state.tile_pixels },
        { label: 'Data pin', value: 'GPIO ' + state.display_pin }
      ]}
    ];

    sections.forEach(function(sec) {
      var card = document.createElement('div');
      card.className = 'review-card';
      card.addEventListener('click', function() {
        showStep(sec.step);
      });
      var header = '<div class="review-card-header"><span>' + escHtml(sec.title) + '</span><span class="review-edit">Edit</span></div>';
      var body = sec.items.map(function(it) {
        return '<div class="review-item"><span class="review-label">' + escHtml(it.label) + '</span><span class="review-value">' + escHtml(it.value) + '</span></div>';
      }).join('');
      card.innerHTML = header + body;
      reviewSections.appendChild(card);
    });
  }

  function maskKey(val) {
    if (!val || val.length < 6) return val || '';
    return val.substring(0, 3) + '\u2022'.repeat(Math.min(val.length - 6, 8)) + val.substring(val.length - 3);
  }

  function parseStrictNumber(value) {
    if (!/^[+-]?(?:\d+\.?\d*|\.\d+)$/.test(value)) return null;
    var parsed = Number(value);
    return isNaN(parsed) ? null : parsed;
  }

  function parseStrictPositiveInt(value) {
    if (!/^\d+$/.test(value)) return null;
    var parsed = Number(value);
    if (!isFinite(parsed) || parsed <= 0 || Math.floor(parsed) !== parsed) return null;
    return parsed;
  }

  // --- Navigation ---
  function showStep(n) {
    currentStep = n;
    progress.textContent = 'Step ' + n + ' of ' + TOTAL_STEPS;
    for (var i = 1; i <= TOTAL_STEPS; i++) {
      var sec = document.getElementById('step-' + i);
      if (sec) sec.classList.toggle('active', i === n);
    }
    btnBack.style.visibility = n === 1 ? 'hidden' : 'visible';

    if (n < TOTAL_STEPS) {
      btnNext.textContent = 'Next \u2192';
      btnNext.className = 'nav-btn nav-btn-primary';
    } else {
      btnNext.textContent = 'Save & Connect';
      btnNext.className = 'nav-btn nav-btn-save';
    }

    // Rehydrate fields from state when entering a step
    if (n === 1) {
      wifiSsid.value = state.wifi_ssid;
      wifiPass.value = state.wifi_password;
      wifiErr.style.display = 'none';
      clearInputErrors([wifiSsid, wifiPass]);
    }
    if (n === 2) {
      osClientId.value = state.os_client_id;
      osClientSec.value = state.os_client_sec;
      aeroKey.value = state.aeroapi_key;
      apiErr.style.display = 'none';
      clearInputErrors([osClientId, osClientSec, aeroKey]);
    }
    if (n === 3) {
      centerLat.value = state.center_lat;
      centerLon.value = state.center_lon;
      radiusKm.value = state.radius_km;
      locErr.style.display = 'none';
      clearInputErrors([centerLat, centerLon, radiusKm]);
    }
    if (n === 4) {
      tilesX.value = state.tiles_x;
      tilesY.value = state.tiles_y;
      tilePixels.value = state.tile_pixels;
      displayPin.value = state.display_pin;
      hwErr.style.display = 'none';
      clearInputErrors([tilesX, tilesY, tilePixels, displayPin]);
      updateResolution();
    }
    if (n === 5) {
      buildReview();
    }
  }

  function saveCurrentStepState() {
    if (currentStep === 1) {
      state.wifi_ssid = wifiSsid.value.trim();
      state.wifi_password = wifiPass.value;
    } else if (currentStep === 2) {
      state.os_client_id = osClientId.value.trim();
      state.os_client_sec = osClientSec.value.trim();
      state.aeroapi_key = aeroKey.value.trim();
    } else if (currentStep === 3) {
      state.center_lat = centerLat.value.trim();
      state.center_lon = centerLon.value.trim();
      state.radius_km = radiusKm.value.trim();
    } else if (currentStep === 4) {
      state.tiles_x = tilesX.value.trim();
      state.tiles_y = tilesY.value.trim();
      state.tile_pixels = tilePixels.value.trim();
      state.display_pin = displayPin.value.trim();
    }
  }

  function validateStep(n) {
    if (n === 1) {
      state.wifi_ssid = wifiSsid.value.trim();
      state.wifi_password = wifiPass.value;
      var missing = [];
      if (!state.wifi_ssid) missing.push(wifiSsid);
      if (!state.wifi_password) missing.push(wifiPass);
      if (missing.length > 0) {
        wifiErr.style.display = '';
        markInputErrors(missing);
        return false;
      }
      wifiErr.style.display = 'none';
      clearInputErrors([wifiSsid, wifiPass]);
      return true;
    }
    if (n === 2) {
      state.os_client_id = osClientId.value.trim();
      state.os_client_sec = osClientSec.value.trim();
      state.aeroapi_key = aeroKey.value.trim();
      var missing2 = [];
      if (!state.os_client_id) missing2.push(osClientId);
      if (!state.os_client_sec) missing2.push(osClientSec);
      if (!state.aeroapi_key) missing2.push(aeroKey);
      if (missing2.length > 0) {
        apiErr.style.display = '';
        markInputErrors(missing2);
        return false;
      }
      apiErr.style.display = 'none';
      clearInputErrors([osClientId, osClientSec, aeroKey]);
      return true;
    }
    if (n === 3) {
      state.center_lat = centerLat.value.trim();
      state.center_lon = centerLon.value.trim();
      state.radius_km = radiusKm.value.trim();
      var errs = [];
      var lat = parseStrictNumber(state.center_lat);
      var lon = parseStrictNumber(state.center_lon);
      var rad = parseStrictNumber(state.radius_km);
      if (lat === null || lat < -90 || lat > 90) errs.push(centerLat);
      if (lon === null || lon < -180 || lon > 180) errs.push(centerLon);
      if (rad === null || rad <= 0) errs.push(radiusKm);
      if (errs.length > 0) {
        locErr.textContent = 'Latitude (-90 to 90), longitude (-180 to 180), and a positive radius are required.';
        locErr.style.display = '';
        markInputErrors(errs);
        return false;
      }
      locErr.style.display = 'none';
      clearInputErrors([centerLat, centerLon, radiusKm]);
      return true;
    }
    if (n === 4) {
      state.tiles_x = tilesX.value.trim();
      state.tiles_y = tilesY.value.trim();
      state.tile_pixels = tilePixels.value.trim();
      state.display_pin = displayPin.value.trim();
      var errs4 = [];
      var tx = parseStrictPositiveInt(state.tiles_x);
      var ty = parseStrictPositiveInt(state.tiles_y);
      var tp = parseStrictPositiveInt(state.tile_pixels);
      var dp = parseStrictPositiveInt(state.display_pin);
      if (tx === null) errs4.push(tilesX);
      if (ty === null) errs4.push(tilesY);
      if (tp === null) errs4.push(tilePixels);
      if (dp === null || VALID_PINS.indexOf(dp) === -1) errs4.push(displayPin);
      if (errs4.length > 0) {
        var msg = 'All fields must be positive integers.';
        if (errs4.length === 1 && errs4[0] === displayPin) {
          msg = 'Invalid GPIO pin. Supported: ' + VALID_PINS.join(', ') + '.';
        }
        hwErr.textContent = msg;
        hwErr.style.display = '';
        markInputErrors(errs4);
        return false;
      }
      hwErr.style.display = 'none';
      clearInputErrors([tilesX, tilesY, tilePixels, displayPin]);
      return true;
    }
    // Step 5: no field validation — review only
    return true;
  }

  function markInputErrors(inputs) {
    inputs.forEach(function(el) { el.classList.add('input-error'); });
  }

  function clearInputErrors(inputs) {
    inputs.forEach(function(el) { el.classList.remove('input-error'); });
  }

  // --- Save & Connect (Step 5 final action) ---
  function saveAndConnect() {
    var rebootRequested = false;

    btnNext.disabled = true;
    btnNext.textContent = 'Saving...';

    var payload = {
      wifi_ssid: state.wifi_ssid,
      wifi_password: state.wifi_password,
      os_client_id: state.os_client_id,
      os_client_sec: state.os_client_sec,
      aeroapi_key: state.aeroapi_key,
      center_lat: Number(state.center_lat),
      center_lon: Number(state.center_lon),
      radius_km: Number(state.radius_km),
      tiles_x: Number(state.tiles_x),
      tiles_y: Number(state.tiles_y),
      tile_pixels: Number(state.tile_pixels),
      display_pin: Number(state.display_pin)
    };

    // Merge imported extra keys (brightness, timing, schedule, etc.)
    var key;
    for (key in importedExtras) {
      if (importedExtras.hasOwnProperty(key)) {
        payload[key] = importedExtras[key];
      }
    }

    FW.post('/api/settings', payload).then(function(res) {
      if (!res.body.ok) {
        throw new Error('Save failed: ' + (res.body.error || 'Unknown error'));
      }
      // Settings saved — trigger reboot
      btnNext.textContent = 'Rebooting...';
      rebootRequested = true;
      return FW.post('/api/reboot', {});
    }).then(function(res) {
      if (!res || !res.body || !res.body.ok) {
        throw new Error('Reboot failed: ' + ((res && res.body && res.body.error) || 'Unknown error'));
      }
      showHandoff();
    }).catch(function(err) {
      if (rebootRequested) {
        // Network loss after requesting reboot is expected because the device is leaving AP mode.
        showHandoff();
        return;
      }
      btnNext.disabled = false;
      btnNext.textContent = 'Save & Connect';
      showSaveError(err && err.message ? err.message : 'Save failed: Network error');
    });
  }

  function showSaveError(msg) {
    // Show error inline in the review section
    var errEl = document.getElementById('save-err');
    if (!errEl) {
      errEl = document.createElement('p');
      errEl.id = 'save-err';
      errEl.className = 'field-error';
      reviewSections.parentNode.appendChild(errEl);
    }
    errEl.textContent = msg;
    errEl.style.display = '';
  }

  function showHandoff() {
    // Replace wizard content with handoff message
    var wizard = document.querySelector('.wizard');
    wizard.innerHTML =
      '<div class="handoff">' +
        '<h1>Configuration Saved</h1>' +
        '<p>Your FlightWall is rebooting and connecting to <strong>' + escHtml(state.wifi_ssid) + '</strong>.</p>' +
        '<p>Look at the LED wall for progress.</p>' +
        '<p class="handoff-note">This page will no longer update because the device is leaving setup mode.</p>' +
      '</div>';
  }

  // --- Event Listeners ---
  btnNext.addEventListener('click', function() {
    if (!validateStep(currentStep)) return;
    saveCurrentStepState();
    if (currentStep < TOTAL_STEPS) {
      showStep(currentStep + 1);
    } else {
      saveAndConnect();
    }
  });

  btnBack.addEventListener('click', function() {
    saveCurrentStepState();
    if (currentStep > 1) {
      showStep(currentStep - 1);
    }
  });

  btnManual.addEventListener('click', enterManual);
  btnGeolocate.addEventListener('click', requestGeolocation);

  // Live resolution update on Step 4 inputs
  tilesX.addEventListener('input', updateResolution);
  tilesY.addEventListener('input', updateResolution);
  tilePixels.addEventListener('input', updateResolution);

  // --- Settings Import ---
  function processImportedSettings(text) {
    // Flush any in-progress DOM input to state FIRST — imported values then win over typed values
    saveCurrentStepState();
    // Reset UI and extras at the start of every attempt — covers re-import and error paths (AC #4)
    importStatus.style.display = 'none';
    importStatus.textContent = '';
    importedExtras = {};
    var parsed;
    try {
      parsed = JSON.parse(text);
    } catch (e) {
      FW.showToast('Could not read settings file \u2014 invalid format', 'error');
      return;
    }
    if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) {
      FW.showToast('Could not read settings file \u2014 invalid format', 'error');
      return;
    }
    var count = 0;
    var i, key, val;
    for (i = 0; i < WIZARD_KEYS.length; i++) {
      key = WIZARD_KEYS[i];
      if (Object.prototype.hasOwnProperty.call(parsed, key)) {
        val = parsed[key];
        // Skip null and non-primitive values to avoid "null" / "[object Object]" in state
        if (val !== null && typeof val !== 'object') {
          state[key] = String(val);
          count++;
        }
      }
    }
    for (i = 0; i < KNOWN_EXTRA_KEYS.length; i++) {
      key = KNOWN_EXTRA_KEYS[i];
      if (Object.prototype.hasOwnProperty.call(parsed, key)) {
        val = parsed[key];
        if (val !== null && typeof val !== 'object') {
          importedExtras[key] = val;
          count++;
        }
      }
    }
    if (count === 0) {
      FW.showToast('No recognized settings found in file', 'warning');
      return;
    }
    FW.showToast('Imported ' + count + ' settings', 'success');
    importStatus.textContent = count + ' settings imported';
    importStatus.style.display = '';
    showStep(currentStep);
  }

  function handleImportFile(file) {
    if (!file) return;
    // Reset state and UI on every new import attempt — covers early-return paths that never
    // reach processImportedSettings() (file-too-large, FileReader error), preventing stale
    // extras from a prior successful import from surviving into the POST payload (AC #4).
    importedExtras = {};
    importStatus.style.display = 'none';
    importStatus.textContent = '';
    if (file.size > 1024 * 1024) {
      FW.showToast('Settings file too large \u2014 maximum 1\u00a0MB', 'error');
      return;
    }
    var reader = new FileReader();
    reader.onload = function(e) { processImportedSettings(e.target.result); };
    reader.onerror = function() { FW.showToast('Could not read settings file \u2014 invalid format', 'error'); };
    reader.readAsText(file);
  }

  // Click / keyboard handler for import zone
  importZone.addEventListener('click', function() { importFileInput.click(); });
  importZone.addEventListener('keydown', function(e) {
    if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); importFileInput.click(); }
  });
  importFileInput.addEventListener('change', function() {
    if (importFileInput.files && importFileInput.files[0]) {
      handleImportFile(importFileInput.files[0]);
    }
    importFileInput.value = '';
  });

  // Drag-and-drop for import zone
  importZone.addEventListener('dragenter', function(e) { e.preventDefault(); importZone.classList.add('drag-over'); });
  importZone.addEventListener('dragover', function(e) { e.preventDefault(); });
  importZone.addEventListener('dragleave', function(e) {
    if (!importZone.contains(e.relatedTarget)) importZone.classList.remove('drag-over');
  });
  importZone.addEventListener('drop', function(e) {
    e.preventDefault();
    importZone.classList.remove('drag-over');
    if (e.dataTransfer.files && e.dataTransfer.files[0]) {
      handleImportFile(e.dataTransfer.files[0]);
    }
  });

  // --- Init ---
  hydrateDefaults();
  showStep(1);
  startScan();
})();
