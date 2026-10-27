/*-----------------------------------------------------------------------------
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *  Copyright 2023-2026 by
 *  Frenkel Smeijers
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      The contract between the engine and a platform: the functions each side
 *      calls and the data the platform is handed.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __I_PLATFORM__
#define __I_PLATFORM__

#include <stdint.h>

#include "doomtype.h"
#include "d_event.h"
#include "m_fixed.h"
#include "sounds.h"


//
// The screen, in palette indices
//

#if !defined SCREEN_WIDTH
#define SCREEN_WIDTH  240
#endif

#if !defined SCREEN_HEIGHT
#define SCREEN_HEIGHT 160
#endif

// The status bar's rows at the bottom of the screen: Doom's 32 of 200 scaled, at least 32.
#define ST_HEIGHT (SCREEN_HEIGHT > 200 ? (32 * SCREEN_HEIGHT + 100) / 200 : 32)

// I_GetTime counts tics, this many per second.
#define TIC_RATE          35


//
// Keys the platform posts with D_PostEvent
//

#define KEYD_A              0
#define KEYD_B              1
#define KEYD_L              2
#define KEYD_R              3
#define KEYD_UP             4
#define KEYD_DOWN           5
#define KEYD_LEFT           6
#define KEYD_RIGHT          7
#define KEYD_START          8
#define KEYD_SELECT         9
#define KEYD_MINUS         10
#define KEYD_PLUS          11


//
// Data the platform is handed
//

// A picture in Doom's patch format: a header, then per column the offset of its posts.
typedef struct
{
    int16_t		width;		// bounding box size
    int16_t		height;
    int16_t		leftoffset;	// pixels to the left of origin
    int16_t		topoffset;	// pixels below the origin
    int32_t		columnofs[];	// width offsets from the patch, upper word always zero
} patch_t;

// A post is a run of opaque pixels in a column.
typedef struct
{
  byte topdelta; // -1 is the last post in a column
  byte length;   // length data bytes follows
} post_t;

// A column is a list of 0 or more posts, (byte)-1 terminated.
typedef post_t	column_t;

// One column of the 3D view to draw: rows yl to yh at x. Row y shows texel
// source[frac >> (9 + shift)] through colormap, with the 16-bit
// frac = (texturemid >> 7) + (y - CENTER_Y) * fracstep, shift being TEXTURE_SHIFT for
// walls and SPRITE_SHIFT for sprites.
typedef struct {
  int16_t              x;
  int16_t              yl;
  int16_t              yh;
  uint16_t             fracstep;
  fixed_t              texturemid;

  const byte   * source; // first pixel in a column

  const uint8_t* colormap;
} draw_column_vars_t;


// The row of the 3D view the horizon is on.
extern const int16_t CENTER_Y;

// The light levels of COLORMAP, 34 maps of 256 palette indices.
extern const uint8_t *fullcolormap;


//
// The platform calls these
//

// The graphics WAD and the map WAD, as the platform maps them, both 4-byte aligned.
void W_SetWads(const uint8_t* iwad, const uint8_t* maps);

// Sets the game up and returns; the platform then calls D_DoomStep for every frame.
void D_DoomInit(int argc, const char * const * argv);
void D_DoomStep(void);
// D_DoomInit, then D_DoomStep forever.
_Noreturn void D_DoomMain(int argc, const char * const * argv);

void D_PostEvent(event_t* ev);


//
// The platform provides these
//

// Memory for the zone allocator, aligned like malloc's, its size in *heapSize; at most
// 128 KiB of it is used.
uint8_t* I_ZoneBase(uint32_t *heapSize);

void I_InitTimer(void);
// Tics since I_InitTimer.
int32_t I_GetTime(void);

// Input: called before each tic is processed; posts what happened with D_PostEvent.
void I_InitKeyboard(void);
void I_StartTic(void);

_Noreturn void I_Quit(void);
_Noreturn void I_Error(const char *error, ...);

// Log output; defining I_Printf names the platform's function, else it is printf.
#if defined I_Printf
void I_Printf(const char *format, ...);
#else
#include <stdio.h>
#define I_Printf printf
#endif

// With W_FETCH defined, every read of WAD memory goes through W_Fetch, src being an address
// inside one of the two WADs.
#if defined W_FETCH
void W_Fetch(const void* src, void* dst, uint16_t len);
#endif

// Sound effects.
void DMX_Init(void);
void DMX_Init2(void);
void DMX_Play(sfxenum_t id);
void DMX_Shutdown(void);

void I_InitGraphics(void);

// 256 RGB triples, inside the graphics WAD; called after I_InitGraphics. A platform may hold a
// new palette back until the next I_FinishUpdate.
void I_SetPalette(const uint8_t* palette);

// The frame is complete.
void I_FinishUpdate(void);

// The 3D view, column by column.
void R_DrawColumnWall(const draw_column_vars_t *dcvars);
void R_DrawColumnSprite(const draw_column_vars_t *dcvars);
void R_DrawFuzzColumn(const draw_column_vars_t *dcvars);
void R_DrawColumnFlat(uint8_t color, const draw_column_vars_t *dcvars);
void V_ClearViewWindow(void);

// Pictures on the screen: Scaled in Doom's 320 x 200 coordinates, NotScaled in the screen's pixels.
// The top left corner is at x - leftoffset, y - topoffset; a picture may reach past the
// screen's edges.
void V_DrawPatchScaled(int16_t x, int16_t y, const patch_t* patch);
void V_DrawPatchNotScaled(int16_t x, int16_t y, const patch_t* patch);
// The screen tiled with a 64 x 64 picture.
void V_DrawBackground(const uint8_t* tile);
// length pixels of a raw picture at screen offset `offset`.
void V_DrawRaw(uint32_t offset, const uint8_t* pixels, uint32_t length);
#if defined ST_PARTIAL_REFRESH
// The w x h rectangle at x, y of a raw picture whose rows are SCREEN_WIDTH wide; pixels is the
// rectangle's first pixel.
void V_DrawRawRect(const uint8_t* pixels, int16_t x, int16_t y, int16_t w, int16_t h);
#endif

// Automap lines.
void V_InitDrawLine(void);
void V_ShutdownDrawLine(void);
void V_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);

// Screen wipe between game states; may just finish the frame.
void wipe_StartScreen(void);
void D_Wipe(void);

#endif
