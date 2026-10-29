# WAD format

femto-doom reads two WADs in place, `GRAPHICS.WAD` and `MAPS.WAD`, both little-endian.
[femtowad](../tools/femtowad) builds them from a Doom IWAD:

```bash
femtowad wads doom1.wad <out-dir> [--screen WxH] [--texture-shift N] [--high-detail]
```

## Layout

A WAD is one block of bytes in Doom's layout: a header, the lumps (named pieces of data) and a
directory, the table of every lump's name, offset and size. The platform passes both WADs to
`W_SetWads` as addresses, e.g. in flash, and the engine finds a lump by looking its name up in
the directory.

The engine reads lump counts and sizes as 16 bits, so lumps of 64 KiB or more are only read
piecewise. Lumps with the same data share it.

Both WADs hold a lump `FEMTOWAD` with the format version (u16), checked at start-up against
`WAD_VERSION` in `info.h`. In the graphics WAD the screen width and height follow (u16 each),
checked against `SCREEN_WIDTH` and `SCREEN_HEIGHT`.

## Graphics WAD

Values are i16 unless noted.

| Lump | Content |
|---|---|
| `PLAYPAL` | 14 palettes of 256 RGB triples: normal, then the damage, item pickup and radiation suit tints |
| `PLAYPAL1`–`PLAYPAL5` | `PLAYPAL` brightened for gamma correction levels 1–5 |
| `COLORMAP` | 34 maps of 256 palette indices: 32 light levels, invulnerability, black |
| `TEXTURE1` | texture count (i32), an offset (i32) per texture, then per texture: name (8 B), width, height, patch count, and per patch x, y, patch number |
| `TEXTUREP` | an offset (u16) per texture, then per texture: width mask (u16), width, height, whether patches overlap (u8), patch count (u8), and per patch x, y, lump number, patch width |
| `TEXHEIGH` | height of every texture |
| `TEXCOLS` | every wall texture's columns, ready to draw. Header: `TEXTURE_SHIFT` (u8), `HIGH_DETAIL` (u8), log2 of rows per column (u8), 0 (u8), texture count (u16), 0 (u16); per texture: offset of its columns (u32), width mask (u16), column shift (u8, 2 when four columns share one), 0 (u8); then the columns, 128 >> `TEXTURE_SHIFT` bytes each |
| `SPRITES` | per sprite, in the order of the `SPR_` numbers in `info.h`, and per frame up to `MAX_SPRITE_FRAMES`: the lump of rotations 1–8, a flip mask (u8), a rotate flag (u8) |
| `TITLEPIC`, `HELP2`, `WIMAP0` | title screen, help screen, intermission map: raw pixels, the size of the screen |
| `STBAR` | status bar background: raw pixels, the screen's width × `ST_HEIGHT`; Doom's bar with the arms panel and percent signs drawn in, scaled |
| `FLOOR4_8` | finale background: 64 × 64 raw pixels |
| `DEMO3` | the attract-mode demo |
| status bar, menu and intermission pictures, sprites, wall patches | Doom's patch format (`patch_t` in [i_platform.h](../src/i_platform.h)); the status bar's scaled as the bar is, walls keep every 4th column, sprites every 2nd, equal columns are stored once |

Other flats, music and sound effects are not in it.

## Map WAD

Per map a marker `E1Mx` and nine lumps in this order, then `E1MxTL` and `E1MxGL` of every map.
Values are i16 unless noted. A map has at most 255 sectors, 255 textures and 255 changeable sides.

| Lump | Record |
|---|---|
| `THINGS` | 10 B: x, y, type, angle / 45 (u8), flags (u8: skill levels, ambush); multiplayer things left out |
| `LINEDEFS` | 32 B: x1, y1, x2, y2, the line's own index (u16), dx, dy, front side, back side (u16, 0xFFFF for none), bounding box top, bottom, left, right, tag, flags (u8), slope type (u8), special |
| `SIDEDEFS` | 8 B: x offset, y offset (u8), upper, lower, middle texture (u8), sector (u8), a number from 1 if play can change the side, else 0 (u8) |
| `SEGS` | 18 B: x1, y1, x2, y2, offset, angle + 90°, side (u16), line (u16), front sector (u8), back sector (u8, 0xFF for none) |
| `SSECTORS` | as in Doom |
| `NODES` | as in Doom |
| `SECTORS` | 12 B: floor height, ceiling height, floor colour, ceiling colour (palette index; -2 sky, -3 nukage), light (u8), special (u8), tag |
| `REJECT` | as in Doom |
| `BLOCKMAP` | as in Doom, but a block's list can point into the tail of another one; the engine skips its first entry |

Play can change the sides at the front of lines with a special. Equal sides of other lines are
stored once.

### E1MxTL

The map's things, spawned in advance. Header: thing count, sector count, block count, link
count; then per skill group (easy, normal, hard) the number of things that never think, then per
skill group the number that do (u16 each); then nine u32 offsets of:

| Array | Content |
|---|---|
| things | 14 B: x, y, thing type (u8, 0xFF for none), angle / 45 (u8, bit 7 for ambush), per skill group the thing's slot among those that think or among those that never do (-1 if absent), subsector |
| sector first, block first | where each sector's, then each block's things start in the list below, and the end |
| things by sector and block | thing numbers, in each run the last loaded first |
| link first | where each thing's links start in the next two arrays |
| link sector, link thing | per link a sector a thing touches, and that thing |
| sector link first, sector links | per sector its links, the last loaded first |

### E1MxGL

Each sector's lines, grouped in advance. Header: sector count, subsector count (u16 each);
three u32 offsets of: where each sector's lines start (u16, count + 1), the lines of each sector
(u16), the sector of each subsector (u8, 0xFF for none).
