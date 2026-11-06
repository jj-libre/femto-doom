/*-----------------------------------------------------------------------------
 *
 *
 *  Copyright (C) 2024-2026 Frenkel Smeijers
 *  Copyright (C) 2026 Jakob Jungreuthmayer
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
 *  Intermission screens library.
 *
 *-----------------------------------------------------------------------------*/

#include "doomdef.h"
#include "r_main.h"
#include "v_video.h"
#include "w_wad.h"
#include "wi_lib.h"


static wi_box_t box;


void WI_startBox(void)
{
	box.x0 = box.y0 = INT16_MAX;
	box.x1 = box.y1 = INT16_MIN;
}


wi_box_t WI_box(void)
{
	return box;
}


static void WI_drawPatch(int16_t x, int16_t y, const patch_t* patch)
{
	V_DrawPatchScaled(x, y, patch);

	x -= W_FetchInt16(&patch->leftoffset);
	y -= W_FetchInt16(&patch->topoffset);
	int16_t x1 = x + W_FetchInt16(&patch->width);
	int16_t y1 = y + W_FetchInt16(&patch->height);

	if (x < box.x0)
		box.x0 = x;
	if (y < box.y0)
		box.y0 = y;
	if (x1 > box.x1)
		box.x1 = x1;
	if (y1 > box.y1)
		box.y1 = y1;
}


static void WI_drawNamePatch(int16_t x, int16_t y, const char* name)
{
	WI_drawPatch(x, y, W_GetLumpByName(name));
}


// Doom coordinate v on a screen axis len pixels long, rounded down or, with up, up; on the screen.
static int16_t WI_toScreen(int16_t v, int16_t len, int16_t vgalen, boolean up)
{
	int32_t s = ((int32_t)v * len + (up ? vgalen - 1 : 0)) / vgalen;
	return s < 0 ? 0 : s > len ? len : s;
}


void WI_restoreBox(const wi_box_t* b, const uint8_t* raw)
{
	int16_t x0 = WI_toScreen(b->x0, SCREEN_WIDTH,  SCREEN_WIDTH_VGA,  false);
	int16_t y0 = WI_toScreen(b->y0, SCREEN_HEIGHT, SCREEN_HEIGHT_VGA, false);
	int16_t x1 = WI_toScreen(b->x1, SCREEN_WIDTH,  SCREEN_WIDTH_VGA,  true);
	int16_t y1 = WI_toScreen(b->y1, SCREEN_HEIGHT, SCREEN_HEIGHT_VGA, true);

	if (x0 < x1 && y0 < y1)
		V_DrawRawRect(raw + (uint32_t)y0 * SCREEN_WIDTH + x0, x0, y0, x1 - x0, y1 - y0);
}


// GLOBAL LOCATIONS
#define WI_TITLEY      2


// SINGLE-PLAYER STUFF
#define SP_STATSX     50
#define SP_STATSY     50

#define SP_TIMEX      8
#define SP_TIMEY      160


// "Finished!" graphics
static const char* const finished = "WIF";


// "Entering" graphic
static const char* const entering = "WIENTER";


// : graphic
static const char* const colon = "WICOLON";


// Time sucks.
static const char* const sucks = "WISUCKS";


// % graphic
static const char* const percent = "WIPCNT";


// You Are Here graphic
static const char* const yah = "WIURH0";


// splat
static const char* const splat = "WISPLAT";


// ====================================================================
// WI_drawLF
// Purpose: Draw the "Finished" level name before showing stats
// Args:    none
// Returns: void
//
void WI_drawLF(int16_t map)
{
	int16_t y = WI_TITLEY;
	char lname[] = "WILV00";
	lname[5] = '0' + map;

	// draw <LevelName>
	const patch_t* patch = W_GetLumpByName(lname);
	WI_drawPatch((SCREEN_WIDTH_VGA - W_FetchInt16(&patch->width))/2, y, patch);

	y += (5 * W_FetchInt16(&patch->height)) / 4;

	// draw "Finished!"
	const patch_t* finishedpatch = W_GetLumpByName(finished);
	WI_drawPatch((SCREEN_WIDTH_VGA - W_FetchInt16(&finishedpatch->width))/2, y, finishedpatch);
}


// ====================================================================
// WI_drawEL
// Purpose: Draw introductory "Entering" and level name
// Args:    none
// Returns: void
//
void WI_drawEL(int16_t map)
{
	int16_t y = WI_TITLEY;
	char lname[] = "WILV00";
	lname[5] = '0' + map;

	const patch_t* patch = W_GetLumpByName(lname);

	// draw "Entering"
	const patch_t* enteringpatch = W_GetLumpByName(entering);
	WI_drawPatch((SCREEN_WIDTH_VGA - W_FetchInt16(&enteringpatch->width))/2, y, enteringpatch);

	// draw level
	y += (5 * W_FetchInt16(&patch->height)) / 4;
	WI_drawPatch((SCREEN_WIDTH_VGA - W_FetchInt16(&patch->width))/2, y, patch);
}


int16_t WI_getColonWidth(void)
{
	return V_NumPatchWidth(W_GetNumForName(colon));
}


void WI_drawColon(int16_t x, int16_t y)
{
	WI_drawNamePatch(x, y, colon);
}


void WI_drawSucks(int16_t x, int16_t y)
{
	const patch_t* patch = W_GetLumpByName(sucks);
	WI_drawPatch(x - W_FetchInt16(&patch->width), y, patch);
}


void WI_drawPercentSign(int16_t x, int16_t y)
{
	WI_drawNamePatch(x, y, percent);
}


// ====================================================================
// WI_drawNum
// Purpose: Draws a number.  If digits > 0, then use that many digits
//          minimum, otherwise only use as many as necessary
// Args:    x, y   -- location
//          n      -- the number to be drawn
//          digits -- number of digits minimum or zero
// Returns: new x position after drawing (note we are going to the left)

//fontwidth = num[0]->width;
#define fontwidth 11

int16_t WI_drawNum(int16_t x, int16_t y, int16_t n, int16_t digits)
{
	char name[] = "WINUM0";

	// draw the new number
	while (digits--)
	{
		x -= fontwidth;
		name[5] = '0' + n % 10;
		WI_drawNamePatch(x, y, name);
		n /= 10;
	}

	return x;
}


// ====================================================================
// WI_drawStats
// Purpose: Put the solo stats on the screen
// Args:    none
// Returns: void

//lineHeight = (3 * num[0]->height) / 2;
#define lineHeight 18

static const struct
{
	const char* label;
	int16_t     x, y;   // of the label
	int16_t     valuex; // where the value's digits end
} stats[WI_NUMSTATS] =
{
	{"WIOSTK",  SP_STATSX,                      SP_STATSY,                             SCREEN_WIDTH_VGA - SP_STATSX},
	{"WIOSTI",  SP_STATSX,                      SP_STATSY + lineHeight,                SCREEN_WIDTH_VGA - SP_STATSX},
	{"WISCRT2", SP_STATSX,                      SP_STATSY + 2 * lineHeight,            SCREEN_WIDTH_VGA - SP_STATSX},
	{"WITIME",  SP_TIMEX,                       SP_TIMEY,                              SCREEN_WIDTH_VGA / 2 - SP_TIMEX},
	{"WIMSTT",  SP_TIMEX,                       (SP_TIMEY + SCREEN_HEIGHT_VGA) / 2,    SCREEN_WIDTH_VGA / 2 - SP_TIMEX},
	{"WIPAR",   SCREEN_WIDTH_VGA / 2 + SP_TIMEX, SP_TIMEY,                             SCREEN_WIDTH_VGA - SP_TIMEX},
};


void WI_drawStatLabels(void)
{
	for (int8_t i = 0; i < WI_NUMSTATS; i++)
		WI_drawNamePatch(stats[i].x, stats[i].y, stats[i].label);
}


void WI_drawStat(int8_t i, int32_t value)
{
	if (i < 3)
		WI_drawPercent(stats[i].valuex, stats[i].y, (int16_t)value);
	else
		WI_drawTime(stats[i].valuex, stats[i].y, value);
}


typedef struct
{
  int16_t   x;       // x/y coordinate pair structure
  int16_t   y;
} point_t;

#define NUMMAPS     9

static const point_t lnodes[NUMMAPS] =
{
    { 185, 164 }, // location of level 1
    { 148, 143 }, // location of level 2
    {  69, 122 }, // location of level 3
    { 209, 102 }, // location of level 4
    { 116,  89 }, // location of level 5
    { 166,  55 }, // location of level 6
    {  71,  56 }, // location of level 7
    { 135,  29 }, // location of level 8
    {  71,  24 }  // location of level 9
};


void WI_drawSplat(int16_t i)
{
	WI_drawNamePatch(lnodes[i].x, lnodes[i].y, splat);
}


void WI_drawYouAreHere(int16_t i)
{
	WI_drawNamePatch(lnodes[i].x, lnodes[i].y, yah);
}
