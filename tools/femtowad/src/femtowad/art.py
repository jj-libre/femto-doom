"""Lumps femto-doom reads that a Doom IWAD lacks or holds at another size, made from the IWAD.

Pictures the platform scales are kept as they are. Full-screen pictures become raw images of the
screen's size. The status bar keeps Doom's layout scaled to the screen: its background becomes a
raw image as wide as the screen and ST_HEIGHT high, and the pictures drawn on it are scaled by the
same factors, except that the smallest, the 4 x 6 digits and the key cards, never shrink. On a
320 x 200 screen all of them stay at Doom's size.
"""

from __future__ import annotations

import math
from collections.abc import Iterable, Sequence
from typing import NamedTuple

from .palette import Color, Palette, closest, parse_palette
from .picture import Picture, Pixels
from .wad import Wad

FULL_SCREEN_PICTURES = ("HELP2", "TITLEPIC", "WIMAP0")
BAR_PICTURE_PREFIXES = ("STYSNUM", "STGNUM", "STKEYS", "STF")
DOOM_SCREEN = (320, 200)
DOOM_BAR_HEIGHT = 32
SMALL_PICTURE_ROWS = 6
DOOM_PERCENT_AT = ((90, 3), (221, 3))
ARMS_AT = (104, 0)
GAMMA_LEVELS = 5
FONT_SCALE = 2
FONT_SPACE = 4
MENU_TEXT = {"M_ARUN": "ALWAYS RUN:", "M_GAMMA": "GAMMA BOOST:"}


class Screen(NamedTuple):
    width: int
    height: int

    @classmethod
    def parse(cls, text: str) -> Screen:
        width, height = (int(n) for n in text.lower().split("x"))
        return cls(width, height)

    @property
    def bar_height(self) -> int:
        """ST_HEIGHT: Doom's 32 status bar rows of 200, scaled, at least 32."""
        return max(DOOM_BAR_HEIGHT, (DOOM_BAR_HEIGHT * self.height + 100) // 200)

    @property
    def bar_scale(self) -> tuple[float, float]:
        return self.width / DOOM_SCREEN[0], self.bar_height / DOOM_BAR_HEIGHT


def generate_art(iwad: Wad, screen: Screen, names: Iterable[str]) -> dict[str, bytes]:
    """New lumps first, in the order they are appended, then those replacing the IWAD's.

    `names` are the lumps the engine reads; the status bar's pictures among them are scaled.
    """
    palette = parse_palette(iwad["PLAYPAL"])
    doom_size = screen == DOOM_SCREEN
    art = {name: menu_text(iwad, text) for name, text in MENU_TEXT.items()}
    for digit in range(10):
        art[f"STGANUM{digit}"] = bar_picture(iwad[f"STTNUM{digit}"], screen, palette)
    for level in range(1, GAMMA_LEVELS + 1):
        art[f"PLAYPAL{level}"] = gamma_corrected(iwad["PLAYPAL"], level)
    for name in FULL_SCREEN_PICTURES:
        pixels = Picture.parse(iwad[name]).pixels()
        art[name] = raw(pixels if doom_size else resample(pixels, *screen, palette))
    bar = doom_status_bar(iwad)
    art["STBAR"] = raw(bar if screen.bar_scale == (1, 1) else resample(bar, screen.width, screen.bar_height, palette))
    for name in sorted(names):
        if name.startswith(BAR_PICTURE_PREFIXES):
            art[name] = bar_picture(iwad[name], screen, palette)
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


def scaled(n: int, scale: float) -> int:
    return math.floor(n * scale)


def bar_picture(lump: bytes, screen: Screen, palette: Palette) -> bytes:
    """A picture drawn on the status bar, scaled as the bar is; the smallest never shrink."""
    sx, sy = screen.bar_scale
    picture = Picture.parse(lump)
    if picture.height <= SMALL_PICTURE_ROWS:
        sx, sy = max(sx, 1), max(sy, 1)
    if (sx, sy) == (1, 1):
        return lump
    width, height = max(1, scaled(picture.width, sx)), max(1, scaled(picture.height, sy))
    pixels = resample(picture.pixels(), width, height, palette)
    return Picture.from_pixels(pixels, scaled(picture.left, sx), scaled(picture.top, sy)).encode()


def draw(target: Pixels, pixels: Pixels, x: int, y: int) -> None:
    for dy, row in enumerate(pixels):
        for dx, p in enumerate(row):
            if p is not None:
                target[y + dy][x + dx] = p


def doom_status_bar(iwad: Wad) -> Pixels:
    """The IWAD's bar with the arms panel and percent signs Doom draws over it."""
    bar = Picture.parse(iwad["STBAR"]).pixels()
    draw(bar, Picture.parse(iwad["STARMS"]).pixels(), *ARMS_AT)
    percent = Picture.parse(iwad["STTPRCNT"]).pixels()
    for x, y in DOOM_PERCENT_AT:
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
