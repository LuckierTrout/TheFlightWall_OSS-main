/**
 * Wizard Page Object
 *
 * Page object for the FlightWall Setup Wizard (AP mode).
 * Handles the multi-step onboarding flow for initial device configuration.
 *
 * Architecture Reference:
 * - Wizard served in AP mode (device acts as access point)
 * - Steps: WiFi (1) → API Keys (2) → Location (3) → Hardware (4) → Review (5) → Test Your Wall (6)
 * - Step 6 triggers positioning pattern, confirms layout, runs RGB test, then saves & reboots
 * - "Yes, it matches" on Step 6 completes the RGB test sequence, saves settings, triggers reboot
 */
import { type Locator, expect } from '@playwright/test';
import { BasePage } from './BasePage.js';

export type WizardStep =
  | 'wifi'
  | 'api'
  | 'location'
  | 'hardware'
  | 'review'
  | 'test';

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
    // Use ID — text is "Next →" for steps 1-5; nav bar hidden on step 6 (has its own buttons)
    return this.page.locator('#btn-next');
  }

  get backButton(): Locator {
    // Use exact ID — avoids matching "No — take me back" on Step 6 which also contains "back"
    return this.page.locator('#btn-back');
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

  get timezoneSelect(): Locator {
    return this.page.locator('#wizard-timezone');
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
    // Use the wizard Step 6 canvas ID; the wizard has no other canvas element
    return this.page.locator('#wizard-position-preview');
  }

  // ============================================================================
  // Step 5: Review
  // ============================================================================

  /** "Configuration Saved" heading shown after save + reboot handoff completes. */
  get completeMessage(): Locator {
    return this.page.locator('h1', { hasText: 'Configuration Saved' });
  }

  // ============================================================================
  // Step 6: Test Your Wall
  // ============================================================================

  get testPositionCanvas(): Locator {
    return this.page.locator('#wizard-position-preview');
  }

  get testYesButton(): Locator {
    return this.page.locator('#btn-test-yes');
  }

  get testNoButton(): Locator {
    return this.page.locator('#btn-test-no');
  }

  get testStatusText(): Locator {
    return this.page.locator('#wizard-test-status');
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
    // Primary: parse the "#progress" indicator text ("Step N of 6")
    const indicator = await this.stepIndicator.textContent();
    const match = indicator?.match(/step\s*(\d+)/i);
    if (match) {
      return parseInt(match[1], 10);
    }
    // Fallback: check which #step-N section is visible (6 steps total)
    for (let i = 1; i <= 6; i++) {
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
   * Select a timezone from the wizard-timezone dropdown.
   * @param iana - IANA timezone name (e.g. "America/Los_Angeles")
   */
  async selectTimezone(iana: string): Promise<void> {
    await this.timezoneSelect.selectOption(iana);
  }

  /**
   * Configure location manually, with optional timezone selection.
   */
  async configureLocation(
    lat: number,
    lon: number,
    radiusKm: number,
    timezone?: string
  ): Promise<void> {
    await this.fillInput(this.centerLatInput, String(lat));
    await this.fillInput(this.centerLonInput, String(lon));
    await this.fillInput(this.radiusKmInput, String(radiusKm));
    if (timezone !== undefined) {
      await this.selectTimezone(timezone);
    }
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
  // Step 5: Review Actions
  // ============================================================================

  // Step 5 is now a pass-through review step — clicking Next advances to Step 6.

  // ============================================================================
  // Step 6: Test Your Wall Actions
  // ============================================================================

  /**
   * On Step 6, confirm the layout matches ("Yes, it matches") to run the RGB color
   * test sequence and then save + reboot. Waits for the handoff message.
   */
  async completeWizard(): Promise<void> {
    // Step 6 shows the "Yes, it matches" primary button; nav bar is hidden.
    await expect(this.testYesButton).toBeVisible({ timeout: 5_000 });
    await this.testYesButton.click();
    // Wait for the handoff message — RGB test → save → reboot sequence completes
    await expect(this.completeMessage).toBeVisible({ timeout: 15_000 });
  }

  // ============================================================================
  // Full Wizard Flow
  // ============================================================================

  /**
   * Complete the entire 6-step wizard with provided configuration.
   * Steps: WiFi (1) → API Keys (2) → Location (3) → Hardware (4) → Review (5) → Test Your Wall (6).
   * On Step 6, clicks "Yes, it matches" to trigger RGB test → save → reboot.
   */
  async completeSetup(config: {
    wifi: { ssid: string; password: string };
    api: {
      openSkyClientId: string;
      openSkyClientSecret: string;
      aeroApiKey: string;
    };
    location: { lat: number; lon: number; radiusKm: number; timezone?: string };
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
      config.location.radiusKm,
      config.location.timezone
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

    // Step 5: Review — click Next to advance to Step 6
    await this.goNext();

    // Step 6: Test Your Wall — click "Yes, it matches" to complete
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
