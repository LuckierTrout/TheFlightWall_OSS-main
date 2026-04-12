/**
 * Health Page Object
 *
 * Page object for the FlightWall System Health page.
 * Provides visibility into subsystem status, device metrics, and diagnostics.
 *
 * Architecture Reference:
 * - SystemStatus registry provides subsystem health
 * - Subsystems: WiFi, OpenSky, AeroAPI, CDN, NVS, LittleFS
 * - Status levels: OK, Warning, Error
 */
import { type Locator, expect } from '@playwright/test';
import { BasePage } from './BasePage.js';

export type SubsystemName =
  | 'wifi'
  | 'opensky'
  | 'aeroapi'
  | 'cdn'
  | 'nvs'
  | 'littlefs';

export type StatusLevel = 'ok' | 'warning' | 'error';

export class HealthPage extends BasePage {
  // ============================================================================
  // Page Identification
  // ============================================================================

  get path(): string {
    return '/health.html';
  }

  get pageIdentifier(): Locator {
    return this.page.locator('text=System Health');
  }

  // ============================================================================
  // Refresh Controls
  // ============================================================================

  get refreshButton(): Locator {
    return this.page.locator('#refresh-btn, button', { hasText: /refresh/i });
  }

  get lastUpdatedTime(): Locator {
    return this.page.locator('#last-updated, .last-updated');
  }

  // ============================================================================
  // Subsystem Status Elements
  // ============================================================================

  get wifiStatus(): Locator {
    return this.page.locator('#wifi-status, [data-subsystem="wifi"]');
  }

  get openSkyStatus(): Locator {
    return this.page.locator('#opensky-status, [data-subsystem="opensky"]');
  }

  get aeroApiStatus(): Locator {
    return this.page.locator('#aeroapi-status, [data-subsystem="aeroapi"]');
  }

  get cdnStatus(): Locator {
    return this.page.locator('#cdn-status, [data-subsystem="cdn"]');
  }

  get nvsStatus(): Locator {
    return this.page.locator('#nvs-status, [data-subsystem="nvs"]');
  }

  get littleFsStatus(): Locator {
    return this.page.locator('#littlefs-status, [data-subsystem="littlefs"]');
  }

  // ============================================================================
  // Device Metrics Elements
  // ============================================================================

  get heapFreeDisplay(): Locator {
    return this.page.locator('#heap-free, .heap-free');
  }

  get heapTotalDisplay(): Locator {
    return this.page.locator('#heap-total, .heap-total');
  }

  get uptimeDisplay(): Locator {
    return this.page.locator('#uptime, .uptime');
  }

  get firmwareVersionDisplay(): Locator {
    return this.page.locator('#firmware-version, .firmware-version');
  }

  // ============================================================================
  // WiFi Details Elements
  // ============================================================================

  get wifiIpDisplay(): Locator {
    return this.page.locator('#wifi-ip, .wifi-ip');
  }

  get wifiRssiDisplay(): Locator {
    return this.page.locator('#wifi-rssi, .wifi-rssi');
  }

  get wifiSsidDisplay(): Locator {
    return this.page.locator('#wifi-ssid, .wifi-ssid');
  }

  get wifiModeDisplay(): Locator {
    return this.page.locator('#wifi-mode, .wifi-mode');
  }

  // ============================================================================
  // Flight Info Elements
  // ============================================================================

  get flightCountDisplay(): Locator {
    return this.page.locator('#flight-count, .flight-count');
  }

  get lastFetchTimeDisplay(): Locator {
    return this.page.locator('#last-fetch, .last-fetch');
  }

  get currentFlightIndexDisplay(): Locator {
    return this.page.locator('#current-index, .current-index');
  }

  // ============================================================================
  // API Quota Elements
  // ============================================================================

  get aeroApiCallsDisplay(): Locator {
    return this.page.locator('#aeroapi-calls, .aeroapi-calls');
  }

  get aeroApiLimitDisplay(): Locator {
    return this.page.locator('#aeroapi-limit, .aeroapi-limit');
  }

  get aeroApiUsageBar(): Locator {
    return this.page.locator('#aeroapi-usage, .usage-bar');
  }

  // ============================================================================
  // Actions
  // ============================================================================

  /**
   * Refresh the health status data.
   */
  async refresh(): Promise<void> {
    const responsePromise = this.page.waitForResponse((res) =>
      res.url().includes('/api/status')
    );
    await this.refreshButton.click();
    await responsePromise;
    // Wait for UI to update
    await this.page.waitForTimeout(500);
  }

  /**
   * Get the status locator for a specific subsystem.
   */
  getSubsystemStatus(subsystem: SubsystemName): Locator {
    const mapping: Record<SubsystemName, Locator> = {
      wifi: this.wifiStatus,
      opensky: this.openSkyStatus,
      aeroapi: this.aeroApiStatus,
      cdn: this.cdnStatus,
      nvs: this.nvsStatus,
      littlefs: this.littleFsStatus,
    };
    return mapping[subsystem];
  }

  /**
   * Get the status level of a subsystem (ok, warning, error).
   */
  async getSubsystemLevel(subsystem: SubsystemName): Promise<StatusLevel> {
    const statusElement = this.getSubsystemStatus(subsystem);
    const classes = await statusElement.getAttribute('class');

    if (classes?.includes('error') || classes?.includes('status-error')) {
      return 'error';
    }
    if (classes?.includes('warning') || classes?.includes('status-warning')) {
      return 'warning';
    }
    return 'ok';
  }

  /**
   * Get the status message for a subsystem.
   */
  async getSubsystemMessage(subsystem: SubsystemName): Promise<string> {
    const statusElement = this.getSubsystemStatus(subsystem);
    const messageElement = statusElement.locator('.message, .status-message');
    if (await messageElement.isVisible()) {
      return (await messageElement.textContent()) ?? '';
    }
    // Fallback: extract from element text
    return (await statusElement.textContent()) ?? '';
  }

  // ============================================================================
  // Device Metrics Helpers
  // ============================================================================

  /**
   * Get the current free heap in bytes.
   */
  async getFreeHeap(): Promise<number> {
    const text = await this.heapFreeDisplay.textContent();
    // Parse values like "180KB" or "180000"
    const match = text?.match(/(\d+(?:\.\d+)?)\s*(KB|MB|B)?/i);
    if (!match) return 0;

    let value = parseFloat(match[1]);
    const unit = match[2]?.toUpperCase();

    if (unit === 'KB') value *= 1024;
    if (unit === 'MB') value *= 1024 * 1024;

    return Math.floor(value);
  }

  /**
   * Get the uptime in seconds.
   */
  async getUptimeSeconds(): Promise<number> {
    const text = await this.uptimeDisplay.textContent();
    // Parse formats like "1d 2h 30m" or "86400s" or "1 day, 2 hours"
    let totalSeconds = 0;

    const dayMatch = text?.match(/(\d+)\s*d(ay)?s?/i);
    const hourMatch = text?.match(/(\d+)\s*h(our)?s?/i);
    const minMatch = text?.match(/(\d+)\s*m(in(ute)?)?s?/i);
    const secMatch = text?.match(/(\d+)\s*s(ec(ond)?)?s?/i);

    if (dayMatch) totalSeconds += parseInt(dayMatch[1], 10) * 86400;
    if (hourMatch) totalSeconds += parseInt(hourMatch[1], 10) * 3600;
    if (minMatch) totalSeconds += parseInt(minMatch[1], 10) * 60;
    if (secMatch) totalSeconds += parseInt(secMatch[1], 10);

    return totalSeconds;
  }

  /**
   * Get the firmware version string.
   */
  async getFirmwareVersion(): Promise<string> {
    return (await this.firmwareVersionDisplay.textContent())?.trim() ?? '';
  }

  // ============================================================================
  // Assertions
  // ============================================================================

  /**
   * Assert a subsystem has a specific status level.
   */
  async expectSubsystemStatus(
    subsystem: SubsystemName,
    expectedLevel: StatusLevel
  ): Promise<void> {
    const level = await this.getSubsystemLevel(subsystem);
    expect(level).toBe(expectedLevel);
  }

  /**
   * Assert all subsystems are healthy (no errors).
   */
  async expectAllSubsystemsHealthy(): Promise<void> {
    const subsystems: SubsystemName[] = [
      'wifi',
      'opensky',
      'aeroapi',
      'cdn',
      'nvs',
      'littlefs',
    ];

    for (const subsystem of subsystems) {
      const level = await this.getSubsystemLevel(subsystem);
      expect(level, `${subsystem} should not be in error state`).not.toBe(
        'error'
      );
    }
  }

  /**
   * Assert free heap is above a minimum threshold.
   */
  async expectMinimumHeap(minBytes: number): Promise<void> {
    const freeHeap = await this.getFreeHeap();
    expect(freeHeap).toBeGreaterThanOrEqual(minBytes);
  }

  /**
   * Assert the firmware version matches a pattern.
   */
  async expectFirmwareVersion(pattern: RegExp): Promise<void> {
    const version = await this.getFirmwareVersion();
    expect(version).toMatch(pattern);
  }

  /**
   * Assert WiFi is connected in STA mode.
   */
  async expectWifiConnected(): Promise<void> {
    await this.expectSubsystemStatus('wifi', 'ok');
    const mode = await this.wifiModeDisplay.textContent();
    expect(mode?.toLowerCase()).toContain('sta');
  }

  /**
   * Assert API quota is within limits.
   */
  async expectApiQuotaAvailable(): Promise<void> {
    const calls = await this.aeroApiCallsDisplay.textContent();
    const limit = await this.aeroApiLimitDisplay.textContent();

    const callsNum = parseInt(calls ?? '0', 10);
    const limitNum = parseInt(limit ?? '0', 10);

    expect(callsNum).toBeLessThan(limitNum);
  }

  /**
   * Assert flights are being tracked.
   */
  async expectFlightsTracked(): Promise<void> {
    const countText = await this.flightCountDisplay.textContent();
    const count = parseInt(countText ?? '0', 10);
    expect(count).toBeGreaterThan(0);
  }
}
