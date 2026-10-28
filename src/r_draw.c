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
 *      Rendering main loop and setup functions,
 *       utility functions (BSP, geometry, trigonometry).
 *      See tables.c, too.
 *
 *-----------------------------------------------------------------------------*/

#include <stddef.h>
#include <inttypes.h>
#include <stdint.h>

#include "d_player.h"
#include "w_wad.h"
#include "r_main.h"
#include "p_maputl.h"
#include "r_things.h"
#include "m_fixed.h"
#include "st_stuff.h"
#include "i_platform.h"
#include "g_game.h"
#include "m_random.h"

#include "r_view.h"
#include "globdata.h"


#if VIEW_WINDOW_HEIGHT < 126
typedef int8_t height_t;
#define DROPOFF_OVERFLOW SCHAR_MAX
#else
typedef int16_t height_t;
#define DROPOFF_OVERFLOW SHRT_MAX
#endif


// Silhouette, needed for clipping Segs (mainly)
// and sprites representing things.
#define SIL_NONE    0
#define SIL_BOTTOM  1
#define SIL_TOP     2
#define SIL_BOTH    3

typedef struct drawseg_s
{
  fixed_t scale1, scale2, scalestep;
  uint16_t curline;                     // index in _g_segs
  int16_t bsilheight;                   // do not clip sprites above this, R_SilHeight
  int16_t tsilheight;                   // do not clip sprites below this

  // Lists for sprite clipping, R_Clip,
  // all three adjusted so [x1] is first value.

  int16_t sprtopclip, sprbottomclip;
  int16_t maskedtexturecol; // dropoff overflow
  viewx_t x1, x2;
  uint8_t silhouette;                       // 0=none, 1=bottom, 2=top, 3=both
} drawseg_t;

// Silhouette heights are plane heights in quarter units, which stay within 8191 units.
#define SIL_ALWAYS INT16_MAX   // bsilheight that clips sprites at any height
#define SIL_NEVER  INT16_MIN   // tsilheight that clips sprites at any height

static fixed_t R_SilHeight(int16_t h)
{
  return h == SIL_ALWAYS ? INT32_MAX : h == SIL_NEVER ? INT32_MIN : (fixed_t)h << (FRACBITS - 2);
}


#if defined MAX_DRAWSEGS
#elif VIEW_WINDOW_WIDTH * VIEW_WINDOW_HEIGHT <= 38 * 28
#define MAX_DRAWSEGS    64
#else
#define MAX_DRAWSEGS   128
#endif

static drawseg_t _s_drawsegs[MAX_DRAWSEGS];


#if !defined MAX_OPENINGS
#define MAX_OPENINGS (VIEW_WINDOW_WIDTH*16)
#endif

static height_t openings[MAX_OPENINGS];
static height_t* lastopening;




static viewx_t viewangletox(int16_t va)
{
#ifdef RANGE_CHECK
	if (va < 0)
		I_Error("viewangletox: va < 0: %i", va);
	else if (va >= 4096)
		I_Error("viewangletox: va >= 4096: %i", va);
#endif

#if VIEWANGLETOXMAX == -1023
	return viewangletoxTable[va];
#else
	if (va < VIEWANGLETOXMAX)	//               0 <= va < VIEWANGLETOXMAX
		return VIEW_WINDOW_WIDTH;
	else if (3073 <= va)		//            3073 <= va < 4096
		return 0;
	else						// VIEWANGLETOXMAX <= va < 3073
		return viewangletoxTable[va - VIEWANGLETOXMAX];
#endif
}


static const angle_t tantoangleTable[2049];

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
static const angle16_t* tantoangle16Table = ((angle16_t*)&tantoangleTable[0]);
#else
static const angle16_t* tantoangle16Table = ((angle16_t*)&tantoangleTable[0]) + 1;
#endif

#define tantoangle(t) tantoangleTable[t]
#define tantoangle16(t) tantoangle16Table[(t)*2]


#if defined SMALL_TRIG
#define finetangent(a) ((a) < 2048 ? finetangentHalf[a] : -finetangentHalf[4095 - (a)])
#else
#define finetangent(a) finetangentTable[a]
#endif

static height_t floorclip[VIEW_WINDOW_WIDTH];
static height_t ceilingclip[VIEW_WINDOW_WIDTH];


static height_t screenheightarray[VIEW_WINDOW_WIDTH] = { [0 ... VIEW_WINDOW_WIDTH - 1] = VIEW_WINDOW_HEIGHT };

static height_t negonearray[VIEW_WINDOW_WIDTH] = { [0 ... VIEW_WINDOW_WIDTH - 1] = -1 };

// A drawseg's clip list: an offset into openings, or one of these.
#define CLIP_NONE   INT16_MIN
#define CLIP_SCREEN (INT16_MIN + 1)
#define CLIP_NEGONE (INT16_MIN + 2)

static height_t* R_Clip(int16_t c)
{
  return c == CLIP_NONE ? NULL : c == CLIP_SCREEN ? screenheightarray : c == CLIP_NEGONE ? negonearray : openings + c;
}


//*****************************************
//Globals.
//*****************************************

int16_t numnodes;
const mapnode_t* nodes;

static fixed_t  viewx, viewy, viewz;
static fixed_t  viewcos, viewsin;

angle_t  viewangle;
static angle16_t viewangle16;

static byte solidcol[VIEW_WINDOW_WIDTH];

static const seg_t    * curline;
static seg_t curseg;
static uint8_t maplineflags;
static sector_t * frontsector;
static sector_t * backsector;
static drawseg_t *ds_p;

static int16_t floorplane_color;
static int16_t ceilingplane_color;

static angle16_t             rw_angle1;

static angle16_t         rw_normalangle; // angle to line origin
static int16_t         rw_distance;

static int16_t      rw_stopx;

static fixed_t  rw_scale;
static fixed_t  rw_scalestep;

static int32_t      worldtop;
static int32_t      worldbottom;

static boolean didsolidcol; /* True if at least one column was marked solid */

static boolean  maskedtexture;
static int16_t      toptexture;
static int16_t      bottomtexture;
static int16_t      midtexture;
static texcols_t textoptexture;
static texcols_t texbottomtexture;
static texcols_t texmidtexture;

static fixed_t  rw_midtexturemid;
static fixed_t  rw_toptexturemid;
static fixed_t  rw_bottomtexturemid;

const uint8_t *fullcolormap;
const uint8_t* fixedcolormap;

static int16_t extralight;                           // bumped light from gun blasts


static height_t   *mfloorclip;   // dropoff overflow
static height_t   *mceilingclip; // dropoff overflow
static fixed_t spryscale;
static fixed_t sprtopscreen;

static angle16_t  rw_centerangle;
static int16_t  rw_offset;
static int16_t      rw_lightlevel;

static height_t      *maskedtexturecol; // dropoff overflow

const int16_t  * textureheight; //needed for texture pegging (and TFE fix - killough)


static fixed_t  topfrac;
static fixed_t  topstep;
static fixed_t  bottomfrac;
static fixed_t  bottomstep;

static fixed_t  pixhigh;
static fixed_t  pixlow;

static fixed_t  pixhighstep;
static fixed_t  pixlowstep;

static int32_t      worldhigh;
static int32_t      worldlow;


uint16_t validcount = 1;         // increment every time a check is made

void P_NextValidCount(void)
{
  ++validcount;
  memset(&_g_linebits[LB_CHECKED * _g_linebytes], 0, _g_linebytes);
  memset(_g_secbits, 0, _g_secbytes);
}

//*****************************************
// Constants
//*****************************************

#define COLEXTRABITS (8 - 1)

static const int16_t CENTERX = VIEW_WINDOW_WIDTH  / 2;
       const int16_t CENTER_Y = VIEW_WINDOW_HEIGHT / 2;

static const fixed_t PROJECTION = (VIEW_WINDOW_WIDTH / 2) << FRACBITS;

static const fixed_t  PSPRITESCALE  = FRACUNIT * VIEW_WINDOW_WIDTH / SCREEN_WIDTH_VGA;
static const fixed_t  PSPRITEISCALE = FRACUNIT * SCREEN_WIDTH_VGA / VIEW_WINDOW_WIDTH; // = FixedReciprocal(PSPRITESCALE)

static const fixed_t  PSPRITEYSCALE = FRACUNIT * (VIEW_WINDOW_HEIGHT * 5 / 4) / SCREEN_HEIGHT_VGA;
static const uint16_t PSPRITEYFRACSTEP = (FRACUNIT * SCREEN_HEIGHT_VGA / (VIEW_WINDOW_HEIGHT * 5 / 4)) >> COLEXTRABITS; // = FixedReciprocal(PSPRITEYSCALE) >> COLEXTRABITS

static const angle16_t clipangle = 0x2008; // = xtoviewangleTable[0]


static uint32_t mulu(uint16_t a, uint16_t b) {
	return (uint32_t)a * b;
}


fixed_t CONSTFUNC FixedMul(fixed_t a, fixed_t b)
{
	// Is the result a negative number?
	uint32_t neg = (a ^ b) < 0 ? 0xffff : 0;

	uint16_t alw, ahw, blw, bhw;
	int32_t result;

	// Only work with unsigned numbers.
	a = D_abs(a);
	b = D_abs(b);
	alw = a;
	ahw = a >> FRACBITS;
	blw = b;
	bhw = b >> FRACBITS;

	if (bhw == 0) {
		uint32_t hl = mulu(ahw, blw);

		// Make sure we round towards -inf
		uint32_t ll = (mulu(alw, blw) + neg) >> FRACBITS;

		result = hl + ll;
	} else {
		uint32_t hh = mulu(ahw, bhw) << FRACBITS;
		uint32_t hl = mulu(ahw, blw);
		uint32_t lh = mulu(alw, bhw);

		// Make sure we round towards -inf
		uint32_t ll = (mulu(alw, blw) + neg) >> FRACBITS;

		result = hh + hl + lh + ll;
	}

	if (neg) result = -result;
	return result;
}


//
// FixedMulAngle
// b should be coming from finesine() or finecosine(), so its high word is either 0x0000 or 0xffff
//
fixed_t CONSTFUNC FixedMulAngle(fixed_t a, fixed_t b)
{
	// Is the result a negative number?
	uint32_t neg = (a ^ b) < 0 ? 0xffff : 0;

	uint16_t alw, ahw, blw;
	uint32_t hl, ll;
	int32_t result;

	// Only work with unsigned numbers.
	a = D_abs(a);
	b = D_abs(b);
	alw = a;
	ahw = a >> FRACBITS;
	blw = b;

	hl = mulu(ahw, blw);

	// Make sure we round towards -inf
	ll = (mulu(alw, blw) + neg) >> FRACBITS;

	result = hl + ll;

	if (neg) result = -result;
	return result;
}


fixed_t CONSTFUNC FixedMul3216(fixed_t a, uint16_t blw)
{
	boolean neg = a < 0;

	uint16_t alw, ahw;
	uint32_t ll, hl;
	fixed_t r;

	a = D_abs(a);

	alw = a;
	ahw = a >> FRACBITS;

	ll = mulu(alw, blw);
	hl = mulu(ahw, blw);
	r = (ll >> FRACBITS) + hl;
	if (neg) r = -r;
	return r;
}


//Approx fixed point divide of a/b using reciprocal. -> a * (1/b).
fixed_t CONSTFUNC FixedApproxDiv(fixed_t a, fixed_t b)
{
	if ((uint32_t)b <= 0xffffu)
		return FixedMul(a, FixedReciprocalSmall(b));
	else
		return FixedMul3216(a, FixedReciprocalBig(b));
}


//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//

static PUREFUNC int16_t R_PointOnSide(fixed_t x, fixed_t y, const mapnode_t* node)
{
	int16_t ix = x >> FRACBITS;

	if (!node->dx)
		return ix <= node->x ? node->dy > 0 : node->dy < 0;

	int16_t iy = y >> FRACBITS;

	if (!node->dy)
		return iy <= node->y ? node->dx < 0 : node->dx > 0;

	x -= (fixed_t)node->x << FRACBITS;
	y -= (fixed_t)node->y << FRACBITS;

	ix = x >> FRACBITS;
	iy = y >> FRACBITS;

	// Try to quickly decide by looking at sign bits.
	if ((node->dy ^ node->dx ^ ix ^ iy) < 0)
		return (node->dy ^ ix) < 0;  // (left is negative)

	//return FixedMul(y, node->dx) >= FixedMul(x, node->dy);
	return (y >> 8) * node->dx >= (x >> 8) * node->dy;
}

//
// R_PointInSubsector
//

int16_t R_PointInSubsector(fixed_t x, fixed_t y)
{
	static fixed_t prevx;
	static fixed_t prevy;
	static int16_t prevr;

	if (prevx == x && prevy == y)
		return prevr;

	prevx = x;
	prevy = y;

	int16_t nodenum = numnodes-1;

	// special case for trivial maps (single subsector, no nodes)
	if (numnodes == 0)
	{
		prevr = 0;
		return prevr;
	}

	while (!(nodenum & NF_SUBSECTOR))
	{
		mapnode_t node;
		W_Fetch(&nodes[nodenum], &node, sizeof(node));
		nodenum = node.children[R_PointOnSide(x, y, &node)];
	}

	prevr = (int16_t)(nodenum & ~NF_SUBSECTOR);
	return prevr;
}


#define SLOPERANGE 2048

static CONSTFUNC int16_t SlopeDiv(uint32_t num, uint32_t den)
{
    den = den >> 8;

    if (den == 0)
        return SLOPERANGE;

    const uint16_t ans = (num << 3) / den;//FixedApproxDiv(num << 3, den) >> FRACBITS;

    return (ans <= SLOPERANGE) ? ans : SLOPERANGE;
}


static CONSTFUNC int16_t SlopeDiv16(uint16_t n, uint16_t d)
{
	if (d == 0)
		return SLOPERANGE;

	const uint16_t ans = ((uint32_t)n * SLOPERANGE) / d;

	return (ans <= SLOPERANGE) ? ans : SLOPERANGE;
}


//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangleTable[] table.
//


CONSTFUNC angle_t R_PointToAngle3(fixed_t x, fixed_t y)
{
    if ( (!x) && (!y) )
        return 0;

    if (x>= 0)
    {
        // x >=0
        if (y>= 0)
        {
            // y>= 0

            if (x>y)
            {
                // octant 0
                return tantoangle(SlopeDiv(y,x));
            }
            else
            {
                // octant 1
                return ANG90-1-tantoangle(SlopeDiv(x,y));
            }
        }
        else
        {
            // y<0
            y = -y;

            if (x>y)
            {
                // octant 8
                return -tantoangle(SlopeDiv(y,x));
            }
            else
            {
                // octant 7
                return ANG270+tantoangle(SlopeDiv(x,y));
            }
        }
    }
    else
    {
        // x<0
        x = -x;

        if (y>= 0)
        {
            // y>= 0
            if (x>y)
            {
                // octant 3
                return ANG180-1-tantoangle(SlopeDiv(y,x));
            }
            else
            {
                // octant 2
                return ANG90+ tantoangle(SlopeDiv(x,y));
            }
        }
        else
        {
            // y<0
            y = -y;

            if (x>y)
            {
                // octant 4
                return ANG180+tantoangle(SlopeDiv(y,x));
            }
            else
            {
                // octant 5
                return ANG270-1-tantoangle(SlopeDiv(x,y));
            }
        }
    }
}


#define R_PointToAngle(x,y) R_PointToAngle16((x)>>FRACBITS,(y)>>FRACBITS)


static angle16_t R_PointToAngle16(int16_t x, int16_t y)
{
    x = x - (viewx >> FRACBITS);
    y = y - (viewy >> FRACBITS);

    if (!x && !y)
        return 0;

    if (x >= 0)
    {
        // x >= 0
        if (y >= 0)
        {
            // y >= 0

            if (x > y)
            {
                // octant 0
                return tantoangle16(SlopeDiv16(y, x));
            }
            else
            {
                // octant 1
                return ANG90_16 - 1 - tantoangle16(SlopeDiv16(x, y));
            }
        }
        else
        {
            // y < 0
            y = -y;

            if (x > y)
            {
                // octant 8
                return -tantoangle16(SlopeDiv16(y, x));
            }
            else
            {
                // octant 7
                return ANG270_16 + tantoangle16(SlopeDiv16(x, y));
            }
        }
    }
    else
    {
        // x < 0
        x = -x;

        if (y >= 0)
        {
            // y >= 0
            if (x > y)
            {
                // octant 3
                return ANG180_16 - 1 - tantoangle16(SlopeDiv16(y, x));
            }
            else
            {
                // octant 2
                return ANG90_16 + tantoangle16(SlopeDiv16(x, y));
            }
        }
        else
        {
            // y < 0
            y = -y;

            if (x > y)
            {
                // octant 4
                return ANG180_16 + tantoangle16(SlopeDiv16(y, x));
            }
            else
            {
                // octant 5
                return ANG270_16 - 1 - tantoangle16(SlopeDiv16(x, y));
            }
        }
    }
}


#define SLOPEBITS    11
#define DBITS      (FRACBITS-SLOPEBITS)

static CONSTFUNC int16_t R_PointToDist(int16_t x, int16_t y)
{
    if (viewx == (fixed_t)x << FRACBITS && viewy == (fixed_t)y << FRACBITS)
        return 0;

    fixed_t dx = D_abs(((fixed_t)x << FRACBITS) - viewx);
    fixed_t dy = D_abs(((fixed_t)y << FRACBITS) - viewy);

    if (dy > dx)
    {
        fixed_t t = dx;
        dx = dy;
        dy = t;
    }

    return dx / finecosineapprox((FixedApproxDiv(dy,dx) >> DBITS) / 2);
}


// Lighting constants.

#define LIGHTSEGSHIFT      4


// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.

#define NUMCOLORMAPS 32


const uint8_t* R_LoadColorMap(int16_t lightlevel)
{
    if (fixedcolormap)
        return fixedcolormap;
    else
    {
        if (curline)
        {
            if (curseg.v1.y == curseg.v2.y)
                lightlevel -= 1 << LIGHTSEGSHIFT;
            else if (curseg.v1.x == curseg.v2.x)
                lightlevel += 1 << LIGHTSEGSHIFT;
        }

        lightlevel += (extralight +_g_gamma) << LIGHTSEGSHIFT;

        int16_t cm = ((256-lightlevel)>>2) - 24;

        if(cm >= NUMCOLORMAPS)
            cm = NUMCOLORMAPS-1;
        else if(cm < 0)
            cm = 0;

        return fullcolormap + cm*256;
    }
}


//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//

typedef void (*R_DrawColumn_f)(const draw_column_vars_t *dcvars);

static void R_DrawMaskedColumn(R_DrawColumn_f colfunc, draw_column_vars_t *dcvars, const column_t* column, int16_t shift)
{
    const fixed_t basetexturemid = dcvars->texturemid;

    const height_t fclip_x = mfloorclip[dcvars->x];
    const height_t cclip_x = mceilingclip[dcvars->x];

    post_t post;
    W_Fetch(column, &post, sizeof(post));

    while (post.topdelta != 0xff)
    {
        // calculate unclipped screen coordinates for post
        const int32_t topscreen = sprtopscreen + spryscale*((int32_t)post.topdelta << shift);
        const int32_t bottomscreen = topscreen + spryscale*((int32_t)post.length << shift);

        int16_t yh = (bottomscreen-1)>>FRACBITS;
        int16_t yl = (topscreen+FRACUNIT-1)>>FRACBITS;

        if (yh >= fclip_x)
            yh = fclip_x - 1;

        if (yl <= cclip_x)
            yl = cclip_x + 1;

        // killough 3/2/98, 3/27/98: Failsafe against overflow/crash:
        if (yl <= yh && yh < VIEW_WINDOW_HEIGHT)
        {
            dcvars->source =  (const byte*)column + 3;

            dcvars->texturemid = basetexturemid - (((int32_t)post.topdelta << shift)<<FRACBITS);

            dcvars->yh = yh;
            dcvars->yl = yl;

            // Drawn by either R_DrawColumn or (SHADOW) R_DrawFuzzColumn.
            colfunc (dcvars);
        }

        column = (const column_t*)((const byte*)column + post.length + 4);
        W_Fetch(column, &post, sizeof(post));
    }

    dcvars->texturemid = basetexturemid;
}


//
// R_InitColormaps
//
void R_InitColormaps(void)
{
	fullcolormap = W_GetLumpByName("COLORMAP");
}


//
// A vissprite_t is a thing that will be drawn during a refresh.
// i.e. a sprite object that is partly visible.
//

typedef struct vissprite_s
{
  int16_t x1, x2;
  fixed_t gx, gy;              // for line side calculation
  fixed_t gz;                   // global bottom for silhouette clipping
  fixed_t startfrac;           // horizontal position of x1
  fixed_t scale;
  fixed_t xiscale;             // negative if flipped
  fixed_t texturemid;
  uint16_t fracstep;

  int16_t lump_num;
  int16_t patch_topoffset;

  // for color translation and shadow draw, maxbright frames as well
  const uint8_t* colormap;

} vissprite_t;


void R_DrawFuzzColumn (const draw_column_vars_t *dcvars);


//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
// CPhipps - new wad lump handling, *'s to const*'s
static void R_DrawVisSprite(const vissprite_t *vis)
{
    fixed_t  frac;

    R_DrawColumn_f colfunc = R_DrawColumnSprite;
    draw_column_vars_t dcvars;
    dcvars.colormap = vis->colormap;

    // killough 4/11/98: rearrange and handle translucent sprites
    // mixed with translucent/non-translucenct 2s normals

    if (!dcvars.colormap)   // NULL colormap = shadow draw
        colfunc = R_DrawFuzzColumn;    // killough 3/14/98

    // proff 11/06/98: Changed for high-res
    dcvars.fracstep = vis->fracstep;
    dcvars.texturemid = vis->texturemid;
    frac = vis->startfrac;

    spryscale = vis->scale;
    sprtopscreen = CENTER_Y * FRACUNIT - FixedMul(dcvars.texturemid, spryscale);


    const patch_t* patch = W_GetLumpByNum(vis->lump_num);
    const int16_t width = W_FetchInt16(&patch->width);

    dcvars.x = vis->x1;

    while (dcvars.x < VIEW_WINDOW_WIDTH)
    {
        const column_t* column = (const column_t*) ((const byte*)patch + W_FetchInt32(&patch->columnofs[frac >> FRACBITS]));
        R_DrawMaskedColumn(colfunc, &dcvars, column, SPRITE_SHIFT);

        frac += vis->xiscale;

        if(((frac >> FRACBITS) >= width) || frac < 0)
            break;

        dcvars.x++;
    }
}


//
// R_RenderMaskedSegRange
//

static void R_RenderMaskedSegRange(const drawseg_t *ds, int16_t x1, int16_t x2)
{
	draw_column_vars_t dcvars;

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?

	curline = _g_segs + ds->curline;  // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	W_Fetch(curline, &curseg, sizeof(curseg));

	frontsector = &_g_sectors[curseg.frontsectornum];
	backsector  = &_g_sectors[curseg.backsectornum];

	side_t side;
	P_GetSide(curseg.sidenum, &side);
	int16_t texnum = R_GetTextureTranslation(side.midtexture);

	// killough 4/13/98: get correct lightlevel for 2s normal textures
	rw_lightlevel = frontsector->lightlevel;

	maskedtexturecol = R_Clip(ds->maskedtexturecol);
	rw_scalestep     = ds->scalestep;
	spryscale        = ds->scale1 + (x1 - ds->x1) * rw_scalestep;
	mfloorclip       = R_Clip(ds->sprbottomclip);
	mceilingclip     = R_Clip(ds->sprtopclip);

	// find positioning
	if (W_FetchUInt8(&_g_maplines[curseg.linenum].flags) & ML_DONTPEGBOTTOM)
	{
		dcvars.texturemid = SEC_FLOORHEIGHT(frontsector) > SEC_FLOORHEIGHT(backsector) ? SEC_FLOORHEIGHT(frontsector) : SEC_FLOORHEIGHT(backsector);
		dcvars.texturemid = dcvars.texturemid + ((int32_t)(W_FetchInt16(&textureheight[texnum]) << TEXTURE_SHIFT) << FRACBITS) - viewz;
	}
	else
	{
		dcvars.texturemid =SEC_CEILINGHEIGHT(frontsector)<SEC_CEILINGHEIGHT(backsector) ? SEC_CEILINGHEIGHT(frontsector) : SEC_CEILINGHEIGHT(backsector);
		dcvars.texturemid = dcvars.texturemid - viewz;
	}

	dcvars.texturemid += (((int32_t)W_FetchUInt8(&_g_mapsides[curseg.sidenum].rowoffset)) << FRACBITS);

	dcvars.colormap = R_LoadColorMap(rw_lightlevel);

	texture_t texture;
	W_Fetch(R_GetTexture(texnum), &texture, sizeof(texture));

	const uint16_t widthmask = texture.widthmask;

	// draw the columns
	// simple texture == 1 patch
	const patch_t* patch = W_GetLumpByNum(texture.patches[0].patch_num);

	for (dcvars.x = x1 ; dcvars.x <= x2 ; dcvars.x++, spryscale += rw_scalestep)
	{
		height_t xc = maskedtexturecol[dcvars.x];

		if (xc != DROPOFF_OVERFLOW) // dropoff overflow
		{
			xc &= widthmask;

			sprtopscreen = CENTER_Y * FRACUNIT - FixedMul(dcvars.texturemid, spryscale);

			dcvars.fracstep = FixedReciprocal((uint32_t)spryscale) >> COLEXTRABITS;

			// draw the texture
			const column_t* column = (const column_t*) ((const byte*)patch + W_FetchInt32(&patch->columnofs[xc]));

			R_DrawMaskedColumn(R_DrawColumnWall, &dcvars, column, TEXTURE_SHIFT);
			maskedtexturecol[dcvars.x] = DROPOFF_OVERFLOW; // dropoff overflow
		}
	}

	curline = NULL; /* cph 2001/11/18 - must clear curline now we're done with it, so R_LoadColorMap doesn't try using it for other things */
}


static boolean R_PointOnSegSide(fixed_t x, fixed_t y, const seg_t* line)
{
    vertex16_t v[2];
    W_Fetch(line, v, sizeof(v));

    const int16_t lx = v[0].x;
    const int16_t ly = v[0].y;
    const int16_t ldx = v[1].x - lx;
    const int16_t ldy = v[1].y - ly;

    if (!ldx)
        return x <= (fixed_t)lx << FRACBITS ? ldy > 0 : ldy < 0;

    if (!ldy)
        return y <= (fixed_t)ly << FRACBITS ? ldx < 0 : ldx > 0;

    x -= (fixed_t)lx << FRACBITS;
    y -= (fixed_t)ly << FRACBITS;

    // Try to quickly decide by looking at sign bits.
    if ((ldy ^ ldx ^ (x >> FRACBITS) ^ (y >> FRACBITS)) < 0)
        return (ldy ^ (x >> FRACBITS)) < 0;          // (left is negative)

    return FixedMul3216(y, ldx) >= FixedMul3216(x, ldy);
}


//
// R_DrawSprite
//

static void R_DrawSprite (const vissprite_t* spr)
{
    height_t* clipbot = floorclip;
    height_t* cliptop = ceilingclip;

    fixed_t scale;
    fixed_t lowscale;

    for (int16_t x = spr->x1; x <= spr->x2; x++)
    {
        clipbot[x] = VIEW_WINDOW_HEIGHT;
        cliptop[x] = -1;
    }


    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale is the clip seg.

    // Modified by Lee Killough:
    // (pointer check was originally nonportable
    // and buggy, by going past LEFT end of array):

    const drawseg_t* drawsegs  =_s_drawsegs;

    for (const drawseg_t* ds = ds_p; ds-- > drawsegs; )  // new -- killough
    {
        // determine if the drawseg obscures the sprite
        if (ds->x1 > spr->x2 || ds->x2 < spr->x1 || (!ds->silhouette && ds->maskedtexturecol == CLIP_NONE))
            continue;      // does not cover sprite

        const int16_t r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
        const int16_t r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

        if (ds->scale1 > ds->scale2)
        {
            lowscale = ds->scale2;
            scale    = ds->scale1;
        }
        else
        {
            lowscale = ds->scale1;
            scale    = ds->scale2;
        }

        if (scale < spr->scale || (lowscale < spr->scale && !R_PointOnSegSide (spr->gx, spr->gy, _g_segs + ds->curline)))
        {
            if (ds->maskedtexturecol != CLIP_NONE)       // masked mid texture?
                R_RenderMaskedSegRange(ds, r1, r2);

            continue;               // seg is behind sprite
        }

        // clip this piece of the sprite
        // killough 3/27/98: optimized and made much shorter

        fixed_t gzt = spr->gz + (((int32_t)spr->patch_topoffset) << FRACBITS);
        const height_t* sprbottomclip = R_Clip(ds->sprbottomclip);
        const height_t* sprtopclip    = R_Clip(ds->sprtopclip);

        if ((ds->silhouette & SIL_BOTTOM && spr->gz < R_SilHeight(ds->bsilheight))  // bottom sil
         && (ds->silhouette & SIL_TOP    && gzt     > R_SilHeight(ds->tsilheight))) // top sil
        {
            for (int16_t x = r1; x <= r2; x++)
            {
                if (clipbot[x] == VIEW_WINDOW_HEIGHT)
                    clipbot[x] = sprbottomclip[x];

                if (cliptop[x] == -1)
                    cliptop[x] = sprtopclip[x];
            }
        }
        else if (ds->silhouette & SIL_BOTTOM && spr->gz < R_SilHeight(ds->bsilheight)) // bottom sil
        {
            for (int16_t x = r1; x <= r2; x++)
            {
                if (clipbot[x] == VIEW_WINDOW_HEIGHT)
                    clipbot[x] = sprbottomclip[x];
            }
        }
        else if (ds->silhouette & SIL_TOP && gzt > R_SilHeight(ds->tsilheight)) // top sil
        {
            for (int16_t x = r1; x <= r2; x++)
            {
                if (cliptop[x] == -1)
                    cliptop[x] = sprtopclip[x];
            }
        }
    }

    // all clipping has been performed, so draw the sprite
    mfloorclip   = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite (spr);
}


/*
 * Frame flags:
 * handles maximum brightness (torches, muzzle flare, light sources)
 */



//
// R_DrawPSprite
//

#define BASEXCENTER (SCREEN_WIDTH_VGA  / 2)
#define BASEYCENTER (SCREEN_HEIGHT_VGA / 2)

static void R_DrawPSprite (pspdef_t *psp, int16_t lightlevel)
{
    int16_t           x1, x2;
    uint32_t hl;
    const spriteframe_t *sprdef;
    spriteframe_t sprframe;
    vissprite_t   *vis;
    vissprite_t   avis;
    fixed_t       topoffset;

    // decide which patch to use
    sprdef   = &sprites[psp->state->sprite * MAX_SPRITE_FRAMES];
    W_Fetch(&sprdef[psp->state->frame & FF_FRAMEMASK], &sprframe, sizeof(sprframe));

    patch_t patch;
    W_Fetch(W_GetLumpByNum(sprframe.lump[0]), &patch, offsetof(patch_t, columnofs));
    // calculate edges of the shape
    int16_t tx = psp->sx - BASEXCENTER;

    tx -= patch.leftoffset;
    hl = (uint32_t) tx * PSPRITESCALE;
    x1 = CENTERX + (hl >> FRACBITS);

    tx += patch.width;
    hl = (uint32_t) tx * PSPRITESCALE;
    x2 = CENTERX + (hl >> FRACBITS) - 1;

    // off the side
    if (x2 < 0 || x1 > VIEW_WINDOW_WIDTH)
    {
        return;
    }

    topoffset = ((int32_t)patch.topoffset) << FRACBITS;

    // store information in a vissprite
    vis = &avis;
    // killough 12/98: fix psprite positioning problem
    vis->texturemid = (BASEYCENTER<<FRACBITS) /* +  FRACUNIT/2 */ -
            (psp->sy-topoffset);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= VIEW_WINDOW_WIDTH ? VIEW_WINDOW_WIDTH - 1 : x2;
    // proff 11/06/98: Added for high-res
    vis->scale = PSPRITEYSCALE;
    vis->fracstep = PSPRITEYFRACSTEP;

    vis->xiscale = PSPRITEISCALE;
    vis->startfrac = 0;

    if (vis->x1 > x1)
        vis->startfrac = vis->xiscale*(vis->x1-x1);

    vis->lump_num = sprframe.lump[0];

    if (_g_player.powers[pw_invisibility] > 4*32 || _g_player.powers[pw_invisibility] & 8)
        vis->colormap = NULL;                    // shadow draw
    else if (fixedcolormap)
        vis->colormap = fixedcolormap;           // fixed color
    else if (psp->state->frame & FF_FULLBRIGHT)
        vis->colormap = fullcolormap;            // full bright // killough 3/20/98
    else
        vis->colormap = R_LoadColorMap(lightlevel);  // local light

    R_DrawVisSprite(vis);
}



//
// R_DrawPlayerSprites
//

static void R_DrawPlayerSprites(void)
{

  int16_t i, lightlevel = P_SubsectorSector(_g_player.mo->subsector)->lightlevel;
  pspdef_t *psp;

  // clip to screen bounds
  mfloorclip   = screenheightarray;
  mceilingclip = negonearray;

  // add all active psprites
  for (i=0, psp=_g_player.psprites; i<NUMPSPRITES; i++,psp++)
    if (psp->state)
      R_DrawPSprite (psp, lightlevel);
}


//
// R_SortVisSprites
//

// insertion sort
static void isort(vissprite_t **s, int16_t n)
{
	for (int16_t i = 1; i < n; i++)
	{
		vissprite_t *temp = s[i];
		if (s[i - 1]->scale < temp->scale)
		{
			int16_t j = i;
			while ((s[j] = s[j - 1])->scale < temp->scale && --j)
				;
			s[j] = temp;
		}
	}
}

#define MAXVISSPRITES 8
static int16_t num_vissprite;
static vissprite_t vissprites[MAXVISSPRITES];
static vissprite_t* vissprite_ptrs[MAXVISSPRITES];

static void R_SortVisSprites (void)
{
    int16_t i = num_vissprite;

    if (i)
    {
        while (--i >= 0)
            vissprite_ptrs[i] = vissprites + i;

        isort(vissprite_ptrs, num_vissprite);
    }
}

//
// R_DrawMasked
//

static void R_DrawMasked(void)
{
    drawseg_t *ds;
    drawseg_t* drawsegs = _s_drawsegs;


    R_SortVisSprites();

    // draw all vissprites back to front
    for (int16_t i = num_vissprite; --i >= 0; )
        R_DrawSprite(vissprite_ptrs[i]);

    // render any remaining masked mid textures

    for (ds=ds_p ; ds-- > drawsegs ; )
        if (ds->maskedtexturecol != CLIP_NONE)
            R_RenderMaskedSegRange(ds, ds->x1, ds->x2);

    R_DrawPlayerSprites ();
}


//
// R_NewVisSprite
//
static vissprite_t *R_NewVisSprite(void)
{
    if (num_vissprite >= MAXVISSPRITES)
    {
#ifdef RANGE_CHECK
        I_Error("Vissprite overflow.");
#endif
        return NULL;
    }

    return vissprites + num_vissprite++;
}


//
// R_ClearSprites
// Called at frame start.
//

static void R_ClearSprites(void)
{
    num_vissprite = 0;
}


//*******************************************

//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//

static fixed_t R_ScaleFromGlobalAngle(int16_t x)
{
  int16_t anglea = ANG90_16 + xtoviewangleTable[x];
  int16_t angleb = anglea + viewangle16 - rw_normalangle;

  fixed_t den = rw_distance * finesineapprox(anglea >> ANGLETOFINESHIFT_16);

// proff 11/06/98: Changed for high-res
  fixed_t num = VIEW_WINDOW_HEIGHT * finesineapprox(angleb >> ANGLETOFINESHIFT_16);

  return den > num>>16 ? (num = FixedApproxDiv(num, den)) > 64*FRACUNIT ?
    64*FRACUNIT : num < 256 ? 256 : num : 64*FRACUNIT;
}


//
// R_ProjectSprite
// Generates a vissprite for a thing if it might be visible.
//

#define MINZ        (FRACUNIT*4)
#define MAXZ        (FRACUNIT*1280)

#define SPR_FLIPPED(s, r) (s->flipmask & (1 << r))

static void R_ProjectSprite (mobj_t* thing, int16_t lightlevel)
{
    const fixed_t fx = thing->x;
    const fixed_t fy = thing->y;
    const fixed_t fz = thing->z;

    const fixed_t tr_x = fx - viewx;
    const fixed_t tr_y = fy - viewy;

    fixed_t xc = FixedMulAngle(tr_x, viewcos);
    fixed_t ys = FixedMulAngle(tr_y, viewsin);
    const fixed_t tz = xc - (-ys);

    // thing is behind view plane?
    if (tz < MINZ)
        return;

    //Too far away.
    if(tz > MAXZ)
        return;

    fixed_t yc = FixedMulAngle(tr_y, viewcos);
    fixed_t xs = FixedMulAngle(tr_x, viewsin);
    fixed_t tx = -(yc + (-xs));

    // too far off the side?
    if (D_abs(tx)>(tz<<2))
        return;

    // decide which patch to use for sprite relative to player
    const spriteframe_t       *sprdef   = &sprites[thing->sprite * MAX_SPRITE_FRAMES];
    spriteframe_t sprframecopy;
    W_Fetch(&sprdef[thing->frame & FF_FRAMEMASK], &sprframecopy, sizeof(sprframecopy));
    const spriteframe_t* sprframe = &sprframecopy;

    uint16_t rot = 0;

    if (sprframe->rotate)
    {
        // choose a different rotation based on player view
        angle16_t ang = R_PointToAngle(fx, fy);
        rot = (angle16_t)(ang - (angle16_t)(thing->angle >> FRACBITS) + (angle16_t)(ANG45_16 / 2) * 9) >> 13;
    }

    const boolean flip = (boolean)SPR_FLIPPED(sprframe, rot);
    patch_t patchcopy;
    W_Fetch(W_GetLumpByNum(sprframe->lump[rot]), &patchcopy, offsetof(patch_t, columnofs));
    const patch_t* patch = &patchcopy;

    /* calculate edges of the shape
     * cph 2003/08/1 - fraggle points out that this offset must be flipped
     * if the sprite is flipped; e.g. FreeDoom imp is messed up by this. */
    if (flip)
        tx -= ((int32_t)(patch->width - patch->leftoffset)) << FRACBITS;
    else
        tx -= ((int32_t)patch->leftoffset) << FRACBITS;

    //const fixed_t xscale = FixedDiv(PROJECTION, tz);
    const fixed_t xscale = PROJECTION / (tz >> FRACBITS);

    fixed_t xl = CENTERX * FRACUNIT + FixedMul(tx,xscale);
    const int16_t x1 = (xl >> FRACBITS);

    // off the side?
    if (x1 > VIEW_WINDOW_WIDTH)
    {
        return;
    }

    fixed_t xr = CENTERX * FRACUNIT - FRACUNIT + FixedMul(tx + (((int32_t)patch->width) << FRACBITS), xscale);
    const int16_t x2 = (xr >> FRACBITS);

    // off the side?
    if (xr < 0)
    {
        return;
    }

    //Too small.
    if (xr <= (xl + (FRACUNIT >> 2)))
    {
        return;
    }


    // store information in a vissprite
    vissprite_t* vis = R_NewVisSprite ();

    //No more vissprites.
    if(!vis)
    {
        return;
    }

    //vis->scale           = FixedDiv(PROJECTIONY, tz);
    vis->scale           = (VIEW_WINDOW_HEIGHT * FRACUNIT) / (tz >> FRACBITS);
    vis->fracstep        = tz / (VIEW_WINDOW_HEIGHT << COLEXTRABITS);
    vis->lump_num        = sprframe->lump[rot];
    vis->patch_topoffset = patch->topoffset;
    vis->gx              = fx;
    vis->gy              = fy;
    vis->gz              = fz;
    vis->texturemid      = (fz + (((int32_t)patch->topoffset) << FRACBITS)) - viewz;
    vis->x1              = x1 < 0 ? 0 : x1;
    vis->x2              = x2 >= VIEW_WINDOW_WIDTH ? VIEW_WINDOW_WIDTH - 1 : x2;


    const fixed_t iscale = FixedReciprocal(xscale);

    if (flip)
    {
        vis->startfrac = (((int32_t)patch->width)<<FRACBITS)-1;
        vis->xiscale = -iscale;
    }
    else
    {
        vis->startfrac = 0;
        vis->xiscale = iscale;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale*(vis->x1-x1);

    // get light level
    if (thing->flags & MF_SHADOW)
        vis->colormap = NULL;             // shadow draw
    else if (fixedcolormap)
        vis->colormap = fixedcolormap;      // fixed map
    else if (thing->frame & FF_FULLBRIGHT)
        vis->colormap = fullcolormap;     // full bright  // killough 3/20/98
    else
        vis->colormap = R_LoadColorMap(lightlevel); // diminished light
}

//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
// killough 9/18/98: add lightlevel as parameter, fixing underwater lighting
static int16_t spritelight;

static boolean R_ProjectLoadThing(mobj_t* thing)
{
  R_ProjectSprite(thing, spritelight);
  return true;
}

static void R_AddSprites(int16_t subsec, int16_t lightlevel)
{
  sector_t* sec=P_SubsectorSector(subsec);
  mobj_t* thing;

  // BSP is traversed by subsector.
  // A sector might have been split into several
  //  subsectors during BSP building.
  // Thus we check whether its already added.

  if (SEC_BIT(SB_CHECKED, sec - _g_sectors))
    return;

  // Well, now it will be done.
  SEC_SETBIT(SB_CHECKED, sec - _g_sectors);

  // Handle all things in sector.

  for (thing = P_Mobj(SEC_THINGS(sec)); thing; thing = P_Mobj(thing->snext))
    if (P_SubsectorSector(thing->subsector) == sec)
      R_ProjectSprite(thing, lightlevel);

  spritelight = lightlevel;
  P_SectorLoadThings(sec - _g_sectors, R_ProjectLoadThing);
}


#if defined FLAT_WALL
#define R_DrawSegTextureColumn(w,x,y,z) R_DrawColumnFlat(x,z)
#else
static void R_DrawSegTextureColumn(const texcols_t* tex, int16_t texture, int16_t texcolumn, draw_column_vars_t* dcvars)
{
    UNUSED(texture);
    dcvars->source = tex->columns + ((uint32_t)((uint16_t)(texcolumn & tex->widthmask) >> tex->cshift) << TEXCOLBITS);
    R_DrawColumnWall(dcvars);
}
#endif

//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling textures.
// boolean segtextured is true if any of the segs textures might be visible.
// boolean markfloor is false if the back side is the same plane.
// CALLED: CORE LOOPING ROUTINE.
//

static void R_RenderSegLoop(int16_t rw_x, boolean segtextured, boolean markfloor, boolean markceiling)
{
    draw_column_vars_t dcvars;
    int16_t  texturecolumn = 0;   // shut up compiler warning

    dcvars.colormap = R_LoadColorMap(rw_lightlevel);

    for ( ; rw_x < rw_stopx ; rw_x++)
    {
        // mark floor / ceiling areas

        int16_t yh = bottomfrac>>FRACBITS;
        int16_t yl = (topfrac+FRACUNIT-1)>>FRACBITS;

        int16_t cc_rwx = ceilingclip[rw_x];
        int16_t fc_rwx = floorclip[rw_x];

        // no space above wall?
        int16_t bottom,top = cc_rwx+1;

        dcvars.x  = rw_x;

        if (yl < top)
            yl = top;

        if (markceiling)
        {
            bottom = yl-1;

            if (bottom >= fc_rwx)
                bottom = fc_rwx-1;

            if (top <= bottom)
            {
                dcvars.yl = top;
                dcvars.yh = bottom;
                if (ceilingplane_color == -2)
                    R_DrawSky(&dcvars);
                else
                    R_DrawColumnFlat(ceilingplane_color, &dcvars);
            }
            // SoM: this should be set here
            cc_rwx = bottom;
        }

        bottom = fc_rwx-1;
        if (yh > bottom)
            yh = bottom;

        if (markfloor)
        {

            top  = yh < cc_rwx ? cc_rwx : yh;

            if (++top <= bottom)
            {
                dcvars.yl = top;
                dcvars.yh = bottom;
                R_DrawColumnFlat(floorplane_color, &dcvars);
            }
            // SoM: This should be set here to prevent overdraw
            fc_rwx = top;
        }

        // texturecolumn and lighting are independent of wall tiers
        if (segtextured)
        {
            // calculate texture offset
#if !defined FLAT_WALL
			int16_t ang = (angle16_t)(rw_centerangle + xtoviewangleTable[rw_x]) >> ANGLETOFINESHIFT_16;
			fixed_t tan = finetangent(ang);
			texturecolumn = rw_offset;
			texturecolumn -= (rw_distance * tan) >> FRACBITS;
#endif

            dcvars.fracstep = FixedReciprocal((uint32_t)rw_scale) >> COLEXTRABITS;
        }

        // draw the wall tiers
        if (midtexture)
        {

            dcvars.yl = yl;     // single sided line
            dcvars.yh = yh;
            dcvars.texturemid = rw_midtexturemid;
            //

            R_DrawSegTextureColumn(&texmidtexture, midtexture, texturecolumn, &dcvars);

            cc_rwx = VIEW_WINDOW_HEIGHT;
            fc_rwx = -1;
        }
        else
        {

            // two sided line
            if (toptexture)
            {
                // top wall
                int16_t mid = pixhigh>>FRACBITS;
                pixhigh += pixhighstep;

                if (mid >= fc_rwx)
                    mid = fc_rwx-1;

                if (mid >= yl)
                {
                    dcvars.yl = yl;
                    dcvars.yh = mid;
                    dcvars.texturemid = rw_toptexturemid;

                    R_DrawSegTextureColumn(&textoptexture, toptexture, texturecolumn, &dcvars);

                    cc_rwx = mid;
                }
                else
                    cc_rwx = yl-1;
            }
            else  // no top wall
            {

                if (markceiling)
                    cc_rwx = yl-1;
            }

            if (bottomtexture)          // bottom wall
            {
                int16_t mid = (pixlow+FRACUNIT-1)>>FRACBITS;
                pixlow += pixlowstep;

                // no space above wall?
                if (mid <= cc_rwx)
                    mid = cc_rwx+1;

                if (mid <= yh)
                {
                    dcvars.yl = mid;
                    dcvars.yh = yh;
                    dcvars.texturemid = rw_bottomtexturemid;

                    R_DrawSegTextureColumn(&texbottomtexture, bottomtexture, texturecolumn, &dcvars);

                    fc_rwx = mid;
                }
                else
                    fc_rwx = yh+1;
            }
            else        // no bottom wall
            {
                if (markfloor)
                    fc_rwx = yh+1;
            }

            // cph - if we completely blocked further sight through this column,
            // add this info to the solid columns array for r_bsp.c
            if ((markceiling || markfloor) && (fc_rwx <= cc_rwx + 1))
            {
                solidcol[rw_x] = 1;
                didsolidcol = true;
            }

            // save texturecol for backdrawing of masked mid texture
            if (maskedtexture)
                maskedtexturecol[rw_x] = texturecolumn;
        }

        rw_scale += rw_scalestep;
        topfrac += topstep;
        bottomfrac += bottomstep;

        floorclip[rw_x] = fc_rwx;
        ceilingclip[rw_x] = cc_rwx;
    }
}

static boolean R_CheckOpenings(const int16_t start)
{
    int16_t pos = lastopening - openings;
    int16_t need = 2 * (rw_stopx - start) + pos;

#ifdef RANGE_CHECK
    if(need > MAX_OPENINGS)
        I_Error("Openings overflow. Need = %d", need);
#endif

    return need <= MAX_OPENINGS;
}


static void R_ClearOpeningClippingDetermination(void)
{
	// opening / clipping determination
	for (viewx_t i = 0; i < VIEW_WINDOW_WIDTH; i++)
		floorclip[i] = VIEW_WINDOW_HEIGHT, ceilingclip[i] = -1;
}


static void R_ClearOpenings(void)
{
	lastopening = openings;
}


/* CPhipps -
 * Mod - returns a % b, guaranteeing 0 <= a < b
 * (notice that the C standard for % does not guarantee this)
 */

inline static int16_t CONSTFUNC Mod(uint8_t a, int16_t b)
{
    if(!a)
        return 0;

    if (b & (b-1))
    {
        int16_t r = a % b;
        return ((r<0) ? r+b : r);
    }
    else
        return (a & (b-1));
}


//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
static void R_StoreWallRange(const int16_t start, const int16_t stop)
{
    // don't overflow and crash
    if (ds_p == &_s_drawsegs[MAX_DRAWSEGS])
    {
#ifdef RANGE_CHECK
        I_Error("Drawsegs overflow.");
#endif
        return;
    }


    side_t sidecopy;
    const uint8_t rowoffset = P_GetSide(curseg.sidenum, &sidecopy);
    const side_t* sidedef = &sidecopy;
    // mark the segment as visible for auto map
    LN_SETBIT(LB_MAPPED, curseg.linenum);

    // calculate rw_distance for scale calculation
    rw_normalangle = curseg.angle;

    angle16_t offsetangle = rw_normalangle - rw_angle1;

    if (D_abs((int32_t)((angle_t)offsetangle << FRACBITS)) > ANG90)
        offsetangle = ANG90_16;

    int16_t hyp = R_PointToDist(curseg.v1.x, curseg.v1.y);

    rw_distance = (hyp * finecosineapprox(offsetangle >> ANGLETOFINESHIFT_16)) >> FRACBITS;

    int16_t rw_x = ds_p->x1 = start;
    ds_p->x2 = stop;
    ds_p->curline = curline - _g_segs;
    rw_stopx = stop+1;

    //Openings overflow. Nevermind.
    if(!R_CheckOpenings(start))
        return;

    // calculate scale at both ends and step
    ds_p->scale1 = rw_scale = R_ScaleFromGlobalAngle(start);

    if (stop > start)
    {
        ds_p->scale2 = R_ScaleFromGlobalAngle(stop);
        ds_p->scalestep = rw_scalestep = (ds_p->scale2 - rw_scale) / (stop - start);
    }
    else
        ds_p->scale2 = ds_p->scale1;

    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed

    worldtop = SEC_CEILINGHEIGHT(frontsector) - viewz;
    worldbottom = SEC_FLOORHEIGHT(frontsector) - viewz;

    midtexture = toptexture = bottomtexture = maskedtexture = 0;
    ds_p->maskedtexturecol = CLIP_NONE;

    boolean markfloor, markceiling;

    if (!backsector)
    {
        // single sided line
        midtexture = R_GetTextureTranslation(sidedef->midtexture);
        texmidtexture = R_GetTexCols(midtexture);

        // a single sided line is terminal, so it must mark ends
        markfloor = markceiling = true;

        if (maplineflags & ML_DONTPEGBOTTOM)
        {         // bottom of texture at bottom
            fixed_t vtop = SEC_FLOORHEIGHT(frontsector) + ((int32_t)(W_FetchInt16(&textureheight[sidedef->midtexture]) << TEXTURE_SHIFT) << FRACBITS);
            rw_midtexturemid = vtop - viewz;
        }
        else        // top of texture at top
            rw_midtexturemid = worldtop;

        rw_midtexturemid += ((int32_t)Mod(rowoffset, (W_FetchInt16(&textureheight[midtexture]) << TEXTURE_SHIFT))) << FRACBITS;

        ds_p->silhouette = SIL_BOTH;
        ds_p->sprtopclip = CLIP_SCREEN;
        ds_p->sprbottomclip = CLIP_NEGONE;
        ds_p->bsilheight = SIL_ALWAYS;
        ds_p->tsilheight = SIL_NEVER;
    }
    else      // two sided line
    {
        ds_p->sprtopclip = ds_p->sprbottomclip = CLIP_NONE;
        ds_p->silhouette = SIL_NONE;

        if(LN_BIT(LB_CLOSED, curseg.linenum))
        { /* cph - closed 2S line e.g. door */
            // cph - killough's (outdated) comment follows - this deals with both
            // "automap fixes", his and mine
            // killough 1/17/98: this test is required if the fix
            // for the automap bug (r_bsp.c) is used, or else some
            // sprites will be displayed behind closed doors. That
            // fix prevents lines behind closed doors with dropoffs
            // from being displayed on the automap.

            ds_p->silhouette = SIL_BOTH;
            ds_p->sprtopclip = CLIP_SCREEN;
            ds_p->sprbottomclip = CLIP_NEGONE;
            ds_p->bsilheight = SIL_ALWAYS;
            ds_p->tsilheight = SIL_NEVER;

        }
        else
        { /* not solid - old code */

            if (SEC_FLOORHEIGHT(frontsector) > SEC_FLOORHEIGHT(backsector))
            {
                ds_p->silhouette = SIL_BOTTOM;
                ds_p->bsilheight = frontsector->floorheight;
            }
            else
                if (SEC_FLOORHEIGHT(backsector) > viewz)
                {
                    ds_p->silhouette = SIL_BOTTOM;
                    ds_p->bsilheight = SIL_ALWAYS;
                }

            if (SEC_CEILINGHEIGHT(frontsector) < SEC_CEILINGHEIGHT(backsector))
            {
                ds_p->silhouette |= SIL_TOP;
                ds_p->tsilheight = frontsector->ceilingheight;
            }
            else
                if (SEC_CEILINGHEIGHT(backsector) < viewz)
                {
                    ds_p->silhouette |= SIL_TOP;
                    ds_p->tsilheight = SIL_NEVER;
                }
        }

        worldhigh = SEC_CEILINGHEIGHT(backsector) - viewz;
        worldlow = SEC_FLOORHEIGHT(backsector) - viewz;

        // hack to allow height changes in outdoor areas
        if (SEC_CEILINGSKY(frontsector) && SEC_CEILINGSKY(backsector))
            worldtop = worldhigh;

        markfloor = worldlow != worldbottom
                || backsector->floorpic != frontsector->floorpic
                || backsector->lightlevel != frontsector->lightlevel
                ;

        markceiling = worldhigh != worldtop
                || backsector->ceilingpic != frontsector->ceilingpic
                || backsector->lightlevel != frontsector->lightlevel
                ;

        if (SEC_CEILINGHEIGHT(backsector) <= SEC_FLOORHEIGHT(frontsector) || SEC_FLOORHEIGHT(backsector) >= SEC_CEILINGHEIGHT(frontsector))
            markceiling = markfloor = true;   // closed door

        if (worldhigh < worldtop)   // top texture
        {
            toptexture = R_GetTextureTranslation(sidedef->toptexture);
            textoptexture = R_GetTexCols(toptexture);
            rw_toptexturemid = maplineflags & ML_DONTPEGTOP ? worldtop :
                                                                        SEC_CEILINGHEIGHT(backsector) + ((int32_t)(W_FetchInt16(&textureheight[sidedef->toptexture]) << TEXTURE_SHIFT) << FRACBITS) - viewz;
            rw_toptexturemid += ((int32_t)Mod(rowoffset, (W_FetchInt16(&textureheight[toptexture]) << TEXTURE_SHIFT))) << FRACBITS;
        }

        if (worldlow > worldbottom) // bottom texture
        {
            bottomtexture = R_GetTextureTranslation(sidedef->bottomtexture);
            texbottomtexture = R_GetTexCols(bottomtexture);
            rw_bottomtexturemid = maplineflags & ML_DONTPEGBOTTOM ? worldtop : worldlow;

            rw_bottomtexturemid += ((int32_t)Mod(rowoffset, (W_FetchInt16(&textureheight[bottomtexture]) << TEXTURE_SHIFT))) << FRACBITS;
        }

        // allocate space for masked texture tables
        if (sidedef->midtexture)    // masked midtexture
        {
            maskedtexture = true;
            maskedtexturecol = lastopening - rw_x;
            ds_p->maskedtexturecol = maskedtexturecol - openings;
            lastopening += rw_stopx - rw_x;
        }
    }

    // calculate rw_offset (only needed for textured lines)
    boolean segtextured = ((midtexture | toptexture | bottomtexture | maskedtexture) > 0);

    if (segtextured)
    {
        fixed_t rw_offset32 = hyp * -finesineapprox(offsetangle >> ANGLETOFINESHIFT_16);
        rw_offset = rw_offset32 >> FRACBITS;
        rw_offset += sidedef->textureoffset + curseg.offset;

        rw_centerangle = ANG90_16 + viewangle16 - rw_normalangle;

        rw_lightlevel = frontsector->lightlevel;
    }

    // if a floor / ceiling plane is on the wrong side of the view
    // plane, it is definitely invisible and doesn't need to be marked.
    if (SEC_FLOORHEIGHT(frontsector) >= viewz)       // above view plane
        markfloor = false;
    if (SEC_CEILINGHEIGHT(frontsector) <= viewz &&
            !SEC_CEILINGSKY(frontsector))   // below view plane
        markceiling = false;

    // calculate incremental stepping values for texture edges
    topstep = -FixedMul(worldtop, rw_scalestep);
    topfrac = (CENTER_Y * FRACUNIT) - FixedMul(worldtop, rw_scale);

    bottomstep = -FixedMul(worldbottom, rw_scalestep);
    bottomfrac = (CENTER_Y * FRACUNIT) - FixedMul(worldbottom, rw_scale);

    if (backsector)
    {
        if (worldhigh < worldtop)
        {
            pixhigh = (CENTER_Y * FRACUNIT) - FixedMul(worldhigh, rw_scale);
            pixhighstep = -FixedMul(worldhigh, rw_scalestep);
        }
        if (worldlow > worldbottom)
        {
            pixlow = (CENTER_Y * FRACUNIT) - FixedMul(worldlow, rw_scale);
            pixlowstep = -FixedMul(worldlow, rw_scalestep);
        }
    }

    // render it
    if (markceiling)
    {
        if (ceilingplane_color == -1)
            markceiling = false;
    }

    if (markfloor)
    {
        if (floorplane_color == -1)
            markfloor = false;
    }

    didsolidcol = false;
    R_RenderSegLoop(rw_x, segtextured, markfloor, markceiling);

    /* cph - if a column was made solid by this wall, we _must_ save full clipping info */
    if (backsector && didsolidcol)
    {
        if (!(ds_p->silhouette & SIL_BOTTOM))
        {
            ds_p->silhouette |= SIL_BOTTOM;
            ds_p->bsilheight = backsector->floorheight;
        }
        if (!(ds_p->silhouette & SIL_TOP))
        {
            ds_p->silhouette |= SIL_TOP;
            ds_p->tsilheight = backsector->ceilingheight;
        }
    }

    // save sprite clipping info
    if ((ds_p->silhouette & SIL_TOP || maskedtexture) && ds_p->sprtopclip == CLIP_NONE)
    {
        memcpy((byte*)lastopening, (const byte*)(ceilingclip+start), sizeof(height_t)*(rw_stopx-start));
        ds_p->sprtopclip = (lastopening - openings) - start;
        lastopening += rw_stopx - start;
    }

    if ((ds_p->silhouette & SIL_BOTTOM || maskedtexture) && ds_p->sprbottomclip == CLIP_NONE)
    {
        memcpy((byte*)lastopening, (const byte*)(floorclip+start), sizeof(height_t)*(rw_stopx-start));
        ds_p->sprbottomclip = (lastopening - openings) - start;
        lastopening += rw_stopx - start;
    }

    if (maskedtexture && !(ds_p->silhouette & SIL_TOP))
    {
        ds_p->silhouette |= SIL_TOP;
        ds_p->tsilheight = SIL_NEVER;
    }

    if (maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
    {
        ds_p->silhouette |= SIL_BOTTOM;
        ds_p->bsilheight = SIL_ALWAYS;
    }

    ds_p++;
}


// killough 1/18/98 -- This function is used to fix the automap bug which
// showed lines behind closed doors simply because the door had a dropoff.
//
// cph - converted to R_RecalcLineFlags. This recalculates all the flags for
// a line, including closure and texture tiling.

static void R_RecalcLineFlags(int16_t n)
{
    side_t sidecopy;
    P_GetSide(curseg.sidenum, &sidecopy);
    const side_t* side = &sidecopy;

    LN_SETBIT(LB_DONE, n);
    LN_CLRBIT(LB_CLOSED, n);
    LN_CLRBIT(LB_IGNORE, n);

    /* First decide if the line is closed, normal, or invisible */
    if (!(maplineflags & ML_TWOSIDED)
            || SEC_CEILINGHEIGHT(backsector) <= SEC_FLOORHEIGHT(frontsector)
            || SEC_FLOORHEIGHT(backsector) >= SEC_CEILINGHEIGHT(frontsector)
            || (
                // if door is closed because back is shut:
                SEC_CEILINGHEIGHT(backsector) <= SEC_FLOORHEIGHT(backsector)

                // preserve a kind of transparent door/lift special effect:
                && (SEC_CEILINGHEIGHT(backsector) >= SEC_CEILINGHEIGHT(frontsector) ||
                    side->toptexture)

                && (SEC_FLOORHEIGHT(backsector) <= SEC_FLOORHEIGHT(frontsector) ||
                    side->bottomtexture)

                // properly render skies (consider door "open" if both ceilings are sky):
                && (!SEC_CEILINGSKY(backsector) ||
                    !SEC_CEILINGSKY(frontsector))
                )
            )
        LN_SETBIT(LB_CLOSED, n);
    else
    {
        // Reject empty lines used for triggers
        //  and special events.
        // Identical floor and ceiling on both sides,
        // identical light levels on both sides,
        // and no middle texture.
        // CPhipps - recode for speed, not certain if this is portable though
        if (SEC_CEILINGHEIGHT(backsector) != SEC_CEILINGHEIGHT(frontsector)
                || SEC_FLOORHEIGHT(backsector) != SEC_FLOORHEIGHT(frontsector)
                || side->midtexture
                || backsector->ceilingpic != frontsector->ceilingpic
                || backsector->floorpic != frontsector->floorpic
                || backsector->lightlevel != frontsector->lightlevel)
        {
        } else
            LN_SETBIT(LB_IGNORE, n);
    }
}


// CPhipps -
// R_ClipWallSegment
//
// Replaces the old R_Clip*WallSegment functions. It draws bits of walls in those
// columns which aren't solid, and updates the solidcol[] array appropriately

static void R_ClipWallSegment(int16_t first, int16_t last, const boolean solid)
{
    byte *p;
    while (first < last)
    {
        if (solidcol[first])
        {
            if (!(p = memchr(solidcol+first, 0, last-first)))
                return; // All solid

            first = p - solidcol;
        }
        else
        {
            int16_t to;
            if (!(p = memchr(solidcol+first, 1, last-first)))
                to = last;
            else
                to = p - solidcol;

            R_StoreWallRange(first, to-1);

            if (solid)
            {
                memset(solidcol + first, 1, to - first);
            }

            first = to;
        }
    }
}

//
// R_ClearClipSegs
//

//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//

static void R_AddLine(const seg_t* line)
{
    curline = line;
    W_Fetch(line, &curseg, sizeof(curseg));

    angle16_t angle1 = R_PointToAngle16(curseg.v1.x, curseg.v1.y);
    angle16_t angle2 = R_PointToAngle16(curseg.v2.x, curseg.v2.y);

    // Clip to view edges.
    angle16_t span = angle1 - angle2;

    // Back side, i.e. backface culling
    if (span >= ANG180_16)
        return;

    // Global angle needed by segcalc.
    rw_angle1 = angle1;
    angle1 -= viewangle16;
    angle2 -= viewangle16;

    angle16_t tspan = angle1 + clipangle;
    if (tspan > 2 * clipangle)
    {
        tspan -= 2 * clipangle;

        // Totally off the left edge?
        if (tspan >= span)
            return;

        angle1 = clipangle;
    }

    tspan = clipangle - angle2;
    if (tspan > 2 * clipangle)
    {
        tspan -= 2 * clipangle;

        // Totally off the left edge?
        if (tspan >= span)
            return;
        angle2 = -clipangle;
    }

    // The seg is in the view range,
    // but not necessarily visible.

    // killough 1/31/98: Here is where "slime trails" can SOMETIMES occur:
    viewx_t x1 = viewangletox((angle16_t)(angle1 + ANG90_16) >> ANGLETOFINESHIFT_16);
    viewx_t x2 = viewangletox((angle16_t)(angle2 + ANG90_16) >> ANGLETOFINESHIFT_16);

    // Does not cross a pixel?
    if (x1 >= x2)       // killough 1/31/98 -- change == to >= for robustness
        return;

    backsector = curseg.backsectornum != NO_INDEX8 ? &_g_sectors[curseg.backsectornum] : NULL;

    /* cph - roll up linedef properties in flags */
    maplineflags = W_FetchUInt8(&_g_maplines[curseg.linenum].flags);

    if (_g_linedonetic != (uint16_t)_g_gametic)
    {
        memset(&_g_linebits[LB_DONE * _g_linebytes], 0, _g_linebytes);
        _g_linedonetic = _g_gametic;
    }

    if (!LN_BIT(LB_DONE, curseg.linenum))
        R_RecalcLineFlags(curseg.linenum);

    if (!LN_BIT(LB_IGNORE, curseg.linenum))
    {
        R_ClipWallSegment (x1, x2, LN_BIT(LB_CLOSED, curseg.linenum) != 0);
    }
}

//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//

static void R_Subsector(int16_t num)
{
    int16_t         count;
    const seg_t      * line;
    mapsubsector_t mapsub;
    W_Fetch(&_g_mapsubsectors[num], &mapsub, sizeof(mapsub));

    frontsector = P_SubsectorSector(num);
    count = mapsub.numsegs;
    line = &_g_segs[mapsub.firstseg];

    if (SEC_FLOORHEIGHT(frontsector) < viewz)
        floorplane_color = R_GetPlaneColor(SEC_FLOORPIC(frontsector), frontsector->lightlevel);
    else
        floorplane_color = -1;


    if (SEC_CEILINGSKY(frontsector)) {
        ceilingplane_color = -2;
        R_LoadSkyPatch();
    } else if (SEC_CEILINGHEIGHT(frontsector) > viewz)
        ceilingplane_color = R_GetPlaneColor(SEC_CEILINGPIC(frontsector), frontsector->lightlevel);
    else
        ceilingplane_color = -1;

    R_AddSprites(num, frontsector->lightlevel);
    while (count--)
    {
        R_AddLine (line);
        line++;
        curline = NULL; /* cph 2001/11/18 - must clear curline now we're done with it, so R_LoadColorMap doesn't try using it for other things */
    }
}

//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//

static const byte checkcoord[12][4] =
{
  {3,0,2,1},
  {3,0,2,0},
  {3,1,2,0},
  {0},
  {2,0,2,1},
  {0,0,0,0},
  {3,1,3,0},
  {0},
  {2,0,3,1},
  {2,1,3,1},
  {2,1,3,0}
};


static boolean R_CheckBBox(const int16_t* bspcoord)
{
    // Find the corners of the box
    // that define the edges from current viewpoint.
    int16_t boxpos = (viewx <= ((fixed_t)bspcoord[BOXLEFT]<<FRACBITS) ? 0 : viewx < ((fixed_t)bspcoord[BOXRIGHT]<<FRACBITS) ? 1 : 2) +
            (viewy >= ((fixed_t)bspcoord[BOXTOP]<<FRACBITS) ? 0 : viewy > ((fixed_t)bspcoord[BOXBOTTOM]<<FRACBITS) ? 4 : 8);

    if (boxpos == 5)
        return true;

    const byte* check = checkcoord[boxpos];
    angle16_t angle1 = R_PointToAngle16(bspcoord[check[0]], bspcoord[check[1]]) - viewangle16;
    angle16_t angle2 = R_PointToAngle16(bspcoord[check[2]], bspcoord[check[3]]) - viewangle16;


    // cph - replaced old code, which was unclear and badly commented
    // Much more efficient code now
    if ((int16_t)angle1 < (int16_t)angle2)
    { /* it's "behind" us */
        /* Either angle1 or angle2 is behind us, so it doesn't matter if we
     * change it to the corect sign
     */
        if (ANG180_16 <= angle1 && angle1 < ANG270_16)
            angle1 = INT16_MAX; /* which is ANG180_16 - 1 */
        else
            angle2 = INT16_MIN;
    }

    if ((int16_t)angle2 >=  (int16_t)clipangle) return false; // Both off left edge
    if ((int16_t)angle1 <= -(int16_t)clipangle) return false; // Both off right edge
    if ((int16_t)angle1 >=  (int16_t)clipangle) angle1 =  clipangle; // Clip at left edge
    if ((int16_t)angle2 <= -(int16_t)clipangle) angle2 = -clipangle; // Clip at right edge

    // Find the first clippost
    //  that touches the source post
    //  (adjacent pixels are touching).

    viewx_t sx1 = viewangletox((angle16_t)(angle1 + ANG90_16) >> ANGLETOFINESHIFT_16);
    viewx_t sx2 = viewangletox((angle16_t)(angle2 + ANG90_16) >> ANGLETOFINESHIFT_16);
    //    const cliprange_t *start;

    // Does not cross a pixel.
    if (sx1 == sx2)
        return false;

    if (!memchr(solidcol+sx1, 0, sx2-sx1)) return false;
    // All columns it covers are already solidly covered


    return true;
}

//Render a BSP subsector if bspnum is a leaf node.
//Return false if bspnum is frame node.





static boolean R_RenderBspSubsector(int16_t bspnum)
{
    // Found a subsector?
    if (bspnum & NF_SUBSECTOR)
    {
        if (bspnum == -1)
            R_Subsector (0);
        else
            R_Subsector (bspnum & (~NF_SUBSECTOR));

        return true;
    }

    return false;
}

// RenderBSPNode
// Renders all subsectors below a given node,
//  walking the tree with a stack of back sides.
// Just call with BSP root.

static NOINLINE int16_t R_FetchBSPNode(int16_t bspnum, int16_t* back, int16_t* backbox)
{
	mapnode_t bsp;
	W_Fetch(&nodes[bspnum], &bsp, sizeof(bsp));

//
// decide which side the view point is on
//
	int16_t side = R_PointOnSide(viewx, viewy, &bsp);

	*back = bsp.children[side ^ 1];
	memcpy(backbox, bsp.bbox[side ^ 1], sizeof(bsp.bbox[0]));
	return bsp.children[side];
}

static void R_RenderBSPNode(int16_t bspnum)
{
	int16_t back[MAX_BSP_DEPTH];
	int16_t backbox[MAX_BSP_DEPTH][4];
	int16_t sp = 0;

	while (true)
	{
		while (!R_RenderBspSubsector(bspnum))
		{
			if (sp == MAX_BSP_DEPTH)
				I_Error("R_RenderBSPNode: deeper than %i", MAX_BSP_DEPTH);
			bspnum = R_FetchBSPNode(bspnum, &back[sp], backbox[sp]);
			sp++;
		}
		do
		{
			if (sp == 0)
				return;
			sp--;
		} while (!R_CheckBBox(backbox[sp]));
		bspnum = back[sp];
	}
}


static void R_ClearDrawSegs(void)
{
    ds_p = _s_drawsegs;
}

static void R_ClearClipSegs (void)
{
    memset(solidcol, 0, VIEW_WINDOW_WIDTH);
}


//
// R_SetupFrame
//

static void R_SetupFrame (player_t *player)
{
    viewx = player->mo->x;
    viewy = player->mo->y;
    viewz = player->viewz;
    viewangle = player->mo->angle;
    viewangle16 = viewangle >> FRACBITS;

    extralight = player->extralight;

    viewsin = finesineapprox(  viewangle16 >> ANGLETOFINESHIFT_16);
    viewcos = finecosineapprox(viewangle16 >> ANGLETOFINESHIFT_16);

    if (player->fixedcolormap)
    {
        fixedcolormap = fullcolormap   // killough 3/20/98: use fullcolormap
                + player->fixedcolormap*256;
    }
    else
        fixedcolormap = NULL;

    P_NextValidCount();
}


//
// R_RenderView
//
void R_RenderPlayerView (player_t* player)
{
    R_SetupFrame (player);

    // Clear buffers.
    R_ClearClipSegs ();
    R_ClearDrawSegs ();
    R_ClearOpeningClippingDetermination ();


    R_ClearOpenings ();
    R_ClearSprites ();

    // The head node is the last node output.
    R_RenderBSPNode (numnodes-1);

    R_FreeSkyPatch ();

    R_DrawMasked ();
}




// ArcTan LUT,
//  maps tan(angle) to angle fast. Gotta search.
//
// Effective size is 2049;
// The +1 size is to handle the case when x==y without additional checking.
static const angle_t tantoangleTable[2049] =
{
    0,333772,667544,1001315,1335086,1668857,2002626,2336395,
    2670163,3003929,3337694,3671457,4005219,4338979,4672736,5006492,
    5340245,5673995,6007743,6341488,6675230,7008968,7342704,7676435,
    8010164,8343888,8677609,9011325,9345037,9678744,10012447,10346145,
    10679838,11013526,11347209,11680887,12014558,12348225,12681885,13015539,
    13349187,13682829,14016464,14350092,14683714,15017328,15350936,15684536,
    16018129,16351714,16685291,17018860,17352422,17685974,18019518,18353054,
    18686582,19020100,19353610,19687110,20020600,20354080,20687552,21021014,
    21354466,21687906,22021338,22354758,22688168,23021568,23354956,23688332,
    24021698,24355052,24688396,25021726,25355046,25688352,26021648,26354930,
    26688200,27021456,27354702,27687932,28021150,28354356,28687548,29020724,
    29353888,29687038,30020174,30353296,30686404,31019496,31352574,31685636,
    32018684,32351718,32684734,33017736,33350722,33683692,34016648,34349584,
    34682508,35015412,35348300,35681172,36014028,36346868,36679688,37012492,
    37345276,37678044,38010792,38343524,38676240,39008936,39341612,39674272,
    40006912,40339532,40672132,41004716,41337276,41669820,42002344,42334848,
    42667332,42999796,43332236,43664660,43997060,44329444,44661800,44994140,
    45326456,45658752,45991028,46323280,46655512,46987720,47319908,47652072,
    47984212,48316332,48648428,48980500,49312548,49644576,49976580,50308556,
    50640512,50972444,51304352,51636236,51968096,52299928,52631740,52963524,
    53295284,53627020,53958728,54290412,54622068,54953704,55285308,55616888,
    55948444,56279972,56611472,56942948,57274396,57605816,57937212,58268576,
    58599916,58931228,59262512,59593768,59924992,60256192,60587364,60918508,
    61249620,61580704,61911760,62242788,62573788,62904756,63235692,63566604,
    63897480,64228332,64559148,64889940,65220696,65551424,65882120,66212788,
    66543420,66874024,67204600,67535136,67865648,68196120,68526568,68856984,
    69187360,69517712,69848024,70178304,70508560,70838776,71168960,71499112,
    71829224,72159312,72489360,72819376,73149360,73479304,73809216,74139096,
    74468936,74798744,75128520,75458264,75787968,76117632,76447264,76776864,
    77106424,77435952,77765440,78094888,78424304,78753688,79083032,79412336,
    79741608,80070840,80400032,80729192,81058312,81387392,81716432,82045440,
    82374408,82703336,83032224,83361080,83689896,84018664,84347400,84676096,
    85004760,85333376,85661952,85990488,86318984,86647448,86975864,87304240,
    87632576,87960872,88289128,88617344,88945520,89273648,89601736,89929792,
    90257792,90585760,90913688,91241568,91569408,91897200,92224960,92552672,
    92880336,93207968,93535552,93863088,94190584,94518040,94845448,95172816,
    95500136,95827416,96154648,96481832,96808976,97136080,97463136,97790144,
    98117112,98444032,98770904,99097736,99424520,99751256,100077944,100404592,
    100731192,101057744,101384248,101710712,102037128,102363488,102689808,103016080,
    103342312,103668488,103994616,104320696,104646736,104972720,105298656,105624552,
    105950392,106276184,106601928,106927624,107253272,107578872,107904416,108229920,
    108555368,108880768,109206120,109531416,109856664,110181872,110507016,110832120,
    111157168,111482168,111807112,112132008,112456856,112781648,113106392,113431080,
    113755720,114080312,114404848,114729328,115053760,115378136,115702464,116026744,
    116350960,116675128,116999248,117323312,117647320,117971272,118295176,118619024,
    118942816,119266560,119590248,119913880,120237456,120560984,120884456,121207864,
    121531224,121854528,122177784,122500976,122824112,123147200,123470224,123793200,
    124116120,124438976,124761784,125084528,125407224,125729856,126052432,126374960,
    126697424,127019832,127342184,127664472,127986712,128308888,128631008,128953072,
    129275080,129597024,129918912,130240744,130562520,130884232,131205888,131527480,
    131849016,132170496,132491912,132813272,133134576,133455816,133776992,134098120,
    134419184,134740176,135061120,135382000,135702816,136023584,136344272,136664912,
    136985488,137306016,137626464,137946864,138267184,138587456,138907664,139227808,
    139547904,139867920,140187888,140507776,140827616,141147392,141467104,141786752,
    142106336,142425856,142745312,143064720,143384048,143703312,144022512,144341664,
    144660736,144979744,145298704,145617584,145936400,146255168,146573856,146892480,
    147211040,147529536,147847968,148166336,148484640,148802880,149121056,149439152,
    149757200,150075168,150393072,150710912,151028688,151346400,151664048,151981616,
    152299136,152616576,152933952,153251264,153568496,153885680,154202784,154519824,
    154836784,155153696,155470528,155787296,156104000,156420624,156737200,157053696,
    157370112,157686480,158002768,158318976,158635136,158951216,159267232,159583168,
    159899040,160214848,160530592,160846256,161161840,161477376,161792832,162108208,
    162423520,162738768,163053952,163369040,163684080,163999040,164313936,164628752,
    164943504,165258176,165572784,165887312,166201776,166516160,166830480,167144736,
    167458912,167773008,168087040,168400992,168714880,169028688,169342432,169656096,
    169969696,170283216,170596672,170910032,171223344,171536576,171849728,172162800,
    172475808,172788736,173101600,173414384,173727104,174039728,174352288,174664784,
    174977200,175289536,175601792,175913984,176226096,176538144,176850096,177161984,
    177473792,177785536,178097200,178408784,178720288,179031728,179343088,179654368,
    179965568,180276704,180587744,180898720,181209616,181520448,181831184,182141856,
    182452448,182762960,183073408,183383760,183694048,184004240,184314368,184624416,
    184934400,185244288,185554096,185863840,186173504,186483072,186792576,187102000,
    187411344,187720608,188029808,188338912,188647936,188956896,189265760,189574560,
    189883264,190191904,190500448,190808928,191117312,191425632,191733872,192042016,
    192350096,192658096,192966000,193273840,193581584,193889264,194196848,194504352,
    194811792,195119136,195426400,195733584,196040688,196347712,196654656,196961520,
    197268304,197574992,197881616,198188144,198494592,198800960,199107248,199413456,
    199719584,200025616,200331584,200637456,200943248,201248960,201554576,201860128,
    202165584,202470960,202776256,203081456,203386592,203691632,203996592,204301472,
    204606256,204910976,205215600,205520144,205824592,206128960,206433248,206737456,
    207041584,207345616,207649568,207953424,208257216,208560912,208864512,209168048,
    209471488,209774832,210078112,210381296,210684384,210987408,211290336,211593184,
    211895936,212198608,212501184,212803680,213106096,213408432,213710672,214012816,
    214314880,214616864,214918768,215220576,215522288,215823920,216125472,216426928,
    216728304,217029584,217330784,217631904,217932928,218233856,218534704,218835472,
    219136144,219436720,219737216,220037632,220337952,220638192,220938336,221238384,
    221538352,221838240,222138032,222437728,222737344,223036880,223336304,223635664,
    223934912,224234096,224533168,224832160,225131072,225429872,225728608,226027232,
    226325776,226624240,226922608,227220880,227519056,227817152,228115168,228413088,
    228710912,229008640,229306288,229603840,229901312,230198688,230495968,230793152,
    231090256,231387280,231684192,231981024,232277760,232574416,232870960,233167440,
    233463808,233760096,234056288,234352384,234648384,234944304,235240128,235535872,
    235831504,236127056,236422512,236717888,237013152,237308336,237603424,237898416,
    238193328,238488144,238782864,239077488,239372016,239666464,239960816,240255072,
    240549232,240843312,241137280,241431168,241724960,242018656,242312256,242605776,
    242899200,243192512,243485744,243778896,244071936,244364880,244657744,244950496,
    245243168,245535744,245828224,246120608,246412912,246705104,246997216,247289216,
    247581136,247872960,248164688,248456320,248747856,249039296,249330640,249621904,
    249913056,250204128,250495088,250785968,251076736,251367424,251658016,251948512,
    252238912,252529200,252819408,253109520,253399536,253689456,253979280,254269008,
    254558640,254848176,255137632,255426976,255716224,256005376,256294432,256583392,
    256872256,257161024,257449696,257738272,258026752,258315136,258603424,258891600,
    259179696,259467696,259755600,260043392,260331104,260618704,260906224,261193632,
    261480960,261768176,262055296,262342320,262629248,262916080,263202816,263489456,
    263776000,264062432,264348784,264635024,264921168,265207216,265493168,265779024,
    266064784,266350448,266636000,266921472,267206832,267492096,267777264,268062336,
    268347312,268632192,268916960,269201632,269486208,269770688,270055072,270339360,
    270623552,270907616,271191616,271475488,271759296,272042976,272326560,272610048,
    272893440,273176736,273459936,273743040,274026048,274308928,274591744,274874432,
    275157024,275439520,275721920,276004224,276286432,276568512,276850528,277132416,
    277414240,277695936,277977536,278259040,278540448,278821728,279102944,279384032,
    279665056,279945952,280226752,280507456,280788064,281068544,281348960,281629248,
    281909472,282189568,282469568,282749440,283029248,283308960,283588544,283868032,
    284147424,284426720,284705920,284985024,285264000,285542912,285821696,286100384,
    286378976,286657440,286935840,287214112,287492320,287770400,288048384,288326240,
    288604032,288881696,289159264,289436768,289714112,289991392,290268576,290545632,
    290822592,291099456,291376224,291652896,291929440,292205888,292482272,292758528,
    293034656,293310720,293586656,293862496,294138240,294413888,294689440,294964864,
    295240192,295515424,295790560,296065600,296340512,296615360,296890080,297164704,
    297439200,297713632,297987936,298262144,298536256,298810240,299084160,299357952,
    299631648,299905248,300178720,300452128,300725408,300998592,301271680,301544640,
    301817536,302090304,302362976,302635520,302908000,303180352,303452608,303724768,
    303996800,304268768,304540608,304812320,305083968,305355520,305626944,305898272,
    306169472,306440608,306711616,306982528,307253344,307524064,307794656,308065152,
    308335552,308605856,308876032,309146112,309416096,309685984,309955744,310225408,
    310494976,310764448,311033824,311303072,311572224,311841280,312110208,312379040,
    312647776,312916416,313184960,313453376,313721696,313989920,314258016,314526016,
    314793920,315061728,315329408,315597024,315864512,316131872,316399168,316666336,
    316933408,317200384,317467232,317733984,318000640,318267200,318533632,318799968,
    319066208,319332352,319598368,319864288,320130112,320395808,320661408,320926912,
    321192320,321457632,321722816,321987904,322252864,322517760,322782528,323047200,
    323311744,323576192,323840544,324104800,324368928,324632992,324896928,325160736,
    325424448,325688096,325951584,326215008,326478304,326741504,327004608,327267584,
    327530464,327793248,328055904,328318496,328580960,328843296,329105568,329367712,
    329629760,329891680,330153536,330415264,330676864,330938400,331199808,331461120,
    331722304,331983392,332244384,332505280,332766048,333026752,333287296,333547776,
    333808128,334068384,334328544,334588576,334848512,335108352,335368064,335627712,
    335887200,336146624,336405920,336665120,336924224,337183200,337442112,337700864,
    337959552,338218112,338476576,338734944,338993184,339251328,339509376,339767296,
    340025120,340282848,340540480,340797984,341055392,341312704,341569888,341826976,
    342083968,342340832,342597600,342854272,343110848,343367296,343623648,343879904,
    344136032,344392064,344648000,344903808,345159520,345415136,345670656,345926048,
    346181344,346436512,346691616,346946592,347201440,347456224,347710880,347965440,
    348219872,348474208,348728448,348982592,349236608,349490528,349744320,349998048,
    350251648,350505152,350758528,351011808,351264992,351518048,351771040,352023872,
    352276640,352529280,352781824,353034272,353286592,353538816,353790944,354042944,
    354294880,354546656,354798368,355049952,355301440,355552800,355804096,356055264,
    356306304,356557280,356808128,357058848,357309504,357560032,357810464,358060768,
    358311008,358561088,358811104,359060992,359310784,359560480,359810048,360059520,
    360308896,360558144,360807296,361056352,361305312,361554144,361802880,362051488,
    362300032,362548448,362796736,363044960,363293056,363541024,363788928,364036704,
    364284384,364531936,364779392,365026752,365274016,365521152,365768192,366015136,
    366261952,366508672,366755296,367001792,367248192,367494496,367740704,367986784,
    368232768,368478656,368724416,368970080,369215648,369461088,369706432,369951680,
    370196800,370441824,370686752,370931584,371176288,371420896,371665408,371909792,
    372154080,372398272,372642336,372886304,373130176,373373952,373617600,373861152,
    374104608,374347936,374591168,374834304,375077312,375320224,375563040,375805760,
    376048352,376290848,376533248,376775520,377017696,377259776,377501728,377743584,
    377985344,378227008,378468544,378709984,378951328,379192544,379433664,379674688,
    379915584,380156416,380397088,380637696,380878176,381118560,381358848,381599040,
    381839104,382079072,382318912,382558656,382798304,383037856,383277280,383516640,
    383755840,383994976,384233984,384472896,384711712,384950400,385188992,385427488,
    385665888,385904160,386142336,386380384,386618368,386856224,387093984,387331616,
    387569152,387806592,388043936,388281152,388518272,388755296,388992224,389229024,
    389465728,389702336,389938816,390175200,390411488,390647680,390883744,391119712,
    391355584,391591328,391826976,392062528,392297984,392533312,392768544,393003680,
    393238720,393473632,393708448,393943168,394177760,394412256,394646656,394880960,
    395115136,395349216,395583200,395817088,396050848,396284512,396518080,396751520,
    396984864,397218112,397451264,397684288,397917248,398150080,398382784,398615424,
    398847936,399080320,399312640,399544832,399776928,400008928,400240832,400472608,
    400704288,400935872,401167328,401398720,401629984,401861120,402092192,402323136,
    402553984,402784736,403015360,403245888,403476320,403706656,403936896,404167008,
    404397024,404626944,404856736,405086432,405316032,405545536,405774912,406004224,
    406233408,406462464,406691456,406920320,407149088,407377760,407606336,407834784,
    408063136,408291392,408519520,408747584,408975520,409203360,409431072,409658720,
    409886240,410113664,410340992,410568192,410795296,411022304,411249216,411476032,
    411702720,411929312,412155808,412382176,412608480,412834656,413060736,413286720,
    413512576,413738336,413964000,414189568,414415040,414640384,414865632,415090784,
    415315840,415540800,415765632,415990368,416215008,416439552,416663968,416888288,
    417112512,417336640,417560672,417784576,418008384,418232096,418455712,418679200,
    418902624,419125920,419349120,419572192,419795200,420018080,420240864,420463552,
    420686144,420908608,421130976,421353280,421575424,421797504,422019488,422241344,
    422463104,422684768,422906336,423127776,423349120,423570400,423791520,424012576,
    424233536,424454368,424675104,424895744,425116288,425336736,425557056,425777280,
    425997408,426217440,426437376,426657184,426876928,427096544,427316064,427535488,
    427754784,427974016,428193120,428412128,428631040,428849856,429068544,429287168,
    429505664,429724064,429942368,430160576,430378656,430596672,430814560,431032352,
    431250048,431467616,431685120,431902496,432119808,432336992,432554080,432771040,
    432987936,433204736,433421408,433637984,433854464,434070848,434287104,434503296,
    434719360,434935360,435151232,435367008,435582656,435798240,436013696,436229088,
    436444352,436659520,436874592,437089568,437304416,437519200,437733856,437948416,
    438162880,438377248,438591520,438805696,439019744,439233728,439447584,439661344,
    439875008,440088576,440302048,440515392,440728672,440941824,441154880,441367872,
    441580736,441793472,442006144,442218720,442431168,442643552,442855808,443067968,
    443280032,443492000,443703872,443915648,444127296,444338880,444550336,444761696,
    444972992,445184160,445395232,445606176,445817056,446027840,446238496,446449088,
    446659552,446869920,447080192,447290400,447500448,447710432,447920320,448130112,
    448339776,448549376,448758848,448968224,449177536,449386720,449595808,449804800,
    450013664,450222464,450431168,450639776,450848256,451056640,451264960,451473152,
    451681248,451889248,452097152,452304960,452512672,452720288,452927808,453135232,
    453342528,453549760,453756864,453963904,454170816,454377632,454584384,454791008,
    454997536,455203968,455410304,455616544,455822688,456028704,456234656,456440512,
    456646240,456851904,457057472,457262912,457468256,457673536,457878688,458083744,
    458288736,458493600,458698368,458903040,459107616,459312096,459516480,459720768,
    459924960,460129056,460333056,460536960,460740736,460944448,461148064,461351584,
    461554976,461758304,461961536,462164640,462367680,462570592,462773440,462976160,
    463178816,463381344,463583776,463786144,463988384,464190560,464392608,464594560,
    464796448,464998208,465199872,465401472,465602944,465804320,466005600,466206816,
    466407904,466608896,466809824,467010624,467211328,467411936,467612480,467812896,
    468013216,468213440,468413600,468613632,468813568,469013440,469213184,469412832,
    469612416,469811872,470011232,470210528,470409696,470608800,470807776,471006688,
    471205472,471404192,471602784,471801312,471999712,472198048,472396288,472594400,
    472792448,472990400,473188256,473385984,473583648,473781216,473978688,474176064,
    474373344,474570528,474767616,474964608,475161504,475358336,475555040,475751648,
    475948192,476144608,476340928,476537184,476733312,476929376,477125344,477321184,
    477516960,477712640,477908224,478103712,478299104,478494400,478689600,478884704,
    479079744,479274656,479469504,479664224,479858880,480053408,480247872,480442240,
    480636512,480830656,481024736,481218752,481412640,481606432,481800128,481993760,
    482187264,482380704,482574016,482767264,482960416,483153472,483346432,483539296,
    483732064,483924768,484117344,484309856,484502240,484694560,484886784,485078912,
    485270944,485462880,485654720,485846464,486038144,486229696,486421184,486612576,
    486803840,486995040,487186176,487377184,487568096,487758912,487949664,488140320,
    488330880,488521312,488711712,488901984,489092160,489282240,489472256,489662176,
    489851968,490041696,490231328,490420896,490610336,490799712,490988960,491178144,
    491367232,491556224,491745120,491933920,492122656,492311264,492499808,492688256,
    492876608,493064864,493253056,493441120,493629120,493817024,494004832,494192544,
    494380160,494567712,494755136,494942496,495129760,495316928,495504000,495691008,
    495877888,496064704,496251424,496438048,496624608,496811040,496997408,497183680,
    497369856,497555936,497741920,497927840,498113632,498299360,498484992,498670560,
    498856000,499041376,499226656,499411840,499596928,499781920,499966848,500151680,
    500336416,500521056,500705600,500890080,501074464,501258752,501442944,501627040,
    501811072,501995008,502178848,502362592,502546240,502729824,502913312,503096704,
    503280000,503463232,503646368,503829408,504012352,504195200,504377984,504560672,
    504743264,504925760,505108192,505290496,505472736,505654912,505836960,506018944,
    506200832,506382624,506564320,506745952,506927488,507108928,507290272,507471552,
    507652736,507833824,508014816,508195744,508376576,508557312,508737952,508918528,
    509099008,509279392,509459680,509639904,509820032,510000064,510180000,510359872,
    510539648,510719328,510898944,511078432,511257856,511437216,511616448,511795616,
    511974688,512153664,512332576,512511392,512690112,512868768,513047296,513225792,
    513404160,513582432,513760640,513938784,514116800,514294752,514472608,514650368,
    514828064,515005664,515183168,515360608,515537952,515715200,515892352,516069440,
    516246432,516423328,516600160,516776896,516953536,517130112,517306592,517482976,
    517659264,517835488,518011616,518187680,518363648,518539520,518715296,518891008,
    519066624,519242144,519417600,519592960,519768256,519943424,520118528,520293568,
    520468480,520643328,520818112,520992800,521167392,521341888,521516320,521690656,
    521864896,522039072,522213152,522387168,522561056,522734912,522908640,523082304,
    523255872,523429376,523602784,523776096,523949312,524122464,524295552,524468512,
    524641440,524814240,524986976,525159616,525332192,525504640,525677056,525849344,
    526021568,526193728,526365792,526537760,526709632,526881440,527053152,527224800,
    527396352,527567840,527739200,527910528,528081728,528252864,528423936,528594880,
    528765760,528936576,529107296,529277920,529448480,529618944,529789344,529959648,
    530129856,530300000,530470048,530640000,530809888,530979712,531149440,531319072,
    531488608,531658080,531827488,531996800,532166016,532335168,532504224,532673184,
    532842080,533010912,533179616,533348288,533516832,533685312,533853728,534022048,
    534190272,534358432,534526496,534694496,534862400,535030240,535197984,535365632,
    535533216,535700704,535868128,536035456,536202720,536369888,536536992,536704000,
    536870912
};
