/*-----------------------------------------------------------------------------
 *
 *
 *  Copyright (C) 2023-2026 Frenkel Smeijers
 *  Copyright (C) 2026 Jakob Jungreuthmayer
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
 *      Render sky
 *
 *-----------------------------------------------------------------------------*/

#include "r_defs.h"
#include "r_main.h"
#include "w_wad.h"
#include "r_view.h"


#define ANGLETOSKYSHIFT         22

#define COLEXTRABITS (8 - 1)


const int16_t skyflatnum = -2;

static uint16_t skywidthmask;


static const patch_t* skypatch;


void R_LoadSkyPatch(void)
{
	// Do nothing
}


void R_FreeSkyPatch(void)
{
	// Do nothing
}


void R_DrawSky(draw_column_vars_t *dcvars)
{
	dcvars->texturemid = (SCREEN_HEIGHT_VGA / 2) * FRACUNIT;

	if (!(dcvars->colormap = fixedcolormap))
		dcvars->colormap = fullcolormap;

	dcvars->fracstep = ((FRACUNIT * SCREEN_HEIGHT_VGA) / (VIEW_WINDOW_HEIGHT + 16)) >> COLEXTRABITS;

	int16_t xc = viewangle >> FRACBITS;
	xc += xtoviewangleTable[dcvars->x];
	xc >>= ANGLETOSKYSHIFT - FRACBITS;
	xc &= skywidthmask;

	const column_t* column = (const column_t*) ((const byte*)skypatch + W_FetchInt32(&skypatch->columnofs[xc]));

	dcvars->source = (const byte*)column + 3;
	R_DrawColumnWall(dcvars);
}



// Set the sky map.
void R_InitSky(void)
{
	int16_t skytexture = R_CheckTextureNumForName("SKY1");
	texture_t tex;
	W_Fetch(R_GetTexture(skytexture), &tex, sizeof(tex));
	skywidthmask = tex.widthmask;

	skypatch = W_GetLumpByNum(tex.patches[0].patch_num);
}
