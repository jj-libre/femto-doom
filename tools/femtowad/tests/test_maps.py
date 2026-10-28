# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2026 Jakob Jungreuthmayer
from __future__ import annotations

import struct

import pytest

from femtowad.lists import csr, pack_arrays
from femtowad.maps import byte, pack_blockmap, short


def block_lines(words: tuple[int, ...], block: int) -> list[int]:
    at = words[4 + block] + 1
    lines = []
    while words[at] != -1:
        lines.append(words[at])
        at += 1
    return lines


def test_blockmap() -> None:
    blocks = [[2, 3], [], [1, 2, 3], [4], [2, 3]]
    words = pack_blockmap(-64, 128, 5, 1, blocks)
    assert words[:4] == (-64, 128, 5, 1)
    assert [block_lines(words, b) for b in range(len(blocks))] == blocks
    assert words.count(-1) == 2


def test_csr() -> None:
    assert csr([2, 0, None, 2, 2], 4) == [0, 1, 1, 4, 4]


def test_pack_arrays() -> None:
    data = pack_arrays(b"HD", [b"abc", b"", b"de"])
    assert struct.unpack_from("<3I", data, 2) == (14, 17, 17)
    assert data[14:] == b"abcde"


def test_narrowing() -> None:
    assert [byte(-1), byte(255)] == [255, 255]
    assert [short(-1), short(0xFFFF), short(0x7FFF)] == [-1, -1, 0x7FFF]
    with pytest.raises(ValueError):
        byte(256)
    with pytest.raises(ValueError):
        short(-0x8001)
