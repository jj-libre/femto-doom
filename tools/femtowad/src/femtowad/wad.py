# SPDX-License-Identifier: GPL-2.0-or-later
"""WAD files: reading any little-endian IWAD or PWAD, writing femto-doom's layout."""

from __future__ import annotations

import struct
from collections.abc import Iterable, Iterator
from dataclasses import dataclass
from pathlib import Path

HEADER = struct.Struct("<4sii")
ENTRY = struct.Struct("<ii8s")
MAX_LUMPS = 0x7FFF


def name8(raw: bytes) -> str:
    """A name field as Doom tools compare it: trimmed and upper case."""
    return raw.decode("latin-1").strip("".join(map(chr, range(33)))).upper()


@dataclass(frozen=True)
class Lump:
    name: str
    data: bytes


class Wad:
    """An ordered list of lumps, addressed by index or by a name that must be unique."""

    def __init__(self, lumps: Iterable[Lump] = ()) -> None:
        self.lumps = list(lumps)

    @classmethod
    def read(cls, path: Path) -> Wad:
        data = path.read_bytes()
        ident, count, table = HEADER.unpack_from(data)
        if ident not in (b"IWAD", b"PWAD"):
            raise ValueError(f"{path} is not a WAD file")
        lumps = []
        for i in range(count):
            pos, size, name = ENTRY.unpack_from(data, table + ENTRY.size * i)
            lumps.append(Lump(name.rstrip(b"\0").decode("ascii"), data[pos : pos + size]))
        return cls(lumps)

    def __iter__(self) -> Iterator[Lump]:
        return iter(self.lumps)

    def __len__(self) -> int:
        return len(self.lumps)

    def __contains__(self, name: str) -> bool:
        return any(lump.name == name for lump in self.lumps)

    def __getitem__(self, name: str) -> bytes:
        return self.lumps[self.index(name)].data

    def __setitem__(self, name: str, data: bytes) -> None:
        if name in self:
            self.lumps[self.index(name)] = Lump(name, data)
        else:
            self.lumps.append(Lump(name, data))

    def index(self, name: str) -> int:
        found = [i for i, lump in enumerate(self.lumps) if lump.name == name]
        if len(found) != 1:
            raise KeyError(f"{len(found)} lumps named {name}")
        return found[0]

    def between(self, start: str, end: str) -> list[Lump]:
        return self.lumps[self.index(start) + 1 : self.index(end)]

    def with_prefix(self, prefix: str) -> list[Lump]:
        return [lump for lump in self.lumps if lump.name.startswith(prefix)]

    def replace(self, index: int, data: bytes) -> None:
        self.lumps[index] = Lump(self.lumps[index].name, data)

    def remove(self, name: str) -> None:
        del self.lumps[next(i for i, lump in enumerate(self.lumps) if lump.name == name)]

    def remove_prefix(self, prefix: str) -> None:
        self.lumps = [lump for lump in self.lumps if not lump.name.startswith(prefix)]

    def to_bytes(self) -> bytes:
        """The directory first, then each distinct lump once, 4-byte aligned.

        Lumps with the same data share it. The engine reads the lump count and sizes as 16 bits;
        it reads lumps of 64 KiB or more, such as TEXCOLS, only piecewise.
        """
        if len(self.lumps) > MAX_LUMPS:
            raise ValueError(f"{len(self.lumps)} lumps, at most {MAX_LUMPS}")
        directory = bytearray(HEADER.pack(b"IWAD", len(self.lumps), HEADER.size))
        body = bytearray()
        start = HEADER.size + ENTRY.size * len(self.lumps)
        stored: dict[bytes, int] = {}
        for lump in self.lumps:
            if not lump.data:
                pos = 0
            elif lump.data in stored:
                pos = stored[lump.data]
            else:
                pos = stored[lump.data] = start + len(body)
                body += lump.data + bytes(-len(lump.data) % 4)
            directory += ENTRY.pack(pos, len(lump.data), lump.name.encode("ascii"))
        return bytes(directory + body)
