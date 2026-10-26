"""r_view.c and r_view.h: the tangent table and, per supported view width, the tables that map
view angles to screen columns and back, as Doom's R_InitTextureMapping computes them."""

from __future__ import annotations

from collections.abc import Sequence
from dataclasses import dataclass

from .trig import FINETANGENT

VIEW_WIDTHS = (30, 38, 40, 60, 80, 120, 160, 240, 320)
"""The values VIEW_WINDOW_WIDTH may take."""

FRACBITS = 16
FRACUNIT = 1 << FRACBITS
FINEANGLES = 8192
FIELDOFVIEW = 2048
ANGLETOFINESHIFT = 19
ANG90 = 0x40000000
COMPACT_END = 3073
"""View angles from here on map to column 0."""
FULL_TABLE = -1023
"""VIEWANGLETOXMAX of a table that covers every view angle."""


@dataclass(frozen=True)
class ViewTables:
    width: int
    viewangletox: tuple[int, ...]
    """From VIEWANGLETOXMAX on, or every angle when that is FULL_TABLE."""
    viewangletoxmax: int
    xtoviewangle: tuple[int, ...]
    """The upper 16 bits of each column's angle."""


def fixed_mul(a: int, b: int) -> int:
    return (a * b) >> FRACBITS


def view_tables(width: int) -> ViewTables:
    centerxfrac = (width // 2) << FRACBITS
    focallength = int(centerxfrac * FRACUNIT / FINETANGENT[FINEANGLES // 4 + FIELDOFVIEW // 2])
    columns = []
    for tangent in FINETANGENT:
        if tangent > FRACUNIT * 2:
            column = -1
        elif tangent < -FRACUNIT * 2:
            column = width + 1
        else:
            column = (centerxfrac - fixed_mul(tangent, focallength) + FRACUNIT - 1) >> FRACBITS
            column = max(-1, min(width + 1, column))
        columns.append(column)
    xtoviewangle = []
    for x in range(width + 1):
        angle = next(i for i, column in enumerate(columns) if column <= x)
        xtoviewangle.append((((angle << ANGLETOFINESHIFT) - ANG90) & 0xFFFFFFFF) >> 16)
    clamped = [0 if c == -1 else width if c == width + 1 else c for c in columns]
    start = next(i for i, c in enumerate(clamped) if c != width)
    if all(c == 0 for c in clamped[COMPACT_END:]):
        return ViewTables(width, tuple(clamped[start:COMPACT_END]), start, tuple(xtoviewangle))
    return ViewTables(width, tuple(clamped), FULL_TABLE, tuple(xtoviewangle))


def rows(values: Sequence[int], per_line: int, hex_digits: int = 0) -> str:
    text = [f"0x{v:0{hex_digits}X}" if hex_digits else str(v) for v in values]
    return "".join("\t" + ", ".join(text[i : i + per_line]) + ",\n" for i in range(0, len(text), per_line))


def width_chain(blocks: dict[int, str]) -> str:
    out = []
    for n, (width, body) in enumerate(blocks.items()):
        out.append(f"#{'if' if n == 0 else 'elif'} VIEW_WINDOW_WIDTH == {width}\n{body}")
    out.append("#else\n#error unsupported VIEW_WINDOW_WIDTH value\n#endif\n")
    return "".join(out)


def r_view_h(licence: str) -> str:
    tables = [view_tables(w) for w in VIEW_WIDTHS]
    maxima = width_chain({t.width: f"#define VIEWANGLETOXMAX {t.viewangletoxmax}\n" for t in tables})
    return f"""{licence}
#ifndef __R_VIEW__
#define __R_VIEW__

#include <stdint.h>

#include "doomdef.h"
#include "m_fixed.h"
#include "tables.h"

{maxima}
#if VIEW_WINDOW_WIDTH < 256
typedef uint8_t viewx_t;
#else
typedef uint16_t viewx_t;
#endif

// The column each view angle falls on, from VIEWANGLETOXMAX to 3073.
extern const viewx_t viewangletoxTable[4096 - 1023 - VIEWANGLETOXMAX];

// The view angle of each column.
extern const angle16_t xtoviewangleTable[VIEW_WINDOW_WIDTH + 1];

#if defined SMALL_TRIG
extern const fixed_t finetangentHalf[2048];
#else
extern const fixed_t finetangentTable[4096];
#endif

#endif
"""


def r_view_c(licence: str) -> str:
    tables = [view_tables(w) for w in VIEW_WIDTHS]
    toangle = width_chain({t.width: rows(t.xtoviewangle, 16, 4) for t in tables})
    tox = width_chain({t.width: rows(t.viewangletox, 16) for t in tables})
    return f"""{licence}
#include "r_view.h"

const angle16_t xtoviewangleTable[VIEW_WINDOW_WIDTH + 1] =
{{
{toangle}}};

const viewx_t viewangletoxTable[4096 - 1023 - VIEWANGLETOXMAX] =
{{
{tox}}};

#if defined SMALL_TRIG
const fixed_t finetangentHalf[2048] =
{{
{rows(FINETANGENT[:2048], 8)}}};
#else
const fixed_t finetangentTable[4096] =
{{
{rows(FINETANGENT, 8)}}};
#endif
"""
