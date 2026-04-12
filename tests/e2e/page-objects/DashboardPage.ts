/**
 * Dashboard Page Object
 *
 * Page object for the FlightWall Dashboard (STA mode).
 * Provides methods for interacting with all dashboard settings cards.
 *
 * Architecture Reference:
 * - Dashboard served in STA mode (connected to WiFi)
 * - Settings applied via POST /api/settings
 * - Hot-reload keys take effect immediately
 * - Reboot-required keys trigger device restart
 */
import { type Locator, expect } from '@playwright/test';
import { BasePage } from './BasePage.js';

export class DashboardPage extends BasePage {
  // ============================================================================
  // Page Identification
  // ============================================================================

  get path(): string {
    return '/';
  }

  get pageIdentifier(): Locator {
    return this.page.locator('title', { hasText: 'FlightWall' });
  }

  // ============================================================================
  // Display Card Elements
  // ============================================================================

  get brightnessSlider(): Locator {
    return this.page.locator('#brightness');
  }

  get brightnessValue(): Locator {
    return this.page.locator('#brightness-val');
  }

  get colorRSlider(): Locator {
    return this.page.locator('#color-r');
  }

  get colorGSlider(): Locator {
    return this.page.locator('#color-g');
  }

  get colorBSlider(): Locator {
    return this.page.locator('#color-b');
  }

  get colorPreview(): Locator {
    return this.page.locator('#color-preview');
  }

  get deviceIp(): Locator {
    return this.page.locator('#device-ip');
  }

  // ============================================================================
  // Timing Card Elements
  // ============================================================================

  get fetchIntervalSlider(): Locator {
    return this.page.locator('#fetch-interval');
  }

  get fetchIntervalLabel(): Locator {
    return this.page.locator('#fetch-label');
  }

  get fetchEstimate(): Locator {
    return this.page.locator('#fetch-estimate');
  }

  get displayCycleSlider(): Locator {
    return this.page.locator('#display-cycle');
  }

  get displayCycleLabel(): Locator {
    return this.page.locator('#cycle-label');
  }

  // ============================================================================
  // Network & API Card Elements
  // ============================================================================

  get wifiSsidInput(): Locator {
    return this.page.locator('#d-wifi-ssid');
  }

  get wifiPasswordInput(): Locator {
    return this.page.locator('#d-wifi-pass');
  }

  get openSkyClientIdInput(): Locator {
    return this.page.locator('#d-os-client-id');
  }

  get openSkyClientSecretInput(): Locator {
    return this.page.locator('#d-os-client-sec');
  }

  get aeroApiKeyInput(): Locator {
    return this.page.locator('#d-aeroapi-key');
  }

  get wifiScanButton(): Locator {
    return this.page.locator('#d-btn-scan');
  }

  get wifiScanResults(): Locator {
    return this.page.locator('#d-scan-results');
  }

  // ============================================================================
  // Hardware Card Elements
  // ============================================================================

  get tilesXInput(): Locator {
    return this.page.locator('#d-tiles-x');
  }

  get tilesYInput(): Locator {
    return this.page.locator('#d-tiles-y');
  }

  get tilePixelsSelect(): Locator {
    return this.page.locator('#d-tile-pixels');
  }

  get displayPinInput(): Locator {
    return this.page.locator('#d-display-pin');
  }

  get originCornerSelect(): Locator {
    return this.page.locator('#d-origin-corner');
  }

  get scanDirectionSelect(): Locator {
    return this.page.locator('#d-scan-dir');
  }

  get zigzagCheckbox(): Locator {
    return this.page.locator('#d-zigzag');
  }

  get previewCanvas(): Locator {
    return this.page.locator('#preview-canvas');
  }

  // ============================================================================
  // Location Card Elements
  // ============================================================================

  get centerLatInput(): Locator {
    return this.page.locator('#d-center-lat');
  }

  get centerLonInput(): Locator {
    return this.page.locator('#d-center-lon');
  }

  get radiusKmInput(): Locator {
    return this.page.locator('#d-radius-km');
  }

  // ============================================================================
  // Action Buttons
  // ============================================================================

  get saveButton(): Locator {
    return this.page.locator('button', { hasText: /save|apply/i });
  }

  get rebootButton(): Locator {
    return this.page.locator('button', { hasText: /reboot/i });
  }

  get resetButton(): Locator {
    return this.page.locator('button', { hasText: /reset|factory/i });
  }

  // ============================================================================
  // Navigation Links
  // ============================================================================

  get healthPageLink(): Locator {
    return this.page.locator('a[href*="health"]');
  }

  // ============================================================================
  // Display Settings Actions
  // ============================================================================

  /**
   * Set the brightness value (0-255).
   */
  async setBrightness(value: number): Promise<void> {
    await this.setSliderValue(this.brightnessSlider, value);
  }

  /**
   * Get the current brightness value.
   */
  async getBrightness(): Promise<number> {
    const valueText = await this.brightnessValue.textContent();
    return parseInt(valueText ?? '0', 10);
  }

  /**
   * Set the text color RGB values.
   */
  async setTextColor(r: number, g: number, b: number): Promise<void> {
    await this.setSliderValue(this.colorRSlider, r);
    await this.setSliderValue(this.colorGSlider, g);
    await this.setSliderValue(this.colorBSlider, b);
  }

  /**
   * Get the color preview background color.
   */
  async getColorPreviewStyle(): Promise<string> {
    return this.colorPreview.evaluate((el) =>
      window.getComputedStyle(el).backgroundColor
    );
  }

  // ============================================================================
  // Timing Settings Actions
  // ============================================================================

  /**
   * Set the fetch interval in seconds.
   */
  async setFetchInterval(seconds: number): Promise<void> {
    await this.setSliderValue(this.fetchIntervalSlider, seconds);
  }

  /**
   * Set the display cycle duration in seconds.
   */
  async setDisplayCycle(seconds: number): Promise<void> {
    await this.setSliderValue(this.displayCycleSlider, seconds);
  }

  // ============================================================================
  // Network Settings Actions
  // ============================================================================

  /**
   * Set WiFi credentials.
   */
  async setWifiCredentials(ssid: string, password: string): Promise<void> {
    await this.fillInput(this.wifiSsidInput, ssid);
    await this.fillInput(this.wifiPasswordInput, password);
  }

  /**
   * Set OpenSky API credentials.
   */
  async setOpenSkyCredentials(
    clientId: string,
    clientSecret: string
  ): Promise<void> {
    await this.fillInput(this.openSkyClientIdInput, clientId);
    await this.fillInput(this.openSkyClientSecretInput, clientSecret);
  }

  /**
   * Set AeroAPI key.
   */
  async setAeroApiKey(apiKey: string): Promise<void> {
    await this.fillInput(this.aeroApiKeyInput, apiKey);
  }

  /**
   * Trigger a WiFi network scan.
   */
  async scanForNetworks(): Promise<void> {
    await this.wifiScanButton.click();
    // Wait for scan results to populate
    await this.page.waitForResponse((res) =>
      res.url().includes('/api/wifi/scan')
    );
  }

  // ============================================================================
  // Hardware Settings Actions
  // ============================================================================

  /**
   * Set the matrix tile configuration.
   */
  async setMatrixConfig(
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

  /**
   * Configure matrix wiring (origin, scan direction, zigzag).
   */
  async setMatrixWiring(
    originCorner: number,
    scanDirection: number,
    zigzag: boolean
  ): Promise<void> {
    await this.originCornerSelect.selectOption(String(originCorner));
    await this.scanDirectionSelect.selectOption(String(scanDirection));
    await this.zigzagCheckbox.setChecked(zigzag);
  }

  // ============================================================================
  // Location Settings Actions
  // ============================================================================

  /**
   * Set the geographic center point.
   */
  async setLocation(lat: number, lon: number, radiusKm: number): Promise<void> {
    await this.fillInput(this.centerLatInput, String(lat));
    await this.fillInput(this.centerLonInput, String(lon));
    await this.fillInput(this.radiusKmInput, String(radiusKm));
  }

  // ============================================================================
  // Form Actions
  // ============================================================================

  /**
   * Save current settings and wait for API response.
   * Returns the API response for verification.
   */
  async saveSettings(): Promise<{
    ok: boolean;
    applied?: string[];
    reboot_required?: boolean;
    error?: string;
  }> {
    const responsePromise = this.page.waitForResponse((res) =>
      res.url().includes('/api/settings')
    );
    await this.saveButton.click();
    const response = await responsePromise;
    return response.json();
  }

  /**
   * Request device reboot.
   */
  async rebootDevice(): Promise<void> {
    await this.rebootButton.click();
    // Expect confirmation dialog or immediate response
    await this.page.waitForResponse(
      (res) => res.url().includes('/api/reboot'),
      { timeout: 5_000 }
    );
  }

  /**
   * Request factory reset.
   */
  async factoryReset(): Promise<void> {
    await this.resetButton.click();
    // Should show confirmation dialog
    const confirmButton = this.page.locator('button', { hasText: /confirm/i });
    if (await confirmButton.isVisible()) {
      await confirmButton.click();
    }
    await this.page.waitForResponse(
      (res) => res.url().includes('/api/reset'),
      { timeout: 5_000 }
    );
  }

  // ============================================================================
  // Assertions
  // ============================================================================

  /**
   * Assert that settings were applied successfully.
   */
  async expectSettingsApplied(
    expectedKeys: string[],
    rebootRequired = false
  ): Promise<void> {
    const result = await this.saveSettings();
    expect(result.ok).toBe(true);
    expect(result.applied).toEqual(expect.arrayContaining(expectedKeys));
    expect(result.reboot_required).toBe(rebootRequired);
  }

  /**
   * Assert that the preview canvas shows valid dimensions.
   */
  async expectPreviewCanvasVisible(): Promise<void> {
    await expect(this.previewCanvas).toBeVisible();
    const boundingBox = await this.previewCanvas.boundingBox();
    expect(boundingBox?.width).toBeGreaterThan(0);
    expect(boundingBox?.height).toBeGreaterThan(0);
  }

  /**
   * Assert the device IP is displayed.
   */
  async expectDeviceIpVisible(): Promise<void> {
    await expect(this.deviceIp).toBeVisible();
    const ip = await this.deviceIp.textContent();
    expect(ip).toMatch(/\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/);
  }
}
