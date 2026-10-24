"""Adds femto-doom's lumps to a little-endian WAD from jWadUtil.

In the graphics WAD, TEXCOLS holds every wall texture as columns at a fixed stride, the bytes
R_DrawColumnWall samples, so a wall column costs no patch, directory or column offset lookup.

In the map WAD, E1MxTL holds per map what spawning the map's things leaves behind, so it
needs no memory: each thing's spawn data and slot per skill, and, in reverse load order, the
things in each sector and block and the sectors each thing touches. E1MxGL holds what
P_GroupLines would build: the lines of each sector and the sector of each subsector.

usage: python -I tools/femtowad.py <in.wad> <out.wad> [--texshift=N] [--high-detail]
"""
import struct
import sys
from pathlib import Path

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


def i32(v):
    return (v + 0x80000000) % 0x100000000 - 0x80000000


def i16(v):
    return (v + 0x8000) % 0x10000 - 0x8000


def engine_info(root):
    """mobjinfo of the femto-doom checkout at `root`: per type, doomednum, radius and flags."""
    import re
    flags = {m[1]: int(m[2], 16) for m in re.finditer(r"#define\s+(MF_\w+)\s+0x([0-9A-Fa-f]+)", (root / "p_mobj.h").read_text("latin-1"))}
    header = (root / "info.h").read_text("latin-1")
    types = re.findall(r"^\s*(MT_\w+)", header[header.index("MT_PLAYER"):], re.M)
    source = (root / "info.c").read_text("latin-1")
    info = {}
    for block in re.finditer(r"\{\s*//\s*(MT_\w+)\s*\n(.*?)\n\s*\}", source, re.S):
        fields = [line.split("//")[0].strip().rstrip(",") for line in block[2].splitlines() if "//" in line]
        doomednum = int(fields[0])
        radius = eval(fields[16].replace("FRACUNIT", "65536"))
        flag = 0
        for name in fields[21].split("|"):
            name = name.strip()
            flag |= flags[name] if name in flags else int(name or 0)
        info[types.index(block[1])] = (doomednum, radius, flag)
    return info, types.index("MT_MISC0"), flags


def map_lumps(directory, mapname):
    names = [name for name, _, _ in directory]
    first = names.index(mapname)
    out = {}
    for name, pos, size in directory[first + 1:first + 12]:
        if name in out or (name.startswith(b"E") and name[2:3] == b"M"):
            break
        out[name] = (pos, size)
    return out


class Map:
    def __init__(self, wad, directory, mapname):
        import struct
        self.wad = wad
        self.lumps = map_lumps(directory, mapname)

        def records(name, fmt):
            pos, size = self.lumps[name]
            n = struct.calcsize("<" + fmt)
            return [struct.unpack_from("<" + fmt, wad, pos + k) for k in range(0, size - n + 1, n)]

        self.things = records(b"THINGS", "hhhbb")
        self.lines = records(b"LINEDEFS", "hhhhHhhHHhhhhhBbh")
        self.sides = records(b"SIDEDEFS", "hBbbbBb")
        self.segs = records(b"SEGS", "hhhhhhHHBB")
        self.ssectors = records(b"SSECTORS", "hh")
        self.nodes = records(b"NODES", "hhhh8h2H")
        pos, size = self.lumps[b"BLOCKMAP"]
        self.blockmap = struct.unpack_from(f"<{size // 2}h", wad, pos)
        self.orgx, self.orgy, self.bwidth, self.bheight = self.blockmap[:4]
        self.numsectors = self.lumps[b"SECTORS"][1] // 12
        self.subsector_sector = []
        for numsegs, firstseg in self.ssectors:
            sector = None
            for seg in self.segs[firstseg:firstseg + numsegs]:
                if seg[6] != 0xFFFF:
                    sector = self.sides[seg[6]][5]
                    break
            self.subsector_sector.append(sector)

    def point_on_side(self, x, y, node):
        nx, ny, ndx, ndy = node[:4]
        ix = i16(x >> 16)
        if not ndx:
            return int(ndy > 0) if ix <= nx else int(ndy < 0)
        iy = i16(y >> 16)
        if not ndy:
            return int(ndx < 0) if iy <= ny else int(ndx > 0)
        x = i32(x - (nx << 16))
        y = i32(y - (ny << 16))
        ix, iy = i16(x >> 16), i16(y >> 16)
        if (ndy ^ ndx ^ ix ^ iy) < 0:
            return int((ndy ^ ix) < 0)
        return int(i32((y >> 8) * ndx) >= i32((x >> 8) * ndy))

    def point_sector(self, x, y):
        if not self.nodes:
            return self.subsector_sector[0]
        n = len(self.nodes) - 1
        while not n & 0x8000:
            node = self.nodes[n]
            n = node[12 + self.point_on_side(x, y, node)]
        return self.subsector_sector[n & 0x7FFF]

    def block(self, x, y):
        bx = i16(i32(x - (self.orgx << 16)) >> 23)
        by = i16(i32(y - (self.orgy << 16)) >> 23)
        if 0 <= bx < self.bwidth and 0 <= by < self.bheight:
            return by * self.bwidth + bx
        return None

    def block_lines(self, bx, by):
        if not (0 <= bx < self.bwidth and 0 <= by < self.bheight):
            return []
        at = self.blockmap[4 + by * self.bwidth + bx] + 1
        out = []
        while self.blockmap[at] != -1:
            out.append(self.blockmap[at])
            at += 1
        return out

    @staticmethod
    def line_side(x, y, line):
        v1x, v1y, dx, dy = line[0], line[1], line[5], line[6]
        if not dx:
            return int(dy > 0) if x <= (v1x << 16) else int(dy < 0)
        if not dy:
            return int(dx < 0) if y <= (v1y << 16) else int(dx > 0)
        return int(i32((i32(y - (v1y << 16)) >> 8) * dx) >= i32(dy * (i32(x - (v1x << 16)) >> 8)))

    def box_on_line_side(self, box, line):
        top, bottom, left, right = box
        v1x, v1y, dx, dy, slope = line[0], line[1], line[5], line[6], line[15]
        if slope == 1:
            p = right < (v1x << 16)
            return int(p) ^ int(dy < 0) if (left < (v1x << 16)) == p else -1
        if slope == 2:
            p = self.line_side(left, top, line)
            return p if self.line_side(right, bottom, line) == p else -1
        if slope == 3:
            p = self.line_side(right, top, line)
            return p if self.line_side(left, bottom, line) == p else -1
        p = top > (v1y << 16)
        return int(p) ^ int(dx < 0) if (bottom > (v1y << 16)) == p else -1

    def touched_sectors(self, x, y, radius):
        box = (i32(y + radius), i32(y - radius), i32(x - radius), i32(x + radius))
        xl = i16(i32(box[2] - (self.orgx << 16)) >> 23)
        xh = i16(i32(box[3] - (self.orgx << 16)) >> 23)
        yl = i16(i32(box[1] - (self.orgy << 16)) >> 23)
        yh = i16(i32(box[0] - (self.orgy << 16)) >> 23)
        seen, sectors = set(), set()
        for bx in range(xl, xh + 1):
            for by in range(yl, yh + 1):
                for ln in self.block_lines(bx, by):
                    if ln in seen:
                        continue
                    seen.add(ln)
                    line = self.lines[ln]
                    btop, bbottom, bleft, bright = line[9:13]
                    if box[3] <= bleft << 16 or box[2] >= bright << 16 or box[0] <= bbottom << 16 or box[1] >= btop << 16:
                        continue
                    if self.box_on_line_side(box, line) != -1:
                        continue
                    front = self.sides[line[7]][5]
                    sectors.add(front)
                    if line[8] != 0xFFFF and self.sides[line[8]][5] != front:
                        sectors.add(self.sides[line[8]][5])
        sectors.add(self.point_sector(x, y))
        return sectors


def thing_lists(mp, info, misc0, flags):
    by_doomednum = {}
    for t, (doomednum, _, _) in sorted(info.items()):
        by_doomednum.setdefault(doomednum, t)
    groups = (1, 2, 4)
    counts = [[0, 0] for _ in groups]
    entries, sectors, blocks, nodes = [], [], [], []
    for x, y, doomednum, angle, options in mp.things:
        mt = 0 if doomednum == 1 else by_doomednum.get(doomednum)
        slots = [-1, -1, -1]
        sector = block = None
        touched = []
        if mt is not None:
            static = mt >= misc0
            for g, bit in enumerate(groups):
                if doomednum == 1 or options & bit:
                    slots[g] = counts[g][static]
                    counts[g][static] += 1
            _, radius, f = info[mt]
            if not f & flags["MF_NOSECTOR"]:
                sector = mp.point_sector(x << 16, y << 16)
                touched = sorted(mp.touched_sectors(x << 16, y << 16, radius))
            if not f & flags["MF_NOBLOCKMAP"]:
                block = mp.block(x << 16, y << 16)
        entries.append(struct.pack("<hhBB3h", x, y, 0xFF if mt is None else mt, (angle & 0x7F) | (0x80 if options & 8 else 0), *slots))
        sectors.append(sector)
        blocks.append(block)
        nodes.append(touched)

    def csr(keys, size):
        first = [0] * (size + 1)
        for k in keys:
            if k is not None:
                first[k + 1] += 1
        for k in range(size):
            first[k + 1] += first[k]
        return first

    numthings, numblocks = len(entries), mp.bwidth * mp.bheight
    sectorfirst = csr(sectors, mp.numsectors)
    blockfirst = [v + sectorfirst[-1] for v in csr(blocks, numblocks)]
    ordered = [None] * blockfirst[-1]
    fill = sectorfirst[:-1] + blockfirst[:-1]
    for t in range(numthings - 1, -1, -1):
        if sectors[t] is not None:
            ordered[fill[sectors[t]]] = t
            fill[sectors[t]] += 1
        if blocks[t] is not None:
            ordered[fill[mp.numsectors + blocks[t]]] = t
            fill[mp.numsectors + blocks[t]] += 1
    nodefirst = [0]
    nodesector, nodething = [], []
    for t, touched in enumerate(nodes):
        nodesector += touched
        nodething += [t] * len(touched)
        nodefirst.append(len(nodesector))
    numnodes = len(nodesector)
    secnodefirst = csr(nodesector, mp.numsectors)
    secnodes = [0] * numnodes
    fill = secnodefirst[:-1]
    for k in range(numnodes - 1, -1, -1):
        secnodes[fill[nodesector[k]]] = k
        fill[nodesector[k]] += 1

    arrays = [b"".join(entries)] + [struct.pack(f"<{len(a)}h", *a) for a in
              (sectorfirst, blockfirst, ordered, nodefirst, nodesector, nodething, secnodefirst, secnodes)]
    header_size = 12 + 12 + 4 * len(arrays)
    offsets, at = [], header_size
    for a in arrays:
        offsets.append(at)
        at += len(a)
    header = struct.pack("<6H", VERSION, numthings, mp.numsectors, numblocks, numnodes, 0)
    header += struct.pack("<6H", *(c[1] for c in counts), *(c[0] for c in counts))
    header += struct.pack(f"<{len(offsets)}I", *offsets)
    return header + b"".join(arrays)



def geometry_lists(mp):
    lists = [[] for _ in range(mp.numsectors)]
    for i, line in enumerate(mp.lines):
        front = mp.sides[line[7]][5]
        lists[front].append(i)
        if line[8] != 0xFFFF and mp.sides[line[8]][5] != front:
            lists[mp.sides[line[8]][5]].append(i)
    first = [0]
    for l in lists:
        first.append(first[-1] + len(l))
    lines = [i for l in lists for i in l]
    sectors = [0xFF if s is None else s for s in mp.subsector_sector]
    arrays = [struct.pack(f"<{len(first)}H", *first), struct.pack(f"<{len(lines)}H", *lines), bytes(sectors)]
    offsets, at = [], 8 + 4 * len(arrays)
    for a in arrays:
        offsets.append(at)
        at += len(a)
    header = struct.pack("<4H", 2, mp.numsectors, len(sectors), 0) + struct.pack(f"<{len(offsets)}I", *offsets)
    return header + b"".join(arrays)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = dict((a[2:].split("=") + ["1"])[:2] for a in sys.argv[1:] if a.startswith("--"))
    if len(args) != 2:
        sys.exit(__doc__)
    wad = open(args[0], "rb").read()
    directory = lumps(wad)
    names = [name for name, _, _ in directory]
    if b"TEXCOLS" in names or any(len(name) == 6 and name[2:3] == b"M" and name[4:] in (b"TL", b"GL") for name in names):
        sys.exit("already converted")
    if b"TEXTURE1" in names:
        texcols = texture_columns(wad, directory, int(flags.get("texshift", 0)), "high-detail" in flags)
        write(args[1], wad, [(b"TEXCOLS", texcols)])
        return
    info, misc0, mflags = engine_info(Path(__file__).resolve().parents[1])
    maps = [name for name in names if len(name) == 4 and name[0:1] == b"E" and name[2:3] == b"M"]
    extra = []
    for name in maps:
        mp = Map(wad, directory, name)
        extra += [(name + b"TL", thing_lists(mp, info, misc0, mflags)), (name + b"GL", geometry_lists(mp))]
    print(f"thing and geometry lists: {len(maps)} maps, {sum(len(d) for _, d in extra)} B")
    write(args[1], wad, extra)


if __name__ == "__main__":
    main()
