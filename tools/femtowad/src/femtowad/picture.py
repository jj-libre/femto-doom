# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2026 Jakob Jungreuthmayer
"""Doom pictures: column-major images made of posts, the format of patches and sprites."""

from __future__ import annotations

import struct
from dataclasses import dataclass

HEADER = struct.Struct("<hhhh")
TRANSPARENT = None
MAX_HEIGHT = 254

Pixels = list[list[int | None]]


@dataclass
class Picture:
    width: int
    height: int
    left: int
    top: int
    columns: list[bytes]
    """Each column's posts as stored, through the closing 0xFF."""

    @classmethod
    def parse(cls, data: bytes) -> Picture:
        width, height, left, top = HEADER.unpack_from(data)
        offsets = struct.unpack_from(f"<{width}i", data, HEADER.size)
        columns = []
        for start in offsets:
            end = start
            while data[end] != 0xFF:
                end += data[end + 1] + 4
            columns.append(data[start : end + 1])
        return cls(width, height, left, top, columns)

    @classmethod
    def from_pixels(cls, pixels: Pixels, left: int = 0, top: int = 0) -> Picture:
        height, width = len(pixels), len(pixels[0])
        if height > MAX_HEIGHT:
            raise ValueError(f"pictures are at most {MAX_HEIGHT} rows, posts start at a byte")
        columns = []
        for x in range(width):
            column = bytearray()
            y = 0
            while y < height:
                if pixels[y][x] is TRANSPARENT:
                    y += 1
                    continue
                run = y
                while run < height and pixels[run][x] is not TRANSPARENT:
                    run += 1
                values = bytes(p for k in range(y, run) if (p := pixels[k][x]) is not None)
                column += bytes((y, run - y, values[0])) + values + values[-1:]
                y = run
            columns.append(bytes(column + b"\xff"))
        return cls(width, height, left, top, columns)

    def pixels(self) -> Pixels:
        image: Pixels = [[TRANSPARENT] * self.width for _ in range(self.height)]
        for x, column in enumerate(self.columns):
            at = 0
            while column[at] != 0xFF:
                top, length = column[at], column[at + 1]
                for y in range(length):
                    if top + y < self.height:
                        image[top + y][x] = column[at + 3 + y]
                at += length + 4
        return image

    def keep_every(self, step: int) -> Picture:
        """Each column replaced by the nearest kept column to its left, every step-th one."""
        columns = [self.columns[x - x % step] for x in range(self.width)]
        return Picture(self.width, self.height, self.left, self.top, columns)

    def encode(self) -> bytes:
        """The picture with each distinct column stored once."""
        header = HEADER.pack(self.width, self.height, self.left, self.top)
        body = bytearray()
        start = HEADER.size + 4 * self.width
        stored: dict[bytes, int] = {}
        offsets = []
        for column in self.columns:
            if column not in stored:
                stored[column] = start + len(body)
                body += column
            offsets.append(stored[column])
        return header + struct.pack(f"<{self.width}i", *offsets) + bytes(body)
