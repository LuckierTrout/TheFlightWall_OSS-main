/**
 * FlightWall E2E Test Configuration
 *
 * Playwright configuration optimized for testing the FlightWall web dashboard.
 * Tests can run against:
 * - Mock server (default for CI/dev) - localhost:3000
 * - Real device - via FLIGHTWALL_BASE_URL env var
 *
 * @see https://playwright.dev/docs/test-configuration
 */
import { defineConfig, devices } from '@playwright/test';

// Use environment variables for flexible test targeting
const BASE_URL = process.env.FLIGHTWALL_BASE_URL || 'http://localhost:3000';
const IS_CI = !!process.env.CI;

export default defineConfig({
  // Test file discovery
  testDir: './tests',
  testMatch: '**/*.spec.ts',

  // Parallel execution settings
  fullyParallel: true,
  workers: IS_CI ? 1 : 4,

  // Fail fast in CI to save resources
  forbidOnly: IS_CI,
  retries: IS_CI ? 2 : 0,

  // Reporter configuration
  reporter: [
    ['list'],
    ['html', { open: 'never' }],
    ...(IS_CI ? [['github'] as const] : []),
  ],

  // Global timeout settings
  timeout: 30_000,
  expect: {
    timeout: 5_000,
  },

  // Shared settings for all projects
  use: {
    baseURL: BASE_URL,
    trace: 'on-first-retry',
    screenshot: 'only-on-failure',
    video: 'retain-on-failure',

    // ESP32 web server may be slower than typical web apps
    navigationTimeout: 15_000,
    actionTimeout: 10_000,

    // FlightWall-specific test attributes
    testIdAttribute: 'data-testid',
  },

  // Browser projects
  projects: [
    // Desktop browsers - primary targets
    {
      name: 'chromium',
      use: { ...devices['Desktop Chrome'] },
    },
    {
      name: 'firefox',
      use: { ...devices['Desktop Firefox'] },
    },
    {
      name: 'webkit',
      use: { ...devices['Desktop Safari'] },
    },

    // Mobile browsers - secondary targets (FlightWall is primarily desktop)
    {
      name: 'mobile-chrome',
      use: { ...devices['Pixel 5'] },
    },
    {
      name: 'mobile-safari',
      use: { ...devices['iPhone 13'] },
    },
  ],

  // Local development server (mock server)
  webServer: process.env.FLIGHTWALL_BASE_URL
    ? undefined
    : {
        command: 'npx tsx mock-server/server.ts',
        url: 'http://localhost:3000/api/status',
        reuseExistingServer: !IS_CI,
        timeout: 120_000,
        stdout: 'pipe',
        stderr: 'pipe',
      },

  // Output directories
  outputDir: 'test-results',
});
