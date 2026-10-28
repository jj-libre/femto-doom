# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2026 Jakob Jungreuthmayer
"""The map WAD: each map in the layout the engine reads in place, vertexes folded into lines and
segs, sides shared and numbered by byte, flats replaced by colours, blockmap lists shared, then
the thing and geometry lists of every map."""

from __future__ import annotations

import math
import re
import struct
from collections.abc import Callable, Sequence
from typing import Any, NamedTuple

from .level import NO_SECTOR, NO_SIDE, Level, Line, Node, Sector, Seg, Side, Subsector, Thing
from .lists import geometry_lists, number_changeable_sides, thing_lists
from .palette import Color, Palette, closest, parse_palette
from .wad import Lump, Wad, name8

MAP_MARKER = re.compile(r"E\dM\d|MAP\d\d")
SOURCE_LUMPS = (
    "THINGS",
    "LINEDEFS",
    "SIDEDEFS",
    "VERTEXES",
    "SEGS",
    "SSECTORS",
    "NODES",
    "SECTORS",
    "REJECT",
    "BLOCKMAP",
)
SOURCE_NO_SIDE = -1
MAX_SECTORS = 255
ML_TWOSIDED = 4
MTF_NOTSINGLE = 16
MULTIPLAYER_STARTS = {2, 3, 4, 11}
ANG90 = 0x4000
FLAT_SKY = -2
FLAT_NUKAGE = -3


class Vertex(NamedTuple):
    x: int
    y: int


class SourceLine(NamedTuple):
    v1: Vertex
    v2: Vertex
    sides: tuple[int, int]
    flags: int
    special: int
    tag: int


def records(data: bytes, fmt: str) -> list[tuple[Any, ...]]:
    return list(struct.iter_unpack("<" + fmt, data))


def byte(value: int) -> int:
    """A short the source stores where the engine keeps a byte."""
    if not -128 <= value < 256:
        raise ValueError(f"{value} does not fit in a byte")
    return value & 0xFF


def short(value: int) -> int:
    if not -0x8000 <= value < 0x10000:
        raise ValueError(f"{value} does not fit in a short")
    return (value + 0x8000) % 0x10000 - 0x8000


def build_maps(iwad: Wad, textures: Sequence[str]) -> Wad:
    """Each map's marker and lumps, then E1MxTL and E1MxGL of every map."""
    palette = parse_palette(iwad["PLAYPAL"])
    maps = Wad()
    lists: list[Lump] = []
    for index, marker in enumerate(iwad.lumps):
        if not MAP_MARKER.fullmatch(marker.name):
            continue
        source = {}
        for lump in iwad.lumps[index + 1 : index + 1 + len(SOURCE_LUMPS)]:
            if lump.name not in SOURCE_LUMPS or lump.name in source:
                break
            source[lump.name] = lump.data
        level = MapConverter(marker.name, source, textures, iwad, palette).convert()
        lists += [
            Lump(marker.name + "TL", thing_lists(level)),
            Lump(marker.name + "GL", geometry_lists(level)),
        ]
        level.sides = number_changeable_sides(level)
        maps.lumps += [Lump(marker.name, b""), *(Lump(name, data) for name, data in level.lumps().items())]
    maps.lumps += lists
    print(f"thing and geometry lists: {len(lists) // 2} maps, {sum(len(lump.data) for lump in lists)} B")
    return maps


class MapConverter:
    """One map from the IWAD; `lumps` maps each name in SOURCE_LUMPS to its data."""

    def __init__(self, name: str, lumps: dict[str, bytes], textures: Sequence[str], wad: Wad, palette: Palette) -> None:
        self.name = name
        self.lumps = lumps
        self.textures = {texture: i for i, texture in reversed(list(enumerate(textures)))}
        self.wad = wad
        self.palette = palette
        self.vertexes = [Vertex(*v) for v in records(lumps["VERTEXES"], "hh")]

    def convert(self) -> Level:
        sectors = self.sectors()
        if len(sectors) > MAX_SECTORS:
            raise ValueError(f"{self.name}: {len(sectors)} sectors, at most {MAX_SECTORS}: sides store a byte")
        lines, sides = share_sides(self.source_lines(), self.sides())
        return Level(
            self.name,
            self.things(),
            [engine_line(number, line) for number, line in enumerate(lines)],
            sides,
            self.segs(lines, sides),
            [Subsector(*s) for s in records(self.lumps["SSECTORS"], "hh")],
            Node.unpack(self.lumps["NODES"]),
            sectors,
            self.lumps["REJECT"],
            self.blockmap(),
        )

    def things(self) -> list[Thing]:
        out = []
        for x, y, angle, kind, options in records(self.lumps["THINGS"], "hhhhh"):
            if kind in MULTIPLAYER_STARTS or options & MTF_NOTSINGLE:
                continue
            direction = int(angle / 45)
            if not 0 <= direction < 256:
                raise ValueError(f"{self.name}: thing angle {angle}")
            out.append(Thing(x, y, kind, direction, byte(options)))
        return out

    def source_lines(self) -> list[SourceLine]:
        return [
            SourceLine(self.vertexes[v1], self.vertexes[v2], (front, back), byte(flags), byte(special), byte(tag))
            for v1, v2, flags, special, tag, front, back in records(self.lumps["LINEDEFS"], "hhhhhhh")
        ]

    def texture(self, name: bytes) -> int:
        number = self.textures.get(name8(name), 0)
        if number > 0xFF:
            raise ValueError(f"{self.name}: texture {name!r} is number {number}, sides store a byte")
        return number

    def sides(self) -> list[Side]:
        return [
            Side(offset, byte(rowoffset), self.texture(top), self.texture(bottom), self.texture(mid), byte(sector))
            for offset, rowoffset, top, bottom, mid, sector in records(self.lumps["SIDEDEFS"], "hh8s8s8sh")
        ]

    def segs(self, lines: list[SourceLine], sides: list[Side]) -> list[Seg]:
        out = []
        for v1, v2, angle, linenum, direction, offset in records(self.lumps["SEGS"], "hhhhhh"):
            line = lines[linenum]
            side = line.sides[direction]
            back = NO_SECTOR
            if line.flags & ML_TWOSIDED and line.sides[direction ^ 1] != SOURCE_NO_SIDE:
                back = sides[line.sides[direction ^ 1]].sector
            start, end = self.vertexes[v1], self.vertexes[v2]
            angle = short(angle + ANG90)
            out.append(Seg(start.x, start.y, end.x, end.y, offset, angle, side, linenum, sides[side].sector, back))
        return out

    def sectors(self) -> list[Sector]:
        colors = FlatColors(self.wad, self.palette)
        return [
            Sector(
                floor,
                ceiling,
                colors.floor(name8(floorpic)),
                colors.ceiling(name8(ceilingpic)),
                byte(light),
                byte(special),
                tag,
            )
            for floor, ceiling, floorpic, ceilingpic, light, special, tag in records(self.lumps["SECTORS"], "hh8s8shhh")
        ]

    def blockmap(self) -> tuple[int, ...]:
        data = self.lumps["BLOCKMAP"]
        orgx, orgy, width, height = struct.unpack_from("<4h", data)
        words = struct.unpack(f"<{len(data) // 2}h", data)
        blocks = []
        for block in range(width * height):
            at = words[4 + block] + 1
            blocks.append(list(words[at : words.index(-1, at)]))
        return pack_blockmap(orgx, orgy, width, height, blocks)


def share_sides(lines: list[SourceLine], sides: list[Side]) -> tuple[list[SourceLine], list[Side]]:
    """Lines without a special share equal sides; the sides of special lines stay their own."""
    out: list[Side] = []
    shared: dict[Side, int] = {}

    def own(side: Side) -> int:
        out.append(side)
        return len(out) - 1

    def share(side: Side) -> int:
        if side not in shared:
            shared[side] = own(side)
        return shared[side]

    new_lines = []
    for line in lines:
        add: Callable[[Side], int] = own if line.special else share
        front, back = line.sides
        new_sides = (add(sides[front]), SOURCE_NO_SIDE if back == SOURCE_NO_SIDE else add(sides[back]))
        new_lines.append(line._replace(sides=new_sides))
    return new_lines, out


def engine_line(number: int, line: SourceLine) -> Line:
    (x1, y1), (x2, y2) = line.v1, line.v2
    dx, dy = short(x2 - x1), short(y2 - y1)
    if dx == 0:
        slope = 1
    elif dy == 0:
        slope = 0
    elif dx ^ dy >= 0:
        slope = 2
    else:
        slope = 3
    front, back = (NO_SIDE if s == SOURCE_NO_SIDE else s for s in line.sides)
    return Line(
        x1,
        y1,
        x2,
        y2,
        number,
        dx,
        dy,
        front,
        back,
        max(y1, y2),
        min(y1, y2),
        min(x1, x2),
        max(x1, x2),
        line.tag,
        line.flags,
        slope,
        line.special,
    )


def pack_blockmap(orgx: int, orgy: int, width: int, height: int, blocks: list[list[int]]) -> tuple[int, ...]:
    """Longest lists first; a list that ends a stored one points into it."""
    words = [orgx, orgy, width, height] + [-1] * len(blocks)
    stored: list[tuple[list[int], int]] = []
    for block in sorted(range(len(blocks)), key=lambda b: len(blocks[b]), reverse=True):
        lines = blocks[block]
        offset = next(
            (
                at + len(other) - len(lines)
                for other, at in stored
                if len(lines) <= len(other) and other[len(other) - len(lines) :] == lines
            ),
            None,
        )
        if offset is None:
            offset = len(words)
            words += [0, *lines, -1]
            stored.append((lines, offset))
        words[4 + block] = short(offset)
    return tuple(words)


class FlatColors:
    """Each flat of a map as the palette colour nearest its average; no two flats share one."""

    def __init__(self, wad: Wad, palette: Palette) -> None:
        self.wad = wad
        self.free = dict(enumerate(palette))
        self.palette = palette
        self.assigned: dict[str, int] = {}

    def floor(self, flat: str) -> int:
        return FLAT_NUKAGE if flat.startswith("NUKAGE") else self.color(flat)

    def ceiling(self, flat: str) -> int:
        return FLAT_SKY if flat == "F_SKY1" else self.floor(flat)

    def color(self, flat: str) -> int:
        if flat not in self.assigned:
            average = blend([self.palette[pixel] for pixel in self.wad[flat]])
            index = closest(average, self.free)
            del self.free[index]
            self.assigned[flat] = index
        return self.assigned[flat]


def blend(colors: Sequence[Color]) -> Color:
    """The root mean square of each channel."""
    n = len(colors)
    return Color(*(math.isqrt(sum(c[k] * c[k] for c in colors) // n) for k in range(3)))
