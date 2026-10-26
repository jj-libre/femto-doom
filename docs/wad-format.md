# WAD format

femto-doom reads two WADs in place, a graphics WAD and a map WAD, both little-endian.
`tools/femtowad` builds them from a Doom IWAD:

```bash
uv run --project tools/femtowad femtowad wads doom1.wad <out-dir> [--texshift N] [--high-detail]
```

## Directory

| Field | Size |
|---|---|
| `"IWAD"` | 4 |
| lump count | 4, the engine reads 16 bits |
| directory offset | 4 |

Then per lump: data offset (4), size (4, the engine reads 16 bits; lumps of 64 KiB or more are
only read piecewise), name (8). Lumps with the same data share it.

## Graphics WAD

| Lump | Content |
|---|---|
| `PLAYPAL`, `PLAYPAL1`–`PLAYPAL5` | 14 palettes of 256 RGB triples; the numbered ones for gamma levels 1–5 |
| `COLORMAP` | 34 light level maps of 256 bytes |
| `TEXTURE1` | count, offsets, then per texture: name (8), width, height, patch count, and per patch x, y, patch number |
| `TEXTUREP` | per texture an offset (u16), then: width mask (u16), width, height, overlapped (u8), patch count (u8), and per patch x, y, lump number, patch width |
| `TEXHEIGH` | height of every texture (i16) |
| `TEXCOLS` | version 1 (u8), `TEXSHIFT` (u8), `HIGH_DETAIL` (u8), log2 of rows per column (u8), texture count (u16), 0 (u16); per texture: offset (u32), width mask (u16), column shift (u8, 2 when four columns share one), 0 (u8); then every texture's columns, 128 >> `TEXSHIFT` bytes each |
| `SPRITES` | per sprite of `sprnames` and per frame up to `MAX_SPRITE_FRAMES`: the lump of rotations 1–8 (i16 each), a flip mask (u8), a rotate flag (u8) |
| `TITLEPIC`, `HELP2`, `WIMAP0` | 240 × 160 raw pixels |
| `STBAR` | 240 × 32 raw pixels |
| `FLOOR4_8` | 64 × 64 raw pixels, the finale background |
| `DEMO3` | the attract-mode demo |
| status bar, menu and intermission pictures, sprites, wall patches | Doom's patch format; walls keep every 4th column, sprites every 2nd, equal columns are stored once |

Other flats, music and sound effects are not in it.

## Map WAD

Per map a marker `E1Mx` and nine lumps in this order, then `E1MxTL` and `E1MxGL` of every map.
Values are i16 unless noted. A map has at most 255 sectors, 255 textures and 255 changeable sides.

| Lump | Record |
|---|---|
| `THINGS` | 10 B: x, y, type, angle / 45 (u8), options (u8); multiplayer things left out |
| `LINEDEFS` | 32 B: x1, y1, x2, y2, line number (u16), dx, dy, front side, back side (u16, 0xFFFF for none), box top, bottom, left, right, tag, flags (u8), slope type (u8), special |
| `SIDEDEFS` | 8 B: x offset, y offset (u8), upper, lower, middle texture (u8), sector (u8), number from 1 if play can change the side (u8, sides at the front of lines with a special) |
| `SEGS` | 18 B: x1, y1, x2, y2, offset, angle + 90°, side (u16), line (u16), front sector (u8), back sector (u8, 0xFF for none) |
| `SSECTORS` | as in Doom |
| `NODES` | as in Doom |
| `SECTORS` | 12 B: floor height, ceiling height, floor colour, ceiling colour (palette index; -2 sky, -3 nukage), light (u8), special (u8), tag |
| `REJECT` | as in Doom |
| `BLOCKMAP` | as in Doom, but a block's list can point into the tail of another one; the engine skips its first entry |

Sides of lines without a special that are equal are stored once.

### E1MxTL, version 3

What spawning the map's things would leave in memory. Header: version, thing count, sector
count, block count, node count, 0; then per skill group (easy, normal, hard) the number of things
that never think, then per skill group the number that do (u16 each); then nine u32 offsets of:

| Array | Content |
|---|---|
| things | 14 B: x, y, thing type (u8, 0xFF for none), angle / 45 (u8, bit 7 for ambush), slot per skill group (-1 if absent), subsector |
| sector first, block first | where each sector's, then each block's things start in the list below, and the end |
| things by sector and block | thing numbers, in each run the last loaded first |
| node first | where each thing's sectors start in the next two arrays |
| node sector, node thing | per node the sector a thing touches and that thing |
| sector node first, sector nodes | per sector its nodes, the last loaded first |

### E1MxGL, version 2

What grouping the lines would build. Header: version, sector count, subsector count, 0 (u16 each);
three u32 offsets of: where each sector's lines start (u16, count + 1), the lines of each sector
(u16), the sector of each subsector (u8, 0xFF for none).
