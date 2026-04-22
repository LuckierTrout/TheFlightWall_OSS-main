/*
Purpose: Logo widget — real RGB565 pipeline (Story LE-1.5).

Renders a 32×32 RGB565 bitmap loaded from LittleFS via LogoManager, or the
PROGMEM airplane fallback sprite when the requested logo_id does not exist.
Zero per-frame heap allocation: the shared 2 KB `ctx.logoBuffer` (owned by
NeoMatrixDisplay) is reused as a scratch buffer, matching the canonical
ClassicCardMode::renderLogoZone() pattern.

Contract:
  - `spec.content` holds the logo ICAO/id without the `.bin` extension
    (e.g. "UAL"). LogoManager::loadLogo() appends `.bin` internally and
    normalizes case via trim() + toUpperCase().
  - `drawBitmapRGB565` dimensions are passed as LOGO_WIDTH × LOGO_HEIGHT
    (32×32). The zone dimensions (spec.w × spec.h) are forwarded separately
    so drawBitmapRGB565 can clip / center the bitmap into the zone.

Why the bitmap path vs. a rectangle-fill primitive?
  Spike measurements (epic LE-1): per-pixel rectangle fills of a 32×32
  region cost ~2.67× render-time baseline, whereas the RGB565 bitmap blit
  is ~1.23×. A second 2 KB static scratch buffer would also duplicate the
  NeoMatrixDisplay-owned ctx.logoBuffer — wasteful BSS for no benefit.

Call-ordering note (test contract):
  LogoManager::loadLogo() is called BEFORE the ctx.matrix == nullptr guard
  so that hardware-free unit tests can inspect ctx.logoBuffer after a call
  to renderLogo(). On real hardware the guard short-circuits drawBitmapRGB565
  only — the buffer is still populated (cheap on an ESP32 LittleFS read and
  matches ClassicCardMode's per-frame behavior).
*/

#include "widgets/LogoWidget.h"

#include "core/LogoManager.h"
#include "utils/DisplayUtils.h"

// Uniform padding (in pixels) inside the widget bounds. Kept at 0 because
// LayoutEngine + the universal panel frame already provide a 1 px visual
// gap around every zone — adding more here stacks into 2 px on the top/
// left, which looked wrong. Raise to 1 if a future design drops the frame.
static constexpr int LOGO_PAD_PX = 0;

bool renderLogo(const WidgetSpec& spec, const RenderContext& ctx) {
    // Minimum dimension floor (AC #5) — preserves V1 stub behavior.
    if ((int)spec.w < 8 || (int)spec.h < 8) return true;

    // Null buffer guard (AC #3) — cannot blit or load without a buffer.
    if (ctx.logoBuffer == nullptr) return true;

    // Load RGB565 bitmap from LittleFS; on any failure (missing file, bad
    // size, empty/whitespace ICAO), LogoManager copies the PROGMEM airplane
    // fallback sprite into ctx.logoBuffer. Return value is intentionally
    // ignored — the buffer is always populated with renderable pixels.
    LogoManager::loadLogo(spec.content, ctx.logoBuffer);

    // Hardware-free test path — buffer is loaded above; skip the blit.
    if (ctx.matrix == nullptr) return true;

    // Apply uniform inside-padding. If the zone is too small to accommodate
    // the padding on both sides, fall back to the full zone so the logo is
    // still visible (degrades gracefully rather than disappearing).
    int16_t drawX = spec.x;
    int16_t drawY = spec.y;
    uint16_t drawW = spec.w;
    uint16_t drawH = spec.h;
    if ((int)spec.w > 2 * LOGO_PAD_PX && (int)spec.h > 2 * LOGO_PAD_PX) {
        drawX = spec.x + LOGO_PAD_PX;
        drawY = spec.y + LOGO_PAD_PX;
        drawW = spec.w - 2 * LOGO_PAD_PX;
        drawH = spec.h - 2 * LOGO_PAD_PX;
    }

    DisplayUtils::drawBitmapRGB565(ctx.matrix, drawX, drawY,
                                   LOGO_WIDTH, LOGO_HEIGHT,
                                   ctx.logoBuffer,
                                   drawW, drawH);
    return true;
}
