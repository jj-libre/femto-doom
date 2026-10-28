// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2023-2026 by Frenkel Smeijers
// Copyright (C) 2026 by Jakob Jungreuthmayer
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//	Remark: this was the only stuff that, according
//	 to John Carmack, might have been useful for
//	 Quake.
//
//---------------------------------------------------------------------

#ifndef __Z_ZONE__
#define __Z_ZONE__

#include <stddef.h>
#include <stdint.h>
#include "doomtype.h"

void Z_Init(void);
void* Z_TryMallocStatic(uint16_t size);
void* Z_MallocStatic(uint16_t size);
void* Z_MallocLevel(uint16_t size);
void* Z_CallocLevel(uint16_t size);
void* Z_CallocLevSpec(uint16_t size);
void Z_Free(const void* ptr);
void Z_FreeTags(void);
void Z_CheckHeap(void);

boolean Z_EqualNames(const char* farName, const char* nearName);
void Z_CopyName(char* name8, const char* name);

// A zone address in units of 1 << ZREF_SHIFT bytes. 0 is NULL: the zone
// starts with a block header, so no object lives at offset 0.
typedef uint16_t zref_t;

#define ZREF_SHIFT	1
#define ZREF_SPAN	((uint32_t)UINT16_MAX << ZREF_SHIFT)

extern uint8_t* _g_zonebase;

zref_t Z_Ref(const void* ptr);

static inline void* Z_Ptr(zref_t ref)
{
	return ref ? _g_zonebase + ((uint32_t)ref << ZREF_SHIFT) : NULL;
}

#endif
