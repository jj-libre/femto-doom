/*-----------------------------------------------------------------------------
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
 *      Thing types, their states and sprites.
 *      Written by tools/femtowad from src/femtowad/info.py; edit that and run
 *      `uv run --project tools/femtowad femtowad generate`.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __INFO__
#define __INFO__

#include "d_think.h"
#include "sounds.h"

enum
{
  SPR_TROO,
  SPR_SHTG,
  SPR_PUNG,
  SPR_PISG,
  SPR_PISF,
  SPR_SHTF,
  SPR_CHGG,
  SPR_CHGF,
  SPR_MISG,
  SPR_MISF,
  SPR_SAWG,
  SPR_BLUD,
  SPR_PUFF,
  SPR_BAL1,
  SPR_MISL,
  SPR_TFOG,
  SPR_PLAY,
  SPR_POSS,
  SPR_SPOS,
  SPR_SARG,
  SPR_BAL7,
  SPR_BOSS,
  SPR_ARM1,
  SPR_ARM2,
  SPR_BAR1,
  SPR_BEXP,
  SPR_BON1,
  SPR_BON2,
  SPR_BKEY,
  SPR_RKEY,
  SPR_YKEY,
  SPR_STIM,
  SPR_MEDI,
  SPR_SOUL,
  SPR_PINS,
  SPR_SUIT,
  SPR_PMAP,
  SPR_PVIS,
  SPR_CLIP,
  SPR_AMMO,
  SPR_ROCK,
  SPR_BROK,
  SPR_SHEL,
  SPR_SBOX,
  SPR_BPAK,
  SPR_MGUN,
  SPR_CSAW,
  SPR_LAUN,
  SPR_SHOT,
  SPR_COLU,
  SPR_POL5,
  SPR_CAND,
  SPR_CBRA,
  SPR_ELEC,
  SPR_TRED,
  NUMSPRITES
};

typedef int8_t spritenum_t;

typedef enum
{
  S_NULL,
  S_LIGHTDONE,
  S_PUNCH,
  S_PUNCHDOWN,
  S_PUNCHUP,
  S_PUNCH1,
  S_PUNCH2,
  S_PUNCH3,
  S_PUNCH4,
  S_PUNCH5,
  S_PISTOL,
  S_PISTOLDOWN,
  S_PISTOLUP,
  S_PISTOL1,
  S_PISTOL2,
  S_PISTOL3,
  S_PISTOL4,
  S_PISTOLFLASH,
  S_SGUN,
  S_SGUNDOWN,
  S_SGUNUP,
  S_SGUN1,
  S_SGUN2,
  S_SGUN3,
  S_SGUN4,
  S_SGUN5,
  S_SGUN6,
  S_SGUN7,
  S_SGUN8,
  S_SGUN9,
  S_SGUNFLASH1,
  S_SGUNFLASH2,
  S_CHAIN,
  S_CHAINDOWN,
  S_CHAINUP,
  S_CHAIN1,
  S_CHAIN2,
  S_CHAIN3,
  S_CHAINFLASH1,
  S_CHAINFLASH2,
  S_MISSILE,
  S_MISSILEDOWN,
  S_MISSILEUP,
  S_MISSILE1,
  S_MISSILE2,
  S_MISSILE3,
  S_MISSILEFLASH1,
  S_MISSILEFLASH2,
  S_MISSILEFLASH3,
  S_MISSILEFLASH4,
  S_SAW,
  S_SAWB,
  S_SAWDOWN,
  S_SAWUP,
  S_SAW1,
  S_SAW2,
  S_SAW3,
  S_BLOOD1,
  S_BLOOD2,
  S_BLOOD3,
  S_PUFF1,
  S_PUFF2,
  S_PUFF3,
  S_PUFF4,
  S_TBALL1,
  S_TBALL2,
  S_TBALLX1,
  S_TBALLX2,
  S_TBALLX3,
  S_ROCKET,
  S_EXPLODE1,
  S_EXPLODE2,
  S_EXPLODE3,
  S_TFOG,
  S_TFOG01,
  S_TFOG02,
  S_TFOG2,
  S_TFOG3,
  S_TFOG4,
  S_TFOG5,
  S_TFOG6,
  S_TFOG7,
  S_TFOG8,
  S_TFOG9,
  S_TFOG10,
  S_PLAY,
  S_PLAY_RUN1,
  S_PLAY_RUN2,
  S_PLAY_RUN3,
  S_PLAY_RUN4,
  S_PLAY_ATK1,
  S_PLAY_ATK2,
  S_PLAY_PAIN,
  S_PLAY_PAIN2,
  S_PLAY_DIE1,
  S_PLAY_DIE2,
  S_PLAY_DIE3,
  S_PLAY_DIE4,
  S_PLAY_DIE5,
  S_PLAY_DIE6,
  S_PLAY_DIE7,
  S_PLAY_XDIE1,
  S_PLAY_XDIE2,
  S_PLAY_XDIE3,
  S_PLAY_XDIE4,
  S_PLAY_XDIE5,
  S_PLAY_XDIE6,
  S_PLAY_XDIE7,
  S_PLAY_XDIE8,
  S_PLAY_XDIE9,
  S_POSS_STND,
  S_POSS_STND2,
  S_POSS_RUN1,
  S_POSS_RUN2,
  S_POSS_RUN3,
  S_POSS_RUN4,
  S_POSS_RUN5,
  S_POSS_RUN6,
  S_POSS_RUN7,
  S_POSS_RUN8,
  S_POSS_ATK1,
  S_POSS_ATK2,
  S_POSS_ATK3,
  S_POSS_PAIN,
  S_POSS_PAIN2,
  S_POSS_DIE1,
  S_POSS_DIE2,
  S_POSS_DIE3,
  S_POSS_DIE4,
  S_POSS_DIE5,
  S_POSS_XDIE1,
  S_POSS_XDIE2,
  S_POSS_XDIE3,
  S_POSS_XDIE4,
  S_POSS_XDIE5,
  S_POSS_XDIE6,
  S_POSS_XDIE7,
  S_POSS_XDIE8,
  S_POSS_XDIE9,
  S_SPOS_STND,
  S_SPOS_STND2,
  S_SPOS_RUN1,
  S_SPOS_RUN2,
  S_SPOS_RUN3,
  S_SPOS_RUN4,
  S_SPOS_RUN5,
  S_SPOS_RUN6,
  S_SPOS_RUN7,
  S_SPOS_RUN8,
  S_SPOS_ATK1,
  S_SPOS_ATK2,
  S_SPOS_ATK3,
  S_SPOS_PAIN,
  S_SPOS_PAIN2,
  S_SPOS_DIE1,
  S_SPOS_DIE2,
  S_SPOS_DIE3,
  S_SPOS_DIE4,
  S_SPOS_DIE5,
  S_SPOS_XDIE1,
  S_SPOS_XDIE2,
  S_SPOS_XDIE3,
  S_SPOS_XDIE4,
  S_SPOS_XDIE5,
  S_SPOS_XDIE6,
  S_SPOS_XDIE7,
  S_SPOS_XDIE8,
  S_SPOS_XDIE9,
  S_TROO_STND,
  S_TROO_STND2,
  S_TROO_RUN1,
  S_TROO_RUN2,
  S_TROO_RUN3,
  S_TROO_RUN4,
  S_TROO_RUN5,
  S_TROO_RUN6,
  S_TROO_RUN7,
  S_TROO_RUN8,
  S_TROO_ATK1,
  S_TROO_ATK2,
  S_TROO_ATK3,
  S_TROO_PAIN,
  S_TROO_PAIN2,
  S_TROO_DIE1,
  S_TROO_DIE2,
  S_TROO_DIE3,
  S_TROO_DIE4,
  S_TROO_DIE5,
  S_TROO_XDIE1,
  S_TROO_XDIE2,
  S_TROO_XDIE3,
  S_TROO_XDIE4,
  S_TROO_XDIE5,
  S_TROO_XDIE6,
  S_TROO_XDIE7,
  S_TROO_XDIE8,
  S_SARG_STND,
  S_SARG_STND2,
  S_SARG_RUN1,
  S_SARG_RUN2,
  S_SARG_RUN3,
  S_SARG_RUN4,
  S_SARG_RUN5,
  S_SARG_RUN6,
  S_SARG_RUN7,
  S_SARG_RUN8,
  S_SARG_ATK1,
  S_SARG_ATK2,
  S_SARG_ATK3,
  S_SARG_PAIN,
  S_SARG_PAIN2,
  S_SARG_DIE1,
  S_SARG_DIE2,
  S_SARG_DIE3,
  S_SARG_DIE4,
  S_SARG_DIE5,
  S_SARG_DIE6,
  S_BRBALL1,
  S_BRBALL2,
  S_BRBALLX1,
  S_BRBALLX2,
  S_BRBALLX3,
  S_BOSS_STND,
  S_BOSS_STND2,
  S_BOSS_RUN1,
  S_BOSS_RUN2,
  S_BOSS_RUN3,
  S_BOSS_RUN4,
  S_BOSS_RUN5,
  S_BOSS_RUN6,
  S_BOSS_RUN7,
  S_BOSS_RUN8,
  S_BOSS_ATK1,
  S_BOSS_ATK2,
  S_BOSS_ATK3,
  S_BOSS_PAIN,
  S_BOSS_PAIN2,
  S_BOSS_DIE1,
  S_BOSS_DIE2,
  S_BOSS_DIE3,
  S_BOSS_DIE4,
  S_BOSS_DIE5,
  S_BOSS_DIE6,
  S_BOSS_DIE7,
  S_ARM1,
  S_ARM1A,
  S_ARM2,
  S_ARM2A,
  S_BAR1,
  S_BAR2,
  S_BEXP,
  S_BEXP2,
  S_BEXP3,
  S_BEXP4,
  S_BEXP5,
  S_BON1,
  S_BON1A,
  S_BON1B,
  S_BON1C,
  S_BON1D,
  S_BON1E,
  S_BON2,
  S_BON2A,
  S_BON2B,
  S_BON2C,
  S_BON2D,
  S_BON2E,
  S_BKEY,
  S_BKEY2,
  S_RKEY,
  S_RKEY2,
  S_YKEY,
  S_YKEY2,
  S_STIM,
  S_MEDI,
  S_SOUL,
  S_SOUL2,
  S_SOUL3,
  S_SOUL4,
  S_SOUL5,
  S_SOUL6,
  S_PINS,
  S_PINS2,
  S_PINS3,
  S_PINS4,
  S_SUIT,
  S_PMAP,
  S_PMAP2,
  S_PMAP3,
  S_PMAP4,
  S_PMAP5,
  S_PMAP6,
  S_PVIS,
  S_PVIS2,
  S_CLIP,
  S_AMMO,
  S_ROCK,
  S_BROK,
  S_SHEL,
  S_SBOX,
  S_BPAK,
  S_MGUN,
  S_CSAW,
  S_LAUN,
  S_SHOT,
  S_COLU,
  S_GIBS,
  S_CANDLESTIK,
  S_CANDELABRA,
  S_TECHPILLAR,
  S_REDTORCH,
  S_REDTORCH2,
  S_REDTORCH3,
  S_REDTORCH4,
  NUMSTATES
} statenum_t;

enum
{
  AC_NULL,
  AC_Light0,
  AC_WeaponReady,
  AC_Lower,
  AC_Raise,
  AC_Punch,
  AC_ReFire,
  AC_FirePistol,
  AC_Light1,
  AC_FireShotgun,
  AC_Light2,
  AC_FireCGun,
  AC_GunFlash,
  AC_FireMissile,
  AC_Saw,
  AC_Explode,
  AC_Pain,
  AC_PlayerScream,
  AC_Fall,
  AC_XScream,
  AC_Look,
  AC_Chase,
  AC_FaceTarget,
  AC_PosAttack,
  AC_Scream,
  AC_SPosAttack,
  AC_TroopAttack,
  AC_SargAttack,
  AC_BruisAttack,
  AC_BossDeath,
  NUMACTIONS
};

extern const actionf_t stateactions[NUMACTIONS];

#define FF_FULLBRIGHT   0x80  /* flag in thing->frame */
#define FF_FRAMEMASK    0x7f

typedef struct
{
  spritenum_t sprite;       /* sprite number to show                       */
  uint8_t     frame;        /* which frame/subframe of the sprite is shown */
  int8_t      tics;         /* number of gametics this frame should last   */
  uint8_t     action;       /* index into stateactions, AC_NULL if none    */
  uint16_t    nextstate;    /* linked list pointer to next state or zero   */
} state_t;

extern const state_t  states[NUMSTATES];
extern const char* const sprnames[];

enum
{
  MT_PLAYER,
  MT_POSSESSED,
  MT_SHOTGUY,
  MT_TROOP,
  MT_SERGEANT,
  MT_SHADOWS,
  MT_BRUISER,
  MT_BRUISERSHOT,
  MT_BARREL,
  MT_TROOPSHOT,
  MT_ROCKET,
  MT_PUFF,
  MT_BLOOD,
  MT_TFOG,
  MT_TELEPORTMAN,
  MT_MISC0,
  MT_MISC1,
  MT_MISC2,
  MT_MISC3,
  MT_MISC4,
  MT_MISC5,
  MT_MISC6,
  MT_MISC10,
  MT_MISC11,
  MT_MISC12,
  MT_INS,
  MT_MISC14,
  MT_MISC15,
  MT_MISC16,
  MT_CLIP,
  MT_MISC17,
  MT_MISC18,
  MT_MISC19,
  MT_MISC22,
  MT_MISC23,
  MT_MISC24,
  MT_CHAINGUN,
  MT_MISC26,
  MT_MISC27,
  MT_SHOTGUN,
  MT_MISC31,
  MT_MISC43,
  MT_MISC48,
  MT_MISC49,
  MT_MISC50,
  MT_MISC62,
  MT_MISC68,
  MT_MISC69,
  MT_MISC71,
  MT_NOTHING,
  NUMMOBJTYPES
};

typedef int8_t mobjtype_t;

typedef struct
{
  int32_t speed;
  int32_t radius;
  int32_t height;
  uint32_t flags;         /* MF_ flags */
  int16_t doomednum;      /* thing number in maps */
  int16_t spawnhealth;
  int16_t reactiontime;   /* tics it waits after it wakes up before it attacks */
  int16_t mass;
  int16_t damage;         /* of a missile */
  uint16_t spawnstate;
  uint16_t seestate;      /* when it sees you or wakes up */
  uint16_t painstate;
  uint16_t meleestate;
  uint16_t missilestate;
  uint16_t deathstate;
  uint16_t xdeathstate;   /* gibbed */
  uint8_t painchance;     /* out of 256, the chance of going to painstate when hurt */
  uint8_t seesound;
  uint8_t attacksound;
  uint8_t painsound;
  uint8_t deathsound;
  uint8_t activesound;    /* made wandering around, with a chance of 3 in 256 */
} mobjinfo_t;

extern const mobjinfo_t mobjinfo[NUMMOBJTYPES];

// Call P_SpecialThing when touched.
#define MF_SPECIAL      0x00000001UL
// Blocks.
#define MF_SOLID        0x00000002UL
// Can be hit.
#define MF_SHOOTABLE    0x00000004UL
// Don't use the sector links (invisible but touchable).
#define MF_NOSECTOR     0x00000008UL
// Don't use the blocklinks (inert but displayable)
#define MF_NOBLOCKMAP   0x00000010UL
// Not to be activated by sound, deaf monster.
#define MF_AMBUSH       0x00000020UL
// Will try to attack right back.
#define MF_JUSTHIT      0x00000040UL
// Will take at least one step before attacking.
#define MF_JUSTATTACKED 0x00000080UL
// Don't apply gravity (every tic), that is, object will float, keeping current height or changing it actively.
#define MF_NOGRAVITY    0x00000200UL
// This allows jumps from high places.
#define MF_DROPOFF      0x00000400UL
// For players, will pick up items.
#define MF_PICKUP       0x00000800UL
// Player cheat. ???
#define MF_NOCLIP       0x00001000UL
// Don't hit same species, explode on block. Player missiles as well as fireballs of various kinds.
#define MF_MISSILE      0x00010000UL
// Dropped by a demon, not level spawned. E.g. ammo clips dropped by dying former humans.
#define MF_DROPPED      0x00020000UL
// Use fuzzy draw (shadow demons or spectres), temporary player invisibility powerup.
#define MF_SHADOW       0x00040000UL
// Don't bleed when shot (use puff), barrels and shootable furniture shall not bleed.
#define MF_NOBLOOD      0x00080000UL
// Don't stop moving halfway off a step, that is, have dead bodies slide down all the way.
#define MF_CORPSE       0x00100000UL
// On kill, count this enemy object towards intermission kill total. Happy gathering.
#define MF_COUNTKILL    0x00400000UL
// On picking up, count this item object towards intermission item total.
#define MF_COUNTITEM    0x00800000UL
#define MF_POOLED       0x10000000UL
#define MF_STATIC       0x20000000UL
#define MF_REMOVED      0x40000000UL
// Still linked where the map put it, through the load lists; sprev is its load index.
#define MF_INPLACE      0x80000000UL

#define MAX_SPRITE_FRAMES 23

// The format of both WADs, kept in their FEMTOWAD lumps.
#define WAD_VERSION 2

#endif
