"""What the WADs must match in the femto-doom checkout they are built for."""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class MobjInfo:
    doomednum: int
    radius: int
    flags: int


@dataclass(frozen=True)
class Engine:
    mobjs: dict[int, MobjInfo]
    """Per thing type, from info.c's mobjinfo."""
    first_static: int
    """The first thing type that never thinks, MT_MISC0."""
    flags: dict[str, int]
    sprites: tuple[str, ...]
    """info.c's sprnames, in the order SPRITES is indexed by."""
    sprite_frames: int
    sounds: tuple[str, ...]
    """The sound effects after sfx_None, in the order the engine numbers them."""

    @classmethod
    def read(cls, root: Path) -> Engine:
        def source(name: str) -> str:
            return (root / name).read_text("latin-1")

        flags = {m[1]: int(m[2], 16) for m in re.finditer(r"#define\s+(MF_\w+)\s+0x([0-9A-Fa-f]+)", source("p_mobj.h"))}
        header = source("info.h")
        types = re.findall(r"^\s*(MT_\w+)", header[header.index("MT_PLAYER") :], re.M)
        info = source("info.c")
        mobjs = {}
        for block in re.finditer(r"\{\s*//\s*(MT_\w+)\s*\n(.*?)\n\s*\}", info, re.S):
            fields = {}
            for line in block[2].splitlines():
                if "//" in line:
                    value, comment = line.split("//", 1)
                    fields[comment.strip()] = value.strip().rstrip(",")
            radius = int(fields["radius"].removesuffix("*FRACUNIT")) << 16
            mobjs[types.index(block[1])] = MobjInfo(
                int(fields["doomednum"]), radius, parse_flags(fields["flags"], flags)
            )
        sprnames = re.search(r"sprnames\[NUMSPRITES\]\s*=\s*\{(.*?)\}", info, re.S)
        frames = re.search(r"#define\s+MAX_SPRITE_FRAMES\s+(\d+)", source("r_things.h"))
        sfx = re.search(r"\bsfx_None,(.*?)\bNUMSFX", source("sounds.h"), re.S)
        if sprnames is None or frames is None or sfx is None:
            raise ValueError(f"{root} does not look like a femto-doom checkout")
        return cls(
            mobjs,
            types.index("MT_MISC0"),
            flags,
            tuple(re.findall(r'"(\w+)"', sprnames[1])),
            int(frames[1]),
            tuple(name.upper() for name in re.findall(r"\bsfx_(\w+)", sfx[1])),
        )

    def mobj_by_doomednum(self) -> dict[int, int]:
        out: dict[int, int] = {}
        for mobjtype, info in sorted(self.mobjs.items()):
            out.setdefault(info.doomednum, mobjtype)
        return out


def parse_flags(expression: str, flags: dict[str, int]) -> int:
    value = 0
    for name in (part.strip() for part in expression.split("|")):
        value |= flags[name] if name in flags else int(name or 0)
    return value
