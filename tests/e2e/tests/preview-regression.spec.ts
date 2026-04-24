/**
 * Preview Renderer Pixel Regression (Story LE-2.7)
 *
 * Goal: catch font/catalog drift between the browser-side preview renderer
 * (firmware/data-src/preview.js + preview-glyphs.js) and the firmware widget
 * renderer.
 *
 * Strategy: load the real preview.js + preview-glyphs.js into a minimal HTML
 * harness, invoke FWPreview.render() against a handful of canonical layouts
 * using deterministic mock flight data, and snapshot the resulting <canvas>
 * pixel output. Playwright's built-in toHaveScreenshot() manages the baseline
 * PNGs under tests/e2e/tests/preview-regression.spec.ts-snapshots/.
 *
 * Baseline workflow:
 *   - First run (or after intentional renderer change):
 *       cd tests/e2e && npm run test:update-snapshots -- preview-regression
 *     This records the baseline PNGs. Commit them alongside the code change.
 *   - Subsequent runs:
 *       cd tests/e2e && npm test -- preview-regression
 *     Fails if a canvas has drifted more than maxDiffPixels from its baseline.
 *
 * Why chromium-only:
 *   Pixel output is identical across browsers for the deterministic content
 *   we render (no antialiasing, nearest-neighbor upscale, fixed glyph tables).
 *   Restricting to chromium avoids baseline bloat. If cross-browser coverage
 *   becomes important, drop the `project` filter here.
 */
import { test, expect } from '@playwright/test';

const LAYOUTS = [
  {
    name: 'le2-catalog-all-fields',
    description:
      'Exercises every LE-2 catalog entry (aircraft_short, aircraft_full, ' +
      'origin_iata, destination_iata, speed_mph). Matches the fixture layout_d ' +
      'used by the firmware golden-frame test for structural parity.',
    layout: {
      id: 'pre_d',
      name: 'Preview D',
      v: 1,
      canvas: { w: 160, h: 32 },
      bg: '#000000',
      widgets: [
        {
          id: 'd1', type: 'flight_field',
          x: 0, y: 0, w: 80, h: 8,
          color: '#FFFFFF', content: 'aircraft_short',
        },
        {
          id: 'd2', type: 'flight_field',
          x: 0, y: 8, w: 80, h: 8,
          color: '#FFFFFF', content: 'aircraft_full',
        },
        {
          id: 'd3', type: 'flight_field',
          x: 0, y: 16, w: 40, h: 8,
          color: '#00FF00', content: 'origin_iata',
        },
        {
          id: 'd4', type: 'flight_field',
          x: 40, y: 16, w: 40, h: 8,
          color: '#00FF00', content: 'destination_iata',
        },
        {
          id: 'd5', type: 'metric',
          x: 80, y: 0, w: 64, h: 8,
          color: '#FFFF00', content: 'speed_mph',
        },
      ],
    },
  },
  {
    name: 'legacy-mixed',
    description:
      'Smoke layout covering a text + clock + legacy flight_field catalog ' +
      'entries. Catches drift in the non-LE-2 paths too.',
    layout: {
      id: 'pre_legacy',
      name: 'Preview Legacy',
      v: 1,
      canvas: { w: 160, h: 32 },
      bg: '#000000',
      widgets: [
        {
          id: 'l1', type: 'text',
          x: 2, y: 2, w: 80, h: 8,
          color: '#FFFFFF', content: 'FLIGHTWALL',
        },
        {
          id: 'l2', type: 'flight_field',
          x: 2, y: 12, w: 48, h: 8,
          color: '#FFFF00', content: 'airline',
        },
        {
          id: 'l3', type: 'flight_field',
          x: 2, y: 22, w: 48, h: 8,
          color: '#00FF00', content: 'origin_icao',
        },
      ],
    },
  },
];

const MOCK_FLIGHT = {
  ident: 'UAL123',
  ident_icao: 'UAL123',
  ident_iata: 'UA123',
  operator_code: 'UAL',
  operator_icao: 'UAL',
  operator_iata: 'UA',
  origin_icao: 'KSFO',
  origin_iata: 'SFO',
  destination_icao: 'KLAX',
  destination_iata: 'LAX',
  aircraft_code: 'B738',
  airline_display_name_full: 'United Airlines',
  aircraft_display_name_short: '737-800',
  aircraft_display_name_full: 'Boeing 737-800',
  altitude_kft: 34.0,
  speed_mph: 487.0,
  track_deg: 247.0,
  vertical_rate_fps: -12.5,
  distance_km: 18.2,
  bearing_deg: 213.0,
};

/**
 * Harness page: minimal HTML with a single <canvas> plus the real preview.js
 * + preview-glyphs.js loaded from the mock server. The harness exposes
 * window.__renderLayout(layout, flight) for the test to call. Pixel output
 * matches what the dashboard live-preview card renders for the same inputs.
 */
const HARNESS_HTML = `<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8" />
  <title>FWPreview Harness</title>
  <style>
    html, body { margin: 0; padding: 0; background: #000; }
    #preview {
      /* Nearest-neighbor upscale so each logical pixel is a sharp square. */
      image-rendering: pixelated;
      image-rendering: crisp-edges;
      display: block;
    }
  </style>
</head>
<body>
  <canvas id="preview" width="320" height="64"></canvas>
  <script src="/preview-glyphs.js"></script>
  <script src="/preview.js"></script>
  <script>
    // FWPreview.render takes a single opts object — see preview.js ~line 562.
    // We feed it the flat widget list plus matrix dimensions; it manages the
    // framebuffer + nearest-neighbor upscale onto the target canvas itself.
    window.__renderLayout = function (layout, flight) {
      var canvas = document.getElementById('preview');
      window.FWPreview.render({
        canvas: canvas,
        matrixW: layout.canvas.w,
        matrixH: layout.canvas.h,
        scale: 2, // 2x upscale keeps the screenshot small but still legible
        background: layout.bg || '#000000',
        flight: flight || null,
        widgets: layout.widgets || [],
      });
      return true;
    };
  </script>
</body>
</html>`;

test.describe('Preview renderer pixel regression', () => {
  // Chromium only — pixel-diff stability across browsers isn't worth the
  // baseline-bloat for this use case (see file-level comment).
  test.skip(({ browserName }) => browserName !== 'chromium',
    'Pixel-diff baseline is chromium-only by design');

  for (const spec of LAYOUTS) {
    test(spec.name, async ({ page }) => {
      // Serve the harness HTML inline so we don't ship a test-only file
      // in firmware/data-src/. preview.js + preview-glyphs.js are loaded
      // from the mock server by URL.
      await page.route('**/__preview_harness__', (route) => {
        route.fulfill({ status: 200, contentType: 'text/html', body: HARNESS_HTML });
      });
      await page.goto('/__preview_harness__');

      // Wait for FWPreview to register itself on window (preview.js is an IIFE
      // that publishes a global).
      await page.waitForFunction(() =>
        typeof (window as unknown as { FWPreview?: unknown }).FWPreview !== 'undefined',
      { timeout: 5_000 });

      const rendered = await page.evaluate(
        ({ layout, flight }) =>
          (window as unknown as {
            __renderLayout: (l: unknown, f: unknown) => boolean;
          }).__renderLayout(layout, flight),
        { layout: spec.layout, flight: MOCK_FLIGHT },
      );
      expect(rendered, `${spec.name}: __renderLayout returned truthy`).toBe(true);

      const canvas = page.locator('#preview');
      await expect(canvas).toHaveScreenshot(`${spec.name}.png`, {
        // Pixel-exact match by default. preview.js is deterministic for fixed
        // inputs (no antialiasing, no time-based animation). If the fonts or
        // catalog drift, this fails — which is exactly the gate we want.
        maxDiffPixels: 0,
      });
    });
  }
});
