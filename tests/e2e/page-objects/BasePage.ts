/**
 * Base Page Object
 *
 * Abstract base class for all FlightWall page objects.
 * Provides common utilities for navigation, waiting, and element interaction.
 *
 * Design Principles:
 * - Encapsulate page structure and locators
 * - Provide fluent, chainable methods where appropriate
 * - Handle ESP32-specific timing considerations (slower than typical web apps)
 */
import { type Page, type Locator, expect } from '@playwright/test';

export abstract class BasePage {
  /**
   * Create a new page object instance.
   * @param page - Playwright Page object
   */
  constructor(protected readonly page: Page) {}

  // ============================================================================
  // Abstract Properties (must be implemented by subclasses)
  // ============================================================================

  /**
   * The URL path for this page (e.g., '/', '/health.html').
   */
  abstract get path(): string;

  /**
   * A unique element that identifies this page is loaded.
   * Used for waitForPageLoad() verification.
   */
  abstract get pageIdentifier(): Locator;

  // ============================================================================
  // Navigation
  // ============================================================================

  /**
   * Navigate to this page.
   */
  async navigate(): Promise<this> {
    await this.page.goto(this.path);
    await this.waitForPageLoad();
    return this;
  }

  /**
   * Wait for the page to be fully loaded and ready for interaction.
   * Includes network idle check and page identifier visibility.
   */
  async waitForPageLoad(): Promise<void> {
    await this.page.waitForLoadState('networkidle');
    await expect(this.pageIdentifier).toBeVisible({ timeout: 10_000 });
  }

  // ============================================================================
  // Element Locators
  // ============================================================================

  /**
   * Get element by data-testid attribute.
   */
  protected getByTestId(testId: string): Locator {
    return this.page.getByTestId(testId);
  }

  /**
   * Get element by role.
   */
  protected getByRole(
    role: Parameters<Page['getByRole']>[0],
    options?: Parameters<Page['getByRole']>[1]
  ): Locator {
    return this.page.getByRole(role, options);
  }

  /**
   * Get element by text content.
   */
  protected getByText(
    text: string | RegExp,
    options?: Parameters<Page['getByText']>[1]
  ): Locator {
    return this.page.getByText(text, options);
  }

  /**
   * Get element by label.
   */
  protected getByLabel(
    text: string | RegExp,
    options?: Parameters<Page['getByLabel']>[1]
  ): Locator {
    return this.page.getByLabel(text, options);
  }

  /**
   * Get element by placeholder text.
   */
  protected getByPlaceholder(
    text: string | RegExp,
    options?: Parameters<Page['getByPlaceholder']>[1]
  ): Locator {
    return this.page.getByPlaceholder(text, options);
  }

  // ============================================================================
  // Common Actions
  // ============================================================================

  /**
   * Fill an input field, clearing it first.
   */
  protected async fillInput(locator: Locator, value: string): Promise<void> {
    await locator.clear();
    await locator.fill(value);
  }

  /**
   * Set a slider (range input) to a specific value.
   * Handles the ESP32 web UI's slider behavior.
   */
  protected async setSliderValue(
    locator: Locator,
    value: number
  ): Promise<void> {
    // Playwright doesn't have native slider support, so we use evaluate
    await locator.evaluate((el: HTMLInputElement, val: number) => {
      el.value = String(val);
      el.dispatchEvent(new Event('input', { bubbles: true }));
      el.dispatchEvent(new Event('change', { bubbles: true }));
    }, value);
  }

  /**
   * Click a button and wait for any triggered network requests to complete.
   */
  protected async clickAndWait(
    locator: Locator,
    options?: { timeout?: number }
  ): Promise<void> {
    await Promise.all([
      this.page.waitForLoadState('networkidle', {
        timeout: options?.timeout ?? 10_000,
      }),
      locator.click(),
    ]);
  }

  // ============================================================================
  // Assertions
  // ============================================================================

  /**
   * Assert that a toast notification appears with the expected message.
   */
  async expectToast(
    message: string | RegExp,
    type: 'success' | 'error' | 'info' = 'success'
  ): Promise<void> {
    const toast = this.page.locator('.toast, [role="alert"]').filter({
      hasText: message,
    });
    await expect(toast).toBeVisible({ timeout: 5_000 });
  }

  /**
   * Assert that the page contains specific text.
   */
  async expectText(text: string | RegExp): Promise<void> {
    await expect(this.page.getByText(text)).toBeVisible();
  }

  // ============================================================================
  // API Interaction Helpers
  // ============================================================================

  /**
   * Wait for a specific API response.
   */
  async waitForApiResponse(
    urlPattern: string | RegExp,
    options?: { timeout?: number }
  ): Promise<{ ok: boolean; data?: unknown; error?: string }> {
    const response = await this.page.waitForResponse(
      (res) =>
        typeof urlPattern === 'string'
          ? res.url().includes(urlPattern)
          : urlPattern.test(res.url()),
      { timeout: options?.timeout ?? 10_000 }
    );
    return response.json();
  }

  /**
   * Intercept and mock an API endpoint.
   */
  async mockApiEndpoint(
    urlPattern: string | RegExp,
    response: object,
    options?: { status?: number }
  ): Promise<void> {
    await this.page.route(urlPattern, (route) => {
      route.fulfill({
        status: options?.status ?? 200,
        contentType: 'application/json',
        body: JSON.stringify(response),
      });
    });
  }

  // ============================================================================
  // Screenshot Helpers
  // ============================================================================

  /**
   * Take a screenshot for visual comparison.
   */
  async screenshot(name: string): Promise<Buffer> {
    return this.page.screenshot({
      fullPage: true,
      path: `test-results/screenshots/${name}.png`,
    });
  }
}
