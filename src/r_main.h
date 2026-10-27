/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
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
 *      Renderer main interface.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __R_MAIN__
#define __R_MAIN__

#include "d_player.h"
#include "r_data.h"


//Global vars.

extern int16_t numnodes;
extern const mapnode_t* nodes;


extern angle_t viewangle;

extern const uint8_t* fixedcolormap;

extern const int16_t  * textureheight; //needed for texture pegging (and TFE fix - killough)


//
// Utility functions.
//

angle_t R_PointToAngle3(fixed_t x, fixed_t y);
#define R_PointToAngle2(x1,y1,x2,y2) R_PointToAngle3((x2)-(x1),(y2)-(y1))
int16_t R_PointInSubsector(fixed_t x, fixed_t y);

void R_InitColormaps(void);
const uint8_t* R_LoadColorMap(int16_t lightlevel);

int16_t V_NumPatchWidth(int16_t num);


//
// REFRESH - the actual rendering functions.
//

void R_RenderPlayerView(player_t *player);   // Called by G_Drawer.

void R_DrawPlanes (void);
visplane_t* R_FindPlane(fixed_t height, int16_t picnum, int16_t lightlevel);
visplane_t* R_DupPlane(const visplane_t* pl, int16_t start, int16_t stop);
visplane_t* R_CheckPlane(visplane_t* pl, int16_t start, int16_t stop);
byte R_GetPlaneColor(int16_t picnum, int16_t lightlevel);
void R_ResetPlanes(void);
void R_ClearPlanes(void);

int16_t R_GetTextureTranslation(int16_t texture);

void R_LoadSkyPatch(void);
void R_FreeSkyPatch(void);
void R_DrawSky(draw_column_vars_t *dcvars);

#endif
