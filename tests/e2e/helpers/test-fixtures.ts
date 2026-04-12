/**
 * Playwright Test Fixtures
 *
 * Custom Playwright test fixtures that extend the base test with
 * FlightWall-specific setup and teardown.
 */
import { test as base, type Page } from '@playwright/test';
import { DashboardPage, WizardPage, HealthPage } from '../page-objects/index.js';
import { MockApiManager } from './api-helpers.js';

// ============================================================================
// Fixture Types
// ============================================================================

export interface FlightWallFixtures {
  /** Dashboard page object (STA mode) */
  dashboardPage: DashboardPage;
  /** Wizard page object (AP mode) */
  wizardPage: WizardPage;
  /** Health page object */
  healthPage: HealthPage;
  /** Mock API manager for controlling API responses */
  mockApi: MockApiManager;
}

// ============================================================================
// Extended Test with FlightWall Fixtures
// ============================================================================

export const test = base.extend<FlightWallFixtures>({
  // Dashboard page object
  dashboardPage: async ({ page }, use) => {
    const dashboardPage = new DashboardPage(page);
    await use(dashboardPage);
  },

  // Wizard page object
  wizardPage: async ({ page }, use) => {
    const wizardPage = new WizardPage(page);
    await use(wizardPage);
  },

  // Health page object
  healthPage: async ({ page }, use) => {
    const healthPage = new HealthPage(page);
    await use(healthPage);
  },

  // Mock API manager with auto-setup
  mockApi: async ({ page }, use) => {
    const mockApi = new MockApiManager(page);
    await mockApi.setupRoutes();
    await use(mockApi);
  },
});

// Re-export expect for convenience
export { expect } from '@playwright/test';

// ============================================================================
// Additional Test Setup Helpers
// ============================================================================

/**
 * Configure a page for dashboard testing with mocked API.
 */
export async function setupDashboardTest(page: Page): Promise<{
  dashboard: DashboardPage;
  mockApi: MockApiManager;
}> {
  const mockApi = new MockApiManager(page);
  await mockApi.setupRoutes();

  const dashboard = new DashboardPage(page);
  await dashboard.navigate();

  return { dashboard, mockApi };
}

/**
 * Configure a page for wizard testing with mocked API.
 */
export async function setupWizardTest(page: Page): Promise<{
  wizard: WizardPage;
  mockApi: MockApiManager;
}> {
  const mockApi = new MockApiManager(page);
  await mockApi.setupRoutes();

  // Mock AP mode status
  mockApi.updateStatus({
    wifi_detail: {
      ip: '192.168.4.1',
      rssi: 0,
      ssid: 'FlightWall-Setup',
      mode: 'ap',
    },
  });

  const wizard = new WizardPage(page);
  await wizard.navigate();

  return { wizard, mockApi };
}

/**
 * Configure a page for health page testing with mocked API.
 */
export async function setupHealthTest(page: Page): Promise<{
  health: HealthPage;
  mockApi: MockApiManager;
}> {
  const mockApi = new MockApiManager(page);
  await mockApi.setupRoutes();

  const health = new HealthPage(page);
  await health.navigate();

  return { health, mockApi };
}

// ============================================================================
// Test Annotations
// ============================================================================

/**
 * Mark a test as requiring a real device connection.
 * These tests are skipped when running against mock server.
 */
export function requiresDevice() {
  return test.describe.configure({ mode: 'serial' });
}

/**
 * Mark a test as slow (increases timeout).
 */
export function slow() {
  return test.slow();
}
