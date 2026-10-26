# Platform interface

femto-doom is the engine only. A platform builds it, hands it the two WADs, feeds it time and
input, and draws what it asks for.

## Building

Add `src/` to the include path and compile every `.c` in it, with the options in
[build.md](build.md). C11 with GNU extensions; enums must be `int`-sized (`-fno-short-enums` on
compilers that shorten them), because `boolean` is an enum and the engine casts arrays of it to
`int *`.

## Running

```c
W_SetWads(graphics_wad, map_wad);   /* both mapped in memory, see wad-format.md */
D_DoomMain(argc, argv);             /* sets the game up and returns */
for (;;)
    D_DoomStep();                   /* runs the tics that are due, draws one frame */
```

With `W_FETCH` defined, the engine never dereferences WAD memory itself: every read goes through
`W_Fetch(src, dst, len)`, `src` being an address inside one of the two images. That lets the WADs
live in memory the CPU cannot address directly, such as SPI flash.

## What the platform provides

System and sound, declared in `i_system.h`:

| Function | Does |
|---|---|
| `I_ZoneBase(&size)` | memory for the zone allocator and its size |
| `I_InitTimer`, `I_GetTime` | time in tics, 35 per second |
| `I_InitKeyboard`, `I_StartTic` | input: `I_StartTic` posts `event_t`s with `D_PostEvent`, keys are `KEYD_*` from `doomdef.h` |
| `I_Quit`, `I_Error(fmt, ...)` | leave the game; neither returns |
| `I_Printf(fmt, ...)` | log output; without `-DI_Printf=name` it is `printf` |
| `DMX_Init`, `DMX_Init2`, `DMX_Play(sfx)`, `DMX_Shutdown` | sound effects by `sfxenum_t` |

Video, declared in `i_video.h`. The screen is `SCREENWIDTH` × `SCREENHEIGHT` (240 × 160) indexed
pixels; the 3D view is `VIEWWINDOWWIDTH` × `VIEWWINDOWHEIGHT` of it.

| Function | Does |
|---|---|
| `I_InitGraphics`, `I_FinishUpdate` | set up; end of a frame |
| `I_ReloadPalette`, `I_SetPalette(n)` | palette `n` of `PLAYPAL`, or of `PLAYPAL1`–`PLAYPAL5` for gamma level `_g_gamma` |
| `R_DrawColumnWall`, `R_DrawColumnSprite`, `R_DrawFuzzColumn` | one textured column of the view, see `draw_column_vars_t` in `r_main.h` |
| `R_DrawColumnFlat(color, ...)` | one column in a single colour: floors, ceilings |
| `V_ClearViewWindow` | clear the view |
| `V_DrawPatchScaled`, `V_DrawPatchNotScaled` | a picture in Doom's patch format at x, y |
| `V_DrawRaw(lump, offset)`, `V_DrawRawRect` | a raw picture lump at a screen offset; the rectangle form only with `ST_PARTIAL_REFRESH` |
| `V_DrawBackground(lump)` | the screen tiled with a 64 × 64 raw lump |
| `V_InitDrawLine`, `V_DrawLine`, `V_ShutdownDrawLine` | automap lines |
| `ST_Drawer` | redraw the status bar when `ST_NeedUpdate()`, by calling `ST_doRefresh()` |
| `wipe_StartScreen`, `D_Wipe` | screen wipe between game states; may just finish the frame |

Also from the platform: the C library functions the engine calls (`memcpy`, `memset`, `memcmp`,
`memchr`, `strcpy`, `strncpy`, `strlen`, `strcasecmp`, `sprintf`).
