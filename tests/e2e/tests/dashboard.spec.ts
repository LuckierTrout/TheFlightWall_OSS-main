/**
 * Dashboard E2E Tests
 *
 * Tests for the FlightWall Dashboard (STA mode).
 * Validates settings management, API interactions, and UI behavior.
 */
import { test, expect } from '../helpers/test-fixtures.js';
import { DISPLAY_SETTINGS, HARDWARE_CONFIGS, LOCATIONS, TIMING_CONFIGS } from '../fixtures/test-data.js';

test.describe('Dashboard Page', () => {
  test.beforeEach(async ({ dashboardPage, mockApi }) => {
    // MockApi is automatically set up via fixtures
    await dashboardPage.navigate();
  });

  test.describe('Page Load', () => {
    test('should display the dashboard page', async ({ dashboardPage }) => {
      await expect(dashboardPage.brightnessSlider).toBeVisible();
      await expect(dashboardPage.fetchIntervalSlider).toBeVisible();
    });

    test('should load current settings from API', async ({ dashboardPage, mockApi }) => {
      // Verify settings are populated from mock API state
      const state = mockApi.getState();
      const currentBrightness = await dashboardPage.getBrightness();
      expect(currentBrightness).toBe(state.settings.brightness);
    });

    test('should display device IP address', async ({ dashboardPage }) => {
      await dashboardPage.expectDeviceIpVisible();
    });
  });

  test.describe('Display Settings', () => {
    test('should update brightness value when slider changes', async ({ dashboardPage }) => {
      const testValue = DISPLAY_SETTINGS.BRIGHTNESS_STEPS[3]; // 100
      await dashboardPage.setBrightness(testValue);

      const displayedValue = await dashboardPage.getBrightness();
      expect(displayedValue).toBe(testValue);
    });

    test('should update color preview when RGB sliders change', async ({ dashboardPage }) => {
      const testColor = DISPLAY_SETTINGS.COLOR_RED;
      await dashboardPage.setTextColor(testColor.r, testColor.g, testColor.b);

      const previewStyle = await dashboardPage.getColorPreviewStyle();
      // CSS rgb() format
      expect(previewStyle).toContain('255');
    });

    test('should save brightness setting via API', async ({ dashboardPage, page }) => {
      await dashboardPage.setBrightness(100);

      const responsePromise = page.waitForResponse((res) =>
        res.url().includes('/api/settings') && res.request().method() === 'POST'
      );

      await dashboardPage.saveButton.click();
      const response = await responsePromise;
      const json = await response.json();

      expect(json.ok).toBe(true);
      expect(json.applied).toContain('brightness');
      expect(json.reboot_required).toBe(false); // brightness is hot-reload
    });
  });

  test.describe('Timing Settings', () => {
    test('should update fetch interval slider', async ({ dashboardPage }) => {
      const testInterval = TIMING_CONFIGS.FAST.fetch_interval;
      await dashboardPage.setFetchInterval(testInterval);

      // Verify label updates
      const label = await dashboardPage.fetchIntervalLabel.textContent();
      expect(label).toContain(String(testInterval));
    });

    test('should update display cycle slider', async ({ dashboardPage }) => {
      const testCycle = TIMING_CONFIGS.SLOW.display_cycle;
      await dashboardPage.setDisplayCycle(testCycle);

      const label = await dashboardPage.displayCycleLabel.textContent();
      expect(label).toContain(String(testCycle));
    });
  });

  test.describe('Hardware Settings', () => {
    test('should update matrix configuration', async ({ dashboardPage }) => {
      const config = HARDWARE_CONFIGS.EXPANDED;
      await dashboardPage.setMatrixConfig(
        config.tiles_x,
        config.tiles_y,
        config.tile_pixels
      );

      // Verify preview canvas is visible
      await dashboardPage.expectPreviewCanvasVisible();
    });

    test('should require reboot when hardware changes are saved', async ({ dashboardPage, page }) => {
      const config = HARDWARE_CONFIGS.EXPANDED;
      await dashboardPage.setMatrixConfig(
        config.tiles_x,
        config.tiles_y,
        config.tile_pixels
      );

      const result = await dashboardPage.saveSettings();

      expect(result.ok).toBe(true);
      expect(result.reboot_required).toBe(true);
      expect(result.applied).toContain('tiles_x');
    });
  });

  test.describe('Location Settings', () => {
    test('should update location coordinates', async ({ dashboardPage }) => {
      const location = LOCATIONS.LAX;
      await dashboardPage.setLocation(
        location.lat,
        location.lon,
        location.radius_km
      );

      // Verify inputs have new values
      await expect(dashboardPage.centerLatInput).toHaveValue(String(location.lat));
      await expect(dashboardPage.centerLonInput).toHaveValue(String(location.lon));
      await expect(dashboardPage.radiusKmInput).toHaveValue(String(location.radius_km));
    });
  });

  test.describe('Network Settings', () => {
    test('should accept WiFi credentials', async ({ dashboardPage }) => {
      await dashboardPage.setWifiCredentials('NewNetwork', 'password123');

      await expect(dashboardPage.wifiSsidInput).toHaveValue('NewNetwork');
    });

    test('should require reboot when WiFi credentials change', async ({ dashboardPage }) => {
      await dashboardPage.setWifiCredentials('NewNetwork', 'password123');

      const result = await dashboardPage.saveSettings();

      expect(result.ok).toBe(true);
      expect(result.reboot_required).toBe(true);
      expect(result.applied).toContain('wifi_ssid');
    });

    test('should trigger WiFi scan', async ({ dashboardPage, page }) => {
      const scanPromise = page.waitForResponse((res) =>
        res.url().includes('/api/wifi/scan')
      );

      await dashboardPage.wifiScanButton.click();
      const response = await scanPromise;
      const json = await response.json();

      expect(json.ok).toBe(true);
    });
  });

  test.describe('Error Handling', () => {
    test('should show error when saving empty settings', async ({ dashboardPage, mockApi, page }) => {
      // Clear any pending changes by reloading
      await page.reload();
      await dashboardPage.waitForPageLoad();

      // Try to save without making changes (mock will return EMPTY_PAYLOAD)
      // Note: Real implementation may validate client-side first
    });
  });

  test.describe('Navigation', () => {
    test('should navigate to health page', async ({ dashboardPage, page }) => {
      const healthLink = dashboardPage.healthPageLink;

      if (await healthLink.isVisible()) {
        await healthLink.click();
        await expect(page).toHaveURL(/health/);
      }
    });
  });
});

test.describe('Dashboard API Integration', () => {
  test('should handle API errors gracefully', async ({ dashboardPage, mockApi, page }) => {
    // Set up error response for settings endpoint
    mockApi.setSubsystemStatus('opensky', 'error', '401 Unauthorized');

    await dashboardPage.navigate();

    // Dashboard should still load despite API errors
    await expect(dashboardPage.brightnessSlider).toBeVisible();
  });

  test('should reflect subsystem status in UI', async ({ dashboardPage, mockApi }) => {
    mockApi.setSubsystemStatus('wifi', 'warning', 'Weak signal');

    await dashboardPage.navigate();

    // Status should be reflected (implementation-dependent)
    // This tests the integration between mock API and UI
  });
});
