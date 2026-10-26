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
 *      LineOfSight/Visibility checks, uses REJECT Lookup Table.
 *
 *-----------------------------------------------------------------------------*/

#include "d_player.h"
#include "r_main.h"
#include "p_map.h"
#include "p_maputl.h"
#include "p_setup.h"
#include "w_wad.h"
#include "i_platform.h"

#include "globdata.h"


typedef struct {
  fixed_t sightzstart, t2x, t2y;   // eye z of looker
  divline_t strace;                // from t1 to t2
  fixed_t topslope, bottomslope;   // slopes to top and bottom of target
  fixed_t bbox[4];
  fixed_t maxz,minz;               // cph - z optimisations for 2sided lines
} los_t;


static los_t los;


//
// P_DivlineSide
// Returns side 0 (front), 1 (back), or 2 (on).
//

static int8_t P_DivlineSide(fixed_t x, fixed_t y, const divline_t *node)
{
  fixed_t left, right;
  return
    !node->dx ? x == node->x ? 2 : x <= node->x ? node->dy > 0 : node->dy < 0 :
    !node->dy ? y == node->y ? 2 : y <= node->y ? node->dx < 0 : node->dx > 0 :
    (right = ((y - node->y) >> FRACBITS) * (node->dx >> FRACBITS)) <
    (left  = ((x - node->x) >> FRACBITS) * (node->dy >> FRACBITS)) ? 0 :
    right == left ? 2 : 1;
}


static uint16_t PUREFUNC P_InterceptVector2(const divline_t *v2, const divline_t *v1)
{
	fixed_t a = (v1->dy >> FRACBITS) * ((v1->x - v2->x) >> 8);
	fixed_t b = (v1->dx >> FRACBITS) * ((v2->y - v1->y) >> 8);

	fixed_t num = a + b;

	if (num == 0)
		return 0;

	fixed_t c = FixedMul(v2->dx, v1->dy >> 8);
	fixed_t d = FixedMul(v2->dy, v1->dx >> 8);

	fixed_t den = c - d;

	if (den == 0 || (den >> 12) == 0)
		return 0;

	fixed_t r = (num << 4) / (den >> 12);
	return (uint32_t)r <= 0xffffu ? r : 0xffffu;
}


//
// P_CrossSubsector
// Returns true
//  if strace crosses the given subsector successfully.
//

static boolean P_CrossSubsector(int16_t num)
{
    mapsubsector_t mapsub;
    W_Fetch(&_g_mapsubsectors[num], &mapsub, sizeof(mapsub));

    const seg_t* segp = _g_segs + mapsub.firstseg;
    int16_t count;
    fixed_t opentop = 0, openbottom = 0;
    const sector_t* front = NULL;
    const sector_t* back  = NULL;

    for (count = mapsub.numsegs; --count >= 0; segp++)
    { // check lines
        seg_t segcopy;
        W_Fetch(segp, &segcopy, sizeof(segcopy));
        const seg_t* seg = &segcopy;
        int16_t linenum = seg->linenum;

        divline_t divl;

        // allready checked other side?
        if(LN_BIT(LB_CHECKED, linenum))
            continue;

        LN_SETBIT(LB_CHECKED, linenum);

        line_t maplinecopy;
        W_Fetch(&_g_maplines[linenum], &maplinecopy, sizeof(maplinecopy));
        const line_t* mapline = &maplinecopy;

        if ((fixed_t)mapline->bbox[BOXLEFT  ]<<FRACBITS > los.bbox[BOXRIGHT ] ||
            (fixed_t)mapline->bbox[BOXRIGHT ]<<FRACBITS < los.bbox[BOXLEFT  ] ||
            (fixed_t)mapline->bbox[BOXBOTTOM]<<FRACBITS > los.bbox[BOXTOP   ] ||
            (fixed_t)mapline->bbox[BOXTOP   ]<<FRACBITS < los.bbox[BOXBOTTOM])
            continue;

        // cph - do what we can before forced to check intersection
        if (mapline->flags & ML_TWOSIDED)
        {

            // no wall to block sight with?
            front = &_g_sectors[seg->frontsectornum];
            back  = &_g_sectors[seg->backsectornum];
            if (SEC_FLOORHEIGHT(front) == SEC_FLOORHEIGHT(back) && SEC_CEILINGHEIGHT(front) == SEC_CEILINGHEIGHT(back))
                continue;

            // possible occluder
            // because of ceiling height differences
            opentop = SEC_CEILINGHEIGHT(front) < SEC_CEILINGHEIGHT(back) ?
                        SEC_CEILINGHEIGHT(front) : SEC_CEILINGHEIGHT(back) ;

            // because of floor height differences
            openbottom = SEC_FLOORHEIGHT(front) > SEC_FLOORHEIGHT(back) ?
                        SEC_FLOORHEIGHT(front) : SEC_FLOORHEIGHT(back) ;

            // cph - reject if does not intrude in the z-space of the possible LOS
            if ((opentop >= los.maxz) && (openbottom <= los.minz))
                continue;
        }

        // Forget this line if it doesn't cross the line of sight
        fixed_t v1x = (fixed_t)mapline->v1.x<<FRACBITS;
        fixed_t v1y = (fixed_t)mapline->v1.y<<FRACBITS;
        fixed_t v2x = (fixed_t)mapline->v2.x<<FRACBITS;
        fixed_t v2y = (fixed_t)mapline->v2.y<<FRACBITS;

        if (P_DivlineSide(v1x, v1y, &los.strace) == P_DivlineSide(v2x, v2y, &los.strace))
            continue;

        divl.x = v1x;
        divl.y = v1y;
        divl.dx = v2x - v1x;
        divl.dy = v2y - v1y;

        // line isn't crossed?
        if (P_DivlineSide(los.strace.x, los.strace.y, &divl) == P_DivlineSide(los.t2x, los.t2y, &divl))
            continue;


        // cph - if bottom >= top or top < minz or bottom > maxz then it must be
        // solid wrt this LOS
        if (!(mapline->flags & ML_TWOSIDED) || (openbottom >= opentop) ||
                (opentop < los.minz) || (openbottom > los.maxz))
            return false;

        // crosses a two sided line
        uint16_t frac = P_InterceptVector2(&los.strace, &divl);

        if (SEC_FLOORHEIGHT(front) != SEC_FLOORHEIGHT(back))
        {
            fixed_t slope = frac != 0 ? ((openbottom - los.sightzstart) >> FRACBITS) * FixedReciprocalSmall(frac) : INT32_MAX;
            if (slope > los.bottomslope)
                los.bottomslope = slope;
        }

        if (SEC_CEILINGHEIGHT(front) != SEC_CEILINGHEIGHT(back))
        {
            fixed_t slope = frac != 0 ? ((opentop - los.sightzstart) >> FRACBITS) * FixedReciprocalSmall(frac) : INT32_MAX;
            if (slope < los.topslope)
                los.topslope = slope;
        }

        if (los.topslope <= los.bottomslope)
            return false;               // stop

    }
    // passed the subsector ok
    return true;
}


static NOINLINE boolean P_CrossBSPNodeSides(int16_t bspnum, int16_t* children)
{
    mapnode_t bsp;
    W_Fetch(nodes + bspnum, &bsp, sizeof(bsp));

    divline_t dl;
    dl.x = ((fixed_t)bsp.x << FRACBITS);
    dl.y = ((fixed_t)bsp.y << FRACBITS);
    dl.dx = ((fixed_t)bsp.dx << FRACBITS);
    dl.dy = ((fixed_t)bsp.dy << FRACBITS);

    int8_t side,side2;
    side = P_DivlineSide(los.strace.x,los.strace.y,&dl)&1;
    side2= P_DivlineSide(los.t2x, los.t2y, &dl);

    children[0] = bsp.children[side];
    children[1] = bsp.children[side^1];
    return side == side2;
}

static boolean P_CrossBSPNode(int16_t bspnum)
{
    int16_t ending[MAX_BSP_DEPTH];
    int16_t sp = 0;

    while (true)
    {
        while (!(bspnum & NF_SUBSECTOR))
        {
            int16_t children[2];

            if (!P_CrossBSPNodeSides(bspnum, children))
            {
                if (sp == MAX_BSP_DEPTH)
                    I_Error("P_CrossBSPNode: deeper than %i", MAX_BSP_DEPTH);
                ending[sp++] = children[1];
            }
            bspnum = children[0];
        }
        if (!P_CrossSubsector(bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR))
            return false;
        if (sp == 0)
            return true;
        bspnum = ending[--sp];
    }
}


static uint32_t linearAddress(const void* ptr)
{
	return (uint32_t)(uintptr_t)ptr;
}


// Which thing the last result was for. A map thing's mobj can change while it sleeps, so it is
// known by its load index.
static uint32_t P_SightKey(const mobj_t* t)
{
  if (!(t->flags & MF_STATIC) && P_Ext(t)->loadthing)
    return 0xffff0000UL | (uint16_t)P_Ext(t)->loadthing;
  if ((t->flags & (MF_STATIC | MF_INPLACE)) == (MF_STATIC | MF_INPLACE))
    return 0xffff0000UL | (uint16_t)(t->sprev + 1);
  return linearAddress(t);
}

//
// P_CheckSight
// Returns true
//  if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
// killough 4/20/98: cleaned up, made to use new LOS struct

boolean P_CheckSight(mobj_t* t1, mobj_t* t2)
{
  static uint32_t prevlat1;
  static uint32_t prevlat2;
  static boolean prevr;

  if (prevlat1 == P_SightKey(t1)
   && prevlat2 == P_SightKey(t2))
    return prevr;
  prevlat1 = P_SightKey(t1);
  prevlat2 = P_SightKey(t2);

  const sector_t* s1 = P_SubsectorSector(t1->subsector);
  const sector_t* s2 = P_SubsectorSector(t2->subsector);
  int16_t pnum = (s1-_g_sectors)*_g_numsectors + (s2-_g_sectors);

  // First check for trivial rejection.
  // Determine subsector entries in REJECT table.
  //
  // Check in REJECT table.

  if (W_FetchUInt8(&_g_rejectmatrix[pnum>>3]) & (1 << (pnum&7)))   // can't possibly be connected
  {
    prevr = false;
    return prevr;
  }

  /* killough 11/98: shortcut for melee situations
   * same subsector? obviously visible
   * cph - compatibility optioned for demo sync, cf HR06-UV.LMP */
  if (t1->subsector == t2->subsector)
  {
    prevr = true;
    return prevr;
  }

  // An unobstructed LOS is possible.
  // Now look from eyes of t1 to any part of t2.

  P_NextValidCount();

  los.topslope = (los.bottomslope = t2->z - (los.sightzstart =
                                             t1->z + t1->height -
                                             (t1->height>>2))) + t2->height;
  los.strace.dx = (los.t2x = t2->x) - (los.strace.x = t1->x);
  los.strace.dy = (los.t2y = t2->y) - (los.strace.y = t1->y);

  if (t1->x > t2->x)
    los.bbox[BOXRIGHT] = t1->x, los.bbox[BOXLEFT] = t2->x;
  else
    los.bbox[BOXRIGHT] = t2->x, los.bbox[BOXLEFT] = t1->x;

  if (t1->y > t2->y)
    los.bbox[BOXTOP] = t1->y, los.bbox[BOXBOTTOM] = t2->y;
  else
    los.bbox[BOXTOP] = t2->y, los.bbox[BOXBOTTOM] = t1->y;


    los.maxz = INT32_MAX; los.minz = INT32_MIN;

  // the head node is the last node output
  prevr = P_CrossBSPNode(numnodes-1);
  return prevr;
}
