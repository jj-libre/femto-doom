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
 *  Plats (i.e. elevator platforms) code, raising/lowering.
 *
 *-----------------------------------------------------------------------------*/

#include <stdint.h>

#include "d_player.h"
#include "m_random.h"
#include "r_main.h"
#include "p_spec.h"
#include "p_tick.h"
#include "s_sound.h"
#include "sounds.h"

#include "globdata.h"


static void P_RemovePlat(plat_t* plat);

//
// T_PlatRaise()
//
// Action routine to move a plat up and down
//
// Passed a plat structure containing all pertinent information about the move
// No return
//

void T_PlatRaise(thinker_t* th)
{
  plat_t* plat = (plat_t*)th;
  result_e      res;

  // handle plat moving, up, down, or waiting
  switch(plat->status)
  {
    case up: // plat moving up
      res = T_MovePlaneFloor(plat->sector,plat->speed,plat->high,1);

      // if a pure raise type, make the plat moving sound
      if (plat->type == raiseToNearestAndChange)
      {
        if (!((int16_t)_g_leveltime&7))
          S_StartSound2(plat->sector, sfx_stnmov);
      }

      // if encountered an obstacle, and not a crush type, reverse direction
      if (res == crushed)
      {
        plat->count = plat->wait;
        plat->status = down;
        S_StartSound2(plat->sector, sfx_pstart);
      }
      else  // else handle reaching end of up stroke
      {
        if (res == pastdest) // end of stroke
        {
          // not an instant toggle type, wait, make plat stop sound
          plat->count = plat->wait;
          plat->status = waiting;
          S_StartSound2(plat->sector, sfx_pstop);

          // lift types and pure raise types are done at end of up stroke
          // only the perpetual type waits then goes back up
          switch(plat->type)
          {
            case downWaitUpStay:
            case raiseToNearestAndChange:
              P_RemovePlat(plat);
            default:
              break;
          }
        }
      }
      break;

    case down: // plat moving down
      res = T_MovePlaneFloor(plat->sector,plat->speed,plat->low,-1);

      // handle reaching end of down stroke
      if (res == pastdest)
      {
        // not an instant toggle, start waiting, make plat stop sound
        plat->count = plat->wait;
        plat->status = waiting;
        S_StartSound2(plat->sector,sfx_pstop);

        //jff 1/26/98 remove the plat if it bounced so it can be tried again
        //only affects plats that raise and bounce
        //killough 1/31/98: relax compatibility to demo_compatibility

          switch(plat->type)
          {
            case raiseToNearestAndChange:
              P_RemovePlat(plat);
            default:
              break;
          }

      }
      break;

    case waiting: // plat is waiting
      if (!--plat->count)  // downcount and check for delay elapsed
      {
        if (SEC_FLOORHEIGHT(plat->sector) == plat->low)
          plat->status = up;     // if at bottom, start up
        else
          plat->status = down;   // if at top, start down

        // make plat start sound
        S_StartSound2(plat->sector,sfx_pstart);
      }
      break;
  }
}


//
// EV_DoPlat
//
// Handle Plat linedef types
//
// Passed the linedef that activated the plat and the type of plat action.
// Returns true if a thinker is started
//

boolean EV_DoPlat(const line_t* line, plattype_e type)
{
  plat_t* plat;
  int16_t             secnum;
  boolean             rtn;
  sector_t*       sec;

  secnum = -1;
  rtn = false;

  // act on all sectors tagged the same as the activating linedef
  while ((secnum = P_FindSectorFromLineTag(LN_TAG(line),secnum)) >= 0)
  {
    sec = &_g_sectors[secnum];

    // don't start a second floor function if already moving
    if (SEC_FLOORDATA(sec) != 0) // multiple thinkers
      continue;

    // Create a thinker
    rtn = true;
    plat = Z_CallocLevSpec(sizeof(*plat));
    P_AddThinker(&plat->thinker);

    plat->type = type;
    plat->sector = sec;
    P_Mover(plat->sector)->floordata = Z_Ref(plat); //jff 2/23/98 multiple thinkers
    plat->thinker.function = T_PlatRaise;
    plat->tag = LN_TAG(line);

    //jff 1/26/98 Avoid raise plat bouncing a head off a ceiling and then
    //going down forever -- default low to plat height when triggered
    plat->low = SEC_FLOORHEIGHT(sec);

    // set up plat according to type
    switch(type)
    {
      case raiseToNearestAndChange:
        plat->speed = PLATSPEED/2;
        sec->floorpic = SIDE_SECTOR(LN_SIDENUM(line, 0))->floorpic;
        plat->high = P_FindNextHighestFloor(sec);
        plat->wait = 0;
        plat->status = up;
        sec->special = 0;
        //jff 3/14/98 clear old field as well
        SEC_CLRBIT(SB_SECRET, sec - _g_sectors);

        S_StartSound2(sec,sfx_stnmov);
        break;

      case downWaitUpStay:
        plat->speed = PLATSPEED * 4;
        plat->low = P_FindLowestFloorSurrounding(sec);

        if (plat->low > SEC_FLOORHEIGHT(sec))
          plat->low = SEC_FLOORHEIGHT(sec);

        plat->high = SEC_FLOORHEIGHT(sec);
        plat->wait = 35*PLATWAIT;
        plat->status = down;
        S_StartSound2(sec,sfx_pstart);
        break;

      default:
        break;
    }
  }
  return rtn;
}

//
// P_RemovePlat()
//
// Remove a plat that has finished moving
//
// Passed a pointer to the plat to remove
// Returns nothing
//
static void P_RemovePlat(plat_t* plat)
{
  P_Mover(plat->sector)->floordata = Z_Ref(0); //jff 2/23/98 multiple thinkers

  P_RemoveThinker(&plat->thinker);
}
