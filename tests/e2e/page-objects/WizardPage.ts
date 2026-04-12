/**
 * Wizard Page Object
 *
 * Page object for the FlightWall Setup Wizard (AP mode).
 * Handles the multi-step onboarding flow for initial device configuration.
 *
 * Architecture Reference:
 * - Wizard served in AP mode (device acts as access point)
 * - Steps: WiFi → API Credentials → Location → Hardware → Test → Complete
 * - Reboot required after wizard completion
 */
import { type Locator, expect } from '@playwright/test';
import { BasePage } from './BasePage.js';

export type WizardStep =
  | 'wifi'
  | 'api'
  | 'location'
  | 'hardware'
  | 'test'
  | 'complete';

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
    return this.page.locator('.step-indicator, .wizard-steps');
  }

  get nextButton(): Locator {
    return this.page.locator('button', { hasText: /next|continue/i });
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

  get tilePixelsSelect(): Locator {
    return this.page.locator('#tile-pixels, [name="tile_pixels"]');
  }

  get displayPinInput(): Locator {
    return this.page.locator('#display-pin, [name="display_pin"]');
  }

  get previewCanvas(): Locator {
    return this.page.locator('#preview-canvas, canvas');
  }

  // ============================================================================
  // Step 5: Test Display
  // ============================================================================

  get testPatternButton(): Locator {
    return this.page.locator('button', { hasText: /test pattern|show test/i });
  }

  get calibrationModeToggle(): Locator {
    return this.page.locator('#calibration-mode, [name="calibration"]');
  }

  // ============================================================================
  // Step 6: Complete
  // ============================================================================

  get completeMessage(): Locator {
    return this.page.locator('text=/setup complete|ready to use/i');
  }

  get rebootButton(): Locator {
    return this.page.locator('button', { hasText: /reboot|finish|start/i });
  }

  // ============================================================================
  // Settings Import/Export (Foundation Release)
  // ============================================================================

  get importSettingsButton(): Locator {
    return this.page.locator('button', { hasText: /import/i });
  }

  get importFileInput(): Locator {
    return this.page.locator('input[type="file"]');
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
    // Try to extract from step indicator or URL
    const indicator = await this.stepIndicator.textContent();
    const match = indicator?.match(/step\s*(\d+)/i);
    if (match) {
      return parseInt(match[1], 10);
    }
    // Fallback: check which step elements are visible
    if (await this.wifiSsidInput.isVisible()) return 1;
    if (await this.openSkyClientIdInput.isVisible()) return 2;
    if (await this.centerLatInput.isVisible()) return 3;
    if (await this.tilesXInput.isVisible()) return 4;
    if (await this.testPatternButton.isVisible()) return 5;
    if (await this.completeMessage.isVisible()) return 6;
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
   * Scan for available WiFi networks.
   */
  async scanForNetworks(): Promise<void> {
    await this.wifiScanButton.click();
    // Wait for scan to complete
    await this.page.waitForResponse(
      (res) => res.url().includes('/api/wifi/scan'),
      { timeout: 30_000 }
    );
    // Wait for results to render
    await expect(this.wifiNetworkList).toBeVisible();
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
    await this.tilePixelsSelect.selectOption(String(tilePixels));
  }

  /**
   * Set the LED data pin.
   */
  async setDisplayPin(pin: number): Promise<void> {
    await this.fillInput(this.displayPinInput, String(pin));
  }

  // ============================================================================
  // Step 5: Test Actions
  // ============================================================================

  /**
   * Trigger a test pattern on the display.
   */
  async showTestPattern(): Promise<void> {
    await this.testPatternButton.click();
    // Wait for pattern to be sent to device
    await this.page.waitForResponse(
      (res) =>
        res.url().includes('/api/calibration') ||
        res.url().includes('/api/test'),
      { timeout: 5_000 }
    );
  }

  // ============================================================================
  // Step 6: Complete Actions
  // ============================================================================

  /**
   * Complete the wizard and reboot the device.
   */
  async completeWizard(): Promise<void> {
    await expect(this.completeMessage).toBeVisible();
    await this.rebootButton.click();
    // Device will reboot, connection will be lost
    await this.page.waitForResponse(
      (res) => res.url().includes('/api/reboot'),
      { timeout: 5_000 }
    );
  }

  // ============================================================================
  // Full Wizard Flow
  // ============================================================================

  /**
   * Complete the entire wizard with provided configuration.
   */
  async completeSetup(config: {
    wifi: { ssid: string; password: string };
    api: {
      openSkyClientId: string;
      openSkyClientSecret: string;
      aeroApiKey: string;
    };
    location: { lat: number; lon: number; radiusKm: number };
    hardware: { tilesX: number; tilesY: number; tilePixels: number };
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
    await this.goNext();

    // Step 5: Test (optional, skip for speed)
    await this.skipStep();

    // Step 6: Complete
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
   * Assert WiFi scan results are displayed.
   */
  async expectNetworksVisible(): Promise<void> {
    await expect(this.wifiNetworkList).toBeVisible();
    const networks = this.wifiNetworkList.locator('li, .network-item');
    await expect(networks).toHaveCount(
      expect.any(Number) as unknown as number
    );
  }

  /**
   * Assert the preview canvas is rendering.
   */
  async expectPreviewVisible(): Promise<void> {
    await expect(this.previewCanvas).toBeVisible();
  }
}
