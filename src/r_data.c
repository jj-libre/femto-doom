/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2002 by
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
 *      Preparation of data for rendering,
 *      generation of lookups, caching, retrieval by name.
 *
 *-----------------------------------------------------------------------------*/

#include <stdint.h>

#include "d_player.h"
#include "w_wad.h"
#include "r_main.h"
#include "m_fixed.h"
#include "i_platform.h"
#include "r_things.h"
#include "p_tick.h"
#include "p_tick.h"

#include "globdata.h"


int16_t V_NumPatchWidth(int16_t num)
{
	const patch_t* patch = W_GetLumpByNum(num);
	int16_t width = W_FetchInt16(&patch->width);
	return width;
}


//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//

typedef struct
{
  int16_t originx;
  int16_t originy;
  int16_t patch;
} mappatch_t;

typedef char assertMappatchSize[sizeof(mappatch_t) == 6 ? 1 : -1];


typedef struct
{
  char       name[8];
  int16_t      width;
  int16_t      height;
  int16_t      patchcount;
  mappatch_t patches[1];
} maptexture_t;

typedef char assertMaptextureSize[sizeof(maptexture_t) == 20 ? 1 : -1];


// A maptexture_t describes a rectangular texture, which is composed
// of one or more mappatch_t structures that arrange graphic patches.

static const uint16_t *texturep;
static int16_t numtextures;
static const byte* texcols;


const texture_t* R_GetTexture(int16_t texture)
{
#ifdef RANGE_CHECK
    if (texture >= numtextures)
        I_Error("R_GetTexture: Texture %d not in range.", texture);
#endif

#if defined ONE_WALL_TEXTURE
    texture = 46;
#endif

    return (texture_t*)((const byte*)texturep + W_FetchUInt16(&texturep[texture]));
}

texcols_t R_GetTexCols(int16_t texture)
{
    struct { uint32_t offset; uint16_t widthmask; uint8_t cshift, pad; } entry;
    W_Fetch(texcols + 8 + 8 * texture, &entry, sizeof(entry));

    texcols_t tex = { texcols + entry.offset, entry.widthmask, entry.cshift };
    return tex;
}

static int16_t R_GetTextureNumForName(const char* tex_name)
{
    char name8[8];
    Z_CopyName(name8, tex_name);

    const int32_t* maptex = W_GetLumpByName("TEXTURE1");
    const int32_t* directory = maptex+1;

    for (int16_t i = 0; i < numtextures; i++)
    {
        int32_t offset = W_FetchInt32(directory++);

        const maptexture_t* mtexture = (const maptexture_t*) ( (const byte*)maptex + offset);

        char name[8];
        W_Fetch(mtexture->name, name, sizeof(name));
        if (Z_EqualNames(name, name8))
        {
            return i;
        }
    }

    I_Error("R_GetTextureNumForName: texture name: %s not found.", tex_name);
    return -1;
}




//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
// Rewritten by Lee Killough to use hash table for fast lookup. Considerably
// reduces the time needed to start new levels. See w_wad.c for comments on
// the hashing algorithm, which is also used for lump searches.
//
// killough 1/21/98, 1/31/98
//

#define NO_TEXTURE 0

int16_t PUREFUNC R_CheckTextureNumForName (const char *tex_name)
{
    // "NoTexture" marker.
    if (tex_name[0] == '-')
        return NO_TEXTURE;

    return R_GetTextureNumForName(tex_name);
}


//
// R_InitTextures
// Initializes the texture list
//  with the textures from the world map.
//

static void R_InitTextures()
{
	const int32_t* mtex1 = W_GetLumpByName("TEXTURE1");
	numtextures = W_FetchInt32(mtex1);

	texturep = W_GetLumpByName("TEXTUREP");

	textureheight = W_GetLumpByName("TEXHEIGH");

#if defined HIGH_DETAIL
	const uint8_t highdetail = 1;
#else
	const uint8_t highdetail = 0;
#endif
	texcols = W_GetLumpByName("TEXCOLS");
	uint8_t header[3];
	W_Fetch(texcols, header, sizeof(header));
	if (header[0] != TEXTURE_SHIFT || header[1] != highdetail || header[2] != TEXCOLBITS)
		I_Error("R_InitTextures: TEXCOLS built for TEXTURE_SHIFT %i", header[0]);
}


//
// R_Init
// Locates all the lumps
//  that will be used by all views
// Must be called after W_Init.
//

void R_Init(void)
{
  R_InitTextures();
  R_InitSky();
  R_InitSpriteLumps();
  R_InitColormaps();
  R_InitClipArrays();
}
