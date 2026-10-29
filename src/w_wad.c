/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2001 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *  Copyright 2023-2026 by
 *  Frenkel Smeijers
 *  Copyright 2026 by
 *  Jakob Jungreuthmayer
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
 *      Handles WAD file header, directory, lump I/O.
 *
 *-----------------------------------------------------------------------------
 */

#include <stdint.h>
#include <string.h>

#include "d_player.h"
#include "doomtype.h"
#include "i_platform.h"
#include "info.h"
#include "w_wad.h"
#include "globdata.h"

typedef struct
{
	int32_t  filepos;
	uint16_t size;
	int16_t  filler;
	char name[8];
} filelump_t;

typedef struct
{
	char identification[4];
	int16_t  numlumps;
	int16_t  filler;
	int32_t  infotableofs;
} wadinfo_t;

static const uint8_t* doom_iwad;
static const uint8_t* doom_iwad_maps;
static const filelump_t* fileinfo;


void W_SetWads(const uint8_t* iwad, const uint8_t* maps)
{
	doom_iwad      = iwad;
	doom_iwad_maps = maps;
}


void W_Init(void)
{
	const wadinfo_t *header = (const wadinfo_t*)&doom_iwad[0];
	fileinfo = (const filelump_t*)&doom_iwad[W_FetchInt32(&header->infotableofs)];

	const uint16_t* graphics = W_GetLumpByName("FEMTOWAD");
	if (W_FetchUInt16(&graphics[0]) != WAD_VERSION
	 || W_FetchUInt16(W_GetMapLumpByNum(W_GetMapNumForName("FEMTOWAD"))) != WAD_VERSION)
		I_Error("W_Init: the WADs are not in WAD format %i", WAD_VERSION);
	if (W_FetchUInt16(&graphics[1]) != SCREEN_WIDTH || W_FetchUInt16(&graphics[2]) != SCREEN_HEIGHT)
		I_Error("W_Init: the graphics WAD is for a %i x %i screen", W_FetchUInt16(&graphics[1]), W_FetchUInt16(&graphics[2]));
}


uint16_t PUREFUNC W_LumpLength(int16_t num)
{
	return W_FetchUInt16(&fileinfo[num].size);
}


static const filelump_t* W_MapFileInfo(void)
{
	const wadinfo_t *mapheader = (const wadinfo_t*)&doom_iwad_maps[0];
	return (const filelump_t*)&doom_iwad_maps[W_FetchInt32(&mapheader->infotableofs)];
}


uint16_t PUREFUNC W_MapLumpLength(int16_t num)
{
	return W_FetchUInt16(&W_MapFileInfo()[num].size);
}


int16_t PUREFUNC W_GetNumForName(const char *name)
{
	char name8[8];
	Z_CopyName(name8, name);

	const wadinfo_t *header = (const wadinfo_t*)&doom_iwad[0];
	const int16_t numlumps = W_FetchInt16(&header->numlumps);

	for (int16_t i = 0; i < numlumps; i++)
	{
		char lumpname[8];
		W_Fetch(fileinfo[i].name, lumpname, sizeof(lumpname));
		if (Z_EqualNames(lumpname, name8))
			return i;
	}

	I_Error("W_GetNumForName: %.8s not found", name);
	return -1;
}


int16_t PUREFUNC W_GetMapNumForName(const char *name)
{
	char name8[8];
	Z_CopyName(name8, name);

	const wadinfo_t *mapheader = (const wadinfo_t*)&doom_iwad_maps[0];
	const filelump_t* mapfileinfo = W_MapFileInfo();

	const int16_t numlumps = W_FetchInt16(&mapheader->numlumps);

	for (int16_t i = 0; i < numlumps; i++)
	{
		char lumpname[8];
		W_Fetch(mapfileinfo[i].name, lumpname, sizeof(lumpname));
		if (Z_EqualNames(lumpname, name8))
			return i;
	}

	I_Error("W_GetMapNumForName: %.8s not found", name);
	return -1;
}


const void* PUREFUNC W_GetLumpByNum(int16_t num)
{
	return &doom_iwad[W_FetchInt32(&fileinfo[num].filepos)];
}


const void* PUREFUNC W_GetMapLumpByNum(int16_t num)
{
	return &doom_iwad_maps[W_FetchInt32(&W_MapFileInfo()[num].filepos)];
}
