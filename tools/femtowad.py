"""Adds femto-doom's lumps to a little-endian graphics WAD from jWadUtil.

TEXCOLS holds every wall texture as columns at a fixed stride, the bytes R_DrawColumnWall
samples, so a wall column costs no patch, directory or column offset lookup.

usage: python -I tools/femtowad.py <in.wad> <out.wad> [--texshift=N] [--high-detail]
"""
import struct
import sys

VERSION = 1


def lumps(wad):
    ident, count, table = struct.unpack_from("<4sii", wad)
    if ident not in (b"IWAD", b"PWAD"):
        sys.exit("not a little-endian WAD")
    out = []
    for i in range(count):
        pos, size = struct.unpack_from("<ii", wad, table + 16 * i)
        out.append((wad[table + 16 * i + 8:table + 16 * i + 16].rstrip(b"\0"), pos, size))
    return out


def write(path, wad, extra):
    old = lumps(wad)
    body = bytearray(wad)
    entries = [(pos, size, name) for name, pos, size in old]
    for name, data in extra:
        body += b"\0" * (-len(body) % 4)
        entries.append((len(body), len(data), name))
        body += data
    body += b"\0" * (-len(body) % 4)
    table = len(body)
    for pos, size, name in entries:
        body += struct.pack("<ii8s", pos, size, name)
    struct.pack_into("<4sii", body, 0, wad[:4], len(entries), table)
    open(path, "wb").write(body)


def texture_columns(wad, directory, texshift, high_detail):
    by_name = {name: (pos, size) for name, pos, size in directory}
    texturep = by_name[b"TEXTUREP"][0]
    count = struct.unpack_from("<i", wad, by_name[b"TEXTURE1"][0])[0]
    n = 128 >> texshift

    def patch(num):
        return directory[num][1]

    def raw(at):
        return bytes(wad[at:at + n]).ljust(n, b"\0")

    def column(tex, xc):
        widthmask, width, height, overlapped, patchcount, patches = tex
        if overlapped:
            if not high_detail:
                xc &= 0xFFFC
            xc &= widthmask
            cache = bytearray(128)
            for originx, originy, num, _ in patches:
                if xc < originx:
                    continue
                p = patch(num)
                if xc >= originx + struct.unpack_from("<h", wad, p)[0]:
                    continue
                col = p + struct.unpack_from("<i", wad, p + 8 + 4 * (xc - originx))[0]
                while wad[col] != 0xFF:
                    topdelta, length = wad[col], wad[col + 1]
                    count_, position = length, originy + topdelta
                    if position < 0:
                        count_ += position
                        position = 0
                    if position + count_ > height:
                        count_ = height - position
                    if count_ > 0:
                        cache[position:position + count_] = wad[col + 3:col + 3 + count_]
                    col += length + 4
            for y in range(height, 128):
                cache[y] = cache[y % height]
            return bytes(cache[:n])
        xc &= widthmask
        if patchcount == 1:
            num, x = patches[0][2], xc
        else:
            hit = next(((p[2], xc - p[0]) for p in patches if p[0] <= xc < p[0] + p[3]), None)
            if hit is None:
                return b"\0" * n
            num, x = hit
        p = patch(num)
        return raw(p + struct.unpack_from("<i", wad, p + 8 + 4 * x)[0] + 3)

    entries, data, shared = [], bytearray(), 0
    for t in range(count):
        at = texturep + struct.unpack_from("<H", wad, texturep + 2 * t)[0]
        widthmask, width, height, overlapped, patchcount = struct.unpack_from("<HhhBB", wad, at)
        patches = [struct.unpack_from("<hhhh", wad, at + 8 + 8 * k) for k in range(patchcount)]
        tex = (widthmask, width, height, overlapped, patchcount, patches)
        cols = [column(tex, xc) for xc in range(widthmask + 1)]
        cshift = 2 if all(cols[x] == cols[x & ~3] for x in range(len(cols))) else 0
        shared += cshift == 2
        entries.append(struct.pack("<IHBB", len(data), widthmask, cshift, 0))
        for c in cols[::1 << cshift]:
            data += c
    header = struct.pack("<BBBBHH", VERSION, texshift, int(high_detail), n.bit_length() - 1, count, 0)
    table = len(header) + 8 * count
    entries = [struct.pack("<I", struct.unpack_from("<I", e)[0] + table) + e[4:] for e in entries]
    print(f"TEXCOLS: {count} textures, {shared} with columns shared by 4, {len(data)} B of columns")
    return header + b"".join(entries) + data


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = dict((a[2:].split("=") + ["1"])[:2] for a in sys.argv[1:] if a.startswith("--"))
    if len(args) != 2:
        sys.exit(__doc__)
    wad = open(args[0], "rb").read()
    directory = lumps(wad)
    if any(name == b"TEXCOLS" for name, _, _ in directory):
        sys.exit("already converted")
    texcols = texture_columns(wad, directory, int(flags.get("texshift", 0)), "high-detail" in flags)
    write(args[1], wad, [(b"TEXCOLS", texcols)])


if __name__ == "__main__":
    main()
