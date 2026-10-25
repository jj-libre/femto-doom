"""Palette colours and nearest-colour search."""

from __future__ import annotations

from collections.abc import Mapping
from typing import NamedTuple


class Color(NamedTuple):
    r: int
    g: int
    b: int

    def distance(self, other: Color) -> int:
        return (self.r - other.r) ** 2 + (self.g - other.g) ** 2 + (self.b - other.b) ** 2


Palette = list[Color]


def parse_palette(playpal: bytes, number: int = 0) -> Palette:
    base = 768 * number
    return [Color(*playpal[base + 3 * i : base + 3 * i + 3]) for i in range(256)]


def closest(color: Color, candidates: Mapping[int, Color]) -> int:
    """The first index, in the mapping's order, of a candidate nearest to `color`."""
    return min(candidates, key=lambda index: color.distance(candidates[index]))
