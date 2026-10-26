"""Per map lumps that spare the engine the memory of spawning things and grouping lines.

E1MxTL holds the map's things, spawned in advance: each thing's spawn data, subsector
and slot per skill, and, in reverse load order, the things in each sector and block and the
sectors each thing touches. E1MxGL holds the lines, grouped in advance: the lines of each sector
and the sector of each subsector. Sides play can change, those at the front of lines with a
special, get a number.
"""

from __future__ import annotations

import struct
from collections.abc import Sequence

from .info import MF, MOBJS
from .level import NO_SECTOR, Level, Side

SKILL_GROUPS = (1, 2, 4)
FIRST_STATIC = next(i for i, mobj in enumerate(MOBJS) if mobj.name == "MISC0")
"""Thing types from here on never think."""
MTF_AMBUSH = 8
NO_MOBJTYPE = 0xFF
PLAYER_START = 1
MAX_CHANGEABLE_SIDES = 255


def csr(keys: Sequence[int | None], size: int) -> list[int]:
    """Where each key's run starts in a list sorted by key, and the end."""
    first = [0] * (size + 1)
    for key in keys:
        if key is not None:
            first[key + 1] += 1
    for k in range(size):
        first[k + 1] += first[k]
    return first


def pack_arrays(header: bytes, arrays: Sequence[bytes]) -> bytes:
    """The header, a 32-bit offset per array, then the arrays."""
    at = len(header) + 4 * len(arrays)
    offsets = []
    for array in arrays:
        offsets.append(at)
        at += len(array)
    return header + struct.pack(f"<{len(offsets)}I", *offsets) + b"".join(arrays)


def shorts(values: Sequence[int]) -> bytes:
    return struct.pack(f"<{len(values)}h", *values)


def thing_lists(level: Level) -> bytes:
    by_doomednum: dict[int, int] = {}
    for number, mobj in enumerate(MOBJS):
        by_doomednum.setdefault(mobj.doomednum, number)
    counts = [[0, 0] for _ in SKILL_GROUPS]
    entries = []
    sectors: list[int | None] = []
    blocks: list[int | None] = []
    touching: list[list[int]] = []
    for thing in level.things:
        mobjtype = 0 if thing.doomednum == PLAYER_START else by_doomednum.get(thing.doomednum)
        slots = [-1] * len(SKILL_GROUPS)
        sector = block = None
        subsector = -1
        touched: list[int] = []
        if mobjtype is not None:
            static = mobjtype >= FIRST_STATIC
            for group, bit in enumerate(SKILL_GROUPS):
                if thing.doomednum == PLAYER_START or thing.options & bit:
                    slots[group] = counts[group][static]
                    counts[group][static] += 1
            info = MOBJS[mobjtype]
            x, y = thing.x << 16, thing.y << 16
            subsector = level.point_subsector(x, y)
            if not info.flags & MF.NOSECTOR:
                sector = level.subsector_sector[subsector]
                touched = sorted(level.touched_sectors(x, y, info.radius))
            if not info.flags & MF.NOBLOCKMAP:
                block = level.block(x, y)
        angle = (thing.angle & 0x7F) | (0x80 if thing.options & MTF_AMBUSH else 0)
        entries.append(
            struct.pack(
                "<hhBB3hh", thing.x, thing.y, NO_MOBJTYPE if mobjtype is None else mobjtype, angle, *slots, subsector
            )
        )
        sectors.append(sector)
        blocks.append(block)
        touching.append(touched)

    numthings = len(entries)
    sectorfirst = csr(sectors, level.numsectors)
    blockfirst = [v + sectorfirst[-1] for v in csr(blocks, level.numblocks)]
    ordered = [0] * blockfirst[-1]
    fill = sectorfirst[:-1] + blockfirst[:-1]
    for t in reversed(range(numthings)):
        sector, block = sectors[t], blocks[t]
        for key in (sector, None if block is None else level.numsectors + block):
            if key is not None:
                ordered[fill[key]] = t
                fill[key] += 1

    nodefirst = [0]
    nodesector: list[int] = []
    nodething: list[int] = []
    for t, touched in enumerate(touching):
        nodesector += touched
        nodething += [t] * len(touched)
        nodefirst.append(len(nodesector))
    numnodes = len(nodesector)
    secnodefirst = csr(nodesector, level.numsectors)
    secnodes = [0] * numnodes
    fill = secnodefirst[:-1]
    for k in reversed(range(numnodes)):
        secnodes[fill[nodesector[k]]] = k
        fill[nodesector[k]] += 1

    header = struct.pack("<4H", numthings, level.numsectors, level.numblocks, numnodes)
    header += struct.pack("<6H", *(c[1] for c in counts), *(c[0] for c in counts))
    arrays = [b"".join(entries)] + [
        shorts(a) for a in (sectorfirst, blockfirst, ordered, nodefirst, nodesector, nodething, secnodefirst, secnodes)
    ]
    return pack_arrays(header, arrays)


def geometry_lists(level: Level) -> bytes:
    lines: list[list[int]] = [[] for _ in range(level.numsectors)]
    for number, line in enumerate(level.lines):
        for sector in level.side_sectors(line):
            lines[sector].append(number)
    first = [0]
    for sector_lines in lines:
        first.append(first[-1] + len(sector_lines))
    flat = [number for sector_lines in lines for number in sector_lines]
    sectors = bytes(NO_SECTOR if s is None else s for s in level.subsector_sector)
    header = struct.pack("<2H", level.numsectors, len(sectors))
    return pack_arrays(header, [struct.pack(f"<{len(first)}H", *first), struct.pack(f"<{len(flat)}H", *flat), sectors])


def number_changeable_sides(level: Level) -> list[Side]:
    """The sides at the front of lines with a special, numbered from 1."""
    numbers: dict[int, int] = {}
    for line in level.lines:
        if line.special and line.front not in numbers:
            numbers[line.front] = len(numbers) + 1
    if len(numbers) > MAX_CHANGEABLE_SIDES:
        raise ValueError(f"{level.name}: {len(numbers)} changeable sides, at most {MAX_CHANGEABLE_SIDES}")
    return [side._replace(changeable=numbers.get(i, 0)) for i, side in enumerate(level.sides)]
