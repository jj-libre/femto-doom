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
 *      Refresh/rendering module, shared data struct definitions.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __R_DEFS__
#define __R_DEFS__

// Screenwidth.
#include "doomdef.h"

// Some more or less basic data types
// we depend on.
#include "m_fixed.h"

// We rely on the thinker data struct
// to handle sound origins in sectors.
#include "d_think.h"

// SECTORS do store MObjs anyway.
#include "p_mobj.h"

#include "w_wad.h"


//
// INTERNAL MAP TYPES
//  used by play and refresh
//

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally,
// like some DOOM-alikes ("wt", "WebView") do.
//
typedef struct
{
  fixed_t x, y;
} vertex_t;

typedef struct
{
  int16_t x, y;
} vertex16_t;

// Each sector has a degenmobj_t in its center for sound origin purposes.
typedef struct
{
  fixed_t x;
  fixed_t y;
} degenmobj_t;

typedef char assertDegenmobjSize[sizeof(degenmobj_t) == 8 ? 1 : -1];


//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//

// Its lines are in the map WAD's E1MxGL; P_SectorSoundOrg finds its sound origin.
// Its things and touching nodes are in chains shared by SECBUCKETS sectors, and the
// sound target is _g_soundtarget for every sector with SB_HEARD.
typedef struct
{
  int16_t floorheight;    // in quarter map units, SEC_FLOORHEIGHT
  int16_t ceilingheight;

  uint8_t floorpic;       // P_FlatByte, SEC_FLOORPIC
  uint8_t ceilingpic;

  uint8_t lightlevel;
  int8_t special;
} sector_t;

// A flat in a byte: 0xFD and up are the negative flats (sky, nukage).
static inline int16_t SEC_FLAT(uint8_t p)
{
  return p >= 0xFD ? p - 0x100 : p;
}
#define SEC_FLOORPIC(s)      SEC_FLAT((s)->floorpic)
#define SEC_CEILINGPIC(s)    SEC_FLAT((s)->ceilingpic)
#define SEC_CEILINGSKY(s)    ((s)->ceilingpic == (uint8_t)skyflatnum)
uint8_t P_FlatByte(int16_t pic);

#define SECBUCKETS 16
extern zref_t* _g_secthings;    // mobj_t chains by sector index, SECBUCKETS of them
extern zref_t* _g_sectouching;  // msecnode_t chains by sector index
#define SEC_THINGS(s)   (_g_secthings[((s) - _g_sectors) & (SECBUCKETS - 1)])
#define SEC_TOUCHING(s) (_g_sectouching[((s) - _g_sectors) & (SECBUCKETS - 1)])

extern zref_t _g_soundtarget;

// Planes move by quarter units between heights that are whole or quarter units, so a
// quarter unit is exact. P_PlaneHeight refuses any other height and any beyond 8191 units.
#define SEC_FLOORHEIGHT(s)   ((fixed_t)(s)->floorheight << (FRACBITS - 2))
#define SEC_CEILINGHEIGHT(s) ((fixed_t)(s)->ceilingheight << (FRACBITS - 2))
int16_t P_PlaneHeight(fixed_t height);

// A sector gets a mover_t when a floor or ceiling thinker first acts on it.
typedef struct
{
  int16_t sector;

  // thinker_t for reversable actions
  zref_t floordata;    // jff 2/22/98 make thinkers on
  zref_t ceilingdata;  // floors, ceilings, lighting,
} mover_t;

extern mover_t* _g_movers;

mover_t* P_Mover(sector_t* sector);
zref_t SEC_FLOORDATA(const sector_t* s);
zref_t SEC_CEILINGDATA(const sector_t* s);

// A bit per sector in each plane of _g_secbits.
enum
{
  SB_CHECKED,  // since validcount last stepped
  SB_SECRET,   // jff 2/16/98 remembers if sector WAS secret (automap)
  SB_SOUND2,   // the sound that flooded it last crossed a sound-blocking line
  SB_SPREAD,   // the sound still has to spread from it
  SB_HEARD,    // _g_soundtarget is its sound target
  SB_PLANES
};

extern uint8_t* _g_secbits;
extern int16_t _g_secbytes;
#define SEC_BIT(p, i)    (_g_secbits[(p) * _g_secbytes + ((i) >> 3)] & (1 << ((i) & 7)))
#define SEC_SETBIT(p, i) (_g_secbits[(p) * _g_secbytes + ((i) >> 3)] |= (1 << ((i) & 7)))
#define SEC_CLRBIT(p, i) (_g_secbits[(p) * _g_secbytes + ((i) >> 3)] &= ~(1 << ((i) & 7)))

typedef char assertSectorSize[sizeof(sector_t) == 8 ? 1 : -1];


typedef struct {
  int16_t floorheight;
  int16_t ceilingheight;
  int16_t floorpic;
  int16_t ceilingpic;
  uint8_t lightlevel;
  int8_t special;
  int16_t tag;
} mapsector_t;

typedef char assertMapsectorSize[sizeof(mapsector_t) == 12 ? 1 : -1];


//
// The SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
//

// The sector it faces is in the map WAD's SIDEDEFS: SIDE_SECTOR.
typedef struct
{
    uint8_t textureoffset; // add this to the calculated texture column

    int8_t toptexture;
    int8_t bottomtexture;
    int8_t midtexture;
} side_t;

typedef char assertSideSize[sizeof(side_t) == 4 ? 1 : -1];


typedef struct {
  int16_t textureoffset;
  uint8_t rowoffset;
   int8_t toptexture;
   int8_t bottomtexture;
   int8_t midtexture;
  uint8_t sector;  // Front sector, towards viewer.
  uint8_t changeable;
} mapsidedef_t;

typedef char assertMapsidedefSize[sizeof(mapsidedef_t) == 8 ? 1 : -1];

// Play changes only the sides at the front of lines with a special. femtowad numbers them
// from 1 in mapsidedef_t.changeable, and only they are in _g_sides.
uint8_t P_GetSide(int16_t n, side_t* side);  // returns the row offset
side_t* P_ChangeableSide(int16_t n);


//
// Move clipping aid for LineDefs.
//

#define ST_HORIZONTAL	0
#define ST_VERTICAL		1
#define ST_POSITIVE		2
#define ST_NEGATIVE		3

typedef enum
{
    RF_IGNORE   = 8,     // Renderer can skip this line
    RF_CLOSED   =16      // Line blocks view
} flags_e;


// What play and rendering change about a line: a bit per line in each plane of
// _g_linebits. LB_DONE says LB_CLOSED and LB_IGNORE are this tic's; at load every line
// is done with neither. LB_CHECKED is a line's validcount
// matching, so each step of validcount clears it.
enum
{
    LB_MAPPED,   // seen, for the automap
    LB_CLEARED,  // its special has been used up
    LB_DONE,
    LB_CLOSED,
    LB_IGNORE,
    LB_CHECKED,
    LB_PLANES
};

#define LN_BIT(p, i)    (_g_linebits[(p) * _g_linebytes + ((i) >> 3)] & (1 << ((i) & 7)))
#define LN_SETBIT(p, i) (_g_linebits[(p) * _g_linebytes + ((i) >> 3)] |= (1 << ((i) & 7)))
#define LN_CLRBIT(p, i) (_g_linebits[(p) * _g_linebytes + ((i) >> 3)] &= ~(1 << ((i) & 7)))

typedef struct line_s
{
	vertex16_t v1;
	vertex16_t v2;			// Vertices, from v1 to v2.
	uint16_t lineno;		// line number.
	int16_t dx, dy;			// Precalculated v2 - v1 for side checking.

	uint16_t sidenum[2];	// Visual appearance: SideDefs. front and back.
	int16_t bbox[4];		// Line bounding box.

	int16_t tag;
	uint8_t flags;			// Animation related.
	int8_t slopetype;		// To aid move clipping.
	int16_t const_special;
} line_t;

typedef char assertLineSize[sizeof(line_t) == 32 ? 1 : -1];


#define LN_FLAGS(l)      W_FetchUInt8(&(l)->flags)
#define LN_TAG(l)        W_FetchInt16(&(l)->tag)
#define LN_SIDENUM(l, s) W_FetchUInt16(&(l)->sidenum[s])
#define LN_LINENO(l)     W_FetchUInt16(&(l)->lineno)

sector_t* LN_BackSector(const line_t* l);

#define LN_FRONTSECTOR(l) (SIDE_SECTOR(LN_SIDENUM(l, 0)))
#define LN_BACKSECTOR(l) LN_BackSector(l)

#define LN_SPECIAL(l) (LN_BIT(LB_CLEARED, LN_LINENO(l)) ? 0 : W_FetchInt16(&(l)->const_special))
#define LN_CLEARSPECIAL(l) LN_SETBIT(LB_CLEARED, LN_LINENO(l))

#define SIDE_SECTOR(n) (&_g_sectors[W_FetchUInt8(&_g_mapsides[n].sector)])

#define SEC_LINEFIRST(s) W_FetchUInt16(&_g_seclinefirst[(s) - _g_sectors])
#define SEC_LINECOUNT(s) (W_FetchUInt16(&_g_seclinefirst[(s) - _g_sectors + 1]) - SEC_LINEFIRST(s))
#define SEC_LINE(s, i) (&_g_maplines[W_FetchInt16(&_g_seclines[SEC_LINEFIRST(s) + (i)])])


// phares 3/14/98
//
// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_snext
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in an mobj_t and flows
// through the m_tnext links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

typedef struct msecnode_s
{
  zref_t m_sector; // a sector_t containing this object
  zref_t m_thing;  // this mobj_t
  zref_t m_tprev;  // prev msecnode_t for this thing
  zref_t m_tnext;  // next msecnode_t for this thing
  zref_t m_sprev;  // prev msecnode_t for this sector
  zref_t m_snext;  // next msecnode_t for this sector
  uint8_t visited; // killough 4/4/98, 4/7/98: used in search algorithms
} msecnode_t;

typedef char assertMsecnodeSize[sizeof(msecnode_t) == 14 ? 1 : -1];

static inline msecnode_t* P_Node(zref_t ref)
{
  return Z_Ptr(ref);
}

static inline sector_t* P_Sector(zref_t ref)
{
  return Z_Ptr(ref);
}

//
// The LineSeg.
//

/*
typedef struct
{
  vertex_t *v1, *v2;
  fixed_t offset;
  angle_t angle;
  side_t* sidedef;
  const line_t* linedef;

  // Sector references.
  // Could be retrieved from linedef, too
  // (but that would be slower -- killough)
  // backsector is NULL for one sided lines

  sector_t *frontsector, *backsector;
} seg_t;
*/

//
// The LineSeg.
//
typedef struct
{
    vertex16_t v1;
    vertex16_t v2;            // Vertices, from v1 to v2.

    int16_t offset;
    angle16_t angle;

    uint16_t sidenum;
    uint16_t linenum;

    uint8_t frontsectornum;
    uint8_t backsectornum;
} seg_t;

typedef char assertSegSize[sizeof(seg_t) == 18 ? 1 : -1];


//
// A SubSector.
// References a Sector.
// Basically, this is a list of LineSegs,
//  indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//

// Its sector is in the map WAD's E1MxGL.
#define P_SubsectorSector(ss) (&_g_sectors[W_FetchUInt8(&_g_subsecsector[ss])])


typedef struct {
	int16_t		numsegs;
	int16_t		firstseg;
} mapsubsector_t;

typedef char assertMapsubsectorSize[sizeof(mapsubsector_t) == 4 ? 1 : -1];


//
// OTHER TYPES
//

// Patches, posts and columns are in i_platform.h.


//
// Sprites are patches with a special naming convention
//  so they can be recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with
//  x indicating the rotation, x = 0, 1-7.
// The sprite and frame specified by a thing_t
//  is range checked at run time.
// A sprite is a patch_t that is assumed to represent
//  a three dimensional object and may have multiple
//  rotations pre drawn.
// Horizontal flipping is used to save space,
//  thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used
// for all views: NNNNF0
//

typedef struct
{
  // Lump to use for view angles 0-7.
  int16_t lump[8];

  // Flip bit (1 = flip) to use for view angles 0-7.
  //byte  flip[8];
  byte flipmask;

  // If false use 0 for any position.
  // Note: as eight entries are available,
  //  we might as well insert the same name eight times.
  byte rotate;

} spriteframe_t;

typedef char assertSpriteframeSize[sizeof(spriteframe_t) == 18 ? 1 : -1];


//
// Now what is a visplane, anyway?
//
// Go to http://classicgaming.com/doom/editing/ to find out -- killough
//

typedef struct visplane
{
  struct visplane* next;        // Next visplane in hash chain -- killough
  int16_t picnum, lightlevel;
  int16_t minx, maxx;
  fixed_t height;
  boolean modified;

  byte		pad1;
  byte		pad2;
  byte		pad3;
  // Here lies the rub for all
  //  dynamic resize/change of resolution.
  byte		top[VIEW_WINDOW_WIDTH];
  byte		pad4;
  byte		pad5;
  // See above.
  byte		bottom[VIEW_WINDOW_WIDTH];
  byte		pad6;

} visplane_t;

#endif
