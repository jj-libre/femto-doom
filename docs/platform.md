# Platform interface

femto-doom is the engine only. A platform builds it, hands it the two WADs, feeds it time and
input, and draws the engine's output. The whole contract is [`src/i_platform.h`](../src/i_platform.h);
[`examples/host`](../examples/host/host.c) implements all of it without a screen.

## Building

Add `src/` to the include path and compile every `.c` in it, with the options in
[build.md](build.md). C11, little-endian; `int` must be at least 32 bits and enums `int`-sized
(`-fno-short-enums` on compilers that shorten them). Of the C library only `memcpy`, `memset` and
`memcmp` are used.

## Running

```c
W_SetWads(graphics_wad, map_wad);   /* both mapped in memory, see wad-format.md */
D_DoomInit(argc, argv);             /* sets the game up and returns */
for (;;)
    D_DoomStep();                   /* runs the tics that are due, draws one frame */
```

`D_DoomMain(argc, argv)` does the same and never returns, for a platform with nothing to do
between frames.

Input goes in from `I_StartTic` as `event_t`s posted with `D_PostEvent`, the keys being `KEYD_*`.

With `W_FETCH` defined, every read of WAD memory goes through the platform's
`W_Fetch(src, dst, len)`, `src` being an address inside one of the two WADs. Pointers the engine
hands the platform point into the WADs too; the platform reads them the same way.

## Platform functions

| Group | Functions |
|---|---|
| system | `I_ZoneBase`, `I_InitTimer`, `I_GetTime`, `I_InitKeyboard`, `I_StartTic`, `I_Quit`, `I_Log`, `I_Fatal`, `W_Fetch` with `W_FETCH` |
| sound | `DMX_Init`, `DMX_Init2`, `DMX_Play` |
| frame | `I_InitGraphics`, `I_SetPalette`, `I_FinishUpdate`, `wipe_StartScreen`, `D_Wipe` |
| 3D view | `R_DrawColumnWall`, `R_DrawColumnSprite`, `R_DrawFuzzColumn`, `R_DrawColumnFlat`, `V_ClearViewWindow` |
| pictures | `V_DrawPatchScaled`, `V_DrawPatchNotScaled`, `V_DrawBackground`, `V_DrawRaw`, `V_DrawRawRect` with `ST_PARTIAL_REFRESH` |
| automap | `V_InitDrawLine`, `V_DrawLine`, `V_ShutdownDrawLine` |

The screen is `SCREEN_WIDTH` × `SCREEN_HEIGHT` palette indices, the status bar its bottom
`ST_HEIGHT` rows; the 3D view is `VIEW_WINDOW_WIDTH` × `VIEW_WINDOW_HEIGHT`. `V_DrawPatchScaled`
takes Doom's 320 × 200 coordinates, `V_DrawPatchNotScaled` the screen's pixels.

The engine never draws the border around a view smaller than the screen above the status bar;
clearing it is up to the platform.
