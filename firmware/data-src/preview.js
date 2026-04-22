/*
 * FlightWall layout preview renderer (ES5, no deps).
 *
 * Goal: pixel-accurate preview of what the real LED wall will render for a
 * Custom Layout widget list + current flight data. Mirrors the firmware's
 * widget implementations (TextWidget, ClockWidget, MetricWidget,
 * FlightFieldWidget) so the editor can show edits before flashing.
 *
 * Text rendering follows Adafruit_GFX conventions:
 *   - Default 5x7 font (advanced at 6x8): GLCD_FONT is 256 glyphs of 5 column
 *     bytes, where each byte packs 7 pixel rows (LSB at the top).
 *   - TomThumb font (3x5 glyphs, 4x6 advance): packed bits, glyphs addressed
 *     by offset/width/height/xAdvance/xOffset/yOffset.
 *
 * Exposes a single global FWPreview:
 *   FWPreview.render(ctx, opts)  — paints one frame onto a <canvas>.
 *
 * Dependencies (must be loaded before this file):
 *   FWGlyphs (from preview-glyphs.js)
 */
(function (global) {
  'use strict';

  if (!global.FWGlyphs) {
    throw new Error('preview.js requires preview-glyphs.js to be loaded first');
  }

  var G = global.FWGlyphs;

  /* Firmware constants that matter for layout math. WIDGET_CHAR_W/H match the
     default 6x8 font used by widget renderers; text widgets advance 6px per
     glyph (5 visible + 1 blank spacer) and stand 8px tall cell-wise. */
  var WIDGET_CHAR_W = 6;
  var WIDGET_CHAR_H = 8;
  var LOGO_WIDTH = 32;
  var LOGO_HEIGHT = 32;

  /* ---------- Pixel framebuffer helpers ---------- */

  /* Create a logical W*H pixel buffer backed by a typed array. The preview
     paints into this buffer, then upscales onto the target canvas with
     nearest-neighbor interpolation so each logical pixel becomes a sharp
     colored square, matching how LED panels look. */
  function createFrame(w, h) {
    return { w: w, h: h, pixels: new Uint32Array(w * h) };
  }

  function clearFrame(frame, rgba) {
    frame.pixels.fill(rgba >>> 0);
  }

  function setPixel(frame, x, y, rgba) {
    if (x < 0 || y < 0 || x >= frame.w || y >= frame.h) return;
    frame.pixels[y * frame.w + x] = rgba >>> 0;
  }

  /* Pack r/g/b (0-255) into a little-endian RGBA8888 value that ImageData's
     Uint32 view will see as the pixel. On little-endian canvases (all common
     browsers on desktop/mobile) the byte order in memory is R, G, B, A — but
     the Uint32 interpretation is A<<24 | B<<16 | G<<8 | R. */
  function rgba(r, g, b) {
    return (
      (0xff << 24) |
      ((b & 0xff) << 16) |
      ((g & 0xff) << 8) |
      (r & 0xff)
    ) >>> 0;
  }

  /* Parse a CSS hex color into a packed RGBA value. Accepts both `#rgb`
     shorthand (each nibble doubled) and the full `#rrggbb` form — the
     shorthand path is important because some widgets and callers pass
     `#000` or `#fff` as defaults. Returns a fallback white on malformed
     input so a typo in a widget's color field never blanks the preview. */
  function parseHexColor(hex) {
    if (typeof hex !== 'string' || hex.charAt(0) !== '#') {
      return rgba(255, 255, 255);
    }
    var r, g, b;
    if (hex.length === 4) {
      // `#rgb` shorthand — duplicate each nibble to form a full byte.
      r = parseInt(hex.charAt(1), 16); r = (r << 4) | r;
      g = parseInt(hex.charAt(2), 16); g = (g << 4) | g;
      b = parseInt(hex.charAt(3), 16); b = (b << 4) | b;
    } else if (hex.length === 7) {
      r = parseInt(hex.substr(1, 2), 16);
      g = parseInt(hex.substr(3, 2), 16);
      b = parseInt(hex.substr(5, 2), 16);
    } else {
      return rgba(255, 255, 255);
    }
    if (isNaN(r) || isNaN(g) || isNaN(b)) return rgba(255, 255, 255);
    return rgba(r, g, b);
  }

  /* ---------- Default 5x7 font rendering (Adafruit GFX classic) ---------- */

  /* Draw a single glyph. Byte i of a glyph's 5-byte row holds column i. Bit 0
     of each byte is the topmost visible pixel; bits 0..6 are the 7 rows.
     Bit 7 is ignored in the default 5x7 behavior. The 6th column (right
     spacer) stays blank by convention — we don't emit it.

     `scale` integer-scales each pixel into a scale×scale square, matching
     Adafruit_GFX's `setTextSize(N)` semantics. `color` is a packed RGBA.
     Pixels that don't map into the frame are silently dropped so rendering
     is clip-safe. */
  function drawCharGlcd(frame, x, y, ch, color, scale) {
    var code = ch.charCodeAt(0) & 0xff;
    var base = code * 5;
    var s = scale || 1;
    for (var col = 0; col < 5; col++) {
      var bits = G.GLCD_FONT[base + col];
      for (var row = 0; row < 7; row++) {
        if (bits & (1 << row)) {
          // Paint the pixel as a scale×scale block so size > 1 produces
          // chunky scaled glyphs instead of interpolated ones.
          var px = x + col * s;
          var py = y + row * s;
          for (var sy = 0; sy < s; sy++) {
            for (var sx = 0; sx < s; sx++) {
              setPixel(frame, px + sx, py + sy, color);
            }
          }
        }
      }
    }
  }

  /* Draw a string left-to-right. Per-glyph advance is 6*scale px, line
     height is 8*scale px for \n. Matches how Adafruit_GFX's Print stream
     advances under the classic font + setTextSize(N). */
  function drawTextGlcd(frame, x, y, text, color, scale) {
    var s = scale || 1;
    var startX = x;
    for (var i = 0; i < text.length; i++) {
      var ch = text.charAt(i);
      if (ch === '\n') {
        x = startX;
        y += 8 * s;
        continue;
      }
      drawCharGlcd(frame, x, y, ch, color, s);
      x += 6 * s;
    }
  }

  /* ---------- TomThumb rendering (Adafruit_GFX custom font path) ---------- */

  /* Look up a TomThumb glyph tuple. Returns null for out-of-range codepoints
     so callers can skip or substitute. */
  function tomThumbGlyph(ch) {
    var code = ch.charCodeAt(0);
    if (code < G.TOMTHUMB_FIRST || code > G.TOMTHUMB_LAST) return null;
    return G.TOMTHUMB_GLYPHS[code - G.TOMTHUMB_FIRST];
  }

  /* Look up a Picopixel glyph tuple. Same packed layout as TomThumb —
     (offset, width, height, xAdvance, xOffset, yOffset). */
  function picopixelGlyph(ch) {
    var code = ch.charCodeAt(0);
    if (code < G.PICOPIXEL_FIRST || code > G.PICOPIXEL_LAST) return null;
    return G.PICOPIXEL_GLYPHS[code - G.PICOPIXEL_FIRST];
  }

  /* Shared GFX-custom-font renderer. TomThumb and Picopixel share the same
     bitmap packing (row-major bits, MSB first) and the same per-glyph tuple
     shape, so the render loop is identical — the only difference is which
     bitmap + glyph tables we look up from. */
  function drawTextGfxFont(frame, cursorX, cursorY, text, color, scale,
                           bitmaps, glyphLookup) {
    var s = scale || 1;
    for (var i = 0; i < text.length; i++) {
      var g = glyphLookup(text.charAt(i));
      if (g === null) {
        cursorX += 2 * s;
        continue;
      }
      var bitmapOffset = g[0], w = g[1], h = g[2];
      var xAdv = g[3], xOff = g[4], yOff = g[5];
      var bitIndex = 0;
      for (var yy = 0; yy < h; yy++) {
        for (var xx = 0; xx < w; xx++) {
          var byteIndex = bitmapOffset + (bitIndex >> 3);
          var bitInByte = 7 - (bitIndex & 7);
          var bit = (bitmaps[byteIndex] >> bitInByte) & 1;
          if (bit) {
            var px = cursorX + (xOff + xx) * s;
            var py = cursorY + (yOff + yy) * s;
            for (var sy = 0; sy < s; sy++) {
              for (var sx = 0; sx < s; sx++) {
                setPixel(frame, px + sx, py + sy, color);
              }
            }
          }
          bitIndex++;
        }
      }
      cursorX += xAdv * s;
    }
  }

  function drawTextPicopixel(frame, cursorX, cursorY, text, color, scale) {
    drawTextGfxFont(frame, cursorX, cursorY, text, color, scale,
                    G.PICOPIXEL_BITMAPS, picopixelGlyph);
  }

  /* TomThumb renderer — thin wrapper around the shared GFX-custom-font
     renderer. `cursorY` is the baseline Y (in logical pixels); glyphs
     extend upward from the baseline; descenders ('g','p','y') may push
     below. See drawTextGfxFont above for the render loop. */
  function drawTextTomThumb(frame, cursorX, cursorY, text, color, scale) {
    drawTextGfxFont(frame, cursorX, cursorY, text, color, scale,
                    G.TOMTHUMB_BITMAPS, tomThumbGlyph);
  }

  /* ---------- Layout engine (mirror of firmware/core/LayoutEngine.cpp) ---------- */

  /* Compute zone rectangles from hardware config. This mirrors the C++
     LayoutEngine::compute path exactly so a preview's zone math matches the
     device. Returns { matrixWidth, matrixHeight, logoZone, flightZone,
     telemetryZone, mode, valid }.

     Guards: tilesX/Y/tilePixels must be positive and matrix width must be
     >= height (the wall is oriented landscape). zone_layout=1 flips to the
     full-width-bottom layout where telemetry spans the whole bottom. */
  function computeLayout(hw) {
    var tilesX = hw.tiles_x >>> 0;
    var tilesY = hw.tiles_y >>> 0;
    var tilePixels = hw.tile_pixels >>> 0;
    var mw = tilesX * tilePixels;
    var mh = tilesY * tilePixels;

    if (tilesX === 0 || tilesY === 0 || tilePixels === 0 || mw < mh) {
      return {
        valid: false, mode: 'compact',
        matrixWidth: mw, matrixHeight: mh,
        logoZone: { x: 0, y: 0, w: 0, h: 0 },
        flightZone: { x: 0, y: 0, w: 0, h: 0 },
        telemetryZone: { x: 0, y: 0, w: 0, h: 0 },
      };
    }

    var mode = mh < 32 ? 'compact' : (mh < 48 ? 'full' : 'expanded');

    // Auto defaults: logo zone equals a square of the matrix height; split 50/50.
    var logoW = mh;
    var splitY = (mh / 2) | 0;

    if (hw.zone_logo_pct > 0 && hw.zone_logo_pct <= 99) {
      logoW = ((mw * hw.zone_logo_pct) / 100) | 0;
      if (logoW < 1) logoW = 1;
      if (logoW >= mw) logoW = mw - 1;
    }
    if (hw.zone_split_pct > 0 && hw.zone_split_pct <= 99) {
      splitY = ((mh * hw.zone_split_pct) / 100) | 0;
      if (splitY < 1) splitY = 1;
      if (splitY >= mh) splitY = mh - 1;
    }

    var result = {
      valid: true, mode: mode,
      matrixWidth: mw, matrixHeight: mh,
      logoZone:      { x: 0,     y: 0,      w: logoW,        h: mh },
      flightZone:    { x: logoW, y: 0,      w: mw - logoW,   h: splitY },
      telemetryZone: { x: logoW, y: splitY, w: mw - logoW,   h: mh - splitY },
    };

    if (hw.zone_layout === 1) {
      result.logoZone      = { x: 0,     y: 0,      w: logoW,        h: splitY };
      result.flightZone    = { x: logoW, y: 0,      w: mw - logoW,   h: splitY };
      result.telemetryZone = { x: 0,     y: splitY, w: mw,           h: mh - splitY };
    }

    return result;
  }

  /* ---------- Widget renderers (mirror of firmware/widgets/*.cpp) ---------- */

  /* Apply the widget's case transform to a string before truncation.
     Mirrors firmware/widgets/WidgetFont.cpp:applyTextTransform — ASCII only,
     non-ASCII bytes pass through unchanged, unknown mode is a no-op. */
  function applyTransform(text, mode) {
    if (text == null) return '';
    if (!mode || mode === 'none') return String(text);
    if (mode === 'upper') return String(text).toUpperCase();
    if (mode === 'lower') return String(text).toLowerCase();
    return String(text);
  }

  /* Safely truncate a string to `maxCols` 6-pixel columns. When the input
     exceeds the budget we keep (maxCols - 3) chars and append "..." so the
     rendered width stays inside the widget zone — matches the char* overload
     in DisplayUtils::truncateToColumns. */
  function truncateToColumns(text, maxCols) {
    if (text == null) return '';
    var s = String(text);
    if (s.length <= maxCols) return s;
    if (maxCols <= 3) return s.substring(0, maxCols);
    return s.substring(0, maxCols - 3) + '...';
  }

  /* Resolve font metrics for a spec that may carry a font id ("default",
     "tomthumb", "picopixel") and an integer text_size (1-3). Falls back to
     the default font + size 1 so legacy specs render exactly as they did
     before this feature landed. Keep in sync with widgetFontMetrics() in
     firmware/core/WidgetRegistry.cpp. */
  function resolveFontMetrics(spec) {
    var scale = spec.text_size | 0;
    if (scale < 1) scale = 1;
    if (scale > 3) scale = 3;
    var font = spec.font;
    if (font === 'tomthumb') {
      return { font: font, scale: scale, charW: 4 * scale, charH: 6 * scale };
    }
    if (font === 'picopixel') {
      return { font: font, scale: scale, charW: 4 * scale, charH: 7 * scale };
    }
    return { font: 'default', scale: scale, charW: 6 * scale, charH: 8 * scale };
  }

  function renderTextWidget(frame, spec, color) {
    if (!spec.content) return;
    drawTextWithFont(frame, spec, spec.content, color);
  }

  /* Paint a single line of widget content with the spec's font + size. Shared
     by Clock / Metric / FlightField once they've resolved their content
     string, so all four text-rendering widget types produce pixel-identical
     output for the same (spec, content). Align of 0/1/2 = left/center/right,
     matching firmware parseAlign semantics (strings already translated by
     the top-level render()). */
  function drawTextWithFont(frame, spec, content, color) {
    var m = resolveFontMetrics(spec);
    var maxCols = (spec.w / m.charW) | 0;
    if (maxCols <= 0) return;
    // Transform before truncation so the ellipsis lands at the correct
    // visual column. Clock's digits are unaffected by upper/lower.
    var transformed = applyTransform(content, spec.text_transform);
    var text = truncateToColumns(transformed, maxCols);
    var textPixels = text.length * m.charW;
    if (textPixels > spec.w) textPixels = spec.w;

    var drawX = spec.x;
    if (spec.align === 1) drawX = spec.x + ((spec.w - textPixels) / 2 | 0);
    else if (spec.align === 2) drawX = spec.x + (spec.w - textPixels);

    var drawY = spec.y;
    if (spec.h > m.charH) drawY = spec.y + ((spec.h - m.charH) / 2 | 0);

    // Custom GFX fonts (TomThumb, Picopixel) use a baseline cursor; shift
     // drawY down by charH-1 so the glyph cell stays inside our centered box.
    if (m.font === 'tomthumb') {
      drawTextTomThumb(frame, drawX, drawY + (m.charH - 1), text, color, m.scale);
    } else if (m.font === 'picopixel') {
      drawTextPicopixel(frame, drawX, drawY + (m.charH - 1), text, color, m.scale);
    } else {
      drawTextGlcd(frame, drawX, drawY, text, color, m.scale);
    }
  }

  /* Clock widget: HH:MM in 24-hour, the browser supplies the time. The
     firmware caches every 30s to avoid syscalls; here we just read Date on
     each render since preview rerenders are cheap and infrequent. */
  function renderClockWidget(frame, spec, color) {
    var m = resolveFontMetrics(spec);
    // Need room for "HH:MM" (5 chars) under the chosen font + scale.
    if (spec.w < 5 * m.charW || spec.h < m.charH) return;
    var now = new Date();
    var hh = ('0' + now.getHours()).slice(-2);
    var mm = ('0' + now.getMinutes()).slice(-2);
    drawTextWithFont(frame, spec, hh + ':' + mm, color);
  }

  /* Mirror of MetricWidget's metric catalog + unit conversions. Returns
     { value, suffix, decimals } for a field id; unknown ids fall back to
     altitude_ft as the first catalog entry (same as the firmware). Null
     source values render as "--". */
  var METRIC_SUFFIXES = { altitude_ft: 'ft', speed_kts: 'kts', speed_mph: 'mph',
    heading_deg: 'deg', vertical_rate_fpm: 'fpm', distance_nm: 'nm',
    bearing_deg: '\u00B0' };

  function resolveMetric(id, flight) {
    function num(v) { return (v == null || isNaN(v)) ? null : Number(v); }
    var result = { value: null, suffix: '', decimals: 0 };
    var key = id;
    if (!METRIC_SUFFIXES.hasOwnProperty(key)) key = 'altitude_ft';
    switch (key) {
      case 'altitude_ft':
        result.value = num(flight && flight.altitude_kft);
        if (result.value !== null) result.value *= 1000;
        result.suffix = 'ft'; break;
      case 'speed_kts':
        result.value = num(flight && flight.speed_mph);
        if (result.value !== null) result.value *= 0.8689762;
        result.suffix = 'kts'; break;
      case 'speed_mph':
        // Live ground speed — already in mph on the wire.
        result.value = num(flight && flight.speed_mph);
        result.suffix = 'mph'; break;
      case 'heading_deg':
        result.value = num(flight && flight.track_deg);
        result.suffix = 'deg'; break;
      case 'vertical_rate_fpm':
        result.value = num(flight && flight.vertical_rate_fps);
        if (result.value !== null) result.value *= 60;
        result.suffix = 'fpm'; break;
      case 'distance_nm':
        result.value = num(flight && flight.distance_km);
        if (result.value !== null) result.value *= 0.5399568;
        result.suffix = 'nm'; result.decimals = 1; break;
      case 'bearing_deg':
        result.value = num(flight && flight.bearing_deg);
        result.suffix = '\u00B0'; break;
    }
    return result;
  }

  function formatMetric(m) {
    if (m.value === null) return '--';
    var n = m.decimals > 0 ? m.value.toFixed(m.decimals) : String(Math.trunc(m.value));
    return n + m.suffix;
  }

  function renderMetricWidget(frame, spec, color, flight) {
    var m = resolveFontMetrics(spec);
    if (spec.w < m.charW || spec.h < m.charH) return;
    var text = flight ? formatMetric(resolveMetric(spec.content, flight)) : '--';
    drawTextWithFont(frame, spec, text, color);
  }

  /* FlightField resolvers — keys MUST match firmware/widgets/FlightFieldWidget.cpp
     kFlightFieldCatalog exactly. Fallback chains mirror the C++ resolveField()
     precedence (e.g. callsign prefers ICAO → IATA → raw ident). Unknown ids
     fall through to kCatalog[0] = 'callsign', matching resolveFieldOrFallback. */
  var FLIGHT_FIELD_RESOLVERS = {
    callsign: function (f) {
      return f.ident_icao || f.ident_iata || f.ident || '';
    },
    flight_number: function (f) {
      // Catalog alias — same backing field as callsign per firmware AC #6.
      return f.ident_icao || f.ident_iata || f.ident || '';
    },
    airline: function (f) {
      // Mirrors FlightFieldWidget.cpp airline resolver: prefer the
      // CDN-resolved display name, fall back to operator_icao so the
      // editor preview renders "UAL" while the device's CDN cache warms up.
      return f.airline_display_name_full || f.operator_icao || '';
    },
    aircraft_type: function (f) {
      return f.aircraft_display_name_short || f.aircraft_code || '';
    },
    aircraft_short: function (f) {
      // Keep in sync with FlightFieldWidget.cpp: short → fall back to code.
      return f.aircraft_display_name_short || f.aircraft_code || '';
    },
    aircraft_full: function (f) {
      // Three-layer fallback matches the firmware resolver so the preview
      // never shows "--" when the short name is present.
      return f.aircraft_display_name_full
          || f.aircraft_display_name_short
          || f.aircraft_code
          || '';
    },
    origin_icao: function (f) { return f.origin_icao || ''; },
    destination_icao: function (f) { return f.destination_icao || ''; }
  };

  function resolveFlightField(id, flight) {
    if (!flight) return '--';
    // Match firmware resolveFieldOrFallback: unknown id → kCatalog[0] = callsign.
    var fn = FLIGHT_FIELD_RESOLVERS[id] || FLIGHT_FIELD_RESOLVERS.callsign;
    var val = fn(flight);
    return (val === '' || val == null) ? '--' : String(val);
  }

  function renderFlightFieldWidget(frame, spec, color, flight) {
    var m = resolveFontMetrics(spec);
    if (spec.w < m.charW || spec.h < m.charH) return;
    var text = resolveFlightField(spec.content, flight);
    drawTextWithFont(frame, spec, text, color);
  }

  /* Logo widget: blit a 32x32 RGB565 bitmap (already byte-swapped to native
     uint16 by the caller's logo loader) centered inside the widget's zone.
     `logoBitmap` is a Uint16Array of length 1024. Falls back to a dashed
     placeholder + ICAO text when no bitmap is available (flight has no
     operator icao, or the logo file hasn't been fetched yet). Matches the
     firmware's LogoManager + drawBitmapRGB565 pipeline. */
  var LOGO_W = 32;
  var LOGO_H = 32;

  function rgb565ToRgba(pixel) {
    var r = (pixel >> 11) & 0x1F;
    var g = (pixel >> 5) & 0x3F;
    var b = pixel & 0x1F;
    return rgba((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2));
  }

  function renderLogoBitmap(frame, spec, bitmap) {
    // Center + crop, mirroring DisplayUtils::drawBitmapRGB565 semantics.
    var offsetX = 0, offsetY = 0;
    var drawW = LOGO_W, drawH = LOGO_H;
    if (spec.w < LOGO_W) { offsetX = ((LOGO_W - spec.w) / 2) | 0; drawW = spec.w; }
    if (spec.h < LOGO_H) { offsetY = ((LOGO_H - spec.h) / 2) | 0; drawH = spec.h; }

    var destX = spec.x;
    var destY = spec.y;
    if (spec.w > LOGO_W) destX = spec.x + (((spec.w - LOGO_W) / 2) | 0);
    if (spec.h > LOGO_H) destY = spec.y + (((spec.h - LOGO_H) / 2) | 0);

    for (var row = 0; row < drawH; row++) {
      for (var col = 0; col < drawW; col++) {
        var src = bitmap[(row + offsetY) * LOGO_W + (col + offsetX)];
        if (src !== 0) {
          setPixel(frame, destX + col, destY + row, rgb565ToRgba(src));
        }
      }
    }
  }

  function renderLogoPlaceholder(frame, spec, color, flight) {
    // Dashed border + ICAO hint. Used whenever the bitmap isn't loaded yet
    // (no operator_icao on the flight, or the fetch is still in flight).
    for (var x = spec.x; x < spec.x + spec.w; x += 2) {
      setPixel(frame, x, spec.y, color);
      setPixel(frame, x, spec.y + spec.h - 1, color);
    }
    for (var y = spec.y; y < spec.y + spec.h; y += 2) {
      setPixel(frame, spec.x, y, color);
      setPixel(frame, spec.x + spec.w - 1, y, color);
    }
    var icao = (flight && (flight.operator_icao || flight.operator_iata)) || 'LOGO';
    var trunc = truncateToColumns(icao, (spec.w / WIDGET_CHAR_W) | 0);
    var textPixels = trunc.length * WIDGET_CHAR_W;
    var drawX = spec.x + ((spec.w - textPixels) / 2 | 0);
    var drawY = spec.y + ((spec.h - WIDGET_CHAR_H) / 2 | 0);
    drawTextGlcd(frame, drawX, drawY, trunc, color);
  }

  function renderLogoWidget(frame, spec, color, flight, logoCache) {
    var icao = flight && flight.operator_icao;
    var bitmap = (icao && logoCache) ? logoCache[icao] : null;
    if (bitmap && bitmap.length === LOGO_W * LOGO_H) {
      renderLogoBitmap(frame, spec, bitmap);
    } else {
      renderLogoPlaceholder(frame, spec, color, flight);
    }
  }

  /* ---------- Top-level render ---------- */

  /* Render one frame to the target canvas.
     opts: {
       canvas:      HTMLCanvasElement
       matrixW/H:   logical matrix dimensions
       scale:       integer upscale factor (defaults to canvas.width / matrixW)
       widgets:     [{type,x,y,w,h,color,content,align}]
       flight:      current flight object (or null)
       background:  '#000' by default
     }
  */
  function render(opts) {
    var canvas = opts.canvas;
    if (!canvas) return;
    var ctx = canvas.getContext('2d');
    var matrixW = opts.matrixW;
    var matrixH = opts.matrixH;
    var scale = opts.scale || (canvas.width / matrixW) | 0 || 1;

    // Size the target to an integer multiple of the matrix dimensions so
    // nearest-neighbor upscaling produces crisp squares.
    canvas.width = matrixW * scale;
    canvas.height = matrixH * scale;

    // Paint the background first.
    var bgRgba = opts.background
      ? parseHexColor(opts.background)
      : rgba(8, 8, 12);
    var frame = createFrame(matrixW, matrixH);
    clearFrame(frame, bgRgba);

    var flight = opts.flight || null;
    var widgets = opts.widgets || [];

    for (var i = 0; i < widgets.length; i++) {
      var w = widgets[i];
      if (!w || typeof w.type !== 'string') continue;
      var color = parseHexColor(w.color || '#FFFFFF');
      // Widget.align is stored as a string in the editor ('left'/'center'/
      // 'right') and only translated to the numeric firmware encoding by
      // CustomLayoutMode::parseAlign(). Do the same translation here so the
      // preview doesn't render everything left-aligned.
      var alignNum = 0;
      if (w.align === 'center') alignNum = 1;
      else if (w.align === 'right') alignNum = 2;
      else if (typeof w.align === 'number') alignNum = w.align | 0;
      var spec = {
        x: w.x | 0, y: w.y | 0, w: w.w | 0, h: w.h | 0,
        content: w.content || '',
        align: alignNum,
        font: w.font || 'default',
        text_size: (w.text_size | 0) || 1,
        text_transform: w.text_transform || 'none'
      };
      switch (w.type) {
        case 'text':         renderTextWidget(frame, spec, color); break;
        case 'clock':        renderClockWidget(frame, spec, color); break;
        case 'metric':       renderMetricWidget(frame, spec, color, flight); break;
        case 'flight_field': renderFlightFieldWidget(frame, spec, color, flight); break;
        case 'logo':         renderLogoWidget(frame, spec, color, flight, opts.logoCache || null); break;
        default:
          // Unknown widget type — draw a faint rectangle so it's visible.
          for (var yy = spec.y; yy < spec.y + spec.h; yy++) {
            for (var xx = spec.x; xx < spec.x + spec.w; xx++) {
              setPixel(frame, xx, yy, rgba(60, 60, 60));
            }
          }
          break;
      }
    }

    // Blit the logical frame to the canvas via ImageData, upscaled by `scale`
    // with manual nearest-neighbor — the browser's imageSmoothingEnabled=false
    // handles upscaling of a small ImageData, but blitting directly without a
    // temp avoids allocating a second ImageData at the full target size. Cost
    // is still O(matrixW * matrixH * scale^2) which is trivial for 80x48 @ 8x.
    var imageData = ctx.createImageData(scale, scale);
    var imgPixels = new Uint32Array(imageData.data.buffer);
    for (var py = 0; py < matrixH; py++) {
      for (var px = 0; px < matrixW; px++) {
        var pixel = frame.pixels[py * matrixW + px];
        imgPixels.fill(pixel);
        ctx.putImageData(imageData, px * scale, py * scale);
      }
    }

    // Subtle grid overlay so the LED-pixel nature is obvious even when widgets
    // are blank. Draw on top of the pixel blit so colors underneath show through.
    ctx.strokeStyle = 'rgba(255,255,255,0.05)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (var gx = 0; gx <= matrixW; gx++) {
      var sx = gx * scale + 0.5;
      ctx.moveTo(sx, 0);
      ctx.lineTo(sx, matrixH * scale);
    }
    for (var gy = 0; gy <= matrixH; gy++) {
      var sy = gy * scale + 0.5;
      ctx.moveTo(0, sy);
      ctx.lineTo(matrixW * scale, sy);
    }
    ctx.stroke();
  }

  global.FWPreview = {
    render: render,
    computeLayout: computeLayout,
    drawTextGlcd: drawTextGlcd,
    drawTextTomThumb: drawTextTomThumb,
    parseHexColor: parseHexColor
  };
})(typeof window !== 'undefined' ? window : this);
