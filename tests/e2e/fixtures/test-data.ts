/**
 * FlightWall Test Data Fixtures
 *
 * Static test data for various testing scenarios.
 * Organized by test category for easy discovery.
 */

// ============================================================================
// Display Settings Test Data
// ============================================================================

export const DISPLAY_SETTINGS = {
  /** Minimum valid brightness */
  MIN_BRIGHTNESS: 0,
  /** Maximum valid brightness */
  MAX_BRIGHTNESS: 255,
  /** Default brightness per architecture */
  DEFAULT_BRIGHTNESS: 5,
  /** Test brightness values for slider testing */
  BRIGHTNESS_STEPS: [0, 25, 50, 100, 150, 200, 255],

  /** Pure white RGB */
  COLOR_WHITE: { r: 255, g: 255, b: 255 },
  /** Pure red RGB */
  COLOR_RED: { r: 255, g: 0, b: 0 },
  /** Pure green RGB */
  COLOR_GREEN: { r: 0, g: 255, b: 0 },
  /** Pure blue RGB */
  COLOR_BLUE: { r: 0, g: 0, b: 255 },
  /** Amber (warm) RGB */
  COLOR_AMBER: { r: 255, g: 191, b: 0 },
} as const;

// ============================================================================
// Hardware Configuration Test Data
// ============================================================================

export const HARDWARE_CONFIGS = {
  /** Standard 10x2 @ 16px (160x32 matrix) - architecture test vector 1 */
  STANDARD: {
    tiles_x: 10,
    tiles_y: 2,
    tile_pixels: 16,
    expectedWidth: 160,
    expectedHeight: 32,
    expectedMode: 'full' as const,
  },

  /** Narrow 5x2 @ 16px (80x32 matrix) - architecture test vector 2 */
  NARROW: {
    tiles_x: 5,
    tiles_y: 2,
    tile_pixels: 16,
    expectedWidth: 80,
    expectedHeight: 32,
    expectedMode: 'full' as const,
  },

  /** Expanded 12x3 @ 16px (192x48 matrix) - architecture test vector 3 */
  EXPANDED: {
    tiles_x: 12,
    tiles_y: 3,
    tile_pixels: 16,
    expectedWidth: 192,
    expectedHeight: 48,
    expectedMode: 'expanded' as const,
  },

  /** Compact 10x1 @ 16px (160x16 matrix) - architecture test vector 4 */
  COMPACT: {
    tiles_x: 10,
    tiles_y: 1,
    tile_pixels: 16,
    expectedWidth: 160,
    expectedHeight: 16,
    expectedMode: 'compact' as const,
  },

  /** Invalid: width < height */
  INVALID_TALL: {
    tiles_x: 1,
    tiles_y: 10,
    tile_pixels: 16,
    shouldBeInvalid: true,
  },
} as const;

// ============================================================================
// Location Test Data
// ============================================================================

export const LOCATIONS = {
  /** San Francisco (default per architecture) */
  SAN_FRANCISCO: {
    lat: 37.7749,
    lon: -122.4194,
    radius_km: 10,
  },

  /** Los Angeles International Airport */
  LAX: {
    lat: 33.9425,
    lon: -118.4081,
    radius_km: 15,
  },

  /** London Heathrow */
  HEATHROW: {
    lat: 51.4700,
    lon: -0.4543,
    radius_km: 20,
  },

  /** Tokyo Haneda */
  HANEDA: {
    lat: 35.5494,
    lon: 139.7798,
    radius_km: 25,
  },

  /** Edge case: minimum radius */
  MIN_RADIUS: {
    lat: 40.6413,
    lon: -73.7781,
    radius_km: 1,
  },

  /** Edge case: maximum radius */
  MAX_RADIUS: {
    lat: 40.6413,
    lon: -73.7781,
    radius_km: 100,
  },
} as const;

// ============================================================================
// Timing Configuration Test Data
// ============================================================================

export const TIMING_CONFIGS = {
  /** Default timing per architecture */
  DEFAULT: {
    fetch_interval: 30,
    display_cycle: 3,
  },

  /** Fast refresh for busy airports */
  FAST: {
    fetch_interval: 15,
    display_cycle: 2,
  },

  /** Slow refresh for power saving */
  SLOW: {
    fetch_interval: 120,
    display_cycle: 10,
  },

  /** Edge case: minimum intervals */
  MINIMUM: {
    fetch_interval: 10,
    display_cycle: 1,
  },

  /** Edge case: maximum intervals */
  MAXIMUM: {
    fetch_interval: 300,
    display_cycle: 60,
  },
} as const;

// ============================================================================
// WiFi Test Data
// ============================================================================

export const WIFI_CREDENTIALS = {
  /** Valid network credentials (non-secret test data) */
  VALID: {
    ssid: 'TestNetwork',
    password: 'testpassword123',
  },

  /** WPA2 network with complex password */
  WPA2_COMPLEX: {
    ssid: 'SecureNet_5G',
    password: 'C0mpl3x!P@ssw0rd#2024',
  },

  /** Open network (no password) */
  OPEN: {
    ssid: 'FreeWiFi',
    password: '',
  },

  /** Edge case: maximum length SSID (32 chars) */
  MAX_SSID: {
    ssid: 'A'.repeat(32),
    password: 'password',
  },

  /** Edge case: maximum length password (63 chars) */
  MAX_PASSWORD: {
    ssid: 'TestNet',
    password: 'a'.repeat(63),
  },
} as const;

// ============================================================================
// API Credentials Test Data (for testing input validation)
// ============================================================================

export const API_CREDENTIALS = {
  /** Sample OpenSky credentials (test format, not real) */
  OPENSKY_SAMPLE: {
    client_id: 'test_client_id_12345',
    client_secret: 'test_client_secret_abcdef',
  },

  /** Sample AeroAPI key (test format, not real) */
  AEROAPI_SAMPLE: {
    key: 'aeroapi_test_key_xyz123',
  },
} as const;

// ============================================================================
// Flight Data Test Fixtures (for display testing)
// ============================================================================

export const FLIGHTS = {
  /** Typical United flight */
  UAL123: {
    ident_icao: 'UAL123',
    operator_icao: 'UAL',
    flight_number: 'UA 123',
    origin_code: 'SFO',
    destination_code: 'LAX',
    aircraft_type: 'B738',
    altitude_kft: 35.0,
    speed_mph: 520.0,
    track_deg: 180.0,
    vertical_rate_fps: 0.0,
  },

  /** Delta flight with climb */
  DAL456: {
    ident_icao: 'DAL456',
    operator_icao: 'DAL',
    flight_number: 'DL 456',
    origin_code: 'JFK',
    destination_code: 'ORD',
    aircraft_type: 'A320',
    altitude_kft: 28.0,
    speed_mph: 480.0,
    track_deg: 270.0,
    vertical_rate_fps: 15.0,
  },

  /** Southwest flight descending */
  SWA789: {
    ident_icao: 'SWA789',
    operator_icao: 'SWA',
    flight_number: 'WN 789',
    origin_code: 'DEN',
    destination_code: 'PHX',
    aircraft_type: 'B737',
    altitude_kft: 12.0,
    speed_mph: 350.0,
    track_deg: 200.0,
    vertical_rate_fps: -20.0,
  },

  /** Flight with missing telemetry (NaN scenario) */
  MISSING_TELEMETRY: {
    ident_icao: 'AAL999',
    operator_icao: 'AAL',
    flight_number: 'AA 999',
    origin_code: 'DFW',
    destination_code: 'MIA',
    aircraft_type: 'B777',
    altitude_kft: NaN,
    speed_mph: NaN,
    track_deg: NaN,
    vertical_rate_fps: NaN,
  },
} as const;

// ============================================================================
// Error Messages Test Data
// ============================================================================

export const ERROR_MESSAGES = {
  /** Empty payload error */
  EMPTY_PAYLOAD: {
    error: 'No settings provided',
    code: 'EMPTY_PAYLOAD',
  },

  /** Invalid WiFi credentials */
  WIFI_INVALID: {
    error: 'Invalid WiFi credentials',
    code: 'WIFI_INVALID',
  },

  /** API authentication failed */
  AUTH_FAILED: {
    error: '401 Unauthorized — check credentials',
    code: 'AUTH_FAILED',
  },

  /** Rate limited */
  RATE_LIMITED: {
    error: '429 rate limited, retry 30s',
    code: 'RATE_LIMITED',
  },

  /** Storage full */
  STORAGE_FULL: {
    error: 'LittleFS storage full',
    code: 'STORAGE_FULL',
  },
} as const;

// ============================================================================
// Test Selectors (data-testid attributes)
// ============================================================================

export const TEST_IDS = {
  // Dashboard sections
  DISPLAY_CARD: 'display-card',
  TIMING_CARD: 'timing-card',
  NETWORK_CARD: 'network-card',
  HARDWARE_CARD: 'hardware-card',
  LOCATION_CARD: 'location-card',
  HEALTH_CARD: 'health-card',

  // Display controls
  BRIGHTNESS_SLIDER: 'brightness-slider',
  BRIGHTNESS_VALUE: 'brightness-value',
  COLOR_R_SLIDER: 'color-r',
  COLOR_G_SLIDER: 'color-g',
  COLOR_B_SLIDER: 'color-b',
  COLOR_PREVIEW: 'color-preview',

  // Timing controls
  FETCH_INTERVAL_SLIDER: 'fetch-interval',
  DISPLAY_CYCLE_SLIDER: 'display-cycle',

  // Hardware controls
  TILES_X_INPUT: 'd-tiles-x',
  TILES_Y_INPUT: 'd-tiles-y',
  TILE_PIXELS_SELECT: 'd-tile-pixels',
  DISPLAY_PIN_INPUT: 'd-display-pin',
  PREVIEW_CANVAS: 'preview-canvas',

  // WiFi controls
  WIFI_SSID_INPUT: 'd-wifi-ssid',
  WIFI_PASSWORD_INPUT: 'd-wifi-pass',
  WIFI_SCAN_BUTTON: 'd-btn-scan',
  WIFI_SCAN_RESULTS: 'd-scan-results',

  // Actions
  SAVE_BUTTON: 'btn-save',
  REBOOT_BUTTON: 'btn-reboot',
  RESET_BUTTON: 'btn-reset',

  // Status indicators
  STATUS_TOAST: 'status-toast',
  ERROR_MESSAGE: 'error-message',
} as const;
