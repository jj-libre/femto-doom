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
 *      WAD I/O functions.
 *
 *-----------------------------------------------------------------------------*/


#ifndef __W_WAD__
#define __W_WAD__

#include "doomtype.h"
#include "i_platform.h"


void W_Init(void);

int16_t           PUREFUNC W_GetNumForName(const char *name);
const char* PUREFUNC W_GetNameForNum(int16_t num);
uint16_t          PUREFUNC W_LumpLength(   int16_t num);
const void* PUREFUNC W_GetLumpByNum( int16_t num);
void                       W_ReadLumpByNum(int16_t num, void* ptr);

int16_t           PUREFUNC W_GetMapNumForName(const char *name);
uint16_t          PUREFUNC W_MapLumpLength(   int16_t num);
const void* PUREFUNC W_GetMapLumpByNum( int16_t num);

#define W_GetLumpByName(x)    W_GetLumpByNum(W_GetNumForName(x))

#if !defined W_FETCH
#include <string.h>
#define W_Fetch(src, dst, len) memcpy(dst, src, len)
#endif

static inline uint8_t W_FetchUInt8(const void* src)
{
	uint8_t v;
	W_Fetch(src, &v, sizeof(v));
	return v;
}

static inline int16_t W_FetchInt16(const void* src)
{
	int16_t v;
	W_Fetch(src, &v, sizeof(v));
	return v;
}

static inline uint16_t W_FetchUInt16(const void* src)
{
	uint16_t v;
	W_Fetch(src, &v, sizeof(v));
	return v;
}

static inline int32_t W_FetchInt32(const void* src)
{
	int32_t v;
	W_Fetch(src, &v, sizeof(v));
	return v;
}

#endif
