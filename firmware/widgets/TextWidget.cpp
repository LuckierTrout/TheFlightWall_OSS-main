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

#include <cstring>

bool renderText(const WidgetSpec& spec, const RenderContext& ctx) {
    // Empty content is a valid (silent) widget — nothing to draw, but not
    // an error. Return true so the caller's success metric stays accurate.
    if (spec.content[0] == '\0') return true;

    // Zone too narrow for even one glyph → no-op success.
    int maxCols = (int)spec.w / WIDGET_CHAR_W;
    if (maxCols <= 0) return true;

    // Zone shorter than a full glyph row is clipped by the matrix itself;
    // still acceptable per AC #7 (min height = font size).
    //
    // Test harness uses ctx.matrix == nullptr to exercise dispatch without
    // real hardware. We accept that path as a success so the test suite
    // remains portable (see Dev Notes → Test Infrastructure).
    if (ctx.matrix == nullptr) return true;

    // Stack buffer sized to spec.content (48) + safety margin. DisplayUtils
    // truncates into `out` without touching the heap.
    char out[48];
    DisplayUtils::truncateToColumns(spec.content, maxCols, out, sizeof(out));

    // Compute draw-x from alignment. Use the *rendered* width so the
    // ellipsis appended by truncateToColumns is positioned correctly.
    int textPixels = (int)strlen(out) * WIDGET_CHAR_W;
    if (textPixels > (int)spec.w) textPixels = (int)spec.w;

    int16_t drawX = spec.x;
    switch (spec.align) {
        case 1: drawX = spec.x + (int16_t)(((int)spec.w - textPixels) / 2); break;
        case 2: drawX = spec.x + (int16_t)((int)spec.w - textPixels);       break;
        case 0:
        default: /* left */ break;
    }

    // Vertically center when there is slack. For zones smaller than a
    // glyph we render at spec.y (matrix clipping handles OOB).
    int16_t drawY = spec.y;
    if ((int)spec.h > WIDGET_CHAR_H) {
        drawY = spec.y + (int16_t)(((int)spec.h - WIDGET_CHAR_H) / 2);
    }

    DisplayUtils::drawTextLine(ctx.matrix, drawX, drawY, out, spec.color);
    return true;
}
