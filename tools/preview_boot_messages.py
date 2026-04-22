#!/usr/bin/env python3
"""
Pixel-accurate preview of FlightWall boot messages.

Renders each StartupPhase message onto a simulated LED matrix using the exact
TomThumb glyph data from Adafruit_GFX_Library. Matches the centering logic
now used in `NeoMatrixDisplay::displayMessage()` so the output is a faithful
preview of what the wall will show.

Usage:
    python3 tools/preview_boot_messages.py [output_path]

Default output: `tools/boot_preview.png`
"""

import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

# --- TomThumb glyph data (copied from firmware/.pio/.../Fonts/TomThumb.h) ---
# Bitmap bytes and (offset, width, height, xAdvance, xOffset, yOffset) per
# ASCII codepoint 0x20..0x7E.

TOMTHUMB_FIRST = 0x20
TOMTHUMB_LAST = 0x7E
TOMTHUMB_YADVANCE = 6

TOMTHUMB_BITMAPS = bytes([
    0x00, 0xE8, 0xB4, 0xBE, 0xFA, 0x79, 0xE4, 0x85, 0x42, 0xDB, 0xD6, 0xC0,
    0x6A, 0x40, 0x95, 0x80, 0xAA, 0x80, 0x5D, 0x00, 0x60, 0xE0, 0x80, 0x25,
    0x48, 0x76, 0xDC, 0x75, 0x40, 0xC5, 0x4E, 0xC5, 0x1C, 0xB7, 0x92, 0xF3,
    0x1C, 0x73, 0xDE, 0xE5, 0x48, 0xF7, 0xDE, 0xF7, 0x9C, 0xA0, 0x46, 0x2A,
    0x22, 0xE3, 0x80, 0x88, 0xA8, 0xE5, 0x04, 0x57, 0xC6, 0x57, 0xDA, 0xD7,
    0x5C, 0x72, 0x46, 0xD6, 0xDC, 0xF3, 0xCE, 0xF3, 0xC8, 0x73, 0xD6, 0xB7,
    0xDA, 0xE9, 0x2E, 0x24, 0xD4, 0xB7, 0x5A, 0x92, 0x4E, 0xBF, 0xDA, 0xBF,
    0xFA, 0x56, 0xD4, 0xD7, 0x48, 0x56, 0xF6, 0xD7, 0xEA, 0x71, 0x1C, 0xE9,
    0x24, 0xB6, 0xD6, 0xB6, 0xA4, 0xB7, 0xFA, 0xB5, 0x5A, 0xB5, 0x24, 0xE5,
    0x4E, 0xF2, 0x4E, 0x88, 0x80, 0xE4, 0x9E, 0x54, 0xE0, 0x90, 0xCE, 0xF0,
    0x9A, 0xDC, 0x72, 0x30, 0x2E, 0xD6, 0x77, 0x30, 0x2B, 0xA4, 0x77, 0x94,
    0x9A, 0xDA, 0xB8, 0x20, 0x9A, 0x80, 0x97, 0x6A, 0xC9, 0x2E, 0xFF, 0xD0,
    0xD6, 0xD0, 0x56, 0xA0, 0xD6, 0xE8, 0x76, 0xB2, 0x72, 0x40, 0x79, 0xE0,
    0x5D, 0x26, 0xB6, 0xB0, 0xB7, 0xA0, 0xBF, 0xF0, 0xA9, 0x50, 0xB5, 0x94,
    0xEF, 0x70, 0x6A, 0x26, 0xD8, 0xC8, 0xAC, 0x78,
])

# (offset, width, height, xAdvance, xOffset, yOffset) per glyph, 0x20..0x7E
TOMTHUMB_GLYPHS = [
    (0, 1, 1, 2, 0, -5),    (1, 1, 5, 2, 0, -5),    (2, 3, 2, 4, 0, -5),
    (3, 3, 5, 4, 0, -5),    (5, 3, 5, 4, 0, -5),    (7, 3, 5, 4, 0, -5),
    (9, 3, 5, 4, 0, -5),    (11, 1, 2, 2, 0, -5),   (12, 2, 5, 3, 0, -5),
    (14, 2, 5, 3, 0, -5),   (16, 3, 3, 4, 0, -5),   (18, 3, 3, 4, 0, -4),
    (20, 2, 2, 3, 0, -2),   (21, 3, 1, 4, 0, -3),   (22, 1, 1, 2, 0, -1),
    (23, 3, 5, 4, 0, -5),   (25, 3, 5, 4, 0, -5),   (27, 2, 5, 3, 0, -5),
    (29, 3, 5, 4, 0, -5),   (31, 3, 5, 4, 0, -5),   (33, 3, 5, 4, 0, -5),
    (35, 3, 5, 4, 0, -5),   (37, 3, 5, 4, 0, -5),   (39, 3, 5, 4, 0, -5),
    (41, 3, 5, 4, 0, -5),   (43, 3, 5, 4, 0, -5),   (45, 1, 3, 2, 0, -4),
    (46, 2, 4, 3, 0, -4),   (47, 3, 5, 4, 0, -5),   (49, 3, 3, 4, 0, -4),
    (51, 3, 5, 4, 0, -5),   (53, 3, 5, 4, 0, -5),   (55, 3, 5, 4, 0, -5),
    (57, 3, 5, 4, 0, -5),   (59, 3, 5, 4, 0, -5),   (61, 3, 5, 4, 0, -5),
    (63, 3, 5, 4, 0, -5),   (65, 3, 5, 4, 0, -5),   (67, 3, 5, 4, 0, -5),
    (69, 3, 5, 4, 0, -5),   (71, 3, 5, 4, 0, -5),   (73, 3, 5, 4, 0, -5),
    (75, 3, 5, 4, 0, -5),   (77, 3, 5, 4, 0, -5),   (79, 3, 5, 4, 0, -5),
    (81, 3, 5, 4, 0, -5),   (83, 3, 5, 4, 0, -5),   (85, 3, 5, 4, 0, -5),
    (87, 3, 5, 4, 0, -5),   (89, 3, 5, 4, 0, -5),   (91, 3, 5, 4, 0, -5),
    (93, 3, 5, 4, 0, -5),   (95, 3, 5, 4, 0, -5),   (97, 3, 5, 4, 0, -5),
    (99, 3, 5, 4, 0, -5),   (101, 3, 5, 4, 0, -5),  (103, 3, 5, 4, 0, -5),
    (105, 3, 5, 4, 0, -5),  (107, 3, 5, 4, 0, -5),  (109, 3, 5, 4, 0, -5),
    (111, 3, 3, 4, 0, -4),  (113, 3, 5, 4, 0, -5),  (115, 3, 2, 4, 0, -5),
    (116, 3, 1, 4, 0, -1),  (117, 2, 2, 3, 0, -5),  (118, 3, 4, 4, 0, -4),
    (120, 3, 5, 4, 0, -5),  (122, 3, 4, 4, 0, -4),  (124, 3, 5, 4, 0, -5),
    (126, 3, 4, 4, 0, -4),  (128, 3, 5, 4, 0, -5),  (130, 3, 5, 4, 0, -4),
    (132, 3, 5, 4, 0, -5),  (134, 1, 5, 2, 0, -5),  (135, 3, 6, 4, 0, -5),
    (138, 3, 5, 4, 0, -5),  (140, 3, 5, 4, 0, -5),  (142, 3, 4, 4, 0, -4),
    (144, 3, 4, 4, 0, -4),  (146, 3, 4, 4, 0, -4),  (148, 3, 5, 4, 0, -4),
    (150, 3, 5, 4, 0, -4),  (152, 3, 4, 4, 0, -4),  (154, 3, 4, 4, 0, -4),
    (156, 3, 5, 4, 0, -5),  (158, 3, 4, 4, 0, -4),  (160, 3, 4, 4, 0, -4),
    (162, 3, 4, 4, 0, -4),  (164, 3, 4, 4, 0, -4),  (166, 3, 5, 4, 0, -4),
    (168, 3, 4, 4, 0, -4),  (170, 3, 5, 4, 0, -5),  (172, 1, 5, 2, 0, -5),
    (173, 3, 5, 4, 0, -5),  (175, 3, 2, 4, 0, -5),
]

# --- Matrix + render config ---

MATRIX_W = 80
MATRIX_H = 48
SCALE = 10  # each matrix pixel is drawn as SCALE×SCALE on the preview image
GRID_COLOR = (30, 30, 30)
BG_COLOR = (0, 0, 0)
LABEL_HEIGHT = 28

# Status colors mirrored from firmware/src/main.cpp display task:
#   PENDING: configured text color (default white)
#   OK:      (0, 220, 0)
#   FAIL:    (220, 0, 0)
PENDING = (255, 255, 255)
OK = (0, 220, 0)
FAIL = (220, 0, 0)

MESSAGES = [
    ("Setup Mode",                     PENDING, "AP mode — no Wi-Fi creds yet"),
    ("Connecting to WiFi...",          PENDING, "Startup: wifi phase"),
    ("WiFi OK",                        OK,      "Startup: wifi connected"),
    ("IP: 192.168.1.77",               PENDING, "Startup: address shown 2s"),
    ("Authenticating APIs...",         PENDING, "Startup: auth pause 1s"),
    ("Fetching flights...",            PENDING, "Startup: first fetch"),
    ("Flights OK",                     OK,      "Startup: first data in"),
    ("WiFi FAIL - Reopen Setup",       FAIL,    "Wi-Fi failure fallback"),
    ("Saving config...",               PENDING, "Dashboard reboot triggered"),
]


def glyph_for(ch):
    """Return the TomThumb glyph tuple for a character, or None for out-of-range."""
    code = ord(ch)
    if code < TOMTHUMB_FIRST or code > TOMTHUMB_LAST:
        return None
    return TOMTHUMB_GLYPHS[code - TOMTHUMB_FIRST]


def text_bounds(text):
    """Mirror Adafruit_GFX getTextBounds() for a single-line string in TomThumb.

    Returns (x_min, y_min, width, height) relative to the cursor origin.
    """
    x = 0
    x_min = 0
    y_min = 0
    x_max = 0
    y_max = 0
    first = True
    for ch in text:
        g = glyph_for(ch)
        if g is None:
            x += 2  # fall back to approx space width
            continue
        _off, w, h, xa, xo, yo = g
        gx_min = x + xo
        gx_max = x + xo + w - 1
        gy_min = yo
        gy_max = yo + h - 1
        if first:
            x_min, y_min, x_max, y_max = gx_min, gy_min, gx_max, gy_max
            first = False
        else:
            x_min = min(x_min, gx_min)
            y_min = min(y_min, gy_min)
            x_max = max(x_max, gx_max)
            y_max = max(y_max, gy_max)
        x += xa
    if first:
        return 0, 0, 0, 0
    return x_min, y_min, x_max - x_min + 1, y_max - y_min + 1


def render_to_matrix(text, color):
    """Render `text` onto a MATRIX_W × MATRIX_H pixel grid.

    Uses the same centering rule as NeoMatrixDisplay::displayMessage():
        cursorX = (MATRIX_W - w) / 2 - x1
        cursorY = (MATRIX_H - h) / 2 - y1
    """
    grid = [[BG_COLOR] * MATRIX_W for _ in range(MATRIX_H)]
    x1, y1, w, h = text_bounds(text)
    if w == 0 or h == 0:
        return grid

    cursor_x = (MATRIX_W - w) // 2 - x1
    cursor_y = (MATRIX_H - h) // 2 - y1

    for ch in text:
        g = glyph_for(ch)
        if g is None:
            cursor_x += 2
            continue
        off, gw, gh, xa, xo, yo = g
        bit_index = 0
        for yy in range(gh):
            for xx in range(gw):
                byte_index = off + (bit_index >> 3)
                bit_in_byte = 7 - (bit_index & 7)
                if byte_index < len(TOMTHUMB_BITMAPS) and (TOMTHUMB_BITMAPS[byte_index] >> bit_in_byte) & 1:
                    px = cursor_x + xo + xx
                    py = cursor_y + yo + yy
                    if 0 <= px < MATRIX_W and 0 <= py < MATRIX_H:
                        grid[py][px] = color
                bit_index += 1
        cursor_x += xa
    return grid


def draw_matrix(draw, origin, grid):
    """Paint one matrix grid at pixel coords `origin` on the output image."""
    ox, oy = origin
    for y in range(MATRIX_H):
        for x in range(MATRIX_W):
            color = grid[y][x]
            x0 = ox + x * SCALE
            y0 = oy + y * SCALE
            x1 = x0 + SCALE - 1
            y1 = y0 + SCALE - 1
            draw.rectangle([x0, y0, x1, y1], fill=color, outline=GRID_COLOR)


def load_label_font():
    """Find a usable TTF for the per-panel labels; fall back to PIL default."""
    candidates = [
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ]
    for path in candidates:
        if Path(path).exists():
            try:
                return ImageFont.truetype(path, 16)
            except Exception:
                continue
    return ImageFont.load_default()


def main():
    out_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "boot_preview.png"

    panel_w = MATRIX_W * SCALE
    panel_h = MATRIX_H * SCALE
    row_h = LABEL_HEIGHT + panel_h + 12
    img_w = panel_w + 24
    img_h = row_h * len(MESSAGES) + 12

    img = Image.new("RGB", (img_w, img_h), (18, 18, 22))
    draw = ImageDraw.Draw(img)
    font = load_label_font()

    for i, (text, color, note) in enumerate(MESSAGES):
        y = 12 + i * row_h
        label = f'"{text}"   —   {note}'
        draw.text((12, y + 4), label, fill=(220, 220, 220), font=font)
        grid = render_to_matrix(text, color)
        draw_matrix(draw, (12, y + LABEL_HEIGHT), grid)

    img.save(out_path)
    print(f"Wrote {out_path} ({img_w}x{img_h}, matrix {MATRIX_W}x{MATRIX_H}, scale {SCALE}x)")


if __name__ == "__main__":
    main()
