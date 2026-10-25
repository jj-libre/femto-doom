"""A map as the engine reads it, its lumps, and the engine's geometry queries on it with the
engine's integer wrapping."""

from __future__ import annotations

import struct
from collections.abc import Iterator, Sequence
from dataclasses import dataclass, field
from typing import NamedTuple

FRACBITS = 16
MAPBLOCKSHIFT = FRACBITS + 7
NF_SUBSECTOR = 0x8000
NO_SIDE = 0xFFFF
NO_SECTOR = 0xFF

THING = struct.Struct("<hhhBB")
LINE = struct.Struct("<hhhhHhhHHhhhhhBBh")
SIDE = struct.Struct("<hBBBBBB")
SEG = struct.Struct("<hhhhhhHHBB")
SUBSECTOR = struct.Struct("<hh")
NODE = struct.Struct("<hhhh8hHH")
SECTOR = struct.Struct("<hhhhBBh")


def i32(v: int) -> int:
    return (v + 0x80000000) % 0x100000000 - 0x80000000


def i16(v: int) -> int:
    return (v + 0x8000) % 0x10000 - 0x8000


class Thing(NamedTuple):
    x: int
    y: int
    doomednum: int
    angle: int
    """In steps of 45 degrees."""
    options: int


class Line(NamedTuple):
    v1x: int
    v1y: int
    v2x: int
    v2y: int
    number: int
    dx: int
    dy: int
    front: int
    back: int
    top: int
    bottom: int
    left: int
    right: int
    tag: int
    flags: int
    slope: int
    special: int


class Side(NamedTuple):
    textureoffset: int
    rowoffset: int
    toptexture: int
    bottomtexture: int
    midtexture: int
    sector: int
    changeable: int = 0
    """Numbered from 1 for the sides play can change."""


class Seg(NamedTuple):
    v1x: int
    v1y: int
    v2x: int
    v2y: int
    offset: int
    angle: int
    side: int
    line: int
    frontsector: int
    backsector: int


class Subsector(NamedTuple):
    numsegs: int
    firstseg: int


class Node(NamedTuple):
    x: int
    y: int
    dx: int
    dy: int
    right_box: tuple[int, int, int, int]
    left_box: tuple[int, int, int, int]
    right: int
    left: int

    @classmethod
    def unpack(cls, data: bytes) -> list[Node]:
        return [
            cls(x, y, dx, dy, (b[0], b[1], b[2], b[3]), (b[4], b[5], b[6], b[7]), right, left)
            for x, y, dx, dy, *b, right, left in NODE.iter_unpack(data)
        ]

    def flat(self) -> tuple[int, ...]:
        return (self.x, self.y, self.dx, self.dy, *self.right_box, *self.left_box, self.right, self.left)


class Sector(NamedTuple):
    floorheight: int
    ceilingheight: int
    floorpic: int
    """A palette colour, or FLAT_NUKAGE or FLAT_SKY."""
    ceilingpic: int
    lightlevel: int
    special: int
    tag: int


class Box(NamedTuple):
    top: int
    bottom: int
    left: int
    right: int


def pack(record: struct.Struct, rows: Sequence[tuple[int, ...]]) -> bytes:
    return b"".join(record.pack(*row) for row in rows)


@dataclass
class Level:
    name: str
    things: list[Thing]
    lines: list[Line]
    sides: list[Side]
    segs: list[Seg]
    subsectors: list[Subsector]
    nodes: list[Node]
    sectors: list[Sector]
    reject: bytes
    blockmap: tuple[int, ...]
    """Origin, size, an offset per block, then the line lists the offsets point into."""
    subsector_sector: list[int | None] = field(init=False)

    def __post_init__(self) -> None:
        self.orgx, self.orgy, self.bwidth, self.bheight = self.blockmap[:4]
        self.subsector_sector = [self.first_sector(subsector) for subsector in self.subsectors]

    def lumps(self) -> dict[str, bytes]:
        """In the order the engine expects after the map's marker."""
        return {
            "THINGS": pack(THING, self.things),
            "LINEDEFS": pack(LINE, self.lines),
            "SIDEDEFS": pack(SIDE, self.sides),
            "SEGS": pack(SEG, self.segs),
            "SSECTORS": pack(SUBSECTOR, self.subsectors),
            "NODES": pack(NODE, [node.flat() for node in self.nodes]),
            "SECTORS": pack(SECTOR, self.sectors),
            "REJECT": self.reject,
            "BLOCKMAP": struct.pack(f"<{len(self.blockmap)}h", *self.blockmap),
        }

    @property
    def numsectors(self) -> int:
        return len(self.sectors)

    @property
    def numblocks(self) -> int:
        return self.bwidth * self.bheight

    def first_sector(self, subsector: Subsector) -> int | None:
        for seg in self.segs[subsector.firstseg : subsector.firstseg + subsector.numsegs]:
            if seg.side != NO_SIDE:
                return self.sides[seg.side].sector
        return None

    def side_sectors(self, line: Line) -> Iterator[int]:
        """The sector in front of the line, then the one behind it if that is another."""
        front = self.sides[line.front].sector
        yield front
        if line.back != NO_SIDE and self.sides[line.back].sector != front:
            yield self.sides[line.back].sector

    @staticmethod
    def point_on_side(x: int, y: int, node: Node) -> int:
        ix = i16(x >> FRACBITS)
        if not node.dx:
            return int(node.dy > 0) if ix <= node.x else int(node.dy < 0)
        iy = i16(y >> FRACBITS)
        if not node.dy:
            return int(node.dx < 0) if iy <= node.y else int(node.dx > 0)
        x = i32(x - (node.x << FRACBITS))
        y = i32(y - (node.y << FRACBITS))
        ix, iy = i16(x >> FRACBITS), i16(y >> FRACBITS)
        if (node.dy ^ node.dx ^ ix ^ iy) < 0:
            return int((node.dy ^ ix) < 0)
        return int(i32((y >> 8) * node.dx) >= i32((x >> 8) * node.dy))

    def point_subsector(self, x: int, y: int) -> int:
        if not self.nodes:
            return 0
        n = len(self.nodes) - 1
        while not n & NF_SUBSECTOR:
            node = self.nodes[n]
            n = node.left if self.point_on_side(x, y, node) else node.right
        return n & ~NF_SUBSECTOR

    def point_sector(self, x: int, y: int) -> int | None:
        return self.subsector_sector[self.point_subsector(x, y)]

    def block_coordinate(self, v: int, origin: int) -> int:
        return i16(i32(v - (origin << FRACBITS)) >> MAPBLOCKSHIFT)

    def block(self, x: int, y: int) -> int | None:
        bx, by = self.block_coordinate(x, self.orgx), self.block_coordinate(y, self.orgy)
        if 0 <= bx < self.bwidth and 0 <= by < self.bheight:
            return by * self.bwidth + bx
        return None

    def block_lines(self, bx: int, by: int) -> list[int]:
        if not (0 <= bx < self.bwidth and 0 <= by < self.bheight):
            return []
        at = self.blockmap[4 + by * self.bwidth + bx] + 1
        return list(self.blockmap[at : self.blockmap.index(-1, at)])

    @staticmethod
    def line_side(x: int, y: int, line: Line) -> int:
        if not line.dx:
            return int(line.dy > 0) if x <= (line.v1x << FRACBITS) else int(line.dy < 0)
        if not line.dy:
            return int(line.dx < 0) if y <= (line.v1y << FRACBITS) else int(line.dx > 0)
        left = i32((i32(y - (line.v1y << FRACBITS)) >> 8) * line.dx)
        right = i32(line.dy * (i32(x - (line.v1x << FRACBITS)) >> 8))
        return int(left >= right)

    def box_on_line_side(self, box: Box, line: Line) -> int:
        """0 or 1 for the side the whole box is on, -1 if the line crosses it."""
        if line.slope == 1:
            right = box.right < (line.v1x << FRACBITS)
            return int(right) ^ int(line.dy < 0) if (box.left < (line.v1x << FRACBITS)) == right else -1
        if line.slope == 2:
            side = self.line_side(box.left, box.top, line)
            return side if self.line_side(box.right, box.bottom, line) == side else -1
        if line.slope == 3:
            side = self.line_side(box.right, box.top, line)
            return side if self.line_side(box.left, box.bottom, line) == side else -1
        above = box.top > (line.v1y << FRACBITS)
        return int(above) ^ int(line.dx < 0) if (box.bottom > (line.v1y << FRACBITS)) == above else -1

    def touched_sectors(self, x: int, y: int, radius: int) -> set[int]:
        """The sectors a thing's box touches, as P_CreateSecNodeList finds them."""
        box = Box(i32(y + radius), i32(y - radius), i32(x - radius), i32(x + radius))
        seen: set[int] = set()
        sectors: set[int] = set()
        for bx in range(self.block_coordinate(box.left, self.orgx), self.block_coordinate(box.right, self.orgx) + 1):
            for by in range(
                self.block_coordinate(box.bottom, self.orgy), self.block_coordinate(box.top, self.orgy) + 1
            ):
                for number in self.block_lines(bx, by):
                    if number in seen:
                        continue
                    seen.add(number)
                    line = self.lines[number]
                    if (
                        box.right <= line.left << FRACBITS
                        or box.left >= line.right << FRACBITS
                        or box.top <= line.bottom << FRACBITS
                        or box.bottom >= line.top << FRACBITS
                    ):
                        continue
                    if self.box_on_line_side(box, line) != -1:
                        continue
                    sectors.update(self.side_sectors(line))
        sector = self.point_sector(x, y)
        if sector is not None:
            sectors.add(sector)
        return sectors
