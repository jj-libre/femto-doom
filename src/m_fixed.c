/*-----------------------------------------------------------------------------
 *
 *
 *  Copyright (C) 2026 Frenkel Smeijers
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
 *      Calculate 0xffffffffu / v
 *
 *-----------------------------------------------------------------------------*/

#include "m_fixed.h"


#if !defined RECIPROCAL_DIV
#define RECIPROCAL_DIV(v) (0xffffffffu / (v))
#endif

#define RECIPROCAL16(v) ((v) ? RECIPROCAL_DIV(v) : 0)


fixed_t CONSTFUNC FixedReciprocal(fixed_t v)
{
	if (v <= 0xffffu)
		return FixedReciprocalSmall(v);
	else
		return FixedReciprocalBig(v);
}


#if !defined __GNUC__
static int __builtin_clz(uint32_t v)
{
	const int bits = sizeof(v) * CHAR_BIT;
	
	for (int i = 0; i < bits; i++)
		if ((v >> (bits - 1 - i)) & 1)
			return i;

	return bits;
}
#endif


uint16_t CONSTFUNC FixedReciprocalBig(fixed_t v)
{
	int s = 31 - __builtin_clz(v) - 15;
	return RECIPROCAL16((uint32_t)v >> s) >> s;
}


fixed_t CONSTFUNC FixedReciprocalSmall(uint16_t v)
{
	return RECIPROCAL16(v);
}
