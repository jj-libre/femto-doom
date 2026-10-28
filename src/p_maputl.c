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
 *      Movement/collision utility functions,
 *      as used by function in p_map.c.
 *      BLOCKMAP Iterator functions,
 *      and some PIT_* functions to use for iteration.
 *
 *-----------------------------------------------------------------------------*/

#include "d_player.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_setup.h"
#include "i_platform.h"
#include "info.h"
#include "m_random.h"
#include "w_wad.h"
#include "p_tick.h"
#include <stdio.h>

#include "globdata.h"


fixed_t _g_opentop;
fixed_t _g_openbottom;
fixed_t _g_openrange;
fixed_t _g_lowfloor;


divline_t _g_trace;


// 1/11/98 killough: Intercept limit removed
#if !defined MAX_INTERCEPTS
#define MAX_INTERCEPTS 64
#endif
static intercept_t intercepts[MAX_INTERCEPTS];
static intercept_t* intercept_p;


//
// P_AproxDistance
// Gives an estimation of distance (not exact)
//

fixed_t CONSTFUNC P_AproxDistance(fixed_t dx, fixed_t dy)
{
  dx = D_abs(dx);
  dy = D_abs(dy);
  if (dx < dy)
    return dx+dy-(dx>>1);
  return dx+dy-(dy>>1);
}

//
// P_PointOnLineSide
// Returns 0 or 1
//
// killough 5/3/98: reformatted, cleaned up

static int16_t PUREFUNC P_PointOnLineCopySide(fixed_t x, fixed_t y, const line_t* line)
{
  return
    !line->dx ? x <= (fixed_t)line->v1.x<<FRACBITS ? line->dy > 0 : line->dy < 0 :
    !line->dy ? y <= (fixed_t)line->v1.y<<FRACBITS ? line->dx < 0 : line->dx > 0 :
    //FixedMul(y-((fixed_t)line->v1.y<<FRACBITS), line->dx>>FRACBITS) >= FixedMul(line->dy>>FRACBITS, x-((fixed_t)line->v1.x<<FRACBITS));
    ((y - ((fixed_t)line->v1.y<<FRACBITS)) >> 8) * line->dx >= line->dy * ((x - ((fixed_t)line->v1.x<<FRACBITS)) >> 8);
}

int16_t P_PointOnLineSide(fixed_t x, fixed_t y, const line_t* line)
{
  line_t l;
  W_Fetch(line, &l, sizeof(l));
  return P_PointOnLineCopySide(x, y, &l);
}

//
// P_BoxOnLineSide
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
// killough 5/3/98: reformatted, cleaned up

static int16_t PUREFUNC P_BoxOnLineCopySide(const fixed_t *tmbox, const line_t* ld)
{
    int16_t p;
    switch (ld->slopetype)
    {

    default: // shut up compiler warnings -- killough
    case ST_HORIZONTAL:
        return
                (tmbox[BOXBOTTOM] > (fixed_t)ld->v1.y<<FRACBITS) == (p = tmbox[BOXTOP] > (fixed_t)ld->v1.y<<FRACBITS) ?
                    p ^ (ld->dx < 0) : -1;
    case ST_VERTICAL:
        return
                (tmbox[BOXLEFT] < (fixed_t)ld->v1.x<<FRACBITS) == (p = tmbox[BOXRIGHT] < (fixed_t)ld->v1.x<<FRACBITS) ?
                    p ^ (ld->dy < 0) : -1;
    case ST_POSITIVE:
        return
                P_PointOnLineCopySide(tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld) ==
                (p = P_PointOnLineCopySide(tmbox[BOXLEFT], tmbox[BOXTOP], ld)) ? p : -1;
    case ST_NEGATIVE:
        return
                (P_PointOnLineCopySide(tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld)) ==
                (p = P_PointOnLineCopySide(tmbox[BOXRIGHT], tmbox[BOXTOP], ld)) ? p : -1;
    }
}

int16_t P_BoxOnLineSide(const fixed_t *tmbox, const line_t* ld)
{
    line_t l;
    W_Fetch(ld, &l, sizeof(l));
    return P_BoxOnLineCopySide(tmbox, &l);
}

//
// P_PointOnDivlineSide
// Returns 0 or 1.
//
// killough 5/3/98: reformatted, cleaned up

static int16_t PUREFUNC P_PointOnDivlineSide(fixed_t x, fixed_t y, const divline_t *line)
{
  return
    !line->dx ? x <= line->x ? line->dy > 0 : line->dy < 0 :
    !line->dy ? y <= line->y ? line->dx < 0 : line->dx > 0 :
    (line->dy^line->dx^(x -= line->x)^(y -= line->y)) < 0 ? (line->dy^x) < 0 :
    //FixedMul(y>>8, line->dx>>8) >= FixedMul(line->dy>>8, x>>8);
    (y >> 8) * (line->dx >> 16) >= (line->dy >> 16) * (x >> 8);
}

//
// P_MakeDivline
//

static void P_MakeDivline(const line_t* li, divline_t *dl)
{
  dl->x = (fixed_t)li->v1.x<<FRACBITS;
  dl->y = (fixed_t)li->v1.y<<FRACBITS;
  dl->dx = (fixed_t)li->dx<<FRACBITS;
  dl->dy = (fixed_t)li->dy<<FRACBITS;
}


static inline fixed_t CONSTFUNC FixedDiv(fixed_t a, fixed_t b)
{
	if (a < 0)
	{
		a = -a;
		b = -b;
	}

	uint16_t ibit = 1;
	while (b < a)
	{
		b    <<= 1;
		ibit <<= 1;
	}

	int16_t ch = 0;
	for (; ibit != 0; ibit >>= 1)
	{
		if (a >= b)
		{
			a  -= b;
			ch |= ibit;
		}
		a <<= 1;
	}

	uint16_t cl = 0;
	if (a >= b) {a -= b; cl |= 0x8000;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x4000;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x2000;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x1000;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0800;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0400;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0200;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0100;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0080;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0040;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0020;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0010;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0008;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0004;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0002;} a <<= 1;
	if (a >= b) {a -= b; cl |= 0x0001;}

	return (((fixed_t)ch) << FRACBITS) | cl;
}


//
// P_InterceptVector
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//

static fixed_t PUREFUNC P_InterceptVector3(const divline_t *v2, const divline_t *v1)
{
	fixed_t a = (v1->dy >> FRACBITS) * ((v1->x - v2->x) >> 8);
	fixed_t b = (v1->dx >> FRACBITS) * ((v2->y - v1->y) >> 8);
	fixed_t c = FixedMul(v2->dx, v1->dy >> 8);
	fixed_t d = FixedMul(v2->dy, v1->dx >> 8);

	fixed_t num = a + b;
	fixed_t den = c - d;

	if (num == 0 || den == 0)
		return 0;
	else if ((num ^ den) < 0)
		return -1;
	else
		return FixedDiv(num, den);
}

//
// P_LineOpening
// Sets opentop and openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//

sector_t* LN_BackSector(const line_t* l)
{
    const uint16_t side = LN_SIDENUM(l, 1);
    return side != NO_INDEX ? SIDE_SECTOR(side) : NULL;
}

void P_LineOpening(const line_t* linedef)
{
    // moved front and back outside P-LineOpening and changed
    // them to these so we can pick up the new friction value
    // in PIT_CheckLine()
    sector_t* openfrontsector;
    sector_t* openbacksector;

    if (LN_SIDENUM(linedef, 1) == NO_INDEX)      // single sided line
    {
        _g_openrange = 0;
        return;
    }

    openfrontsector = LN_FRONTSECTOR(linedef);
    openbacksector  = LN_BACKSECTOR(linedef);

    if (SEC_CEILINGHEIGHT(openfrontsector) < SEC_CEILINGHEIGHT(openbacksector))
        _g_opentop = SEC_CEILINGHEIGHT(openfrontsector);
    else
        _g_opentop = SEC_CEILINGHEIGHT(openbacksector);

    if (SEC_FLOORHEIGHT(openfrontsector) > SEC_FLOORHEIGHT(openbacksector))
    {
        _g_openbottom = SEC_FLOORHEIGHT(openfrontsector);
        _g_lowfloor = SEC_FLOORHEIGHT(openbacksector);
    }
    else
    {
        _g_openbottom = SEC_FLOORHEIGHT(openbacksector);
        _g_lowfloor = SEC_FLOORHEIGHT(openfrontsector);
    }
    _g_openrange = _g_opentop - _g_openbottom;
}

//
// THING POSITION SETTING
//

//
// Things the map spawns are not chained while they stay where the map put them: the chains
// then hold only what was linked since. The map WAD's E1MxTL lists, per sector and block,
// the map things in reverse load order, as the chains would have them after their later
// links, and the sectors each thing touched after loading. Memory keeps a bit per thing and
// per touch. A map static never moves and is never shot, so its state follows from its first
// tics and the time since loading, and its heights are its sector's until a moving sector
// changes them; the mobj_t code sees is rebuilt whenever it is visited.
//

typedef struct
{
  int16_t x, y;
  uint8_t type;
  uint8_t angle;  // in 45 degree steps, | 0x80 for MF_AMBUSH
  int16_t slot[3];  // per skill group, -1 if it does not spawn
  int16_t subsector;
} loadthing_t;

typedef char assertLoadthingSize[sizeof(loadthing_t) == 14 ? 1 : -1];

typedef struct
{
  uint16_t numthings, numsectors, numblocks, numnodes;
  uint16_t numstatics[3], numthinking[3];
  uint32_t things, sectorfirst, blockfirst, list, nodefirst, nodesector, nodething, secnodefirst, secnodes;
} loadheader_t;

#define STATICZCHUNK 4

typedef struct
{
  zref_t next;
  int16_t slot[STATICZCHUNK];
  fixed_t z[STATICZCHUNK], floorz[STATICZCHUNK], ceilingz[STATICZCHUNK];
} staticz_t;

static const byte* tl;
static const loadthing_t* tlthings;
static const int16_t* tlsectorfirst;
static const int16_t* tlblockfirst;
static const int16_t* tllist;
static const int16_t* tlnodefirst;
static const int16_t* tlnodesector;
static const int16_t* tlnodething;
static const int16_t* tlsecnodefirst;
static const int16_t* tlsecnodes;

static boolean loadingthings;
static int16_t loadingthing;
static int16_t skillgroup;
static int16_t numloadthings, numloadnodes, numloadstatics;
static uint16_t numloadsectors, numloadblocks;
static zref_t* loadrefs;
static int16_t numthinking;
static int16_t* slotthing;
static uint8_t* sleeprec;
static uint8_t* statictics;
static uint8_t* staticmoved;
static zref_t staticz;
static int16_t numstaticz;
static uint32_t staticclock;
static int16_t changesector = -1;
static fixed_t changefloor, changeceiling;
static uint8_t* inplace;
static uint8_t* nodealive;
static uint8_t* nodevisited;

#define BIT(bits, i)    ((bits)[(i) >> 3] & (1 << ((i) & 7)))
#define SETBIT(bits, i) ((bits)[(i) >> 3] |= (1 << ((i) & 7)))
#define CLRBIT(bits, i) ((bits)[(i) >> 3] &= ~(1 << ((i) & 7)))

static const mobj_t staticproxy = { .flags = MF_STATIC };

void P_BeginLoadThings(int16_t map)
{
  char name[9];
  sprintf(name, "E1M%cTL", '0' + map);
  tl = W_GetMapLumpByNum(W_GetMapNumForName(name));

  loadheader_t h;
  W_Fetch(tl, &h, sizeof(h));

  tlthings       = (const loadthing_t*)(tl + h.things);
  tlsectorfirst  = (const int16_t*)(tl + h.sectorfirst);
  tlblockfirst   = (const int16_t*)(tl + h.blockfirst);
  tllist         = (const int16_t*)(tl + h.list);
  tlnodefirst    = (const int16_t*)(tl + h.nodefirst);
  tlnodesector   = (const int16_t*)(tl + h.nodesector);
  tlnodething    = (const int16_t*)(tl + h.nodething);
  tlsecnodefirst = (const int16_t*)(tl + h.secnodefirst);
  tlsecnodes     = (const int16_t*)(tl + h.secnodes);

  if (_g_gameskill == sk_baby || _g_gameskill == sk_easy)
    skillgroup = 0;
  else if (_g_gameskill == sk_hard || _g_gameskill == sk_nightmare)
    skillgroup = 2;
  else
    skillgroup = 1;

  numloadthings  = h.numthings;
  numloadnodes   = h.numnodes;
  numloadsectors = h.numsectors;
  numloadblocks  = h.numblocks;
  numloadstatics = h.numstatics[skillgroup];
  numthinking = h.numthinking[skillgroup];
  loadrefs    = Z_CallocLevel(numthinking * sizeof(*loadrefs));
  slotthing   = Z_CallocLevel(numthinking * sizeof(*slotthing));
  sleeprec    = Z_CallocLevel(numthinking);
  for (int16_t t = 0; t < h.numthings; t++)
  {
    loadthing_t lt;
    W_Fetch(&tlthings[t], &lt, sizeof(lt));
    if (lt.slot[skillgroup] >= 0 && lt.type < MT_MISC0)
      slotthing[lt.slot[skillgroup]] = t;
  }
  statictics  = Z_CallocLevel((numloadstatics + 1) / 2);
  staticmoved = Z_CallocLevel((numloadstatics + 7) / 8);
  staticz     = 0;
  numstaticz  = 0;
  staticclock = 0;
  inplace     = Z_CallocLevel((numloadthings + 7) / 8);
  nodealive   = Z_CallocLevel((numloadnodes + 7) / 8);
  nodevisited = Z_CallocLevel((numloadnodes + 7) / 8);
  loadingthings = true;
}

void P_SetLoadingThing(int16_t thing)
{
  loadingthing = thing;
}

void P_EndLoadThings(void)
{
  if (numloadsectors != _g_numsectors || numloadblocks != _g_bmapwidth * _g_bmapheight)
    I_Error("P_EndLoadThings: E1MxTL built for another map");
  loadingthings = false;
}

static int16_t P_LoadSlot(int16_t thing)
{
  int16_t slot = W_FetchInt16(&tlthings[thing].slot[skillgroup]);
  if (slot < 0)
    I_Error("P_LoadSlot: map thing %i does not spawn", thing);
  return slot;
}

// The part of P_SetThingPosition loading does.
static void P_PlaceLoadThing(mobj_t* thing)
{
  const int16_t t = loadingthing;
  thing->flags |= MF_INPLACE;
  thing->sprev = t;
  SETBIT(inplace, t);
  if (!(thing->flags & MF_STATIC))
  {
    P_Ext(thing)->loadthing = t + 1;
    loadrefs[P_LoadSlot(t)] = Z_Ref(thing);
  }
  if (!(thing->flags & MF_NOSECTOR))
  {
    const int16_t last = W_FetchInt16(&tlnodefirst[t + 1]);
    for (int16_t k = W_FetchInt16(&tlnodefirst[t]); k < last; k++)
      SETBIT(nodealive, k);
    P_MarkSecNodeLines(thing);
  }
}

boolean P_SpawnLoadStatic(fixed_t x, fixed_t y, mobjtype_t type)
{
  if (!loadingthings)
    return false;

  // what P_SpawnMobj and P_SpawnMapThing do for it
  const mobjinfo_t* info = &mobjinfo[type];
  mobj_t mobj;
  memset(&mobj, 0, sizeof(mobj));
  mobj.type   = type;
  mobj.x      = x;
  mobj.y      = y;
  mobj.radius = info->radius;
  mobj.height = info->height;
  mobj.flags  = info->flags | MF_STATIC;
  if (mobj.flags & (MF_SHOOTABLE | MF_MISSILE | MF_COUNTKILL))
    I_Error("P_SpawnLoadStatic: static thing %i acts", type);

  P_Random();

  mobj.state = info->spawnstate;
  mobj.tics  = states[info->spawnstate].tics;

  P_SetThingPosition(&mobj);
  if (mobj.subsector != W_FetchInt16(&tlthings[loadingthing].subsector))
    I_Error("P_SpawnLoadStatic: map thing %i in another subsector", loadingthing);

  if (mobj.tics > 0)
  {
    const int16_t slot = P_LoadSlot(loadingthing);
    const int16_t tics = 1 + (P_Random() % mobj.tics);
    if (tics > 15)
      I_Error("P_SpawnLoadStatic: static thing %i lasts %i tics", type, tics);
    statictics[slot >> 1] |= tics << ((slot & 1) * 4);
  }

  if (mobj.flags & MF_COUNTITEM)
    _g_totalitems++;
  return true;
}

static int16_t P_NextStaticState(int16_t state)
{
  do
    state = states[state].nextstate;
  while (!states[state].tics);
  return state;
}

// Where P_CycleState, once per P_RunLoadStatics, has taken a map static by now; none of their
// states has an action or ends them.
static void P_StaticPhase(int16_t slot, mobj_t* mobj)
{
  int16_t state = mobj->state;
  int16_t tics  = states[state].tics;
  uint32_t n = 0;
  if (tics > 0)
    tics = (statictics[slot >> 1] >> ((slot & 1) * 4)) & 15, n = staticclock;
  if (tics >= 0 && n >= (uint32_t)tics)
  {
    n -= tics;
    state = P_NextStaticState(state);

    uint32_t period = 0;
    int16_t s = state;
    for (int16_t k = 0; ; k++)
    {
      if (k == 16)
        I_Error("P_StaticPhase: state %i does not cycle", state);
      if (states[s].tics < 0)
      {
        period = 0;
        break;
      }
      period += states[s].tics;
      s = P_NextStaticState(s);
      if (s == state)
        break;
    }
    if (period)
      n %= period;

    while ((tics = states[state].tics) >= 0 && n >= (uint32_t)tics)
    {
      n -= tics;
      state = P_NextStaticState(state);
    }
  }
  mobj->state  = state;
  mobj->tics   = tics < 0 ? -1 : (int16_t)(tics - n);
  mobj->frame  = states[state].frame;
  mobj->sprite = states[state].sprite;
}

// A sector that moves has already taken its new heights when P_CheckSector adjusts the things
// touching it, so until then its statics stand at the old ones.
static boolean P_OwnNodeVisited(int16_t t, int16_t sector)
{
  const int16_t last = W_FetchInt16(&tlnodefirst[t + 1]);
  for (int16_t k = W_FetchInt16(&tlnodefirst[t]); k < last; k++)
    if (W_FetchInt16(&tlnodesector[k]) == sector)
      return BIT(nodevisited, k) != 0;
  return true;
}

// The heights of a map thing that no moving sector changed yet.
static void P_LoadThingHeights(int16_t t, int16_t subsector, mobj_t* mobj)
{
  if (P_SubsectorSector(subsector) - _g_sectors == changesector && !P_OwnNodeVisited(t, changesector))
  {
    mobj->z        = changefloor;
    mobj->floorz   = changefloor;
    mobj->ceilingz = changeceiling;
  }
  else
  {
    const sector_t* sector = P_SubsectorSector(subsector);
    mobj->z        = SEC_FLOORHEIGHT(sector);
    mobj->floorz   = SEC_FLOORHEIGHT(sector);
    mobj->ceilingz = SEC_CEILINGHEIGHT(sector);
  }
}

static staticz_t* P_StaticZ(int16_t slot, int16_t* i)
{
  for (staticz_t* c = Z_Ptr(staticz); c; c = Z_Ptr(c->next))
    for (*i = 0; *i < STATICZCHUNK; (*i)++)
      if (c->slot[*i] == slot)
        return c;
  return NULL;
}

static void P_StaticMobj(int16_t t, int16_t slot, const loadthing_t* lt, mobj_t* mobj)
{
  const mobjinfo_t* info = &mobjinfo[lt->type];
  memset(mobj, 0, sizeof(*mobj));
  mobj->x         = (fixed_t)lt->x << FRACBITS;
  mobj->y         = (fixed_t)lt->y << FRACBITS;
  mobj->angle     = ANG45 * (lt->angle & 0x7f);
  mobj->sprev     = t;
  mobj->subsector = lt->subsector;
  if (BIT(staticmoved, slot))
  {
    int16_t i;
    const staticz_t* c = P_StaticZ(slot, &i);
    mobj->z        = c->z[i];
    mobj->floorz   = c->floorz[i];
    mobj->ceilingz = c->ceilingz[i];
  }
  else
    P_LoadThingHeights(t, lt->subsector, mobj);
  mobj->radius    = info->radius;
  mobj->height    = info->height;
  mobj->flags     = info->flags | MF_STATIC | MF_INPLACE | (lt->angle & 0x80 ? MF_AMBUSH : 0);
  mobj->state     = info->spawnstate;
  mobj->type      = lt->type;
  P_StaticPhase(slot, mobj);
}

// Heights a visit changed to other than the sector's, which only a moving sector does, are kept.
static void P_KeepStaticZ(int16_t slot, const mobj_t* mobj, const fixed_t* old)
{
  int16_t i;
  staticz_t* c;
  if (BIT(staticmoved, slot))
    c = P_StaticZ(slot, &i);
  else
  {
    if (mobj->z == old[0] && mobj->floorz == old[1] && mobj->ceilingz == old[2])
      return;

    const sector_t* sector = P_SubsectorSector(mobj->subsector);
    if (mobj->z == SEC_FLOORHEIGHT(sector) && mobj->floorz == SEC_FLOORHEIGHT(sector)
        && mobj->ceilingz == SEC_CEILINGHEIGHT(sector))
      return;

    i = numstaticz % STATICZCHUNK;
    if (!i)
    {
      c = Z_CallocLevel(sizeof(*c));
      c->next = staticz;
      staticz = Z_Ref(c);
      for (int16_t k = 0; k < STATICZCHUNK; k++)
        c->slot[k] = -1;
    }
    else
      c = Z_Ptr(staticz);
    numstaticz++;
    c->slot[i] = slot;
    SETBIT(staticmoved, slot);
  }
  c->z[i]        = mobj->z;
  c->floorz[i]   = mobj->floorz;
  c->ceilingz[i] = mobj->ceilingz;
}

//
// A map monster or barrel that has not changed since it spawned sleeps: it has no mobj, only
// the state it reached and its tics in sleeprec, and P_RunThinkers counts its tics down among
// the map things in load order. On the tic its state changes, and whenever something visits
// it, it gets a mobj again, and keeps it if that no longer equals its sleeping form.
//

static boolean P_Asleep(int16_t slot)
{
  return !loadrefs[slot] && BIT(inplace, slotthing[slot]);
}

// What P_SpawnMobj, P_SpawnMapThing and the time since have made of a sleeping map thing.
static void P_SleeperMobj(int16_t slot, uint8_t rec, mobjext_t* ext)
{
  const int16_t t = slotthing[slot];
  loadthing_t lt;
  W_Fetch(&tlthings[t], &lt, sizeof(lt));
  const mobjinfo_t* info = &mobjinfo[lt.type];
  const uint32_t pooled = ext->mo.flags & MF_POOLED;
  memset(ext, 0, sizeof(*ext));
  mobj_t* mobj = &ext->mo;
  mobj->x         = (fixed_t)lt.x << FRACBITS;
  mobj->y         = (fixed_t)lt.y << FRACBITS;
  mobj->angle     = ANG45 * (lt.angle & 0x7f);
  mobj->sprev     = t;
  mobj->subsector = lt.subsector;
  P_LoadThingHeights(t, lt.subsector, mobj);
  mobj->radius    = info->radius;
  mobj->height    = info->height;
  mobj->flags     = info->flags | pooled | MF_INPLACE | (lt.angle & 0x80 ? MF_AMBUSH : 0);
  mobj->state     = info->spawnstate + (rec >> 4);
  mobj->tics      = rec & 15;
  mobj->sprite    = states[mobj->state].sprite;
  mobj->frame     = states[mobj->state].frame;
  mobj->type      = lt.type;
  ext->dropoffz     = mobj->floorz;
  ext->health       = info->spawnhealth;
  ext->reactiontime = _g_gameskill != sk_nightmare ? info->reactiontime : 0;
  ext->loadthing    = t + 1;
}

// Whether a map thing's mobj equals its sleeping form, which rec then holds.
static boolean P_SleepForm(int16_t slot, const mobjext_t* ext, uint8_t* rec)
{
  const mobj_t* mobj = &ext->mo;
  if (mobj->type == MT_PLAYER || mobj->type == MT_TELEPORTMAN || ext->thinker.function != P_MobjThinker
      || _g_linetarget == mobj)
    return false;
  const int16_t offset = mobj->state - mobjinfo[mobj->type].spawnstate;
  if (offset < 0 || offset > 15 || mobj->tics < 1 || mobj->tics > 15)
    return false;
  *rec = offset << 4 | mobj->tics;
  mobjext_t form;
  form.mo.flags = mobj->flags & MF_POOLED;
  P_SleeperMobj(slot, *rec, &form);
  return !memcmp((const uint8_t*)&form + sizeof(thinker_t), (const uint8_t*)ext + sizeof(thinker_t),
                   sizeof(form) - sizeof(thinker_t));
}

static void P_Sleep(int16_t slot, mobjext_t* ext, uint8_t rec)
{
  sleeprec[slot] = rec;
  loadrefs[slot] = 0;
  if (ext->mo.flags & MF_POOLED)
    ext->mo.type = MT_NOTHING;
  else
    Z_Free(ext);
}

static mobjext_t* P_WakeMobj(int16_t slot)
{
  mobjext_t* ext = P_Ext(P_NewMobj(MT_POSSESSED));
  P_SleeperMobj(slot, sleeprec[slot], ext);
  ext->thinker.function = P_MobjThinker;
  loadrefs[slot] = Z_Ref(&ext->mo);
  return ext;
}

static void P_SettleMobj(int16_t slot, mobjext_t* ext)
{
  uint8_t rec;
  if (BIT(inplace, slotthing[slot]) && P_SleepForm(slot, ext, &rec))
    P_Sleep(slot, ext, rec);
  else
    P_AddLoadThinker(&ext->thinker, slotthing[slot]);
}

// Runs the sleeping map things before map thing t and skips t, or stops after one whose turn
// woke a later one before t.
int16_t P_RunSleepers(int16_t slot, int16_t t)
{
  thinker_t* const next = _g_runnext;
  for (; slot < numthinking && slotthing[slot] <= t; slot++)
    if (slotthing[slot] < t && P_Asleep(slot))
    {
      const uint8_t rec = sleeprec[slot];
      if ((rec & 15) > 1)
        sleeprec[slot] = rec - 1;
      else
      {
        _g_runlast = slotthing[slot];
        mobjext_t* ext = P_WakeMobj(slot);
        P_MobjThinker(&ext->thinker);
        P_SettleMobj(slot, ext);
        if (_g_runnext != next)
          return slot + 1;
      }
    }
  return slot;
}

void P_SleepIfUnchanged(mobjext_t* ext)
{
  const mobj_t* mobj = &ext->mo;
  if (!(mobj->flags & MF_INPLACE) || !ext->loadthing || mobj->type == MT_PLAYER || ext->target
      || ext->health != mobjinfo[mobj->type].spawnhealth)
    return;
  const int16_t slot = P_LoadSlot(ext->loadthing - 1);
  uint8_t rec;
  if (P_SleepForm(slot, ext, &rec))
  {
    P_UnlinkThinker(&ext->thinker);
    P_Sleep(slot, ext, rec);
  }
}

void P_SleepAtLoad(mobj_t* mobj)
{
  if (!loadingthings || (mobj->flags & MF_STATIC))
    return;
  mobjext_t* ext = P_Ext(mobj);
  const int16_t slot = P_LoadSlot(loadingthing);
  uint8_t rec;
  if (P_SleepForm(slot, ext, &rec))
  {
    P_UnlinkThinker(&ext->thinker);
    P_Sleep(slot, ext, rec);
  }
}

// A map thing's mobj now lives at mobj.
void P_MoveLoadRef(int16_t loadthing, mobj_t* mobj)
{
  if (loadthing)
    loadrefs[P_LoadSlot(loadthing - 1)] = Z_Ref(mobj);
}

boolean P_SleeperOfType(mobjtype_t type)
{
  for (int16_t slot = 0; slot < numthinking; slot++)
    if (P_Asleep(slot) && (mobjtype_t)W_FetchUInt8(&tlthings[slotthing[slot]].type) == type)
      return true;
  return false;
}

static boolean P_VisitLoadThing(int16_t t, boolean func(mobj_t*))
{
  loadthing_t lt;
  W_Fetch(&tlthings[t], &lt, sizeof(lt));
  const int16_t slot = lt.slot[skillgroup];
  if (lt.type < MT_MISC0)
  {
    if (loadrefs[slot])
      return func(P_Mobj(loadrefs[slot]));
    mobjext_t* ext = P_WakeMobj(slot);
    const boolean r = func(&ext->mo);
    P_SettleMobj(slot, ext);
    return r;
  }

  mobj_t mobj;
  P_StaticMobj(t, slot, &lt, &mobj);
  const fixed_t old[3] = { mobj.z, mobj.floorz, mobj.ceilingz };
  boolean r = func(&mobj);
  if (BIT(inplace, t))
    P_KeepStaticZ(slot, &mobj, old);
  return r;
}

void P_RunLoadStatics(void)
{
  staticclock++;
}

static void P_SetInterceptThing(intercept_t* in, mobj_t* thing)
{
  in->isaline = false;
  if ((thing->flags & (MF_STATIC | MF_INPLACE)) == (MF_STATIC | MF_INPLACE))
    in->d.thing = (mobj_t*)&staticproxy;
  else if ((thing->flags & MF_INPLACE) && !P_Ext(thing)->thinker.next)
  {
    in->isaline     = INTERCEPT_SLEEPER;
    in->d.loadthing = P_Ext(thing)->loadthing - 1;
  }
  else
    in->d.thing = thing;
}

static boolean P_TraverseSleeper(intercept_t* in, traverser_t func)
{
  const int16_t slot = P_LoadSlot(in->d.loadthing);
  in->isaline = false;
  if (loadrefs[slot])
  {
    in->d.thing = P_Mobj(loadrefs[slot]);
    return func(in);
  }
  mobjext_t* ext = P_WakeMobj(slot);
  in->d.thing = &ext->mo;
  const boolean r = func(in);
  P_SettleMobj(slot, ext);
  return r;
}

int16_t P_LoadIndex(mobj_t* thing)
{
  if (loadingthings)
    return -1;
  if (thing->flags & MF_STATIC)
    return thing->flags & MF_INPLACE ? (int16_t)thing->sprev : -1;
  return P_Ext(thing)->loadthing - 1;
}

int16_t P_LoadNodeFirst(int16_t i)
{
  return W_FetchInt16(&tlnodefirst[i]);
}

boolean P_LoadNodeAlive(int16_t k)
{
  return BIT(nodealive, k) != 0;
}

int16_t P_LoadNodeSector(int16_t k)
{
  return W_FetchInt16(&tlnodesector[k]);
}

void P_KillLoadNode(int16_t k)
{
  CLRBIT(nodealive, k);
}

void P_KillLoadNodes(mobj_t* thing)
{
  int16_t i = P_LoadIndex(thing);
  if (i >= 0)
  {
    const int16_t last = W_FetchInt16(&tlnodefirst[i + 1]);
    for (int16_t k = W_FetchInt16(&tlnodefirst[i]); k < last; k++)
      CLRBIT(nodealive, k);
  }
}

void P_BeginLoadNodeChange(int16_t sector, fixed_t oldfloor, fixed_t oldceiling)
{
  const int16_t last = W_FetchInt16(&tlsecnodefirst[sector + 1]);
  for (int16_t j = W_FetchInt16(&tlsecnodefirst[sector]); j < last; j++)
    CLRBIT(nodevisited, W_FetchInt16(&tlsecnodes[j]));
  changesector  = sector;
  changefloor   = oldfloor;
  changeceiling = oldceiling;
}

void P_EndLoadNodeChange(void)
{
  changesector = -1;
}

static void (*changefunc)(mobj_t*);

static boolean P_ChangeThing(mobj_t* thing)
{
  if (!(thing->flags & MF_NOBLOCKMAP))
    changefunc(thing);
  return true;
}

boolean P_ChangeLoadNodeThing(int16_t sector, void func(mobj_t*))
{
  const int16_t last = W_FetchInt16(&tlsecnodefirst[sector + 1]);
  for (int16_t j = W_FetchInt16(&tlsecnodefirst[sector]); j < last; j++)
  {
    int16_t k = W_FetchInt16(&tlsecnodes[j]);
    if (BIT(nodealive, k) && !BIT(nodevisited, k))
    {
      SETBIT(nodevisited, k);
      void (*saved)(mobj_t*) = changefunc;
      changefunc = func;
      P_VisitLoadThing(W_FetchInt16(&tlnodething[k]), P_ChangeThing);
      changefunc = saved;
      return true;
    }
  }
  return false;
}

static boolean P_LoadListThings(const int16_t* first, boolean func(mobj_t*))
{
  int16_t range[2];
  W_Fetch(first, range, sizeof(range));
  for (int16_t k = range[0]; k < range[1]; k++)
  {
    int16_t t = W_FetchInt16(&tllist[k]);
    if (BIT(inplace, t))
      if (!P_VisitLoadThing(t, func))
        return false;
  }
  return true;
}

boolean P_SectorLoadThings(int16_t sector, boolean func(mobj_t*))
{
  return P_LoadListThings(&tlsectorfirst[sector], func);
}

//
// P_UnsetThingPosition
// Unlinks a thing from block map and sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//

void P_UnsetThingPosition(mobj_t* thing)
{
  if (thing->flags & MF_INPLACE)
    {
      CLRBIT(inplace, thing->sprev);
      thing->flags &= ~MF_INPLACE;
      if (!(thing->flags & MF_NOSECTOR))
        {
          P_SetSeclist(P_Node(thing->touching_sectorlist));
          thing->touching_sectorlist = 0;
        }
      return;
    }

  if (!(thing->flags & MF_NOSECTOR))
    {
      /* invisible things don't need to be in sector list
       * unlink from subsector
       *
       * killough 8/11/98: simpler scheme using pointers-to-pointers for prev
       * pointers, allows head node pointers to be treated like everything else
       */

      zref_t* sprev = Z_Ptr(thing->sprev);
      zref_t snext = thing->snext;
      if ((*sprev = snext))  // unlink from sector list
        P_Mobj(snext)->sprev = thing->sprev;

        // phares 3/14/98
        //
        // Save the sector list pointed to by touching_sectorlist.
        // In P_SetThingPosition, we'll keep any nodes that represent
        // sectors the Thing still touches. We'll add new ones then, and
        // delete any nodes for sectors the Thing has vacated. Then we'll
        // put it back into touching_sectorlist. It's done this way to
        // avoid a lot of deleting/creating for nodes, when most of the
        // time you just get back what you deleted anyway.
        //
        // If this Thing is being removed entirely, then the calling
        // routine will clear out the nodes in sector_list.

      P_SetSeclist(P_Node(thing->touching_sectorlist));
      thing->touching_sectorlist = 0; //to be restored by P_SetThingPosition
    }

  if (!(thing->flags & MF_NOBLOCKMAP))
    {
      /* inert things don't need to be in blockmap
       *
       * killough 8/11/98: simpler scheme using pointers-to-pointers for prev
       * pointers, allows head node pointers to be treated like everything else
       *
       * Also more robust, since it doesn't depend on current position for
       * unlinking. Old method required computing head node based on position
       * at time of unlinking, assuming it was the same position as during
       * linking.
       */

      zref_t bnext;
      zref_t* bprev = Z_Ptr(thing->bprev);
      if (bprev && (*bprev = bnext = thing->bnext))  // unlink from block map
        P_Mobj(bnext)->bprev = thing->bprev;
    }
}

//
// P_SetThingPosition
// Links a thing into both a block and a subsector
// based on it's x y.
// Sets thing->subsector properly
//

void P_SetThingPosition(mobj_t* thing)
{                                                      // link into subsector
  int16_t ss = R_PointInSubsector(thing->x, thing->y);
  thing->subsector = ss;
  if (loadingthings)
    {
      P_PlaceLoadThing(thing);
      return;
    }
  if (!(thing->flags & MF_NOSECTOR))
    {
      // invisible things don't go into the sector links

      // killough 8/11/98: simpler scheme using pointer-to-pointer prev
      // pointers, allows head nodes to be treated like everything else

      zref_t* link = &SEC_THINGS(P_SubsectorSector(ss));
      zref_t snext = *link;
      if ((thing->snext = snext))
        P_Mobj(snext)->sprev = Z_Ref(&thing->snext);
      thing->sprev = Z_Ref(link);
      *link = Z_Ref(thing);

      // phares 3/16/98
      //
      // If sector_list isn't NULL, it has a collection of sector
      // nodes that were just removed from this Thing.

      // Collect the sectors the object will live in by looking at
      // the existing sector_list and adding new nodes and deleting
      // obsolete ones.

      // When a node is deleted, its sector links (the links starting
      // at sector_t->touching_thinglist) are broken. When a node is
      // added, new sector links are created.

      P_CreateSecNodeList(thing);
    }

  // link into blockmap
  if (!(thing->flags & MF_NOBLOCKMAP))
    {
      // inert things don't need to be in blockmap
      int16_t blockx = (thing->x - _g_bmaporgx)>>MAPBLOCKSHIFT;
      int16_t blocky = (thing->y - _g_bmaporgy)>>MAPBLOCKSHIFT;
      if (0 <= blockx && blockx < _g_bmapwidth && 0 <= blocky && blocky < _g_bmapheight)
        {
        // killough 8/11/98: simpler scheme using pointer-to-pointer prev
        // pointers, allows head nodes to be treated like everything else

        zref_t* link = &_g_blocklinks[(blocky*_g_bmapwidth+blockx) & (BLOCKBUCKETS - 1)];
        zref_t bnext = *link;
        if ((thing->bnext = bnext))
          P_Mobj(bnext)->bprev = Z_Ref(&thing->bnext);
        thing->bprev = Z_Ref(link);
        *link = Z_Ref(thing);
      }
      else        // thing is off the map
        thing->bnext = 0, thing->bprev = 0;
    }
}

//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//

//
// P_BlockLinesIterator
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
// killough 5/3/98: reformatted, cleaned up

boolean P_BlockLinesIterator(int16_t x, int16_t y, boolean func(const line_t*))
{

    if (!(0 <= x && x < _g_bmapwidth && 0 <= y && y <_g_bmapheight))
        return true;

    const int16_t offset = W_FetchInt16(&_g_blockmap[y*_g_bmapwidth+x]);
    const int16_t* list = _g_blockmaplump+offset;     // original was reading         // phares


    // delmiting 0 as linedef 0     // phares

    // killough 1/31/98: for compatibility we need to use the old method.
    // Most demos go out of sync, and maybe other problems happen, if we
    // don't consider linedef 0. For safety this should be qualified.

    list++;     // skip 0 starting delimiter                      // phares

    for (int16_t lineno; (lineno = W_FetchInt16(list)) != -1 ; list++)  // phares
    {
        const line_t *mld = &_g_maplines[lineno];

        if (LN_BIT(LB_CHECKED, lineno))
            continue;       // line has already been checked

        LN_SETBIT(LB_CHECKED, lineno);

        if (!func(mld))
            return false;
    }

    return true;  // everything was checked
}

//
// P_BlockThingsIterator
// Blocks share chains, each still in the order of its block's links. A linked thing only
// moves between unlinking and linking again, so its position tells its block.
//

static int16_t P_ThingBlock(const mobj_t* thing)
{
  int16_t blockx = (thing->x - _g_bmaporgx)>>MAPBLOCKSHIFT;
  int16_t blocky = (thing->y - _g_bmaporgy)>>MAPBLOCKSHIFT;
  return blocky*_g_bmapwidth+blockx;
}

boolean P_BlockThingsIterator(int16_t x, int16_t y, boolean func(mobj_t*))
{
  mobj_t* mobj;
  if (0 <= x && x < _g_bmapwidth && 0 <= y && y < _g_bmapheight)
    {
    const int16_t b = y*_g_bmapwidth+x;
    for (mobj = P_Mobj(_g_blocklinks[b & (BLOCKBUCKETS - 1)]); mobj; mobj = P_Mobj(mobj->bnext))
      if (P_ThingBlock(mobj) == b && !func(mobj))
        return false;
    return P_LoadListThings(&tlblockfirst[b], func);
    }
  return true;
}

//
// INTERCEPT ROUTINES
//

// Check for limit and double size if necessary -- killough
static boolean check_intercept(void)
{
    size_t offset = intercept_p - intercepts;

    return (offset < MAX_INTERCEPTS);
}


// PIT_AddLineIntercepts.
// Looks for lines in the given block
// that intercept the given trace
// to add to the intercepts list.
//
// A line is crossed if its endpoints
// are on opposite sides of the trace.
//
// killough 5/3/98: reformatted, cleaned up

static boolean PIT_AddLineIntercepts(const line_t* ld)
{
  int16_t       s1;
  int16_t       s2;
  fixed_t   frac;
  divline_t dl;
  line_t l;
  W_Fetch(ld, &l, sizeof(l));

  // avoid precision problems with two routines
  if (_g_trace.dx >  FRACUNIT*16 || _g_trace.dy >  FRACUNIT*16 ||
      _g_trace.dx < -FRACUNIT*16 || _g_trace.dy < -FRACUNIT*16)
    {
      s1 = P_PointOnDivlineSide ((fixed_t)l.v1.x<<FRACBITS, (fixed_t)l.v1.y<<FRACBITS, &_g_trace);
      s2 = P_PointOnDivlineSide ((fixed_t)l.v2.x<<FRACBITS, (fixed_t)l.v2.y<<FRACBITS, &_g_trace);
    }
  else
    {
      s1 = P_PointOnLineCopySide (_g_trace.x, _g_trace.y, &l);
      s2 = P_PointOnLineCopySide (_g_trace.x+_g_trace.dx, _g_trace.y+_g_trace.dy, &l);
    }

  if (s1 == s2)
    return true;        // line isn't crossed

  // hit the line
  P_MakeDivline(&l, &dl);
  frac = P_InterceptVector3(&_g_trace, &dl);

  if (frac < 0)
    return true;        // behind source

  if(!check_intercept())
    return false;

  intercept_p->frac = frac;
  intercept_p->isaline = true;
  intercept_p->d.line = ld;
  intercept_p++;

  return true;  // continue
}

//
// PIT_AddThingIntercepts
//
// killough 5/3/98: reformatted, cleaned up

static boolean PIT_AddThingIntercepts(mobj_t* thing)
{
  fixed_t   x1, y1;
  fixed_t   x2, y2;
  int16_t       s1, s2;
  divline_t dl;
  fixed_t   frac;

  // check a corner to corner crossection for hit
  if ((_g_trace.dx ^ _g_trace.dy) > 0)
    {
      x1 = thing->x - thing->radius;
      y1 = thing->y + thing->radius;
      x2 = thing->x + thing->radius;
      y2 = thing->y - thing->radius;
    }
  else
    {
      x1 = thing->x - thing->radius;
      y1 = thing->y - thing->radius;
      x2 = thing->x + thing->radius;
      y2 = thing->y + thing->radius;
    }

  s1 = P_PointOnDivlineSide (x1, y1, &_g_trace);
  s2 = P_PointOnDivlineSide (x2, y2, &_g_trace);

  if (s1 == s2)
    return true;                // line isn't crossed

  dl.x = x1;
  dl.y = y1;
  dl.dx = x2-x1;
  dl.dy = y2-y1;

  frac = P_InterceptVector3(&_g_trace, &dl);

  if (frac < 0)
    return true;                // behind source

  if(!check_intercept())
      return false;

  intercept_p->frac = frac;
  P_SetInterceptThing(intercept_p, thing);
  intercept_p++;

  return true;          // keep going
}

//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all lines.
//

static boolean P_TraverseIntercepts(traverser_t func)
{
  intercept_t *in = NULL;
  int16_t count = intercept_p - intercepts;
  while (count--)
    {
      fixed_t dist = INT32_MAX;
      intercept_t *scan;
      for (scan = intercepts; scan < intercept_p; scan++)
        if (scan->frac < dist)
          dist = (in=scan)->frac;
      if (dist > FRACUNIT)
        return true;    // checked everything in range
      if (!(in->isaline == INTERCEPT_SLEEPER ? P_TraverseSleeper(in, func) : func(in)))
        return false;           // don't bother going farther
      in->frac = INT32_MAX;
    }
  return true;                  // everything was traversed
}

//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all lines.
//

boolean P_PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2,
                       int16_t flags, boolean trav(intercept_t *))
{
  int16_t xt1, yt1;
  int16_t xt2, yt2;
  fixed_t xstep, ystep;
  fixed_t xintercept, yintercept;
  int16_t     mapx, mapy;
  int16_t     mapxstep, mapystep;
  int16_t     count;

  P_NextValidCount();
  intercept_p = intercepts;

  if (!((x1-_g_bmaporgx)&(MAPBLOCKSIZE-1)))
    x1 += FRACUNIT;     // don't side exactly on a line

  if (!((y1-_g_bmaporgy)&(MAPBLOCKSIZE-1)))
    y1 += FRACUNIT;     // don't side exactly on a line

  _g_trace.x = x1;
  _g_trace.y = y1;
  _g_trace.dx = x2 - x1;
  _g_trace.dy = y2 - y1;

  x1 -= _g_bmaporgx;
  y1 -= _g_bmaporgy;
  xt1 = x1>>MAPBLOCKSHIFT;
  yt1 = y1>>MAPBLOCKSHIFT;

  x2 -= _g_bmaporgx;
  y2 -= _g_bmaporgy;
  xt2 = x2>>MAPBLOCKSHIFT;
  yt2 = y2>>MAPBLOCKSHIFT;

  if (xt2 > xt1)
  {
    mapxstep = 1;
    fixed_t partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
    ystep = FixedApproxDiv (y2-y1,x2-x1);
    yintercept = (y1>>MAPBTOFRAC) + FixedMul3216(ystep, partial);
  }
  else if (xt2 < xt1)
  {
    mapxstep = -1;
    fixed_t partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
    ystep = FixedApproxDiv (y2-y1,x1-x2);
    yintercept = (y1>>MAPBTOFRAC) + FixedMul3216(ystep, partial);
  }
  else // xt2 == xt1
  {
    mapxstep = 0;
    ystep = 256*FRACUNIT;
    yintercept = (y1>>MAPBTOFRAC) + ystep;
  }


  if (yt2 > yt1)
  {
    mapystep = 1;
    fixed_t partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
    xstep = FixedApproxDiv (x2-x1,y2-y1);
    xintercept = (x1>>MAPBTOFRAC) + FixedMul3216(xstep, partial);
  }
  else if (yt2 < yt1)
  {
    mapystep = -1;
    fixed_t partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
    xstep = FixedApproxDiv (x2-x1,y1-y2);
    xintercept = (x1>>MAPBTOFRAC) + FixedMul3216(xstep, partial);
  }
  else // yt2 == yt1
  {
    mapystep = 0;
    xstep = 256*FRACUNIT;
    xintercept = (x1>>MAPBTOFRAC) + xstep;
  }


  // Step through map blocks.
  // Count is present to prevent a round off error
  // from skipping the break.

  mapx = xt1;
  mapy = yt1;

  for (count = 0; count < 64; count++)
    {
      if (flags & PT_ADDLINES)
        if (!P_BlockLinesIterator(mapx, mapy,PIT_AddLineIntercepts))
          return false; // early out

      if (flags & PT_ADDTHINGS)
        if (!P_BlockThingsIterator(mapx, mapy,PIT_AddThingIntercepts))
          return false; // early out

      if (mapx == xt2 && mapy == yt2)
        break;

      if ((yintercept >> FRACBITS) == mapy)
        {
          yintercept += ystep;
          mapx += mapxstep;
        }
      else
        if ((xintercept >> FRACBITS) == mapx)
          {
            xintercept += xstep;
            mapy += mapystep;
          }
    }

  // go through the sorted list
  return P_TraverseIntercepts(trav);
}
