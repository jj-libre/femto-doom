/*-----------------------------------------------------------------------------
 *
 *
 *  Copyright (C) 2024 Frenkel Smeijers
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
 *  Intermission screens.
 *  Text mode version
 *
 *-----------------------------------------------------------------------------*/

#ifndef __WI_LIB__
#define __WI_LIB__

#include <stdint.h>

// Part of Doom's 320 x 200 screen; x1 and y1 are just outside it.
typedef struct
{
	int16_t x0, y0, x1, y1;
} wi_box_t;

// The area covered by the pictures drawn since WI_startBox.
void WI_startBox(void);
wi_box_t WI_box(void);

// Draws the raw full-screen picture over box.
void WI_restoreBox(const wi_box_t* box, const uint8_t* raw);

void WI_drawLF(int16_t map);
void WI_drawEL(int16_t map);

int16_t WI_getColonWidth(void);
void WI_drawColon(int16_t x, int16_t y);
void WI_drawSucks(int16_t x, int16_t y);
void WI_drawPercentSign(int16_t x, int16_t y);

int16_t WI_drawNum(int16_t x, int16_t y, int16_t n, int16_t digits);

// Kills, items and secrets as percentages, then time, total time and par time.
#define WI_NUMSTATS 6
void WI_drawStatLabels(void);
void WI_drawStat(int8_t i, int32_t value);

void WI_drawSplat(int16_t i);
void WI_drawYouAreHere(int16_t i);

void WI_drawPercent(int16_t x, int16_t y, int16_t p);
void WI_drawTime(int16_t x, int16_t y, int32_t t);

#endif
