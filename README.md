# femto-doom

Doom was released by id Software in 1993 for DOS PCs, requiring at least 4 MB of RAM. Its source
code was published in 1997 and put under the GPL in 1999. Since then Doom has been ported to
almost anything with a screen, most ports loading the game data into RAM and drawing into a frame
buffer.

femto-doom is based on [Doom64KB](https://github.com/FrenkelS/Doom64KB) by Frenkel Smeijers, which
runs Doom in 64 KB of RAM with the game data in ROM. femto-doom goes further: E1M1 runs in about
16 KB of RAM, with all game data read in place from read-only memory.

femto-doom is a pure Doom engine, without any platform code. The platform hands it the game data,
time and input, and draws what it outputs; [i_platform.h](src/i_platform.h) is the whole interface.

<p>
<img src="docs/img/fireball.png" width="49%" alt="Imps and a fireball, 240 x 160">
<img src="docs/img/sergeant.png" width="49%" alt="A sergeant, 240 x 160">
</p>

## Features

- Shareware episode: all nine maps, all monsters
- Little RAM: packed map data, compact things, sleeping monsters take almost none, no recursion
- Game data read in place, never loaded; optionally through the platform, e.g. from external flash
- No frame buffer: the 3D view column by column and pictures go straight to the platform
- Any screen size, 3D view up to 320 pixels wide
- Build options trade detail and speed for memory: wall and sprite resolution, lookup tables, per-frame limits
- Plain C11, no operating system needed
- Automap over the 3D view, rotating with the player
- Floors and ceilings in one colour, light per sector without darkening with distance
- Sound effects as events, samples from the platform
- No music, saving or multiplayer

## Getting started

femto-doom ships no game data. [femtowad](tools/femtowad) builds its two WADs from Doom's
shareware `doom1.wad`:

```bash
uv tool install ./tools/femtowad
femtowad wads doom1.wad wads/
```

A platform compiles [src](src) together with its own code; see [platform.md](docs/platform.md).

- [platform.md](docs/platform.md): building the engine and the platform interface
- [build.md](docs/build.md): build options
- [wad-format.md](docs/wad-format.md): the WADs and the tool that builds them

## Example

[host](examples/host) is a platform without a screen: it draws into memory, prints frame hashes
and saves screenshots.

```bash
make -C examples/host
examples/host/host wads/GRAPHICS.WAD wads/MAPS.WAD -shot 500 shot.ppm
make -C examples/host check WADS=$PWD/wads
```

## Licence

GPL-2.0, see [LICENSE](LICENSE).
