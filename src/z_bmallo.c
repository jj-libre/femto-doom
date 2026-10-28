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
 * This is designed to be a fast allocator for small, regularly used block sizes
 *-----------------------------------------------------------------------------
 */

#include <string.h>

#include "doomtype.h"
#include "z_zone.h"
#include "z_bmallo.h"
#include "i_platform.h"


typedef struct bmalpool_s {
	struct bmalpool_s* nextpool;
	uint32_t               used;
} bmalpool_t;

typedef char assertBmalpoolSize[sizeof(void*) != 4 || sizeof(bmalpool_t) == 8 ? 1 : -1];


inline static void* getelem(bmalpool_t* p, size_t size, size_t n)
{
	return ((byte*)p) + sizeof(bmalpool_t) + size * n;
}


inline static PUREFUNC int16_t iselem(const bmalpool_t* pool, size_t size, const void* p)
{
	int32_t dif = (const char*)p - (const char*)pool;
	if (dif < 0)
		return -1;

	dif -= sizeof(bmalpool_t);
	dif /= size;
	return (uint32_t)dif >= CHAR_BIT * sizeof(uint32_t) ? -1 : dif;
}


void* Z_BMalloc(struct block_memory_alloc_s *pzone)
{
	bmalpool_t** pool = (bmalpool_t**)&(pzone->firstpool);
	while (*pool != NULL) {
		if ((*pool)->used == 0xffffffff) {
			pool = &((*pool)->nextpool);
		} else {
			size_t n = __builtin_ctz(~(*pool)->used);
			(*pool)->used |= (1U << n);
			return getelem(*pool, pzone->size, n);
		}
	}

	// Nothing available, must allocate a new pool
	bmalpool_t* newpool;

	// CPhipps: Allocate new memory, initialised to 0

	*pool = newpool = Z_CallocLevel(sizeof(bmalpool_t) + pzone->size * CHAR_BIT * sizeof(uint32_t));

	// Return element 0 from this pool to satisfy the request
	newpool->used = 1;
	return getelem(newpool, pzone->size, 0);
}


void Z_BFree(struct block_memory_alloc_s *pzone, void* p)
{
	bmalpool_t** pool = (bmalpool_t**)&(pzone->firstpool);

	while (*pool != NULL) {
		int16_t n = iselem(*pool, pzone->size, p);
		if (n >= 0) {
			(*pool)->used &= ~(1U << n);
			if ((*pool)->used == 0) {
				// Block is all unused, can be freed
				bmalpool_t* oldpool = *pool;
				*pool = (*pool)->nextpool;
				Z_Free(oldpool);
			}
			return;
		} else
			pool = &((*pool)->nextpool);
	}

	I_Error("Z_BFree: Free not in zone");
}
