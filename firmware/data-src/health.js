/* FlightWall System Health — Story 2.4 */
(function() {
  'use strict';

  var wifiBody = document.getElementById('wifi-body');
  var apisBody = document.getElementById('apis-body');
  var quotaBody = document.getElementById('quota-body');
  var deviceBody = document.getElementById('device-body');
  var flightBody = document.getElementById('flight-body');
  var refreshBtn = document.getElementById('refresh-btn');

  function dotColor(level) {
    if (level === 'ok') return 'var(--success)';
    if (level === 'warning') return '#d29922';
    return 'var(--error)';
  }

  function formatUptime(ms) {
    var s = Math.floor(ms / 1000);
    var d = Math.floor(s / 86400); s %= 86400;
    var h = Math.floor(s / 3600); s %= 3600;
    var m = Math.floor(s / 60); s %= 60;
    var parts = [];
    if (d > 0) parts.push(d + 'd');
    if (h > 0) parts.push(h + 'h');
    if (m > 0) parts.push(m + 'm');
    if (parts.length === 0) parts.push(s + 's');
    return parts.join(' ');
  }

  function formatAgo(fetchMs, nowMs) {
    if (!fetchMs) return 'Never';
    var diff = nowMs - fetchMs;
    if (diff < 0) diff = 0;
    var s = Math.floor(diff / 1000);
    if (s < 60) return s + 's ago';
    var m = Math.floor(s / 60);
    if (m < 60) return m + 'm ago';
    var h = Math.floor(m / 60);
    return h + 'h ' + (m % 60) + 'm ago';
  }

  function formatBytes(b) {
    if (b < 1024) return b + ' B';
    return (b / 1024).toFixed(1) + ' KiB';
  }

  function escHtml(value) {
    var el = document.createElement('span');
    el.textContent = value === undefined || value === null ? '' : String(value);
    return el.innerHTML;
  }

  function row(label, value) {
    return '<div class="status-row"><span class="status-label">' + escHtml(label) + '</span><span class="status-value">' + escHtml(value) + '</span></div>';
  }

  function dotRow(label, level, text) {
    return '<div class="status-row"><span class="status-label">' + escHtml(label) +
      '</span><span class="status-value"><span class="status-dot" style="background:' +
      dotColor(level) + '"></span>' + escHtml(text) + '</span></div>';
  }

  function renderWifi(d) {
    var w = d.wifi_detail;
    var html = '';
    if (w.mode === 'STA') {
      html += row('Network', w.ssid || '—');
      html += row('Signal', w.rssi + ' dBm');
      html += row('IP', w.ip);
    } else if (w.mode === 'AP') {
      html += row('Mode', 'Access Point');
      html += row('AP SSID', w.ssid || '—');
      html += row('AP IP', w.ip);
      if (w.clients !== undefined) html += row('Clients', w.clients);
    } else {
      html += row('Mode', 'Off');
    }
    wifiBody.innerHTML = html;
  }

  function renderApis(d) {
    var sub = d.subsystems;
    var map = { opensky: 'OpenSky', aeroapi: 'AeroAPI', cdn: 'CDN' };
    var html = '';
    for (var key in map) {
      if (sub[key]) {
        html += dotRow(map[key], sub[key].level, sub[key].message || sub[key].level.toUpperCase());
      }
    }
    apisBody.innerHTML = html;
  }

  function renderQuota(d) {
    var q = d.quota;
    var html = '';
    html += row('Since boot', q.fetches_since_boot);
    html += row('Monthly limit', q.limit.toLocaleString());
    html += row('Est. monthly polls', q.estimated_monthly_polls.toLocaleString());
    html += row('Fetch interval', q.fetch_interval_s + 's');
    if (q.over_pace) {
      quotaBody.parentNode.classList.add('over-pace');
      html += '<div class="status-row"><span class="estimate-warning">Over pace — reduce fetch interval or accept higher usage</span></div>';
    } else {
      quotaBody.parentNode.classList.remove('over-pace');
    }
    quotaBody.innerHTML = html;
  }

  function renderDevice(d) {
    var dev = d.device;
    var html = '';
    html += row('Uptime', formatUptime(dev.uptime_ms));
    html += row('Free heap', formatBytes(dev.free_heap));
    html += row('Storage', formatBytes(dev.fs_used) + ' / ' + formatBytes(dev.fs_total));
    deviceBody.innerHTML = html;
  }

  function renderFlight(d) {
    var f = d.flight;
    var uptime = d.device.uptime_ms;
    var html = '';
    html += row('Last fetch', formatAgo(f.last_fetch_ms, uptime));
    html += row('Flights in range', f.state_vectors);
    html += row('Enriched', f.enriched_flights);
    html += row('Logos matched', f.logos_matched === 0 ? '—' : f.logos_matched);
    flightBody.innerHTML = html;
  }

  function refresh() {
    refreshBtn.disabled = true;
    refreshBtn.textContent = 'Loading...';
    FW.get('/api/status').then(function(res) {
      refreshBtn.disabled = false;
      refreshBtn.textContent = 'Refresh';
      if (!res.body.ok || !res.body.data) {
        FW.showToast('Failed to load status', 'error');
        return;
      }
      var d = res.body.data;
      renderWifi(d);
      renderApis(d);
      renderQuota(d);
      renderDevice(d);
      renderFlight(d);
    }).catch(function() {
      refreshBtn.disabled = false;
      refreshBtn.textContent = 'Refresh';
      FW.showToast('Cannot reach device', 'error');
    });
  }

  refreshBtn.addEventListener('click', refresh);
  refresh();
})();
