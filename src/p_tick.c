/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000,2002 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *  Copyright 2023, 2024 by
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
 *      Thinker, Ticker.
 *
 *-----------------------------------------------------------------------------*/

#include "d_player.h"
#include "p_user.h"
#include "p_spec.h"
#include "p_tick.h"
#include "p_map.h"
#include "p_maputl.h"
#include "p_mobj.h"

#include "globdata.h"


int32_t _g_leveltime; // tics in game play for par

// killough 8/29/98: we maintain several separate threads, each containing
// a special class of thinkers, to allow more efficient searches.
thinker_t _g_thinkerclasscap;

// The thinker P_RunThinkers runs next, and the load index of the map thing thinking now. A map
// thing linked right before runnext runs next if it has not had its turn yet.
thinker_t* _g_runnext;
int16_t _g_runlast;


//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//



//
// P_InitThinkers
//

void P_InitThinkers(void)
{
  _g_thinkerclasscap.prev = _g_thinkerclasscap.next  = &_g_thinkerclasscap;
}

//
// P_AddThinker
// Adds a new thinker at the end of the list.
//

void P_AddThinker(thinker_t* thinker)
{
  _g_thinkerclasscap.prev->next = thinker;
  thinker->next = &_g_thinkerclasscap;
  thinker->prev = _g_thinkerclasscap.prev;
  _g_thinkerclasscap.prev = thinker;
}

void P_UnlinkThinker(thinker_t* thinker)
{
  (thinker->next->prev = thinker->prev)->next = thinker->next;
}

//
// killough 11/98:
//
// Make currentthinker external, so that P_RemoveThinkerDelayed
// can adjust currentthinker when thinkers self-remove.


//
// P_RemoveThinkerDelayed()
//
// Called automatically as part of the thinker loop in P_RunThinkers(),
// on nodes which are pending deletion.
//
// If this thinker has no more pointers referencing it indirectly,
// remove it, and set currentthinker to one node preceeding it, so
// that the next step in P_RunThinkers() will get its successor.
//

static void P_RemoveThinkerDelayed(thinker_t* thinker)
{

    thinker_t* next = thinker->next;
    /* Note that currentthinker is guaranteed to point to us,
         * and since we're freeing our memory, we had better change that. So
         * point it to thinker->prev, so the iterator will correctly move on to
         * thinker->prev->next = thinker->next */
    (next->prev = thinker->prev)->next = next;

    Z_Free(thinker);
}

static void P_RemoveThingDelayed(thinker_t* thinker)
{

    thinker_t* next = thinker->next;
    /* Note that currentthinker is guaranteed to point to us,
         * and since we're freeing our memory, we had better change that. So
         * point it to thinker->prev, so the iterator will correctly move on to
         * thinker->prev->next = thinker->next */
    (next->prev = thinker->prev)->next = next;

    mobj_t* thing = P_MobjOfThinker(thinker);

    if(thing->flags & MF_POOLED)
        thing->type = MT_NOTHING;
    else
        Z_Free(thinker);
}

//
// P_RemoveThinker
//
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
// killough 4/25/98:
//
// Instead of marking the function with -1 value cast to a function pointer,
// set the function to P_RemoveThinkerDelayed(), so that later, it will be
// removed automatically as part of the thinker process.
//

void P_RemoveThinker(thinker_t* thinker)
{
  thinker->function = P_RemoveThinkerDelayed;
}

// Map things spawn first and thinkers are only ever added at the end, so the map things
// lead the list in load order.
static int16_t P_ThinkerLoadIndex(thinker_t* th)
{
  if (th->function == P_MobjThinker || th->function == P_RemoveThingDelayed)
  {
    const int16_t i = ((mobjext_t*)th)->loadthing;
    if (i)
      return i - 1;
  }
  return INT16_MAX;
}

// Links a map thing that was asleep where it would have been all along.
void P_AddLoadThinker(thinker_t* thinker, int16_t t)
{
  thinker_t* next = _g_thinkerclasscap.next;
  while (next != &_g_thinkerclasscap && P_ThinkerLoadIndex(next) < t)
    next = next->next;
  thinker->next = next;
  thinker->prev = next->prev;
  next->prev->next = thinker;
  next->prev = thinker;
  if (next == _g_runnext && t > _g_runlast)
    _g_runnext = thinker;
}

void P_RemoveThing(mobj_t* thing)
{
  if (thing->flags & MF_STATIC)
    thing->flags |= MF_REMOVED;
  else
    P_Ext(thing)->thinker.function = P_RemoveThingDelayed;
}


/* cph 2002/01/13 - iterator for thinker list
 * WARNING: Do not modify thinkers between calls to this function
 */
thinker_t* P_NextThinker(thinker_t* th)
{
  thinker_t* top = &_g_thinkerclasscap;
  if (!th) th = top;
  th = th->next;
  return th == top ? NULL : th;
}


//
// P_RunThinkers
//
// killough 4/25/98:
//
// Fix deallocator to stop using "next" pointer after node has been freed
// (a Doom bug).
//
// Process each thinker. For thinkers which are marked deleted, we must
// load the "next" pointer prior to freeing the node. In Doom, the "next"
// pointer was loaded AFTER the thinker was freed, which could have caused
// crashes.
//
// But if we are not deleting the thinker, we should reload the "next"
// pointer after calling the function, in case additional thinkers are
// added at the end of the list.
//
// killough 11/98:
//
// Rewritten to delete nodes implicitly, by making currentthinker
// external and using P_RemoveThinkerDelayed() implicitly.
//

static void P_RunThinkers (void)
{
    thinker_t* th = _g_thinkerclasscap.next;
    thinker_t* th_end = &_g_thinkerclasscap;
    int16_t sleeper = 0;

    _g_runlast = -1;
    for (;;)
    {
        const int16_t t = th == th_end ? INT16_MAX : P_ThinkerLoadIndex(th);
        _g_runnext = th;
        sleeper = P_RunSleepers(sleeper, t);
        if (_g_runnext != th)
        {
            th = _g_runnext;
            continue;
        }
        if (th == th_end)
            break;

        _g_runlast = t;
        _g_runnext = th->next;
        if(th->function)
            th->function(th);
        if (th->function == P_MobjThinker)
            P_SleepIfUnchanged((mobjext_t*)th);

        th = _g_runnext;
    }
    _g_runnext = NULL;

    P_RunStatics();
}


void P_Ticker (void)
{
  /* pause if in menu and at least one tic has been run
   *
   * killough 9/29/98: note that this ties in with basetic,
   * since G_Ticker does the pausing during recording or
   * playback, and compenates by incrementing basetic.
   *
   * All of this complicated mess is used to preserve demo sync.
   */

  if (_g_menuactive && !_g_demoplayback && _g_player.viewz != 1)
    return;

               // not if this is an intermission screen
  if(_g_gamestate==GS_LEVEL)
    P_PlayerThink(&_g_player);

  P_RunThinkers();
  P_UpdateSpecials();
  P_MapEnd();
  _g_leveltime++;                       // for par times
}

