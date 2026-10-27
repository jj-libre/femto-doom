from __future__ import annotations

import struct
from pathlib import Path

import pytest

from femtowad.wad import ENTRY, HEADER, MAX_LUMPS, Lump, Wad, name8


def directory(data: bytes) -> list[tuple[int, int, bytes]]:
    _, count, table = HEADER.unpack_from(data)
    return [ENTRY.unpack_from(data, table + ENTRY.size * i) for i in range(count)]


def test_round_trip(tmp_path: Path) -> None:
    wad = Wad([Lump("A", b"abc"), Lump("EMPTY", b""), Lump("LONGNAME", bytes(range(256)))])
    path = tmp_path / "T.WAD"
    path.write_bytes(wad.to_bytes())
    assert Wad.read(path).lumps == wad.lumps


def test_layout() -> None:
    data = Wad([Lump("A", b"abc"), Lump("B", b"abc"), Lump("C", b""), Lump("D", b"12345")]).to_bytes()
    ident, count, table = HEADER.unpack_from(data)
    assert (ident, count, table) == (b"IWAD", 4, HEADER.size)
    (a, _, _), (b, _, _), (c, size, _), (d, _, _) = directory(data)
    assert a == b
    assert (c, size) == (0, 0)
    assert a % 4 == 0 and d % 4 == 0
    assert data[d : d + 5] == b"12345"


def test_too_many_lumps() -> None:
    with pytest.raises(ValueError):
        Wad([Lump("A", b"")] * (MAX_LUMPS + 1)).to_bytes()


def test_not_a_wad(tmp_path: Path) -> None:
    path = tmp_path / "T.WAD"
    path.write_bytes(struct.pack("<4sii", b"ZWAD", 0, 12))
    with pytest.raises(ValueError):
        Wad.read(path)


def test_names() -> None:
    wad = Wad([Lump("A", b"1"), Lump("B", b"2"), Lump("B", b"3")])
    assert wad["A"] == b"1"
    with pytest.raises(KeyError):
        wad["B"]
    with pytest.raises(KeyError):
        wad["C"]
    wad["A"] = b"4"
    wad["C"] = b"5"
    assert [(lump.name, lump.data) for lump in wad] == [("A", b"4"), ("B", b"2"), ("B", b"3"), ("C", b"5")]


def test_name8() -> None:
    assert name8(b"stbar\0\0\0") == "STBAR"
    assert name8(b" e1m1 \0\0") == "E1M1"
