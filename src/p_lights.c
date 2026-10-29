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
 *  Action routines for lighting thinkers
 *  Spawn sector based lighting effects.
 *  Handle lighting linedef types
 *
 *-----------------------------------------------------------------------------*/

#include "d_player.h"
#include "doomdef.h"
#include "m_random.h"
#include "r_main.h"
#include "p_spec.h"
#include "p_tick.h"

#include "globdata.h"

//////////////////////////////////////////////////////////
//
// Lighting action routines, called once per tick
//
//////////////////////////////////////////////////////////

//
// T_LightFlash()
//
// Broken light flashing action routine, called once per tick
//
// Passed a lightflash_t structure containing light levels and timing
// Returns nothing
//

typedef struct
{
  thinker_t thinker;
  sector_t* sector;
  int16_t maxlight;
  int16_t minlight;
  int8_t count;
} lightflash_t;

typedef char assertLightflashSize[sizeof(void*) != 4 || sizeof(lightflash_t) == 24 ? 1 : -1];


static void T_LightFlash(thinker_t* th)
{
  lightflash_t* flash = (lightflash_t*)th;
  if (--flash->count)
    return;

  if (flash->sector->lightlevel == flash->maxlight)
  {
    flash->sector->lightlevel = flash->minlight;
    flash->count = (P_Random() & 7) + 1;
  }
  else
  {
    flash->sector->lightlevel = flash->maxlight;
    flash->count = (P_Random() & 64) + 1;
  }

}

//
// T_StrobeFlash()
//
// Strobe light flashing action routine, called once per tick
//
// Passed a strobe_t structure containing light levels and timing
// Returns nothing
//

typedef struct
{
  thinker_t thinker;
  sector_t* sector;
  int16_t minlight;
  int16_t maxlight;
  int8_t count;
  int8_t darktime;
} strobe_t;

typedef char assertStrobeSize[sizeof(void*) != 4 || sizeof(strobe_t) == 24 ? 1 : -1];


static void T_StrobeFlash(thinker_t* th)
{
  strobe_t* flash = (strobe_t*)th;
  if (--flash->count)
    return;

  if (flash->sector->lightlevel == flash->minlight)
  {
    flash-> sector->lightlevel = flash->maxlight;
    flash->count = STROBEBRIGHT;
  }
  else
  {
    flash-> sector->lightlevel = flash->minlight;
    flash->count =flash->darktime;
  }
}

//
// T_Glow()
//
// Glowing light action routine, called once per tick
//
// Passed a glow_t structure containing light levels and timing
// Returns nothing
//

typedef struct
{
  thinker_t thinker;
  sector_t* sector;
  int16_t minlight;
  int16_t maxlight;
  int8_t direction;
} glow_t;

typedef char assertGlowSize[sizeof(void*) != 4 || sizeof(glow_t) == 24 ? 1 : -1];


static void T_Glow(thinker_t* th)
{
  glow_t* g = (glow_t*)th;
  int16_t lightlevel = g->sector->lightlevel;
  switch(g->direction)
  {
    case -1:
      // light dims
      lightlevel -= GLOWSPEED;
      if (lightlevel <= g->minlight)
      {
        lightlevel += GLOWSPEED;
        g->direction = 1;
      }
      break;

    case 1:
      // light brightens
      lightlevel += GLOWSPEED;
      if (lightlevel >= g->maxlight)
      {
        lightlevel -= GLOWSPEED;
        g->direction = -1;
      }
      break;
  }
  g->sector->lightlevel = lightlevel;
}


//
// P_FindMinSurroundingLight()
//
// Passed a sector and a light level, returns the smallest light level
// in a surrounding sector less than that passed. If no smaller light
// level exists, the light level passed is returned.
//
static int16_t P_FindMinSurroundingLight(sector_t* sector, int16_t max)
{
  int16_t         i;
  int16_t         min;
  const line_t*     line;
  sector_t*   check;

  min = max;
  for (i=0 ; i < SEC_LINECOUNT(sector) ; i++)
  {
    line = SEC_LINE(sector, i);
    check = getNextSector(line,sector);

    if (!check)
      continue;

    if (check->lightlevel < min)
      min = check->lightlevel;
  }
  return min;
}


//////////////////////////////////////////////////////////
//
// Sector lighting type spawners
//
// After the map has been loaded, each sector is scanned
// for specials that spawn thinkers
//
//////////////////////////////////////////////////////////

//
// P_SpawnLightFlash()
//
// Spawns a broken light flash lighting thinker
//
// Passed the sector that spawned the thinker
// Returns nothing
//
void P_SpawnLightFlash (sector_t* sector)
{
  lightflash_t* flash;

  // nothing special about it during gameplay
  sector->special = 0;

  flash = Z_CallocLevSpec(sizeof(*flash));

  P_AddThinker (&flash->thinker);

  flash->thinker.function = T_LightFlash;
  flash->sector = sector;
  flash->maxlight = sector->lightlevel;

  flash->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
  flash->count = (P_Random() & 64) + 1;
}

//
// P_SpawnStrobeFlash
//
// Spawns a blinking light thinker
//
// Passed the sector that spawned the thinker, speed of blinking
// and whether blinking is to by syncrhonous with other sectors
//
// Returns nothing
//
void P_SpawnStrobeFlash(sector_t* sector, int8_t fastOrSlow, boolean inSync)
{
  strobe_t* flash;

  flash = Z_CallocLevSpec(sizeof(*flash));

  P_AddThinker (&flash->thinker);

  flash->sector = sector;
  flash->darktime = fastOrSlow;
  flash->thinker.function = T_StrobeFlash;
  flash->maxlight = sector->lightlevel;
  flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);

  if (flash->minlight == flash->maxlight)
    flash->minlight = 0;

  // nothing special about it during gameplay
  sector->special = 0;

  if (!inSync)
    flash->count = (P_Random()&7)+1;
  else
    flash->count = 1;
}

//
// P_SpawnGlowingLight()
//
// Spawns a glowing light (smooth oscillation from min to max) thinker
//
// Passed the sector that spawned the thinker
// Returns nothing
//
void P_SpawnGlowingLight(sector_t* sector)
{
  glow_t* g;

  g = Z_CallocLevSpec(sizeof(*g));

  P_AddThinker(&g->thinker);

  g->sector = sector;
  g->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
  g->maxlight = sector->lightlevel;
  g->thinker.function = T_Glow;
  g->direction = -1;

  sector->special = 0;
}

//////////////////////////////////////////////////////////
//
// Linedef lighting function handlers
//
//////////////////////////////////////////////////////////

//
// EV_LightTurnOn()
//
// Turn sectors tagged to line lights on to specified or max neighbor level
//
// Passed the activating line, and a level to set the light to
// If level passed is 0, the maximum neighbor lighting is used
//
void EV_LightTurnOn(const line_t* line, int16_t bright)
{
	int16_t i;

	// search all sectors for ones with same tag as activating line

	for (i = -1; (i = P_FindSectorFromLineTag(LN_TAG(line),i)) >= 0;)
	{
		sector_t* temp;
		sector_t* sector = _g_sectors+i;
		int16_t j, tbright = bright; //jff 5/17/98 search for maximum PER sector

		// bright = 0 means to search for highest light level surrounding sector

		if (!bright)
			for (j = 0;j < SEC_LINECOUNT(sector); j++)
				if ((temp = getNextSector(SEC_LINE(sector, j),sector)) && temp->lightlevel > tbright)
					tbright = temp->lightlevel;

		sector->lightlevel = tbright;
	}
}
