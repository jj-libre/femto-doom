"""Lumps femto-doom reads that a Doom IWAD lacks or holds at another size, made from the IWAD.

Full-screen pictures become 240 x 160 raw images. The status bar is rebuilt 240 wide from the
IWAD's own bar, with its widgets where st_stuff.c draws them: labels and digit cells are copied,
plain texture between them is thinned out. Big digits and the percent sign are narrowed to 10
pixels.
"""

from __future__ import annotations

import math
from collections.abc import Sequence
from itertools import pairwise
from typing import NamedTuple

from .palette import Color, Palette, closest, parse_palette
from .picture import Picture, Pixels
from .wad import Wad

SCREEN_WIDTH, SCREEN_HEIGHT = 240, 160
FULL_SCREEN_PICTURES = ("HELP2", "TITLEPIC", "WIMAP0")
BAR_WIDTH, BAR_HEIGHT = 240, 32
LABEL_ROWS = 23
TALL_DIGIT_WIDTH = 10
PERCENT_AT = ((66, 6), (166, 6))
ARMS_AT = (104, 0)
GAMMA_LEVELS = 5
FONT_SCALE = 2
FONT_SPACE = 4
MENU_TEXT = {"M_ARUN": "ALWAYS RUN:", "M_GAMMA": "GAMMA BOOST:"}


class Anchor(NamedTuple):
    """Bar column `x` shows source column `source`; equal steps between anchors copy columns.

    Per panel of the bar, BAR_TOP anchors the rows above the labels and BAR_LABELS the rows
    with them. Labels, digit cells and edges are copied, the texture between is thinned out.
    """

    x: int
    source: int


BAR_TOP = {
    "ammo": ((0, 0), (2, 2), (33, 46), (35, 48)),
    "health": ((37, 50), (77, 104), (78, 105)),
    "arms": ((79, 106), (82, 111), (86, 115), (90, 123), (94, 127), (98, 135), (102, 139), (107, 143)),
    "face": ((108, 144), (134, 177), (135, 178)),
    "armor": ((137, 180), (176, 235), (178, 237)),
    "keys": ((178, 239), (187, 248)),
    "ammo table": ((187, 249), (189, 251), (189, 254), (206, 271), (220, 292), (224, 296), (238, 318), (240, 320)),
}
BAR_LABELS = BAR_TOP | {
    "ammo": ((0, 0), (2, 2), (2, 8), (33, 39), (33, 46), (35, 48)),
    "health": ((37, 50), (37, 55), (77, 95), (77, 104), (78, 105)),
    "arms": ((79, 106), (79, 110), (106, 137), (107, 143)),
    "armor": ((137, 180), (137, 188), (173, 224), (176, 235), (178, 237)),
}


def generate_art(iwad: Wad) -> dict[str, bytes]:
    """New lumps first, in the order they are appended, then those replacing the IWAD's."""
    palette = parse_palette(iwad["PLAYPAL"])
    art = {name: menu_text(iwad, text) for name, text in MENU_TEXT.items()}
    for digit in range(10):
        art[f"STGANUM{digit}"] = narrow(Picture.parse(iwad[f"STTNUM{digit}"]), palette).encode()
    for level in range(1, GAMMA_LEVELS + 1):
        art[f"PLAYPAL{level}"] = gamma_corrected(iwad["PLAYPAL"], level)
    for name in FULL_SCREEN_PICTURES:
        pixels = Picture.parse(iwad[name]).pixels()
        art[name] = raw(resample(pixels, SCREEN_WIDTH, SCREEN_HEIGHT, palette))
    art["STBAR"] = raw(status_bar(iwad, palette))
    return art


def raw(pixels: Pixels) -> bytes:
    return bytes(0 if p is None else p for row in pixels for p in row)


def gamma_corrected(playpal: bytes, level: int) -> bytes:
    """PLAYPAL brightened as Doom's gamma tables do: v' = 255 (v / 255) ^ (1 - level / 8)."""
    exponent = 1 - level / 8
    table = bytes(round(255 * (v / 255) ** exponent) for v in range(256))
    return playpal.translate(table)


class Resampler:
    """Box filtering in squared channel values, then the nearest palette colour."""

    def __init__(self, palette: Palette) -> None:
        self.palette = palette
        self.candidates = dict(enumerate(palette))
        self.nearest: dict[Color, int] = {}

    def color(self, weighted: Sequence[tuple[float, int]]) -> int:
        total = sum(w for w, _ in weighted)
        mean = Color(
            *(math.isqrt(round(sum(w * self.palette[p][k] ** 2 for w, p in weighted) / total)) for k in range(3))
        )
        if mean not in self.nearest:
            self.nearest[mean] = closest(mean, self.candidates)
        return self.nearest[mean]


def box_weights(source: int, target: int) -> list[list[tuple[float, int]]]:
    """Per target cell, the overlapping source cells and how much of each it covers."""
    scale = source / target
    out = []
    for t in range(target):
        start, end = t * scale, (t + 1) * scale
        cells = []
        for s in range(math.floor(start), math.ceil(end)):
            overlap = min(end, s + 1) - max(start, s)
            if overlap > 1e-9:
                cells.append((overlap, s))
        out.append(cells)
    return out


def resample(pixels: Pixels, width: int, height: int, palette: Palette) -> Pixels:
    """Transparent where most of the covered area is transparent."""
    columns = box_weights(len(pixels[0]), width)
    rows = box_weights(len(pixels), height)
    resampler = Resampler(palette)
    out: Pixels = []
    for row_cells in rows:
        line: list[int | None] = []
        for column_cells in columns:
            weighted = [(wy * wx, pixels[y][x]) for wy, y in row_cells for wx, x in column_cells]
            opaque = [(w, p) for w, p in weighted if p is not None]
            if sum(w for w, _ in opaque) * 2 <= sum(w for w, _ in weighted):
                line.append(None)
            else:
                line.append(resampler.color(opaque))
        out.append(line)
    return out


def narrow(picture: Picture, palette: Palette) -> Picture:
    pixels = resample(picture.pixels(), TALL_DIGIT_WIDTH, picture.height, palette)
    return Picture.from_pixels(pixels, picture.left, picture.top)


def draw(target: Pixels, pixels: Pixels, x: int, y: int) -> None:
    for dy, row in enumerate(pixels):
        for dx, p in enumerate(row):
            if p is not None:
                target[y + dy][x + dx] = p


def column_map(panels: dict[str, tuple[tuple[int, int], ...]]) -> list[int]:
    """The source column of every bar column; a repeated `x` jumps in the source."""
    points = [Anchor(*a) for anchors in panels.values() for a in anchors]
    out = []
    for a, b in pairwise(points):
        width = b.x - a.x
        out += [a.source + (b.source - a.source) * k // width for k in range(width)]
    if len(out) != BAR_WIDTH:
        raise AssertionError(f"status bar column map is {len(out)} wide")
    return out


def status_bar(iwad: Wad, palette: Palette) -> Pixels:
    source = Picture.parse(iwad["STBAR"]).pixels()
    draw(source, Picture.parse(iwad["STARMS"]).pixels(), *ARMS_AT)
    top, labels = column_map(BAR_TOP), column_map(BAR_LABELS)
    bar: Pixels = [
        [source[y][(top if y < LABEL_ROWS else labels)[x]] for x in range(BAR_WIDTH)] for y in range(BAR_HEIGHT)
    ]
    percent = narrow(Picture.parse(iwad["STTPRCNT"]), palette).pixels()
    for x, y in PERCENT_AT:
        draw(bar, percent, x, y)
    return bar


def menu_text(iwad: Wad, text: str) -> bytes:
    """`text` in the heads-up font at twice its size."""
    glyphs = [None if ch == " " else Picture.parse(iwad[f"STCFN{ord(ch):03d}"]).pixels() for ch in text]
    height = max(len(g) for g in glyphs if g is not None)
    columns: list[list[int | None]] = []
    for glyph in glyphs:
        if glyph is None:
            columns += [[None] * height] * FONT_SPACE
        else:
            columns += [[glyph[y][x] if y < len(glyph) else None for y in range(height)] for x in range(len(glyph[0]))]
    pixels: Pixels = [
        [columns[x // FONT_SCALE][y // FONT_SCALE] for x in range(len(columns) * FONT_SCALE)]
        for y in range(height * FONT_SCALE)
    ]
    return Picture.from_pixels(pixels).encode()
