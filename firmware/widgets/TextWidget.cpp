/*
Purpose: Render a static text widget (Story LE-1.2, AC #4/#7).

Behavior:
  - Returns true on success (including no-op cases: empty content, zone too
    small to fit a single character). False is reserved for hard errors;
    there are none on the current path.
  - Truncates to the widget's column width via DisplayUtils::truncateToColumns
    (char* overload — no heap allocation).
  - Supports align=0 (left), 1 (center), 2 (right). Centering/right-align
    uses the truncated text's pixel width so multi-character suffixes like
    "..." stay inside the zone.
  - Enforces the CHAR_H minimum height: if spec.h < CHAR_H we still render
    at spec.y (no vertical centering offset) so at least partial glyphs
    remain visible within the matrix bounds.
  - Guards nullptr matrix to keep the unit tests hardware-free (matches the
    CustomLayoutMode::render pattern).
*/

#include "widgets/TextWidget.h"

#include "utils/DisplayUtils.h"
#include "widgets/WidgetFont.h"

#include <cstring>

bool renderText(const WidgetSpec& spec, const RenderContext& ctx) {
    // Empty content is a valid (silent) widget — nothing to draw, but not
    // an error. Return true so the caller's success metric stays accurate.
    if (spec.content[0] == '\0') return true;

    // Font + scale come from the spec; widgetFontMetrics() clamps size to
    // a sane [1, 3] range so even a corrupted layout byte can't crash us.
    // The default font (font_id == 0, size == 1) reproduces the legacy
    // 6x8 behavior used by every layout written before this change.
    WidgetFontId fontId = resolveWidgetFontId(spec.font_id);
    uint8_t textSize = spec.text_size == 0 ? 1 : spec.text_size;
    WidgetFontMetrics metrics = widgetFontMetrics(fontId, textSize);

    // Zone too narrow for even one glyph → no-op success.
    int maxCols = (int)spec.w / metrics.charW;
    if (maxCols <= 0) return true;

    // Test harness uses ctx.matrix == nullptr to exercise dispatch without
    // real hardware. We accept that path as a success so the test suite
    // remains portable (see Dev Notes → Test Infrastructure).
    if (ctx.matrix == nullptr) return true;

    // Stack buffer sized to spec.content (48) + safety margin. DisplayUtils
    // truncates into `out` without touching the heap. We apply any case
    // transform to a scratch copy *before* truncation so the ellipsis from
    // truncateToColumns lands at the right visual column.
    char scratch[sizeof(spec.content)];
    size_t contentLen = strlen(spec.content);
    if (contentLen >= sizeof(scratch)) contentLen = sizeof(scratch) - 1;
    memcpy(scratch, spec.content, contentLen);
    scratch[contentLen] = '\0';
    applyTextTransform(scratch, spec.text_transform);

    char out[48];
    DisplayUtils::truncateToColumns(scratch, maxCols, out, sizeof(out));

    // Compute draw-x from alignment. Use the *rendered* width so the
    // ellipsis appended by truncateToColumns is positioned correctly.
    int textPixels = (int)strlen(out) * metrics.charW;
    if (textPixels > (int)spec.w) textPixels = (int)spec.w;

    int16_t drawX = spec.x;
    switch (spec.align) {
        case 1: drawX = spec.x + (int16_t)(((int)spec.w - textPixels) / 2); break;
        case 2: drawX = spec.x + (int16_t)((int)spec.w - textPixels);       break;
        case 0:
        default: /* left */ break;
    }

    // Apply the widget's font + scale for this render. Restore both to the
    // defaults afterward so the rest of the frame (subsequent widgets using
    // the classic 6x8 font) renders with the expected metrics. Custom GFX
    // fonts use a baseline cursor convention — TomThumb glyphs extend
    // upward from the baseline, so we offset drawY by (charH - 1) to
    // position the top of the glyph where the widget expects it.
    int16_t drawY = spec.y;
    if ((int)spec.h > metrics.charH) {
        drawY = spec.y + (int16_t)(((int)spec.h - metrics.charH) / 2);
    }

    ctx.matrix->setTextSize(textSize);
    drawY += applyWidgetFont(ctx.matrix, fontId, metrics.charH);
    DisplayUtils::drawTextLine(ctx.matrix, drawX, drawY, out, spec.color);

    // Reset to canonical state (default font, 1x) so widgets later in the
    // frame don't inherit our selection.
    ctx.matrix->setFont(nullptr);
    ctx.matrix->setTextSize(1);
    return true;
}
