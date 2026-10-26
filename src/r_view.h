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
 *      Tangents and, per supported view width, the view angle tables.
 *      Written by tools/femtowad from src/femtowad/viewgen.py; edit that and run
 *      `uv run --project tools/femtowad femtowad generate`.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __R_VIEW__
#define __R_VIEW__

#include <stdint.h>

#include "doomdef.h"
#include "m_fixed.h"
#include "tables.h"

#if VIEW_WINDOW_WIDTH == 30
#define VIEWANGLETOXMAX 1068
#elif VIEW_WINDOW_WIDTH == 38
#define VIEWANGLETOXMAX 1059
#elif VIEW_WINDOW_WIDTH == 40
#define VIEWANGLETOXMAX 1057
#elif VIEW_WINDOW_WIDTH == 60
#define VIEWANGLETOXMAX 1046
#elif VIEW_WINDOW_WIDTH == 80
#define VIEWANGLETOXMAX 1040
#elif VIEW_WINDOW_WIDTH == 120
#define VIEWANGLETOXMAX 1034
#elif VIEW_WINDOW_WIDTH == 160
#define VIEWANGLETOXMAX 1032
#elif VIEW_WINDOW_WIDTH == 240
#define VIEWANGLETOXMAX 1029
#elif VIEW_WINDOW_WIDTH == 320
#define VIEWANGLETOXMAX 1028
#else
#error unsupported VIEW_WINDOW_WIDTH value
#endif

#if VIEW_WINDOW_WIDTH < 256
typedef uint8_t viewx_t;
#else
typedef uint16_t viewx_t;
#endif

// The column each view angle falls on, from VIEWANGLETOXMAX to 3073.
extern const viewx_t viewangletoxTable[4096 - 1023 - VIEWANGLETOXMAX];

// The view angle of each column.
extern const angle16_t xtoviewangleTable[VIEW_WINDOW_WIDTH + 1];

#if defined SMALL_TRIG
extern const fixed_t finetangentHalf[2048];
#else
extern const fixed_t finetangentTable[4096];
#endif

#endif
