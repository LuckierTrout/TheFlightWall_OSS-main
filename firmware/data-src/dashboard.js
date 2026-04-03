/* FlightWall Dashboard — Display, Timing, Network/API, and Hardware settings */

/**
 * Shared zone calculation algorithm (must match C++ LayoutEngine::compute exactly).
 * @param {number} tilesX - Number of horizontal tiles
 * @param {number} tilesY - Number of vertical tiles
 * @param {number} tilePixels - Pixels per tile edge
 * @returns {object} { matrixWidth, matrixHeight, mode, logoZone, flightZone, telemetryZone, valid }
 */
function computeLayout(tilesX, tilesY, tilePixels) {
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

  var halfH = Math.floor(mh / 2);

  return {
    matrixWidth: mw,
    matrixHeight: mh,
    mode: mode,
    logoZone:      { x: 0,  y: 0,     w: mh,      h: mh },
    flightZone:    { x: mh, y: 0,     w: mw - mh, h: halfH },
    telemetryZone: { x: mh, y: halfH, w: mw - mh, h: mh - halfH },
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
  var btnApplyNetwork = document.getElementById('btn-apply-network');
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
  var btnApplyHardware = document.getElementById('btn-apply-hardware');

  var VALID_PINS = [0,2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33];

  var debounceTimer = null;
  var DEBOUNCE_MS = 400;
  var SECONDS_PER_MONTH = 2592000;
  var OPENSKY_WARN_THRESHOLD = 4000;
  var previewLastLayout = null;
  var hardwareInputDirty = false;
  var suppressHardwareInputHandler = false;

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
    applySettings({ brightness: parseInt(brightness.value, 10) });
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
    debouncedApply({
      text_color_r: r,
      text_color_g: g,
      text_color_b: b
    });
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
    applySettings({ fetch_interval: parseInt(fetchInterval.value, 10) });
  });

  // --- Timing: display cycle ---
  function updateCycleLabel(seconds) {
    cycleLabel.textContent = parseInt(seconds, 10) + ' s';
  }

  displayCycle.addEventListener('input', function() {
    updateCycleLabel(displayCycle.value);
  });
  displayCycle.addEventListener('change', function() {
    applySettings({ display_cycle: parseInt(displayCycle.value, 10) });
  });

  // --- Network & API: Apply ---
  btnApplyNetwork.addEventListener('click', function() {
    var payload = {
      wifi_ssid: dWifiSsid.value,
      wifi_password: dWifiPass.value,
      os_client_id: dOsClientId.value.trim(),
      os_client_sec: dOsClientSec.value.trim(),
      aeroapi_key: dAeroKey.value.trim()
    };
    applyWithReboot(payload, btnApplyNetwork, 'Apply');
  });

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

      // Label if space permits (zone scaled width > 40px)
      if (rw > 40 && rh > 16) {
        ctx.fillStyle = z.color;
        ctx.font = Math.min(12, Math.floor(rh * 0.4)) + 'px sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(z.label, rx + rw / 2, ry + rh / 2);
      }
    });

    // Matrix outline
    ctx.strokeStyle = '#30363d';
    ctx.lineWidth = 1;
    ctx.strokeRect(0, 0, drawWidth, drawHeight);
  }

  function updatePreviewFromInputs() {
    var dims = parseHardwareDimensionsFromInputs();
    var layout;
    if (!dims) {
      layout = { valid: false };
    } else {
      layout = computeLayout(dims.tilesX, dims.tilesY, dims.tilePixels);
      layout.hardware = {
        tilesX: dims.tilesX,
        tilesY: dims.tilesY,
        tilePixels: dims.tilePixels
      };
    }
    renderLayoutCanvas(layout);
  }

  function onHardwareInput() {
    if (!suppressHardwareInputHandler) {
      hardwareInputDirty = true;
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
  });

  // --- Hardware: Apply ---
  btnApplyHardware.addEventListener('click', function() {
    var tilesX = parseUint8Field(dTilesX, 'Tiles X', false);
    if (tilesX === null) return;
    var tilesY = parseUint8Field(dTilesY, 'Tiles Y', false);
    if (tilesY === null) return;
    var tilePixels = parseUint8Field(dTilePixels, 'Pixels per tile', false);
    if (tilePixels === null) return;
    var dp = parseUint8Field(dDisplayPin, 'Display data pin', true);
    if (dp === null || VALID_PINS.indexOf(dp) === -1) {
      FW.showToast('Invalid GPIO pin. Supported: ' + VALID_PINS.join(', '), 'error');
      return;
    }
    var originCorner = parseUint8Field(dOriginCorner, 'Origin corner', true);
    if (originCorner === null) return;
    var scanDir = parseUint8Field(dScanDir, 'Scan direction', true);
    if (scanDir === null) return;
    var zigzag = parseUint8Field(dZigzag, 'Zigzag', true);
    if (zigzag === null) return;
    var payload = {
      tiles_x: tilesX,
      tiles_y: tilesY,
      tile_pixels: tilePixels,
      display_pin: dp,
      origin_corner: originCorner,
      scan_dir: scanDir,
      zigzag: zigzag
    };
    applyWithReboot(payload, btnApplyHardware, 'Apply');
  });

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
  var calibrationActive = false;
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
    FW.post('/api/calibration/stop', {}).catch(function() {
      // Best-effort stop — ignore network errors
    });
  }

  function setCalibrationOpen(shouldOpen) {
    calBody.style.display = shouldOpen ? '' : 'none';
    calToggle.classList.toggle('open', shouldOpen);

    if (shouldOpen) {
      syncCalibrationFromSettings();
      renderCalibrationCanvas();
      startCalibrationMode();
    } else {
      stopCalibrationMode();
    }
  }

  function onCalibrationChange() {
    var values = readCalibrationValues();

    // Sync back to hardware fields so Apply picks them up
    dOriginCorner.value = values.origin_corner;
    dScanDir.value = values.scan_dir;
    dZigzag.value = values.zigzag;

    // Instant canvas update
    renderCalibrationCanvas();

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

  // Stop calibration on page unload
  window.addEventListener('beforeunload', function() {
    if (calibrationActive) {
      // Synchronous XHR for reliable unload delivery
      try {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', '/api/calibration/stop', false);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.send('{}');
      } catch (e) { /* best effort */ }
    }
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
      var buf = new Uint16Array(reader.result);
      if (buf.length !== 1024) {
        entry.valid = false;
        entry.error = 'Decode error: unexpected pixel count';
        updateRowState(entry);
        updateUploadButton();
        return;
      }

      // Decode RGB565 -> RGBA for canvas
      var canvas = document.createElement('canvas');
      canvas.width = 32;
      canvas.height = 32;
      var ctx = canvas.getContext('2d');
      var imgData = ctx.createImageData(32, 32);
      var pixels = imgData.data;

      for (var i = 0; i < 1024; i++) {
        var px = buf[i];
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
        var pixels = new Uint16Array(buf);
        var ctx = canvas.getContext('2d');
        var imgData = ctx.createImageData(32, 32);
        var d = imgData.data;
        for (var i = 0; i < 1024; i++) {
          var px = pixels[i];
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

  // --- Init ---
  var settingsPromise = loadSettings();

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
