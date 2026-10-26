# Platform interface

femto-doom is the engine only. A platform builds it, hands it the two WADs, feeds it time and
input, and draws what it asks for. The whole contract is [`src/i_platform.h`](../src/i_platform.h).

## Building

Add `src/` to the include path and compile every `.c` in it, with the options in
[build.md](build.md). C11 with GNU extensions; enums must be `int`-sized (`-fno-short-enums` on
compilers that shorten them), because `boolean` is an enum and the engine casts arrays of it to
`int *`. The engine calls `memcpy`, `memset`, `memcmp`, `memchr`, `strcpy`, `strncpy`, `strlen`,
`strcasecmp` and `sprintf` from the C library.

## Running

```c
W_SetWads(graphics_wad, map_wad);   /* both mapped in memory, see wad-format.md */
D_DoomMain(argc, argv);             /* sets the game up and returns */
for (;;)
    D_DoomStep();                   /* runs the tics that are due, draws one frame */
```

Input goes in from `I_StartTic` as `event_t`s posted with `D_PostEvent`, the keys being `KEYD_*`.

With `W_FETCH` defined, the engine never dereferences WAD memory itself: every read goes through
the platform's `W_Fetch(src, dst, len)`, `src` being an address inside one of the two images. That
lets the WADs live in memory the CPU cannot address directly, such as SPI flash. Pointers the
engine hands the platform (palettes, patches, raw pictures, column sources) point into the WADs
too, and the platform reads them the same way.

## What the platform provides

| Group | Functions |
|---|---|
| system | `I_ZoneBase`, `I_InitTimer`, `I_GetTime` (tics, `TICRATE` per second), `I_InitKeyboard`, `I_StartTic`, `I_Quit`, `I_Error`, `I_Printf` (or `printf`), `W_Fetch` with `W_FETCH` |
| sound | `DMX_Init`, `DMX_Init2`, `DMX_Play`, `DMX_Shutdown` |
| frame | `I_InitGraphics`, `I_SetPalette` (256 RGB triples, after `I_InitGraphics`), `I_FinishUpdate`, `wipe_StartScreen`, `D_Wipe` |
| 3D view | `R_DrawColumnWall`, `R_DrawColumnSprite`, `R_DrawFuzzColumn`, `R_DrawColumnFlat`, `R_DrawSpan` without `FLAT_SPAN`, `V_ClearViewWindow` |
| pictures | `V_DrawPatchScaled`, `V_DrawPatchNotScaled`, `V_DrawBackground`, `V_DrawRaw`, `V_DrawRawRect` with `ST_PARTIAL_REFRESH` |
| automap | `V_InitDrawLine`, `V_DrawLine`, `V_ShutdownDrawLine` |

The screen is `SCREENWIDTH` × `SCREENHEIGHT` (240 × 160) palette indices; the 3D view is
`VIEWWINDOWWIDTH` × `VIEWWINDOWHEIGHT` of it, its horizon on row `CENTERY`. Column drawing gets a
`draw_column_vars_t` and the light levels in `fullcolormap`; pictures come in Doom's patch format,
`patch_t` with its posts.
