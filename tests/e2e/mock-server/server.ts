/**
 * FlightWall Mock Server
 *
 * A lightweight mock server that simulates the ESP32 FlightWall web portal.
 * Used for local E2E test development without requiring actual hardware.
 *
 * Features:
 * - Serves static HTML/CSS/JS from firmware/data-src/
 * - Provides mock API endpoints matching the real device
 * - Simulates device state (settings, status, layout)
 * - Supports both AP mode (wizard) and STA mode (dashboard)
 *
 * Usage:
 *   npx tsx mock-server/server.ts
 *   npx tsx mock-server/server.ts --port 3001 --mode ap
 */
import http, { type IncomingMessage, type ServerResponse } from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

// ============================================================================
// Configuration
// ============================================================================

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '../../..');
const WEB_ASSETS_DIR = path.join(PROJECT_ROOT, 'firmware/data-src');

// Parse command line arguments
const args = process.argv.slice(2);
const PORT = parseInt(args.find((a) => a.startsWith('--port='))?.split('=')[1] ?? '3000', 10);
const MODE = (args.find((a) => a.startsWith('--mode='))?.split('=')[1] ?? 'sta') as 'ap' | 'sta';

// ============================================================================
// Mock Device State
// ============================================================================

interface DeviceState {
  settings: {
    brightness: number;
    text_color_r: number;
    text_color_g: number;
    text_color_b: number;
    fetch_interval: number;
    display_cycle: number;
    tiles_x: number;
    tiles_y: number;
    tile_pixels: number;
    display_pin: number;
    origin_corner: number;
    scan_dir: number;
    zigzag: number;
    center_lat: number;
    center_lon: number;
    radius_km: number;
    wifi_ssid: string;
    logo_width_pct: number;
    flight_height_pct: number;
    layout_mode: number;
  };
  status: {
    subsystems: Record<string, { level: string; message: string }>;
    wifi_detail: { ip: string; rssi: number; ssid: string; mode: string };
    device: {
      heap_free: number;
      heap_total: number;
      uptime_seconds: number;
      firmware_version: string;
    };
    flight: { current_count: number; last_fetch_ms: number; current_index: number };
    quota: { aeroapi_calls: number; aeroapi_limit: number };
  };
  wifiNetworks: Array<{ ssid: string; rssi: number; encryption: number; channel: number }>;
  logos: Array<{ name: string; size: number }>;
  isScanning: boolean;
}

const state: DeviceState = {
  settings: {
    brightness: 40,
    text_color_r: 255,
    text_color_g: 255,
    text_color_b: 255,
    fetch_interval: 30,
    display_cycle: 5,
    tiles_x: 10,
    tiles_y: 2,
    tile_pixels: 16,
    display_pin: 25,
    origin_corner: 0,
    scan_dir: 0,
    zigzag: 0,
    center_lat: 37.7749,
    center_lon: -122.4194,
    radius_km: 10,
    wifi_ssid: 'MockNetwork',
    logo_width_pct: 0,
    flight_height_pct: 0,
    layout_mode: 0,
  },
  status: {
    subsystems: {
      wifi: { level: 'ok', message: MODE === 'ap' ? 'AP Mode Active' : 'Connected, -52 dBm' },
      opensky: { level: MODE === 'ap' ? 'error' : 'ok', message: MODE === 'ap' ? 'Not configured' : 'Authenticated ✓' },
      aeroapi: { level: MODE === 'ap' ? 'error' : 'ok', message: MODE === 'ap' ? 'Not configured' : 'Connected ✓' },
      cdn: { level: MODE === 'ap' ? 'error' : 'ok', message: MODE === 'ap' ? 'No network' : 'Reachable ✓' },
      nvs: { level: 'ok', message: 'OK' },
      littlefs: { level: 'ok', message: '847KB / 1.9MB used' },
    },
    wifi_detail: {
      ip: MODE === 'ap' ? '192.168.4.1' : '192.168.1.100',
      rssi: MODE === 'ap' ? 0 : -52,
      ssid: MODE === 'ap' ? 'FlightWall-Setup' : 'MockNetwork',
      mode: MODE,
    },
    device: {
      heap_free: 180000,
      heap_total: 320000,
      uptime_seconds: 3600,
      firmware_version: '1.0.0-mock',
    },
    flight: {
      current_count: MODE === 'ap' ? 0 : 3,
      last_fetch_ms: MODE === 'ap' ? 0 : 5000,
      current_index: 0,
    },
    quota: {
      aeroapi_calls: 150,
      aeroapi_limit: 1000,
    },
  },
  wifiNetworks: [
    { ssid: 'HomeNetwork', rssi: -45, encryption: 3, channel: 6 },
    { ssid: 'Neighbor_5G', rssi: -65, encryption: 3, channel: 36 },
    { ssid: 'CoffeeShop', rssi: -70, encryption: 0, channel: 11 },
    { ssid: 'Airport_Free', rssi: -80, encryption: 0, channel: 1 },
  ],
  logos: [
    { name: 'UAL', size: 2048 },
    { name: 'DAL', size: 2048 },
    { name: 'AAL', size: 2048 },
    { name: 'SWA', size: 2048 },
  ],
  isScanning: false,
};

// ============================================================================
// Layout Calculation (mirrors firmware LayoutEngine)
// ============================================================================

function computeLayout() {
  const { tiles_x, tiles_y, tile_pixels, logo_width_pct, flight_height_pct, layout_mode } = state.settings;
  const mw = tiles_x * tile_pixels;
  const mh = tiles_y * tile_pixels;

  let mode: string;
  if (mh < 32) mode = 'compact';
  else if (mh < 48) mode = 'full';
  else mode = 'expanded';

  let logoW = mh;
  if (logo_width_pct > 0 && logo_width_pct <= 99) {
    logoW = Math.round((mw * logo_width_pct) / 100);
    logoW = Math.max(1, Math.min(mw - 1, logoW));
  }

  let splitY = Math.floor(mh / 2);
  if (flight_height_pct > 0 && flight_height_pct <= 99) {
    splitY = Math.round((mh * flight_height_pct) / 100);
    splitY = Math.max(1, Math.min(mh - 1, splitY));
  }

  let logoZone, flightZone, telemetryZone;
  if (layout_mode === 1) {
    logoZone = { x: 0, y: 0, w: logoW, h: splitY };
    flightZone = { x: logoW, y: 0, w: mw - logoW, h: splitY };
    telemetryZone = { x: 0, y: splitY, w: mw, h: mh - splitY };
  } else {
    logoZone = { x: 0, y: 0, w: logoW, h: mh };
    flightZone = { x: logoW, y: 0, w: mw - logoW, h: splitY };
    telemetryZone = { x: logoW, y: splitY, w: mw - logoW, h: mh - splitY };
  }

  return {
    matrix: { width: mw, height: mh, mode },
    logo_zone: logoZone,
    flight_zone: flightZone,
    telemetry_zone: telemetryZone,
    hardware: { tiles_x, tiles_y, tile_pixels },
  };
}

// ============================================================================
// HTTP Request Handling
// ============================================================================

const MIME_TYPES: Record<string, string> = {
  '.html': 'text/html',
  '.css': 'text/css',
  '.js': 'application/javascript',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.ico': 'image/x-icon',
};

function sendJson(res: ServerResponse, data: unknown, status = 200): void {
  res.writeHead(status, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(data));
}

function sendError(res: ServerResponse, error: string, code: string, status = 400): void {
  sendJson(res, { ok: false, error, code }, status);
}

function readBody(req: IncomingMessage): Promise<string> {
  return new Promise((resolve, reject) => {
    let body = '';
    req.on('data', (chunk) => (body += chunk.toString()));
    req.on('end', () => resolve(body));
    req.on('error', reject);
  });
}

async function handleRequest(req: IncomingMessage, res: ServerResponse): Promise<void> {
  const url = new URL(req.url ?? '/', `http://localhost:${PORT}`);
  const pathname = url.pathname;
  const method = req.method ?? 'GET';

  // CORS headers
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, DELETE, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  // API routes
  if (pathname.startsWith('/api/')) {
    await handleApiRoute(pathname, method, req, res);
    return;
  }

  // Static file serving
  await handleStaticFile(pathname, res);
}

async function handleApiRoute(
  pathname: string,
  method: string,
  req: IncomingMessage,
  res: ServerResponse
): Promise<void> {
  // GET /api/settings
  if (pathname === '/api/settings' && method === 'GET') {
    sendJson(res, { ok: true, data: state.settings });
    return;
  }

  // POST /api/settings
  if (pathname === '/api/settings' && method === 'POST') {
    const body = await readBody(req);
    let data: Record<string, unknown>;
    try {
      data = JSON.parse(body);
    } catch {
      sendError(res, 'Invalid JSON', 'INVALID_JSON');
      return;
    }

    if (Object.keys(data).length === 0) {
      sendError(res, 'No settings provided', 'EMPTY_PAYLOAD');
      return;
    }

    const rebootKeys = [
      'wifi_ssid',
      'wifi_password',
      'os_client_id',
      'os_client_sec',
      'aeroapi_key',
      'display_pin',
      'tiles_x',
      'tiles_y',
      'tile_pixels',
    ];

    const applied: string[] = [];
    let rebootRequired = false;

    for (const [key, value] of Object.entries(data)) {
      if (key in state.settings) {
        (state.settings as Record<string, unknown>)[key] = value;
        applied.push(key);
        if (rebootKeys.includes(key)) {
          rebootRequired = true;
        }
      }
    }

    sendJson(res, { ok: true, applied, reboot_required: rebootRequired });
    return;
  }

  // GET /api/status
  if (pathname === '/api/status' && method === 'GET') {
    // Update uptime
    state.status.device.uptime_seconds += 1;
    sendJson(res, { ok: true, data: state.status });
    return;
  }

  // GET /api/layout
  if (pathname === '/api/layout' && method === 'GET') {
    sendJson(res, { ok: true, data: computeLayout() });
    return;
  }

  // GET /api/wifi/scan
  if (pathname === '/api/wifi/scan' && method === 'GET') {
    if (!state.isScanning) {
      state.isScanning = true;
      sendJson(res, { ok: true, scanning: true, data: [] });
      setTimeout(() => {
        state.isScanning = false;
      }, 2000);
    } else {
      sendJson(res, { ok: true, scanning: false, data: state.wifiNetworks });
    }
    return;
  }

  // GET /api/logos
  if (pathname === '/api/logos' && method === 'GET') {
    sendJson(res, {
      ok: true,
      data: state.logos,
      storage: {
        used: state.logos.length * 2048,
        total: 1900000,
        logo_count: state.logos.length,
      },
    });
    return;
  }

  // POST /api/reboot
  if (pathname === '/api/reboot' && method === 'POST') {
    sendJson(res, { ok: true, message: 'Rebooting...' });
    return;
  }

  // POST /api/reset
  if (pathname === '/api/reset' && method === 'POST') {
    sendJson(res, { ok: true, message: 'Factory reset complete' });
    return;
  }

  // POST /api/calibration
  if (pathname === '/api/calibration' && method === 'POST') {
    sendJson(res, { ok: true, message: 'Calibration mode toggled' });
    return;
  }

  // 404 for unknown API routes
  sendError(res, 'Not found', 'NOT_FOUND', 404);
}

async function handleStaticFile(pathname: string, res: ServerResponse): Promise<void> {
  // Default to index page based on mode
  if (pathname === '/') {
    pathname = MODE === 'ap' ? '/wizard.html' : '/dashboard.html';
  }

  const filePath = path.join(WEB_ASSETS_DIR, pathname);
  const ext = path.extname(filePath);
  const mimeType = MIME_TYPES[ext] ?? 'application/octet-stream';

  try {
    const content = await fs.promises.readFile(filePath);
    res.writeHead(200, { 'Content-Type': mimeType });
    res.end(content);
  } catch {
    res.writeHead(404, { 'Content-Type': 'text/plain' });
    res.end('Not found');
  }
}

// ============================================================================
// Server Startup
// ============================================================================

const server = http.createServer(handleRequest);

server.listen(PORT, () => {
  console.log(`
╔════════════════════════════════════════════════════════════╗
║       FlightWall Mock Server                               ║
╠════════════════════════════════════════════════════════════╣
║  Mode:    ${MODE === 'ap' ? 'AP (Wizard)    ' : 'STA (Dashboard)'}                                ║
║  URL:     http://localhost:${PORT}                            ║
║  Assets:  ${WEB_ASSETS_DIR.slice(-40).padEnd(40)}   ║
╚════════════════════════════════════════════════════════════╝

API Endpoints:
  GET  /api/settings     - Current device settings
  POST /api/settings     - Apply settings (JSON body)
  GET  /api/status       - System health status
  GET  /api/layout       - Current display layout
  GET  /api/wifi/scan    - WiFi network scan
  GET  /api/logos        - Uploaded logos list
  POST /api/reboot       - Trigger device reboot
  POST /api/reset        - Factory reset

Press Ctrl+C to stop.
  `);
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down...');
  server.close(() => {
    process.exit(0);
  });
});
