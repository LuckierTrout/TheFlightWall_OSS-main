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

  deviceIp.textContent = window.location.hostname || '';

  // --- Load current settings ---
  function loadSettings() {
    FW.get('/api/settings').then(function(res) {
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

      // Hardware
      if (d.tiles_x !== undefined) dTilesX.value = d.tiles_x;
      if (d.tiles_y !== undefined) dTilesY.value = d.tiles_y;
      if (d.tile_pixels !== undefined) dTilePixels.value = d.tile_pixels;
      if (d.display_pin !== undefined) dDisplayPin.value = d.display_pin;
      if (d.origin_corner !== undefined) dOriginCorner.value = d.origin_corner;
      if (d.scan_dir !== undefined) dScanDir.value = d.scan_dir;
      if (d.zigzag !== undefined) dZigzag.value = d.zigzag;
      updateHwResolution();

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
    var tx = parseInt(dTilesX.value, 10);
    var ty = parseInt(dTilesY.value, 10);
    var tp = parseInt(dTilePixels.value, 10);
    if (tx > 0 && ty > 0 && tp > 0) {
      dResText.textContent = 'Display: ' + (tx * tp) + ' x ' + (ty * tp) + ' pixels';
    } else {
      dResText.textContent = '';
    }
  }

  dTilesX.addEventListener('input', updateHwResolution);
  dTilesY.addEventListener('input', updateHwResolution);
  dTilePixels.addEventListener('input', updateHwResolution);

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

  // --- Init ---
  loadSettings();
})();
