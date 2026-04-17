/**
 * API Test Helpers
 *
 * Utility functions for API mocking, interception, and validation
 * in Playwright tests. Mimics the ESP32 FlightWall API behavior.
 */
import { type Page, type Route, type Request } from '@playwright/test';
import {
  type ApiResponse,
  type SettingsData,
  type StatusData,
  type LayoutData,
  type LogosData,
  type WifiNetwork,
  type DisplayModesData,
  DEFAULT_SETTINGS,
  HEALTHY_STATUS,
  STANDARD_LAYOUT,
  LOGOS_DATA,
  WIFI_SCAN_RESULTS,
  DEFAULT_DISPLAY_MODES,
  successResponse,
  errorResponse,
  settingsApplyResponse,
} from '../fixtures/api-responses.js';

// ============================================================================
// Types
// ============================================================================

export interface MockApiOptions {
  /** Delay before responding (simulates network/device latency) */
  delay?: number;
  /** HTTP status code */
  status?: number;
}

export interface MockServerState {
  settings: SettingsData;
  status: StatusData;
  layout: LayoutData;
  logos: LogosData;
  wifiNetworks: WifiNetwork[];
  isScanning: boolean;
  displayModes: DisplayModesData;
}

// ============================================================================
// Mock API Manager
// ============================================================================

/**
 * Manages API mocking for a Playwright page.
 * Simulates the ESP32 FlightWall web portal behavior.
 */
export class MockApiManager {
  private state: MockServerState;

  constructor(private page: Page) {
    // Initialize with default state
    this.state = {
      settings: { ...DEFAULT_SETTINGS },
      status: { ...HEALTHY_STATUS },
      layout: { ...STANDARD_LAYOUT },
      logos: { ...LOGOS_DATA },
      wifiNetworks: [...WIFI_SCAN_RESULTS],
      isScanning: false,
      displayModes: JSON.parse(JSON.stringify(DEFAULT_DISPLAY_MODES)),
    };
  }

  /**
   * Set up all API route handlers.
   */
  async setupRoutes(): Promise<void> {
    // GET /api/settings
    await this.page.route('**/api/settings', async (route, request) => {
      if (request.method() === 'GET') {
        await this.handleGetSettings(route);
      } else if (request.method() === 'POST') {
        await this.handlePostSettings(route, request);
      }
    });

    // GET /api/status
    await this.page.route('**/api/status', async (route) => {
      await this.handleGetStatus(route);
    });

    // GET /api/layout
    await this.page.route('**/api/layout', async (route) => {
      await this.handleGetLayout(route);
    });

    // GET /api/wifi/scan
    await this.page.route('**/api/wifi/scan', async (route) => {
      await this.handleWifiScan(route);
    });

    // GET /api/logos
    await this.page.route('**/api/logos', async (route) => {
      await this.handleGetLogos(route);
    });

    // POST /api/reboot
    await this.page.route('**/api/reboot', async (route) => {
      await this.handleReboot(route);
    });

    // POST /api/reset
    await this.page.route('**/api/reset', async (route) => {
      await this.handleReset(route);
    });

    // GET /api/display/modes (ds-3.3, ds-3.6)
    await this.page.route('**/api/display/modes', async (route) => {
      await this.handleGetDisplayModes(route);
    });

    // POST /api/display/mode (ds-3.4)
    await this.page.route('**/api/display/mode', async (route, request) => {
      if (request.method() === 'POST') {
        await this.handlePostDisplayMode(route, request);
      } else {
        await route.continue();
      }
    });

    // POST /api/display/notification/dismiss (ds-3.6)
    await this.page.route('**/api/display/notification/dismiss', async (route) => {
      await this.handleDismissNotification(route);
    });
  }

  // ============================================================================
  // State Manipulation
  // ============================================================================

  /**
   * Update mock settings state.
   */
  updateSettings(partial: Partial<SettingsData>): void {
    this.state.settings = { ...this.state.settings, ...partial };
  }

  /**
   * Update mock status state.
   */
  updateStatus(partial: Partial<StatusData>): void {
    this.state.status = { ...this.state.status, ...partial };
  }

  /**
   * Set subsystem health status.
   */
  setSubsystemStatus(
    subsystem: keyof StatusData['subsystems'],
    level: 'ok' | 'warning' | 'error',
    message: string
  ): void {
    this.state.status.subsystems[subsystem] = { level, message };
  }

  /**
   * Get current mock state (for assertions).
   */
  getState(): MockServerState {
    return { ...this.state };
  }

  /**
   * Update display modes state (ds-3.6 upgrade banner testing).
   */
  updateDisplayModes(partial: Partial<DisplayModesData>): void {
    this.state.displayModes = { ...this.state.displayModes, ...partial };
  }

  /**
   * Enable upgrade notification banner (ds-3.6).
   */
  enableUpgradeNotification(): void {
    this.state.displayModes.upgrade_notification = true;
  }

  /**
   * Disable upgrade notification banner (ds-3.6).
   */
  disableUpgradeNotification(): void {
    this.state.displayModes.upgrade_notification = false;
  }

  /**
   * Simulate registry error for mode switch (ds-3.4, ds-3.6).
   */
  setRegistryError(error: string | undefined): void {
    this.state.displayModes.registry_error = error;
  }

  /**
   * Remove live_flight mode from available modes (ds-3.6 AC #7 testing).
   */
  removeLiveFlightMode(): void {
    this.state.displayModes.modes = this.state.displayModes.modes.filter(
      (m) => m.id !== 'live_flight'
    );
  }

  // ============================================================================
  // Route Handlers
  // ============================================================================

  private async handleGetSettings(route: Route): Promise<void> {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify(successResponse(this.state.settings)),
    });
  }

  private async handlePostSettings(route: Route, request: Request): Promise<void> {
    const body = request.postDataJSON();

    if (!body || Object.keys(body).length === 0) {
      await route.fulfill({
        status: 400,
        contentType: 'application/json',
        body: JSON.stringify(errorResponse('No settings provided', 'EMPTY_PAYLOAD')),
      });
      return;
    }

    // Determine which keys require reboot
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

    for (const [key, value] of Object.entries(body)) {
      if (key in this.state.settings) {
        (this.state.settings as Record<string, unknown>)[key] = value;
        applied.push(key);
        if (rebootKeys.includes(key)) {
          rebootRequired = true;
        }
      }
    }

    // Update layout if hardware changed
    if (
      applied.includes('tiles_x') ||
      applied.includes('tiles_y') ||
      applied.includes('tile_pixels')
    ) {
      this.updateLayoutFromSettings();
    }

    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify(settingsApplyResponse(applied, rebootRequired)),
    });
  }

  private async handleGetStatus(route: Route): Promise<void> {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify(successResponse(this.state.status)),
    });
  }

  private async handleGetLayout(route: Route): Promise<void> {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify(successResponse(this.state.layout)),
    });
  }

  private async handleWifiScan(route: Route): Promise<void> {
    // First call starts scan, subsequent calls return results
    if (!this.state.isScanning) {
      this.state.isScanning = true;
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({ ok: true, scanning: true, data: [] }),
      });
      // Reset scanning flag after a delay (simulated async scan)
      setTimeout(() => {
        this.state.isScanning = false;
      }, 2000);
    } else {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          ok: true,
          scanning: false,
          data: this.state.wifiNetworks,
        }),
      });
    }
  }

  private async handleGetLogos(route: Route): Promise<void> {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({
        ok: true,
        data: this.state.logos.logos,
        storage: this.state.logos.storage,
      }),
    });
  }

  private async handleReboot(route: Route): Promise<void> {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ ok: true, message: 'Rebooting...' }),
    });
  }

  private async handleReset(route: Route): Promise<void> {
    // Reset state to defaults
    this.state.settings = { ...DEFAULT_SETTINGS };
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ ok: true, message: 'Factory reset complete' }),
    });
  }

  private async handleGetDisplayModes(route: Route): Promise<void> {
    const dm = this.state.displayModes;
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({
        ok: true,
        data: {
          modes: dm.modes,
          active_mode: dm.active_mode,
          switch_state: dm.switch_state,
          upgrade_notification: dm.upgrade_notification,
          ...(dm.registry_error ? { registry_error: dm.registry_error } : {}),
        },
      }),
    });
  }

  private async handlePostDisplayMode(route: Route, request: Request): Promise<void> {
    const body = request.postDataJSON() as { mode?: string };
    const modeId = body?.mode;

    if (!modeId) {
      await route.fulfill({
        status: 400,
        contentType: 'application/json',
        body: JSON.stringify(errorResponse('Missing mode field', 'MISSING_MODE')),
      });
      return;
    }

    const modeExists = this.state.displayModes.modes.some((m) => m.id === modeId);
    if (!modeExists) {
      await route.fulfill({
        status: 404,
        contentType: 'application/json',
        body: JSON.stringify(errorResponse('Mode not found', 'MODE_NOT_FOUND')),
      });
      return;
    }

    // Check for simulated registry error
    if (this.state.displayModes.registry_error) {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          ok: true,
          data: { registry_error: this.state.displayModes.registry_error },
        }),
      });
      return;
    }

    // Simulate successful mode switch
    this.state.displayModes.modes.forEach((m) => {
      m.active = m.id === modeId;
    });
    this.state.displayModes.active_mode = modeId;
    this.state.displayModes.switch_state = 'idle';

    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ ok: true, data: { active_mode: modeId } }),
    });
  }

  private async handleDismissNotification(route: Route): Promise<void> {
    this.state.displayModes.upgrade_notification = false;
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ ok: true }),
    });
  }

  // ============================================================================
  // Layout Calculation (mirrors firmware LayoutEngine)
  // ============================================================================

  private updateLayoutFromSettings(): void {
    const { tiles_x, tiles_y, tile_pixels } = this.state.settings;
    const mw = tiles_x * tile_pixels;
    const mh = tiles_y * tile_pixels;

    let mode: 'compact' | 'full' | 'expanded';
    if (mh < 32) mode = 'compact';
    else if (mh < 48) mode = 'full';
    else mode = 'expanded';

    const logoW = mh; // Square logo
    const splitY = Math.floor(mh / 2);

    this.state.layout = {
      matrix: { width: mw, height: mh, mode },
      logo_zone: { x: 0, y: 0, w: logoW, h: mh },
      flight_zone: { x: logoW, y: 0, w: mw - logoW, h: splitY },
      telemetry_zone: { x: logoW, y: splitY, w: mw - logoW, h: mh - splitY },
      hardware: { tiles_x, tiles_y, tile_pixels },
    };
  }
}

// ============================================================================
// Standalone Helper Functions
// ============================================================================

/**
 * Wait for a specific API call to complete.
 */
export async function waitForApi(
  page: Page,
  urlPattern: string | RegExp,
  options?: { timeout?: number }
): Promise<ApiResponse<unknown>> {
  const response = await page.waitForResponse(
    (res) =>
      typeof urlPattern === 'string'
        ? res.url().includes(urlPattern)
        : urlPattern.test(res.url()),
    { timeout: options?.timeout ?? 10_000 }
  );
  return response.json();
}

/**
 * Mock a single API endpoint with a custom response.
 */
export async function mockApiEndpoint(
  page: Page,
  urlPattern: string | RegExp,
  response: object,
  options?: MockApiOptions
): Promise<void> {
  await page.route(urlPattern, async (route) => {
    if (options?.delay) {
      await new Promise((resolve) => setTimeout(resolve, options.delay));
    }
    await route.fulfill({
      status: options?.status ?? 200,
      contentType: 'application/json',
      body: JSON.stringify(response),
    });
  });
}

/**
 * Mock API to return an error response.
 */
export async function mockApiError(
  page: Page,
  urlPattern: string | RegExp,
  errorMessage: string,
  errorCode: string,
  httpStatus = 400
): Promise<void> {
  await mockApiEndpoint(
    page,
    urlPattern,
    errorResponse(errorMessage, errorCode),
    { status: httpStatus }
  );
}

/**
 * Clear all route handlers for a page.
 */
export async function clearApiMocks(page: Page): Promise<void> {
  await page.unrouteAll({ behavior: 'ignoreErrors' });
}
