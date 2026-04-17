/**
 * FlightWall API Response Fixtures
 *
 * Mock API responses matching the actual ESP32 web portal contract.
 * These fixtures ensure tests validate against realistic data structures.
 *
 * Architecture Reference:
 * - API envelope: { ok: true, data: {...} } or { ok: false, error: "...", code: "..." }
 * - Naming: snake_case for all JSON fields (per architecture rule)
 */

// ============================================================================
// Type Definitions (matching firmware API contracts)
// ============================================================================

export interface ApiResponse<T> {
  ok: boolean;
  data?: T;
  error?: string;
  code?: string;
}

export interface SettingsData {
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
  // Sensitive fields omitted in GET responses
}

export interface StatusData {
  subsystems: {
    wifi: SubsystemStatus;
    opensky: SubsystemStatus;
    aeroapi: SubsystemStatus;
    cdn: SubsystemStatus;
    nvs: SubsystemStatus;
    littlefs: SubsystemStatus;
  };
  wifi_detail: {
    ip: string;
    rssi: number;
    ssid: string;
    mode: 'ap' | 'sta';
  };
  device: {
    heap_free: number;
    heap_total: number;
    uptime_seconds: number;
    firmware_version: string;
  };
  flight: {
    current_count: number;
    last_fetch_ms: number;
    current_index: number;
  };
  quota: {
    aeroapi_calls: number;
    aeroapi_limit: number;
  };
}

export interface SubsystemStatus {
  level: 'ok' | 'warning' | 'error';
  message: string;
}

export interface LayoutData {
  matrix: {
    width: number;
    height: number;
    mode: 'compact' | 'full' | 'expanded';
  };
  logo_zone: ZoneRect;
  flight_zone: ZoneRect;
  telemetry_zone: ZoneRect;
  hardware: {
    tiles_x: number;
    tiles_y: number;
    tile_pixels: number;
  };
}

export interface ZoneRect {
  x: number;
  y: number;
  w: number;
  h: number;
}

export interface WifiNetwork {
  ssid: string;
  rssi: number;
  encryption: number;
  channel: number;
}

export interface LogoInfo {
  name: string;
  size: number;
}

export interface LogosData {
  logos: LogoInfo[];
  storage: {
    used: number;
    total: number;
    logo_count: number;
  };
}

// Display modes types (ds-3.3, ds-3.4, ds-3.6)
export interface DisplayMode {
  id: string;
  name: string;
  description: string;
  active: boolean;
}

export interface DisplayModesData {
  modes: DisplayMode[];
  active_mode: string;
  switch_state: 'idle' | 'switching';
  upgrade_notification: boolean;
  registry_error?: string;
}

// ============================================================================
// Default Mock Responses
// ============================================================================

/**
 * Default settings matching the architecture's compile-time defaults.
 */
export const DEFAULT_SETTINGS: SettingsData = {
  brightness: 5,
  text_color_r: 255,
  text_color_g: 255,
  text_color_b: 255,
  fetch_interval: 30,
  display_cycle: 3,
  tiles_x: 10,
  tiles_y: 2,
  tile_pixels: 16,
  display_pin: 25,
  origin_corner: 0,
  scan_dir: 0,
  zigzag: 0,
  center_lat: 37.7749,
  center_lon: -122.4194,
  radius_km: 10.0,
  wifi_ssid: 'TestNetwork',
};

/**
 * Standard layout for 10x2 @ 16px (160x32 matrix, "full" mode).
 * Matches architecture test vector 1.
 */
export const STANDARD_LAYOUT: LayoutData = {
  matrix: {
    width: 160,
    height: 32,
    mode: 'full',
  },
  logo_zone: { x: 0, y: 0, w: 32, h: 32 },
  flight_zone: { x: 32, y: 0, w: 128, h: 16 },
  telemetry_zone: { x: 32, y: 16, w: 128, h: 16 },
  hardware: {
    tiles_x: 10,
    tiles_y: 2,
    tile_pixels: 16,
  },
};

/**
 * Healthy system status (all subsystems OK).
 */
export const HEALTHY_STATUS: StatusData = {
  subsystems: {
    wifi: { level: 'ok', message: 'Connected, -52 dBm' },
    opensky: { level: 'ok', message: 'Authenticated ✓' },
    aeroapi: { level: 'ok', message: 'Connected ✓' },
    cdn: { level: 'ok', message: 'Reachable ✓' },
    nvs: { level: 'ok', message: 'OK' },
    littlefs: { level: 'ok', message: '847KB / 1.9MB used' },
  },
  wifi_detail: {
    ip: '192.168.1.100',
    rssi: -52,
    ssid: 'TestNetwork',
    mode: 'sta',
  },
  device: {
    heap_free: 180000,
    heap_total: 320000,
    uptime_seconds: 86400,
    firmware_version: '1.0.0',
  },
  flight: {
    current_count: 3,
    last_fetch_ms: 5000,
    current_index: 0,
  },
  quota: {
    aeroapi_calls: 150,
    aeroapi_limit: 1000,
  },
};

/**
 * System status with warnings (partial degradation).
 */
export const WARNING_STATUS: StatusData = {
  ...HEALTHY_STATUS,
  subsystems: {
    ...HEALTHY_STATUS.subsystems,
    wifi: { level: 'warning', message: 'Weak signal, -78 dBm' },
    aeroapi: { level: 'warning', message: '429 rate limited, retry 30s' },
  },
  wifi_detail: {
    ...HEALTHY_STATUS.wifi_detail,
    rssi: -78,
  },
};

/**
 * System status in AP mode (wizard scenario).
 */
export const AP_MODE_STATUS: StatusData = {
  ...HEALTHY_STATUS,
  wifi_detail: {
    ip: '192.168.4.1',
    rssi: 0,
    ssid: 'FlightWall-Setup',
    mode: 'ap',
  },
  subsystems: {
    wifi: { level: 'ok', message: 'AP Mode Active' },
    opensky: { level: 'error', message: 'Not configured' },
    aeroapi: { level: 'error', message: 'Not configured' },
    cdn: { level: 'error', message: 'No network' },
    nvs: { level: 'ok', message: 'OK' },
    littlefs: { level: 'ok', message: '40KB / 1.9MB used' },
  },
};

/**
 * Sample WiFi scan results.
 */
export const WIFI_SCAN_RESULTS: WifiNetwork[] = [
  { ssid: 'HomeNetwork', rssi: -45, encryption: 3, channel: 6 },
  { ssid: 'Neighbor_5G', rssi: -65, encryption: 3, channel: 36 },
  { ssid: 'CoffeeShop', rssi: -70, encryption: 0, channel: 11 },
  { ssid: 'Airport_Free', rssi: -80, encryption: 0, channel: 1 },
];

/**
 * Sample logos list.
 */
export const LOGOS_DATA: LogosData = {
  logos: [
    { name: 'UAL', size: 2048 },
    { name: 'DAL', size: 2048 },
    { name: 'AAL', size: 2048 },
    { name: 'SWA', size: 2048 },
  ],
  storage: {
    used: 8192,
    total: 1900000,
    logo_count: 4,
  },
};

/**
 * Default display modes (ds-3.3, ds-3.4, ds-3.6).
 * Includes classic_card (active), live_flight, and minimal modes.
 */
export const DEFAULT_DISPLAY_MODES: DisplayModesData = {
  modes: [
    {
      id: 'classic_card',
      name: 'Classic Card',
      description: 'Traditional flight card display',
      active: true,
    },
    {
      id: 'live_flight',
      name: 'Live Flight Card',
      description: 'Real-time flight tracking with live updates',
      active: false,
    },
    {
      id: 'minimal',
      name: 'Minimal',
      description: 'Clean, minimal display',
      active: false,
    },
  ],
  active_mode: 'classic_card',
  switch_state: 'idle',
  upgrade_notification: false,
};

/**
 * Display modes with upgrade notification enabled (ds-3.6).
 * Use for testing the upgrade banner visibility.
 */
export const DISPLAY_MODES_WITH_UPGRADE: DisplayModesData = {
  ...DEFAULT_DISPLAY_MODES,
  upgrade_notification: true,
};

// ============================================================================
// Response Factory Functions
// ============================================================================

/**
 * Create a successful API response.
 */
export function successResponse<T>(data: T): ApiResponse<T> {
  return { ok: true, data };
}

/**
 * Create an error API response.
 */
export function errorResponse(error: string, code: string): ApiResponse<never> {
  return { ok: false, error, code };
}

/**
 * Create settings POST success response.
 */
export function settingsApplyResponse(
  applied: string[],
  rebootRequired = false
): { ok: true; applied: string[]; reboot_required: boolean } {
  return { ok: true, applied, reboot_required: rebootRequired };
}
