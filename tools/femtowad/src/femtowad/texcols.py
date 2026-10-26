"""TEXCOLS: every wall texture as columns at a fixed stride, the bytes R_DrawColumnWall samples,
so drawing a wall column needs no patch, directory or column offset lookup.

A column is read from the written graphics WAD, because a short post is sampled past its end.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass

from .wad import ENTRY, HEADER

VIEW_ROWS = 128
QUANTIZE = 0xFFFC


@dataclass(frozen=True)
class TexturePatch:
    originx: int
    originy: int
    lump: int
    width: int


@dataclass(frozen=True)
class WallTexture:
    widthmask: int
    width: int
    height: int
    overlapped: bool
    patches: tuple[TexturePatch, ...]


def read_texturep(data: bytes, count: int) -> list[WallTexture]:
    textures = []
    for offset in struct.unpack_from(f"<{count}H", data):
        widthmask, width, height, overlapped, patchcount = struct.unpack_from("<HhhBB", data, offset)
        patches = tuple(TexturePatch(*struct.unpack_from("<hhhh", data, offset + 8 + 8 * k)) for k in range(patchcount))
        textures.append(WallTexture(widthmask, width, height, bool(overlapped), patches))
    return textures


class ColumnReader:
    def __init__(self, image: bytes, texshift: int, high_detail: bool) -> None:
        self.image = image
        _, count, table = HEADER.unpack_from(image)
        self.positions: list[int] = [ENTRY.unpack_from(image, table + ENTRY.size * i)[0] for i in range(count)]
        self.rows = VIEW_ROWS >> texshift
        self.high_detail = high_detail

    def post_start(self, lump: int, x: int) -> int:
        start = self.positions[lump]
        offset: int = struct.unpack_from("<i", self.image, start + 8 + 4 * x)[0]
        return start + offset

    def column(self, texture: WallTexture, xc: int) -> bytes:
        if texture.overlapped:
            return self.composed(texture, xc)
        xc &= texture.widthmask
        if len(texture.patches) == 1:
            lump, x = texture.patches[0].lump, xc
        else:
            hit = next(
                ((p.lump, xc - p.originx) for p in texture.patches if p.originx <= xc < p.originx + p.width), None
            )
            if hit is None:
                return bytes(self.rows)
            lump, x = hit
        start = self.post_start(lump, x) + 3
        return self.image[start : start + self.rows].ljust(self.rows, b"\0")

    def composed(self, texture: WallTexture, xc: int) -> bytes:
        if not self.high_detail:
            xc &= QUANTIZE
        xc &= texture.widthmask
        height = texture.height
        cache = bytearray(VIEW_ROWS)
        for patch in texture.patches:
            if xc < patch.originx:
                continue
            data = self.image
            if xc >= patch.originx + struct.unpack_from("<h", data, self.positions[patch.lump])[0]:
                continue
            at = self.post_start(patch.lump, xc - patch.originx)
            while data[at] != 0xFF:
                topdelta, length = data[at], data[at + 1]
                count, position = length, patch.originy + topdelta
                if position < 0:
                    count += position
                    position = 0
                count = min(count, height - position)
                if count > 0:
                    cache[position : position + count] = data[at + 3 : at + 3 + count]
                at += length + 4
        for y in range(height, VIEW_ROWS):
            cache[y] = cache[y % height]
        return bytes(cache[: self.rows])


def texture_columns(image: bytes, texture1: bytes, texturep: bytes, texshift: int, high_detail: bool) -> bytes:
    count = struct.unpack_from("<i", texture1)[0]
    reader = ColumnReader(image, texshift, high_detail)
    entries = bytearray()
    columns = bytearray()
    shared = 0
    table = 8 + 8 * count
    for texture in read_texturep(texturep, count):
        cols = [reader.column(texture, xc) for xc in range(texture.widthmask + 1)]
        cshift = 2 if all(cols[x] == cols[x & ~3] for x in range(len(cols))) else 0
        shared += cshift == 2
        entries += struct.pack("<IHBx", table + len(columns), texture.widthmask, cshift)
        columns += b"".join(cols[:: 1 << cshift])
    header = struct.pack("<BBBxHxx", texshift, high_detail, reader.rows.bit_length() - 1, count)
    print(f"TEXCOLS: {count} textures, {shared} with columns shared by 4, {len(columns)} B of columns")
    return header + entries + columns
