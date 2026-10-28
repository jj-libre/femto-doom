# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2026 Jakob Jungreuthmayer
"""info.c and info.h written from info.py."""

from __future__ import annotations

from collections.abc import Iterable

from .cheader import licence
from .info import ACTIONS, FRACUNIT, MAX_SPRITE_FRAMES, MF, MF_COMMENTS, MOBJS, SPRITES, STATES, WAD_VERSION, MobjInfo

LICENCE = licence("Thing types, their states and sprites.", "info.py")

STATE_STRUCT = """\
#define FF_FULLBRIGHT   0x80  /* flag in thing->frame */
#define FF_FRAMEMASK    0x7f

typedef struct
{
  spritenum_t sprite;       /* sprite number to show                       */
  uint8_t     frame;        /* which frame/subframe of the sprite is shown */
  int8_t      tics;         /* number of gametics this frame should last   */
  uint8_t     action;       /* index into stateactions, AC_NULL if none    */
  uint16_t    nextstate;    /* linked list pointer to next state or zero   */
} state_t;
"""

MOBJINFO_STRUCT = """\
typedef struct
{
  int32_t speed;
  int32_t radius;
  int32_t height;
  uint32_t flags;         /* MF_ flags */
  int16_t doomednum;      /* thing number in maps */
  int16_t spawnhealth;
  int16_t reactiontime;   /* tics it waits after it wakes up before it attacks */
  int16_t mass;
  int16_t damage;         /* of a missile */
  uint16_t spawnstate;
  uint16_t seestate;      /* when it sees you or wakes up */
  uint16_t painstate;
  uint16_t meleestate;
  uint16_t missilestate;
  uint16_t deathstate;
  uint16_t xdeathstate;   /* gibbed */
  uint8_t painchance;     /* out of 256, the chance of going to painstate when hurt */
  uint8_t seesound;
  uint8_t attacksound;
  uint8_t painsound;
  uint8_t deathsound;
  uint8_t activesound;    /* made wandering around, with a chance of 3 in 256 */
} mobjinfo_t;
"""

FREE_SLOT = "NOTHING"
"""The type of a free thing slot, after all real ones; it has no mobjinfo."""

STATE_FIELDS = ("spawnstate", "seestate", "painstate", "meleestate", "missilestate", "deathstate", "xdeathstate")
SOUND_FIELDS = ("seesound", "attacksound", "painsound", "deathsound", "activesound")


def enum(names: Iterable[str], count: str, typedef: str = "") -> str:
    body = "".join(f"  {name},\n" for name in names)
    close = f"}} {typedef};" if typedef else "};"
    return f"{'typedef enum' if typedef else 'enum'}\n{{\n{body}  {count}\n{close}\n"


def fixed(value: int) -> str:
    if value and value % FRACUNIT == 0:
        return f"{value // FRACUNIT}*FRACUNIT"
    return str(value)


def flags(value: MF) -> str:
    names = [f"MF_{flag.name}" for flag in MF if flag in value]
    return "|".join(names) if names else "0"


def mobjinfo_entry(mobj: MobjInfo) -> str:
    fields = [
        (fixed(mobj.speed), "speed"),
        (fixed(mobj.radius), "radius"),
        (fixed(mobj.height), "height"),
        (flags(mobj.flags), "flags"),
        (str(mobj.doomednum), "doomednum"),
        (str(mobj.spawnhealth), "spawnhealth"),
        (str(mobj.reactiontime), "reactiontime"),
        (str(mobj.mass), "mass"),
        (str(mobj.damage), "damage"),
        *((f"S_{getattr(mobj, name)}", name) for name in STATE_FIELDS),
        (str(mobj.painchance), "painchance"),
        *((f"sfx_{getattr(mobj, name)}", name) for name in SOUND_FIELDS),
    ]
    lines = "".join(f"    {value},  // {name}\n" for value, name in fields)
    return f"  {{  // MT_{mobj.name}\n{lines}  }},\n"


def info_h() -> str:
    flag_lines = []
    for flag in MF:
        comment = MF_COMMENTS.get(flag)
        if comment:
            flag_lines.append(f"// {comment}")
        name = f"MF_{flag.name}"
        flag_lines.append(f"#define {name:<15} 0x{flag.value:08X}U")
    parts = [
        LICENCE,
        "#ifndef __INFO__\n#define __INFO__\n",
        '#include "d_think.h"\n#include "sounds.h"\n',
        enum((f"SPR_{s}" for s in SPRITES), "NUMSPRITES"),
        "typedef int8_t spritenum_t;\n",
        enum((f"S_{s.name}" for s in STATES), "NUMSTATES", "statenum_t"),
        enum(["AC_NULL", *(f"AC_{a}" for a in ACTIONS)], "NUMACTIONS"),
        "extern const actionf_t stateactions[NUMACTIONS];\n",
        STATE_STRUCT,
        "extern const state_t  states[NUMSTATES];\nextern const char* const sprnames[];\n",
        enum([*(f"MT_{m.name}" for m in MOBJS), f"MT_{FREE_SLOT}"], "NUMMOBJTYPES"),
        "typedef int8_t mobjtype_t;\n",
        MOBJINFO_STRUCT,
        "extern const mobjinfo_t mobjinfo[NUMMOBJTYPES];\n",
        "\n".join(flag_lines) + "\n",
        f"#define MAX_SPRITE_FRAMES {MAX_SPRITE_FRAMES}\n",
        f"// The format of both WADs, kept in their FEMTOWAD lumps.\n#define WAD_VERSION {WAD_VERSION}\n",
        "#endif\n",
    ]
    return "\n".join(parts)


def info_c() -> str:
    names = [f'"{s}",' for s in SPRITES]
    sprnames = "".join("  " + " ".join(names[i : i + 12]) + "\n" for i in range(0, len(names), 12))
    actions = "".join(f"  A_{a},\n" for a in ACTIONS)
    states = []
    for state in STATES:
        frame = f"FF_FULLBRIGHT|{state.frame}" if state.bright else str(state.frame)
        action = f"AC_{state.action}" if state.action else "AC_NULL"
        states.append(
            f"  {{SPR_{state.sprite}, {frame}, {state.tics}, {action}, S_{state.next}}},  // S_{state.name}\n"
        )
    parts = [
        LICENCE,
        "".join(
            f'#include "{h}"\n'
            for h in ("doomdef.h", "sounds.h", "m_fixed.h", "p_mobj.h", "p_enemy.h", "p_pspr.h", "info.h")
        ),
        f"const char * const sprnames[NUMSPRITES] = {{\n{sprnames}}};\n",
        f"const actionf_t stateactions[NUMACTIONS] = {{\n  NULL,\n{actions}}};\n",
        f"const state_t states[NUMSTATES] = {{\n{''.join(states)}}};\n",
        f"const mobjinfo_t mobjinfo[NUMMOBJTYPES] = {{\n{''.join(mobjinfo_entry(m) for m in MOBJS)}}};\n",
    ]
    return "\n".join(parts)


def check_references() -> None:
    """Every name a state or thing refers to exists."""
    states = {s.name for s in STATES}
    for state in STATES:
        if state.sprite not in SPRITES or state.next not in states or (state.action and state.action not in ACTIONS):
            raise ValueError(f"state {state.name} refers to an unknown sprite, state or action")
    for mobj in MOBJS:
        for name in STATE_FIELDS:
            if getattr(mobj, name) not in states:
                raise ValueError(f"thing {mobj.name}: {name} {getattr(mobj, name)} is no state")
