"""The graphics WAD: the IWAD's lumps the engine reads, in the layout it reads in place.

Walls keep every 4th column and sprites every 2nd, and pictures store equal columns once.
TEXTUREP lists each texture's patches as lump numbers, SPRITES each sprite frame's lumps, and
TEXCOLS every wall texture's columns.
"""

from __future__ import annotations

import struct
from collections.abc import Mapping
from dataclasses import dataclass

from .art import GAMMA_LEVELS
from .info import MAX_SPRITE_FRAMES, SPRITES
from .picture import Picture
from .texcols import texture_columns
from .wad import Lump, Wad, name8

NAMED_LUMPS = (
    "PLAYPAL", "COLORMAP", "DEMO3", "TEXTURE1", "TITLEPIC", "HELP2", "STBAR", "WIMAP0", "FLOOR4_8",
    "STFGOD0", "STFDEAD0",
    "M_DOOM", "M_NGAME", "M_OPTION", "M_QUITG", "M_NEWG", "M_SKILL", "M_JKILL", "M_ROUGH", "M_HURT", "M_ULTRA",
    "M_NMARE", "M_OPTTTL", "M_ENDGAM", "M_MESSG", "M_MSGON", "M_MSGOFF", "M_ARUN", "M_GAMMA", "M_SKULL1", "M_SKULL2",
    "M_THERML", "M_THERMM", "M_THERMR", "M_THERMO",
    "WIURH0", "WISPLAT", "WIOSTK", "WIOSTI", "WIF", "WIMSTT", "WITIME", "WIPAR", "WIPCNT", "WICOLON", "WISUCKS",
    "WISCRT2", "WIENTER",
)  # fmt: skip
NUMCARDS = 3
PAIN_FACES = 5
STRAIGHT_FACES = 3
LEVEL_NAMES = 9
RAW_PICTURES = {"STBAR", "WIMAP0", "TITLEPIC", "HELP2"}
PICTURE_PREFIXES = ("ST", "M_", "WI")
PLAYER_SPRITES_KEPT = {"PLAYN0", "PLAYW0", "PLAYV0"}
SPRITE_FRAME = struct.Struct("<8hBB")


@dataclass(frozen=True)
class PatchRef:
    originx: int
    originy: int
    patch: int


@dataclass(frozen=True)
class Texture:
    name: bytes
    width: int
    height: int
    patches: tuple[PatchRef, ...]


def read_textures(data: bytes) -> list[Texture]:
    count = struct.unpack_from("<i", data)[0]
    textures = []
    for offset in struct.unpack_from(f"<{count}i", data, 4):
        name, _, width, height, _, patchcount = struct.unpack_from("<8sihhih", data, offset)
        patches = tuple(PatchRef(*struct.unpack_from("<hhh", data, offset + 22 + 10 * k)) for k in range(patchcount))
        textures.append(Texture(name, width, height, patches))
    return textures


def texture_names(iwad: Wad) -> list[str]:
    return [name8(texture.name) for texture in read_textures(iwad["TEXTURE1"])]


def lumps_read_by_name() -> set[str]:
    names = set(NAMED_LUMPS)
    names |= {f"PLAYPAL{level}" for level in range(1, GAMMA_LEVELS + 1)}
    names |= {f"STCFN{code:03d}" for code in range(ord("!"), ord("_") + 1)}
    names |= {f"{prefix}{digit}" for prefix in ("STGANUM", "STYSNUM", "WINUM") for digit in range(10)}
    names |= {f"STGNUM{digit}" for digit in range(2, 8)}
    names |= {f"STKEYS{card}" for card in range(NUMCARDS)}
    names |= {f"WILV0{level}" for level in range(LEVEL_NAMES)}
    for pain in range(PAIN_FACES):
        names |= {f"STFST{pain}{straight}" for straight in range(STRAIGHT_FACES)}
        names |= {f"STFTR{pain}0", f"STFTL{pain}0", f"STFOUCH{pain}", f"STFEVL{pain}", f"STFKILL{pain}"}
    return names


def build_graphics(iwad: Wad, art: Mapping[str, bytes], texshift: int, high_detail: bool) -> Wad:
    """`art` replaces lumps of the same name or is appended. Of lumps sharing a name only the last
    is kept, the one Doom's lookup finds."""
    textures = read_textures(iwad["TEXTURE1"])
    pnames = patch_names(iwad["PNAMES"])
    patches = {pnames[ref.patch] for texture in textures for ref in texture.patches}
    sprites = {lump.name for lump in iwad.between("S_START", "S_END") if lump.name[:4] in SPRITES}
    by_name = lumps_read_by_name()

    kept = by_name | patches | sprites | {"PNAMES"}
    last = {lump.name: i for i, lump in enumerate(iwad)}
    wad = Wad(lump for i, lump in enumerate(iwad) if lump.name in kept and last[lump.name] == i)
    for name, data in art.items():
        wad[name] = data
    missing = sorted(name for name in by_name if name not in wad)
    if missing:
        raise ValueError(f"the IWAD lacks lumps the engine reads: {' '.join(missing)}")

    use_zombieman_for_player(wad)
    shrink_pictures(wad, sprites, patches)
    wad["TEXTURE1"] = pack_textures(textures)
    wad["TEXHEIGH"] = struct.pack(f"<{len(textures)}h", *(t.height for t in textures))
    wad.lumps[wad.index("PNAMES")] = Lump("TEXTUREP", texture_patches(wad, textures, pnames))
    wad["SPRITES"] = sprite_frames(wad, sprites)
    wad["TEXCOLS"] = texture_columns(wad.to_bytes(), wad["TEXTURE1"], wad["TEXTUREP"], texshift, high_detail)
    return wad


def patch_names(pnames: bytes) -> list[str]:
    count = struct.unpack_from("<i", pnames)[0]
    return [name8(pnames[4 + 8 * i : 12 + 8 * i]) for i in range(count)]


def pack_textures(textures: list[Texture]) -> bytes:
    """TEXTURE1 without the fields the engine ignores."""
    body = bytearray()
    start = 4 + 4 * len(textures)
    offsets = []
    for texture in textures:
        offsets.append(start + len(body))
        body += struct.pack("<8shhh", texture.name, texture.width, texture.height, len(texture.patches))
        for patch in texture.patches:
            body += struct.pack("<hhh", patch.originx, patch.originy, patch.patch)
    return struct.pack(f"<i{len(textures)}i", len(textures), *offsets) + body


def use_zombieman_for_player(wad: Wad) -> None:
    """The player's sprites become the zombieman's, so they share storage."""
    zombieman = wad.with_prefix("POSS")
    for lump in wad.with_prefix("PLAY"):
        if lump.name.startswith("PLAYPAL") or lump.name in PLAYER_SPRITES_KEPT:
            continue
        suffix = lump.name[4:]
        wad[lump.name] = next(z.data for z in zombieman if z.name.endswith(suffix))
    wad["PLAYV0"] = wad["POSSU0"]


def shrink_pictures(wad: Wad, sprites: set[str], walls: set[str]) -> None:
    for i, lump in enumerate(wad.lumps):
        if lump.name in sprites:
            picture = Picture.parse(lump.data).keep_every(2)
        elif lump.name in walls:
            picture = Picture.parse(lump.data).keep_every(4)
        elif lump.name.startswith(PICTURE_PREFIXES) and lump.name not in RAW_PICTURES:
            picture = Picture.parse(lump.data)
        else:
            continue
        wad.replace(i, picture.encode())


def texture_patches(wad: Wad, textures: list[Texture], pnames: list[str]) -> bytes:
    """Per texture its width mask, size and patches as lump numbers with widths.

    Overlapping patches make the engine compose columns instead of reading one patch.
    """
    entries = []
    for texture in textures:
        refs = []
        for patch in texture.patches:
            number = wad.index(pnames[patch.patch])
            width = struct.unpack_from("<h", wad.lumps[number].data)[0]
            refs.append((patch.originx, patch.originy, number, width))
        overlapped = any(a[0] + a[3] > b[0] and a[0] < b[0] + b[3] for i, a in enumerate(refs) for b in refs[i + 1 :])
        widthmask = (1 << max(texture.width, 1).bit_length() - 1) - 1
        entry = struct.pack("<HhhBB", widthmask, texture.width, texture.height, overlapped, len(refs))
        entries.append(entry + b"".join(struct.pack("<hhhh", *ref) for ref in refs))
    start = 2 * len(textures)
    offsets = []
    for entry in entries:
        offsets.append(start)
        start += len(entry)
    return struct.pack(f"<{len(offsets)}H", *offsets) + b"".join(entries)


def sprite_frames(wad: Wad, sprites: set[str]) -> bytes:
    """Per sprite and frame the lump of each of 8 rotations, a flip mask and a rotate flag."""
    table = bytearray(SPRITE_FRAME.size * MAX_SPRITE_FRAMES * len(SPRITES))
    for number, lump in enumerate(wad.lumps):
        if lump.name not in sprites:
            continue
        sprite = SPRITES.index(lump.name[:4])
        at = SPRITE_FRAME.size * (MAX_SPRITE_FRAMES * sprite + ord(lump.name[4]) - ord("A"))
        *rotations, flips, rotates = SPRITE_FRAME.unpack_from(table, at)
        rotation = int(lump.name[5])
        if rotation == 0:
            rotations = [number] * 8
        else:
            rotations[rotation - 1] = number
            rotates = 1
        if len(lump.name) == 8:
            mirrored = int(lump.name[7])
            rotations[mirrored - 1] = number
            flips |= 1 << mirrored - 1
        SPRITE_FRAME.pack_into(table, at, *rotations, flips, rotates)
    return bytes(table)
