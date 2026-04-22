#!/usr/bin/env python3
"""
Extract Adafruit_GFX font data into a JS data module for the dashboard preview.

Parses:
  - `glcdfont.c`          (default 5x7 font rendered in a 6x8 advance cell)
  - `Fonts/TomThumb.h`    (compact 3x5 glyphs in a 4x6 advance cell)
  - `Fonts/Picopixel.h`   (4x5-ish glyphs in a 7px line advance)

Writes `firmware/data-src/preview-glyphs.js` with:
  - `GLCD_FONT`        : 1280-byte Uint8Array (256 glyphs × 5 columns, each col = 7-bit
                         vertical pixel pattern; default Adafruit font convention).
  - `TOMTHUMB_BITMAPS` : packed bitmap bytes for ASCII 0x20..0x7E.
  - `TOMTHUMB_GLYPHS`  : per-char [offset, width, height, xAdvance, xOffset, yOffset].
  - `PICOPIXEL_BITMAPS`: same packed format as TomThumb.
  - `PICOPIXEL_GLYPHS` : same per-char tuple layout as TomThumb.

Run:  python3 tools/generate_preview_glyphs.py
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
GFX_LIB_CANDIDATES = [
    PROJECT_ROOT / "firmware/.pio/libdeps/esp32dev/Adafruit GFX Library",
    PROJECT_ROOT / "firmware/.pio/libdeps/esp32dev_le_baseline/Adafruit GFX Library",
    PROJECT_ROOT / "firmware/.pio/libdeps/esp32dev_le_spike/Adafruit GFX Library",
]


def locate_gfx_lib() -> Path:
    for path in GFX_LIB_CANDIDATES:
        if (path / "glcdfont.c").exists() and (path / "Fonts/TomThumb.h").exists() \
                and (path / "Fonts/Picopixel.h").exists():
            return path
    print(
        "ERROR: could not find Adafruit GFX library under firmware/.pio/libdeps/.",
        file=sys.stderr,
    )
    print("Run `pio run` at least once to install dependencies, then rerun.", file=sys.stderr)
    sys.exit(1)


HEX_BYTE = re.compile(r"0x([0-9A-Fa-f]{1,2})")
GLYPH_TUPLE = re.compile(
    r"\{\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\}"
)


def parse_glcd_font(source: str) -> list[int]:
    """Return the flat byte array from `static const unsigned char font[] PROGMEM = { ... };`."""
    # Isolate the array body. The file declares one `font[]` array; we grab the
    # text between the first '{' and its matching closing '};'.
    start = source.index("font[]")
    brace = source.index("{", start)
    end = source.index("};", brace)
    body = source[brace + 1 : end]
    return [int(m.group(1), 16) for m in HEX_BYTE.finditer(body)]


def _slice_array_body(source: str, array_name: str) -> str:
    """Return the text between `<array_name>[] ... { ... };` for a flat initializer.

    The TomThumb arrays contain an inline `#if (TOMTHUMB_USE_EXTENDED)` block,
    so we can't simply truncate at the first `};`. We locate the array's opening
    brace and walk forward, matching `{` / `}` while ignoring preprocessor
    directives, until the outermost closing brace is found.
    """
    start = source.index(f"{array_name}[]")
    brace = source.index("{", start)
    depth = 0
    i = brace
    while i < len(source):
        ch = source[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1 : i]
        i += 1
    raise ValueError(f"Unterminated initializer for {array_name}[]")


def parse_gfx_font(
    source: str, bitmaps_name: str, glyphs_name: str, extended_macro: str | None = None
) -> tuple[list[int], list[tuple[int, int, int, int, int, int]]]:
    """Parse an Adafruit GFX custom-font .h into (bitmaps, glyphs) for ASCII 0x20..0x7E.

    Some bundled fonts (TomThumb) conditionally emit extended glyphs under a
    `#if (<MACRO>)` directive. Pass the macro name so we slice it off.
    """
    bm_body = _slice_array_body(source, bitmaps_name)
    gl_body = _slice_array_body(source, glyphs_name)

    if extended_macro:
        guard = f"#if ({extended_macro})"
        cut = bm_body.find(guard)
        if cut != -1:
            bm_body = bm_body[:cut]
        cut = gl_body.find(guard)
        if cut != -1:
            gl_body = gl_body[:cut]

    bitmaps = [int(m.group(1), 16) for m in HEX_BYTE.finditer(bm_body)]
    glyphs = [
        tuple(int(x) for x in m.groups()) for m in GLYPH_TUPLE.finditer(gl_body)
    ]
    expected = 0x7E - 0x20 + 1  # 95 printable ASCII glyphs
    if len(glyphs) != expected:
        print(
            f"WARNING: parsed {len(glyphs)} {glyphs_name} entries, expected {expected}.",
            file=sys.stderr,
        )
    return bitmaps, glyphs


def parse_tomthumb(source: str) -> tuple[list[int], list[tuple[int, int, int, int, int, int]]]:
    return parse_gfx_font(
        source,
        bitmaps_name="TomThumbBitmaps",
        glyphs_name="TomThumbGlyphs",
        extended_macro="TOMTHUMB_USE_EXTENDED",
    )


def parse_picopixel(source: str) -> tuple[list[int], list[tuple[int, int, int, int, int, int]]]:
    return parse_gfx_font(
        source,
        bitmaps_name="PicopixelBitmaps",
        glyphs_name="PicopixelGlyphs",
    )


def emit_uint8_rows(values: list[int], per_row: int = 16) -> str:
    lines = []
    for i in range(0, len(values), per_row):
        chunk = ", ".join(f"0x{v:02X}" for v in values[i : i + per_row])
        lines.append(f"    {chunk},")
    return "\n".join(lines)


def emit_glyph_rows(glyphs: list[tuple[int, int, int, int, int, int]]) -> str:
    lines = []
    for i, g in enumerate(glyphs):
        ch = chr(0x20 + i)
        display = ch if ch != "\\" else "\\\\"
        lines.append(
            f"    [{g[0]:>4}, {g[1]}, {g[2]}, {g[3]}, {g[4]:>3}, {g[5]:>3}],  // 0x{0x20 + i:02X} '{display}'"
        )
    return "\n".join(lines)


def main() -> None:
    gfx_lib = locate_gfx_lib()
    glcd_path = gfx_lib / "glcdfont.c"
    tt_path = gfx_lib / "Fonts/TomThumb.h"
    pico_path = gfx_lib / "Fonts/Picopixel.h"

    glcd_bytes = parse_glcd_font(glcd_path.read_text())
    expected_glcd = 256 * 5
    if len(glcd_bytes) != expected_glcd:
        print(
            f"WARNING: parsed {len(glcd_bytes)} glcd bytes, expected {expected_glcd}.",
            file=sys.stderr,
        )

    tt_bitmaps, tt_glyphs = parse_tomthumb(tt_path.read_text())
    pico_bitmaps, pico_glyphs = parse_picopixel(pico_path.read_text())

    out_path = PROJECT_ROOT / "firmware/data-src/preview-glyphs.js"
    out = [
        "/* Generated by tools/generate_preview_glyphs.py — do not edit by hand. */",
        "/* Sourced from the Adafruit GFX Library bundled via PlatformIO. */",
        "(function (global) {",
        "  'use strict';",
        "",
        "  /* Default Adafruit GFX 5x7 font (advanced as 6x8). 256 glyphs × 5 column-bytes. */",
        f"  /* Total bytes: {len(glcd_bytes)} */",
        "  var GLCD_FONT = new Uint8Array([",
        emit_uint8_rows(glcd_bytes),
        "  ]);",
        "",
        "  /* TomThumb 3x5 font (advanced as 4x6), ASCII 0x20..0x7E only. */",
        "  var TOMTHUMB_BITMAPS = new Uint8Array([",
        emit_uint8_rows(tt_bitmaps),
        "  ]);",
        "",
        "  /* [bitmapOffset, width, height, xAdvance, xOffset, yOffset] */",
        "  var TOMTHUMB_GLYPHS = [",
        emit_glyph_rows(tt_glyphs),
        "  ];",
        "",
        "  /* Picopixel 4x5-ish font (yAdvance=7), ASCII 0x20..0x7E only. */",
        "  var PICOPIXEL_BITMAPS = new Uint8Array([",
        emit_uint8_rows(pico_bitmaps),
        "  ]);",
        "",
        "  var PICOPIXEL_GLYPHS = [",
        emit_glyph_rows(pico_glyphs),
        "  ];",
        "",
        "  global.FWGlyphs = {",
        "    GLCD_FONT: GLCD_FONT,",
        "    TOMTHUMB_BITMAPS: TOMTHUMB_BITMAPS,",
        "    TOMTHUMB_GLYPHS: TOMTHUMB_GLYPHS,",
        "    TOMTHUMB_FIRST: 0x20,",
        "    TOMTHUMB_LAST: 0x7E,",
        "    PICOPIXEL_BITMAPS: PICOPIXEL_BITMAPS,",
        "    PICOPIXEL_GLYPHS: PICOPIXEL_GLYPHS,",
        "    PICOPIXEL_FIRST: 0x20,",
        "    PICOPIXEL_LAST: 0x7E,",
        "  };",
        "})(typeof window !== 'undefined' ? window : this);",
        "",
    ]

    out_path.write_text("\n".join(out))
    print(
        f"Wrote {out_path} — GLCD {len(glcd_bytes)} bytes, "
        f"TomThumb {len(tt_bitmaps)} / {len(tt_glyphs)} glyphs, "
        f"Picopixel {len(pico_bitmaps)} / {len(pico_glyphs)} glyphs"
    )


if __name__ == "__main__":
    main()
