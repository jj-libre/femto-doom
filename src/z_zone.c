// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2023-2026 by Frenkel Smeijers
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Zone Memory Allocation. Neat.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdint.h>
#include "z_zone.h"
#include "doomdef.h"
#include "i_system.h"


//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
#define PU_STATIC		1	// static entire execution time
#define PU_LEVEL		2	// static until level exited
#define PU_LEVSPEC		3	// a special thinker in a level
#define PU_CACHE		4

#define PU_PURGELEVEL PU_CACHE


//
// ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// The rover can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
//

#if defined INSTRUMENTED
    static int32_t running_count = 0;
#endif


#define	ZONEID	0x1dea

// Blocks are addressed by index, in 4-byte units from _g_zonebase.
#define ZONE_UNIT	4
typedef uint16_t zindex_t;

typedef struct
{
    uint16_t  size;			// in ZONE_UNIT, including the header and possibly tiny fragments
    uint8_t   tag;			// purgelevel, 0 for a free block
    uint8_t   unused;
    zindex_t  next;
    zindex_t  prev;
#if defined ZONEIDCHECK
    uint16_t id;			// should be ZONEID
#endif
} memblock_t;

#define HEADER_UNITS	((sizeof(memblock_t) + ZONE_UNIT - 1) / ZONE_UNIT)

// A free fragment smaller than this stays with the block allocated before it.
#if !defined MINFRAGMENT
#define MINFRAGMENT		16
#endif


uint8_t* _g_zonebase;

static memblock_t* mainzone_sentinal;
static zindex_t   mainzone_rover;


static zindex_t pointerToIndex(const void* ptr)
{
	return ((const uint8_t*)ptr - _g_zonebase) / ZONE_UNIT;
}

static memblock_t* indexToPointer(zindex_t i)
{
	return (memblock_t*)(_g_zonebase + (uint32_t)i * ZONE_UNIT);
}


boolean Z_EqualNames(const char* farName, const char* nearName)
{
	return memcmp(farName, nearName, 8) == 0;
}


zref_t Z_Ref(const void* ptr)
{
	if (ptr == NULL)
		return 0;

	uint32_t offset = (const uint8_t*)ptr - _g_zonebase;
	if (offset == 0 || offset > ZREF_SPAN || (offset & ((1 << ZREF_SHIFT) - 1)) != 0)
		I_Error("Z_Ref: 0x%lx is not a zone address", offset);

	return offset >> ZREF_SHIFT;
}


//
// Z_Init
//
void Z_Init (void)
{
	// allocate all available conventional memory.
	uint32_t heapSize;
	static uint8_t* mainzone; mainzone = I_ZoneBase(&heapSize);

	_g_zonebase = mainzone;
	if (heapSize > ZREF_SPAN)
		heapSize = ZREF_SPAN;
	heapSize &= ~(uint32_t)(ZONE_UNIT - 1);

	// the sentinal heads the zone, so no object lives at offset 0
	mainzone_sentinal = (memblock_t*)mainzone;
	zindex_t sentinal = pointerToIndex(mainzone_sentinal);

	// set the rest of the zone to one free block
	mainzone_rover = sentinal + HEADER_UNITS;
	memblock_t* block = indexToPointer(mainzone_rover);

	mainzone_sentinal->size = HEADER_UNITS;
	mainzone_sentinal->tag  = PU_STATIC;
	mainzone_sentinal->next = mainzone_rover;
	mainzone_sentinal->prev = mainzone_rover;

	block->size = heapSize / ZONE_UNIT - HEADER_UNITS;
	block->tag  = 0;
	block->prev = sentinal;
	block->next = sentinal;
#if defined ZONEIDCHECK
	block->id   = ZONEID;
#endif

	I_Printf("%ld bytes allocated for zone\n", (uint32_t)block->size * ZONE_UNIT);
}


static void Z_FreeBlock(memblock_t* block)
{
#if defined ZONEIDCHECK
    if (block->id != ZONEID)
        I_Error("Z_FreeBlock: block has id %x instead of ZONEID", block->id);
#endif

    // mark as free
    block->tag = 0;


#if defined INSTRUMENTED
    running_count -= block->size * ZONE_UNIT;
    I_Printf("Free: %ld\n", running_count);
#endif

    memblock_t* other = indexToPointer(block->prev);

    if (!other->tag)
    {
        // merge with previous free block
        other->size += block->size;
        other->next  = block->next;
        indexToPointer(other->next)->prev = block->prev; // == pointerToIndex(other);

        if (pointerToIndex(block) == mainzone_rover)
            mainzone_rover = block->prev; // == pointerToIndex(other);

        block = other;
    }

    other = indexToPointer(block->next);
    if (!other->tag)
    {
        // merge the next free block onto the end
        block->size += other->size;
        block->next  = other->next;
        indexToPointer(block->next)->prev = pointerToIndex(block);

        if (pointerToIndex(other) == mainzone_rover)
            mainzone_rover = pointerToIndex(block);
    }
}


//
// Z_Free
//
void Z_Free (const void* ptr)
{
	Z_FreeBlock(indexToPointer(pointerToIndex(ptr) - HEADER_UNITS));
}


static uint32_t Z_GetLargestFreeBlockSize(void)
{
	zindex_t sentinal = pointerToIndex(mainzone_sentinal);

	uint32_t largestFreeBlockSize = 0;

	for (memblock_t* block = indexToPointer(mainzone_sentinal->next); pointerToIndex(block) != sentinal; block = indexToPointer(block->next))
		if (!block->tag && (uint32_t)block->size * ZONE_UNIT > largestFreeBlockSize)
			largestFreeBlockSize = (uint32_t)block->size * ZONE_UNIT;

	return largestFreeBlockSize;
}

static uint32_t Z_GetTotalFreeMemory(void)
{
	zindex_t sentinal = pointerToIndex(mainzone_sentinal);

	uint32_t totalFreeMemory = 0;

	for (memblock_t* block = indexToPointer(mainzone_sentinal->next); pointerToIndex(block) != sentinal; block = indexToPointer(block->next))
		if (!block->tag)
			totalFreeMemory += (uint32_t)block->size * ZONE_UNIT;

	return totalFreeMemory;
}


//
// Z_TryMalloc
//
static void* Z_TryMalloc(uint16_t size, int8_t tag)
{
    // scan through the block list,
    // looking for the first free block
    // of sufficient size,
    // throwing out any purgable blocks along the way.

    // account for size of block header
    const uint16_t units = (size + (ZONE_UNIT - 1)) / ZONE_UNIT + HEADER_UNITS;

    // if there is a free block behind the rover,
    //  back up over them
    memblock_t* base = indexToPointer(mainzone_rover);

    memblock_t* previous_block = indexToPointer(base->prev);
    if (!previous_block->tag)
        base = previous_block;

    memblock_t* rover = base;
    zindex_t    start_index = base->prev;

    do
    {
        if (pointerToIndex(rover) == start_index)
        {
            // scanned all the way around the list
            return NULL;
        }

        if (rover->tag)
        {
            if (rover->tag < PU_PURGELEVEL)
            {
                // hit a block that can't be purged,
                //  so move base past it
                base = rover = indexToPointer(rover->next);
            }
            else
            {
                // free the rover block (adding the size to base)

                // the rover can be the base block
                base  = indexToPointer(base->prev);
                Z_FreeBlock(rover);
                base  = indexToPointer(base->next);
                rover = indexToPointer(base->next);
            }
        }
        else
            rover = indexToPointer(rover->next);

    } while (base->tag || base->size < units);
    // found a block big enough

    uint16_t newblock_units = base->size - units;
    if ((uint32_t)newblock_units * ZONE_UNIT > MINFRAGMENT)
    {
        // there will be a free fragment after the allocated block
        zindex_t base_index     = pointerToIndex(base);
        zindex_t newblock_index = base_index + units;

        memblock_t* newblock = indexToPointer(newblock_index);
        newblock->size = newblock_units;
        newblock->tag  = 0;
        newblock->next = base->next;
        newblock->prev = base_index;
#if defined ZONEIDCHECK
        newblock->id   = ZONEID;
#endif

        indexToPointer(base->next)->prev = newblock_index;
        base->size = units;
        base->next = newblock_index;
    }

    base->tag = tag;
#if defined ZONEIDCHECK
    base->id  = ZONEID;
#endif

    // next allocation will start looking here
    mainzone_rover = base->next;

#if defined INSTRUMENTED
    running_count += base->size * ZONE_UNIT;
    I_Printf("Alloc: %ld (%ld)\n", base->size * ZONE_UNIT, running_count);
#endif

    return indexToPointer(pointerToIndex(base) + HEADER_UNITS);
}


static void* Z_Malloc(uint16_t size, int8_t tag) {
	void* ptr = Z_TryMalloc(size, tag);
	if (!ptr)
		I_Error ("Z_Malloc: failed to allocate %u B, max free block %li B, total free %li", size, Z_GetLargestFreeBlockSize(), Z_GetTotalFreeMemory());
	return ptr;
}


void* Z_TryMallocStatic(uint16_t size)
{
	return Z_TryMalloc(size, PU_STATIC);
}


void* Z_MallocStatic(uint16_t size)
{
	return Z_Malloc(size, PU_STATIC);
}


void* Z_MallocLevel(uint16_t size)
{
	return Z_Malloc(size, PU_LEVEL);
}


void* Z_CallocLevel(uint16_t size)
{
    void* ptr = Z_Malloc(size, PU_LEVEL);
    memset(ptr, 0, size);
    return ptr;
}


void* Z_CallocLevSpec(uint16_t size)
{
	void* ptr = Z_Malloc(size, PU_LEVSPEC);
	memset(ptr, 0, size);
	return ptr;
}


//
// Z_FreeTags
//
void Z_FreeTags(void)
{
    zindex_t sentinal = pointerToIndex(mainzone_sentinal);
    memblock_t* next;

    for (memblock_t* block = indexToPointer(mainzone_sentinal->next); pointerToIndex(block) != sentinal; block = next)
    {
        // get link before freeing
        next = indexToPointer(block->next);

        if (PU_LEVEL <= block->tag && block->tag <= (PU_PURGELEVEL - 1))
            Z_FreeBlock(block);
    }
}

//
// Z_CheckHeap
//
void Z_CheckHeap (void)
{
    zindex_t sentinal = pointerToIndex(mainzone_sentinal);

    for (memblock_t* block = indexToPointer(mainzone_sentinal->next); ; block = indexToPointer(block->next))
    {
        if (block->next == sentinal)
        {
            // all blocks have been hit
            break;
        }

#if defined ZONEIDCHECK
        if (block->id != ZONEID)
            I_Error("Z_CheckHeap: block has id %x instead of ZONEID", block->id);
#endif

        if (pointerToIndex(block) + block->size != block->next)
            I_Error ("Z_CheckHeap: block size does not touch the next block\n");

        if (indexToPointer(block->next)->prev != pointerToIndex(block))
            I_Error ("Z_CheckHeap: next block doesn't have proper back link\n");

        if (!block->tag && !indexToPointer(block->next)->tag)
            I_Error ("Z_CheckHeap: two consecutive free blocks\n");
    }
}
