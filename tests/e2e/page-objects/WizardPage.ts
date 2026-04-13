/**
 * Wizard Page Object
 *
 * Page object for the FlightWall Setup Wizard (AP mode).
 * Handles the multi-step onboarding flow for initial device configuration.
 *
 * Architecture Reference:
 * - Wizard served in AP mode (device acts as access point)
 * - Steps: WiFi → API Credentials → Location → Hardware → Review & Connect (5 steps)
 * - "Save & Connect" on Step 5 posts settings, triggers reboot, shows handoff message
 */
import { type Locator, expect } from '@playwright/test';
import { BasePage } from './BasePage.js';

export type WizardStep =
  | 'wifi'
  | 'api'
  | 'location'
  | 'hardware'
  | 'review';

export class WizardPage extends BasePage {
  // ============================================================================
  // Page Identification
  // ============================================================================

  get path(): string {
    return '/';
  }

  get pageIdentifier(): Locator {
    return this.page.locator('text=FlightWall Setup');
  }

  // ============================================================================
  // Step Navigation
  // ============================================================================

  get stepIndicator(): Locator {
    return this.page.locator('#progress');
  }

  get nextButton(): Locator {
    // Use ID — text changes from "Next →" (steps 1-4) to "Save & Connect" (step 5)
    return this.page.locator('#btn-next');
  }

  get backButton(): Locator {
    return this.page.locator('button', { hasText: /back|previous/i });
  }

  get skipButton(): Locator {
    return this.page.locator('button', { hasText: /skip/i });
  }

  // ============================================================================
  // Step 1: WiFi Configuration
  // ============================================================================

  get wifiSsidInput(): Locator {
    return this.page.locator('#wifi-ssid, [name="wifi_ssid"]');
  }

  get wifiPasswordInput(): Locator {
    return this.page.locator('#wifi-pass, [name="wifi_password"]');
  }

  get wifiScanButton(): Locator {
    return this.page.locator('button', { hasText: /scan/i });
  }

  get wifiNetworkList(): Locator {
    return this.page.locator('.wifi-networks, #scan-results');
  }

  // ============================================================================
  // Step 2: API Credentials
  // ============================================================================

  get openSkyClientIdInput(): Locator {
    return this.page.locator('#os-client-id, [name="os_client_id"]');
  }

  get openSkyClientSecretInput(): Locator {
    return this.page.locator('#os-client-sec, [name="os_client_sec"]');
  }

  get aeroApiKeyInput(): Locator {
    return this.page.locator('#aeroapi-key, [name="aeroapi_key"]');
  }

  // ============================================================================
  // Step 3: Location Configuration
  // ============================================================================

  get centerLatInput(): Locator {
    return this.page.locator('#center-lat, [name="center_lat"]');
  }

  get centerLonInput(): Locator {
    return this.page.locator('#center-lon, [name="center_lon"]');
  }

  get radiusKmInput(): Locator {
    return this.page.locator('#radius-km, [name="radius_km"]');
  }

  get useMyLocationButton(): Locator {
    return this.page.locator('button', { hasText: /my location|detect/i });
  }

  get mapContainer(): Locator {
    return this.page.locator('#map, .leaflet-container');
  }

  // ============================================================================
  // Step 4: Hardware Configuration
  // ============================================================================

  get tilesXInput(): Locator {
    return this.page.locator('#tiles-x, [name="tiles_x"]');
  }

  get tilesYInput(): Locator {
    return this.page.locator('#tiles-y, [name="tiles_y"]');
  }

  get tilePixelsInput(): Locator {
    // #tile-pixels is <input type="text">, not a <select>
    return this.page.locator('#tile-pixels, [name="tile_pixels"]');
  }

  get displayPinInput(): Locator {
    return this.page.locator('#display-pin, [name="display_pin"]');
  }

  get previewCanvas(): Locator {
    return this.page.locator('#preview-canvas, canvas');
  }

  // ============================================================================
  // Step 5: Review & Connect
  // ============================================================================

  /** "Configuration Saved" heading shown after Save & Connect triggers the reboot handoff. */
  get completeMessage(): Locator {
    return this.page.locator('h1', { hasText: 'Configuration Saved' });
  }

  // ============================================================================
  // Settings Import/Export (Foundation Release)
  // ============================================================================

  get importSettingsButton(): Locator {
    // Import zone is a div[role="button"], not a <button> element
    return this.page.locator('#settings-import-zone');
  }

  get importFileInput(): Locator {
    return this.page.locator('input[type="file"]');
  }

  get importStatus(): Locator {
    return this.page.locator('#import-status');
  }

  // ============================================================================
  // Step Navigation Actions
  // ============================================================================

  /**
   * Navigate to the next wizard step.
   */
  async goNext(): Promise<void> {
    await this.nextButton.click();
    await this.page.waitForLoadState('networkidle');
  }

  /**
   * Navigate to the previous wizard step.
   */
  async goBack(): Promise<void> {
    await this.backButton.click();
    await this.page.waitForLoadState('networkidle');
  }

  /**
   * Skip the current optional step.
   */
  async skipStep(): Promise<void> {
    if (await this.skipButton.isVisible()) {
      await this.skipButton.click();
      await this.page.waitForLoadState('networkidle');
    }
  }

  /**
   * Get the current step number (1-based).
   */
  async getCurrentStep(): Promise<number> {
    // Primary: parse the "#progress" indicator text ("Step N of 5")
    const indicator = await this.stepIndicator.textContent();
    const match = indicator?.match(/step\s*(\d+)/i);
    if (match) {
      return parseInt(match[1], 10);
    }
    // Fallback: check which #step-N section is visible (5 steps total)
    for (let i = 1; i <= 5; i++) {
      if (await this.page.locator(`#step-${i}`).isVisible()) return i;
    }
    return 0;
  }

  // ============================================================================
  // Step 1: WiFi Actions
  // ============================================================================

  /**
   * Configure WiFi credentials.
   */
  async configureWifi(ssid: string, password: string): Promise<void> {
    await this.fillInput(this.wifiSsidInput, ssid);
    await this.fillInput(this.wifiPasswordInput, password);
  }

  /**
   * Wait for the automatic WiFi scan to complete and results to appear.
   * Scanning starts automatically when the wizard loads — there is no explicit scan button.
   */
  async scanForNetworks(): Promise<void> {
    // #scan-status hides and #scan-results shows when scan completes (or times out to empty)
    await expect(this.page.locator('#scan-results')).toBeVisible({ timeout: 10_000 });
  }

  /**
   * Select a network from the scan results.
   */
  async selectNetwork(ssid: string): Promise<void> {
    const networkItem = this.wifiNetworkList.locator(`text=${ssid}`);
    await networkItem.click();
    // Network SSID should be populated
    await expect(this.wifiSsidInput).toHaveValue(ssid);
  }

  // ============================================================================
  // Step 2: API Credentials Actions
  // ============================================================================

  /**
   * Configure API credentials.
   */
  async configureApiCredentials(
    openSkyClientId: string,
    openSkyClientSecret: string,
    aeroApiKey: string
  ): Promise<void> {
    await this.fillInput(this.openSkyClientIdInput, openSkyClientId);
    await this.fillInput(this.openSkyClientSecretInput, openSkyClientSecret);
    await this.fillInput(this.aeroApiKeyInput, aeroApiKey);
  }

  // ============================================================================
  // Step 3: Location Actions
  // ============================================================================

  /**
   * Configure location manually.
   */
  async configureLocation(
    lat: number,
    lon: number,
    radiusKm: number
  ): Promise<void> {
    await this.fillInput(this.centerLatInput, String(lat));
    await this.fillInput(this.centerLonInput, String(lon));
    await this.fillInput(this.radiusKmInput, String(radiusKm));
  }

  /**
   * Use browser geolocation to detect location.
   */
  async useMyLocation(): Promise<void> {
    await this.useMyLocationButton.click();
    // Wait for geolocation to complete (browser permission required)
    await this.page.waitForFunction(
      () => {
        const latInput = document.querySelector(
          '#center-lat, [name="center_lat"]'
        ) as HTMLInputElement;
        return latInput && latInput.value !== '';
      },
      { timeout: 10_000 }
    );
  }

  // ============================================================================
  // Step 4: Hardware Actions
  // ============================================================================

  /**
   * Configure matrix dimensions.
   */
  async configureMatrix(
    tilesX: number,
    tilesY: number,
    tilePixels: number
  ): Promise<void> {
    await this.fillInput(this.tilesXInput, String(tilesX));
    await this.fillInput(this.tilesYInput, String(tilesY));
    await this.fillInput(this.tilePixelsInput, String(tilePixels));
  }

  /**
   * Set the LED data pin.
   */
  async setDisplayPin(pin: number): Promise<void> {
    await this.fillInput(this.displayPinInput, String(pin));
  }

  // ============================================================================
  // Step 5: Review & Connect Actions
  // ============================================================================

  /**
   * On Step 5, click "Save & Connect" to post settings and trigger device reboot.
   * The wizard replaces its content with a "Configuration Saved" handoff message.
   */
  async completeWizard(): Promise<void> {
    // Step 5 shows "Save & Connect" as the next-button label
    await this.nextButton.click();
    // Wait for the handoff message — device is now saving and rebooting
    await expect(this.completeMessage).toBeVisible({ timeout: 10_000 });
  }

  // ============================================================================
  // Full Wizard Flow
  // ============================================================================

  /**
   * Complete the entire 5-step wizard with provided configuration.
   * Steps: WiFi (1) → API Keys (2) → Location (3) → Hardware (4) → Review & Connect (5).
   */
  async completeSetup(config: {
    wifi: { ssid: string; password: string };
    api: {
      openSkyClientId: string;
      openSkyClientSecret: string;
      aeroApiKey: string;
    };
    location: { lat: number; lon: number; radiusKm: number };
    hardware: { tilesX: number; tilesY: number; tilePixels: number; displayPin?: number };
  }): Promise<void> {
    // Step 1: WiFi
    await this.configureWifi(config.wifi.ssid, config.wifi.password);
    await this.goNext();

    // Step 2: API Credentials
    await this.configureApiCredentials(
      config.api.openSkyClientId,
      config.api.openSkyClientSecret,
      config.api.aeroApiKey
    );
    await this.goNext();

    // Step 3: Location
    await this.configureLocation(
      config.location.lat,
      config.location.lon,
      config.location.radiusKm
    );
    await this.goNext();

    // Step 4: Hardware
    await this.configureMatrix(
      config.hardware.tilesX,
      config.hardware.tilesY,
      config.hardware.tilePixels
    );
    if (config.hardware.displayPin !== undefined) {
      await this.setDisplayPin(config.hardware.displayPin);
    }
    await this.goNext();

    // Step 5: Review & Connect — click "Save & Connect" to complete
    await this.completeWizard();
  }

  // ============================================================================
  // Assertions
  // ============================================================================

  /**
   * Assert the wizard is on a specific step.
   */
  async expectStep(step: number): Promise<void> {
    const currentStep = await this.getCurrentStep();
    expect(currentStep).toBe(step);
  }

  /**
   * Assert WiFi scan results are displayed and contain at least one network.
   */
  async expectNetworksVisible(): Promise<void> {
    await expect(this.wifiNetworkList).toBeVisible();
    // Scan rows are .scan-row divs rendered by showScanResults() in wizard.js
    const networks = this.wifiNetworkList.locator('.scan-row');
    const count = await networks.count();
    expect(count).toBeGreaterThan(0);
  }

  /**
   * Assert the preview canvas is rendering.
   */
  async expectPreviewVisible(): Promise<void> {
    await expect(this.previewCanvas).toBeVisible();
  }
}
